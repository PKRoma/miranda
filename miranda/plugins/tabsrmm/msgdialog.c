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
#include "m_message.h"
#include "m_popup.h"
#include "m_smileyadd.h"
#include "m_metacontacts.h"

#include "sendqueue.h"
#include "msgdlgutils.h"

#ifdef __GNUWIN32__
#define SES_EXTENDBACKCOLOR 4           // missing from the mingw32 headers
#endif

#define TOOLBAR_PROTO_HIDDEN 1
#define TOOLBAR_SEND_HIDDEN 2

extern MYGLOBALS myGlobals;

int GetTabIndexFromHWND(HWND hwndTab, HWND hwndDlg);
int ActivateTabFromHWND(HWND hwndTab, HWND hwndDlg);
int CutContactName(char *old, char *new, int size);
TCHAR *QuoteText(TCHAR *text, int charsPerLine, int removeExistingQuotes);
DWORD _DBGetContactSettingDword(HANDLE hContact, char *pszSetting, char *pszValue, DWORD dwDefault, int iIgnoreMode);
void _DBWriteContactSettingWString(HANDLE hContact, const char *szKey, const char *szSetting, const wchar_t *value);
int MessageWindowOpened(WPARAM wParam, LPARAM LPARAM);
static DWORD CALLBACK StreamOut(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb);

char *pszIDCSAVE_close = 0, *pszIDCSAVE_save = 0;

static void FlashTab(HWND hwndTab, int iTabindex, BOOL *bState, BOOL mode, int flashImage, int origImage);
void FlashContainer(struct ContainerWindowData *pContainer, int iMode, int iCount);
void ReflashContainer(struct ContainerWindowData *pContainer);
void UpdateContainerMenu(HWND hwndDlg, struct MessageWindowData *dat);

extern HANDLE hMessageWindowList;
extern struct CREOleCallback reOleCallback;
extern HINSTANCE g_hInst;

void ImageDataInsertBitmap(IRichEditOle *ole, HBITMAP hbm);

#if defined(_STREAMTHREADING)
    #define NR_STREAMJOBS 100
    
    extern HANDLE g_hStreamThread;
    extern int g_StreamThreadRunning;
    struct StreamJob StreamJobs[NR_STREAMJOBS + 2];
    int volatile g_StreamJobCurrent = 0;
    CRITICAL_SECTION sjcs;
    void ReplaceIcons(HWND hwndDlg, struct MessageWindowData *dat, LONG startAt, int fAppend);
#endif

extern char *szWarnClose;

void TABSRMM_FireEvent(HANDLE hContact, HWND hwndDlg, unsigned int type, unsigned int subType);
struct ContainerWindowData *FindContainerByName(const TCHAR *name);
int GetContainerNameForContact(HANDLE hContact, TCHAR *szName, int iNameLen);
struct ContainerWindowData *CreateContainer(const TCHAR *name, int iMode, HANDLE hContactFrom);
HWND CreateNewTabForContact(struct ContainerWindowData *pContainer, HANDLE hContact, int isSend, const char *pszInitialText, BOOL bActivateTAb, BOOL bPopupContainer);

static WNDPROC OldMessageEditProc, OldSplitterProc;
static const UINT infoLineControls[] = { IDC_PROTOCOL, IDC_PROTOMENU, IDC_NAME};
static const UINT buttonLineControlsNew[] = { IDC_PIC, IDC_HISTORY, IDC_TIME, IDC_QUOTE, IDC_SAVE, IDC_SENDMENU};
static const UINT sendControls[] = { IDC_MESSAGE, IDC_LOG };
static const UINT formatControls[] = { IDC_SMILEYBTN, IDC_FONTBOLD, IDC_FONTITALIC, IDC_FONTUNDERLINE }; //, IDC_FONTFACE, IDC_FONTCOLOR};
static const UINT controlsToHide[] = { IDOK, IDC_PIC, IDC_PROTOCOL, IDC_FONTUNDERLINE, IDC_HISTORY, -1 };
static const UINT controlsToHide1[] = { IDOK, IDC_FONTUNDERLINE, IDC_FONTITALIC, IDC_FONTBOLD, IDC_PROTOCOL, -1 };
static const UINT controlsToHide2[] = { IDOK, IDC_PIC, IDC_PROTOCOL, -1};
static const UINT addControls[] = { IDC_ADDICON, IDC_ADDTEXT, IDC_ADD, IDC_CANCELADD };

const UINT errorControls[] = { IDC_STATICERRORICON, IDC_STATICTEXT, IDC_RETRY, IDC_CANCELSEND, IDC_MSGSENDLATER };

struct SendJob sendJobs[NR_SENDJOBS];

static int splitterEdges = TRUE;

#if defined(_STREAMTHREADING)

/*
 * thread used for dispatching icon and smiley replacements. These are time consuming and would block
 * the main thread when opening multiple window with lots of history items.
 */

DWORD WINAPI StreamThread(LPVOID param)
{
    do {
        if(g_StreamJobCurrent == 0) {
            SuspendThread(g_hStreamThread);                 // nothing to do...
            continue;
        }
        ReplaceIcons(StreamJobs[0].hwndOwner, StreamJobs[0].dat, StreamJobs[0].startAt, StreamJobs[0].fAppend);
        EnterCriticalSection(&sjcs);
        StreamJobs[0].dat->pendingStream--;
        StreamJobs[0].dat->pContainer->pendingStream--;
        MoveMemory(&StreamJobs[0], &StreamJobs[1], (g_StreamJobCurrent - 1) * sizeof(StreamJobs[0]));
        g_StreamJobCurrent--;
        LeaveCriticalSection(&sjcs);
    } while ( g_StreamThreadRunning );
	return 0;
}

#endif

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

void ShowMultipleControls(HWND hwndDlg, const UINT * controls, int cControls, int state)
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
            ShowMultipleControls(hwndDlg, addControls, 4, SW_SHOW);
        }
        else {
            ShowMultipleControls(hwndDlg, addControls, 4, SW_HIDE);
            dat->bNotOnList = FALSE;
        }
    } else {
        ShowMultipleControls(hwndDlg, buttonLineControlsNew, sizeof(buttonLineControlsNew) / sizeof(buttonLineControlsNew[0]), SW_HIDE);
        ShowMultipleControls(hwndDlg, infoLineControls, sizeof(infoLineControls) / sizeof(infoLineControls[0]), SW_HIDE);
        ShowMultipleControls(hwndDlg, addControls, 4, SW_HIDE);
    }

    ShowWindow(GetDlgItem(hwndDlg, IDC_MULTIPLEICON), SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_TOGGLETOOLBAR), SW_HIDE);
    
// IEVIew MOD Begin
    EnableWindow(GetDlgItem(hwndDlg, IDC_TIME), TRUE);
    
	if (dat->hwndLog) {
		ShowWindow (GetDlgItem(hwndDlg, IDC_LOG), SW_HIDE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_LOG), FALSE);
		ShowWindow (GetDlgItem(hwndDlg, IDC_MESSAGE), SW_SHOW);
        //if(DBGetContactSettingDword(NULL, "IEVIEW", "TemplatesFlags", 0) & 0x01)
        //    EnableWindow(GetDlgItem(hwndDlg, IDC_TIME), FALSE);
            
	} else
		ShowMultipleControls(hwndDlg, sendControls, sizeof(sendControls) / sizeof(sendControls[0]), SW_SHOW);
// IEVIew MOD End
    ShowMultipleControls(hwndDlg, errorControls, sizeof(errorControls) / sizeof(errorControls[0]), dat->dwFlags & MWF_ERRORSTATE ? SW_SHOW : SW_HIDE);

    if(!myGlobals.m_SendFormat)
        ShowMultipleControls(hwndDlg, &formatControls[1], 3, SW_HIDE);
    
