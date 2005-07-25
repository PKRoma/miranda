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

#include "icqoscar.h"



static WORD wCookieSeq;
static icq_cookie_info *cookie = NULL;
static int cookieCount = 0;
CRITICAL_SECTION cookieMutex; // we want this in avatar thread, used as queue lock

typedef struct gateway_index_s
{
  HANDLE hConn;
  DWORD  dwIndex;
} gateway_index;

static gateway_index *gateways = NULL;
static int gatewayCount = 0;

static DWORD *spammerList = NULL;
static int spammerListCount = 0;


extern BYTE gbOverRate;
extern DWORD gtLastRequest;


void icq_EnableMultipleControls(HWND hwndDlg, const UINT *controls, int cControls, int state)
{
	int i;

	for (i = 0; i < cControls; i++)
		EnableWindow(GetDlgItem(hwndDlg, controls[i]), state);
}



void icq_ShowMultipleControls(HWND hwndDlg, const UINT *controls, int cControls, int state)
{
	int i;

	for(i = 0; i < cControls; i++)
		ShowWindow(GetDlgItem(hwndDlg, controls[i]), state);
}



// Maps the ICQ status flag (as seen in the status change SNACS) and returns
// a Miranda style status.
int IcqStatusToMiranda(WORD nIcqStatus)
{
	int nMirandaStatus;

	// :NOTE: The order in which the flags are compared are important!
	// I dont like this method but it works.

	if (nIcqStatus & ICQ_STATUSF_INVISIBLE)
		nMirandaStatus = ID_STATUS_INVISIBLE;
	else
		if (nIcqStatus & ICQ_STATUSF_DND)
			nMirandaStatus = ID_STATUS_DND;
	else
		if (nIcqStatus & ICQ_STATUSF_OCCUPIED)
			nMirandaStatus = ID_STATUS_OCCUPIED;
	else
		if (nIcqStatus & ICQ_STATUSF_NA)
			nMirandaStatus = ID_STATUS_NA;
	else
		if (nIcqStatus & ICQ_STATUSF_AWAY)
			nMirandaStatus = ID_STATUS_AWAY;
	else
		if (nIcqStatus & ICQ_STATUSF_FFC)
			nMirandaStatus = ID_STATUS_FREECHAT;
	else
		// Can be discussed, but I think 'online' is the most generic ICQ status
		nMirandaStatus = ID_STATUS_ONLINE;

	return nMirandaStatus;
}



WORD MirandaStatusToIcq(int nMirandaStatus)
{
	WORD nIcqStatus;


	switch (nMirandaStatus)
	{

	case ID_STATUS_ONLINE:
		nIcqStatus = ICQ_STATUS_ONLINE;
		break;

	case ID_STATUS_AWAY:
		nIcqStatus = ICQ_STATUS_AWAY;
		break;

	case ID_STATUS_OUTTOLUNCH:
	case ID_STATUS_NA:
		nIcqStatus = ICQ_STATUS_NA;
		break;

	case ID_STATUS_ONTHEPHONE:
	case ID_STATUS_OCCUPIED:
		nIcqStatus = ICQ_STATUS_OCCUPIED;
		break;

	case ID_STATUS_DND:
		nIcqStatus = ICQ_STATUS_DND;
		break;

	case ID_STATUS_INVISIBLE:
		nIcqStatus = ICQ_STATUS_INVISIBLE;
		break;

	case ID_STATUS_FREECHAT:
		nIcqStatus = ICQ_STATUS_FFC;
		break;

	case ID_STATUS_OFFLINE:
		// Oscar doesnt have anything that maps to this status. This should never happen.
		_ASSERTE(nMirandaStatus != ID_STATUS_OFFLINE);
		nIcqStatus = 0;
		break;

	default:
		// Online seems to be a good default.
		// Since it cant be offline, it must be a new type of online status.
		nIcqStatus = ICQ_STATUS_ONLINE;
		break;
	}

	return nIcqStatus;
}


