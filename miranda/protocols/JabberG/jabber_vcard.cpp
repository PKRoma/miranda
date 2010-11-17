/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-09  George Hazan

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

Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#include "jabber_iq.h"
#include "jabber_caps.h"

/////////////////////////////////////////////////////////////////////////////////////////

int CJabberProto::SendGetVcard( const TCHAR* jid )
{
	if (!m_bJabberOnline) return 0;

	int iqId = SerialNext();
	JABBER_IQ_PROCID procId = !lstrcmp( jid, m_szJabberJID ) ? IQ_PROC_GETVCARD : IQ_PROC_NONE;

	IqAdd( iqId, procId, &CJabberProto::OnIqResultGetVcard );
	m_ThreadInfo->send(
		XmlNodeIq( _T("get"), iqId, jid ) << XCHILDNS( _T("vCard"), _T(JABBER_FEAT_VCARD_TEMP))
			<< XATTR( _T("prodid"), _T("-//HandGen//NONSGML vGen v1.0//EN")) << XATTR( _T("version"), _T("2.0")));

	return iqId;
}

/////////////////////////////////////////////////////////////////////////////////////////

static void SetDialogField( CJabberProto* ppro, HWND hwndDlg, int nDlgItem, char* key, bool bTranslate = false )
{
	DBVARIANT dbv;

	if ( !DBGetContactSettingTString( NULL, ppro->m_szModuleName, key, &dbv )) {
		SetDlgItemText( hwndDlg, nDlgItem, ( bTranslate ) ? TranslateTS(dbv.ptszVal) : dbv.ptszVal );
		JFreeVariant( &dbv );
	}
	else SetDlgItemTextA( hwndDlg, nDlgItem, "" );
}

static INT_PTR CALLBACK PersonalDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	const unsigned long iPageId = 0;
	CJabberProto* ppro = ( CJabberProto* )GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

	switch ( msg ) {
	case WM_INITDIALOG:
		if (!lParam) break; // Launched from userinfo
		ppro = ( CJabberProto* )lParam;
		TranslateDialogDefault( hwndDlg );
		SendMessage( GetDlgItem( hwndDlg, IDC_GENDER ), CB_ADDSTRING, 0, ( LPARAM )TranslateT( "Male" ));
		SendMessage( GetDlgItem( hwndDlg, IDC_GENDER ), CB_ADDSTRING, 0, ( LPARAM )TranslateT( "Female" ));
		SetWindowLongPtr( hwndDlg, GWLP_USERDATA, lParam );
		SendMessage( hwndDlg, WM_JABBER_REFRESH_VCARD, 0, 0 );
		ppro->WindowSubscribe(hwndDlg);
		break;
	case WM_JABBER_REFRESH_VCARD:
	{
		SetDialogField( ppro, hwndDlg, IDC_FULLNAME, "FullName" );
		SetDialogField( ppro, hwndDlg, IDC_NICKNAME, "Nick" );
		SetDialogField( ppro, hwndDlg, IDC_FIRSTNAME, "FirstName" );
		SetDialogField( ppro, hwndDlg, IDC_MIDDLE, "MiddleName" );
		SetDialogField( ppro, hwndDlg, IDC_LASTNAME, "LastName" );
		SetDialogField( ppro, hwndDlg, IDC_BIRTH, "BirthDate" );
		SetDialogField( ppro, hwndDlg, IDC_GENDER, "GenderString", true );
		SetDialogField( ppro, hwndDlg, IDC_OCCUPATION, "Role" );
		SetDialogField( ppro, hwndDlg, IDC_HOMEPAGE, "Homepage" );
		break;
	}
	case WM_COMMAND:
		if (( ( HWND )lParam==GetFocus() && HIWORD( wParam )==EN_CHANGE ) ||
			(( HWND )lParam==GetDlgItem( hwndDlg, IDC_GENDER ) && ( HIWORD( wParam )==CBN_EDITCHANGE||HIWORD( wParam )==CBN_SELCHANGE )) )
		{
			ppro->m_vCardUpdates |= (1UL<<iPageId);
			SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
		}
		break;
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->idFrom == 0) {
			switch (((LPNMHDR)lParam)->code) {
			case PSN_PARAMCHANGED:
				SendMessage(hwndDlg, WM_INITDIALOG, 0, ((PSHNOTIFY*)lParam)->lParam);
				break;
			case PSN_APPLY:
				ppro->m_vCardUpdates &= ~(1UL<<iPageId);
				ppro->SaveVcardToDB( hwndDlg, iPageId );
				if (!ppro->m_vCardUpdates)
					ppro->SetServerVcard( ppro->m_bPhotoChanged, ppro->m_szPhotoFileName );
				break;
		}	}
		break;
	case WM_DESTROY:
		ppro->WindowUnsubscribe(hwndDlg);
		break;
	}
	return FALSE;
}

