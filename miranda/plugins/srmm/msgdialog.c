/*
SRMM

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
*/
#include "commonheaders.h"
#pragma hdrstop

#define TIMERID_MSGSEND      0
#define TIMERID_FLASHWND     1
#define TIMERID_TYPE         2
#define TIMEOUT_FLASHWND     900
#define TIMEOUT_ANTIBOMB     4000       //multiple-send bombproofing: send max 3 messages every 4 seconds
#define ANTIBOMB_COUNT       3
#define TIMEOUT_TYPEOFF      10000      //send type off after 10 seconds of inactivity
#define SB_CHAR_WIDTH        45;

#if defined(_UNICODE)
	#define SEND_FLAGS PREF_UNICODE
#else
	#define SEND_FLAGS 0
#endif

extern HCURSOR hCurSplitNS, hCurSplitWE, hCurHyperlinkHand;
extern HANDLE hMessageWindowList, hHookWinEvt;
extern struct CREOleCallback reOleCallback;
extern HINSTANCE g_hInst;
extern HBITMAP g_hbmUnknown;

static void UpdateReadChars(HWND hwndDlg, HWND hwndStatus);

static WNDPROC OldMessageEditProc, OldSplitterProc;
static const UINT infoLineControls[] = { IDC_PROTOCOL, IDC_NAME };
static const UINT buttonLineControls[] = { IDC_ADD, IDC_USERMENU, IDC_DETAILS, IDC_HISTORY, IDC_TIME };
static const UINT sendControls[] = { IDC_MESSAGE };

static void NotifyLocalWinEvent(HANDLE hContact, HWND hwnd, unsigned int type) {
	MessageWindowEventData mwe = { 0 };

	if (hContact==NULL || hwnd==NULL) return;
	mwe.cbSize = sizeof(mwe);
	mwe.hContact = hContact;
	mwe.hwndWindow = hwnd;
	mwe.szModule = SRMMMOD;
	mwe.uType = type;
	NotifyEventHooks(hHookWinEvt, 0, (LPARAM)&mwe);
}

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

static void ShowMultipleControls(HWND hwndDlg, const UINT * controls, int cControls, int state)
{
	int i;
	for (i = 0; i < cControls; i++)
		ShowWindow(GetDlgItem(hwndDlg, controls[i]), state);
}

static void SetDialogToType(HWND hwndDlg)
{
	struct MessageWindowData *dat;
	WINDOWPLACEMENT pl = { 0 };

	dat = (struct MessageWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);
	if (dat->hContact) {
		ShowMultipleControls(hwndDlg, infoLineControls, sizeof(infoLineControls) / sizeof(infoLineControls[0]), dat->showInfo ? SW_SHOW : SW_HIDE);
	}
	else
		ShowMultipleControls(hwndDlg, infoLineControls, sizeof(infoLineControls) / sizeof(infoLineControls[0]), SW_HIDE);
	if (dat->hContact) {
		ShowMultipleControls(hwndDlg, buttonLineControls, sizeof(buttonLineControls) / sizeof(buttonLineControls[0]), dat->showButton ? SW_SHOW : SW_HIDE);
		if (!DBGetContactSettingByte(dat->hContact, "CList", "NotOnList", 0))
			ShowWindow(GetDlgItem(hwndDlg, IDC_ADD), SW_HIDE);
	}
	else {
		ShowMultipleControls(hwndDlg, buttonLineControls, sizeof(buttonLineControls) / sizeof(buttonLineControls[0]), SW_HIDE);
	}
	ShowMultipleControls(hwndDlg, sendControls, sizeof(sendControls) / sizeof(sendControls[0]), SW_SHOW);
	if (!dat->hwndStatus) {
		dat->hwndStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hwndDlg, NULL, g_hInst, NULL);
		SendMessage(dat->hwndStatus, SB_SETMINHEIGHT, GetSystemMetrics(SM_CYSMICON), 0);
	}
	if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_CHARCOUNT, SRMSGDEFSET_CHARCOUNT)) {
		RECT rc;
		int statwidths[2];

		GetWindowRect(dat->hwndStatus, &rc);
		statwidths[0] = rc.right - rc.left - SB_CHAR_WIDTH;
		statwidths[1] = -1;
		SendMessage(dat->hwndStatus, SB_SETPARTS, 2, (LPARAM) statwidths);
	}
	else {
		int statwidths[] = { -1 };

		SendMessage(dat->hwndStatus, SB_SETPARTS, 1, (LPARAM) statwidths);
	}
	UpdateReadChars(hwndDlg, dat->hwndStatus);
	ShowWindow(GetDlgItem(hwndDlg, IDC_TYPINGNOTIFY), SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDCANCEL), SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_SPLITTER), SW_SHOW);
	ShowWindow(GetDlgItem(hwndDlg, IDOK), dat->showSend ? SW_SHOW : SW_HIDE);
	EnableWindow(GetDlgItem(hwndDlg, IDOK), GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE))?TRUE:FALSE);
	SendMessage(hwndDlg, DM_UPDATETITLE, 0, 0);
	SendMessage(hwndDlg, WM_SIZE, 0, 0);
	pl.length = sizeof(pl);
	GetWindowPlacement(hwndDlg, &pl);
	if (!IsWindowVisible(hwndDlg))
		pl.showCmd = SW_HIDE;
	SetWindowPlacement(hwndDlg, &pl);   //in case size is smaller than new minimum
}

struct SavedMessageData
{
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
	DWORD keyStates;            //use MOD_ defines from RegisterHotKey()
};

struct MsgEditSubclassData
{
	DWORD lastEnterTime;
	struct SavedMessageData *keyboardMsgQueue;
	int msgQueueCount;
};

static void SaveKeyboardMessage(struct MsgEditSubclassData *dat, UINT message, WPARAM wParam, LPARAM lParam)
{
	dat->keyboardMsgQueue = (struct SavedMessageData *) realloc(dat->keyboardMsgQueue, sizeof(struct SavedMessageData) * (dat->msgQueueCount + 1));
	dat->keyboardMsgQueue[dat->msgQueueCount].message = message;
	dat->keyboardMsgQueue[dat->msgQueueCount].wParam = wParam;
	dat->keyboardMsgQueue[dat->msgQueueCount].lParam = lParam;
	dat->keyboardMsgQueue[dat->msgQueueCount].keyStates = (GetKeyState(VK_SHIFT) & 0x8000 ? MOD_SHIFT : 0) | (GetKeyState(VK_CONTROL) & 0x8000 ? MOD_CONTROL : 0) | (GetKeyState(VK_MENU) & 0x8000 ? MOD_ALT : 0);
	dat->msgQueueCount++;
}

#define EM_REPLAYSAVEDKEYSTROKES  (WM_USER+0x100)
#define EM_SUBCLASSED             (WM_USER+0x101)
#define EM_UNSUBCLASSED           (WM_USER+0x102)
#define ENTERCLICKTIME   1000   //max time in ms during which a double-tap on enter will cause a send
#define EDITMSGQUEUE_PASSTHRUCLIPBOARD  //if set the typing queue won't capture ctrl-C etc because people might want to use them on the read only text
                                                  //todo: decide if this should be set or not
