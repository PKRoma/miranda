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
#include <time.h>
#include <malloc.h>
#include <sys/stat.h>
#include <io.h>

#include "yahoo.h"
#include "resource.h"

#include <m_langpack.h>
#include <m_options.h>
#include <m_userinfo.h>
#include <m_png.h>
#include <m_system.h>

#include "avatar.h"
#include "file_transfer.h"

static BOOL CALLBACK AvatarDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

extern yahoo_local_account *ylad;

int OnDetailsInit(WPARAM wParam, LPARAM lParam)
{
  char* szProto;
  OPTIONSDIALOGPAGE odp = { 0 };
  char szAvtCaption[MAX_PATH+8];

  szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, lParam, 0);
  if ((lstrcmp(szProto, yahooProtocolName)) && lParam)
    return 0;

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
  if (!GetOpenFileName(&ofn)){
    free(szDest);
    return NULL;
  }

  return szDest;
}

/*
 *31 bit hash function  - this is based on g_string_hash function from glib
 */
int YAHOO_avt_hash(const char *key, long ksize)
{
  const char *p = key;
  int h = *p;
  long l = 0;
  
  if (h)
	for (p += 1; l < ksize; p++, l++)
	  h = (h << 5) - h + *p;

  return h;
}
	
static int calcHash(char* szFile)
{
  if (szFile) {
    HANDLE hFile = NULL, hMap = NULL;
    BYTE* ppMap = NULL;
    long cbFileSize = 0;
    int ck = 0;	

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

void upload_avt(int id, int fd, int error, void *data);

/**************** Send Avatar ********************/

HBITMAP YAHOO_SetAvatar(const char *szFile)
{
	char szMyFile[MAX_PATH+1];
    HBITMAP avt;

	avt = (HBITMAP)CallService(MS_UTILS_LOADBITMAP, 0, (WPARAM)szFile);
	
	if (avt == NULL)
		return NULL;
	  
	if (( avt = YAHOO_StretchBitmap( avt )) == NULL )
		return NULL;
	
	GetAvatarFileName(NULL, szMyFile, MAX_PATH, 2);
	  
	if (avt && YAHOO_SaveBitmapAsAvatar( avt, szMyFile) == 0) {
		unsigned int hash;
				
		hash = calcHash(szMyFile);
		if (hash) {
			LOG(("[YAHOO_SetAvatar] File: '%s' CK: %d", szMyFile, hash));	
			  
			/* now check and make sure we don't reupload same thing over again */
			if (hash != YAHOO_GetDword("AvatarHash", 0)) {
				YAHOO_SetString(NULL, "AvatarFile", szMyFile);
				DBWriteContactSettingDword(NULL, yahooProtocolName, "TMPAvatarHash", hash);
			
				YAHOO_SendAvatar(szMyFile);
			} else {
				LOG(("[YAHOO_SetAvatar] Same checksum and avatar on YahooFT. Not Reuploading."));	  
			}
		}
	}

	return avt;
}

static BOOL CALLBACK AvatarDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_INITDIALOG:
	  //MessageBox(NULL, "HALLO AVATARS!!", "AA", MB_OK);
    TranslateDialogDefault(hwndDlg);
    {
	  DBVARIANT dbv;
      char szAvatar[MAX_PATH];

		ShowWindow(GetDlgItem(hwndDlg, -1), SW_SHOW);
        if (!yahooLoggedIn){
          EnableWindow(GetDlgItem(hwndDlg, IDC_SETAVATAR), FALSE);
          EnableWindow(GetDlgItem(hwndDlg, IDC_DELETEAVATAR), FALSE);
		  EnableWindow(GetDlgItem(hwndDlg, IDC_SHARE_AVATAR), FALSE);
        }

		SetButtonCheck( hwndDlg, IDC_SHARE_AVATAR, YAHOO_GetByte( "ShareAvatar", 0 ) == 2 );
		
		if (!DBGetContactSetting(NULL, yahooProtocolName, "AvatarFile", &dbv)) {
			HBITMAP avt;

			lstrcpy(szAvatar, dbv.pszVal);
			DBFreeVariant(&dbv);
			
			avt = (HBITMAP)CallService(MS_UTILS_LOADBITMAP, 0, (WPARAM)szAvatar);
			if (avt) {
				avt = (HBITMAP)SendDlgItemMessage(hwndDlg, IDC_AVATAR, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)avt);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SHARE_AVATAR), TRUE);
				if (avt) DeleteObject(avt); // we release old avatar if any
			}
		} else {
			EnableWindow(GetDlgItem(hwndDlg, IDC_DELETEAVATAR), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_SHARE_AVATAR), FALSE);
		}
    }
    return TRUE;
  
  case WM_COMMAND:
    switch(LOWORD(wParam))
    {
    case IDC_SETAVATAR:
      {
        char* szFile;
        HBITMAP avt;
		
        if ((szFile = ChooseAvatarFileName()) != NULL) {
		  avt = YAHOO_SetAvatar(szFile);

		  free(szFile);

		  if (avt){
			avt = (HBITMAP)SendDlgItemMessage(hwndDlg, IDC_AVATAR, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)avt);
			EnableWindow(GetDlgItem(hwndDlg, IDC_DELETEAVATAR), TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_SHARE_AVATAR), TRUE);
		  }
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
		
		YAHOO_SetByte("ShareAvatar",0);
		SetButtonCheck(hwndDlg, IDC_SHARE_AVATAR, 0);
		EnableWindow(GetDlgItem(hwndDlg, IDC_DELETEAVATAR), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_SHARE_AVATAR), FALSE);
      }
      break;
	case IDC_SHARE_AVATAR:
		YAHOO_SetByte("ShareAvatar",IsDlgButtonChecked(hwndDlg, IDC_SHARE_AVATAR) ? 2 : 0);
		/* Send a Yahoo packet saying we don't got an avatar anymore */
		YAHOO_set_avatar(YAHOO_GetByte( "ShareAvatar", 0 )? 2 : 0);
    }
    break;
  }

  return FALSE;
}

