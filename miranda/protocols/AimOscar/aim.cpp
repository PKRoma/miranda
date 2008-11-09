#include "aim.h"

PLUGINLINK *pluginLink;
MD5_INTERFACE md5i;
MM_INTERFACE mmi;
UTF8_INTERFACE utfi;
LIST_INTERFACE li;

HINSTANCE hInstance;

static HANDLE hMooduleLoaded;


#define AIM_OSCAR_VERSION "\0\x08\1\x0"
const char AIM_CLIENT_ID_STRING[]="Miranda Oscar Plugin, version 0.8.1.0";
char AIM_CAP_MIRANDA[]="MirandaA\0\0\0\0\0\0\0";

/////////////////////////////////////////////////////////////////////////////
// Protocol instances
static int sttCompareProtocols(const CAimProto *p1, const CAimProto *p2)
{
	return _tcscmp(p1->m_tszUserName, p2->m_tszUserName);
}

OBJLIST<CAimProto> g_Instances(1, sttCompareProtocols);

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
	PLUGIN_MAKE_VERSION(0,8,1,0),
	"Provides basic support for AOL® OSCAR Instant Messenger protocol. [Built: "__DATE__" "__TIME__"]",
	"Boris Krasnovskiy, Aaron Myles Landwehr",
	"borkra@miranda-im.org",
	"© 2008 Boris Krasnovskiy, 2005-2006 Aaron Myles Landwehr",
	"http://www.miranda-im.org",
	UNICODE_AWARE,		//not transient
	0,		//doesn't replace anything built-in
    {0xb4ef58c4, 0x4458, 0x4e47, { 0xa7, 0x67, 0x5c, 0xae, 0xe5, 0xe7, 0xc, 0x81 }} //{B4EF58C4-4458-4e47-A767-5CAEE5E70C81}
};

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	unsigned long mv=_htonl(mirandaVersion);
	memcpy(&AIM_CAP_MIRANDA[8],&mv,sizeof(DWORD));
	memcpy(&AIM_CAP_MIRANDA[12],AIM_OSCAR_VERSION,sizeof(DWORD));
	return &pluginInfo;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Interface information

static const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};
extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

////////////////////////////////////////////////////////////////////////////////////////
//	OnModulesLoaded - finalizes plugin's configuration on load

static int OnModulesLoaded( WPARAM wParam, LPARAM lParam )
{
	aim_links_init();

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Load

static PROTO_INTERFACE* protoInit( const char* pszProtoName, const TCHAR* tszUserName )
{
	CAimProto *ppro = new CAimProto( pszProtoName, tszUserName );
	g_Instances.insert(ppro);
	return ppro;
}

static int protoUninit( PROTO_INTERFACE* ppro )
{
	g_Instances.remove((CAimProto*)ppro);
	return 0;
}

extern "C" int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink = link;
	mir_getMMI( &mmi );
	mir_getMD5I( &md5i );
	mir_getUTFI( &utfi );
	mir_getLI( &li );

	hMooduleLoaded = HookEvent( ME_SYSTEM_MODULESLOADED, OnModulesLoaded );

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
	aim_links_destroy();
	UnhookEvent(hMooduleLoaded);
	return 0;
}
