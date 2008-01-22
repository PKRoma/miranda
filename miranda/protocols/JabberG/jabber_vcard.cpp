/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#include <commctrl.h>
#include "jabber_iq.h"
#include "resource.h"
#include "jabber_caps.h"

/////////////////////////////////////////////////////////////////////////////////////////

static BOOL CALLBACK JabberVcardDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam );

int __cdecl CJabberProto::JabberMenuHandleVcard( WPARAM wParam, LPARAM lParam )
{
	if ( IsWindow( hwndJabberVcard ))
		SetForegroundWindow( hwndJabberVcard );
	else
		hwndJabberVcard = CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_VCARD ), NULL, JabberVcardDlgProc, ( LPARAM )this );

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

int CJabberProto::JabberSendGetVcard( const TCHAR* jid )
{
	int iqId = JabberSerialNext();
	JabberIqAdd( iqId, ( jid == jabberJID ) ? IQ_PROC_GETVCARD : IQ_PROC_NONE, &CJabberProto::JabberIqResultGetVcard );

	XmlNodeIq iq( "get", iqId, jid );
	XmlNode* vs = iq.addChild( "vCard" ); vs->addAttr( "xmlns", JABBER_FEAT_VCARD_TEMP ); 
	vs->addAttr( "prodid", "-//HandGen//NONSGML vGen v1.0//EN" ); vs->addAttr( "version", "2.0" );
	jabberThreadInfo->send( iq );
	return iqId;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct VcardPage
{
	HWND hwnd;
	int dlgId;
	DLGPROC dlgProc;
};

struct VcardTab
{
	CJabberProto* ppro;
	int pageCount;
	int currentPage;
	RECT rectTab;
	VcardPage *page;
	BOOL changed;
	int updateAnimFrame;
	TCHAR* szUpdating;
	BOOL animating;
};

static void SetDialogField( CJabberProto* ppro, HWND hwndDlg, int nDlgItem, char* key )
{
	DBVARIANT dbv;

	if ( !DBGetContactSettingTString( NULL, ppro->szProtoName, key, &dbv )) {
		SetDlgItemText( hwndDlg, nDlgItem, dbv.ptszVal );
		JFreeVariant( &dbv );
	}
	else SetDlgItemTextA( hwndDlg, nDlgItem, "" );
}

static BOOL CALLBACK PersonalDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg );
		SendMessage( GetDlgItem( hwndDlg, IDC_GENDER ), CB_ADDSTRING, 0, ( LPARAM )TranslateT( "Male" ));
		SendMessage( GetDlgItem( hwndDlg, IDC_GENDER ), CB_ADDSTRING, 0, ( LPARAM )TranslateT( "Female" ));
		SendMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
		SetWindowLong( hwndDlg, GWL_USERDATA, lParam );
		return TRUE;
	case WM_JABBER_REFRESH:
	{
		CJabberProto* ppro = ( CJabberProto* )GetWindowLong( hwndDlg, GWL_USERDATA );
		SetDialogField( ppro, hwndDlg, IDC_FULLNAME, "FullName" );
		SetDialogField( ppro, hwndDlg, IDC_NICKNAME, "Nick" );
		SetDialogField( ppro, hwndDlg, IDC_FIRSTNAME, "FirstName" );
		SetDialogField( ppro, hwndDlg, IDC_MIDDLE, "MiddleName" );
		SetDialogField( ppro, hwndDlg, IDC_LASTNAME, "LastName" );
		SetDialogField( ppro, hwndDlg, IDC_BIRTH, "BirthDate" );
		SetDialogField( ppro, hwndDlg, IDC_GENDER, "GenderString" );
		SetDialogField( ppro, hwndDlg, IDC_OCCUPATION, "Role" );
		SetDialogField( ppro, hwndDlg, IDC_HOMEPAGE, "Homepage" );
		break;
	}
	case WM_COMMAND:
		if (( ( HWND )lParam==GetFocus() && HIWORD( wParam )==EN_CHANGE ) ||
			(( HWND )lParam==GetDlgItem( hwndDlg, IDC_GENDER ) && ( HIWORD( wParam )==CBN_EDITCHANGE||HIWORD( wParam )==CBN_SELCHANGE )) )
			SendMessage( GetParent( hwndDlg ), WM_JABBER_CHANGED, 0, 0 );
		break;
	}
	return FALSE;
}

static BOOL CALLBACK HomeDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg );
		SendMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
		SetWindowLong( hwndDlg, GWL_USERDATA, lParam );
		return TRUE;
	case WM_JABBER_REFRESH:
	{
		CJabberProto* ppro = ( CJabberProto* )GetWindowLong( hwndDlg, GWL_USERDATA );
		SetDialogField( ppro, hwndDlg, IDC_ADDRESS1, "Street" );
		SetDialogField( ppro, hwndDlg, IDC_ADDRESS2, "Street2" );
		SetDialogField( ppro, hwndDlg, IDC_CITY, "City" );
		SetDialogField( ppro, hwndDlg, IDC_STATE, "State" );
		SetDialogField( ppro, hwndDlg, IDC_ZIP, "ZIP" );
		SetDialogField( ppro, hwndDlg, IDC_COUNTRY, "CountryName" );
		break;
	}
	case WM_COMMAND:
		if (( HWND )lParam==GetFocus() && HIWORD( wParam )==EN_CHANGE )
			SendMessage( GetParent( hwndDlg ), WM_JABBER_CHANGED, 0, 0 );
		break;
	}
	return FALSE;
}

static BOOL CALLBACK WorkDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg );
		SendMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
		SetWindowLong( hwndDlg, GWL_USERDATA, lParam );
		return TRUE;
	case WM_JABBER_REFRESH:
	{
		CJabberProto* ppro = ( CJabberProto* )GetWindowLong( hwndDlg, GWL_USERDATA );
		SetDialogField( ppro, hwndDlg, IDC_COMPANY, "Company" );
		SetDialogField( ppro, hwndDlg, IDC_DEPARTMENT, "CompanyDepartment" );
		SetDialogField( ppro, hwndDlg, IDC_TITLE, "CompanyPosition" );
		SetDialogField( ppro, hwndDlg, IDC_ADDRESS1, "CompanyStreet" );
		SetDialogField( ppro, hwndDlg, IDC_ADDRESS2, "CompanyStreet2" );
		SetDialogField( ppro, hwndDlg, IDC_CITY, "CompanyCity" );
		SetDialogField( ppro, hwndDlg, IDC_STATE, "CompanyState" );
		SetDialogField( ppro, hwndDlg, IDC_ZIP, "CompanyZIP" );
		SetDialogField( ppro, hwndDlg, IDC_COUNTRY, "CompanyCountryName" );
		break;
	}
	case WM_COMMAND:
		if (( HWND )lParam==GetFocus() && HIWORD( wParam )==EN_CHANGE )
			SendMessage( GetParent( hwndDlg ), WM_JABBER_CHANGED, 0, 0 );
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////

static char szPhotoFileName[MAX_PATH];
static char szPhotoType[33];
static BOOL bPhotoChanged;

