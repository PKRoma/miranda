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

UNICODE done

*/
#include "commonheaders.h"

void ReloadExtraIcons( void );

extern BOOL(WINAPI *MySetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD);
extern struct CluiData g_CluiData;
extern pfnDrawAlpha pDrawAlpha;
extern DWORD g_gdiplusToken;
extern WNDPROC OldStatusBarProc;
extern HANDLE hExtraImageApplying;
extern SIZE g_oldSize;
extern POINT g_oldPos;
extern HIMAGELIST himlExtraImages;
extern COLORREF g_CLUISkinnedBkColorRGB;
extern HPEN g_hPenCLUIFrames;

static BOOL CALLBACK DlgProcCluiOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcSBarOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcPlusOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

static UINT expertOnlyControls[] = {
	IDC_BRINGTOFRONT, IDC_AUTOSIZE, IDC_STATIC21, IDC_MAXSIZEHEIGHT, IDC_MAXSIZESPIN, IDC_STATIC22, IDC_AUTOSIZEUPWARD, IDC_SHOWMAINMENU, IDC_CLIENTDRAG
};

static void __setFlag(DWORD dwFlag, int iMode)
{
	g_CluiData.dwFlags = iMode ? g_CluiData.dwFlags | dwFlag : g_CluiData.dwFlags & ~dwFlag;
}

int CluiOptInit(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;

	ZeroMemory(&odp, sizeof(odp));
	odp.cbSize = sizeof(odp);
	odp.position = 0;
	odp.hInstance = g_hInst;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_CLUI);
	odp.pszTitle = Translate("Window");
	odp.pszGroup = Translate("Contact List");
	odp.pfnDlgProc = DlgProcCluiOpts;
	odp.flags = ODPF_BOLDGROUPS;
	odp.nIDBottomSimpleControl = IDC_STWINDOWGROUP;
	odp.expertOnlyControls = expertOnlyControls;
	odp.nExpertOnlyControls = sizeof(expertOnlyControls) / sizeof(expertOnlyControls[0]);
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_SBAR);
	odp.pszTitle = Translate("Status Bar");
	odp.pfnDlgProc = DlgProcSBarOpts;
	odp.flags = ODPF_BOLDGROUPS | ODPF_EXPERTONLY;
	odp.nIDBottomSimpleControl = 0;
	odp.nExpertOnlyControls = 0;
	odp.expertOnlyControls = NULL;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_PLUS);
	odp.pszTitle = Translate("Advanced options");
	odp.pfnDlgProc = DlgProcPlusOpts;
	odp.flags = ODPF_BOLDGROUPS;
	odp.nIDBottomSimpleControl = 0;
	odp.nExpertOnlyControls = 0;
	odp.expertOnlyControls = NULL;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);
	return 0;
}

