/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005     George Hazan

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

*/

#include "jabber.h"
#include <commctrl.h>
#include "resource.h"
#include "jabber_iq.h"

#define GC_SERVER_LIST_SIZE 5

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
			JabberSend( jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/disco#items'/></iq>", iqId, ( char* )lParam );
			// <iq/> result will send WM_JABBER_REFRESH to update the list with real data
		}
	}
	else {
		hwndJabberGroupchat = CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_GROUPCHAT ), NULL, JabberGroupchatDlgProc, lParam );
	}

	return 0;
}

static BOOL sortAscending;
static int sortColumn;

static int CALLBACK GroupchatCompare( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	JABBER_LIST_ITEM *item1, *item2;
	char* localJID1, *localJID2;
	int res;

	res = 0;
	item1 = JabberListGetItemPtr( LIST_ROOM, ( char* )lParam1 );
	item2 = JabberListGetItemPtr( LIST_ROOM, ( char* )lParam2 );
	if ( item1!=NULL && item2!=NULL ) {
		switch ( lParamSort ) {
		case 0:	// sort by JID column
			{
				localJID1 = JabberTextDecode( item1->jid );
				localJID2 = JabberTextDecode( item2->jid );
				if ( localJID1!=NULL && localJID2!=NULL )
					res = strcmp( localJID1, localJID2 );
				free( localJID1 );
				free( localJID2 );
			}
			break;
		case 1: // sort by Name column
			if ( item1->name!=NULL && item2->name!=NULL )
				res = strcmp( item1->name, item2->name );
			break;
		}
	}
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
	char text[128];
	//HANDLE hContact;
	char* p;
	int iqId;

	switch ( msg ) {
	case WM_INITDIALOG:
		// lParam is the initial conference server ( if any )
		SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )LoadIcon( hInst, MAKEINTRESOURCE( IDI_GROUP )) );
		TranslateDialogDefault( hwndDlg );
		sortColumn = -1;
		// Add columns
		lv = GetDlgItem( hwndDlg, IDC_ROOM );
		lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		lvCol.pszText = JTranslate( "JID" );
		lvCol.cx = 210;
		lvCol.iSubItem = 0;
		ListView_InsertColumn( lv, 0, &lvCol );
		lvCol.pszText = JTranslate( "Name" );
		lvCol.cx = 150;
		lvCol.iSubItem = 1;
		ListView_InsertColumn( lv, 1, &lvCol );
		lvCol.pszText = JTranslate( "Type" );
		lvCol.cx = 60;
		lvCol.iSubItem = 2;
		ListView_InsertColumn( lv, 2, &lvCol );
		if ( jabberOnline ) {
			if (( char* )lParam != NULL ) {
				SetDlgItemText( hwndDlg, IDC_SERVER, ( char* )lParam );
				iqId = JabberSerialNext();
				JabberIqAdd( iqId, IQ_PROC_DISCOROOMSERVER, JabberIqResultDiscoRoomItems );
				JabberSend( jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/disco#items'/></iq>", iqId, ( char* )lParam );
			}
			else {
				DBVARIANT dbv;
				int i;

				for ( i=0; i<GC_SERVER_LIST_SIZE; i++ ) {
					_snprintf( text, sizeof( text ), "GcServerLast%d", i );
					if ( !DBGetContactSetting( NULL, jabberProtoName, text, &dbv )) {
						SendDlgItemMessage( hwndDlg, IDC_SERVER, CB_ADDSTRING, 0, ( LPARAM )dbv.pszVal );
						JFreeVariant( &dbv );
					}
				}
			}
		}
		else
			EnableWindow( GetDlgItem( hwndDlg, IDC_JOIN ), FALSE );
		return TRUE;
	case WM_JABBER_ACTIVATE:
		// lParam = server from which agent information is obtained
		if ( lParam )
			SetDlgItemText( hwndDlg, IDC_SERVER, ( char* )lParam );
		ListView_DeleteAllItems( GetDlgItem( hwndDlg, IDC_ROOM ));
		EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), FALSE );
		return TRUE;
	case WM_JABBER_REFRESH:
		// lParam = server from which agent information is obtained
		{
			int i;

			if ( lParam ){
				char server[128];
				DBVARIANT dbv;

				strncpy( server, ( char* )lParam, sizeof( server ));
				for ( i=0; i<GC_SERVER_LIST_SIZE; i++ ) {
					_snprintf( text, sizeof( text ), "GcServerLast%d", i );
					if ( !DBGetContactSetting( NULL, jabberProtoName, text, &dbv )) {
						JSetString( NULL, text, server );
						if ( !stricmp( dbv.pszVal, ( char* )lParam )) {
							JFreeVariant( &dbv );
							break;
						}
						strncpy( server, dbv.pszVal, sizeof( server ));
						JFreeVariant( &dbv );
					}
					else {
						JSetString( NULL, text, server );
						break;
					}
				}
				SendDlgItemMessage( hwndDlg, IDC_SERVER, CB_RESETCONTENT, 0, 0 );
				for ( i=0; i<GC_SERVER_LIST_SIZE; i++ ) {
					_snprintf( text, sizeof( text ), "GcServerLast%d", i );
					if ( !DBGetContactSetting( NULL, jabberProtoName, text, &dbv )) {
						SendDlgItemMessage( hwndDlg, IDC_SERVER, CB_ADDSTRING, 0, ( LPARAM )dbv.pszVal );
						JFreeVariant( &dbv );
					}
				}
				SetDlgItemText( hwndDlg, IDC_SERVER, ( char* )lParam );
			}
			i = 0;
			lv = GetDlgItem( hwndDlg, IDC_ROOM );
			ListView_DeleteAllItems( lv );
			lvItem.iItem = 0;
			while (( i=JabberListFindNext( LIST_ROOM, i )) >= 0 ) {
				if (( item=JabberListGetItemPtrFromIndex( i )) != NULL ) {
					lvItem.mask = LVIF_PARAM | LVIF_TEXT;
					lvItem.iSubItem = 0;
					lvItem.lParam = ( LPARAM )item->jid;
					lvItem.pszText = item->jid;
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
		if ( jabberOnline ) {
			EnableWindow( GetDlgItem( hwndDlg, IDC_JOIN ), TRUE );
			GetDlgItemText( hwndDlg, IDC_SERVER, text, sizeof( text ));
			EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), ( text[0]!='\0' ));
		}
		else {
			EnableWindow( GetDlgItem( hwndDlg, IDC_JOIN ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), FALSE );
			SetDlgItemText( hwndDlg, IDC_SERVER, "" );
			lv = GetDlgItem( hwndDlg, IDC_ROOM );
			ListView_DeleteAllItems( lv );
		}
		break;
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
		case IDC_JOIN:
			lv = GetDlgItem( hwndDlg, IDC_ROOM );
			if (( lvItem.iItem=ListView_GetNextItem( lv, -1, LVNI_SELECTED )) >= 0 ) {
				lvItem.iSubItem = 0;
				lvItem.mask = LVIF_PARAM;
				ListView_GetItem( lv, &lvItem );
				ListView_SetItemState( lv, lvItem.iItem, 0, LVIS_SELECTED ); // Unselect the item
				DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_GROUPCHAT_JOIN ), hwndDlg, JabberGroupchatJoinDlgProc, ( LPARAM )lvItem.lParam );
			}
			else {
				GetDlgItemText( hwndDlg, IDC_SERVER, text, sizeof( text ));
				DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_GROUPCHAT_JOIN ), hwndDlg, JabberGroupchatJoinDlgProc, ( LPARAM )text );
			}
			return TRUE;
		case IDC_SERVER:
			GetDlgItemText( hwndDlg, IDC_SERVER, text, sizeof( text ));
			if ( jabberOnline && ( text[0] || HIWORD( wParam )==CBN_SELCHANGE ))
				EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), TRUE );
			break;
		case IDC_BROWSE:
			GetDlgItemText( hwndDlg, IDC_SERVER, text, sizeof( text ));
			if ( jabberOnline && text[0] ) {
				EnableWindow( GetDlgItem( hwndDlg, IDC_BROWSE ), FALSE );
				ListView_DeleteAllItems( GetDlgItem( hwndDlg, IDC_ROOM ));
				GetDlgItemText( hwndDlg, IDC_SERVER, text, sizeof( text ));
				if (( p=JabberTextEncode( text )) != NULL ) {
					iqId = JabberSerialNext();
					JabberIqAdd( iqId, IQ_PROC_DISCOROOMSERVER, JabberIqResultDiscoRoomItems );
					JabberSend( jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/disco#items'/></iq>", iqId, p );
					free( p );
				}
			}
			return TRUE;
		case IDCLOSE:
			DestroyWindow( hwndDlg );
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

void JabberGroupchatJoinRoom( char* server, char* room, char* nick, char* password )
{
	JABBER_LIST_ITEM *item;
	char passwordText[256];
	char text[128];
	int status;

	_snprintf( text, sizeof( text ), "%s@%s/%s", room, server, nick );
	item = JabberListAdd( LIST_CHATROOM, text );
	if ( item->nick ) free( item->nick );
	item->nick = _strdup( nick );
	status = ( jabberStatus==ID_STATUS_INVISIBLE )?ID_STATUS_ONLINE:jabberStatus;
	if ( password && password[0]!='\0' ) {
		_snprintf( passwordText, sizeof( passwordText ), "<x xmlns='http://jabber.org/protocol/muc'><password>%s</password></x>", password );
		JabberSendPresenceTo( status, text, passwordText );
	}
	else {
		JabberSendPresenceTo( status, text, "<x xmlns='http://jabber.org/protocol/muc'/>" );
	}
}

static BOOL CALLBACK JabberGroupchatJoinDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	char text[128];
	char* p;

	switch ( msg ) {
	case WM_INITDIALOG:
		{
			DBVARIANT dbv;
			char* nick, *room;

			// lParam is the room JID ( room@server ) in UTF-8
			hwndJabberJoinGroupchat = hwndDlg;
			TranslateDialogDefault( hwndDlg );
			if ( lParam ){
				strncpy( text, ( char* )lParam, sizeof( text ));
				if (( p=strchr( text, '@' )) != NULL ) {
					*p = '\0';
					// Need to save room name in UTF-8 in case the room codepage is different
					// from the local code page
					room = strdup( text );
					SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG ) room );
					room = JabberTextDecode( text );
					SetDlgItemText( hwndDlg, IDC_ROOM, room );
					SetDlgItemText( hwndDlg, IDC_SERVER, p+1 );
					free( room );
				}
				else
					SetDlgItemText( hwndDlg, IDC_SERVER, text );
			}
			if ( !DBGetContactSetting( NULL, jabberProtoName, "Nick", &dbv )) {
				SetDlgItemText( hwndDlg, IDC_NICK, dbv.pszVal );
				JFreeVariant( &dbv );
			}
			else {
				if (( nick=JabberNickFromJID( jabberJID )) != NULL ) {
					SetDlgItemText( hwndDlg, IDC_NICK, nick );
					free( nick );
				}
			}
		}
		return TRUE;
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_ROOM:
			{
				char* str;

				if (( HWND )lParam==GetFocus() && HIWORD( wParam )==EN_CHANGE ) {
					// Change in IDC_ROOM field is detected,
					// invalidate the saved UTF-8 room name if any
					str = ( char* )GetWindowLong( hwndDlg, GWL_USERDATA );
					if ( str != NULL ) {
						free( str );
						SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG ) NULL );
					}
				}
			}
			break;
		case IDOK:
			{
				char* server, *room, *nick, *password, *str;

				GetDlgItemText( hwndDlg, IDC_SERVER, text, sizeof( text ));
				server = JabberTextEncode( text );;
				if (( str=( char* )GetWindowLong( hwndDlg, GWL_USERDATA )) != NULL )
					room = _strdup( str );
				else {
					GetDlgItemText( hwndDlg, IDC_ROOM, text, sizeof( text ));
					room = JabberTextEncode( text );
				}
				GetDlgItemText( hwndDlg, IDC_NICK, text, sizeof( text ));
				nick = JabberTextEncode( text );
				GetDlgItemText( hwndDlg, IDC_PASSWORD, text, sizeof( text ));
				password = JabberTextEncode( text );
				JabberGroupchatJoinRoom( server, room, nick, password );
				free( password );
				free( nick );
				free( room );
				free( server );
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
			char* str;

			if (( str=( char* )GetWindowLong( hwndDlg, GWL_USERDATA )) != NULL )
				free( str );
			hwndJabberJoinGroupchat = NULL;
		}
		break;
	}
	return FALSE;
}

