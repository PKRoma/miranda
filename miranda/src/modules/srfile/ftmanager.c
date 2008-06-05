/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project,
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
#include "file.h"

static HWND hwndFtMgr = NULL;

struct TFtMgrData
{
	HWND hwndIncoming;
	HWND hwndOutgoing;

	HANDLE hhkPreshutdown;
	HANDLE hhkShutdown;
};

struct TLayoutWindowInfo
{
	HWND hwnd;
	RECT rc;
};

struct TLayoutWindowList
{
	struct TLayoutWindowInfo **items;
	int realCount, limit, increment;
	FSortFunc sortFunc;
};

struct TFtPageData
{
	struct TLayoutWindowList *wnds;
	int height, dataHeight, scrollPos;
};

static void LayoutTransfers(HWND hwnd, struct TFtPageData *dat)
{
	int top = 0;
	RECT rc;
	GetClientRect(hwnd, &rc);

	dat->scrollPos = GetScrollPos(hwnd, SB_VERT);
	dat->height = rc.bottom - rc.top;

	if (dat->wnds->realCount)
	{
		int i;
		HDWP hdwp;

		hdwp = BeginDeferWindowPos(dat->wnds->realCount);
		top -= dat->scrollPos;
		for (i = 0; i < dat->wnds->realCount; ++i)
		{
			int height = dat->wnds->items[i]->rc.bottom - dat->wnds->items[i]->rc.top;
			hdwp = DeferWindowPos(hdwp, dat->wnds->items[i]->hwnd, NULL, 0, top, rc.right, height, SWP_NOZORDER);
			top += height;
		}
		top += dat->scrollPos;
		EndDeferWindowPos(hdwp);
	}

	dat->dataHeight = top;

	{
		SCROLLINFO si = {0};
		si.cbSize = sizeof(si);
		si.fMask = SIF_DISABLENOSCROLL|SIF_PAGE|SIF_RANGE;
		si.nPage = dat->height;
		si.nMin = 0;
		si.nMax = dat->dataHeight;
		SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
	}
}

static BOOL CALLBACK FtMgrPageDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct TFtPageData *dat = (struct TFtPageData *)GetWindowLong(hwnd, GWL_USERDATA);

	switch (msg)
	{
	case WM_INITDIALOG:
	{
		// Force scrollbar visibility
		SCROLLINFO si = {0};
		si.cbSize = sizeof(si);
		si.fMask = SIF_DISABLENOSCROLL;
		SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

		dat = (struct TFtPageData *)mir_alloc(sizeof(struct TFtPageData));
		dat->wnds = (struct TLayoutWindowList *)List_Create(0, 1);
		dat->scrollPos = 0;
		SetWindowLong(hwnd, GWL_USERDATA, (LONG)dat);
		break;
	}

	case WM_FT_ADD:
	{
		struct TLayoutWindowInfo *wnd = (struct TLayoutWindowInfo *)mir_alloc(sizeof(struct TLayoutWindowInfo));
		wnd->hwnd = (HWND)lParam;
		GetWindowRect(wnd->hwnd, &wnd->rc);
		List_Insert((SortedList *)dat->wnds, wnd, dat->wnds->realCount);
		LayoutTransfers(hwnd, dat);
		break;
	}

	case WM_FT_RESIZE:
	{
		int i;
		for (i = 0; i < dat->wnds->realCount; ++i)
			if (dat->wnds->items[i]->hwnd == (HWND)lParam)
			{
				GetWindowRect(dat->wnds->items[i]->hwnd, &dat->wnds->items[i]->rc);
				break;
			}
		LayoutTransfers(hwnd, dat);
		break;
	}

	case WM_FT_REMOVE:
	{
		int i;
		for (i = 0; i < dat->wnds->realCount; ++i)
			if (dat->wnds->items[i]->hwnd == (HWND)lParam)
			{
				mir_free(dat->wnds->items[i]);
				List_Remove((SortedList *)dat->wnds, i);
				break;
			}
		LayoutTransfers(hwnd, dat);
		break;
	}

	case WM_FT_CLEANUP:
	{
		int i;
		for (i = 0; i < dat->wnds->realCount; ++i)
			SendMessage(dat->wnds->items[i]->hwnd, WM_FT_CLEANUP, wParam, lParam);
		break;
	}

	case WM_SIZE:
	{
		LayoutTransfers(hwnd, dat);
		break;
	}

	case WM_MOUSEWHEEL:
	{
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		if (zDelta)
		{
			int i, nScrollLines = 0;
			SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, (void*)&nScrollLines, 0);
			for (i = 0; i < (nScrollLines + 1) / 2; i++ )
				SendMessage(hwnd, WM_VSCROLL, (zDelta < 0) ? SB_LINEDOWN : SB_LINEUP, 0);
		}

		SetWindowLong(hwnd, DWL_MSGRESULT, 0);
		return TRUE;
	}

	case WM_VSCROLL:
	{
		int pos = dat->scrollPos;
		switch (LOWORD(wParam))
		{
		case SB_LINEDOWN:
			pos += 15;
			break;
		case SB_LINEUP:
			pos -= 15;
			break;
		case SB_PAGEDOWN:
			pos += dat->height - 10;
			break;
		case SB_PAGEUP:
			pos -= dat->height - 10;
			break;
		case SB_THUMBTRACK:
			pos = HIWORD(wParam);
			break;
		}

		if (pos > dat->dataHeight - dat->height) pos = dat->dataHeight - dat->height;
		if (pos < 0) pos = 0;

		if (dat->scrollPos != pos)
		{
			ScrollWindow(hwnd, 0, dat->scrollPos - pos, NULL, NULL);
			SetScrollPos(hwnd, SB_VERT, pos, TRUE);
			dat->scrollPos = pos;
		}
		break;
	}

	case M_PRESHUTDOWN:
	{
		int i;
		for (i = 0; i < dat->wnds->realCount; ++i)
			PostMessage(dat->wnds->items[i]->hwnd, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), 0);
		break;
	}

	case WM_DESTROY:
	{
		int i;
		for (i = 0; i < dat->wnds->realCount; ++i)
			mir_free(dat->wnds->items[i]);
		List_Destroy((SortedList *)dat->wnds);
		mir_free(dat->wnds);
		mir_free(dat);
		break;
	}
	}

	return FALSE;
}