/* 
 * YAHOO_StretchBitmap (copied from MSN, Thanks George)
 */
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

/*
 * YAHOO_SaveBitmapAsAvatar - updates the avatar database settings and file from a bitmap
 */
int YAHOO_SaveBitmapAsAvatar( HBITMAP hBitmap, const char* szFileName ) 
{
	BITMAPINFO* bmi;
	HDC hdc;
	HBITMAP hOldBitmap;
	BITMAPINFOHEADER* pDib;
	BYTE* pDibBits;
	long dwPngSize = 0;
	DIB2PNG convertor;
	
	if ( !ServiceExists(MS_DIB2PNG)) {
		MessageBox( NULL, Translate( "Your png2dib.dll is either obsolete or damaged. " ),
				Translate( "Error" ), MB_OK | MB_ICONSTOP );
		return 1;
	}

	hdc = CreateCompatibleDC( NULL );
	hOldBitmap = ( HBITMAP )SelectObject( hdc, hBitmap );

	bmi = ( BITMAPINFO* )_alloca( sizeof( BITMAPINFO ) + sizeof( RGBQUAD )*256 );
	memset( bmi, 0, sizeof (BITMAPINFO ));
	bmi->bmiHeader.biSize = 0x28;
	if ( GetDIBits( hdc, hBitmap, 0, 96, NULL, bmi, DIB_RGB_COLORS ) == 0 ) {
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

	convertor.pbmi = ( BITMAPINFO* )pDib;
	convertor.pDiData = pDibBits;
	convertor.pResult = NULL;
	convertor.pResultLen = &dwPngSize;
	if ( !CallService( MS_DIB2PNG, 0, (LPARAM)&convertor )) {
		GlobalFree( pDib );
		return 2;
	}

	convertor.pResult = (char *)malloc(dwPngSize);
	CallService( MS_DIB2PNG, 0, (LPARAM)&convertor );

	GlobalFree( pDib );
	{	
		FILE* out;
		
		out = fopen( szFileName, "wb" );
		if ( out != NULL ) {
			fwrite( convertor.pResult, dwPngSize, 1, out );
			fclose( out );
		}	
	}
	free(convertor.pResult);

	return ERROR_SUCCESS;
}

void upload_avt(int id, int fd, int error, void *data)
{
    y_filetransfer *sf = (y_filetransfer*) data;
    long size = 0;
	char buf[1024];
	int rw;			/* */
	DWORD dw;		/* needed for ReadFile */
	HANDLE myhFile;
	
	if (fd < 1 || error) {
		LOG(("[get_fd] Connect Failed!"));
		return;
	}

    myhFile  = CreateFile(sf->filename,
                          GENERIC_READ,
                          FILE_SHARE_READ|FILE_SHARE_WRITE,
			           NULL,
			           OPEN_EXISTING,
			           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			           0);

	if (myhFile == INVALID_HANDLE_VALUE) {
		LOG(("[get_fd] Can't open file for reading?!"));
		return;
	}
	
	LOG(("Sending file: %s size: %ld", sf->filename, sf->fsize));
	
	do {
		rw = ReadFile(myhFile, buf, sizeof(buf), &dw, NULL);
	
		if (rw != 0) {
			rw = Netlib_Send((HANDLE)fd, buf, dw, MSG_NODUMP);
			
			if (rw < 1) {
				LOG(("Upload Failed. Send error?"));
				YAHOO_ShowError(Translate("Yahoo Error"), Translate("Avatar upload failed!?!"));
				break;
			}
			
			size += rw;
		}
	} while (rw >= 0 && size < sf->fsize);
	
	CloseHandle(myhFile);
	
	do {
		rw = Netlib_Recv((HANDLE)fd, buf, sizeof(buf), 0);
		LOG(("Got: %d bytes", rw));
	} while (rw > 0);
	
    LOG(("File send complete!"));
}

void __cdecl yahoo_send_avt_thread(void *psf) 
{
	y_filetransfer *sf = psf;
	
	if (sf == NULL) {
		YAHOO_DebugLog("[yahoo_send_avt_thread] SF IS NULL!!!");
		return;
	}
	
	YAHOO_SetByte("AvatarUL", 1);
	yahoo_send_avatar(ylad->id, sf->filename, sf->fsize, &upload_avt, sf);

	free(sf->filename);
	free(sf);
	if (YAHOO_GetByte("AvatarUL", 1) == 1) YAHOO_SetByte("AvatarUL", 0);
}

void YAHOO_SendAvatar(const char *szFile)
{
	y_filetransfer *sf;
	struct _stat statbuf;
	
	if ( _stat( szFile, &statbuf ) != 0 ) {
		LOG(("[YAHOO_SendAvatar] Error reading File information?!"));
		return;
	}

	sf = (y_filetransfer*) malloc(sizeof(y_filetransfer));
	sf->filename = strdup(szFile);
	sf->cancel = 0;
	sf->fsize = statbuf.st_size;
	
	YAHOO_DebugLog("[Uploading avatar] filename: %s size: %ld", sf->filename, sf->fsize);

	mir_forkthread(yahoo_send_avt_thread, sf);
}

struct avatar_info{
	char *who;
	char *pic_url;
	int cksum;
};

static void __cdecl yahoo_recv_avatarthread(void *pavt) 
{
	PROTO_AVATAR_INFORMATION AI;
	struct avatar_info *avt = pavt;
	int 	error = 0;
	HANDLE 	hContact = 0;
	char 	buf[4096];
	
	if (avt == NULL) {
		YAHOO_DebugLog("AVT IS NULL!!!");
		return;
	}

	if (!yahooLoggedIn) {
		YAHOO_DebugLog("We are not logged in!!!");
		return;
	}
	
//    ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
	
	LOG(("yahoo_recv_avatarthread who:%s url:%s checksum: %d", avt->who, avt->pic_url, avt->cksum));

	hContact = getbuddyH(avt->who);
		
	if (!hContact){
		LOG(("ERROR: Can't find buddy: %s", avt->who));
		error = 1;
	} else if (!error) {
		DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", avt->cksum);
		DBWriteContactSettingDword(hContact, yahooProtocolName, "PictLoading", 1);
	}
	
    if(!error) {

    NETLIBHTTPREQUEST nlhr={0},*nlhrReply;
	
	nlhr.cbSize		= sizeof(nlhr);
	nlhr.requestType= REQUEST_GET;
	nlhr.flags		= NLHRF_NODUMP|NLHRF_GENERATEHOST|NLHRF_SMARTAUTHHEADER;
	nlhr.szUrl		= avt->pic_url;

	nlhrReply=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)hNetlibUser,(LPARAM)&nlhr);

	if(nlhrReply) {
		
		if (nlhrReply->resultCode != 200) {
			LOG(("Update server returned '%d' instead of 200. It also sent the following: %s", nlhrReply->resultCode, nlhrReply->szResultDescr));
			// make sure it's a real problem and not a problem w/ our connection
			yahoo_send_picture_info(ylad->id, avt->who, 3, avt->pic_url, avt->cksum);
			error = 1;
		} else if (nlhrReply->dataLength < 1 || nlhrReply->pData == NULL) {
			LOG(("No data??? Got %d bytes.", nlhrReply->dataLength));
			error = 1;
		} else {
			HANDLE myhFile;

			GetAvatarFileName(hContact, buf, 1024, DBGetContactSettingByte(hContact, yahooProtocolName,"AvatarType", 0));
			DeleteFile(buf);
			
			LOG(("Saving file: %s size: %u", buf, nlhrReply->dataLength));
			myhFile = CreateFile(buf,
								GENERIC_WRITE,
								FILE_SHARE_WRITE,
								NULL, OPEN_ALWAYS,  FILE_ATTRIBUTE_NORMAL,  0);
	
			if(myhFile !=INVALID_HANDLE_VALUE) {
				DWORD c;
				
				WriteFile(myhFile, nlhrReply->pData, nlhrReply->dataLength, &c, NULL );
				CloseHandle(myhFile);
				
				DBWriteContactSettingDword(hContact, yahooProtocolName, "PictLastCheck", 0);
			} else {
				LOG(("Can not open file for writing: %s", buf));
				error = 1;
			}
		}
		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,0,(LPARAM)nlhrReply);
	}
	}
	
	if (DBGetContactSettingDword(hContact, yahooProtocolName, "PictCK", 0) != avt->cksum) {
		LOG(("WARNING: Checksum updated during download?!"));
		error = 1; /* don't use this one? */
	} 
    
	DBWriteContactSettingDword(hContact, yahooProtocolName, "PictLoading", 0);
	LOG(("File download complete!?"));

	if (error) 
		buf[0]='\0';
	
	free(avt->who);
	free(avt->pic_url);
	free(avt);
	
	AI.cbSize = sizeof AI;
	AI.format = PA_FORMAT_PNG;
	AI.hContact = hContact;
	lstrcpy(AI.filename,buf);

	if (error) 
		DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", 0);
	
	ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_AVATAR, !error ? ACKRESULT_SUCCESS:ACKRESULT_FAILED,(HANDLE) &AI, 0);

}

