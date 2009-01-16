////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003-2006 Adam Strzelecki <ono+miranda@java.pl>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////

#include "gg.h"
#include <errno.h>

// Plugin info
PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
	"Gadu-Gadu Protocol",
	PLUGIN_MAKE_VERSION(0, 0, 4, 8),
	"Provides support for Gadu-Gadu protocol",
	"Adam Strzelecki",
	"ono+miranda@java.pl",
	"Copyright © 2003-2008 Adam Strzelecki",
	"http://www.miranda-im.pl/",
	0,
	0,
	// {F3FF65F3-250E-416A-BEE9-58C93F85AB33}
	{ 0xf3ff65f3, 0x250e, 0x416a, { 0xbe, 0xe9, 0x58, 0xc9, 0x3f, 0x85, 0xab, 0x33 } }
};
static const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};

// Other variables
HINSTANCE hInstance;
PLUGINLINK *pluginLink;
struct MM_INTERFACE mmi;

// Event hooks
static HANDLE hHookModulesLoaded = NULL;
static HANDLE hHookPreShutdown = NULL;

static unsigned long crc_table[256];

//////////////////////////////////////////////////////////
// Extra winsock function for error description
char *ws_strerror(int code)
{
	static char err_desc[160];

	// Not a windows error display WinSock
	if(code == 0)
	{
		char buff[128];
		int len;
		len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
				  NULL, WSAGetLastError(), 0, buff,
				  sizeof(buff), NULL);
		if(len == 0)
			mir_snprintf(err_desc, sizeof(err_desc), "WinSock %u: Unknown error.", WSAGetLastError());
		else
			mir_snprintf(err_desc, sizeof(err_desc), "WinSock %d: %s", WSAGetLastError(), buff);
		return err_desc;
	}

	// Return normal error
	return strerror(code);
}

//////////////////////////////////////////////////////////
// Build the crc table
void crc_gentable(void)
{
	unsigned long crc, poly;
	int	i, j;

	poly = 0xEDB88320L;
	for (i = 0; i < 256; i++)
	{
		crc = i;
		for (j = 8; j > 0; j--)
		{
			if (crc & 1)
				crc = (crc >> 1) ^ poly;
			else
				crc >>= 1;
		}
		crc_table[i] = crc;
	}
}

//////////////////////////////////////////////////////////
// Calculate the crc value
unsigned long crc_get(char *mem)
{
	register unsigned long crc = 0xFFFFFFFF;
	while(mem && *mem)
		crc = ((crc>>8) & 0x00FFFFFF) ^ crc_table[(crc ^ *(mem++)) & 0xFF];

	return (crc ^ 0xFFFFFFFF);
}

//////////////////////////////////////////////////////////
// http_error_string()
//
// returns http error text
const char *http_error_string(int h)
{
	switch (h)
	{
		case 0:
			return Translate((errno == ENOMEM) ? "HTTP failed memory" : "HTTP failed connecting");
		case GG_ERROR_RESOLVING:
			return Translate("HTTP failed resolving");
		case GG_ERROR_CONNECTING:
			return Translate("HTTP failed connecting");
		case GG_ERROR_READING:
			return Translate("HTTP failed reading");
		case GG_ERROR_WRITING:
			return Translate("HTTP failed writing");
	}

	return Translate("Unknown HTTP error");
}

//////////////////////////////////////////////////////////
// Gets plugin info
DWORD gMirandaVersion = 0;
// For compatibility with old versions
__declspec(dllexport) PLUGININFO *MirandaPluginInfo(DWORD mirandaVersion)
{
	pluginInfo.cbSize = sizeof(PLUGININFO);
	gMirandaVersion = mirandaVersion;
	return (PLUGININFO *)&pluginInfo;
}
__declspec(dllexport) PLUGININFOEX *MirandaPluginInfoEx(DWORD mirandaVersion)
{
	pluginInfo.cbSize = sizeof(PLUGININFOEX);
	gMirandaVersion = mirandaVersion;
	return &pluginInfo;
}
__declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}
void CreateProtoService(const char* szService, GGPROTOFUNC serviceProc, GGPROTO *gg)
{
	char str[MAXMODULELABELLENGTH];
	mir_snprintf(str, sizeof(str), "%s%s", gg->proto.m_szModuleName, szService);
	CreateServiceFunctionObj(str, (MIRANDASERVICEOBJ)serviceProc, gg);
}

