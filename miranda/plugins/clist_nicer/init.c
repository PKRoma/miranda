/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2004 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

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

#include "commonheaders.h"

HINSTANCE g_hInst = 0;
PLUGINLINK * pluginLink;
struct MM_INTERFACE memoryManagerInterface;
extern HWND hwndContactList;

PLUGININFO pluginInfo = {
	sizeof(PLUGININFO),
	"Nicer contact list",
	PLUGIN_MAKE_VERSION(0,3,4,10),
	"Display contacts, event notifications, protocol status",
	"Pixel, egoDust, cyreve",
	"",
	"Copyright 2000-2005 Miranda-IM project",
	"http://www.miranda-im.org",
	0,
	DEFMOD_CLISTALL
};

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID reserved)
{
	g_hInst = hInstDLL;
	DisableThreadLibraryCalls(g_hInst);
	return TRUE;
}


__declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	if ( mirandaVersion < PLUGIN_MAKE_VERSION(0,3,4,0) ) return NULL;
	return &pluginInfo;
}

int LoadContactListModule(void);
int LoadCLCModule(void); 
int LoadCLUIModule(); 

static int systemModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	LoadCLUIModule();
	return 0;
}

int __declspec(dllexport) CListInitialise(PLUGINLINK * link)
{
	int rc=0;
	pluginLink=link;
#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif	
	// get the internal malloc/free()
	memset(&memoryManagerInterface,0,sizeof(memoryManagerInterface));
	memoryManagerInterface.cbSize = sizeof(memoryManagerInterface);
	CallService(MS_SYSTEM_GET_MMI, 0, (LPARAM)&memoryManagerInterface);
	rc=LoadContactListModule();
	if (rc==0) rc=LoadCLCModule();
	HookEvent(ME_SYSTEM_MODULESLOADED, systemModulesLoaded);
	return rc;
}

// a plugin loader aware of CList exports will never call this.
int __declspec(dllexport) Load(PLUGINLINK * link)
{
	return 1;
}

int __declspec(dllexport) Unload(void)
{
	if (IsWindow(hwndContactList)) DestroyWindow(hwndContactList);
	return 0;
}

