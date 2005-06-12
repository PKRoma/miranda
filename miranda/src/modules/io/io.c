/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2005 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

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

#include "../../core/commonheaders.h"
#include "../database/dblists.h"
#include <m_io.h>
#include <m_io_ui.h>
#include "iomsgs.h"

#define XXX(msg) OutputDebugString(msg "\r\n");

DWORD (WINAPI *MyMsgWaitForMultipleObjectsEx)(DWORD,CONST HANDLE*,DWORD,DWORD,DWORD);
static DWORD MsgWaitForMultipleObjectsExWorkaround(DWORD nCount, const HANDLE *pHandles, 
	DWORD dwMsecs, DWORD dwWakeMask, DWORD dwFlags);

// -------- protocol services that need to be implemented --------

typedef struct {
	int cbSize;
	char * szProto;
	char * szID; 		// can't be NULL
	HANDLE hContact; 	// if the contact isnt in the DB, set to NULL
	HANDLE hTransport;	// protocol transfer object, if not used must be freed at some point
} MIOINVITE;

/*
	wParam:
	lParam: (LPARAM)&MIOINVITE;
	Affect: The protocol recv'd an invite, proto will get PS_IO_ACCEPT called if the invite is accepted
	or PS_IO_DENY if it is not.
	Returns: Not needed
	Warning: This service can be called from any thread but the reply will be on the main thread.
*/
#define MIO_INVITE_ASK "Miranda/IO/Ask"

static __inline int MIO_InviteAsk(char * szProto, char * szID, HANDLE hContact, HANDLE hTransport)
{
	MIOINVITE i = {0};
	i.cbSize=sizeof(i);
	i.szProto=szProto;
	i.szID=szID;
	i.hContact=hContact;
	i.hTransport=hTransport;
	return CallService(MIO_INVITE_ASK, 0, (LPARAM)&i);
}

#define PS_IO_ACCEPT "/IO/Accept"
#define PS_IO_DENY "/IO/Deny"

/*
*/
#define PS_IO_ "/"

/*
	wParam: (WPARAM) (HANDLE) Wait object (Win32 event object) which is inititally not signalled.
	lParam: (LPARAM) (HANDLE) IO object
	Affect: The IO subsystem requests the protocol allocate a transfer object and return it, this object
		will be later freed as needed.
	Returns: non zero on success, zero on failure, the return value is the transport object.
	Note: This service will always be called via the main thread
	Note 2: This transfer object is not optional, if the protocol does not need extra transfer requirements it must
		fake these objects since they must exist.
	Note 3: You do not own the event object, once it has become signalled, forget about it. Use SetEvent() to signal
		the object once the protocol knows the connection result.
*/
#define PS_TIO_ALLOC "/TIO/Alloc"

/*
	wParam: TIO_CAP_*
	lParam: (LPARAM) (HANDLE) transfer object allocated by a proto
	Affect: Queries the status/condition of a transport object
	Returns: Depends on CAP query
	Note: If the transport object doesnt support a given object query it should silently fail and return 0
*/
#define TIO_CAP_CONNECTED 0x1 // returns 0 if connected, otherwise it is the error condition
#define PS_TIO_GETCAP "TIO/GetProp"

/*
	wParam: 0
	lParam: (LPARAM) (HANDLE) transfer object
	Affect: The IO system is done with the transfer object, it should release any resources, including 
		the transfer object itself.
	Returns: 0 on success, non zero on failure
*/
#define PS_TIO_RELEASE "TIO/Release"

// -------- end -------------------------------------------------------

// ------------------------------- header ----------------------

#define MIOCTX_FLAG_NULL 0x0
#define MIOCTX_FLAG_INVITED 0x1 // if set don't need to allocate a transfer object since we got one already.

typedef struct {
	int cbSize;
} MIOMSG;

typedef struct {
	int cbSize;	
	unsigned flags;
	char * szProto;	// makes own copy
	HANDLE hTransport;
	HANDLE ctx; 	// in: set to NULL, out: return handle
} MIOCTXFLAGS;

