/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright(C) 2002-2004 George Hazan (modification) and Richard Hughes (original)

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

/////////////////////////////////////////////////////////////////////////////////////////
// Global variables 

int      uniqueEventId = 0;
int      msnSearchID = -1;
HANDLE   msnGetInfoContact = NULL;
char*    msnExternalIP = NULL;
HANDLE   msnMainThread;
int      msnOtherContactsBlocked = 0;
HANDLE   hHookOnUserInfoInit = NULL;
bool     msnRunningUnderNT = false;

MYOPTIONS MyOptions;

char* msnProtocolName = NULL;

int	msnProtVersion = 0;
char* msnProtChallenge = NULL;
char* msnProductID  = NULL;

char* mailsoundname;
char* ModuleName;

PLUGININFO pluginInfo =
{
	sizeof(PLUGININFO),
	"MSN Protocol",
	__VERSION_DWORD,
	"Adds support for communicating with users of the MSN Messenger network",
	"George Hazan",
	"george_hazan@hotmail.com",
	"© 2001-4 Richard Hughes, George Hazan",
	"http://miranda-im.org/download/details.php?action=viewfile&id=702",
	0,	0
};

volatile bool		msnLoggedIn;
volatile HANDLE	msnNSSocket;

int				msnStatusMode,
					msnDesiredStatus;
HANDLE			msnMenuItems[ MENU_ITEMS_COUNT ];
HANDLE			hNetlibUser = NULL;
bool				msnUseExtendedPopups;

int MsnOnDetailsInit( WPARAM wParam, LPARAM lParam );

/////////////////////////////////////////////////////////////////////////////////////////
//	Main DLL function

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
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

#define NEWSTR_ALLOCA(A) (A==NULL)?NULL:strcpy((char*)alloca(strlen(A)+1),A)

static char* rtrim( char *string )
{
   char* p = string + strlen( string ) - 1;

   while ( p >= string )
   {  if ( *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' )
         break;

		*p-- = 0;
   }
   return string;
}

static int OnModulesLoaded( WPARAM wParam, LPARAM lParam )
{
	char szModule[ 100 ], szBuffer[ 100 ];
	strupr( strcpy( szModule, msnProtocolName ));

	WORD wPort = MSN_GetWord( NULL, "YourPort", 0xFFFF );
	if ( wPort != 0xFFFF ) {
		MSN_SetByte( "NLSpecifyIncomingPorts", 1 );

		ltoa( wPort, szBuffer, 10 );
		MSN_SetString( NULL, "NLIncomingPorts", szBuffer );

		DBDeleteContactSetting( NULL, szModule, "YourPort" );
	}

	_snprintf( szBuffer, sizeof szBuffer, "%s plugin connections", msnProtocolName );

	NETLIBUSER nlu = {0};
	nlu.cbSize = sizeof( nlu );
	nlu.flags = NUF_INCOMING | NUF_OUTGOING | NUF_HTTPCONNS;
	nlu.szSettingsModule = szModule;
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
		if ( RegOpenKey( HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", &hSettings ))
			return 0;

		char tValue[ 256 ];
		DWORD tType = REG_SZ, tValueLen = sizeof( tValue );
		int tResult = RegQueryValueEx( hSettings, "ProxyServer", NULL, &tType, ( BYTE* )tValue, &tValueLen );
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

	msnUseExtendedPopups = ServiceExists( MS_POPUP_ADDPOPUPEX ) != 0;
	hHookOnUserInfoInit = HookEvent( ME_USERINFO_INITIALISE, MsnOnDetailsInit );
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

int __declspec(dllexport) Load( PLUGINLINK* link )
{
	pluginLink = link;
	DuplicateHandle( GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &msnMainThread, THREAD_SET_CONTEXT, FALSE, 0 );

	char path[MAX_PATH];
	char* protocolname;
	char* fend;

	GetModuleFileName( hInst, path, sizeof( path ));

	protocolname = strrchr(path,'\\');
	protocolname++;
	fend = strrchr(path,'.');
	*fend = '\0';
	CharUpper( protocolname );
	msnProtocolName = strdup( protocolname );

	_snprintf( path, sizeof( path ), "%s:HotmailNotify", protocolname );
	ModuleName = strdup( path );

//	Uninstalling purposes
//	if (ServiceExists("PluginSweeper/Add"))
//		MSN_CallService("PluginSweeper/Add",(WPARAM)MSN_Translate(ModuleName),(LPARAM)ModuleName);

	HookEvent( ME_SYSTEM_MODULESLOADED, OnModulesLoaded );

	srand(( unsigned int )time( NULL ));
	msnRunningUnderNT = ( GetVersion() & 0x80000000 ) == 0;

	LoadOptions();
	HookEvent( ME_OPT_INITIALISE, MsnOptInit );
	HookEvent( ME_SYSTEM_PRESHUTDOWN, OnPreShutdown );

	MSN_InitThreads();

	PROTOCOLDESCRIPTOR pd;
	memset( &pd, 0, sizeof( pd ));
	pd.cbSize = sizeof( pd );
	pd.szName = msnProtocolName;
	pd.type = PROTOTYPE_PROTOCOL;
	MSN_CallService( MS_PROTO_REGISTERMODULE, 0, ( LPARAM )&pd );

	//set all contacts to 'offline'
	{
		HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		while ( hContact != NULL )
		{
			if ( !lstrcmp( msnProtocolName, ( char* )MSN_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact,0 )))
				MSN_SetWord( hContact, "Status", ID_STATUS_OFFLINE );

			hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT,( WPARAM )hContact, 0 );
	}	}

	char mailsoundtemp[ 64 ];
	strcpy( mailsoundtemp, protocolname );
	strcat( mailsoundtemp, ": " );
	strcat( mailsoundtemp,  MSN_Translate( "Hotmail" ));
	mailsoundname = strdup( mailsoundtemp );

	SkinAddNewSound( mailsoundtemp, mailsoundtemp, "hotmail.wav" );

	msnStatusMode = msnDesiredStatus = ID_STATUS_OFFLINE;
	msnLoggedIn = 0;
	MSN_WS_Init();
	LoadMsnServices();
	Lists_Init();
	MsgQueue_Init();
	P2pSessions_Init();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Unload a plugin

int __declspec( dllexport ) Unload( void )
{
	if ( msnLoggedIn )
		MSN_SendPacket( msnNSSocket, "OUT", NULL );

	if ( hHookOnUserInfoInit )
		UnhookEvent( hHookOnUserInfoInit );

	Threads_Uninit();
	MSN_WS_CleanUp();
	MsgQueue_Uninit();
	Lists_Uninit();
	P2pSessions_Uninit();
	Netlib_CloseHandle( hNetlibUser );

	UnloadMsnServices();

	free( mailsoundname );
	free( msnProtocolName );
	free( ModuleName );

	CloseHandle( msnMainThread );

	if ( kv ) free( kv );
	if ( sid ) free( sid );
	if ( passport ) free( passport );
	if ( MSPAuth ) free( MSPAuth );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MirandaPluginInfo - returns an information about a plugin

__declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	if ( mirandaVersion < PLUGIN_MAKE_VERSION( 0, 1, 2, 0 ))
		return NULL;

	return &pluginInfo;
}
