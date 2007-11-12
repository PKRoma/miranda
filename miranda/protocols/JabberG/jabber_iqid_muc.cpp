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

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_iqid_muc.cpp,v $
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "resource.h"
#include "jabber_list.h"
#include "jabber_iq.h"
#include <commctrl.h>
#include "jabber_caps.h"

void JabberAddMucListItem( JABBER_MUC_JIDLIST_INFO* jidListInfo, TCHAR* str );
void JabberDeleteMucListItem( JABBER_MUC_JIDLIST_INFO* jidListInfo, TCHAR* str );
BOOL JabberEnterString( TCHAR* result, size_t resultLen );

void JabberIqResultBrowseRooms( XmlNode *iqNode, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;
	XmlNode *confNode, *roomNode;
	TCHAR* type, *category, *jid, *str;
	JABBER_LIST_ITEM *item;
	int i, j;

	// RECVED: room list
	// ACTION: refresh groupchat room list dialog
	JabberLog( "<iq/> iqIdBrowseRooms" );
	if (( type=JabberXmlGetAttrValue( iqNode, "type" )) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		JabberListRemoveList( LIST_ROOM );
		for ( i=0; i<iqNode->numChild; i++ ) {
			if (( confNode=iqNode->child[i] )!=NULL && confNode->name!=NULL ) {
				if ( !strcmp( confNode->name, "item" )) {
					if (( category=JabberXmlGetAttrValue( confNode, "category" ))!=NULL && !lstrcmp( category, _T("conference"))) {
						for ( j=0; j<confNode->numChild; j++ ) {
							if (( roomNode=confNode->child[j] )!=NULL && !strcmp( roomNode->name, "item" )) {
								if (( category=JabberXmlGetAttrValue( roomNode, "category" ))!=NULL && !lstrcmp( category, _T("conference"))) {
									if (( jid=JabberXmlGetAttrValue( roomNode, "jid" )) != NULL ) {
										item = JabberListAdd( LIST_ROOM, jid );
										if (( str=JabberXmlGetAttrValue( roomNode, "name" )) != NULL )
											item->name = mir_tstrdup( str );
										if (( str=JabberXmlGetAttrValue( roomNode, "type" )) != NULL )
											item->type = mir_tstrdup( str );
					}	}	}	}	}
				}
				else if ( !strcmp( confNode->name, "conference" )) {
					for ( j=0; j<confNode->numChild; j++ ) {
						if (( roomNode=confNode->child[j] )!=NULL && !strcmp( roomNode->name, "conference" )) {
							if (( jid=JabberXmlGetAttrValue( roomNode, "jid" )) != NULL ) {
								item = JabberListAdd( LIST_ROOM, jid );
								if (( str=JabberXmlGetAttrValue( roomNode, "name" )) != NULL )
									item->name = mir_tstrdup( str );
								if (( str=JabberXmlGetAttrValue( roomNode, "type" )) != NULL )
									item->type = mir_tstrdup( str );
		}	}	}	}	}	}

		if ( hwndJabberGroupchat != NULL ) {
			if (( jid=JabberXmlGetAttrValue( iqNode, "from" )) != NULL )
				SendMessage( hwndJabberGroupchat, WM_JABBER_REFRESH, 0, ( LPARAM )jid );
			else
				SendMessage( hwndJabberGroupchat, WM_JABBER_REFRESH, 0, ( LPARAM )info->server );
		}
	}
}

void JabberSetMucConfig( XmlNode* node, void *from )
{
	if ( jabberThreadInfo && from ) {
		XmlNodeIq iq( "set", NOID, ( TCHAR* )from );
		XmlNode* query = iq.addQuery( xmlnsOwner );
		query->addChild( node );
		jabberThreadInfo->send( iq );
}	}

