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
// File name      : $URL$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


#define LISTSIZE 100
static CRITICAL_SECTION listmutex;
static HANDLE hQueueEvent = NULL;
static int nUserCount = 0;
static int bPendingUsers = 0;
static BOOL bEnabled = FALSE;
static BOOL bRunning = FALSE;
static HANDLE hInfoThread = NULL;
static DWORD dwUpdateThreshold;
typedef struct s_userinfo {
  DWORD dwUin;
  HANDLE hContact;
} userinfo;
static userinfo userList[LISTSIZE];


// Retrieve users' info
unsigned __stdcall icq_InfoUpdateThread(void* arg);



void icq_InitInfoUpdate(void)
{
  int i;
  
  // Create wait objects
  hQueueEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  
  if (hQueueEvent)
  {
    // Init mutexes
    InitializeCriticalSection(&listmutex);
    
    // Init list
    for (i = 0; i<LISTSIZE; i++)
    {
      userList[i].dwUin = 0;
      userList[i].hContact = NULL;
    }

    dwUpdateThreshold = ICQGetContactSettingByte(NULL, "InfoUpdate", UPDATE_THRESHOLD)*3600*24;

    hInfoThread = (HANDLE)forkthreadex(NULL, 0, icq_InfoUpdateThread, 0, 0, (DWORD*)&hInfoThread);
  }

  bPendingUsers = 0;
}