// smileybutton stuff...
    
    if(myGlobals.g_SmileyAddAvail && myGlobals.m_SmileyPluginEnabled && dat->hwndLog == 0) {
        nrSmileys = CheckValidSmileyPack(dat->szProto, &hButtonIcon);

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
    
    if(nrSmileys == 0)
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

    ShowWindow(GetDlgItem(hwndDlg, IDC_TYPINGNOTIFY), SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_SPLITTER), SW_SHOW);
    ShowWindow(GetDlgItem(hwndDlg, IDOK), showToolbar ? SW_SHOW : SW_HIDE);

    ShowWindow(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), (dat->sendMode & SMODE_MULTIPLE) ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_CLIST), (dat->sendMode & SMODE_MULTIPLE) ? SW_SHOW : SW_HIDE);
    
    EnableWindow(GetDlgItem(hwndDlg, IDOK), GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE)) != 0);
    SendMessage(hwndDlg, DM_UPDATETITLE, 0, 1);
    SendMessage(hwndDlg, WM_SIZE, 0, 0);

    EnableWindow(GetDlgItem(hwndDlg, IDC_CONTACTPIC), FALSE);
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
            if (wParam == 23 && GetKeyState(VK_CONTROL) & 0x8000) {             // ctrl-w close tab
                SendMessage(GetParent(hwnd), WM_CLOSE, 1, 0);
                return 0;
            }
            if (wParam == 18 && GetKeyState(VK_CONTROL) & 0x8000) {             // ctrl-r most recent unread
                SendMessage(GetParent(hwnd), DM_QUERYPENDING, DM_QUERY_MOSTRECENT, 0);
                return 0;
            }
            if (wParam == 0x0c && GetKeyState(VK_CONTROL) & 0x8000) {
                SendMessage(GetParent(hwnd), WM_COMMAND, IDM_CLEAR, 0);         // ctrl-l (clear log)
                return 0;
            }
            if (wParam == 0x0f && GetKeyState(VK_CONTROL) & 0x8000) {
                CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CONTAINEROPTIONS), GetParent(hwnd), DlgProcContainerOptions, (LPARAM) mwdat->pContainer);
                return 0;
            }
            if((GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000)) {
                switch(wParam) {
                    case 0x02:               // bold
                        CheckDlgButton(GetParent(hwnd), IDC_FONTBOLD, IsDlgButtonChecked(GetParent(hwnd), IDC_FONTBOLD) == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED);
                        SendMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(IDC_FONTBOLD, IDC_MESSAGE), 0);
                        return 0;
                    case 0x09:
                        CheckDlgButton(GetParent(hwnd), IDC_FONTITALIC, IsDlgButtonChecked(GetParent(hwnd), IDC_FONTITALIC) == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED);
                        SendMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(IDC_FONTITALIC, IDC_MESSAGE), 0);
                        return 0;
                    case 21:
                        CheckDlgButton(GetParent(hwnd), IDC_FONTUNDERLINE, IsDlgButtonChecked(GetParent(hwnd), IDC_FONTUNDERLINE) == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED);
                        SendMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(IDC_FONTUNDERLINE, IDC_MESSAGE), 0);
                        return 0;
                    case 19:
                        PostMessage(GetParent(hwnd), WM_COMMAND, IDC_SENDMENU, (LPARAM)GetDlgItem(GetParent(hwnd), IDC_SENDMENU));
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
            if(wParam == VK_INSERT && (GetKeyState(VK_SHIFT) & 0x8000)) {
                SendMessage(hwnd, EM_PASTESPECIAL, CF_TEXT, 0);
                return 0;
            }
            if ((GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000)) {
                if (wParam == 0x9) {            // ctrl-shift tab
                    SendMessage(GetParent(hwnd), DM_SELECTTAB, DM_SELECT_PREV, 0);
                    return 0;
                }
            }
            if ((GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000)) {
                if (wParam == 'V') {
                    SendMessage(hwnd, EM_PASTESPECIAL, CF_TEXT, 0);
                    return 0;
                }
                if (wParam == VK_TAB) {
                    SendMessage(GetParent(hwnd), DM_SELECTTAB, DM_SELECT_NEXT, 0);
                    return 0;
                }
                if (wParam == VK_F4) {
                    SendMessage(GetParent(hwnd), WM_CLOSE, 1, 0);
                    return 0;
                }
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
                                }
                            }
                            else {
                                if(mwdat->history[mwdat->iHistoryCurrent].szText != NULL) {
                                    SetWindowText(hwnd, _T(""));
                                    SendMessage(hwnd, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM) mwdat->history[mwdat->iHistoryCurrent].szText);
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
            if ((GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000)) {
                switch (wParam) {
                    case VK_UP:
                    case VK_DOWN:
                    case VK_PRIOR:
                    case VK_NEXT:
                    case VK_HOME:
                    case VK_END:
                        {
                            SCROLLINFO si = {0};
                            
                            si.cbSize = sizeof(si);
                            si.fMask = SIF_PAGE | SIF_RANGE;
                            GetScrollInfo(GetDlgItem(GetParent(hwnd), IDC_LOG), SB_VERT, &si);
                            si.fMask = SIF_POS;
                            si.nPos = si.nMax - si.nPage + 1;
                            SetScrollInfo(GetDlgItem(GetParent(hwnd), IDC_LOG), SB_VERT, &si, TRUE);
                            if (myGlobals.m_MsgLogHotkeys) {
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

                                PostMessage(GetDlgItem(GetParent(hwnd), IDC_LOG), WM_VSCROLL, wp, 0);
                                return 0;
                            } else
                                break;
                        }
                }
            }
            if (wParam == VK_RETURN)
                break;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_MOUSEWHEEL:
        case WM_KILLFOCUS:
            break;
        case WM_SYSCHAR:
            if(VkKeyScan((TCHAR)wParam) == 'S' && GetKeyState(VK_MENU) & 0x8000) {
                if (!(GetWindowLong(hwnd, GWL_STYLE) & ES_READONLY)) {
                    PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
                    return 0;
                }
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
            break;
        case WM_SYSKEYDOWN:
            if(wParam == VK_LEFT && GetKeyState(VK_MENU) & 0x8000) {
                SendMessage(GetParent(hwnd), DM_SELECTTAB, DM_SELECT_PREV, 0);
                return 0;
            }
            if(wParam == VK_RIGHT && GetKeyState(VK_MENU) & 0x8000) {
                SendMessage(GetParent(hwnd), DM_SELECTTAB, DM_SELECT_NEXT, 0);
                return 0;
            }
            break;
        case WM_INPUTLANGCHANGEREQUEST: {
                if (myGlobals.m_AutoLocaleSupport) {
                    SendMessage(GetParent(hwnd), DM_SETLOCALE, wParam, lParam);
                    PostMessage(GetParent(hwnd), DM_SAVELOCALE, 0, 0);
                    return DefWindowProc(hwnd, WM_INPUTLANGCHANGEREQUEST, wParam, lParam);
                }
                break;
            }
        case WM_INPUTLANGCHANGE:
            return DefWindowProc(hwnd, WM_INPUTLANGCHANGE, wParam, lParam);
    }
    return CallWindowProc(OldMessageEditProc, hwnd, msg, wParam, lParam);
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
                OffsetRect(&urc->rcItem, -(rcButton.right - rcButton.left), 0);
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
                OffsetRect(&urc->rcItem, -(rcButton.right - rcButton.left), 0);
            return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
        case IDC_PROTOCOL:
        case IDC_PROTOMENU:
        case IDC_PIC:
        case IDC_USERMENU:
        case IDC_DETAILS:
        case IDC_HISTORY:
        case IDC_TIME:
        case IDC_QUOTE:
        case IDC_SAVE:
        case IDC_RETRY:
        case IDC_CANCELSEND:
        case IDC_MSGSENDLATER:
        case IDC_STATICTEXT:
        case IDC_STATICERRORICON:
            if(urc->wId == IDC_RETRY || urc->wId == IDC_CANCELSEND || urc->wId == IDC_MSGSENDLATER || urc->wId == IDC_STATICTEXT || urc->wId == IDC_STATICERRORICON)
                return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;

            if(urc->wId != IDC_PROTOCOL && urc->wId != IDC_PROTOMENU)
                OffsetRect(&urc->rcItem, 12, 0);
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if(dat->controlsHidden & TOOLBAR_PROTO_HIDDEN && urc->wId == IDC_PROTOMENU)
                OffsetRect(&urc->rcItem, -(rcButton.right - rcButton.left), 0);
            if (urc->wId == IDC_PROTOCOL || urc->wId == IDC_PROTOMENU)
                return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
            if (showToolbar && !(dat->controlsHidden & TOOLBAR_SEND_HIDDEN)) {
                urc->rcItem.left -= 40;
                urc->rcItem.right -= 40;
            }
            if (dat->showPic && (dat->splitterY <= (dat->bottomOffset + (dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC ? 33 : 26))))
                OffsetRect(&urc->rcItem, -(dat->pic.cx + 2), 0);

            return RD_ANCHORX_RIGHT | RD_ANCHORY_BOTTOM;
        case IDC_MULTIPLEICON:
            return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
        case IDC_ADDTEXT:
        case IDC_ADDICON:
        case IDC_ADD:
        case IDC_CANCELADD:
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if(!showToolbar) {
                urc->rcItem.bottom += 24;
                urc->rcItem.top += 24;
            }
            if(!splitterEdges)
                OffsetRect(&urc->rcItem, 0, 2);
            return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
        case IDC_LOG:
            if(dat->dwFlags & MWF_ERRORSTATE)
                urc->rcItem.top += 38;
            if (dat->sendMode & SMODE_MULTIPLE)
                urc->rcItem.right -= (dat->multiSplitterX + 3);
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if (!showToolbar)
                urc->rcItem.bottom += (splitterEdges ? 24 : 26);
            if(dat->bNotOnList)
                urc->rcItem.bottom -= 24;
            if(!splitterEdges)
                urc->rcItem.bottom += 2;
            return RD_ANCHORX_WIDTH | RD_ANCHORY_HEIGHT;
        case IDC_SPLITTER:
        case IDC_SPLITTER5:
            urc->rcItem.right +=1;
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if (urc->wId == IDC_SPLITTER && dat->splitterY <= (dat->bottomOffset + (dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC ? 33 : 26)) && dat->showPic && showToolbar)
                urc->rcItem.right -= (dat->pic.cx + 2);
            if(dat->bNotOnList && urc->wId == IDC_SPLITTER5)
                OffsetRect(&urc->rcItem, 0, -24);
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
            return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
        case IDOK:
        case IDC_SENDMENU:
            OffsetRect(&urc->rcItem, 12, 0);
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;

            if (dat->showPic && (dat->splitterY <= (dat->bottomOffset + (dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC ? 33 : 26))))
                OffsetRect(&urc->rcItem, -(dat->pic.cx + 2), 0);
            
            return RD_ANCHORX_RIGHT | RD_ANCHORY_BOTTOM;
        case IDC_MULTISPLITTER:
            urc->rcItem.left -= dat->multiSplitterX;
            urc->rcItem.right -= dat->multiSplitterX;
            urc->rcItem.bottom = iClistOffset;
            if(!splitterEdges)
                urc->rcItem.bottom += 2;
            return RD_ANCHORX_RIGHT | RD_ANCHORY_CUSTOM;
        case IDC_CLIST:
            urc->rcItem.left = urc->dlgNewSize.cx - dat->multiSplitterX;
            urc->rcItem.right = urc->dlgNewSize.cx - 3;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if (!showToolbar)
                urc->rcItem.bottom += (splitterEdges ? 24 : 26);
            if(dat->bNotOnList)
                urc->rcItem.bottom -= 24;
            if(!splitterEdges)
                urc->rcItem.bottom += 2;
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_HEIGHT;
        case IDC_TYPINGNOTIFY:
            return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
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
    
    if (dat != 0)
        dat->iTabID = GetTabIndexFromHWND(hwndTab, hwndDlg);
    else {
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


                pszIDCSAVE_close = Translate("Close session");
                pszIDCSAVE_save = Translate("Save and close session");

                dat->hContact = newData->hContact;
                WindowList_Add(hMessageWindowList, hwndDlg, dat->hContact);
                dat->szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)dat->hContact, 0);
                dat->bIsMeta = IsMetaContact(hwndDlg, dat) ? TRUE : FALSE;
                if(dat->hContact && dat->szProto != NULL)
                    dat->wStatus = DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE);
                else
                    dat->wStatus = ID_STATUS_OFFLINE;
                
                GetContactUIN(hwndDlg, dat);
                dat->showUIElements = dat->pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;
                dat->sendMode |= DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "forceansi", 0) ? SMODE_FORCEANSI : 0;
                dat->sendMode |= dat->hContact == 0 ? SMODE_MULTIPLE : 0;
                
                dat->hwnd = hwndDlg;
                SetMessageLog(hwndDlg, dat);

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
                
                SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
                dat->bTabFlash = FALSE;
                dat->mayFlashTab = FALSE;
                dat->iTabImage = newData->iTabImage;
                
                dat->multiSplitterX = (int) DBGetContactSettingDword(NULL, SRMSGMOD, "multisplit", 150);
                dat->showTypingWin = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGWIN, SRMSGDEFSET_SHOWTYPINGWIN);
                dat->nTypeMode = PROTOTYPE_SELFTYPING_OFF;
                SetTimer(hwndDlg, TIMERID_TYPE, 1000, NULL);
                dat->iLastEventType = 0xffffffff;
                
                // load log option flags...
                dat->dwFlags = (DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT) & MWF_LOG_ALL);

                if(!myGlobals.m_IgnoreContactSettings) {
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
                
                if(dat->hContact) {
                    dat->codePage = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "ANSIcodepage", CP_ACP);
                    dat->dwFlags |= (myGlobals.m_RTLDefault == 0 ? (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 0) ? MWF_LOG_RTL : 0) : (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 1) ? MWF_LOG_RTL : 0));
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
                if (dat->splitterY == -1)
                    dat->splitterY = dat->originalSplitterY + 60;

                GetWindowRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc);
                dat->minEditBoxSize.cx = rc.right - rc.left;
                dat->minEditBoxSize.cy = rc.bottom - rc.top;

                SetDlgItemTextA(hwndDlg, IDC_ADDTEXT, Translate("This contact is not on your list"));
                SetDlgItemTextA(hwndDlg, IDC_ADD, Translate("Add it"));
                SetDlgItemTextA(hwndDlg, IDC_CANCELADD, Translate("Leave it"));

                SendMessage(hwndDlg, DM_LOADBUTTONBARICONS, 0, 0);

                // translate hotkeys...

                SetDlgItemTextA(hwndDlg, IDC_SMILEYBTN, Translate("tb_&Emoticon"));
                SetDlgItemTextA(hwndDlg, IDC_HISTORY, Translate("tb_&History"));
                SetDlgItemTextA(hwndDlg, IDC_QUOTE, Translate("tb_&Quote"));
                SetDlgItemTextA(hwndDlg, IDC_PIC, Translate("tb_&Picmenu"));
                SetDlgItemTextA(hwndDlg, IDC_PROTOCOL, Translate("tb_&Details"));
                SetDlgItemTextA(hwndDlg, IDC_NAME, Translate("tb_&User"));
                SetDlgItemTextA(hwndDlg, IDC_TIME, Translate("tb_&Messagelog"));
                SetDlgItemTextA(hwndDlg, IDC_TOGGLETOOLBAR, Translate("tb_&ToggleTB"));
                
                SendDlgItemMessage(hwndDlg, IDC_FONTBOLD, BUTTONSETASPUSHBTN, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_FONTITALIC, BUTTONSETASPUSHBTN, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_FONTUNDERLINE, BUTTONSETASPUSHBTN, 0, 0);
                
                // Make them flat buttons
                if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "nlflat", 0)) {
                    for (i = 0; i < sizeof(buttonLineControlsNew) / sizeof(buttonLineControlsNew[0]); i++)
                        SendMessage(GetDlgItem(hwndDlg, buttonLineControlsNew[i]), BUTTONSETASFLATBTN, 0, 0);
                    for (i = 0; i < sizeof(errorControls) / sizeof(errorControls[0]); i++)
                        SendMessage(GetDlgItem(hwndDlg, errorControls[i]), BUTTONSETASFLATBTN, 0, 0);
                    for (i = 0; i < sizeof(formatControls) / sizeof(formatControls[0]); i++)
                        SendMessage(GetDlgItem(hwndDlg, formatControls[i]), BUTTONSETASFLATBTN, 0, 0);
                    SendMessage(GetDlgItem(hwndDlg, IDOK), BUTTONSETASFLATBTN, 0, 0);
                    SendMessage(GetDlgItem(hwndDlg, IDC_NAME), BUTTONSETASFLATBTN, 0, 0);
                    SendMessage(GetDlgItem(hwndDlg, IDC_PROTOCOL), BUTTONSETASFLATBTN, 0, 0);
                    SendMessage(GetDlgItem(hwndDlg, IDC_PROTOMENU), BUTTONSETASFLATBTN, 0, 0);
                }

                dat->dwFlags |= MWF_INITMODE;
                TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_OPENING, 0);
                
                SendMessage(GetDlgItem(hwndDlg, IDC_HISTORY), BUTTONADDTOOLTIP, (WPARAM) Translate("View User's History"), 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_TIME), BUTTONADDTOOLTIP, (WPARAM) Translate("Message Log Options"), 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_SENDMENU), BUTTONADDTOOLTIP, (WPARAM) Translate("Send Menu"), 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_QUOTE), BUTTONADDTOOLTIP, (WPARAM) Translate("Quote Text"), 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_PIC), BUTTONADDTOOLTIP, (WPARAM) Translate("Avatar Options"), 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_SMILEYBTN), BUTTONADDTOOLTIP, (WPARAM) Translate("Insert Emoticon"), 0);

                SendMessage(GetDlgItem(hwndDlg, IDC_SAVE), BUTTONADDTOOLTIP, (WPARAM) pszIDCSAVE_close, 0);
                SendMessage(GetDlgItem(hwndDlg, IDOK), BUTTONADDTOOLTIP, (WPARAM) Translate("Send message"), 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_PROTOCOL), BUTTONADDTOOLTIP, (WPARAM) Translate("View User's Details"), 0);
                SendDlgItemMessage(hwndDlg, IDC_PROTOMENU, BUTTONADDTOOLTIP, (WPARAM) Translate("Protocol Menu"), 0);
                SendDlgItemMessage(hwndDlg, IDC_FONTBOLD, BUTTONADDTOOLTIP, (WPARAM) Translate("Bold text"), 0);
                SendDlgItemMessage(hwndDlg, IDC_FONTITALIC, BUTTONADDTOOLTIP, (WPARAM) Translate("Italic text"), 0);
                SendDlgItemMessage(hwndDlg, IDC_FONTUNDERLINE, BUTTONADDTOOLTIP, (WPARAM) Translate("Underlined text"), 0);
                SendDlgItemMessage(hwndDlg, IDC_FONTFACE, BUTTONADDTOOLTIP, (WPARAM) Translate("Select font"), 0);
                SendDlgItemMessage(hwndDlg, IDC_FONTCOLOR, BUTTONADDTOOLTIP, (WPARAM) Translate("Select font color"), 0);

                EnableWindow(GetDlgItem(hwndDlg, IDC_TYPINGNOTIFY), FALSE);
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETOLECALLBACK, 0, (LPARAM) & reOleCallback);
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_LINK);
                SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_KEYEVENTS | ENM_CHANGE);
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETUNDOLIMIT, 0, 0);

                SetWindowTextA(GetDlgItem(hwndDlg, IDC_RETRY), Translate("&Retry"));
                SetWindowTextA(GetDlgItem(hwndDlg, IDC_CANCELSEND), Translate("&Cancel"));
                SetWindowTextA(GetDlgItem(hwndDlg, IDC_MSGSENDLATER), Translate("Send &later"));
                
                /* OnO: higligh lines to their end */
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETEDITSTYLE, SES_EXTENDBACKCOLOR, SES_EXTENDBACKCOLOR);
                
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

                if (dat->hContact)
                    FindFirstEvent(hwndDlg, dat);
                dat->stats.started = time(NULL);
                SendMessage(hwndDlg, DM_OPTIONSAPPLIED, 0, 0);
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
#if defined ( _UNICODE )
                    if(!DBGetContactSetting(dat->hContact, SRMSGMOD, "SavedMsgW", &dbv)) {
                        SETTEXTEX stx = {ST_DEFAULT, 1200};
                        if(dbv.type == DBVT_ASCIIZ && dbv.cchVal > 0) // at least the 0x0000 is always there... 
                            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)Utf8Decode(dbv.pszVal));
