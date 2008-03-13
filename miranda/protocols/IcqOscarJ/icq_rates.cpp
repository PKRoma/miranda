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
//  Rate Management stuff
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


//
// Rate Level 1 Management
/////////////////////////////

CRITICAL_SECTION ratesMutex;

rates* gRates;

rates* ratesCreate(BYTE* pBuffer, WORD wLen)
{
  rates* pRates;
  WORD wCount;
  int i;

  unpackWord(&pBuffer, &wCount);
  wLen -= 2;
  
  pRates = (rates*)SAFE_MALLOC(sizeof(rates)+(wCount)*sizeof(rates_group));
  pRates->nGroups = wCount;
  // Parse Group details
  for (i=0; i<wCount; i++)
  {
    rates_group* pGroup = &pRates->groups[i];

    if (wLen >= 35)
    {
      pBuffer += 2; // Group ID
      unpackDWord(&pBuffer, &pGroup->dwWindowSize);
      unpackDWord(&pBuffer, &pGroup->dwClearLevel);
      unpackDWord(&pBuffer, &pGroup->dwAlertLevel);
      unpackDWord(&pBuffer, &pGroup->dwLimitLevel);
      pBuffer += 8;
      unpackDWord(&pBuffer, &pGroup->dwMaxLevel);
      pBuffer += 5;
      wLen -= 35;
    }
    else
    { // packet broken, put some basic defaults
      pGroup->dwWindowSize = 10;
      pGroup->dwMaxLevel = 5000;
    }
    pGroup->rCurrentLevel = pGroup->dwMaxLevel;
  }
  // Parse Group associated pairs
  for (i=0; i<wCount; i++)
  {
    rates_group* pGroup = &pRates->groups[i];
    WORD wNum;
    int n;

    if (wLen<4) break;
    pBuffer += 2; // Group ID
    unpackWord(&pBuffer, &wNum);
    wLen -= 4;
    if (wLen < wNum*4) break;
    pGroup->nPairs = wNum;
    pGroup->pPairs = (WORD*)SAFE_MALLOC(wNum*4);
    for (n=0; n<wNum*2; n++)
    {
      WORD wItem;

      unpackWord(&pBuffer, &wItem);
      pGroup->pPairs[n] = wItem;
    }
#ifdef _DEBUG
    NetLog_Server("Rates: %d# %d pairs.", i+1, wNum);
#endif
    wLen -= wNum*4;
  }

  return pRates;
}



void ratesRelease(rates** pRates)
{
  if (pRates)
  {
    rates* rates = *pRates;

    EnterCriticalSection(&ratesMutex);

    if (rates)
    {
      int i;

      for (i = 0; i < rates->nGroups; i++)
      {
        SAFE_FREE((void**)&rates->groups[i].pPairs);
      }
      SAFE_FREE((void**)pRates);
    }
    LeaveCriticalSection(&ratesMutex);
  }
}



WORD ratesGroupFromSNAC(rates* pRates, WORD wFamily, WORD wCommand)
{
  int i;

  if (pRates)
  {
    for (i = 0; i < pRates->nGroups; i++)
    {
      rates_group* group = &pRates->groups[i];
      int j;

      for (j = 0; j < 2*group->nPairs; j += 2)
      {
        if (group->pPairs[j] == wFamily && group->pPairs[j + 1] == wCommand)
        { // we found the group
          return (WORD)(i + 1);
        }
      }
    }
    _ASSERTE(0);
  }

  return 0; // Failure
}



WORD ratesGroupFromPacket(rates* pRates, icq_packet* pPacket)
{
  if (pRates)
  {
    if (pPacket->nChannel == ICQ_DATA_CHAN && pPacket->wLen >= 0x10)
    {
      WORD wFam, wCmd;
      BYTE* pBuf = pPacket->pData + 6;

      unpackWord(&pBuf, &wFam);
      unpackWord(&pBuf, &wCmd);

      return ratesGroupFromSNAC(pRates, wFam, wCmd);
    }
  }
  return 0;
}



static rates_group* getRatesGroup(rates* pRates, WORD wGroup)
{
  if (pRates && wGroup <= pRates->nGroups)
  {
    return &pRates->groups[wGroup-1];
  }
  return NULL;
}


