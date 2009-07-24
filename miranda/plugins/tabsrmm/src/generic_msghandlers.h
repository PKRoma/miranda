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

The Hotkey-Handler is a hidden dialog window which needs to be in place for
handling the global hotkeys registered by tabSRMM.

$Id$

*/

/*
 * prototypes from generic_msghandlers.c
*/

void DM_SetDBButtonStates(HWND hwndChild, struct _MessageWindowData *dat);
int BTN_GetStockItem(ButtonItem *item, const char *szName);
HWND DM_CreateClist(HWND hwndParent, struct _MessageWindowData *dat);

LRESULT DM_ScrollToBottom(HWND hwndDlg, struct _MessageWindowData *dat, WPARAM wParam, LPARAM lParam);
LRESULT DM_LoadLocale(HWND hwndDlg, struct _MessageWindowData *dat);
LRESULT DM_SaveLocale(HWND hwndDlg, struct _MessageWindowData *dat, WPARAM wParam, LPARAM lParam);
LRESULT DM_UpdateLastMessage(HWND hwndDlg, struct _MessageWindowData *dat);
LRESULT DM_RecalcPictureSize(HWND hwndDlg, struct _MessageWindowData *dat);
LRESULT DM_WMCopyHandler(HWND hwnd, WNDPROC oldWndProc, WPARAM wParam, LPARAM lParam);
LRESULT DM_MouseWheelHandler(HWND hwnd, HWND hwndParent, struct _MessageWindowData *mwdat, WPARAM wParam, LPARAM lParam);
LRESULT DM_ThemeChanged(HWND hwnd, struct _MessageWindowData *dat);

void BB_InitDlgButtons(HWND hdlg,struct _MessageWindowData *dat);

BOOL BB_SetButtonsPos(HWND hwnd,struct _MessageWindowData *dat);
 void  InitButtonsBarModule();
 void BB_CustomButtonClick(struct _MessageWindowData *dat,DWORD idFrom ,HWND hwndFrom, BOOL code) ;

