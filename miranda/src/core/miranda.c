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

#include "commonheaders.h"

int InitialiseModularEngine(void);
void DestroyingModularEngine(void);
void DestroyModularEngine(void);
void UnloadDatabaseModule(void);
int UnloadNewPluginsModule(void);

DWORD (WINAPI *MyMsgWaitForMultipleObjectsEx)(DWORD,CONST HANDLE*,DWORD,DWORD,DWORD);
static DWORD MsgWaitForMultipleObjectsExWorkaround(DWORD nCount, const HANDLE *pHandles, 
	DWORD dwMsecs, DWORD dwWakeMask, DWORD dwFlags);

static HANDLE hOkToExitEvent,hModulesLoadedEvent;
HANDLE hShutdownEvent,hPreShutdownEvent;
static HANDLE hWaitObjects[MAXIMUM_WAIT_OBJECTS-1];
static char *pszWaitServices[MAXIMUM_WAIT_OBJECTS-1];
static int waitObjectCount=0;
HANDLE hStackMutex,hMirandaShutdown,hThreadQueueEmpty;

struct THREAD_WAIT_ENTRY {
	DWORD dwThreadId;	// valid if hThread isn't signalled
	HANDLE hThread;
};
struct THREAD_WAIT_ENTRY *WaitingThreads=NULL;
int WaitingThreadsCount=0;

struct FORK_ARG {
	HANDLE hEvent;
	void (__cdecl *threadcode)(void*);
	unsigned (__stdcall *threadcodeex)(void*);
	void *arg;
};

