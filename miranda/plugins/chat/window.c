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

extern HBRUSH		hEditBkgBrush;
extern HANDLE		hSendEvent;
extern HANDLE		hBuildMenuEvent ;
extern HINSTANCE	g_hInst;
extern HICON		hIcons[OPTIONS_ICONCOUNT];
extern struct		CREOleCallback reOleCallback;
extern HIMAGELIST	hImageList;
extern HMENU		g_hMenu;
extern BOOL			SmileyAddInstalled;

static WNDPROC OldSplitterProc;
static WNDPROC OldMessageProc;
static WNDPROC OldNicklistProc;
static WNDPROC OldFilterButtonProc;
static WNDPROC OldLogProc;
typedef struct
{
	time_t lastEnterTime;
	char szTabSave[20];
} MESSAGESUBDATA;

struct FilterList
{
    char *szDescr;
	int iEvent;
	HTREEITEM hItem;

// this one should also be in Translate( ) template
} static filterList[] = {
	{"Actions", GC_EVENT_ACTION, NULL},
	{"Messages", GC_EVENT_MESSAGE, NULL},
	{"Nick changes", GC_EVENT_NICK, NULL},
	{"Users joining", GC_EVENT_JOIN, NULL},
	{"Users leaving", GC_EVENT_PART, NULL},
	{"Topic changes", GC_EVENT_TOPIC, NULL},
	{"Status changes", GC_EVENT_ADDSTATUS, NULL},
	{"Information ", GC_EVENT_INFORMATION, NULL},
	{"Disconnects", GC_EVENT_QUIT, NULL},
	{"User kicks", GC_EVENT_KICK, NULL},
	{"Notices", GC_EVENT_NOTICE, NULL}
};
static LRESULT CALLBACK SplitterSubclassProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg) {
		case WM_NCHITTEST:
			return HTCLIENT;
		case WM_SETCURSOR:
		{	RECT rc;
			GetClientRect(hwnd,&rc);
			SetCursor(rc.right>rc.bottom?LoadCursor(NULL, IDC_SIZENS):LoadCursor(NULL, IDC_SIZEWE));
			return TRUE;
		}
		case WM_LBUTTONDOWN:
			SetCapture(hwnd);
			return 0;
		case WM_MOUSEMOVE:
			if(GetCapture()==hwnd) {
				RECT rc;
				GetClientRect(hwnd,&rc);
				SendMessage(GetParent(hwnd),GC_SPLITTERMOVED,rc.right>rc.bottom?(short)HIWORD(GetMessagePos())+rc.bottom/2:(short)LOWORD(GetMessagePos())+rc.right/2,(LPARAM)hwnd);
			}
			return 0;
		case WM_LBUTTONUP:
			ReleaseCapture();
			return 0;
	}
	return CallWindowProc(OldSplitterProc,hwnd,msg,wParam,lParam);
}

static void	InitButtons(HWND hwndDlg, CHATWINDOWDATA* dat)
{
	MODULE * pInfo = MM_FindModule(dat->pszModule);

	/* why the hell does this crash??????
	//	HICON hSmileyIcon = NULL;

	if(SmileyAddInstalled )
	{
		SMADD_GETICON smgi;
		SMADD_INFO smainfo;

        smainfo.cbSize = sizeof(smainfo);
        smainfo.Protocolname = "OOBI"; //dat->pszModule;
        CallService(MS_SMILEYADD_GETINFO, 0, (LPARAM)&smainfo);

		smgi.cbSize = sizeof(smgi);
		smgi.Protocolname = "OOBI"; //dat->pszModule;
		smgi.SmileySequence = ":)";
		smgi.SmileyIcon = NULL;
		smgi.Smileylength = 0;
		CallService(MS_SMILEYADD_GETSMILEYICON, 0, (LPARAM)&smgi);
		if (smgi.SmileyIcon)
			hSmileyIcon = smgi.SmileyIcon;
	}
*/
	SendDlgItemMessage(hwndDlg,IDC_SMILEY,BM_SETIMAGE,IMAGE_ICON,(LPARAM)/*hSmileyIcon?hSmileyIcon:*/hIcons[3]);
	SendDlgItemMessage(hwndDlg,IDC_BOLD,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIcons[1]);
	SendDlgItemMessage(hwndDlg,IDC_ITALICS,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIcons[2]);
	SendDlgItemMessage(hwndDlg,IDC_UNDERLINE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIcons[0]);
	SendDlgItemMessage(hwndDlg,IDC_COLOR,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIcons[7]);
	SendDlgItemMessage(hwndDlg,IDC_BKGCOLOR,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIcons[5]);
	SendDlgItemMessage(hwndDlg,IDC_HISTORY,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIcons[9]);
	SendDlgItemMessage(hwndDlg,IDC_SHOWNICKLIST,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIcons[10]);
	SendDlgItemMessage(hwndDlg,IDC_CHANMGR,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIcons[4]);
	SendDlgItemMessage(hwndDlg,IDC_FILTER,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIcons[8]);

	SendDlgItemMessage(hwndDlg,IDC_SMILEY, BUTTONSETASFLATBTN, 0, 0);
	SendDlgItemMessage(hwndDlg,IDC_BOLD, BUTTONSETASFLATBTN, 0, 0);
	SendDlgItemMessage(hwndDlg,IDC_ITALICS, BUTTONSETASFLATBTN, 0, 0);
	SendDlgItemMessage(hwndDlg,IDC_UNDERLINE, BUTTONSETASFLATBTN, 0, 0);
	SendDlgItemMessage(hwndDlg,IDC_BKGCOLOR, BUTTONSETASFLATBTN, 0, 0);
	SendDlgItemMessage(hwndDlg,IDC_COLOR, BUTTONSETASFLATBTN, 0, 0);
	SendDlgItemMessage(hwndDlg,IDC_HISTORY, BUTTONSETASFLATBTN, 0, 0);
	SendDlgItemMessage(hwndDlg,IDC_SHOWNICKLIST, BUTTONSETASFLATBTN, 0, 0);
	SendDlgItemMessage(hwndDlg,IDC_CHANMGR, BUTTONSETASFLATBTN, 0, 0);
	SendDlgItemMessage(hwndDlg,IDC_FILTER, BUTTONSETASFLATBTN, 0, 0);

	SendMessage(GetDlgItem(hwndDlg,IDC_SMILEY), BUTTONADDTOOLTIP, (WPARAM)Translate("Insert a smiley"), 0);
	SendMessage(GetDlgItem(hwndDlg,IDC_BOLD), BUTTONADDTOOLTIP, (WPARAM)Translate("Make the text bold"), 0);
	SendMessage(GetDlgItem(hwndDlg,IDC_ITALICS), BUTTONADDTOOLTIP, (WPARAM)Translate("Make the text italicized"), 0);
	SendMessage(GetDlgItem(hwndDlg,IDC_UNDERLINE), BUTTONADDTOOLTIP, (WPARAM)Translate("Make the text underlined"), 0);
	SendMessage(GetDlgItem(hwndDlg,IDC_BKGCOLOR), BUTTONADDTOOLTIP, (WPARAM)Translate("Select a background color for the text"), 0);
	SendMessage(GetDlgItem(hwndDlg,IDC_COLOR), BUTTONADDTOOLTIP, (WPARAM)Translate("Select a foreground color for the text"), 0);
	SendMessage(GetDlgItem(hwndDlg,IDC_HISTORY), BUTTONADDTOOLTIP, (WPARAM)Translate("Show the history"), 0);
	SendMessage(GetDlgItem(hwndDlg,IDC_SHOWNICKLIST), BUTTONADDTOOLTIP, (WPARAM)Translate("Show/hide the nicklist"), 0);
	SendMessage(GetDlgItem(hwndDlg,IDC_CHANMGR), BUTTONADDTOOLTIP, (WPARAM)Translate("Control this room"), 0);
	SendMessage(GetDlgItem(hwndDlg,IDC_FILTER), BUTTONADDTOOLTIP, (WPARAM)Translate("Enable/disable the event filter"), 0);
	SendDlgItemMessage(hwndDlg, IDC_BOLD, BUTTONSETASPUSHBTN, 0, 0);
	SendDlgItemMessage(hwndDlg, IDC_ITALICS, BUTTONSETASPUSHBTN, 0, 0);
	SendDlgItemMessage(hwndDlg, IDC_UNDERLINE, BUTTONSETASPUSHBTN, 0, 0);
	SendDlgItemMessage(hwndDlg, IDC_COLOR, BUTTONSETASPUSHBTN, 0, 0);
	SendDlgItemMessage(hwndDlg, IDC_BKGCOLOR, BUTTONSETASPUSHBTN, 0, 0);
	SendDlgItemMessage(hwndDlg, IDC_SHOWNICKLIST, BUTTONSETASPUSHBTN, 0, 0);
	SendDlgItemMessage(hwndDlg, IDC_FILTER, BUTTONSETASPUSHBTN, 0, 0);
                        
	if (pInfo)
	{
		EnableWindow(GetDlgItem(hwndDlg, IDC_BOLD), pInfo->bBold);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ITALICS), pInfo->bItalics);
		EnableWindow(GetDlgItem(hwndDlg, IDC_UNDERLINE), pInfo->bUnderline);
		EnableWindow(GetDlgItem(hwndDlg, IDC_COLOR), pInfo->bColor);
		EnableWindow(GetDlgItem(hwndDlg, IDC_BKGCOLOR), pInfo->bBkgColor);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHANMGR), pInfo->bChanMgr);
	}
	CheckDlgButton(hwndDlg, IDC_FILTER,  dat->bFilterEnabled?BST_CHECKED:BST_UNCHECKED);
}


static UINT CreateGCMenu(HWND hwndDlg, HMENU *hMenu, int iIndex, POINT pt, CHATWINDOWDATA * dat, char * pszUID)
{
	GCMENUITEMS gcmi = {0};
	int i;
	HMENU hSubMenu = 0;

	*hMenu = GetSubMenu(g_hMenu, iIndex);
	CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) *hMenu, 0);
	gcmi.pszID = dat->pszID;
	gcmi.pszModule = dat->pszModule;
	gcmi.pszUID = pszUID;

	if(iIndex == 1)
	{
		int i = GetRichTextLength(GetDlgItem(hwndDlg, IDC_LOG));

		EnableMenuItem(*hMenu, ID_CLEARLOG, MF_ENABLED);
		EnableMenuItem(*hMenu, ID_COPYALL, MF_ENABLED);
		if (!i)
		{
			EnableMenuItem(*hMenu, ID_COPYALL, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(*hMenu, ID_CLEARLOG, MF_BYCOMMAND | MF_GRAYED);
		}
		gcmi.Type = MENU_ON_LOG;
	}
	else if(iIndex == 0)
		gcmi.Type = MENU_ON_NICKLIST;

	NotifyEventHooks(hBuildMenuEvent, 0, (WPARAM)&gcmi);
	if (gcmi.nItems > 0)
		AppendMenu(*hMenu, MF_SEPARATOR, 0, 0);
	for(i = 0; i < gcmi.nItems; i++)
	{
		if(gcmi.Item[i].uType == MENU_NEWPOPUP)
		{
			hSubMenu = CreateMenu();
			AppendMenu(*hMenu, gcmi.Item[i].bDisabled?MF_POPUP|MF_GRAYED:MF_POPUP, (UINT)hSubMenu, gcmi.Item[i].pszDesc);
		}
		else if(gcmi.Item[i].uType == MENU_POPUPITEM)
			AppendMenu(hSubMenu==0?*hMenu:hSubMenu, gcmi.Item[i].bDisabled?MF_STRING|MF_GRAYED:MF_STRING, gcmi.Item[i].dwID, gcmi.Item[i].pszDesc);
		else if(gcmi.Item[i].uType == MENU_POPUPSEPARATOR)
			AppendMenu(hSubMenu==0?*hMenu:hSubMenu, MF_SEPARATOR, 0, gcmi.Item[i].pszDesc);
		else if(gcmi.Item[i].uType == MENU_SEPARATOR)
			AppendMenu(*hMenu, MF_SEPARATOR, 0, gcmi.Item[i].pszDesc);
		else if(gcmi.Item[i].uType == MENU_ITEM)
			AppendMenu(*hMenu, gcmi.Item[i].bDisabled?MF_STRING|MF_GRAYED:MF_STRING, gcmi.Item[i].dwID, gcmi.Item[i].pszDesc);
	}
	return TrackPopupMenu(*hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);

}
static void DestroyGCMenu(HMENU *hMenu, int iIndex)
{
	MENUITEMINFO mi;
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_SUBMENU;
	while(GetMenuItemInfo(*hMenu, iIndex, TRUE, &mi)) 
	{
		if(mi.hSubMenu != NULL)
			DestroyMenu(mi.hSubMenu);
		RemoveMenu(*hMenu, iIndex, MF_BYPOSITION);
	}
}
static BOOL DoEventHookAsync(HWND hwnd, char * pszID, char * pszModule, int iType, char * pszUID, char * pszText, DWORD dwItem)
{
	GCHOOK * gch = malloc(sizeof(GCHOOK));
	GCDEST * gcd = malloc(sizeof(GCDEST));

	if (pszID)
	{
		gcd->pszID = malloc(lstrlen(pszID)+1);
		lstrcpy(gcd->pszID, pszID);
	} else gcd->pszID = NULL;
	if (pszModule)
	{
		gcd->pszModule = malloc(lstrlen(pszModule)+1);
		lstrcpy(gcd->pszModule, pszModule);
	}else gcd->pszModule = NULL;
	if (pszUID)
	{
		gch->pszUID = malloc(lstrlen(pszUID)+1);
		lstrcpy(gch->pszUID, pszUID);
	}else gch->pszUID = NULL;
	if (pszText)
	{
		gch->pszText = malloc(lstrlen(pszText)+1);
		lstrcpy(gch->pszText, pszText);
	}else gch->pszText = NULL;

	gcd->iType = iType;
	gch->dwData = dwItem;
	gch->pDest = gcd;
	PostMessage(hwnd, GC_FIREHOOK, 0, (LPARAM) gch);
	return TRUE;
}
static BOOL DoEventHook(HWND hwnd, char * pszID, char * pszModule, int iType, char * pszUID, char * pszText, DWORD dwItem)
{
	GCHOOK gch = {0};
	GCDEST gcd = {0};

	gcd.pszID = pszID;
	gcd.pszModule = pszModule;
	gcd.iType = iType;
	gch.pDest = &gcd;
	gch.pszText = pszText;
	gch.pszUID = pszUID;
	gch.dwData = dwItem;
	NotifyEventHooks(hSendEvent,0,(WPARAM)&gch);
	return TRUE;
}

