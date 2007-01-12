/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

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
#include "fuse_common.h"
#pragma hdrstop

typedef struct {
	MIRANDAHOOK pfnHook;
	HWND hwnd;
	UINT message;
} THookSubscriber;

typedef struct {
	char name[MAXMODULELABELLENGTH];
	DWORD hookHash;
	int subscriberCount;
	THookSubscriber *subscriber;
} THookList;

typedef struct {
	char name[MAXMODULELABELLENGTH];
	DWORD nameHash;
	MIRANDASERVICE pfnService;
	DWORD dwTimesLow,dwTimesHigh;
} TServiceList;

typedef struct {
	int hookId;
	HANDLE hDoneEvent;
	WPARAM wParam;
	LPARAM lParam;
	int result;
} THookToMainThreadItem;

typedef struct {
	HANDLE hDoneEvent;
	WPARAM wParam;
	LPARAM lParam;
	int result;
	const char *name;
} TServiceToMainThreadItem;

static THookList *hook;
static TServiceList *service;
static int hookCount,serviceCount;
static CRITICAL_SECTION csHooks,csServices;
static DWORD mainThreadId;
static HANDLE hMainThread;

int DumpErrorInformation(EXCEPTION_POINTERS *ep)
{	
	HANDLE hThread;
	CONTEXT context,*c=ep->ContextRecord;
	log_printf("<font color=red>");
	log_printf("- Exception Information -<br>");
	DuplicateHandle(GetCurrentProcess(),GetCurrentThread(),GetCurrentProcess(),&hThread,THREAD_GET_CONTEXT,FALSE,0);
	if (GetThreadContext(hThread,&context)) {
		HANDLE hSnap;
		hSnap=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId());
		if (hSnap != INVALID_HANDLE_VALUE) {
			MODULEENTRY32 moduleInfo;
			moduleInfo.dwSize=sizeof(moduleInfo);
			if (Module32First(hSnap,&moduleInfo)) {
				for (;;) {
					if (moduleInfo.dwSize==sizeof(moduleInfo)) {
						DWORD dwExceptAddr=(DWORD)ep->ExceptionRecord->ExceptionAddress;
						DWORD dwStart=(DWORD)moduleInfo.modBaseAddr,dwEnd=dwStart+moduleInfo.modBaseSize;
						if (dwExceptAddr >= dwStart && dwExceptAddr <= dwEnd) {
							log_printf("module\t: %s",moduleInfo.szModule);	break;
						} //if
					} //if
					moduleInfo.dwSize=sizeof(moduleInfo);
					if (!Module32Next(hSnap,&moduleInfo)) break;
				} //for
			} //if
			CloseHandle(hSnap);
		} //if
		
		/* try and walk the stack to show debugging information */
		 if (SymInitialize(GetCurrentProcess(),NULL,TRUE)) {

			STACKFRAME sf;
			memset(&sf,0,sizeof(sf));
			sf.AddrPC.Offset=c->Eip;
			sf.AddrPC.Mode=AddrModeFlat;
			sf.AddrFrame.Offset=c->Ebp;
			sf.AddrFrame.Mode=AddrModeFlat;

			SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_LOAD_LINES|SYMOPT_CASE_INSENSITIVE|SYMOPT_NO_CPP);
			/* StackWalk() */
			if (StackWalk(IMAGE_FILE_MACHINE_I386,GetCurrentProcess(),hThread,&sf,c,NULL,(void*)SymFunctionTableAccess,(void*)SymGetModuleBase,NULL)) {
				DWORD symOffset=0;
				IMAGEHLP_SYMBOL *pSym;
				pSym=calloc(1,sizeof(IMAGEHLP_SYMBOL)+0x400);
				pSym->SizeOfStruct=sizeof(IMAGEHLP_SYMBOL);
				pSym->MaxNameLength=0x400;
				if (sf.AddrPC.Offset) {
					if (SymGetSymFromAddr(GetCurrentProcess(),sf.AddrPC.Offset,&symOffset,pSym)) {
						log_printf("proc \t: %s",pSym->Name);
					} else {
						log_printf("proc \t: unable to find symbolic information.");
					} //if
				} else {
					log_printf("symbols\t: StackWalk() failed, GetLastError=%x",GetLastError());
				} //if
				free(pSym);
			} //if
			/* cleanup */
			SymCleanup(GetCurrentProcess());
		 } //if

	} //if:GetThreadContext
	CloseHandle(hThread);	
	log_printf("code\t: 0x%x",ep->ExceptionRecord->ExceptionCode);
	log_printf("address\t: 0x%x",ep->ExceptionRecord->ExceptionAddress);
	if (ep->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION) 
	{
		log_printf("write\t: 0x%x",ep->ExceptionRecord->ExceptionInformation[1]);
	} //if
	log_printf("chained\t: 0x%x",ep->ExceptionRecord->ExceptionRecord);
	log_printf("</font>");
	log_flush();
	return 0;
}

