/*
Scriver

Copyright 2000-2007 Miranda ICQ/IM project,

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
#include "m_metacontacts.h"
#include "statusicon.h"

#include <malloc.h>
#include <commctrl.h>

#define TIMERID_MSGSEND      0
#define TIMERID_FLASHWND     1
#define TIMERID_TYPE         2
#define TIMERID_UNREAD       3
#define TIMEOUT_ANTIBOMB     4000       //multiple-send bombproofing: send max 3 messages every 4 seconds
#define ANTIBOMB_COUNT       3
#define TIMEOUT_TYPEOFF      10000      //send type off after 10 seconds of inactivity
#define TIMEOUT_UNREAD		 800       //multiple-send bombproofing: send max 3 messages every 4 seconds
#define VALID_AVATAR(x)      (x==PA_FORMAT_PNG||x==PA_FORMAT_JPEG||x==PA_FORMAT_ICON||x==PA_FORMAT_BMP||x==PA_FORMAT_GIF)

#define ENTERCLICKTIME   1000   //max time in ms during which a double-tap on enter will cause a send

extern HCURSOR hCurSplitNS, hCurSplitWE, hCurHyperlinkHand, hDragCursor;
extern HANDLE hHookWinEvt;
extern HANDLE hHookWinPopup;
extern struct CREOleCallback reOleCallback, reOleCallback2;
extern HINSTANCE g_hInst;

static void UpdateReadChars(HWND hwndDlg, struct MessageWindowData * dat);

static WNDPROC OldMessageEditProc, OldSplitterProc, OldLogEditProc;
static TCHAR *buttonNames[] = {_T("User Menu"), _T("User Details"), _T("Smiley"), _T("Add Contact"), _T("History"), _T("Quote"), _T("Close"), _T("Send")};
static const UINT buttonLineControls[] = { IDC_USERMENU, IDC_DETAILS, IDC_SMILEYS, IDC_ADD, IDC_HISTORY, IDC_QUOTE, IDCANCEL, IDOK};
static char buttonAlignment[] = { 0, 0, 0, 1, 1, 1, 1, 1};
static UINT buttonSpacing[] = { 0, 0, 12, 0, 0, 0, 0, 0};
static UINT buttonWidth[] = { 24, 24, 24, 24, 24, 24, 24, 38};


static DWORD CALLBACK EditStreamCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb)
{
    char *szFilename = (char *)dwCookie;
    FILE *file;
	file = fopen(szFilename, "ab");
	if (file != NULL) {
		*pcb = fwrite(pbBuff, cb, 1, file);
		fclose(file);
		return 0;
	}
    return 1;
}

static DWORD CALLBACK StreamOutCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb)
{
	MessageSendQueueItem * msi = (MessageSendQueueItem *) dwCookie;
	msi->sendBuffer = (char *)mir_realloc(msi->sendBuffer, msi->sendBufferSize + cb + 2);
	memcpy (msi->sendBuffer + msi->sendBufferSize, pbBuff, cb);
	msi->sendBufferSize += cb;
	*((TCHAR *)(msi->sendBuffer+msi->sendBufferSize)) = '\0';
	*pcb = cb;
    return 0;
}

TCHAR *GetRichEditSelection(HWND hwnd) {
	CHARRANGE sel;
	SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM)&sel);
	if (sel.cpMin!=sel.cpMax) {
		MessageSendQueueItem msi;
		EDITSTREAM stream;
		DWORD dwFlags = 0;
		ZeroMemory(&stream, sizeof(stream));
		stream.pfnCallback = StreamOutCallback;
		stream.dwCookie = (DWORD) &msi;
#if defined( _UNICODE )
		dwFlags = SF_TEXT|SF_UNICODE|SFF_SELECTION;
#else
		dwFlags = SF_TEXT|SFF_SELECTION;
#endif
		msi.sendBuffer = NULL;
		msi.sendBufferSize = 0;
		SendMessage(hwnd, EM_STREAMOUT, (WPARAM)dwFlags, (LPARAM) & stream);
		return (TCHAR *)msi.sendBuffer;
	}
	return NULL;
}

void SaveLog(HWND hwnd, const char *filename) {
	EDITSTREAM stream;
	DWORD dwFlags = 0;
	ZeroMemory(&stream, sizeof(stream));
	stream.pfnCallback = EditStreamCallback;
	stream.dwCookie = (DWORD) filename;
	dwFlags = SF_TEXT|SF_UNICODE;
	SendMessage(hwnd, EM_STREAMOUT, (WPARAM)dwFlags, (LPARAM) &stream);
}

static TCHAR *GetIEViewSelection(struct MessageWindowData *dat) {
	IEVIEWEVENT event;
	ZeroMemory(&event, sizeof(event));
	event.cbSize = sizeof(event);
#ifdef _UNICODE
	event.dwFlags = 0;
#else
	event.dwFlags = IEEF_NO_UNICODE;
#endif
	event.codepage = dat->windowData.codePage;
	event.hwnd = dat->hwndLog;
	event.hContact = dat->windowData.hContact;
	event.iType = IEE_GET_SELECTION;
	return mir_tstrdup((TCHAR *)CallService(MS_IEVIEW_EVENT, 0, (LPARAM)&event));
}

static TCHAR *GetQuotedTextW(TCHAR * text) {
	int i, j, l, newLine, wasCR;
	TCHAR *out;
#ifdef _UNICODE
	l = wcslen(text);
#else
	l = strlen(text);
#endif
	newLine = 1;
	wasCR = 0;
	for (i=j=0; i<l; i++) {
		if (text[i]=='\r') {
			wasCR = 1;
			newLine = 1;
			j += text[i+1]!='\n' ? 2 : 1;
		} else if (text[i]=='\n') {
			newLine = 1;
			j += wasCR ? 1 : 2;
			wasCR = 0;
		} else {
			j++;
			if (newLine) {
				//for (;i<l && text[i]=='>';i++) j--;
				j+=2;
			}
			newLine = 0;
			wasCR = 0;
		}
	}
	j+=3;
	out = (TCHAR *)mir_alloc(sizeof(TCHAR) * j);
	newLine = 1;
	wasCR = 0;
	for (i=j=0; i<l; i++) {
		if (text[i]=='\r') {
			wasCR = 1;
			newLine = 1;
			out[j++] = '\r';
			if (text[i+1]!='\n') {
				out[j++]='\n';
			}
		} else if (text[i]=='\n') {
			newLine = 1;
			if (!wasCR) {
				out[j++]='\r';
			}
			out[j++]='\n';
			wasCR = 0;
		} else {
			if (newLine) {
				out[j++]='>';
				out[j++]=' ';
				//for (;i<l && text[i]=='>';i++) j--;
			}
			newLine = 0;
			wasCR = 0;
			out[j++]=text[i];
		}
	}
	out[j++]='\r';
	out[j++]='\n';
	out[j++]='\0';
	return out;
}

static void saveDraftMessage(HWND hwnd, HANDLE hContact, int codepage) {
	char *textBuffer = GetRichTextEncoded(hwnd, codepage);
	if (textBuffer != NULL) {
		g_dat->draftList = tcmdlist_append2(g_dat->draftList, hContact, textBuffer);
		mir_free(textBuffer);
	} else {
		g_dat->draftList = tcmdlist_remove2(g_dat->draftList, hContact);
	}
}

void NotifyLocalWinEvent(HANDLE hContact, HWND hwnd, unsigned int type) {
	MessageWindowEventData mwe = { 0 };
	BOOL bChat = FALSE;
	if (hContact==NULL || hwnd==NULL) return;
	mwe.cbSize = sizeof(mwe);
	mwe.hContact = hContact;
	mwe.hwndWindow = hwnd;
	mwe.szModule = SRMMMOD;
	mwe.uType = type;
	mwe.uFlags = MSG_WINDOW_UFLAG_MSG_BOTH;
	bChat = (WindowList_Find(g_dat->hMessageWindowList, hContact) == NULL);
	mwe.hwndInput = GetDlgItem(hwnd, bChat ? IDC_CHAT_MESSAGE : IDC_MESSAGE);
	mwe.hwndLog = GetDlgItem(hwnd, bChat ? IDC_CHAT_LOG : IDC_LOG);
	NotifyEventHooks(hHookWinEvt, 0, (LPARAM)&mwe);
}

static BOOL IsUtfSendAvailable(HANDLE hContact)
{
	char* szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
	if ( szProto == NULL )
		return FALSE;

	return ( CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_4, 0) & PF4_IMSENDUTF ) ? TRUE : FALSE;
}

#if defined(_UNICODE)
int RTL_Detect(WCHAR *pszwText)
{
    WORD *infoTypeC2;
    int i;
    int iLen = lstrlenW(pszwText);

    infoTypeC2 = (WORD *)mir_alloc(sizeof(WORD) * (iLen + 2));

    if(infoTypeC2) {
        ZeroMemory(infoTypeC2, sizeof(WORD) * (iLen + 2));

        GetStringTypeW(CT_CTYPE2, pszwText, iLen, infoTypeC2);

        for(i = 0; i < iLen; i++) {
            if(infoTypeC2[i] == C2_RIGHTTOLEFT) {
                mir_free(infoTypeC2);
                return 1;
            }
        }
        mir_free(infoTypeC2);
    }
    return 0;
}
#endif

static void AddToFileList(char ***pppFiles,int *totalCount,const TCHAR* szFilename)
{
	*pppFiles=(char**)mir_realloc(*pppFiles,(++*totalCount+1)*sizeof(char*));
	(*pppFiles)[*totalCount] = NULL;

	#if defined( _UNICODE )
	{
		TCHAR tszShortName[ MAX_PATH ];
		char  szShortName[ MAX_PATH ];
		BOOL  bIsDefaultCharUsed = FALSE;
		WideCharToMultiByte( CP_ACP, 0, szFilename, -1, szShortName, sizeof( szShortName ), NULL, &bIsDefaultCharUsed );
		if ( bIsDefaultCharUsed ) {
			if ( GetShortPathName( szFilename, tszShortName, SIZEOF( tszShortName )) == 0 )
		      WideCharToMultiByte( CP_ACP, 0, szFilename, -1, szShortName, sizeof( szShortName ), NULL, NULL );
			else
				WideCharToMultiByte( CP_ACP, 0, tszShortName, -1, szShortName, sizeof( szShortName ), NULL, NULL );
		}
		(*pppFiles)[*totalCount-1] = mir_strdup( szShortName );
	}
	#else
		(*pppFiles)[*totalCount-1] = mir_strdup( szFilename );
	#endif

	if ( GetFileAttributes(szFilename) & FILE_ATTRIBUTE_DIRECTORY ) {
		WIN32_FIND_DATA fd;
		HANDLE hFind;
		TCHAR szPath[MAX_PATH];
		lstrcpy(szPath,szFilename);
		lstrcat(szPath,_T("\\*"));
		if (( hFind = FindFirstFile( szPath, &fd )) != INVALID_HANDLE_VALUE ) {
			do {
				if ( !lstrcmp(fd.cFileName,_T(".")) || !lstrcmp(fd.cFileName,_T(".."))) continue;
				lstrcpy(szPath,szFilename);
				lstrcat(szPath,_T("\\"));
				lstrcat(szPath,fd.cFileName);
				AddToFileList(pppFiles,totalCount,szPath);
			}
				while( FindNextFile( hFind,&fd ));
			FindClose( hFind );
}	}	}

static int GetToolbarWidth()
{
	int i, w = 0;
	for (i = 0; i < sizeof(buttonLineControls) / sizeof(buttonLineControls[0]); i++) {
//		if (g_dat->buttonVisibility & (1 << i)) {
			if (buttonLineControls[i] != IDC_SMILEYS || g_dat->smileyServiceExists) {
				w += buttonWidth[i] + buttonSpacing[i];
			}
//		}
	}
	return w;
}

static void ShowMultipleControls(HWND hwndDlg, const UINT * controls, int cControls, int state)
{
	int i;
	for (i = 0; i < cControls; i++)
		ShowWindow(GetDlgItem(hwndDlg, controls[i]), (g_dat->buttonVisibility & (1 << i)) ? state : SW_HIDE);
}

static void SetDialogToType(HWND hwndDlg)
{
	struct MessageWindowData *dat;
	ParentWindowData *pdat;

	dat = (struct MessageWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);
	pdat = (ParentWindowData *) GetWindowLong(GetParent(hwndDlg), GWL_USERDATA);
	if (dat->windowData.hContact) {
		ShowMultipleControls(hwndDlg, buttonLineControls, sizeof(buttonLineControls) / sizeof(buttonLineControls[0]), (pdat->flags2&SMF2_SHOWTOOLBAR) ? SW_SHOW : SW_HIDE);
		if (!DBGetContactSettingByte(dat->windowData.hContact, "CList", "NotOnList", 0)) {
			ShowWindow(GetDlgItem(hwndDlg, IDC_ADD), SW_HIDE);
		}
		if (!g_dat->smileyServiceExists) {
			ShowWindow(GetDlgItem(hwndDlg, IDC_SMILEYS), SW_HIDE);
		}
	} else {
		ShowMultipleControls(hwndDlg, buttonLineControls, sizeof(buttonLineControls) / sizeof(buttonLineControls[0]), SW_HIDE);
	}
	ShowWindow(GetDlgItem(hwndDlg, IDC_MESSAGE), SW_SHOW);
	if (dat->hwndLog != NULL) {
		ShowWindow (GetDlgItem(hwndDlg, IDC_LOG), SW_HIDE);
	} else {
		ShowWindow (GetDlgItem(hwndDlg, IDC_LOG), SW_SHOW);
	}
	ShowWindow(GetDlgItem(hwndDlg, IDC_SPLITTER), SW_SHOW);
	UpdateReadChars(hwndDlg, dat);
	EnableWindow(GetDlgItem(hwndDlg, IDOK), GetRichTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE), dat->windowData.codePage, FALSE)?TRUE:FALSE);
	SendMessage(hwndDlg, DM_UPDATETITLEBAR, 0, 0);
	SendMessage(hwndDlg, WM_SIZE, 0, 0);
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
};

static LRESULT CALLBACK LogEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
/*	switch (msg) {

		case WM_CHAR:
			if (!(GetKeyState(VK_CONTROL) & 0x8000)) {
				SetFocus(GetDlgItem(GetParent(hwnd), IDC_MESSAGE));
				SendMessage(GetDlgItem(GetParent(hwnd), IDC_MESSAGE), msg, wParam, lParam);
				return 0;
			}
			break;

		case WM_KEYDOWN:
			if (GetKeyState(VK_CONTROL) & 0x8000) {
				if (wParam == VK_TAB) {	// ctrl-(shift) tab
					if (GetKeyState(VK_SHIFT) & 0x8000) {
						SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATEPREV, 0, (LPARAM)GetParent(hwnd));
						return 0;
					} else {
						SendMessage(GetParent(GetParent(hwnd)), CM_ACTIVATENEXT, 0, (LPARAM)GetParent(hwnd));
						return 0;
					}
				}
			}
			break;

	}*/
	return CallWindowProc(OldLogEditProc, hwnd, msg, wParam, lParam);
}

                                                  //todo: decide if this should be set or not
