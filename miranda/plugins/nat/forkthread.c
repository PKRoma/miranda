#include "natcommon.h"

struct FORK_ARG {
	HANDLE hEvent;
	void (__cdecl *threadcode)(void*);
	unsigned (__stdcall *threadcodeex)(void*);
	void *arg;
};

void __cdecl forkthread_r(struct FORK_ARG *fa)
{	
	void (*callercode)(void*)=fa->threadcode;
	void *arg=fa->arg;
	CallService(MS_SYSTEM_THREAD_PUSH,0,0);
	SetEvent(fa->hEvent);
	__try {
		callercode(arg);
	} __finally {
		CallService(MS_SYSTEM_THREAD_POP,0,0);
	} 
	return;
}

unsigned long forkthread (
	void (__cdecl *threadcode)(void*),
	unsigned long stacksize,
	void *arg
)
{
	unsigned long rc;
	struct FORK_ARG fa;
	fa.hEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
	fa.threadcode=threadcode;
	fa.arg=arg;
	rc=_beginthread(forkthread_r,stacksize,&fa);
	if ((unsigned long)-1L != rc) {
		WaitForSingleObject(fa.hEvent,INFINITE);
	} //if
	CloseHandle(fa.hEvent);
	return rc;
}

unsigned long __stdcall forkthreadex_r(struct FORK_ARG *fa)
{
	unsigned (__stdcall * threadcode) (void *)=fa->threadcodeex;
	void *arg=fa->arg;
	unsigned long rc;
	
	CallService(MS_SYSTEM_THREAD_PUSH,0,0);
	SetEvent(fa->hEvent);
	__try {
		rc=threadcode(arg);
	} __finally {
		CallService(MS_SYSTEM_THREAD_POP,0,0);
	}
	return rc;
}

unsigned long forkthreadex(
	void *sec,
	unsigned stacksize,
	unsigned (__stdcall *threadcode)(void*),
	void *arg,
	unsigned cf,
	unsigned *thraddr
)
{
	unsigned long rc;
	struct FORK_ARG fa;
	fa.threadcodeex=threadcode;
	fa.arg=arg;
	fa.hEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
	rc=_beginthreadex(sec,stacksize,forkthreadex_r,&fa,0,thraddr);
	if (rc) {
		WaitForSingleObject(fa.hEvent,INFINITE);
	} 
	CloseHandle(fa.hEvent);
	return rc;
}