void JabberIqResultGetMuc( XmlNode *iqNode, void *userdata )
{
	XmlNode *queryNode, *xNode;
	TCHAR *type, *from, *str;

	// RECVED: room config form
	// ACTION: show the form
	JabberLog( "<iq/> iqIdGetMuc" );
	if (( type=JabberXmlGetAttrValue( iqNode, "type" )) == NULL ) return;
	if (( from=JabberXmlGetAttrValue( iqNode, "from" )) == NULL ) return;

	if ( !_tcscmp( type, _T("result"))) {
		if (( queryNode=JabberXmlGetChild( iqNode, "query" )) != NULL ) {
			str = JabberXmlGetAttrValue( queryNode, "xmlns" );
			if ( !lstrcmp( str, _T("http://jabber.org/protocol/muc#owner" ))) {
				if (( xNode=JabberXmlGetChild( queryNode, "x" )) != NULL ) {
					str = JabberXmlGetAttrValue( xNode, "xmlns" );
					if ( !lstrcmp( str, _T(JABBER_FEAT_DATA_FORMS)))
						JabberFormCreateDialog( xNode, _T("Jabber Conference Room Configuration"), JabberSetMucConfig, mir_tstrdup( from ));
}	}	}	}	}

void JabberIqResultDiscoRoomItems( XmlNode *iqNode, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;
	XmlNode *queryNode, *itemNode;
	TCHAR* type, *jid, *from;
	JABBER_LIST_ITEM *item;
	int i;
	int iqId;

	// RECVED: room list
	// ACTION: refresh groupchat room list dialog
	JabberLog( "<iq/> iqIdDiscoRoomItems" );
	if (( type=JabberXmlGetAttrValue( iqNode, "type" )) == NULL ) return;
	if (( from=JabberXmlGetAttrValue( iqNode, "from" )) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		if (( queryNode=JabberXmlGetChild( iqNode, "query" )) != NULL ) {
			JabberListRemoveList( LIST_ROOM );
			for ( i=0; i<queryNode->numChild; i++ ) {
				if (( itemNode=queryNode->child[i] )!=NULL && itemNode->name!=NULL && !strcmp( itemNode->name, "item" )) {
					if (( jid=JabberXmlGetAttrValue( itemNode, "jid" )) != NULL ) {
						item = JabberListAdd( LIST_ROOM, jid );
						item->name = mir_tstrdup( JabberXmlGetAttrValue( itemNode, "name" ));
		}	}	}	}

		if ( hwndJabberGroupchat != NULL ) {
			if (( jid=JabberXmlGetAttrValue( iqNode, "from" )) != NULL )
				SendMessage( hwndJabberGroupchat, WM_JABBER_REFRESH, 0, ( LPARAM )jid );
			else
				SendMessage( hwndJabberGroupchat, WM_JABBER_REFRESH, 0, ( LPARAM )info->server );
		}
	}
	else if ( !_tcscmp( type, _T("error"))) {
		// disco is not supported, try browse
		iqId = JabberSerialNext();
		JabberIqAdd( iqId, IQ_PROC_BROWSEROOMS, JabberIqResultBrowseRooms );

		XmlNodeIq iq( "get", iqId, from );
		XmlNode* query = iq.addQuery( JABBER_FEAT_BROWSE );
		jabberThreadInfo->send( iq );
}	}

