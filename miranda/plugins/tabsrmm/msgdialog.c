/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
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


$Id$
*/

#include "commonheaders.h"
#pragma hdrstop
// IEVIew MOD Begin
// IEVIew MOD End
#include "sendqueue.h"
#include "chat/chat.h"
#include <uxtheme.h>

#define TOOLBAR_PROTO_HIDDEN 1
#define TOOLBAR_SEND_HIDDEN 2

#ifdef __MATHMOD_SUPPORT
	#include "m_MathModule.h"
#endif

extern MYGLOBALS myGlobals;
extern NEN_OPTIONS nen_options;
extern TemplateSet RTL_Active, LTR_Active;
extern HANDLE hMessageWindowList;
extern HINSTANCE g_hInst;
extern char *szWarnClose;
extern struct RTFColorTable *rtf_ctable;
extern PSLWA pSetLayeredWindowAttributes;
extern COLORREF g_ContainerColorKey;
extern StatusItems_t StatusItems[];
extern struct GlobalLogSettings_t g_Settings;
extern CRITICAL_SECTION cs_sessions;

extern PITA pfnIsThemeActive;
extern POTD pfnOpenThemeData;
extern PDTB pfnDrawThemeBackground;
extern PCTD pfnCloseThemeData;
extern PDTT pfnDrawThemeText;
extern PITBPT pfnIsThemeBackgroundPartiallyTransparent;
extern PDTPB  pfnDrawThemeParentBackground;
extern PGTBCR pfnGetThemeBackgroundContentRect;

extern          char *FilterEventMarkersA(char *szText);

char *xStatusDescr[] = { "Angry", "Duck", "Tired", "Party", "Beer", "Thinking", "Eating", "TV", "Friends", "Coffee",
                         "Music", "Business", "Camera", "Funny", "Phone", "Games", "College", "Shopping", "Sick", "Sleeping",
                         "Surfing", "@Internet", "Engineering", "Typing", "Eating... yummy", "Having fun", "Chit chatting",
						 "Crashing", "Going to toilet", "<undef>", "<undef>", "<undef>"};
                         
static DWORD CALLBACK StreamOut(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb);

TCHAR *pszIDCSAVE_close = 0, *pszIDCSAVE_save = 0;

static WNDPROC OldMessageEditProc, OldAvatarWndProc, OldMessageLogProc, OldIEViewProc = 0;;
WNDPROC OldSplitterProc = 0;

static const UINT infoLineControls[] = { IDC_PROTOCOL, /* IDC_PROTOMENU, */ IDC_NAME, /* IDC_INFOPANELMENU */};
static const UINT buttonLineControlsNew[] = { IDC_PIC, IDC_HISTORY, IDC_TIME, IDC_QUOTE, IDC_SAVE /*, IDC_SENDMENU */};
static const UINT sendControls[] = { IDC_MESSAGE, IDC_LOG };
static const UINT formatControls[] = { IDC_SMILEYBTN, IDC_FONTBOLD, IDC_FONTITALIC, IDC_FONTUNDERLINE, IDC_FONTFACE }; //, IDC_FONTFACE, IDC_FONTCOLOR};
static const UINT controlsToHide[] = { IDOK, IDC_PIC, IDC_PROTOCOL, IDC_FONTFACE, IDC_FONTUNDERLINE, IDC_HISTORY, -1 };
static const UINT controlsToHide1[] = { IDOK, IDC_FONTFACE, IDC_FONTUNDERLINE, IDC_FONTITALIC, IDC_FONTBOLD, IDC_PROTOCOL, -1 };
static const UINT controlsToHide2[] = { IDOK, IDC_PIC, IDC_PROTOCOL, -1};
static const UINT addControls[] = { IDC_ADD, IDC_CANCELADD };

const UINT infoPanelControls[] = {IDC_PANELPIC, IDC_PANELNICK, IDC_PANELUIN, 
                                  IDC_PANELSTATUS, IDC_APPARENTMODE, IDC_TOGGLENOTES, IDC_NOTES, IDC_PANELSPLITTER};
const UINT errorControls[] = { IDC_STATICERRORICON, IDC_STATICTEXT, IDC_RETRY, IDC_CANCELSEND, IDC_MSGSENDLATER};
const UINT errorButtons[] = { IDC_RETRY, IDC_CANCELSEND, IDC_MSGSENDLATER};

static struct _tooltips { int id; TCHAR *szTip;} tooltips[] = {
    IDC_HISTORY, _T("View User's History"),
    IDC_TIME, _T("Message Log Options"),
    IDC_QUOTE, _T("Quote last message OR selected text"),
    IDC_PIC, _T("Avatar Options"),
    IDC_SMILEYBTN, _T("Insert Emoticon"),
    IDC_ADD, _T("Add this contact permanently to your contact list"),
    IDC_CANCELADD, _T("Do not add this contact permanently"),
    IDOK, _T("Send message\nClick dropdown arrow for sending options"),
    IDC_FONTBOLD, _T("Bold text"),
    IDC_FONTITALIC, _T("Italic text"),
    IDC_FONTUNDERLINE, _T("Underlined text"),
    IDC_FONTFACE, _T("Select font color"),
    IDC_TOGGLENOTES, _T("Toggle notes display"),
    IDC_APPARENTMODE, _T("Set your visibility for this contact"),
    -1, NULL
};

static struct _buttonicons { int id; HICON *pIcon; } buttonicons[] = {
    IDC_HISTORY, &myGlobals.g_buttonBarIcons[1],
    IDC_TIME, &myGlobals.g_buttonBarIcons[2],
    IDC_QUOTE, &myGlobals.g_buttonBarIcons[8],
    IDC_SAVE, &myGlobals.g_buttonBarIcons[6],
    IDC_PIC, &myGlobals.g_buttonBarIcons[10],
    IDOK, &myGlobals.g_buttonBarIcons[9],
    IDC_ADD, &myGlobals.g_buttonBarIcons[0],
    IDC_CANCELADD, &myGlobals.g_buttonBarIcons[6],
    IDC_FONTBOLD, &myGlobals.g_buttonBarIcons[17],
    IDC_FONTITALIC, &myGlobals.g_buttonBarIcons[18],
    IDC_FONTUNDERLINE, &myGlobals.g_buttonBarIcons[19],
    IDC_FONTFACE, &myGlobals.g_buttonBarIcons[21],
    IDC_NAME, &myGlobals.g_buttonBarIcons[4],
    IDC_LOGFROZEN, &myGlobals.g_buttonBarIcons[24],
    IDC_TOGGLENOTES, &myGlobals.g_buttonBarIcons[28],
    IDC_TOGGLESIDEBAR, &myGlobals.g_buttonBarIcons[25],
    -1, NULL
};

struct SendJob *sendJobs = NULL;
static int splitterEdges = -1;

static void ResizeIeView(HWND hwndDlg, struct MessageWindowData *dat, DWORD px, DWORD py, DWORD cx, DWORD cy)
{
    RECT rcRichEdit, rcIeView;
    POINT pt;
    IEVIEWWINDOW ieWindow;

	GetWindowRect(GetDlgItem(hwndDlg, IDC_LOG), &rcRichEdit);
    pt.x = rcRichEdit.left;
    pt.y = rcRichEdit.top;
    ScreenToClient(hwndDlg, &pt);
    ieWindow.cbSize = sizeof(IEVIEWWINDOW);
    ieWindow.iType = IEW_SETPOS;
    ieWindow.parent = hwndDlg;
    ieWindow.hwnd = dat->hwndIEView;
    if(cx != 0 || cy != 0) {
        ieWindow.x = px;
        ieWindow.y = py;
        ieWindow.cx = cx;
        ieWindow.cy = cy;
    }
    else {
        ieWindow.x = pt.x;
        ieWindow.y = pt.y;
        ieWindow.cx = rcRichEdit.right - rcRichEdit.left;
        ieWindow.cy = rcRichEdit.bottom - rcRichEdit.top;
    }
    GetWindowRect(dat->hwndIEView, &rcIeView);
    if(ieWindow.cx != 0 && ieWindow.cy != 0) {
        CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
    }
}

#ifdef __MATHMOD_SUPPORT
static void updatePreview(HWND hwndDlg, struct MessageWindowData *dat)
{	
	TMathWindowInfo mathWndInfo;

	int len=GetWindowTextLengthA( GetDlgItem( hwndDlg, IDC_MESSAGE) );
	RECT windRect;
	char * thestr = malloc(len+5);
	GetWindowTextA( GetDlgItem( hwndDlg, IDC_MESSAGE), thestr, len+1);
	GetWindowRect(dat->pContainer->hwnd,&windRect);
	mathWndInfo.top=windRect.top;
	mathWndInfo.left=windRect.left;
	mathWndInfo.right=windRect.right;
	mathWndInfo.bottom=windRect.bottom;

	CallService(MTH_SETFORMULA,0,(LPARAM) thestr);
	CallService(MTH_RESIZE,0,(LPARAM) &mathWndInfo);
	free(thestr);
}

static void updateMathWindow(HWND hwndDlg, struct MessageWindowData *dat)
{
    WINDOWPLACEMENT cWinPlace;

    if(!myGlobals.m_MathModAvail)
        return;
    
    updatePreview(hwndDlg, dat);
    CallService(MTH_SHOW, 0, 0);
    cWinPlace.length=sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(dat->pContainer->hwnd, &cWinPlace);
    return;
    if (cWinPlace.showCmd == SW_SHOWMAXIMIZED)
    {
        RECT rcWindow;
        GetWindowRect(hwndDlg, &rcWindow);
        if(CallService(MTH_GET_PREVIEW_SHOWN,0,0))
            MoveWindow(dat->pContainer->hwnd,rcWindow.left,rcWindow.top,rcWindow.right-rcWindow.left,GetSystemMetrics(SM_CYSCREEN)-CallService(MTH_GET_PREVIEW_HEIGHT ,0,0),1);
        else
            MoveWindow(dat->pContainer->hwnd,rcWindow.left,rcWindow.top,rcWindow.right-rcWindow.left,GetSystemMetrics(SM_CYSCREEN),1);
    }
}
#endif

LRESULT CALLBACK IEViewSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct MessageWindowData *mwdat = (struct MessageWindowData *)GetWindowLong(GetParent(hwnd), GWL_USERDATA);

    switch(msg) {
        case WM_NCCALCSIZE:
            return(NcCalcRichEditFrame(hwnd, mwdat, ID_EXTBKHISTORY, msg, wParam, lParam, OldIEViewProc));
		case WM_NCPAINT:
			return(DrawRichEditFrame(hwnd, mwdat, ID_EXTBKHISTORY, msg, wParam, lParam, OldIEViewProc));
	}
	return CallWindowProc(OldIEViewProc, hwnd, msg, wParam, lParam);
}

/*
 * update state of the container - this is called whenever a tab becomes active, no matter how and
 * deals with various things like updating the title bar, removing flashing icons, updating the
 * session list, switching the keyboard layout (autolocale active)  and the general container status.
 *
 * it protects itself from being called more than once per session activation and is valid for
 * normal IM sessions *only*. Group chat sessions have their own activation handler (see chat/window.c)
*/

static void MsgWindowUpdateState(HWND hwndDlg, struct MessageWindowData *dat, UINT msg)
{
    HWND hwndTab = GetParent(hwndDlg);

    if(dat && dat->iTabID >= 0) {
        if(msg == WM_ACTIVATE) {
            if (dat->pContainer->dwFlags & CNT_TRANSPARENCY && pSetLayeredWindowAttributes != NULL && !dat->pContainer->bSkinned) {
                DWORD trans = LOWORD(dat->pContainer->dwTransparency);
                pSetLayeredWindowAttributes(dat->pContainer->hwnd, 0, (BYTE)trans, (dat->pContainer->dwFlags & CNT_TRANSPARENCY ? LWA_ALPHA : 0));
            }
        }
        if(dat->pContainer->hwndSaved == hwndDlg)
            return;

        dat->pContainer->hwndSaved = hwndDlg;

        dat->dwTickLastEvent = 0;
        dat->dwFlags &= ~MWF_DIVIDERSET;
        if (KillTimer(hwndDlg, TIMERID_FLASHWND)) {
            FlashTab(dat, hwndTab, dat->iTabID, &dat->bTabFlash, FALSE, dat->hTabIcon);
            dat->mayFlashTab = FALSE;
        }
        if(dat->pContainer->dwFlashingStarted != 0) {
            FlashContainer(dat->pContainer, 0, 0);
            dat->pContainer->dwFlashingStarted = 0;
        }
        if(dat->dwFlagsEx & MWF_SHOW_FLASHCLIST) {
            dat->dwFlagsEx &= ~MWF_SHOW_FLASHCLIST;
            if(dat->hFlashingEvent !=0)
                CallService(MS_CLIST_REMOVEEVENT, (WPARAM)dat->hContact, (LPARAM)dat->hFlashingEvent);
            dat->hFlashingEvent = 0;
        }
        if(dat->pContainer->dwFlags & CNT_NEED_UPDATETITLE) {
            dat->pContainer->dwFlags &= ~CNT_NEED_UPDATETITLE;
            //SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
        }
        if (dat->dwFlags & MWF_DEFERREDREMAKELOG) {
            SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
            dat->dwFlags &= ~MWF_DEFERREDREMAKELOG;
        }

        if(dat->dwFlags & MWF_NEEDCHECKSIZE)
            PostMessage(hwndDlg, DM_SAVESIZE, 0, 0);		

        if (myGlobals.m_AutoLocaleSupport && dat->hContact != 0) {
            if(dat->hkl == 0)
                DM_LoadLocale(hwndDlg, dat);
            PostMessage(hwndDlg, DM_SETLOCALE, 0, 0);
        }
        SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
        SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
        UpdateStatusBar(hwndDlg, dat);
        dat->dwLastActivity = GetTickCount();
        dat->pContainer->dwLastActivity = dat->dwLastActivity;
        UpdateContainerMenu(hwndDlg, dat);
        UpdateTrayMenuState(dat, FALSE);
        if(myGlobals.m_TipOwner == dat->hContact)
            RemoveBalloonTip();
    #if defined(__MATHMOD_SUPPORT)
        if(myGlobals.m_MathModAvail) {
            CallService(MTH_Set_ToolboxEditHwnd,0,(LPARAM)GetDlgItem(hwndDlg, IDC_MESSAGE)); 
            updateMathWindow(hwndDlg, dat);
        }
    #endif                
        dat->dwLastUpdate = GetTickCount();
        if(dat->hContact)
            PostMessage(hwndDlg, DM_REMOVEPOPUPS, PU_REMOVE_ON_FOCUS, 0);
        if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
            InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELUIN), NULL, FALSE);
            InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELSTATUS), NULL, FALSE);
        }
        if(dat->dwFlags & MWF_DEFERREDSCROLL && dat->hwndIEView == 0) {
            HWND hwnd = GetDlgItem(hwndDlg, IDC_LOG);

            SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
            dat->dwFlags &= ~MWF_DEFERREDSCROLL;
            SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_TOP, 0), 0);
            SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
            DM_ScrollToBottom(hwndDlg, dat, 0, 1);
            PostMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEDOWN, 0), 0);
        }
        DM_SetDBButtonStates(hwndDlg, dat);
        if(dat->hwndIEView) {
            //GetRealIEViewWindow(hwndDlg, dat);
            RECT rcRTF;
            POINT pt;

            GetWindowRect(GetDlgItem(hwndDlg, IDC_LOG), &rcRTF);
            rcRTF.left += 20; rcRTF.top += 20;
            pt.x = rcRTF.left; pt.y = rcRTF.top;
            if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "subclassIEView", 0) && dat->oldIEViewProc == 0) {
                WNDPROC wndProc = (WNDPROC)SetWindowLong(dat->hwndIEView, GWL_WNDPROC, (LONG)IEViewSubclassProc);
                if(OldIEViewProc == 0)
                    OldIEViewProc = wndProc;
                dat->oldIEViewProc = wndProc;
            }
            dat->hwndIWebBrowserControl = WindowFromPoint(pt);
        }
        if(dat->dwFlagsEx & MWF_EX_DELAYEDSPLITTER) {
            dat->dwFlagsEx &= ~MWF_EX_DELAYEDSPLITTER;
            ShowWindow(dat->pContainer->hwnd, SW_RESTORE);
            PostMessage(hwndDlg, DM_SPLITTERMOVEDGLOBAL, dat->wParam, dat->lParam);
            dat->wParam = dat->lParam = 0;
        }
        if(dat->dwFlagsEx & MWF_EX_AVATARCHANGED) {
            dat->dwFlagsEx &= ~MWF_EX_AVATARCHANGED;
            PostMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
        }
    }
}

static void ConfigurePanel(HWND hwndDlg, struct MessageWindowData *dat)
{
    const UINT cntrls[] = {IDC_PANELNICK, IDC_PANELUIN, IDC_PANELSTATUS, IDC_APPARENTMODE};

    ShowMultipleControls(hwndDlg, cntrls, 4, dat->dwFlagsEx & MWF_SHOW_INFONOTES ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hwndDlg, IDC_NOTES), dat->dwFlagsEx & MWF_SHOW_INFONOTES ? SW_SHOW : SW_HIDE);
}
static void ShowHideInfoPanel(HWND hwndDlg, struct MessageWindowData *dat)
{
    HBITMAP hbm = dat->dwFlagsEx & MWF_SHOW_INFOPANEL ? dat->hOwnPic : (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown);
    BITMAP bm;
    
    if(dat->dwFlags & MWF_ERRORSTATE)
        return;
    
    dat->iRealAvatarHeight = 0;
    AdjustBottomAvatarDisplay(hwndDlg, dat);
    GetObject(hbm, sizeof(bm), &bm);
    CalcDynamicAvatarSize(hwndDlg, dat, &bm);
    ShowMultipleControls(hwndDlg, infoPanelControls, 8, dat->dwFlagsEx & MWF_SHOW_INFOPANEL ? SW_SHOW : SW_HIDE);
	
    if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
        GetAvatarVisibility(hwndDlg, dat);
        ConfigurePanel(hwndDlg, dat);
        UpdateApparentModeDisplay(hwndDlg, dat);
        InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELNICK), NULL, FALSE);
        InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELUIN), NULL, FALSE);
        InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELSTATUS), NULL, FALSE);
    }
    SendMessage(hwndDlg, WM_SIZE, 0, 0);
    InvalidateRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), NULL, TRUE);
    DM_ScrollToBottom(hwndDlg, dat, 0, 1);
}
// drop files onto message input area...

static void AddToFileList(char ***pppFiles,int *totalCount,const char *szFilename)
{
	*pppFiles=(char**)realloc(*pppFiles,(++*totalCount+1)*sizeof(char*));
	(*pppFiles)[*totalCount]=NULL;
	(*pppFiles)[*totalCount-1]=_strdup(szFilename);
	if(GetFileAttributesA(szFilename)&FILE_ATTRIBUTE_DIRECTORY) {
		WIN32_FIND_DATAA fd;
		HANDLE hFind;
		char szPath[MAX_PATH];
		lstrcpyA(szPath,szFilename);
		lstrcatA(szPath,"\\*");
		if(hFind=FindFirstFileA(szPath,&fd)) {
			do {
				if(!lstrcmpA(fd.cFileName,".") || !lstrcmpA(fd.cFileName,"..")) continue;
				lstrcpyA(szPath,szFilename);
				lstrcatA(szPath,"\\");
				lstrcatA(szPath,fd.cFileName);
				AddToFileList(pppFiles,totalCount,szPath);
			} while(FindNextFileA(hFind,&fd));
			FindClose(hFind);
		}
	}
}

void ShowMultipleControls(HWND hwndDlg, const UINT *controls, int cControls, int state)
{
    int i;
    for (i = 0; i < cControls; i++)
        ShowWindow(GetDlgItem(hwndDlg, controls[i]), state);
}

void SetDialogToType(HWND hwndDlg)
{
    struct MessageWindowData *dat;
    int showToolbar = 0;
    
    dat = (struct MessageWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);
    showToolbar = dat->pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;
    
    if (dat->hContact) {
        ShowMultipleControls(hwndDlg, buttonLineControlsNew, sizeof(buttonLineControlsNew) / sizeof(buttonLineControlsNew[0]), showToolbar ? SW_SHOW : SW_HIDE);
        ShowMultipleControls(hwndDlg, infoLineControls, sizeof(infoLineControls) / sizeof(infoLineControls[0]), showToolbar ? SW_SHOW : SW_HIDE);
        ShowMultipleControls(hwndDlg, formatControls, sizeof(formatControls) / sizeof(formatControls[0]), showToolbar ? SW_SHOW : SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_SMILEYBTN), showToolbar ? SW_SHOW : SW_HIDE);
        
        if (DBGetContactSettingByte(dat->hContact, "CList", "NotOnList", 0)) {
            dat->bNotOnList = TRUE;
            ShowMultipleControls(hwndDlg, addControls, 2, SW_SHOW);
        }
        else {
            ShowMultipleControls(hwndDlg, addControls, 2, SW_HIDE);
            dat->bNotOnList = FALSE;
        }
    } else {
        ShowMultipleControls(hwndDlg, buttonLineControlsNew, sizeof(buttonLineControlsNew) / sizeof(buttonLineControlsNew[0]), SW_HIDE);
        ShowMultipleControls(hwndDlg, infoLineControls, sizeof(infoLineControls) / sizeof(infoLineControls[0]), SW_HIDE);
        ShowMultipleControls(hwndDlg, addControls, 2, SW_HIDE);
    }

    ShowWindow(GetDlgItem(hwndDlg, IDC_TOGGLETOOLBAR), SW_HIDE);

    ShowWindow(GetDlgItem(hwndDlg, IDC_LOGFROZEN), SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_LOGFROZENTEXT), SW_HIDE);
    
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIME), TRUE);
    
	if (dat->hwndIEView) {
		ShowWindow (GetDlgItem(hwndDlg, IDC_LOG), SW_HIDE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_LOG), FALSE);
		ShowWindow (GetDlgItem(hwndDlg, IDC_MESSAGE), SW_SHOW);
        //if(DBGetContactSettingDword(NULL, "IEVIEW", "TemplatesFlags", 0) & 0x01)
        //    EnableWindow(GetDlgItem(hwndDlg, IDC_TIME), FALSE);
            
	} else
		ShowMultipleControls(hwndDlg, sendControls, sizeof(sendControls) / sizeof(sendControls[0]), SW_SHOW);
    ShowMultipleControls(hwndDlg, errorControls, sizeof(errorControls) / sizeof(errorControls[0]), dat->dwFlags & MWF_ERRORSTATE ? SW_SHOW : SW_HIDE);

    if(!dat->SendFormat)
        ShowMultipleControls(hwndDlg, &formatControls[1], 4, SW_HIDE);
    
// smileybutton stuff...
    ConfigureSmileyButton(hwndDlg, dat);
    
    if(dat->pContainer->hwndActive == hwndDlg)
        UpdateReadChars(hwndDlg, dat);

    SetDlgItemTextA(hwndDlg, IDOK, "Send");
    SetDlgItemText(hwndDlg, IDC_STATICTEXT, TranslateT("A message failed to send successfully."));

    DM_RecalcPictureSize(hwndDlg, dat);
    GetAvatarVisibility(hwndDlg, dat);
    
    ShowWindow(GetDlgItem(hwndDlg,IDC_CONTACTPIC), dat->showPic ? SW_SHOW : SW_HIDE);

    ShowWindow(GetDlgItem(hwndDlg, IDC_SPLITTER), SW_SHOW);
    ShowWindow(GetDlgItem(hwndDlg, IDOK), showToolbar ? SW_SHOW : SW_HIDE);

    ShowWindow(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), (dat->sendMode & SMODE_MULTIPLE) ? SW_SHOW : SW_HIDE);
    
    EnableSendButton(hwndDlg, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE)) != 0);
    SendMessage(hwndDlg, DM_UPDATETITLE, 0, 1);
    SendMessage(hwndDlg, WM_SIZE, 0, 0);

    EnableWindow(GetDlgItem(hwndDlg, IDC_CONTACTPIC), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PANELPIC), FALSE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_TOGGLESIDEBAR), myGlobals.m_SideBarEnabled ? SW_SHOW : SW_HIDE);

    // info panel stuff
    if(!(dat->dwFlags & MWF_ERRORSTATE)) {
        ShowMultipleControls(hwndDlg, infoPanelControls, 8, dat->dwFlagsEx & MWF_SHOW_INFOPANEL ? SW_SHOW : SW_HIDE);
        if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL)
            ConfigurePanel(hwndDlg, dat);
    }
}

UINT NcCalcRichEditFrame(HWND hwnd, struct MessageWindowData *mwdat, UINT skinID, UINT msg, WPARAM wParam, LPARAM lParam, WNDPROC OldWndProc)
{
    LRESULT orig;
    NCCALCSIZE_PARAMS *nccp = (NCCALCSIZE_PARAMS *)lParam;
    BOOL bReturn = FALSE;

    if(myGlobals.g_DisableScrollbars) {
        SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_VSCROLL);
        EnableScrollBar(hwnd, SB_VERT, ESB_DISABLE_BOTH);
        ShowScrollBar(hwnd, SB_VERT, FALSE);
    }
    orig = CallWindowProc(OldWndProc, hwnd, msg, wParam, lParam);

    if(mwdat && mwdat->pContainer->bSkinned && !mwdat->bFlatMsgLog) {
        StatusItems_t *item = &StatusItems[skinID];
        if(!item->IGNORED) {
            /*
            nccp->rgrc[0].left += item->MARGIN_LEFT;
            nccp->rgrc[0].right -= item->MARGIN_RIGHT;
            nccp->rgrc[0].bottom -= item->MARGIN_BOTTOM;
            nccp->rgrc[0].top += item->MARGIN_TOP;*/
            return WVR_REDRAW;
        }
    }
    if(mwdat && mwdat->hTheme && wParam && pfnGetThemeBackgroundContentRect) {
        RECT rcClient;
        HDC hdc = GetDC(GetParent(hwnd));

        if(pfnGetThemeBackgroundContentRect(mwdat->hTheme, hdc, 1, 1, &nccp->rgrc[0], &rcClient) == S_OK) {
            if(EqualRect(&rcClient, &nccp->rgrc[0]))
                InflateRect(&rcClient, -1, -1);
            CopyRect(&nccp->rgrc[0], &rcClient);
            bReturn = TRUE;
        }
        ReleaseDC(GetParent(hwnd), hdc);
        if(bReturn)
            return WVR_REDRAW;
        else
            return orig;
    }
    if(mwdat && (mwdat->sendMode & SMODE_MULTIPLE || mwdat->sendMode & SMODE_CONTAINER) && skinID == ID_EXTBKINPUTAREA) {
        InflateRect(&nccp->rgrc[0], -1, -1);
        return WVR_REDRAW;
    }
    return orig;
}

/*
 * process WM_NCPAINT for the rich edit control. Draw a visual style border and avoid classic static edge / client edge
 * may also draw a skin item around the rich edit control.
 */

UINT DrawRichEditFrame(HWND hwnd, struct MessageWindowData *mwdat, UINT skinID, UINT msg, WPARAM wParam, LPARAM lParam, WNDPROC OldWndProc)
{
	StatusItems_t *item = &StatusItems[skinID];
	LRESULT result = 0;
    BOOL isMultipleReason;

    result = CallWindowProc(OldWndProc, hwnd, msg, wParam, lParam);			// do default processing (otherwise, NO scrollbar as it is painted in NC_PAINT)
    if(!mwdat)
        return result;

	// isMultipleReason means that the msg window is in multisend mode (draw attention by rendering a red border around the text input field)
    isMultipleReason = ((skinID == ID_EXTBKINPUTAREA) && (mwdat->sendMode & SMODE_MULTIPLE || mwdat->sendMode & SMODE_CONTAINER));

	if(isMultipleReason || ((mwdat && mwdat->hTheme) || (mwdat && mwdat->pContainer->bSkinned && !item->IGNORED && !mwdat->bFlatMsgLog))) {
		HDC hdc = GetWindowDC(hwnd);
		RECT rcWindow;
		POINT pt;
		LONG left_off, top_off, right_off, bottom_off;
		//HDC dcMem;
		//HBITMAP hbm, hbmOld;
        LONG dwStyle = GetWindowLong(hwnd, GWL_STYLE);
        LONG dwExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        
		GetWindowRect(hwnd, &rcWindow);
		pt.x = pt.y = 0;
		ClientToScreen(hwnd, &pt);
		left_off = pt.x - rcWindow.left;
        if(dwStyle & WS_VSCROLL && dwExStyle & WS_EX_RTLREADING)
            left_off -= myGlobals.ncm.iScrollWidth;
		top_off = pt.y - rcWindow.top;
		
		if(mwdat->pContainer->bSkinned && !item->IGNORED) {
			right_off = item->MARGIN_RIGHT;
			bottom_off = item->MARGIN_BOTTOM;
		} else {
			right_off = left_off;
			bottom_off = top_off;
		}

		rcWindow.right -= rcWindow.left;
		rcWindow.bottom -= rcWindow.top;
		rcWindow.left = rcWindow.top = 0;

		// clip the client area from the dc to avoid flickering
        ExcludeClipRect(hdc, left_off, top_off, rcWindow.right - right_off, rcWindow.bottom - bottom_off);

        if(mwdat->pContainer->bSkinned && !item->IGNORED) {					// this drawing is done by the parent actually 
            ReleaseDC(hwnd, hdc);											// (skin underlays).
            return result;
		}
		else if(pfnDrawThemeBackground) {									// XP visual styles support
            if(isMultipleReason) {
                HBRUSH br = CreateSolidBrush(RGB(255, 130, 130));
                FillRect(hdc, &rcWindow, br);
                DeleteObject(br);
            }
            else
                pfnDrawThemeBackground(mwdat->hTheme, hdc, 1, 1, &rcWindow, &rcWindow);
        }
		ReleaseDC(hwnd, hdc);
		return result;
	}
	return result;
}

