/* 
Copyright (C) 2006 Ricardo Pescuma Domenecci, Nightwish

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  
*/

#include "commonheaders.h"


/*
It has 1 queue:
A queue to request items. One request is done at a time, REQUEST_WAIT_TIME miliseconts after it has beeing fired
   ACKRESULT_STATUS. This thread only requests the avatar (and maybe add it to the cache queue)
*/

#define REQUEST_WAIT_TIME 3000

// Time to wait before re-requesting an avatar that failed
#define REQUEST_FAIL_WAIT_TIME (3 * 60 * 60 * 1000)

// Time to wait before re-requesting an avatar that received an wait for
#define REQUEST_WAITFOR_WAIT_TIME (30 * 60 * 1000)

// Number of mileseconds the threads wait until take a look if it is time to request another item
#define POOL_DELAY 1000

// Number of mileseconds the threads wait after a GAIR_WAITFOR is returned
#define REQUEST_DELAY 18000


// Prototypes ///////////////////////////////////////////////////////////////////////////

ThreadQueue requestQueue = {0};

static void RequestThread(void *vParam);

extern HANDLE hShutdownEvent;
extern char *g_szMetaName;
extern int ChangeAvatar(HANDLE hContact, BOOL fLoad, BOOL fNotifyHist = FALSE, int pa_format = 0);
extern int DeleteAvatar(HANDLE hContact);
extern void MakePathRelative(HANDLE hContact, char *path);
int Proto_GetDelayAfterFail(const char *proto);

struct CacheNode *FindAvatarInCache(HANDLE hContact, BOOL add, BOOL findAny = FALSE);

extern HANDLE hEventContactAvatarChanged;
extern BOOL g_AvatarHistoryAvail;
extern FI_INTERFACE *fei;

#ifdef _DEBUG
int _DebugTrace(const char *fmt, ...);
int _DebugTrace(HANDLE hContact, const char *fmt, ...);
#endif


// Functions ////////////////////////////////////////////////////////////////////////////


// Itens with higher priority at end
static int QueueSortItems(void *i1, void *i2)
{
	return ((QueueItem*)i2)->check_time - ((QueueItem*)i1)->check_time;
}


void InitPolls() 
{
	// Init request queue
	ZeroMemory(&requestQueue, sizeof(requestQueue));
	requestQueue.queue = List_Create(0, 20);
	requestQueue.queue->sortFunc = QueueSortItems;
	requestQueue.waitTime = REQUEST_WAIT_TIME;
    InitializeCriticalSection(&requestQueue.cs);
	mir_forkthread(RequestThread, NULL);
}


void FreePolls()
{
}


// Return true if this protocol can have avatar requested
static BOOL PollProtocolCanHaveAvatar(const char *szProto)
{
	int pCaps = CallProtoService(szProto, PS_GETCAPS, PFLAGNUM_4, 0);
	int status = CallProtoService(szProto, PS_GETSTATUS, 0, 0);
	return (pCaps & PF4_AVATARS)
		&& (g_szMetaName == NULL || strcmp(g_szMetaName, szProto))
		&& status > ID_STATUS_OFFLINE && status != ID_STATUS_INVISIBLE;
}

// Return true if this protocol has to be checked
static BOOL PollCheckProtocol(const char *szProto)
{
	return DBGetContactSettingByte(NULL, AVS_MODULE, szProto, 1);
}

// Return true if this contact can have avatar requested
static BOOL PollContactCanHaveAvatar(HANDLE hContact, const char *szProto)
{
	int status = DBGetContactSettingWord(hContact, szProto, "Status", ID_STATUS_OFFLINE);
	return status != ID_STATUS_OFFLINE
		&& !DBGetContactSettingByte(hContact, "CList", "NotOnList", 0)
		&& DBGetContactSettingByte(hContact, "CList", "ApparentMode", 0) != ID_STATUS_OFFLINE;
}

// Return true if this contact has to be checked
static BOOL PollCheckContact(HANDLE hContact, const char *szProto)
{
	return !DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0)
            && FindAvatarInCache(hContact, FALSE, TRUE) != NULL;
}

static void QueueRemove(ThreadQueue &queue, HANDLE hContact)
{
    EnterCriticalSection(&queue.cs);

	if (queue.queue->items != NULL)
	{
		for (int i = queue.queue->realCount - 1 ; i >= 0 ; i-- )
		{
			QueueItem *item = (QueueItem*) queue.queue->items[i];
			
			if (item->hContact == hContact)
			{
				mir_free(item);
				List_Remove(queue.queue, i);
			}
		}
	}

	LeaveCriticalSection(&queue.cs);
}

static void QueueAdd(ThreadQueue &queue, HANDLE hContact, int waitTime)
{
    if(fei == NULL)
        return;

    EnterCriticalSection(&queue.cs);

	// Only add if not exists yet
	int i;
	for (i = queue.queue->realCount - 1; i >= 0; i--)
	{
		if (((QueueItem *) queue.queue->items[i])->hContact == hContact)
			break;
	}

	if (i < 0)
	{
		QueueItem *item = (QueueItem *) mir_alloc0(sizeof(QueueItem));

		if (item != NULL) 
		{
			item->hContact = hContact;
			item->check_time = GetTickCount() + waitTime;

			List_InsertOrdered(queue.queue, item);
		}
	}

	LeaveCriticalSection(&queue.cs);
}

// Add an contact to a queue
void QueueAdd(ThreadQueue &queue, HANDLE hContact)
{
	QueueAdd(queue, hContact, queue.waitTime);
}

