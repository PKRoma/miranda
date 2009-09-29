/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda IM: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2009 Miranda ICQ/IM project,
 * all portions of this codebase are copyrighted to the people
 * listed in contributors.txt.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * you should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * part of tabSRMM messaging plugin for Miranda.
 *
 * (C) 2005-2009 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 * prototypes from generic_msghandlers.c
 *
 */

void 	TSAPI		DM_SetDBButtonStates	(HWND hwndChild, struct _MessageWindowData *dat);
int 	TSAPI		BTN_GetStockItem		(ButtonItem *item, const TCHAR *szName);
HWND 	TSAPI		DM_CreateClist			(const _MessageWindowData *dat);

void 	TSAPI		DM_OptionsApplied		(_MessageWindowData *dat, WPARAM wParam, LPARAM lParam);
void 	TSAPI		DM_UpdateTitle			(_MessageWindowData *dat, WPARAM wParam, LPARAM lParam);
LRESULT TSAPI		DM_ScrollToBottom		(_MessageWindowData *dat, WPARAM wParam, LPARAM lParam);
LRESULT TSAPI		DM_LoadLocale			(_MessageWindowData *dat);
LRESULT TSAPI		DM_SaveLocale			(_MessageWindowData *dat, WPARAM wParam, LPARAM lParam);
LRESULT TSAPI		DM_UpdateLastMessage	(const _MessageWindowData *dat);
LRESULT __stdcall 	DM_RecalcPictureSize	(_MessageWindowData *dat);
LRESULT TSAPI		DM_WMCopyHandler		(HWND hwnd, WNDPROC oldWndProc, WPARAM wParam, LPARAM lParam);
LRESULT TSAPI		DM_MouseWheelHandler	(HWND hwnd, HWND hwndParent, struct _MessageWindowData *mwdat, WPARAM wParam, LPARAM lParam);
LRESULT TSAPI		DM_ThemeChanged			(_MessageWindowData *dat);
void	TSAPI		DM_Typing				(_MessageWindowData *dat);
void	TSAPI		DM_FreeTheme			(_MessageWindowData *dat);
void	TSAPI		DM_NotifyTyping			(_MessageWindowData *dat, int mode);
int 	TSAPI 		DM_SplitterGlobalEvent	(_MessageWindowData *dat, WPARAM wParam, LPARAM lParam);
void 	TSAPI		BB_InitDlgButtons		(_MessageWindowData *dat);

BOOL 	TSAPI		BB_SetButtonsPos		(_MessageWindowData *dat);
void 	TSAPI		BB_CustomButtonClick	(_MessageWindowData *dat,DWORD idFrom ,HWND hwndFrom, BOOL code) ;
void	TSAPI		DM_EventAdded			(_MessageWindowData *dat, WPARAM wParam, LPARAM lParam);
