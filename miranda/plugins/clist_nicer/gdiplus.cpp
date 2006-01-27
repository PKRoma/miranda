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
#include "extbackg.h"
#include "clc.h"
#include "clist.h"
#include "alphablend.h"
#include "m_avatars.h"
#include "m_popup.h"

#undef Translate
#include "gdiplus.h"

extern "C" struct CluiData g_CluiData;
extern "C" BYTE saved_alpha;
extern "C" DWORD g_gdiplusToken;
extern "C" BOOL g_inCLCpaint;
extern "C" StatusItems_t *StatusItems;

BYTE gdiPlusFail = false;

typedef enum {
    True,
    False,
    MaybeButMaybeNot
} Boolean;

extern "C" BYTE MY_DBGetContactSettingByte(HANDLE hContact, char *szModule, char *szSetting, BYTE defaultval);

extern "C" void InitGdiPlus(void)
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	__try {
		if (g_gdiplusToken == 0)
			Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ) {
		gdiPlusFail = true;
	}
}

extern "C" void ShutdownGdiPlus(void)
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	__try {
		if (g_gdiplusToken)
			Gdiplus::GdiplusShutdown(g_gdiplusToken);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ) {
		gdiPlusFail = true;
    }
    g_gdiplusToken = 0;
}

using namespace Gdiplus;

static ColorMatrix ClrMatrix =         { 
            1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f
};

typedef struct _tagImgCache {
    Image *im;
    struct avatarCacheEntry *ace;
} ImgCache;

static ImgCache *imgCache = NULL;
static int g_maxImgCache = 0;

/*
extern "C" void InitImgCache()
{
    imgCache = (ImgCache *)malloc(100 * sizeof(ImgCache));
    ZeroMemory(imgCache, sizeof(ImgCache) * 100);
    g_maxImgCache = 100;
}
*/

/*
extern "C" void FreeImgCache()
{
    for(int i = 0; i < g_maxImgCache; i++) {
        if(imgCache[i].ace != NULL && imgCache[i].im != NULL) {
            delete imgCache[i].im;
            imgCache[i].ace = NULL;
            imgCache[i].im = NULL;
        }
    }
    free(imgCache);
}
/*
/*
extern "C" void RemoveFromImgCache(HANDLE hContact, struct avatarCacheEntry *ace)
{
    if(ace == NULL)
        return;
    
    for(int i = 0; i < g_maxImgCache; i++) {
        if(imgCache[i].ace == ace && imgCache[i].im != NULL) {
            //_DebugPopup(hContact, "deleting from cache... %d", i);
            delete imgCache[i].im;
            imgCache[i].ace = NULL;
            imgCache[i].im = NULL;
            break;
        }
    }
}
*/
extern "C" int g_hottrack;

Graphics *g = NULL;

