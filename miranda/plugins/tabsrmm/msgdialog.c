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
#include "../../include/m_clc.h"
#include "../../include/m_clui.h"
#include "../../include/m_userinfo.h"
#include "../../include/m_history.h"
#include "../../include/m_addcontact.h"
// BEGIN MOD#11: Files beeing dropped ?
#include "../../include/m_file.h"
// END MOD#11
#include "msgs.h"
#include "m_message.h"
#include "m_popup.h"
#include "m_smileyadd.h"

#define TIMERID_MSGSEND      0
#define TIMERID_ANTIBOMB     2
#define TIMERID_TYPE         3
#define TIMEOUT_ANTIBOMB     4000       // multiple-send bombproofing: send max 3 messages every 4 seconds
#define ANTIBOMB_COUNT       3
#define TIMEOUT_TYPEOFF      10000      // send type off after 10 seconds of inactivity
#define SB_CHAR_WIDTH        45

#ifdef __MATHMOD_SUPPORT
//mathMod begin
#define MTH_SHOW "Math/Show"
#define MTH_HIDE "Math/Hide"
#define MTH_RESIZE "Math/Resize"
#define MTH_SETFORMULA "Math/SetFormula"
#define MTH_Set_Srmm_HWND "Math/SetSrmmHWND" //übergibt fenster-Handle des aktuellen Message-Dialogs
#define MTH_GET_PREVIEW_HEIGHT "Math/getPreviewHeight"
#define MTH_GET_PREVIEW_SHOWN "Math/getPreviewShown"
#define MTH_SUBSTITUTE_DELIMITER "Math/SubstituteDelimiter"
typedef struct
{
    int top;
	int left;
	int right;
	int bottom;
}	TMathWindowInfo;
typedef struct
{
	HWND EditHandle;
	char* Substitute;
}	TMathSubstInfo;

//mathMod end
#endif

#ifdef __GNUWIN32__
#define SES_EXTENDBACKCOLOR 4           // missing from the mingw32 headers
#endif

extern int g_hotkeyHwnd;

int _log(const char *fmt, ...); // XXX debuglog
int GetTabIndexFromHWND(HWND hwndTab, HWND hwndDlg);
int ActivateTabFromHWND(HWND hwndTab, HWND hwndDlg);
int CutContactName(char *old, char *new, int size);
int GetProtoIconFromList(const char *szProto, int iStatus);
TCHAR *QuoteText(TCHAR *text, int charsPerLine, int removeExistingQuotes);
DWORD _DBGetContactSettingDword(HANDLE hContact, char *pszSetting, char *pszValue, DWORD dwDefault, int iIgnoreMode);
void _DBWriteContactSettingWString(HANDLE hContact, const char *szKey, const char *szSetting, const wchar_t *value);
int MessageWindowOpened(WPARAM wParam, LPARAM LPARAM);
static DWORD CALLBACK StreamOut(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb);

char *pszIDCSAVE_close = 0, *pszIDCSAVE_save = 0;

void FlashTab(HWND hwndTab, int iTabindex, BOOL *bState, BOOL mode, int flashImage, int origImage);

extern HCURSOR hCurSplitNS, hCurSplitWE, hCurHyperlinkHand;
extern HANDLE hMessageWindowList, hMessageSendList;
extern struct CREOleCallback reOleCallback;
extern HINSTANCE g_hInst, g_hIconDLL;

void ImageDataInsertBitmap(IRichEditOle *ole, HBITMAP hbm);

#if defined(_STREAMTHREADING)
    // stream thread stuff
    #define NR_STREAMJOBS 100
    
    extern HANDLE g_hStreamThread;
    extern int g_StreamThreadRunning;
    struct StreamJob StreamJobs[NR_STREAMJOBS + 2];
    int volatile g_StreamJobCurrent = 0;
    CRITICAL_SECTION sjcs;
    void ReplaceIcons(HWND hwndDlg, struct MessageWindowData *dat, LONG startAt);
#endif

HICON g_buttonBarIcons[NR_BUTTONBARICONS];
extern int g_SmileyAddAvail;

extern int g_IconEmpty, g_IconMsgEvent, g_IconTypingEvent, g_IconFileEvent, g_IconUrlEvent, g_IconError, g_IconSend;
extern HICON g_iconErr;

extern char *szWarnClose;
extern HMENU g_hMenuEncoding;

static void UpdateReadChars(HWND hwndDlg, HWND hwndStatus);
static int CheckValidSmileyPack(char *szProto, HICON *icon);
static void SetSelftypingIcon(HWND dlg, struct MessageWindowData *dat, int iMode);
static void HandleIconFeedback(HWND hwndDlg, struct MessageWindowData *dat, int iIcon);

// sending queue stuff

static int AddToSendQueue(HWND hwndDlg, struct MessageWindowData *dat, int iLen);
static int SendQueuedMessage(HWND hwndDlg, struct MessageWindowData *dat);
static void ShiftSendQueueDown(HWND hwndDlg, struct MessageWindowData *dat);
static void CheckSendQueue(HWND hwndDlg, struct MessageWindowData *dat);
static void LogErrorMessage(HWND hwndDlg, struct MessageWindowData *dat, int iSendJobIndex, char *szErrMsg);
static void UpdateUnsentDisplay(HWND hwndDlg, struct MessageWindowData *dat);
static void RecallFailedMessage(HWND hwndDlg, struct MessageWindowData *dat);
static void UpdateSaveAndSendButton(HWND hwndDlg, struct MessageWindowData *dat);
static void NotifyDeliveryFailure(HWND hwndDlg, struct MessageWindowData *dat);
static int CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void ShowErrorControls(HWND hwndDlg, struct MessageWindowData *dat, int showCmd);
static void EnableSending(HWND hwndDlg, struct MessageWindowData *dat, int iMode);
static void UpdateStatusBar(HWND hwndDlg, struct MessageWindowData *dat);
int MsgWindowMenuHandler(HWND hwndDlg, struct MessageWindowData *dat, int selection, int menuId);
int MsgWindowUpdateMenu(HWND hwndDlg, struct MessageWindowData *dat, HMENU submenu, int menuID);

extern BOOL CALLBACK SelectContainerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcContainerOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern int ActivateExistingTab(struct ContainerWindowData *pContainer, HWND hwndChild);
static void CreateSmileyIcon(struct MessageWindowData *dat, HICON hIcon);

struct ContainerWindowData *FindContainerByName(const TCHAR *name);
int GetContainerNameForContact(HANDLE hContact, TCHAR *szName, int iNameLen);
struct ContainerWindowData *CreateContainer(const TCHAR *name, int iMode, HANDLE hContactFrom);
HWND CreateNewTabForContact(struct ContainerWindowData *pContainer, HANDLE hContact, int isSend, const char *pszInitialText, BOOL bActivateTAb, BOOL bPopupContainer);

static WNDPROC OldMessageEditProc, OldSplitterProc;
static const UINT infoLineControls[] = { IDC_PROTOCOL, IDC_NAME};
static const UINT buttonLineControlsNew[] = { IDC_ADD, IDC_SMILEYBTN, IDC_PIC, IDC_HISTORY, IDC_TIME, IDC_MULTIPLE, IDC_QUOTE, IDC_SAVE};
static const UINT sendControls[] = { IDC_MESSAGE, IDC_LOG };
static const UINT errorControls[] = { IDC_STATICERRORICON, IDC_STATICTEXT, IDC_RETRY, IDC_CANCELSEND, IDC_MSGSENDLATER };

static char *MsgServiceName(HANDLE hContact)
{
#ifdef _UNICODE
    char szServiceName[100];
    char *szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
    if (szProto == NULL)
        return PSS_MESSAGE;

    _snprintf(szServiceName, sizeof(szServiceName), "%s%sW", szProto, PSS_MESSAGE);
    if (ServiceExists(szServiceName))
        return PSS_MESSAGE "W";
#endif
    return PSS_MESSAGE;
}

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
        ReplaceIcons(StreamJobs[0].hwndOwner, StreamJobs[0].dat, StreamJobs[0].startAt);
        EnterCriticalSection(&sjcs);
        StreamJobs[0].dat->pendingStream--;
        StreamJobs[0].dat->pContainer->pendingStream--;
        MoveMemory(&StreamJobs[0],  &StreamJobs[1], (g_StreamJobCurrent - 1) * sizeof(StreamJobs[0]));
        g_StreamJobCurrent--;
        LeaveCriticalSection(&sjcs);
    } while ( g_StreamThreadRunning );
    
	return 0;
}

#endif

DWORD WINAPI LoadPictureThread(LPVOID param)
{
    HBITMAP hBm;
    HWND hwndDlg = (HWND)param;
    struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwndDlg, GWL_USERDATA);
    DBVARIANT dbv;

    if(dat) {
        if (!DBGetContactSetting(dat->hContact, SRMSGMOD_T, "MOD_Pic",&dbv)) {
            hBm = (HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);
            DBFreeVariant(&dbv);
            if(hBm == 0)
                return 0;
            if(IsWindow(hwndDlg)) {
                dat = (struct MessageWindowData *)GetWindowLong(hwndDlg, GWL_USERDATA);
                if(dat) {
                    dat->hContactPic = hBm;
                    SendMessage((HWND)param, DM_PICTHREADCOMPLETE, 0, (LPARAM)hBm);
                }
                else
                    DeleteObject(hBm);
            }
            else
                DeleteObject(hBm);
        }
    }
    return 0;
}
// BEGIN MOD#11: Files beeing dropped ?
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
// END MOD#11

static void ShowMultipleControls(HWND hwndDlg, const UINT * controls, int cControls, int state)
{
    int i;
    for (i = 0; i < cControls; i++)
        ShowWindow(GetDlgItem(hwndDlg, controls[i]), state);
}

static void SetDialogToType(HWND hwndDlg)
{
    struct MessageWindowData *dat;
    WINDOWPLACEMENT pl = { 0};
    int iWantStatusbar = 0;
    HICON hButtonIcon = 0;
    int nrSmileys;
    int showInfo, showButton, showSend;
    
    dat = (struct MessageWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);
    showInfo = dat->showUIElements & MWF_UI_SHOWINFO;
    showButton = dat->showUIElements & MWF_UI_SHOWBUTTON;
    showSend = dat->showUIElements & MWF_UI_SHOWSEND;
    
    if (dat->hContact) {
        ShowMultipleControls(hwndDlg, infoLineControls, sizeof(infoLineControls) / sizeof(infoLineControls[0]), showInfo ? SW_SHOW : SW_HIDE);
    } else
        ShowMultipleControls(hwndDlg, infoLineControls, sizeof(infoLineControls) / sizeof(infoLineControls[0]), SW_HIDE);
    if (dat->hContact) {
        ShowMultipleControls(hwndDlg, buttonLineControlsNew, sizeof(buttonLineControlsNew) / sizeof(buttonLineControlsNew[0]), showButton ? SW_SHOW : SW_HIDE);

        if (!DBGetContactSettingByte(dat->hContact, "CList", "NotOnList", 0))
            ShowWindow(GetDlgItem(hwndDlg, IDC_ADD), SW_HIDE);
    } else {
        ShowMultipleControls(hwndDlg, buttonLineControlsNew, sizeof(buttonLineControlsNew) / sizeof(buttonLineControlsNew[0]), SW_HIDE);

        ShowWindow(GetDlgItem(hwndDlg, IDC_MULTIPLE), showButton ? SW_SHOW : SW_HIDE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_MULTIPLE), FALSE);
    }
    
    ShowMultipleControls(hwndDlg, sendControls, sizeof(sendControls) / sizeof(sendControls[0]), SW_SHOW);
    ShowMultipleControls(hwndDlg, errorControls, sizeof(errorControls) / sizeof(errorControls[0]), SW_HIDE);
    
    if (showButton) {
        ShowWindow(GetDlgItem(hwndDlg, IDC_MULTIPLE), SW_SHOW);
        EnableWindow(GetDlgItem(hwndDlg, IDC_MULTIPLE), TRUE);
    }

// smileybutton stuff...
    
    if(g_SmileyAddAvail && DBGetContactSettingByte(NULL, "SmileyAdd", "PluginSupportEnabled", 0)) {
        nrSmileys = CheckValidSmileyPack(dat->szProto, &hButtonIcon);

        if(hButtonIcon == 0) {
            SMADD_GETICON smadd_iconinfo;

            smadd_iconinfo.cbSize = sizeof(smadd_iconinfo);
            smadd_iconinfo.Protocolname = dat->szProto;
            smadd_iconinfo.SmileySequence = ":)";
            smadd_iconinfo.Smileylength = 0;
            CallService(MS_SMILEYADD_GETSMILEYICON, 0, (LPARAM)&smadd_iconinfo);

            if(dat->hSmileyIcon)
                DeleteObject(dat->hSmileyIcon);

            if(smadd_iconinfo.SmileyIcon) {
                CreateSmileyIcon(dat, smadd_iconinfo.SmileyIcon);
                SendDlgItemMessage(hwndDlg, IDC_SMILEYBTN, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)dat->hSmileyIcon);
            }
            else {
                dat->hSmileyIcon = 0;
                SendDlgItemMessage(hwndDlg, IDC_SMILEYBTN, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_buttonBarIcons[11]);
            }
        }
        else {
            SendDlgItemMessage(hwndDlg, IDC_SMILEYBTN, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hButtonIcon);
        }
    }
    
    if(nrSmileys == 0)
        dat->doSmileys = 0;
    
    ShowWindow(GetDlgItem(hwndDlg, IDC_SMILEYBTN), (dat->doSmileys && showButton) ? SW_SHOW : SW_HIDE);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SMILEYBTN), dat->doSmileys ? TRUE : FALSE);
    
    if(dat->pContainer->hwndActive == hwndDlg)
        UpdateReadChars(hwndDlg, dat->pContainer->hwndStatus);

    {
        TCHAR szSendLabel[50];
#if defined (_UNICODE)
        MultiByteToWideChar(CP_ACP, 0, Translate("&Xend"), -1, szSendLabel, 40);
#else
        strncpy(szSendLabel, Translate("&Xend"), 40);
#endif        
        SetDlgItemText(hwndDlg, IDOK, szSendLabel);
    }

// BEGIN MOD#33: Show contact's picture
    SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
    if (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "MOD_ShowPic",0))
        ShowWindow(GetDlgItem(hwndDlg,IDC_CONTACTPIC),SW_SHOW);
    else
        ShowWindow(GetDlgItem(hwndDlg,IDC_CONTACTPIC),SW_HIDE);
// END MOD#33

    if((showButton || showInfo || showSend))
        ShowWindow(GetDlgItem(hwndDlg, IDC_SPLITTER5), SW_SHOW);
    else
        ShowWindow(GetDlgItem(hwndDlg, IDC_SPLITTER5), SW_HIDE);

    ShowWindow(GetDlgItem(hwndDlg, IDC_TYPINGNOTIFY), SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_SPLITTER), SW_SHOW);
    ShowWindow(GetDlgItem(hwndDlg, IDOK), showSend ? SW_SHOW : SW_HIDE);

    ShowWindow(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), dat->multiple ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hwndDlg, IDC_CLIST), dat->multiple ? SW_SHOW : SW_HIDE);
    
    EnableWindow(GetDlgItem(hwndDlg, IDOK), GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE)) != 0);
        
    SendMessage(hwndDlg, DM_UPDATETITLE, 0, 1);
    SendMessage(hwndDlg, WM_SIZE, 0, 0);
    /*
    pl.length = sizeof(pl);
    GetWindowPlacement(hwndDlg, &pl);
    if (!IsWindowVisible(hwndDlg))
        pl.showCmd = SW_HIDE;
    SetWindowPlacement(hwndDlg, &pl);   //in case size is smaller than new minimum
    */
}

// BEGIN MOD#33: Show contact's picture
void ShowPicture(HWND hwndDlg, struct MessageWindowData *dat, BOOL changePic, BOOL showNewPic, BOOL startThread)
{
    DBVARIANT dbv;
    RECT rc;
    int picFailed = FALSE;
    int iUnknown = FALSE;
    
    if(changePic && startThread) {
        if (dat->hContactPic) {
            DeleteObject(dat->hContactPic);
            dat->hContactPic=NULL;
        }
        DBDeleteContactSetting(dat->hContact, SRMSGMOD_T, "MOD_Pic");
        DBDeleteContactSetting(dat->hContact, SRMSGMOD_T, "MOD_Pic_failed");
    }
    
    if (showNewPic) {
        if (startThread && dat->hContactPic) {
            DeleteObject(dat->hContactPic);
            dat->hContactPic=NULL;
        }
        if (DBGetContactSetting(dat->hContact, SRMSGMOD_T, "MOD_Pic",&dbv)) {
            if (!DBGetContactSetting(dat->hContact,"ContactPhoto","File",&dbv)) {
                DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "MOD_Pic" ,dbv.pszVal);
                DBFreeVariant(&dbv);
            } else {
                if (!DBGetContactSetting(dat->hContact, dat->szProto, "Photo", &dbv)) {
                    DBWriteContactSettingString(dat->hContact, SRMSGMOD_T,"MOD_Pic",dbv.pszVal);
                    DBFreeVariant(&dbv);
                }
                else
                    iUnknown = TRUE;
            }
        }
        if (!DBGetContactSetting(dat->hContact, SRMSGMOD_T, "MOD_Pic",&dbv) || iUnknown) {
            BITMAP bminfo;
            BOOL isNoPic = FALSE;
            int maxImageSizeX=500;
            int maxImageSizeY=300;
            DWORD dwThreadId;
            HANDLE hFile;
            
            /*
             * use non-blocking (in its own thread) avatar loading only if its not a local file (i.e. ICQ web photo).
             * note that remote avatars via MSN are considered to be local files, because downloading these is a
             * 2-stage process (notificaton about the change and a further notification when the file is ready
             */
            
            if(iUnknown) {
                dat->hContactPic = LoadImage(g_hIconDLL, MAKEINTRESOURCE(IDB_UNKNOWNAVATAR), IMAGE_BITMAP, 0, 0, 0);
            }
            else {
                if((hFile = CreateFileA(dbv.pszVal, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
                    if(startThread && dat->hThread == 0) {
                        dat->hThread = CreateThread(NULL, 0, LoadPictureThread, (LPVOID)hwndDlg, 0, &dwThreadId);
                        DBFreeVariant(&dbv);
                        return;
                    }
                }
                else {
                    CloseHandle(hFile);
                    dat->hContactPic=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);
                }
            }

            GetObject(dat->hContactPic,sizeof(bminfo),&bminfo);
            if (bminfo.bmWidth>maxImageSizeX || bminfo.bmWidth<=0 || bminfo.bmHeight<=0 || bminfo.bmHeight>maxImageSizeY) 
                isNoPic=TRUE;
            if (isNoPic) {
                if (!DBGetContactSettingByte(dat->hContact, SRMSGMOD_T,"MOD_Pic_failed",0)) {
                    _DebugPopup(dat->hContact, "%s %s", dbv.pszVal, Translate("has either a wrong size (max 150 x 150) or is not a recognized image file"));
                    DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "MOD_Pic_failed", 1);
                    picFailed = TRUE;
                }
                if(dat->hContactPic)
                    DeleteObject(dat->hContactPic);
                dat->hContactPic = 0;
            } else
                DBWriteContactSettingByte(dat->hContact,SRMSGMOD_T,"MOD_Pic_failed",0);
            DBFreeVariant(&dbv);
        } else {
            if(dat->hContactPic)
                DeleteObject(dat->hContactPic);
            dat->hContactPic = 0;
        }

        if(dat->hContactPic) {
            dat->showPic = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "MOD_ShowPic", 0);;
            ShowWindow(GetDlgItem(hwndDlg, IDC_CONTACTPIC), dat->showPic ? SW_SHOW : SW_HIDE);
            SendMessage(hwndDlg, DM_RECALCPICTURESIZE, 0, 0);
            SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
        }
        else {
            dat->showPic = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "MOD_ShowPic", 0);
            ShowWindow(GetDlgItem(hwndDlg, IDC_CONTACTPIC), dat->showPic ? SW_SHOW : SW_HIDE);
            dat->pic.cy = dat->pic.cx = 60;
            InvalidateRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), NULL, TRUE);
            SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
        }
    } else {
        dat->showPic = dat->showPic ? 0 : 1;
        DBWriteContactSettingByte(dat->hContact,SRMSGMOD_T,"MOD_ShowPic",(BYTE)dat->showPic);
        SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
    }

    GetWindowRect(GetDlgItem(hwndDlg,IDC_CONTACTPIC),&rc);
    if (dat->minEditBoxSize.cy+3>dat->splitterY)
        SendMessage(hwndDlg,DM_SPLITTERMOVED,(WPARAM)rc.bottom-dat->minEditBoxSize.cy,(LPARAM)GetDlgItem(hwndDlg,IDC_SPLITTER));
    if (!showNewPic)
        SetDialogToType(hwndDlg);
        //SendMessage(hwndDlg,DM_OPTIONSAPPLIED,0,1);
    else
        SendMessage(hwndDlg,WM_SIZE,0,0);

    SendMessage(hwndDlg, DM_CALCMINHEIGHT, 0, 0);
    SendMessage(dat->pContainer->hwnd, DM_REPORTMINHEIGHT, (WPARAM) hwndDlg, (LPARAM) dat->uMinHeight);
}
// END MOD#33

