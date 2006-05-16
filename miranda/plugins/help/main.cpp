/*
Miranda IM Help Plugin
Copyright (C) 2002 Richard Hughes

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

#if defined( UNICODE ) && !defined(_UNICODE)
	#define _UNICODE 
#endif
#include <tchar.h>

#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <stdio.h>
#include <newpluginapi.h>
#include <m_database.h>
#include <m_system.h>
#include <m_netlib.h>
#include <m_langpack.h>
#include "help.h"

HINSTANCE hInst;
PLUGINLINK *pluginLink;
extern HWND hwndHelpDlg;
HANDLE hNetlibUser;

static PLUGININFO pluginInfo={
	sizeof(PLUGININFO),
	"Help",
	PLUGIN_MAKE_VERSION(0,1,2,2),
	"Provides context sensitive help in the Miranda dialog boxes.",
	"Richard Hughes",
	"cyreve at users.sourceforge.net",
	"© 2002 Richard Hughes",
	"http://miranda-icq.sourceforge.net/",
	0,		//not transient
	0		//doesn't replace anything built-in
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst=hinstDLL;
	return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	if(mirandaVersion<PLUGIN_MAKE_VERSION(0,1,0,0)) return NULL;
	return &pluginInfo;
}

static int HelpModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	NETLIBUSER nlu={0};
	nlu.cbSize=sizeof(nlu);
	nlu.flags=NUF_OUTGOING|NUF_HTTPCONNS|NUF_NOHTTPSOPTION;
	nlu.szSettingsModule="HelpPlugin";
	nlu.szDescriptiveName=Translate("Help plugin HTTP connections");
	hNetlibUser=(HANDLE)CallService(MS_NETLIB_REGISTERUSER,0,(LPARAM)&nlu);
	return 0;
}

extern "C" int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink=link;
	HookEvent(ME_SYSTEM_MODULESLOADED,HelpModulesLoaded);
	if(LoadLibraryA("riched20.dll")==NULL) return 1;
	if(InstallDialogBoxHook()) return 1;
	InitOptions();
	InitDialogCache();
	return 0;
}

extern "C" int __declspec(dllexport) Unload(void)
{
	RemoveDialogBoxHook();
	if(IsWindow(hwndHelpDlg)) DestroyWindow(hwndHelpDlg);
	FreeDialogCache();
	Netlib_CloseHandle(hNetlibUser);
	return 0;
}