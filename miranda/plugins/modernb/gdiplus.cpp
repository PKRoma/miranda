/*
 * test for gdi+
 */

#include <malloc.h>

#ifdef _DEBUG
#	define _CRTDBG_MAP_ALLOC
#	include <stdlib.h>
//#	include <crtdbg.h>
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
#include "resource.h"
#include "forkthread.h"
#include <win2k.h>
//#include <newpluginapi.h>
//#include <m_clist.h>
//#include <m_clc.h>
//#include <m_clui.h>
//#include <m_plugins.h>
//#include <m_system.h>
//#include <m_database.h>
//#include <m_langpack.h>
//#include <m_button.h>
//#include <m_options.h>
//#include <m_protosvc.h>
//#include <m_utils.h>
//#include <m_skin.h>
//#include <m_contacts.h>
//#include <m_file.h>
//#include <m_addcontact.h>
//#include "clc.h"
//#include "clist.h"
////#include "alphablend.h"
////#include "extBackg.h"
#include "m_avatars.h"
//#include "m_popup.h"

#undef Translate
#include "gdiplus.h"

extern "C" struct CluiData g_CluiData;
extern "C" BYTE saved_alpha;
extern "C" DWORD g_gdiplusToken;
extern "C" BYTE gdiPlusFail;// = false;

extern "C" int MyStrLen(char *a);

DWORD g_gdiplusToken;
BYTE gdiPlusFail=false;

extern "C" void InitGdiPlus(void)
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  gdiPlusFail = false;
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



extern "C" int g_hottrack;
//extern "C" BOOL __fastcall GetItemByStatus(int status, StatusItems_t *retitem);

DWORD argb_from_cola(COLORREF col, BYTE alpha)
{
    return((BYTE) (alpha) << 24 | col);
}
extern "C" HBITMAP intLoadGlyphImageByGDIPlus(char *szFileName)
{
  WCHAR *string;
  string=(WCHAR*)malloc(sizeof(WCHAR)*(MyStrLen(szFileName)+2));
  MultiByteToWideChar(CP_ACP, 0, szFileName, -1, string, (MyStrLen(szFileName)+2)*sizeof(WCHAR)); 
  // Create a Bitmap object from a JPEG file.
  Bitmap bitmap(string,0);
  free(string);
  // Clone a portion of the bitmap.
  Bitmap* clone = bitmap.Clone(0, 0, bitmap.GetWidth(), bitmap.GetHeight(), PixelFormat32bppPARGB);
  HBITMAP hbmp=NULL;
  if (clone)
  {
    clone->GetHBITMAP(Color(0,0,0),&hbmp); 
    delete clone;
  }
  return hbmp;
}
extern "C" void TextOutWithGDIp(HDC hDestDC, int x, int y, LPCTSTR lpString, int nCount)
{
//   Graphics s(hDestDC);  
//   HBITMAP hs;
//   hs=(HBITMAP)GetCurrentObject(hDestDC,OBJ_BITMAP);
//   Bitmap sb(hs,NULL);
//   Bitmap *b=(sb.Clone(x,y,150,30,PixelFormat32bppARGB));   
//   Graphics g(b);//(100,100,PixelFormat32bppPARGB);
//   //g.DrawImage(sb);
//  // s.SetCompositingMode(CompositingModeSourceCopy);
//  // g.SetCompositingMode(CompositingModeSourceCopy);
//   g.DrawImage(&sb,0,0,x,y,100,30,UnitPixel);
//   //s.SetCompositingMode(CompositingModeSourceCopy);
//   //g.SetCompositingMode(CompositingModeSourceCopy);
//   // Create a string.
//   
//   WCHAR *string;
//   string=(WCHAR*)malloc(sizeof(WCHAR)*(nCount+2));
//   MultiByteToWideChar(CP_ACP, 0, lpString, -1, string, (nCount+2)*sizeof(WCHAR));
//   Font myFont(hDestDC);
//   
//   PointF origin((float)0, (float)0);
//   PointF origin2((float)x, (float)y);
//   g.SetTextRenderingHint(TextRenderingHintSystemDefault);
//   g.SetSmoothingMode(SmoothingModeAntiAlias);
//   COLORREF ref=GetTextColor(hDestDC);
//   SolidBrush blackBrush(Color(255, GetRValue(ref),GetGValue(ref),GetBValue(ref)));
//   g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
//   g.DrawString(string,nCount,&myFont,origin, &blackBrush);   
//   //g.SetCompositingMode(CompositingModeSourceCopy);
//   //s.SetCompositingMode(CompositingModeSourceCopy);
//   free(string);
//   //HDC temp=g.GetHDC();
//   //BitBlt(hDestDC,x,y,100,100,temp,0,0,SRCCOPY);
//   //g.ReleaseHDC(temp);	
//   s.DrawImage(b,origin2);
//
}

