/*
 * $Id$
 *
 * myYahoo Miranda Plugin 
 *
 * Authors: Gennady Feldman (aka Gena01) 
 *          Laurent Marechal (aka Peorth)
 *
 * This code is under GPL and is based on AIM, MSN and Miranda source code.
 * I want to thank Robert Rainwater and George Hazan for their code and support
 * and for answering some of my questions during development of this plugin.
 */
#include <windows.h>
#include "yahoo.h"
#include "http_gateway.h"
#include "version.h"
#include "resource.h"
#include <malloc.h>
#include <time.h>

#include <m_system.h>
#include <m_langpack.h>
#include <m_options.h>
#include <m_skin.h>
#include <m_message.h>
#include <m_idle.h>
#include <m_userinfo.h>

static BOOL CALLBACK AvatarDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

static HMODULE sttPngLib = NULL;

//extern pfnConvertPng2dib png2dibConvertor;

pfnConvertPng2dib png2dibConvertor = NULL;
pfnConvertDib2png dib2pngConvertor = NULL;

int YAHOO_SaveBitmapAsAvatar( HBITMAP hBitmap, const char* szFileName );
HBITMAP YAHOO_StretchBitmap( HBITMAP hBitmap );

/*
 * The Following PNG related stuff copied from MSN. Thanks George!
 */
BOOL YAHOO_LoadPngModule()
{
	if ( sttPngLib == NULL ) {
		if (( sttPngLib = LoadLibrary( "png2dib.dll" )) == NULL ) {
			char tDllPath[ MAX_PATH ];
			GetModuleFileName( hinstance, tDllPath, sizeof( tDllPath ));
			{
				char* p = strrchr( tDllPath, '\\' );
				if ( p != NULL )
					strcpy( p+1, "png2dib.dll" );
				else
					strcpy( tDllPath, "png2dib.dll" );
			}

			if (( sttPngLib = LoadLibrary( tDllPath )) == NULL ) {
				MessageBox( NULL,
					Translate( "Please install png2dib.dll for avatar support. " ),
					Translate( "Error" ),
					MB_OK | MB_ICONSTOP );
				YAHOO_SetByte("ShowAvatars", 0);
				return FALSE;
		}	}

		png2dibConvertor = ( pfnConvertPng2dib )GetProcAddress( sttPngLib, "mempng2dib" );
		dib2pngConvertor = ( pfnConvertDib2png )GetProcAddress( sttPngLib, "dib2mempng" );
		//getver           = ( pfnGetVer )        GetProcAddress( sttPngLib, "getver" );
		if ( png2dibConvertor == NULL) { // || dib2pngConvertor == NULL || getver == NULL ) {
			FreeLibrary( sttPngLib ); sttPngLib = NULL;
			MessageBox( NULL,
				Translate( "Your png2dib.dll is either obsolete or damaged. " ),
				Translate( "Error" ),
				MB_OK | MB_ICONSTOP );

			//goto LBL_Error;
			return FALSE;
	}	}

	return TRUE;
}

int OnDetailsInit(WPARAM wParam, LPARAM lParam)
{
  char* szProto;
  OPTIONSDIALOGPAGE odp;
  char szAvtCaption[MAX_PATH+8];

  //MessageBox(NULL, "HALLO!!", "AA", MB_OK);
  
  szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, lParam, 0);
  if ((lstrcmp(szProto, yahooProtocolName)) && lParam)
    return 0;

  //MessageBox(NULL, "HALLO 123!!", "AA", MB_OK);
  
  if ((lParam == 0) && YAHOO_GetByte( "ShowAvatars", 0 ))
  {
	 // Avatar page only for valid contacts
	  odp.cbSize = sizeof(odp);
	  odp.hIcon = NULL;
	  odp.hInstance = hinstance;
      odp.pfnDlgProc = AvatarDlgProc;
      odp.position = -1899999997;
      odp.pszTemplate = MAKEINTRESOURCE(IDD_INFO_AVATAR);
      snprintf(szAvtCaption, sizeof(szAvtCaption), "%s %s", Translate(yahooProtocolName), Translate("Avatar"));
      odp.pszTitle = szAvtCaption;

	  CallService(MS_USERINFO_ADDPAGE, wParam, (LPARAM)&odp);
  }

  return 0;
}

