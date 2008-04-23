/*
astyle --force-indent=tab=4 --brackets=linux --indent-switches
		--pad=oper --one-line=keep-blocks  --unpad=paren

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

$Id$

*/

#include "../commonheaders.h"
#include "../resource.h"

//#include "../m_MathModule.h"

// externs...
extern PSLWA pSetLayeredWindowAttributes;
extern COLORREF g_ContainerColorKey;
extern StatusItems_t StatusItems[];
extern LRESULT CALLBACK SplitterSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern NEN_OPTIONS nen_options;
extern HRESULT(WINAPI *MyCloseThemeData)(HANDLE);
extern BOOL g_framelessSkinmode;
extern SESSION_INFO g_TabSession;
extern HANDLE g_hEvent_MsgPopup;

//MAD
extern HANDLE hMessageWindowList;

int g_cLinesPerPage=0;
int g_iWheelCarryover=0;
//

extern MYGLOBALS	myGlobals;
extern HBRUSH		hListBkgBrush;
extern HANDLE		hSendEvent;
extern HICON		hIcons[30];
extern struct		CREOleCallback reOleCallback;
extern HMENU		g_hMenu;
extern int        g_sessionshutdown;
extern char*      szWarnClose;
extern WNDPROC OldSplitterProc;

#define DPISCALEX(argX) ((int) ((argX) * myGlobals.g_DPIscaleX))
#define DPISCALEY(argY) ((int) ((argY) * myGlobals.g_DPIscaleY))

static WNDPROC OldMessageProc;
static WNDPROC OldNicklistProc;
static WNDPROC OldFilterButtonProc;
static WNDPROC OldLogProc;
static HKL hkl = NULL;
static HCURSOR hCurHyperlinkHand;

extern PITA pfnIsThemeActive;
extern POTD pfnOpenThemeData;
extern PDTB pfnDrawThemeBackground;
extern PCTD pfnCloseThemeData;
extern PDTT pfnDrawThemeText;
extern PITBPT pfnIsThemeBackgroundPartiallyTransparent;
extern PDTPB  pfnDrawThemeParentBackground;
extern PGTBCR pfnGetThemeBackgroundContentRect;

typedef struct {
	time_t lastEnterTime;
	TCHAR  szTabSave[20];
} MESSAGESUBDATA;


static const CLSID IID_ITextDocument= { 0x8CC497C0,0xA1DF,0x11CE, { 0x80,0x98, 0x00,0xAA, 0x00,0x47,0xBE,0x5D} };


/*
 * checking if theres's protected text at the point
 * emulates EN_LINK WM_NOTIFY to parent to process links
 */
static BOOL CheckCustomLink(HWND hwndDlg, POINT* ptClient, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL bUrlNeeded)
{
	long res = 0, cnt = 0;
	long cpMin = 0, cpMax = 0;
	POINT ptEnd = {0};
	IRichEditOle* RichEditOle = NULL;
	ITextDocument* TextDocument = NULL;
	ITextRange* TextRange = NULL;
	ITextFont* TextFont = NULL;
	BOOL bIsCustomLink = FALSE;

	POINT pt = *ptClient;
	ClientToScreen(hwndDlg, &pt);

	do  {
		if (!SendMessage(hwndDlg, EM_GETOLEINTERFACE, 0, (LPARAM)&RichEditOle)) break;
		if (RichEditOle->lpVtbl->QueryInterface(RichEditOle, &IID_ITextDocument, (void**)&TextDocument) != S_OK) break;
		if (TextDocument->lpVtbl->RangeFromPoint(TextDocument, pt.x, pt.y, &TextRange) != S_OK) break;

		TextRange->lpVtbl->GetStart(TextRange,&cpMin);
		cpMax = cpMin+1;
		TextRange->lpVtbl->SetEnd(TextRange,cpMax);

		if (TextRange->lpVtbl->GetFont(TextRange, &TextFont) != S_OK) break;

		TextFont->lpVtbl->GetProtected(TextFont, &res);
		if (res != tomTrue) break;

		TextRange->lpVtbl->GetPoint(TextRange, tomEnd+TA_BOTTOM+TA_RIGHT, &ptEnd.x, &ptEnd.y);
		if (pt.x > ptEnd.x || pt.y > ptEnd.y) break;

		if (bUrlNeeded) {
			TextRange->lpVtbl->GetStoryLength(TextRange, &cnt);
			for (; cpMin > 0; cpMin--) {
				res = tomTrue;
				TextRange->lpVtbl->SetIndex(TextRange, tomCharacter, cpMin+1, tomTrue);
				TextFont->lpVtbl->GetProtected(TextFont, &res);
				if (res != tomTrue) { cpMin++; break; }
			}
			for (cpMax--; cpMax < cnt; cpMax++) {
				res = tomTrue;
				TextRange->lpVtbl->SetIndex(TextRange, tomCharacter, cpMax+1, tomTrue);
				TextFont->lpVtbl->GetProtected(TextFont, &res);
				if (res != tomTrue) break;
			}
		}

		bIsCustomLink = (cpMin < cpMax);
		} while(FALSE);

	if (TextFont) TextFont->lpVtbl->Release(TextFont);
	if (TextRange) TextRange->lpVtbl->Release(TextRange);
	if (TextDocument) TextDocument->lpVtbl->Release(TextDocument);
	if (RichEditOle) RichEditOle->lpVtbl->Release(RichEditOle);

	if (bIsCustomLink) {
		ENLINK enlink = {0};
		enlink.nmhdr.hwndFrom = hwndDlg;
		enlink.nmhdr.idFrom = IDC_CHAT_LOG;
		enlink.nmhdr.code = EN_LINK;
		enlink.msg = uMsg;
		enlink.wParam = wParam;
		enlink.lParam = lParam;
		enlink.chrg.cpMin = cpMin;
		enlink.chrg.cpMax = cpMax;	
		SendMessage(GetParent(hwndDlg), WM_NOTIFY, (WPARAM)IDC_CHAT_LOG, (LPARAM)&enlink);
	}
	return bIsCustomLink;
}




static BOOL IsStringValidLink(TCHAR* pszText)
{
	TCHAR *p = pszText;

	if (pszText == NULL)
		return FALSE;
	if (lstrlen(pszText) < 5)
		return FALSE;

	while (*p) {
		if (*p == (TCHAR)'"')
			return FALSE;
		p++;
	}
	if (_totlower(pszText[0]) == 'w' && _totlower(pszText[1]) == 'w' && _totlower(pszText[2]) == 'w' && pszText[3] == '.' && _istalnum(pszText[4]))
		return TRUE;

	return(_tcsstr(pszText, _T("://")) == NULL ? FALSE : TRUE);
}

/*
 * called whenever a group chat tab becomes active (either by switching tabs or activating a
 * container window
 */

static void Chat_UpdateWindowState(HWND hwndDlg, struct MessageWindowData *dat, UINT msg)
{
	HWND hwndTab = GetParent(hwndDlg);
	SESSION_INFO *si = dat->si;

	if (dat == NULL)
		return;

	if (msg == WM_ACTIVATE) {
		if (dat->pContainer->dwFlags & CNT_TRANSPARENCY && pSetLayeredWindowAttributes != NULL && !dat->pContainer->bSkinned) {
			DWORD trans = LOWORD(dat->pContainer->dwTransparency);
			pSetLayeredWindowAttributes(dat->pContainer->hwnd, g_ContainerColorKey, (BYTE)trans, (dat->pContainer->bSkinned ? LWA_COLORKEY : 0) | (dat->pContainer->dwFlags & CNT_TRANSPARENCY ? LWA_ALPHA : 0));
		}
	}

	if (dat->pContainer->hwndSaved == hwndDlg)
		return;

	dat->pContainer->hwndSaved = hwndDlg;

	SetActiveSession(si->ptszID, si->pszModule);
	dat->hTabIcon = dat->hTabStatusIcon;

	if (dat->iTabID >= 0) {
		if (DBGetContactSettingWord(si->hContact, si->pszModule , "ApparentMode", 0) != 0)
			DBWriteContactSettingWord(si->hContact, si->pszModule , "ApparentMode", (LPARAM) 0);
		if (CallService(MS_CLIST_GETEVENT, (WPARAM)si->hContact, (LPARAM)0))
			CallService(MS_CLIST_REMOVEEVENT, (WPARAM)si->hContact, (LPARAM)szChatIconString);

		SendMessage(hwndDlg, GC_UPDATETITLE, 0, 1);
		dat->dwTickLastEvent = 0;
		dat->dwFlags &= ~MWF_DIVIDERSET;
		if (KillTimer(hwndDlg, TIMERID_FLASHWND) || dat->iFlashIcon) {
			FlashTab(dat, hwndTab, dat->iTabID, &dat->bTabFlash, FALSE, dat->hTabIcon);
			dat->mayFlashTab = FALSE;
			dat->iFlashIcon = 0;
		}
		if (dat->pContainer->dwFlashingStarted != 0) {
			FlashContainer(dat->pContainer, 0, 0);
			dat->pContainer->dwFlashingStarted = 0;
		}
		dat->pContainer->dwFlags &= ~CNT_NEED_UPDATETITLE;

		if (dat->dwFlags & MWF_NEEDCHECKSIZE)
			PostMessage(hwndDlg, DM_SAVESIZE, 0, 0);

		if (myGlobals.m_AutoLocaleSupport && dat->hContact != 0) {
			if (dat->hkl == 0)
				DM_LoadLocale(hwndDlg, dat);
			PostMessage(hwndDlg, DM_SETLOCALE, 0, 0);
		}
		SetFocus(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE));
		dat->dwLastActivity = dat->dwLastUpdate = GetTickCount();
		dat->pContainer->dwLastActivity = dat->dwLastActivity;
		UpdateContainerMenu(hwndDlg, dat);
		UpdateTrayMenuState(dat, FALSE);
		DM_SetDBButtonStates(hwndDlg, dat);

		if (g_Settings.MathMod) {
			CallService(MTH_Set_ToolboxEditHwnd, 0, (LPARAM)GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE));
			MTH_updateMathWindow(hwndDlg, dat);
		}

		if (dat->dwFlagsEx & MWF_EX_DELAYEDSPLITTER) {
			dat->dwFlagsEx &= ~MWF_EX_DELAYEDSPLITTER;
			ShowWindow(dat->pContainer->hwnd, SW_RESTORE);
			PostMessage(hwndDlg, DM_SPLITTERMOVEDGLOBAL, dat->wParam, dat->lParam);
			PostMessage(hwndDlg, WM_SIZE, 0, 0);
			dat->wParam = dat->lParam = 0;
		}
	}
	//mad
	BB_SetButtonsPos(hwndDlg,dat);
	//
}


/*
 * initialize button bar, set all the icons and ensure proper button state

 */

static void	InitButtons(HWND hwndDlg, SESSION_INFO* si)
{
	BOOL isFlat = DBGetContactSettingByte(NULL, SRMSGMOD_T, "tbflat", 0);
	BOOL isThemed = !DBGetContactSettingByte(NULL, SRMSGMOD_T, "nlflat", 0);
	MODULEINFO *pInfo = si ? MM_FindModule(si->pszModule) : NULL;
	BOOL bNicklistEnabled = si ? si->bNicklistEnabled : FALSE;
	BOOL bFilterEnabled = si ? si->bFilterEnabled : FALSE;

	int i = 0;

	SendDlgItemMessage(hwndDlg, IDC_SHOWNICKLIST, BM_SETIMAGE, IMAGE_ICON, bNicklistEnabled ? (LPARAM)myGlobals.g_buttonBarIcons[36] : (LPARAM)myGlobals.g_buttonBarIcons[35]);
	SendDlgItemMessage(hwndDlg, IDC_FILTER, BM_SETIMAGE, IMAGE_ICON, bFilterEnabled ? (LPARAM)myGlobals.g_buttonBarIcons[34] : (LPARAM)myGlobals.g_buttonBarIcons[33]);
	SendDlgItemMessage(hwndDlg, IDC_CHAT_TOGGLESIDEBAR, BM_SETIMAGE, IMAGE_ICON, (LPARAM)myGlobals.g_buttonBarIcons[25]);

	if (pInfo) {
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_BOLD), pInfo->bBold);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ITALICS), pInfo->bItalics);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_UNDERLINE), pInfo->bUnderline);
		EnableWindow(GetDlgItem(hwndDlg, IDC_COLOR), pInfo->bColor);
		EnableWindow(GetDlgItem(hwndDlg, IDC_BKGCOLOR), pInfo->bBkgColor);
		if (si->iType == GCW_CHATROOM)
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHANMGR), pInfo->bChanMgr);
	}

}

static int splitterEdges = FALSE;

static UINT _toolbarCtrls[] = { IDC_SMILEYBTN, IDC_CHAT_BOLD, IDC_CHAT_UNDERLINE, IDC_ITALICS, IDC_COLOR, IDC_BKGCOLOR,
								IDC_CHAT_HISTORY, IDC_SHOWNICKLIST, IDC_FILTER, IDC_CHANMGR, IDOK, IDC_CHAT_CLOSE, 0
							  };

/*
 * resizer callback for the group chat session window. Called from Mirandas dialog
 * resizing service
 */

static int RoomWndResize(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL *urc)
{
	RECT rc, rcTabs;
	SESSION_INFO* si = (SESSION_INFO*)lParam;
	struct      MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwndDlg, GWL_USERDATA);
	int			TabHeight;
	BOOL		bToolbar = !(dat->pContainer->dwFlags & CNT_HIDETOOLBAR);
	BOOL		bBottomToolbar = dat->pContainer->dwFlags & CNT_BOTTOMTOOLBAR ? 1 : 0;

	BOOL		bNick = si->iType != GCW_SERVER && si->bNicklistEnabled;
	BOOL		bTabs = 0;
	BOOL		bTabBottom = 0;
	int         i = 0;
	static      int msgBottom = 0, msgTop = 0;

	rc.bottom = rc.top = rc.left = rc.right = 0;

	GetClientRect(hwndDlg, &rcTabs);
	TabHeight = rcTabs.bottom - rcTabs.top;

	ShowWindow(GetDlgItem(hwndDlg, IDC_SMILEYBTN), myGlobals.g_SmileyAddAvail/* && bToolbar */? SW_SHOW : SW_HIDE);

	if (si->iType != GCW_SERVER) {
		ShowWindow(GetDlgItem(hwndDlg, IDC_LIST), si->bNicklistEnabled ? SW_SHOW : SW_HIDE);
		ShowWindow(GetDlgItem(hwndDlg, IDC_SPLITTERX), si->bNicklistEnabled ? SW_SHOW : SW_HIDE);

		EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWNICKLIST), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_FILTER), TRUE);
		if (si->iType == GCW_CHATROOM)
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHANMGR), MM_FindModule(si->pszModule)->bChanMgr);
	} else {
		ShowWindow(GetDlgItem(hwndDlg, IDC_LIST), SW_HIDE);
		ShowWindow(GetDlgItem(hwndDlg, IDC_SPLITTERX), SW_HIDE);
	}

	if (si->iType == GCW_SERVER) {
		EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWNICKLIST), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_FILTER), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHANMGR), FALSE);
	}
	ShowWindow(GetDlgItem(hwndDlg, IDC_CHAT_TOGGLESIDEBAR), myGlobals.m_SideBarEnabled ? SW_SHOW : SW_HIDE);

	switch (urc->wId) {
		case IDC_CHAT_LOG:
			urc->rcItem.top = bTabs ? (bTabBottom ? 0 : rcTabs.top - 1) : 0;
			urc->rcItem.left = 0;
			urc->rcItem.right = bNick ? urc->dlgNewSize.cx - si->iSplitterX : urc->dlgNewSize.cx;
			urc->rcItem.bottom = (bToolbar&&!bBottomToolbar) ? (urc->dlgNewSize.cy - si->iSplitterY - DPISCALEY(23)) : (urc->dlgNewSize.cy - si->iSplitterY - DPISCALEY(2));
			//if (!splitterEdges)
			//	urc->rcItem.bottom += 2;
			if (dat->pContainer->bSkinned) {
				StatusItems_t *item = &StatusItems[ID_EXTBKHISTORY];
				if (!item->IGNORED) {
					urc->rcItem.left += item->MARGIN_LEFT;
					urc->rcItem.right -= item->MARGIN_RIGHT;
					urc->rcItem.top += item->MARGIN_TOP;
					urc->rcItem.bottom -= item->MARGIN_BOTTOM;
				}
			}
			return RD_ANCHORX_CUSTOM | RD_ANCHORY_CUSTOM;

		case IDC_LIST:
			urc->rcItem.top = bTabs ? (bTabBottom ? 0 : rcTabs.top - 1) : 0;
			urc->rcItem.right = urc->dlgNewSize.cx ;
			urc->rcItem.left = urc->dlgNewSize.cx - si->iSplitterX + 2;
			urc->rcItem.bottom = (bToolbar&&!bBottomToolbar) ? (urc->dlgNewSize.cy - si->iSplitterY - DPISCALEY(23)) : (urc->dlgNewSize.cy - si->iSplitterY - DPISCALEY(2));
			//if (!splitterEdges)
			//	urc->rcItem.bottom += 2;
			if (dat->pContainer->bSkinned) {
				StatusItems_t *item = &StatusItems[ID_EXTBKUSERLIST];
				if (!item->IGNORED) {
					urc->rcItem.left += item->MARGIN_LEFT;
					urc->rcItem.right -= item->MARGIN_RIGHT;
					urc->rcItem.top += item->MARGIN_TOP;
					urc->rcItem.bottom -= item->MARGIN_BOTTOM;
				}
			}
			return RD_ANCHORX_CUSTOM | RD_ANCHORY_CUSTOM;

		case IDC_SPLITTERX:
			urc->rcItem.right = urc->dlgNewSize.cx - si->iSplitterX + 2;
			urc->rcItem.left = urc->dlgNewSize.cx - si->iSplitterX;
			urc->rcItem.bottom = (bToolbar&&!bBottomToolbar) ? (urc->dlgNewSize.cy - si->iSplitterY - DPISCALEY(23)) : (urc->dlgNewSize.cy - si->iSplitterY - DPISCALEY(2));
			urc->rcItem.top = bTabs ? rcTabs.top : 1;
			return RD_ANCHORX_CUSTOM | RD_ANCHORY_CUSTOM;

		case IDC_SPLITTERY:
			urc->rcItem.right = urc->dlgNewSize.cx;
			urc->rcItem.top = (bToolbar&&!bBottomToolbar) ? urc->dlgNewSize.cy - si->iSplitterY : urc->dlgNewSize.cy - si->iSplitterY;
			urc->rcItem.bottom = (bToolbar&&!bBottomToolbar) ? (urc->dlgNewSize.cy - si->iSplitterY + DPISCALEY(2)) : (urc->dlgNewSize.cy - si->iSplitterY + DPISCALEY(2));
			if (myGlobals.m_SideBarEnabled)
				urc->rcItem.left = 9;
			else
				urc->rcItem.left = 0;
			urc->rcItem.bottom++;
			urc->rcItem.top++;
			return RD_ANCHORX_CUSTOM | RD_ANCHORY_CUSTOM;

		case IDC_CHAT_MESSAGE:
			urc->rcItem.right = urc->dlgNewSize.cx ;
			urc->rcItem.top = urc->dlgNewSize.cy - si->iSplitterY + 3;
			urc->rcItem.bottom = urc->dlgNewSize.cy; // - 1 ;
			msgBottom = urc->rcItem.bottom;
			msgTop = urc->rcItem.top;
			if (myGlobals.m_SideBarEnabled)
				urc->rcItem.left += 9;
			//mad
			if (bBottomToolbar&&bToolbar)
				urc->rcItem.bottom -= DPISCALEY(24);
			//
			if (dat->pContainer->bSkinned) {
				StatusItems_t *item = &StatusItems[ID_EXTBKINPUTAREA];
				if (!item->IGNORED) {
					urc->rcItem.left += item->MARGIN_LEFT;
					urc->rcItem.right -= item->MARGIN_RIGHT;
					urc->rcItem.top += item->MARGIN_TOP;
					urc->rcItem.bottom -= item->MARGIN_BOTTOM;
				}
			}
			return RD_ANCHORX_CUSTOM | RD_ANCHORY_CUSTOM;

		case IDC_CHAT_TOGGLESIDEBAR:
			urc->rcItem.right = 8;
			urc->rcItem.left = 0;
			urc->rcItem.bottom = msgBottom;
			urc->rcItem.top = msgTop;
			return RD_ANCHORX_CUSTOM | RD_ANCHORY_CUSTOM;

	}

	return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;
}


