
#if defined(UNICODE)

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
#include <math.h>
#include "m_avatars.h"

#include "gdiplus.h"

using namespace Gdiplus;

static ColorMatrix ClrMatrix =         { 
            1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f
};

extern "C" int _DebugPopup(HANDLE hContact, const char *fmt, ...);

extern "C" void DrawWithGDIp(HDC hDC, DWORD x, DWORD y, DWORD width, DWORD height, DWORD srcWidth, DWORD srcHeight, struct avatarCacheEntry *ace, HBITMAP hbm)
{
    Image *im = NULL;
    Bitmap *bm = NULL;
    ImageAttributes attr;
    
    WCHAR szwFilename[MAX_PATH];

    Rect rect(x, y, width, height);
    Graphics *g = new Graphics(hDC);
    
    g->SetInterpolationMode(InterpolationModeHighQualityBicubic);
    if(ace == NULL)
        goto plain;

    if(ace->dwFlags & AVS_PREMULTIPLIED && ace->szFilename) {
        MultiByteToWideChar(CP_ACP, 0, ace->szFilename, -1, szwFilename, MAX_PATH);
        im = new Image(szwFilename);
        if(im != NULL) {
            g->DrawImage(im, rect, 0, 0, srcWidth, srcHeight, UnitPixel, &attr, NULL, NULL);
            delete im;
        }
    }
    else {
plain:
        bm = new Bitmap(hbm, 0);
        g->DrawImage(bm, rect, 0, 0, srcWidth, srcHeight, UnitPixel, &attr, NULL, NULL);
        delete bm;
    }
    delete g;
}

#endif
