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
//  Handles packet & message cookies
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"

#define INVALID_COOKIE_INDEX -1

void CIcqProto::RemoveExpiredCookies()
{
	time_t tNow = time(NULL);

	for (int i = cookies.getCount()-1; i >= 0; i--)
  {
    icq_cookie_info *cookie = cookies[i];

		if ((cookie->dwTime + COOKIE_TIMEOUT) < tNow)
    {
			cookies.remove(i);
      SAFE_FREE((void**)&cookie);
    }
  }
}

// Generate and allocate cookie
DWORD CIcqProto::AllocateCookie(BYTE bType, WORD wIdent, HANDLE hContact, void *pvExtra)
{
	EnterCriticalSection(&cookieMutex);

	DWORD dwThisSeq = wCookieSeq++;
	dwThisSeq &= 0x7FFF;
	dwThisSeq |= wIdent<<0x10;

	icq_cookie_info* p = (icq_cookie_info*)SAFE_MALLOC(sizeof(icq_cookie_info));
	p->bType = bType;
	p->dwCookie = dwThisSeq;
	p->hContact = hContact;
	p->pvExtra = pvExtra;
	p->dwTime = time(NULL);
	cookies.insert(p);
	
	LeaveCriticalSection(&cookieMutex);

	return dwThisSeq;
}

DWORD CIcqProto::GenerateCookie(WORD wIdent)
{
	DWORD dwThisSeq;

	EnterCriticalSection(&cookieMutex);
	dwThisSeq = wCookieSeq++;
	dwThisSeq &= 0x7FFF;
	dwThisSeq |= wIdent<<0x10;
	LeaveCriticalSection(&cookieMutex);

	return dwThisSeq;
}

int CIcqProto::GetCookieType(DWORD dwCookie)
{
	EnterCriticalSection(&cookieMutex);

	int i = cookies.getIndex(( icq_cookie_info* )&dwCookie );
	if ( i != INVALID_COOKIE_INDEX )
		i = cookies[i]->bType;

	LeaveCriticalSection(&cookieMutex);

	return i;
}

int CIcqProto::FindCookie(DWORD dwCookie, HANDLE *phContact, void **ppvExtra)
{
	int nFound = 0;

	EnterCriticalSection(&cookieMutex);

	int i = cookies.getIndex(( icq_cookie_info* )&dwCookie );
	if (i != INVALID_COOKIE_INDEX)
	{
		if (phContact)
			*phContact = cookies[i]->hContact;
		if (ppvExtra)
			*ppvExtra = cookies[i]->pvExtra;

		// Cookie found
		nFound = 1;
	}

	LeaveCriticalSection(&cookieMutex);

	return nFound;
}

int CIcqProto::FindCookieByData(void *pvExtra, DWORD *pdwCookie, HANDLE *phContact)
{
	int nFound = 0;

	EnterCriticalSection(&cookieMutex);

	for (int i = 0; i < cookies.getCount(); i++)
	{
		if (pvExtra == cookies[i]->pvExtra)
		{
			if (phContact)
				*phContact = cookies[i]->hContact;
			if (pdwCookie)
				*pdwCookie = cookies[i]->dwCookie;

			// Cookie found, exit loop
			nFound = 1;
			break;
		}
	}

	LeaveCriticalSection(&cookieMutex);

	return nFound;
}

int CIcqProto::FindCookieByType(BYTE bType, DWORD *pdwCookie, HANDLE *phContact, void** ppvExtra)
{
  int nFound = 0;

  EnterCriticalSection(&cookieMutex);

  for (int i = 0; i < cookies.getCount(); i++)
  {
    if (bType == cookies[i]->bType)
    {
      if (pdwCookie)
        *pdwCookie = cookies[i]->dwCookie;
      if (phContact)
        *phContact = cookies[i]->hContact;
      if (ppvExtra)
        *ppvExtra = cookies[i]->pvExtra;

      // Cookie found, exit loop
      nFound = 1;
      break;
    }
  }

  LeaveCriticalSection(&cookieMutex);

  return nFound;
}

