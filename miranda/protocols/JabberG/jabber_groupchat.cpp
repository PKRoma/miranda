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

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_groupchat.cpp,v $
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include <commctrl.h>
#include "resource.h"
#include "jabber_iq.h"
#include "jabber_caps.h"

#define GC_SERVER_LIST_SIZE 5

struct JabberGcRecentInfo
{
	TCHAR *room, *server, *nick, *password;

	JabberGcRecentInfo()
	{
		this->room = this->server = this->nick = this->password = NULL;
	}
	JabberGcRecentInfo(const TCHAR *room, const TCHAR *server, const TCHAR *nick = NULL, const TCHAR *password = NULL)
	{
		this->room = this->server = this->nick = this->password = NULL;
		fillData(room, server, nick, password);
	}
	JabberGcRecentInfo(const TCHAR *jid)
	{
		this->room = this->server = this->nick = this->password = NULL;
		fillData(jid);
	}
	JabberGcRecentInfo(int iRecent)
	{
		this->room = this->server = this->nick = this->password = NULL;
		loadRecent(iRecent);
	}

	~JabberGcRecentInfo()
	{
		cleanup();
	}

	void cleanup()
	{
		if (room)		mir_free(room);
		if (server)		mir_free(server);
		if (nick)		mir_free(nick);
		if (password)	mir_free(password);
		room = server = nick = password = NULL;
	}

	BOOL equals(const TCHAR *room, const TCHAR *server, const TCHAR *nick = NULL, const TCHAR *password = NULL)
	{
		return
			null_strequals(this->room, room) &&
			null_strequals(this->server, server) &&
			null_strequals(this->nick, nick) &&
			null_strequals(this->password, password);
	}

	void fillForm(HWND hwndDlg)
	{
		SetDlgItemText(hwndDlg, IDC_SERVER, server ? server : _T(""));
		SetDlgItemText(hwndDlg, IDC_ROOM, room ? room : _T(""));
		SetDlgItemText(hwndDlg, IDC_NICK, nick ? nick : _T(""));
		SetDlgItemText(hwndDlg, IDC_PASSWORD, password ? password : _T(""));
	}

	void fillData(const TCHAR *room, const TCHAR *server, const TCHAR *nick = NULL, const TCHAR *password = NULL)
	{
		cleanup();
		this->room		= room		? mir_tstrdup(room)		: NULL;
		this->server	= server	? mir_tstrdup(server)	: NULL;
		this->nick		= nick		? mir_tstrdup(nick)		: NULL;
		this->password	= password	? mir_tstrdup(password)	: NULL;
	}

	void fillData(const TCHAR *jid)
	{
		TCHAR *room, *server, *nick=NULL;
		room = NEWTSTR_ALLOCA(jid);
		server = _tcschr(room, _T('@'));
		if (server)
		{
			*server++ = 0;
			nick = _tcschr(server, _T('/'));
			if (nick) *nick++ = 0;
		} else
		{
			server = room;
			room = NULL;
		}

		fillData(room, server, nick);
	}

	BOOL loadRecent(int iRecent)
	{
		DBVARIANT dbv;
		char setting[MAXMODULELABELLENGTH];

		cleanup();

		mir_snprintf(setting, sizeof(setting), "rcMuc_%d_server", iRecent);
		if (!JGetStringT(NULL, setting, &dbv))
		{
			server = mir_tstrdup(dbv.ptszVal);
			JFreeVariant(&dbv);
		}

		mir_snprintf(setting, sizeof(setting), "rcMuc_%d_room", iRecent);
		if (!JGetStringT(NULL, setting, &dbv))
		{
			room = mir_tstrdup(dbv.ptszVal);
			JFreeVariant(&dbv);
		}

		mir_snprintf(setting, sizeof(setting), "rcMuc_%d_nick", iRecent);
		if (!JGetStringT(NULL, setting, &dbv))
		{
			nick = mir_tstrdup(dbv.ptszVal);
			JFreeVariant(&dbv);
		}

		mir_snprintf(setting, sizeof(setting), "rcMuc_%d_passwordW", iRecent);
		password = JGetStringCrypt(NULL, setting);

		return room || server || nick || password;
	}

	void saveRecent(int iRecent)
	{
		char setting[MAXMODULELABELLENGTH];

		mir_snprintf(setting, sizeof(setting), "rcMuc_%d_server", iRecent);
		if (server)
			JSetStringT(NULL, setting, server);
		else
			JDeleteSetting(NULL, setting);

		mir_snprintf(setting, sizeof(setting), "rcMuc_%d_room", iRecent);
		if (room)
			JSetStringT(NULL, setting, room);
		else
			JDeleteSetting(NULL, setting);

		mir_snprintf(setting, sizeof(setting), "rcMuc_%d_nick", iRecent);
		if (nick)
			JSetStringT(NULL, setting, nick);
		else
			JDeleteSetting(NULL, setting);

		mir_snprintf(setting, sizeof(setting), "rcMuc_%d_passwordW", iRecent);
		if (password)
			JSetStringCrypt(NULL, setting, password);
		else
			JDeleteSetting(NULL, setting);
	}

private:
	BOOL null_strequals(const TCHAR *str1, const TCHAR *str2)
	{
		if (!str1 && !str2) return TRUE;
		if (!str1 && str2 && !*str2) return TRUE;
		if (!str2 && str1 && !*str1) return TRUE;

		if (!str1 && str2) return FALSE;
		if (!str2 && str1) return FALSE;

		return !lstrcmp(str1, str2);
	}
};

static BOOL CALLBACK JabberGroupchatDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam );
static BOOL CALLBACK JabberGroupchatJoinDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam );

int JabberMenuHandleGroupchat( WPARAM wParam, LPARAM lParam )
{
	int iqId;

	// lParam is the initial conference server to browse ( if any )
	if ( IsWindow( hwndJabberGroupchat )) {
		SetForegroundWindow( hwndJabberGroupchat );
		if ( lParam != 0 ) {
			SendMessage( hwndJabberGroupchat, WM_JABBER_ACTIVATE, 0, lParam );	// Just to update the IDC_SERVER and clear the list
			iqId = JabberSerialNext();
			JabberIqAdd( iqId, IQ_PROC_DISCOROOMSERVER, JabberIqResultDiscoRoomItems );

			XmlNodeIq iq( "get", iqId, ( TCHAR* )lParam );
			XmlNode* query = iq.addQuery( JABBER_FEAT_DISCO_ITEMS );
			jabberThreadInfo->send( iq );
			// <iq/> result will send WM_JABBER_REFRESH to update the list with real data
		}
	}
	else hwndJabberGroupchat = CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_GROUPCHAT ), NULL, JabberGroupchatDlgProc, lParam );

	return 0;
}


int JabberMenuHandleJoinGroupchat( WPARAM wParam, LPARAM lParam )
{
	JabberGroupchatJoinRoomByJid( NULL, NULL );
	return 0;
}

static BOOL sortAscending;
static int sortColumn;

static int CALLBACK GroupchatCompare( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	JABBER_LIST_ITEM *item1, *item2;
	int res = 0;
	item1 = JabberListGetItemPtr( LIST_ROOM, ( TCHAR* )lParam1 );
	item2 = JabberListGetItemPtr( LIST_ROOM, ( TCHAR* )lParam2 );
	if ( item1!=NULL && item2!=NULL ) {
		switch ( lParamSort ) {
		case 0:	// sort by JID column
			res = lstrcmp( item1->jid, item2->jid );
			break;
		case 1: // sort by Name column
			res = lstrcmp( item1->name, item2->name );
			break;
	}	}

	if ( !sortAscending )
		res *= -1;

	return res;
}

