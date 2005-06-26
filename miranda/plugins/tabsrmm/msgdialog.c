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
#include "m_ieview.h"
// IEVIew MOD End
#include "../../include/m_clc.h"
#include "../../include/m_clui.h"
#include "../../include/m_userinfo.h"
#include "../../include/m_history.h"
#include "../../include/m_addcontact.h"
#include "../../include/m_file.h"
#include "msgs.h"
#include "m_popup.h"
#include "nen.h"
#include "m_smileyadd.h"
#include "m_metacontacts.h"

#include "sendqueue.h"
#include "msgdlgutils.h"
#include "functions.h"

#define TOOLBAR_PROTO_HIDDEN 1
#define TOOLBAR_SEND_HIDDEN 2

#ifdef __MATHMOD_SUPPORT
	#include "m_MathModule.h"
#endif

extern MYGLOBALS myGlobals;
extern NEN_OPTIONS nen_options;
extern TemplateSet RTL_Active, LTR_Active;
extern LOGFONTA logfonts[MSGDLGFONTCOUNT + 2];
extern COLORREF fontcolors[MSGDLGFONTCOUNT + 2];
extern HANDLE hMessageWindowList;
extern struct CREOleCallback reOleCallback;
extern HINSTANCE g_hInst;
extern char *szWarnClose;
extern struct RTFColorTable rtf_ctable[];
extern PSLWA pSetLayeredWindowAttributes;

wchar_t *testTooltip = L"Ein tooltip text zum testen";
char *xStatusDescr[] = { "Angry", "Duck", "Tired", "Party", "Beer", "Thinking", "Eating", "TV", "Friends", "Coffee",
                         "Music", "Business", "Camera", "Funny", "Phone", "Games", "College", "Shopping", "Sick", "Sleeping",
                         "Surfing", "@Internet", "Engeieering", "Typing"};
                         
int GetTabIndexFromHWND(HWND hwndTab, HWND hwndDlg);
int ActivateTabFromHWND(HWND hwndTab, HWND hwndDlg);

int CutContactName(char *old, char *new, int size);

TCHAR *QuoteText(TCHAR *text, int charsPerLine, int removeExistingQuotes);
void _DBWriteContactSettingWString(HANDLE hContact, const char *szKey, const char *szSetting, const wchar_t *value);
int MessageWindowOpened(WPARAM wParam, LPARAM LPARAM);
static DWORD CALLBACK StreamOut(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb);
HMENU BuildMCProtocolMenu(HWND hwndDlg);
int tabSRMM_ShowPopup(WPARAM wParam, LPARAM lParam, WORD eventType, int windowOpen, struct ContainerWindowData *pContainer, HWND hwndChild, char *szProto, struct MessageWindowData *dat);

char *pszIDCSAVE_close = 0, *pszIDCSAVE_save = 0;

static void FlashTab(struct MessageWindowData *dat, HWND hwndTab, int iTabindex, BOOL *bState, BOOL mode, HICON origImage);
void FlashContainer(struct ContainerWindowData *pContainer, int iMode, int iCount);
void ReflashContainer(struct ContainerWindowData *pContainer);
void UpdateContainerMenu(HWND hwndDlg, struct MessageWindowData *dat);

void ImageDataInsertBitmap(IRichEditOle *ole, HBITMAP hbm);

void TABSRMM_FireEvent(HANDLE hContact, HWND hwndDlg, unsigned int type, unsigned int subType);
struct ContainerWindowData *FindContainerByName(const TCHAR *name);
int GetContainerNameForContact(HANDLE hContact, TCHAR *szName, int iNameLen);
struct ContainerWindowData *CreateContainer(const TCHAR *name, int iMode, HANDLE hContactFrom);

static WNDPROC OldMessageEditProc, OldSplitterProc, OldAvatarWndProc;

static const UINT infoLineControls[] = { IDC_PROTOCOL, IDC_PROTOMENU, IDC_NAME, IDC_INFOPANELMENU};
static const UINT buttonLineControlsNew[] = { IDC_PIC, IDC_HISTORY, IDC_TIME, IDC_QUOTE, IDC_SAVE, IDC_SENDMENU};
static const UINT sendControls[] = { IDC_MESSAGE, IDC_LOG };
static const UINT formatControls[] = { IDC_SMILEYBTN, IDC_FONTBOLD, IDC_FONTITALIC, IDC_FONTUNDERLINE, IDC_FONTFACE }; //, IDC_FONTFACE, IDC_FONTCOLOR};
static const UINT controlsToHide[] = { IDOK, IDC_PIC, IDC_PROTOCOL, IDC_FONTFACE, IDC_FONTUNDERLINE, IDC_HISTORY, -1 };
static const UINT controlsToHide1[] = { IDOK, IDC_FONTFACE, IDC_FONTUNDERLINE, IDC_FONTITALIC, IDC_FONTBOLD, IDC_PROTOCOL, -1 };
static const UINT controlsToHide2[] = { IDOK, IDC_PIC, IDC_PROTOCOL, -1};
static const UINT addControls[] = { IDC_ADD, IDC_CANCELADD };

const UINT infoPanelControls[] = {IDC_PANELPIC, IDC_PANEL, IDC_PANELNICK, IDC_PANELUIN, IDC_PANELNICKLABEL, IDC_PANELUINLABEL, 
                                  IDC_PANELSTATUS, IDC_APPARENTMODE, IDC_TOGGLENOTES, IDC_NOTES, IDC_PANELSPLITTER, IDC_READSTATUS};
const UINT errorControls[] = { IDC_STATICERRORICON, IDC_STATICTEXT, IDC_RETRY, IDC_CANCELSEND, IDC_MSGSENDLATER};

static struct _tooltips { int id; char *szTip;} tooltips[] = {
    IDC_HISTORY, "View User's History",
    IDC_TIME, "Message Log Options",
    IDC_SENDMENU, "Send Menu",
    IDC_QUOTE, "Quote last message OR selected text",
    IDC_PIC, "Avatar Options",
    IDC_SMILEYBTN, "Insert Emoticon",
    IDC_ADD, "Add this contact permanently to your contact list",
    IDC_CANCELADD, "Do not add this contact permanently",
    IDOK, "Send message",
    IDC_PROTOMENU, "Protocol Menu",
    IDC_FONTBOLD, "Bold text",
    IDC_FONTITALIC, "Italic text",
    IDC_FONTUNDERLINE, "Underlined text",
    IDC_FONTFACE, "Select font color",
    IDC_INFOPANELMENU, "Info panel controls",
    IDC_TOGGLENOTES, "Toggle notes display",
    IDC_APPARENTMODE, "Set your visibility for this contact",
    -1, NULL
};

static struct _buttonicons { int id; HICON *pIcon; } buttonicons[] = {
    IDC_HISTORY, &myGlobals.g_buttonBarIcons[1],
    IDC_TIME, &myGlobals.g_buttonBarIcons[2],
    IDC_SENDMENU, &myGlobals.g_buttonBarIcons[16],
    IDC_QUOTE, &myGlobals.g_buttonBarIcons[8],
    IDC_SAVE, &myGlobals.g_buttonBarIcons[6],
    IDC_PIC, &myGlobals.g_buttonBarIcons[10],
    IDOK, &myGlobals.g_buttonBarIcons[9],
    IDC_ADD, &myGlobals.g_buttonBarIcons[0],
    IDC_CANCELADD, &myGlobals.g_buttonBarIcons[6],
    IDC_PROTOMENU, &myGlobals.g_buttonBarIcons[16],
    IDC_FONTBOLD, &myGlobals.g_buttonBarIcons[17],
    IDC_FONTITALIC, &myGlobals.g_buttonBarIcons[18],
    IDC_FONTUNDERLINE, &myGlobals.g_buttonBarIcons[19],
    IDC_FONTFACE, &myGlobals.g_buttonBarIcons[21],
    IDC_NAME, &myGlobals.g_buttonBarIcons[4],
    IDC_LOGFROZEN, &myGlobals.g_buttonBarIcons[24],
    IDC_INFOPANELMENU, &myGlobals.g_buttonBarIcons[16],
    IDC_TOGGLENOTES, &myGlobals.g_buttonBarIcons[28],
    -1, NULL
};

struct SendJob sendJobs[NR_SENDJOBS];

static int splitterEdges = TRUE;

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
    ieWindow.hwnd = dat->hwndLog;
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
    GetWindowRect(dat->hwndLog, &rcIeView);
    if(ieWindow.cx != 0 && ieWindow.cy != 0) {
        CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
    }
}

static void ConfigurePanel(HWND hwndDlg, struct MessageWindowData *dat)
{
    const UINT cntrls[] = {IDC_PANELNICKLABEL, IDC_PANELUINLABEL, IDC_PANELNICK, IDC_PANELUIN, IDC_PANELSTATUS, IDC_APPARENTMODE, IDC_READSTATUS};

    ShowMultipleControls(hwndDlg, cntrls, 7, dat->dwEventIsShown & MWF_SHOW_INFONOTES ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hwndDlg, IDC_NOTES), dat->dwEventIsShown & MWF_SHOW_INFONOTES ? SW_SHOW : SW_HIDE);
}
static void ShowHideInfoPanel(HWND hwndDlg, struct MessageWindowData *dat)
{
    HBITMAP hbm = dat->dwEventIsShown & MWF_SHOW_INFOPANEL ? dat->hOwnPic : dat->hContactPic;
    BITMAP bm;
    
    if(dat->dwFlags & MWF_ERRORSTATE)
        return;
    
    dat->iRealAvatarHeight = 0;
    AdjustBottomAvatarDisplay(hwndDlg, dat);
    GetObject(hbm, sizeof(bm), &bm);
    CalcDynamicAvatarSize(hwndDlg, dat, &bm);
    ShowMultipleControls(hwndDlg, infoPanelControls, 12, dat->dwEventIsShown & MWF_SHOW_INFOPANEL ? SW_SHOW : SW_HIDE);
    if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL)
        ConfigurePanel(hwndDlg, dat);
    SendMessage(hwndDlg, WM_SIZE, 0, 0);
    SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 1);
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
    HICON hButtonIcon = 0;
    int nrSmileys = 0;
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

    ShowWindow(GetDlgItem(hwndDlg, IDC_MULTIPLEICON), (dat->sendMode & SMODE_MULTIPLE || dat->sendMode & SMODE_CONTAINER) ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_TOGGLETOOLBAR), SW_HIDE);

    ShowWindow(GetDlgItem(hwndDlg, IDC_LOGFROZEN), SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_LOGFROZENTEXT), SW_HIDE);
    
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIME), TRUE);
    
	if (dat->hwndLog) {
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
    
    if(myGlobals.g_SmileyAddAvail && myGlobals.m_SmileyPluginEnabled && dat->hwndLog == 0) {
        nrSmileys = CheckValidSmileyPack(dat->bIsMeta ? dat->szMetaProto : dat->szProto, &hButtonIcon);

        dat->doSmileys = 1;
        
        if(hButtonIcon == 0) {
            dat->hSmileyIcon = 0;
            SendDlgItemMessage(hwndDlg, IDC_SMILEYBTN, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[11]);
        }
        else {
            SendDlgItemMessage(hwndDlg, IDC_SMILEYBTN, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hButtonIcon);
            dat->hSmileyIcon = 0;
        }
    }
    else if(dat->hwndLog != 0) {
        dat->doSmileys = 1;
        nrSmileys = 1;
        if(dat->hSmileyIcon == 0) {
            DeleteObject(dat->hSmileyIcon);
            dat->hSmileyIcon = 0;
        }
        SendDlgItemMessage(hwndDlg, IDC_SMILEYBTN, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[11]);
    }
    
    if(nrSmileys == 0 || dat->hContact == 0)
        dat->doSmileys = 0;
    
    ShowWindow(GetDlgItem(hwndDlg, IDC_SMILEYBTN), (dat->doSmileys && showToolbar) ? SW_SHOW : SW_HIDE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SMILEYBTN), dat->doSmileys ? TRUE : FALSE);
    
    if(dat->pContainer->hwndActive == hwndDlg)
        UpdateReadChars(hwndDlg, dat);

    SetDlgItemTextA(hwndDlg, IDOK, "Send");
    SetDlgItemTextA(hwndDlg, IDC_STATICTEXT, Translate("A message failed to send successfully."));

    SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
    GetAvatarVisibility(hwndDlg, dat);
    
    ShowWindow(GetDlgItem(hwndDlg,IDC_CONTACTPIC), dat->showPic ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_SPLITTER5), (showToolbar && splitterEdges) ? SW_SHOW : SW_HIDE);

    ShowWindow(GetDlgItem(hwndDlg, IDC_SPLITTER), SW_SHOW);
    ShowWindow(GetDlgItem(hwndDlg, IDOK), showToolbar ? SW_SHOW : SW_HIDE);

    ShowWindow(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), (dat->sendMode & SMODE_MULTIPLE) ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_CLIST), (dat->sendMode & SMODE_MULTIPLE) ? SW_SHOW : SW_HIDE);
    
    EnableWindow(GetDlgItem(hwndDlg, IDOK), GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE)) != 0);
    SendMessage(hwndDlg, DM_UPDATETITLE, 0, 1);
    SendMessage(hwndDlg, WM_SIZE, 0, 0);

    EnableWindow(GetDlgItem(hwndDlg, IDC_CONTACTPIC), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PANELPIC), FALSE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_TOGGLESIDEBAR), myGlobals.m_SideBarEnabled ? SW_SHOW : SW_HIDE);

    // info panel stuff
    ShowMultipleControls(hwndDlg, infoPanelControls, 12, dat->dwEventIsShown & MWF_SHOW_INFOPANEL ? SW_SHOW : SW_HIDE);
    if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL)
        ConfigurePanel(hwndDlg, dat);
}

static LRESULT CALLBACK MessageEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct MsgEditSubclassData *dat;
    struct MessageWindowData *mwdat = (struct MessageWindowData *)GetWindowLong(GetParent(hwnd), GWL_USERDATA);

    dat = (struct MsgEditSubclassData *) GetWindowLong(hwnd, GWL_USERDATA);
    switch (msg) {
		case WM_DROPFILES:
			SendMessage(GetParent(hwnd),WM_DROPFILES,(WPARAM)wParam,(LPARAM)lParam);
			break;
        case WM_CHAR:
            if (wParam == 0x0d && (GetKeyState(VK_CONTROL) & 0x8000) && myGlobals.m_MathModAvail) {
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
            if((GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000)) {
                switch(wParam) {
                    case 0x02:               // bold
                        if(mwdat->SendFormat) {
                            SendMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(IDC_FONTBOLD, IDC_MESSAGE), 0);
                        }
                        return 0;
                    case 0x09:
                        if(mwdat->SendFormat) {
                            SendMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(IDC_FONTITALIC, IDC_MESSAGE), 0);
                        }
                        return 0;
                    case 21:
                        if(mwdat->SendFormat) {
                            SendMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(IDC_FONTUNDERLINE, IDC_MESSAGE), 0);
                        }
                        return 0;
                    case 25:
                        PostMessage(GetParent(hwnd), DM_SPLITTEREMERGENCY, 0, 0);
                        return 0;
                }
                break;
            }
            break;
        case WM_MOUSEWHEEL:
        {
            RECT rc;
            POINT pt;
            TCHITTESTINFO hti;
            HWND hwndTab;
            
            if(myGlobals.m_WheelDefault)
                break;
            
            GetCursorPos(&pt);
            GetWindowRect(hwnd, &rc);
            if(PtInRect(&rc, pt))
                break;
            if(mwdat->pContainer->dwFlags & CNT_SIDEBAR) {
                GetWindowRect(GetDlgItem(mwdat->pContainer->hwnd, IDC_SIDEBAR), &rc);
                if(PtInRect(&rc, pt)) {
                    short amount = (short)(HIWORD(wParam));
                    SendMessage(mwdat->pContainer->hwnd, WM_COMMAND, MAKELONG(amount > 0 ? IDC_SIDEBARUP : IDC_SIDEBARDOWN, 0), 0);
                    return 0;
                }
            }
            GetWindowRect(GetDlgItem(GetParent(hwnd), IDC_LOG), &rc);
            if(PtInRect(&rc, pt)) {
                if(mwdat->hwndLog != 0) {
                    /*int i;
                    WPARAM wParam;
                    short amount = (short)(HIWORD(wParam));
                    if(amount > 0)
                        wParam = (WPARAM)MAKELONG(SB_LINEUP, 0);
                    else if(amount < 0)
                        wParam = (WPARAM)MAKELONG(SB_LINEDOWN, 0);
                    amount = abs(amount) / WHEEL_DELTA;
                    _DebugPopup(0, "amount: %d", amount);
                    for(i = 0; i < amount; i++)
                        SendMessage(mwdat->hwndLog, WM_VSCROLL, wParam, 0);*/
                    return 0;
                }
                else
                    SendMessage(GetDlgItem(GetParent(hwnd), IDC_LOG), WM_MOUSEWHEEL, wParam, lParam);
                return 0;
            }
            hwndTab = GetDlgItem(mwdat->pContainer->hwnd, IDC_MSGTABS);
            GetCursorPos(&hti.pt);
            ScreenToClient(hwndTab, &hti.pt);
            hti.flags = 0;
            if(TabCtrl_HitTest(hwndTab, &hti) != -1) {
                SendMessage(hwndTab, WM_MOUSEWHEEL, wParam, -1);
                return 0;
            }
            break;
        }
        case WM_KEYUP:
            break;
        case WM_KEYDOWN:
            if(wParam == VK_RETURN) {
                if (GetKeyState(VK_SHIFT) & 0x8000) {
                    if(myGlobals.m_SendOnShiftEnter) {
                        PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
                        return 0;
                    }
                    else
                        break;
                }
                if (((GetKeyState(VK_CONTROL) & 0x8000) != 0 && !(GetKeyState(VK_SHIFT) & 0x8000)) ^ (0 != myGlobals.m_SendOnEnter)) {
                    PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
                    return 0;
                }
                if(myGlobals.m_SendOnEnter) {
                    if(GetKeyState(VK_CONTROL) & 0x8000)
                        break;
                    else {
                        PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
                        return 0;
                    }
                }
                else 
                    break;
            }
            if ((GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000) && !(GetKeyState(VK_SHIFT) & 0x8000)) {
                if (!(GetKeyState(VK_SHIFT) & 0x8000) && (wParam == VK_UP || wParam == VK_DOWN)) {          // input history scrolling (ctrl-up / down)
                    SETTEXTEX stx = {ST_DEFAULT,CP_UTF8};
                    if(mwdat) {
                        if(mwdat->history != NULL && mwdat->history[0].szText != NULL) {      // at least one entry needs to be alloced, otherwise we get a nice infinite loop ;)
                            if(mwdat->dwFlags & MWF_NEEDHISTORYSAVE) {
                                mwdat->iHistoryCurrent = mwdat->iHistoryTop;
                                if(GetWindowTextLengthA(hwnd) > 0)
                                    SaveInputHistory(GetParent(hwnd), mwdat, (WPARAM)mwdat->iHistorySize, 0);
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
                            SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), EN_CHANGE), (LPARAM) hwnd);
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
            if ((GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_MENU) & 0x8000) && !(GetKeyState(VK_SHIFT) & 0x8000)) {
                switch (wParam) {
                    case VK_UP:
                    case VK_DOWN:
                    case VK_PRIOR:
                    case VK_NEXT:
                    case VK_HOME:
                    case VK_END:
                    {
                        WPARAM wp = 0;
                        if (wParam == VK_UP)
                            wp = MAKEWPARAM(SB_LINEUP, 0);
                        else if (wParam == VK_PRIOR)
                            wp = MAKEWPARAM(SB_PAGEUP, 0);
                        else if (wParam == VK_NEXT)
                            wp = MAKEWPARAM(SB_PAGEDOWN, 0);
                        else if (wParam == VK_HOME)
                            wp = MAKEWPARAM(SB_TOP, 0);
                        else if (wParam == VK_END) {
                            SendMessage(GetParent(hwnd), DM_SCROLLLOGTOBOTTOM, 0, 0);
                            return 0;
                        } else if (wParam == VK_DOWN)
                            wp = MAKEWPARAM(SB_LINEDOWN, 0);

                        SendMessage(GetDlgItem(GetParent(hwnd), IDC_LOG), WM_VSCROLL, wp, 0);
                        return 0;
                    }
                }
            }
            if (wParam == VK_RETURN)
                break;
        case WM_SYSCHAR:
        {
            HWND hwndDlg = GetParent(hwnd);
            
            if(GetKeyState(VK_MENU) & 0x8000) {
                switch (VkKeyScan((TCHAR)wParam)) {
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
                    case 'P':
                        SendMessage(hwndDlg, WM_COMMAND, IDC_PIC, 0);
                        return 0;
                    case 'D':
                        SendMessage(hwndDlg, WM_COMMAND, IDC_PROTOCOL, 0);
                        return 0;
                    case 'U':
                        SendMessage(hwndDlg, WM_COMMAND, IDC_USERMENU, 0);
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
                        mwdat->dwEventIsShown ^= MWF_SHOW_INFOPANEL;
                        ShowHideInfoPanel(GetParent(hwnd), mwdat);
                        return 0;
                    case 'B':
                        MsgWindowMenuHandler(GetParent(hwnd), mwdat, ID_LOGMENU_ACTIVATERTL, MENU_LOGMENU);
                        return 0;
                    case 'M':
                        mwdat->sendMode ^= SMODE_MULTIPLE;
                        if(mwdat->sendMode & SMODE_MULTIPLE || mwdat->sendMode & SMODE_CONTAINER)
                            ShowWindow(GetDlgItem(hwndDlg, IDC_MULTIPLEICON), SW_SHOW);
                        else
                            ShowWindow(GetDlgItem(hwndDlg, IDC_MULTIPLEICON), SW_HIDE);
                        SendMessage(hwndDlg, WM_SIZE, 0, 0);
                        SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 0);
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
                SendMessage(GetParent(hwnd), DM_SETLOCALE, wParam, lParam);
                PostMessage(GetParent(hwnd), DM_SAVELOCALE, 0, 0);
            }
            break;
            */
            return DefWindowProc(hwnd, WM_INPUTLANGCHANGEREQUEST, wParam, lParam);
        }
        case WM_INPUTLANGCHANGE:
            if (myGlobals.m_AutoLocaleSupport && GetFocus() == hwnd && mwdat->pContainer->hwndActive == GetParent(hwnd) && GetForegroundWindow() == mwdat->pContainer->hwnd && GetActiveWindow() == mwdat->pContainer->hwnd) {
                SendMessage(GetParent(hwnd), DM_SAVELOCALE, wParam, lParam);
            }
            return 1;
        case WM_ERASEBKGND:
        {
            HDC hdcMem = CreateCompatibleDC((HDC)wParam);
            HBITMAP hbmMem = (HBITMAP)SelectObject(hdcMem, mwdat->hbmMsgArea);
            RECT rc;
            BITMAP bminfo;
            
            GetObject(mwdat->hbmMsgArea, sizeof(bminfo), &bminfo);
            GetClientRect(hwnd, &rc);
            SetStretchBltMode((HDC)wParam, HALFTONE);
            StretchBlt((HDC)wParam, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, bminfo.bmWidth, bminfo.bmHeight, SRCCOPY);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);
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

static LRESULT CALLBACK SplitterSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
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
            SetCapture(hwnd);
            return 0;
        case WM_MOUSEMOVE:
            if (GetCapture() == hwnd) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                SendMessage(GetParent(hwnd), DM_SPLITTERMOVED, rc.right > rc.bottom ? (short) HIWORD(GetMessagePos()) + rc.bottom / 2 : (short) LOWORD(GetMessagePos()) + rc.right / 2, (LPARAM) hwnd);
            }
            return 0;
        case WM_LBUTTONUP:
            ReleaseCapture();
            SendMessage(GetParent(hwnd), DM_SCROLLLOGTOBOTTOM, 0, 1);
            return 0;
    }
    return CallWindowProc(OldSplitterProc, hwnd, msg, wParam, lParam);
}

