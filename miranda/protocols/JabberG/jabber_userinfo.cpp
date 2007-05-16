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

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_userinfo.cpp,v $
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"

#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

#include <commctrl.h>
#include "jabber_list.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////////////////
// JabberUserInfoDlgProc - main user info dialog

static BOOL CALLBACK JabberUserInfoDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
		hwndJabberInfo = hwndDlg;
		// lParam is hContact
		TranslateDialogDefault( hwndDlg );
		SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG )( HANDLE ) lParam );
		SendMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
		return TRUE;
	case WM_DESTROY:
		hwndJabberInfo = NULL;
		break;
	case WM_JABBER_REFRESH:
		{
			DBVARIANT dbv;
			JABBER_LIST_ITEM *item;
			JABBER_RESOURCE_STATUS *r;

			HWND hwndList = GetDlgItem( hwndDlg, IDC_INFO_RESOURCE );
			int	selectedResource = SendMessage( hwndList, LB_GETCURSEL, 0, 0 );
			SendMessage( hwndList, LB_RESETCONTENT, 0, 0 );
			SetDlgItemTextA( hwndDlg, IDC_INFO_JID, "" );
			SetDlgItemTextA( hwndDlg, IDC_SUBSCRIPTION, "" );
			SetDlgItemText( hwndDlg, IDC_SOFTWARE, TranslateT( "<click resource to view>" ));
			SetDlgItemText( hwndDlg, IDC_VERSION, TranslateT( "<click resource to view>" ));
			SetDlgItemText( hwndDlg, IDC_SYSTEM, TranslateT( "<click resource to view>" ));
			SetDlgItemText( hwndDlg, IDC_IDLE_SINCE, TranslateT( "<click resource to view>" ));
			EnableWindow( GetDlgItem( hwndDlg, IDC_SOFTWARE ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_VERSION ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_SYSTEM ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_IDLE_SINCE ), FALSE );

			HANDLE hContact = ( HANDLE ) GetWindowLong( hwndDlg, GWL_USERDATA );
			if ( !JGetStringT( hContact, "jid", &dbv )) {
				SetDlgItemText( hwndDlg, IDC_INFO_JID, dbv.ptszVal );

				if ( jabberOnline ) {
					if (( item = JabberListGetItemPtr( LIST_VCARD_TEMP, dbv.ptszVal )) == NULL)
						item = JabberListGetItemPtr( LIST_ROSTER, dbv.ptszVal );
					if ( item != NULL )
					{
						if (( r=item->resource ) != NULL ) {
							int count = item->resourceCount;
							for ( int i=0; i<count; i++ ) {
								int index = SendMessage( hwndList, LB_ADDSTRING, 0, ( LPARAM )r[i].resourceName );
								SendMessage( hwndList, LB_SETITEMDATA, index, ( LPARAM )r[i].resourceName );
						}	}

						if ( selectedResource == LB_ERR && item->resourceCount == 1 )
							selectedResource = 0;

						if ( selectedResource != LB_ERR ) {
							SendMessage( hwndList, LB_SETCURSEL, selectedResource, 0 );
							PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_INFO_RESOURCE, LBN_SELCHANGE), 0);
						}

						switch ( item->subscription ) {
						case SUB_BOTH:
							SetDlgItemText( hwndDlg, IDC_SUBSCRIPTION, TranslateT( "both" ));
							break;
						case SUB_TO:
							SetDlgItemText( hwndDlg, IDC_SUBSCRIPTION, TranslateT( "to" ));
							break;
						case SUB_FROM:
							SetDlgItemText( hwndDlg, IDC_SUBSCRIPTION, TranslateT( "from" ));
							break;
						default:
							SetDlgItemText( hwndDlg, IDC_SUBSCRIPTION, TranslateT( "none" ));
							break;
						}

						if ( item->logoffTime > 0 ) {
							TCHAR logoffTime[26];
							_tcsncpy( logoffTime, _tctime(&item->logoffTime), 24 );
							logoffTime[24] = _T( '\0' );
							SetDlgItemText( hwndDlg, IDC_LOGOFF_TIME, logoffTime );
							EnableWindow( GetDlgItem( hwndDlg, IDC_LOGOFF_TIME ), TRUE );
						}
						else if ( !item->logoffTime ) {
							SetDlgItemText( hwndDlg, IDC_LOGOFF_TIME, TranslateT( "unknown" ));
							EnableWindow( GetDlgItem( hwndDlg, IDC_LOGOFF_TIME ), FALSE );
						}
						else {
							SetDlgItemText( hwndDlg, IDC_LOGOFF_TIME, TranslateT( "<not specified>" ));
							EnableWindow( GetDlgItem( hwndDlg, IDC_LOGOFF_TIME ), FALSE );
						}

					}
					else SetDlgItemText( hwndDlg, IDC_SUBSCRIPTION, TranslateT( "none ( not on roster )" ));
				}
				else EnableWindow( hwndList, FALSE );
				JFreeVariant( &dbv );
		}	}
		break;
	case WM_NOTIFY:
		switch (( ( LPNMHDR )lParam )->idFrom ) {
		case 0:
			switch (( ( LPNMHDR )lParam )->code ) {
			case PSN_INFOCHANGED:
				{
					HANDLE hContact = ( HANDLE ) (( LPPSHNOTIFY ) lParam )->lParam;
					SendMessage( hwndDlg, WM_JABBER_REFRESH, 0, ( LPARAM )hContact );
				}
				break;
			}
			break;
		}
		break;
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_INFO_RESOURCE:
			switch ( HIWORD( wParam )) {
			case LBN_SELCHANGE:
				{
					HWND hwndList = GetDlgItem( hwndDlg, IDC_INFO_RESOURCE );
					HANDLE hContact = ( HANDLE ) GetWindowLong( hwndDlg, GWL_USERDATA );

					DBVARIANT dbv;
					if ( !JGetStringT( hContact, "jid", &dbv )) {
						TCHAR* jid = dbv.ptszVal;
						int nItem = SendMessage( hwndList, LB_GETCURSEL, 0, 0 );
						TCHAR* szResource = ( TCHAR* )SendMessage( hwndList, LB_GETITEMDATA, ( WPARAM ) nItem, 0 );

						JABBER_LIST_ITEM* item = NULL;
						if (( item = JabberListGetItemPtr( LIST_VCARD_TEMP, jid )) == NULL)
							item = JabberListGetItemPtr( LIST_ROSTER, jid );
						
						JABBER_RESOURCE_STATUS *r;

						if ( szResource != ( TCHAR* )LB_ERR && item != NULL && ( r=item->resource ) != NULL ) {
							int i;
							for ( i=0; i < item->resourceCount && _tcscmp( r[i].resourceName, szResource ); i++ );
							if ( i < item->resourceCount ) {
								if ( r[i].software != NULL ) {
									SetDlgItemText( hwndDlg, IDC_SOFTWARE, r[i].software );
									EnableWindow( GetDlgItem( hwndDlg, IDC_SOFTWARE ), TRUE );
								}
								else {
									SetDlgItemText( hwndDlg, IDC_SOFTWARE, TranslateT( "<not specified>" ));
									EnableWindow( GetDlgItem( hwndDlg, IDC_SOFTWARE ), FALSE );
								}
								if ( r[i].version != NULL ) {
									SetDlgItemText( hwndDlg, IDC_VERSION, r[i].version );
									EnableWindow( GetDlgItem( hwndDlg, IDC_VERSION ), TRUE );
								}
								else {
									SetDlgItemText( hwndDlg, IDC_VERSION, TranslateT( "<not specified>" ));
									EnableWindow( GetDlgItem( hwndDlg, IDC_VERSION ), FALSE );
								}
								if ( r[i].system != NULL ) {
									SetDlgItemText( hwndDlg, IDC_SYSTEM, r[i].system );
									EnableWindow( GetDlgItem( hwndDlg, IDC_SYSTEM ), TRUE );
								}
								else {
									SetDlgItemText( hwndDlg, IDC_SYSTEM, TranslateT( "<not specified>" ));
									EnableWindow( GetDlgItem( hwndDlg, IDC_SYSTEM ), FALSE );
								}
								if ( r[i].idleStartTime > 0 ) {
									TCHAR logoffTime[26];
									_tcsncpy( logoffTime, _tctime(&r[i].idleStartTime), 24 );
									logoffTime[24] = _T( '\0' );
									SetDlgItemText( hwndDlg, IDC_IDLE_SINCE, logoffTime );
									EnableWindow( GetDlgItem( hwndDlg, IDC_IDLE_SINCE ), TRUE );
								}
								else if ( !r[i].idleStartTime ) {
									SetDlgItemText( hwndDlg, IDC_IDLE_SINCE, TranslateT( "<unknown>" ));
									EnableWindow( GetDlgItem( hwndDlg, IDC_IDLE_SINCE ), FALSE );
								}
								else {
									SetDlgItemText( hwndDlg, IDC_IDLE_SINCE, TranslateT( "<not specified>" ));
									EnableWindow( GetDlgItem( hwndDlg, IDC_IDLE_SINCE ), FALSE );
						}	}	}
						JFreeVariant( &dbv );
			}	}	}
			break;
		}
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberUserPhotoDlgProc - Jabber photo dialog