static BOOL CALLBACK JabberGroupchatDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	HWND lv;
	LVCOLUMN lvCol;
	LVITEM lvItem;
	JABBER_LIST_ITEM *item;

	switch ( msg ) {
	case WM_INITDIALOG:
		// lParam is the initial conference server ( if any )
		SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )LoadIconEx( "group" ));
		TranslateDialogDefault( hwndDlg );
		sortColumn = -1;
		// Add columns
		lv = GetDlgItem( hwndDlg, IDC_ROOM );
		lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		lvCol.pszText = TranslateT( "JID" );
		lvCol.cx = 210;
		lvCol.iSubItem = 0;
		ListView_InsertColumn( lv, 0, &lvCol );
		lvCol.pszText = TranslateT( "Name" );
		lvCol.cx = 150;
		lvCol.iSubItem = 1;
		ListView_InsertColumn( lv, 1, &lvCol );
		lvCol.pszText = TranslateT( "Type" );
		lvCol.cx = 60;
		lvCol.iSubItem = 2;
		ListView_InsertColumn( lv, 2, &lvCol );
		if ( jabberOnline ) {
			if (( TCHAR* )lParam != NULL ) {
				SetDlgItemText( hwndDlg, IDC_SERVER, ( TCHAR* )lParam );
				int iqId = JabberSerialNext();
				JabberIqAdd( iqId, IQ_PROC_DISCOROOMSERVER, JabberIqResultDiscoRoomItems );

				XmlNodeIq iq( "get", iqId, ( TCHAR* )lParam );
				XmlNode* query = iq.addQuery( JABBER_FEAT_DISCO_ITEMS );
				jabberThreadInfo->send( iq );
			}
			else {
				for ( int i=0; i < GC_SERVER_LIST_SIZE; i++ ) {
					char text[100];
					mir_snprintf( text, sizeof( text ), "GcServerLast%d", i );
					DBVARIANT dbv;
					if ( !JGetStringT( NULL, text, &dbv )) {
						SendDlgItemMessage( hwndDlg, IDC_SERVER, CB_ADDSTRING, 0, ( LPARAM )dbv.ptszVal );
						JFreeVariant( &dbv );
			}	}	}
		}
		else EnableWindow( GetDlgItem( hwndDlg, IDC_JOIN ), FALSE );
		return TRUE;

	case WM_JABBER_ACTIVATE:
		// lParam = server from which agent information is obtained
		if ( lParam )
			SetDlgItemText( hwndDlg, IDC_SERVER, ( TCHAR* )lParam );
		ListView_DeleteAllItems( GetDlgItem( hwndDlg, IDC_ROOM ));
		EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), FALSE );
		return TRUE;

	case WM_JABBER_REFRESH:
		// lParam = server from which agent information is obtained
		{
			int i;
			TCHAR szBuffer[256];
			char text[128];

			if ( lParam ){
				_tcsncpy( szBuffer, ( TCHAR* )lParam, SIZEOF( szBuffer ));
				for ( i=0; i<GC_SERVER_LIST_SIZE; i++ ) {
					mir_snprintf( text, SIZEOF( text ), "GcServerLast%d", i );
					DBVARIANT dbv;
					if ( !JGetStringT( NULL, text, &dbv )) {
						JSetStringT( NULL, text, szBuffer );
						if ( !_tcsicmp( dbv.ptszVal, ( TCHAR* )lParam )) {
							JFreeVariant( &dbv );
							break;
						}
						_tcsncpy( szBuffer, dbv.ptszVal, SIZEOF( szBuffer ));
						JFreeVariant( &dbv );
					}
					else {
						JSetStringT( NULL, text, szBuffer );
						break;
				}	}

				SendDlgItemMessage( hwndDlg, IDC_SERVER, CB_RESETCONTENT, 0, 0 );
				for ( i=0; i<GC_SERVER_LIST_SIZE; i++ ) {
					mir_snprintf( text, SIZEOF( text ), "GcServerLast%d", i );
					DBVARIANT dbv;
					if ( !JGetStringT( NULL, text, &dbv )) {
						SendDlgItemMessage( hwndDlg, IDC_SERVER, CB_ADDSTRING, 0, ( LPARAM )dbv.ptszVal );
						JFreeVariant( &dbv );
				}	}

				SetDlgItemText( hwndDlg, IDC_SERVER, ( TCHAR* )lParam );
			}
			i = 0;
			lv = GetDlgItem( hwndDlg, IDC_ROOM );
			ListView_DeleteAllItems( lv );
			LVITEM lvItem;
			lvItem.iItem = 0;
			while (( i=JabberListFindNext( LIST_ROOM, i )) >= 0 ) {
				if (( item=JabberListGetItemPtrFromIndex( i )) != NULL ) {
					lvItem.mask = LVIF_PARAM | LVIF_TEXT;
					lvItem.iSubItem = 0;
					_tcsncpy( szBuffer, item->jid, SIZEOF(szBuffer));
					szBuffer[ SIZEOF(szBuffer)-1 ] = 0;
					lvItem.lParam = ( LPARAM )item->jid;
					lvItem.pszText = szBuffer;
					ListView_InsertItem( lv, &lvItem );

					lvItem.mask = LVIF_TEXT;
					lvItem.iSubItem = 1;
					lvItem.pszText = item->name;
					ListView_SetItem( lv, &lvItem );

					lvItem.iSubItem = 2;
					lvItem.pszText = item->type;
					ListView_SetItem( lv, &lvItem );
					lvItem.iItem++;
				}
				i++;
			}
			EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), TRUE );
		}
		return TRUE;
	case WM_JABBER_CHECK_ONLINE:
	{
		TCHAR text[128];
		if ( jabberOnline ) {
			EnableWindow( GetDlgItem( hwndDlg, IDC_JOIN ), TRUE );
			GetDlgItemText( hwndDlg, IDC_SERVER, text, SIZEOF( text ));
			EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), ( text[0]!='\0' ));
		}
		else {
			EnableWindow( GetDlgItem( hwndDlg, IDC_JOIN ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), FALSE );
			SetDlgItemTextA( hwndDlg, IDC_SERVER, "" );
			lv = GetDlgItem( hwndDlg, IDC_ROOM );
			ListView_DeleteAllItems( lv );
		}
		break;
	}
	case WM_NOTIFY:
		switch ( wParam ) {
		case IDC_ROOM:
			switch (( ( LPNMHDR )lParam )->code ) {
			case LVN_COLUMNCLICK:
				{
					LPNMLISTVIEW pnmlv = ( LPNMLISTVIEW ) lParam;

					if ( pnmlv->iSubItem>=0 && pnmlv->iSubItem<=1 ) {
						if ( pnmlv->iSubItem == sortColumn )
							sortAscending = !sortAscending;
						else {
							sortAscending = TRUE;
							sortColumn = pnmlv->iSubItem;
						}
						ListView_SortItems( GetDlgItem( hwndDlg, IDC_ROOM ), GroupchatCompare, sortColumn );
					}
				}
				break;
			}
			break;
		}
		break;
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case WM_JABBER_JOIN:
			if ( jabberChatDllPresent ) {
				lv = GetDlgItem( hwndDlg, IDC_ROOM );
				if (( lvItem.iItem=ListView_GetNextItem( lv, -1, LVNI_SELECTED )) >= 0 ) {
					lvItem.iSubItem = 0;
					lvItem.mask = LVIF_PARAM;
					ListView_GetItem( lv, &lvItem );
					ListView_SetItemState( lv, lvItem.iItem, 0, LVIS_SELECTED ); // Unselect the item
					DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_GROUPCHAT_JOIN ), hwndDlg, JabberGroupchatJoinDlgProc, ( LPARAM )lvItem.lParam );
				}
				else {
					TCHAR text[128];
					GetDlgItemText( hwndDlg, IDC_SERVER, text, SIZEOF( text ));
					DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_GROUPCHAT_JOIN ), hwndDlg, JabberGroupchatJoinDlgProc, ( LPARAM )text );
			}	}
			else JabberChatDllError();
			return TRUE;

		case WM_JABBER_ADD_TO_ROSTER:
			lv = GetDlgItem( hwndDlg, IDC_ROOM );
			if (( lvItem.iItem=ListView_GetNextItem( lv, -1, LVNI_SELECTED )) >= 0 ) {
				lvItem.iSubItem = 0;
				lvItem.mask = LVIF_PARAM;
				ListView_GetItem( lv, &lvItem );
				TCHAR* jid = ( TCHAR* )lvItem.lParam;
				{	GCSESSION gcw = {0};
					gcw.cbSize = sizeof(GCSESSION);
					gcw.iType = GCW_CHATROOM;
					gcw.ptszID = jid;
					gcw.pszModule = jabberProtoName;
					gcw.dwFlags = GC_TCHAR;
					gcw.ptszName = NEWTSTR_ALLOCA(gcw.ptszID);
					TCHAR* p = ( TCHAR* )_tcschr( gcw.ptszName, '@' );
					if ( p != NULL )
						*p = 0;
					CallService( MS_GC_NEWSESSION, 0, ( LPARAM )&gcw );
				}
				{	XmlNodeIq iq( "set" );
					XmlNode* query = iq.addQuery( JABBER_FEAT_IQ_ROSTER );
					XmlNode* item = query->addChild( "item" ); item->addAttr( "jid", jid );
					jabberThreadInfo->send( iq );
				}
				{	XmlNode p( "presence" ); p.addAttr( "to", jid ); p.addAttr( "type", "subscribe" );
					jabberThreadInfo->send( p );
			}	}
			break;

		case WM_JABBER_ADD_TO_BOOKMARKS:
			lv = GetDlgItem( hwndDlg, IDC_ROOM );
			if (( lvItem.iItem=ListView_GetNextItem( lv, -1, LVNI_SELECTED )) >= 0 ) {
				lvItem.iSubItem = 0;
				lvItem.mask = LVIF_PARAM;
				ListView_GetItem( lv, &lvItem );

				JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_BOOKMARK, ( TCHAR* )lvItem.lParam );
				if ( item == NULL ) {
					item = JabberListGetItemPtr( LIST_ROOM, ( TCHAR* )lvItem.lParam );
					if (item != NULL) {
						item->type = _T("conference");
						JabberAddEditBookmark(NULL, (LPARAM) item);
					}
				}
			}
		break;

		case IDC_SERVER:
		{	TCHAR text[ 128 ];
			GetDlgItemText( hwndDlg, IDC_SERVER, text, SIZEOF( text ));
			if ( jabberOnline && ( text[0] || HIWORD( wParam )==CBN_SELCHANGE ))
				EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), TRUE );
			break;
		}
		case IDC_BROWSE:
		{	TCHAR text[ 128 ];
			GetDlgItemText( hwndDlg, IDC_SERVER, text, SIZEOF( text ));
			if ( jabberOnline && text[0] ) {
				EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), FALSE );
				ListView_DeleteAllItems( GetDlgItem( hwndDlg, IDC_ROOM ));
				GetDlgItemText( hwndDlg, IDC_SERVER, text, SIZEOF( text ));

				int iqId = JabberSerialNext();
				JabberIqAdd( iqId, IQ_PROC_DISCOROOMSERVER, JabberIqResultDiscoRoomItems );

				XmlNodeIq iq( "get", iqId, text );
				XmlNode* query = iq.addQuery( JABBER_FEAT_DISCO_ITEMS );
				jabberThreadInfo->send( iq );
			}
			return TRUE;
		}
		case IDCANCEL:
		case IDCLOSE:
			DestroyWindow( hwndDlg );
			return TRUE;
		}
		break;
	case WM_CONTEXTMENU:
		if ( jabberOnline && ( HWND )wParam == GetDlgItem( hwndDlg, IDC_ROOM )) {
			HMENU hMenu = CreatePopupMenu();
			AppendMenu( hMenu, MF_STRING, WM_JABBER_JOIN, TranslateT( "Join" ));
			AppendMenu( hMenu, MF_STRING, WM_JABBER_ADD_TO_ROSTER, TranslateT( "Add to roster" ));
			if ( jabberThreadInfo->jabberServerCaps & JABBER_CAPS_PRIVATE_STORAGE ) AppendMenu( hMenu, MF_STRING, WM_JABBER_ADD_TO_BOOKMARKS, TranslateT( "Add to Bookmarks" ));
			TrackPopupMenu( hMenu, TPM_LEFTALIGN | TPM_NONOTIFY, LOWORD(lParam), HIWORD(lParam), 0, hwndDlg, 0 );
			::DestroyMenu( hMenu );
			return TRUE;
		}
		break;
	case WM_CLOSE:
		DestroyWindow( hwndDlg );
		break;
	case WM_DESTROY:
		hwndJabberGroupchat = NULL;
		break;
	}
	return FALSE;
}