int MirandaStatusToSupported(int nMirandaStatus)
{
	int nSupportedStatus;

	switch (nMirandaStatus)
	{

		// These status mode does not need any mapping
	case ID_STATUS_ONLINE:
	case ID_STATUS_AWAY:
	case ID_STATUS_NA:
	case ID_STATUS_OCCUPIED:
	case ID_STATUS_DND:
	case ID_STATUS_INVISIBLE:
	case ID_STATUS_FREECHAT:
	case ID_STATUS_OFFLINE:
		nSupportedStatus = nMirandaStatus;
		break;

		// This mode is not support and must be mapped to something else
	case ID_STATUS_OUTTOLUNCH:
		nSupportedStatus = ID_STATUS_NA;
		break;

		// This mode is not support and must be mapped to something else
	case ID_STATUS_ONTHEPHONE:
		nSupportedStatus = ID_STATUS_OCCUPIED;
		break;

		// This is not supposed to happen.
	default:
		_ASSERTE(0);
		// Online seems to be a good default.
		nSupportedStatus = ID_STATUS_ONLINE;
		break;
	}

	return nSupportedStatus;
}



char* MirandaStatusToString(int mirandaStatus)
{
	return (char *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, mirandaStatus, 0);
}


static void verToStr(char* szStr, int v)
{
  if (v&0xFF)
    sprintf(szStr, "%s%u.%u.%u.%u%s", szStr, (v>>24)&0x7F, (v>>16)&0xFF, (v>>8)&0xFF, v&0xFF, v&0x80000000?" alpha":"");
  else if ((v>>8)&0xFF)
    sprintf(szStr, "%s%u.%u.%u%s", szStr, (v>>24)&0x7F, (v>>16)&0xFF, (v>>8)&0xFF, v&0x80000000?" alpha":"");
  else
    sprintf(szStr, "%s%u.%u%s", szStr, (v>>24)&0x7F, (v>>16)&0xFF, v&0x80000000?" alpha":"");
}


// Dont free the returned string.
// Function is not multithread safe.
char* MirandaVersionToString(int v, int m)
{
	static char szVersion[64];

	if (!v) // this is not Miranda
		return NULL;
	else
	{
    strcpy(szVersion, "Miranda IM ");

		if (v == 1)
			verToStr(szVersion, 0x80010200);
		else if ((v&0x7FFFFFFF) <= 0x030301)
      verToStr(szVersion, v);
    else 
    {
      if (m)
      {
        verToStr(szVersion, m);
        strcat(szVersion, " ");
      }
      strcat(szVersion, "(ICQ v");
      verToStr(szVersion, v);
      strcat(szVersion, ")");
    }
	}

	return szVersion;
}



void InitCookies(void)
{
  InitializeCriticalSection(&cookieMutex);

  cookieCount = 0;
  cookie = NULL;
  wCookieSeq = 2;
}


void UninitCookies(void)
{
  SAFE_FREE(&cookie);

  DeleteCriticalSection(&cookieMutex);
}


// Generate and allocate cookie
DWORD AllocateCookie(WORD wIdent, DWORD dwUin, void *pvExtra)
{
  DWORD dwThisSeq;

  EnterCriticalSection(&cookieMutex);

  dwThisSeq = wCookieSeq++;
  dwThisSeq &= 0x7FFF;
  dwThisSeq |= wIdent<<0x10;

  cookie = (icq_cookie_info *)realloc(cookie, sizeof(icq_cookie_info) * (cookieCount + 1));
  cookie[cookieCount].dwCookie = dwThisSeq;
  cookie[cookieCount].dwUin = dwUin;
  cookie[cookieCount].pvExtra = pvExtra;
  cookie[cookieCount].dwTime = time(NULL);
  cookieCount++;

  LeaveCriticalSection(&cookieMutex);

  return dwThisSeq;
}


DWORD GenerateCookie(WORD wIdent)
{
  DWORD dwThisSeq;

  EnterCriticalSection(&cookieMutex);
  dwThisSeq = wCookieSeq++;
  dwThisSeq &= 0x7FFF;
  dwThisSeq |= wIdent<<0x10;
  LeaveCriticalSection(&cookieMutex);

  return dwThisSeq;
}