extern "C" void DrawAvatarImageWithGDIp(HDC hDestDC,int x, int y, DWORD width, DWORD height, HBITMAP hbmp, int x1, int y1, DWORD width1, DWORD height1,DWORD flag,BYTE alpha)
{
   BITMAP bmp;
   Bitmap *bm;
   BYTE * bmbits=NULL;
   GetObject(hbmp,sizeof(BITMAP),&bmp);
   Graphics g(hDestDC);   
   if (bmp.bmBitsPixel==32 && (flag&AVS_PREMULTIPLIED))
   {   
      bmbits=(BYTE*)bmp.bmBits;
      if (!bmbits)
      {
        bmbits=(BYTE*)malloc(bmp.bmHeight*bmp.bmWidthBytes);
        GetBitmapBits(hbmp,bmp.bmHeight*bmp.bmWidthBytes,bmbits);
      }
      bm= new Bitmap(bmp.bmWidth,bmp.bmHeight,bmp.bmWidthBytes,PixelFormat32bppPARGB,bmbits);
      bm->RotateFlip(RotateNoneFlipY);
      if (!bmp.bmBits) 
      {
      bm->RotateFlip(RotateNoneFlipY);
        free(bmbits);
      }
   }
   else 
     bm=new Bitmap(hbmp,NULL);
   
   ImageAttributes attr;
    ColorMatrix ClrMatrix = 
    { 
            1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, ((float)alpha)/255, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f
    };
    attr.SetColorMatrix(&ClrMatrix, ColorMatrixFlagsDefault,ColorAdjustTypeBitmap);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
	RectF rect((float)x,(float)y,(float)width,(float)height);
    g.DrawImage(bm, rect, (float)x1, (float)y1, (float)width1, (float)height1 , UnitPixel, &attr, NULL, NULL);
    delete bm;
}
extern "C" bool AlphaBlengGDIPlus(HDC hdcDest,int nXOriginDest,int nYOriginDest,int nWidthDest,int nHeightDest,
								  HDC hdcSrc,int nXOriginSrc,int nYOriginSrc,int nWidthSrc,int nHeightSrc, 
								  BLENDFUNCTION * bf)
{
	Graphics g(hdcDest);
	BITMAP bmp;
	HBITMAP hbmp=(HBITMAP)GetCurrentObject(hdcSrc,OBJ_BITMAP);
	GetObject(hbmp,sizeof(BITMAP),&bmp);
	Bitmap *bm=new Bitmap(hbmp,NULL);
	if (bmp.bmBitsPixel==32 && bf->AlphaFormat)
	{   
		bm= new Bitmap(bmp.bmWidth,bmp.bmHeight,bmp.bmWidthBytes,PixelFormat32bppPARGB,(BYTE*)bmp.bmBits);
		bm->RotateFlip(RotateNoneFlipY);
	}
	else 
		bm=new Bitmap(hbmp,NULL);
	ImageAttributes attr;
	ColorMatrix ClrMatrix = 
	{ 
		1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, ((float)bf->SourceConstantAlpha)/255, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f, 1.0f
	};
	attr.SetColorMatrix(&ClrMatrix, ColorMatrixFlagsDefault,ColorAdjustTypeBitmap);
  
 if (bf->BlendFlags&128 && nWidthDest<nWidthSrc && nHeightDest<nHeightSrc)
  {
		g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetPixelOffsetMode(PixelOffsetModeHalf);
    attr.SetGamma((REAL)0.8,ColorAdjustTypeBitmap);
  }
	else
	{
		g.SetInterpolationMode(InterpolationModeLowQuality);
		//g.SetPixelOffsetMode(PixelOffsetModeHalf);
	}
	
	RectF rect((float)nXOriginDest,(float)nYOriginDest,(float)nWidthDest,(float)nHeightDest);
	g.DrawImage(bm, rect, (float)nXOriginSrc, (float)nYOriginSrc, (float)nWidthSrc, (float)nHeightSrc , UnitPixel, &attr, NULL, NULL);
	delete bm;
	return TRUE;
}
//extern "C" void DrawWithGDIp(HDC hDC, DWORD x, DWORD y, DWORD width, DWORD height, UCHAR alpha, struct ClcContact *contact)
//{
//    Image *im = NULL;
//    
//    WCHAR szwFilename[MAX_PATH];
//
//    Rect rect(x, y, width, height);
//    Graphics g(hDC);
//    ImageAttributes attr;
//    
//    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
//
//    if(alpha < 255) {
//        ClrMatrix.m[3][3] = (REAL)(alpha) / 255.0f;
//        attr.SetColorMatrix(&ClrMatrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
//        //g.SetCompositingQuality(CompositingQualityGammaCorrected);
//    }
//    if(contact->ace->dwFlags & AVS_PREMULTIPLIED) {
//        if(contact->gdipObject == NULL) {
//            // create or find it in cache
//            int iAvail = -1;
//            for(int i = 0; i < g_maxImgCache; i++) {
//                if(imgCache[i].ace == contact->ace && imgCache[i].im != NULL) {
//                    contact->gdipObject = (LPVOID)imgCache[i].im;
//                    im = imgCache[i].im;
//                    //_DebugPopup(contact->hContact, "found in cache: %d", i);
//                    break;
//                }
//                if(imgCache[i].ace == NULL)
//                    iAvail = i;
//            }
//            if(i == g_maxImgCache) {
//                if(iAvail == -1) {
//                    g_maxImgCache += 10;
//                    imgCache = (ImgCache *)realloc(imgCache, sizeof(ImgCache) * g_maxImgCache);
//                    ZeroMemory(&imgCache[g_maxImgCache - 11], 10 * sizeof(ImgCache));
//                    iAvail = i;
//                }
//                MultiByteToWideChar(CP_ACP, 0, contact->ace->szFilename, -1, szwFilename, MAX_PATH);
//                imgCache[iAvail].ace = contact->ace;
//                imgCache[iAvail].im = im = new Image(szwFilename);
//                contact->gdipObject = (LPVOID)imgCache[iAvail].im;
//                //_DebugPopup(contact->hContact, "creating image in cache: %d", iAvail);
//                
//            }
//        }
//        else
//            im = (Image *)contact->gdipObject;
//        
//        g.DrawImage(im, rect, 0, 0, contact->ace->bmWidth, contact->ace->bmHeight, UnitPixel, &attr, NULL, NULL);
//    }
//    else {
//        Bitmap bm(contact->ace->hbmPic, 0);
//        g.DrawImage(&bm, rect, 0, 0, contact->ace->bmWidth, contact->ace->bmHeight, UnitPixel, &attr, NULL, NULL);
//    }
//}

