#ifndef ITR_H__
#define ITR_H__ 1

#include <windows.h>
#include <process.h>
#include "miranda.h"

typedef void (*ITRWORKERFUNCTION) (HANDLE hHeap, void * pExtra);

typedef struct {
	DWORD dwThreadId; 	// ID of the worker thread
	HANDLE hHeap;		// a heap shared between worker thread and main, everything allocated belongs to worker
	HANDLE hStartEvent;	// owned by main, set by worker at thread start
	ITRWORKERFUNCTION workerFunction;
	void * pExtra;
} ITRWORKER;

#define WM_ITR_QUIT (WM_USER+1)
#define WM_ITR_BASE (WM_USER+10)

void ITR_NewWorker(ITRWORKER * w, ITRWORKERFUNCTION wFunc, void * pExtra);
void ITR_FreeWorker(ITRWORKER * w);
void * ITR_Alloc(ITRWORKER * w, size_t nBytes);
int ITR_PostEvent(ITRWORKER * w, UINT msg, WPARAM a, LPARAM b);
BOOL ITR_ActiveWorker(ITRWORKER * w);
















// contains handles/information for two threads to do async/sync communication
typedef struct {
	DWORD dwThreadId; 	// ID of the worker thread
	HANDLE hHeap;		// a heap shared between worker thread and main, everything allocated belongs to worker
} ITR;

typedef unsigned int ITR_ID;

typedef struct ITR_IDQ 
{
	struct ITR_IDQ * next;
	HANDLE hThread;
	ITR_ID id;
} ITR_IDQ;

typedef struct {
	unsigned int event;
	void * arg1;
	void * arg2;
} ITR_MSG;

typedef struct ITR_FIFO {
	struct ITR_FIFO * next;
	struct ITR_FIFO * prev;
	ITR_ID id;
	ITR_MSG msg;
} ITR_FIFO;

typedef struct ITRQ
{
	CRITICAL_SECTION lock;
	ITR_IDQ * threadQ; // all the threads ever created, in FILO order
	ITR_FIFO * firstPush, * lastPush;
} ITRQ;

void ITR_new(ITRQ * itr);
void ITR_delete(ITRQ * itr);
ITR_ID ITR_register(ITRQ * itr, void (__stdcall * code) (void *), void * arg);
void ITR_unregister(ITRQ * itr, ITR_ID id);
int ITR_push(ITRQ * itr, ITR_ID id, ITR_MSG * msg);
int ITR_pop(ITRQ * itr, ITR_ID id, ITR_MSG * msg);
void ITR_waitfor(ITRQ * itr);
int ITR_event(ITRQ * itr, ITR_ID id, unsigned int event, void * arg1, void * arg2);

#endif /* ITR_H__ */
