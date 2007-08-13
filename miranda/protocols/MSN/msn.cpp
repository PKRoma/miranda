/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-7 Boris Krasnovskiy.
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

HINSTANCE hInst;
PLUGINLINK *pluginLink;

MM_INTERFACE   mmi;
LIST_INTERFACE li;
UTF8_INTERFACE utfi;
MD5_INTERFACE  md5i;
SHA1_INTERFACE sha1i;

/////////////////////////////////////////////////////////////////////////////////////////
// Initialization routines
int		MsnOnDetailsInit( WPARAM, LPARAM );
int		MsnWindowEvent(WPARAM wParam, LPARAM lParam);

int		LoadMsnServices( void );
void    UnloadMsnServices( void );
void	MsgQueue_Init( void );
void	MsgQueue_Uninit( void );
void	P2pSessions_Uninit( void );
void	P2pSessions_Init( void );
void	Threads_Uninit( void );
int		MsnOptInit( WPARAM wParam, LPARAM lParam );
void	UninitSsl( void );

/////////////////////////////////////////////////////////////////////////////////////////
// Global variables

int      msnSearchID = -1;
char*    msnExternalIP = NULL;
char*    msnPreviousUUX = NULL;
HANDLE   msnMainThread;
unsigned msnOtherContactsBlocked = 0;
HANDLE   hMSNNudge = NULL;
bool	 msnHaveChatDll = false;

MYOPTIONS MyOptions;

MSN_StatusMessage msnModeMsgs[ MSN_NUM_MODES ] = {
	{ ID_STATUS_ONLINE,     NULL },
	{ ID_STATUS_AWAY,       NULL },
	{ ID_STATUS_NA,         NULL },
	{ ID_STATUS_DND,        NULL },
	{ ID_STATUS_OCCUPIED,   NULL },
	{ ID_STATUS_ONTHEPHONE, NULL },
	{ ID_STATUS_OUTTOLUNCH, NULL } };

LISTENINGTOINFO msnCurrentMedia;

char* msnProtocolName = NULL;
char* msnProtChallenge = NULL;
char* msnProductID  = NULL;

char* mailsoundname;
char* alertsoundname;
char* ModuleName;

PLUGININFOEX pluginInfo =
{
	sizeof(PLUGININFOEX),
	"MSN Protocol",
	__VERSION_DWORD,
	"Adds support for communicating with users of the MSN Messenger network",
	"Boris Krasnovskiy, George Hazan, Richard Hughes",
	"borkra@miranda-im.org",
	"© 2001-2007 Richard Hughes, George Hazan, Boris Krasnovskiy",
	"http://miranda-im.org",
	UNICODE_AWARE,	
	0,
    #if defined( _UNICODE )
	{0xdc39da8a, 0x8385, 0x4cd9, {0xb2, 0x98, 0x80, 0x67, 0x7b, 0x8f, 0xe6, 0xe4}} //{DC39DA8A-8385-4cd9-B298-80677B8FE6E4}
    #else
    {0x29aa3a80, 0x3368, 0x4b78, { 0x82, 0xc1, 0xdf, 0xc7, 0x29, 0x6a, 0x58, 0x99 }} //{29AA3A80-3368-4b78-82C1-DFC7296A5899}
    #endif
};

bool        msnLoggedIn = false;
ThreadData*	msnNsThread = NULL;

unsigned	msnStatusMode,
			msnDesiredStatus;
HANDLE		hNetlibUser = NULL;
HANDLE		hInitChat = NULL;

int CompareHandles( const void* p1, const void* p2 )
{	return (long)p1 - (long)p2;
}
static LIST<void> arHooks( 20, CompareHandles );

int MsnContactDeleted( WPARAM wParam, LPARAM lParam );
int MsnDbSettingChanged(WPARAM wParam,LPARAM lParam);
int MsnGroupChange(WPARAM wParam,LPARAM lParam);
int MsnOnDetailsInit( WPARAM wParam, LPARAM lParam );
int MsnRebuildContactMenu( WPARAM wParam, LPARAM lParam );
int MsnIdleChanged( WPARAM wParam, LPARAM lParam );

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