// Returns TRUE if user was queued
// Returns FALSE if the list was full
BOOL icq_QueueUser(HANDLE hContact)
{
  if (nUserCount < LISTSIZE)
  {
    int i, nChecked = 0, nFirstFree = -1;
    BOOL bFound = FALSE;

    EnterCriticalSection(&listmutex);

    // Check if in list
    for (i = 0; (i<LISTSIZE && nChecked<nUserCount); i++)
    {
      if (userList[i].hContact)
      {
        nChecked++;
        if (userList[i].hContact == hContact)
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
      DWORD dwUin = ICQGetContactSettingUIN(hContact);

      if (dwUin)
      {
        userList[nFirstFree].dwUin = dwUin;
        userList[nFirstFree].hContact = hContact;
        nUserCount++;
#ifdef _DEBUG
        NetLog_Server("Queued user %u, place %u, count %u", userList[nFirstFree].dwUin, nFirstFree, nUserCount);
#endif
        // Notify worker thread
        if (hQueueEvent)
          SetEvent(hQueueEvent);
      }
    }

    LeaveCriticalSection(&listmutex);

    return TRUE;
  }

  return FALSE;
}



void icq_DequeueUser(DWORD dwUin)
{
  if (nUserCount > 0) 
  {
    int i, nChecked = 0;
    // Check if in list
    EnterCriticalSection(&listmutex);
    for (i = 0; (i<LISTSIZE && nChecked<nUserCount); i++) 
    {
      if (userList[i].dwUin) 
      {
        nChecked++;
        // Remove from list
        if (userList[i].dwUin == dwUin) 
        {
#ifdef _DEBUG
          NetLog_Server("Dequeued user %u", userList[i].dwUin);
#endif
          userList[i].dwUin = 0;
          userList[i].hContact = NULL;
          nUserCount--;
          break;
        }
      }
    }
    LeaveCriticalSection(&listmutex);
  }
}



void icq_RescanInfoUpdate()
{
  HANDLE hContact = NULL;
  DWORD dwCurrentTime = 0;
  BOOL bOldEnable = bEnabled;

  bPendingUsers = 0;
  /* This is here, cause we do not want to emit large number of reuqest at once,
    fill queue, and let thread deal with it */
  bEnabled = 0; // freeze thread
  // Queue all outdated users
  dwCurrentTime = time(NULL);
  hContact = ICQFindFirstContact();

  while (hContact != NULL)
  {
    if ((dwCurrentTime - ICQGetContactSettingDword(hContact, "InfoTS", 0)) > dwUpdateThreshold)
    {
      // Queue user
      if (!icq_QueueUser(hContact))
      { // The queue is full, pause queuing contacts
        bPendingUsers = 1;
        break; 
      }
    }
    hContact = ICQFindNextContact(hContact);
  }
  icq_EnableUserLookup(bOldEnable); // wake up thread
}



void icq_EnableUserLookup(BOOL bEnable)
{
  bEnabled = bEnable;

  // Notify worker thread
  if (bEnabled && hQueueEvent)
    SetEvent(hQueueEvent);
}



unsigned __stdcall icq_InfoUpdateThread(void* arg)
{
  int i;
  DWORD dwWait;

  bRunning = TRUE;

  while (bRunning)
  {
    // Wait for a while
    ResetEvent(hQueueEvent);

    if (!nUserCount && bPendingUsers) // whole queue processed, check if more users needs updating
      icq_RescanInfoUpdate();

    if (!nUserCount || !bEnabled || !icqOnline)
    {
      dwWait = WAIT_TIMEOUT;
      while (bRunning && dwWait == WAIT_TIMEOUT)
      { // wait for new work or until we should end
        dwWait = WaitForSingleObjectEx(hQueueEvent, 10000, TRUE);
      }
    }
    if (!bRunning) break;

    switch (dwWait) 
    {
    case WAIT_IO_COMPLETION:
      // Possible shutdown in progress
      break;

    case WAIT_OBJECT_0:
    case WAIT_TIMEOUT:
      // Time to check for new users
      if (!bEnabled) continue; // we can't send requests now

#ifdef _DEBUG
      NetLog_Server("Users %u", nUserCount);
#endif
      if (nUserCount && icqOnline)
      {
        EnterCriticalSection(&listmutex);
        for (i = 0; i<LISTSIZE; i++)
        {
          if (userList[i].hContact)
          {
            // Check TS again, maybe it has been updated while we slept
            if ((time(NULL) - ICQGetContactSettingDword(userList[i].hContact, "InfoTS", 0)) > dwUpdateThreshold) 
            {
              WORD wGroup;

              EnterCriticalSection(&ratesMutex);
              if (!gRates)
              { // we cannot send info request - icq is offline
                LeaveCriticalSection(&ratesMutex);
                break;
              }
              wGroup = ratesGroupFromSNAC(gRates, ICQ_EXTENSIONS_FAMILY, ICQ_META_CLI_REQ);
              while (ratesNextRateLevel(gRates, wGroup) < ratesGetLimitLevel(gRates, wGroup, RML_IDLE_50))
              { // we are over rate, need to wait before sending
                int nDelay = ratesDelayToLevel(gRates, wGroup, ratesGetLimitLevel(gRates, wGroup, RML_IDLE_50) + 200);

                LeaveCriticalSection(&ratesMutex);
                LeaveCriticalSection(&listmutex);
#ifdef _DEBUG
                NetLog_Server("Rates: InfoUpdate delayed %dms", nDelay);
#endif
                SleepEx(nDelay, TRUE); // do not keep things locked during sleep
                if (!bRunning)
                { // need to end as fast as possible
                  NetLog_Server("%s thread ended.", "Info-Update");

                  return 0;
                }
                EnterCriticalSection(&listmutex);
                EnterCriticalSection(&ratesMutex);
                if (!gRates) // we lost connection when we slept, go away
                  break;
              }
              if (!gRates)
              { // we cannot send info request - icq is offline
                LeaveCriticalSection(&ratesMutex);
                break;
              }
              LeaveCriticalSection(&ratesMutex);

              if (!userList[i].hContact) continue; // user was dequeued during our sleep
#ifdef _DEBUG
              NetLog_Server("Request info for user %u", userList[i].dwUin);
#endif
              sendUserInfoAutoRequest(userList[i].hContact, userList[i].dwUin);

              // Dequeue user and go back to sleep
              userList[i].dwUin = 0;
              userList[i].hContact = NULL;
              nUserCount--;
              break;
            }
            else
            {
#ifdef _DEBUG
              NetLog_Server("Dequeued absolete user %u", userList[i].dwUin);
#endif
              // Dequeue user and find another one
              userList[i].dwUin = 0;
              userList[i].hContact = NULL;
              nUserCount--;
              // continue for loop
            }
          }
        }
        LeaveCriticalSection(&listmutex);
      }
      break;

    default:
      // Something strange happened. Exit
      bRunning = FALSE;
      break;
    }
  }
  NetLog_Server("%s thread ended.", "Info-Update");

  return 0;
}



// Clean up before exit
void icq_InfoUpdateCleanup(void)
{
  bRunning = FALSE;
  SetEvent(hQueueEvent); // break queue loop
  if (hInfoThread) WaitForSingleObjectEx(hInfoThread, INFINITE, TRUE);
  // Uninit mutex
  DeleteCriticalSection(&listmutex);
  CloseHandle(hQueueEvent);
  CloseHandle(hInfoThread);
}
