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
#include "yahoo.h"
#include "http_gateway.h"
#include "version.h"
#include "resource.h"

#include <m_langpack.h>
#include <m_idle.h>
#include <m_userinfo.h>
#include <time.h>

/*
 * Global Variables
 */
HINSTANCE   hInstance;
PLUGINLINK* pluginLink;

MM_INTERFACE   mmi;
UTF8_INTERFACE utfi;
MD5_INTERFACE  md5i;
SHA1_INTERFACE sha1i;
LIST_INTERFACE li;

HANDLE hNetlibUser = NULL;

PLUGININFOEX pluginInfo={
		sizeof(PLUGININFOEX),
#ifdef YAHOO_CVSBUILD
		"Yahoo Protocol Beta/Nightly",
#else
		"Yahoo Protocol",
#endif
		__VERSION_DWORD,
		"Yahoo Protocol support via libyahoo2 library. [Built: "__DATE__" "__TIME__"]",
		"Gennady Feldman",
		"gena01@miranda-im.org",
		"© 2003-2009 Gennady Feldman, Laurent Marechal",
		"http://www.miranda-im.org",
		UNICODE_AWARE, //not transient
		0, //DEFMOD_PROTOCOLYAHOO - no core yahoo protocol
        {0xa6648b6c, 0x6fb8, 0x4551, { 0xb4, 0xe7, 0x1, 0x36, 0xf9, 0x16, 0xd4, 0x85 }} //{A6648B6C-6FB8-4551-B4E7-0136F916D485}
};

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
extern "C" BOOL WINAPI DllMain(HINSTANCE hinst,DWORD /*fdwReason*/,LPVOID /*lpvReserved*/)
{
	hInstance = hinst;
	return TRUE;
}

/*
 *	Load - loads plugin into memory
 */
 
//=====================================================
// Name : Load
// Parameters: PLUGINLINK *link
// Returns : int
// Description : Called when plugin is loaded into Miranda
//=====================================================

static int CompareProtos( const CYahooProto* p1, const CYahooProto* p2 )
{
	return lstrcmp(p1->m_tszUserName, p2->m_tszUserName);
}

LIST<CYahooProto> g_instances( 1, CompareProtos );

static CYahooProto* yahooProtoInit( const char* pszProtoName, const TCHAR* tszUserName )
{
	CYahooProto* ppro = new CYahooProto( pszProtoName, tszUserName );
	
	g_instances.insert( ppro );
	
	return ppro;
}

static int yahooProtoUninit( CYahooProto* ppro )
{
	g_instances.remove( ppro );
	delete ppro;
	
	return 0;
}

extern "C" int __declspec(dllexport)Load(PLUGINLINK *link)
{
 	pluginLink = link;
	
	/**
	 * Grab the interface handles (through pluginLink)
	 */
	mir_getMMI( &mmi );
	mir_getLI( &li );
	mir_getUTFI( &utfi );
	mir_getMD5I( &md5i );
	mir_getSHA1I( &sha1i );
	
	
	PROTOCOLDESCRIPTOR pd = { 0 };
	pd.cbSize = sizeof(pd);
	pd.szName = "YAHOO";
	pd.type   = PROTOTYPE_PROTOCOL;
	pd.fnInit = ( pfnInitProto )yahooProtoInit;
	pd.fnUninit = ( pfnUninitProto )yahooProtoUninit;
	CallService( MS_PROTO_REGISTERMODULE, 0, ( LPARAM )&pd );

	// Initialize our important variable
	register_callbacks();
	
	return 0;
}

/*
 * Unload - Unloads plugin
 * Parameters: void
 */

extern "C" int __declspec(dllexport) Unload(void)
{
	DebugLog("Unload");
	Netlib_CloseHandle( hNetlibUser );
	
	return 0;
}

/*
 * MirandaPluginInfoEx - Sets plugin info
 * Parameters: (DWORD mirandaVersion)
 */
extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	/*
     * We require Miranda 0.8.0.24
	 * This requires the latest trunk... [md5, sha, etc..]
	 */
    if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 8, 0, 24)) {
		MessageBoxA( NULL, 
				"Yahoo plugin cannot be loaded. It requires Miranda IM 0.8.0.24 or later.", 
				"Yahoo", 
				MB_OK|MB_ICONWARNING|MB_SETFOREGROUND|MB_TOPMOST );

        return NULL;
	}

    return &pluginInfo;
}

/*
 * MirandaPluginInterfaces - Notifies the core of interfaces implemented
 * Parameters: none
 */
static const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};

extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}
