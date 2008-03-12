/*
 * test for gdi+
 */

#include "m_stdhdr.h"

#define _CLN_GDIP

#define _WIN32_WINNT 0x0501
#include <shobjidl.h>

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
