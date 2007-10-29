/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

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

static HANDLE hUserInfoList = NULL;

struct UserInfoStringBuf
{
	enum { STRINGBUF_INCREMENT = 1024 };

	TCHAR *buf;
	int size;
	int offset;

	UserInfoStringBuf() { buf = 0; size = 0; offset = 0; }
	~UserInfoStringBuf() { mir_free(buf); }

	void append( TCHAR *str ) {
		if ( !str ) return;

		int length = lstrlen( str );
		if ( size - offset < length + 1 ) {
			size += ( length + STRINGBUF_INCREMENT );
			buf = ( TCHAR * )mir_realloc( buf, size * sizeof( TCHAR ));
		}
		lstrcpy( buf + offset, str );
		offset += length;
	}

	TCHAR *allocate( int length ) {
		if ( size - offset < length ) {
			size += ( length + STRINGBUF_INCREMENT );
			buf = ( TCHAR * )mir_realloc( buf, size * sizeof( TCHAR ));
		}
		return buf + offset;
	}

	void actualize() {
		if ( buf ) offset = lstrlen( buf );
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// JabberUserInfoDlgProc - main user info dialog

struct JabberUserInfoDlgData
{
	HANDLE				hContact;
	JABBER_LIST_ITEM	*item;
	int					resourceCount;
};

static HTREEITEM sttFillInfoLine( HWND hwndTree, HTREEITEM htiRoot, HICON hIcon, TCHAR *title, TCHAR *value )
{
	TCHAR buf[256];
	if ( title )
		mir_sntprintf( buf, SIZEOF(buf), _T("%s: %s"), title, value );
	else
		lstrcpyn( buf, value, SIZEOF( buf ));

	TVINSERTSTRUCT tvis = {0};
	tvis.hParent = htiRoot;
	tvis.hInsertAfter = TVI_LAST;
	tvis.itemex.mask = TVIF_TEXT;
	tvis.itemex.pszText = buf;

	if ( hIcon ) {
		HIMAGELIST himl = TreeView_GetImageList( hwndTree, TVSIL_NORMAL );
		tvis.itemex.mask |= TVIF_IMAGE|TVIF_SELECTEDIMAGE;
		tvis.itemex.iImage =
		tvis.itemex.iSelectedImage = ImageList_AddIcon( himl, hIcon );
	}

	return TreeView_InsertItem( hwndTree, &tvis );
}

static void sttFillResourceInfo( HWND hwndTree, HTREEITEM htiRoot, JABBER_LIST_ITEM *item, JABBER_RESOURCE_STATUS *res )
{
	TCHAR buf[256];
	HTREEITEM htiResource = htiRoot;

	if ( res->resourceName && *res->resourceName )
		htiResource = sttFillInfoLine( hwndTree, htiRoot, LoadSkinnedProtoIcon( jabberProtoName, res->status ), TranslateT("Resource"), res->resourceName );

	{	// StatusMsg
		sttFillInfoLine( hwndTree, htiResource, LoadSkinnedIcon( SKINICON_EVENT_MESSAGE ), TranslateT( "Message" ),
			res->statusMessage ? res->statusMessage : TranslateT( "<not specified>" ));
	}

	{	// Software
		HICON hIcon = NULL;
		if (ServiceExists( "Fingerprint/GetClientIcon" )) {
			char *szMirver = mir_t2a(res->software);
			hIcon = (HICON)CallService( "Fingerprint/GetClientIcon", (WPARAM)szMirver, 1 );
			mir_free( szMirver );
		}

		sttFillInfoLine( hwndTree, htiResource, hIcon, TranslateT( "Software" ),
			res->software ? res->software : TranslateT( "<not specified>" ));
	}

	{	// Version
		sttFillInfoLine( hwndTree, htiResource, NULL, TranslateT( "Version" ),
			res->version ? res->version : TranslateT( "<not specified>" ));
	}

	{	// System
		sttFillInfoLine( hwndTree, htiResource, NULL, TranslateT( "System" ),
			res->system ? res->system : TranslateT( "<not specified>" ));
	}

	{	// Resource priority
		TCHAR szPriority[128];
		mir_sntprintf( szPriority, SIZEOF( szPriority ), _T("%d"), (int)res->priority );
		sttFillInfoLine( hwndTree, htiResource, NULL, TranslateT( "Resource priority" ), szPriority );
	}

	{	// Idle
		if ( item->itemResource.idleStartTime > 0 ) {
			lstrcpyn(buf, _tctime( &item->itemResource.idleStartTime ), SIZEOF( buf ));
			int len = lstrlen(buf);
			if (len > 0) buf[len-1] = 0;
		} else if ( !item->itemResource.idleStartTime )
			lstrcpyn(buf, TranslateT( "unknown" ), SIZEOF( buf ));
		else
			lstrcpyn(buf, TranslateT( "<not specified>" ), SIZEOF( buf ));

		sttFillInfoLine( hwndTree, htiResource, NULL, TranslateT("Idle since"), buf );
	}

	{	// caps
		mir_sntprintf( buf, SIZEOF(buf), _T("%s/%s"), item->jid, res->resourceName );
		JabberCapsBits jcb = JabberGetResourceCapabilites( buf );

		if ( !( jcb & JABBER_RESOURCE_CAPS_ERROR ))
		{
			HTREEITEM htiCaps = sttFillInfoLine( hwndTree, htiResource, LoadIconEx( "main" ), NULL, TranslateT( "Client capabilities" ));
			for ( int i = 0; g_JabberFeatCapPairs[i].szFeature; i++ ) 
				if ( jcb & g_JabberFeatCapPairs[i].jcbCap ) {
					TCHAR szDescription[ 1024 ];
					if ( g_JabberFeatCapPairs[i].szDescription )
						mir_sntprintf( szDescription, SIZEOF( szDescription ), _T("%s (%s)"), g_JabberFeatCapPairs[i].szDescription, g_JabberFeatCapPairs[i].szFeature );
					else
						mir_sntprintf( szDescription, SIZEOF( szDescription ), _T("%s"), g_JabberFeatCapPairs[i].szFeature );
					sttFillInfoLine( hwndTree, htiCaps, NULL, NULL, szDescription );
				}
		}
	}

	TreeView_Expand( hwndTree, htiResource, TVE_EXPAND );
}

static void sttFillUserInfo( HWND hwndTree, JABBER_LIST_ITEM *item )
{
	SendMessage( hwndTree, WM_SETREDRAW, FALSE, 0 );

	TreeView_DeleteAllItems( hwndTree );

	HTREEITEM htiRoot = sttFillInfoLine( hwndTree, NULL, LoadIconEx( "main" ), _T( "JID" ), item->jid );
	TCHAR buf[256];

	{	// subscription
		switch ( item->subscription ) {
			case SUB_BOTH:
				sttFillInfoLine( hwndTree, htiRoot, NULL, TranslateT( "Subscription" ), TranslateT( "both" ));
				break;
			case SUB_TO:
				sttFillInfoLine( hwndTree, htiRoot, NULL, TranslateT( "Subscription" ), TranslateT( "to" ));
				break;
			case SUB_FROM:
				sttFillInfoLine( hwndTree, htiRoot, NULL, TranslateT( "Subscription" ), TranslateT( "from" ));
				break;
			default:
				sttFillInfoLine( hwndTree, htiRoot, NULL, TranslateT( "Subscription" ), TranslateT( "none" ));
				break;
		}
	}

	{	// logoff
		if ( item->itemResource.idleStartTime > 0 ) {
			lstrcpyn( buf, _tctime( &item->itemResource.idleStartTime ), SIZEOF( buf ));
			int len = lstrlen(buf);
			if (len > 0) buf[len-1] = 0;
		} else if ( !item->itemResource.idleStartTime )
			lstrcpyn( buf, TranslateT( "unknown" ), SIZEOF( buf ));
		else
			lstrcpyn( buf, TranslateT( "<not specified>" ), SIZEOF( buf ));

		sttFillInfoLine( hwndTree, htiRoot, NULL, TranslateT( "Last logoff time" ), buf );
	}

	{	// activity
		if (( item->lastSeenResource >= 0 ) && ( item->lastSeenResource < item->resourceCount ))
			lstrcpyn( buf, item->resource[item->lastSeenResource].resourceName, SIZEOF( buf ));
		else
			lstrcpyn( buf, TranslateT( "<no information available>" ), SIZEOF( buf ));

		sttFillInfoLine( hwndTree, htiRoot, NULL, TranslateT( "Last active resource" ), buf );
	}

	{	// resources
		if ( item->resourceCount ) {
			for (int i = 0; i < item->resourceCount; ++i)
				sttFillResourceInfo( hwndTree, htiRoot, item, &item->resource[i] );
		} else if ( item->itemResource.status != ID_STATUS_OFFLINE ) {
			sttFillResourceInfo( hwndTree, htiRoot, item, &item->itemResource );
		}
	}

	TreeView_Expand( hwndTree, htiRoot, TVE_EXPAND );

	SendMessage( hwndTree, WM_SETREDRAW, TRUE, 0 );
	RedrawWindow( hwndTree, NULL, NULL, RDW_INVALIDATE );
}

static void sttGetNodeText( HWND hwndTree, HTREEITEM hti, UserInfoStringBuf *buf, int indent = 0 )
{
	for ( int i = 0; i < indent; ++i )
		buf->append( _T( "\t" ));

	TVITEMEX tvi = {0};
	tvi.mask = TVIF_HANDLE|TVIF_TEXT|TVIF_STATE;
	tvi.hItem = hti;
	tvi.cchTextMax = 256;
	tvi.pszText = buf->allocate( tvi.cchTextMax );
	if (!TreeView_GetItem( hwndTree, &tvi )) { // failure, maybe item was removed...
		buf->buf[ buf->offset ] = 0;
		buf->actualize();
		return;
	}

	buf->actualize();
	buf->append( _T( "\r\n" ));

	if ( tvi.state & TVIS_EXPANDED )
		for ( hti = TreeView_GetChild( hwndTree, hti ); hti; hti = TreeView_GetNextSibling( hwndTree, hti ))
			sttGetNodeText( hwndTree, hti, buf, indent + 1 );
}

static BOOL CALLBACK JabberUserInfoDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	JabberUserInfoDlgData *dat = (JabberUserInfoDlgData *)GetWindowLong( hwndDlg, GWL_USERDATA );

	switch ( msg ) {
		case WM_INITDIALOG:
			{
			// lParam is hContact
			TranslateDialogDefault( hwndDlg );

			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_OTHER_USERDETAILS));

			dat = (JabberUserInfoDlgData *)mir_alloc(sizeof(JabberUserInfoDlgData));
			dat->resourceCount = -1;

			if ( CallService(MS_DB_CONTACT_IS, (WPARAM)lParam, 0 )) {
				dat->hContact = (HANDLE)lParam;
				DBVARIANT dbv = {0};
				if (JGetStringT(dat->hContact, "jid", &dbv)) break;
				JABBER_LIST_ITEM *item = NULL;
				if (!(dat->item = JabberListGetItemPtr( LIST_VCARD_TEMP, dbv.ptszVal )))
					dat->item = JabberListGetItemPtr( LIST_ROSTER, dbv.ptszVal );
				JFreeVariant(&dbv);
			} else
			if (!IsBadReadPtr((void *)lParam, sizeof(JABBER_LIST_ITEM)))
			{
				dat->hContact = NULL;
				dat->item = (JABBER_LIST_ITEM *)lParam;
			}

			RECT rc; GetClientRect( hwndDlg, &rc );
			MoveWindow( GetDlgItem( hwndDlg, IDC_TV_INFO ), 5, 5, rc.right-10, rc.bottom-10, TRUE );

			HIMAGELIST himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR|ILC_COLOR32|ILC_MASK, 5, 1);
			ImageList_AddIcon(himl, LoadSkinnedIcon(SKINICON_OTHER_SMALLDOT));
			TreeView_SetImageList(GetDlgItem(hwndDlg, IDC_TV_INFO), himl, TVSIL_NORMAL);

			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)dat);
			WindowList_Add(hUserInfoList, hwndDlg, dat->hContact);

			SendMessage(hwndDlg, WM_JABBER_REFRESH, 0, 0);
			return TRUE;
		}

		case WM_DESTROY:
		{
			WindowList_Remove(hUserInfoList, hwndDlg);
			mir_free(dat);
			ImageList_Destroy(TreeView_SetImageList(GetDlgItem(hwndDlg, IDC_TV_INFO), NULL, TVSIL_NORMAL));
			break;
		}

		case WM_JABBER_REFRESH:
		{
			if (!dat->item)
			{
				DBVARIANT dbv = {0};
				if (JGetStringT(dat->hContact, "jid", &dbv)) break;
				JABBER_LIST_ITEM *item = NULL;
				if (!(dat->item = JabberListGetItemPtr(LIST_VCARD_TEMP, dbv.ptszVal)))
					dat->item = JabberListGetItemPtr(LIST_ROSTER, dbv.ptszVal);
				JFreeVariant(&dbv);

				if (!dat->item) break;
			}
			sttFillUserInfo(GetDlgItem(hwndDlg, IDC_TV_INFO), dat->item);
			break;
		}

		case WM_SIZE:
		{
			MoveWindow(GetDlgItem(hwndDlg, IDC_TV_INFO), 5, 5, LOWORD(lParam)-10, HIWORD(lParam)-10, TRUE);
			break;
		}

		case WM_CLOSE:
		{
			DestroyWindow(hwndDlg);
			break;
		}

		case WM_NOTIFY:
		{
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

			case IDC_TV_INFO:
				switch (( ( LPNMHDR )lParam )->code ) {
				case NM_RCLICK:
					{
						HWND hwndTree = GetDlgItem(hwndDlg, IDC_TV_INFO);
						TVHITTESTINFO tvhti = {0};
						GetCursorPos(&tvhti.pt);
						ScreenToClient(hwndTree, &tvhti.pt);
						TreeView_HitTest(hwndTree, &tvhti);
						ClientToScreen(hwndTree, &tvhti.pt);
						if (tvhti.flags & TVHT_ONITEM)
						{
							HMENU hMenu = CreatePopupMenu();
							AppendMenu(hMenu, MF_STRING, (UINT_PTR)1, TranslateT("Copy"));
							AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
							AppendMenu(hMenu, MF_STRING, (UINT_PTR)0, TranslateT("Cancel"));
							if (TrackPopupMenu(hMenu, TPM_RETURNCMD, tvhti.pt.x, tvhti.pt.y, 0, hwndDlg, NULL))
							{
								UserInfoStringBuf buf;
								sttGetNodeText(hwndTree, tvhti.hItem, &buf);

								OpenClipboard(hwndDlg);
								EmptyClipboard();
								HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sizeof(TCHAR)*(lstrlen(buf.buf)+1));
								TCHAR *s = (TCHAR *)GlobalLock(hMem);
								lstrcpy(s, buf.buf);
								GlobalUnlock(hMem);
								#ifdef UNICODE
									SetClipboardData(CF_UNICODETEXT, hMem);
								#else
									SetClipboardData(CF_TEXT, hMem);
								#endif
								CloseClipboard();
							}
							DestroyMenu(hMenu);
						}
					}
					break;
				}
				break;
			}
			break;
		}
	}
	return FALSE;
}

/*
void JabberUserInfoShowBox(JABBER_LIST_ITEM *item)
{
	HWND hwnd = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_INFOBOX), NULL, JabberUserInfoDlgProc, (LPARAM)item);
	ShowWindow(hwnd, SW_SHOW);
}
*/
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
						JabberBitmapPremultiplyChannels(photoInfo->hBitmap);
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
			odp.pszTitle = LPGEN("Photo");
			JCallService( MS_USERINFO_ADDPAGE, wParam, ( LPARAM )&odp );
	}	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberUserInfoUpdate

void JabberUserInfoInit()
{
	hUserInfoList = ( HANDLE )CallService( MS_UTILS_ALLOCWINDOWLIST, 0, 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberUserInfoUpdate

void JabberUserInfoUpdate( HANDLE hContact )
{
	if ( !hContact ) {
		WindowList_BroadcastAsync( hUserInfoList, WM_JABBER_REFRESH, 0, 0 );
	} else
	if ( HWND hwnd = WindowList_Find( hUserInfoList, hContact ))
	{
		PostMessage( hwnd, WM_JABBER_REFRESH, 0, 0 );
	}
}
