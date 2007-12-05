/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
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
#include "clc.h"
#include <m_hotkeys.h>

static int hkHideShow(WPARAM wParam,LPARAM lParam)
{
	cli.pfnShowHide(0,0);
	return 0;
}
/*
int hkSearch(WPARAM wParam,LPARAM lParam)
{
	DBVARIANT dbv={0};
	if(!DBGetContactSettingString(NULL,"CList","SearchUrl",&dbv)) {
		CallService(MS_UTILS_OPENURL,DBGetContactSettingByte(NULL,"CList","HKSearchNewWnd",0),(LPARAM)dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	return 0;
}
*/
static int hkRead(WPARAM wParam,LPARAM lParam)
{
	if(cli.pfnEventsProcessTrayDoubleClick(0)==0) return TRUE;
	SetForegroundWindow(cli.hwndContactList);
	SetFocus(cli.hwndContactList);
	return 0;
}

static int hkOpts(WPARAM wParam,LPARAM lParam)
{
	CallService("Options/OptionsCommand",0, 0);
	return 0;
}
/*
static int hkCloseMiranda(WPARAM wParam,LPARAM lParam)
{
	CallService("CloseAction",0,0);
	return 0;
}

int hkRestoreStatus(WPARAM wParam,LPARAM lParam)
{
	int nStatus = DBGetContactSettingWord(NULL, "CList", "Status", ID_STATUS_OFFLINE);
	if (nStatus != ID_STATUS_OFFLINE)
		PostMessage(cli.hwndContactList, WM_COMMAND, nStatus, 0);

	return 0;
}
*/
static int hkAllOffline(WPARAM wParam,LPARAM lParam)
{
	PROTOCOLDESCRIPTOR** ppProtoDesc;
	int nProtoCount;
	int nProto;


	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&nProtoCount, (LPARAM)&ppProtoDesc);
	for (nProto = 0; nProto < nProtoCount; nProto++)
	{
		if (ppProtoDesc[nProto]->type == PROTOTYPE_PROTOCOL)
			CallProtoService(ppProtoDesc[nProto]->szName, PS_SETSTATUS, ID_STATUS_OFFLINE, 0);
	}
	return 0;
}

int InitClistHotKeys(void)
{
	HOTKEYDESC shk;

	CreateServiceFunction("CLIST/HK/SHOWHIDE",hkHideShow);
	CreateServiceFunction("CLIST/HK/Opts",hkOpts);
	CreateServiceFunction("CLIST/HK/Read",hkRead);
//	CreateServiceFunction("CLIST/HK/CloseMiranda",hkCloseMiranda);
//	CreateServiceFunction("CLIST/HK/RestoreStatus",hkRestoreStatus);
	CreateServiceFunction("CLIST/HK/AllOffline",hkAllOffline);

	shk.cbSize=sizeof(shk);
	shk.pszDescription="Show Hide Contact List";
	shk.pszName="ShowHide";
	shk.pszSection="Main";
	shk.pszService="CLIST/HK/SHOWHIDE";
	shk.DefHotKey = HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_SHIFT, 'A');
	CallService(MS_HOTKEY_REGISTER,0,(LPARAM)&shk);

	shk.pszDescription="Read Message";
	shk.pszName="ReadMessage";
	shk.pszSection="Main";
	shk.pszService="CLIST/HK/Read";
	shk.DefHotKey = HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_SHIFT, 'I');
	CallService(MS_HOTKEY_REGISTER,0,(LPARAM)&shk);
/*
	shk.pszDescription="Search in site";
	shk.pszName="SearchInWeb";
	shk.pszSection="Main";
	shk.pszService="CLIST/HK/Search";
	shk.DefHotKey=846;
	CallService(MS_HOTKEY_REGISTER,0,(LPARAM)&shk);	
*/
	shk.pszDescription="Open Options Page";
	shk.pszName="ShowOptions";
	shk.pszSection="Main";
	shk.pszService="CLIST/HK/Opts";
	shk.DefHotKey = HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_SHIFT, 'O');
	CallService(MS_HOTKEY_REGISTER,0,(LPARAM)&shk);	

	shk.pszDescription="Open Find User Dialog";
	shk.pszName="FindUsers";
	shk.pszSection="Main";
	shk.pszService="FindAdd/FindAddCommand";
	shk.DefHotKey = HOTKEYCODE(HOTKEYF_CONTROL|HOTKEYF_SHIFT, 'F');
	CallService(MS_HOTKEY_REGISTER,0,(LPARAM)&shk);	
/*
	shk.pszDescription="Close Miranda";
	shk.pszName="CloseMiranda";
	shk.pszSection="Main";
	shk.pszService="CLIST/HK/CloseMiranda";
	shk.DefHotKey=0;
	CallService(MS_HOTKEY_REGISTER,0,(LPARAM)&shk);	

	shk.pszDescription="Restore last status";
	shk.pszName="RestoreLastStatus";
	shk.pszSection="Status";
	shk.pszService="CLIST/HK/RestoreStatus";
	shk.DefHotKey=0;
	CallService(MS_HOTKEY_REGISTER,0,(LPARAM)&shk);	
*/
	shk.pszDescription="Set All Offline";
	shk.pszName="AllOffline";
	shk.pszSection="Status";
	shk.pszService="CLIST/HK/AllOffline";
	shk.DefHotKey=0;
	CallService(MS_HOTKEY_REGISTER,0,(LPARAM)&shk);	

	return 0;
}


int fnHotKeysRegister(HWND hwnd)
{
	return 0;
}

void fnHotKeysUnregister(HWND hwnd)
{
}

int fnHotKeysProcess(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	return TRUE;
}

int fnHotkeysProcessMessage(WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}
