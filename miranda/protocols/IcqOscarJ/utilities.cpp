// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001-2002 Jon Keating, Richard Hughes
// Copyright © 2002-2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004-2008 Joe Kucera
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
// File name      : $URL$
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

struct gateway_index
{
	HANDLE hConn;
	DWORD  dwIndex;
};

static gatewayMutexRef = 0;
static CRITICAL_SECTION gatewayMutex;

static gateway_index *gateways = NULL;
static int gatewayCount = 0;

static DWORD *spammerList = NULL;
static int spammerListCount = 0;


void EnableDlgItem(HWND hwndDlg, UINT control, int state)
{
	EnableWindow(GetDlgItem(hwndDlg, control), state);
}

void icq_EnableMultipleControls(HWND hwndDlg, const UINT *controls, int cControls, int state)
{
	int i;

	for (i = 0; i < cControls; i++)
		EnableDlgItem(hwndDlg, controls[i], state);
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

	switch (nMirandaStatus) {
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

	switch (nMirandaStatus) {

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

char *MirandaStatusToString(int mirandaStatus)
{
	return (char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, mirandaStatus, 0);
}

char *MirandaStatusToStringUtf(int mirandaStatus)
{ // return miranda status description in utf-8, use unicode service is possible
	return mtchar_to_utf8((TCHAR*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, mirandaStatus, gbUnicodeCore ? GCMDF_UNICODE : 0));
}

char** CIcqProto::MirandaStatusToAwayMsg(int nStatus)
{
	switch (nStatus) {

	case ID_STATUS_AWAY:
		return &m_modeMsgs.szAway;

	case ID_STATUS_NA:
		return &m_modeMsgs.szNa;

	case ID_STATUS_OCCUPIED:
		return &m_modeMsgs.szOccupied;

	case ID_STATUS_DND:
		return &m_modeMsgs.szDnd;

	case ID_STATUS_FREECHAT:
		return &m_modeMsgs.szFfc;

	default:
		return NULL;
	}
}

int AwayMsgTypeToStatus(int nMsgType)
{
	switch (nMsgType) {
	case MTYPE_AUTOAWAY:
		return ID_STATUS_AWAY;

	case MTYPE_AUTOBUSY:
		return ID_STATUS_OCCUPIED;

	case MTYPE_AUTONA:
		return ID_STATUS_NA;

	case MTYPE_AUTODND:
		return ID_STATUS_DND;

	case MTYPE_AUTOFFC:
		return ID_STATUS_FREECHAT;

	default:
		return ID_STATUS_OFFLINE;
	}
}

void SetGatewayIndex(HANDLE hConn, DWORD dwIndex)
{
	EnterCriticalSection(&gatewayMutex);

	for (int i = 0; i < gatewayCount; i++)
	{
		if (hConn == gateways[i].hConn)
		{
			gateways[i].dwIndex = dwIndex;
			LeaveCriticalSection(&gatewayMutex);
			return;
		}
	}

	gateways = (gateway_index *)SAFE_REALLOC(gateways, sizeof(gateway_index) * (gatewayCount + 1));
	gateways[gatewayCount].hConn = hConn;
	gateways[gatewayCount].dwIndex = dwIndex;
	gatewayCount++;

	LeaveCriticalSection(&gatewayMutex);
	return;
}

DWORD GetGatewayIndex(HANDLE hConn)
{
	int i;

	EnterCriticalSection(&gatewayMutex);

	for (i = 0; i < gatewayCount; i++)
	{
		if (hConn == gateways[i].hConn)
		{
			LeaveCriticalSection(&gatewayMutex);
			return gateways[i].dwIndex;
		}
	}

	LeaveCriticalSection(&gatewayMutex);
	return 1; // this is default
}

void FreeGatewayIndex(HANDLE hConn)
{
	EnterCriticalSection(&gatewayMutex);

	for (int i = 0; i < gatewayCount; i++)
	{
		if (hConn == gateways[i].hConn)
		{
			gatewayCount--;
			memmove(&gateways[i], &gateways[i+1], sizeof(gateway_index) * (gatewayCount - i));
			gateways = (gateway_index*)SAFE_REALLOC(gateways, sizeof(gateway_index) * gatewayCount);

			// Gateway found, exit loop
			break;
		}
	}

	LeaveCriticalSection(&gatewayMutex);
}

void CIcqProto::AddToSpammerList(DWORD dwUIN)
{
	EnterCriticalSection(&cookieMutex);

	spammerList = (DWORD *)SAFE_REALLOC(spammerList, sizeof(DWORD) * (spammerListCount + 1));
	spammerList[spammerListCount] = dwUIN;
	spammerListCount++;

	LeaveCriticalSection(&cookieMutex);
}

BOOL CIcqProto::IsOnSpammerList(DWORD dwUIN)
{
	EnterCriticalSection(&cookieMutex);

	for (int i = 0; i < spammerListCount; i++)
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

// ICQ contacts cache

void CIcqProto::AddToContactsCache(HANDLE hContact, DWORD dwUin, const char *szUid)
{
	if (!hContact || (!dwUin && !szUid))
		return;

#ifdef _DEBUG
  NetLog_Server("Adding contact to cache: %u%s%s", dwUin, dwUin ? "" : " - ", dwUin ? "" : szUid);
#endif

  icq_contacts_cache *cache_item = (icq_contacts_cache*)SAFE_MALLOC(sizeof(icq_contacts_cache));
  cache_item->hContact = hContact;
  cache_item->dwUin = dwUin;
  if (!dwUin)
    cache_item->szUid = null_strdup(szUid);

	EnterCriticalSection(&contactsCacheMutex);
  contactsCache.insert(cache_item);
	LeaveCriticalSection(&contactsCacheMutex);
}


void CIcqProto::InitContactsCache()
{
	InitializeCriticalSection(&contactsCacheMutex);
  if (!gatewayMutexRef++)
	  InitializeCriticalSection(&gatewayMutex);

	// build cache
	EnterCriticalSection(&contactsCacheMutex);

  HANDLE hContact = FindFirstContact();

	while (hContact)
	{
		DWORD dwUin;
    uid_str szUid;

    if (!getContactUid(hContact, &dwUin, &szUid))
      AddToContactsCache(hContact, dwUin, szUid);

		hContact = FindNextContact(hContact);
	}

	LeaveCriticalSection(&contactsCacheMutex);
}


void CIcqProto::UninitContactsCache(void)
{
  EnterCriticalSection(&contactsCacheMutex);
  // cleanup the cache
	for (int i = 0; i < contactsCache.getCount(); i++)
  {
    icq_contacts_cache *cache_item = contactsCache[i];

    SAFE_FREE((void**)&cache_item->szUid);
    SAFE_FREE((void**)&cache_item);
  }
  contactsCache.destroy();
  LeaveCriticalSection(&contactsCacheMutex);
	DeleteCriticalSection(&contactsCacheMutex);
  if (!--gatewayMutexRef)
	  DeleteCriticalSection(&gatewayMutex);
}


void CIcqProto::DeleteFromContactsCache(HANDLE hContact)
{
  EnterCriticalSection(&contactsCacheMutex);

	for (int i = 0; i < contactsCache.getCount(); i++)
	{
    icq_contacts_cache *cache_item = contactsCache[i];

		if (cache_item->hContact == hContact)
		{
#ifdef _DEBUG
			NetLog_Server("Removing contact from cache: %u%s%s, position: %u", cache_item->dwUin, cache_item->dwUin ? "" : " - ", cache_item->dwUin ? "" : cache_item->szUid, i);
#endif
      contactsCache.remove(i);
      // Release memory
      SAFE_FREE((void**)&cache_item->szUid);
      SAFE_FREE((void**)&cache_item);
			break;
    }
  }
  LeaveCriticalSection(&contactsCacheMutex);
}


HANDLE CIcqProto::HandleFromCacheByUid(DWORD dwUin, const char *szUid)
{
	HANDLE hContact = NULL;
  icq_contacts_cache cache_item = {NULL, dwUin, szUid};

	EnterCriticalSection(&contactsCacheMutex);
  // find in list
  int i = contactsCache.getIndex(&cache_item);
  if (i != -1)
		hContact = contactsCache[i]->hContact;
  LeaveCriticalSection(&contactsCacheMutex);

	return hContact;
}


HANDLE CIcqProto::HContactFromUIN(DWORD dwUin, int *Added)
{
	HANDLE hContact;

	if (Added) *Added = 0;

	hContact = HandleFromCacheByUid(dwUin, NULL);
	if (hContact) return hContact;

	hContact = FindFirstContact();
	while (hContact)
	{
		DWORD dwContactUin;

		dwContactUin = getContactUin(hContact);
		if (dwContactUin == dwUin)
		{
			AddToContactsCache(hContact, dwUin, NULL);
			return hContact;
		}

		hContact = FindNextContact(hContact);
	}

	//not present: add
	if (Added)
	{
		hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
		if (!hContact)
		{
			NetLog_Server("Failed to create ICQ contact %u", dwUin);
			return INVALID_HANDLE_VALUE;
		}

		if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)m_szModuleName) != 0)
		{
			// For some reason we failed to register the protocol to this contact
			CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
			NetLog_Server("Failed to register ICQ contact %u", dwUin);
			return INVALID_HANDLE_VALUE;
		}

		setSettingDword(hContact, UNIQUEIDSETTING, dwUin);

		if (!bIsSyncingCL)
		{
			DBWriteContactSettingByte(hContact, "CList", "NotOnList", 1);
			setContactHidden(hContact, 1);

			setSettingWord(hContact, "Status", ID_STATUS_OFFLINE);

			icq_QueueUser(hContact);

			if (icqOnline())
			{
				icq_sendNewContact(dwUin, NULL);
			}
			if (getSettingByte(NULL, "KillSpambots", DEFAULT_KILLSPAM_ENABLED))
				icq_sendCheckSpamBot(hContact, dwUin, NULL);
		}
		AddToContactsCache(hContact, dwUin, NULL);
		*Added = 1;

		return hContact;
	}

	// not in list, check that uin do not belong to us
	if (getContactUin(NULL) == dwUin)
		return NULL;

	return INVALID_HANDLE_VALUE;
}


HANDLE CIcqProto::HContactFromUID(DWORD dwUin, const char *szUid, int *Added)
{
	HANDLE hContact;

	if (dwUin)
		return HContactFromUIN(dwUin, Added);

	if (Added) *Added = 0;

	if (!m_bAimEnabled) return INVALID_HANDLE_VALUE;

  hContact = HandleFromCacheByUid(dwUin, szUid);
	if (hContact) return hContact;

	hContact = FindFirstContact();
	while (hContact)
	{
    DWORD dwContactUin;
    uid_str szContactUid;

		if (!getContactUid(hContact, &dwContactUin, &szContactUid))
		{
			if (!dwContactUin && !stricmpnull(szContactUid, szUid))
			{
				if (strcmpnull(szContactUid, szUid))
				{ // fix case in SN
					setSettingString(hContact, UNIQUEIDSETTING, szUid);
				}
				return hContact;
			}
		}
		hContact = FindNextContact(hContact);
	}

	//not present: add
	if (Added)
	{
		hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
		CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)m_szModuleName);

		setSettingString(hContact, UNIQUEIDSETTING, szUid);

		if (!bIsSyncingCL)
		{
			DBWriteContactSettingByte(hContact, "CList", "NotOnList", 1);
			setContactHidden(hContact, 1);

			setSettingWord(hContact, "Status", ID_STATUS_OFFLINE);

			if (icqOnline())
			{
				icq_sendNewContact(0, szUid);
			}
			if (getSettingByte(NULL, "KillSpambots", DEFAULT_KILLSPAM_ENABLED))
				icq_sendCheckSpamBot(hContact, 0, szUid);
		}
		AddToContactsCache(hContact, 0, szUid);
		*Added = 1;

		return hContact;
	}

	return INVALID_HANDLE_VALUE;
}