void JabberGroupchatJoinRoom( const TCHAR* server, const TCHAR* room, const TCHAR* nick, const TCHAR* password )
{
	bool found = false;
	for (int i = 0 ; i < 5; ++i)
	{
		JabberGcRecentInfo info;
		if (!info.loadRecent(i))
			continue;
		if (info.equals(room, server, nick, password))
		{
			found = true;
			break;
		}
	}

	if (!found)
	{
		for (int i = 3; i >= 0; --i)
		{
			JabberGcRecentInfo info;
			if (info.loadRecent(i)) info.saveRecent(i+1);
		}

		JabberGcRecentInfo info(room, server, nick, password);
		info.saveRecent(0);
	}

	TCHAR text[128];
	mir_sntprintf( text, SIZEOF(text), _T("%s@%s/%s"), room, server, nick );

	JABBER_LIST_ITEM* item = JabberListAdd( LIST_CHATROOM, text );
	replaceStr( item->nick, nick );

	int status = ( jabberStatus == ID_STATUS_INVISIBLE ) ? ID_STATUS_ONLINE : jabberStatus;
	XmlNode* x = new XmlNode( "x" ); x->addAttr( "xmlns", JABBER_FEAT_MUC );
	if ( password && password[0]!='\0' )
		x->addChild( "password", password );
//	XmlNode *history = x->addChild( "history" );
//	history->addAttr( "maxstanzas", 20 );
	JabberSendPresenceTo( status, text, x );
}

void JabberGroupchatJoinRoomByJid( HWND hwndParent, TCHAR *jid )
{
	DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_GROUPCHAT_JOIN ), hwndParent, JabberGroupchatJoinDlgProc, ( LPARAM )jid );
}

////////////////////////////////////////////////////////////////////////////////
// Join Dialog
static int sttTextLineHeight = 16;