static LRESULT CALLBACK MessageEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int result = -1;
	struct MsgEditSubclassData *dat;
	struct MessageWindowData *pdat;
	CommonWindowData *windowData;
	BOOL isShift = GetKeyState(VK_SHIFT) & 0x8000;
	BOOL isCtrl = GetKeyState(VK_CONTROL) & 0x8000;
	BOOL isAlt = GetKeyState(VK_MENU) & 0x8000;
	dat = (struct MsgEditSubclassData *) GetWindowLong(hwnd, GWL_USERDATA);
	pdat=(struct MessageWindowData *)GetWindowLong(GetParent(hwnd),GWL_USERDATA);
	windowData = &pdat->windowData;

	result = InputAreaShortcuts(hwnd, msg, wParam, lParam, windowData);
	if (result != -1) {
		return result;
	}

	switch (msg) {
	case EM_SUBCLASSED:
		dat = (struct MsgEditSubclassData *) mir_alloc(sizeof(struct MsgEditSubclassData));
		SetWindowLong(hwnd, GWL_USERDATA, (LONG) dat);
		dat->lastEnterTime = 0;
		return 0;

	case WM_KEYDOWN:
		{
			if (wParam == VK_RETURN) {
				if (isCtrl && isShift) {
					PostMessage(GetParent(hwnd), WM_COMMAND, IDC_SENDALL, 0);
					return 0;
				}
				if ((isCtrl != 0) ^ (0 != (g_dat->flags & SMF_SENDONENTER))) {
					PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
					return 0;
				}
				if (g_dat->flags & SMF_SENDONDBLENTER) {
					if (dat->lastEnterTime + ENTERCLICKTIME < GetTickCount())
						dat->lastEnterTime = GetTickCount();
					else {
						SendMessage(hwnd, WM_KEYDOWN, VK_BACK, 0);
						SendMessage(hwnd, WM_KEYUP, VK_BACK, 0);
						PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
						return 0;
					}
				}
			}
			else
				dat->lastEnterTime = 0;

		}
		break;
		//fall through
	case WM_MOUSEWHEEL:
		if ((GetWindowLong(hwnd, GWL_STYLE) & WS_VSCROLL) == 0) {
			SendMessage(GetDlgItem(GetParent(hwnd), IDC_LOG), WM_MOUSEWHEEL, wParam, lParam);
		}
		break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_KILLFOCUS:
		dat->lastEnterTime = 0;
		break;
	case WM_SYSCHAR:
		dat->lastEnterTime = 0;
		if ((wParam == 's' || wParam == 'S') && isAlt) {
			PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
			return 0;
		}
		break;
	case WM_DROPFILES:
		SendMessage(GetParent(hwnd), WM_DROPFILES, wParam, lParam);
		return 0;
	case WM_CONTEXTMENU:
		InputAreaContextMenu(hwnd, wParam, lParam, pdat->windowData.hContact);
		return TRUE;
	case EM_UNSUBCLASSED:
		mir_free(dat);
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

static void SubclassMessageEdit(HWND hwnd) {
	OldMessageEditProc = (WNDPROC) SetWindowLong(hwnd, GWL_WNDPROC, (LONG) MessageEditSubclassProc);
	SendMessage(hwnd, EM_SUBCLASSED, 0, 0);
}

static void UnsubclassMessageEdit(HWND hwnd) {
	SendMessage(hwnd, EM_UNSUBCLASSED, 0, 0);
	SetWindowLong(hwnd, GWL_WNDPROC, (LONG) OldMessageEditProc);
}

static void SubclassLogEdit(HWND hwnd) {
	OldLogEditProc = (WNDPROC) SetWindowLong(hwnd, GWL_WNDPROC, (LONG) LogEditSubclassProc);
	SendMessage(hwnd, EM_SUBCLASSED, 0, 0);
}

static void UnsubclassLogEdit(HWND hwnd) {
	SendMessage(hwnd, EM_UNSUBCLASSED, 0, 0);
	SetWindowLong(hwnd, GWL_WNDPROC, (LONG) OldLogEditProc);
}

static void MessageDialogResize(HWND hwndDlg, struct MessageWindowData *dat, int w, int h) {
	HDWP hdwp;
	ParentWindowData *pdat = dat->parent;
	int i, lPos, rPos, vPos, aPos;
	int vSplitterPos = 0, hSplitterPos = dat->splitterPos, toolbarHeight = pdat->flags2&SMF2_SHOWTOOLBAR ? dat->toolbarSize.cy : 0;
	int splitterHeight = 2;
	int hSplitterMinTop = toolbarHeight + dat->minLogBoxHeight, hSplitterMinBottom = dat->minEditBoxHeight;

	if (h-hSplitterPos < hSplitterMinTop) {
		hSplitterPos = h - hSplitterMinTop;
	}
	if (hSplitterPos < hSplitterMinBottom) {
		hSplitterPos = hSplitterMinBottom;
	}
	dat->splitterPos = hSplitterPos;
	SendMessage(hwndDlg, DM_AVATARCALCSIZE, 0, 0);
	if (hSplitterPos + toolbarHeight < dat->avatarHeight) {
		hSplitterPos = dat->avatarHeight - toolbarHeight;
	}
	dat->splitterPos = hSplitterPos;
	hdwp = BeginDeferWindowPos(12);
	hdwp = DeferWindowPos(hdwp, GetDlgItem(hwndDlg, IDC_LOG), 0, 0, 0, w-vSplitterPos, h-hSplitterPos-toolbarHeight, SWP_NOZORDER);
	hdwp = DeferWindowPos(hdwp, GetDlgItem(hwndDlg, IDC_MESSAGE), 0, 0, h - hSplitterPos + splitterHeight, w-(dat->avatarWidth ? dat->avatarWidth+1 : 0), hSplitterPos - splitterHeight, SWP_NOZORDER);
	/* make the splitter a little bit bigger */
	hdwp = DeferWindowPos(hdwp, GetDlgItem(hwndDlg, IDC_SPLITTER), 0, 0, h - hSplitterPos-1, w-dat->avatarWidth, splitterHeight + 1, SWP_NOZORDER);
	lPos = 0;
	if (dat->avatarHeight + 1 < hSplitterPos) {
		rPos = w;
		aPos = h - (hSplitterPos + dat->avatarHeight - 1) / 2;
	} else {
		rPos = w - dat->avatarWidth;
		aPos = h - (hSplitterPos + toolbarHeight + dat->avatarHeight + 1) / 2;
	}
	vPos = h - hSplitterPos - toolbarHeight + 1;
	for (i = 0; i < sizeof(buttonLineControls) / sizeof(buttonLineControls[0]); i++) {
		if (!buttonAlignment[i] && (g_dat->buttonVisibility & (1 << i))) {
			lPos += buttonSpacing[i];
			hdwp = DeferWindowPos(hdwp, GetDlgItem(hwndDlg, buttonLineControls[i]), 0, lPos, vPos, buttonWidth[i], toolbarHeight - splitterHeight, SWP_NOZORDER);
			lPos += buttonWidth[i];
		}
	}
	for (i = sizeof(buttonLineControls) / sizeof(buttonLineControls[0]) - 1; i >=0; i--) {
		if (buttonAlignment[i] && (g_dat->buttonVisibility & (1 << i))) {
			rPos -= buttonSpacing[i] + buttonWidth[i];
			hdwp = DeferWindowPos(hdwp, GetDlgItem(hwndDlg, buttonLineControls[i]), 0, rPos, vPos, buttonWidth[i], toolbarHeight - splitterHeight, SWP_NOZORDER);
		}
	}
	hdwp = DeferWindowPos(hdwp, GetDlgItem(hwndDlg, IDC_AVATAR), 0, w-dat->avatarWidth, aPos, dat->avatarWidth, dat->avatarHeight, SWP_NOZORDER);
//	hdwp = DeferWindowPos(hdwp, GetDlgItem(hwndDlg, IDC_AVATAR), 0, w-dat->avatarWidth, h - (hSplitterPos + toolbarHeight + dat->avatarHeight)/2, dat->avatarWidth, dat->avatarHeight, SWP_NOZORDER);
	EndDeferWindowPos(hdwp);
	if (dat->hwndLog != NULL) {
		IEVIEWWINDOW ieWindow;
		ieWindow.cbSize = sizeof(IEVIEWWINDOW);
		ieWindow.iType = IEW_SETPOS;
		ieWindow.parent = hwndDlg;
		ieWindow.hwnd = dat->hwndLog;
        ieWindow.x = 0;
        ieWindow.y = 0;
        ieWindow.cx = w-vSplitterPos;
        ieWindow.cy = h-hSplitterPos - toolbarHeight - 1;
		CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
	} else {
		RedrawWindow(GetDlgItem(hwndDlg, IDC_LOG), NULL, NULL, RDW_INVALIDATE);
	}
	RedrawWindow(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, NULL, RDW_INVALIDATE);
	if (g_dat->flags&SMF_AVATAR && IsWindowVisible(GetDlgItem(hwndDlg, IDC_AVATAR))) {
		RedrawWindow(GetDlgItem(hwndDlg, IDC_AVATAR), NULL, NULL, RDW_INVALIDATE);
	}
}

static void UpdateReadChars(HWND hwndDlg, struct MessageWindowData * dat)
{
	if (dat->parent->hwndActive == hwndDlg) {
		TCHAR szText[256];
		StatusBarData sbd;
		int len = GetRichTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE), dat->windowData.codePage, FALSE);
		sbd.iItem = 1;
		sbd.iFlags = SBDF_TEXT | SBDF_ICON;
		sbd.hIcon = NULL;
		sbd.pszText = szText;
		mir_sntprintf(szText, SIZEOF(szText), _T("%d"), len);
		SendMessage(dat->hwndParent, CM_UPDATESTATUSBAR, (WPARAM)&sbd, (LPARAM)hwndDlg);
	}
}

void ShowAvatar(HWND hwndDlg, struct MessageWindowData *dat) {
	DBVARIANT dbv;

	if (g_dat->avatarServiceExists) {
		if (dat->ace != NULL) {
			dat->avatarPic = (dat->ace->dwFlags & AVS_HIDEONCLIST) ? NULL : dat->ace->hbmPic;
		} else {
			dat->avatarPic = NULL;
		}
	} else {
		if (dat->avatarPic)  {
			DeleteObject(dat->avatarPic);
			dat->avatarPic=0;
		}
		if (!DBGetContactSettingString(dat->windowData.hContact, SRMMMOD, SRMSGSET_AVATAR, &dbv)) {
			HANDLE hFile;
			char tmpPath[MAX_PATH];
			/* relative to absolute here */
			strcpy(tmpPath, dbv.pszVal);
			if (ServiceExists(MS_UTILS_PATHTOABSOLUTE)) {
				CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)tmpPath);
			}
			DBFreeVariant(&dbv);
			if((hFile = CreateFileA(tmpPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {
				dat->avatarPic=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)tmpPath);
				CloseHandle(hFile);
			}
		}
	}
	SendMessage(hwndDlg, DM_AVATARCALCSIZE, 0, 0);
	SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 0);
	SendMessage(hwndDlg, WM_SIZE, 0, 0);
}

static BOOL IsTypingNotificationSupported(struct MessageWindowData *dat) {
	DWORD typeCaps;
	if (!dat->windowData.hContact)
		return FALSE;
	if (!dat->szProto)
		return FALSE;
	typeCaps = CallProtoService(dat->szProto, PS_GETCAPS, PFLAGNUM_4, 0);
	if (!(typeCaps & PF4_SUPPORTTYPING))
		return FALSE;
	return TRUE;
}

static BOOL IsTypingNotificationEnabled(struct MessageWindowData *dat) {
	DWORD protoStatus;
	DWORD protoCaps;
	if (!DBGetContactSettingByte(dat->windowData.hContact, SRMMMOD, SRMSGSET_TYPING, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW)))
		return FALSE;
	protoStatus = CallProtoService(dat->szProto, PS_GETSTATUS, 0, 0);
	if (protoStatus < ID_STATUS_ONLINE)
		return FALSE;
	protoCaps = CallProtoService(dat->szProto, PS_GETCAPS, PFLAGNUM_1, 0);
	if (protoCaps & PF1_VISLIST && DBGetContactSettingWord(dat->windowData.hContact, dat->szProto, "ApparentMode", 0) == ID_STATUS_OFFLINE)
		return FALSE;
	if (protoCaps & PF1_INVISLIST && protoStatus == ID_STATUS_INVISIBLE && DBGetContactSettingWord(dat->windowData.hContact, dat->szProto, "ApparentMode", 0) != ID_STATUS_ONLINE)
		return FALSE;
	if (DBGetContactSettingByte(dat->windowData.hContact, "CList", "NotOnList", 0)
		&& !DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_TYPINGUNKNOWN, SRMSGDEFSET_TYPINGUNKNOWN))
		return FALSE;
	return TRUE;
}

// Don't send to protocols who don't support typing
// Don't send to users who are unchecked in the typing notification options
// Don't send to protocols that are offline
// Don't send to users who are not visible and
// Don't send to users who are not on the visible list when you are in invisible mode.
static void NotifyTyping(struct MessageWindowData *dat, int mode) {
	if (!IsTypingNotificationSupported(dat)) {
		return;
	}
	if (!IsTypingNotificationEnabled(dat)) {
		return;
	}
	// End user check
	dat->nTypeMode = mode;
	CallService(MS_PROTO_SELFISTYPING, (WPARAM) dat->windowData.hContact, dat->nTypeMode);
}


static int MeasureMenuItem(WPARAM wParam, LPARAM lParam)
{
	LPMEASUREITEMSTRUCT mis = (LPMEASUREITEMSTRUCT) lParam;
	if (mis->itemData != 0xDEAD) {
		return FALSE;
	}
	mis->itemWidth = max(0, GetSystemMetrics(SM_CXSMICON) - GetSystemMetrics(SM_CXMENUCHECK) + 4);
	mis->itemHeight = GetSystemMetrics(SM_CYSMICON) + 2;
	return TRUE;
}