struct MsgEditSubclassData {
    DWORD lastEnterTime;
};

#define EM_SUBCLASSED             (WM_USER+0x101)
#define EM_UNSUBCLASSED           (WM_USER+0x102)
#define ENTERCLICKTIME   1000   //max time in ms during which a double-tap on enter will cause a send
#define EDITMSGQUEUE_PASSTHRUCLIPBOARD  //if set the typing queue won't capture ctrl-C etc because people might want to use them on the read only text
                                                  //todo: decide if this should be set or not
static LRESULT CALLBACK MessageEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct MsgEditSubclassData *dat;
    struct MessageWindowData *mwdat = (struct MessageWindowData *)GetWindowLong(GetParent(hwnd), GWL_USERDATA);

    dat = (struct MsgEditSubclassData *) GetWindowLong(hwnd, GWL_USERDATA);
    switch (msg) {
// BEGIN MOD#11: Files beeing dropped ?
		case WM_DROPFILES:
			SendMessage(GetParent(hwnd),WM_DROPFILES,(WPARAM)wParam,(LPARAM)lParam);
			break;
// END MOD#11
        case EM_SUBCLASSED:
            dat = (struct MsgEditSubclassData *) malloc(sizeof(struct MsgEditSubclassData));
            SetWindowLong(hwnd, GWL_USERDATA, (LONG) dat);
            dat->lastEnterTime = 0;
            return 0;
        case WM_CHAR:
            if (wParam == 21 && GetKeyState(VK_CONTROL) & 0x8000) {             // ctrl-U next unread tab
                SendMessage(GetParent(hwnd), DM_QUERYPENDING, DM_QUERY_NEXT, 0);
                return 0;
            }
            if (wParam == 23 && GetKeyState(VK_CONTROL) & 0x8000) {             // ctrl-w close tab
                SendMessage(GetParent(hwnd), WM_CLOSE, 1, 0);
                return 0;
            }
            if (wParam == 18 && GetKeyState(VK_CONTROL) & 0x8000) {             // ctrl-r most recent unread
                SendMessage(GetParent(hwnd), DM_QUERYPENDING, DM_QUERY_MOSTRECENT, 0);
                return 0;
            }
            if (GetWindowLong(hwnd, GWL_STYLE) & ES_READONLY)
                break;
            if (wParam == 0x0c && GetKeyState(VK_CONTROL) & 0x8000) {
                SendMessage(GetParent(hwnd), WM_COMMAND, IDM_CLEAR, 0);         // ctrl-l (clear log)
                return 0;
            }
        case WM_KEYUP:
            break;
        case WM_KEYDOWN:
            if(wParam == VK_RETURN) {
                if (GetKeyState(VK_SHIFT) & 0x8000) {
                    PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
                    return 0;
                }
                if (((GetKeyState(VK_CONTROL) & 0x8000) != 0) ^ (0 != DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SENDONENTER, SRMSGDEFSET_SENDONENTER))) {
                    PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
                    return 0;
                }
                if (DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SENDONDBLENTER, SRMSGDEFSET_SENDONDBLENTER)) {
                    if (dat->lastEnterTime + ENTERCLICKTIME < GetTickCount())
                        dat->lastEnterTime = GetTickCount();
                    else {
                        SendMessage(hwnd, WM_CHAR, '\b', 0);
                        PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
                        return 0;
                    }
                }
            }
            if ((GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000)) {
                if (wParam == 0x9) {            // ctrl-shift tab
                    SendMessage(GetParent(hwnd), DM_SELECTTAB, DM_SELECT_PREV, 0);
                    return 0;
                }
            }
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                if (wParam == VK_TAB) {
                    SendMessage(GetParent(hwnd), DM_SELECTTAB, DM_SELECT_NEXT, 0);
                    return 0;
                }
                if (wParam == VK_F4) {
                    SendMessage(GetParent(hwnd), WM_CLOSE, 1, 0);
                    return 0;
                }
                if (!(GetKeyState(VK_SHIFT) & 0x8000) && (wParam == VK_UP || wParam == VK_DOWN)) {          // input history scrolling (ctrl-up / down)
                    if(mwdat) {
                        if(mwdat->history != NULL && mwdat->history[0].szText != NULL) {      // at least one entry needs to be alloced, otherwise we get a nice infinite loop ;)
                            if(mwdat->dwFlags & MWF_NEEDHISTORYSAVE) {
                                mwdat->iHistoryCurrent = mwdat->iHistoryTop;
                                if(GetWindowTextLengthA(hwnd) > 0)
                                    SendMessage(GetParent(hwnd), DM_SAVEINPUTHISTORY, (WPARAM)mwdat->iHistorySize, 0);
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
                                    SendMessage(hwnd, EM_REPLACESEL, TRUE, (LPARAM) mwdat->history[mwdat->iHistorySize].szText);
                                }
                            }
                            else {
                                if(mwdat->history[mwdat->iHistoryCurrent].szText != NULL) {
                                    SetWindowText(hwnd, _T(""));
                                    SendMessage(hwnd, EM_REPLACESEL, TRUE, (LPARAM) mwdat->history[mwdat->iHistoryCurrent].szText);
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
                            if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "hotkeys", 0)) {
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
// XXX tabmod
            if (wParam == VK_RETURN)
                break;
            //fall through
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_MOUSEWHEEL:
        case WM_KILLFOCUS:
            dat->lastEnterTime = 0;
            break;
        case WM_SYSCHAR:
            dat->lastEnterTime = 0;
            if(VkKeyScan((TCHAR)wParam) == 'S' && GetKeyState(VK_MENU) && 0x8000) {
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
                if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "al", 0)) {
                    SendMessage(GetParent(hwnd), DM_SETLOCALE, wParam, lParam);
                    PostMessage(GetParent(hwnd), DM_SAVELOCALE, 0, 0);
                    return DefWindowProc(hwnd, WM_INPUTLANGCHANGEREQUEST, wParam, lParam);
                }
                break;
            }
        case EM_UNSUBCLASSED:
            free(dat);
            return 0;
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
                SetCursor(rc.right > rc.bottom ? hCurSplitNS : hCurSplitWE);
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
    int showInfo, showButton, showSend;
    struct MessageWindowData *dat = (struct MessageWindowData *) lParam;
    int iClistOffset = 0;

    showInfo = dat->showUIElements & MWF_UI_SHOWINFO;
    showButton = dat->showUIElements & MWF_UI_SHOWBUTTON;
    showSend = dat->showUIElements & MWF_UI_SHOWSEND;

    if(dat->dwFlags & MWF_CLISTMODE) {
        RECT rc;
        GetClientRect(GetDlgItem(hwndDlg, IDC_LOG), &rc);
        iClistOffset = rc.bottom;
    }

    if (!showInfo && !showButton) {
        int i;
        for (i = 0; i < sizeof(buttonLineControlsNew) / sizeof(buttonLineControlsNew[0]); i++)
            if (buttonLineControlsNew[i] == urc->wId)
                OffsetRect(&urc->rcItem, 0, -(dat->splitterY +10));
    }
    switch (urc->wId) {
        case IDC_NAME:
            {
                int len;
                HWND h;

                h = GetDlgItem(hwndDlg, IDC_NAME);
                len = GetWindowTextLength(h);
                if (len > 0) {
                    HDC hdc;
                    SIZE textSize;
                    TCHAR buf[256];

                    GetWindowText(h, buf, sizeof(buf));
                    hdc = GetDC(h);
                    SelectObject(hdc, (HFONT) SendMessage(GetDlgItem(hwndDlg, IDOK1), WM_GETFONT, 0, 0));
                    GetTextExtentPoint32(hdc, buf, lstrlen(buf), &textSize);
                    urc->rcItem.right = urc->rcItem.left + textSize.cx + 12;        // padding
                    if (showButton && urc->rcItem.right > urc->dlgNewSize.cx - dat->nLabelRight)
                        urc->rcItem.right = urc->dlgNewSize.cx - dat->nLabelRight;
                    ReleaseDC(h, hdc);
                }
                urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
                urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
                return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
            }
        case IDC_PROTOCOL:
        case IDC_ADD:
        case IDC_SMILEYBTN:
        case IDC_PIC:
        case IDC_USERMENU:
        case IDC_DETAILS:
        case IDC_HISTORY:
        case IDC_TIME:
        case IDC_MULTIPLE:
        case IDC_QUOTE:
        case IDC_SAVE:
        case IDC_RETRY:
        case IDC_CANCELSEND:
        case IDC_MSGSENDLATER:
        case IDC_STATICTEXT:
        case IDC_STATICERRORICON:
            if(urc->wId == IDC_RETRY || urc->wId == IDC_CANCELSEND || urc->wId == IDC_MSGSENDLATER || urc->wId == IDC_STATICTEXT || urc->wId == IDC_STATICERRORICON)
                return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;

            if(urc->wId != IDC_PROTOCOL)
                OffsetRect(&urc->rcItem, 9, 0);
            if (dat->multiple && !iClistOffset && (urc->wId != IDC_PROTOCOL))
                OffsetRect(&urc->rcItem, -(dat->multiSplitterX + 3), 0);        // fix multisplitter clipping /w some visual styles

            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if (urc->wId == IDC_PROTOCOL)
                return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
            if (showSend) {
                urc->rcItem.left -= 40;
                urc->rcItem.right -= 40;
            }
            if (dat->showPic && (dat->splitterY <= (dat->bottomOffset + 27)))
                OffsetRect(&urc->rcItem, -(dat->pic.cx + 2), 0);

            return RD_ANCHORX_RIGHT | RD_ANCHORY_BOTTOM;
        case IDC_LOG:
            if(dat->dwFlags & MWF_ERRORSTATE)
                urc->rcItem.top += 38;
            if (dat->multiple)
                urc->rcItem.right -= (dat->multiSplitterX + 3);
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if (!showButton && !showSend && !showInfo)
                urc->rcItem.bottom += 24;
            else
                urc->rcItem.bottom -= 1;
            return RD_ANCHORX_WIDTH | RD_ANCHORY_HEIGHT;
        case IDC_SPLITTER:
        case IDC_SPLITTER5:
            urc->rcItem.right +=1;
            if (dat->multiple && !iClistOffset)
                urc->rcItem.right -= (dat->multiSplitterX + 3);
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if (urc->wId == IDC_SPLITTER && dat->splitterY <= (dat->bottomOffset + 27) && dat->showPic)
                urc->rcItem.right -= (dat->pic.cx + 4);
            return RD_ANCHORX_WIDTH | RD_ANCHORY_BOTTOM;
        case IDC_CONTACTPIC:
            urc->rcItem.top=urc->rcItem.bottom-(dat->pic.cy +2);
            urc->rcItem.left=urc->rcItem.right-(dat->pic.cx +2);

            if (dat->multiple && !iClistOffset ) {
                urc->rcItem.left-=(dat->multiSplitterX +2);
                urc->rcItem.right-=(dat->multiSplitterX +2);
            }
            return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
        case IDC_MESSAGE:
            {
                urc->rcItem.right = urc->dlgNewSize.cx;
                if (dat->showPic)
                    urc->rcItem.right -= dat->pic.cx+4;
                if (dat->multiple && !iClistOffset) {
                    urc->rcItem.right -= (dat->multiSplitterX + 3);
                }
                urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
                return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
            }
        case IDOK:
            OffsetRect(&urc->rcItem, 9, 0);
            if (dat->multiple && !iClistOffset) {
                urc->rcItem.left -= (dat->multiSplitterX +3);      // fix multisplitter clipping /w some visual styles...
                urc->rcItem.right -= (dat->multiSplitterX +3);     // dito
            }
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            if (dat->showPic && (dat->splitterY <= (dat->bottomOffset + 27)))
                OffsetRect(&urc->rcItem, -(dat->pic.cx + 2), 0);
            return RD_ANCHORX_RIGHT | RD_ANCHORY_BOTTOM;
        case IDC_MULTISPLITTER:
            urc->rcItem.left -= dat->multiSplitterX;
            urc->rcItem.right -= dat->multiSplitterX;
            if(iClistOffset) {
                urc->rcItem.bottom = iClistOffset;
                return RD_ANCHORX_RIGHT | RD_ANCHORY_CUSTOM;
            }
            return RD_ANCHORX_RIGHT | RD_ANCHORY_HEIGHT;
        case IDC_CLIST:
            urc->rcItem.left = urc->dlgNewSize.cx - dat->multiSplitterX;
            urc->rcItem.right = urc->dlgNewSize.cx - 3;
            if(iClistOffset) {
                urc->rcItem.bottom = iClistOffset + 3;
                return RD_ANCHORX_CUSTOM | RD_ANCHORY_CUSTOM;
            }
            return RD_ANCHORX_CUSTOM | RD_ANCHORY_HEIGHT;
        case IDC_TYPINGNOTIFY:
            return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
    }
    return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
}

static void UpdateReadChars(HWND hwndDlg, HWND hwndStatus)
{
    if (hwndStatus && SendMessage(hwndStatus, SB_GETPARTS, 0, 0) == 4) {
        TCHAR buf[128];
        int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE));

        _sntprintf(buf, sizeof(buf), _T("%d"), len);
        SendMessage(hwndStatus, SB_SETTEXT, 2, (LPARAM) buf);
    }
}

