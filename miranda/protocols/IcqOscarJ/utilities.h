// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006,2007 Joe Kucera
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// -----------------------------------------------------------------------------
//
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/utilities.h,v $
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#ifndef __UTILITIES_H
#define __UTILITIES_H

typedef struct icq_ack_args_s
{
  HANDLE hContact;
  int    nAckType;
  int    nAckResult;
  HANDLE hSequence;
  LPARAM pszMessage;
} icq_ack_args;

/*---------* Functions *---------------*/

void EnableDlgItem(HWND hwndDlg, UINT control, int state);
void icq_EnableMultipleControls(HWND hwndDlg, const UINT* controls, int cControls, int state);
void icq_ShowMultipleControls(HWND hwndDlg, const UINT* controls, int cControls, int state);
int IcqStatusToMiranda(WORD wStatus);
WORD MirandaStatusToIcq(int nStatus);
int MirandaStatusToSupported(int nMirandaStatus);
char *MirandaStatusToString(int);
char *MirandaStatusToStringUtf(int);
char**MirandaStatusToAwayMsg(int nStatus);

int AwayMsgTypeToStatus(int nMsgType);

void SetGatewayIndex(HANDLE hConn, DWORD dwIndex);
DWORD GetGatewayIndex(HANDLE hConn);
void FreeGatewayIndex(HANDLE hConn);

void AddToSpammerList(DWORD dwUIN);
BOOL IsOnSpammerList(DWORD dwUIN);

void InitCache();
void UninitCache();
void DeleteFromCache(HANDLE hContact);
HANDLE HContactFromUIN(DWORD dwUin, int *Added);
HANDLE HContactFromUID(DWORD dwUIN, char *pszUID, int *Added);
char *NickFromHandle(HANDLE hContact);
char *NickFromHandleUtf(HANDLE hContact);
char *strUID(DWORD dwUIN, char *pszUID);
void SetContactHidden(HANDLE hContact, BYTE bHidden);

size_t __fastcall strlennull(const char *string);
int __fastcall strcmpnull(const char *str1, const char *str2);
int null_snprintf(char *buffer, size_t count, const char* fmt, ...);
char* __fastcall null_strdup(const char *string);

void parseServerAddress(char *szServer, WORD* wPort);

char *DemangleXml(const char *string, int len);
char *MangleXml(const char *string, int len);
char *EliminateHtml(const char *string, int len);
char* ApplyEncoding(const char *string, const char* pszEncoding);


void ResetSettingsOnListReload(void);
void ResetSettingsOnConnect(void);
void ResetSettingsOnLoad(void);
int RandRange(int nLow, int nHigh);

BOOL IsStringUIN(char* pszString);

void icq_SendProtoAck(HANDLE hContact, DWORD dwCookie, int nAckResult, int nAckType, char* pszMessage);

void SetCurrentStatus(int nStatus);

BOOL writeDbInfoSettingString(HANDLE hContact, const char* szSetting, char** buf, WORD* pwLength);
BOOL writeDbInfoSettingWord(HANDLE hContact, const char *szSetting, char **buf, WORD* pwLength);
BOOL writeDbInfoSettingWordWithTable(HANDLE hContact, const char *szSetting, struct fieldnames_t *table, char **buf, WORD* pwLength);
BOOL writeDbInfoSettingByte(HANDLE hContact, const char *szSetting, char **buf, WORD* pwLength);
BOOL writeDbInfoSettingByteWithTable(HANDLE hContact, const char *szSetting, struct fieldnames_t *table, char **buf, WORD* pwLength);

int GetGMTOffset(void);

BOOL validateStatusMessageRequest(HANDLE hContact, WORD byMessageType);

#define icqOnline ((gnCurrentStatus != ID_STATUS_OFFLINE) && (gnCurrentStatus != ID_STATUS_CONNECTING))

void __fastcall SAFE_FREE(void** p);
void* __fastcall SAFE_MALLOC(size_t size);

void LinkContactPhotoToFile(HANDLE hContact, char* szFile);
void ContactPhotoSettingChanged(HANDLE hContact);

HANDLE NetLib_OpenConnection(HANDLE hUser, const char* szIdent, NETLIBOPENCONNECTION* nloc);
HANDLE NetLib_BindPort(NETLIBNEWCONNECTIONPROC_V2 pFunc, void* lParam, WORD* pwPort, DWORD* pdwIntIP);
void NetLib_SafeCloseHandle(HANDLE *hConnection, int bServerConn);
int NetLog_Server(const char *fmt,...);
int NetLog_Direct(const char *fmt,...);
int NetLog_Uni(BOOL bDC, const char *fmt,...);

int ICQBroadcastAck(HANDLE hContact,int type,int result,HANDLE hProcess,LPARAM lParam);

int __fastcall ICQTranslateDialog(HWND hwndDlg);
char* __fastcall ICQTranslate(const char* src);
char* __fastcall ICQTranslateUtf(const char* src);
char* __fastcall ICQTranslateUtfStatic(const char* src, char* buf);

HANDLE ICQCreateThreadEx(pThreadFuncEx AFunc, void* arg, DWORD* pThreadID);
void ICQCreateThread(pThreadFuncEx AFunc, void* arg);

char* GetUserPassword(BOOL bAlways);
WORD GetMyStatusFlags();

/* Unicode FS utility functions */

int FileStatUtf(const char *path, struct _stati64 *buffer);
int MakeDirUtf(const char *dir);
int OpenFileUtf(const char *filename, int oflag, int pmode);

/* Unicode UI utility functions */
wchar_t* GetWindowTextUcs(HWND hWnd);
void SetWindowTextUcs(HWND hWnd, wchar_t *text);
char* GetWindowTextUtf(HWND hWnd);
char* GetDlgItemTextUtf(HWND hwndDlg, int iItem);
void SetWindowTextUtf(HWND hWnd, const char* szText);
void SetDlgItemTextUtf(HWND hwndDlg, int iItem, const char* szText);
LONG SetWindowLongUtf(HWND hWnd, int nIndex, LONG dwNewLong);
LRESULT CallWindowProcUtf(WNDPROC OldProc, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int ComboBoxAddStringUtf(HWND hCombo, const char* szString, DWORD data);
int ListBoxAddStringUtf(HWND hList, const char* szString);

int MessageBoxUtf(HWND hWnd, const char* szText, const char* szCaption, UINT uType);
HWND DialogBoxUtf(BOOL bModal, HINSTANCE hInstance, const char* szTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
HWND CreateDialogUtf(HINSTANCE hInstance, const char* lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc);

#endif /* __UTILITIES_H */
