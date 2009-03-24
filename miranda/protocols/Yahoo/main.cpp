/*
 * $Id$
 *
 * myYahoo Miranda Plugin 
 *
 * Authors: Gennady Feldman (aka Gena01) 
 *          Laurent Marechal (aka Peorth)
 *
 * This code is under GPL and is based on AIM, MSN and Miranda source code.
 * I want to thank Robert Rainwater and George Hazan for their code and support
 * and for answering some of my questions during development of this plugin.
 */
#include "yahoo.h"
#include "http_gateway.h"
#include "version.h"
#include "resource.h"

#include <m_langpack.h>
#include <m_skin.h>
#include <m_idle.h>
#include <m_userinfo.h>
#include <time.h>
#include "options.h"

extern char *szStartMsg;
extern yahoo_local_account *ylad;

/*
 * Global Variables
 */
HINSTANCE   hinstance;
PLUGINLINK* pluginLink;
char		yahooProtocolName[MAX_PATH];

struct MM_INTERFACE		mmi;
struct UTF8_INTERFACE	utfi;
struct MD5_INTERFACE	md5i;
struct SHA1_INTERFACE	sha1i;

HANDLE					hNetlibUser = NULL;
HANDLE					YahooMenuItems[ MENU_ITEMS_COUNT ];
static HANDLE   		hHookOptsInit;
static HANDLE   		hHookModulesLoaded;
static HANDLE   		hHookSettingChanged;
static HANDLE   		hHookUserTyping;
HANDLE   				hHookContactDeleted;
HANDLE   				hHookIdle;
HANDLE   				hYahooNudge = NULL;
int    					yahooStatus = ID_STATUS_OFFLINE;
BOOL					yahooLoggedIn = FALSE;

PLUGININFOEX pluginInfo={
		sizeof(PLUGININFOEX),
#ifdef YAHOO_CVSBUILD
		"Yahoo Protocol Beta/Nightly",
#else
		"Yahoo Protocol",
#endif
		__VERSION_DWORD,
		"Yahoo Protocol support via libyahoo2 library. [Built: "__DATE__" "__TIME__"]",
		"Gennady Feldman",
		"gena01@miranda-im.org",
		"© 2003-2009 Gennady Feldman, Laurent Marechal",
		"http://www.miranda-im.org",
		0, //not transient
		0, //DEFMOD_PROTOCOLYAHOO - no core yahoo protocol
        {0xa6648b6c, 0x6fb8, 0x4551, { 0xb4, 0xe7, 0x1, 0x36, 0xf9, 0x16, 0xd4, 0x85 }} //{A6648B6C-6FB8-4551-B4E7-0136F916D485}
};

/*
 * WINAPI DllMain - main entry point into a DLL
 * Parameters: 
 *          HINSTANCE hinst,
 *          DWORD fdwReason,
 *          LPVOID lpvReserved
 * Returns : 
 *           BOOL
 * 
 */
BOOL WINAPI DllMain(HINSTANCE hinst,DWORD fdwReason,LPVOID lpvReserved)
{
	hinstance = hinst;
	
	return TRUE;
}

/*
 *	Load - loads plugin into memory
 */
 
static int OnModulesLoaded( WPARAM wParam, LPARAM lParam )
{
	char tModuleDescr[ 100 ];
	NETLIBUSER nlu = {0};

	wsprintf(tModuleDescr, Translate( "%s plugin connections" ), yahooProtocolName);
	
	nlu.cbSize = sizeof(nlu);

#ifdef HTTP_GATEWAY
	nlu.flags = NUF_OUTGOING | NUF_HTTPGATEWAY| NUF_HTTPCONNS;
#else
   	nlu.flags = NUF_OUTGOING | NUF_HTTPCONNS;
#endif

	nlu.szSettingsModule = yahooProtocolName;
	nlu.szDescriptiveName = tModuleDescr;
	
#ifdef HTTP_GATEWAY
	// Here comes the Gateway Code! 
	nlu.szHttpGatewayHello = NULL;
	nlu.szHttpGatewayUserAgent = "User-Agent: Mozilla/4.01 [en] (Win95; I)";
 	nlu.pfnHttpGatewayInit = YAHOO_httpGatewayInit;
	nlu.pfnHttpGatewayBegin = NULL;
	nlu.pfnHttpGatewayWrapSend = YAHOO_httpGatewayWrapSend;
	nlu.pfnHttpGatewayUnwrapRecv = YAHOO_httpGatewayUnwrapRecv;
#endif	
	
	hNetlibUser = ( HANDLE )YAHOO_CallService( MS_NETLIB_REGISTERUSER, 0, ( LPARAM )&nlu );

	YahooMenuInit();
	
	hHookSettingChanged = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, YAHOO_util_dbsettingchanged);
	hHookIdle = HookEvent(ME_IDLE_CHANGED, YahooIdleEvent);
	
	return 0;
}