void __cdecl forkthread_r(void * arg)
{	
	struct FORK_ARG * fa = (struct FORK_ARG *) arg;
	void (*callercode)(void*)=fa->threadcode;
	void * cookie=fa->arg;
	CallService(MS_SYSTEM_THREAD_PUSH,0,0);
	SetEvent(fa->hEvent);
	__try {
		callercode(cookie);
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

unsigned __stdcall forkthreadex_r(void * arg)
{
	struct FORK_ARG *fa=(struct FORK_ARG *)arg;
	unsigned (__stdcall * threadcode) (void *)=fa->threadcodeex;
	void *cookie=fa->arg;
	unsigned long rc;
	
	CallService(MS_SYSTEM_THREAD_PUSH,0,0);
	SetEvent(fa->hEvent);
	__try {
		rc=threadcode(cookie);
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
	rc=_beginthreadex(sec,stacksize,forkthreadex_r,(void *)&fa,0,thraddr);
	if (rc) {
		WaitForSingleObject(fa.hEvent,INFINITE);
	} 
	CloseHandle(fa.hEvent);
	return rc;
}

static void __stdcall DummyAPCFunc(DWORD dwArg)
{
	/* called in the context of thread that cleared it's APC queue */
	return;
}

static int MirandaWaitForMutex(HANDLE hEvent)
{
	for (;;) {
		// will get WAIT_IO_COMPLETE for QueueUserAPC() which isnt a result
		DWORD rc=MsgWaitForMultipleObjectsExWorkaround(1, &hEvent, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE);
		if ( rc == WAIT_OBJECT_0 + 1 ) {
			MSG msg;
			while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) {
				if ( IsDialogMessage(msg.hwnd, &msg) ) continue;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		} else if ( rc==WAIT_OBJECT_0 ) {
			// got object
			return 1;
		} else if ( rc==WAIT_ABANDONED_0 || rc == WAIT_FAILED ) return 0;		
	}
}

static void UnwindThreadWait(void)
{
	// acquire the list and wake up any alertable threads
	if ( MirandaWaitForMutex(hStackMutex) ) {
		int j;
		for (j=0;j<WaitingThreadsCount;j++)
			QueueUserAPC(DummyAPCFunc,WaitingThreads[j].hThread, 0);
		ReleaseMutex(hStackMutex);
	}
	// wait til the thread list is empty 
	MirandaWaitForMutex(hThreadQueueEmpty);
}

int UnwindThreadPush(WPARAM wParam,LPARAM lParam)
{
	ResetEvent(hThreadQueueEmpty); // thread list is not empty
	if (WaitForSingleObject(hStackMutex,INFINITE)==WAIT_OBJECT_0) 
	{
		HANDLE hThread=0;		
		DuplicateHandle(GetCurrentProcess(),GetCurrentThread(),GetCurrentProcess(),&hThread,THREAD_SET_CONTEXT,FALSE,0);
		WaitingThreads=realloc(WaitingThreads,sizeof(struct THREAD_WAIT_ENTRY)*(WaitingThreadsCount+1));
		WaitingThreads[WaitingThreadsCount].hThread=hThread;
		WaitingThreads[WaitingThreadsCount].dwThreadId=GetCurrentThreadId();
		WaitingThreadsCount++;
#ifdef _DEBUG
		{
			char szBuf[64];
			_snprintf(szBuf,sizeof(szBuf),"*** pushing thread (%x)\n",GetCurrentThreadId());
			OutputDebugString(szBuf);
		}
#endif
		ReleaseMutex(hStackMutex);		
	} //if
	return 0;
}

int UnwindThreadPop(WPARAM wParam,LPARAM lParam)
{
	if (WaitForSingleObject(hStackMutex,INFINITE)==WAIT_OBJECT_0)
	{
		DWORD dwThreadId=GetCurrentThreadId();
		int j;
		for (j=0;j<WaitingThreadsCount;j++) {
			if (WaitingThreads[j].dwThreadId == dwThreadId) {
				CloseHandle(WaitingThreads[j].hThread);
				WaitingThreadsCount--;
				if (j<WaitingThreadsCount) memmove(&WaitingThreads[j],&WaitingThreads[j+1],(WaitingThreadsCount-j) * sizeof(struct THREAD_WAIT_ENTRY));
				if (!WaitingThreadsCount)
				{
					free(WaitingThreads);
					WaitingThreads=NULL;
					WaitingThreadsCount=0;
					ReleaseMutex(hStackMutex);
					SetEvent(hThreadQueueEmpty); // thread list is empty now
					return 0;
				} else {
					WaitingThreads=realloc(WaitingThreads,sizeof(struct THREAD_WAIT_ENTRY)*WaitingThreadsCount);
				} //if
				ReleaseMutex(hStackMutex);
				return 0;
			} //if
		} //for
		ReleaseMutex(hStackMutex);
	} //if
	return 1;
}

int MirandaIsTerminated(WPARAM wParam,LPARAM lParam)
{
	return WaitForSingleObject(hMirandaShutdown,0)==WAIT_OBJECT_0;
}

static void __cdecl compactHeapsThread(void *dummy)
{
	while (!Miranda_Terminated()) 
	{
		HANDLE hHeaps[256];
		DWORD hc;
		SleepEx((1000*60)*5,TRUE); // every 5 minutes
		hc=GetProcessHeaps(255,(PHANDLE)&hHeaps);
		if (hc != 0 && hc < 256) {
			DWORD j;
			for (j=0;j<hc;j++) HeapCompact(hHeaps[j],0);
		}		
	} //while
}

static void InsertRegistryKey(void)
{
	if(DBGetContactSettingByte(NULL,"_Sys","CreateRegKey",1)) {
		HKEY hKey;	
		DWORD dw;
		if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Miranda",0,NULL,0,KEY_CREATE_SUB_KEY|KEY_SET_VALUE,NULL,&hKey,&dw)==ERROR_SUCCESS) {
			char str[MAX_PATH],*str2;
			GetModuleFileName(NULL,str,sizeof(str));
			str2=strrchr(str,'\\');
			if(str2!=NULL) *str2=0;
			RegSetValueEx(hKey,"Install_Dir",0,REG_SZ,(PBYTE)str,lstrlen(str)+1);
			RegCloseKey(hKey);
		}
	}
}

DWORD CALLBACK APCWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg==WM_NULL) SleepEx(0,TRUE);
	return DefWindowProc(hwnd,msg,wParam,lParam);
}

HWND hAPCWindow=NULL;
void (*SetIdleCallback) (void)=NULL;

static int SystemSetIdleCallback(WPARAM wParam, LPARAM lParam)
{
	if (lParam && SetIdleCallback==NULL) {
		SetIdleCallback=(void (*)(void))lParam;
		return 1;
	}
	return 0;
}

static DWORD dwEventTime=0;
void checkIdle(MSG * msg)
{	
	switch(msg->message) {
		case WM_MOUSEACTIVATE:
		case WM_MOUSEMOVE:
		case WM_CHAR:
		{
			dwEventTime = GetTickCount();			
		}
	}
}

static int SystemGetIdle(WPARAM wParam, LPARAM lParam)
{
	if ( lParam ) *(DWORD*)lParam = dwEventTime;
	return 0;
}