static INT_PTR CALLBACK HomeDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	const unsigned long iPageId = 1;
	CJabberProto* ppro = ( CJabberProto* )GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

	switch ( msg ) {
	case WM_INITDIALOG:
		if (!lParam) break; // Launched from userinfo
		ppro = ( CJabberProto* )lParam;
		TranslateDialogDefault( hwndDlg );
		SetWindowLongPtr( hwndDlg, GWLP_USERDATA, lParam );
		SendMessage( hwndDlg, WM_JABBER_REFRESH_VCARD, 0, 0 );
		ppro->WindowSubscribe(hwndDlg);
		break;
	case WM_JABBER_REFRESH_VCARD:
	{
		SetDialogField( ppro, hwndDlg, IDC_ADDRESS1, "Street" );
		SetDialogField( ppro, hwndDlg, IDC_ADDRESS2, "Street2" );
		SetDialogField( ppro, hwndDlg, IDC_CITY, "City" );
		SetDialogField( ppro, hwndDlg, IDC_STATE, "State" );
		SetDialogField( ppro, hwndDlg, IDC_ZIP, "ZIP" );
		SetDialogField( ppro, hwndDlg, IDC_COUNTRY, "Country" );
		break;
	}
	case WM_COMMAND:
		if (( HWND )lParam==GetFocus() && HIWORD( wParam )==EN_CHANGE )
		{
			ppro->m_vCardUpdates |= (1UL<<iPageId);
			SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
		}
		break;
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->idFrom == 0) {
			switch (((LPNMHDR)lParam)->code) {
			case PSN_PARAMCHANGED:
				SendMessage(hwndDlg, WM_INITDIALOG, 0, ((PSHNOTIFY*)lParam)->lParam);
				break;
			case PSN_APPLY:
				ppro->m_vCardUpdates &= ~(1UL<<iPageId);
				ppro->SaveVcardToDB( hwndDlg, iPageId );
				if (!ppro->m_vCardUpdates)
					ppro->SetServerVcard( ppro->m_bPhotoChanged, ppro->m_szPhotoFileName );
				break;
		}	}
		break;
	case WM_DESTROY:
		ppro->WindowUnsubscribe(hwndDlg);
		break;
	}
	return FALSE;
}

static INT_PTR CALLBACK WorkDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	const unsigned long iPageId = 2;
	CJabberProto* ppro = ( CJabberProto* )GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

	switch ( msg ) {
	case WM_INITDIALOG:
		if (!lParam) break; // Launched from userinfo
		ppro = ( CJabberProto* )lParam;
		TranslateDialogDefault( hwndDlg );
		SetWindowLongPtr( hwndDlg, GWLP_USERDATA, lParam );
		SendMessage( hwndDlg, WM_JABBER_REFRESH_VCARD, 0, 0 );
		ppro->WindowSubscribe(hwndDlg);
		break;
	case WM_JABBER_REFRESH_VCARD:
	{
		SetDialogField( ppro, hwndDlg, IDC_COMPANY, "Company" );
		SetDialogField( ppro, hwndDlg, IDC_DEPARTMENT, "CompanyDepartment" );
		SetDialogField( ppro, hwndDlg, IDC_TITLE, "CompanyPosition" );
		SetDialogField( ppro, hwndDlg, IDC_ADDRESS1, "CompanyStreet" );
		SetDialogField( ppro, hwndDlg, IDC_ADDRESS2, "CompanyStreet2" );
		SetDialogField( ppro, hwndDlg, IDC_CITY, "CompanyCity" );
		SetDialogField( ppro, hwndDlg, IDC_STATE, "CompanyState" );
		SetDialogField( ppro, hwndDlg, IDC_ZIP, "CompanyZIP" );
		SetDialogField( ppro, hwndDlg, IDC_COUNTRY, "CompanyCountry" );
		break;
	}
	case WM_COMMAND:
		if (( HWND )lParam==GetFocus() && HIWORD( wParam )==EN_CHANGE )
		{
			ppro->m_vCardUpdates |= (1UL<<iPageId);
			SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
		}
		break;
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->idFrom == 0) {
			switch (((LPNMHDR)lParam)->code) {
			case PSN_PARAMCHANGED:
				SendMessage(hwndDlg, WM_INITDIALOG, 0, ((PSHNOTIFY*)lParam)->lParam);
				break;
			case PSN_APPLY:
				ppro->m_vCardUpdates &= ~(1UL<<iPageId);
				ppro->SaveVcardToDB( hwndDlg, iPageId );
				if (!ppro->m_vCardUpdates)
					ppro->SetServerVcard( ppro->m_bPhotoChanged, ppro->m_szPhotoFileName );
				break;
		}	}
		break;
	case WM_DESTROY:
		ppro->WindowUnsubscribe(hwndDlg);
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////

struct PhotoDlgProcData
{
	CJabberProto* ppro;
//	char szPhotoFileName[MAX_PATH];
//	BOOL bPhotoChanged;
	HBITMAP hBitmap;
};