static int DrawMenuItem(WPARAM wParam, LPARAM lParam)
{
	int y;
	LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;
	if (dis->itemData != 0xDEAD) {
		return FALSE;
	}
	y = (dis->rcItem.bottom - dis->rcItem.top - GetSystemMetrics(SM_CYSMICON)) / 2 + 1;
	if (dis->itemState & ODS_SELECTED) {
		if (dis->itemState & ODS_CHECKED) {
			RECT rc;
			rc.left = 2;
			rc.right = GetSystemMetrics(SM_CXSMICON) + 2;
			rc.top = y;
			rc.bottom = rc.top + GetSystemMetrics(SM_CYSMICON) + 2;
			FillRect(dis->hDC, &rc, GetSysColorBrush(COLOR_HIGHLIGHT));
			ImageList_DrawEx(g_dat->hButtonIconList, dis->itemID, dis->hDC, 2, y, 0, 0, CLR_NONE, CLR_DEFAULT, ILD_SELECTED);
		} else
			ImageList_DrawEx(g_dat->hButtonIconList, dis->itemID, dis->hDC, 2, y, 0, 0, CLR_NONE, CLR_DEFAULT, ILD_FOCUS);
	} else {
		if (dis->itemState & ODS_CHECKED) {
			HBRUSH hBrush;
			RECT rc;
			COLORREF menuCol, hiliteCol;
			rc.left = 0;
			rc.right = GetSystemMetrics(SM_CXSMICON) + 4;
			rc.top = y - 2;
			rc.bottom = rc.top + GetSystemMetrics(SM_CYSMICON) + 4;
			DrawEdge(dis->hDC, &rc, BDR_SUNKENOUTER, BF_RECT);
			InflateRect(&rc, -1, -1);
			menuCol = GetSysColor(COLOR_MENU);
			hiliteCol = GetSysColor(COLOR_3DHIGHLIGHT);
			hBrush = CreateSolidBrush(RGB
				((GetRValue(menuCol) + GetRValue(hiliteCol)) / 2, (GetGValue(menuCol) + GetGValue(hiliteCol)) / 2,
				(GetBValue(menuCol) + GetBValue(hiliteCol)) / 2));
			FillRect(dis->hDC, &rc, hBrush);
			DeleteObject(hBrush);
			ImageList_DrawEx(g_dat->hButtonIconList, dis->itemID, dis->hDC, 2, y, 0, 0, CLR_NONE, GetSysColor(COLOR_MENU), ILD_BLEND25);
		} else
			ImageList_DrawEx(g_dat->hButtonIconList, dis->itemID, dis->hDC, 2, y, 0, 0, CLR_NONE, CLR_NONE, ILD_NORMAL);
	}
	return TRUE;
}

static BOOL CALLBACK ConfirmSendAllDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
			RECT rcParent, rcChild;
			TranslateDialogDefault( hwndDlg );
			GetWindowRect(GetParent(hwndDlg), &rcParent);
			GetWindowRect(hwndDlg, &rcChild);
			rcChild.bottom -= rcChild.top;
			rcChild.right -= rcChild.left;
			rcParent.bottom -= rcParent.top;
			rcParent.right -= rcParent.left;
			rcChild.left = rcParent.left + (rcParent.right - rcChild.right) / 2;
			rcChild.top = rcParent.top + (rcParent.bottom - rcChild.bottom) / 2;
			MoveWindow(hwndDlg, rcChild.left, rcChild.top, rcChild.right, rcChild.bottom, FALSE);
		}
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDYES:
		case IDNO:
		case IDCANCEL:
			{
				int result = LOWORD(wParam);
				if (IsDlgButtonChecked(hwndDlg, IDC_REMEMBER)) {
					result |= 0x10000;
				}
				EndDialog(hwndDlg, result);
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}


