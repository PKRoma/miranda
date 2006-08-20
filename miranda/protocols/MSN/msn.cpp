/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2003-5 George Hazan.
Copyright (c) 2002-3 Richard Hughes (original version).

Miranda IM: the free icq client for MS Windows
Copyright (C) 2000-2002 Richard Hughes, Roland Rabien & Tristan Van de Vreede

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "msn_global.h"
#include "version.h"

#pragma comment( lib, "shlwapi.lib" )

HINSTANCE hInst;
PLUGINLINK *pluginLink;

struct MM_INTERFACE memoryManagerInterface;

/////////////////////////////////////////////////////////////////////////////////////////
// Initialization routines
int		MsnOnDetailsInit( WPARAM, LPARAM );

int		LoadMsnServices( void );
void     UnloadMsnServices( void );
void		MsgQueue_Init( void );
void		MsgQueue_Uninit( void );
void		Lists_Init( void );
void		Lists_Uninit( void );
void		P2pSessions_Uninit( void );
void		P2pSessions_Init( void );
void		Threads_Uninit( void );
int		MsnOptInit( WPARAM wParam, LPARAM lParam );
void		UninitSsl( void );

/////////////////////////////////////////////////////////////////////////////////////////
// Global variables

int      uniqueEventId = 0;
int      msnSearchID = -1;
char*    msnExternalIP = NULL;
char*    msnPreviousUUX = NULL;
HANDLE   msnMainThread;
int      msnOtherContactsBlocked = 0;
HANDLE   hHookOnUserInfoInit = NULL;
HANDLE   hGroupAddEvent = NULL;
HANDLE	 hMSNNudge = NULL;
bool		msnHaveChatDll = false;

MYOPTIONS MyOptions;

MSN_StatusMessage msnModeMsgs[ MSN_NUM_MODES ] = {
	{ ID_STATUS_ONLINE,     NULL },
	{ ID_STATUS_AWAY,       NULL },
	{ ID_STATUS_NA,         NULL },
	{ ID_STATUS_DND,        NULL },
	{ ID_STATUS_OCCUPIED,   NULL },
	{ ID_STATUS_ONTHEPHONE, NULL },
	{ ID_STATUS_OUTTOLUNCH, NULL } };

char* msnProtocolName = NULL;
char* msnProtChallenge = NULL;
char* msnProductID  = NULL;

char* mailsoundname;
char* ModuleName;

PLUGININFO pluginInfo =
{
	sizeof(PLUGININFO),
	#if defined( _UNICODE )
		"MSN Protocol (Unicode)",
	#else
		"MSN Protocol",
	#endif
	__VERSION_DWORD,
	"Adds support for communicating with users of the MSN Messenger network",
	"George Hazan",
	"george_hazan@hotmail.com",
	"© 2001-5 Richard Hughes, George Hazan",
	"http://miranda-im.org/download/details.php?action=viewfile&id=702",
	0,	0
};

bool			volatile msnLoggedIn = false;
ThreadData*	volatile msnNsThread = NULL;

int				msnStatusMode,
					msnDesiredStatus;
HANDLE			msnMenuItems[ MENU_ITEMS_COUNT ];
HANDLE			hNetlibUser = NULL;
HANDLE         hInitChat = NULL;
HANDLE			hEvInitChat = NULL;
bool				msnUseExtendedPopups;

int MsnOnDetailsInit( WPARAM wParam, LPARAM lParam );

int MSN_GCEventHook( WPARAM wParam, LPARAM lParam );
int MSN_GCMenuHook( WPARAM wParam, LPARAM lParam );
int MSN_ChatInit( WPARAM wParam, LPARAM lParam );

/////////////////////////////////////////////////////////////////////////////////////////
//	Main DLL function

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst = hinstDLL;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	OnModulesLoaded - finalizes plugin's configuration on load