static INT_PTR CALLBACK PhotoDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	const unsigned long iPageId = 3;

	TCHAR szAvatarFileName[ MAX_PATH ], szTempPath[MAX_PATH], szTempFileName[MAX_PATH];
	PhotoDlgProcData* dat = ( PhotoDlgProcData* )GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

	switch ( msg ) {
	case WM_INITDIALOG:
		if (!lParam) break; // Launched from userinfo
		TranslateDialogDefault( hwndDlg );
		SendDlgItemMessage( hwndDlg, IDC_LOAD, BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadImage( hInst, MAKEINTRESOURCE( IDI_OPEN ), IMAGE_ICON, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 ));
		SendDlgItemMessage( hwndDlg, IDC_LOAD, BUTTONSETASFLATBTN, 0, 0);
		SendDlgItemMessage( hwndDlg, IDC_DELETE, BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadImage( hInst, MAKEINTRESOURCE( IDI_DELETE ), IMAGE_ICON, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 ));
		SendDlgItemMessage( hwndDlg, IDC_DELETE, BUTTONSETASFLATBTN, 0, 0);
		ShowWindow( GetDlgItem( hwndDlg, IDC_SAVE ), SW_HIDE );
		{
			dat = new PhotoDlgProcData;
			dat->ppro = ( CJabberProto* )lParam;
			dat->hBitmap = NULL;
			dat->ppro->m_bPhotoChanged = FALSE;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, ( LONG_PTR )dat );
			dat->ppro->WindowSubscribe(hwndDlg);
		}
		SendMessage( hwndDlg, WM_JABBER_REFRESH_VCARD, 0, 0 );
		break;

	case WM_JABBER_REFRESH_VCARD:
		if ( dat->hBitmap ) {
			DeleteObject( dat->hBitmap );
			dat->hBitmap = NULL;
			DeleteFile( dat->ppro->m_szPhotoFileName );
			dat->ppro->m_szPhotoFileName[0] = '\0';
		}
		EnableWindow( GetDlgItem( hwndDlg, IDC_DELETE ), FALSE );
		dat->ppro->GetAvatarFileName( NULL, szAvatarFileName, sizeof( szAvatarFileName ));
		if ( _taccess( szAvatarFileName, 0 ) == 0 ) {
			if ( GetTempPath( SIZEOF( szTempPath ), szTempPath ) <= 0 )
				_tcscpy( szTempPath, _T(".\\"));
			if ( GetTempFileName( szTempPath, _T("jab"), 0, szTempFileName ) > 0 ) {
				dat->ppro->Log( "Temp file = " TCHAR_STR_PARAM, szTempFileName );
				if ( CopyFile( szAvatarFileName, szTempFileName, FALSE ) == TRUE ) {
					char* p = mir_t2a( szTempFileName );
					if (( dat->hBitmap=( HBITMAP ) JCallService( MS_UTILS_LOADBITMAP, 0, ( LPARAM )p )) != NULL ) {
						JabberBitmapPremultiplyChannels( dat->hBitmap );
						_tcscpy( dat->ppro->m_szPhotoFileName, szTempFileName );
						EnableWindow( GetDlgItem( hwndDlg, IDC_DELETE ), TRUE );
					}
					else DeleteFile( szTempFileName );
					mir_free(p);
				}
				else DeleteFile( szTempFileName );
		}	}

		dat->ppro->m_bPhotoChanged = FALSE;
		InvalidateRect( hwndDlg, NULL, TRUE );
		UpdateWindow( hwndDlg );
		break;

	case WM_JABBER_CHANGED:
		dat->ppro->SetServerVcard( dat->ppro->m_bPhotoChanged, dat->ppro->m_szPhotoFileName );
		break;

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_LOAD:
			{
				TCHAR szFilter[512];
				union {
					TCHAR szFileName[MAX_PATH];
					char  boo[512];
				};

				#if defined( _UNICODE )
					JCallService( MS_UTILS_GETBITMAPFILTERSTRINGS, sizeof( boo ), ( LPARAM )boo );
					memset( szFilter, 0, sizeof( szFilter ));
					MultiByteToWideChar( CP_ACP, 0, boo, -1, szFilter, SIZEOF(szFilter));
				#else
					JCallService( MS_UTILS_GETBITMAPFILTERSTRINGS, sizeof( szFilter ), ( LPARAM )szFilter );
				#endif

				OPENFILENAME ofn = {0};
				ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
				ofn.hwndOwner = hwndDlg;
				ofn.lpstrFilter = szFilter;
				ofn.lpstrCustomFilter = NULL;
				ofn.lpstrFile = szFileName;
				ofn.nMaxFile = _MAX_PATH;
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_DONTADDTORECENT;
				szFileName[0] = '\0';
				if ( GetOpenFileName( &ofn )) {
					struct _stat st;
					HBITMAP hNewBitmap;

					dat->ppro->Log( "File selected is " TCHAR_STR_PARAM, szFileName );
					if ( _tstat( szFileName, &st )<0 || st.st_size>40*1024 ) {
						MessageBox( hwndDlg, TranslateT( "Only JPG, GIF, and BMP image files smaller than 40 KB are supported." ), TranslateT( "Jabber vCard" ), MB_OK|MB_SETFOREGROUND );
						break;
					}
					if ( GetTempPath( SIZEOF( szTempPath ), szTempPath ) <= 0 )
						_tcscpy( szTempPath, _T(".\\"));
					if ( GetTempFileName( szTempPath, _T("jab"), 0, szTempFileName ) > 0 ) {
						dat->ppro->Log( "Temp file = " TCHAR_STR_PARAM, szTempFileName );
						if ( CopyFile( szFileName, szTempFileName, FALSE ) == TRUE ) {
							char* pszTemp = mir_t2a( szTempFileName );
							if (( hNewBitmap=( HBITMAP ) JCallService( MS_UTILS_LOADBITMAP, 0, ( LPARAM )pszTemp )) != NULL ) {
								if ( dat->hBitmap ) {
									DeleteObject( dat->hBitmap );
									DeleteFile( dat->ppro->m_szPhotoFileName );
								}

								dat->hBitmap = hNewBitmap;
								_tcscpy( dat->ppro->m_szPhotoFileName, szTempFileName );
								dat->ppro->m_bPhotoChanged = TRUE;
								EnableWindow( GetDlgItem( hwndDlg, IDC_DELETE ), TRUE );
								InvalidateRect( hwndDlg, NULL, TRUE );
								UpdateWindow( hwndDlg );
								dat->ppro->m_vCardUpdates |= (1UL<<iPageId);
								SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
							}
							else DeleteFile( szTempFileName );

							mir_free( pszTemp );
						}
						else DeleteFile( szTempFileName );
					}
				}
			}
			break;
		case IDC_DELETE:
			if ( dat->hBitmap ) {
				DeleteObject( dat->hBitmap );
				dat->hBitmap = NULL;
				DeleteFile( dat->ppro->m_szPhotoFileName );
				dat->ppro->m_szPhotoFileName[0] = '\0';
				dat->ppro->m_bPhotoChanged = TRUE;
				EnableWindow( GetDlgItem( hwndDlg, IDC_DELETE ), FALSE );
				InvalidateRect( hwndDlg, NULL, TRUE );
				UpdateWindow( hwndDlg );
				dat->ppro->m_vCardUpdates |= (1UL<<iPageId);
				SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			}
			break;
		}
		break;
	case WM_PAINT:
		if ( dat->hBitmap ) {
			BITMAP bm;
			HDC hdcMem;
			HWND hwndCanvas;
			HDC hdcCanvas;
			POINT ptSize, ptOrg, pt, ptFitSize;
			RECT rect;

			hwndCanvas = GetDlgItem( hwndDlg, IDC_CANVAS );
			hdcCanvas = GetDC( hwndCanvas );
			hdcMem = CreateCompatibleDC( hdcCanvas );
			SelectObject( hdcMem, dat->hBitmap );
			SetMapMode( hdcMem, GetMapMode( hdcCanvas ));
			GetObject( dat->hBitmap, sizeof( BITMAP ), ( LPVOID ) &bm );
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

			RECT rc;
			GetClientRect(hwndCanvas, &rc);
			if (JabberIsThemeActive && JabberDrawThemeParentBackground && JabberIsThemeActive())
				JabberDrawThemeParentBackground(hwndCanvas, hdcCanvas, &rc);
			else
				FillRect(hdcCanvas, &rc, (HBRUSH)GetSysColorBrush(COLOR_BTNFACE));

			if (JabberAlphaBlend && (bm.bmBitsPixel == 32)) {
				BLENDFUNCTION bf = {0};
				bf.AlphaFormat = AC_SRC_ALPHA;
				bf.BlendOp = AC_SRC_OVER;
				bf.SourceConstantAlpha = 255;
				JabberAlphaBlend( hdcCanvas, pt.x, pt.y, ptFitSize.x, ptFitSize.y, hdcMem, ptOrg.x, ptOrg.y, ptSize.x, ptSize.y, bf );
			}
			else {
				SetStretchBltMode( hdcCanvas, COLORONCOLOR );
				StretchBlt( hdcCanvas, pt.x, pt.y, ptFitSize.x, ptFitSize.y, hdcMem, ptOrg.x, ptOrg.y, ptSize.x, ptSize.y, SRCCOPY );
			}

			DeleteDC( hdcMem );
		}
		break;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->idFrom == 0) {
			switch (((LPNMHDR)lParam)->code) {
			case PSN_PARAMCHANGED:
				SendMessage(hwndDlg, WM_INITDIALOG, 0, ((PSHNOTIFY*)lParam)->lParam);
				break;
			case PSN_APPLY:
				dat->ppro->m_vCardUpdates &= ~(1UL<<iPageId);
				dat->ppro->SaveVcardToDB( hwndDlg, iPageId );
				if (!dat->ppro->m_vCardUpdates)
					dat->ppro->SetServerVcard( dat->ppro->m_bPhotoChanged, dat->ppro->m_szPhotoFileName );
				break;
		}	}
		break;

	case WM_DESTROY:
		DestroyIcon(( HICON )SendDlgItemMessage( hwndDlg, IDC_LOAD, BM_SETIMAGE, IMAGE_ICON, 0 ));
		DestroyIcon(( HICON )SendDlgItemMessage( hwndDlg, IDC_DELETE, BM_SETIMAGE, IMAGE_ICON, 0 ));
		dat->ppro->WindowUnsubscribe(hwndDlg);
		if ( dat->hBitmap ) {
			dat->ppro->Log( "Delete bitmap" );
			DeleteObject( dat->hBitmap );
			DeleteFile( dat->ppro->m_szPhotoFileName );
		}
		delete dat;
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////