/*
 * subclassing for the message input control (a richedit text control)
 */

static LRESULT CALLBACK MessageSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	MESSAGESUBDATA *dat;
	SESSION_INFO* Parentsi;
	struct MessageWindowData *mwdat;
	HWND hwndParent = GetParent(hwnd);

	mwdat = (struct MessageWindowData *)GetWindowLong(hwndParent, GWL_USERDATA);
	Parentsi = mwdat->si;

	dat = (MESSAGESUBDATA *) GetWindowLong(hwnd, GWL_USERDATA);
	switch (msg) {
		case WM_NCCALCSIZE:
			return(NcCalcRichEditFrame(hwnd, mwdat, ID_EXTBKINPUTAREA, msg, wParam, lParam, OldMessageProc));

		case WM_NCPAINT:
			return(DrawRichEditFrame(hwnd, mwdat, ID_EXTBKINPUTAREA, msg, wParam, lParam, OldMessageProc));

		case EM_SUBCLASSED:
			dat = (MESSAGESUBDATA *) mir_alloc(sizeof(MESSAGESUBDATA));

			SetWindowLong(hwnd, GWL_USERDATA, (LONG) dat);
			dat->szTabSave[0] = '\0';
			dat->lastEnterTime = 0;
			return 0;

		case WM_CONTEXTMENU: {
			HMENU hMenu, hSubMenu;
			CHARRANGE sel, all = { 0, -1};
			int iSelection;
			int iPrivateBG = DBGetContactSettingByte(mwdat->hContact, SRMSGMOD_T, "private_bg", 0);
			MessageWindowPopupData mwpd;
			POINT pt;
			int idFrom = IDC_CHAT_MESSAGE;

			GetCursorPos(&pt);
			hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_CONTEXT));
			hSubMenu = GetSubMenu(hMenu, 2);
			RemoveMenu(hSubMenu, 9, MF_BYPOSITION);
			RemoveMenu(hSubMenu, 8, MF_BYPOSITION);
			RemoveMenu(hSubMenu, 4, MF_BYPOSITION);
			EnableMenuItem(hSubMenu, IDM_PASTEFORMATTED, MF_BYCOMMAND | (mwdat->SendFormat != 0 ? MF_ENABLED : MF_GRAYED));
			CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hSubMenu, 0);

			SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM) & sel);
			if (sel.cpMin == sel.cpMax) {
				EnableMenuItem(hSubMenu, IDM_COPY, MF_BYCOMMAND | MF_GRAYED);
				if (idFrom == IDC_CHAT_MESSAGE)
					EnableMenuItem(hSubMenu, IDM_CUT, MF_BYCOMMAND | MF_GRAYED);
			}
			if (idFrom == IDC_CHAT_MESSAGE) {
				// First notification
				mwpd.cbSize = sizeof(mwpd);
				mwpd.uType = MSG_WINDOWPOPUP_SHOWING;
				mwpd.uFlags = (idFrom == IDC_LOG ? MSG_WINDOWPOPUP_LOG : MSG_WINDOWPOPUP_INPUT);
				mwpd.hContact = mwdat->hContact;
				mwpd.hwnd = hwnd;
				mwpd.hMenu = hSubMenu;
				mwpd.selection = 0;
				mwpd.pt = pt;
				NotifyEventHooks(g_hEvent_MsgPopup, 0, (LPARAM)&mwpd);
			}

			iSelection = TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, GetParent(hwnd), NULL);

			if (idFrom == IDC_CHAT_MESSAGE) {
				// Second notification
				mwpd.selection = iSelection;
				mwpd.uType = MSG_WINDOWPOPUP_SELECTED;
				NotifyEventHooks(g_hEvent_MsgPopup, 0, (LPARAM)&mwpd);
			}

			switch (iSelection) {
				case IDM_COPY:
					SendMessage(hwnd, WM_COPY, 0, 0);
					break;
				case IDM_CUT:
					SendMessage(hwnd, WM_CUT, 0, 0);
					break;
				case IDM_PASTE:
				case IDM_PASTEFORMATTED:
					if (idFrom == IDC_CHAT_MESSAGE)
						SendMessage(hwnd, EM_PASTESPECIAL, (iSelection == IDM_PASTE) ? CF_TEXT : 0, 0);
					break;
				case IDM_COPYALL:
					SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM) & all);
					SendMessage(hwnd, WM_COPY, 0, 0);
					SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM) & sel);
					break;
				case IDM_SELECTALL:
					SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM) & all);
					break;
			}
			DestroyMenu(hMenu);
			return TRUE;
		}

		case WM_MOUSEWHEEL: {
			LRESULT result = DM_MouseWheelHandler(hwnd, hwndParent, mwdat, wParam, lParam);
			if (result == 0)
				return 0;

			dat->lastEnterTime = 0;
			break;
		}

		case EM_REPLACESEL:
			PostMessage(hwnd, EM_ACTIVATE, 0, 0);
			break;

		case EM_ACTIVATE:
			SetActiveWindow(hwndParent);
			break;

		case WM_SYSCHAR: {
			HWND hwndDlg = hwndParent;
			BOOL isShift = GetKeyState(VK_SHIFT) & 0x8000;
			BOOL isCtrl = GetKeyState(VK_CONTROL) & 0x8000;
			BOOL isMenu = GetKeyState(VK_MENU) & 0x8000;

			if ((wParam >= '0' && wParam <= '9') && isMenu) {       // ALT-1 -> ALT-0 direct tab selection
				BYTE bChar = (BYTE)wParam;
				int iIndex;

				if (bChar == '0')
					iIndex = 10;
				else
					iIndex = bChar - (BYTE)'0';
				SendMessage(mwdat->pContainer->hwnd, DM_SELECTTAB, DM_SELECT_BY_INDEX, (LPARAM)iIndex);
				return 0;
			}
			if (isMenu && !isShift && !isCtrl) {
				switch (LOBYTE(VkKeyScan((TCHAR)wParam))) {
					case 'S':
						if (!(GetWindowLong(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE), GWL_STYLE) & ES_READONLY)) {
							PostMessage(hwndDlg, WM_COMMAND, IDOK, 0);
							return 0;
						}
					case'E':
						SendMessage(hwndDlg, WM_COMMAND, IDC_SMILEYBTN, 0);
						return 0;
					case 'H':
						SendMessage(hwndDlg, WM_COMMAND, IDC_CHAT_HISTORY, 0);
						return 0;
					case 'T':
						SendMessage(hwndDlg, WM_COMMAND, IDC_TOGGLETOOLBAR, 0);
						return 0;
				}
			}
		}
		break;

		case WM_CHAR: {
			BOOL isShift = GetKeyState(VK_SHIFT) & 0x8000;
			BOOL isCtrl = GetKeyState(VK_CONTROL) & 0x8000;
			BOOL isMenu = GetKeyState(VK_MENU) & 0x8000;

			//MAD: sound on typing..						 
			if(myGlobals.g_bSoundOnTyping&&!isMenu&&!isCtrl&&!(mwdat->pContainer->dwFlags&CNT_NOSOUND)&&wParam!=VK_ESCAPE&&!(wParam==VK_TAB&&myGlobals.m_AllowTab)) 
				SkinPlaySound("SoundOnTyping");
			//MAD

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
				for (i = 0; i < iLen; i++)
					SendMessage(hwnd, WM_KEYDOWN, mwdat->dwFlags & MWF_LOG_RTL ? VK_RIGHT : VK_LEFT, 0);
				return 0;
			}

			if (GetWindowLong(hwnd, GWL_STYLE) & ES_READONLY)
				break;

			if (wParam == 9 && isCtrl && !isMenu) // ctrl-i (italics)
				return TRUE;

			if (wParam == VK_SPACE && isCtrl && !isMenu) // ctrl-space (paste clean text)
				return TRUE;

			if (wParam == '\n' || wParam == '\r') {
				if (isShift) {
					if (!myGlobals.m_SendOnShiftEnter)
						break;

					PostMessage(hwndParent, WM_COMMAND, IDOK, 0);
					return 0;
				}
				if ((isCtrl && !isShift) ^(0 != myGlobals.m_SendOnEnter)) {
					PostMessage(hwndParent, WM_COMMAND, IDOK, 0);
					return 0;
				}
				if (myGlobals.m_SendOnEnter || myGlobals.m_SendOnDblEnter) {
					if (isCtrl)
						break;
					else {
						if (myGlobals.m_SendOnDblEnter) {
							if (dat->lastEnterTime + 2 < time(NULL)) {
								dat->lastEnterTime = time(NULL);
								break;
							} else {
								SendMessage(hwnd, WM_KEYDOWN, VK_BACK, 0);
								SendMessage(hwnd, WM_KEYUP, VK_BACK, 0);
								PostMessage(hwndParent, WM_COMMAND, IDOK, 0);
								return 0;
							}
						}

						PostMessage(hwndParent, WM_COMMAND, IDOK, 0);
						return 0;
					}
				} else break;
			} else dat->lastEnterTime = 0;

			if (wParam == 1 && isCtrl && !isMenu) {      //ctrl-a
				SendMessage(hwnd, EM_SETSEL, 0, -1);
				return 0;
			}
			break;
		}
		case WM_KEYDOWN: {
			static int start, end;
			BOOL isShift = GetKeyState(VK_SHIFT) & 0x8000;
			BOOL isCtrl = GetKeyState(VK_CONTROL) & 0x8000;
			BOOL isAlt = GetKeyState(VK_MENU) & 0x8000;
			//MAD: sound on typing..
			if(myGlobals.g_bSoundOnTyping&&!isAlt&&wParam == VK_DELETE) 
				SkinPlaySound("SoundOnTyping");
			//
			if (wParam == VK_INSERT && !isShift && !isCtrl && !isAlt) {
				mwdat->fInsertMode = !mwdat->fInsertMode;
				SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), EN_CHANGE), (LPARAM) hwnd);
			}
			if (wParam == VK_CAPITAL || wParam == VK_NUMLOCK)
				SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), EN_CHANGE), (LPARAM) hwnd);

			if (isCtrl && isAlt && !isShift) {
				switch (wParam) {
					case VK_UP:
					case VK_DOWN:
					case VK_PRIOR:
					case VK_NEXT:
					case VK_HOME:
					case VK_END: {
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
							DM_ScrollToBottom(hwndParent, mwdat, 0, 0);
							return 0;
						} else if (wParam == VK_DOWN)
							wp = MAKEWPARAM(SB_LINEDOWN, 0);

						SendMessage(GetDlgItem(hwndParent, IDC_CHAT_LOG), WM_VSCROLL, wp, 0);
						return 0;
					}
				}
			}

			if (wParam == VK_RETURN) {
				dat->szTabSave[0] = '\0';

				if (isShift) {
					if (myGlobals.m_SendOnShiftEnter) {
						PostMessage(hwndParent, WM_COMMAND, IDOK, 0);
						return 0;
					} else
						break;
				}

				if (((isCtrl) != 0) ^(0 != myGlobals.m_SendOnEnter))
					return 0;

				if (myGlobals.m_SendOnDblEnter)
					if (dat->lastEnterTime + 2 >= time(NULL))
						return 0;

				break;
			}
