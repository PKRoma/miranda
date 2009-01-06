/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-09  George Hazan
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
#include "jabber_iq.h"
#include "jabber_caps.h"
#include "jabber_rc.h"

#include <m_fontservice.h>
#include <m_icolib.h>

#include "sdk/m_assocmgr.h"
#include "sdk/m_folders.h"
#include "sdk/m_wizard.h"
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
XML_API         xi;

CLIST_INTERFACE* pcli;

/////////////////////////////////////////////////////////////////////////////
// Theme API
BOOL (WINAPI *JabberAlphaBlend)(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION) = NULL;
BOOL (WINAPI *JabberIsThemeActive)() = NULL;
HRESULT (WINAPI *JabberDrawThemeParentBackground)(HWND, HDC, RECT *) = NULL;
/////////////////////////////////////////////////////////////////////////////

HANDLE hMainThread = NULL;
DWORD  jabberMainThreadId;
BOOL   jabberChatDllPresent = FALSE;
HANDLE hModulesLoaded, hModulesLoadedTB;

void JabberUserInfoInit(void);

int bSecureIM;

/////////////////////////////////////////////////////////////////////////////
// Protocol instances
static int sttCompareProtocols(const CJabberProto *p1, const CJabberProto *p2)
{
	return lstrcmp(p1->m_tszUserName, p2->m_tszUserName);
}

LIST<CJabberProto> g_Instances(1, sttCompareProtocols);
/////////////////////////////////////////////////////////////////////////////

extern "C" BOOL WINAPI DllMain( HINSTANCE hModule, DWORD, LPVOID )
{
	#ifdef _DEBUG
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif
	hInst = hModule;
	return TRUE;
}