/*
 * resizer proc for the "new" layout.
 */

static int MessageDialogResize(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL * urc)
{
    struct MessageWindowData *dat = (struct MessageWindowData *) lParam;
    int iClistOffset = 0;
    RECT rc, rcButton;
    static int uinWidth;
    int showToolbar = dat->pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;
    
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

    switch (urc->wId) {
        case IDC_NAME:
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if(dat->controlsHidden & TOOLBAR_PROTO_HIDDEN)
                OffsetRect(&urc->rcItem, -(rcButton.right + 10), 0);
             return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
        case IDC_SMILEYBTN:
        case IDC_FONTBOLD:
        case IDC_FONTITALIC:
        case IDC_FONTUNDERLINE:
        case IDC_FONTFACE:
        case IDC_FONTCOLOR:
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if(!dat->doSmileys)
                OffsetRect(&urc->rcItem, -22, 0);
            if(dat->controlsHidden & TOOLBAR_PROTO_HIDDEN)
                OffsetRect(&urc->rcItem, -(rcButton.right + 10), 0);
            return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
        case IDC_TOGGLENOTES:
            return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;
        case IDC_PANELSPLITTER:
            urc->rcItem.bottom = dat->panelHeight;
            urc->rcItem.top = dat->panelHeight - 2;
            return RD_ANCHORX_WIDTH | RD_ANCHORY_TOP;
        case IDC_NOTES:
            urc->rcItem.bottom = dat->panelHeight - 4;
            urc->rcItem.right = urc->dlgNewSize.cx - (dat->panelHeight + 2);
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_TOP;
        case IDC_RETRY:
        case IDC_CANCELSEND:
        case IDC_MSGSENDLATER:
        case IDC_STATICTEXT:
        case IDC_STATICERRORICON:
            if(dat->dwEventIsShown & MWF_SHOW_SCROLLINGDISABLED)
                OffsetRect(&urc->rcItem, 0, 24);
            return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;
        case IDC_PROTOCOL:
        case IDC_PROTOMENU:
        case IDC_PIC:
        case IDC_USERMENU:
        case IDC_HISTORY:
        case IDC_TIME:
        case IDC_QUOTE:
        case IDC_SAVE:
        case IDC_INFOPANELMENU:
            if(urc->wId != IDC_PROTOCOL && urc->wId != IDC_PROTOMENU && urc->wId != IDC_INFOPANELMENU)
                OffsetRect(&urc->rcItem, 12, 0);
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if(dat->controlsHidden & TOOLBAR_PROTO_HIDDEN && urc->wId == IDC_PROTOMENU)
                OffsetRect(&urc->rcItem, -(rcButton.right + 10), 0);
            if (urc->wId == IDC_PROTOCOL || urc->wId == IDC_PROTOMENU || urc->wId == IDC_INFOPANELMENU)
                return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
            if (showToolbar && !(dat->controlsHidden & TOOLBAR_SEND_HIDDEN)) {
                urc->rcItem.left -= 40;
                urc->rcItem.right -= 40;
            }
            if (dat->showPic && (dat->splitterY <= (dat->bottomOffset + (dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC ? 32 : 25))))
                OffsetRect(&urc->rcItem, -(dat->pic.cx + 2), 0);
            return RD_ANCHORX_RIGHT | RD_ANCHORY_BOTTOM;
        case IDC_MULTIPLEICON:
            if(myGlobals.m_SideBarEnabled)
               OffsetRect(&urc->rcItem, 10, 0);
            return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
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
            return RD_ANCHORX_RIGHT | RD_ANCHORY_BOTTOM;
        case IDC_LOG:
            if(dat->dwFlags & MWF_ERRORSTATE && !(dat->dwEventIsShown & MWF_SHOW_INFOPANEL))
                urc->rcItem.top += 38;
            if (dat->sendMode & SMODE_MULTIPLE)
                urc->rcItem.right -= (dat->multiSplitterX + 3);
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if (!showToolbar)
                urc->rcItem.bottom += (splitterEdges ? 24 : 26);
            if(!splitterEdges)
                urc->rcItem.bottom += 2;
            if(dat->dwEventIsShown & MWF_SHOW_SCROLLINGDISABLED)
                urc->rcItem.top += 24;
            if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL)
                urc->rcItem.top += dat->panelHeight;
            return RD_ANCHORX_WIDTH | RD_ANCHORY_HEIGHT;
        case IDC_PANELPIC:
            urc->rcItem.left = urc->rcItem.right - (dat->panelHeight - 2);
            urc->rcItem.bottom = urc->rcItem.top + (dat->panelHeight - 2);
            return RD_ANCHORX_RIGHT | RD_ANCHORY_TOP;
        case IDC_PANEL:
            return RD_ANCHORX_WIDTH | RD_ANCHORY_TOP;
        case IDC_PANELSTATUS:
            urc->rcItem.right = urc->dlgNewSize.cx - dat->panelHeight - 15;
            urc->rcItem.left = urc->dlgNewSize.cx - dat->panelHeight - 15 - dat->panelStatusCX;
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_TOP;
        case IDC_READSTATUS:
            urc->rcItem.right = urc->dlgNewSize.cx - (dat->panelHeight);
            urc->rcItem.left = urc->rcItem.right - 14;
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_TOP;
        case IDC_PANELNICK:
            urc->rcItem.right = urc->dlgNewSize.cx - dat->panelHeight - 15 - dat->panelStatusCX - 2;
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_TOP;
        case IDC_APPARENTMODE:
            urc->rcItem.right -= (dat->panelHeight + 3);
            urc->rcItem.left -= (dat->panelHeight + 3);
            return RD_ANCHORX_CUSTOM | RD_ANCHORX_RIGHT;
        case IDC_PANELUIN:
            urc->rcItem.right = urc->dlgNewSize.cx - (dat->panelHeight + 2 + 25);
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_TOP;
        case IDC_PANELNICKLABEL:
        case IDC_PANELUINLABEL:
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_TOP;
        case IDC_SPLITTER:
        case IDC_SPLITTER5:
            urc->rcItem.right +=1;
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if (urc->wId == IDC_SPLITTER && dat->splitterY <= (dat->bottomOffset + (dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC ? 32 : 25)) && dat->showPic && showToolbar)
                urc->rcItem.right -= (dat->pic.cx + 2);
            return RD_ANCHORX_WIDTH | RD_ANCHORY_BOTTOM;
        case IDC_CONTACTPIC:
            urc->rcItem.top=urc->rcItem.bottom-(dat->pic.cy +2);
            urc->rcItem.left=urc->rcItem.right-(dat->pic.cx +2);
            return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
        case IDC_MESSAGE:
            urc->rcItem.right = urc->dlgNewSize.cx;
            if (dat->showPic)
                urc->rcItem.right -= dat->pic.cx+2;
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            if(dat->sendMode & SMODE_MULTIPLE || dat->sendMode & SMODE_CONTAINER)
                urc->rcItem.left += 26;
            if(dat->bNotOnList) {
                if(urc->rcItem.bottom - urc->rcItem.top > 48)
                    urc->rcItem.right -= 26;
                else
                    urc->rcItem.right -= 52;
            }
            if(myGlobals.m_SideBarEnabled)
               urc->rcItem.left += 10;
            return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
        case IDC_TOGGLESIDEBAR:
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
        case IDOK:
        case IDC_SENDMENU:
            OffsetRect(&urc->rcItem, 12, 0);
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;

            if (dat->showPic && (dat->splitterY <= (dat->bottomOffset + (dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC ? 32 : 25))))
                OffsetRect(&urc->rcItem, -(dat->pic.cx + 2), 0);
            return RD_ANCHORX_RIGHT | RD_ANCHORY_BOTTOM;
        case IDC_MULTISPLITTER:
            if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL)
                urc->rcItem.top += dat->panelHeight;
            urc->rcItem.left -= dat->multiSplitterX;
            urc->rcItem.right -= dat->multiSplitterX;
            urc->rcItem.bottom = iClistOffset;
            if(!splitterEdges)
                urc->rcItem.bottom += 2;
            return RD_ANCHORX_RIGHT | RD_ANCHORY_CUSTOM;
        case IDC_CLIST:
            if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL)
                urc->rcItem.top += dat->panelHeight;
            urc->rcItem.left = urc->dlgNewSize.cx - dat->multiSplitterX;
            urc->rcItem.right = urc->dlgNewSize.cx - 3;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if (!showToolbar)
                urc->rcItem.bottom += (splitterEdges ? 24 : 26);
            if(!splitterEdges)
                urc->rcItem.bottom += 2;
            if(dat->dwEventIsShown & MWF_SHOW_SCROLLINGDISABLED)
                urc->rcItem.top += 24;
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_HEIGHT;
        case IDC_LOGFROZEN:
            if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL)
                OffsetRect(&urc->rcItem, 0, dat->panelHeight);
            return RD_ANCHORX_RIGHT | RD_ANCHORY_TOP;
        case IDC_LOGFROZENTEXT:
            urc->rcItem.right = urc->dlgNewSize.cx - 20;
            if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL)
                OffsetRect(&urc->rcItem, 0, dat->panelHeight);
            return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;
    }
    return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
}