COLORREF __inline _revcolref(COLORREF colref)
{
    return RGB(GetBValue(colref), GetGValue(colref), GetRValue(colref));
}

BYTE __inline _percent_to_byte(UINT32 percent)
{
    return(BYTE) ((FLOAT) (((FLOAT) percent) / 100) * 255);
}

DWORD _argb_from_cola(COLORREF col, UINT32 alpha)
{
    return((BYTE) _percent_to_byte(alpha) << 24 | col);
}

//extern "C" void GDIp_DrawAlpha(HWND hwnd, HDC hdcwnd, PRECT rc, DWORD basecolor, BYTE alpha, DWORD basecolor2, BOOL transparent, DWORD FLG_GRADIENT, DWORD FLG_CORNER, DWORD BORDERSTYLE)
//{
//    if (g_hottrack) {
//        StatusItems_t ht;
//        GetItemByStatus(ID_EXTBKHOTTRACK, &ht);
//        if (ht.IGNORED == 0) {
//            basecolor = ht.COLOR;
//            basecolor2 = ht.COLOR2;
//            alpha = ht.ALPHA;
//            FLG_GRADIENT = ht.GRADIENT;
//            transparent = ht.COLOR2_TRANSPARENT;
//            BORDERSTYLE = ht.BORDERSTYLE;
//        }
//    }
//
//    basecolor = _argb_from_cola(_revcolref(basecolor), alpha);
//    basecolor2 = _argb_from_cola(_revcolref(basecolor2), transparent ? 0 : alpha);
//    
//    Rect rect(rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top);
//    Color clr1(basecolor);
//    Color clr2(basecolor2);
//
//    Point left_line[2], top_line[2], right_line[2], bottom_line[2];
//
//    DWORD left = rc->left, right = rc->right, top = rc->top - 1, bottom = rc->bottom;
//    int half_height = (bottom - top) / 2;
//    int half_width = (right - left) / 2;
//
//    GraphicsPath gp;
//    DWORD radius = g_CluiData.cornerRadius;
//    radius = min(radius, (bottom - top) / 2);
//    int bevel = radius / 3;
//
//    Graphics g(hdcwnd);
//    //g.SetCompositingQuality(CompositingQualityGammaCorrected);
//    //g.SetCompositingMode(CompositingModeSourceOver);
//
//    
//    if(FLG_CORNER & CORNER_ACTIVE && (FLG_CORNER & CORNER_TL || FLG_CORNER & CORNER_BL))
//        left--;
//    
//    
//    if(FLG_CORNER & CORNER_ACTIVE) {            // construct the path for the clist item - rounded
//                                                // gdi+ doesn't know a RoundRect, so it has to happen manually using lines and curves
//        Point splinePoints[3];
//        
//        g.SetSmoothingMode(SmoothingModeHighQuality);
//        if(FLG_CORNER & CORNER_TL) {
//            left_line[0].X = left;
//            left_line[0].Y = top + radius;
//            top_line[0].X = left + radius;
//            top_line[0].Y = top;
//        }
//        else {
//            left_line[0].X = left;
//            left_line[0].Y = top;
//            top_line[0].X = left;
//            top_line[0].Y = top;
//        }
//        if(FLG_CORNER & CORNER_TR) {
//            top_line[1].X = right - radius;
//            top_line[1].Y = top;
//            right_line[0].X = right;
//            right_line[0].Y = top + radius;
//        }
//        else {
//            top_line[1].X = right;
//            top_line[1].Y = top;
//            right_line[0].X = right;
//            right_line[0].Y = top;
//        }
//        if(FLG_CORNER & CORNER_BL) {
//            left_line[1].X = left;
//            left_line[1].Y = bottom - radius;
//            bottom_line[0].X = left + radius;
//            bottom_line[0].Y = bottom;
//        }
//        else {
//            left_line[1].X = left;
//            left_line[1].Y = bottom;
//            bottom_line[0].X = left;
//            bottom_line[0].Y = bottom;
//        }
//        if(FLG_CORNER & CORNER_BR) {
//            right_line[1].X = right;
//            right_line[1].Y = bottom - radius;
//            bottom_line[1].X = right - radius;
//            bottom_line[1].Y = bottom;
//        }
//        else {
//            right_line[1].X = right;
//            right_line[1].Y = bottom;
//            bottom_line[1].X = right;
//            bottom_line[1].Y = bottom;
//        }
//        gp.AddLine(left_line[1], left_line[0]);
//        if(FLG_CORNER & CORNER_TL) {
//            splinePoints[0] = left_line[0];
//            splinePoints[1].X = left + bevel; splinePoints[1].Y = top + bevel;
//            gp.AddCurve(splinePoints, 2, 1.0f);
//            //gp.AddArc(left, top, radius * 2, radius * 2, 180.0f, 90.0f);
//        }
//        gp.AddLine(top_line[0], top_line[1]);
//        if(FLG_CORNER & CORNER_TR) {
//            splinePoints[0] = top_line[1];
//            splinePoints[1].X = right - bevel; splinePoints[1].Y = top + bevel;
//            gp.AddCurve(splinePoints, 2, 1.0f);
//            //gp.AddArc(right - 2 * radius, top, radius * 2, radius * 2, 270.0f, 90.0f);
//        }
//        gp.AddLine(right_line[0], right_line[1]);
//        if(FLG_CORNER & CORNER_BR) {
//            splinePoints[0] = right_line[1];
//            splinePoints[1].X = right - bevel; splinePoints[1].Y = bottom - bevel;
//            gp.AddCurve(splinePoints, 2, 1.0f);
//            //gp.AddArc(right - 2 * radius, bottom - 2 * radius, radius * 2, radius * 2, 0.0f, 90.0f);
//        }
//        gp.AddLine(bottom_line[1], bottom_line[0]);
//        if(FLG_CORNER & CORNER_BL) {
//            splinePoints[0] = bottom_line[0];
//            splinePoints[1].X = left + bevel; splinePoints[1].Y = bottom - bevel;
//            gp.AddCurve(splinePoints, 2, 1.0f);
//            //gp.AddArc(left, bottom - 2 * radius, radius * 2, radius * 2, 90.0f, 90.0f);
//        }
//
//        g.SetClip(rect);
//    }
//    else 
//        gp.AddRectangle(rect);
//    
//    if(FLG_GRADIENT & GRADIENT_ACTIVE) {
//        LinearGradientBrush *grbrush = 0;
//        
//        if(FLG_GRADIENT & GRADIENT_LR)
//            grbrush = new LinearGradientBrush(Point(left, half_height), Point(right, half_height), clr1, clr2);
//        else if(FLG_GRADIENT & GRADIENT_RL)
//            grbrush = new LinearGradientBrush(Point(left, half_height), Point(right, half_height), clr2, clr1);
//        else if(FLG_GRADIENT & GRADIENT_TB)
//            grbrush = new LinearGradientBrush(Point(half_width, top), Point(half_width, bottom), clr1, clr2);
//        else if(FLG_GRADIENT & GRADIENT_BT)
//            grbrush = new LinearGradientBrush(Point(half_width, top), Point(half_width, bottom), clr2, clr1);
//        //gp.Outline(NULL, 200.0f);
//        //Pen pen(Color(0xff000000));
//        //g.DrawPath(&pen, &gp);
//        if(grbrush) {
//            g.FillPath(grbrush, &gp);
//            delete grbrush;
//        }
//    }
//    else {
//        SolidBrush brush(clr1);
//        g.FillPath(&brush, &gp);
//    }
//    if(BORDERSTYLE >= 0)
//        ::DrawEdge(hdcwnd, rc, BORDERSTYLE, BF_RECT | BF_SOFT);
//    saved_alpha = (UCHAR) (basecolor >> 24);
//}
//
//