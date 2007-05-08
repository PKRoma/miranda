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



typedef struct gateway_index_s
{
  HANDLE hConn;
  DWORD  dwIndex;
} gateway_index;

extern CRITICAL_SECTION cookieMutex; 

static gateway_index *gateways = NULL;
static int gatewayCount = 0;

static DWORD *spammerList = NULL;
static int spammerListCount = 0;

typedef struct icq_contacts_cache_s
{
  HANDLE hContact;
  DWORD dwUin;
} icq_contacts_cache;

static icq_contacts_cache *contacts_cache = NULL;
static int cacheCount = 0;
static int cacheListSize = 0;
static CRITICAL_SECTION cacheMutex;

extern BOOL bIsSyncingCL;


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



char* MirandaStatusToStringUtf(int mirandaStatus)
{
  char* szRes = NULL;

  if (gbUnicodeCore)
  { // we can get unicode version, request, give utf-8
    szRes = make_utf8_string((wchar_t *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, mirandaStatus, GCMDF_UNICODE));
  }
  else
  { // we are ansi only, get it, convert to utf-8
    utf8_encode(MirandaStatusToString(mirandaStatus), &szRes);
  }
  return szRes;
}



char**MirandaStatusToAwayMsg(int nStatus)
{
  switch (nStatus)
  {

  case ID_STATUS_AWAY:
    return &modeMsgs.szAway;
    break;

  case ID_STATUS_NA:
    return &modeMsgs.szNa;

  case ID_STATUS_OCCUPIED:
    return &modeMsgs.szOccupied;

  case ID_STATUS_DND:
    return &modeMsgs.szDnd;

  case ID_STATUS_FREECHAT:
    return &modeMsgs.szFfc;

  default:
    return NULL;
  }
}



