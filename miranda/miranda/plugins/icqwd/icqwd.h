#include <windows.h>
#include "../../SDK/headers_c/newpluginapi.h"
#include "../../SDK/headers_c/m_protosvc.h"
#include "../../SDK/headers_c/m_addcontact.h"

#define WATCHERCLASS "__WebWatcherClass__"

typedef struct {
	int cbSize;
	char uin[64];
} ICQFILEINFO;