static BOOL IsHighlighted(CHATWINDOWDATA * dat, char * pszText)
{
	if(g_LogOptions.HighlightEnabled && g_LogOptions.pszHighlightWords && pszText)
	{

		char* p1 = g_LogOptions.pszHighlightWords;
		char* p2 = NULL;
		char* p3 = pszText;
		static char szWord1[1000];
		static char szWord2[1000];
		static char szTrimString[] = ":,.!?;\'>)";
		
		// compare word for word
		while (*p1 != '\0')
		{
			// find the next/first word in the highlight word string
			// skip 'spaces' be4 the word
			while(*p1 == ' ' && *p1 != '\0') 
				p1 += 1;
			
			//find the end of the word
			p2 = strchr(p1, ' '); 
			if (!p2)
				p2 = strchr(p1, '\0');
			if (p1 == p2)
				return FALSE;

			// copy the word into szWord1
			lstrcpyn(szWord1, p1, p2-p1>998?999:p2-p1+1); 
			p1 = p2;
			
			// replace %m with the users nickname
			p2 = strchr(szWord1, '%');
			if (p2 && p2[1] == 'm' && dat->pMe)
			{
				char szTemp[50];

				p2[1] = 's';
				lstrcpyn(szTemp, szWord1, 999);
				_snprintf(szWord1, sizeof(szWord1), szTemp, dat->pMe->pszNick);
			}

			// time to get the next/first word in the incoming text string
			while(*p3 != '\0') 
			{
				// skip 'spaces' be4 the word
				while(*p3 == ' ' && *p3 != '\0')
					p3 += 1;
				
				//find the end of the word
				p2 = strchr(p3, ' ');
				if (!p2)
					p2 = strchr(p3, '\0');


				if (p3 != p2)
				{
					// eliminate ending character if needed
					if(p2-p3 > 1 && strchr(szTrimString, p2[-1]))
						p2 -= 1;

					// copy the word into szWord2 and remove formatting
					lstrcpyn(szWord2, p3, p2-p3>998?999:p2-p3+1); 

					// reset the pointer if it was touched because of an ending character
					if(*p2 != '\0' && *p2 != ' ')
						p2 += 1;
					p3 = p2;

					CharLower(szWord1);
					CharLower(szWord2);
					
					// compare the words, using wildcards
					if (WCCmp(szWord1, RemoveFormatting(szWord2)))
						return TRUE;
				}
			}
			p3 = pszText;
		}

	}
	return FALSE;
}
static int RoomWndResize(HWND hwndDlg,LPARAM lParam,UTILRESIZECONTROL *urc)
{
	CHATWINDOWDATA *dat=(CHATWINDOWDATA*)lParam;
	RECT rc;

	BOOL		bTop = (BOOL)DBGetContactSettingByte(NULL, "Chat", "ShowTopButtons", 1);
	BOOL		bFormat = (BOOL)DBGetContactSettingByte(NULL, "Chat", "ShowFormatButtons", 1);
	BOOL		bSend = (BOOL)DBGetContactSettingByte(NULL, "Chat", "ShowSend", 0);
	BOOL		bNick = IsWindowVisible(GetDlgItem(hwndDlg, IDC_NICKLIST));
	ShowWindow(GetDlgItem(hwndDlg, IDC_SMILEY), bFormat?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_BOLD), bFormat?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_UNDERLINE), bFormat?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_ITALICS), bFormat?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_COLOR), bFormat?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_BKGCOLOR), bFormat?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_HISTORY), bTop?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_SHOWNICKLIST), bTop?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_FILTER), bTop?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_CHANMGR), bTop?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDOK), bSend?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_SPLITTERX), bNick?SW_SHOW:SW_HIDE);

	switch(urc->wId) {
		case IDOK:
			GetWindowRect(dat->hwndStatus, &rc);
			urc->rcItem.top = urc->dlgNewSize.cy - dat->iSplitterY+27;
			urc->rcItem.bottom = urc->dlgNewSize.cy - (rc.bottom-rc.top)-1;
			return RD_ANCHORX_RIGHT|RD_ANCHORY_CUSTOM;
		case IDC_HISTORY:
		case IDC_CHANMGR:
		case IDC_SHOWNICKLIST:
		case IDC_FILTER:
			return RD_ANCHORX_RIGHT|RD_ANCHORY_TOP;
		case IDC_LOG:
			urc->rcItem.top = bTop|g_LogOptions.bShowName?27:0;
			urc->rcItem.right = bNick?urc->dlgNewSize.cx - dat->iSplitterX:urc->dlgNewSize.cx;
			urc->rcItem.bottom = bFormat?(urc->dlgNewSize.cy - dat->iSplitterY):(urc->dlgNewSize.cy - dat->iSplitterY+25);
			return RD_ANCHORX_LEFT|RD_ANCHORY_CUSTOM;
		case IDC_SPLITTERY:
			urc->rcItem.top = bFormat?urc->dlgNewSize.cy - dat->iSplitterY:urc->dlgNewSize.cy - dat->iSplitterY+25;
			urc->rcItem.bottom = bFormat?(urc->dlgNewSize.cy - dat->iSplitterY+2):(urc->dlgNewSize.cy - dat->iSplitterY+27);
			return RD_ANCHORX_WIDTH|RD_ANCHORY_CUSTOM;
		case IDC_MESSAGE:
			GetWindowRect(dat->hwndStatus, &rc);
			urc->rcItem.right = bSend?urc->dlgNewSize.cx - 64:urc->dlgNewSize.cx ;
			urc->rcItem.top = urc->dlgNewSize.cy - dat->iSplitterY+27;
			urc->rcItem.bottom = urc->dlgNewSize.cy - (rc.bottom-rc.top)-1 ;
			return RD_ANCHORX_LEFT|RD_ANCHORY_CUSTOM;
		case IDC_SMILEY:
		case IDC_ITALICS:
		case IDC_BOLD:
		case IDC_UNDERLINE:
		case IDC_COLOR:
		case IDC_BKGCOLOR:
			urc->rcItem.top = urc->dlgNewSize.cy - dat->iSplitterY+2;
			urc->rcItem.bottom = urc->dlgNewSize.cy - dat->iSplitterY+26;
			return RD_ANCHORX_LEFT|RD_ANCHORY_CUSTOM;
		case IDC_SPLITTERX:
			urc->rcItem.right = urc->dlgNewSize.cx - dat->iSplitterX+3;
			urc->rcItem.left = urc->dlgNewSize.cx - dat->iSplitterX;
			urc->rcItem.bottom = bFormat?urc->dlgNewSize.cy - dat->iSplitterY-3:urc->dlgNewSize.cy - dat->iSplitterY+20;
			urc->rcItem.top = bTop || g_LogOptions.bShowName?25:0;
			return RD_ANCHORX_CUSTOM|RD_ANCHORY_CUSTOM;
		case IDC_NICKLIST:
			urc->rcItem.top = bTop|g_LogOptions.bShowName?27:0;
			urc->rcItem.right = urc->dlgNewSize.cx ;
			urc->rcItem.left = urc->dlgNewSize.cx - dat->iSplitterX + 3;
			urc->rcItem.bottom = bFormat?urc->dlgNewSize.cy - dat->iSplitterY:urc->dlgNewSize.cy - dat->iSplitterY+25;
			return RD_ANCHORX_CUSTOM|RD_ANCHORY_CUSTOM;
	
	}
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
	
}