HANDLE CIcqProto::HContactFromAuthEvent(HANDLE hEvent)
{
	DBEVENTINFO dbei;
	DWORD body[2];

	ZeroMemory(&dbei, sizeof(dbei));
	dbei.cbSize = sizeof(dbei);
	dbei.cbBlob = sizeof(DWORD)*2;
	dbei.pBlob = (PBYTE)&body;

	if (CallService(MS_DB_EVENT_GET, (WPARAM)hEvent, (LPARAM)&dbei))
		return INVALID_HANDLE_VALUE;

	if (dbei.eventType != EVENTTYPE_AUTHREQUEST)
		return INVALID_HANDLE_VALUE;

	if (strcmpnull(dbei.szModule, m_szModuleName))
		return INVALID_HANDLE_VALUE;

	return (HANDLE)body[1]; // this is bad - needs new auth system
}

char *NickFromHandle(HANDLE hContact)
{
	if (hContact == INVALID_HANDLE_VALUE)
		return null_strdup(ICQTranslate("<invalid>"));

	return null_strdup((char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0));
}

char *NickFromHandleUtf(HANDLE hContact)
{
	if (hContact == INVALID_HANDLE_VALUE)
		return ICQTranslateUtf(LPGEN("<invalid>"));

	return mtchar_to_utf8((TCHAR*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, gbUnicodeCore ? GCDNF_UNICODE : 0));
}