void YAHOO_get_avatar(const char *who, const char *pic_url, long cksum)
{
	struct avatar_info *avt;
	
	avt = malloc(sizeof(struct avatar_info));
	avt->who = _strdup(who);
	avt->pic_url = _strdup(pic_url);
	avt->cksum = cksum;
	
	mir_forkthread(yahoo_recv_avatarthread, (void *) avt);
}

void ext_yahoo_got_picture(int id, const char *me, const char *who, const char *pic_url, int cksum, int type)
{
	HANDLE 	hContact = 0;
		
	LOG(("[ext_yahoo_got_picture] for %s with url %s (checksum: %d) type: %d", who, pic_url, cksum, type));
	
	
	/*
	  Type:
	
		1 - Send Avatar Info
		2 - Got Avatar Info
		3 - YIM6 didn't like my avatar? Expired? We need to invalidate and re-load
	 */
	switch (type) {
	case 1: 
		{
			int cksum=0;
			DBVARIANT dbv;
			
			/* need to send avatar info */
			if (!YAHOO_GetByte( "ShowAvatars", 0 )) {
				LOG(("[ext_yahoo_got_picture] We are not using/showing avatars!"));
				yahoo_send_picture_update(id, who, 0); // no avatar (disabled)
				return;
			}
		
			LOG(("[ext_yahoo_got_picture] Getting ready to send info!"));
			/* need to read CheckSum */
			cksum = YAHOO_GetDword("AvatarHash", 0);
			if (cksum) {
				if (!DBGetContactSetting(NULL, yahooProtocolName, "AvatarURL", &dbv)) {
					time_t ts;
					
					time(&ts);
					/* check expiration time */
					if (YAHOO_GetDword("AvatarExpires", ts) <= ts) {
						/* expired! */
						LOG(("[ext_yahoo_got_picture] Expired?? url: %s checksum: %d Expiration: %lu ", dbv.pszVal, cksum, YAHOO_GetDword("AvatarExpires", 0)));
					} else {
						LOG(("[ext_yahoo_got_picture] Sending url: %s checksum: %d to '%s'!", dbv.pszVal, cksum, who));
						//void yahoo_send_picture_info(int id, const char *me, const char *who, const char *pic_url, int cksum)
						yahoo_send_picture_info(id, who, 2, dbv.pszVal, cksum);
						DBFreeVariant(&dbv);
						break;
					}
					
				} 
				
				/*
				 * Try to re-upload the avatar
				 */
				if (YAHOO_GetByte("AvatarUL", 0) != 1){
					// NO avatar URL??
					if (!DBGetContactSetting(NULL, yahooProtocolName, "AvatarFile", &dbv)) {
						struct _stat statbuf;
						
						if (_stat( dbv.pszVal, &statbuf ) != 0 ) {
							LOG(("[ext_yahoo_got_picture] Avatar File Missing? Can't find file: %s", dbv.pszVal));
						} else {
							DBWriteContactSettingString(NULL, yahooProtocolName, "AvatarInv", who);
							YAHOO_SendAvatar(dbv.pszVal);
						}
						
						DBFreeVariant(&dbv);
					} else {
						LOG(("[ext_yahoo_got_picture] No Local Avatar File??? "));
					}
				} else 
						LOG(("[ext_yahoo_got_picture] Another avatar upload in progress?"));
			}
		}
		break;
	case 2: /*
		     * We got Avatar Info for our buddy. 
		     */
			if (!YAHOO_GetByte( "ShowAvatars", 0 )) {
				LOG(("[ext_yahoo_got_picture] We are not using/showing avatars!"));
				return;
			}
		
			/* got avatar info, so set miranda up */
			hContact = getbuddyH(who);
			
			if (!hContact) {
				LOG(("[ext_yahoo_got_picture] Buddy not on my buddy list?."));
				return;
			}
			
			if (!cksum || cksum == -1) {
				LOG(("[ext_yahoo_got_picture] Resetting avatar."));
				DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", 0);
				
				yahoo_reset_avatar(hContact);
			} else {
				char z[1024];
				
				if (pic_url == NULL) {
					LOG(("[ext_yahoo_got_picture] WARNING: Empty URL for avatar?"));
					return;
				}
				
				GetAvatarFileName(hContact, z, 1024, DBGetContactSettingByte(hContact, yahooProtocolName,"AvatarType", 0));
				
				if (DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0) != cksum || _access( z, 0 ) != 0 ) {
					
					YAHOO_DebugLog("[ext_yahoo_got_picture] Checksums don't match or avatar file is missing. Current: %d, New: %d",(int)DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0), cksum);

					YAHOO_get_avatar(who, pic_url, cksum);
				}
			}

		break;
	case 3: 
		/*
		 * Our Avatar is not good anymore? Need to re-upload??
		 */
		 /* who, pic_url, cksum */
		{
			int mcksum=0;
			DBVARIANT dbv;
			
			/* need to send avatar info */
			if (!YAHOO_GetByte( "ShowAvatars", 0 )) {
				LOG(("[ext_yahoo_got_picture] We are not using/showing avatars!"));
				yahoo_send_picture_update(id, who, 0); // no avatar (disabled)
				return;
			}
		
			LOG(("[ext_yahoo_got_picture] Getting ready to send info!"));
			/* need to read CheckSum */
			mcksum = YAHOO_GetDword("AvatarHash", 0);
			if (mcksum == 0) {
				/* this should NEVER Happen??? */
				LOG(("[ext_yahoo_got_picture] No personal checksum? and Invalidate?!"));
				yahoo_send_picture_update(id, who, 0); // no avatar (disabled)
				return;
			}
			
			LOG(("[ext_yahoo_got_picture] My Checksum: %d", mcksum));
			
			if (!DBGetContactSetting(NULL, yahooProtocolName, "AvatarURL", &dbv)){
					if (lstrcmpi(pic_url, dbv.pszVal) == 0) {
						DBVARIANT dbv2;
						time_t  ts;
						DWORD	ae;
						
						if (mcksum != cksum)
							LOG(("[ext_yahoo_got_picture] WARNING: Checksums don't match!"));	
						
						time(&ts);
						ae = YAHOO_GetDword("AvatarExpires", 0);
						
						if (ae != 0 && ae > (ts - 300)) {
							LOG(("[ext_yahoo_got_picture] We just reuploaded! Stop screwing with Yahoo FT. "));
							
							// don't leak stuff
							DBFreeVariant(&dbv);

							break;
						}
						
						LOG(("[ext_yahoo_got_picture] Buddy: %s told us this is bad??Expired??. Re-uploading", who));
						DBDeleteContactSetting(NULL, yahooProtocolName, "AvatarURL");
						
						if (!DBGetContactSetting(NULL, yahooProtocolName, "AvatarFile", &dbv2)) {
							DBWriteContactSettingString(NULL, yahooProtocolName, "AvatarInv", who);
							YAHOO_SendAvatar(dbv2.pszVal);
							DBFreeVariant(&dbv2);
						} else {
							LOG(("[ext_yahoo_got_picture] No Local Avatar File??? "));
						}
					} else {
						LOG(("[ext_yahoo_got_picture] URL doesn't match? Tell them the right thing!!!"));
						yahoo_send_picture_info(id, who, 2, dbv.pszVal, mcksum);
					}
					// don't leak stuff
					DBFreeVariant(&dbv);
			} else {
				LOG(("[ext_yahoo_got_picture] no AvatarURL?"));
			}
		}
		break;
	default:
		LOG(("[ext_yahoo_got_picture] Unknown request/packet type exiting!"));
	}
	
	LOG(("ext_yahoo_got_picture exiting"));
}