static BOOL CALLBACK PhotoDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	static HBITMAP hBitmap;
	char szTempPath[MAX_PATH], szTempFileName[MAX_PATH];
	CJabberProto* ppro = ( CJabberProto* )GetWindowLong( hwndDlg, GWL_USERDATA );

	switch ( msg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg );
		hBitmap = NULL;
		SendMessage( GetDlgItem( hwndDlg, IDC_LOAD ), BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadImage( hInst, MAKEINTRESOURCE( IDI_OPEN ), IMAGE_ICON, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 ));
		SendMessage( GetDlgItem( hwndDlg, IDC_DELETE ), BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadImage( hInst, MAKEINTRESOURCE( IDI_DELETE ), IMAGE_ICON, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 ));
		ShowWindow( GetDlgItem( hwndDlg, IDC_SAVE ), SW_HIDE );
		SendMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
		SetWindowLong( hwndDlg, GWL_USERDATA, lParam );
		return TRUE;

	case WM_JABBER_REFRESH:
		if ( hBitmap ) {
			DeleteObject( hBitmap );
			hBitmap = NULL;
			DeleteFileA( szPhotoFileName );
			szPhotoFileName[0] = '\0';
		}
		EnableWindow( GetDlgItem( hwndDlg, IDC_DELETE ), FALSE );
		if ( ppro->jabberVcardPhotoFileName ) {
			if ( GetTempPathA( sizeof( szTempPath ), szTempPath ) <= 0 )
				strcpy( szTempPath, ".\\" );
			if ( GetTempFileNameA( szTempPath, "jab", 0, szTempFileName ) > 0 ) {
				ppro->JabberLog( "Temp file = %s", szTempFileName );
				if ( CopyFileA( ppro->jabberVcardPhotoFileName, szTempFileName, FALSE ) == TRUE ) {
					if (( hBitmap=( HBITMAP ) JCallService( MS_UTILS_LOADBITMAP, 0, ( LPARAM )szTempFileName )) != NULL ) {
						JabberBitmapPremultiplyChannels(hBitmap);
						strcpy( szPhotoFileName, szTempFileName );
						EnableWindow( GetDlgItem( hwndDlg, IDC_DELETE ), TRUE );
					}
					else DeleteFileA( szTempFileName );
				}
				else DeleteFileA( szTempFileName );
			}
		}
		bPhotoChanged = FALSE;
		InvalidateRect( hwndDlg, NULL, TRUE );
		UpdateWindow( hwndDlg );
		break;

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_LOAD:
			{
				char szFilter[512];
				char szFileName[_MAX_PATH];

				JCallService( MS_UTILS_GETBITMAPFILTERSTRINGS, ( WPARAM ) sizeof( szFilter ), ( LPARAM )szFilter );

				OPENFILENAMEA ofn = {0};
				ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
				ofn.hwndOwner = hwndDlg;
				ofn.lpstrFilter = szFilter;
				ofn.lpstrCustomFilter = NULL;
				ofn.lpstrFile = szFileName;
				ofn.nMaxFile = _MAX_PATH;
				ofn.Flags = OFN_FILEMUSTEXIST;
				szFileName[0] = '\0';
				if ( GetOpenFileNameA( &ofn )) {
					struct _stat st;
					HBITMAP hNewBitmap;

					ppro->JabberLog( "File selected is %s", szFileName );
					if ( _stat( szFileName, &st )<0 || st.st_size>40*1024 ) {
						MessageBox( hwndDlg, TranslateT( "Only JPG, GIF, and BMP image files smaller than 40 KB are supported." ), TranslateT( "Jabber vCard" ), MB_OK|MB_SETFOREGROUND );
						break;
					}
					if ( GetTempPathA( sizeof( szTempPath ), szTempPath ) <= 0 )
						strcpy( szTempPath, ".\\" );
					if ( GetTempFileNameA( szTempPath, "jab", 0, szTempFileName ) > 0 ) {
						ppro->JabberLog( "Temp file = %s", szTempFileName );
						if ( CopyFileA( szFileName, szTempFileName, FALSE ) == TRUE ) {
							if (( hNewBitmap=( HBITMAP ) JCallService( MS_UTILS_LOADBITMAP, 0, ( LPARAM )szTempFileName )) != NULL ) {
								if ( hBitmap ) {
									DeleteObject( hBitmap );
									DeleteFileA( szPhotoFileName );
								}

								char* p = strrchr( szFileName, '.' );
								if ( p != NULL ) {
									if ( !stricmp( p, ".bmp" ))
										strcpy( szPhotoType, "image/bmp" );
									else if ( !stricmp( p, ".gif" ))
										strcpy( szPhotoType, "image/gif" );
									else
										strcpy( szPhotoType, "image/jpeg" );
								}
								else
									strcpy( szPhotoType, "image/jpeg" );
								hBitmap = hNewBitmap;
								strcpy( szPhotoFileName, szTempFileName );
								bPhotoChanged = TRUE;
								EnableWindow( GetDlgItem( hwndDlg, IDC_DELETE ), TRUE );
								InvalidateRect( hwndDlg, NULL, TRUE );
								UpdateWindow( hwndDlg );
								SendMessage( GetParent( hwndDlg ), WM_JABBER_CHANGED, 0, 0 );
							}
							else DeleteFileA( szTempFileName );
						}
						else DeleteFileA( szTempFileName );
					}
				}
			}
			break;
		case IDC_DELETE:
			if ( hBitmap ) {
				DeleteObject( hBitmap );
				hBitmap = NULL;
				DeleteFileA( szPhotoFileName );
				szPhotoFileName[0] = '\0';
				bPhotoChanged = TRUE;
				EnableWindow( GetDlgItem( hwndDlg, IDC_DELETE ), FALSE );
				InvalidateRect( hwndDlg, NULL, TRUE );
				UpdateWindow( hwndDlg );
				SendMessage( GetParent( hwndDlg ), WM_JABBER_CHANGED, 0, 0 );
			}
			break;
		}
		break;
	case WM_PAINT:
		if ( hBitmap ) {
			BITMAP bm;
			HDC hdcMem;
			HWND hwndCanvas;
			HDC hdcCanvas;
			POINT ptSize, ptOrg, pt, ptFitSize;
			RECT rect;

			hwndCanvas = GetDlgItem( hwndDlg, IDC_CANVAS );
			hdcCanvas = GetDC( hwndCanvas );
			hdcMem = CreateCompatibleDC( hdcCanvas );
			SelectObject( hdcMem, hBitmap );
			SetMapMode( hdcMem, GetMapMode( hdcCanvas ));
			GetObject( hBitmap, sizeof( BITMAP ), ( LPVOID ) &bm );
			ptSize.x = bm.bmWidth;
			ptSize.y = bm.bmHeight;
			DPtoLP( hdcCanvas, &ptSize, 1 );
			ptOrg.x = ptOrg.y = 0;
			DPtoLP( hdcMem, &ptOrg, 1 );
			GetClientRect( hwndCanvas, &rect );
			InvalidateRect( hwndCanvas, NULL, TRUE );
			UpdateWindow( hwndCanvas );
			if ( ptSize.x<=rect.right && ptSize.y<=rect.bottom ) {
				pt.x = ( rect.right - ptSize.x )/2;
				pt.y = ( rect.bottom - ptSize.y )/2;
				ptFitSize = ptSize;
			}
			else {
				if (( ( float )( ptSize.x-rect.right ))/ptSize.x > (( float )( ptSize.y-rect.bottom ))/ptSize.y ) {
					ptFitSize.x = rect.right;
					ptFitSize.y = ( ptSize.y*rect.right )/ptSize.x;
					pt.x = 0;
					pt.y = ( rect.bottom - ptFitSize.y )/2;
				}
				else {
					ptFitSize.x = ( ptSize.x*rect.bottom )/ptSize.y;
					ptFitSize.y = rect.bottom;
					pt.x = ( rect.right - ptFitSize.x )/2;
					pt.y = 0;
				}
			}

			if (JabberIsThemeActive && JabberDrawThemeParentBackground && JabberIsThemeActive())
			{
				RECT rc; GetClientRect(hwndCanvas, &rc);
				JabberDrawThemeParentBackground(hwndCanvas, hdcCanvas, &rc);
			} else
			{
				RECT rc; GetClientRect(hwndCanvas, &rc);
				FillRect(hdcCanvas, &rc, (HBRUSH)GetSysColorBrush(COLOR_BTNFACE));
			}

			if (JabberAlphaBlend && (bm.bmBitsPixel == 32))
			{
				BLENDFUNCTION bf = {0};
				bf.AlphaFormat = AC_SRC_ALPHA;
				bf.BlendOp = AC_SRC_OVER;
				bf.SourceConstantAlpha = 255;
				JabberAlphaBlend( hdcCanvas, pt.x, pt.y, ptFitSize.x, ptFitSize.y, hdcMem, ptOrg.x, ptOrg.y, ptSize.x, ptSize.y, bf );
			} else
			{
				SetStretchBltMode( hdcCanvas, COLORONCOLOR );
				StretchBlt( hdcCanvas, pt.x, pt.y, ptFitSize.x, ptFitSize.y, hdcMem, ptOrg.x, ptOrg.y, ptSize.x, ptSize.y, SRCCOPY );
			}

			DeleteDC( hdcMem );
		}
		break;
	case WM_DESTROY:
		if ( hBitmap ) {
			ppro->JabberLog( "Delete bitmap" );
			DeleteObject( hBitmap );
			DeleteFileA( szPhotoFileName );
			szPhotoFileName[0] = '\0';
		}
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////

