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
//  Internal Database API
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


static BOOL bUtfReadyDB = FALSE;

void InitDB()
{
  bUtfReadyDB = ServiceExists(MS_DB_CONTACT_GETSETTING_STR);
  if (!bUtfReadyDB)
    NetLog_Server("Warning: DB module does not support Unicode.");
}



void __stdcall ICQCreateResidentSetting(const char* szSetting)
{
  char pszSetting[2*MAX_PATH];

  strcpy(pszSetting, gpszICQProtoName);
  strcat(pszSetting, "/");
  strcat(pszSetting, szSetting);
  CallService(MS_DB_SETSETTINGRESIDENT, 1, (WPARAM)pszSetting);
}



BYTE __stdcall ICQGetContactSettingByte(HANDLE hContact, const char* szSetting, BYTE bDef)
{
  return DBGetContactSettingByte(hContact, gpszICQProtoName, szSetting, bDef);
}



WORD __stdcall ICQGetContactSettingWord(HANDLE hContact, const char* szSetting, WORD wDef)
{
  return DBGetContactSettingWord(hContact, gpszICQProtoName, szSetting, wDef);
}



DWORD __stdcall ICQGetContactSettingDword(HANDLE hContact, const char* szSetting, DWORD dwDef)
{
  DBVARIANT dbv;
  DBCONTACTGETSETTING cgs;
  DWORD dwRes;

  cgs.szModule = gpszICQProtoName;
  cgs.szSetting = szSetting;
  cgs.pValue = &dbv;
  if (CallService(MS_DB_CONTACT_GETSETTING,(WPARAM)hContact,(LPARAM)&cgs))
    return dwDef; // not found, give default

  if (dbv.type != DBVT_DWORD)
    dwRes = dwDef; // invalid type, give default
  else // found and valid, give result
    dwRes = dbv.dVal;

  ICQFreeVariant(&dbv);
	return dwRes;
}



DWORD __stdcall ICQGetContactSettingUIN(HANDLE hContact)
{
  return ICQGetContactSettingDword(hContact, UNIQUEIDSETTING, 0);
}



int __stdcall ICQGetContactSettingUID(HANDLE hContact, DWORD *pdwUin, uid_str* ppszUid)
{
  DBVARIANT dbv;
  int iRes = 1;

  *pdwUin = 0;
  if (ppszUid) *ppszUid[0] = '\0';

  if (!ICQGetContactSetting(hContact, UNIQUEIDSETTING, &dbv))
  {
    if (dbv.type == DBVT_DWORD)
    {
      *pdwUin = dbv.dVal;
      iRes = 0;
    }
    else if (dbv.type == DBVT_ASCIIZ)
    {
      if (ppszUid && gbAimEnabled) 
      {
        strcpy(*ppszUid, dbv.pszVal);
        iRes = 0;
      }
      else
        NetLog_Server("AOL screennames not accepted");
    }
    ICQFreeVariant(&dbv);
  }
  return iRes;
}



int __stdcall ICQGetContactSetting(HANDLE hContact, const char* szSetting, DBVARIANT *dbv)
{
  if (bUtfReadyDB)
    return DBGetContactSettingW(hContact, gpszICQProtoName, szSetting, dbv);
  else
    return DBGetContactSetting(hContact, gpszICQProtoName, szSetting, dbv);
}



int __stdcall ICQGetContactSettingString(HANDLE hContact, const char* szSetting, DBVARIANT *dbv)
{
  if (bUtfReadyDB)
    return DBGetContactSettingString(hContact, gpszICQProtoName, szSetting, dbv);
  else
    return ICQGetContactSetting(hContact, szSetting, dbv);
}



char* __stdcall UniGetContactSettingUtf(HANDLE hContact, const char *szModule,const char* szSetting, char* szDef)
{
  DBVARIANT dbv = {DBVT_DELETED};
  char* szRes;

  if (bUtfReadyDB)
  {
    if (DBGetContactSettingUTF8String(hContact, szModule, szSetting, &dbv))
      return null_strdup(szDef);
    
    szRes = null_strdup(dbv.pszVal);
    ICQFreeVariant(&dbv);
  }
  else
  { // old DB, we need to convert the string to UTF-8
    if (DBGetContactSetting(hContact, szModule, szSetting, &dbv))
      return null_strdup(szDef);

    szRes = ansi_to_utf8(dbv.pszVal);

    ICQFreeVariant(&dbv);
  }
  return szRes;
}



char* __stdcall ICQGetContactSettingUtf(HANDLE hContact, const char* szSetting, char* szDef)
{
  return UniGetContactSettingUtf(hContact, gpszICQProtoName, szSetting, szDef);
}



WORD __stdcall ICQGetContactStatus(HANDLE hContact)
{
  return ICQGetContactSettingWord(hContact, "Status", ID_STATUS_OFFLINE);
}