extern "C" void DrawWithGDIp(HDC hDC, DWORD x, DWORD y, DWORD width, DWORD height, UCHAR alpha, struct ClcContact *contact)
{
    Image *im = NULL;
    
    //WCHAR szwFilename[MAX_PATH];

    Rect rect(x, y, width, height);
    //Graphics g(hDC);
    ImageAttributes attr;
    
    g->SetInterpolationMode(InterpolationModeHighQualityBicubic);
    //g->SetInterpolationMode(InterpolationModeNearestNeighbor);
    
    if(alpha < 255) {
        ClrMatrix.m[3][3] = (REAL)(alpha) / 255.0f;
        attr.SetColorMatrix(&ClrMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
        //g.SetCompositingQuality(CompositingQualityGammaCorrected);
    }
    /*
    if(contact->ace->dwFlags & AVS_PREMULTIPLIED) {
    //if(0) {
        if(contact->gdipObject == NULL) {
            // create or find it in cache
            int iAvail = -1;
            for(int i = 0; i < g_maxImgCache; i++) {
                if(imgCache[i].ace == contact->ace && imgCache[i].im != NULL) {
                    contact->gdipObject = (LPVOID)imgCache[i].im;
                    im = imgCache[i].im;
                    //_DebugPopup(contact->hContact, "found in cache: %d", i);
                    break;
                }
                if(imgCache[i].ace == NULL)
                    iAvail = i;
            }
            if(i == g_maxImgCache) {
                if(iAvail == -1) {
                    g_maxImgCache += 10;
                    imgCache = (ImgCache *)realloc(imgCache, sizeof(ImgCache) * g_maxImgCache);
                    ZeroMemory(&imgCache[g_maxImgCache - 11], 10 * sizeof(ImgCache));
                    iAvail = i;
                }
                MultiByteToWideChar(CP_ACP, 0, contact->ace->szFilename, -1, szwFilename, MAX_PATH);
                imgCache[iAvail].ace = contact->ace;
                imgCache[iAvail].im = im = new Image(szwFilename);
                contact->gdipObject = (LPVOID)imgCache[iAvail].im;
                //_DebugPopup(contact->hContact, "creating image in cache: %d", iAvail);
                
            }
        }
        else
            im = (Image *)contact->gdipObject;
        
        g.DrawImage(im, rect, 0, 0, contact->ace->bmWidth, contact->ace->bmHeight, UnitPixel, &attr, NULL, NULL);
    }
    else {*/
	 if ( contact->ace != NULL ) {
        Bitmap bm(contact->ace->hbmPic, 0);
        g->DrawImage(&bm, rect, 0, 0, contact->ace->bmWidth, contact->ace->bmHeight, UnitPixel, &attr, NULL, NULL);
	 }
}

COLORREF __forceinline _revcolref(COLORREF colref)
{
    return RGB(GetBValue(colref), GetGValue(colref), GetRValue(colref));
}

BYTE __forceinline _percent_to_byte(UINT32 percent)
{
    return(BYTE) ((FLOAT) (((FLOAT) percent) / 100) * 255);
}

DWORD _argb_from_cola(COLORREF col, UINT32 alpha)
{
    return((BYTE) _percent_to_byte(alpha) << 24 | col);
}

extern "C" void GDIp_DrawAlpha(HDC hdcwnd, PRECT rc, DWORD basecolor, BYTE alpha, DWORD basecolor2, BOOL transparent, DWORD FLG_GRADIENT, DWORD FLG_CORNER, DWORD BORDERSTYLE, ImageItem *item)
{
    if (g_hottrack && g_inCLCpaint) {
        StatusItems_t *ht = &StatusItems[ID_EXTBKHOTTRACK - ID_STATUS_OFFLINE];
        if (ht->IGNORED == 0) {
            basecolor = ht->COLOR;
            basecolor2 = ht->COLOR2;
            alpha = ht->ALPHA;
            FLG_GRADIENT = ht->GRADIENT;
            transparent = ht->COLOR2_TRANSPARENT;
            BORDERSTYLE = ht->BORDERSTYLE;
        }
    }

    Graphics my_g(hdcwnd);
    
    basecolor = _argb_from_cola(_revcolref(basecolor), alpha);
    basecolor2 = _argb_from_cola(_revcolref(basecolor2), transparent ? 0 : alpha);
    
    Rect rect(rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top);
    Color clr1(basecolor);
    Color clr2(basecolor2);

    Point left_line[2], top_line[2], right_line[2], bottom_line[2];

    DWORD left = rc->left, right = rc->right, top = rc->top - 1, bottom = rc->bottom;
    int half_height = (bottom - top) / 2;
    int half_width = (right - left) / 2;

    GraphicsPath gp;
    DWORD radius = g_CluiData.cornerRadius;
    radius = min(radius, (bottom - top) / 2);
    int bevel = radius / 3;

    //Graphics g(hdcwnd);
    //g.SetCompositingQuality(CompositingQualityGammaCorrected);
    //g.SetCompositingMode(CompositingModeSourceOver);

    
    if(FLG_CORNER & CORNER_ACTIVE && (FLG_CORNER & CORNER_TL || FLG_CORNER & CORNER_BL))
        left--;
    
    
    if(FLG_CORNER & CORNER_ACTIVE) {            // construct the path for the clist item - rounded
                                                // gdi+ doesn't know a RoundRect, so it has to happen manually using lines and curves
        Point splinePoints[3];
        
        my_g.SetSmoothingMode(SmoothingModeHighQuality);
        if(FLG_CORNER & CORNER_TL) {
            left_line[0].X = left;
            left_line[0].Y = top + radius;
            top_line[0].X = left + radius;
            top_line[0].Y = top;
        }
        else {
            left_line[0].X = left;
            left_line[0].Y = top;
            top_line[0].X = left;
            top_line[0].Y = top;
        }
        if(FLG_CORNER & CORNER_TR) {
            top_line[1].X = right - radius;
            top_line[1].Y = top;
            right_line[0].X = right;
            right_line[0].Y = top + radius;
        }
        else {
            top_line[1].X = right;
            top_line[1].Y = top;
            right_line[0].X = right;
            right_line[0].Y = top;
        }
        if(FLG_CORNER & CORNER_BL) {
            left_line[1].X = left;
            left_line[1].Y = bottom - radius;
            bottom_line[0].X = left + radius;
            bottom_line[0].Y = bottom;
        }
        else {
            left_line[1].X = left;
            left_line[1].Y = bottom;
            bottom_line[0].X = left;
            bottom_line[0].Y = bottom;
        }
        if(FLG_CORNER & CORNER_BR) {
            right_line[1].X = right;
            right_line[1].Y = bottom - radius;
            bottom_line[1].X = right - radius;
            bottom_line[1].Y = bottom;
        }
        else {
            right_line[1].X = right;
            right_line[1].Y = bottom;
            bottom_line[1].X = right;
            bottom_line[1].Y = bottom;
        }
        gp.AddLine(left_line[1], left_line[0]);
        if(FLG_CORNER & CORNER_TL) {
            splinePoints[0] = left_line[0];
            splinePoints[1].X = left + bevel; splinePoints[1].Y = top + bevel;
            gp.AddCurve(splinePoints, 2, 1.0f);
            //gp.AddArc(left, top, radius * 2, radius * 2, 180.0f, 90.0f);
        }
        gp.AddLine(top_line[0], top_line[1]);
        if(FLG_CORNER & CORNER_TR) {
            splinePoints[0] = top_line[1];
            splinePoints[1].X = right - bevel; splinePoints[1].Y = top + bevel;
            gp.AddCurve(splinePoints, 2, 1.0f);
            //gp.AddArc(right - 2 * radius, top, radius * 2, radius * 2, 270.0f, 90.0f);
        }
        gp.AddLine(right_line[0], right_line[1]);
        if(FLG_CORNER & CORNER_BR) {
            splinePoints[0] = right_line[1];
            splinePoints[1].X = right - bevel; splinePoints[1].Y = bottom - bevel;
            gp.AddCurve(splinePoints, 2, 1.0f);
            //gp.AddArc(right - 2 * radius, bottom - 2 * radius, radius * 2, radius * 2, 0.0f, 90.0f);
        }
        gp.AddLine(bottom_line[1], bottom_line[0]);
        if(FLG_CORNER & CORNER_BL) {
            splinePoints[0] = bottom_line[0];
            splinePoints[1].X = left + bevel; splinePoints[1].Y = bottom - bevel;
            gp.AddCurve(splinePoints, 2, 1.0f);
            //gp.AddArc(left, bottom - 2 * radius, radius * 2, radius * 2, 90.0f, 90.0f);
        }

        my_g.SetClip(rect);
    }
    else 
        gp.AddRectangle(rect);
    
    if(FLG_GRADIENT & GRADIENT_ACTIVE) {
        LinearGradientBrush *grbrush = 0;
        
        if(FLG_GRADIENT & GRADIENT_LR)
            grbrush = new LinearGradientBrush(Point(left, half_height), Point(right, half_height), clr1, clr2);
        else if(FLG_GRADIENT & GRADIENT_RL)
            grbrush = new LinearGradientBrush(Point(left, half_height), Point(right, half_height), clr2, clr1);
        else if(FLG_GRADIENT & GRADIENT_TB)
            grbrush = new LinearGradientBrush(Point(half_width, top), Point(half_width, bottom), clr1, clr2);
        else if(FLG_GRADIENT & GRADIENT_BT)
            grbrush = new LinearGradientBrush(Point(half_width, top), Point(half_width, bottom), clr2, clr1);
        //gp.Outline(NULL, 200.0f);
        //Pen pen(Color(0xff000000));
        //g.DrawPath(&pen, &gp);
        if(grbrush) {
            my_g.FillPath(grbrush, &gp);
            delete grbrush;
        }
    }
    else {
        SolidBrush brush(clr1);
        my_g.FillPath(&brush, &gp);
    }
    if(BORDERSTYLE >= 0) {
        HPEN hPenOld = 0;
        POINT pt;

        switch(BORDERSTYLE) {
            case BDR_RAISEDOUTER:                 // raised
                MoveToEx(hdcwnd, rc->left, rc->bottom - 1, &pt);
                hPenOld = (HPEN)SelectObject(hdcwnd, g_CluiData.hPen3DBright);
                LineTo(hdcwnd, rc->left, rc->top);
                LineTo(hdcwnd, rc->right, rc->top);
                SelectObject(hdcwnd, g_CluiData.hPen3DDark);
                MoveToEx(hdcwnd, rc->right - 1, rc->top + 1, &pt);
                LineTo(hdcwnd, rc->right - 1, rc->bottom - 1);
                LineTo(hdcwnd, rc->left - 1, rc->bottom - 1);
                break;
            case BDR_SUNKENINNER:
                MoveToEx(hdcwnd, rc->left, rc->bottom - 1, &pt);
                hPenOld = (HPEN)SelectObject(hdcwnd, g_CluiData.hPen3DDark);
                LineTo(hdcwnd, rc->left, rc->top);
                LineTo(hdcwnd, rc->right, rc->top);
                MoveToEx(hdcwnd, rc->right - 1, rc->top + 1, &pt);
                SelectObject(hdcwnd, g_CluiData.hPen3DBright);
                LineTo(hdcwnd, rc->right - 1, rc->bottom - 1);
                LineTo(hdcwnd, rc->left, rc->bottom - 1);
                break;
            default:
                DrawEdge(hdcwnd, rc, BORDERSTYLE, BF_RECT | BF_SOFT);
                break;
        }
        if(hPenOld)
            SelectObject(hdcwnd, hPenOld);
    }
    saved_alpha = (UCHAR) (basecolor >> 24);
}


extern "C" void CreateG(HDC hdc)
{
    if(g == NULL)
        g = new Graphics(hdc);

    g->SetTextRenderingHint(TextRenderingHintAntiAlias);
    //g->SetCompositingMode(CompositingModeSourceOver);
    if(MY_DBGetContactSettingByte(NULL, "CLC", "gdiplusgamma", 0))
        g->SetCompositingQuality(CompositingQualityHighQuality);
}

extern "C" void DeleteG()
{
    if(g) {
        delete g;
        g = NULL;
    }
}

extern "C" int MeasureTextHQ(HDC hdc, HFONT hFont, RECT *rc, WCHAR *szwText)
{
    Font font(hdc, hFont);
    RectF rectF((REAL)rc->left, (REAL)rc->top, (REAL)(rc->right - rc->left), (REAL)(rc->bottom - rc->top));
    RectF rectTarget;
    g->MeasureString(szwText, -1, &font, rectF, &rectTarget);
    rc->left = (int)rectTarget.X;
    rc->top = (int)rectTarget.Y;
    rc->right = rc->left + (int)rectTarget.Width;
    rc->bottom = rc->top + (int)rectTarget.Height;
    return 0;
}


extern "C" int DrawTextHQ(HDC hdc, HFONT hFont, RECT *rc, WCHAR *szwText, COLORREF colorref, int fontHeight, int g_RTL)
{
    StringFormat sf(StringFormatFlagsNoFontFallback | (g_RTL ? StringFormatFlagsDirectionRightToLeft : 0), LANG_NEUTRAL);
    sf.SetTrimming(StringTrimmingEllipsisCharacter);
    Font font(hdc, hFont);
    Color clr(GetRValue(colorref), GetGValue(colorref), GetBValue(colorref));
    SolidBrush brush(clr);
    RectF rectF((REAL)(rc->left - 2), (REAL)rc->top, (REAL)((rc->right - rc->left) + 2), (REAL)(rc->bottom - rc->top));
    if(fontHeight)
        rectF.Height = (REAL)fontHeight;
    //g->SetTextContrast(2000);
    g->DrawString(szwText, -1, &font, rectF, &sf, &brush);
    return 0;
}

extern "C" const char *MakeVariablesString(const char *src, const char *UIN)
{
    static std::string final;
    final.assign(src);

    unsigned curmark = 0;
    
    while((curmark = final.find("%%UIN%%", curmark)) != final.npos) {
        final.erase(curmark, 7);
        final.insert(curmark, UIN);
    }
    return final.c_str();
}

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