#define MIO_STATE_INVALIDATE 0x1	// Handle has been MIO_CloseHandle()'d
#define MIO_STATE_B 0x2 
#define MIO_STATE_C 0x4
#define MIO_STATE_D 0x8
#define MIO_STATE_E 0x10
#define MIO_STATE_F 0x20
#define MIO_STATE_G 0x40
#define MIO_STATE_H 0x80

typedef struct { // private!
	unsigned flags;		// MIO_FLAG_*
	unsigned state;		// MIO_STATE_*
	unsigned handle;	// # assocated with ctx
	DWORD hThreadId;	// thread #, readonly once set
	char * szProto;		// proto, readonly once set
	HANDLE hThread;		// thread handle, readonly once set (never changed)
	HANDLE hStartEvent;	// wait object which must be signalled when the thread has init'd, readonly once set.
	HANDLE hTransport;	// transport object (if any), (mutex)
	HANDLE hTransportEvent; // readonly once set, CloseHandle() only on CTX shutdown - is manual event
	char * szUIClass;		// readonly once set, the UI class which should be used for this IO object, do not free()!
	HANDLE hUI;				// readonly once set, handle to the associated UI instance
} MIOCTX;





/*
	wParam: 0
	lParam: (LPARAM) &MIOCTXFLAGS
	Returns: 0 on success, non zero on failure: the handle is in .ctx
	Affect: create a new IO object set up for a protocol with its flags, if hTransport is NULL the IO object
		will request an object via PS_TIO_ALLOC, if hTransport is non NULL, PS_TIO_RELEASE will be called 
		when the object is no longer needed. If you are using hTransport != NULL make sure 
		MIOCTX_FLAG_INVITED is set.
	Warning: Make sure ServiceExists() returns TRUE before calling this function!
	Note: See MIO_new()
*/
#define MS_MIO_NEW "Miranda/IO/New"


/*
	wParam: 0
	lParam: (LPARAM) &MIOEVENT;
	Affect: Send an event to an IO object
	Returns: 0 on success, non zero on failure
*/
typedef struct {
	int cbSize;
	HANDLE hIO;
	UINT msg;
	WPARAM wParam;
	LPARAM lParam;
} MIOSENDEVENT;
#define MS_MIO_SENDEVENT "Miranda/IO/SendEvent"

/*
	wParam: 0
	lParam: (LPARAM)&MIOHEAP;
	Affect: Allocate, reallocate or free a memory block. 
	Returns: 0 on success, non zero on failure - the return memory pointer is in .ptr
	Vital: All memory passed to the IO system (incl message structures and their content) if dynamic MUST be
		allocated on this heap because there might not be a deallocator event.
	Note: The memory is allocated on a private heap object (win32) and should not be shared 
	Note 2: All memory is returned initalised.
	Note 3: see MIO_HeapAlloc(), MIO_HeapFree(), MIO_HeapRealloc
	Note 4: This is thread safe
	Note 5: all memory is set to zero
*/
#define MIOHEAP_MALLOC 1
#define MIOHEAP_FREE 2
#define MIOHEAP_REALLOC 3
typedef struct {
	int cbSize;
	int type;	// MIOHEAP_*
	size_t nbytes;
	void * ptr;	// in/out
} MIOHEAP;
#define MS_MIO_HEAP_REQ "Miranda/IO/HeapRequest"

/*
	wParam: 0
	lParam: (LPARAM) (HANDLE) IO object
	Affect: Request the IO system close the IO object and release any resources, close any transport objects
		and give up
	Returns: 0 on success, non zero on failure
*/
#define MS_MIO_CLOSEHANDLE "Miranda/IO/CloseHandle"

static __inline HANDLE MIO_new(char * szProto, HANDLE hTransport, unsigned flags)
{
	MIOCTXFLAGS f={0};
	f.cbSize=sizeof(f);
	f.szProto=szProto;
	f.hTransport=hTransport;
	f.flags=flags;
	if ( CallService(MS_MIO_NEW, 0, (LPARAM)&f) ) return NULL;
	return f.ctx;
}

static __inline void * MIO_HeapAlloc(size_t nbytes)
{
	MIOHEAP heap={0};
	heap.cbSize=sizeof(heap);
	heap.type=MIOHEAP_MALLOC;
	heap.nbytes=nbytes;
	CallService(MS_MIO_HEAP_REQ, 0, (LPARAM)&heap);	
	return heap.ptr;
}

