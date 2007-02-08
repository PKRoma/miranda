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

extern BOOL(WINAPI *MySetLayeredWindowAttributes)(HWND, COLORREF, BYTE, DWORD);
extern struct CluiData g_CluiData;
extern WNDPROC OldStatusBarProc;
extern HANDLE hExtraImageApplying;
extern SIZE g_oldSize;
extern POINT g_oldPos;
extern COLORREF g_CLUISkinnedBkColorRGB;
extern HPEN g_hPenCLUIFrames;
 
static int opt_clui_changed = 0;

BOOL CALLBACK DlgProcCluiOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
            opt_clui_changed = 0;
			TranslateDialogDefault(hwndDlg);
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
		if ((LOWORD(wParam) == IDC_FRAMEGAP || LOWORD(wParam) == IDC_HIDETIME || LOWORD(wParam) == IDC_CLIPBORDER || LOWORD(wParam) == IDC_ROWGAP || LOWORD(wParam) == IDC_TITLETEXT ||
			LOWORD(wParam) == IDC_MAXSIZEHEIGHT || LOWORD(wParam) == IDC_CLEFT || LOWORD(wParam) == IDC_CRIGHT || LOWORD(wParam) == IDC_CTOP
			|| LOWORD(wParam) == IDC_CBOTTOM) && (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
			return 0;
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
        opt_clui_changed = 1;
		break;

	case WM_HSCROLL:
		{
			char str[10];
			wsprintfA(str, "%d%%", 100 * SendDlgItemMessage(hwndDlg, IDC_TRANSINACTIVE, TBM_GETPOS, 0, 0) / 255);
			SetDlgItemTextA(hwndDlg, IDC_INACTIVEPERC, str);
			wsprintfA(str, "%d%%", 100 * SendDlgItemMessage(hwndDlg, IDC_TRANSACTIVE, TBM_GETPOS, 0, 0) / 255);
			SetDlgItemTextA(hwndDlg, IDC_ACTIVEPERC, str);
		}
		if (wParam != 0x12345678) {
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            opt_clui_changed = 1;
        }
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code) {
		case PSN_APPLY:
			{
				BOOL translated;
				BYTE oldFading;
				BYTE windowStyle = (BYTE)SendDlgItemMessage(hwndDlg, IDC_BORDERSTYLE, CB_GETCURSEL, 0, 0);
				COLORREF clr_cluiframes;

                if(!opt_clui_changed)
                    return TRUE;

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

				g_CluiData.bClipBorder = (BYTE)GetDlgItemInt(hwndDlg, IDC_CLIPBORDER, &translated, FALSE);
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
                ShowWindow(pcli->hwndContactList, SW_SHOW);
                SendMessage(pcli->hwndContactList, WM_SIZE, 0, 0);
                SetWindowPos(pcli->hwndContactList, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
                RedrawWindow(pcli->hwndContactList, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
				g_CluiData.fadeinout = oldFading;
				SFL_SetState(g_CluiData.bUseFloater & CLUI_FLOATER_AUTOHIDE ? (DBGetContactSettingByte(NULL, "CList", "State", SETTING_STATE_NORMAL) == SETTING_STATE_NORMAL ? 0 : 1) : 1);
                opt_clui_changed = 0;
				return TRUE;
			}
		}
		break;
	}
	return FALSE;
}

static int opt_sbar_changed = 0;

BOOL CALLBACK DlgProcSBarOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
    case WM_INITDIALOG:
        opt_sbar_changed = 0;
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
        opt_sbar_changed = 1;
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code) {
        case PSN_APPLY:
            if(!opt_sbar_changed)
                return TRUE;

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
            opt_sbar_changed = 0;
			return TRUE;
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
