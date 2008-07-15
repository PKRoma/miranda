/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2008 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "msn_global.h"
#include "msn_proto.h"
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

void    MsnLinks_Init( void );


/////////////////////////////////////////////////////////////////////////////////////////
// Global variables

bool	msnHaveChatDll = false;
int		avsPresent = -1;

PLUGININFOEX pluginInfo =
{
	sizeof(PLUGININFOEX),
	"MSN Protocol",
	__VERSION_DWORD,
	"Adds support for communicating with users of the MSN Messenger network",
	"Boris Krasnovskiy, George Hazan, Richard Hughes",
	"borkra@miranda-im.org",
	"© 2001-2008 Richard Hughes, George Hazan, Boris Krasnovskiy",
	"http://miranda-im.org",
	UNICODE_AWARE,	
	0,
    #if defined( _UNICODE )
	{0xdc39da8a, 0x8385, 0x4cd9, {0xb2, 0x98, 0x80, 0x67, 0x7b, 0x8f, 0xe6, 0xe4}} //{DC39DA8A-8385-4cd9-B298-80677B8FE6E4}
    #else
    {0x29aa3a80, 0x3368, 0x4b78, { 0x82, 0xc1, 0xdf, 0xc7, 0x29, 0x6a, 0x58, 0x99 }} //{29AA3A80-3368-4b78-82C1-DFC7296A5899}
    #endif
};

int MSN_GCEventHook( WPARAM wParam, LPARAM lParam );
int MSN_GCMenuHook( WPARAM wParam, LPARAM lParam );
int MSN_ChatInit( WPARAM wParam, LPARAM lParam );

/////////////////////////////////////////////////////////////////////////////
// Protocol instances
static int sttCompareProtocols(const CMsnProto *p1, const CMsnProto *p2)
{
	return _tcscmp(p1->m_tszUserName, p2->m_tszUserName);
}

OBJLIST<CMsnProto> g_Instances(1, sttCompareProtocols);

/////////////////////////////////////////////////////////////////////////////////////////
//	Main DLL function

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst = hinstDLL;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	OnModulesLoaded - finalizes plugin's configuration on load

static int OnModulesLoaded( WPARAM wParam, LPARAM lParam )
{
	avsPresent = ServiceExists(MS_AV_SETMYAVATAR) != 0;
	msnHaveChatDll = ServiceExists(MS_GC_REGISTER) != 0;

	MsnLinks_Init();

	return 0;
}


static CMsnProto* msnProtoInit( const char* pszProtoName, const TCHAR* tszUserName )
{
	CMsnProto *ppro = new CMsnProto( pszProtoName, tszUserName );
	g_Instances.insert(ppro);
	return ppro;
}

static int msnProtoUninit( CMsnProto* ppro )
{
	g_Instances.remove(ppro);
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Performs a primary set of actions upon plugin loading

extern "C" int __declspec(dllexport) Load( PLUGINLINK* link )
{
	pluginLink = link;

	mir_getLI( &li );
	mir_getMMI( &mmi );
	mir_getUTFI( &utfi );
	mir_getMD5I( &md5i );
    mir_getSHA1I( &sha1i );

	HookEvent( ME_SYSTEM_MODULESLOADED, OnModulesLoaded );

	srand(( unsigned int )time( NULL ));

	PROTOCOLDESCRIPTOR pd = {0};
	pd.cbSize = sizeof( pd );
	pd.szName = "MSN";
	pd.fnInit = ( pfnInitProto )msnProtoInit;
	pd.fnUninit = ( pfnUninitProto )msnProtoUninit;
	pd.type = PROTOTYPE_PROTOCOL;
	MSN_CallService( MS_PROTO_REGISTERMODULE, 0, ( LPARAM )&pd );

	MsnInitIcons();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Unload a plugin

extern "C" int __declspec( dllexport ) Unload( void )
{
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