static INT_PTR CALLBACK NoteDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	const unsigned long iPageId = 4;
	CJabberProto* ppro = ( CJabberProto* )GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

	switch ( msg ) {
	case WM_INITDIALOG:
		if (!lParam) break; // Launched from userinfo
		ppro = ( CJabberProto* )lParam;
		TranslateDialogDefault( hwndDlg );
		SetWindowLongPtr( hwndDlg, GWLP_USERDATA, lParam );
		SendMessage( hwndDlg, WM_JABBER_REFRESH_VCARD, 0, 0 );
		ppro->WindowSubscribe(hwndDlg);
		break;
	case WM_JABBER_REFRESH_VCARD:
	{
		SetDialogField( ppro, hwndDlg, IDC_DESC, "About" );
		break;
	}
	case WM_COMMAND:
		if (( HWND )lParam==GetFocus() && HIWORD( wParam )==EN_CHANGE )
		{
			ppro->m_vCardUpdates |= (1UL<<iPageId);
			SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
		}
		break;
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->idFrom == 0) {
			switch (((LPNMHDR)lParam)->code) {
			case PSN_PARAMCHANGED:
				SendMessage(hwndDlg, WM_INITDIALOG, 0, ((PSHNOTIFY*)lParam)->lParam);
				break;
			case PSN_APPLY:
				ppro->m_vCardUpdates &= ~(1UL<<iPageId);
				ppro->SaveVcardToDB( hwndDlg, iPageId );
				if (!ppro->m_vCardUpdates)
					ppro->SetServerVcard( ppro->m_bPhotoChanged, ppro->m_szPhotoFileName );
				break;
		}	}
		break;
	case WM_DESTROY:
		ppro->WindowUnsubscribe(hwndDlg);
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