// 			if(wParam == VK_TAB&&myGlobals.m_AllowTab) {
// 				SendMessage(hwnd, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)"\t"); 
// 				return TRUE;
// 			}

			if (wParam == VK_TAB && isShift && !isCtrl) { // SHIFT-TAB (go to nick list)
				SetFocus(GetDlgItem(hwndParent, IDC_LIST));
				return TRUE;
			}

			if ((wParam == VK_NEXT && isCtrl && !isShift) || (wParam == VK_TAB && isCtrl && !isShift)) { // CTRL-TAB (switch tab/window)
				SendMessage(mwdat->pContainer->hwnd, DM_SELECTTAB, DM_SELECT_NEXT, 0);
				return TRUE;
			}

			if ((wParam == VK_PRIOR && isCtrl && !isShift) || (wParam == VK_TAB && isCtrl && isShift)) { // CTRL_SHIFT-TAB (switch tab/window)
				SendMessage(mwdat->pContainer->hwnd, DM_SELECTTAB, DM_SELECT_PREV, 0);
				return TRUE;
			}

			if (wParam == VK_TAB && !isCtrl && !isShift) {    //tab-autocomplete
				TCHAR* pszText = NULL;
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
				iLen = SendMessage(hwnd, EM_GETTEXTLENGTHEX, (WPARAM) & gtl, (LPARAM)NULL);
				if (iLen > 0) {
					TCHAR *pszName = NULL;
					TCHAR *pszSelName = NULL;
					pszText = mir_alloc(sizeof(TCHAR) * (iLen + 100));

					gt.cb = iLen + 99;
					gt.flags = GT_DEFAULT;
#if defined( _UNICODE )
					gt.codepage = 1200;
#else
					gt.codepage = CP_ACP;
#endif

					SendMessage(hwnd, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)pszText);
					while (start > 0 && pszText[start-1] != ' ' && pszText[start-1] != 13 && pszText[start-1] != VK_TAB)
						start--;
					while (end < iLen && pszText[end] != ' ' && pszText[end] != 13 && pszText[end-1] != VK_TAB)
						end ++;

					if (dat->szTabSave[0] == '\0')
						lstrcpyn(dat->szTabSave, pszText + start, end - start + 1);

					pszSelName = mir_alloc(sizeof(TCHAR) * (end - start + 1));
					lstrcpyn(pszSelName, pszText + start, end - start + 1);
					pszName = UM_FindUserAutoComplete(Parentsi->pUsers, dat->szTabSave, pszSelName);
					if (pszName == NULL) {
						pszName = dat->szTabSave;
						SendMessage(hwnd, EM_SETSEL, start, end);
						if (end != start)
							SendMessage(hwnd, EM_REPLACESEL, FALSE, (LPARAM) pszName);
						dat->szTabSave[0] = '\0';
					} else {
						SendMessage(hwnd, EM_SETSEL, start, end);
						if (end != start)
							SendMessage(hwnd, EM_REPLACESEL, FALSE, (LPARAM) pszName);
					}
					mir_free(pszText);
					mir_free(pszSelName);
				}

				SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
				RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
				return 0;
			} else {
				if (dat->szTabSave[0] != '\0' && wParam != VK_RIGHT && wParam != VK_LEFT
						&& wParam != VK_SPACE && wParam != VK_RETURN && wParam != VK_BACK
						&& wParam != VK_DELETE) {
					if (g_Settings.AddColonToAutoComplete && start == 0)
						SendMessage(hwnd, EM_REPLACESEL, FALSE, (LPARAM) _T(": "));
				}
				dat->szTabSave[0] = '\0';
			}

			if (wParam == VK_F4 && isCtrl && !isAlt) { // ctrl-F4 (close tab)
				SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(IDC_CHAT_CLOSE, BN_CLICKED), 0);
				return TRUE;
			}

			if (wParam == 0x49 && isCtrl && !isAlt) { // ctrl-i (italics)
				CheckDlgButton(hwndParent, IDC_ITALICS, IsDlgButtonChecked(hwndParent, IDC_ITALICS) == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED);
				SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(IDC_ITALICS, 0), 0);
				return TRUE;
			}

			if (wParam == 0x42 && isCtrl && !isAlt) { // ctrl-b (bold)
				CheckDlgButton(hwndParent, IDC_CHAT_BOLD, IsDlgButtonChecked(hwndParent, IDC_CHAT_BOLD) == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED);
				SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(IDC_CHAT_BOLD, 0), 0);
				return TRUE;
			}

			if (wParam == 0x55 && isCtrl && !isAlt) { // ctrl-u (paste clean text)
				CheckDlgButton(hwndParent, IDC_CHAT_UNDERLINE, IsDlgButtonChecked(hwndParent, IDC_CHAT_UNDERLINE) == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED);
				SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(IDC_CHAT_UNDERLINE, 0), 0);
				return TRUE;
			}

			if (wParam == 0x4b && isCtrl && !isAlt) { // ctrl-k (paste clean text)
				CheckDlgButton(hwndParent, IDC_COLOR, IsDlgButtonChecked(hwndParent, IDC_COLOR) == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED);
				SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(IDC_COLOR, 0), 0);
				return TRUE;
			}

			if (wParam == VK_SPACE && isCtrl && !isAlt) { // ctrl-space (paste clean text)
				CheckDlgButton(hwndParent, IDC_BKGCOLOR, BST_UNCHECKED);
				CheckDlgButton(hwndParent, IDC_COLOR, BST_UNCHECKED);
				CheckDlgButton(hwndParent, IDC_CHAT_BOLD, BST_UNCHECKED);
				CheckDlgButton(hwndParent, IDC_CHAT_UNDERLINE, BST_UNCHECKED);
				CheckDlgButton(hwndParent, IDC_ITALICS, BST_UNCHECKED);
				SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(IDC_BKGCOLOR, 0), 0);
				SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(IDC_COLOR, 0), 0);
				SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(IDC_CHAT_BOLD, 0), 0);
				SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(IDC_CHAT_UNDERLINE, 0), 0);
				SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(IDC_ITALICS, 0), 0);
				return TRUE;
			}

			if (wParam == 0x4c && isCtrl && !isAlt) { // ctrl-l (paste clean text)
				CheckDlgButton(hwndParent, IDC_BKGCOLOR, IsDlgButtonChecked(hwndParent, IDC_BKGCOLOR) == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED);
				SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(IDC_BKGCOLOR, 0), 0);
				return TRUE;
			}

			if (wParam == 0x46 && isCtrl && !isAlt) { // ctrl-f (paste clean text)
				if (IsWindowEnabled(GetDlgItem(hwndParent, IDC_FILTER)))
					SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(IDC_FILTER, 0), 0);
				return TRUE;
			}

			if (wParam == 0x4e && isCtrl && !isAlt) { // ctrl-n (nicklist)
				if (IsWindowEnabled(GetDlgItem(hwndParent, IDC_SHOWNICKLIST)))
					SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(IDC_SHOWNICKLIST, 0), 0);
				return TRUE;
			}

			if (wParam == 0x48 && isCtrl && !isAlt) { // ctrl-h (history)
				SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(IDC_CHAT_HISTORY, 0), 0);
				return TRUE;
			}

			if (wParam == 0x4f && isCtrl && !isAlt) { // ctrl-o (options)
				if (IsWindowEnabled(GetDlgItem(hwndParent, IDC_CHANMGR)))
					SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(IDC_CHANMGR, 0), 0);
				return TRUE;
			}

			if ((wParam == 45 && isShift || wParam == 0x56 && isCtrl) && !isAlt) { // ctrl-v (paste clean text)
				SendMessage(hwnd, EM_PASTESPECIAL, CF_TEXT, 0);
				return TRUE;
			}

			if (wParam == 0x57 && isCtrl && !isAlt) { // ctrl-w (close window)
				PostMessage(hwndParent, WM_CLOSE, 0, 1);
				return TRUE;
			}

			if (wParam == VK_NEXT || wParam == VK_PRIOR) {
				HWND htemp = hwndParent;
				SendDlgItemMessage(htemp, IDC_CHAT_LOG, msg, wParam, lParam);
				dat->lastEnterTime = 0;
				return TRUE;
			}

			if (wParam == VK_UP && isCtrl && !isAlt) {
				int iLen;
				GETTEXTLENGTHEX gtl = {0};
				SETTEXTEX ste;
				LOGFONTA lf;
				char* lpPrevCmd = SM_GetPrevCommand(Parentsi->ptszID, Parentsi->pszModule);

				SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);

				LoadLogfont(MSGFONTID_MESSAGEAREA, &lf, NULL, FONTMODULE);
				ste.flags = ST_DEFAULT;
				ste.codepage = CP_ACP;
				if (lpPrevCmd)
					SendMessage(hwnd, EM_SETTEXTEX, (WPARAM)&ste, (LPARAM)lpPrevCmd);
				else
					SetWindowText(hwnd, _T(""));

				gtl.flags = GTL_PRECISE;
				gtl.codepage = CP_ACP;
				iLen = SendMessage(hwnd, EM_GETTEXTLENGTHEX, (WPARAM) & gtl, (LPARAM)NULL);
				SendMessage(hwnd, EM_SCROLLCARET, 0, 0);
				SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
				RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
				SendMessage(hwnd, EM_SETSEL, iLen, iLen);
				dat->lastEnterTime = 0;
				return TRUE;
			}

			if (wParam == VK_DOWN && isCtrl && !isAlt) {
				int iLen;
				GETTEXTLENGTHEX gtl = {0};
				SETTEXTEX ste;

				char* lpPrevCmd = SM_GetNextCommand(Parentsi->ptszID, Parentsi->pszModule);
				SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);

				ste.flags = ST_DEFAULT;
				ste.codepage = CP_ACP;
				if (lpPrevCmd)
					SendMessage(hwnd, EM_SETTEXTEX, (WPARAM)&ste, (LPARAM) lpPrevCmd);
				else
					SetWindowText(hwnd, _T(""));

				gtl.flags = GTL_PRECISE;
				gtl.codepage = CP_ACP;
				iLen = SendMessage(hwnd, EM_GETTEXTLENGTHEX, (WPARAM) & gtl, (LPARAM)NULL);
				SendMessage(hwnd, EM_SCROLLCARET, 0, 0);
				SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
				RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
				SendMessage(hwnd, EM_SETSEL, iLen, iLen);
				dat->lastEnterTime = 0;
				return TRUE;
			}

			if (wParam == VK_RETURN)
				break;
			//fall through
		}

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_KILLFOCUS:
			dat->lastEnterTime = 0;
			break;

		case WM_KEYUP:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP: {
			CHARFORMAT2 cf;
			UINT u = 0;
			UINT u2 = 0;
			COLORREF cr;

			LoadLogfont(MSGFONTID_MESSAGEAREA, NULL, &cr, FONTMODULE);

			cf.cbSize = sizeof(CHARFORMAT2);
			cf.dwMask = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_BACKCOLOR | CFM_COLOR;
			SendMessage(hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

			if (MM_FindModule(Parentsi->pszModule) && MM_FindModule(Parentsi->pszModule)->bColor) {
				int index = Chat_GetColorIndex(Parentsi->pszModule, cf.crTextColor);
				u = IsDlgButtonChecked(GetParent(hwnd), IDC_COLOR);

				if (index >= 0) {
					Parentsi->bFGSet = TRUE;
					Parentsi->iFG = index;
				}

				if (u == BST_UNCHECKED && cf.crTextColor != cr)
					CheckDlgButton(hwndParent, IDC_COLOR, BST_CHECKED);
				else if (u == BST_CHECKED && cf.crTextColor == cr)
					CheckDlgButton(hwndParent, IDC_COLOR, BST_UNCHECKED);
			}

			if (MM_FindModule(Parentsi->pszModule) && MM_FindModule(Parentsi->pszModule)->bBkgColor) {
				int index = Chat_GetColorIndex(Parentsi->pszModule, cf.crBackColor);
				COLORREF crB = (COLORREF)DBGetContactSettingDword(NULL, FONTMODULE, "inputbg", GetSysColor(COLOR_WINDOW));
				u = IsDlgButtonChecked(hwndParent, IDC_BKGCOLOR);

				if (index >= 0) {
					Parentsi->bBGSet = TRUE;
					Parentsi->iBG = index;
				}

				if (u == BST_UNCHECKED && cf.crBackColor != crB)
					CheckDlgButton(hwndParent, IDC_BKGCOLOR, BST_CHECKED);
				else if (u == BST_CHECKED && cf.crBackColor == crB)
					CheckDlgButton(hwndParent, IDC_BKGCOLOR, BST_UNCHECKED);
			}

			if (MM_FindModule(Parentsi->pszModule) && MM_FindModule(Parentsi->pszModule)->bBold) {
				u = IsDlgButtonChecked(hwndParent, IDC_CHAT_BOLD);
				u2 = cf.dwEffects;
				u2 &= CFE_BOLD;
				if (u == BST_UNCHECKED && u2)
					CheckDlgButton(hwndParent, IDC_CHAT_BOLD, BST_CHECKED);
				else if (u == BST_CHECKED && u2 == 0)
					CheckDlgButton(hwndParent, IDC_CHAT_BOLD, BST_UNCHECKED);
			}

			if (MM_FindModule(Parentsi->pszModule) && MM_FindModule(Parentsi->pszModule)->bItalics) {
				u = IsDlgButtonChecked(hwndParent, IDC_ITALICS);
				u2 = cf.dwEffects;
				u2 &= CFE_ITALIC;
				if (u == BST_UNCHECKED && u2)
					CheckDlgButton(hwndParent, IDC_ITALICS, BST_CHECKED);
				else if (u == BST_CHECKED && u2 == 0)
					CheckDlgButton(hwndParent, IDC_ITALICS, BST_UNCHECKED);
			}

			if (MM_FindModule(Parentsi->pszModule) && MM_FindModule(Parentsi->pszModule)->bUnderline) {
				u = IsDlgButtonChecked(hwndParent, IDC_CHAT_UNDERLINE);
				u2 = cf.dwEffects;
				u2 &= CFE_UNDERLINE;
				if (u == BST_UNCHECKED && u2)
					CheckDlgButton(hwndParent, IDC_CHAT_UNDERLINE, BST_CHECKED);
				else if (u == BST_CHECKED && u2 == 0)
					CheckDlgButton(hwndParent, IDC_CHAT_UNDERLINE, BST_UNCHECKED);
			}
		}
		break;

		case WM_INPUTLANGCHANGEREQUEST:
			return DefWindowProc(hwnd, WM_INPUTLANGCHANGEREQUEST, wParam, lParam);

		case WM_INPUTLANGCHANGE:
			if (myGlobals.m_AutoLocaleSupport && GetFocus() == hwnd && mwdat->pContainer->hwndActive == hwndParent && GetForegroundWindow() == mwdat->pContainer->hwnd && GetActiveWindow() == mwdat->pContainer->hwnd)
				DM_SaveLocale(hwndParent, mwdat, wParam, lParam);

			return 1;

		case EM_UNSUBCLASSED:
			mir_free(dat);
			return 0;
	}

	return CallWindowProc(OldMessageProc, hwnd, msg, wParam, lParam);
}


/*
 * subclassing for the message filter dialog (set and configure event filters for the current
 * session
 */

static BOOL CALLBACK FilterWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SESSION_INFO * si = (SESSION_INFO *)GetWindowLong(hwndDlg, GWL_USERDATA);
	switch (uMsg) {
		case WM_INITDIALOG: {
			DWORD dwMask, dwFlags;

			si = (SESSION_INFO *)lParam;
			dwMask = DBGetContactSettingDword(si->hContact, "Chat", "FilterMask", 0);
			dwFlags = DBGetContactSettingDword(si->hContact, "Chat", "FilterFlags", 0);
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)si);

			CheckDlgButton(hwndDlg, IDC_1, dwMask & GC_EVENT_ACTION ? (dwFlags & GC_EVENT_ACTION ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_2, dwMask & GC_EVENT_MESSAGE ? (dwFlags & GC_EVENT_MESSAGE ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_3, dwMask & GC_EVENT_NICK ? (dwFlags & GC_EVENT_NICK ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_4, dwMask & GC_EVENT_JOIN ? (dwFlags & GC_EVENT_JOIN ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_5, dwMask & GC_EVENT_PART ? (dwFlags & GC_EVENT_PART ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_6, dwMask & GC_EVENT_TOPIC ? (dwFlags & GC_EVENT_TOPIC ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_7, dwMask & GC_EVENT_ADDSTATUS ? (dwFlags & GC_EVENT_ADDSTATUS ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_8, dwMask & GC_EVENT_INFORMATION ? (dwFlags & GC_EVENT_INFORMATION ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_9, dwMask & GC_EVENT_QUIT ? (dwFlags & GC_EVENT_QUIT ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_10, dwMask & GC_EVENT_KICK ? (dwFlags & GC_EVENT_KICK ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_11, dwMask & GC_EVENT_NOTICE ? (dwFlags & GC_EVENT_NOTICE ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);

			dwMask = DBGetContactSettingDword(si->hContact, "Chat", "PopupMask", 0);
			dwFlags = DBGetContactSettingDword(si->hContact, "Chat", "PopupFlags", 0);
			CheckDlgButton(hwndDlg, IDC_P1, dwMask & GC_EVENT_ACTION ? (dwFlags & GC_EVENT_ACTION ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_P2, dwMask & GC_EVENT_MESSAGE ? (dwFlags & GC_EVENT_MESSAGE ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_P3, dwMask & GC_EVENT_NICK ? (dwFlags & GC_EVENT_NICK ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_P4, dwMask & GC_EVENT_JOIN ? (dwFlags & GC_EVENT_JOIN ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_P5, dwMask & GC_EVENT_PART ? (dwFlags & GC_EVENT_PART ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_P6, dwMask & GC_EVENT_TOPIC ? (dwFlags & GC_EVENT_TOPIC ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_P7, dwMask & GC_EVENT_ADDSTATUS ? (dwFlags & GC_EVENT_ADDSTATUS ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_P8, dwMask & GC_EVENT_INFORMATION ? (dwFlags & GC_EVENT_INFORMATION ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_P9, dwMask & GC_EVENT_QUIT ? (dwFlags & GC_EVENT_QUIT ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_P10, dwMask & GC_EVENT_KICK ? (dwFlags & GC_EVENT_KICK ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_P11, dwMask & GC_EVENT_NOTICE ? (dwFlags & GC_EVENT_NOTICE ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);

			dwMask = DBGetContactSettingDword(si->hContact, "Chat", "TrayIconMask", 0);
			dwFlags = DBGetContactSettingDword(si->hContact, "Chat", "TrayIconFlags", 0);
			CheckDlgButton(hwndDlg, IDC_T1, dwMask & GC_EVENT_ACTION ? (dwFlags & GC_EVENT_ACTION ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_T2, dwMask & GC_EVENT_MESSAGE ? (dwFlags & GC_EVENT_MESSAGE ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_T3, dwMask & GC_EVENT_NICK ? (dwFlags & GC_EVENT_NICK ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_T4, dwMask & GC_EVENT_JOIN ? (dwFlags & GC_EVENT_JOIN ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_T5, dwMask & GC_EVENT_PART ? (dwFlags & GC_EVENT_PART ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_T6, dwMask & GC_EVENT_TOPIC ? (dwFlags & GC_EVENT_TOPIC ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_T7, dwMask & GC_EVENT_ADDSTATUS ? (dwFlags & GC_EVENT_ADDSTATUS ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_T8, dwMask & GC_EVENT_INFORMATION ? (dwFlags & GC_EVENT_INFORMATION ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_T9, dwMask & GC_EVENT_QUIT ? (dwFlags & GC_EVENT_QUIT ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_T10, dwMask & GC_EVENT_KICK ? (dwFlags & GC_EVENT_KICK ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);
			CheckDlgButton(hwndDlg, IDC_T11, dwMask & GC_EVENT_NOTICE ? (dwFlags & GC_EVENT_NOTICE ? BST_CHECKED : BST_UNCHECKED) : BST_INDETERMINATE);

			break;
		}
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
			SetTextColor((HDC)wParam, RGB(60, 60, 150));
			SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
			return (BOOL)GetSysColorBrush(COLOR_WINDOW);

		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_INACTIVE) {
				int iFlags = 0;
				UINT result;
				DWORD dwMask = 0, dwFlags = 0;

				result = IsDlgButtonChecked(hwndDlg, IDC_1);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_ACTION : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_ACTION : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_2);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_MESSAGE : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_MESSAGE : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_3);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_NICK : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_NICK : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_4);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_JOIN : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_JOIN : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_5);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_PART : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_PART : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_6);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_TOPIC : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_TOPIC : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_7);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_ADDSTATUS : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_ADDSTATUS : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_8);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_INFORMATION : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_INFORMATION : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_9);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_QUIT : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_QUIT : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_10);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_KICK : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_KICK : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_11);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_NOTICE : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_NOTICE : 0);

				if (iFlags&GC_EVENT_ADDSTATUS)
					iFlags |= GC_EVENT_REMOVESTATUS;

				if (si) {
					if (dwMask == 0) {
						DBDeleteContactSetting(si->hContact, "Chat", "FilterFlags");
						DBDeleteContactSetting(si->hContact, "Chat", "FilterMask");
					} else {
						DBWriteContactSettingDword(si->hContact, "Chat", "FilterFlags", iFlags);
						DBWriteContactSettingDword(si->hContact, "Chat", "FilterMask", dwMask);
					}
				}

				dwMask = iFlags = 0;

				result = IsDlgButtonChecked(hwndDlg, IDC_P1);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_ACTION : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_ACTION : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_P2);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_MESSAGE : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_MESSAGE : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_P3);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_NICK : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_NICK : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_P4);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_JOIN : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_JOIN : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_P5);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_PART : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_PART : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_P6);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_TOPIC : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_TOPIC : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_P7);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_ADDSTATUS : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_ADDSTATUS : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_P8);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_INFORMATION : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_INFORMATION : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_P9);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_QUIT : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_QUIT : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_P10);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_KICK : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_KICK : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_P11);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_NOTICE : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_NOTICE : 0);

				if (iFlags&GC_EVENT_ADDSTATUS)
					iFlags |= GC_EVENT_REMOVESTATUS;

				if (si) {
					if (dwMask == 0) {
						DBDeleteContactSetting(si->hContact, "Chat", "PopupFlags");
						DBDeleteContactSetting(si->hContact, "Chat", "PopupMask");
					} else {
						DBWriteContactSettingDword(si->hContact, "Chat", "PopupFlags", iFlags);
						DBWriteContactSettingDword(si->hContact, "Chat", "PopupMask", dwMask);
					}
				}

				dwMask = iFlags = 0;

				result = IsDlgButtonChecked(hwndDlg, IDC_T1);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_ACTION : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_ACTION : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_T2);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_MESSAGE : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_MESSAGE : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_T3);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_NICK : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_NICK : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_T4);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_JOIN : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_JOIN : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_T5);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_PART : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_PART : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_T6);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_TOPIC : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_TOPIC : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_T7);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_ADDSTATUS : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_ADDSTATUS : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_T8);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_INFORMATION : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_INFORMATION : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_T9);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_QUIT : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_QUIT : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_T10);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_KICK : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_KICK : 0);

				result = IsDlgButtonChecked(hwndDlg, IDC_T11);
				dwMask |= (result != BST_INDETERMINATE ? GC_EVENT_NOTICE : 0);
				iFlags |= (result == BST_CHECKED ? GC_EVENT_NOTICE : 0);

				if (iFlags&GC_EVENT_ADDSTATUS)
					iFlags |= GC_EVENT_REMOVESTATUS;

				if (si) {
					if (dwMask == 0) {
						DBDeleteContactSetting(si->hContact, "Chat", "TrayIconFlags");
						DBDeleteContactSetting(si->hContact, "Chat", "TrayIconMask");
					} else {
						DBWriteContactSettingDword(si->hContact, "Chat", "TrayIconFlags", iFlags);
						DBWriteContactSettingDword(si->hContact, "Chat", "TrayIconMask", dwMask);
					}
					Chat_SetFilters(si);
					SendMessage(si->hWnd, GC_CHANGEFILTERFLAG, 0, (LPARAM)iFlags);
					if (si->bFilterEnabled)
						SendMessage(si->hWnd, GC_REDRAWLOG, 0, 0);
				}

				PostMessage(hwndDlg, WM_CLOSE, 0, 0);
			}
			break;

		case WM_CLOSE:
			DestroyWindow(hwndDlg);
			break;

		case WM_DESTROY:
			SetWindowLong(hwndDlg, GWL_USERDATA, 0);
			break;
	}
	return(FALSE);
}


static LRESULT CALLBACK ButtonSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndParent = GetParent(hwnd);

	switch (msg) {
		case WM_RBUTTONUP: {
			HWND hFilter = GetDlgItem(hwndParent, IDC_FILTER);
			HWND hColor = GetDlgItem(hwndParent, IDC_COLOR);
			HWND hBGColor = GetDlgItem(hwndParent, IDC_BKGCOLOR);

			if (DBGetContactSettingByte(NULL, "Chat", "RightClickFilter", 0) != 0) {
				if (hFilter == hwnd)
					SendMessage(hwndParent, GC_SHOWFILTERMENU, 0, 0);
				if (hColor == hwnd)
					SendMessage(hwndParent, GC_SHOWCOLORCHOOSER, 0, (LPARAM)IDC_COLOR);
				if (hBGColor == hwnd)
					SendMessage(hwndParent, GC_SHOWCOLORCHOOSER, 0, (LPARAM)IDC_BKGCOLOR);
			}
		}
		break;
	}

	return CallWindowProc(OldFilterButtonProc, hwnd, msg, wParam, lParam);
}