void ext_yahoo_got_picture_checksum(int id, const char *me, const char *who, int cksum)
{
	HANDLE 	hContact = 0;

	LOG(("ext_yahoo_got_picture_checksum for %s checksum: %d", who, cksum));

	hContact = getbuddyH(who);
	if (hContact == NULL) {
		LOG(("Buddy Not Found. Skipping avatar update"));
		return;
	}
	
	/* Last thing check the checksum and request new one if we need to */
	if (!cksum || cksum == -1) {
		DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", 0);
		
        yahoo_reset_avatar(hContact);
	} else {
		if (DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0) != cksum) {
			//char szFile[MAX_PATH];

			// Now save the new checksum. No rush requesting new avatar yet.
			DBWriteContactSettingDword(hContact, yahooProtocolName, "PictCK", cksum);
			
			// Need to delete the Avatar File!!
			/*GetAvatarFileName(hContact, szFile, sizeof szFile, 0);
			DeleteFile(szFile);
			
			// Reset the avatar and cleanup.
			yahoo_reset_avatar(hContact);*/
		}
	}
	
}

void ext_yahoo_got_picture_update(int id, const char *me, const char *who, int buddy_icon)
{
	HANDLE 	hContact = 0;

	LOG(("ext_yahoo_got_picture_update for %s buddy_icon: %d", who, buddy_icon));

	hContact = getbuddyH(who);
	if (hContact == NULL) {
		LOG(("Buddy Not Found. Skipping avatar update"));
		return;
	}
	
	DBWriteContactSettingByte(hContact, yahooProtocolName, "AvatarType", buddy_icon);
	
	/* Last thing check the checksum and request new one if we need to */
	yahoo_reset_avatar(hContact);
}