#else
    				if(!DBGetContactSetting(dat->hContact, SRMSGMOD, "SavedMsg", &dbv)) {
                        if(dbv.cchVal > 0)
                            SetDlgItemTextA(hwndDlg, IDC_MESSAGE, dbv.pszVal);
#endif
                        DBFreeVariant(&dbv);
    					{
    						int len = GetWindowTextLength(GetDlgItem(hwndDlg,IDC_MESSAGE));
                            if(dat->pContainer->hwndActive == hwndDlg)
                                UpdateReadChars(hwndDlg, dat);
                            UpdateSaveAndSendButton(hwndDlg, dat);
    					}
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
                /*
                 * set locale if saved to contact
                 */
                if (myGlobals.m_AutoLocaleSupport && dat->hContact != 0) {
                    DBVARIANT dbv;
                    int res;
                    char szKLName[KL_NAMELENGTH+1];
                    UINT flags = newData->iActivate ? KLF_ACTIVATE : 0;

                    res = DBGetContactSetting(dat->hContact, SRMSGMOD_T, "locale", &dbv);
                    if (res == 0 && dbv.type == DBVT_ASCIIZ) {
#if defined ( _UNICODE ) 
                        dat->hkl = LoadKeyboardLayoutA(dbv.pszVal, KLF_ACTIVATE);
#else
                        dat->hkl = LoadKeyboardLayout(dbv.pszVal, KLF_ACTIVATE);
#endif
                        if (dat->hkl && newData->iActivate)
                            PostMessage(hwndDlg, DM_SETLOCALE, 0, 0);
                        DBFreeVariant(&dbv);
                    } else {
#if defined ( _UNICODE )
                        GetKeyboardLayoutNameA(szKLName);
                        dat->hkl = LoadKeyboardLayoutA(szKLName, 0);
#else
                        GetKeyboardLayoutName(szKLName);
                        dat->hkl = LoadKeyboardLayout(szKLName, 0);
#endif
                        DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "locale", szKLName);
                    }
                }
                SendMessage(dat->pContainer->hwnd, DM_QUERYCLIENTAREA, 0, (LPARAM)&rc);
                if (newData->iActivate) {
                    if(!dat->hThread) {
                        ShowPicture(hwndDlg,dat,FALSE,TRUE, TRUE);
                        LoadSplitter(hwndDlg, dat);
                    }
                    SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
                    SetWindowPos(hwndDlg, HWND_TOP, rc.left, rc.top, (rc.right - rc.left), (rc.bottom - rc.top), 0);
                    SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                    if(IsIconic(dat->pContainer->hwnd) && !IsZoomed(dat->pContainer->hwnd) && myGlobals.m_AutoSwitchTabs) {
                        DBEVENTINFO dbei = {0};

                        dbei.flags = 0;
                        dbei.eventType = EVENTTYPE_MESSAGE;
                        dat->iFlashIcon = myGlobals.g_IconMsgEvent;
                        SetTimer(hwndDlg, TIMERID_FLASHWND, TIMEOUT_FLASHWND, NULL);
                        dat->mayFlashTab = TRUE;
                        dat->dwTickLastEvent = GetTickCount();
                        if(dat->pContainer->dwFlags & CNT_CREATE_MINIMIZED)
                            FlashOnClist(hwndDlg, dat, dat->hDbEventFirst, &dbei);
                        SendMessage(dat->pContainer->hwnd, DM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_EVENT_MESSAGE));
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
                dat->dwLastActivity = GetTickCount();
                dat->pContainer->dwLastActivity = dat->dwLastActivity;
                TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_OPEN, 0);
                if(dat->pContainer->dwFlags & CNT_CREATE_MINIMIZED) {
                    dat->pContainer->dwFlags &= ~CNT_CREATE_MINIMIZED;
                    dat->pContainer->dwFlags |= CNT_DEFERREDCONFIGURE;
                    dat->pContainer->hwndActive = hwndDlg;
                    SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                    SendMessage(dat->pContainer->hwnd, DM_RESTOREWINDOWPOS, 0, 0);
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
                HICON hIcon;

                t_hwnd = dat->pContainer->hwnd;

                if (dat->szProto) {
                    hIcon = LoadSkinnedProtoIcon(dat->szProto, dat->wStatus);
                    SendDlgItemMessage(hwndDlg, IDC_PROTOCOL, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

                    if (dat->pContainer->hwndActive == hwndDlg)
                        SendMessage(t_hwnd, DM_SETICON, (WPARAM) ICON_BIG, (LPARAM) hIcon);
                    break;
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
                    char *szContactName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) dat->hContact, 0);

                    _snprintf(szBuf, sizeof(szBuf), Translate("%s is typing..."), szContactName);
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
                        _snprintf(fmt, sizeof(fmt), Translate("UIN: %s"), dat->uin);
                    else
                        _snprintf(fmt, sizeof(fmt), Translate("Last received: %s at %s"), date, time);
                    SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) fmt);
                    SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 0, (LPARAM) NULL);
                } else {
                    SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) "");
                    SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 0, (LPARAM) NULL);
                }
                break;
            }
            /*
             * configures the toolbar only... if lParam != 0, then it also calls
             * SetDialogToType() to reconfigure the message window
             */
            
        case DM_CONFIGURETOOLBAR:
            dat->showUIElements = dat->pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;
            if(lParam == 1)
                SetDialogToType(hwndDlg);
            dat->iButtonBarNeeds = dat->showUIElements ? (myGlobals.m_AllowSendButtonHidden ? 0 : 40) : 0;
            dat->iButtonBarNeeds += (dat->showUIElements ? (dat->doSmileys ? 180 : 154) : 0);
            
            dat->iButtonBarNeeds += (dat->showUIElements) ? 28 : 0;
            dat->iButtonBarReallyNeeds = dat->iButtonBarNeeds + (dat->showUIElements ? (myGlobals.m_AllowSendButtonHidden ? 122 : 82) : 0);
            if(!myGlobals.m_SendFormat)
                dat->iButtonBarReallyNeeds -= 78;
            if(lParam == 1) {
                SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
                SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
                SendMessage(hwndDlg, WM_SIZE, 0, 0);
                SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 0);
            }
            break;
        case DM_LOADBUTTONBARICONS:
            SendDlgItemMessage(hwndDlg, IDC_HISTORY, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[1]);
            SendDlgItemMessage(hwndDlg, IDC_TIME, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[2]);
            SendDlgItemMessage(hwndDlg, IDC_SENDMENU, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[16]);
            SendDlgItemMessage(hwndDlg, IDC_QUOTE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[8]);
            SendDlgItemMessage(hwndDlg, IDC_SAVE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[6]);
            SendDlgItemMessage(hwndDlg, IDC_PIC, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[10]);
            SendDlgItemMessage(hwndDlg, IDOK, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[9]);
            SendDlgItemMessage(hwndDlg, IDC_STATICERRORICON, STM_SETICON, (WPARAM)myGlobals.g_iconErr, 0);
            SendDlgItemMessage(hwndDlg, IDC_ADDICON, STM_SETICON, (WPARAM)myGlobals.g_buttonBarIcons[0], 0);
            SendDlgItemMessage(hwndDlg, IDC_MULTIPLEICON, STM_SETICON, (WPARAM)myGlobals.g_buttonBarIcons[3], 0);
            SendDlgItemMessage(hwndDlg, IDC_PROTOMENU, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[16]);
            SendDlgItemMessage(hwndDlg, IDC_FONTBOLD, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[17]);
            SendDlgItemMessage(hwndDlg, IDC_FONTITALIC, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[18]);
            SendDlgItemMessage(hwndDlg, IDC_FONTUNDERLINE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[19]);
            SendDlgItemMessage(hwndDlg, IDC_FONTFACE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[20]);
            SendDlgItemMessage(hwndDlg, IDC_FONTCOLOR, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[21]);
            SendDlgItemMessage(hwndDlg, IDC_NAME, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[4]);
            break;
        case DM_OPTIONSAPPLIED:
            if (wParam == 1) {      // 1 means, the message came from message log options page, so reload the defaults...
                if(myGlobals.m_IgnoreContactSettings) {
                    dat->dwFlags &= ~(MWF_LOG_ALL);
                    dat->dwFlags |= DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);
                }
            }

            if(dat->hContact && dat->szProto != NULL) {
                if(!dat->bIsMeta)
                    dat->hProtoIcon = (HICON)LoadSkinnedProtoIcon(dat->szProto, ID_STATUS_ONLINE);
                else {
                    DWORD dwForcedContactNum = 0;
                    CallService(MS_MC_GETFORCESTATE, (WPARAM)dat->hContact, (LPARAM)&dwForcedContactNum);
                    DBWriteContactSettingDword(dat->hContact, "MetaContacts", "tabSRMM_forced", dwForcedContactNum);
                    dat->hProtoIcon = (HICON)LoadSkinnedProtoIcon(GetCurrentMetaContactProto(hwndDlg, dat), ID_STATUS_ONLINE);
                }
            }
            
            dat->showUIElements = dat->pContainer->dwFlags & CNT_HIDETOOLBAR ? 0 : 1;
            
            dat->dwEventIsShown = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWURLS, SRMSGDEFSET_SHOWURLS) ? MWF_SHOW_URLEVENTS : 0;
            dat->dwEventIsShown |= DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWFILES, SRMSGDEFSET_SHOWFILES) ? MWF_SHOW_FILEEVENTS : 0;
            dat->dwEventIsShown |= DBGetContactSettingByte(NULL, SRMSGMOD_T, "in_out_icons", 0) ? MWF_SHOW_INOUTICONS : 0;
            dat->dwEventIsShown |= DBGetContactSettingByte(NULL, SRMSGMOD_T, "emptylinefix", 1) ? MWF_SHOW_EMPTYLINEFIX : 0;
            dat->dwEventIsShown |= MWF_SHOW_MICROLF;
            dat->dwEventIsShown |= DBGetContactSettingByte(NULL, SRMSGMOD_T, "followupts", 1) ? MWF_SHOW_MARKFOLLOWUPTS : 0;
            dat->dwEventIsShown |= DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "splitoverride", 0) ? MWF_SHOW_SPLITTEROVERRIDE : 0;
            
            dat->iAvatarDisplayMode = myGlobals.m_AvatarDisplayMode;
            
            if(dat->dwFlags & MWF_LOG_GRID && DBGetContactSettingByte(NULL, SRMSGMOD_T, "wantvgrid", 0))
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,1));     // XXX margins in the log (looks slightly better)
            else
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0,0));     // XXX margins in the log (looks slightly better)
                
            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3,3));     // XXX margins in the message area as well
            
            dat->showTypingWin = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGWIN, SRMSGDEFSET_SHOWTYPINGWIN);
            
            SetDialogToType(hwndDlg);
            SendMessage(hwndDlg, DM_CONFIGURETOOLBAR, 0, 0);

            if (dat->hBkgBrush)
                DeleteObject(dat->hBkgBrush);
            if (dat->hInputBkgBrush)
                DeleteObject(dat->hInputBkgBrush);
            {
                char *szStreamOut = NULL;
                SETTEXTEX stx = {ST_DEFAULT,CP_UTF8};
                
                COLORREF colour = DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR);
                COLORREF inputcolour = DBGetContactSettingDword(NULL, SRMSGMOD_T, "inputbg", SRMSGDEFSET_BKGCOLOUR);
                COLORREF inputcharcolor;
                CHARFORMAT2A cf2 = {0};
                LOGFONTA lf;
                HDC hdc = GetDC(NULL);
                int logPixelSY = GetDeviceCaps(hdc, LOGPIXELSY);
                ReleaseDC(NULL, hdc);

                if(GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MESSAGE)) > 0)
                    szStreamOut = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, (CP_UTF8 << 16) | (SF_RTFNOOBJS|SFF_PLAINRTF|SF_USECODEPAGE));

                dat->hBkgBrush = CreateSolidBrush(colour);
                dat->hInputBkgBrush = CreateSolidBrush(inputcolour);
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETBKGNDCOLOR, 0, colour);
                SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETBKGNDCOLOR, 0, inputcolour);
                LoadMsgDlgFont(MSGFONTID_MESSAGEAREA, &lf, &inputcharcolor);
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
#ifdef _UNICODE
                wchar_t w_newtitle[256];