#ifdef __MATHMOD_SUPPORT
//mathMod begin
void updatePreview(HWND hwndDlg)
{	
	TMathWindowInfo mathWndInfo;

	int len=GetWindowTextLengthA( GetDlgItem( hwndDlg, IDC_MESSAGE) );
	RECT windRect;
	char * thestr = malloc(len+5);
	GetWindowTextA( GetDlgItem( hwndDlg, IDC_MESSAGE),(LPTSTR) thestr, len+1);
	// größe ermitteln:
	GetWindowRect(hwndDlg,&windRect);
	mathWndInfo.top=windRect.top;
	mathWndInfo.left=windRect.left;
	mathWndInfo.right=windRect.right;
	mathWndInfo.bottom=windRect.bottom;

	CallService(MTH_Set_Srmm_HWND,0,(LPARAM)hwndDlg); 
	CallService(MTH_SETFORMULA,0,(LPARAM) thestr);
	CallService(MTH_RESIZE,0,(LPARAM) &mathWndInfo);
	free(thestr);
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
        if(dat == NULL && (msg == WM_ACTIVATE || msg == WM_SETFOCUS)) {
            //_DebugPopup(NULL, "dat == 0 on %x (window = %d)", msg, hwndDlg);
            return 0;
        }
        
    }
    switch (msg) {
        case WM_INITDIALOG:
            {
                RECT rc, rc2;
                POINT pt;
                int i;

                struct NewMessageWindowLParam *newData = (struct NewMessageWindowLParam *) lParam;
                //TranslateDialogDefault(hwndDlg);
                dat = (struct MessageWindowData *) malloc(sizeof(struct MessageWindowData));
                ZeroMemory((void *) dat, sizeof(struct MessageWindowData));
                if (newData->iTabID >= 0) {
                    dat->pContainer = newData->pContainer;
                } else {      // toplevel
                    dat->pContainer = NULL;
                }
                SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) dat);

                dat->wOldStatus = -1;
                dat->iOldHash = -1;

                newData->item.lParam = (LPARAM) hwndDlg;
                TabCtrl_SetItem(hwndTab, newData->iTabID, &newData->item);

                pszIDCSAVE_close = Translate("Close session");
                pszIDCSAVE_save = Translate("Save and close session");

                dat->hContact = newData->hContact;
                dat->szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)dat->hContact, 0);
                
                if(dat->hContact && dat->szProto != NULL)
                    dat->wStatus = DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE);
                else
                    dat->wStatus = ID_STATUS_OFFLINE;
                
                dat->hwnd = hwndDlg;
                
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
                dat->history[dat->iHistorySize].szText = (TCHAR *)malloc((HISTORY_INITIAL_ALLOCSIZE + 1) * sizeof(TCHAR));
                dat->history[dat->iHistorySize].lLen = HISTORY_INITIAL_ALLOCSIZE;

                // send jobs
                for(i = 0; i < NR_SENDJOBS; i++) {
                    dat->sendJobs[i].sendBuffer = NULL;
                    dat->sendJobs[i].sendInfo = NULL;
                    dat->sendJobs[i].dwLen = 0;
                }
                
                if(!DBGetContactSettingByte(NULL, SRMSGMOD_T, "splitteredges", 1)) {
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
                    SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER5), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER5), GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
                }
                SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
                dat->bTabFlash = FALSE;
                dat->mayFlashTab = FALSE;
                dat->iTabImage = newData->iTabImage;
                dat->hAckEvent = NULL;
                dat->dwTickLastEvent = 0;
                dat->showUIElements = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWINFOLINE, SRMSGDEFSET_SHOWINFOLINE) ? MWF_UI_SHOWINFO : 0;
                dat->showUIElements |= DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWBUTTONLINE, SRMSGDEFSET_SHOWBUTTONLINE) ? MWF_UI_SHOWBUTTON : 0;
                dat->showUIElements |= DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SENDBUTTON, SRMSGDEFSET_SENDBUTTON) ? MWF_UI_SHOWSEND : 0;
                //dat->hNewEvent = HookEventMessage(ME_DB_EVENT_ADDED, hwndDlg, HM_DBEVENTADDED);
                dat->hBkgBrush = NULL;
                dat->hInputBkgBrush = NULL;
                dat->hDbEventFirst = NULL;
                dat->sendBuffer = NULL;
                dat->multiSplitterX = (int) DBGetContactSettingDword(NULL, SRMSGMOD, "multisplit", 150);
                dat->multiple = 0;
                dat->nTypeSecs = 0;
                dat->nLastTyping = 0;
                dat->showTyping = 0;
                dat->showTypingWin = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGWIN, SRMSGDEFSET_SHOWTYPINGWIN);
                dat->nTypeMode = PROTOTYPE_SELFTYPING_OFF;
                SetTimer(hwndDlg, TIMERID_TYPE, 1000, NULL);
                dat->lastMessage = 0;
                dat->iLastEventType = 0xffffffff;
                
                // load log option flags...
                dat->dwFlags = (DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT) & MWF_LOG_ALL);

                dat->doSmileys = DBGetContactSettingByte(NULL, "SmileyAdd", "PluginSupportEnabled", 0);
                
                if(!DBGetContactSettingByte(NULL, SRMSGMOD_T, "ignorecontactsettings", 0)) {
                    DWORD dwLocalFlags = 0;
                    int dwLocalSmAdd = 0;
                    int doLimitAvatar = dat->dwFlags & MWF_LOG_LIMITAVATARHEIGHT;
                    
                    if(dat->hContact) {
                        dwLocalFlags = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "mwflags", 0xffffffff);
                        if(dwLocalFlags != 0xffffffff)
                            dat->dwFlags = dwLocalFlags | ((dat->dwFlags & MWF_LOG_LIMITAVATARHEIGHT) ? MWF_LOG_LIMITAVATARHEIGHT : 0);
                        dwLocalSmAdd = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "doSmileys", 0xffffffff);
                        if(dwLocalSmAdd != 0xffffffff)
                            dat->doSmileys = dwLocalSmAdd;
                    }
                }
                if(!g_SmileyAddAvail)
                    dat->doSmileys = 0;
                
                if(dat->hContact) {
                    dat->codePage = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "ANSIcodepage", CP_ACP);
                    dat->dwFlags |= DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 0) ? MWF_LOG_RTL : 0;
                }

// BEGIN MOD#33: Show contact's picture
                dat->showPic = DBGetContactSettingByte(dat->hContact,SRMSGMOD_T,"MOD_ShowPic",0);
// END MOD#33
                SendMessage(hwndDlg, DM_LOADSPLITTERPOS, 0, 0);

                GetWindowRect(GetDlgItem(hwndDlg, IDC_SMILEYBTN), &rc);
                GetWindowRect(hwndDlg, &rc2);
                dat->nLabelRight = rc2.right - rc.left;

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

                GetWindowRect(GetDlgItem(hwndDlg, IDC_ADD), &rc);

                WindowList_Add(hMessageWindowList, hwndDlg, dat->hContact);

                SendDlgItemMessage(hwndDlg, IDC_ADD, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_buttonBarIcons[0]);
                SendDlgItemMessage(hwndDlg, IDC_HISTORY, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_buttonBarIcons[1]);
                SendDlgItemMessage(hwndDlg, IDC_TIME, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_buttonBarIcons[2]);
                SendDlgItemMessage(hwndDlg, IDC_MULTIPLE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_buttonBarIcons[3]);
                SendDlgItemMessage(hwndDlg, IDC_QUOTE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_buttonBarIcons[8]);
                SendDlgItemMessage(hwndDlg, IDC_SAVE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_buttonBarIcons[6]);
                SendDlgItemMessage(hwndDlg, IDC_PIC, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_buttonBarIcons[10]);
                SendDlgItemMessage(hwndDlg, IDOK, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_buttonBarIcons[9]);
                SendDlgItemMessage(hwndDlg, IDC_MULTIPLE, BUTTONSETASPUSHBTN, 0, 0);
                SendDlgItemMessage(hwndDlg, IDC_STATICERRORICON, STM_SETICON, (WPARAM)g_iconErr, 0);
                
            // Make them flat buttons
                if (!DBGetContactSettingByte(NULL, SRMSGMOD_T, "fulluin", 1)) {
                    SendDlgItemMessage(hwndDlg, IDC_NAME, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_buttonBarIcons[4]);
                }
                if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "nlflat", 0)) {
                    for (i = 0; i < sizeof(buttonLineControlsNew) / sizeof(buttonLineControlsNew[0]); i++)
                        SendMessage(GetDlgItem(hwndDlg, buttonLineControlsNew[i]), BUTTONSETASFLATBTN, 0, 0);
                    for (i = 0; i < sizeof(errorControls) / sizeof(errorControls[0]); i++)
                        SendMessage(GetDlgItem(hwndDlg, errorControls[i]), BUTTONSETASFLATBTN, 0, 0);
                    SendMessage(GetDlgItem(hwndDlg, IDOK), BUTTONSETASFLATBTN, 0, 0);
                    SendMessage(GetDlgItem(hwndDlg, IDC_NAME), BUTTONSETASFLATBTN, 0, 0);
                    SendMessage(GetDlgItem(hwndDlg, IDC_PROTOCOL), BUTTONSETASFLATBTN, 0, 0);
                }

                dat->dwFlags |= MWF_INITMODE;
                
                SendMessage(GetDlgItem(hwndDlg, IDC_ADD), BUTTONADDTOOLTIP, (WPARAM) Translate("Add Contact Permanently to List"), 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_HISTORY), BUTTONADDTOOLTIP, (WPARAM) Translate("View User's History"), 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_TIME), BUTTONADDTOOLTIP, (WPARAM) Translate("Message Log Options"), 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_MULTIPLE), BUTTONADDTOOLTIP, (WPARAM) Translate("Send Message to Multiple Users"), 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_QUOTE), BUTTONADDTOOLTIP, (WPARAM) Translate("Quote Text"), 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_PIC), BUTTONADDTOOLTIP, (WPARAM) Translate("Avatar Options"), 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_SMILEYBTN), BUTTONADDTOOLTIP, (WPARAM) Translate("Insert Emoticon"), 0);

                SendMessage(GetDlgItem(hwndDlg, IDC_SAVE), BUTTONADDTOOLTIP, (WPARAM) pszIDCSAVE_close, 0);
                SendMessage(GetDlgItem(hwndDlg, IDOK), BUTTONADDTOOLTIP, (WPARAM) Translate("Send message"), 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_PROTOCOL), BUTTONADDTOOLTIP, (WPARAM) Translate("View User's Details"), 0);

                EnableWindow(GetDlgItem(hwndDlg, IDC_TYPINGNOTIFY), FALSE);
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETOLECALLBACK, 0, (LPARAM) & reOleCallback);
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_LINK);
                SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_LINK | ENM_CHANGE);
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETUNDOLIMIT, 0, 0);

                /* OnO: higligh lines to their end */
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETEDITSTYLE, SES_EXTENDBACKCOLOR, SES_EXTENDBACKCOLOR);
                
                /* duh, how come we didnt use this from the start? */
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_AUTOURLDETECT, (WPARAM) TRUE, 0);
                if (dat->hContact) {
                    int  pCaps;
                    HANDLE hItem;
                    
                    hItem = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM) dat->hContact, 0);
                    if (hItem)
                        SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM) hItem, 1);
                    
                    if (dat->szProto) {
                        int nMax;
                        nMax = CallProtoService(dat->szProto, PS_GETCAPS, PFLAG_MAXLENOFMESSAGE, (LPARAM) dat->hContact);
                        if (nMax)
                            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_LIMITTEXT, (WPARAM) nMax, 0);
                    /* get around a lame bug in the Windows template resource code where richedits are limited to 0x7FFF */
                        SendDlgItemMessage(hwndDlg, IDC_LOG, EM_LIMITTEXT, (WPARAM) sizeof(TCHAR) * 0x7FFFFFFF, 0);
                        pCaps = CallProtoService(dat->szProto, PS_GETCAPS, PFLAGNUM_4, 0);
                        if(pCaps & PF4_AVATARS) {
                            SendMessage(hwndDlg, DM_RETRIEVEAVATAR, 0, 0);
                        }
                        else
                            dat->hPictAck = 0;
                    }
                }
                OldMessageEditProc = (WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_WNDPROC, (LONG) MessageEditSubclassProc);
                SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SUBCLASSED, 0, 0);
            // Hack alert.  Read the CList/UseGroups setting an hide/show groups in our clist
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

                if (dat->hContact) {
                    int historyMode = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_LOADHISTORY, SRMSGDEFSET_LOADHISTORY);
                // This finds the first message to display, it works like shit
                    dat->hDbEventFirst = (HANDLE) CallService(MS_DB_EVENT_FINDFIRSTUNREAD, (WPARAM) dat->hContact, 0);
// BEGIN MOD#32: Use different fonts for old history events
                    dat->isHistoryCount=0;
// END MOD#32
                    switch (historyMode) {
                        case LOADHISTORY_COUNT:
                            {
                                int i;
                                HANDLE hPrevEvent;
                                DBEVENTINFO dbei = { 0};
                                dbei.cbSize = sizeof(dbei);
                                for (i = DBGetContactSettingWord(NULL, SRMSGMOD, SRMSGSET_LOADCOUNT, SRMSGDEFSET_LOADCOUNT); i > 0; i--) {
                                    if (dat->hDbEventFirst == NULL)
                                        hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDLAST, (WPARAM) dat->hContact, 0);
                                    else
                                        hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDPREV, (WPARAM) dat->hDbEventFirst, 0);
                                    if (hPrevEvent == NULL)
                                        break;
                                    dbei.cbBlob = 0;
                                    dat->hDbEventFirst = hPrevEvent;
                                    CallService(MS_DB_EVENT_GET, (WPARAM) dat->hDbEventFirst, (LPARAM) & dbei);
                                    if (!DbEventIsShown(dat, &dbei))
                                        i++;
// BEGIN MOD#32: Use different fonts for old history events
                                    dat->isHistoryCount++;
// END MOD#32
                                }
                                break;
                            }
                        case LOADHISTORY_TIME:
                            {
                                HANDLE hPrevEvent;
                                DBEVENTINFO dbei = { 0};
                                DWORD firstTime;

                                dbei.cbSize = sizeof(dbei);
                                if (dat->hDbEventFirst == NULL)
                                    dbei.timestamp = time(NULL);
                                else
                                    CallService(MS_DB_EVENT_GET, (WPARAM) dat->hDbEventFirst, (LPARAM) & dbei);
                                firstTime = dbei.timestamp - 60 * DBGetContactSettingWord(NULL, SRMSGMOD, SRMSGSET_LOADTIME, SRMSGDEFSET_LOADTIME);
                                for (;;) {
                                    if (dat->hDbEventFirst == NULL)
                                        hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDLAST, (WPARAM) dat->hContact, 0);
                                    else
                                        hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDPREV, (WPARAM) dat->hDbEventFirst, 0);
                                    if (hPrevEvent == NULL)
                                        break;
                                    dbei.cbBlob = 0;
                                    CallService(MS_DB_EVENT_GET, (WPARAM) hPrevEvent, (LPARAM) & dbei);
                                    if (dbei.timestamp < firstTime)
                                        break;
                                    dat->hDbEventFirst = hPrevEvent;
// BEGIN MOD#32: Use different fonts for old history events
                                    dat->isHistoryCount++;
// END MOD#32
                                }
                                break;
                            }
                        default:
                            {
// BEGIN MOD#32: Use different fonts for old history events
                                dat->isHistoryCount=0;
// END MOD#32
                                break;
                            }
                    }
                }
                if(newData->iActivate) {
                    SendMessage(hwndDlg, DM_OPTIONSAPPLIED, 0, 0);
                    dat->dwFlags &= ~MWF_INITMODE;
                }
                
                
                if (dat->hContact == NULL) {
                    CheckDlgButton(hwndDlg, IDC_MULTIPLE, BST_CHECKED);
                    SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_MULTIPLE, BN_CLICKED), 0);
                }
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
                        if(dbv.type == DBVT_ASCIIZ && dbv.cchVal > 0)  { // at least the 0x0000 is always there... 
                            SetDlgItemTextW(hwndDlg, IDC_MESSAGE, (LPCWSTR)Utf8Decode(dbv.pszVal));
                        }
#else
    				if(!DBGetContactSetting(dat->hContact, SRMSGMOD, "SavedMsg", &dbv)) {
                        if(dbv.cchVal > 0)
                            SetDlgItemTextA(hwndDlg, IDC_MESSAGE, dbv.pszVal);
#endif
                        DBFreeVariant(&dbv);
    					{
    						int len = GetWindowTextLength(GetDlgItem(hwndDlg,IDC_MESSAGE));
                            if(dat->pContainer->hwndActive == hwndDlg)
                                UpdateReadChars(hwndDlg, dat->pContainer->hwndStatus);
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
                if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "al", 0) && dat->hContact != 0) {
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
                        if (dat->hkl)
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
            // XXX autolocale
                SendMessage(dat->pContainer->hwnd, DM_QUERYCLIENTAREA, 0, (LPARAM)&rc);
                if (newData->iActivate) {
                    if(!dat->hThread)
                        ShowPicture(hwndDlg,dat,FALSE,TRUE, TRUE);
                    SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
                    SetWindowPos(hwndDlg, HWND_TOP, rc.left + 1, rc.top, (rc.right - rc.left) - 8, (rc.bottom - rc.top) - 2, 0);
                    SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                    if(IsIconic(dat->pContainer->hwnd) && DBGetContactSettingByte(NULL, SRMSGMOD_T, "autoswitchtabs", 0)) {
                        dat->iFlashIcon = g_IconMsgEvent;
                        SetTimer(hwndDlg, TIMERID_FLASHWND, TIMEOUT_FLASHWND, NULL);
                        dat->mayFlashTab = TRUE;
                        dat->dwTickLastEvent = GetTickCount();
                    }
                    ShowWindow(hwndDlg, SW_SHOW);
                    dat->pContainer->hwndActive = hwndDlg;
                    SetActiveWindow(hwndDlg);
                    SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                }
                else {
                    dat->dwFlags |= (MWF_WASBACKGROUNDCREATE | MWF_NEEDCHECKSIZE);
                    dat->iFlashIcon = g_IconMsgEvent;
                    SetTimer(hwndDlg, TIMERID_FLASHWND, TIMEOUT_FLASHWND, NULL);
                    dat->mayFlashTab = TRUE;
                    dat->dwTickLastEvent = GetTickCount();
                }
                
                SendMessage(hwndDlg, DM_CALCMINHEIGHT, 0, 0);
                SendMessage(dat->pContainer->hwnd, DM_REPORTMINHEIGHT, (WPARAM) hwndDlg, (LPARAM) dat->uMinHeight);
                if(dat->pContainer->dwFlags & CNT_CREATE_MINIMIZED) {
                    dat->pContainer->dwFlags &= ~CNT_CREATE_MINIMIZED;
                    dat->pContainer->dwFlags |= CNT_DEFERREDCONFIGURE;
                    SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                    ShowWindow(dat->pContainer->hwnd, SW_MINIMIZE);
                    SendMessage(dat->pContainer->hwnd, DM_RESTOREWINDOWPOS, 0, 0);
                    //SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
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
                BYTE showStatus = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_STATUSICON, SRMSGDEFSET_STATUSICON);

                t_hwnd = dat->pContainer->hwnd;

                if (dat->szProto) {
                    hIcon = LoadSkinnedProtoIcon(dat->szProto, dat->wStatus);
                    SendDlgItemMessage(hwndDlg, IDC_PROTOCOL, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

                    if (!showStatus) {
                        SendMessage(t_hwnd, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) LoadSkinnedIcon(SKINICON_EVENT_MESSAGE));
                        break;
                    }

                    if (dat->pContainer->hwndActive == hwndDlg)
                        SendMessage(t_hwnd, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) hIcon);
                    break;
                }
                break;
            }
        case DM_UPDATELASTMESSAGE:
            {
                if (!dat->pContainer->hwndStatus || dat->nTypeSecs)
                    break;
                if (dat->pContainer->hwndActive != hwndDlg)
                    break;
                if (dat->lastMessage) {
                    DBTIMETOSTRING dbtts;
                    char date[64], time[64], fmt[128];

                    dbtts.szFormat = "d";
                    dbtts.cbDest = sizeof(date);
                    dbtts.szDest = date;
                    CallService(MS_DB_TIME_TIMESTAMPTOSTRING, dat->lastMessage, (LPARAM) & dbtts);
                    dbtts.szFormat = "t";
                    dbtts.cbDest = sizeof(time);
                    dbtts.szDest = time;
                    CallService(MS_DB_TIME_TIMESTAMPTOSTRING, dat->lastMessage, (LPARAM) & dbtts);
                    _snprintf(fmt, sizeof(fmt), Translate("Last message received on %s at %s"), date, time);
                    SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) fmt);
                    SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 0, (LPARAM) NULL);
                } else {
                    SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) "");
                    SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 0, (LPARAM) NULL);
                }
                break;
            }
        case DM_OPTIONSAPPLIED:

#ifdef __MATHMOD_SUPPORT            
            //mathMod begin
			CallService(MATH_SETBKGCOLOR, 0, (LPARAM)DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_MATH_BKGCOLOUR, SRMSGDEFSET_MATH_BKGCOLOUR));
			//mathMod end