struct RoomInfo
{
	enum Overlay { ROOM_WAIT, ROOM_FAIL, ROOM_BOOKMARK, ROOM_DEFAULT };
	Overlay	overlay;
	TCHAR	*line1, *line2;
};

static int sttRoomListAppend(HWND hwndList, RoomInfo::Overlay overlay, TCHAR *line1, TCHAR *line2, TCHAR *name)
{
	RoomInfo *info = (RoomInfo *)mir_alloc(sizeof(RoomInfo));
	info->overlay = overlay;
	info->line1 = line1 ? mir_tstrdup(line1) : 0;
	info->line2 = line2 ? mir_tstrdup(line2) : 0;

	int id = SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)name);
	SendMessage(hwndList, CB_SETITEMDATA, id, (LPARAM)info);
	SendMessage(hwndList, CB_SETITEMHEIGHT, id, sttTextLineHeight * 2);
	return id;
}

static void sttIqResultDiscovery(XmlNode *iqNode, void *userdata, CJabberIqInfo *pInfo)
{
	if (!iqNode || !userdata || !pInfo)
		return;

	HWND hwndList = (HWND)pInfo->GetUserData();
	SendMessage(hwndList, CB_SHOWDROPDOWN, FALSE, 0);
	SendMessage(hwndList, CB_RESETCONTENT, 0, 0);

	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT )
	{
		XmlNode *query = JabberXmlGetChild( iqNode, "query" );
		if ( !query )
		{
			sttRoomListAppend(hwndList, RoomInfo::ROOM_FAIL,
				TranslateT("Jabber Error"),
				TranslateT("Failed to retrieve room list from server."),
				_T(""));
		} else
		{
			bool found = false;
			XmlNode *item;
			for ( int i = 1; item = JabberXmlGetNthChild( query, "item", i ); i++ )
			{
				TCHAR *jid = JabberXmlGetAttrValue(item, "jid");
				TCHAR *name = NEWTSTR_ALLOCA(jid);
				if (name)
				{
					if (TCHAR *p = _tcschr(name, _T('@')))
						*p = 0;
				} else
				{
					name = _T("");
				}

				sttRoomListAppend(hwndList,
					JabberListGetItemPtr(LIST_BOOKMARK, jid) ? RoomInfo::ROOM_BOOKMARK : RoomInfo::ROOM_DEFAULT,
					JabberXmlGetAttrValue(item, "name"),
					jid, name);

				found = true;
			}

			if (!found)
			{
				sttRoomListAppend(hwndList, RoomInfo::ROOM_FAIL,
					TranslateT("Jabber Error"),
					TranslateT("No rooms available on server."),
					_T(""));
			}
		}
	} else
	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_ERROR )
	{
		XmlNode *errorNode = JabberXmlGetChild( iqNode, "error" );
		TCHAR* str = JabberErrorMsg( errorNode );
		sttRoomListAppend(hwndList, RoomInfo::ROOM_FAIL,
			TranslateT("Jabber Error"),
			str,
			_T(""));
		mir_free( str );
	} else
	{
		sttRoomListAppend(hwndList, RoomInfo::ROOM_FAIL,
			TranslateT("Jabber Error"),
			TranslateT("Room list request timed out."),
			_T(""));
	}

	SendMessage(hwndList, CB_SHOWDROPDOWN, TRUE, 0);
}

static void sttJoinDlgShowRecentItems(HWND hwndDlg, int newCount)
{
	RECT rcTitle, rcLastItem;
	GetWindowRect(GetDlgItem(hwndDlg, IDC_TXT_RECENT), &rcTitle);
	GetWindowRect(GetDlgItem(hwndDlg, IDC_RECENT5), &rcLastItem);

	ShowWindow(GetDlgItem(hwndDlg, IDC_TXT_RECENT), newCount ? SW_SHOW : SW_HIDE);

	int oldCount = 5;
	for (int idc = IDC_RECENT1; idc <= IDC_RECENT5; ++idc)
	{
//		if (IsWindowVisible(GetDlgItem(hwndDlg, idc)))
//			++oldCount;

		ShowWindow(GetDlgItem(hwndDlg, idc), (idc - IDC_RECENT1 < newCount) ? SW_SHOW : SW_HIDE);
	}

	int curRecentHeight = rcLastItem.bottom - rcTitle.top - (5 - oldCount) * (rcLastItem.bottom - rcLastItem.top);
	int newRecentHeight = rcLastItem.bottom - rcTitle.top - (5 - newCount) * (rcLastItem.bottom - rcLastItem.top);
	if (!newCount) newRecentHeight = 0;
	int offset = newRecentHeight - curRecentHeight;

	RECT rc;
	int ctrls[] = { IDC_BOOKMARKS, IDOK, IDCANCEL };
	for (int i = 0; i < SIZEOF(ctrls); ++i)
	{
		GetWindowRect(GetDlgItem(hwndDlg, ctrls[i]), &rc);
		MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rc, 2);
		SetWindowPos(GetDlgItem(hwndDlg, ctrls[i]), NULL, rc.left, rc.top + offset, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
	}

	GetWindowRect(hwndDlg, &rc);
	SetWindowPos(hwndDlg, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top+offset, SWP_NOMOVE|SWP_NOZORDER);
}