static __inline int MIO_HeapFree(void * p)
{
	MIOHEAP heap={0};
	heap.cbSize=sizeof(heap);
	heap.type=MIOHEAP_FREE;
	heap.ptr=p;
	return CallService(MS_MIO_HEAP_REQ, 0, (LPARAM)&heap);	
}

static __inline void * MIO_HeapRealloc(void * p, size_t nbytes)
{
	MIOHEAP heap={0};
	heap.cbSize=sizeof(heap);
	heap.type=MIOHEAP_REALLOC;
	heap.nbytes=nbytes;
	heap.ptr=p;
	if ( CallService(MS_MIO_HEAP_REQ, 0, (LPARAM)&heap) == 0 ) return heap.ptr;
	return NULL;
}

static __inline int MIO_CloseHandle(HANDLE hIO)
{
	return CallService(MS_MIO_CLOSEHANDLE, 0, (LPARAM)hIO);
}

static __inline int MIO_SendEvent(HANDLE hIO, UINT msg, WPARAM wParam, LPARAM lParam)
{
	MIOSENDEVENT e={0};
	e.cbSize=sizeof(e);
	e.hIO=hIO;
	e.msg=msg;
	e.wParam=wParam;
	e.lParam=lParam;
	return CallService(MS_MIO_SENDEVENT, 0, (LPARAM)&e);
}

// ------------------------------- end of header ----------------------

static SortedList mioContextList;
static CRITICAL_SECTION csMIO;
static HANDLE hHeapMIO=NULL;
static char ** mioClassList=NULL;
static unsigned mioClassListCount=0;


static unsigned MIO_GetState(MIOCTX * ctx)
{
	unsigned f;
	EnterCriticalSection(&csMIO);
	f=ctx->state;
	LeaveCriticalSection(&csMIO);
	return f;
}

static void MIO_SetState(MIOCTX * ctx, unsigned bit)
{
	EnterCriticalSection(&csMIO);
	ctx->state |= bit;
	LeaveCriticalSection(&csMIO);
}

static void MIO_UnsetState(MIOCTX * ctx, unsigned bit)
{
	EnterCriticalSection(&csMIO);
	ctx->state &= ~bit;
	LeaveCriticalSection(&csMIO);
}

static HANDLE MIO_GetTransport(MIOCTX * ctx)
{
	HANDLE h;
	EnterCriticalSection(&csMIO);
	h=ctx->hTransport;
	LeaveCriticalSection(&csMIO);	
	return h;
}

static int __inline CallProtoServiceSync(char * szProto, char * szService, WPARAM w, LPARAM l)
{
	char sz[MAX_PATH];
	mir_snprintf(sz,sizeof(sz),"%s%s", szProto, szService);
	return CallServiceSync(sz, w, l);
}

// waits for an event object and spins any message pump
static int MIO_WaitForEvent(HANDLE hEvent)
{
	for (;;) {
		// will get WAIT_IO_COMPLETION for QueueUserAPC() which isnt a result
		DWORD rc=MsgWaitForMultipleObjectsExWorkaround(hEvent != NULL ? 1 : 0, &hEvent, INFINITE, QS_ALLEVENTS, 0);
		if ( rc == WAIT_IO_COMPLETION ) {
			if ( Miranda_Terminated() ) return 3;
		}
		if ( rc == WAIT_OBJECT_0 + 1 ) {
			// got msg
			return 1;
		} else if ( rc==WAIT_OBJECT_0 ) {
			// got object
			return 0;
		} else if ( rc==WAIT_ABANDONED_0 || rc == WAIT_FAILED ) return 1;		
	}
}

// only called via shutdown or thread shutdown
void MIO_FreeContext(MIOCTX * ctx, int thread)
{
	MIOCTX c={0};
	int index=0;
	c.handle=ctx->handle;
	EnterCriticalSection(&csMIO);
	if ( !List_GetIndex(&mioContextList,&c,&index) ) {
		LeaveCriticalSection(&csMIO);
		XXX("MIOCTX* wasnt in list!");
		return;
	}
	List_Remove(&mioContextList,index);
	LeaveCriticalSection(&csMIO);
	CloseHandle(ctx->hThread);
	CloseHandle(ctx->hTransportEvent);
	if ( ctx->szProto ) free(ctx->szProto);
	free(ctx);
}

