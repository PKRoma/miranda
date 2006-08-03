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
#include <windows.h>
#include "yahoo.h"
#include "http_gateway.h"
#include "version.h"
#include "resource.h"

#include <m_system.h>
#include <m_langpack.h>
#include <m_options.h>
#include <m_skin.h>
#include <m_message.h>
#include <m_idle.h>
#include <m_userinfo.h>

//#define HTTP_GATEWAY
extern char *szStartMsg;

/*
 * Global Variables
 */
HINSTANCE		hinstance;
PLUGINLINK		*pluginLink;
char			yahooProtocolName[MAX_PATH];

HANDLE		    hNetlibUser = NULL;
HANDLE			YahooMenuItems[ MENU_ITEMS_COUNT ];
static HANDLE   hHookOptsInit;
static HANDLE   hHookModulesLoaded;
static HANDLE   hHookSettingChanged;
static HANDLE   hHookUserTyping;
static HANDLE   hHookUserInfoInit;
HANDLE   hHookContactDeleted;
HANDLE   hHookIdle;
HANDLE   hYahooNudge = NULL;

pthread_mutex_t connectionHandleMutex;

PLUGININFO pluginInfo={
		sizeof(PLUGININFO),
#ifdef YAHOO_CVSBUILD
		"Yahoo Protocol Beta/Nightly",
#else
		"Yahoo Protocol",
#endif
		__VERSION_DWORD,
#ifdef YAHOO_CVSBUILD
		"Yahoo Protocol support via libyahoo2 library. [Built: "__DATE__" "__TIME__"]",
#else
		"Yahoo Protocol support via libyahoo2 library.",
#endif		
		"Gennady Feldman, Laurent Marechal",
		"gena01@gmail.com",
		"� 2003-2006 G.Feldman",
		"http://www.miranda-im.org/download/details.php?action=viewfile&id=1248",
		0, //not transient
		0 //DEFMOD_PROTOCOLYAHOO - no core yahoo protocol
};

int    yahooStatus = ID_STATUS_OFFLINE;
BOOL   yahooLoggedIn = FALSE;

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
	hinstance=hinst;
	return TRUE;
}


/*
 * MainInit - Called at very beginning of plugin
 * Parameters: wparam , lparam
 * Returns : int
 */
/*int MainInit(WPARAM wparam,LPARAM lparam)
{
	return 0;
}*/

/*
 * MirandaPluginInfo - Sets plugin info
 * Parameters: (DWORD mirandaVersion)
 */
__declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	//
	// We require Miranda 0.4
	//
    if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 4, 0, 0)) {
		MessageBox( NULL, 
				"Yahoo plugin cannot be loaded. It requires Miranda IM 0.4 or later.", 
				"Yahoo", 
				MB_OK|MB_ICONWARNING|MB_SETFOREGROUND|MB_TOPMOST );

        return NULL;
	}

    return &pluginInfo;
}

/*
 * Unload - Unloads plugin
 * Parameters: void
 */

int __declspec(dllexport) Unload(void)
{
	YAHOO_DebugLog("Unload");
	
	//stop_timer();
	
	if (yahooLoggedIn)
		yahoo_logout();

	YAHOO_DebugLog("Logged out");
	pthread_mutex_destroy(&connectionHandleMutex);

	LocalEventUnhook(hHookContactDeleted);
	LocalEventUnhook(hHookIdle);
	LocalEventUnhook(hHookModulesLoaded);
	LocalEventUnhook(hHookOptsInit);
	LocalEventUnhook(hHookSettingChanged);
	LocalEventUnhook(hHookUserTyping);
	
	if (szStartMsg)
		free(szStartMsg);
	
	YAHOO_DebugLog("Before Netlib_CloseHandle");
    Netlib_CloseHandle( hNetlibUser );

	return 0;
}

int YahooIdleEvent(WPARAM wParam, LPARAM lParam);
int OnDetailsInit(WPARAM wParam, LPARAM lParam);