#endif
                char newtitle[128];
                char *szStatus = NULL, *contactName, *pszNewTitleEnd, newcontactname[128], *temp;
                TCITEM item;
                int    iHash = 0;

                DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) wParam;

                ZeroMemory((void *)newcontactname,  sizeof(newcontactname));

                pszNewTitleEnd = "Message Session";
                
                if (dat->hContact) {
                    int iHasName;
                    char fulluin[128];
                    if (dat->szProto) {

                        if(dat->bIsMeta)                    // update uin for metacontacts (may change at any time)
                            GetContactUIN(hwndDlg, dat);
                        
                        contactName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) dat->hContact, 0);
                        iHasName = (int)dat->uin[0];        // dat->uin[0] == 0 if there is no valid UIN
                    /*
                     * cut nickname on tabs...
                     */
                        temp = contactName;
                        while(*temp)
                            iHash += (*(temp++) * (int)(temp - contactName + 1));

                        dat->wStatus = DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE);

                        if (iHash != dat->iOldHash || dat->wStatus != dat->wOldStatus || lParam != 0) {
                            if (myGlobals.m_CutContactNameOnTabs)
                                CutContactName(contactName, newcontactname, sizeof(newcontactname));
                            else
                                strncpy(newcontactname, contactName, sizeof(newcontactname));

                            szStatus = (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, dat->szProto == NULL ? ID_STATUS_OFFLINE : dat->wStatus, 0);
                            if (strlen(newcontactname) != 0 && szStatus != NULL) {
                                if (myGlobals.m_StatusOnTabs)
                                    _snprintf(newtitle, 127, "%s (%s)", newcontactname, szStatus);
                                else
                                    _snprintf(newtitle, 127, "%s", newcontactname);
                            } else {
                                _snprintf(newtitle, 127, "%s", "Forward");
                            }
                        }
                        if (!cws || (!strcmp(cws->szModule, dat->szProto) && !strcmp(cws->szSetting, "Status"))) {
                            InvalidateRect(GetDlgItem(hwndDlg, IDC_PROTOCOL), NULL, TRUE);
                            SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
                        }
                        _snprintf(fulluin, sizeof(fulluin), Translate("UIN: %s (SHIFT click copies it to the clipboard)"), iHasName ? dat->uin : Translate("No UIN"));
                        SendMessage(GetDlgItem(hwndDlg, IDC_NAME), BUTTONADDTOOLTIP, iHasName ? (WPARAM)fulluin : (WPARAM)"", 0);
                        
                    }
                } else
                    lstrcpynA(newtitle, pszNewTitleEnd, sizeof(newtitle));

                ZeroMemory((void *)&item, sizeof(item));
                item.mask = TCIF_TEXT;

                if (iHash != dat->iOldHash || dat->wStatus != dat->wOldStatus || lParam != 0) {
                    if(dat->hContact != 0 && myGlobals.m_LogStatusChanges != 0 && DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "logstatus", -1) != 0) {
                        if(dat->wStatus != dat->wOldStatus && dat->hContact != 0 && dat->wOldStatus != (WORD)-1 && !(dat->dwFlags & MWF_INITMODE)) {             // log status changes to message log
                            DBEVENTINFO dbei;
                            char buffer[450];
                            HANDLE hNewEvent;
                            int iLen;
                            
                            char *szOldStatus = (char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)dat->wOldStatus, 0);
                            char *szNewStatus = (char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)dat->wStatus, 0);
                            
                            if(dat->szProto != NULL) {
                                if(dat->wStatus == ID_STATUS_OFFLINE)
                                    _snprintf(buffer, sizeof(buffer), Translate("signed off (was %s)"), szOldStatus);
                                else if(dat->wOldStatus == ID_STATUS_OFFLINE)
                                    _snprintf(buffer, sizeof(buffer), Translate("signed on (%s)"), szNewStatus);
                                else
                                    _snprintf(buffer, sizeof(buffer), Translate("is now %s (was %s)"), szNewStatus, szOldStatus);
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

                    dat->iOldHash = iHash;
                    dat->wOldStatus = dat->wStatus;
#ifdef _UNICODE
                    if (MultiByteToWideChar(CP_ACP, 0, newtitle, -1, w_newtitle, sizeof(w_newtitle)) != 0) {
                        item.pszText = w_newtitle;
                        item.cchTextMax = sizeof(w_newtitle);
                    }
#else
                    item.pszText = newtitle;
                    item.cchTextMax = 127;;
#endif
                    item.mask = TCIF_TEXT | TCIF_IMAGE;     // XXX update tab text + icon

                    if (dat->iTabID  >= 0) {
                        item.iImage = GetProtoIconFromList(dat->szProto, (int)dat->wStatus);       // proto icon support
                        dat->iTabImage = item.iImage;                               // save new icon for flashing...
                        if(dat->dwFlags & MWF_ERRORSTATE)                        // dont update icon if we are in error state (leave the 
                            item.mask &= ~TCIF_IMAGE;
                        TabCtrl_SetItem(hwndTab, dat->iTabID, &item);
                    }
                    if (dat->pContainer->hwndActive == hwndDlg)
                        SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
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
            if (dat->iTabID >= 0) {
                SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                dat->dwTickLastEvent = 0;
                dat->dwFlags &= ~MWF_DIVIDERSET;
                if (KillTimer(hwndDlg, TIMERID_FLASHWND)) {
                    FlashTab(hwndTab, dat->iTabID, &dat->bTabFlash, FALSE, 0, dat->iTabImage);
                    dat->mayFlashTab = FALSE;
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

                if (myGlobals.m_AutoLocaleSupport && dat->hContact != 0)
                    SendMessage(hwndDlg, DM_SETLOCALE, 0, 0);
                SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                UpdateStatusBar(hwndDlg, dat);
                dat->dwLastActivity = GetTickCount();
                dat->pContainer->dwLastActivity = dat->dwLastActivity;
                UpdateContainerMenu(hwndDlg, dat);
            }
            return 1;
            break;
        case WM_ACTIVATE:
            if (LOWORD(wParam) != WA_ACTIVE)
                break;
            //fall through
        case WM_MOUSEACTIVATE:
            if (dat->iTabID == -1) {
                _DebugPopup(dat->hContact, "ACTIVATE Critical: iTabID == -1");
                break;
            } else {
                dat->dwFlags &= ~MWF_DIVIDERSET;
                /* if(dat->pContainer->dwTickLastEvent == dat->dwTickLastEvent)
                    dat->pContainer->dwTickLastEvent = 0; */
                dat->dwTickLastEvent = 0;
                if (KillTimer(hwndDlg, TIMERID_FLASHWND)) {
                    FlashTab(hwndTab, dat->iTabID, &dat->bTabFlash, FALSE, 0, dat->iTabImage);
                    dat->mayFlashTab = FALSE;
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

                if (myGlobals.m_AutoLocaleSupport && dat->hContact != 0)
                    SendMessage(hwndDlg, DM_SETLOCALE, 0, 0);

                UpdateStatusBar(hwndDlg, dat);
                dat->dwLastActivity = GetTickCount();
                dat->pContainer->dwLastActivity = dat->dwLastActivity;
                UpdateContainerMenu(hwndDlg, dat);
            }
            break;
        case WM_GETMINMAXINFO:
            {
                MINMAXINFO *mmi = (MINMAXINFO *) lParam;
                RECT rcWindow, rcLog;
                GetWindowRect(hwndDlg, &rcWindow);
                GetWindowRect(GetDlgItem(hwndDlg, IDC_LOG), &rcLog);
                mmi->ptMinTrackSize.x = rcWindow.right - rcWindow.left - ((rcLog.right - rcLog.left) - dat->minEditBoxSize.cx);
                mmi->ptMinTrackSize.y = rcWindow.bottom - rcWindow.top - ((rcLog.bottom - rcLog.top) - dat->minEditBoxSize.cy);
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
                    
                if (IsIconic(hwndDlg))
                    break;
                ZeroMemory(&urd, sizeof(urd));
                urd.cbSize = sizeof(urd);
                urd.hInstance = g_hInst;
                urd.hwndDlg = hwndDlg;
                urd.lParam = (LPARAM) dat;
                urd.lpTemplate = MAKEINTRESOURCEA(IDD_MSGSPLITNEW);
                urd.pfnResizer = MessageDialogResize;

                if(dat->hContactPic != 0) {
                    GetObject(dat->hContactPic, sizeof(bminfo), &bminfo);
                    CalcDynamicAvatarSize(hwndDlg, dat, &bminfo);
                }

                if (dat->uMinHeight > 0 && HIWORD(lParam) >= dat->uMinHeight) {
                    if (dat->splitterY > HIWORD(lParam) - MINLOGHEIGHT) {
                        dat->splitterY = HIWORD(lParam) - MINLOGHEIGHT;
                        dat->dynaSplitter = dat->splitterY - 34;
                        SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
                        SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
                    }
                    if (dat->splitterY < MINSPLITTERY) {
                        if(dat->showPic)
                            LoadSplitter(hwndDlg, dat);
                    }
                }

                // dynamic toolbar layout...
                
                GetClientRect(hwndDlg, &rc);
                buttonBarSpace = (rc.right - rc.left) - 6;
                if(dat->showPic) {
                    if(dat->splitterY <= (dat->bottomOffset + 33))
                        buttonBarSpace = (rc.right - rc.left - 6) - (dat->pic.cx + 6);
                }

                dat->controlsHidden = FALSE;

                delta = dat->iButtonBarReallyNeeds - buttonBarSpace;

                if(dat->hContact != 0 && dat->showUIElements != 0) {
                    const UINT *hideThisControls = myGlobals.m_ToolbarHideMode ? controlsToHide : controlsToHide1;
                    if(!myGlobals.m_SendFormat)
                        hideThisControls = controlsToHide2;
                    for(i = 0;;i++) {
                        if(hideThisControls[i] == -1)
                            break;
                        if(hideThisControls[i] == IDOK && myGlobals.m_AllowSendButtonHidden == 0)
                            continue;               // ignore sendbutton if we don't want to hide it
                        if(saved < delta) {
                            ShowWindow(GetDlgItem(hwndDlg, hideThisControls[i]), SW_HIDE);
                            if(hideThisControls[i] == IDC_PROTOCOL)
                                dat->controlsHidden |= TOOLBAR_PROTO_HIDDEN;
                            if(hideThisControls[i] == IDOK)
                                dat->controlsHidden |= TOOLBAR_SEND_HIDDEN;
                            saved += (hideThisControls[i] == IDOK ? 40 : 26);
                        }
                        else {
                            if(!IsWindowVisible(GetDlgItem(hwndDlg, hideThisControls[i])))
                                ShowWindow(GetDlgItem(hwndDlg, hideThisControls[i]), SW_SHOW);
                        }
                    }
                }
                
                CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM) & urd);
                // IEVIew MOD Begin
                if (dat->hwndLog != 0) {
                    RECT rcRichEdit;
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
                    ieWindow.x = pt.x;
                    ieWindow.y = pt.y;
                    ieWindow.cx = rcRichEdit.right - rcRichEdit.left;
                    ieWindow.cy = rcRichEdit.bottom - rcRichEdit.top;
                    CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
                }
                // IEVIew MOD End
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
                    dat->splitterY = rc.bottom - pt.y + 26;
                /*
                 * attempt to fix splitter troubles..
                 * hardcoded limits... better solution is possible, but this works for now
                 */
                 //   if (dat->showPic) {
                        // handle dynamic resizing of picture while splitter is moving...
                        if (dat->splitterY <= MINSPLITTERY)           // min splitter size
                            dat->splitterY = oldSplitterY;
                        else if (dat->splitterY > ((rc.bottom - rc.top) - 50)) 
                            dat->splitterY = oldSplitterY;
                        else {
                            if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC) {
                                dat->dynaSplitter = (rc.bottom - pt.y) - 8;
                                SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
                            }
                            else {
                                dat->dynaSplitter = (rc.bottom - pt.y) - 8;
                                if(dat->splitterY <= dat->bottomOffset)           // min splitter size
                                    dat->splitterY = oldSplitterY;
                                SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
                            }
                        }
                 //   }
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
                if(myGlobals.m_IgnoreContactSettings) {
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
            /*
             * inserts a bitmap into the rich edit control, replacing the selected text
             * wParam = ole interface pointer
             * lParam = handle to the bitmap.
             * this is called from the icon replacement code AND smileyadd, if we are
             * running in multithreaded mode (separate thread for replacing icons and smileys).
             * needs to work this way, because the ole objects can only be inserted by the thread that "owns" the rich edit control. OLE sucks :)
             */
        case DM_INSERTICON:
            {
                IRichEditOle *ole = (IRichEditOle *)wParam;
                ImageDataInsertBitmap(ole, (HBITMAP)lParam);
                return 0;
            }
        case DM_REMAKELOG:
            dat->lastEventTime = 0;
            dat->iLastEventType = -1;
            StreamInEvents(hwndDlg, dat->hDbEventFirst, -1, 0, NULL);
            break;
        case DM_APPENDTOLOG:
            StreamInEvents(hwndDlg, (HANDLE) wParam, 1, 1, NULL);
            break;
        case DM_SCROLLLOGTOBOTTOM:
            {
                SCROLLINFO si = { 0 };

                if(!IsIconic(dat->pContainer->hwnd)) {
                    dat->dwFlags &= ~MWF_DEFERREDSCROLL;
                    if ((GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG), GWL_STYLE) & WS_VSCROLL) == 0)
                        break;
                    if(lParam)
                        SendMessage(GetDlgItem(hwndDlg, IDC_LOG), WM_SIZE, 0, 0);
                    si.cbSize = sizeof(si);
                    si.fMask = SIF_PAGE | SIF_RANGE;
                    GetScrollInfo(GetDlgItem(hwndDlg, IDC_LOG), SB_VERT, &si);
                    si.fMask = SIF_POS;
                    si.nPos = si.nMax - si.nPage + 1;
                    SetScrollInfo(GetDlgItem(hwndDlg, IDC_LOG), SB_VERT, &si, TRUE);
                    if(wParam)
                        SendMessage(GetDlgItem(hwndDlg, IDC_LOG), WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
                    else
                        PostMessage(GetDlgItem(hwndDlg, IDC_LOG), WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
                    if(lParam)
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_LOG), NULL, FALSE);
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
                            dat->dwTickLastEvent = GetTickCount();
                            //dat->pContainer->dwTickLastEvent = dat->dwTickLastEvent;
                            if(!iDividerSet)
                                SendMessage(hwndDlg, DM_ADDDIVIDER, 0, 0);
                        }
                        else {
                            if(dat->pContainer->hwndActive != hwndDlg) {
                                dat->dwTickLastEvent = GetTickCount();
                                //dat->pContainer->dwTickLastEvent = dat->dwTickLastEvent;
                                if(!iDividerSet)
                                    SendMessage(hwndDlg, DM_ADDDIVIDER, 0, 0);
                            }
                        }
                    }
                    
                    if ((HANDLE) lParam != dat->hDbEventFirst) {
                        HANDLE nextEvent = (HANDLE) CallService(MS_DB_EVENT_FINDNEXT, lParam, 0);
                        if(myGlobals.m_FixFutureTimestamps || nextEvent == 0) {
                            SendMessage(hwndDlg, DM_APPENDTOLOG, lParam, 0);
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
           
            if (wParam >= TIMERID_MSGSEND) {
                int iIndex = wParam - TIMERID_MSGSEND;

                if(iIndex < NR_SENDJOBS) {      // single sendjob timer
                    KillTimer(hwndDlg, wParam);
                    _snprintf(sendJobs[iIndex].szErrorMsg, sizeof(sendJobs[iIndex].szErrorMsg), Translate("Delivery failure: %s"), Translate("The message send timed out"));
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
                        FlashTab(hwndTab, dat->iTabID, &dat->bTabFlash, TRUE, dat->iFlashIcon, dat->iTabImage);
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
                        SendMessage(dat->pContainer->hwnd, DM_UPDATEWINICON, 0, 0);
                        HandleIconFeedback(hwndDlg, dat, -1);
                        SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, 0, 0);
                        if(!(dat->pContainer->dwFlags & CNT_NOFLASH) && dat->showTypingWin)
                            ReflashContainer(dat->pContainer);
                    }
                } else {
                    if (dat->nTypeSecs > 0) {
                        char szBuf[256];
                        char *szContactName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) dat->hContact, 0);

                        _snprintf(szBuf, sizeof(szBuf), Translate("%s is typing..."), szContactName);
                        dat->nTypeSecs--;
                        if(dat->pContainer->hwndStatus && dat->pContainer->hwndActive == hwndDlg) {
                            SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) szBuf);
                            SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 0, (LPARAM) myGlobals.g_buttonBarIcons[5]);
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
                        if ((GetForegroundWindow() != dat->pContainer->hwnd) || (dat->pContainer->hwndStatus == 0))
                            SendMessage(dat->pContainer->hwnd, DM_SETICON, (WPARAM) ICON_BIG, (LPARAM) myGlobals.g_buttonBarIcons[5]);
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
                                sendJobs[dat->iCurrentQueueError].hSendId[i] = (HANDLE) CallContactService(sendJobs[dat->iCurrentQueueError].hContact[i], MsgServiceName(sendJobs[dat->iCurrentQueueError].hContact[i], dat), dat->sendMode & SMODE_FORCEANSI ? 0 : SEND_FLAGS, (LPARAM) sendJobs[dat->iCurrentQueueError].sendBuffer);
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
// XXX tab mod
        case DM_SELECTTAB:
            SendMessage(dat->pContainer->hwnd, DM_SELECTTAB, wParam, lParam);       // pass the msg to our container
            break;