#ifdef __MATHMOD_SUPPORT
//mathMod begin
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
    WINDOWPLACEMENT  cWinPlace;

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
//mathMod end
#endif

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
    HWND   hwndTab;
    
    dat = (struct MessageWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);
    
    hwndTab = GetParent(hwndDlg);
    
    if(dat == 0) {
        if(dat == NULL && (msg == WM_ACTIVATE || msg == WM_SETFOCUS))
            return 0;
    }
    
    switch (msg) {
        case WM_INITDIALOG:
            {
                RECT rc;
                POINT pt;
                int i;
                
                struct NewMessageWindowLParam *newData = (struct NewMessageWindowLParam *) lParam;
                dat = (struct MessageWindowData *) malloc(sizeof(struct MessageWindowData));
                ZeroMemory((void *) dat, sizeof(struct MessageWindowData));
                if (newData->iTabID >= 0)
                    dat->pContainer = newData->pContainer;
                SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) dat);

                dat->wOldStatus = -1;
                dat->iOldHash = -1;

                newData->item.lParam = (LPARAM) hwndDlg;
                TabCtrl_SetItem(hwndTab, newData->iTabID, &newData->item);
                dat->iTabID = newData->iTabID;
                
                pszIDCSAVE_close = Translate("Close session");
                pszIDCSAVE_save = Translate("Save and close session");

                dat->hContact = newData->hContact;
                WindowList_Add(hMessageWindowList, hwndDlg, dat->hContact);
                BroadCastContainer(dat->pContainer, DM_REFRESHTABINDEX, 0, 0);
                
                dat->szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)dat->hContact, 0);
                dat->bIsMeta = IsMetaContact(hwndDlg, dat) ? TRUE : FALSE;
                if(dat->hContact && dat->szProto != NULL) {
                    dat->wStatus = DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE);
                    mir_snprintf(dat->szNickname, 80, "%s", (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) dat->hContact, 0));
                    dat->szStatus = (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, dat->szProto == NULL ? ID_STATUS_OFFLINE : dat->wStatus, 0);
                }
                else
                    dat->wStatus = ID_STATUS_OFFLINE;
                
                GetContactUIN(hwndDlg, dat);
                dat->showUIElements = dat->pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;
                dat->sendMode |= DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "forceansi", 0) ? SMODE_FORCEANSI : 0;
                dat->sendMode |= dat->hContact == 0 ? SMODE_MULTIPLE : 0;
                dat->sendMode |= DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "no_ack", 0) ? SMODE_NOACK : 0;
                
                dat->hwnd = hwndDlg;

                dat->ltr_templates = &LTR_Active;
                dat->rtl_templates = &RTL_Active;
                // input history stuff (initialise it..)

                dat->iHistorySize = DBGetContactSettingByte(NULL, SRMSGMOD_T, "historysize", 10);
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

                if(!DBGetContactSettingByte(NULL, SRMSGMOD_T, "splitteredges", 1)) {
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER5), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER5), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
                    splitterEdges = FALSE;
                }
                else
                    splitterEdges = TRUE;
                if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "flatlog", 0)) {
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_LOG), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
                    if(dat->hwndLog)
                        SetWindowLong(dat->hwndLog, GWL_EXSTYLE, GetWindowLong(dat->hwndLog, GWL_EXSTYLE) & ~(WS_EX_STATICEDGE | WS_EX_CLIENTEDGE));
                }

                if(dat->bIsMeta)
                    SendMessage(hwndDlg, DM_UPDATEMETACONTACTINFO, 0, 0);
                else
                    SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
                dat->bTabFlash = FALSE;
                dat->mayFlashTab = FALSE;
                //dat->iTabImage = newData->iTabImage;

                
                dat->multiSplitterX = (int) DBGetContactSettingDword(NULL, SRMSGMOD, "multisplit", 150);
                dat->showTypingWin = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGWIN, SRMSGDEFSET_SHOWTYPINGWIN);
                dat->nTypeMode = PROTOTYPE_SELFTYPING_OFF;
                SetTimer(hwndDlg, TIMERID_TYPE, 1000, NULL);
                dat->iLastEventType = 0xffffffff;
                
                // load log option flags...
                dat->dwFlags = (DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT) & MWF_LOG_ALL);

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
                
                SetWindowPos(dat->hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                ZeroMemory((void *)&dat->ti, sizeof(dat->ti));
                dat->ti.cbSize = sizeof(dat->ti);
                dat->ti.lpszText = Translate("No status message available");
                dat->ti.hinst = g_hInst;
                dat->ti.hwnd = hwndDlg;
                dat->ti.uFlags = TTF_TRACK | TTF_IDISHWND;
                dat->ti.uId = (UINT_PTR)hwndDlg;
                SendMessageA(dat->hwndTip, TTM_ADDTOOLA, 0, (LPARAM)&dat->ti);
                
                dat->dwEventIsShown = GetInfoPanelSetting(hwndDlg, dat) ? dat->dwEventIsShown | MWF_SHOW_INFOPANEL : dat->dwEventIsShown & ~MWF_SHOW_INFOPANEL;
                dat->dwEventIsShown |= DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "splitoverride", 0) ? MWF_SHOW_SPLITTEROVERRIDE : 0;
                SetMessageLog(hwndDlg, dat);

                if(dat->hContact) {
                    dat->codePage = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "ANSIcodepage", CP_ACP);
                    dat->dwFlags |= (myGlobals.m_RTLDefault == 0 ? (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 0) ? MWF_LOG_RTL : 0) : (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 1) ? MWF_LOG_RTL : 0));
                    LoadPanelHeight(hwndDlg, dat);
                }

                dat->iAvatarDisplayMode = myGlobals.m_AvatarDisplayMode;
                dat->showPic = GetAvatarVisibility(hwndDlg, dat);

                GetWindowRect(GetDlgItem(hwndDlg, IDC_SMILEYBTN), &rc);

                ShowWindow(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_CLIST), SW_HIDE);

                GetWindowRect(GetDlgItem(hwndDlg, IDC_SPLITTER), &rc);
                pt.y = (rc.top + rc.bottom) / 2;
                pt.x = 0;
                ScreenToClient(hwndDlg, &pt);
                dat->originalSplitterY = pt.y;
                if(dat->pContainer->dwFlags & CNT_CREATE_MINIMIZED || (IsIconic(dat->pContainer->hwnd) && dat->pContainer->bInTray == 0))
                    dat->originalSplitterY--;
                if (dat->splitterY == -1)
                    dat->splitterY = dat->originalSplitterY + 60;

                GetWindowRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc);
                dat->minEditBoxSize.cx = rc.right - rc.left;
                dat->minEditBoxSize.cy = rc.bottom - rc.top;

                SetDlgItemTextA(hwndDlg, IDC_ADD, Translate("Add it"));
                SetDlgItemTextA(hwndDlg, IDC_CANCELADD, Translate("Leave it"));

                SendMessage(hwndDlg, DM_LOADBUTTONBARICONS, 0, 0);

                SendDlgItemMessage(hwndDlg, IDC_FONTBOLD, BUTTONSETASPUSHBTN, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_FONTITALIC, BUTTONSETASPUSHBTN, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_FONTUNDERLINE, BUTTONSETASPUSHBTN, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_APPARENTMODE, BUTTONSETASPUSHBTN, 0, 0);
                
                for (i = 0; i < sizeof(buttonLineControlsNew) / sizeof(buttonLineControlsNew[0]); i++)
                    SendMessage(GetDlgItem(hwndDlg, buttonLineControlsNew[i]), BUTTONSETASFLATBTN, 0, 0);
                //for (i = 0; i < sizeof(errorControls) / sizeof(errorControls[0]); i++)
                    //SendMessage(GetDlgItem(hwndDlg, errorControls[i]), BUTTONSETASFLATBTN, 0, 0);
                for (i = 0; i < sizeof(formatControls) / sizeof(formatControls[0]); i++)
                    SendMessage(GetDlgItem(hwndDlg, formatControls[i]), BUTTONSETASFLATBTN, 0, 0);
                for (i = 0; i < sizeof(infoLineControls) / sizeof(infoLineControls[0]); i++)
                    SendMessage(GetDlgItem(hwndDlg, infoLineControls[i]), BUTTONSETASFLATBTN, 0, 0);

                SendMessage(GetDlgItem(hwndDlg, IDOK), BUTTONSETASFLATBTN, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_TOGGLENOTES), BUTTONSETASFLATBTN, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_ADD), BUTTONSETASFLATBTN, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_CANCELADD), BUTTONSETASFLATBTN, 0, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_LOGFROZEN), BUTTONSETASFLATBTN, 0, 0);

                SendDlgItemMessage(hwndDlg, IDC_APPARENTMODE, BUTTONSETASFLATBTN, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_APPARENTMODE, BUTTONSETASFLATBTN + 10, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_TOGGLENOTES, BUTTONSETASFLATBTN + 10, 0, 0);
                
                SendDlgItemMessage(hwndDlg, IDC_RETRY, BUTTONSETASFLATBTN + 10, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_CANCELSEND, BUTTONSETASFLATBTN + 10, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_MSGSENDLATER, BUTTONSETASFLATBTN + 10, 0, 0);

                dat->dwFlags |= MWF_INITMODE;
                TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_OPENING, 0);

                for(i = 0;;i++) {
                    if(tooltips[i].id == -1)
                        break;
                    SendDlgItemMessage(hwndDlg, tooltips[i].id, BUTTONADDTOOLTIP, (WPARAM)Translate(tooltips[i].szTip), 0);
                }
                
                SetDlgItemTextA(hwndDlg, IDC_LOGFROZENTEXT, Translate("Message Log is frozen"));
                
                SendMessage(GetDlgItem(hwndDlg, IDC_SAVE), BUTTONADDTOOLTIP, (WPARAM) pszIDCSAVE_close, 0);
                if(dat->bIsMeta)
                    SendMessage(GetDlgItem(hwndDlg, IDC_PROTOCOL), BUTTONADDTOOLTIP, (WPARAM) Translate("View User's Details (Right click for MetaContact control)"), 0);
                else
                    SendMessage(GetDlgItem(hwndDlg, IDC_PROTOCOL), BUTTONADDTOOLTIP, (WPARAM) Translate("View User's Details"), 0);
                
                SetWindowTextA(GetDlgItem(hwndDlg, IDC_RETRY), Translate("Retry"));
                SetWindowTextA(GetDlgItem(hwndDlg, IDC_CANCELSEND), Translate("Cancel"));
                SetWindowTextA(GetDlgItem(hwndDlg, IDC_MSGSENDLATER), Translate("Send later"));

                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETOLECALLBACK, 0, (LPARAM) & reOleCallback);
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETUNDOLIMIT, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_KEYEVENTS | ENM_LINK);
                SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_SCROLL | ENM_KEYEVENTS | ENM_CHANGE);
                
                /* OnO: higligh lines to their end */
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETEDITSTYLE, SES_EXTENDBACKCOLOR, SES_EXTENDBACKCOLOR);
                SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETLANGOPTIONS, 0, (LPARAM) SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETLANGOPTIONS, 0, 0) & ~IMF_AUTOKEYBOARD);

                /*
                 * add us to the tray list (if it exists)
                 */

                if(myGlobals.g_hMenuTrayUnread != 0 && dat->hContact != 0 && dat->szProto != NULL)
                    UpdateTrayMenu(0, dat->wStatus, dat->szProto, dat->szStatus, dat->hContact, FALSE);
                    
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_AUTOURLDETECT, (WPARAM) TRUE, 0);
                if (dat->hContact) {
                    int  pCaps;
                    HANDLE hItem;
                    
                    hItem = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM) dat->hContact, 0);
                    if (hItem)
                        SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM) hItem, 1);
                    
                    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXLIMITTEXT, 0, 0x80000000);
                    if (dat->szProto) {
                        int nMax;
                        nMax = CallProtoService(dat->szProto, PS_GETCAPS, PFLAG_MAXLENOFMESSAGE, (LPARAM) dat->hContact);
                        if (nMax)
                            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_EXLIMITTEXT, 0, (LPARAM)nMax);
                        else
                            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_EXLIMITTEXT, 0, (LPARAM)7500);
                        pCaps = CallProtoService(dat->szProto, PS_GETCAPS, PFLAGNUM_4, 0);
                        if(pCaps & PF4_AVATARS)
                            SendMessage(hwndDlg, DM_RETRIEVEAVATAR, 0, 0);
                    }
                }
                OldMessageEditProc = (WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_WNDPROC, (LONG) MessageEditSubclassProc);
                OldAvatarWndProc = (WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, IDC_CONTACTPIC), GWL_WNDPROC, (LONG) AvatarSubclassProc);
                SetWindowLong(GetDlgItem(hwndDlg, IDC_PANELPIC), GWL_WNDPROC, (LONG) AvatarSubclassProc);
                
                if (CallService(MS_CLUI_GETCAPS, 0, 0) & CLUIF_DISABLEGROUPS && !DBGetContactSettingByte(NULL, "CList", "UseGroups", SETTING_USEGROUPS_DEFAULT))
                    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETUSEGROUPS, (WPARAM) FALSE, 0);
                else
                    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETUSEGROUPS, (WPARAM) TRUE, 0);
                if (CallService(MS_CLUI_GETCAPS, 0, 0) & CLUIF_HIDEEMPTYGROUPS && DBGetContactSettingByte(NULL, "CList", "HideEmptyGroups", SETTING_USEGROUPS_DEFAULT))
                    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETHIDEEMPTYGROUPS, (WPARAM) TRUE, 0);
                else
                    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETHIDEEMPTYGROUPS, (WPARAM) FALSE, 0);
                OldSplitterProc = (WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_WNDPROC, (LONG) SplitterSubclassProc);
                SetWindowLong(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), GWL_WNDPROC, (LONG) SplitterSubclassProc);
                SetWindowLong(GetDlgItem(hwndDlg, IDC_PANELSPLITTER), GWL_WNDPROC, (LONG) SplitterSubclassProc);
                
                if (dat->hContact)
                    FindFirstEvent(hwndDlg, dat);
                dat->stats.started = time(NULL);
                SendMessage(hwndDlg, DM_OPTIONSAPPLIED, 0, 0);
                LoadOwnAvatar(hwndDlg, dat);
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
                                SendMessage(hwndDlg, DM_UPDATELASTMESSAGE, 0, 0);
                                break;
                            }
                        }
                        while (hdbEvent = (HANDLE) CallService(MS_DB_EVENT_FINDPREV, (WPARAM) hdbEvent, 0));
                    }

                }
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
                        if(dat->pContainer->hwndActive == hwndDlg)
                            UpdateReadChars(hwndDlg, dat);
                        UpdateSaveAndSendButton(hwndDlg, dat);
    				}
                    if(!DBGetContactSetting(dat->hContact, "UserInfo", "MyNotes", &dbv)) {
                        SetDlgItemTextA(hwndDlg, IDC_NOTES, dbv.pszVal);
                        DBFreeVariant(&dbv);
                    }
    			}
                if (newData->szInitialText) {
                    int len;
                    SetDlgItemTextA(hwndDlg, IDC_MESSAGE, newData->szInitialText);
                    len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE));
                    PostMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_SETSEL, len, len);
                    if(len)
                        EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);
                }
                SendMessage(dat->pContainer->hwnd, DM_QUERYCLIENTAREA, 0, (LPARAM)&rc);
                if (newData->iActivate) {
                    SetWindowPos(hwndDlg, HWND_TOP, rc.left, rc.top, (rc.right - rc.left), (rc.bottom - rc.top), 0);
                    LoadSplitter(hwndDlg, dat);
                    ShowPicture(hwndDlg,dat,FALSE,TRUE);
                    //if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL && myGlobals.m_AvatarDisplayMode == AVATARMODE_DYNAMIC)
                        //AdjustBottomAvatarDisplay(hwndDlg, dat);
                    SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
                    PostMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                    if(IsIconic(dat->pContainer->hwnd) && !IsZoomed(dat->pContainer->hwnd) && myGlobals.m_AutoSwitchTabs) {
                        DBEVENTINFO dbei = {0};

                        dbei.flags = 0;
                        dbei.eventType = EVENTTYPE_MESSAGE;
                        dat->iFlashIcon = myGlobals.g_IconMsgEvent;
                        SetTimer(hwndDlg, TIMERID_FLASHWND, TIMEOUT_FLASHWND, NULL);
                        dat->mayFlashTab = TRUE;
                        dat->dwTickLastEvent = GetTickCount();
                        //if(dat->pContainer->dwFlags & CNT_CREATE_MINIMIZED)
                        FlashOnClist(hwndDlg, dat, dat->hDbEventFirst, &dbei);
                        PostMessage(dat->pContainer->hwnd, DM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_EVENT_MESSAGE));
                    }
                    ShowWindow(hwndDlg, SW_SHOW);
                    dat->pContainer->hwndActive = hwndDlg;
                    SetActiveWindow(hwndDlg);
                    SetForegroundWindow(hwndDlg);
                    SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                }
                else {
                    DBEVENTINFO dbei = {0};

                    dbei.flags = 0;
                    dbei.eventType = EVENTTYPE_MESSAGE;
                    dat->dwFlags |= (MWF_WASBACKGROUNDCREATE | MWF_NEEDCHECKSIZE);
                    dat->iFlashIcon = myGlobals.g_IconMsgEvent;
                    SetTimer(hwndDlg, TIMERID_FLASHWND, TIMEOUT_FLASHWND, NULL);
                    dat->mayFlashTab = TRUE;
                    dat->dwTickLastEvent = GetTickCount();
                    dat->pContainer->dwFlags |= CNT_NEED_UPDATETITLE;
                    SendMessage(dat->pContainer->hwnd, DM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_EVENT_MESSAGE));
                    FlashOnClist(hwndDlg, dat, dat->hDbEventFirst, &dbei);
                    if(GetForegroundWindow() != dat->pContainer->hwnd || GetActiveWindow() != dat->pContainer->hwnd)
                        FlashContainer(dat->pContainer, 1, 0);
                }
                SendMessage(hwndDlg, DM_CALCMINHEIGHT, 0, 0);
                SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
                SendMessage(dat->pContainer->hwnd, DM_REPORTMINHEIGHT, (WPARAM) hwndDlg, (LPARAM) dat->uMinHeight);
                dat->dwLastActivity = GetTickCount() - 1000;
                dat->pContainer->dwLastActivity = dat->dwLastActivity;
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
                if(dat->pContainer->dwFlags & CNT_CREATE_MINIMIZED) {
                    dat->pContainer->dwFlags &= ~CNT_CREATE_MINIMIZED;
                    dat->pContainer->dwFlags |= CNT_DEFERREDCONFIGURE;
                    dat->pContainer->hwndActive = hwndDlg;
                    SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                    return FALSE;
                }
                return TRUE;
            }
        case DM_TYPING:
            {
                dat->nTypeSecs = (int) lParam > 0 ? (int) lParam : 0;
                break;
            }
        case DM_UPDATEWINICON:
            {
                HWND t_hwnd;
                char *szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
                WORD wStatus = dat->bIsMeta ? dat->wMetaStatus : dat->wStatus;
                t_hwnd = dat->pContainer->hwnd;
                
                if (szProto) {
                    dat->hTabIcon = dat->hTabStatusIcon = LoadSkinnedProtoIcon(szProto, wStatus);
                    SendDlgItemMessage(hwndDlg, IDC_PROTOCOL, BUTTONSETASFLATBTN + 11, 0, dat->dwEventIsShown & MWF_SHOW_ISIDLE ? 1 : 0);
                    SendDlgItemMessage(hwndDlg, IDC_PROTOCOL, BM_SETIMAGE, IMAGE_ICON, (LPARAM) dat->hTabIcon);

                    if (dat->pContainer->hwndActive == hwndDlg)
                        SendMessage(t_hwnd, DM_SETICON, (WPARAM) ICON_BIG, (LPARAM) dat->hTabIcon);
                }
                break;
            }
        case DM_UPDATELASTMESSAGE:
            {
                if (dat->pContainer->hwndStatus == 0)
                    break;
                if (dat->pContainer->hwndActive != hwndDlg)
                    break;
                if(dat->showTyping) {
                    char szBuf[80];

                    mir_snprintf(szBuf, sizeof(szBuf), Translate("%s is typing..."), dat->szNickname);
                    SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) szBuf);
                    SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 0, (LPARAM) myGlobals.g_buttonBarIcons[5]);
                    break;
                }
                if (dat->lastMessage || dat->pContainer->dwFlags & CNT_UINSTATUSBAR) {
                    DBTIMETOSTRING dbtts;
                    char date[64], time[64], fmt[128];

                    if(!(dat->pContainer->dwFlags & CNT_UINSTATUSBAR)) {
                        dbtts.szFormat = "d";
                        dbtts.cbDest = sizeof(date);
                        dbtts.szDest = date;
                        CallService(MS_DB_TIME_TIMESTAMPTOSTRING, dat->lastMessage, (LPARAM) & dbtts);
                        if(dat->pContainer->dwFlags & CNT_UINSTATUSBAR && lstrlenA(date) > 6)
                            date[lstrlenA(date) - 5] = 0;
                        dbtts.szFormat = "t";
                        dbtts.cbDest = sizeof(time);
                        dbtts.szDest = time;
                        CallService(MS_DB_TIME_TIMESTAMPTOSTRING, dat->lastMessage, (LPARAM) & dbtts);
                    }
                    if(dat->pContainer->dwFlags & CNT_UINSTATUSBAR)
                        mir_snprintf(fmt, sizeof(fmt), Translate("UIN: %s"), dat->uin);
                    else
                        mir_snprintf(fmt, sizeof(fmt), Translate("Last received: %s at %s"), date, time);
                    SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) fmt);
                    SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 0, (LPARAM)(nen_options.bFloaterInWin ? myGlobals.g_buttonBarIcons[16] : 0));
                    if(dat->pContainer->hwndSlist)
                        SendMessage(dat->pContainer->hwndSlist, BM_SETIMAGE, IMAGE_ICON, (LPARAM)myGlobals.g_buttonBarIcons[16]);
                } else {
                    SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) "");
                    SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 0, (LPARAM)(nen_options.bFloaterInWin ? myGlobals.g_buttonBarIcons[16] : 0));
                    if(dat->pContainer->hwndSlist)
                        SendMessage(dat->pContainer->hwndSlist, BM_SETIMAGE, IMAGE_ICON, (LPARAM)myGlobals.g_buttonBarIcons[16]);
                }
                break;
            }
            /*
             * configures the toolbar only... if lParam != 0, then it also calls
             * SetDialogToType() to reconfigure the message window
             */
            
        case DM_CONFIGURETOOLBAR:
            dat->showUIElements = dat->pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;
            if(lParam == 1) {
                GetSendFormat(hwndDlg, dat, 1);
                SetDialogToType(hwndDlg);
            }
            dat->iButtonBarNeeds = dat->showUIElements ? (myGlobals.m_AllowSendButtonHidden ? 0 : 40) : 0;
            dat->iButtonBarNeeds += (dat->showUIElements ? (dat->doSmileys ? 202 : 180) : 0);
            
            dat->iButtonBarNeeds += (dat->showUIElements) ? 28 : 0;
            dat->iButtonBarReallyNeeds = dat->iButtonBarNeeds + (dat->showUIElements ? (myGlobals.m_AllowSendButtonHidden ? 110 : 70) : 0);
            if(!dat->SendFormat)
                dat->iButtonBarReallyNeeds -= 88;
            if(lParam == 1) {
                SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
                SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
                SendMessage(hwndDlg, WM_SIZE, 0, 0);
                SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 1);
            }

            /*
             * message area background image...
             */
            
            if(dat->hContact) {
                if(DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "private_bg", 0)) {
                    DBVARIANT dbv;
                    if(DBGetContactSetting(dat->hContact, SRMSGMOD_T, "bgimage", &dbv) == 0) {
                        HBITMAP hbm = (HBITMAP)CallService(MS_UTILS_LOADBITMAP, 0, (LPARAM)dbv.pszVal);
                        if(dat->hbmMsgArea != 0 && dat->hbmMsgArea != myGlobals.m_hbmMsgArea)
                            DeleteObject(dat->hbmMsgArea);
                        if(hbm != 0)
                            dat->hbmMsgArea = hbm;
                        else
                            dat->hbmMsgArea = myGlobals.m_hbmMsgArea;
                        DBFreeVariant(&dbv);
                    }
                }
                else {
                    if(dat->hbmMsgArea != 0 && dat->hbmMsgArea != myGlobals.m_hbmMsgArea)
                        DeleteObject(dat->hbmMsgArea);
                    dat->hbmMsgArea = myGlobals.m_hbmMsgArea;
                }
                
                if(dat->hbmMsgArea)
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_EXSTYLE) | WS_EX_TRANSPARENT);
                else
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
            }
            break;
        case DM_LOADBUTTONBARICONS:
        {
            int i;

            for(i = 0;;i++) {
                if(buttonicons[i].id == -1)
                    break;
                SendDlgItemMessage(hwndDlg, buttonicons[i].id, BM_SETIMAGE, IMAGE_ICON, (LPARAM)*buttonicons[i].pIcon);
            }
            SendDlgItemMessage(hwndDlg, IDC_MULTIPLEICON, STM_SETICON, (WPARAM)myGlobals.g_buttonBarIcons[3], 0);
            SendDlgItemMessage(hwndDlg, IDC_STATICERRORICON, STM_SETICON, (WPARAM)myGlobals.g_iconErr, 0);

            break;
        }
        case DM_OPTIONSAPPLIED:
            if (wParam == 1) {      // 1 means, the message came from message log options page, so reload the defaults...
                if(DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "mwoverride", 0) == 0) {
                    dat->dwFlags &= ~(MWF_LOG_ALL);
                    dat->dwFlags |= DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);
                }
            }

            if(dat->hContact) {
                dat->dwIsFavoritOrRecent = MAKELONG((WORD)DBGetContactSettingWord(dat->hContact, SRMSGMOD_T, "isFavorite", 0), (WORD)DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "isRecent", 0));
                LoadTimeZone(hwndDlg, dat);
            }

            if(dat->hContact && dat->szProto != NULL && dat->bIsMeta) {
                DWORD dwForcedContactNum = 0;
                CallService(MS_MC_GETFORCESTATE, (WPARAM)dat->hContact, (LPARAM)&dwForcedContactNum);
                DBWriteContactSettingDword(dat->hContact, "MetaContacts", "tabSRMM_forced", dwForcedContactNum);
            }

            dat->showUIElements = dat->pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;
            
            dat->dwEventIsShown = DBGetContactSettingByte(NULL, SRMSGMOD_T, SRMSGSET_SHOWURLS, SRMSGDEFSET_SHOWURLS) ? MWF_SHOW_URLEVENTS : 0;
            dat->dwEventIsShown |= DBGetContactSettingByte(NULL, SRMSGMOD_T, SRMSGSET_SHOWFILES, SRMSGDEFSET_SHOWFILES) ? MWF_SHOW_FILEEVENTS : 0;
            dat->dwEventIsShown |= DBGetContactSettingByte(NULL, SRMSGMOD_T, "emptylinefix", 1) ? MWF_SHOW_EMPTYLINEFIX : 0;
            dat->dwEventIsShown |= MWF_SHOW_MICROLF;
            dat->dwEventIsShown |= DBGetContactSettingByte(NULL, SRMSGMOD_T, "followupts", 1) ? MWF_SHOW_MARKFOLLOWUPTS : 0;
            dat->dwEventIsShown |= DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "splitoverride", 0) ? MWF_SHOW_SPLITTEROVERRIDE : 0;
            dat->dwEventIsShown |= DBGetContactSettingByte(NULL, SRMSGMOD_T, "log_bbcode", 1) ? MWF_SHOW_BBCODE : 0;
            dat->dwEventIsShown |= DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "uselocaltime", 0) ? MWF_SHOW_USELOCALTIME : 0;
            dat->dwEventIsShown = GetInfoPanelSetting(hwndDlg, dat) ? dat->dwEventIsShown | MWF_SHOW_INFOPANEL : dat->dwEventIsShown & ~MWF_SHOW_INFOPANEL;

            dat->iAvatarDisplayMode = myGlobals.m_AvatarDisplayMode;

            if(dat->dwFlags & MWF_LOG_GRID && DBGetContactSettingByte(NULL, SRMSGMOD_T, "wantvgrid", 0))
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,1));     // XXX margins in the log (looks slightly better)
            else
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0,0));     // XXX margins in the log (looks slightly better)
                
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
                COLORREF inputcolour = DBGetContactSettingDword(NULL, FONTMODULE, "inputbg", SRMSGDEFSET_BKGCOLOUR);
                COLORREF inputcharcolor;
                CHARFORMAT2A cf2 = {0};
                LOGFONTA lf;
                int i = 0;
                HDC hdc = GetDC(NULL);
                int logPixelSY = GetDeviceCaps(hdc, LOGPIXELSY);
                ReleaseDC(NULL, hdc);

                if(GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MESSAGE)) > 0)
                    szStreamOut = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, (CP_UTF8 << 16) | (SF_RTFNOOBJS|SFF_PLAINRTF|SF_USECODEPAGE));

                dat->hBkgBrush = CreateSolidBrush(colour);
                dat->hInputBkgBrush = CreateSolidBrush(inputcolour);
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETBKGNDCOLOR, 0, colour);
                SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETBKGNDCOLOR, 0, inputcolour);
                lf = logfonts[MSGFONTID_MESSAGEAREA];
                inputcharcolor = fontcolors[MSGFONTID_MESSAGEAREA];
                /*
                 * correct the input area text color to avoid a color from the table of usable bbcode colors
                 */
                while(rtf_ctable[i].szName != NULL) {
                    if(rtf_ctable[i].clr == inputcharcolor)
                        inputcharcolor = RGB(GetRValue(inputcharcolor), GetGValue(inputcharcolor), GetBValue(inputcharcolor) == 0 ? GetBValue(inputcharcolor) + 1 : GetBValue(inputcharcolor) - 1);
                    i++;
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
                
                if (dat->dwFlags & MWF_LOG_RTL) {
                    PARAFORMAT2 pf2;
                    ZeroMemory((void *)&pf2, sizeof(pf2));
                    pf2.cbSize = sizeof(pf2);
                    pf2.dwMask = PFM_RTLPARA;
                    pf2.wEffects = PFE_RTLPARA;
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE) | WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE) | WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
                    SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
                    SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
                } else {
                    PARAFORMAT2 pf2;
                    ZeroMemory((void *)&pf2, sizeof(pf2));
                    pf2.cbSize = sizeof(pf2);
                    pf2.dwMask = PFM_RTLPARA;
                    pf2.wEffects = 0;
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE) &~ (WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR));
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE) &~ (WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR));
                    SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
                    SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
    
                }
                
                if(szStreamOut != NULL) {
                    SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)szStreamOut);
                    free(szStreamOut);
                }
            }