static __inline int CallAbstractServiceSync(char * szClass, char * szService, WPARAM w, LPARAM l)
{
	char sz[MAX_PATH];
	mir_snprintf(sz,sizeof(sz),"%s%s",szClass,szService);
	return CallServiceSync(sz, w, l);
}

static __inline int AbstractServiceExists(char * szClass, char * szService)
{
	char sz[MAX_PATH];
	mir_snprintf(sz,sizeof(sz),"%s%s",szClass,szService);
	return ServiceExists(sz);
}

void debuglog(char * szClass, HANDLE hUI, char * szText)
{
	CallAbstractServiceSync(szClass, "__debug__", (WPARAM)hUI, (LPARAM)szText);
}

static unsigned __stdcall MIO_thread(MIOCTX * ctx)
{
	HANDLE hTransport = MIO_GetTransport(ctx);	
	HANDLE hEvent=ctx->hTransportEvent;
	MSG dummy;
	int rc;
	// create a message queue
	PeekMessage(&dummy, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	// let the source thread go, handle will be closed by source thread
	SetEvent(ctx->hStartEvent);
	// make sure a transport object exists
	if ( !(ctx->flags & MIOCTX_FLAG_INVITED) || hTransport == NULL ) 
	{
		// get a transport object 
		hTransport=(HANDLE)CallProtoServiceSync(ctx->szProto, PS_TIO_ALLOC, (WPARAM)hEvent, (LPARAM)ctx->handle);
		if ( hTransport == NULL ) {
			XXX("Failed to create transport handle");
			return 1;
		}
	}

	debuglog(ctx->szUIClass, ctx->hUI, "[thread] Waiting for transport object...");
	WaitForSingleObject(hEvent,INFINITE);
	debuglog(ctx->szUIClass, ctx->hUI, "[thread] Awake!"); 

	rc=CallProtoServiceSync(ctx->szProto, PS_TIO_GETCAP, TIO_CAP_CONNECTED, (LPARAM)hTransport);
	if ( rc != 0 ) {
		debuglog(ctx->szUIClass, ctx->hUI, "Transfer object says it failed to connect, damn.");
	}
	debuglog(ctx->szUIClass, ctx->hUI, "connected, waiting on message queue...");
	for ( ;; ) {	
		MSG msg;
		MIO_WaitForEvent(NULL);
		while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) {	
			debuglog(ctx->szUIClass, ctx->hUI, "Processing Message Loop...");
			switch ( msg.message ) {
				case WM_MIO_CLOSEHANDLE: {
					CallProtoServiceSync(ctx->szProto, PS_TIO_RELEASE, 0, (LPARAM)hTransport);
					XXX("[thread] got WM_MIO_CLOSEHANDLE");
					MIO_FreeContext(ctx,1);
					return 1;
				}
				case WM_MIO_DUMMY: {
					debuglog(ctx->szUIClass, ctx->hUI, " Got WM_MIO_DUMMY... ");
					break;
				}
			}
		} //while
	} // for
	return 0;
}

static MIOCTX * MIO_FindContext(unsigned handle)
{
	MIOCTX ctx = {0}, * p;
	ctx.handle=handle;
	EnterCriticalSection(&csMIO);
	p=List_Find(&mioContextList, &ctx);
	LeaveCriticalSection(&csMIO);
	return p;
}

static HANDLE MIO_CreateUI(char * szUIClass, char * szProto, unsigned handle)
{
	// make sure we have a (hidden) UI object 
	MIO_UICREATE uic={0};
	uic.cbSize=sizeof(MIO_UICREATE);
	uic.flags=0;
	uic.hIO=(HANDLE)handle;
	if ( CallAbstractServiceSync(szUIClass, MS_MIO_UI_CREATE, 0, (LPARAM)&uic) ) return NULL;
	return uic.hUI;
}

