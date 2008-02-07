/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2007  Michael Stepura, George Hazan

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
#include <commctrl.h>
#include "resource.h"
#include "jabber_iq.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Bookmarks editor window

struct JabberAddBookmarkDlgParam {
	CJabberProto* ppro;
	JABBER_LIST_ITEM* m_item;
};

static BOOL CALLBACK JabberAddBookmarkDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	JabberAddBookmarkDlgParam* param = (JabberAddBookmarkDlgParam*)GetWindowLong( hwndDlg, GWL_USERDATA );

	TCHAR text[128];
	JABBER_LIST_ITEM *item;

	switch ( msg ) {
	case WM_INITDIALOG:
		param = (JabberAddBookmarkDlgParam*)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, lParam );

		param->ppro->m_hwndJabberAddBookmark = hwndDlg;
		TranslateDialogDefault( hwndDlg );
		if ( item = param->m_item ) {
			if ( !lstrcmp( item->type, _T("conference") )) {
				if (!_tcschr( item->jid, _T( '@' ))) {	  //no room name - consider it is transport
					SendDlgItemMessage(hwndDlg, IDC_AGENT_RADIO, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow( GetDlgItem( hwndDlg, IDC_NICK ), FALSE );
					EnableWindow( GetDlgItem( hwndDlg, IDC_PASSWORD ), FALSE );
				}
				else SendDlgItemMessage(hwndDlg, IDC_ROOM_RADIO, BM_SETCHECK, BST_CHECKED, 0);
			}
			else {
				SendDlgItemMessage(hwndDlg, IDC_URL_RADIO, BM_SETCHECK, BST_CHECKED, 0);
				EnableWindow( GetDlgItem( hwndDlg, IDC_NICK ), FALSE );
				EnableWindow( GetDlgItem( hwndDlg, IDC_PASSWORD ), FALSE );
				SendDlgItemMessage(hwndDlg, IDC_CHECK_BM_AUTOJOIN, BM_SETCHECK, BST_UNCHECKED, 0);
				EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_BM_AUTOJOIN), FALSE );
			}

			EnableWindow( GetDlgItem( hwndDlg, IDC_ROOM_RADIO), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_URL_RADIO), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_AGENT_RADIO), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_BM_AUTOJOIN), FALSE );

			if ( item->jid ) SetDlgItemText( hwndDlg, IDC_ROOM_JID, item->jid );
			if ( item->name ) SetDlgItemText( hwndDlg, IDC_NAME, item->name );
			if ( item->nick ) SetDlgItemText( hwndDlg, IDC_NICK, item->nick );
			if ( item->password ) SetDlgItemText( hwndDlg, IDC_PASSWORD, item->password );
			if ( item->bAutoJoin ) SendDlgItemMessage( hwndDlg, IDC_CHECK_BM_AUTOJOIN, BM_SETCHECK, BST_CHECKED, 0 );
			if ( SendDlgItemMessage(hwndDlg, IDC_ROOM_RADIO, BM_GETCHECK,0, 0) == BST_CHECKED )
				EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_BM_AUTOJOIN), TRUE );
		}
		else {
			EnableWindow( GetDlgItem( hwndDlg, IDOK ), FALSE );
			SendDlgItemMessage(hwndDlg, IDC_ROOM_RADIO, BM_SETCHECK, BST_CHECKED, 0);
		}
		return TRUE;

	case WM_COMMAND:
		switch ( HIWORD(wParam) ) {
			case BN_CLICKED:
				switch (LOWORD (wParam) ) {
					case IDC_ROOM_RADIO:
						EnableWindow( GetDlgItem( hwndDlg, IDC_NICK ), TRUE );
						EnableWindow( GetDlgItem( hwndDlg, IDC_PASSWORD ), TRUE );
						EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_BM_AUTOJOIN), TRUE );
						break;
					case IDC_AGENT_RADIO:
					case IDC_URL_RADIO:
						EnableWindow( GetDlgItem( hwndDlg, IDC_NICK ), FALSE );
						EnableWindow( GetDlgItem( hwndDlg, IDC_PASSWORD ), FALSE );
						SendDlgItemMessage(hwndDlg, IDC_CHECK_BM_AUTOJOIN, BM_SETCHECK, BST_UNCHECKED, 0);
						EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_BM_AUTOJOIN), FALSE );
						break;
				}
		}

		switch ( LOWORD( wParam )) {
		case IDC_ROOM_JID:
			if (( HWND )lParam==GetFocus() && HIWORD( wParam )==EN_CHANGE )
				EnableWindow( GetDlgItem( hwndDlg, IDOK ), GetDlgItemText( hwndDlg, IDC_ROOM_JID, text, SIZEOF( text )));
			break;

		case IDOK:
			{
				GetDlgItemText( hwndDlg, IDC_ROOM_JID, text, SIZEOF( text ));
				TCHAR* roomJID = NEWTSTR_ALLOCA( text );

				TCHAR* currJID = param->m_item->jid;
				if ( currJID )
					param->ppro->ListRemove( LIST_BOOKMARK, currJID );

				item = param->ppro->ListAdd( LIST_BOOKMARK, roomJID );
				item->bUseResource = TRUE;

				if ( SendDlgItemMessage(hwndDlg, IDC_URL_RADIO, BM_GETCHECK,0, 0) == BST_CHECKED )
					replaceStr( item->type, _T( "url" ));
				else
					replaceStr( item->type, _T( "conference" ));

				GetDlgItemText( hwndDlg, IDC_NICK, text, SIZEOF( text ));
				replaceStr( item->nick, text );

				GetDlgItemText( hwndDlg, IDC_PASSWORD, text, SIZEOF( text ));
				replaceStr( item->password, text );

				GetDlgItemText( hwndDlg, IDC_NAME, text, SIZEOF( text ));
				replaceStr( item->name, ( text[0] == 0 ) ? roomJID : text );

				item->bAutoJoin = (SendDlgItemMessage(hwndDlg, IDC_CHECK_BM_AUTOJOIN, BM_GETCHECK,0, 0) == BST_CHECKED );
				{
					int iqId = param->ppro->SerialNext();
					param->ppro->IqAdd( iqId, IQ_PROC_SETBOOKMARKS, &CJabberProto::OnIqResultSetBookmarks);

					XmlNodeIq iq( "set", iqId);
					param->ppro->SetBookmarkRequest(iq);
					param->ppro->m_ThreadInfo->send( iq );
				}
			}
			// fall through
		case IDCANCEL:
			EndDialog( hwndDlg, 0 );
			break;
		}
		break;

	case WM_JABBER_CHECK_ONLINE:
		if ( !param->ppro->m_bJabberOnline )
			EndDialog( hwndDlg, 0 );
		break;

	case WM_DESTROY:
		param->ppro->m_hwndJabberAddBookmark = NULL;
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Bookmarks manager window