static BOOL CALLBACK NoteDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg );
		SendMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
		SetWindowLong( hwndDlg, GWL_USERDATA, lParam );
		return TRUE;
	case WM_JABBER_REFRESH:
	{
		CJabberProto* ppro = ( CJabberProto* )GetWindowLong( hwndDlg, GWL_USERDATA );
		SetDialogField( ppro, hwndDlg, IDC_DESC, "About" );
		break;
	}
	case WM_COMMAND:
		if (( HWND )lParam==GetFocus() && HIWORD( wParam )==EN_CHANGE )
			SendMessage( GetParent( hwndDlg ), WM_JABBER_CHANGED, 0, 0 );
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct EditDlgParam
{
	int id;
	CJabberProto* ppro;
};

static BOOL CALLBACK EditEmailDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	EditDlgParam* dat = ( EditDlgParam* )GetWindowLong( hwndDlg, GWL_USERDATA );

	switch ( msg ) {
	case WM_INITDIALOG:
		{
			EditDlgParam* dat = ( EditDlgParam* )lParam;
			SetWindowLong( hwndDlg, GWL_USERDATA, lParam );

			TranslateDialogDefault( hwndDlg );

			if ( lParam >= 0 ) {
				DBVARIANT dbv;
				char idstr[33];
				WORD nFlag;

				SetWindowText( hwndDlg, TranslateT( "Jabber vCard: Edit Email Address" ));
				wsprintfA( idstr, "e-mail%d", lParam );
				if ( !DBGetContactSettingString( NULL, dat->ppro->szProtoName, idstr, &dbv )) {
					SetDlgItemTextA( hwndDlg, IDC_EMAIL, dbv.pszVal );
					JFreeVariant( &dbv );
					wsprintfA( idstr, "e-mailFlag%d", lParam );
					nFlag = DBGetContactSettingWord( NULL, dat->ppro->szProtoName, idstr, 0 );
					if ( nFlag & JABBER_VCEMAIL_HOME ) CheckDlgButton( hwndDlg, IDC_HOME, TRUE );
					if ( nFlag & JABBER_VCEMAIL_WORK ) CheckDlgButton( hwndDlg, IDC_WORK, TRUE );
					if ( nFlag & JABBER_VCEMAIL_INTERNET ) CheckDlgButton( hwndDlg, IDC_INTERNET, TRUE );
					if ( nFlag & JABBER_VCEMAIL_X400 ) CheckDlgButton( hwndDlg, IDC_X400, TRUE );
		}	}	}
		return TRUE;

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDOK:
			{
				TCHAR text[128];
				char idstr[33];
				DBVARIANT dbv;
				WORD nFlag;

				if ( dat->id < 0 ) {
					for ( dat->id=0;;dat->id++ ) {
						mir_snprintf( idstr, SIZEOF(idstr), "e-mail%d", dat->id );
						if ( DBGetContactSettingString( NULL, dat->ppro->szProtoName, idstr, &dbv )) break;
						JFreeVariant( &dbv );
				}	}
				GetDlgItemText( hwndDlg, IDC_EMAIL, text, SIZEOF( text ));
				mir_snprintf( idstr, SIZEOF(idstr), "e-mail%d", dat->id );
				dat->ppro->JSetStringT( NULL, idstr, text );

				nFlag = 0;
				if ( IsDlgButtonChecked( hwndDlg, IDC_HOME )) nFlag |= JABBER_VCEMAIL_HOME;
				if ( IsDlgButtonChecked( hwndDlg, IDC_WORK )) nFlag |= JABBER_VCEMAIL_WORK;
				if ( IsDlgButtonChecked( hwndDlg, IDC_INTERNET )) nFlag |= JABBER_VCEMAIL_INTERNET;
				if ( IsDlgButtonChecked( hwndDlg, IDC_X400 )) nFlag |= JABBER_VCEMAIL_X400;
				mir_snprintf( idstr, SIZEOF(idstr), "e-mailFlag%d", dat->id );
				dat->ppro->JSetWord( NULL, idstr, nFlag );
			}
			// fall through
		case IDCANCEL:
			EndDialog( hwndDlg, wParam );
			break;
		}
	}
	return FALSE;
}

