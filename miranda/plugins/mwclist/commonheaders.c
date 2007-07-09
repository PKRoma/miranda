#include "commonheaders.h"

#define SAFE_PTR(a) a?(IsBadReadPtr(a,1)?a=NULL:a):a

void *mir_calloc( size_t num, size_t size )
{
 	void *p=mir_alloc(num*size);
	if (p==NULL) return NULL;
	memset(p,0,num*size);
	return p;
}

int __cdecl MyStrCmp (const char *a, const char *b)
{
	SAFE_PTR(a);
	SAFE_PTR(b);
	if (!(a&&b)) return a!=b;
	return (strcmp(a,b));
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