void JabberGroupchatProcessPresence( XmlNode *node, void *userdata )
{
	struct ThreadData *info;
	//HANDLE hContact;
	XmlNode *showNode, *statusNode, *errorNode, *xNode, *itemNode, *n;
	JABBER_LIST_ITEM *item;
	char* from, *type, *room, *show, *nick, *statusCode;
	int status;
	char* str, *p;
	//char roomNick[128];
	int iqId, i;
	BOOL roomCreated;

	if ( !node || !node->name || strcmp( node->name, "presence" )) return;
	if (( info=( struct ThreadData * ) userdata ) == NULL ) return;

	if (( from=JabberXmlGetAttrValue( node, "from" ))!=NULL && ( nick=strchr( from, '/' ))!=NULL && *( ++nick )!='\0' ) {
		JabberStringDecode( from );

		if (( item=JabberListGetItemPtr( LIST_CHATROOM, from )) != NULL ) {

			type = JabberXmlGetAttrValue( node, "type" );
			if ( type==NULL || ( !strcmp( type, "available" )) ) {
				if (( room=JabberNickFromJID( from )) != NULL ) {
					if ( item->hwndGcDlg == NULL )
						JabberGcLogCreate( from );

					// Update status of room participant
					status = ID_STATUS_ONLINE;
					if (( showNode=JabberXmlGetChild( node, "show" )) != NULL ) {
						if (( show=showNode->text ) != NULL ) {
							if ( !strcmp( show, "away" )) status = ID_STATUS_AWAY;
							else if ( !strcmp( show, "xa" )) status = ID_STATUS_NA;
							else if ( !strcmp( show, "dnd" )) status = ID_STATUS_DND;
							else if ( !strcmp( show, "chat" )) status = ID_STATUS_FREECHAT;
					}	}

					if (( statusNode=JabberXmlGetChild( node, "status" ))!=NULL && statusNode->text!=NULL )
						str = JabberTextDecode( statusNode->text );
					else
						str = NULL;
					JabberListAddResource( LIST_CHATROOM, from, status, str );
					if ( str ) free( str );

					// Check for the case when changing nick
					if ( item->nick==NULL && item->newNick!=NULL && !strcmp( item->newNick, nick )) {
						item->nick = _strdup( nick );
						free( item->newNick );
						item->newNick = NULL;
					}

					roomCreated = FALSE;

					// Check additional MUC info for this user
					if (( xNode=JabberXmlGetChildWithGivenAttrValue( node, "x", "xmlns", "http://jabber.org/protocol/muc#user" )) != NULL ) {
						if (( itemNode=JabberXmlGetChild( xNode, "item" )) != NULL ) {
							for ( i=0; i<item->resourceCount && strcmp( item->resource[i].resourceName, nick ); i++ );
							if ( i < item->resourceCount ) {
								item->resource[i].affiliation = AFFILIATION_NONE;
								item->resource[i].role = ROLE_NONE;
								if (( str=JabberXmlGetAttrValue( itemNode, "affiliation" )) != NULL ) {
									if ( !strcmp( str, "owner" )) item->resource[i].affiliation = AFFILIATION_OWNER;
									else if ( !strcmp( str, "admin" )) item->resource[i].affiliation = AFFILIATION_ADMIN;
									else if ( !strcmp( str, "member" )) item->resource[i].affiliation = AFFILIATION_MEMBER;
									else if ( !strcmp( str, "outcast" )) item->resource[i].affiliation = AFFILIATION_OUTCAST;
								}
								if (( str=JabberXmlGetAttrValue( itemNode, "role" )) != NULL ) {
									if ( !strcmp( str, "moderator" )) item->resource[i].role = ROLE_MODERATOR;
									else if ( !strcmp( str, "participant" )) item->resource[i].role = ROLE_PARTICIPANT;
									else if ( !strcmp( str, "visitor" )) item->resource[i].role = ROLE_VISITOR;
						}	}	}

						if (( statusNode=JabberXmlGetChild( xNode, "status" )) != NULL )
							if (( statusCode=JabberXmlGetAttrValue( statusNode, "code" )) != NULL )
								if ( !strcmp( statusCode, "201" ))
									roomCreated = TRUE;
					}

					// Update groupchat log window
					JabberGcLogUpdateMemberStatus( from );

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
						 !strcmp( str, "http://jabber.org/protocol/muc#owner" )) ) {
						// A new room just created by me
						// Request room config
						iqId = JabberSerialNext();
						JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultGetMuc );
						JabberSend( jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/muc#owner'/></iq>", iqId, item->jid );
					}

					free( room );
				}
			}
			else if ( !strcmp( type, "unavailable" )) {
				int forceType;

				forceType = -1;
				if ( item->nick!=NULL && ( p=strchr( from, '/' ))!=NULL && *( ++p )!='\0' ) {
					if ( !strcmp( p, item->nick )) {
						// Set item->nick to NULL for the change nick process ( it does no harm even if it is actually not a change nick process )
						free( item->nick );
						item->nick = NULL;
						// Now evil part, check if I am kicked or banned
						if (( xNode=JabberXmlGetChildWithGivenAttrValue( node, "x", "xmlns", "http://jabber.org/protocol/muc#user" )) != NULL ) {
							itemNode = JabberXmlGetChild( xNode, "item" );
							n = JabberXmlCopyNode( itemNode );
							if (( statusNode=JabberXmlGetChild( xNode, "status" ))!=NULL && ( statusCode=JabberXmlGetAttrValue( statusNode, "code" ))!=NULL ) {
								if ( !strcmp( statusCode, "301" ))
									forceType = 301;
								else if ( !strcmp( statusCode, "307" ))
									forceType = 307;
							}
							if ( forceType >= 0 )
								PostMessage( item->hwndGcDlg, WM_JABBER_GC_FORCE_QUIT, ( WPARAM ) forceType, ( LPARAM )n );
				}	}	}

				JabberListRemoveResource( LIST_CHATROOM, from );
				JabberGcLogUpdateMemberStatus( from );
			}
			else if ( !strcmp( type, "error" )) {
				errorNode = JabberXmlGetChild( node, "error" );
				str = JabberErrorMsg( errorNode );
				MessageBox( NULL, str, JTranslate( "Jabber Error Message" ), MB_OK|MB_SETFOREGROUND );
				//JabberListRemoveResource( LIST_CHATROOM, from );
				JabberListRemove( LIST_CHATROOM, from );
				free( str );
}	}	}	}

