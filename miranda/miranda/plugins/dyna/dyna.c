#include <windows.h>
#include <direct.h>
#include <stdio.h>
#include <newpluginapi.h>
#include <m_clist.h>
#include <m_skin.h>
#include <m_utils.h>
#include <m_system.h>

typedef struct {
	HINSTANCE hInst;
	int (*Unload) (void);
} PLUGINLOADINFO;

static PLUGINLOADINFO * pluginLoaded = NULL;
static int pluginLoadedCount = 0;

HINSTANCE g_hInst;
PLUGINLINK *pluginLink;
DWORD g_CoreVersion;
PLUGINLINK pluginLinkFake;

PLUGININFO pluginInfo={
	sizeof(PLUGININFO),
	"Dynamic Plugin Loader",
	PLUGIN_MAKE_VERSION(0,1,0,2),
	"A cheeky hack to load certain plugins without having to restart Miranda",
	"Sam Kothari",
	"egodust@users.sf.net",
	"© 2005 Sam Kothari",
	"http://www.miranda-im.org/",
	0,		//not transient
	0		//doesn't replace anything built-in
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	g_hInst=hinstDLL;
	return TRUE;
}

static HANDLE MyHookEvent(const char * hook, MIRANDAHOOK hookProc)
{
	if ( strcmpi(hook, ME_SYSTEM_MODULESLOADED) == 0) {
		hookProc(0, 0);
	}
	return HookEvent(hook, hookProc);
}

static int ModuleAlreadyLoaded(char * plugin)
{
	char * p = strrchr(plugin, '\\');
	if ( p != NULL ) {
		char * d = strchr(p, '.');
		if ( d != NULL ) {
			char buf[MAX_PATH];
			mir_snprintf(buf,sizeof(buf),"%s", p+1, d-1);
			if ( GetModuleHandle(buf) == NULL ) return 0;
		}
	}
	return 1;
}

static void LoadPlugin(char * plugin) 
{
	PLUGININFO * (*MirandaPluginInfo) (DWORD);
	int (*Load) (PLUGINLINK *);
	int (*Unload) (void);
	HINSTANCE hInst=NULL;
	// already loaded?
	if ( ModuleAlreadyLoaded(plugin) ) return;
	// load it
	hInst=LoadLibrary(plugin);
	if ( hInst == NULL ) return;
	// has it got our export settings?
	MirandaPluginInfo=(PLUGININFO *(*)(DWORD))GetProcAddress(hInst, "MirandaPluginInfo");
	Load=(int (*)(PLUGINLINK*))GetProcAddress(hInst, "Load");
	Unload=(int (*)(void))GetProcAddress(hInst, "Unload");
	if ( MirandaPluginInfo == NULL || Load == NULL || Unload == NULL ) {
		FreeLibrary(hInst);
		return;
	}
	if ( GetProcAddress(hInst,"DatabasePluginInfo") || GetProcAddress(hInst,"CListInitialise") ) {
		FreeLibrary(hInst);
		return;
	}
	if ( MirandaPluginInfo(g_CoreVersion) == NULL ) {
		FreeLibrary(hInst);
		return;
	}
	if ( Load(pluginLink) ) {
		FreeLibrary(hInst);
		return;
	}
	// add this puppy to the list..
	pluginLoaded=realloc(pluginLoaded, sizeof(PLUGINLOADINFO) * (pluginLoadedCount+1));
	pluginLoaded[pluginLoadedCount].hInst=hInst;
	pluginLoaded[pluginLoadedCount].Unload=Unload;
	pluginLoadedCount++;
	{
		char buf[MAX_PATH];
		mir_snprintf(buf,sizeof(buf),"Loaded '%s'.",strrchr(plugin,'\\')+1);
		MessageBox(0,buf,"",MB_OK | MB_ICONINFORMATION);
	}
}

static int DynaLoadPluginRequest(WPARAM wParam,LPARAM lParam)
{
	char nFile[MAX_PATH];
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.Flags=OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrFile=nFile;
	ofn.nMaxFile=sizeof(nFile);
	// change to this Miranda's plugin dir
	{
		char * p;
		GetModuleFileName(NULL, nFile, sizeof(nFile));
		p=strrchr(nFile, '\\');
		if ( p != NULL ) *p=0;		
		mir_snprintf(nFile, sizeof(nFile)-10, "%s\\Plugins\\", nFile);
		_chdir(nFile);
		ZeroMemory(nFile,sizeof(nFile));
	}
	// find which plugin to get
	if ( GetOpenFileName(&ofn) ) LoadPlugin(nFile);
	return 0;
}

__declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	return &pluginInfo;
}

int __declspec(dllexport) Load(PLUGINLINK *link)
{
	CLISTMENUITEM mi;

	pluginLink=link;
	memcpy(&pluginLinkFake, pluginLink, sizeof(PLUGINLINK));
	pluginLinkFake.HookEvent=MyHookEvent;
	g_CoreVersion=(DWORD)CallService(MS_SYSTEM_GETVERSION, 0, 0);

	DisableThreadLibraryCalls(g_hInst);

	CreateServiceFunction("Dyna/MenuCommand",DynaLoadPluginRequest);
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.position=2100000000;
	mi.flags=0;
	mi.hIcon=LoadSkinnedIcon(SKINICON_OTHER_MIRANDA);
	mi.pszName="&Load Plugin...";
	mi.pszService="Dyna/MenuCommand";
	CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);
	return 0;
}

int __declspec(dllexport) Unload(void)
{
	int j;
	for (j=0 ; j<pluginLoadedCount; j++) {
		if ( pluginLoaded[j].Unload != NULL ) pluginLoaded[j].Unload();
		FreeLibrary(pluginLoaded[j].hInst);
	}
	if ( pluginLoaded != NULL ) free(pluginLoaded);
	return 0;
}