static LRESULT CALLBACK MessageLogSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct MessageWindowData *mwdat = (struct MessageWindowData *)GetWindowLong(GetParent(hwnd), GWL_USERDATA);

    switch(msg) {
        case WM_KILLFOCUS:
            {
                CHARRANGE cr;
                SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM)&cr);
                if(cr.cpMax != cr.cpMin) {
                    cr.cpMin = cr.cpMax;
                    SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM)&cr);
                }
                break;
            }
        case WM_CHAR:
            if(wParam == 0x03 && (GetKeyState(VK_CONTROL) & 0x8000))
                return SendMessage(hwnd, WM_COPY, 0, 0);
            break;
        case WM_COPY:
            {
                return(DM_WMCopyHandler(hwnd, OldMessageLogProc, wParam, lParam));
            }
        case WM_NCCALCSIZE:
            return(NcCalcRichEditFrame(hwnd, mwdat, ID_EXTBKHISTORY, msg, wParam, lParam, OldMessageLogProc));
		case WM_NCPAINT:
			return(DrawRichEditFrame(hwnd, mwdat, ID_EXTBKHISTORY, msg, wParam, lParam, OldMessageLogProc));
	}
	return CallWindowProc(OldMessageLogProc, hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK MessageEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LONG lastEnterTime = GetWindowLong(hwnd, GWL_USERDATA);
    HWND hwndParent = GetParent(hwnd);
    struct MessageWindowData *mwdat = (struct MessageWindowData *)GetWindowLong(hwndParent, GWL_USERDATA);

    switch (msg) {
		case WM_NCCALCSIZE:
            return(NcCalcRichEditFrame(hwnd, mwdat, ID_EXTBKINPUTAREA, msg, wParam, lParam, OldMessageEditProc));
		case WM_NCPAINT:
			return(DrawRichEditFrame(hwnd, mwdat, ID_EXTBKINPUTAREA, msg, wParam, lParam, OldMessageEditProc));
		case WM_DROPFILES:
			SendMessage(hwndParent,WM_DROPFILES,(WPARAM)wParam,(LPARAM)lParam);
			break;
        case WM_CHAR:
        {
            BOOL isCtrl = GetKeyState(VK_CONTROL) & 0x8000;
            BOOL isShift = GetKeyState(VK_SHIFT) & 0x8000;
            BOOL isAlt = GetKeyState(VK_MENU) & 0x8000;
            
            if (wParam == 0x0d && isCtrl && myGlobals.m_MathModAvail) {
                TCHAR toInsert[100];
                BYTE keyState[256];
                int i;
                int iLen = _tcslen(myGlobals.m_MathModStartDelimiter);
                ZeroMemory(keyState, 256);
                _tcsncpy(toInsert, myGlobals.m_MathModStartDelimiter, 30);
                _tcsncat(toInsert, myGlobals.m_MathModStartDelimiter, 30);
                SendMessage(hwnd, EM_REPLACESEL, TRUE, (LPARAM)toInsert);
                SetKeyboardState(keyState);
                for(i = 0; i < iLen; i++)
                    SendMessage(hwnd, WM_KEYDOWN, mwdat->dwFlags & MWF_LOG_RTL ? VK_RIGHT : VK_LEFT, 0);
                return 0;
            }
            if(isCtrl && !isAlt) {
                switch(wParam) {
                    case 0x02:               // bold
                        if(mwdat->SendFormat) {
                            SendMessage(hwndParent, WM_COMMAND, MAKELONG(IDC_FONTBOLD, IDC_MESSAGE), 0);
                        }
                        return 0;
                    case 0x09:
                        if(mwdat->SendFormat) {
                            SendMessage(hwndParent, WM_COMMAND, MAKELONG(IDC_FONTITALIC, IDC_MESSAGE), 0);
                        }
                        return 0;
                    case 21:
                        if(mwdat->SendFormat) {
                            SendMessage(hwndParent, WM_COMMAND, MAKELONG(IDC_FONTUNDERLINE, IDC_MESSAGE), 0);
                        }
                        return 0;
                    case 25:
                        PostMessage(hwndParent, DM_SPLITTEREMERGENCY, 0, 0);
                        return 0;
                    case 0x0b:
                        SetWindowText(hwnd, _T(""));
                        return 0;
                }
                break;
            }
            break;
        }
        case WM_MOUSEWHEEL:
        {
            LRESULT result = DM_MouseWheelHandler(hwnd, hwndParent, mwdat, wParam, lParam);

            if(result == 0)
                return 0;
            break;
        }
		case WM_PASTE:
		case EM_PASTESPECIAL:
		{
            if(OpenClipboard(hwnd)) {
                HANDLE hClip = GetClipboardData(CF_TEXT);
                if(hClip) {
                    if(lstrlenA((char *)hClip) > mwdat->nMax) {
                        TCHAR szBuffer[512];
                        if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "autosplit", 0))
                            _sntprintf(szBuffer, 512, TranslateT("WARNING: The message you are trying to paste exceeds the message size limit for the active protocol. It will be sent in chunks of max %d characters"), mwdat->nMax - 10);
                        else
                            _sntprintf(szBuffer, 512, TranslateT("The message you are trying to paste exceeds the message size limit for the active protocol. Only the first %d characters will be sent."), mwdat->nMax);
                        SendMessage(hwndParent, DM_ACTIVATETOOLTIP, IDC_MESSAGE, (LPARAM)szBuffer);
                    }
                }
                CloseClipboard();
            }
            return CallWindowProc(OldMessageEditProc, hwnd, msg, wParam, lParam);
		} 
        case WM_KEYUP:
            break;
        case WM_KEYDOWN:
        {
            BOOL isCtrl = GetKeyState(VK_CONTROL) & 0x8000;
            BOOL isShift = GetKeyState(VK_SHIFT) & 0x8000;
            BOOL isAlt = GetKeyState(VK_MENU) & 0x8000;
            
            if(wParam == VK_RETURN) {
                if (isShift) {
                    if(myGlobals.m_SendOnShiftEnter) {
                        PostMessage(hwndParent, WM_COMMAND, IDOK, 0);
                        return 0;
                    }
                    else
                        break;
                }
                if ((isCtrl && !isShift) ^ (0 != myGlobals.m_SendOnEnter)) {
                    PostMessage(hwndParent, WM_COMMAND, IDOK, 0);
                    return 0;
                }
                if(myGlobals.m_SendOnEnter || myGlobals.m_SendOnDblEnter) {
                    if(isCtrl)
                        break;
                    else {
                        if (myGlobals.m_SendOnDblEnter) {
                            if (lastEnterTime + 2 < time(NULL)) {
                                lastEnterTime = time(NULL);
                                SetWindowLong(hwnd, GWL_USERDATA, lastEnterTime);
                                break;
                            }
                            else {
                                SendMessage(hwnd, WM_KEYDOWN, VK_BACK, 0);
                                SendMessage(hwnd, WM_KEYUP, VK_BACK, 0);
                                PostMessage(hwndParent, WM_COMMAND, IDOK, 0);
                                return 0;
                            }
                        }
                        PostMessage(hwndParent, WM_COMMAND, IDOK, 0);
                        return 0;
                    }
                }
                else 
                    break;
            }
            else
                SetWindowLong(hwnd, GWL_USERDATA, 0);
            
            if (isCtrl && !isAlt && !isShift) {
                if (!isShift && (wParam == VK_UP || wParam == VK_DOWN)) {          // input history scrolling (ctrl-up / down)
                    SETTEXTEX stx = {ST_DEFAULT,CP_UTF8};

                    SetWindowLong(hwnd, GWL_USERDATA, 0);
                    if(mwdat) {
                        if(mwdat->history != NULL && mwdat->history[0].szText != NULL) {      // at least one entry needs to be alloced, otherwise we get a nice infinite loop ;)
                            if(mwdat->dwFlags & MWF_NEEDHISTORYSAVE) {
                                mwdat->iHistoryCurrent = mwdat->iHistoryTop;
                                if(GetWindowTextLengthA(hwnd) > 0)
                                    SaveInputHistory(hwndParent, mwdat, (WPARAM)mwdat->iHistorySize, 0);
                                else
                                    mwdat->history[mwdat->iHistorySize].szText[0] = (TCHAR)'\0';
                            }
                            if(wParam == VK_UP) {
                                if(mwdat->iHistoryCurrent == 0)
                                    break;
                                mwdat->iHistoryCurrent--;
                            }
                            else {
                                mwdat->iHistoryCurrent++;
                                if(mwdat->iHistoryCurrent > mwdat->iHistoryTop)
                                    mwdat->iHistoryCurrent = mwdat->iHistoryTop;
                            }
                            if(mwdat->iHistoryCurrent == mwdat->iHistoryTop) {
                                if(mwdat->history[mwdat->iHistorySize].szText != NULL) {            // replace the temp buffer 
                                    SetWindowText(hwnd, _T(""));
                                    SendMessage(hwnd, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM) mwdat->history[mwdat->iHistorySize].szText);
                                    SendMessage(hwnd, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
                                }
                            }
                            else {
                                if(mwdat->history[mwdat->iHistoryCurrent].szText != NULL) {
                                    SetWindowText(hwnd, _T(""));
                                    SendMessage(hwnd, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM) mwdat->history[mwdat->iHistoryCurrent].szText);
                                    SendMessage(hwnd, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
                                }
                                else
                                    SetWindowText(hwnd, _T(""));
                            }
                            SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), EN_CHANGE), (LPARAM) hwnd);
                            mwdat->dwFlags &= ~MWF_NEEDHISTORYSAVE;
                        }
                        else {
                            if(mwdat->pContainer->hwndStatus)
                                SetWindowTextA(mwdat->pContainer->hwndStatus, "The input history is empty.");
                        }
                    }
                    return 0;
                }
            }
            if(isCtrl && isAlt && !isShift) {
                switch (wParam) {
                    case VK_UP:
                    case VK_DOWN:
                    case VK_PRIOR:
                    case VK_NEXT:
                    case VK_HOME:
                    case VK_END:
                    {
                        WPARAM wp = 0;

                        SetWindowLong(hwnd, GWL_USERDATA, 0);
                        if (wParam == VK_UP)
                            wp = MAKEWPARAM(SB_LINEUP, 0);
                        else if (wParam == VK_PRIOR)
                            wp = MAKEWPARAM(SB_PAGEUP, 0);
                        else if (wParam == VK_NEXT)
                            wp = MAKEWPARAM(SB_PAGEDOWN, 0);
                        else if (wParam == VK_HOME)
                            wp = MAKEWPARAM(SB_TOP, 0);
                        else if (wParam == VK_END) {
                            DM_ScrollToBottom(hwndParent, mwdat, 0, 0);
                            return 0;
                        } else if (wParam == VK_DOWN)
                            wp = MAKEWPARAM(SB_LINEDOWN, 0);

                        if(mwdat->hwndIEView == 0)
                            SendMessage(GetDlgItem(hwndParent, IDC_LOG), WM_VSCROLL, wp, 0);
                        else
                            SendMessage(mwdat->hwndIWebBrowserControl, WM_VSCROLL, wp, 0);
                        return 0;
                    }
                }
            }
            if (wParam == VK_RETURN)
                break;
        }
        case WM_SYSCHAR:
        {
            HWND hwndDlg = hwndParent;
            BOOL isCtrl = GetKeyState(VK_CONTROL) & 0x8000;
            BOOL isShift = GetKeyState(VK_SHIFT) & 0x8000;
            BOOL isAlt = GetKeyState(VK_MENU) & 0x8000;
            
            if(isAlt && !isShift && !isCtrl) {
                switch (LOBYTE(VkKeyScan((TCHAR)wParam))) {
                    case 'S':
                        if (!(GetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_STYLE) & ES_READONLY)) {
                            PostMessage(hwndDlg, WM_COMMAND, IDOK, 0);
                            return 0;
                        }
                    case'E':
                        SendMessage(hwndDlg, WM_COMMAND, IDC_SMILEYBTN, 0);
                        return 0;
                    case 'H':
                        SendMessage(hwndDlg, WM_COMMAND, IDC_HISTORY, 0);
                        return 0;
                    case 'Q':
                        SendMessage(hwndDlg, WM_COMMAND, IDC_QUOTE, 0);
                        return 0;
					case 'F':
						CallService(MS_FILE_SENDFILE, (WPARAM)mwdat->hContact, 0);
						return 0;
                    case 'P':
                        SendMessage(hwndDlg, WM_COMMAND, IDC_PIC, 0);
                        return 0;
                    case 'D':
                        SendMessage(hwndDlg, WM_COMMAND, IDC_PROTOCOL, 0);
                        return 0;
                    case 'U':
                        SendMessage(hwndDlg, WM_COMMAND, MAKELONG(IDC_NAME, BN_CLICKED), 0);
                        return 0;
                    case 'L':
                        SendMessage(hwndDlg, WM_COMMAND, IDC_TIME, 0);
                        return 0;
                    case 'T':
                        SendMessage(hwndDlg, WM_COMMAND, IDC_TOGGLETOOLBAR, 0);
                        return 0;
                    case 'I':
                        if(mwdat->dwFlags & MWF_ERRORSTATE)
                            return 0;
                        mwdat->dwFlagsEx ^= MWF_SHOW_INFOPANEL;
                        ShowHideInfoPanel(hwndParent, mwdat);
                        return 0;
                    case 'B':
                        MsgWindowMenuHandler(hwndParent, mwdat, ID_LOGMENU_ACTIVATERTL, MENU_LOGMENU);
                        return 0;
                    case 'M':
                        mwdat->sendMode ^= SMODE_MULTIPLE;
                        if(mwdat->sendMode & SMODE_MULTIPLE) {
							HWND hwndClist = DM_CreateClist(hwndParent, mwdat);
                        }
						else {
							if(IsWindow(GetDlgItem(hwndParent, IDC_CLIST)))
								DestroyWindow(GetDlgItem(hwndParent, IDC_CLIST));
						}
                        SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE);
                        SendMessage(hwndDlg, WM_SIZE, 0, 0);
                        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW | RDW_ERASE);
                        DM_ScrollToBottom(hwndParent, mwdat, 0, 0);
                        ShowWindow(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), (mwdat->sendMode & SMODE_MULTIPLE) ? SW_SHOW : SW_HIDE);
                        ShowWindow(GetDlgItem(hwndDlg, IDC_CLIST), (mwdat->sendMode & SMODE_MULTIPLE) ? SW_SHOW : SW_HIDE);
                        if(mwdat->sendMode & SMODE_MULTIPLE)
                            SetFocus(GetDlgItem(hwndDlg, IDC_CLIST));
                        else
                            SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                        return 0;
                    default:
                        break;
                }
                if((wParam >= '0' && wParam <= '9') && (GetKeyState(VK_MENU) & 0x8000)) {       // ALT-1 -> ALT-0 direct tab selection
                    BYTE bChar = (BYTE)wParam;
                    int iIndex;

                    if(bChar == '0')
                        iIndex = 10;
                    else
                        iIndex = bChar - (BYTE)'0';
                    SendMessage(mwdat->pContainer->hwnd, DM_SELECTTAB, DM_SELECT_BY_INDEX, (LPARAM)iIndex);
                    return 0;
                }
            }
            break;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_KILLFOCUS:
            break;
        case WM_INPUTLANGCHANGEREQUEST: 
        {
            /*
            if (myGlobals.m_AutoLocaleSupport) {
                SendMessage(hwndParent, DM_SETLOCALE, wParam, lParam);
                PostMessage(hwndParent, DM_SAVELOCALE, 0, 0);
            }
            break;
            */
            return DefWindowProc(hwnd, WM_INPUTLANGCHANGEREQUEST, wParam, lParam);
        }
        case WM_INPUTLANGCHANGE:
            if (myGlobals.m_AutoLocaleSupport && GetFocus() == hwnd && mwdat->pContainer->hwndActive == hwndParent && GetForegroundWindow() == mwdat->pContainer->hwnd && GetActiveWindow() == mwdat->pContainer->hwnd) {
                DM_SaveLocale(hwndParent, mwdat, wParam, lParam);
                SendMessage(hwnd, EM_SETLANGOPTIONS, 0, (LPARAM) SendMessage(hwnd, EM_GETLANGOPTIONS, 0, 0) & ~IMF_AUTOKEYBOARD);
            }
            return 1;
        case WM_ERASEBKGND:
        {
			if(mwdat->pContainer->bSkinned) {
				return 0;
			}
            return 1;
        }
        /*
         * sent by smileyadd when the smiley selection window dies
         * just grab the focus :)
         */
        case WM_USER + 100:
            SetFocus(hwnd);
            break;
    }
    return CallWindowProc(OldMessageEditProc, hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK AvatarSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_ERASEBKGND:
            return TRUE;
    }
    return CallWindowProc(OldAvatarWndProc, hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK MsgIndicatorSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(GetParent(hwnd), GWL_USERDATA);

    switch(msg) {
        case WM_PAINT:
        {
            RECT rc;
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);

            GetClientRect(hwnd, &rc);

            if(dat && dat->pContainer->bSkinned) {
                HPEN hPenOld = SelectObject(dc, myGlobals.g_SkinLightShadowPen);
                Rectangle(dc, 0, 0, rc.right, 2);
                //SkinDrawBG(hwnd, dat->pContainer->hwnd, dat->pContainer, &rc, dc);
                SelectObject(dc, hPenOld);
            }
            else if(myGlobals.m_visualMessageSizeIndicator)
                FillRect(dc, &rc, GetSysColorBrush(COLOR_3DHIGHLIGHT));

            if(myGlobals.m_visualMessageSizeIndicator) {
                HBRUSH br = CreateSolidBrush(RGB(0, 255, 0));
                HBRUSH brOld = SelectObject(dc, br);
                if(!myGlobals.m_autoSplit) {
                    float fMax = (float)dat->nMax;
                    float uPercent = (float)dat->textLen / ((fMax / (float)100.0) ? (fMax / (float)100.0) : (float)75.0);
                    float fx = ((float)rc.right / (float)100.0) * uPercent;

                    rc.right = (LONG)fx;
                    FillRect(dc, &rc, br);
                }
                else {
                    float baselen = (dat->textLen <= dat->nMax) ? (float)dat->textLen : (float)dat->nMax;
                    float fMax = (float)dat->nMax;
                    float uPercent = baselen / ((fMax / (float)100.0) ? (fMax / (float)100.0) : (float)75.0);
                    float fx;
                    LONG  width = rc.right;
                    if(dat->textLen >= dat->nMax)
                        rc.right = rc.right / 3;
                    fx = ((float)rc.right / (float)100.0) * uPercent;
                    rc.right = (LONG)fx;
                    FillRect(dc, &rc, br);
                    if(dat->textLen >= dat->nMax) {
                        SelectObject(dc, brOld);
                        DeleteObject(br);
                        br = CreateSolidBrush(RGB(255, 0, 0));
                        brOld = SelectObject(dc, br);
                        rc.left = width / 3;
                        rc.right = width;
                        uPercent = (float)dat->textLen / (float)200.0;
                        fx = ((float)(rc.right - rc.left) / (float)100.0) * uPercent;
                        rc.right = rc.left + (LONG)fx;
                        FillRect(dc, &rc, br);
                    }
                }
                SelectObject(dc, brOld);
                DeleteObject(br);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return CallWindowProc(OldSplitterProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK SplitterSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndParent = GetParent(hwnd);
    struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwndParent, GWL_USERDATA);
    
    switch (msg) {
        case WM_NCHITTEST:
            return HTCLIENT;
        case WM_SETCURSOR:
            {
                RECT rc;
                GetClientRect(hwnd, &rc);
                SetCursor(rc.right > rc.bottom ? myGlobals.hCurSplitNS : myGlobals.hCurSplitWE);
                return TRUE;
            }
        case WM_LBUTTONDOWN:
			{
				if(hwnd == GetDlgItem(hwndParent, IDC_SPLITTER) || hwnd == GetDlgItem(hwndParent, IDC_SPLITTERY)) {
					RECT rc;

					struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwndParent, GWL_USERDATA);
					if(dat) {
						GetClientRect(hwnd, &rc);
						dat->savedSplitter = rc.right > rc.bottom ? (short) HIWORD(GetMessagePos()) + rc.bottom / 2 : (short) LOWORD(GetMessagePos()) + rc.right / 2;
					}
				}
				SetCapture(hwnd);
				return 0;
			}
        case WM_MOUSEMOVE:
            if (GetCapture() == hwnd) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                SendMessage(hwndParent, DM_SPLITTERMOVED, rc.right > rc.bottom ? (short) HIWORD(GetMessagePos()) + rc.bottom / 2 : (short) LOWORD(GetMessagePos()) + rc.right / 2, (LPARAM) hwnd);
                if(dat->pContainer->bSkinned)
                    InvalidateRect(hwndParent, NULL, FALSE);
            }
            return 0;
        case WM_PAINT:
            {
                struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(GetParent(hwnd), GWL_USERDATA);
                RECT rc;
                PAINTSTRUCT ps;
                HDC dc = BeginPaint(hwnd, &ps);

                if(dat && dat->pContainer->bSkinned) {
                    GetClientRect(hwnd, &rc);
                    SkinDrawBG(hwnd, dat->pContainer->hwnd, dat->pContainer, &rc, dc);
                }
                else {
                    GetClientRect(hwnd, &rc);
                    FillRect(dc, &rc, GetSysColorBrush(COLOR_3DFACE));
                }
                EndPaint(hwnd, &ps);
                return 0;
            }
        case WM_NCPAINT:
            {
                if(splitterEdges == -1)
                    splitterEdges = DBGetContactSettingByte(NULL, SRMSGMOD_T, "splitteredges", 1);

                if(dat && dat->pContainer->bSkinned && splitterEdges > 0) {
                    HDC dc = GetWindowDC(hwnd);
                    POINT pt;
                    RECT rc;
                    HPEN hPenOld;
                    
                    GetWindowRect(hwnd, &rc);
                    if(rc.right - rc.left > rc.bottom - rc.top) {
                        MoveToEx(dc, 0, 0, &pt);
                        hPenOld = SelectObject(dc, myGlobals.g_SkinDarkShadowPen);
                        LineTo(dc, rc.right - rc.left, 0);
                        MoveToEx(dc, rc.right - rc.left, 1, &pt);
                        SelectObject(dc, myGlobals.g_SkinLightShadowPen);
                        LineTo(dc, -1, 1);
                        SelectObject(dc, hPenOld);
                        ReleaseDC(hwnd, dc);
                    }
                    else {
                        MoveToEx(dc, 0, 0, &pt);
                        hPenOld = SelectObject(dc, myGlobals.g_SkinDarkShadowPen);
                        LineTo(dc, 0, rc.bottom - rc.top);
                        MoveToEx(dc, 1, rc.bottom - rc.top, &pt);
                        SelectObject(dc, myGlobals.g_SkinLightShadowPen);
                        LineTo(dc, 1, -1);
                        SelectObject(dc, hPenOld);
                        ReleaseDC(hwnd, dc);
                    }
                    return 0;
                }
                break;
            }
            case WM_LBUTTONUP:
            {
                HWND hwndCapture = GetCapture();

                ReleaseCapture();
                DM_ScrollToBottom(hwndParent, dat, 0, 1);
                if(dat && dat->bType == SESSIONTYPE_IM && hwnd == GetDlgItem(hwndParent, IDC_PANELSPLITTER)) {
                    dat->panelWidth = -1;
                    SendMessage(hwndParent, WM_SIZE, 0, 0);
                }
                else if((dat && dat->bType == SESSIONTYPE_IM && hwnd == GetDlgItem(hwndParent, IDC_SPLITTER)) ||
                        (dat && dat->bType == SESSIONTYPE_CHAT && hwnd == GetDlgItem(hwndParent, IDC_SPLITTERY))) {
                    RECT rc;
                    POINT pt;
                    int selection;
                    HMENU hMenu = GetSubMenu(dat->pContainer->hMenuContext, 12);
                    LONG messagePos = GetMessagePos();

                    GetClientRect(hwnd, &rc);
                    if(hwndCapture != hwnd || dat->savedSplitter == (rc.right > rc.bottom ? (short) HIWORD(messagePos) + rc.bottom / 2 : (short) LOWORD(messagePos) + rc.right / 2))
                        break;
                    GetCursorPos(&pt);
                    selection = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndParent, NULL);
                    switch(selection) {
                        case ID_SPLITTERCONTEXT_SAVEFORTHISCONTACTONLY:
                        {
                            HWND hwndParent = GetParent(hwnd);

                            dat->dwFlagsEx |= MWF_SHOW_SPLITTEROVERRIDE;
                            DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "splitoverride", 1);
                            if(dat->bType == SESSIONTYPE_IM)
                                SaveSplitter(hwndParent, dat);
                            break;
                        }
                        case ID_SPLITTERCONTEXT_SETPOSITIONFORTHISSESSION:
                            break;
                        case ID_SPLITTERCONTEXT_SAVEGLOBALFORALLSESSIONS:
                        {
                            RECT rcWin;

                            GetWindowRect(hwndParent, &rcWin);
                            if(dat->bType == SESSIONTYPE_IM) {
                                dat->dwFlagsEx &= ~(MWF_SHOW_SPLITTEROVERRIDE);
                                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "splitoverride", 0);
                                WindowList_Broadcast(hMessageWindowList, DM_SPLITTERMOVEDGLOBAL, 
                                                     rcWin.bottom - HIWORD(messagePos), rc.bottom);
                            }
                            else {
                                SM_BroadcastMessage(NULL, DM_SAVESIZE, 0, 0, 0);
                                SM_BroadcastMessage(NULL, DM_SPLITTERMOVEDGLOBAL, rcWin.bottom - (short)HIWORD(messagePos) + rc.bottom / 2, (LPARAM)rc.bottom, 0);
                                SM_BroadcastMessage(NULL, WM_SIZE, 0, 0, 1);
                                DBWriteContactSettingWord(NULL, "Chat", "splitY", (WORD)g_Settings.iSplitterY);
                            }

                            break;
                        }
                        default:
                            SendMessage(hwndParent, DM_SPLITTERMOVED, dat->savedSplitter, (LPARAM) hwnd);
                            DM_ScrollToBottom(hwndParent, dat, 0, 1);
                            break;
                    }
                }
                return 0;
            }
    }
    return CallWindowProc(OldSplitterProc, hwnd, msg, wParam, lParam);
}

/*
 *  resizer proc for the "new" layout.
 */


static int MessageDialogResize(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL * urc)
{
    struct MessageWindowData *dat = (struct MessageWindowData *) lParam;
    int iClistOffset = 0;
    RECT rc, rcButton;
    static int uinWidth, msgTop = 0, msgBottom = 0, not_on_list = 0;
    
    int showToolbar = dat->pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;
    int panelHeight = dat->panelHeight + 1;
	int panelWidth = (dat->panelWidth != -1 ? dat->panelWidth + 2: 0);
    int s_offset = 0;
    
    GetClientRect(GetDlgItem(hwndDlg, IDC_LOG), &rc);
    GetClientRect(GetDlgItem(hwndDlg, IDC_PROTOCOL), &rcButton);

    iClistOffset = rc.bottom;
    if(dat->panelStatusCX == 0)
        dat->panelStatusCX = 80;
        
    if (!showToolbar) {
        int i;
        for (i = 0; i < sizeof(buttonLineControlsNew) / sizeof(buttonLineControlsNew[0]); i++)
            if (buttonLineControlsNew[i] == urc->wId)
                OffsetRect(&urc->rcItem, 0, -(dat->splitterY +10));
    }

    s_offset = 1;

    switch (urc->wId) {
        case IDC_NAME:
            urc->rcItem.top -= (dat->splitterY - dat->originalSplitterY + s_offset);
            urc->rcItem.bottom -= (dat->splitterY - dat->originalSplitterY + s_offset);
            urc->rcItem.bottom++; urc->rcItem.right++;
            if(dat->controlsHidden & TOOLBAR_PROTO_HIDDEN)
                OffsetRect(&urc->rcItem, -(rcButton.right + 2), 0);
            return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
        case IDC_SMILEYBTN:
        case IDC_FONTBOLD:
        case IDC_FONTITALIC:
        case IDC_FONTUNDERLINE:
        case IDC_FONTFACE:
        case IDC_FONTCOLOR:
            urc->rcItem.top -= (dat->splitterY - dat->originalSplitterY + s_offset);
            urc->rcItem.bottom -= (dat->splitterY - dat->originalSplitterY + s_offset);
            urc->rcItem.bottom++; urc->rcItem.right++;
            if(!dat->doSmileys)
                OffsetRect(&urc->rcItem, -22, 0);
            if(dat->controlsHidden & TOOLBAR_PROTO_HIDDEN)
                OffsetRect(&urc->rcItem, -(rcButton.right + 2), 0);
            return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
        case IDC_TOGGLENOTES:
            return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;
        case IDC_PANELSPLITTER:
            urc->rcItem.bottom = panelHeight;
            urc->rcItem.top = panelHeight - 2;
            return RD_ANCHORX_WIDTH | RD_ANCHORY_TOP;
        case IDC_NOTES:
            urc->rcItem.bottom = panelHeight - 4;
            urc->rcItem.right = urc->dlgNewSize.cx - panelWidth;
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_TOP;
        case IDC_RETRY:
        case IDC_CANCELSEND:
        case IDC_MSGSENDLATER:
        case IDC_STATICTEXT:
        case IDC_STATICERRORICON:
            return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;
        case IDC_PROTOCOL:
        case IDC_PIC:
        case IDC_HISTORY:
        case IDC_TIME:
        case IDC_QUOTE:
        case IDC_SAVE:
        //case IDC_INFOPANELMENU:
            if(urc->wId != IDC_PROTOCOL) // && urc->wId != IDC_PROTOMENU && urc->wId != IDC_INFOPANELMENU)
                OffsetRect(&urc->rcItem, 12, 0);
            urc->rcItem.top -= (dat->splitterY - dat->originalSplitterY + s_offset);
            urc->rcItem.bottom -= (dat->splitterY - dat->originalSplitterY + s_offset);
            urc->rcItem.bottom++; urc->rcItem.right++;
            if (urc->wId == IDC_PROTOCOL) // || urc->wId == IDC_PROTOMENU || urc->wId == IDC_INFOPANELMENU)
                return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
            if (showToolbar && !(dat->controlsHidden & TOOLBAR_SEND_HIDDEN)) {
                urc->rcItem.left -= 38;
                urc->rcItem.right -= 38;
            } else if(showToolbar)
				OffsetRect(&urc->rcItem, 14, 0);

            if(dat->fMustOffset)
                OffsetRect(&urc->rcItem, -(dat->pic.cx + 2), 0);
            return RD_ANCHORX_RIGHT | RD_ANCHORY_BOTTOM;
        case IDC_ADD:
        case IDC_CANCELADD:
            if(urc->wId == IDC_ADD && dat->bNotOnList) {
                RECT rc;
                GetWindowRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc);
                if(rc.bottom - rc.top > 48) {
                    OffsetRect(&urc->rcItem, 24, -24);
                }
            }
            if(dat->showPic)
                OffsetRect(&urc->rcItem, -(dat->pic.cx+2), 0);
            urc->rcItem.bottom++; urc->rcItem.right++;
            return RD_ANCHORX_RIGHT | RD_ANCHORY_BOTTOM;
        case IDC_LOG:
            if(dat->dwFlags & MWF_ERRORSTATE && !(dat->dwFlagsEx & MWF_SHOW_INFOPANEL))
                urc->rcItem.top += 38;
            if (dat->sendMode & SMODE_MULTIPLE)
                urc->rcItem.right -= (dat->multiSplitterX + 3);
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if (!showToolbar)
                urc->rcItem.bottom += 23;
            if(dat->dwFlagsEx & MWF_SHOW_SCROLLINGDISABLED)
                urc->rcItem.top += 24;
            if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL)
                urc->rcItem.top += panelHeight;
            urc->rcItem.bottom += 3;
            if(dat->pContainer->bSkinned) {
                StatusItems_t *item = &StatusItems[ID_EXTBKHISTORY];
                if(!item->IGNORED) {
                    urc->rcItem.left += item->MARGIN_LEFT;
                    urc->rcItem.right -= item->MARGIN_RIGHT;
                    urc->rcItem.top += item->MARGIN_TOP;
                    urc->rcItem.bottom -= item->MARGIN_BOTTOM;
                }
            }
            return RD_ANCHORX_WIDTH | RD_ANCHORY_HEIGHT;
        case IDC_PANELPIC:
			urc->rcItem.left = urc->rcItem.right - (panelWidth > 0 ? panelWidth - 2 : panelHeight + 2);
            urc->rcItem.bottom = urc->rcItem.top + (panelHeight - 3);
            if (myGlobals.g_FlashAvatarAvail) {
		    	RECT rc = { urc->rcItem.left,  urc->rcItem.top, urc->rcItem.right, urc->rcItem.bottom };
    			if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
    				FLASHAVATAR fa = {0}; 

                    fa.hContact = dat->hContact;
                    fa.id = 25367;
                    fa.cProto = dat->szProto;
					CallService(MS_FAVATAR_RESIZE, (WPARAM)&fa, (LPARAM)&rc);
				}
			}
            return RD_ANCHORX_RIGHT | RD_ANCHORY_TOP;
        case IDC_PANELSTATUS:
        {
            urc->rcItem.right = urc->dlgNewSize.cx - panelWidth;
            urc->rcItem.left = urc->dlgNewSize.cx - panelWidth - dat->panelStatusCX;
            urc->rcItem.bottom = panelHeight - 3;
            urc->rcItem.top = urc->rcItem.bottom - dat->ipFieldHeight;
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_TOP;
        }
        case IDC_PANELNICK:
        {
            urc->rcItem.right = urc->dlgNewSize.cx - panelWidth - 27;
            urc->rcItem.bottom = panelHeight - 3 - dat->ipFieldHeight - 2;;
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_TOP;
        }
        case IDC_APPARENTMODE:
        {
            urc->rcItem.right -= (panelWidth + 3);
            urc->rcItem.left -= (panelWidth + 3);
            return RD_ANCHORX_CUSTOM | RD_ANCHORX_RIGHT;
        }
        case IDC_PANELUIN:
        {
            urc->rcItem.right = urc->dlgNewSize.cx - (panelWidth + 2) - dat->panelStatusCX;
            urc->rcItem.bottom = panelHeight - 3;
            urc->rcItem.top = urc->rcItem.bottom - dat->ipFieldHeight;
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_TOP;
        }
        case IDC_SPLITTER:
            urc->rcItem.right = urc->dlgNewSize.cx;
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            urc->rcItem.bottom = urc->rcItem.top + 2;
            OffsetRect(&urc->rcItem, 0, 1);
            if(myGlobals.m_SideBarEnabled)
                urc->rcItem.left = 9;
            else
                urc->rcItem.left = 0;

            if(dat->fMustOffset)
                urc->rcItem.right -= (dat->pic.cx + 2);
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_BOTTOM;
        case IDC_CONTACTPIC:
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            //urc->rcItem.top=urc->rcItem.bottom-(dat->pic.cy +2);
            urc->rcItem.left=urc->rcItem.right-(dat->pic.cx +2);
            if((urc->rcItem.bottom - urc->rcItem.top) < (dat->pic.cy) && dat->showPic) {
                urc->rcItem.top=urc->rcItem.bottom-(dat->pic.cy);
                dat->fMustOffset = TRUE;
            } else
                dat->fMustOffset = FALSE;

            return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
        case IDC_MESSAGE:
            urc->rcItem.right = urc->dlgNewSize.cx;
            if (dat->showPic)
                urc->rcItem.right -= dat->pic.cx+2;
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            if(dat->bNotOnList) {
                if(urc->rcItem.bottom - urc->rcItem.top > 48) {
                    urc->rcItem.right -= 26;
                    not_on_list = 26;
                }
                else {
                    urc->rcItem.right -= 52;
                    not_on_list = 52;
                }
            }
            else
                not_on_list = 0;
            if(myGlobals.m_SideBarEnabled)
                urc->rcItem.left += 9;
            if(myGlobals.m_visualMessageSizeIndicator)
                urc->rcItem.bottom -= 2;
            msgTop = urc->rcItem.top;
            msgBottom = urc->rcItem.bottom;
            if(dat->pContainer->bSkinned) {
                StatusItems_t *item = &StatusItems[ID_EXTBKINPUTAREA];
                if(!item->IGNORED) {
                    urc->rcItem.left += item->MARGIN_LEFT;
                    urc->rcItem.right -= item->MARGIN_RIGHT;
                    urc->rcItem.top += item->MARGIN_TOP;
                    urc->rcItem.bottom -= item->MARGIN_BOTTOM;
                }
            }
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_BOTTOM;
        case IDOK:
            OffsetRect(&urc->rcItem, 12, 0);
            urc->rcItem.top -= (dat->splitterY - dat->originalSplitterY + s_offset);
            urc->rcItem.bottom -= (dat->splitterY - dat->originalSplitterY + s_offset);
            urc->rcItem.bottom++;

            if(dat->fMustOffset)
                OffsetRect(&urc->rcItem, -(dat->pic.cx + 2), 0);
            return RD_ANCHORX_RIGHT | RD_ANCHORY_BOTTOM;
        case IDC_MULTISPLITTER:
            if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL)
                urc->rcItem.top += panelHeight;
            urc->rcItem.left -= dat->multiSplitterX;
            urc->rcItem.right -= dat->multiSplitterX;
            urc->rcItem.bottom = iClistOffset;
            return RD_ANCHORX_RIGHT | RD_ANCHORY_CUSTOM;
        case IDC_TOGGLESIDEBAR:
            urc->rcItem.right = 8;
            urc->rcItem.left = 0;
            urc->rcItem.bottom = msgBottom;
            urc->rcItem.top = msgTop;
            return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
        case IDC_LOGFROZEN:
            if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL)
                OffsetRect(&urc->rcItem, 0, panelHeight);
            return RD_ANCHORX_RIGHT | RD_ANCHORY_TOP;
        case IDC_LOGFROZENTEXT:
            urc->rcItem.right = urc->dlgNewSize.cx - 20;
            if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL)
                OffsetRect(&urc->rcItem, 0, panelHeight);
            return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;
        case IDC_MSGINDICATOR:
            urc->rcItem.right = urc->dlgNewSize.cx;
            if (dat->showPic)
                urc->rcItem.right -= dat->pic.cx+2;
            if(myGlobals.m_SideBarEnabled)
                urc->rcItem.left += 9;
            urc->rcItem.right -= not_on_list;
            OffsetRect(&urc->rcItem, 0, -1);
            ShowWindow(GetDlgItem(hwndDlg, IDC_MSGINDICATOR), myGlobals.m_visualMessageSizeIndicator ? SW_SHOW : SW_HIDE);
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_BOTTOM;
    }
    return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
}

static void NotifyTyping(struct MessageWindowData *dat, int mode)
{
    DWORD protoStatus;
    DWORD protoCaps;
    DWORD typeCaps;

    if (!dat->hContact)
        return;

    DeletePopupsForContact(dat->hContact, PU_REMOVE_ON_TYPE);
    
    // Don't send to protocols who don't support typing
    // Don't send to users who are unchecked in the typing notification options
    // Don't send to protocols that are offline
    // Don't send to users who are not visible and
    // Don't send to users who are not on the visible list when you are in invisible mode.
    if (!DBGetContactSettingByte(dat->hContact, SRMSGMOD, SRMSGSET_TYPING, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW)))
        return;
    if (!dat->szProto)
        return;
    protoStatus = CallProtoService(dat->szProto, PS_GETSTATUS, 0, 0);
    protoCaps = CallProtoService(dat->szProto, PS_GETCAPS, PFLAGNUM_1, 0);
    typeCaps = CallProtoService(dat->szProto, PS_GETCAPS, PFLAGNUM_4, 0);

    if (!(typeCaps & PF4_SUPPORTTYPING))
        return;
    if (protoStatus < ID_STATUS_ONLINE)
        return;
    if (protoCaps & PF1_VISLIST && DBGetContactSettingWord(dat->hContact, dat->szProto, "ApparentMode", 0) == ID_STATUS_OFFLINE)
        return;
    if (protoCaps & PF1_INVISLIST && protoStatus == ID_STATUS_INVISIBLE && DBGetContactSettingWord(dat->hContact, dat->szProto, "ApparentMode", 0) != ID_STATUS_ONLINE)
        return;
    if (DBGetContactSettingByte(dat->hContact, "CList", "NotOnList", 0)
        && !DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGUNKNOWN, SRMSGDEFSET_TYPINGUNKNOWN))
        return;
    // End user check
    dat->nTypeMode = mode;
    CallService(MS_PROTO_SELFISTYPING, (WPARAM) dat->hContact, dat->nTypeMode);
}