static BOOL CALLBACK FtMgrDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct TFtMgrData *dat = (struct TFtMgrData *)GetWindowLong(hwnd, GWL_USERDATA);

	switch (msg)
	{
	case WM_INITDIALOG:
	{
		TCITEM tci = {0};
		HWND hwndTab = GetDlgItem(hwnd, IDC_TABS);

		TranslateDialogDefault(hwndDlg);
		Window_SetIcon_IcoLib(hwnd, SKINICON_EVENT_FILE);

		dat = (struct TFtMgrData *)mir_alloc(sizeof(struct TFtMgrData));
		SetWindowLong(hwnd, GWL_USERDATA, (LONG)dat);

		dat->hhkPreshutdown = HookEventMessage(ME_SYSTEM_PRESHUTDOWN, hwnd, M_PRESHUTDOWN);
		dat->hhkShutdown = HookEventMessage(ME_SYSTEM_SHUTDOWN, hwnd, WM_DESTROY);

		dat->hwndIncoming = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FTPAGE), hwnd, FtMgrPageDlgProc);
		dat->hwndOutgoing = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FTPAGE), hwnd, FtMgrPageDlgProc);
		ShowWindow(dat->hwndIncoming, SW_SHOW);

		tci.mask = TCIF_PARAM|TCIF_TEXT;
		tci.pszText = TranslateT("Incoming");
		tci.lParam = (LPARAM)dat->hwndIncoming;
		TabCtrl_InsertItem(hwndTab, 0, &tci);
		tci.pszText = TranslateT("Outgoing");
		tci.lParam = (LPARAM)dat->hwndOutgoing;
		TabCtrl_InsertItem(hwndTab, 1, &tci);

		Utils_RestoreWindowPosition(hwnd, NULL, "SRFile", "FtMgrDlg_");
		// Fall through to setup initial placement
	}

	case WM_SIZE:
	{
		RECT rc, rcButton;
		HDWP hdwp;
		HWND hwndTab = GetDlgItem(hwnd, IDC_TABS);

		GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &rcButton);
		OffsetRect(&rcButton, -rcButton.left, -rcButton.top);

		GetClientRect(hwnd, &rc);
		InflateRect(&rc, -6, -6);

		hdwp = BeginDeferWindowPos(3);

		hdwp = DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_CLEAR), NULL, rc.left, rc.bottom-rcButton.bottom, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
		hdwp = DeferWindowPos(hdwp, GetDlgItem(hwnd, IDCANCEL), NULL, rc.right-rcButton.right, rc.bottom-rcButton.bottom, 0, 0, SWP_NOZORDER|SWP_NOSIZE);

		rc.bottom -= rcButton.bottom + 5;

		hdwp = DeferWindowPos(hdwp, hwndTab, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER);

		EndDeferWindowPos(hdwp);

		GetWindowRect(hwndTab, &rc);
		MapWindowPoints(NULL, hwnd, (LPPOINT)&rc, 2);
		TabCtrl_AdjustRect(hwndTab, FALSE, &rc);
		InflateRect(&rc, -5, -5);

		hdwp = BeginDeferWindowPos(2);

		hdwp = DeferWindowPos(hdwp, dat->hwndIncoming, HWND_TOP, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, 0);
		hdwp = DeferWindowPos(hdwp, dat->hwndOutgoing, HWND_TOP, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, 0);

		EndDeferWindowPos(hdwp);

		break;
	}

	case WM_MOUSEWHEEL:
	{
		if (IsWindowVisible(dat->hwndIncoming)) SendMessage(dat->hwndIncoming, msg, wParam, lParam);
		if (IsWindowVisible(dat->hwndOutgoing)) SendMessage(dat->hwndOutgoing, msg, wParam, lParam);
		break;
	}

	case WM_FT_SELECTPAGE:
	{
		TCITEM tci = {0};
		HWND hwndTab = GetDlgItem(hwnd, IDC_TABS);

		if (TabCtrl_GetCurSel(hwndTab) == (int)wParam) break;

		tci.mask = TCIF_PARAM;

		TabCtrl_GetItem(hwndTab, TabCtrl_GetCurSel(hwndTab), &tci);
		ShowWindow((HWND)tci.lParam, SW_HIDE);

		TabCtrl_SetCurSel(hwndTab, wParam);

		TabCtrl_GetItem(hwndTab, TabCtrl_GetCurSel(hwndTab), &tci);
		ShowWindow((HWND)tci.lParam, SW_SHOW);

		break;
	}

	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
		lpmmi->ptMinTrackSize.x = 300;
		lpmmi->ptMinTrackSize.y = 400;
		return 0;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDCANCEL:
				PostMessage(hwnd, WM_CLOSE , 0, 0);
				break;

			case IDC_CLEAR:
				PostMessage(dat->hwndIncoming, WM_FT_CLEANUP, 0, 0);
				PostMessage(dat->hwndOutgoing, WM_FT_CLEANUP, 0, 0);
				break;
		}
		break;

	case WM_NOTIFY:
	{
		switch (((LPNMHDR)lParam)->idFrom)
		{
		case IDC_TABS:
		{
			HWND hwndTab = GetDlgItem(hwnd, IDC_TABS);
			switch (((LPNMHDR)lParam)->code)
			{
			case TCN_SELCHANGING:
			{
				TCITEM tci = {0};
				tci.mask = TCIF_PARAM;
				TabCtrl_GetItem(hwndTab, TabCtrl_GetCurSel(hwndTab), &tci);
				ShowWindow((HWND)tci.lParam, SW_HIDE);
				break;
			}

			case TCN_SELCHANGE:
			{
				TCITEM tci = {0};
				tci.mask = TCIF_PARAM;
				TabCtrl_GetItem(hwndTab, TabCtrl_GetCurSel(hwndTab), &tci);
				ShowWindow((HWND)tci.lParam, SW_SHOW);
				break;
			}
			}
			break;
		}
		}
		break;
	}

	case M_PRESHUTDOWN:
		SendMessage(dat->hwndIncoming, M_PRESHUTDOWN, 0, 0);
		SendMessage(dat->hwndOutgoing, M_PRESHUTDOWN, 0, 0);
		break;

	case WM_CLOSE:
		ShowWindow(hwnd, SW_HIDE);
		return TRUE; /* Disable default IDCANCEL notification */

	case WM_DESTROY:
		UnhookEvent(dat->hhkPreshutdown);
		UnhookEvent(dat->hhkShutdown);
		Window_FreeIcon_IcoLib(hwnd);
		DestroyWindow(dat->hwndIncoming);
		DestroyWindow(dat->hwndOutgoing);
		mir_free(dat);
		SetWindowLong(hwnd, GWL_USERDATA, 0);
		Utils_SaveWindowPosition(hwnd, NULL, "SRFile", "FtMgrDlg_");
		break;
	}

	return FALSE;
}