//////////////////////////////////////////////////////////
// Cleanups from last plugin
void gg_cleanuplastplugin(GGPROTO *gg, DWORD version)
{
	HANDLE hContact;
	char *szProto;

	// Remove bad e-mail and phones from
	if(version < PLUGIN_MAKE_VERSION(0, 0, 1, 4))
	{
#ifdef DEBUGMODE
		gg_netlog(gg, "gg_cleanuplastplugin(%d): Cleaning junk Phone settings from < 0.0.1.4 ...", version);
#endif
		// Look for contact in DB
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		while (hContact)
		{
			szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
			if(szProto != NULL && !strcmp(szProto, GG_PROTO))
			{
				// Do contact cleanup
				DBDeleteContactSetting(hContact, GG_PROTO, GG_KEY_EMAIL);
				DBDeleteContactSetting(hContact, GG_PROTO, "Phone");
			}
			hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
		}
	}

	// Remove GG entries for non GG contacts
	if(version < PLUGIN_MAKE_VERSION(0, 0, 3, 5))
	{
#ifdef DEBUGMODE
		gg_netlog(gg, "gg_cleanuplastplugin(%d): Cleaning junk Nick settings from < 0.0.3.5 ...", version);
#endif
		// Look for contact in DB
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		while (hContact)
		{
			szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
			if(szProto != NULL && strcmp(szProto, GG_PROTO))
			{
				// Do nick entry cleanup
				DBDeleteContactSetting(hContact, GG_PROTO, GG_KEY_NICK);
			}
			hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
		}
	}

	// Store this plugin version
	DBWriteContactSettingDword(NULL, GG_PROTO, GG_PLUGINVERSION, pluginInfo.version);
}

//////////////////////////////////////////////////////////
// When miranda loaded its modules
int gg_modulesloaded(WPARAM wParam, LPARAM lParam)
{
	// Init SSL library
	gg_ssl_init();

	return 0;
}

