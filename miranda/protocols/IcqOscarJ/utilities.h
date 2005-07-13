// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005 Joe Kucera
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
// File name      : $Source$
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

typedef struct icq_cookie_info_s
{
	DWORD dwCookie;
	DWORD dwUin;
	void *pvExtra;
  DWORD dwTime;
} icq_cookie_info;

typedef struct icq_ack_args_s
{
	HANDLE hContact;
	int    nAckType;
	int    nAckResult;
	HANDLE hSequence;
	LPARAM pszMessage;
} icq_ack_args;

/*---------* Functions *---------------*/

void icq_EnableMultipleControls(HWND hwndDlg, const UINT* controls, int cControls, int state);
void icq_ShowMultipleControls(HWND hwndDlg, const UINT* controls, int cControls, int state);
int IcqStatusToMiranda(WORD wStatus);
WORD MirandaStatusToIcq(int nStatus);
int MirandaStatusToSupported(int nMirandaStatus);
char *MirandaStatusToString(int);
char *MirandaVersionToString(int, int);

void InitCookies(void);
void UninitCookies(void);
DWORD AllocateCookie(WORD wIdent, DWORD dwUin, void *pvExtra);
int FindCookie(DWORD wCookie, DWORD *pdwUin, void **ppvExtra);
int FindCookieByData(void *pvExtra,DWORD *pdwCookie, DWORD *pdwUin);
void FreeCookie(DWORD wCookie);
DWORD GenerateCookie(WORD wIdent);

void SetGatewayIndex(HANDLE hConn, DWORD dwIndex);
DWORD GetGatewayIndex(HANDLE hConn);
void FreeGatewayIndex(HANDLE hConn);

HANDLE HContactFromUIN(DWORD, int);
//HANDLE HContactFromUID(char* pszUID, int);
char *NickFromHandle(HANDLE);

size_t __fastcall strlennull(const char *string);
int __fastcall strcmpnull(const char *str1, const char *str2);
int null_snprintf(char *buffer, size_t count, const char* fmt, ...);

char *DemangleXml(const char *string, int len);
char *MangleXml(const char *string, int len);

void ResetSettingsOnListReload(void);
void ResetSettingsOnConnect(void);
void ResetSettingsOnLoad(void);
int RandRange(int nLow, int nHigh);

BOOL IsStringUIN(char* pszString);

void __cdecl icq_ProtocolAckThread(icq_ack_args* pArguments);
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

void LinkContactPhotoToFile(HANDLE hContact, char* szFile);
void ContactPhotoSettingChanged(HANDLE hContact);

int NetLog_Server(const char *fmt,...);
int NetLog_Direct(const char *fmt,...);

int ICQBroadcastAck(HANDLE hContact,int type,int result,HANDLE hProcess,LPARAM lParam);

int __fastcall ICQTranslateDialog(HWND hwndDlg);

char* GetUserPassword(BOOL bAlways);


#endif /* __UTILITIES_H */