HWND FtMgr_Show()
{
	if (!hwndFtMgr)
		hwndFtMgr = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FTMGR), NULL, FtMgrDlgProc);

	ShowWindow(hwndFtMgr, SW_SHOW);
	SetForegroundWindow(hwndFtMgr);
	return hwndFtMgr;
}

void FtMgr_Destroy()
{
	if (hwndFtMgr)
		DestroyWindow(hwndFtMgr);
}

void FtMgr_ShowPage(int page)
{
	if (hwndFtMgr)
		SendMessage(hwndFtMgr, WM_FT_SELECTPAGE, page, 0);
}

HWND FtMgr_AddTransfer(struct FileDlgData *fdd)
{
	struct TFtMgrData *dat = (struct TFtMgrData *)GetWindowLong(FtMgr_Show(), GWL_USERDATA);
	HWND hwndBox = fdd->send ? dat->hwndOutgoing : dat->hwndIncoming;
	HWND hwndFt = CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FILETRANSFERINFO), hwndBox, DlgProcFileTransfer, (LPARAM)fdd);
	ShowWindow(hwndFt, SW_SHOW);
	SendMessage(hwndBox, WM_FT_ADD, 0, (LPARAM)hwndFt);
	FtMgr_ShowPage(fdd->send ? 1 : 0);
	return hwndFt;
}