static unsigned MIO_AddContext(char * szProto, MIOCTXFLAGS * f)
{
	for(;;) { 		
		unsigned r=rand();	
		int index;
		MIOCTX ctx={0};
		HANDLE hUI=NULL;
		char * szUIClass = mioClassList ? mioClassList[0] : "";
		ctx.handle=r;		
		EnterCriticalSection(&csMIO);
		if ( MIO_FindContext(r) != NULL ) continue;		
		// the handle is unique and we have the lock, but we have to release it to switch threads, gah
		if ( !List_GetIndex(&mioContextList,&ctx,&index) ) {
			// insert a NULL CTX* so any lookup will fail
			List_Insert(&mioContextList,NULL,index);
			LeaveCriticalSection(&csMIO);
			// left list with new handle pointing to nothing
			hUI=MIO_CreateUI(szUIClass, szProto, r);
			// enter the lock again
			EnterCriticalSection(&csMIO);
		}
		// "index" might not be valid so look it up again
		if ( !List_GetIndex(&mioContextList,&ctx,&index) ) {
			HANDLE hStartEvent;
			MIOCTX * p=calloc(sizeof(MIOCTX),1);
			p->szProto=_strdup(szProto);
			p->handle=r;
			p->flags=f->flags;
			XXX("[warn] Using first found UI class...");
			p->szUIClass=szUIClass;
			p->hUI=hUI;
			// init
			if ( f->hTransport != NULL ) p->flags |= MIOCTX_FLAG_INVITED;
			p->hTransport=f->hTransport;
			hStartEvent=p->hStartEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
			p->hTransportEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
			p->hThread=(HANDLE)forkthreadex(NULL, 0, MIO_thread, p, 0, &p->hThreadId);
			List_Insert(&mioContextList,p,index);
			
			LeaveCriticalSection(&csMIO);		
			WaitForSingleObject(hStartEvent,INFINITE);
			CloseHandle(hStartEvent);			
			return r;
		}
		LeaveCriticalSection(&csMIO);
	}
	return -1;
}

// returns 1 if an expected proto service is missing
static int MIO_CheckProtoServices(char * szProto)
{
	char sz[MAX_PATH];
	mir_snprintf(sz,sizeof(sz),"%s%s", szProto, PS_TIO_ALLOC);
	if ( !ServiceExists(sz) ) return 1;
	mir_snprintf(sz,sizeof(sz),"%s%s", szProto, PS_TIO_GETCAP);
	if ( !ServiceExists(sz) ) return 1;
	mir_snprintf(sz,sizeof(sz),"%s%s", szProto, PS_TIO_RELEASE);
	if ( !ServiceExists(sz) ) return 1;
	return 0;
}

static int MIO_New2(WPARAM wParam, LPARAM lParam)
{
	MIOCTXFLAGS * f = (MIOCTXFLAGS *) lParam;
	if ( f && f->cbSize==sizeof(MIOCTXFLAGS) && f->ctx == NULL && f->szProto != NULL
		&& MIO_CheckProtoServices(f->szProto) == 0 ) {
		f->ctx=(HANDLE)MIO_AddContext(f->szProto, f);		
		return 0;
	}	
	return 1;
}

static int MIO_HeapReq2(WPARAM wParam, LPARAM lParam)
{
	MIOHEAP * heap = (MIOHEAP *) lParam;
	if ( !( heap != NULL && heap->cbSize==sizeof(MIOHEAP) ) ) return 1;
	switch ( heap->type ) {
		case MIOHEAP_MALLOC: {
			heap->ptr=HeapAlloc(hHeapMIO, HEAP_ZERO_MEMORY, heap->nbytes);
			return heap->ptr != NULL ? 0 : 1;
		}
		case MIOHEAP_FREE: {
			if ( heap->ptr != NULL ) HeapFree(hHeapMIO, 0, heap->ptr);
			heap->ptr=NULL;
			return 0;
		}
		case MIOHEAP_REALLOC: {
			void * p = heap->ptr=HeapReAlloc(hHeapMIO, HEAP_ZERO_MEMORY, heap->ptr, heap->nbytes);
			if ( p != NULL ) { 
				heap->ptr=p;
				return 0;
			}
			break;
		}
	}
	return 1;
}

