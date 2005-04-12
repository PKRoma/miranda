/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2004 Miranda ICQ/IM project, 
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
#ifndef _COMMON_HEADERS_H_
#define _COMMON_HEADERS_H_ 1

//#include "AggressiveOptimize.h"

#include <malloc.h>

#ifdef _DEBUG
#	define _CRTDBG_MAP_ALLOC
#	include <stdlib.h>
#	include <crtdbg.h>
#endif

#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <time.h>
#include <stddef.h>
#include <process.h>
#include <io.h>
#include <string.h>
#include <direct.h>
#include "m_clist.h"
#include "resource.h"
#include "forkthread.h"
#include <win2k.h>

#include <newpluginapi.h>
#include <m_system.h>
#include <m_database.h>
#include <m_langpack.h>
#include <m_button.h>
#include <m_options.h>
#include <m_protosvc.h>
#include <m_utils.h>
#include <m_skin.h>
#include <m_contacts.h>
#include <m_plugins.h>
#include "m_genmenu.h"
#include "genmenu.h"
#include "m_clui.h"
#include "m_clc.h"
#include "clc.h"
#include "clist.h"
#include "icolib.h"
#include "dblists.h"
#include <m_userinfo.h>
#include ".\CLUIFrames\cluiframes.h"
#include ".\CLUIFrames\m_cluiframes.h"
#include  "m_metacontacts.h"

#define CLS_CONTACTLIST 1

// shared vars
extern HINSTANCE g_hInst;

/* most free()'s are invalid when the code is executed from a dll, so this changes
 all the bad free()'s to good ones, however it's still incorrect code. The reasons for not
 changing them include:

  * DBFreeVariant has a CallService() lookup
  * free() is executed in some large loops to do with clist creation of group data
  * easy search and replace

*/

extern struct MM_INTERFACE memoryManagerInterface;

#define alloc(n) mir_alloc(n)
//#define free(ptr) mir_free(ptr)
//#define realloc(ptr,size) mir_realloc(ptr,size)


#define mir_alloc(n) memoryManagerInterface.mmi_malloc(n)
#define mir_free(ptr) mir_free_proxy(ptr)
#define mir_realloc(ptr,size) memoryManagerInterface.mmi_realloc(ptr,size)

#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW 0x00020000	
#endif

#ifndef MYCMP
#define MYCMP 1

static int mir_free_proxy(void *ptr)
{
	if (ptr==NULL||IsBadCodePtr(ptr))
	{
		char buf[256];
		wsprintf(buf,"Bad code ptr in mir_free_proxy ptr: %x\r\n",ptr);
		//ASSERT("Bad code ptr");
		DebugBreak();
		OutputDebugStr(buf);
		return 0;
	}
	memoryManagerInterface.mmi_free(ptr);
	return 0;

}

static int __cdecl MyStrCmp (const char *a, const char *b)
{
	
	if (a==NULL&&b==NULL) return 0;
	if ((int)a<1000||(int)b<1000||IsBadCodePtr((FARPROC)a)||IsBadCodePtr((FARPROC)b)) 
	{
		return 1;
	}
	//OutputDebugStr("MY\r\n");
	//undef();
	return (strcmp(a,b));
};

static int __cdecl MyStrLen (const char *a)
{
	
	if (a==NULL) return 0;
	if ((int)a<1000||IsBadCodePtr((FARPROC)a)) 
	{
		return 0;
	}
	//OutputDebugStr("MY\r\n");
	//undef();
	return (strlen(a));
};

#define strcmp(a,b) MyStrCmp(a,b)
#define strlen(a) MyStrLen(a)
#endif



__inline void *mir_calloc( size_t num, size_t size )
{
 	void *p=mir_alloc(num*size);
	if (p==NULL) return NULL;
	memset(p,0,num*size);
	return p;
};

__inline char * mir_strdup(const char * src)
{
	char * p;
	if (src==NULL) return NULL;
	p= mir_alloc( strlen(src)+1 );
	strcpy(p, src);
	return p;
}

static char *DBGetString(HANDLE hContact,const char *szModule,const char *szSetting)
{
	char *str=NULL;
	DBVARIANT dbv;
	DBGetContactSetting(hContact,szModule,szSetting,&dbv);
	if(dbv.type==DBVT_ASCIIZ)
		str=strdup(dbv.pszVal);
	DBFreeVariant(&dbv);
	return str;
}

static DWORD exceptFunction(LPEXCEPTION_POINTERS EP) 
{ 
    //printf("1 ");                     // printed first 
	char buf[4096];
	
	
	sprintf(buf,"\r\nExceptCode: %x\r\nExceptFlags: %x\r\nExceptAddress: %x\r\n",
		EP->ExceptionRecord->ExceptionCode,
		EP->ExceptionRecord->ExceptionFlags,
		EP->ExceptionRecord->ExceptionAddress
		);
	OutputDebugStr(buf);
	MessageBoxA(0,buf,"clist_mw Exception",0);

    
	return EXCEPTION_EXECUTE_HANDLER; 
} 

#endif