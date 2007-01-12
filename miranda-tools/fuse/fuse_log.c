#include "fuse_common.h"
#pragma hdrstop

FILE *fplog=NULL;

DWORD GetMirandaVersion(void)
{
	/* stolen from GetMirandaVersion */
	char filename[MAX_PATH];
	DWORD unused;
	DWORD verInfoSize,blockSize;
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
	return ver;
}

void log_dump_miranda_info(void)
{
	/* stolen from GetMirandaVersionText */
	char filename[MAX_PATH],*productVersion;
	DWORD unused;
	DWORD verInfoSize,blockSize;
	PVOID pVerInfo;

	GetModuleFileName(NULL,filename,sizeof(filename));
	verInfoSize=GetFileVersionInfoSize(filename,&unused);
	pVerInfo=malloc(verInfoSize);
	GetFileVersionInfo(filename,0,verInfoSize,pVerInfo);
	VerQueryValue(pVerInfo,"\\StringFileInfo\\000004b0\\ProductVersion",&productVersion,&blockSize);
	log_printf("Running Miranda %s",productVersion);
	free(pVerInfo);
}

void log_dump_os_info(void)
{
	OSVERSIONINFO ovi;
	ovi.dwOSVersionInfoSize=sizeof(ovi);
	if (GetVersionEx(&ovi)) {
		log_printf("Running Windows %d.%d (Build: %d, Platform-Id: %d)\r\nAdditional Info: %s",ovi.dwMajorVersion,ovi.dwMinorVersion,ovi.dwBuildNumber,ovi.dwPlatformId,ovi.szCSDVersion);
	} //if
	return;
}

static DWORD g_dwMirandaVersion=0;
static void *g_stack_ptr=NULL;

void log_dump_plugin_info(const char *szFile, WIN32_FIND_DATA *fd, DWORD *cunts)
{
	HINSTANCE hPlug;
	hPlug=LoadLibrary(szFile);
	if (hPlug) {
		PLUGININFO* (*MirandaPluginInfo)(DWORD);
		log_printf("querying %s...",fd->cFileName);		
		log_flush();
		MirandaPluginInfo=(void*)GetProcAddress(hPlug,"MirandaPluginInfo");
		if (MirandaPluginInfo) {
			PLUGININFO *pi=NULL;
			int bad_cdecl=0;

			// detect CDECL
			__asm {
				/*
				 cli,sti won't execute under XP/2000 without checking for IOPL access
				 since we can't directly modify ESP to point back where it was supposed to be,
				 we can modify with instructions such as ADD/SUB, for underflows caused by STDCALL
				 popping "MirandaVersion" off the stack, however the export may have pushed
				 or on pupose popped lots of information -- if it's overwritten anything
				 whilst doing such, we're screwed anyway.

				 This all this is happening, we can't log of pending doom, since all the
				 logging functions internally need the stack and I don't fancy writing out
				 strings using assembler.

				 */
				mov  eax, g_dwMirandaVersion
				push eax
				mov  g_stack_ptr,esp
				call MirandaPluginInfo
				mov  edx,g_stack_ptr
				cmp  esp,edx
				je   got_cdecl

				inc  bad_cdecl
				jl   underflow
				// esp is higher, too many pops
				mov  ebx,esp
				sub  ebx,edx;
				sub  esp,ebx
				jmp  got_cdecl
			underflow:
				// esp is lower, too many pushes
				mov ebx,esp
				sub edx,ebx
				add esp,edx
			got_cdecl:
				pop  ebx
				mov  pi,eax
			} //asm
			if (bad_cdecl) {
				log_printf("\tdubious: %s may damage Miranda (reason: MirandaPluginInfo() is fucked!)",fd->cFileName);
				(*cunts)++;
			} else {
				if (pi) {
					if (pi->cbSize==sizeof(PLUGININFO)) {
						log_printf("\tversion\t: %d,%d,%d,%d",(pi->version>>24)&0xFF,(pi->version>>16)&0xFF,(pi->version>>8)&0xFF,(pi->version)&0xFF);
						log_printf("\tname\t: %s",pi->shortName);
						log_printf("\tauthor\t: %s",pi->author);
					} else {
						log_printf("\tdubious:PLUGININFO request returned a mismatch structure size of %d bytes (hint: compiler alignment issues)",pi->cbSize);
						(*cunts)++;
					} //if
				} else {
					log_printf("\tmaybe dubious: PLUGININFO request was returned with NULL reply (no worries)");
				} //if
			} //if
		} else {
			log_printf("maybe a problem, %s does not have MirandaPluginInfo export function",fd->cFileName);
		} //if

		FreeLibrary(hPlug);
	} else {
		log_printf("error %x, loading of \"%s\" failed",GetLastError(),szFile);
	} //if
	log_printf(".");
	return;
}

