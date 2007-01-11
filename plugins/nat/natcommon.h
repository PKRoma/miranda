#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_netlib.h>
#include <m_langpack.h>
#include "m_nat.h"

#define NAT_SCRIPT_URL "http://miranda-icq.sourceforge.net/getip.php"

// forkthread.c
unsigned long forkthread (
	void (__cdecl *threadcode)(void*),
	unsigned long stacksize,
	void *arg
);

unsigned long forkthreadex(
	void *sec,
	unsigned stacksize,
	unsigned (__stdcall *threadcode)(void*),
	void *arg,
	unsigned cf,
	unsigned *thraddr
);