static BOOL CALLBACK JabberGroupchatJoinDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	TCHAR text[128];

	switch ( msg ) {
	case WM_INITDIALOG:
		{
			// lParam is the room JID ( room@server ) in UTF-8
			hwndJabberJoinGroupchat = hwndDlg;
			TranslateDialogDefault( hwndDlg );

			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIconEx("group"));
			SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIconEx("group"));

			JabberGcRecentInfo *info = NULL;
			if ( lParam ) {
				info = new JabberGcRecentInfo((TCHAR *)lParam);
			} else
			{
				OpenClipboard(hwndDlg);
#ifdef UNICODE
				HANDLE hData = GetClipboardData(CF_UNICODETEXT);
#else
				HANDLE hData = GetClipboardData(CF_TEXT);
#endif
				if (hData)
				{
					TCHAR *buf = (TCHAR *)GlobalLock(hData);
					if (buf && _tcschr(buf, _T('@')) && !_tcschr(buf, _T(' ')))
						info = new JabberGcRecentInfo(buf);
					GlobalUnlock(hData);
				}
				CloseClipboard();
			}

			if (info)
			{
				info->fillForm(hwndDlg);
				delete info;
			}

			DBVARIANT dbv;
			if ( !JGetStringT( NULL, "Nick", &dbv )) {
				SetDlgItemText( hwndDlg, IDC_NICK, dbv.ptszVal );
				JFreeVariant( &dbv );
			}
			else {
				TCHAR* nick = JabberNickFromJID( jabberJID );
				SetDlgItemText( hwndDlg, IDC_NICK, nick );
				mir_free( nick );
			}

			{
				TEXTMETRIC tm = {0};
				HDC hdc = GetDC(hwndDlg);
				GetTextMetrics(hdc, &tm);
				ReleaseDC(hwndDlg, hdc);
				sttTextLineHeight = tm.tmHeight;
				SendDlgItemMessage(hwndDlg, IDC_ROOM, CB_SETITEMHEIGHT, -1, sttTextLineHeight-1);
			}

			{
				LOGFONT lf = {0};
				HFONT hfnt = (HFONT)SendDlgItemMessage(hwndDlg, IDC_TITLE, WM_GETFONT, 0, 0);
				GetObject(hfnt, sizeof(lf), &lf);
				lf.lfWeight = FW_BOLD;
				SendDlgItemMessage(hwndDlg, IDC_TITLE, WM_SETFONT, (WPARAM)CreateFontIndirect(&lf), TRUE);
				SendDlgItemMessage(hwndDlg, IDC_TXT_RECENT, WM_SETFONT, (WPARAM)CreateFontIndirect(&lf), TRUE);
			}

			SendDlgItemMessage(hwndDlg, IDC_BOOKMARKS, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIconEx("bookmarks"));
			SendDlgItemMessage(hwndDlg, IDC_BOOKMARKS, BUTTONSETASFLATBTN, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_BOOKMARKS, BUTTONADDTOOLTIP, (WPARAM)TranslateT("Bookmarks"), BATF_TCHAR);
			SendDlgItemMessage(hwndDlg, IDC_BOOKMARKS, BUTTONSETASPUSHBTN, 0, 0);

			JabberComboLoadRecentStrings(hwndDlg, IDC_SERVER, "joinWnd_rcSvr");

			int i = 0;
			for ( ; i < 5; ++i)
			{
				TCHAR jid[256];
				JabberGcRecentInfo info;
				if (info.loadRecent(i))
				{
					mir_sntprintf(jid, SIZEOF(jid), _T("%s@%s (%s)"),
						info.room, info.server,
						info.nick ? info.nick : TranslateT("<no nick>") );
					SetDlgItemText(hwndDlg, IDC_RECENT1+i, jid);
				} else
				{
					break;
				}
			}
			sttJoinDlgShowRecentItems(hwndDlg, i);
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if ( ((HWND)lParam == GetDlgItem(hwndDlg, IDC_WHITERECT)) ||
			 ((HWND)lParam == GetDlgItem(hwndDlg, IDC_TITLE)) ||
			 ((HWND)lParam == GetDlgItem(hwndDlg, IDC_DESCRIPTION)) )
			return (BOOL)GetStockObject(WHITE_BRUSH);
		return FALSE;

	case WM_DELETEITEM:
	{
		LPDELETEITEMSTRUCT lpdis = (LPDELETEITEMSTRUCT)lParam;
		if (lpdis->CtlID != IDC_ROOM)
			break;

		RoomInfo *info = (RoomInfo *)lpdis->itemData;
		if (info->line1) mir_free(info->line1);
		if (info->line2) mir_free(info->line2);
		mir_free(info);

		break;
	}

	case WM_MEASUREITEM:
	{
		LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
		if (lpmis->CtlID != IDC_ROOM)
			break;

		lpmis->itemHeight = 2*sttTextLineHeight;
		if (lpmis->itemID == -1)
			lpmis->itemHeight = sttTextLineHeight-1;

		break;
	}

	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
		if (lpdis->CtlID != IDC_ROOM)
			break;

		if (lpdis->itemID < 0)
			break;

		RoomInfo *info = (RoomInfo *)SendDlgItemMessage(hwndDlg, IDC_ROOM, CB_GETITEMDATA, lpdis->itemID, 0);
		COLORREF clLine1, clLine2, clBack;

		if (lpdis->itemState & ODS_SELECTED)
		{
			FillRect(lpdis->hDC, &lpdis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
			clBack = GetSysColor(COLOR_HIGHLIGHT);
			clLine1 = GetSysColor(COLOR_HIGHLIGHTTEXT);
		} else
		{
			FillRect(lpdis->hDC, &lpdis->rcItem, GetSysColorBrush(COLOR_WINDOW));
			clBack = GetSysColor(COLOR_WINDOW);
			clLine1 = GetSysColor(COLOR_WINDOWTEXT);
		}
		clLine2 = RGB(
				GetRValue(clLine1) * 0.66 + GetRValue(clBack) * 0.34,
				GetGValue(clLine1) * 0.66 + GetGValue(clBack) * 0.34,
				GetBValue(clLine1) * 0.66 + GetBValue(clBack) * 0.34
			);

		SetBkMode(lpdis->hDC, TRANSPARENT);

		RECT rc;

		rc = lpdis->rcItem;
		rc.bottom -= (rc.bottom - rc.top) / 2;
		rc.left += 20;
		SetTextColor(lpdis->hDC, clLine1);
		DrawText(lpdis->hDC, info->line1, lstrlen(info->line1), &rc, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER|DT_WORD_ELLIPSIS);

		rc = lpdis->rcItem;
		rc.top += (rc.bottom - rc.top) / 2;
		rc.left += 20;
		SetTextColor(lpdis->hDC, clLine2);
		DrawText(lpdis->hDC, info->line2, lstrlen(info->line2), &rc, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER|DT_WORD_ELLIPSIS);

		DrawIconEx(lpdis->hDC, lpdis->rcItem.left+1, lpdis->rcItem.top+1, LoadIconEx("group"), 16, 16, 0, NULL, DI_NORMAL);
		switch (info->overlay)
		{
		case RoomInfo::ROOM_WAIT:
			DrawIconEx(lpdis->hDC, lpdis->rcItem.left+1, lpdis->rcItem.top+1, LoadIconEx("disco_progress"), 16, 16, 0, NULL, DI_NORMAL);
			break;
		case RoomInfo::ROOM_FAIL:
			DrawIconEx(lpdis->hDC, lpdis->rcItem.left+1, lpdis->rcItem.top+1, LoadIconEx("disco_fail"), 16, 16, 0, NULL, DI_NORMAL);
			break;
		case RoomInfo::ROOM_BOOKMARK:
			DrawIconEx(lpdis->hDC, lpdis->rcItem.left+1, lpdis->rcItem.top+1, LoadIconEx("disco_ok"), 16, 16, 0, NULL, DI_NORMAL);
			break;
		}
	}

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_SERVER:
			switch (HIWORD(wParam)) {
			case CBN_EDITCHANGE:
			case CBN_SELCHANGE:
				{
					int iqid = GetWindowLong(GetDlgItem(hwndDlg, IDC_ROOM), GWL_USERDATA);
					if (iqid)
					{
						g_JabberIqManager.ExpireIq(iqid);
						SetWindowLong(GetDlgItem(hwndDlg, IDC_ROOM), GWL_USERDATA, 0);
					}
					SendDlgItemMessage(hwndDlg, IDC_ROOM, CB_RESETCONTENT, 0, 0);
				}
				break;
			}
			break;

		case IDC_ROOM:
			switch (HIWORD(wParam)) {
			case CBN_DROPDOWN:
				if (!SendDlgItemMessage(hwndDlg, IDC_ROOM, CB_GETCOUNT, 0, 0))
				{
					int iqid = GetWindowLong(GetDlgItem(hwndDlg, IDC_ROOM), GWL_USERDATA);
					if (iqid)
					{
						g_JabberIqManager.ExpireIq(iqid);
						SetWindowLong(GetDlgItem(hwndDlg, IDC_ROOM), GWL_USERDATA, 0);
					}

					SendDlgItemMessage(hwndDlg, IDC_ROOM, CB_RESETCONTENT, 0, 0);

					int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_SERVER)) + 1;
					TCHAR *server = (TCHAR *)_alloca(len * sizeof(TCHAR));
					GetWindowText(GetDlgItem(hwndDlg, IDC_SERVER), server, len);

					if (*server)
					{
						sttRoomListAppend(GetDlgItem(hwndDlg, IDC_ROOM), RoomInfo::ROOM_WAIT, TranslateT("Loading..."), TranslateT("Please wait for room list to download."), _T(""));

						CJabberIqInfo *pInfo = g_JabberIqManager.AddHandler(sttIqResultDiscovery, JABBER_IQ_TYPE_GET, server, 0, -1, (void *)GetDlgItem(hwndDlg, IDC_ROOM));
						pInfo->SetTimeout(30000);
						XmlNodeIq iq(pInfo);
						iq.addQuery(JABBER_FEAT_DISCO_ITEMS);
						jabberThreadInfo->send(iq);

						SetWindowLong(GetDlgItem(hwndDlg, IDC_ROOM), GWL_USERDATA, pInfo->GetIqId());
					} else
					{
						sttRoomListAppend(GetDlgItem(hwndDlg, IDC_ROOM), RoomInfo::ROOM_FAIL,
							TranslateT("Jabber Error"),
							TranslateT("Please specify groupchat directory first."),
							_T(""));
					}
				}
				break;
			}
			break;

		case IDC_BOOKMARKS:
			{
				HMENU hMenu = CreatePopupMenu();

				int i = 0;
				while ((i = JabberListFindNext(LIST_BOOKMARK, i)) >= 0)
				{
					JABBER_LIST_ITEM *item = 0;
					if (item = JabberListGetItemPtrFromIndex(i))
						if (!lstrcmp(item->type, _T("conference")))
							AppendMenu(hMenu, MF_STRING, (UINT_PTR)item, item->name);
					i++;
				}
				AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
				AppendMenu(hMenu, MF_STRING, (UINT_PTR)-1, TranslateT("Bookmarks..."));
				AppendMenu(hMenu, MF_STRING, (UINT_PTR)0, TranslateT("Cancel"));

				RECT rc; GetWindowRect(GetDlgItem(hwndDlg, IDC_BOOKMARKS), &rc);
				CheckDlgButton(hwndDlg, IDC_BOOKMARKS, TRUE);
				int res = TrackPopupMenu(hMenu, TPM_RETURNCMD, rc.left, rc.bottom, 0, hwndDlg, NULL);
				CheckDlgButton(hwndDlg, IDC_BOOKMARKS, FALSE);
				DestroyMenu(hMenu);

				if (res == -1)
				{
					JabberMenuHandleBookmarks(0, 0);
				} else
				if (res)
				{
					JABBER_LIST_ITEM *item = (JABBER_LIST_ITEM *)res;
					TCHAR *room = NEWTSTR_ALLOCA(item->jid);
					if (room)
					{
						TCHAR *server = _tcschr(room, _T('@'));
						if (server)
						{
							*server++ = 0;

							SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_SERVER, CBN_EDITCHANGE), (LPARAM)GetDlgItem(hwndDlg, IDC_SERVER));

							SetDlgItemText(hwndDlg, IDC_SERVER, server);
							SetDlgItemText(hwndDlg, IDC_ROOM, room);
							SetDlgItemText(hwndDlg, IDC_NICK, item->nick);
							SetDlgItemText(hwndDlg, IDC_PASSWORD, item->password);
						}
					}
				}
			}
			break;


		case IDC_RECENT1:
		case IDC_RECENT2:
		case IDC_RECENT3:
		case IDC_RECENT4:
		case IDC_RECENT5:
			{
				JabberGcRecentInfo info(LOWORD( wParam ) - IDC_RECENT1);
				info.fillForm(hwndDlg);
			}
			// fall through

		case IDOK:
			{
				GetDlgItemText( hwndDlg, IDC_SERVER, text, SIZEOF( text ));
				TCHAR* server = NEWTSTR_ALLOCA( text ), *room;

				JabberComboAddRecentString(hwndDlg, IDC_SERVER, "joinWnd_rcSvr", server);

				GetDlgItemText( hwndDlg, IDC_ROOM, text, SIZEOF( text ));
				room = NEWTSTR_ALLOCA( text );

				GetDlgItemText( hwndDlg, IDC_NICK, text, SIZEOF( text ));
				TCHAR* nick = NEWTSTR_ALLOCA( text );

				GetDlgItemText( hwndDlg, IDC_PASSWORD, text, SIZEOF( text ));
				TCHAR* password = NEWTSTR_ALLOCA( text );

				JabberGroupchatJoinRoom( server, room, nick, password );
			}
			// fall through
		case IDCANCEL:
			EndDialog( hwndDlg, 0 );
			break;
		}
		break;
	case WM_JABBER_CHECK_ONLINE:
		if ( !jabberOnline ) EndDialog( hwndDlg, 0 );
		break;
	case WM_DESTROY:
		{
			hwndJabberJoinGroupchat = NULL;

			DeleteObject((HFONT)SendDlgItemMessage(hwndDlg, IDC_TITLE, WM_GETFONT, 0, 0));
		}
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGroupchatProcessPresence - handles the group chat presence packet