// XXX tab mod
        case DM_SAVELOCALE: {
                if (myGlobals.m_AutoLocaleSupport && dat->hContact) {
                    char szKLName[KL_NAMELENGTH + 1];
#if defined ( _UNICODE )
                    GetKeyboardLayoutNameA(szKLName);
#else
                    GetKeyboardLayoutName(szKLName);
#endif
                    DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "locale", szKLName);
#if defined ( _UNICODE )
                    dat->hkl = LoadKeyboardLayoutA(szKLName, 0);
#else                
                    dat->hkl = LoadKeyboardLayout(szKLName, 0);
#endif
                    if(dat->pContainer->hwndStatus && dat->pContainer->hwndActive == hwndDlg)
                        SendMessage(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) Translate("Input locale saved."));
                }
                break;
            }
        case DM_SETLOCALE:
            if (myGlobals.m_AutoLocaleSupport && dat->hContact != 0) {
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
                *wStatus = dat->wStatus;
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
                    if(!dat->hThread)
                        ShowPicture(hwndDlg,dat,FALSE,TRUE, TRUE);
                    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETSCROLLPOS, 0, (LPARAM)&pt);
                    LoadSplitter(hwndDlg, dat);
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
                    PostMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 1, 1);
                    if(dat->hwndLog != 0)
                        SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                }
                else {
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 1, 1);
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
            
            if(dat->hContactPic == 0) {
                dat->pic.cy = dat->pic.cx = 60;
                break;
            }
            GetObject(dat->hContactPic,sizeof(bminfo),&bminfo);
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
                int iOffset = 0;
                
                if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC) {
                    dat->bottomOffset = dat->dynaSplitter + 100;
                    break;
                }
                if(dat->hContactPic) {
                    dat->bottomOffset = dat->iRealAvatarHeight;
                    dat->bottomOffset += 4;
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
            RECT rcPicture;
            
            GetWindowRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), &rcPicture);
            GetCursorPos(&pt);
            if(dat->hContactPic && dat->showPic != 0 &&  pt.x >= rcPicture.left && pt.x <= rcPicture.right && pt.y >= rcPicture.top && pt.y <= rcPicture.bottom) {
                int iSelection, isHandled;
                POINT pt;
                HMENU submenu = GetSubMenu(dat->pContainer->hMenuContext, 1);

                GetCursorPos(&pt);
                MsgWindowUpdateMenu(hwndDlg, dat, submenu, MENU_PICMENU);
                iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
                isHandled = MsgWindowMenuHandler(hwndDlg, dat, iSelection, MENU_PICMENU);
                if(isHandled)
                    RedrawWindow(GetDlgItem(hwndDlg, IDC_PIC), NULL, NULL, RDW_INVALIDATE);
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
                        WCHAR wszTemp[CONTAINER_NAMELEN + 2];
                        _tcsncpy(wszTemp, Utf8Decode(dbv.pszVal), CONTAINER_NAMELEN);
                        SendMessage(hwndDlg, DM_CONTAINERSELECTED, 0, (LPARAM) wszTemp);
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
                RECT rc;
                POINT pt;
                
                if (dat->pContainer->dwFlags & CNT_NOTITLE && dat->dwFlags & MWF_MOUSEDOWN) {
                    GetWindowRect(dat->pContainer->hwnd, &rc);
                    GetCursorPos(&pt);
                    MoveWindow(dat->pContainer->hwnd, rc.left - (dat->ptLast.x - pt.x), rc.top - (dat->ptLast.y - pt.y), rc.right - rc.left, rc.bottom - rc.top, TRUE);
                    dat->ptLast = pt;
                }
                break;
            }
        case WM_MEASUREITEM:
            return CallService(MS_CLIST_MENUMEASUREITEM, wParam, lParam);
        case WM_NCHITTEST:
            SendMessage(dat->pContainer->hwnd, WM_NCHITTEST, wParam, lParam);
            break;
        case WM_DRAWITEM:
            {
                LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;
                if (dis->hwndItem == GetDlgItem(hwndDlg, IDC_TYPINGNOTIFY)) {
                    DrawIconEx(dis->hDC, dis->rcItem.left, dis->rcItem.top, myGlobals.g_buttonBarIcons[5], GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL);
                    return TRUE;
                } else if (dis->hwndItem == GetDlgItem(hwndDlg, IDC_CONTACTPIC) && dat->hContactPic && dat->showPic) {
                    HPEN hPen;
                    BITMAP bminfo;
                    double dAspect = 0, dNewWidth = 0;
                    DWORD iMaxHeight = 0, top;
                    RECT rc;
                    
                    GetObject(dat->hContactPic, sizeof(bminfo), &bminfo);

                    GetClientRect(hwndDlg, &rc);
                    dAspect = (double)dat->iRealAvatarHeight / (double)bminfo.bmHeight;
                    dNewWidth = (double)bminfo.bmWidth * dAspect;
                    if(dNewWidth > (double)(rc.right - rc.left) * 0.8)
                        dNewWidth = (double)(rc.right - rc.left) * 0.8;
                    iMaxHeight = dat->iRealAvatarHeight;
                    hPen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
                    SelectObject(dis->hDC, hPen);

                    if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC)
                        Rectangle(dis->hDC, 0, 0, dat->pic.cx, dat->pic.cy);
                    else {
                        top = (dat->pic.cy - dat->iRealAvatarHeight) / 2;
                        Rectangle(dis->hDC, 0, top - 1, dat->pic.cx, top + dat->iRealAvatarHeight + 1);
                    }
                    
                    DeleteObject(hPen);
                    if(dat->hContactPic && dat->showPic) {
                        HDC hdcMem = CreateCompatibleDC(dis->hDC);
                        HBITMAP hbmMem = (HBITMAP)SelectObject(hdcMem, dat->hContactPic);
                        if(dat->iRealAvatarHeight != bminfo.bmHeight) {
                            SetStretchBltMode(dis->hDC, HALFTONE);
                            if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC)
                                StretchBlt(dis->hDC, 1, 1, (int)dNewWidth, iMaxHeight, hdcMem, 0, 0, bminfo.bmWidth, bminfo.bmHeight, SRCCOPY);
                            else
                                StretchBlt(dis->hDC, 1, top, (int)dNewWidth, iMaxHeight, hdcMem, 0, 0, bminfo.bmWidth, bminfo.bmHeight, SRCCOPY);
                        }
                        else {
                            if(dat->iAvatarDisplayMode == AVATARMODE_DYNAMIC)
                                BitBlt(dis->hDC, 1, 1, (int)dNewWidth, iMaxHeight, hdcMem, 0, 0, SRCCOPY);
                            else
                                BitBlt(dis->hDC, 1, top, (int)dNewWidth, iMaxHeight, hdcMem, 0, 0, SRCCOPY);
                        }
                        DeleteObject(hbmMem);
                        DeleteDC(hdcMem);
                    }
                    return TRUE;
                }
                return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
            }
        case WM_COMMAND:
            if (CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(LOWORD(wParam), MPCF_CONTACTMENU), (LPARAM) dat->hContact))
                break;
            switch (LOWORD(wParam)) {
                case IDOK:
                    {
                    int bufSize;
                    char *streamOut = NULL;
                    TCHAR *decoded = NULL, *converted = NULL;
                    
                    if (!IsWindowEnabled(GetDlgItem(hwndDlg, IDOK)))
                        break;

                    TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_CUSTOM, tabMSG_WINDOW_EVT_CUSTOM_BEFORESEND);
                    
#if defined( _UNICODE )
                    streamOut = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, myGlobals.m_SendFormat ? 0 : (CP_UTF8 << 16) | (SF_TEXT|SF_USECODEPAGE));
                    if(streamOut != NULL) {
                        decoded = Utf8Decode(streamOut);
                        converted = (TCHAR *)malloc((lstrlenW(decoded) + 2) * sizeof(TCHAR));
                        if(converted != NULL) {
                            _tcscpy(converted, decoded);
                            if(myGlobals.m_SendFormat) {
                                DoRtfToTags(converted, dat);
                                DoTrimMessage(converted);
                            }
                            bufSize = WideCharToMultiByte(dat->codePage, 0, converted, -1, dat->sendBuffer, 0, 0, 0);
                            dat->sendBuffer = (char *) realloc(dat->sendBuffer, bufSize * (sizeof(TCHAR) + 1));
                            WideCharToMultiByte(dat->codePage, 0, converted, -1, dat->sendBuffer, bufSize, 0, 0);
                            CopyMemory(&dat->sendBuffer[bufSize], converted, (lstrlenW(converted) + 1) * sizeof(WCHAR));
                            free(converted);
                        }
                        free(streamOut);
                    }
