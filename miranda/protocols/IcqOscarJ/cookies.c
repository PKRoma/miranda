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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/cookies.c,v $
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



static WORD wCookieSeq;
static icq_cookie_info *cookie = NULL;
static int cookieCount = 0;
static int cookieSize = 0;
CRITICAL_SECTION cookieMutex; // we want this in avatar thread, used as queue lock

static int ResizeCookieList(int nSize)
{
  if ((cookieSize < nSize) || ((cookieSize > nSize + 6) && nSize))
  {
    icq_cookie_info *pNew;
    int newSize;

    if (cookieSize < nSize)
      newSize = cookieSize + 4;
    else
      newSize = cookieSize - 4;

    pNew = (icq_cookie_info *)SAFE_REALLOC(cookie, sizeof(icq_cookie_info) * newSize);

    if (!pNew)
    { // realloc failed, cookies intact... try again
      NetLog_Server("ResizeCookieList: realloc failed.");

      return 1; // Failure
    }
    else
    {
      cookie = pNew;
      cookieSize = newSize;
    }
  }
  return 0; // Success
}


#define INVALID_COOKIE_INDEX -1

static int FindCookieIndex(DWORD dwCookie)
{
  int i;

  for (i = 0; i < cookieCount; i++)
  {
    if (dwCookie == cookie[i].dwCookie)
    {
      return i;
    }
  }
  return INVALID_COOKIE_INDEX;
}



static void RemoveCookieIndex(int iCookie)
{
  cookieCount--;
  memmove(&cookie[iCookie], &cookie[iCookie + 1], sizeof(icq_cookie_info) * (cookieCount - iCookie));
  ResizeCookieList(cookieCount);
}



static void RemoveExpiredCookies()
{
  int i;
  DWORD tNow = time(NULL);

  for (i = 0; i < cookieCount; i++)
  {
    if ((cookie[i].dwTime + COOKIE_TIMEOUT) < tNow)
    { // cookie expired, remove too
      RemoveCookieIndex(i);
      i--; // fix the loop
    }
  }
}



void InitCookies(void)
{
  InitializeCriticalSection(&cookieMutex);

  cookieCount = 0;
  cookieSize = 0;
  cookie = NULL;
  wCookieSeq = 2;

  ResizeCookieList(4);
}



void UninitCookies(void)
{
  SAFE_FREE(&cookie);

  DeleteCriticalSection(&cookieMutex);
}



// Generate and allocate cookie
DWORD AllocateCookie(BYTE bType, WORD wIdent, HANDLE hContact, void *pvExtra)
{
  DWORD dwThisSeq;

  EnterCriticalSection(&cookieMutex);

  if (ResizeCookieList(cookieCount + 1))
  { // resizing failed...
    LeaveCriticalSection(&cookieMutex);
    // this is horrible, but can't do anything better
    return GenerateCookie(wIdent);
  }

  dwThisSeq = wCookieSeq++;
  dwThisSeq &= 0x7FFF;
  dwThisSeq |= wIdent<<0x10;

  cookie[cookieCount].bType = bType;
  cookie[cookieCount].dwCookie = dwThisSeq;
  cookie[cookieCount].hContact = hContact;
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



int GetCookieType(DWORD dwCookie)
{
  int i;

  EnterCriticalSection(&cookieMutex);

  i = FindCookieIndex(dwCookie);

  if (i != INVALID_COOKIE_INDEX)
    i = cookie[i].bType;

  LeaveCriticalSection(&cookieMutex);

  return i;
}



int FindCookie(DWORD dwCookie, HANDLE *phContact, void **ppvExtra)
{
  int i;
  int nFound = 0;


  EnterCriticalSection(&cookieMutex);

  i = FindCookieIndex(dwCookie);

  if (i != INVALID_COOKIE_INDEX)
  {
    if (phContact)
      *phContact = cookie[i].hContact;
    if (ppvExtra)
      *ppvExtra = cookie[i].pvExtra;

    // Cookie found
    nFound = 1;
  }

  LeaveCriticalSection(&cookieMutex);

  return nFound;
}



int FindCookieByData(void *pvExtra,DWORD *pdwCookie, HANDLE *phContact)
{
  int i;
  int nFound = 0;


  EnterCriticalSection(&cookieMutex);

  for (i = 0; i < cookieCount; i++)
  {
    if (pvExtra == cookie[i].pvExtra)
    {
      if (phContact)
        *phContact = cookie[i].hContact;
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



int FindMessageCookie(DWORD dwMsgID1, DWORD dwMsgID2, DWORD *pdwCookie, HANDLE *phContact, message_cookie_data **ppvExtra)
{
  int i;
  int nFound = 0;


  EnterCriticalSection(&cookieMutex);

  for (i = 0; i < cookieCount; i++)
  {
    if (cookie[i].bType == CKT_MESSAGE || cookie[i].bType == CKT_FILE || cookie[i].bType == CKT_REVERSEDIRECT)
    { // message cookie found
      message_cookie_data *pCookie = (message_cookie_data*)cookie[i].pvExtra;

      if (pCookie->dwMsgID1 == dwMsgID1 && pCookie->dwMsgID2 == dwMsgID2)
      {
        if (phContact)
          *phContact = cookie[i].hContact;
        if (pdwCookie)
          *pdwCookie = cookie[i].dwCookie;
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



void FreeCookie(DWORD dwCookie)
{
  int i;

  EnterCriticalSection(&cookieMutex);

  i = FindCookieIndex(dwCookie);

  if (i != INVALID_COOKIE_INDEX)
  { // Cookie found, remove from list
    RemoveCookieIndex(i);
  }
  RemoveExpiredCookies();

  LeaveCriticalSection(&cookieMutex);
}



void ReleaseCookie(DWORD dwCookie)
{
  int i;

  EnterCriticalSection(&cookieMutex);

  i = FindCookieIndex(dwCookie);

  if (i != INVALID_COOKIE_INDEX)
  { // Cookie found, remove from list
    SAFE_FREE(&cookie[i].pvExtra);
    RemoveCookieIndex(i);
  }
  RemoveExpiredCookies();

  LeaveCriticalSection(&cookieMutex);
}



void InitMessageCookie(message_cookie_data *pCookie)
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



message_cookie_data *CreateMessageCookie(WORD bMsgType, BYTE bAckType)
{
  message_cookie_data *pCookie;

  pCookie = (message_cookie_data*)SAFE_MALLOC(bMsgType == MTYPE_PLAIN ? sizeof(message_cookie_data_ex) : sizeof(message_cookie_data));
  if (pCookie)
  {
    pCookie->bMessageType = bMsgType;
    pCookie->nAckType = bAckType;

    InitMessageCookie(pCookie);
  }
  return pCookie;
}