static BOOL sortAscending;
static int sortColumn;

static int CALLBACK BookmarkCompare( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	CJabberProto* param = ( CJabberProto* )lParamSort;
	JABBER_LIST_ITEM *item1, *item2;
	int res = 0;
	item1 = param->ListGetItemPtr( LIST_BOOKMARK, ( TCHAR* )lParam1 );
	item2 = param->ListGetItemPtr( LIST_BOOKMARK, ( TCHAR* )lParam2 );
	if ( item1 != NULL && item2 != NULL ) {
		switch ( lParamSort ) {
		case 1:	// sort by JID column
			res = lstrcmp( item1->jid, item2->jid );
			break;
		case 0: // sort by Name column
			res = lstrcmp( item1->name, item2->name );
			break;
		case 2:
			res = lstrcmp( item1->nick, item2->nick );
			break;
	}	}

	if ( !sortAscending )
		res *= -1;

	return res;
}

int JabberBookmarksDlgResizer(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL *urc)
{
	switch ( urc->wId ) {
		case IDC_WHITERECT:
		case IDC_TITLE:
		case IDC_DESCRIPTION:
		case IDC_FRAME1:
			return RD_ANCHORX_LEFT|RD_ANCHORY_TOP|RD_ANCHORX_WIDTH;

		case IDC_BM_LIST:
			return RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;

		case IDCLOSE:
			return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;

		case IDC_ADD:
		case IDC_EDIT:
		case IDC_REMOVE:
			return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
	}
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
}

