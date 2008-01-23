/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

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

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_ssl.h"
#include "jabber_iq.h"
#include "jabber_caps.h"
#include "jabber_rc.h"
#include "resource.h"
#include "version.h"

#include "sdk/m_icolib.h"
#include "sdk/m_folders.h"
#include "sdk/m_wizard.h"
#include "sdk/m_assocmgr.h"
#include "sdk/m_toolbar.h"

HINSTANCE hInst;
PLUGINLINK *pluginLink;

PLUGININFOEX pluginInfo = {
	sizeof( PLUGININFOEX ),
	"Jabber Protocol",
	__VERSION_DWORD,
	"Jabber protocol plugin for Miranda IM ( "__DATE__" )",
	"George Hazan, Maxim Mluhov, Victor Pavlychko, Artem Shpynov, Michael Stepura",
	"ghazan@miranda-im.org",
	"(c) 2005-07 George Hazan, Maxim Mluhov, Victor Pavlychko, Artem Shpynov, Michael Stepura",
	"http://miranda-im.org",
	UNICODE_AWARE,
	0,
    #if defined( _UNICODE )
    {0x1ee5af12, 0x26b0, 0x4290, { 0x8f, 0x97, 0x16, 0x77, 0xcb, 0xe, 0xfd, 0x2b }} //{1EE5AF12-26B0-4290-8F97-1677CB0EFD2B}
    #else
    {0xf7f5861d, 0x988d, 0x479d, { 0xa5, 0xbb, 0x80, 0xc7, 0xfa, 0x8a, 0xd0, 0xef }} //{F7F5861D-988D-479d-A5BB-80C7FA8AD0EF}
    #endif
};

MM_INTERFACE    mmi;
LIST_INTERFACE  li;
UTF8_INTERFACE  utfi;
MD5_INTERFACE   md5i;
SHA1_INTERFACE  sha1i;

/////////////////////////////////////////////////////////////////////////////
// Theme API
BOOL (WINAPI *JabberAlphaBlend)(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION) = NULL;
BOOL (WINAPI *JabberIsThemeActive)() = NULL;
HRESULT (WINAPI *JabberDrawThemeParentBackground)(HWND, HDC, RECT *) = NULL;
/////////////////////////////////////////////////////////////////////////////

HANDLE hMainThread = NULL;
DWORD  jabberMainThreadId;
BOOL   jabberChatDllPresent = FALSE;
HANDLE hNetlibUser;

// SSL-related global variable
HMODULE hLibSSL = NULL;
PVOID jabberSslCtx;

const char xmlnsAdmin[] = "http://jabber.org/protocol/muc#admin";
const char xmlnsOwner[] = "http://jabber.org/protocol/muc#owner";

static int sttCompareHandles( const void* p1, const void* p2 )
{	return (long)p1 - (long)p2;
}
LIST<void> arHooks( 20, sttCompareHandles );
LIST<void> arServices( 20, sttCompareHandles );

void JabberUserInfoInit(void);

int bSecureIM;

extern "C" BOOL WINAPI DllMain( HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved )
{
	#ifdef _DEBUG
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif
	hInst = hModule;
	return TRUE;
}

extern "C" __declspec( dllexport ) PLUGININFOEX *MirandaPluginInfoEx( DWORD mirandaVersion )
{
	if ( mirandaVersion < PLUGIN_MAKE_VERSION( 0,8,0,8 )) {
		MessageBoxA( NULL, "The Jabber protocol plugin cannot be loaded. It requires Miranda IM 0.8.0.8 or later.", "Jabber Protocol Plugin", MB_OK|MB_ICONWARNING|MB_SETFOREGROUND|MB_TOPMOST );
		return NULL;
	}

	return &pluginInfo;
}

static const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};
extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

///////////////////////////////////////////////////////////////////////////////
// OnPreShutdown - prepares Miranda to be shut down