#endif            
            
            if (wParam == 1) {      // 1 means, the message came from message log options page, so reload the defaults...
                if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "ignorecontactsettings", 0)) {
                    dat->dwFlags &= ~(MWF_LOG_ALL);
                    dat->dwFlags |= DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);
                }
            }
            dat->showUIElements = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWINFOLINE, SRMSGDEFSET_SHOWINFOLINE) ? MWF_UI_SHOWINFO : 0;
            dat->showUIElements |= DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWBUTTONLINE, SRMSGDEFSET_SHOWBUTTONLINE) ? MWF_UI_SHOWBUTTON : 0;
            dat->showUIElements |= DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SENDBUTTON, SRMSGDEFSET_SENDBUTTON) ? MWF_UI_SHOWSEND : 0;

            dat->dwEventIsShown = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWURLS, SRMSGDEFSET_SHOWURLS) ? MWF_SHOW_URLEVENTS : 0;
            dat->dwEventIsShown |= DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWFILES, SRMSGDEFSET_SHOWFILES) ? MWF_SHOW_FILEEVENTS : 0;
            dat->dwEventIsShown |= DBGetContactSettingByte(NULL, SRMSGMOD_T, "in_out_icons", 0) ? MWF_SHOW_INOUTICONS : 0;
            dat->dwEventIsShown |= DBGetContactSettingByte(NULL, SRMSGMOD_T, "emptylinefix", 0) ? MWF_SHOW_EMPTYLINEFIX : 0;
            
            if(dat->dwFlags & MWF_LOG_GRID)
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,1));     // XXX margins in the log (looks slightly better)
            else
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0,0));     // XXX margins in the log (looks slightly better)
                
            dat->showTypingWin = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGWIN, SRMSGDEFSET_SHOWTYPINGWIN);
            dat->dwFlags = DBGetContactSettingByte(NULL, SRMSGMOD_T, "clistmode", 0) ? dat->dwFlags | MWF_CLISTMODE : dat->dwFlags & ~MWF_CLISTMODE;
            
            SetDialogToType(hwndDlg);
            if (dat->hBkgBrush)
                DeleteObject(dat->hBkgBrush);
            if (dat->hInputBkgBrush)
                DeleteObject(dat->hInputBkgBrush);
            {
                COLORREF colour = DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR);
                COLORREF inputcolour = DBGetContactSettingDword(NULL, SRMSGMOD_T, "inputbg", SRMSGDEFSET_BKGCOLOUR);
                COLORREF inputcharcolor;
                CHARFORMAT2A cf2 = {0};
                LOGFONTA lf;
                HDC hdc = GetDC(NULL);
                int logPixelSY = GetDeviceCaps(hdc, LOGPIXELSY);
                ReleaseDC(NULL, hdc);
                
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
//                _DebugPopup(dat->hContact, "size: %d", cf2.yHeight);
                SendDlgItemMessageA(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, 0, (LPARAM)&cf2);
            }
            if (dat->dwFlags & MWF_LOG_RTL) {
                SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE) | WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
                SetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE) | WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
            } else {
                SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE) &~ (WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR));
                SetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE) &~ (WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR));
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
                    if (dat->szProto) {
                        CONTACTINFO ci;
                        int hasName = 0;
                        char buf[128];

                        contactName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) dat->hContact, 0);
                        ZeroMemory(&ci, sizeof(ci));
                        ci.cbSize = sizeof(ci);
                        ci.hContact = dat->hContact;
                        ci.szProto = dat->szProto;
                        ci.dwFlag = CNF_UNIQUEID;
                        if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
                            switch (ci.type) {
                                case CNFT_ASCIIZ:
                                    hasName = 1;
                                    _snprintf(buf, sizeof(buf), "%s", ci.pszVal);
                                    miranda_sys_free(ci.pszVal);
                                    break;
                                case CNFT_DWORD:
                                    hasName = 1;
                                    _snprintf(buf, sizeof(buf), "%u", ci.dVal);
                                    break;
                            }
                        }
                    /*
                     * cut nickname on tabs...
                     */
                        temp = contactName;
                        while(*temp)
                            iHash += (*(temp++) * (int)(temp - contactName + 1));

                        dat->wStatus = DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE);

                        if (iHash != dat->iOldHash || dat->wStatus != dat->wOldStatus || lParam != 0) {
                            if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "cuttitle", 0))
                                CutContactName(contactName, newcontactname, sizeof(newcontactname));
                            else
                                strncpy(newcontactname, contactName, sizeof(newcontactname));

                            if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "fulluin", 1))
                                SetDlgItemTextA(hwndDlg, IDC_NAME, hasName ? buf : contactName);
                            else
                                SendDlgItemMessage(hwndDlg, IDC_NAME, BUTTONADDTOOLTIP, hasName ? (WPARAM) buf : (WPARAM) contactName, 0);

                            szStatus = (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, dat->szProto == NULL ? ID_STATUS_OFFLINE : dat->wStatus, 0);
                            if (strlen(newcontactname) != 0 && szStatus != NULL) {
                                if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "tabstatus", 0))
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
                    }
                } else
                    lstrcpynA(newtitle, pszNewTitleEnd, sizeof(newtitle));

                ZeroMemory((void *)&item, sizeof(item));
                item.mask = TCIF_TEXT;

                if (iHash != dat->iOldHash || dat->wStatus != dat->wOldStatus || lParam != 0) {
                    if(dat->hContact != 0 && DBGetContactSettingByte(NULL, SRMSGMOD_T, "logstatus", 0) != 0 && DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "logstatus", -1) != 0) {
                        if(dat->wStatus != dat->wOldStatus && dat->hContact != 0 && dat->wOldStatus != (WORD)-1 && !(dat->dwFlags & MWF_INITMODE)) {             // log status changes to message log
                            DBEVENTINFO dbei;
                            char buffer[450];
                            HANDLE hNewEvent;
                            int iLen;
                            
                            char *szOldStatus = (char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)dat->wOldStatus, 0);
                            char *szNewStatus = (char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)dat->wStatus, 0);
                            
                            if(dat->szProto != NULL) {
                                if(dat->wStatus == ID_STATUS_OFFLINE) {
                                    _snprintf(buffer, sizeof(buffer), Translate("signed off (was %s)"), szOldStatus);
                                } else if(dat->wOldStatus == ID_STATUS_OFFLINE) {
                                    _snprintf(buffer, sizeof(buffer), Translate("signed on (%s)"), szNewStatus);
                                } else {
                                    _snprintf(buffer, sizeof(buffer), Translate("is now %s (was %s)"), szNewStatus, szOldStatus);
                                }
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
                break;
            }
        case DM_ADDDIVIDER:
            {
                if(!(dat->dwFlags & MWF_DIVIDERSET) && DBGetContactSettingByte(NULL, SRMSGMOD_T, "usedividers", 0)) {
                    dat->dwFlags |= MWF_DIVIDERWANTED;
                    dat->dwFlags |= MWF_DIVIDERSET;
                }
                break;
            }
        case WM_SETFOCUS:
            if (dat->iTabID >= 0) {
                SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                dat->dwTickLastEvent = 0;
                dat->dwFlags &= ~MWF_DIVIDERSET;
                if (KillTimer(hwndDlg, TIMERID_FLASHWND)) {
                    FlashTab(hwndTab, dat->iTabID, &dat->bTabFlash, FALSE, 0, dat->iTabImage);
                    dat->mayFlashTab = FALSE;
                }
                if (dat->dwFlags & MWF_DEFERREDREMAKELOG) {
                    SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                    dat->dwFlags &= ~MWF_DEFERREDREMAKELOG;
                }
                
                if(dat->dwFlags & MWF_NEEDCHECKSIZE)
                    SendMessage(hwndDlg, DM_SAVESIZE, 0, 0);

                if(dat->dwFlags & MWF_DEFERREDSCROLL)
                    SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 0);

    // XXX autolocale stuff
                if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "al", 0) && dat->hContact != 0) {
                    SendMessage(hwndDlg, DM_SETLOCALE, 0, 0);
                }
                SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                UpdateStatusBar(hwndDlg, dat);
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
                dat->dwTickLastEvent = 0;
                if (KillTimer(hwndDlg, TIMERID_FLASHWND)) {
                    FlashTab(hwndTab, dat->iTabID, &dat->bTabFlash, FALSE, 0, dat->iTabImage);
                    dat->mayFlashTab = FALSE;
                }
                if (dat->dwFlags & MWF_DEFERREDREMAKELOG) {
                    SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                    dat->dwFlags &= ~MWF_DEFERREDREMAKELOG;
                }
                if(dat->dwFlags & MWF_NEEDCHECKSIZE)
                    SendMessage(hwndDlg, DM_SAVESIZE, 0, 0);

                if(dat->dwFlags & MWF_DEFERREDSCROLL)
                    SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 0);

                if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "al", 0) && dat->hContact != 0)
                    SendMessage(hwndDlg, DM_SETLOCALE, 0, 0);

                UpdateStatusBar(hwndDlg, dat);
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
                    }
                    if (dat->splitterY < MINSPLITTERY) {
                        if(dat->showPic)
                            SendMessage(hwndDlg, DM_ALIGNSPLITTERMAXLOG, 0, 0);
                        else
                            SendMessage(hwndDlg, DM_LOADSPLITTERPOS, 0, 0);
                    }
                }

                CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM) & urd);
            // The statusbar sometimes draws over these 2 controls so 
            // redraw them
                RedrawWindow(GetDlgItem(hwndDlg, IDC_NAME), NULL, NULL, RDW_INVALIDATE);        // force redraw for this control
                RedrawWindow(GetDlgItem(hwndDlg, IDC_SPLITTER), NULL, NULL, RDW_INVALIDATE);        // force redraw for this control
				RedrawWindow(GetDlgItem(hwndDlg, IDC_SPLITTER5), NULL, NULL, RDW_INVALIDATE);        // force redraw for this control
#ifdef __MATHMOD_SUPPORT    			
                //mathMod begin
    			{
    						TMathWindowInfo mathWndInfo;
    												
    						RECT windRect;
    						// größe ermitteln:
    						GetWindowRect(hwndDlg,&windRect);
    						mathWndInfo.top=windRect.top;
    						mathWndInfo.left=windRect.left;
    						mathWndInfo.right=windRect.right;
    						mathWndInfo.bottom=windRect.bottom;
    						CallService(MTH_Set_Srmm_HWND,0,(LPARAM) hwndDlg); 
    						CallService(MTH_RESIZE,0,(LPARAM) &mathWndInfo);
    			}
    			//mathMod end
#endif                
                break;
            }
#ifdef __MATHMOD_SUPPORT    		
            //mathMod begin
    		case WM_MOVE:
    		{
    			TMathWindowInfo mathWndInfo;
    
    			RECT windRect;
    			// größe ermitteln:
    			GetWindowRect(hwndDlg,&windRect);
    			mathWndInfo.top=windRect.top;
    			mathWndInfo.left=windRect.left;
    			mathWndInfo.right=windRect.right;
    			mathWndInfo.bottom=windRect.bottom;
    			CallService(MTH_Set_Srmm_HWND,0,(LPARAM) hwndDlg);
    			CallService(MTH_RESIZE,0,(LPARAM) &mathWndInfo);
    			break;
    		}
    		//mathMod end
#endif            
        case DM_SPLITTERMOVED:
            {
                POINT pt;
                RECT rc;
                RECT rcLog;
                GetWindowRect(GetDlgItem(hwndDlg, IDC_LOG), &rcLog);
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
                    if (dat->showPic) {
                        int iOffset = (dat->showUIElements == 0) ? 28 : 0;
                        if (dat->splitterY < (dat->bottomOffset + iOffset) || dat->splitterY < MINSPLITTERY)
                            dat->splitterY = oldSplitterY;
                    } else {
                        if (dat->splitterY < MINSPLITTERY)           // min splitter size
                            dat->splitterY = oldSplitterY;
                    }
                    if (dat->splitterY > ((rc.bottom - rc.top) - 50))    // splitter top limit
                        dat->splitterY = oldSplitterY;
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
                if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "ignorecontactsettings", 0)) {
                    dat->dwFlags &= ~(MWF_LOG_ALL);
                    dat->dwFlags |= (lParam & MWF_LOG_ALL);
                    dat->dwFlags |= MWF_DEFERREDREMAKELOG;
                }
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
            StreamInEvents(hwndDlg, dat->hDbEventFirst, -1, 0, NULL);
            break;
        case DM_APPENDTOLOG:   //takes wParam=hDbEvent
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
                        
                        if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "div_popupconfig", 0)) {
                            if(!MessageWindowOpened((WPARAM)dat->hContact, 0)) {
                                iDividerSet = 1;
                                SendMessage(hwndDlg, DM_ADDDIVIDER, 0, 0);
                            }
                        }
                        if((GetForegroundWindow() != dat->pContainer->hwnd || GetActiveWindow() != dat->pContainer->hwnd)) {
                            dat->dwTickLastEvent = GetTickCount();
                            if(!iDividerSet)
                                SendMessage(hwndDlg, DM_ADDDIVIDER, 0, 0);
                        }
                        else {
                            if(dat->pContainer->hwndActive != hwndDlg) {
                                dat->dwTickLastEvent = GetTickCount();
                                if(!iDividerSet)
                                    SendMessage(hwndDlg, DM_ADDDIVIDER, 0, 0);
                            }
                        }
                    }
                    
                    if ((HANDLE) lParam != dat->hDbEventFirst && (HANDLE) CallService(MS_DB_EVENT_FINDNEXT, lParam, 0) == NULL) {
                        SendMessage(hwndDlg, DM_APPENDTOLOG, lParam, 0);
                    }
                    else
                        SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                    if (dat->iTabID == -1) {
                        MessageBoxA(0, "DBEVENTADDED Critical: iTabID == -1", "Error", MB_OK);
                    }
                    // tab flashing
                    if ((IsIconic(dat->pContainer->hwnd) || TabCtrl_GetCurSel(hwndTab) != dat->iTabID) && !(dbei.flags & DBEF_SENT) && dbei.eventType != EVENTTYPE_STATUSCHANGE) {
                        switch (dbei.eventType) {
                            case EVENTTYPE_MESSAGE:
                                dat->iFlashIcon = g_IconMsgEvent;
                                break;
                            case EVENTTYPE_URL:
                                dat->iFlashIcon = g_IconUrlEvent;
                                break;
                            case EVENTTYPE_FILE:
                                dat->iFlashIcon = g_IconFileEvent;
                                break;
                            default:
                                dat->iFlashIcon = g_IconMsgEvent;
                                break;
                        }
                        SetTimer(hwndDlg, TIMERID_FLASHWND, TIMEOUT_FLASHWND, NULL);
                        dat->mayFlashTab = TRUE;
                    }
                    /*
                     * autoswitch tab if option is set AND container is minimized (otherwise, we never autoswitch)
                     * never switch for status changes...
                     */
                    if(!(dbei.flags & DBEF_SENT) && dbei.eventType != EVENTTYPE_STATUSCHANGE) {
                        if(IsIconic(dat->pContainer->hwnd) && DBGetContactSettingByte(NULL, SRMSGMOD_T, "autoswitchtabs", 0) && dat->pContainer->hwndActive != hwndDlg) {
                            //SendMessage(dat->pContainer->hwndActive, DM_QUERYLASTUNREAD, 0, (LPARAM)&dwTimestamp);
                            //if(!dwTimestamp) {      // the active tab has no unread events, so we can switch safely...
                            int iItem = GetTabIndexFromHWND(GetParent(hwndDlg), hwndDlg);
                            if (iItem >= 0) {
                                TabCtrl_SetCurSel(GetParent(hwndDlg), iItem);
                                ShowWindow(dat->pContainer->hwndActive, SW_HIDE);
                                dat->pContainer->hwndActive = hwndDlg;
                                SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)dat->hContact, 0);
                                dat->pContainer->dwFlags |= CNT_DEFERREDTABSELECT;
                            }
                            //}
                        }
                    }
                    /*
                     * flash window if it is not focused
                     */
                    if ((GetActiveWindow() != dat->pContainer->hwnd || GetForegroundWindow() != dat->pContainer->hwnd) && !(dbei.flags & DBEF_SENT) && dbei.eventType != EVENTTYPE_STATUSCHANGE) {
                        if (!(dat->pContainer->dwFlags & CNT_NOFLASH)) {
                            if (dat->pContainer->isFlashing)
                                dat->pContainer->nFlash = 0;
                            else {
                                SetTimer(dat->pContainer->hwnd, TIMERID_FLASHWND, TIMEOUT_FLASHWND, NULL);
                                dat->pContainer->isFlashing = TRUE;
                            }
                        }
                    }
                }
                break;
            }
        case WM_TIMER:
            if (wParam == TIMERID_MSGSEND) {
                char szErrorMsg[512];
                KillTimer(hwndDlg, wParam);
                
                _snprintf(szErrorMsg, 500, Translate("Delivery failure: %s"), Translate("The message send timed out"));
                LogErrorMessage(hwndDlg, dat, 0, (char *)szErrorMsg);
                RecallFailedMessage(hwndDlg, dat);
                ShowErrorControls(hwndDlg, dat, TRUE);
                HandleIconFeedback(hwndDlg, dat, g_IconError);
                
            } else if (wParam == TIMERID_FLASHWND) {
                if (dat->iTabID == -1) {
                    MessageBoxA(0, "TIMER FLASH Critical: iTabID == -1", "Error", MB_OK);
                } else {
                    if (dat->mayFlashTab)
                        FlashTab(hwndTab, dat->iTabID, &dat->bTabFlash, TRUE, dat->iFlashIcon, dat->iTabImage);
                }
            } else if (wParam == TIMERID_ANTIBOMB) {
                int sentSoFar = 0, i;
                KillTimer(hwndDlg, wParam);
                for (i = 0; i < dat->sendJobs[0].sendCount; i++) {
                    if (dat->sendJobs[0].sendInfo[i].hContact == NULL)
                        continue;
                    if (sentSoFar >= ANTIBOMB_COUNT) {
                        KillTimer(hwndDlg, TIMERID_MSGSEND);
                        SetTimer(hwndDlg, TIMERID_MSGSEND, DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_MSGTIMEOUT, SRMSGDEFSET_MSGTIMEOUT), NULL);
                        SetTimer(hwndDlg, TIMERID_ANTIBOMB, TIMEOUT_ANTIBOMB, NULL);
                        break;
                    }
                    dat->sendJobs[0].sendInfo[i].hSendId = (HANDLE) CallContactService(dat->sendJobs[0].sendInfo[i].hContact, MsgServiceName(dat->sendJobs[0].sendInfo[i].hContact), 0, (LPARAM) dat->sendJobs[0].sendBuffer);
                    sentSoFar++;
                }
            } else if (wParam == TIMERID_TYPE) {
                if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON && GetTickCount() - dat->nLastTyping > TIMEOUT_TYPEOFF) {
                    NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
                }
                if (dat->showTyping) {
                    if (dat->nTypeSecs) {
                        dat->nTypeSecs--;
                        if (GetForegroundWindow() == dat->pContainer->hwnd)
                            SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
                    } else {
                        SendMessage(hwndDlg, DM_UPDATELASTMESSAGE, 0, 0);
                        if (dat->showTypingWin) {
                            SendMessage(dat->pContainer->hwnd, DM_UPDATEWINICON, 0, 0);
                            HandleIconFeedback(hwndDlg, dat, -1);
                        }
                        dat->showTyping = 0;
                    }
                } else {
                    if (dat->nTypeSecs) {
                        char szBuf[256];
                        char *szContactName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) dat->hContact, 0);

                        _snprintf(szBuf, sizeof(szBuf), Translate("%s is typing a message..."), szContactName);
                        dat->nTypeSecs--;
                        if(dat->pContainer->hwndStatus && dat->pContainer->hwndActive == hwndDlg) {
                            SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 0, (LPARAM) szBuf);
                            SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 0, (LPARAM) g_buttonBarIcons[5]);
                        }
                        if(IsIconic(dat->pContainer->hwnd) || GetForegroundWindow() != dat->pContainer->hwnd || GetActiveWindow() != dat->pContainer->hwnd) {
                            if(IsIconic(dat->pContainer->hwnd) && DBGetContactSettingByte(NULL, SRMSGMOD_T, "autoswitchtabs", 0)) {
                                SetWindowTextA(dat->pContainer->hwnd, szBuf);
                                dat->pContainer->dwFlags |= CNT_NEED_UPDATETITLE;
                            }
                        }
                        if (dat->pContainer->hwndActive != hwndDlg) {
                            if(dat->mayFlashTab)
                                dat->iFlashIcon = g_IconTypingEvent;
                            HandleIconFeedback(hwndDlg, dat, g_IconTypingEvent);
                        }
                        else {          // active tab may show icon if status bar is disabled
                            if(!dat->pContainer->hwndStatus) {
                                if(TabCtrl_GetItemCount(GetParent(hwndDlg)) > 1 || !(dat->pContainer->dwFlags & CNT_HIDETABS)) {
                                    HandleIconFeedback(hwndDlg, dat, g_IconTypingEvent);
                                }
                            }
                        }
                        if ((dat->showTypingWin && GetForegroundWindow() != dat->pContainer->hwnd) || (dat->showTypingWin && dat->pContainer->hwndStatus == 0))
                            SendMessage(dat->pContainer->hwnd, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) g_buttonBarIcons[5]);
                        dat->showTyping = 1;
                    }
                }
            }
            break;
        case DM_ERRORDECIDED:
            switch (wParam) {
                case MSGERROR_CANCEL:
                case MSGERROR_SENDLATER:
                    if(wParam == MSGERROR_SENDLATER) {
                        if(ServiceExists(BUDDYPOUNCE_SERVICENAME)) {
                            int iLen = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MESSAGE));
                            int iOffset = 0;
                            char *szMessage = (char *)malloc(iLen + 10);
                            /*
                        	if (!DBGetContactSetting(dat->hContact, "buddypounce", "PounceMsg",&dbv))
                        	{
                                if(lstrlenA(dbv.pszVal) > 0) {
                                    szMessage[0] = '\r';
                                    szMessage[1] = '\n';
                                    iOffset = 2;
                                }
                        		DBFreeVariant(&dbv);
                            } */
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
                                //dbei.cbBlob = lstrlenA(dat->sendBuffer) + 1;
                                dbei.cbBlob = lstrlenA(szNote) + 1;
#if defined( _UNICODE )
                                //dbei.cbBlob *= sizeof(TCHAR) + 1;
#endif
                                //dbei.pBlob = (PBYTE) dat->sendBuffer;
                                dbei.pBlob = (PBYTE) szNote;
                                //hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) dat->hContact, (LPARAM) & dbei);
                                StreamInEvents(hwndDlg,  0, 1, 1, &dbei);
                                SkinPlaySound("SendMsg");
                                if (dat->hDbEventFirst == NULL) {
                                    //dat->hDbEventFirst = hNewEvent;
                                    SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                                }
                                SendMessage(hwndDlg, DM_SAVEINPUTHISTORY, 0, 0);
                                EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);

                                SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
                                if(dat->pContainer->hwndActive == hwndDlg)
                                    UpdateReadChars(hwndDlg, dat->pContainer->hwndStatus);
                                SendDlgItemMessage(hwndDlg, IDC_SAVE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_buttonBarIcons[6]);
                                SendDlgItemMessage(hwndDlg, IDC_SAVE, BUTTONADDTOOLTIP, (WPARAM) pszIDCSAVE_close, 0);
                                dat->dwFlags &= ~MWF_SAVEBTN_SAV;
                                free(szMessage);
                            }
                        }
                    }
                    if(dat->sendJobs[0].sendInfo) {
                        free(dat->sendJobs[0].sendInfo);
                        dat->sendJobs[0].sendInfo = NULL;
                    }
                    ShowErrorControls(hwndDlg, dat, FALSE);
                    HandleIconFeedback(hwndDlg, dat, -1);
                    CheckSendQueue(hwndDlg, dat);
                    break;
                case MSGERROR_RETRY:
                    {
                        int i;
                        for (i = 0; i < dat->sendJobs[0].sendCount; i++) {
                            if (dat->sendJobs[0].sendInfo[i].hSendId == NULL && dat->sendJobs[0].sendInfo[i].hContact == NULL)
                                continue;
                            dat->sendJobs[0].sendInfo[i].hSendId = (HANDLE) CallContactService(dat->sendJobs[0].sendInfo[i].hContact, MsgServiceName(dat->sendJobs[0].sendInfo[i].hContact), 0, (LPARAM) dat->sendJobs[0].sendBuffer);
                        }
                    }
                    SetTimer(hwndDlg, TIMERID_MSGSEND, DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_MSGTIMEOUT, SRMSGDEFSET_MSGTIMEOUT), NULL);
                    ShowErrorControls(hwndDlg, dat, FALSE);
                    HandleIconFeedback(hwndDlg, dat, -1);
                    break;
            }
            break;
