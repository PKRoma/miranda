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
PLUGININFO pluginInfo = {
	sizeof(PLUGININFO),
	"Gadu-Gadu Protocol",
	PLUGIN_MAKE_VERSION(0, 0, 4, 1),
	"Provides support for Gadu-Gadu protocol",
	"Adam Strzelecki",
	"ono+miranda@java.pl",
	"Copyright © 2003-2006 Adam Strzelecki",
	"http://www.miranda.kom.pl/",
	0,
	0
};

// Other variables
PLUGINLINK *pluginLink;
HINSTANCE hInstance;
int ggStatus = ID_STATUS_OFFLINE;			// gadu-gadu status
int ggDesiredStatus = ID_STATUS_OFFLINE;	// gadu-gadu desired status
HANDLE hNetlib; 							// used just for logz
char ggProto[64] = GGDEF_PROTO;				// proto id get from DLL name	(def GG from GG.dll or GGdebug.dll)
											// (static variable because may be needed after destroy)
char *ggProtoName = NULL;					// proto name get from DLL name (def Gadu-Gadu from GG.dll or GGdebug.dll)
char *ggProtoError = NULL;					// proto error get from DLL name (def Gadu-Gadu from GG.dll or GGdebug.dll)

// Status messages
struct gg_status_msgs ggModeMsg;

// Event hooks
static HANDLE hHookOptsInit;
static HANDLE hHookUserInfoInit;
static HANDLE hHookModulesLoaded;
static HANDLE hHookPreShutdown;
static HANDLE hHookSettingDeleted;
static HANDLE hHookSettingChanged;

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
			snprintf(err_desc, sizeof(err_desc), "WinSock %u: Unknown error.", WSAGetLastError());
		else
			snprintf(err_desc, sizeof(err_desc), "WinSock %d: %s", WSAGetLastError(), buff);
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

void gg_refreshblockedicon()
{
	// Store blocked icon
	char strFmt1[MAX_PATH], strFmt2[MAX_PATH];
	GetModuleFileName(hInstance, strFmt1, sizeof(strFmt1));
	snprintf(strFmt2, sizeof(strFmt2), "%s,-%d", strFmt1, IDI_STOP);
	snprintf(strFmt1, sizeof(strFmt1), "%s%d", GG_PROTO, ID_STATUS_DND);
	DBWriteContactSettingString(NULL, "Icons", strFmt1, strFmt2);
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
__declspec(dllexport) PLUGININFO *MirandaPluginInfo(DWORD mirandaVersion)
{
	gMirandaVersion = mirandaVersion;
	return &pluginInfo;
}

//////////////////////////////////////////////////////////
// Cleanups from last plugin
void gg_cleanuplastplugin(DWORD version)
{
	HANDLE hContact;
	char *szProto;

	// Remove bad e-mail and phones from
	if(version < PLUGIN_MAKE_VERSION(0, 0, 1, 4))
	{
#ifdef DEBUGMODE
		gg_netlog("gg_cleanuplastplugin(%d): Cleaning junk Phone settings from < 0.0.1.4 ...", version);
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
		gg_netlog("gg_cleanuplastplugin(%d): Cleaning junk Nick settings from < 0.0.3.5 ...", version);
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
	NETLIBUSER nlu = { 0 };
	char *szTitle = NULL,
		 *szError = NULL,
		 *szConnection = NULL;
	DWORD dwVersion = 0;

	szConnection = Translate("connection");
	szTitle = malloc(strlen(GG_PROTONAME) + strlen(szConnection) + 2);
	strcpy(szTitle, GG_PROTONAME);
	strcat(szTitle, " ");
	strcat(szTitle, szConnection);

	nlu.cbSize = sizeof(nlu);
	nlu.flags = NUF_OUTGOING | NUF_INCOMING | NUF_HTTPCONNS;
	nlu.szSettingsModule = GG_PROTO;
	nlu.szDescriptiveName = szTitle;
	hNetlib = (HANDLE) CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM) & nlu);
	hHookOptsInit = HookEvent(ME_OPT_INITIALISE, gg_options_init);
	hHookUserInfoInit = HookEvent(ME_USERINFO_INITIALISE, gg_details_init);
	hHookSettingDeleted = HookEvent(ME_DB_CONTACT_DELETED, gg_userdeleted);
	hHookSettingChanged = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, gg_dbsettingchanged);

	free(szTitle);

	// Init SSL library
	gg_ssl_init();

	// Init misc thingies
	/* gg_userinfo_init(); DEPRECATED */
	gg_keepalive_init();
	gg_import_init();
	/* gg_chpass_init(); DEPRECATED */
	gg_img_init();
	gg_gc_init();

	// Make error message
	szError = Translate("Error");
	ggProtoError = malloc(strlen(GG_PROTONAME) + strlen(szError) + 2);
	strcpy(ggProtoError, GG_PROTONAME);
	strcat(ggProtoError, " ");
	strcat(ggProtoError, szError);

	// Do last plugin cleanup if not actual version
	if((dwVersion = DBGetContactSettingDword(NULL, GG_PROTO, GG_PLUGINVERSION, 0)) < pluginInfo.version)
		gg_cleanuplastplugin(dwVersion);

	return 0;
}