BOOL CALLBACK DlgProcMessage(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HMENU hToolbarMenu;
	struct MessageWindowData *dat;
	dat = (struct MessageWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);
	if (!dat && msg!=WM_INITDIALOG) return FALSE;
	switch (msg) {
	case WM_INITDIALOG:
		{
			int len = 0;
			int notifyUnread = 0;
			NewMessageWindowLParam *newData = (NewMessageWindowLParam *) lParam;
			//TranslateDialogDefault(hwndDlg);
			dat = (struct MessageWindowData *) mir_alloc(sizeof(struct MessageWindowData));
			ZeroMemory(dat, sizeof(struct MessageWindowData));
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) dat);
			dat->windowData.hContact = newData->hContact;
			NotifyLocalWinEvent(dat->windowData.hContact, hwndDlg, MSG_WINDOW_EVT_OPENING);
//			SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETTEXTMODE, TM_PLAINTEXT, 0);

			dat->hwnd = hwndDlg;
			dat->hwndParent = GetParent(hwndDlg);
			dat->parent = (ParentWindowData *) GetWindowLong(dat->hwndParent, GWL_USERDATA);
			dat->hwndLog = NULL;
			dat->szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) dat->windowData.hContact, 0);
			dat->avatarPic = 0;
			if (dat->windowData.hContact && dat->szProto != NULL)
				dat->wStatus = DBGetContactSettingWord(dat->windowData.hContact, dat->szProto, "Status", ID_STATUS_OFFLINE);
			else
				dat->wStatus = ID_STATUS_OFFLINE;
			dat->wOldStatus = dat->wStatus;
			dat->hDbEventFirst = NULL;
			dat->hDbEventLast = NULL;
			dat->hDbUnreadEventFirst = NULL;
			dat->messagesInProgress = 0;
			dat->nTypeSecs = 0;
			dat->nLastTyping = 0;
			dat->showTyping = 0;
			dat->showUnread = 0;
			dat->sendAllConfirm = 0;
			dat->nTypeMode = PROTOTYPE_SELFTYPING_OFF;
			SetTimer(hwndDlg, TIMERID_TYPE, 1000, NULL);
			dat->lastMessage = 0;
			dat->lastEventType = -1;
			dat->lastEventTime = time(NULL);
			dat->startTime = time(NULL);
			dat->flags = 0;
			if (DBGetContactSettingByte(dat->windowData.hContact, SRMMMOD, "UseRTL", (BYTE) 0)) {
				dat->flags |= SMF_RTL;
			}
			if (DBGetContactSettingByte(dat->windowData.hContact, SRMMMOD, "DisableUnicode", (BYTE) 0)) {
				dat->flags |= SMF_DISABLE_UNICODE;
			}
			dat->flags |= ServiceExists(MS_IEVIEW_WINDOW) ? g_dat->flags & SMF_USEIEVIEW : 0;
			{
				PARAFORMAT2 pf2;
				ZeroMemory((void *)&pf2, sizeof(pf2));
				pf2.cbSize = sizeof(pf2);
				pf2.dwMask = PFM_RTLPARA;
				if (!(dat->flags & SMF_RTL)) {
					pf2.wEffects = 0;
					SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE) & ~(WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR));
				} else {
					pf2.wEffects = PFE_RTLPARA;
					SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE) | WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
				}
				SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
				/* Workaround to make Richedit display RTL messages correctly */
				ZeroMemory((void *)&pf2, sizeof(pf2));
				pf2.cbSize = sizeof(pf2);
				pf2.dwMask = PFM_RTLPARA | PFM_OFFSETINDENT | PFM_RIGHTINDENT;
				pf2.wEffects = PFE_RTLPARA;
				pf2.dxStartIndent = 30;
				pf2.dxRightIndent = 30;
				SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
				pf2.dwMask = PFM_RTLPARA;
				pf2.wEffects = 0;
				SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
				if (dat->flags & SMF_RTL) {
					SetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE) | WS_EX_LEFTSCROLLBAR);
				} else {
					SetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE) & ~WS_EX_LEFTSCROLLBAR);
				}
			}
			dat->windowData.codePage = DBGetContactSettingWord(dat->windowData.hContact, SRMMMOD, "CodePage", (WORD) CP_ACP);
			dat->ace = NULL;
			GetWindowRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &dat->minEditInit);
			dat->minEditBoxHeight = dat->minEditInit.bottom - dat->minEditInit.top;
			dat->minLogBoxHeight = dat->minEditBoxHeight;
			dat->splitterPos = (int) DBGetContactSettingDword((g_dat->flags & SMF_SAVESPLITTERPERCONTACT) ? dat->windowData.hContact : NULL, SRMMMOD, "splitterPos", (DWORD) - 1);
			dat->toolbarSize.cy = DBGetContactSettingDword((g_dat->flags & SMF_SAVESPLITTERPERCONTACT) ? dat->windowData.hContact : NULL, SRMMMOD, "splitterHeight", (DWORD) 24);
			dat->toolbarSize.cy = max(min(dat->toolbarSize.cy, 24), 18);
			dat->toolbarSize.cx = GetToolbarWidth();
			if (dat->splitterPos == -1) {
				dat->splitterPos = dat->minEditBoxHeight;
			}
			WindowList_Add(g_dat->hMessageWindowList, hwndDlg, dat->windowData.hContact);

			if (newData->szInitialText) {
	#if defined(_UNICODE)
				if(newData->isWchar)
					SetDlgItemText(hwndDlg, IDC_MESSAGE, (TCHAR *)newData->szInitialText);
				else
					SetDlgItemTextA(hwndDlg, IDC_MESSAGE, newData->szInitialText);
	#else
				SetDlgItemTextA(hwndDlg, IDC_MESSAGE, newData->szInitialText);
	#endif
			} else if (g_dat->flags & SMF_SAVEDRAFTS) {
				TCmdList *draft = tcmdlist_get2(g_dat->draftList, dat->windowData.hContact);
				if (draft != NULL) {
					len = SetRichTextEncoded(GetDlgItem(hwndDlg, IDC_MESSAGE), draft->szCmd, dat->windowData.codePage);
				}
				PostMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_SETSEL, len, len);
			}

			SendMessage(hwndDlg, DM_CHANGEICONS, 0, 0);
			// Make them flat buttons
			{
				int i;
				for (i = 0; i < sizeof(buttonLineControls) / sizeof(buttonLineControls[0]); i++)
					SendMessage(GetDlgItem(hwndDlg, buttonLineControls[i]), BUTTONSETASFLATBTN, 0, 0);
			}
			SendMessage(GetDlgItem(hwndDlg, IDC_ADD), BUTTONADDTOOLTIP, (WPARAM) Translate("Add Contact Permanently to List"), 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_USERMENU), BUTTONADDTOOLTIP, (WPARAM) Translate("User Menu"), 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_DETAILS), BUTTONADDTOOLTIP, (WPARAM) Translate("View User's Details"), 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_HISTORY), BUTTONADDTOOLTIP, (WPARAM) Translate("View User's History"), 0);

			SendMessage(GetDlgItem(hwndDlg, IDC_QUOTE), BUTTONADDTOOLTIP, (WPARAM) Translate("Quote Text"), 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_SMILEYS), BUTTONADDTOOLTIP, (WPARAM) Translate("Insert Emoticon"), 0);
			SendMessage(GetDlgItem(hwndDlg, IDOK), BUTTONADDTOOLTIP, (WPARAM) Translate("Send Message"), 0);
			SendMessage(GetDlgItem(hwndDlg, IDCANCEL), BUTTONADDTOOLTIP, (WPARAM) Translate("Close Session"), 0);

			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETOLECALLBACK, 0, (LPARAM) & reOleCallback);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_LINK | ENM_KEYEVENTS);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETEDITSTYLE, SES_EXTENDBACKCOLOR, SES_EXTENDBACKCOLOR);
			SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETLANGOPTIONS, 0, (LPARAM) SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETLANGOPTIONS, 0, 0) & ~IMF_AUTOKEYBOARD);
			SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETOLECALLBACK, 0, (LPARAM) & reOleCallback2);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETLANGOPTIONS, 0, (LPARAM) SendDlgItemMessage(hwndDlg, IDC_LOG, EM_GETLANGOPTIONS, 0, 0) & ~(IMF_AUTOKEYBOARD | IMF_AUTOFONTSIZEADJUST));
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0,0));
			SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_KEYEVENTS | ENM_CHANGE);
			if (dat->flags & SMF_USEIEVIEW) {
				IEVIEWWINDOW ieWindow;
				ieWindow.cbSize = sizeof(IEVIEWWINDOW);
				ieWindow.iType = IEW_CREATE;
				ieWindow.dwFlags = 0;
				ieWindow.dwMode = IEWM_SCRIVER;
				ieWindow.parent = hwndDlg;
				ieWindow.x = 0;
				ieWindow.y = 0;
				ieWindow.cx = 200;
				ieWindow.cy = 300;
				CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
				dat->hwndLog = ieWindow.hwnd;
				if (dat->hwndLog == NULL) {
					dat->flags ^= SMF_USEIEVIEW;
				}
			}
			/* duh, how come we didnt use this from the start? */
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_AUTOURLDETECT, (WPARAM) TRUE, 0);
			if (dat->windowData.hContact) {
				if (dat->szProto) {
					int nMax;
					nMax = CallProtoService(dat->szProto, PS_GETCAPS, PFLAG_MAXLENOFMESSAGE, (LPARAM) dat->windowData.hContact);
					if (nMax)
						SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_LIMITTEXT, (WPARAM) nMax, 0);
				}
			}
			/* get around a lame bug in the Windows template resource code where richedits are limited to 0x7FFF */
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_LIMITTEXT, (WPARAM) sizeof(TCHAR) * 0x7FFFFFFF, 0);
			SubclassLogEdit(GetDlgItem(hwndDlg, IDC_LOG));
			SubclassMessageEdit(GetDlgItem(hwndDlg, IDC_MESSAGE));
			OldSplitterProc = (WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_WNDPROC, (LONG) SplitterSubclassProc);
			if (dat->windowData.hContact) {
				int historyMode = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_LOADHISTORY, SRMSGDEFSET_LOADHISTORY);
				// This finds the first message to display, it works like shit
				dat->hDbEventFirst = (HANDLE) CallService(MS_DB_EVENT_FINDFIRSTUNREAD, (WPARAM) dat->windowData.hContact, 0);
				if (dat->hDbEventFirst != NULL) {
					DBEVENTINFO dbei = { 0 };
					dbei.cbSize = sizeof(dbei);
					CallService(MS_DB_EVENT_GET, (WPARAM) dat->hDbEventFirst, (LPARAM) & dbei);
					if (dbei.eventType == EVENTTYPE_MESSAGE && !(dbei.flags & DBEF_READ) && !(dbei.flags & DBEF_SENT)) {
						notifyUnread = 1;
					}
				}
				switch (historyMode) {
				case LOADHISTORY_COUNT:
					{
						int i;
						HANDLE hPrevEvent;
						DBEVENTINFO dbei = { 0 };
						dbei.cbSize = sizeof(dbei);
						for (i = DBGetContactSettingWord(NULL, SRMMMOD, SRMSGSET_LOADCOUNT, SRMSGDEFSET_LOADCOUNT); i > 0; i--) {
							if (dat->hDbEventFirst == NULL)
								hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDLAST, (WPARAM) dat->windowData.hContact, 0);
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
						if (dat->hDbEventFirst == NULL) {
							dbei.timestamp = time(NULL);
							hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDLAST, (WPARAM) dat->windowData.hContact, 0);
						} else {
							CallService(MS_DB_EVENT_GET, (WPARAM) dat->hDbEventFirst, (LPARAM) & dbei);
							hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDPREV, (WPARAM) dat->hDbEventFirst, 0);
						}
						firstTime = dbei.timestamp - 60 * DBGetContactSettingWord(NULL, SRMMMOD, SRMSGSET_LOADTIME, SRMSGDEFSET_LOADTIME);
						for (;;) {
							if (hPrevEvent == NULL)
								break;
							dbei.cbBlob = 0;
							CallService(MS_DB_EVENT_GET, (WPARAM) hPrevEvent, (LPARAM) & dbei);
							if (dbei.timestamp < firstTime)
								break;
							if (DbEventIsShown(&dbei, dat))
								dat->hDbEventFirst = hPrevEvent;
							hPrevEvent = (HANDLE) CallService(MS_DB_EVENT_FINDPREV, (WPARAM) hPrevEvent, 0);
						}
						break;
					}
				}
			}
			SendMessage(dat->hwndParent, CM_ADDCHILD, (WPARAM) hwndDlg, (LPARAM) dat->windowData.hContact);
			{
				DBEVENTINFO dbei = { 0 };
				HANDLE hdbEvent;

				dbei.cbSize = sizeof(dbei);
				hdbEvent = (HANDLE) CallService(MS_DB_EVENT_FINDLAST, (WPARAM) dat->windowData.hContact, 0);
				if (hdbEvent) {
					do {
						ZeroMemory(&dbei, sizeof(dbei));
						dbei.cbSize = sizeof(dbei);
						CallService(MS_DB_EVENT_GET, (WPARAM) hdbEvent, (LPARAM) & dbei);
						if (dbei.eventType == EVENTTYPE_MESSAGE && !(dbei.flags & DBEF_SENT)) {
							dat->lastMessage = dbei.timestamp;
							break;
						}
					}
					while ((hdbEvent = (HANDLE) CallService(MS_DB_EVENT_FINDPREV, (WPARAM) hdbEvent, 0)));
				}
			}
			SendMessage(hwndDlg, DM_OPTIONSAPPLIED, 0, 0);
			SendMessage(GetParent(hwndDlg), CM_POPUPWINDOW, (WPARAM) (newData->flags & NMWLP_INCOMING), (LPARAM) hwndDlg);
			if (notifyUnread) {
				if (GetForegroundWindow() != dat->hwndParent || dat->parent->hwndActive != hwndDlg) {
					dat->showUnread = 1;
					SendMessage(hwndDlg, DM_UPDATEICON, 0, 1);
					SetTimer(hwndDlg, TIMERID_UNREAD, TIMEOUT_UNREAD, NULL);
				}
				SendMessage(dat->hwndParent, CM_STARTFLASHING, 0, 0);
			}
			dat->messagesInProgress = ReattachSendQueueItems(hwndDlg, dat->windowData.hContact);
			if (dat->messagesInProgress > 0) {
				SendMessage(hwndDlg, DM_SHOWMESSAGESENDING, 0, 0);
			}
			NotifyLocalWinEvent(dat->windowData.hContact, hwndDlg, MSG_WINDOW_EVT_OPEN);
			return TRUE;
		}
	case DM_GETCONTEXTMENU:
		{
			HMENU hMenu = (HMENU) CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM) dat->windowData.hContact, 0);
			SetWindowLong(hwndDlg, DWL_MSGRESULT, (LONG)hMenu);
			return TRUE;
		}
	case WM_CONTEXTMENU:
		if (dat->hwndParent == (HWND) wParam) {
			POINT pt;
			HMENU hMenu = (HMENU) CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM) dat->windowData.hContact, 0);
			GetCursorPos(&pt);
			TrackPopupMenu(hMenu, 0, pt.x, pt.y, 0, hwndDlg, NULL);
			DestroyMenu(hMenu);
		}
		break;
	case WM_LBUTTONDBLCLK:
		SendMessage(dat->hwndParent, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		break;
	case WM_RBUTTONUP:
		{
			int i;
			POINT pt;
			MENUITEMINFO mii;
			hToolbarMenu = CreatePopupMenu();
			for (i = 0; i < sizeof(buttonLineControls) / sizeof(buttonLineControls[0]); i++) {
				ZeroMemory(&mii, sizeof(mii));
				mii.cbSize = sizeof(mii);
				mii.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE | MIIM_DATA | MIIM_BITMAP;
				mii.fType = MFT_STRING;
				mii.fState = (g_dat->buttonVisibility & (1<< i)) ? MFS_CHECKED : MFS_UNCHECKED;
				mii.wID = i + 1;
				mii.dwItemData = 0xDEAD;
				mii.hbmpItem = HBMMENU_CALLBACK;
				mii.dwTypeData = TranslateTS((buttonNames[i]));
				InsertMenuItem(hToolbarMenu, i, TRUE, &mii);
			}
//			CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hToolbarMenu, 0);
			pt.x = (short) LOWORD(GetMessagePos());
			pt.y = (short) HIWORD(GetMessagePos());
			i = TrackPopupMenu(hToolbarMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
			if (i > 0) {
				g_dat->buttonVisibility ^= (1 << (i - 1));
				DBWriteContactSettingDword(NULL, SRMMMOD, SRMSGSET_BUTTONVISIBILITY, g_dat->buttonVisibility);
				WindowList_Broadcast(g_dat->hMessageWindowList, DM_OPTIONSAPPLIED, 0, 0);
			}
			DestroyMenu(hToolbarMenu);
			return TRUE;
		}
	case WM_DROPFILES:
	{
		if (dat->szProto==NULL) break;
		if (!(CallProtoService(dat->szProto, PS_GETCAPS, PFLAGNUM_1,0)&PF1_FILESEND)) break;
		if (dat->wStatus==ID_STATUS_OFFLINE) break;
		if (dat->windowData.hContact!=NULL) {
			TCHAR szFilename[MAX_PATH];
			HDROP hDrop = (HDROP)wParam;
			int fileCount = DragQueryFile(hDrop,-1,NULL,0), totalCount = 0, i;
			char** ppFiles = NULL;
			for ( i=0; i < fileCount; i++ ) {
				DragQueryFile(hDrop, i, szFilename, SIZEOF(szFilename));
				AddToFileList(&ppFiles, &totalCount, szFilename);
			}
			CallServiceSync(MS_FILE_SENDSPECIFICFILES, (WPARAM)dat->windowData.hContact, (LPARAM)ppFiles);
			for(i=0;ppFiles[i];i++) mir_free(ppFiles[i]);
			mir_free(ppFiles);
		}
		break;
	}
	case HM_AVATARACK:
	{
		ACKDATA *pAck = (ACKDATA *)lParam;
		PROTO_AVATAR_INFORMATION *pai = (PROTO_AVATAR_INFORMATION *)pAck->hProcess;
		if (pAck->hContact!=dat->windowData.hContact)
			return 0;
		if (pAck->type != ACKTYPE_AVATAR)
			return 0;
		if (pAck->result == ACKRESULT_STATUS) {
			SendMessage(hwndDlg, DM_GETAVATAR, 0, 0);
			return 0;
		}
		if (pai==NULL)
			return 0;
		if (pAck->result == ACKRESULT_SUCCESS) {
			if (pai->filename&&strlen(pai->filename)&&VALID_AVATAR(pai->format)) {
				DBWriteContactSettingString(dat->windowData.hContact, SRMMMOD, SRMSGSET_AVATAR, pai->filename);
				ShowAvatar(hwndDlg, dat);
			}
		} else if (pAck->result == ACKRESULT_FAILED) {
			DBDeleteContactSetting(dat->windowData.hContact, SRMMMOD, SRMSGSET_AVATAR);
			SendMessage(hwndDlg, DM_GETAVATAR, 0, 0);
		}
		break;
	}
	case DM_CHANGEICONS:
		SendDlgItemMessage(hwndDlg, IDC_ADD, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_dat->hIcons[SMF_ICON_ADD]);
		SendDlgItemMessage(hwndDlg, IDC_DETAILS, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_dat->hIcons[SMF_ICON_USERDETAILS]);
		SendDlgItemMessage(hwndDlg, IDC_HISTORY, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_dat->hIcons[SMF_ICON_HISTORY]);
		SendDlgItemMessage(hwndDlg, IDC_QUOTE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_dat->hIcons[SMF_ICON_QUOTE]);
		SendDlgItemMessage(hwndDlg, IDC_SMILEYS, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_dat->hIcons[SMF_ICON_SMILEY]);
		SendDlgItemMessage(hwndDlg, IDOK, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_dat->hIcons[SMF_ICON_SEND]);
		SendDlgItemMessage(hwndDlg, IDCANCEL, BM_SETIMAGE, IMAGE_ICON, (LPARAM) g_dat->hIcons[SMF_ICON_CANCEL]);
		SendMessage(hwndDlg, DM_UPDATESTATUSBAR, 0, 0);
		break;
	case DM_AVATARCALCSIZE:
	{
		BITMAP bminfo;
		ParentWindowData *pdat;
		RECT rc;
		double aspect;
		pdat = dat->parent;
		dat->avatarWidth = 0;
		dat->avatarHeight = 0;
		if (dat->avatarPic==0||!(g_dat->flags&SMF_AVATAR)) {
			ShowWindow(GetDlgItem(hwndDlg, IDC_AVATAR), SW_HIDE);
			return 0;
		}
		GetObject(dat->avatarPic, sizeof(bminfo), &bminfo);
		if ( bminfo.bmWidth == 0 || bminfo.bmHeight == 0 ) {
			ShowWindow(GetDlgItem(hwndDlg, IDC_AVATAR), SW_HIDE);
			return 0;
		}
		dat->avatarHeight = dat->splitterPos + ((pdat->flags2&SMF2_SHOWTOOLBAR) ? dat->toolbarSize.cy : 0);//- 3;
		if (g_dat->flags & SMF_LIMITAVATARH) {
			if (dat->avatarHeight < g_dat->limitAvatarMinH) {
				dat->avatarHeight = g_dat->limitAvatarMinH;
			}
			if (dat->avatarHeight > g_dat->limitAvatarMaxH) {
				dat->avatarHeight = g_dat->limitAvatarMaxH;
			}
		} else if (g_dat->flags & SMF_ORIGINALAVATARH) {
			dat->avatarHeight = bminfo.bmHeight;
		}
		aspect = (double)dat->avatarHeight / (double)bminfo.bmHeight;
		GetClientRect(hwndDlg, &rc);
		dat->avatarWidth = (int)(bminfo.bmWidth * aspect);
		// if edit box width < min then adjust avatarWidth
		if (rc.right - dat->avatarWidth < dat->toolbarSize.cx) {
			dat->avatarWidth = rc.right - dat->toolbarSize.cx;
			if (dat->avatarWidth < 0) dat->avatarWidth = 0;
			aspect = (double)dat->avatarWidth / (double)bminfo.bmWidth;
			dat->avatarHeight = (int)(bminfo.bmHeight * aspect);
		}
		if (rc.bottom - dat->avatarHeight < dat->minLogBoxHeight) {
			dat->avatarHeight = rc.bottom - dat->minLogBoxHeight;
			if (dat->avatarHeight < 0) dat->avatarHeight = 0;
			aspect = (double)dat->avatarHeight / (double)bminfo.bmHeight;
			dat->avatarWidth = (int)(bminfo.bmWidth * aspect);
		}
		ShowWindow(GetDlgItem(hwndDlg, IDC_AVATAR), SW_SHOW);
		break;
	}
	case DM_AVATARCHANGED:
		if ((HANDLE) wParam == NULL) {
			dat->ace = (struct avatarCacheEntry *)CallService(MS_AV_GETAVATARBITMAP, (WPARAM)dat->windowData.hContact, 0);
			ShowAvatar(hwndDlg, dat);
		} else if (dat->windowData.hContact == (HANDLE) wParam) {
			dat->ace = (struct avatarCacheEntry *) lParam;
			ShowAvatar(hwndDlg, dat);
		}
		break;
	case DM_GETAVATAR:
	{
		PROTO_AVATAR_INFORMATION pai;
		int result;
		//Disable avatars
        if (!(g_dat->flags&SMF_AVATAR)) {
			SendMessage(hwndDlg, DM_AVATARCALCSIZE, 0, 0);
			break;
		}
		//Use contact photo
        if (!(CallProtoService(dat->szProto, PS_GETCAPS, PFLAGNUM_4, 0)&PF4_AVATARS)) {
			DBVARIANT dbv;
			if (!DBGetContactSettingString(dat->windowData.hContact, "ContactPhoto", "File", &dbv)) {
				DBWriteContactSettingString(dat->windowData.hContact, SRMMMOD, SRMSGSET_AVATAR, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			ShowAvatar(hwndDlg, dat);
			break;
		}
		if(DBGetContactSettingWord(dat->windowData.hContact, dat->szProto, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE) {
			ShowAvatar(hwndDlg, dat);
			break;
		}
		ZeroMemory((void *)&pai, sizeof(pai));
		pai.cbSize = sizeof(pai);
		pai.hContact = dat->windowData.hContact;
		pai.format = PA_FORMAT_UNKNOWN;
		strcpy(pai.filename, "");
		result = CallProtoService(dat->szProto, PS_GETAVATARINFO, GAIF_FORCE, (LPARAM)&pai);
		if (result==GAIR_SUCCESS) {
			if (VALID_AVATAR(pai.format)) {
				DBVARIANT dbv;
				DBWriteContactSettingString(dat->windowData.hContact, SRMMMOD, SRMSGSET_AVATAR, pai.filename);
				if (DBGetContactSettingString(dat->windowData.hContact, "ContactPhoto", "File", &dbv)) {
					DBWriteContactSettingString(dat->windowData.hContact, "ContactPhoto", "File", pai.filename);
				} else {
					DBFreeVariant(&dbv);
				}
			} else DBDeleteContactSetting(dat->windowData.hContact, SRMMMOD, SRMSGSET_AVATAR);
			ShowAvatar(hwndDlg, dat);
		} else if (result==GAIR_NOAVATAR) {
			DBVARIANT dbv;
			if (!DBGetContactSettingString(dat->windowData.hContact, "ContactPhoto", "File", &dbv)) {
				DBWriteContactSettingString(dat->windowData.hContact, SRMMMOD, SRMSGSET_AVATAR, dbv.pszVal);
				DBFreeVariant(&dbv);
			} else {
				DBDeleteContactSetting(dat->windowData.hContact, SRMMMOD, SRMSGSET_AVATAR);
			}
			ShowAvatar(hwndDlg, dat);
		}
		break;
	}
	case DM_TYPING:
		{
			dat->nTypeSecs = (int) lParam > 0 ? (int) lParam : 0;
			break;
		}
	case DM_UPDATEICON:
		if (dat->szProto) {
			int wStatus;
			HICON hIcon;
			TitleBarData tbd;
			TabControlData tcd;
			char *szProto = dat->szProto;
			HANDLE hContact = dat->windowData.hContact;
			if (strcmp(dat->szProto, "MetaContacts") == 0 && DBGetContactSettingByte(NULL,"CLC","Meta",0) == 0) {
				hContact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT,(UINT)dat->windowData.hContact, 0);
				if (hContact != NULL) {
					szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,(UINT)hContact,0);
				} else {
					hContact = dat->windowData.hContact;
				}
			}
			wStatus = DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE);
			if (dat->statusIcon == NULL || wStatus != dat->wStatus || lParam != 0) {
				HICON hStatusIcon = NULL;
				dat->wStatus = wStatus;
				if (ServiceExists(MS_CLIST_GETCONTACTICON) && ServiceExists(MS_CLIST_GETICONSIMAGELIST)) {
					HIMAGELIST iml = (HIMAGELIST)CallService(MS_CLIST_GETICONSIMAGELIST,0,0);
					int index = CallService(MS_CLIST_GETCONTACTICON,(WPARAM)dat->windowData.hContact,0);
					if (iml && index>=0)
						hStatusIcon = ImageList_GetIcon(iml,index,ILD_NORMAL);
				}
				if (hStatusIcon == NULL) {
					hIcon = LoadSkinnedProtoIcon(szProto, dat->wStatus);
					hStatusIcon = CopyIcon(hIcon);
					CallService(MS_SKIN2_RELEASEICON,(WPARAM)hIcon , 0);
				}

				if (dat->statusIcon != NULL) DestroyIcon(dat->statusIcon);
				if (dat->statusIconOverlay != NULL) DestroyIcon(dat->statusIconOverlay);
				dat->statusIcon = hStatusIcon;
				{
					int index = ImageList_ReplaceIcon(g_dat->hHelperIconList, 0, hStatusIcon);
					dat->statusIconOverlay = ImageList_GetIcon(g_dat->hHelperIconList, index, ILD_TRANSPARENT|INDEXTOOVERLAYMASK(1));
				}
				SendDlgItemMessage(hwndDlg, IDC_USERMENU, BM_SETIMAGE, IMAGE_ICON, (LPARAM)dat->statusIcon);
			}
			tbd.iFlags = TBDF_ICON;
			hIcon = NULL;
			if (dat->showTyping && (g_dat->flags2&SMF2_SHOWTYPINGWIN)) {
				tbd.hIcon = g_dat->hIcons[SMF_ICON_TYPING];
			} else if ((g_dat->flags & SMF_STATUSICON) && ! (dat->showUnread && (GetActiveWindow() != dat->hwndParent || GetForegroundWindow() != dat->hwndParent))) {
				tbd.hIcon = dat->statusIcon;
			} else {
				hIcon = tbd.hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
			}
			SendMessage(dat->hwndParent, CM_UPDATETITLEBAR, (WPARAM)&tbd, (LPARAM)hwndDlg);
			CallService(MS_SKIN2_RELEASEICON,(WPARAM)hIcon, 0);
			tcd.iFlags = TCDF_ICON;
			if (dat->showTyping) {
				tcd.hIcon = g_dat->hIcons[SMF_ICON_TYPING];
			} else if (dat->showUnread & 1) {
				tcd.hIcon = dat->statusIconOverlay;
			} else {
				tcd.hIcon = dat->statusIcon;
			}
			SendMessage(dat->hwndParent, CM_UPDATETABCONTROL, (WPARAM)&tcd, (LPARAM)hwndDlg);
		}
		break;
	case DM_UPDATETITLEBAR:
		{
			DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING *) wParam;
			if (dat->windowData.hContact) {
				if (dat->szProto) {
					TitleBarData tbd;
					TabControlData tcd;
					CONTACTINFO ci;
					char buf[128];
					buf[0] = 0;
					ZeroMemory(&ci, sizeof(ci));
					ci.cbSize = sizeof(ci);
					ci.hContact = dat->windowData.hContact;
					ci.szProto = dat->szProto;
					ci.dwFlag = CNF_UNIQUEID;
					if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
						switch (ci.type) {
						case CNFT_ASCIIZ:
							mir_snprintf(buf, sizeof(buf), Translate("User Menu - %s"), ci.pszVal);
							miranda_sys_free(ci.pszVal);
							break;
						case CNFT_DWORD:
							mir_snprintf(buf, sizeof(buf), Translate("User Menu - %u"), ci.dVal);
							break;
						}
					}
					SendMessage(GetDlgItem(hwndDlg, IDC_USERMENU), BUTTONADDTOOLTIP, (WPARAM) buf, 0);
					if (!cws || (!strcmp(cws->szModule, dat->szProto) && !strcmp(cws->szSetting, "Status"))) {
						SendMessage(hwndDlg, DM_UPDATEICON, 0, 1);
					}
					// log status change
					if ((dat->wStatus != dat->wOldStatus || lParam != 0)
						&& DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWSTATUSCH, SRMSGDEFSET_SHOWSTATUSCH)) {
						DBEVENTINFO dbei;
						TCHAR buffer[512];
						char blob[2048];
						HANDLE hNewEvent;
						int iLen;
						TCHAR *szOldStatus = mir_tstrdup((TCHAR *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM) dat->wOldStatus, GCMDF_TCHAR));
						TCHAR *szNewStatus = mir_tstrdup((TCHAR *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM) dat->wStatus, GCMDF_TCHAR));

						if (dat->wStatus == ID_STATUS_OFFLINE) {
							iLen = mir_sntprintf(buffer, SIZEOF(buffer), TranslateT("signed off (was %s)"), szOldStatus);
							SendMessage(hwndDlg, DM_TYPING, 0, 0);
						}
						else if (dat->wOldStatus == ID_STATUS_OFFLINE) {
							iLen = mir_sntprintf(buffer, SIZEOF(buffer), TranslateT("signed on (%s)"), szNewStatus);
						}
						else {
							iLen = mir_sntprintf(buffer, SIZEOF(buffer), TranslateT("is now %s (was %s)"), szNewStatus, szOldStatus);
						}
					#if defined( _UNICODE )
						{
							int ansiLen = WideCharToMultiByte(CP_ACP, 0, buffer, -1, blob, sizeof(blob), 0, 0);
							memcpy( blob+ansiLen, buffer, sizeof(TCHAR)*(iLen+1));
							dbei.cbBlob = ansiLen + sizeof(TCHAR)*(iLen+1);
						}
					#else
						{
							int wLen = MultiByteToWideChar(CP_ACP, 0, buffer, -1, NULL, 0 );
							memcpy( blob, buffer, iLen+1 );
							MultiByteToWideChar(CP_ACP, 0, buffer, -1, (WCHAR*)&blob[iLen+1], wLen+1 );
							dbei.cbBlob = iLen+1 + sizeof(WCHAR)*wLen;
						}
					#endif
						//iLen = strlen(buffer) + 1;
						//MultiByteToWideChar(CP_ACP, 0, buffer, iLen, (LPWSTR) & buffer[iLen], iLen);
						dbei.cbSize = sizeof(dbei);
						dbei.pBlob = (PBYTE) blob;
					//	dbei.cbBlob = (strlen(buffer) + 1) * (sizeof(TCHAR) + 1);
						dbei.eventType = EVENTTYPE_STATUSCHANGE;
						dbei.flags = 0;
						dbei.timestamp = time(NULL);
						dbei.szModule = dat->szProto;
						hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) dat->windowData.hContact, (LPARAM) & dbei);
						if (dat->hDbEventFirst == NULL) {
							dat->hDbEventFirst = hNewEvent;
							SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
						}
						dat->wOldStatus = dat->wStatus;
						mir_free(szOldStatus);
						mir_free(szNewStatus);
					}
					tbd.iFlags = TBDF_TEXT;
					tbd.pszText = GetWindowTitle(dat->windowData.hContact, dat->szProto);
					SendMessage(dat->hwndParent, CM_UPDATETITLEBAR, (WPARAM)&tbd, (LPARAM)hwndDlg);
					mir_free(tbd.pszText);
					tcd.iFlags = TCDF_TEXT;
					tcd.pszText = GetTabName(dat->windowData.hContact);
					SendMessage(dat->hwndParent, CM_UPDATETABCONTROL, (WPARAM)&tcd, (LPARAM)hwndDlg);
					mir_free(tcd.pszText);
				}
			}

			break;
		}
	case DM_OPTIONSAPPLIED:
		{
			PARAFORMAT2 pf2 = {0};
			CHARFORMAT2 cf2 = {0};
			LOGFONT lf;
			COLORREF colour;
			dat->flags &= ~SMF_USEIEVIEW;
			dat->flags |= ServiceExists(MS_IEVIEW_WINDOW) ? g_dat->flags & SMF_USEIEVIEW : 0;
			if (dat->flags & SMF_USEIEVIEW && dat->hwndLog == NULL) {
				IEVIEWWINDOW ieWindow;
				ieWindow.cbSize = sizeof(IEVIEWWINDOW);
				ieWindow.iType = IEW_CREATE;
				ieWindow.dwFlags = 0;
				ieWindow.dwMode = IEWM_SCRIVER;
				ieWindow.parent = hwndDlg;
				ieWindow.x = 0;
				ieWindow.y = 0;
				ieWindow.cx = 200;
				ieWindow.cy = 300;
				CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
				dat->hwndLog = ieWindow.hwnd;
				if (dat->hwndLog == NULL) {
					dat->flags ^= SMF_USEIEVIEW;
				}
			} else if (!(dat->flags & SMF_USEIEVIEW) && dat->hwndLog != NULL) {
				if (dat->hwndLog != NULL) {
					IEVIEWWINDOW ieWindow;
					ieWindow.cbSize = sizeof(IEVIEWWINDOW);
					ieWindow.iType = IEW_DESTROY;
					ieWindow.hwnd = dat->hwndLog;
					CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
				}
				dat->hwndLog = NULL;
			}
			if(g_dat->avatarServiceExists) {
				dat->ace = (struct avatarCacheEntry *)CallService(MS_AV_GETAVATARBITMAP, (WPARAM)dat->windowData.hContact, 0);
			}
			SendMessage(hwndDlg, DM_GETAVATAR, 0, 0);
			SetDialogToType(hwndDlg);

			colour = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETBKGNDCOLOR, 0, colour);
			colour = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_INPUTBKGCOLOUR, SRMSGDEFSET_INPUTBKGCOLOUR);
			SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETBKGNDCOLOR, 0, colour);
			InvalidateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, FALSE);
			LoadMsgDlgFont(MSGFONTID_MESSAGEAREA, &lf, &colour);
			cf2.dwMask = CFM_COLOR | CFM_FACE | CFM_CHARSET | CFM_SIZE | CFM_WEIGHT | CFM_BOLD | CFM_ITALIC;
			cf2.cbSize = sizeof(cf2);
			cf2.crTextColor = colour;
			cf2.bCharSet = lf.lfCharSet;
			_tcsncpy(cf2.szFaceName, lf.lfFaceName, LF_FACESIZE);
			cf2.dwEffects = ((lf.lfWeight >= FW_BOLD) ? CFE_BOLD : 0) | (lf.lfItalic ? CFE_ITALIC : 0);
			cf2.wWeight = (WORD)lf.lfWeight;
			cf2.bPitchAndFamily = lf.lfPitchAndFamily;
			cf2.yHeight = abs(lf.lfHeight) * 15;
			SendDlgItemMessageA(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, 0, (LPARAM)&cf2);
			SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETLANGOPTIONS, 0, (LPARAM) SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETLANGOPTIONS, 0, 0) & ~IMF_AUTOKEYBOARD);

			pf2.cbSize = sizeof(pf2);
			pf2.dwMask = PFM_OFFSET;
			pf2.dxOffset = (g_dat->flags & SMF_INDENTTEXT) ? g_dat->indentSize * 15 : 0;
 			SetDlgItemText(hwndDlg, IDC_LOG, _T(""));
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETLANGOPTIONS, 0, (LPARAM) SendDlgItemMessage(hwndDlg, IDC_LOG, EM_GETLANGOPTIONS, 0, 0) & ~(IMF_AUTOKEYBOARD | IMF_AUTOFONTSIZEADJUST));

			SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
			SendMessage(hwndDlg, DM_UPDATEICON, 0, 1);
			SendMessage(hwndDlg, DM_UPDATESTATUSBAR, 0, 0);
			break;
		}
    case DM_USERNAMETOCLIP:
		{
			CONTACTINFO ci;
			char buf[128];
			HGLOBAL hData;

			buf[0] = 0;
			if(dat->windowData.hContact) {
				ZeroMemory(&ci, sizeof(ci));
				ci.cbSize = sizeof(ci);
				ci.hContact = dat->windowData.hContact;
				ci.szProto = dat->szProto;
				ci.dwFlag = CNF_UNIQUEID;
				if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
					switch (ci.type) {
						case CNFT_ASCIIZ:
							mir_snprintf(buf, sizeof(buf), "%s", ci.pszVal);
							miranda_sys_free(ci.pszVal);
							break;
						case CNFT_DWORD:
							mir_snprintf(buf, sizeof(buf), "%u", ci.dVal);
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
	case DM_SWITCHTOOLBAR:
		SetDialogToType(hwndDlg);
//		SendMessage(dat->hwndParent, DM_SWITCHTOOLBAR, 0, 0);
		break;
	case DM_GETCODEPAGE:
		SetWindowLong(hwndDlg, DWL_MSGRESULT, dat->windowData.codePage);
		return TRUE;
	case DM_SETCODEPAGE:
		dat->windowData.codePage = (int) lParam;
		SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
		break;
	case DM_SWITCHUNICODE:
#if defined( _UNICODE )
		{
			StatusIconData sid = {0};
			dat->flags ^= SMF_DISABLE_UNICODE;
			sid.cbSize = sizeof(sid);
			sid.szModule = SRMMMOD;
			sid.flags = (dat->flags & SMF_DISABLE_UNICODE) ? MBF_DISABLED : 0;
			ModifyStatusIcon((WPARAM)dat->windowData.hContact, (LPARAM) &sid);
			SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
		}
#endif
		break;
	case DM_SWITCHTYPING:
		if (IsTypingNotificationSupported(dat)) {
			StatusIconData sid = {0};
			sid.cbSize = sizeof(sid);
			sid.szModule = SRMMMOD;
			sid.dwId = 1;
			BYTE typingNotify = (DBGetContactSettingByte(dat->windowData.hContact, SRMMMOD, SRMSGSET_TYPING,
				DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW)));
			DBWriteContactSettingByte(dat->windowData.hContact, SRMMMOD, SRMSGSET_TYPING, !typingNotify);
			sid.flags = typingNotify ? MBF_DISABLED : 0;
			ModifyStatusIcon((WPARAM)dat->windowData.hContact, (LPARAM) &sid);
		}
		break;
	case DM_SWITCHRTL:
		{
			PARAFORMAT2 pf2;
			ZeroMemory((void *)&pf2, sizeof(pf2));
			pf2.cbSize = sizeof(pf2);
			pf2.dwMask = PFM_RTLPARA;
			dat->flags ^= SMF_RTL;
			if (dat->flags&SMF_RTL) {
				pf2.wEffects = PFE_RTLPARA;
				SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE) | WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
				SetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE) | WS_EX_LEFTSCROLLBAR);
			} else {
				pf2.wEffects = 0;
				SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE),GWL_EXSTYLE) &~ (WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR));
				SetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE,GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG),GWL_EXSTYLE) &~ (WS_EX_LEFTSCROLLBAR));
			}
			SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETPARAFORMAT, 0, (LPARAM)&pf2);
		}
		SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
		break;
	case DM_GETWINDOWSTATE:
		{
			UINT state = 0;

			state |= MSG_WINDOW_STATE_EXISTS;
			if (IsWindowVisible(hwndDlg))
				state |= MSG_WINDOW_STATE_VISIBLE;
			if (GetForegroundWindow()==dat->hwndParent)
				state |= MSG_WINDOW_STATE_FOCUS;
			if (IsIconic(dat->hwndParent))
				state |= MSG_WINDOW_STATE_ICONIC;
			SetWindowLong(hwndDlg, DWL_MSGRESULT, state);
			return TRUE;

		}
	case DM_ACTIVATE:
	case WM_ACTIVATE:
		if (LOWORD(wParam) != WA_ACTIVE)
			break;
		//fall through
	case WM_MOUSEACTIVATE:
		if (dat->hDbUnreadEventFirst != NULL) {
			HANDLE hDbEvent = dat->hDbUnreadEventFirst;
			dat->hDbUnreadEventFirst = NULL;
			while (hDbEvent != NULL) {
				DBEVENTINFO dbei;
				ZeroMemory(&dbei, sizeof(dbei));
				dbei.cbSize = sizeof(dbei);
				dbei.cbBlob = 0;
				CallService(MS_DB_EVENT_GET, (WPARAM) hDbEvent, (LPARAM) & dbei);
				if (!(dbei.flags & DBEF_SENT) && (dbei.eventType == EVENTTYPE_MESSAGE || dbei.eventType == EVENTTYPE_URL)) {
					CallService(MS_CLIST_REMOVEEVENT, (WPARAM) dat->windowData.hContact, (LPARAM) hDbEvent);
				}
				hDbEvent = (HANDLE) CallService(MS_DB_EVENT_FINDNEXT, (WPARAM) hDbEvent, 0);
			}
		}
		if (dat->showUnread) {
			dat->showUnread = 0;
			KillTimer(hwndDlg, TIMERID_UNREAD);
			SendMessage(hwndDlg, DM_UPDATEICON, 0, 0);
		}
		break;
	case WM_LBUTTONDOWN:
		SendMessage(dat->hwndParent, WM_LBUTTONDOWN, wParam, lParam);
		return TRUE;
	case DM_SETFOCUS:
		if (lParam == WM_MOUSEACTIVATE) {
			HWND hLog;
			RECT rc;
			POINT pt;
			GetCursorPos(&pt);
			if (dat->hwndLog != NULL) {
				hLog = dat->hwndLog;
			} else {
				hLog = GetDlgItem(hwndDlg, IDC_LOG);
			}
			GetWindowRect(hLog, &rc);
			if (pt.x >= rc.left && pt.x <= rc.right && pt.y >= rc.top && pt.y <=rc.bottom) {
		//		SetFocus(hLog);
				return TRUE;
			}
		}
		SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
		return TRUE;
	case WM_SETFOCUS:
		SendMessage(dat->hwndParent, CM_ACTIVATECHILD, 0, (LPARAM)hwndDlg);
		PostMessage(hwndDlg, DM_SETFOCUS, 0, 0);