static LRESULT CALLBACK MessageEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct MsgEditSubclassData *dat;
	struct MessageWindowData *pdat;

	pdat=(struct MessageWindowData *)GetWindowLong(GetParent(hwnd),GWL_USERDATA);
	dat = (struct MsgEditSubclassData *) GetWindowLong(hwnd, GWL_USERDATA);
	switch (msg) {
	case EM_SUBCLASSED:
		dat = (struct MsgEditSubclassData *) malloc(sizeof(struct MsgEditSubclassData));
		SetWindowLong(hwnd, GWL_USERDATA, (LONG) dat);
		dat->lastEnterTime = 0;
		dat->keyboardMsgQueue = NULL;
		dat->msgQueueCount = 0;
		return 0;
	case EM_SETREADONLY:
		if (wParam) {
			if (dat->keyboardMsgQueue)
				free(dat->keyboardMsgQueue);
			dat->keyboardMsgQueue = NULL;
			dat->msgQueueCount = 0;
		}
		break;
	case EM_REPLAYSAVEDKEYSTROKES:
		{
			int i;
			BYTE keyStateArray[256], originalKeyStateArray[256];
			MSG msg;

			while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			GetKeyboardState(originalKeyStateArray);
			GetKeyboardState(keyStateArray);
			for (i = 0; i < dat->msgQueueCount; i++) {
				keyStateArray[VK_SHIFT] = dat->keyboardMsgQueue[i].keyStates & MOD_SHIFT ? 0x80 : 0;
				keyStateArray[VK_CONTROL] = dat->keyboardMsgQueue[i].keyStates & MOD_CONTROL ? 0x80 : 0;
				keyStateArray[VK_MENU] = dat->keyboardMsgQueue[i].keyStates & MOD_ALT ? 0x80 : 0;
				SetKeyboardState(keyStateArray);
				PostMessage(hwnd, dat->keyboardMsgQueue[i].message, dat->keyboardMsgQueue[i].wParam, dat->keyboardMsgQueue[i].lParam);
				while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			if (dat->keyboardMsgQueue)
				free(dat->keyboardMsgQueue);
			dat->keyboardMsgQueue = NULL;
			dat->msgQueueCount = 0;
			SetKeyboardState(originalKeyStateArray);
			return 0;
		}
	case WM_CHAR:
		if (GetWindowLong(hwnd, GWL_STYLE) & ES_READONLY)
			break;
		//for saved msg queue the keyup/keydowns generate wm_chars themselves
		if (wParam == '\n' || wParam == '\r') {
			if (((GetKeyState(VK_CONTROL) & 0x8000) != 0) ^ (0 != DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDONENTER, SRMSGDEFSET_SENDONENTER))) {
				PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
				return 0;
			}
			if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDONDBLENTER, SRMSGDEFSET_SENDONDBLENTER)) {
				if (dat->lastEnterTime + ENTERCLICKTIME < GetTickCount())
					dat->lastEnterTime = GetTickCount();
				else {
					SendMessage(hwnd, WM_CHAR, '\b', 0);
					PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
					return 0;
				}
			}
		}
		else
			dat->lastEnterTime = 0;
		if (wParam == 1 && GetKeyState(VK_CONTROL) & 0x8000) {      //ctrl-a
			SendMessage(hwnd, EM_SETSEL, 0, -1);
			return 0;
		}
		if (wParam == 23 && GetKeyState(VK_CONTROL) & 0x8000) {     // ctrl-w
			SendMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
			return 0;
		}
		if (wParam == 127 && GetKeyState(VK_CONTROL) & 0x8000) {    //ctrl-backspace
			DWORD start, end;
			TCHAR *text;
			int textLen;
			SendMessage(hwnd, EM_GETSEL, (WPARAM) & end, (LPARAM) (PDWORD) NULL);
			SendMessage(hwnd, WM_KEYDOWN, VK_LEFT, 0);
			SendMessage(hwnd, EM_GETSEL, (WPARAM) & start, (LPARAM) (PDWORD) NULL);
			textLen = GetWindowTextLength(hwnd);
			text = (TCHAR *) malloc(sizeof(TCHAR) * (textLen + 1));
			GetWindowText(hwnd, text, textLen + 1);
			MoveMemory(text + start, text + end, sizeof(TCHAR) * (textLen + 1 - end));
			SetWindowText(hwnd, text);
			free(text);
			SendMessage(hwnd, EM_SETSEL, start, start);
			SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), EN_CHANGE), (LPARAM) hwnd);
			return 0;
		}
		break;
	case WM_KEYUP:
		if (GetWindowLong(hwnd, GWL_STYLE) & ES_READONLY) {
			int i;
			//mustn't allow keyups for which there is no keydown
			for (i = 0; i < dat->msgQueueCount; i++)
				if (dat->keyboardMsgQueue[i].message == WM_KEYDOWN && dat->keyboardMsgQueue[i].wParam == wParam)
					break;
			if (i == dat->msgQueueCount)
				break;
	#ifdef EDITMSGQUEUE_PASSTHRUCLIPBOARD
			if (GetKeyState(VK_CONTROL) & 0x8000) {
				if (wParam == 'X' || wParam == 'C' || wParam == 'V' || wParam == VK_INSERT)
					break;
			}
			if (GetKeyState(VK_SHIFT) & 0x8000) {
				if (wParam == VK_INSERT || wParam == VK_DELETE)
					break;
			}
	#endif
			SaveKeyboardMessage(dat, msg, wParam, lParam);
			return 0;
		}
		break;
	case WM_KEYDOWN:
		if (GetWindowLong(hwnd, GWL_STYLE) & ES_READONLY) {
	#ifdef EDITMSGQUEUE_PASSTHRUCLIPBOARD
			if (GetKeyState(VK_CONTROL) & 0x8000) {
				if (wParam == 'X' || wParam == 'C' || wParam == 'V' || wParam == VK_INSERT)
					break;
			}
			if (GetKeyState(VK_SHIFT) & 0x8000) {
				if (wParam == VK_INSERT || wParam == VK_DELETE)
					break;
			}
	#endif
			SaveKeyboardMessage(dat, msg, wParam, lParam);
			return 0;
		}
		if (wParam == VK_UP && (GetKeyState(VK_CONTROL) & 0x8000) && DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_CTRLSUPPORT, SRMSGDEFSET_CTRLSUPPORT) && !DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOCLOSE, SRMSGDEFSET_AUTOCLOSE)) {
			if (pdat->cmdList) {
				if (!pdat->cmdListCurrent) {
					pdat->cmdListCurrent = tcmdlist_last(pdat->cmdList);
					SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
					SetWindowText(hwnd, pdat->cmdListCurrent->szCmd);
					SendMessage(hwnd, EM_SCROLLCARET, 0,0);
					SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
					SendMessage(hwnd, EM_SETSEL, 0, -1);
				}
				else if (pdat->cmdListCurrent->prev) {
					pdat->cmdListCurrent = pdat->cmdListCurrent->prev;
					SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
					SetWindowText(hwnd, pdat->cmdListCurrent->szCmd);
					SendMessage(hwnd, EM_SCROLLCARET, 0,0);
					SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
					SendMessage(hwnd, EM_SETSEL, 0, -1);
				}
			}
			EnableWindow(GetDlgItem(GetParent(hwnd), IDOK), GetWindowTextLength(GetDlgItem(GetParent(hwnd), IDC_MESSAGE)) != 0);
			UpdateReadChars(GetParent(hwnd), pdat->hwndStatus);
		}
		else if (wParam == VK_DOWN && (GetKeyState(VK_CONTROL) & 0x8000) && DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_CTRLSUPPORT, SRMSGDEFSET_CTRLSUPPORT) && !DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOCLOSE, SRMSGDEFSET_AUTOCLOSE)) {
			if (pdat->cmdList) {
				if (!pdat->cmdListCurrent) 
					pdat->cmdListCurrent = tcmdlist_last(pdat->cmdList);
				if (pdat->cmdListCurrent->next) {
					pdat->cmdListCurrent = pdat->cmdListCurrent->next;
					SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
					SetWindowText(hwnd, pdat->cmdListCurrent->szCmd);
					SendMessage(hwnd, EM_SCROLLCARET, 0,0);
					SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
					SendMessage(hwnd, EM_SETSEL, 0, -1);
				}
				else {
					pdat->cmdListCurrent = 0;
					SetWindowTextA(hwnd, "");
				}
			}
			EnableWindow(GetDlgItem(GetParent(hwnd), IDOK), GetWindowTextLength(GetDlgItem(GetParent(hwnd), IDC_MESSAGE)) != 0);
			UpdateReadChars(GetParent(hwnd), pdat->hwndStatus);
		}
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
		if ((wParam == 's' || wParam == 'S') && GetKeyState(VK_MENU) & 0x8000) {
			if (GetWindowLong(hwnd, GWL_STYLE) & ES_READONLY)
				SaveKeyboardMessage(dat, msg, wParam, lParam);
			else
				PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
			return 0;
		}
		break;
	case EM_UNSUBCLASSED:
		if (dat->keyboardMsgQueue)
			free(dat->keyboardMsgQueue);
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
			return 0;
	}
	return CallWindowProc(OldSplitterProc, hwnd, msg, wParam, lParam);
}

static int MessageDialogResize(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL * urc)
{
    struct MessageWindowData *dat = (struct MessageWindowData *) lParam;

    if (!dat->showInfo && !dat->showButton) {
        int i;
        for (i = 0; i < sizeof(buttonLineControls) / sizeof(buttonLineControls[0]); i++)
            if (buttonLineControls[i] == urc->wId)
                OffsetRect(&urc->rcItem, 0, -dat->lineHeight);
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
                SelectObject(hdc, (HFONT) SendMessage(GetDlgItem(hwndDlg, IDOK), WM_GETFONT, 0, 0));
                GetTextExtentPoint32(hdc, buf, lstrlen(buf), &textSize);
                urc->rcItem.right = urc->rcItem.left + textSize.cx + 10;
                if (dat->showButton && urc->rcItem.right > urc->dlgNewSize.cx - dat->nLabelRight)
                    urc->rcItem.right = urc->dlgNewSize.cx - dat->nLabelRight;
                ReleaseDC(h, hdc);
            }
        }
        case IDC_PROTOCOL:
            return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;
        case IDC_ADD:
        case IDC_USERMENU:
        case IDC_DETAILS:
        case IDC_HISTORY:
        case IDC_TIME:
            return RD_ANCHORX_RIGHT | RD_ANCHORY_TOP;
        case IDC_LOG:
            if (!dat->showInfo && !dat->showButton)
                urc->rcItem.top -= dat->lineHeight;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            return RD_ANCHORX_WIDTH | RD_ANCHORY_HEIGHT;
        case IDC_SPLITTER:
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            return RD_ANCHORX_WIDTH | RD_ANCHORY_BOTTOM;
        case IDC_MESSAGE:
        {
            if (!dat->showSend)
                urc->rcItem.right = urc->dlgNewSize.cx - urc->rcItem.left;
			if (dat->showAvatar) {
				urc->rcItem.left = dat->avatarWidth+4;
			}
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            if (!dat->showSend)
                return RD_ANCHORX_CUSTOM | RD_ANCHORY_BOTTOM;
            return RD_ANCHORX_WIDTH | RD_ANCHORY_BOTTOM;
        }
        case IDCANCEL:
        case IDOK:
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            return RD_ANCHORX_RIGHT | RD_ANCHORY_BOTTOM;
        case IDC_TYPINGNOTIFY:
            return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;
		case IDC_AVATAR:
            urc->rcItem.top=urc->rcItem.bottom-(dat->avatarHeight + 2);
            urc->rcItem.right=urc->rcItem.left+(dat->avatarWidth + 2);
            return RD_ANCHORX_LEFT|RD_ANCHORY_BOTTOM;
    }
    return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;
}