// XXX tab mod
        case DM_SELECTTAB:
            SendMessage(dat->pContainer->hwnd, DM_SELECTTAB, wParam, lParam);       // pass the msg to our container
            break;
// XXX tab mod
        case DM_SAVELOCALE: {
                if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "al", 0) && dat->hContact) {
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
            if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "al", 0) && dat->hContact != 0) {
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
                    SendMessage(hwndDlg, DM_OPTIONSAPPLIED, 0, 0);
                    dat->dwFlags &= ~MWF_INITMODE;
                    if(dat->lastMessage)
                        SendMessage(hwndDlg, DM_UPDATELASTMESSAGE, 0, 0);
                }
                SendMessage(dat->pContainer->hwnd, DM_QUERYCLIENTAREA, 0, (LPARAM)&rcClient);
                MoveWindow(hwndDlg, rcClient.left +1, rcClient.top, (rcClient.right - rcClient.left) - 8, (rcClient.bottom - rcClient.top) -2, TRUE);
                if(dat->dwFlags & MWF_WASBACKGROUNDCREATE) {
                    POINT pt = {0};;
                    
                    dat->dwFlags &= ~MWF_WASBACKGROUNDCREATE;
                    if(!dat->hThread)
                        ShowPicture(hwndDlg,dat,FALSE,TRUE, TRUE);
                    SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETSCROLLPOS, 0, (LPARAM)&pt);
                }
                SendMessage(hwndDlg, WM_SIZE, 0, 0);
                SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 0);
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
            DWORD iMaxHeight;
            
            GetObject(dat->hContactPic,sizeof(bminfo),&bminfo);
            //InvalidateRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), NULL, TRUE);
            iMaxHeight = DBGetContactSettingDword(NULL, SRMSGMOD_T, "avatarheight", 100);
            if(dat->dwFlags & MWF_LOG_LIMITAVATARHEIGHT && bminfo.bmHeight > (LONG)iMaxHeight) {
                double aspect = (double)iMaxHeight / (double)bminfo.bmHeight;
                double newWidth = (double)bminfo.bmWidth * aspect;

                dat->pic.cy = iMaxHeight + 2*1;
                dat->pic.cx = (int)newWidth + 2*1;
            }
            else {
                dat->pic.cx=bminfo.bmWidth+2*1;
                dat->pic.cy=bminfo.bmHeight+2*1;
            }
            SendMessage(hwndDlg, WM_SIZE, 0, 0);
            break;
        }
        /*
         * recalculate minimum allowed splitter position to avoid clipping on the
         * user picture.
         */
        case DM_UPDATEPICLAYOUT:
            if (dat->showPic) {
                POINT pt1;
                RECT rc1, rc2;
                int iOffset = 0;
                
                SendMessage(hwndDlg, WM_SIZE, 0, 0);
                GetClientRect(hwndDlg, &rc2);
                GetWindowRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), &rc1);
                pt1.x = rc1.left;
                pt1.y = rc1.top;
                ScreenToClient(hwndDlg, &pt1);
                dat->bottomOffset = (rc2.bottom - rc2.top) - pt1.y + 1;
                // XXX for splitter at top of bar               dat->bottomOffset = (rc2.bottom - rc2.top) - pt1.y + (rc1.bottom - rc1.top) + 5 - 24;
                iOffset = (dat->showUIElements == 0) ? 28 : 0;
                if (dat->splitterY < (dat->bottomOffset + iOffset)) {
                    dat->splitterY = dat->bottomOffset + iOffset;
                    if(!wParam)
                        SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 0);
                }
            } else if (dat->hContact) {
                SendMessage(hwndDlg, DM_LOADSPLITTERPOS, 0, 0);
				if(!wParam)
					SendMessage(hwndDlg, WM_SIZE, 0, 0);
            }
            break;
        case DM_LOADSPLITTERPOS:
            {
                if(dat->dwFlags & MWF_LOG_GLOBALSPLITTER)
                    dat->splitterY = (int) DBGetContactSettingDword(NULL, SRMSGMOD, "splitsplity", (DWORD) 150);
                else {
                    if(dat->showPic && dat->hContact)
                        dat->splitterY = (int) DBGetContactSettingDword(dat->hContact, SRMSGMOD, "splitsplity", (DWORD) 150);
                    else {
                        if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "ignorecontactsettings", 0))
                            dat->splitterY = (int) DBGetContactSettingDword(NULL, SRMSGMOD, "splitsplity", (DWORD) 150);
                        else {
                            dat->splitterY = (int) DBGetContactSettingDword(dat->hContact, SRMSGMOD, "splitsplity", DBGetContactSettingDword(NULL, SRMSGMOD, "splitsplity", (DWORD) 150));
                        }
                    }
                }
                if(dat->splitterY < MINSPLITTERY || dat->splitterY == -1) {
                    if(!dat->showPic)
                        dat->splitterY = MINSPLITTERY;
                }
                break;
            }
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
            
            GetCursorPos(&pt);
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
                        WCHAR *wszTemp = Utf8Decode(dbv.pszVal);
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
                if (dis->hwndItem == GetDlgItem(hwndDlg, IDC_PROTOCOL)) {
                    if (dat->szProto) {
                        HICON hIcon;

                        hIcon = LoadSkinnedProtoIcon(dat->szProto, dat->wStatus);
                        if (hIcon)
                            DrawIconEx(dis->hDC, dis->rcItem.left, dis->rcItem.top, hIcon, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL);
                    }
                } else if (dis->hwndItem == GetDlgItem(hwndDlg, IDC_TYPINGNOTIFY)) {
                    DrawIconEx(dis->hDC, dis->rcItem.left, dis->rcItem.top, g_buttonBarIcons[5], GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL);
                } else if (dis->hwndItem == GetDlgItem(hwndDlg, IDC_CONTACTPIC)) {
                    HPEN hPen;
                    hPen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
                    SelectObject(dis->hDC, hPen);
                    Rectangle(dis->hDC, 0, 0, dat->pic.cx, dat->pic.cy);
                    DeleteObject(hPen);
                    if(dat->hContactPic && dat->showPic) {
                        BITMAP bminfo;
                        DWORD iMaxHeight;
                        HDC hdcMem = CreateCompatibleDC(dis->hDC);
                        HBITMAP hbmMem = (HBITMAP)SelectObject(hdcMem, dat->hContactPic);
                        GetObject(dat->hContactPic, sizeof(bminfo), &bminfo);
                        iMaxHeight = DBGetContactSettingDword(NULL, SRMSGMOD_T, "avatarheight", 100);
                        if(dat->dwFlags & MWF_LOG_LIMITAVATARHEIGHT && bminfo.bmHeight > (LONG)iMaxHeight) {
                            double aspect = (double)iMaxHeight / (double)bminfo.bmHeight;
                            double newWidth = (double)bminfo.bmWidth * aspect;
                            SetStretchBltMode(dis->hDC, COLORONCOLOR);
                            StretchBlt(dis->hDC, 1, 1, (int)newWidth, iMaxHeight, hdcMem, 0, 0, bminfo.bmWidth, bminfo.bmHeight, SRCCOPY);
                        } else {
                            BitBlt(dis->hDC, 1, 1, bminfo.bmWidth, bminfo.bmHeight, hdcMem, 0, 0, SRCCOPY);
                        }
                        DeleteObject(hbmMem);
                        DeleteDC(hdcMem);
                    }
                }
                return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
            }
        case WM_COMMAND:
            if (CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(LOWORD(wParam), MPCF_CONTACTMENU), (LPARAM) dat->hContact))
                break;
            switch (LOWORD(wParam)) {
                case IDOK:
                    {
                    //this is a 'send' button
                    int bufSize;
                    char *allTmp;
#if defined(_UNICODE)
                    TCHAR *allTmpW;
#endif                    
                    
                    if (!IsWindowEnabled(GetDlgItem(hwndDlg, IDOK)))
                        break;

                    bufSize = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MESSAGE)) + 1;
                    dat->sendBuffer = (char *) realloc(dat->sendBuffer, bufSize * (sizeof(TCHAR) + 1));
                    GetDlgItemTextA(hwndDlg, IDC_MESSAGE, dat->sendBuffer, bufSize);
#if defined( _UNICODE )
                    GetDlgItemTextW(hwndDlg, IDC_MESSAGE, (TCHAR *) & dat->sendBuffer[bufSize], bufSize);
#endif
                    if (dat->sendBuffer[0] == 0)
                        break;

// BEGIN /all /MOD, contributed by JdGordon
#define SEND2ALLTEXT Translate("/all")
                    if (!strncmp(dat->sendBuffer,SEND2ALLTEXT, strlen(SEND2ALLTEXT))) {
                        HWND contacthwnd;
                        TCITEM tci;
                        BOOL bOldState;
                        int tabCount = TabCtrl_GetItemCount(hwndTab), i, chars2add=0;
                        
                        allTmp = (char *)malloc(bufSize + 1);
                        // copy the msg to temp
                        strcpy(allTmp, dat->sendBuffer);
                        // stop idiots from doing /all/all/all....
                        while(!strncmp(&allTmp[strlen(SEND2ALLTEXT)+chars2add],SEND2ALLTEXT,strlen(SEND2ALLTEXT)))
                           chars2add += strlen(SEND2ALLTEXT);
                        // copy it back without the /all
                        strcpy(dat->sendBuffer, &allTmp[strlen(SEND2ALLTEXT)+chars2add]);
                        // no tabs... break and send normally
#ifdef _UNICODE
#define SEND2ALLTEXTW (_T("/all"))
                        allTmpW = (TCHAR *)malloc((bufSize + 1) * sizeof(TCHAR));
                        lstrcpyW(allTmpW, (TCHAR *)&dat->sendBuffer[bufSize]);
                        // stop idiots from doing /all/all/all....
                        while(!wcsncmp(&allTmpW[lstrlenW(SEND2ALLTEXTW) + chars2add], SEND2ALLTEXTW, lstrlenW(SEND2ALLTEXTW)))
                           chars2add += lstrlenW(SEND2ALLTEXTW);
                        // copy it back without the /all
                        lstrcpyW((TCHAR *)&dat->sendBuffer[strlen(dat->sendBuffer) + 1], &allTmpW[lstrlenW(SEND2ALLTEXTW) + chars2add]);
                        free(allTmpW);
#endif
                        free(allTmp);
                        if (!tabCount)
                           break;

                        ZeroMemory((void *)&tci, sizeof(tci));
                        tci.mask = TCIF_PARAM;
   
                        for (i = 0; i < tabCount; i++) {   
                           TabCtrl_GetItem(hwndTab, i, &tci);
                           // get the contact from the tabs lparam which hopefully is the tabs hwnd so we can get its userdata.... hopefully
                           contacthwnd = (HWND)tci.lParam;
                           if (IsWindow(contacthwnd)) {   
                              // if the contact hwnd is the current contact then ignore it and let the normal code deal with the msg
                              if (contacthwnd != hwndDlg) {
                                 // send the buffer to the contacts msg typing area
#ifdef _UNICODE
                                 SetDlgItemTextW(contacthwnd, IDC_MESSAGE, (TCHAR *)&dat->sendBuffer[strlen(dat->sendBuffer) + 1]);
#else
                                 SetDlgItemTextA(contacthwnd, IDC_MESSAGE, (char*)dat->sendBuffer);
#endif                                     
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
                    }
// END /all /MOD
                    if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON) {
                        NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
                    }
                    /*
                    if(dat->codePage != CP_ACP) {
                        BOOL defaultchar;
                        WideCharToMultiByte(dat->codePage, 0, &dat->sendBuffer[bufSize], -1, dat->sendBuffer, bufSize, NULL, &defaultchar);
                        MessageBoxA(0, dat->sendBuffer, "ansi", MB_OK);
                    }
                    */
                    /*
                    {
                        CHARFORMAT2 cf2, cf3;
                        int i;
                        ZeroMemory((void *)&cf2, sizeof(cf2));
                        cf2.cbSize = sizeof(cf2);
                        SendDlgItemMessage(hwndDlg, IDC_MESSAGE, WM_SETREDRAW, FALSE, 0);
                        cf3 = cf2;
                        for(i = 0; i < bufSize; i++) {
                            SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_SETSEL, i, i+1);
                            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
                            if(memcmp(&cf2, &cf3, sizeof(cf2)) != 0) {
                            }
                            cf3 = cf2;
                        }
                        SendDlgItemMessage(hwndDlg, IDC_MESSAGE, WM_SETREDRAW, TRUE, 0);
                    } */
                    AddToSendQueue(hwndDlg, dat, bufSize);
                    return TRUE;
                    }
                case IDC_QUOTE:
                    {   CHARRANGE sel;
                        TCHAR *szText, *szQuoted;
                        int i;
#ifdef _UNICODE
                        TCHAR *szConverted;
                        int iAlloced = 0;
                        int iSize = 0;
#ifdef __GNUWIN32__
                        _GETTEXTEX gtx;
#else                        
                        GETTEXTEX gtx;
#endif                        
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
#else                              
                            szQuoted=QuoteText(szText, 64, 0);
#endif
                            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_REPLACESEL, TRUE, (LPARAM)szQuoted);
                            
                            free(szText);
                            free(szQuoted);
#ifdef _UNICODE
                            if(iAlloced)
                                free(szConverted);
#endif                            
                        } else {
                            szText = (TCHAR *)malloc(((sel.cpMax - sel.cpMin) + 100) * sizeof(TCHAR));
#ifdef _UNICODE
                            gtx.cb = ((sel.cpMax - sel.cpMin) + 1) * sizeof(TCHAR);
                            gtx.codepage = 1200;
                            gtx.flags = GT_SELECTION | GT_DEFAULT;
                            gtx.lpDefaultChar = 0;
                            gtx.lpUsedDefChar = 0;
                            SendDlgItemMessage(hwndDlg, IDC_LOG, EM_GETTEXTEX, (WPARAM)&gtx, (LPARAM)szText);
                            i=0;
                            while(szText[i]) {
                                if(szText[i] == 0x0b)
                                    szText[i] = '\r';
                                i++;
                            }
                            szQuoted = QuoteText(szText, 64, 0);
                            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_REPLACESEL, TRUE, (LPARAM)szQuoted);
                            free(szQuoted);
