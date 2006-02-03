#ifndef THREAD_H
#define THREAD_H
#include <stdio.h>
#include <windows.h>
#include <process.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_netlib.h>
#include <m_database.h>
#include "defines.h"
#include "aim.h"
struct FORK_ARG
{
	HANDLE hEvent;
	void (__cdecl *threadcode)(void*);
	void *arg;
};
typedef void ( __cdecl* pThreadFunc )( void* );
static void __cdecl forkthread_r(struct FORK_ARG *fa);
unsigned long ForkThread(pThreadFunc threadcode,void *arg);
void set_status_thread(int status);
void accept_file_thread(char* szFile);
void redirected_file_thread(char* blob);
void proxy_file_thread(char* blob);
#endif