/*
 * subclassing for the message history display (rich edit control in which the chat history appears)
 */

static LRESULT CALLBACK LogSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndParent = GetParent(hwnd);
	struct MessageWindowData *mwdat = (struct MessageWindowData *)GetWindowLong(hwndParent, GWL_USERDATA);

	switch (msg) {
		case WM_NCCALCSIZE:
			return(NcCalcRichEditFrame(hwnd, mwdat, ID_EXTBKHISTORY, msg, wParam, lParam, OldLogProc));

		case WM_NCPAINT:
			return(DrawRichEditFrame(hwnd, mwdat, ID_EXTBKHISTORY, msg, wParam, lParam, OldLogProc));

		case WM_COPY:
			return(DM_WMCopyHandler(hwnd, OldLogProc, wParam, lParam));

		
	case WM_SETCURSOR:
			if (g_Settings.ClickableNicks && (LOWORD(lParam) == HTCLIENT)) {
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hwnd,&pt);
				if (CheckCustomLink(hwnd, &pt, msg, wParam, lParam, FALSE)) return TRUE;
			}
			break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
			if (g_Settings.ClickableNicks) {
				POINT pt={LOWORD(lParam), HIWORD(lParam)};
				CheckCustomLink(hwnd, &pt, msg, wParam, lParam, TRUE);
			}
			break;   

		case WM_LBUTTONUP:
			{
			CHARRANGE sel;
			if (g_Settings.ClickableNicks) {
				POINT pt={LOWORD(lParam), HIWORD(lParam)};
				CheckCustomLink(hwnd, &pt, msg, wParam, lParam, TRUE);
			}  
			SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM) &sel);
			if (sel.cpMin != sel.cpMax) {
				SendMessage(hwnd, WM_COPY, 0, 0);
				sel.cpMin = sel.cpMax ;
				SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM) & sel);
			}
			SetFocus(GetDlgItem(hwndParent, IDC_CHAT_MESSAGE));
		}
		break;

		case WM_KEYDOWN:
			if (wParam == 0x57 && GetKeyState(VK_CONTROL) & 0x8000) { // ctrl-w (close window)
				PostMessage(hwndParent, WM_CLOSE, 0, 1);
				return TRUE;
			}
			break;

		case WM_ACTIVATE: {
			if (LOWORD(wParam) == WA_INACTIVE) {
				CHARRANGE sel;
				SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM) &sel);
				if (sel.cpMin != sel.cpMax) {
					sel.cpMin = sel.cpMax ;
					SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM) & sel);
				}
			}
		}
		break;

		case WM_CHAR:
			SetFocus(GetDlgItem(hwndParent, IDC_CHAT_MESSAGE));
			SendMessage(GetDlgItem(hwndParent, IDC_CHAT_MESSAGE), WM_CHAR, wParam, lParam);
			break;
	}

	return CallWindowProc(OldLogProc, hwnd, msg, wParam, lParam);
}


/*
 * process mouse - hovering for the nickname list. fires events so the protocol can
 * show the userinfo - tooltip.
 */

static void ProcessNickListHovering(HWND hwnd, int hoveredItem, POINT * pt, SESSION_INFO * parentdat)
{
	static int currentHovered = -1;
	static HWND hwndToolTip = NULL;
	static HWND oldParent = NULL;
	TOOLINFO ti = {0};
	RECT clientRect;
	BOOL bNewTip = FALSE;
	USERINFO *ui1 = NULL;

	if (hoveredItem == currentHovered) return;
	currentHovered = hoveredItem;

	if (oldParent != hwnd && hwndToolTip) {
		SendMessage(hwndToolTip, TTM_DELTOOL, 0, 0);
		DestroyWindow(hwndToolTip);
		hwndToolTip = NULL;
	}
	if (hoveredItem == -1) {

		SendMessage(hwndToolTip, TTM_ACTIVATE, 0, 0);

	} else {

		if (!hwndToolTip) {
			hwndToolTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS,  NULL,
										 WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
										 CW_USEDEFAULT, CW_USEDEFAULT,  CW_USEDEFAULT,  CW_USEDEFAULT,
										 hwnd, NULL, g_hInst,  NULL);
			bNewTip = TRUE;
		}

		GetClientRect(hwnd, &clientRect);
		ti.cbSize = sizeof(TOOLINFO);
		ti.uFlags = TTF_SUBCLASS;
		ti.hinst = g_hInst;
		ti.hwnd = hwnd;
		ti.uId = 1;
		ti.rect = clientRect;

		ti.lpszText = NULL;

		ui1 = SM_GetUserFromIndex(parentdat->ptszID, parentdat->pszModule, currentHovered);
		if (ui1) {
			char serviceName[256];
			_snprintf(serviceName, SIZEOF(serviceName), "%s"MS_GC_PROTO_GETTOOLTIPTEXT, parentdat->pszModule);
			if (ServiceExists(serviceName))
				ti.lpszText = (TCHAR*)CallService(serviceName, (WPARAM)parentdat->ptszID, (LPARAM)ui1->pszUID);
		}

		SendMessage(hwndToolTip, bNewTip ? TTM_ADDTOOL : TTM_UPDATETIPTEXT, 0, (LPARAM) &ti);
		SendMessage(hwndToolTip, TTM_ACTIVATE, (ti.lpszText != NULL) , 0);
		SendMessage(hwndToolTip, TTM_SETMAXTIPWIDTH, 0 , 400);
		if (ti.lpszText)
			mir_free(ti.lpszText);
	}
}

/*
 * subclassing for the nickname list control. It is an ownerdrawn listbox
 */

static LRESULT CALLBACK NicklistSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndParent = GetParent(hwnd);
	struct MessageWindowData *mwdat = (struct MessageWindowData *)GetWindowLong(hwndParent, GWL_USERDATA);

	switch (msg) {
		//MAD: attemp to fix weird bug, when combobox with hidden vscroll
		//can't be scrolled with mouse-wheel. 
		case WM_NCCALCSIZE:{
		   if (myGlobals.g_DisableScrollbars&&myGlobals.g_NickListScrollBarFix) {
			RECT lpRect;
			LONG itemHeight;

			GetClientRect (hwnd, &lpRect);
			itemHeight = SendMessage(hwnd, LB_GETITEMHEIGHT, 0, 0);
			g_cLinesPerPage = (lpRect.bottom - lpRect.top) /itemHeight	;
			 }
			return(NcCalcRichEditFrame(hwnd, mwdat, ID_EXTBKUSERLIST, msg, wParam, lParam, OldNicklistProc));
			}
		 //
		case WM_NCPAINT:
			return(DrawRichEditFrame(hwnd, mwdat, ID_EXTBKUSERLIST, msg, wParam, lParam, OldNicklistProc));

		case WM_ERASEBKGND: {
			HDC dc = (HDC)wParam;
			struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwndParent, GWL_USERDATA);
			SESSION_INFO * parentdat = dat->si;
			if (dc) {
				int height, index, items = 0;

				index = SendMessage(hwnd, LB_GETTOPINDEX, 0, 0);
				if (index == LB_ERR || parentdat->nUsersInNicklist <= 0)
					return 0;

				items = parentdat->nUsersInNicklist - index;
				height = SendMessage(hwnd, LB_GETITEMHEIGHT, 0, 0);

				if (height != LB_ERR) {
					RECT rc = {0};
					GetClientRect(hwnd, &rc);

					if (rc.bottom - rc.top > items * height) {
						rc.top = items * height;
						FillRect(dc, &rc, hListBkgBrush);
					}
				}
			}
		}
		return 1;

		//MAD
		case WM_MOUSEWHEEL: {  
			if (myGlobals.g_DisableScrollbars&&myGlobals.g_NickListScrollBarFix)
				{
				UINT uScroll;
				int dLines;
				short zDelta=(short)HIWORD(wParam);
				if (!SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &uScroll, 0)) 
					uScroll = 3;    /* default value */

				if (uScroll == WHEEL_PAGESCROLL)
					uScroll = g_cLinesPerPage;
				if (uScroll == 0)
					return 0;

				zDelta += g_iWheelCarryover;    /* Accumulate wheel motion */


				dLines = zDelta * (int)uScroll / WHEEL_DELTA;


				//Record the unused portion as the next carryover.
				g_iWheelCarryover = zDelta - dLines * WHEEL_DELTA / (int)uScroll;


				// scrolling.
				while (abs(dLines)) {
					if (dLines > 0) {
						SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
						dLines--;
						} else {
							SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
							dLines++;
						}
					}
				return 0;
				}
			}break;
//MAD_
		case WM_KEYDOWN:
			if (wParam == 0x57 && GetKeyState(VK_CONTROL) & 0x8000) { // ctrl-w (close window)
				PostMessage(hwndParent, WM_CLOSE, 0, 1);
				return TRUE;
			}
			if (wParam == VK_ESCAPE || wParam == VK_UP || wParam == VK_DOWN || wParam == VK_NEXT ||
					wParam == VK_PRIOR || wParam == VK_TAB || wParam == VK_HOME || wParam == VK_END) {
				if (mwdat && mwdat->si) {
					SESSION_INFO *si = (SESSION_INFO *)mwdat->si;
					si->szSearch[0] = 0;
					si->iSearchItem = -1;
				}
			}
			break;

		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			if (mwdat && mwdat->si) {                   // set/kill focus invalidates incremental search status
				SESSION_INFO *si = (SESSION_INFO *)mwdat->si;
				si->szSearch[0] = 0;
				si->iSearchItem = -1;
			}
			break;
		case WM_CHAR:
		case WM_UNICHAR: {
			/*
			* simple incremental search for the user (nick) - list control
			* typing esc or movement keys will clear the current search string
			*/

			if (mwdat && mwdat->si) {
				SESSION_INFO *si = (SESSION_INFO *)mwdat->si;
				if (wParam == 27 && si->szSearch[0]) {						// escape - reset everything
					si->szSearch[0] = 0;
					si->iSearchItem = -1;
					break;
				} else if (wParam == '\b' && si->szSearch[0])					// backspace
					si->szSearch[lstrlen(si->szSearch) - 1] = '\0';
				else if (wParam < ' ')
					break;
				else {
					TCHAR szNew[2];
					szNew[0] = (TCHAR) wParam;
					szNew[1] = '\0';
					if (lstrlen(si->szSearch) >= SIZEOF(si->szSearch) - 2) {
						MessageBeep(MB_OK);
						break;
					}
					_tcscat(si->szSearch, szNew);
				}
				if (si->szSearch[0]) {
					int     iItems = SendMessage(hwnd, LB_GETCOUNT, 0, 0);
					int     i;
					USERINFO *ui;

					/*
					* iterate over the (sorted) list of nicknames and search for the
					* string we have
					*/

					for (i = 0; i < iItems; i++) {
						ui = UM_FindUserFromIndex(si->pUsers, i);
						if (ui) {
							if (!_tcsnicmp(ui->pszNick, si->szSearch, lstrlen(si->szSearch))) {
								SendMessage(hwnd, LB_SETSEL, FALSE, -1);
								SendMessage(hwnd, LB_SETSEL, TRUE, i);
								si->iSearchItem = i;
								InvalidateRect(hwnd, NULL, FALSE);
								return 0;
							}
						}
					}
					if (i == iItems) {
						MessageBeep(MB_OK);
						si->szSearch[lstrlen(si->szSearch) - 1] = '\0';
						return 0;
					}
				}
			}
			break;
		}
		case WM_RBUTTONDOWN: {
			int iCounts = SendMessage(hwnd, LB_GETSELCOUNT, 0, 0);

			if (iCounts != LB_ERR && iCounts > 1)
				return 0;
			SendMessage(hwnd, WM_LBUTTONDOWN, wParam, lParam);
			break;
		}

		case WM_RBUTTONUP:
			SendMessage(hwnd, WM_LBUTTONUP, wParam, lParam);
			break;

		case WM_MEASUREITEM: {
			MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *) lParam;
			if (mis->CtlType == ODT_MENU)
				return CallService(MS_CLIST_MENUMEASUREITEM, wParam, lParam);
			return FALSE;
		}
		case WM_DRAWITEM: {
			DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *) lParam;
			if (dis->CtlType == ODT_MENU)
				return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
			return FALSE;
		}
		case WM_CONTEXTMENU: {
			TVHITTESTINFO hti;
			int item;
			int height;
			USERINFO * ui;
			struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwndParent, GWL_USERDATA);
			SESSION_INFO * parentdat = dat->si;


			hti.pt.x = (short) LOWORD(lParam);
			hti.pt.y = (short) HIWORD(lParam);
			if (hti.pt.x == -1 && hti.pt.y == -1) {
				int index = SendMessage(hwnd, LB_GETCURSEL, 0, 0);
				int top = SendMessage(hwnd, LB_GETTOPINDEX, 0, 0);
				height = SendMessage(hwnd, LB_GETITEMHEIGHT, 0, 0);
				hti.pt.x = 4;
				hti.pt.y = (index - top) * height + 1;
			} else
				ScreenToClient(hwnd, &hti.pt);

			item = (DWORD)(SendMessage(hwnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(hti.pt.x, hti.pt.y)));
			if ( HIWORD( item ) == 1 )
				item = (DWORD)(-1);
			else
				item &= 0xFFFF;

			ui = SM_GetUserFromIndex(parentdat->ptszID, parentdat->pszModule, item);
			//			ui = (USERINFO *)SendMessage(GetDlgItem(hwndParent, IDC_LIST), LB_GETITEMDATA, item, 0);
			if (ui) {
				HMENU hMenu = 0;
				UINT uID;
				USERINFO uinew;

				memcpy(&uinew, ui, sizeof(USERINFO));
				if (hti.pt.x == -1 && hti.pt.y == -1)
					hti.pt.y += height - 4;
				ClientToScreen(hwnd, &hti.pt);
				uID = CreateGCMenu(hwnd, &hMenu, 0, hti.pt, parentdat, uinew.pszUID, NULL);

				switch (uID) {
					case 0:
						break;

					case ID_MESS:
						DoEventHookAsync(GetParent(hwnd), parentdat->ptszID, parentdat->pszModule, GC_USER_PRIVMESS, ui->pszUID, NULL, (LPARAM)NULL);
						break;

					default: {
						int iCount = SendMessage(hwnd, LB_GETCOUNT, 0, 0);

						if (iCount != LB_ERR) {
							int iSelectedItems = SendMessage(hwnd, LB_GETSELCOUNT, 0, 0);

							if (iSelectedItems != LB_ERR) {
								int *pItems = (int *)malloc(sizeof(int) * (iSelectedItems + 1));

								if (pItems) {
									if (SendMessage(hwnd, LB_GETSELITEMS, (WPARAM)iSelectedItems, (LPARAM)pItems) != LB_ERR) {
										USERINFO *ui1 = NULL;
										int i;

										for (i = 0; i < iSelectedItems; i++) {
											ui1 = SM_GetUserFromIndex(parentdat->ptszID, parentdat->pszModule, pItems[i]);
											if (ui1)
												DoEventHookAsync(hwndParent, parentdat->ptszID, parentdat->pszModule, GC_USER_NICKLISTMENU, ui1->pszUID, NULL, (LPARAM)uID);
										}
									}
									free(pItems);
								}
							}
						}
						//DoEventHookAsync(hwndParent, parentdat->ptszID, parentdat->pszModule, GC_USER_NICKLISTMENU, ui->pszUID, NULL, (LPARAM)uID);
						break;
					}
				}
				DestroyGCMenu(&hMenu, 1);
				return TRUE;
			}
		}
		break;
		case WM_MOUSEMOVE: {
			POINT pt;
			RECT clientRect;
			BOOL bInClient;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			GetClientRect(hwnd, &clientRect);
			bInClient = PtInRect(&clientRect, pt);
 /*
			//Mouse capturing/releasing
			if (bInClient && GetCapture() != hwnd)
				SetCapture(hwnd);
			else if (!bInClient)
				SetCapture(NULL);
  */
			if (bInClient) {
				//hit test item under mouse
				struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwndParent, GWL_USERDATA);
				SESSION_INFO * parentdat = dat->si;

				DWORD nItemUnderMouse = (DWORD)SendMessage(hwnd, LB_ITEMFROMPOINT, 0, lParam);
				if (HIWORD(nItemUnderMouse) == 1)
					nItemUnderMouse = (DWORD)(-1);
				else
					nItemUnderMouse &= 0xFFFF;

				ProcessNickListHovering(hwnd, (int)nItemUnderMouse, &pt, parentdat);
			} else
				ProcessNickListHovering(hwnd, -1, &pt, NULL);
		}
		break;
	}
	return CallWindowProc(OldNicklistProc, hwnd, msg, wParam, lParam);
}

/*
 * calculate the required rectangle for a string using the given font. This is more
 * precise than using GetTextExtentPoint...()
 */

int GetTextPixelSize(TCHAR* pszText, HFONT hFont, BOOL bWidth)
{
	HDC hdc;
	HFONT hOldFont;
	RECT rc = {0};
	int i;

	if (!pszText || !hFont)
		return 0;

	hdc = GetDC(NULL);
	hOldFont = SelectObject(hdc, hFont);
	i = DrawText(hdc, pszText , -1, &rc, DT_CALCRECT);
	SelectObject(hdc, hOldFont);
	ReleaseDC(NULL, hdc);
	return bWidth ? rc.right - rc.left : rc.bottom - rc.top;
}

struct FORK_ARG {
	HANDLE hEvent;
	void (__cdecl *threadcode)(void*);
	unsigned(__stdcall *threadcodeex)(void*);
	void *arg;
};


/*
 * thread supporting functions. Needed when a large chunk of evetns must be sent to the message
 * history display. This is done in a separate thread to avoid blocking the main thread for
 * longer than absolutely needed.
 */

static void __cdecl forkthread_r(void *param)
{
	struct FORK_ARG *fa = (struct FORK_ARG*)param;
	void (*callercode)(void*) = fa->threadcode;
	void *arg = fa->arg;

	CallService(MS_SYSTEM_THREAD_PUSH, 0, 0);

	SetEvent(fa->hEvent);

	__try {
		callercode(arg);
	} __finally {
		CallService(MS_SYSTEM_THREAD_POP, 0, 0);
	}
}


static unsigned long forkthread(void (__cdecl *threadcode)(void*), unsigned long stacksize, void *arg)
{
	unsigned long rc;
	struct FORK_ARG fa;

	fa.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	fa.threadcode = threadcode;
	fa.arg = arg;

	rc = _beginthread(forkthread_r, stacksize, &fa);

	if ((unsigned long) - 1L != rc)
		WaitForSingleObject(fa.hEvent, INFINITE);

	CloseHandle(fa.hEvent);
	return rc;
}


static void __cdecl phase2(void * lParam)
{
	SESSION_INFO* si = (SESSION_INFO*) lParam;
	Sleep(30);
	if (si && si->hWnd)
		PostMessage(si->hWnd, GC_REDRAWLOG3, 0, 0);
}


/*
 * the actual group chat session window procedure. Handles the entire chat session window
 * which is usually a (tabbed) child of a container class window.
 */

