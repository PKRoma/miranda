#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Force 512 byte section alignment in the PE file
#pragma comment(linker, "/OPT:NOWIN98")

BOOL WINAPI DllMainTiny(HANDLE hDllHandle,DWORD dwReason,LPVOID lpreserved)
{
	return TRUE;
}
