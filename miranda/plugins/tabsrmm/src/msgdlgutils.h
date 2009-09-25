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
 *
 */

#ifndef _MSGDLGUTILS_H
#define _MSGDLGUTILS_H

#define WANT_IEVIEW_LOG 1
#define WANT_HPP_LOG 2

void  	TSAPI CalcDynamicAvatarSize			(_MessageWindowData *dat, BITMAP *bminfo);
int   	TSAPI IsMetaContact					(const _MessageWindowData *dat);
char* 	TSAPI GetCurrentMetaContactProto	(_MessageWindowData *dat);
void	TSAPI WriteStatsOnClose				(_MessageWindowData *dat);
int		TSAPI MsgWindowUpdateMenu			(_MessageWindowData *dat, HMENU submenu, int menuID);
int		TSAPI MsgWindowMenuHandler			(_MessageWindowData *dat, int selection, int menuId);
int  	TSAPI GetAvatarVisibility			(HWND hwndDlg, _MessageWindowData *dat);
void 	TSAPI UpdateStatusBar				(const _MessageWindowData *dat);
int		TSAPI CheckValidSmileyPack			(const char *szProto, HANDLE hContact, HICON *hButtonIcon);
TCHAR*	TSAPI QuoteText						(const TCHAR *text, int charsPerLine, int removeExistingQuotes);
void	TSAPI UpdateReadChars				(const _MessageWindowData *dat);
void	TSAPI ShowPicture					(_MessageWindowData *dat, BOOL showNewPic);
void	TSAPI AdjustBottomAvatarDisplay		(_MessageWindowData *dat);
void	TSAPI SetDialogToType				(HWND hwndDlg);
void	TSAPI FlashOnClist					(HWND hwndDlg, _MessageWindowData *dat, HANDLE hEvent, DBEVENTINFO *dbei);
char*	TSAPI Message_GetFromStream			(HWND hwndDlg, const _MessageWindowData* dat, DWORD dwPassedFlags);
BOOL	TSAPI DoRtfToTags					(TCHAR * pszText, const _MessageWindowData *dat);
void	TSAPI DoTrimMessage					(TCHAR *msg);
void	TSAPI SaveInputHistory				(HWND hwndDlg, struct _MessageWindowData *dat, WPARAM wParam, LPARAM lParam);
void	TSAPI GetContactUIN					(_MessageWindowData *dat);
void	TSAPI SetMessageLog					(_MessageWindowData *dat);
void	TSAPI SwitchMessageLog				(_MessageWindowData *dat, int iMode);
UINT	TSAPI GetIEViewMode					(HWND hwndDlg, HANDLE hContact);
void	TSAPI FindFirstEvent				(_MessageWindowData *dat);
void	TSAPI SaveSplitter					(_MessageWindowData *dat);
void	TSAPI LoadSplitter					(_MessageWindowData *dat);
void	TSAPI PlayIncomingSound				(const struct ContainerWindowData *pContainer, HWND hwnd);
void	TSAPI GetSendFormat					(_MessageWindowData *dat, int mode);
void	TSAPI GetLocaleID					(_MessageWindowData *dat, const TCHAR *szKLName);
void	TSAPI LoadOwnAvatar					(_MessageWindowData *dat);
void	TSAPI LoadContactAvatar				(_MessageWindowData *dat);
void	TSAPI LoadTimeZone					(_MessageWindowData *dat);
void	TSAPI HandlePasteAndSend			(const _MessageWindowData *dat);
int		TSAPI MsgWindowDrawHandler			(WPARAM wParam, LPARAM lParam, HWND hwndDlg, _MessageWindowData *dat);
void	TSAPI LoadOverrideTheme				(_MessageWindowData *dat);
void	TSAPI LoadThemeDefaults				(_MessageWindowData *dat);
void	TSAPI ConfigureSmileyButton			(_MessageWindowData *dat);
int     TSAPI CutContactName				(const TCHAR *szold, TCHAR *sznew, unsigned int size);
void	TSAPI SendNudge						(const _MessageWindowData *dat);
void	TSAPI EnableSendButton				(const _MessageWindowData *dat, int iMode);
LRESULT TSAPI GetSendButtonState			(HWND hwnd);
HICON	TSAPI GetXStatusIcon				(const _MessageWindowData *dat);
void	TSAPI FlashTab						(_MessageWindowData *dat, HWND hwndTab, int iTabindex, BOOL *bState, BOOL mode, HICON origImage);
void	TSAPI GetClientIcon					(_MessageWindowData *dat);
void	TSAPI GetMaxMessageLength			(_MessageWindowData *dat);
void	TSAPI RearrangeTab					(HWND hwndDlg, const _MessageWindowData *dat, int iMode, BOOL fSavePos);
void	TSAPI GetCachedStatusMsg			(_MessageWindowData *dat);
BOOL	TSAPI IsStatusEvent					(int eventType);
void	TSAPI GetMyNick						(_MessageWindowData *dat);
int		TSAPI FindRTLLocale					(_MessageWindowData *dat);
HICON	TSAPI MY_GetContactIcon				(const _MessageWindowData *dat);
void	TSAPI CheckAndDestroyIEView			(_MessageWindowData *dat);

// mathmod

void	TSAPI MTH_updateMathWindow			(const _MessageWindowData *dat);

extern INT_PTR CALLBACK SelectContainerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK DlgProcContainerOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

struct RTFColorTable {
    TCHAR szName[10];
    COLORREF clr;
    int index;
    int menuid;
};

#endif
