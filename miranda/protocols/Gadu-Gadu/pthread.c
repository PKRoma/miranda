/*
AOL Instant Messenger Plugin for Miranda IM

Copyright (c) 2003 Robert Rainwater

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

// Safe thread code for Miranda IM
struct FORK_ARG {
	HANDLE hEvent;
	void (__cdecl *threadcode)(void*);
	unsigned (__stdcall *threadcodeex)(void*);
	void *arg;
};

unsigned __stdcall forkthreadex_r(void *param)
{
    struct FORK_ARG *fa = (struct FORK_ARG *)param;
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

// Minipthread code from Miranda IM source
GGINLINE int pthread_create(pthread_t * tid, const pthread_attr_t * attr, void *(__stdcall * thread_start) (void *), void *param)
{
    tid->hThread = (HANDLE) forkthreadex(NULL, 0, (unsigned (__stdcall *) (void *)) thread_start, param, 0, (unsigned *) &tid->dwThreadId);
    return 0;
}

int pthread_detach(pthread_t * tid)
{
    CloseHandle(tid->hThread);
    return 0;
}

int pthread_join(pthread_t * tid, void **value_ptr)
{
    if (tid->dwThreadId == GetCurrentThreadId())
        return 0;
    WaitForSingleObject(tid->hThread, INFINITE);
    return 0;
}
