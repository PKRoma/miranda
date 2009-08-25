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

$Id$
*/

#ifndef _MSGDLGUTILS_H
#define _MSGDLGUTILS_H

#define WANT_IEVIEW_LOG 1
#define WANT_HPP_LOG 2

//MAD
HWND GetLastChild(HWND hwndParent);
//MAD_
void        CalcDynamicAvatarSize(_MessageWindowData *dat, BITMAP *bminfo);
int         IsMetaContact(const _MessageWindowData *dat);
char        *GetCurrentMetaContactProto(_MessageWindowData *dat);
void        WriteStatsOnClose(_MessageWindowData *dat);
int         MsgWindowUpdateMenu(_MessageWindowData *dat, HMENU submenu, int menuID);
int         MsgWindowMenuHandler(_MessageWindowData *dat, int selection, int menuId);
int         GetAvatarVisibility(HWND hwndDlg, struct _MessageWindowData *dat);
void        UpdateStatusBar(const _MessageWindowData *dat);
void        UpdateStatusBarTooltips(HWND hwndDlg, struct _MessageWindowData *dat, int iSecIMStatus);
void        SetSelftypingIcon(HWND dlg, struct _MessageWindowData *dat, int iMode);
int         CheckValidSmileyPack(char *szProto, HANDLE hContact, HICON *hButtonIcon);
TCHAR       *QuoteText(TCHAR *text,int charsPerLine,int removeExistingQuotes);
void        UpdateReadChars(const _MessageWindowData *dat);
void        ShowPicture(_MessageWindowData *dat, BOOL showNewPic);
void        AdjustBottomAvatarDisplay(_MessageWindowData *dat);
void        SetDialogToType(HWND hwndDlg);
void        FlashOnClist(HWND hwndDlg, struct _MessageWindowData *dat, HANDLE hEvent, DBEVENTINFO *dbei);
char        *Message_GetFromStream(HWND hwndDlg, struct _MessageWindowData* dat, DWORD dwPassedFlags);
BOOL        DoRtfToTags(TCHAR * pszText, struct _MessageWindowData *dat);
void        DoTrimMessage(TCHAR *msg);
void        SaveInputHistory(HWND hwndDlg, struct _MessageWindowData *dat, WPARAM wParam, LPARAM lParam);
void        GetContactUIN(HWND hwndDlg, struct _MessageWindowData *dat);
void        SetMessageLog(HWND hwndDlg, struct _MessageWindowData *dat);
void        SwitchMessageLog(HWND hwndDlg, struct _MessageWindowData *dat, int iMode);
unsigned int GetIEViewMode(HWND hwndDlg, HANDLE hContact);
void        FindFirstEvent(_MessageWindowData *dat);
void        SaveSplitter(_MessageWindowData *dat);
void        LoadSplitter(_MessageWindowData *dat);
void        PlayIncomingSound(struct ContainerWindowData *pContainer, HWND hwnd);
void        SwitchMessageLog(HWND hwndDlg, struct _MessageWindowData *dat, int iMode);
void        GetSendFormat(HWND hwndDlg, struct _MessageWindowData *dat, int mode);
void        GetLocaleID(struct _MessageWindowData *dat, char *szKLName);
void        LoadOwnAvatar(_MessageWindowData *dat);
void        LoadContactAvatar(HWND hwndDlg, struct _MessageWindowData *dat);
BYTE        GetInfoPanelSetting(const _MessageWindowData *dat);
void        LoadPanelHeight(_MessageWindowData *dat);
void        LoadTimeZone(HWND hwndDlg, struct _MessageWindowData *dat);
void        HandlePasteAndSend(HWND hwndDlg, struct _MessageWindowData *dat);
int         MsgWindowDrawHandler(WPARAM wParam, LPARAM lParam, HWND hwndDlg, struct _MessageWindowData *dat);
void        LoadOverrideTheme(HWND hwndDlg, struct _MessageWindowData *dat);
void        LoadThemeDefaults(HWND hwndDlg, struct _MessageWindowData *dat);
void        SaveMessageLogFlags(HWND hwndDlg, struct _MessageWindowData *dat);
void        ConfigureSmileyButton(HWND hwndDlg, struct _MessageWindowData *dat);
int         CutContactName(TCHAR *szold, TCHAR *sznew, unsigned int size);
void        SendNudge(const _MessageWindowData *dat);
void        EnableSendButton(HWND hwnd, int iMode);
LRESULT     GetSendButtonState(HWND hwnd);
HICON       GetXStatusIcon(struct _MessageWindowData *dat);
void        FlashTab(struct _MessageWindowData *dat, HWND hwndTab, int iTabindex, BOOL *bState, BOOL mode, HICON origImage);
void        GetClientIcon(_MessageWindowData *dat);
void        GetMaxMessageLength(_MessageWindowData *dat);
void        RearrangeTab(HWND hwndDlg, struct _MessageWindowData *dat, int iMode, BOOL fSavePos);
void        GetCachedStatusMsg(_MessageWindowData *dat);
size_t      MY_pathToRelative(const char *pSrc, char *pOut);
size_t      MY_pathToAbsolute(const char *pSrc, char *pOut);
void        GetRealIEViewWindow(HWND hwndDlg, struct _MessageWindowData *dat);
BOOL        IsStatusEvent(int eventType);
void        GetMyNick(_MessageWindowData *dat);
int         FindRTLLocale(struct _MessageWindowData *dat);
HICON       MY_GetContactIcon(struct _MessageWindowData *dat);
void 		CheckAndDestroyIEView(struct _MessageWindowData *dat);
// mathmod

void        MTH_updateMathWindow(const _MessageWindowData *dat);

extern INT_PTR CALLBACK SelectContainerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK DlgProcContainerOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

struct RTFColorTable {
    TCHAR szName[10];
    COLORREF clr;
    int index;
    int menuid;
};

#endif