int FindCookie(DWORD dwCookie, DWORD *pdwUin, void **ppvExtra)
{
	int i;
	int nFound = 0;


	EnterCriticalSection(&cookieMutex);

	for (i = 0; i < cookieCount; i++)
	{
		if (dwCookie == cookie[i].dwCookie)
		{
			if (pdwUin)
				*pdwUin = cookie[i].dwUin;
			if (ppvExtra)
				*ppvExtra = cookie[i].pvExtra;

			// Cookie found, exit loop
			nFound = 1;
			break;

		}
	}

	LeaveCriticalSection(&cookieMutex);

	return nFound;
}


int FindCookieByData(void *pvExtra,DWORD *pdwCookie, DWORD *pdwUin)
{
	int i;
	int nFound = 0;


	EnterCriticalSection(&cookieMutex);

	for (i = 0; i < cookieCount; i++)
	{
		if (pvExtra == cookie[i].pvExtra)
		{
			if (pdwUin)
				*pdwUin = cookie[i].dwUin;
			if (pdwCookie)
				*pdwCookie = cookie[i].dwCookie;

			// Cookie found, exit loop
			nFound = 1;
			break;

		}
	}

	LeaveCriticalSection(&cookieMutex);

	return nFound;
}


void FreeCookie(DWORD dwCookie)
{
	int i;


	EnterCriticalSection(&cookieMutex);

	for (i = 0; i < cookieCount; i++)
	{
		if (dwCookie == cookie[i].dwCookie)
		{
			cookieCount--;
			memmove(&cookie[i], &cookie[i+1], sizeof(icq_cookie_info) * (cookieCount - i));
			cookie = (icq_cookie_info*)realloc(cookie, sizeof(icq_cookie_info) * cookieCount);

			// Cookie found, exit loop
			break;
		}
    if (cookie[i].dwTime + COOKIE_TIMEOUT < (DWORD)time(NULL))
    { // cookie expired, remove too
			cookieCount--;
			memmove(&cookie[i], &cookie[i+1], sizeof(icq_cookie_info) * (cookieCount - i));
			cookie = (icq_cookie_info*)realloc(cookie, sizeof(icq_cookie_info) * cookieCount);
      i--; // fix the loop
    }
	}

	LeaveCriticalSection(&cookieMutex);
}


void SetGatewayIndex(HANDLE hConn, DWORD dwIndex)
{
  int i;

  EnterCriticalSection(&cookieMutex);

	for (i = 0; i < gatewayCount; i++)
	{
		if (hConn == gateways[i].hConn)
		{
      gateways[i].dwIndex = dwIndex;
      
    	LeaveCriticalSection(&cookieMutex);

      return;
		}
	}

  gateways = (gateway_index *)realloc(gateways, sizeof(gateway_index) * (gatewayCount + 1));
  gateways[gatewayCount].hConn = hConn;
  gateways[gatewayCount].dwIndex = dwIndex;
  gatewayCount++;

  LeaveCriticalSection(&cookieMutex);

  return;
}


DWORD GetGatewayIndex(HANDLE hConn)
{
	int i;

	EnterCriticalSection(&cookieMutex);

	for (i = 0; i < gatewayCount; i++)
	{
		if (hConn == gateways[i].hConn)
		{
    	LeaveCriticalSection(&cookieMutex);

      return gateways[i].dwIndex;
		}
	}

	LeaveCriticalSection(&cookieMutex);

	return 1; // this is default
}


void FreeGatewayIndex(HANDLE hConn)
{
	int i;


	EnterCriticalSection(&cookieMutex);

	for (i = 0; i < gatewayCount; i++)
	{
		if (hConn == gateways[i].hConn)
		{
			gatewayCount--;
			memmove(&gateways[i], &gateways[i+1], sizeof(gateway_index) * (gatewayCount - i));
			gateways = (gateway_index*)realloc(gateways, sizeof(gateway_index) * gatewayCount);

			// Gateway found, exit loop
			break;
		}
	}

	LeaveCriticalSection(&cookieMutex);
}


void AddToSpammerList(DWORD dwUIN)
{
  EnterCriticalSection(&cookieMutex);

  spammerList = (DWORD *)realloc(spammerList, sizeof(DWORD) * (spammerListCount + 1));
  spammerList[spammerListCount] = dwUIN;
  spammerListCount++;

  LeaveCriticalSection(&cookieMutex);
}


