/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-2  Richard Hughes, Roland Rabien & Tristan Van de Vreede

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "msn_global.h"

/*This is just stuff for coordinating switchboard threads, the actual threads
are in msn_threads.c and msn_commands.c
*/

struct SwitchboardData {
	int status;
	DWORD threadId;
	SOCKET s;
	HANDLE *hContacts;
	int contactCount;
};

static struct SwitchboardData *switchboard;
static int switchboardCount;
static CRITICAL_SECTION csSwitchboardData;

void Switchboards_Init(void)
{
	InitializeCriticalSection(&csSwitchboardData);
	switchboardCount=0;
	switchboard=NULL;
}

void Switchboards_Uninit(void)
{
	if(switchboard) free(switchboard);
	DeleteCriticalSection(&csSwitchboardData);
}

/********** Routines for threads *************/

static int FindThisSwitchboard(void)
{
	int i;
	DWORD threadId=GetCurrentThreadId();
	for(i=0;i<switchboardCount;i++)
		if(switchboard[i].threadId==threadId) return i;
	return -1;  //never happens
}

void Switchboards_New(SOCKET s)
{
	EnterCriticalSection(&csSwitchboardData);
	switchboard=(struct SwitchboardData*)realloc(switchboard,sizeof(struct SwitchboardData)*(switchboardCount+1));
	switchboard[switchboardCount].threadId=GetCurrentThreadId();
	switchboard[switchboardCount].s=s;
	switchboard[switchboardCount].status=SBSTATUS_NEW;
	switchboard[switchboardCount].contactCount=0;
	switchboard[switchboardCount].hContacts=NULL;
	switchboardCount++;
	LeaveCriticalSection(&csSwitchboardData);
}

void Switchboards_Delete(void)
{
	int i;

	EnterCriticalSection(&csSwitchboardData);
	if((i=FindThisSwitchboard())==-1) {LeaveCriticalSection(&csSwitchboardData); return;}
	if(switchboard[i].contactCount) free(switchboard[i].hContacts);
	switchboardCount--;
	memmove(switchboard+i,switchboard+i+1,sizeof(struct SwitchboardData)*(switchboardCount-i));
	switchboard=(struct SwitchboardData*)realloc(switchboard,sizeof(struct SwitchboardData)*switchboardCount);
	LeaveCriticalSection(&csSwitchboardData);
}

void Switchboards_ChangeStatus(int newStatus)
{
	int i;

	EnterCriticalSection(&csSwitchboardData);
	if((i=FindThisSwitchboard())==-1) {LeaveCriticalSection(&csSwitchboardData); return;}
	switchboard[i].status=newStatus;
	LeaveCriticalSection(&csSwitchboardData);
}

int Switchboards_ContactJoined(HANDLE hContact)
{
	int i;

	EnterCriticalSection(&csSwitchboardData);
	if((i=FindThisSwitchboard())==-1) {LeaveCriticalSection(&csSwitchboardData); return 0;}
	switchboard[i].hContacts=(HANDLE*)realloc(switchboard[i].hContacts,sizeof(HANDLE)*(switchboard[i].contactCount+1));
	switchboard[i].hContacts[switchboard[i].contactCount++]=hContact;
	LeaveCriticalSection(&csSwitchboardData);
	return i+1;
}

//returns the number of contacts in the channel after the change
int Switchboards_ContactLeft(HANDLE hContact)
{
	int i,j,ret;

	EnterCriticalSection(&csSwitchboardData);
	if((i=FindThisSwitchboard())==-1) {LeaveCriticalSection(&csSwitchboardData); return 0;}
	for(j=0;j<switchboard[i].contactCount;j++)
		if(switchboard[i].hContacts[j]==hContact) break;
	if(j==switchboard[i].contactCount) {
		ret=switchboard[i].contactCount;
		LeaveCriticalSection(&csSwitchboardData);
		return ret;
	}
	ret=--switchboard[i].contactCount;
	memmove(switchboard[i].hContacts+j,switchboard[i].hContacts+j+1,sizeof(HANDLE)*(switchboard[i].contactCount-j));
	switchboard[i].hContacts=(HANDLE*)realloc(switchboard[i].hContacts,sizeof(HANDLE)*switchboard[i].contactCount);
	LeaveCriticalSection(&csSwitchboardData);
	return ret;
}

/********** Routines for main thread *************/

SOCKET Switchboards_SocketFromHContact(HANDLE hContact)
{
	SOCKET s=SOCKET_ERROR;
	int i;

	EnterCriticalSection(&csSwitchboardData);
	for(i=0;i<switchboardCount;i++) {
		if(switchboard[i].contactCount!=1) continue;
		if(switchboard[i].hContacts[0]==hContact) {s=switchboard[i].s; break;}
	}
	LeaveCriticalSection(&csSwitchboardData);
	return s;
}