static INT_PTR CALLBACK EditEmailDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	EditDlgParam* dat = ( EditDlgParam* )GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

	switch ( msg ) {
	case WM_INITDIALOG:
		{
			EditDlgParam* dat = ( EditDlgParam* )lParam;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, lParam );

			TranslateDialogDefault( hwndDlg );

			if ( lParam >= 0 ) {
				DBVARIANT dbv;
				char idstr[33];
				WORD nFlag;

				SetWindowText( hwndDlg, TranslateT( "Jabber vCard: Edit Email Address" ));
				wsprintfA( idstr, "e-mail%d", dat->id );
				if ( !DBGetContactSettingString( NULL, dat->ppro->m_szModuleName, idstr, &dbv )) {
					SetDlgItemTextA( hwndDlg, IDC_EMAIL, dbv.pszVal );
					JFreeVariant( &dbv );
					wsprintfA( idstr, "e-mailFlag%d", lParam );
					nFlag = DBGetContactSettingWord( NULL, dat->ppro->m_szModuleName, idstr, 0 );
					if ( nFlag & JABBER_VCEMAIL_HOME ) CheckDlgButton( hwndDlg, IDC_HOME, TRUE );
					if ( nFlag & JABBER_VCEMAIL_WORK ) CheckDlgButton( hwndDlg, IDC_WORK, TRUE );
					if ( nFlag & JABBER_VCEMAIL_INTERNET ) CheckDlgButton( hwndDlg, IDC_INTERNET, TRUE );
					if ( nFlag & JABBER_VCEMAIL_X400 ) CheckDlgButton( hwndDlg, IDC_X400, TRUE );
		}	}	}
		break;

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
						if ( DBGetContactSettingString( NULL, dat->ppro->m_szModuleName, idstr, &dbv )) break;
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