char *strUID(DWORD dwUIN, char *pszUID)
{
	if (dwUIN)
		_ltoa(dwUIN, pszUID, 10);

	return pszUID;
}

/* a strlen() that likes NULL */
size_t __fastcall strlennull(const char *string)
{
	if (string)
		return strlen(string);

	return 0;
}


/* a wcslen() that likes NULL */
size_t __fastcall strlennull(const WCHAR *string)
{
  if (string)
    return wcslen(string);

  return 0;
}


/* a strcmp() that likes NULL */
int __fastcall strcmpnull(const char *str1, const char *str2)
{
	if (str1 && str2)
		return strcmp(str1, str2);

  if (!str1 && !str2)
    return 0;

  return 1;
}

/* a stricmp() that likes NULL */
int __fastcall stricmpnull(const char *str1, const char *str2)
{
	if (str1 && str2)
		return _stricmp(str1, str2);

  if (!str1 && !str2)
    return 0;

	return 1;
}

char* __fastcall strstrnull(const char *str, const char *substr)
{
	if (str)
		return (char*)strstr(str, substr);

	return NULL;
}

int null_snprintf(char *buffer, size_t count, const char* fmt, ...)
{
	va_list va;
	int len;

	ZeroMemory(buffer, count);
	va_start(va, fmt);
	len = _vsnprintf(buffer, count-1, fmt, va);
	va_end(va);
	return len;
}

char* __fastcall null_strdup(const char *string)
{
	if (string)
		return _strdup(string);

	return NULL;
}

size_t __fastcall null_strcut(char *string, size_t maxlen)
{ // limit the string to max length (null & utf-8 strings ready)
	size_t len = strlennull(string);

	if (len < maxlen) 
		return len;

	len = maxlen;

	if (UTF8_IsValid(string)) // handle utf-8 string
	{ // find the first byte of possible multi-byte character
		while ((string[len] & 0xc0) == 0x80) len--;
	}
	// simply cut the string
	string[len] = '\0';

	return len;
}

void parseServerAddress(char* szServer, WORD* wPort)
{
	int i = 0;

	while (szServer[i] && szServer[i] != ':') i++;
	if (szServer[i] == ':')
	{ // port included
		*wPort = atoi(&szServer[i + 1]);
	} // otherwise do not change port

	szServer[i] = '\0';
}

