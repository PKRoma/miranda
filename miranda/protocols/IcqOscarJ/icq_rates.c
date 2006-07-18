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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/icq_rates.c,v $
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Rate Management stuff
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"

// links to functions that are under Rate Control
extern DWORD sendXStatusDetailsRequest(HANDLE hContact, int bForced);

static int rGroupXtrazRequest = 4500; // represents higher limits for ICQ Rate Group #3
static int tGroupXtrazRequest;
static int rGroupMsgResponse = 6000;  //  dtto #1
static int tGroupMsgResponse;

static CRITICAL_SECTION ratesMutex;  // we need to be thread safe

static rate_record **pendingList1; // rate queue for xtraz requests
static int pendingListSize1;

static rate_record **pendingList2; // rate queue for msg responses
static int pendingListSize2;


  
typedef struct rate_delay_args_s
{
  int nDelay;
  void (*delaycode)();
} rate_delay_args;

void __cdecl rateDelayThread(rate_delay_args* pArgs)
{
  SleepEx(pArgs->nDelay, TRUE);
  pArgs->delaycode();

  SAFE_FREE(&pArgs);
  return;
}



void InitDelay(int nDelay, void (*delaycode)())
{
  rate_delay_args* pArgs;

//#ifdef _DEBUG
  NetLog_Server("Delay %dms", nDelay);
//#endif

  pArgs = (rate_delay_args*)SAFE_MALLOC(sizeof(rate_delay_args)); // This will be freed in the new thread

  pArgs->nDelay = nDelay;
  pArgs->delaycode = delaycode;

  forkthread(rateDelayThread, 0, pArgs);
}



static void RatesTimer1()
{
  rate_record *item;
  int nLev;

  if (!pendingList1) return;

  EnterCriticalSection(&ratesMutex);
  // take from queue, execute
  item = pendingList1[0];
  if (pendingListSize1 > 1)
  { // we need to keep order
    memmove(&pendingList1[0], &pendingList1[1], (pendingListSize1 - 1)*sizeof(rate_record*));
  }
  else
    SAFE_FREE((void**)&pendingList1);
  pendingListSize1--;
  nLev = rGroupXtrazRequest*19/20 + (GetTickCount() - tGroupXtrazRequest)/20;
  rGroupXtrazRequest = nLev < 4500 ? nLev : 4500;
  tGroupXtrazRequest = GetTickCount();
  if (pendingListSize1 && icqOnline)
  { // in queue remains some items, setup timer
    int nDelay = (3800 - rGroupXtrazRequest)*20;

    if (nDelay < 10) nDelay = 10;
    InitDelay(nDelay, RatesTimer1);
  }
  if (!icqOnline)
  {
    int i;

    for (i=0; i<pendingListSize1; i++) SAFE_FREE(&pendingList1[i]);
    SAFE_FREE((void**)&pendingList1);
    pendingListSize1 = 0;
  }
  LeaveCriticalSection(&ratesMutex);

  if (icqOnline)
  {
    NetLog_Server("Rates: Resuming Xtraz request.");
    if (item->bType = RIT_XSTATUS_REQUEST)
      sendXStatusDetailsRequest(item->hContact, FALSE);
  }
  else
    NetLog_Server("Rates: Discarding request.");
  SAFE_FREE(&item);
}



static void putItemToQueue1(rate_record *item, int nLev)
{
  int nDelay = (3800 - nLev)*20;

  if (!icqOnline) return;

  NetLog_Server("Rates: Delaying operation.");

  if (pendingListSize1)
  { // something already in queue, check duplicity
    rate_record *tmp;
    int i;

    for (i = 0; i < pendingListSize1; i++)
    { // TODO: make this more universal - for more xtraz msg types
      if (pendingList1[i]->hContact == item->hContact)
        return; // request xstatus from same contact, do it only once
    }
    pendingListSize1++;
    pendingList1 = (rate_record**)realloc(pendingList1, pendingListSize1*sizeof(rate_record*));
    tmp = (rate_record*)SAFE_MALLOC(sizeof(rate_record));
    memcpy(tmp, item, sizeof(rate_record));
    pendingList1[pendingListSize1 - 1] = tmp;
  }
  else
  {
    rate_record *tmp;

    pendingListSize1++;
    pendingList1 = (rate_record**)SAFE_MALLOC(sizeof(rate_record*));
    tmp = (rate_record*)SAFE_MALLOC(sizeof(rate_record));
    memcpy(tmp, item, sizeof(rate_record));
    pendingList1[0] = tmp;

    if (nDelay < 10) nDelay = 10;
    if (nDelay < item->nMinDelay) nDelay = item->nMinDelay;
    InitDelay(nDelay, RatesTimer1);
  }
}