static char* ChooseAvatarFileName()
{
  char* szDest = (char*)malloc(MAX_PATH+0x10);
  char str[MAX_PATH];
  char szFilter[512];
  OPENFILENAME ofn = {0};
  str[0] = 0;
  szDest[0]='\0';
  CallService(MS_UTILS_GETBITMAPFILTERSTRINGS,sizeof(szFilter),(LPARAM)szFilter);
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.lpstrFilter = szFilter;
  //ofn.lpstrFilter = "PNG Bitmaps (*.png)\0*.png\0";
  ofn.lpstrFile = szDest;
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
  ofn.nMaxFile = MAX_PATH;
  ofn.nMaxFileTitle = MAX_PATH;
  ofn.lpstrDefExt = "png";
  if (!GetOpenFileName(&ofn))
  {
    free(&szDest);
    return NULL;
  }

  return szDest;
}

/*
 *31 bit hash function  - this is based on g_string_hash function from glib
 */
unsigned int YAHOO_avt_hash(const char *key, long ksize)
{
  const char *p = key;
  unsigned int h = *p;
  long l = 0;
  
  if (h)
	for (p += 1; l < ksize; p++, l++)
	  h = (h << 5) - h + *p;

  return h;
}
	
long calcMD5Hash(char* szFile)
{
  if (szFile) {
    HANDLE hFile = NULL, hMap = NULL;
    BYTE* ppMap = NULL;
    long cbFileSize = 0;
    long ck = 0;	

    if ((hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL )) != INVALID_HANDLE_VALUE)
      if ((hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL)
        if ((ppMap = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL)
          cbFileSize = GetFileSize( hFile, NULL );

    if (cbFileSize != 0){
      ck = YAHOO_avt_hash((char *)ppMap, cbFileSize);
    }

    if (ppMap != NULL) UnmapViewOfFile(ppMap);
    if (hMap  != NULL) CloseHandle(hMap);
    if (hFile != NULL) CloseHandle(hFile);

    if (ck) return ck;
  }
  
  return 0;
}

/**************** Send Avatar ********************/
void __cdecl yahoo_send_avt_thread(void *psf) 
{
	y_filetransfer *sf = psf;
	
//    ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
	if (sf == NULL) {
		YAHOO_DebugLog("SF IS NULL!!!");
		return;
	}
	YAHOO_DebugLog("[Uploading avatar] filename: %s ", sf->filename);
	
	YAHOO_SendAvatar(sf);
	free(sf->filename);
	free(sf);
}


static BOOL CALLBACK AvatarDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_INITDIALOG:
	  //MessageBox(NULL, "HALLO AVATARS!!", "AA", MB_OK);
    TranslateDialogDefault(hwndDlg);
    {
      /*DBVARIANT dbvHash, dbvSaved;
      DWORD dwUIN;*/
      //uid_str szUID;
	  DBVARIANT dbv;
      char szAvatar[MAX_PATH];

		ShowWindow(GetDlgItem(hwndDlg, -1), SW_SHOW);
        if (!yahooLoggedIn){
          EnableWindow(GetDlgItem(hwndDlg, IDC_SETAVATAR), FALSE);
          EnableWindow(GetDlgItem(hwndDlg, IDC_DELETEAVATAR), FALSE);
        }

		//GetAvatarFileName(NULL, szAvatar, MAX_PATH);
		//YAHOO_SetString(NULL, "AvatarFile", szMyFile);
		
		if (!DBGetContactSetting(NULL, yahooProtocolName, "AvatarFile", &dbv)) {
			HBITMAP avt;

			lstrcpy(szAvatar, dbv.pszVal);
			DBFreeVariant(&dbv);
			
			avt = (HBITMAP)CallService(MS_UTILS_LOADBITMAP, 0, (WPARAM)szAvatar);
			if (avt) {
				avt = (HBITMAP)SendDlgItemMessage(hwndDlg, IDC_AVATAR, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)avt);
				if (avt) DeleteObject(avt); // we release old avatar if any
			}
		}
    }
    return TRUE;
  
  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDC_SETAVATAR:
      {
        char* szFile;
        
        if ((szFile = ChooseAvatarFileName()) != NULL){ 
		  // user selected file for his avatar
          char szMyFile[MAX_PATH+1];
          //int dwPaFormat = DetectAvatarFormat(szFile);

          HBITMAP avt = (HBITMAP)CallService(MS_UTILS_LOADBITMAP, 0, (WPARAM)szFile);

		  if (avt == NULL)
			break;
		  
		  if (( avt = YAHOO_StretchBitmap( avt )) == NULL )
			break;

          //GetFullAvatarFileName(0, NULL, dwPaFormat, szMyFile, MAX_PATH);
		  GetAvatarFileName(NULL, szMyFile, MAX_PATH, 2);
		  
		  //lstrcpy(szMyFile, szFile);
		  YAHOO_SaveBitmapAsAvatar( avt, szMyFile);
          /*if (!CopyFile(szFile, szMyFile, FALSE)){
            YAHOO_DebugLog("Failed to copy our avatar to local storage.");*/
            
          //}

          if (avt) {
            unsigned int hash;
			y_filetransfer *sf;
			
            hash = calcMD5Hash(szMyFile);
            if (hash) {
			  YAHOO_DebugLog("[Avatar Info] File: '%s' CK: %d", szMyFile, hash);	
			  
			  /* now check and make sure we don't reupload same thing over again */
			  if (hash != YAHOO_GetDword("AvatarHash", 0) || time(NULL) > YAHOO_GetDword("AvatarTS",0)) {
			  YAHOO_SetString(NULL, "AvatarFile", szMyFile);
			  DBWriteContactSettingDword(NULL, yahooProtocolName, "AvatarHash", hash);

			  sf = (y_filetransfer*) malloc(sizeof(y_filetransfer));
			  sf->filename = strdup(szMyFile);
			  sf->cancel = 0;
			  pthread_create(yahoo_send_avt_thread, sf);
			  } else {
				YAHOO_DebugLog("[Avatar Info] Same checksum and avatar on YahooFT. Not Reuploading.");	  
			  }
            }
          }
          free(&szFile);
          if (avt) avt = (HBITMAP)SendDlgItemMessage(hwndDlg, IDC_AVATAR, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)avt);
          if (avt) DeleteObject(avt); // we release old avatar if any
        }
      }
      break;
    case IDC_DELETEAVATAR:
      {
        HBITMAP avt;

		YAHOO_DebugLog("[Deleting Avatar Info]");	
		
        avt = (HBITMAP)SendDlgItemMessage(hwndDlg, IDC_AVATAR, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
        if (avt) DeleteObject(avt); // we release old avatar if any
		
		/* remove ALL our Avatar Info Keys */
		DBDeleteContactSetting(NULL, yahooProtocolName, "AvatarFile");	
		DBDeleteContactSetting(NULL, yahooProtocolName, "AvatarHash");
		DBDeleteContactSetting(NULL, yahooProtocolName, "AvatarURL");	
		DBDeleteContactSetting(NULL, yahooProtocolName, "AvatarTS");	
		
		/* Send a Yahoo packet saying we don't got an avatar anymore */
		YAHOO_set_avatar(0);
		
		/* clear the avatar window */
		InvalidateRect( hwndDlg, NULL, TRUE );
      }
      break;
    }
    break;
  }

  return FALSE;
}