char *DemangleXml(const char *string, int len)
{
	char *szWork = (char*)SAFE_MALLOC(len+1), *szChar = szWork;
	int i;

	for (i=0; i<len; i++)
	{
		if (!_strnicmp(string+i, "&gt;", 4))
		{
			*szChar = '>';
			szChar++;
			i += 3;
		}
		else if (!_strnicmp(string+i, "&lt;", 4))
		{
			*szChar = '<';
			szChar++;
			i += 3;
		}
		else if (!_strnicmp(string+i, "&amp;", 5))
		{
			*szChar = '&';
			szChar++;
			i += 4;
		}
		else if (!_strnicmp(string+i, "&quot;", 6))
		{
			*szChar = '"';
			szChar++;
			i += 5;
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
		if (string[i]=='<' || string[i]=='>') l += 4; else if (string[i]=='&') l += 5; else l++;
	}
	szChar = szWork = (char*)SAFE_MALLOC(l + 1);
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
		else if (string[i]=='&')
		{
			*(DWORD*)szChar = 'pma&';
			szChar += 4;
			*szChar = ';';
			szChar++;
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

char *EliminateHtml(const char *string, int len)
{
	char *tmp = (char*)SAFE_MALLOC(len + 1);
	int i,j;
	BOOL tag = FALSE;
	char *res;

	for (i=0,j=0;i<len;i++)
	{
		if (!tag && string[i] == '<')
		{
			if ((i + 4 <= len) && (!_strnicmp(string + i, "<br>", 4) || !_strnicmp(string + i, "<br/>", 5)))
			{ // insert newline
				tmp[j] = '\r';
				j++;
				tmp[j] = '\n';
				j++;
			}
			tag = TRUE;
		}
		else if (tag && string[i] == '>')
		{
			tag = FALSE;
		}
		else if (!tag)
		{
			tmp[j] = string[i];
			j++;
		}
		tmp[j] = '\0';
	}
	SAFE_FREE((void**)&string);
	res = DemangleXml(tmp, strlennull(tmp));
	SAFE_FREE((void**)&tmp);

	return res;
}

char *ApplyEncoding(const char *string, const char *pszEncoding)
{ // decode encoding to Utf-8
	if (string && pszEncoding)
	{ // we do only encodings known to icq5.1 // TODO: check if this is enough
		if (!_strnicmp(pszEncoding, "utf-8", 5))
		{ // it is utf-8 encoded
			return null_strdup(string);
		}
		if (!_strnicmp(pszEncoding, "unicode-2-0", 11))
		{ // it is UCS-2 encoded
			int wLen = strlennull((WCHAR*)string) + 1;
			WCHAR *szStr = (WCHAR*)_alloca(wLen*2);
			BYTE *tmp = (BYTE*)string;

			unpackWideString(&tmp, szStr, (WORD)(wLen*2));

			return make_utf8_string(szStr);
		}
		if (!_strnicmp(pszEncoding, "iso-8859-1", 10))
		{ // we use "Latin I" instead - it does the job
			return ansi_to_utf8_codepage(string, 1252);
		}
	}
	if (string)
	{ // consider it CP_ACP
		return ansi_to_utf8(string);
	}

	return NULL;
}

void CIcqProto::ResetSettingsOnListReload()
{
	HANDLE hContact;

	// Reset a bunch of session specific settings
	setSettingWord(NULL, "SrvVisibilityID", 0);
	setSettingWord(NULL, "SrvAvatarID", 0);
	setSettingWord(NULL, "SrvPhotoID", 0);
	setSettingWord(NULL, "SrvRecordCount", 0);

	hContact = FindFirstContact();

	while (hContact)
	{
		// All these values will be restored during the serv-list receive
		setSettingWord(hContact, "ServerId", 0);
		setSettingWord(hContact, "SrvGroupId", 0);
		setSettingWord(hContact, "SrvPermitId", 0);
		setSettingWord(hContact, "SrvDenyId", 0);
		setSettingByte(hContact, "Auth", 0);
    deleteSetting(hContact, "ServerData");

		hContact = FindNextContact(hContact);
	}

	FlushSrvGroupsCache();
}

void CIcqProto::ResetSettingsOnConnect()
{
	HANDLE hContact;

	// Reset a bunch of session specific settings
	setSettingByte(NULL, "SrvVisibility", 0);
	setSettingDword(NULL, "IdleTS", 0);

	hContact = FindFirstContact();

	while (hContact)
	{
		setSettingDword(hContact, "LogonTS", 0);
		setSettingDword(hContact, "IdleTS", 0);
		setSettingDword(hContact, "TickTS", 0);
		setSettingByte(hContact, "TemporaryVisible", 0);

		// All these values will be restored during the login
		if (getContactStatus(hContact) != ID_STATUS_OFFLINE)
			setSettingWord(hContact, "Status", ID_STATUS_OFFLINE);

		hContact = FindNextContact(hContact);
	}
}

void CIcqProto::ResetSettingsOnLoad()
{
	HANDLE hContact;

	setSettingDword(NULL, "IdleTS", 0);
	setSettingDword(NULL, "LogonTS", 0);

	hContact = FindFirstContact();

	while (hContact)
	{
		setSettingDword(hContact, "LogonTS", 0);
		setSettingDword(hContact, "IdleTS", 0);
		setSettingDword(hContact, "TickTS", 0);
		if (getContactStatus(hContact) != ID_STATUS_OFFLINE)
		{
			setSettingWord(hContact, "Status", ID_STATUS_OFFLINE);

			deleteSetting(hContact, DBSETTING_XSTATUSID);
			deleteSetting(hContact, DBSETTING_XSTATUSNAME);
			deleteSetting(hContact, DBSETTING_XSTATUSMSG);
		}
		setSettingByte(hContact, "DCStatus", 0);

		hContact = FindNextContact(hContact);
	}
}

int RandRange(int nLow, int nHigh)
{
	return nLow + (int)((nHigh-nLow+1)*rand()/(RAND_MAX+1.0));
}

BOOL IsStringUIN(char* pszString)
{
	int i;
	int nLen = strlennull(pszString);


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

static DWORD __stdcall icq_ProtocolAckThread(icq_ack_args* pArguments)
{
	pArguments->ppro->BroadcastAck(pArguments->hContact, pArguments->nAckType, pArguments->nAckResult, pArguments->hSequence, pArguments->pszMessage);

	if (pArguments->nAckResult == ACKRESULT_SUCCESS)
		pArguments->ppro->NetLog_Server("Sent fake message ack");
	else if (pArguments->nAckResult == ACKRESULT_FAILED)
		pArguments->ppro->NetLog_Server("Message delivery failed");

	SAFE_FREE((void**)(char **)&pArguments->pszMessage);
	SAFE_FREE((void**)&pArguments);

	return 0;
}

void CIcqProto::icq_SendProtoAck(HANDLE hContact, DWORD dwCookie, int nAckResult, int nAckType, char* pszMessage)
{
	icq_ack_args* pArgs;


	pArgs = (icq_ack_args*)SAFE_MALLOC(sizeof(icq_ack_args)); // This will be freed in the new thread

	pArgs->ppro = this;
	pArgs->hContact = hContact;
	pArgs->hSequence = (HANDLE)dwCookie;
	pArgs->nAckResult = nAckResult;
	pArgs->nAckType = nAckType;
	pArgs->pszMessage = (LPARAM)null_strdup(pszMessage);

	ICQCreateThread((pThreadFuncEx)icq_ProtocolAckThread, pArgs);
}

void CIcqProto::SetCurrentStatus(int nStatus)
{
	int nOldStatus = m_iStatus;

	m_iStatus = nStatus;
	BroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)nOldStatus, nStatus);
}

BOOL CIcqProto::writeDbInfoSettingString(HANDLE hContact, const char* szSetting, char** buf, WORD* pwLength)
{
	WORD wLen;

	if (*pwLength < 2)
		return FALSE;

	unpackLEWord((LPBYTE*)buf, &wLen);
	*pwLength -= 2;

	if (*pwLength < wLen)
		return FALSE;

	if ((wLen > 0) && (**buf) && ((*buf)[wLen-1]==0)) // Make sure we have a proper string
	{
		WORD wCp = getSettingWord(hContact, "InfoCodePage", getSettingWord(hContact, "InfoCP", CP_ACP));

		if (wCp != CP_ACP)
		{
			char *szUtf = ansi_to_utf8_codepage(*buf, wCp);

			if (szUtf)
			{
				setSettingStringUtf(hContact, szSetting, szUtf);
				SAFE_FREE((void**)&szUtf);
			}
			else
				setSettingString(hContact, szSetting, *buf);
		}
		else
			setSettingString(hContact, szSetting, *buf);
	}
	else
		deleteSetting(hContact, szSetting);

	*buf += wLen;
	*pwLength -= wLen;

	return TRUE;
}

BOOL CIcqProto::writeDbInfoSettingWord(HANDLE hContact, const char *szSetting, char **buf, WORD* pwLength)
{
	WORD wVal;


	if (*pwLength < 2)
		return FALSE;

	unpackLEWord((LPBYTE*)buf, &wVal);
	*pwLength -= 2;

	if (wVal != 0)
		setSettingWord(hContact, szSetting, wVal);
	else
		deleteSetting(hContact, szSetting);

	return TRUE;
}

BOOL CIcqProto::writeDbInfoSettingWordWithTable(HANDLE hContact, const char *szSetting, struct fieldnames_t *table, char **buf, WORD* pwLength)
{
	WORD wVal;
	char sbuf[MAX_PATH];
	char *text;

	if (*pwLength < 2)
		return FALSE;

	unpackLEWord((LPBYTE*)buf, &wVal);
	*pwLength -= 2;

	text = LookupFieldNameUtf(table, wVal, sbuf, MAX_PATH);
	if (text)
		setSettingStringUtf(hContact, szSetting, text);
	else
		deleteSetting(hContact, szSetting);

	return TRUE;
}

BOOL CIcqProto::writeDbInfoSettingByte(HANDLE hContact, const char *pszSetting, char **buf, WORD* pwLength)
{
	BYTE byVal;

	if (*pwLength < 1)
		return FALSE;

	unpackByte((LPBYTE*)buf, &byVal);
	*pwLength -= 1;

	if (byVal != 0)
		setSettingByte(hContact, pszSetting, byVal);
	else
		deleteSetting(hContact, pszSetting);

	return TRUE;
}

BOOL CIcqProto::writeDbInfoSettingByteWithTable(HANDLE hContact, const char *szSetting, struct fieldnames_t *table, char **buf, WORD* pwLength)
{
	BYTE byVal;
	char sbuf[MAX_PATH];
	char *text;

	if (*pwLength < 1)
		return FALSE;

	unpackByte((LPBYTE*)buf, &byVal);
	*pwLength -= 1;

	text = LookupFieldNameUtf(table, byVal, sbuf, MAX_PATH);
	if (text)
		setSettingStringUtf(hContact, szSetting, text);
	else
		deleteSetting(hContact, szSetting);

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

BOOL CIcqProto::validateStatusMessageRequest(HANDLE hContact, WORD byMessageType)
{
	// Privacy control
	if (getSettingByte(NULL, "StatusMsgReplyCList", 0))
	{
		// Don't send statusmessage to unknown contacts
		if (hContact == INVALID_HANDLE_VALUE)
			return FALSE;

		// Don't send statusmessage to temporary contacts or hidden contacts
		if (DBGetContactSettingByte(hContact, "CList", "NotOnList", 0) ||
			DBGetContactSettingByte(hContact, "CList", "Hidden", 0))
			return FALSE;

		// Don't send statusmessage to invisible contacts
		if (getSettingByte(NULL, "StatusMsgReplyVisible", 0))
		{
			WORD wStatus = getContactStatus(hContact);
			if (wStatus == ID_STATUS_OFFLINE)
				return FALSE;
		}
	}

	// Dont send messages to people you are hiding from
	if (hContact != INVALID_HANDLE_VALUE &&
		getSettingWord(hContact, "ApparentMode", 0) == ID_STATUS_OFFLINE)
	{
		return FALSE;
	}

	// Dont respond to request for other statuses than your current one
	if ((byMessageType == MTYPE_AUTOAWAY && m_iStatus != ID_STATUS_AWAY) ||
		(byMessageType == MTYPE_AUTOBUSY && m_iStatus != ID_STATUS_OCCUPIED) ||
		(byMessageType == MTYPE_AUTONA   && m_iStatus != ID_STATUS_NA) ||
		(byMessageType == MTYPE_AUTODND  && m_iStatus != ID_STATUS_DND) ||
		(byMessageType == MTYPE_AUTOFFC  && m_iStatus != ID_STATUS_FREECHAT))
	{
		return FALSE;
	}

	if (hContact != INVALID_HANDLE_VALUE && m_iStatus==ID_STATUS_INVISIBLE &&
		getSettingWord(hContact, "ApparentMode", 0) != ID_STATUS_ONLINE)
	{
		if (!getSettingByte(hContact, "TemporaryVisible", 0))
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

void* __fastcall SAFE_MALLOC(size_t size)
{
	void* p = NULL;

	if (size)
	{
		p = malloc(size);

		if (p)
			ZeroMemory(p, size);
	}
	return p;
}

void* __fastcall SAFE_REALLOC(void* p, size_t size)
{
	if (p)
	{
		return realloc(p, size);
	}
	else
		return SAFE_MALLOC(size);
}

HANDLE NetLib_OpenConnection(HANDLE hUser, const char* szIdent, NETLIBOPENCONNECTION* nloc)
{
	HANDLE hConnection;

	Netlib_Logf(hUser, "%sConnecting to %s:%u", szIdent?szIdent:"", nloc->szHost, nloc->wPort);

	nloc->cbSize = sizeof(NETLIBOPENCONNECTION);
	nloc->flags |= NLOCF_V2;

	hConnection = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)hUser, (LPARAM)nloc);
	if (!hConnection && (GetLastError() == 87))
	{ // this ensures, an old Miranda will be able to connect also
		nloc->cbSize = NETLIBOPENCONNECTION_V1_SIZE;
		hConnection = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)hUser, (LPARAM)nloc);
	}
	return hConnection;
}

HANDLE CIcqProto::NetLib_BindPort(NETLIBNEWCONNECTIONPROC_V2 pFunc, void* lParam, WORD* pwPort, DWORD* pdwIntIP)
{
	NETLIBBIND nlb = {0};
	HANDLE hBoundPort;

	nlb.cbSize = sizeof(NETLIBBIND); 
	nlb.pfnNewConnectionV2 = pFunc;
	nlb.pExtra = lParam;
	SetLastError(ERROR_INVALID_PARAMETER); // this must be here - NetLib does not set any error :((
	hBoundPort = (HANDLE)CallService(MS_NETLIB_BINDPORT, (WPARAM)m_hDirectNetlibUser, (LPARAM)&nlb);
	if (!hBoundPort && (GetLastError() == ERROR_INVALID_PARAMETER))
	{ // this ensures older Miranda also can bind a port for a dc - pre 0.6
		nlb.cbSize = NETLIBBIND_SIZEOF_V2;
		hBoundPort = (HANDLE)CallService(MS_NETLIB_BINDPORT, (WPARAM)m_hDirectNetlibUser, (LPARAM)&nlb);
	}
	if (pwPort) *pwPort = nlb.wPort;
	if (pdwIntIP) *pdwIntIP = nlb.dwInternalIP;

	return hBoundPort;
}

void NetLib_CloseConnection(HANDLE *hConnection, int bServerConn)
{
	if (*hConnection)
	{
		int sck = CallService(MS_NETLIB_GETSOCKET, (WPARAM)*hConnection, (LPARAM)0);

		if (sck!=INVALID_SOCKET) shutdown(sck, 2); // close gracefully

		NetLib_SafeCloseHandle(hConnection);

		if (bServerConn)
			FreeGatewayIndex(*hConnection);
	}
}

void NetLib_SafeCloseHandle(HANDLE *hConnection)
{
	if (*hConnection)
	{
		Netlib_CloseHandle(*hConnection);
		*hConnection = NULL;
	}
}

int CIcqProto::NetLog_Server(const char *fmt,...)
{
	va_list va;
	char szText[1024];

	va_start(va,fmt);
	mir_vsnprintf(szText,sizeof(szText),fmt,va);
	va_end(va);
	return CallService(MS_NETLIB_LOG,(WPARAM)m_hServerNetlibUser,(LPARAM)szText);
}

int CIcqProto::NetLog_Direct(const char *fmt,...)
{
	va_list va;
	char szText[1024];

	va_start(va,fmt);
	mir_vsnprintf(szText,sizeof(szText),fmt,va);
	va_end(va);
	return CallService(MS_NETLIB_LOG,(WPARAM)m_hDirectNetlibUser,(LPARAM)szText);
}

int CIcqProto::NetLog_Uni(BOOL bDC, const char *fmt,...)
{
	va_list va; 
	char szText[1024];
	HANDLE hNetlib;

	va_start(va,fmt);
	mir_vsnprintf(szText,sizeof(szText),fmt,va);
	va_end(va);

	if (bDC)
		hNetlib = m_hDirectNetlibUser;
	else
		hNetlib = m_hServerNetlibUser;

	return CallService(MS_NETLIB_LOG,(WPARAM)hNetlib,(LPARAM)szText);
}

int CIcqProto::BroadcastAck(HANDLE hContact,int type,int result,HANDLE hProcess,LPARAM lParam)
{
	ACKDATA ack={0};

	ack.cbSize = sizeof(ACKDATA);
	ack.szModule = m_szModuleName;
	ack.hContact = hContact;
	ack.type = type;
	ack.result = result;
	ack.hProcess = hProcess;
	ack.lParam = lParam;
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

char* __fastcall ICQTranslate(const char *src)
{
	return (char*)CallService(MS_LANGPACK_TRANSLATESTRING,0,(LPARAM)src);
}

char* __fastcall ICQTranslateUtf(const char *src)
{ // this takes UTF-8 strings only!!!
	char *szRes = NULL;

	if (!strlennull(src))
	{ // for the case of empty strings
		return null_strdup(src);
	}

	{ // we can use unicode translate (0.5+)
		WCHAR* usrc = make_unicode_string(src);

		szRes = make_utf8_string(TranslateW(usrc));

		SAFE_FREE((void**)&usrc);
	}
	return szRes;
}

char* __fastcall ICQTranslateUtfStatic(const char *src, char *buf, size_t bufsize)
{ // this takes UTF-8 strings only!!!
	if (strlennull(src))
	{ // we can use unicode translate (0.5+)
		WCHAR *usrc = make_unicode_string(src);

		make_utf8_string_static(TranslateW(usrc), buf, bufsize);

		SAFE_FREE((void**)&usrc);
	}
	else
		buf[0] = '\0';

	return buf;
}

HANDLE ICQCreateThreadEx(pThreadFuncEx AFunc, void* arg, DWORD* pThreadID)
{
	FORK_THREADEX_PARAMS params;
	DWORD dwThreadId;
	HANDLE hThread;

	params.pFunc      = AFunc;
	params.arg        = arg;
	params.iStackSize = 0;
	params.threadID   = (UINT*)&dwThreadId;
	hThread = (HANDLE)CallService(MS_SYSTEM_FORK_THREAD_EX, 0, (LPARAM)&params);
	if (pThreadID)
		*pThreadID = dwThreadId;

	return hThread;
}

void ICQCreateThread(pThreadFuncEx AFunc, void* arg)
{
	HANDLE hThread = ICQCreateThreadEx(AFunc, arg, NULL);

	CloseHandle(hThread);
}

char* CIcqProto::GetUserPassword(BOOL bAlways)
{
	if (m_szPassword[0] != '\0' && (m_bRememberPwd || bAlways))
		return m_szPassword;

	if (!getSettingStringStatic(NULL, "Password", m_szPassword, sizeof(m_szPassword)))
	{
		CallService(MS_DB_CRYPT_DECODESTRING, strlennull(m_szPassword) + 1, (LPARAM)m_szPassword);

		if (!strlennull(m_szPassword)) return NULL;

		m_bRememberPwd = TRUE;

		return m_szPassword;
	}

	return NULL;
}

WORD CIcqProto::GetMyStatusFlags()
{
	WORD wFlags = 0;

	// Webaware setting bit flag
	if (getSettingByte(NULL, "WebAware", 0))
		wFlags = STATUS_WEBAWARE;

	// DC setting bit flag
	switch (getSettingByte(NULL, "DCType", 0))
	{
	case 0:
		break;

	case 1:
		wFlags = wFlags | STATUS_DCCONT;
		break;

	case 2:
		wFlags = wFlags | STATUS_DCAUTH;
		break;

	default:
		wFlags = wFlags | STATUS_DCDISABLED;
		break;
	}
	return wFlags;
}

int IsValidRelativePath(const char *filename)
{
	if (strstrnull(filename, "..\\") || strstrnull(filename, "../") ||
		strstrnull(filename, ":\\") || strstrnull(filename, ":/") ||
		filename[0] == '\\' || filename[0] == '/')
		return 0; // Contains malicious chars, Failure

	return 1; // Success
}

char *ExtractFileName(const char *fullname)
{
	char *szFileName;

	 // already is only filename
	if (((szFileName = strrchr((char*)fullname, '\\')) == NULL) && ((szFileName = strrchr((char*)fullname, '/')) == NULL))
		return (char*)fullname;

	return szFileName+1;  // skip backslash
}

char *FileNameToUtf(const char *filename)
{
	#if defined( _UNICODE )
		// reasonable only on NT systems
		HINSTANCE hKernel;
		DWORD (CALLBACK *RealGetLongPathName)(LPCWSTR, LPWSTR, DWORD);

		hKernel = GetModuleHandleA("KERNEL32");
		*(FARPROC *)&RealGetLongPathName = GetProcAddress(hKernel, "GetLongPathNameW");

		if (RealGetLongPathName)
		{ // the function is available (it is not on old NT systems)
			WCHAR *unicode, *usFileName = NULL;
			int wchars;

			wchars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, filename,
				strlennull(filename), NULL, 0);

			unicode = (WCHAR*)_alloca((wchars + 1) * sizeof(WCHAR));
			unicode[wchars] = 0;

			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, filename,
				strlennull(filename), unicode, wchars);

			wchars = RealGetLongPathName(unicode, usFileName, 0);
			usFileName = (WCHAR*)_alloca((wchars + 1) * sizeof(WCHAR));
			RealGetLongPathName(unicode, usFileName, wchars);

			return make_utf8_string(usFileName);
		}
	#endif

	return ansi_to_utf8(filename);
}

