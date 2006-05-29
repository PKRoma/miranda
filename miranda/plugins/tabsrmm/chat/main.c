/*
Chat module plugin for Miranda IM

Copyright (C) 2003 J�rgen Persson

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

//globals
struct MM_INTERFACE	mmi = {0};					// structure which keeps pointers to mirandas alloc, free and realloc
HANDLE			g_hWindowList;
HMENU			g_hMenu = NULL;

FONTINFO		aFonts[OPTIONS_FONTCOUNT];
HICON			hIcons[30];
BOOL			IEviewInstalled = FALSE;
HBRUSH			hEditBkgBrush = NULL;
HBRUSH			hListBkgBrush = NULL;

struct GlobalLogSettings_t g_Settings;

//HIMAGELIST		hIconsList = NULL;

char *			pszActiveWndID = 0;
char *			pszActiveWndModule = 0;
int             g_chat_integration_enabled = 0;
int             g_chat_fully_initialized = 0;

int Chat_Load(PLUGINLINK *link)
{
	BOOL bFlag = FALSE;

	#ifndef NDEBUG //mem leak detector :-) Thanks Tornado!
	int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
	flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
	_CrtSetDbgFlag(flag); // Set flag to the new value
	#endif

    if(!DBGetContactSettingByte(NULL, SRMSGMOD_T, "enable_chat", 0))
        return 0;
    
    g_chat_integration_enabled = 1;
    
	CallService(MS_SYSTEM_GET_MMI, 0, (LPARAM) &mmi);
	g_hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU));
    //OleInitialize(NULL);
	HookEvents();
	CreateServiceFunctions();
	CreateHookableEvents();
	OptionsInit();
	TabsInit();
	return 0;
}


int Chat_Unload(void)
{
    if(!g_chat_integration_enabled)
        return 0;
    
    DBWriteContactSettingWord(NULL, "Chat", "SplitterX", (WORD)g_Settings.iSplitterX);
	DBWriteContactSettingWord(NULL, "Chat", "splitY", (WORD)g_Settings.iSplitterY);
	DBWriteContactSettingDword(NULL, "Chat", "roomx", g_Settings.iX);
	DBWriteContactSettingDword(NULL, "Chat", "roomy", g_Settings.iY);
	DBWriteContactSettingDword(NULL, "Chat", "roomwidth" , g_Settings.iWidth);
	DBWriteContactSettingDword(NULL, "Chat", "roomheight", g_Settings.iHeight);

	CList_SetAllOffline(TRUE);

//	RichUtil_Unload();
	if(pszActiveWndID)
		free(pszActiveWndID);
	if(pszActiveWndModule)
		free(pszActiveWndModule);

	DestroyMenu(g_hMenu);
	DestroyServiceFunctions();
	FreeIcons();
	OptionsUnInit();
	UnhookEvents();
	return 0;
}

void LoadLogIcons(void)
{
    ZeroMemory(hIcons, sizeof(HICON) * (ICON_STATUS5 - ICON_ACTION));
    hIcons[ICON_ACTION] = LoadIconEx(IDI_ACTION, "log_action", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_ACTION),IMAGE_ICON,0,0,0);
	hIcons[ICON_ADDSTATUS] = LoadIconEx(IDI_ADDSTATUS, "log_addstatus", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_ADDSTATUS),IMAGE_ICON,0,0,0);
	hIcons[ICON_HIGHLIGHT] = LoadIconEx(IDI_HIGHLIGHT, "log_highlight", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_HIGHLIGHT),IMAGE_ICON,0,0,0);
	hIcons[ICON_INFO] = LoadIconEx(IDI_INFO, "log_info", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_INFO),IMAGE_ICON,0,0,0);
	hIcons[ICON_JOIN] = LoadIconEx(IDI_JOIN, "log_join", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_JOIN),IMAGE_ICON,0,0,0);
	hIcons[ICON_KICK] = LoadIconEx(IDI_KICK, "log_kick", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_KICK),IMAGE_ICON,0,0,0);
	hIcons[ICON_MESSAGE] = LoadIconEx(IDI_MESSAGE, "log_message_in", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_MESSAGE),IMAGE_ICON,0,0,0);
	hIcons[ICON_MESSAGEOUT] = LoadIconEx(IDI_MESSAGEOUT, "log_message_out", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_MESSAGEOUT),IMAGE_ICON,0,0,0);
	hIcons[ICON_NICK] = LoadIconEx(IDI_NICK, "log_nick", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_NICK),IMAGE_ICON,0,0,0);
	hIcons[ICON_NOTICE] = LoadIconEx(IDI_NOTICE, "log_notice", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_NOTICE),IMAGE_ICON,0,0,0);
	hIcons[ICON_PART] = LoadIconEx(IDI_PART, "log_part", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_PART),IMAGE_ICON,0,0,0);
	hIcons[ICON_QUIT] = LoadIconEx(IDI_QUIT, "log_quit", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_QUIT),IMAGE_ICON,0,0,0);
	hIcons[ICON_REMSTATUS] = LoadIconEx(IDI_REMSTATUS, "log_removestatus", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_REMSTATUS),IMAGE_ICON,0,0,0);
	hIcons[ICON_TOPIC] = LoadIconEx(IDI_TOPIC, "log_topic", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_TOPIC),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS1] = LoadIconEx(IDI_STATUS1, "status1", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS1),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS2] = LoadIconEx(IDI_STATUS2, "status2", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS2),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS3] = LoadIconEx(IDI_STATUS3, "status3", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS3),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS4] = LoadIconEx(IDI_STATUS4, "status4", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS4),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS0] = LoadIconEx(IDI_STATUS0, "status0", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS0),IMAGE_ICON,0,0,0);
	hIcons[ICON_STATUS5] = LoadIconEx(IDI_STATUS5, "status5", 16, 16); //LoadImage(g_hInst,MAKEINTRESOURCE(IDI_STATUS5),IMAGE_ICON,0,0,0);

	return;
}
void LoadIcons(void)
{
	int i;

	for(i = 0; i < 20; i++)
		hIcons[i] = NULL;

	LoadLogIcons();
	g_Settings.hIconOverlay = LoadIconEx(IDI_OVERLAY, "overlay", 16, 16);
	LoadMsgLogBitmaps();
	return ;
}

void FreeIcons(void)
{
	FreeMsgLogBitmaps();
	return;
}