#else                    
                    streamOut = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, myGlobals.m_SendFormat ? (SF_RTFNOOBJS|SFF_PLAINRTF) : (SF_TEXT));
                    if(streamOut != NULL) {
                        converted = (TCHAR *)malloc((lstrlenA(streamOut) + 2)* sizeof(TCHAR));
                        if(converted != NULL) {
                            _tcscpy(converted, streamOut);
                            if(myGlobals.m_SendFormat) {
                                DoRtfToTags(converted, dat);
                                DoTrimMessage(converted);
                            }
                            bufSize = lstrlenA(converted) + 1;
                            dat->sendBuffer = (char *) realloc(dat->sendBuffer, bufSize * sizeof(char));
                            CopyMemory(dat->sendBuffer, converted, bufSize);
                            free(converted);
                        }
                        free(streamOut);
                    }
#endif  // unicode
                    if (dat->sendBuffer[0] == 0)
                        break;

                    if (dat->sendMode & SMODE_SENDLATER) {
#if defined(_UNICODE)
                        if(ServiceExists("BuddyPounce/AddToPounce")) {
                            CallService("BuddyPounce/AddToPounce", (WPARAM)dat->hContact, (LPARAM)dat->sendBuffer);
                            break;
                        }
#else
                        if(ServiceExists("BuddyPounce/AddToPounce")) {
                            CallService("BuddyPounce/AddToPounce", (WPARAM)dat->hContact, (LPARAM)dat->sendBuffer);
                            break;
                        }
#endif
                    }
                    if (dat->sendMode & SMODE_CONTAINER && dat->pContainer->hwndActive == hwndDlg && GetForegroundWindow() == dat->pContainer->hwnd) {
                        HWND contacthwnd;
                        TCITEM tci;
                        BOOL bOldState;
                        int tabCount = TabCtrl_GetItemCount(hwndTab), i;
                        char *szFromStream = NULL;

#if defined(_UNICODE)
                        szFromStream = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, myGlobals.m_SendFormat ? 0 : (CP_UTF8 << 16) | (SF_TEXT|SF_USECODEPAGE));