static INT_PTR CALLBACK EditPhoneDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	EditDlgParam* dat = ( EditDlgParam* )GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

	switch ( msg ) {
	case WM_INITDIALOG:
		{
			EditDlgParam* dat = ( EditDlgParam* )lParam;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, lParam );

			TranslateDialogDefault( hwndDlg );
			if ( dat->id >= 0 ) {
				DBVARIANT dbv;
				char idstr[33];
				WORD nFlag;

				SetWindowText( hwndDlg, TranslateT( "Jabber vCard: Edit Phone Number" ));
				wsprintfA( idstr, "Phone%d", dat->id );
				if ( !DBGetContactSettingString( NULL, dat->ppro->m_szModuleName, idstr, &dbv )) {
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
		break;

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
						if ( DBGetContactSettingString( NULL, dat->ppro->m_szModuleName, idstr, &dbv )) break;
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
static INT_PTR CALLBACK ContactDlgProc( HWND hwndDlg, UINT msg, WPARAM, LPARAM lParam )
{
	const unsigned long iPageId = 5;
	CJabberProto* ppro = ( CJabberProto* )GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

	switch( msg ) {
	case WM_INITDIALOG:
		if (!lParam) break; // Launched from userinfo
		ppro = ( CJabberProto* )lParam;
		{
			LVCOLUMN lvc;
			RECT rc;

			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, lParam );

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

			ppro->WindowSubscribe(hwndDlg);
		}
		break;
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
				if ( DBGetContactSettingTString( NULL, ppro->m_szModuleName, idstr, &dbv )) break;
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
				if ( DBGetContactSettingTString( NULL, ppro->m_szModuleName, idstr, &dbv )) break;
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
		case 0: {
			switch (((LPNMHDR)lParam)->code) {
			case PSN_PARAMCHANGED:
				SendMessage(hwndDlg, WM_INITDIALOG, 0, ((PSHNOTIFY*)lParam)->lParam);
				break;
			case PSN_APPLY:
				ppro->m_vCardUpdates &= ~(1UL<<iPageId);
				ppro->SaveVcardToDB( hwndDlg, iPageId );
				if (!ppro->m_vCardUpdates)
					ppro->SetServerVcard( ppro->m_bPhotoChanged, ppro->m_szPhotoFileName );
				break;
			}	}
			break;

		case IDC_EMAILS:
		case IDC_PHONES:
			switch (( ( LPNMHDR )lParam )->code ) {
			case NM_CUSTOMDRAW:
				{
					NMLVCUSTOMDRAW *nm = ( NMLVCUSTOMDRAW * ) lParam;

					switch ( nm->nmcd.dwDrawStage ) {
					case CDDS_PREPAINT:
					case CDDS_ITEMPREPAINT:
						SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, CDRF_NOTIFYSUBITEMDRAW );
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
							SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, CDRF_SKIPDEFAULT );
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
							EditDlgParam param = { -1, ppro };
							int res;
							if ( nm->hdr.idFrom == IDC_PHONES )
								res = DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_VCARD_ADDPHONE ), hwndDlg, EditPhoneDlgProc, ( LPARAM )&param );
							else
								res = DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_VCARD_ADDEMAIL ), hwndDlg, EditEmailDlgProc, ( LPARAM )&param );
							if ( res != IDOK )
								break;
							SendMessage( hwndDlg, M_REMAKELISTS, 0, 0 );
							ppro->m_vCardUpdates |= (1UL<<iPageId);
							SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
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
								if ( DBGetContactSettingString( NULL, ppro->m_szModuleName, idstr, &dbv )) break;
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
							ppro->m_vCardUpdates |= (1UL<<iPageId);
							SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
						}
						else if ( hti.iSubItem == 2 ) {
							EditDlgParam param = { lvi.lParam, ppro };
							int res;
							if ( nm->hdr.idFrom == IDC_PHONES )
								res = DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_VCARD_ADDPHONE ), hwndDlg, EditPhoneDlgProc, ( LPARAM )&param );
							else
								res = DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_VCARD_ADDEMAIL ), hwndDlg, EditEmailDlgProc, ( LPARAM )&param );
							if ( res != IDOK )
								break;
							SendMessage( hwndDlg,M_REMAKELISTS,0,0 );
							ppro->m_vCardUpdates |= (1UL<<iPageId);
							SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
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
	case WM_DESTROY:
		ppro->WindowUnsubscribe(hwndDlg);
		break;
	}
	return FALSE;
}

void CJabberProto::SaveVcardToDB( HWND hwndPage, int iPage )
{
	TCHAR text[2048];

	// Page 0: Personal
	switch (iPage) {
	case 0:
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
		switch( SendMessage( GetDlgItem( hwndPage, IDC_GENDER ), CB_GETCURSEL, 0, 0 )) {
			case 0:	JSetString( NULL, "GenderString", "Male" );   break;
			case 1:	JSetString( NULL, "GenderString", "Female" ); break;
			default: JSetString( NULL, "GenderString", "" );       break;
		}
		GetDlgItemText( hwndPage, IDC_OCCUPATION, text, SIZEOF( text ));
		JSetStringT( NULL, "Role", text );
		GetDlgItemText( hwndPage, IDC_HOMEPAGE, text, SIZEOF( text ));
		JSetStringT( NULL, "Homepage", text );
		break;

	// Page 1: Home
	case 1:
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
		JSetStringT( NULL, "Country", text );
		break;

	// Page 2: Work
	case 2:
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
		JSetStringT( NULL, "CompanyCountry", text );
		break;

	// Page 3: Photo
	// not needed to be saved into db

	// Page 4: Note
	case 4:
		GetDlgItemText( hwndPage, IDC_DESC, text, SIZEOF( text ));
		JSetStringT( NULL, "About", text );
		break;

	// Page 5: Contacts
	// is always synced with db
}	}

void CJabberProto::AppendVcardFromDB( HXML n, char* tag, char* key )
{
	if ( n == NULL || tag == NULL || key == NULL )
		return;

	DBVARIANT dbv;
	if ( DBGetContactSettingTString( NULL, m_szModuleName, key, &dbv ))
		n << XCHILD( _A2T(tag));
	else {
		n << XCHILD( _A2T(tag), dbv.ptszVal );
		JFreeVariant( &dbv );
}	}