/*
 *	Load - loads plugin into memory
 */
 
static int OnModulesLoaded( WPARAM wParam, LPARAM lParam )
{
	char tModule[ 100 ], tModuleDescr[ 100 ];
	NETLIBUSER nlu = {0};

	CharUpper( lstrcpy( tModule, yahooProtocolName ));
	lstrcpyn(tModuleDescr, yahooProtocolName , sizeof( tModuleDescr ) - 25);
	lstrcat(tModuleDescr," plugin connections");
	
	nlu.cbSize = sizeof(nlu);

#ifdef HTTP_GATEWAY
	nlu.flags = NUF_OUTGOING | NUF_HTTPGATEWAY;
#else
   	nlu.flags = NUF_OUTGOING;
#endif

	nlu.szSettingsModule = tModule;
	nlu.szDescriptiveName = Translate( tModuleDescr );
	
#ifdef HTTP_GATEWAY
	// Here comes the Gateway Code! 
	nlu.szHttpGatewayHello = NULL;//"http://http.proxy.icq.com/hello";
	nlu.szHttpGatewayUserAgent = "Mozilla/4.08 [en] (WinNT; U ;Nav)";//"Mozilla/4.5 [en] (X11; U; FreeBSD 2.2.8-STABLE i386)";
 //"Mozilla/4.01 [en] (Win95; I)";//"Mozilla/4.08 [en] (WinNT; U ;Nav)";//\nTest-Header: boo";
	nlu.pfnHttpGatewayInit = YAHOO_httpGatewayInit;
	nlu.pfnHttpGatewayBegin = NULL;
	nlu.pfnHttpGatewayWrapSend = NULL;
	nlu.pfnHttpGatewayUnwrapRecv = YAHOO_httpGatewayUnwrapRecv;
#endif	
	
	hNetlibUser = ( HANDLE )YAHOO_CallService( MS_NETLIB_REGISTERUSER, 0, ( LPARAM )&nlu );
	
	hHookOptsInit = HookEvent( ME_OPT_INITIALISE, YahooOptInit );
    hHookSettingChanged = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, YAHOO_util_dbsettingchanged);
	hHookIdle = HookEvent(ME_IDLE_CHANGED, YahooIdleEvent);
	hHookUserInfoInit = HookEvent(ME_USERINFO_INITIALISE, OnDetailsInit);
	
	// Add support for Plugin Uninstaller
	//DBWriteContactSettingString(NULL, "Uninstall", "Yahoo", yahooProtocolName);

	//add as a known module in DB Editor ++
	CallService("DBEditorpp/RegisterSingleModule",(WPARAM)yahooProtocolName, 0);
	
	//start_timer();
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
	PROTOCOLDESCRIPTOR pd;
	char path[MAX_PATH], tNudge[250];
	char* protocolname;
	
 	pluginLink=link;
	//
	// Need to disable threading since we got our own routines.
	//
	DisableThreadLibraryCalls(hinstance);
	
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

	// 1.
	hHookModulesLoaded = HookEvent( ME_SYSTEM_MODULESLOADED, OnModulesLoaded );
	// Create nudge event
	lstrcpyn(tNudge, yahooProtocolName , sizeof( tNudge ) - 7);
	lstrcat(tNudge, "/Nudge");
	hYahooNudge = CreateHookableEvent(tNudge);
	
	// 2.
	ZeroMemory(&pd,sizeof(pd));
	pd.cbSize=sizeof(pd);
	pd.szName=yahooProtocolName;
	pd.type=PROTOTYPE_PROTOCOL;
	CallService(MS_PROTO_REGISTERMODULE,0,(LPARAM)&pd);

	register_callbacks();
	// 3.
	yahoo_logoff_buddies();

	SkinAddNewSoundEx(Translate( "mail" ), yahooProtocolName, "New E-mail available in Inbox" );

	LoadYahooServices();

	pthread_mutex_init(&connectionHandleMutex);	
    return 0;
}