void ext_yahoo_got_picture_status(int id, const char *me, const char *who, int buddy_icon)
{
	HANDLE 	hContact = 0;

	LOG(("ext_yahoo_got_picture_status for %s buddy_icon: %d", who, buddy_icon));

	hContact = getbuddyH(who);
	if (hContact == NULL) {
		LOG(("Buddy Not Found. Skipping avatar update"));
		return;
	}
	
	DBWriteContactSettingByte(hContact, yahooProtocolName, "AvatarType", buddy_icon);
	
	/* Last thing check the checksum and request new one if we need to */
	yahoo_reset_avatar(hContact);
}

void yahoo_reset_avatar(HANDLE 	hContact)
{
	LOG(("[YAHOO_RESET_AVATAR]"));

	ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, 0);
}

void YAHOO_request_avatar(const char* who)
{
	time_t  last_chk, cur_time;
	HANDLE 	hContact = 0;
	char    szFile[MAX_PATH];
	
	if (!YAHOO_GetByte( "ShowAvatars", 0 )) {
		LOG(("Avatars disabled, but available for: %s", who));
		return;
	}
	
	hContact = getbuddyH(who);
	
	if (!hContact)
		return;
	
	
	GetAvatarFileName(hContact, szFile, sizeof szFile, DBGetContactSettingByte(hContact, yahooProtocolName,"AvatarType", 0));
	DeleteFile(szFile);
	
	time(&cur_time);
	last_chk = DBGetContactSettingDword(hContact, yahooProtocolName, "PictLastCheck", 0);
	
	/*
	 * time() - in seconds ( 60*60 = 1 hour)
	 */
	if (DBGetContactSettingDword(hContact, yahooProtocolName,"PictCK", 0) == 0 || 
		last_chk == 0 || (cur_time - last_chk) > 60) {
			
		DBWriteContactSettingDword(hContact, yahooProtocolName, "PictLastCheck", cur_time);

		LOG(("Requesting Avatar for: %s", who));
		yahoo_request_buddy_avatar(ylad->id, who);
	} else {
		LOG(("Avatar Not Available for: %s Last Check: %ld Current: %ld (Flood Check in Effect)", who, last_chk, cur_time));
	}
}