static void sttFillJidList(HWND hwndDlg)
{
	JABBER_MUC_JIDLIST_INFO *jidListInfo;
	XmlNode *iqNode, *queryNode, *itemNode;
	TCHAR* from, *jid, *reason, *nick;
	LVITEM lvi;
	HWND hwndList;
	int count, i;

	TCHAR *filter = NULL;
	if (GetWindowLong(GetDlgItem(hwndDlg, IDC_FILTER), GWL_USERDATA))
	{
		int filterLength = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_FILTER)) + 1;
		filter = (TCHAR *)_alloca(filterLength * sizeof(TCHAR));
		GetDlgItemText(hwndDlg, IDC_FILTER, filter, filterLength);
	}

	jidListInfo = ( JABBER_MUC_JIDLIST_INFO * ) GetWindowLong( hwndDlg, GWL_USERDATA );
	if (!jidListInfo) return;

	hwndList = GetDlgItem( hwndDlg, IDC_LIST );
	SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);

	count = ListView_GetItemCount( hwndList );
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;
	for ( i=0; i<count; i++ ) {
		lvi.iItem = i;
		if ( ListView_GetItem( hwndList, &lvi ) == TRUE ) {
			if ( lvi.lParam!=( LPARAM )( -1 ) && lvi.lParam!=( LPARAM )( NULL )) {
				mir_free(( void * ) lvi.lParam );
			}
		}
	}
	ListView_DeleteAllItems( hwndList );

	// Populate displayed list from iqNode
	if (( iqNode = jidListInfo->iqNode ) != NULL ) {
		if (( from = JabberXmlGetAttrValue( iqNode, "from" )) != NULL ) {
			if (( queryNode=JabberXmlGetChild( iqNode, "query" )) != NULL ) {
				lvi.mask = LVIF_TEXT | LVIF_PARAM;
				lvi.iSubItem = 0;
				lvi.iItem = 0;
				for ( i=0; i<queryNode->numChild; i++ ) {
					if (( itemNode=queryNode->child[i] ) != NULL ) {
						if (( jid=JabberXmlGetAttrValue( itemNode, "jid" )) != NULL ) {
							lvi.pszText = jid;
							if ( jidListInfo->type == MUC_BANLIST ) {										
								if (( reason = JabberXmlGetChild( itemNode, "reason" )->text ) != NULL ) {
									TCHAR jidreason[ 200 ];
									mir_sntprintf( jidreason, SIZEOF( jidreason ), _T("%s (%s)") , jid, reason );
									lvi.pszText = jidreason;
							}	}

							if ( jidListInfo->type == MUC_VOICELIST || jidListInfo->type == MUC_MODERATORLIST ) {										
								if (( nick = JabberXmlGetAttrValue( itemNode, "nick" )) != NULL ) {
									TCHAR nickjid[ 200 ];
									mir_sntprintf( nickjid, SIZEOF( nickjid ), _T("%s (%s)") , nick, jid );
									lvi.pszText = nickjid;
							}	}

							if (filter && *filter && !_tcsstr(lvi.pszText, filter))
								continue;

							lvi.lParam = ( LPARAM )mir_tstrdup( jid );

							ListView_InsertItem( hwndList, &lvi );
							lvi.iItem++;
	}	}	}	}	}	}

	lvi.mask = LVIF_PARAM;
	lvi.lParam = ( LPARAM )( -1 );
	ListView_InsertItem( hwndList, &lvi );

	SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);
	RedrawWindow(hwndList, NULL, NULL, RDW_INVALIDATE);
}

static int sttJidListResizer(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL *urc)
{
	switch (urc->wId)
	{
	case IDC_LIST:
		return RD_ANCHORX_LEFT|RD_ANCHORY_TOP|RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;
	case IDC_FILTER:
		return RD_ANCHORX_LEFT|RD_ANCHORY_BOTTOM|RD_ANCHORX_WIDTH;
	case IDC_BTN_FILTERRESET:
	case IDC_BTN_FILTERAPPLY:
		return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
	}
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
}

