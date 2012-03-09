// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright � 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001-2002 Jon Keating, Richard Hughes
// Copyright � 2002-2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004-2010 Joe Kucera
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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

static icq_critical_section *gatewayMutex = NULL;

static gateway_index *gateways = NULL;
static int gatewayCount = 0;

static DWORD *spammerList = NULL;
static int spammerListCount = 0;


void MoveDlgItem(HWND hwndDlg, int iItem, int left, int top, int width, int height)
{
	RECT rc;

	rc.left = left;
	rc.top = top;
	rc.right = left + width;
	rc.bottom = top + height;
	MapDialogRect(hwndDlg, &rc);
	MoveWindow(GetDlgItem(hwndDlg, iItem), rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
}


void EnableDlgItem(HWND hwndDlg, UINT control, int state)
{
	EnableWindow(GetDlgItem(hwndDlg, control), state);
}


void ShowDlgItem(HWND hwndDlg, UINT control, int state)
{
	ShowWindow(GetDlgItem(hwndDlg, control), state);
}


void icq_EnableMultipleControls(HWND hwndDlg, const UINT *controls, int cControls, int state)
{
	for (int i = 0; i < cControls; i++)
		EnableDlgItem(hwndDlg, controls[i], state);
}


void icq_ShowMultipleControls(HWND hwndDlg, const UINT *controls, int cControls, int state)
{
	for (int i = 0; i < cControls; i++)
		ShowDlgItem(hwndDlg, controls[i], state);
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
case ID_STATUS_OFFLINE:
	nSupportedStatus = nMirandaStatus;
	break;

case ID_STATUS_FREECHAT:
	nSupportedStatus = ID_STATUS_ONLINE;
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
	return tchar_to_utf8((TCHAR*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, mirandaStatus, GSMDF_TCHAR));
}

char** CIcqProto::MirandaStatusToAwayMsg(int nStatus)
{
	switch (nStatus) {

case ID_STATUS_ONLINE:
	return &m_modeMsgs.szOnline;

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
case MTYPE_AUTOONLINE:
	return ID_STATUS_ONLINE;

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
	icq_lock l(gatewayMutex);

	for (int i = 0; i < gatewayCount; i++)
	{
		if (hConn == gateways[i].hConn)
		{
			gateways[i].dwIndex = dwIndex;
			return;
		}
	}

	gateways = (gateway_index *)SAFE_REALLOC(gateways, sizeof(gateway_index) * (gatewayCount + 1));
	gateways[gatewayCount].hConn = hConn;
	gateways[gatewayCount].dwIndex = dwIndex;
	gatewayCount++;
}


DWORD GetGatewayIndex(HANDLE hConn)
{
	icq_lock l(gatewayMutex);

	for (int i = 0; i < gatewayCount; i++)
	{
		if (hConn == gateways[i].hConn)
			return gateways[i].dwIndex;
	}

	return 1; // this is default
}


void FreeGatewayIndex(HANDLE hConn)
{
	icq_lock l(gatewayMutex);

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
}


void CIcqProto::AddToSpammerList(DWORD dwUIN)
{
	icq_lock l(gatewayMutex);

	spammerList = (DWORD *)SAFE_REALLOC(spammerList, sizeof(DWORD) * (spammerListCount + 1));
	spammerList[spammerListCount] = dwUIN;
	spammerListCount++;
}


BOOL CIcqProto::IsOnSpammerList(DWORD dwUIN)
{
	icq_lock l(gatewayMutex);

	for (int i = 0; i < spammerListCount; i++)
	{
		if (dwUIN == spammerList[i])
			return TRUE;
	}

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

	icq_lock l(contactsCacheMutex);
	contactsCache.insert(cache_item);
}


void CIcqProto::InitContactsCache()
{
	if (!gatewayMutex)
		gatewayMutex = new icq_critical_section();
	else
		gatewayMutex->_Lock();

	contactsCacheMutex = new icq_critical_section();

	// build cache
	icq_lock l(contactsCacheMutex);

	HANDLE hContact = FindFirstContact();

	while (hContact)
	{
		DWORD dwUin;
		uid_str szUid;

		if (!getContactUid(hContact, &dwUin, &szUid))
			AddToContactsCache(hContact, dwUin, szUid);

		hContact = FindNextContact(hContact);
	}
}


void CIcqProto::UninitContactsCache(void)
{
	contactsCacheMutex->Enter();

	// cleanup the cache
	for (int i = 0; i < contactsCache.getCount(); i++)
	{
		icq_contacts_cache *cache_item = contactsCache[i];

		SAFE_FREE((void**)&cache_item->szUid);
		SAFE_FREE((void**)&cache_item);
	}

	contactsCache.destroy();

	contactsCacheMutex->Leave();

	SAFE_DELETE(&contactsCacheMutex);

	if (gatewayMutex && gatewayMutex->getLockCount() > 1)
		gatewayMutex->_Release();
	else
		SAFE_DELETE(&gatewayMutex);
}


void CIcqProto::DeleteFromContactsCache(HANDLE hContact)
{
	icq_lock l(contactsCacheMutex);

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
}


HANDLE CIcqProto::HandleFromCacheByUid(DWORD dwUin, const char *szUid)
{
	icq_contacts_cache cache_item = {NULL, dwUin, szUid};

	icq_lock l(contactsCacheMutex);
	// find in list
	int i = contactsCache.getIndex(&cache_item);
	if (i != -1)
		return contactsCache[i]->hContact;

	return NULL;
}


HANDLE CIcqProto::HContactFromUIN(DWORD dwUin, int *Added)
{
	if (Added) *Added = 0;

	HANDLE hContact = HandleFromCacheByUid(dwUin, NULL);
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
				icq_sendNewContact(dwUin, NULL);
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
	if (dwUin)
		return HContactFromUIN(dwUin, Added);

	if (Added) *Added = 0;

	if (!m_bAimEnabled) return INVALID_HANDLE_VALUE;

	HANDLE hContact = HandleFromCacheByUid(dwUin, szUid);
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
				icq_sendNewContact(0, szUid);
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
	DWORD body[3];

	ZeroMemory(&dbei, sizeof(dbei));
	dbei.cbSize = sizeof(dbei);
	dbei.cbBlob = sizeof(DWORD) + sizeof(HANDLE);
	dbei.pBlob = (PBYTE)&body;

	if (CallService(MS_DB_EVENT_GET, (WPARAM)hEvent, (LPARAM)&dbei))
		return INVALID_HANDLE_VALUE;

	if (dbei.eventType != EVENTTYPE_AUTHREQUEST)
		return INVALID_HANDLE_VALUE;

	if (strcmpnull(dbei.szModule, m_szModuleName))
		return INVALID_HANDLE_VALUE;

	return *(HANDLE*)&body[1]; // this is bad - needs new auth system
}

char *NickFromHandle(HANDLE hContact)
{
	if (hContact == INVALID_HANDLE_VALUE)
		return null_strdup(Translate("<invalid>"));

	return null_strdup((char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0));
}

char *NickFromHandleUtf(HANDLE hContact)
{
	if (hContact == INVALID_HANDLE_VALUE)
		return ICQTranslateUtf(LPGEN("<invalid>"));

	return tchar_to_utf8((TCHAR*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR));
}

char *strUID(DWORD dwUIN, char *pszUID)
{
	if (dwUIN && pszUID)
		_ltoa(dwUIN, pszUID, 10);

	return pszUID;
}


/* a strlen() that likes NULL */
int __fastcall strlennull(const char *string)
{
	if (string)
		return (int)strlen(string);

	return 0;
}


/* a wcslen() that likes NULL */
int __fastcall strlennull(const WCHAR *string)
{
	if (string)
		return (int)wcslen(string);

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

int null_snprintf(char *buffer, size_t count, const char *fmt, ...)
{
	va_list va;
	int len;

	ZeroMemory(buffer, count);
	va_start(va, fmt);
	len = _vsnprintf(buffer, count-1, fmt, va);
	va_end(va);
	return len;
}

int null_snprintf(WCHAR *buffer, size_t count, const WCHAR *fmt, ...)
{
	va_list va;
	int len;

	ZeroMemory(buffer, count * sizeof(WCHAR));
	va_start(va, fmt);
	len = _vsnwprintf(buffer, count, fmt, va);
	va_end(va);
	return len;
}


char* __fastcall null_strdup(const char *string)
{
	if (string)
		return _strdup(string);

	return NULL;
}


WCHAR* __fastcall null_strdup(const WCHAR *string)
{
	if (string)
		return wcsdup(string);

	return NULL;
}


char* __fastcall null_strcpy(char *dest, const char *src, size_t maxlen)
{
	if (!dest)
		return NULL;

	if (src && src[0])
	{
		strncpy(dest, src, maxlen);
		dest[maxlen] = '\0';
	}
	else
		dest[0] = '\0';

	return dest;
}


WCHAR* __fastcall null_strcpy(WCHAR *dest, const WCHAR *src, size_t maxlen)
{
	if (!dest)
		return NULL;

	if (src && src[0])
	{
		wcsncpy(dest, src, maxlen);
		dest[maxlen] = '\0';
	}
	else
		dest[0] = '\0';

	return dest;
}


int __fastcall null_strcut(char *string, int maxlen)
{ // limit the string to max length (null & utf-8 strings ready)
	int len = (int)strlennull(string);

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
		if (string[i]=='<' || string[i]=='>') l += 4; else if (string[i]=='&') l += 5; else if (string[i]=='"') l += 6; else l++;
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
		else if (string[i]=='"')
		{
			*(DWORD*)szChar = 'ouq&';
			szChar += 4;
			*(WORD*)szChar = ';t';
			szChar += 2;
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
	// Reset a bunch of session specific settings
	setSettingWord(NULL, DBSETTING_SERVLIST_PRIVACY, 0);
	setSettingWord(NULL, DBSETTING_SERVLIST_METAINFO, 0);
	setSettingWord(NULL, DBSETTING_SERVLIST_AVATAR, 0);
	setSettingWord(NULL, DBSETTING_SERVLIST_PHOTO, 0);
	setSettingWord(NULL, "SrvRecordCount", 0);
	deleteSetting(NULL, DBSETTING_SERVLIST_UNHANDLED);

	HANDLE hContact = FindFirstContact();

	while (hContact)
	{
		// All these values will be restored during the serv-list receive
		setSettingWord(hContact, DBSETTING_SERVLIST_ID, 0);
		setSettingWord(hContact, DBSETTING_SERVLIST_GROUP, 0);
		setSettingWord(hContact, DBSETTING_SERVLIST_PERMIT, 0);
		setSettingWord(hContact, DBSETTING_SERVLIST_DENY, 0);
		deleteSetting(hContact, DBSETTING_SERVLIST_IGNORE);
		setSettingByte(hContact, "Auth", 0);
		deleteSetting(hContact, DBSETTING_SERVLIST_DATA);

		hContact = FindNextContact(hContact);
	}

	FlushSrvGroupsCache();
}

void CIcqProto::ResetSettingsOnConnect()
{
	// Reset a bunch of session specific settings
	setSettingByte(NULL, "SrvVisibility", 0);
	setSettingDword(NULL, "IdleTS", 0);

	HANDLE hContact = FindFirstContact();

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
	setSettingDword(NULL, "IdleTS", 0);
	setSettingDword(NULL, "LogonTS", 0);

	HANDLE hContact = FindFirstContact();

	while (hContact)
	{
		setSettingDword(hContact, "LogonTS", 0);
		setSettingDword(hContact, "IdleTS", 0);
		setSettingDword(hContact, "TickTS", 0);
		if (getContactStatus(hContact) != ID_STATUS_OFFLINE)
		{
			setSettingWord(hContact, "Status", ID_STATUS_OFFLINE);

			deleteSetting(hContact, DBSETTING_XSTATUS_ID);
			deleteSetting(hContact, DBSETTING_XSTATUS_NAME);
			deleteSetting(hContact, DBSETTING_XSTATUS_MSG);
		}
		setSettingByte(hContact, "DCStatus", 0);

		hContact = FindNextContact(hContact);
	}
}

int RandRange(int nLow, int nHigh)
{
	return nLow + (int)((nHigh-nLow+1)*rand()/(RAND_MAX+1.0));
}


BOOL IsStringUIN(const char *pszString)
{
	int i;
	int nLen = strlennull(pszString);

	if (nLen > 0 && pszString[0] != '0')
	{
		for (i=0; i<nLen; i++)
			if ((pszString[i] < '0') || (pszString[i] > '9'))
				return FALSE;

		return TRUE;
	}

	return FALSE;
}


void __cdecl CIcqProto::ProtocolAckThread(icq_ack_args* pArguments)
{
	Sleep(150);

	if (pArguments->nAckResult == ACKRESULT_SUCCESS)
		NetLog_Server("Sent fake message ack");
	else if (pArguments->nAckResult == ACKRESULT_FAILED)
		NetLog_Server("Message delivery failed");

	BroadcastAck(pArguments->hContact, pArguments->nAckType, pArguments->nAckResult, pArguments->hSequence, pArguments->pszMessage);

	SAFE_FREE((void**)(char **)&pArguments->pszMessage);
	SAFE_FREE((void**)&pArguments);
}

void CIcqProto::SendProtoAck(HANDLE hContact, DWORD dwCookie, int nAckResult, int nAckType, char* pszMessage)
{
	icq_ack_args* pArgs = (icq_ack_args*)SAFE_MALLOC(sizeof(icq_ack_args)); // This will be freed in the new thread
	pArgs->hContact = hContact;
	pArgs->hSequence = (HANDLE)dwCookie;
	pArgs->nAckResult = nAckResult;
	pArgs->nAckType = nAckType;
	pArgs->pszMessage = (LPARAM)null_strdup(pszMessage);

	ForkThread(( IcqThreadFunc )&CIcqProto::ProtocolAckThread, pArgs );
}

void CIcqProto::SetCurrentStatus(int nStatus)
{
	int nOldStatus = m_iStatus;

	m_iStatus = nStatus;
	BroadcastAck(NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)nOldStatus, nStatus);
}


int CIcqProto::IsMetaInfoChanged(HANDLE hContact)
{
	DBVARIANT infoToken = {DBVT_DELETED};
	int res = 0;

	if (!getSetting(hContact, DBSETTING_METAINFO_TOKEN, &infoToken))
	{ // contact does have info from directory, check if it is not outdated
		double dInfoTime = 0;
		double dInfoSaved = 0;

		if ((dInfoTime = getSettingDouble(hContact, DBSETTING_METAINFO_TIME, 0)) > 0)
		{
			if ((dInfoSaved = getSettingDouble(hContact, DBSETTING_METAINFO_SAVED, 0)) > 0)
			{
				if (dInfoSaved < dInfoTime)
					res = 2; // directory info outdated
			}
			else
				res = 1; // directory info not saved at all
		}

		ICQFreeVariant(&infoToken);
	}
	else
	{ // it cannot be detected if user info was not changed, so use a generic threshold
		DBVARIANT infoSaved = {DBVT_DELETED};
		DWORD dwInfoTime = 0;

		if (!getSetting(hContact, DBSETTING_METAINFO_SAVED, &infoSaved))
		{
			if (infoSaved.type == DBVT_BLOB && infoSaved.cpbVal == 8)
			{
				double dwTime = *(double*)infoSaved.pbVal;

				dwInfoTime = (dwTime - 25567) * 86400;
			}
			else if (infoSaved.type == DBVT_DWORD)
				dwInfoTime = infoSaved.dVal;

			ICQFreeVariant(&infoSaved);

			if ((time(NULL) - dwInfoTime) > 14*3600*24)
			{ 
				res = 3; // threshold exceeded
			}
		}
		else
			res = 4; // no timestamp found
	}

	return res;
}


void __cdecl CIcqProto::SetStatusNoteThread(void *pDelay)
{
	if (pDelay)
		SleepEx((DWORD)pDelay, TRUE);

	cookieMutex->Enter();

	if (icqOnline() && (setStatusNoteText || setStatusMoodData))
	{ // send status note change packets, write status note to database
		if (setStatusNoteText)
		{ // change status note in directory
			m_ratesMutex->Enter();
			if (m_rates)
			{ // rate management
				WORD wGroup = m_rates->getGroupFromSNAC(ICQ_EXTENSIONS_FAMILY, ICQ_META_CLI_REQUEST);

				while (m_rates->getNextRateLevel(wGroup) < m_rates->getLimitLevel(wGroup, RML_LIMIT))
				{ // we are over rate, need to wait before sending
					int nDelay = m_rates->getDelayToLimitLevel(wGroup, RML_IDLE_10);

					m_ratesMutex->Leave();
					cookieMutex->Leave();
#ifdef _DEBUG
					NetLog_Server("Rates: SetStatusNote delayed %dms", nDelay);
#endif
					SleepEx(nDelay, TRUE); // do not keep things locked during sleep
					cookieMutex->Enter();
					m_ratesMutex->Enter();
					if (!m_rates) // we lost connection when we slept, go away
						break;
				}
			}
			m_ratesMutex->Leave();

			BYTE *pBuffer = NULL;
			int cbBuffer = 0;

			ppackTLV(&pBuffer, &cbBuffer, 0x226, strlennull(setStatusNoteText), (BYTE*)setStatusNoteText);
			icq_changeUserDirectoryInfoServ(pBuffer, cbBuffer, DIRECTORYREQUEST_UPDATENOTE);

			SAFE_FREE((void**)&pBuffer);
		}

		if (setStatusNoteText || setStatusMoodData)
		{ // change status note and mood in session data
			m_ratesMutex->Enter();
			if (m_rates)
			{ // rate management
				WORD wGroup = m_rates->getGroupFromSNAC(ICQ_SERVICE_FAMILY, ICQ_CLIENT_SET_STATUS);

				while (m_rates->getNextRateLevel(wGroup) < m_rates->getLimitLevel(wGroup, RML_LIMIT))
				{ // we are over rate, need to wait before sending
					int nDelay = m_rates->getDelayToLimitLevel(wGroup, RML_IDLE_10);

					m_ratesMutex->Leave();
					cookieMutex->Leave();
#ifdef _DEBUG
					NetLog_Server("Rates: SetStatusNote delayed %dms", nDelay);
#endif
					SleepEx(nDelay, TRUE); // do not keep things locked during sleep
					cookieMutex->Enter();
					m_ratesMutex->Enter();
					if (!m_rates) // we lost connection when we slept, go away
						break;
				}
			}
			m_ratesMutex->Leave();

			// check if the session data were not updated already
			char *szCurrentStatusNote = getSettingStringUtf(NULL, DBSETTING_STATUS_NOTE, NULL);
			char *szCurrentStatusMood = NULL;
			DBVARIANT dbv = {DBVT_DELETED};

			if (m_bMoodsEnabled && !getSettingString(NULL, DBSETTING_STATUS_MOOD, &dbv))
				szCurrentStatusMood = dbv.pszVal;

			if (!setStatusNoteText && szCurrentStatusNote)
				setStatusNoteText = null_strdup(szCurrentStatusNote);
			if (m_bMoodsEnabled && !setStatusMoodData && szCurrentStatusMood)
				setStatusMoodData = null_strdup(szCurrentStatusMood);

			if (strcmpnull(szCurrentStatusNote, setStatusNoteText) || (m_bMoodsEnabled && strcmpnull(szCurrentStatusMood, setStatusMoodData)))
			{
				setSettingStringUtf(NULL, DBSETTING_STATUS_NOTE, setStatusNoteText);
				if (m_bMoodsEnabled)
					setSettingString(NULL, DBSETTING_STATUS_MOOD, setStatusMoodData);

				WORD wStatusNoteLen = strlennull(setStatusNoteText);
				WORD wStatusMoodLen = m_bMoodsEnabled ? strlennull(setStatusMoodData) : 0;
				icq_packet packet;
				WORD wDataLen = (wStatusNoteLen ? wStatusNoteLen + 4 : 0) + 4 + wStatusMoodLen + 4;

				serverPacketInit(&packet, wDataLen + 14);
				packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_SET_STATUS);
				// Change only session data
				packWord(&packet, 0x1D);              // TLV 1D
				packWord(&packet, wDataLen);          // TLV length
				packWord(&packet, 0x02);              // Item Type
				if (wStatusNoteLen)
				{
					packWord(&packet, 0x400 | (WORD)(wStatusNoteLen + 4)); // Flags + Item Length
					packWord(&packet, wStatusNoteLen);  // Text Length
					packBuffer(&packet, (LPBYTE)setStatusNoteText, wStatusNoteLen);
					packWord(&packet, 0);               // Encoding not specified (utf-8 is default)
				}
				else
					packWord(&packet, 0);               // Flags + Item Length
				packWord(&packet, 0x0E);              // Item Type
				packWord(&packet, wStatusMoodLen);    // Flags + Item Length
				if (wStatusMoodLen)
					packBuffer(&packet, (LPBYTE)setStatusMoodData, wStatusMoodLen); // Mood

				sendServPacket(&packet);
			}
			SAFE_FREE(&szCurrentStatusNote);
			ICQFreeVariant(&dbv);
		}
	}
	SAFE_FREE(&setStatusNoteText);
	SAFE_FREE(&setStatusMoodData);

	cookieMutex->Leave();
}


int CIcqProto::SetStatusNote(const char *szStatusNote, DWORD dwDelay, int bForce)
{
	int bChanged = FALSE;

	// bForce is intended for login sequence - need to call this earlier than icqOnline()
	// the process is delayed and icqOnline() is ready when needed inside SetStatusNoteThread()
	if (!bForce && !icqOnline()) return bChanged;

	// reuse generic critical section (used for cookies list and object variables locks)
	icq_lock l(cookieMutex);

	if (!setStatusNoteText && (!m_bMoodsEnabled || !setStatusMoodData))
	{ // check if the status note was changed and if yes, create thread to change it
		char *szCurrentStatusNote = getSettingStringUtf(NULL, DBSETTING_STATUS_NOTE, NULL);

		if (strcmpnull(szCurrentStatusNote, szStatusNote))
		{ // status note was changed
			// create thread to change status note on existing server connection
			setStatusNoteText = null_strdup(szStatusNote);

			if (dwDelay)
				ForkThread(&CIcqProto::SetStatusNoteThread, (void*)dwDelay);
			else // we cannot afford any delay, so do not run in separate thread
				SetStatusNoteThread(NULL);

			bChanged = TRUE;
		}
		SAFE_FREE(&szCurrentStatusNote);
	}
	else
	{ // only alter status note object with new status note, keep the thread waiting for execution
		SAFE_FREE(&setStatusNoteText);
		setStatusNoteText = null_strdup(szStatusNote);

		bChanged = TRUE;
	}

	return bChanged;
}


int CIcqProto::SetStatusMood(const char *szMoodData, DWORD dwDelay)
{
	int bChanged = FALSE;

	if (!icqOnline()) return bChanged;

	// reuse generic critical section (used for cookies list and object variables locks)
	icq_lock l(cookieMutex);

	if (!setStatusNoteText && !setStatusMoodData)
	{ // check if the status mood was changed and if yes, create thread to change it
		char *szCurrentStatusMood = NULL;
		DBVARIANT dbv = {DBVT_DELETED};

		if (!getSettingString(NULL, DBSETTING_STATUS_MOOD, &dbv))
			szCurrentStatusMood = dbv.pszVal;

		if (strcmpnull(szCurrentStatusMood, szMoodData))
		{ // status mood was changed
			// create thread to change status mood on existing server connection
			setStatusMoodData = null_strdup(szMoodData);
			if (dwDelay)
				ForkThread(&CIcqProto::SetStatusNoteThread, (void*)dwDelay);
			else // we cannot afford any delay, so do not run in separate thread
				SetStatusNoteThread(NULL);

			bChanged = TRUE;
		}
		ICQFreeVariant(&dbv);
	}
	else
	{ // only alter status mood object with new status mood, keep the thread waiting for execution
		SAFE_FREE(&setStatusMoodData);
		setStatusMoodData = null_strdup(szMoodData);

		bChanged = TRUE;
	}

	return bChanged;
}


void CIcqProto::writeDbInfoSettingTLVStringUtf(HANDLE hContact, const char *szSetting, oscar_tlv_chain *chain, WORD wTlv)
{
	oscar_tlv *pTLV = chain->getTLV(wTlv, 1);

	if (pTLV && pTLV->wLen > 0)
	{
		char *str = (char*)_alloca(pTLV->wLen + 1); 

		memcpy(str, pTLV->pData, pTLV->wLen);
		str[pTLV->wLen] = '\0';
		setSettingStringUtf(hContact, szSetting, str);
	}
	else
		deleteSetting(hContact, szSetting);
}


void CIcqProto::writeDbInfoSettingTLVString(HANDLE hContact, const char *szSetting, oscar_tlv_chain *chain, WORD wTlv)
{
	oscar_tlv *pTLV = chain->getTLV(wTlv, 1);

	if (pTLV && pTLV->wLen > 0)
	{
		char *str = (char*)_alloca(pTLV->wLen + 1); 

		memcpy(str, pTLV->pData, pTLV->wLen);
		str[pTLV->wLen] = '\0';
		setSettingString(hContact, szSetting, str);
	}
	else
		deleteSetting(hContact, szSetting);
}


void CIcqProto::writeDbInfoSettingTLVWord(HANDLE hContact, const char *szSetting, oscar_tlv_chain *chain, WORD wTlv)
{
	int num = chain->getNumber(wTlv, 1);

	if (num > 0)
		setSettingWord(hContact, szSetting, num);
	else
		deleteSetting(hContact, szSetting);
}


void CIcqProto::writeDbInfoSettingTLVByte(HANDLE hContact, const char *szSetting, oscar_tlv_chain *chain, WORD wTlv)
{
	int num = chain->getNumber(wTlv, 1);

	if (num > 0)
		setSettingByte(hContact, szSetting, num);
	else
		deleteSetting(hContact, szSetting);
}


void CIcqProto::writeDbInfoSettingTLVDouble(HANDLE hContact, const char *szSetting, oscar_tlv_chain *chain, WORD wTlv)
{
	double num = chain->getDouble(wTlv, 1);

	if (num > 0)
		setSettingDouble(hContact, szSetting, num);
	else
		deleteSetting(hContact, szSetting);
}

void CIcqProto::writeDbInfoSettingTLVDate(HANDLE hContact, const char* szSettingYear, const char* szSettingMonth, const char* szSettingDay, oscar_tlv_chain* chain, WORD wTlv)
{
	double time = chain->getDouble(wTlv, 1);

	if (time > 0)
	{ // date is stored as double with unit equal to a day, incrementing since 1/1/1900 0:00 GMT
		SYSTEMTIME sTime = {0};
		if (VariantTimeToSystemTime(time + 2, &sTime))
		{
			setSettingWord(hContact, szSettingYear, sTime.wYear);
			setSettingByte(hContact, szSettingMonth, (BYTE)sTime.wMonth);
			setSettingByte(hContact, szSettingDay, (BYTE)sTime.wDay);
		}
		else
		{
			deleteSetting(hContact, szSettingYear);
			deleteSetting(hContact, szSettingMonth);
			deleteSetting(hContact, szSettingDay);
		}
	}
	else
	{
		deleteSetting(hContact, szSettingYear);
		deleteSetting(hContact, szSettingMonth);
		deleteSetting(hContact, szSettingDay);
	}
}


void CIcqProto::writeDbInfoSettingTLVBlob(HANDLE hContact, const char *szSetting, oscar_tlv_chain *chain, WORD wTlv)
{
	oscar_tlv *pTLV = chain->getTLV(wTlv, 1);

	if (pTLV && pTLV->wLen > 0)
		setSettingBlob(hContact, szSetting, pTLV->pData, pTLV->wLen);
	else
		deleteSetting(hContact, szSetting);
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

BOOL CIcqProto::writeDbInfoSettingWordWithTable(HANDLE hContact, const char *szSetting, const FieldNamesItem *table, char **buf, WORD* pwLength)
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

BOOL CIcqProto::writeDbInfoSettingByteWithTable(HANDLE hContact, const char *szSetting, const FieldNamesItem *table, char **buf, WORD* pwLength)
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

char* time2text(time_t time)
{
	tm *local = localtime(&time);

	if (local)
	{
		char *str = asctime(local);
		str[24] = '\0'; // remove new line
		return str;
	}
	else
		return "<invalid>";
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
	if ((byMessageType == MTYPE_AUTOONLINE && m_iStatus != ID_STATUS_ONLINE) ||
		(byMessageType == MTYPE_AUTOAWAY && m_iStatus != ID_STATUS_AWAY) ||
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


void __fastcall SAFE_DELETE(void_struct **p)
{
	if (*p)
	{
		delete *p;
		*p = NULL;
	}
}


void __fastcall SAFE_DELETE(lockable_struct **p)
{
	if (*p)
	{
		(*p)->_Release();
		*p = NULL;
	}
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


DWORD ICQWaitForSingleObject(HANDLE hObject, DWORD dwMilliseconds, int bWaitAlways)
{
	DWORD dwResult;

	do { // will get WAIT_IO_COMPLETION for QueueUserAPC(), ignore it unless terminating
		dwResult = WaitForSingleObjectEx(hObject, dwMilliseconds, TRUE);
	} while (dwResult == WAIT_IO_COMPLETION && (bWaitAlways || !Miranda_Terminated()));

	return dwResult;
}


HANDLE NetLib_OpenConnection(HANDLE hUser, const char* szIdent, NETLIBOPENCONNECTION* nloc)
{
	Netlib_Logf(hUser, "%sConnecting to %s:%u", szIdent?szIdent:"", nloc->szHost, nloc->wPort);

	nloc->cbSize = sizeof(NETLIBOPENCONNECTION);
	nloc->flags |= NLOCF_V2;

	return (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)hUser, (LPARAM)nloc);
}


HANDLE CIcqProto::NetLib_BindPort(NETLIBNEWCONNECTIONPROC_V2 pFunc, void* lParam, WORD* pwPort, DWORD* pdwIntIP)
{
	NETLIBBIND nlb = {0};

	nlb.cbSize = sizeof(NETLIBBIND); 
	nlb.pfnNewConnectionV2 = pFunc;
	nlb.pExtra = lParam;
	SetLastError(ERROR_INVALID_PARAMETER); // this must be here - NetLib does not set any error :((

	HANDLE hBoundPort = (HANDLE)CallService(MS_NETLIB_BINDPORT, (WPARAM)m_hDirectNetlibUser, (LPARAM)&nlb);

	if (pwPort) *pwPort = nlb.wPort;
	if (pdwIntIP) *pdwIntIP = nlb.dwInternalIP;

	return hBoundPort;
}


void NetLib_CloseConnection(HANDLE *hConnection, int bServerConn)
{
	if (*hConnection)
	{
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

void CIcqProto::ForkThread( IcqThreadFunc pFunc, void* arg )
{
	CloseHandle(( HANDLE )mir_forkthreadowner(( pThreadFuncOwner )*( void** )&pFunc, this, arg, NULL ));
}

HANDLE CIcqProto::ForkThreadEx( IcqThreadFunc pFunc, void* arg, UINT* threadID )
{
	return ( HANDLE )mir_forkthreadowner(( pThreadFuncOwner )*( void** )&pFunc, this, arg, threadID );
}


char* CIcqProto::GetUserStoredPassword(char *szBuffer, int cbSize)
{
	if (!getSettingStringStatic(NULL, "Password", szBuffer, cbSize))
	{
		CallService(MS_DB_CRYPT_DECODESTRING, strlennull(szBuffer) + 1, (LPARAM)szBuffer);

		if (strlennull(szBuffer))
			return szBuffer;
	}
	return NULL;
}


char* CIcqProto::GetUserPassword(BOOL bAlways)
{
	if (m_szPassword[0] != '\0' && (m_bRememberPwd || bAlways))
		return m_szPassword;

	if (GetUserStoredPassword(m_szPassword, sizeof(m_szPassword)))
	{
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
		wFlags |= STATUS_WEBAWARE;

	// DC setting bit flag
	switch (getSettingByte(NULL, "DCType", 0))
	{
	case 0:
		break;

	case 1:
		wFlags |= STATUS_DCCONT;
		break;

	case 2:
		wFlags |= STATUS_DCAUTH;
		break;

	default:
		wFlags |= STATUS_DCDISABLED;
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


const char* ExtractFileName(const char *fullname)
{
	const char *szFileName;

	// already is only filename
	if (((szFileName = strrchr(fullname, '\\')) == NULL) && ((szFileName = strrchr(fullname, '/')) == NULL))
		return fullname;

	return szFileName + 1;  // skip backslash
}


char* FileNameToUtf(const TCHAR *filename)
{
#if defined( _UNICODE )
	// reasonable only on NT systems
	HINSTANCE hKernel = GetModuleHandle(_T("KERNEL32"));
	DWORD (CALLBACK *RealGetLongPathName)(LPCWSTR, LPWSTR, DWORD);

	*(FARPROC *)&RealGetLongPathName = GetProcAddress(hKernel, "GetLongPathNameW");

	if (RealGetLongPathName)
	{ // the function is available (it is not on old NT systems)
		WCHAR *usFileName = NULL;
		int wchars = RealGetLongPathName(filename, usFileName, 0);
		usFileName = (WCHAR*)_alloca((wchars + 1) * sizeof(WCHAR));
		RealGetLongPathName(filename, usFileName, wchars);

		return make_utf8_string(usFileName);
	}
	return make_utf8_string(filename);
#else
	return ansi_to_utf8(filename);
#endif
}


int FileAccessUtf(const char *path, int mode)
{
	int size = strlennull(path) + 2;
	TCHAR *szPath = (TCHAR*)_alloca(size * sizeof(TCHAR));

	if (utf8_to_tchar_static(path, szPath, size))
		return _taccess(szPath, mode);

	return -1;
}


int FileStatUtf(const char *path, struct _stati64 *buffer)
{
	int size = strlennull(path) + 2;
	TCHAR *szPath = (TCHAR*)_alloca(size * sizeof(TCHAR));

	if (utf8_to_tchar_static(path, szPath, size))
		return _tstati64(szPath, buffer);

	return -1;
}


int MakeDirUtf(const char *dir)
{
	int wRes = -1;
	int size = strlennull(dir) + 2;
	TCHAR *szDir = (TCHAR*)_alloca(size * sizeof(TCHAR));

	if (utf8_to_tchar_static(dir, szDir, size))
	{ // _tmkdir can created only one dir at once
		wRes = _tmkdir(szDir);
		// check if dir not already existed - return success if yes
		if (wRes == -1 && errno == 17 /* EEXIST */)
			wRes = 0;
		else if (wRes && errno == 2 /* ENOENT */)
		{ // failed, try one directory less first
			char *szLast = (char*)strrchr(dir, '\\');
			if (!szLast) szLast = (char*)strrchr(dir, '/');
			if (szLast)
			{
				char cOld = *szLast;

				*szLast = '\0';
				if (!MakeDirUtf(dir))
					wRes = _tmkdir(szDir);

				*szLast = cOld;
			}
		}
	}

	return wRes;
}


int OpenFileUtf(const char *filename, int oflag, int pmode)
{
	int size = strlennull(filename) + 2;
	TCHAR *szFile = (TCHAR*)_alloca(size * sizeof(TCHAR));

	if (utf8_to_tchar_static(filename, szFile, size))
		return _topen(szFile, oflag, pmode);

	return -1;
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


char* GetWindowTextUtf(HWND hWnd)
{
	int nLen = GetWindowTextLength(hWnd);
	TCHAR *szText = (TCHAR*)_alloca((nLen + 2) * sizeof(TCHAR));

	GetWindowText(hWnd, szText, nLen + 1);

	return tchar_to_utf8(szText);
}


char* GetDlgItemTextUtf(HWND hwndDlg, int iItem)
{
	return GetWindowTextUtf(GetDlgItem(hwndDlg, iItem));
}


void SetWindowTextUtf(HWND hWnd, const char *szText)
{
	int size = strlennull(szText) + 2;
	TCHAR *tszText = (TCHAR*)_alloca(size * sizeof(TCHAR));

	if (utf8_to_tchar_static(szText, tszText, size))
		SetWindowText(hWnd, tszText);
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
	SendProtoAck(hContact, dwCookie, ACKRESULT_FAILED, nType, Translate(szErrorMsg));
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


HANDLE CIcqProto::HookProtoEvent(const char* szEvent, IcqEventFunc pFunc)
{
	return ::HookEventObj(szEvent, (MIRANDAHOOKOBJ)*(void**)&pFunc, this);
}


HANDLE CIcqProto::CreateProtoEvent(const char* szEvent)
{
	char str[MAX_PATH + 32];
	strcpy(str, m_szModuleName);
	strcat(str, szEvent);
	return CreateHookableEvent(str);
}
