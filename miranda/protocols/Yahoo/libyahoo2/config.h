#define HAVE_STRING_H 1
#define STDC_HEADERS 1
#define HAVE_STRCHR 1
#define HAVE_MEMCPY 1

#define PACKAGE "libyahoo2"
#define VERSION "0.7.5"

/* Add some WIN32 size savers */
#include <windows.h>

#define strlen lstrlen
#define strcat lstrcat
#define strcmp lstrcmp
#define strcpy lstrcpy

#define USE_STRUCT_CALLBACKS
