// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004,2005 Martin Öberg, Sam Kothari, Robert Rainwater
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
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"



#define LISTSIZE 100
static CRITICAL_SECTION listmutex;
static HANDLE hQueueEvent = NULL;
static HANDLE hDummyEvent = NULL;
static int nUserCount = 0;
static BOOL bEnabled = TRUE;
typedef struct s_userinfo {
	DWORD dwUin;
	HANDLE hContact;
} userinfo;
static userinfo userList[LISTSIZE];

extern HANDLE ghServerNetlibUser;
extern char gpszICQProtoName[MAX_PATH];
extern int gnCurrentStatus;



// Retrieve users' info
void __cdecl icq_InfoUpdateThread(void* arg);



void icq_InitInfoUpdate(void)
{

	int i;
	

	// Create wait objects
	hQueueEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	hDummyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	if (hQueueEvent && hDummyEvent)
	{

		// Init mutexes
		InitializeCriticalSection(&listmutex);
		
		// Init list
		for (i = 0; i<LISTSIZE; i++)
		{
			userList[i].dwUin = 0;
			userList[i].hContact = NULL;
		}
		
		forkthread(icq_InfoUpdateThread, 0, 0);

	}

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
			userList[nFirstFree].dwUin = DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0);
			userList[nFirstFree].hContact = hContact;
			nUserCount++;
#ifdef _DEBUG
			Netlib_Logf(ghServerNetlibUser, "Queued user %u, place %u, count %u", userList[nFirstFree].dwUin, nFirstFree, nUserCount);
#endif
			// Notify worker thread
			if (hQueueEvent)
				SetEvent(hQueueEvent);
		}

		LeaveCriticalSection(&listmutex);

		return TRUE;

	}

	return FALSE;

}



void icq_DequeueUser(DWORD dwUin)
{

	if (nUserCount > 0) {
		int i, nChecked = 0;
		// Check if in list
		EnterCriticalSection(&listmutex);
		for (i = 0; (i<LISTSIZE && nChecked<nUserCount); i++) {
			if (userList[i].dwUin) {
				nChecked++;
				// Remove from list
				if (userList[i].dwUin == dwUin) {
#ifdef _DEBUG
					Netlib_Logf(ghServerNetlibUser, "Dequeued user %u", userList[i].dwUin);
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
	char* szProto = NULL;
	DWORD dwUin = 0;
	DWORD dwCurrentTime = 0;

	// Queue all outdated users
	dwCurrentTime = time(NULL);
	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

	while (hContact != NULL)
	{
		szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (szProto != NULL && !strcmp(szProto, gpszICQProtoName))
		{
			if ((dwCurrentTime - DBGetContactSettingDword(hContact, gpszICQProtoName, "InfoTS", 0)) > UPDATE_THRESHOLD)
			{
				dwUin = DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0);
				// Queue user
				if (!icq_QueueUser(hContact))
					break; // Early exit when list is full
			}
		}
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
	}

}



void icq_EnableUserLookup(BOOL bEnable)
{

	bEnabled = bEnable;

	// Notify worker thread
	if (bEnabled && hQueueEvent)
		SetEvent(hQueueEvent);

}



void __cdecl icq_InfoUpdateThread(void* arg)
{

	int i;
	DWORD dwWait;
	BOOL bKeepRunning = TRUE;
	

	while (bKeepRunning)
	{

		// Wait for a while
		ResetEvent(hQueueEvent);

		if ((nUserCount > 0) && bEnabled && icqOnline)
			dwWait = WaitForSingleObjectEx(hDummyEvent, 2500, TRUE);
		else
			dwWait = WaitForSingleObjectEx(hQueueEvent, INFINITE, TRUE);

		switch (dwWait) {
		case WAIT_IO_COMPLETION:
			// Possible shutdown in progress
			if (Miranda_Terminated())
				bKeepRunning = FALSE;
			break;
			
		case WAIT_OBJECT_0:
		case WAIT_TIMEOUT:
			// Time to check for new users
#ifdef _DEBUG
			Netlib_Logf(ghServerNetlibUser, "Users %u", nUserCount);
#endif
			if (nUserCount > 0 && icqOnline)
			{
				EnterCriticalSection(&listmutex);
				for (i = 0; i<LISTSIZE; i++)
				{
					if (userList[i].hContact)
					{
						// Check TS again, maybe it has been updated while we slept
						if ((time(NULL) - DBGetContactSettingDword(userList[i].hContact, gpszICQProtoName, "InfoTS", 0)) > UPDATE_THRESHOLD) {
#ifdef _DEBUG
							Netlib_Logf(ghServerNetlibUser, "Request info for user %u", userList[i].dwUin);
#endif
							sendUserInfoAutoRequest(userList[i].dwUin);

							// Dequeue user and go back to sleep
							userList[i].dwUin = 0;
							userList[i].hContact = NULL;
							nUserCount--;
							break;
						}
						else
						{
#ifdef _DEBUG
							Netlib_Logf(ghServerNetlibUser, "Dequeued absolete user %u", userList[i].dwUin);
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
			bKeepRunning = FALSE;
			break;
		}

	}

	return;

}



// Clean up before exit
void icq_InfoUpdateCleanup(void)
{
	// Uninit mutex
	DeleteCriticalSection(&listmutex);
	CloseHandle(hQueueEvent);
	CloseHandle(hDummyEvent);

}