// 
// YAHOO_StretchBitmap (copied from MSN, Thanks George)
//
HBITMAP YAHOO_StretchBitmap( HBITMAP hBitmap )
{
	BITMAPINFO bmStretch; 
	BITMAP bmp;
	UINT* ptPixels;
	HDC hDC, hBmpDC;
	HBITMAP hOldBitmap1, hOldBitmap2, hStretchedBitmap;
	int side, dx, dy;
	
	ZeroMemory(&bmStretch, sizeof(bmStretch));
	bmStretch.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmStretch.bmiHeader.biWidth = 96;
	bmStretch.bmiHeader.biHeight = 96;
	bmStretch.bmiHeader.biPlanes = 1;
	bmStretch.bmiHeader.biBitCount = 32;

	hStretchedBitmap = CreateDIBSection( NULL, &bmStretch, DIB_RGB_COLORS, ( void* )&ptPixels, NULL, 0);
	if ( hStretchedBitmap == NULL ) {
		YAHOO_DebugLog( "Bitmap creation failed with error %ld", GetLastError() );
		return NULL;
	}

	hDC = CreateCompatibleDC( NULL );
	hOldBitmap1 = ( HBITMAP )SelectObject( hDC, hBitmap );
	GetObject( hBitmap, sizeof( BITMAP ), &bmp );

	hBmpDC = CreateCompatibleDC( hDC );
	hOldBitmap2 = ( HBITMAP )SelectObject( hBmpDC, hStretchedBitmap );

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

	SelectObject( hDC, hOldBitmap1 );
	DeleteObject( hBitmap );
	DeleteDC( hDC );

	SelectObject( hBmpDC, hOldBitmap2 );
	DeleteDC( hBmpDC );
	return hStretchedBitmap;
}

