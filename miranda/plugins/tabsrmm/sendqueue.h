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

#define TIMERID_MSGSEND      100
#define TIMERID_MULTISEND_BASE (TIMERID_MSGSEND + NR_SENDJOBS)
#define TIMERID_TYPE         3
#define TIMERID_AWAYMSG      4
#define TIMEOUT_TYPEOFF      10000      // send type off after 10 seconds of inactivity
#define SB_CHAR_WIDTH        45

#define SQ_INPROGRESS 1
#define SQ_ERROR 2
#define SQ_UNDEFINED 0

/*
 * send flags
 */

#if defined(_UNICODE)
    #define SEND_FLAGS PREF_UNICODE
#else
    #define SEND_FLAGS 0
#endif

void ClearSendJob(int iIndex);
int FindNextFailedMsg(HWND hwndDlg, struct MessageWindowData *dat);
void HandleQueueError(HWND hwndDlg, struct MessageWindowData *dat, int iEntry);
int AddToSendQueue(HWND hwndDlg, struct MessageWindowData *dat, int iLen, int dwFlags);
static int SendQueuedMessage(HWND hwndDlg, struct MessageWindowData *dat, int iEntry);
void CheckSendQueue(HWND hwndDlg, struct MessageWindowData *dat);
void LogErrorMessage(HWND hwndDlg, struct MessageWindowData *dat, int iSendJobIndex, TCHAR *szErrMsg);
void RecallFailedMessage(HWND hwndDlg, struct MessageWindowData *dat, int iEntry);
void UpdateSaveAndSendButton(HWND hwndDlg, struct MessageWindowData *dat);
void NotifyDeliveryFailure(HWND hwndDlg, struct MessageWindowData *dat);
void ShowErrorControls(HWND hwndDlg, struct MessageWindowData *dat, int showCmd);
void EnableSending(HWND hwndDlg, struct MessageWindowData *dat, int iMode);
void UpdateReadChars(HWND hwndDlg, struct MessageWindowData *dat);
void ShowMultipleControls(HWND hwndDlg, const UINT * controls, int cControls, int state);
void HandleIconFeedback(HWND hwndDlg, struct MessageWindowData *dat, HICON iIcon);
int GetProtoIconFromList(const char *szProto, int iStatus);
int ActivateExistingTab(struct ContainerWindowData *pContainer, HWND hwndChild);
LRESULT WarnPendingJobs(unsigned int uNrMessages);
int AckMessage(HWND hwndDlg, struct MessageWindowData *dat, WPARAM wParam, LPARAM lParam);
static int CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

char *MsgServiceName(HANDLE hContact, struct MessageWindowData *dat, int isUnicode);
int RTL_Detect(WCHAR *pszwText);