int AwayMsgTypeToStatus(int nMsgType)
{
  switch (nMsgType)
  {
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



// ICQ contacts cache
static void AddToCache(HANDLE hContact, DWORD dwUin)
{
  int i = 0;

  if (!hContact || !dwUin)
    return;

  EnterCriticalSection(&cacheMutex);

  if (cacheCount + 1 >= cacheListSize)
  {
    cacheListSize += 100;
    contacts_cache = (icq_contacts_cache *)realloc(contacts_cache, sizeof(icq_contacts_cache) * cacheListSize);
  }

#ifdef _DEBUG
  Netlib_Logf(ghServerNetlibUser, "Adding contact to cache: %u, position: %u", dwUin, cacheCount);
#endif

  contacts_cache[cacheCount].hContact = hContact;
  contacts_cache[cacheCount].dwUin = dwUin;

  cacheCount++;

  LeaveCriticalSection(&cacheMutex);
}



void InitCache(void)
{
  HANDLE hContact;

  InitializeCriticalSection(&cacheMutex);
  cacheCount = 0;
  cacheListSize = 0;
  contacts_cache = NULL;

  // build cache
  EnterCriticalSection(&cacheMutex);

  hContact = ICQFindFirstContact();

  while (hContact)
  {
    DWORD dwUin;

    dwUin = ICQGetContactSettingUIN(hContact);
    if (dwUin) AddToCache(hContact, dwUin);

    hContact = ICQFindNextContact(hContact);
  }

  LeaveCriticalSection(&cacheMutex);
}



void UninitCache(void)
{
  SAFE_FREE(&contacts_cache);

  DeleteCriticalSection(&cacheMutex);
}



void DeleteFromCache(HANDLE hContact)
{
  int i;

  if (cacheCount == 0)
    return;

  EnterCriticalSection(&cacheMutex);

  for (i = cacheCount-1; i >= 0; i--)
    if (contacts_cache[i].hContact == hContact)
    {
      cacheCount--;

#ifdef _DEBUG
      Netlib_Logf(ghServerNetlibUser, "Removing contact from cache: %u, position: %u", contacts_cache[i].dwUin, i);
#endif
      // move last contact to deleted position
      if (i < cacheCount)
        memcpy(&contacts_cache[i], &contacts_cache[cacheCount], sizeof(icq_contacts_cache));

      // clear last contact position
      ZeroMemory(&contacts_cache[cacheCount], sizeof(icq_contacts_cache));

      break;
    }

  LeaveCriticalSection(&cacheMutex);
}



static HANDLE HandleFromCacheByUin(DWORD dwUin)
{
  int i;
  HANDLE hContact = NULL;

  if (cacheCount == 0)
    return hContact;

  EnterCriticalSection(&cacheMutex);

  for (i = cacheCount-1; i >= 0; i--)
    if (contacts_cache[i].dwUin == dwUin)
    {
      hContact = contacts_cache[i].hContact;
      break;
    }

  LeaveCriticalSection(&cacheMutex);

  return hContact;
}



HANDLE HContactFromUIN(DWORD uin, int *Added)
{
  HANDLE hContact;

  if (Added) *Added = 0;

  hContact = HandleFromCacheByUin(uin);
  if (hContact) return hContact;

  hContact = ICQFindFirstContact();
  while (hContact != NULL)
  {
    DWORD dwUin;

    dwUin = ICQGetContactSettingUIN(hContact);
    if (dwUin == uin)
    {
      AddToCache(hContact, dwUin);
      return hContact;
    }

    hContact = ICQFindNextContact(hContact);
  }

  //not present: add
  if (Added)
  {
    hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
    if (!hContact)
    {
      NetLog_Server("Failed to create ICQ contact %u", uin);
      return INVALID_HANDLE_VALUE;
    }

    if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)gpszICQProtoName) != 0)
    {
      // For some reason we failed to register the protocol to this contact
      CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
      NetLog_Server("Failed to register ICQ contact %u", uin);
      return INVALID_HANDLE_VALUE;
    }

    ICQWriteContactSettingDword(hContact, UNIQUEIDSETTING, uin);

    if (!bIsSyncingCL)
    {
      DBWriteContactSettingByte(hContact, "CList", "NotOnList", 1);
      SetContactHidden(hContact, 1);

      ICQWriteContactSettingWord(hContact, "Status", ID_STATUS_OFFLINE);

      icq_QueueUser(hContact);

      if (icqOnline)
      {
        icq_sendNewContact(uin, NULL);
      }
    }
    AddToCache(hContact, uin);
    *Added = 1;

    return hContact;
  }

  // not in list, check that uin do not belong to us
  if (ICQGetContactSettingUIN(NULL) == uin)
    return NULL;

  return INVALID_HANDLE_VALUE;
}



HANDLE HContactFromUID(DWORD dwUIN, char* pszUID, int *Added)
{
  HANDLE hContact;
  DWORD dwUin;
  uid_str szUid;

  if (dwUIN)
    return HContactFromUIN(dwUIN, Added);

  if (Added) *Added = 0;

  if (!gbAimEnabled) return INVALID_HANDLE_VALUE;

  hContact = ICQFindFirstContact();
  while (hContact != NULL)
  {
    if (!ICQGetContactSettingUID(hContact, &dwUin, &szUid))
    {
      if (!dwUin && !stricmp(szUid, pszUID))
      {
        if (strcmpnull(szUid, pszUID))
        { // fix case in SN
          ICQWriteContactSettingString(hContact, UNIQUEIDSETTING, pszUID);
        }
        return hContact;
      }
    }
    hContact = ICQFindNextContact(hContact);
  }

  //not present: add
  if (Added)
  {
    hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
    CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)gpszICQProtoName);

    ICQWriteContactSettingString(hContact, UNIQUEIDSETTING, pszUID);

    if (!bIsSyncingCL)
    {
      DBWriteContactSettingByte(hContact, "CList", "NotOnList", 1);
      SetContactHidden(hContact, 1);

      ICQWriteContactSettingWord(hContact, "Status", ID_STATUS_OFFLINE);

      if (icqOnline)
      {
        icq_sendNewContact(0, pszUID);
      }
    }
    *Added = 1;

    return hContact;
  }

  return INVALID_HANDLE_VALUE;
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
    return ICQTranslateUtf("<invalid>");

  if (gbUnicodeCore)
    return make_utf8_string((wchar_t*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_UNICODE));
  else
  {
    unsigned char *utf = NULL;

    utf8_encode((char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0), &utf);
    return utf;
  }
}