static BOOL CALLBACK EditPhoneDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	EditDlgParam* dat = ( EditDlgParam* )GetWindowLong( hwndDlg, GWL_USERDATA );

	switch ( msg ) {
	case WM_INITDIALOG:
		{
			EditDlgParam* dat = ( EditDlgParam* )lParam;
			SetWindowLong( hwndDlg, GWL_USERDATA, lParam );

			TranslateDialogDefault( hwndDlg );
			if ( dat->id >= 0 ) {
				DBVARIANT dbv;
				char idstr[33];
				WORD nFlag;

				SetWindowText( hwndDlg, TranslateT( "Jabber vCard: Edit Phone Number" ));
				wsprintfA( idstr, "Phone%d", dat->id );
				if ( !DBGetContactSettingString( NULL, dat->ppro->szProtoName, idstr, &dbv )) {
					SetDlgItemTextA( hwndDlg, IDC_PHONE, dbv.pszVal );
					JFreeVariant( &dbv );
					wsprintfA( idstr, "PhoneFlag%d", dat->id );
					nFlag = dat->ppro->JGetWord( NULL, idstr, 0 );
					if ( nFlag & JABBER_VCTEL_HOME ) CheckDlgButton( hwndDlg, IDC_HOME, TRUE );
					if ( nFlag & JABBER_VCTEL_WORK ) CheckDlgButton( hwndDlg, IDC_WORK, TRUE );
					if ( nFlag & JABBER_VCTEL_VOICE ) CheckDlgButton( hwndDlg, IDC_VOICE, TRUE );
					if ( nFlag & JABBER_VCTEL_FAX ) CheckDlgButton( hwndDlg, IDC_FAX, TRUE );
					if ( nFlag & JABBER_VCTEL_PAGER ) CheckDlgButton( hwndDlg, IDC_PAGER, TRUE );
					if ( nFlag & JABBER_VCTEL_MSG ) CheckDlgButton( hwndDlg, IDC_MSG, TRUE );
					if ( nFlag & JABBER_VCTEL_CELL ) CheckDlgButton( hwndDlg, IDC_CELL, TRUE );
					if ( nFlag & JABBER_VCTEL_VIDEO ) CheckDlgButton( hwndDlg, IDC_VIDEO, TRUE );
					if ( nFlag & JABBER_VCTEL_BBS ) CheckDlgButton( hwndDlg, IDC_BBS, TRUE );
					if ( nFlag & JABBER_VCTEL_MODEM ) CheckDlgButton( hwndDlg, IDC_MODEM, TRUE );
					if ( nFlag & JABBER_VCTEL_ISDN ) CheckDlgButton( hwndDlg, IDC_ISDN, TRUE );
					if ( nFlag & JABBER_VCTEL_PCS ) CheckDlgButton( hwndDlg, IDC_PCS, TRUE );
		}	}	}
		return TRUE;

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDOK:
			{
				char text[128];
				char idstr[33];
				DBVARIANT dbv;
				WORD nFlag;

				if ( dat->id < 0 ) {
					for ( dat->id=0;;dat->id++ ) {
						wsprintfA( idstr, "Phone%d", dat->id );
						if ( DBGetContactSettingString( NULL, dat->ppro->szProtoName, idstr, &dbv )) break;
						JFreeVariant( &dbv );
					}
				}
				GetDlgItemTextA( hwndDlg, IDC_PHONE, text, SIZEOF( text ));
				wsprintfA( idstr, "Phone%d", dat->id );
				dat->ppro->JSetString( NULL, idstr, text );
				nFlag = 0;
				if ( IsDlgButtonChecked( hwndDlg, IDC_HOME )) nFlag |= JABBER_VCTEL_HOME;
				if ( IsDlgButtonChecked( hwndDlg, IDC_WORK )) nFlag |= JABBER_VCTEL_WORK;
				if ( IsDlgButtonChecked( hwndDlg, IDC_VOICE )) nFlag |= JABBER_VCTEL_VOICE;
				if ( IsDlgButtonChecked( hwndDlg, IDC_FAX )) nFlag |= JABBER_VCTEL_FAX;
				if ( IsDlgButtonChecked( hwndDlg, IDC_PAGER )) nFlag |= JABBER_VCTEL_PAGER;
				if ( IsDlgButtonChecked( hwndDlg, IDC_MSG )) nFlag |= JABBER_VCTEL_MSG;
				if ( IsDlgButtonChecked( hwndDlg, IDC_CELL )) nFlag |= JABBER_VCTEL_CELL;
				if ( IsDlgButtonChecked( hwndDlg, IDC_VIDEO )) nFlag |= JABBER_VCTEL_VIDEO;
				if ( IsDlgButtonChecked( hwndDlg, IDC_BBS )) nFlag |= JABBER_VCTEL_BBS;
				if ( IsDlgButtonChecked( hwndDlg, IDC_MODEM )) nFlag |= JABBER_VCTEL_MODEM;
				if ( IsDlgButtonChecked( hwndDlg, IDC_ISDN )) nFlag |= JABBER_VCTEL_ISDN;
				if ( IsDlgButtonChecked( hwndDlg, IDC_PCS )) nFlag |= JABBER_VCTEL_PCS;
				wsprintfA( idstr, "PhoneFlag%d", dat->id );
				dat->ppro->JSetWord( NULL, idstr, nFlag );
			}
			// fall through
		case IDCANCEL:
			EndDialog( hwndDlg, wParam );
			break;
		}
	}
	return FALSE;
}