int __cdecl CJabberProto::OnPreShutdown( WPARAM wParam, LPARAM lParam )
{
	if ( hwndJabberAgents ) {
		::SendMessage( hwndJabberAgents, WM_CLOSE, 0, 0 );
		hwndJabberAgents = NULL;
	}
	if ( hwndJabberGroupchat ) {
		::SendMessage( hwndJabberGroupchat, WM_CLOSE, 0, 0 );
		hwndJabberGroupchat = NULL;
	}
	if ( hwndJabberJoinGroupchat ) {
		::SendMessage( hwndJabberJoinGroupchat, WM_CLOSE, 0, 0 );
		hwndJabberJoinGroupchat = NULL;
	}
	if ( hwndAgentReg ) {
		::SendMessage( hwndAgentReg, WM_CLOSE, 0, 0 );
		hwndAgentReg = NULL;
	}
	if ( hwndAgentRegInput ) {
		::SendMessage( hwndAgentRegInput, WM_CLOSE, 0, 0 );
		hwndAgentRegInput = NULL;
	}
	if ( hwndRegProgress ) {
		::SendMessage( hwndRegProgress, WM_CLOSE, 0, 0 );
		hwndRegProgress = NULL;
	}
	if ( hwndJabberVcard ) {
		::SendMessage( hwndJabberVcard, WM_CLOSE, 0, 0 );
		hwndJabberVcard = NULL;
	}
	if ( hwndMucVoiceList ) {
		::SendMessage( hwndMucVoiceList, WM_CLOSE, 0, 0 );
		hwndMucVoiceList = NULL;
	}
	if ( hwndMucMemberList ) {
		::SendMessage( hwndMucMemberList, WM_CLOSE, 0, 0 );
		hwndMucMemberList = NULL;
	}
	if ( hwndMucModeratorList ) {
		::SendMessage( hwndMucModeratorList, WM_CLOSE, 0, 0 );
		hwndMucModeratorList = NULL;
	}
	if ( hwndMucBanList ) {
		::SendMessage( hwndMucBanList, WM_CLOSE, 0, 0 );
		hwndMucBanList = NULL;
	}
	if ( hwndMucAdminList ) {
		::SendMessage( hwndMucAdminList, WM_CLOSE, 0, 0 );
		hwndMucAdminList = NULL;
	}
	if ( hwndMucOwnerList ) {
		::SendMessage( hwndMucOwnerList, WM_CLOSE, 0, 0 );
		hwndMucOwnerList = NULL;
	}
	if ( hwndJabberChangePassword ) {
		::SendMessage( hwndJabberChangePassword, WM_CLOSE, 0, 0 );
		hwndJabberChangePassword = NULL;
	}
	if ( hwndJabberBookmarks ) {
		::SendMessage( hwndJabberBookmarks, WM_CLOSE, 0, 0 );
		hwndJabberBookmarks = NULL;
	}
	if ( hwndJabberAddBookmark ) {
		::SendMessage( hwndJabberAddBookmark, WM_CLOSE, 0, 0 );
		hwndJabberAddBookmark = NULL;
	}
	if ( hwndPrivacyRule ) {
		::SendMessage( hwndPrivacyRule, WM_CLOSE, 0, 0 );
		hwndPrivacyRule = NULL;
	}
	if ( hwndPrivacyLists ) {
		::SendMessage( hwndPrivacyLists, WM_CLOSE, 0, 0 );
		hwndPrivacyLists = NULL;
	}
	if ( hwndServiceDiscovery ) {
		::SendMessage( hwndServiceDiscovery, WM_CLOSE, 0, 0 );
		hwndServiceDiscovery = NULL;
	}
	hwndAgentManualReg = NULL;

	m_iqManager.ExpireAll();
	m_iqManager.Shutdown();
	JabberConsoleUninit();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// OnModulesLoaded - execute some code when all plugins are initialized

void JabberMenuHideSrmmIcon(HANDLE hContact);

static COLORREF crCols[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

static int OnModulesLoaded( WPARAM wParam, LPARAM lParam )
{
	bSecureIM = (ServiceExists("SecureIM/IsContactSecured"));
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// OnLoad - initialize the plugin instance

int CJabberProto::OnModulesLoadedEx( WPARAM wParam, LPARAM lParam )
{
	JHookEvent( ME_TB_MODULELOADED, &CJabberProto::OnModernToolbarInit );

	if ( ServiceExists( MS_GC_REGISTER )) {
		jabberChatDllPresent = true;

		GCREGISTER gcr = {0};
		gcr.cbSize = sizeof( GCREGISTER );
		gcr.dwFlags = GC_TYPNOTIF|GC_CHANMGR;
		gcr.iMaxText = 0;
		gcr.nColors = 16;
		gcr.pColors = &crCols[0];
		gcr.pszModuleDispName = szProtoName;
		gcr.pszModule = szProtoName;
		CallServiceSync( MS_GC_REGISTER, NULL, ( LPARAM )&gcr );

		JHookEvent( ME_GC_EVENT, &CJabberProto::JabberGcEventHook );
		JHookEvent( ME_GC_BUILDMENU, &CJabberProto::JabberGcMenuHook );

		char szEvent[ 200 ];
		mir_snprintf( szEvent, sizeof szEvent, "%s\\ChatInit", szProtoName );
		hInitChat = CreateHookableEvent( szEvent );
		JHookEvent( szEvent, &CJabberProto::JabberGcInit );
	}

	if ( ServiceExists( MS_MSG_ADDICON )) {
		StatusIconData sid = {0};
		sid.cbSize = sizeof(sid);
		sid.szModule = szProtoName;
		sid.hIcon = LoadIconEx("main");
		sid.hIconDisabled = LoadIconEx("main");
		sid.flags = MBF_HIDDEN;
		sid.szTooltip = "Jabber Resource";
		CallService(MS_MSG_ADDICON, 0, (LPARAM) &sid);
		JHookEvent( ME_MSG_ICONPRESSED, &CJabberProto::OnProcessSrmmIconClick );
		JHookEvent( ME_MSG_WINDOWEVENT, &CJabberProto::OnProcessSrmmEvent );

		HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		while ( hContact != NULL ) {
			char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
			if ( szProto != NULL && !strcmp( szProto, szProtoName ))
				JabberMenuHideSrmmIcon(hContact);
			hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
	}	}

	DBEVENTTYPEDESCR dbEventType = {0};
	dbEventType.cbSize = sizeof(DBEVENTTYPEDESCR);
	dbEventType.eventType = JABBER_DB_EVENT_TYPE_CHATSTATES;
	dbEventType.module = szProtoName;
	dbEventType.descr = "Chat state notifications";
	JCallService( MS_DB_EVENT_REGISTERTYPE, 0, (LPARAM)&dbEventType );

	// file associations manager plugin support
	if ( ServiceExists( MS_ASSOCMGR_ADDNEWURLTYPE )) {
		char szService[ MAXMODULELABELLENGTH ];
		mir_snprintf( szService, SIZEOF( szService ), "%s%s", szProtoName, JS_PARSE_XMPP_URI );
		AssocMgr_AddNewUrlTypeT( "xmpp:", TranslateT("Jabber Link Protocol"), hInst, IDI_JABBER, szService, 0 );
	}

	JabberCheckAllContactsAreTransported();

	// Set all contacts to offline
	HANDLE hContact = ( HANDLE )CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) {
		char* szProto = ( char* )CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
		if ( szProto != NULL && !strcmp( szProto, szProtoName )) {
			JabberSetContactOfflineStatus( hContact );

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

		hContact = ( HANDLE )CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
	}
	return 0;
}

static CJabberProto* jabberProtoInit( const char* szProtoName, const TCHAR* tszUserName )
{
	CJabberProto* ppro = new CJabberProto( szProtoName );
	if ( !ppro )
		return NULL;

	ppro->tszUserName = mir_tstrdup( tszUserName );

	ppro->JHookEvent( ME_SYSTEM_MODULESLOADED, &CJabberProto::OnModulesLoadedEx );

	char text[ MAX_PATH ];
	mir_snprintf( text, sizeof( text ), "%s/Status", ppro->szProtoName );
	JCallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )text );
	return ppro;
}

static int jabberProtoUninit( CJabberProto* ppro )
{
	delete ppro;
	return 0;
}

extern "C" int __declspec( dllexport ) Load( PLUGINLINK *link )
{
	pluginLink = link;

	// set the memory, lists & utf8 managers
	mir_getMMI( &mmi );
	mir_getLI( &li );
	mir_getUTFI( &utfi );
	mir_getMD5I( &md5i );
	mir_getSHA1I( &sha1i );

	DuplicateHandle( GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &hMainThread, THREAD_SET_CONTEXT, FALSE, 0 );
	jabberMainThreadId = GetCurrentThreadId();

	// Register protocol module
	PROTOCOLDESCRIPTOR pd;
	ZeroMemory( &pd, sizeof( PROTOCOLDESCRIPTOR ));
	pd.cbSize = sizeof( PROTOCOLDESCRIPTOR );
	pd.szName = "JABBER";
	pd.fnInit = ( pfnInitProto )jabberProtoInit;
	pd.fnUninit = ( pfnUninitProto )jabberProtoUninit;
	pd.type = PROTOTYPE_PROTOCOL;
	CallService( MS_PROTO_REGISTERMODULE, 0, ( LPARAM )&pd );

	// Load some fuctions
	HINSTANCE hDll;
	if ( hDll = LoadLibraryA("msimg32.dll" ))
		JabberAlphaBlend = (BOOL (WINAPI *)(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION)) GetProcAddress(hDll, "AlphaBlend");

	if ( IsWinVerXPPlus() ) {
		if ( hDll = GetModuleHandleA("uxtheme")) {
			JabberDrawThemeParentBackground = (HRESULT (WINAPI *)(HWND,HDC,RECT *))GetProcAddress(hDll, "DrawThemeParentBackground");
			JabberIsThemeActive = (BOOL (WINAPI *)())GetProcAddress(hDll, "IsThemeActive");
	}	}

	srand(( unsigned ) time( NULL ));
	JabberUserInfoInit();
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

	for ( i=0; i < arServices.getCount(); i++ )
		DestroyServiceFunction( arServices[i] );
	arServices.destroy();

	if ( hMainThread )
		CloseHandle( hMainThread );
	return 0;
}