//////////////////////////////////////////////////////////
// When Miranda starting shutdown sequence
int gg_preshutdown(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

//////////////////////////////////////////////////////////
// Custom protocol event
int gg_event(PROTO_INTERFACE *proto, PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam)
{
	GGPROTO *gg = (GGPROTO *)proto;
	switch( eventType )
	{
		case EV_PROTO_ONLOAD:
		{
			NETLIBUSER nlu = { 0 };

			nlu.cbSize = sizeof(nlu);
			nlu.flags = NUF_OUTGOING | NUF_INCOMING | NUF_HTTPCONNS;
			nlu.szSettingsModule = gg->proto.m_szModuleName;
			if (gg->unicode_core) {
				WCHAR name[128];
				_snwprintf(name, sizeof(name)/sizeof(name[0]), TranslateW(L"%s connection"), gg->proto.m_tszUserName);
				nlu.ptszDescriptiveName = (char *)name;
				nlu.flags |= NUF_UNICODE;
			} else {
				char name[128];
				mir_snprintf(name, sizeof(name)/sizeof(name[0]), Translate("%s connection"), gg->proto.m_tszUserName);
				nlu.ptszDescriptiveName = name;
			}

			gg->netlib = (HANDLE) CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM) & nlu);
			gg->hookOptsInit = HookProtoEvent(ME_OPT_INITIALISE, gg_options_init, gg);
			gg->hookUserInfoInit = HookProtoEvent(ME_USERINFO_INITIALISE, gg_details_init, gg);
			gg->hookSettingDeleted = HookProtoEvent(ME_DB_CONTACT_DELETED, gg_userdeleted, gg);
			gg->hookSettingChanged = HookProtoEvent(ME_DB_CONTACT_SETTINGCHANGED, gg_dbsettingchanged, gg);
			gg->hookIconsChanged = HookProtoEvent(ME_SKIN2_ICONSCHANGED, gg_iconschanged, gg);
#ifdef DEBUGMODE
			gg_netlog(gg, "gg_event(EV_PROTO_ONLOAD): loading modules...");
#endif

			// Init misc thingies
			gg_icolib_init(gg);
			gg_gc_init(gg);
			gg_keepalive_init(gg);
			gg_import_init(gg);
			gg_img_init(gg);

			break;
		}
		case EV_PROTO_ONEXIT:
#ifdef DEBUGMODE
			gg_netlog(gg, "gg_event(EV_PROTO_ONEXIT)/gg_preshutdown(): signalling shutdown...");
#endif
			gg_img_shutdown(gg);
			break;
		/*
		case EV_PROTO_ONOPTIONS:
		case EV_PROTO_ONRENAME:
		{	
			CLISTMENUITEM clmi = { 0 };
			clmi.cbSize = sizeof( CLISTMENUITEM );
			clmi.flags = CMIM_NAME | CMIF_TCHAR;
			clmi.ptszName = m_tszUserName;
			CallService(MS_CLIST_MODIFYMENUITEM, ( WPARAM )mainMenuRoot, ( LPARAM )&clmi);
		}
		*/	
	}
	return TRUE;
}

//////////////////////////////////////////////////////////
// Module instance initialization
PROTO_INTERFACE * gg_proto_init(const char* pszProtoName, const TCHAR* tszUserName)
{
	DWORD dwVersion;
	GGPROTO *gg = malloc(sizeof(GGPROTO));
	char szVer[MAX_PATH];
	ZeroMemory(gg, sizeof(GGPROTO));
	gg->proto.vtbl = malloc(sizeof(PROTO_INTERFACE_VTBL));
	// Are we running under unicode Miranda core ?
	CallService(MS_SYSTEM_GETVERSIONTEXT, MAX_PATH, (LPARAM)szVer);
	_strlwr(szVer); // make sure it is lowercase
	gg->unicode_core = (strstr(szVer, "unicode") != NULL);

	// Init mutex
	pthread_mutex_init(&gg->sess_mutex, NULL);
	pthread_mutex_init(&gg->ft_mutex, NULL);
	pthread_mutex_init(&gg->img_mutex, NULL);

	// Init instance names
	gg->proto.m_szModuleName = mir_strdup(pszProtoName);
	gg->proto.m_szProtoName = GGDEF_PROTONAME;

/* Anyway we won't get Unicode in GG yet */
#ifdef _UNICODE
	gg->name = gg->proto.m_tszUserName = mir_tstrdup(tszUserName);
#else
	gg->proto.m_tszUserName = gg->unicode_core ? (TCHAR *)mir_wstrdup((wchar_t *)tszUserName) : (TCHAR *)mir_strdup((char *)tszUserName);
	gg->name = gg->unicode_core ? mir_u2a((wchar_t *)tszUserName) : mir_strdup(tszUserName);
#endif

	// Register services
	gg_registerservices(gg);
	gg_setalloffline(gg);
	gg_refreshblockedicon(gg);

	if((dwVersion = DBGetContactSettingDword(NULL, GG_PROTO, GG_PLUGINVERSION, 0)) < pluginInfo.version)
		gg_cleanuplastplugin(gg, dwVersion);

	return (PROTO_INTERFACE *)gg;
}