BOOL IsOnSpammerList(DWORD dwUIN)
{
	int i;

	EnterCriticalSection(&cookieMutex);

	for (i = 0; i < spammerListCount; i++)
	{
		if (dwUIN == spammerList[i])
		{
      LeaveCriticalSection(&cookieMutex);

      return TRUE;
		}
	}

	LeaveCriticalSection(&cookieMutex);

  return FALSE;
}


HANDLE HContactFromUIN(DWORD uin, int allowAdd)
{
	HANDLE hContact;
	char* szProto;

	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL)
	{
		szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);

		if (szProto != NULL && !strcmp(szProto, gpszICQProtoName))
			if (ICQGetContactSettingDword(hContact, UNIQUEIDSETTING, 0) == uin)
				return hContact;

		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}

  // not in list, check that uin do not belong to us
	if (ICQGetContactSettingDword(NULL, UNIQUEIDSETTING, 0) == uin)
		return NULL;

	//not present: add
	if (allowAdd)
	{
		hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
		CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)gpszICQProtoName);

		ICQWriteContactSettingDword(hContact, UNIQUEIDSETTING, uin);
		DBWriteContactSettingByte(hContact, "CList", "NotOnList", 1);
		DBWriteContactSettingByte(hContact, "CList", "Hidden", 1);

		icq_QueueUser(hContact);

		if (icqOnline)
		{
			icq_sendNewContact(uin);
		}

		return hContact;
	}

	return INVALID_HANDLE_VALUE;
}


/*
HANDLE HContactFromUID(char* pszUID, int allowAdd)
{

	HANDLE hContact;
	char* szProto;
	DBVARIANT dbv;


	if (DBGetContactSetting(NULL, gpszICQProtoName, UNIQUEIDSETTING, &dbv))
	{
		if (strcmp(dbv.pszVal, pszUID))
		{
			DBFreeVariant(&dbv);
			return NULL;
		}
	}


	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL)
	{

		szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);

		if (szProto != NULL && !strcmp(szProto, gpszICQProtoName))
			if (DBGetContactSetting(hContact, gpszICQProtoName, UNIQUEIDSETTING, &dbv))
			{
				if (strcmp(dbv.pszVal, pszUID))
				{
					DBFreeVariant(&dbv);
					return hContact;
				}
			}
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}


	//not present: add
	if (allowAdd)
	{
		hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
		CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)gpszICQProtoName);

		DBWriteContactSettingString(hContact, gpszICQProtoName, UNIQUEIDSETTING, pszUID);
		DBWriteContactSettingByte(hContact, "CList", "NotOnList", 1);
		DBWriteContactSettingByte(hContact, "CList", "Hidden", 1);

		icq_QueueUser(hContact);

		if (icqOnline)
		{
//			if (!(gbSsiEnabled && DBGetContactSettingByte(NULL, gpszICQProtoName, "ServerAddRemove", DEFAULT_SS_ADDREMOVE)))
//				icq_sendNewContact(pszUID);
//			// else need to wait for CList/NotOnList to be deleted
		}

		return hContact;

	}

	return INVALID_HANDLE_VALUE;

}
*/


char *NickFromHandle(HANDLE hContact)
{
	if (hContact == INVALID_HANDLE_VALUE)
		return _strdup("<invalid>");

	return _strdup((char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0));
}



/* a strlen() that likes NULL */
size_t __fastcall strlennull(const char *string)
{
	if (string)
		return strlen(string);

	return 0;
}


/* a strcmp() that likes NULL */
int __fastcall strcmpnull(const char *str1, const char *str2)
{
  if (str1 && str2) 
    return strcmp(str1, str2);

  return 1;
}


int null_snprintf(char *buffer, size_t count, const char* fmt, ...)
{
	va_list va;
	int len;

	va_start(va, fmt);
	len = _vsnprintf(buffer, count-1, fmt, va);
	va_end(va);
	buffer[count-1] = 0;
	return len;
}


char *DemangleXml(const char *string, int len)
{ 
  char *szWork = (char*)malloc(len+1), *szChar = szWork;
  int i;

  for (i=0; i<len; i++)
  {
    if (!strnicmp(string+i, "&gt;", 4))
    {
      *szChar = '>';
      szChar++;
      i += 3;
    }
    else if (!strnicmp(string+i, "&lt;", 4))
    {
      *szChar = '<';
      szChar++;
      i += 3;
    }
    else
    {
      *szChar = string[i];
      szChar++;
    }
  }
  *szChar = '\0';

  return szWork;
}