BOOL CALLBACK DlgProcMessage(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct MessageWindowData *dat = 0;
    HWND   hwndTab, hwndContainer;
    struct ContainerWindowData *m_pContainer = 0;
    
    dat = (struct MessageWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);

    hwndTab = GetParent(hwndDlg);
    
    if(dat == 0) {
        if(dat == NULL && (msg == WM_ACTIVATE || msg == WM_SETFOCUS))
            return 0;
    }
    else {
        m_pContainer = dat->pContainer;
        hwndContainer = m_pContainer->hwnd;
    }
    switch (msg) {
        case WM_INITDIALOG:
            {
                RECT rc;
                POINT pt;
                int i;
                BOOL isFlat = DBGetContactSettingByte(NULL, SRMSGMOD_T, "tbflat", 0);
                BOOL isThemed = !DBGetContactSettingByte(NULL, SRMSGMOD_T, "nlflat", 0);
                HWND hwndItem;

                struct NewMessageWindowLParam *newData = (struct NewMessageWindowLParam *) lParam;

                dat = (struct MessageWindowData *) malloc(sizeof(struct MessageWindowData));
                ZeroMemory((void *) dat, sizeof(struct MessageWindowData));
                if (newData->iTabID >= 0) {
                    dat->pContainer = newData->pContainer;
                    m_pContainer = dat->pContainer;
                    hwndContainer = m_pContainer->hwnd;
                }
                SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) dat);

                if(rtf_ctable == NULL)
                    RTF_CTableInit();

                dat->wOldStatus = -1;
                dat->iOldHash = -1;
                dat->bType = SESSIONTYPE_IM;
                
                newData->item.lParam = (LPARAM) hwndDlg;
                TabCtrl_SetItem(hwndTab, newData->iTabID, &newData->item);
                dat->iTabID = newData->iTabID;

                DM_ThemeChanged(hwndDlg, dat);

                pszIDCSAVE_close = TranslateT("Close session");
                pszIDCSAVE_save = TranslateT("Save and close session");

				dat->hContact = newData->hContact;
                WindowList_Add(hMessageWindowList, hwndDlg, dat->hContact);
                BroadCastContainer(m_pContainer, DM_REFRESHTABINDEX, 0, 0);
                
                dat->szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)dat->hContact, 0);
                dat->bIsMeta = IsMetaContact(hwndDlg, dat) ? TRUE : FALSE;
                if(dat->hContact && dat->szProto != NULL) {
                    dat->wStatus = DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE);
#if defined(_UNICODE)
                    MY_GetContactDisplayNameW(dat->hContact, dat->szNickname, 84, dat->szProto, dat->codePage);
#else
                    mir_snprintf(dat->szNickname, 80, "%s", (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) dat->hContact, 0));
#endif                    
                    mir_snprintf(dat->szStatus, safe_sizeof(dat->szStatus), "%s", (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, dat->szProto == NULL ? ID_STATUS_OFFLINE : dat->wStatus, 0));
                    dat->avatarbg = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "avbg", GetSysColor(COLOR_3DFACE));
                }
                else
                    dat->wStatus = ID_STATUS_OFFLINE;
                
				GetContactUIN(hwndDlg, dat);
                GetClientIcon(dat, hwndDlg);

                dat->showUIElements = m_pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;
                dat->sendMode |= DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "forceansi", 0) ? SMODE_FORCEANSI : 0;
                dat->sendMode |= dat->hContact == 0 ? SMODE_MULTIPLE : 0;
                dat->sendMode |= DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "no_ack", 0) ? SMODE_NOACK : 0;

                dat->hwnd = hwndDlg;

                dat->ltr_templates = &LTR_Active;
                dat->rtl_templates = &RTL_Active;
                // input history stuff (initialise it..)

                dat->iHistorySize = DBGetContactSettingByte(NULL, SRMSGMOD_T, "historysize", 10);
                if(dat->iHistorySize < 10)
                    dat->iHistorySize = 10;
                dat->history = malloc(sizeof(struct InputHistory) * (dat->iHistorySize + 1));
                dat->iHistoryCurrent = 0;
                dat->iHistoryTop = 0;
                if(dat->history) {
                    for(i = 0; i <= dat->iHistorySize; i++) {
                        dat->history[i].szText = NULL;
                        dat->history[i].lLen = 0;
                    }
                }
                dat->hQueuedEvents = malloc(sizeof(HANDLE) * EVENT_QUEUE_SIZE);
                dat->iEventQueueSize = EVENT_QUEUE_SIZE;
                dat->iCurrentQueueError = -1;
                dat->history[dat->iHistorySize].szText = (TCHAR *)malloc((HISTORY_INITIAL_ALLOCSIZE + 1) * sizeof(TCHAR));
                dat->history[dat->iHistorySize].lLen = HISTORY_INITIAL_ALLOCSIZE;
				
				/*
				 * message history limit
				 * hHistoryEvents holds up to n event handles
				 */

				dat->maxHistory = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "maxhist", DBGetContactSettingDword(NULL, SRMSGMOD_T, "maxhist", 0));
				dat->curHistory = 0;
				if(dat->maxHistory)
					dat->hHistoryEvents = (HANDLE *)malloc(dat->maxHistory * sizeof(HANDLE));
				else
					dat->hHistoryEvents = NULL;

                if(dat->bIsMeta)
                    SendMessage(hwndDlg, DM_UPDATEMETACONTACTINFO, 0, 0);
                else
                    SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
                dat->bTabFlash = FALSE;
                dat->mayFlashTab = FALSE;
                //dat->iTabImage = newData->iTabImage;
                GetMyNick(hwndDlg, dat);

                
                dat->multiSplitterX = (int) DBGetContactSettingDword(NULL, SRMSGMOD, "multisplit", 150);
                dat->showTypingWin = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGWIN, SRMSGDEFSET_SHOWTYPINGWIN);
                dat->nTypeMode = PROTOTYPE_SELFTYPING_OFF;
                SetTimer(hwndDlg, TIMERID_TYPE, 1000, NULL);
                dat->iLastEventType = 0xffffffff;
                
                // load log option flags...
                dat->dwFlags = (DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT) & MWF_LOG_ALL);

                /*
                 * consider per-contact message setting overrides
                 */

                if(DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "mwoverride", 0)) {
                    DWORD dwLocalFlags = 0;
                    int dwLocalSmAdd = 0;
                    
                    if(dat->hContact) {
                        dwLocalFlags = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "mwflags", 0xffffffff);
                        if(dwLocalFlags != 0xffffffff)
                            dat->dwFlags = dwLocalFlags;
                        dwLocalSmAdd = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "doSmileys", 0xffffffff);
                        if(dwLocalSmAdd != 0xffffffff)
                            dat->doSmileys = dwLocalSmAdd;
                    }
                }
                dat->hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_BALLOON, CW_USEDEFAULT, CW_USEDEFAULT,
                                                 CW_USEDEFAULT, CW_USEDEFAULT, hwndDlg, NULL, g_hInst, (LPVOID) NULL);
                
                ZeroMemory((void *)&dat->ti, sizeof(dat->ti));
                dat->ti.cbSize = sizeof(dat->ti);
                dat->ti.lpszText = myGlobals.m_szNoStatus;
                dat->ti.hinst = g_hInst;
                dat->ti.hwnd = hwndDlg;
                dat->ti.uFlags = TTF_TRACK | TTF_IDISHWND | TTF_TRANSPARENT;
                dat->ti.uId = (UINT_PTR)hwndDlg;
                SendMessageA(dat->hwndTip, TTM_ADDTOOLA, 0, (LPARAM)&dat->ti);
                
                dat->dwFlagsEx = GetInfoPanelSetting(hwndDlg, dat) ? dat->dwFlagsEx | MWF_SHOW_INFOPANEL : dat->dwFlagsEx & ~MWF_SHOW_INFOPANEL;
                dat->dwFlagsEx |= DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "splitoverride", 0) ? MWF_SHOW_SPLITTEROVERRIDE : 0;
                SetMessageLog(hwndDlg, dat);
                LoadOverrideTheme(hwndDlg, dat);
				dat->panelWidth = -1;
                if(dat->hContact) {
                    dat->codePage = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "ANSIcodepage", CP_ACP);
                    dat->dwFlags |= (myGlobals.m_RTLDefault == 0 ? (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 0) ? MWF_LOG_RTL : 0) : (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 1) ? MWF_LOG_RTL : 0));
                    LoadPanelHeight(hwndDlg, dat);
                }

                dat->showPic = GetAvatarVisibility(hwndDlg, dat);
                GetWindowRect(GetDlgItem(hwndDlg, IDC_SMILEYBTN), &rc);

                ShowWindow(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), SW_HIDE);

                GetWindowRect(GetDlgItem(hwndDlg, IDC_SPLITTER), &rc);
                pt.y = (rc.top + rc.bottom) / 2;
                pt.x = 0;
                ScreenToClient(hwndDlg, &pt);
                dat->originalSplitterY = pt.y;
                if(m_pContainer->dwFlags & CNT_CREATE_MINIMIZED || (IsIconic(hwndContainer) && m_pContainer->bInTray == 0))
                    dat->originalSplitterY--;
                if (dat->splitterY == -1)
                    dat->splitterY = dat->originalSplitterY + 60;

                GetWindowRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc);
                dat->minEditBoxSize.cx = rc.right - rc.left;
                dat->minEditBoxSize.cy = rc.bottom - rc.top;

                SendMessage(hwndDlg, DM_LOADBUTTONBARICONS, 0, 0);

                SendDlgItemMessage(hwndDlg, IDC_FONTBOLD, BUTTONSETASPUSHBTN, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_FONTITALIC, BUTTONSETASPUSHBTN, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_FONTUNDERLINE, BUTTONSETASPUSHBTN, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_APPARENTMODE, BUTTONSETASPUSHBTN, 0, 0);
                
                if(m_pContainer->bSkinned && !StatusItems[ID_EXTBKBUTTONSNPRESSED].IGNORED &&
                   !StatusItems[ID_EXTBKBUTTONSPRESSED].IGNORED && !StatusItems[ID_EXTBKBUTTONSMOUSEOVER].IGNORED) {
                    isFlat = TRUE;
                    isThemed = FALSE;
                }
                for (i = 0; i < sizeof(buttonLineControlsNew) / sizeof(buttonLineControlsNew[0]); i++) {
                    hwndItem = GetDlgItem(hwndDlg, buttonLineControlsNew[i]);
					SendMessage(hwndItem, BUTTONSETASFLATBTN, 0, isFlat ? 0 : 1);
                    SendMessage(hwndItem, BUTTONSETASFLATBTN + 10, 0, isThemed ? 1 : 0);
                    SendMessage(hwndItem, BUTTONSETASFLATBTN + 12, 0, (LPARAM)m_pContainer);
                }
                for (i = 0; i < sizeof(formatControls) / sizeof(formatControls[0]); i++) {
                    hwndItem = GetDlgItem(hwndDlg, formatControls[i]);
                    SendMessage(hwndItem, BUTTONSETASFLATBTN, 0, isFlat ? 0 : 1);
                    SendMessage(hwndItem, BUTTONSETASFLATBTN + 10, 0, isThemed ? 1 : 0);
                    SendMessage(hwndItem, BUTTONSETASFLATBTN + 12, 0, (LPARAM)m_pContainer);
                }
                for (i = 0; i < sizeof(infoLineControls) / sizeof(infoLineControls[0]); i++) {
                    hwndItem = GetDlgItem(hwndDlg, infoLineControls[i]);
                    SendMessage(hwndItem, BUTTONSETASFLATBTN, 0, isFlat ? 0 : 1);
                    SendMessage(hwndItem, BUTTONSETASFLATBTN + 10, 0, isThemed ? 1 : 0);
                    SendMessage(hwndItem, BUTTONSETASFLATBTN + 12, 0, (LPARAM)m_pContainer);
                }

                for (i = 0; i < sizeof(errorButtons) / sizeof(errorButtons[0]); i++) {
                    hwndItem = GetDlgItem(hwndDlg, errorButtons[i]);
                    SendMessage(hwndItem, BUTTONSETASFLATBTN, 0, isFlat ? 0 : 1);
                    SendMessage(hwndItem, BUTTONSETASFLATBTN + 10, 0, isThemed ? 1 : 0);
                    SendMessage(hwndItem, BUTTONSETASFLATBTN + 12, 0, (LPARAM)m_pContainer);
                }

                hwndItem = GetDlgItem(hwndDlg, IDOK);
                SendMessage(hwndItem, BUTTONSETASFLATBTN, 0, isFlat ? 0 : 1);
                SendMessage(hwndItem, BUTTONSETASFLATBTN + 10, 0, isThemed ? 1 : 0);
                SendMessage(hwndItem, BUTTONSETASFLATBTN + 12, 0, (LPARAM)m_pContainer);
                
				SendMessage(GetDlgItem(hwndDlg, IDOK), BUTTONSETARROW, IDC_SENDMENU, 0);
				SendMessage(GetDlgItem(hwndDlg, IDC_PROTOCOL), BUTTONSETARROW, IDC_PROTOMENU, 0);
				SendMessage(GetDlgItem(hwndDlg, IDC_NAME), BUTTONSETARROW, IDC_INFOPANELMENU, 0);
                
                SendMessage(GetDlgItem(hwndDlg, IDC_TOGGLENOTES), BUTTONSETASFLATBTN, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_TOGGLESIDEBAR), BUTTONSETASFLATBTN, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_ADD), BUTTONSETASFLATBTN, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_CANCELADD), BUTTONSETASFLATBTN, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_LOGFROZEN), BUTTONSETASFLATBTN, 0, 0);

                SendDlgItemMessage(hwndDlg, IDC_APPARENTMODE, BUTTONSETASFLATBTN, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_APPARENTMODE, BUTTONSETASFLATBTN + 10, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_APPARENTMODE, BUTTONSETASFLATBTN + 12, 0, (LPARAM)m_pContainer);
                SendDlgItemMessage(hwndDlg, IDC_TOGGLENOTES, BUTTONSETASFLATBTN + 10, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_TOGGLENOTES, BUTTONSETASFLATBTN + 12, 0, (LPARAM)m_pContainer);

                SendDlgItemMessage(hwndDlg, IDC_TOGGLESIDEBAR, BUTTONSETASFLATBTN + 10, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_TOGGLESIDEBAR, BUTTONSETASFLATBTN + 12, 0, (LPARAM)m_pContainer);

                dat->dwFlags |= MWF_INITMODE;
                TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_OPENING, 0);

                for(i = 0;;i++) {
                    if(tooltips[i].id == -1)
                        break;
                    SendDlgItemMessage(hwndDlg, tooltips[i].id, BUTTONADDTOOLTIP, (WPARAM)TranslateTS(tooltips[i].szTip), 0);
                }
                
                SetDlgItemText(hwndDlg, IDC_LOGFROZENTEXT, TranslateT("Message Log is frozen"));
                
                SendMessage(GetDlgItem(hwndDlg, IDC_SAVE), BUTTONADDTOOLTIP, (WPARAM)pszIDCSAVE_close, 0);
                if(dat->bIsMeta)
                    SendMessage(GetDlgItem(hwndDlg, IDC_PROTOCOL), BUTTONADDTOOLTIP, (WPARAM) TranslateT("View User's Details\nRight click for MetaContact control\nClick dropdown for window settings"), 0);
                else
                    SendMessage(GetDlgItem(hwndDlg, IDC_PROTOCOL), BUTTONADDTOOLTIP, (WPARAM) TranslateT("View User's Details\nClick dropdown for window settings"), 0);
                
                SetWindowText(GetDlgItem(hwndDlg, IDC_RETRY), TranslateT("Retry"));
                SetWindowText(GetDlgItem(hwndDlg, IDC_CANCELSEND), TranslateT("Cancel"));
                SetWindowText(GetDlgItem(hwndDlg, IDC_MSGSENDLATER), TranslateT("Send later"));

                //SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETOLECALLBACK, 0, (LPARAM) & reOleCallback);
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETUNDOLIMIT, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_KEYEVENTS | ENM_LINK);
                SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_SCROLL | ENM_KEYEVENTS | ENM_CHANGE);
                
                /* OnO: higligh lines to their end */
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETEDITSTYLE, SES_EXTENDBACKCOLOR, SES_EXTENDBACKCOLOR);

                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETLANGOPTIONS, 0, SendDlgItemMessage(hwndDlg, IDC_LOG, EM_GETLANGOPTIONS, 0, 0) & ~IMF_AUTOFONTSIZEADJUST);

                /*
                 * add us to the tray list (if it exists)
                 */

                if(myGlobals.g_hMenuTrayUnread != 0 && dat->hContact != 0 && dat->szProto != NULL)
                    UpdateTrayMenu(0, dat->wStatus, dat->szProto, dat->szStatus, dat->hContact, FALSE);
                    
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_AUTOURLDETECT, (WPARAM) TRUE, 0);
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXLIMITTEXT, 0, 0x80000000);

                /*
                 * subclassing stuff
                 */

                OldMessageEditProc = (WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_WNDPROC, (LONG) MessageEditSubclassProc);
                
                OldAvatarWndProc = (WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, IDC_CONTACTPIC), GWL_WNDPROC, (LONG) AvatarSubclassProc);
                SetWindowLong(GetDlgItem(hwndDlg, IDC_PANELPIC), GWL_WNDPROC, (LONG) AvatarSubclassProc);

                OldSplitterProc = (WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_WNDPROC, (LONG) SplitterSubclassProc);
                SetWindowLong(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), GWL_WNDPROC, (LONG) SplitterSubclassProc);
                SetWindowLong(GetDlgItem(hwndDlg, IDC_PANELSPLITTER), GWL_WNDPROC, (LONG) SplitterSubclassProc);
                SetWindowLong(GetDlgItem(hwndDlg, IDC_MSGINDICATOR), GWL_WNDPROC, (LONG) MsgIndicatorSubclassProc);
                //SetMessageLog(hwndDlg, dat);
                
                /*
                 * load old messages from history (if wanted...)
                 */

                if(dat->hContact) {
                    FindFirstEvent(hwndDlg, dat);
                    GetMaxMessageLength(hwndDlg, dat);
                    GetCachedStatusMsg(hwndDlg, dat);
                }
                dat->stats.started = time(NULL);
                LoadContactAvatar(hwndDlg, dat);
				SendMessage(hwndDlg, DM_OPTIONSAPPLIED, 0, 0);
                LoadOwnAvatar(hwndDlg, dat);
                /*
                 * restore saved msg if any...
                 */
                if(dat->hContact) {
    				DBVARIANT dbv;
                    if(!DBGetContactSetting(dat->hContact, SRMSGMOD, "SavedMsg", &dbv)) {
                        SETTEXTEX stx = {ST_DEFAULT, CP_UTF8};
                        int len;
                        if(dbv.type == DBVT_ASCIIZ && lstrlenA(dbv.pszVal) > 0)
                            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)dbv.pszVal);
                        DBFreeVariant(&dbv);
                        len = GetWindowTextLength(GetDlgItem(hwndDlg,IDC_MESSAGE));
                        UpdateSaveAndSendButton(hwndDlg, dat);
                        if(m_pContainer->hwndActive == hwndDlg)
                            UpdateReadChars(hwndDlg, dat);
    				}
                    if(!DBGetContactSetting(dat->hContact, "UserInfo", "MyNotes", &dbv)) {
                        SetDlgItemTextA(hwndDlg, IDC_NOTES, dbv.pszVal);
                        DBFreeVariant(&dbv);
                    }
    			}
                if (newData->szInitialText) {
                    int len;
#if defined(_UNICODE)
                    if(newData->isWchar)
                        SetDlgItemTextW(hwndDlg, IDC_MESSAGE, (TCHAR *)newData->szInitialText);
                    else
                        SetDlgItemTextA(hwndDlg, IDC_MESSAGE, newData->szInitialText);
#else
                    SetDlgItemTextA(hwndDlg, IDC_MESSAGE, newData->szInitialText);
#endif
                    len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE));
                    PostMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_SETSEL, len, len);
                    if(len)
                        EnableSendButton(hwndDlg, TRUE);
                }
                dat->dwFlags &= ~MWF_INITMODE;
                {
                    DBEVENTINFO dbei = { 0};
                    HANDLE hdbEvent;

                    dbei.cbSize = sizeof(dbei);
                    hdbEvent = (HANDLE) CallService(MS_DB_EVENT_FINDLAST, (WPARAM) dat->hContact, 0);
                    if (hdbEvent) {
                        do {
                            ZeroMemory(&dbei, sizeof(dbei));
                            dbei.cbSize = sizeof(dbei);
                            CallService(MS_DB_EVENT_GET, (WPARAM) hdbEvent, (LPARAM) & dbei);
                            if (dbei.eventType == EVENTTYPE_MESSAGE && !(dbei.flags & DBEF_SENT)) {
                                dat->lastMessage = dbei.timestamp;
                                DM_UpdateLastMessage(hwndDlg, dat);
                                break;
                            }
                        }
                        while (hdbEvent = (HANDLE) CallService(MS_DB_EVENT_FINDPREV, (WPARAM) hdbEvent, 0));
                    }

                }
                SendMessage(hwndContainer, DM_QUERYCLIENTAREA, 0, (LPARAM)&rc);
				
				{
					WNDCLASSA wndClass;

					ZeroMemory(&wndClass, sizeof(wndClass));
					GetClassInfoA(g_hInst, "RichEdit20A", &wndClass);
					OldMessageLogProc = wndClass.lpfnWndProc;
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_LOG), GWL_WNDPROC, (LONG) MessageLogSubclassProc);
				}
                
                SetWindowPos(dat->hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
                SetWindowPos(hwndDlg, 0, rc.left, rc.top, (rc.right - rc.left), (rc.bottom - rc.top), newData->iActivate ? 0 : SWP_NOZORDER | SWP_NOACTIVATE);
                LoadSplitter(hwndDlg, dat);
                ShowPicture(hwndDlg,dat,TRUE);
                if(m_pContainer->dwFlags & CNT_CREATE_MINIMIZED || !newData->iActivate || m_pContainer->dwFlags & CNT_DEFERREDTABSELECT) {
                    DBEVENTINFO dbei = {0};

                    dbei.flags = 0;
                    dbei.eventType = EVENTTYPE_MESSAGE;
                    dat->iFlashIcon = myGlobals.g_IconMsgEvent;
                    SetTimer(hwndDlg, TIMERID_FLASHWND, TIMEOUT_FLASHWND, NULL);
                    dat->mayFlashTab = TRUE;
                    FlashOnClist(hwndDlg, dat, dat->hDbEventFirst, &dbei);
                    SendMessage(hwndContainer, DM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_EVENT_MESSAGE));
                    m_pContainer->dwFlags |= CNT_NEED_UPDATETITLE;
                    dat->dwFlags |= (MWF_NEEDCHECKSIZE | MWF_WASBACKGROUNDCREATE);
                    dat->dwFlags |= MWF_DEFERREDSCROLL;
                }
                if(newData->iActivate) {
                    m_pContainer->hwndActive = hwndDlg;
                    ShowWindow(hwndDlg, SW_SHOW);
                    SetActiveWindow(hwndDlg);
                    SetForegroundWindow(hwndDlg);
                    SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                    PostMessage(hwndContainer, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                }
                else if(m_pContainer->dwFlags & CNT_CREATE_MINIMIZED) {
                    dat->dwFlags |= MWF_DEFERREDSCROLL;
                    ShowWindow(hwndDlg, SW_SHOWNOACTIVATE);
                    m_pContainer->hwndActive = hwndDlg;
                    m_pContainer->dwFlags |= CNT_DEFERREDCONFIGURE;
                    PostMessage(hwndContainer, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                }
                SendMessage(hwndDlg, DM_CALCMINHEIGHT, 0, 0);
                DM_RecalcPictureSize(hwndDlg, dat);
                SendMessage(hwndContainer, DM_REPORTMINHEIGHT, (WPARAM) hwndDlg, (LPARAM) dat->uMinHeight);
                dat->dwLastActivity = GetTickCount() - 1000;
                m_pContainer->dwLastActivity = dat->dwLastActivity;
                TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_OPEN, 0);
                /*
                 * show a popup if wanted...
                 */
                if(newData->bWantPopup) {
                    DBEVENTINFO dbei = {0};
                    newData->bWantPopup = FALSE;
                    CallService(MS_DB_EVENT_GET, (WPARAM)newData->hdbEvent, (LPARAM)&dbei);
                    tabSRMM_ShowPopup((WPARAM)dat->hContact, (LPARAM)newData->hdbEvent, dbei.eventType, 0, 0, hwndDlg, dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat);
                }
                if(m_pContainer->dwFlags & CNT_CREATE_MINIMIZED) {
                    m_pContainer->dwFlags &= ~CNT_CREATE_MINIMIZED;
                    //dat->pContainer->dwFlags |= CNT_DEFERREDCONFIGURE;
                    m_pContainer->hwndActive = hwndDlg;
                    //SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                    return FALSE;
                }
                return newData->iActivate ? TRUE : FALSE;
            }
        case DM_TYPING:
            {
                dat->nTypeSecs = (int) lParam > 0 ? (int) lParam : 0;
                return 0;
            }
        case DM_UPDATEWINICON:
            {
                HWND t_hwnd;
                char *szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
                WORD wStatus = dat->bIsMeta ? dat->wMetaStatus : dat->wStatus;
                t_hwnd = m_pContainer->hwnd;

				if(dat->hXStatusIcon) {
					DestroyIcon(dat->hXStatusIcon);
					dat->hXStatusIcon = 0;
				}
                if (szProto) {
                    dat->hTabIcon = dat->hTabStatusIcon = LoadSkinnedProtoIcon(szProto, wStatus);
					if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "use_xicons", 0))
						dat->hXStatusIcon = GetXStatusIcon(dat);
                    SendDlgItemMessage(hwndDlg, IDC_PROTOCOL, BUTTONSETASFLATBTN + 11, 0, dat->dwFlagsEx & MWF_SHOW_ISIDLE ? 1 : 0);
					SendDlgItemMessage(hwndDlg, IDC_PROTOCOL, BM_SETIMAGE, IMAGE_ICON, (LPARAM)(dat->hXStatusIcon ? dat->hXStatusIcon : dat->hTabIcon));

                    if (m_pContainer->hwndActive == hwndDlg)
                        SendMessage(t_hwnd, DM_SETICON, (WPARAM) ICON_BIG, (LPARAM)(dat->hXStatusIcon ? dat->hXStatusIcon : dat->hTabIcon));
                }
                return 0;
            }
            /*
             * configures the toolbar only... if lParam != 0, then it also calls
             * SetDialogToType() to reconfigure the message window
             */
            
        case DM_CONFIGURETOOLBAR:
            dat->showUIElements = m_pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;

            splitterEdges = DBGetContactSettingByte(NULL, SRMSGMOD_T, "splitteredges", 1);

            if(splitterEdges == 0) {
                SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
                SetWindowLong(GetDlgItem(hwndDlg, IDC_PANELSPLITTER), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
            } else {
                SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_EXSTYLE) | WS_EX_STATICEDGE);
                SetWindowLong(GetDlgItem(hwndDlg, IDC_PANELSPLITTER), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_EXSTYLE) | WS_EX_STATICEDGE);
            }
            if(lParam == 1) {
                GetSendFormat(hwndDlg, dat, 1);
                SetDialogToType(hwndDlg);
            }
            dat->iButtonBarNeeds = dat->showUIElements ? (myGlobals.m_AllowSendButtonHidden ? 0 : 40) : 0;
            dat->iButtonBarNeeds += (dat->showUIElements ? (dat->doSmileys ? 220 : 198) : 0);
            
            dat->iButtonBarNeeds += (dat->showUIElements) ? 28 : 0;
            dat->iButtonBarReallyNeeds = dat->iButtonBarNeeds + (dat->showUIElements ? (myGlobals.m_AllowSendButtonHidden ? 110 : 70) : 0);
            if(!dat->SendFormat)
                dat->iButtonBarReallyNeeds -= 96;
            if(lParam == 1) {
                DM_RecalcPictureSize(hwndDlg, dat);
                SendMessage(hwndDlg, WM_SIZE, 0, 0);
                DM_ScrollToBottom(hwndDlg, dat, 0, 1);
            }
           return 0;
        case DM_LOADBUTTONBARICONS:
        {
            int i;

            for(i = 0;;i++) {
                if(buttonicons[i].id == -1)
                    break;
                SendDlgItemMessage(hwndDlg, buttonicons[i].id, BM_SETIMAGE, IMAGE_ICON, (LPARAM)*buttonicons[i].pIcon);
                SendDlgItemMessage(hwndDlg, buttonicons[i].id, BUTTONSETASFLATBTN + 12, 0, (LPARAM)m_pContainer);
            }
            return 0;
        }
        case DM_OPTIONSAPPLIED:
            dat->szMicroLf[0] = 0;
            if (wParam == 1) {      // 1 means, the message came from message log options page, so reload the defaults...
                if(DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "mwoverride", 0) == 0) {
                    dat->dwFlags &= ~(MWF_LOG_ALL);
                    dat->dwFlags |= DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);
                }
				dat->panelWidth = -1;
            }

            if(!(dat->dwFlags & MWF_SHOW_PRIVATETHEME))
                LoadThemeDefaults(hwndDlg, dat);
            
            if(dat->hContact) {
                dat->dwIsFavoritOrRecent = MAKELONG((WORD)DBGetContactSettingWord(dat->hContact, SRMSGMOD_T, "isFavorite", 0), (WORD)DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "isRecent", 0));
                LoadTimeZone(hwndDlg, dat);
            }

            if(dat->hContact && dat->szProto != NULL && dat->bIsMeta) {
                DWORD dwForcedContactNum = 0;
                CallService(MS_MC_GETFORCESTATE, (WPARAM)dat->hContact, (LPARAM)&dwForcedContactNum);
                DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "tabSRMM_forced", dwForcedContactNum);
            }

            dat->showUIElements = m_pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;
            
            dat->dwFlagsEx = DBGetContactSettingByte(NULL, SRMSGMOD_T, SRMSGSET_SHOWURLS, SRMSGDEFSET_SHOWURLS) ? MWF_SHOW_URLEVENTS : 0;
            dat->dwFlagsEx |= DBGetContactSettingByte(NULL, SRMSGMOD_T, SRMSGSET_SHOWFILES, SRMSGDEFSET_SHOWFILES) ? MWF_SHOW_FILEEVENTS : 0;
            dat->dwFlagsEx |= DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "splitoverride", 0) ? MWF_SHOW_SPLITTEROVERRIDE : 0;
            dat->dwFlagsEx |= DBGetContactSettingByte(NULL, SRMSGMOD_T, "log_bbcode", 1) ? MWF_SHOW_BBCODE : 0;
            dat->dwFlagsEx |= DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "uselocaltime", 0) ? MWF_SHOW_USELOCALTIME : 0;
            dat->dwFlagsEx = GetInfoPanelSetting(hwndDlg, dat) ? dat->dwFlagsEx | MWF_SHOW_INFOPANEL : dat->dwFlagsEx & ~MWF_SHOW_INFOPANEL;

            SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0,0));
                
            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3,3));     // XXX margins in the message area as well
            
            dat->showTypingWin = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGWIN, SRMSGDEFSET_SHOWTYPINGWIN);

            GetSendFormat(hwndDlg, dat, 1);
            SetDialogToType(hwndDlg);
            SendMessage(hwndDlg, DM_CONFIGURETOOLBAR, 0, 0);

            if (dat->hBkgBrush)
                DeleteObject(dat->hBkgBrush);
            if (dat->hInputBkgBrush)
                DeleteObject(dat->hInputBkgBrush);
            {
                char *szStreamOut = NULL;
                SETTEXTEX stx = {ST_DEFAULT,CP_UTF8};
                COLORREF colour = DBGetContactSettingDword(NULL, FONTMODULE, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR);
                COLORREF inputcharcolor;
                CHARFORMAT2A cf2 = {0};
                LOGFONTA lf;
                int i = 0;

                dat->inputbg = dat->theme.inputbg;
                if(GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MESSAGE)) > 0)
                    szStreamOut = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, (CP_UTF8 << 16) | (SF_RTFNOOBJS|SFF_PLAINRTF|SF_USECODEPAGE));

                dat->hBkgBrush = CreateSolidBrush(colour);
                dat->hInputBkgBrush = CreateSolidBrush(dat->inputbg);
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETBKGNDCOLOR, 0, colour);
                SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETBKGNDCOLOR, 0, dat->inputbg);
                lf = dat->theme.logFonts[MSGFONTID_MESSAGEAREA];
                inputcharcolor = dat->theme.fontColors[MSGFONTID_MESSAGEAREA];
                /*
                 * correct the input area text color to avoid a color from the table of usable bbcode colors
                 */
                for(i = 0; i < myGlobals.rtf_ctablesize; i++) {
                    if(rtf_ctable[i].clr == inputcharcolor)
                        inputcharcolor = RGB(GetRValue(inputcharcolor), GetGValue(inputcharcolor), GetBValue(inputcharcolor) == 0 ? GetBValue(inputcharcolor) + 1 : GetBValue(inputcharcolor) - 1);
                }
                cf2.dwMask = CFM_COLOR | CFM_FACE | CFM_CHARSET | CFM_SIZE | CFM_WEIGHT | CFM_BOLD | CFM_ITALIC;
                cf2.cbSize = sizeof(cf2);
                cf2.crTextColor = inputcharcolor;
                cf2.bCharSet = lf.lfCharSet;
                strncpy(cf2.szFaceName, lf.lfFaceName, LF_FACESIZE);
                cf2.dwEffects = ((lf.lfWeight >= FW_BOLD) ? CFE_BOLD : 0) | (lf.lfItalic ? CFE_ITALIC : 0);
                cf2.wWeight = (WORD)lf.lfWeight;
                cf2.bPitchAndFamily = lf.lfPitchAndFamily;
                cf2.yHeight = abs(lf.lfHeight) * 15;
                SendDlgItemMessageA(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, 0, (LPARAM)&cf2);

                /*
                 * setup the rich edit control(s)
                 * LOG is always set to RTL, because this is needed for proper bidirectional operation later.
                 * The real text direction is then enforced by the streaming code which adds appropiate paragraph
                 * and textflow formatting commands to the 
                 */
                {
                    PARAFORMAT2 pf2;
                    ZeroMemory((void *)&pf2, sizeof(pf2));
                    pf2.cbSize = sizeof(pf2);

                    pf2.wEffects = PFE_RTLPARA;// dat->dwFlags & MWF_LOG_RTL ? PFE_RTLPARA : 0;
                    pf2.dwMask = PFM_RTLPARA;
                    if(FindRTLLocale(dat))
                        SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
                    if(!(dat->dwFlags & MWF_LOG_RTL)) {
                        pf2.wEffects = 0;
                        SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
                    }
                    SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETLANGOPTIONS, 0, (LPARAM) SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETLANGOPTIONS, 0, 0) & ~IMF_AUTOKEYBOARD);

                    pf2.wEffects = PFE_RTLPARA;
                    pf2.dwMask |= PFM_OFFSET;
                    if(dat->dwFlags & MWF_INITMODE) {
                        pf2.dwMask |= (PFM_RIGHTINDENT | PFM_OFFSETINDENT);
                        pf2.dxStartIndent = 30;
                        pf2.dxRightIndent = 30;
                    }
                    pf2.dxOffset = dat->theme.left_indent + 30;
                    SetDlgItemText(hwndDlg, IDC_LOG, _T(""));
                    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
                    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETLANGOPTIONS, 0, (LPARAM) SendDlgItemMessage(hwndDlg, IDC_LOG, EM_GETLANGOPTIONS, 0, 0) & ~IMF_AUTOKEYBOARD);
                }

                /*
                 * set the scrollbars etc to RTL/LTR (only for manual RTL mode)
                 */

                if (dat->dwFlags & MWF_LOG_RTL) {
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE) | WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE) | WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_NOTES),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_NOTES),GWL_EXSTYLE) | (WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR));
                } else {
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE) &~ (WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR));
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE) &~ (WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR));
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_NOTES),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_NOTES),GWL_EXSTYLE) & ~(WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR));
                }
                SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
                InvalidateRect(GetDlgItem(hwndDlg, IDC_NOTES), NULL, FALSE);
                if(szStreamOut != NULL) {
                    SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)szStreamOut);
                    free(szStreamOut);
                }
            }