//
// YAHOO_SaveBitmapAsAvatar - updates the avatar database settins and file from a bitmap
//
int YAHOO_SaveBitmapAsAvatar( HBITMAP hBitmap, const char* szFileName ) 
{
	BITMAPINFO* bmi;
	HDC hdc;
	HBITMAP hOldBitmap;
	BITMAPINFOHEADER* pDib;
	BYTE* pDibBits;
	long dwPngSize = 0;
	
	if ( !YAHOO_LoadPngModule())
		return 1;

	hdc = CreateCompatibleDC( NULL );
	hOldBitmap = ( HBITMAP )SelectObject( hdc, hBitmap );

	bmi = ( BITMAPINFO* )_alloca( sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256 );
	memset( bmi, 0, sizeof (BITMAPINFO ));
	bmi->bmiHeader.biSize = 0x28;
	if ( GetDIBits( hdc, hBitmap, 0, 96, NULL, bmi, DIB_RGB_COLORS ) == 0 ) {
		/*TWinErrorCode errCode;
		MSN_ShowError( "Unable to get the bitmap: error %d (%s)", errCode.mErrorCode, errCode.getText() );*/
		return 2;
	}

	pDib = ( BITMAPINFOHEADER* )GlobalAlloc( LPTR, sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256 + bmi->bmiHeader.biSizeImage );
	if ( pDib == NULL )
		return 3;

	memcpy( pDib, bmi, sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256 );
	pDibBits = (( BYTE* )pDib ) + sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256;

	GetDIBits( hdc, hBitmap, 0, pDib->biHeight, pDibBits, ( BITMAPINFO* )pDib, DIB_RGB_COLORS );
	SelectObject( hdc, hOldBitmap );
	DeleteDC( hdc );

	if ( dib2pngConvertor(( BITMAPINFO* )pDib, pDibBits, NULL, &dwPngSize ) == 0 ) {
		GlobalFree( pDib );
		return 2;
	}

	{	char* pPngMemBuffer = (char*) malloc(dwPngSize);
		dib2pngConvertor(( BITMAPINFO* )pDib, pDibBits, pPngMemBuffer, &dwPngSize );
		GlobalFree( pDib );
		{	
			FILE* out;
			char tFileName[ MAX_PATH ];
			GetAvatarFileName( NULL, tFileName, sizeof tFileName, 2);
			out = fopen( tFileName, "wb" );
			if ( out != NULL ) {
				fwrite( pPngMemBuffer, dwPngSize, 1, out );
				fclose( out );
			}	
		}
		free(pPngMemBuffer);
	}
	return ERROR_SUCCESS;
}

