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
//  Rate Management stuff
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"

//
// Rate Level 1 Management
/////////////////////////////

rates::rates(CIcqProto *ppro, BYTE *pBuffer, WORD wLen)
{
  nGroups = 0;
  memset(&groups, 0, MAX_RATES_GROUP_COUNT * sizeof(rates_group));
  this->ppro = ppro;

  // Parse Rate Data Block
	WORD wCount;
	unpackWord(&pBuffer, &wCount);
	wLen -= 2;

  if (wCount > MAX_RATES_GROUP_COUNT)
  { // just sanity check
    ppro->NetLog_Server("Rates: Error: Data packet contains too many rate groups!");
    wCount = MAX_RATES_GROUP_COUNT;
  }

	nGroups = wCount;
	// Parse Group details
  int i;
	for (i=0; i<wCount; i++)
	{
		rates_group *pGroup = &groups[i];

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
		rates_group *pGroup = &groups[i];
		WORD wNum;

		if (wLen<4) break;
		pBuffer += 2; // Group ID
		unpackWord(&pBuffer, &wNum);
		wLen -= 4;
		if (wLen < wNum*4) break;
		pGroup->nPairs = wNum;
		pGroup->pPairs = (WORD*)SAFE_MALLOC(wNum*4);
		for (int n=0; n<wNum*2; n++)
		{
			WORD wItem;

			unpackWord(&pBuffer, &wItem);
			pGroup->pPairs[n] = wItem;
		}
#ifdef _DEBUG
		ppro->NetLog_Server("Rates: %d# %d pairs.", i+1, wNum);
#endif
		wLen -= wNum*4;
	}
}


rates::~rates()
{
	for (int i = 0; i < nGroups; i++)
		SAFE_FREE((void**)&groups[i].pPairs);

  nGroups = 0;
}