static void RatesTimer2()
{
  rate_record *item;
  int nLev;

  if (!pendingList2) return;

  EnterCriticalSection(&ratesMutex);
  // take from queue, execute
  item = pendingList2[0];
  if (pendingListSize2 > 1)
  { // we need to keep order
    memmove(&pendingList2[0], &pendingList2[1], (pendingListSize2 - 1)*sizeof(rate_record*));
  }
  else
    SAFE_FREE((void**)&pendingList2);
  pendingListSize2--;
  nLev = rGroupMsgResponse*79/80 + (GetTickCount() - tGroupMsgResponse)/80;
  rGroupMsgResponse = nLev < 6000 ? nLev : 6000;
  tGroupMsgResponse = GetTickCount();
  if (pendingListSize2 && icqOnline)
  { // in queue remains some items, setup timer
    int nDelay = (4500 - rGroupMsgResponse)*80;

    if (nDelay < 10) nDelay = 10;
    InitDelay(nDelay, RatesTimer2);
  }
  if (!icqOnline)
  {
    int i;

    for (i=0; i<pendingListSize2; i++) SAFE_FREE(&pendingList2[i]);
    SAFE_FREE((void**)&pendingList2);
    pendingListSize2 = 0;
  }
  LeaveCriticalSection(&ratesMutex);

  if (icqOnline)
  {
    NetLog_Server("Rates: Resuming message response.");
    if (item->bType == RIT_AWAYMSG_RESPONSE)
    {
      icq_sendAwayMsgReplyServ(item->dwUin, item->dwMid1, item->dwMid2, item->wCookie, item->wVersion, item->msgType, MirandaStatusToAwayMsg(AwayMsgTypeToStatus(item->msgType)));
    }
    else if (item->bType == RIT_XSTATUS_RESPONSE)
    {
      SendXtrazNotifyResponse(item->dwUin, item->dwMid1, item->dwMid2, item->wCookie, item->szData, strlennull(item->szData), item->bThruDC);
    }
  }
  else
    NetLog_Server("Rates: Discarding response.");
  SAFE_FREE(&item->szData);
  SAFE_FREE(&item);
}



static void putItemToQueue2(rate_record *item, int nLev)
{
  int nDelay = (4500 - nLev)*80;
  rate_record *tmp;

  if (!icqOnline) return;

  NetLog_Server("Rates: Delaying operation.");

  pendingListSize2++;
  pendingList2 = (rate_record**)realloc(pendingList2, pendingListSize2*sizeof(rate_record*));
  tmp = (rate_record*)SAFE_MALLOC(sizeof(rate_record));
  memcpy(tmp, item, sizeof(rate_record));
  tmp->szData = null_strdup(item->szData);
  pendingList2[pendingListSize2 - 1] = tmp;

  if (pendingListSize2 == 1)
  { // queue was empty setup timer
    if (nDelay < 10) nDelay = 10;
    InitDelay(nDelay, RatesTimer2);
  }
}



int handleRateItem(rate_record *item, BOOL bAllowDelay)
{
  int tNow = GetTickCount();

  EnterCriticalSection(&ratesMutex);

  if (item->rate_group == 0x101)
  { // xtraz request
    int nLev = rGroupXtrazRequest*19/20 + (tNow - tGroupXtrazRequest)/20;

    if ((nLev < 3800 || item->nMinDelay) && bAllowDelay)
    { // limit reached or min delay configured, add to queue
      putItemToQueue1(item, nLev);
      LeaveCriticalSection(&ratesMutex);
      return 1;
    }
    else
    {
      rGroupXtrazRequest = nLev < 4500 ? nLev : 4500;
      tGroupXtrazRequest = tNow;
    }
  }
  else if (item->rate_group == 0x102)
  { // msg response
    int nLev = rGroupMsgResponse*79/80 + (tNow - tGroupMsgResponse)/80;

    if (nLev < 4500)
    { // limit reached or min delay configured, add to queue
      putItemToQueue2(item, nLev);
      LeaveCriticalSection(&ratesMutex);
      return 1;
    }
    else
    {
      rGroupMsgResponse = nLev < 6000 ? nLev : 6000;
      tGroupMsgResponse = tNow;
    }
  }
  LeaveCriticalSection(&ratesMutex);

  return 0;
}



void InitRates()
{
  InitializeCriticalSection(&ratesMutex);

  pendingListSize1 = 0;
  pendingList1 = NULL;
  pendingListSize2 = 0;
  pendingList2 = NULL;

  tGroupXtrazRequest = tGroupMsgResponse = GetTickCount();
}



void UninitRates()
{
  SAFE_FREE((void**)&pendingList1);
  SAFE_FREE((void**)&pendingList2);
  DeleteCriticalSection(&ratesMutex);
}
