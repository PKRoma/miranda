/*
Chat module plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson

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
#include "chat.h"

extern HANDLE	g_hInst;
extern HIMAGELIST hImageList;
extern BOOL		SmileyAddInstalled;
extern BOOL		PopUpInstalled;

HANDLE			hSendEvent;
HANDLE			hBuildMenuEvent ;
HANDLE			g_hModulesLoaded;
HANDLE			g_hSystemPreShutdown;
HANDLE			g_hHookContactDblClick;
HANDLE			g_hIconsChanged;

#define SIZEOF_STRUCT_GCREGISTER_V1 28
#define SIZEOF_STRUCT_GCWINDOW_V1	32
#define SIZEOF_STRUCT_GCEVENT_V1	44

void HookEvents(void)
{
	g_hModulesLoaded =			HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	g_hHookContactDblClick=		HookEvent(ME_CLIST_DOUBLECLICKED, CList_RoomDoubleclicked);
	g_hSystemPreShutdown =		HookEvent(ME_SYSTEM_PRESHUTDOWN, PreShutdown);
	g_hIconsChanged =			HookEvent(ME_SKIN_ICONSCHANGED, IconsChanged);
	return;
}

void UnhookEvents(void)
{
	UnhookEvent(g_hModulesLoaded);
	UnhookEvent(g_hSystemPreShutdown);
	UnhookEvent(g_hHookContactDblClick);
	UnhookEvent(g_hIconsChanged);
	return;
}

void CreateServiceFunctions(void)
{
	CreateServiceFunction(MS_GC_REGISTER, Service_Register);
	CreateServiceFunction(MS_GC_NEWCHAT,Service_NewChat);
	CreateServiceFunction(MS_GC_EVENT,Service_AddEvent);
	CreateServiceFunction(MS_GC_GETEVENTPTR,Service_GetAddEventPtr);
	CreateServiceFunction("GChat/DblClickEvent",CList_EventDoubleclicked);
	return;
}

void CreateHookableEvents(void)
{
	hSendEvent = CreateHookableEvent(ME_GC_EVENT);
	hBuildMenuEvent = CreateHookableEvent(ME_GC_BUILDMENU);
	return;
}

int ModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	{ //add as a known module in DB Editor ++
	   DBVARIANT dbv; 
	   if (DBGetContactSetting(NULL, "KnownModules", "Chat", &dbv)) 
		  DBWriteContactSettingString(NULL, "KnownModules", "Chat","Chat,ChatFonts"); 
	   DBFreeVariant(&dbv); 
	} 

	if(ServiceExists(MS_SMILEYADD_SHOWSELECTION))
		SmileyAddInstalled = TRUE;
	if(ServiceExists(MS_POPUP_ADDPOPUPEX))
		PopUpInstalled = TRUE;
	CList_SetAllOffline(TRUE);

 	return 0;
}

int PreShutdown(WPARAM wParam,LPARAM lParam)
{
	WM_RemoveAll();
	MM_RemoveAll();
	return 0;
}

int IconsChanged(WPARAM wParam,LPARAM lParam)
{
	if(g_LogOptions.GroupOpenIcon)
		DestroyIcon(g_LogOptions.GroupOpenIcon);
	g_LogOptions.GroupOpenIcon = CopyIcon(LoadSkinnedIcon(SKINICON_OTHER_GROUPOPEN));

	if(g_LogOptions.GroupClosedIcon)
		DestroyIcon(g_LogOptions.GroupClosedIcon);
	g_LogOptions.GroupClosedIcon = CopyIcon(LoadSkinnedIcon(SKINICON_OTHER_GROUPSHUT));
	WM_BroadcastMessage(NULL, GC_UPDATENICKLIST, 0, 0, FALSE);
	return 0;
}

int Service_Register(WPARAM wParam, LPARAM lParam)
{
	GCREGISTER *gcr = (GCREGISTER *)lParam;
	MODULE newModule;

	if(gcr->cbSize != SIZEOF_STRUCT_GCREGISTER_V1)
		return 1;


	newModule.pszModule = (char *)gcr->pszModule;
	newModule.pszModDispName = (char *)gcr->pszModuleDispName;
	newModule.nColorCount = gcr->nColors;
	newModule.crColors = gcr->pColors;
	newModule.bBold = gcr->dwFlags&GC_BOLD;
	newModule.bUnderline = gcr->dwFlags&GC_UNDERLINE;
	newModule.bItalics = gcr->dwFlags&GC_ITALICS;
	newModule.bColor = gcr->dwFlags&GC_COLOR;
	newModule.bBkgColor = gcr->dwFlags&GC_BKGCOLOR;
	newModule.bAckMsg = gcr->dwFlags&GC_ACKMSG;
	newModule.bChanMgr = gcr->dwFlags&GC_CHANMGR;
	newModule.iMaxText = gcr->iMaxText;

	if(MM_AddModule(&newModule))
	{
		CheckColorsInModule((char*)gcr->pszModule);
		CList_SetAllOffline(TRUE);
		return 0;
	}
	return 1;
}

int Service_NewChat(WPARAM wParam, LPARAM lParam)
{
	GCWINDOW *gcw =(GCWINDOW *)lParam;
	NEWCHATWINDOWLPARAM newWinData={0};
	HWND hChatWnd;

	if(gcw->cbSize != SIZEOF_STRUCT_GCWINDOW_V1)
		return 1;

	if(MM_FindModule((char *)gcw->pszModule))
	{
		HWND hwnd ;
		newWinData.iType =	gcw->iType;
		newWinData.dwItemData =	gcw->dwItemData;
		newWinData.pszID =	(char *)gcw->pszID;
		newWinData.pszModule =	(char *)gcw->pszModule;
		newWinData.pszName =	(char *)gcw->pszName;
		newWinData.pszStatusbarText =	(char *)gcw->pszStatusbarText;

		hwnd = WM_FindWindow((char *)gcw->pszID, (char *)gcw->pszModule);
		if(!hwnd)
		{
			hChatWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CHANNEL), NULL, RoomWndProc, (LPARAM)&newWinData);
			WM_AddWindow(hChatWnd, (char *)gcw->pszID, (char *)gcw->pszModule);
		}
		else
			SendMessage(hwnd, GC_NICKLISTREINIT, 0, 0);
		return 0;
	}
	return 1;
}

int Service_AddEvent(WPARAM wParam, LPARAM lParam)
{
	GCEVENT *gce = (GCEVENT*)lParam;
	GCDEST *gcd = (GCDEST*)gce->pDest;
	NEWEVENTLPARAM nul = {0};

	if(gce->cbSize != SIZEOF_STRUCT_GCEVENT_V1)
		return 1;

	nul.pszName = (char*)gce->pszNick;
	nul.pszUID = (char*)gce->pszUID;
	nul.pszText = (char*)gce->pszText;
	nul.pszStatus = (char*)gce->pszStatus;
	nul.pszUserInfo = (char *)gce->pszUserInfo;
	nul.bAddLog = gce->bAddToLog;
	nul.bIsMe = gce->bIsMe;
	nul.dwItemData = gce->dwItemData;
	nul.time = gce->time;
	nul.bBroadcasted = FALSE;

	if(gcd->pszID || gcd->iType == GC_EVENT_NOTICE || gcd->iType == GC_EVENT_INFORMATION)
	{
		int i = WM_SendMessage(gcd->pszID, gcd->pszModule, WM_USER+500+gcd->iType, (WPARAM)wParam, (LPARAM)&nul);
		if(gcd->iType == GC_EVENT_GETITEMDATA || gcd->iType == GC_EVENT_CONTROL)
			return nul.dwItemData;
		return i;
	}
	else
	{
		nul.bBroadcasted = TRUE;
		if(WM_BroadcastMessage(gcd->pszModule, WM_USER+500+gcd->iType, (WPARAM)wParam, (LPARAM)&nul, FALSE))
			return 0;
	}
	return 1;
}
int Service_GetAddEventPtr(WPARAM wParam, LPARAM lParam)
{
	GCPTRS * gp = (GCPTRS *) lParam;
	gp->pfnAddEvent = Service_AddEvent;

	return 0;
}