static void sttOpenBookmarkItem(CJabberProto* ppro, HWND hwndDlg)
{
	TCHAR room[ 512 ], *server, *p;
	TCHAR text[ 512 ];
	LVITEM lvItem={0};
	HWND lv = GetDlgItem( hwndDlg, IDC_BM_LIST);

	if (( lvItem.iItem=ListView_GetNextItem( lv, -1, LVNI_SELECTED )) >= 0 ) {

		lvItem.iSubItem = 0;
		lvItem.mask = LVIF_PARAM;
		ListView_GetItem( lv, &lvItem );

		if (lvItem.lParam)
		{
			JABBER_LIST_ITEM *item = ppro->ListGetItemPtr(LIST_BOOKMARK, ( TCHAR* )lvItem.lParam);

			if(!lstrcmpi(item->type, _T("conference") )){
				if ( ppro->m_bChatDllPresent ) {
					_tcsncpy( text, ( TCHAR* )lvItem.lParam, SIZEOF( text ));
					_tcsncpy( room, text, SIZEOF( room ));

					p = _tcstok( room, _T( "@" ));
					server = _tcstok( NULL, _T( "@" ));

					lvItem.iSubItem = 2;
					lvItem.mask = LVIF_TEXT;
					lvItem.cchTextMax = SIZEOF(text);
					lvItem.pszText = text;

					ListView_GetItem( lv, &lvItem );

					ListView_SetItemState( lv, lvItem.iItem, 0, LVIS_SELECTED ); // Unselect the item
					/* some hack for using bookmark to transport not under XEP-0048 */
					if (!server) {	//the room name is not provided let consider that it is transport and send request to registration
						ppro->RegisterAgent( NULL, room );
					}
					else {
						if ( text[0] != _T('\0') )
							ppro->GroupchatJoinRoom( server, p, text, item->password );
						else
						{
							TCHAR* nick = JabberNickFromJID( ppro->m_szJabberJID );
							ppro->GroupchatJoinRoom( server, p, nick, item->password );
							mir_free( nick );
						}
					}
				}
				else JabberChatDllError();
			}
			else {
				char* szUrl = mir_t2a( (TCHAR*)lvItem.lParam );
				JCallService( MS_UTILS_OPENURL, 1, (LPARAM)szUrl );
				mir_free( szUrl );
			}
		}
	}
}

static BOOL CALLBACK JabberBookmarksDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	CJabberProto* ppro = (CJabberProto*)GetWindowLong( hwndDlg, GWL_USERDATA );
	HWND lv;
	LVCOLUMN lvCol;
	LVITEM lvItem;
	JABBER_LIST_ITEM *item;
	HIMAGELIST hIml;    // A handle to the image list.

	switch ( msg ) {
	case WM_INITDIALOG:
	{
		ppro = (CJabberProto*)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG )ppro );

		TranslateDialogDefault( hwndDlg );
		SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )ppro->LoadIconEx( "bookmarks" ));
		ppro->m_hwndJabberBookmarks = hwndDlg;

		//EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), FALSE );
		EnableWindow( GetDlgItem( hwndDlg, IDC_ADD ), FALSE );
		EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT ), FALSE );
		EnableWindow( GetDlgItem( hwndDlg, IDC_REMOVE ), FALSE );
		sortColumn = -1;

		lv = GetDlgItem( hwndDlg, IDC_BM_LIST );

		ListView_SetExtendedListViewStyle(lv, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP );
		hIml = ImageList_Create(16, 16,	 IsWinVerXPPlus() ? ILC_COLOR32 | ILC_MASK : ILC_COLOR16 | ILC_MASK, 2, 1);
		if (hIml) {
			ImageList_AddIcon(hIml, (HICON)JCallService(MS_SKIN_LOADPROTOICON,(WPARAM)ppro->m_szProtoName,(LPARAM)ID_STATUS_ONLINE) );
			ImageList_AddIcon(hIml, (HICON)JCallService(MS_SKIN_LOADICON, SKINICON_EVENT_URL ,0));
			ListView_SetImageList (lv, hIml, LVSIL_SMALL);
		}

		// Add columns
		lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		lvCol.pszText = TranslateT( "Bookmark Name" );
		lvCol.cx = DBGetContactSettingWord( NULL, ppro->m_szProtoName, "bookmarksWnd_cx0", 120 );
		lvCol.iSubItem = 0;
		ListView_InsertColumn( lv, 0, &lvCol );

		lvCol.pszText = TranslateT( "Room JID / URL" );
		lvCol.cx = DBGetContactSettingWord( NULL, ppro->m_szProtoName, "bookmarksWnd_cx1", 210 );
		lvCol.iSubItem = 1;
		ListView_InsertColumn( lv, 1, &lvCol );

		lvCol.pszText = TranslateT( "Nick" );
		lvCol.cx = DBGetContactSettingWord( NULL, ppro->m_szProtoName, "bookmarksWnd_cx2", 90 );
		lvCol.iSubItem = 2;
		ListView_InsertColumn( lv, 2, &lvCol );