char *MangleXml(const char *string, int len)
{
  int i, l = 1;
  char *szWork, *szChar;

  for (i = 0; i<len; i++)
  {
    if (string[i]=='<' || string[i]=='>') l += 4; else l++;
  }
  szChar = szWork = (char*)malloc(l + 1);
  for (i = 0; i<len; i++)
  {
    if (string[i]=='<') 
    {
      *(DWORD*)szChar = ';tl&';
      szChar += 4;
    }
    else if (string[i]=='>')
    {
      *(DWORD*)szChar = ';tg&';
      szChar += 4;
    }
    else
    {
      *szChar = string[i];
      szChar++;
    }
  }
  *szChar = '\0';

  return szWork;
}


void ResetSettingsOnListReload()
{
  HANDLE hContact;
  char *szProto;

  // Reset a bunch of session specific settings
  ICQWriteContactSettingWord(NULL, "SrvVisibilityID", 0);
  ICQWriteContactSettingWord(NULL, "SrvAvatarID", 0);
  ICQWriteContactSettingWord(NULL, "SrvRecordCount", 0);

  hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

  while (hContact)
  {
    szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);

    if (szProto != NULL && !strcmp(szProto, gpszICQProtoName))
    {
      // All these values will be restored during the serv-list receive
      ICQWriteContactSettingWord(hContact, "ServerId", 0);
      ICQWriteContactSettingWord(hContact, "SrvGroupId", 0);
      ICQWriteContactSettingWord(hContact, "SrvPermitId", 0);
      ICQWriteContactSettingWord(hContact, "SrvDenyId", 0);
      ICQWriteContactSettingByte(hContact, "Auth", 0);
    }

    hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
  }
}



void ResetSettingsOnConnect()
{
	HANDLE hContact;
	char *szProto;

  gbOverRate = 0; // init
  gtLastRequest = 0;

	// Reset a bunch of session specific settings
  ICQWriteContactSettingByte(NULL, "SrvVisibility", 0);
  ICQWriteContactSettingDword(NULL, "IdleTS", 0);

	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

	while (hContact)
	{
		szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);

		if (szProto != NULL && !strcmp(szProto, gpszICQProtoName))
		{
			ICQWriteContactSettingDword(hContact, "LogonTS", 0);
			ICQWriteContactSettingDword(hContact, "IdleTS", 0);
			ICQWriteContactSettingDword(hContact, "TickTS", 0);
      ICQDeleteContactSetting(hContact, "TemporaryVisible");

			// All these values will be restored during the login
			if (ICQGetContactSettingWord(hContact, "Status", ID_STATUS_OFFLINE) != ID_STATUS_OFFLINE)
				ICQWriteContactSettingWord(hContact, "Status", ID_STATUS_OFFLINE);
		}

		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
}



