#ifndef THREAD_H
#define THREAD_H
#include "defines.h"
struct FORK_ARG
{
	HANDLE hEvent;
	void (__cdecl *threadcode)(void*);
	void *arg;
};
typedef void ( __cdecl* pThreadFunc )( void* );
void __cdecl forkthread_r(struct FORK_ARG *fa);
unsigned long ForkThread(pThreadFunc threadcode,void *arg);
void aim_keepalive_thread(void* fa);
//void contact_setting_changed_thread(char* data);
//void message_box_thread(char* data);
void accept_file_thread(char* szFile);
void redirected_file_thread(char* blob);
void proxy_file_thread(char* blob);
#endif
