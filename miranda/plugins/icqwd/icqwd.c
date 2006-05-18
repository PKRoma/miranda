#include "icqwd.h"
#include <stdio.h>
#include <string.h>

static PLUGININFO plugInfo = {
	sizeof(PLUGININFO), 
	"ICQwd", 
	PLUGIN_MAKE_VERSION(0,0,5,0),
	"Easily add people from websites and .icq files, just like good ol' ICQ",
	"egoDust",
	"egodust at miranda-im.org",
	"open source",	
	"http://www.miranda-im.org",
	0,
	0
};

/* plugin section */
PLUGINLINK *pluginLink;
HINSTANCE hInst;
ATOM aWatcherClass;
HWND hWatcher;

LRESULT CALLBACK WatcherWndProc(HWND hwnd, UINT msg, WPARAM wParam,LPARAM lParam)
{
	switch (msg) {
		case WM_COPYDATA:
		{
			ICQFILEINFO ifi;
			COPYDATASTRUCT *cds=(COPYDATASTRUCT*)lParam;
			ADDCONTACTSTRUCT acs;
			PROTOSEARCHRESULT psr;
			if (cds && cds->lpData && cds->cbData == sizeof(ICQFILEINFO) && cds->dwData==1) {
				/* copy the data out of the passed structure */
				memmove(&ifi,cds->lpData,sizeof(ICQFILEINFO));
				/* let the thread that called is go */
				ReplyMessage(0); 
				/* wOot! */
				acs.handleType=HANDLE_SEARCHRESULT;
				acs.szProto="ICQ";
				acs.psr=&psr;
				memset(&psr,0,sizeof(PROTOSEARCHRESULT));
				psr.cbSize=sizeof(PROTOSEARCHRESULT);
				psr.nick=ifi.uin;
				CallService(MS_ADDCONTACT_SHOW,(WPARAM)NULL,(LPARAM)&acs);
			} //if
			break;
		}
	} //switch
	return DefWindowProc(hwnd,msg,wParam,lParam);
}

void RegisterAssoc(void)
{
	// register .icq
	HKEY hkey;
	char szBuf[MAX_PATH], szExe[MAX_PATH*2];

	if (RegCreateKey(HKEY_CLASSES_ROOT,".icq\\shell\\open\\command",&hkey)==ERROR_SUCCESS) 
	{
		GetModuleFileName(hInst,szBuf,sizeof(szBuf));
		wsprintf(szExe,"RUNDLL32.EXE %s,icqCommit %%1",szBuf);
		RegSetValue(hkey,NULL,REG_SZ,szExe,strlen(szExe));
		RegCloseKey(hkey);
	}
	
	if (RegCreateKey(HKEY_CLASSES_ROOT,"MIME\\Database\\Content Type\\application/x-icq",&hkey)==ERROR_SUCCESS) 
	{	
		strcpy(szBuf,".icq");
		RegSetValueEx(hkey,"Extension",0,REG_SZ,szBuf,strlen(szBuf));
		RegCloseKey(hkey);
	}
	return;
}

void UnregisterAssoc(void)
{	
	return;
}

void RegisterWatcher(void)
{
	WNDCLASS wc;
	memset(&wc,0,sizeof(WNDCLASS));
	wc.lpfnWndProc=WatcherWndProc;
	wc.hInstance=hInst;
	wc.lpszClassName=WATCHERCLASS;
	aWatcherClass=RegisterClass(&wc);
	hWatcher=CreateWindowEx(
		0,
		WATCHERCLASS,"",
		0,					
		0,0,0,0,			
		NULL,				
		NULL,
		hInst,
		NULL
	);
	return;
}

void UnregisterWatcher(void)
{
	if (hWatcher) DestroyWindow(hWatcher);
	if (aWatcherClass) UnregisterClass(WATCHERCLASS,hInst);
	return;
}

PLUGININFO* __declspec(dllexport) MirandaPluginInfo(DWORD mirandaVersion)
{
	return &plugInfo;
}

int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink=link;
	DisableThreadLibraryCalls(hInst);
	RegisterWatcher();
	RegisterAssoc();
	return 0;
}

int __declspec(dllexport) Unload(void)
{
	UnregisterAssoc();
	UnregisterWatcher();
	return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{	
	hInst=hinstDLL;
	return TRUE;
}

/* file payload section */

BOOL CALLBACK EnumThreadWndProc(HWND hwnd,LPARAM lParam)
{
	char szBuf[64];
	if (GetClassName(hwnd,szBuf,sizeof(szBuf))) {
		if (!strcmp(szBuf,WATCHERCLASS)) {
			COPYDATASTRUCT cds;
			ICQFILEINFO *ifi=(ICQFILEINFO*)lParam;
			cds.dwData=1;
			cds.cbData=sizeof(ICQFILEINFO);
			cds.lpData=ifi;
			SendMessageTimeout(hwnd,WM_COPYDATA,(WPARAM)hwnd,(LPARAM)&cds,SMTO_ABORTIFHUNG|SMTO_BLOCK,150,NULL);
		} //if
	};
	return TRUE;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	char szBuf[32];	
	if (GetClassName(hwnd,szBuf,sizeof(szBuf))) {
		if (!strcmp(szBuf,"Miranda")) {
			EnumThreadWindows(GetWindowThreadProcessId(hwnd,NULL),EnumThreadWndProc,lParam);
		} //if
	} //if
	return TRUE;
}

int GetIcqInfo(char *szBuf,ICQFILEINFO *ifi)
{
	/* why not use strtok? */
	char *p,*v;
	p=strchr(szBuf,10);
	while (p != NULL) {
		*p=0;
		/* got the line, look for the = */
		v=strchr(szBuf,'=');
		if (v) {
			/* everything before 'v' is the name in szBuf, v++ is the value */
			*v=0;
			v++;
			if (szBuf && *v != 0) {
				/* all the .icq files I've seen only have UIN= in them */
				if (!strcmp(szBuf,"UIN")) {
					strncpy(ifi->uin,v,sizeof(ifi->uin));
					return 1;
				} //if
			} //if
		} //if
		szBuf=++p;
		p=strchr(szBuf,10);
	} //while
	return 0;
}

void __declspec(dllexport) CALLBACK icqCommit(HWND hwnd, HINSTANCE hInst, char *lpszCmdLine, int nCmdShow)
{
	FILE *fp;	
	if (lpszCmdLine) {
		fp=fopen(lpszCmdLine,"r");
		if (fp) {
			char szBuf[0x1000];
			memset(&szBuf,0,sizeof(szBuf));			
			if (fread(&szBuf,1,sizeof(szBuf),fp)) {
				ICQFILEINFO ifi;
				memset(&ifi,0,sizeof(ICQFILEINFO));
				ifi.cbSize=sizeof(ICQFILEINFO);
				if (GetIcqInfo(szBuf,&ifi)) {
					EnumWindows(EnumWindowsProc,(LPARAM)&ifi);
				} //if
			} //if
			fclose(fp);
		} //if
	} //if
	return;
}