static BOOL CALLBACK JabberMucJidListDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) {
	case WM_INITDIALOG:
		{
			// lParam is ( JABBER_MUC_JIDLIST_INFO * )
			LVCOLUMN lvc;
			RECT rc;
			HWND hwndList;

			TranslateDialogDefault( hwndDlg );
			hwndList = GetDlgItem( hwndDlg, IDC_LIST );
			GetClientRect( hwndList, &rc );
			rc.right -= GetSystemMetrics( SM_CXVSCROLL );
			lvc.mask = LVCF_WIDTH;
			lvc.cx = rc.right - 20;
			ListView_InsertColumn( hwndList, 0, &lvc );
			lvc.cx = 20;
			ListView_InsertColumn( hwndList, 1, &lvc );
			SendMessage( hwndDlg, WM_JABBER_REFRESH, 0, lParam );

			static struct
			{
				int idc;
				TCHAR *title;
				char *icon;
				bool push;
			} buttons[] =
			{
				{IDC_BTN_FILTERAPPLY,	_T("Apply filter"),		"sd_filter_apply",	false},
				{IDC_BTN_FILTERRESET,	_T("Reset filter"),		"sd_filter_reset",	false},
			};
			for (int i = 0; i < SIZEOF(buttons); ++i)
			{
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIconEx(buttons[i].icon));
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONSETASFLATBTN, 0, 0);
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONADDTOOLTIP, (WPARAM)TranslateTS(buttons[i].title), BATF_TCHAR);
				if (buttons[i].push)
					SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONSETASPUSHBTN, 0, 0);
			}

			Utils_RestoreWindowPosition(hwndDlg, NULL, jabberProtoName, "jidListWnd_");
		}
		return TRUE;
	case WM_SIZE:
		{
			UTILRESIZEDIALOG urd = {0};
			urd.cbSize = sizeof(urd);
			urd.hInstance = hInst;
			urd.hwndDlg = hwndDlg;
			urd.lpTemplate = MAKEINTRESOURCEA(IDD_JIDLIST);
			urd.pfnResizer = sttJidListResizer;
			CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM)&urd);

			RECT listrc;
			LVCOLUMN lvc;
			HWND hwndList = GetDlgItem( hwndDlg, IDC_LIST );
			GetClientRect( hwndList, &listrc );
			lvc.mask = LVCF_WIDTH;
			listrc.right -= GetSystemMetrics( SM_CXVSCROLL );
			lvc.cx = listrc.right - 20;
			SendMessage(hwndList, LVM_SETCOLUMN, 0, (LPARAM)&lvc);
			break;
		}
		break;

	case WM_JABBER_REFRESH:
		{
			// lParam is ( JABBER_MUC_JIDLIST_INFO * )
			JABBER_MUC_JIDLIST_INFO *jidListInfo;
			XmlNode *iqNode, *queryNode;
			TCHAR* from, *localFrom;
			TCHAR title[256];

			// Clear current GWL_USERDATA, if any
			jidListInfo = ( JABBER_MUC_JIDLIST_INFO * ) GetWindowLong( hwndDlg, GWL_USERDATA );
			if ( jidListInfo != NULL ) {
				if ( jidListInfo->roomJid != NULL )
					mir_free( jidListInfo->roomJid );
				if ( jidListInfo->iqNode != NULL )
					delete jidListInfo->iqNode;
				mir_free( jidListInfo );
			}

			// Set new GWL_USERDATA
			jidListInfo = ( JABBER_MUC_JIDLIST_INFO * ) lParam;
			SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG ) jidListInfo );

			// Populate displayed list from iqNode
			lstrcpyn( title, TranslateT( "JID List" ), SIZEOF( title ));
			if (( jidListInfo=( JABBER_MUC_JIDLIST_INFO * ) lParam ) != NULL ) {
				if (( iqNode = jidListInfo->iqNode ) != NULL ) {
					if (( from = JabberXmlGetAttrValue( iqNode, "from" )) != NULL ) {
						jidListInfo->roomJid = mir_tstrdup( from );
						
						if (( queryNode=JabberXmlGetChild( iqNode, "query" )) != NULL ) {
							localFrom = mir_tstrdup( from );
							mir_sntprintf( title, SIZEOF( title ), _T("%s, %d items (%s)"),
								( jidListInfo->type==MUC_VOICELIST ) ? TranslateT( "Voice List" ) :
								( jidListInfo->type==MUC_MEMBERLIST ) ? TranslateT( "Member List" ) :
								( jidListInfo->type==MUC_MODERATORLIST ) ? TranslateT( "Moderator List" ) :
								( jidListInfo->type==MUC_BANLIST ) ? TranslateT( "Ban List" ) :
								( jidListInfo->type==MUC_ADMINLIST ) ? TranslateT( "Admin List" ) :
								( jidListInfo->type==MUC_OWNERLIST ) ? TranslateT( "Owner List" ) :
							TranslateT( "JID List" ), queryNode->numChild,
								localFrom );
							mir_free( localFrom );
			}	}	}	}
			SetWindowText( hwndDlg, title );

			SetWindowLong(GetDlgItem(hwndDlg, IDC_FILTER), GWL_USERDATA, 0);
			sttFillJidList(hwndDlg);
		}
		break;
	case WM_NOTIFY:
		if (( ( LPNMHDR )lParam )->idFrom == IDC_LIST ) {
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
							if ( nm->iSubItem == 1 ) {
								if( nm->nmcd.lItemlParam == ( LPARAM )( -1 ))
									hIcon = ( HICON )LoadImage( hInst, MAKEINTRESOURCE( IDI_ADDCONTACT ), IMAGE_ICON, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );
								else
									hIcon = ( HICON )LoadImage( hInst, MAKEINTRESOURCE( IDI_DELETE ), IMAGE_ICON, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );
								DrawIconEx( nm->nmcd.hdc, ( rc.left+rc.right-GetSystemMetrics( SM_CXSMICON ))/2, ( rc.top+rc.bottom-GetSystemMetrics( SM_CYSMICON ))/2,hIcon, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0, GetSysColorBrush(COLOR_WINDOW), DI_NORMAL );
								DestroyIcon( hIcon );
								SetWindowLong( hwndDlg, DWL_MSGRESULT, CDRF_SKIPDEFAULT );
								return TRUE;
				}	}	}	}
				break;
			case NM_CLICK:
				{
					JABBER_MUC_JIDLIST_INFO *jidListInfo;
					NMLISTVIEW *nm = ( NMLISTVIEW * ) lParam;
					LVITEM lvi;
					LVHITTESTINFO hti;
					TCHAR text[128];

					if ( nm->iSubItem < 1 ) break;
					jidListInfo = ( JABBER_MUC_JIDLIST_INFO * ) GetWindowLong( hwndDlg, GWL_USERDATA );
					hti.pt.x = ( short ) LOWORD( GetMessagePos());
					hti.pt.y = ( short ) HIWORD( GetMessagePos());
					ScreenToClient( nm->hdr.hwndFrom, &hti.pt );
					if ( ListView_SubItemHitTest( nm->hdr.hwndFrom, &hti ) == -1 )
						break;

					if ( hti.iSubItem != 1 )
						break;

					lvi.mask = LVIF_PARAM | LVIF_TEXT;
					lvi.iItem = hti.iItem;
					lvi.iSubItem = 0;
					lvi.pszText = text;
					lvi.cchTextMax = sizeof( text );
					ListView_GetItem( nm->hdr.hwndFrom, &lvi );
					if ( lvi.lParam == ( LPARAM )( -1 )) {
						TCHAR szBuffer[ 1024 ];
						_tcscpy( szBuffer, jidListInfo->type2str());
						if ( !JabberEnterString( szBuffer, SIZEOF(szBuffer)))
							break;

						// Trim leading and trailing whitespaces
						TCHAR *p = szBuffer, *q;
						for ( p = szBuffer; *p!='\0' && isspace( BYTE( *p )); p++);
						for ( q = p; *q!='\0' && !isspace( BYTE( *q )); q++);
						if (*q != '\0') *q = '\0';
						if (*p == '\0')
							break;

						JabberAddMucListItem( jidListInfo, p );
					}
					else {
						//delete
						TCHAR msgText[128];

						mir_sntprintf( msgText, SIZEOF( msgText ), _T("%s %s?"), TranslateT( "Removing" ), text );
						if ( MessageBox( hwndDlg, msgText, jidListInfo->type2str(), MB_YESNO|MB_SETFOREGROUND ) == IDYES ) {
							JabberDeleteMucListItem( jidListInfo, ( TCHAR* )lvi.lParam );
							mir_free(( void * )lvi.lParam );
							ListView_DeleteItem( nm->hdr.hwndFrom, hti.iItem );
				}	}	}
				break;
			}
			break;
		}
		break;
	case WM_COMMAND:
		if ((LOWORD(wParam) == IDC_BTN_FILTERAPPLY) ||
			((LOWORD(wParam) == IDOK) && (GetFocus() == GetDlgItem(hwndDlg, IDC_FILTER))))
		{
			SetWindowLong(GetDlgItem(hwndDlg, IDC_FILTER), GWL_USERDATA, 1);
			sttFillJidList(hwndDlg);
		} else
		if ((LOWORD(wParam) == IDC_BTN_FILTERRESET) ||
			((LOWORD(wParam) == IDCANCEL) && (GetFocus() == GetDlgItem(hwndDlg, IDC_FILTER))))
		{
			SetWindowLong(GetDlgItem(hwndDlg, IDC_FILTER), GWL_USERDATA, 0);
			sttFillJidList(hwndDlg);
		}
		break;