#else
                            SendDlgItemMessageA(hwndDlg,IDC_LOG,EM_GETSELTEXT,0,(LPARAM)szText);
                            i = 0;
                            while(szText[i]) {
                                if(szText[i] == 0x0b)
                                    szText[i] = '\r';
                                i++;
                            }
                            szQuoted=QuoteText(szText, 64, 0);
                            SendDlgItemMessageA(hwndDlg, IDC_MESSAGE, EM_REPLACESEL, TRUE, (LPARAM)szQuoted);
                            free(szQuoted);
#endif
                            free(szText);
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
                        RECT rc;
                        HMENU hMenu = (HMENU) CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM) dat->hContact, 0);
                        GetWindowRect(GetDlgItem(hwndDlg, LOWORD(wParam)), &rc);
                        TrackPopupMenu(hMenu, 0, rc.left, rc.bottom, 0, hwndDlg, NULL);
                        DestroyMenu(hMenu);
                    }
                    break;
                case IDC_HISTORY:
                    // OnO: RTF log
                    if(GetKeyState(VK_SHIFT) & 0x8000)
                    {
                        EDITSTREAM stream = { 0 };
                        stream.dwCookie = (DWORD_PTR)dat;
                        stream.dwError = 0;
                        stream.pfnCallback = StreamOut;
                        SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_STREAMOUT, SF_RTF | SF_USECODEPAGE, (LPARAM) & stream);
                    }
                    else
                        CallService(MS_HISTORY_SHOWCONTACTHISTORY, (WPARAM) dat->hContact, 0);
                    break;
                case IDC_DETAILS:
                    CallService(MS_USERINFO_SHOWDIALOG, (WPARAM) dat->hContact, 0);
                    break;
                case IDC_SMILEYBTN:
                    if(dat->doSmileys && g_SmileyAddAvail) {
                        SMADD_SHOWSEL smaddInfo;
                        HICON hButtonIcon = 0;
                        RECT rc;
                        
						if(CheckValidSmileyPack(dat->szProto, &hButtonIcon) != 0) {
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
							CallService(MS_SMILEYADD_SHOWSELECTION, 0, (LPARAM) &smaddInfo);
						}
                    }
                    break;
                case IDC_TIME: {
                    RECT rc;
                    HMENU submenu = GetSubMenu(dat->pContainer->hMenuContext, 2);
                    int iSelection, isHandled;
                    DWORD dwOldFlags = dat->dwFlags;
                    
                    MsgWindowUpdateMenu(hwndDlg, dat, submenu, MENU_LOGMENU);
                    
                    GetWindowRect(GetDlgItem(hwndDlg, IDC_TIME), &rc);

                    iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, rc.left, rc.bottom, 0, hwndDlg, NULL);
                    isHandled = MsgWindowMenuHandler(hwndDlg, dat, iSelection, MENU_LOGMENU);

                    if(dat->dwFlags != dwOldFlags) {
                        WindowList_Broadcast(hMessageWindowList, DM_DEFERREDREMAKELOG, (WPARAM)hwndDlg, (LPARAM)(dat->dwFlags & MWF_LOG_ALL));
                        if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "ignorecontactsettings", 0))
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", dat->dwFlags & MWF_LOG_ALL);
                    }
                    break;
                }
                case IDC_MULTIPLE: {
                    RECT rc;

                    dat->multiple = IsDlgButtonChecked(hwndDlg, IDC_MULTIPLE);
                    SendMessage(dat->pContainer->hwnd, DM_QUERYCLIENTAREA, 0, (LPARAM)&rc);
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                    SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 0);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), dat->multiple ? SW_SHOW : SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_CLIST), dat->multiple ? SW_SHOW : SW_HIDE);
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
                            ShowWindow(GetDlgItem(hwndDlg, IDC_ADD), FALSE);
                        }
                        break;
                    }
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
// BEGIN MOD#33: Show contact's picture
                case IDC_CONTACTPIC:
                    SendMessage(hwndDlg, DM_RETRIEVEAVATAR, 0, 0);
                    if(GetWindowLong(hwndDlg, DWL_MSGRESULT) == 0)
                        ShowPicture(hwndDlg,dat,TRUE,TRUE,TRUE);
                    break;
// END MOD#33
                case IDM_CLEAR:
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
#ifdef __MATHMOD_SUPPORT					
                    //mathMod begin
					if (HIWORD(wParam) == EN_CHANGE)
					{
						WINDOWPLACEMENT  cWinPlace;
						{	// substitute Delimiter:
						DBVARIANT dbv;
						TMathSubstInfo substInfo;
						substInfo.EditHandle = GetDlgItem(hwndDlg,IDC_MESSAGE);
						if (ServiceExists(MTH_SUBSTITUTE_DELIMITER))
						{
							if (DBGetContactSetting(NULL, SRMSGMOD, SRMSGSET_MATH_SUBSTITUTE_DELIMITER, &dbv))
							{
								substInfo.Substitute = malloc(50);
								lstrcpynA(substInfo.Substitute,SRMSGDEFSET_MATH_SUBSTITUTE_DELIMITER , 49);
								CallService(MTH_SUBSTITUTE_DELIMITER,0,(LPARAM)&substInfo);
								free(substInfo.Substitute);
							}
							else
							{
								substInfo.Substitute = dbv.pszVal;
								CallService(MTH_SUBSTITUTE_DELIMITER,0,(LPARAM)&substInfo);
							    DBFreeVariant(&dbv);
							}
						}

						}
						updatePreview(hwndDlg);
						CallService(MTH_SHOW, 0, 0);
						SetForegroundWindow(hwndDlg);
						cWinPlace.length=sizeof(WINDOWPLACEMENT);
						GetWindowPlacement(hwndDlg,&cWinPlace);
						if (cWinPlace.showCmd == SW_SHOWMAXIMIZED)
						{
							RECT rcWindow;
							GetWindowRect(hwndDlg, &rcWindow);
							if(CallService(MTH_GET_PREVIEW_SHOWN,0,0))
								MoveWindow(hwndDlg,rcWindow.left,rcWindow.top,rcWindow.right-rcWindow.left,GetSystemMetrics(SM_CYSCREEN)-CallService(MTH_GET_PREVIEW_HEIGHT ,0,0),1);
							else
								MoveWindow(hwndDlg,rcWindow.left,rcWindow.top,rcWindow.right-rcWindow.left,GetSystemMetrics(SM_CYSCREEN),1);
						}

					}
					//mathMod end
#endif                    
                    if (HIWORD(wParam) == EN_CHANGE) {
                        if(dat->pContainer->hwndActive == hwndDlg)
                            UpdateReadChars(hwndDlg, dat->pContainer->hwndStatus);
                        dat->dwFlags |= MWF_NEEDHISTORYSAVE;

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
                            switch (((MSGFILTER *) lParam)->msg) {
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
                                            InsertMenuA(hSubMenu, 6, MF_BYPOSITION | MF_POPUP, (UINT_PTR) g_hMenuEncoding, Translate("ANSI Encoding"));
                                            for(i = 0; i < GetMenuItemCount(g_hMenuEncoding); i++)
                                                CheckMenuItem(g_hMenuEncoding, i, MF_BYPOSITION | MF_UNCHECKED);
                                            if(dat->codePage == CP_ACP)
                                                CheckMenuItem(g_hMenuEncoding, 0, MF_BYPOSITION | MF_CHECKED);
                                            else
                                                CheckMenuItem(g_hMenuEncoding, dat->codePage, MF_BYCOMMAND | MF_CHECKED);
                                                
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
                                                    SendMessage(((NMHDR *) lParam)->hwndFrom, WM_PASTE, 0, 0);
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
                                            RemoveMenu(hSubMenu, 0, MF_BYPOSITION);
#endif                                        
                                        DestroyMenu(hMenu);
                                        if(dat->codePage != oldCodepage)
                                            SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                                        
                                        SetWindowLong(hwndDlg, DWL_MSGRESULT, TRUE);
                                        return TRUE;
                                    }
                            }
                            break;
                        case EN_LINK:
                            switch (((ENLINK *) lParam)->msg) {
                                case WM_SETCURSOR:
                                    SetCursor(hCurHyperlinkHand);
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
                int iSelection, isHandled;
                POINT pt;
                HMENU submenu = GetSubMenu(dat->pContainer->hMenuContext, 1);

                GetCursorPos(&pt);
                MsgWindowUpdateMenu(hwndDlg, dat, submenu, MENU_PICMENU);
                iSelection = TrackPopupMenu(submenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
                isHandled = MsgWindowMenuHandler(hwndDlg, dat, iSelection, MENU_PICMENU);
                if(isHandled)
                    RedrawWindow(GetDlgItem(hwndDlg, IDC_PIC), NULL, NULL, RDW_INVALIDATE);
            }
            break;
        case HM_EVENTSENT:
            {
                ACKDATA *ack = (ACKDATA *) lParam;
                DBEVENTINFO dbei = { 0};
                HANDLE hNewEvent;
                int i;

                if (ack->type != ACKTYPE_MESSAGE)
                    break;

                switch (ack->result) {
                    case ACKRESULT_FAILED: {
                        char szErrorMsg[512];
                        
                        KillTimer(hwndDlg, TIMERID_MSGSEND);
                        _snprintf(szErrorMsg, 500, Translate("Delivery failure: %s"), (char *)ack->lParam);
                        LogErrorMessage(hwndDlg, dat, 0, (char *)szErrorMsg);
                        RecallFailedMessage(hwndDlg, dat);
                        ShowErrorControls(hwndDlg, dat, TRUE);
                        HandleIconFeedback(hwndDlg, dat, g_IconError);
                        return 0;
                    }
                }                   //switch

                for (i = 0; i < dat->sendJobs[0].sendCount; i++)
                    if (ack->hProcess == dat->sendJobs[0].sendInfo[i].hSendId && ack->hContact == dat->sendJobs[0].sendInfo[i].hContact)
                        break;
                if (i == dat->sendJobs[0].sendCount)
                    break;

                dbei.cbSize = sizeof(dbei);
                dbei.eventType = EVENTTYPE_MESSAGE;
                dbei.flags = DBEF_SENT;
                dbei.szModule = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) dat->sendJobs[0].sendInfo[i].hContact, 0);
                dbei.timestamp = time(NULL);
                dbei.cbBlob = lstrlenA(dat->sendJobs[0].sendBuffer) + 1;
#if defined( _UNICODE )
                dbei.cbBlob *= sizeof(TCHAR) + 1;
#endif
                dbei.pBlob = (PBYTE) dat->sendJobs[0].sendBuffer;
                hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) dat->sendJobs[0].sendInfo[i].hContact, (LPARAM) & dbei);
                SkinPlaySound("SendMsg");
                if (dat->sendJobs[0].sendInfo[i].hContact == dat->hContact) {
                    if (dat->hDbEventFirst == NULL) {
                        dat->hDbEventFirst = hNewEvent;
                        SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                    }
                }
                dat->sendJobs[0].sendInfo[i].hSendId = NULL;
                dat->sendJobs[0].sendInfo[i].hContact = NULL;
                for (i = 0; i < dat->sendJobs[0].sendCount; i++)
                    if (dat->sendJobs[0].sendInfo[i].hContact || dat->sendJobs[0].sendInfo[i].hSendId)
                        break;
                if (i == dat->sendJobs[0].sendCount) {
                //all messages sent
                    
                    if(dat->sendJobs[0].sendCount > 1)          // re-enable sending if it was a multisend with more than 1 entry
                        EnableSending(hwndDlg, dat, TRUE);
                    
                    dat->sendJobs[0].sendCount = 0;
                    free(dat->sendJobs[0].sendInfo);
                    dat->sendJobs[0].sendInfo = NULL;
                    KillTimer(hwndDlg, TIMERID_MSGSEND);

                    CheckSendQueue(hwndDlg, dat);
                }
                break;
            }
            /*
             * save the current content of the input box to the history stack...
             * increase stack
             * wParam = special mode to store it at a special position without caring about the
             * stack pointer and stuff..
             */
        case DM_SAVEINPUTHISTORY:
            {
                int iLength = 0;
                int oldTop = 0;
                
                if(wParam) {
                    oldTop = dat->iHistoryTop;
                    dat->iHistoryTop = (int)wParam;
                }
                
                // input history stuff - add the line to the stack
                iLength = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MESSAGE));
                if(iLength > 0 && dat->history != NULL) {
                    if((dat->iHistoryTop == dat->iHistorySize) && oldTop == 0) {          // shift the stack down...
                        struct InputHistory ihTemp = dat->history[0];
                        dat->iHistoryTop--;
                        MoveMemory((void *)&dat->history[0], (void *)&dat->history[1], (dat->iHistorySize - 1) * sizeof(struct InputHistory));
                        dat->history[dat->iHistoryTop] = ihTemp;
                    }
                    if(iLength > dat->history[dat->iHistoryTop].lLen) {
                        if(dat->history[dat->iHistoryTop].szText == NULL) {
                            if(iLength < HISTORY_INITIAL_ALLOCSIZE)
                                iLength = HISTORY_INITIAL_ALLOCSIZE;
                            dat->history[dat->iHistoryTop].szText = (TCHAR *)malloc((iLength + 1) * sizeof(TCHAR));
                        }
                        else
                            dat->history[dat->iHistoryTop].szText = (TCHAR *)realloc(dat->history[dat->iHistoryTop].szText, (iLength + 1) * sizeof(TCHAR));
                        dat->history[dat->iHistoryTop].lLen = iLength;
                    }
                    GetWindowText(GetDlgItem(hwndDlg, IDC_MESSAGE), dat->history[dat->iHistoryTop].szText, dat->history[dat->iHistoryTop].lLen + 1);
                    if(!oldTop) {
                        if(dat->iHistoryTop < dat->iHistorySize) {
                            dat->iHistoryTop++;
                            dat->iHistoryCurrent = dat->iHistoryTop;
                        }
                    }
                }
                if(oldTop)
                    dat->iHistoryTop = oldTop;
                break;
            }
        case DM_SAVEPERCONTACT:
            if (!dat->showPic) {
                if(!(dat->dwFlags &  MWF_LOG_GLOBALSPLITTER) && !DBGetContactSettingByte(NULL, SRMSGMOD_T, "ignorecontactsettings", 0)) {
                    DBWriteContactSettingDword(dat->hContact, SRMSGMOD, "splitsplity", dat->splitterY);
                }
                else {
                    DBWriteContactSettingDword(NULL, SRMSGMOD, "splitsplity", dat->splitterY);
                }
            } else {
                if (dat->hContact && !(dat->dwFlags & MWF_LOG_GLOBALSPLITTER)) {
                    DBWriteContactSettingDword(dat->hContact, SRMSGMOD, "splitsplity", dat->splitterY);
                }
            }
            if (dat->hContact) {        // save contact specific settings
                if(!DBGetContactSettingByte(NULL, SRMSGMOD_T, "ignorecontactsettings", 0)) {
                    DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "mwflags", dat->dwFlags & MWF_LOG_ALL);
                }
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
            
        case DM_ALIGNSPLITTERMAXLOG:
            SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 1, 0);
            dat->splitterY = dat->bottomOffset;
            dat->splitterY = dat->splitterY < MINSPLITTERY ? MINSPLITTERY : dat->splitterY;
            SendMessage(hwndDlg, DM_SPLITTERMOVED, (WPARAM)dat->splitterY, (LPARAM)GetDlgItem(hwndDlg, IDC_SPLITTER));
            SendMessage(hwndDlg, WM_SIZE, 0, 0);
            SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 1);
            break;
        case DM_ALIGNSPLITTERFULL:
            SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 1, 0);
            dat->splitterY = dat->bottomOffset + 28;
            dat->splitterY = dat->splitterY < MINSPLITTERY ? MINSPLITTERY : dat->splitterY;
            SendMessage(hwndDlg, DM_SPLITTERMOVED, (WPARAM)dat->splitterY, (LPARAM)GetDlgItem(hwndDlg, IDC_SPLITTER));
            SendMessage(hwndDlg, WM_SIZE, 0, 0);
            SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 1);
            break;

        case DM_PICTHREADCOMPLETE:
            if(dat->hThread) {
                CloseHandle(dat->hThread);
                dat->hThread = 0;
                ShowPicture(hwndDlg, dat, FALSE, TRUE, FALSE);
            }
            break;
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
                if(DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE)
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
                    DBWriteContactSettingString(dat->hContact, SRMSGMOD_T, "MOD_Pic", pai_s.filename);
                    DBWriteContactSettingString(dat->hContact, "ContactPhoto", "File",pai_s.filename);
                    ShowPicture(hwndDlg, dat, FALSE, TRUE, TRUE);
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
               
                if(wParam == 0 && lParam == 0 && !DBGetContactSettingByte(NULL, SRMSGMOD_T, "escmode", 0)) {
                    SendMessage(dat->pContainer->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                    return TRUE;
                }
                
                if(dat->iSendJobCurrent > 0)
                    return TRUE;
                
#if defined(_STREAMTHREADING)
                if(dat->pendingStream)
                    return TRUE;
#endif                
                if(!lParam) {
                    if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "warnonexit", 0)) {
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
                    SetWindowPos(dat->pContainer->hwndActive, HWND_TOP, rc.left +1, rc.top, (rc.right - rc.left) - 8, (rc.bottom - rc.top) - 2, SWP_SHOWWINDOW);
                    ShowWindow((HWND)item.lParam, SW_SHOW);
                    SetForegroundWindow(dat->pContainer->hwndActive);
                    SetFocus(dat->pContainer->hwndActive);
                    SendMessage(dat->pContainer->hwnd, WM_SIZE, 0, 0);
                    RedrawWindow(dat->pContainer->hwndActive, NULL, NULL, RDW_INVALIDATE);
                    UpdateWindow(dat->pContainer->hwndActive);
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
// BEGIN MOD#26: Autosave notsent message (by Corsario & bi0)
			if(dat->hContact) {
				TCHAR *AutosaveMessage;
                
				int bufSize = GetWindowTextLength(GetDlgItem(hwndDlg,IDC_MESSAGE));
                if (bufSize) {
                    bufSize++;
    				AutosaveMessage=(TCHAR*)malloc(bufSize * sizeof(TCHAR));
#ifdef _UNICODE
    				GetDlgItemTextW(hwndDlg,IDC_MESSAGE,AutosaveMessage,bufSize);
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

            if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON) {
                NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
            }
            if (dat->hAckEvent)
                UnhookEvent(dat->hAckEvent);
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
                // free send queue jobs
                for(i = 0; i < NR_SENDJOBS; i++) {
                    if(dat->sendJobs[i].sendBuffer) {
                        //_DebugPopup(dat->hContact, "sendqueue free: %s (%d total)", dat->sendJobs[i].sendBuffer, dat->iSendJobMax);
                        free(dat->sendJobs[i].sendBuffer);
                    }
                    if(dat->sendJobs[i].sendInfo)
                        free(dat->sendJobs[i].sendInfo);
                }
            }
            
// BEGIN MOD#33: Show contact's picture
            if (dat->hContactPic)
                DeleteObject(dat->hContactPic);
// END MOD#33
            if (dat->hSmileyIcon)
                DeleteObject(dat->hSmileyIcon);
            
            WindowList_Remove(hMessageWindowList, hwndDlg);
            SendMessage(hwndDlg, DM_SAVEPERCONTACT, 0, 0);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_MULTISPLITTER), GWL_WNDPROC, (LONG) OldSplitterProc);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_WNDPROC, (LONG) OldSplitterProc);
            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_UNSUBCLASSED, 0, 0);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_WNDPROC, (LONG) OldMessageEditProc);
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
            //_DebugPopup(dat->hContact, "WM_DESTROY (hwnd = %d)", hwndDlg);
            if (dat)
                free(dat);
            else
                MessageBoxA(0,"dat == 0 in WM_DESTROY", "Warning", MB_OK);
            SetWindowLong(hwndDlg, GWL_USERDATA, 0);
            