void log_dump_plugins_info(void)
{
	char szBuf[MAX_PATH];
	char szFile[MAX_PATH];
	char *sz;
	WIN32_FIND_DATA fd;
	HANDLE hFind;
	DWORD goodies=0,cunts=0;

	g_dwMirandaVersion=GetMirandaVersion();

	log_printf("-");
	GetModuleFileName(NULL,szBuf,sizeof(szBuf));
	sz=strrchr(szBuf,'\\');
	if (sz) *sz=0;
	strcat(szBuf,"\\Plugins\\");
	SetCurrentDirectory(szBuf);
	log_printf("preparing to load external modules, using x86=%s<br>", _M_IX86?"yes":"no");
	log_flush(); // put everything to disk incase things go tits up.
	hFind=FindFirstFile("*.dll",&fd);
	while (hFind !=  INVALID_HANDLE_VALUE) 
	{
		if (!(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
			_snprintf(szFile,sizeof(szFile), "%s%s",szBuf,fd.cFileName);
			log_dump_plugin_info(szFile,&fd,&cunts);
			goodies++;
		} //if
		if (!FindNextFile(hFind,&fd)) break;
	}
	FindClose(&fd);

	if (!cunts) {
		log_printf("plugin condition: no problems found, looked at %d plugin%s",goodies,goodies!=1?"s":"");
	} else {
		log_printf("plugin condition: there were problems with %d plugin%s, see above.",cunts,cunts!=1?"s":"");
	} //if
	log_printf("");
	
	return;
} 

int fuse_log_init(void)
{
	char szBuf[MAX_PATH];
	char *sz;
	GetModuleFileName(NULL,szBuf,sizeof(szBuf));
	sz=strrchr(szBuf,'\\');
	if (sz) *sz=0;
	strcat(szBuf,"\\miranda_core_log.html");
	fplog=fopen(szBuf,"w+t");
	if (fplog) {
		fprintf(fplog,"<pre>fuse (compiled on %s at %s)\r\n",__DATE__,__TIME__);
		log_printf("logfile: \"%s\"<br>",szBuf);
		log_dump_miranda_info();
		log_dump_os_info();
		log_dump_plugins_info();
	} //if
	return 0;
}

int fuse_log_deinit(void)
{
	if (fplog) {
		fclose(fplog);
	} //if
	return 0;
}

void log_printf(const char *szFmt,...)
{
	extern FILE *fplog;
	if (fplog) {
		char sz[1024];
		va_list va;
		va_start(va,szFmt);
		_vsnprintf(sz,sizeof(sz),szFmt,va);
		va_end(va);
		fprintf(fplog,"%s<br>",sz);
	}
	return;
}

void log_flush(void)
{
	if (fplog) fflush(fplog);
	return;
}

int log_modulefromaddress(HANDLE hSnap,void *address,MODULEENTRY32 *modInfo)
{
	if (hSnap) {
		MODULEENTRY32 moduleInfo;
		moduleInfo.dwSize=sizeof(moduleInfo);
		if (Module32First(hSnap,&moduleInfo)) {
			for (;;) {
				DWORD dwStart=(DWORD)moduleInfo.modBaseAddr,dwEnd=dwStart+moduleInfo.modBaseSize;
				if ((DWORD)address >= dwStart && (DWORD)address <= dwEnd) {
					memmove(modInfo,&moduleInfo,sizeof(moduleInfo));
					return 1;					
				} //if
				moduleInfo.dwSize=sizeof(moduleInfo);
				if (!Module32Next(hSnap,&moduleInfo)) break;
			} //for
		} //if		
	} //if
	return 0;
}
