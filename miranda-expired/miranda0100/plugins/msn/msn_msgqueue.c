/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-1  Richard Hughes, Roland Rabien & Tristan Van de Vreede

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

//a few little functions to manage queuing send message requests until the
//connection is established

struct MsgQueueEntry {
	HANDLE hContact;
	char *message;
	DWORD flags;
	int seq;
	int allocatedToThread;
};

static struct MsgQueueEntry *msgQueue;
static int msgQueueCount;
static CRITICAL_SECTION csMsgQueue;
static int msgQueueSeq;

void MsgQueue_Init(void)
{
	msgQueueCount=0;
	msgQueue=NULL;
	msgQueueSeq=1;
	InitializeCriticalSection(&csMsgQueue);
}

void MsgQueue_Uninit(void)
{
	if(msgQueueCount) free(msgQueue);
	DeleteCriticalSection(&csMsgQueue);
}

int MsgQueue_Add(HANDLE hContact,const char *msg,DWORD flags)
{
	int seq;

	EnterCriticalSection(&csMsgQueue);
	msgQueue=(struct MsgQueueEntry*)realloc(msgQueue,sizeof(struct MsgQueueEntry)*(msgQueueCount+1));
	msgQueue[msgQueueCount].hContact=hContact;
	msgQueue[msgQueueCount].message=_strdup(msg);
	msgQueue[msgQueueCount].flags=flags;
	seq=msgQueueSeq++;
	msgQueue[msgQueueCount].seq=seq;
	msgQueue[msgQueueCount].allocatedToThread=0;
	msgQueueCount++;
	LeaveCriticalSection(&csMsgQueue);
	return seq;
}

//for threads to determine who they should connect to
HANDLE MsgQueue_GetNextRecipient(void)
{
	int i;
	HANDLE ret;

	EnterCriticalSection(&csMsgQueue);
	ret=NULL;
	for(i=0;i<msgQueueCount;i++)
		if(!msgQueue[i].allocatedToThread) {
			msgQueue[i].allocatedToThread=1;
			ret=msgQueue[i].hContact;
			break;
		}
	LeaveCriticalSection(&csMsgQueue);
	return ret;
}

//deletes from list. Must free() return value
char *MsgQueue_GetNext(HANDLE hContact,PDWORD pFlags,int *seq)
{
	int i;
	char *ret;

	EnterCriticalSection(&csMsgQueue);
	for(i=0;i<msgQueueCount;i++)
		if(msgQueue[i].hContact==hContact) break;
	if(i==msgQueueCount) {
		LeaveCriticalSection(&csMsgQueue);
		return NULL;
	}
	ret=msgQueue[i].message;
	if(pFlags!=NULL) *pFlags=msgQueue[i].flags;
	if(seq!=NULL) *seq=msgQueue[i].seq;
	msgQueueCount--;
	memmove(msgQueue+i,msgQueue+i+1,sizeof(struct MsgQueueEntry)*(msgQueueCount-i));
	msgQueue=(struct MsgQueueEntry*)realloc(msgQueue,sizeof(struct MsgQueueEntry)*msgQueueCount);
	LeaveCriticalSection(&csMsgQueue);
	return ret;
}

int MsgQueue_AllocateUniqueSeq(void)
{
	int seq;

	EnterCriticalSection(&csMsgQueue);
	seq=msgQueueSeq++;
	LeaveCriticalSection(&csMsgQueue);
	return seq;
}