/*	case WM_SETCURSOR:
		if ( LOWORD( LPARAM )!= HTCLIENT ) break;
		if ( GetForegroundWindow() == GetParent( hwndDlg )) {
			POINT pt;
			GetCursorPos( &pt );
			ScreenToClient( hwndDlg,&pt );
			SetFocus( ChildWindowFromPoint( hwndDlg,pt ));	  //ugly hack because listviews ignore their first click
		}
		break;
*/	case WM_CLOSE:
		{
			JABBER_MUC_JIDLIST_INFO *jidListInfo;
			HWND hwndList;
			int count, i;
			LVITEM lvi;

			// Free lParam of the displayed list items
			hwndList = GetDlgItem( hwndDlg, IDC_LIST );
			count = ListView_GetItemCount( hwndList );
			lvi.mask = LVIF_PARAM;
			lvi.iSubItem = 0;
			for ( i=0; i<count; i++ ) {
				lvi.iItem = i;
				if ( ListView_GetItem( hwndList, &lvi ) == TRUE ) {
					if ( lvi.lParam!=( LPARAM )( -1 ) && lvi.lParam!=( LPARAM )( NULL )) {
						mir_free(( void * ) lvi.lParam );
					}
				}
			}
			ListView_DeleteAllItems( hwndList );

			jidListInfo = ( JABBER_MUC_JIDLIST_INFO * ) GetWindowLong( hwndDlg, GWL_USERDATA );
			switch ( jidListInfo->type ) {
			case MUC_VOICELIST:
				hwndMucVoiceList = NULL;
				break;
			case MUC_MEMBERLIST:
				hwndMucMemberList = NULL;
				break;
			case MUC_MODERATORLIST:
				hwndMucModeratorList = NULL;
				break;
			case MUC_BANLIST:
				hwndMucBanList = NULL;
				break;
			case MUC_ADMINLIST:
				hwndMucAdminList = NULL;
				break;
			case MUC_OWNERLIST:
				hwndMucOwnerList = NULL;
				break;
			}

			DestroyWindow( hwndDlg );
		}
		break;
	case WM_DESTROY:
		{
			JABBER_MUC_JIDLIST_INFO *jidListInfo;

			// Clear GWL_USERDATA
			jidListInfo = ( JABBER_MUC_JIDLIST_INFO * ) GetWindowLong( hwndDlg, GWL_USERDATA );
			if ( jidListInfo != NULL ) {
				if ( jidListInfo->iqNode != NULL )
					delete jidListInfo->iqNode;
				if ( jidListInfo->roomJid != NULL )
					mir_free( jidListInfo->roomJid );
				mir_free( jidListInfo );
			}

			Utils_SaveWindowPosition(hwndDlg, NULL, jabberProtoName, "jidListWnd_");
		}
		break;
	}
	return FALSE;
}