//		SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
		return TRUE;
	case DM_SETPARENT:
		dat->hwndParent = (HWND) lParam;
		dat->parent = (ParentWindowData *) GetWindowLong(dat->hwndParent, GWL_USERDATA);
		SetParent(hwndDlg, dat->hwndParent);
		return TRUE;
	case WM_GETMINMAXINFO:
		{
			MINMAXINFO *mmi = (MINMAXINFO *) lParam;
			int minBottomHeight = dat->toolbarSize.cy + dat->minEditBoxHeight;
			if (minBottomHeight < g_dat->limitAvatarMinH) {
			}
			mmi->ptMinTrackSize.x = dat->toolbarSize.cx;// + dat->avatarWidth;
			mmi->ptMinTrackSize.y = dat->minLogBoxHeight + minBottomHeight;

			return 0;
		}
	case WM_SIZE:
		{
			if (wParam==SIZE_RESTORED || wParam==SIZE_MAXIMIZED) {
				RECT rc;
				int dlgWidth, dlgHeight;
				dlgWidth = LOWORD(lParam);
				dlgHeight = HIWORD(lParam);
				/*if (dlgWidth == 0 && dlgHeight ==0) */{
					GetClientRect(hwndDlg, &rc);
					dlgWidth = rc.right - rc.left;
					dlgHeight = rc.bottom - rc.top;
				}
				MessageDialogResize(hwndDlg, dat, dlgWidth, dlgHeight);
			}
			return TRUE;
		}
	case DM_SPLITTERMOVED:
		{
			POINT pt;
			RECT rc;
			RECT rcLog;
			if ((HWND) lParam == GetDlgItem(hwndDlg, IDC_SPLITTER)) {
				BOOL isCtrl = GetKeyState(VK_CONTROL) & 0x8000;
				int oldSplitterY;
				GetWindowRect(GetDlgItem(hwndDlg, IDC_LOG), &rcLog);
				GetClientRect(hwndDlg, &rc);
				pt.x = 0;
				pt.y = wParam;
				ScreenToClient(hwndDlg, &pt);
				if (isCtrl) {
					oldSplitterY = dat->toolbarSize.cy + dat->splitterPos - (rc.bottom - pt.y);
					oldSplitterY = max(min(oldSplitterY, 24), 18);
					if (oldSplitterY == dat->toolbarSize.cy) {
						break;
					}
					pt.y = oldSplitterY - dat->toolbarSize.cy - dat->splitterPos + rc.bottom;
					dat->toolbarSize.cy = oldSplitterY;
				}
				oldSplitterY = dat->splitterPos;
				dat->splitterPos = rc.bottom - pt.y;
			/*
				GetWindowRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc);
				if (rc.bottom - rc.top + (dat->splitterPos - oldSplitterY) < dat->minEditBoxSize.cy)
					dat->splitterPos = oldSplitterY + dat->minEditBoxSize.cy - (rc.bottom - rc.top);
				if (rcLog.bottom - rcLog.top - (dat->splitterPos - oldSplitterY) < dat->minEditBoxSize.cy)
					dat->splitterPos = oldSplitterY - dat->minEditBoxSize.cy + (rcLog.bottom - rcLog.top);
					*/
				SendMessage(hwndDlg, WM_SIZE, 0, 0);
			}
			break;
		}
	case DM_REMAKELOG:
		dat->lastEventType = -1;
		if (wParam == 0 || (HANDLE) wParam == dat->windowData.hContact) {
			//StreamInEvents(hwndDlg, dat->hDbEventFirst, 0, 0);
			StreamInEvents(hwndDlg, dat->hDbEventFirst, -1, 0);
		}
		InvalidateRect(GetDlgItem(hwndDlg, IDC_LOG), NULL, FALSE);
		break;
	case DM_APPENDTOLOG:   //takes wParam=hDbEvent
		StreamInEvents(hwndDlg, (HANDLE) wParam, 1, 1);
		break;
	case DM_SCROLLLOGTOBOTTOM:
		if (dat->hwndLog == NULL) {
			/*
			int	nMin, nMax;
			HWND hwndLog = GetDlgItem(hwndDlg, IDC_LOG);
			GetScrollRange(hwndLog, SB_VERT, &nMin, &nMax);
			SetScrollPos(hwndLog, SB_VERT, nMax, TRUE);
			PostMessage(hwndLog, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, nMax), (LPARAM) NULL);
			*/
			SCROLLINFO si = { 0 };
			if ((GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG), GWL_STYLE) & WS_VSCROLL) == 0)
				break;
			si.cbSize = sizeof(si);
			si.fMask = SIF_PAGE | SIF_RANGE;
			if (GetScrollInfo(GetDlgItem(hwndDlg, IDC_LOG), SB_VERT, &si)) {
				si.fMask = SIF_POS;
				si.nPos = si.nMax - si.nPage + 1;
				SetScrollInfo(GetDlgItem(hwndDlg, IDC_LOG), SB_VERT, &si, TRUE);
				PostMessage(GetDlgItem(hwndDlg, IDC_LOG), WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
			}
			RedrawWindow(GetDlgItem(hwndDlg, IDC_LOG), NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		} else {
			IEVIEWWINDOW ieWindow;
			ieWindow.cbSize = sizeof(IEVIEWWINDOW);
			ieWindow.iType = IEW_SCROLLBOTTOM;
			ieWindow.hwnd = dat->hwndLog;
			CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
		}
		break;
	case HM_DBEVENTADDED:
		if ((HANDLE) wParam == dat->windowData.hContact)
		{
			DBEVENTINFO dbei = { 0 };

			dbei.cbSize = sizeof(dbei);
			dbei.cbBlob = 0;
			CallService(MS_DB_EVENT_GET, lParam, (LPARAM) & dbei);
			if (dat->hDbEventFirst == NULL)
				dat->hDbEventFirst = (HANDLE) lParam;
			if (DbEventIsShown(&dbei, dat)) {
				if (dbei.eventType == EVENTTYPE_MESSAGE && !(dbei.flags & (DBEF_SENT))) {
					/* store the event when the container is hidden so that clist notifications can be removed */
					if (!IsWindowVisible(GetParent(hwndDlg)) && dat->hDbUnreadEventFirst == NULL)
						dat->hDbUnreadEventFirst = (HANDLE) lParam;
					dat->lastMessage = dbei.timestamp;
					SendMessage(hwndDlg, DM_UPDATESTATUSBAR, 0, 0);
					if (GetForegroundWindow()==dat->hwndParent && dat->parent->hwndActive == hwndDlg)
						SkinPlaySound("RecvMsgActive");
					else SkinPlaySound("RecvMsgInactive");
					if ((g_dat->flags2 & SMF2_SWITCHTOACTIVE) && (IsIconic(dat->hwndParent) || GetActiveWindow() != dat->hwndParent) && IsWindowVisible(dat->hwndParent)) {
						SendMessage(dat->hwndParent, CM_ACTIVATECHILD, 0, (LPARAM) hwndDlg);
					}
					if (IsAutoPopup(dat->windowData.hContact)) {
						SendMessage(GetParent(hwndDlg), CM_POPUPWINDOW, (WPARAM) 1, (LPARAM) hwndDlg);
					}
				}
				if ((HANDLE) lParam != dat->hDbEventFirst && (HANDLE) CallService(MS_DB_EVENT_FINDNEXT, lParam, 0) == NULL)
					SendMessage(hwndDlg, DM_APPENDTOLOG, lParam, 0);
				else
					SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
				if (!(dbei.flags & DBEF_SENT) && dbei.eventType != EVENTTYPE_STATUSCHANGE) {
					if (GetActiveWindow() != dat->hwndParent || GetForegroundWindow() != dat->hwndParent || dat->parent->hwndActive != hwndDlg) {
						dat->showUnread = 1;
						SendMessage(hwndDlg, DM_UPDATEICON, 0, 0);
						SetTimer(hwndDlg, TIMERID_UNREAD, TIMEOUT_UNREAD, NULL);
					}
					SendMessage(dat->hwndParent, CM_STARTFLASHING, 0, 0);
				}
			}
		}
		break;
	case DM_UPDATESTATUSBAR:
		if (dat->parent->hwndActive == hwndDlg) {
			TCHAR szText[256];
			StatusBarData sbd= {0};
			StatusIconData sid = {0};
			sbd.iFlags = SBDF_TEXT | SBDF_ICON;
			if (dat->messagesInProgress && (g_dat->flags & SMF_SHOWPROGRESS)) {
				sbd.hIcon = g_dat->hIcons[SMF_ICON_DELIVERING];
				sbd.pszText = szText;
				mir_sntprintf(szText, SIZEOF(szText), TranslateT("Sending in progress: %d message(s) left..."), dat->messagesInProgress);
			} else if (dat->nTypeSecs) {
				TCHAR *szContactName = GetNickname(dat->windowData.hContact, dat->szProto);
				sbd.hIcon = g_dat->hIcons[SMF_ICON_TYPING];
				sbd.pszText = szText;
				mir_sntprintf(szText, SIZEOF(szText), TranslateT("%s is typing a message..."), szContactName);
				mir_free(szContactName);
				dat->nTypeSecs--;
			} else if (dat->lastMessage) {
				DBTIMETOSTRINGT dbtts;
				TCHAR date[64], time[64];
				dbtts.szFormat = _T("d");
				dbtts.cbDest = SIZEOF(date);
				dbtts.szDest = date;
#if defined ( _UNICODE )
				CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, dat->lastMessage, (LPARAM) & dbtts);
#else
				CallService(MS_DB_TIME_TIMESTAMPTOSTRING, dat->lastMessage, (LPARAM) & dbtts);
#endif
				dbtts.szFormat = _T("t");
				dbtts.cbDest = SIZEOF(time);
				dbtts.szDest = time;
#if defined ( _UNICODE )
				CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, dat->lastMessage, (LPARAM) & dbtts);
#else
				CallService(MS_DB_TIME_TIMESTAMPTOSTRING, dat->lastMessage, (LPARAM) & dbtts);
#endif
				mir_sntprintf(szText, SIZEOF(szText), TranslateT("Last message received on %s at %s."), date, time);
				sbd.pszText = szText;
			} else {
				sbd.pszText =  _T("");
			}
			SendMessage(dat->hwndParent, CM_UPDATESTATUSBAR, (WPARAM)&sbd, (LPARAM)hwndDlg);
			UpdateReadChars(hwndDlg, dat);
			sid.cbSize = sizeof(sid);
			sid.szModule = SRMMMOD;