static DWORD MsgWaitForMultipleObjectsExWorkaround(DWORD nCount, const HANDLE *pHandles, 
	DWORD dwMsecs, DWORD dwWakeMask, DWORD dwFlags)
{
	if ( MyMsgWaitForMultipleObjectsEx != NULL )
		return MyMsgWaitForMultipleObjectsEx(nCount, pHandles, dwMsecs, dwWakeMask, dwFlags);
	// can wait for msgs and objects but not APC, or objects and APC but not msgs!
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	DWORD myPid=0;
	int messageloop=1;

	BOOL (WINAPI *MyPeekMessage) (LPMSG,HWND,UINT,UINT,UINT);
	BOOL (WINAPI *MyIsDialogMessage) (HWND,LPMSG);
	LRESULT (WINAPI *MyDispatchMessage) (LPMSG);

	MyPeekMessage=IsWinVerNT() ? (BOOL (WINAPI *)(LPMSG,HWND,UINT,UINT,UINT))GetProcAddress(GetModuleHandle("user32"),"PeekMessageW") : PeekMessageA;
	MyIsDialogMessage=IsWinVerNT() ? (BOOL (WINAPI *)(HWND,LPMSG))GetProcAddress(GetModuleHandle("user32"),"IsDialogMessageW") : IsDialogMessageA;
	MyDispatchMessage=IsWinVerNT() ? (LRESULT (WINAPI *)(LPMSG))GetProcAddress(GetModuleHandle("user32"),"DispatchMessageW") : DispatchMessageA;

#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	
	if (InitialiseModularEngine())
	{
		NotifyEventHooks(hShutdownEvent,0,0);
		UnloadDatabaseModule();
		UnloadNewPluginsModule();
		DestroyModularEngine();
		return 1;
	}
	InsertRegistryKey();
	hAPCWindow=CreateWindowEx(0,"STATIC",NULL,0, 0,0,0,0, NULL,NULL,NULL,NULL); // lame
	SetWindowLong(hAPCWindow,GWL_WNDPROC,(LONG)APCWndProc);
	hStackMutex=CreateMutex(NULL,FALSE,NULL);
	hMirandaShutdown=CreateEvent(NULL,TRUE,FALSE,NULL);
	hThreadQueueEmpty=CreateEvent(NULL,TRUE,TRUE,NULL);
	NotifyEventHooks(hModulesLoadedEvent,0,0);
	MyMsgWaitForMultipleObjectsEx=(DWORD (WINAPI *)(DWORD,CONST HANDLE*,DWORD,DWORD,DWORD))GetProcAddress(GetModuleHandle("user32"),"MsgWaitForMultipleObjectsEx");
	forkthread(compactHeapsThread,0,NULL);	
	CreateServiceFunction(MS_SYSTEM_SETIDLECALLBACK,SystemSetIdleCallback);
	CreateServiceFunction(MS_SYSTEM_GETIDLE, SystemGetIdle);
	dwEventTime=GetTickCount();
	myPid=GetCurrentProcessId();
	for(;messageloop;) {
		MSG msg;
		DWORD rc;
		BOOL dying=FALSE;
		rc=MsgWaitForMultipleObjectsExWorkaround(waitObjectCount, hWaitObjects, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE);
		if ( rc >= WAIT_OBJECT_0 && rc < WAIT_OBJECT_0 + waitObjectCount) {
			rc -= WAIT_OBJECT_0;
			CallService(pszWaitServices[rc], (WPARAM) hWaitObjects[rc], 0);
		}
		// 
		while ( MyPeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) {
			if ( msg.message != WM_QUIT ) {
				HWND h=GetForegroundWindow();
				DWORD pid = 0;
				checkIdle(&msg);
				if ( h != NULL && GetWindowThreadProcessId(h,&pid) && pid==myPid 
					&& GetClassLong(h, GCW_ATOM)==32770 ) {
					if ( MyIsDialogMessage(h, &msg) ) continue;
				}
				TranslateMessage(&msg);
				MyDispatchMessage(&msg);						
				if ( SetIdleCallback != NULL ) 
					SetIdleCallback();
			} else if ( !dying ) {
				dying++;
				SetEvent(hMirandaShutdown);
				NotifyEventHooks(hPreShutdownEvent, 0, 0);
				DestroyingModularEngine();
				// this spins and processes the msg loop, objects and APC.
				UnwindThreadWait();
				NotifyEventHooks(hShutdownEvent, 0, 0);
				// if the hooks generated any messages, it'll get processed before the second WM_QUIT
				PostQuitMessage(0);
			} else if ( dying ) {
				messageloop=0;	
			}
		} // while
	}
	UnloadDatabaseModule();  
	UnloadNewPluginsModule();
	DestroyModularEngine();
	CloseHandle(hStackMutex);
	CloseHandle(hMirandaShutdown);
	CloseHandle(hThreadQueueEmpty);
	DestroyWindow(hAPCWindow);
	return 0;
}

static int SystemShutdownProc(WPARAM wParam,LPARAM lParam)
{
	return 0;
}

static int OkToExit(WPARAM wParam,LPARAM lParam)
{
	return NotifyEventHooks(hOkToExitEvent,0,0)==0;
}