static LRESULT CALLBACK MessageSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{ 
    MESSAGESUBDATA *dat;
	CHATWINDOWDATA *Parentdat;

	Parentdat=(CHATWINDOWDATA *)GetWindowLong(GetParent(hwnd),GWL_USERDATA);
    dat = (MESSAGESUBDATA *) GetWindowLong(hwnd, GWL_USERDATA);
    switch (msg) {
        case EM_SUBCLASSED:
			dat = (MESSAGESUBDATA *) malloc(sizeof(MESSAGESUBDATA));

			SetWindowLong(hwnd, GWL_USERDATA, (LONG) dat);
			dat->szTabSave[0] = '\0';
			dat->lastEnterTime = 0;
			return 0;
		case WM_MOUSEWHEEL:
			SendMessage(GetDlgItem(GetParent(hwnd), IDC_LOG), WM_MOUSEWHEEL, wParam, lParam);
			dat->lastEnterTime = 0;
			return TRUE;
			break;
		case EM_REPLACESEL:
			PostMessage(hwnd, EM_ACTIVATE, 0, 0);
			break;
		case EM_ACTIVATE:
			SetActiveWindow(GetParent(hwnd));
			break;
 		case WM_CHAR:
			if (GetWindowLong(hwnd, GWL_STYLE) & ES_READONLY)
				break;

			if (wParam == 9 && GetKeyState(VK_CONTROL) & 0x8000 && !(GetKeyState(VK_MENU) & 0x8000)) // ctrl-i (italics)
			{
				return TRUE;
			}
			if (wParam == VK_SPACE && GetKeyState(VK_CONTROL) & 0x8000 && !(GetKeyState(VK_MENU) & 0x8000)) // ctrl-space (paste clean text)
			{
				return TRUE;
			}

			if (wParam == '\n' || wParam == '\r') 
			{
				if (((GetKeyState(VK_CONTROL) & 0x8000) != 0) ^ (0 != DBGetContactSettingByte(NULL, "Chat", "SendOnEnter", 1))) 
				{

					PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
					return 0;
				}
				if (DBGetContactSettingByte(NULL, "Chat", "SendOnDblEnter", 0)) 
				{
					if (dat->lastEnterTime + 2 < time(NULL))
						dat->lastEnterTime = time(NULL);
					else 
					{
						SendMessage(hwnd, WM_KEYDOWN, VK_BACK, 0);
						SendMessage(hwnd, WM_KEYUP, VK_BACK, 0);
						PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
						return 0;
					}
				}
			}
			else
				dat->lastEnterTime = 0;

			if (wParam == 1 && GetKeyState(VK_CONTROL) & 0x8000 && !(GetKeyState(VK_MENU) & 0x8000)) {      //ctrl-a
				SendMessage(hwnd, EM_SETSEL, 0, -1);
				return 0;
			}
		break;
		case WM_KEYDOWN:
			{
			static int start, end;
 			if (wParam == VK_RETURN) 
			{
				dat->szTabSave[0] = '\0';
				if (((GetKeyState(VK_CONTROL) & 0x8000) != 0) ^ (0 != DBGetContactSettingByte(NULL, "Chat", "SendOnEnter", 1))) 
				{
					return 0;
				}
				if (DBGetContactSettingByte(NULL, "Chat", "SendOnDblEnter", 0)) 
				{
					if (dat->lastEnterTime + 2 >= time(NULL))
					{
						return 0;
					}
					break;
				}
				break;
			}
           if (wParam == VK_TAB && !(GetKeyState(VK_CONTROL) & 0x8000)) {    //tab-autocomplete
                char *pszText = NULL;
                int iLen;
				GETTEXTLENGTHEX gtl = {0};
				GETTEXTEX gt = {0};
				LRESULT lResult = (LRESULT)SendMessage(hwnd, EM_GETSEL, (WPARAM)NULL, (LPARAM)NULL);

				SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
				start = LOWORD(lResult);
				end = HIWORD(lResult);
				SendMessage(hwnd, EM_SETSEL, end, end);
				gtl.flags = GTL_PRECISE;
				gtl.codepage = CP_ACP;
				iLen = SendMessage(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, (LPARAM)NULL);
				if (iLen >0)
				{
					char *pszName = NULL;
					char *pszSelName = NULL;
					pszText = malloc(iLen+100);

					gt.cb = iLen+99;
					gt.flags = GT_DEFAULT;
					gt.codepage = CP_ACP;

					SendMessage(hwnd, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)pszText);
					while ( start >0 && pszText[start-1] != ' ' && pszText[start-1] != 13 && pszText[start-1] != VK_TAB)
						start--;
					while (end < iLen && pszText[end] != ' ' && pszText[end] != 13 && pszText[end-1] != VK_TAB)
						end ++;

					if( dat->szTabSave[0] =='\0')
					{
						lstrcpyn(dat->szTabSave, pszText+start, end-start+1);
					}
					pszSelName= malloc(end-start+1);
					lstrcpyn(pszSelName, pszText+start, end-start+1);
					pszName = UM_FindUserAutoComplete(Parentdat->pUserList, dat->szTabSave, pszSelName);
					if(pszName == NULL)
					{
						pszName = dat->szTabSave;
						SendMessage(hwnd, EM_SETSEL, start, end);
						if (end !=start)
							SendMessage(hwnd, EM_REPLACESEL, FALSE, (LPARAM) pszName);
						dat->szTabSave[0] = '\0';
					}
					else
					{
						SendMessage(hwnd, EM_SETSEL, start, end);
						if (end !=start)
							SendMessage(hwnd, EM_REPLACESEL, FALSE, (LPARAM) pszName);
					}
					free(pszText);
					free(pszSelName);
					SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
					RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);

					return 0;
				}

				SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
				RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
			}
			else
			{
				if(dat->szTabSave[0] != '\0' && wParam != VK_RIGHT && wParam != VK_LEFT 
					&& wParam != VK_SPACE && wParam != VK_RETURN && wParam != VK_BACK 
					&& wParam != VK_DELETE )
				{

					if(g_LogOptions.AddColonToAutoComplete && start == 0)
					{
						SendMessage(hwnd, EM_REPLACESEL, FALSE, (LPARAM) ": ");
					}
				}
				dat->szTabSave[0] = '\0';
			}
			}
			if (wParam == 0x49 && GetKeyState(VK_CONTROL) & 0x8000 && !(GetKeyState(VK_MENU) & 0x8000)) // ctrl-i (italics)
			{
				CheckDlgButton(GetParent(hwnd), IDC_ITALICS, IsDlgButtonChecked(GetParent(hwnd), IDC_ITALICS) == BST_UNCHECKED?BST_CHECKED:BST_UNCHECKED);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_ITALICS, 0), 0);
				return TRUE;		
			}			
			if (wParam == 0x42 && GetKeyState(VK_CONTROL) & 0x8000 && !(GetKeyState(VK_MENU) & 0x8000) ) // ctrl-b (bold)
			{
				CheckDlgButton(GetParent(hwnd), IDC_BOLD, IsDlgButtonChecked(GetParent(hwnd), IDC_BOLD) == BST_UNCHECKED?BST_CHECKED:BST_UNCHECKED);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_BOLD, 0), 0);
				return TRUE;
		
			}			
			if (wParam == 0x55 && GetKeyState(VK_CONTROL) & 0x8000 && !(GetKeyState(VK_MENU) & 0x8000)) // ctrl-u (paste clean text)
			{
				CheckDlgButton(GetParent(hwnd), IDC_UNDERLINE, IsDlgButtonChecked(GetParent(hwnd), IDC_UNDERLINE) == BST_UNCHECKED?BST_CHECKED:BST_UNCHECKED);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_UNDERLINE, 0), 0);
				return TRUE;
		
			}			
			if (wParam == 0x4b && GetKeyState(VK_CONTROL) & 0x8000 && !(GetKeyState(VK_MENU) & 0x8000)) // ctrl-k (paste clean text)
			{
				CheckDlgButton(GetParent(hwnd), IDC_COLOR, IsDlgButtonChecked(GetParent(hwnd), IDC_COLOR) == BST_UNCHECKED?BST_CHECKED:BST_UNCHECKED);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_COLOR, 0), 0);
				return TRUE;
		
			}			
			if (wParam == VK_SPACE && GetKeyState(VK_CONTROL) & 0x8000 && !(GetKeyState(VK_MENU) & 0x8000)) // ctrl-space (paste clean text)
			{
				CheckDlgButton(GetParent(hwnd), IDC_BKGCOLOR, BST_UNCHECKED);
				CheckDlgButton(GetParent(hwnd), IDC_COLOR, BST_UNCHECKED);
				CheckDlgButton(GetParent(hwnd), IDC_BOLD, BST_UNCHECKED);
				CheckDlgButton(GetParent(hwnd), IDC_UNDERLINE, BST_UNCHECKED);
				CheckDlgButton(GetParent(hwnd), IDC_ITALICS, BST_UNCHECKED);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_BKGCOLOR, 0), 0);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_COLOR, 0), 0);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_BOLD, 0), 0);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_UNDERLINE, 0), 0);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_ITALICS, 0), 0);
				return TRUE;
		
			}			
			if (wParam == 0x4c && GetKeyState(VK_CONTROL) & 0x8000 && !(GetKeyState(VK_MENU) & 0x8000)) // ctrl-l (paste clean text)
			{
				CheckDlgButton(GetParent(hwnd), IDC_BKGCOLOR, IsDlgButtonChecked(GetParent(hwnd), IDC_BKGCOLOR) == BST_UNCHECKED?BST_CHECKED:BST_UNCHECKED);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_BKGCOLOR, 0), 0);
				return TRUE;
		
			}			
			if (wParam == 0x46 && GetKeyState(VK_CONTROL) & 0x8000 && !(GetKeyState(VK_MENU) & 0x8000)) // ctrl-f (paste clean text)
			{
				CheckDlgButton(GetParent(hwnd), IDC_FILTER, IsDlgButtonChecked(GetParent(hwnd), IDC_FILTER) == BST_UNCHECKED?BST_CHECKED:BST_UNCHECKED);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_FILTER, 0), 0);
				return TRUE;
		
			}			
			if (wParam == 0x4e && GetKeyState(VK_CONTROL) & 0x8000 && !(GetKeyState(VK_MENU) & 0x8000)) // ctrl-n (paste clean text)
			{
				CheckDlgButton(GetParent(hwnd), IDC_SHOWNICKLIST, IsDlgButtonChecked(GetParent(hwnd), IDC_SHOWNICKLIST) == BST_UNCHECKED?BST_CHECKED:BST_UNCHECKED);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_SHOWNICKLIST, 0), 0);
				return TRUE;
		
			}			
			if (wParam == 0x48 && GetKeyState(VK_CONTROL) & 0x8000 && !(GetKeyState(VK_MENU) & 0x8000)) // ctrl-h (paste clean text)
			{
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_HISTORY, 0), 0);
				return TRUE;
		
			}			
			if (wParam == 0x4f && GetKeyState(VK_CONTROL) & 0x8000 && !(GetKeyState(VK_MENU) & 0x8000)) // ctrl-o (paste clean text)
			{
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_CHANMGR, 0), 0);
				return TRUE;
		
			}			
			if ((wParam == 45 && GetKeyState(VK_SHIFT) & 0x8000 || wParam == 0x56 && GetKeyState(VK_CONTROL) & 0x8000 )&& !(GetKeyState(VK_MENU) & 0x8000)) // ctrl-v (paste clean text)
			{
				SendMessage(hwnd, EM_PASTESPECIAL, CF_TEXT, 0);
				return TRUE;
		
			}			
			if (wParam == 0x57 && GetKeyState(VK_CONTROL) & 0x8000 && !(GetKeyState(VK_MENU) & 0x8000)) // ctrl-w (close window)
			{
				PostMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
				return TRUE;
		
			}			
			if (wParam == VK_NEXT || wParam == VK_PRIOR)
			{
				HWND htemp = GetParent(hwnd);
				SendDlgItemMessage(htemp, IDC_LOG, msg, wParam, lParam);
		        dat->lastEnterTime = 0;
				return TRUE;
			}
			if (wParam == VK_UP && (GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000))
			{
			    int iLen;
				GETTEXTLENGTHEX gtl = {0};
				SETTEXTEX ste;
				LOGFONT lf;
				char *lpPrevCmd = WM_GetPrevCommand(Parentdat->pszID, Parentdat->pszModule);

				SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);

				LoadMsgDlgFont(17, &lf, NULL);
				ste.flags = ST_DEFAULT;
				ste.codepage = CP_ACP;
				if(lpPrevCmd)
					SendMessage(hwnd, EM_SETTEXTEX, (WPARAM)&ste, (LPARAM)lpPrevCmd);
				else
					SetWindowText(hwnd, "");

				gtl.flags = GTL_PRECISE;
				gtl.codepage = CP_ACP;
				iLen = SendMessage(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, (LPARAM)NULL);
				SendMessage(hwnd, EM_SCROLLCARET, 0,0);
				SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
				RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
				SendMessage(hwnd, EM_SETSEL,iLen,iLen);
		        dat->lastEnterTime = 0;
				return TRUE;
			}
			if (wParam == VK_DOWN && (GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000))
			{

				int iLen;
				GETTEXTLENGTHEX gtl = {0};
				SETTEXTEX ste;

				char *lpPrevCmd = WM_GetNextCommand(Parentdat->pszID, Parentdat->pszModule);
				SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);

				ste.flags = ST_DEFAULT;
				ste.codepage = CP_ACP;
				if(lpPrevCmd)
					SendMessage(hwnd, EM_SETTEXTEX, (WPARAM)&ste, (LPARAM) lpPrevCmd);
				else
					SetWindowText(hwnd, "");

				gtl.flags = GTL_PRECISE;
				gtl.codepage = CP_ACP;
				iLen = SendMessage(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, (LPARAM)NULL);
				SendMessage(hwnd, EM_SCROLLCARET, 0,0);
				SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
				RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
				SendMessage(hwnd, EM_SETSEL,iLen,iLen);
		        dat->lastEnterTime = 0;
				return TRUE;

			}
			if (wParam == VK_RETURN)
                break;
            //fall through
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_KILLFOCUS:

			dat->lastEnterTime = 0;
            break;
		case WM_RBUTTONDOWN:
			{
				CHARRANGE sel, all = { 0, -1 };
				POINT pt;
				UINT uID = 0;
				HMENU hSubMenu;

				hSubMenu = GetSubMenu(g_hMenu, 4);
				CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hSubMenu, 0);
				SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM) & sel);

				EnableMenuItem(hSubMenu, ID_MESSAGE_UNDO, SendMessage(hwnd, EM_CANUNDO, 0,0)?MF_ENABLED:MF_GRAYED);
				EnableMenuItem(hSubMenu, ID_MESSAGE_REDO, SendMessage(hwnd, EM_CANREDO, 0,0)?MF_ENABLED:MF_GRAYED);
				EnableMenuItem(hSubMenu, ID_MESSAGE_COPY, sel.cpMax!=sel.cpMin?MF_ENABLED:MF_GRAYED);
				EnableMenuItem(hSubMenu, ID_MESSAGE_CUT, sel.cpMax!=sel.cpMin?MF_ENABLED:MF_GRAYED);

				dat->lastEnterTime = 0;

				pt.x = (short) LOWORD(lParam);
				pt.y = (short) HIWORD(lParam);
				ClientToScreen(hwnd, &pt);
				
				uID = TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
				switch (uID) 
				{
				case 0:
					break;
               case ID_MESSAGE_UNDO:
					{
                    SendMessage(hwnd, EM_UNDO, 0, 0);
                    }break;
               case ID_MESSAGE_REDO:
					{
                    SendMessage(hwnd, EM_REDO, 0, 0);
                    }break;
                case ID_MESSAGE_COPY:
					{
                    SendMessage(hwnd, WM_COPY, 0, 0);
                    }break;
               case ID_MESSAGE_CUT:
					{
                    SendMessage(hwnd, WM_CUT, 0, 0);
                    }break;
               case ID_MESSAGE_PASTE:
					{
                    SendMessage(hwnd, EM_PASTESPECIAL, CF_TEXT, 0);
                    }break;
                 case ID_MESSAGE_SELECTALL:
					{
                    SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM) & all);
                    }break;
                case ID_MESSAGE_CLEAR:
					{
                    SetWindowText(hwnd, "");
                    }break;
			   default:
					break;

				}
				PostMessage(hwnd, WM_KEYUP, 0, 0 );

			}break;
			
        case WM_KEYUP:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
			{
			CHARFORMAT2 cf;
			UINT u = 0;
			UINT u2 = 0;
			COLORREF cr;

			LoadMsgDlgFont(17, NULL, &cr);
			
			cf.cbSize = sizeof(CHARFORMAT2);
			cf.dwMask = CFM_BOLD|CFM_ITALIC|CFM_UNDERLINE|CFM_BACKCOLOR|CFM_COLOR;
			SendMessage(hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

			if(MM_FindModule(Parentdat->pszModule) && MM_FindModule(Parentdat->pszModule)->bColor)
			{
				int index = GetColorIndex(Parentdat->pszModule, cf.crTextColor);
				u = IsDlgButtonChecked(GetParent(hwnd), IDC_COLOR);

				if(index >= 0)
				{
					Parentdat->bFGSet = TRUE;
					Parentdat->iFG = index;
				}

				if(u == BST_UNCHECKED && cf.crTextColor != cr)
					CheckDlgButton(GetParent(hwnd), IDC_COLOR, BST_CHECKED);
				else if(u == BST_CHECKED && cf.crTextColor == cr)
					CheckDlgButton(GetParent(hwnd), IDC_COLOR, BST_UNCHECKED);
			}
			if(MM_FindModule(Parentdat->pszModule) && MM_FindModule(Parentdat->pszModule)->bBkgColor)
			{
				int index = GetColorIndex(Parentdat->pszModule, cf.crBackColor);
				COLORREF crB = (COLORREF)DBGetContactSettingDword(NULL, "Chat", "ColorMessageBG", GetSysColor(COLOR_WINDOW));
				u = IsDlgButtonChecked(GetParent(hwnd), IDC_BKGCOLOR);
				
				if(index >= 0)
				{
					Parentdat->bBGSet = TRUE;
					Parentdat->iBG = index;
				}		
				if(u == BST_UNCHECKED && cf.crBackColor != crB)
					CheckDlgButton(GetParent(hwnd), IDC_BKGCOLOR, BST_CHECKED);
				else if(u == BST_CHECKED && cf.crBackColor == crB)
					CheckDlgButton(GetParent(hwnd), IDC_BKGCOLOR, BST_UNCHECKED);
			}
			if(MM_FindModule(Parentdat->pszModule) && MM_FindModule(Parentdat->pszModule)->bBold)
			{
				u = IsDlgButtonChecked(GetParent(hwnd), IDC_BOLD);
				u2 = cf.dwEffects;
				u2 &= CFE_BOLD;
				if(u == BST_UNCHECKED && u2)
					CheckDlgButton(GetParent(hwnd), IDC_BOLD, BST_CHECKED);
				else if(u == BST_CHECKED && u2 == 0)
					CheckDlgButton(GetParent(hwnd), IDC_BOLD, BST_UNCHECKED);
			}
				
			if(MM_FindModule(Parentdat->pszModule) && MM_FindModule(Parentdat->pszModule)->bItalics)
			{
				u = IsDlgButtonChecked(GetParent(hwnd), IDC_ITALICS);
				u2 = cf.dwEffects;
				u2 &= CFE_ITALIC;
				if(u == BST_UNCHECKED && u2)
					CheckDlgButton(GetParent(hwnd), IDC_ITALICS, BST_CHECKED);
				else if(u == BST_CHECKED && u2 == 0)
					CheckDlgButton(GetParent(hwnd), IDC_ITALICS, BST_UNCHECKED);
			}
			if(MM_FindModule(Parentdat->pszModule) && MM_FindModule(Parentdat->pszModule)->bUnderline)
			{
				u = IsDlgButtonChecked(GetParent(hwnd), IDC_UNDERLINE);
				u2 = cf.dwEffects;
				u2 &= CFE_UNDERLINE;
				if(u == BST_UNCHECKED && u2)
					CheckDlgButton(GetParent(hwnd), IDC_UNDERLINE, BST_CHECKED);
				else if(u == BST_CHECKED && u2 == 0)
					CheckDlgButton(GetParent(hwnd), IDC_UNDERLINE, BST_UNCHECKED);
			}

            }break;

        case EM_UNSUBCLASSED:
			free(dat);
			return 0;
		default:break;
	} 

	return CallWindowProc(OldMessageProc, hwnd, msg, wParam, lParam); 

} 
static LRESULT CALLBACK NicklistSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{ 
    switch (msg) 
	{
	case WM_RBUTTONDOWN:
			PostMessage(hwnd, WM_RBUTTONUP, wParam, lParam);
			break;
	case WM_KEYDOWN:
			if ((wParam == 0x57 && GetKeyState(VK_CONTROL) & 0x8000)) // ctrl-w (close window)
			{
				PostMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
				return TRUE;
		
			}
			break;

	case WM_RBUTTONUP:
		{
			TVHITTESTINFO hti;
			CHATWINDOWDATA *parentdat =(CHATWINDOWDATA *)GetWindowLong(GetParent(hwnd),GWL_USERDATA);


			hti.pt.x = (short) LOWORD(lParam);
			hti.pt.y = (short) HIWORD(lParam);
			if(TreeView_HitTest(hwnd,&hti))
			{
				if(hti.flags&TVHT_ONITEM) 
				{
					TVITEM tvi = {0};
					tvi.mask=TVIF_PARAM|TVIF_HANDLE;
					tvi.hItem=hti.hItem;
					TreeView_GetItem(hwnd,&tvi);
					if (tvi.lParam)
					{
						HMENU hMenu = 0;
						UINT uID;
						
						ClientToScreen(hwnd, &hti.pt);
						uID = CreateGCMenu(hwnd, &hMenu, 0, hti.pt, parentdat, ((USERINFO *)tvi.lParam)->pszUID); 

						switch (uID) 
						{
						case 0:
							break;
                        case ID_MESS:
							{
								DoEventHookAsync(GetParent(hwnd), parentdat->pszID, parentdat->pszModule, GC_USER_PRIVMESS, ((USERINFO *)tvi.lParam)->pszUID, NULL, (LPARAM)NULL);
							}break;
 						default:
							DoEventHookAsync(GetParent(hwnd), parentdat->pszID, parentdat->pszModule, GC_USER_NICKLISTMENU, ((USERINFO *)tvi.lParam)->pszUID, NULL, (LPARAM)uID);
							break;

						}
						DestroyGCMenu(&hMenu, 1);
					}
				}
			}


		}break;
		default:break;
	} 

	return CallWindowProc(OldNicklistProc, hwnd, msg, wParam, lParam); 

} 

