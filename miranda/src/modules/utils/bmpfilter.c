/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "../../core/commonheaders.h"
#include <ocidl.h>
#include <olectl.h>

static int BmpFilterLoadBitmap(WPARAM wParam,LPARAM lParam)
{
	IPicture *pic;
	HBITMAP hBmp,hBmpCopy;
	HBITMAP hOldBitmap, hOldBitmap2;
	BITMAP bmpInfo;
	WCHAR pszwFilename[MAX_PATH];
	HDC hdc,hdcMem1,hdcMem2;
	short picType;
	const char *szFile=(const char *)lParam;
	char szFilename[MAX_PATH];
	int filenameLen;
    
    if (!CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)szFile, (LPARAM)szFilename))
        mir_snprintf(szFilename, sizeof(szFilename), "%s", szFile);
	filenameLen=lstrlenA(szFilename);
	if(filenameLen>4) {
		if(!lstrcmpiA(szFilename+filenameLen-4,".bmp") || !lstrcmpiA(szFilename+filenameLen-4,".rle")) {
			//LoadImage can do this much faster
			return (int)LoadImageA(GetModuleHandle(NULL),szFilename,IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
		}
	}
	OleInitialize(NULL);
	MultiByteToWideChar(CP_ACP,0,szFilename,-1,pszwFilename,MAX_PATH);
	if(S_OK!=OleLoadPicturePath(pszwFilename,NULL,0,0,&IID_IPicture,(PVOID*)&pic)) {
		OleUninitialize();
		return (int)(HBITMAP)NULL;
	}
	pic->lpVtbl->get_Type(pic,&picType);
	if(picType!=PICTYPE_BITMAP) {
		pic->lpVtbl->Release(pic);
		OleUninitialize();
		return (int)(HBITMAP)NULL;
	}
	pic->lpVtbl->get_Handle(pic,(OLE_HANDLE*)&hBmp);
	GetObject(hBmp,sizeof(bmpInfo),&bmpInfo);

	//need to copy bitmap so we can free the IPicture
	hdc=GetDC(NULL);
	hdcMem1=CreateCompatibleDC(hdc);
	hdcMem2=CreateCompatibleDC(hdc);
	hOldBitmap=SelectObject(hdcMem1,hBmp);
	hBmpCopy=CreateCompatibleBitmap(hdcMem1,bmpInfo.bmWidth,bmpInfo.bmHeight);
	hOldBitmap2=SelectObject(hdcMem2,hBmpCopy);
	BitBlt(hdcMem2,0,0,bmpInfo.bmWidth,bmpInfo.bmHeight,hdcMem1,0,0,SRCCOPY);
	SelectObject(hdcMem1,hOldBitmap);
	SelectObject(hdcMem2,hOldBitmap2);
	DeleteDC(hdcMem2);
	DeleteDC(hdcMem1);
	ReleaseDC(NULL,hdc);

	DeleteObject(hBmp);
	pic->lpVtbl->Release(pic);
	OleUninitialize();
	return (int)hBmpCopy;
}

static int BmpFilterGetStrings(WPARAM wParam,LPARAM lParam)
{
	int bytesLeft=wParam;
	char *filter=(char*)lParam,*pfilter;

	lstrcpynA(filter,Translate("All Bitmaps"),bytesLeft); bytesLeft-=lstrlenA(filter);
	strncat(filter," (*.bmp;*.jpg;*.gif)",bytesLeft);
	pfilter=filter+lstrlenA(filter)+1; bytesLeft=wParam-(pfilter-filter);
	lstrcpynA(pfilter,"*.BMP;*.RLE;*.JPG;*.JPEG;*.GIF",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);

	lstrcpynA(pfilter,Translate("Windows Bitmaps"),bytesLeft); bytesLeft-=lstrlenA(pfilter);
	strncat(pfilter," (*.bmp;*.rle)",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);
	lstrcpynA(pfilter,"*.BMP;*.RLE",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);

	lstrcpynA(pfilter,Translate("JPEG Bitmaps"),bytesLeft); bytesLeft-=lstrlenA(pfilter);
	strncat(pfilter," (*.jpg;*.jpeg)",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);
	lstrcpynA(pfilter,"*.JPG;*.JPEG",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);

	lstrcpynA(pfilter,Translate("GIF Bitmaps"),bytesLeft); bytesLeft-=lstrlenA(pfilter);
	strncat(pfilter," (*.gif)",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);
	lstrcpynA(pfilter,"*.GIF",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);

	lstrcpynA(pfilter,Translate("All Files"),bytesLeft); bytesLeft-=lstrlenA(pfilter);
	strncat(pfilter," (*)",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);
	lstrcpynA(pfilter,"*",bytesLeft);
	pfilter+=lstrlenA(pfilter)+1; bytesLeft=wParam-(pfilter-filter);

	if(bytesLeft) *pfilter='\0';
	return 0;
}

int InitBitmapFilter(void)
{
	CreateServiceFunction(MS_UTILS_LOADBITMAP,BmpFilterLoadBitmap);
	CreateServiceFunction(MS_UTILS_GETBITMAPFILTERSTRINGS,BmpFilterGetStrings);
	return 0;
}