static int sttGetStatusCode( XmlNode* node )
{
	XmlNode* statusNode = JabberXmlGetChild( node, "status" );
	if ( statusNode == NULL )
		return -1;

	TCHAR* statusCode = JabberXmlGetAttrValue( statusNode, "code" );
	if ( statusCode == NULL )
		return -1;

	return _ttol( statusCode );
}

void sttRenameParticipantNick( JABBER_LIST_ITEM* item, TCHAR* oldNick, XmlNode *itemNode )
{
	TCHAR* newNick = JabberXmlGetAttrValue( itemNode, "nick" );
	TCHAR* jid = JabberXmlGetAttrValue( itemNode, "jid" );
	if ( newNick == NULL )
		return;

	for ( int i=0; i < item->resourceCount; i++ ) {
		JABBER_RESOURCE_STATUS& RS = item->resource[i];
		if ( !lstrcmp( RS.resourceName, oldNick )) {
			replaceStr( RS.resourceName, newNick );

			if ( !lstrcmp( item->nick, oldNick )) {
				replaceStr( item->nick, newNick );

				HANDLE hContact = JabberHContactFromJID( item->jid );
				if ( hContact != NULL )
					JSetStringT( hContact, "MyNick", newNick );
			}

			GCDEST gcd = { jabberProtoName, NULL, GC_EVENT_CHUID };
			gcd.ptszID = item->jid;

			GCEVENT gce = {0};
			gce.cbSize = sizeof(GCEVENT);
			gce.pDest = &gcd;
			gce.ptszNick = oldNick;
			gce.ptszText = newNick;
			if (jid != NULL)
				gce.ptszUserInfo = jid;
			gce.time = time(0);
			gce.dwFlags = GC_TCHAR;
			CallServiceSync( MS_GC_EVENT, NULL, ( LPARAM )&gce );

			gcd.iType = GC_EVENT_NICK;
			gce.ptszNick = oldNick;
			gce.ptszUID = newNick;
			gce.ptszText = newNick;
			CallServiceSync( MS_GC_EVENT, NULL, ( LPARAM )&gce );
			break;
}	}	}