int FileStatUtf(const char *path, struct _stati64 *buffer)
{
	int wRes = -1;

	#if defined( _UNICODE )
		WCHAR* usPath = make_unicode_string(path);
		wRes = _wstati64(usPath, buffer);
		SAFE_FREE((void**)&usPath);
	#else
		int size = strlennull(path)+2;
		char* szAnsiPath = (char*)_alloca(size);
		if (utf8_decode_static(path, szAnsiPath, size))
			wRes = _stati64(szAnsiPath, buffer);
	#endif

	return wRes;
}

int MakeDirUtf(const char *dir)
{
	int wRes = -1;
	char *szLast;

	#if defined( _UNICODE )
		WCHAR* usDir = make_unicode_string(dir);
		// _wmkdir can created only one dir at once
		wRes = _wmkdir(usDir);
		// check if dir not already existed - return success if yes
		if (wRes == -1 && errno == 17 /* EEXIST */)
			wRes = 0;
		else if (wRes && errno == 2 /* ENOENT */)
		{ // failed, try one directory less first
			szLast = (char*)strrchr((char*)dir, '\\');
			if (!szLast) szLast = (char*)strrchr((char*)dir, '/');
			if (szLast)
			{
				char cOld = *szLast;

				*szLast = '\0';
				if (!MakeDirUtf(dir))
					wRes = _wmkdir(usDir);
				*szLast = cOld;
			}
		}
		SAFE_FREE((void**)&usDir);
	#else
		int size = strlennull(dir)+2;
		char* szAnsiDir = (char*)_alloca(size);

		if (utf8_decode_static(dir, szAnsiDir, size))
		{ // _mkdir can create only one dir at once
			wRes = _mkdir(szAnsiDir);
			// check if dir not already existed - return success if yes
			if (wRes == -1 && errno == 17 /* EEXIST */)
				wRes = 0;
			else if (wRes && errno == 2 /* ENOENT */)
			{ // failed, try one directory less first
				szLast = (char*)strrchr((char*)dir, '\\');
				if (!szLast) szLast = (char*)strrchr((char*)dir, '/');
				if (szLast)
				{
					char cOld = *szLast;

					*szLast = '\0';
					if (!MakeDirUtf(dir))
						wRes = _mkdir(szAnsiDir);
					*szLast = cOld;
				}
			}
		}
	#endif

	return wRes;
}