int InitialiseModularEngine(void)
{
	hookCount=serviceCount=0;
	hook=NULL;
	service=NULL;
	InitializeCriticalSection(&csHooks);
	InitializeCriticalSection(&csServices);

	mainThreadId=GetCurrentThreadId();
	DuplicateHandle(GetCurrentProcess(),GetCurrentThread(),GetCurrentProcess(),&hMainThread,THREAD_SET_CONTEXT,FALSE,0);

	log_printf("InitialiseModularEngine: initialised, APC object handle: %x",(DWORD)hMainThread);

	return 0;

}

void DestroyingModularEngine(void)
{	
	int i, j;
	HANDLE hSnap;
	hSnap=hSnap=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId());
	log_printf("<hr>core state dump <i>before</i> shutdown, service information will be dumped <i>at</i> shutdown, note that hook hashes may collide; hook name hashes are used for reducing strcmp().<hr>");
	if (hSnap) {
		MODULEENTRY32 moduleInfo;
		moduleInfo.dwSize=sizeof(MODULEENTRY32);
		if (Module32First(hSnap,&moduleInfo)) {
			for (;;) {
				log_printf("module: %s<br>\tusage=%d%%, address: %08x-%08x <br>.",moduleInfo.szModule,moduleInfo.ProccntUsage,moduleInfo.modBaseAddr,moduleInfo.modBaseAddr+moduleInfo.modBaseSize);
				moduleInfo.dwSize=sizeof(MODULEENTRY32);
				if (!Module32Next(hSnap,&moduleInfo)) break;
			} //for
		} //if
		log_printf(".");

		EnterCriticalSection(&csHooks);
		for (i=0; i<hookCount; i++) {
			log_printf("<b>hook</b> \"%s\", ",hook[i].name,hook[i].hookHash);
			if (hook[i].subscriberCount) {
				for (j=0;j<hook[i].subscriberCount;j++) {
					void *pfnHook=hook[i].subscriber[j].pfnHook;
					int badhook=0;
					/*
						hooks are checked again before shutdown because the caller
						maybe using a function from another DLL which maybe GetProcAddress()'d
						this wouldn't be valid after such a DLL was freed,

						It could be checked at the actual hook calling I suppose but this is
						pretty near the NotifyEventHook() that causes shutdown which can be a
						a high source of error.
					*/
					__try {
						if (IsBadCodePtr(pfnHook)) {
							badhook=1;
						} //if
					} __except (DumpErrorInformation(GetExceptionInformation())) {
						hook[i].subscriber[j].pfnHook=NULL;
						badhook=1;
					} //try
					if (!badhook && !log_modulefromaddress(hSnap,pfnHook,&moduleInfo)) {
						strcpy(moduleInfo.szModule,"unknown");
					} //if
					log_printf("\tsubscriber: address=0x%08x, module=%s",badhook?"didnt scan":pfnHook,moduleInfo.szModule);
				} //for
				log_printf(".");
			} else {
				log_printf("\tno subscribers");
			} //if
		} //for
		LeaveCriticalSection(&csHooks);

	} else {
		log_printf("<font color=red>CreateToolhelp32Snapshot failed, GetLastError()=%x</font>",GetLastError());
	} //if
	CloseHandle(hSnap);
	return;
}