#ifdef UNICODE
		if (IsWinVerXPPlus())
		{
			ListView_EnableGroupView(lv, TRUE);

			LVGROUP group = {0};
			group.cbSize = sizeof(group);
			group.mask = LVGF_HEADER|LVGF_GROUPID;

			group.pszHeader = TranslateT("Conferences");
			group.cchHeader = lstrlen(group.pszHeader);
			group.iGroupId = 0;
			ListView_InsertGroup(lv, -1, &group);

			group.pszHeader = TranslateT("Links");
			group.cchHeader = lstrlen(group.pszHeader);
			group.iGroupId = 1;
			ListView_InsertGroup(lv, -1, &group);
		}
#endif

		if ( ppro->m_bJabberOnline ) {
			if ( !( ppro->m_ThreadInfo->bBookmarksLoaded )) {
				int iqId = ppro->SerialNext();
				ppro->IqAdd( iqId, IQ_PROC_DISCOBOOKMARKS, &CJabberProto::OnIqResultDiscoBookmarks);

				XmlNodeIq iq( "get", iqId);
				XmlNode* query = iq.addQuery( JABBER_FEAT_PRIVATE_STORAGE );
				XmlNode* storage = query->addChild( "storage" );
				storage->addAttr( "xmlns", "storage:bookmarks" );

				ppro->m_ThreadInfo->send( iq );
			}
			else SendMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0);
		}

		static struct
		{
			int idc;
			TCHAR *title;
			char *icon;
			int coreIcon;
			bool push;
		} buttons[] =
		{
			{IDC_ADD,		_T("Add"),		NULL, SKINICON_OTHER_ADDCONTACT,	false},
			{IDC_EDIT,		_T("Edit"),		NULL, SKINICON_OTHER_RENAME,		false},
			{IDC_REMOVE,	_T("Remove"),	NULL, SKINICON_OTHER_DELETE,		false},
		};
		for (int i = 0; i < SIZEOF(buttons); ++i)
		{
			SendDlgItemMessage(hwndDlg, buttons[i].idc, BM_SETIMAGE, IMAGE_ICON,
				(LPARAM)(buttons[i].icon ?
					ppro->LoadIconEx(buttons[i].icon) :
					LoadSkinnedIcon(buttons[i].coreIcon)));
			SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONSETASFLATBTN, 0, 0);
			SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONADDTOOLTIP, (WPARAM)TranslateTS(buttons[i].title), BATF_TCHAR);
			if (buttons[i].push)
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONSETASPUSHBTN, 0, 0);
		}

		LOGFONT lf;
		GetObject((HFONT)SendDlgItemMessage(hwndDlg, IDC_TITLE, WM_GETFONT, 0, 0), sizeof(lf), &lf);
		lf.lfWeight = FW_BOLD;
		HFONT hfnt = CreateFontIndirect(&lf);
		SendDlgItemMessage(hwndDlg, IDC_TITLE, WM_SETFONT, (WPARAM)hfnt, TRUE);

		Utils_RestoreWindowPosition( hwndDlg, NULL, ppro->m_szProtoName, "bookmarksWnd_" );
		return TRUE;
	}

	case WM_CTLCOLORSTATIC:
		if ( ((HWND)lParam == GetDlgItem(hwndDlg, IDC_WHITERECT)) ||
			 ((HWND)lParam == GetDlgItem(hwndDlg, IDC_TITLE)) ||
			 ((HWND)lParam == GetDlgItem(hwndDlg, IDC_DESCRIPTION)) )
			return (BOOL)GetStockObject(WHITE_BRUSH);
		return FALSE;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
			lpmmi->ptMinTrackSize.x = 451;
			lpmmi->ptMinTrackSize.y = 320;
			return 0;
		}

	case WM_SIZE:
		{
			UTILRESIZEDIALOG urd;
			urd.cbSize = sizeof(urd);
			urd.hwndDlg = hwndDlg;
			urd.hInstance = hInst;
			urd.lpTemplate = MAKEINTRESOURCEA( IDD_BOOKMARKS );
			urd.lParam = 0;
			urd.pfnResizer = JabberBookmarksDlgResizer;
			CallService( MS_UTILS_RESIZEDIALOG, 0, (LPARAM)&urd );
			return TRUE;
		}

	case WM_JABBER_ACTIVATE:
		ListView_DeleteAllItems( GetDlgItem( hwndDlg, IDC_BM_LIST));
		//EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), FALSE );
		EnableWindow( GetDlgItem( hwndDlg, IDC_ADD ), FALSE );
		EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT ), FALSE );
		EnableWindow( GetDlgItem( hwndDlg, IDC_REMOVE ), FALSE );
		return TRUE;

	case WM_JABBER_REFRESH:
		{
			lv = GetDlgItem( hwndDlg, IDC_BM_LIST);
			ListView_DeleteAllItems( lv );

			LVITEM lvItem;
			lvItem.iItem = 0;
			for ( int i=0; ( i = ppro->ListFindNext( LIST_BOOKMARK, i )) >= 0; i++ ) {
				if (( item = ppro->ListGetItemPtrFromIndex( i )) != NULL ) {
					TCHAR szBuffer[256];
					lvItem.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
					lvItem.iSubItem = 0;

					if (IsWinVerXPPlus()) lvItem.mask |= LVIF_GROUPID;

					if(!lstrcmpi(item->type, _T("conference") )) lvItem.iGroupId = lvItem.iImage = 0;
					else lvItem.iGroupId = lvItem.iImage = 1;

					lvItem.lParam = ( LPARAM )item->jid;
					lvItem.pszText = item->name;
					ListView_InsertItem( lv, &lvItem );

					lvItem.mask = LVIF_TEXT;
					_tcsncpy( szBuffer, item->jid, SIZEOF(szBuffer));
					szBuffer[ SIZEOF(szBuffer)-1 ] = 0;
					lvItem.iSubItem = 1;
					lvItem.pszText = szBuffer;
					ListView_SetItem( lv, &lvItem );

					if(!lstrcmpi(item->type, _T("conference") )){

						if(item->nick){lvItem.mask = LVIF_TEXT;
						lvItem.iSubItem = 2;
						_tcsncpy( szBuffer, item->nick, SIZEOF(szBuffer));
						szBuffer[ SIZEOF(szBuffer)-1 ] = 0;
						lvItem.pszText = szBuffer;}
						ListView_SetItem( lv, &lvItem );
					}

					lvItem.iItem++;
			}	}

			if ( lvItem.iItem > 0 ) {
				EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT ), TRUE );
				EnableWindow( GetDlgItem( hwndDlg, IDC_REMOVE ), TRUE );
			}
			//EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), TRUE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_ADD ), TRUE );
		}
		return TRUE;

	case WM_JABBER_CHECK_ONLINE:
		if ( !ppro->m_bJabberOnline ) {
			//EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_ADD ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_REMOVE ), FALSE );

			lv = GetDlgItem( hwndDlg, IDC_BM_LIST);
			ListView_DeleteAllItems( lv );
		}
		else //EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), TRUE );
			SendMessage (hwndDlg, WM_COMMAND, IDC_BROWSE, 0);
		break;

	case WM_NOTIFY:
		switch ( wParam ) {
		case IDC_BM_LIST:
			switch (( ( LPNMHDR )lParam )->code ) {
			case LVN_COLUMNCLICK:
				{
					LPNMLISTVIEW pnmlv = ( LPNMLISTVIEW ) lParam;

					if ( pnmlv->iSubItem>=0 && pnmlv->iSubItem<=2 ) {
						if ( pnmlv->iSubItem == sortColumn )
							sortAscending = !sortAscending;
						else {
							sortAscending = TRUE;
							sortColumn = pnmlv->iSubItem;
						}
						ListView_SortItems( GetDlgItem( hwndDlg, IDC_BM_LIST), BookmarkCompare, sortColumn );
					}
				}
				break;
			case LVN_ITEMACTIVATE:
				sttOpenBookmarkItem(ppro, hwndDlg);
				return TRUE;
			}
			break;
		}
		break;
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_BROWSE:
			if ( ppro->m_bJabberOnline) {
				//EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), FALSE );
				EnableWindow( GetDlgItem( hwndDlg, IDC_ADD ), FALSE );
				EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT ), FALSE );
				EnableWindow( GetDlgItem( hwndDlg, IDC_REMOVE ), FALSE );

				ListView_DeleteAllItems( GetDlgItem( hwndDlg, IDC_BM_LIST));

				int iqId = ppro->SerialNext();
				ppro->IqAdd( iqId, IQ_PROC_DISCOBOOKMARKS, &CJabberProto::OnIqResultDiscoBookmarks);

				XmlNodeIq iq( "get", iqId);

				XmlNode* query = iq.addQuery( JABBER_FEAT_PRIVATE_STORAGE );
				XmlNode* storage = query->addChild("storage");
				storage->addAttr("xmlns","storage:bookmarks");

				ppro->m_ThreadInfo->send( iq );
			}
			return TRUE;

		case IDOK:
			sttOpenBookmarkItem(ppro, hwndDlg);
			return TRUE;

		case IDC_ADD:
			if ( ppro->m_bJabberOnline) {
				JabberAddBookmarkDlgParam param;
				param.ppro = ppro;
				param.m_item = NULL;
				DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_BOOKMARK_ADD ), hwndDlg, JabberAddBookmarkDlgProc, (LPARAM)&param );
			}
			return TRUE;

		case IDC_EDIT:
			if ( ppro->m_bJabberOnline) {
				lv = GetDlgItem( hwndDlg, IDC_BM_LIST);
				if (( lvItem.iItem=ListView_GetNextItem( lv, -1, LVNI_SELECTED )) >= 0 ) {
					lvItem.iSubItem = 0;
					lvItem.mask = LVIF_PARAM;
					ListView_GetItem( lv, &lvItem );

					item = ppro->ListGetItemPtr(LIST_BOOKMARK, ( TCHAR* )lvItem.lParam);

					if (item) {
						JabberAddBookmarkDlgParam param;
						param.ppro = ppro;
						param.m_item = item;
						DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_BOOKMARK_ADD ), hwndDlg, JabberAddBookmarkDlgProc, (LPARAM)&param );
					}

					ListView_SetItemState( lv, lvItem.iItem, 0, LVIS_SELECTED ); // Unselect the item
			}	}
			return TRUE;

		case IDC_REMOVE:
			if ( ppro->m_bJabberOnline) {
				lv = GetDlgItem( hwndDlg, IDC_BM_LIST);
				if (( lvItem.iItem=ListView_GetNextItem( lv, -1, LVNI_SELECTED )) >= 0 ) {

					//EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), FALSE );
					EnableWindow( GetDlgItem( hwndDlg, IDC_ADD ), FALSE );
					EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT ), FALSE );
					EnableWindow( GetDlgItem( hwndDlg, IDC_REMOVE ), FALSE );

					lvItem.iSubItem = 0;
					lvItem.mask = LVIF_PARAM;
					ListView_GetItem( lv, &lvItem );

					ppro->ListRemove(LIST_BOOKMARK, ( TCHAR* )lvItem.lParam);

					ListView_SetItemState( lv, lvItem.iItem, 0, LVIS_SELECTED ); // Unselect the item

					int iqId = ppro->SerialNext();
					ppro->IqAdd( iqId, IQ_PROC_SETBOOKMARKS, &CJabberProto::OnIqResultSetBookmarks);

					XmlNodeIq iq( "set", iqId );
					ppro->SetBookmarkRequest( iq );
					ppro->m_ThreadInfo->send( iq );
			}	}
			return TRUE;

		case IDCLOSE:
		case IDCANCEL:
			PostMessage( hwndDlg, WM_CLOSE, 0, 0 );
			return TRUE;
		}
		break;

	case WM_CLOSE:
		{
			HWND hwndList = GetDlgItem( hwndDlg, IDC_BM_LIST );
			LVCOLUMN lvc = { 0 };
			lvc.mask = LVCF_WIDTH;
			ListView_GetColumn( hwndList, 0, &lvc );
			DBWriteContactSettingWord( NULL, ppro->m_szProtoName, "bookmarksWnd_cx0", lvc.cx );
			ListView_GetColumn( hwndList, 1, &lvc );
			DBWriteContactSettingWord( NULL, ppro->m_szProtoName, "bookmarksWnd_cx1", lvc.cx );
			ListView_GetColumn( hwndList, 2, &lvc );
			DBWriteContactSettingWord( NULL, ppro->m_szProtoName, "bookmarksWnd_cx2", lvc.cx );

			Utils_SaveWindowPosition( hwndDlg, NULL, ppro->m_szProtoName, "bookmarksWnd_" );
			DestroyWindow( hwndDlg );
			break;
		}

	case WM_DESTROY:
		DeleteObject((HFONT)SendDlgItemMessage(hwndDlg, IDC_TITLE, WM_GETFONT, 0, 0));
		ppro->m_hwndJabberBookmarks = NULL;
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Launches the Bookmarks manager window