// END MOD#23
            if (hwndDlg == dat->pContainer->hwndActive)
                SendMessage(dat->pContainer->hwnd, WM_SIZE, 0, 0);
            InvalidateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, FALSE);
            if (!lParam)
                SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
            
            if(dat->pContainer->hwndStatus && dat->hContact)
                SetSelftypingIcon(hwndDlg, dat, DBGetContactSettingByte(dat->hContact, SRMSGMOD, SRMSGSET_TYPING, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW)));
            
            SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
            SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
            break;
        case DM_UPDATETITLE:
            {
                char newtitle[128], *szProto = 0, *szOldMetaProto = 0;
                char *pszNewTitleEnd, newcontactname[128], *temp;
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
                dat->szStatus = NULL;
                
                pszNewTitleEnd = "Message Session";
                
                if(dat->iTabID == -1)
                    break;
                ZeroMemory((void *)&item, sizeof(item));
                if (dat->hContact) {
                    int iHasName;
                    char fulluin[128];
                    if (dat->szProto) {

                        if(dat->bIsMeta) {
                            szOldMetaProto = dat->szMetaProto;
                            szProto = GetCurrentMetaContactProto(hwndDlg, dat);
                            GetContactUIN(hwndDlg, dat);
                        }
                        szActProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
                        hActContact = dat->bIsMeta ? dat->hSubContact : dat->hContact;
                        
                        mir_snprintf(dat->szNickname, 80, "%s", (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hActContact, 0));
                        iHasName = (int)dat->uin[0];        // dat->uin[0] == 0 if there is no valid UIN
                        dat->idle = DBGetContactSettingDword(dat->hContact, szActProto, "IdleTS", 0);
                        dat->dwEventIsShown =  dat->idle ? dat->dwEventIsShown | MWF_SHOW_ISIDLE : dat->dwEventIsShown & ~MWF_SHOW_ISIDLE;
                        dat->xStatus = DBGetContactSettingByte(hActContact, szActProto, "XStatusId", 0);
                        
                    /*
                     * cut nickname on tabs...
                     */
                        temp = dat->szNickname;
                        while(*temp)
                            iHash += (*(temp++) * (int)(temp - dat->szNickname + 1));

                        dat->wStatus = DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE);
                        dat->szStatus = (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, dat->szProto == NULL ? ID_STATUS_OFFLINE : dat->wStatus, 0);
                        wOldApparentMode = dat->wApparentMode;
                        dat->wApparentMode = DBGetContactSettingWord(hActContact, szActProto, "ApparentMode", 0);
                        
                        if (iHash != dat->iOldHash || dat->wStatus != dat->wOldStatus || lParam != 0) {
                            if (myGlobals.m_CutContactNameOnTabs)
                                CutContactName(dat->szNickname, newcontactname, sizeof(newcontactname));
                            else
                                strncpy(newcontactname, dat->szNickname, sizeof(newcontactname));

                            if (strlen(newcontactname) != 0 && dat->szStatus != NULL) {
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
                        mir_snprintf(fulluin, sizeof(fulluin), Translate("UIN: %s (SHIFT click copies it to the clipboard)"), iHasName ? dat->uin : Translate("No UIN"));
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
                                    mir_snprintf(buffer, sizeof(buffer), Translate("signed off (was %s)"), szOldStatus);
                                else if(dat->wOldStatus == ID_STATUS_OFFLINE)
                                    mir_snprintf(buffer, sizeof(buffer), Translate("signed on (%s)"), szNewStatus);
                                else
                                    mir_snprintf(buffer, sizeof(buffer), Translate("is now %s (was %s)"), szNewStatus, szOldStatus);
                            }
                            iLen = strlen(buffer) + 1;
                            MultiByteToWideChar(CP_ACP, 0, buffer, iLen, (LPWSTR)&buffer[iLen], iLen);
                            dbei.cbSize = sizeof(dbei);
                            dbei.pBlob = (PBYTE) buffer;
                            dbei.cbBlob = (strlen(buffer) + 1) * (sizeof(TCHAR) + 1);
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
                        const wchar_t *newTitle = EncodeWithNickname(newtitle, newcontactname, dat->codePage);
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
                    if (dat->pContainer->hwndActive == hwndDlg && (dat->iOldHash != iHash || dat->wOldStatus != dat->wStatus))
                        SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
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

                    if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELSTATUS), NULL, FALSE);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELNICK), NULL, FALSE);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELUIN), NULL, FALSE);
                        UpdateApparentModeDisplay(hwndDlg, dat);
                    }
                    dat->lastRetrievedStatusMsg = 0;
                }
                // care about MetaContacts and update the statusbar icon with the currently "most online" contact...
                if(dat->bIsMeta) {
                    PostMessage(hwndDlg, DM_UPDATEMETACONTACTINFO, 0, 0);
                    if(dat->pContainer->dwFlags & CNT_UINSTATUSBAR)
                        PostMessage(hwndDlg, DM_UPDATELASTMESSAGE, 0, 0);
                }
                break;
            }
        case DM_ADDDIVIDER:
            if(!(dat->dwFlags & MWF_DIVIDERSET) && myGlobals.m_UseDividers) {
                if(GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_LOG)) > 0) {
                    dat->dwFlags |= MWF_DIVIDERWANTED;
                    dat->dwFlags |= MWF_DIVIDERSET;
                }
            }
            break;
        case WM_SETFOCUS:
            if(GetTickCount() - dat->dwLastUpdate < (DWORD)200) {
                SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                UpdateStatusBar(hwndDlg, dat);
                return 1;
            }
            if (dat->iTabID >= 0) {
                ConfigureSideBar(hwndDlg, dat);
                SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
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
                if(dat->dwEventIsShown & MWF_SHOW_FLASHCLIST) {
                    dat->dwEventIsShown &= ~MWF_SHOW_FLASHCLIST;
                    if(dat->hFlashingEvent !=0)
                        CallService(MS_CLIST_REMOVEEVENT, (WPARAM)dat->hContact, (LPARAM)dat->hFlashingEvent);
                    dat->hFlashingEvent = 0;
                }
                if (dat->dwFlags & MWF_DEFERREDREMAKELOG) {
                    SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                    dat->dwFlags &= ~MWF_DEFERREDREMAKELOG;
                }
                
                if(dat->dwFlags & MWF_NEEDCHECKSIZE)
                    PostMessage(hwndDlg, DM_SAVESIZE, 0, 0);

                if(dat->dwFlags & MWF_DEFERREDSCROLL)
                    SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 0);

                if (myGlobals.m_AutoLocaleSupport && dat->hContact != 0) {
                    if(dat->hkl == 0)
                        SendMessage(hwndDlg, DM_LOADLOCALE, 0, 0);
                    PostMessage(hwndDlg, DM_SETLOCALE, 0, 0);
                }
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
                    DeletePopupsForContact(dat->hContact, PU_REMOVE_ON_FOCUS);
                if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELUIN), NULL, FALSE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_READSTATUS), NULL, FALSE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELSTATUS), NULL, FALSE);
                }
            }
            return 1;
        case WM_ACTIVATE:
            if (LOWORD(wParam) != WA_ACTIVE)
                break;
            //fall through
        case WM_MOUSEACTIVATE:
            if((GetTickCount() - dat->dwLastUpdate) < (DWORD)200) {
                if (dat->pContainer->dwFlags & CNT_TRANSPARENCY && pSetLayeredWindowAttributes != NULL)
                    pSetLayeredWindowAttributes(dat->pContainer->hwnd, RGB(255,255,255), (BYTE)LOWORD(dat->pContainer->dwTransparency), LWA_ALPHA);
                //SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                UpdateStatusBar(hwndDlg, dat);
                break;
            }
            if (dat->iTabID == -1) {
                _DebugPopup(dat->hContact, "ACTIVATE Critical: iTabID == -1");
                break;
            } else {
                if (dat->pContainer->dwFlags & CNT_TRANSPARENCY && pSetLayeredWindowAttributes != NULL)
                    pSetLayeredWindowAttributes(dat->pContainer->hwnd, RGB(255,255,255), (BYTE)LOWORD(dat->pContainer->dwTransparency), LWA_ALPHA);
                ConfigureSideBar(hwndDlg, dat);
                dat->dwFlags &= ~MWF_DIVIDERSET;
                dat->dwTickLastEvent = 0;
                if (KillTimer(hwndDlg, TIMERID_FLASHWND)) {
                    FlashTab(dat, hwndTab, dat->iTabID, &dat->bTabFlash, FALSE, dat->hTabIcon);
                    dat->mayFlashTab = FALSE;
                }
                if(dat->pContainer->dwFlashingStarted != 0) {
                    FlashContainer(dat->pContainer, 0, 0);
                    dat->pContainer->dwFlashingStarted = 0;
                }
                if(dat->dwEventIsShown & MWF_SHOW_FLASHCLIST) {
                    dat->dwEventIsShown &= ~MWF_SHOW_FLASHCLIST;
                    if(dat->hFlashingEvent !=0)
                        CallService(MS_CLIST_REMOVEEVENT, (WPARAM)dat->hContact, (LPARAM)dat->hFlashingEvent);
                    dat->hFlashingEvent = 0;
                }
                if (dat->dwFlags & MWF_DEFERREDREMAKELOG) {
                    SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                    dat->dwFlags &= ~MWF_DEFERREDREMAKELOG;
                }
                if(dat->dwFlags & MWF_NEEDCHECKSIZE)
                    PostMessage(hwndDlg, DM_SAVESIZE, 0, 0);

                if(dat->dwFlags & MWF_DEFERREDSCROLL)
                    SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 0);

                if (myGlobals.m_AutoLocaleSupport && dat->hContact != 0) {
                    if(dat->hkl == 0)
                        SendMessage(hwndDlg, DM_LOADLOCALE, 0, 0);
                    PostMessage(hwndDlg, DM_SETLOCALE, 0, 0);
                }
                //SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                UpdateStatusBar(hwndDlg, dat);
                dat->dwLastActivity = GetTickCount();
                dat->pContainer->dwLastActivity = dat->dwLastActivity;
                UpdateContainerMenu(hwndDlg, dat);
                /*
                 * delete ourself from the tray menu..
                 */
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
                    DeletePopupsForContact(dat->hContact, PU_REMOVE_ON_FOCUS);
                if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELUIN), NULL, FALSE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_READSTATUS), NULL, FALSE);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELSTATUS), NULL, FALSE);
                }
            }
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
                RECT rc;
                int saved = 0, i;
                int delta;
                HBITMAP hbm = dat->dwEventIsShown & MWF_SHOW_INFOPANEL ? dat->hOwnPic : dat->hContactPic;
                    
                if (IsIconic(hwndDlg))
                    break;
                ZeroMemory(&urd, sizeof(urd));
                urd.cbSize = sizeof(urd);
                urd.hInstance = g_hInst;
                urd.hwndDlg = hwndDlg;
                urd.lParam = (LPARAM) dat;
                urd.lpTemplate = MAKEINTRESOURCEA(IDD_MSGSPLITNEW);
                urd.pfnResizer = MessageDialogResize;

                if (dat->uMinHeight > 0 && HIWORD(lParam) >= dat->uMinHeight) {
                    if (dat->splitterY > HIWORD(lParam) - MINLOGHEIGHT) {
                        dat->splitterY = HIWORD(lParam) - MINLOGHEIGHT;
                        dat->dynaSplitter = dat->splitterY - 34;
                        SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
                        SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
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
                if(dat->showPic) {
                    if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC) {
                        if(dat->splitterY <= (dat->bottomOffset + 32))
                            buttonBarSpace = rc.right - (dat->pic.cx + 2);
                    }
                    else if(dat->iAvatarDisplayMode == AVATARMODE_STATIC) {
                        if(!myGlobals.m_AlwaysFullToolbarWidth && dat->splitterY <= (dat->bottomOffset + 25))
                            buttonBarSpace = rc.right - (dat->pic.cx + 2);
                    }
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
                                saved += 10;
                                ShowWindow(GetDlgItem(hwndDlg, IDC_INFOPANELMENU), SW_HIDE);
                            }
                            if(hideThisControls[i] == IDOK)
                                dat->controlsHidden |= TOOLBAR_SEND_HIDDEN;
                            saved += (hideThisControls[i] == IDOK ? 40 : 21);
                        }
                        else {
                            if(!IsWindowVisible(GetDlgItem(hwndDlg, hideThisControls[i]))) {
                                ShowWindow(GetDlgItem(hwndDlg, hideThisControls[i]), SW_SHOW);
                                if(hideThisControls[i] == IDC_PROTOCOL)
                                    ShowWindow(GetDlgItem(hwndDlg, IDC_INFOPANELMENU), SW_SHOW);
                            }
                        }
                    }
                }
                
                CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM) & urd);
                if (dat->hwndLog != 0)
                    ResizeIeView(hwndDlg, dat, 0, 0, 0, 0);
                if(dat->hbmMsgArea)
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, TRUE);
                break;
            }
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
                    if (dat->splitterY <= MINSPLITTERY)           // min splitter size
                        dat->splitterY = oldSplitterY;
                    else if (dat->splitterY > ((rc.bottom - rc.top) - 50)) 
                        dat->splitterY = oldSplitterY;
                    else {
                        if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC) {
                            dat->dynaSplitter = (rc.bottom - pt.y) - 11;
                            SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
                        }
                        else {
                            dat->dynaSplitter = (rc.bottom - pt.y) - 9;
                            if(dat->splitterY <= dat->bottomOffset)           // min splitter size
                                dat->splitterY = oldSplitterY;
                            SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
                        }
                    }
                }
                else if((HWND) lParam == GetDlgItem(hwndDlg, IDC_PANELSPLITTER)) {
                    RECT rc;
                    POINT pt;
                    pt.x = 0; pt.y = wParam;
                    ScreenToClient(hwndDlg, &pt);
                    GetClientRect(hwndDlg, &rc);
                    if(pt.y + 6 >= 51 && pt.y + 6 < 100)
                        dat->panelHeight = pt.y + 6;
                    SendMessage(hwndDlg, WM_SIZE, DM_SPLITTERMOVED, 0);
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_PANELUIN), NULL, FALSE);
                    break;
                }
                SendMessage(hwndDlg, WM_SIZE, DM_SPLITTERMOVED, 0);
                break;
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
            break;
        case DM_FORCEDREMAKELOG:
            if((HWND) wParam == hwndDlg)
                SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
            else {
                dat->dwFlags &= ~(MWF_LOG_ALL);
                dat->dwFlags |= (lParam & MWF_LOG_ALL);
                dat->dwFlags |= MWF_DEFERREDREMAKELOG;
            }
            break;
        case DM_REMAKELOG:
            dat->lastEventTime = 0;
            dat->iLastEventType = -1;
            StreamInEvents(hwndDlg, dat->hDbEventFirst, -1, 0, NULL);
            break;
        case DM_APPENDTOLOG:
            StreamInEvents(hwndDlg, (HANDLE) wParam, 1, 1, NULL);
            break;
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
                SetDlgItemTextA(hwndDlg, IDC_LOGFROZENTEXT, Translate("Message Log is frozen"));
                break;
            }
        case DM_SCROLLLOGTOBOTTOM:
            {
                SCROLLINFO si = { 0 };

                if(dat->dwEventIsShown & MWF_SHOW_SCROLLINGDISABLED)
                    break;
                if(!IsIconic(dat->pContainer->hwnd)) {
                    if(dat->hwndLog) {
                        IEVIEWWINDOW iew = {0};
                        iew.cbSize = sizeof(IEVIEWWINDOW);
                        iew.iType = IEW_SCROLLBOTTOM;
                        iew.hwnd = dat->hwndLog;
                        CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&iew);
                    }
                    else {
                        HWND hwnd = dat->hwndLog ? dat->hwndLog : GetDlgItem(hwndDlg, IDC_LOG);
                        dat->dwFlags &= ~MWF_DEFERREDSCROLL;
                        if ((GetWindowLong(hwnd, GWL_STYLE) & WS_VSCROLL) == 0)
                            break;
                        if(lParam)
                            SendMessage(hwnd, WM_SIZE, 0, 0);
                        si.cbSize = sizeof(si);
                        si.fMask = SIF_PAGE | SIF_RANGE;
                        GetScrollInfo(hwnd, SB_VERT, &si);
                        si.fMask = SIF_POS;
                        si.nPos = si.nMax - si.nPage + 1;
                        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
                        if(wParam)
                            SendMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
                        else
                            PostMessage(hwnd, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
                        if(lParam)
                            InvalidateRect(hwnd, NULL, FALSE);
                    }
                }
                else
                    dat->dwFlags |= MWF_DEFERREDSCROLL;
                
                break;
            }
        case DM_FORCESCROLL:
            {
                int len;

                len = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_LOG));
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETSEL, len - 1, len - 1);
                break;
            }
            /*
             * this is called whenever a new event has been added to the database.
             */
        case HM_DBEVENTADDED:
            if ((HANDLE) wParam != dat->hContact)
                break;
            if (dat->hContact == NULL)
                break;
            {
                DBEVENTINFO dbei = { 0};
                DWORD dwTimestamp = 0;
                
                dbei.cbSize = sizeof(dbei);
                dbei.cbBlob = 0;
                CallService(MS_DB_EVENT_GET, lParam, (LPARAM) & dbei);
                if (dat->hDbEventFirst == NULL)
                    dat->hDbEventFirst = (HANDLE) lParam;
                
                if (dbei.eventType == EVENTTYPE_MESSAGE && (dbei.flags & DBEF_READ))
                    break;
                if (DbEventIsShown(dat, &dbei)) {
                    if (dbei.eventType == EVENTTYPE_MESSAGE && dat->pContainer->hwndStatus && !(dbei.flags & (DBEF_SENT))) {
                        dat->lastMessage = dbei.timestamp;
                        SendMessage(hwndDlg, DM_UPDATELASTMESSAGE, 0, 0);
                    }
                    /*
                     * set the message log divider to mark new (maybe unseen) messages, if the container has
                     * been minimized or in the background.
                     */
                    if(!(dbei.flags & DBEF_SENT) && dbei.eventType != EVENTTYPE_STATUSCHANGE) {
                        int iDividerSet = 0;
                        
                        if(myGlobals.m_DividersUsePopupConfig) {
                            if(!MessageWindowOpened((WPARAM)dat->hContact, 0)) {
                                iDividerSet = 1;
                                SendMessage(hwndDlg, DM_ADDDIVIDER, 0, 0);
                            }
                        }
                        if((GetForegroundWindow() != dat->pContainer->hwnd || GetActiveWindow() != dat->pContainer->hwnd)) {
                            if(!iDividerSet)
                                SendMessage(hwndDlg, DM_ADDDIVIDER, 0, 0);
                        }
                        else {
                            if(dat->pContainer->hwndActive != hwndDlg) {
                                if(!iDividerSet)
                                    SendMessage(hwndDlg, DM_ADDDIVIDER, 0, 0);
                            }
                        }
                        tabSRMM_ShowPopup(wParam, lParam, dbei.eventType, 1, dat->pContainer, hwndDlg, dat->bIsMeta ? dat->szMetaProto : dat->szProto, dat);
                    }
                    
                    if ((HANDLE) lParam != dat->hDbEventFirst) {
                        HANDLE nextEvent = (HANDLE) CallService(MS_DB_EVENT_FINDNEXT, lParam, 0);
                        if(myGlobals.m_FixFutureTimestamps || nextEvent == 0) {
                            if(!(dat->dwEventIsShown & MWF_SHOW_SCROLLINGDISABLED))
                                SendMessage(hwndDlg, DM_APPENDTOLOG, lParam, 0);
                            else {
                                char szBuf[40];
                                if(dat->iNextQueuedEvent >= dat->iEventQueueSize) {
                                    dat->hQueuedEvents = realloc(dat->hQueuedEvents, (dat->iEventQueueSize + 10) * sizeof(HANDLE));
                                    dat->iEventQueueSize += 10;
                                }
                                dat->hQueuedEvents[dat->iNextQueuedEvent++] = (HANDLE)lParam;
                                mir_snprintf(szBuf, sizeof(szBuf), Translate("Message Log is frozen (%d queued)"), dat->iNextQueuedEvent);
                                SetDlgItemTextA(hwndDlg, IDC_LOGFROZENTEXT, szBuf);
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
                    
                    if (dat->iTabID == -1)
                        MessageBoxA(0, "DBEVENTADDED Critical: iTabID == -1", "Error", MB_OK);
                    
                    dat->dwLastActivity = GetTickCount();
                    dat->pContainer->dwLastActivity = dat->dwLastActivity;
                    // tab flashing
                    if ((IsIconic(dat->pContainer->hwnd) || TabCtrl_GetCurSel(hwndTab) != dat->iTabID) && !(dbei.flags & DBEF_SENT) && dbei.eventType != EVENTTYPE_STATUSCHANGE) {
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
                    if(!(dbei.flags & DBEF_SENT) && dbei.eventType != EVENTTYPE_STATUSCHANGE) {
                        if(IsIconic(dat->pContainer->hwnd) && !IsZoomed(dat->pContainer->hwnd) && myGlobals.m_AutoSwitchTabs && dat->pContainer->hwndActive != hwndDlg) {
                            int iItem = GetTabIndexFromHWND(GetParent(hwndDlg), hwndDlg);
                            if (iItem >= 0) {
                                TabCtrl_SetCurSel(GetParent(hwndDlg), iItem);
                                ShowWindow(dat->pContainer->hwndActive, SW_HIDE);
                                dat->pContainer->hwndActive = hwndDlg;
                                SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                                dat->pContainer->dwFlags |= CNT_DEFERREDTABSELECT;
                            }
                        }
                    }
                    /*
                     * flash window if it is not focused
                     */
                    if ((GetActiveWindow() != dat->pContainer->hwnd || GetForegroundWindow() != dat->pContainer->hwnd) && !(dbei.flags & DBEF_SENT) && dbei.eventType != EVENTTYPE_STATUSCHANGE) {
                        if (!(dat->pContainer->dwFlags & CNT_NOFLASH))
                            FlashContainer(dat->pContainer, 1, 0);
                        // XXX set the message icon in the container
                        SendMessage(dat->pContainer->hwnd, DM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_EVENT_MESSAGE));
                        dat->pContainer->dwFlags |= CNT_NEED_UPDATETITLE;
                    }
                    /*
                     * play a sound
                     */
                    if (dbei.eventType == EVENTTYPE_MESSAGE && !(dbei.flags & (DBEF_SENT)))
                        PlayIncomingSound(dat->pContainer, hwndDlg);
                }
                break;
            }
        case WM_TIMER:
            /*
             * timer id for message timeouts is composed like:
             * for single message sends: basevalue (TIMERID_MSGSEND) + send queue index
             * for multisend: each send entry (hContact/hSendID pair) has its own timer starting at TIMERID_MSGSEND + NR_SENDJOBS in blocks
             * of SENDJOBS_MAX_SENDS)
             */
           
            if (wParam == TIMERID_AWAYMSG || wParam == TIMERID_AWAYMSG + 1) {
                POINT pt;
                RECT rc, rcNick;
                
                KillTimer(hwndDlg, wParam);
                dat->dwEventIsShown &= ~MWF_SHOW_AWAYMSGTIMER;
                GetCursorPos(&pt);
                GetWindowRect(GetDlgItem(hwndDlg, IDC_PANELSTATUS), &rc);
                GetWindowRect(GetDlgItem(hwndDlg, IDC_PANELNICK), &rcNick);
                if(wParam == TIMERID_AWAYMSG && PtInRect(&rc, pt)) {
                    if(GetTickCount() - dat->lastRetrievedStatusMsg > 60000) {
                        SendMessage(hwndDlg, DM_ACTIVATETOOLTIP, 0, (LPARAM)Translate("Retrieving..."));
                        if(!(dat->hProcessAwayMsg = (HANDLE)CallContactService(dat->bIsMeta ? dat->hSubContact : dat->hContact, PSS_GETAWAYMSG, 0, 0)))
                            SendMessage(hwndDlg, DM_ACTIVATETOOLTIP, 0, (LPARAM)Translate("No status message available"));
                        dat->lastRetrievedStatusMsg = GetTickCount();
                    }
                    else
                        SendMessage(hwndDlg, DM_ACTIVATETOOLTIP, 0, 0);
                }
                else if(wParam == (TIMERID_AWAYMSG + 1) && PtInRect(&rcNick, pt) && dat->xStatus > 0) {
                    char szBuffer[128];
                    mir_snprintf(szBuffer, sizeof(szBuffer), "Extended status: %s", xStatusDescr[dat->xStatus - 1]);
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
                if (dat->iTabID == -1) {
                    MessageBoxA(0, "TIMER FLASH Critical: iTabID == -1", "Error", MB_OK);
                } else {
                    if (dat->mayFlashTab)
                        FlashTab(dat, hwndTab, dat->iTabID, &dat->bTabFlash, TRUE, dat->hTabIcon);
                }
            } else if (wParam == TIMERID_TYPE) {
                if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON && GetTickCount() - dat->nLastTyping > TIMEOUT_TYPEOFF) {
                    NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
                }
                if (dat->showTyping) {
                    if (dat->nTypeSecs > 0) {
                        dat->nTypeSecs--;
                        if (GetForegroundWindow() == dat->pContainer->hwnd)
                            SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
                    } else {
                        dat->showTyping = 0;
                        SendMessage(hwndDlg, DM_UPDATELASTMESSAGE, 0, 0);
                        SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
                        HandleIconFeedback(hwndDlg, dat, (HICON)-1);
                        SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, 0, 0);
                        if(!(dat->pContainer->dwFlags & CNT_NOFLASH) && dat->showTypingWin)
                            ReflashContainer(dat->pContainer);
                    }
                } else {
                    if (dat->nTypeSecs > 0) {
                        char szBuf[256];

                        mir_snprintf(szBuf, sizeof(szBuf), Translate("%s is typing..."), dat->szNickname);
                        dat->nTypeSecs--;
                        if(dat->pContainer->hwndStatus && dat->pContainer->hwndActive == hwndDlg) {
                            SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) szBuf);
                            SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 0, (LPARAM) myGlobals.g_buttonBarIcons[5]);
                            if(dat->pContainer->hwndSlist)
                                SendMessage(dat->pContainer->hwndSlist, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[5]);
                        }
                        if(IsIconic(dat->pContainer->hwnd) || GetForegroundWindow() != dat->pContainer->hwnd || GetActiveWindow() != dat->pContainer->hwnd) {
                            SetWindowTextA(dat->pContainer->hwnd, szBuf);
                            dat->pContainer->dwFlags |= CNT_NEED_UPDATETITLE;
                            if(!(dat->pContainer->dwFlags & CNT_NOFLASH) && dat->showTypingWin)
                                ReflashContainer(dat->pContainer);
                        }
                        if (dat->pContainer->hwndActive != hwndDlg) {
                            if(dat->mayFlashTab)
                                dat->iFlashIcon = myGlobals.g_IconTypingEvent;
                            HandleIconFeedback(hwndDlg, dat, myGlobals.g_IconTypingEvent);
                        }
                        else {          // active tab may show icon if status bar is disabled
                            if(!dat->pContainer->hwndStatus) {
                                if(TabCtrl_GetItemCount(GetParent(hwndDlg)) > 1 || !(dat->pContainer->dwFlags & CNT_HIDETABS)) {
                                    HandleIconFeedback(hwndDlg, dat, myGlobals.g_IconTypingEvent);
                                }
                            }
                        }
                        if ((GetForegroundWindow() != dat->pContainer->hwnd) || (dat->pContainer->hwndStatus == 0)) {
                            SendMessage(dat->pContainer->hwnd, DM_SETICON, (WPARAM) ICON_BIG, (LPARAM) myGlobals.g_buttonBarIcons[5]);
                            if(dat->pContainer->hwndSlist)
                                SendMessage(dat->pContainer->hwndSlist, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[5]);
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
                                if(!nen_options.iNoSounds && !(dat->pContainer->dwFlags & CNT_NOSOUND))
                                    SkinPlaySound("SendMsg");
                                if (dat->hDbEventFirst == NULL)
                                    SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                                SaveInputHistory(hwndDlg, dat, 0, 0);
                                EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);

                                if(dat->pContainer->hwndActive == hwndDlg)
                                    UpdateReadChars(hwndDlg, dat);
                                SendDlgItemMessage(hwndDlg, IDC_SAVE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[6]);
                                SendDlgItemMessage(hwndDlg, IDC_SAVE, BUTTONADDTOOLTIP, (WPARAM) pszIDCSAVE_close, 0);
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
                                sendJobs[dat->iCurrentQueueError].hSendId[i] = (HANDLE) CallContactService(sendJobs[dat->iCurrentQueueError].hContact[i], MsgServiceName(sendJobs[dat->iCurrentQueueError].hContact[i], dat, sendJobs[dat->iCurrentQueueError].dwFlags), dat->sendMode & SMODE_FORCEANSI ? 0 : sendJobs[dat->iCurrentQueueError].dwFlags, (LPARAM) sendJobs[dat->iCurrentQueueError].sendBuffer);
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
            SendMessage(dat->pContainer->hwnd, DM_SELECTTAB, wParam, lParam);       // pass the msg to our container
            break;
        case DM_SAVELOCALE: 
        {
            if (myGlobals.m_AutoLocaleSupport && dat->hContact && dat->pContainer->hwndActive == hwndDlg) {
                char szKLName[KL_NAMELENGTH + 1];
                
                if((HKL)lParam != dat->hkl) {
                    dat->hkl = (HKL)lParam;
                    ActivateKeyboardLayout(dat->hkl, 0);
                    GetKeyboardLayoutNameA(szKLName);
                    DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "locale", szKLName);
                    GetLocaleID(dat, szKLName);
                    UpdateReadChars(hwndDlg, dat);
                }
            }
            break;
        }
        case DM_LOADLOCALE:
            /*
             * set locale if saved to contact
             */
            if(dat->dwFlags & MWF_WASBACKGROUNDCREATE)
                break;
            if (myGlobals.m_AutoLocaleSupport && dat->hContact != 0) {
                DBVARIANT dbv;
                int res;
                char szKLName[KL_NAMELENGTH+1];
                UINT flags = KLF_ACTIVATE;

                res = DBGetContactSetting(dat->hContact, SRMSGMOD_T, "locale", &dbv);
                if (res == 0 && dbv.type == DBVT_ASCIIZ) {
                    dat->hkl = LoadKeyboardLayoutA(dbv.pszVal, KLF_ACTIVATE);
                    PostMessage(hwndDlg, DM_SETLOCALE, 0, 0);
                    GetLocaleID(dat, dbv.pszVal);
                    DBFreeVariant(&dbv);
                } else {
                    GetKeyboardLayoutNameA(szKLName);
                    dat->hkl = LoadKeyboardLayoutA(szKLName, 0);
                    DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "locale", szKLName);
                    GetLocaleID(dat, szKLName);
                }
                UpdateReadChars(hwndDlg, dat);
            }
            break;
        case DM_SETLOCALE:
            if(dat->dwFlags & MWF_WASBACKGROUNDCREATE)
                break;
            if (dat->pContainer->hwndActive == hwndDlg && myGlobals.m_AutoLocaleSupport && dat->hContact != 0 && dat->pContainer->hwnd == GetForegroundWindow() && dat->pContainer->hwnd == GetActiveWindow()) {
                if (lParam == 0) {
                    if (GetKeyboardLayout(0) != dat->hkl) {
                        ActivateKeyboardLayout(dat->hkl, 0);
                    }
                } else {
                    dat->hkl = (HKL) lParam;
                    ActivateKeyboardLayout(dat->hkl, 0);
                }
            }
            break;
        /*
         * return timestamp (in ticks) of last recent message which has not been read yet.
         * 0 if there is none
         * lParam = pointer to a dword receiving the value.
         */
        case DM_QUERYLASTUNREAD:{
                DWORD *pdw = (DWORD *)lParam;
                *pdw = dat->dwTickLastEvent;
                return 0;
            }
        case DM_QUERYCONTAINER: {
                struct ContainerWindowData **pc = (struct ContainerWindowData **) lParam;
                *pc = dat->pContainer;
                return 0;
            }
        case DM_QUERYCONTAINERHWND: {
                HWND *pHwnd = (HWND *) lParam;
                *pHwnd = dat->pContainer->hwnd;
                return 0;
            }
        case DM_QUERYHCONTACT: {
                HANDLE *phContact = (HANDLE *) lParam;
                *phContact = dat->hContact;
                return 0;
            }
        case DM_QUERYSTATUS: {
                WORD *wStatus = (WORD *) lParam;
                *wStatus = dat->bIsMeta ? dat->wMetaStatus : dat->wStatus;
                return 0;
            }
        case DM_QUERYFLAGS: {
                DWORD *dwFlags = (DWORD *) lParam;
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
                dat->uMinHeight = height;
                return 0;
            }
        case DM_QUERYMINHEIGHT: {
                UINT *puMinHeight = (UINT *) lParam;

                *puMinHeight = dat->uMinHeight;
                return 0;
            }
        case DM_SAVESIZE: {
                RECT rcClient;

                dat->dwFlags &= ~MWF_NEEDCHECKSIZE;
                if(dat->dwFlags & MWF_WASBACKGROUNDCREATE) {
                    dat->dwFlags &= ~MWF_INITMODE;
                    if(dat->lastMessage)
                        SendMessage(hwndDlg, DM_UPDATELASTMESSAGE, 0, 0);
                }
                SendMessage(dat->pContainer->hwnd, DM_QUERYCLIENTAREA, 0, (LPARAM)&rcClient);
                MoveWindow(hwndDlg, rcClient.left, rcClient.top, (rcClient.right - rcClient.left), (rcClient.bottom - rcClient.top), TRUE);
                if(dat->dwFlags & MWF_WASBACKGROUNDCREATE) {
                    POINT pt = {0};;
                    
                    dat->dwFlags &= ~MWF_WASBACKGROUNDCREATE;
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    LoadSplitter(hwndDlg, dat);
                    ShowPicture(hwndDlg,dat,FALSE,TRUE);
                    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETSCROLLPOS, 0, (LPARAM)&pt);
                    SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
                    SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
                    SendMessage(hwndDlg, DM_LOADLOCALE, 0, 0);
                    SendMessage(hwndDlg, DM_SETLOCALE, 0, 0);
                    PostMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 1, 1);
                    if(dat->hwndLog != 0)
                        SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                }
                else {
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    PostMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 1, 1);
                }
                break;
            }
        case DM_CHECKSIZE:
            dat->dwFlags |= MWF_NEEDCHECKSIZE;
            break;
        /*
         * sent by the message input area hotkeys. just pass it to our container
         */
        case DM_QUERYPENDING:
            SendMessage(dat->pContainer->hwnd, DM_QUERYPENDING, wParam, lParam);
            break;
        case DM_RECALCPICTURESIZE:
        {
            BITMAP bminfo;
            HBITMAP hbm = dat->dwEventIsShown & MWF_SHOW_INFOPANEL ? dat->hOwnPic : dat->hContactPic;
            
            if(hbm == 0) {
                dat->pic.cy = dat->pic.cx = 60;
                break;
            }
            GetObject(hbm, sizeof(bminfo), &bminfo);
            CalcDynamicAvatarSize(hwndDlg, dat, &bminfo);
            SendMessage(hwndDlg, WM_SIZE, 0, 0);
            break;
        }
        /*
         * recalculate minimum allowed splitter position to avoid clipping on the
         * user picture.
         */
        case DM_UPDATEPICLAYOUT:
            if (dat->showPic) {
                HBITMAP hbm = dat->dwEventIsShown & MWF_SHOW_INFOPANEL ? dat->hOwnPic : dat->hContactPic;
                int iOffset = 0;
                
                if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC) {
                    dat->bottomOffset = dat->dynaSplitter + 100;
                    break;
                }
                if(hbm) {
                    dat->bottomOffset = dat->iRealAvatarHeight;
                    dat->bottomOffset += 3;
                }
            }
            break;
        case WM_LBUTTONDBLCLK:
            if(GetKeyState(VK_CONTROL) & 0x8000) {
                SendMessage(dat->pContainer->hwnd, WM_CLOSE, 1, 0);
                break;
            }
            if(GetKeyState(VK_SHIFT) & 0x8000) {
                SendMessage(dat->pContainer->hwnd, WM_SYSCOMMAND, IDM_NOTITLE, 0);
                break;
            }
            SendMessage(dat->pContainer->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            break;
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
        case WM_RBUTTONUP: {
            POINT pt;
            int iSelection;
            HMENU subMenu;
            int isHandled;
            RECT rcPicture, rcPanelPicture;
            int menuID = 0;
            
            GetWindowRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), &rcPicture);
            GetWindowRect(GetDlgItem(hwndDlg, IDC_PANELPIC), &rcPanelPicture);
            GetCursorPos(&pt);
            
            if(PtInRect(&rcPicture, pt))
                menuID = MENU_PICMENU;
            else if(PtInRect(&rcPanelPicture, pt))
                menuID = MENU_PANELPICMENU;
            
            if((menuID == MENU_PICMENU && (dat->hContactPic || dat->hOwnPic) && dat->showPic !=0) || (menuID == MENU_PANELPICMENU && dat->dwEventIsShown & MWF_SHOW_INFOPANEL)) {
                int iSelection, isHandled;
                HMENU submenu = 0;

                submenu = GetSubMenu(dat->pContainer->hMenuContext, menuID == MENU_PICMENU ? 1 : 11);
                GetCursorPos(&pt);
                MsgWindowUpdateMenu(hwndDlg, dat, submenu, menuID);
                iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
                isHandled = MsgWindowMenuHandler(hwndDlg, dat, iSelection, menuID);
                break;
            }
            subMenu = GetSubMenu(dat->pContainer->hMenuContext, 0);
            EnableMenuItem(subMenu, ID_TABMENU_SWITCHTONEXTTAB, dat->pContainer->iChilds > 1 ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(subMenu, ID_TABMENU_SWITCHTOPREVIOUSTAB, dat->pContainer->iChilds > 1 ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem(subMenu, ID_TABMENU_ATTACHTOCONTAINER, DBGetContactSettingByte(NULL, SRMSGMOD_T, "useclistgroups", 0) || DBGetContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0) ? MF_GRAYED : MF_ENABLED);
            
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
                    if(!DBGetContactSetting(NULL, szKey, szIndex, &dbv)) {
#if defined(_UNICODE)
                        WCHAR *wszTemp = Utf8_Decode(dbv.pszVal);
                        SendMessage(hwndDlg, DM_CONTAINERSELECTED, 0, (LPARAM) wszTemp);
                        free(wszTemp);
#else                        
                        SendMessage(hwndDlg, DM_CONTAINERSELECTED, 0, (LPARAM) dbv.pszVal);
#endif                        
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
                
                if (dat->pContainer->dwFlags & CNT_NOTITLE && dat->dwFlags & MWF_MOUSEDOWN) {
                    GetWindowRect(dat->pContainer->hwnd, &rc);
                    MoveWindow(dat->pContainer->hwnd, rc.left - (dat->ptLast.x - pt.x), rc.top - (dat->ptLast.y - pt.y), rc.right - rc.left, rc.bottom - rc.top, TRUE);
                    dat->ptLast = pt;
                }
                else if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL && !(dat->dwEventIsShown & MWF_SHOW_INFONOTES)) {
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_PANELSTATUS), &rc);
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_PANELNICK), &rcNick);
                    if(PtInRect(&rc, pt) && myGlobals.m_DoStatusMsg) { // && !PtInRect(&rc, dat->ptLast)) {
                        if(!(dat->dwEventIsShown & MWF_SHOW_AWAYMSGTIMER)) {
                            SetTimer(hwndDlg, TIMERID_AWAYMSG, 500, 0);
                            dat->dwEventIsShown |= MWF_SHOW_AWAYMSGTIMER;
                        }
                        break;
                    }
                    else if(PtInRect(&rcNick, pt) && myGlobals.m_DoStatusMsg) {
                        if(!(dat->dwEventIsShown & MWF_SHOW_AWAYMSGTIMER)) {
                            SetTimer(hwndDlg, TIMERID_AWAYMSG + 1, 500, 0);
                            dat->dwEventIsShown |= MWF_SHOW_AWAYMSGTIMER;
                        }
                        break;
                    }
                    else if(IsWindowVisible(dat->hwndTip)) {
                        GetWindowRect(GetDlgItem(hwndDlg, IDC_PANEL), &rc);
                        if(PtInRect(&rc, pt))
                            SendMessage(dat->hwndTip, TTM_TRACKACTIVATE, FALSE, 0);
                    }
                }
                break;
            }
        case WM_MEASUREITEM:
            {
                LPMEASUREITEMSTRUCT lpmi = (LPMEASUREITEMSTRUCT) lParam;
                lpmi->itemHeight = 0;
                lpmi->itemWidth  += 6;
                return CallService(MS_CLIST_MENUMEASUREITEM, wParam, lParam);
            }
        case WM_NCHITTEST:
            SendMessage(dat->pContainer->hwnd, WM_NCHITTEST, wParam, lParam);
            break;
        case WM_DRAWITEM:
            {
                LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;
                if(dis->CtlType == ODT_BUTTON && dis->CtlID == IDC_TOGGLESIDEBAR) {
                    HICON hIcon;
                    DWORD bStyle = 0;
                    if(dat->pContainer->dwFlags & CNT_SIDEBAR)
                        bStyle = DFCS_PUSHED;
                    else
                        bStyle = DFCS_FLAT;
                    DrawFrameControl(dis->hDC, &dis->rcItem, DFC_BUTTON, DFCS_BUTTONPUSH | bStyle);
                    hIcon = myGlobals.g_buttonBarIcons[25];
                    DrawState(dis->hDC, NULL, NULL, (LPARAM) hIcon, 0,
                              (dis->rcItem.right + dis->rcItem.left - myGlobals.m_smcxicon) / 2,
                              (dis->rcItem.bottom + dis->rcItem.top - myGlobals.m_smcyicon) / 2,
                              myGlobals.m_smcxicon, myGlobals.m_smcyicon,
                              DST_ICON | DSS_NORMAL);
                    return TRUE;
                } else if (dis->CtlType == ODT_MENU && dis->hwndItem == (HWND)GetSubMenu(myGlobals.g_hMenuContext, 7)) {
                    RECT rc = { 0 };
                    HBRUSH old, col;
                    COLORREF clr;
                    switch(dis->itemID) {
                        case ID_FONT_RED:
                            clr = RGB(255, 0, 0);
                            break;
                        case ID_FONT_BLUE:
                            clr = RGB(0, 0, 255);
                            break;
                        case ID_FONT_GREEN:
                            clr = RGB(0, 255, 0);
                            break;
                        case ID_FONT_MAGENTA:
                            clr = RGB(255, 0, 255);
                            break;
                        case ID_FONT_YELLOW:
                            clr = RGB(255, 255, 0);
                            break;
                        case ID_FONT_WHITE:
                            clr = RGB(255, 255, 255);
                            break;
                        case ID_FONT_DEFAULTCOLOR:
                            clr = GetSysColor(COLOR_MENU);
                            break;
                        default:
                            clr = 0;
                    }
                    col = CreateSolidBrush(clr);
                    old = SelectObject(dis->hDC, col);
                    rc.left = 2; rc.top = dis->rcItem.top - 5;
                    rc.right = 18; rc.bottom = dis->rcItem.bottom + 4;
                    Rectangle(dis->hDC, rc.left - 1, rc.top - 1, rc.right + 1, rc.bottom + 1);
                    FillRect(dis->hDC, &rc, col);
                    SelectObject(dis->hDC, old);
                    DeleteObject(col);
                    return TRUE;
                } else if ((dis->hwndItem == GetDlgItem(hwndDlg, IDC_CONTACTPIC) && dat->hContactPic && dat->showPic) || (dis->hwndItem == GetDlgItem(hwndDlg, IDC_PANELPIC) && dat->dwEventIsShown & MWF_SHOW_INFOPANEL && dat->hContactPic)) {
                    HPEN hPen, hOldPen;
                    HBRUSH hOldBrush;
                    BITMAP bminfo;
                    double dAspect = 0, dNewWidth = 0, dNewHeight = 0;
                    DWORD iMaxHeight = 0, top, cx, cy;
                    RECT rc, rcClient;
                    HDC hdcDraw;
                    HBITMAP hbmDraw, hbmOld;
                    BOOL bPanelPic = dis->hwndItem == GetDlgItem(hwndDlg, IDC_PANELPIC);
                    
                    if(bPanelPic)
                        GetObject(dat->hContactPic, sizeof(bminfo), &bminfo);
                    else 
                        GetObject(dat->dwEventIsShown & MWF_SHOW_INFOPANEL ? dat->hOwnPic : dat->hContactPic, sizeof(bminfo), &bminfo);

                    GetClientRect(hwndDlg, &rc);
                    GetClientRect(dis->hwndItem, &rcClient);
                    cx = rcClient.right;
                    cy = rcClient.bottom;
                    hdcDraw = CreateCompatibleDC(dis->hDC);
                    hbmDraw = CreateCompatibleBitmap(dis->hDC, cx, cy);
                    hbmOld = SelectObject(hdcDraw, hbmDraw);
                    
                    if(bPanelPic) {
                        if(bminfo.bmHeight > bminfo.bmWidth) {
                            dAspect = (double)(cy - 2) / (double)bminfo.bmHeight;
                            dNewWidth = (double)bminfo.bmWidth * dAspect;
                            dNewHeight = cy - 2;
                        }
                        else {
                            dAspect = (double)(cx - 2) / (double)bminfo.bmWidth;
                            dNewHeight = (double)bminfo.bmHeight * dAspect;
                            dNewWidth = cx - 2;
                        }
                    }
                    else {
                        dAspect = (double)dat->iRealAvatarHeight / (double)bminfo.bmHeight;
                        dNewWidth = (double)bminfo.bmWidth * dAspect;
                        if(dNewWidth > (double)(rc.right) * 0.8)
                            dNewWidth = (double)(rc.right) * 0.8;
                        iMaxHeight = dat->iRealAvatarHeight;
                    }
                    hPen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
                    hOldPen = SelectObject(hdcDraw, hPen);
                    hOldBrush = SelectObject(hdcDraw, GetSysColorBrush(COLOR_3DFACE));
                    FillRect(hdcDraw, &rcClient, GetSysColorBrush(COLOR_3DFACE));
                    if(!bPanelPic) {
                        if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC)
                            Rectangle(hdcDraw, 0, 0, dat->pic.cx, dat->pic.cy);
                        else {
                            top = (dat->pic.cy - dat->iRealAvatarHeight) / 2;
                            Rectangle(hdcDraw, 0, top - 1, dat->pic.cx, top + dat->iRealAvatarHeight + 1);
                        }
                    }
                    if(((dat->dwEventIsShown & MWF_SHOW_INFOPANEL ? dat->hOwnPic : dat->hContactPic) && dat->showPic) || bPanelPic) {
                        HDC hdcMem = CreateCompatibleDC(dis->hDC);
                        HBITMAP hbmMem = (HBITMAP)SelectObject(hdcMem, bPanelPic ? dat->hContactPic : (dat->dwEventIsShown & MWF_SHOW_INFOPANEL ? dat->hOwnPic : dat->hContactPic));
                        if(bPanelPic) {
                            RECT rcFrame = rcClient;
                            rcFrame.left = rcFrame.right - ((LONG)dNewWidth + 2);
                            rcFrame.bottom = rcFrame.top + (LONG)dNewHeight + 2;
                            SetStretchBltMode(hdcDraw, HALFTONE);
                            Rectangle(hdcDraw, rcFrame.left, rcFrame.top, rcFrame.right, rcFrame.bottom);
                            StretchBlt(hdcDraw, rcFrame.left + 1, 1, (int)dNewWidth, (int)dNewHeight, hdcMem, 0, 0, bminfo.bmWidth, bminfo.bmHeight, SRCCOPY);
                        }
                        else {
                            if(dat->iRealAvatarHeight != bminfo.bmHeight) {
                                SetStretchBltMode(hdcDraw, HALFTONE);
                                if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC)
                                    StretchBlt(hdcDraw, 1, 1, (int)dNewWidth, iMaxHeight, hdcMem, 0, 0, bminfo.bmWidth, bminfo.bmHeight, SRCCOPY);
                                else
                                    StretchBlt(hdcDraw, 1, top, (int)dNewWidth, iMaxHeight, hdcMem, 0, 0, bminfo.bmWidth, bminfo.bmHeight, SRCCOPY);
                            }
                            else {
                                if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC)
                                    BitBlt(hdcDraw, 1, 1, (int)dNewWidth, iMaxHeight, hdcMem, 0, 0, SRCCOPY);
                                else
                                    BitBlt(hdcDraw, 1, top, (int)dNewWidth, iMaxHeight, hdcMem, 0, 0, SRCCOPY);
                            }
                        }
                        DeleteObject(hbmMem);
                        DeleteDC(hdcMem);
                    }
                    SelectObject(hdcDraw, hOldPen);
                    SelectObject(hdcDraw, hOldBrush);
                    DeleteObject(hPen);
                    BitBlt(dis->hDC, 0, 0, cx, cy, hdcDraw, 0, 0, SRCCOPY);
                    SelectObject(hdcDraw, hbmOld);
                    DeleteObject(hbmDraw);
                    DeleteDC(hdcDraw);
                    return TRUE;
                }
                else if(dis->hwndItem == GetDlgItem(hwndDlg, IDC_PANELSTATUS) && dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
                    char *szProto = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
                    SIZE sProto = {0}, sStatus = {0};
                    DWORD oldPanelStatusCX = dat->panelStatusCX;
                    RECT rc;
                    
                    if(dat->szStatus)
                        GetTextExtentPoint32A(dis->hDC, dat->szStatus, lstrlenA(dat->szStatus), &sStatus);
                    if(szProto)
                        GetTextExtentPoint32A(dis->hDC, szProto, lstrlenA(szProto), &sProto);

                    dat->panelStatusCX = sStatus.cx + sProto.cx + 16 + 18;
                    
                    if(dat->panelStatusCX != oldPanelStatusCX)
                        SendMessage(hwndDlg, WM_SIZE, 0, 0);

                    GetClientRect(dis->hwndItem, &rc);
                    FillRect(dis->hDC, &rc, GetSysColorBrush(COLOR_3DFACE));
                    
                    if(dat->hTabStatusIcon) {
                        if(dat->dwEventIsShown & MWF_SHOW_ISIDLE && myGlobals.m_IdleDetect) {
                            ImageList_ReplaceIcon(myGlobals.g_hImageList, 0, dat->hTabStatusIcon);
                            ImageList_DrawEx(myGlobals.g_hImageList, 0, dis->hDC, 3, (rc.bottom + rc.top - myGlobals.m_smcyicon) / 2, 0, 0, CLR_NONE, CLR_NONE, ILD_SELECTED);
                        }
                        else
                            DrawIconEx(dis->hDC, 3, (rc.bottom + rc.top - myGlobals.m_smcyicon) / 2, dat->hTabStatusIcon, myGlobals.m_smcxicon, myGlobals.m_smcyicon, 0, 0, DI_NORMAL | DI_COMPAT);
                    }
                    rc.left += 22;
                    if(dat->szStatus)
                        DrawTextA(dis->hDC, dat->szStatus, lstrlenA(dat->szStatus), &rc, DT_SINGLELINE | DT_VCENTER);
                    if(szProto) {
                        rc.left = rc.right - sProto.cx - 3;
                        SetTextColor(dis->hDC, GetSysColor(COLOR_HOTLIGHT));
                        DrawTextA(dis->hDC, szProto, lstrlenA(szProto), &rc, DT_SINGLELINE | DT_VCENTER);
                    }
                    return TRUE;
                }
                else if(dis->hwndItem == GetDlgItem(hwndDlg, IDC_PANELNICK) && dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
                    FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_3DFACE));
                    dis->rcItem.left +=2;
                    if(dat->szNickname) {
#if defined(_UNICODE)
                        wchar_t szNickW[512];
#endif                        
                        if(dat->xStatus > 0 && dat->xStatus < 24) {
                            HICON xIcon = ImageList_ExtractIcon(NULL, myGlobals.g_xIcons, dat->xStatus - 1);
                            DrawIconEx(dis->hDC, 3, (dis->rcItem.bottom + dis->rcItem.top - myGlobals.m_smcyicon) / 2, xIcon, myGlobals.m_smcxicon, myGlobals.m_smcyicon, 0, 0, DI_NORMAL | DI_COMPAT);
                            DestroyIcon(xIcon);
                            dis->rcItem.left += 21;
                        }
#if defined(_UNICODE)
                        MultiByteToWideChar(dat->codePage, 0, dat->szNickname, -1, szNickW, 512);
                        szNickW[511] = 0;
                        DrawTextW(dis->hDC, szNickW, lstrlenW(szNickW), &dis->rcItem, DT_SINGLELINE | DT_VCENTER);
#else
                        DrawTextA(dis->hDC, dat->szNickname, lstrlenA(dat->szNickname), &dis->rcItem, DT_SINGLELINE | DT_VCENTER);
#endif                        
                    }
                    return TRUE;
                }
                else if(dis->hwndItem == GetDlgItem(hwndDlg, IDC_PANELUIN) && dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
                    char szBuf[256];
                    FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_3DFACE));
                    dis->rcItem.left +=2;
                    if(dat->uin) {
                        SIZE sUIN, sTime;
                        if(dat->idle) {
                            time_t diff = time(NULL) - dat->idle;
                            int i_hrs = diff / 3600;
                            int i_mins = (diff - i_hrs * 3600) / 60;
                            mir_snprintf(szBuf, sizeof(szBuf), "%s    Idle: %dh,%02dm", dat->uin, i_hrs, i_mins);
                            GetTextExtentPoint32A(dis->hDC, szBuf, lstrlenA(szBuf), &sUIN);
                            DrawTextA(dis->hDC, szBuf, lstrlenA(szBuf), &dis->rcItem, DT_SINGLELINE | DT_VCENTER);
                        }
                        else {
                            GetTextExtentPoint32A(dis->hDC, dat->uin, lstrlenA(dat->uin), &sUIN);
                            DrawTextA(dis->hDC, dat->uin, lstrlenA(dat->uin), &dis->rcItem, DT_SINGLELINE | DT_VCENTER);
                        }
                        if(dat->timezone != -1) {
                            DBTIMETOSTRING dbtts;
                            char szResult[80];
                            time_t final_time;
                            time_t now = time(NULL);

                            final_time = now - dat->timediff;
                            dbtts.szDest = szResult;
                            dbtts.cbDest = 70;
                            dbtts.szFormat = "t";
                            CallService(MS_DB_TIME_TIMESTAMPTOSTRING, final_time, (LPARAM) & dbtts);
                            GetTextExtentPoint32A(dis->hDC, szResult, lstrlenA(szResult), &sTime);
                            if(sUIN.cx + sTime.cx + 23 < dis->rcItem.right - dis->rcItem.left) {
                                dis->rcItem.left = dis->rcItem.right - sTime.cx - 3 - 18;
                                DrawIconEx(dis->hDC, dis->rcItem.left, (dis->rcItem.bottom + dis->rcItem.top - myGlobals.m_smcyicon) / 2, myGlobals.g_IconClock, myGlobals.m_smcxicon, myGlobals.m_smcyicon, 0, 0, DI_NORMAL | DI_COMPAT);
                                dis->rcItem.left += 18;
                                DrawTextA(dis->hDC, szResult, lstrlenA(szResult), &dis->rcItem, DT_SINGLELINE | DT_VCENTER);
                            }
                        }
                    }
                    return TRUE;
                }
                else if(dis->hwndItem == GetDlgItem(hwndDlg, IDC_READSTATUS) && dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
                    FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_3DFACE));
                    DrawIconEx(dis->hDC, (dis->rcItem.right + dis->rcItem.left - myGlobals.m_smcxicon) / 2, (dis->rcItem.bottom + dis->rcItem.top - myGlobals.m_smcyicon) / 2, myGlobals.m_DoStatusMsg ? myGlobals.g_IconChecked : myGlobals.g_IconUnchecked, myGlobals.m_smcxicon, myGlobals.m_smcyicon, 0, 0, DI_NORMAL | DI_COMPAT);
                }
                return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
            }
        case WM_COMMAND:
            if (CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(LOWORD(wParam), MPCF_CONTACTMENU), (LPARAM) dat->hContact))
                break;
            switch (LOWORD(wParam)) {
                case IDOK:
                    {
                    int bufSize = 0, memRequired = 0, isUnicode = 0;
                    char *streamOut = NULL;
                    TCHAR *decoded = NULL, *converted = NULL;
                    
                    if (!IsWindowEnabled(GetDlgItem(hwndDlg, IDOK)))
                        break;

                    TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_CUSTOM, tabMSG_WINDOW_EVT_CUSTOM_BEFORESEND);
#if defined( _UNICODE )
                    streamOut = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, dat->SendFormat ? 0 : (CP_UTF8 << 16) | (SF_TEXT|SF_USECODEPAGE));
                    if(streamOut != NULL) {
                        decoded = Utf8_Decode(streamOut);
                        if(decoded != NULL) {
                            if(dat->SendFormat) {
                                DoRtfToTags(decoded, dat);
                                DoTrimMessage(decoded);
                            }
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
                    streamOut = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, dat->SendFormat ? (SF_RTFNOOBJS|SFF_PLAINRTF) : (SF_TEXT));
                    if(streamOut != NULL) {
                        converted = (TCHAR *)malloc((lstrlenA(streamOut) + 2)* sizeof(TCHAR));
                        if(converted != NULL) {
                            _tcscpy(converted, streamOut);
                            if(dat->SendFormat) {
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
                        if(dat->hwndLog != 0 && dat->pContainer->hwndStatus) {
                            SendMessage(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM)Translate("Message saved for later delivery"));
                        }
                        else
                            LogErrorMessage(hwndDlg, dat, -1, Translate("Message saved for later delivery"));
                        
                        SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
                        break;
                    }
                    if (dat->sendMode & SMODE_CONTAINER && dat->pContainer->hwndActive == hwndDlg && GetForegroundWindow() == dat->pContainer->hwnd) {
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
                                 bOldState = IsWindowEnabled(GetDlgItem(contacthwnd, IDOK));
                                 EnableWindow(GetDlgItem(contacthwnd,IDOK), 1);
                                 // simulate an ok
                                 // SendDlgItemMessage(contacthwnd, IDOK, BM_CLICK, 0, 0);
                                 SendMessage(contacthwnd, WM_COMMAND, IDOK, 0);
                                 EnableWindow(GetDlgItem(contacthwnd, IDOK), bOldState);
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
                    AddToSendQueue(hwndDlg, dat, memRequired, isUnicode);
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
                        int iSize = 0;
                        SETTEXTEX stx = {ST_SELECTION, 1200};
#endif                        
                        if(dat->hwndLog != 0) {                 // IEView quoting support..
                            TCHAR *selected = 0, *szQuoted = 0;
                            IEVIEWEVENT event;
                            ZeroMemory((void *)&event, sizeof(event));
                            event.cbSize = sizeof(IEVIEWEVENT);
                            event.hwnd = dat->hwndLog;
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
                        HMENU submenu = GetSubMenu(dat->pContainer->hMenuContext, 7);
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
                            while(rtf_ctable[i].szName != NULL) {
                                if(rtf_ctable[i].clr == cf.crTextColor)
                                    cf.crTextColor = RGB(GetRValue(cf.crTextColor), GetGValue(cf.crTextColor), GetBValue(cf.crTextColor) == 0 ? GetBValue(cf.crTextColor) + 1 : GetBValue(cf.crTextColor) - 1);
                                i++;
                            }
                            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
                            break;
                        }
                        for(i = 0;;i++) {
                            if(rtf_ctable[i].szName == NULL)
                                break;
                            if(rtf_ctable[i].menuid == iSelection) {
                                cf.crTextColor = rtf_ctable[i].clr;
                                SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
                            }
                        }
                        break;
                    }
                case IDCANCEL: {
                    ShowWindow(dat->pContainer->hwnd, SW_MINIMIZE);
                    return FALSE;
                    break;
                    }
                case IDC_SAVE:
                    SendMessage(hwndDlg, WM_CLOSE, 1, 0);
                    break;
                case IDC_NAME:
                case IDC_USERMENU:
                    {
                        if(GetKeyState(VK_SHIFT) & 0x8000)    // copy UIN
                            SendMessage(hwndDlg, DM_UINTOCLIPBOARD, 0, 0);
                        else {
                            RECT rc;
                            HMENU hMenu = (HMENU) CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM) dat->hContact, 0);
                            GetWindowRect(GetDlgItem(hwndDlg, LOWORD(wParam)), &rc);
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
                    if(dat->doSmileys && (myGlobals.g_SmileyAddAvail || dat->hwndLog != 0)) {
                        SMADD_SHOWSEL smaddInfo;
                        HICON hButtonIcon = 0;
                        RECT rc;
                        
                        if(CheckValidSmileyPack(dat->bIsMeta ? dat->szMetaProto : dat->szProto, &hButtonIcon) != 0 || dat->hwndLog != 0) {
                            smaddInfo.cbSize = sizeof(SMADD_SHOWSEL);
                            smaddInfo.hwndTarget = GetDlgItem(hwndDlg, IDC_MESSAGE);
                            smaddInfo.targetMessage = EM_REPLACESEL;
                            smaddInfo.targetWParam = TRUE;
                            smaddInfo.Protocolname = dat->bIsMeta ? dat->szMetaProto : dat->szProto;
                            GetWindowRect(GetDlgItem(hwndDlg, IDC_SMILEYBTN), &rc);

                            smaddInfo.Direction = 0;
                            smaddInfo.xPosition = rc.left;
                            smaddInfo.yPosition = rc.top + 24;
                            dat->dwFlags |= MWF_SMBUTTONSELECTED;
                            if(dat->hwndLog)
                                CallService(MS_IEVIEW_SHOWSMILEYSELECTION, 0, (LPARAM)&smaddInfo);
                            else
                                CallService(MS_SMILEYADD_SHOWSELECTION, (WPARAM)dat->pContainer->hwnd, (LPARAM) &smaddInfo);
                        }
                    }
                    break;
                case IDC_TIME: {
                    RECT rc;
                    HMENU submenu = GetSubMenu(dat->pContainer->hMenuContext, 2);
                    int iSelection, isHandled;
                    DWORD dwOldFlags = dat->dwFlags;
                    DWORD dwOldEventIsShown = dat->dwEventIsShown;
                    
                    MsgWindowUpdateMenu(hwndDlg, dat, submenu, MENU_LOGMENU);
                    
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_TIME), &rc);

                    iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, rc.left, rc.bottom, 0, hwndDlg, NULL);
                    isHandled = MsgWindowMenuHandler(hwndDlg, dat, iSelection, MENU_LOGMENU);

                    if(dat->dwFlags != dwOldFlags || dat->dwEventIsShown != dwOldEventIsShown) {
                        if(!DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "mwoverride", 0)) {
                            WindowList_Broadcast(hMessageWindowList, DM_DEFERREDREMAKELOG, (WPARAM)hwndDlg, (LPARAM)(dat->dwFlags & MWF_LOG_ALL));
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", dat->dwFlags & MWF_LOG_ALL);
                        }
                        else
                            SendMessage(hwndDlg, DM_DEFERREDREMAKELOG, (WPARAM)hwndDlg, 0);
                    }
                    break;
                }
                case IDC_LOGFROZEN:
                    if(dat->dwEventIsShown & MWF_SHOW_SCROLLINGDISABLED)
                        SendMessage(hwndDlg, DM_REPLAYQUEUE, 0, 0);
                    dat->dwEventIsShown &= ~MWF_SHOW_SCROLLINGDISABLED;
                    ShowWindow(GetDlgItem(hwndDlg, IDC_LOGFROZEN), SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_LOGFROZENTEXT), SW_HIDE);
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    PostMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 1);
                    break;
                case IDC_READSTATUS:
                    myGlobals.m_DoStatusMsg = !myGlobals.m_DoStatusMsg;
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_READSTATUS), NULL, FALSE);
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "dostatusmsg", myGlobals.m_DoStatusMsg);
                    break;
                case IDC_PROTOMENU: {
                    RECT rc;
                    HMENU submenu = GetSubMenu(dat->pContainer->hMenuContext, 4);
                    int iSelection;
                    int iOldGlobalSendFormat = myGlobals.m_SendFormat;
                    
                    if(dat->hContact) {
                        unsigned int iOldIEView = GetIEViewMode(hwndDlg, dat);
                        unsigned int iNewIEView = 0;
                        int iLocalFormat = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "sendformat", 0);
                        int iNewLocalFormat = iLocalFormat;
                        
                        GetWindowRect(GetDlgItem(hwndDlg, IDC_PROTOMENU), &rc);

                        EnableMenuItem(submenu, 0, MF_BYPOSITION | (ServiceExists(MS_IEVIEW_WINDOW) ? MF_ENABLED : MF_GRAYED));
                        
                        CheckMenuItem(submenu, ID_IEVIEWSETTING_USEGLOBAL, MF_BYCOMMAND | (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", 0) == 0 ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_IEVIEWSETTING_FORCEIEVIEW, MF_BYCOMMAND | (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", 0) == 1 ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_IEVIEWSETTING_FORCEDEFAULTMESSAGELOG, MF_BYCOMMAND | (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", 0) == (BYTE)-1 ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_SPLITTER_AUTOSAVEONCLOSE, MF_BYCOMMAND | (myGlobals.m_SplitterSaveOnClose ? MF_CHECKED : MF_UNCHECKED));

                        CheckMenuItem(submenu, ID_MODE_GLOBAL, MF_BYCOMMAND | (!(dat->dwEventIsShown & MWF_SHOW_SPLITTEROVERRIDE) ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_MODE_PRIVATE, MF_BYCOMMAND | (dat->dwEventIsShown & MWF_SHOW_SPLITTEROVERRIDE ? MF_CHECKED : MF_UNCHECKED));

                        /*
                         * formatting menu..
                         */

                        CheckMenuItem(submenu, ID_GLOBAL_BBCODE, MF_BYCOMMAND | ((myGlobals.m_SendFormat == SENDFORMAT_BBCODE) ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_GLOBAL_SIMPLETAGS, MF_BYCOMMAND | ((myGlobals.m_SendFormat == SENDFORMAT_SIMPLE) ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_GLOBAL_OFF, MF_BYCOMMAND | ((myGlobals.m_SendFormat == SENDFORMAT_NONE) ? MF_CHECKED : MF_UNCHECKED));

                        CheckMenuItem(submenu, ID_THISCONTACT_GLOBALSETTING, MF_BYCOMMAND | ((iLocalFormat == SENDFORMAT_NONE) ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_THISCONTACT_BBCODE, MF_BYCOMMAND | ((iLocalFormat == SENDFORMAT_BBCODE) ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_THISCONTACT_SIMPLETAGS, MF_BYCOMMAND | ((iLocalFormat == SENDFORMAT_SIMPLE) ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_THISCONTACT_OFF, MF_BYCOMMAND | ((iLocalFormat == -1) ? MF_CHECKED : MF_UNCHECKED));

                        EnableMenuItem(submenu, ID_FAVORITES_ADDCONTACTTOFAVORITES, LOWORD(dat->dwIsFavoritOrRecent) == 0 ? MF_ENABLED : MF_GRAYED);
                        EnableMenuItem(submenu, ID_FAVORITES_REMOVECONTACTFROMFAVORITES, LOWORD(dat->dwIsFavoritOrRecent) == 0 ? MF_GRAYED : MF_ENABLED);
                        
                        iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, rc.left, rc.bottom, 0, hwndDlg, NULL);
                        switch(iSelection) {
                            case ID_IEVIEWSETTING_USEGLOBAL:
                                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", 0);
                                break;
                            case ID_IEVIEWSETTING_FORCEIEVIEW:
                                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", 1);
                                break;
                            case ID_IEVIEWSETTING_FORCEDEFAULTMESSAGELOG:
                                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", -1);
                                break;
                            case ID_SPLITTER_AUTOSAVEONCLOSE:
                                myGlobals.m_SplitterSaveOnClose ^= 1;
                                DBWriteContactSettingByte(NULL, SRMSGMOD_T, "splitsavemode", myGlobals.m_SplitterSaveOnClose);
                                break;
                            case ID_SPLITTER_SAVENOW:
                                SaveSplitter(hwndDlg, dat);
                                break;
                            case ID_MODE_GLOBAL:
                                dat->dwEventIsShown &= ~(MWF_SHOW_SPLITTEROVERRIDE);
                                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "splitoverride", 0);
                                break;
                            case ID_MODE_PRIVATE:
                                dat->dwEventIsShown |= MWF_SHOW_SPLITTEROVERRIDE;
                                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "splitoverride", 1);
                                break;
                            case ID_GLOBAL_BBCODE:
                                myGlobals.m_SendFormat = SENDFORMAT_BBCODE;
                                break;
                            case ID_GLOBAL_SIMPLETAGS:
                                myGlobals.m_SendFormat = SENDFORMAT_SIMPLE;
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
                            case ID_THISCONTACT_SIMPLETAGS:
                                iNewLocalFormat = SENDFORMAT_SIMPLE;
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
                            DBWriteContactSettingByte(0, SRMSGMOD_T, "sendformat", myGlobals.m_SendFormat);
                        if(iNewLocalFormat != iLocalFormat || myGlobals.m_SendFormat != iOldGlobalSendFormat) {
                            dat->SendFormat = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "sendformat", myGlobals.m_SendFormat);
                            if(dat->SendFormat == -1)           // per contact override to disable it..
                                dat->SendFormat = 0;
                            WindowList_Broadcast(hMessageWindowList, DM_CONFIGURETOOLBAR, 0, 1);
                        }
                        iNewIEView = GetIEViewMode(hwndDlg, dat);
                        if(iNewIEView != iOldIEView)
                            SwitchMessageLog(hwndDlg, dat, iNewIEView);
                        ConfigureSideBar(hwndDlg, dat);
                    }
                    break;
                }
                case IDC_TOGGLETOOLBAR:
                    if(GetKeyState(VK_SHIFT) & 0x8000) {
                        dat->pContainer->dwFlags ^= CNT_NOMENUBAR;
                        SendMessage(dat->pContainer->hwnd, DM_CONFIGURECONTAINER, 0, 0);
                    }
                    else {
                        dat->pContainer->dwFlags ^= CNT_HIDETOOLBAR;
                        BroadCastContainer(dat->pContainer, DM_CONFIGURETOOLBAR, 0, 1);
                    }
                    SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                    break;
                case IDC_TOGGLENOTES:
                    if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
                        dat->dwEventIsShown ^= MWF_SHOW_INFONOTES;
                        ConfigurePanel(hwndDlg, dat);
                    }
                    break;
                case IDC_APPARENTMODE:
                {
                    HMENU subMenu = GetSubMenu(dat->pContainer->hMenuContext, 10);
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
                    HMENU submenu = GetSubMenu(dat->pContainer->hMenuContext, 9);
                    int iSelection;
                    BYTE bNewGlobal = dat->pContainer->dwFlags & CNT_INFOPANEL ? 1 : 0;
                    BYTE bNewLocal = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "infopanel", 0);
                    BYTE bLocal = bNewLocal, bGlobal = bNewGlobal;
                    DWORD dwOld = dat->dwEventIsShown;
                    
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_INFOPANELMENU), &rc);
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
                            dat->dwEventIsShown ^= MWF_SHOW_INFOPANEL;
                            ShowHideInfoPanel(hwndDlg, dat);
                            return 0;
                    }
                    if(bNewGlobal != bGlobal)
                        ApplyContainerSetting(dat->pContainer, CNT_INFOPANEL, bNewGlobal ? 1 : 0);
                    if(bNewLocal != bLocal)
                        DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "infopanel", bNewLocal);

                    if(bNewGlobal != bGlobal);
                        //WindowList_Broadcast(hMessageWindowList, DM_SETINFOPANEL, 0, 0);
                    else {
                        dat->dwEventIsShown = GetInfoPanelSetting(hwndDlg, dat) ? dat->dwEventIsShown | MWF_SHOW_INFOPANEL : dat->dwEventIsShown & ~MWF_SHOW_INFOPANEL;
                        if(dat->dwEventIsShown != dwOld) {
                            ShowHideInfoPanel(hwndDlg, dat);
                        }
                    }
                    break;
                }
                case IDC_SENDMENU: {
                    RECT rc;
                    HMENU submenu = GetSubMenu(dat->pContainer->hMenuContext, 3);
                    int iSelection;

                    GetWindowRect(GetDlgItem(hwndDlg, IDC_SENDMENU), &rc);
                    CheckMenuItem(submenu, ID_SENDMENU_SENDTOMULTIPLEUSERS, MF_BYCOMMAND | (dat->sendMode & SMODE_MULTIPLE ? MF_CHECKED : MF_UNCHECKED));
                    CheckMenuItem(submenu, ID_SENDMENU_SENDDEFAULT, MF_BYCOMMAND | (dat->sendMode == 0 ? MF_CHECKED : MF_UNCHECKED));
                    CheckMenuItem(submenu, ID_SENDMENU_SENDTOCONTAINER, MF_BYCOMMAND | (dat->sendMode & SMODE_CONTAINER ? MF_CHECKED : MF_UNCHECKED));
                    CheckMenuItem(submenu, ID_SENDMENU_FORCEANSISEND, MF_BYCOMMAND | (dat->sendMode & SMODE_FORCEANSI ? MF_CHECKED : MF_UNCHECKED));
                    EnableMenuItem(submenu, ID_SENDMENU_SENDLATER, MF_BYCOMMAND | (ServiceExists("BuddyPounce/AddToPounce") ? MF_ENABLED : MF_GRAYED));
                    CheckMenuItem(submenu, ID_SENDMENU_SENDLATER, MF_BYCOMMAND | (dat->sendMode & SMODE_SENDLATER ? MF_CHECKED : MF_UNCHECKED));
                    CheckMenuItem(submenu, ID_SENDMENU_SENDWITHOUTTIMEOUTS, MF_BYCOMMAND | (dat->sendMode & SMODE_NOACK ? MF_CHECKED : MF_UNCHECKED));
                    EnableMenuItem(submenu, ID_SENDMENU_SENDWITHOUTTIMEOUTS, MF_GRAYED);
                    
                    if(lParam)
                        iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, rc.left, rc.bottom, 0, hwndDlg, NULL);
                    else
                        iSelection = HIWORD(wParam);
                    
                    switch(iSelection) {
                        case ID_SENDMENU_SENDTOMULTIPLEUSERS:
                            dat->sendMode ^= SMODE_MULTIPLE;
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
                            break;
                    }
                    DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "no_ack", dat->sendMode & SMODE_NOACK ? 1 : 0);
                    DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "forceansi", dat->sendMode & SMODE_FORCEANSI ? 1 : 0);
                    if(dat->sendMode & SMODE_MULTIPLE || dat->sendMode & SMODE_CONTAINER)
                        ShowWindow(GetDlgItem(hwndDlg, IDC_MULTIPLEICON), SW_SHOW);
                    else
                        ShowWindow(GetDlgItem(hwndDlg, IDC_MULTIPLEICON), SW_HIDE);
                    SendMessage(dat->pContainer->hwnd, DM_QUERYCLIENTAREA, 0, (LPARAM)&rc);
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 0);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), (dat->sendMode & SMODE_MULTIPLE) ? SW_SHOW : SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_CLIST), (dat->sendMode & SMODE_MULTIPLE) ? SW_SHOW : SW_HIDE);
                    if(dat->hbmMsgArea)
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
                    ApplyContainerSetting(dat->pContainer, CNT_SIDEBAR, dat->pContainer->dwFlags & CNT_SIDEBAR ? 0 : 1);
                    SendMessage(hwndDlg, WM_SETREDRAW, FALSE, 0);
                    SendMessage(dat->pContainer->hwnd, DM_CONFIGURECONTAINER, 0, 0);
                    SendMessage(dat->pContainer->hwnd, WM_SIZE, 0, 1);
                    RedrawWindow(dat->pContainer->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
                    SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                    SendMessage(hwndDlg, WM_SETREDRAW, TRUE, 0);
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    RedrawWindow(hwndDlg, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
                    break;
                case IDC_PIC:
                    {
                        RECT rc;
                        int iSelection, isHandled;
                        
                        HMENU submenu = GetSubMenu(dat->pContainer->hMenuContext, 1);
                        GetWindowRect(GetDlgItem(hwndDlg, IDC_PIC), &rc);
                        MsgWindowUpdateMenu(hwndDlg, dat, submenu, MENU_PICMENU);
                        iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, rc.left, rc.bottom, 0, hwndDlg, NULL);
                        isHandled = MsgWindowMenuHandler(hwndDlg, dat, iSelection, MENU_PICMENU);
                        if(isHandled)
                            RedrawWindow(GetDlgItem(hwndDlg, IDC_PIC), NULL, NULL, RDW_INVALIDATE);
                    }
                    break;
                case IDM_CLEAR:
                    if (dat->hwndLog != 0) {
                        IEVIEWEVENT event;
                        event.cbSize = sizeof(IEVIEWEVENT);
                        event.iType = IEE_CLEAR_LOG;
                        event.hwnd = dat->hwndLog;
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
                        
                        DBWriteContactSettingByte(dat->hContact, SRMSGMOD, SRMSGSET_TYPING, !iCurrentTypingMode);
                        if(dat->pContainer->hwndStatus) {
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
                    if ((HIWORD(wParam) == EN_VSCROLL || HIWORD(wParam) == EN_HSCROLL) && dat->hbmMsgArea != 0) {
                        RECT rc;
                        GetUpdateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc, FALSE);
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc, TRUE);
                    }
                    if (HIWORD(wParam) == EN_CHANGE) {
                        RECT rc;
                        if(dat->pContainer->hwndActive == hwndDlg)
                            UpdateReadChars(hwndDlg, dat);
                        dat->dwFlags |= MWF_NEEDHISTORYSAVE;
                        dat->dwLastActivity = GetTickCount();
                        dat->pContainer->dwLastActivity = dat->dwLastActivity;
                        UpdateSaveAndSendButton(hwndDlg, dat);

                        if(dat->dwFlags & MWF_SMBUTTONSELECTED) {
                            SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwndDlg, IDC_MESSAGE), (LPARAM)TRUE);
                            SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                            dat->dwFlags &= ~MWF_SMBUTTONSELECTED;
                        }
                        if (!(GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_SHIFT) & 0x8000)) {
                            dat->nLastTyping = GetTickCount();
                            if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE))) {
                                if (dat->nTypeMode == PROTOTYPE_SELFTYPING_OFF) {
                                    NotifyTyping(dat, PROTOTYPE_SELFTYPING_ON);
                                }
                            } else {
                                if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON) {
                                    NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
                                }
                            }
                        }
                        if(dat->hbmMsgArea != 0) {
                            GetUpdateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc, FALSE);
                            InvalidateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc, TRUE);
                        }
                    }
            }
            break;
        case WM_NOTIFY:
            /*
            if(dat != 0 && ((NMHDR *)lParam)->hwndFrom == dat->hwndTip) {
                if(((NMHDR *)lParam)->code == TTN_NEEDTEXT) {
                    NMTTDISPINFO *nmtt = (NMTTDISPINFO *) lParam;
                    nmtt->hinst = 0;
                    nmtt->lpszText = testTooltip;
                    nmtt->uFlags = TTF_IDISHWND;
                }
                break;
            }*/
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
                            if(IsWindowVisible(dat->hwndTip))
                                SendMessage(dat->hwndTip, TTM_TRACKACTIVATE, FALSE, 0);
                            if(msg == WM_CHAR) {
                                if((GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_SHIFT) & 0x8000)) {
                                    switch (wp) {
                                        case 23:                // ctrl - w
                                            PostMessage(hwndDlg, WM_CLOSE, 1, 0);
                                            break;
                                        case 18:                // ctrl - r
                                            SendMessage(hwndDlg, DM_QUERYPENDING, DM_QUERY_MOSTRECENT, 0);
                                            break;
                                        case 0x0c:              // ctrl - l
                                            SendMessage(hwndDlg, WM_COMMAND, IDM_CLEAR, 0);
                                            break;
                                        case 0x0f:              // ctrl - o
                                            if(dat->pContainer->hWndOptions == 0)
                                                CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CONTAINEROPTIONS), dat->pContainer->hwnd, DlgProcContainerOptions, (LPARAM) dat->pContainer);
                                            break;
                                        case 19:
                                            PostMessage(hwndDlg, WM_COMMAND, IDC_SENDMENU, (LPARAM)GetDlgItem(hwndDlg, IDC_SENDMENU));
                                            break;
                                        case 16:
                                            PostMessage(hwndDlg, WM_COMMAND, IDC_PROTOMENU, (LPARAM)GetDlgItem(hwndDlg, IDC_PROTOMENU));
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
                                    SendMessage(dat->pContainer->hwnd, DM_SELECTTAB, DM_SELECT_NEXT, 0);
                                    return 1;
                                }
                                if(wp == VK_SUBTRACT) {
                                    SendMessage(dat->pContainer->hwnd, DM_SELECTTAB, DM_SELECT_PREV, 0);
                                    return 1;
                                }
                            }
                            if(msg == WM_KEYDOWN && wp == VK_F12) {
                                if(dat->dwEventIsShown & MWF_SHOW_SCROLLINGDISABLED)
                                    SendMessage(hwndDlg, DM_REPLAYQUEUE, 0, 0);
                                dat->dwEventIsShown ^= MWF_SHOW_SCROLLINGDISABLED;
                                ShowWindow(GetDlgItem(hwndDlg, IDC_LOGFROZEN), dat->dwEventIsShown & MWF_SHOW_SCROLLINGDISABLED ? SW_SHOW : SW_HIDE);
                                ShowWindow(GetDlgItem(hwndDlg, IDC_LOGFROZENTEXT), dat->dwEventIsShown & MWF_SHOW_SCROLLINGDISABLED ? SW_SHOW : SW_HIDE);
                                SendMessage(hwndDlg, WM_SIZE, 0, 0);
                                PostMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 1);
                                break;
                            }
                            if(msg == WM_MOUSEWHEEL && (((NMHDR *)lParam)->idFrom == IDC_LOG || ((NMHDR *)lParam)->idFrom == IDC_MESSAGE)) {
                                RECT rc;
                                POINT pt;

                                GetCursorPos(&pt);
                                GetWindowRect(GetDlgItem(hwndDlg, IDC_LOG), &rc);
                                if(PtInRect(&rc, pt))
                                    return 0;
                                //SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), WM_MOUSEWHEEL, wp, lp);
                                //((MSGFILTER *)lParam)->msg = WM_NULL;
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
                                                    SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)streamOut);
                                                    free(streamOut);
                                                }
                                                SendMessage(GetDlgItem(hwndDlg, IDC_LOG), EM_EXSETSEL, 0, (LPARAM)&cr);
                                                SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                                            }
                                            else if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "autocopy", 0) && !(GetKeyState(VK_SHIFT) & 0x8000)) {
                                                SendMessage(GetDlgItem(hwndDlg, IDC_LOG), WM_COPY, 0, 0);
                                                SendMessage(GetDlgItem(hwndDlg, IDC_LOG), EM_EXSETSEL, 0, (LPARAM)&cr);
                                                SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
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
                                        int oldCodepage = dat->codePage;
                                        int idFrom = ((NMHDR *)lParam)->idFrom;
                                        int iPrivateBG = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "private_bg", 0);
                                        
                                        hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_CONTEXT));
                                        if(idFrom == IDC_LOG)
                                            hSubMenu = GetSubMenu(hMenu, 0);
                                        else {
                                            hSubMenu = GetSubMenu(hMenu, 2);
                                            EnableMenuItem(hSubMenu, ID_EDITOR_LOADBACKGROUNDIMAGE, MF_BYCOMMAND | (iPrivateBG ? MF_GRAYED : MF_ENABLED));
                                            EnableMenuItem(hSubMenu, ID_EDITOR_REMOVEBACKGROUNDIMAGE, MF_BYCOMMAND | (iPrivateBG ? MF_GRAYED : MF_ENABLED));
                                            EnableMenuItem(hSubMenu, IDM_PASTEFORMATTED, MF_BYCOMMAND | (dat->SendFormat != 0 ? MF_ENABLED : MF_GRAYED));
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
                                            InsertMenuA(hSubMenu, 6, MF_BYPOSITION | MF_POPUP, (UINT_PTR) myGlobals.g_hMenuEncoding, Translate("ANSI Encoding"));
                                            for(i = 0; i < GetMenuItemCount(myGlobals.g_hMenuEncoding); i++)
                                                CheckMenuItem(myGlobals.g_hMenuEncoding, i, MF_BYPOSITION | MF_UNCHECKED);
                                            if(dat->codePage == CP_ACP)
                                                CheckMenuItem(myGlobals.g_hMenuEncoding, 0, MF_BYPOSITION | MF_CHECKED);
                                            else
                                                CheckMenuItem(myGlobals.g_hMenuEncoding, dat->codePage, MF_BYCOMMAND | MF_CHECKED);
                                            CheckMenuItem(hSubMenu, ID_LOG_FREEZELOG, MF_BYCOMMAND | (dat->dwEventIsShown & MWF_SHOW_SCROLLINGDISABLED ? MF_CHECKED : MF_UNCHECKED));
                                                
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
                                                case ID_EDITOR_LOADBACKGROUNDIMAGE:
                                                {
                                                    char FileName[MAX_PATH];
                                                    char Filters[512];
                                                    OPENFILENAMEA ofn={0};

                                                    CallService(MS_UTILS_GETBITMAPFILTERSTRINGS,sizeof(Filters),(LPARAM)(char*)Filters);
                                                    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
                                                    ofn.hwndOwner=0;
                                                    ofn.lpstrFile = FileName;
                                                    ofn.lpstrFilter = Filters;
                                                    ofn.nMaxFile = MAX_PATH;
                                                    ofn.nMaxFileTitle = MAX_PATH;
                                                    ofn.Flags=OFN_HIDEREADONLY;
                                                    ofn.lpstrInitialDir = ".";
                                                    *FileName = '\0';
                                                    ofn.lpstrDefExt="";
                                                    if (GetOpenFileNameA(&ofn)) {
                                                        DBWriteContactSettingString(NULL, SRMSGMOD_T, "bgimage", FileName);
                                                        LoadMsgAreaBackground();
                                                        WindowList_Broadcast(hMessageWindowList, DM_CONFIGURETOOLBAR, 0, 0);
                                                        InvalidateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, TRUE);
                                                    }
                                                    break;
                                                }
                                                case ID_EDITOR_REMOVEBACKGROUNDIMAGE:
                                                    if(myGlobals.m_hbmMsgArea != 0) {
                                                        DeleteObject(myGlobals.m_hbmMsgArea);
                                                        myGlobals.m_hbmMsgArea = 0;
                                                        WindowList_Broadcast(hMessageWindowList, DM_CONFIGURETOOLBAR, 0, 0);
                                                        InvalidateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, TRUE);
                                                        DBWriteContactSettingString(NULL, SRMSGMOD_T, "bgimage", "");
                                                    }
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
                                            dat->wOldStatus = 0;
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
                                DBWriteContactSettingDword(dat->hContact, "MetaContacts", "tabSRMM_forced", -1);
                            else
                                _DebugMessage(hwndDlg, dat, Translate("Unforce failed"));
                        }
                        else {
                            if(CallService(MS_MC_FORCESENDCONTACTNUM, (WPARAM)dat->hContact,  (LPARAM)(iSelection - 100)) == 0)
                                DBWriteContactSettingDword(dat->hContact, "MetaContacts", "tabSRMM_forced", (DWORD)(iSelection - 100));
                            else
                                _DebugMessage(hwndDlg, dat, Translate("The selected protocol cannot be forced at this time"));
                        }
                    }
                    else if(iSelection >= 1000) {                        // the "default" menu...
                        CallService(MS_MC_SETDEFAULTCONTACTNUM, (WPARAM)dat->hContact, (LPARAM)(iSelection - 1000));
                    }
                    DestroyMenu(hMC);
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
                    if(sendJobs[iFound].sendCount > 1) {         // multisend is different...
                        char szErrMsg[256];
                        mir_snprintf(szErrMsg, sizeof(szErrMsg), "Multisend: failed sending to: %s", dat->szNickname);
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
#endif
                dbei.pBlob = (PBYTE) sendJobs[iFound].sendBuffer;
                hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) sendJobs[iFound].hContact[i], (LPARAM) & dbei);
                if(!nen_options.iNoSounds && !(dat->pContainer->dwFlags & CNT_NOSOUND))
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
                break;
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
            ActivateExistingTab(dat->pContainer, hwndDlg);
            break;
            /*
             * sent by the select container dialog box when a container was selected...
             * lParam = (TCHAR *)selected name...
             */
        case DM_CONTAINERSELECTED:
            {
                struct ContainerWindowData *pNewContainer = 0;
                TCHAR *szNewName = (TCHAR *)lParam;
                int iOldItems = TabCtrl_GetItemCount(hwndTab);
                if (!_tcsncmp(dat->pContainer->szName, szNewName, CONTAINER_NAMELEN))
                    break;
                pNewContainer = FindContainerByName(szNewName);
                if (pNewContainer == NULL)
                    pNewContainer = CreateContainer(szNewName, FALSE, dat->hContact);
#if defined (_UNICODE)
                _DBWriteContactSettingWString(dat->hContact, SRMSGMOD_T, "containerW", szNewName);
#else
                DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "container", szNewName);