static int MIO_CloseHandle2(WPARAM wParam, LPARAM lParam)
{
	MIOCTX * ctx = MIO_FindContext((unsigned)lParam);
	if ( ctx == NULL ) return 1;
	if ( MIO_GetState(ctx) & MIO_STATE_INVALIDATE ) {
		XXX("CloseHandle(): Handle already marked for closure.");
		return 1;
	}
	MIO_SetState(ctx, MIO_STATE_INVALIDATE);
	if ( WaitForSingleObject(ctx->hThread,0) != WAIT_OBJECT_0 ) {
		// thread alive still, signal any block waiting for connection, tell it to die.
		SetEvent(ctx->hTransportEvent);
		return PostThreadMessage(ctx->hThreadId, WM_MIO_CLOSEHANDLE, 0, 0) ? 0 : 1;
	}
	// no thread running, but CTX* still exists
	XXX("CloseHandle(): CTX* exists but thread does not!");
	return 1;
}

static int MIO_SendEvent2(WPARAM wParam, LPARAM lParam)
{
	MIOSENDEVENT * e = (MIOSENDEVENT *) lParam;
	MIOCTX * ctx;
	if ( !( e && e->cbSize==sizeof(MIOSENDEVENT) && e->hIO != NULL ) ) return 1;
	ctx=MIO_FindContext((unsigned)e->hIO);
	if ( ctx == NULL ) return 1;
	if ( PostThreadMessage(ctx->hThreadId, e->msg, e->wParam, e->lParam) == 0 ) return 1;
	return 0;
}

static int mioContextListSort(MIOCTX * a, MIOCTX * b)
{
	if ( a && b ) {
		if ( a->handle < b->handle ) return -1;
		if ( a->handle > b->handle ) return 1;
		return 0;
	}
	return 1;
}

static int allocwait(HANDLE hEvent)
{
	XXX("::Starting wait... 7s");
	SleepEx(1000 * 7,TRUE);
	XXX("::Signally IO subsystem...");
	SetEvent(hEvent);
	XXX("::Done, qutting");
	return 255;
}

// PS_TIO_ALLOC
static int Proto_Tio_Alloc(WPARAM wParam, LPARAM lParam)
{
	forkthread(allocwait, 0, (void *)wParam);
	return 1;
}
// PS_TIO_GETCAP
static int Proto_Tio_GetCap(WPARAM wParam, LPARAM lParam)
{
	switch ( wParam ) {
		case TIO_CAP_CONNECTED: {
			// is the transport connected or not.
			return 0;
		}
	}
	return 0;
}
// PS_TIO_RELEASE
static int Proto_Tio_Release(WPARAM w, LPARAM l)
{
	XXX("(transport object)->release()");
	return 0;
}

static int MIO_UI_RegisterClass2(WPARAM wParam, LPARAM lParam)
{
	char * szClass = (char *) lParam;
	if ( szClass == NULL ) return 1;
	if ( !AbstractServiceExists(szClass,MS_MIO_UI_GETCAPS) || !AbstractServiceExists(szClass,MS_MIO_UI_CREATE) ) return 1;
	mioClassList=realloc(mioClassList,(mioClassListCount+1) * sizeof(char *));
	mioClassList[mioClassListCount++]=_strdup(szClass);
	return 0;
}

static int CListClicked(WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact=(HANDLE)wParam;
	// TODO: Find existing single chat and reshow window?
	HANDLE hIO=MIO_new("MSN", NULL, MIOCTX_FLAG_NULL);
	return 0;
}

static int Test(WPARAM wParam, LPARAM lParam)	// ME_SYSTEM_MODULESLOADED
{	
	HookEvent(ME_CLIST_DOUBLECLICKED, CListClicked);
	return 0;
}


static DWORD MsgWaitForMultipleObjectsExWorkaround(DWORD nCount, const HANDLE *pHandles, 
	DWORD dwMsecs, DWORD dwWakeMask, DWORD dwFlags)
{
	DWORD rc;
	if ( MyMsgWaitForMultipleObjectsEx != NULL )
		return MyMsgWaitForMultipleObjectsEx(nCount, pHandles, dwMsecs, dwWakeMask, dwFlags);
	rc=MsgWaitForMultipleObjects(nCount, pHandles, FALSE, 50, QS_ALLINPUT);
	if ( rc == WAIT_TIMEOUT ) rc=WaitForMultipleObjectsEx(nCount, pHandles, FALSE, 20, TRUE);
	return rc;
}