#else
                        szFromStream = Message_GetFromStream(GetDlgItem(hwndDlg, IDC_MESSAGE), dat, myGlobals.m_SendFormat ? (SF_RTFNOOBJS|SFF_PLAINRTF) : (SF_TEXT));
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
                    AddToSendQueue(hwndDlg, dat, bufSize);
                    return TRUE;
                    }
                case IDC_QUOTE:
                    {   CHARRANGE sel;
                        TCHAR *szQuoted, *szText;
                        char *szFromStream = NULL;
                        //int i;
#ifdef _UNICODE
                        TCHAR *szConverted;
                        int iAlloced = 0;
                        int iSize = 0;
                        SETTEXTEX stx = {ST_SELECTION, 1200};
                        //GETTEXTEX gtx;
#endif                        

                        if (dat->hDbEventLast==NULL) break;
                        SendDlgItemMessage(hwndDlg,IDC_LOG,EM_EXGETSEL,0,(LPARAM)&sel);
                        if (sel.cpMin==sel.cpMax) {
                            DBEVENTINFO dbei={0};
                            int iDescr;
                            
                            dbei.cbSize=sizeof(dbei);
                            dbei.cbBlob=CallService(MS_DB_EVENT_GETBLOBSIZE,(WPARAM)dat->hDbEventLast,0);
                            szText=(TCHAR *)malloc((dbei.cbBlob+1) * sizeof(TCHAR));   //URLs are made one char bigger for crlf
                            dbei.pBlob=(BYTE *)szText;
                            CallService(MS_DB_EVENT_GET,(WPARAM)dat->hDbEventLast,(LPARAM)&dbei);
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
                            szQuoted = QuoteText(Utf8Decode(szFromStream), 64, 0);
                            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)szQuoted);
                            // SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_REPLACESEL, TRUE, (LPARAM)szQuoted);
                            free(szQuoted);
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
    					CHARFORMAT2 cf;
    					cf.cbSize = sizeof(CHARFORMAT2);
    					cf.dwMask = CFM_BOLD|CFM_ITALIC|CFM_UNDERLINE;
    					cf.dwEffects = 0;
    
    					if(LOWORD(wParam) == IDC_FONTBOLD && !IsWindowEnabled(GetDlgItem(hwndDlg,IDC_FONTBOLD))) 
    						break;
    					if(LOWORD(wParam) == IDC_FONTITALIC && !IsWindowEnabled(GetDlgItem(hwndDlg,IDC_FONTITALIC))) 
    						break;
    					if(LOWORD(wParam) == IDC_FONTUNDERLINE && !IsWindowEnabled(GetDlgItem(hwndDlg,IDC_FONTUNDERLINE))) 
    						break;
    					if (IsDlgButtonChecked(hwndDlg, IDC_FONTBOLD))
    						cf.dwEffects |= CFE_BOLD;
    					if (IsDlgButtonChecked(hwndDlg, IDC_FONTITALIC))
    						cf.dwEffects |= CFE_ITALIC;
    					if (IsDlgButtonChecked(hwndDlg, IDC_FONTUNDERLINE))
    						cf.dwEffects |= CFE_UNDERLINE;
    					SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
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
                case IDC_DETAILS:
                    CallService(MS_USERINFO_SHOWDIALOG, (WPARAM) dat->hContact, 0);
                    break;
                case IDC_SMILEYBTN:
                    if(dat->doSmileys && (myGlobals.g_SmileyAddAvail || dat->hwndLog != 0)) {
                        SMADD_SHOWSEL smaddInfo;
                        HICON hButtonIcon = 0;
                        RECT rc;
                        
                        if(CheckValidSmileyPack(dat->szProto, &hButtonIcon) != 0 || dat->hwndLog != 0) {
                            smaddInfo.cbSize = sizeof(SMADD_SHOWSEL);
                            smaddInfo.hwndTarget = GetDlgItem(hwndDlg, IDC_MESSAGE);
                            smaddInfo.targetMessage = EM_REPLACESEL;
                            smaddInfo.targetWParam = TRUE;
                            smaddInfo.Protocolname = dat->szProto;
                            GetWindowRect(GetDlgItem(hwndDlg, IDC_SMILEYBTN), &rc);

                            smaddInfo.Direction = 0;
                            smaddInfo.xPosition = rc.left;
                            smaddInfo.yPosition = rc.top + 24;
                            dat->dwFlags |= MWF_SMBUTTONSELECTED;
                            if(dat->hwndLog)
                                CallService(MS_IEVIEW_SHOWSMILEYSELECTION, 0, (LPARAM)&smaddInfo);
                            else
                                CallService(MS_SMILEYADD_SHOWSELECTION, 0, (LPARAM) &smaddInfo);
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
                        WindowList_Broadcast(hMessageWindowList, DM_DEFERREDREMAKELOG, (WPARAM)hwndDlg, (LPARAM)(dat->dwFlags & MWF_LOG_ALL));
                        if(myGlobals.m_IgnoreContactSettings)
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", dat->dwFlags & MWF_LOG_ALL);
                    }
                    break;
                }
                case IDC_PROTOMENU: {
                    RECT rc;
                    HMENU submenu = GetSubMenu(dat->pContainer->hMenuContext, 4);
                    int iSelection;

                    if(dat->hContact) {
                        unsigned int iOldIEView = GetIEViewMode(hwndDlg, dat);
                        unsigned int iNewIEView = 0;
                        GetWindowRect(GetDlgItem(hwndDlg, IDC_PROTOMENU), &rc);

                        EnableMenuItem(submenu, 0, MF_BYPOSITION | (ServiceExists(MS_IEVIEW_WINDOW) ? MF_ENABLED : MF_GRAYED));
                        
                        CheckMenuItem(submenu, ID_IEVIEWSETTING_USEGLOBAL, MF_BYCOMMAND | (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", 0) == 0 ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_IEVIEWSETTING_FORCEIEVIEW, MF_BYCOMMAND | (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", 0) == 1 ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_IEVIEWSETTING_FORCEDEFAULTMESSAGELOG, MF_BYCOMMAND | (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "ieview", 0) == (BYTE)-1 ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_SPLITTER_AUTOSAVEONCLOSE, MF_BYCOMMAND | (myGlobals.m_SplitterSaveOnClose ? MF_CHECKED : MF_UNCHECKED));

                        CheckMenuItem(submenu, ID_MODE_GLOBAL, MF_BYCOMMAND | ((myGlobals.m_SplitterMode == 0 && !(dat->dwEventIsShown & MWF_SHOW_SPLITTEROVERRIDE)) ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_MODE_PERCONTACT, MF_BYCOMMAND | ((myGlobals.m_SplitterMode == 1 && !(dat->dwEventIsShown & MWF_SHOW_SPLITTEROVERRIDE)) ? MF_CHECKED : MF_UNCHECKED));
                        CheckMenuItem(submenu, ID_MODE_PRIVATE, MF_BYCOMMAND | (dat->dwEventIsShown & MWF_SHOW_SPLITTEROVERRIDE ? MF_CHECKED : MF_UNCHECKED));

                        EnableMenuItem(submenu, ID_MODE_GLOBAL, MF_BYCOMMAND | (dat->dwEventIsShown & MWF_SHOW_SPLITTEROVERRIDE ? MF_GRAYED : MF_ENABLED));
                        EnableMenuItem(submenu, ID_MODE_PERCONTACT, MF_BYCOMMAND | (dat->dwEventIsShown & MWF_SHOW_SPLITTEROVERRIDE ? MF_GRAYED : MF_ENABLED));
                        
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
                                myGlobals.m_SplitterMode = 0;
                                DBWriteContactSettingByte(NULL, SRMSGMOD_T, "splittermode", 0);
                                break;
                            case ID_MODE_PERCONTACT:
                                myGlobals.m_SplitterMode = 1;
                                DBWriteContactSettingByte(NULL, SRMSGMOD_T, "splittermode", 1);
                                break;
                            case ID_MODE_PRIVATE:
                                dat->dwEventIsShown ^= MWF_SHOW_SPLITTEROVERRIDE;
                                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "splitoverride", (BYTE)(dat->dwEventIsShown & MWF_SHOW_SPLITTEROVERRIDE));
                                break;
                        }
                        iNewIEView = GetIEViewMode(hwndDlg, dat);
                        if(iNewIEView != iOldIEView) {
                            if(iNewIEView) {            // switch from rtf to IEview
                                SetDlgItemText(hwndDlg, IDC_LOG, _T(""));
                                EnableWindow(GetDlgItem(hwndDlg, IDC_LOG), FALSE);
                                ShowWindow(GetDlgItem(hwndDlg, IDC_LOG), SW_HIDE);
                                SetMessageLog(hwndDlg, dat);
                            }
                            else                      // switch from IEView to rtf
                                SetMessageLog(hwndDlg, dat);
                            SetDialogToType(hwndDlg);
                            SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                            SendMessage(hwndDlg, WM_SIZE, 0, 0);
                            UpdateContainerMenu(hwndDlg, dat);
                        }
                    }
                    break;
                }
                case IDC_TOGGLETOOLBAR:
                    if(GetKeyState(VK_SHIFT) & 0x8000)
                        dat->pContainer->dwFlags ^= CNT_NOMENUBAR;
                    else if(GetKeyState(VK_CONTROL) & 0x8000)
                        dat->pContainer->dwFlags ^= CNT_NOSTATUSBAR;
                    else
                        dat->pContainer->dwFlags ^= CNT_HIDETOOLBAR;
                    
                    SendMessage(dat->pContainer->hwnd, DM_CONFIGURECONTAINER, 0, 0);
                    SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                    break;
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
                    }
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
                            ShowMultipleControls(hwndDlg, addControls, 4, SW_HIDE);
                            SendMessage(hwndDlg, WM_SIZE, 0, 0);
                        }
                        break;
                    }
                case IDC_CANCELADD:
                    dat->bNotOnList = FALSE;
                    ShowMultipleControls(hwndDlg, addControls, 4, SW_HIDE);
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    break;
// BEGIN MOD#33: Show contact's picture
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
// END MOD#33
                case IDM_CLEAR:
                    // IEVIew MOD Begin
                    if (dat->hwndLog != 0) {
                        IEVIEWEVENT event;
                        event.cbSize = sizeof(IEVIEWEVENT);
                        event.iType = IEE_CLEAR_LOG;
                        event.hwnd = dat->hwndLog;
                        event.hContact = dat->hContact;
                        CallService(MS_IEVIEW_EVENT, 0, (LPARAM)&event);
                    }
                    // IEVIew MOD End

                    SetDlgItemText(hwndDlg, IDC_LOG, _T(""));
                    dat->hDbEventFirst = NULL;
                    break;
                case IDC_PROTOCOL:
                    CallService(MS_USERINFO_SHOWDIALOG, (WPARAM) dat->hContact, 0);
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
                            if(iCurrentTypingMode) {
                                SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) Translate("Sending typing notifications disabled."));
                                SetSelftypingIcon(hwndDlg, dat, FALSE);
                            }
                            else {
                                SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) Translate("Sending typing notifications enabled."));
                                SetSelftypingIcon(hwndDlg, dat, TRUE);
                            }
                        }
                    }
                    break;
                case IDC_MESSAGE:
                    if (HIWORD(wParam) == EN_CHANGE) {
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
                    }
            }
            break;
        case WM_NOTIFY:
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
                            CHARFORMAT2 cf2;

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
                                        
                                        hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_CONTEXT));
                                        if(idFrom == IDC_LOG)
                                            hSubMenu = GetSubMenu(hMenu, 0);
                                        else
                                            hSubMenu = GetSubMenu(hMenu, 2);
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
                                            }
                                        }
