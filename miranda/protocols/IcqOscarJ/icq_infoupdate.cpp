// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001-2002 Jon Keating, Richard Hughes
// Copyright � 2002-2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004-2010 Joe Kucera
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
//  Background thread for automatic update of user details
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


// Retrieve users' info
void CIcqProto::icq_InitInfoUpdate(void)
{
	// Create wait objects
	hInfoQueueEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (hInfoQueueEvent)
	{
		// Init mutexes
		infoUpdateMutex = new icq_critical_section();

		// Init list
		for (int i = 0; i<LISTSIZE; i++)
		{
			m_infoUpdateList[i].dwUin = 0;
			m_infoUpdateList[i].hContact = NULL;
			m_infoUpdateList[i].queued = 0;
		}

		hInfoThread = ForkThreadEx( &CIcqProto::InfoUpdateThread, NULL );
	}

	bInfoPendingUsers = 0;
	dwInfoActiveRequest = 0;
}

// Returns TRUE if user was queued
// Returns FALSE if the list was full
BOOL CIcqProto::icq_QueueUser(HANDLE hContact)
{
	if ( !infoUpdateMutex )
		return FALSE;

	if (nInfoUserCount < LISTSIZE)
	{
		int i, nChecked = 0, nFirstFree = -1;
		BOOL bFound = FALSE;

		infoUpdateMutex->Enter();

		// Check if in list
		for (i = 0; (i<LISTSIZE && nChecked < nInfoUserCount); i++)
		{
			if (m_infoUpdateList[i].hContact)
			{
				nChecked++;
				if (m_infoUpdateList[i].hContact == hContact)
				{
					bFound = TRUE;
					break;
				}
			}
			else if (nFirstFree == -1)
			{
				nFirstFree = i;
			}
		}
		if (nFirstFree == -1)
			nFirstFree = i;

		// Add to list
		if (!bFound)
		{
			DWORD dwUin = getContactUin(hContact);

			if (dwUin)
			{
				m_infoUpdateList[nFirstFree].dwUin = dwUin;
				m_infoUpdateList[nFirstFree].hContact = hContact;
				m_infoUpdateList[nFirstFree].queued = time(NULL);
				nInfoUserCount++;
#ifdef _DEBUG
				NetLog_Server("Queued user %u, place %u, count %u", dwUin, nFirstFree, nInfoUserCount);
#endif
				// Notify worker thread
				if (hInfoQueueEvent && bInfoUpdateEnabled)
					SetEvent(hInfoQueueEvent);
			}
		}

		infoUpdateMutex->Leave();

		return TRUE;
	}

	return FALSE;
}


void CIcqProto::icq_DequeueUser(DWORD dwUin)
{
	if (nInfoUserCount > 0) 
	{
		int nChecked = 0;
		// Check if in list
		infoUpdateMutex->Enter();
		for (int i = 0; (i < LISTSIZE && nChecked < nInfoUserCount); i++) 
		{
			if (m_infoUpdateList[i].dwUin) 
			{
				nChecked++;
				// Remove from list
				if (m_infoUpdateList[i].dwUin == dwUin) 
				{
#ifdef _DEBUG
					NetLog_Server("Dequeued user %u", m_infoUpdateList[i].dwUin);
#endif
					m_infoUpdateList[i].dwUin = 0;
					m_infoUpdateList[i].hContact = NULL;
					m_infoUpdateList[i].queued = 0;
					nInfoUserCount--;
					break;
				}
			}
		}
		infoUpdateMutex->Leave();
	}
}


void CIcqProto::icq_RescanInfoUpdate()
{
	HANDLE hContact = NULL;
	BOOL bOldEnable = bInfoUpdateEnabled;

	bInfoPendingUsers = 0;
	/* This is here, cause we do not want to emit large number of reuqest at once,
	fill queue, and let thread deal with it */
	bInfoUpdateEnabled = 0; // freeze thread
	// Queue all outdated users
	hContact = FindFirstContact();

	while (hContact != NULL)
	{
		if (IsMetaInfoChanged(hContact))
		{ // Queue user
			if (!icq_QueueUser(hContact))
			{ // The queue is full, pause queuing contacts
				bInfoPendingUsers = 1;
				break; 
			}
		}
		hContact = FindNextContact(hContact);
	}
	icq_EnableUserLookup(bOldEnable); // wake up thread
}