// END MOD#23
            if (hwndDlg == m_pContainer->hwndActive)
                SendMessage(hwndContainer, WM_SIZE, 0, 0);
            InvalidateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, FALSE);
            if (!lParam)
                SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
            
            if(m_pContainer->hwndStatus && dat->hContact)
                SetSelftypingIcon(hwndDlg, dat, DBGetContactSettingByte(dat->hContact, SRMSGMOD, SRMSGSET_TYPING, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW)));
            
            SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);

            break;
        case DM_UPDATETITLE:
            {
                char newtitle[128], *szProto = 0, *szOldMetaProto = 0;
                char *pszNewTitleEnd;
                TCHAR newcontactname[128];
                TCHAR *temp;
                TCITEM item;
                int    iHash = 0;
                WORD wOldApparentMode;
                DWORD dwOldIdle = dat->idle;
                DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) wParam;
                char *szActProto = 0;
                HANDLE hActContact = 0;
                BYTE oldXStatus = dat->xStatus;
                
                ZeroMemory((void *)newcontactname,  sizeof(newcontactname));
                dat->szNickname[0] = 0;
                dat->szStatus[0] = 0;
                
                pszNewTitleEnd = "Message Session";
                
                if(dat->iTabID == -1)
                    break;
                ZeroMemory((void *)&item, sizeof(item));
                if (dat->hContact) {
                    int iHasName;
                    TCHAR fulluin[128];
                    if (dat->szProto) {

                        if(dat->bIsMeta) {
                            szOldMetaProto = dat->szMetaProto;
                            szProto = GetCurrentMetaContactProto(hwndDlg, dat);
                            GetContactUIN(hwndDlg, dat);
                        }
                        szActProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
                        hActContact = dat->bIsMeta ? dat->hSubContact : dat->hContact;
#if defined(_UNICODE)
                        MY_GetContactDisplayNameW(dat->hContact, dat->szNickname, 84, dat->szProto, dat->codePage);
#else
                        mir_snprintf(dat->szNickname, 80, "%s", (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hActContact, 0));
#endif                        
                        iHasName = (int)dat->uin[0];        // dat->uin[0] == 0 if there is no valid UIN
                        dat->idle = DBGetContactSettingDword(dat->hContact, szActProto, "IdleTS", 0);
                        dat->dwFlagsEx =  dat->idle ? dat->dwFlagsEx | MWF_SHOW_ISIDLE : dat->dwFlagsEx & ~MWF_SHOW_ISIDLE;
                        dat->xStatus = DBGetContactSettingByte(hActContact, szActProto, "XStatusId", 0);
                        
                    /*
                     * cut nickname on tabs...
                     */
                        temp = dat->szNickname;
                        while(*temp)
                            iHash += (*(temp++) * (int)(temp - dat->szNickname + 1));

                        dat->wStatus = DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE);
	                    mir_snprintf(dat->szStatus, safe_sizeof(dat->szStatus), "%s", (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, dat->szProto == NULL ? ID_STATUS_OFFLINE : dat->wStatus, 0));
                        wOldApparentMode = dat->wApparentMode;
                        dat->wApparentMode = DBGetContactSettingWord(hActContact, szActProto, "ApparentMode", 0);
                        
                        if (iHash != dat->iOldHash || dat->wStatus != dat->wOldStatus || dat->xStatus != oldXStatus || lParam != 0) {
                            if (myGlobals.m_CutContactNameOnTabs)
                                CutContactName(dat->szNickname, newcontactname, sizeof(newcontactname) / sizeof(TCHAR));
                            else
                                lstrcpyn(newcontactname, dat->szNickname, sizeof(newcontactname) / sizeof(TCHAR));

                            if (lstrlen(newcontactname) != 0 && dat->szStatus != NULL) {
                                if (myGlobals.m_StatusOnTabs)
#if defined(_UNICODE)
                                    mir_snprintf(newtitle, 127, "%s (%s)", "%nick%", dat->szStatus);
#else
                                    mir_snprintf(newtitle, 127, "%s (%s)", newcontactname, dat->szStatus);
#endif
                                else
#if defined(_UNICODE)
                                    mir_snprintf(newtitle, 127, "%s", "%nick%");
#else
                                    mir_snprintf(newtitle, 127, "%s", newcontactname);
#endif
                            } else
                                mir_snprintf(newtitle, 127, "%s", "Forward");
                            
                            item.mask |= TCIF_TEXT;
                        }
                        SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
#if defined _UNICODE
                        {
                            WCHAR wUIN[80];
                            MultiByteToWideChar(dat->codePage, 0, dat->uin, -1, wUIN, 80);
                            wUIN[79] = 0;
                            mir_sntprintf(fulluin, safe_sizeof(fulluin), TranslateT("UIN: %s (SHIFT click -> copy to clipboard)\nClick for contact menu\nClick dropdown for infopanel settings."), iHasName ? wUIN : TranslateT("No UIN"));
                        }
#else
                        mir_sntprintf(fulluin, safe_sizeof(fulluin), TranslateT("UIN: %s (SHIFT click -> copy to clipboard)\nClick for contact menu\nClick dropdown for infopanel settings."), iHasName ? dat->uin : TranslateT("No UIN"));
#endif
                        SendMessage(GetDlgItem(hwndDlg, IDC_NAME), BUTTONADDTOOLTIP, iHasName ? (WPARAM)fulluin : (WPARAM)"", 0);
                        
                    }
                } else
                    lstrcpynA(newtitle, pszNewTitleEnd, sizeof(newtitle));

                if (dat->xStatus != oldXStatus || dat->idle != dwOldIdle || iHash != dat->iOldHash || dat->wApparentMode != wOldApparentMode || dat->wStatus != dat->wOldStatus || lParam != 0 || (dat->bIsMeta && dat->szMetaProto != szOldMetaProto)) {
                    if(dat->hContact != 0 && myGlobals.m_LogStatusChanges != 0 && DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "logstatus", -1) != 0) {
                        if(dat->wStatus != dat->wOldStatus && dat->hContact != 0 && dat->wOldStatus != (WORD)-1 && !(dat->dwFlags & MWF_INITMODE)) {             // log status changes to message log
                            DBEVENTINFO dbei;
                            char buffer[450];
                            HANDLE hNewEvent;
                            int iLen;
                            
                            char *szOldStatus = (char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)dat->wOldStatus, 0);
                            char *szNewStatus = dat->szStatus;
                            
                            if(dat->szProto != NULL) {
                                if(dat->wStatus == ID_STATUS_OFFLINE)
                                    mir_snprintf(buffer, sizeof(buffer), Translate("signed off."));
                                else if(dat->wOldStatus == ID_STATUS_OFFLINE)
                                    mir_snprintf(buffer, sizeof(buffer), Translate("signed on and is now %s."), szNewStatus);
                                else
                                    mir_snprintf(buffer, sizeof(buffer), Translate("changed status from %s to %s."), szOldStatus, szNewStatus);
                            }
                            iLen = strlen(buffer) + 1;
#if defined(_UNICODE)
                            MultiByteToWideChar(myGlobals.m_LangPackCP, 0, buffer, iLen, (LPWSTR)&buffer[iLen], (450 - iLen) / sizeof(wchar_t));
                            dbei.cbBlob = iLen * (sizeof(TCHAR) + 1);
#else
							dbei.cbBlob = iLen;
#endif
                            dbei.cbSize = sizeof(dbei);
                            dbei.pBlob = (PBYTE) buffer;
                            dbei.eventType = EVENTTYPE_STATUSCHANGE;
                            dbei.flags = DBEF_READ;
                            dbei.timestamp = time(NULL);
                            dbei.szModule = dat->szProto;
                            hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) dat->hContact, (LPARAM) & dbei);
                            if (dat->hDbEventFirst == NULL) {
                                dat->hDbEventFirst = hNewEvent;
                                SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                            }
                        }
                    }

                    if(item.mask & TCIF_TEXT) {
#ifdef _UNICODE
                        const wchar_t *newTitle = EncodeWithNickname(newtitle, newcontactname, myGlobals.m_LangPackCP);
                        int len = lstrlenW(newTitle);
                        if (len != 0) {
                            wcsncpy(dat->newtitle, newTitle, 127);
                            dat->newtitle[127] = 0;
                            item.pszText = dat->newtitle;
                            item.cchTextMax = 128;
                        }
#else
                        item.pszText = newtitle;
                        strncpy(dat->newtitle, newtitle, sizeof(dat->newtitle));
                        dat->newtitle[127] = 0;
                        item.cchTextMax = 127;;
#endif
                    }
                    if (dat->iTabID  >= 0)
                        TabCtrl_SetItem(hwndTab, dat->iTabID, &item);
					if (m_pContainer->hwndActive == hwndDlg && (dat->iOldHash != iHash || dat->wOldStatus != dat->wStatus || dat->xStatus != oldXStatus))
                        SendMessage(hwndContainer, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);

                    dat->iOldHash = iHash;
                    dat->wOldStatus = dat->wStatus;
                    
                    UpdateTrayMenuState(dat, TRUE);
                    if(LOWORD(dat->dwIsFavoritOrRecent))
                        AddContactToFavorites(dat->hContact, dat->szNickname, szActProto, dat->szStatus, dat->wStatus, LoadSkinnedProtoIcon(dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->bIsMeta ? dat->wMetaStatus : dat->wStatus), 0, myGlobals.g_hMenuFavorites, dat->codePage);
                    if(DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "isRecent", 0)) {
                        dat->dwIsFavoritOrRecent |= 0x00010000;
                        AddContactToFavorites(dat->hContact, dat->szNickname, szActProto, dat->szStatus, dat->wStatus, LoadSkinnedProtoIcon(dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->bIsMeta ? dat->wMetaStatus : dat->wStatus), 0, myGlobals.g_hMenuRecent, dat->codePage);
                    }
                    else
                        dat->dwIsFavoritOrRecent &= 0x0000ffff;

                    if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELSTATUS), NULL, FALSE);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELNICK), NULL, FALSE);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELUIN), NULL, FALSE);
                        UpdateApparentModeDisplay(hwndDlg, dat);
                    }
                    
  					if (myGlobals.g_FlashAvatarAvail) {
  						FLASHAVATAR fa = {0}; 

                        fa.hContact = dat->hContact;
						fa.hWindow = 0;
                        fa.id = 25367;
                        fa.cProto = dat->szProto;

						CallService(MS_FAVATAR_GETINFO, (WPARAM)&fa, 0);
   						dat->hwndFlash = fa.hWindow;
   						if(dat->hwndFlash) {
   							BOOL isInfoPanel = dat->dwFlagsEx & MWF_SHOW_INFOPANEL;
   							SetParent(dat->hwndFlash, GetDlgItem(hwndDlg, isInfoPanel ? IDC_PANELPIC : IDC_CONTACTPIC));
   						}
   					}
                    dat->lastRetrievedStatusMsg = 0;
                }
                // care about MetaContacts and update the statusbar icon with the currently "most online" contact...
                if(dat->bIsMeta) {
                    PostMessage(hwndDlg, DM_UPDATEMETACONTACTINFO, 0, 0);
                    PostMessage(hwndDlg, DM_OWNNICKCHANGED, 0, 0);
                    if(m_pContainer->dwFlags & CNT_UINSTATUSBAR)
                        DM_UpdateLastMessage(hwndDlg, dat);
                }
                return 0;
            }
        case DM_UPDATESTATUSMSG:
            GetCachedStatusMsg(hwndDlg, dat);
            InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELNICK), NULL, FALSE);
            return 0;
        case DM_OWNNICKCHANGED:
            GetMyNick(hwndDlg, dat);
            return 0;
        case DM_ADDDIVIDER:
            if(!(dat->dwFlags & MWF_DIVIDERSET) && myGlobals.m_UseDividers) {
                if(GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_LOG)) > 0) {
                    dat->dwFlags |= MWF_DIVIDERWANTED;
                    dat->dwFlags |= MWF_DIVIDERSET;
                }
            }
            return 0;

        case WM_SETFOCUS:
		   if(myGlobals.g_FlashAvatarAvail) { // own avatar draw
			    FLASHAVATAR fa = { 0 };
				fa.cProto = dat->szProto;
				fa.id = 25367;
				
				CallService(MS_FAVATAR_GETINFO, (WPARAM)&fa, 0);
				if(fa.hWindow) {
					if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
						SetParent(fa.hWindow, GetDlgItem(hwndDlg, IDC_CONTACTPIC));
						ShowWindow(fa.hWindow, SW_SHOW);
					} else {
						ShowWindow(fa.hWindow, SW_HIDE);
					}
				}
			}
           MsgWindowUpdateState(hwndDlg, dat, WM_SETFOCUS);
           SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
           return 1;

        case WM_ACTIVATE:
            if (LOWORD(wParam) != WA_ACTIVE) {
                break;
            }
            //fall through
        case WM_MOUSEACTIVATE:
            MsgWindowUpdateState(hwndDlg, dat, WM_ACTIVATE);
            return 1;

        case WM_GETMINMAXINFO:
            {
                MINMAXINFO *mmi = (MINMAXINFO *) lParam;
                RECT rcWindow, rcLog;
                GetWindowRect(hwndDlg, &rcWindow);
                GetWindowRect(GetDlgItem(hwndDlg, IDC_LOG), &rcLog);
                mmi->ptMinTrackSize.x = rcWindow.right - rcWindow.left - ((rcLog.right - rcLog.left) - dat->minEditBoxSize.cx);
                mmi->ptMinTrackSize.y = rcWindow.bottom - rcWindow.top - ((rcLog.bottom - rcLog.top) - dat->minEditBoxSize.cy);
#ifdef __MATHMOD_SUPPORT    			
                //mathMod begin		// set maximum size, to fit formula-preview on the screen.
    			if (CallService(MTH_GET_PREVIEW_SHOWN,0,0))	//when preview is shown, fit the maximum size of message-dialog.
    				mmi->ptMaxSize.y = GetSystemMetrics(SM_CYSCREEN)-CallService(MTH_GET_PREVIEW_HEIGHT ,0,0);//max-height
    			else
    				mmi->ptMaxSize.y = GetSystemMetrics(SM_CYSCREEN);				
    			//mathMod end 
#endif                
                return 0;
            }
        case WM_SIZE:
            {
                UTILRESIZEDIALOG urd;
                BITMAP bminfo;
                int buttonBarSpace;
                RECT rc, rcPic;
                int saved = 0, i;
                int delta;
                HBITMAP hbm = dat->dwFlagsEx & MWF_SHOW_INFOPANEL ? dat->hOwnPic : (dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown);
                RECT rcPanelBottom;

                if (IsIconic(hwndDlg))
                    break;
                ZeroMemory(&urd, sizeof(urd));
                urd.cbSize = sizeof(urd);
                urd.hInstance = g_hInst;
                urd.hwndDlg = hwndDlg;
                urd.lParam = (LPARAM) dat;
                urd.lpTemplate = MAKEINTRESOURCEA(IDD_MSGSPLITNEW);
                urd.pfnResizer = /* dat->dwEventIsShown & MWF_SHOW_RESIZEIPONLY ? MessageDialogResizeIP : */ MessageDialogResize;

                if(dat->ipFieldHeight == 0) {
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_PANELUIN), &rcPanelBottom);
                    dat->ipFieldHeight = rcPanelBottom.bottom - rcPanelBottom.top;
                }

                if (dat->uMinHeight > 0 && HIWORD(lParam) >= dat->uMinHeight) {
                    if (dat->splitterY > HIWORD(lParam) - MINLOGHEIGHT) {
                        dat->splitterY = HIWORD(lParam) - MINLOGHEIGHT;
                        dat->dynaSplitter = dat->splitterY - 34;
                        DM_RecalcPictureSize(hwndDlg, dat);
                    }
                    if (dat->splitterY < MINSPLITTERY)
                        LoadSplitter(hwndDlg, dat);
                }

                if(hbm != 0) {
                    GetObject(hbm, sizeof(bminfo), &bminfo);
                    CalcDynamicAvatarSize(hwndDlg, dat, &bminfo);
                }

                // dynamic toolbar layout...
                
                GetClientRect(hwndDlg, &rc);
                buttonBarSpace = rc.right;

                {
                    POINT pt;
                    GetClientRect(GetDlgItem(hwndDlg, IDC_SPLITTER), &rcPic);
                    pt.x = 0; pt.y = rcPic.bottom;
                    MapWindowPoints(GetDlgItem(hwndDlg, IDC_SPLITTER), hwndDlg, &pt, 1);

                    if((rc.bottom - pt.y <= dat->pic.cy) && dat->showPic)
                        dat->fMustOffset = TRUE;
                    else
                        dat->fMustOffset = FALSE;
                }

                if(dat->showPic) {
                    if(dat->fMustOffset)
                        buttonBarSpace = rc.right - (dat->pic.cx + 2);
                }

                dat->controlsHidden = FALSE;

                delta = dat->iButtonBarReallyNeeds - buttonBarSpace;

                if(dat->hContact != 0 && dat->showUIElements != 0) {
                    const UINT *hideThisControls = myGlobals.m_ToolbarHideMode ? controlsToHide : controlsToHide1;
                    if(!dat->SendFormat)
                        hideThisControls = controlsToHide2;
                    for(i = 0;;i++) {
                        if(hideThisControls[i] == -1)
                            break;
                        if(hideThisControls[i] == IDOK && myGlobals.m_AllowSendButtonHidden == 0)
                            continue;               // ignore sendbutton if we don't want to hide it
                        if(saved < delta) {
                            ShowWindow(GetDlgItem(hwndDlg, hideThisControls[i]), SW_HIDE);
                            if(hideThisControls[i] == IDC_PROTOCOL) {
                                dat->controlsHidden |= TOOLBAR_PROTO_HIDDEN;
                                saved += 8; //10;
                                //ShowWindow(GetDlgItem(hwndDlg, IDC_INFOPANELMENU), SW_HIDE);
                            }
                            if(hideThisControls[i] == IDOK)
                                dat->controlsHidden |= TOOLBAR_SEND_HIDDEN;
                            saved += (hideThisControls[i] == IDOK ? 50 : 23);
                        }
                        else {
                            if(!IsWindowVisible(GetDlgItem(hwndDlg, hideThisControls[i]))) {
                                ShowWindow(GetDlgItem(hwndDlg, hideThisControls[i]), SW_SHOW);
                                //if(hideThisControls[i] == IDC_PROTOCOL)
                                //    ShowWindow(GetDlgItem(hwndDlg, IDC_INFOPANELMENU), SW_SHOW);
                            }
                        }
                    }
                }
                CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM) & urd);
                
                if(GetDlgItem(hwndDlg, IDC_CLIST) != 0) {
                    RECT rc, rcClient, rcLog;
                    GetClientRect(hwndDlg, &rcClient);
                    GetClientRect(GetDlgItem(hwndDlg, IDC_LOG), &rcLog);
                    rc.top = 0; 
                    rc.right = rcClient.right - 3;
                    rc.left = rcClient.right - dat->multiSplitterX;
                    rc.bottom = rcLog.bottom;
                    if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL)
                        rc.top += dat->panelHeight;
                    if(dat->dwFlagsEx & MWF_SHOW_SCROLLINGDISABLED)
                        rc.top += 24;
                    MoveWindow(GetDlgItem(hwndDlg, IDC_CLIST), rc.left, rc.top, rc.right - rc.left, rcLog.bottom - rcLog.top, FALSE);
                }
                
                if (dat->hwndIEView != 0)
                    ResizeIeView(hwndDlg, dat, 0, 0, 0, 0);
                if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELUIN), NULL, FALSE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELNICK), NULL, FALSE);
                }
                if(wParam == 0 && lParam == 0 && m_pContainer->bSkinned)
                    RedrawWindow(hwndDlg, NULL, NULL, RDW_UPDATENOW | RDW_NOCHILDREN | RDW_INVALIDATE);
                break;
            }
        case DM_UPDATEPICLAYOUT:
            LoadContactAvatar(hwndDlg, dat);
            SendMessage(hwndDlg, WM_SIZE, 0, 0);
            return 0;
		case DM_SPLITTERMOVEDGLOBAL:
			if(!(dat->dwFlagsEx & MWF_SHOW_SPLITTEROVERRIDE)) {
				short newMessagePos;
				RECT rcWin, rcClient;

                if(IsIconic(m_pContainer->hwnd) || m_pContainer->hwndActive != hwndDlg) {
                    dat->dwFlagsEx |= MWF_EX_DELAYEDSPLITTER;
                    dat->wParam = wParam;
                    dat->lParam = lParam;
                    return 0;
                }
                GetWindowRect(hwndDlg, &rcWin);
				GetClientRect(hwndDlg, &rcClient);
				newMessagePos = (short)rcWin.bottom - (short)wParam;

                //SendMessage(hwndDlg, DM_SAVESIZE, 0, 0);
                SendMessage(hwndDlg, DM_SPLITTERMOVED, newMessagePos + lParam / 2, (LPARAM)GetDlgItem(hwndDlg, IDC_SPLITTER));
				SaveSplitter(hwndDlg, dat);
                PostMessage(hwndDlg, DM_DELAYEDSCROLL, 0, 1);
			}
			return 0;
        case DM_SPLITTERMOVED:
            {
                POINT pt;
                RECT rc;
                
                if ((HWND) lParam == GetDlgItem(hwndDlg, IDC_MULTISPLITTER)) {
                    int oldSplitterX;
                    GetClientRect(hwndDlg, &rc);
                    pt.x = wParam;
                    pt.y = 0;
                    ScreenToClient(hwndDlg, &pt);
                    oldSplitterX = dat->multiSplitterX;
                    dat->multiSplitterX = rc.right - pt.x;
                    if (dat->multiSplitterX < 25)
                        dat->multiSplitterX = 25;

                    if (dat->multiSplitterX > ((rc.right - rc.left) - 80))
                        dat->multiSplitterX = oldSplitterX;

                } else if ((HWND) lParam == GetDlgItem(hwndDlg, IDC_SPLITTER)) {
                    int oldSplitterY;

                    GetClientRect(hwndDlg, &rc);
                    pt.x = 0;
                    pt.y = wParam;
                    ScreenToClient(hwndDlg, &pt);

                    oldSplitterY = dat->splitterY;
                    dat->splitterY = rc.bottom - pt.y + 23;
                /*
                 * attempt to fix splitter troubles..
                 * hardcoded limits... better solution is possible, but this works for now
                 */
                    if (dat->splitterY < MINSPLITTERY)           // min splitter size
                        dat->splitterY = MINSPLITTERY;
                    else if (dat->splitterY > ((rc.bottom - rc.top) - 50)) 
                        dat->splitterY = oldSplitterY;
                    else {
                        dat->dynaSplitter = (rc.bottom - pt.y) - 11;
                        DM_RecalcPictureSize(hwndDlg, dat);
                    }
                }
                else if((HWND) lParam == GetDlgItem(hwndDlg, IDC_PANELSPLITTER)) {
                    RECT rc;
                    POINT pt;
                    pt.x = 0; pt.y = wParam;
                    ScreenToClient(hwndDlg, &pt);
                    GetClientRect(hwndDlg, &rc);
                    if(pt.y + 2 >= 51 && pt.y + 2 < 100)
                        dat->panelHeight = pt.y + 2;
                    SendMessage(hwndDlg, WM_SIZE, DM_SPLITTERMOVED, 0);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELUIN), NULL, FALSE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELNICK), NULL, FALSE);
                    break;
                }
                SendMessage(hwndDlg, WM_SIZE, DM_SPLITTERMOVED, 0);
                return 0;
            }
            /*
             * queue a dm_remakelog
             * wParam = hwnd of the sender, so we can directly do a DM_REMAKELOG if the msg came
             * from ourself. otherwise, the dm_remakelog will be deferred until next window
             * activation (focus)
             */
        case DM_DEFERREDREMAKELOG:
            if((HWND) wParam == hwndDlg)
                SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
            else {
                if(DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "mwoverride", 0) == 0) {
                    dat->dwFlags &= ~(MWF_LOG_ALL);
                    dat->dwFlags |= (lParam & MWF_LOG_ALL);
                    dat->dwFlags |= MWF_DEFERREDREMAKELOG;
                }
            }
            return 0;
        case DM_FORCEDREMAKELOG:
            if((HWND) wParam == hwndDlg)
                SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
            else {
                dat->dwFlags &= ~(MWF_LOG_ALL);
                dat->dwFlags |= (lParam & MWF_LOG_ALL);
                dat->dwFlags |= MWF_DEFERREDREMAKELOG;
            }
            return 0;
        case DM_REMAKELOG:
            dat->szMicroLf[0] = 0;
            dat->lastEventTime = 0;
            dat->iLastEventType = -1;
            StreamInEvents(hwndDlg, dat->hDbEventFirst, -1, 0, NULL);
            return 0;
        case DM_APPENDTOLOG:
            if((HANDLE)wParam != dat->hDbEventLastFeed) {
                dat->hDbEventLastFeed = (HANDLE)wParam;
                StreamInEvents(hwndDlg, (HANDLE) wParam, 1, 1, NULL);
            }
            else {
                TCHAR szBuffer[256];
                mir_sntprintf(szBuffer, safe_sizeof(szBuffer), TranslateT("Duplicate event handle detected"));
                SendMessage(hwndDlg, DM_ACTIVATETOOLTIP, IDC_MESSAGE, (LPARAM)szBuffer);
            }
            return 0;
            /*
             * replays queued events after the message log has been frozen for a while
             */
        case DM_REPLAYQUEUE:
            {
                int i;

                for(i = 0; i < dat->iNextQueuedEvent; i++) {
                    if(dat->hQueuedEvents[i] != 0)
                        StreamInEvents(hwndDlg, dat->hQueuedEvents[i], 1, 1, NULL);
                }
                dat->iNextQueuedEvent = 0;
                SetDlgItemText(hwndDlg, IDC_LOGFROZENTEXT, TranslateT("Message Log is frozen"));
                return 0;
            }
        case DM_SCROLLIEVIEW:
            {
                IEVIEWWINDOW iew = {0};

                iew.cbSize = sizeof(IEVIEWWINDOW);
                iew.iType = IEW_SCROLLBOTTOM;
                iew.hwnd = dat->hwndIEView;
                CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&iew);
                return 0;
            }
        case DM_DELAYEDSCROLL:
            return DM_ScrollToBottom(hwndDlg, dat, wParam, lParam);
        case DM_FORCESCROLL:
            {
                SCROLLINFO *psi = (SCROLLINFO *)lParam;
                POINT *ppt = (POINT *)wParam;

                HWND hwnd = GetDlgItem(hwndDlg, IDC_LOG);
                int len;

                if(wParam == 0 && lParam == 0)
                    return(DM_ScrollToBottom(hwndDlg, dat, 0, 1));

                if(dat->hwndIEView == 0) {
                    len = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_LOG));
                    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETSEL, len - 1, len - 1);
                    //SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETSEL, -1, -1);
                }

                if(psi == NULL)
                    return(DM_ScrollToBottom(hwndDlg, dat, 0, 0));

                if ((UINT)psi->nPos >= (UINT)psi->nMax-psi->nPage-5 || psi->nMax-psi->nMin-psi->nPage < 50)
                    DM_ScrollToBottom(hwndDlg, dat, 0, 0);
                else
                    SendMessage(dat->hwndIEView ? dat->hwndIEView : hwnd, EM_SETSCROLLPOS, 0, (LPARAM)ppt);
                
                return 0;
            }
            /*
             * this is called whenever a new event has been added to the database.
             * this CAN be posted (some sanity checks required).
             */
        case HM_DBEVENTADDED:
            if (!dat)
                return 0;
            if ((HANDLE)wParam != dat->hContact)
                return 0;
            if (dat->hContact == NULL)
                return 0;
            {
                DBEVENTINFO dbei = {0};
                DWORD dwTimestamp = 0;
                BOOL  fIsStatusChangeEvent = FALSE;
                dbei.cbSize = sizeof(dbei);
                dbei.cbBlob = 0;
                CallService(MS_DB_EVENT_GET, lParam, (LPARAM) & dbei);
                if (dat->hDbEventFirst == NULL)
                    dat->hDbEventFirst = (HANDLE) lParam;
                
                fIsStatusChangeEvent = IsStatusEvent(dbei.eventType);

                if (dbei.eventType == EVENTTYPE_MESSAGE && (dbei.flags & DBEF_READ))
                    break;
                if (DbEventIsShown(dat, &dbei)) {
                    if (dbei.eventType == EVENTTYPE_MESSAGE && m_pContainer->hwndStatus && !(dbei.flags & (DBEF_SENT))) {
                        dat->lastMessage = dbei.timestamp;
                        PostMessage(hwndDlg, DM_UPDATELASTMESSAGE, 0, 0);
                    }
                    /*
                     * set the message log divider to mark new (maybe unseen) messages, if the container has
                     * been minimized or in the background.
                     */
                    if(!(dbei.flags & DBEF_SENT) && !fIsStatusChangeEvent) {
                        
                        if(myGlobals.m_DividersUsePopupConfig && myGlobals.m_UseDividers) {
                            if(!MessageWindowOpened((WPARAM)dat->hContact, 0))
                                SendMessage(hwndDlg, DM_ADDDIVIDER, 0, 0);
                        }
                        else if(myGlobals.m_UseDividers) {
                            if((GetForegroundWindow() != hwndContainer || GetActiveWindow() != hwndContainer))
                                SendMessage(hwndDlg, DM_ADDDIVIDER, 0, 0);
                            else {
                                if(m_pContainer->hwndActive != hwndDlg)
                                    SendMessage(hwndDlg, DM_ADDDIVIDER, 0, 0);
                            }
                        }
                        tabSRMM_ShowPopup(wParam, lParam, dbei.eventType, 1, m_pContainer, hwndDlg, dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat);
                    }
                    
                    if ((HANDLE) lParam != dat->hDbEventFirst) {
                        HANDLE nextEvent = (HANDLE) CallService(MS_DB_EVENT_FINDNEXT, lParam, 0);
                        if(myGlobals.m_FixFutureTimestamps || nextEvent == 0) {
                            if(!(dat->dwFlagsEx & MWF_SHOW_SCROLLINGDISABLED))
                                SendMessage(hwndDlg, DM_APPENDTOLOG, lParam, 0);
                            else {
                                TCHAR szBuf[40];

                                if(dat->iNextQueuedEvent >= dat->iEventQueueSize) {
                                    dat->hQueuedEvents = realloc(dat->hQueuedEvents, (dat->iEventQueueSize + 10) * sizeof(HANDLE));
                                    dat->iEventQueueSize += 10;
                                }
                                dat->hQueuedEvents[dat->iNextQueuedEvent++] = (HANDLE)lParam;
                                mir_sntprintf(szBuf, safe_sizeof(szBuf), TranslateT("Message Log is frozen (%d queued)"), dat->iNextQueuedEvent);
                                SetDlgItemText(hwndDlg, IDC_LOGFROZENTEXT, szBuf);
                                RedrawWindow(GetDlgItem(hwndDlg, IDC_LOGFROZENTEXT), NULL, NULL, RDW_INVALIDATE);
                            }
                            if(dbei.eventType == EVENTTYPE_MESSAGE && !(dbei.flags & DBEF_SENT)) {
                                dat->stats.iReceived++;
                                dat->stats.iReceivedBytes += dat->stats.lastReceivedChars;
                            }
                        }
                        else
                            SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                    }
                    else
                        SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                    
                    /*
                    if (dat->iTabID == -1)
                        MessageBoxA(0, "DBEVENTADDED Critical: iTabID == -1", "Error", MB_OK);
                    */
                    //dat->dwLastActivity = GetTickCount();
                    //dat->pContainer->dwLastActivity = dat->dwLastActivity;
                    // tab flashing
                    if ((/* IsIconic(hwndContainer) ||*/ TabCtrl_GetCurSel(hwndTab) != dat->iTabID) && !(dbei.flags & DBEF_SENT) && !fIsStatusChangeEvent) {
                        switch (dbei.eventType) {
                            case EVENTTYPE_MESSAGE:
                                dat->iFlashIcon = myGlobals.g_IconMsgEvent;
                                break;
                            case EVENTTYPE_URL:
                                dat->iFlashIcon = myGlobals.g_IconUrlEvent;
                                break;
                            case EVENTTYPE_FILE:
                                dat->iFlashIcon = myGlobals.g_IconFileEvent;
                                break;
                            default:
                                dat->iFlashIcon = myGlobals.g_IconMsgEvent;
                                break;
                        }
                        SetTimer(hwndDlg, TIMERID_FLASHWND, TIMEOUT_FLASHWND, NULL);
                        dat->mayFlashTab = TRUE;
                    }
                    /*
                     * try to flash the contact list...
                     */

                    FlashOnClist(hwndDlg, dat, (HANDLE)lParam, &dbei);
                    /*
                     * autoswitch tab if option is set AND container is minimized (otherwise, we never autoswitch)
                     * never switch for status changes...
                     */
                    if(!(dbei.flags & DBEF_SENT) && !fIsStatusChangeEvent) {
                        if(IsIconic(hwndContainer) && !IsZoomed(hwndContainer) && myGlobals.m_AutoSwitchTabs && m_pContainer->hwndActive != hwndDlg) {
                            int iItem = GetTabIndexFromHWND(GetParent(hwndDlg), hwndDlg);
                            if (iItem >= 0) {
                                TabCtrl_SetCurSel(GetParent(hwndDlg), iItem);
                                ShowWindow(m_pContainer->hwndActive, SW_HIDE);
                                m_pContainer->hwndActive = hwndDlg;
                                SendMessage(hwndContainer, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                                m_pContainer->dwFlags |= CNT_DEFERREDTABSELECT;
                            }
                        }
                    }
                    /*
                     * flash window if it is not focused
                     */
                    if ((GetActiveWindow() != hwndContainer || GetForegroundWindow() != hwndContainer) && !(dbei.flags & DBEF_SENT) && !fIsStatusChangeEvent) {
                        if (!(m_pContainer->dwFlags & CNT_NOFLASH))
                            FlashContainer(m_pContainer, 1, 0);
                        // XXX set the message icon in the container
                        SendMessage(hwndContainer, DM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_EVENT_MESSAGE));
                        m_pContainer->dwFlags |= CNT_NEED_UPDATETITLE;
                    }
                    /*
                     * play a sound
                     */
                    if (dbei.eventType == EVENTTYPE_MESSAGE && !(dbei.flags & (DBEF_SENT)))
                        PostMessage(hwndDlg, DM_PLAYINCOMINGSOUND, 0, 0);
                }
                return 0;
            }
        case WM_TIMER:
            /*
             * timer id for message timeouts is composed like:
             * for single message sends: basevalue (TIMERID_MSGSEND) + send queue index
             * for multisend: each send entry (hContact/hSendID pair) has its own timer starting at TIMERID_MSGSEND + NR_SENDJOBS in blocks
             * of SENDJOBS_MAX_SENDS)
             */
           
            if (wParam >= TIMERID_AWAYMSG && wParam <= TIMERID_AWAYMSG + 2) {
                POINT pt;
                RECT rc, rcNick;

                KillTimer(hwndDlg, wParam);
                dat->dwFlagsEx &= ~MWF_SHOW_AWAYMSGTIMER;
                GetCursorPos(&pt);
                GetWindowRect(GetDlgItem(hwndDlg, IDC_PANELSTATUS), &rc);
                GetWindowRect(GetDlgItem(hwndDlg, IDC_PANELNICK), &rcNick);
                if(wParam == TIMERID_AWAYMSG + 2 && PtInRect(&rc, pt) && pt.x >= rc.right - 20) {
                    DBVARIANT dbv = {0};
                    
                    if(!DBGetContactSettingTString(dat->hContact, dat->szProto, "MirVer", &dbv)) {
                        SendMessage(hwndDlg, DM_ACTIVATETOOLTIP, IDC_PANELSTATUS + 1, (LPARAM)dbv.ptszVal);
                        DBFreeVariant(&dbv);
                    }
                    else
                        SendMessage(hwndDlg, DM_ACTIVATETOOLTIP, IDC_PANELSTATUS + 1, (LPARAM)TranslateT("Unknown client"));
                }
                else if(wParam == TIMERID_AWAYMSG && PtInRect(&rc, pt)) {
                    if(GetTickCount() - dat->lastRetrievedStatusMsg > 60000) {
                        SendMessage(hwndDlg, DM_ACTIVATETOOLTIP, 0, (LPARAM)TranslateT("Retrieving..."));
                        if(!(dat->hProcessAwayMsg = (HANDLE)CallContactService(dat->bIsMeta ? dat->hSubContact : dat->hContact, PSS_GETAWAYMSG, 0, 0)))
                            SendMessage(hwndDlg, DM_ACTIVATETOOLTIP, 0, (LPARAM)myGlobals.m_szNoStatus);
                        dat->lastRetrievedStatusMsg = GetTickCount();
                    }
                    else
                        SendMessage(hwndDlg, DM_ACTIVATETOOLTIP, 0, 0);
                }
                else if(wParam == (TIMERID_AWAYMSG + 1) && PtInRect(&rcNick, pt) && dat->xStatus > 0) {
                    TCHAR szBuffer[1025];
                    DBVARIANT dbv;
                    mir_sntprintf(szBuffer, safe_sizeof(szBuffer), _T("No extended status message available"));
                    if(!DBGetContactSettingTString(dat->bIsMeta ? dat->hSubContact : dat->hContact, dat->bIsMeta ? dat->szMetaProto : dat->szProto, "XStatusMsg", &dbv)) {
                        if(lstrlen(dbv.ptszVal) > 2)
                            mir_sntprintf(szBuffer, safe_sizeof(szBuffer), _T("%s"), dbv.ptszVal);
                        DBFreeVariant(&dbv);
                    }
                    SendMessage(hwndDlg, DM_ACTIVATETOOLTIP, IDC_PANELNICK, (LPARAM)szBuffer);
                }
                break;
            }
            if (wParam >= TIMERID_MSGSEND) {
                int iIndex = wParam - TIMERID_MSGSEND;

                if(iIndex < NR_SENDJOBS) {      // single sendjob timer
                    KillTimer(hwndDlg, wParam);
                    mir_snprintf(sendJobs[iIndex].szErrorMsg, sizeof(sendJobs[iIndex].szErrorMsg), Translate("Delivery failure: %s"), Translate("The message send timed out"));
                    sendJobs[iIndex].iStatus = SQ_ERROR;
                    if(!nen_options.iNoSounds && !(m_pContainer->dwFlags & CNT_NOSOUND))
                        SkinPlaySound("SendError");
                    if(!(dat->dwFlags & MWF_ERRORSTATE))
                        HandleQueueError(hwndDlg, dat, iIndex);
                    break;
                }
                else if(wParam >= TIMERID_MULTISEND_BASE) {
                    int iJobIndex = (wParam - TIMERID_MULTISEND_BASE) / SENDJOBS_MAX_SENDS;
                    int iSendIndex = (wParam - TIMERID_MULTISEND_BASE) % SENDJOBS_MAX_SENDS;
                    KillTimer(hwndDlg, wParam);
                    _DebugPopup(dat->hContact, "MULTISEND timeout: %d, %d", iJobIndex, iSendIndex);
                    break;
                }
            } else if (wParam == TIMERID_FLASHWND) {
                if (dat->mayFlashTab)
                    FlashTab(dat, hwndTab, dat->iTabID, &dat->bTabFlash, TRUE, dat->hTabIcon);
                break;
            } else if (wParam == TIMERID_TYPE) {
                if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON && GetTickCount() - dat->nLastTyping > TIMEOUT_TYPEOFF) {
                    NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
                }
                if (dat->showTyping) {
                    if (dat->nTypeSecs > 0) {
                        dat->nTypeSecs--;
                        if (GetForegroundWindow() == hwndContainer)
                            SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
                    } else {
                        dat->showTyping = 0;
                        DM_UpdateLastMessage(hwndDlg, dat);
                        SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
                        HandleIconFeedback(hwndDlg, dat, (HICON)-1);
                        SendMessage(hwndContainer, DM_UPDATETITLE, 0, 0);
                        if(!(m_pContainer->dwFlags & CNT_NOFLASH) && dat->showTypingWin)
                            ReflashContainer(m_pContainer);
                    }
                } else {
                    if (dat->nTypeSecs > 0) {
                        TCHAR szBuf[256];

                        _sntprintf(szBuf, safe_sizeof(szBuf), TranslateT("%s is typing..."), dat->szNickname);
                        szBuf[255] = 0;
                        dat->nTypeSecs--;
                        if(m_pContainer->hwndStatus && m_pContainer->hwndActive == hwndDlg) {
                            SendMessage(m_pContainer->hwndStatus, SB_SETTEXT, 0, (LPARAM) szBuf);
                            SendMessage(m_pContainer->hwndStatus, SB_SETICON, 0, (LPARAM) myGlobals.g_buttonBarIcons[5]);
                            if(m_pContainer->hwndSlist)
                                SendMessage(m_pContainer->hwndSlist, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[5]);
                        }
                        if(IsIconic(hwndContainer) || GetForegroundWindow() != hwndContainer || GetActiveWindow() != hwndContainer) {
                            SetWindowText(hwndContainer, szBuf);
                            m_pContainer->dwFlags |= CNT_NEED_UPDATETITLE;
                            if(!(m_pContainer->dwFlags & CNT_NOFLASH) && dat->showTypingWin)
                                ReflashContainer(m_pContainer);
                        }
                        if (m_pContainer->hwndActive != hwndDlg) {
                            if(dat->mayFlashTab)
                                dat->iFlashIcon = myGlobals.g_IconTypingEvent;
                            HandleIconFeedback(hwndDlg, dat, myGlobals.g_IconTypingEvent);
                        }
                        else {          // active tab may show icon if status bar is disabled
                            if(!m_pContainer->hwndStatus) {
                                if(TabCtrl_GetItemCount(GetParent(hwndDlg)) > 1 || !(m_pContainer->dwFlags & CNT_HIDETABS)) {
                                    HandleIconFeedback(hwndDlg, dat, myGlobals.g_IconTypingEvent);
                                }
                            }
                        }
                        if ((GetForegroundWindow() != hwndContainer) || (m_pContainer->hwndStatus == 0)) {
                            SendMessage(hwndContainer, DM_SETICON, (WPARAM) ICON_BIG, (LPARAM) myGlobals.g_buttonBarIcons[5]);
                            //if(dat->pContainer->hwndSlist)
                            //    SendMessage(dat->pContainer->hwndSlist, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[5]);
                        }
                        dat->showTyping = 1;
                    }
                }
            }
            break;
        case DM_ERRORDECIDED:
            switch (wParam) {
                case MSGERROR_CANCEL:
                case MSGERROR_SENDLATER:
                {
                    int iNextFailed;
                    
                    if(!(dat->dwFlags & MWF_ERRORSTATE))
                        break;
                    
                    if(wParam == MSGERROR_SENDLATER) {
                        if(ServiceExists(BUDDYPOUNCE_SERVICENAME)) {
                            int iLen = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MESSAGE));
                            int iOffset = 0;
                            char *szMessage = (char *)malloc(iLen + 10);
                            if(szMessage) {
                                char *szNote = "The message has been successfully queued for later delivery.";
                                DBEVENTINFO dbei;
                                GetDlgItemTextA(hwndDlg, IDC_MESSAGE, szMessage + iOffset, iLen + 1);
                                CallService(BUDDYPOUNCE_SERVICENAME, (WPARAM)dat->hContact, (LPARAM)szMessage);
                                dbei.cbSize = sizeof(dbei);
                                dbei.eventType = EVENTTYPE_MESSAGE;
                                dbei.flags = DBEF_SENT;
                                dbei.szModule = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) dat->hContact, 0);
                                dbei.timestamp = time(NULL);
                                dbei.cbBlob = lstrlenA(szNote) + 1;
                                dbei.pBlob = (PBYTE) szNote;
                                StreamInEvents(hwndDlg,  0, 1, 1, &dbei);
                                if(!nen_options.iNoSounds && !(m_pContainer->dwFlags & CNT_NOSOUND))
                                    SkinPlaySound("SendMsg");
                                if (dat->hDbEventFirst == NULL)
                                    SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                                SaveInputHistory(hwndDlg, dat, 0, 0);
                                EnableSendButton(hwndDlg, FALSE);

                                if(m_pContainer->hwndActive == hwndDlg)
                                    UpdateReadChars(hwndDlg, dat);
                                SendDlgItemMessage(hwndDlg, IDC_SAVE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[6]);
                                SendDlgItemMessage(hwndDlg, IDC_SAVE, BUTTONADDTOOLTIP, (WPARAM)pszIDCSAVE_close, 0);
                                dat->dwFlags &= ~MWF_SAVEBTN_SAV;
                                free(szMessage);
                            }
                        }
                    }
                    dat->iOpenJobs--;
                    myGlobals.iSendJobCurrent--;
                    if(dat->iCurrentQueueError >= 0 && dat->iCurrentQueueError < NR_SENDJOBS)
                        ClearSendJob(dat->iCurrentQueueError);
                    else
                        _DebugPopup(dat->hContact, "iCurrentQueueError out of bounds (%d)", dat->iCurrentQueueError);
                    dat->iCurrentQueueError = -1;
                    ShowErrorControls(hwndDlg, dat, FALSE);
                    if(wParam != MSGERROR_CANCEL || (wParam == MSGERROR_CANCEL && lParam == 0))
                        SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
                    CheckSendQueue(hwndDlg, dat);
                    if((iNextFailed = FindNextFailedMsg(hwndDlg, dat)) >= 0)
                        HandleQueueError(hwndDlg, dat, iNextFailed);
                    break;
                }
                case MSGERROR_RETRY:
                    {
                        int i, resent = 0;;

                        if(!(dat->dwFlags & MWF_ERRORSTATE))
                            break;
                        
                        if(dat->iCurrentQueueError >= 0 && dat->iCurrentQueueError < NR_SENDJOBS) {
                            for (i = 0; i < sendJobs[dat->iCurrentQueueError].sendCount; i++) {
                                if (sendJobs[dat->iCurrentQueueError].hSendId[i] == NULL && sendJobs[dat->iCurrentQueueError].hContact[i] == NULL)
                                    continue;
                                sendJobs[dat->iCurrentQueueError].hSendId[i] = (HANDLE) CallContactService(sendJobs[dat->iCurrentQueueError].hContact[i], 
                                                                                                           MsgServiceName(sendJobs[dat->iCurrentQueueError].hContact[i], dat, sendJobs[dat->iCurrentQueueError].dwFlags), (dat->sendMode & SMODE_FORCEANSI) ? (sendJobs[dat->iCurrentQueueError].dwFlags & ~PREF_UNICODE) : sendJobs[dat->iCurrentQueueError].dwFlags, (LPARAM) sendJobs[dat->iCurrentQueueError].sendBuffer);
                                resent++;
                            }
                        }
                        else
                            _DebugPopup(dat->hContact, "iCurrentQueueError out of bounds (%d)", dat->iCurrentQueueError);
                        if(resent) {
                            int iNextFailed;
                            SetTimer(hwndDlg, TIMERID_MSGSEND + dat->iCurrentQueueError, myGlobals.m_MsgTimeout, NULL);
                            sendJobs[dat->iCurrentQueueError].iStatus = SQ_INPROGRESS;
                            dat->iCurrentQueueError = -1;
                            ShowErrorControls(hwndDlg, dat, FALSE);
                            SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
                            CheckSendQueue(hwndDlg, dat);
                            if((iNextFailed = FindNextFailedMsg(hwndDlg, dat)) >= 0)
                                HandleQueueError(hwndDlg, dat, iNextFailed);
                        }
                    }
                    break;
            }
            break;
        case DM_SELECTTAB:
            SendMessage(hwndContainer, DM_SELECTTAB, wParam, lParam);       // pass the msg to our container
            return 0;

        case DM_SETLOCALE:
            if(dat->dwFlags & MWF_WASBACKGROUNDCREATE)
                break;
            if (m_pContainer->hwndActive == hwndDlg && myGlobals.m_AutoLocaleSupport && dat->hContact != 0 && hwndContainer == GetForegroundWindow() && hwndContainer == GetActiveWindow()) {
                if (lParam == 0) {
                    if (GetKeyboardLayout(0) != dat->hkl) {
                        ActivateKeyboardLayout(dat->hkl, 0);
                    }
                } else {
                    dat->hkl = (HKL) lParam;
                    ActivateKeyboardLayout(dat->hkl, 0);
                }
            }
            return 0;
        /*
         * return timestamp (in ticks) of last recent message which has not been read yet.
         * 0 if there is none
         * lParam = pointer to a dword receiving the value.
         */
        case DM_QUERYLASTUNREAD:{
                DWORD *pdw = (DWORD *)lParam;
                if(pdw)
                    *pdw = dat->dwTickLastEvent;
                return 0;
            }
        case DM_QUERYCONTAINER: {
                struct ContainerWindowData **pc = (struct ContainerWindowData **) lParam;
                if(pc)
                    *pc = m_pContainer;
                return 0;
            }
        case DM_QUERYCONTAINERHWND: {
                HWND *pHwnd = (HWND *) lParam;
                if(pHwnd)
                    *pHwnd = hwndContainer;
                return 0;
            }
        case DM_QUERYHCONTACT: {
                HANDLE *phContact = (HANDLE *) lParam;
                if(phContact)
                    *phContact = dat->hContact;
                return 0;
            }
        case DM_QUERYSTATUS: {
                WORD *wStatus = (WORD *) lParam;
                if(wStatus)
                    *wStatus = dat->bIsMeta ? dat->wMetaStatus : dat->wStatus;
                return 0;
            }
        case DM_QUERYFLAGS: {
                DWORD *dwFlags = (DWORD *) lParam;
                if(dwFlags)
                    *dwFlags = dat->dwFlags;
                return 0;        
            }
        case DM_CALCMINHEIGHT: {
                UINT height = 0;

                if (dat->showPic)
                    height += (dat->pic.cy > MINSPLITTERY) ? dat->pic.cy : MINSPLITTERY;
                else
                    height += MINSPLITTERY;
                if (dat->showUIElements != 0)
                    height += 30;

                height += (MINLOGHEIGHT + 30);          // log and padding for statusbar etc.
                if(m_pContainer->dwFlags & CNT_INFOPANEL)
                    height += dat->panelHeight;
                if(!(m_pContainer->dwFlags & CNT_NOMENUBAR))
                    height += myGlobals.ncm.iMenuHeight;
                dat->uMinHeight = height;
                return 0;
            }
        case DM_QUERYMINHEIGHT: {
                UINT *puMinHeight = (UINT *) lParam;

                if(puMinHeight)
                    *puMinHeight = dat->uMinHeight;
                return 0;
            }
        case DM_UPDATELASTMESSAGE:
            DM_UpdateLastMessage(hwndDlg, dat);
            return 0;
        case DM_SAVESIZE: {
                RECT rcClient;

                if(dat->dwFlags & MWF_NEEDCHECKSIZE)
                    lParam = 0;

                dat->dwFlags &= ~MWF_NEEDCHECKSIZE;
                if(dat->dwFlags & MWF_WASBACKGROUNDCREATE) {
                    dat->dwFlags &= ~MWF_INITMODE;
                    if(dat->lastMessage)
                        DM_UpdateLastMessage(hwndDlg, dat);
                }
                SendMessage(hwndContainer, DM_QUERYCLIENTAREA, 0, (LPARAM)&rcClient);
                MoveWindow(hwndDlg, rcClient.left, rcClient.top, (rcClient.right - rcClient.left), (rcClient.bottom - rcClient.top), TRUE);
                if(dat->dwFlags & MWF_WASBACKGROUNDCREATE) {
                    dat->dwFlags &= ~MWF_WASBACKGROUNDCREATE;
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    LoadSplitter(hwndDlg, dat);
                    PostMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
                    DM_LoadLocale(hwndDlg, dat);
                    SendMessage(hwndDlg, DM_SETLOCALE, 0, 0);
                    //DM_ScrollToBottom(hwndDlg, dat, 1, 1);
                    if(dat->hwndIEView != 0)
                        SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                    //SetWindowPos(dat->hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                }
                else {
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    if(lParam == 0) {
                        PostMessage(hwndDlg, DM_FORCESCROLL, 0, 0);
                    }
                }
                return 0;
            }
        case DM_CHECKSIZE:
            dat->dwFlags |= MWF_NEEDCHECKSIZE;
            return 0;
        /*
         * sent by the message input area hotkeys. just pass it to our container
         */
        case DM_QUERYPENDING:
            SendMessage(hwndContainer, DM_QUERYPENDING, wParam, lParam);
            return 0;

        case WM_LBUTTONDBLCLK:
            if(GetKeyState(VK_CONTROL) & 0x8000) {
                SendMessage(hwndContainer, WM_CLOSE, 1, 0);
                break;
            }
            if(GetKeyState(VK_SHIFT) & 0x8000) {
                SendMessage(hwndContainer, WM_SYSCOMMAND, IDM_NOTITLE, 0);
                break;
            }
            SendMessage(hwndContainer, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            break;

        case WM_LBUTTONDOWN: 
        {
            POINT tmp; //+ Protogenes
            POINTS cur; //+ Protogenes
            GetCursorPos(&tmp); //+ Protogenes
            cur.x = (SHORT)tmp.x; //+ Protogenes
            cur.y = (SHORT)tmp.y; //+ Protogenes

            SendMessage(dat->hwndTip, TTM_TRACKACTIVATE, FALSE, 0);
            SendMessage(hwndContainer, WM_NCLBUTTONDOWN, HTCAPTION, *((LPARAM*)(&cur))); //+ Protogenes
            break;
        }
        case WM_LBUTTONUP:
        {
            POINT tmp; //+ Protogenes
            POINTS cur; //+ Protogenes
            GetCursorPos(&tmp); //+ Protogenes
            cur.x = (SHORT)tmp.x; //+ Protogenes
            cur.y = (SHORT)tmp.y; //+ Protogenes

            SendMessage(hwndContainer, WM_NCLBUTTONUP, HTCAPTION, *((LPARAM*)(&cur))); //+ Protogenes
            break;
        }

            /*
        case WM_LBUTTONDOWN:
            dat->dwFlags |= MWF_MOUSEDOWN;
            GetCursorPos(&dat->ptLast);
            SetCapture(hwndDlg);
            SendMessage(dat->hwndTip, TTM_TRACKACTIVATE, FALSE, 0);
            break;
        case WM_LBUTTONUP:
            dat->dwFlags &= ~MWF_MOUSEDOWN;
            ReleaseCapture();
            break;
            */

        case WM_RBUTTONUP: {
            POINT pt;
            int iSelection;
            HMENU subMenu;
            int isHandled;
            RECT rcPicture, rcPanelPicture, rcPanelNick;
            int menuID = 0;
            
            GetWindowRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), &rcPicture);
            GetWindowRect(GetDlgItem(hwndDlg, IDC_PANELPIC), &rcPanelPicture);
            GetWindowRect(GetDlgItem(hwndDlg, IDC_PANELNICK), &rcPanelNick);
            rcPanelNick.left = rcPanelNick.right - 30;
            GetCursorPos(&pt);
            
            if(PtInRect(&rcPicture, pt))
                menuID = MENU_PICMENU;
            else if(PtInRect(&rcPanelPicture, pt) || PtInRect(&rcPanelNick, pt))
                menuID = MENU_PANELPICMENU;
            
            if((menuID == MENU_PICMENU && ((dat->ace ? dat->ace->hbmPic : myGlobals.g_hbmUnknown) || dat->hOwnPic) && dat->showPic !=0) || (menuID == MENU_PANELPICMENU && dat->dwFlagsEx & MWF_SHOW_INFOPANEL)) {
                int iSelection, isHandled;
                HMENU submenu = 0;

                submenu = GetSubMenu(m_pContainer->hMenuContext, menuID == MENU_PICMENU ? 1 : 11);
                if(menuID == MENU_PANELPICMENU) {
                    int iCount = GetMenuItemCount(submenu);

                    if(iCount < 5) {
                        HMENU visMenu = GetSubMenu(GetSubMenu(m_pContainer->hMenuContext, 1), 0);
                        InsertMenu(submenu, 0, MF_POPUP, (UINT_PTR)visMenu, TranslateT("Show Contact Picture"));
                    }
                }
                GetCursorPos(&pt);
                MsgWindowUpdateMenu(hwndDlg, dat, submenu, menuID);
                iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
                isHandled = MsgWindowMenuHandler(hwndDlg, dat, iSelection, menuID);
                break;
            }
            subMenu = GetSubMenu(m_pContainer->hMenuContext, 0);

            MsgWindowUpdateMenu(hwndDlg, dat, subMenu, MENU_TABCONTEXT);
            
            iSelection = TrackPopupMenu(subMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
            if(iSelection >= IDM_CONTAINERMENU) {
                DBVARIANT dbv = {0};
                char szIndex[10];
#if defined (_UNICODE)
                char *szKey = "TAB_ContainersW";
#else    
                char *szKey = "TAB_Containers";
#endif    
                _snprintf(szIndex, 8, "%d", iSelection - IDM_CONTAINERMENU);
                if(iSelection - IDM_CONTAINERMENU >= 0) {
                    if(!DBGetContactSettingTString(NULL, szKey, szIndex, &dbv)) {
                        SendMessage(hwndDlg, DM_CONTAINERSELECTED, 0, (LPARAM)dbv.ptszVal);
                        DBFreeVariant(&dbv);
                    }
                }
                
                break;
            }
            isHandled = MsgWindowMenuHandler(hwndDlg, dat, iSelection, MENU_TABCONTEXT);
            break;
        }
        case WM_MOUSEMOVE:
            {
                RECT rc, rcNick;
                POINT pt;
                GetCursorPos(&pt);
                
                /*
                if (dat->pContainer->dwFlags & CNT_NOTITLE && dat->dwFlags & MWF_MOUSEDOWN) {
                    GetWindowRect(dat->pContainer->hwnd, &rc);
                    MoveWindow(dat->pContainer->hwnd, rc.left - (dat->ptLast.x - pt.x), rc.top - (dat->ptLast.y - pt.y), rc.right - rc.left, rc.bottom - rc.top, TRUE);
                    dat->ptLast = pt;
                }
                else*/ if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL && !(dat->dwFlagsEx & MWF_SHOW_INFONOTES)) {
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_PANELSTATUS), &rc);
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_PANELNICK), &rcNick);
                    if(PtInRect(&rc, pt) && (myGlobals.m_DoStatusMsg || dat->hClientIcon)) { 
                        if(!(dat->dwFlagsEx & MWF_SHOW_AWAYMSGTIMER)) {
                            if(dat->hClientIcon && pt.x >= rc.right - 20)
                                SetTimer(hwndDlg, TIMERID_AWAYMSG + 2, 500, 0);
                            else
                                SetTimer(hwndDlg, TIMERID_AWAYMSG, 500, 0);
                            dat->dwFlagsEx |= MWF_SHOW_AWAYMSGTIMER;
                        }
                        break;
                    }
                    else if(PtInRect(&rcNick, pt) && myGlobals.m_DoStatusMsg) {
                        if(!(dat->dwFlagsEx & MWF_SHOW_AWAYMSGTIMER)) {
                            SetTimer(hwndDlg, TIMERID_AWAYMSG + 1, 500, 0);
                            dat->dwFlagsEx |= MWF_SHOW_AWAYMSGTIMER;
                        }
                        break;
                    }
                    else if(IsWindowVisible(dat->hwndTip)) {
                        GetWindowRect(GetDlgItem(hwndDlg, IDC_PANELSTATUS), &rc);
                        if(!PtInRect(&rc, pt))
                            SendMessage(dat->hwndTip, TTM_TRACKACTIVATE, FALSE, 0);
                    }
                }
                break;
            }
        case WM_CTLCOLOREDIT:
            {	
                if((HWND)lParam != GetDlgItem(hwndDlg,IDC_NOTES)) 
                    break;
                if(dat->theme.fontColors != NULL)
                    SetTextColor((HDC)wParam, dat->theme.fontColors[MSGFONTID_MESSAGEAREA]);
                SetBkColor((HDC)wParam, dat->inputbg);
                return (BOOL)dat->hInputBkgBrush;
            }
        case WM_MEASUREITEM:
            {
                LPMEASUREITEMSTRUCT lpmi = (LPMEASUREITEMSTRUCT) lParam;
                lpmi->itemHeight = 0;
                lpmi->itemWidth  += 6;
                return CallService(MS_CLIST_MENUMEASUREITEM, wParam, lParam);
            }
        case WM_NCHITTEST:
            SendMessage(hwndContainer, WM_NCHITTEST, wParam, lParam);
            break;
        case WM_DRAWITEM:
            return MsgWindowDrawHandler(wParam, lParam, hwndDlg, dat);
        case WM_APPCOMMAND:
        {
            DWORD cmd = GET_APPCOMMAND_LPARAM(lParam);
            if(cmd == APPCOMMAND_BROWSER_BACKWARD || cmd == APPCOMMAND_BROWSER_FORWARD) {
                SendMessage(hwndContainer, DM_SELECTTAB, cmd == APPCOMMAND_BROWSER_BACKWARD ? DM_SELECT_PREV : DM_SELECT_NEXT, 0);
                return 1;
            }
            break;
        }
        case WM_HELP:
            PostMessage(hwndContainer, WM_COMMAND, ID_HELP_MESSAGEWINDOWHELP, 0);
            break;
        case WM_COMMAND:

            if(!dat)
                break;
            
            if (CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(LOWORD(wParam), MPCF_CONTACTMENU), (LPARAM) dat->hContact))
                break;
            switch (LOWORD(wParam)) {
                case IDOK:
                    {
                    int bufSize = 0, memRequired = 0, isUnicode = 0, isRtl = 0;
                    char *streamOut = NULL;
                    TCHAR *decoded = NULL, *converted = NULL;
                    FINDTEXTEXA fi = {0};
                    int final_sendformat = dat->SendFormat;
                    HWND hwndEdit = GetDlgItem(hwndDlg, IDC_MESSAGE);
                    PARAFORMAT2 pf2 = {0};

                    // don't parse text formatting when the message contains curly braces - these are used by the rtf syntax
                    // and the parser currently cannot handle them properly in the text - XXX needs to be fixed later.
                    
                    fi.chrg.cpMin = 0;
                    fi.chrg.cpMax = -1;
                    fi.lpstrText = "{";
                    final_sendformat = SendDlgItemMessageA(hwndDlg, IDC_MESSAGE, EM_FINDTEXTEX, FR_DOWN, (LPARAM)&fi) == -1 ? final_sendformat : 0;
                    fi.lpstrText = "}";
                    final_sendformat = SendDlgItemMessageA(hwndDlg, IDC_MESSAGE, EM_FINDTEXTEX, FR_DOWN, (LPARAM)&fi) == -1 ? final_sendformat : 0;
                    
                    if (GetSendButtonState(hwndDlg) == PBS_DISABLED)
                        break;

#if defined( _UNICODE )
                    streamOut = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, final_sendformat ? 0 : (CP_UTF8 << 16) | (SF_TEXT|SF_USECODEPAGE));
                    if(streamOut != NULL) {
                        decoded = Utf8_Decode(streamOut);
                        if(decoded != NULL) {
                            if(final_sendformat)
                                DoRtfToTags(decoded, dat);
                            DoTrimMessage(decoded);
                            bufSize = WideCharToMultiByte(dat->codePage, 0, decoded, -1, dat->sendBuffer, 0, 0, 0);
                            if(myGlobals.m_Send7bitStrictAnsi) {
                                if(!IsUnicodeAscii(decoded, lstrlenW(decoded))) {
                                    isUnicode = TRUE;
                                    memRequired = bufSize + ((lstrlenW(decoded) + 1) * sizeof(WCHAR));
                                }
                                else {
                                    isUnicode = FALSE;
                                    memRequired = bufSize;
                                }
                            }
                            else {
                                isUnicode = TRUE;
                                memRequired = bufSize + ((lstrlenW(decoded) + 1) * sizeof(WCHAR));
                            }

                            /* 
                             * try to detect RTL
                             */

                            SendMessage(hwndEdit, WM_SETREDRAW, FALSE, 0);
                            pf2.cbSize = sizeof(pf2);
                            pf2.dwMask = PFM_RTLPARA;
                            SendMessage(hwndEdit, EM_SETSEL, 0, -1);
                            SendMessage(hwndEdit, EM_GETPARAFORMAT, 0, (LPARAM)&pf2);
                            if(pf2.wEffects & PFE_RTLPARA)
                                isRtl = RTL_Detect(decoded);
                            SendMessage(hwndEdit, WM_SETREDRAW, TRUE, 0);
                            SendMessage(hwndEdit, EM_SETSEL, -1, -1);
                            InvalidateRect(hwndEdit, NULL, FALSE);

                            if(memRequired > dat->iSendBufferSize) {
                                dat->sendBuffer = (char *) realloc(dat->sendBuffer, memRequired);
                                dat->iSendBufferSize = memRequired;
                            }
                            WideCharToMultiByte(dat->codePage, 0, decoded, -1, dat->sendBuffer, bufSize, 0, 0);
                            if(isUnicode)
                                CopyMemory(&dat->sendBuffer[bufSize], decoded, (lstrlenW(decoded) + 1) * sizeof(WCHAR));
                            free(decoded);
                        }
                        free(streamOut);
                    }
#else                    
                    streamOut = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, final_sendformat ? (SF_RTFNOOBJS|SFF_PLAINRTF) : (SF_TEXT));
                    if(streamOut != NULL) {
                        converted = (TCHAR *)malloc((lstrlenA(streamOut) + 2)* sizeof(TCHAR));
                        if(converted != NULL) {
                            _tcscpy(converted, streamOut);
                            if(final_sendformat) {
                                DoRtfToTags(converted, dat);
                                DoTrimMessage(converted);
                            }
                            bufSize = memRequired = lstrlenA(converted) + 1;
                            if(memRequired > dat->iSendBufferSize) {
                                dat->sendBuffer = (char *) realloc(dat->sendBuffer, memRequired);
                                dat->iSendBufferSize = memRequired;
                            }
                            CopyMemory(dat->sendBuffer, converted, bufSize);
                            free(converted);
                        }
                        free(streamOut);
                    }