BOOL CALLBACK RoomWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SESSION_INFO * si = NULL;
	HWND hwndTab = GetParent(hwndDlg);
	struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwndDlg, GWL_USERDATA);
	if (dat)
		si = (SESSION_INFO *)dat->si;

	if (dat == NULL && (uMsg == WM_ACTIVATE || uMsg == WM_SETFOCUS))
		return 0;

	switch (uMsg) {
		case WM_INITDIALOG: {
			int mask;
			struct NewMessageWindowLParam *newData = (struct NewMessageWindowLParam *) lParam;
			struct MessageWindowData *dat;
			SESSION_INFO * psi = (SESSION_INFO*)newData->hdbEvent;
			RECT rc;

			dat = (struct MessageWindowData *)malloc(sizeof(struct MessageWindowData));
			ZeroMemory(dat, sizeof(struct MessageWindowData));
			si = psi;
			dat->si = (void *)psi;
			dat->hContact = psi->hContact;
			dat->szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)psi->hContact, 0);

			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)dat);
			dat->bType = SESSIONTYPE_CHAT;
			dat->isIRC = SM_IsIRC(si);
			newData->item.lParam = (LPARAM) hwndDlg;
			TabCtrl_SetItem(hwndTab, newData->iTabID, &newData->item);
			dat->iTabID = newData->iTabID;
			dat->pContainer = newData->pContainer;
			psi->pContainer = newData->pContainer;
			dat->hwnd = hwndDlg;
			psi->hWnd = hwndDlg;
			psi->iSplitterY = g_Settings.iSplitterY;
			dat->fInsertMode = FALSE;

			dat->codePage = DBGetContactSettingDword(dat->hContact, SRMSGMOD_T, "ANSIcodepage", CP_ACP);
			//MAD
			WindowList_Add(hMessageWindowList, hwndDlg, dat->hContact);
			//
			BroadCastContainer(dat->pContainer, DM_REFRESHTABINDEX, 0, 0);

			SendDlgItemMessage(hwndDlg, IDC_CHAT_LOG, EM_SETOLECALLBACK, 0, (LPARAM) & reOleCallback);
			//MAD
			myGlobals.g_NickListScrollBarFix=DBGetContactSettingByte(NULL, SRMSGMOD_T, "ScrollBarFix", 0);
			
			BB_InitDlgButtons(hwndDlg,dat);
			//TODO: unify "change font color" button behavior in IM and Chat windows
			SendMessage(GetDlgItem(hwndDlg,IDC_COLOR), BUTTONSETASPUSHBTN, 0, 0);
			//
			OldSplitterProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTERX), GWL_WNDPROC, (LONG)SplitterSubclassProc);
			SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTERY), GWL_WNDPROC, (LONG)SplitterSubclassProc);
			OldNicklistProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg, IDC_LIST), GWL_WNDPROC, (LONG)NicklistSubclassProc);
			OldLogProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg, IDC_CHAT_LOG), GWL_WNDPROC, (LONG)LogSubclassProc);
			OldFilterButtonProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg, IDC_FILTER), GWL_WNDPROC, (LONG)ButtonSubclassProc);
			SetWindowLong(GetDlgItem(hwndDlg, IDC_COLOR), GWL_WNDPROC, (LONG)ButtonSubclassProc);
			SetWindowLong(GetDlgItem(hwndDlg, IDC_BKGCOLOR), GWL_WNDPROC, (LONG)ButtonSubclassProc);
			OldMessageProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE), GWL_WNDPROC, (LONG)MessageSubclassProc);
			SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_SUBCLASSED, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHAT_LOG, EM_AUTOURLDETECT, 1, 0);

			TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_OPENING, 0);

			mask = (int)SendDlgItemMessage(hwndDlg, IDC_CHAT_LOG, EM_GETEVENTMASK, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHAT_LOG, EM_SETEVENTMASK, 0, mask | ENM_LINK | ENM_MOUSEEVENTS);

			mask = (int)SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_GETEVENTMASK, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_SETEVENTMASK, 0, mask | ENM_CHANGE);

			SendDlgItemMessage(hwndDlg, IDC_CHAT_LOG, EM_LIMITTEXT, (WPARAM)0x7FFFFFFF, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
			SendDlgItemMessage(hwndDlg, IDC_CHAT_LOG, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));

			EnableWindow(GetDlgItem(hwndDlg, IDC_SMILEYBTN), TRUE);

			if (myGlobals.g_hMenuTrayUnread != 0 && dat->hContact != 0 && dat->szProto != NULL)
				UpdateTrayMenu(0, dat->wStatus, dat->szProto, dat->szStatus, dat->hContact, FALSE);

			DM_ThemeChanged(hwndDlg, dat);
			SendMessage(GetDlgItem(hwndDlg, IDC_CHAT_LOG), EM_HIDESELECTION, TRUE, 0);

			splitterEdges = DBGetContactSettingByte(NULL, SRMSGMOD_T, "splitteredges", 1);
			if (splitterEdges) {
				SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTERX), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTERX), GWL_EXSTYLE) | WS_EX_STATICEDGE);
				SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTERY), GWL_EXSTYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTERY), GWL_EXSTYLE) | WS_EX_STATICEDGE);
			}

			SendDlgItemMessage(hwndDlg, IDC_CHAT_TOGGLESIDEBAR, BUTTONSETASFLATBTN + 10, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHAT_TOGGLESIDEBAR, BUTTONSETASFLATBTN + 12, 0, (LPARAM)dat->pContainer);
			SendDlgItemMessage(hwndDlg, IDC_CHAT_TOGGLESIDEBAR, BUTTONSETASFLATBTN, 0, 0);

			dat->wOldStatus = -1;
			SendMessage(hwndDlg, GC_SETWNDPROPS, 0, 0);
			SendMessage(hwndDlg, GC_UPDATESTATUSBAR, 0, 0);
			SendMessage(hwndDlg, GC_UPDATETITLE, 0, 1);
			SendMessage(dat->pContainer->hwnd, DM_QUERYCLIENTAREA, 0, (LPARAM)&rc);
			SetWindowPos(hwndDlg, HWND_TOP, rc.left, rc.top, (rc.right - rc.left), (rc.bottom - rc.top), 0);
			ShowWindow(hwndDlg, SW_SHOW);
			PostMessage(hwndDlg, GC_UPDATENICKLIST, 0, 0);
			dat->pContainer->hwndActive = hwndDlg;
			//mad
			BB_SetButtonsPos(hwndDlg,dat);
			TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_OPEN, 0);
		}
		break;

		case WM_SETFOCUS:
			if (g_sessionshutdown)
				break;

			Chat_UpdateWindowState(hwndDlg, dat, WM_SETFOCUS);
			SetFocus(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE));
			return 1;

		case DM_LOADBUTTONBARICONS: {
			BB_UpdateIcons(hwndDlg, dat);
			return 0;
			}

		case GC_SETWNDPROPS: {
			HICON hIcon;
			COLORREF colour = DBGetContactSettingDword(NULL, FONTMODULE, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR);
			InitButtons(hwndDlg, si);
			ConfigureSmileyButton(hwndDlg, dat);
			hIcon = si->wStatus == ID_STATUS_ONLINE ? MM_FindModule(si->pszModule)->hOnlineIcon : MM_FindModule(si->pszModule)->hOfflineIcon;
			// stupid hack to make icons show. I dunno why this is needed currently
			if (!hIcon) {
				MM_IconsChanged();
				hIcon = si->wStatus == ID_STATUS_ONLINE ? MM_FindModule(si->pszModule)->hOnlineIcon : MM_FindModule(si->pszModule)->hOfflineIcon;
			}

			SendDlgItemMessage(hwndDlg, IDC_CHAT_LOG, EM_SETBKGNDCOLOR, 0, colour);

			{ //messagebox
				COLORREF	    crFore;
				LOGFONTA        lf;
				CHARFORMAT2A    cf2 = {0};
				COLORREF crB = DBGetContactSettingDword(NULL, FONTMODULE, "inputbg", GetSysColor(COLOR_WINDOW));

				LoadLogfont(MSGFONTID_MESSAGEAREA, &lf, &crFore, FONTMODULE);
				cf2.dwMask = CFM_COLOR | CFM_FACE | CFM_CHARSET | CFM_SIZE | CFM_WEIGHT | CFM_ITALIC | CFM_BACKCOLOR;
				cf2.cbSize = sizeof(cf2);
				cf2.crTextColor = crFore;
				cf2.bCharSet = lf.lfCharSet;
				cf2.crBackColor = crB;
				strncpy(cf2.szFaceName, lf.lfFaceName, LF_FACESIZE);
				cf2.dwEffects = 0; //((lf.lfWeight >= FW_BOLD) ? CFE_BOLD : 0) | (lf.lfItalic ? CFE_ITALIC : 0);
				cf2.wWeight = 0; //(WORD)lf.lfWeight;
				cf2.bPitchAndFamily = lf.lfPitchAndFamily;
				cf2.yHeight = abs(lf.lfHeight) * 15;
				SetDlgItemText(hwndDlg, IDC_CHAT_MESSAGE, _T(""));
				SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_SETBKGNDCOLOR, 0, (LPARAM)crB);
				SendDlgItemMessageA(hwndDlg, IDC_CHAT_MESSAGE, EM_SETCHARFORMAT, 0, (LPARAM)&cf2);
			}
			SendDlgItemMessage(hwndDlg, IDOK, BUTTONSETASFLATBTN + 14, 0, 0);
			{
				// nicklist
				int ih;
				int ih2;
				int font;
				int height;

				ih = GetTextPixelSize(_T("AQGglö"), g_Settings.UserListFont, FALSE);
				ih2 = GetTextPixelSize(_T("AQGglö"), g_Settings.UserListHeadingsFont, FALSE);
				height = DBGetContactSettingByte(NULL, "Chat", "NicklistRowDist", 12);
				font = ih > ih2 ? ih : ih2;

				// make sure we have space for icon!

				if (g_Settings.ShowContactStatus)
					font = font > 16 ? font : 16;


				SendMessage(GetDlgItem(hwndDlg, IDC_LIST), LB_SETITEMHEIGHT, 0, (LPARAM)height > font ? height : font);
				InvalidateRect(GetDlgItem(hwndDlg, IDC_LIST), NULL, TRUE);
			}
			SendDlgItemMessage(hwndDlg, IDC_FILTER, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIconEx(si->bFilterEnabled ? IDI_FILTER : IDI_FILTER2, si->bFilterEnabled ? "filter" : "filter2", 0, 0));
			SendMessage(hwndDlg, WM_SIZE, 0, 0);
			SendMessage(hwndDlg, GC_REDRAWLOG2, 0, 0);
		}
		break;

		case DM_UPDATETITLE:
			return(SendMessage(hwndDlg, GC_UPDATETITLE, wParam, lParam));

		case GC_UPDATETITLE: {
			TCHAR szTemp [100];
			HICON hIcon;
			BOOL fNoCopy = TRUE;

			lstrcpyn(dat->szNickname, si->ptszName, 130);
			dat->szNickname[129] = 0;

			dat->wStatus = si->wStatus;

			if (lstrlen(dat->szNickname) > 0) {
				if (DBGetContactSettingByte(NULL, SRMSGMOD_T, "cuttitle", 0))
					CutContactName(dat->szNickname, dat->newtitle, safe_sizeof(dat->newtitle));
				else {
					lstrcpyn(dat->newtitle, dat->szNickname, safe_sizeof(dat->newtitle));
					dat->newtitle[129] = 0;
				}
			}

			switch (si->iType) {
 				case GCW_CHATROOM:
 					hIcon = dat->wStatus <= ID_STATUS_OFFLINE ? LoadSkinnedProtoIcon(si->pszModule, ID_STATUS_OFFLINE) : LoadSkinnedProtoIcon(si->pszModule, dat->wStatus);
 					fNoCopy = FALSE;
 					mir_sntprintf(szTemp, SIZEOF(szTemp),
 								  (si->nUsersInNicklist == 1) ? TranslateT("%s: Chat Room (%u user%s)") : TranslateT("%s: Chat Room (%u users%s)"),
 								  si->ptszName, si->nUsersInNicklist, si->bFilterEnabled ? TranslateT(", event filter active") : _T(""));
 					break;
				case GCW_PRIVMESS:
					mir_sntprintf(szTemp, SIZEOF(szTemp),
								  (si->nUsersInNicklist == 1) ? TranslateT("%s: Message Session") : TranslateT("%s: Message Session (%u users)"),
								  si->ptszName, si->nUsersInNicklist);
					break;
				case GCW_SERVER:
					mir_sntprintf(szTemp, SIZEOF(szTemp), _T("%s: Server"), si->ptszName);
					hIcon = LoadIconEx(IDI_CHANMGR, "window", 16, 16);
					break;
			}

			dat->hTabStatusIcon = hIcon;

			if (lParam)
				dat->hTabIcon = dat->hTabStatusIcon;

			if (dat->wStatus != dat->wOldStatus) {
				TCITEM item;

				ZeroMemory(&item, sizeof(item));
				item.mask = TCIF_TEXT;

				lstrcpyn(dat->szStatus, (TCHAR *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)dat->wStatus, GCMDF_TCHAR), 50);
				dat->szStatus[49] = 0;
				item.pszText = dat->newtitle;
				item.cchTextMax = 120;
				TabCtrl_SetItem(hwndTab, dat->iTabID, &item);
			}
			dat->wOldStatus = dat->wStatus;
			SetWindowText(hwndDlg, szTemp);
			if (dat->pContainer->hwndActive == hwndDlg) {
				SendMessage(dat->pContainer->hwnd, DM_UPDATETITLE, (WPARAM)hwndDlg, 1);
				SendMessage(hwndDlg, GC_UPDATESTATUSBAR, 0, 0);
			}
		}
		break;

		case GC_UPDATESTATUSBAR:
			if (dat->pContainer->hwndActive != hwndDlg || dat->pContainer->hwndStatus == 0 || g_sessionshutdown)
				break;
	//TODO: check tooltip module, if not presented, show normal tooltip
			if (si->pszModule != NULL) {
				TCHAR* ptszDispName = NULL; 
				TCHAR  szFinalStatusBarText[512];
				MODULEINFO* mi=NULL;
				int    x = 12;
				
				//Mad: strange rare crash here...
				mi=MM_FindModule(si->pszModule);
				if(!mi) break;
				
				ptszDispName=a2tf((TCHAR*)mi->pszModDispName, 0, 0);
				if(!ptszDispName) break;

				x += GetTextPixelSize(ptszDispName, (HFONT)SendMessage(dat->pContainer->hwndStatus, WM_GETFONT, 0, 0), TRUE);
				x += GetSystemMetrics(SM_CXSMICON);

				if (si->ptszStatusbarText)
					mir_sntprintf(szFinalStatusBarText, SIZEOF(szFinalStatusBarText), _T("%s %s"), ptszDispName, si->ptszStatusbarText);
				else
					lstrcpyn(szFinalStatusBarText, ptszDispName, SIZEOF(szFinalStatusBarText));

				SendMessage(dat->pContainer->hwndStatus, SB_SETTEXT, 0, (LPARAM)szFinalStatusBarText);
				SendMessage(dat->pContainer->hwndStatus, SB_SETICON, 0, (LPARAM)(nen_options.bFloaterInWin ? myGlobals.g_buttonBarIcons[16] : 0));
//				SendMessage(dat->pContainer->hwndStatus, SB_SETTIPTEXT, 0, (LPARAM)szFinalStatusBarText);
				UpdateStatusBar(hwndDlg, dat);
				mir_free(ptszDispName);
				return TRUE;
			}
			break;

		case WM_SIZE: {
			UTILRESIZEDIALOG urd;

			if (wParam == SIZE_MAXIMIZED)
				PostMessage(hwndDlg, GC_SCROLLTOBOTTOM, 0, 0);

			if (IsIconic(hwndDlg)) break;
			ZeroMemory(&urd, sizeof(urd));
			urd.cbSize = sizeof(urd);
			urd.hInstance = g_hInst;
			urd.hwndDlg = hwndDlg;
			urd.lParam = (LPARAM)si;
			urd.lpTemplate = MAKEINTRESOURCEA(IDD_CHANNEL);
			urd.pfnResizer = RoomWndResize;
			CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM)&urd);
			//mad
			BB_SetButtonsPos(hwndDlg,dat);
			}
		break;

		case GC_REDRAWWINDOW:
			InvalidateRect(hwndDlg, NULL, TRUE);
			break;

		case GC_REDRAWLOG:
			si->LastTime = 0;
			if (si->pLog) {
				LOGINFO * pLog = si->pLog;
				if (si->iEventCount > 60) {
					int index = 0;
					while (index < 59) {
						if (pLog->next == NULL)
							break;
						pLog = pLog->next;
						if (si->iType != GCW_CHATROOM || !si->bFilterEnabled || (si->iLogFilterFlags&pLog->iType) != 0)
							index++;
					}
					Log_StreamInEvent(hwndDlg, pLog, si, TRUE, FALSE);
					forkthread(phase2, 0, (void *)si);
				} else Log_StreamInEvent(hwndDlg, si->pLogEnd, si, TRUE, FALSE);
			} else SendMessage(hwndDlg, GC_EVENT_CONTROL + WM_USER + 500, WINDOW_CLEARLOG, 0);
			break;

		case GC_REDRAWLOG2:
			si->LastTime = 0;
			if (si->pLog)
				Log_StreamInEvent(hwndDlg, si->pLogEnd, si, TRUE, FALSE);
			break;

		case GC_REDRAWLOG3:
			si->LastTime = 0;
			if (si->pLog)
				Log_StreamInEvent(hwndDlg, si->pLogEnd, si, TRUE, TRUE);
			break;

		case GC_ADDLOG:
			if (g_Settings.UseDividers && g_Settings.DividersUsePopupConfig) {
				if (!MessageWindowOpened(0, (LPARAM)hwndDlg))
					SendMessage(hwndDlg, DM_ADDDIVIDER, 0, 0);
			} else if (g_Settings.UseDividers) {
				if ((GetForegroundWindow() != dat->pContainer->hwnd || GetActiveWindow() != dat->pContainer->hwnd))
					SendMessage(hwndDlg, DM_ADDDIVIDER, 0, 0);
				else {
					if (dat->pContainer->hwndActive != hwndDlg)
						SendMessage(hwndDlg, DM_ADDDIVIDER, 0, 0);
				}
			}

			if (si->pLogEnd)
				Log_StreamInEvent(hwndDlg, si->pLog, si, FALSE, FALSE);
			else
				SendMessage(hwndDlg, GC_EVENT_CONTROL + WM_USER + 500, WINDOW_CLEARLOG, 0);
			break;

		case GC_ACKMESSAGE:
			SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_SETREADONLY, FALSE, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, WM_SETTEXT, 0, (LPARAM)_T(""));
			return TRUE;

		case WM_CTLCOLORLISTBOX:
			SetBkColor((HDC) wParam, g_Settings.crUserListBGColor);
			return (BOOL) hListBkgBrush;

		case WM_MEASUREITEM: {
			MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *) lParam;

			if (mis->CtlType == ODT_MENU)
				return CallService(MS_CLIST_MENUMEASUREITEM, wParam, lParam);
			else {
				int ih = GetTextPixelSize(_T("AQGglö"), g_Settings.UserListFont, FALSE);
				int ih2 = GetTextPixelSize(_T("AQGglö"), g_Settings.UserListHeadingsFont, FALSE);
				int font = ih > ih2 ? ih : ih2;
				int height = DBGetContactSettingByte(NULL, "Chat", "NicklistRowDist", 12);

				mis->itemHeight = height > font ? height : font;

				// make sure we have enough space for icon!

				if (g_Settings.ShowContactStatus)
					mis->itemHeight = (mis->itemHeight > 16) ? mis->itemHeight : 16;

			}
			return TRUE;
		}

		case WM_DRAWITEM: {
			DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *) lParam;

			if (dis->CtlType == ODT_MENU)
				return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
			else {
				if (dis->CtlID == IDC_LIST) {
					HFONT  hFont, hOldFont;
					HICON  hIcon;
					int offset, x_offset = 0;
					int height;
					int index = dis->itemID;
					USERINFO * ui = UM_FindUserFromIndex(si->pUsers, index);
					char szIndicator = 0;

					if (ui) {
						height = dis->rcItem.bottom - dis->rcItem.top;

						if (height&1)
							height++;
						if (height == 10)
							offset = 0;
						else
							offset = height / 2;
						hIcon = SM_GetStatusIcon(si, ui, &szIndicator);
						hFont = (ui->iStatusEx == 0) ? g_Settings.UserListFont : g_Settings.UserListHeadingsFont;
						hOldFont = (HFONT) SelectObject(dis->hDC, hFont);
						SetBkMode(dis->hDC, TRANSPARENT);

						if (/*dis->itemAction == ODA_FOCUS || */ dis->itemState & ODS_SELECTED) {
							FillRect(dis->hDC, &dis->rcItem, g_Settings.SelectionBGBrush);
							SetTextColor(dis->hDC, g_Settings.nickColors[6]);
						} else {
							FillRect(dis->hDC, &dis->rcItem, hListBkgBrush);
							if (g_Settings.ColorizeNicks && szIndicator != 0) {
								COLORREF clr;

								switch (szIndicator) {
									case '@':
										clr = g_Settings.nickColors[0];
										break;
									case '%':
										clr = g_Settings.nickColors[1];
										break;
									case '+':
										clr = g_Settings.nickColors[2];
										break;
									case '!':
										clr = g_Settings.nickColors[3];
										break;
									case '*':
										clr = g_Settings.nickColors[4];
										break;
								}
								SetTextColor(dis->hDC, clr);
							} else SetTextColor(dis->hDC, ui->iStatusEx == 0 ? g_Settings.crUserListColor : g_Settings.crUserListHeadingsColor);
						}
						x_offset = 2;

						if (g_Settings.ShowContactStatus && g_Settings.ContactStatusFirst && ui->ContactStatus) {
							HICON hIcon = LoadSkinnedProtoIcon(si->pszModule, ui->ContactStatus);
							DrawIconEx(dis->hDC, x_offset, dis->rcItem.top + offset - 8, hIcon, 16, 16, 0, NULL, DI_NORMAL);
							x_offset += 18;
						}

						if (g_Settings.ClassicIndicators) {
							char szTemp[3];
							SIZE szUmode;

							szTemp[1] = 0;
							szTemp[0] = szIndicator;
							if (szTemp[0]) {
								GetTextExtentPoint32A(dis->hDC, szTemp, 1, &szUmode);
								TextOutA(dis->hDC, x_offset, dis->rcItem.top, szTemp, 1);
								x_offset += szUmode.cx + 2;
							} else x_offset += 8;
						} else {
							DrawIconEx(dis->hDC, x_offset, dis->rcItem.top + offset - 5, hIcon, 10, 10, 0, NULL, DI_NORMAL);
							x_offset += 12;
						}

						if (g_Settings.ShowContactStatus && !g_Settings.ContactStatusFirst && ui->ContactStatus) {
							HICON hIcon = LoadSkinnedProtoIcon(si->pszModule, ui->ContactStatus);
							DrawIconEx(dis->hDC, x_offset, dis->rcItem.top + offset - 8, hIcon, 16, 16, 0, NULL, DI_NORMAL);
							x_offset += 18;
						}

						{
							SIZE sz;

							if (si->iSearchItem != -1 && si->iSearchItem == index && si->szSearch[0]) {
								COLORREF clr_orig = GetTextColor(dis->hDC);
								GetTextExtentPoint32(dis->hDC, ui->pszNick, lstrlen(si->szSearch), &sz);
								SetTextColor(dis->hDC, RGB(250, 250, 0));
								TextOut(dis->hDC, x_offset, (dis->rcItem.top + dis->rcItem.bottom - sz.cy) / 2, ui->pszNick, lstrlen(si->szSearch));
								SetTextColor(dis->hDC, clr_orig);
								x_offset += sz.cx;
								TextOut(dis->hDC, x_offset, (dis->rcItem.top + dis->rcItem.bottom - sz.cy) / 2, ui->pszNick + lstrlen(si->szSearch), lstrlen(ui->pszNick) - lstrlen(si->szSearch));
							} else {
								GetTextExtentPoint32(dis->hDC, ui->pszNick, lstrlen(ui->pszNick), &sz);
								TextOut(dis->hDC, x_offset, (dis->rcItem.top + dis->rcItem.bottom - sz.cy) / 2, ui->pszNick, lstrlen(ui->pszNick));
								SelectObject(dis->hDC, hOldFont);
							}
						}
					}
					return TRUE;
				}
			}
		}
		break;
		case WM_CONTEXTMENU:{
			//mad
			DWORD idFrom=GetDlgCtrlID((HWND)wParam);
			if(idFrom>=MIN_CBUTTONID&&idFrom<=MAX_CBUTTONID)
				BB_CustomButtonClick(dat,idFrom,(HWND) wParam,1);
			}break;
			//
		case GC_UPDATENICKLIST: {
			int i = SendMessage(GetDlgItem(hwndDlg, IDC_LIST), LB_GETTOPINDEX, 0, 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_LIST), LB_SETCOUNT, si->nUsersInNicklist, 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_LIST), LB_SETTOPINDEX, i, 0);
			SendMessage(hwndDlg, GC_UPDATETITLE, 0, 0);
		}
		break;

		case GC_EVENT_CONTROL + WM_USER + 500: {
			switch (wParam) {
				case SESSION_OFFLINE:
					SendMessage(hwndDlg, GC_UPDATESTATUSBAR, 0, 0);
					SendMessage(si->hWnd, GC_UPDATENICKLIST, (WPARAM)0, (LPARAM)0);
					return TRUE;

				case SESSION_ONLINE:
					SendMessage(hwndDlg, GC_UPDATESTATUSBAR, 0, 0);
					return TRUE;

				case WINDOW_HIDDEN:
					SendMessage(hwndDlg, GC_CLOSEWINDOW, 0, 1);
					return TRUE;

				case WINDOW_CLEARLOG:
					SetDlgItemText(hwndDlg, IDC_CHAT_LOG, _T(""));
					return TRUE;

				case SESSION_TERMINATE:
					if (CallService(MS_CLIST_GETEVENT, (WPARAM)si->hContact, (LPARAM)0))
						CallService(MS_CLIST_REMOVEEVENT, (WPARAM)si->hContact, (LPARAM)szChatIconString);
					si->wState &= ~STATE_TALK;
					DBWriteContactSettingWord(si->hContact, si->pszModule , "ApparentMode", (LPARAM) 0);
					SendMessage(hwndDlg, GC_CLOSEWINDOW, 0, lParam == 2 ? lParam : 1);
					return TRUE;

				case WINDOW_MINIMIZE:
					ShowWindow(hwndDlg, SW_MINIMIZE);
					goto LABEL_SHOWWINDOW;

				case WINDOW_MAXIMIZE:
					ShowWindow(hwndDlg, SW_MAXIMIZE);
					goto LABEL_SHOWWINDOW;

				case SESSION_INITDONE:
					if (DBGetContactSettingByte(NULL, "Chat", "PopupOnJoin", 0) != 0)
						return TRUE;
					// fall through
				case WINDOW_VISIBLE:
					if (IsIconic(hwndDlg))
						ShowWindow(hwndDlg, SW_NORMAL);
LABEL_SHOWWINDOW:
					SendMessage(hwndDlg, WM_SIZE, 0, 0);
					SendMessage(hwndDlg, GC_REDRAWLOG, 0, 0);
					SendMessage(hwndDlg, GC_UPDATENICKLIST, 0, 0);
					SendMessage(hwndDlg, GC_UPDATESTATUSBAR, 0, 0);
					ShowWindow(hwndDlg, SW_SHOW);
					SendMessage(hwndDlg, WM_SIZE, 0, 0);
					SetForegroundWindow(hwndDlg);
					return TRUE;
			}
		}
		break;

		case DM_SPLITTERMOVED: {
			POINT pt;
			RECT rc;
			RECT rcLog;
			BOOL bFormat = TRUE; //IsWindowVisible(GetDlgItem(hwndDlg,IDC_SMILEY));

			static int x = 0;

			GetWindowRect(GetDlgItem(hwndDlg, IDC_CHAT_LOG), &rcLog);
			if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_SPLITTERX)) {
				int oldSplitterX;
				GetClientRect(hwndDlg, &rc);
				pt.x = wParam;
				pt.y = 0;
				ScreenToClient(hwndDlg, &pt);

				oldSplitterX = si->iSplitterX;
				si->iSplitterX = rc.right - pt.x + 1;
				if (si->iSplitterX < 35)
					si->iSplitterX = 35;
				if (si->iSplitterX > rc.right - rc.left - 35)
					si->iSplitterX = rc.right - rc.left - 35;
				g_Settings.iSplitterX = si->iSplitterX;
			} else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_SPLITTERY) || lParam == -1) {
				int oldSplitterY;
				GetClientRect(hwndDlg, &rc);
				pt.x = 0;
				pt.y = wParam;
				ScreenToClient(hwndDlg, &pt);

				oldSplitterY = si->iSplitterY;
				si->iSplitterY = bFormat ? rc.bottom - pt.y + DPISCALEY(1) : rc.bottom - pt.y + DPISCALEY(20);
				if (si->iSplitterY < DPISCALEY(30))
					si->iSplitterY = DPISCALEY(30);
				if (si->iSplitterY > rc.bottom - rc.top - DPISCALEY(40))
					si->iSplitterY = rc.bottom - rc.top - DPISCALEY(40);
				g_Settings.iSplitterY = si->iSplitterY;
				}
			if (x == 2) {
				PostMessage(hwndDlg, WM_SIZE, 0, 0);
				x = 0;
			} else x++;
		
			}
		break;

		case GC_FIREHOOK:
			if (lParam) {
				GCHOOK* gch = (GCHOOK *) lParam;
				NotifyEventHooks(hSendEvent, 0, (WPARAM)gch);
				if (gch->pDest) {
					mir_free(gch->pDest->pszID);
					mir_free(gch->pDest->pszModule);
					mir_free(gch->pDest);
				}
				mir_free(gch->ptszText);
				mir_free(gch->ptszUID);
				mir_free(gch);
			}
			break;

		case GC_CHANGEFILTERFLAG:
			if (si->iLogFilterFlags == 0 && si->bFilterEnabled)
				SendMessage(hwndDlg, WM_COMMAND, IDC_FILTER, 0);
			break;

		case GC_SHOWFILTERMENU: {
			RECT rc;
			HWND hwnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_FILTER), hwndDlg, FilterWndProc, (LPARAM)si);
			GetWindowRect(GetDlgItem(hwndDlg, IDC_FILTER), &rc);
			SetWindowPos(hwnd, HWND_TOP, rc.left - 85, (IsWindowVisible(GetDlgItem(hwndDlg, IDC_FILTER)) || IsWindowVisible(GetDlgItem(hwndDlg, IDC_CHAT_BOLD))) ? rc.top - 206 : rc.top - 186, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
		}
		break;

		case DM_SPLITTERMOVEDGLOBAL_NOSYNC:
			return 0;

		case DM_SPLITTERMOVEDGLOBAL: {
			short newMessagePos;
			RECT rcWin, rcClient;

			if (IsIconic(dat->pContainer->hwnd) || dat->pContainer->hwndActive != hwndDlg) {
				dat->dwFlagsEx |= MWF_EX_DELAYEDSPLITTER;
				dat->wParam = wParam;
				dat->lParam = lParam;
				return 0;
			}
			GetWindowRect(hwndDlg, &rcWin);
			GetClientRect(hwndDlg, &rcClient);
			newMessagePos = (short)rcWin.bottom - (short)wParam;

			SendMessage(hwndDlg, DM_SPLITTERMOVED, newMessagePos + lParam / 2, (LPARAM)GetDlgItem(hwndDlg, IDC_SPLITTERY));
			//PostMessage(hwndDlg, DM_DELAYEDSCROLL, 0, 1);
			return 0;
		}

		case GC_SHOWCOLORCHOOSER: {
			HWND ColorWindow;
			RECT rc;
			BOOL bFG = lParam == IDC_COLOR ? TRUE : FALSE;
			COLORCHOOSER * pCC = mir_alloc(sizeof(COLORCHOOSER));

			GetWindowRect(GetDlgItem(hwndDlg, bFG ? IDC_COLOR : IDC_BKGCOLOR), &rc);
			pCC->hWndTarget = GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE);
			pCC->pModule = MM_FindModule(si->pszModule);
			pCC->xPosition = rc.left + 3;
			pCC->yPosition = IsWindowVisible(GetDlgItem(hwndDlg, IDC_COLOR)) ? rc.top - 1 : rc.top + 20;
			pCC->bForeground = bFG;
			pCC->si = si;

			ColorWindow = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_COLORCHOOSER), hwndDlg, DlgProcColorToolWindow, (LPARAM) pCC);
		}
		break;

		case GC_SCROLLTOBOTTOM: {
			SCROLLINFO si = { 0 };
			return(DM_ScrollToBottom(hwndDlg, dat, wParam, lParam));
		}

		case WM_TIMER:
			if (wParam == TIMERID_FLASHWND)
				if (dat->mayFlashTab)
					FlashTab(dat, hwndTab, dat->iTabID, &dat->bTabFlash, TRUE, dat->hTabIcon);
			break;

		case WM_ACTIVATE:
			if (LOWORD(wParam) != WA_ACTIVE) {
				dat->pContainer->hwndSaved = 0;
				break;
			}

			//fall through
		case WM_MOUSEACTIVATE:
			Chat_UpdateWindowState(hwndDlg, dat, WM_ACTIVATE);
			return 1;

		case WM_NOTIFY: {
			LPNMHDR pNmhdr = (LPNMHDR)lParam;
			switch (pNmhdr->code) {
				case EN_MSGFILTER:
					if (pNmhdr->idFrom = IDC_CHAT_LOG && ((MSGFILTER *) lParam)->msg == WM_RBUTTONUP) {
						CHARRANGE sel, all = { 0, -1 };
						POINT pt;
						UINT uID = 0;
						HMENU hMenu = 0;
						TCHAR pszWord[4096];
						int pos;

						pt.x = (short) LOWORD(((ENLINK *) lParam)->lParam);
						pt.y = (short) HIWORD(((ENLINK *) lParam)->lParam);
						ClientToScreen(pNmhdr->hwndFrom, &pt);

						{ // fixing stuff for searches
							long iCharIndex, iLineIndex, iChars, start, end, iRes;
							POINTL ptl;

							pszWord[0] = '\0';
							ptl.x = (LONG)pt.x;
							ptl.y = (LONG)pt.y;
							ScreenToClient(GetDlgItem(hwndDlg, IDC_CHAT_LOG), (LPPOINT)&ptl);
							iCharIndex = SendMessage(GetDlgItem(hwndDlg, IDC_CHAT_LOG), EM_CHARFROMPOS, 0, (LPARAM) & ptl);
							if (iCharIndex < 0)
								break;
							iLineIndex = SendMessage(GetDlgItem(hwndDlg, IDC_CHAT_LOG), EM_EXLINEFROMCHAR, 0, (LPARAM)iCharIndex);
							iChars = SendMessage(GetDlgItem(hwndDlg, IDC_CHAT_LOG), EM_LINEINDEX, (WPARAM)iLineIndex, 0);
							start = SendMessage(GetDlgItem(hwndDlg, IDC_CHAT_LOG), EM_FINDWORDBREAK, WB_LEFT, iCharIndex);//-iChars;
							end = SendMessage(GetDlgItem(hwndDlg, IDC_CHAT_LOG), EM_FINDWORDBREAK, WB_RIGHT, iCharIndex);//-iChars;

							if (end - start > 0) {
								TEXTRANGE tr;
								CHARRANGE cr;
								static char szTrimString[] = ":;,.!?\'\"><()[]- \r\n";
								ZeroMemory(&tr, sizeof(TEXTRANGE));

								cr.cpMin = start;
								cr.cpMax = end;
								tr.chrg = cr;
								tr.lpstrText = (TCHAR *)pszWord;
								iRes = SendMessage(GetDlgItem(hwndDlg, IDC_CHAT_LOG), EM_GETTEXTRANGE, 0, (LPARAM) & tr);

								if (iRes > 0) {
									int iLen = lstrlen(pszWord) - 1;
									while (iLen >= 0 && strchr(szTrimString, pszWord[iLen])) {
										pszWord[iLen] = '\0';
										iLen--;
									}
								}
							}
						}

						uID = CreateGCMenu(hwndDlg, &hMenu, 1, pt, si, NULL, pszWord);

						if ((uID > 800 && uID < 1400) || uID == CP_UTF8 || uID == 20866) {
							dat->codePage = uID;
							DBWriteContactSettingDword(dat->hContact, SRMSGMOD_T, "ANSIcodepage", dat->codePage);
						} else if (uID == 500) {
							dat->codePage = CP_ACP;
							DBDeleteContactSetting(dat->hContact, SRMSGMOD_T, "ANSIcodepage");
						} else {
							switch (uID) {
								case 0:
									PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
									break;

								case ID_COPYALL:
									SendMessage(pNmhdr->hwndFrom, EM_EXGETSEL, 0, (LPARAM) & sel);
									SendMessage(pNmhdr->hwndFrom, EM_EXSETSEL, 0, (LPARAM) & all);
									SendMessage(pNmhdr->hwndFrom, WM_COPY, 0, 0);
									SendMessage(pNmhdr->hwndFrom, EM_EXSETSEL, 0, (LPARAM) & sel);
									PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
									break;

								case ID_CLEARLOG: {
									SESSION_INFO* s = SM_FindSession(si->ptszID, si->pszModule);
									if (s) {
										SetDlgItemText(hwndDlg, IDC_CHAT_LOG, _T(""));
										LM_RemoveAll(&s->pLog, &s->pLogEnd);
										s->iEventCount = 0;
										s->LastTime = 0;
										si->iEventCount = 0;
										si->LastTime = 0;
										si->pLog = s->pLog;
										si->pLogEnd = s->pLogEnd;
										PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
									}
								}
								break;

								case ID_SEARCH_GOOGLE: {
									char szURL[4096];
									if (pszWord[0]) {
										mir_snprintf(szURL, sizeof(szURL), "http://www.google.com/search?q=" TCHAR_STR_PARAM, pszWord);
										CallService(MS_UTILS_OPENURL, 1, (LPARAM) szURL);
									}
									PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
								}
								break;

								case ID_SEARCH_WIKIPEDIA: {
									char szURL[4096];
									if (pszWord[0]) {
										mir_snprintf(szURL, sizeof(szURL), "http://en.wikipedia.org/wiki/" TCHAR_STR_PARAM, pszWord);
										CallService(MS_UTILS_OPENURL, 1, (LPARAM) szURL);
									}
									PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
								}
								break;

								default:
									PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
									DoEventHookAsync(hwndDlg, si->ptszID, si->pszModule, GC_USER_LOGMENU, NULL, NULL, (LPARAM)uID);
									break;
							}
						}
						if (si->iType != GCW_SERVER && !(si->dwFlags & GC_UNICODE)) {
							pos = GetMenuItemCount(hMenu);
							RemoveMenu(hMenu, pos - 1, MF_BYPOSITION);
							RemoveMenu(myGlobals.g_hMenuEncoding, 1, MF_BYPOSITION);
						}
						DestroyGCMenu(&hMenu, 5);
					}
					break;

				case EN_LINK:
					if (pNmhdr->idFrom = IDC_CHAT_LOG) {
						switch (((ENLINK *) lParam)->msg) {
							case WM_SETCURSOR:

								if (g_Settings.ClickableNicks) {
									if (!hCurHyperlinkHand)
										hCurHyperlinkHand = LoadCursor(NULL, IDC_HAND);
									if (hCurHyperlinkHand != GetCursor())
										SetCursor(hCurHyperlinkHand);
									return TRUE;
								}
								break;

  
							case WM_RBUTTONDOWN:
							case WM_LBUTTONUP:
							case WM_LBUTTONDBLCLK: {
								TEXTRANGE tr;
								CHARRANGE sel;
								BOOL isLink = FALSE;
								UINT msg = ((ENLINK *) lParam)->msg;

								tr.lpstrText = NULL;
								SendMessage(pNmhdr->hwndFrom, EM_EXGETSEL, 0, (LPARAM) & sel);
								if (sel.cpMin != sel.cpMax)
									break;
								tr.chrg = ((ENLINK *) lParam)->chrg;
								tr.lpstrText = mir_alloc(sizeof(TCHAR) * (tr.chrg.cpMax - tr.chrg.cpMin + 2));
								SendMessage(pNmhdr->hwndFrom, EM_GETTEXTRANGE, 0, (LPARAM) & tr);

								isLink = IsStringValidLink(tr.lpstrText);

								if (isLink) {
									char* pszUrl = t2a(tr.lpstrText, 0);
									if (((ENLINK *) lParam)->msg == WM_RBUTTONDOWN) {
										HMENU hSubMenu;
										POINT pt;

										hSubMenu = GetSubMenu(g_hMenu, 2);
										CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hSubMenu, 0);
										pt.x = (short) LOWORD(((ENLINK *) lParam)->lParam);
										pt.y = (short) HIWORD(((ENLINK *) lParam)->lParam);
										ClientToScreen(((NMHDR *) lParam)->hwndFrom, &pt);
										switch (TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL)) {
											case ID_NEW:
												CallService(MS_UTILS_OPENURL, 1, (LPARAM) pszUrl);
												break;
											case ID_CURR:
												CallService(MS_UTILS_OPENURL, 0, (LPARAM) pszUrl);
												break;
											case ID_COPY: {
												HGLOBAL hData;
												if (!OpenClipboard(hwndDlg))
													break;
												EmptyClipboard();
												hData = GlobalAlloc(GMEM_MOVEABLE, sizeof(TCHAR) * (lstrlen(tr.lpstrText) + 1));
												lstrcpy((TCHAR*)GlobalLock(hData), tr.lpstrText);
												GlobalUnlock(hData);
#if defined( _UNICODE )
												SetClipboardData(CF_UNICODETEXT, hData);
#else
												SetClipboardData(CF_TEXT, hData);
#endif
											}
											CloseClipboard();
											SetFocus(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE));
											break;
										}
										mir_free(tr.lpstrText);
										mir_free(pszUrl);
										return TRUE;
									} else if (((ENLINK *) lParam)->msg == WM_LBUTTONUP) {
										CallService(MS_UTILS_OPENURL, 1, (LPARAM) pszUrl);
										SetFocus(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE));
										mir_free(tr.lpstrText);
										mir_free(pszUrl);
										return TRUE;
									}
								} else if (g_Settings.ClickableNicks) {                    // clicked a nick name
									CHARRANGE chr;
									TEXTRANGE tr2;
									TCHAR tszAplTmpl[] = _T("%s:"),
														 *tszAppeal, *tszTmp;
									size_t st;

									if (msg == WM_RBUTTONDOWN) {
										USERINFO *ui = si->pUsers;
										HMENU     hMenu = 0;
										USERINFO  uiNew;
										while (ui) {
											if (!lstrcmp(ui->pszNick, tr.lpstrText)) {
												POINT pt;
												UINT  uID;

												pt.x = (short) LOWORD(((ENLINK *) lParam)->lParam);
												pt.y = (short) HIWORD(((ENLINK *) lParam)->lParam);
												ClientToScreen(((NMHDR *) lParam)->hwndFrom, &pt);
												CopyMemory(&uiNew, ui, sizeof(USERINFO));
												uID = CreateGCMenu(hwndDlg, &hMenu, 0, pt, si, uiNew.pszUID, NULL);
												switch (uID) {
													case 0:
														break;

													case ID_MESS:
														DoEventHookAsync(hwndDlg, si->ptszID, si->pszModule, GC_USER_PRIVMESS, ui->pszUID, NULL, (LPARAM)NULL);
														break;

													default:
														DoEventHookAsync(hwndDlg, si->ptszID, si->pszModule, GC_USER_NICKLISTMENU, ui->pszUID, NULL, (LPARAM)uID);
														break;
												}
												DestroyGCMenu(&hMenu, 1);
												return TRUE;
											}
											ui = ui->next;
										}
										return TRUE;
									} 
			                                                 else if (msg == WM_LBUTTONUP) {
										USERINFO	*ui = si->pUsers;
										//BOOL		fFound = TRUE;

										/*while(ui) {
											if(!lstrcmp(ui->pszNick, tr.lpstrText)) {
												fFound = TRUE;
												break;
											}
											ui = ui->next;
										}*/

										//if(fFound) {
											SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_EXGETSEL, 0, (LPARAM) &chr);
											tszTmp = tszAppeal = (TCHAR *) malloc((_tcslen(tr.lpstrText) + _tcslen(tszAplTmpl) + 3) * sizeof(TCHAR));
											tr2.lpstrText = (LPTSTR) malloc(sizeof(TCHAR) * 2);
											if (chr.cpMin) {
												/* prepend nick with space if needed */
												tr2.chrg.cpMin = chr.cpMin - 1;
												tr2.chrg.cpMax = chr.cpMin;
												SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_GETTEXTRANGE, 0, (LPARAM) &tr2);
												if (! _istspace(*tr2.lpstrText))
													*tszTmp++ = _T(' ');
												_tcscpy(tszTmp, tr.lpstrText);
											}
											else
												/* in the beginning of the message window */
												_stprintf(tszAppeal, tszAplTmpl, tr.lpstrText);
											st = _tcslen(tszAppeal);
											if (chr.cpMax != -1) {
												tr2.chrg.cpMin = chr.cpMax;
												tr2.chrg.cpMax = chr.cpMax + 1;
												/* if there is no space after selection,
												or there is nothing after selection at all... */
												if (! SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE,	EM_GETTEXTRANGE, 0, (LPARAM) &tr2) || ! _istspace(*tr2.lpstrText)) {
														tszAppeal[st++] = _T(' ');
														tszAppeal[st++] = _T('\0');
												}
											}
											else {
												tszAppeal[st++] = _T(' ');
												tszAppeal[st++] = _T('\0');
											}
											SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_REPLACESEL,  FALSE, (LPARAM)tszAppeal);
											free((void *) tr2.lpstrText);
											free((void *) tszAppeal);
									}
								}
								SetFocus(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE));
								mir_free(tr.lpstrText);
								return TRUE;
							}
						}
						return TRUE;
					}
					return TRUE;
			}
		}
		break;

		case WM_LBUTTONDOWN: {
			POINT tmp; //+ Protogenes
			POINTS cur; //+ Protogenes
			GetCursorPos(&tmp); //+ Protogenes
			cur.x = (SHORT)tmp.x; //+ Protogenes
			cur.y = (SHORT)tmp.y; //+ Protogenes

			SendMessage(dat->pContainer->hwnd, WM_NCLBUTTONDOWN, HTCAPTION, *((LPARAM*)(&cur))); //+ Protogenes
		}
		break;

		case WM_LBUTTONUP: {
			POINT tmp; //+ Protogenes
			POINTS cur; //+ Protogenes
			GetCursorPos(&tmp); //+ Protogenes
			cur.x = (SHORT)tmp.x; //+ Protogenes
			cur.y = (SHORT)tmp.y; //+ Protogenes

			SendMessage(dat->pContainer->hwnd, WM_NCLBUTTONUP, HTCAPTION, *((LPARAM*)(&cur))); //+ Protogenes
		}
		break;

		case WM_APPCOMMAND: {
			DWORD cmd = GET_APPCOMMAND_LPARAM(lParam);
			if (cmd == APPCOMMAND_BROWSER_BACKWARD || cmd == APPCOMMAND_BROWSER_FORWARD) {
				SendMessage(dat->pContainer->hwnd, DM_SELECTTAB, cmd == APPCOMMAND_BROWSER_BACKWARD ? DM_SELECT_PREV : DM_SELECT_NEXT, 0);
				return 1;
			}
		}
		break;

		case WM_COMMAND:
			//mad
			if(LOWORD(wParam)>=MIN_CBUTTONID&&LOWORD(wParam)<=MAX_CBUTTONID){
				BB_CustomButtonClick(dat,LOWORD(wParam) ,GetDlgItem(hwndDlg,LOWORD(wParam)),0);
				break;
				}
			//
			switch (LOWORD(wParam)) {
				case IDC_LIST:
					if (HIWORD(wParam) == LBN_DBLCLK) {
						TVHITTESTINFO hti;
						int item;
						USERINFO * ui;

						hti.pt.x = (short)LOWORD(GetMessagePos());
						hti.pt.y = (short)HIWORD(GetMessagePos());
						ScreenToClient(GetDlgItem(hwndDlg, IDC_LIST), &hti.pt);

						item = LOWORD(SendMessage(GetDlgItem(hwndDlg, IDC_LIST), LB_ITEMFROMPOINT, 0, MAKELPARAM(hti.pt.x, hti.pt.y)));
						ui = UM_FindUserFromIndex(si->pUsers, item);
						//ui = SM_GetUserFromIndex(si->pszID, si->pszModule, item);
						if (ui) {
							if (g_Settings.DoubleClick4Privat ? GetKeyState(VK_SHIFT) & 0x8000 : !(GetKeyState(VK_SHIFT) & 0x8000)) {
								LRESULT lResult = (LRESULT)SendMessage(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE), EM_GETSEL, (WPARAM)NULL, (LPARAM)NULL);
								int start = LOWORD(lResult);
								TCHAR* pszName = (TCHAR*)alloca(sizeof(TCHAR) * (lstrlen(ui->pszUID) + 3));
								if (start == 0)
									mir_sntprintf(pszName, lstrlen(ui->pszUID) + 3, _T("%s: "), ui->pszUID);
								else
									mir_sntprintf(pszName, lstrlen(ui->pszUID) + 2, _T("%s "), ui->pszUID);

								SendMessage(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE), EM_REPLACESEL, FALSE, (LPARAM) pszName);
								PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
								SetFocus(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE));
							} else DoEventHookAsync(hwndDlg, si->ptszID, si->pszModule, GC_USER_PRIVMESS, ui->pszUID, NULL, (LPARAM)NULL);
						}

						return TRUE;
					} else if (HIWORD(wParam) == LBN_KILLFOCUS)
						RedrawWindow(GetDlgItem(hwndDlg, IDC_LIST), NULL, NULL, RDW_INVALIDATE);
					break;

				case IDC_CHAT_TOGGLESIDEBAR:
					ApplyContainerSetting(dat->pContainer, CNT_SIDEBAR, dat->pContainer->dwFlags & CNT_SIDEBAR ? 0 : 1);
					break;

				case IDCANCEL:
					ShowWindow(dat->pContainer->hwnd, SW_MINIMIZE);
					return FALSE;

				case IDOK: {
					char*  pszRtf;
					TCHAR* ptszText/*, *p1*/;
					if (GetSendButtonState(hwndDlg) == PBS_DISABLED)
						break;

					pszRtf = Chat_Message_GetFromStream(hwndDlg, si);
					SM_AddCommand(si->ptszID, si->pszModule, pszRtf);
					ptszText = Chat_DoRtfToTags(pszRtf, si);
// 					p1 = _tcschr(ptszText, '\0');
// 
// 					//remove trailing linebreaks
// 					while (p1 > ptszText && (*p1 == '\0' || *p1 == '\r' || *p1 == '\n')) {
// 						*p1 = '\0';
// 						p1--;
// 					}
					DoTrimMessage(ptszText);

					if (MM_FindModule(si->pszModule)->bAckMsg) {
						EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE), FALSE);
						SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_SETREADONLY, TRUE, 0);
					} else SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, WM_SETTEXT, 0, (LPARAM)_T(""));

					EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);

					DoEventHookAsync(hwndDlg, si->ptszID, si->pszModule, GC_USER_MESSAGE, NULL, ptszText, (LPARAM)NULL);
					mir_free(pszRtf);