// (c) by George Hazan
int __stdcall ICQGetContactStaticString(HANDLE hContact, const char* valueName, char* dest, int dest_len)
{
  DBVARIANT dbv;
  DBCONTACTGETSETTING sVal;

  dbv.pszVal = dest;
  dbv.cchVal = dest_len;
  dbv.type = DBVT_ASCIIZ;

  sVal.pValue = &dbv;
  sVal.szModule = gpszICQProtoName;
  sVal.szSetting = valueName;

  if (CallService(MS_DB_CONTACT_GETSETTINGSTATIC, (WPARAM)hContact, (LPARAM)&sVal) != 0)
  {
    dbv.pszVal = dest;
    dbv.cchVal = dest_len;
    dbv.type = DBVT_UTF8;

    if (CallService(MS_DB_CONTACT_GETSETTINGSTATIC, (WPARAM)hContact, (LPARAM)&sVal) != 0)
      return 1; // this is here due to DB module bug...
  }

  return (dbv.type != DBVT_ASCIIZ);
}



int __stdcall ICQDeleteContactSetting(HANDLE hContact, const char* szSetting)
{
  return DBDeleteContactSetting(hContact, gpszICQProtoName, szSetting);
}



int __stdcall ICQWriteContactSettingByte(HANDLE hContact, const char* szSetting, BYTE bValue)
{
  return DBWriteContactSettingByte(hContact, gpszICQProtoName, szSetting, bValue);
}



int __stdcall ICQWriteContactSettingWord(HANDLE hContact, const char* szSetting, WORD wValue)
{
  return DBWriteContactSettingWord(hContact, gpszICQProtoName, szSetting, wValue);
}



int __stdcall ICQWriteContactSettingDword(HANDLE hContact, const char* szSetting, DWORD dwValue)
{
  return DBWriteContactSettingDword(hContact, gpszICQProtoName, szSetting, dwValue);
}



int __stdcall ICQWriteContactSettingString(HANDLE hContact, const char* szSetting, const char* szValue)
{
  return DBWriteContactSettingString(hContact, gpszICQProtoName, szSetting, szValue);
}



int __stdcall UniWriteContactSettingUtf(HANDLE hContact, const char *szModule, const char* szSetting, const char* szValue)
{
  if (bUtfReadyDB)
    return DBWriteContactSettingUTF8String(hContact, szModule, szSetting, szValue);
  else
  { // old DB, we need to convert the string to Ansi
    int size = strlennull(szValue) + 2;
    char* szAnsi = (char*)_alloca(size);

    if (utf8_decode_static(szValue, szAnsi, size))
      return DBWriteContactSettingString(hContact, szModule, szSetting, szAnsi);
    // failed to convert - give error

    return 1;
  }
}



int __stdcall ICQWriteContactSettingUtf(HANDLE hContact, const char* szSetting, const char* szValue)
{
  return UniWriteContactSettingUtf(hContact, gpszICQProtoName, szSetting, szValue);
}



int __stdcall ICQWriteContactSettingBlob(HANDLE hContact, const char *szSetting, const BYTE *val, const int cbVal)
{
  DBCONTACTWRITESETTING cws;

  cws.szModule = gpszICQProtoName;
  cws.szSetting = szSetting;
  cws.value.type = DBVT_BLOB;
  cws.value.pbVal = (LPBYTE)val;
  cws.value.cpbVal = cbVal;
  return CallService(MS_DB_CONTACT_WRITESETTING, (WPARAM)hContact, (LPARAM)&cws);
}



int __stdcall ICQFreeVariant(DBVARIANT* dbv)
{
  return DBFreeVariant(dbv);
}



int __fastcall IsICQContact(HANDLE hContact)
{
  char* szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);

  return !strcmpnull(szProto, gpszICQProtoName);
}



HANDLE __stdcall ICQAddEvent(HANDLE hContact, WORD wType, DWORD dwTime, DWORD flags, DWORD cbBlob, PBYTE pBlob)
{
  DBEVENTINFO dbei = {0};

  dbei.cbSize = sizeof(dbei);
  dbei.szModule = gpszICQProtoName;
  dbei.timestamp = dwTime;
  dbei.flags = flags;
  dbei.eventType = wType;
  dbei.cbBlob = cbBlob;
  dbei.pBlob = pBlob;

  return (HANDLE)CallService(MS_DB_EVENT_ADD, (WPARAM)hContact, (LPARAM)&dbei);
}



HANDLE __fastcall ICQFindFirstContact()
{
  HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, (LPARAM)gpszICQProtoName);

  if (IsICQContact(hContact))
  {
    return hContact;
  }
  return ICQFindNextContact(hContact);
}



HANDLE __fastcall ICQFindNextContact(HANDLE hContact)
{
  hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,(LPARAM)gpszICQProtoName);

  while (hContact != NULL)
  {
    if (IsICQContact(hContact))
    {
      return hContact;
    }
    hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,(LPARAM)gpszICQProtoName);
  }
  return hContact;
}



char* __stdcall ICQGetContactCListGroup(HANDLE hContact)
{
  return UniGetContactSettingUtf(hContact, "CList", "Group", NULL);
}



int __stdcall ICQSetContactCListGroup(HANDLE hContact, const char *szGroup)
{
  /// TODO
  return 0;
}
