#include "thread.h"
static void __cdecl forkthread_r(struct FORK_ARG *fa)
{	
	void (*callercode)(void*) = fa->threadcode;
	void *arg = fa->arg;
	CallService(MS_SYSTEM_THREAD_PUSH, 0, 0);
	SetEvent(fa->hEvent);
	__try {
		callercode(arg);
	} __finally {
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
		CallService(MS_SYSTEM_THREAD_POP, 0, 0);
	} 
	return;
}

unsigned long ForkThread(pThreadFunc threadcode,void *arg)
{
	struct FORK_ARG fa;
	fa.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	fa.threadcode = threadcode;
	fa.arg = arg;
	unsigned long rc = _beginthread((pThreadFunc)forkthread_r,0, &fa);
	if ((unsigned long) -1L != rc) {
		WaitForSingleObject(fa.hEvent, INFINITE);
	}
	CloseHandle(fa.hEvent);
	return rc;
}