static BOOL CALLBACK DlgProcCluiOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);

			CheckDlgButton(hwndDlg, IDC_SHOWCLIENTICONS, g_CluiData.dwExtraImageMask & EIMG_SHOW_CLIENT);
			CheckDlgButton(hwndDlg, IDC_SHOWEXTENDEDSTATUS, g_CluiData.dwExtraImageMask & EIMG_SHOW_EXTRA);
			CheckDlgButton(hwndDlg, IDC_EXTRAMAIL, g_CluiData.dwExtraImageMask & EIMG_SHOW_MAIL);
			CheckDlgButton(hwndDlg, IDC_EXTRAWEB, g_CluiData.dwExtraImageMask & EIMG_SHOW_URL);
			CheckDlgButton(hwndDlg, IDC_EXTRAPHONE, g_CluiData.dwExtraImageMask & EIMG_SHOW_SMS);
			CheckDlgButton(hwndDlg, IDC_EXTRARESERVED, g_CluiData.dwExtraImageMask & EIMG_SHOW_RESERVED);
			CheckDlgButton(hwndDlg, IDC_EXTRARESERVED2, g_CluiData.dwExtraImageMask & EIMG_SHOW_RESERVED2);
            CheckDlgButton(hwndDlg, IDC_EXTRARESERVED3, g_CluiData.dwExtraImageMask & EIMG_SHOW_RESERVED3);
            CheckDlgButton(hwndDlg, IDC_EXTRARESERVED4, g_CluiData.dwExtraImageMask & EIMG_SHOW_RESERVED4);
            CheckDlgButton(hwndDlg, IDC_EXTRARESERVED5, g_CluiData.dwExtraImageMask & EIMG_SHOW_RESERVED5);

			CheckDlgButton(hwndDlg, IDC_BRINGTOFRONT, DBGetContactSettingByte(NULL, "CList", "BringToFront", SETTING_BRINGTOFRONT_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_ONTOP, DBGetContactSettingByte(NULL, "CList", "OnTop", SETTING_ONTOP_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_MIN2TRAY, DBGetContactSettingByte(NULL, "CList", "Min2Tray", SETTING_MIN2TRAY_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOWMAINMENU, DBGetContactSettingByte(NULL, "CLUI", "ShowMainMenu", SETTING_SHOWMAINMENU_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CLIENTDRAG, DBGetContactSettingByte(NULL, "CLUI", "ClientAreaDrag", SETTING_CLIENTDRAG_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_FADEINOUT, g_CluiData.fadeinout ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_AUTOSIZE, g_CluiData.autosize);
			CheckDlgButton(hwndDlg, IDC_DROPSHADOW, DBGetContactSettingByte(NULL, "CList", "WindowShadow", 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_ONDESKTOP, DBGetContactSettingByte(NULL, "CList", "OnDesktop", 0) ? BST_CHECKED : BST_UNCHECKED);

			SendDlgItemMessage(hwndDlg, IDC_BORDERSTYLE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Title bar"));
			SendDlgItemMessage(hwndDlg, IDC_BORDERSTYLE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Tool Window"));
			SendDlgItemMessage(hwndDlg, IDC_BORDERSTYLE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Thin border"));
			SendDlgItemMessage(hwndDlg, IDC_BORDERSTYLE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("No border"));
			SendDlgItemMessage(hwndDlg, IDC_BORDERSTYLE, CB_SETCURSEL, DBGetContactSettingByte(NULL, "CLUI", "WindowStyle", 1), 0);

			SendDlgItemMessage(hwndDlg, IDC_MAXSIZESPIN, UDM_SETRANGE, 0, MAKELONG(100, 0));
			SendDlgItemMessage(hwndDlg, IDC_MAXSIZESPIN, UDM_SETPOS, 0, DBGetContactSettingByte(NULL, "CLUI", "MaxSizeHeight", 75));

			SendDlgItemMessage(hwndDlg, IDC_CLIPBORDERSPIN, UDM_SETRANGE, 0, MAKELONG(10, 0));
			SendDlgItemMessage(hwndDlg, IDC_CLIPBORDERSPIN, UDM_SETPOS, 0, g_CluiData.bClipBorder);

			SendDlgItemMessage(hwndDlg, IDC_CLEFTSPIN, UDM_SETRANGE, 0, MAKELONG(255, 0));
			SendDlgItemMessage(hwndDlg, IDC_CRIGHTSPIN, UDM_SETRANGE, 0, MAKELONG(255, 0));
			SendDlgItemMessage(hwndDlg, IDC_CTOPSPIN, UDM_SETRANGE, 0, MAKELONG(255, 0));
			SendDlgItemMessage(hwndDlg, IDC_CBOTTOMSPIN, UDM_SETRANGE, 0, MAKELONG(255, 0));

			SendDlgItemMessage(hwndDlg, IDC_CLUIFRAMESBDR, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, "CLUI", "clr_frameborder", RGB(40, 40, 40)));

			SendDlgItemMessage(hwndDlg, IDC_CLEFTSPIN, UDM_SETPOS, 0, g_CluiData.bCLeft - (g_CluiData.dwFlags & CLUI_FRAME_CLISTSUNKEN ? 3 : 0));
			SendDlgItemMessage(hwndDlg, IDC_CRIGHTSPIN, UDM_SETPOS, 0, g_CluiData.bCRight - (g_CluiData.dwFlags & CLUI_FRAME_CLISTSUNKEN ? 3 : 0));
			SendDlgItemMessage(hwndDlg, IDC_CTOPSPIN, UDM_SETPOS, 0, g_CluiData.bCTop);
			SendDlgItemMessage(hwndDlg, IDC_CBOTTOMSPIN, UDM_SETPOS, 0, g_CluiData.bCBottom);

			CheckDlgButton(hwndDlg, IDC_AUTOSIZEUPWARD, DBGetContactSettingByte(NULL, "CLUI", "AutoSizeUpward", 0) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_AUTOHIDE, DBGetContactSettingByte(NULL, "CList", "AutoHide", SETTING_AUTOHIDE_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			SendDlgItemMessage(hwndDlg, IDC_HIDETIMESPIN, UDM_SETRANGE, 0, MAKELONG(900, 1));
			SendDlgItemMessage(hwndDlg, IDC_HIDETIMESPIN, UDM_SETPOS, 0, MAKELONG(DBGetContactSettingWord(NULL, "CList", "HideTime", SETTING_HIDETIME_DEFAULT), 0));
			EnableWindow(GetDlgItem(hwndDlg, IDC_HIDETIME), IsDlgButtonChecked(hwndDlg, IDC_AUTOHIDE));
			EnableWindow(GetDlgItem(hwndDlg, IDC_HIDETIMESPIN), IsDlgButtonChecked(hwndDlg, IDC_AUTOHIDE));
			EnableWindow(GetDlgItem(hwndDlg, IDC_STATIC01), IsDlgButtonChecked(hwndDlg, IDC_AUTOHIDE));
			if (!IsDlgButtonChecked(hwndDlg, IDC_AUTOSIZE)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_STATIC21), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_STATIC22), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_MAXSIZEHEIGHT), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_MAXSIZESPIN), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AUTOSIZEUPWARD), FALSE);
			} {
				DBVARIANT dbv;
				if (!DBGetContactSettingTString(NULL, "CList", "TitleText", &dbv)) {
					SetDlgItemText(hwndDlg, IDC_TITLETEXT, dbv.ptszVal);
					DBFreeVariant(&dbv);
				} else
					SetDlgItemTextA(hwndDlg, IDC_TITLETEXT, MIRANDANAME);
			}

			if (!IsWinVer2000Plus()) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_FADEINOUT), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENT), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_DROPSHADOW), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_FULLTRANSPARENT), FALSE);
			} else {
				CheckDlgButton(hwndDlg, IDC_TRANSPARENT, g_CluiData.isTransparent ? BST_CHECKED : BST_UNCHECKED);
				CheckDlgButton(hwndDlg, IDC_FULLTRANSPARENT, g_CluiData.bFullTransparent ? BST_CHECKED : BST_UNCHECKED);
			}
			if (!IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENT)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_STATIC11), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_STATIC12), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSACTIVE), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSINACTIVE), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ACTIVEPERC), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_INACTIVEPERC), FALSE);
			}
			SendDlgItemMessage(hwndDlg, IDC_TRANSACTIVE, TBM_SETRANGE, FALSE, MAKELONG(1, 255));
			SendDlgItemMessage(hwndDlg, IDC_TRANSINACTIVE, TBM_SETRANGE, FALSE, MAKELONG(1, 255));
			SendDlgItemMessage(hwndDlg, IDC_TRANSACTIVE, TBM_SETPOS, TRUE, g_CluiData.alpha);
			SendDlgItemMessage(hwndDlg, IDC_TRANSINACTIVE, TBM_SETPOS, TRUE, g_CluiData.autoalpha);
			SendMessage(hwndDlg, WM_HSCROLL, 0x12345678, 0);

			CheckDlgButton(hwndDlg, IDC_ROUNDEDBORDER, g_CluiData.dwFlags & CLUI_FRAME_ROUNDEDFRAME);
			SendDlgItemMessage(hwndDlg, IDC_FRAMEGAPSPIN, UDM_SETRANGE, 0, MAKELONG(10, 0));
			SendDlgItemMessage(hwndDlg, IDC_FRAMEGAPSPIN, UDM_SETPOS, 0, (LPARAM)g_CluiData.gapBetweenFrames);

			SendDlgItemMessage(hwndDlg, IDC_EXICONSCALESPIN, UDM_SETRANGE, 0, MAKELONG(20, 8));
			SendDlgItemMessage(hwndDlg, IDC_EXICONSCALESPIN, UDM_SETPOS, 0, (LPARAM)g_CluiData.exIconScale);

			return TRUE;
		}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_AUTOHIDE) {
			EnableWindow(GetDlgItem(hwndDlg, IDC_HIDETIME), IsDlgButtonChecked(hwndDlg, IDC_AUTOHIDE));
			EnableWindow(GetDlgItem(hwndDlg, IDC_HIDETIMESPIN), IsDlgButtonChecked(hwndDlg, IDC_AUTOHIDE));
			EnableWindow(GetDlgItem(hwndDlg, IDC_STATIC01), IsDlgButtonChecked(hwndDlg, IDC_AUTOHIDE));
		} else if (LOWORD(wParam) == IDC_TRANSPARENT) {
			EnableWindow(GetDlgItem(hwndDlg, IDC_STATIC11), IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENT));
			EnableWindow(GetDlgItem(hwndDlg, IDC_STATIC12), IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENT));
			EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSACTIVE), IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENT));
			EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSINACTIVE), IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENT));
			EnableWindow(GetDlgItem(hwndDlg, IDC_ACTIVEPERC), IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENT));
			EnableWindow(GetDlgItem(hwndDlg, IDC_INACTIVEPERC), IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENT));
		} else if (LOWORD(wParam) == IDC_AUTOSIZE) {
			EnableWindow(GetDlgItem(hwndDlg, IDC_STATIC21), IsDlgButtonChecked(hwndDlg, IDC_AUTOSIZE));
			EnableWindow(GetDlgItem(hwndDlg, IDC_STATIC22), IsDlgButtonChecked(hwndDlg, IDC_AUTOSIZE));
			EnableWindow(GetDlgItem(hwndDlg, IDC_MAXSIZEHEIGHT), IsDlgButtonChecked(hwndDlg, IDC_AUTOSIZE));
			EnableWindow(GetDlgItem(hwndDlg, IDC_MAXSIZESPIN), IsDlgButtonChecked(hwndDlg, IDC_AUTOSIZE));
			EnableWindow(GetDlgItem(hwndDlg, IDC_AUTOSIZEUPWARD), IsDlgButtonChecked(hwndDlg, IDC_AUTOSIZE));
		}
		if ((LOWORD(wParam) == IDC_EXICONSCALE || LOWORD(wParam) == IDC_FRAMEGAP || LOWORD(wParam) == IDC_HIDETIME || LOWORD(wParam) == IDC_CLIPBORDER || LOWORD(wParam) == IDC_ROWGAP || LOWORD(wParam) == IDC_TITLETEXT ||
			LOWORD(wParam) == IDC_MAXSIZEHEIGHT || LOWORD(wParam) == IDC_CLEFT || LOWORD(wParam) == IDC_CRIGHT || LOWORD(wParam) == IDC_CTOP
			|| LOWORD(wParam) == IDC_CBOTTOM) && (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
			return 0;
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		break;

	case WM_HSCROLL:
		{
			char str[10];
			wsprintfA(str, "%d%%", 100 * SendDlgItemMessage(hwndDlg, IDC_TRANSINACTIVE, TBM_GETPOS, 0, 0) / 255);
			SetDlgItemTextA(hwndDlg, IDC_INACTIVEPERC, str);
			wsprintfA(str, "%d%%", 100 * SendDlgItemMessage(hwndDlg, IDC_TRANSACTIVE, TBM_GETPOS, 0, 0) / 255);
			SetDlgItemTextA(hwndDlg, IDC_ACTIVEPERC, str);
		}
		if (wParam != 0x12345678)
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code) {
		case PSN_APPLY:
			{
				BOOL translated;
				int oldexIconScale = g_CluiData.exIconScale;
				BYTE oldFading;
				BYTE windowStyle = (BYTE)SendDlgItemMessage(hwndDlg, IDC_BORDERSTYLE, CB_GETCURSEL, 0, 0);
				COLORREF clr_cluiframes;

				DBWriteContactSettingByte(NULL, "CLUI", "FadeInOut", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_FADEINOUT));
				g_CluiData.fadeinout = IsDlgButtonChecked(hwndDlg, IDC_FADEINOUT) ? 1 : 0;
				oldFading = g_CluiData.fadeinout;
				g_CluiData.fadeinout = FALSE;

				DBWriteContactSettingByte(NULL, "CLUI", "WindowStyle", windowStyle);
				g_CluiData.gapBetweenFrames = GetDlgItemInt(hwndDlg, IDC_FRAMEGAP, &translated, FALSE);

				DBWriteContactSettingDword(NULL, "CLUIFrames", "GapBetweenFrames", g_CluiData.gapBetweenFrames);
				DBWriteContactSettingByte(NULL, "CList", "OnTop", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_ONTOP));
				SetWindowPos(pcli->hwndContactList, IsDlgButtonChecked(hwndDlg, IDC_ONTOP) ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

				g_CluiData.bCLeft = (BYTE)SendDlgItemMessage(hwndDlg, IDC_CLEFTSPIN, UDM_GETPOS, 0, 0);
				g_CluiData.bCRight = (BYTE)SendDlgItemMessage(hwndDlg, IDC_CRIGHTSPIN, UDM_GETPOS, 0, 0);
				g_CluiData.bCTop = (BYTE)SendDlgItemMessage(hwndDlg, IDC_CTOPSPIN, UDM_GETPOS, 0, 0);
				g_CluiData.bCBottom = (BYTE)SendDlgItemMessage(hwndDlg, IDC_CBOTTOMSPIN, UDM_GETPOS, 0, 0);

				DBWriteContactSettingDword(NULL, "CLUI", "clmargins", MAKELONG(MAKEWORD(g_CluiData.bCLeft, g_CluiData.bCRight), MAKEWORD(g_CluiData.bCTop, g_CluiData.bCBottom)));
				SendMessage(pcli->hwndContactList, WM_SIZE, 0, 0);

                DBWriteContactSettingByte(NULL, "CList", "BringToFront", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_BRINGTOFRONT));
				if (windowStyle != SETTING_WINDOWSTYLE_DEFAULT) {
					// Window must be hidden to dynamically remove the taskbar button.
					// See http://msdn.microsoft.com/library/en-us/shellcc/platform/shell/programmersguide/shell_int/shell_int_programming/taskbar.asp
					WINDOWPLACEMENT p;
					p.length = sizeof(p);
					GetWindowPlacement(pcli->hwndContactList, &p);
					ShowWindow(pcli->hwndContactList, SW_HIDE);
					SetWindowLong(pcli->hwndContactList, GWL_EXSTYLE, GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);
					SetWindowPlacement(pcli->hwndContactList, &p);
					ShowWindow(pcli->hwndContactList, SW_SHOW);
				} else
					SetWindowLong(pcli->hwndContactList, GWL_EXSTYLE, GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE) & ~WS_EX_TOOLWINDOW);

                if (IsDlgButtonChecked(hwndDlg, IDC_ONDESKTOP)) {
                    HWND hProgMan = FindWindowA("Progman", NULL);
                    if (IsWindow(hProgMan)) {
                        SetParent(pcli->hwndContactList, hProgMan);
                        SetWindowLong(pcli->hwndContactList, GWL_EXSTYLE, GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE) & ~WS_EX_LAYERED);
                    }
                } else
                    SetParent(pcli->hwndContactList, NULL);

				__setFlag(CLUI_SHOWCLIENTICONS, IsDlgButtonChecked(hwndDlg, IDC_SHOWCLIENTICONS));
				__setFlag(CLUI_SHOWXSTATUS, IsDlgButtonChecked(hwndDlg, IDC_SHOWEXTENDEDSTATUS));

				g_CluiData.dwExtraImageMask = (IsDlgButtonChecked(hwndDlg, IDC_EXTRAMAIL) ? EIMG_SHOW_MAIL : 0) |
					(IsDlgButtonChecked(hwndDlg, IDC_EXTRAWEB) ? EIMG_SHOW_URL : 0) |
					(IsDlgButtonChecked(hwndDlg, IDC_SHOWEXTENDEDSTATUS) ? EIMG_SHOW_EXTRA : 0) |
					(IsDlgButtonChecked(hwndDlg, IDC_EXTRAPHONE) ? EIMG_SHOW_SMS : 0) |
					(IsDlgButtonChecked(hwndDlg, IDC_EXTRARESERVED) ? EIMG_SHOW_RESERVED : 0) |
					(IsDlgButtonChecked(hwndDlg, IDC_SHOWCLIENTICONS) ? EIMG_SHOW_CLIENT : 0) |
					(IsDlgButtonChecked(hwndDlg, IDC_EXTRARESERVED2) ? EIMG_SHOW_RESERVED2 : 0) |
                    (IsDlgButtonChecked(hwndDlg, IDC_EXTRARESERVED3) ? EIMG_SHOW_RESERVED3 : 0) |
                    (IsDlgButtonChecked(hwndDlg, IDC_EXTRARESERVED4) ? EIMG_SHOW_RESERVED4 : 0) |
                    (IsDlgButtonChecked(hwndDlg, IDC_EXTRARESERVED5) ? EIMG_SHOW_RESERVED5 : 0);

				g_CluiData.bClipBorder = (BYTE)GetDlgItemInt(hwndDlg, IDC_CLIPBORDER, &translated, FALSE);

				DBWriteContactSettingDword(NULL, "CLUI", "ximgmask", g_CluiData.dwExtraImageMask);
				DBWriteContactSettingDword(NULL, "CLUI", "Frameflags", g_CluiData.dwFlags);
				DBWriteContactSettingByte(NULL, "CLUI", "clipborder", g_CluiData.bClipBorder);

				DBWriteContactSettingByte(NULL, "CLUI", "ShowMainMenu", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWMAINMENU));
				DBWriteContactSettingByte(NULL, "CLUI", "ClientAreaDrag", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_CLIENTDRAG));

				clr_cluiframes = (COLORREF)SendDlgItemMessage(hwndDlg, IDC_CLUIFRAMESBDR, CPM_GETCOLOUR, 0, 0);

				if(g_hPenCLUIFrames)
					DeleteObject(g_hPenCLUIFrames);
				g_hPenCLUIFrames = CreatePen(PS_SOLID, 1, clr_cluiframes);
				DBWriteContactSettingDword(NULL, "CLUI", "clr_frameborder", clr_cluiframes);

				ApplyCLUIBorderStyle(pcli->hwndContactList);

				if (!IsDlgButtonChecked(hwndDlg, IDC_SHOWMAINMENU))
					SetMenu(pcli->hwndContactList, NULL);
				else
					SetMenu(pcli->hwndContactList, pcli->hMenuMain);

				DBWriteContactSettingByte(NULL, "CList", "Min2Tray", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_MIN2TRAY));
				if (IsIconic(pcli->hwndContactList) && windowStyle != SETTING_WINDOWSTYLE_TOOLWINDOW)
					ShowWindow(pcli->hwndContactList, IsDlgButtonChecked(hwndDlg, IDC_MIN2TRAY) ? SW_HIDE : SW_SHOW); 
				{
					TCHAR title[256];
					GetDlgItemText(hwndDlg, IDC_TITLETEXT, title, sizeof(title));
					DBWriteContactSettingTString(NULL, "CList", "TitleText", title);
					SetWindowText(pcli->hwndContactList, title);
				}
				g_CluiData.dwFlags = IsDlgButtonChecked(hwndDlg, IDC_ROUNDEDBORDER) ? g_CluiData.dwFlags | CLUI_FRAME_ROUNDEDFRAME : g_CluiData.dwFlags & ~CLUI_FRAME_ROUNDEDFRAME;
				DBWriteContactSettingByte(NULL, "CLUI", "AutoSize", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AUTOSIZE));
				
				if((g_CluiData.autosize = IsDlgButtonChecked(hwndDlg, IDC_AUTOSIZE) ? 1 : 0)) {
					SendMessage(pcli->hwndContactList, WM_SIZE, 0, 0);
					SendMessage(pcli->hwndContactTree, WM_SIZE, 0, 0);
				}

				DBWriteContactSettingByte(NULL, "CLUI", "MaxSizeHeight", (BYTE) GetDlgItemInt(hwndDlg, IDC_MAXSIZEHEIGHT, NULL, FALSE));
				DBWriteContactSettingByte(NULL, "CLUI", "AutoSizeUpward", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AUTOSIZEUPWARD));
				DBWriteContactSettingByte(NULL, "CList", "AutoHide", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AUTOHIDE));
				DBWriteContactSettingWord(NULL, "CList", "HideTime", (WORD) SendDlgItemMessage(hwndDlg, IDC_HIDETIMESPIN, UDM_GETPOS, 0, 0));

				DBWriteContactSettingByte(NULL, "CList", "Transparent", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENT));
				g_CluiData.isTransparent = IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENT) ? 1 : 0;
				DBWriteContactSettingByte(NULL, "CList", "Alpha", (BYTE) SendDlgItemMessage(hwndDlg, IDC_TRANSACTIVE, TBM_GETPOS, 0, 0));
				g_CluiData.alpha = (BYTE) SendDlgItemMessage(hwndDlg, IDC_TRANSACTIVE, TBM_GETPOS, 0, 0);
				DBWriteContactSettingByte(NULL, "CList", "AutoAlpha", (BYTE) SendDlgItemMessage(hwndDlg, IDC_TRANSINACTIVE, TBM_GETPOS, 0, 0));
				g_CluiData.autoalpha = (BYTE) SendDlgItemMessage(hwndDlg, IDC_TRANSINACTIVE, TBM_GETPOS, 0, 0);
				DBWriteContactSettingByte(NULL, "CList", "WindowShadow", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_DROPSHADOW));
				DBWriteContactSettingByte(NULL, "CList", "OnDesktop", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_ONDESKTOP));
				DBWriteContactSettingDword(NULL, "CLUI", "Frameflags", g_CluiData.dwFlags);
				g_CluiData.bFullTransparent = IsDlgButtonChecked(hwndDlg, IDC_FULLTRANSPARENT) ? 1 : 0;
				DBWriteContactSettingByte(NULL, "CLUI", "fulltransparent", (BYTE)g_CluiData.bFullTransparent);

				g_CluiData.exIconScale = SendDlgItemMessage(hwndDlg, IDC_EXICONSCALESPIN, UDM_GETPOS, 0, 0);
				g_CluiData.exIconScale = (g_CluiData.exIconScale < 8 || g_CluiData.exIconScale > 20) ? 16 : g_CluiData.exIconScale;

				DBWriteContactSettingByte(NULL, "CLC", "ExIconScale", (BYTE)g_CluiData.exIconScale);
				if (g_CluiData.bLayeredHack && MySetLayeredWindowAttributes)
					SetWindowLong(pcli->hwndContactList, GWL_EXSTYLE, GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE) | WS_EX_LAYERED);

				if(g_CLUISkinnedBkColorRGB)
					g_CluiData.colorkey = g_CLUISkinnedBkColorRGB;
				else if(g_CluiData.bClipBorder == 0 && !(g_CluiData.dwFlags & CLUI_FRAME_ROUNDEDFRAME))
					g_CluiData.colorkey = DBGetContactSettingDword(NULL, "CLC", "BkColour", CLCDEFAULT_BKCOLOUR);
				else {
					SendMessage(pcli->hwndContactList, WM_SIZE, 0, 0);
					g_CluiData.colorkey = RGB(255, 0, 255);
				}
				if (g_CluiData.isTransparent || g_CluiData.bFullTransparent) {
					if(MySetLayeredWindowAttributes) {
						SetWindowLong(pcli->hwndContactList, GWL_EXSTYLE, GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE) & ~WS_EX_LAYERED);
						SetWindowLong(pcli->hwndContactList, GWL_EXSTYLE, GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE) | WS_EX_LAYERED);
						MySetLayeredWindowAttributes(pcli->hwndContactList, 0, 255, LWA_ALPHA | LWA_COLORKEY);
						MySetLayeredWindowAttributes(pcli->hwndContactList, 
							(COLORREF)(g_CluiData.bFullTransparent ? g_CluiData.colorkey : 0),
							(BYTE)(g_CluiData.isTransparent ? g_CluiData.autoalpha : 255),
							(DWORD)((g_CluiData.isTransparent ? LWA_ALPHA : 0L) | (g_CluiData.bFullTransparent ? LWA_COLORKEY : 0L)));
					}
				} else {
					if (MySetLayeredWindowAttributes)
						MySetLayeredWindowAttributes(pcli->hwndContactList, RGB(0, 0, 0), (BYTE)255, LWA_ALPHA);
					if(!g_CluiData.bLayeredHack)
						SetWindowLong(pcli->hwndContactList, GWL_EXSTYLE, GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE) & ~WS_EX_LAYERED);
				}

                ConfigureCLUIGeometry(1);
				if(oldexIconScale != g_CluiData.exIconScale) {
					ImageList_RemoveAll(himlExtraImages);
					ImageList_SetIconSize(himlExtraImages, g_CluiData.exIconScale, g_CluiData.exIconScale);
					if(g_CluiData.IcoLib_Avail)
						IcoLibReloadIcons();
					else {
						CLN_LoadAllIcons(0);
						pcli->pfnReloadProtoMenus();
                        //FYR: Not necessary. It is already notified in pfnReloadProtoMenus
                        //NotifyEventHooks(pcli->hPreBuildStatusMenuEvent, 0, 0);
						ReloadExtraIcons();
					}
					pcli->pfnClcBroadcast(CLM_AUTOREBUILD, 0, 0);
				}
                ShowWindow(pcli->hwndContactList, SW_SHOW);
                SendMessage(pcli->hwndContactList, WM_SIZE, 0, 0);
                SetWindowPos(pcli->hwndContactList, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
                RedrawWindow(pcli->hwndContactList, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
				g_CluiData.fadeinout = oldFading;
				SFL_SetState(g_CluiData.bUseFloater & CLUI_FLOATER_AUTOHIDE ? (DBGetContactSettingByte(NULL, "CList", "State", SETTING_STATE_NORMAL) == SETTING_STATE_NORMAL ? 0 : 1) : 1);
				return TRUE;
			}
		}
		break;
	}
	return FALSE;
}