#if defined( _UNICODE )
					free(ptszText);
#endif
					SetFocus(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE));
				}
				break;

				case IDC_SHOWNICKLIST:
					if (!IsWindowEnabled(GetDlgItem(hwndDlg, IDC_SHOWNICKLIST)))
						break;
					if (si->iType == GCW_SERVER)
						break;

					si->bNicklistEnabled = !si->bNicklistEnabled;

					SendDlgItemMessage(hwndDlg, IDC_SHOWNICKLIST, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIconEx(si->bNicklistEnabled ? IDI_HIDENICKLIST : IDI_SHOWNICKLIST, si->bNicklistEnabled ? "hidenicklist" : "shownicklist", 0, 0));
					SendMessage(hwndDlg, WM_SIZE, 0, 0);
					if (dat->pContainer->bSkinned)
						InvalidateRect(hwndDlg, NULL, TRUE);
					PostMessage(hwndDlg, GC_SCROLLTOBOTTOM, 0, 0);
					break;

				case IDC_CHAT_MESSAGE:
					if (g_Settings.MathMod)
						MTH_updateMathWindow(hwndDlg, dat);

					if (HIWORD(wParam) == EN_CHANGE) {
						if (dat->pContainer->hwndActive == hwndDlg)
							UpdateReadChars(hwndDlg, dat);
						dat->dwLastActivity = GetTickCount();
						dat->pContainer->dwLastActivity = dat->dwLastActivity;
						SendDlgItemMessage(hwndDlg, IDOK, BUTTONSETASFLATBTN + 14, GetRichTextLength(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE)) != 0, 0);
						EnableWindow(GetDlgItem(hwndDlg, IDOK), GetRichTextLength(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE)) != 0);
					}
					break;

				case IDC_SMILEY:
				case IDC_SMILEYBTN: {
					SMADD_SHOWSEL3 smaddInfo = {0};
					RECT rc;

					if (lParam == 0)
						GetWindowRect(GetDlgItem(hwndDlg, IDC_SMILEYBTN), &rc);
					else
						GetWindowRect((HWND)lParam, &rc);
					smaddInfo.cbSize = sizeof(SMADD_SHOWSEL3);
					smaddInfo.hwndTarget = GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE);
					smaddInfo.targetMessage = EM_REPLACESEL;
					smaddInfo.targetWParam = TRUE;
					smaddInfo.Protocolname = si->pszModule;
					smaddInfo.Direction = 0;
					smaddInfo.xPosition = rc.left;
					smaddInfo.yPosition = rc.top + 24;
					smaddInfo.hContact = si->hContact;
					smaddInfo.hwndParent = dat->pContainer->hwnd;
					if (myGlobals.g_SmileyAddAvail)
						CallService(MS_SMILEYADD_SHOWSELECTION, 0, (LPARAM) &smaddInfo);
				}
				break;

				case IDC_CHAT_HISTORY: {
					char szFile[MAX_PATH];
					char szName[MAX_PATH];
					char szFolder[MAX_PATH];
					MODULEINFO * pInfo = MM_FindModule(si->pszModule);

					if (!IsWindowEnabled(GetDlgItem(hwndDlg, IDC_CHAT_HISTORY)))
						break;

					if (ServiceExists("MSP/HTMLlog/ViewLog") && strstr(si->pszModule, "IRC")) {
#if defined(_UNICODE)
						char szName[MAX_PATH];

						WideCharToMultiByte(CP_ACP, 0, si->ptszName, -1, szName, MAX_PATH, 0, 0);
						szName[MAX_PATH - 1] = 0;
						CallService("MSP/HTMLlog/ViewLog", (WPARAM)si->pszModule, (LPARAM)szName);
#else
						CallService("MSP/HTMLlog/ViewLog", (WPARAM)si->pszModule, (LPARAM)si->ptszName);
#endif
					} else if (pInfo) {
						mir_snprintf(szName, MAX_PATH, "%s", pInfo->pszModDispName ? pInfo->pszModDispName : si->pszModule);
						ValidateFilename(szName);
						mir_snprintf(szFolder, MAX_PATH, "%s\\%s", g_Settings.pszLogDir, szName);
#if defined(_UNICODE) 
					{
						wchar_t wszName[MAX_PATH];
						mir_sntprintf(wszName, MAX_PATH, _T("%s.log"), si->ptszID);
						WideCharToMultiByte(CP_ACP, 0, wszName, -1, szName, MAX_PATH, 0, 0);
						szName[MAX_PATH - 1] = 0;
					}
#else
						mir_snprintf(szName, MAX_PATH, "%s.log", si->ptszID);
#endif
					ValidateFilename(szName);
					mir_snprintf(szFile, MAX_PATH, "%s\\%s", szFolder, szName);
					ShellExecuteA(hwndDlg, "open", szFile, NULL, NULL, SW_SHOW);
				}
		}
		break;

		case IDC_CHAT_CLOSE:
			SendMessage(hwndDlg, WM_CLOSE, 0, 1);
			break;

		case IDC_CHANMGR:
			if (!IsWindowEnabled(GetDlgItem(hwndDlg, IDC_CHANMGR)))
				break;
			DoEventHookAsync(hwndDlg, si->ptszID, si->pszModule, GC_USER_CHANMGR, NULL, NULL, (LPARAM)NULL);
			break;

		case IDC_FILTER:
			if (!IsWindowEnabled(GetDlgItem(hwndDlg, IDC_FILTER)))
				break;

			if (si->iLogFilterFlags == 0 && !si->bFilterEnabled) {
				MessageBox(0, TranslateT("The filter canoot be enabled, because there are no event types selected either global or for this chat room"), TranslateT("Event filter error"), MB_OK);
				si->bFilterEnabled = 0;
			} else
				si->bFilterEnabled = !si->bFilterEnabled;
			SendDlgItemMessage(hwndDlg, IDC_FILTER, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIconEx(si->bFilterEnabled ? IDI_FILTER : IDI_FILTER2, si->bFilterEnabled ? "filter" : "filter2", 0, 0));
			if (si->bFilterEnabled && DBGetContactSettingByte(NULL, "Chat", "RightClickFilter", 0) == 0) {
				SendMessage(hwndDlg, GC_SHOWFILTERMENU, 0, 0);
				break;
			}
			SendMessage(hwndDlg, GC_REDRAWLOG, 0, 0);
			SendMessage(hwndDlg, GC_UPDATETITLE, 0, 0);
			DBWriteContactSettingByte(si->hContact, "Chat", "FilterEnabled", (BYTE)si->bFilterEnabled);
			break;

		case IDC_BKGCOLOR: {
			CHARFORMAT2 cf;

			cf.cbSize = sizeof(CHARFORMAT2);
			cf.dwEffects = 0;

			if (!IsWindowEnabled(GetDlgItem(hwndDlg, IDC_BKGCOLOR)))
				break;

			if (IsDlgButtonChecked(hwndDlg, IDC_BKGCOLOR)) {
				if (DBGetContactSettingByte(NULL, "Chat", "RightClickFilter", 0) == 0)
					SendMessage(hwndDlg, GC_SHOWCOLORCHOOSER, 0, (LPARAM)IDC_BKGCOLOR);
				else if (si->bBGSet) {
					cf.dwMask = CFM_BACKCOLOR;
					cf.crBackColor = MM_FindModule(si->pszModule)->crColors[si->iBG];
					SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				}
			} else {
				cf.dwMask = CFM_BACKCOLOR;
				cf.crBackColor = (COLORREF)DBGetContactSettingDword(NULL, FONTMODULE, "inputbg", GetSysColor(COLOR_WINDOW));
				SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
			}
		}
		break;

		case IDC_COLOR: {
			CHARFORMAT2 cf;
			cf.cbSize = sizeof(CHARFORMAT2);
			cf.dwEffects = 0;

			if (!IsWindowEnabled(GetDlgItem(hwndDlg, IDC_COLOR)))
				break;

			if (IsDlgButtonChecked(hwndDlg, IDC_COLOR)) {
				if (DBGetContactSettingByte(NULL, "Chat", "RightClickFilter", 0) == 0)
					SendMessage(hwndDlg, GC_SHOWCOLORCHOOSER, 0, (LPARAM)IDC_COLOR);
				else if (si->bFGSet) {
					cf.dwMask = CFM_COLOR;
					cf.crTextColor = MM_FindModule(si->pszModule)->crColors[si->iFG];
					SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				}
			} else {
				COLORREF cr;

				LoadLogfont(MSGFONTID_MESSAGEAREA, NULL, &cr, FONTMODULE);
				cf.dwMask = CFM_COLOR;
				cf.crTextColor = cr;
				SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
			}
		}
		break;

		case IDC_CHAT_BOLD:
		case IDC_ITALICS:
		case IDC_CHAT_UNDERLINE: {
			CHARFORMAT2 cf;
			cf.cbSize = sizeof(CHARFORMAT2);
			cf.dwMask = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
			cf.dwEffects = 0;

			if (LOWORD(wParam) == IDC_CHAT_BOLD && !IsWindowEnabled(GetDlgItem(hwndDlg, IDC_CHAT_BOLD)))
				break;
			if (LOWORD(wParam) == IDC_ITALICS && !IsWindowEnabled(GetDlgItem(hwndDlg, IDC_ITALICS)))
				break;
			if (LOWORD(wParam) == IDC_CHAT_UNDERLINE && !IsWindowEnabled(GetDlgItem(hwndDlg, IDC_CHAT_UNDERLINE)))
				break;
			if (IsDlgButtonChecked(hwndDlg, IDC_CHAT_BOLD))
				cf.dwEffects |= CFE_BOLD;
			if (IsDlgButtonChecked(hwndDlg, IDC_ITALICS))
				cf.dwEffects |= CFE_ITALIC;
			if (IsDlgButtonChecked(hwndDlg, IDC_CHAT_UNDERLINE))
				cf.dwEffects |= CFE_UNDERLINE;

			SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
		}
	}
	break;

