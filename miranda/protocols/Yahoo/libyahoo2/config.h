#define HAVE_STRING_H 1
#define STDC_HEADERS 1
#define HAVE_STRCHR 1
#define HAVE_MEMCPY 1

#define PACKAGE "libyahoo2"
#define VERSION "0.7.5"

#include <m_stdhdr.h>

#include <windows.h>
#include <stdio.h>

#define strlen lstrlenA
#define strcat lstrcatA
#define strcmp lstrcmpA
#define strcpy lstrcpyA

#ifdef _MSC_VER

#define strncasecmp strnicmp

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