void CIcqProto::icq_EnableUserLookup(BOOL bEnable)
{
	bInfoUpdateEnabled = bEnable;

	// Notify worker thread
	if (bInfoUpdateEnabled && hInfoQueueEvent)
		SetEvent(hInfoQueueEvent);
}


void __cdecl CIcqProto::InfoUpdateThread( void* )
{
	int i;
	DWORD dwWait = WAIT_OBJECT_0;

	NetLog_Server("%s thread starting.", "Info-Update");

	bInfoUpdateRunning = TRUE;

	while (bInfoUpdateRunning)
	{
		// Wait for a while
		ResetEvent(hInfoQueueEvent);

		if (!nInfoUserCount && bInfoPendingUsers) // whole queue processed, check if more users needs updating
			icq_RescanInfoUpdate();

		if (!nInfoUserCount || !bInfoUpdateEnabled || !icqOnline())
		{
			dwWait = WAIT_TIMEOUT;
			while (bInfoUpdateRunning && dwWait == WAIT_TIMEOUT)
			{ // wait for new work or until we should end
				dwWait = ICQWaitForSingleObject(hInfoQueueEvent, 10000);
			}
		}
		if (!bInfoUpdateRunning) break;

		switch (dwWait) {
		case WAIT_IO_COMPLETION:
			// Possible shutdown in progress
			break;

		case WAIT_OBJECT_0:
		case WAIT_TIMEOUT:
			// Time to check for new users
			if (!bInfoUpdateEnabled) continue; // we can't send requests now      

			if (nInfoUserCount && icqOnline())
			{
				time_t now = time(NULL);
				BOOL bNotReady = FALSE, bTimeOuted = FALSE;

				// Check the list, take only users that were there for at least 5sec
				// wait if any user is there shorter than 5sec and not a single user is there longer than 20sec
				infoUpdateMutex->Enter();
				for (i = 0; i<LISTSIZE; i++)
				{
					if (m_infoUpdateList[i].hContact)
					{
						if (m_infoUpdateList[i].queued + 20 < now)
						{
							bTimeOuted = TRUE;
							break;
						}
						else if (m_infoUpdateList[i].queued + 5 >= now)
							bNotReady = TRUE;
					}
				}
				infoUpdateMutex->Leave();

				if (!bTimeOuted && bNotReady)
				{
					SleepEx(1000, TRUE);
					if (!bInfoUpdateRunning)
					{ // need to end as fast as possible
						NetLog_Server("%s thread ended.", "Info-Update");
						return;
					}
					continue;
				}

				if (FindCookie(dwInfoActiveRequest, NULL, NULL))
				{ // only send another request, when the previous is completed
#ifdef _DEBUG
					NetLog_Server("Info-Update: Request 0x%x still in progress.", dwInfoActiveRequest);
#endif
					SleepEx(1000, TRUE);
					if (!bInfoUpdateRunning)
					{ // need to end as fast as possible
						NetLog_Server("%s thread ended.", "Info-Update");
						return;
					}
					continue;
				}

#ifdef _DEBUG
				NetLog_Server("Info-Update: Users %u in queue.", nInfoUserCount);
#endif
				// Either some user is waiting long enough, or all users are ready (waited at least the minimum time)
				m_ratesMutex->Enter();
				if (!m_rates)
				{ // we cannot send info request - icq is offline
					m_ratesMutex->Leave();
					break;
				}
				WORD wGroup = m_rates->getGroupFromSNAC(ICQ_EXTENSIONS_FAMILY, ICQ_META_CLI_REQUEST);
				while (m_rates->getNextRateLevel(wGroup) < m_rates->getLimitLevel(wGroup, RML_IDLE_30))
				{ // we are over rate, need to wait before sending
					int nDelay = m_rates->getDelayToLimitLevel(wGroup, RML_IDLE_50);

					m_ratesMutex->Leave();
#ifdef _DEBUG
					NetLog_Server("Rates: InfoUpdate delayed %dms", nDelay);
#endif
					SleepEx(nDelay, TRUE); // do not keep things locked during sleep
					if (!bInfoUpdateRunning)
					{ // need to end as fast as possible
						NetLog_Server("%s thread ended.", "Info-Update");
						return;
					}
					m_ratesMutex->Enter();
					if (!m_rates) // we lost connection when we slept, go away
						break;
				}
				if (!m_rates)
				{ // we cannot send info request - icq is offline
					m_ratesMutex->Leave();
					break;
				}
				m_ratesMutex->Leave();

				userinfo *hContactList[LISTSIZE];
				int nListIndex = 0;
				BYTE *pRequestData = NULL;
				int nRequestSize = 0;

				infoUpdateMutex->Enter();
				for (i = 0; i<LISTSIZE; i++)
				{
					if (m_infoUpdateList[i].hContact)
					{
						// check TS again, maybe it has been updated while we slept
						if (IsMetaInfoChanged(m_infoUpdateList[i].hContact))
						{
							if (m_infoUpdateList[i].queued + 5 < now)
							{
								BYTE *pItem = NULL;
								int nItemSize = 0;
								DBVARIANT dbv = {DBVT_DELETED};

								if (!getSetting(m_infoUpdateList[i].hContact, DBSETTING_METAINFO_TOKEN, &dbv))
								{ // retrieve user details using privacy token
									ppackTLV(&pItem, &nItemSize, 0x96, dbv.cpbVal, dbv.pbVal);
									ICQFreeVariant(&dbv);
								}
								// last updated time
								ppackTLVDouble(&pItem, &nItemSize, 0x64, getSettingDouble(m_infoUpdateList[i].hContact, DBSETTING_METAINFO_TIME, 0));

								ppackTLVUID(&pItem, &nItemSize, 0x32, m_infoUpdateList[i].dwUin, NULL);
								ppackWord(&pRequestData, &nRequestSize, (WORD)nItemSize);
								ppackBuffer(&pRequestData, &nRequestSize, nItemSize, pItem);
								// take a reference
								SAFE_FREE((void**)&pItem);
								hContactList[nListIndex++] = &m_infoUpdateList[i];
							}
						}
						else
						{
#ifdef _DEBUG
							NetLog_Server("Dequeued absolete user %u", m_infoUpdateList[i].dwUin);
#endif
							// Dequeue user and find another one
							m_infoUpdateList[i].dwUin = 0;
							m_infoUpdateList[i].hContact = NULL;
							nInfoUserCount--;
							// continue for loop
						}
					}
				}

#ifdef _DEBUG
				NetLog_Server("Request info for %u user(s).", nListIndex);
#endif
				if (!nListIndex)
				{ // no users to request info for
					infoUpdateMutex->Leave();
					break;
				}
				if (!(dwInfoActiveRequest = sendUserInfoMultiRequest(pRequestData, nRequestSize, nListIndex)))
				{ // sending data packet failed
					SAFE_FREE((void**)&pRequestData);
					infoUpdateMutex->Leave();
					break;
				}
				SAFE_FREE((void**)&pRequestData);

				for (i = 0; i<nListIndex; i++)
				{ // Dequeue users and go back to sleep
					hContactList[i]->dwUin = 0;
					hContactList[i]->hContact = NULL;
					hContactList[i]->queued = 0;
					nInfoUserCount--;
				}
				infoUpdateMutex->Leave();
			}
			break;

		default:
			// Something strange happened. Exit
			bInfoUpdateRunning = FALSE;
			break;
		}
	}
	NetLog_Server("%s thread ended.", "Info-Update");
}


// Clean up before exit
void CIcqProto::icq_InfoUpdateCleanup(void)
{
	bInfoUpdateRunning = FALSE;
	SetEvent(hInfoQueueEvent); // break queue loop
	if (hInfoThread) ICQWaitForSingleObject(hInfoThread, INFINITE, TRUE);
	// Uninit mutex
	SAFE_DELETE(&infoUpdateMutex);
	CloseHandle(hInfoQueueEvent);
	CloseHandle(hInfoThread);
}
