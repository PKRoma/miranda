/*
Chat module plugin for Miranda IM

Copyright (C) 2003 J�rgen Persson
Copyright 2003-2008 Miranda ICQ/IM project,

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

extern struct GlobalMessageData *g_dat;
extern CRITICAL_SECTION	cs;
//globals
HINSTANCE   g_hInst;
HMENU       g_hMenu = NULL;
HANDLE      hJoinMenuItem, hLeaveMenuItem;

FONTINFO    aFonts[OPTIONS_FONTCOUNT];
HICON       hIcons[30];
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
	OptionsInit();
	AddIcons();
	LoadIcons();
	{
		CLISTMENUITEM mi = { 0 };
		mi.cbSize = sizeof(mi);
		mi.position = -2000090001;
		mi.flags = CMIF_DEFAULT;
		mi.hIcon = LoadSkinnedIcon( SKINICON_CHAT_JOIN );
		mi.pszName = LPGEN("&Join");
		mi.pszService = "GChat/JoinChat";
		hJoinMenuItem = ( HANDLE )CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);

		mi.position = -2000090000;
		mi.flags = CMIF_NOTOFFLINE;
		mi.hIcon = LoadSkinnedIcon( SKINICON_CHAT_LEAVE );
		mi.pszName = LPGEN("&Leave");
		mi.pszService = "GChat/LeaveChat";
		hLeaveMenuItem = ( HANDLE )CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);
	}
	return 0;
}


int Chat_Unload(void)
{
	DBWriteContactSettingWord(NULL, "Chat", "SplitterX", (WORD)g_Settings.iSplitterX);
	DBWriteContactSettingWord(NULL, "Chat", "SplitterY", (WORD)g_Settings.iSplitterY);

	CList_SetAllOffline(TRUE);

	mir_free( pszActiveWndID );
	mir_free( pszActiveWndModule );

	DestroyHookableEvents();

	DestroyMenu(g_hMenu);
	FreeIcons();
	OptionsUnInit();
	DeleteCriticalSection(&cs);
	return 0;
}

void ReleaseLogIcons() {
	ReleaseIconEx("log_action");
	ReleaseIconEx("log_addstatus");
	ReleaseIconEx("log_highlight");
	ReleaseIconEx("log_info");
	ReleaseIconEx("log_join");
	ReleaseIconEx("log_kick");
	ReleaseIconEx("log_message_in");
	ReleaseIconEx("log_message_out");
	ReleaseIconEx("log_nick");
	ReleaseIconEx("log_notice");
	ReleaseIconEx("log_part");
	ReleaseIconEx("log_quit");
	ReleaseIconEx("log_removestatus");
	ReleaseIconEx("log_topic");
	ReleaseIconEx("status0");
	ReleaseIconEx("status1");
	ReleaseIconEx("status2");
	ReleaseIconEx("status3");
	ReleaseIconEx("status4");
	ReleaseIconEx("status5");
	ReleaseIconEx("window");
	ReleaseIconEx("overlay");
}

void LoadLogIcons(void)
{
	hIcons[ICON_ACTION] = LoadIconEx(IDI_ACTION, "log_action", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_ACTION),IMAGE_ICON,0,0,0);
	hIcons[ICON_ADDSTATUS] = LoadIconEx(IDI_ADDSTATUS, "log_addstatus", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_ADDSTATUS),IMAGE_ICON,0,0,0);
	hIcons[ICON_HIGHLIGHT] = LoadIconEx(IDI_NOTICE, "log_highlight", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_HIGHLIGHT),IMAGE_ICON,0,0,0);
	hIcons[ICON_INFO] = LoadIconEx(IDI_INFO, "log_info", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_INFO),IMAGE_ICON,0,0,0);
	hIcons[ICON_JOIN] = LoadIconEx(IDI_JOIN, "log_join", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_JOIN),IMAGE_ICON,0,0,0);
	hIcons[ICON_KICK] = LoadIconEx(IDI_KICK, "log_kick", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_KICK),IMAGE_ICON,0,0,0);
	hIcons[ICON_MESSAGE] = LoadIconEx(IDI_INCOMING, "log_message_in", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_MESSAGE),IMAGE_ICON,0,0,0);
	hIcons[ICON_MESSAGEOUT] = LoadIconEx(IDI_OUTGOING, "log_message_out", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_MESSAGEOUT),IMAGE_ICON,0,0,0);
	hIcons[ICON_NICK] = LoadIconEx(IDI_NICK, "log_nick", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_NICK),IMAGE_ICON,0,0,0);
	hIcons[ICON_NOTICE] = LoadIconEx(IDI_CHAT_NOTICE, "log_notice", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_NOTICE),IMAGE_ICON,0,0,0);
	hIcons[ICON_PART] = LoadIconEx(IDI_PART, "log_part", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_PART),IMAGE_ICON,0,0,0);
	hIcons[ICON_QUIT] = LoadIconEx(IDI_QUIT, "log_quit", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_QUIT),IMAGE_ICON,0,0,0);
	hIcons[ICON_REMSTATUS] = LoadIconEx(IDI_REMSTATUS, "log_removestatus", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_REMSTATUS),IMAGE_ICON,0,0,0);
	hIcons[ICON_TOPIC] = LoadIconEx(IDI_TOPIC, "log_topic", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_TOPIC),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS0] = LoadIconEx(IDI_STATUS0, "status0", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS0),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS1] = LoadIconEx(IDI_STATUS1, "status1", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS1),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS2] = LoadIconEx(IDI_STATUS2, "status2", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS2),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS3] = LoadIconEx(IDI_STATUS3, "status3", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS3),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS4] = LoadIconEx(IDI_STATUS4, "status4", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS4),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS5] = LoadIconEx(IDI_STATUS5, "status5", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS5),IMAGE_ICON,0,0,0);
	hIcons[ICON_WINDOW] = LoadIconEx(IDI_CHANMGR, "window", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS5),IMAGE_ICON,0,0,0);
	hIcons[ICON_OVERLAY] = LoadIconEx(IDI_OVERLAY, "overlay", 10, 10); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS5),IMAGE_ICON,0,0,0);

	return;
}
void LoadIcons(void)
{
	int i;

	for(i = 0; i < 20; i++)
		hIcons[i] = NULL;

	LoadLogIcons();
	LoadMsgLogBitmaps();

	return ;
}

void FreeIcons(void)
{
	FreeMsgLogBitmaps();
	return;
}
