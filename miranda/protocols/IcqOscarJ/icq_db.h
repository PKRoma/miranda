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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/icq_db.h,v $
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#ifndef __ICQ_DB_H
#define __ICQ_DB_H

void InitDB();

void __stdcall ICQCreateResidentSetting(const char* szSetting);

BYTE __stdcall ICQGetContactSettingByte(HANDLE hContact, const char* szSetting, BYTE bDef);
WORD __stdcall ICQGetContactSettingWord(HANDLE hContact, const char* szSetting, WORD wDef);
DWORD __stdcall ICQGetContactSettingDword(HANDLE hContact, const char* szSetting, DWORD dwDef);
DWORD __stdcall ICQGetContactSettingUIN(HANDLE hContact);
int __stdcall ICQGetContactSettingUID(HANDLE hContact, DWORD *pdwUin, uid_str* ppszUid);
int __stdcall ICQGetContactSetting(HANDLE hContact, const char* szSetting, DBVARIANT *dbv);
char* __stdcall ICQGetContactSettingUtf(HANDLE hContact, const char* szSetting, char* szDef);

WORD __stdcall ICQGetContactStatus(HANDLE hContact);

int __stdcall ICQGetContactStaticString(HANDLE hContact, const char* valueName, char* dest, int dest_len);

int __stdcall ICQDeleteContactSetting(HANDLE hContact, const char* szSetting);

int __stdcall ICQWriteContactSettingByte(HANDLE hContact, const char* szSetting, BYTE bValue);
int __stdcall ICQWriteContactSettingWord(HANDLE hContact, const char* szSetting, WORD wValue);
int __stdcall ICQWriteContactSettingDword(HANDLE hContact, const char* szSetting, DWORD dwValue);
int __stdcall ICQWriteContactSettingString(HANDLE hContact, const char* szSetting, char* szValue);
int __stdcall ICQWriteContactSettingUtf(HANDLE hContact, const char* szSetting, char* szValue);

int __stdcall ICQWriteContactSettingBlob(HANDLE hContact,const char *szSetting,const char *val, const int cbVal);

char* __stdcall UniGetContactSettingUtf(HANDLE hContact, const char *szModule,const char* szSetting, char* szDef);
int __stdcall UniWriteContactSettingUtf(HANDLE hContact, const char *szModule,const char* szSetting, char* szValue);

int __stdcall ICQFreeVariant(DBVARIANT* dbv);

HANDLE __stdcall ICQAddEvent(HANDLE hContact, WORD wType, DWORD dwTime, DWORD flags, DWORD cbBlob, PBYTE pBlob);

int __fastcall IsICQContact(HANDLE hContact);
HANDLE __fastcall ICQFindFirstContact();
HANDLE __fastcall ICQFindNextContact(HANDLE hContact);

#endif /* __ICQ_DB_H */