void DestroyModularEngine(void)
{
	int i,useless=0;
	EnterCriticalSection(&csHooks);
	log_printf("DestroyModularEngine: destroying modular core (serviceCount=%d, hookCount=%d)\r\n",serviceCount,hookCount);
	LeaveCriticalSection(&csHooks);	

	log_printf("<hr>");
	log_printf("service information");
	log_printf("<hr>");

	for (i=0;i<serviceCount;i++) {
		log_printf("free service\t: \"%s\"",service[i].name);
		if (service[i].dwTimesLow || service[i].dwTimesHigh) {
			log_printf("\tlow\t: <b>%d</b> \t high\t: %d",service[i].dwTimesLow,service[i].dwTimesHigh);
		} else {
			log_printf("\tnot invoked.");
			useless++;
		} //if
		log_printf(".");
	} //if
	if (serviceCount && useless) {
		log_printf("<br>stats: out of <b>%d</b> service%s, <b>%d</b> were not called <b>at all</b><br>",serviceCount,serviceCount!=1?"s":"",useless);
	} //if

	for(i=0;i<hookCount;i++) {
		__try {
			if(hook[i].subscriberCount) { 
				free(hook[i].subscriber);
			} //if
		} __except (DumpErrorInformation(GetExceptionInformation())) {
		}  //try
	}
	if(hookCount) free(hook);
	if(serviceCount) free(service);
	DeleteCriticalSection(&csHooks);
	DeleteCriticalSection(&csServices);
	CloseHandle(hMainThread);
	log_printf("<b>DestroyModularEngine deinitialised OK</b>");
}

DWORD NameHashFunction(const char *szStr)
{
#if defined _M_IX86 && !defined _NUMEGA_BC_FINALCHECK
	__asm {		   //this breaks if szStr is empty
		xor  edx,edx
		xor  eax,eax
		mov  esi,szStr
		mov  al,[esi]
		xor  cl,cl
	lph_top:	 //only 4 of 9 instructions in here don't use AL, so optimal pipe use is impossible
		xor  edx,eax
		inc  esi
		xor  eax,eax
		and  cl,31
		mov  al,[esi]
		add  cl,5
		test al,al
		rol  eax,cl		 //rol is u-pipe only, but pairable
		                 //rol doesn't touch z-flag
		jnz  lph_top  //5 clock tick loop. not bad.

		xor  eax,edx
	}
#else
	DWORD hash=0;
	int i;
	int shift=0;
	for(i=0;szStr[i];i++) {
		hash^=szStr[i]<<shift;
		if(shift>24) hash^=(szStr[i]>>(32-shift))&0x7F;
		shift=(shift+5)&0x1F;
	}
	return hash;
#endif
}

///////////////////////////////HOOKS

static int FindHookByName(const char *name)
{
	int i;

	for(i=0;i<hookCount;i++)
		if(!strcmp(hook[i].name,name)) return i;
	return -1;
}

static int FindHookByHashAndName(DWORD Hash, const char *name)
{
	int i;
	for (i=0; i<hookCount; i++)
		if (hook[i].hookHash=Hash) 
		{
			if (!strcmp(hook[i].name, name)) return i;
		}
	return -1;
}

HANDLE CreateHookableEvent(const char *name)
{
	DWORD Hash = NameHashFunction(name);
	HANDLE ret;

	EnterCriticalSection(&csHooks);
	if (FindHookByHashAndName(Hash, name) != -1) 
	{
		LeaveCriticalSection(&csHooks);
		log_printf("<font color=red>CreateHookableEvent failure, tried to create \"%s\" but it already existed</font>",name);
		return NULL;
	}
	hook=(THookList*)realloc(hook,sizeof(THookList)*(hookCount+1));
	strcpy(hook[hookCount].name,name);
	hook[hookCount].hookHash=Hash;
	hook[hookCount].subscriberCount=0;
	hook[hookCount].subscriber=NULL;
	hookCount++;
	ret=(HANDLE)hookCount;
	LeaveCriticalSection(&csHooks);
	log_printf("CreateHookableEvent success, created \"%s\" with handle (%d), hash 0x%08x",name,(DWORD)ret,(DWORD)Hash);
	return ret;
}

