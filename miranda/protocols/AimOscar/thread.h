#ifndef THREAD_H
#define THREAD_H
#include <stdio.h>
#include <windows.h>
#include <process.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_netlib.h>
struct FORK_ARG
{
	HANDLE hEvent;
	void (__cdecl *threadcode)(void*);
	void *arg;
};
typedef void ( __cdecl* pThreadFunc )( void* );
static void __cdecl forkthread_r(struct FORK_ARG *fa);
unsigned long ForkThread(pThreadFunc threadcode,void *arg);
#endif