void JabberGroupchatProcessMessage( XmlNode *node, void *userdata )
{
	struct ThreadData *info;
	XmlNode *bodyNode, *subjectNode, *xNode;
	char* from, *type, *p;
	JABBER_LIST_ITEM *item;

	if ( !node->name || strcmp( node->name, "message" )) return;
	if (( info=( struct ThreadData * ) userdata ) == NULL ) return;

	if (( from=JabberXmlGetAttrValue( node, "from" )) != NULL ) {

		if (( item=JabberListGetItemPtr( LIST_CHATROOM, from )) != NULL ) {

			type = JabberXmlGetAttrValue( node, "type" );

			if ( type!=NULL && !strcmp( type, "error" )) {
			}

			else {

				if (( bodyNode=JabberXmlGetChild( node, "body" )) != NULL ) {
					if ( bodyNode->text != NULL ) {
						if ( item->hwndGcDlg == NULL ) {
							JabberGcLogCreate( from );
						}
						if (( subjectNode=JabberXmlGetChild( node, "subject" ))!=NULL && subjectNode->text!=NULL && subjectNode->text[0]!='\0' ) {
							JabberGcLogSetTopic( from, subjectNode->text );
						}

						time_t msgTime = 0;
						BOOL delivered = FALSE;
						for ( int i = 1; ( xNode=JabberXmlGetNthChild( node, "x", i )) != NULL; i++ )
							if (( p=JabberXmlGetAttrValue( xNode, "xmlns" )) != NULL )
								if ( !strcmp( p, "jabber:x:delay" ) && msgTime==0 )
									if (( p=JabberXmlGetAttrValue( xNode, "stamp" )) != NULL )
										msgTime = JabberIsoToUnixTime( p );

						time_t now = time( NULL );
						if ( msgTime==0 || msgTime>now )
							msgTime = now;
						JabberGcLogAppend( from, msgTime, bodyNode->text );
}	}	}	}	}	}