static BOOL CALLBACK FilterWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static CHATWINDOWDATA* dat = NULL;
	switch (uMsg)
	{	
	case WM_INITDIALOG:
	{	
		TVINSERTSTRUCT tvis;
		int i;

		SetWindowLong(GetDlgItem(hwndDlg,IDC_FILTERLIST),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_FILTERLIST),GWL_STYLE)|TVS_NOHSCROLL|TVS_CHECKBOXES);
		dat = (CHATWINDOWDATA *)lParam;
		tvis.hParent=TVI_ROOT;
		tvis.hInsertAfter=TVI_LAST;
		tvis.item.mask=TVIF_TEXT;
		for (i = 0; i < sizeof(filterList) / sizeof(filterList[0]); i++) {
			tvis.item.pszText=Translate(filterList[i].szDescr);
			filterList[i].hItem = TreeView_InsertItem(GetDlgItem(hwndDlg, IDC_FILTERLIST),&tvis);
		}
	}break;
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
	{
		if((HWND)lParam==GetDlgItem(hwndDlg,IDC_TEXTO))
		{
			SetTextColor((HDC)wParam,RGB(60,60,150));
			SetBkColor((HDC)wParam,GetSysColor(COLOR_WINDOW));
			return (BOOL)GetSysColorBrush(COLOR_WINDOW);
		}
	}break;
	case GC_FILTERFIX:
	{	
		int i;
		for (i = 0; i < sizeof(filterList) / sizeof(filterList[0]); i++) {
			BOOL bSet = (dat->iLogFilterFlags&filterList[i].iEvent)?TRUE:FALSE;
			TreeView_SetCheckState(GetDlgItem(hwndDlg, IDC_FILTERLIST), filterList[i].hItem, bSet);
		}
	}break;
	case WM_NOTIFY:
	{
		LPNMHDR pNmhdr = (LPNMHDR)lParam;
		switch (pNmhdr->code)
		{
		case NM_CUSTOMDRAW:
		{
			if (pNmhdr->idFrom = IDC_FILTERLIST)
			{
				LPNMTVCUSTOMDRAW pCustomDraw = (LPNMTVCUSTOMDRAW)lParam;
				switch (pCustomDraw->nmcd.dwDrawStage)
				{
					case CDDS_PREPAINT:
					SetWindowLongPtr(hwndDlg,DWL_MSGRESULT,CDRF_NOTIFYITEMDRAW); 
					return(TRUE); 

					case CDDS_ITEMPREPAINT:
						{
						switch (pCustomDraw->iLevel)
						{
							case 0:
								{
								pCustomDraw->clrTextBk = GetSysColor(COLOR_WINDOW);
								pCustomDraw->clrText = RGB(0,0,0);
								break;
								}
							default:break;
						}
						SetWindowLongPtr(hwndDlg,DWL_MSGRESULT, CDRF_NEWFONT); 
						return (TRUE); 
					}
				}
			}
		}break;
		}
	}
	case WM_ACTIVATE:
		{	
			if(LOWORD(wParam) == WA_INACTIVE)
			{
				TVITEM tvi;
				int iFlags = 0;
				int i;

				tvi.mask=TVIF_HANDLE|TVIF_STATE;
				for (i = 0; i < sizeof(filterList) / sizeof(filterList[0]); i++) {
					tvi.hItem = filterList[i].hItem;
					TreeView_GetItem(GetDlgItem(hwndDlg, IDC_FILTERLIST),&tvi);
					if(!((tvi.state&TVIS_STATEIMAGEMASK)>>12==1)) 
						iFlags |= filterList[i].iEvent;
				}
				if (iFlags&GC_EVENT_ADDSTATUS)
					iFlags |= GC_EVENT_REMOVESTATUS;
				SendMessage(GetParent(hwndDlg), GC_CHANGEFILTERFLAG, 0, (LPARAM)iFlags);
				if(IsDlgButtonChecked(GetParent(hwndDlg),IDC_SHOWNICKLIST)== BST_CHECKED)
					SendMessage(GetParent(hwndDlg), GC_REDRAWLOG, 0, 0);
				PostMessage(hwndDlg, WM_CLOSE, 0, 0);
			}
		}break;
	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		break;
		default:break;
	}
	return(FALSE);
}
static LRESULT CALLBACK ButtonSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{ 
    switch (msg) 
	{
	case WM_RBUTTONUP:
		{
			HWND hFilter = GetDlgItem(GetParent(hwnd), IDC_FILTER);
			HWND hColor = GetDlgItem(GetParent(hwnd), IDC_COLOR);
			HWND hBGColor = GetDlgItem(GetParent(hwnd), IDC_BKGCOLOR);

			if (DBGetContactSettingByte(NULL, "Chat", "RightClickFilter", 0) != 0)
			{
				if(hFilter == hwnd)
					SendMessage(GetParent(hwnd), GC_SHOWFILTERMENU, 0, 0);
				if(hColor == hwnd)
					SendMessage(GetParent(hwnd), GC_SHOWCOLORCHOOSER, 0, (LPARAM)IDC_COLOR);
				if(hBGColor == hwnd)
					SendMessage(GetParent(hwnd), GC_SHOWCOLORCHOOSER, 0, (LPARAM)IDC_BKGCOLOR);
			}
		}break;

		default:break;
	} 

	return CallWindowProc(OldFilterButtonProc, hwnd, msg, wParam, lParam); 

} 
static LRESULT CALLBACK LogSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{ 
    switch (msg) 
	{

	case WM_LBUTTONUP:
		{
			CHARRANGE sel;

			SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM) &sel);
			if(sel.cpMin != sel.cpMax)
			{
				SendMessage(hwnd, WM_COPY, 0, 0);
				sel.cpMin = sel.cpMax ;
				SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM) & sel);
			}
			PostMessage(GetDlgItem(GetParent(hwnd), IDC_MESSAGE), WM_MOUSEACTIVATE, 0, 0);
			break;


		}
	case WM_KEYDOWN:
			if (wParam == 0x57 && GetKeyState(VK_CONTROL) & 0x8000) // ctrl-w (close window)
			{
				PostMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
				return TRUE;
		
			}
			break;
	case WM_ACTIVATE:
		{
			if(LOWORD(wParam) == WA_INACTIVE)
			{
				CHARRANGE sel;
				SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM) &sel);
				if(sel.cpMin != sel.cpMax)
				{
					sel.cpMin = sel.cpMax ;
					SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM) & sel);
				}

			}
		}break;
	case WM_CHAR:
		{
			SetFocus(GetDlgItem(GetParent(hwnd), IDC_MESSAGE));
			SendMessage(GetDlgItem(GetParent(hwnd), IDC_MESSAGE), WM_CHAR, wParam, lParam);
		}break;

		default:break;
	} 

	return CallWindowProc(OldLogProc, hwnd, msg, wParam, lParam); 

} 
static int RestoreWindowPosition(HWND hwnd, HANDLE hContact, char * szModule, char * szNamePrefix, UINT showCmd)
{
	WINDOWPLACEMENT wp;
	char szSettingName[64];
	int x,y, width, height;;

	wp.length=sizeof(wp);
	GetWindowPlacement(hwnd,&wp);
	if (hContact)
	{
		wsprintf(szSettingName,"%sx",szNamePrefix);
		x=DBGetContactSettingDword(hContact,szModule,szSettingName,-1);
		wsprintf(szSettingName,"%sy",szNamePrefix);
		y=(int)DBGetContactSettingDword(hContact,szModule,szSettingName,-1);
		wsprintf(szSettingName,"%swidth",szNamePrefix);
		width=DBGetContactSettingDword(hContact,szModule,szSettingName,-1);
		wsprintf(szSettingName,"%sheight",szNamePrefix);
		height=DBGetContactSettingDword(hContact,szModule,szSettingName,-1);
	}
	else
	{
		x = g_LogOptions.iX;
		y = g_LogOptions.iY;
		width = g_LogOptions.iWidth;
		height = g_LogOptions.iHeight;

	}
	if(x==-1) 
		return 0;
	wp.rcNormalPosition.left=x;
	wp.rcNormalPosition.top=y;
	wp.rcNormalPosition.right=wp.rcNormalPosition.left+width;
	wp.rcNormalPosition.bottom=wp.rcNormalPosition.top+height;
	wp.showCmd = showCmd;
	SetWindowPlacement(hwnd,&wp);
	return 1;
}
int GetTextPixelSize(char * pszText, HFONT hFont, BOOL bWidth)
{ 
	HDC hdc;
	HFONT hOldFont;
	RECT rc = {0};
	int i;

	if (!pszText || !hFont)
		return 0;
	hdc=GetDC(NULL);
	hOldFont = SelectObject(hdc, hFont);
	i = DrawText(hdc, pszText , -1, &rc, DT_CALCRECT);
//GetTextExtentPoint32(hdc,pszText,lstrlen(pszText),&textSize);
	SelectObject(hdc, hOldFont);
	ReleaseDC(NULL,hdc);
	return bWidth?rc.right-rc.left:i;
}

BOOL CALLBACK RoomWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CHATWINDOWDATA *dat;
	dat=(CHATWINDOWDATA *)GetWindowLong(hwndDlg,GWL_USERDATA);
	switch (uMsg)
	{	
		case WM_INITDIALOG:
		{	
			NEWCHATWINDOWLPARAM *newData=(NEWCHATWINDOWLPARAM*)lParam;
			MODULE *min;
			int mask;

			TranslateDialogDefault(hwndDlg);

			dat=(CHATWINDOWDATA*)malloc(sizeof(CHATWINDOWDATA));
			SetWindowLong(hwndDlg,GWL_USERDATA,(LONG)dat);

			SendMessage(hwndDlg, WM_SETREDRAW, FALSE, 0);
			OldSplitterProc=(WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_SPLITTERX),GWL_WNDPROC,(LONG)SplitterSubclassProc);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_SPLITTERY),GWL_WNDPROC,(LONG)SplitterSubclassProc);
			OldNicklistProc=(WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_NICKLIST),GWL_WNDPROC,(LONG)NicklistSubclassProc);
			OldLogProc=(WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_LOG),GWL_WNDPROC,(LONG)LogSubclassProc);
			OldFilterButtonProc=(WNDPROC)SetWindowLong(GetDlgItem(hwndDlg,IDC_FILTER),GWL_WNDPROC,(LONG)ButtonSubclassProc);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_COLOR),GWL_WNDPROC,(LONG)ButtonSubclassProc);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_BKGCOLOR),GWL_WNDPROC,(LONG)ButtonSubclassProc);
			OldMessageProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_WNDPROC,(LONG)MessageSubclassProc); 
            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SUBCLASSED, 0, 0);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_NICKLIST),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_NICKLIST),GWL_STYLE)|TVS_NOHSCROLL);
			
			SendMessage(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)hIcons[6]); // Tell the dialog to use it
			
			dat->pszID = malloc(lstrlen(newData->pszID)+1);
			lstrcpy(dat->pszID, newData->pszID);
			dat->pszModule = malloc(lstrlen(newData->pszModule)+1);
			lstrcpy(dat->pszModule, newData->pszModule);
			dat->pszName = malloc(lstrlen(newData->pszName)+1);
			lstrcpy(dat->pszName, newData->pszName);
			min = MM_FindModule(dat->pszModule);
			dat->iSplitterX = g_LogOptions.iSplitterX;
			dat->iSplitterY = g_LogOptions.iSplitterY;
			dat->iLogFilterFlags = (int)DBGetContactSettingDword(NULL, "Chat", "FilterFlags", 0xFFFF);
			dat->iType = newData->iType;
			dat->iEventCount = 0;
			dat->nFlash = 0;
			dat->pStatusList = NULL;
			dat->pUserList = NULL;
			dat->pEventListStart = NULL;
			dat->pEventListEnd = NULL;
			dat->pszTopic = NULL;
			dat->pMe = NULL;
            dat->windowWasCascaded = 0;
			dat->nUsersInNicklist = 0;
			dat->ItemData = 0;
			dat->LastTime = 0;
			dat->bBGSet = FALSE;
			dat->bFGSet = FALSE;
			dat->hwndStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP| SBT_TOOLTIPS , 0, 0, 0, 0, hwndDlg, NULL, g_hInst, NULL);
			dat->bFilterEnabled = DBGetContactSettingByte(NULL, "Chat", "FilterEnabled", 0);

			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_AUTOURLDETECT, 1, 0);
			mask = (int)SendDlgItemMessage(hwndDlg, IDC_LOG, EM_GETEVENTMASK, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETEVENTMASK, 0, mask | ENM_LINK | ENM_MOUSEEVENTS);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_LIMITTEXT, (WPARAM)sizeof(TCHAR)*0x7FFFFFFF, 0);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETOLECALLBACK, 0, (LPARAM) & reOleCallback);
			{
				char szTemp[256];
				if(dat->iType == GCW_SERVER)
					_snprintf(szTemp, sizeof(szTemp), "Server: %s", newData->pszName);
				else
					_snprintf(szTemp, sizeof(szTemp), "%s", newData->pszName);

				dat->hContact = CList_AddRoom(newData->pszModule, newData->pszID, szTemp, newData->iType); 
			}

			DBWriteContactSettingString(dat->hContact, dat->pszModule , "Topic", "");
			DBWriteContactSettingString(dat->hContact, dat->pszModule, "StatusBar", "");
