/*
Miranda NetDetect plugin
Copyright (C) 2001-2  Richard Hughes

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
#include <wininet.h>
#include "../../miranda32/random/plugins/newpluginapi.h"
#include "../../miranda32/database/m_database.h"
#include "../../miranda32/ui/contactlist/m_clist.h"

#pragma comment(linker, "/OPT:NOWIN98")

HINSTANCE hInst;
PLUGINLINK *pluginLink;

PLUGININFO pluginInfo={
	sizeof(PLUGININFO),
	"NetDetect",
	PLUGIN_MAKE_VERSION(0,1,0,0),
	"Connects and disconnects Miranda according to the presence of your Internet connection",
	"Richard Hughes",
	"cyreve@users.sourceforge.net",
	"© 2001-2 Richard Hughes",
	"http://miranda-icq.sourceforge.net/",
	0,		//not transient
	0		//doesn't replace anything built-in
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst=hinstDLL;
	return TRUE;
}

static int newStatus=-1;

static int IsConnected(void)
{
	DWORD flags;
	int conn=InternetGetConnectedState(&flags,0);
	if(conn && flags&INTERNET_CONNECTION_LAN && !(flags&INTERNET_CONNECTION_MODEM)) conn=0;
	return conn;
}

static VOID CALLBACK NetDetectTimer(HWND hwnd,UINT message,UINT idEvent,DWORD dwTime)
{
	int mirandaStatus;
	int conn=IsConnected();

	mirandaStatus=CallService(MS_CLIST_GETSTATUSMODE,0,0);
	if(mirandaStatus==ID_STATUS_OFFLINE && conn) {
		int status=DBGetContactSettingWord(NULL,"CList","Status",ID_STATUS_OFFLINE);
		if(newStatus!=-1) {status=newStatus; newStatus=-1;}
		if(status!=ID_STATUS_OFFLINE) CallService(MS_CLIST_SETSTATUSMODE,status,0);
	}
	else if(mirandaStatus!=ID_STATUS_OFFLINE && !conn) {
		CallService(MS_CLIST_SETSTATUSMODE,ID_STATUS_OFFLINE,0);
		DBWriteContactSettingWord(NULL,"CList","Status",(WORD)mirandaStatus);
	}
}

__declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	return &pluginInfo;
}

int __declspec(dllexport) Load(PLUGINLINK *link)
{
	int mirandaStatus;

	pluginLink=link;
	mirandaStatus=DBGetContactSettingWord(NULL,"CList","Status",ID_STATUS_OFFLINE);
	if(!IsConnected() && mirandaStatus!=ID_STATUS_OFFLINE) {
		newStatus=mirandaStatus;
		DBWriteContactSettingWord(NULL,"CList","Status",ID_STATUS_OFFLINE);
	}
	SetTimer(NULL,1,5000,NetDetectTimer);
	return 0;
}

int __declspec(dllexport) Unload(void)
{
	return 0;
}