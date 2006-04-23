#include "commonheaders.h"

#define SAFE_PTR(a) a?(IsBadReadPtr(a,1)?a=NULL:a):a

int mir_free_proxy(void *ptr)
{
	if (ptr==NULL) //||IsBadCodePtr((FARPROC)ptr))
		return 0;
	memoryManagerInterface.mmi_free(ptr);
	return 0;
}

void *mir_calloc( size_t num, size_t size )
{
 	void *p=mir_alloc(num*size);
	if (p==NULL) return NULL;
	memset(p,0,num*size);
	return p;
}

char * mir_strdup(const char * src)
{
	char * p;
	if (src==NULL) return NULL;
	p= mir_alloc( strlen(src)+1 );
	strcpy(p, src);
	return p;
}

extern __inline wchar_t * mir_strdupW(const wchar_t * src)
{
	wchar_t * p;
	if (src==NULL) return NULL;
	p=(wchar_t *) mir_alloc((lstrlenW(src)+1)*sizeof(wchar_t));
	if (!p) return 0;
	lstrcpyW(p, src);
	return p;
}

int __cdecl MyStrCmp (const char *a, const char *b)
{
	SAFE_PTR(a);
	SAFE_PTR(b);
	if (!(a&&b)) return a!=b;
	return (strcmp(a,b));
}

char *DBGetStringA(HANDLE hContact,const char *szModule,const char *szSetting)
{
	char *str=NULL;
	DBVARIANT dbv={0};
	DBGetContactSetting(hContact,szModule,szSetting,&dbv);
	if(dbv.type==DBVT_ASCIIZ)
		str=mir_strdup(dbv.pszVal);

	DBFreeVariant(&dbv);
	return str;
}

DWORD exceptFunction(LPEXCEPTION_POINTERS EP)
{
    //printf("1 ");                     // printed first
	char buf[4096];


	sprintf(buf,"\r\nExceptCode: %x\r\nExceptFlags: %x\r\nExceptAddress: %x\r\n",
		EP->ExceptionRecord->ExceptionCode,
		EP->ExceptionRecord->ExceptionFlags,
		EP->ExceptionRecord->ExceptionAddress
		);
	OutputDebugStringA(buf);
	MessageBoxA(0,buf,"clist_mw Exception",0);


	return EXCEPTION_EXECUTE_HANDLER;
}
