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

#include "sha1.h"

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_BitmapToAvatarDibBits - rescales a bitmap to 96x96 pixels and creates a DIB from it

int __stdcall MSN_BitmapToAvatarDibBits( HBITMAP hBitmap, BITMAPINFOHEADER*& ppDib, BYTE*& ppDibBits )
{
	BITMAP bmp;
	HDC hDC = CreateCompatibleDC( NULL );
	SelectObject( hDC, hBitmap );
	GetObject( hBitmap, sizeof( BITMAP ), &bmp );

	HDC hBmpDC = CreateCompatibleDC( hDC );
	HBITMAP hStretchedBitmap = CreateBitmap( 96, 96, 1, GetDeviceCaps( hDC, BITSPIXEL ), NULL );
	SelectObject( hBmpDC, hStretchedBitmap );
	int side, dx, dy;

	if ( bmp.bmWidth > bmp.bmHeight ) {
		side = bmp.bmHeight;
		dx = ( bmp.bmWidth - bmp.bmHeight )/2;
		dy = 0;
	}
	else {
		side = bmp.bmWidth;
		dx = 0;
		dy = ( bmp.bmHeight - bmp.bmWidth )/2;
	}

	SetStretchBltMode( hBmpDC, HALFTONE );
	StretchBlt( hBmpDC, 0, 0, 96, 96, hDC, dx, dy, side, side, SRCCOPY );
	DeleteObject( hBitmap );	
	DeleteDC( hDC );

	BITMAPINFO* bmi = ( BITMAPINFO* )alloca( sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256 );
	memset( bmi, 0, sizeof BITMAPINFO );
	bmi->bmiHeader.biSize = 0x28;
	if ( GetDIBits( hBmpDC, hStretchedBitmap, 0, 96, NULL, bmi, DIB_RGB_COLORS ) == 0 ) {
		TWinErrorCode errCode;
		MSN_ShowError( "Unable to get the bitmap: error %d (%s)", errCode.mErrorCode, errCode.getText() );
		return 2;
	}

	ppDib = ( BITMAPINFOHEADER* )GlobalAlloc( LPTR, sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256 + bmi->bmiHeader.biSizeImage );
	if ( ppDib == NULL )
		return 3;

	memcpy( ppDib, bmi, sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256 );
	ppDibBits = (( BYTE* )ppDib ) + sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256;

	GetDIBits( hBmpDC, hStretchedBitmap, 0, ppDib->biHeight, ppDibBits, ( BITMAPINFO* )ppDib, DIB_RGB_COLORS );
	DeleteObject( hStretchedBitmap );	
	DeleteDC( hBmpDC );
	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_DibBitsToAvatar - updates the avatar database settins and file from a DIB

int __stdcall MSN_DibBitsToAvatar( BITMAPINFOHEADER* pDib, BYTE* pDibBits )
{
	if ( !MSN_LoadPngModule())
		return 1;

	long dwPngSize = 0;
	if ( dib2pngConvertor(( BITMAPINFO* )pDib, pDibBits, NULL, &dwPngSize ) == 0 )
		return 2;

	BYTE* pPngMemBuffer = new BYTE[ dwPngSize ];
	dib2pngConvertor(( BITMAPINFO* )pDib, pDibBits, pPngMemBuffer, &dwPngSize );

	SHA1Context sha1ctx;
	BYTE sha1c[ SHA1HashSize ], sha1d[ SHA1HashSize ];
	char szSha1c[ 40 ], szSha1d[ 40 ];
	SHA1Reset( &sha1ctx );
	SHA1Input( &sha1ctx, pPngMemBuffer, dwPngSize );
	SHA1Result( &sha1ctx, sha1d );
	{	NETLIBBASE64 nlb = { szSha1d, sizeof szSha1d, ( PBYTE )sha1d, sizeof sha1d };
		MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb ));
	}

	SHA1Reset( &sha1ctx );

	char szEmail[ MSN_MAX_EMAIL_LEN ];
	MSN_GetStaticString( "e-mail", NULL, szEmail, sizeof szEmail );
	SHA1Input( &sha1ctx, ( PBYTE )"Creator", 7 );
	SHA1Input( &sha1ctx, ( PBYTE )szEmail, strlen( szEmail ));

	char szFileSize[ 20 ];
	ltoa( dwPngSize, szFileSize, 10 );
	SHA1Input( &sha1ctx, ( PBYTE )"Size", 4 );
	SHA1Input( &sha1ctx, ( PBYTE )szFileSize, strlen( szFileSize ));

	SHA1Input( &sha1ctx, ( PBYTE )"Type", 4 );
	SHA1Input( &sha1ctx, ( PBYTE )"3", 1 );

	SHA1Input( &sha1ctx, ( PBYTE )"Location", 8 );
	SHA1Input( &sha1ctx, ( PBYTE )"TFR43.dat", 9 );

	SHA1Input( &sha1ctx, ( PBYTE )"Friendly", 8 );
	SHA1Input( &sha1ctx, ( PBYTE )"AAA=", 4 );

	SHA1Input( &sha1ctx, ( PBYTE )"SHA1D", 5 );
	SHA1Input( &sha1ctx, ( PBYTE )szSha1d, strlen( szSha1d ));
	SHA1Result( &sha1ctx, sha1c );
	{	NETLIBBASE64 nlb = { szSha1c, sizeof szSha1c, ( PBYTE )sha1c, sizeof sha1c };
		MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb ));
	}
	{	char* szBuffer = ( char* )alloca( 1000 );
		_snprintf( szBuffer, 1000,
			"<msnobj Creator=\"%s\" Size=\"%ld\" Type=\"3\" Location=\"TFR43.dat\" Friendly=\"AAA=\" SHA1D=\"%s\" SHA1C=\"%s\"/>",
			szEmail, dwPngSize, szSha1d, szSha1c );

		char* szEncodedBuffer = ( char* )alloca( 1000 );
		UrlEncode( szBuffer, szEncodedBuffer, 1000 );

		MSN_SetString( NULL, "PictObject", szEncodedBuffer );
	}
	{	char tFileName[ MAX_PATH ];
		MSN_GetAvatarFileName( NULL, tFileName, sizeof tFileName );
		FILE* out = fopen( tFileName, "wb" );
		if ( out != NULL ) {
			fwrite( pPngMemBuffer, dwPngSize, 1, out );
			fclose( out );
	}	}
	delete pPngMemBuffer;
	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_LoadPictureToBitmap - retrieves a bitmap handle for any kind of pictures