#if defined ( _UNICODE )
			sid.flags = (dat->flags & SMF_DISABLE_UNICODE) ? MBF_DISABLED : 0;
#else
			sid.flags = MBF_DISABLED;
#endif
			ModifyStatusIcon((WPARAM)dat->windowData.hContact, (LPARAM) &sid);
			sid.dwId = 1;
			if (IsTypingNotificationSupported(dat) && g_dat->flags2 & SMF2_SHOWTYPINGSWITCH) {
				sid.flags = (DBGetContactSettingByte(dat->windowData.hContact, SRMMMOD, SRMSGSET_TYPING,
					DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW))) ? 0 : MBF_DISABLED;
			} else {
				sid.flags = MBF_HIDDEN;
			}
			ModifyStatusIcon((WPARAM)dat->windowData.hContact, (LPARAM) &sid);
		}
		break;
	case DM_CLEARLOG:
	// IEVIew MOD Begin
		if (dat->hwndLog != NULL) {
			IEVIEWEVENT event;
			ZeroMemory(&event, sizeof(event));
			event.cbSize = sizeof(event);
			event.iType = IEE_CLEAR_LOG;
			event.dwFlags = ((dat->flags & SMF_RTL) ? IEEF_RTL : 0) | ((dat->flags & SMF_DISABLE_UNICODE) ? IEEF_NO_UNICODE : 0);
			event.hwnd = dat->hwndLog;
			event.hContact = dat->windowData.hContact;
			event.codepage = dat->windowData.codePage;
			event.pszProto = dat->szProto;
			CallService(MS_IEVIEW_EVENT, 0, (LPARAM)&event);
		}
	// IEVIew MOD End
		SetDlgItemText(hwndDlg, IDC_LOG, _T(""));
		dat->hDbEventFirst = NULL;
		dat->lastEventType = -1;
		break;
	case WM_TIMER:
		if (wParam == TIMERID_MSGSEND) {
			ReportSendQueueTimeouts(hwndDlg);
		} else if (wParam == TIMERID_TYPE) {
			if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON && GetTickCount() - dat->nLastTyping > TIMEOUT_TYPEOFF) {
				NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
			}
			if (dat->showTyping) {
				if (dat->nTypeSecs) {
					dat->nTypeSecs--;
				}
				else {
					dat->showTyping = 0;
					SendMessage(hwndDlg, DM_UPDATESTATUSBAR, 0, 0);
					SendMessage(hwndDlg, DM_UPDATEICON, 0, 0);
				}
			}
			else {
				if (dat->nTypeSecs) {
					dat->showTyping = 1;
					SendMessage(hwndDlg, DM_UPDATESTATUSBAR, 0, 0);
					SendMessage(hwndDlg, DM_UPDATEICON, 0, 0);
				}
			}
		} else if (wParam == TIMERID_UNREAD) {
			TabControlData tcd;
			tcd.iFlags = TCDF_ICON;
			if (!dat->showTyping) {
				dat->showUnread++;
				if (dat->showUnread & 1) {
					tcd.hIcon = dat->statusIconOverlay;
				} else {
					tcd.hIcon = dat->statusIcon;
				}
				SendMessage(dat->hwndParent, CM_UPDATETABCONTROL, (WPARAM)&tcd, (LPARAM)hwndDlg);
			}
		}
		break;
	case DM_SENDMESSAGE:
		if (lParam) {
			MessageSendQueueItem *msi = (MessageSendQueueItem *)lParam;
			MessageSendQueueItem *item = CreateSendQueueItem(hwndDlg);
			item->hContact = dat->windowData.hContact;
			item->proto = mir_strdup(dat->szProto);
			item->flags = msi->flags;
			item->codepage = dat->windowData.codePage;
			if ( IsUtfSendAvailable( dat->windowData.hContact )) {
				char* szMsgUtf;
				#if defined( _UNICODE )
					szMsgUtf = mir_utf8encodeW( (TCHAR *)&msi->sendBuffer[strlen(msi->sendBuffer) + 1] );
					item->flags &= ~PREF_UNICODE;
				#else
					szMsgUtf = mir_utf8encodecp(msi->sendBuffer, dat->windowData.codePage);
				#endif
				if (!szMsgUtf) {
					break;
				}
				if  (*szMsgUtf == 0) {
					mir_free(szMsgUtf);
					break;
				}
				item->sendBufferSize = strlen(szMsgUtf) + 1;
				item->sendBuffer = szMsgUtf;
				item->flags |= PREF_UTF;
			} else {
				item->sendBufferSize = msi->sendBufferSize;
				item->sendBuffer = (char *) mir_alloc(msi->sendBufferSize);
				memcpy(item->sendBuffer, msi->sendBuffer, msi->sendBufferSize);
			}
			SendMessage(hwndDlg, DM_STARTMESSAGESENDING, 0, 0);
			SendSendQueueItem(item);
		}
		break;
	case DM_STARTMESSAGESENDING:
		dat->messagesInProgress++;
	case DM_SHOWMESSAGESENDING:
		SetTimer(hwndDlg, TIMERID_MSGSEND, 1000, NULL);
		if (g_dat->flags & SMF_SHOWPROGRESS) {
			SendMessage(dat->hwnd, DM_UPDATESTATUSBAR, 0, 0);
		}
		break;
	case DM_STOPMESSAGESENDING:
		if (dat->messagesInProgress>0) {
			dat->messagesInProgress--;
			if (g_dat->flags & SMF_SHOWPROGRESS) {
				SendMessage(dat->hwnd, DM_UPDATESTATUSBAR, 0, 0);
			}
		}
		if (dat->messagesInProgress == 0) {
			KillTimer(hwndDlg, TIMERID_MSGSEND);
		}
		break;
	case DM_SHOWERRORMESSAGE:
		if (lParam) {
			ErrorWindowData *ewd = (ErrorWindowData *) lParam;
			SendMessage(hwndDlg, DM_STOPMESSAGESENDING, 0, 0);
			ewd->queueItem->hwndErrorDlg = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_MSGSENDERROR), hwndDlg, ErrorDlgProc, (LPARAM) ewd);//hwndDlg
		}
		break;
	case DM_ERRORDECIDED:
		{
			MessageSendQueueItem *item = (MessageSendQueueItem *) lParam;
			item->hwndErrorDlg = NULL;
			switch (wParam) {
			case MSGERROR_CANCEL:
				RemoveSendQueueItem(item);
				SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
				break;
			case MSGERROR_RETRY:
				SendMessage(hwndDlg, DM_STARTMESSAGESENDING, 0, 0);
				SendSendQueueItem(item);
				break;
			}
		}
		break;
	case WM_MEASUREITEM:
		if (!MeasureMenuItem(wParam, lParam)) {
			return CallService(MS_CLIST_MENUMEASUREITEM, wParam, lParam);
		}
		return TRUE;

	case WM_DRAWITEM:
			{
				LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;
				if (dis->hwndItem == (HWND)hToolbarMenu) {
					return DrawMenuItem(wParam, lParam);
				} else if (dis->hwndItem == GetDlgItem(hwndDlg, IDC_AVATAR) && dat->avatarPic && (g_dat->flags&SMF_AVATAR)) {
					HDC hdcMem = CreateCompatibleDC(dis->hDC);
					HBITMAP hbmMem = CreateCompatibleBitmap(dis->hDC, dat->avatarWidth, dat->avatarHeight);
					RECT rect;
					hbmMem = (HBITMAP) SelectObject(hdcMem, hbmMem);
					rect.top = 0;
					rect.left = 0;
					rect.right = dat->avatarWidth;
					rect.bottom = dat->avatarHeight;
					if (pfnIsAppThemed && pfnDrawThemeParentBackground && pfnIsAppThemed()) {
						pfnDrawThemeParentBackground(dis->hwndItem, hdcMem, &rect);
					} else {
						FillRect(hdcMem, &rect, GetSysColorBrush(COLOR_3DFACE));
					}
					if (!g_dat->avatarServiceExists) {
						BITMAP bminfo;
						HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
						HDC hdcTemp = CreateCompatibleDC(dis->hDC);
						HBITMAP hbmTemp = (HBITMAP)SelectObject(hdcTemp, dat->avatarPic);
						hPen = (HPEN)SelectObject(hdcMem, hPen);
						Rectangle(hdcMem, 0, 0, dat->avatarWidth, dat->avatarHeight);
						GetObject(dat->avatarPic, sizeof(bminfo), &bminfo);
						SetStretchBltMode(hdcMem, HALFTONE);
						StretchBlt(hdcMem, 1, 1, dat->avatarWidth-2, dat->avatarHeight-2, hdcTemp, 0, 0, bminfo.bmWidth, bminfo.bmHeight, SRCCOPY);
						hbmTemp = (HBITMAP) SelectObject(hdcTemp, hbmTemp);
						DeleteDC(hdcTemp);
						hPen = (HPEN)SelectObject(hdcMem, hPen);
						DeleteObject(hPen);
					} else {
						AVATARDRAWREQUEST adr;
						ZeroMemory(&adr, sizeof(adr));
						adr.cbSize = sizeof (AVATARDRAWREQUEST);
						adr.hContact = dat->windowData.hContact;
						adr.hTargetDC = hdcMem;
						adr.rcDraw.left = 0;
						adr.rcDraw.top = 0;
						adr.rcDraw.right = dat->avatarWidth;
						adr.rcDraw.bottom = dat->avatarHeight;
						adr.dwFlags = AVDRQ_DRAWBORDER | AVDRQ_HIDEBORDERONTRANSPARENCY;
						CallService(MS_AV_DRAWAVATAR, (WPARAM)0, (LPARAM)&adr);
					}
					BitBlt(dis->hDC, 0, 0, dat->avatarWidth, dat->avatarHeight, hdcMem, 0, 0, SRCCOPY);
					hbmMem = (HBITMAP) SelectObject(hdcMem, hbmMem);
					DeleteObject(hbmMem);
					DeleteDC(hdcMem);
					return TRUE;
				}
				return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
			}
	case WM_COMMAND:
		if (!lParam && CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(LOWORD(wParam), MPCF_CONTACTMENU), (LPARAM) dat->windowData.hContact))
			break;
		switch (LOWORD(wParam)) {
		case IDC_SENDALL:
			{
				int result;
				if (dat->sendAllConfirm == 0) {
					result = DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_CONFIRM_SENDALL), hwndDlg, ConfirmSendAllDlgProc, (LPARAM)hwndDlg);
					if (result & 0x10000) {
						dat->sendAllConfirm = result;
					}
				} else {
					result = dat->sendAllConfirm;
				}
				if (LOWORD(result) != IDYES) {
					break;
				}

			}
		case IDOK:
			//this is a 'send' button
			if (!IsWindowEnabled(GetDlgItem(hwndDlg, IDOK)))
				break;
			//if(GetKeyState(VK_CTRL) & 0x8000) {    // copy user name
					//SendMessage(hwndDlg, DM_USERNAMETOCLIP, 0, 0);
			//}
			if (dat->windowData.hContact != NULL) {
				GETTEXTEX  gt = {0};
				PARAFORMAT2 pf2;
				MessageSendQueueItem msi = { 0 };
				int bufSize;
				int ansiBufSize = GetRichTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE), dat->windowData.codePage, TRUE) + 1;
				bufSize = ansiBufSize;
				ZeroMemory((void *)&pf2, sizeof(pf2));
				pf2.cbSize = sizeof(pf2);
				pf2.dwMask = PFM_RTLPARA;
				SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETPARAFORMAT, 0, (LPARAM)&pf2);
				if (pf2.wEffects & PFE_RTLPARA)
					msi.flags |= PREF_RTL;
				#if defined( _UNICODE )
					bufSize += GetRichTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE), 1200, TRUE) + 2;
				#endif
				msi.sendBufferSize = bufSize;
				msi.sendBuffer = (char *) mir_alloc(msi.sendBufferSize);
				msi.flags |= PREF_TCHAR;

				gt.flags = GT_USECRLF;
				gt.cb = ansiBufSize;
				gt.codepage = dat->windowData.codePage;
				SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETTEXTEX, (WPARAM) &gt, (LPARAM) msi.sendBuffer);
				#if defined( _UNICODE )
					gt.cb = bufSize - ansiBufSize;
					gt.codepage = 1200;
					SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETTEXTEX, (WPARAM) &gt, (LPARAM) &msi.sendBuffer[ansiBufSize]);
					if ( RTL_Detect((wchar_t *)&msi.sendBuffer[ansiBufSize] ))
						msi.flags |= PREF_RTL;
				#endif
				if (msi.sendBuffer[0] == 0) {
					mir_free (msi.sendBuffer);
					break;
				}
				{
					/* Store messaging history */
					char *msgText = GetRichTextEncoded(GetDlgItem(hwndDlg, IDC_MESSAGE), dat->windowData.codePage);
					TCmdList *cmdListNew = tcmdlist_last(dat->windowData.cmdList);
					while (cmdListNew != NULL && cmdListNew->temporary) {
						dat->windowData.cmdList = tcmdlist_remove(dat->windowData.cmdList, cmdListNew);
						cmdListNew = tcmdlist_last(dat->windowData.cmdList);
					}
					if (msgText != NULL) {
						dat->windowData.cmdList = tcmdlist_append(dat->windowData.cmdList, msgText, 20, FALSE);
						mir_free(msgText);
					}
					dat->windowData.cmdListCurrent = NULL;
				}
				if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON)
					NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);

				SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
				EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
				if (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOMIN, SRMSGDEFSET_AUTOMIN))
					ShowWindow(dat->hwndParent, SW_MINIMIZE);
				if (LOWORD(wParam) == IDC_SENDALL) {
					SendMessage(dat->hwndParent, DM_SENDMESSAGE, 0, (LPARAM) &msi);
				} else {
					SendMessage(hwndDlg, DM_SENDMESSAGE, 0, (LPARAM) &msi);
				}
				mir_free (msi.sendBuffer);
			}
			return TRUE;
		case IDCANCEL:
			DestroyWindow(hwndDlg);
			return TRUE;
		case IDC_USERMENU:
			{
				if(GetKeyState(VK_SHIFT) & 0x8000) {    // copy user name
					SendMessage(hwndDlg, DM_USERNAMETOCLIP, 0, 0);
				}
				else {
					RECT rc;
					HMENU hMenu = (HMENU) CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM) dat->windowData.hContact, 0);
					GetWindowRect(GetDlgItem(hwndDlg, LOWORD(wParam)), &rc);
					TrackPopupMenu(hMenu, 0, rc.left, rc.bottom, 0, hwndDlg, NULL);
					DestroyMenu(hMenu);
				}
			}
			break;
		case IDC_HISTORY:
			CallService(MS_HISTORY_SHOWCONTACTHISTORY, (WPARAM) dat->windowData.hContact, 0);
			break;
		case IDC_DETAILS:
			CallService(MS_USERINFO_SHOWDIALOG, (WPARAM) dat->windowData.hContact, 0);
			break;
		case IDC_SMILEYS:
			if (g_dat->smileyServiceExists) {
				SMADD_SHOWSEL3 smaddInfo;
				RECT rc;
				smaddInfo.cbSize = sizeof(SMADD_SHOWSEL3);
				smaddInfo.hwndParent = dat->hwndParent;
				smaddInfo.hwndTarget = GetDlgItem(hwndDlg, IDC_MESSAGE);
				smaddInfo.targetMessage = EM_REPLACESEL;
				smaddInfo.targetWParam = TRUE;
				smaddInfo.Protocolname = dat->szProto;
				if (dat->szProto!=NULL && strcmp(dat->szProto,"MetaContacts")==0) {
					HANDLE hContact = (HANDLE) CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM) dat->windowData.hContact, 0);
					if (hContact!=NULL) {
						smaddInfo.Protocolname = (char*) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
					}
				}
				GetWindowRect(GetDlgItem(hwndDlg, IDC_SMILEYS), &rc);
				smaddInfo.Direction = 0;
				smaddInfo.xPosition = rc.left;
				smaddInfo.yPosition = rc.bottom;
				smaddInfo.hContact = dat->windowData.hContact;
				CallService(MS_SMILEYADD_SHOWSELECTION, 0, (LPARAM) &smaddInfo);
			}
			break;
		case IDC_QUOTE:
			{
				DBEVENTINFO dbei = { 0 };
				SETTEXTEX  st;
				TCHAR *buffer = NULL;
				st.flags = ST_SELECTION;
#ifdef _UNICODE
				st.codepage = 1200;
#else
				st.codepage = CP_ACP;
#endif
				if (dat->hDbEventLast==NULL) break;
				if (dat->hwndLog != NULL) {
					buffer = GetIEViewSelection(dat);
				} else {
					buffer = GetRichEditSelection(GetDlgItem(hwndDlg, IDC_LOG));
				}
				if (buffer!=NULL) {
					TCHAR *quotedBuffer = GetQuotedTextW(buffer);
					SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_SETTEXTEX, (WPARAM) &st, (LPARAM)quotedBuffer);
					mir_free(quotedBuffer);
					mir_free(buffer);
					SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
					break;
				}
				dbei.cbSize = sizeof(dbei);
				dbei.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM) dat->hDbEventLast, 0);
				if (dbei.cbBlob == 0xFFFFFFFF) break;
				dbei.pBlob = (PBYTE) mir_alloc(dbei.cbBlob);
				CallService(MS_DB_EVENT_GET, (WPARAM)  dat->hDbEventLast, (LPARAM) & dbei);
				if (dbei.eventType == EVENTTYPE_MESSAGE || dbei.eventType == EVENTTYPE_STATUSCHANGE) {
					TCHAR *buffer = DbGetEventTextT( &dbei, CP_ACP );
					if (buffer!=NULL) {
						TCHAR *quotedBuffer = GetQuotedTextW(buffer);
						SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), EM_SETTEXTEX, (WPARAM) &st, (LPARAM)quotedBuffer);
						mir_free(quotedBuffer);
						mir_free(buffer);
					}
				}
				mir_free(dbei.pBlob);
				SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
				break;
			}
		case IDC_ADD:
			{
				ADDCONTACTSTRUCT acs = { 0 };

				acs.handle = dat->windowData.hContact;
				acs.handleType = HANDLE_CONTACT;
				acs.szProto = 0;
				CallService(MS_ADDCONTACT_SHOW, (WPARAM) hwndDlg, (LPARAM) & acs);
			}
			if (!DBGetContactSettingByte(dat->windowData.hContact, "CList", "NotOnList", 0)) {
				ShowWindow(GetDlgItem(hwndDlg, IDC_ADD), FALSE);
			}
		case IDC_MESSAGE:
			if (HIWORD(wParam) == EN_CHANGE) {
				int len = GetRichTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE), dat->windowData.codePage, FALSE);
				dat->windowData.cmdListCurrent = NULL;
				UpdateReadChars(hwndDlg, dat);
				EnableWindow(GetDlgItem(hwndDlg, IDOK), len != 0);
				if (!(GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_SHIFT) & 0x8000)) {
					dat->nLastTyping = GetTickCount();
					if (len != 0) {
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
			break;
		}
		break;
	case WM_NOTIFY:
		{
			LPNMHDR pNmhdr;
			pNmhdr = (LPNMHDR)lParam;
			switch (pNmhdr->idFrom) {
			case IDC_LOG:
				switch (pNmhdr->code) {
				case EN_MSGFILTER:
					switch (((MSGFILTER *) lParam)->msg) {
					case WM_CHAR:
						if (!(GetKeyState(VK_CONTROL) & 0x8000)) {
							SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
							SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), ((MSGFILTER *) lParam)->msg, ((MSGFILTER *) lParam)->wParam, ((MSGFILTER *) lParam)->lParam);
							SetWindowLong(hwndDlg, DWL_MSGRESULT, TRUE);
						}
						return TRUE;
					case WM_KEYDOWN:
						if (GetKeyState(VK_CONTROL) & 0x8000 && ((MSGFILTER *) lParam)->wParam== VK_TAB) {
							if (GetKeyState(VK_SHIFT) & 0x8000) {
								SendMessage(GetParent(hwndDlg), CM_ACTIVATEPREV, 0, (LPARAM)hwndDlg);
							} else {
								SendMessage(GetParent(hwndDlg), CM_ACTIVATENEXT, 0, (LPARAM)hwndDlg);
							}
							SetWindowLong(hwndDlg, DWL_MSGRESULT, TRUE);
						}
						return TRUE;
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
							pt.x = (short) LOWORD(((MSGFILTER *) lParam)->lParam);
							pt.y = (short) HIWORD(((MSGFILTER *) lParam)->lParam);
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
								SendMessage(hwndDlg, DM_CLEARLOG, 0, 0);
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
							TEXTRANGE tr;
							CHARRANGE sel;
							char* pszUrl;

							SendMessage(pNmhdr->hwndFrom, EM_EXGETSEL, 0, (LPARAM) & sel);
							if (sel.cpMin != sel.cpMax)
								break;
							tr.chrg = ((ENLINK *) lParam)->chrg;
							tr.lpstrText = mir_alloc(sizeof(TCHAR)*(tr.chrg.cpMax - tr.chrg.cpMin + 8));
							SendMessage(pNmhdr->hwndFrom, EM_GETTEXTRANGE, 0, (LPARAM) & tr);
							if (_tcschr(tr.lpstrText, _T('@')) != NULL && _tcschr(tr.lpstrText, _T(':')) == NULL && _tcschr(tr.lpstrText, _T('/')) == NULL) {
								MoveMemory(tr.lpstrText + sizeof(TCHAR) * 7, tr.lpstrText, sizeof(TCHAR)*(tr.chrg.cpMax - tr.chrg.cpMin + 1));
								CopyMemory(tr.lpstrText, _T("mailto:"), sizeof(TCHAR) * 7);
							}
							pszUrl = t2a( (const TCHAR *)tr.lpstrText );
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
									CallService(MS_UTILS_OPENURL, 1, (LPARAM) pszUrl);
									break;
								case IDM_OPENEXISTING:
									CallService(MS_UTILS_OPENURL, 0, (LPARAM) pszUrl);
									break;
								case IDM_COPYLINK:
									{
										HGLOBAL hData;
										if (!OpenClipboard(hwndDlg))
											break;
										EmptyClipboard();
										hData = GlobalAlloc(GMEM_MOVEABLE, sizeof(TCHAR)*(lstrlen(tr.lpstrText) + 1));
										lstrcpy(GlobalLock(hData), tr.lpstrText);
										GlobalUnlock(hData);
									#if defined( _UNICODE )
										SetClipboardData(CF_UNICODETEXT, hData);
									#else
										SetClipboardData(CF_TEXT, hData);
									 #endif
										CloseClipboard();
										break;
									}
								}
								DestroyMenu(hMenu);
								mir_free(tr.lpstrText);
								mir_free(pszUrl);
								SetWindowLong(hwndDlg, DWL_MSGRESULT, TRUE);
								return TRUE;
							}
							CallService(MS_UTILS_OPENURL, 1, (LPARAM) pszUrl);
							SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
							mir_free(tr.lpstrText);
							mir_free(pszUrl);
							break;
						}
					}
					break;
				}
				break;
			case IDC_MESSAGE:
				switch (((NMHDR *) lParam)->code) {
				case EN_MSGFILTER:
					switch (((MSGFILTER *) lParam)->msg) {
					case WM_RBUTTONUP:
						SetWindowLong(hwndDlg, DWL_MSGRESULT, TRUE);
						return TRUE;
					}
				}
			}
		}
		break;
	case WM_DESTROY:
		NotifyLocalWinEvent(dat->windowData.hContact, hwndDlg, MSG_WINDOW_EVT_CLOSING);
		if (dat->nTypeMode == PROTOTYPE_SELFTYPING_ON) {
			NotifyTyping(dat, PROTOTYPE_SELFTYPING_OFF);
		}
		if (dat->statusIcon != NULL) DestroyIcon(dat->statusIcon);
		if (dat->statusIconOverlay != NULL) DestroyIcon(dat->statusIconOverlay);
		dat->statusIcon = NULL;
		dat->statusIconOverlay = NULL;
		ReleaseSendQueueItems(hwndDlg);
		if (g_dat->flags & SMF_SAVEDRAFTS) {
			saveDraftMessage(GetDlgItem(hwndDlg, IDC_MESSAGE), dat->windowData.hContact, dat->windowData.codePage);
		} else {
			g_dat->draftList = tcmdlist_remove2(g_dat->draftList, dat->windowData.hContact);
		}
		tcmdlist_free(dat->windowData.cmdList);
		WindowList_Remove(g_dat->hMessageWindowList, hwndDlg);
		SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_WNDPROC, (LONG) OldSplitterProc);
		UnsubclassMessageEdit(GetDlgItem(hwndDlg, IDC_MESSAGE));
		UnsubclassLogEdit(GetDlgItem(hwndDlg, IDC_LOG));
		{
			HFONT hFont;
			hFont = (HFONT) SendDlgItemMessage(hwndDlg, IDC_MESSAGE, WM_GETFONT, 0, 0);
			if (hFont != NULL && hFont != (HFONT) SendDlgItemMessage(hwndDlg, IDOK, WM_GETFONT, 0, 0))
				DeleteObject(hFont);
		}
		DBWriteContactSettingByte(dat->windowData.hContact, SRMMMOD, "UseRTL", (BYTE) ((dat->flags & SMF_RTL) ? 1 : 0));
		DBWriteContactSettingByte(dat->windowData.hContact, SRMMMOD, "DisableUnicode", (BYTE) ((dat->flags & SMF_DISABLE_UNICODE) ? 1 : 0));
		DBWriteContactSettingWord(dat->windowData.hContact, SRMMMOD, "CodePage", (WORD) dat->windowData.codePage);
		DBWriteContactSettingDword((g_dat->flags & SMF_SAVESPLITTERPERCONTACT) ? dat->windowData.hContact : NULL, SRMMMOD, "splitterPos", dat->splitterPos);
		DBWriteContactSettingDword((g_dat->flags & SMF_SAVESPLITTERPERCONTACT) ? dat->windowData.hContact : NULL, SRMMMOD, "splitterHeight", dat->toolbarSize.cy);
		if (dat->avatarPic && !g_dat->avatarServiceExists)
			DeleteObject(dat->avatarPic);
		NotifyLocalWinEvent(dat->windowData.hContact, hwndDlg, MSG_WINDOW_EVT_CLOSE);
		if (dat->windowData.hContact && (g_dat->flags & SMF_DELTEMP)) {
			if (DBGetContactSettingByte(dat->windowData.hContact, "CList", "NotOnList", 0)) {
				CallService(MS_DB_CONTACT_DELETE, (WPARAM)dat->windowData.hContact, 0);
			}
		}
		if (dat->hwndLog != NULL) {
			IEVIEWWINDOW ieWindow;
			ieWindow.cbSize = sizeof(IEVIEWWINDOW);
			ieWindow.iType = IEW_DESTROY;
			ieWindow.hwnd = dat->hwndLog;
			CallService(MS_IEVIEW_WINDOW, 0, (LPARAM)&ieWindow);
		}
		SetWindowLong(hwndDlg, GWL_USERDATA, 0);
		SendMessage(dat->hwndParent, CM_REMOVECHILD, 0, (LPARAM) hwndDlg);
		mir_free(dat);
		break;
	}
	return FALSE;
}
