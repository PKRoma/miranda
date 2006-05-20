#include "fuse_common.h"
#pragma hdrstop

int InitialiseModularEngine(void);
void DestroyModularEngine(void);
void DestroyingModularEngine(void);
HANDLE CreateHookableEvent(const char *name);
int DestroyHookableEvent(HANDLE hEvent);
int NotifyEventHooks(HANDLE hEvent,WPARAM wParam,LPARAM lParam);
HANDLE HookEvent(const char *name,MIRANDAHOOK hookProc);
HANDLE HookEventMessage(const char *name,HWND hwnd,UINT message);
int UnhookEvent(HANDLE hHook);
HANDLE CreateServiceFunction(const char *name,MIRANDASERVICE serviceProc);
int DestroyServiceFunction(HANDLE hService);
int CallService(const char *name,WPARAM wParam,LPARAM lParam);
int ServiceExists(const char *name);
int CallServiceSync(const char *name, WPARAM wParam, LPARAM lParam);
int fuse_log_init(void);
int fuse_log_deinit(void);


static DWORD gStartTick;


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}

int __declspec(dllexport) fuse_ctrl(DWORD dwMsg, void* Param) 
{	
	int rc;
	switch (dwMsg) {
		case FUSE_DEATH:
		{
			DestroyingModularEngine();
			return 0;
		}
		case FUSE_DEFMOD:
		{	
			if (Param) {
				log_printf("LoadDefaultModules state %d (zero is good)", *((int*)Param));
			} else {
				log_printf("problem: LoadDefaultModules state unknown!");
			} //if
			return 0;
		}
		case FUSE_INIT:
		{
			gStartTick=GetTickCount();
			fuse_log_init();
			log_printf("starting modular engine..");
			rc=InitialiseModularEngine();
			log_printf("done, module engine state: %s",rc?"<font color=red>Failed</font>!":"OK");
			if (!rc) {
				FUSE_LINK *fl=Param;
				log_printf("processing FUSE_LINK");
				if (fl && fl->cbSize==sizeof(FUSE_LINK)) {
					fl->CreateHookableEvent=CreateHookableEvent;
					fl->DestroyHookableEvent=DestroyHookableEvent;
					fl->NotifyEventHooks=NotifyEventHooks;
					fl->HookEvent=HookEvent;
					fl->HookEventMessage=HookEventMessage;
					fl->UnhookEvent=UnhookEvent;
					fl->CreateServiceFunction=CreateServiceFunction;
					fl->CreateTransientServiceFunction=CreateServiceFunction;
					fl->DestroyServiceFunction=DestroyServiceFunction;
					fl->CallService=CallService;
					fl->ServiceExists=ServiceExists;
					fl->CallServiceSync=CallServiceSync;
					log_printf("<font color=green>FUSE_LINK given to Miranda, reported size of FUSE_LINK is %d bytes</font>", fl->cbSize);					
				} //if
			}
			return rc;
		}
		case FUSE_DEINIT:
		{
			log_printf("<font color=green>Got FUSE_DEINIT, cleaning up..</font>");
			DestroyModularEngine();
			log_printf("DestroyModuleEngine() returned.");
			log_printf("Runtime\t: %d seconds",GetTickCount()-gStartTick);
			fuse_log_deinit();
			return 0;
		}
	} //switch
	return 0;
}