int __stdcall MSN_EnterBitmapFileName( char* szDest )
{
	char szFilter[ 512 ];
	_snprintf( szFilter, sizeof( szFilter ),
		"%s%c*.BMP;*.RLE;*.JPG;*.JPEG;*.GIF;*.PNG%c%s%c*.BMP;*.RLE%c%s%c*.JPG;*.JPEG%c%s%c*.GIF%c%s%c*.PNG%c%s%c*%c%c0",
			MSN_Translate( "All Bitmaps" ), 0, 0,
			MSN_Translate( "Windows Bitmaps" ), 0, 0,
			MSN_Translate( "JPEG Bitmaps" ), 0, 0,
			MSN_Translate( "GIF Bitmaps" ), 0, 0,
			MSN_Translate( "PNG Bitmaps" ), 0, 0, 
			MSN_Translate( "All Files" ), 0, 0, 0 );

	*szDest = 0;

	char str[ MAX_PATH ]; str[0] = 0;
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof( OPENFILENAME );
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = szDest;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.nMaxFile = MAX_PATH;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrDefExt = "bmp";
	if ( !GetOpenFileName( &ofn ))
		return 1;

	return ERROR_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_LoadPictureToBitmap - retrieves a bitmap handle for any kind of pictures

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

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_PngToDibBits - loads a PNG image into the DIB

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

	if ( cbFileSize != 0 ) {
		if ( png2dibConvertor(( char* )ppMap, cbFileSize, &ppDib ))
			ppDibBits = ( BYTE* )( ppDib+1 );
		else 
			cbFileSize = 0;
	}

	if ( ppMap != NULL )	UnmapViewOfFile( ppMap );
	if ( hMap  != NULL )	CloseHandle( hMap );
	if ( hFile != NULL ) CloseHandle( hFile );

	return ( cbFileSize != 0 ) ? ERROR_SUCCESS : 2;
}