int ratesNextRateLevel(rates* pRates, WORD wGroup)
{
  rates_group* pGroup = getRatesGroup(pRates, wGroup);

  if (pGroup)
  {
    int nLevel = pGroup->rCurrentLevel*(pGroup->dwWindowSize-1)/pGroup->dwWindowSize + (GetTickCount() - pGroup->tCurrentLevel)/pGroup->dwWindowSize;

    return nLevel < (int)pGroup->dwMaxLevel ? nLevel : pGroup->dwMaxLevel;
  }
  return -1; // Failure
}



int ratesDelayToLevel(rates* pRates, WORD wGroup, int nLevel)
{
  rates_group* pGroup = getRatesGroup(pRates, wGroup);

  if (pGroup)
  {
    return (nLevel - pGroup->rCurrentLevel)*pGroup->dwWindowSize + pGroup->rCurrentLevel;
  }
  return 0; // Failure
}



void ratesPacketSent(rates* pRates, icq_packet* pPacket)
{
  if (pRates)
  {
    WORD wGroup = ratesGroupFromPacket(pRates, pPacket);

    if (wGroup)
      ratesUpdateLevel(pRates, wGroup, ratesNextRateLevel(pRates, wGroup));
  }
}



void ratesUpdateLevel(rates* pRates, WORD wGroup, int nLevel)
{
  rates_group* pGroup = getRatesGroup(pRates, wGroup);

  if (pGroup)
  {
    pGroup->rCurrentLevel = nLevel;
    pGroup->tCurrentLevel = GetTickCount();
#ifdef _DEBUG
    NetLog_Server("Rates: New level %d for #%d", nLevel, wGroup);
#endif
  }
}



int ratesGetLimitLevel(rates* pRates, WORD wGroup, int nLevel)
{
  rates_group* pGroup = getRatesGroup(pRates, wGroup);

  if (pGroup)
  {
    switch(nLevel)
    {
    case RML_CLEAR:
      return pGroup->dwClearLevel;

    case RML_ALERT:
      return pGroup->dwAlertLevel;

    case RML_LIMIT:
      return pGroup->dwLimitLevel;

    case RML_IDLE_10:
      return pGroup->dwClearLevel + ((pGroup->dwMaxLevel - pGroup->dwClearLevel)/10);

    case RML_IDLE_30:
      return pGroup->dwClearLevel + (3*(pGroup->dwMaxLevel - pGroup->dwClearLevel)/10);

    case RML_IDLE_50:
      return pGroup->dwClearLevel + ((pGroup->dwMaxLevel - pGroup->dwClearLevel)/2);

    case RML_IDLE_70:
      return pGroup->dwClearLevel + (7*(pGroup->dwMaxLevel - pGroup->dwClearLevel)/10);
    }
  }
  return 9999; // some high number - without rates we allow anything
}


//
// Rate Level 2 Management
/////////////////////////////

// links to functions that are under Rate Control
extern DWORD sendXStatusDetailsRequest(HANDLE hContact, int bForced);

static CRITICAL_SECTION ratesListsMutex;  // we need to be thread safe

static rate_record **pendingList1; // rate queue for xtraz requests
static int pendingListSize1;

static rate_record **pendingList2; // rate queue for msg responses
static int pendingListSize2;


  
typedef struct rate_delay_args_s
{
  int nDelay;
  void (*delaycode)();
} rate_delay_args;

static DWORD __stdcall rateDelayThread(rate_delay_args* pArgs)
{
  SleepEx(pArgs->nDelay, TRUE);
  pArgs->delaycode();

  SAFE_FREE((void**)&pArgs);
  return 0;
}



void InitDelay(int nDelay, void (*delaycode)())
{
  rate_delay_args* pArgs;

#ifdef _DEBUG
  NetLog_Server("Delay %dms", nDelay);
#endif

  pArgs = (rate_delay_args*)SAFE_MALLOC(sizeof(rate_delay_args)); // This will be freed in the new thread

  pArgs->nDelay = nDelay;
  pArgs->delaycode = delaycode;

  ICQCreateThread((pThreadFuncEx)rateDelayThread, pArgs);
}