int OpenFileUtf(const char *filename, int oflag, int pmode)
{
	int hFile = -1;

	#if defined( _UNICODE )
		WCHAR* usFile = make_unicode_string(filename);
		hFile = _wopen(usFile, oflag, pmode);
		SAFE_FREE((void**)&usFile);
	#else
		int size = strlennull(filename)+2;
		char* szAnsiFile = (char*)_alloca(size);

		if (utf8_decode_static(filename, szAnsiFile, size))
			hFile = _open(szAnsiFile, oflag, pmode); 
	#endif

	return hFile;
}

WCHAR *GetWindowTextUcs(HWND hWnd)
{
	WCHAR *utext;

	#if defined( _UNICODE )
		int nLen = GetWindowTextLengthW(hWnd);

		utext = (WCHAR*)SAFE_MALLOC((nLen+2)*sizeof(WCHAR));
		GetWindowTextW(hWnd, utext, nLen + 1);
	#else
		char *text;
		int wchars, nLen = GetWindowTextLengthA(hWnd);

		text = (char*)_alloca(nLen+2);
		GetWindowTextA(hWnd, text, nLen + 1);

		wchars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, text,
			strlennull(text), NULL, 0);

		utext = (WCHAR*)SAFE_MALLOC((wchars + 1)*sizeof(WCHAR));

		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, text,
			strlennull(text), utext, wchars);
	#endif
	return utext;
}