typedef struct {
	HANDLE hContact;
	HBITMAP hBitmap;
} USER_PHOTO_INFO;

static BOOL CALLBACK JabberUserPhotoDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	USER_PHOTO_INFO *photoInfo;

	photoInfo = ( USER_PHOTO_INFO * ) GetWindowLong( hwndDlg, GWL_USERDATA );

	switch ( msg ) {
	case WM_INITDIALOG:
		// lParam is hContact
		TranslateDialogDefault( hwndDlg );
		photoInfo = ( USER_PHOTO_INFO * ) mir_alloc( sizeof( USER_PHOTO_INFO ));
		photoInfo->hContact = ( HANDLE ) lParam;
		photoInfo->hBitmap = NULL;
		SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG ) photoInfo );
		SendMessage( GetDlgItem( hwndDlg, IDC_SAVE ), BM_SETIMAGE, IMAGE_ICON, ( LPARAM )LoadImage( hInst, MAKEINTRESOURCE( IDI_SAVE ), IMAGE_ICON, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 ));
		ShowWindow( GetDlgItem( hwndDlg, IDC_LOAD ), SW_HIDE );
		ShowWindow( GetDlgItem( hwndDlg, IDC_DELETE ), SW_HIDE );
		SendMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
		return TRUE;
	case WM_NOTIFY:
		switch (( ( LPNMHDR )lParam )->idFrom ) {
		case 0:
			switch (( ( LPNMHDR )lParam )->code ) {
			case PSN_INFOCHANGED:
				SendMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
				break;
			}
			break;
		}
		break;
	case WM_JABBER_REFRESH:
		{
			JABBER_LIST_ITEM *item;
			DBVARIANT dbv;

			if ( photoInfo->hBitmap ) {
				DeleteObject( photoInfo->hBitmap );
				photoInfo->hBitmap = NULL;
			}
			ShowWindow( GetDlgItem( hwndDlg, IDC_SAVE ), SW_HIDE );
			if ( !JGetStringT( photoInfo->hContact, "jid", &dbv )) {
				TCHAR* jid = dbv.ptszVal;
				if (( item = JabberListGetItemPtr( LIST_VCARD_TEMP, jid )) == NULL)
					item = JabberListGetItemPtr( LIST_ROSTER, jid );
				if ( item != NULL ) {
					if ( item->photoFileName ) {
						JabberLog( "Showing picture from %s", item->photoFileName );
						photoInfo->hBitmap = ( HBITMAP ) JCallService( MS_UTILS_LOADBITMAP, 0, ( LPARAM )item->photoFileName );
						ShowWindow( GetDlgItem( hwndDlg, IDC_SAVE ), SW_SHOW );
					}
				}
				JFreeVariant( &dbv );
			}
			InvalidateRect( hwndDlg, NULL, TRUE );
			UpdateWindow( hwndDlg );
		}
		break;
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_SAVE:
			{
				DBVARIANT dbv;
				JABBER_LIST_ITEM *item;
				HANDLE hFile;
				OPENFILENAMEA ofn;
				static char szFilter[512];
				unsigned char buffer[3];
				char szFileName[_MAX_PATH];
				DWORD n;

				if ( JGetStringT( photoInfo->hContact, "jid", &dbv ))
					break;

				TCHAR* jid = dbv.ptszVal;
				if (( item = JabberListGetItemPtr( LIST_VCARD_TEMP, jid )) == NULL)
					item = JabberListGetItemPtr( LIST_ROSTER, jid );
				if ( item != NULL ) {
					if (( hFile=CreateFileA( item->photoFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL )) != INVALID_HANDLE_VALUE ) {
						if ( ReadFile( hFile, buffer, 3, &n, NULL ) && n==3 ) {
							if ( !strncmp(( char* )buffer, "BM", 2 )) {
								mir_snprintf( szFilter, sizeof( szFilter ), "BMP %s ( *.bmp )", JTranslate( "format" ));
								n = strlen( szFilter );
								strncpy( szFilter+n+1, "*.BMP", sizeof( szFilter )-n-2 );
							}
							else if ( !strncmp(( char* )buffer, "GIF", 3 )) {
								mir_snprintf( szFilter, sizeof( szFilter ), "GIF %s ( *.gif )", JTranslate( "format" ));
								n = strlen( szFilter );
								strncpy( szFilter+n+1, "*.GIF", sizeof( szFilter )-n-2 );
							}
							else if ( buffer[0]==0xff && buffer[1]==0xd8 && buffer[2]==0xff ) {
								mir_snprintf( szFilter, sizeof( szFilter ), "JPEG %s ( *.jpg;*.jpeg )", JTranslate( "format" ));
								n = strlen( szFilter );
								strncpy( szFilter+n+1, "*.JPG;*.JPEG", sizeof( szFilter )-n-2 );
							}
							else {
								mir_snprintf( szFilter, sizeof( szFilter ), "%s ( *.* )", JTranslate( "Unknown format" ));
								n = strlen( szFilter );
								strncpy( szFilter+n+1, "*.*", sizeof( szFilter )-n-2 );
							}
							szFilter[sizeof( szFilter )-1] = '\0';

							ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
							ofn.hwndOwner = hwndDlg;
							ofn.hInstance = NULL;
							ofn.lpstrFilter = szFilter;
							ofn.lpstrCustomFilter = NULL;
							ofn.nMaxCustFilter = 0;
							ofn.nFilterIndex = 0;
							ofn.lpstrFile = szFileName;
							ofn.nMaxFile = _MAX_PATH;
							ofn.lpstrFileTitle = NULL;
							ofn.nMaxFileTitle = 0;
							ofn.lpstrInitialDir = NULL;
							ofn.lpstrTitle = NULL;
							ofn.Flags = OFN_OVERWRITEPROMPT;
							ofn.nFileOffset = 0;
							ofn.nFileExtension = 0;
							ofn.lpstrDefExt = NULL;
							ofn.lCustData = 0L;
							ofn.lpfnHook = NULL;
							ofn.lpTemplateName = NULL;
							szFileName[0] = '\0';
							if ( GetSaveFileNameA( &ofn )) {
								JabberLog( "File selected is %s", szFileName );
								CopyFileA( item->photoFileName, szFileName, FALSE );
							}
						}
						CloseHandle( hFile );
					}
				}
				JFreeVariant( &dbv );

			}
			break;
		}
		break;
	case WM_PAINT:
		if ( !jabberOnline )
			SetDlgItemText( hwndDlg, IDC_CANVAS, TranslateT( "<Photo not available while offline>" ));
		else if ( !photoInfo->hBitmap )
			SetDlgItemText( hwndDlg, IDC_CANVAS, TranslateT( "<No photo>" ));
		else {
			BITMAP bm;
			POINT ptSize, ptOrg, pt, ptFitSize;
			RECT rect;

			SetDlgItemTextA( hwndDlg, IDC_CANVAS, "" );
			HBITMAP hBitmap = photoInfo->hBitmap;
			HWND hwndCanvas = GetDlgItem( hwndDlg, IDC_CANVAS );
			HDC hdcCanvas = GetDC( hwndCanvas );
			HDC hdcMem = CreateCompatibleDC( hdcCanvas );
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
				BitBlt( hdcCanvas, pt.x, pt.y, ptSize.x, ptSize.y, hdcMem, ptOrg.x, ptOrg.y, SRCCOPY );
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
				SetStretchBltMode( hdcCanvas, COLORONCOLOR );
				StretchBlt( hdcCanvas, pt.x, pt.y, ptFitSize.x, ptFitSize.y, hdcMem, ptOrg.x, ptOrg.y, ptSize.x, ptSize.y, SRCCOPY );
			}
			DeleteDC( hdcMem );
		}
		break;
	case WM_DESTROY:
		if ( photoInfo->hBitmap ) {
			JabberLog( "Delete bitmap" );
			DeleteObject( photoInfo->hBitmap );
		}
		if ( photoInfo ) mir_free( photoInfo );
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberUserInfoInit - initializes user info option dialogs

