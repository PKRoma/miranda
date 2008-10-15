#define HAVE_STRING_H 1
#define STDC_HEADERS 1
#define HAVE_STRCHR 1
#define HAVE_MEMCPY 1

#define PACKAGE "libyahoo2"
#define VERSION "0.7.5"

/* Add some WIN32 size savers */
#define _USE_32BIT_TIME_T

#include <windows.h>
#include <stdio.h>

#define strlen lstrlen
#define strcat lstrcat
#define strcmp lstrcmp
#define strcpy lstrcpy

#ifdef _MSC_VER

#define strncasecmp _strnicmp

#define wsnprintf _wsnprintf
#define snprintf _snprintf
#define vsnprintf _vsnprintf

#endif

#define USE_STRUCT_CALLBACKS

#define write(a,b,c) send(a,b,c,0)
#define read(a,b,c)  recv(a,b,c,0)

#include <newpluginapi.h>
#include <m_netlib.h>
#define close(a)	 Netlib_CloseHandle((HANDLE)a)