static void UpdateReadChars(HWND hwndDlg, HWND hwndStatus)
{
	if (hwndStatus && SendMessage(hwndStatus, SB_GETPARTS, 0, 0) == 2) {
		TCHAR buf[128];
		int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE));

		_sntprintf(buf, sizeof(buf), _T("%d"), len);
		SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM) buf);
	}
}

void ShowAvatar(HWND hwndDlg, struct MessageWindowData *dat) {
	DBVARIANT dbv;

	if (dat->avatarPic!=g_hbmUnknown) {
		DeleteObject(dat->avatarPic);
        dat->avatarPic=NULL;
	}
	if (DBGetContactSetting(dat->hContact, SRMMMOD, SRMSGSET_AVATAR, &dbv)) {
		dat->avatarPic=g_hbmUnknown;
		SendMessage(hwndDlg, DM_AVATARCALCSIZE, 0, 0);
		SendMessage(hwndDlg, DM_UPDATESIZEBAR, 0, 0);
		SendMessage(hwndDlg, DM_AVATARSIZECHANGE, 0, 0);
	}
	else {
		HANDLE hFile;

		if((hFile = CreateFileA(dbv.pszVal, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
			dat->avatarPic=g_hbmUnknown;
			SendMessage(hwndDlg, DM_AVATARCALCSIZE, 0, 0);
			SendMessage(hwndDlg, DM_UPDATESIZEBAR, 0, 0);
			SendMessage(hwndDlg, DM_AVATARSIZECHANGE, 0, 0);
		}
		else {
			dat->avatarPic=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);
			SendMessage(hwndDlg, DM_AVATARCALCSIZE, 0, 0);
			SendMessage(hwndDlg, DM_UPDATESIZEBAR, 0, 0);
			SendMessage(hwndDlg, DM_AVATARSIZECHANGE, 0, 0);
			CloseHandle(hFile);
		}
		DBFreeVariant(&dbv);
	}
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
	if (!DBGetContactSettingByte(dat->hContact, SRMMMOD, SRMSGSET_TYPING, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW)))
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
		&& !DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_TYPINGUNKNOWN, SRMSGDEFSET_TYPINGUNKNOWN))
		return;
	// End user check
	dat->nTypeMode = mode;
	CallService(MS_PROTO_SELFISTYPING, (WPARAM) dat->hContact, dat->nTypeMode);
}