WORD rates::getGroupFromSNAC(WORD wFamily, WORD wCommand)
{
	if (this)
	{
		for (int i = 0; i < nGroups; i++)
		{
			rates_group* group = &groups[i];

			for (int j = 0; j < 2*group->nPairs; j += 2)
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


WORD rates::getGroupFromPacket(icq_packet *pPacket)
{
	if (this)
	{
		if (pPacket->nChannel == ICQ_DATA_CHAN && pPacket->wLen >= 0x10)
		{
			WORD wFam, wCmd;
			BYTE* pBuf = pPacket->pData + 6;

			unpackWord(&pBuf, &wFam);
			unpackWord(&pBuf, &wCmd);

			return getGroupFromSNAC(wFam, wCmd);
		}
	}
	return 0;
}


rates_group* rates::getGroup(WORD wGroup)
{
	if (this && wGroup && wGroup <= nGroups)
		return &groups[wGroup-1];

	return NULL;
}


int rates::getNextRateLevel(WORD wGroup)
{
	rates_group *pGroup = getGroup(wGroup);

  if (pGroup)
	{
		int nLevel = pGroup->rCurrentLevel*(pGroup->dwWindowSize-1)/pGroup->dwWindowSize + (GetTickCount() - pGroup->tCurrentLevel)/pGroup->dwWindowSize;

		return nLevel < (int)pGroup->dwMaxLevel ? nLevel : pGroup->dwMaxLevel;
	}
	return -1; // Failure
}


int rates::getDelayToLimitLevel(WORD wGroup, int nLevel)
{
	rates_group *pGroup = getGroup(wGroup);

	if (pGroup)
		return (getLimitLevel(wGroup, nLevel) - pGroup->rCurrentLevel)*pGroup->dwWindowSize + pGroup->rCurrentLevel;

	return 0; // Failure
}


void rates::packetSent(icq_packet *pPacket)
{
	if (this)
	{
		WORD wGroup = getGroupFromPacket(pPacket);

		if (wGroup)
			updateLevel(wGroup, getNextRateLevel(wGroup));
	}
}


void rates::updateLevel(WORD wGroup, int nLevel)
{
	rates_group *pGroup = getGroup(wGroup);

	if (pGroup)
	{
		pGroup->rCurrentLevel = nLevel;
		pGroup->tCurrentLevel = GetTickCount();
#ifdef _DEBUG
		ppro->NetLog_Server("Rates: New level %d for #%d", nLevel, wGroup);
#endif
	}
}


int rates::getLimitLevel(WORD wGroup, int nLevel)
{
	rates_group *pGroup = getGroup(wGroup);

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


void rates::initAckPacket(icq_packet *pPacket)
{
	serverPacketInit(pPacket, 10 + nGroups * sizeof(WORD)); 
	packFNACHeader(pPacket, ICQ_SERVICE_FAMILY, ICQ_CLIENT_RATE_ACK);
  for (WORD wGroup = 1; wGroup <= nGroups; wGroup++)
    packWord(pPacket, wGroup);
}



//
// Rate Level 2 Management
/////////////////////////////

// links to functions that are under Rate Control
struct rate_delay_args
{
	int nDelay;
	IcqRateFunc delaycode;
};

void __cdecl CIcqProto::rateDelayThread(rate_delay_args* pArgs)
{
	SleepEx(pArgs->nDelay, TRUE);
	(this->*pArgs->delaycode)();
	SAFE_FREE((void**)&pArgs);
}

void CIcqProto::InitDelay(int nDelay, IcqRateFunc delaycode )
{
#ifdef _DEBUG
	NetLog_Server("Delay %dms", nDelay);
#endif

	rate_delay_args* pArgs = (rate_delay_args*)SAFE_MALLOC(sizeof(rate_delay_args)); // This will be freed in the new thread

	pArgs->nDelay = nDelay;
	pArgs->delaycode = delaycode;

	ForkThread(( IcqThreadFunc )&CIcqProto::rateDelayThread, pArgs );
}

void CIcqProto::RatesTimer1()
{
	rate_record *item;
	int bSetupTimer = 0;

	EnterCriticalSection(&ratesListsMutex);
	if (!pendingList1)
	{
		LeaveCriticalSection(&ratesListsMutex);
		return;
	}

	if (!icqOnline())
	{
		for (int i=0; i < pendingListSize1; i++)
			SAFE_FREE((void**)&pendingList1[i]);
		SAFE_FREE((void**)&pendingList1);
		pendingListSize1 = 0;
		LeaveCriticalSection(&ratesListsMutex);
		return;
	}
	// take from queue, execute
	item = pendingList1[0];

	EnterCriticalSection(&ratesMutex);
	if (m_rates->getNextRateLevel(item->wGroup) < m_rates->getLimitLevel(item->wGroup, RML_IDLE_30))
	{ // the rate is higher, keep sleeping
		int nDelay = m_rates->getDelayToLimitLevel(item->wGroup, m_rates->getLimitLevel(item->wGroup, RML_IDLE_50));

		LeaveCriticalSection(&ratesMutex);
		LeaveCriticalSection(&ratesListsMutex);
		if (nDelay < 10) nDelay = 10;
		InitDelay(nDelay, &CIcqProto::RatesTimer1);
		return;
	}
	LeaveCriticalSection(&ratesMutex);

	if (pendingListSize1 > 1)
	{ // we need to keep order
		memmove(&pendingList1[0], &pendingList1[1], (pendingListSize1 - 1)*sizeof(rate_record*));
	}
	else
		SAFE_FREE((void**)&pendingList1);
	pendingListSize1--;

	if (pendingListSize1)
		bSetupTimer = 1;

	LeaveCriticalSection(&ratesListsMutex);

	if (icqOnline())
	{
		NetLog_Server("Rates: Resuming Xtraz request.");
		if (item->bType = RIT_XSTATUS_REQUEST)
			sendXStatusDetailsRequest(item->hContact, FALSE);
	}
	else
		NetLog_Server("Rates: Discarding request.");

	if (bSetupTimer)
	{
		// in queue remained some items, setup timer
		EnterCriticalSection(&ratesMutex);
		int nDelay = m_rates->getDelayToLimitLevel(item->wGroup, RML_IDLE_50);
		LeaveCriticalSection(&ratesMutex);

		if (nDelay < 10) nDelay = 10;
		InitDelay(nDelay, &CIcqProto::RatesTimer1);
	}
	SAFE_FREE((void**)&item);
}

static void putItemToQueue1(CIcqProto* ppro, rate_record *item, int nLev)
{
	if (!ppro->icqOnline()) return;

	ppro->NetLog_Server("Rates: Delaying operation.");

	if (ppro->pendingListSize1)
	{
		// TODO: make this more universal - for more xtraz msg types
		for (int i = 0; i < ppro->pendingListSize1; i++)
			if (ppro->pendingList1[i]->hContact == item->hContact)
				return; // request xstatus from same contact, do it only once

		ppro->pendingListSize1++;
		ppro->pendingList1 = (rate_record**)SAFE_REALLOC(ppro->pendingList1, ppro->pendingListSize1*sizeof(rate_record*));
		rate_record *tmp = (rate_record*)SAFE_MALLOC(sizeof(rate_record));
		memcpy(tmp, item, sizeof(rate_record));
		ppro->pendingList1[ppro->pendingListSize1 - 1] = tmp;
	}
	else
	{
		EnterCriticalSection(&ppro->ratesMutex);
		int nDelay = ppro->m_rates->getDelayToLimitLevel(item->wGroup, RML_IDLE_50);
		LeaveCriticalSection(&ppro->ratesMutex);

		ppro->pendingListSize1++;
		ppro->pendingList1 = (rate_record**)SAFE_MALLOC(sizeof(rate_record*));
		rate_record *tmp = (rate_record*)SAFE_MALLOC(sizeof(rate_record));
		memcpy(tmp, item, sizeof(rate_record));
		ppro->pendingList1[0] = tmp;

		if (nDelay < 10) nDelay = 10;
		if (nDelay < item->nMinDelay) nDelay = item->nMinDelay;
		ppro->InitDelay(nDelay, &CIcqProto::RatesTimer1);
	}
}

void CIcqProto::RatesTimer2()
{
	rate_record *item;
	int bSetupTimer = 0;

	EnterCriticalSection(&ratesListsMutex);
	if (!pendingList2)
	{
		LeaveCriticalSection(&ratesListsMutex);
		return;
	}

	if (!icqOnline())
	{
		for (int i=0; i < pendingListSize2; i++)
		{
			SAFE_FREE((void**)&pendingList2[i]->szData);
			SAFE_FREE((void**)&pendingList2[i]);
		}
		SAFE_FREE((void**)&pendingList2);
		pendingListSize2 = 0;
		LeaveCriticalSection(&ratesListsMutex);
		return;
	}

	// take from queue, execute
	item = pendingList2[0];

	EnterCriticalSection(&ratesMutex);
	if (m_rates->getNextRateLevel(item->wGroup) < m_rates->getLimitLevel(item->wGroup, RML_IDLE_10))
	{ // the rate is higher, keep sleeping
		int nDelay = m_rates->getDelayToLimitLevel(item->wGroup, RML_IDLE_30);

		LeaveCriticalSection(&ratesMutex);
		LeaveCriticalSection(&ratesListsMutex);
		if (nDelay < 10) nDelay = 10;
		InitDelay(nDelay, &CIcqProto::RatesTimer2);
		return;
	}

	LeaveCriticalSection(&ratesMutex);
	if (pendingListSize2 > 1)
	{ // we need to keep order
		memmove(&pendingList2[0], &pendingList2[1], (pendingListSize2 - 1)*sizeof(rate_record*));
	}
	else
		SAFE_FREE((void**)&pendingList2);
	pendingListSize2--;

	if (pendingListSize2)
		bSetupTimer = 1;

	LeaveCriticalSection(&ratesListsMutex);

	if (icqOnline())
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
		EnterCriticalSection(&ratesMutex);
		int nDelay = m_rates->getDelayToLimitLevel(item->wGroup, RML_IDLE_30);
		LeaveCriticalSection(&ratesMutex);

		if (nDelay < 10) nDelay = 10;
		InitDelay(nDelay, &CIcqProto::RatesTimer2);
	}
	SAFE_FREE((void**)&item);
}

static void putItemToQueue2(CIcqProto* ppro, rate_record *item, int nLev)
{
	rate_record *tmp;
	int bFound = FALSE;

	if (!ppro->icqOnline())
		return;

	ppro->NetLog_Server("Rates: Delaying operation.");

	if (ppro->pendingListSize2)
	{
		int i;

		for (i = 0; i < ppro->pendingListSize2; i++)
		{
			if (ppro->pendingList2[i]->hContact == item->hContact && ppro->pendingList2[i]->bType == item->bType)
			{ // there is older response in the queue, discard it (server won't accept it anyway)
				SAFE_FREE((void**)&ppro->pendingList2[i]->szData);
				SAFE_FREE((void**)&ppro->pendingList2[i]);
				memcpy(&ppro->pendingList2[i], &ppro->pendingList2[i + 1], (ppro->pendingListSize2 - i - 1)*sizeof(rate_record*));
				bFound = TRUE;
			}
		}
	}
	if (!bFound)
	{ // not found, enlarge the queue
		ppro->pendingListSize2++;
		ppro->pendingList2 = (rate_record**)SAFE_REALLOC(ppro->pendingList2, ppro->pendingListSize2*sizeof(rate_record*));
	}
	tmp = (rate_record*)SAFE_MALLOC(sizeof(rate_record));
	memcpy(tmp, item, sizeof(rate_record));
	tmp->szData = null_strdup(item->szData);
	ppro->pendingList2[ppro->pendingListSize2 - 1] = tmp;

	if (ppro->pendingListSize2 == 1)
	{ // queue was empty setup timer
		EnterCriticalSection(&ppro->ratesMutex);
		int nDelay = ppro->m_rates->getDelayToLimitLevel(item->wGroup, RML_IDLE_30);
		LeaveCriticalSection(&ppro->ratesMutex);

		if (nDelay < 10) nDelay = 10;
		ppro->InitDelay(nDelay, &CIcqProto::RatesTimer2);
	}
}

int CIcqProto::handleRateItem(rate_record *item, BOOL bAllowDelay)
{
	if (!m_rates)
		return 0;

	EnterCriticalSection(&ratesListsMutex);

	if (item->nRequestType == 0x101)
	{ // xtraz request
		EnterCriticalSection(&ratesMutex);
		int nLevel = m_rates->getNextRateLevel(item->wGroup);
		int nLimit = m_rates->getLimitLevel(item->wGroup, RML_IDLE_50) + 200;
		LeaveCriticalSection(&ratesMutex);

		if ((nLevel < nLimit || item->nMinDelay) && bAllowDelay)
		{ // limit reached or min delay configured, add to queue
			putItemToQueue1(this, item, nLevel);
			LeaveCriticalSection(&ratesListsMutex);
			return 1;
		}
	}
	else if (item->nRequestType == 0x102)
	{ // msg response
		EnterCriticalSection(&ratesMutex);
		int nLevel = m_rates->getNextRateLevel(item->wGroup);
		int nLimit = m_rates->getLimitLevel(item->wGroup, RML_IDLE_30) + 100;
		LeaveCriticalSection(&ratesMutex);

		if (nLevel < nLimit)
		{ // limit reached, add to queue
			putItemToQueue2(this, item, nLevel);
			LeaveCriticalSection(&ratesListsMutex);
			return 1;
		}
	}
	LeaveCriticalSection(&ratesListsMutex);

	return 0;
}