static BOOL CALLBACK DlgProcSBarOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		CheckDlgButton(hwndDlg, IDC_SHOWSBAR, DBGetContactSettingByte(NULL, "CLUI", "ShowSBar", 1) ? BST_CHECKED : BST_UNCHECKED); {
			BYTE showOpts = DBGetContactSettingByte(NULL, "CLUI", "SBarShow", 1);
			CheckDlgButton(hwndDlg, IDC_SHOWICON, showOpts & 1 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOWPROTO, showOpts & 2 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOWSTATUS, showOpts & 4 ? BST_CHECKED : BST_UNCHECKED);
		}
		CheckDlgButton(hwndDlg, IDC_RIGHTSTATUS, DBGetContactSettingByte(NULL, "CLUI", "SBarRightClk", 0) ? BST_UNCHECKED : BST_CHECKED);
		CheckDlgButton(hwndDlg, IDC_RIGHTMIRANDA, !IsDlgButtonChecked(hwndDlg, IDC_RIGHTSTATUS) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_EQUALSECTIONS, g_CluiData.bEqualSections ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SBPANELBEVEL, DBGetContactSettingByte(NULL, "CLUI", "SBarBevel", 1) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SHOWGRIP, DBGetContactSettingByte(NULL, "CLUI", "ShowGrip", 1) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SKINBACKGROUND, g_CluiData.bSkinnedStatusBar);
		CheckDlgButton(hwndDlg, IDC_SHOWXSTATUS, g_CluiData.bShowXStatusOnSbar);
        CheckDlgButton(hwndDlg, IDC_MARKLOCKED, DBGetContactSettingByte(NULL, "CLUI", "sbar_showlocked", 1));

		if (!IsDlgButtonChecked(hwndDlg, IDC_SHOWSBAR)) {
			EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWICON), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWPROTO), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWSTATUS), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_RIGHTSTATUS), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_RIGHTMIRANDA), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EQUALSECTIONS), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_SBPANELBEVEL), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWGRIP), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_SKINBACKGROUND), FALSE);
		}
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_SHOWSBAR) {
			EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWICON), IsDlgButtonChecked(hwndDlg, IDC_SHOWSBAR));
			EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWPROTO), IsDlgButtonChecked(hwndDlg, IDC_SHOWSBAR));
			EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWSTATUS), IsDlgButtonChecked(hwndDlg, IDC_SHOWSBAR));
			EnableWindow(GetDlgItem(hwndDlg, IDC_RIGHTSTATUS), IsDlgButtonChecked(hwndDlg, IDC_SHOWSBAR));
			EnableWindow(GetDlgItem(hwndDlg, IDC_RIGHTMIRANDA), IsDlgButtonChecked(hwndDlg, IDC_SHOWSBAR));
			EnableWindow(GetDlgItem(hwndDlg, IDC_EQUALSECTIONS), IsDlgButtonChecked(hwndDlg, IDC_SHOWSBAR));
			EnableWindow(GetDlgItem(hwndDlg, IDC_SBPANELBEVEL), IsDlgButtonChecked(hwndDlg, IDC_SHOWSBAR));
			EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWGRIP), IsDlgButtonChecked(hwndDlg, IDC_SHOWSBAR));
			EnableWindow(GetDlgItem(hwndDlg, IDC_SKINBACKGROUND), IsDlgButtonChecked(hwndDlg, IDC_SHOWSBAR));
		}
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code) {
		case PSN_APPLY:
			DBWriteContactSettingByte(NULL, "CLUI", "ShowSBar", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWSBAR));
			DBWriteContactSettingByte(NULL, "CLUI", "SBarShow", (BYTE) ((IsDlgButtonChecked(hwndDlg, IDC_SHOWICON) ? 1 : 0) | (IsDlgButtonChecked(hwndDlg, IDC_SHOWPROTO) ? 2 : 0) | (IsDlgButtonChecked(hwndDlg, IDC_SHOWSTATUS) ? 4 : 0)));
			DBWriteContactSettingByte(NULL, "CLUI", "SBarRightClk", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_RIGHTMIRANDA));
			DBWriteContactSettingByte(NULL, "CLUI", "EqualSections", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_EQUALSECTIONS));
			DBWriteContactSettingByte(NULL, "CLUI", "sb_skinned", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SKINBACKGROUND));
            DBWriteContactSettingByte(NULL, "CLUI", "sbar_showlocked", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_MARKLOCKED));

			g_CluiData.bEqualSections = IsDlgButtonChecked(hwndDlg, IDC_EQUALSECTIONS) ? 1 : 0;
			g_CluiData.bSkinnedStatusBar = IsDlgButtonChecked(hwndDlg, IDC_SKINBACKGROUND) ? 1 : 0;
			g_CluiData.bShowXStatusOnSbar = IsDlgButtonChecked(hwndDlg, IDC_SHOWXSTATUS) ? 1 : 0;
			DBWriteContactSettingByte(NULL, "CLUI", "xstatus_sbar", (BYTE)g_CluiData.bShowXStatusOnSbar);
			DBWriteContactSettingByte(NULL, "CLUI", "SBarBevel", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SBPANELBEVEL));
			if (DBGetContactSettingByte(NULL, "CLUI", "ShowGrip", 1) != (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWGRIP)) {
				HWND parent = GetParent(pcli->hwndStatus);
				int flags = WS_CHILD | CCS_BOTTOM;
				DBWriteContactSettingByte(NULL, "CLUI", "ShowGrip", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWGRIP));
				ShowWindow(pcli->hwndStatus, SW_HIDE);
				SetWindowLong(pcli->hwndStatus, GWL_WNDPROC, (LONG)OldStatusBarProc);
				DestroyWindow(pcli->hwndStatus);
				flags |= DBGetContactSettingByte(NULL, "CLUI", "ShowSBar", 1) ? WS_VISIBLE : 0;
				flags |= DBGetContactSettingByte(NULL, "CLUI", "ShowGrip", 1) ? SBARS_SIZEGRIP : 0;
				pcli->hwndStatus = CreateWindow(STATUSCLASSNAME, NULL, flags, 0, 0, 0, 0, parent, NULL, g_hInst, NULL);
				OldStatusBarProc = (WNDPROC)SetWindowLong(pcli->hwndStatus, GWL_WNDPROC, (LONG)NewStatusBarWndProc);
			}
			if (IsDlgButtonChecked(hwndDlg, IDC_SHOWSBAR)) {
				ShowWindow(pcli->hwndStatus, SW_SHOW);
                SendMessage(pcli->hwndStatus, WM_SIZE, 0, 0);
				g_CluiData.dwFlags |= CLUI_FRAME_SBARSHOW;
			} else {
				ShowWindow(pcli->hwndStatus, SW_HIDE);
				g_CluiData.dwFlags &= ~CLUI_FRAME_SBARSHOW;
			}
			DBWriteContactSettingDword(NULL, "CLUI", "Frameflags", g_CluiData.dwFlags);
			ConfigureCLUIGeometry(1);
			SendMessage(pcli->hwndContactList, WM_SIZE, 0, 0);
			CluiProtocolStatusChanged(0, 0);
			PostMessage(pcli->hwndContactList, CLUIINTM_REDRAW, 0, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

static UINT avatar_controls[] = { IDC_ALIGNMENT, IDC_AVATARSBORDER, IDC_AVATARSROUNDED, IDC_AVATARBORDERCLR, IDC_ALWAYSALIGNNICK, IDC_AVATARHEIGHT, IDC_AVATARSIZESPIN, 0 };

static BOOL CALLBACK DlgProcPlusOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
			DWORD dwFlags = g_CluiData.dwFlags;
			int i = 0;

			TranslateDialogDefault(hwndDlg);

			CheckDlgButton(hwndDlg, IDC_SHOWSTATUSICONS, (dwFlags & CLUI_FRAME_STATUSICONS) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOWVISIBILITY, dwFlags & CLUI_SHOWVISI);
			CheckDlgButton(hwndDlg, IDC_SHOWMETA, dwFlags & CLUI_USEMETAICONS);
			CheckDlgButton(hwndDlg, IDC_NOAVATARSOFFLINE, g_CluiData.bNoOfflineAvatars);
			SendDlgItemMessage(hwndDlg, IDC_DUALROWMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Never"));
			SendDlgItemMessage(hwndDlg, IDC_DUALROWMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always"));
			SendDlgItemMessage(hwndDlg, IDC_DUALROWMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("When space allows it"));
			SendDlgItemMessage(hwndDlg, IDC_DUALROWMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("When needed"));

			SendDlgItemMessage(hwndDlg, IDC_DUALROWMODE, CB_SETCURSEL, (WPARAM)g_CluiData.dualRowMode, 0);

			CheckDlgButton(hwndDlg, IDC_SHOWBUTTONBAR, dwFlags & CLUI_FRAME_SHOWTOPBUTTONS);
			CheckDlgButton(hwndDlg, IDC_SHOWBOTTOMBUTTONS, dwFlags & CLUI_FRAME_SHOWBOTTOMBUTTONS);
			CheckDlgButton(hwndDlg, IDC_CLISTSUNKEN, dwFlags & CLUI_FRAME_CLISTSUNKEN);

			CheckDlgButton(hwndDlg, IDC_EVENTAREAAUTOHIDE, dwFlags & CLUI_FRAME_AUTOHIDENOTIFY);
			CheckDlgButton(hwndDlg, IDC_EVENTAREASUNKEN, (dwFlags & CLUI_FRAME_EVENTAREASUNKEN) ? BST_CHECKED : BST_UNCHECKED);

			if(g_CluiData.bAvatarServiceAvail) {
				CheckDlgButton(hwndDlg, IDC_CLISTAVATARS, (dwFlags & CLUI_FRAME_AVATARS) ? BST_CHECKED : BST_UNCHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_CLISTAVATARS), TRUE);
                while(avatar_controls[i] != 0)
                    EnableWindow(GetDlgItem(hwndDlg, avatar_controls[i++]), TRUE);
			}
			else {
				CheckDlgButton(hwndDlg, IDC_CLISTAVATARS, FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_CLISTAVATARS), FALSE);
                while(avatar_controls[i] != 0)
                    EnableWindow(GetDlgItem(hwndDlg, avatar_controls[i++]), FALSE);
			}
			CheckDlgButton(hwndDlg, IDC_AVATARSBORDER, (dwFlags & CLUI_FRAME_AVATARBORDER) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_AVATARSROUNDED, (dwFlags & CLUI_FRAME_ROUNDAVATAR) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_ALWAYSALIGNNICK, (dwFlags & CLUI_FRAME_ALWAYSALIGNNICK) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOWSTATUSMSG, (dwFlags & CLUI_FRAME_SHOWSTATUSMSG) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_OVERLAYICONS, (dwFlags & CLUI_FRAME_OVERLAYICONS) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SELECTIVEICONS, (dwFlags & CLUI_FRAME_SELECTIVEICONS) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_RENDERGDIP, (dwFlags & CLUI_FRAME_GDIPLUS) ? BST_CHECKED : BST_UNCHECKED);

			SendMessage(hwndDlg, WM_COMMAND, IDC_CLISTAVATARS, 0);
			SendDlgItemMessage(hwndDlg, IDC_AVATARBORDERCLR, CPM_SETCOLOUR, 0, g_CluiData.avatarBorder);

			SendDlgItemMessage(hwndDlg, IDC_RADIUSSPIN, UDM_SETRANGE, 0, MAKELONG(10, 2));
			SendDlgItemMessage(hwndDlg, IDC_RADIUSSPIN, UDM_SETPOS, 0, g_CluiData.avatarRadius);

			SendDlgItemMessage(hwndDlg, IDC_AVATARSIZESPIN, UDM_SETRANGE, 0, MAKELONG(100, 16));
			SendDlgItemMessage(hwndDlg, IDC_AVATARSIZESPIN, UDM_SETPOS, 0, g_CluiData.avatarSize);

			SendDlgItemMessage(hwndDlg, IDC_AVATARPADDINGSPIN, UDM_SETRANGE, 0, MAKELONG(10, 0));
			SendDlgItemMessage(hwndDlg, IDC_AVATARPADDINGSPIN, UDM_SETPOS, 0, g_CluiData.avatarPadding);

			EnableWindow(GetDlgItem(hwndDlg, IDC_RADIUS), IsDlgButtonChecked(hwndDlg, IDC_AVATARSROUNDED) ? TRUE : FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_RADIUSSPIN), IsDlgButtonChecked(hwndDlg, IDC_AVATARSROUNDED) ? TRUE : FALSE);

			EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARBORDERCLR), IsDlgButtonChecked(hwndDlg, IDC_AVATARSBORDER) ? TRUE : FALSE);

			EnableWindow(GetDlgItem(hwndDlg, IDC_RENDERGDIP), g_gdiplusToken != 0 ? TRUE : FALSE);

			SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_INSERTSTRING, -1, (LPARAM)TranslateT("With Nickname - left"));
			SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Far left"));
			SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Far right"));
			SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_INSERTSTRING, -1, (LPARAM)TranslateT("With Nickname - right"));

			SendDlgItemMessage(hwndDlg, IDC_CLISTALIGN, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Never"));
			SendDlgItemMessage(hwndDlg, IDC_CLISTALIGN, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always"));
			SendDlgItemMessage(hwndDlg, IDC_CLISTALIGN, CB_INSERTSTRING, -1, (LPARAM)TranslateT("For RTL only"));
			SendDlgItemMessage(hwndDlg, IDC_CLISTALIGN, CB_INSERTSTRING, -1, (LPARAM)TranslateT("RTL TEXT only"));

			SendDlgItemMessage(hwndDlg, IDC_CLISTALIGN, CB_SETCURSEL, g_CluiData.bUseDCMirroring, 0);

			CheckDlgButton(hwndDlg, IDC_STATUSICONSCENTERED, g_CluiData.bCenterStatusIcons ? 1 : 0);
			CheckDlgButton(hwndDlg, IDC_SHOWLOCALTIME, g_CluiData.bShowLocalTime ? 1 : 0);
			CheckDlgButton(hwndDlg, IDC_SHOWLOCALTIMEONLYWHENDIFFERENT, g_CluiData.bShowLocalTimeSelective ? 1 : 0);

			EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWLOCALTIMEONLYWHENDIFFERENT), IsDlgButtonChecked(hwndDlg, IDC_SHOWLOCALTIME));

			if(dwFlags & CLUI_FRAME_AVATARSLEFT)
				SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_SETCURSEL, 1, 0);
			else if(dwFlags & CLUI_FRAME_AVATARSRIGHT)
				SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_SETCURSEL, 2, 0);
			else if(dwFlags & CLUI_FRAME_AVATARSRIGHTWITHNICK)
				SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_SETCURSEL, 3, 0);
			else
				SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_SETCURSEL, 0, 0);
			return TRUE;
		}
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDC_CLISTAVATARS:
            if((HWND)lParam != GetFocus())
                return 0;
            break;
		case IDC_SHOWLOCALTIME:
			EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWLOCALTIMEONLYWHENDIFFERENT), IsDlgButtonChecked(hwndDlg, IDC_SHOWLOCALTIME));
			break;
		case IDC_AVATARSROUNDED:
			EnableWindow(GetDlgItem(hwndDlg, IDC_RADIUS), IsDlgButtonChecked(hwndDlg, IDC_AVATARSROUNDED) ? TRUE : FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_RADIUSSPIN), IsDlgButtonChecked(hwndDlg, IDC_AVATARSROUNDED) ? TRUE : FALSE);
			break;
		case IDC_AVATARSBORDER:
			EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARBORDERCLR), IsDlgButtonChecked(hwndDlg, IDC_AVATARSBORDER) ? TRUE : FALSE);
			break;
		case IDC_SHOWMETA:
			__setFlag(CLUI_USEMETAICONS, IsDlgButtonChecked(hwndDlg, IDC_SHOWMETA));
			pcli->pfnClcBroadcast(CLM_AUTOREBUILD, 0, 0);
			break;
		}
		if ((LOWORD(wParam) == IDC_RADIUS || LOWORD(wParam) == IDC_AVATARHEIGHT || LOWORD(wParam) == IDC_AVATARPADDING) && (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
			return 0;
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code) {
		case PSN_APPLY:
			{
				BOOL translated;
				LRESULT sel = SendDlgItemMessage(hwndDlg, IDC_ALIGNMENT, CB_GETCURSEL, 0, 0);
				BYTE bOldMirrorMode = g_CluiData.bUseDCMirroring;

				__setFlag(CLUI_FRAME_STATUSICONS, IsDlgButtonChecked(hwndDlg, IDC_SHOWSTATUSICONS));
				__setFlag(CLUI_SHOWVISI, IsDlgButtonChecked(hwndDlg, IDC_SHOWVISIBILITY));
				__setFlag(CLUI_USEMETAICONS, IsDlgButtonChecked(hwndDlg, IDC_SHOWMETA));

				__setFlag(CLUI_FRAME_AVATARS, IsDlgButtonChecked(hwndDlg, IDC_CLISTAVATARS));
				__setFlag(CLUI_FRAME_AVATARBORDER, IsDlgButtonChecked(hwndDlg, IDC_AVATARSBORDER));
				__setFlag(CLUI_FRAME_ROUNDAVATAR, IsDlgButtonChecked(hwndDlg, IDC_AVATARSROUNDED));

				__setFlag(CLUI_FRAME_EVENTAREASUNKEN, IsDlgButtonChecked(hwndDlg, IDC_EVENTAREASUNKEN));
				__setFlag(CLUI_FRAME_AUTOHIDENOTIFY, IsDlgButtonChecked(hwndDlg, IDC_EVENTAREAAUTOHIDE));

				__setFlag(CLUI_FRAME_SHOWTOPBUTTONS, IsDlgButtonChecked(hwndDlg, IDC_SHOWBUTTONBAR));
				__setFlag(CLUI_FRAME_SHOWBOTTOMBUTTONS, IsDlgButtonChecked(hwndDlg, IDC_SHOWBOTTOMBUTTONS));
				__setFlag(CLUI_FRAME_CLISTSUNKEN, IsDlgButtonChecked(hwndDlg, IDC_CLISTSUNKEN));
				__setFlag(CLUI_FRAME_ALWAYSALIGNNICK, IsDlgButtonChecked(hwndDlg, IDC_ALWAYSALIGNNICK));

				__setFlag(CLUI_FRAME_SELECTIVEICONS, IsDlgButtonChecked(hwndDlg, IDC_SELECTIVEICONS));
				__setFlag(CLUI_FRAME_OVERLAYICONS, IsDlgButtonChecked(hwndDlg, IDC_OVERLAYICONS));
				__setFlag(CLUI_FRAME_SHOWSTATUSMSG, IsDlgButtonChecked(hwndDlg, IDC_SHOWSTATUSMSG));
				__setFlag(CLUI_FRAME_GDIPLUS, IsDlgButtonChecked(hwndDlg, IDC_RENDERGDIP));

				pDrawAlpha = NULL;

				g_CluiData.dualRowMode = (BYTE)SendDlgItemMessage(hwndDlg, IDC_DUALROWMODE, CB_GETCURSEL, 0, 0);
				if(g_CluiData.dualRowMode == CB_ERR)
					g_CluiData.dualRowMode = 0;

				if(sel != CB_ERR) {
					g_CluiData.dwFlags &= ~(CLUI_FRAME_AVATARSLEFT | CLUI_FRAME_AVATARSRIGHT | CLUI_FRAME_AVATARSRIGHTWITHNICK);
					if(sel == 1)
						__setFlag(CLUI_FRAME_AVATARSLEFT, 1);
					else if(sel == 2)
						__setFlag(CLUI_FRAME_AVATARSRIGHT, 1);
					else if(sel == 3)
						__setFlag(CLUI_FRAME_AVATARSRIGHTWITHNICK, 1);
				}
				if(g_CluiData.hBrushAvatarBorder)
					DeleteObject(g_CluiData.hBrushAvatarBorder);
				g_CluiData.avatarBorder = SendDlgItemMessage(hwndDlg, IDC_AVATARBORDERCLR, CPM_GETCOLOUR, 0, 0);
				g_CluiData.hBrushAvatarBorder = CreateSolidBrush(g_CluiData.avatarBorder);
				g_CluiData.avatarRadius = GetDlgItemInt(hwndDlg, IDC_RADIUS, &translated, FALSE);
				g_CluiData.avatarSize = GetDlgItemInt(hwndDlg, IDC_AVATARHEIGHT, &translated, FALSE);
				g_CluiData.avatarPadding = GetDlgItemInt(hwndDlg, IDC_AVATARPADDING, &translated, FALSE);
				g_CluiData.bNoOfflineAvatars = IsDlgButtonChecked(hwndDlg, IDC_NOAVATARSOFFLINE) ? TRUE : FALSE;
				g_CluiData.bCenterStatusIcons = IsDlgButtonChecked(hwndDlg, IDC_STATUSICONSCENTERED) ? TRUE : FALSE;
				g_CluiData.bShowLocalTime = IsDlgButtonChecked(hwndDlg, IDC_SHOWLOCALTIME) ? 1 : 0;
				g_CluiData.bShowLocalTimeSelective = IsDlgButtonChecked(hwndDlg, IDC_SHOWLOCALTIMEONLYWHENDIFFERENT) ? 1 : 0;
				g_CluiData.bUseDCMirroring = (BYTE)SendDlgItemMessage(hwndDlg, IDC_CLISTALIGN, CB_GETCURSEL, 0, 0);
				DBWriteContactSettingByte(NULL, "CLC", "MirrorDC", g_CluiData.bUseDCMirroring);
				DBWriteContactSettingByte(NULL, "CLC", "ShowLocalTime", (BYTE)g_CluiData.bShowLocalTime);
				DBWriteContactSettingByte(NULL, "CLC", "SelectiveLocalTime", (BYTE)g_CluiData.bShowLocalTimeSelective);
				KillTimer(pcli->hwndContactTree, TIMERID_REFRESH);
				if(g_CluiData.bShowLocalTime)
					SetTimer(pcli->hwndContactTree, TIMERID_REFRESH, 65000, NULL);
				DBWriteContactSettingDword(NULL, "CLC", "avatarborder", g_CluiData.avatarBorder);
				DBWriteContactSettingDword(NULL, "CLUI", "Frameflags", g_CluiData.dwFlags);
				DBWriteContactSettingDword(NULL, "CLC", "avatarradius", g_CluiData.avatarRadius);
				DBWriteContactSettingWord(NULL, "CList", "AvatarSize", (WORD)g_CluiData.avatarSize);
				DBWriteContactSettingByte(NULL, "CLC", "DualRowMode", g_CluiData.dualRowMode);
				DBWriteContactSettingByte(NULL, "CList", "AvatarPadding", g_CluiData.avatarPadding);
				DBWriteContactSettingByte(NULL, "CList", "NoOfflineAV", (BYTE)g_CluiData.bNoOfflineAvatars);
				DBWriteContactSettingByte(NULL, "CLC", "si_centered", (BYTE)g_CluiData.bCenterStatusIcons);
				if(!pDrawAlpha)
					pDrawAlpha = (g_CluiData.dwFlags & CLUI_FRAME_GDIPLUS  && g_gdiplusToken) ? (pfnDrawAlpha)GDIp_DrawAlpha : (pfnDrawAlpha)DrawAlpha;
				ConfigureFrame();
				ConfigureCLUIGeometry(1);
				ConfigureEventArea(pcli->hwndContactList);
                HideShowNotifyFrame();
				SendMessage(pcli->hwndContactTree, WM_SIZE, 0, 0);
				SendMessage(pcli->hwndContactList, WM_SIZE, 0, 0);
                pcli->pfnClcBroadcast(CLM_AUTOREBUILD, 0, 0);
				PostMessage(pcli->hwndContactList, CLUIINTM_REDRAW, 0, 0);
				return TRUE;
			}
		}
		break;
	}
	return FALSE;
}

