/*
Miranda SMS Plugin
Copyright (C) 2001  Richard Hughes

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
#include "resource.h"
#include "../../miranda32/random/plugins/newpluginapi.h"
#include "../../miranda32/ui/contactlist/m_clist.h"

BOOL CALLBACK SendSmsDlgProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam);
void InitSmsSend(void);
void UninitSmsSend(void);
char *PrettyPrintXML(const char *xml);

HINSTANCE hInst;
PLUGINLINK *pluginLink;
static HWND hwndSendSms;

PLUGININFO pluginInfo={
	sizeof(PLUGININFO),
	"SMS",
	PLUGIN_MAKE_VERSION(0,1,2,0),
	"Send SMS text messages to mobile phones through the ICQ network",
	"Richard Hughes",
	"cyreve@users.sourceforge.net",
	"© 2001 Richard Hughes",
	"http://miranda-icq.sourceforge.net/",
	0,		//not transient
	0		//doesn't replace anything built-in
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst=hinstDLL;
	return TRUE;
}

static int SendSMSMenuCommand(WPARAM wParam,LPARAM lParam)
{
	if(IsWindow(hwndSendSms)) {
		SetForegroundWindow(hwndSendSms);
		SetFocus(hwndSendSms);
	}
	else
		hwndSendSms=CreateDialog(hInst,MAKEINTRESOURCE(IDD_SENDSMS),NULL,SendSmsDlgProc);
	return 0;
}

extern "C" __declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	if(mirandaVersion<PLUGIN_MAKE_VERSION(0,1,2,0))
		return NULL;
	return &pluginInfo;
}

extern "C" int __declspec(dllexport) Load(PLUGINLINK *link)
{
	CLISTMENUITEM mi;

	pluginLink=link;
	CreateServiceFunction("SMS/SendSMS",SendSMSMenuCommand);
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.position=300050000;
	mi.flags=0;
	mi.hIcon=LoadIcon(hInst,MAKEINTRESOURCE(IDI_SMS));
	mi.pszName="Send &SMS...";
	mi.pszService="SMS/SendSMS";
	CallService(MS_CLIST_ADDMAINMENUITEM,0,(LPARAM)&mi);
	InitSmsSend();
	return 0;
}

extern "C" int __declspec(dllexport) Unload(void)
{
	UninitSmsSend();
	return 0;
}