int __cdecl CJabberProto::OnMenuHandleBookmarks( WPARAM wParam, LPARAM lParam )
{
	if ( IsWindow( m_hwndJabberBookmarks )) {
		SetForegroundWindow( m_hwndJabberBookmarks );

		SendMessage( m_hwndJabberBookmarks, WM_JABBER_ACTIVATE, 0, 0 );	// Just to clear the list
		int iqId = SerialNext();
		IqAdd( iqId, IQ_PROC_DISCOBOOKMARKS, &CJabberProto::OnIqResultDiscoBookmarks);

		XmlNodeIq iq( "get", iqId);

		XmlNode* query = iq.addQuery( "jabber:iq:private" );
		XmlNode* storage = query->addChild("storage");
		storage->addAttr("xmlns","storage:bookmarks");

		// <iq/> result will send WM_JABBER_REFRESH to update the list with real data
		m_ThreadInfo->send( iq );
	}
	else CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_BOOKMARKS ), NULL, JabberBookmarksDlgProc, (LPARAM)this );

	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Launches the Bookmark details window, lParam is JABBER_BOOKMARK_ITEM*
int CJabberProto::AddEditBookmark( JABBER_LIST_ITEM* item )
{
	if ( m_bJabberOnline) {
		JabberAddBookmarkDlgParam param;
		param.ppro = this;
		param.m_item = item;//(JABBER_LIST_ITEM*)lParam;
		DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_BOOKMARK_ADD ), NULL, JabberAddBookmarkDlgProc, (LPARAM)&param );
	}
	return 0;
}