case WM_KEYDOWN:
	SetFocus(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE));
	break;

case WM_MOVE:
	break;

case WM_ERASEBKGND:
	if (dat->pContainer->bSkinned)
		return TRUE;
	break;

case WM_NCPAINT:
	if (dat->pContainer->bSkinned)
		return 0;
	break;
case WM_PAINT:
	if (dat->pContainer->bSkinned) {
		PAINTSTRUCT ps;
		RECT rcClient, rcWindow, rc;
		StatusItems_t *item;
		POINT pt;
		UINT item_ids[3] = {ID_EXTBKUSERLIST, ID_EXTBKHISTORY, ID_EXTBKINPUTAREA};
		UINT ctl_ids[3] = {IDC_LIST, IDC_CHAT_LOG, IDC_CHAT_MESSAGE};
		int  i;

		HDC hdc = BeginPaint(hwndDlg, &ps);
		GetClientRect(hwndDlg, &rcClient);
		SkinDrawBG(hwndDlg, dat->pContainer->hwnd, dat->pContainer, &rcClient, hdc);

		for (i = 0; i < 3; i++) {
			item = &StatusItems[item_ids[i]];
			if (!item->IGNORED) {

				GetWindowRect(GetDlgItem(hwndDlg, ctl_ids[i]), &rcWindow);
				pt.x = rcWindow.left;
				pt.y = rcWindow.top;
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

case WM_GETMINMAXINFO: {
	MINMAXINFO* mmi = (MINMAXINFO*)lParam;
	mmi->ptMinTrackSize.x = si->iSplitterX + 43;
	if (mmi->ptMinTrackSize.x < 350)
		mmi->ptMinTrackSize.x = 350;

	mmi->ptMinTrackSize.y = si->iSplitterY + 80;
}
break;

case WM_RBUTTONUP: {
	POINT pt;
	int iSelection;
	HMENU subMenu;
	int isHandled;
	int menuID = 0;

	GetCursorPos(&pt);
	subMenu = GetSubMenu(dat->pContainer->hMenuContext, 0);

	MsgWindowUpdateMenu(hwndDlg, dat, subMenu, MENU_TABCONTEXT);

	iSelection = TrackPopupMenu(subMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL);
	if (iSelection >= IDM_CONTAINERMENU) {
		DBVARIANT dbv = {0};
		char szIndex[10];
#if defined (_UNICODE)
		char *szKey = "TAB_ContainersW";
#else
		char *szKey = "TAB_Containers";
#endif
		_snprintf(szIndex, 8, "%d", iSelection - IDM_CONTAINERMENU);
		if (iSelection - IDM_CONTAINERMENU >= 0) {
			if (!DBGetContactSettingTString(NULL, szKey, szIndex, &dbv)) {
				SendMessage(hwndDlg, DM_CONTAINERSELECTED, 0, (LPARAM)dbv.ptszVal);
				DBFreeVariant(&dbv);
			}
		}

		break;
	}
	isHandled = MsgWindowMenuHandler(hwndDlg, dat, iSelection, MENU_TABCONTEXT);
}
break;

case WM_LBUTTONDBLCLK: {
	if (LOWORD(lParam) < 30)
		PostMessage(hwndDlg, GC_SCROLLTOBOTTOM, 0, 0);
	if (GetKeyState(VK_CONTROL) & 0x8000) {
		SendMessage(dat->pContainer->hwnd, WM_CLOSE, 1, 0);
		break;
	}
	if (GetKeyState(VK_SHIFT) & 0x8000 && !g_framelessSkinmode) {
		SendMessage(dat->pContainer->hwnd, WM_SYSCOMMAND, IDM_NOTITLE, 0);
		break;
	}
	SendMessage(dat->pContainer->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	break;
}

case WM_CLOSE:
	if (wParam == 0 && lParam == 0 && !myGlobals.m_EscapeCloses) {
		SendMessage(dat->pContainer->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		return TRUE;
	}
	if (lParam && myGlobals.m_WarnOnClose)
		if (MessageBox(dat->pContainer->hwnd, TranslateTS(szWarnClose), _T("Miranda"), MB_YESNO | MB_ICONQUESTION) == IDNO)
			return TRUE;

	SendMessage(hwndDlg, GC_CLOSEWINDOW, 0, 1);
	break;

case DM_CONTAINERSELECTED: {
	struct ContainerWindowData *pNewContainer = 0;
	TCHAR *szNewName = (TCHAR *)lParam;
	int iOldItems = TabCtrl_GetItemCount(hwndTab);
	if (!_tcsncmp(dat->pContainer->szName, szNewName, CONTAINER_NAMELEN))
		break;
	pNewContainer = FindContainerByName(szNewName);
	if (pNewContainer == NULL)
		pNewContainer = CreateContainer(szNewName, FALSE, dat->hContact);
#if defined (_UNICODE)
	DBWriteContactSettingTString(dat->hContact, SRMSGMOD_T, "containerW", szNewName);
#else
	DBWriteContactSettingTString(dat->hContact, SRMSGMOD_T, "container", szNewName);
#endif
	PostMessage(myGlobals.g_hwndHotkeyHandler, DM_DOCREATETAB_CHAT, (WPARAM)pNewContainer, (LPARAM)hwndDlg);
	if (iOldItems > 1)                // there were more than 1 tab, container is still valid
		SendMessage(dat->pContainer->hwndActive, WM_SIZE, 0, 0);
	SetForegroundWindow(pNewContainer->hwnd);
	SetActiveWindow(pNewContainer->hwnd);
}
break;
// container API support functions

case DM_QUERYCONTAINER: {
	struct ContainerWindowData **pc = (struct ContainerWindowData **) lParam;
	if (pc)
		*pc = dat->pContainer;
	return 0;
}

case DM_QUERYHCONTACT: {
	HANDLE *phContact = (HANDLE *) lParam;
	if (phContact)
		*phContact = dat->hContact;
	return 0;
}

case GC_CLOSEWINDOW: {
	int iTabs, i;
	TCITEM item = {0};
	RECT rc;
	struct ContainerWindowData *pContainer = dat->pContainer;
	BOOL   bForced = (lParam == 2);

	iTabs = TabCtrl_GetItemCount(hwndTab);
	if (iTabs == 1) {
		if (!bForced && g_sessionshutdown == 0) {
			PostMessage(GetParent(GetParent(hwndDlg)), WM_CLOSE, 0, 1);
			return 1;
		}
	}

	dat->pContainer->iChilds--;
	i = GetTabIndexFromHWND(hwndTab, hwndDlg);

	/*
	* after closing a tab, we need to activate the tab to the left side of
	* the previously open tab.
	* normally, this tab has the same index after the deletion of the formerly active tab
	* unless, of course, we closed the last (rightmost) tab.
	*/
	if (!dat->pContainer->bDontSmartClose && iTabs > 1 && !bForced) {
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
	//SM_SetTabbedWindowHwnd(0, 0);
	DestroyWindow(hwndDlg);
	if (iTabs == 1)
		PostMessage(GetParent(GetParent(hwndDlg)), WM_CLOSE, 0, 1);
	else
		SendMessage(pContainer->hwnd, WM_SIZE, 0, 0);

	return 0;
}

case DM_SETLOCALE:
	if (dat->dwFlags & MWF_WASBACKGROUNDCREATE)
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
	return 0;

case DM_SAVESIZE: {
	RECT rcClient;

	if (dat->dwFlags & MWF_NEEDCHECKSIZE)
		lParam = 0;

	dat->dwFlags &= ~MWF_NEEDCHECKSIZE;
	if (dat->dwFlags & MWF_WASBACKGROUNDCREATE)
		dat->dwFlags &= ~MWF_INITMODE;

	SendMessage(dat->pContainer->hwnd, DM_QUERYCLIENTAREA, 0, (LPARAM)&rcClient);
	MoveWindow(hwndDlg, rcClient.left, rcClient.top, (rcClient.right - rcClient.left), (rcClient.bottom - rcClient.top), TRUE);
	if (dat->dwFlags & MWF_WASBACKGROUNDCREATE) {
		POINT pt = {0};;

		dat->dwFlags &= ~MWF_WASBACKGROUNDCREATE;
		SendMessage(hwndDlg, WM_SIZE, 0, 0);
		//LoadSplitter(hwndDlg, dat);
		SendDlgItemMessage(hwndDlg, IDC_CHAT_LOG, EM_SETSCROLLPOS, 0, (LPARAM)&pt);
		DM_LoadLocale(hwndDlg, dat);
		SendMessage(hwndDlg, DM_SETLOCALE, 0, 0);
	} else {
		SendMessage(hwndDlg, WM_SIZE, 0, 0);
		if (lParam == 0)
			PostMessage(hwndDlg, GC_SCROLLTOBOTTOM, 1, 1);
	}
	return 0;
}

case DM_GETWINDOWSTATE: {
	UINT state = 0;

	state |= MSG_WINDOW_STATE_EXISTS;
	if (IsWindowVisible(hwndDlg))
		state |= MSG_WINDOW_STATE_VISIBLE;
	if (GetForegroundWindow() == dat->pContainer->hwnd)
		state |= MSG_WINDOW_STATE_FOCUS;
	if (IsIconic(dat->pContainer->hwnd))
		state |= MSG_WINDOW_STATE_ICONIC;
	SetWindowLong(hwndDlg, DWL_MSGRESULT, state);
	return TRUE;
}

case DM_ADDDIVIDER:
	if (!(dat->dwFlags & MWF_DIVIDERSET) && g_Settings.UseDividers) {
		if (GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_CHAT_LOG)) > 0) {
			dat->dwFlags |= MWF_DIVIDERWANTED;
			dat->dwFlags |= MWF_DIVIDERSET;
		}
	}
	return 0;

case DM_CHECKSIZE:
	dat->dwFlags |= MWF_NEEDCHECKSIZE;
	return 0;

case DM_REFRESHTABINDEX:
	dat->iTabID = GetTabIndexFromHWND(GetParent(hwndDlg), hwndDlg);
	if (dat->iTabID == -1)
		_DebugPopup(si->hContact, _T("WARNING: new tabindex: %d"), dat->iTabID);
	return 0;

case DM_STATUSBARCHANGED:
	UpdateStatusBar(hwndDlg, dat);
	break;

	//mad: bb-api
case DM_BBNEEDUPDATE:{
	if(lParam)
		CB_ChangeButton(hwndDlg,dat,(CustomButtonData*)lParam);
	else
		BB_InitDlgButtons(hwndDlg,dat);

	BB_SetButtonsPos(hwndDlg, dat);
	}break;

case DM_CBDESTROY:{
	if(lParam)
		CB_DestroyButton(hwndDlg,dat,(DWORD)wParam,(DWORD)lParam);
	else
		CB_DestroyAllButtons(hwndDlg,dat);
	}break;		
	//	

case DM_CONFIGURETOOLBAR:
	SendMessage(hwndDlg, WM_SIZE, 0, 0);
	break;

case DM_SMILEYOPTIONSCHANGED:
	ConfigureSmileyButton(hwndDlg, dat);
	SendMessage(hwndDlg, GC_REDRAWLOG, 0, 1);
	break;

case EM_THEMECHANGED:
	if (dat->hTheme && pfnCloseThemeData) {
		pfnCloseThemeData(dat->hTheme);
		dat->hTheme = 0;
	}
	return DM_ThemeChanged(hwndDlg, dat);

case WM_NCDESTROY:
	if (dat) {
		free(dat);
		SetWindowLong(hwndDlg, GWL_USERDATA, 0);
	}
	break;

case WM_DESTROY: {
	int i;

	if (CallService(MS_CLIST_GETEVENT, (WPARAM)si->hContact, (LPARAM)0))
		CallService(MS_CLIST_REMOVEEVENT, (WPARAM)si->hContact, (LPARAM)szChatIconString);
	si->wState &= ~STATE_TALK;
	si->hWnd = NULL;
	//SetWindowLong(hwndDlg,GWL_USERDATA,0);
	SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTERX), GWL_WNDPROC, (LONG)OldSplitterProc);
	SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTERY), GWL_WNDPROC, (LONG)OldSplitterProc);
	SetWindowLong(GetDlgItem(hwndDlg, IDC_LIST), GWL_WNDPROC, (LONG)OldNicklistProc);
	SendDlgItemMessage(hwndDlg, IDC_CHAT_MESSAGE, EM_UNSUBCLASSED, 0, 0);
	SetWindowLong(GetDlgItem(hwndDlg, IDC_CHAT_MESSAGE), GWL_WNDPROC, (LONG)OldMessageProc);
	SetWindowLong(GetDlgItem(hwndDlg, IDC_CHAT_LOG), GWL_WNDPROC, (LONG)OldLogProc);
	SetWindowLong(GetDlgItem(hwndDlg, IDC_FILTER), GWL_WNDPROC, (LONG)OldFilterButtonProc);
	SetWindowLong(GetDlgItem(hwndDlg, IDC_COLOR), GWL_WNDPROC, (LONG)OldFilterButtonProc);
	SetWindowLong(GetDlgItem(hwndDlg, IDC_BKGCOLOR), GWL_WNDPROC, (LONG)OldFilterButtonProc);

	TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_CLOSING, 0);

	DBWriteContactSettingWord(NULL, "Chat", "SplitterX", (WORD)g_Settings.iSplitterX);
	DBWriteContactSettingWord(NULL, "Chat", "splitY", (WORD)g_Settings.iSplitterY);

	UpdateTrayMenuState(dat, FALSE);               // remove me from the tray menu (if still there)
	if (myGlobals.g_hMenuTrayUnread)
		DeleteMenu(myGlobals.g_hMenuTrayUnread, (UINT_PTR)dat->hContact, MF_BYCOMMAND);

	if (dat->hSmileyIcon)
		DestroyIcon(dat->hSmileyIcon);

 			if (hCurHyperlinkHand) 
 				DestroyCursor(hCurHyperlinkHand);

	i = GetTabIndexFromHWND(hwndTab, hwndDlg);
	if (i >= 0) {
		SendMessage(hwndTab, WM_USER + 100, 0, 0);              // clean up tooltip
		TabCtrl_DeleteItem(hwndTab, i);
		BroadCastContainer(dat->pContainer, DM_REFRESHTABINDEX, 0, 0);
		dat->iTabID = -1;
	}
	//MAD
	WindowList_Remove(hMessageWindowList, hwndDlg);
	//
	TABSRMM_FireEvent(dat->hContact, hwndDlg, MSG_WINDOW_EVT_CLOSE, 0);
	break;
}
}

return(FALSE);
}