#define M_REMAKELISTS  ( WM_USER+1 )
static BOOL CALLBACK ContactDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	CJabberProto* ppro = ( CJabberProto* )GetWindowLong( hwndDlg, GWL_USERDATA );

	switch( msg ) {
	case WM_INITDIALOG:
		{
			LVCOLUMN lvc;
			RECT rc;

			SetWindowLong( hwndDlg, GWL_USERDATA, lParam );

			TranslateDialogDefault( hwndDlg );
			GetClientRect( GetDlgItem( hwndDlg,IDC_EMAILS ), &rc );
			rc.right -= GetSystemMetrics( SM_CXVSCROLL );
			lvc.mask = LVCF_WIDTH;
			lvc.cx = 30;
			ListView_InsertColumn( GetDlgItem( hwndDlg,IDC_EMAILS ), 0, &lvc );
			ListView_InsertColumn( GetDlgItem( hwndDlg,IDC_PHONES ), 0, &lvc );
			lvc.cx = rc.right - 30 - 40;
			ListView_InsertColumn( GetDlgItem( hwndDlg,IDC_EMAILS ), 1, &lvc );
			ListView_InsertColumn( GetDlgItem( hwndDlg,IDC_PHONES ), 1, &lvc );
			lvc.cx = 20;
			ListView_InsertColumn( GetDlgItem( hwndDlg,IDC_EMAILS ), 2, &lvc );
			ListView_InsertColumn( GetDlgItem( hwndDlg,IDC_EMAILS ), 3, &lvc );
			ListView_InsertColumn( GetDlgItem( hwndDlg,IDC_PHONES ), 2, &lvc );
			ListView_InsertColumn( GetDlgItem( hwndDlg,IDC_PHONES ), 3, &lvc );
			SendMessage( hwndDlg, M_REMAKELISTS, 0, 0 );
		}
		return TRUE;
	case M_REMAKELISTS:
		{
			LVITEM lvi;
			int i;
			char idstr[33];
			TCHAR number[20];
			DBVARIANT dbv;

			//e-mails
			ListView_DeleteAllItems( GetDlgItem( hwndDlg, IDC_EMAILS ));
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iSubItem = 0;
			lvi.iItem = 0;
			for ( i=0;;i++ ) {
				wsprintfA( idstr, "e-mail%d", i );
				if ( DBGetContactSettingTString( NULL, ppro->szProtoName, idstr, &dbv )) break;
				wsprintf( number, _T("%d"), i+1 );
				lvi.pszText = number;
				lvi.lParam = ( LPARAM )i;
				ListView_InsertItem( GetDlgItem( hwndDlg, IDC_EMAILS ), &lvi );
				ListView_SetItemText( GetDlgItem( hwndDlg, IDC_EMAILS ), lvi.iItem, 1, dbv.ptszVal );
				JFreeVariant( &dbv );
				lvi.iItem++;
			}
			lvi.mask = LVIF_PARAM;
			lvi.lParam = ( LPARAM )( -1 );
			ListView_InsertItem( GetDlgItem( hwndDlg, IDC_EMAILS ), &lvi );
			//phones
			ListView_DeleteAllItems( GetDlgItem( hwndDlg, IDC_PHONES ));
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iSubItem = 0;
			lvi.iItem = 0;
			for ( i=0;;i++ ) {
				wsprintfA( idstr, "Phone%d", i );
				if ( DBGetContactSettingTString( NULL, ppro->szProtoName, idstr, &dbv )) break;
				wsprintf( number, _T("%d"), i+1 );
				lvi.pszText = number;
				lvi.lParam = ( LPARAM )i;
				ListView_InsertItem( GetDlgItem( hwndDlg, IDC_PHONES ), &lvi );
				ListView_SetItemText( GetDlgItem( hwndDlg, IDC_PHONES ), lvi.iItem, 1, dbv.ptszVal );
				JFreeVariant( &dbv );
				lvi.iItem++;
			}
			lvi.mask = LVIF_PARAM;
			lvi.lParam = ( LPARAM )( -1 );
			ListView_InsertItem( GetDlgItem( hwndDlg, IDC_PHONES ), &lvi );
		}
		break;
	case WM_NOTIFY:
		switch (( ( LPNMHDR )lParam )->idFrom ) {
		case IDC_EMAILS:
		case IDC_PHONES:
			switch (( ( LPNMHDR )lParam )->code ) {
			case NM_CUSTOMDRAW:
				{
					NMLVCUSTOMDRAW *nm = ( NMLVCUSTOMDRAW * ) lParam;

					switch ( nm->nmcd.dwDrawStage ) {
					case CDDS_PREPAINT:
					case CDDS_ITEMPREPAINT:
						SetWindowLong( hwndDlg, DWL_MSGRESULT, CDRF_NOTIFYSUBITEMDRAW );
						return TRUE;
					case CDDS_SUBITEM|CDDS_ITEMPREPAINT:
						{
							RECT rc;
							HICON hIcon;

							ListView_GetSubItemRect( nm->nmcd.hdr.hwndFrom, nm->nmcd.dwItemSpec, nm->iSubItem, LVIR_LABEL, &rc );
							if( nm->nmcd.lItemlParam==( LPARAM )( -1 ) && nm->iSubItem==3 )
								hIcon = ( HICON )LoadImage( hInst, MAKEINTRESOURCE( IDI_ADDCONTACT ), IMAGE_ICON, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );
							else if ( nm->iSubItem==2 && nm->nmcd.lItemlParam!=( LPARAM )( -1 ))
								hIcon = ( HICON )LoadImage( hInst, MAKEINTRESOURCE( IDI_EDIT ), IMAGE_ICON, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );
							else if ( nm->iSubItem==3 && nm->nmcd.lItemlParam!=( LPARAM )( -1 ))
								hIcon = ( HICON )LoadImage( hInst, MAKEINTRESOURCE( IDI_DELETE ), IMAGE_ICON, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );
							else break;
							DrawIconEx( nm->nmcd.hdc, ( rc.left+rc.right-GetSystemMetrics( SM_CXSMICON ))/2, ( rc.top+rc.bottom-GetSystemMetrics( SM_CYSMICON ))/2,hIcon, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0, NULL, DI_NORMAL );
							DestroyIcon( hIcon );
							SetWindowLong( hwndDlg, DWL_MSGRESULT, CDRF_SKIPDEFAULT );
						}
						return TRUE;
					}
				}
				break;
			case NM_CLICK:
				{
					NMLISTVIEW *nm = ( NMLISTVIEW * ) lParam;
					LVITEM lvi;
					const char* szIdTemplate = nm->hdr.idFrom==IDC_PHONES?"Phone%d":"e-mail%d";
					const char* szFlagTemplate = nm->hdr.idFrom==IDC_PHONES?"PhoneFlag%d":"e-mailFlag%d";
					LVHITTESTINFO hti;

					if ( nm->iSubItem < 2 ) break;
					hti.pt.x = ( short ) LOWORD( GetMessagePos());
					hti.pt.y = ( short ) HIWORD( GetMessagePos());
					ScreenToClient( nm->hdr.hwndFrom, &hti.pt );
					if ( ListView_SubItemHitTest( nm->hdr.hwndFrom, &hti ) == -1 ) break;
					lvi.mask = LVIF_PARAM;
					lvi.iItem = hti.iItem;
					lvi.iSubItem = 0;
					ListView_GetItem( nm->hdr.hwndFrom, &lvi );
					if ( lvi.lParam == ( LPARAM )( -1 )) {
						if ( hti.iSubItem == 3 ) {
							//add
							if ( DialogBoxParam( hInst, MAKEINTRESOURCE( nm->hdr.idFrom==IDC_PHONES?IDD_VCARD_ADDPHONE:IDD_VCARD_ADDEMAIL ), hwndDlg, nm->hdr.idFrom==IDC_PHONES?EditPhoneDlgProc:EditEmailDlgProc, ( LPARAM )( -1 )) != IDOK )
								break;
							SendMessage( hwndDlg, M_REMAKELISTS, 0, 0 );
							SendMessage( GetParent( hwndDlg ), WM_JABBER_CHANGED, 0, 0 );
						}
					}
					else {
						if ( hti.iSubItem == 3 ) {
							//delete
							int i;
							char idstr[33];
							DBVARIANT dbv;

							for( i=lvi.lParam;;i++ ) {
								WORD nFlag;

								wsprintfA( idstr, szIdTemplate, i+1 );
								if ( DBGetContactSettingString( NULL, ppro->szProtoName, idstr, &dbv )) break;
								wsprintfA( idstr,szIdTemplate,i );
								ppro->JSetString( NULL, idstr, dbv.pszVal );
								wsprintfA( idstr, szFlagTemplate, i+1 );
								JFreeVariant( &dbv );
								nFlag = ppro->JGetWord( NULL, idstr, 0 );
								wsprintfA( idstr, szFlagTemplate, i );
								ppro->JSetWord( NULL, idstr, nFlag );
							}
							wsprintfA( idstr, szIdTemplate, i );
							ppro->JDeleteSetting( NULL, idstr );
							wsprintfA( idstr, szFlagTemplate, i );
							ppro->JDeleteSetting( NULL, idstr );
							SendMessage( hwndDlg, M_REMAKELISTS, 0, 0 );
							SendMessage( GetParent( hwndDlg ), WM_JABBER_CHANGED, 0, 0 );
						}
						else if ( hti.iSubItem == 2 ) {
							//edit
							if ( DialogBoxParam( hInst, MAKEINTRESOURCE( nm->hdr.idFrom==IDC_PHONES?IDD_VCARD_ADDPHONE:IDD_VCARD_ADDEMAIL ), hwndDlg, nm->hdr.idFrom==IDC_PHONES?EditPhoneDlgProc:EditEmailDlgProc, lvi.lParam ) != IDOK )
								break;
							SendMessage( hwndDlg,M_REMAKELISTS,0,0 );
							SendMessage( GetParent( hwndDlg ), WM_JABBER_CHANGED, 0, 0 );
						}
					}
				}
				break;
			}
			break;
		}
		break;
	case WM_SETCURSOR:
		if ( LOWORD( lParam ) != HTCLIENT ) break;
		if ( GetForegroundWindow() == GetParent( hwndDlg )) {
			POINT pt;
			GetCursorPos( &pt );
			ScreenToClient( hwndDlg,&pt );
			SetFocus( ChildWindowFromPoint( hwndDlg,pt ));	  //ugly hack because listviews ignore their first click
		}
		break;
	}
	return FALSE;
}