void JabberGroupchatProcessPresence( XmlNode *node, void *userdata )
{
	ThreadData* info;
	XmlNode *showNode, *statusNode, *errorNode, *itemNode, *n, *priorityNode;
	TCHAR* from;
	int status, newRes;
	int i;
	BOOL roomCreated;

	if ( !node || !node->name || strcmp( node->name, "presence" )) return;
	if (( info=( ThreadData* ) userdata ) == NULL ) return;
	if (( from=JabberXmlGetAttrValue( node, "from" )) == NULL ) return;

	TCHAR* nick = _tcschr( from, '/' );
	if ( nick == NULL || nick[1] == '\0' )
		return;
	nick++;

	JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_CHATROOM, from );
	if ( item == NULL )
		return;

	XmlNode* xNode = JabberXmlGetChildWithGivenAttrValue( node, "x", "xmlns", _T("http://jabber.org/protocol/muc#user"));

	TCHAR* type = JabberXmlGetAttrValue( node, "type" );
	if ( type == NULL || !_tcscmp( type, _T("available"))) {
		TCHAR* room = JabberNickFromJID( from );
		if ( room == NULL )
			return;

		JabberGcLogCreate( item );

		// Update status of room participant
		status = ID_STATUS_ONLINE;
		if (( showNode=JabberXmlGetChild( node, "show" )) != NULL ) {
			if ( showNode->text != NULL ) {
				if ( !_tcscmp( showNode->text , _T("away"))) status = ID_STATUS_AWAY;
				else if ( !_tcscmp( showNode->text , _T("xa"))) status = ID_STATUS_NA;
				else if ( !_tcscmp( showNode->text , _T("dnd"))) status = ID_STATUS_DND;
				else if ( !_tcscmp( showNode->text , _T("chat"))) status = ID_STATUS_FREECHAT;
		}	}

		TCHAR* str;
		if (( statusNode=JabberXmlGetChild( node, "status" ))!=NULL && statusNode->text!=NULL )
			str = statusNode->text;
		else
			str = NULL;

		char priority = 0;
		if (( priorityNode = JabberXmlGetChild( node, "priority" )) != NULL && priorityNode->text != NULL )
			priority = (char)_ttoi( priorityNode->text );

		newRes = ( JabberListAddResource( LIST_CHATROOM, from, status, str, priority ) == 0 ) ? 0 : GC_EVENT_JOIN;

		roomCreated = FALSE;

		// Check additional MUC info for this user
		if ( xNode != NULL ) {
			if (( itemNode=JabberXmlGetChild( xNode, "item" )) != NULL ) {
				JABBER_RESOURCE_STATUS* r = item->resource;
				for ( i=0; i<item->resourceCount && _tcscmp( r->resourceName, nick ); i++, r++ );
				if ( i < item->resourceCount ) {
					if (( str=JabberXmlGetAttrValue( itemNode, "affiliation" )) != NULL ) {
						if ( !_tcscmp( str, _T("owner")))        r->affiliation = AFFILIATION_OWNER;
						else if ( !_tcscmp( str, _T("admin")))   r->affiliation = AFFILIATION_ADMIN;
						else if ( !_tcscmp( str, _T("member")))  r->affiliation = AFFILIATION_MEMBER;
						else if ( !_tcscmp( str, _T("none")))	 r->affiliation = AFFILIATION_NONE;
						else if ( !_tcscmp( str, _T("outcast"))) r->affiliation = AFFILIATION_OUTCAST;
					}
					if (( str=JabberXmlGetAttrValue( itemNode, "role" )) != NULL ) {
						JABBER_GC_ROLE newRole = r->role;

						if ( !_tcscmp( str, _T("moderator")))        newRole = ROLE_MODERATOR;
						else if ( !_tcscmp( str, _T("participant"))) newRole = ROLE_PARTICIPANT;
						else if ( !_tcscmp( str, _T("visitor")))     newRole = ROLE_VISITOR;
						else                                         newRole = ROLE_NONE;

						if ( newRole != r->role && r->role != ROLE_NONE ) {
							JabberGcLogUpdateMemberStatus( item, nick, NULL, GC_EVENT_REMOVESTATUS, NULL );
							newRes = GC_EVENT_ADDSTATUS;
						}
						r->role = newRole;
					}
					if ( str = JabberXmlGetAttrValue( itemNode, "jid" ))
						replaceStr( r->szRealJid, str );
			}	}

			if ( sttGetStatusCode( xNode ) == 201 )
				roomCreated = TRUE;
		}

		// Update groupchat log window
		JabberGcLogUpdateMemberStatus( item, nick, str, newRes, NULL );

		HANDLE hContact = JabberHContactFromJID( from );
		if ( hContact != NULL )
			JSetWord( hContact, "Status", status );

		// Update room status
		//if ( item->status != ID_STATUS_ONLINE ) {
		//	item->status = ID_STATUS_ONLINE;
		//	JSetWord( hContact, "Status", ( WORD )ID_STATUS_ONLINE );
		//	JabberLog( "Room %s online", from );
		//}

		// Check <created/>
		if ( roomCreated ||
			(( n=JabberXmlGetChild( node, "created" ))!=NULL &&
				( str=JabberXmlGetAttrValue( n, "xmlns" ))!=NULL &&
				!_tcscmp( str, _T("http://jabber.org/protocol/muc#owner"))) ) {
			// A new room just created by me
			// Request room config
			int iqId = JabberSerialNext();
			JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultGetMuc );

			XmlNodeIq iq( "get", iqId, item->jid );
			XmlNode* query = iq.addQuery( xmlnsOwner );
			jabberThreadInfo->send( iq );
		}

		mir_free( room );
	}
	else if ( !lstrcmp( type, _T("unavailable"))) {
		TCHAR* str = 0;
		if ( xNode != NULL && item->nick != NULL ) {
			itemNode = JabberXmlGetChild( xNode, "item" );
			XmlNode* reasonNode = JabberXmlGetChild( itemNode, "reason" );
			str = JabberXmlGetAttrValue( itemNode, "jid" );
			int iStatus = sttGetStatusCode( xNode );
			if ( !lstrcmp( nick, item->nick )) {
				switch( iStatus ) {
				case 301:
				case 307:
					JabberGcQuit( item, iStatus, reasonNode );
					break;

				case 303:
					sttRenameParticipantNick( item, nick, itemNode );
					return;
			}	}
			else {
				switch( iStatus ) {
				case 303:
					sttRenameParticipantNick( item, nick, itemNode );
					return;

				case 301:
				case 307:
				case 322:
					JabberListRemoveResource( LIST_CHATROOM, from );
					JabberGcLogUpdateMemberStatus( item, nick, str, GC_EVENT_KICK, reasonNode, iStatus );
					return;
		}	}	}
		
		statusNode = JabberXmlGetChild( node, "status" );
		JabberListRemoveResource( LIST_CHATROOM, from );
		JabberGcLogUpdateMemberStatus( item, nick, str, GC_EVENT_PART, statusNode );

		HANDLE hContact = JabberHContactFromJID( from );
		if ( hContact != NULL )
			JSetWord( hContact, "Status", ID_STATUS_OFFLINE );
	}
	else if ( !lstrcmp( type, _T("error"))) {
		errorNode = JabberXmlGetChild( node, "error" );
		TCHAR* str = JabberErrorMsg( errorNode );
		MessageBox( NULL, str, TranslateT( "Jabber Error Message" ), MB_OK|MB_SETFOREGROUND );
		//JabberListRemoveResource( LIST_CHATROOM, from );
		JABBER_LIST_ITEM* item = JabberListGetItemPtr (LIST_CHATROOM, from );
		if ( item != NULL) 
			if (!item->bChatActive) JabberListRemove( LIST_CHATROOM, from );
		mir_free( str );
}	}

void strdel( char* parBuffer, int len )
{
	char* p;
	for ( p = parBuffer+len; *p != 0; p++ )
		p[ -len ] = *p;

	p[ -len ] = '\0';
}

