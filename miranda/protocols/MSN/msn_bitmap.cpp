/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright(C) 2002-2004 George Hazan (modification) and Richard Hughes (original)

Miranda IM: the free icq client for MS Windows
Copyright (C) 2000-2002 Richard Hughes, Roland Rabien & Tristan Van de Vreede

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

#include "msn_global.h"

int __stdcall MSN_PngToDibBits( const char* pszFileName, BITMAPINFOHEADER*& ppDib, BYTE*& ppDibBits )
{
	if ( !MSN_LoadPngModule())
		return 1;

	HANDLE hFile = NULL, hMap = NULL;
	BYTE* ppMap = NULL;
	long  cbFileSize = 0;

	if (( hFile = CreateFile( pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL )) != INVALID_HANDLE_VALUE )
		if (( hMap = CreateFileMapping( hFile, NULL, PAGE_READONLY, 0, 0, NULL )) != NULL )
			if (( ppMap = ( BYTE* )::MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, 0 )) != NULL )
				cbFileSize = GetFileSize( hFile, NULL );

	if ( cbFileSize != 0 )
		if ( png2dibConvertor(( char* )ppMap, cbFileSize, &ppDib ))
			ppDibBits = ( BYTE* )( ppDib+1 );

	if ( ppMap != NULL )	UnmapViewOfFile( ppMap );
	if ( hMap  != NULL )	CloseHandle( hMap );
	if ( hFile != NULL ) CloseHandle( hFile );

	return cbFileSize == 0;
}

HBITMAP __stdcall MSN_LoadPictureToBitmap( const char* pszFileName )
{
	if ( memicmp( pszFileName + strlen(pszFileName)-4, ".PNG", 4 ) != 0 )
		return ( HBITMAP )MSN_CallService( MS_UTILS_LOADBITMAP, 0, ( LPARAM )pszFileName );

	BITMAPINFOHEADER* pDib;
	BYTE* pDibBits;
	if ( MSN_PngToDibBits( pszFileName, pDib, pDibBits ))
		return NULL;

	HDC sDC = GetDC( NULL );
	HBITMAP hBitmap = CreateDIBitmap( sDC, pDib, CBM_INIT, pDibBits, ( BITMAPINFO* )pDib, DIB_PAL_COLORS );
	SelectObject( sDC, hBitmap );
	DeleteDC( sDC );
	return hBitmap;
}