char *strUID(DWORD dwUIN, char *pszUID)
{
  if (dwUIN)
    ltoa(dwUIN, pszUID, 10);

  return pszUID;
}



void SetContactHidden(HANDLE hContact, BYTE bHidden)
{
  DBWriteContactSettingByte(hContact, "CList", "Hidden", bHidden);

  if (!bHidden) // clear zero setting
    DBDeleteContactSetting(hContact, "CList", "Hidden");
}



/* a strlennull() that likes NULL */
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

  ZeroMemory(buffer, count);
  va_start(va, fmt);
  len = _vsnprintf(buffer, count-1, fmt, va);
  va_end(va);
  return len;
}



char* __fastcall null_strdup(const char *string)
{
  if (string)
    return strdup(string);

  return NULL;
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
    else if (!strnicmp(string+i, "&amp;", 5))
    {
      *szChar = '&';
      szChar++;
      i += 4;
    }
    else if (!strnicmp(string+i, "&quot;", 6))
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
      if ((i + 4 <= len) && (!strnicmp(string + i, "<br>", 4) || !strnicmp(string + i, "<br/>", 5)))
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
  SAFE_FREE(&tmp);

  return res;
}



char* ApplyEncoding(const char *string, const char* pszEncoding)
{ // decode encoding to Utf-8
  if (string && pszEncoding)
  { // we do only encodings known to icq5.1 // TODO: check if this is enough
    if (!strnicmp(pszEncoding, "utf-8", 5))
    { // it is utf-8 encoded
      return null_strdup(string);
    }
    else if (!strnicmp(pszEncoding, "unicode-2-0", 11))
    { // it is UCS-2 encoded
      int wLen = wcslen((wchar_t*)string) + 1;
      wchar_t *szStr = (wchar_t*)_alloca(wLen*2);
      char *tmp = (char*)string;

      unpackWideString(&tmp, szStr, (WORD)(wLen*2));

      return make_utf8_string(szStr);
    }
    else if (!strnicmp(pszEncoding, "iso-8859-1", 10))
    { // we use "Latin I" instead - it does the job
      char *szRes = ansi_to_utf8_codepage(string, 1252);
      
      return szRes;
    }
  }
  if (string)
  { // consider it CP_ACP
    char *szRes = ansi_to_utf8(string);

    return szRes;
  }

  return NULL;
}



void ResetSettingsOnListReload()
{
  HANDLE hContact;

  // Reset a bunch of session specific settings
  ICQWriteContactSettingWord(NULL, "SrvVisibilityID", 0);
  ICQWriteContactSettingWord(NULL, "SrvAvatarID", 0);
  ICQWriteContactSettingWord(NULL, "SrvPhotoID", 0);
  ICQWriteContactSettingWord(NULL, "SrvRecordCount", 0);

  hContact = ICQFindFirstContact();

  while (hContact)
  {
    // All these values will be restored during the serv-list receive
    ICQWriteContactSettingWord(hContact, "ServerId", 0);
    ICQWriteContactSettingWord(hContact, "SrvGroupId", 0);
    ICQWriteContactSettingWord(hContact, "SrvPermitId", 0);
    ICQWriteContactSettingWord(hContact, "SrvDenyId", 0);
    ICQWriteContactSettingByte(hContact, "Auth", 0);

    hContact = ICQFindNextContact(hContact);
  }

  FlushSrvGroupsCache();
}