#endif  // unicode
                    if (dat->sendBuffer[0] == 0 || memRequired == 0)
                        break;

                    if (dat->sendMode & SMODE_SENDLATER) {
#if defined(_UNICODE)
                        if(ServiceExists("BuddyPounce/AddToPounce"))
                            CallService("BuddyPounce/AddToPounce", (WPARAM)dat->hContact, (LPARAM)dat->sendBuffer);
#else
                        if(ServiceExists("BuddyPounce/AddToPounce"))
                            CallService("BuddyPounce/AddToPounce", (WPARAM)dat->hContact, (LPARAM)dat->sendBuffer);
#endif
                        if(dat->hwndIEView != 0 && m_pContainer->hwndStatus) {
                            SendMessage(m_pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM)Translate("Message saved for later delivery"));
                        }
                        else
                            LogErrorMessage(hwndDlg, dat, -1, Translate("Message saved for later delivery"));
                        
                        SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
                        break;
                    }
                    if (dat->sendMode & SMODE_CONTAINER && m_pContainer->hwndActive == hwndDlg && GetForegroundWindow() == hwndContainer) {
                        HWND contacthwnd;
                        TCITEM tci;
                        BOOL bOldState;
                        int tabCount = TabCtrl_GetItemCount(hwndTab), i;
                        char *szFromStream = NULL;

#if defined(_UNICODE)
                        szFromStream = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, dat->SendFormat ? 0 : (CP_UTF8 << 16) | (SF_TEXT|SF_USECODEPAGE));