#ifdef __MATHMOD_SUPPORT			
            //mathMod begin
			CallService(MTH_Set_Srmm_HWND,0,(LPARAM) 0);	// set srmm-hwnd 0
			CallService(MTH_HIDE, 0, 0);
			//mathMod end
#endif            
            break;
    }
    return FALSE;
}

/*
 * flash a tab icon if mode = true, otherwise restore default icon
 * store flashing state into bState
 */

void FlashTab(HWND hwndTab, int iTabindex, BOOL *bState, BOOL mode, int flashImage, int origImage)
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
 * return value MUST be free()'d by caller.
 */

TCHAR *QuoteText(TCHAR *text,int charsPerLine,int removeExistingQuotes)
{
    int inChar,outChar,lineChar;
    int justDoneLineBreak,bufSize;
    TCHAR *strout;

#ifdef _UNICODE
    bufSize = lstrlenW(text) + 23;
#else
    bufSize=strlen(text)+23;
#endif
    strout=(TCHAR*)malloc(bufSize * sizeof(TCHAR));
    inChar=0;
    justDoneLineBreak=1;
    for (outChar=0,lineChar=0;text[inChar];) {
        if (outChar>=bufSize-8) {
            bufSize+=20;
            strout=(TCHAR *)realloc(strout, bufSize * sizeof(TCHAR));
        }
        if (justDoneLineBreak && text[inChar]!='\r' && text[inChar]!='\n') {
            if (removeExistingQuotes)
                if (text[inChar]=='>') {
                    while (text[++inChar]!='\n');
                    inChar++;
                    continue;
                }
            strout[outChar++]='>'; strout[outChar++]=' ';
            lineChar=2;
        }
        if (lineChar==charsPerLine && text[inChar]!='\r' && text[inChar]!='\n') {
            int decreasedBy;
            for (decreasedBy=0;lineChar>10;lineChar--,inChar--,outChar--,decreasedBy++)
                if (strout[outChar]==' ' || strout[outChar]=='\t' || strout[outChar]=='-') break;
            if (lineChar<=10) {
                lineChar+=decreasedBy; inChar+=decreasedBy; outChar+=decreasedBy;
            } else inChar++;
            strout[outChar++]='\r'; strout[outChar++]='\n';
            justDoneLineBreak=1;
            continue;
        }
        strout[outChar++]=text[inChar];
        lineChar++;
        if (text[inChar]=='\n' || text[inChar]=='\r') {
            if (text[inChar]=='\r' && text[inChar+1]!='\n')
                strout[outChar++]='\n';
            justDoneLineBreak=1;
            lineChar=0;
        } else justDoneLineBreak=0;
        inChar++;
    }
    strout[outChar++]='\r';
    strout[outChar++]='\n';
    strout[outChar]=0;
    return strout;
}

/*
 * stream function to write the contents of the message log to an rtf file
 */

static DWORD CALLBACK StreamOut(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb)
{                                                                                                        
    HANDLE hFile;
    char szName[256];

    struct MessageWindowData *dat = (struct MessageWindowData *)dwCookie;
    
    _snprintf(szName, 255, "rtflogs\\%s.rtf", (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)dat->hContact, 0));
    
    if(( hFile = CreateFileA(szName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE )      
    {                                                                                                    
        SetFilePointer(hFile, 0, NULL, FILE_END);                                                        
        WriteFile(hFile, pbBuff, cb, pcb, NULL);                                                         
        *pcb = cb;                                                                                       
        CloseHandle(hFile);                                                                              
        return 0;                                                                                        
    }                                                                                                    
    return 1;                                                                                            
}

/*
 * add a message to the sending queue.
 * iLen = length of the message in dat->sendBuffer
 */
static int AddToSendQueue(HWND hwndDlg, struct MessageWindowData *dat, int iLen)
{
    int iLength = 0;
    
    if(dat->iSendJobCurrent >= NR_SENDJOBS) {
        _DebugPopup(dat->hContact, "send queue full");
        return 0;
    }
    iLength = iLen;
    if(iLength > 0) {
        if(iLength > (int)dat->sendJobs[dat->iSendJobCurrent].dwLen) {
            if(dat->sendJobs[dat->iSendJobCurrent].sendBuffer == NULL) {
                if(iLength < HISTORY_INITIAL_ALLOCSIZE)
                    iLength = HISTORY_INITIAL_ALLOCSIZE;
                dat->sendJobs[dat->iSendJobCurrent].sendBuffer = (char *)malloc((iLength + 1) * (sizeof(TCHAR) + 1));
            }
            else
                dat->sendJobs[dat->iSendJobCurrent].sendBuffer = (char *)realloc(dat->sendJobs[dat->iSendJobCurrent].sendBuffer, (iLength + 1) * (sizeof(TCHAR) + 1));
            dat->sendJobs[dat->iSendJobCurrent].dwLen = iLength;
        }
        MoveMemory(dat->sendJobs[dat->iSendJobCurrent].sendBuffer, dat->sendBuffer, iLen * (sizeof(TCHAR) + 1));
        //_DebugPopup(dat->hContact, "Added: %s (entry: %d)", dat->sendJobs[dat->iSendJobCurrent].sendBuffer, dat->iSendJobCurrent);
        dat->iSendJobCurrent++;
        dat->iSendJobMax = dat->iSendJobCurrent > dat->iSendJobMax ? dat->iSendJobCurrent : dat->iSendJobMax;
    }

    SendMessage(hwndDlg, DM_SAVEINPUTHISTORY, 0, 0);
    SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
    EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
    SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));

    if(dat->pContainer->hwndActive == hwndDlg) {
        UpdateUnsentDisplay(hwndDlg, dat);
        UpdateReadChars(hwndDlg, dat->pContainer->hwndStatus);
    }
    UpdateSaveAndSendButton(hwndDlg, dat);
    
    if(dat->iSendJobCurrent == 1)
        SendQueuedMessage(hwndDlg, dat);

    return 0;
}

static int SendQueuedMessage(HWND hwndDlg, struct MessageWindowData *dat)
{
    int flags = 0;
    
#if defined(_UNICODE)
    flags = PREF_UNICODE;
#endif

    if(dat->hAckEvent == 0)
        dat->hAckEvent = HookEventMessage(ME_PROTO_ACK, hwndDlg, HM_EVENTSENT);
    
    if (dat->multiple) {
        HANDLE hContact, hItem;
        int sentSoFar = 0;
        dat->sendJobs[0].sendCount = 0;
        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
        do {
            hItem = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM) hContact, 0);
            if (hItem && SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM) hItem, 0)) {
                dat->sendJobs[0].sendCount++;
                dat->sendJobs[0].sendInfo = (struct MessageSendInfo *) realloc(dat->sendJobs[0].sendInfo, sizeof(struct MessageSendInfo) * dat->sendJobs[0].sendCount);
                dat->sendJobs[0].sendInfo[dat->sendJobs[0].sendCount - 1].hContact = hContact;
                if (sentSoFar < ANTIBOMB_COUNT) {
                    dat->sendJobs[0].sendInfo[dat->sendJobs[0].sendCount - 1].hSendId = (HANDLE) CallContactService(hContact, MsgServiceName(hContact), flags, (LPARAM) dat->sendJobs[0].sendBuffer);
                    sentSoFar++;
                } else
                    dat->sendJobs[0].sendInfo[dat->sendJobs[0].sendCount - 1].hSendId = NULL;
            }
        } while (hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0));
        if (dat->sendJobs[0].sendCount == 0) {
            MessageBoxA(hwndDlg, Translate("You haven't selected any contacts from the list. Click the checkbox box next to a name to send the message to that person."), Translate("Send Message"), MB_OK);
            CheckSendQueue(hwndDlg, dat);
            return 0;
        }
        if (sentSoFar < dat->sendJobs[0].sendCount)
            SetTimer(hwndDlg, TIMERID_ANTIBOMB, TIMEOUT_ANTIBOMB, NULL);
        
        if(dat->sendJobs[0].sendCount > 1)              // disable queuing more messages in multisend mode...
            EnableSending(hwndDlg, dat, FALSE);
        
    } else {
        if (dat->hContact == NULL)
            return 0;  //never happens
        
        dat->sendJobs[0].sendCount = 1;
        if(dat->sendJobs[0].sendInfo == NULL)
            dat->sendJobs[0].sendInfo = (struct MessageSendInfo *) malloc(sizeof(struct MessageSendInfo) * dat->sendJobs[0].sendCount);
        else
            dat->sendJobs[0].sendInfo = (struct MessageSendInfo *) realloc(dat->sendJobs[0].sendInfo, sizeof(struct MessageSendInfo) * dat->sendJobs[0].sendCount);
        dat->sendJobs[0].sendInfo[0].hContact = dat->hContact;
        dat->sendJobs[0].sendInfo[0].hSendId = (HANDLE) CallContactService(dat->hContact, MsgServiceName(dat->hContact), flags, (LPARAM) dat->sendJobs[0].sendBuffer);
    }

    // give icon feedback...

    HandleIconFeedback(hwndDlg, dat, g_IconSend);
    
    //create a timeout timer
    SetTimer(hwndDlg, TIMERID_MSGSEND, DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_MSGTIMEOUT, SRMSGDEFSET_MSGTIMEOUT), NULL);
    if (DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_AUTOMIN, SRMSGDEFSET_AUTOMIN))
        SendMessage(dat->pContainer->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);

    return 0;
}

static void ShiftSendQueueDown(HWND hwndDlg, struct MessageWindowData *dat)
{
    struct SendJob sendJobTemp;

    if(dat->iSendJobCurrent > 0) {
        sendJobTemp = dat->sendJobs[0];             // recycle the already allocated pointer (AddToSendQueue() will care about realloc'ing it if necessary)
        MoveMemory((void *)&dat->sendJobs[0], (void *)&dat->sendJobs[1], (NR_SENDJOBS - 1) * sizeof(struct SendJob));
        dat->iSendJobCurrent--;
        dat->sendJobs[dat->iSendJobMax - 1] = sendJobTemp;
    }
    else
        _DebugPopup(dat->hContact, "Warning: shiftsendqueue with no pending events");

    if(dat->pContainer->hwndActive == hwndDlg)
        UpdateUnsentDisplay(hwndDlg, dat);
}

/*
 * this is called when:
 *
 * ) a delivery has completed successfully
 * ) user decided to cancel a failed send
 * it removes the completed / canceled send job from the queue and schedules the next job to send (if any)
 */

static void CheckSendQueue(HWND hwndDlg, struct MessageWindowData *dat)
{
    ShiftSendQueueDown(hwndDlg, dat);
    if(dat->iSendJobCurrent == 0) {
        if(dat->hAckEvent) {
            UnhookEvent(dat->hAckEvent);
            dat->hAckEvent = NULL;
        }
        HandleIconFeedback(hwndDlg, dat, -1);
    }
    else
        SendQueuedMessage(hwndDlg, dat);        // trigger send
}

static void LogErrorMessage(HWND hwndDlg, struct MessageWindowData *dat, int iSendJobIndex, char *szErrMsg)
{
    DBEVENTINFO dbei = {0};
    int iMsgLen;
    iMsgLen = lstrlenA(dat->sendJobs[iSendJobIndex].sendBuffer) + 1;
#if defined(_UNICODE)
    iMsgLen *= 3;
#endif
    dbei.eventType = EVENTTYPE_ERRMSG;
    dbei.cbSize = sizeof(dbei);
    dbei.pBlob = (BYTE *)dat->sendJobs[iSendJobIndex].sendBuffer;
    dbei.cbBlob = iMsgLen;
    dbei.timestamp = time(NULL);
    dbei.szModule = szErrMsg;
    StreamInEvents(hwndDlg, NULL, 1, 1, &dbei);
}

static void UpdateUnsentDisplay(HWND hwndDlg, struct MessageWindowData *dat)
{
    if (dat->pContainer->hwndStatus && SendMessage(dat->pContainer->hwndStatus, SB_GETPARTS, 0, 0) >= 2) {
        char buf[128];

        _snprintf(buf, sizeof(buf), Translate("%d Unsent"), dat->iSendJobCurrent);
        SendMessageA(dat->pContainer->hwndStatus, SB_SETTEXTA, 1, (LPARAM) buf);
    }
}

static void EnableSending(HWND hwndDlg, struct MessageWindowData *dat, int iMode)
{
    SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETREADONLY, (WPARAM) iMode ? FALSE : TRUE, 0);
    EnableWindow(GetDlgItem(hwndDlg, IDC_CLIST), iMode ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDOK), iMode);
}

static void ShowErrorControls(HWND hwndDlg, struct MessageWindowData *dat, int showCmd)
{
    if(showCmd) {
        dat->dwFlags |= MWF_ERRORSTATE;
        dat->iTabImage = g_IconError;
    }
    else {
        dat->dwFlags &= ~MWF_ERRORSTATE;
        dat->iTabImage = GetProtoIconFromList(dat->szProto, dat->wStatus);
    }

#if defined(_STREAMTHREADING)
    SendMessage(GetDlgItem(hwndDlg, IDC_LOG), WM_SETREDRAW, TRUE, 0);
    InvalidateRect(GetDlgItem(hwndDlg, IDC_LOG), NULL, FALSE);
    SendMessage(hwndDlg, WM_SIZE, 0, 0);
#endif    
    ShowMultipleControls(hwndDlg, errorControls, sizeof(errorControls) / sizeof(errorControls[0]), showCmd ? SW_SHOW : SW_HIDE);

    SendMessage(hwndDlg, WM_SIZE, 0, 0);
    SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 1);
    
    if(dat->sendJobs[0].sendCount > 1)
        EnableSending(hwndDlg, dat, TRUE);
}

static void RecallFailedMessage(HWND hwndDlg, struct MessageWindowData *dat)
{
    int iLen = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MESSAGE));
    NotifyDeliveryFailure(hwndDlg, dat);
    if(iLen == 0) {                     // message area is empty, so we can recall the failed message...
#if defined(_UNICODE)
        SetDlgItemTextW(hwndDlg, IDC_MESSAGE, (LPCWSTR)&dat->sendJobs[0].sendBuffer[lstrlenA(dat->sendJobs[0].sendBuffer) + 1]);
#else
        SetDlgItemTextA(hwndDlg, IDC_MESSAGE, (char *)dat->sendJobs[0].sendBuffer);
#endif
        UpdateSaveAndSendButton(hwndDlg, dat);
    }
}
static void UpdateSaveAndSendButton(HWND hwndDlg, struct MessageWindowData *dat)
{
    int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE));
    
    if(len && !IsWindowEnabled(GetDlgItem(hwndDlg, IDOK)))
        EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);
    else if(len == 0 && IsWindowEnabled(GetDlgItem(hwndDlg, IDOK)))
        EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);

    if (len) {          // looks complex but avoids flickering on the button while typing.
        if (!(dat->dwFlags & MWF_SAVEBTN_SAV)) {
            SendDlgItemMessage(hwndDlg, IDC_SAVE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_buttonBarIcons[7]);
            SendDlgItemMessage(hwndDlg, IDC_SAVE, BUTTONADDTOOLTIP, (WPARAM) pszIDCSAVE_save, 0);
            dat->dwFlags |= MWF_SAVEBTN_SAV;
        }
    } else {
        SendDlgItemMessage(hwndDlg, IDC_SAVE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_buttonBarIcons[6]);
        SendDlgItemMessage(hwndDlg, IDC_SAVE, BUTTONADDTOOLTIP, (WPARAM) pszIDCSAVE_close, 0);
        dat->dwFlags &= ~MWF_SAVEBTN_SAV;
    }
}

