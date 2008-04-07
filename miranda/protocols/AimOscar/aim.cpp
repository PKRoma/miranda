#include "aim.h"

PLUGINLINK *pluginLink;
MD5_INTERFACE md5i;
MM_INTERFACE mmi;
HINSTANCE hInstance;

#define AIM_OSCAR_VERSION "\0\0\0\x07"
char* AIM_CLIENT_ID_STRING="Miranda Oscar Plugin, version 0.8.0.0";
char AIM_CAP_MIRANDA[]="MirandaA\0\0\0\0\0\0\0";

/////////////////////////////////////////////////////////////////////////////////////////
// Dll entry point

DWORD WINAPI DllMain(HINSTANCE hinstDLL,DWORD /*fdwReason*/,LPVOID /*lpvReserved*/)
{
	hInstance = hinstDLL;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Plugin information

PLUGININFOEX pluginInfo={
	sizeof(PLUGININFOEX),
	"AIM OSCAR Plugin",
	PLUGIN_MAKE_VERSION(0,8,0,0),
	"Provides basic support for AOL® OSCAR Instant Messenger protocol. [Built: "__DATE__" "__TIME__"]",
	"Aaron Myles Landwehr",
	"aaron@miranda-im.org",
	"© 2005-2006 Aaron Myles Landwehr",
	"http://www.snaphat.com/oscar",
	UNICODE_AWARE,		//not transient
	0,		//doesn't replace anything built-in
    {0xb4ef58c4, 0x4458, 0x4e47, { 0xa7, 0x67, 0x5c, 0xae, 0xe5, 0xe7, 0xc, 0x81 }} //{B4EF58C4-4458-4e47-A767-5CAEE5E70C81}
};

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	unsigned long mv=_htonl(mirandaVersion);
	memcpy((char*)&AIM_CAP_MIRANDA[8],&mv,sizeof(DWORD));
	memcpy((char*)&AIM_CAP_MIRANDA[12],(char*)&AIM_OSCAR_VERSION,sizeof(DWORD));
	return &pluginInfo;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Interface information

static const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};
extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Load

static PROTO_INTERFACE* protoInit( const char* pszProtoName, const TCHAR* tszUserName )
{
	return new CAimProto( pszProtoName, tszUserName );
}

static int protoUninit( PROTO_INTERFACE* ppro )
{
	delete ( CAimProto* )ppro;
	return 0;
}

extern "C" int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink = link;
	mir_getMMI( &mmi );
	mir_getMD5I( &md5i );

	PROTOCOLDESCRIPTOR pd = { 0 };
	pd.cbSize = sizeof(pd);
	pd.szName = "AIM";
	pd.type = PROTOTYPE_PROTOCOL;
	pd.fnInit = protoInit;
	pd.fnUninit = protoUninit;
	CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM) & pd);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Unload

extern "C" int __declspec(dllexport) Unload(void)
{
	return 0;
}