#else
                        szFromStream = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, dat->SendFormat ? (SF_RTFNOOBJS|SFF_PLAINRTF) : (SF_TEXT));
#endif
                        ZeroMemory((void *)&tci, sizeof(tci));
                        tci.mask = TCIF_PARAM;
   
                        for (i = 0; i < tabCount; i++) {   
                           TabCtrl_GetItem(hwndTab, i, &tci);
                           // get the contact from the tabs lparam which hopefully is the tabs hwnd so we can get its userdata.... hopefully
                           contacthwnd = (HWND)tci.lParam;
                           if (IsWindow(contacthwnd)) {   
                              // if the contact hwnd is the current contact then ignore it and let the normal code deal with the msg
                              if (contacthwnd != hwndDlg) {
#if defined(_UNICODE)
                                 SETTEXTEX stx = {ST_DEFAULT, CP_UTF8};
#else
                                 SETTEXTEX stx = {ST_DEFAULT, CP_ACP};
#endif                                 
                                 // send the buffer to the contacts msg typing area
                                 SendDlgItemMessage(contacthwnd, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)szFromStream);
                                 // enable the IDOK
                                 bOldState = GetSendButtonState(contacthwnd);
                                 EnableSendButton(contacthwnd, 1);
                                 // simulate an ok
                                 // SendDlgItemMessage(contacthwnd, IDOK, BM_CLICK, 0, 0);
                                 SendMessage(contacthwnd, WM_COMMAND, IDOK, 0);
								 EnableSendButton(contacthwnd, bOldState == PBS_DISABLED ? 0 : 1);
                              }
                           }
                        }
                        if(szFromStream)
                            free(szFromStream);
                    }
// END /all /MOD
                    if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON) {
                        NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
                    }
                    DeletePopupsForContact(dat->hContact, PU_REMOVE_ON_SEND);
                    if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "allow_sendhook", 0)) {
                        int result = TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_CUSTOM, MAKELONG((isUnicode ? PREF_UNICODE : 0) | (isRtl ? PREF_RTL : 0), tabMSG_WINDOW_EVT_CUSTOM_BEFORESEND));
                        if(result)
                            return TRUE;
                    }
                    AddToSendQueue(hwndDlg, dat, memRequired, (isUnicode ? PREF_UNICODE : 0) | (isRtl ? PREF_RTL : 0));
                    return TRUE;
                    }
                case IDC_QUOTE:
                    {   CHARRANGE sel;
                        TCHAR *szQuoted, *szText;
                        char *szFromStream = NULL;
                        HANDLE hDBEvent = 0;
#ifdef _UNICODE
                        TCHAR *szConverted;
                        int iAlloced = 0;
                        unsigned int iSize = 0;
                        SETTEXTEX stx = {ST_SELECTION, 1200};
#endif                        
                        if(dat->hwndIEView != 0) {                 // IEView quoting support..
                            TCHAR *selected = 0, *szQuoted = 0;
                            IEVIEWEVENT event;
                            ZeroMemory((void *)&event, sizeof(event));
                            event.cbSize = sizeof(IEVIEWEVENT);
                            event.hwnd = dat->hwndIEView;
                            event.hContact = dat->hContact;
                            event.dwFlags = 0;
#if !defined(_UNICODE)
                            event.dwFlags |= IEEF_NO_UNICODE;
#endif                            
                            event.iType = IEE_GET_SELECTION;
                            selected = (TCHAR *)CallService(MS_IEVIEW_EVENT, 0, (LPARAM)&event);
                            if(selected != NULL) {
                                szQuoted = QuoteText(selected, 64, 0);
#if defined(_UNICODE)
                                SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)szQuoted);
#else
                                SendDlgItemMessageA(hwndDlg, IDC_MESSAGE, EM_REPLACESEL, TRUE, (LPARAM)szQuoted);
#endif                                
                                if(szQuoted)
                                    free(szQuoted);
                                break;
                            }
                            else {
                                hDBEvent = (HANDLE)CallService(MS_DB_EVENT_FINDLAST, (WPARAM)dat->hContact, 0);
                                goto quote_from_last;
                            }
                        }
                        if (dat->hDbEventLast==NULL) 
                            break;
                        else
                            hDBEvent = dat->hDbEventLast;
quote_from_last:                        
                        SendDlgItemMessage(hwndDlg,IDC_LOG,EM_EXGETSEL,0,(LPARAM)&sel);
                        if (sel.cpMin==sel.cpMax) {
                            DBEVENTINFO dbei={0};
                            int iDescr;
                            
                            dbei.cbSize=sizeof(dbei);
                            dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)hDBEvent,0);
                            szText=(TCHAR *)malloc((dbei.cbBlob+1) * sizeof(TCHAR));   //URLs are made one char bigger for crlf
                            dbei.pBlob=(BYTE *)szText;
                            CallService(MS_DB_EVENT_GET,(WPARAM)hDBEvent,(LPARAM)&dbei);
#ifdef _UNICODE
                            iSize = strlen((char *)dbei.pBlob) + 1;
                            if(iSize != dbei.cbBlob)
                                szConverted = (TCHAR *) &dbei.pBlob[iSize];
                            else {
                                szConverted = (TCHAR *)malloc(sizeof(TCHAR) * iSize);
                                iAlloced = TRUE;
                                MultiByteToWideChar(CP_ACP, 0, (char *) dbei.pBlob, -1, szConverted, iSize);
                            }
#endif                            
                            if (dbei.eventType==EVENTTYPE_URL) {
                                iDescr=lstrlenA((char *)szText);
                                MoveMemory(szText+iDescr+2,szText+iDescr+1,dbei.cbBlob-iDescr-1);
                                szText[iDescr]='\r'; szText[iDescr+1]='\n';
#ifdef _UNICODE
                                szConverted = (TCHAR *)malloc(sizeof(TCHAR) * (1 + lstrlenA((char *)szText)));
                                MultiByteToWideChar(CP_ACP, 0, (char *) szText, -1, szConverted, 1 + lstrlenA((char *)szText));
                                iAlloced = TRUE;
#endif                                
                            }
                            if (dbei.eventType==EVENTTYPE_FILE) {
                                iDescr=lstrlenA((char *)(szText+sizeof(DWORD)));
                                MoveMemory(szText,szText+sizeof(DWORD),iDescr);
                                MoveMemory(szText+iDescr+2,szText+sizeof(DWORD)+iDescr,dbei.cbBlob-iDescr-sizeof(DWORD)-1);
                                szText[iDescr]='\r'; szText[iDescr+1]='\n';
#ifdef _UNICODE
                                szConverted = (TCHAR *)malloc(sizeof(TCHAR) * (1 + lstrlenA((char *)szText)));
                                MultiByteToWideChar(CP_ACP, 0, (char *) szText, -1, szConverted, 1 + lstrlenA((char *)szText));
                                iAlloced = TRUE;
#endif                                
                            }
#ifdef _UNICODE
                            szQuoted = QuoteText(szConverted, 64, 0);
                            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)szQuoted);
#else                              
                            szQuoted=QuoteText(szText, 64, 0);
                            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_REPLACESEL, TRUE, (LPARAM)szQuoted);
#endif
                            
                            free(szText);
                            free(szQuoted);
#ifdef _UNICODE
                            if(iAlloced)
                                free(szConverted);
#endif                            
                        } else {
#ifdef _UNICODE
                            wchar_t *converted = 0;
                            szFromStream = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_LOG), dat, SF_TEXT|SF_USECODEPAGE|SFF_SELECTION);
                            converted = Utf8_Decode(szFromStream);
                            szQuoted = QuoteText(converted, 64, 0);
                            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)szQuoted);
                            free(szQuoted);
                            free(converted);
                            free(szFromStream);
#else
                            szFromStream = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_LOG), dat, SF_TEXT|SFF_SELECTION);
                            szQuoted=QuoteText(szFromStream, 64, 0);
                            SendDlgItemMessageA(hwndDlg, IDC_MESSAGE, EM_REPLACESEL, TRUE, (LPARAM)szQuoted);
                            free(szQuoted);
                            free(szFromStream);
#endif
                        }
                        SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                        break;
                    }
    			case IDC_FONTBOLD:
    			case IDC_FONTITALIC:
    			case IDC_FONTUNDERLINE:
    				{
    					CHARFORMAT2 cf = {0}, cfOld = {0};
                        int cmd = LOWORD(wParam);
                        BOOL isBold, isItalic, isUnderline;
    					cf.cbSize = sizeof(CHARFORMAT2);
                        
                        if(dat->SendFormat == 0)            // dont use formatting if disabled
                            break;

                        cfOld.cbSize = sizeof(CHARFORMAT2);
                        cfOld.dwMask = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
                        SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfOld);
                        isBold = (cfOld.dwEffects & CFE_BOLD) && (cfOld.dwMask & CFM_BOLD);
                        isItalic = (cfOld.dwEffects & CFE_ITALIC) && (cfOld.dwMask & CFM_ITALIC);
                        isUnderline = (cfOld.dwEffects & CFE_UNDERLINE) && (cfOld.dwMask & CFM_UNDERLINE);
                        
                        if(cmd == IDC_FONTBOLD && !IsWindowEnabled(GetDlgItem(hwndDlg,IDC_FONTBOLD))) 
    						break;
    					if(cmd == IDC_FONTITALIC && !IsWindowEnabled(GetDlgItem(hwndDlg,IDC_FONTITALIC))) 
    						break;
    					if(cmd == IDC_FONTUNDERLINE && !IsWindowEnabled(GetDlgItem(hwndDlg,IDC_FONTUNDERLINE))) 
    						break;
                        if(cmd == IDC_FONTBOLD) {
                            cf.dwEffects = isBold ? 0 : CFE_BOLD;
                            cf.dwMask = CFM_BOLD;
                            CheckDlgButton(hwndDlg, IDC_FONTBOLD, !isBold);
                        }
                        else if(cmd == IDC_FONTITALIC) {
                            cf.dwEffects = isItalic ? 0 : CFE_ITALIC;
                            cf.dwMask = CFM_ITALIC;
                            CheckDlgButton(hwndDlg, IDC_FONTITALIC, !isItalic);
                        }
                        else if(cmd == IDC_FONTUNDERLINE) {
                            cf.dwEffects = isUnderline ? 0 : CFE_UNDERLINE;
                            cf.dwMask = CFM_UNDERLINE;
                            CheckDlgButton(hwndDlg, IDC_FONTUNDERLINE, !isUnderline);
                        }
                        SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
                        break;
    				}
                case IDC_FONTFACE:
                    {
                        HMENU submenu = GetSubMenu(m_pContainer->hMenuContext, 7);
                        RECT rc;
                        int iSelection, i;
                        CHARFORMAT2 cf = {0};
                        cf.cbSize = sizeof(CHARFORMAT2);
                        cf.dwMask = CFM_COLOR;
                        cf.dwEffects = 0;
                        
                        GetWindowRect(GetDlgItem(hwndDlg, IDC_FONTFACE), &rc);
                        iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, rc.left, rc.bottom, 0, hwndDlg, NULL);
                        if(iSelection == ID_FONT_CLEARALLFORMATTING) {
                            cf.dwMask = CFM_BOLD | CFM_COLOR | CFM_ITALIC | CFM_UNDERLINE;
                            cf.crTextColor = DBGetContactSettingDword(NULL, SRMSGMOD_T, "Font16Col", 0);
                            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
                            break;
                        }
                        if(iSelection == ID_FONT_DEFAULTCOLOR) {
                            int i = 0;
                            cf.crTextColor = DBGetContactSettingDword(NULL, SRMSGMOD_T, "Font16Col", 0);
                            for(i = 0; i < myGlobals.rtf_ctablesize; i++) {
                                if(rtf_ctable[i].clr == cf.crTextColor)
                                    cf.crTextColor = RGB(GetRValue(cf.crTextColor), GetGValue(cf.crTextColor), GetBValue(cf.crTextColor) == 0 ? GetBValue(cf.crTextColor) + 1 : GetBValue(cf.crTextColor) - 1);
                            }
                            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
                            break;
                        }
                        for(i = 0; i < RTF_CTABLE_DEFSIZE; i++) {
                            if(rtf_ctable[i].menuid == iSelection) {
                                cf.crTextColor = rtf_ctable[i].clr;
                                SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
                            }
                        }
                        break;
                    }
                case IDCANCEL: {
                    ShowWindow(hwndContainer, SW_MINIMIZE);
                    return FALSE;
                    break;
                    }
                case IDC_SAVE:
                    SendMessage(hwndDlg, WM_CLOSE, 1, 0);
                    break;
                case IDC_NAME:
                    {
                        if(GetKeyState(VK_SHIFT) & 0x8000)    // copy UIN
                            SendMessage(hwndDlg, DM_UINTOCLIPBOARD, 0, 0);
                        else {
                            RECT rc;
                            HMENU hMenu = (HMENU) CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM) dat->hContact, 0);
                            if(lParam == 0)
                                GetWindowRect(GetDlgItem(hwndDlg, IDC_NAME), &rc);
                            else
                                GetWindowRect((HWND)lParam, &rc);
                            TrackPopupMenu(hMenu, 0, rc.left, rc.bottom, 0, hwndDlg, NULL);
                            DestroyMenu(hMenu);
                        }
                    }
                    break;
                case IDC_HISTORY:
                    // OnO: RTF log
                    CallService(MS_HISTORY_SHOWCONTACTHISTORY, (WPARAM) dat->hContact, 0);
                    break;
                case IDC_SMILEYBTN:
                    if(dat->doSmileys && (myGlobals.g_SmileyAddAvail || dat->hwndIEView != 0)) {
                        HICON hButtonIcon = 0;
                        RECT rc;
                        HANDLE hContact = dat->bIsMeta ? dat->hSubContact : dat->hContact;
                        
                        if(CheckValidSmileyPack(dat->bIsMeta ? dat->szMetaProto : dat->szProto, hContact, &hButtonIcon) != 0 || dat->hwndIEView != 0) {
                            SMADD_SHOWSEL3 smaddInfo = {0};

                            if(lParam == 0)
                                GetWindowRect(GetDlgItem(hwndDlg, IDC_SMILEYBTN), &rc);
                            else
                                GetWindowRect((HWND)lParam, &rc);
                            smaddInfo.cbSize = sizeof(SMADD_SHOWSEL3);
                            smaddInfo.hwndTarget = GetDlgItem(hwndDlg, IDC_MESSAGE);
                            smaddInfo.targetMessage = EM_REPLACESEL;
                            smaddInfo.targetWParam = TRUE;
                            smaddInfo.Protocolname = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
                            smaddInfo.Direction = 0;
                            smaddInfo.xPosition = rc.left;
                            smaddInfo.yPosition = rc.top + 24;
                            smaddInfo.hwndParent = hwndContainer;
                            smaddInfo.hContact = hContact;
                            CallService(MS_SMILEYADD_SHOWSELECTION, (WPARAM)hwndContainer, (LPARAM) &smaddInfo);
							if(hButtonIcon != 0)
								DestroyIcon(hButtonIcon);
                        }
                    }
                    break;
                case IDC_TIME: {
                    RECT rc;
                    HMENU submenu = GetSubMenu(m_pContainer->hMenuContext, 2);
                    int iSelection, isHandled;
                    DWORD dwOldFlags = dat->dwFlags;
                    DWORD dwOldEventIsShown = dat->dwFlagsEx;
                    
                    MsgWindowUpdateMenu(hwndDlg, dat, submenu, MENU_LOGMENU);
                    
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_TIME), &rc);

                    iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, rc.left, rc.bottom, 0, hwndDlg, NULL);
                    isHandled = MsgWindowMenuHandler(hwndDlg, dat, iSelection, MENU_LOGMENU);

                    if(dat->dwFlags != dwOldFlags || dat->dwFlagsEx != dwOldEventIsShown) {
                        if(!DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "mwoverride", 0)) {
                            SaveMessageLogFlags(hwndDlg, dat);
                            WindowList_Broadcast(hMessageWindowList, DM_DEFERREDREMAKELOG, (WPARAM)hwndDlg, (LPARAM)(dat->dwFlags & MWF_LOG_ALL));
                        }
                        else {
                            DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "mwflags", dat->dwFlags & MWF_LOG_ALL);
                            SendMessage(hwndDlg, DM_DEFERREDREMAKELOG, (WPARAM)hwndDlg, 0);
                        }
                    }
                    break;
                }
                case IDC_LOGFROZEN:
                    if(dat->dwFlagsEx & MWF_SHOW_SCROLLINGDISABLED)
                        SendMessage(hwndDlg, DM_REPLAYQUEUE, 0, 0);
                    dat->dwFlagsEx &= ~MWF_SHOW_SCROLLINGDISABLED;
                    ShowWindow(GetDlgItem(hwndDlg, IDC_LOGFROZEN), SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_LOGFROZENTEXT), SW_HIDE);
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    DM_ScrollToBottom(hwndDlg, dat, 1, 1);
                    break;
                case IDC_PROTOMENU: {
                    RECT rc;
                    HMENU submenu = GetSubMenu(m_pContainer->hMenuContext, 4);
                    int iSelection;
                    int iOldGlobalSendFormat = myGlobals.m_SendFormat;
                    
                    if(dat->hContact) {
                        unsigned int iOldIEView = GetIEViewMode(hwndDlg, dat);
                        unsigned int iNewIEView = 0;
                        int iLocalFormat = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "sendformat", 0);
                        int iNewLocalFormat = iLocalFormat;
                        
                        GetWindowRect(GetDlgItem(hwndDlg, IDC_PROTOCOL), &rc);

                        EnableMenuItem(submenu, 0, MF_BYPOSITION | (ServiceExists(MS_IEVIEW_WINDOW) ? MF_ENABLED : MF_GRAYED));
                        
                        CheckMenuItem(submenu, ID_IEVIEWSETTING_USEGLOBAL, MF_BYCOMMAND | (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", 0) == 0 ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_IEVIEWSETTING_FORCEIEVIEW, MF_BYCOMMAND | (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", 0) == 1 ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_IEVIEWSETTING_FORCEDEFAULTMESSAGELOG, MF_BYCOMMAND | (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", 0) == (BYTE)-1 ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_SPLITTER_AUTOSAVEONCLOSE, MF_BYCOMMAND | (myGlobals.m_SplitterSaveOnClose ? MF_CHECKED : MF_UNCHECKED));

                        CheckMenuItem(submenu, ID_MODE_GLOBAL, MF_BYCOMMAND | (!(dat->dwFlagsEx & MWF_SHOW_SPLITTEROVERRIDE) ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_MODE_PRIVATE, MF_BYCOMMAND | (dat->dwFlagsEx & MWF_SHOW_SPLITTEROVERRIDE ? MF_CHECKED : MF_UNCHECKED));

                        /*
                         * formatting menu..
                         */

                        CheckMenuItem(submenu, ID_GLOBAL_BBCODE, MF_BYCOMMAND | ((myGlobals.m_SendFormat) ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_GLOBAL_OFF, MF_BYCOMMAND | ((myGlobals.m_SendFormat == SENDFORMAT_NONE) ? MF_CHECKED : MF_UNCHECKED));

                        CheckMenuItem(submenu, ID_THISCONTACT_GLOBALSETTING, MF_BYCOMMAND | ((iLocalFormat == SENDFORMAT_NONE) ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_THISCONTACT_BBCODE, MF_BYCOMMAND | ((iLocalFormat > 0) ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_THISCONTACT_OFF, MF_BYCOMMAND | ((iLocalFormat == -1) ? MF_CHECKED : MF_UNCHECKED));

                        EnableMenuItem(submenu, ID_FAVORITES_ADDCONTACTTOFAVORITES, LOWORD(dat->dwIsFavoritOrRecent) == 0 ? MF_ENABLED : MF_GRAYED);
                        EnableMenuItem(submenu, ID_FAVORITES_REMOVECONTACTFROMFAVORITES, LOWORD(dat->dwIsFavoritOrRecent) == 0 ? MF_GRAYED : MF_ENABLED);
                        
                        iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, rc.left, rc.bottom, 0, hwndDlg, NULL);
                        switch(iSelection) {
                            case ID_IEVIEWSETTING_USEGLOBAL:
                                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", 0);
                                break;
                            case ID_IEVIEWSETTING_FORCEDEFAULTMESSAGELOG:
                                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", -1);
                                break;
                            case ID_IEVIEWSETTING_FORCEIEVIEW:
                                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", 1);
                                break;
                            case ID_SPLITTER_AUTOSAVEONCLOSE:
                                myGlobals.m_SplitterSaveOnClose ^= 1;
                                DBWriteContactSettingByte(NULL, SRMSGMOD_T, "splitsavemode", (BYTE)myGlobals.m_SplitterSaveOnClose);
                                break;
                            case ID_SPLITTER_SAVENOW:
                                SaveSplitter(hwndDlg, dat);
                                break;
                            case ID_MODE_GLOBAL:
                                dat->dwFlagsEx &= ~(MWF_SHOW_SPLITTEROVERRIDE);
                                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "splitoverride", 0);
								LoadSplitter(hwndDlg, dat);
			                    AdjustBottomAvatarDisplay(hwndDlg, dat);
                                DM_RecalcPictureSize(hwndDlg, dat);
								SendMessage(hwndDlg, WM_SIZE, 0, 0);
                                break;
                            case ID_MODE_PRIVATE:
                                dat->dwFlagsEx |= MWF_SHOW_SPLITTEROVERRIDE;
                                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "splitoverride", 1);
								LoadSplitter(hwndDlg, dat);
			                    AdjustBottomAvatarDisplay(hwndDlg, dat);
                                DM_RecalcPictureSize(hwndDlg, dat);
								SendMessage(hwndDlg, WM_SIZE, 0, 0);
                                break;
                            case ID_GLOBAL_BBCODE:
                                myGlobals.m_SendFormat = SENDFORMAT_BBCODE;
                                break;
                            case ID_GLOBAL_OFF:
                                myGlobals.m_SendFormat = SENDFORMAT_NONE;
                                break;
                            case ID_THISCONTACT_GLOBALSETTING:
                                iNewLocalFormat = 0;
                                break;
                            case ID_THISCONTACT_BBCODE:
                                iNewLocalFormat = SENDFORMAT_BBCODE;
                                break;
                            case ID_THISCONTACT_OFF:
                                iNewLocalFormat = -1;
                                break;
                            case ID_FAVORITES_ADDCONTACTTOFAVORITES:
                                DBWriteContactSettingWord(dat->hContact, SRMSGMOD_T, "isFavorite", 1);
                                dat->dwIsFavoritOrRecent |= 0x00000001;
                                AddContactToFavorites(dat->hContact, dat->szNickname, dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->szStatus, dat->wStatus, LoadSkinnedProtoIcon(dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->bIsMeta ? dat->wMetaStatus : dat->wStatus), 1, myGlobals.g_hMenuFavorites, dat->codePage);
                                break;
                            case ID_FAVORITES_REMOVECONTACTFROMFAVORITES:
                                DBWriteContactSettingWord(dat->hContact, SRMSGMOD_T, "isFavorite", 0);
                                dat->dwIsFavoritOrRecent &= 0xffff0000;
                                DeleteMenu(myGlobals.g_hMenuFavorites, (UINT_PTR)dat->hContact, MF_BYCOMMAND);
                                break;
                        }
                        if(iNewLocalFormat == 0)
                            DBDeleteContactSetting(dat->hContact, SRMSGMOD_T, "sendformat");
                        else if(iNewLocalFormat != iLocalFormat)
                            DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "sendformat", iNewLocalFormat);
                        
                        if(myGlobals.m_SendFormat != iOldGlobalSendFormat)
                            DBWriteContactSettingByte(0, SRMSGMOD_T, "sendformat", (BYTE)myGlobals.m_SendFormat);
                        if(iNewLocalFormat != iLocalFormat || myGlobals.m_SendFormat != iOldGlobalSendFormat) {
                            dat->SendFormat = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "sendformat", myGlobals.m_SendFormat);
                            if(dat->SendFormat == -1)           // per contact override to disable it..
                                dat->SendFormat = 0;
                            WindowList_Broadcast(hMessageWindowList, DM_CONFIGURETOOLBAR, 0, 1);
                        }
                        iNewIEView = GetIEViewMode(hwndDlg, dat);
                        if(iNewIEView != iOldIEView)
                            SwitchMessageLog(hwndDlg, dat, iNewIEView);
                    }
                    break;
                }
                case IDC_TOGGLETOOLBAR:
                    if(lParam == 1)
                        ApplyContainerSetting(m_pContainer, CNT_NOMENUBAR, m_pContainer->dwFlags & CNT_NOMENUBAR ? 0 : 1);
                    else
                        ApplyContainerSetting(m_pContainer, CNT_HIDETOOLBAR, m_pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1);
                    SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                    break;
                case IDC_TOGGLENOTES:
                    if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
                        dat->dwFlagsEx ^= MWF_SHOW_INFONOTES;
                        ConfigurePanel(hwndDlg, dat);
                    }
                    break;
                case IDC_APPARENTMODE:
                {
                    HMENU subMenu = GetSubMenu(m_pContainer->hMenuContext, 10);
                    int iSelection;
                    RECT rc;
                    WORD wOldApparentMode = dat->wApparentMode;
                    int pCaps = CallProtoService(dat->bIsMeta ? dat->szMetaProto : dat->szProto, PS_GETCAPS, PFLAGNUM_1, 0);

                    CheckMenuItem(subMenu, ID_APPARENTMENU_YOUAPPEARALWAYSOFFLINEORHAVETHISCONTACTBLOCKED, MF_BYCOMMAND | (dat->wApparentMode == ID_STATUS_OFFLINE ? MF_CHECKED : MF_UNCHECKED));
                    CheckMenuItem(subMenu, ID_APPARENTMENU_YOUAREALWAYSVISIBLETOTHISCONTACT, MF_BYCOMMAND | (dat->wApparentMode == ID_STATUS_ONLINE ? MF_CHECKED : MF_UNCHECKED));
                    CheckMenuItem(subMenu, ID_APPARENTMENU_YOURSTATUSDETERMINESVISIBLITYTOTHISCONTACT, MF_BYCOMMAND | (dat->wApparentMode == 0 ? MF_CHECKED : MF_UNCHECKED));

                    EnableMenuItem(subMenu, ID_APPARENTMENU_YOUAPPEARALWAYSOFFLINEORHAVETHISCONTACTBLOCKED, MF_BYCOMMAND | (pCaps & PF1_VISLIST ? MF_ENABLED : MF_GRAYED));
                    EnableMenuItem(subMenu, ID_APPARENTMENU_YOUAREALWAYSVISIBLETOTHISCONTACT, MF_BYCOMMAND | (pCaps & PF1_INVISLIST ? MF_ENABLED : MF_GRAYED));
                    
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_APPARENTMODE), &rc);
                    iSelection = TrackPopupMenu(subMenu, TPM_RETURNCMD, rc.left, rc.bottom, 0, hwndDlg, NULL);
                    switch(iSelection) {
                        case ID_APPARENTMENU_YOUAPPEARALWAYSOFFLINEORHAVETHISCONTACTBLOCKED:
                            dat->wApparentMode = ID_STATUS_OFFLINE;
                            break;
                        case ID_APPARENTMENU_YOUAREALWAYSVISIBLETOTHISCONTACT:
                            dat->wApparentMode = ID_STATUS_ONLINE;
                            break;
                        case ID_APPARENTMENU_YOURSTATUSDETERMINESVISIBLITYTOTHISCONTACT:
                            dat->wApparentMode = 0;
                            break;
                    }
                    if(dat->wApparentMode != wOldApparentMode) {
                        //DBWriteContactSettingWord(dat->bIsMeta ? dat->hSubContact : dat->hContact, dat->bIsMeta ? dat->szMetaProto : dat->szProto, "ApparentMode", dat->wApparentMode);
                        CallContactService(dat->bIsMeta ? dat->hSubContact : dat->hContact, PSS_SETAPPARENTMODE, (WPARAM)dat->wApparentMode, 0);
                    }
                    UpdateApparentModeDisplay(hwndDlg, dat);
                    break;
                }
                case IDC_INFOPANELMENU: {
                    RECT rc;
                    HMENU submenu = GetSubMenu(m_pContainer->hMenuContext, 9);
                    int iSelection;
                    BYTE bNewGlobal = m_pContainer->dwFlags & CNT_INFOPANEL ? 1 : 0;
                    BYTE bNewLocal = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "infopanel", 0);
                    BYTE bLocal = bNewLocal, bGlobal = bNewGlobal;
                    DWORD dwOld = dat->dwFlagsEx;
                    
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_NAME), &rc);
                    CheckMenuItem(submenu, ID_GLOBAL_ENABLED, MF_BYCOMMAND | (bGlobal ? MF_CHECKED : MF_UNCHECKED));
                    CheckMenuItem(submenu, ID_GLOBAL_DISABLED, MF_BYCOMMAND | (bGlobal ? MF_UNCHECKED : MF_CHECKED));
                    CheckMenuItem(submenu, ID_THISCONTACT_ALWAYSOFF, MF_BYCOMMAND | (bLocal == (BYTE)-1 ? MF_CHECKED : MF_UNCHECKED));
                    CheckMenuItem(submenu, ID_THISCONTACT_ALWAYSON, MF_BYCOMMAND | (bLocal == 1 ? MF_CHECKED : MF_UNCHECKED));
                    CheckMenuItem(submenu, ID_THISCONTACT_USEGLOBALSETTING, MF_BYCOMMAND | (bLocal == 0 ? MF_CHECKED : MF_UNCHECKED));
                    iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, rc.left, rc.bottom, 0, hwndDlg, NULL);
                    switch(iSelection) {
                        case ID_GLOBAL_ENABLED:
                            bNewGlobal = 1;
                            break;
                        case ID_GLOBAL_DISABLED:
                            bNewGlobal = 0;
                            break;
                        case ID_THISCONTACT_USEGLOBALSETTING:
                            bNewLocal = 0;
                            break;
                        case ID_THISCONTACT_ALWAYSON:
                            bNewLocal = 1;
                            break;
                        case ID_THISCONTACT_ALWAYSOFF:
                            bNewLocal = -1;
                            break;
                        case ID_INFOPANEL_QUICKTOGGLE:
                            dat->dwFlagsEx ^= MWF_SHOW_INFOPANEL;
                            ShowHideInfoPanel(hwndDlg, dat);
                            return 0;
                        default:
                            return 0;
                    }
                    if(bNewGlobal != bGlobal)
                        ApplyContainerSetting(m_pContainer, CNT_INFOPANEL, bNewGlobal ? 1 : 0);
                    if(bNewLocal != bLocal)
                        DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "infopanel", bNewLocal);

                    if(bNewGlobal != bGlobal);
                        //WindowList_Broadcast(hMessageWindowList, DM_SETINFOPANEL, 0, 0);
                    else {
                        dat->dwFlagsEx = GetInfoPanelSetting(hwndDlg, dat) ? dat->dwFlagsEx | MWF_SHOW_INFOPANEL : dat->dwFlagsEx & ~MWF_SHOW_INFOPANEL;
                        if(dat->dwFlagsEx != dwOld) {
                            ShowHideInfoPanel(hwndDlg, dat);
                        }
                    }
                    break;
                }
                case IDC_SENDMENU: {
                    RECT rc;
                    HMENU submenu = GetSubMenu(m_pContainer->hMenuContext, 3);
                    int iSelection;

                    GetWindowRect(GetDlgItem(hwndDlg, IDOK), &rc);
                    CheckMenuItem(submenu, ID_SENDMENU_SENDTOMULTIPLEUSERS, MF_BYCOMMAND | (dat->sendMode & SMODE_MULTIPLE ? MF_CHECKED : MF_UNCHECKED));
                    CheckMenuItem(submenu, ID_SENDMENU_SENDDEFAULT, MF_BYCOMMAND | (dat->sendMode == 0 ? MF_CHECKED : MF_UNCHECKED));
                    CheckMenuItem(submenu, ID_SENDMENU_SENDTOCONTAINER, MF_BYCOMMAND | (dat->sendMode & SMODE_CONTAINER ? MF_CHECKED : MF_UNCHECKED));
                    CheckMenuItem(submenu, ID_SENDMENU_FORCEANSISEND, MF_BYCOMMAND | (dat->sendMode & SMODE_FORCEANSI ? MF_CHECKED : MF_UNCHECKED));
                    EnableMenuItem(submenu, ID_SENDMENU_SENDLATER, MF_BYCOMMAND | (ServiceExists("BuddyPounce/AddToPounce") ? MF_ENABLED : MF_GRAYED));
                    CheckMenuItem(submenu, ID_SENDMENU_SENDLATER, MF_BYCOMMAND | (dat->sendMode & SMODE_SENDLATER ? MF_CHECKED : MF_UNCHECKED));
                    CheckMenuItem(submenu, ID_SENDMENU_SENDWITHOUTTIMEOUTS, MF_BYCOMMAND | (dat->sendMode & SMODE_NOACK ? MF_CHECKED : MF_UNCHECKED));
					{
						char *szFinalProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
						char szServiceName[128];

						mir_snprintf(szServiceName, 128, "%s/SendNudge", szFinalProto);
						EnableMenuItem(submenu, ID_SENDMENU_SENDNUDGE, MF_BYCOMMAND | ((ServiceExists(szServiceName) && ServiceExists(MS_NUDGE_SEND)) ? MF_ENABLED : MF_GRAYED));
					}                    
                    if(lParam)
                        iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, rc.left, rc.bottom, 0, hwndDlg, NULL);
                    else
                        iSelection = HIWORD(wParam);
                    
                    switch(iSelection) {
                        case ID_SENDMENU_SENDTOMULTIPLEUSERS:
                            dat->sendMode ^= SMODE_MULTIPLE;
                            if(dat->sendMode & SMODE_MULTIPLE) {
                                HWND hwndClist = DM_CreateClist(hwndDlg, dat);;
                            }
                            break;
						case ID_SENDMENU_SENDNUDGE:
							SendNudge(dat, hwndDlg);
							break;
                        case ID_SENDMENU_SENDDEFAULT:
                            dat->sendMode = 0;
                            break;
                        case ID_SENDMENU_SENDTOCONTAINER:
                            dat->sendMode ^= SMODE_CONTAINER;
                            break;
                        case ID_SENDMENU_FORCEANSISEND:
                            dat->sendMode ^= SMODE_FORCEANSI;
                            break;
                        case ID_SENDMENU_SENDLATER:
                            dat->sendMode ^= SMODE_SENDLATER;
                            break;
                        case ID_SENDMENU_SENDWITHOUTTIMEOUTS:
                            dat->sendMode ^= SMODE_NOACK;
							if(dat->sendMode & SMODE_NOACK)
								DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "no_ack", 1);
							else
								DBDeleteContactSetting(dat->hContact, SRMSGMOD_T, "no_ack");
                            break;
                    }
                    DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "no_ack", (BYTE)(dat->sendMode & SMODE_NOACK ? 1 : 0));
                    DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "forceansi", (BYTE)(dat->sendMode & SMODE_FORCEANSI ? 1 : 0));
                    SetWindowPos(GetDlgItem(hwndDlg, IDC_MESSAGE), 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE);
                    if(dat->sendMode & SMODE_MULTIPLE || dat->sendMode & SMODE_CONTAINER)
                        RedrawWindow(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW | RDW_ERASE);
                    else {
                        if(IsWindow(GetDlgItem(hwndDlg, IDC_CLIST)))
                            DestroyWindow(GetDlgItem(hwndDlg, IDC_CLIST));
                        RedrawWindow(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW | RDW_ERASE);
                    }
                    SendMessage(hwndContainer, DM_QUERYCLIENTAREA, 0, (LPARAM)&rc);
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    DM_ScrollToBottom(hwndDlg, dat, 1, 1);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), (dat->sendMode & SMODE_MULTIPLE) ? SW_SHOW : SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_CLIST), (dat->sendMode & SMODE_MULTIPLE) ? SW_SHOW : SW_HIDE);
                    if(m_pContainer->bSkinned)
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, TRUE);
                    break;
                    }
                case IDC_ADD:
                    {
                        ADDCONTACTSTRUCT acs = { 0};

                        acs.handle = dat->hContact;
                        acs.handleType = HANDLE_CONTACT;
                        acs.szProto = 0;
                        CallService(MS_ADDCONTACT_SHOW, (WPARAM) hwndDlg, (LPARAM) & acs);
                        if (!DBGetContactSettingByte(dat->hContact, "CList", "NotOnList", 0)) {
                            dat->bNotOnList = FALSE;
                            ShowMultipleControls(hwndDlg, addControls, 2, SW_HIDE);
                            SendMessage(hwndDlg, WM_SIZE, 0, 0);
                        }
                        break;
                    }
                case IDC_CANCELADD:
                    dat->bNotOnList = FALSE;
                    ShowMultipleControls(hwndDlg, addControls, 2, SW_HIDE);
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    break;
                case IDC_TOGGLESIDEBAR:
                {
                    ApplyContainerSetting(m_pContainer, CNT_SIDEBAR, m_pContainer->dwFlags & CNT_SIDEBAR ? 0 : 1);
                    break;
                }
                case IDC_PIC:
                    {
                        RECT rc;
                        int iSelection, isHandled;
                        
                        HMENU submenu = GetSubMenu(m_pContainer->hMenuContext, 1);
                        GetWindowRect(GetDlgItem(hwndDlg, IDC_PIC), &rc);
                        MsgWindowUpdateMenu(hwndDlg, dat, submenu, MENU_PICMENU);
                        iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, rc.left, rc.bottom, 0, hwndDlg, NULL);
                        isHandled = MsgWindowMenuHandler(hwndDlg, dat, iSelection, MENU_PICMENU);
                        if(isHandled)
                            RedrawWindow(GetDlgItem(hwndDlg, IDC_PIC), NULL, NULL, RDW_INVALIDATE);
                    }
                    break;
                case IDM_CLEAR:
                    if (dat->hwndIEView != 0) {
                        IEVIEWEVENT event;
                        event.cbSize = sizeof(IEVIEWEVENT);
                        event.iType = IEE_CLEAR_LOG;
                        event.hwnd = dat->hwndIEView;
                        event.hContact = dat->hContact;
                        CallService(MS_IEVIEW_EVENT, 0, (LPARAM)&event);
                    }
                    SetDlgItemText(hwndDlg, IDC_LOG, _T(""));
                    dat->hDbEventFirst = NULL;
                    break;
                case IDC_PROTOCOL:
                    CallService(MS_USERINFO_SHOWDIALOG, (WPARAM) (dat->bIsMeta ? dat->hSubContact :  dat->hContact), 0);
                    break;
