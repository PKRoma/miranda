/*
Change ICQ Details plugin for Miranda IM

Copyright © 2001-2005 Richard Hughes, Martin Öberg

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



#include <windows.h>
#include <commctrl.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_protocols.h>
#include <m_userinfo.h>
#include <m_langpack.h>
#include "resource.h"

BOOL CALLBACK ChangeInfoDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

HINSTANCE hInst;
PLUGINLINK *pluginLink;

PLUGININFO pluginInfo={
	sizeof(PLUGININFO),
	"Change User Details",
	PLUGIN_MAKE_VERSION(0,3,2,0),
	"Adds an extra tab to the user info dialog to change your own details on the server.",
	"Richard Hughes",
	"info at 'miranda-im.org",
	"© 2001-3 Richard Hughes",
	"http://www.miranda-im.org",
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
	if(mirandaVersion<PLUGIN_MAKE_VERSION(0,1,2,0)) return NULL;
	return &pluginInfo;
}

static int UserInfoInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp={0};

	if((HANDLE)lParam!=NULL) return 0;
	if(!CallService(MS_PROTO_ISPROTOCOLLOADED,0,(LPARAM)"ICQ")) return 0;
	odp.cbSize=sizeof(odp);
	odp.hIcon=NULL;
	odp.hInstance=hInst;
	odp.pfnDlgProc=ChangeInfoDlgProc;
	odp.position=2000000000;
	odp.pszTemplate=MAKEINTRESOURCE(IDD_INFO_CHANGEINFO);
	odp.pszTitle=Translate("Change ICQ Details");
	CallService(MS_USERINFO_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}

static int ModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	HookEvent(ME_USERINFO_INITIALISE,UserInfoInit);
	return 0;
}

extern "C" int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink=link;
	InitCommonControls();
	HookEvent(ME_SYSTEM_MODULESLOADED,ModulesLoaded);
	return 0;
}

extern "C" int __declspec(dllexport) Unload(void)
{
	return 0;
}