#if defined(_UNICODE)
                                        if(idFrom == IDC_LOG)
                                            RemoveMenu(hSubMenu, 6, MF_BYPOSITION);
#endif                                        
                                        DestroyMenu(hMenu);
                                        if(dat->codePage != oldCodepage)
                                            SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                                        
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
            if((HWND)wParam == GetDlgItem(hwndDlg, IDC_CONTACTPIC)) {
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
                        char *contactName = (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)sendJobs[iFound].hContact[i], 0);
                        _snprintf(szErrMsg, sizeof(szErrMsg), "Multisend: failed sending to: %s", contactName);
                        LogErrorMessage(hwndDlg, dat, -1, szErrMsg);
                        goto verify;
                    }
                    else {
                        _snprintf(sendJobs[iFound].szErrorMsg, sizeof(sendJobs[iFound].szErrorMsg), "Delivery failure: %s", (char *)ack->lParam);
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
                dbei.cbBlob *= sizeof(TCHAR) + 1;
#endif
                dbei.pBlob = (PBYTE) sendJobs[iFound].sendBuffer;
                hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) sendJobs[iFound].hContact[i], (LPARAM) & dbei);
                SkinPlaySound("SendMsg");

                /*
                 * if this is a multisend job, AND the ack was from a different contact (not the session "owner" hContact)
                 * then we print a small message telling the user that the message has been delivered to *foo*
                 */

                if(sendJobs[iFound].hContact[i] != sendJobs[iFound].hOwner) {
                    char szErrMsg[256];
                    char *contactName = (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)sendJobs[iFound].hContact[i], 0);
                    _snprintf(szErrMsg, sizeof(szErrMsg), "Multisend: successfully sent to: %s", contactName);
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
                if(!myGlobals.m_IgnoreContactSettings)
                    DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "mwflags", dat->dwFlags & MWF_LOG_ALL);
            }
            DBWriteContactSettingDword(NULL, SRMSGMOD, "multisplit", dat->multiSplitterX);
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
                WindowList_Remove(hMessageWindowList, hwndDlg);
                CreateNewTabForContact(pNewContainer, dat->hContact, 0, NULL, TRUE, TRUE);
                SendMessage(hwndDlg, WM_CLOSE, 0, 1);
                if (iOldItems > 1) {                // there were more than 1 tab, container is still valid
                    RedrawWindow(dat->pContainer->hwndActive, NULL, NULL, RDW_INVALIDATE);
                }
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
            ShowPicture(hwndDlg, dat, TRUE, TRUE, TRUE);
            return 0;

        case DM_STATUSBARCHANGED:
            UpdateStatusBar(hwndDlg, dat);
            break;
        case DM_PICTHREADCOMPLETE:
            if(dat->hThread) {
                CloseHandle(dat->hThread);
                dat->hThread = 0;
                ShowPicture(hwndDlg, dat, FALSE, TRUE, FALSE);
            }
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
                if(result == GAIR_WAITFOR)
                    _DebugPopup(dat->hContact, "Retrieving Avatar...");
                else if(result == GAIR_SUCCESS) {
                    if(!DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "noremoteavatar", 0)) {
                        DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "MOD_Pic", pai_s.filename);
                        DBWriteContactSettingString(dat->hContact, "ContactPhoto", "File",pai_s.filename);
                        ShowPicture(hwndDlg, dat, FALSE, TRUE, TRUE);
                    }
                }
                SetWindowLong(hwndDlg, DWL_MSGRESULT, 1);
            }
            return 1;
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
            
            strncpy(szFilter, "Rich Edit file_*.rtf", MAX_PATH);
            szFilter[14] = '\0';
            strncpy(szFilename, (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)dat->hContact, 0), MAX_PATH);
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
            if(dat->dwTickLastEvent > dat->dwLastActivity)
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
            
            if((isForced = DBGetContactSettingDword(dat->hContact, "MetaContacts", "tabSRMM_forced", -1)) >= 0) {
                char szTemp[64];
                _snprintf(szTemp, sizeof(szTemp), "Status%d", isForced);
                if(DBGetContactSettingWord(dat->hContact, "MetaContacts", szTemp, 0) == ID_STATUS_OFFLINE) {
                    _DebugPopup(dat->hContact, Translate("MetaContact: The enforced protocol (%d) is now offline.\nReverting to default protocol selection."), isForced);
                    CallService(MS_MC_UNFORCESENDCONTACT, (WPARAM)dat->hContact, 0);
                    DBWriteContactSettingDword(dat->hContact, "MetaContacts", "tabSRMM_forced", -1);
                }
            }
            dat->hProtoIcon = (HICON)LoadSkinnedProtoIcon(GetCurrentMetaContactProto(hwndDlg, dat), ID_STATUS_ONLINE);
            if(dat->pContainer->hwndActive == hwndDlg && dat->pContainer->hwndStatus != 0) {
                SendMessage(dat->pContainer->hwndStatus, SB_SETICON, myGlobals.g_SecureIMAvail ? 3 : 2, (LPARAM)dat->hProtoIcon);
                UpdateStatusBarTooltips(hwndDlg, dat, -1);
            }
            break;
        }
		case DM_SECURE_CHANGED:
		{
			UpdateStatusBar(hwndDlg, dat);
			break;
		}
        case WM_INPUTLANGCHANGE:
            return DefWindowProc(hwndDlg, WM_INPUTLANGCHANGE, wParam, lParam);

// BEGIN MOD#11: Files beeing dropped ?
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
// END MOD#11
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
                
#if defined(_STREAMTHREADING)
                if(dat->pendingStream)
                    return TRUE;
#endif                
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
                    //RedrawWindow(dat->pContainer->hwndActive, NULL, NULL, RDW_INVALIDATE);
                    //UpdateWindow(dat->pContainer->hwndActive);
                }
                DestroyWindow(hwndDlg);
                if(iTabs == 1)
                    PostMessage(GetParent(GetParent(hwndDlg)), WM_CLOSE, 0, 1);
                else {
                    SendMessage(pContainer->hwnd, WM_SIZE, 0, 0);
                    SendMessage(pContainer->hwndActive, WM_SIZE, 0, 0);
                }
                //SetWindowLong(hwndDlg, GWL_USERDATA, 0);
                
                break;
            }
        case WM_DESTROY:
            TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_CLOSING, 0);
// BEGIN MOD#26: Autosave notsent message (by Corsario & bi0)
			if(dat->hContact) {
				TCHAR *AutosaveMessage;
#ifdef _UNICODE
                GETTEXTEX gtx;
#endif                
                
				int bufSize = GetWindowTextLength(GetDlgItem(hwndDlg,IDC_MESSAGE));
                if (bufSize) {
                    bufSize++;
    				AutosaveMessage=(TCHAR*)malloc(bufSize * sizeof(TCHAR));
#ifdef _UNICODE
                    gtx.cb = bufSize * sizeof(TCHAR);
                    gtx.codepage = 1200;
                    gtx.flags = GT_USECRLF;
                    gtx.lpDefaultChar = 0;
                    gtx.lpUsedDefChar = 0;
                    SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETTEXTEX, (WPARAM)&gtx, (LPARAM)AutosaveMessage);
    				// old RichEdit20W version GetDlgItemTextW(hwndDlg,IDC_MESSAGE,AutosaveMessage,bufSize);
                    _DBWriteContactSettingWString(dat->hContact, SRMSGMOD, "SavedMsgW", AutosaveMessage);
#else
                    GetDlgItemTextA(hwndDlg,IDC_MESSAGE,AutosaveMessage,bufSize);
                    DBWriteContactSettingString(dat->hContact,SRMSGMOD,"SavedMsg",(char *)AutosaveMessage);
#endif
    				free(AutosaveMessage);
                }
                else {
#ifdef _UNICODE
                    DBDeleteContactSetting(dat->hContact, SRMSGMOD, "SavedMsgW");
#else
                    DBDeleteContactSetting(dat->hContact,SRMSGMOD,"SavedMsg");
#endif
                }
			}
// END MOD#26

            /*
             * make sure to delete OUR last event...
             */
            /* if(dat->pContainer->dwTickLastEvent == dat->dwTickLastEvent)
                dat->pContainer->dwTickLastEvent = 0; */
            
            if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON) {
                NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
            }
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
            }
            
// BEGIN MOD#33: Show contact's picture
            if (dat->hContactPic && dat->hContactPic != myGlobals.g_hbmUnknown)
                DeleteObject(dat->hContactPic);
// END MOD#33
            if (dat->hSmileyIcon)
                DeleteObject(dat->hSmileyIcon);
            
            WindowList_Remove(hMessageWindowList, hwndDlg);
            SendMessage(hwndDlg, DM_SAVEPERCONTACT, 0, 0);
            if(myGlobals.m_SplitterSaveOnClose)
                SaveSplitter(hwndDlg, dat);
            if(!dat->stats.bWritten) {
                WriteStatsOnClose(hwndDlg, dat);
                dat->stats.bWritten = TRUE;
            }
            SetWindowLong(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), GWL_WNDPROC, (LONG) OldSplitterProc);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_WNDPROC, (LONG) OldSplitterProc);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_WNDPROC, (LONG) OldMessageEditProc);

            // remove temporary contacts...
            
            if (dat->hContact && DBGetContactSettingByte(NULL, SRMSGMOD_T, "deletetemp", 0)) {
                if (DBGetContactSettingByte(dat->hContact, "CList", "NotOnList", 0)) {
                    CallService(MS_DB_CONTACT_DELETE, (WPARAM)dat->hContact, 0);
                }
            }

            // metacontacts support

            if(dat->bIsMeta) {
                DBWriteContactSettingDword(dat->hContact, "MetaContacts", "tabSRMM_forced", -1);
                CallService(MS_MC_UNFORCESENDCONTACT, (WPARAM)dat->hContact, 0);
            }
            
// XXX tab support
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
                    dat->iTabID = -1;
                }
                
            }
// XXX end mod
            TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_CLOSE, 0);
            
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
            else
                MessageBoxA(0,"dat == 0 in WM_DESTROY", "Warning", MB_OK);
            SetWindowLong(hwndDlg, GWL_USERDATA, 0);
            break;
    }
    return FALSE;
}

/*
 * flash a tab icon if mode = true, otherwise restore default icon
 * store flashing state into bState
 */

static void FlashTab(HWND hwndTab, int iTabindex, BOOL *bState, BOOL mode, int flashImage, int origImage)
{
    TCITEM item;

    ZeroMemory((void *)&item, sizeof(item));
    item.mask = TCIF_IMAGE;

    if (mode) {

        if (*bState)
            item.iImage = flashImage;
        else
            item.iImage = origImage;

        *bState = !(*bState);

    } else
        item.iImage = origImage;

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
        WriteFile(hFile, pbBuff, cb, pcb, NULL);                                                         
        *pcb = cb;                                                                                       
        CloseHandle(hFile);                                                                              
        return 0;                                                                                        
    }                                                                                                    
    return 1;                                                                                            
}

