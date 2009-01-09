/*
IRC plugin for Miranda IM

Copyright (C) 2003-05 Jurgen Persson
Copyright (C) 2007-08 George Hazan

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

PLUGINLINK*    pluginLink;
HINSTANCE      hInst = NULL;

char           mirandapath[MAX_PATH];
int            mirVersion;

UTF8_INTERFACE utfi;
MM_INTERFACE   mmi;
LIST_INTERFACE li;

void InitTimers( void );
void UninitTimers( void );

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

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD, LPVOID)
{
	hInst = hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	if ( mirandaVersion < PLUGIN_MAKE_VERSION( 0, 8, 0, 9 )) {
		MessageBox( NULL, LPGENT("The IRC protocol plugin cannot be loaded. It requires Miranda IM 0.8.0.9 or later."), LPGENT("IRC Protocol Plugin"), MB_OK|MB_ICONWARNING|MB_SETFOREGROUND|MB_TOPMOST );
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

/////////////////////////////////////////////////////////////////////////////////////////

static CIrcProto* ircProtoInit( const char* pszProtoName, const TCHAR* tszUserName )
{
	return new CIrcProto( pszProtoName, tszUserName );
}

static int ircProtoUninit( CIrcProto* ppro )
{
	delete ppro;
	return 0;
}

extern "C" int __declspec(dllexport) Load( PLUGINLINK *link )
{
	#ifndef NDEBUG
		int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
		flag |= _CRTDBG_LEAK_CHECK_DF;
		_CrtSetDbgFlag(flag);
	#endif

	pluginLink = link;
	mir_getMMI( &mmi );
	mir_getUTFI( &utfi );
	mir_getLI( &li );

	GetModuleFileNameA(hInst, mirandapath, MAX_PATH);
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
	}	}

	InitTimers();

	// register protocol
	PROTOCOLDESCRIPTOR pd = { 0 };
	pd.cbSize = sizeof( pd );
	pd.szName = "IRC";
	pd.type = PROTOTYPE_PROTOCOL;
	pd.fnInit = ( pfnInitProto )ircProtoInit;
	pd.fnUninit = ( pfnUninitProto )ircProtoUninit;
	CallService( MS_PROTO_REGISTERMODULE, 0, (LPARAM)&pd );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

extern "C" int __declspec(dllexport) Unload(void)
{
	UninitTimers();
	return 0;
}
