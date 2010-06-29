/*
Chat module plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson
Copyright 2003-2009 Miranda ICQ/IM project,

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

#include "../commonheaders.h"
#include "chat.h"

void RegisterFonts( void );

extern struct GlobalMessageData *g_dat;
extern CRITICAL_SECTION	cs;
//globals
HINSTANCE   g_hInst;
HMENU       g_hMenu = NULL;
HANDLE      hJoinMenuItem, hLeaveMenuItem;

FONTINFO    aFonts[OPTIONS_FONTCOUNT];
HBRUSH      hListBkgBrush = NULL;
HBRUSH      hListSelectedBkgBrush = NULL;

TCHAR*      pszActiveWndID = 0;
char*       pszActiveWndModule = 0;

struct GlobalLogSettings_t g_Settings;

int Chat_Load()
{
	InitializeCriticalSection(&cs);
	g_hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU));
	CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) g_hMenu, 0);
	HookEvents();
	CreateServiceFunctions();
	CreateHookableEvents();
	return 0;
}

int Chat_Unload(void)
{
	DBWriteContactSettingWord(NULL, "Chat", "SplitterX", (WORD)g_Settings.iSplitterX);
	DBWriteContactSettingWord(NULL, "Chat", "SplitterY", (WORD)g_Settings.iSplitterY);

	CList_SetAllOffline(TRUE, NULL);

	mir_free( pszActiveWndID );
	mir_free( pszActiveWndModule );

	DestroyHookableEvents();

	DestroyMenu(g_hMenu);
	FreeIcons();
	OptionsUnInit();
	DeleteCriticalSection(&cs);
	return 0;
}

int Chat_ModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	char* mods[3] = { "Chat", "ChatFonts" };
	CallService( "DBEditorpp/RegisterModule", (WPARAM)mods, 2 );
	RegisterFonts();
	OptionsInit();
	LoadIcons();
	{
		CLISTMENUITEM mi = { 0 };
		mi.cbSize = sizeof(mi);
		mi.position = -2000090001;
		mi.flags = CMIF_DEFAULT | CMIF_ICONFROMICOLIB;
		mi.icolibItem = LoadSkinnedIconHandle( SKINICON_CHAT_JOIN );
		mi.pszName = LPGEN("&Join");
		mi.pszService = "GChat/JoinChat";
		hJoinMenuItem = ( HANDLE )CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);

		mi.position = -2000090000;
		mi.flags = CMIF_NOTOFFLINE | CMIF_ICONFROMICOLIB;
		mi.icolibItem = LoadSkinnedIconHandle( SKINICON_CHAT_LEAVE );
		mi.pszName = LPGEN("&Leave");
		mi.pszService = "GChat/LeaveChat";
		hLeaveMenuItem = ( HANDLE )CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);
	}
	CList_SetAllOffline(TRUE, NULL);
 	return 0;
}


void LoadIcons(void)
{
	LoadMsgLogBitmaps();
	return ;
}

void FreeIcons(void)
{
	FreeMsgLogBitmaps();
	return;
}
