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

#include <m_system.h>
#include <m_langpack.h>
#include <m_options.h>
#include <m_skin.h>
#include <m_message.h>
#include <m_idle.h>

//#define HTTP_GATEWAY

/*
 * Global Variables
 */
HINSTANCE		hinstance;
PLUGINLINK		*pluginLink;
char			yahooProtocolName[MAX_PATH], mailsoundname[MAX_PATH];

HANDLE		    hNetlibUser = NULL;
HANDLE			YahooMenuItems[ MENU_ITEMS_COUNT ];
static HANDLE   hHookOptsInit;
static HANDLE   hHookModulesLoaded;
static HANDLE   hHookSettingChanged;
static HANDLE   hHookUserTyping;
HANDLE   hHookContactDeleted;
HANDLE   hHookIdle;

pthread_mutex_t connectionHandleMutex;

PLUGININFO pluginInfo={
		sizeof(PLUGININFO),
		"Yahoo Protocol",
		__VERSION_DWORD,
		"Yahoo Protocol support via libyahoo2 library.",
		"Gennady Feldman",
		"gena01@gmail.com",
		"© 2003-2005 G.Feldman",
		"http://www.miranda-im.org/download/details.php?action=viewfile&id=1248",
		0,
		0 //DEFMOD_PROTOCOLYAHOO
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
	// We using the latest/greatest API, including Typing Notifications and other stuff.
	//
    if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 4, 0, 0))
        return NULL;

    return &pluginInfo;
}

/*
 * Unload - Unloads plugin
 * Parameters: void
 */

__declspec(dllexport)int Unload(void)
{
	YAHOO_DebugLog("Unload");
	
	stop_timer();
	
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
	
	
	//YAHOO_DebugLog("Waiting for server thread to finish...");
	//SleepEx(100, TRUE);
	YAHOO_DebugLog("Before Netlib_CloseHandle");
    Netlib_CloseHandle( hNetlibUser );

	return 0;
}

/*
 * Convert old yahoo plugin contact to the new one
 * Parameters: void
 */
 
 void yahoo_convert_legacy() {
	HANDLE hContact;
	DBVARIANT dbv;
	char *szProto;

	if (DBGetContactSettingByte(NULL, yahooProtocolName, "LegacyConvert", 0)) return;
	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    while (hContact) {
		szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (szProto != NULL && !lstrcmp(szProto, yahooProtocolName)) {
			if (!DBGetContactSetting(hContact, yahooProtocolName, "id", &dbv)) {
				 DBWriteContactSettingString(hContact, yahooProtocolName, "yahoo_id", dbv.pszVal);
				 DBFreeVariant(&dbv);
				 DBDeleteContactSetting(hContact, yahooProtocolName, "id");
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	
	DBWriteContactSettingByte(NULL, yahooProtocolName, "LegacyConvert", 1);
}

int YahooIdleEvent(WPARAM wParam, LPARAM lParam);

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
	nlu.flags =  NUF_OUTGOING | NUF_HTTPGATEWAY;
#else
   	nlu.flags = NUF_INCOMING | NUF_OUTGOING | NUF_HTTPCONNS;
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
	
	// Add support for Plugin Uninstaller
	//DBWriteContactSettingString(NULL, "Uninstall", "Yahoo", yahooProtocolName);

	//add as a known module in DB Editor ++
	CallService("DBEditorpp/RegisterSingleModule",(WPARAM)yahooProtocolName, 0);
	
	yahoo_convert_legacy();

	start_timer();
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
	char path[MAX_PATH];
	char* protocolname;
	char* fend;
		
 	pluginLink=link;
	//
	// Need to disable threading since we got our own routines.
	//
	DisableThreadLibraryCalls(hinstance);
	
	GetModuleFileName( hinstance, path, sizeof( path ));

	protocolname = strrchr(path,'\\');
	protocolname++;
	fend = strrchr(path,'.');
	*fend = '\0';
	CharUpper( protocolname );
	
	lstrcpyn(yahooProtocolName, protocolname, MAX_PATH);

	lstrcpyn( mailsoundname, yahooProtocolName, MAX_PATH - 10);
	lstrcat( mailsoundname, ": " );
	lstrcat( mailsoundname,  Translate( "mail" ));
	
	SkinAddNewSound( mailsoundname, mailsoundname, "yahoomail.wav" );
	//SkinAddNewSoundEx( mailsoundname, mailsoundname, Translate( "mail" ), "yahoomail.wav" );

	// 1.
	LoadYahooServices();
	
	// 2.
	ZeroMemory(&pd,sizeof(pd));
	pd.cbSize=sizeof(pd);
	pd.szName=yahooProtocolName;
	pd.type=PROTOTYPE_PROTOCOL;
	CallService(MS_PROTO_REGISTERMODULE,0,(LPARAM)&pd);

	register_callbacks();
	
	// 3.
	hHookModulesLoaded = HookEvent( ME_SYSTEM_MODULESLOADED, OnModulesLoaded );
	
	yahoo_logoff_buddies();
	
	pthread_mutex_init(&connectionHandleMutex);	
    return 0;
}