void CJabberProto::SaveVcardToDB( VcardTab *dat )
{
	HWND hwndPage;
	TCHAR text[2048];

	if ( dat==NULL || dat->page==NULL ) return;

	// Page 0: Personal
	if (( hwndPage=dat->page[0].hwnd ) != NULL ) {
		GetDlgItemText( hwndPage, IDC_FULLNAME, text, SIZEOF( text ));
		JSetStringT( NULL, "FullName", text );
		GetDlgItemText( hwndPage, IDC_NICKNAME, text, SIZEOF( text ));
		JSetStringT( NULL, "Nick", text );
		GetDlgItemText( hwndPage, IDC_FIRSTNAME, text, SIZEOF( text ));
		JSetStringT( NULL, "FirstName", text );
		GetDlgItemText( hwndPage, IDC_MIDDLE, text, SIZEOF( text ));
		JSetStringT( NULL, "MiddleName", text );
		GetDlgItemText( hwndPage, IDC_LASTNAME, text, SIZEOF( text ));
		JSetStringT( NULL, "LastName", text );
		GetDlgItemText( hwndPage, IDC_BIRTH, text, SIZEOF( text ));
		JSetStringT( NULL, "BirthDate", text );
		GetDlgItemText( hwndPage, IDC_GENDER, text, SIZEOF( text ));
		JSetStringT( NULL, "GenderString", text );
		GetDlgItemText( hwndPage, IDC_OCCUPATION, text, SIZEOF( text ));
		JSetStringT( NULL, "Role", text );
		GetDlgItemText( hwndPage, IDC_HOMEPAGE, text, SIZEOF( text ));
		JSetStringT( NULL, "Homepage", text );
	}
	// Page 1: Contacts
	//		no need to save, always in sync with the DB
	// Page 2: Home
	if (( hwndPage=dat->page[2].hwnd ) != NULL ) {
		GetDlgItemText( hwndPage, IDC_ADDRESS1, text, SIZEOF( text ));
		JSetStringT( NULL, "Street", text );
		GetDlgItemText( hwndPage, IDC_ADDRESS2, text, SIZEOF( text ));
		JSetStringT( NULL, "Street2", text );
		GetDlgItemText( hwndPage, IDC_CITY, text, SIZEOF( text ));
		JSetStringT( NULL, "City", text );
		GetDlgItemText( hwndPage, IDC_STATE, text, SIZEOF( text ));
		JSetStringT( NULL, "State", text );
		GetDlgItemText( hwndPage, IDC_ZIP, text, SIZEOF( text ));
		JSetStringT( NULL, "ZIP", text );
		GetDlgItemText( hwndPage, IDC_COUNTRY, text, SIZEOF( text ));
		JSetStringT( NULL, "CountryName", text );
	}
	// Page 3: Work
	if (( hwndPage=dat->page[3].hwnd ) != NULL ) {
		GetDlgItemText( hwndPage, IDC_COMPANY, text, SIZEOF( text ));
		JSetStringT( NULL, "Company", text );
		GetDlgItemText( hwndPage, IDC_DEPARTMENT, text, SIZEOF( text ));
		JSetStringT( NULL, "CompanyDepartment", text );
		GetDlgItemText( hwndPage, IDC_TITLE, text, SIZEOF( text ));
		JSetStringT( NULL, "CompanyPosition", text );
		GetDlgItemText( hwndPage, IDC_ADDRESS1, text, SIZEOF( text ));
		JSetStringT( NULL, "CompanyStreet", text );
		GetDlgItemText( hwndPage, IDC_ADDRESS2, text, SIZEOF( text ));
		JSetStringT( NULL, "CompanyStreet2", text );
		GetDlgItemText( hwndPage, IDC_CITY, text, SIZEOF( text ));
		JSetStringT( NULL, "CompanyCity", text );
		GetDlgItemText( hwndPage, IDC_STATE, text, SIZEOF( text ));
		JSetStringT( NULL, "CompanyState", text );
		GetDlgItemText( hwndPage, IDC_ZIP, text, SIZEOF( text ));
		JSetStringT( NULL, "CompanyZIP", text );
		GetDlgItemText( hwndPage, IDC_COUNTRY, text, SIZEOF( text ));
		JSetStringT( NULL, "CompanyCountryName", text );
	}
	// Page 4: Photo
	//		not saved in the database
	// Page 5: Note
	if (( hwndPage=dat->page[5].hwnd ) != NULL ) {
		GetDlgItemText( hwndPage, IDC_DESC, text, SIZEOF( text ));
		JSetStringT( NULL, "About", text );
	}
}

void CJabberProto::AppendVcardFromDB( XmlNode* n, char* tag, char* key )
{
	if ( n == NULL || tag == NULL || key == NULL )
		return;

	DBVARIANT dbv;
	if ( DBGetContactSettingTString( NULL, szProtoName, key, &dbv ))
		n->addChild( tag );
	else {
		n->addChild( tag, dbv.ptszVal );
		JFreeVariant( &dbv );
}	}