static int UnloadIOObjects(WPARAM wParam, LPARAM lParam) // ME_SYSTEM_PRESHUTDOWN
{	
	int j;
	EnterCriticalSection(&csMIO);
	for (j=0; j<mioContextList.realCount; j++) {
		MIOCTX * ctx = mioContextList.items[j];
		PostThreadMessage(ctx->hThreadId, WM_MIO_CLOSEHANDLE, 0, 0);
	}
	LeaveCriticalSection(&csMIO);
	return 0;
}

static int UnloadIOModule(WPARAM wParam, LPARAM lParam)
{
	unsigned j;
	for (j=0; j<mioClassListCount; j++) {
		char * p = mioClassList[j];
		if ( p != NULL ) free(p);
	}
	if ( mioClassList != NULL ) free(mioClassList);
	List_Destroy(&mioContextList);
	DeleteCriticalSection(&csMIO);
	HeapDestroy(hHeapMIO);
	return 0;
}

// for a while
static int LoadChatBridge(void);

int LoadIOModule(void)
{
	hHeapMIO=HeapCreate(0, 0, 0);
	MyMsgWaitForMultipleObjectsEx=(DWORD (WINAPI *)(DWORD,CONST HANDLE*,DWORD,DWORD,DWORD))GetProcAddress(GetModuleHandle("user32"),"MsgWaitForMultipleObjectsEx");
	InitializeCriticalSection(&csMIO);
	HookEvent(ME_SYSTEM_PRESHUTDOWN, UnloadIOObjects);
	HookEvent(ME_SYSTEM_SHUTDOWN, UnloadIOModule);
	ZeroMemory(&mioContextList,sizeof(mioContextList));
	mioContextList.sortFunc=mioContextListSort;
	mioContextList.increment=1;
	srand(time(NULL));
	CreateServiceFunction(MS_MIO_NEW, MIO_New2);
	CreateServiceFunction(MS_MIO_HEAP_REQ, MIO_HeapReq2);
	CreateServiceFunction(MS_MIO_CLOSEHANDLE, MIO_CloseHandle2);
	CreateServiceFunction(MS_MIO_SENDEVENT, MIO_SendEvent2);
	/**/
	CreateServiceFunction(MS_MIO_UI_REGISTERCLASS, MIO_UI_RegisterClass2);
	{
		CreateProtoServiceFunction("MSN", PS_TIO_ALLOC, Proto_Tio_Alloc);
		CreateProtoServiceFunction("MSN", PS_TIO_GETCAP, Proto_Tio_GetCap);
		CreateProtoServiceFunction("MSN", PS_TIO_RELEASE, Proto_Tio_Release);
	}
	//HookEvent(ME_SYSTEM_MODULESLOADED, Test);
	return LoadChatBridge();
}



#include <m_chat.h>
static int chat_getcaps(WPARAM wParam, LPARAM lParam) // MS_MIO_UI_GETCAPS
{
	switch ( wParam ) {
		case -1: {
			break;
		} 
	}
	return 0;
}

static int chat_create(WPARAM wParam, LPARAM lParam) // MS_MIO_UI_CREATE
{
	MIO_UICREATE * ui = (MIO_UICREATE *) lParam;
	char szID[32]={0};
	if ( !( ui != NULL && ui->cbSize==sizeof(MIO_UICREATE) && ui->hIO != NULL ) ) return 1;
	XXX("[chat] MS_MIO_UI_CREATE!");
	{
		GCREGISTER gc={0};
		gc.cbSize=sizeof(gc);
		gc.dwFlags=GC_TYPNOTIF;
		gc.pszModule="MSN";
		gc.pszModuleDispName="MSN";
		gc.iMaxText=512;
		CallService(MS_GC_REGISTER, 0, (LPARAM)&gc);
	}
	{
		GCWINDOW w={0};		
		itoa((int)ui->hIO, szID, 10);
		w.cbSize=sizeof(w);
		w.iType=GCW_CHATROOM;
		w.pszModule="MSN";
		w.pszName="XXX";
		w.pszID=szID; // the room id is the unique handle of the IO object
		w.bDisableNickList=TRUE;
		w.dwItemData=(DWORD)ui->hIO;
		CallService(MS_GC_NEWCHAT, 0, (LPARAM)&w);
		// get uniqueness from the IO object
		ui->hUI = ui->hIO;
	}
	{
		GCEVENT e={0};
		GCDEST dst={0};
		e.cbSize=sizeof(e);
		e.pDest=&dst;
		dst.pszModule="MSN";
		dst.pszID=szID;
		dst.iType=GC_EVENT_CONTROL;
		CallService(MS_GC_EVENT, WINDOW_INITDONE, (LPARAM)&e);
	}
	return 0;
}

