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

//MAD
HWND GetLastChild(HWND hwndParent);
//MAD_
void        CalcDynamicAvatarSize(HWND hwndDlg, struct MessageWindowData *dat, BITMAP *bminfo);
int         IsMetaContact(HWND hwndDlg, struct MessageWindowData *dat);
char        *GetCurrentMetaContactProto(HWND hwndDlg, struct MessageWindowData *dat);
void        WriteStatsOnClose(HWND hwndDlg, struct MessageWindowData *dat);
int         MsgWindowUpdateMenu(HWND hwndDlg, struct MessageWindowData *dat, HMENU submenu, int menuID);
int         MsgWindowMenuHandler(HWND hwndDlg, struct MessageWindowData *dat, int selection, int menuId);
int         GetAvatarVisibility(HWND hwndDlg, struct MessageWindowData *dat);
void        UpdateStatusBar(HWND hwndDlg, struct MessageWindowData *dat);
void        UpdateStatusBarTooltips(HWND hwndDlg, struct MessageWindowData *dat, int iSecIMStatus);
void        SetSelftypingIcon(HWND dlg, struct MessageWindowData *dat, int iMode);
int         CheckValidSmileyPack(char *szProto, HANDLE hContact, HICON *hButtonIcon);
TCHAR       *QuoteText(TCHAR *text,int charsPerLine,int removeExistingQuotes);
void        UpdateReadChars(HWND hwndDlg, struct MessageWindowData *dat);
void        ShowPicture(HWND hwndDlg, struct MessageWindowData *dat, BOOL showNewPic);
void        AdjustBottomAvatarDisplay(HWND hwndDlg, struct MessageWindowData *dat);
void        SetDialogToType(HWND hwndDlg);
void        FlashOnClist(HWND hwndDlg, struct MessageWindowData *dat, HANDLE hEvent, DBEVENTINFO *dbei);
char        *Message_GetFromStream(HWND hwndDlg, struct MessageWindowData* dat, DWORD dwPassedFlags);
BOOL        DoRtfToTags(TCHAR * pszText, struct MessageWindowData *dat);
void        DoTrimMessage(TCHAR *msg);
void        SaveInputHistory(HWND hwndDlg, struct MessageWindowData *dat, WPARAM wParam, LPARAM lParam);
void        GetContactUIN(HWND hwndDlg, struct MessageWindowData *dat);
void        SetMessageLog(HWND hwndDlg, struct MessageWindowData *dat);
void        SwitchMessageLog(HWND hwndDlg, struct MessageWindowData *dat, int iMode);
unsigned int GetIEViewMode(HWND hwndDlg, HANDLE hContact);
void        FindFirstEvent(HWND hwndDlg, struct MessageWindowData *dat);
void        SaveSplitter(HWND hwndDlg, struct MessageWindowData *dat);
void        LoadSplitter(HWND hwndDlg, struct MessageWindowData *dat);
void        PlayIncomingSound(struct ContainerWindowData *pContainer, HWND hwnd);
void        SwitchMessageLog(HWND hwndDlg, struct MessageWindowData *dat, int iMode);
void        GetSendFormat(HWND hwndDlg, struct MessageWindowData *dat, int mode);
void        GetLocaleID(struct MessageWindowData *dat, char *szKLName);
void        GetDataDir();
void        LoadOwnAvatar(HWND hwndDlg, struct MessageWindowData *dat);
void        LoadContactAvatar(HWND hwndDlg, struct MessageWindowData *dat);
BYTE        GetInfoPanelSetting(HWND hwndDlg, struct MessageWindowData *dat);
void        UpdateApparentModeDisplay(HWND hwndDlg, struct MessageWindowData *dat);
void        LoadPanelHeight(HWND hwndDlg, struct MessageWindowData *dat);
void        LoadTimeZone(HWND hwndDlg, struct MessageWindowData *dat);
void        HandlePasteAndSend(HWND hwndDlg, struct MessageWindowData *dat);
int         MsgWindowDrawHandler(WPARAM wParam, LPARAM lParam, HWND hwndDlg, struct MessageWindowData *dat);
void        LoadOverrideTheme(HWND hwndDlg, struct MessageWindowData *dat);
void        LoadThemeDefaults(HWND hwndDlg, struct MessageWindowData *dat);
void        SaveMessageLogFlags(HWND hwndDlg, struct MessageWindowData *dat);
void        ConfigureSmileyButton(HWND hwndDlg, struct MessageWindowData *dat);
int         CutContactName(TCHAR *szold, TCHAR *sznew, unsigned int size);
void        SendNudge(struct MessageWindowData *dat, HWND hwndDlg);
void        EnableSendButton(HWND hwnd, int iMode);
LRESULT     GetSendButtonState(HWND hwnd);
HICON       GetXStatusIcon(struct MessageWindowData *dat);
void        FlashTab(struct MessageWindowData *dat, HWND hwndTab, int iTabindex, BOOL *bState, BOOL mode, HICON origImage);
void        GetClientIcon(struct MessageWindowData *dat, HWND hwndDlg);
void        GetMaxMessageLength(HWND hwndDlg, struct MessageWindowData *dat);
void        RearrangeTab(HWND hwndDlg, struct MessageWindowData *dat, int iMode, BOOL fSavePos);
void        GetCachedStatusMsg(HWND hwndDlg, struct MessageWindowData *dat);
int         MY_pathToRelative(const char *pSrc, char *pOut);
int         MY_pathToAbsolute(const char *pSrc, char *pOut);
void        GetRealIEViewWindow(HWND hwndDlg, struct MessageWindowData *dat);
BOOL        IsStatusEvent(int eventType);
void        GetMyNick(HWND hwndDlg, struct MessageWindowData *dat);
int         FindRTLLocale(struct MessageWindowData *dat);
HICON       MY_GetContactIcon(struct MessageWindowData *dat);

// mathmod

void        MTH_updatePreview(HWND hwndDlg, struct MessageWindowData *dat);
void        MTH_updateMathWindow(HWND hwndDlg, struct MessageWindowData *dat);

extern INT_PTR CALLBACK SelectContainerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK DlgProcContainerOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

struct RTFColorTable {
    TCHAR szName[10];
    COLORREF clr;
    int index;
    int menuid;
};

#endif