void YAHOO_bcast_picture_update(int buddy_icon)
{
	HANDLE hContact;
	char *szProto;
	
	/* need to get online buddies and then send then picture_update packets (ARGH YAHOO!)*/
	for ( hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		   hContact != NULL;
			hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 ))
	{
		szProto = ( char* )YAHOO_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact, 0 );
		if ( szProto != NULL && !lstrcmp( szProto, yahooProtocolName ))
		{
			if (YAHOO_GetWord(hContact, "Status", ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE) {
							DBVARIANT dbv;

				if ( DBGetContactSetting( hContact, yahooProtocolName, YAHOO_LOGINID, &dbv ))
					continue;

				yahoo_send_picture_update(ylad->id, dbv.pszVal, buddy_icon);
				DBFreeVariant( &dbv );
			}
		}
	}
}

void YAHOO_set_avatar(int buddy_icon)
{
	yahoo_send_picture_status(ylad->id,buddy_icon);
	
	YAHOO_bcast_picture_update(buddy_icon);
}

void YAHOO_bcast_picture_checksum(int cksum)
{
	HANDLE hContact;
	char *szProto;
	
	yahoo_send_picture_checksum(ylad->id, NULL, cksum);
	
	/* need to get online buddies and then send then picture_update packets (ARGH YAHOO!)*/
	for ( hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		   hContact != NULL;
			hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 ))
	{
		szProto = ( char* )YAHOO_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact, 0 );
		if ( szProto != NULL && !lstrcmp( szProto, yahooProtocolName ))
		{
			if (YAHOO_GetWord(hContact, "Status", ID_STATUS_OFFLINE)!=ID_STATUS_OFFLINE) {
							DBVARIANT dbv;

				if ( DBGetContactSetting( hContact, yahooProtocolName, YAHOO_LOGINID, &dbv ))
					continue;

				yahoo_send_picture_checksum(ylad->id, dbv.pszVal, cksum);
				DBFreeVariant( &dbv );
			}
		}
	}
}