void CJabberProto::SetServerVcard( BOOL bPhotoChanged, TCHAR* szPhotoFileName )
{
	if (!m_bJabberOnline) return;

	DBVARIANT dbv;
	int  iqId;
	TCHAR *szFileName;
	int  i;
	char idstr[33];
	WORD nFlag;

	iqId = SerialNext();
	IqAdd( iqId, IQ_PROC_SETVCARD, &CJabberProto::OnIqResultSetVcard );

	XmlNodeIq iq( _T("set"), iqId );
	HXML v = iq << XCHILDNS( _T("vCard"), _T(JABBER_FEAT_VCARD_TEMP));

	AppendVcardFromDB( v, "FN", "FullName" );

	HXML n = v << XCHILD( _T("N"));
	AppendVcardFromDB( n, "GIVEN", "FirstName" );
	AppendVcardFromDB( n, "MIDDLE", "MiddleName" );
	AppendVcardFromDB( n, "FAMILY", "LastName" );

	AppendVcardFromDB( v, "NICKNAME", "Nick" );
	AppendVcardFromDB( v, "BDAY", "BirthDate" );
	AppendVcardFromDB( v, "GENDER", "GenderString" );

	for ( i=0;;i++ ) {
		wsprintfA( idstr, "e-mail%d", i );
		if ( DBGetContactSettingTString( NULL, m_szModuleName, idstr, &dbv )) 
			break;

		HXML e = v << XCHILD( _T("EMAIL"), dbv.ptszVal );
		JFreeVariant( &dbv );
		AppendVcardFromDB( e, "USERID", idstr );

		wsprintfA( idstr, "e-mailFlag%d", i );
		nFlag = DBGetContactSettingWord( NULL, m_szModuleName, idstr, 0 );
		if ( nFlag & JABBER_VCEMAIL_HOME )     e << XCHILD( _T("HOME"));
		if ( nFlag & JABBER_VCEMAIL_WORK )     e << XCHILD( _T("WORK"));
		if ( nFlag & JABBER_VCEMAIL_INTERNET ) e << XCHILD( _T("INTERNET"));
		if ( nFlag & JABBER_VCEMAIL_X400 )     e << XCHILD( _T("X400"));
	}

	n = v << XCHILD( _T("ADR"));
	n << XCHILD( _T("HOME"));
	AppendVcardFromDB( n, "STREET", "Street" );
	AppendVcardFromDB( n, "EXTADR", "Street2" );
	AppendVcardFromDB( n, "EXTADD", "Street2" );	// for compatibility with client using old vcard format
	AppendVcardFromDB( n, "LOCALITY", "City" );
	AppendVcardFromDB( n, "REGION", "State" );
	AppendVcardFromDB( n, "PCODE", "ZIP" );
	AppendVcardFromDB( n, "CTRY", "Country" );
	AppendVcardFromDB( n, "COUNTRY", "Country" );	// for compatibility with client using old vcard format

	n = v << XCHILD( _T("ADR"));
	n << XCHILD( _T("WORK"));
	AppendVcardFromDB( n, "STREET", "CompanyStreet" );
	AppendVcardFromDB( n, "EXTADR", "CompanyStreet2" );
	AppendVcardFromDB( n, "EXTADD", "CompanyStreet2" );	// for compatibility with client using old vcard format
	AppendVcardFromDB( n, "LOCALITY", "CompanyCity" );
	AppendVcardFromDB( n, "REGION", "CompanyState" );
	AppendVcardFromDB( n, "PCODE", "CompanyZIP" );
	AppendVcardFromDB( n, "CTRY", "CompanyCountry" );
	AppendVcardFromDB( n, "COUNTRY", "CompanyCountry" );	// for compatibility with client using old vcard format

	n = v << XCHILD( _T("ORG"));
	AppendVcardFromDB( n, "ORGNAME", "Company" );
	AppendVcardFromDB( n, "ORGUNIT", "CompanyDepartment" );
	
	AppendVcardFromDB( v, "TITLE", "CompanyPosition" );
	AppendVcardFromDB( v, "ROLE", "Role" );
	AppendVcardFromDB( v, "URL", "Homepage" );
	AppendVcardFromDB( v, "DESC", "About" );

	for ( i=0;;i++ ) {
		wsprintfA( idstr, "Phone%d", i );
		if ( DBGetContactSettingTString( NULL, m_szModuleName, idstr, &dbv )) break;
		JFreeVariant( &dbv );

		n = v << XCHILD( _T("TEL"));
		AppendVcardFromDB( n, "NUMBER", idstr );

		wsprintfA( idstr, "PhoneFlag%d", i );
		nFlag = JGetWord( NULL, idstr, 0 );
		if ( nFlag & JABBER_VCTEL_HOME )  n << XCHILD( _T("HOME"));
		if ( nFlag & JABBER_VCTEL_WORK )  n << XCHILD( _T("WORK"));
		if ( nFlag & JABBER_VCTEL_VOICE ) n << XCHILD( _T("VOICE"));
		if ( nFlag & JABBER_VCTEL_FAX )   n << XCHILD( _T("FAX"));
		if ( nFlag & JABBER_VCTEL_PAGER ) n << XCHILD( _T("PAGER"));
		if ( nFlag & JABBER_VCTEL_MSG )   n << XCHILD( _T("MSG"));
		if ( nFlag & JABBER_VCTEL_CELL )  n << XCHILD( _T("CELL"));
		if ( nFlag & JABBER_VCTEL_VIDEO ) n << XCHILD( _T("VIDEO"));
		if ( nFlag & JABBER_VCTEL_BBS )   n << XCHILD( _T("BBS"));
		if ( nFlag & JABBER_VCTEL_MODEM ) n << XCHILD( _T("MODEM"));
		if ( nFlag & JABBER_VCTEL_ISDN )  n << XCHILD( _T("ISDN"));
		if ( nFlag & JABBER_VCTEL_PCS )   n << XCHILD( _T("PCS"));
	}

	TCHAR szAvatarName[ MAX_PATH ];
	GetAvatarFileName( NULL, szAvatarName, sizeof( szAvatarName ));
	if ( bPhotoChanged )
		szFileName = szPhotoFileName;
	else
		szFileName = szAvatarName;

	// Set photo element, also update the global jabberVcardPhotoFileName to reflect the update
	Log( "Before update, file name = " TCHAR_STR_PARAM, szFileName );
	if ( szFileName == NULL || szFileName[0] == 0 ) {
		v << XCHILD( _T("PHOTO"));
		DeleteFile( szAvatarName );
		JDeleteSetting( NULL, "AvatarSaved" );
		JDeleteSetting( NULL, "AvatarHash" );
	}
	else {
		HANDLE hFile;
		struct _stat st;
		char* buffer, *str;
		DWORD nRead;

		Log( "Saving picture from " TCHAR_STR_PARAM, szFileName );
		if ( _tstat( szFileName, &st ) >= 0 ) {
			// Note the FILE_SHARE_READ attribute so that the CopyFile can succeed
			if (( hFile=CreateFile( szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL )) != INVALID_HANDLE_VALUE ) {
				if (( buffer=( char* )mir_alloc( st.st_size )) != NULL ) {
					if ( ReadFile( hFile, buffer, st.st_size, &nRead, NULL )) {
						if (( str=JabberBase64Encode( buffer, nRead )) != NULL ) {
							n = v << XCHILD( _T("PHOTO"));
							TCHAR* szFileType;
							switch( JabberGetPictureType( buffer )) {
								case PA_FORMAT_PNG:  szFileType = _T("image/png");   break;
								case PA_FORMAT_GIF:  szFileType = _T("image/gif");   break;
								case PA_FORMAT_BMP:  szFileType = _T("image/bmp");   break;
								default:             szFileType = _T("image/jpeg");  break;
							}
							n << XCHILD( _T("TYPE"), szFileType );

							n << XCHILD( _T("BINVAL"), _A2T(str));
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

							m_options.AvatarType = JabberGetPictureType( buffer );	

							if ( bPhotoChanged ) {
								DeleteFile( szAvatarName );

								GetAvatarFileName( NULL, szAvatarName, sizeof( szAvatarName ));

								CopyFile( szFileName, szAvatarName, FALSE );
							}

							JSetString( NULL, "AvatarHash", buf );
							JSetString( NULL, "AvatarSaved", buf );
					}	}
					mir_free( buffer );
				}
				CloseHandle( hFile );
	}	}	}

	m_ThreadInfo->send( iq );
}

