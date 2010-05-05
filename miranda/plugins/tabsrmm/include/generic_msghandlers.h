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

void 	TSAPI		DM_SetDBButtonStates	(HWND hwndChild, struct TWindowData *dat);
int 	TSAPI		BTN_GetStockItem		(ButtonItem *item, const TCHAR *szName);
HWND 	TSAPI		DM_CreateClist			(TWindowData *dat);

void 	TSAPI		DM_OptionsApplied		(TWindowData *dat, WPARAM wParam, LPARAM lParam);
void 	TSAPI		DM_UpdateTitle			(TWindowData *dat, WPARAM wParam, LPARAM lParam);
LRESULT TSAPI		DM_ScrollToBottom		(TWindowData *dat, WPARAM wParam, LPARAM lParam);
LRESULT TSAPI		DM_LoadLocale			(TWindowData *dat);
LRESULT TSAPI		DM_SaveLocale			(TWindowData *dat, WPARAM wParam, LPARAM lParam);
LRESULT TSAPI		DM_UpdateLastMessage	(const TWindowData *dat);
LRESULT __stdcall 	DM_RecalcPictureSize	(TWindowData *dat);
LRESULT TSAPI		DM_WMCopyHandler		(HWND hwnd, WNDPROC oldWndProc, WPARAM wParam, LPARAM lParam);
LRESULT TSAPI		DM_MouseWheelHandler	(HWND hwnd, HWND hwndParent, struct TWindowData *mwdat, WPARAM wParam, LPARAM lParam);
LRESULT TSAPI		DM_ThemeChanged			(TWindowData *dat);
void	TSAPI		DM_Typing				(TWindowData *dat);
void	TSAPI		DM_FreeTheme			(TWindowData *dat);
void	TSAPI		DM_NotifyTyping			(TWindowData *dat, int mode);
int 	TSAPI 		DM_SplitterGlobalEvent	(TWindowData *dat, WPARAM wParam, LPARAM lParam);
void 	TSAPI		BB_InitDlgButtons		(TWindowData *dat);

BOOL 	TSAPI		BB_SetButtonsPos		(TWindowData *dat);
void 	TSAPI		BB_CustomButtonClick	(TWindowData *dat,DWORD idFrom ,HWND hwndFrom, BOOL code) ;
void	TSAPI		DM_EventAdded			(TWindowData *dat, WPARAM wParam, LPARAM lParam);
void 	TSAPI		DM_InitRichEdit			(TWindowData *dat);
LRESULT TSAPI 		DM_ContainerCmdHandler	(TContainerData *pContainer, UINT cmd, WPARAM wParam, LPARAM lParam);
LRESULT TSAPI		DM_MsgWindowCmdHandler	(HWND hwndDlg, TContainerData *pContainer, TWindowData *dat, UINT cmd, WPARAM wParam, LPARAM lParam);
LRESULT TSAPI		DM_GenericHotkeysCheck	(MSG *message, TWindowData *dat);
void 	TSAPI 		DM_DismissTip			(TWindowData *dat, const POINT& pt);
void 	TSAPI 		DM_InitTip				(TWindowData *dat);
void	TSAPI		DM_HandleAutoSizeRequest(TWindowData *dat, REQRESIZE* rr);