static int GetMirandaVersion(WPARAM wParam,LPARAM lParam)
{
	char filename[MAX_PATH];
	DWORD unused;
	DWORD verInfoSize;
	UINT blockSize;
	PVOID pVerInfo;
	VS_FIXEDFILEINFO *vsffi;
	DWORD ver;

	GetModuleFileName(NULL,filename,sizeof(filename));
	verInfoSize=GetFileVersionInfoSize(filename,&unused);
	pVerInfo=malloc(verInfoSize);
	GetFileVersionInfo(filename,0,verInfoSize,pVerInfo);
	VerQueryValue(pVerInfo,"\\",(PVOID*)&vsffi,&blockSize);
	ver=(((vsffi->dwProductVersionMS>>16)&0xFF)<<24)|
	    ((vsffi->dwProductVersionMS&0xFF)<<16)|
		(((vsffi->dwProductVersionLS>>16)&0xFF)<<8)|
		(vsffi->dwProductVersionLS&0xFF);
	free(pVerInfo);
	return (int)ver;
}

static int GetMirandaVersionText(WPARAM wParam,LPARAM lParam)
{
	char filename[MAX_PATH],*productVersion;
	DWORD unused;
	DWORD verInfoSize;
	UINT blockSize;
	PVOID pVerInfo;

	GetModuleFileName(NULL,filename,sizeof(filename));
	verInfoSize=GetFileVersionInfoSize(filename,&unused);
	pVerInfo=malloc(verInfoSize);
	GetFileVersionInfo(filename,0,verInfoSize,pVerInfo);
	VerQueryValue(pVerInfo,"\\StringFileInfo\\000004b0\\ProductVersion",(void*)&productVersion,&blockSize);
	lstrcpyn((char*)lParam,productVersion,wParam);
	free(pVerInfo);
	return 0;
}

int WaitOnHandle(WPARAM wParam,LPARAM lParam)
{
	if(waitObjectCount>=MAXIMUM_WAIT_OBJECTS-1) return 1;
	hWaitObjects[waitObjectCount]=(HANDLE)wParam;
	pszWaitServices[waitObjectCount]=(char*)lParam;
	waitObjectCount++;
	return 0;
}

static int RemoveWait(WPARAM wParam,LPARAM lParam)
{
	int i;

	for(i=0;i<waitObjectCount;i++)
		if(hWaitObjects[i]==(HANDLE)wParam) break;
	if(i==waitObjectCount) return 1;
	waitObjectCount--;
	MoveMemory(&hWaitObjects[i],&hWaitObjects[i+1],sizeof(HANDLE)*(waitObjectCount-i));
	MoveMemory(&pszWaitServices[i],&pszWaitServices[i+1],sizeof(char*)*(waitObjectCount-i));
	return 0;
}

int GetMemoryManagerInterface(WPARAM wParam, LPARAM lParam)
{
	struct MM_INTERFACE *mmi = (struct MM_INTERFACE*) lParam;
	if (mmi || mmi->cbSize == sizeof(struct MM_INTERFACE)) 
	{
		mmi->mmi_malloc = malloc;
		mmi->mmi_realloc = realloc;
		mmi->mmi_free = free;
		return 0;
	}
	return 1;
}

int LoadSystemModule(void)
{
	InitCommonControls();
	hShutdownEvent=CreateHookableEvent(ME_SYSTEM_SHUTDOWN);
	hPreShutdownEvent=CreateHookableEvent(ME_SYSTEM_PRESHUTDOWN);
	hModulesLoadedEvent=CreateHookableEvent(ME_SYSTEM_MODULESLOADED);
	hOkToExitEvent=CreateHookableEvent(ME_SYSTEM_OKTOEXIT);	
	HookEvent(ME_SYSTEM_SHUTDOWN,SystemShutdownProc);
	CreateServiceFunction(MS_SYSTEM_THREAD_PUSH,UnwindThreadPush);
	CreateServiceFunction(MS_SYSTEM_THREAD_POP,UnwindThreadPop);
	CreateServiceFunction(MS_SYSTEM_TERMINATED,MirandaIsTerminated);
	CreateServiceFunction(MS_SYSTEM_OKTOEXIT,OkToExit);	
	CreateServiceFunction(MS_SYSTEM_GETVERSION,GetMirandaVersion);
	CreateServiceFunction(MS_SYSTEM_GETVERSIONTEXT,GetMirandaVersionText);
	CreateServiceFunction(MS_SYSTEM_WAITONHANDLE,WaitOnHandle);
	CreateServiceFunction(MS_SYSTEM_REMOVEWAIT,RemoveWait);
	CreateServiceFunction(MS_SYSTEM_GET_MMI,GetMemoryManagerInterface);
	return 0;
}