void ResetSettingsOnLoad()
{
	HANDLE hContact;
	char *szProto;

  ICQWriteContactSettingDword(NULL, "IdleTS", 0);
  ICQWriteContactSettingDword(NULL, "LogonTS", 0);

	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

	while (hContact)
	{
		szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (szProto != NULL && !strcmp(szProto, gpszICQProtoName))
		{
			ICQWriteContactSettingDword(hContact, "LogonTS", 0);
			ICQWriteContactSettingDword(hContact, "IdleTS", 0);
			ICQWriteContactSettingDword(hContact, "TickTS", 0);
			if (ICQGetContactSettingWord(hContact, "Status", ID_STATUS_OFFLINE) != ID_STATUS_OFFLINE)
      {
				ICQWriteContactSettingWord(hContact, "Status", ID_STATUS_OFFLINE);

        ICQDeleteContactSetting(hContact, "XStatusId");
        ICQDeleteContactSetting(hContact, "XStatusName");
        ICQDeleteContactSetting(hContact, "XStatusMsg");
      }
		}
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
}



int RandRange(int nLow, int nHigh)
{
	return nLow + (int)((nHigh-nLow+1)*rand()/(RAND_MAX+1.0));
}



BOOL IsStringUIN(char* pszString)
{
	int i;
	int nLen = strlen(pszString);


	if (nLen > 0 && pszString[0] != '0')
	{
		for (i=0; i<nLen; i++)
		{
			if ((pszString[i] < '0') || (pszString[i] > '9'))
				return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}



void __cdecl icq_ProtocolAckThread(icq_ack_args* pArguments)
{
	DWORD dwUin;
	void* pvExtra;


	ICQBroadcastAck(pArguments->hContact, pArguments->nAckType, pArguments->nAckResult, pArguments->hSequence, pArguments->pszMessage);

	if (FindCookie((WORD)pArguments->hSequence, &dwUin, &pvExtra))
		FreeCookie((WORD)pArguments->hSequence);

	if (pArguments->nAckResult == ACKRESULT_SUCCESS)
		NetLog_Server("Sent fake message ack");
	else if (pArguments->nAckResult == ACKRESULT_FAILED)
		NetLog_Server("Message delivery failed");

	SAFE_FREE(&(char *)pArguments->pszMessage);
	SAFE_FREE(&pArguments);

	return;
}



void icq_SendProtoAck(HANDLE hContact, DWORD dwCookie, int nAckResult, int nAckType, char* pszMessage)
{
	icq_ack_args* pArgs;


	pArgs = malloc(sizeof(icq_ack_args)); // This will be freed in the new thread

	pArgs->hContact = hContact;
	pArgs->hSequence = (HANDLE)dwCookie;
	pArgs->nAckResult = nAckResult;
	pArgs->nAckType = nAckType;
	if (pszMessage)
		pArgs->pszMessage = (LPARAM)strdup(pszMessage);
	else
		pArgs->pszMessage = (LPARAM)NULL;

	forkthread(icq_ProtocolAckThread, 0, pArgs);
}


void SetCurrentStatus(int nStatus)
{
  int nOldStatus = gnCurrentStatus;

  gnCurrentStatus = nStatus;
  ICQBroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)nOldStatus, nStatus);
}


BOOL writeDbInfoSettingString(HANDLE hContact, const char* szSetting, char** buf, WORD* pwLength)
{
	WORD wLen;


	if (*pwLength < 2)
		return FALSE;

	unpackLEWord(buf, &wLen);
	*pwLength -= 2;

	if (*pwLength < wLen)
		return FALSE;

	if ((wLen > 0) && (**buf) && ((*buf)[wLen-1]==0)) // Make sure we have a proper string
		ICQWriteContactSettingString(hContact, szSetting, *buf);
	else
		ICQDeleteContactSetting(hContact, szSetting);

	*buf += wLen;
	*pwLength -= wLen;

	return TRUE;
}



BOOL writeDbInfoSettingWord(HANDLE hContact, const char *szSetting, char **buf, WORD* pwLength)
{
	WORD wVal;


	if (*pwLength < 2)
		return FALSE;

	unpackLEWord(buf, &wVal);
	*pwLength -= 2;

	if (wVal != 0)
		ICQWriteContactSettingWord(hContact, szSetting, wVal);
	else
		ICQDeleteContactSetting(hContact, szSetting);

	return TRUE;
}



BOOL writeDbInfoSettingWordWithTable(HANDLE hContact, const char *szSetting, struct fieldnames_t *table, char **buf, WORD* pwLength)
{
	WORD wVal;
	char *text;


	if (*pwLength < 2)
		return FALSE;

	unpackLEWord(buf, &wVal);
	*pwLength -= 2;

	text = LookupFieldName(table, wVal);
	if (text)
		ICQWriteContactSettingString(hContact, szSetting, text);
	else
		ICQDeleteContactSetting(hContact, szSetting);

	return TRUE;
}



BOOL writeDbInfoSettingByte(HANDLE hContact, const char *pszSetting, char **buf, WORD* pwLength)
{
	BYTE byVal;


	if (*pwLength < 1)
		return FALSE;

	unpackByte(buf, &byVal);
	*pwLength -= 1;

	if (byVal != 0)
		ICQWriteContactSettingByte(hContact, pszSetting, byVal);
	else
		ICQDeleteContactSetting(hContact, pszSetting);

	return TRUE;
}



BOOL writeDbInfoSettingByteWithTable(HANDLE hContact, const char *szSetting, struct fieldnames_t *table, char **buf, WORD* pwLength)
{
	BYTE byVal;
	char *text;


	if (*pwLength < 1)
		return FALSE;

	unpackByte(buf, &byVal);
	*pwLength -= 1;

	text = LookupFieldName(table, byVal);
	if (text)
		ICQWriteContactSettingString(hContact, szSetting, text);
	else
		ICQDeleteContactSetting(hContact, szSetting);

	return TRUE;
}



// Returns the current GMT offset in seconds
int GetGMTOffset(void)
{
	TIME_ZONE_INFORMATION tzinfo;
	DWORD dwResult;
	int nOffset = 0;


	dwResult = GetTimeZoneInformation(&tzinfo);

	switch(dwResult)
	{

	case TIME_ZONE_ID_STANDARD:
		nOffset = -(tzinfo.Bias + tzinfo.StandardBias) * 60;
		break;

	case TIME_ZONE_ID_DAYLIGHT:
		nOffset = -(tzinfo.Bias + tzinfo.DaylightBias) * 60;
		break;

	case TIME_ZONE_ID_UNKNOWN:
	case TIME_ZONE_ID_INVALID:
	default:
		nOffset = 0;
		break;

	}

	return nOffset;
}



BOOL validateStatusMessageRequest(HANDLE hContact, WORD byMessageType)
{
	// Privacy control
	if (ICQGetContactSettingByte(NULL, "StatusMsgReplyCList", 0))
	{
		// Don't send statusmessage to unknown contacts
		if (hContact == INVALID_HANDLE_VALUE)
			return FALSE;

		// Don't send statusmessage to temporary contacts or hidden contacts
		if (DBGetContactSettingByte(hContact, "CList", "NotOnList", 0) ||
			DBGetContactSettingByte(hContact, "CList", "Hidden", 0))
			return FALSE;

		// Don't send statusmessage to invisible contacts
		if (ICQGetContactSettingByte(NULL, "StatusMsgReplyVisible", 0))
		{
			WORD wStatus = ICQGetContactSettingWord(hContact, "Status", ID_STATUS_OFFLINE);
			if (wStatus == ID_STATUS_OFFLINE)
				return FALSE;
		}
	}

	// Dont send messages to people you are hiding from
	if (hContact != INVALID_HANDLE_VALUE &&
		ICQGetContactSettingWord(hContact, "ApparentMode", 0) == ID_STATUS_OFFLINE)
	{
    return FALSE;
	}

	// Dont respond to request for other statuses than your current one
	if ((byMessageType == MTYPE_AUTOAWAY && gnCurrentStatus != ID_STATUS_AWAY) ||
		(byMessageType == MTYPE_AUTOBUSY && gnCurrentStatus != ID_STATUS_OCCUPIED) ||
		(byMessageType == MTYPE_AUTONA   && gnCurrentStatus != ID_STATUS_NA) ||
		(byMessageType == MTYPE_AUTODND  && gnCurrentStatus != ID_STATUS_DND) ||
		(byMessageType == MTYPE_AUTOFFC  && gnCurrentStatus != ID_STATUS_FREECHAT))
	{
		return FALSE;
	}

  if (hContact != INVALID_HANDLE_VALUE && gnCurrentStatus==ID_STATUS_INVISIBLE &&
    ICQGetContactSettingWord(hContact, "ApparentMode", 0) != ID_STATUS_ONLINE)
  {
		if (!ICQGetContactSettingByte(hContact, "TemporaryVisible", 0))
    { // Allow request to temporary visible contacts
      return FALSE;
    }
  }

	// All OK!
	return TRUE;
}


void __fastcall SAFE_FREE(void** p)
{
	if (*p)
	{
		free(*p);
		*p = NULL;
	}
}


static int bPhotoLock = 0;

void LinkContactPhotoToFile(HANDLE hContact, char* szFile)
{ // set contact photo if linked if no photo set link
  if (ICQGetContactSettingByte(NULL, "AvatarsAutoLink", DEFAULT_LINK_AVATARS))
  {
    bPhotoLock = 1;
    {
      if (DBGetContactSettingByte(hContact, "ContactPhoto", "ICQLink", 0))
      { // we are linked update DB
        if (szFile)
        {
          DBDeleteContactSetting(hContact, "ContactPhoto", "File"); // delete that setting
          DBDeleteContactSetting(hContact, "ContactPhoto", "Link");
          if (DBWriteContactSettingString(hContact, "ContactPhoto", "File", szFile))
            NetLog_Server("Avatar file could not be linked to ContactPhoto.");
        }
        else
        { // no file, unlink
          DBDeleteContactSetting(hContact, "ContactPhoto", "File");
          DBDeleteContactSetting(hContact, "ContactPhoto", "ICQLink");
        }
      }
      else if (szFile) 
      { // link only if file valid
        DBVARIANT dbv;
        if (DBGetContactSetting(hContact, "ContactPhoto", "File", &dbv))
        {
          if (DBGetContactSetting(hContact, "ContactPhoto", "Link", &dbv))
          { // no photo defined
            DBWriteContactSettingString(hContact, "ContactPhoto", "File", szFile);
            DBWriteContactSettingByte(hContact, "ContactPhoto", "ICQLink", 1);
          }
          DBFreeVariant(&dbv);
        }
        else
        { // some file already defined, check if it is not the same, if yes, set link
          if (!strcmp(dbv.pszVal, szFile))
          {
            DBWriteContactSettingByte(hContact, "ContactPhoto", "ICQLink", 1);
          }
          DBFreeVariant(&dbv);
        }
      }
    }
    bPhotoLock = 0;
  }
}

static int bNoChanging = 0;

void ContactPhotoSettingChanged(HANDLE hContact)
{ // the setting was changed - if it is done externaly unlink...
  if (bNoChanging) return;
  bNoChanging = 1;

  if (!bPhotoLock && ICQGetContactSettingByte(NULL, "AvatarsAutoLink", 0))
    DBDeleteContactSetting(hContact, "ContactPhoto", "ICQLink");

  bNoChanging = 0;
}


int NetLog_Server(const char *fmt,...)
{
	va_list va;
	char szText[1024];

	va_start(va,fmt);
	mir_vsnprintf(szText,sizeof(szText),fmt,va);
	va_end(va);
	return CallService(MS_NETLIB_LOG,(WPARAM)ghServerNetlibUser,(LPARAM)szText);
}


int NetLog_Direct(const char *fmt,...)
{
	va_list va;
	char szText[1024];

	va_start(va,fmt);
	mir_vsnprintf(szText,sizeof(szText),fmt,va);
	va_end(va);
	return CallService(MS_NETLIB_LOG,(WPARAM)ghDirectNetlibUser,(LPARAM)szText);
}


int ICQBroadcastAck(HANDLE hContact,int type,int result,HANDLE hProcess,LPARAM lParam)
{
	ACKDATA ack={0};

	ack.cbSize=sizeof(ACKDATA);
	ack.szModule=gpszICQProtoName;
  ack.hContact=hContact;
	ack.type=type; 
  ack.result=result;
	ack.hProcess=hProcess; 
  ack.lParam=lParam;
	return CallService(MS_PROTO_BROADCASTACK,0,(LPARAM)&ack);
}


int __fastcall ICQTranslateDialog(HWND hwndDlg)
{
	LANGPACKTRANSLATEDIALOG lptd;

	lptd.cbSize=sizeof(lptd);
	lptd.flags=0;
	lptd.hwndDlg=hwndDlg;
	lptd.ignoreControls=NULL;
	return CallService(MS_LANGPACK_TRANSLATEDIALOG,0,(LPARAM)&lptd);
}


char* GetUserPassword(BOOL bAlways)
{
  if (gpszPassword[0] != '\0' && (gbRememberPwd || bAlways))
    return gpszPassword;

  if (!ICQGetContactStaticString(NULL, "Password", gpszPassword, sizeof(gpszPassword)))
  {
    CallService(MS_DB_CRYPT_DECODESTRING, strlen(gpszPassword) + 1, (LPARAM)gpszPassword);

    if (!strlen(gpszPassword)) return NULL;

    gbRememberPwd = TRUE;

    return gpszPassword;
  }

  return NULL;
}