static int gg_proto_uninit(PROTO_INTERFACE *proto)
{
	GGPROTO *gg = (GGPROTO *)proto;

#ifdef DEBUGMODE
	gg_netlog(gg, "gg_proto_uninit(): destroying protocol interface");
#endif

	// Destroy modules
	gg_img_destroy(gg);
	gg_keepalive_destroy(gg);
	gg_gc_destroy(gg);
	gg_import_shutdown(gg);

	// Close handles
	LocalEventUnhook(gg->hookOptsInit);
	LocalEventUnhook(gg->hookSettingDeleted);
	LocalEventUnhook(gg->hookSettingChanged);
	LocalEventUnhook(gg->hookIconsChanged);
	Netlib_CloseHandle(gg->netlib);

	// Destroy mutex
	pthread_mutex_destroy(&gg->sess_mutex);
	pthread_mutex_destroy(&gg->ft_mutex);
	pthread_mutex_destroy(&gg->img_mutex);

	// Free status messages
	if(gg->modemsg.online)    free(gg->modemsg.online);
	if(gg->modemsg.away)      free(gg->modemsg.away);
	if(gg->modemsg.invisible) free(gg->modemsg.invisible);
	if(gg->modemsg.offline)   free(gg->modemsg.offline);

	mir_free(gg->proto.m_szModuleName);
	mir_free(gg->proto.m_tszUserName);
#ifndef _UNICODE
	mir_free(gg->name);
#endif
	free(gg->proto.vtbl);
	free(gg);
	return 0;
}

//////////////////////////////////////////////////////////
// When plugin is loaded
int __declspec(dllexport) Load(PLUGINLINK * link)
{
	WSADATA wsaData;
	PROTOCOLDESCRIPTOR pd;

	pluginLink = link;
	mir_getMMI(&mmi);

	// Init winsock
	if (WSAStartup(MAKEWORD( 1, 1 ), &wsaData))
		return 1;

	// Hook system events
	hHookModulesLoaded = HookEvent(ME_SYSTEM_MODULESLOADED, gg_modulesloaded);
	hHookPreShutdown = HookEvent(ME_SYSTEM_PRESHUTDOWN, gg_preshutdown);

	// Prepare protocol name
	ZeroMemory(&pd, sizeof(pd));
	pd.cbSize = sizeof(pd);
	pd.szName = GGDEF_PROTO;
	pd.fnInit = gg_proto_init;
	pd.fnUninit = gg_proto_uninit;
	pd.type = PROTOTYPE_PROTOCOL;

	// Register module
	CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM) &pd);

	return 0;
}

//////////////////////////////////////////////////////////
// when plugin is unloaded
int __declspec(dllexport) Unload()
{
	LocalEventUnhook(hHookModulesLoaded);
	LocalEventUnhook(hHookPreShutdown);

	// Uninit SSL library
	gg_ssl_uninit();

	// Cleanup WinSock
	WSACleanup();

	return 0;
}


//////////////////////////////////////////////////////////
// DEBUGING FUNCTIONS