static void NotifyDeliveryFailure(HWND hwndDlg, struct MessageWindowData *dat)
{
    POPUPDATA ppd;
    int     ibsize = 1023;

    if(CallService(MS_POPUP_QUERY, PUQS_GETSTATUS, 0) == 1) {
        ZeroMemory((void *)&ppd, sizeof(ppd));
        ppd.lchContact = dat->hContact;
        ppd.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
        strncpy(ppd.lpzContactName, (char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)dat->hContact,0), MAX_CONTACTNAME);
        strcpy(ppd.lpzText, "A message delivery has failed.\nClick to open the message window.");
        ppd.colorText = RGB(0,0,0);
        ppd.colorBack = RGB(255,0,0);
        ppd.PluginData = hwndDlg;
        ppd.PluginWindowProc = (WNDPROC)PopupDlgProc;
        ppd.lchIcon = g_iconErr;
        CallService(MS_POPUP_ADDPOPUP, (WPARAM)&ppd, 0);
    }
}

static int CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message) {
        case WM_COMMAND:
            if (HIWORD(wParam) == STN_CLICKED) {
                HWND hwnd;
                struct MessageWindowData *dat;
                
                hwnd = (HWND)CallService(MS_POPUP_GETPLUGINDATA, (WPARAM)hWnd,(LPARAM)&hwnd);
                dat = (struct MessageWindowData *)GetWindowLong(hwnd, GWL_USERDATA);
                if(dat) {
                    ActivateExistingTab(dat->pContainer, hwnd);
                }
                return TRUE;
            }
            break;
        case WM_LBUTTONDOWN: {
            break;
        }
        default:
            break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
 * checks, if there is a valid smileypack installed for the given protocol
 */

static int CheckValidSmileyPack(char *szProto, HICON *hButtonIcon)
{
    SMADD_INFO smainfo;

    if(g_SmileyAddAvail) {
        smainfo.cbSize = sizeof(smainfo);
        smainfo.Protocolname = szProto;
        CallService(MS_SMILEYADD_GETINFO, 0, (LPARAM)&smainfo);
        *hButtonIcon = smainfo.ButtonIcon;
        return smainfo.NumberOfVisibleSmileys;
    }
    else
        return 0;
}

/*
 * resize the default :) smiley to fit on the button (smileys may be bigger than 16x16.. so it needs to be done)
 */

static void CreateSmileyIcon(struct MessageWindowData *dat, HICON hIcon)
{
    HBITMAP hBmp, hoBmp;
    HDC hdc, hdcMem;
    BITMAPINFOHEADER bih = {0};
    int widthBytes;
    RECT rc;
    HBRUSH hBkgBrush;
    ICONINFO ii;
    BITMAP bm;

    int sizex = 0;
    int sizey = 0;
    
    GetIconInfo(hIcon, &ii);
    GetObject(ii.hbmColor, sizeof(bm), &bm);
    sizex = GetSystemMetrics(SM_CXSMICON);
    sizey = GetSystemMetrics(SM_CYSMICON);
    DeleteObject(ii.hbmMask);
    DeleteObject(ii.hbmColor);

    hBkgBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    bih.biSize = sizeof(bih);
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    
    bih.biHeight = sizex;
    bih.biPlanes = 1;
    bih.biWidth = sizey;
    widthBytes = ((bih.biWidth*bih.biBitCount + 31) >> 5) * 4;
    rc.top = rc.left = 0;
    rc.right = bih.biWidth;
    rc.bottom = bih.biHeight;
    hdc = GetDC(NULL);
    hBmp = CreateCompatibleBitmap(hdc, bih.biWidth, bih.biHeight);
    hdcMem = CreateCompatibleDC(hdc);
    hoBmp = (HBITMAP)SelectObject(hdcMem, hBmp);
    FillRect(hdcMem, &rc, hBkgBrush);
    DrawIconEx(hdcMem, 0, 0, hIcon, bih.biWidth, bih.biHeight, 0, NULL, DI_NORMAL);
    SelectObject(hdcMem, hoBmp);

    DeleteDC(hdcMem);
    DeleteObject(hoBmp);
    ReleaseDC(NULL, hdc);
    DeleteObject(hBkgBrush);
    dat->hSmileyIcon = hBmp;
}

static void SetSelftypingIcon(HWND dlg, struct MessageWindowData *dat, int iMode)
{
    if(dat->pContainer->hwndStatus && dat->pContainer->hwndActive == dlg) {
        int nParts = SendMessage(dat->pContainer->hwndStatus, SB_GETPARTS, 0, 0);
        
        if(iMode)
            SendMessage(dat->pContainer->hwndStatus, SB_SETICON, nParts - 1, (LPARAM)g_buttonBarIcons[12]);
        else
            SendMessage(dat->pContainer->hwndStatus, SB_SETICON, nParts - 1, (LPARAM)g_buttonBarIcons[13]);
        
        InvalidateRect(dat->pContainer->hwndStatus, NULL, TRUE);
    }
}

static void UpdateStatusBar(HWND hwndDlg, struct MessageWindowData *dat)
{
    if(dat->pContainer->hwndStatus && dat->pContainer->hwndActive == hwndDlg) {
        SetSelftypingIcon(hwndDlg, dat, DBGetContactSettingByte(dat->hContact, SRMSGMOD, SRMSGSET_TYPING, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW)));
        SendMessage(hwndDlg, DM_UPDATELASTMESSAGE, 0, 0);
        UpdateReadChars(hwndDlg, dat->pContainer->hwndStatus);
    }
}
static void HandleIconFeedback(HWND hwndDlg, struct MessageWindowData *dat, int iIcon)
{
    TCITEM item = {0};
    int iOldIcon;
    
    item.mask = TCIF_IMAGE;
    TabCtrl_GetItem(GetDlgItem(dat->pContainer->hwnd, IDC_MSGTABS), dat->iTabID, &item);
    iOldIcon = item.iImage;

    if(iIcon == -1) {    // restore status image
        if(dat->dwFlags & MWF_ERRORSTATE) {
            iIcon = g_IconError;
        }
        else {
            iIcon = dat->iTabImage;
        }
    }
        
    item.iImage = iIcon;
    if (dat->pContainer->iChilds > 1 || !(dat->pContainer->dwFlags & CNT_HIDETABS)) {       // we have tabs
    }
    TabCtrl_SetItem(GetDlgItem(dat->pContainer->hwnd, IDC_MSGTABS), dat->iTabID, &item);
}

int MsgWindowMenuHandler(HWND hwndDlg, struct MessageWindowData *dat, int selection, int menuId)
{
    if(menuId == MENU_PICMENU || menuId == MENU_TABCONTEXT) {
        switch(selection) {
            case ID_TABMENU_SWITCHTONEXTTAB:
                SendMessage(dat->pContainer->hwnd, DM_SELECTTAB, (WPARAM) DM_SELECT_NEXT, 0);
                return 1;
            case ID_TABMENU_SWITCHTOPREVIOUSTAB:
                SendMessage(dat->pContainer->hwnd, DM_SELECTTAB, (WPARAM) DM_SELECT_PREV, 0);
                return 1;
            case ID_TABMENU_ATTACHTOCONTAINER:
                CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SELECTCONTAINER), hwndDlg, SelectContainerDlgProc, (LPARAM) hwndDlg);
                return 1;
            case ID_TABMENU_CONTAINEROPTIONS:
                CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CONTAINEROPTIONS), hwndDlg, DlgProcContainerOptions, (LPARAM) dat->pContainer);
                return 1;
            case ID_TABMENU_CLOSECONTAINER:
                SendMessage(dat->pContainer->hwnd, WM_CLOSE, 0, 0);
                return 1;
            case ID_TABMENU_CLOSETAB:
                SendMessage(hwndDlg, WM_CLOSE, 1, 0);
                return 1;
            case ID_PICMENU_TOGGLEAVATARDISPLAY:
                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "MOD_ShowPic", dat->showPic ? 0 : 1);
                ShowPicture(hwndDlg,dat,FALSE,FALSE, FALSE);
                SendMessage(hwndDlg, DM_UPDATEPICLAYOUT, 0, 0);
                SendMessage(hwndDlg, WM_SIZE, 0, 0);
                SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 1);
                return 1;
            case ID_PICMENU_ALIGNFORMAXIMUMLOGSIZE:
                SendMessage(hwndDlg, DM_ALIGNSPLITTERMAXLOG, 0, 0);
                return 1;
            case ID_PICMENU_ALIGNFORFULL:
                SendMessage(hwndDlg, DM_ALIGNSPLITTERFULL, 0, 0);
                return 1;
            case ID_PICMENU_RESETTHEAVATAR:
                DBDeleteContactSetting(dat->hContact, SRMSGMOD_T, "MOD_Pic");
                DBDeleteContactSetting(dat->hContact, SRMSGMOD_T, "MOD_Pic_failed");
                if(dat->hContactPic)
                    DeleteObject(dat->hContactPic);
                dat->hContactPic = 0;
                DBDeleteContactSetting(dat->hContact, "ContactPhoto", "File");
                dat->pic.cx = 60;
                dat->pic.cy = 60;
                InvalidateRect(GetDlgItem(hwndDlg, IDC_CONTACTPIC), NULL, TRUE);
                SendMessage(hwndDlg, DM_ALIGNSPLITTERFULL, 0, 0);
                return 1;
            case ID_PICMENU_LOADALOCALPICTUREASAVATAR:
                {
                    char FileName[MAX_PATH];
                    char Filters[512];
                    OPENFILENAMEA ofn={0};

                    CallService(MS_UTILS_GETBITMAPFILTERSTRINGS,sizeof(Filters),(LPARAM)(char*)Filters);
                    ofn.lStructSize=sizeof(ofn);
                    ofn.hwndOwner=hwndDlg;
                    ofn.lpstrFile = FileName;
                    ofn.lpstrFilter = Filters;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags=OFN_HIDEREADONLY|OFN_FILEMUSTEXIST;
                    *FileName = '\0';
                    if (GetOpenFileNameA(&ofn))
                        DBWriteContactSettingString(dat->hContact, "ContactPhoto", "File",FileName);
                    else
                        return 1;
                }
                return 1;
        }
    }
    else if(menuId == MENU_LOGMENU) {
        int iLocalTime = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "uselocaltime", 0);
        int iRtl = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 0);
        int iLogStatus = (DBGetContactSettingByte(NULL, SRMSGMOD_T, "logstatus", 0) != 0) && (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "logstatus", -1) != 0);
        DWORD dwOldFlags = dat->dwFlags;

        switch(selection) {

            case ID_LOGMENU_SHOWTIMESTAMP:
                dat->dwFlags ^= MWF_LOG_SHOWTIME;
                return 1;
            case ID_LOGMENU_MESSAGEBODYINANEWLINE:
                dat->dwFlags  ^= MWF_LOG_NEWLINE;
                return 1;
            case ID_LOGMENU_USECONTACTSLOCALTIME:
                iLocalTime ^=1;
                if(dat->hContact) {
                    DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "uselocaltime", (BYTE) iLocalTime);
                    SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
                }
                return 1;
            case ID_LOGMENU_INDENTMESSAGEBODY:
                dat->dwFlags ^= MWF_LOG_INDENT;
                return 1;
            case ID_LOGMENU_ACTIVATERTL:
                iRtl ^= 1;
                dat->dwFlags = iRtl ? dat->dwFlags | MWF_LOG_RTL : dat->dwFlags & ~MWF_LOG_RTL;
                if(dat->hContact) {
                    DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", (BYTE) iRtl);
                    SendMessage(hwndDlg, DM_OPTIONSAPPLIED, 0, 0);
                }
                return 1;
            case ID_LOGMENU_SHOWDATE:
                dat->dwFlags ^= MWF_LOG_SHOWDATES;
                return 1;
            case ID_LOGMENU_SHOWSECONDS:
                dat->dwFlags ^= MWF_LOG_SHOWSECONDS;
                return 1;
            case ID_LOGMENU_SHOWMESSAGEICONS:
                dat->dwFlags ^= MWF_LOG_SHOWICONS;
                return 1;
            case ID_LOGMENU_SHOWNICKNAME:
                dat->dwFlags ^= MWF_LOG_SHOWNICK;
                return 1;
            case ID_LOGMENU_UNDERLINETIMESTAMP:
                dat->dwFlags ^= MWF_LOG_UNDERLINE;
                return 1;
            case ID_LOGMENU_DISPLAYTIMESTAMPAFTERNICKNAME:
                dat->dwFlags ^= MWF_LOG_SWAPNICK;
                return 1;
            case ID_LOGMENU_LOADDEFAULTS:
                dat->dwFlags &= ~(MWF_LOG_ALL);
                dat->dwFlags |= (DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT) & MWF_LOG_ALL);
                return 1;
            case ID_LOGMENU_ALWAYSUSEGLOBALSPLITTERPOSITION:
                dat->dwFlags ^= MWF_LOG_GLOBALSPLITTER;
                if(dat->dwFlags & MWF_LOG_GLOBALSPLITTER)
                    dat->splitterY = DBGetContactSettingDword(NULL, SRMSGMOD, "splitsplity", 150);
                else
                    dat->splitterY = DBGetContactSettingDword(dat->hContact, SRMSGMOD, "splitsplity", 150);
                if(dat->splitterY < dat->bottomOffset && dat->showPic)
                    dat->splitterY = dat->bottomOffset;
                if(dat->splitterY < MINSPLITTERY)
                    dat->splitterY = MINSPLITTERY;
                SendMessage(hwndDlg, WM_SIZE, 0, 0);
                return 1;
            case ID_LOGMENU_SAVESPLITTERPOSITIONASGLOBALDEFAULT:
                DBWriteContactSettingDword(NULL, SRMSGMOD, "splitsplity", dat->splitterY);
                return 1;
            case ID_LOGITEMSTOSHOW_LOGSTATUSCHANGES:
                DBWriteContactSettingByte(dat->hContact, SRMSGMOD_T, "logstatus", iLogStatus ? 0 : -1);
                return 1;
            case ID_LOGMENU_SAVETHESESETTINGSASDEFAULTVALUES:
                DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", dat->dwFlags & MWF_LOG_ALL);
                WindowList_Broadcast(hMessageWindowList, DM_DEFERREDREMAKELOG, (WPARAM)hwndDlg, (LPARAM)(dat->dwFlags & MWF_LOG_ALL));
                return 1;
            case ID_MESSAGELOGFORMATTING_SHOWGRID:
                dat->dwFlags ^= MWF_LOG_GRID;
                if(dat->dwFlags & MWF_LOG_GRID)
                    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(1,1));     // XXX margins in the log (looks slightly better)
                else
                    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0,0));     // XXX margins in the log (looks slightly better)
                return 1;
            case ID_MESSAGELOGFORMATTING_USEINDIVIDUALBACKGROUNDCOLORS:
                dat->dwFlags ^= MWF_LOG_INDIVIDUALBKG;
                return 1;
            case ID_MESSAGELOGFORMATTING_GROUPMESSAGES:
                dat->dwFlags ^= MWF_LOG_GROUPMODE;
                return 1;
            case ID_TIMESTAMPSETTINGS_USELONGDATEFORMAT:
                dat->dwFlags ^= MWF_LOG_LONGDATES;
                return 1;
            case ID_TIMESTAMPSETTINGS_USERELATIVETIMESTAMPS:
                dat->dwFlags ^= MWF_LOG_USERELATIVEDATES;
                return 1;
        }
    }
    return 0;
}

int MsgWindowUpdateMenu(HWND hwndDlg, struct MessageWindowData *dat, HMENU submenu, int menuID)
{
    if(menuID == MENU_LOGMENU) {
        int iLocalTime = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "uselocaltime", 0);
        int iRtl = DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "RTL", 0);
        int iLogStatus = (DBGetContactSettingByte(NULL, SRMSGMOD_T, "logstatus", 0) != 0) && (DBGetContactSettingByte(dat->hContact, SRMSGMOD_T, "logstatus", -1) != 0);

        CheckMenuItem(submenu, ID_LOGMENU_SHOWTIMESTAMP, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SHOWTIME ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_ALWAYSUSEGLOBALSPLITTERPOSITION, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_GLOBALSPLITTER ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_MESSAGEBODYINANEWLINE, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_NEWLINE ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_USECONTACTSLOCALTIME, MF_BYCOMMAND | iLocalTime ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_SHOWDATE, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SHOWDATES ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_SHOWSECONDS, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SHOWSECONDS ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_INDENTMESSAGEBODY, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_INDENT ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_SHOWMESSAGEICONS, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SHOWICONS ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_SHOWNICKNAME, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SHOWNICK ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_UNDERLINETIMESTAMP, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_UNDERLINE ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_ACTIVATERTL, MF_BYCOMMAND | iRtl ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGMENU_DISPLAYTIMESTAMPAFTERNICKNAME, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_SWAPNICK ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_LOGITEMSTOSHOW_LOGSTATUSCHANGES, MF_BYCOMMAND | iLogStatus ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_MESSAGELOGFORMATTING_SHOWGRID, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_GRID ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_MESSAGELOGFORMATTING_USEINDIVIDUALBACKGROUNDCOLORS, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_INDIVIDUALBKG ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_MESSAGELOGFORMATTING_GROUPMESSAGES, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_GROUPMODE ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_TIMESTAMPSETTINGS_USELONGDATEFORMAT, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_LONGDATES ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_TIMESTAMPSETTINGS_USERELATIVETIMESTAMPS, MF_BYCOMMAND | dat->dwFlags & MWF_LOG_USERELATIVEDATES ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(submenu, ID_MESSAGELOG_MESSAGELOGSETTINGSAREGLOBAL, MF_BYCOMMAND | DBGetContactSettingByte(NULL, SRMSGMOD_T, "ignorecontactsettings", 0) ? MF_UNCHECKED : MF_CHECKED);
        
        EnableMenuItem(submenu, ID_LOGMENU_SHOWDATE, dat->dwFlags & MWF_LOG_SHOWTIME ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(submenu, ID_LOGMENU_SHOWSECONDS, dat->dwFlags & MWF_LOG_SHOWTIME ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(submenu, ID_LOGMENU_LOADDEFAULTS, DBGetContactSettingByte(NULL, SRMSGMOD_T, "ignorecontactsettings", 0) ? MF_GRAYED : MF_ENABLED);
    }
    else if(menuID == MENU_PICMENU) {
        EnableMenuItem(submenu, ID_PICMENU_ALIGNFORFULL, MF_BYCOMMAND | ( dat->showPic ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(submenu, ID_PICMENU_ALIGNFORMAXIMUMLOGSIZE, MF_BYCOMMAND | ( dat->showPic ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(submenu, ID_PICMENU_RESETTHEAVATAR, MF_BYCOMMAND | ( dat->showPic ? MF_ENABLED : MF_GRAYED));
        EnableMenuItem(submenu, ID_PICMENU_LOADALOCALPICTUREASAVATAR, MF_BYCOMMAND | ( dat->showPic ? MF_ENABLED : MF_GRAYED));
    }
}