void CJabberProto::SetServerVcard()
{
	DBVARIANT dbv;
	int  iqId;
	char *szFileName;
	int  i;
	char idstr[33];
	WORD nFlag;

	iqId = JabberSerialNext();
	JabberIqAdd( iqId, IQ_PROC_SETVCARD, &CJabberProto::JabberIqResultSetVcard );

	XmlNodeIq iq( "set", iqId );
	XmlNode* v = iq.addChild( "vCard" ); v->addAttr( "xmlns", JABBER_FEAT_VCARD_TEMP );

	AppendVcardFromDB( v, "FN", "FullName" );

	XmlNode* n = v->addChild( "N" );
	AppendVcardFromDB( n, "GIVEN", "FirstName" );
	AppendVcardFromDB( n, "MIDDLE", "MiddleName" );
	AppendVcardFromDB( n, "FAMILY", "LastName" );

	AppendVcardFromDB( v, "NICKNAME", "Nick" );
	AppendVcardFromDB( v, "BDAY", "BirthDate" );
	AppendVcardFromDB( v, "GENDER", "GenderString" );

	for ( i=0;;i++ ) {
		wsprintfA( idstr, "e-mail%d", i );
		if ( DBGetContactSettingTString( NULL, szProtoName, idstr, &dbv )) 
			break;

		XmlNode* e = v->addChild( "EMAIL", dbv.ptszVal );
		JFreeVariant( &dbv );
		AppendVcardFromDB( e, "USERID", idstr );

		wsprintfA( idstr, "e-mailFlag%d", i );
		nFlag = DBGetContactSettingWord( NULL, szProtoName, idstr, 0 );
		if ( nFlag & JABBER_VCEMAIL_HOME ) e->addChild( "HOME" );
		if ( nFlag & JABBER_VCEMAIL_WORK ) e->addChild( "WORK" );
		if ( nFlag & JABBER_VCEMAIL_INTERNET ) e->addChild( "INTERNET" );
		if ( nFlag & JABBER_VCEMAIL_X400 ) e->addChild( "X400" );
	}

	n = v->addChild( "ADR" );
	n->addChild( "HOME" );
	AppendVcardFromDB( n, "STREET", "Street" );
	AppendVcardFromDB( n, "EXTADR", "Street2" );
	AppendVcardFromDB( n, "EXTADD", "Street2" );	// for compatibility with client using old vcard format
	AppendVcardFromDB( n, "LOCALITY", "City" );
	AppendVcardFromDB( n, "REGION", "State" );
	AppendVcardFromDB( n, "PCODE", "ZIP" );
	AppendVcardFromDB( n, "CTRY", "CountryName" );
	AppendVcardFromDB( n, "COUNTRY", "CountryName" );	// for compatibility with client using old vcard format

	n = v->addChild( "ADR" );
	n->addChild( "WORK" );
	AppendVcardFromDB( n, "STREET", "CompanyStreet" );
	AppendVcardFromDB( n, "EXTADR", "CompanyStreet2" );
	AppendVcardFromDB( n, "EXTADD", "CompanyStreet2" );	// for compatibility with client using old vcard format
	AppendVcardFromDB( n, "LOCALITY", "CompanyCity" );
	AppendVcardFromDB( n, "REGION", "CompanyState" );
	AppendVcardFromDB( n, "PCODE", "CompanyZIP" );
	AppendVcardFromDB( n, "CTRY", "CompanyCountryName" );
	AppendVcardFromDB( n, "COUNTRY", "CompanyCountryName" );	// for compatibility with client using old vcard format

	n = v->addChild( "ORG" );
	AppendVcardFromDB( n, "ORGNAME", "Company" );
	AppendVcardFromDB( n, "ORGUNIT", "CompanyDepartment" );

	AppendVcardFromDB( v, "TITLE", "CompanyPosition" );
	AppendVcardFromDB( v, "ROLE", "Role" );
	AppendVcardFromDB( v, "URL", "Homepage" );
	AppendVcardFromDB( v, "DESC", "About" );

	for ( i=0;;i++ ) {
		wsprintfA( idstr, "Phone%d", i );
		if ( DBGetContactSettingTString( NULL, szProtoName, idstr, &dbv )) break;
		JFreeVariant( &dbv );

		n = v->addChild( "TEL" );
		AppendVcardFromDB( n, "NUMBER", idstr );

		wsprintfA( idstr, "PhoneFlag%d", i );
		nFlag = JGetWord( NULL, idstr, 0 );
		if ( nFlag & JABBER_VCTEL_HOME )  n->addChild( "HOME" );
		if ( nFlag & JABBER_VCTEL_WORK )  n->addChild( "WORK" );
		if ( nFlag & JABBER_VCTEL_VOICE ) n->addChild( "VOICE" );
		if ( nFlag & JABBER_VCTEL_FAX )   n->addChild( "FAX" );
		if ( nFlag & JABBER_VCTEL_PAGER ) n->addChild( "PAGER" );
		if ( nFlag & JABBER_VCTEL_MSG )   n->addChild( "MSG" );
		if ( nFlag & JABBER_VCTEL_CELL )  n->addChild( "CELL" );
		if ( nFlag & JABBER_VCTEL_VIDEO ) n->addChild( "VIDEO" );
		if ( nFlag & JABBER_VCTEL_BBS )   n->addChild( "BBS" );
		if ( nFlag & JABBER_VCTEL_MODEM ) n->addChild( "MODEM" );
		if ( nFlag & JABBER_VCTEL_ISDN )  n->addChild( "ISDN" );
		if ( nFlag & JABBER_VCTEL_PCS )   n->addChild( "PCS" );
	}

	if ( bPhotoChanged )
		szFileName = ( szPhotoFileName[0] ) ? szPhotoFileName : NULL;
	else
		szFileName = jabberVcardPhotoFileName;

	// Set photo element, also update the global jabberVcardPhotoFileName to reflect the update
	JabberLog( "Before update, jabberVcardPhotoFileName = %s", jabberVcardPhotoFileName );
	if ( szFileName == NULL ) {
		v->addChild( "PHOTO" );
		if ( jabberVcardPhotoFileName ) {
			DeleteFileA( jabberVcardPhotoFileName );
			mir_free( jabberVcardPhotoFileName );
			jabberVcardPhotoFileName = NULL;
	}	}
	else {
		char szTempPath[MAX_PATH], szTempFileName[MAX_PATH];
		HANDLE hFile;
		struct _stat st;
		char* buffer, *str;
		DWORD nRead;

		JabberLog( "Saving picture from %s", szFileName );
		if ( _stat( szFileName, &st ) >= 0 ) {
			// Note the FILE_SHARE_READ attribute so that the CopyFile can succeed
			if (( hFile=CreateFileA( szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL )) != INVALID_HANDLE_VALUE ) {
				if (( buffer=( char* )mir_alloc( st.st_size )) != NULL ) {
					if ( ReadFile( hFile, buffer, st.st_size, &nRead, NULL )) {
						if (( str=JabberBase64Encode( buffer, nRead )) != NULL ) {
							n = v->addChild( "PHOTO" );
							char* szFileType;
							switch( JabberGetPictureType( buffer )) {
								case PA_FORMAT_PNG:  szFileType = "image/png";   break;
								case PA_FORMAT_GIF:  szFileType = "image/gif";   break;
								case PA_FORMAT_BMP:  szFileType = "image/bmp";   break;
								default:             szFileType = "image/jpeg";  break;
							}
							n->addChild( "TYPE", szFileType );

							n->addChild( "BINVAL", str );
							mir_free( str );

							// NEED TO UPDATE OUR AVATAR HASH:

							mir_sha1_byte_t digest[MIR_SHA1_HASH_SIZE];
							mir_sha1_ctx sha1ctx;
							mir_sha1_init( &sha1ctx );
							mir_sha1_append( &sha1ctx, (mir_sha1_byte_t*)buffer, nRead );
							mir_sha1_finish( &sha1ctx, digest );

							char buf[MIR_SHA1_HASH_SIZE*2+1];
							for ( int j=0; j<MIR_SHA1_HASH_SIZE; j++ )
								sprintf( buf+( j<<1 ), "%02x", digest[j] );

							JSetByte( "AvatarType", JabberGetPictureType( buffer ));	
							JSetString( NULL, "AvatarHash", buf );
							JSetString( NULL, "AvatarSaved", buf );

							if ( bPhotoChanged ) {
								if ( jabberVcardPhotoFileName ) {
									DeleteFileA( jabberVcardPhotoFileName );
									mir_free( jabberVcardPhotoFileName );
									jabberVcardPhotoFileName = NULL;
								}

								if ( GetTempPathA( sizeof( szTempPath ), szTempPath ) <= 0 )
									strcpy( szTempPath, ".\\" );
								if ( GetTempFileNameA( szTempPath, "jab", 0, szTempFileName ) > 0 ) {
									JabberLog( "New global file is %s", szTempFileName );
									if ( CopyFileA( szFileName, szTempFileName, FALSE ))
										jabberVcardPhotoFileName = mir_strdup( szTempFileName );
									else 
										DeleteFileA( szTempFileName );
					}	}	}	}
					mir_free( buffer );
				}
				CloseHandle( hFile );
	}	}	}

	jabberThreadInfo->send( iq );
}

void CJabberProto::JabberUpdateVCardPhoto( char * szFileName )
{
	bPhotoChanged=1;
	strncpy( szPhotoFileName,szFileName,MAX_PATH );
	SetServerVcard();
}

static void ThemeDialogBackground( HWND hwnd ) {
	if ( IsWinVerXPPlus()) {
		static HMODULE hThemeAPI = NULL;
		if ( !hThemeAPI ) hThemeAPI = GetModuleHandleA( "uxtheme" );
		if ( hThemeAPI ) {
			HRESULT ( STDAPICALLTYPE *MyEnableThemeDialogTexture )( HWND,DWORD ) = ( HRESULT ( STDAPICALLTYPE* )( HWND,DWORD ))GetProcAddress( hThemeAPI,"EnableThemeDialogTexture" );
			if ( MyEnableThemeDialogTexture )
				MyEnableThemeDialogTexture( hwnd,0x00000002|0x00000004 ); //0x00000002|0x00000004=ETDT_ENABLETAB
}	}	}