int JabberUserInfoInit( WPARAM wParam, LPARAM lParam )
{
	if ( !JCallService( MS_PROTO_ISPROTOCOLLOADED, 0, ( LPARAM )jabberProtoName ))
		return 0;

	OPTIONSDIALOGPAGE odp = {0};
	odp.cbSize = sizeof( odp );
	odp.hInstance = hInst;

	HANDLE hContact = ( HANDLE )lParam;
	if ( hContact ) {
		char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
		if ( szProto != NULL && !strcmp( szProto, jabberProtoName )) {
			odp.pfnDlgProc = JabberUserInfoDlgProc;
			odp.position = -2000000000;
			odp.pszTemplate = MAKEINTRESOURCEA( IDD_INFO_JABBER );
			odp.pszTitle = jabberModuleName;
			JCallService( MS_USERINFO_ADDPAGE, wParam, ( LPARAM )&odp );

			odp.pfnDlgProc = JabberUserPhotoDlgProc;
			odp.position = 2000000000;
			odp.pszTemplate = MAKEINTRESOURCEA( IDD_VCARD_PHOTO );
			odp.pszTitle = JTranslate( "Photo" );
			JCallService( MS_USERINFO_ADDPAGE, wParam, ( LPARAM )&odp );
	}	}
	return 0;
}
