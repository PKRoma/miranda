// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006 Joe Kucera
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

    pNew = (icq_cookie_info *)realloc(cookie, sizeof(icq_cookie_info) * newSize);

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
DWORD AllocateCookie(WORD wIdent, DWORD dwUin, void *pvExtra)
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
  DWORD tNow = time(NULL);


  EnterCriticalSection(&cookieMutex);

  for (i = 0; i < cookieCount; i++)
  {
    if (dwCookie == cookie[i].dwCookie)
    {
      cookieCount--;
      memmove(&cookie[i], &cookie[i+1], sizeof(icq_cookie_info) * (cookieCount - i));
      ResizeCookieList(cookieCount);

      // Cookie found, exit loop
      break;
    }
    if ((cookie[i].dwTime + COOKIE_TIMEOUT) < tNow)
    { // cookie expired, remove too
      cookieCount--;
      memmove(&cookie[i], &cookie[i+1], sizeof(icq_cookie_info) * (cookieCount - i));
      ResizeCookieList(cookieCount);
      i--; // fix the loop
    }
  }

  LeaveCriticalSection(&cookieMutex);
}