static void RatesTimer1()
{
  rate_record *item;
  int bSetupTimer = 0;

  EnterCriticalSection(&ratesListsMutex);
  if (!pendingList1)
  {
    LeaveCriticalSection(&ratesListsMutex);
    return;
  }
  
  if (!icqOnline)
  {
    int i;

    for (i=0; i<pendingListSize1; i++) SAFE_FREE((void**)&pendingList1[i]);
    SAFE_FREE((void**)(void**)&pendingList1);
    pendingListSize1 = 0;
    LeaveCriticalSection(&ratesListsMutex);
    
    return;
  }
  // take from queue, execute
  item = pendingList1[0];

  EnterCriticalSection(&ratesMutex);
  if (ratesNextRateLevel(gRates, item->wGroup) < ratesGetLimitLevel(gRates, item->wGroup, RML_IDLE_30))
  { // the rate is higher, keep sleeping
    int nDelay = ratesDelayToLevel(gRates, item->wGroup, ratesGetLimitLevel(gRates, item->wGroup, RML_IDLE_50));

    LeaveCriticalSection(&ratesMutex);
    LeaveCriticalSection(&ratesListsMutex);
    if (nDelay < 10) nDelay = 10;
    InitDelay(nDelay, RatesTimer1);

    return;
  }
  LeaveCriticalSection(&ratesMutex);

  if (pendingListSize1 > 1)
  { // we need to keep order
    memmove(&pendingList1[0], &pendingList1[1], (pendingListSize1 - 1)*sizeof(rate_record*));
  }
  else
    SAFE_FREE((void**)(void**)&pendingList1);
  pendingListSize1--;

  if (pendingListSize1)
    bSetupTimer = 1;

  LeaveCriticalSection(&ratesListsMutex);

  if (icqOnline)
  {
    NetLog_Server("Rates: Resuming Xtraz request.");
    if (item->bType = RIT_XSTATUS_REQUEST)
      sendXStatusDetailsRequest(item->hContact, FALSE);
  }
  else
    NetLog_Server("Rates: Discarding request.");

  if (bSetupTimer)
  { // in queue remained some items, setup timer
    int nDelay;

    EnterCriticalSection(&ratesMutex);
    nDelay = ratesDelayToLevel(gRates, item->wGroup, ratesGetLimitLevel(gRates, item->wGroup, RML_IDLE_50));
    LeaveCriticalSection(&ratesMutex);

    if (nDelay < 10) nDelay = 10;
    InitDelay(nDelay, RatesTimer1);
  }
  SAFE_FREE((void**)&item);
}



static void putItemToQueue1(rate_record *item, int nLev)
{
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
    pendingList1 = (rate_record**)SAFE_REALLOC(pendingList1, pendingListSize1*sizeof(rate_record*));
    tmp = (rate_record*)SAFE_MALLOC(sizeof(rate_record));
    memcpy(tmp, item, sizeof(rate_record));
    pendingList1[pendingListSize1 - 1] = tmp;
  }
  else
  {
    rate_record *tmp;
    int nDelay;
    
    EnterCriticalSection(&ratesMutex);
    nDelay = ratesDelayToLevel(gRates, item->wGroup, ratesGetLimitLevel(gRates, item->wGroup, RML_IDLE_50));
    LeaveCriticalSection(&ratesMutex);

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
  int bSetupTimer = 0;

  EnterCriticalSection(&ratesListsMutex);
  if (!pendingList2)
  {
    LeaveCriticalSection(&ratesListsMutex);
    return;
  }
  
  if (!icqOnline)
  {
    int i;

    for (i=0; i<pendingListSize2; i++)
    {
      SAFE_FREE((void**)&pendingList2[i]->szData);
      SAFE_FREE((void**)&pendingList2[i]);
    }
    SAFE_FREE((void**)(void**)&pendingList2);
    pendingListSize2 = 0;
    LeaveCriticalSection(&ratesListsMutex);
    
    return;
  }
  // take from queue, execute
  item = pendingList2[0];

  EnterCriticalSection(&ratesMutex);
  if (ratesNextRateLevel(gRates, item->wGroup) < ratesGetLimitLevel(gRates, item->wGroup, RML_IDLE_10))
  { // the rate is higher, keep sleeping
    int nDelay = ratesDelayToLevel(gRates, item->wGroup, ratesGetLimitLevel(gRates, item->wGroup, RML_IDLE_30));

    LeaveCriticalSection(&ratesMutex);
    LeaveCriticalSection(&ratesListsMutex);
    if (nDelay < 10) nDelay = 10;
    InitDelay(nDelay, RatesTimer2);

    return;
  }

  LeaveCriticalSection(&ratesMutex);
  if (pendingListSize2 > 1)
  { // we need to keep order
    memmove(&pendingList2[0], &pendingList2[1], (pendingListSize2 - 1)*sizeof(rate_record*));
  }
  else
    SAFE_FREE((void**)(void**)&pendingList2);
  pendingListSize2--;

  if (pendingListSize2)
    bSetupTimer = 1;

  LeaveCriticalSection(&ratesListsMutex);

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
  SAFE_FREE((void**)&item->szData);

  if (bSetupTimer)
  { // in queue remained some items, setup timer
    int nDelay;
    
    EnterCriticalSection(&ratesMutex);
    nDelay = ratesDelayToLevel(gRates, item->wGroup, ratesGetLimitLevel(gRates, item->wGroup, RML_IDLE_30));
    LeaveCriticalSection(&ratesMutex);

    if (nDelay < 10) nDelay = 10;
    InitDelay(nDelay, RatesTimer2);
  }
  SAFE_FREE((void**)&item);
}



