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
#include <process.h>
#include "pthread.h"

// Minipthread code from Miranda IM source
int pthread_create(pthread_t * tid, const pthread_attr_t * attr, void *(__stdcall * thread_start) (void *), void *param)
{
    tid->hThread = (HANDLE) _beginthreadex(NULL, 0, (unsigned (__stdcall *) (void *)) thread_start, param, 0, (unsigned *) &tid->dwThreadId);
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