typedef struct {
	char* roomJid;
	char* from;
	char* reason;
	char* password;
} JABBER_GROUPCHAT_INVITE_INFO;

static BOOL CALLBACK JabberGroupchatInviteAcceptDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
		{
			JABBER_GROUPCHAT_INVITE_INFO *inviteInfo = ( JABBER_GROUPCHAT_INVITE_INFO * ) lParam;
			char* nick, *str;

			TranslateDialogDefault( hwndDlg );
			SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG ) inviteInfo );
			if (( str=JabberTextDecode( inviteInfo->from )) != NULL ) {
				SetDlgItemText( hwndDlg, IDC_FROM, str );
				free( str );
			}
			if (( str=JabberTextDecode( inviteInfo->roomJid )) != NULL ) {
				SetDlgItemText( hwndDlg, IDC_ROOM, str );
				free( str );
			}
			if (( str=JabberTextDecode( inviteInfo->reason )) != NULL ) {
				SetDlgItemText( hwndDlg, IDC_REASON, str );
				free( str );
			}
			nick = JabberNickFromJID( jabberJID );
			SetDlgItemText( hwndDlg, IDC_NICK, nick );
			free( nick );
			SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )LoadIcon( hInst, MAKEINTRESOURCE( IDI_GROUP )) );
		}
		return TRUE;
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_ACCEPT:
			{
				JABBER_GROUPCHAT_INVITE_INFO *inviteInfo;
				char roomJid[256], text[128];
				char* server, *room, *nick;

				inviteInfo = ( JABBER_GROUPCHAT_INVITE_INFO * ) GetWindowLong( hwndDlg, GWL_USERDATA );
				strncpy( roomJid, inviteInfo->roomJid, sizeof( roomJid ));
				room = strtok( roomJid, "@" );
				server = strtok( NULL, "@" );
				GetDlgItemText( hwndDlg, IDC_NICK, text, sizeof( text ));
				nick = JabberTextEncode( text );
				JabberGroupchatJoinRoom( server, room, nick, inviteInfo->password );
				free( nick );
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
	free( inviteInfo->roomJid );
	free( inviteInfo->from );
	free( inviteInfo->reason );
	free( inviteInfo->password );
	free( inviteInfo );
}

void JabberGroupchatProcessInvite( char* roomJid, char* from, char* reason, char* password )
{
	JABBER_GROUPCHAT_INVITE_INFO *inviteInfo;

	if ( roomJid == NULL )
		return;

	inviteInfo = ( JABBER_GROUPCHAT_INVITE_INFO * ) malloc( sizeof( JABBER_GROUPCHAT_INVITE_INFO ));
	inviteInfo->roomJid = _strdup( roomJid );
	inviteInfo->from = ( from!=NULL )?_strdup( from ):NULL;
	inviteInfo->reason = ( reason!=NULL )?_strdup( reason ):NULL;
	inviteInfo->password = ( password!=NULL )?_strdup( password ):NULL;
	JabberForkThread(( JABBER_THREAD_FUNC )JabberGroupchatInviteAcceptThread, 0, ( void * ) inviteInfo );
}