// error control
                case IDC_CANCELSEND:
                    SendMessage(hwndDlg, DM_ERRORDECIDED, MSGERROR_CANCEL, 0);
                    break;
                case IDC_RETRY:
                    SendMessage(hwndDlg, DM_ERRORDECIDED, MSGERROR_RETRY, 0);
                    break;
                case IDC_MSGSENDLATER:
                    SendMessage(hwndDlg, DM_ERRORDECIDED, MSGERROR_SENDLATER, 0);
                    break;
                case IDC_SELFTYPING:
                    if(dat->hContact) {
                        int iCurrentTypingMode = DBGetContactSettingByte(dat->hContact, SRMSGMOD, SRMSGSET_TYPING, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW));
                        
                        DBWriteContactSettingByte(dat->hContact, SRMSGMOD, SRMSGSET_TYPING, (BYTE)!iCurrentTypingMode);
                        if(m_pContainer->hwndStatus) {
                            if(iCurrentTypingMode)
                                SetSelftypingIcon(hwndDlg, dat, FALSE);
                            else
                                SetSelftypingIcon(hwndDlg, dat, TRUE);
                        }
                    }
                    break;
                case IDC_MESSAGE:
#ifdef __MATHMOD_SUPPORT					
                    //mathMod begin
					if(myGlobals.m_MathModAvail && HIWORD(wParam) == EN_CHANGE)
                        updateMathWindow(hwndDlg, dat);
					//mathMod end
#endif                     
                    /*
					if ((HIWORD(wParam) == EN_VSCROLL || HIWORD(wParam) == EN_HSCROLL) && m_pContainer->bSkinned) {
                        RECT rc;
                        GetUpdateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc, FALSE);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc, TRUE);
                    }
					*/
                    if (HIWORD(wParam) == EN_CHANGE) {
                        if(m_pContainer->hwndActive == hwndDlg)
                            UpdateReadChars(hwndDlg, dat);
                        dat->dwFlags |= MWF_NEEDHISTORYSAVE;
                        dat->dwLastActivity = GetTickCount();
                        m_pContainer->dwLastActivity = dat->dwLastActivity;
                        UpdateSaveAndSendButton(hwndDlg, dat);
                        if (!(GetKeyState(VK_CONTROL) & 0x8000)) {
                            dat->nLastTyping = GetTickCount();
                            if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE))) {
                                if (dat->nTypeMode == PROTOTYPE_SELFTYPING_OFF) {
                                    if(!(dat->dwFlags & MWF_INITMODE))
                                        NotifyTyping(dat, PROTOTYPE_SELFTYPING_ON);
                                }
                            } else {
                                if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON) {
                                    NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
                                }
                            }
                        }
						/*
                        if(m_pContainer->bSkinned) {
                            GetUpdateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc, FALSE);
                            InvalidateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc, TRUE);
                        }
						*/
                    }
            }
            break;
        case WM_NOTIFY:
            if(dat != 0 && ((NMHDR *)lParam)->hwndFrom == dat->hwndTip) {
                if(((NMHDR *)lParam)->code == NM_CLICK)
                    SendMessage(dat->hwndTip, TTM_TRACKACTIVATE, FALSE, 0);
                break;
            }
            switch (((NMHDR *) lParam)->idFrom) {
                case IDC_CLIST:
                    switch (((NMHDR *) lParam)->code) {
                        case CLN_OPTIONSCHANGED:
                            SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETGREYOUTFLAGS, 0, 0);
                            SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETLEFTMARGIN, 2, 0);
                            break;
                    }
                    break;
                case IDC_LOG:
                case IDC_MESSAGE:
                    switch (((NMHDR *) lParam)->code) {
                        case EN_MSGFILTER:
                        {
                            DWORD msg = ((MSGFILTER *) lParam)->msg;
                            WPARAM wp = ((MSGFILTER *) lParam)->wParam;
                            LPARAM lp = ((MSGFILTER *) lParam)->lParam;
                            CHARFORMAT2 cf2;

                            if(wp == VK_BROWSER_BACK || wp == VK_BROWSER_FORWARD)
                                return 1;
                            if(msg == WM_CHAR) {
                                if((GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_SHIFT) & 0x8000)) {
                                    switch (wp) {
                                        case 23:                // ctrl - w
                                            PostMessage(hwndDlg, WM_CLOSE, 1, 0);
                                            break;
										case 25:
											PostMessage(hwndDlg, DM_SPLITTEREMERGENCY, 0, 0);
											break;
                                        case 18:                // ctrl - r
                                            SendMessage(hwndDlg, DM_QUERYPENDING, DM_QUERY_MOSTRECENT, 0);
                                            break;
                                        case 0x0c:              // ctrl - l
                                            SendMessage(hwndDlg, WM_COMMAND, IDM_CLEAR, 0);
                                            break;
                                        case 0x0f:              // ctrl - o
                                            if(m_pContainer->hWndOptions == 0)
                                                CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CONTAINEROPTIONS), hwndContainer, DlgProcContainerOptions, (LPARAM)m_pContainer);
                                            break;
                                        case 0x04:              // ctrl - d  (paste and send)
                                            HandlePasteAndSend(hwndDlg, dat);
                                            break;
                                        case 19:
                                            PostMessage(hwndDlg, WM_COMMAND, IDC_SENDMENU, IDC_SENDMENU);
                                            break;
                                        case 16:
                                            PostMessage(hwndDlg, WM_COMMAND, IDC_PROTOMENU, IDC_PROTOMENU);
                                            break;
                                        case 20:
                                            PostMessage(hwndDlg, WM_COMMAND, IDC_TOGGLETOOLBAR, 1);
                                            break;
										case 14:				// ctrl - n (send a nudge if protocol supports the /SendNudge service)
											SendNudge(dat, hwndDlg);
											break;
                                    }
                                    return 1;
                                }
                            }
                            if(msg == WM_KEYDOWN) {
                                if(wp == VK_INSERT && (GetKeyState(VK_SHIFT) & 0x8000)) {
                                    SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_PASTESPECIAL, CF_TEXT, 0);
                                    ((MSGFILTER *) lParam)->msg = WM_NULL;
                                    ((MSGFILTER *) lParam)->wParam = 0;
                                    ((MSGFILTER *) lParam)->lParam = 0;
                                    return 0;
                                }
                                if ((GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000)) {
                                    if (wp == 0x9) {            // ctrl-shift tab
                                        SendMessage(hwndDlg, DM_SELECTTAB, DM_SELECT_PREV, 0);
                                        ((MSGFILTER *) lParam)->msg = WM_NULL;
                                        ((MSGFILTER *) lParam)->wParam = 0;
                                        ((MSGFILTER *) lParam)->lParam = 0;
                                        return 0;
                                    }
                                }
                                if((GetKeyState(VK_CONTROL)) & 0x8000 && !(GetKeyState(VK_SHIFT) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000)) {
                                    if (wp == 'V') {
                                        SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_PASTESPECIAL, CF_TEXT, 0);
                                        ((MSGFILTER *) lParam)->msg = WM_NULL;
                                        ((MSGFILTER *) lParam)->wParam = 0;
                                        ((MSGFILTER *) lParam)->lParam = 0;
                                        return 0;
                                    }
                                    if (wp == VK_TAB) {
                                        SendMessage(hwndDlg, DM_SELECTTAB, DM_SELECT_NEXT, 0);
                                        ((MSGFILTER *) lParam)->msg = WM_NULL;
                                        ((MSGFILTER *) lParam)->wParam = 0;
                                        ((MSGFILTER *) lParam)->lParam = 0;
                                        return 0;
                                    }
                                    if (wp == VK_F4) {
                                        PostMessage(hwndDlg, WM_CLOSE, 1, 0);
                                        return 1;
                                    }
                                    if (wp == VK_PRIOR) {
                                        SendMessage(hwndDlg, DM_SELECTTAB, DM_SELECT_PREV, 0);
                                        return 1;
                                    }
                                    if (wp == VK_NEXT) {
                                        SendMessage(hwndDlg, DM_SELECTTAB, DM_SELECT_NEXT, 0);
                                        return 1;
                                    }
                                }
                            }
                            if(msg == WM_SYSKEYDOWN && (GetKeyState(VK_MENU) & 0x8000)) {
                                if(wp == VK_MULTIPLY) {
                                    SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                                    return 1;
                                }
                                if(wp == VK_DIVIDE) {
                                    SetFocus(GetDlgItem(hwndDlg, IDC_LOG));
                                    return 1;
                                }
                                if(wp == VK_ADD) {
                                    SendMessage(hwndContainer, DM_SELECTTAB, DM_SELECT_NEXT, 0);
                                    return 1;
                                }
                                if(wp == VK_SUBTRACT) {
                                    SendMessage(hwndContainer, DM_SELECTTAB, DM_SELECT_PREV, 0);
                                    return 1;
                                }
								if(wp == 'C') {
									CallService(MS_TABMSG_SETUSERPREFS, (WPARAM)dat->hContact, 0);
									return 1;
								}

                            }
                            if(msg == WM_KEYDOWN && wp == VK_F12) {
                                if(dat->dwFlagsEx & MWF_SHOW_SCROLLINGDISABLED)
                                    SendMessage(hwndDlg, DM_REPLAYQUEUE, 0, 0);
                                dat->dwFlagsEx ^= MWF_SHOW_SCROLLINGDISABLED;
                                ShowWindow(GetDlgItem(hwndDlg, IDC_LOGFROZEN), dat->dwFlagsEx & MWF_SHOW_SCROLLINGDISABLED ? SW_SHOW : SW_HIDE);
                                ShowWindow(GetDlgItem(hwndDlg, IDC_LOGFROZENTEXT), dat->dwFlagsEx & MWF_SHOW_SCROLLINGDISABLED ? SW_SHOW : SW_HIDE);
                                SendMessage(hwndDlg, WM_SIZE, 0, 0);
                                DM_ScrollToBottom(hwndDlg, dat, 1, 1);
                                break;
                            }
                            if(msg == WM_MOUSEWHEEL && (((NMHDR *)lParam)->idFrom == IDC_LOG || ((NMHDR *)lParam)->idFrom == IDC_MESSAGE)) {
                                RECT rc;
                                POINT pt;

                                GetCursorPos(&pt);
                                GetWindowRect(GetDlgItem(hwndDlg, IDC_LOG), &rc);
                                if(PtInRect(&rc, pt)) {
                                    short wDirection = (short)HIWORD(wp);
                                    if(LOWORD(wp) & MK_SHIFT) {
                                        if(wDirection < 0)
                                            SendMessage(GetDlgItem(hwndDlg, IDC_LOG), WM_VSCROLL, MAKEWPARAM(SB_PAGEDOWN, 0), 0);
                                        else if(wDirection > 0)
                                            SendMessage(GetDlgItem(hwndDlg, IDC_LOG), WM_VSCROLL, MAKEWPARAM(SB_PAGEUP, 0), 0);
                                        return 0;
                                    }
                                    return 0;
                                }
                                return 1;
                            }
                                
                            if(msg == WM_CHAR && wp == 'c') {
                                if(GetKeyState(VK_CONTROL) & 0x8000) {
                                    SendDlgItemMessage(hwndDlg, ((NMHDR *)lParam)->code, WM_COPY, 0, 0);
                                    break;
                                }
                            }
                            if(msg == WM_LBUTTONDOWN || msg == WM_KEYUP || msg == WM_LBUTTONUP) {
                                int bBold = IsDlgButtonChecked(hwndDlg, IDC_FONTBOLD);
                                int bItalic = IsDlgButtonChecked(hwndDlg, IDC_FONTITALIC);
                                int bUnder = IsDlgButtonChecked(hwndDlg, IDC_FONTUNDERLINE);
                                
                                cf2.cbSize = sizeof(CHARFORMAT2);
                                cf2.dwMask = CFM_BOLD|CFM_ITALIC|CFM_UNDERLINE;
                                cf2.dwEffects = 0;
                                SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
                                if(cf2.dwEffects & CFE_BOLD) {
                                    if(bBold == BST_UNCHECKED)
                                        CheckDlgButton(hwndDlg, IDC_FONTBOLD, BST_CHECKED);
                                }
                                else {
                                    if(bBold == BST_CHECKED)
                                        CheckDlgButton(hwndDlg, IDC_FONTBOLD, BST_UNCHECKED);
                                }
                                
                                if(cf2.dwEffects & CFE_ITALIC) {
                                    if(bItalic == BST_UNCHECKED)
                                        CheckDlgButton(hwndDlg, IDC_FONTITALIC, BST_CHECKED);
                                }
                                else {
                                    if(bItalic == BST_CHECKED)
                                        CheckDlgButton(hwndDlg, IDC_FONTITALIC, BST_UNCHECKED);
                                }
                                
                                if(cf2.dwEffects & CFE_UNDERLINE) {
                                    if(bUnder == BST_UNCHECKED)
                                        CheckDlgButton(hwndDlg, IDC_FONTUNDERLINE, BST_CHECKED);
                                }
                                else {
                                    if(bUnder == BST_CHECKED)
                                        CheckDlgButton(hwndDlg, IDC_FONTUNDERLINE, BST_UNCHECKED);
                                }
                            }
                            switch (msg) {
                                case WM_LBUTTONDOWN:
                                    {
                                        HCURSOR hCur = GetCursor();
                                        if (hCur == LoadCursor(NULL, IDC_SIZENS) || hCur == LoadCursor(NULL, IDC_SIZEWE)
                                            || hCur == LoadCursor(NULL, IDC_SIZENESW) || hCur == LoadCursor(NULL, IDC_SIZENWSE)) {
                                            SetWindowLong(hwndDlg, DWL_MSGRESULT, TRUE);
                                            return TRUE;
                                        }
                                        break;
                                    }
                                    /*
                                     * auto-select-and-copy handling...
                                     * if enabled, releasing the lmb with an active selection automatically copies the selection
                                     * to the clipboard.
                                     * holding ctrl while releasing the button pastes the selection to the input area, using plain text
                                     * holding ctrl-alt does the same, but pastes formatted text
                                     */
                                case WM_LBUTTONUP:
                                    if(((NMHDR *) lParam)->idFrom == IDC_LOG) {
                                        CHARRANGE cr;
                                        SendMessage(GetDlgItem(hwndDlg, IDC_LOG), EM_EXGETSEL, 0, (LPARAM)&cr);
                                        if(cr.cpMax != cr.cpMin) {
                                            cr.cpMin = cr.cpMax;
                                            if((GetKeyState(VK_CONTROL) & 0x8000) && DBGetContactSettingByte(NULL, SRMSGMOD_T, "autocopy", 0)){
                                                SETTEXTEX stx = {ST_KEEPUNDO | ST_SELECTION,CP_UTF8};
                                                char *streamOut = NULL;
                                                if(GetKeyState(VK_MENU) & 0x8000)
                                                    streamOut = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_LOG), dat, (CP_UTF8 << 16) | (SF_RTFNOOBJS|SFF_PLAINRTF|SFF_SELECTION|SF_USECODEPAGE));
                                                else
                                                    streamOut = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_LOG), dat, (CP_UTF8 << 16) | (SF_TEXT|SFF_SELECTION|SF_USECODEPAGE));
                                                if(streamOut) {
                                                    FilterEventMarkersA(streamOut);
                                                    SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)streamOut);
                                                    free(streamOut);
                                                }
                                                SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                                            }
                                            else if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "autocopy", 0) && !(GetKeyState(VK_SHIFT) & 0x8000)) {
                                                SendMessage(GetDlgItem(hwndDlg, IDC_LOG), WM_COPY, 0, 0);
                                                SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                                                if(m_pContainer->hwndStatus)
                                                    SendMessage(m_pContainer->hwndStatus, SB_SETTEXT, 0, (LPARAM)TranslateT("Selection copied to clipboard"));
                                            }
                                        }
                                    }
                                    break;
                                case WM_MOUSEMOVE:
                                    {
                                        HCURSOR hCur = GetCursor();
                                        if (hCur == LoadCursor(NULL, IDC_SIZENS) || hCur == LoadCursor(NULL, IDC_SIZEWE)
                                            || hCur == LoadCursor(NULL, IDC_SIZENESW) || hCur == LoadCursor(NULL, IDC_SIZENWSE))
                                            SetCursor(LoadCursor(NULL, IDC_ARROW));
                                        
                                        break;
                                    }
                                case WM_RBUTTONUP:
                                    {
                                        HMENU hMenu, hSubMenu;
                                        POINT pt;
                                        CHARRANGE sel, all = { 0, -1};
                                        int iSelection;
                                        unsigned int oldCodepage = dat->codePage;
                                        int idFrom = ((NMHDR *)lParam)->idFrom;
                                        int iPrivateBG = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "private_bg", 0);
                                        
                                        hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_CONTEXT));
                                        if(idFrom == IDC_LOG)
                                            hSubMenu = GetSubMenu(hMenu, 0);
                                        else {
                                            hSubMenu = GetSubMenu(hMenu, 2);
                                            EnableMenuItem(hSubMenu, IDM_PASTEFORMATTED, MF_BYCOMMAND | (dat->SendFormat != 0 ? MF_ENABLED : MF_GRAYED));
                                            EnableMenuItem(hSubMenu, ID_EDITOR_PASTEANDSENDIMMEDIATELY, MF_BYCOMMAND | (myGlobals.m_PasteAndSend ? MF_ENABLED : MF_GRAYED));
                                            CheckMenuItem(hSubMenu, ID_EDITOR_SHOWMESSAGELENGTHINDICATOR, MF_BYCOMMAND | (myGlobals.m_visualMessageSizeIndicator ? MF_CHECKED : MF_UNCHECKED));
                                        }
                                        CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hSubMenu, 0);
                                        SendMessage(((NMHDR *) lParam)->hwndFrom, EM_EXGETSEL, 0, (LPARAM) & sel);
                                        if (sel.cpMin == sel.cpMax) {
                                            EnableMenuItem(hSubMenu, IDM_COPY, MF_BYCOMMAND | MF_GRAYED);
                                            if(idFrom == IDC_MESSAGE)
                                                EnableMenuItem(hSubMenu, IDM_CUT, MF_BYCOMMAND | MF_GRAYED);
                                        }
                                        pt.x = (short) LOWORD(((ENLINK *) lParam)->lParam);
                                        pt.y = (short) HIWORD(((ENLINK *) lParam)->lParam);
                                        ClientToScreen(((NMHDR *) lParam)->hwndFrom, &pt);
#if defined(_UNICODE)
                                        if(idFrom == IDC_LOG)  {
                                            int i;
                                            InsertMenuA(hSubMenu, 5, MF_BYPOSITION | MF_SEPARATOR, 0, 0);
                                            InsertMenu(hSubMenu, 6, MF_BYPOSITION | MF_POPUP, (UINT_PTR) myGlobals.g_hMenuEncoding, TranslateT("ANSI Encoding"));
                                            for(i = 0; i < GetMenuItemCount(myGlobals.g_hMenuEncoding); i++)
                                                CheckMenuItem(myGlobals.g_hMenuEncoding, i, MF_BYPOSITION | MF_UNCHECKED);
                                            if(dat->codePage == CP_ACP)
                                                CheckMenuItem(myGlobals.g_hMenuEncoding, 0, MF_BYPOSITION | MF_CHECKED);
                                            else
                                                CheckMenuItem(myGlobals.g_hMenuEncoding, dat->codePage, MF_BYCOMMAND | MF_CHECKED);
                                            CheckMenuItem(hSubMenu, ID_LOG_FREEZELOG, MF_BYCOMMAND | (dat->dwFlagsEx & MWF_SHOW_SCROLLINGDISABLED ? MF_CHECKED : MF_UNCHECKED));
                                                
                                        }
#endif                                        
                                        iSelection = TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
                                        if(iSelection > 800 && iSelection < 1400 && ((NMHDR *)lParam)->idFrom == IDC_LOG) {
                                            dat->codePage = iSelection;
                                            DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "ANSIcodepage", dat->codePage);
                                        }
                                        else if(iSelection == 500 && ((NMHDR *)lParam)->idFrom == IDC_LOG) {
                                            dat->codePage = CP_ACP;
                                            DBDeleteContactSetting(dat->hContact, SRMSGMOD_T, "ANSIcodepage");
                                        }
                                        else {
                                            switch (iSelection) {
                                                case IDM_COPY:
                                                    SendMessage(((NMHDR *) lParam)->hwndFrom, WM_COPY, 0, 0);
                                                    break;
                                                case IDM_CUT:
                                                    SendMessage(((NMHDR *) lParam)->hwndFrom, WM_CUT, 0, 0);
                                                    break;
                                                case IDM_PASTE:
                                                case IDM_PASTEFORMATTED:
                                                    if(idFrom == IDC_MESSAGE)
                                                        SendMessage(((NMHDR *) lParam)->hwndFrom, EM_PASTESPECIAL, (iSelection == IDM_PASTE) ? CF_TEXT : 0, 0);
                                                    break;
                                                case IDM_COPYALL:
                                                    SendMessage(((NMHDR *) lParam)->hwndFrom, EM_EXSETSEL, 0, (LPARAM) & all);
                                                    SendMessage(((NMHDR *) lParam)->hwndFrom, WM_COPY, 0, 0);
                                                    SendMessage(((NMHDR *) lParam)->hwndFrom, EM_EXSETSEL, 0, (LPARAM) & sel);
                                                    break;
                                                case IDM_SELECTALL:
                                                    SendMessage(((NMHDR *) lParam)->hwndFrom, EM_EXSETSEL, 0, (LPARAM) & all);
                                                    break;
                                                case IDM_CLEAR:
                                                    SetDlgItemText(hwndDlg, IDC_LOG, _T(""));
                                                    dat->hDbEventFirst = NULL;
                                                    break;
                                                case ID_LOG_FREEZELOG:
                                                    SendMessage(GetDlgItem(hwndDlg, IDC_LOG), WM_KEYDOWN, VK_F12, 0);
                                                    break;
                                                case ID_EDITOR_SHOWMESSAGELENGTHINDICATOR:
                                                    myGlobals.m_visualMessageSizeIndicator = !myGlobals.m_visualMessageSizeIndicator;
                                                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "msgsizebar", (BYTE)myGlobals.m_visualMessageSizeIndicator);
                                                    WindowList_Broadcast(hMessageWindowList, DM_CONFIGURETOOLBAR, 0, 0);
                                                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                                                    //SetWindowPos(GetDlgItem(hwndDlg, IDC_SPLITTER), 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
                                                    //RedrawWindow(GetDlgItem(hwndDlg, IDC_SPLITTER), NULL, NULL, RDW_ERASE | RDW_UPDATENOW | RDW_INVALIDATE | RDW_FRAME);
                                                    break;
                                                case ID_EDITOR_PASTEANDSENDIMMEDIATELY:
                                                    HandlePasteAndSend(hwndDlg, dat);
                                                    break;
                                            }
                                        }
#if defined(_UNICODE)
                                        if(idFrom == IDC_LOG)
                                            RemoveMenu(hSubMenu, 6, MF_BYPOSITION);
#endif                                        
                                        DestroyMenu(hMenu);
                                        if(dat->codePage != oldCodepage) {
                                            SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                                            dat->iOldHash = 0;
                                            SendMessage(hwndDlg, DM_UPDATETITLE, 0, 0);
                                        }
                                        SetWindowLong(hwndDlg, DWL_MSGRESULT, TRUE);
                                        return TRUE;
                                    }
                            }
                            break;
                        }
                        case EN_LINK:
                            switch (((ENLINK *) lParam)->msg) {
                                case WM_SETCURSOR:
                                    SetCursor(myGlobals.hCurHyperlinkHand);
                                    SetWindowLong(hwndDlg, DWL_MSGRESULT, TRUE);
                                    return TRUE;
                                case WM_RBUTTONDOWN:
                                case WM_LBUTTONUP:
                                    {
                                        TEXTRANGEA tr;
                                        CHARRANGE sel;

                                        SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXGETSEL, 0, (LPARAM) & sel);
                                        if (sel.cpMin != sel.cpMax)
                                            break;
                                        tr.chrg = ((ENLINK *) lParam)->chrg;
                                        tr.lpstrText = malloc(tr.chrg.cpMax - tr.chrg.cpMin + 8);
                                        SendDlgItemMessageA(hwndDlg, IDC_LOG, EM_GETTEXTRANGE, 0, (LPARAM) & tr);
                                        if (strchr(tr.lpstrText, '@') != NULL && strchr(tr.lpstrText, ':') == NULL && strchr(tr.lpstrText, '/') == NULL) {
                                            MoveMemory(tr.lpstrText + 7, tr.lpstrText, tr.chrg.cpMax - tr.chrg.cpMin + 1);
                                            CopyMemory(tr.lpstrText, "mailto:", 7);
                                        }
                                        if (((ENLINK *) lParam)->msg == WM_RBUTTONDOWN) {
                                            HMENU hMenu, hSubMenu;
                                            POINT pt;

                                            hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_CONTEXT));
                                            hSubMenu = GetSubMenu(hMenu, 1);
                                            CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hSubMenu, 0);
                                            pt.x = (short) LOWORD(((ENLINK *) lParam)->lParam);
                                            pt.y = (short) HIWORD(((ENLINK *) lParam)->lParam);
                                            ClientToScreen(((NMHDR *) lParam)->hwndFrom, &pt);
                                            switch (TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL)) {
                                                case IDM_OPENNEW:
                                                    CallService(MS_UTILS_OPENURL, 1, (LPARAM) tr.lpstrText);
                                                    break;
                                                case IDM_OPENEXISTING:
                                                    CallService(MS_UTILS_OPENURL, 0, (LPARAM) tr.lpstrText);
                                                    break;
                                                case IDM_COPYLINK:
                                                    {
                                                        HGLOBAL hData;
                                                        if (!OpenClipboard(hwndDlg))
                                                            break;
                                                        EmptyClipboard();
                                                        hData = GlobalAlloc(GMEM_MOVEABLE, lstrlenA(tr.lpstrText) + 1);
                                                        lstrcpyA(GlobalLock(hData), tr.lpstrText);
                                                        GlobalUnlock(hData);
                                                        SetClipboardData(CF_TEXT, hData);
                                                        CloseClipboard();
                                                        break;
                                                    }
                                            }
                                            free(tr.lpstrText);
                                            DestroyMenu(hMenu);
                                            SetWindowLong(hwndDlg, DWL_MSGRESULT, TRUE);
                                            return TRUE;
                                        } else {
                                            CallService(MS_UTILS_OPENURL, 1, (LPARAM) tr.lpstrText);
                                            SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                                        }

                                        free(tr.lpstrText);
                                        break;
                                    }
                            }
                            break;
                    }
                    break;
            }
            break;
        case WM_CONTEXTMENU:
            if((HWND)wParam == GetDlgItem(hwndDlg, IDC_PROTOCOL) && dat->hContact != 0) {
                POINT pt;
                HMENU hMC;

                GetCursorPos(&pt);
                hMC = BuildMCProtocolMenu(hwndDlg);
                if(hMC) {
                    int iSelection = 0;
                    iSelection = TrackPopupMenu(hMC, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
                    if(iSelection < 1000 && iSelection >= 100) {         // the "force" submenu...
                        if(iSelection == 999) {                           // un-force
                            if(CallService(MS_MC_UNFORCESENDCONTACT, (WPARAM)dat->hContact, 0) == 0)
                                DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "tabSRMM_forced", -1);
                            else
                                _DebugMessage(hwndDlg, dat, Translate("Unforce failed"));
                        }
                        else {
                            if(CallService(MS_MC_FORCESENDCONTACTNUM, (WPARAM)dat->hContact,  (LPARAM)(iSelection - 100)) == 0)
                                DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "tabSRMM_forced", (DWORD)(iSelection - 100));
                            else
                                _DebugMessage(hwndDlg, dat, Translate("The selected protocol cannot be forced at this time"));
                        }
                    }
                    else if(iSelection >= 1000) {                        // the "default" menu...
                        CallService(MS_MC_SETDEFAULTCONTACTNUM, (WPARAM)dat->hContact, (LPARAM)(iSelection - 1000));
                    }
                    DestroyMenu(hMC);
					return TRUE;
                }
                break;
            }
            break;
            /*
             * this is now *only* called from the global ME_PROTO_ACK handler (static int ProtoAck() in msgs.c)
             * it receives:
             * wParam = index of the sendjob in the queue in the low word, index of the found sendID in the high word
                        (normally 0, but if its a multisend job, then the sendjob may contain more than one hContact/hSendId
                        pairs.)
             * lParam = the original ackdata
             *
             * the "per message window" ACK hook is gone, the global ack handler cares about all types of ack's (currently
             * *_MESSAGE and *_AVATAR and dispatches them to the owner windows).
             */
        case HM_EVENTSENT:
            {
                ACKDATA *ack = (ACKDATA *) lParam;
                DBEVENTINFO dbei = { 0};
                HANDLE hNewEvent;
                int i, iFound = NR_SENDJOBS, iNextFailed;

                iFound = (int)(LOWORD(wParam));
                i = (int)(HIWORD(wParam));
                
                if(iFound < 0 || iFound >= NR_SENDJOBS || i < 0 || i >= SENDJOBS_MAX_SENDS) {       // sanity checks (unlikely to happen).
                    _DebugPopup(dat->hContact, "Warning: HM_EVENTSENT with invalid data (sq-index = %d, sendId-index = %d", iFound, i);
                    break;
                }
                if (ack->type != ACKTYPE_MESSAGE) {
                    _DebugPopup(dat->hContact, "Warning: HM_EVENTSENT received unknown/invalid ACKTYPE (%d)", ack->type);
                    break;                                       // should not happen, but who knows...
                }

                if(sendJobs[iFound].iStatus == SQ_ERROR) {       // received ack for a job which is already in error state...
                    if(dat->iCurrentQueueError == iFound) {
                        dat->iCurrentQueueError = -1;
                        ShowErrorControls(hwndDlg, dat, FALSE);
                    }
                }
               
                if(ack->result == ACKRESULT_FAILED) {
                    /*
                     * "hard" errors are handled differently in multisend. There is no option to retry - once failed, they
                     * are discarded and the user is notified with a small log message.
                     */
                    if(!nen_options.iNoSounds && !(m_pContainer->dwFlags & CNT_NOSOUND))
                        SkinPlaySound("SendError");
                    if(sendJobs[iFound].sendCount > 1) {         // multisend is different...
                        char szErrMsg[256];
                        mir_snprintf(szErrMsg, sizeof(szErrMsg), "Multisend: failed sending to: %s", dat->szProto);
                        LogErrorMessage(hwndDlg, dat, -1, szErrMsg);
                        goto verify;
                    }
                    else {
                        mir_snprintf(sendJobs[iFound].szErrorMsg, sizeof(sendJobs[iFound].szErrorMsg), "Delivery failure: %s", (char *)ack->lParam);
                        sendJobs[iFound].iStatus = SQ_ERROR;
                        KillTimer(hwndDlg, TIMERID_MSGSEND + iFound);
                        if(!(dat->dwFlags & MWF_ERRORSTATE))
                            HandleQueueError(hwndDlg, dat, iFound);
                    }
                    return 0;
                }

                dbei.cbSize = sizeof(dbei);
                dbei.eventType = EVENTTYPE_MESSAGE;
                dbei.flags = DBEF_SENT;
                dbei.szModule = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) sendJobs[iFound].hContact[i], 0);
                dbei.timestamp = time(NULL);
                dbei.cbBlob = lstrlenA(sendJobs[iFound].sendBuffer) + 1;
                dat->stats.iSentBytes += (dbei.cbBlob - 1);
                dat->stats.iSent++;