void JabberGroupchatProcessMessage( XmlNode *node, void *userdata )
{
	ThreadData* info;
	XmlNode *n, *xNode;
	TCHAR* from, *type, *p, *nick, *msgText;
	JABBER_LIST_ITEM *item;

	if ( !node->name || strcmp( node->name, "message" )) return;
	if (( info=( ThreadData* ) userdata ) == NULL ) return;
	if (( from = JabberXmlGetAttrValue( node, "from" )) == NULL ) return;
	if (( item = JabberListGetItemPtr( LIST_CHATROOM, from )) == NULL ) return;

	if (( type = JabberXmlGetAttrValue( node, "type" )) == NULL ) return;
	if ( !lstrcmp( type, _T("error")))
		return;

	GCDEST gcd = { jabberProtoName, NULL, 0 };
	gcd.ptszID = item->jid;

	if (( n = JabberXmlGetChild( node, "subject" )) != NULL ) {
		if ( n->text == NULL || n->text[0] == '\0' )
			return;

		msgText = n->text;

		gcd.iType = GC_EVENT_TOPIC;

		if ( from != NULL ) {
			nick = _tcschr( from, '/' );
			if ( nick == NULL || nick[1] == '\0' )
				nick = NULL;
			else
				nick++;
		}
		else nick = NULL;
		replaceStr(item->itemResource.statusMessage, msgText);
	}
	else {
		if (( n = JabberXmlGetChild( node, "body" )) == NULL ) return;
		if ( n->text == NULL )
			return;

		nick = _tcschr( from, '/' );
		if ( nick == NULL || nick[1] == '\0' )
			return;
		nick++;

		msgText = n->text;

		if ( _tcsncmp( msgText, _T("/me "), 4 ) == 0 && _tcslen( msgText ) > 4 ) {
			msgText += 4;
			gcd.iType = GC_EVENT_ACTION;
		}
		else gcd.iType = GC_EVENT_MESSAGE;
	}

	JabberGcLogCreate( item );

	time_t msgTime = 0;
	BOOL delivered = FALSE;
	for ( int i = 1; ( xNode=JabberXmlGetNthChild( node, "x", i )) != NULL; i++ )
		if (( p=JabberXmlGetAttrValue( xNode, "xmlns" )) != NULL )
			if ( !_tcscmp( p, _T("jabber:x:delay")) && msgTime==0 )
				if (( p=JabberXmlGetAttrValue( xNode, "stamp" )) != NULL )
					msgTime = JabberIsoToUnixTime( p );

	time_t now = time( NULL );
	if ( msgTime == 0 || msgTime > now )
		msgTime = now;

	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.pDest = &gcd;
	gce.ptszUID = nick;
	gce.ptszNick = nick;
	gce.time = msgTime;
	gce.ptszText = EscapeChatTags( msgText );
	gce.bIsMe = lstrcmp( nick, item->nick ) == 0;
	gce.dwFlags = GC_TCHAR | GCEF_ADDTOLOG;
	CallServiceSync( MS_GC_EVENT, NULL, (LPARAM)&gce );

	item->bChatActive = 2;

	if ( gcd.iType == GC_EVENT_TOPIC ) {
		gce.dwFlags &= ~GCEF_ADDTOLOG;
		gcd.iType = GC_EVENT_SETSBTEXT;
		CallServiceSync( MS_GC_EVENT, NULL, (LPARAM)&gce );
	}

	mir_free( (void*)gce.pszText ); // Since we processed msgText and created a new string
}

/////////////////////////////////////////////////////////////////////////////////////////
// Accepting groupchat invitations

typedef struct {
	TCHAR* roomJid;
	TCHAR* from;
	TCHAR* reason;
	TCHAR* password;
}
	JABBER_GROUPCHAT_INVITE_INFO;

static void JabberAcceptGroupchatInvite( TCHAR* roomJid, TCHAR* reason, TCHAR* password )
{
	TCHAR room[256], *server, *p;
	_tcsncpy( room, roomJid, SIZEOF( room ));
	p = _tcstok( room, _T( "@" ));
	server = _tcstok( NULL, _T( "@" ));
	JabberGroupchatJoinRoom( server, p, reason, password );
}

static BOOL CALLBACK JabberGroupchatInviteAcceptDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
		{
			JABBER_GROUPCHAT_INVITE_INFO *inviteInfo = ( JABBER_GROUPCHAT_INVITE_INFO * ) lParam;

			TranslateDialogDefault( hwndDlg );
			SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG ) inviteInfo );
			SetDlgItemText( hwndDlg, IDC_FROM, inviteInfo->from );
			SetDlgItemText( hwndDlg, IDC_ROOM, inviteInfo->roomJid );

			if ( inviteInfo->reason != NULL )
				SetDlgItemText( hwndDlg, IDC_REASON, inviteInfo->reason );

			TCHAR* myNick = JabberNickFromJID( jabberJID );
			SetDlgItemText( hwndDlg, IDC_NICK, myNick );
			mir_free( myNick );

			SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )LoadIconEx( "group" ));
		}
		return TRUE;
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_ACCEPT:
			{
				JABBER_GROUPCHAT_INVITE_INFO *inviteInfo = ( JABBER_GROUPCHAT_INVITE_INFO * ) GetWindowLong( hwndDlg, GWL_USERDATA );
				TCHAR text[128];
				GetDlgItemText( hwndDlg, IDC_NICK, text, SIZEOF( text ));
				JabberAcceptGroupchatInvite( inviteInfo->roomJid, text, inviteInfo->password );
			}
			// Fall through
		case IDCANCEL:
		case IDCLOSE:
			EndDialog( hwndDlg, 0 );
			return TRUE;
		}
		break;
	case WM_CLOSE:
		EndDialog( hwndDlg, 0 );
		break;
	}

	return FALSE;
}

static void __cdecl JabberGroupchatInviteAcceptThread( JABBER_GROUPCHAT_INVITE_INFO *inviteInfo )
{
	DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_GROUPCHAT_INVITE_ACCEPT ), NULL, JabberGroupchatInviteAcceptDlgProc, ( LPARAM )inviteInfo );
	mir_free( inviteInfo->roomJid );
	mir_free( inviteInfo->from );
	mir_free( inviteInfo->reason );
	mir_free( inviteInfo->password );
	mir_free( inviteInfo );
}

void JabberGroupchatProcessInvite( TCHAR* roomJid, TCHAR* from, TCHAR* reason, TCHAR* password )
{
	if ( roomJid == NULL )
		return;

	if (JabberListGetItemPtr( LIST_CHATROOM, roomJid ))
		return;

	if ( JGetByte( "AutoAcceptMUC", FALSE ) == FALSE ) {
		JABBER_GROUPCHAT_INVITE_INFO* inviteInfo = ( JABBER_GROUPCHAT_INVITE_INFO * ) mir_alloc( sizeof( JABBER_GROUPCHAT_INVITE_INFO ));
		inviteInfo->roomJid  = mir_tstrdup( roomJid );
		inviteInfo->from     = mir_tstrdup( from );
		inviteInfo->reason   = mir_tstrdup( reason );
		inviteInfo->password = mir_tstrdup( password );
		mir_forkthread(( pThreadFunc )JabberGroupchatInviteAcceptThread, inviteInfo );
	}
	else {
		TCHAR* myNick = JabberNickFromJID( jabberJID );
		JabberAcceptGroupchatInvite( roomJid, myNick, password );
		mir_free( myNick );
}	}
