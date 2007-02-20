/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber.cpp,v $
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_ssl.h"
#include "jabber_iq.h"
#include "resource.h"
#include "version.h"
#include "sdk/m_icolib.h"

HINSTANCE hInst;
PLUGINLINK *pluginLink;

PLUGININFO pluginInfo = {
	sizeof( PLUGININFO ),
	#if defined( _UNICODE )
		"Jabber Protocol (Unicode)",
	#else
		"Jabber Protocol",
	#endif
	__VERSION_DWORD,
	"Jabber protocol plugin for Miranda IM ( "__DATE__" )",
	"George Hazan",
	"ghazan@miranda-im.org",
	"( c ) 2002-05 Santithorn Bunchua, George Hazan",
	"http://miranda-im.org/download/details.php?action=viewfile&id=437",
	UNICODE_AWARE,
	0
};

MM_INTERFACE    mmi;
LIST_INTERFACE  li;
UTF8_INTERFACE  utfi;
MD5_INTERFACE   md5i;
SHA1_INTERFACE  sha1i;

HANDLE hMainThread = NULL;
DWORD jabberMainThreadId;
char* jabberProtoName;	// "JABBER"
char* jabberModuleName;	// "Jabber"
HANDLE hNetlibUser;
// Main jabber server connection thread global variables
ThreadData* jabberThreadInfo = NULL;
BOOL   jabberConnected = FALSE;
time_t jabberLoggedInTime = 0;
BOOL   jabberOnline = FALSE;
BOOL   jabberChatDllPresent = FALSE;
int    jabberStatus = ID_STATUS_OFFLINE;
int    jabberDesiredStatus;
BOOL   modeMsgStatusChangePending = FALSE;
BOOL   jabberChangeStatusMessageOnly = FALSE;
TCHAR* jabberJID = NULL;
char*  streamId = NULL;
DWORD  jabberLocalIP;
UINT   jabberCodePage;
int    jabberSearchID;
JABBER_MODEMSGS modeMsgs;
CRITICAL_SECTION modeMsgMutex;
char* jabberVcardPhotoFileName = NULL;
char* jabberVcardPhotoType = NULL;
BOOL  jabberSendKeepAlive;

// SSL-related global variable
HMODULE hLibSSL = NULL;
PVOID jabberSslCtx;

const char xmlnsAdmin[] = "http://jabber.org/protocol/muc#admin";
const char xmlnsOwner[] = "http://jabber.org/protocol/muc#owner";

HWND hwndJabberAgents = NULL;
HWND hwndJabberGroupchat = NULL;
HWND hwndJabberJoinGroupchat = NULL;
HWND hwndAgentReg = NULL;
HWND hwndAgentRegInput = NULL;
HWND hwndAgentManualReg = NULL;
HWND hwndRegProgress = NULL;
HWND hwndJabberVcard = NULL;
HWND hwndMucVoiceList = NULL;
HWND hwndMucMemberList = NULL;
HWND hwndMucModeratorList = NULL;
HWND hwndMucBanList = NULL;
HWND hwndMucAdminList = NULL;
HWND hwndMucOwnerList = NULL;
HWND hwndJabberChangePassword = NULL;
HWND hwndJabberBookmarks = NULL;
HWND hwndJabberAddBookmark = NULL;

// Service and event handles
HANDLE heventRawXMLIn;
HANDLE heventRawXMLOut;

HANDLE hInitChat = NULL;

static int compareTransports( const TCHAR* p1, const TCHAR* p2 )
{	return _tcsicmp( p1, p2 );
}
LIST<TCHAR> jabberTransports( 50, compareTransports );

static int sttCompareHandles( const void* p1, const void* p2 )
{	return (long)p1 - (long)p2;
}
LIST<void> arHooks( 20, sttCompareHandles ); 

int JabberOptInit( WPARAM wParam, LPARAM lParam );
int JabberUserInfoInit( WPARAM wParam, LPARAM lParam );
int JabberMsgUserTyping( WPARAM wParam, LPARAM lParam );
void JabberMenuInit( void );
int JabberSvcInit( void );
int JabberSvcUninit( void );