extern "C" __declspec( dllexport ) PLUGININFOEX *MirandaPluginInfoEx( DWORD mirandaVersion )
{
	if ( mirandaVersion < PLUGIN_MAKE_VERSION( 0,8,0,21 )) {
		MessageBoxA( NULL, "The Jabber protocol plugin cannot be loaded. It requires Miranda IM 0.8.0.21 or later.", "Jabber Protocol Plugin", MB_OK|MB_ICONWARNING|MB_SETFOREGROUND|MB_TOPMOST );
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

int __cdecl CJabberProto::OnPreShutdown( WPARAM, LPARAM )
{
	UI_SAFE_CLOSE_HWND(m_hwndAgentRegInput);
	UI_SAFE_CLOSE_HWND(m_hwndRegProgress);
	UI_SAFE_CLOSE_HWND(m_hwndMucVoiceList);
	UI_SAFE_CLOSE_HWND(m_hwndMucMemberList);
	UI_SAFE_CLOSE_HWND(m_hwndMucModeratorList);
	UI_SAFE_CLOSE_HWND(m_hwndMucBanList);
	UI_SAFE_CLOSE_HWND(m_hwndMucAdminList);
	UI_SAFE_CLOSE_HWND(m_hwndMucOwnerList);
	UI_SAFE_CLOSE_HWND(m_hwndJabberChangePassword);
	UI_SAFE_CLOSE_HWND(m_hwndJabberAddBookmark);
	UI_SAFE_CLOSE_HWND(m_hwndPrivacyRule);

	UI_SAFE_CLOSE(m_pDlgPrivacyLists);
	UI_SAFE_CLOSE(m_pDlgBookmarks);
	UI_SAFE_CLOSE(m_pDlgServiceDiscovery);
	UI_SAFE_CLOSE(m_pDlgJabberJoinGroupchat);

	m_iqManager.ExpireAll();
	m_iqManager.Shutdown();
	ConsoleUninit();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// OnModulesLoaded - execute some code when all plugins are initialized

static int g_SvcParseXmppUri(WPARAM w, LPARAM l)
{
	if (CJabberProto *ppro = JabberChooseInstance(false))
		return ppro->JabberServiceParseXmppURI(w, l);
	return 0;
}

static int OnModulesLoaded( WPARAM, LPARAM )
{
	bSecureIM = (ServiceExists("SecureIM/IsContactSecured"));

	// file associations manager plugin support
	if ( ServiceExists( MS_ASSOCMGR_ADDNEWURLTYPE )) {
		CreateServiceFunction("JABBER/*" JS_PARSE_XMPP_URI, g_SvcParseXmppUri );
		AssocMgr_AddNewUrlTypeT( "xmpp:", TranslateT("Jabber Link Protocol"), hInst, IDI_JABBER, "JABBER/*" JS_PARSE_XMPP_URI, 0 );
	}

	{
		// init fontservice for info frame
		FontID fontid = {0};
		fontid.cbSize = sizeof(fontid);
		lstrcpyA(fontid.group, "Jabber");
		lstrcpyA(fontid.dbSettingsGroup, GLOBAL_SETTING_MODULE);
		fontid.flags = FIDF_DEFAULTVALID;

		fontid.deffontsettings.charset = DEFAULT_CHARSET;
		fontid.deffontsettings.colour = GetSysColor(COLOR_WINDOWTEXT);
		fontid.deffontsettings.size = -11;
		lstrcpyA(fontid.deffontsettings.szFace, "MS Shell Dlg");
		fontid.deffontsettings.style = 0;

		lstrcpyA(fontid.name, "Frame title");
		lstrcpyA(fontid.prefix, "fntFrameTitle");
		fontid.deffontsettings.style = DBFONTF_BOLD;
		CallService(MS_FONT_REGISTER, (WPARAM)&fontid, 0);

		lstrcpyA(fontid.name, "Frame text");
		lstrcpyA(fontid.prefix, "fntFrameClock");
		fontid.deffontsettings.style = 0;
		CallService(MS_FONT_REGISTER, (WPARAM)&fontid, 0);

		ColourID colourid = {0};
		colourid.cbSize = sizeof(colourid);
		lstrcpyA(colourid.group, "Jabber");
		lstrcpyA(colourid.dbSettingsGroup, GLOBAL_SETTING_MODULE);

		lstrcpyA(colourid.name, "Background");
		lstrcpyA(colourid.setting, "clFrameBack");
		colourid.defcolour = GetSysColor(COLOR_WINDOW);
		CallService(MS_COLOUR_REGISTER, (WPARAM)&colourid, 0);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// OnLoad - initialize the plugin instance

static CJabberProto* jabberProtoInit( const char* pszProtoName, const TCHAR* tszUserName )
{
	CJabberProto *ppro = new CJabberProto( pszProtoName, tszUserName );
	g_Instances.insert(ppro);
	return ppro;
}

static int jabberProtoUninit( CJabberProto* ppro )
{
	g_Instances.remove(ppro);
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
	mir_getXI( &xi );

	pcli = ( CLIST_INTERFACE* )CallService(MS_CLIST_RETRIEVE_INTERFACE, 0, (LPARAM)hInst);
	if ( (int)pcli == CALLSERVICE_NOTFOUND ) {
LBL_Error:
		MessageBoxA( NULL, "This version of plugin requires Miranda IM 0.8.0.9 or later", "Fatal error", MB_OK );
		return 1;
	}
	if ( pcli->version < 6 )
		goto LBL_Error;

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

	g_IconsInit();
	hModulesLoaded = HookEvent(ME_SYSTEM_MODULESLOADED, OnModulesLoaded);
	hModulesLoadedTB = HookEvent(ME_TB_MODULELOADED, g_OnModernToolbarInit);
	JabberUserInfoInit();

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Unload - destroy the plugin instance

extern "C" int __declspec( dllexport ) Unload( void )
{
	UnhookEvent(hModulesLoaded);
	UnhookEvent(hModulesLoadedTB);

	if ( hMainThread )
		CloseHandle( hMainThread );

	g_Instances.destroy();
	return 0;
}