#ifdef DEBUGMODE
struct
{
	int type;
	char *text;
} ggdebug_eventype2string[] =
{
	{GG_EVENT_NONE,					"GG_EVENT_NONE"},
	{GG_EVENT_MSG,					"GG_EVENT_MSG"},
	{GG_EVENT_NOTIFY,				"GG_EVENT_NOTIFY"},
	{GG_EVENT_NOTIFY_DESCR,			"GG_EVENT_NOTIFY_DESCR"},
	{GG_EVENT_STATUS,				"GG_EVENT_STATUS"},
	{GG_EVENT_ACK,					"GG_EVENT_ACK"},
	{GG_EVENT_PONG,					"GG_EVENT_PONG"},
	{GG_EVENT_CONN_FAILED,			"GG_EVENT_CONN_FAILED"},
	{GG_EVENT_CONN_SUCCESS,			"GG_EVENT_CONN_SUCCESS"},
	{GG_EVENT_DISCONNECT,			"GG_EVENT_DISCONNECT"},
	{GG_EVENT_DCC_NEW,				"GG_EVENT_DCC_NEW"},
	{GG_EVENT_DCC_ERROR,			"GG_EVENT_DCC_ERROR"},
	{GG_EVENT_DCC_DONE,				"GG_EVENT_DCC_DONE"},
	{GG_EVENT_DCC_CLIENT_ACCEPT,	"GG_EVENT_DCC_CLIENT_ACCEPT"},
	{GG_EVENT_DCC_CALLBACK,			"GG_EVENT_DCC_CALLBACK"},
	{GG_EVENT_DCC_NEED_FILE_INFO,	"GG_EVENT_DCC_NEED_FILE_INFO"},
	{GG_EVENT_DCC_NEED_FILE_ACK,	"GG_EVENT_DCC_NEED_FILE_ACK"},
	{GG_EVENT_DCC_NEED_VOICE_ACK,	"GG_EVENT_DCC_NEED_VOICE_ACK"},
	{GG_EVENT_DCC_VOICE_DATA,		"GG_EVENT_DCC_VOICE_DATA"},
	{GG_EVENT_PUBDIR50_SEARCH_REPLY,"GG_EVENT_PUBDIR50_SEARCH_REPLY"},
	{GG_EVENT_PUBDIR50_READ,		"GG_EVENT_PUBDIR50_READ"},
	{GG_EVENT_PUBDIR50_WRITE,		"GG_EVENT_PUBDIR50_WRITE"},
	{GG_EVENT_STATUS60,				"GG_EVENT_STATUS60"},
	{GG_EVENT_NOTIFY60,				"GG_EVENT_NOTIFY60"},
	{GG_EVENT_USERLIST,				"GG_EVENT_USERLIST"},
	{GG_EVENT_IMAGE_REQUEST,		"GG_EVENT_IMAGE_REQUEST"},
	{GG_EVENT_IMAGE_REPLY,			"GG_EVENT_IMAGE_REPLY"},
	{GG_EVENT_DCC_ACK,				"GG_EVENT_DCC_ACK"},
	{-1,							"<unknown event>"}
};

const char *ggdebug_eventtype(struct gg_event *e)
{
	int i;
	for(i = 0; ggdebug_eventype2string[i].type != -1; i++)
		if(ggdebug_eventype2string[i].type == e->type)
			return ggdebug_eventype2string[i].text;
	return ggdebug_eventype2string[i].text;
}

void gg_debughandler(int level, const char *format, va_list ap)
{
	char szText[1024], *szFormat = _strdup(format);
	// Kill end line
	char *nl = strrchr(szFormat, '\n');
	if(nl) *nl = 0;

	strncpy(szText, "[libgadu] \0", sizeof(szText));

	mir_vsnprintf(szText + strlen(szText), sizeof(szText) - strlen(szText), szFormat, ap);
	CallService(MS_NETLIB_LOG, (WPARAM) NULL, (LPARAM) szText);
	free(szFormat);
}

////////////////////////////////////////////////////////////
// Log funcion
int gg_netlog(const GGPROTO *gg, const char *fmt, ...)
{
	va_list va;
	char szText[1024];

	mir_snprintf(szText, sizeof(szText), "[%s] ", GG_PROTO);

	va_start(va, fmt);
	mir_vsnprintf(szText + strlen(szText), sizeof(szText) - strlen(szText), fmt, va);
	va_end(va);
	return CallService(MS_NETLIB_LOG, (WPARAM) gg->netlib, (LPARAM) szText);
}

#endif

////////////////////////////////////////////////////////////////////////////
// Image dialog call thread
void gg_img_dlgcall(void *empty);
void *__stdcall gg_img_dlgthread(void *empty)
{
	gg_img_dlgcall(empty);
	return NULL;
}

//////////////////////////////////////////////////////////
// main DLL function
BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
	crc_gentable();
	hInstance = hInst;
#ifdef DEBUGMODE
	gg_debug_handler = gg_debughandler;
#endif
	return TRUE;
}