static int OnModulesLoaded( WPARAM wParam, LPARAM lParam )
{
	if ( !ServiceExists( MS_DB_CONTACT_GETSETTING_STR )) {
		MessageBox( NULL, TranslateT( "This plugin requires db3x plugin version 0.5.1.0 or later" ), _T("MSN"), MB_OK );
		return 1;
	}

	char szBuffer[ MAX_PATH ];

	mir_snprintf( szBuffer, sizeof(szBuffer), MSN_Translate("%s plugin connections"), msnProtocolName );

	NETLIBUSER nlu = {0};
	nlu.cbSize = sizeof( nlu );
	nlu.flags = NUF_INCOMING | NUF_OUTGOING | NUF_HTTPCONNS;
	nlu.szSettingsModule = msnProtocolName;
	nlu.szDescriptiveName = szBuffer;

	if ( MyOptions.UseGateway ) {
		nlu.flags |= NUF_HTTPGATEWAY;
		nlu.szHttpGatewayUserAgent = (char*)MSN_USER_AGENT;
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
				tDelim += 5;
				memmove(tValue, tDelim, strlen(tDelim)+1);

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
			nls.useProxy = MyOptions.UseProxy = tValue[0] != 0;
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
		gcr.dwFlags = GC_TYPNOTIF | GC_CHANMGR;
		gcr.iMaxText = 0;
		gcr.nColors = 16;
		gcr.pColors = &crCols[0];
		gcr.pszModuleDispName = msnProtocolName;
		gcr.pszModule = msnProtocolName;
		CallServiceSync( MS_GC_REGISTER, 0, ( LPARAM )&gcr );

		arHooks.insert( HookEvent( ME_GC_EVENT, MSN_GCEventHook ));
		arHooks.insert( HookEvent( ME_GC_BUILDMENU, MSN_GCMenuHook ));

		char szEvent[ 200 ];
		mir_snprintf( szEvent, sizeof szEvent, "%s\\ChatInit", msnProtocolName );
		hInitChat = CreateHookableEvent( szEvent );
		arHooks.insert( HookEvent( szEvent, MSN_ChatInit ));
	}

	MsnInitMenus();

//	arHooks.insert( HookEvent( ME_USERINFO_INITIALISE, MsnOnDetailsInit ));
	arHooks.insert( HookEvent( ME_MSG_WINDOWEVENT, MsnWindowEvent ));
	arHooks.insert( HookEvent( ME_IDLE_CHANGED, MsnIdleChanged ));
	arHooks.insert( HookEvent( ME_DB_CONTACT_DELETED, MsnContactDeleted ));
	arHooks.insert( HookEvent( ME_DB_CONTACT_SETTINGCHANGED, MsnDbSettingChanged ));
	arHooks.insert( HookEvent( ME_CLIST_PREBUILDCONTACTMENU, MsnRebuildContactMenu ));
	arHooks.insert( HookEvent( ME_CLIST_GROUPCHANGE, MsnGroupChange ));

	InitCustomFolders();

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnPreShutdown - prepare a global Miranda shutdown

static int OnPreShutdown( WPARAM wParam, LPARAM lParam )
{
	MSN_CloseThreads();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Performs a primary set of actions upon plugin loading

extern "C" int __declspec(dllexport) Load( PLUGINLINK* link )
{
	pluginLink = link;
	DuplicateHandle( GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &msnMainThread, THREAD_SET_CONTEXT, FALSE, 0 );

	// get the internal malloc/mir_free()
	mir_getLI( &li );
	mir_getMMI( &mmi );
	mir_getUTFI( &utfi );
	mir_getMD5I( &md5i );
    mir_getSHA1I( &sha1i );

	char path[MAX_PATH];
	char* protocolname;
	char* fend;

	GetModuleFileNameA( hInst, path, sizeof( path ));

	protocolname = strrchr(path,'\\');
	protocolname++;
	fend = strrchr(path,'.');
	*fend = '\0';
	_strupr( protocolname );
	msnProtocolName = mir_strdup( protocolname );

	mir_snprintf( path, sizeof( path ), "%s:HotmailNotify", protocolname );
	ModuleName = mir_strdup( path );

	mir_snprintf( path, sizeof( path ), "%s/Status", protocolname );
	MSN_CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

//	MSN_CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )"CList/StatusMsg" );

	mir_snprintf( path, sizeof( path ), "%s/IdleTS", protocolname );
	MSN_CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/p2pMsgId", protocolname );
	MSN_CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/AccList", protocolname );
	MSN_CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/MobileEnabled", protocolname );
	MSN_CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/MobileAllowed", protocolname );
	MSN_CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	arHooks.insert( HookEvent( ME_SYSTEM_MODULESLOADED, OnModulesLoaded ));

	srand(( unsigned int )time( NULL ));

	LoadOptions();
	arHooks.insert( HookEvent( ME_OPT_INITIALISE, MsnOptInit ));
	arHooks.insert( HookEvent( ME_SYSTEM_PRESHUTDOWN, OnPreShutdown ));

	char evtname[250];
	sprintf(evtname,"%s/Nudge",protocolname);
	hMSNNudge = CreateHookableEvent(evtname);

	MSN_InitThreads();

	PROTOCOLDESCRIPTOR pd = {0};
	pd.cbSize = sizeof( pd );
	pd.szName = msnProtocolName;
	pd.type = PROTOTYPE_PROTOCOL;
	MSN_CallService( MS_PROTO_REGISTERMODULE, 0, ( LPARAM )&pd );

	HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) {
		if ( MSN_IsMyContact( hContact ))
		{
			MSN_DeleteSetting( hContact, "Status" );
			MSN_DeleteSetting( hContact, "IdleTS" );
			MSN_DeleteSetting( hContact, "p2pMsgId" );
			MSN_DeleteSetting( hContact, "AccList" );
//			DBDeleteContactSetting( hContact, "CList", "StatusMsg" );
		}
		hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT,( WPARAM )hContact, 0 );
	}
	MSN_DeleteSetting( NULL, "MobileEnabled" );
	MSN_DeleteSetting( NULL, "MobileAllowed" );

	if (MSN_GetStaticString( "LoginServer", NULL, evtname, sizeof( evtname )) == 0 &&
		(strcmp(evtname, MSN_DEFAULT_LOGIN_SERVER) == 0 ||
		strcmp(evtname, MSN_DEFAULT_GATEWAY) == 0))
		MSN_DeleteSetting( NULL, "LoginServer" );

	mailsoundname = ( char* )mir_alloc( 64 );
	mir_snprintf(mailsoundname, 64, "%s:Hotmail", protocolname);
	SkinAddNewSoundEx(mailsoundname, protocolname, MSN_Translate( "Live Mail" ));
	alertsoundname = ( char* )mir_alloc( 64 );
	mir_snprintf(alertsoundname, 64, "%s:Alerts", protocolname);
	SkinAddNewSoundEx(alertsoundname, protocolname, MSN_Translate( "Live Alert" ));

	msnStatusMode = msnDesiredStatus = ID_STATUS_OFFLINE;
	memset(&msnCurrentMedia, 0, sizeof(msnCurrentMedia));

	msnLoggedIn = false;
	LoadMsnServices();
	MsnInitIcons();
	MsgQueue_Init();
	P2pSessions_Init();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Unload a plugin

extern "C" int __declspec( dllexport ) Unload( void )
{
	int i;

	for ( i=0; i < arHooks.getCount(); i++ )
		UnhookEvent( arHooks[i] );
	arHooks.destroy();

	if ( hInitChat )
		DestroyHookableEvent( hInitChat );

	if ( hMSNNudge )
		DestroyHookableEvent( hMSNNudge );

	UnloadMsnServices();

	UninitSsl();
	MSN_FreeGroups();
	Threads_Uninit();
	MsgQueue_Uninit();
	P2pSessions_Uninit();
	CachedMsg_Uninit();
	Netlib_CloseHandle( hNetlibUser );

	mir_free( mailsoundname );
	mir_free( alertsoundname );
	mir_free( msnProtocolName );
	mir_free( ModuleName );

	CloseHandle( msnMainThread );

	for ( i=0; i < MSN_NUM_MODES; i++ )
		if ( msnModeMsgs[ i ].m_msg )
			mir_free( msnModeMsgs[ i ].m_msg );

	mir_free( sid );
	mir_free( passport );
	mir_free( MSPAuth );
	mir_free( rru );
	mir_free( profileURL );
	mir_free( profileURLId );
	mir_free( urlId );

	mir_free( msnPreviousUUX );
	mir_free( msnExternalIP );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MirandaPluginInfoEx - returns an information about a plugin

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	if ( mirandaVersion < PLUGIN_MAKE_VERSION( 0, 7, 0, 0 )) {
		MessageBox( NULL, _T("The MSN protocol plugin cannot be loaded. It requires Miranda IM 0.7.0.1 or later."), _T("MSN Protocol Plugin"), MB_OK|MB_ICONWARNING|MB_SETFOREGROUND|MB_TOPMOST );
		return NULL;
	}

	return &pluginInfo;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MirandaPluginInterfaces - returns the protocol interface to the core
static const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};
extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}