static VOID CALLBACK JabberMucJidListCreateDialogApcProc( DWORD param )
{
	XmlNode *iqNode, *queryNode;
	TCHAR* from;
	JABBER_MUC_JIDLIST_INFO *jidListInfo;
	HWND *pHwndJidList;

	if (( jidListInfo=( JABBER_MUC_JIDLIST_INFO * ) param ) == NULL ) return;
	if (( iqNode=jidListInfo->iqNode ) == NULL )                      return;
	if (( from=JabberXmlGetAttrValue( iqNode, "from" )) == NULL )     return;
	if (( queryNode=JabberXmlGetChild( iqNode, "query" )) == NULL )   return;

	switch ( jidListInfo->type ) {
	case MUC_VOICELIST:
		pHwndJidList = &hwndMucVoiceList;
		break;
	case MUC_MEMBERLIST:
		pHwndJidList = &hwndMucMemberList;
		break;
	case MUC_MODERATORLIST:
		pHwndJidList = &hwndMucModeratorList;
		break;
	case MUC_BANLIST:
		pHwndJidList = &hwndMucBanList;
		break;
	case MUC_ADMINLIST:
		pHwndJidList = &hwndMucAdminList;
		break;
	case MUC_OWNERLIST:
		pHwndJidList = &hwndMucOwnerList;
		break;
	default:
		mir_free( jidListInfo );
		return;
	}

	if ( *pHwndJidList!=NULL && IsWindow( *pHwndJidList )) {
		SetForegroundWindow( *pHwndJidList );
		SendMessage( *pHwndJidList, WM_JABBER_REFRESH, 0, ( LPARAM )jidListInfo );
	}
	else *pHwndJidList = CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_JIDLIST ), GetForegroundWindow(), JabberMucJidListDlgProc, ( LPARAM )jidListInfo );
}