static BOOL CALLBACK JabberVcardDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	VcardTab* dat = ( VcardTab* ) GetWindowLong( hwndDlg, GWL_USERDATA );

	switch ( msg ) {
	case WM_INITDIALOG:
	{
		SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )dat->ppro->LoadIconEx( "vcard" ));
		TranslateDialogDefault( hwndDlg );

		dat = ( VcardTab * ) mir_alloc( sizeof( VcardTab ));
		memset( dat, 0, sizeof( VcardTab ));
		dat->ppro = ( CJabberProto* )lParam;
		dat->pageCount = 6;
		dat->currentPage = 0;
		dat->changed = FALSE;
		dat->updateAnimFrame = 0;
		dat->animating = FALSE;
		dat->page = ( VcardPage * ) mir_alloc( dat->pageCount * sizeof( VcardPage ));
		memset( dat->page, 0, dat->pageCount * sizeof( VcardPage ));

		EnableWindow( GetDlgItem( hwndDlg, IDC_UPDATE ), dat->ppro->jabberOnline );

		HWND hwndTabs = GetDlgItem( hwndDlg, IDC_TABS );

		TCITEM tci = { 0 };
		tci.mask = TCIF_TEXT;
		tci.lParam = lParam;
		// Page 0: Personal
		dat->page[0].dlgId = IDD_VCARD_PERSONAL;
		dat->page[0].dlgProc = PersonalDlgProc;
		tci.pszText = TranslateT( "Personal" );
		TabCtrl_InsertItem( hwndTabs, 0, &tci );
		// Page 1: Contacts
		dat->page[1].dlgId = IDD_VCARD_CONTACT;
		dat->page[1].dlgProc = ContactDlgProc;
		tci.pszText = TranslateT( "Contacts" );
		TabCtrl_InsertItem( hwndTabs, 1, &tci );
		// Page 2: Home
		dat->page[2].dlgId = IDD_VCARD_HOME;
		dat->page[2].dlgProc = HomeDlgProc;
		tci.pszText = TranslateT( "Home" );
		TabCtrl_InsertItem( hwndTabs, 2, &tci );
		// Page 3: Work
		dat->page[3].dlgId = IDD_VCARD_WORK;
		dat->page[3].dlgProc = WorkDlgProc;
		tci.pszText = TranslateT( "Work" );
		TabCtrl_InsertItem( hwndTabs, 3, &tci );
		// Page 4: Photo
		dat->page[4].dlgId = IDD_VCARD_PHOTO;
		dat->page[4].dlgProc = PhotoDlgProc;
		tci.pszText = TranslateT( "Photo" );
		TabCtrl_InsertItem( hwndTabs, 4, &tci );
		// Page 5: Note
		dat->page[5].dlgId = IDD_VCARD_NOTE;
		dat->page[5].dlgProc = NoteDlgProc;

		tci.pszText = TranslateT( "Note" );
		TabCtrl_InsertItem( hwndTabs, 5, &tci );

		GetWindowRect( hwndTabs, &( dat->rectTab ));
		TabCtrl_AdjustRect( hwndTabs, FALSE, &( dat->rectTab ));
		{	POINT pt = {0,0};
			ClientToScreen( hwndDlg, &pt );
			OffsetRect( &( dat->rectTab ), -pt.x, -pt.y );
		}

		TabCtrl_SetCurSel( hwndTabs, dat->currentPage );
		dat->page[dat->currentPage].hwnd = CreateDialogParam( hInst, MAKEINTRESOURCE( dat->page[dat->currentPage].dlgId ), hwndDlg, dat->page[dat->currentPage].dlgProc, 0 );
		ThemeDialogBackground( dat->page[dat->currentPage].hwnd );
		SetWindowPos( dat->page[dat->currentPage].hwnd, HWND_TOP, dat->rectTab.left, dat->rectTab.top, 0, 0, SWP_NOSIZE );
		ShowWindow( dat->page[dat->currentPage].hwnd, SW_SHOW );

		SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG ) dat );

		bPhotoChanged = FALSE;
		szPhotoFileName[0] = '\0';

		if ( dat->ppro->jabberOnline )
			SendMessage( hwndDlg, WM_COMMAND, IDC_UPDATE, 0 );
		return TRUE;
	}
	case WM_CTLCOLORSTATIC:
		switch (GetDlgCtrlID((HWND)lParam)) {
		case IDC_WHITERECT:
		case IDC_LOGO:
		case IDC_NAME:
		case IDC_DESCRIPTION:
			SetBkColor(( HDC ) wParam, RGB( 255, 255, 255 ));
			return ( BOOL ) GetStockObject( WHITE_BRUSH );

		case IDC_UPDATING:
			SetBkColor(( HDC )wParam, GetSysColor( COLOR_3DFACE ));
			return ( BOOL ) GetSysColorBrush( COLOR_3DFACE );
		}
		break;

	case WM_NOTIFY:
		switch ( wParam ) {
		case IDC_TABS:
			switch (( ( LPNMHDR ) lParam )->code ) {
			case TCN_SELCHANGE:
				if ( dat->currentPage>=0 && dat->page[dat->currentPage].hwnd!=NULL )
					ShowWindow( dat->page[dat->currentPage].hwnd, SW_HIDE );
				dat->currentPage = TabCtrl_GetCurSel( GetDlgItem( hwndDlg, IDC_TABS ));
				if ( dat->currentPage >= 0 ) {
					if ( dat->page[dat->currentPage].hwnd == NULL ) {
						dat->page[dat->currentPage].hwnd = CreateDialogParam( hInst, MAKEINTRESOURCE( dat->page[dat->currentPage].dlgId ), hwndDlg, dat->page[dat->currentPage].dlgProc, 0 );
						ThemeDialogBackground( dat->page[dat->currentPage].hwnd );
						SetWindowPos( dat->page[dat->currentPage].hwnd, HWND_TOP, dat->rectTab.left, dat->rectTab.top, 0, 0, SWP_NOSIZE );
					}
					ShowWindow( dat->page[dat->currentPage].hwnd, SW_SHOW );
				}
				break;
			}
			break;
		}
		break;
	case WM_JABBER_CHANGED:
		dat->changed = TRUE;
		EnableWindow( GetDlgItem( hwndDlg, IDC_SAVE ), dat->ppro->jabberOnline );
		break;
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_UPDATE:
			EnableWindow( GetDlgItem( hwndDlg,IDC_UPDATE ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg,IDC_SAVE ), FALSE );
			dat->szUpdating = TranslateT( "Updating" );
			SetDlgItemText( hwndDlg, IDC_UPDATING, dat->szUpdating );
			ShowWindow( GetDlgItem( hwndDlg, IDC_UPDATING ), SW_SHOW );
			dat->ppro->JabberSendGetVcard( dat->ppro->jabberJID );
			dat->animating = TRUE;
			SetTimer( hwndDlg, 1, 200, NULL );
			break;
		case IDC_SAVE:
			EnableWindow( GetDlgItem( hwndDlg,IDC_UPDATE ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg,IDC_SAVE ), FALSE );
			dat->szUpdating = TranslateT( "Saving" );
			SetDlgItemText( hwndDlg, IDC_UPDATING, dat->szUpdating );
			ShowWindow( GetDlgItem( hwndDlg, IDC_UPDATING ), SW_SHOW );
			dat->animating = TRUE;
			SetTimer( hwndDlg, 1, 200, NULL );
			dat->ppro->SaveVcardToDB( dat );
			dat->ppro->SetServerVcard();
			break;
		case IDCLOSE:
			DestroyWindow( hwndDlg );
			break;
		}
		break;
	case WM_TIMER:
		{	TCHAR str[128];
			mir_sntprintf( str, SIZEOF(str), _T("%.*s%s%.*s"), dat->updateAnimFrame%5, _T("...."), dat->szUpdating, dat->updateAnimFrame%5, _T("...."));
			SetDlgItemText( hwndDlg, IDC_UPDATING, str );
			if (( ++dat->updateAnimFrame ) >= 5 ) dat->updateAnimFrame = 0;
		}
		break;
	case WM_JABBER_CHECK_ONLINE:
		EnableWindow( GetDlgItem( hwndDlg, IDC_UPDATE ), dat->ppro->jabberOnline );
		if ( dat->changed )
			EnableWindow( GetDlgItem( hwndDlg, IDC_SAVE ), dat->ppro->jabberOnline );
		break;
	case WM_JABBER_REFRESH:
		if ( dat->animating ) {
			KillTimer( hwndDlg, 1 );
			dat->animating = FALSE;
			ShowWindow( GetDlgItem( hwndDlg, IDC_UPDATING ), FALSE );
		}
		{	for ( int i=0; i<dat->pageCount; i++ )
				if ( dat->page[i].hwnd != NULL )
					SendMessage( dat->page[i].hwnd, WM_JABBER_REFRESH, 0, 0 );
		}
		dat->changed = FALSE;
		EnableWindow( GetDlgItem( hwndDlg, IDC_UPDATE ), TRUE );
		EnableWindow( GetDlgItem( hwndDlg, IDC_SAVE ), FALSE );
		break;
	case WM_CLOSE:
		DestroyWindow( hwndDlg );
		break;
	case WM_DESTROY:
		dat->ppro->hwndJabberVcard = NULL;
		for ( int i=0; i<dat->pageCount; i++ )
			if ( dat->page[i].hwnd != NULL )
				DestroyWindow( dat->page[i].hwnd );

		if ( dat->page ) mir_free( dat->page );
		if ( dat ) mir_free( dat );
		break;
	}
	return FALSE;
}
