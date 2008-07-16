/*
IRC plugin for Miranda IM

Copyright (C) 2003-2005 Jurgen Persson
Copyright (C) 2007 George Hazan

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

#include "irc.h"
#include "version.h"

CIrcSession       g_ircSession;          // Representation of the IRC-connection
char*             IRCPROTONAME = NULL;
char*             ALTIRCPROTONAME = NULL;
char*             pszServerFile = NULL;
char              mirandapath[MAX_PATH];
DWORD             mirVersion = NULL;
CRITICAL_SECTION  cs;
CRITICAL_SECTION  m_gchook;
PLUGINLINK*       pluginLink;
HINSTANCE         g_hInstance = NULL;
PREFERENCES*      prefs;

HMODULE				m_ssleay32 = NULL;

UTF8_INTERFACE    utfi;
MM_INTERFACE		mmi;

// Information about the plugin
PLUGININFOEX pluginInfo =
{
	sizeof( PLUGININFOEX ),
	__PLUGIN_NAME,
	__VERSION_DWORD,
	__DESC,
	__AUTHOR,
	__AUTHOREMAIL,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	0,
	{0xb529402b, 0x53ba, 0x4c81, { 0x9e, 0x27, 0xd4, 0x31, 0xeb, 0xe8, 0xec, 0x36 }} //{B529402B-53BA-4c81-9E27-D431EBE8EC36}
};

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	g_hInstance=hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	if ( mirandaVersion < PLUGIN_MAKE_VERSION( 0, 7, 0, 0 )) {
		MessageBox( NULL, LPGENT("The IRC protocol plugin cannot be loaded. It requires Miranda IM 0.7.0.0 or later."), LPGENT("IRC Protocol Plugin"), MB_OK|MB_ICONWARNING|MB_SETFOREGROUND|MB_TOPMOST );
		return NULL;
	}
	mirVersion = mirandaVersion;
	return &pluginInfo;
}

static const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};

extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

static void GetModuleName( void )	 // ripped from msn
{
	GetModuleFileNameA(g_hInstance, mirandapath, MAX_PATH);
	char* p = strrchr( mirandapath, '\\' );
	if ( p ) {
		*p++ = '\0';
		char* p1 = strrchr( p, '.' );
		*p1 = '\0';
		char* p2 = p;
		while ( *p2 ) {
			if ( *p2 == ' ' )
				*p2 = '_';
			p2++;
		}

		IRCPROTONAME = strdup( p );
		ALTIRCPROTONAME = new char[lstrlenA( IRCPROTONAME ) + 7 ];
		CharUpperA(IRCPROTONAME);

		if ( lstrcmpiA( IRCPROTONAME, "IRC" ))
			mir_snprintf( ALTIRCPROTONAME, lstrlenA( IRCPROTONAME ) + 7 , "IRC (%s)", IRCPROTONAME );
		else
			mir_snprintf( ALTIRCPROTONAME, lstrlenA( IRCPROTONAME ) + 7 , "%s", IRCPROTONAME );
}	}

extern "C" int __declspec(dllexport) Load( PLUGINLINK *link )
{
	#ifndef NDEBUG
		int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
		flag |= _CRTDBG_LEAK_CHECK_DF;
		_CrtSetDbgFlag(flag);
	#endif

	pluginLink=link;
	mir_getMMI( &mmi );
	mir_getUTFI( &utfi );

	if ( !mirVersion || mirVersion < PLUGIN_MAKE_VERSION( 0, 7, 0 ,0 )) {
		TCHAR szVersion[] = _T("0.7"); // minimum required version
		TCHAR szText[] = LPGENT("The IRC protocol could not be loaded as it is dependant on Miranda IM version %s or later.\n\nDo you want to download an update from the Miranda website now?");
		TCHAR* szTemp = ( TCHAR* )alloca( sizeof( TCHAR )*( lstrlen(szVersion) + lstrlen(szText) + 10 ));
		mir_sntprintf(szTemp, lstrlen(szVersion) + lstrlen(szText) + 10, szText, szVersion);
		if ( IDYES == MessageBox( NULL, TranslateTS(szTemp), TranslateT( "Information" ), MB_YESNO | MB_ICONINFORMATION ))
			CallService( MS_UTILS_OPENURL, 1, (LPARAM) "http://miranda-im.org/");
		return 1;
	}

	GetModuleName();

	InitializeCriticalSection(&cs);
	InitializeCriticalSection(&m_gchook);

	m_ssleay32 = LoadLibraryA("winssl.dll");
	if (m_ssleay32 == NULL) 
		m_ssleay32 = LoadLibraryA("cyassl.dll");
	if (m_ssleay32 == NULL) 
		m_ssleay32 = LoadLibraryA("ssleay32.dll");
	if (m_ssleay32 == NULL) 
		m_ssleay32 = LoadLibraryA("libssl32.DLL" );

	// register protocol
	{	PROTOCOLDESCRIPTOR pd;
		ZeroMemory( &pd, sizeof( pd ) );
		pd.cbSize = sizeof( pd );
		pd.szName = IRCPROTONAME;
		pd.type = PROTOTYPE_PROTOCOL;
		CallService( MS_PROTO_REGISTERMODULE, 0, (LPARAM)&pd );
	}

	HookEvents();
	CreateServiceFunctions();
	InitPrefs();
	CList_SetAllOffline(true);
	return 0;
}

extern "C" int __declspec(dllexport) Unload(void)
{
	CList_SetAllOffline( TRUE );

	DeleteCriticalSection( &cs );
	DeleteCriticalSection( &m_gchook );

	if ( m_ssleay32 )
		FreeLibrary( m_ssleay32 );

	UnhookEvents();
	UnInitOptions();
	free( IRCPROTONAME );
	delete [] ALTIRCPROTONAME;
	return 0;
}