//=====================================================
// Name : Load
// Parameters: PLUGINLINK *link
// Returns : int
// Description : Called when plugin is loaded into Miranda
//=====================================================

int __declspec(dllexport)Load(PLUGINLINK *link)
{
	PROTOCOLDESCRIPTOR pd = { 0 };
	char path[MAX_PATH], tNudge[250];
	char* protocolname;
	
 	pluginLink = link;
	mir_getMMI( &mmi );
	mir_getUTFI( &utfi );
	mir_getMD5I( &md5i );
	mir_getSHA1I( &sha1i );
	
	/*
	 * Need to disable threading since we got our own routines.
	 */
	DisableThreadLibraryCalls( hinstance );
	
	GetModuleFileName( hinstance, path, sizeof( path ));

	protocolname = strrchr(path,'\\');
	
	if (protocolname != NULL) {
		char* fend;
		
		protocolname++;
		fend = strrchr(path,'.');
		
		if (fend != NULL)
			*fend = '\0';
		
		CharUpper( protocolname );
		lstrcpyn(yahooProtocolName, protocolname, MAX_PATH);
	} else 
		lstrcpy(yahooProtocolName, "YAHOO");

	
	mir_snprintf( path, sizeof( path ), "%s/Status", yahooProtocolName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/YStatus", yahooProtocolName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/YAway", yahooProtocolName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/Mobile", yahooProtocolName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );
	
	mir_snprintf( path, sizeof( path ), "%s/YGMsg", yahooProtocolName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );
	
	mir_snprintf( path, sizeof( path ), "%s/IdleTS", yahooProtocolName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );
	
	mir_snprintf( path, sizeof( path ), "%s/PictLastCheck", yahooProtocolName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/PictLoading", yahooProtocolName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	// 1.
	hHookModulesLoaded = HookEvent( ME_SYSTEM_MODULESLOADED, OnModulesLoaded );
	
	srand(( unsigned int )time( NULL ));

	hHookOptsInit = HookEvent( ME_OPT_INITIALISE, YahooOptInit );

	// Create nudge event
	lstrcpyn(tNudge, yahooProtocolName , sizeof( tNudge ) - 7);
	lstrcat(tNudge, "/Nudge");
	hYahooNudge = CreateHookableEvent(tNudge);
	
	// 2.
	pd.cbSize = sizeof(pd);
	pd.szName = yahooProtocolName;
	pd.type   = PROTOTYPE_PROTOCOL;
	CallService( MS_PROTO_REGISTERMODULE, 0, ( LPARAM )&pd );

	// Initialize our important variable
	ylad = y_new0(yahoo_local_account, 1);
	
	register_callbacks();
	
	// 3.
	yahoo_logoff_buddies();

	SkinAddNewSoundEx(Translate( "mail" ), yahooProtocolName, "New E-mail available in Inbox" );
	
	LoadYahooServices();

	YahooIconsInit();
	
	return 0;
}

/*
 * Unload - Unloads plugin
 * Parameters: void
 */

int __declspec(dllexport) Unload(void)
{
	YAHOO_DebugLog("Unload");
	
	if (yahooLoggedIn)
		yahoo_logout();

	YAHOO_DebugLog("Logged out");

	LocalEventUnhook(hHookContactDeleted);
	LocalEventUnhook(hHookIdle);
	LocalEventUnhook(hHookModulesLoaded);
	LocalEventUnhook(hHookOptsInit);
	LocalEventUnhook(hHookSettingChanged);
	LocalEventUnhook(hHookUserTyping);
	
	FREE(szStartMsg);
	FREE(ylad);
	
	YAHOO_DebugLog("Before Netlib_CloseHandle");
    Netlib_CloseHandle( hNetlibUser );

	return 0;
}

/*
 * MirandaPluginInfoEx - Sets plugin info
 * Parameters: (DWORD mirandaVersion)
 */
__declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	/*
     * We require Miranda 0.8.0.24
	 * This requires the latest trunk... [md5, sha, etc..]
	 */
    if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 8, 0, 24)) {
		MessageBox( NULL, 
				"Yahoo plugin cannot be loaded. It requires Miranda IM 0.8.0.24 or later.", 
				"Yahoo", 
				MB_OK|MB_ICONWARNING|MB_SETFOREGROUND|MB_TOPMOST );

        return NULL;
	}

    return &pluginInfo;
}

/*
 * MirandaPluginInterfaces - Notifies the core of interfaces implemented
 * Parameters: none
 */
static const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};

__declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}
