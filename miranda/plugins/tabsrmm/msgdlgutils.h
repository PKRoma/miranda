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

void CalcDynamicAvatarSize(HWND hwndDlg, struct MessageWindowData *dat, BITMAP *bminfo);
int IsMetaContact(HWND hwndDlg, struct MessageWindowData *dat);
char *GetCurrentMetaContactProto(HWND hwndDlg, struct MessageWindowData *dat);
void WriteStatsOnClose(HWND hwndDlg, struct MessageWindowData *dat);
int MsgWindowUpdateMenu(HWND hwndDlg, struct MessageWindowData *dat, HMENU submenu, int menuID);
int MsgWindowMenuHandler(HWND hwndDlg, struct MessageWindowData *dat, int selection, int menuId);
int GetAvatarVisibility(HWND hwndDlg, struct MessageWindowData *dat);
void HandleIconFeedback(HWND hwndDlg, struct MessageWindowData *dat, int iIcon);
void UpdateStatusBar(HWND hwndDlg, struct MessageWindowData *dat);
void UpdateStatusBarTooltips(HWND hwndDlg, struct MessageWindowData *dat, int iSecIMStatus);
void SetSelftypingIcon(HWND dlg, struct MessageWindowData *dat, int iMode);
int CheckValidSmileyPack(char *szProto, HICON *hButtonIcon);
void CreateSmileyIcon(struct MessageWindowData *dat, HICON hIcon);
TCHAR *QuoteText(TCHAR *text,int charsPerLine,int removeExistingQuotes);
void UpdateReadChars(HWND hwndDlg, struct MessageWindowData *dat);
void ShowPicture(HWND hwndDlg, struct MessageWindowData *dat, BOOL changePic, BOOL showNewPic, BOOL startThread);
DWORD WINAPI LoadPictureThread(LPVOID param);
void SetDialogToType(HWND hwndDlg);
void FlashOnClist(HWND hwndDlg, struct MessageWindowData *dat, HANDLE hEvent, DBEVENTINFO *dbei);
char *Message_GetFromStream(HWND hwndDlg, struct MessageWindowData* dat, DWORD dwPassedFlags);
BOOL DoRtfToTags(TCHAR * pszText, struct MessageWindowData *dat);
void DoTrimMessage(TCHAR *msg);

extern BOOL CALLBACK SelectContainerDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcContainerOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

