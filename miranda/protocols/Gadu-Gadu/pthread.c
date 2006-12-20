/*
AOL Instant Messenger Plugin for Miranda IM

Copyright (c) 2003-2006 Robert Rainwater

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

#include "gg.h"

/* Gena01 - added some defined to fix compilation with mingw gcc */
/* __try/__finally taken from abiword patch found on the web */
#if 0
 #include <crtdbg.h>
#else
#define __try
#define __except(x) if (0) /* don't execute handler */
#define __finally

#define _try __try
#define _except __except
#define _finally __finally
#endif

#include <excpt.h>

/***********************************/
/* Safe thread code for Miranda IM */

struct FORK_ARG {
	HANDLE hEvent;
	void (__cdecl *threadcode)(void*);
	unsigned (__stdcall *threadcodeex)(void*);
	void *arg;
};

unsigned __stdcall forkthreadex_r(void *param)
{
	struct FORK_ARG *fa = (struct FORK_ARG *)param;
	unsigned (__stdcall * threadcode) (void *) = fa->threadcodeex;
	void *arg = fa->arg;
	unsigned long rc;

	CallService(MS_SYSTEM_THREAD_PUSH, 0, 0);
	SetEvent(fa->hEvent);
	/* Push, execute and pop thread */
	__try
	{
		rc = threadcode(arg);
	}
	__finally
	{
		CallService(MS_SYSTEM_THREAD_POP, 0, 0);
	}
	return rc;
}

unsigned long forkthreadex(
	void *sec,
	unsigned stacksize,
	unsigned (__stdcall *threadcode)(void *),
	void *arg,
	unsigned cf,
	unsigned *thraddr
)
{
	unsigned long rc;
	struct FORK_ARG fa;
	fa.threadcodeex = threadcode;
	fa.arg = arg;
	fa.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	/* Wait until thread is pushed into the Miranda IM thread stack */
	if(rc = _beginthreadex(sec, stacksize, forkthreadex_r, &fa, 0, thraddr))
		WaitForSingleObject(fa.hEvent,INFINITE);
	CloseHandle(fa.hEvent);
	return rc;
}

/****************************************/
/* Portable pthread code for Miranda IM */

/* create thread */
GGINLINE int pthread_create(pthread_t *tid, const pthread_attr_t *attr, void *(__stdcall * thread_start) (void *), void *param)
{
	tid->hThread = (HANDLE) forkthreadex(NULL, 0, (unsigned (__stdcall *) (void *)) thread_start, param, 0, (unsigned *) &tid->dwThreadId);
	return 0;
}

/* detach a thread */
int pthread_detach(pthread_t *tid)
{
	CloseHandle(tid->hThread);
	return 0;
}

/* wait for thread termination */
int pthread_join(pthread_t *tid, void **value_ptr)
{
	if(tid->dwThreadId == GetCurrentThreadId())
		return 0;

	WaitForSingleObject(tid->hThread, INFINITE);
	return 0;
}

/* This code is not used */

#if 0

/* get calling thread's ID */
pthread_t *pthread_self(void)
{
	static int poolId = 0;
	static pthread_t tidPool[32];
	/* mark & round pool to 32 items */
	pthread_t *tid = &tidPool[poolId ++];
	poolId %= 32;

	tid->hThread = GetCurrentThread();
	tid->dwThreadId = GetCurrentThreadId();
	return tid;
}

/* cancel execution of a thread */
int pthread_cancel(pthread_t *thread)
{
	return TerminateThread(thread->hThread, 0) ? 0 : 3 /*ESRCH*/;
}

/* terminate thread */
void pthread_exit(void *value_ptr)
{
	_endthreadex((unsigned)value_ptr);
}

#endif