static int chat_log(WPARAM w, LPARAM l)
{
	HANDLE hUI=(HANDLE)w;
	char * log = (char *)l;
	char szID[32]={0};
	GCEVENT e={0};
	GCDEST d={0};

	// the unique name is the unique UI object which is a unique IO object handle too.
	itoa((int)hUI, szID, 10);
	//
	d.pszModule="MSN";	 // damn
	d.pszID=szID;
	d.iType=GC_EVENT_INFORMATION;
	//
	e.cbSize=sizeof(e);
	e.pDest=&d;
	e.pszText=log;
	e.bIsMe=TRUE;
	e.bAddToLog=TRUE;
	e.time=time(NULL);
	CallService(MS_GC_EVENT, 0, (LPARAM)&e);	
	return 0;
}

static int chat_event(WPARAM wParam, LPARAM lParam)
{
	GCHOOK * hook = (GCHOOK *) lParam;
	GCDEST * dest;
	if ( !( hook && hook->pDest && hook->pszText ) ) return 1;
	dest=hook->pDest;
	switch ( dest->iType ) {
		case GC_USER_MESSAGE: {
			// hook->dwData == io object, hook->pszText
			break;
		}
	}	
	return 0;
}

#define CHAT "chat"

static int chat_modulesloaded(WPARAM wParam, LPARAM lParam)
{
	HookEvent(ME_GC_EVENT, chat_event);
	{
		HANDLE hContact = DB_AddVolatile(0, "ICQ");
		CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)"ICQ");
		DBWriteContactSettingDword(hContact, "ICQ", "UIN", 1234567);
		DBWriteContactSettingWord(hContact, "ICQ", "Status", ID_STATUS_ONLINE);
		//DB_DeleteVolatile(hContact);
	}
	{
		unsigned c=0;
		char buf[MAX_PATH];
		{
			HANDLE h1 = DB_AddVolatile(0, "MSN");
			HANDLE h2 = DB_AddVolatile(0, "MSN");
			HANDLE h3 = DB_AddVolatile(0, "MSN");
			HANDLE h4 = DB_AddVolatile(0, "MSN");
			HANDLE h5 = DB_AddVolatile(0, "MSN");

			if ( h1 == INVALID_HANDLE_VALUE || h2 == INVALID_HANDLE_VALUE || 
				h3 == INVALID_HANDLE_VALUE  || h4 == INVALID_HANDLE_VALUE || 
				h5 == INVALID_HANDLE_VALUE ) {
				mir_snprintf(buf,sizeof(buf)," Failed to allocate handle after %u attempts \n ", c);
				OutputDebugString(buf);
				return 0;
			}			
			mir_snprintf(buf,sizeof(buf),"******* %x, %x, %x, %x, %x [%u] \n", h1, h2, h3 ,h4, h5, c);
			OutputDebugString(buf);
			c+=5;
		}
	}
	return 0;
}

// THIS IS LOADED BEFORE EVERYTHING NEARLY
static int LoadChatBridge(void)
{
	CreateUIService(CHAT, MS_MIO_UI_GETCAPS, chat_getcaps);
	CreateUIService(CHAT, MS_MIO_UI_CREATE, chat_create);
	CreateUIService(CHAT, "__debug__", chat_log);	
	HookEvent(ME_SYSTEM_MODULESLOADED, chat_modulesloaded);
	MIO_UI_RegisterClass(CHAT);	
	return 0;
}

// need an 'thing' to do UI text message to -> IO object