void SetWindowTextUcs(HWND hWnd, WCHAR *text)
{
	#if defined( _UNICODE )
		SetWindowTextW(hWnd, text);
	#else
		char *tmp = (char*)SAFE_MALLOC(strlennull(text) + 1);
		WideCharToMultiByte(CP_ACP, 0, text, -1, tmp, strlennull(text)+1, NULL, NULL);
		SetWindowTextA(hWnd, tmp);
		SAFE_FREE((void**)&tmp);
	#endif
}

char *GetWindowTextUtf(HWND hWnd)
{
	TCHAR* szText;

	#if defined( _UNICODE )
		int nLen = GetWindowTextLengthW(hWnd);

		szText = (TCHAR*)_alloca((nLen+2)*sizeof(WCHAR));
		GetWindowTextW(hWnd, (WCHAR*)szText, nLen + 1);
	#else
		int nLen = GetWindowTextLengthA(hWnd);

		szText = (TCHAR*)_alloca(nLen+2);
		GetWindowTextA(hWnd, (char*)szText, nLen + 1);
	#endif

	return tchar_to_utf8(szText);
}

char *GetDlgItemTextUtf(HWND hwndDlg, int iItem)
{
	return GetWindowTextUtf(GetDlgItem(hwndDlg, iItem));
}