/////////////////////////////////////////////////////////////////////////////////////////

void CJabberProto::OnUserInfoInit_VCard( WPARAM wParam, LPARAM )
{
	m_vCardUpdates = 0;
	m_bPhotoChanged = FALSE;
	m_szPhotoFileName[0] = 0;

	OPTIONSDIALOGPAGE odp = {0};
	odp.cbSize = sizeof(odp);
	odp.hInstance = hInst;
	odp.dwInitParam = (LPARAM)this;
	odp.flags = ODPF_TCHAR|ODPF_USERINFOTAB|ODPF_DONTTRANSLATE;
	odp.ptszTitle = m_tszUserName;

	odp.pfnDlgProc = PersonalDlgProc;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_VCARD_PERSONAL);
	odp.ptszTab = LPGENT("General");
	JCallService( MS_USERINFO_ADDPAGE, wParam, ( LPARAM )&odp );

	odp.pfnDlgProc = ContactDlgProc;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_VCARD_CONTACT);
	odp.ptszTab = LPGENT("Contacts");
	JCallService( MS_USERINFO_ADDPAGE, wParam, ( LPARAM )&odp );

	odp.pfnDlgProc = HomeDlgProc;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_VCARD_HOME);
	odp.ptszTab = LPGENT("Home");
	JCallService( MS_USERINFO_ADDPAGE, wParam, ( LPARAM )&odp );

	odp.pfnDlgProc = WorkDlgProc;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_VCARD_WORK);
	odp.ptszTab = LPGENT("Work");
	JCallService( MS_USERINFO_ADDPAGE, wParam, ( LPARAM )&odp );

	odp.pfnDlgProc = PhotoDlgProc;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_VCARD_PHOTO);
	odp.ptszTab = LPGENT("Photo");
	JCallService( MS_USERINFO_ADDPAGE, wParam, ( LPARAM )&odp );

	odp.pfnDlgProc = NoteDlgProc;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_VCARD_NOTE);
	odp.ptszTab = LPGENT("Note");
	JCallService( MS_USERINFO_ADDPAGE, wParam, ( LPARAM )&odp );

	SendGetVcard( m_szJabberJID );
}