int DestroyHookableEvent(HANDLE hEvent)
{
	int hookId=(int)hEvent-1;

	EnterCriticalSection(&csHooks);
	if(hookId>=hookCount || hookId<0) 
	{
		LeaveCriticalSection(&csHooks); 
		log_printf("<font color=red>DestroyHookableEvent: tried to delete %d event, but it was invalid</font>",(DWORD)hEvent);
		return 1;
	}
	if(hook[hookId].name[0]==0) {
		LeaveCriticalSection(&csHooks); 
		log_printf("<font color=red>DestroyHookableEvent: tried to redelete hook with %d (hook name unknown)</font>",(DWORD)hEvent);
		return 1;
	}
	log_printf("DestroyHookableEvent: deleting \"%s\" by request from %d", hook[hookId].name,(DWORD)hEvent);
	hook[hookId].name[0]=0;
	if(hook[hookId].subscriberCount) {
		free(hook[hookId].subscriber);
		hook[hookId].subscriber=NULL;
		hook[hookId].subscriberCount=0;
	}
	LeaveCriticalSection(&csHooks);
	return 0;
}

static int CallHookSubscribers(int hookId,WPARAM wParam,LPARAM lParam)
{
	int i,returnVal=0,isValidHook=0;

	EnterCriticalSection(&csHooks);
__try {
	if(hookId>=hookCount || hookId<0 || hook[hookId].name[0]==0) returnVal=-1;
	else {		
		//NOTE: We've got the critical section while all this lot are called. That's mostly safe, though.
		for(i=0;i<hook[hookId].subscriberCount;i++) {
			isValidHook=1;
			if(hook[hookId].subscriber[i].pfnHook!=NULL) {
				if(returnVal=hook[hookId].subscriber[i].pfnHook(wParam,lParam)) break;
			}
			else if(hook[hookId].subscriber[i].hwnd!=NULL) {
				if(returnVal=SendMessage(hook[hookId].subscriber[i].hwnd,hook[hookId].subscriber[i].message,wParam,lParam)) break;
			}//if
		}//for
	}
} __except (DumpErrorInformation(GetExceptionInformation())) {
	if (isValidHook) {
		log_printf("<font color=red>CallHookSubscribers: exception while calling \"%s\" with wParam=0x%x,lParam=0x%x</font>",hook[hookId].name,wParam,lParam);
		log_flush();
	} //if
} //if
	LeaveCriticalSection(&csHooks);
	return returnVal;
}

static void CALLBACK HookToMainAPCFunc(DWORD dwParam)
{
	THookToMainThreadItem *item=(THookToMainThreadItem*)dwParam;
	item->result=CallHookSubscribers(item->hookId,item->wParam,item->lParam);
	SetEvent(item->hDoneEvent);
	return;
}

int NotifyEventHooks(HANDLE hEvent,WPARAM wParam,LPARAM lParam)
{
	if(GetCurrentThreadId()!=mainThreadId) {
		THookToMainThreadItem item;

		item.hDoneEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
		item.hookId=(int)hEvent-1;
		item.wParam=wParam;
		item.lParam=lParam;

		QueueUserAPC(HookToMainAPCFunc,hMainThread,(DWORD)&item);
		WaitForSingleObject(item.hDoneEvent,INFINITE);
		CloseHandle(item.hDoneEvent);
		return item.result;
	}
	else
		return CallHookSubscribers((int)hEvent-1,wParam,lParam);
}

HANDLE HookEvent(const char *name,MIRANDAHOOK hookProc)
{
	int hookId;
	HANDLE ret;

	__try {
		if (IsBadCodePtr((void*)hookProc)) {
			
			log_printf("<font color=red>HookEvent: tried to hook \"%s\" with illegal MIRANDASERVICE function</font>",name);
			return NULL;
		} //if
	} __except(DumpErrorInformation(GetExceptionInformation())) {
		//
	} //try

	EnterCriticalSection(&csHooks);
	hookId=FindHookByName(name);
	if(hookId==-1) {
		LeaveCriticalSection(&csHooks);
		log_printf("<font color=red>HookEvent: hook \"%s\" not found, address=%x</font>",name,(DWORD)hookProc);
		return NULL;
	}
	hook[hookId].subscriber=(THookSubscriber*)realloc(hook[hookId].subscriber,sizeof(THookSubscriber)*(hook[hookId].subscriberCount+1));
	hook[hookId].subscriber[hook[hookId].subscriberCount].pfnHook=hookProc;
	hook[hookId].subscriber[hook[hookId].subscriberCount].hwnd=NULL;
	hook[hookId].subscriberCount++;
	ret=(HANDLE)((hookId<<16)|hook[hookId].subscriberCount);
	LeaveCriticalSection(&csHooks);
	log_printf("HookEvent: hooked \"%s\" with handle (%d)",name,ret);
	return ret;
}

