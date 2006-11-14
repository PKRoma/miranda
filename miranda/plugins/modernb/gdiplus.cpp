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
#include <win2k.h>
#include "m_avatars.h"

#undef Translate
#include "gdiplus.h"

#include "modern_global_structure.h"

extern "C" BYTE saved_alpha;
extern "C" DWORD g_gdiplusToken;

extern "C" int mir_strlen(const char *a);

DWORD g_gdiplusToken;


extern "C" void InitGdiPlus(void)
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	g_CluiData.fGDIPlusFail = false;
	__try {
		if (g_gdiplusToken == 0)
			Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);
	}
	__except ( EXCEPTION_EXECUTE_HANDLER ) {
		g_CluiData.fGDIPlusFail = true;
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
		g_CluiData.fGDIPlusFail = true;
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

DWORD argb_from_cola(COLORREF col, BYTE alpha)
{
    return((BYTE) (alpha) << 24 | col);
}
extern "C" HBITMAP GDIPlus_LoadGlyphImage(char *szFileName)
{
  WCHAR *string;
  string=(WCHAR*)malloc(sizeof(WCHAR)*(mir_strlen(szFileName)+2));
  MultiByteToWideChar(CP_ACP, 0, szFileName, -1, string, (mir_strlen(szFileName)+2)*sizeof(WCHAR)); 
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
extern "C" bool GDIPlus_AlphaBlend(HDC hdcDest,int nXOriginDest,int nYOriginDest,int nWidthDest,int nHeightDest,
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
COLORREF __inline _revcolref(COLORREF colref)
{
    return RGB(GetBValue(colref), GetGValue(colref), GetRValue(colref));
}