int msn_httpGatewayInit(HANDLE hConn,NETLIBOPENCONNECTION *nloc,NETLIBHTTPREQUEST *nlhr);
int msn_httpGatewayBegin(HANDLE hConn,NETLIBOPENCONNECTION *nloc);
int msn_httpGatewayWrapSend(HANDLE hConn,PBYTE buf,int len,int flags,MIRANDASERVICE pfnNetlibSend);
PBYTE msn_httpGatewayUnwrapRecv(NETLIBHTTPREQUEST *nlhr,PBYTE buf,int len,int *outBufLen,void *(*NetlibRealloc)(void*,size_t));

static COLORREF crCols[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

static HANDLE hChatEvent = NULL, hChatMenu = NULL;

static int OnModulesLoaded( WPARAM wParam, LPARAM lParam )
{
	if ( !ServiceExists( MS_DB_CONTACT_GETSETTING_STR )) {
		MessageBox( NULL, TranslateT( "This plugin requires db3x plugin version 0.5.1.0 or later" ), _T("MSN"), MB_OK );
		return 1;
	}

	char szBuffer[ MAX_PATH ];

	if ( MSN_GetStaticString( "MsnPassportHost", NULL, szBuffer, sizeof szBuffer ))
		MSN_SetString( NULL, "MsnPassportHost", "https://loginnet.passport.com/login2.srf" );

	WORD wPort = MSN_GetWord( NULL, "YourPort", 0xFFFF );
	if ( wPort != 0xFFFF ) {
		MSN_SetByte( "NLSpecifyIncomingPorts", 1 );

		ltoa( wPort, szBuffer, 10 );
		MSN_SetString( NULL, "NLIncomingPorts", szBuffer );

		DBDeleteContactSetting( NULL, msnProtocolName, "YourPort" );
	}

	mir_snprintf( szBuffer, sizeof szBuffer, "%s plugin connections", msnProtocolName );

	NETLIBUSER nlu = {0};
	nlu.cbSize = sizeof( nlu );
	nlu.flags = NUF_INCOMING | NUF_OUTGOING | NUF_HTTPCONNS;
	nlu.szSettingsModule = msnProtocolName;
	nlu.szDescriptiveName = MSN_Translate( szBuffer );

	if ( MyOptions.UseGateway ) {
		nlu.flags |= NUF_HTTPGATEWAY;
		nlu.szHttpGatewayUserAgent = MSN_USER_AGENT;
		nlu.pfnHttpGatewayInit = msn_httpGatewayInit;
		nlu.pfnHttpGatewayWrapSend = msn_httpGatewayWrapSend;
		nlu.pfnHttpGatewayUnwrapRecv = msn_httpGatewayUnwrapRecv;
	}

	hNetlibUser = ( HANDLE )MSN_CallService( MS_NETLIB_REGISTERUSER, 0, ( LPARAM )&nlu );

	if ( MSN_GetByte( "UseIeProxy", 0 )) {
		NETLIBUSERSETTINGS nls = { 0 };
		nls.cbSize = sizeof( nls );
		MSN_CallService(MS_NETLIB_GETUSERSETTINGS,WPARAM(hNetlibUser),LPARAM(&nls));

		HKEY hSettings;
		if ( RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", &hSettings ))
			return 0;

		char tValue[ 256 ];
		DWORD tType = REG_SZ, tValueLen = sizeof( tValue );
		int tResult = RegQueryValueExA( hSettings, "ProxyServer", NULL, &tType, ( BYTE* )tValue, &tValueLen );
		RegCloseKey( hSettings );

		if ( !tResult )
		{
			char* tDelim = strstr( tValue, "http=" );
			if ( tDelim != 0 ) {
				strdel( tValue, int( tDelim - tValue )+5 );

				tDelim = strchr( tValue, ';' );
				if ( tDelim != NULL )
					*tDelim = '\0';
			}

			tDelim = strchr( tValue, ':' );
			if ( tDelim != NULL ) {
				*tDelim = 0;
				nls.wProxyPort = atol( tDelim+1 );
			}

			rtrim( tValue );
			nls.szProxyServer = tValue;
			MyOptions.UseProxy = nls.useProxy = tValue[0] != 0;
			nls.proxyType = PROXYTYPE_HTTP;
			nls.szIncomingPorts = NEWSTR_ALLOCA(nls.szIncomingPorts);
			nls.szOutgoingPorts = NEWSTR_ALLOCA(nls.szOutgoingPorts);
			nls.szProxyAuthPassword = NEWSTR_ALLOCA(nls.szProxyAuthPassword);
			nls.szProxyAuthUser = NEWSTR_ALLOCA(nls.szProxyAuthUser);
			MSN_CallService(MS_NETLIB_SETUSERSETTINGS,WPARAM(hNetlibUser),LPARAM(&nls));
	}	}

	if ( ServiceExists( MS_GC_REGISTER )) {
		msnHaveChatDll = true;

		GCREGISTER gcr = {0};
		gcr.cbSize = sizeof( GCREGISTER );
		gcr.dwFlags = GC_TYPNOTIF|GC_CHANMGR;
		gcr.iMaxText = 0;
		gcr.nColors = 16;
		gcr.pColors = &crCols[0];
		gcr.pszModuleDispName = msnProtocolName;
		gcr.pszModule = msnProtocolName;
		MSN_CallService( MS_GC_REGISTER, NULL, ( LPARAM )&gcr );

		hChatEvent = HookEvent( ME_GC_EVENT, MSN_GCEventHook );
		hChatMenu = HookEvent( ME_GC_BUILDMENU, MSN_GCMenuHook );

		char szEvent[ 200 ];
		mir_snprintf( szEvent, sizeof szEvent, "%s\\ChatInit", msnProtocolName );
		hInitChat = CreateHookableEvent( szEvent );
		hEvInitChat = HookEvent( szEvent, MSN_ChatInit );
	}

	msnUseExtendedPopups = ServiceExists( MS_POPUP_ADDPOPUPEX ) != 0;
	hHookOnUserInfoInit = HookEvent( ME_USERINFO_INITIALISE, MsnOnDetailsInit );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnPreShutdown - prepare a global Miranda shutdown

extern HANDLE hKeepAliveThreadEvt;

static int OnPreShutdown( WPARAM wParam, LPARAM lParam )
{
	if ( hKeepAliveThreadEvt != NULL )
		SetEvent( hKeepAliveThreadEvt );

	MSN_CloseThreads();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Performs a primary set of actions upon plugin loading

extern "C" int __declspec(dllexport) Load( PLUGINLINK* link )
{
	pluginLink = link;
	DuplicateHandle( GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &msnMainThread, THREAD_SET_CONTEXT, FALSE, 0 );

	// get the internal malloc/free()
	memset(&memoryManagerInterface, 0, sizeof(memoryManagerInterface));
	memoryManagerInterface.cbSize = sizeof(memoryManagerInterface);
	CallService(MS_SYSTEM_GET_MMI, 0, (LPARAM) &memoryManagerInterface);

	char path[MAX_PATH];
	char* protocolname;
	char* fend;

	GetModuleFileNameA( hInst, path, sizeof( path ));

	protocolname = strrchr(path,'\\');
	protocolname++;
	fend = strrchr(path,'.');
	*fend = '\0';
	CharUpperA( protocolname );
	msnProtocolName = strdup( protocolname );

	mir_snprintf( path, sizeof( path ), "%s:HotmailNotify", protocolname );
	ModuleName = strdup( path );

//	Uninstalling purposes
//	if (ServiceExists("PluginSweeper/Add"))
//		MSN_CallService("PluginSweeper/Add",(WPARAM)MSN_Translate(ModuleName),(LPARAM)ModuleName);

	HookEvent( ME_SYSTEM_MODULESLOADED, OnModulesLoaded );

	srand(( unsigned int )time( NULL ));

	LoadOptions();
	HookEvent( ME_OPT_INITIALISE, MsnOptInit );
	HookEvent( ME_SYSTEM_PRESHUTDOWN, OnPreShutdown );

	char nudge[250];
	sprintf(nudge,"%s/Nudge",protocolname);
	hMSNNudge = CreateHookableEvent(nudge);
	
	MSN_InitThreads();

	PROTOCOLDESCRIPTOR pd;
	memset( &pd, 0, sizeof( pd ));
	pd.cbSize = sizeof( pd );
	pd.szName = msnProtocolName;
	pd.type = PROTOTYPE_PROTOCOL;
	MSN_CallService( MS_PROTO_REGISTERMODULE, 0, ( LPARAM )&pd );

	HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) {
		if ( !lstrcmpA( msnProtocolName, ( char* )MSN_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact,0 )))
			MSN_SetWord( hContact, "Status", ID_STATUS_OFFLINE );
		hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT,( WPARAM )hContact, 0 );
	}

	char mailsoundtemp[ 64 ];
	strcpy( mailsoundtemp, protocolname );
	strcat( mailsoundtemp, ": " );
	strcat( mailsoundtemp,  MSN_Translate( "Hotmail" ));
	mailsoundname = strdup( mailsoundtemp );
	SkinAddNewSound( mailsoundtemp, mailsoundtemp, "hotmail.wav" );

	msnStatusMode = msnDesiredStatus = ID_STATUS_OFFLINE;
	msnLoggedIn = false;
	LoadMsnServices();
	Lists_Init();
	MsgQueue_Init();
	P2pSessions_Init();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Unload a plugin

extern char* rru;
extern char* profileURL;

extern "C" int __declspec( dllexport ) Unload( void )
{
	if ( msnLoggedIn )
		msnNsThread->sendPacket( "OUT", NULL );

	if ( hHookOnUserInfoInit )
		UnhookEvent( hHookOnUserInfoInit );

	if ( hChatEvent  ) UnhookEvent( hChatEvent );
	if ( hChatMenu   ) UnhookEvent( hChatMenu );
	if ( hEvInitChat ) UnhookEvent( hEvInitChat );

	if ( hInitChat )
		DestroyHookableEvent( hInitChat );

	if ( hMSNNudge )
		DestroyHookableEvent( hMSNNudge );

	UninitSsl();
	MSN_FreeGroups();
	Threads_Uninit();
	MsgQueue_Uninit();
	Lists_Uninit();
	P2pSessions_Uninit();
	Netlib_CloseHandle( hNetlibUser );

	UnloadMsnServices();

	free( mailsoundname );
	free( msnProtocolName );
	free( ModuleName );

	CloseHandle( msnMainThread );

	for ( int i=0; i < MSN_NUM_MODES; i++ )
		if ( msnModeMsgs[ i ].m_msg )
			free( msnModeMsgs[ i ].m_msg );

	if ( kv ) free( kv );
	if ( sid ) free( sid );
	if ( passport ) free( passport );
	if ( MSPAuth ) free( MSPAuth );
	if ( rru ) free( rru );
	if ( profileURL ) free( profileURL );

	if ( msnPreviousUUX ) free( msnPreviousUUX );
	if ( msnExternalIP ) free( msnExternalIP );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MirandaPluginInfo - returns an information about a plugin

extern "C" __declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	if ( mirandaVersion < PLUGIN_MAKE_VERSION( 0, 6, 0, 0 )) {
		MessageBox( NULL, _T("The MSN protocol plugin cannot be loaded. It requires Miranda IM 0.6.0 or later."), _T("MSN Protocol Plugin"), MB_OK|MB_ICONWARNING|MB_SETFOREGROUND|MB_TOPMOST );
		return NULL;
	}

	return &pluginInfo;
}