void ProcessAvatarInfo(HANDLE hContact, int type, PROTO_AVATAR_INFORMATION *pai, const char *szProto)
{
	QueueRemove(requestQueue, hContact);

	if (type == GAIR_SUCCESS) 
	{
		if (pai == NULL)
			return;

		// Fix settings in DB
		DBDeleteContactSetting(hContact, "ContactPhoto", "NeedUpdate");
		DBDeleteContactSetting(hContact, "ContactPhoto", "RFile");
		if (!DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0))
			DBDeleteContactSetting(hContact, "ContactPhoto", "Backup");
		DBWriteContactSettingString(hContact, "ContactPhoto", "File", pai->filename);
		DBWriteContactSettingWord(hContact, "ContactPhoto", "Format", pai->format);

		if (pai->format == PA_FORMAT_PNG || pai->format == PA_FORMAT_JPEG 
			|| pai->format == PA_FORMAT_ICON  || pai->format == PA_FORMAT_BMP
			|| pai->format == PA_FORMAT_GIF)
		{
			// We can load it!
			MakePathRelative(hContact, pai->filename);
			ChangeAvatar(hContact, TRUE, TRUE, pai->format);
		}
		else
		{
			// As we can't load it, notify as deleted
			ChangeAvatar(hContact, FALSE, TRUE, pai->format);
		}
	}
	else if (type == GAIR_NOAVATAR) 
	{
		DBDeleteContactSetting(hContact, "ContactPhoto", "NeedUpdate");

		if (DBGetContactSettingByte(NULL, AVS_MODULE, "RemoveAvatarWhenContactRemoves", 1)) 
		{
			// Delete settings
			DBDeleteContactSetting(hContact, "ContactPhoto", "RFile");
			if (!DBGetContactSettingByte(hContact, "ContactPhoto", "Locked", 0))
				DBDeleteContactSetting(hContact, "ContactPhoto", "Backup");
			DBDeleteContactSetting(hContact, "ContactPhoto", "File");
			DBDeleteContactSetting(hContact, "ContactPhoto", "Format");

			// Fix cache
			ChangeAvatar(hContact, FALSE, TRUE, 0);
		}
	}
	else if (type == GAIR_FAILED) 
	{
		int wait = Proto_GetDelayAfterFail(szProto);
		if (wait > 0)
		{
			// Reschedule to request after needed time (and avoid requests before that)
			EnterCriticalSection(&requestQueue.cs);
			QueueRemove(requestQueue, hContact);
			QueueAdd(requestQueue, hContact, wait);
			LeaveCriticalSection(&requestQueue.cs);
		}
	}
}

int FetchAvatarFor(HANDLE hContact, char *szProto = NULL)
{
	int result = GAIR_NOAVATAR;

	if (szProto == NULL)
		szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);

    if (szProto != NULL && PollProtocolCanHaveAvatar(szProto) && PollContactCanHaveAvatar(hContact, szProto))
	{
		// Can have avatar, but must request it?
		if (
			(g_AvatarHistoryAvail && CallService(MS_AVATARHISTORY_ENABLED, (WPARAM) hContact, 0))
			 || (PollCheckProtocol(szProto) && PollCheckContact(hContact, szProto))
			)
		{
			// Request it
			PROTO_AVATAR_INFORMATION pai_s = {0};
			pai_s.cbSize = sizeof(pai_s);
			pai_s.hContact = hContact;
			pai_s.format = PA_FORMAT_UNKNOWN;
            //_DebugTrace(hContact, "schedule request");
			result = CallProtoService(szProto, PS_GETAVATARINFO, GAIF_FORCE, (LPARAM)&pai_s);
			ProcessAvatarInfo(pai_s.hContact, result, &pai_s, szProto);
		}
	}

	return result;
}


static void RequestThread(void *vParam)
{
	while (!g_shutDown)
	{
		EnterCriticalSection(&requestQueue.cs);

		if (!List_HasItens(requestQueue.queue))
		{
			// No items, so supend thread
			LeaveCriticalSection(&requestQueue.cs);

			mir_sleep(POOL_DELAY);
		}
		else
		{
			// Take a look at first item
			QueueItem *qi = (QueueItem *) List_Peek(requestQueue.queue);

			if (qi->check_time > GetTickCount()) 
			{
				// Not time to request yeat, wait...
				LeaveCriticalSection(&requestQueue.cs);

				mir_sleep(POOL_DELAY);
			}
			else
			{
				// Will request this item
				qi = (QueueItem *) List_Pop(requestQueue.queue);
				HANDLE hContact = qi->hContact;
				mir_free(qi);

				QueueRemove(requestQueue, hContact);

                LeaveCriticalSection(&requestQueue.cs);

				if (FetchAvatarFor(hContact) == GAIR_WAITFOR)
				{
					// Mark to not request this contact avatar for more 30 min
				    EnterCriticalSection(&requestQueue.cs);
					QueueRemove(requestQueue, hContact);
					QueueAdd(requestQueue, hContact, REQUEST_WAITFOR_WAIT_TIME);
					LeaveCriticalSection(&requestQueue.cs);

					// Wait a little until requesting again
					mir_sleep(REQUEST_DELAY);
				}
			}
		}
	}

	DeleteCriticalSection(&requestQueue.cs);
	List_DestroyFreeContents(requestQueue.queue);
	mir_free(requestQueue.queue);
}