HANDLE HookEventMessage(const char *name,HWND hwnd,UINT message)
{
	int hookId;
	HANDLE ret;

	EnterCriticalSection(&csHooks);
	hookId=FindHookByName(name);
	if(hookId==-1) {
		LeaveCriticalSection(&csHooks);
		log_printf("<font color=red>HookEventMessage: hooking %s with window (0x%x) message callback failed.</font>",name,hwnd);
		return NULL;
	}
	log_printf("HookEventMessage: creating %s window 0x%x hook for message callback using 0x%x message-id",name,hwnd,message);
	hook[hookId].subscriber=(THookSubscriber*)realloc(hook[hookId].subscriber,sizeof(THookSubscriber)*(hook[hookId].subscriberCount+1));
	hook[hookId].subscriber[hook[hookId].subscriberCount].pfnHook=NULL;
	hook[hookId].subscriber[hook[hookId].subscriberCount].hwnd=hwnd;
	hook[hookId].subscriber[hook[hookId].subscriberCount].message=message;
	hook[hookId].subscriberCount++;
	ret=(HANDLE)((hookId<<16)|hook[hookId].subscriberCount);
	LeaveCriticalSection(&csHooks);
	return ret;
}

int UnhookEvent(HANDLE hHook)
{
	int hookId=(int)hHook>>16;
	int subscriberId=((int)hHook&0xFFFF)-1;

	EnterCriticalSection(&csHooks);
	if(hookId>=hookCount || hookId<0) {
		LeaveCriticalSection(&csHooks); 
		log_printf("<font color=red>UnhookEvent: tried to unhook with (%d) handle, out of range</font>",hHook);
		return 1;
	}
	if(hook[hookId].name[0]==0) {
		LeaveCriticalSection(&csHooks);
		log_printf("<font color=red>UnhookEvent: tried to unhook with (%d) handle, hook does not exist</font>",hHook);
		return 1;
	}
	if(subscriberId>=hook[hookId].subscriberCount || subscriberId<0) {
		log_printf("<font color=red>UnhookEvent: tried to unhook from \"%s\" with invalid handle (%d)</font>",hook[hookId].name,hHook);
		LeaveCriticalSection(&csHooks); 		
		return 1;
	}
	hook[hookId].subscriber[subscriberId].pfnHook=NULL;
	hook[hookId].subscriber[subscriberId].hwnd=NULL;
	while(hook[hookId].subscriberCount && hook[hookId].subscriber[hook[hookId].subscriberCount-1].pfnHook==NULL && hook[hookId].subscriber[hook[hookId].subscriberCount-1].hwnd==NULL)
		hook[hookId].subscriberCount--;
	LeaveCriticalSection(&csHooks);
	return 0;
}

/////////////////////SERVICES

static __inline TServiceList *FindServiceByHash(DWORD hash)
{
	int first,last,mid;

	if(serviceCount==0) return NULL;
	first=0; last=serviceCount-1;
	if(hash>=service[last].nameHash) {
		if(hash==service[last].nameHash) return &service[last];
		return NULL;
	}
	for(;;) {
	  mid=(first+last)>>1;
	  if(hash>service[mid].nameHash) {
		  if(last-first<=1) break;
		  first=mid;
	  }
	  else if(hash<service[mid].nameHash) {
		  if(last-first<=1) break;
		  last=mid;
	  }
	  else return &service[mid];
	}
	return NULL;
}

static __inline TServiceList *FindServiceByName(const char *name)
{
	return FindServiceByHash(NameHashFunction(name));
}

