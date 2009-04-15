// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001-2002 Jon Keating, Richard Hughes
// Copyright © 2002-2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004-2009 Joe Kucera
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

void CIcqProto::CreateResidentSetting(const char* szSetting)
{
	char pszSetting[2*MAX_PATH];

	strcpy(pszSetting, m_szModuleName);
	strcat(pszSetting, "/");
	strcat(pszSetting, szSetting);
	CallService(MS_DB_SETSETTINGRESIDENT, 1, (WPARAM)pszSetting);
}

int CIcqProto::getSetting(HANDLE hContact, const char* szSetting, DBVARIANT *dbv)
{
  return DBGetContactSettingW(hContact, m_szModuleName, szSetting, dbv);
}

BYTE CIcqProto::getSettingByte(HANDLE hContact, const char* szSetting, BYTE bDef)
{
  return DBGetContactSettingByte(hContact, m_szModuleName, szSetting, bDef);
}

WORD CIcqProto::getSettingWord(HANDLE hContact, const char* szSetting, WORD wDef)
{
  return DBGetContactSettingWord(hContact, m_szModuleName, szSetting, wDef);
}

DWORD CIcqProto::getSettingDword(HANDLE hContact, const char* szSetting, DWORD dwDef)
{
  DBVARIANT dbv = {0};
	DBCONTACTGETSETTING cgs;
	DWORD dwRes;

	cgs.szModule = m_szModuleName;
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

double CIcqProto::getSettingDouble(HANDLE hContact, const char *szSetting, double dDef)
{
  DBVARIANT dbv = {0};
  double dRes;

  if (getSetting(hContact, szSetting, &dbv))
    return dDef; // not found, give default

  if (dbv.type != DBVT_BLOB || dbv.cpbVal != sizeof(double))
    dRes = dDef;
  else
    dRes = *(double*)dbv.pbVal;

  ICQFreeVariant(&dbv);
  return dRes;
}

DWORD CIcqProto::getContactUin(HANDLE hContact)
{
	return getSettingDword(hContact, UNIQUEIDSETTING, 0);
}

int CIcqProto::getContactUid(HANDLE hContact, DWORD *pdwUin, uid_str* ppszUid)
{
	DBVARIANT dbv;
	int iRes = 1;

	*pdwUin = 0;
	if (ppszUid) *ppszUid[0] = '\0';

	if (!getSetting(hContact, UNIQUEIDSETTING, &dbv))
	{
		if (dbv.type == DBVT_DWORD)
		{
			*pdwUin = dbv.dVal;
			iRes = 0;
		}
		else if (dbv.type == DBVT_ASCIIZ)
		{
			if (ppszUid && m_bAimEnabled) 
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

int CIcqProto::getSettingString(HANDLE hContact, const char* szSetting, DBVARIANT *dbv)
{
	return DBGetContactSettingString(hContact, m_szModuleName, szSetting, dbv);
}

char* CIcqProto::getSettingStringUtf(HANDLE hContact, const char *szModule, const char *szSetting, char *szDef)
{
	DBVARIANT dbv = {DBVT_DELETED};
	if (DBGetContactSettingUTF8String(hContact, szModule, szSetting, &dbv))
		return null_strdup(szDef);

	char *szRes = null_strdup(dbv.pszVal);
	ICQFreeVariant(&dbv);
	return szRes;
}

char* CIcqProto::getSettingStringUtf(HANDLE hContact, const char *szSetting, char *szDef)
{
	return getSettingStringUtf(hContact, m_szModuleName, szSetting, szDef);
}

WORD CIcqProto::getContactStatus(HANDLE hContact)
{
  return getSettingWord(hContact, "Status", ID_STATUS_OFFLINE);
}

int CIcqProto::getSettingStringStatic(HANDLE hContact, const char* valueName, char* dest, int dest_len)
{
	DBVARIANT dbv;
	DBCONTACTGETSETTING sVal;

	dbv.pszVal = dest;
	dbv.cchVal = dest_len;
	dbv.type = DBVT_ASCIIZ;

	sVal.pValue = &dbv;
	sVal.szModule = m_szModuleName;
	sVal.szSetting = valueName;

	if (CallService(MS_DB_CONTACT_GETSETTINGSTATIC, (WPARAM)hContact, (LPARAM)&sVal) != 0)
	{
		dbv.pszVal = dest;
		dbv.cchVal = dest_len;
		dbv.type = DBVT_UTF8;

		if (CallService(MS_DB_CONTACT_GETSETTINGSTATIC, (WPARAM)hContact, (LPARAM)&sVal) != 0)
			return 1; // this is here due to DB module bug...
	}

	return (dbv.type != DBVT_ASCIIZ); /// FIXME:
}

int CIcqProto::deleteSetting(HANDLE hContact, const char* szSetting)
{
  return DBDeleteContactSetting(hContact, m_szModuleName, szSetting);
}

int CIcqProto::setSettingByte(HANDLE hContact, const char* szSetting, BYTE bValue)
{
	return DBWriteContactSettingByte(hContact, m_szModuleName, szSetting, bValue);
}

int CIcqProto::setSettingWord(HANDLE hContact, const char* szSetting, WORD wValue)
{
	return DBWriteContactSettingWord(hContact, m_szModuleName, szSetting, wValue);
}

int CIcqProto::setSettingDword(HANDLE hContact, const char* szSetting, DWORD dwValue)
{
	return DBWriteContactSettingDword(hContact, m_szModuleName, szSetting, dwValue);
}

int CIcqProto::setSettingDouble(HANDLE hContact, const char *szSetting, double dValue)
{
  return setSettingBlob(hContact, szSetting, (BYTE*)&dValue, sizeof(double));
}

int CIcqProto::setSettingString(HANDLE hContact, const char* szSetting, const char* szValue)
{
	return DBWriteContactSettingString(hContact, m_szModuleName, szSetting, szValue);
}

int CIcqProto::setSettingStringUtf(HANDLE hContact, const char *szModule, const char* szSetting, const char* szValue)
{
	return DBWriteContactSettingUTF8String(hContact, szModule, szSetting, (char*)szValue);
}

int CIcqProto::setSettingStringUtf(HANDLE hContact, const char* szSetting, const char* szValue)
{
	return setSettingStringUtf(hContact, m_szModuleName, szSetting, szValue);
}

int CIcqProto::setSettingBlob(HANDLE hContact, const char *szSetting, const BYTE *val, const int cbVal)
{
	DBCONTACTWRITESETTING cws;

	cws.szModule = m_szModuleName;
	cws.szSetting = szSetting;
	cws.value.type = DBVT_BLOB;
	cws.value.pbVal = (LPBYTE)val;
	cws.value.cpbVal = cbVal;
	return CallService(MS_DB_CONTACT_WRITESETTING, (WPARAM)hContact, (LPARAM)&cws);
}

int CIcqProto::setContactHidden(HANDLE hContact, BYTE bHidden)
{
  int nResult = DBWriteContactSettingByte(hContact, "CList", "Hidden", bHidden);

	if (!bHidden) // clear zero setting
		DBDeleteContactSetting(hContact, "CList", "Hidden");

  return nResult;
}


int __fastcall ICQFreeVariant(DBVARIANT* dbv)
{
	return DBFreeVariant(dbv);
}

int CIcqProto::IsICQContact(HANDLE hContact)
{
	char* szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);

	return !strcmpnull(szProto, m_szModuleName);
}

HANDLE CIcqProto::AddEvent(HANDLE hContact, WORD wType, DWORD dwTime, DWORD flags, DWORD cbBlob, PBYTE pBlob)
{
	DBEVENTINFO dbei = {0};

	dbei.cbSize = sizeof(dbei);
	dbei.szModule = m_szModuleName;
	dbei.timestamp = dwTime;
	dbei.flags = flags;
	dbei.eventType = wType;
	dbei.cbBlob = cbBlob;
	dbei.pBlob = pBlob;

	return (HANDLE)CallService(MS_DB_EVENT_ADD, (WPARAM)hContact, (LPARAM)&dbei);
}

HANDLE CIcqProto::FindFirstContact()
{
	HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, (LPARAM)m_szModuleName);

	if (IsICQContact(hContact))
	{
		return hContact;
	}
	return FindNextContact(hContact);
}

HANDLE CIcqProto::FindNextContact(HANDLE hContact)
{
	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,(LPARAM)m_szModuleName);

	while (hContact != NULL)
	{
		if (IsICQContact(hContact))
		{
			return hContact;
		}
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,(LPARAM)m_szModuleName);
	}
	return hContact;
}

char* CIcqProto::getContactCListGroup(HANDLE hContact)
{
	return getSettingStringUtf(hContact, "CList", "Group", NULL);
}

int __stdcall ICQSetContactCListGroup(HANDLE hContact, const unsigned char *szGroup)
{
	/// TODO
	return 0;
}