//			DBWriteContactSettingDword(dat->hContact, dat->pszModule, "Count", 0);

			{ //set statusbar
				int x = 12;
				int iStatusbarParts[2];
				char *pszDispName = MM_FindModule(dat->pszModule)->pszModDispName;

				x += GetTextPixelSize(pszDispName, (HFONT)SendMessage(dat->hwndStatus,WM_GETFONT,0,0), TRUE);
				x+=GetSystemMetrics(SM_CXSMICON);
				iStatusbarParts[0] = x; iStatusbarParts[1] = -1;
				SendMessage(dat->hwndStatus,SB_SETMINHEIGHT,GetSystemMetrics(SM_CYSMICON),0);
				SendMessage(dat->hwndStatus,SB_SETPARTS,2 ,(LPARAM)&iStatusbarParts);
				SendMessage(dat->hwndStatus, SB_SETTEXT,0,(LPARAM)pszDispName);
				SendMessage(dat->hwndStatus, SB_SETICON, 0,(LPARAM)MM_FindModule(dat->pszModule)->hOfflineIcon);
				if(newData->pszStatusbarText != 0)
				{
					dat->pszStatusbarText = malloc(lstrlen(newData->pszStatusbarText)+1);
					lstrcpy(dat->pszStatusbarText, newData->pszStatusbarText);
					SendMessage(dat->hwndStatus, SB_SETTEXT,1,(LPARAM)dat->pszStatusbarText);
					SendMessage(dat->hwndStatus, SB_SETTIPTEXT,1,(LPARAM)dat->pszStatusbarText);
					DBWriteContactSettingString(dat->hContact, dat->pszModule, "StatusBar", dat->pszStatusbarText);
				}
				else
					dat->pszStatusbarText = NULL;
			}

			InitButtons(hwndDlg, dat);
		
			SendDlgItemMessage(hwndDlg,IDC_NICKLIST, CCM_SETVERSION,(WPARAM)5,0);
			TreeView_SetImageList(GetDlgItem(hwndDlg, IDC_NICKLIST), hImageList, TVSIL_STATE);
			if(dat->iType == GCW_SERVER || dat->iType == GCW_PRIVMESS)
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWNICKLIST), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_FILTER), FALSE);
				if(dat->iType == GCW_SERVER)
					EnableWindow(GetDlgItem(hwndDlg, IDC_CHANMGR), FALSE);
			}
			
			{
				int savePerContact = DBGetContactSettingByte(NULL, "Chat", "SavePosition", 0);
				if (!RestoreWindowPosition(hwndDlg, dat->hContact, "Chat", "room", SW_HIDE)) 
				{
					if (savePerContact) 
					{
						if (!RestoreWindowPosition(hwndDlg, NULL, "Chat", "room", SW_HIDE))
							SetWindowPos(hwndDlg, 0, 0, 0, 550, 400, SWP_NOZORDER | SWP_NOMOVE|SWP_HIDEWINDOW);
					}
					else
						SetWindowPos(hwndDlg, 0, 0, 0, 550, 400, SWP_NOZORDER | SWP_NOMOVE|SWP_HIDEWINDOW);

					if (DBGetContactSettingByte(NULL, "Chat", "CascadeWindows", 1))
						WM_BroadcastMessage(NULL, GC_CASCADENEWWINDOW, (WPARAM) hwndDlg, (LPARAM) & dat->windowWasCascaded, FALSE);
				}
				else if (!savePerContact && DBGetContactSettingByte(NULL, "Chat", "CascadeWindows", 1))
					WM_BroadcastMessage(NULL, GC_CASCADENEWWINDOW, (WPARAM) hwndDlg, (LPARAM) & dat->windowWasCascaded, FALSE);
			}
			
			if(SmileyAddInstalled)
				EnableWindow(GetDlgItem(hwndDlg, IDC_SMILEY), TRUE);
		
			SendMessage(GetDlgItem(hwndDlg, IDC_LOG), EM_HIDESELECTION, TRUE, 0);

			SendMessage(hwndDlg, GC_UPDATEWINDOW, 0, 0);
			SendMessage(hwndDlg, GC_SETWNDPROPS, 0, 0);

		
		} break;
		case GC_SETWNDPROPS:
		{
			BOOL bFlag = FALSE;

			if(IsWindowVisible(hwndDlg))
			{
				SendMessage(hwndDlg, WM_SETREDRAW, FALSE, 0);
				bFlag = TRUE;
			}

			LoadGlobalSettings();
			dat->nFlashMax = DBGetContactSettingByte(NULL, "Chat", "FlashMax", 4);
			SendMessage(GetDlgItem(hwndDlg, IDC_LOG), EM_SETBKGNDCOLOR , 0, g_LogOptions.crLogBackground);

			{ //nicklist
				int Height1, Height2;
				LOGFONT lf;
				HFONT hFont;
				LONG lStyle = GetWindowLong(GetDlgItem(hwndDlg, IDC_NICKLIST),GWL_STYLE);

				Height1 = (int)DBGetContactSettingByte(NULL, "Chat", "NicklistRowDist", 16);
				LoadMsgDlgFont(18, &lf, NULL);
				hFont =CreateFontIndirect(&lf);
				Height2 = GetTextPixelSize("ABC123gq", hFont, FALSE);
				if (Height2 > 22)
					Height2 = (int)(Height2*(10+ Height2-22))/10;
				if(Height1 < Height2)
					Height1 = Height2;
				DeleteObject(hFont);
				LoadMsgDlgFont(19, &lf, NULL);
				hFont =CreateFontIndirect(&lf);
				Height2 = GetTextPixelSize("ABC123gq", hFont, FALSE);
				if (Height2 > 22)
					Height2 = (int)(Height2*(10+ Height2-22))/10;
				if(Height1 < Height2)
					Height1 = Height2;
				DeleteObject(hFont);
				TreeView_SetItemHeight(GetDlgItem(hwndDlg, IDC_NICKLIST), Height1);

				if(DBGetContactSettingByte(NULL, "Chat", "ShowLines", 1))
					lStyle |= TVS_HASLINES;
				else
					lStyle &= ~TVS_HASLINES;
				SetWindowLong(GetDlgItem(hwndDlg, IDC_NICKLIST),GWL_STYLE, lStyle);
				TreeView_SetLineColor(GetDlgItem(hwndDlg, IDC_NICKLIST), DBGetContactSettingDword(NULL, "Chat", "ColorNicklistLines", GetSysColor(COLOR_INACTIVEBORDER)));
				TreeView_SetBkColor(GetDlgItem(hwndDlg, IDC_NICKLIST), g_LogOptions.crUserListBGColor);
				TreeView_SetIndent(GetDlgItem(hwndDlg, IDC_NICKLIST), DBGetContactSettingByte(NULL, "Chat", "NicklistIndent", 18));
				if(dat->iType == GCW_CHATROOM)
					ShowWindow(GetDlgItem(hwndDlg, IDC_NICKLIST), DBGetContactSettingByte(NULL, "Chat", "ShowNicklist", 1)==1?SW_SHOW:SW_HIDE);
				else
					ShowWindow(GetDlgItem(hwndDlg, IDC_NICKLIST), SW_HIDE);
			}
			{ //messagebox
				COLORREF	crFore;

				CHARFORMAT2 cf;
				LoadMsgDlgFont(17, NULL, &crFore);
				cf.cbSize = sizeof(CHARFORMAT2);
				cf.dwMask = CFM_COLOR|CFM_BOLD|CFM_UNDERLINE|CFM_BACKCOLOR;
				cf.dwEffects = 0;
				cf.crTextColor = crFore;
				cf.crBackColor = (COLORREF)DBGetContactSettingDword(NULL, "Chat", "ColorMessageBG", GetSysColor(COLOR_WINDOW));
				SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_SETBKGNDCOLOR , 0, DBGetContactSettingDword(NULL, "Chat", "ColorMessageBG", GetSysColor(COLOR_WINDOW)));
				SendDlgItemMessage(hwndDlg, IDC_MESSAGE, WM_SETFONT, (WPARAM) g_LogOptions.MessageBoxFont, MAKELPARAM(TRUE, 0));
				SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, (WPARAM)SCF_ALL , (LPARAM)&cf);
			}

			SendMessage(hwndDlg, GC_REDRAWLOG, 0, 0);
			if(bFlag)
				SendMessage(hwndDlg, WM_SETREDRAW, TRUE, 0);
			PostMessage(hwndDlg, WM_SIZE, 0, 0);
			InvalidateRect(hwndDlg, NULL, TRUE);

		}
		case GC_REDRAWLOG:
		{	
			dat->LastTime = 0;
			if(dat->pEventListEnd)
				Log_StreamInEvent(hwndDlg, dat->pEventListEnd, dat, TRUE); 
		} break;

		case GC_UPDATEWINDOW:
		{	
			char szTemp [100];
			switch(dat->iType)
			{
			case GCW_CHATROOM:
				_snprintf(szTemp, sizeof(szTemp), dat->nUsersInNicklist ==1?Translate("%s: Chat Room (%u user)"):Translate("%s: Chat Room (%u users)"), dat->pszName, dat->nUsersInNicklist);
//				removed due to excessive disk writes on busy channels
//				DBWriteContactSettingDword(dat->hContact, dat->pszModule , "Count", dat->nUsersInNicklist);
				break;
			case GCW_PRIVMESS:
				_snprintf(szTemp, sizeof(szTemp), dat->nUsersInNicklist ==1?Translate("%s: Message Session"):Translate("%s: Message Session (%u users)"), dat->pszName, dat->nUsersInNicklist);
				break;
			case GCW_SERVER:
				_snprintf(szTemp, sizeof(szTemp), "%s: Server", dat->pszName);
				break;
			default:break;
			}
			SetWindowText(hwndDlg, szTemp);
		} break;
		case GC_UPDATENICKLIST:
		{	
			RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE|RDW_ALLCHILDREN);
		} break;

		case WM_SIZE:
		{	
			RECT rc;
			UTILRESIZEDIALOG urd;

			if(IsIconic(hwndDlg)) break;
			if(!IsWindowVisible(hwndDlg)) break;
			SendMessage(dat->hwndStatus, WM_SIZE, 0, 0);
			ZeroMemory(&urd,sizeof(urd));
			urd.cbSize=sizeof(urd);
			urd.hInstance=g_hInst;
			urd.hwndDlg=hwndDlg;
			urd.lParam=(LPARAM)dat;
			urd.lpTemplate=MAKEINTRESOURCEA(IDD_CHANNEL);
			urd.pfnResizer=RoomWndResize;
			if(IsWindowVisible(GetDlgItem(hwndDlg, IDC_NICKLIST)))
			{
				if(IsDlgButtonChecked(hwndDlg,IDC_SHOWNICKLIST)!= BST_CHECKED)
					CheckDlgButton(hwndDlg, IDC_SHOWNICKLIST,  BST_CHECKED);
			}
			else
				if(IsDlgButtonChecked(hwndDlg,IDC_SHOWNICKLIST)== BST_CHECKED)
					CheckDlgButton(hwndDlg, IDC_SHOWNICKLIST,  BST_UNCHECKED);

			CallService(MS_UTILS_RESIZEDIALOG,0,(LPARAM)&urd);
			
			RedrawWindow(GetDlgItem(hwndDlg,IDOK), NULL, NULL, RDW_INVALIDATE);
			RedrawWindow(GetDlgItem(hwndDlg,IDC_MESSAGE), NULL, NULL, RDW_INVALIDATE);
			GetClientRect(hwndDlg, &rc);
			rc.top = 0; rc.right -= 60; rc.bottom = 28; rc.left= 105;
			if(g_LogOptions.bShowName && GetTextPixelSize(dat->pszName, g_LogOptions.NameFont, TRUE) >80)
				InvalidateRect(hwndDlg, &rc, TRUE);
			SendMessage(hwndDlg,GC_SAVEWNDPOS,0,1);

		} break;
		case GC_EVENT_ADDGROUP + WM_USER+500:
			{
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			TVINSERTSTRUCT tvis;
			TVITEM tvi = {0};
			HTREEITEM hItem;

			tvi.mask= TVIF_TEXT|TVIF_STATE|TVIF_PARAM;
			tvi.pszText = nlu->pszStatus;
			tvi.lParam = 0;
			tvi.state = TVIS_EXPANDED|INDEXTOSTATEIMAGEMASK(1);
			tvi.stateMask = TVIS_EXPANDED|TVIS_STATEIMAGEMASK ;
			tvis.hInsertAfter = TVI_LAST;
			tvis.hParent = NULL;
			tvis.item = tvi;
			hItem = TreeView_InsertItem(GetDlgItem(hwndDlg, IDC_NICKLIST), &tvis);
			SM_AddStatus(&dat->pStatusList, nlu->pszStatus, hItem);
			return TRUE;
			}break;

		case GC_EVENT_JOIN + WM_USER+500:
			{
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			TVINSERTSTRUCT tvis;
			TVITEM tvi = {0};
			HTREEITEM hItem = 0;
			HTREEITEM h;
			USERINFO * uin = NULL;

			if(dat->pStatusList == 0)
				return 0;			
			if (nlu->pszStatus)
				hItem = SM_FindTVGroup(dat->pStatusList, nlu->pszStatus);
			if(hItem == 0 && dat->pStatusList)
				hItem = dat->pStatusList->hItem;
			tvi.mask= TVIF_TEXT;
			tvi.pszText = nlu->pszName;
			tvis.hInsertAfter = TVI_SORT;
			tvis.hParent = hItem;
			tvis.item = tvi;
			h =TreeView_InsertItem(GetDlgItem(hwndDlg, IDC_NICKLIST), &tvis);
			uin = UM_AddUser(dat->pStatusList, &dat->pUserList, nlu->pszName, nlu->pszUID, nlu->pszStatus, h);
			tvi.mask = TVIF_PARAM|TVIF_HANDLE;
			tvi.hItem = h;
			tvi.lParam = (LPARAM)uin;
			TreeView_SetItem(GetDlgItem(hwndDlg, IDC_NICKLIST), &tvi);
			dat->nUsersInNicklist += 1;
			SendMessage(hwndDlg, GC_UPDATEWINDOW, 0, 0);
			if(nlu->bIsMe)
			{
				dat->pMe = uin;
			}

			if( nlu->bAddLog) {
				if(!LM_AddEvent(&dat->pEventListStart, &dat->pEventListEnd, nlu->pszName, uMsg, nlu->pszText, nlu->pszStatus, nlu->pszUserInfo, nlu->bIsMe, FALSE, nlu->time, &dat->iEventCount, g_LogOptions.iEventLimit))
					PostMessage(hwndDlg, GC_REDRAWLOG, 0, 0);
				Log_StreamInEvent(hwndDlg, dat->pEventListStart, dat, FALSE); 
				SkinPlaySound("ChatJoin");
				DoPopupAndTrayIcon(hwndDlg, uMsg-WM_USER-500, dat, nlu);
			}
			return TRUE;
			}break;

		case GC_EVENT_PART + WM_USER+500:
		case GC_EVENT_QUIT + WM_USER+500:
		case GC_EVENT_KICK + WM_USER+500:
			{
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			USERINFO *uin;

			if(nlu->pszUID)
				uin = UM_FindUser(dat->pUserList, nlu->pszUID);
			else
				uin = dat->pMe;
			if(uin)
			{
				TreeView_DeleteItem(GetDlgItem(hwndDlg, IDC_NICKLIST), uin->hItem);
				UM_RemoveUser(&dat->pUserList, nlu->pszUID);
				dat->nUsersInNicklist -= 1;
				SendMessage(hwndDlg, GC_UPDATEWINDOW, 0, 0);
				if( nlu->bAddLog) {
					if(uMsg == GC_EVENT_PART + WM_USER+500)
						SkinPlaySound("ChatPart");
					else if(uMsg == GC_EVENT_QUIT + WM_USER+500)
						SkinPlaySound("ChatQuit");
					else
						SkinPlaySound("ChatKick");
					if(!LM_AddEvent(&dat->pEventListStart, &dat->pEventListEnd, nlu->pszName, uMsg, nlu->pszText, nlu->pszStatus, nlu->pszUserInfo, nlu->bIsMe, FALSE, nlu->time, &dat->iEventCount, g_LogOptions.iEventLimit))
						PostMessage(hwndDlg, GC_REDRAWLOG, 0, 0);
					Log_StreamInEvent(hwndDlg, dat->pEventListStart, dat, FALSE); 
					DoPopupAndTrayIcon(hwndDlg, uMsg-WM_USER-500, dat, nlu);
				}
				return TRUE;
			}
			}break;
		case GC_NICKLISTREINIT:
			{
				CList_SetOffline(dat->hContact, FALSE);
				SendMessage(dat->hwndStatus, SB_SETICON, 0,(LPARAM)MM_FindModule(dat->pszModule)->hOfflineIcon);
				TreeView_DeleteAllItems(GetDlgItem(hwndDlg, IDC_NICKLIST));
				UM_RemoveAll(&dat->pUserList);
				SM_RemoveAll(&dat->pStatusList);
				dat->pMe = NULL;
				dat->nUsersInNicklist = 0;
				SendMessage(hwndDlg, GC_UPDATEWINDOW, 0, 0);
				return TRUE;
			}break;

		case GC_EVENT_TOPIC + WM_USER+500:
			{
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			dat->pszTopic = realloc(dat->pszTopic, lstrlen(nlu->pszText)+1);
			lstrcpy(dat->pszTopic, nlu->pszText);
			if( nlu->bAddLog) {
				if(!LM_AddEvent(&dat->pEventListStart, &dat->pEventListEnd, nlu->pszName, uMsg, nlu->pszText, nlu->pszStatus, nlu->pszUserInfo, nlu->bIsMe, FALSE, nlu->time, &dat->iEventCount, g_LogOptions.iEventLimit))
					PostMessage(hwndDlg, GC_REDRAWLOG, 0, 0);
				Log_StreamInEvent(hwndDlg, dat->pEventListStart, dat, FALSE); 
				SkinPlaySound("ChatTopic");
				DoPopupAndTrayIcon(hwndDlg, uMsg-WM_USER-500, dat, nlu);
				DBWriteContactSettingString(dat->hContact, dat->pszModule , "Topic", dat->pszTopic);
			}
			return TRUE;
			}break;

		case GC_EVENT_NOTICE + WM_USER+500:
			{
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			if( nlu->bAddLog) {
				if(!LM_AddEvent(&dat->pEventListStart, &dat->pEventListEnd, nlu->pszName, uMsg, nlu->pszText, nlu->pszStatus, nlu->pszUserInfo, nlu->bIsMe, FALSE, nlu->time, &dat->iEventCount, g_LogOptions.iEventLimit))
					PostMessage(hwndDlg, GC_REDRAWLOG, 0, 0);
				Log_StreamInEvent(hwndDlg, dat->pEventListStart, dat, FALSE); 
				SkinPlaySound("ChatNotice");
				DoPopupAndTrayIcon(hwndDlg, uMsg-WM_USER-500, dat, nlu);
			}
			return TRUE;
			}break;
		case GC_EVENT_MESSAGE + WM_USER+500:
		case GC_EVENT_ACTION + WM_USER+500:
			{
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			BOOL bHighl = FALSE;
			if( nlu->bAddLog) 
			{
				if(!nlu->bIsMe)
					bHighl = IsHighlighted(dat, nlu->pszText);
				if(!LM_AddEvent(&dat->pEventListStart, &dat->pEventListEnd, nlu->pszName, uMsg, nlu->pszText, nlu->pszStatus, nlu->pszUserInfo, nlu->bIsMe, bHighl, nlu->time, &dat->iEventCount, g_LogOptions.iEventLimit))
					PostMessage(hwndDlg, GC_REDRAWLOG, 0, 0);
				Log_StreamInEvent(hwndDlg, dat->pEventListStart, dat, FALSE); 
				if(!nlu->bIsMe)
				{
					if(bHighl)
						SkinPlaySound("ChatHighlight");

					if(GetActiveWindow() != hwndDlg || GetForegroundWindow() != hwndDlg)
					{
						if(uMsg == GC_EVENT_MESSAGE + WM_USER+500)
						{

							if(g_LogOptions.FlashWindow)
								SetTimer(hwndDlg, TIMERID_FLASHWND, 900, NULL);
							if(DBGetContactSettingWord(dat->hContact, dat->pszModule,"ApparentMode",(WORD) 0) != 40071)
								DBWriteContactSettingWord(dat->hContact, dat->pszModule,"ApparentMode",(LPARAM)(WORD) 40071);
						}
						if(bHighl)
						{
							char szTemp[256];
							_snprintf(szTemp, 256, Translate("%s wants your attention in room %s"), nlu->pszName, dat->pszName);
							SendMessage(hwndDlg, GC_HIGHLIGHT, 0, (LPARAM)&szTemp);
						}
						else
							SkinPlaySound("ChatMessage");

					}
				}
				else
					if(!nlu->bIsMe)
						SkinPlaySound("ChatAction");
				uMsg -= WM_USER;
				uMsg -= 500;
				if (bHighl)
					uMsg |= GC_EVENT_HIGHLIGHT;
				DoPopupAndTrayIcon(hwndDlg, uMsg, dat, nlu);
			}
			return TRUE;
			}break;
		case GC_EVENT_INFORMATION + WM_USER+500:
			{
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			if( nlu->bAddLog) 
			{
				if(!LM_AddEvent(&dat->pEventListStart, &dat->pEventListEnd, nlu->pszName, uMsg, nlu->pszText, nlu->pszStatus, nlu->pszUserInfo, nlu->bIsMe, FALSE, nlu->time, &dat->iEventCount, g_LogOptions.iEventLimit))
					PostMessage(hwndDlg, GC_REDRAWLOG, 0, 0);
				Log_StreamInEvent(hwndDlg, dat->pEventListStart, dat, FALSE); 
				DoPopupAndTrayIcon(hwndDlg, uMsg-WM_USER-500, dat, nlu);
			}
			return TRUE;
			}break;
		case GC_EVENT_CONTROL + WM_USER+500:
			{
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			switch(wParam) 
			{
			case WINDOW_INITDONE:
				if(DBGetContactSettingByte(NULL, "Chat", "PopupOnJoin", 0)!=0)
				{
					nlu->dwItemData = 1; // stupid fix
					return TRUE;
				}
				// fall through
			case WINDOW_VISIBLE:
				{
					BOOL bRedrawFlag = FALSE;
					SetActiveChatWindow(dat->pszID, dat->pszModule);
					if(!IsWindowVisible(hwndDlg))
						bRedrawFlag = TRUE;
					SendMessage(hwndDlg, WM_SETREDRAW, TRUE, 0);
					if (IsIconic(hwndDlg))
						ShowWindow(hwndDlg, SW_NORMAL);
					SendMessage(hwndDlg, WM_SIZE, 0, 0);
					ShowWindow(hwndDlg, SW_SHOW);
					if(bRedrawFlag)
					{
						InvalidateRect(hwndDlg, NULL, TRUE);
						SendMessage(hwndDlg, GC_REDRAWLOG, 0, 0);
					}
					SetWindowPos(hwndDlg, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE| SWP_NOSIZE | SWP_FRAMECHANGED);
					SetActiveWindow(hwndDlg);

				}
				nlu->dwItemData = 1; // stupid fix
				return TRUE;
			case WINDOW_HIDDEN:
				ShowWindow(hwndDlg, SW_HIDE); 
				SendMessage(hwndDlg, WM_SETREDRAW, FALSE, 0);
				nlu->dwItemData = 1; // stupid fix
				return TRUE;
			case WINDOW_MAXIMIZE:
				ShowWindow(hwndDlg, SW_MAXIMIZE); 
				nlu->dwItemData = 1; // stupid fix
				return TRUE;
			case WINDOW_MINIMIZE:
				ShowWindow(hwndDlg, SW_MINIMIZE); 
				nlu->dwItemData = 1; // stupid fix
				return TRUE;
			case WINDOW_CLEARLOG:
				SetDlgItemText(hwndDlg, IDC_LOG, "");
				LM_RemoveAll(&dat->pEventListStart, &dat->pEventListEnd);
				dat->iEventCount = 0;
				dat->LastTime = 0;
				nlu->dwItemData = 1; // stupid fix
				return TRUE;
			case WINDOW_TERMINATE:
				if(dat->iType == GCW_CHATROOM)
					PostMessage(hwndDlg, GC_CLOSEWINDOW, 0, 0);
				nlu->dwItemData = 1; // stupid fix
				return TRUE;
			case WINDOW_OFFLINE:
					SendMessage(hwndDlg, GC_NICKLISTREINIT, 0, 0);
				nlu->dwItemData = 1; // stupid fix
				return TRUE;
			case WINDOW_ONLINE:
				SendMessage(dat->hwndStatus, SB_SETICON, 0,(LPARAM)MM_FindModule(dat->pszModule)->hOnlineIcon);
				DBWriteContactSettingWord(dat->hContact, dat->pszModule, "Status", ID_STATUS_ONLINE);
				nlu->dwItemData = 1; // stupid fix
				return TRUE;
			default:break;
			}

			return TRUE;
			}break;

		case GC_EVENT_SETSBTEXT + WM_USER+500:
			{	
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			dat->pszStatusbarText = realloc(dat->pszStatusbarText, lstrlen(nlu->pszText)+1);
			lstrcpy(dat->pszStatusbarText, RemoveFormatting(nlu->pszText));
			SendMessage(dat->hwndStatus, SB_SETTEXT, 1, (LPARAM)dat->pszStatusbarText);
			SendMessage(dat->hwndStatus, SB_SETTIPTEXT, 1, (LPARAM)dat->pszStatusbarText);
			DBWriteContactSettingString(dat->hContact, dat->pszModule, "StatusBar", dat->pszStatusbarText);
			return TRUE;
			} break;
		case GC_EVENT_GETITEMDATA + WM_USER+500:
			{
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			nlu->dwItemData = dat->ItemData;
			return TRUE;
			} break;
		case GC_EVENT_SETITEMDATA + WM_USER+500:
			{	
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			dat->ItemData = nlu->dwItemData;
			return TRUE;
			} break;
		case GC_EVENT_SENDMESSAGE + WM_USER+500:
			{	
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			if(dat->iType != GCW_SERVER)
				DoEventHookAsync(hwndDlg, dat->pszID, dat->pszModule, GC_USER_MESSAGE, NULL, nlu->pszText, (LPARAM)NULL);
			return TRUE;
			} break;


		case GC_EVENT_NICK + WM_USER+500:
			{
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			TVITEM tvi ={0};
			USERINFO * uin;

			uin = UM_FindUser(dat->pUserList, nlu->pszUID);
			if (uin)
			{ 
				HTREEITEM hParent;
				HWND hwnd=GetDlgItem(hwndDlg, IDC_NICKLIST);

				tvi.mask = TVIF_TEXT;
				tvi.pszText = nlu->pszText;
				tvi.hItem = uin->hItem;
				TreeView_SetItem(hwnd, &tvi);
				hParent = TreeView_GetParent(hwnd, uin->hItem);
				TreeView_SortChildren(hwnd, hParent, FALSE);
				uin->pszNick = realloc(uin->pszNick, lstrlen(nlu->pszText)+1);
				lstrcpy(uin->pszNick, nlu->pszText);
				if( nlu->bAddLog) {
					if(!LM_AddEvent(&dat->pEventListStart, &dat->pEventListEnd, nlu->pszName, uMsg, nlu->pszText, nlu->pszStatus, nlu->pszUserInfo, nlu->bIsMe, FALSE, nlu->time, &dat->iEventCount, g_LogOptions.iEventLimit))
						PostMessage(hwndDlg, GC_REDRAWLOG, 0, 0);
					Log_StreamInEvent(hwndDlg, dat->pEventListStart, dat, FALSE); 
					SkinPlaySound("ChatNick");
					DoPopupAndTrayIcon(hwndDlg, uMsg-WM_USER-500, dat, nlu);
				}
				return TRUE;
			}
			}break;
		case GC_EVENT_CHID + WM_USER+500:
			{
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			USERINFO * uin = UM_FindUser(dat->pUserList, nlu->pszUID);
			if(uin)
			{
				uin->pszUID = realloc(uin->pszUID, lstrlen(nlu->pszText)+1);
				lstrcpy(uin->pszUID, nlu->pszText);
				return TRUE;
			}
			
			}break;
		case GC_EVENT_CHWINNAME + WM_USER+500:
			{
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			char szTemp[256];
			dat->pszName = realloc(dat->pszName, lstrlen(nlu->pszText)+1);
			lstrcpyn(dat->pszName, nlu->pszText, lstrlen(nlu->pszText)+1);
			if(dat->iType == GCW_SERVER)
				_snprintf(szTemp, sizeof(szTemp), "Server: %s", nlu->pszText);
			else
				_snprintf(szTemp, sizeof(szTemp), "%s", nlu->pszText);

			DBWriteContactSettingString(dat->hContact,dat->pszModule , "Nick", szTemp);
			SendMessage(hwndDlg, GC_UPDATEWINDOW, 0, 0);
			InvalidateRect(hwndDlg, NULL, TRUE);
			return TRUE;
			}break;
		case GC_EVENT_ACK + WM_USER+500:
			{
				SendDlgItemMessage(hwndDlg,IDC_MESSAGE,EM_SETREADONLY,FALSE,0);
				SendDlgItemMessage(hwndDlg,IDC_MESSAGE,WM_SETTEXT,0, (LPARAM)"");
			return TRUE;
			}break;
		case GC_EVENT_ADDSTATUS + WM_USER+500:
		case GC_EVENT_REMOVESTATUS + WM_USER+500:
			{
			NEWEVENTLPARAM *nlu = (NEWEVENTLPARAM *)lParam;
			TVINSERTSTRUCT tvis;
			TVITEM tvi = {0};
			WORD wOldStatus ;
			WORD wNewStatus;
			char * pszNewStatus;
			USERINFO * uin = UM_FindUser(dat->pUserList, nlu->pszUID);
			
			if(uin)
			{
				wNewStatus = SM_StringToWord(dat->pStatusList, nlu->pszStatus);
				wOldStatus = uin->Status;
				if(uMsg == GC_EVENT_ADDSTATUS + WM_USER+500)
					wNewStatus |= wOldStatus;
				else
				{
					wNewStatus ^= 0xFFFF;
					wNewStatus &= wOldStatus;
					if(wNewStatus == 0)
						wNewStatus = SM_StringToWord(dat->pStatusList, "");
				}
				uin->Status = wNewStatus;
				pszNewStatus = SM_WordToString(dat->pStatusList, wNewStatus);
				TreeView_DeleteItem(GetDlgItem(hwndDlg, IDC_NICKLIST), uin->hItem);
				tvi.mask = TVIF_TEXT|TVIF_PARAM;
				tvi.lParam = (LPARAM)uin;
				tvi.pszText = uin->pszNick;
				tvis.hInsertAfter = TVI_SORT;
				tvis.hParent = SM_FindTVGroup(dat->pStatusList, pszNewStatus);
				tvis.item = tvi;
				uin->hItem =TreeView_InsertItem(GetDlgItem(hwndDlg, IDC_NICKLIST), &tvis);
				if( nlu->bAddLog) {
					if(!LM_AddEvent(&dat->pEventListStart, &dat->pEventListEnd, nlu->pszName, uMsg, nlu->pszText, nlu->pszStatus, nlu->pszUserInfo, nlu->bIsMe, FALSE, nlu->time, &dat->iEventCount, g_LogOptions.iEventLimit))
						PostMessage(hwndDlg, GC_REDRAWLOG, 0, 0);
					Log_StreamInEvent(hwndDlg, dat->pEventListStart, dat, FALSE); 
					SkinPlaySound("ChatMode");
					DoPopupAndTrayIcon(hwndDlg, uMsg-WM_USER-500, dat, nlu);
				}
				return TRUE;
			}
			}break;
		case GC_HIGHLIGHT:
			if(DBGetContactSettingByte(NULL, "Chat", "FlashWindowHighlight", 0) != 0)
				SetTimer(hwndDlg, TIMERID_FLASHWND, 900, NULL);
			break;

		case GC_SPLITTERMOVED:
		{	POINT pt;
			RECT rc;
			RECT rcLog;
			BOOL bFormat = IsWindowVisible(GetDlgItem(hwndDlg,IDC_SMILEY));

			GetWindowRect(GetDlgItem(hwndDlg,IDC_LOG),&rcLog);
			if((HWND)lParam==GetDlgItem(hwndDlg,IDC_SPLITTERX)) {
				int oldSplitterX;
				GetClientRect(hwndDlg,&rc);
				pt.x=wParam; pt.y=0;
				ScreenToClient(hwndDlg,&pt);

				oldSplitterX=dat->iSplitterX;
				dat->iSplitterX=rc.right-pt.x+1;
				if(dat->iSplitterX < 35) 
					dat->iSplitterX=35;
				if(dat->iSplitterX > rc.right-rc.left-35) 
					dat->iSplitterX = rc.right-rc.left-35;
				g_LogOptions.iSplitterX = dat->iSplitterX;
			}
			else if((HWND)lParam==GetDlgItem(hwndDlg,IDC_SPLITTERY)) {
				int oldSplitterY;
				GetClientRect(hwndDlg,&rc);
				pt.x=0; pt.y=wParam;
				ScreenToClient(hwndDlg,&pt);

				oldSplitterY=dat->iSplitterY;
				dat->iSplitterY=bFormat?rc.bottom-pt.y+1:rc.bottom-pt.y+26;
				if(dat->iSplitterY<67) 
					dat->iSplitterY=67;
				if(dat->iSplitterY>rc.bottom-rc.top-40) 
					dat->iSplitterY = rc.bottom-rc.top-40;
				g_LogOptions.iSplitterY = dat->iSplitterY;
			}
			PostMessage(hwndDlg,WM_SIZE,0,0);
		}break;
       case GC_CASCADENEWWINDOW:
            if ((HWND) wParam == hwndDlg)
                break;
            {
                RECT rcThis, rcNew;
				DWORD dwFlag;

				dwFlag = SWP_NOZORDER | SWP_NOSIZE;
				if(!IsWindowVisible ((HWND)wParam))
					dwFlag |= SWP_HIDEWINDOW;

                GetWindowRect(hwndDlg, &rcThis);
                GetWindowRect((HWND) wParam, &rcNew);
                if (abs(rcThis.left - rcNew.left) < 3 && abs(rcThis.top - rcNew.top) < 3) {
                    int offset = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
                    SetWindowPos((HWND) wParam, 0, rcNew.left + offset, rcNew.top + offset, 0, 0, dwFlag);
                    *(int *) lParam = 1;
                }
            }
            break;
		case GC_FIREHOOK:
		{
			if (lParam)
			{
				GCHOOK * gch = (GCHOOK *) lParam;
				NotifyEventHooks(hSendEvent,0,(WPARAM)gch);
				if (gch->pDest)
				{
					if (gch->pDest->pszID)
						free(gch->pDest->pszID);
					if (gch->pDest->pszModule)
						free((char*)gch->pDest->pszModule);
					free(gch->pDest);
				}
				if (gch->pszText)
					free(gch->pszText);
				if (gch->pszUID)
					free(gch->pszUID);
				free(gch);
			}
		}	break;
		case GC_CHANGEFILTERFLAG:
		{
			dat->iLogFilterFlags = lParam;
		}	break;
		case GC_SHOWFILTERMENU:
			{
			RECT rc;
    		HWND hwnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_FILTER), hwndDlg, FilterWndProc, (LPARAM)dat);
			GetWindowRect(GetDlgItem(hwndDlg, IDC_FILTER), &rc);
			SetWindowPos(hwnd, HWND_TOP, rc.left-78, (IsWindowVisible(GetDlgItem(hwndDlg, IDC_FILTER))||g_LogOptions.bShowName)?rc.bottom+1:rc.bottom-19, 0, 0, SWP_NOSIZE|SWP_SHOWWINDOW);
			SendMessage(hwnd, GC_FILTERFIX, 0, 0);
			}break;
		case GC_SHOWCOLORCHOOSER:
			{
				HWND ColorWindow;
				RECT rc;
				BOOL bFG = lParam == IDC_COLOR?TRUE:FALSE;
				COLORCHOOSER * pCC = malloc(sizeof(COLORCHOOSER));

				GetWindowRect(GetDlgItem(hwndDlg, bFG?IDC_COLOR:IDC_BKGCOLOR), &rc);
				pCC->hWndTarget = GetDlgItem(hwndDlg, IDC_MESSAGE);
				pCC->pModule = MM_FindModule(dat->pszModule);
				pCC->xPosition = rc.left+3;
				pCC->yPosition = IsWindowVisible(GetDlgItem(hwndDlg, IDC_COLOR))?rc.top-1:rc.top+20;
				pCC->bForeground = bFG;
				pCC->dat = dat;
				
				ColorWindow= CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_COLORCHOOSER), hwndDlg, DlgProcColorToolWindow, (LPARAM) pCC);

			}break;
		
        case WM_TIMER:
			if (wParam == TIMERID_FLASHWND) {
				FlashWindow(hwndDlg, TRUE);
				if (dat->nFlash > dat->nFlashMax) {
					KillTimer(hwndDlg, TIMERID_FLASHWND);
					FlashWindow(hwndDlg, FALSE);
					dat->nFlash = 0;
				}
				dat->nFlash++;
			}break;
		case WM_ACTIVATE:
		{
			if (LOWORD(wParam) != WA_ACTIVE)
				break;
 		} 
		//fall through
		case WM_MOUSEACTIVATE:
			{
//			InvalidateRect(GetDlgItem(hwndDlg,IDC_LOG), NULL, TRUE);
			CHARRANGE sel;

			SendMessage(GetDlgItem(hwndDlg, IDC_LOG), EM_EXGETSEL, 0, (LPARAM) & sel);
			sel.cpMin = sel.cpMax;
			SendMessage(GetDlgItem(hwndDlg, IDC_LOG), EM_EXSETSEL, 0, (LPARAM) & sel);

			SetFocus(GetDlgItem(hwndDlg,IDC_MESSAGE));
			if (KillTimer(hwndDlg, TIMERID_FLASHWND))
				FlashWindow(hwndDlg, FALSE);
			SetActiveChatWindow(dat->pszID, dat->pszModule);
			if(DBGetContactSettingWord(dat->hContact, dat->pszModule ,"ApparentMode", 0) != 0)
				DBWriteContactSettingWord(dat->hContact, dat->pszModule ,"ApparentMode",(LPARAM) 0);

			if(CallService(MS_CLIST_GETEVENT, (WPARAM)dat->hContact, (LPARAM)0))
				CallService(MS_CLIST_REMOVEEVENT, (WPARAM)dat->hContact, (LPARAM)"chaticon");
			}break;
		case WM_NOTIFY:
		{
			LPNMHDR pNmhdr = (LPNMHDR)lParam;
			switch (pNmhdr->code)
			{
			case NM_DBLCLK:
			case NM_RCLICK:
//			case NM_CLICK:
				{
					if (pNmhdr->idFrom = IDC_NICKLIST)
					{
						TVHITTESTINFO hti;
						hti.pt.x=(short)LOWORD(GetMessagePos());
						hti.pt.y=(short)HIWORD(GetMessagePos());
						ScreenToClient(pNmhdr->hwndFrom,&hti.pt);
						if(pNmhdr->code == NM_CLICK && TreeView_HitTest(pNmhdr->hwndFrom,&hti))// && hti.flags&TVHT_ONITEMICON)
						{
							BOOL bExpanded;							
							TVITEM tvi = {0};

							tvi.mask=TVIF_PARAM|TVIF_HANDLE|TVIF_STATE;
							tvi.hItem=hti.hItem;
							TreeView_GetItem(pNmhdr->hwndFrom,&tvi);

							bExpanded = tvi.state&TVIS_EXPANDED?TRUE:FALSE;
							if(!tvi.lParam)
								TreeView_SetItemState(pNmhdr->hwndFrom, tvi.hItem, bExpanded?INDEXTOSTATEIMAGEMASK(1):INDEXTOSTATEIMAGEMASK(1)|TVIS_EXPANDED,  bExpanded?TVIS_STATEIMAGEMASK:TVIS_EXPANDED|TVIS_STATEIMAGEMASK);
							InvalidateRect(pNmhdr->hwndFrom, NULL, TRUE);
						}
						else if(TreeView_HitTest(pNmhdr->hwndFrom,&hti) && hti.flags&TVHT_ONITEM)
						{
							TVITEM tvi = {0};
							tvi.mask=TVIF_PARAM|TVIF_HANDLE;
							tvi.hItem=hti.hItem;
							TreeView_GetItem(pNmhdr->hwndFrom,&tvi);
							if(pNmhdr->code == NM_DBLCLK && tvi.lParam)
							{
								if(GetKeyState(VK_SHIFT) & 0x8000)
								{	
									
									LRESULT lResult = (LRESULT)SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_GETSEL, (WPARAM)NULL, (LPARAM)NULL);
									int start = LOWORD(lResult);
									char * pszName = (char *) malloc(lstrlen(((USERINFO *)tvi.lParam)->pszUID) + 3);
									if(start == 0)
										_snprintf(pszName, lstrlen(((USERINFO *)tvi.lParam)->pszUID)+3, "%s: ", ((USERINFO *)tvi.lParam)->pszUID);
									else
										_snprintf(pszName, lstrlen(((USERINFO *)tvi.lParam)->pszUID)+2, "%s ", ((USERINFO *)tvi.lParam)->pszUID);

									SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_REPLACESEL, FALSE, (LPARAM) pszName);
									PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
	
								}
								else
									DoEventHookAsync(hwndDlg, dat->pszID, dat->pszModule, GC_USER_PRIVMESS, ((USERINFO *)tvi.lParam)->pszUID, NULL, (LPARAM)NULL);
							}
							else
							{
								TreeView_SelectItem(pNmhdr->hwndFrom, hti.hItem);
							}
						}

					}


				}break;

			case EN_MSGFILTER:
				{
					if (pNmhdr->idFrom = IDC_LOG && ((MSGFILTER *) lParam)->msg == WM_RBUTTONUP)
					{
						CHARRANGE sel, all = { 0, -1 };
						POINT pt;
						UINT uID = 0;
						HMENU hMenu = 0;

						pt.x = (short) LOWORD(((ENLINK *) lParam)->lParam);
						pt.y = (short) HIWORD(((ENLINK *) lParam)->lParam);
						ClientToScreen(pNmhdr->hwndFrom, &pt);
						
						uID = CreateGCMenu(hwndDlg, &hMenu, 1, pt, dat, NULL);
						switch (uID) 
						{
						case 0:
							break;
                         case ID_COPYALL:
							{
                             SendMessage(pNmhdr->hwndFrom, EM_EXGETSEL, 0, (LPARAM) & sel);
							SendMessage(pNmhdr->hwndFrom, EM_EXSETSEL, 0, (LPARAM) & all);
                            SendMessage(pNmhdr->hwndFrom, WM_COPY, 0, 0);
							SendMessage(pNmhdr->hwndFrom, EM_EXSETSEL, 0, (LPARAM) & sel);
        					PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0 );
                            }break;
                         case ID_CLEARLOG:
							{
                            SetDlgItemText(hwndDlg, IDC_LOG, "");
							LM_RemoveAll(&dat->pEventListStart, &dat->pEventListEnd);
							dat->iEventCount = 0;
							dat->LastTime = 0;
							PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0 );
                            }break;
					   default:
							PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0 );
							DoEventHookAsync(hwndDlg, dat->pszID, dat->pszModule, GC_USER_LOGMENU, NULL, NULL, (LPARAM)uID);
							break;

						}
						DestroyGCMenu(&hMenu, 3);
					}
				}break;

			case NM_CUSTOMDRAW:
			{
				if (pNmhdr->idFrom = IDC_NICKLIST)
				{
					LPNMTVCUSTOMDRAW pCustomDraw = (LPNMTVCUSTOMDRAW)lParam;
					switch (pCustomDraw->nmcd.dwDrawStage)
					{
						case CDDS_PREPAINT:
						SetWindowLongPtr(hwndDlg,DWL_MSGRESULT,CDRF_NOTIFYITEMDRAW); 
						return(TRUE); 

						case CDDS_ITEMPREPAINT:
							{
							switch (pCustomDraw->iLevel)
							{
								case 0:
									{
									TVITEM tvi = { 0 };
									RECT rc = pCustomDraw->nmcd.rc;
									HBRUSH hBr;
									LOGBRUSH br;
									HICON hIcon;

									tvi.mask = TVIF_HANDLE | TVIF_STATE;
									tvi.hItem = (HTREEITEM)pCustomDraw->nmcd.dwItemSpec;
									TreeView_GetItem(pNmhdr->hwndFrom, &tvi);
									rc.right = rc.left + GetSystemMetrics(SM_CXSMICON);
									rc.bottom = rc.top + GetSystemMetrics(SM_CYSMICON);
									hBr = CreateSolidBrush( g_LogOptions.crUserListBGColor ) ;
									FillRect(pCustomDraw->nmcd.hdc, &rc, hBr);
									hIcon = (tvi.state&TVIS_EXPANDED)?g_LogOptions.GroupOpenIcon:g_LogOptions.GroupClosedIcon ;
									DrawIconEx(pCustomDraw->nmcd.hdc,pCustomDraw->nmcd.rc.left,pCustomDraw->nmcd.rc.top,hIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
									SelectObject(pCustomDraw->nmcd.hdc, g_LogOptions.UserListHeadingsFont);
									GetObject(hBr, sizeof(br), &br); 
									if (pCustomDraw->nmcd.uItemState == (CDIS_FOCUS | CDIS_SELECTED) && br.lbStyle==BS_SOLID) // selected
										pCustomDraw->clrTextBk = br.lbColor;
									pCustomDraw->clrText = g_LogOptions.crUserListHeadingsColor;
									DeleteObject(hBr);
									break;
									}
								case 1:

									SelectObject(pCustomDraw->nmcd.hdc, g_LogOptions.UserListFont);
									if (pCustomDraw->nmcd.uItemState == (CDIS_FOCUS | CDIS_SELECTED)) // selected
										pCustomDraw->clrText = RGB(255, 255, 255);
									else	
										pCustomDraw->clrText = g_LogOptions.crUserListColor;
									break;
									default:break;
							}
							SetWindowLongPtr(hwndDlg,DWL_MSGRESULT, CDRF_NEWFONT); 
							return (TRUE); 
						}
					}
				}
			}break;

			case EN_LINK:
			if(pNmhdr->idFrom = IDC_LOG)
				switch (((ENLINK *) lParam)->msg) 
				{
					case WM_RBUTTONDOWN:
					case WM_LBUTTONUP:
					{
						TEXTRANGE tr;
						CHARRANGE sel;

						SendMessage(pNmhdr->hwndFrom, EM_EXGETSEL, 0, (LPARAM) & sel);
						if (sel.cpMin != sel.cpMax)
							break;
						tr.chrg = ((ENLINK *) lParam)->chrg;
						tr.lpstrText = malloc(tr.chrg.cpMax - tr.chrg.cpMin + 1);
						SendMessage(pNmhdr->hwndFrom, EM_GETTEXTRANGE, 0, (LPARAM) & tr);
						
						if (((ENLINK *) lParam)->msg == WM_RBUTTONDOWN) 
						{
							HMENU hSubMenu;
							POINT pt;

							hSubMenu = GetSubMenu(g_hMenu, 2);
							CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hSubMenu, 0);
							pt.x = (short) LOWORD(((ENLINK *) lParam)->lParam);
							pt.y = (short) HIWORD(((ENLINK *) lParam)->lParam);
							ClientToScreen(((NMHDR *) lParam)->hwndFrom, &pt);
							switch (TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL)) 
							{
								case ID_NEW:
									CallService(MS_UTILS_OPENURL, 1, (LPARAM) tr.lpstrText);
									break;
								case ID_CURR:
									CallService(MS_UTILS_OPENURL, 0, (LPARAM) tr.lpstrText);
									break;
								case ID_COPY:
								{
									HGLOBAL hData;
									if (!OpenClipboard(hwndDlg))
										break;
									EmptyClipboard();
									hData = GlobalAlloc(GMEM_MOVEABLE, lstrlen(tr.lpstrText) + 1);
									lstrcpy((char *) GlobalLock(hData), tr.lpstrText);
									GlobalUnlock(hData);
									SetClipboardData(CF_TEXT, hData);
									CloseClipboard();
									PostMessage(hwndDlg, WM_ACTIVATE, 0, 0 );
									break;
								}
								default:break;
							}
							free(tr.lpstrText);
							return TRUE;
						} 
						else  
						{
							CallService(MS_UTILS_OPENURL, 1, (LPARAM) tr.lpstrText);
							SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
						}

						free(tr.lpstrText);
						break;
					}
				}
				break;
			}
	
		}break;
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				{
                char *pszText = NULL;
				if(!IsWindowEnabled(GetDlgItem(hwndDlg,IDOK))) 
					break;

				pszText = Message_GetFromStream(hwndDlg, dat);

//				MessageBox(hwndDlg, pszText, "text with RTF", 0);

				WM_AddCommand(dat->pszID, dat->pszModule, pszText);

				DoRtfToTags(pszText, dat); // TRUE if success

//				MessageBox(hwndDlg, pszText, "text with tags", 0);


				if(MM_FindModule(dat->pszModule)->bAckMsg) 
				{
					EnableWindow(GetDlgItem(hwndDlg,IDC_MESSAGE),FALSE);
					SendDlgItemMessage(hwndDlg,IDC_MESSAGE,EM_SETREADONLY,TRUE,0);
				}
				else
					SendDlgItemMessage(hwndDlg,IDC_MESSAGE,WM_SETTEXT,0,(LPARAM)"");

				EnableWindow(GetDlgItem(hwndDlg,IDOK),FALSE);

				DoEventHookAsync(hwndDlg, dat->pszID, dat->pszModule, GC_USER_MESSAGE, NULL, pszText, (LPARAM)NULL);
				free(pszText);
				SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));

				}break;
			case IDC_SHOWNICKLIST:
				{
					if(!IsWindowEnabled(GetDlgItem(hwndDlg,IDC_SHOWNICKLIST))) 
						break;
					ShowWindow(GetDlgItem(hwndDlg, IDC_NICKLIST), IsWindowVisible(GetDlgItem(hwndDlg, IDC_NICKLIST))?SW_HIDE:SW_SHOW);
					SendMessage(hwndDlg, WM_SIZE, 0, 0);
				}break;
			case IDC_MESSAGE:
				{
					EnableWindow(GetDlgItem(hwndDlg, IDOK), GetRichTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE)) != 0);
				}break;
			case IDC_SMILEY:
				{
					SMADD_SHOWSEL smaddInfo;
					RECT rc;

					GetWindowRect(GetDlgItem(hwndDlg, IDC_SMILEY), &rc);
					
					smaddInfo.cbSize = sizeof(SMADD_SHOWSEL);
					smaddInfo.hwndTarget = GetDlgItem(hwndDlg, IDC_MESSAGE);
					smaddInfo.targetMessage = EM_REPLACESEL;
					smaddInfo.targetWParam = TRUE;
					smaddInfo.Protocolname = dat->pszModule;
					smaddInfo.Direction = 3;
					smaddInfo.xPosition = rc.left+3;
					smaddInfo.yPosition = rc.top-1;

					if(SmileyAddInstalled)
						CallService(MS_SMILEYADD_SHOWSELECTION, 0, (LPARAM) &smaddInfo);

				}break;
			case IDC_HISTORY:
				{
					char szFile[MAX_PATH];
					char szName[MAX_PATH];
					char szFolder[MAX_PATH];
					MODULE * pInfo = MM_FindModule(dat->pszModule);

					if(!IsWindowEnabled(GetDlgItem(hwndDlg,IDC_HISTORY))) 
						break;

					if (pInfo)
					{

						_snprintf(szName, MAX_PATH,"%s",pInfo->pszModDispName?pInfo->pszModDispName:dat->pszModule);
						ValidateFilename(szName);
						_snprintf(szFolder, MAX_PATH,"%s\\%s", g_LogOptions.pszLogDir, szName );
											
						_snprintf(szName, MAX_PATH,"%s.log",dat->pszID);
						ValidateFilename(szName);

						_snprintf(szFile, MAX_PATH,"%s\\%s", szFolder, szName ); 
						
						ShellExecute(hwndDlg, "open", szFile, NULL, NULL, SW_SHOW);
					}

				}break;
			case IDC_CHANMGR:
				{
				if(!IsWindowEnabled(GetDlgItem(hwndDlg,IDC_CHANMGR))) 
					break;
				DoEventHookAsync(hwndDlg, dat->pszID, dat->pszModule, GC_USER_CHANMGR, NULL, NULL, (LPARAM)NULL);
				}break;
			case IDC_FILTER:
				{
				if(!IsWindowEnabled(GetDlgItem(hwndDlg,IDC_FILTER))) 
					break;
				dat->bFilterEnabled = IsDlgButtonChecked(hwndDlg, IDC_FILTER);
				if (dat->bFilterEnabled && DBGetContactSettingByte(NULL, "Chat", "RightClickFilter", 0) == 0)
				{
					SendMessage(hwndDlg, GC_SHOWFILTERMENU, 0, 0);
					break;
				}
				SendMessage(hwndDlg, GC_REDRAWLOG, 0, 0);
				}break;
			case IDC_BKGCOLOR:
				{
					CHARFORMAT2 cf;

					cf.cbSize = sizeof(CHARFORMAT2);
					cf.dwEffects = 0;
	
					if(!IsWindowEnabled(GetDlgItem(hwndDlg,IDC_BKGCOLOR))) 
						break;
					if (IsDlgButtonChecked(hwndDlg, IDC_BKGCOLOR) )
					{
						if(DBGetContactSettingByte(NULL, "Chat", "RightClickFilter", 0) == 0)
						{
							SendMessage(hwndDlg, GC_SHOWCOLORCHOOSER, 0, (LPARAM)IDC_BKGCOLOR);
						}
						else if (dat->bBGSet)
						{
							cf.dwMask = CFM_BACKCOLOR;
							cf.crBackColor = MM_FindModule(dat->pszModule)->crColors[dat->iBG];
							SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
						}
							
					}
					else
					{
						cf.dwMask = CFM_BACKCOLOR;
						cf.crBackColor = (COLORREF)DBGetContactSettingDword(NULL, "Chat", "ColorMessageBG", GetSysColor(COLOR_WINDOW));
						SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

					}
				}break;
			case IDC_COLOR:
				{
					CHARFORMAT2 cf;

					cf.cbSize = sizeof(CHARFORMAT2);
					cf.dwEffects = 0;
	
					if(!IsWindowEnabled(GetDlgItem(hwndDlg,IDC_COLOR))) 
						break;
					if (IsDlgButtonChecked(hwndDlg, IDC_COLOR) )
					{
						if(DBGetContactSettingByte(NULL, "Chat", "RightClickFilter", 0) == 0)
						{
							SendMessage(hwndDlg, GC_SHOWCOLORCHOOSER, 0, (LPARAM)IDC_COLOR);
						}
						else if (dat->bFGSet)
						{
							cf.dwMask = CFM_COLOR;
							cf.crTextColor = MM_FindModule(dat->pszModule)->crColors[dat->iFG];
							SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
						}
							
					}
					else
					{
						COLORREF cr;
							
						LoadMsgDlgFont(17, NULL, &cr);
						cf.dwMask = CFM_COLOR;
						cf.crTextColor = cr;
						SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

					}
				}break;
			case IDC_BOLD:
			case IDC_ITALICS:
			case IDC_UNDERLINE:
				{
					CHARFORMAT2 cf;
					cf.cbSize = sizeof(CHARFORMAT2);
					cf.dwMask = CFM_BOLD|CFM_ITALIC|CFM_UNDERLINE;
					cf.dwEffects = 0;

					if(LOWORD(wParam) == IDC_BOLD && !IsWindowEnabled(GetDlgItem(hwndDlg,IDC_BOLD))) 
						break;
					if(LOWORD(wParam) == IDC_ITALICS && !IsWindowEnabled(GetDlgItem(hwndDlg,IDC_ITALICS))) 
						break;
					if(LOWORD(wParam) == IDC_UNDERLINE && !IsWindowEnabled(GetDlgItem(hwndDlg,IDC_UNDERLINE))) 
						break;
					if (IsDlgButtonChecked(hwndDlg, IDC_BOLD))
						cf.dwEffects |= CFE_BOLD;
					if (IsDlgButtonChecked(hwndDlg, IDC_ITALICS))
						cf.dwEffects |= CFE_ITALIC;
					if (IsDlgButtonChecked(hwndDlg, IDC_UNDERLINE))
						cf.dwEffects |= CFE_UNDERLINE;

					SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

				}break;

			default:break;
			}
		} break;
		case WM_KEYDOWN:
			SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
		case WM_MOVE:
			{
				SendMessage(hwndDlg,GC_SAVEWNDPOS,0,1);
			}break;
		case GC_SAVEWNDPOS:
		{
			WINDOWPLACEMENT wp = { 0 };
			HANDLE hContact;

			wp.length = sizeof(wp);
			GetWindowPlacement(hwndDlg, &wp);
			if(!lParam) // window destroyed
			{
				if (dat && DBGetContactSettingByte(NULL, "Chat", "SavePosition", 0))
				{
					hContact = dat->hContact;
					DBWriteContactSettingDword(hContact, "Chat", "roomx", wp.rcNormalPosition.left);
					DBWriteContactSettingDword(hContact, "Chat", "roomy", wp.rcNormalPosition.top);
					DBWriteContactSettingDword(hContact, "Chat", "roomwidth" , wp.rcNormalPosition.right - wp.rcNormalPosition.left);
					DBWriteContactSettingDword(hContact, "Chat", "roomheight", wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
				}
			}
			else
			{

				g_LogOptions.iX = wp.rcNormalPosition.left;
				g_LogOptions.iY = wp.rcNormalPosition.top;
				g_LogOptions.iWidth = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
				g_LogOptions.iHeight = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;

			}
		}break;
		case WM_PAINT:
		{
			// this is where the magic begins
			if(g_LogOptions.bShowName && GetUpdateRect(hwndDlg, NULL, FALSE))
			{
				PAINTSTRUCT		ps;
				HFONT			hOldFont;
				int				iWidth, i;
				RECT			rc = {0};
				HDC				hdc = BeginPaint (hwndDlg, &ps);

				GetClientRect(hwndDlg, &rc);

				rc.right -= 130;
				hOldFont = SelectObject(hdc, g_LogOptions.NameFont);
				iWidth = GetTextPixelSize(dat->pszName, (HFONT)g_LogOptions.NameFont, TRUE) + 18;
				if (iWidth > rc.right)
					iWidth = rc.right;
				i = iWidth;
				while (i > 0)
				{
					DrawIconEx(hdc,i - 24,rc.top,g_LogOptions.TagIcon1,24,28,0,NULL,DI_NORMAL);
					i -= 24;
				}
				DrawIconEx(hdc, iWidth ,rc.top,g_LogOptions.TagIcon2,24,28,0,NULL,DI_NORMAL);
				rc.top = 2; rc.left += 12; rc.bottom = rc.top +22; 
				SetBkMode(hdc, TRANSPARENT);
				SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
				DrawText(hdc ,dat->pszName ,   -1, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS );
				SelectObject(hdc, hOldFont);
				
				EndPaint(hwndDlg, &ps);
			}
			
		} break;

		case WM_GETMINMAXINFO:
		{
			MINMAXINFO* mmi = (MINMAXINFO*)lParam;
			mmi->ptMinTrackSize.x = dat->iSplitterX + 43;
			if(mmi->ptMinTrackSize.x < 238)
				mmi->ptMinTrackSize.x = 238;

			mmi->ptMinTrackSize.y = dat->iSplitterY + 74;

			
		} break;
		case WM_CLOSE:
		{
			ShowWindow(hwndDlg, SW_HIDE);
			SendMessage(hwndDlg, WM_SETREDRAW, FALSE, 0);
		} break;

		case GC_CLOSEWINDOW:
		{	
			WM_RemoveWindow(dat->pszID, dat->pszModule);
			if(dat->hContact)
				CList_SetOffline(dat->hContact, dat->iType == GCW_CHATROOM?TRUE:FALSE);
			DestroyWindow(hwndDlg);
		} break;

		case WM_DESTROY:
		{
			SendMessage(hwndDlg,GC_SAVEWNDPOS,0,0);

			DBWriteContactSettingString(dat->hContact, dat->pszModule , "Topic", "");
			DBWriteContactSettingString(dat->hContact, dat->pszModule, "StatusBar", "");
//			DBWriteContactSettingDword(dat->hContact, dat->pszModule, "Count", 0);

			DoEventHook(hwndDlg, dat->pszID, dat->pszModule, GC_USER_TERMINATE, NULL, NULL, (DWORD)dat->ItemData);

			if(CallService(MS_CLIST_GETEVENT, (WPARAM)dat->hContact, (LPARAM)0))
				CallService(MS_CLIST_REMOVEEVENT, (WPARAM)dat->hContact, (LPARAM)"chaticon");
			DBWriteContactSettingWord(dat->hContact, dat->pszModule ,"ApparentMode",(LPARAM) 0);

			DestroyWindow(dat->hwndStatus);
			LM_RemoveAll(&dat->pEventListStart, &dat->pEventListEnd);
			UM_RemoveAll(&dat->pUserList);
			SM_RemoveAll(&dat->pStatusList);
			if(dat->pszID)
				free(dat->pszID);
			if(dat->pszModule)
				free(dat->pszModule);
			if(dat->pszName)
				free(dat->pszName);
			if(dat->pszStatusbarText)
				free(dat->pszStatusbarText);
			if(dat->pszTopic)
				free(dat->pszTopic);
			free(dat);
			SetWindowLong(hwndDlg,GWL_USERDATA,0);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_SPLITTERX),GWL_WNDPROC,(LONG)OldSplitterProc);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_SPLITTERY),GWL_WNDPROC,(LONG)OldSplitterProc);
            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_UNSUBCLASSED, 0, 0);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_MESSAGE),GWL_WNDPROC,(LONG)OldMessageProc);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_NICKLIST),GWL_WNDPROC,(LONG)OldNicklistProc);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_LOG),GWL_WNDPROC,(LONG)OldLogProc);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_FILTER),GWL_WNDPROC,(LONG)OldFilterButtonProc);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_COLOR),GWL_WNDPROC,(LONG)OldFilterButtonProc);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_BKGCOLOR),GWL_WNDPROC,(LONG)OldFilterButtonProc);
		}break;

		default:break;
	}
	return(FALSE);
}