void SetWindowTextUtf(HWND hWnd, const char *szText)
{
	#if defined( _UNICODE )
		WCHAR* usText = make_unicode_string(szText);
		SetWindowTextW(hWnd, usText);
		SAFE_FREE((void**)&usText);
	#else
		int size = strlennull(szText)+2;
		char* szAnsi = (char*)_alloca(size);

		if (utf8_decode_static(szText, szAnsi, size))
			SetWindowTextA(hWnd, szAnsi);
	#endif
}

void SetDlgItemTextUtf(HWND hwndDlg, int iItem, const char *szText)
{
	SetWindowTextUtf(GetDlgItem(hwndDlg, iItem), szText);
}

static int ControlAddStringUtf(HWND ctrl, DWORD msg, const char *szString)
{
	char str[MAX_PATH];
	char *szItem = ICQTranslateUtfStatic(szString, str, MAX_PATH);
	int item = -1;

	#if defined( _UNICODE )
		WCHAR *wItem = make_unicode_string(szItem);
		item = SendMessage(ctrl, msg, 0, (LPARAM)wItem);
		SAFE_FREE((void**)&wItem);
	#else
		int size = strlennull(szItem) + 2;
		char *aItem = (char*)_alloca(size);

		if (utf8_decode_static(szItem, aItem, size))
			item = SendMessage(ctrl, msg, 0, (LPARAM)aItem);
	#endif

	return item;
}

int ComboBoxAddStringUtf(HWND hCombo, const char *szString, DWORD data)
{
	int item = ControlAddStringUtf(hCombo, CB_ADDSTRING, szString);
	SendMessage(hCombo, CB_SETITEMDATA, item, data);

	return item;
}

int ListBoxAddStringUtf(HWND hList, const char *szString)
{
	return ControlAddStringUtf(hList, LB_ADDSTRING, szString);
}

int MessageBoxUtf(HWND hWnd, const char *szText, const char *szCaption, UINT uType)
{
	int res;
	char str[1024];
	char cap[MAX_PATH];

	#if defined( _UNICODE )
		WCHAR *text = make_unicode_string(ICQTranslateUtfStatic(szText, str, 1024));
		WCHAR *caption = make_unicode_string(ICQTranslateUtfStatic(szCaption, cap, MAX_PATH));
		res = MessageBoxW(hWnd, text, caption, uType);
		SAFE_FREE((void**)&caption);
		SAFE_FREE((void**)&text);
	#else
		int size = strlennull(szText) + 2, size2 = strlennull(szCaption) + 2;
		char *text = (char*)_alloca(size);
		char *caption = (char*)_alloca(size2);

		utf8_decode_static(ICQTranslateUtfStatic(szText, str, 1024), text, size);
		utf8_decode_static(ICQTranslateUtfStatic(szCaption, cap, MAX_PATH), caption, size2);
		res = MessageBoxA(hWnd, text, caption, uType);
	#endif

	return res;
}

char* CIcqProto::ConvertMsgToUserSpecificAnsi(HANDLE hContact, const char* szMsg)
{ // this takes utf-8 encoded message
	WORD wCP = getSettingWord(hContact, "CodePage", m_wAnsiCodepage);
	char* szAnsi = NULL;

	if (wCP != CP_ACP) // convert to proper codepage
		if (!utf8_decode_codepage(szMsg, &szAnsi, wCP))
			return NULL;

	return szAnsi;
}

// just broadcast generic send error with dummy cookie and return that cookie
DWORD CIcqProto::ReportGenericSendError(HANDLE hContact, int nType, const char* szErrorMsg)
{ 
	DWORD dwCookie = GenerateCookie(0);
	icq_SendProtoAck(hContact, dwCookie, ACKRESULT_FAILED, nType, ICQTranslate(szErrorMsg));
	return dwCookie;
}

/////////////////////////////////////////////////////////////////////////////////////////

void CIcqProto::CreateProtoService(const char* szService, IcqServiceFunc serviceProc)
{
	char temp[MAX_PATH*2];

	null_snprintf(temp, sizeof(temp), "%s%s", m_szModuleName, szService);
	CreateServiceFunctionObj( temp, ( MIRANDASERVICEOBJ )*( void** )&serviceProc, this );
}

void CIcqProto::CreateProtoServiceParam(const char* szService, IcqServiceFuncParam serviceProc, LPARAM lParam)
{
	char temp[MAX_PATH*2];

	null_snprintf(temp, sizeof(temp), "%s%s", m_szModuleName, szService);
	CreateServiceFunctionObjParam( temp, ( MIRANDASERVICEOBJPARAM )*( void** )&serviceProc, this, lParam );
}

void CIcqProto::HookProtoEvent(const char* szEvent, IcqEventFunc pFunc)
{
	::HookEventObj( szEvent, ( MIRANDAHOOKOBJ )*( void** )&pFunc, this );
}

HANDLE CIcqProto::CreateProtoEvent(const char* szEvent)
{
	char str[MAX_PATH + 32];
	strcpy(str, m_szModuleName);
	strcat(str, szEvent);
	return CreateHookableEvent(str);
}