BOOL CALLBACK DlgProcMessage(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct MessageWindowData *dat;

	dat = (struct MessageWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);
	switch (msg) {
	case WM_INITDIALOG:
		{
			struct NewMessageWindowLParam *newData = (struct NewMessageWindowLParam *) lParam;
			TranslateDialogDefault(hwndDlg);
			dat = (struct MessageWindowData *) malloc(sizeof(struct MessageWindowData));
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) dat);
			{
				dat->hContact = newData->hContact;
				NotifyLocalWinEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_OPENING);
				if (newData->szInitialText) {
					int len;
					SetDlgItemTextA(hwndDlg, IDC_MESSAGE, newData->szInitialText);
					len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE));
					PostMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_SETSEL, len, len);
				}
			}
			dat->szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) dat->hContact, 0);
			{ // avatar stuff
				dat->avatarPic = 0;
				dat->showAvatar = 0;
				dat->limitAvatarH = 0;
				if (CallProtoService(dat->szProto, PS_GETCAPS, PFLAGNUM_4, 0)&PF4_AVATARS) {
					dat->avatarPic = g_hbmUnknown;
					dat->showAvatar = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AVATARENABLE, SRMSGDEFSET_AVATARENABLE);
					dat->limitAvatarH = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_LIMITAVHEIGHT, SRMSGDEFSET_LIMITAVHEIGHT)?DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_AVHEIGHT, SRMSGDEFSET_AVHEIGHT):0;
				}
				SendMessage(hwndDlg, DM_AVATARCALCSIZE, 0, 0);
			}
			if (dat->hContact && dat->szProto != NULL)
				dat->wStatus = DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE);
			else
				dat->wStatus = ID_STATUS_OFFLINE;
			dat->wOldStatus = dat->wStatus;
			dat->hAckEvent = NULL;
			dat->sendInfo = NULL;
			dat->showInfo = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWINFOLINE, SRMSGDEFSET_SHOWINFOLINE);
			dat->showButton = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWBUTTONLINE, SRMSGDEFSET_SHOWBUTTONLINE);
			dat->showSend = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDBUTTON, SRMSGDEFSET_SENDBUTTON);
			dat->hNewEvent = HookEventMessage(ME_DB_EVENT_ADDED, hwndDlg, HM_DBEVENTADDED);
			dat->hBkgBrush = NULL;
			dat->hDbEventFirst = NULL;
			dat->sendBuffer = NULL;
			dat->splitterY = (int) DBGetContactSettingDword(NULL, SRMMMOD, "splittery", (DWORD) - 1);
			dat->windowWasCascaded = 0;
			dat->nFlash = 0;
			dat->nTypeSecs = 0;
			dat->nLastTyping = 0;
			dat->showTyping = 0;
			dat->cmdList = 0;
			dat->cmdListCurrent = 0;
			dat->showTypingWin = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGWIN, SRMSGDEFSET_SHOWTYPINGWIN);
			dat->nTypeMode = PROTOTYPE_SELFTYPING_OFF;
			SetTimer(hwndDlg, TIMERID_TYPE, 1000, NULL);
			dat->lastMessage = 0;
			dat->nFlashMax = DBGetContactSettingByte(NULL, SRMMMOD, "FlashMax", 4);
			{
				RECT rc, rc2;
				GetWindowRect(GetDlgItem(hwndDlg, IDC_USERMENU), &rc);
				GetWindowRect(hwndDlg, &rc2);
				dat->nLabelRight = rc2.right - rc.left;
			}
			{
				RECT rc;
				POINT pt;
				GetWindowRect(GetDlgItem(hwndDlg, IDC_SPLITTER), &rc);
				pt.y = (rc.top + rc.bottom) / 2;
				pt.x = 0;
				ScreenToClient(hwndDlg, &pt);
				dat->originalSplitterY = pt.y;
				if (dat->splitterY == -1)
					dat->splitterY = dat->originalSplitterY + 60;
			}
			WindowList_Add(hMessageWindowList, hwndDlg, dat->hContact);
			GetWindowRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &dat->minEditInit);
			SendMessage(hwndDlg, DM_UPDATESIZEBAR, 0, 0);
			dat->hwndStatus = NULL;
			dat->hIcons[0] = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IDI_ADDCONTACT), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
			dat->hIcons[1] = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IsWinVerXPPlus()? IDI_USERDETAILS32 : IDI_USERDETAILS), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
			dat->hIcons[2] = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IsWinVerXPPlus()? IDI_HISTORY32 : IDI_HISTORY), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
			dat->hIcons[3] = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IsWinVerXPPlus()? IDI_TIMESTAMP32 : IDI_TIMESTAMP), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
			dat->hIcons[4] = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IsWinVerXPPlus()? IDI_DOWNARROW32 : IDI_DOWNARROW), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
			dat->hIcons[5] = (HICON) LoadImage(g_hInst, MAKEINTRESOURCE(IsWinVerXPPlus()? IDI_TYPING32 : IDI_TYPING), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
			SendDlgItemMessage(hwndDlg, IDC_ADD, BM_SETIMAGE, IMAGE_ICON, (LPARAM) dat->hIcons[0]);
			SendDlgItemMessage(hwndDlg, IDC_DETAILS, BM_SETIMAGE, IMAGE_ICON, (LPARAM) dat->hIcons[1]);
			SendDlgItemMessage(hwndDlg, IDC_HISTORY, BM_SETIMAGE, IMAGE_ICON, (LPARAM) dat->hIcons[2]);
			SendDlgItemMessage(hwndDlg, IDC_TIME, BM_SETIMAGE, IMAGE_ICON, (LPARAM) dat->hIcons[3]);
			SendDlgItemMessage(hwndDlg, IDC_USERMENU, BM_SETIMAGE, IMAGE_ICON, (LPARAM) dat->hIcons[4]);
			// Make them pushbuttons
			SendDlgItemMessage(hwndDlg, IDC_TIME, BUTTONSETASPUSHBTN, 0, 0);
			// Make them flat buttons
			{
				int i;
				
				SendMessage(GetDlgItem(hwndDlg, IDC_NAME), BUTTONSETASFLATBTN, 0, 0);
				for (i = 0; i < sizeof(buttonLineControls) / sizeof(buttonLineControls[0]); i++)
					SendMessage(GetDlgItem(hwndDlg, buttonLineControls[i]), BUTTONSETASFLATBTN, 0, 0);
			}
			SendMessage(GetDlgItem(hwndDlg, IDC_ADD), BUTTONADDTOOLTIP, (WPARAM) Translate("Add Contact Permanently to List"), 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_USERMENU), BUTTONADDTOOLTIP, (WPARAM) Translate("User Menu"), 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_DETAILS), BUTTONADDTOOLTIP, (WPARAM) Translate("View User's Details"), 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_HISTORY), BUTTONADDTOOLTIP, (WPARAM) Translate("View User's History"), 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_TIME), BUTTONADDTOOLTIP, (WPARAM) Translate("Display Time in Message Log"), 0);

			EnableWindow(GetDlgItem(hwndDlg, IDC_PROTOCOL), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_TYPINGNOTIFY), FALSE);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETOLECALLBACK, 0, (LPARAM) & reOleCallback);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_LINK);
			/* duh, how come we didnt use this from the start? */
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_AUTOURLDETECT, (WPARAM) TRUE, 0);
			if (dat->hContact) {
				if (dat->szProto) {
					int nMax;
					nMax = CallProtoService(dat->szProto, PS_GETCAPS, PFLAG_MAXLENOFMESSAGE, (LPARAM) dat->hContact);
					if (nMax)
						SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_LIMITTEXT, (WPARAM) nMax, 0);
					/* get around a lame bug in the Windows template resource code where richedits are limited to 0x7FFF */
					SendDlgItemMessage(hwndDlg, IDC_LOG, EM_LIMITTEXT, (WPARAM) sizeof(TCHAR) * 0x7FFFFFFF, 0);
				}
			}
			OldMessageEditProc = (WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_WNDPROC, (LONG) MessageEditSubclassProc);
			SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SUBCLASSED, 0, 0);
			OldSplitterProc = (WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_WNDPROC, (LONG) SplitterSubclassProc);
			if (dat->hContact) {
				int historyMode = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_LOADHISTORY, SRMSGDEFSET_LOADHISTORY);
				// This finds the first message to display, it works like shit
				dat->hDbEventFirst = (HANDLE) CallService(MS_DB_EVENT_FINDFIRSTUNREAD, (WPARAM) dat->hContact, 0);
				switch (historyMode) {
				case LOADHISTORY_COUNT:
					{
						int i;
						HANDLE hPrevEvent;
						DBEVENTINFO dbei = { 0 };
						dbei.cbSize = sizeof(dbei);
						for (i = DBGetContactSettingWord(NULL, SRMMMOD, SRMSGSET_LOADCOUNT, SRMSGDEFSET_LOADCOUNT); i > 0; i--) {
							if (dat->hDbEventFirst == NULL)
								hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDLAST, (WPARAM) dat->hContact, 0);
							else
								hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDPREV, (WPARAM) dat->hDbEventFirst, 0);
							if (hPrevEvent == NULL)
								break;
							dbei.cbBlob = 0;
							dat->hDbEventFirst = hPrevEvent;
							CallService(MS_DB_EVENT_GET, (WPARAM) dat->hDbEventFirst, (LPARAM) & dbei);
							if (!DbEventIsShown(&dbei, dat))
								i++;
						}
						break;
					}
				case LOADHISTORY_TIME:
					{
						HANDLE hPrevEvent;
						DBEVENTINFO dbei = { 0 };
						DWORD firstTime;

						dbei.cbSize = sizeof(dbei);
						if (dat->hDbEventFirst == NULL)
							dbei.timestamp = time(NULL);
						else
							CallService(MS_DB_EVENT_GET, (WPARAM) dat->hDbEventFirst, (LPARAM) & dbei);
						firstTime = dbei.timestamp - 60 * DBGetContactSettingWord(NULL, SRMMMOD, SRMSGSET_LOADTIME, SRMSGDEFSET_LOADTIME);
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
						}
						break;
					}
				}
			}
			SendMessage(hwndDlg, DM_OPTIONSAPPLIED, 0, 0);

			{
				int savePerContact = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SAVEPERCONTACT, SRMSGDEFSET_SAVEPERCONTACT);
				if (Utils_RestoreWindowPosition(hwndDlg, savePerContact ? dat->hContact : NULL, SRMMMOD, "split")) {
					if (savePerContact) {
						if (Utils_RestoreWindowPositionNoMove(hwndDlg, NULL, SRMMMOD, "split"))
							SetWindowPos(hwndDlg, 0, 0, 0, 450, 300, SWP_NOZORDER | SWP_NOMOVE);
					}
					else
						SetWindowPos(hwndDlg, 0, 0, 0, 450, 300, SWP_NOZORDER | SWP_NOMOVE);
				}
				if (!savePerContact && DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_CASCADE, SRMSGDEFSET_CASCADE))
					WindowList_Broadcast(hMessageWindowList, DM_CASCADENEWWINDOW, (WPARAM) hwndDlg, (LPARAM) & dat->windowWasCascaded);
			}
			{
				DBEVENTINFO dbei = { 0 };
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
			
			dat->hAvatarAck = HookEventMessage(ME_PROTO_ACK, hwndDlg, HM_AVATARACK);
			SendMessage(hwndDlg, DM_GETAVATAR, 0, 0);
			ShowWindow(hwndDlg, SW_SHOWNORMAL);
			NotifyLocalWinEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_OPEN);
			return TRUE;
		}
	case WM_CONTEXTMENU:
		{
			if (dat->hwndStatus && dat->hwndStatus == (HWND) wParam) {
				POINT pt;
				HMENU hMenu = (HMENU) CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM) dat->hContact, 0);

				GetCursorPos(&pt);
				TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, hwndDlg, NULL);
				DestroyMenu(hMenu);
			}
			break;
		}
	case HM_AVATARACK:
	{
		ACKDATA *pAck = (ACKDATA *)lParam;
		PROTO_AVATAR_INFORMATION *pai = (PROTO_AVATAR_INFORMATION *)pAck->hProcess;
		HWND hwnd = 0;
		
		if (pAck->hContact!=dat->hContact)
			return 0;
		if (pAck->type != ACKTYPE_AVATAR)
			return 0;
		if (pAck->result == ACKRESULT_SUCCESS) {
			if (pai->filename&&strlen(pai->filename)) {
				DBWriteContactSettingString(dat->hContact, SRMMMOD, SRMSGSET_AVATAR, pai->filename);
				ShowAvatar(hwndDlg, dat);
			}
		}
		else if (pAck->result == ACKRESULT_STATUS) {
			SendMessage(hwndDlg, DM_GETAVATAR, 0, 0);
		}
		else if (pAck->result == ACKRESULT_FAILED) {
			DBDeleteContactSetting(dat->hContact, SRMMMOD, SRMSGSET_AVATAR);
			SendMessage(hwndDlg, DM_GETAVATAR, 0, 0);
		}
		break;
	}
	case DM_AVATARCALCSIZE:
	{
		BITMAP bminfo;

		if (dat->avatarPic==0||dat->showAvatar==0) {
			dat->avatarWidth=50;
			dat->avatarHeight=50;
			ShowWindow(GetDlgItem(hwndDlg, IDC_AVATAR), SW_HIDE);
		}
		GetObject(dat->avatarPic, sizeof(bminfo), &bminfo);
		if (dat->limitAvatarH) {
			double aspect = 0;

			aspect = (double)dat->limitAvatarH / (double)bminfo.bmHeight;
			dat->avatarWidth = (int)(bminfo.bmWidth * aspect + 2); 
			dat->avatarHeight = dat->limitAvatarH + 2;
		}
		else {
			dat->avatarWidth=bminfo.bmWidth+2;
			dat->avatarHeight=bminfo.bmHeight+2;
		}
		ShowWindow(GetDlgItem(hwndDlg, IDC_AVATAR), SW_SHOW);
		break;
	}
	case DM_UPDATESIZEBAR:
	{
		RECT rc;

		dat->minEditBoxSize.cx = dat->minEditInit.right - dat->minEditInit.left;
		dat->minEditBoxSize.cy = dat->minEditInit.bottom - dat->minEditInit.top;
		if (dat->showAvatar) {
			if (dat->minEditBoxSize.cy<=dat->avatarHeight)
				dat->minEditBoxSize.cy = dat->avatarHeight;
		}
		
		GetWindowRect(GetDlgItem(hwndDlg, IDC_ADD), &rc);
		dat->lineHeight = rc.bottom - rc.top + 3;
		break;
	}
	case DM_AVATARSIZECHANGE:
	{
		RECT rc;
		GetWindowRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc);

		if (rc.bottom-rc.top<dat->minEditBoxSize.cy) {
			SendMessage(hwndDlg, DM_SPLITTERMOVED, rc.top-(rc.bottom-rc.top-dat->minEditBoxSize.cy-4), (LPARAM) GetDlgItem(hwndDlg, IDC_SPLITTER));
		}
		SendMessage(hwndDlg, WM_SIZE, 0, 0);
		break;
	}
	case DM_GETAVATAR:
	{
		PROTO_AVATAR_INFORMATION pai;
		int caps = 0, result;
		
		SetWindowLong(hwndDlg, DWL_MSGRESULT, 0);
        if (dat->showAvatar==0) {
			SetWindowLong(hwndDlg, DWL_MSGRESULT, 1);
			return 0;
		}
		if(DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE) {
			ShowAvatar(hwndDlg, dat);
			SetWindowLong(hwndDlg, DWL_MSGRESULT, 1);
			return 0;
		}
		ZeroMemory((void *)&pai, sizeof(pai));
		pai.cbSize = sizeof(pai);
		pai.hContact = dat->hContact;
		pai.format = PA_FORMAT_BMP;
		strcpy(pai.filename, "");
		result = CallProtoService(dat->szProto, PS_GETAVATARINFO, GAIF_FORCE, (LPARAM)&pai);
		if (result==GAIR_SUCCESS) {
			DBWriteContactSettingString(dat->hContact, SRMMMOD, SRMSGSET_AVATAR, pai.filename);
			ShowAvatar(hwndDlg, dat);
		}
		else if (result==GAIR_NOAVATAR) {
			DBDeleteContactSetting(dat->hContact, SRMMMOD, SRMSGSET_AVATAR);
			ShowAvatar(hwndDlg, dat);
		}
		SetWindowLong(hwndDlg, DWL_MSGRESULT, 1);
		break;
	}
	case DM_TYPING:
		{
			dat->nTypeSecs = (int) lParam > 0 ? (int) lParam : 0;
			break;
		}
	case DM_UPDATEWINICON:
		{
			if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_STATUSICON, SRMSGDEFSET_STATUSICON)) {
				WORD wStatus;

				if (dat->szProto) {
					wStatus = DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE);
					SendMessage(hwndDlg, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) LoadSkinnedProtoIcon(dat->szProto, wStatus));
					break;
				}
			}
			SendMessage(hwndDlg, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) LoadSkinnedIcon(SKINICON_EVENT_MESSAGE));
			break;
		}
        case DM_USERNAMETOCLIP:
		{
			CONTACTINFO ci;
			char buf[128];
			HGLOBAL hData;
			
			buf[0] = 0;
			if(dat->hContact) {
				ZeroMemory(&ci, sizeof(ci));
				ci.cbSize = sizeof(ci);
				ci.hContact = dat->hContact;
				ci.szProto = dat->szProto;
				ci.dwFlag = CNF_UNIQUEID;
				if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
					switch (ci.type) {
						case CNFT_ASCIIZ:
							_snprintf(buf, sizeof(buf), "%s", ci.pszVal);
							miranda_sys_free(ci.pszVal);
							break;
						case CNFT_DWORD:
							_snprintf(buf, sizeof(buf), "%u", ci.dVal);
							break;
					}
				}
				if (!OpenClipboard(hwndDlg) || !lstrlenA(buf)) break;
				EmptyClipboard();
				hData = GlobalAlloc(GMEM_MOVEABLE, lstrlenA(buf) + 1);
				lstrcpyA(GlobalLock(hData), buf);
				GlobalUnlock(hData);
				SetClipboardData(CF_TEXT, hData);
				CloseClipboard();
			}
			break;
		}
	case DM_UPDATELASTMESSAGE:
		{
			if (!dat->hwndStatus || dat->nTypeSecs)
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
				_snprintf(fmt, sizeof(fmt), Translate("Last message received on %s at %s."), date, time);
				SendMessageA(dat->hwndStatus, SB_SETTEXTA, 0, (LPARAM) fmt);
				SendMessage(dat->hwndStatus, SB_SETICON, 0, (LPARAM) NULL);
			}
			else {
				SendMessageA(dat->hwndStatus, SB_SETTEXTA, 0, (LPARAM) "");
				SendMessage(dat->hwndStatus, SB_SETICON, 0, (LPARAM) NULL);
			}
			break;
		}
	case DM_OPTIONSAPPLIED:
		dat->showInfo = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWINFOLINE, SRMSGDEFSET_SHOWINFOLINE);
		dat->showButton = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWBUTTONLINE, SRMSGDEFSET_SHOWBUTTONLINE);
		dat->showSend = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDBUTTON, SRMSGDEFSET_SENDBUTTON);
		dat->showTypingWin = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGWIN, SRMSGDEFSET_SHOWTYPINGWIN);
		SetDialogToType(hwndDlg);
		if (dat->hBkgBrush)
			DeleteObject(dat->hBkgBrush);
		{
			COLORREF colour = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR);
			dat->hBkgBrush = CreateSolidBrush(colour);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETBKGNDCOLOR, 0, colour);
		}
		{ // avatar stuff
			dat->avatarPic = 0;
			dat->showAvatar = 0;
			dat->limitAvatarH = 0;
			if (CallProtoService(dat->szProto, PS_GETCAPS, PFLAGNUM_4, 0)&PF4_AVATARS) {
				dat->avatarPic = g_hbmUnknown;
				dat->showAvatar = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AVATARENABLE, SRMSGDEFSET_AVATARENABLE);
				dat->limitAvatarH = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_LIMITAVHEIGHT, SRMSGDEFSET_LIMITAVHEIGHT)?DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_AVHEIGHT, SRMSGDEFSET_AVHEIGHT):0;
			}
			SendMessage(hwndDlg, DM_AVATARCALCSIZE, 0, 0);
			SendMessage(hwndDlg, DM_UPDATESIZEBAR, 0, 0);
		}
		dat->showTime = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTIME, SRMSGDEFSET_SHOWTIME);
		dat->showIcons = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWLOGICONS, SRMSGDEFSET_SHOWLOGICONS);
		dat->showDate = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWDATE, SRMSGDEFSET_SHOWDATE);
		dat->hideNames = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_HIDENAMES, SRMSGDEFSET_HIDENAMES);
		SendDlgItemMessage(hwndDlg, IDC_TIME, BUTTONSETASPUSHBTN, 0, 0);
		CheckDlgButton(hwndDlg, IDC_TIME, dat->showTime ? BST_CHECKED : BST_UNCHECKED);
		InvalidateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, FALSE);
		{
			HFONT hFont;
			LOGFONTA lf;
			hFont = (HFONT) SendDlgItemMessage(hwndDlg, IDC_MESSAGE, WM_GETFONT, 0, 0);
			if (hFont != NULL && hFont != (HFONT) SendDlgItemMessage(hwndDlg, IDOK, WM_GETFONT, 0, 0))
				DeleteObject(hFont);
			LoadMsgDlgFont(MSGFONTID_MESSAGEAREA, &lf, NULL);
			hFont = CreateFontIndirectA(&lf);
			SendDlgItemMessage(hwndDlg, IDC_MESSAGE, WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0));
		}
		SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
		SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
		break;
	case DM_UPDATETITLE:
		{
			char newtitle[256], oldtitle[256];
			char *szStatus, *contactName, *pszNewTitleEnd;
			DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) wParam;

			pszNewTitleEnd = "Message Session";
			if (dat->hContact) {
				if (dat->szProto) {
					CONTACTINFO ci;
					char buf[128];
					int statusIcon = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_STATUSICON, SRMSGDEFSET_STATUSICON);
					
					buf[0] = 0;
					dat->wStatus = DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE);
					contactName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) dat->hContact, 0);
					ZeroMemory(&ci, sizeof(ci));
					ci.cbSize = sizeof(ci);
					ci.hContact = dat->hContact;
					ci.szProto = dat->szProto;
					ci.dwFlag = CNF_UNIQUEID;
					if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
						switch (ci.type) {
						case CNFT_ASCIIZ:
							_snprintf(buf, sizeof(buf), "%s", ci.pszVal);
							miranda_sys_free(ci.pszVal);
							break;
						case CNFT_DWORD:
							_snprintf(buf, sizeof(buf), "%u", ci.dVal);
							break;
						}
					}
					SetDlgItemTextA(hwndDlg, IDC_NAME, buf[0] ? buf : contactName);
					szStatus = (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, dat->szProto == NULL ? ID_STATUS_OFFLINE : DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE), 0);
					if (statusIcon)
						_snprintf(newtitle, sizeof(newtitle), "%s - %s", contactName, Translate(pszNewTitleEnd));
					else
						_snprintf(newtitle, sizeof(newtitle), "%s (%s): %s", contactName, szStatus, Translate(pszNewTitleEnd));
					if (!cws || (!strcmp(cws->szModule, dat->szProto) && !strcmp(cws->szSetting, "Status"))) {
						InvalidateRect(GetDlgItem(hwndDlg, IDC_PROTOCOL), NULL, TRUE);
						if (statusIcon) {
							SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
						}
					}

					// log
					if ((dat->wStatus != dat->wOldStatus || lParam != 0)
						&& DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWSTATUSCH, SRMSGDEFSET_SHOWSTATUSCH)) {
							DBEVENTINFO dbei;
							char buffer[450];
							HANDLE hNewEvent;
							int iLen;

							char *szOldStatus = (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM) dat->wOldStatus, 0);
							char *szNewStatus = (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM) dat->wStatus, 0);

							if (dat->wStatus == ID_STATUS_OFFLINE) {
								_snprintf(buffer, sizeof(buffer), Translate("signed off (was %s)"), szOldStatus);
							}
							else if (dat->wOldStatus == ID_STATUS_OFFLINE) {
								_snprintf(buffer, sizeof(buffer), Translate("signed on (%s)"), szNewStatus);
							}
							else {
								_snprintf(buffer, sizeof(buffer), Translate("is now %s (was %s)"), szNewStatus, szOldStatus);
							}
							iLen = strlen(buffer) + 1;
							MultiByteToWideChar(CP_ACP, 0, buffer, iLen, (LPWSTR) & buffer[iLen], iLen);
							dbei.cbSize = sizeof(dbei);
							dbei.pBlob = (PBYTE) buffer;
							dbei.cbBlob = (strlen(buffer) + 1) * (sizeof(TCHAR) + 1);
							dbei.eventType = EVENTTYPE_STATUSCHANGE;
							dbei.flags = 0;
							dbei.timestamp = time(NULL);
							dbei.szModule = dat->szProto;
							hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) dat->hContact, (LPARAM) & dbei);
							if (dat->hDbEventFirst == NULL) {
								dat->hDbEventFirst = hNewEvent;
								SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
							}
						}
						dat->wOldStatus = dat->wStatus;
				}
			}
			else
				lstrcpynA(newtitle, pszNewTitleEnd, sizeof(newtitle));
			GetWindowTextA(hwndDlg, oldtitle, sizeof(oldtitle));
			if (lstrcmpA(newtitle, oldtitle)) { //swt() flickers even if the title hasn't actually changed
				SetWindowTextA(hwndDlg, newtitle);
				SendMessage(hwndDlg, WM_SIZE, 0, 0);
			}
			break;
		}
	case DM_CASCADENEWWINDOW:
		if ((HWND) wParam == hwndDlg)
			break;
		{
			RECT rcThis, rcNew;
			GetWindowRect(hwndDlg, &rcThis);
			GetWindowRect((HWND) wParam, &rcNew);
			if (abs(rcThis.left - rcNew.left) < 3 && abs(rcThis.top - rcNew.top) < 3) {
				int offset = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
				SetWindowPos((HWND) wParam, 0, rcNew.left + offset, rcNew.top + offset, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
				*(int *) lParam = 1;
			}
		}
		break;
	case WM_ACTIVATE:
		if (LOWORD(wParam) != WA_ACTIVE)
			break;
		//fall through
	case WM_MOUSEACTIVATE:
		if (KillTimer(hwndDlg, TIMERID_FLASHWND))
			FlashWindow(hwndDlg, FALSE);
		break;
	case WM_SETFOCUS:
		SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
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
			if (IsIconic(hwndDlg))
				break;
			if (dat->hwndStatus) {
				SendMessage(dat->hwndStatus, WM_SIZE, 0, 0);
				if (SendMessage(dat->hwndStatus, SB_GETPARTS, 0, 0) == 2) {
					int statwidths[2];
					RECT rc;

					GetWindowRect(dat->hwndStatus, &rc);
					statwidths[0] = rc.right - rc.left - SB_CHAR_WIDTH;
					statwidths[1] = -1;
					SendMessage(dat->hwndStatus, SB_SETPARTS, 2, (LPARAM) statwidths);
				}
			}
			ZeroMemory(&urd, sizeof(urd));
			urd.cbSize = sizeof(urd);
			urd.hInstance = g_hInst;
			urd.hwndDlg = hwndDlg;
			urd.lParam = (LPARAM) dat;
			urd.lpTemplate = MAKEINTRESOURCEA(IDD_MSG);
			urd.pfnResizer = MessageDialogResize;
			CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM) & urd);
			// The statusbar sometimes draws over these 2 controls so 
			// redraw them
			if (dat->hwndStatus) {
				RedrawWindow(GetDlgItem(hwndDlg, IDOK), NULL, NULL, RDW_INVALIDATE);
				RedrawWindow(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, NULL, RDW_INVALIDATE);
			}
			break;
		}
	case DM_SPLITTERMOVED:
		{
			POINT pt;
			RECT rc;
			RECT rcLog;
			GetWindowRect(GetDlgItem(hwndDlg, IDC_LOG), &rcLog);
			if ((HWND) lParam == GetDlgItem(hwndDlg, IDC_SPLITTER)) {
				int oldSplitterY;
				GetClientRect(hwndDlg, &rc);
				pt.x = 0;
				pt.y = wParam;
				ScreenToClient(hwndDlg, &pt);

				oldSplitterY = dat->splitterY;
				dat->splitterY = rc.bottom - pt.y + 23;
				GetWindowRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc);
				if (rc.bottom - rc.top + (dat->splitterY - oldSplitterY) < dat->minEditBoxSize.cy)
					dat->splitterY = oldSplitterY + dat->minEditBoxSize.cy - (rc.bottom - rc.top);
				if (rcLog.bottom - rcLog.top - (dat->splitterY - oldSplitterY) < dat->minEditBoxSize.cy)
					dat->splitterY = oldSplitterY - dat->minEditBoxSize.cy + (rcLog.bottom - rcLog.top);
			}
			SendMessage(hwndDlg, WM_SIZE, 0, 0);
			break;
		}
	case DM_REMAKELOG:
		StreamInEvents(hwndDlg, dat->hDbEventFirst, -1, 0);
		break;
	case DM_APPENDTOLOG:   //takes wParam=hDbEvent
		StreamInEvents(hwndDlg, (HANDLE) wParam, 1, 1);
		break;
	case DM_SCROLLLOGTOBOTTOM:
		{
			SCROLLINFO si = { 0 };
			if ((GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG), GWL_STYLE) & WS_VSCROLL) == 0)
				break;
			si.cbSize = sizeof(si);
			si.fMask = SIF_PAGE | SIF_RANGE;
			GetScrollInfo(GetDlgItem(hwndDlg, IDC_LOG), SB_VERT, &si);
			si.fMask = SIF_POS;
			si.nPos = si.nMax - si.nPage + 1;
			SetScrollInfo(GetDlgItem(hwndDlg, IDC_LOG), SB_VERT, &si, TRUE);
			PostMessage(GetDlgItem(hwndDlg, IDC_LOG), WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
			break;
		}
	case HM_DBEVENTADDED:
		if ((HANDLE) wParam != dat->hContact)
			break;
		if (dat->hContact == NULL)
			break;
		{
			DBEVENTINFO dbei = { 0 };

			dbei.cbSize = sizeof(dbei);
			dbei.cbBlob = 0;
			CallService(MS_DB_EVENT_GET, lParam, (LPARAM) & dbei);
			if (dat->hDbEventFirst == NULL)
				dat->hDbEventFirst = (HANDLE) lParam;
			if (dbei.eventType == EVENTTYPE_MESSAGE && (dbei.flags & DBEF_READ))
				break;
			if (DbEventIsShown(&dbei, dat)) {
				if (dbei.eventType == EVENTTYPE_MESSAGE && dat->hwndStatus && !(dbei.flags & (DBEF_SENT))) {
					dat->lastMessage = dbei.timestamp;
					SendMessage(hwndDlg, DM_UPDATELASTMESSAGE, 0, 0);
				}
				if ((HANDLE) lParam != dat->hDbEventFirst && (HANDLE) CallService(MS_DB_EVENT_FINDNEXT, lParam, 0) == NULL)
					SendMessage(hwndDlg, DM_APPENDTOLOG, lParam, 0);
				else
					SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
				if ((GetActiveWindow() != hwndDlg || GetForegroundWindow() != hwndDlg) && !(dbei.flags & DBEF_SENT)
					&& dbei.eventType != EVENTTYPE_STATUSCHANGE) {
						SetTimer(hwndDlg, TIMERID_FLASHWND, TIMEOUT_FLASHWND, NULL);
					}           //if
			}
			break;
		}
	case WM_TIMER:
		if (wParam == TIMERID_MSGSEND) {
			KillTimer(hwndDlg, wParam);
			ShowWindow(hwndDlg, SW_SHOWNORMAL);
			EnableWindow(hwndDlg, FALSE);
			CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_MSGSENDERROR), hwndDlg, ErrorDlgProc, (LPARAM) Translate("The message send timed out."));
		}
		else if (wParam == TIMERID_FLASHWND) {
			FlashWindow(hwndDlg, TRUE);
			if (dat->nFlash > dat->nFlashMax) {
				KillTimer(hwndDlg, TIMERID_FLASHWND);
				FlashWindow(hwndDlg, FALSE);
				dat->nFlash = 0;
			}
			dat->nFlash++;
		}
		else if (wParam == TIMERID_TYPE) {
			if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON && GetTickCount() - dat->nLastTyping > TIMEOUT_TYPEOFF) {
				NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
			}
			if (dat->showTyping) {
				if (dat->nTypeSecs) {
					dat->nTypeSecs--;
					if (GetForegroundWindow() == hwndDlg)
						SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
				}
				else {
					SendMessage(hwndDlg, DM_UPDATELASTMESSAGE, 0, 0);
					if (dat->showTypingWin)
						SendMessage(hwndDlg, DM_UPDATEWINICON, 0, 0);
					dat->showTyping = 0;
				}
			}
			else {
				if (dat->nTypeSecs) {
					char szBuf[256];
					char *szContactName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) dat->hContact, 0);

					_snprintf(szBuf, sizeof(szBuf), Translate("%s is typing a message..."), szContactName);
					dat->nTypeSecs--;
					SendMessageA(dat->hwndStatus, SB_SETTEXTA, 0, (LPARAM) szBuf);
					SendMessage(dat->hwndStatus, SB_SETICON, 0, (LPARAM) dat->hIcons[5]);
					if (dat->showTypingWin && GetForegroundWindow() != hwndDlg)
						SendMessage(hwndDlg, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) dat->hIcons[5]);
					dat->showTyping = 1;
				}
			}
		}
		break;
	case DM_ERRORDECIDED:
		EnableWindow(hwndDlg, TRUE);
		switch (wParam) {
		case MSGERROR_CANCEL:
			EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);
			SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETREADONLY, FALSE, 0);
			SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
			if (dat->hAckEvent) {
				UnhookEvent(dat->hAckEvent);
				dat->hAckEvent = NULL;
			}
			break;
		case MSGERROR_RETRY:
			{
				int i;
				for (i = 0; i < dat->sendCount; i++) {
					if (dat->sendInfo[i].hSendId == NULL && dat->hContact == NULL)
						continue;
					dat->sendInfo[i].hSendId = (HANDLE) CallContactService(dat->hContact, MsgServiceName(dat->hContact), SEND_FLAGS, (LPARAM) dat->sendBuffer);
				}
			}
			SetTimer(hwndDlg, TIMERID_MSGSEND, DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_MSGTIMEOUT, SRMSGDEFSET_MSGTIMEOUT), NULL);
			break;
			}
			break;
		case WM_CTLCOLOREDIT:
			{
				COLORREF colour;
				if ((HWND) lParam != GetDlgItem(hwndDlg, IDC_MESSAGE))
					break;
				LoadMsgDlgFont(MSGFONTID_MESSAGEAREA, NULL, &colour);
				SetTextColor((HDC) wParam, colour);
				SetBkColor((HDC) wParam, DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR));
				return (BOOL) dat->hBkgBrush;
			}
		case WM_MEASUREITEM:
			return CallService(MS_CLIST_MENUMEASUREITEM, wParam, lParam);
		case WM_DRAWITEM:
			{
				LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;
				if (dis->hwndItem == GetDlgItem(hwndDlg, IDC_PROTOCOL)) {
					if (dat->szProto) {
						HICON hIcon;
						int dwStatus;

						dwStatus = DBGetContactSettingWord(dat->hContact, dat->szProto, "Status", ID_STATUS_OFFLINE);
						hIcon = LoadSkinnedProtoIcon(dat->szProto, dwStatus);
						if (hIcon) {
							if (DBGetContactSettingDword(dat->hContact, dat->szProto, "IdleTS", 0)) {
								HIMAGELIST hImageList;

								hImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON), IsWinVerXPPlus()? ILC_COLOR32 | ILC_MASK : ILC_COLOR16 | ILC_MASK, 1, 0);
								ImageList_AddIcon(hImageList, hIcon);
								ImageList_DrawEx(hImageList, 0, dis->hDC, dis->rcItem.left, dis->rcItem.top, 0, 0, CLR_NONE, CLR_NONE, ILD_SELECTED);
								ImageList_RemoveAll(hImageList);
								ImageList_Destroy(hImageList);
							}
							else DrawIconEx(dis->hDC, dis->rcItem.left, dis->rcItem.top, hIcon, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL);
						}
					}
				}
				else if (dis->hwndItem == GetDlgItem(hwndDlg, IDC_TYPINGNOTIFY)) {
					DrawIconEx(dis->hDC, dis->rcItem.left, dis->rcItem.top, dat->hIcons[5], GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL);
				}
				else if (dis->hwndItem == GetDlgItem(hwndDlg, IDC_AVATAR) && dat->avatarPic && dat->showAvatar) {
					BITMAP bminfo;
					HPEN hPen;

                    hPen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
                    SelectObject(dis->hDC, hPen);
                    Rectangle(dis->hDC, 0, 0, dat->avatarWidth, dat->avatarHeight);
                    DeleteObject(hPen);
					GetObject(dat->avatarPic, sizeof(bminfo), &bminfo);
					{
						HDC hdcMem = CreateCompatibleDC(dis->hDC);
                        HBITMAP hbmMem = (HBITMAP)SelectObject(hdcMem, dat->avatarPic);
						
						if (dat->limitAvatarH) {
							double aspect = 0, w = 0;

							aspect = (double)dat->limitAvatarH / (double)bminfo.bmHeight;
							w = (double)bminfo.bmWidth * aspect; 
							SetStretchBltMode(dis->hDC, HALFTONE);
                            StretchBlt(dis->hDC, 1, 1, (int)w, dat->limitAvatarH, hdcMem, 0, 0, bminfo.bmWidth, bminfo.bmHeight, SRCCOPY);
						}
						else
							BitBlt(dis->hDC, 1, 1, bminfo.bmWidth, bminfo.bmHeight, hdcMem, 0, 0, SRCCOPY);
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
				if (!IsWindowEnabled(GetDlgItem(hwndDlg, IDOK)))
					break;
				{
					//this is a 'send' button
					int bufSize;

					bufSize = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MESSAGE)) + 1;
					dat->sendBuffer = (char *) realloc(dat->sendBuffer, bufSize * (sizeof(TCHAR) + 1));
					GetDlgItemTextA(hwndDlg, IDC_MESSAGE, dat->sendBuffer, bufSize);
			#if defined( _UNICODE )
					GetDlgItemTextW(hwndDlg, IDC_MESSAGE, (TCHAR *) & dat->sendBuffer[bufSize], bufSize);
			#endif
					if (dat->sendBuffer[0] == 0)
						break;
			#if defined( _UNICODE )
					dat->cmdList = tcmdlist_append(dat->cmdList, (TCHAR *) & dat->sendBuffer[bufSize]);
			#else
					dat->cmdList = tcmdlist_append(dat->cmdList, dat->sendBuffer);
			#endif
					dat->cmdListCurrent = 0;
					if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON) {
						NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
					}

					dat->hAckEvent = HookEventMessage(ME_PROTO_ACK, hwndDlg, HM_EVENTSENT);
					if (dat->hContact == NULL)
						break;      //never happens
					dat->sendCount = 1;
					dat->sendInfo = (struct MessageSendInfo *) realloc(dat->sendInfo, sizeof(struct MessageSendInfo) * dat->sendCount);
					dat->sendInfo[0].hSendId = (HANDLE) CallContactService(dat->hContact, MsgServiceName(dat->hContact), SEND_FLAGS, (LPARAM) dat->sendBuffer);
					EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
					SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETREADONLY, TRUE, 0);

					//create a timeout timer
					SetTimer(hwndDlg, TIMERID_MSGSEND, DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_MSGTIMEOUT, SRMSGDEFSET_MSGTIMEOUT), NULL);
					if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOMIN, SRMSGDEFSET_AUTOMIN))
						ShowWindow(hwndDlg, SW_MINIMIZE);
				}
				return TRUE;
			case IDCANCEL:
				DestroyWindow(hwndDlg);
				return TRUE;
			case IDC_USERMENU:
			case IDC_NAME:
				{
					if(GetKeyState(VK_SHIFT) & 0x8000) {    // copy user name
                            SendMessage(hwndDlg, DM_USERNAMETOCLIP, 0, 0);
                    }
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
				CallService(MS_HISTORY_SHOWCONTACTHISTORY, (WPARAM) dat->hContact, 0);
				break;
			case IDC_DETAILS:
				CallService(MS_USERINFO_SHOWDIALOG, (WPARAM) dat->hContact, 0);
				break;
			case IDC_TIME:
				dat->showTime ^= 1;
				DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTIME, (BYTE) dat->showTime);
				SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
				break;
			case IDC_ADD:
				{
					ADDCONTACTSTRUCT acs = { 0 };

					acs.handle = dat->hContact;
					acs.handleType = HANDLE_CONTACT;
					acs.szProto = 0;
					CallService(MS_ADDCONTACT_SHOW, (WPARAM) hwndDlg, (LPARAM) & acs);
				}
				if (!DBGetContactSettingByte(dat->hContact, "CList", "NotOnList", 0)) {
					ShowWindow(GetDlgItem(hwndDlg, IDC_ADD), FALSE);
				}
				break;
			case IDC_MESSAGE:
				if (HIWORD(wParam) == EN_CHANGE) {
					int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE));
					UpdateReadChars(hwndDlg, dat->hwndStatus);
					EnableWindow(GetDlgItem(hwndDlg, IDOK), len != 0);
					if (!(GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_SHIFT) & 0x8000)) {
						dat->nLastTyping = GetTickCount();
						if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE))) {
							if (dat->nTypeMode == PROTOTYPE_SELFTYPING_OFF) {
								NotifyTyping(dat, PROTOTYPE_SELFTYPING_ON);
							}
						}
						else {
							if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON) {
								NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
							}
						}
					}
				}
				break;
				}
				break;
			case WM_NOTIFY:
				switch (((NMHDR *) lParam)->idFrom) {
			case IDC_LOG:
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
					CHARRANGE sel, all = { 0, -1 };

					hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_CONTEXT));
					hSubMenu = GetSubMenu(hMenu, 0);
					CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hSubMenu, 0);
					SendMessage(((NMHDR *) lParam)->hwndFrom, EM_EXGETSEL, 0, (LPARAM) & sel);
					if (sel.cpMin == sel.cpMax)
						EnableMenuItem(hSubMenu, IDM_COPY, MF_BYCOMMAND | MF_GRAYED);
					pt.x = (short) LOWORD(((ENLINK *) lParam)->lParam);
					pt.y = (short) HIWORD(((ENLINK *) lParam)->lParam);
					ClientToScreen(((NMHDR *) lParam)->hwndFrom, &pt);
					switch (TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL)) {
					case IDM_COPY:
						SendMessage(((NMHDR *) lParam)->hwndFrom, WM_COPY, 0, 0);
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
							DestroyMenu(hMenu);
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
						}
						else {
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
	case HM_EVENTSENT:
		{
			ACKDATA *ack = (ACKDATA *) lParam;
			DBEVENTINFO dbei = { 0 };
			HANDLE hNewEvent;
			int i;

			if (ack->type != ACKTYPE_MESSAGE)
				break;

			switch (ack->result) {
			case ACKRESULT_FAILED:
				KillTimer(hwndDlg, TIMERID_MSGSEND);
				ShowWindow(hwndDlg, SW_SHOWNORMAL);
				EnableWindow(hwndDlg, FALSE);
				CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_MSGSENDERROR), hwndDlg, ErrorDlgProc, (LPARAM) strdup((char *) ack->lParam));
				return 0;
			}                   //switch

			for (i = 0; i < dat->sendCount; i++)
				if (ack->hProcess == dat->sendInfo[i].hSendId && ack->hContact == dat->hContact)
					break;
			if (i == dat->sendCount)
				break;

			dbei.cbSize = sizeof(dbei);
			dbei.eventType = EVENTTYPE_MESSAGE;
			dbei.flags = DBEF_SENT;
			dbei.szModule = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) dat->hContact, 0);
			dbei.timestamp = time(NULL);
			dbei.cbBlob = lstrlenA(dat->sendBuffer) + 1;
	#if defined( _UNICODE )
			dbei.cbBlob *= sizeof(TCHAR) + 1;
	#endif
			dbei.pBlob = (PBYTE) dat->sendBuffer;
			hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) dat->hContact, (LPARAM) & dbei);
			SkinPlaySound("SendMsg");
			if (dat->hDbEventFirst == NULL) {
				dat->hDbEventFirst = hNewEvent;
				SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
			}

			dat->sendInfo[i].hSendId = NULL;
			for (i = 0; i < dat->sendCount; i++)
				if (dat->sendInfo[i].hSendId)
					break;
			if (i == dat->sendCount) {
				int len;
				//all messages sent
				dat->sendCount = 0;
				free(dat->sendInfo);
				dat->sendInfo = NULL;
				KillTimer(hwndDlg, TIMERID_MSGSEND);
				SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
				EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
				SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETREADONLY, FALSE, 0);
				SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_REPLAYSAVEDKEYSTROKES, 0, 0);
				if (GetForegroundWindow() == hwndDlg)
					SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
				len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE));
				UpdateReadChars(hwndDlg, dat->hwndStatus);
				EnableWindow(GetDlgItem(hwndDlg, IDOK), len != 0);
				if (dat->hAckEvent) {
					UnhookEvent(dat->hAckEvent);
					dat->hAckEvent = NULL;
				}
				if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOCLOSE, SRMSGDEFSET_AUTOCLOSE))
					DestroyWindow(hwndDlg);
			}
			break;
		}
	case WM_DESTROY:
		NotifyLocalWinEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_CLOSING);
		if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON) {
			NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
		}
		if (dat->hAckEvent)
			UnhookEvent(dat->hAckEvent);
		if (dat->sendInfo)
			free(dat->sendInfo);
		if (dat->hNewEvent)
			UnhookEvent(dat->hNewEvent);
		if (dat->hBkgBrush)
			DeleteObject(dat->hBkgBrush);
		if (dat->sendBuffer != NULL)
			free(dat->sendBuffer);
		if (dat->hwndStatus)
			DestroyWindow(dat->hwndStatus);
		if (dat->hAvatarAck)
			UnhookEvent(dat->hAvatarAck);
		if (dat->avatarPic&&dat->avatarPic!=g_hbmUnknown)
			DeleteObject(dat->avatarPic);
		{
			int i;
			for (i = 0; i < sizeof(dat->hIcons) / sizeof(dat->hIcons[0]); i++)
				DestroyIcon(dat->hIcons[i]);
		}
		tcmdlist_free(dat->cmdList);
		WindowList_Remove(hMessageWindowList, hwndDlg);
		DBWriteContactSettingDword(NULL, SRMMMOD, "splittery", dat->splitterY);
		SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_WNDPROC, (LONG) OldSplitterProc);
		SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_UNSUBCLASSED, 0, 0);
		SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_WNDPROC, (LONG) OldMessageEditProc);
		{
			HFONT hFont;
			hFont = (HFONT) SendDlgItemMessage(hwndDlg, IDC_MESSAGE, WM_GETFONT, 0, 0);
			if (hFont != NULL && hFont != (HFONT) SendDlgItemMessage(hwndDlg, IDOK, WM_GETFONT, 0, 0))
				DeleteObject(hFont);
		}
		{
			WINDOWPLACEMENT wp = { 0 };
			HANDLE hContact;

			if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SAVEPERCONTACT, SRMSGDEFSET_SAVEPERCONTACT))
				hContact = dat->hContact;
			else
				hContact = NULL;
			wp.length = sizeof(wp);
			GetWindowPlacement(hwndDlg, &wp);
			if (!dat->windowWasCascaded) {
				DBWriteContactSettingDword(hContact, SRMMMOD, "splitx", wp.rcNormalPosition.left);
				DBWriteContactSettingDword(hContact, SRMMMOD, "splity", wp.rcNormalPosition.top);
			}
			DBWriteContactSettingDword(hContact, SRMMMOD, "splitwidth", wp.rcNormalPosition.right - wp.rcNormalPosition.left);
			DBWriteContactSettingDword(hContact, SRMMMOD, "splitheight", wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
		}
		if (dat->hContact&&DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_DELTEMP, SRMSGDEFSET_DELTEMP)) {
			if (DBGetContactSettingByte(dat->hContact, "CList", "NotOnList", 0)) {
				CallService(MS_DB_CONTACT_DELETE, (WPARAM)dat->hContact, 0);
			}
		}
		NotifyLocalWinEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_CLOSE);
		free(dat);
		SetWindowLong(hwndDlg, GWL_USERDATA, 0);
		break;
	}
	return FALSE;
}