#if defined( _UNICODE )
                if(sendJobs[iFound].dwFlags & PREF_UNICODE)
                    dbei.cbBlob *= sizeof(TCHAR) + 1;
                if(sendJobs[iFound].dwFlags & PREF_RTL)
                    dbei.flags |= DBEF_RTL;
#endif
                dbei.pBlob = (PBYTE) sendJobs[iFound].sendBuffer;
                hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) sendJobs[iFound].hContact[i], (LPARAM) & dbei);
                if(!nen_options.iNoSounds && !(m_pContainer->dwFlags & CNT_NOSOUND))
                    SkinPlaySound("SendMsg");

                /*
                 * if this is a multisend job, AND the ack was from a different contact (not the session "owner" hContact)
                 * then we print a small message telling the user that the message has been delivered to *foo*
                 */

                if(sendJobs[iFound].hContact[i] != sendJobs[iFound].hOwner) {
                    char szErrMsg[256];
                    char *szReceiver = (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)sendJobs[iFound].hContact[i], 0);
                    mir_snprintf(szErrMsg, sizeof(szErrMsg), "Multisend: successfully sent to: %s", szReceiver);
                    LogErrorMessage(hwndDlg, dat, -1, szErrMsg);
                }

                if (sendJobs[iFound].hContact[i] == dat->hContact) {
                    if (dat->hDbEventFirst == NULL) {
                        dat->hDbEventFirst = hNewEvent;
                        SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                    }
                }
verify:                
                sendJobs[iFound].hSendId[i] = NULL;
                sendJobs[iFound].hContact[i] = NULL;
                sendJobs[iFound].iAcksNeeded--;
                
                if(sendJobs[iFound].iAcksNeeded == 0) {               // everything sent
                    if(sendJobs[iFound].sendCount > 1)
                        EnableSending(hwndDlg, dat, TRUE);
                    ClearSendJob(iFound);
                    KillTimer(hwndDlg, TIMERID_MSGSEND + iFound);
                    dat->iOpenJobs--;
                    myGlobals.iSendJobCurrent--;
                }
                CheckSendQueue(hwndDlg, dat);
                if((iNextFailed = FindNextFailedMsg(hwndDlg, dat)) >= 0 && !(dat->dwFlags & MWF_ERRORSTATE))
                    HandleQueueError(hwndDlg, dat, iNextFailed);
                return 0;
            }
            /*
             * save the current content of the input box to the history stack...
             * increase stack
             * wParam = special mode to store it at a special position without caring about the
             * stack pointer and stuff..
             *
             * if lParam == 0 it will obtain the length from the input box, otherwise
             * lParam will be the length of the required ANSI buffer in bytes and the message will
             * be taken from dat->sendBuffer
             * lParam must then provide the length of the string *INCLUDING* the terminating \0
             * 
             * updated to use RTF streaming and save rich text in utf8 format
             */
        case DM_SAVEPERCONTACT:
            if (dat->hContact) {
                if(DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "mwoverride", 0) != 0)
                    DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "mwflags", dat->dwFlags & MWF_LOG_ALL);
            }
            DBWriteContactSettingDword(NULL, SRMSGMOD, "multisplit", dat->multiSplitterX);
            break;
        case DM_ACTIVATEME:
            ActivateExistingTab(m_pContainer, hwndDlg);
            return 0;
            /*
             * sent by the select container dialog box when a container was selected...
             * lParam = (TCHAR *)selected name...
             */
        case DM_CONTAINERSELECTED:
            {
                struct ContainerWindowData *pNewContainer = 0;
                TCHAR *szNewName = (TCHAR *)lParam;
                int iOldItems = TabCtrl_GetItemCount(hwndTab);
                if (!_tcsncmp(m_pContainer->szName, szNewName, CONTAINER_NAMELEN))
                    break;
                pNewContainer = FindContainerByName(szNewName);
                if (pNewContainer == NULL)
                    pNewContainer = CreateContainer(szNewName, FALSE, dat->hContact);
#if defined (_UNICODE)
                DBWriteContactSettingTString(dat->hContact, SRMSGMOD_T, "containerW", szNewName);
#else
                DBWriteContactSettingTString(dat->hContact, SRMSGMOD_T, "container", szNewName);
#endif                
                dat->fIsReattach = TRUE;
				PostMessage(myGlobals.g_hwndHotkeyHandler, DM_DOCREATETAB, (WPARAM)pNewContainer, (LPARAM)dat->hContact);
                if (iOldItems > 1)                // there were more than 1 tab, container is still valid
                    SendMessage(m_pContainer->hwndActive, WM_SIZE, 0, 0);
                SetForegroundWindow(pNewContainer->hwnd);
                SetActiveWindow(pNewContainer->hwnd);
                break;
            }
        case DM_STATUSBARCHANGED:
            UpdateStatusBar(hwndDlg, dat);
            return 0;
        case DM_MULTISENDTHREADCOMPLETE:
            if(dat->hMultiSendThread) {
                CloseHandle(dat->hMultiSendThread);
                dat->hMultiSendThread = 0;
            }
            return 0;
        case DM_UINTOCLIPBOARD:
        {
            HGLOBAL hData;

            if(dat->hContact) {
                if (!OpenClipboard(hwndDlg))
                    break;
                if(lstrlenA(dat->uin) == 0)
                    break;
                EmptyClipboard();
                hData = GlobalAlloc(GMEM_MOVEABLE, lstrlenA(dat->uin) + 1);
                lstrcpyA(GlobalLock(hData), dat->uin);
                GlobalUnlock(hData);
                SetClipboardData(CF_TEXT, hData);
                CloseClipboard();
                GlobalFree(hData);
            }
            return 0;
        }
        /*
         * broadcasted when GLOBAL info panel setting changes
         */
        case DM_SETINFOPANEL:
            dat->dwFlagsEx = GetInfoPanelSetting(hwndDlg, dat) ? dat->dwFlagsEx | MWF_SHOW_INFOPANEL : dat->dwFlagsEx & ~MWF_SHOW_INFOPANEL;
            if(!(dat->dwFlags & MWF_ERRORSTATE))
                ShowHideInfoPanel(hwndDlg, dat);
            return 0;

        /*
         * show the balloon tooltip control.
         * wParam == id of the "anchor" element, defaults to the panel status field (for away msg retrieval)
         * lParam == new text to show
         */
            
        case DM_ACTIVATETOOLTIP:
        {
            if(IsIconic(hwndContainer) || m_pContainer->bInTray || m_pContainer->hwndActive != hwndDlg)
                break;

            if(dat->hwndTip) {
                RECT rc;
                TCHAR szTitle[256];
                UINT id = wParam;

                if(id == 0)
                    id = IDC_PANELSTATUS;
                
                if(id == IDC_PANELSTATUS + 1)
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_PANELSTATUS), &rc);
                else
                    GetWindowRect(GetDlgItem(hwndDlg, id), &rc);
                SendMessage(dat->hwndTip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(rc.left, rc.bottom));
                if(lParam)
                    dat->ti.lpszText = (TCHAR *)lParam;
                else {
                    if(lstrlen(dat->statusMsg) > 0)
                        dat->ti.lpszText = dat->statusMsg;
                    else
                        dat->ti.lpszText = myGlobals.m_szNoStatus;
                }
                    
                SendMessage(dat->hwndTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&dat->ti);
                SendMessage(dat->hwndTip, TTM_SETMAXTIPWIDTH, 0, 350);
#if defined(_UNICODE)
                switch(id) {
                    case IDC_PANELNICK:
                    {
                        DBVARIANT dbv;

                        if(!DBGetContactSettingTString(dat->bIsMeta ? dat->hSubContact : dat->hContact, dat->bIsMeta ? dat->szMetaProto : dat->szProto, "XStatusName", &dbv)) {
                            if(lstrlen(dbv.ptszVal) > 1) {
                                _sntprintf(szTitle, safe_sizeof(szTitle), TranslateT("Extended status for %s: %s"), dat->szNickname, dbv.ptszVal);
                                szTitle[safe_sizeof(szTitle) - 1] = 0;
                                DBFreeVariant(&dbv);
                                break;
                            }
                            DBFreeVariant(&dbv);
                        }
                        if(dat->xStatus > 0 && dat->xStatus <= 32) {
                            WCHAR xStatusDescW[100];
                            MultiByteToWideChar(myGlobals.m_LangPackCP, 0, xStatusDescr[dat->xStatus - 1], -1, xStatusDescW, 90);
                            _sntprintf(szTitle, safe_sizeof(szTitle), TranslateT("Extended status for %s: %s"), dat->szNickname, xStatusDescW);
                            szTitle[safe_sizeof(szTitle) - 1] = 0;
                        }
                        else
                            return 0;
                        break;
                    }
                    case IDC_PANELSTATUS + 1:
                    {
                        _sntprintf(szTitle, safe_sizeof(szTitle), TranslateT("%s is using"), dat->szNickname);
                        szTitle[safe_sizeof(szTitle) - 1] = 0;
                        break;
                    }
                    case IDC_PANELSTATUS: 
                    {
                        WCHAR szwStatus[100];
                        MultiByteToWideChar(myGlobals.m_LangPackCP, 0, dat->szStatus, -1, szwStatus, 90);
                        _sntprintf(szTitle, safe_sizeof(szTitle), TranslateT("Status message for %s (%s)"), dat->szNickname, szwStatus);
                        szTitle[safe_sizeof(szTitle) - 1] = 0;
                        break;
                    }
                    default:
                        _sntprintf(szTitle, safe_sizeof(szTitle), TranslateT("tabSRMM Information"));
                }
                SendMessage(dat->hwndTip, TTM_SETTITLE, 1, (LPARAM)szTitle);
#else
                switch(id) {
                    case IDC_PANELNICK:
                    {
                        DBVARIANT dbv;

                        if(!DBGetContactSetting(dat->bIsMeta ? dat->hSubContact : dat->hContact, dat->bIsMeta ? dat->szMetaProto : dat->szProto, "XStatusName", &dbv)) {
                            if(dbv.type == DBVT_ASCIIZ)
                                mir_snprintf(szTitle, sizeof(szTitle), Translate("Extended status for %s: %s"), dat->szNickname, dbv.pszVal);
                            DBFreeVariant(&dbv);
                        }
                        else if(dat->xStatus > 0 && dat->xStatus <= 32)
                            mir_snprintf(szTitle, sizeof(szTitle), Translate("Extended status for %s: %s"), dat->szNickname, xStatusDescr[dat->xStatus - 1]);
                        else
                            return 0;
                        break;
                    }
                    case IDC_PANELSTATUS + 1:
                    {
                        mir_snprintf(szTitle, sizeof(szTitle), Translate("%s is using"), dat->szNickname);
                        break;
                    }
                    case IDC_PANELSTATUS:
                        mir_snprintf(szTitle, sizeof(szTitle), Translate("Status message for %s (%s)"), dat->szNickname, dat->szStatus);
                        break;
                    default:
                        mir_snprintf(szTitle, sizeof(szTitle), Translate("tabSRMM Information"));
                }
                SendMessage(dat->hwndTip, TTM_SETTITLEA, 1, (LPARAM)szTitle);
#endif
                SendMessage(dat->hwndTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&dat->ti);
            }
            break;
        }
        case WM_NEXTDLGCTL:
            if(dat->dwFlags & MWF_WASBACKGROUNDCREATE)
                return 1;

            if(!LOWORD(lParam)) {
                if(wParam == 0) {       // next ctrl to get the focus
                    if(GetNextDlgTabItem(hwndDlg, GetFocus(), FALSE) == GetDlgItem(hwndDlg, IDC_MESSAGE)) {
                        SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                        return 1;
                    }
                    if(GetNextDlgTabItem(hwndDlg, GetFocus(), FALSE) == GetDlgItem(hwndDlg, IDC_LOG)) {
                        SetFocus(GetDlgItem(hwndDlg, IDC_LOG));
                        return 1;
                    }
                }
                else {
                    if(GetNextDlgTabItem(hwndDlg, GetFocus(), TRUE) == GetDlgItem(hwndDlg, IDC_MESSAGE)) {
                        SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                        return 1;
                    }
                    if(GetNextDlgTabItem(hwndDlg, GetFocus(), TRUE) == GetDlgItem(hwndDlg, IDC_LOG)) {
                        SetFocus(GetDlgItem(hwndDlg, IDC_LOG));
                        return 1;
                    }
                }
            }
            else {
                if((HWND)wParam == GetDlgItem(hwndDlg, IDC_MESSAGE)) {
                    SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                    return 1;
                }
            }
            return 0;
            break;
        /*
         * save the contents of the log as rtf file
         */
        case DM_SAVEMESSAGELOG:
        {
            char szFilename[MAX_PATH];
            OPENFILENAMEA ofn={0};
            EDITSTREAM stream = { 0 };
            char szFilter[MAX_PATH];
            
            if(dat->hwndIEView != 0) {
                IEVIEWEVENT event = {0};

                event.cbSize = sizeof(IEVIEWEVENT);
                event.hwnd = dat->hwndIEView;
                event.hContact = dat->hContact;
                event.iType = IEE_SAVE_DOCUMENT;
                event.dwFlags = 0;
                event.count = 0;
                event.codepage = 0;
                CallService(MS_IEVIEW_EVENT, 0, (LPARAM)&event);
            }
            else {
                static char *forbiddenCharacters = "%/\\'";
                char   szInitialDir[MAX_PATH];
                int    i;

                strncpy(szFilter, "Rich Edit file\0*.rtf", MAX_PATH);
                mir_snprintf(szFilename, MAX_PATH, "%s.rtf", dat->uin);

                for(i = 0; i < lstrlenA(forbiddenCharacters); i++) {
                    char *szFound = 0;

                    while((szFound = strchr(szFilename, (int)forbiddenCharacters[i])) != NULL)
                        *szFound = '_';
                }
                mir_snprintf(szInitialDir, MAX_PATH, "%s%s\\", myGlobals.szDataPath, "saved message logs");
                if(!PathFileExistsA(szInitialDir))
                    CreateDirectoryA(szInitialDir, NULL);
                ofn.lStructSize=sizeof(ofn);
                ofn.hwndOwner=hwndDlg;
                ofn.lpstrFile = szFilename;
                ofn.lpstrFilter = szFilter;
                ofn.lpstrInitialDir = szInitialDir;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_HIDEREADONLY;
                ofn.lpstrDefExt = "rtf";
                if (GetSaveFileNameA(&ofn)) {
                    stream.dwCookie = (DWORD_PTR)szFilename;
                    stream.dwError = 0;
                    stream.pfnCallback = StreamOut;
                    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_STREAMOUT, SF_RTF | SF_USECODEPAGE, (LPARAM) & stream);
                }
            }
            break;
        }
        /*
         * sent from the containers heartbeat timer
         * wParam = inactivity timer in seconds
         */
        case DM_CHECKAUTOCLOSE:
        {
            if(GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MESSAGE)) > 0)
                break;              // don't autoclose if message input area contains text
            if(dat->dwTickLastEvent >= dat->dwLastActivity)
                break;              // don't autoclose if possibly unread message is waiting
            if(((GetTickCount() - dat->dwLastActivity) / 1000) >= wParam) {
                if(TabCtrl_GetItemCount(GetParent(hwndDlg)) > 1 || DBGetContactSettingByte(NULL, SRMSGMOD_T, "autocloselast", 0))
                    SendMessage(hwndDlg, WM_CLOSE, 0, 1);
            }
            break;
        }

        // metacontact support
        
        case DM_UPDATEMETACONTACTINFO:      // update the icon in the statusbar for the "most online" protocol
        {
            DWORD isForced;
            char *szProto;


            if((isForced = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "tabSRMM_forced", -1)) >= 0) {
                char szTemp[64];
                mir_snprintf(szTemp, sizeof(szTemp), "Status%d", isForced);
                if(DBGetContactSettingWord(dat->hContact, "MetaContacts", szTemp, 0) == ID_STATUS_OFFLINE) {
                    TCHAR szBuffer[200];
                    _sntprintf(szBuffer, 200, TranslateT("Warning: you have selected a subprotocol for sending the following messages which is currently offline"));
					szBuffer[199] = 0;
                    SendMessage(hwndDlg, DM_ACTIVATETOOLTIP, IDC_MESSAGE, (LPARAM)szBuffer);
                }
            }
            szProto = GetCurrentMetaContactProto(hwndDlg, dat);

            if(m_pContainer->hwndActive == hwndDlg && m_pContainer->hwndStatus != 0)
                UpdateStatusBarTooltips(hwndDlg, dat, -1);
            SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
            break;
        }
        case DM_SECURE_CHANGED:
            UpdateStatusBar(hwndDlg, dat);
            break;
		case DM_IEVIEWOPTIONSCHANGED:
			if(dat->hwndIEView)
				SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
			break;
        case DM_SMILEYOPTIONSCHANGED:
            ConfigureSmileyButton(hwndDlg, dat);
            SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
            break;
		case DM_PROTOAVATARCHANGED:
            dat->ace = (AVATARCACHEENTRY *)CallService(MS_AV_GETAVATARBITMAP, (WPARAM)dat->hContact, 0);
			dat->panelWidth = -1;				// force new size calculations
            ShowPicture(hwndDlg, dat, TRUE);
			if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
				InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELPIC), NULL, TRUE);
				SendMessage(hwndDlg, WM_SIZE, 0, 0);
			}
			return 0;
		case DM_MYAVATARCHANGED:
			{
				char *szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;

				if(!strcmp((char *)wParam, szProto) && lstrlenA(szProto) == lstrlenA((char *)wParam))
					LoadOwnAvatar(hwndDlg, dat);
				break;
			}
        case WM_INPUTLANGCHANGE:
            return DefWindowProc(hwndDlg, WM_INPUTLANGCHANGE, wParam, lParam);

        case DM_GETWINDOWSTATE:
        {
            UINT state = 0;
         
            state |= MSG_WINDOW_STATE_EXISTS;
            if (IsWindowVisible(hwndDlg)) 
               state |= MSG_WINDOW_STATE_VISIBLE;
            if (GetForegroundWindow() == hwndContainer) 
               state |= MSG_WINDOW_STATE_FOCUS;
            if (IsIconic(hwndContainer))
               state |= MSG_WINDOW_STATE_ICONIC;
            SetWindowLong(hwndDlg, DWL_MSGRESULT, state);
            return TRUE;
        }
        case DM_CLIENTCHANGED:
        {
            GetClientIcon(dat, hwndDlg);
            if(dat->hClientIcon) {
                //SendMessage(hwndDlg, WM_SIZE, 0, 0);
                InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELSTATUS), NULL, TRUE);
            }
            return 0;
        }
        case DM_REMOVEPOPUPS:
            DeletePopupsForContact(dat->hContact, (DWORD)wParam);
            return 0;
        case EM_THEMECHANGED:
            if(dat->hTheme && pfnCloseThemeData) {
                pfnCloseThemeData(dat->hTheme);
                dat->hTheme = 0;
            }
            return DM_ThemeChanged(hwndDlg, dat);
        case DM_PLAYINCOMINGSOUND:
            if(!dat)
                return 0;
            PlayIncomingSound(m_pContainer, hwndDlg);
            return 0;
        case DM_SPLITTEREMERGENCY:
            dat->splitterY = 150;
            SendMessage(hwndDlg, WM_SIZE, 0, 0);
            break;
        case DM_REFRESHTABINDEX:
            dat->iTabID = GetTabIndexFromHWND(GetParent(hwndDlg), hwndDlg);
            if(dat->iTabID == -1)
                _DebugPopup(dat->hContact, "WARNING: new tabindex: %d", dat->iTabID);
            return 0;
        case WM_DROPFILES:
        {   
            BOOL not_sending=GetKeyState(VK_CONTROL)&0x8000;
         if (!not_sending) {
            if(dat->szProto==NULL) break;
            if(!(CallProtoService(dat->szProto,PS_GETCAPS,PFLAGNUM_1,0)&PF1_FILESEND)) break;
            if(dat->wStatus == ID_STATUS_OFFLINE) break;
         }
         if(dat->hContact!=NULL) {
            HDROP hDrop;
            char **ppFiles=NULL;
            char szFilename[MAX_PATH];
            int fileCount,totalCount=0,i;
        
            hDrop=(HDROP)wParam;
            fileCount=DragQueryFile(hDrop,-1,NULL,0);
            ppFiles=NULL;
            for(i=0;i<fileCount;i++) {
               DragQueryFileA(hDrop,i,szFilename,sizeof(szFilename));
               AddToFileList(&ppFiles,&totalCount,szFilename);
            }
        
            if (!not_sending) {
               CallService(MS_FILE_SENDSPECIFICFILES,(WPARAM)dat->hContact,(LPARAM)ppFiles);
            }
            else {
               #define MS_HTTPSERVER_ADDFILENAME "HTTPServer/AddFileName"
        
               if(ServiceExists(MS_HTTPSERVER_ADDFILENAME)) {
                  char *szHTTPText;
                  int i;
        
                  for(i=0;i<totalCount;i++) {
                     char *szTemp;
                     szTemp=(char*)CallService(MS_HTTPSERVER_ADDFILENAME,(WPARAM)ppFiles[i],0);
                     //lstrcat(szHTTPText,szTemp);
                  }
                  szHTTPText="DEBUG";
                  SendDlgItemMessageA(hwndDlg,IDC_MESSAGE,EM_REPLACESEL,TRUE,(LPARAM)szHTTPText);
                  SetFocus(GetDlgItem(hwndDlg,IDC_MESSAGE));
               }
            }
            for(i=0;ppFiles[i];i++) free(ppFiles[i]);
            free(ppFiles);
         }
        }
        return 0;

        case WM_CLOSE: 
        {
            int iTabs, i;
            TCITEM item = {0};
            RECT rc;
            struct ContainerWindowData *pContainer = dat->pContainer;
            
            // esc handles error controls if we are in error state (error controls visible)
            
            if(wParam == 0 && lParam == 0 && dat->dwFlags & MWF_ERRORSTATE) {
                SendMessage(hwndDlg, DM_ERRORDECIDED, MSGERROR_CANCEL, 0);
                return TRUE;
            }
           
            if(wParam == 0 && lParam == 0 && !myGlobals.m_EscapeCloses) {
                SendMessage(hwndContainer, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                return TRUE;
            }
            
            if(dat->iOpenJobs > 0 && lParam != 2) {
                if(dat->dwFlags & MWF_ERRORSTATE)
                    SendMessage(hwndDlg, DM_ERRORDECIDED, MSGERROR_CANCEL, 1);
                else {
                    TCHAR szBuffer[256];
                    _sntprintf(szBuffer, safe_sizeof(szBuffer), TranslateT("Message delivery in progress (%d unsent). You cannot close the session right now"), dat->iOpenJobs);
                    szBuffer[safe_sizeof(szBuffer) - 1] = 0;
                    SendMessage(hwndDlg, DM_ACTIVATETOOLTIP, IDC_MESSAGE, (LPARAM)szBuffer);
                    return TRUE;
                }
            }
            
            if(!lParam) {
                if (myGlobals.m_WarnOnClose) {
                    if (MessageBox(hwndContainer, TranslateTS(szWarnClose), _T("Miranda"), MB_YESNO | MB_ICONQUESTION) == IDNO) {
                        return TRUE;
                    }
                }
            }
            iTabs = TabCtrl_GetItemCount(hwndTab);
            if(iTabs == 1) {
                PostMessage(GetParent(GetParent(hwndDlg)), WM_CLOSE, 0, 1);
                return 1;
            }
                
            m_pContainer->iChilds--;
            i = GetTabIndexFromHWND(hwndTab, hwndDlg);
            
            /*
             * after closing a tab, we need to activate the tab to the left side of
             * the previously open tab.
             * normally, this tab has the same index after the deletion of the formerly active tab
             * unless, of course, we closed the last (rightmost) tab.
             */
            if (!m_pContainer->bDontSmartClose && iTabs > 1) {
                if (i == iTabs - 1)
                    i--;
                else
                    i++;
                TabCtrl_SetCurSel(hwndTab, i);
                item.mask = TCIF_PARAM;
                TabCtrl_GetItem(hwndTab, i, &item);         // retrieve dialog hwnd for the now active tab...

                m_pContainer->hwndActive = (HWND) item.lParam;
                SendMessage(hwndContainer, DM_QUERYCLIENTAREA, 0, (LPARAM)&rc);
                SetWindowPos(m_pContainer->hwndActive, HWND_TOP, rc.left, rc.top, (rc.right - rc.left), (rc.bottom - rc.top), SWP_SHOWWINDOW);
                ShowWindow((HWND)item.lParam, SW_SHOW);
                SetForegroundWindow(m_pContainer->hwndActive);
                SetFocus(m_pContainer->hwndActive);
                SendMessage(hwndContainer, WM_SIZE, 0, 0);
            }
            DestroyWindow(hwndDlg);
            if(iTabs == 1)
                PostMessage(GetParent(GetParent(hwndDlg)), WM_CLOSE, 0, 1);
            else
                SendMessage(pContainer->hwnd, WM_SIZE, 0, 0);
            break;
        }
		case WM_ERASEBKGND:
        {
            if(m_pContainer->bSkinned)
                return TRUE;
            break;
        }
		case WM_NCPAINT:
			if(m_pContainer->bSkinned)
				return 0;
			break;
		case WM_PAINT:
			if(m_pContainer->bSkinned) {
				PAINTSTRUCT ps;
				RECT rcClient, rcWindow, rc;
                StatusItems_t *item;
                POINT pt;
                UINT item_ids[2] = {ID_EXTBKHISTORY, ID_EXTBKINPUTAREA};
                UINT ctl_ids[2] = {IDC_LOG, IDC_MESSAGE};
                int  i;

				HDC hdc = BeginPaint(hwndDlg, &ps);
				GetClientRect(hwndDlg, &rcClient);
				SkinDrawBG(hwndDlg, hwndContainer, m_pContainer, &rcClient, hdc);

                for(i = 0; i < 2; i++) {
                    item = &StatusItems[item_ids[i]];
                    if(!item->IGNORED) {

                        GetWindowRect(GetDlgItem(hwndDlg, ctl_ids[i]), &rcWindow);
                        pt.x = rcWindow.left; pt.y = rcWindow.top;
                        ScreenToClient(hwndDlg, &pt);
                        rc.left = pt.x - item->MARGIN_LEFT;
                        rc.top = pt.y - item->MARGIN_TOP;
                        rc.right = rc.left + item->MARGIN_RIGHT + (rcWindow.right - rcWindow.left) + item->MARGIN_LEFT;
                        rc.bottom = rc.top + item->MARGIN_BOTTOM + (rcWindow.bottom - rcWindow.top) + item->MARGIN_TOP;
                        DrawAlpha(hdc, &rc, item->COLOR, item->ALPHA, item->COLOR2, item->COLOR2_TRANSPARENT, item->GRADIENT,
                                  item->CORNER, item->BORDERSTYLE, item->imageItem);
                    }
                }
				EndPaint(hwndDlg, &ps);
				return 0;
			}
			break;
        case WM_DESTROY:
			if (myGlobals.g_FlashAvatarAvail) {
				FLASHAVATAR fa = {0}; 

                fa.hContact = dat->hContact;
                fa.id = 25367;
                fa.cProto = dat->szProto;
				CallService(MS_FAVATAR_DESTROY, (WPARAM)&fa, 0);
			}
            EnterCriticalSection(&cs_sessions);
            if(!dat->bWasDeleted) {
                TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_CLOSING, 0);
                AddContactToFavorites(dat->hContact, dat->szNickname, dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->szStatus, dat->wStatus, LoadSkinnedProtoIcon(dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat->bIsMeta ? dat->wMetaStatus : dat->wStatus), 1, myGlobals.g_hMenuRecent, dat->codePage);
                if(dat->hContact) {
                    int len;

                    char *msg = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, (CP_UTF8 << 16) | (SF_TEXT|SF_USECODEPAGE));
                    if(msg) {
                        DBWriteContactSettingString(dat->hContact, SRMSGMOD, "SavedMsg", msg);
                        free(msg);
                    }
                    else
                        DBWriteContactSettingString(dat->hContact, SRMSGMOD, "SavedMsg", "");
                    len = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_NOTES));
                    if(len > 0) {
                        msg = malloc(len + 1);
                        GetDlgItemTextA(hwndDlg, IDC_NOTES, msg, len + 1);
                        DBWriteContactSettingString(dat->hContact, "UserInfo", "MyNotes", msg);
                        free(msg);
                    }
                }
            }
            
            if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON)
                NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);

			if (dat->hTheme && pfnCloseThemeData) {
				pfnCloseThemeData(dat->hTheme);
				dat->hTheme = 0;
			}
            if (dat->hBkgBrush)
                DeleteObject(dat->hBkgBrush);
            if (dat->hInputBkgBrush)
                DeleteObject(dat->hInputBkgBrush);
            if (dat->sendBuffer != NULL)
                free(dat->sendBuffer);
			if (dat->hHistoryEvents)
				free(dat->hHistoryEvents);
            {
                int i;
                // input history stuff (free everything)
                if(dat->history) {
                    for(i = 0; i <= dat->iHistorySize; i++) {
                        if(dat->history[i].szText != NULL) {
                            free(dat->history[i].szText);
                        }
                    }
                    free(dat->history);
                }
                /*
                 * search the sendqueue for unfinished send jobs and free them...
                 */
                for(i = 0; i < NR_SENDJOBS; i++) {
                    if(sendJobs[i].hOwner == dat->hContact && sendJobs[i].iStatus != 0) {
                        //_DebugPopup(dat->hContact, "SendQueue: Clearing orphaned send job (%d)", i);
                        ClearSendJob(i);
                    }
                }
                if(dat->hQueuedEvents)
                    free(dat->hQueuedEvents);
            }
            
            if (dat->hSmileyIcon)
                DestroyIcon(dat->hSmileyIcon);

            if (dat->hClientIcon)
                DestroyIcon(dat->hClientIcon);

			if (dat->hXStatusIcon)
				DestroyIcon(dat->hXStatusIcon);

            if (dat->hwndTip)
                DestroyWindow(dat->hwndTip);
            
            UpdateTrayMenuState(dat, FALSE);               // remove me from the tray menu (if still there)
            if(myGlobals.g_hMenuTrayUnread)
                DeleteMenu(myGlobals.g_hMenuTrayUnread, (UINT_PTR)dat->hContact, MF_BYCOMMAND);
            WindowList_Remove(hMessageWindowList, hwndDlg);
            LeaveCriticalSection(&cs_sessions);

            if(!dat->bWasDeleted) {
                SendMessage(hwndDlg, DM_SAVEPERCONTACT, 0, 0);
                if(myGlobals.m_SplitterSaveOnClose)
                    SaveSplitter(hwndDlg, dat);
                if(!dat->stats.bWritten) {
                    WriteStatsOnClose(hwndDlg, dat);
                    dat->stats.bWritten = TRUE;
                }

            }
            
            SetWindowLong(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), GWL_WNDPROC, (LONG) OldSplitterProc);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_PANELSPLITTER), GWL_WNDPROC, (LONG) OldSplitterProc);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_MSGINDICATOR), GWL_WNDPROC, (LONG) OldSplitterProc);

            SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_WNDPROC, (LONG) OldSplitterProc);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_WNDPROC, (LONG) OldMessageEditProc);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_CONTACTPIC), GWL_WNDPROC, (LONG) OldAvatarWndProc);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_PANELPIC), GWL_WNDPROC, (LONG) OldAvatarWndProc);
			SetWindowLong(GetDlgItem(hwndDlg, IDC_LOG), GWL_WNDPROC, (LONG) OldMessageLogProc);

            // remove temporary contacts...
            
            if (!dat->bWasDeleted && !dat->fIsReattach && dat->hContact && DBGetContactSettingByte(NULL, SRMSGMOD_T, "deletetemp", 0)) {
                if (DBGetContactSettingByte(dat->hContact, "CList", "NotOnList", 0)) {
                    CallService(MS_DB_CONTACT_DELETE, (WPARAM)dat->hContact, 0);
                }
            }
            {
                HFONT hFont;
                TCITEM item;
                int i;

                hFont = (HFONT) SendDlgItemMessage(hwndDlg, IDC_MESSAGE, WM_GETFONT, 0, 0);
                if (hFont != NULL && hFont != (HFONT) SendDlgItemMessage(hwndDlg, IDOK, WM_GETFONT, 0, 0))
                    DeleteObject(hFont);
                
                ZeroMemory((void *)&item, sizeof(item));
                item.mask = TCIF_PARAM;
      
                i = GetTabIndexFromHWND(hwndTab, hwndDlg);
                if (i >= 0) {
                    TabCtrl_DeleteItem(hwndTab, i);
                    BroadCastContainer(m_pContainer, DM_REFRESHTABINDEX, 0, 0);
                    dat->iTabID = -1;
                }
            }
            TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_CLOSE, 0);

            if(dat->hContact == myGlobals.hLastOpenedContact)
                myGlobals.hLastOpenedContact = 0;

            // IEVIew MOD Begin
            if (dat->hwndIEView != 0) {
                IEVIEWWINDOW ieWindow;
                ieWindow.cbSize = sizeof(IEVIEWWINDOW);
                ieWindow.iType = IEW_DESTROY;
                ieWindow.hwnd = dat->hwndIEView;
                if(dat->oldIEViewProc) {
                    SetWindowLong(dat->hwndIEView, GWL_WNDPROC, (LONG)dat->oldIEViewProc);
                    dat->oldIEViewProc = 0;
                }
                CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
            }
            // IEVIew MOD End
            break;
        case WM_NCDESTROY:
            if (dat)
                free(dat);
            SetWindowLong(hwndDlg, GWL_USERDATA, 0);
            break;
    }
    return FALSE;
}

/*
 * stream function to write the contents of the message log to an rtf file
 */

static DWORD CALLBACK StreamOut(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb)
{                                                                                                        
    HANDLE hFile;

    char *szFilename = (char *)dwCookie;
    if(( hFile = CreateFileA(szFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE )      
    {                                                                                                    
        SetFilePointer(hFile, 0, NULL, FILE_END);                                                        
        WriteFile(hFile, pbBuff, cb, (DWORD *)pcb, NULL);                                                         
        *pcb = cb;                                                                                       
        CloseHandle(hFile);                                                                              
        return 0;                                                                                        
    }                                                                                                    
    return 1;                                                                                            
}