void GetAvatarFileName(HANDLE hContact, char* pszDest, int cbLen, int type)
{
  int tPathLen;
  DBVARIANT dbv;
  
  CallService(MS_DB_GETPROFILEPATH, cbLen, (LPARAM)pszDest);

  tPathLen = lstrlen(pszDest);
  _snprintf(pszDest + tPathLen, MAX_PATH-tPathLen, "\\%s\\", yahooProtocolName);
  CreateDirectory(pszDest, NULL);

  if (hContact != NULL && !DBGetContactSetting(hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
		lstrcat(pszDest, dbv.pszVal);
		DBFreeVariant(&dbv);
  }else {
		lstrcat(pszDest, "avatar");
  }
  
  if (type == 1) {
	lstrcat(pszDest, ".swf" );
  } else
	lstrcat(pszDest, ".png" );
  
}

int YahooGetAvatarInfo(WPARAM wParam,LPARAM lParam)
{
	PROTO_AVATAR_INFORMATION* AI = ( PROTO_AVATAR_INFORMATION* )lParam;

	DBVARIANT dbv;
	int avtType;
	
	if (!DBGetContactSetting(AI->hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
		YAHOO_DebugLog("[YAHOO_GETAVATARINFO] For: %s", dbv.pszVal);
		DBFreeVariant(&dbv);
	}else {
		YAHOO_DebugLog("[YAHOO_GETAVATARINFO]");
	}

	if (!YAHOO_GetByte( "ShowAvatars", 0 ) || !yahooLoggedIn) {
		YAHOO_DebugLog("[YAHOO_GETAVATARINFO] %s", yahooLoggedIn ? "We are not using/showing avatars!" : "We are not logged in. Can't load avatars now!");
		
		/*if (DBGetContactSettingDword(AI->hContact, yahooProtocolName,"PictCK", 0) != 0) {
			YAHOO_DebugLog("[YAHOO_GETAVATARINFO] Removing avatar information!");
			
			DBWriteContactSettingDword(AI->hContact, yahooProtocolName, "PictCK", 0);
			DBWriteContactSettingDword(AI->hContact, yahooProtocolName, "PictLastCheck", 0);
			DBWriteContactSettingDword(AI->hContact, yahooProtocolName, "PictLoading", 0);
			//GetAvatarFileName(AI->hContact, AI->filename, sizeof AI->filename);
			//DeleteFile(AI->filename);
		}*/

		return GAIR_NOAVATAR;
	}
	
	avtType  = DBGetContactSettingByte(AI->hContact, yahooProtocolName,"AvatarType", 0);
	YAHOO_DebugLog("[YAHOO_GETAVATARINFO] Avatar Type: %d", avtType);
	
	if ( avtType != 2) {
		if (avtType != 0)
			YAHOO_DebugLog("[YAHOO_GETAVATARINFO] Not handling this type yet!");
		
		return GAIR_NOAVATAR;
	}
	
	if (DBGetContactSettingDword(AI->hContact, yahooProtocolName,"PictCK", 0) != 0) {
		
		GetAvatarFileName(AI->hContact, AI->filename, sizeof AI->filename,DBGetContactSettingByte(AI->hContact, yahooProtocolName,"AvatarType", 0));
		//if ( access( AI->filename, 0 ) == 0 ) {
		AI->format = PA_FORMAT_PNG;
		YAHOO_DebugLog("[YAHOO_GETAVATARINFO] filename: %s", AI->filename);
		
		if (_access( AI->filename, 0 ) == 0 ) {
			return GAIR_SUCCESS;
		} else {
			/* need to request it again? */
			if (YAHOO_GetWord(AI->hContact, "PictLoading", 0) != 0 &&
				(time(NULL) - YAHOO_GetWord(AI->hContact, "PictLastCK", 0) < 500)) {
				YAHOO_DebugLog("[YAHOO_GETAVATARINFO] Waiting for avatar to load!");
				return GAIR_WAITFOR;
			} else if ( yahooLoggedIn ) {
				DBVARIANT dbv;
	  
				if (!DBGetContactSetting(AI->hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
					YAHOO_DebugLog("[YAHOO_GETAVATARINFO] Requesting avatar!");
					
					YAHOO_request_avatar(dbv.pszVal/*who */);
					DBFreeVariant(&dbv);
					return GAIR_WAITFOR;
				} else {
					YAHOO_DebugLog("[YAHOO_GETAVATARINFO] Can't retrieve user id?!");
				}
			}
		}
	} 
	
	YAHOO_DebugLog("[YAHOO_GETAVATARINFO] NO AVATAR???");
	return GAIR_NOAVATAR;
}

/*
 * --=[ AVS / LoadAvatars API/Services ]=--
 */

/*
Optional. Will pass PNG or BMP if this is not found
wParam = 0
lParam = PA_FORMAT_*   // avatar format
return = 1 (supported) or 0 (not supported)
*/
int YahooAvatarFormatSupported(WPARAM wParam, LPARAM lParam)
{
  YAHOO_DebugLog("[YahooAvatarFormatSupported]");

  if (lParam == PA_FORMAT_PNG)
    return 1;
  else
    return 0;
}

/*
Service: /GetMyAvatarMaxSize
wParam=(int *)max width of avatar
lParam=(int *)max height of avatar
return=0
*/
int YahooGetAvatarSize(WPARAM wParam, LPARAM lParam)
{
	YAHOO_DebugLog("[YahooGetAvatarSize]");
	
	if (wParam != 0) *((int*) wParam) = 96;
	if (lParam != 0) *((int*) lParam) = 96;

	return 0;
}

/*
Service: /GetMyAvatar
wParam=(char *)Buffer to file name
lParam=(int)Buffer size
return=0 on success, else on error
*/
int YahooGetMyAvatar(WPARAM wParam, LPARAM lParam)
{
	char *buffer = (char *)wParam;
	int size = (int)lParam;

	YAHOO_DebugLog("[YahooGetMyAvatar]");
	
	if (buffer == NULL || size <= 0)
		return -1;
	

	if (!YAHOO_GetByte( "ShowAvatars", 0 ))
		return -2;
	
	{
		DBVARIANT dbv;
		int ret = -3;

		if (YAHOO_GetDword("AvatarHash", 0)){
			
			if (!DBGetContactSetting(NULL, yahooProtocolName, "AvatarFile", &dbv)){
				if (access(dbv.pszVal, 0) == 0){
					strncpy(buffer, dbv.pszVal, size-1);
					buffer[size-1] = '\0';

					ret = 0;
				}
				DBFreeVariant(&dbv);
			}
		}

		return ret;
	}
}

/*
#define PS_SETMYAVATAR "/SetMyAvatar"
wParam=0
lParam=(const char *)Avatar file name
return=0 for sucess
*/

int YahooSetMyAvatar(WPARAM wParam, LPARAM lParam)
{
	char *szFile = (char *)lParam;
	HANDLE avt;

	YAHOO_DebugLog("[YahooSetMyAvatar]");
	
	avt = YAHOO_SetAvatar(szFile);
	if (avt) {
		DeleteObject(avt); // we release old avatar if any
		return 0; 
	} else 
		return 1; /* error for now */
}

/*
 * --=[ ]=--
 */