int CIcqProto::FindMessageCookie(DWORD dwMsgID1, DWORD dwMsgID2, DWORD *pdwCookie, HANDLE *phContact, cookie_message_data **ppvExtra)
{
	int nFound = 0;


	EnterCriticalSection(&cookieMutex);

	for (int i = 0; i < cookies.getCount(); i++)
	{
		if (cookies[i]->bType == CKT_MESSAGE || cookies[i]->bType == CKT_FILE || cookies[i]->bType == CKT_REVERSEDIRECT)
		{ // message cookie found
			cookie_message_data *pCookie = (cookie_message_data*)cookies[i]->pvExtra;

			if (pCookie->dwMsgID1 == dwMsgID1 && pCookie->dwMsgID2 == dwMsgID2)
			{
				if (phContact)
					*phContact = cookies[i]->hContact;
				if (pdwCookie)
					*pdwCookie = cookies[i]->dwCookie;
				if (ppvExtra)
					*ppvExtra = pCookie;

				// Cookie found, exit loop
				nFound = 1;
				break;
			}
		}
	}

	LeaveCriticalSection(&cookieMutex);

	return nFound;
}

void CIcqProto::FreeCookie(DWORD dwCookie)
{
	EnterCriticalSection(&cookieMutex);

	int i = cookies.getIndex((icq_cookie_info*)&dwCookie);
	if (i != INVALID_COOKIE_INDEX)
  {	// Cookie found, remove from list
    icq_cookie_info *cookie = cookies[i];

		cookies.remove(i);
    SAFE_FREE((void**)&cookie);
  }

	RemoveExpiredCookies();

	LeaveCriticalSection(&cookieMutex);
}

void CIcqProto::FreeCookieByData(BYTE bType, void *pvExtra)
{
	EnterCriticalSection(&cookieMutex);

	for (int i = 0; i < cookies.getCount(); i++)
  {
    icq_cookie_info *cookie = cookies[i];

		if (bType == cookie->bType && pvExtra == cookie->pvExtra)
		{ // Cookie found, remove from list
			cookies.remove(i);
      SAFE_FREE((void**)&cookie);
			break;
		}
  }

	RemoveExpiredCookies();

	LeaveCriticalSection(&cookieMutex);
}

void CIcqProto::ReleaseCookie(DWORD dwCookie)
{
	EnterCriticalSection(&cookieMutex);

	int i = cookies.getIndex(( icq_cookie_info* )&dwCookie );
	if (i != INVALID_COOKIE_INDEX)
	{ // Cookie found, remove from list
    icq_cookie_info *cookie = cookies[i];

		cookies.remove(i);
		SAFE_FREE((void**)&cookie->pvExtra);
    SAFE_FREE((void**)&cookie);
	}
	RemoveExpiredCookies();

	LeaveCriticalSection(&cookieMutex);
}

void CIcqProto::InitMessageCookie(cookie_message_data *pCookie)
{
	DWORD dwMsgID1;
	DWORD dwMsgID2;

	do
	{ // ensure that message ids are unique
		dwMsgID1 = time(NULL);
		dwMsgID2 = RandRange(0, 0x0FFFF);
	} while (FindMessageCookie(dwMsgID1, dwMsgID2, NULL, NULL, NULL));

	if (pCookie)
	{
		pCookie->dwMsgID1 = dwMsgID1;
		pCookie->dwMsgID2 = dwMsgID2;
	}
}

cookie_message_data* CIcqProto::CreateMessageCookie(WORD bMsgType, BYTE bAckType)
{
	cookie_message_data *pCookie = (cookie_message_data*)SAFE_MALLOC(bMsgType == MTYPE_PLAIN ? sizeof(cookie_message_data_ext) : sizeof(cookie_message_data));
	if (pCookie)
	{
		pCookie->bMessageType = bMsgType;
		pCookie->nAckType = bAckType;

		InitMessageCookie(pCookie);
	}
	return pCookie;
}

cookie_message_data* CIcqProto::CreateMessageCookieData(BYTE bMsgType, HANDLE hContact, DWORD dwUin, int bUseSrvRelay)
{
	BYTE bAckType;
	WORD wStatus = getContactStatus(hContact);

	if (!getSettingByte(NULL, "SlowSend", DEFAULT_SLOWSEND))
		bAckType = ACKTYPE_NONE;
	else if ((bUseSrvRelay && ((!dwUin) || (!CheckContactCapabilities(hContact, CAPF_SRV_RELAY)) ||
		(wStatus == ID_STATUS_OFFLINE))) || getSettingByte(NULL, "OnlyServerAcks", DEFAULT_ONLYSERVERACKS))
		bAckType = ACKTYPE_SERVER;
	else
		bAckType = ACKTYPE_CLIENT;

	return CreateMessageCookie(bMsgType, bAckType);
}