static void JabberIqResultMucGetJidList( XmlNode *iqNode, JABBER_MUC_JIDLIST_TYPE listType )
{
	TCHAR* type;
	JABBER_MUC_JIDLIST_INFO *jidListInfo;

	if (( type=JabberXmlGetAttrValue( iqNode, "type" )) == NULL ) return;

	if ( !lstrcmp( type, _T("result" ))) {
		if (( jidListInfo=( JABBER_MUC_JIDLIST_INFO * ) mir_alloc( sizeof( JABBER_MUC_JIDLIST_INFO )) ) != NULL ) {
			jidListInfo->type = listType;
			jidListInfo->roomJid = NULL;	// Set in the dialog procedure
			if (( ( jidListInfo->iqNode )=JabberXmlCopyNode( iqNode )) != NULL ) {
				if ( GetCurrentThreadId() != jabberMainThreadId )
					QueueUserAPC( JabberMucJidListCreateDialogApcProc, hMainThread, ( DWORD )jidListInfo );
				else
					JabberMucJidListCreateDialogApcProc(( DWORD )jidListInfo );
			}
			else mir_free( jidListInfo );
}	}	}

void JabberIqResultMucGetVoiceList( XmlNode *iqNode, void *userdata )
{
	JabberLog( "<iq/> iqResultMucGetVoiceList" );
	JabberIqResultMucGetJidList( iqNode, MUC_VOICELIST );
}

void JabberIqResultMucGetMemberList( XmlNode *iqNode, void *userdata )
{
	JabberLog( "<iq/> iqResultMucGetMemberList" );
	JabberIqResultMucGetJidList( iqNode, MUC_MEMBERLIST );
}

void JabberIqResultMucGetModeratorList( XmlNode *iqNode, void *userdata )
{
	JabberLog( "<iq/> iqResultMucGetModeratorList" );
	JabberIqResultMucGetJidList( iqNode, MUC_MODERATORLIST );
}

void JabberIqResultMucGetBanList( XmlNode *iqNode, void *userdata )
{
	JabberLog( "<iq/> iqResultMucGetBanList" );
	JabberIqResultMucGetJidList( iqNode, MUC_BANLIST );
}

void JabberIqResultMucGetAdminList( XmlNode *iqNode, void *userdata )
{
	JabberLog( "<iq/> iqResultMucGetAdminList" );
	JabberIqResultMucGetJidList( iqNode, MUC_ADMINLIST );
}

void JabberIqResultMucGetOwnerList( XmlNode *iqNode, void *userdata )
{
	JabberLog( "<iq/> iqResultMucGetOwnerList" );
	JabberIqResultMucGetJidList( iqNode, MUC_OWNERLIST );
}

/////////////////////////////////////////////////////////////////////////////////////////

TCHAR* JABBER_MUC_JIDLIST_INFO::type2str() const
{
	switch( type ) {
		case MUC_VOICELIST:     return TranslateT( "Voice List" );
		case MUC_MEMBERLIST:    return TranslateT( "Member List" );
		case MUC_MODERATORLIST: return TranslateT( "Moderator List" );
		case MUC_BANLIST:       return TranslateT( "Ban List" );
		case MUC_ADMINLIST:     return TranslateT( "Admin List" );
		case MUC_OWNERLIST:     return TranslateT( "Owner List" );
	}

	return TranslateT( "JID List" );
}
