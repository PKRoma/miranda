#ifndef _FUSE_LOG_
#define _FUSE_LOG_
void log_printf(const char *szFmt,...);
void log_flush(void);
int log_modulefromaddress(HANDLE hSnap,void *address,MODULEENTRY32 *modInfo);
typedef struct {
	int cbSize;
	char *shortName;
	DWORD version;
	char *description;
	char *author;
	char *authorEmail;
	char *copyright;
	char *homepage;
	BYTE isTransient;
	int replacesDefaultModule;
} PLUGININFO;
typedef struct {
	HANDLE (*CreateHookableEvent)(const char *);
	int (*DestroyHookableEvent)(HANDLE);
	int (*NotifyEventHooks)(HANDLE,WPARAM,LPARAM);
	HANDLE (*HookEvent)(const char *,MIRANDAHOOK);
	HANDLE (*HookEventMessage)(const char *,HWND,UINT);
	int (*UnhookEvent)(HANDLE);
	HANDLE (*CreateServiceFunction)(const char *,MIRANDASERVICE);
	HANDLE (*CreateTransientServiceFunction)(const char *,MIRANDASERVICE);
	int (*DestroyServiceFunction)(HANDLE);
	int (*CallService)(const char *,WPARAM,LPARAM);
	int (*ServiceExists)(const char *);		  //v0.1.0.1+
} PLUGINLINK;
#endif