//////////////////////////////////////////////////////////
// When Miranda starting shutdown sequence
int gg_preshutdown(WPARAM wParam, LPARAM lParam)
{
#ifdef DEBUGMODE
	gg_netlog("gg_preshutdown(): signalling shutdown...");
#endif
	// Shutdown some modules before unload
	gg_img_shutdown();

	return 0;
}

//////////////////////////////////////////////////////////
// Init multiple instances proto name
void init_protonames()
{
	char text[MAX_PATH], *p, *q;
	WIN32_FIND_DATA ffd;
	HANDLE hFind;

	// Try to find name of the file having original letter sizes
	GetModuleFileName(hInstance, text, sizeof(text));
	if((hFind = FindFirstFile(text, &ffd)) != INVALID_HANDLE_VALUE)
	{
		strncpy(text, ffd.cFileName, sizeof(text));
		FindClose(hFind);
	}
	// Check if we have relative or full path
	if(p = strrchr(text, '\\'))
		p++;
	else
		p = text;
	if(q = strrchr(p, '.'))	*q = '\0';
	if((q = strstr(p, "debug")) && strlen(q) == 5)
		*q = '\0';

	// We copy to static variable
	strncpy(ggProto, p, sizeof(ggProto));
	strupr(ggProto);
	// Is it default GG.dll if yes do Gadu-Gadu as a title
	if(!strcmp(ggProto, GGDEF_PROTO))
		ggProtoName = strdup(GGDEF_PROTONAME);
	else
		ggProtoName = strdup(p);
}

//////////////////////////////////////////////////////////
// When plugin is loaded
int __declspec(dllexport) Load(PLUGINLINK * link)
{
	WSADATA wsaData;
	PROTOCOLDESCRIPTOR pd;

	// Init winsock
	if (WSAStartup(MAKEWORD( 1, 1 ), &wsaData))
		return 1;

	// Init proto names
	init_protonames();

	pluginLink = link;

	// Hook system events
	hHookModulesLoaded = HookEvent(ME_SYSTEM_MODULESLOADED, gg_modulesloaded);
	hHookPreShutdown = HookEvent(ME_SYSTEM_PRESHUTDOWN, gg_preshutdown);

	// Prepare protocol name
	ZeroMemory(&pd, sizeof(pd));
	ZeroMemory(&ggModeMsg, sizeof(ggModeMsg));
	pd.cbSize = sizeof(pd);
	pd.szName = GG_PROTO;
	pd.type = PROTOTYPE_PROTOCOL;

	// Register module
	CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM) & pd);

	// Init mutex
	pthread_mutex_init(&threadMutex, NULL);
	pthread_mutex_init(&modeMsgsMutex, NULL);

	// Register services
	gg_registerservices();
	gg_setalloffline();
	gg_refreshblockedicon();
	return 0;
}

//////////////////////////////////////////////////////////
// when plugin is unloaded
int __declspec(dllexport) Unload()
{
	// Log off
	if(gg_isonline()) gg_disconnect();
	// Check threads
	gg_cleanupthreads();
#ifdef DEBUGMODE
	gg_netlog("Unload(): destroying plugin");
#endif
	/* gg_userinfo_destroy(); */
	gg_keepalive_destroy();
	gg_img_destroy();
	gg_gc_destroy();

	pthread_mutex_destroy(&threadMutex);
	pthread_mutex_destroy(&modeMsgsMutex);

	LocalEventUnhook(hHookModulesLoaded);
	LocalEventUnhook(hHookPreShutdown);

	LocalEventUnhook(hHookOptsInit);
	LocalEventUnhook(hHookSettingDeleted);
	LocalEventUnhook(hHookSettingChanged);
#ifdef DEBUGMODE
	gg_netlog("Unload(): closing hNetlib");
#endif
	Netlib_CloseHandle(hNetlib);

	// Free status messages
	if(ggModeMsg.szOnline)		free(ggModeMsg.szOnline);
	if(ggModeMsg.szAway)		free(ggModeMsg.szAway);
	if(ggModeMsg.szInvisible)	free(ggModeMsg.szInvisible);
	if(ggModeMsg.szOffline) 	free(ggModeMsg.szOffline);

	// Uninit SSL library
	gg_ssl_uninit();

	// Cleanup WinSock
	WSACleanup();

	// Cleanup protonames
	if(ggProtoName) free(ggProtoName);
	if(ggProtoError) free(ggProtoError);

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
	char szText[1024], *szFormat = strdup(format);
	// Kill end line
	char *nl = strrchr(szFormat, '\n');
	if(nl) *nl = 0;

	strncpy(szText, GG_PROTO, sizeof(szText));
	strncat(szText, "	   >> libgadu << \0", sizeof(szText) - strlen(szText));

	_vsnprintf(szText + strlen(szText), sizeof(szText) - strlen(szText), szFormat, ap);
	CallService(MS_NETLIB_LOG, (WPARAM) hNetlib, (LPARAM) szText);
	free(szFormat);
}

////////////////////////////////////////////////////////////
// Log funcion
int gg_netlog(const char *fmt, ...)
{
	va_list va;
	char szText[1024];
	strncpy(szText, GG_PROTO, sizeof(szText));
	strncat(szText, "::\0", sizeof(szText) - strlen(szText));

	va_start(va, fmt);
	_vsnprintf(szText + strlen(szText), sizeof(szText) - strlen(szText), fmt, va);
	va_end(va);
	return CallService(MS_NETLIB_LOG, (WPARAM) hNetlib, (LPARAM) szText);
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