#endif                
                PostMessage(myGlobals.g_hwndHotkeyHandler, DM_DOCREATETAB, (WPARAM)pNewContainer, (LPARAM)dat->hContact);
                if (iOldItems > 1)                // there were more than 1 tab, container is still valid
                    SendMessage(dat->pContainer->hwndActive, WM_SIZE, 0, 0);
                SetForegroundWindow(pNewContainer->hwnd);
                SetActiveWindow(pNewContainer->hwnd);
                break;
            }
        /*
         * sent by the hotkeyhandler when it detects a changed user picture
         * for our hContact by anyone who uses the "Photo" page of the mTooltip
         * plugin.
         */
        case DM_PICTURECHANGED:
            ShowPicture(hwndDlg, dat, TRUE, TRUE);
            return 0;

        case DM_STATUSBARCHANGED:
            UpdateStatusBar(hwndDlg, dat);
            break;
        case DM_MULTISENDTHREADCOMPLETE:
            if(dat->hMultiSendThread) {
                CloseHandle(dat->hMultiSendThread);
                dat->hMultiSendThread = 0;
                _DebugPopup(dat->hContact, "multisend thread has ended");
            }
            break;
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
                break;
            }
                
        /*
         * force a reload of the avatar by using the PS_GETAVATARINFO proto service
         */
        case DM_RETRIEVEAVATAR:
        {
            PROTO_AVATAR_INFORMATION pai_s;
            int pCaps = 0;
            int result;
            
            SetWindowLong(hwndDlg, DWL_MSGRESULT, 0);
            if(dat->szProto) {
                if(dat->wStatus == ID_STATUS_OFFLINE)
                    return 0;
                pCaps = CallProtoService(dat->szProto, PS_GETCAPS, PFLAGNUM_4, 0);
                if(!(pCaps & PF4_AVATARS))
                    return 0;
                
                ZeroMemory((void *)&pai_s, sizeof(pai_s));
                pai_s.cbSize = sizeof(pai_s);
                pai_s.hContact = dat->hContact;
                pai_s.format = PA_FORMAT_UNKNOWN;
                strcpy(pai_s.filename, "X");
                result = CallProtoService(dat->szProto, PS_GETAVATARINFO, GAIF_FORCE, (LPARAM)&pai_s);
                if(result == GAIR_WAITFOR) {
                    _DebugPopup(dat->hContact, "Refreshing avatar...");
                }
                else if(result == GAIR_SUCCESS) {
                    if(!DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "noremoteavatar", 0)) {
                        DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "MOD_Pic", pai_s.filename);
                        DBWriteContactSettingString(dat->hContact, "ContactPhoto", "File",pai_s.filename);
                        ShowPicture(hwndDlg, dat, FALSE, TRUE);
                    }
                }
                SetWindowLong(hwndDlg, DWL_MSGRESULT, 1);
            }
            return 1;
        }
        /*
         * broadcast to change the OWN avatar...
         * proto is in wParam (char *)
         * meta-contact aware
         */
        case DM_CHANGELOCALAVATAR:
        {
            if(wParam == 0 || strcmp((char *)wParam, dat->bIsMeta ? dat->szMetaProto : dat->szProto))
                break;                  // don't match
            LoadOwnAvatar(hwndDlg, dat);
            break;
        }
        /*
         * broadcasted when GLOBAL info panel setting changes
         */
        case DM_SETINFOPANEL:
            dat->dwEventIsShown = GetInfoPanelSetting(hwndDlg, dat) ? dat->dwEventIsShown | MWF_SHOW_INFOPANEL : dat->dwEventIsShown & ~MWF_SHOW_INFOPANEL;
            ShowHideInfoPanel(hwndDlg, dat);
            break;

        /*
         * show the balloon tooltip control.
         * wParam == id of the "anchor" element, defaults to the panel status field (for away msg retrieval)
         * lParam == new text to show
         */
            
        case DM_ACTIVATETOOLTIP:
        {
            if(dat->hwndTip && dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
                RECT rc;
                char szTitle[256];
#if defined(_UNICODE)
                const wchar_t *szTitleW;
#endif                
                UINT id = wParam;

                if(IsIconic(dat->pContainer->hwnd) || dat->pContainer->bInTray || dat->pContainer->hwndActive != hwndDlg)
                    break;
                
                if(id == 0)
                    id = IDC_PANELSTATUS;
                GetWindowRect(GetDlgItem(hwndDlg, id), &rc);
                SendMessage(dat->hwndTip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(rc.left, rc.bottom));
                if(lParam) {
                    dat->ti.lpszText = (char *)lParam;
                    SendMessageA(dat->hwndTip, TTM_UPDATETIPTEXTA, 0, (LPARAM)&dat->ti);
                }
                SendMessage(dat->hwndTip, TTM_SETMAXTIPWIDTH, 0, 350);
#if defined(_UNICODE)
                switch(id) {
                    case IDC_PANELNICK:
                        mir_snprintf(szTitle, sizeof(szTitle), Translate("%s has set an extended status"), dat->szNickname);
                        break;
                    default:
                        mir_snprintf(szTitle, sizeof(szTitle), Translate("Status message for %s (%s)"), "%nick%", dat->szStatus);
                }
                szTitleW = EncodeWithNickname(szTitle, dat->szNickname, dat->codePage);
                SendMessage(dat->hwndTip, TTM_SETTITLEW, 1, (LPARAM)szTitleW);
#else
                switch(id) {
                    case IDC_PANELNICK:
                        mir_snprintf(szTitle, sizeof(szTitle), Translate("%s has set an extended status"), dat->szNickname);
                        break;
                    default:
                        mir_snprintf(szTitle, sizeof(szTitle), Translate("Status message for %s (%s)"), dat->szNickname, dat->szStatus);
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
            
            if(dat->hwndLog != 0) {
                IEVIEWEVENT event = {0};

                event.cbSize = sizeof(IEVIEWEVENT);
                event.hwnd = dat->hwndLog;
                event.hContact = dat->hContact;
                event.iType = IEE_SAVE_DOCUMENT;
                event.dwFlags = 0;
                event.count = 0;
                event.codepage = 0;
                CallService(MS_IEVIEW_EVENT, 0, (LPARAM)&event);
            }
            else {
                strncpy(szFilter, "Rich Edit file\0*.rtf", MAX_PATH);
                strncpy(szFilename, dat->szNickname, MAX_PATH);
                strncat(szFilename, ".rtf", MAX_PATH);
                ofn.lStructSize=sizeof(ofn);
                ofn.hwndOwner=hwndDlg;
                ofn.lpstrFile = szFilename;
                ofn.lpstrFilter = szFilter;
                ofn.lpstrInitialDir = "rtflogs";
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
            if((isForced = DBGetContactSettingDword(dat->hContact, "MetaContacts", "tabSRMM_forced", -1)) >= 0) {
                char szTemp[64];
                mir_snprintf(szTemp, sizeof(szTemp), "Status%d", isForced);
                if(DBGetContactSettingWord(dat->hContact, "MetaContacts", szTemp, 0) == ID_STATUS_OFFLINE) {
                    _DebugPopup(dat->hContact, Translate("MetaContact: The enforced protocol (%d) is now offline.\nReverting to default protocol selection."), isForced);
                    CallService(MS_MC_UNFORCESENDCONTACT, (WPARAM)dat->hContact, 0);
                    DBWriteContactSettingDword(dat->hContact, "MetaContacts", "tabSRMM_forced", -1);
                }
            }
            szProto = GetCurrentMetaContactProto(hwndDlg, dat);

            if(dat->pContainer->hwndActive == hwndDlg && dat->pContainer->hwndStatus != 0)
                UpdateStatusBarTooltips(hwndDlg, dat, -1);
            SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
            break;
        }
      case DM_SECURE_CHANGED:
         UpdateStatusBar(hwndDlg, dat);
         break;
        case WM_INPUTLANGCHANGE:
            return DefWindowProc(hwndDlg, WM_INPUTLANGCHANGE, wParam, lParam);

       case DM_GETWINDOWSTATE:
          {
             UINT state = 0;
             
             state |= MSG_WINDOW_STATE_EXISTS;
             if (IsWindowVisible(hwndDlg)) 
                state |= MSG_WINDOW_STATE_VISIBLE;
             if (GetForegroundWindow() == dat->pContainer->hwnd) 
                state |= MSG_WINDOW_STATE_FOCUS;
             if (IsIconic(dat->pContainer->hwnd))
                state |= MSG_WINDOW_STATE_ICONIC;
             return state;
          }
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
                    SendMessage(dat->pContainer->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                    return TRUE;
                }
                
                if(dat->iOpenJobs > 0 && lParam != 2)
                    return TRUE;
                
                if(!lParam) {
                    if (myGlobals.m_WarnOnClose) {
                        if (MessageBoxA(dat->pContainer->hwnd, Translate(szWarnClose), "Miranda", MB_YESNO | MB_ICONQUESTION) == IDNO) {
                            return TRUE;
                        }
                    }
                }
                iTabs = TabCtrl_GetItemCount(hwndTab);
                if(iTabs == 1) {
                    PostMessage(GetParent(GetParent(hwndDlg)), WM_CLOSE, 0, 1);
                    return 1;
                }
                    
                dat->pContainer->iChilds--;
                i = GetTabIndexFromHWND(hwndTab, hwndDlg);
                
                /*
                 * after closing a tab, we need to activate the tab to the left side of
                 * the previously open tab.
                 * normally, this tab has the same index after the deletion of the formerly active tab
                 * unless, of course, we closed the last (rightmost) tab.
                 */
                if (!dat->pContainer->bDontSmartClose && iTabs > 1) {
                    if (i == iTabs - 1)
                        i--;
                    else
                        i++;
                    TabCtrl_SetCurSel(hwndTab, i);
                    item.mask = TCIF_PARAM;
                    TabCtrl_GetItem(hwndTab, i, &item);         // retrieve dialog hwnd for the now active tab...
    
                    dat->pContainer->hwndActive = (HWND) item.lParam;
                    SendMessage(dat->pContainer->hwnd, DM_QUERYCLIENTAREA, 0, (LPARAM)&rc);
                    SetWindowPos(dat->pContainer->hwndActive, HWND_TOP, rc.left, rc.top, (rc.right - rc.left), (rc.bottom - rc.top), SWP_SHOWWINDOW);
                    ShowWindow((HWND)item.lParam, SW_SHOW);
                    SetForegroundWindow(dat->pContainer->hwndActive);
                    SetFocus(dat->pContainer->hwndActive);
                    SendMessage(dat->pContainer->hwnd, WM_SIZE, 0, 0);
                }
                DestroyWindow(hwndDlg);
                if(iTabs == 1)
                    PostMessage(GetParent(GetParent(hwndDlg)), WM_CLOSE, 0, 1);
                else
                    SendMessage(pContainer->hwnd, WM_SIZE, 0, 0);
                break;
            }
        case WM_DESTROY:
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
            if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON)
                NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
            
            if (dat->hBkgBrush)
                DeleteObject(dat->hBkgBrush);
            if (dat->hInputBkgBrush)
                DeleteObject(dat->hInputBkgBrush);
            if (dat->sendBuffer != NULL)
                free(dat->sendBuffer);
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
                        _DebugPopup(dat->hContact, "SendQueue: Clearing orphaned send job (%d)", i);
                        ClearSendJob(i);
                    }
                }
                if(dat->hQueuedEvents)
                    free(dat->hQueuedEvents);
            }
            
            if (dat->hContactPic && dat->hContactPic != myGlobals.g_hbmUnknown)
                DeleteObject(dat->hContactPic);

            if (dat->hOwnPic && dat->hOwnPic != myGlobals.g_hbmUnknown)
                DeleteObject(dat->hOwnPic);

            if (dat->hbmMsgArea && dat->hbmMsgArea != myGlobals.m_hbmMsgArea)
                DeleteObject(dat->hbmMsgArea);
            
            if (dat->hSmileyIcon)
                DeleteObject(dat->hSmileyIcon);

            if (dat->hwndTip)
                DestroyWindow(dat->hwndTip);
            
            UpdateTrayMenuState(dat, FALSE);               // remove me from the tray menu (if still there)
            DeleteMenu(myGlobals.g_hMenuTrayUnread, (UINT_PTR)dat->hContact, MF_BYCOMMAND);
            WindowList_Remove(hMessageWindowList, hwndDlg);
            SendMessage(hwndDlg, DM_SAVEPERCONTACT, 0, 0);
            if(myGlobals.m_SplitterSaveOnClose)
                SaveSplitter(hwndDlg, dat);
            if(!dat->stats.bWritten) {
                WriteStatsOnClose(hwndDlg, dat);
                dat->stats.bWritten = TRUE;
            }

            if(dat->ltr_templates != (TemplateSet *)&LTR_Active)
                free(dat->ltr_templates);
            if(dat->rtl_templates != (TemplateSet *)&RTL_Active)
                free(dat->rtl_templates);
            
            SetWindowLong(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), GWL_WNDPROC, (LONG) OldSplitterProc);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_PANELSPLITTER), GWL_WNDPROC, (LONG) OldSplitterProc);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_WNDPROC, (LONG) OldSplitterProc);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_WNDPROC, (LONG) OldMessageEditProc);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_CONTACTPIC), GWL_WNDPROC, (LONG) OldAvatarWndProc);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_PANELPIC), GWL_WNDPROC, (LONG) OldAvatarWndProc);
            // remove temporary contacts...
            
            if (dat->hContact && DBGetContactSettingByte(NULL, SRMSGMOD_T, "deletetemp", 0)) {
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
                    BroadCastContainer(dat->pContainer, DM_REFRESHTABINDEX, 0, 0);
                    dat->iTabID = -1;
                }
            }
            TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_CLOSE, 0);

            if(dat->hContact == myGlobals.hLastOpenedContact)
                myGlobals.hLastOpenedContact = 0;

            // IEVIew MOD Begin
            if (dat->hwndLog != 0) {
                IEVIEWWINDOW ieWindow;
                ieWindow.cbSize = sizeof(IEVIEWWINDOW);
                ieWindow.iType = IEW_DESTROY;
                ieWindow.hwnd = dat->hwndLog;
                CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
            }
            // IEVIew MOD End

            if (dat)
                free(dat);
            SetWindowLong(hwndDlg, GWL_USERDATA, 0);
            break;
    }
    return FALSE;
}

/*
 * flash a tab icon if mode = true, otherwise restore default icon
 * store flashing state into bState
 */

static void FlashTab(struct MessageWindowData *dat, HWND hwndTab, int iTabindex, BOOL *bState, BOOL mode, HICON origImage)
{
    TCITEM item;

    ZeroMemory((void *)&item, sizeof(item));
    item.mask = TCIF_IMAGE;

    if (mode)
        *bState = !(*bState);
    else
        dat->hTabIcon = origImage;
    item.iImage = 0;
    TabCtrl_SetItem(hwndTab, iTabindex, &item);
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