HANDLE CreateServiceFunction(const char *name,MIRANDASERVICE serviceProc)
{
	DWORD hash = NameHashFunction(name);
	int i;
	EnterCriticalSection(&csServices);
	if (FindServiceByHash(hash) != NULL) {
		LeaveCriticalSection(&csServices);
		log_printf("<font color=red>CreateServiceFunction: tried to create duplicate service \"%s\" for %x </font>",name,(DWORD)serviceProc);
		return NULL;
	}
	__try {
		if (IsBadCodePtr((void*)serviceProc)) {
			LeaveCriticalSection(&csServices);
			log_printf("<font color=red>CreateServiceFunction: tried to create \"%s\" with illegal function pointer (%x)</font>",name,(DWORD)serviceProc);
			return NULL;
		} //if
	} __except(DumpErrorInformation(GetExceptionInformation())) {
	} //try
	service=(TServiceList*)realloc(service,sizeof(TServiceList)*(serviceCount+1));
	for(i=0;i<serviceCount;i++)
		if(hash<service[i].nameHash) break;
	MoveMemory(service+i+1,service+i,sizeof(TServiceList)*(serviceCount-i));
	strcpy(service[i].name,name);
	service[i].nameHash=hash;
	service[i].pfnService=serviceProc;
	service[i].dwTimesLow=service[i].dwTimesHigh=0;
	serviceCount++;
	LeaveCriticalSection(&csServices);
	log_printf("CreateServiceFunction: created service \"%s\" with function pointer %x",name,(DWORD)serviceProc);
	return (HANDLE)hash;
}

int DestroyServiceFunction(HANDLE hService)
{
	TServiceList *pService;
	int i;

	EnterCriticalSection(&csServices);
	pService=FindServiceByHash((DWORD)hService);
	if(pService==NULL) {
		LeaveCriticalSection(&csServices); 
		log_printf("<font color=red>DestroyServiceFunction: Tried to destroy service with handle (%d)</font>",(DWORD)hService);
		return 1;
	}
	i=(int)(pService-service);
	MoveMemory(service+i,service+i+1,sizeof(TServiceList)*(--serviceCount-i));
	LeaveCriticalSection(&csServices);
	return 0;
}

int ServiceExists(const char *name)
{
	int ret;
	EnterCriticalSection(&csServices);
	ret=FindServiceByName(name)!=NULL;
	LeaveCriticalSection(&csServices);
	return ret;
}

int CallService(const char *name,WPARAM wParam,LPARAM lParam)
{
	TServiceList *pService;
	MIRANDASERVICE pfnService;
	int rc;
	
	EnterCriticalSection(&csServices);
	pService=FindServiceByName(name);
	if(pService==NULL) {
		LeaveCriticalSection(&csServices);
		return CALLSERVICE_NOTFOUND;
	}
	pfnService=pService->pfnService;
	if (++pService->dwTimesLow==0) {
		pService->dwTimesHigh++;
	} //if
	LeaveCriticalSection(&csServices);
	__try {
		rc=((int (*)(WPARAM,LPARAM))pfnService)(wParam,lParam);
	} __except(DumpErrorInformation(GetExceptionInformation())) 
	{		
	} //try
	return rc;
}

static void CALLBACK CallServiceToMainAPCFunc(DWORD dwParam)
{
	TServiceToMainThreadItem *item = (TServiceToMainThreadItem*) dwParam;
	item->result = CallService(item->name, item->wParam, item->lParam);
	SetEvent(item->hDoneEvent);
}

int CallServiceSync(const char *name, WPARAM wParam, LPARAM lParam)
{
	if (name==NULL) return CALLSERVICE_NOTFOUND;
	// the service is looked up within the main thread, since the time it takes
	// for the APC queue to clear the service being called maybe removed.
	// even thou it may exists before the call, the critsec can't be locked between calls.
	if (GetCurrentThreadId() != mainThreadId)
	{
		TServiceToMainThreadItem item;
		item.wParam = wParam;
		item.lParam = lParam;
		item.name = name;
		item.hDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		QueueUserAPC(CallServiceToMainAPCFunc, hMainThread, (DWORD) &item);
		WaitForSingleObject(item.hDoneEvent, INFINITE);
		CloseHandle(item.hDoneEvent);
		return item.result;
	} else {
		return CallService(name, wParam, lParam);
	}
}
