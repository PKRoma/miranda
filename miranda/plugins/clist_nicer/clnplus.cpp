/*
 * test for gdi+
 */

#if defined(UNICODE) && !defined(_UNICODE)
#	define _UNICODE
#endif

#include <malloc.h>
#include <string>
#include <tchar.h>

#ifdef _DEBUG
#	define _CRTDBG_MAP_ALLOC
#	include <stdlib.h>
#	include <crtdbg.h>
#endif

#define _CLN_GDIP

#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <time.h>
#include <stddef.h>
#include <process.h>
#include <shobjidl.h>
#include <io.h>
#include <string.h>
#include <direct.h>
#include <math.h>
#include "resource.h"
#include "forkthread.h"
#include <win2k.h>
#include <newpluginapi.h>
#include <m_clist.h>
#include <m_clistint.h>
#include <m_clc.h>
#include <m_clui.h>
#include <m_plugins.h>
#include <m_system.h>
#include <m_database.h>
#include <m_langpack.h>
#include <m_button.h>
#include <m_options.h>
#include <m_protosvc.h>
#include <m_utils.h>
#include <m_skin.h>
#include <m_contacts.h>
#include <m_file.h>
#include <m_addcontact.h>
#include "m_cln_skinedit.h"
#include "extbackg.h"
#include "clc.h"
#include "clist.h"
#include "alphablend.h"
#include "m_avatars.h"
#include "m_popup.h"

#undef Translate

extern "C" void RemoveFromTaskBar(HWND hWnd)
{
    ITaskbarList *pTaskbarList = NULL;

    if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, 0, CLSCTX_INPROC_SERVER, IID_ITaskbarList,
                                   (void **)(&pTaskbarList))) &&	pTaskbarList != NULL) {
        if (SUCCEEDED(pTaskbarList->HrInit())) {
            pTaskbarList->DeleteTab(hWnd);
        }
        pTaskbarList->Release();
    }
}