void ResetSettingsOnConnect()
{
  HANDLE hContact;

  // Reset a bunch of session specific settings
  ICQWriteContactSettingByte(NULL, "SrvVisibility", 0);
  ICQWriteContactSettingDword(NULL, "IdleTS", 0);

  hContact = ICQFindFirstContact();

  while (hContact)
  {
    ICQWriteContactSettingDword(hContact, "LogonTS", 0);
    ICQWriteContactSettingDword(hContact, "IdleTS", 0);
    ICQWriteContactSettingDword(hContact, "TickTS", 0);
    ICQWriteContactSettingByte(hContact, "TemporaryVisible", 0);

    // All these values will be restored during the login
    if (ICQGetContactStatus(hContact) != ID_STATUS_OFFLINE)
      ICQWriteContactSettingWord(hContact, "Status", ID_STATUS_OFFLINE);

    hContact = ICQFindNextContact(hContact);
  }
}



void ResetSettingsOnLoad()
{
  HANDLE hContact;

  ICQWriteContactSettingDword(NULL, "IdleTS", 0);
  ICQWriteContactSettingDword(NULL, "LogonTS", 0);

  hContact = ICQFindFirstContact();

  while (hContact)
  {
    ICQWriteContactSettingDword(hContact, "LogonTS", 0);
    ICQWriteContactSettingDword(hContact, "IdleTS", 0);
    ICQWriteContactSettingDword(hContact, "TickTS", 0);
    if (ICQGetContactStatus(hContact) != ID_STATUS_OFFLINE)
    {
      ICQWriteContactSettingWord(hContact, "Status", ID_STATUS_OFFLINE);

      ICQDeleteContactSetting(hContact, DBSETTING_XSTATUSID);
      ICQDeleteContactSetting(hContact, DBSETTING_XSTATUSNAME);
      ICQDeleteContactSetting(hContact, DBSETTING_XSTATUSMSG);
    }
    ICQWriteContactSettingByte(hContact, "DCStatus", 0);

    hContact = ICQFindNextContact(hContact);
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
  ICQBroadcastAck(pArguments->hContact, pArguments->nAckType, pArguments->nAckResult, pArguments->hSequence, pArguments->pszMessage);

  if (pArguments->nAckResult == ACKRESULT_SUCCESS)
    NetLog_Server("Sent fake message ack");
  else if (pArguments->nAckResult == ACKRESULT_FAILED)
    NetLog_Server("Message delivery failed");

  SAFE_FREE((char **)&pArguments->pszMessage);
  SAFE_FREE(&pArguments);

  return 0;
}



void icq_SendProtoAck(HANDLE hContact, DWORD dwCookie, int nAckResult, int nAckType, char* pszMessage)
{
  icq_ack_args* pArgs;


  pArgs = (icq_ack_args*)SAFE_MALLOC(sizeof(icq_ack_args)); // This will be freed in the new thread

  pArgs->hContact = hContact;
  pArgs->hSequence = (HANDLE)dwCookie;
  pArgs->nAckResult = nAckResult;
  pArgs->nAckType = nAckType;
  pArgs->pszMessage = (LPARAM)null_strdup(pszMessage);

  ICQCreateThread(icq_ProtocolAckThread, pArgs);
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
  {
    WORD wCp = ICQGetContactSettingWord(hContact, "InfoCodePage", ICQGetContactSettingWord(hContact, "InfoCP", CP_ACP));

    if (wCp != CP_ACP)
    {
      char *szUtf = ansi_to_utf8_codepage(*buf, wCp);

      if (szUtf)
      {
        ICQWriteContactSettingUtf(hContact, szSetting, szUtf);
        SAFE_FREE(&szUtf);
      }
      else
        ICQWriteContactSettingString(hContact, szSetting, *buf);
    }
    else
      ICQWriteContactSettingString(hContact, szSetting, *buf);
  }
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
  char sbuf[MAX_PATH];
  char *text;

  if (*pwLength < 2)
    return FALSE;

  unpackLEWord(buf, &wVal);
  *pwLength -= 2;

  text = LookupFieldNameUtf(table, wVal, sbuf);
  if (text)
    ICQWriteContactSettingUtf(hContact, szSetting, text);
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
  char sbuf[MAX_PATH];
  char *text;

  if (*pwLength < 1)
    return FALSE;

  unpackByte(buf, &byVal);
  *pwLength -= 1;

  text = LookupFieldNameUtf(table, byVal, sbuf);
  if (text)
    ICQWriteContactSettingUtf(hContact, szSetting, text);
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
      WORD wStatus = ICQGetContactStatus(hContact);
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



void* __fastcall SAFE_MALLOC(size_t size)
{
  void* p = malloc(size);

  if (p)
    ZeroMemory(p, size);

  return p;
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
          ICQFreeVariant(&dbv);
        }
        else
        { // some file already defined, check if it is not the same, if yes, set link
          if (!strcmpnull(dbv.pszVal, szFile))
          {
            DBWriteContactSettingByte(hContact, "ContactPhoto", "ICQLink", 1);
          }
          ICQFreeVariant(&dbv);
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

  if (!bPhotoLock && ICQGetContactSettingByte(NULL, "AvatarsAutoLink", DEFAULT_LINK_AVATARS))
    DBDeleteContactSetting(hContact, "ContactPhoto", "ICQLink");

  bNoChanging = 0;
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



HANDLE NetLib_BindPort(NETLIBNEWCONNECTIONPROC_V2 pFunc, void* lParam, WORD* pwPort, DWORD* pdwIntIP)
{
  NETLIBBIND nlb = {0};
  HANDLE hBoundPort;

  nlb.cbSize = sizeof(NETLIBBIND); 
  nlb.pfnNewConnectionV2 = pFunc;
  nlb.pExtra = lParam;
  SetLastError(ERROR_INVALID_PARAMETER); // this must be here - NetLib does not set any error :((
  hBoundPort = (HANDLE)CallService(MS_NETLIB_BINDPORT, (WPARAM)ghDirectNetlibUser, (LPARAM)&nlb);
  if (!hBoundPort && (GetLastError() == ERROR_INVALID_PARAMETER))
  { // this ensures older Miranda also can bind a port for a dc - pre 0.6
    nlb.cbSize = NETLIBBIND_SIZEOF_V2;
    hBoundPort = (HANDLE)CallService(MS_NETLIB_BINDPORT, (WPARAM)ghDirectNetlibUser, (LPARAM)&nlb);
  }
  if (pwPort) *pwPort = nlb.wPort;
  if (pdwIntIP) *pdwIntIP = nlb.dwInternalIP;

  return hBoundPort;
}



void NetLib_SafeCloseHandle(HANDLE *hConnection, int bServerConn)
{
  if (*hConnection)
  {
    Netlib_CloseHandle(*hConnection);
    if (bServerConn)
      FreeGatewayIndex(*hConnection);
    *hConnection = NULL;
  }
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



int NetLog_Uni(BOOL bDC, const char *fmt,...)
{
  va_list va; 
  char szText[1024];
  HANDLE hNetlib;

  va_start(va,fmt);
  mir_vsnprintf(szText,sizeof(szText),fmt,va);
  va_end(va);

  if (bDC)
    hNetlib = ghDirectNetlibUser;
  else
    hNetlib = ghServerNetlibUser;

  return CallService(MS_NETLIB_LOG,(WPARAM)hNetlib,(LPARAM)szText);
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



char* __fastcall ICQTranslate(const char* src)
{
  return (char*)CallService(MS_LANGPACK_TRANSLATESTRING,0,(LPARAM)src);
}



char* __fastcall ICQTranslateUtf(const char* src)
{ // this takes UTF-8 strings only!!!
  char* szRes = NULL;

  if (!strlennull(src))
  { // for the case of empty strings
    return null_strdup(src);
  }

  if (gbUtfLangpack)
  { // we can use unicode translate
    wchar_t* usrc = make_unicode_string(src);

    szRes = make_utf8_string(TranslateW(usrc));

    SAFE_FREE(&usrc);
  }
  else
  {
    int size = strlennull(src)+2;
    char* asrc = (char*)_alloca(size);

    utf8_decode_static(src, asrc, size);
    utf8_encode(Translate(asrc), &szRes);
  }
  return szRes;
}



char* __fastcall ICQTranslateUtfStatic(const char* src, char* buf)
{ // this takes UTF-8 strings only!!!
  char* t;
  
  if (strlennull(src))
  {
    t = ICQTranslateUtf(src);
    strcpy(buf, t);
    SAFE_FREE(&t);
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
  params.threadID   = &dwThreadId;
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



char* GetUserPassword(BOOL bAlways)
{
  if (gpszPassword[0] != '\0' && (gbRememberPwd || bAlways))
    return gpszPassword;

  if (!ICQGetContactStaticString(NULL, "Password", gpszPassword, sizeof(gpszPassword)))
  {
    CallService(MS_DB_CRYPT_DECODESTRING, strlennull(gpszPassword) + 1, (LPARAM)gpszPassword);

    if (!strlennull(gpszPassword)) return NULL;

    gbRememberPwd = TRUE;

    return gpszPassword;
  }

  return NULL;
}



WORD GetMyStatusFlags()
{
  WORD wFlags = 0;

  // Webaware setting bit flag
  if (ICQGetContactSettingByte(NULL, "WebAware", 0))
    wFlags = STATUS_WEBAWARE;

  // DC setting bit flag
  switch (ICQGetContactSettingByte(NULL, "DCType", 0))
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



char* FileNameToUtf(const char *filename)
{
  if (gbUnicodeAPI)
  { // reasonable only on NT systems
    HINSTANCE hKernel;
    DWORD (CALLBACK *RealGetLongPathName)(LPCWSTR, LPWSTR, DWORD);

    hKernel = GetModuleHandle("KERNEL32");
    *(FARPROC *)&RealGetLongPathName = GetProcAddress(hKernel, "GetLongPathNameW");

    if (RealGetLongPathName)
    { // the function is available (it is not on old NT systems)
      wchar_t *unicode, *usFileName = NULL;
      int wchars;

      wchars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, filename,
        strlennull(filename), NULL, 0);

      unicode = (wchar_t*)_alloca((wchars + 1) * sizeof(wchar_t));
      unicode[wchars] = 0;

      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, filename,
        strlennull(filename), unicode, wchars);

      wchars = RealGetLongPathName(unicode, usFileName, 0);
      usFileName = (wchar_t*)_alloca((wchars + 1) * sizeof(wchar_t));
      RealGetLongPathName(unicode, usFileName, wchars);

      return make_utf8_string(usFileName);
    }
    else
      return ansi_to_utf8(filename);
  }
  else
    return ansi_to_utf8(filename);
}



int FileStatUtf(const char *path, struct _stati64 *buffer)
{
  int wRes = -1;

  if (gbUnicodeAPI)
  {
    wchar_t* usPath = make_unicode_string(path);

    wRes = _wstati64(usPath, buffer);
    SAFE_FREE(&usPath);
  }
  else
  {
    int size = strlennull(path)+2;
    char* szAnsiPath = (char*)_alloca(size);

    if (utf8_decode_static(path, szAnsiPath, size))
      wRes = _stati64(szAnsiPath, buffer);
  }
  return wRes;
}



int MakeDirUtf(const char *dir)
{
  int wRes = -1;
  char *szLast;

  if (gbUnicodeAPI)
  {
    wchar_t* usDir = make_unicode_string(dir);

    wRes = _wmkdir(usDir);
    if (wRes)
    {
      szLast = strrchr(dir, '\\');
      if (!szLast) szLast = strrchr(dir, '/');
      if (szLast)
      {
        char cOld = *szLast;

        *szLast = '\0';
        if (!MakeDirUtf(dir))
          wRes = _wmkdir(usDir);
        *szLast = cOld;
      }
    }
    SAFE_FREE(&usDir);
  }
  else
  {
    int size = strlennull(dir)+2;
    char* szAnsiDir = (char*)_alloca(size);

    if (utf8_decode_static(dir, szAnsiDir, size))
    { // _mkdir can created only one dir at once
      wRes = _mkdir(szAnsiDir);
      if (wRes)
      { // failed, try one directory less first
        szLast = strrchr(dir, '\\');
        if (!szLast) szLast = strrchr(dir, '/');
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
  }
  return wRes;
}



int OpenFileUtf(const char *filename, int oflag, int pmode)
{
  int hFile = -1;

  if (gbUnicodeAPI)
  {
    wchar_t* usFile = make_unicode_string(filename);

    hFile = _wopen(usFile, oflag, pmode);
    SAFE_FREE(&usFile);
  }
  else
  {
    int size = strlennull(filename)+2;
    char* szAnsiFile = (char*)_alloca(size);

    if (utf8_decode_static(filename, szAnsiFile, size))
      hFile = _open(szAnsiFile, oflag, pmode); 
  }
  return hFile;
}



wchar_t *GetWindowTextUcs(HWND hWnd)
{
  wchar_t *utext;

  if (gbUnicodeAPI)
  {
    int nLen = GetWindowTextLengthW(hWnd);

    utext = (wchar_t*)SAFE_MALLOC((nLen+2)*sizeof(wchar_t));
    GetWindowTextW(hWnd, utext, nLen + 1);
  }
  else
  {
    char *text;
    int wchars, nLen = GetWindowTextLengthA(hWnd);

    text = (char*)_alloca(nLen+2);
    GetWindowTextA(hWnd, text, nLen + 1);

    wchars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, text,
      strlennull(text), NULL, 0);

    utext = (wchar_t*)SAFE_MALLOC((wchars + 1)*sizeof(unsigned short));

    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, text,
      strlennull(text), utext, wchars);
  }
  return utext;
}



void SetWindowTextUcs(HWND hWnd, wchar_t *text)
{
  if (gbUnicodeAPI)
  {
    SetWindowTextW(hWnd, text);
  }
  else
  {
    char *tmp = (char*)SAFE_MALLOC(wcslen(text) + 1);

    WideCharToMultiByte(CP_ACP, 0, text, -1, tmp, wcslen(text)+1, NULL, NULL);
    SetWindowTextA(hWnd, tmp);
    SAFE_FREE(&tmp);
  }
}



char* GetWindowTextUtf(HWND hWnd)
{
  if (gbUnicodeAPI)
  {
    wchar_t* usText;
    int nLen = GetWindowTextLengthW(hWnd);

    usText = (wchar_t*)_alloca((nLen+2)*sizeof(wchar_t));
    GetWindowTextW(hWnd, usText, nLen + 1);
    return make_utf8_string(usText);
  }
  else
  {
    char* szAnsi;
    int nLen = GetWindowTextLengthA(hWnd);

    szAnsi = (char*)_alloca(nLen+2);
    GetWindowTextA(hWnd, szAnsi, nLen + 1);
    return ansi_to_utf8(szAnsi);
  }
}



char* GetDlgItemTextUtf(HWND hwndDlg, int iItem)
{
  return GetWindowTextUtf(GetDlgItem(hwndDlg, iItem));
}



void SetWindowTextUtf(HWND hWnd, const char* szText)
{
  if (gbUnicodeAPI)
  {
    wchar_t* usText = make_unicode_string(szText);

    SetWindowTextW(hWnd, usText);
    SAFE_FREE(&usText);
  }
  else
  {
    int size = strlennull(szText)+2;
    char* szAnsi = (char*)_alloca(size);

    if (utf8_decode_static(szText, szAnsi, size))
      SetWindowTextA(hWnd, szAnsi);
  }
}



void SetDlgItemTextUtf(HWND hwndDlg, int iItem, const char* szText)
{
  SetWindowTextUtf(GetDlgItem(hwndDlg, iItem), szText);
}



LONG SetWindowLongUtf(HWND hWnd, int nIndex, LONG dwNewLong)
{
  if (gbUnicodeAPI)
    return SetWindowLongW(hWnd, nIndex, dwNewLong);
  else
    return SetWindowLongA(hWnd, nIndex, dwNewLong);
}



LRESULT CallWindowProcUtf(WNDPROC OldProc, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (gbUnicodeAPI)
    return CallWindowProcW(OldProc,hWnd,msg,wParam,lParam);
  else
    return CallWindowProcA(OldProc,hWnd,msg,wParam,lParam);
}



static int ControlAddStringUtf(HWND ctrl, DWORD msg, const char* szString)
{
  char str[MAX_PATH];
  char *szItem = ICQTranslateUtfStatic(szString, str);
  int item;

  if (gbUnicodeAPI)
  {
    wchar_t *wItem = make_unicode_string(szItem);

    item = SendMessageW(ctrl, msg, 0, (LPARAM)wItem);
    SAFE_FREE(&wItem);
  }
  else
  {
    int size = strlennull(szItem) + 2;
    char *aItem = (char*)_alloca(size);

    utf8_decode_static(szItem, aItem, size);
    item = SendMessageA(ctrl, msg, 0, (LPARAM)aItem);
  }
  return item;
}



int ComboBoxAddStringUtf(HWND hCombo, const char* szString, DWORD data)
{
  int item = ControlAddStringUtf(hCombo, CB_ADDSTRING, szString);
  SendMessage(hCombo, CB_SETITEMDATA, item, data);

  return item;
}



int ListBoxAddStringUtf(HWND hList, const char* szString)
{
  return ControlAddStringUtf(hList, LB_ADDSTRING, szString);
}



HWND DialogBoxUtf(BOOL bModal, HINSTANCE hInstance, const char* szTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{ // Unicode pump ready dialog box
  if (gbUnicodeAPI)
  {
    if (bModal)
      return (HANDLE)DialogBoxParamW(hInstance, (LPCWSTR)szTemplate, hWndParent, lpDialogFunc, dwInitParam);
    else
      return CreateDialogParamW(hInstance, (LPCWSTR)szTemplate, hWndParent, lpDialogFunc, dwInitParam);
  }
  else
  {
    if (bModal)
      return (HANDLE)DialogBoxParamA(hInstance, szTemplate, hWndParent, lpDialogFunc, dwInitParam);
    else
      return CreateDialogParamA(hInstance, szTemplate, hWndParent, lpDialogFunc, dwInitParam);
  }
}



HWND CreateDialogUtf(HINSTANCE hInstance, const char* lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc)
{
  if (gbUnicodeAPI)
    return CreateDialogW(hInstance, (LPCWSTR)lpTemplate, hWndParent, lpDialogFunc);
  else
    return CreateDialogA(hInstance, lpTemplate, hWndParent, lpDialogFunc);
}



int MessageBoxUtf(HWND hWnd, const char* szText, const char* szCaption, UINT uType)
{
  int res;
  char str[1024];
  char cap[MAX_PATH];

  if (gbUnicodeAPI)
  {
    wchar_t *text = make_unicode_string(ICQTranslateUtfStatic(szText, str));
    wchar_t *caption = make_unicode_string(ICQTranslateUtfStatic(szCaption, cap));
    res = MessageBoxW(hWnd, text, caption, uType);
    SAFE_FREE(&caption);
    SAFE_FREE(&text);
  }
  else
  {
    int size = strlennull(szText) + 2, size2 = strlennull(szCaption) + 2;
    char *text = (char*)_alloca(size);
    char *caption = (char*)_alloca(size2);

    utf8_decode_static(ICQTranslateUtfStatic(szText, str), text, size);
    utf8_decode_static(ICQTranslateUtfStatic(szCaption, cap), caption, size2);
    res = MessageBoxA(hWnd, text, caption, uType);
  }
  return res;
}