int bSecureIM;
extern "C" BOOL WINAPI DllMain( HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved )
{
	#ifdef _DEBUG
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif
	hInst = hModule;
	return TRUE;
}

extern "C" __declspec( dllexport ) PLUGININFO *MirandaPluginInfo( DWORD mirandaVersion )
{
	if ( mirandaVersion < PLUGIN_MAKE_VERSION( 0,7,0,3 )) {
		MessageBoxA( NULL, "The Jabber protocol plugin cannot be loaded. It requires Miranda IM 0.7.0.3 or later.", "Jabber Protocol Plugin", MB_OK|MB_ICONWARNING|MB_SETFOREGROUND|MB_TOPMOST );
		return NULL;
	}

	return &pluginInfo;
}

///////////////////////////////////////////////////////////////////////////////
// OnPreShutdown - prepares Miranda to be shut down

static int OnPreShutdown( WPARAM wParam, LPARAM lParam )
{
	if ( hwndJabberAgents ) SendMessage( hwndJabberAgents, WM_CLOSE, 0, 0 );
	if ( hwndJabberGroupchat ) SendMessage( hwndJabberGroupchat, WM_CLOSE, 0, 0 );
	if ( hwndJabberJoinGroupchat ) SendMessage( hwndJabberJoinGroupchat, WM_CLOSE, 0, 0 );
	if ( hwndAgentReg ) SendMessage( hwndAgentReg, WM_CLOSE, 0, 0 );
	if ( hwndAgentRegInput ) SendMessage( hwndAgentRegInput, WM_CLOSE, 0, 0 );
	if ( hwndRegProgress ) SendMessage( hwndRegProgress, WM_CLOSE, 0, 0 );
	if ( hwndJabberVcard ) SendMessage( hwndJabberVcard, WM_CLOSE, 0, 0 );
	if ( hwndMucVoiceList ) SendMessage( hwndMucVoiceList, WM_CLOSE, 0, 0 );
	if ( hwndMucMemberList ) SendMessage( hwndMucMemberList, WM_CLOSE, 0, 0 );
	if ( hwndMucModeratorList ) SendMessage( hwndMucModeratorList, WM_CLOSE, 0, 0 );
	if ( hwndMucBanList ) SendMessage( hwndMucBanList, WM_CLOSE, 0, 0 );
	if ( hwndMucAdminList ) SendMessage( hwndMucAdminList, WM_CLOSE, 0, 0 );
	if ( hwndMucOwnerList ) SendMessage( hwndMucOwnerList, WM_CLOSE, 0, 0 );
	if ( hwndJabberChangePassword ) SendMessage( hwndJabberChangePassword, WM_CLOSE, 0, 0 );
	if ( hwndJabberBookmarks ) SendMessage( hwndJabberBookmarks, WM_CLOSE, 0, 0 );
	if ( hwndJabberAddBookmark ) SendMessage( hwndJabberAddBookmark, WM_CLOSE, 0, 0 );

	hwndJabberAgents = NULL;
	hwndJabberGroupchat = NULL;
	hwndJabberJoinGroupchat = NULL;
	hwndAgentReg = NULL;
	hwndAgentRegInput = NULL;
	hwndAgentManualReg = NULL;
	hwndRegProgress = NULL;
	hwndJabberVcard = NULL;
	hwndMucVoiceList = NULL;
	hwndMucMemberList = NULL;
	hwndMucModeratorList = NULL;
	hwndMucBanList = NULL;
	hwndMucAdminList = NULL;
	hwndMucOwnerList = NULL;
	hwndJabberChangePassword = NULL;
	hwndJabberBookmarks = NULL;
	hwndJabberAddBookmark = NULL;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// OnModulesLoaded - execute some code when all plugins are initialized

int JabberGcEventHook( WPARAM, LPARAM );
int JabberGcMenuHook( WPARAM, LPARAM );
int JabberGcInit( WPARAM, LPARAM );

int JabberContactDeleted( WPARAM wParam, LPARAM lParam );
int JabberDbSettingChanged( WPARAM wParam, LPARAM lParam );
int JabberMenuPrebuildContactMenu( WPARAM wParam, LPARAM lParam );
void JabberMenuHideSrmmIcon(HANDLE hContact);
int JabberMenuProcessSrmmIconClick( WPARAM wParam, LPARAM lParam );
int JabberMenuProcessSrmmEvent( WPARAM wParam, LPARAM lParam );

static COLORREF crCols[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

static int OnModulesLoaded( WPARAM wParam, LPARAM lParam )
{
	JabberMenuInit();
	JabberWsInit();
	HookEvent( ME_USERINFO_INITIALISE, JabberUserInfoInit );
	bSecureIM = (ServiceExists("SecureIM/IsContactSecured"));

	if ( ServiceExists( MS_GC_REGISTER )) {
		jabberChatDllPresent = true;

		GCREGISTER gcr = {0};
		gcr.cbSize = sizeof( GCREGISTER );
		gcr.dwFlags = GC_TYPNOTIF|GC_CHANMGR;
		gcr.iMaxText = 0;
		gcr.nColors = 16;
		gcr.pColors = &crCols[0];
		gcr.pszModuleDispName = jabberProtoName;
		gcr.pszModule = jabberProtoName;
		JCallService( MS_GC_REGISTER, NULL, ( LPARAM )&gcr );

		arHooks.insert( HookEvent( ME_GC_EVENT, JabberGcEventHook ));
		arHooks.insert( HookEvent( ME_GC_BUILDMENU, JabberGcMenuHook ));

		char szEvent[ 200 ];
		mir_snprintf( szEvent, sizeof szEvent, "%s\\ChatInit", jabberProtoName );
		hInitChat = CreateHookableEvent( szEvent );
		arHooks.insert( HookEvent( szEvent, JabberGcInit ));
	}

	JCreateServiceFunction( JS_GETADVANCEDSTATUSICON, JGetAdvancedStatusIcon );
	arHooks.insert( HookEvent( ME_SKIN2_ICONSCHANGED, ReloadIconsEventHook ));
	arHooks.insert( HookEvent( ME_DB_CONTACT_SETTINGCHANGED, JabberDbSettingChanged ));
	arHooks.insert( HookEvent( ME_DB_CONTACT_DELETED, JabberContactDeleted ));
	arHooks.insert( HookEvent( ME_CLIST_PREBUILDCONTACTMENU, JabberMenuPrebuildContactMenu ));

	if ( ServiceExists( MS_MSG_ADDICON )) {
		StatusIconData sid = {0};
		sid.cbSize = sizeof(sid);
		sid.szModule = jabberProtoName;
		sid.hIcon = LoadIconEx("main");
		sid.hIconDisabled = LoadIconEx("main");
		sid.flags = MBF_HIDDEN;
		sid.szTooltip = "Jabber Resource";
		CallService(MS_MSG_ADDICON, 0, (LPARAM) &sid);
		arHooks.insert( HookEvent( ME_MSG_ICONPRESSED, JabberMenuProcessSrmmIconClick ));
		arHooks.insert( HookEvent( ME_MSG_WINDOWEVENT, JabberMenuProcessSrmmEvent ));

		HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		while ( hContact != NULL ) {
			char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
			if ( szProto != NULL && !strcmp( szProto, jabberProtoName ))
				JabberMenuHideSrmmIcon(hContact);
			hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
	}	}

	JabberCheckAllContactsAreTransported();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// OnLoad - initialize the plugin instance

extern "C" int __declspec( dllexport ) Load( PLUGINLINK *link )
{
	pluginLink = link;

	// set the memory, lists & utf8 managers
	mir_getMMI( &mmi );
	mir_getLI( &li );
	mir_getUTFI( &utfi );
	mir_getMD5I( &md5i );
	mir_getSHA1I( &sha1i );

	// creating the plugins name
	char text[_MAX_PATH];
	char* p, *q;

	GetModuleFileNameA( hInst, text, sizeof( text ));
	p = strrchr( text, '\\' );
	p++;
	q = strrchr( p, '.' );
	*q = '\0';
	jabberProtoName = mir_strdup( p );
	_strupr( jabberProtoName );

	mir_snprintf( text, sizeof( text ), "%s/Status", jabberProtoName );
	JCallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )text );

	jabberModuleName = mir_strdup( jabberProtoName );
	_strlwr( jabberModuleName );
	jabberModuleName[0] = toupper( jabberModuleName[0] );

	JabberLog( "Setting protocol/module name to '%s/%s'", jabberProtoName, jabberModuleName );

	DuplicateHandle( GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &hMainThread, THREAD_SET_CONTEXT, FALSE, 0 );
	jabberMainThreadId = GetCurrentThreadId();

	arHooks.insert( HookEvent( ME_OPT_INITIALISE, JabberOptInit ));
	arHooks.insert( HookEvent( ME_SYSTEM_MODULESLOADED, OnModulesLoaded ));
	arHooks.insert( HookEvent( ME_SYSTEM_PRESHUTDOWN, OnPreShutdown ));

	// Register protocol module
	PROTOCOLDESCRIPTOR pd;
	ZeroMemory( &pd, sizeof( PROTOCOLDESCRIPTOR ));
	pd.cbSize = sizeof( PROTOCOLDESCRIPTOR );
	pd.szName = jabberProtoName;
	pd.type = PROTOTYPE_PROTOCOL;
	JCallService( MS_PROTO_REGISTERMODULE, 0, ( LPARAM )&pd );

	// Set all contacts to offline
	HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) {
		char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
		if ( szProto != NULL && !strcmp( szProto, jabberProtoName )) {
			if ( JGetWord( hContact, "Status", ID_STATUS_OFFLINE ) != ID_STATUS_OFFLINE )
				JSetWord( hContact, "Status", ID_STATUS_OFFLINE );

			if ( JGetByte( hContact, "IsTransport", 0 )) {
				DBVARIANT dbv;
				if ( !JGetStringT( hContact, "jid", &dbv )) {
					TCHAR* domain = NEWTSTR_ALLOCA(dbv.ptszVal);
					TCHAR* resourcepos = _tcschr( domain, '/' );
					if ( resourcepos != NULL )
						*resourcepos = '\0';
					jabberTransports.insert( _tcsdup( domain ));
					JFreeVariant( &dbv );
		}	}	}

		hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
	}

	memset(( char* )&modeMsgs, 0, sizeof( JABBER_MODEMSGS ));
	jabberCodePage = JGetWord( NULL, "CodePage", CP_ACP );

	InitializeCriticalSection( &modeMsgMutex );

	srand(( unsigned ) time( NULL ));
	JabberSerialInit();
	JabberIqInit();
	JabberListInit();
	JabberIconsInit();
	JabberSvcInit();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Unload - destroy the plugin instance

extern "C" int __declspec( dllexport ) Unload( void )
{
	int i;
	for ( i=0; i < arHooks.getCount(); i++ )
		UnhookEvent( arHooks[i] );
	arHooks.destroy();

	if ( hInitChat )
		DestroyHookableEvent( hInitChat );

	JabberSvcUninit();
	JabberSslUninit();
	JabberListUninit();
	JabberIqUninit();
	JabberSerialUninit();
	JabberWsUninit();
	DeleteCriticalSection( &modeMsgMutex );
	mir_free( modeMsgs.szOnline );
	mir_free( modeMsgs.szAway );
	mir_free( modeMsgs.szNa );
	mir_free( modeMsgs.szDnd );
	mir_free( modeMsgs.szFreechat );
	mir_free( jabberModuleName );
	mir_free( jabberProtoName );
	if ( jabberVcardPhotoFileName ) {
		DeleteFileA( jabberVcardPhotoFileName );
		mir_free( jabberVcardPhotoFileName );
	}
	if ( jabberVcardPhotoType ) mir_free( jabberVcardPhotoType );
	if ( streamId ) mir_free( streamId );

	for ( i=0; i < jabberTransports.getCount(); i++ )
		free( jabberTransports[i] );
	jabberTransports.destroy();

	if ( hMainThread ) CloseHandle( hMainThread );
	return 0;
}