DWORD GetCLUIWindowStyle(BYTE style)
{
	DWORD dwBasic = WS_CLIPCHILDREN;

	if(style == SETTING_WINDOWSTYLE_THINBORDER)
		return dwBasic | WS_BORDER;
	else if(style == SETTING_WINDOWSTYLE_TOOLWINDOW || style == 0)
		return dwBasic | (WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_POPUPWINDOW | WS_THICKFRAME);
	else if(style == SETTING_WINDOWSTYLE_NOBORDER)
		return dwBasic;

	return dwBasic;
}

void ApplyCLUIBorderStyle(HWND hwnd)
{
    BYTE windowStyle = DBGetContactSettingByte(NULL, "CLUI", "WindowStyle", 0);
	WINDOWPLACEMENT p;

	p.length = sizeof(p);
	GetWindowPlacement(pcli->hwndContactList, &p);
	ShowWindow(pcli->hwndContactList, SW_HIDE);

	if (windowStyle == SETTING_WINDOWSTYLE_DEFAULT || windowStyle == SETTING_WINDOWSTYLE_TOOLWINDOW) {
		SetWindowLong(pcli->hwndContactList, GWL_STYLE, GetWindowLong(pcli->hwndContactList, GWL_STYLE) | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_POPUPWINDOW | WS_THICKFRAME);
	} else if(windowStyle == SETTING_WINDOWSTYLE_THINBORDER) {
		SetWindowLong(pcli->hwndContactList, GWL_STYLE, GetWindowLong(pcli->hwndContactList, GWL_STYLE) & ~(WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_POPUPWINDOW | WS_THICKFRAME));
		SetWindowLong(pcli->hwndContactList, GWL_STYLE, GetWindowLong(pcli->hwndContactList, GWL_STYLE) | WS_BORDER | WS_CLIPCHILDREN);
	}
	else {
        SetWindowLong(pcli->hwndContactList, GWL_STYLE, GetWindowLong(pcli->hwndContactList, GWL_STYLE) & ~(WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_POPUPWINDOW | WS_THICKFRAME));
        SetWindowLong(pcli->hwndContactList, GWL_STYLE, GetWindowLong(pcli->hwndContactList, GWL_STYLE) | WS_CLIPCHILDREN);
    }

	p.showCmd = SW_HIDE;
	SetWindowPlacement(pcli->hwndContactList, &p);
}
