/*

Copyright 2005 Sam Kothari (egoDust)

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

#include "itr.h"

void ITR_NewWorker_Proxy(void * p)
{
	MSG msg;
	ITRWORKER * w = (ITRWORKER *) p;
	ITRWORKERFUNCTION workerFunction = w->workerFunction;
	HANDLE hHeap = w->hHeap;
	void * pExtra = w->pExtra;
	CallService(MS_SYSTEM_THREAD_PUSH, 0, 0);
	w->dwThreadId=GetCurrentThreadId();
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(w->hStartEvent);
	workerFunction(hHeap, pExtra);
	CallService(MS_SYSTEM_THREAD_POP, 0, 0);
}

// returns 1 if the message was posted
int ITR_PostEvent(ITRWORKER * w, UINT msg, WPARAM a, LPARAM b)
{
	if ( PostThreadMessage(w->dwThreadId, msg, a, b) == 0 ) {
		return 0;
	}
	return 1;
}

void * ITR_Alloc(ITRWORKER * w, size_t nBytes)
{
	// might return NULL
	return HeapAlloc(w->hHeap, 0, nBytes);
}

BOOL ITR_ActiveWorker(ITRWORKER * w)
{
	return w->dwThreadId != 0 && w->hHeap !=NULL;
}

void ITR_NewWorker(ITRWORKER * w, ITRWORKERFUNCTION wFunc, void * pExtra)
{
	// Create everything on the main side of the convo, then wait for the thread to be created
	ZeroMemory(w, sizeof(ITRWORKER));
	w->hStartEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
	w->hHeap=HeapCreate(0, 0, 0);
	w->workerFunction=wFunc;	
	w->pExtra=pExtra;
	_beginthread(ITR_NewWorker_Proxy, 0, (void *)w);
	WaitForSingleObject(w->hStartEvent, INFINITE);
	CloseHandle(w->hStartEvent);
}

void ITR_FreeWorker(ITRWORKER * w)
{
	ITR_PostEvent(w, WM_ITR_QUIT, 0, 0);
	ZeroMemory(w, sizeof(ITRWORKER));
}




#if 0

// None of this code is used, its old and left here for reference

void ITR_new(ITRQ * itr)
{
	memset(itr, 0, sizeof(ITRQ));
	InitializeCriticalSection(&itr->lock);
}

void ITR_delete(ITRQ * itr)
{
	ITR_IDQ * idq = 0;
	EnterCriticalSection(&itr->lock);
	idq = itr->threadQ;
	while ( idq != 0 ) {
		ITR_IDQ * q = idq->next;
		CloseHandle(idq->hThread);
		free(idq);
		idq = q;
	}
	itr->threadQ = 0;
	LeaveCriticalSection(&itr->lock);
	DeleteCriticalSection(&itr->lock);
	memset(itr, 0, sizeof(ITRQ));
}

ITR_ID ITR_register(ITRQ * itr, void (__stdcall * code) (void *), void * arg)
{
	HANDLE h = 0;
	ITR_ID tr = 0;
	ITR_IDQ * idq = 0;
	idq = malloc( sizeof(ITR_IDQ) );
	if (idq == NULL) return 0;
	h=(HANDLE)-1;
	//h = (HANDLE) _beginthreadex(NULL, 0, code, arg, 0, &tr);
	if ( (int)h == -1 ) return 0;
	EnterCriticalSection(&itr->lock);
	idq->next = itr->threadQ;
	idq->hThread = h;
	idq->id = tr;
	itr->threadQ = idq; // new head
	LeaveCriticalSection(&itr->lock);
	return tr;
}

void ITR_unregister(ITRQ * itr, ITR_ID id)
{
	ITR_IDQ * p = 0, * q = 0, * z = 0;
	EnterCriticalSection(&itr->lock);
	p = itr->threadQ;
	while ( p != 0 ) {
		q = p->next;
		if ( p->id == id ) {
			if ( z != 0 ) z->next = p->next;
			if ( p == itr->threadQ ) itr->threadQ = p->next;
			LeaveCriticalSection(&itr->lock);
			CloseHandle(p->hThread);
			p->id=0;
			free(p);
			return;
		}
		z = p;
		p = q;
	}
	LeaveCriticalSection(&itr->lock);
}

int ITR_push(ITRQ * itr, ITR_ID id, ITR_MSG * msg)
{
	ITR_FIFO * p = 0;
	p = malloc( sizeof(ITR_FIFO) );
	if ( p == 0 ) return 0;
	memset(p,0,sizeof(ITR_FIFO));
	p->id = id;
	memmove(&p->msg, msg, sizeof(ITR_MSG));
	EnterCriticalSection(&itr->lock);
	if ( itr->firstPush == 0 ) itr->firstPush = p;
	if ( itr->lastPush != 0 ) itr->lastPush->next = p;
	p->prev = itr->lastPush;
	itr->lastPush = p;
	LeaveCriticalSection(&itr->lock);
	return 1;
}

int ITR_pop(ITRQ * itr, ITR_ID id, ITR_MSG * msg)
{
	ITR_FIFO * p = 0;
	EnterCriticalSection(&itr->lock);
	p = itr->firstPush;
	while ( p != 0 ) {
		if ( p->id == id ) {			
			// remove this item from the link list
			if ( p->prev != 0 ) p->prev->next = p->next;
			if ( p->next != 0 ) p->next->prev = p->prev;
			if ( itr->firstPush == p ) itr->firstPush = p->next;
			if ( itr->lastPush == p ) itr->lastPush = p->prev;			
			LeaveCriticalSection(&itr->lock);			
			memmove(msg, &p->msg, sizeof(ITR_MSG));
			free(p);
			return 1;
		}
		p = p->next;
	}
	LeaveCriticalSection(&itr->lock);
	return 0;
}

void ITR_waitfor(ITRQ * itr)
{
	ITR_IDQ * idq = 0;	
	HANDLE hThread[64]= {0};
	int nThreads = 0;

	EnterCriticalSection(&itr->lock);
	idq = itr->threadQ;
	while ( idq != 0 ) {
		hThread[nThreads++] = idq->hThread;
		idq = idq->next;
	}
	LeaveCriticalSection(&itr->lock);
	WaitForMultipleObjects(nThreads, &hThread[0], TRUE, INFINITE);
}

int ITR_event(ITRQ * itr, ITR_ID id, unsigned int event, void * arg1, void * arg2)
{
	ITR_MSG msg;
	msg.event = event;
	msg.arg1 = arg1;
	msg.arg2 = arg2;
	return ITR_push(itr, id, &msg);
}

#endif // old itr code