static void putItemToQueue2(rate_record *item, int nLev)
{
  rate_record *tmp;
  int bFound = FALSE;

  if (!icqOnline) return;

  NetLog_Server("Rates: Delaying operation.");

  if (pendingListSize2)
  {
    int i;

    for (i = 0; i < pendingListSize2; i++)
    {
      if (pendingList2[i]->hContact == item->hContact && pendingList2[i]->bType == item->bType)
      { // there is older response in the queue, discard it (server won't accept it anyway)
        SAFE_FREE((void**)&pendingList2[i]->szData);
        SAFE_FREE((void**)&pendingList2[i]);
        memcpy(&pendingList2[i], &pendingList2[i + 1], (pendingListSize2 - i - 1)*sizeof(rate_record*));
        bFound = TRUE;
      }
    }
  }
  if (!bFound)
  { // not found, enlarge the queue
    pendingListSize2++;
    pendingList2 = (rate_record**)SAFE_REALLOC(pendingList2, pendingListSize2*sizeof(rate_record*));
  }
  tmp = (rate_record*)SAFE_MALLOC(sizeof(rate_record));
  memcpy(tmp, item, sizeof(rate_record));
  tmp->szData = null_strdup(item->szData);
  pendingList2[pendingListSize2 - 1] = tmp;

  if (pendingListSize2 == 1)
  { // queue was empty setup timer
    int nDelay;
    
    EnterCriticalSection(&ratesMutex);
    nDelay = ratesDelayToLevel(gRates, item->wGroup, ratesGetLimitLevel(gRates, item->wGroup, RML_IDLE_30));
    LeaveCriticalSection(&ratesMutex);

    if (nDelay < 10) nDelay = 10;
    InitDelay(nDelay, RatesTimer2);
  }
}



int handleRateItem(rate_record *item, BOOL bAllowDelay)
{
  if (!gRates) return 0;

  EnterCriticalSection(&ratesListsMutex);

  if (item->nRequestType == 0x101)
  { // xtraz request
    int nLev, nLimit;
    
    EnterCriticalSection(&ratesMutex);
    nLev = ratesNextRateLevel(gRates, item->wGroup);
    nLimit = ratesGetLimitLevel(gRates, item->wGroup, RML_IDLE_50) + 200;
    LeaveCriticalSection(&ratesMutex);

    if ((nLev < nLimit || item->nMinDelay) && bAllowDelay)
    { // limit reached or min delay configured, add to queue
      putItemToQueue1(item, nLev);
      LeaveCriticalSection(&ratesListsMutex);
      return 1;
    }
  }
  else if (item->nRequestType == 0x102)
  { // msg response
    int nLev, nLimit;

    EnterCriticalSection(&ratesMutex);
    nLev = ratesNextRateLevel(gRates, item->wGroup);
    nLimit = ratesGetLimitLevel(gRates, item->wGroup, RML_IDLE_30) + 100;
    LeaveCriticalSection(&ratesMutex);

    if (nLev < nLimit)
    { // limit reached, add to queue
      putItemToQueue2(item, nLev);
      LeaveCriticalSection(&ratesListsMutex);
      return 1;
    }
  }
  LeaveCriticalSection(&ratesListsMutex);

  return 0;
}



void InitRates()
{
  InitializeCriticalSection(&ratesMutex);
  InitializeCriticalSection(&ratesListsMutex);

  pendingListSize1 = 0;
  pendingList1 = NULL;
  pendingListSize2 = 0;
  pendingList2 = NULL;
}



void UninitRates()
{
  SAFE_FREE((void**)(void**)&pendingList1);
  SAFE_FREE((void**)(void**)&pendingList2);
  DeleteCriticalSection(&ratesMutex);
  DeleteCriticalSection(&ratesListsMutex);
}
