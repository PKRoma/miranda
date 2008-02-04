/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
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

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_list.h"
#include "resource.h"
#include "jabber_caps.h"
#include "jabber_privacy.h"
#include "jabber_disco.h"

#include <m_genmenu.h>
#include <m_contacts.h>
#include <m_hotkeys.h>

#include "sdk/m_toolbar.h"

/////////////////////////////////////////////////////////////////////////////////////////
// module data

#define MENUITEM_LASTSEEN	1
#define MENUITEM_SERVER		2
#define MENUITEM_RESOURCES	10

/////////////////////////////////////////////////////////////////////////////////////////
// contact menu services

static void sttEnableMenuItem( HANDLE hMenuItem, BOOL bEnable )
{
	CLISTMENUITEM clmi = {0};
	clmi.cbSize = sizeof( CLISTMENUITEM );
	clmi.flags = CMIM_FLAGS;
	if ( !bEnable )
		clmi.flags |= CMIF_HIDDEN;

	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuItem, ( LPARAM )&clmi );
}

int CJabberProto::OnPrebuildContactMenu( WPARAM wParam, LPARAM lParam )
{
	sttEnableMenuItem( hMenuRequestAuth, FALSE );
	sttEnableMenuItem( hMenuGrantAuth, FALSE );
	sttEnableMenuItem( hMenuRevokeAuth, FALSE );
	sttEnableMenuItem( hMenuJoinLeave, FALSE );
	sttEnableMenuItem( hMenuCommands, FALSE );
	sttEnableMenuItem( hMenuConvert, FALSE );
	sttEnableMenuItem( hMenuRosterAdd, FALSE );
	sttEnableMenuItem( hMenuLogin, FALSE );
	sttEnableMenuItem( hMenuRefresh, FALSE );
	sttEnableMenuItem( hMenuAddBookmark, FALSE );
	sttEnableMenuItem( hMenuResourcesRoot, FALSE );

	HANDLE hContact;
	if (( hContact=( HANDLE )wParam ) == NULL )
		return 0;

	BYTE bIsChatRoom  = (BYTE)JGetByte( hContact, "ChatRoom", 0 );
	BYTE bIsTransport = (BYTE)JGetByte( hContact, "IsTransport", 0 );

	if ((bIsChatRoom == GCW_CHATROOM) || bIsChatRoom == 0 ) {
		DBVARIANT dbv;
		if ( !JGetStringT( hContact, bIsChatRoom?(char*)"ChatRoomID":(char*)"jid", &dbv )) {
			JFreeVariant( &dbv );
			CLISTMENUITEM clmi = { 0 };
			sttEnableMenuItem( hMenuConvert, TRUE );
			clmi.cbSize = sizeof( clmi );
			clmi.pszName = bIsChatRoom ? LPGEN("&Convert to Contact") : LPGEN("&Convert to Chat Room");
			clmi.flags = CMIM_NAME | CMIM_FLAGS;
			JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuConvert, ( LPARAM )&clmi );
	}	}

	if (!jabberOnline)
		return 0;

	if ( bIsChatRoom ) {
		DBVARIANT dbv;
		if ( !JGetStringT( hContact, "ChatRoomID", &dbv )) {
			if ( JabberListGetItemPtr( LIST_ROSTER, dbv.ptszVal ) == NULL )
				sttEnableMenuItem( hMenuRosterAdd, TRUE );

			if ( JabberListGetItemPtr( LIST_BOOKMARK, dbv.ptszVal ) == NULL )
				if ( jabberThreadInfo && jabberThreadInfo->jabberServerCaps & JABBER_CAPS_PRIVATE_STORAGE )
					sttEnableMenuItem( hMenuAddBookmark, TRUE );

			JFreeVariant( &dbv );
	}	}

	if ( bIsChatRoom == GCW_CHATROOM ) {
		CLISTMENUITEM clmi = { 0 };
		clmi.cbSize = sizeof( clmi );
		clmi.pszName = ( JGetWord( hContact, "Status", 0 ) == ID_STATUS_ONLINE ) ? LPGEN("&Leave") : LPGEN("&Join");
		clmi.flags = CMIM_NAME | CMIM_FLAGS;
		JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuJoinLeave, ( LPARAM )&clmi );
		return 0;
	}

	if ( bIsTransport ) {
		sttEnableMenuItem( hMenuLogin, TRUE );
		sttEnableMenuItem( hMenuRefresh, TRUE );
	}

	DBVARIANT dbv;
	if ( !JGetStringT( hContact, "jid", &dbv )) {
		JabberCapsBits jcb = JabberGetTotalJidCapabilites(dbv.ptszVal );
		JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_ROSTER, dbv.ptszVal );
		JFreeVariant( &dbv );
		if ( item != NULL ) {
			BOOL bCtrlPressed = ( GetKeyState( VK_CONTROL)&0x8000 ) != 0;
			sttEnableMenuItem( hMenuRequestAuth, item->subscription == SUB_FROM || item->subscription == SUB_NONE || bCtrlPressed );
			sttEnableMenuItem( hMenuGrantAuth, item->subscription == SUB_TO || item->subscription == SUB_NONE || bCtrlPressed );
			sttEnableMenuItem( hMenuRevokeAuth, item->subscription == SUB_FROM || item->subscription == SUB_BOTH || bCtrlPressed );
			sttEnableMenuItem( hMenuCommands, (( jcb & JABBER_CAPS_COMMANDS ) != 0) || bCtrlPressed);

			if ( item->resourceCount >= 1 ) {
				sttEnableMenuItem( hMenuResourcesRoot, TRUE );

				int nMenuResourceItemsNew = nMenuResourceItems;
				if ( nMenuResourceItems < item->resourceCount ) {
					hMenuResourceItems = (HANDLE *)mir_realloc( hMenuResourceItems, item->resourceCount * sizeof(HANDLE) );
					nMenuResourceItemsNew = item->resourceCount;
				}

				char text[ 200 ];

				CLISTMENUITEM mi = { 0 };
				mi.cbSize = sizeof( CLISTMENUITEM );
				mi.flags = CMIF_CHILDPOPUP;
				mi.position = 0;
				mi.icolibItem = GetIconHandle( IDI_REQUEST );
				mi.pszService = text;
				mi.pszContactOwner = m_szProtoName;

				TCHAR szTmp[512];
				for (int i = 0; i < nMenuResourceItemsNew; ++i) {
					mir_snprintf( text, SIZEOF(text), "/UseResource_%d", i );
					if ( i >= nMenuResourceItems ) {
						JCreateServiceParam( text, &CJabberProto::JabberMenuHandleResource, MENUITEM_RESOURCES+i );
						JCreateService( text, &CJabberProto::JabberMenuHandleRequestAuth );
						mi.pszName = "";
						mi.position = i;
						mi.pszPopupName = (char *)hMenuResourcesRoot;
						hMenuResourceItems[i] = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
					}
					if ( i < item->resourceCount ) {
						CLISTMENUITEM clmi = {0};
						clmi.cbSize = sizeof( CLISTMENUITEM );
						clmi.flags = CMIM_NAME|CMIM_FLAGS | CMIF_CHILDPOPUP|CMIF_TCHAR;
						if ((item->resourceMode == RSMODE_MANUAL) && (item->manualResource == i))
							clmi.flags |= CMIF_CHECKED;
						if (ServiceExists( "Fingerprint/GetClientIcon" )) {
							clmi.flags |= CMIM_ICON;
							JabberFormatMirVer(&item->resource[i], szTmp, SIZEOF(szTmp));
							char *szMirver = mir_t2a(szTmp);
							clmi.hIcon = (HICON)CallService( "Fingerprint/GetClientIcon", (WPARAM)szMirver, 1 );
							mir_free( szMirver );
						}
						mir_sntprintf(szTmp, SIZEOF(szTmp), _T("%s [%s, %d]"),
							item->resource[i].resourceName,
							(TCHAR *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, item->resource[i].status, GCMDF_TCHAR),
							item->resource[i].priority);
						clmi.ptszName = szTmp;
						CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuResourceItems[i], ( LPARAM )&clmi );
					}
					else sttEnableMenuItem( hMenuResourceItems[i], FALSE );
				}

				ZeroMemory(&mi, sizeof(mi));
				mi.cbSize = sizeof( CLISTMENUITEM );

				mi.flags = CMIM_FLAGS | CMIF_CHILDPOPUP|CMIF_ICONFROMICOLIB |
					((item->resourceMode == RSMODE_LASTSEEN) ? CMIF_CHECKED : 0);
				CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuResourcesActive, ( LPARAM )&mi );

				mi.flags = CMIM_FLAGS | CMIF_CHILDPOPUP|CMIF_ICONFROMICOLIB |
					((item->resourceMode == RSMODE_SERVER) ? CMIF_CHECKED : 0);
				CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuResourcesServer, ( LPARAM )&mi );

				nMenuResourceItems = nMenuResourceItemsNew;
			}

			return 0;
	}	}

	return 0;
}

int __cdecl CJabberProto::JabberMenuConvertChatContact( WPARAM wParam, LPARAM lParam )
{
	BYTE bIsChatRoom = (BYTE)JGetByte( (HANDLE ) wParam, "ChatRoom", 0 );
	if ((bIsChatRoom == GCW_CHATROOM) || bIsChatRoom == 0 ) {
		DBVARIANT dbv;
		if ( !JGetStringT( (HANDLE ) wParam, (bIsChatRoom == GCW_CHATROOM)?(char*)"ChatRoomID":(char*)"jid", &dbv )) {
			JDeleteSetting( (HANDLE ) wParam, (bIsChatRoom == GCW_CHATROOM)?"ChatRoomID":"jid");
			JSetStringT( (HANDLE ) wParam, (bIsChatRoom != GCW_CHATROOM)?"ChatRoomID":"jid", dbv.ptszVal);
			JFreeVariant( &dbv );
			JSetByte((HANDLE ) wParam, "ChatRoom", (bIsChatRoom == GCW_CHATROOM)?0:GCW_CHATROOM);
	}	}
	return 0;
}

int __cdecl CJabberProto::JabberMenuRosterAdd( WPARAM wParam, LPARAM lParam )
{
	DBVARIANT dbv;
	if ( !wParam ) return 0; // we do not add ourself to the roster. (buggy situation - should not happen)
	if ( !JGetStringT( ( HANDLE ) wParam, "ChatRoomID", &dbv )) {
		TCHAR *roomID = mir_tstrdup(dbv.ptszVal);
		JFreeVariant( &dbv );
		if ( JabberListGetItemPtr( LIST_ROSTER, roomID ) == NULL ) {
			TCHAR *nick = 0;
			TCHAR *group = 0;
			if ( !DBGetContactSettingTString( ( HANDLE ) wParam, "CList", "Group", &dbv ) ) {
				group = mir_tstrdup(dbv.ptszVal);
				JFreeVariant( &dbv );
			}
			if ( !JGetStringT( ( HANDLE ) wParam, "Nick", &dbv ) ) {
				nick = mir_tstrdup(dbv.ptszVal);
				JFreeVariant( &dbv );
			}
			JabberAddContactToRoster(roomID, nick, group, SUB_NONE);
			if ( JGetByte( "AddRoster2Bookmarks", TRUE ) == TRUE ) {

				JABBER_LIST_ITEM* item = NULL;
				
				item = JabberListGetItemPtr(LIST_BOOKMARK, roomID);
				if (!item) {
					item = ( JABBER_LIST_ITEM* )mir_alloc( sizeof( JABBER_LIST_ITEM ));
					ZeroMemory( item, sizeof( JABBER_LIST_ITEM ));
					item->jid = mir_tstrdup(roomID);
					item->name = mir_tstrdup(nick);
					if ( !JGetStringT( ( HANDLE ) wParam, "MyNick", &dbv ) ) {
						item->nick = mir_tstrdup(dbv.ptszVal);
						JFreeVariant( &dbv );
					}
					JabberAddEditBookmark( item );
					mir_free(item);
				}
			}
			if (nick) mir_free(nick);
			if (nick) mir_free(group);
		}
		mir_free(roomID);
	}
	return 0;
}

int __cdecl CJabberProto::JabberMenuHandleRequestAuth( WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact;
	DBVARIANT dbv;

	if (( hContact=( HANDLE ) wParam )!=NULL && jabberOnline ) {
		if ( !JGetStringT( hContact, "jid", &dbv )) {
			XmlNode presence( "presence" ); presence.addAttr( "to", dbv.ptszVal ); presence.addAttr( "type", "subscribe" );
			jabberThreadInfo->send( presence );
			JFreeVariant( &dbv );
	}	}

	return 0;
}

int __cdecl CJabberProto::JabberMenuHandleGrantAuth( WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact;
	DBVARIANT dbv;

	if (( hContact=( HANDLE ) wParam )!=NULL && jabberOnline ) {
		if ( !JGetStringT( hContact, "jid", &dbv )) {
			XmlNode presence( "presence" ); presence.addAttr( "to", dbv.ptszVal ); presence.addAttr( "type", "subscribed" );
			jabberThreadInfo->send( presence );
			JFreeVariant( &dbv );
	}	}

	return 0;
}

int __cdecl CJabberProto::JabberMenuRevokeAuth( WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact;
	DBVARIANT dbv;

	if (( hContact=( HANDLE ) wParam ) != NULL && jabberOnline ) {
		if ( !JGetStringT( hContact, "jid", &dbv )) {
			XmlNode presence( "presence" ); presence.addAttr( "to", dbv.ptszVal ); presence.addAttr( "type", "unsubscribed" );
			jabberThreadInfo->send( presence );
			JFreeVariant( &dbv );
	}	}

	return 0;
}

int __cdecl CJabberProto::JabberMenuJoinLeave( WPARAM wParam, LPARAM lParam )
{
	DBVARIANT dbv, jid;
	if ( JGetStringT(( HANDLE )wParam, "ChatRoomID", &jid ))
		return 0;

	if ( JGetStringT(( HANDLE )wParam, "MyNick", &dbv ))
		if ( JGetStringT( NULL, "Nick", &dbv )) {
			JFreeVariant( &jid );
			return 0;
		}

	if ( JGetWord(( HANDLE )wParam, "Status", 0 ) != ID_STATUS_ONLINE ) {
		if ( !jabberChatDllPresent ) {
			JabberChatDllError();
			goto LBL_Return;
		}

		TCHAR* p = _tcschr( jid.ptszVal, '@' );
		if ( p == NULL )
			goto LBL_Return;

		*p++ = 0;
		JabberGroupchatJoinRoom( p, jid.ptszVal, dbv.ptszVal, _T(""));
	}
	else {
		JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_CHATROOM, jid.ptszVal );
		if ( item != NULL )
			JabberGcQuit( item, 0, NULL );
	}

LBL_Return:
	JFreeVariant( &dbv );
	JFreeVariant( &jid );
	return 0;
}

int __cdecl CJabberProto::JabberMenuTransportLogin( WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact = ( HANDLE )wParam;
	if ( !JGetByte( hContact, "IsTransport", 0 ))
		return 0;

	DBVARIANT jid;
	if ( JGetStringT( hContact, "jid", &jid ))
		return 0;

	JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_ROSTER, jid.ptszVal );
	if ( item != NULL ) {
		XmlNode p( "presence" ); p.addAttr( "to", item->jid );
		if ( item->itemResource.status == ID_STATUS_ONLINE )
			p.addAttr( "type", "unavailable" );
		jabberThreadInfo->send( p );
	}

	JFreeVariant( &jid );
	return 0;
}

int __cdecl CJabberProto::JabberMenuTransportResolve( WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact = ( HANDLE )wParam;
	if ( !JGetByte( hContact, "IsTransport", 0 ))
		return 0;

	DBVARIANT jid;
	if ( !JGetStringT( hContact, "jid", &jid )) {
		JabberResolveTransportNicks( jid.ptszVal );
		JFreeVariant( &jid );
	}
	return 0;
}

int __cdecl CJabberProto::JabberMenuBookmarkAdd( WPARAM wParam, LPARAM lParam )
{
	DBVARIANT dbv;
	if ( !wParam ) return 0; // we do not add ourself to the roster. (buggy situation - should not happen)
	if ( !JGetStringT( ( HANDLE ) wParam, "ChatRoomID", &dbv )) {
		TCHAR *roomID = mir_tstrdup(dbv.ptszVal);
		JFreeVariant( &dbv );
		if ( JabberListGetItemPtr( LIST_BOOKMARK, roomID ) == NULL ) {
			TCHAR *nick = 0;
			if ( !JGetStringT( ( HANDLE ) wParam, "Nick", &dbv ) ) {
				nick = mir_tstrdup(dbv.ptszVal);
				JFreeVariant( &dbv );
			}
			JABBER_LIST_ITEM* item = NULL;

			item = ( JABBER_LIST_ITEM* )mir_alloc( sizeof( JABBER_LIST_ITEM ));
			ZeroMemory( item, sizeof( JABBER_LIST_ITEM ));
			item->jid = mir_tstrdup(roomID);
			item->name = ( TCHAR* )JCallService( MS_CLIST_GETCONTACTDISPLAYNAME, wParam, GCDNF_TCHAR );
			item->type = _T("conference");
			if ( !JGetStringT(( HANDLE ) wParam, "MyNick", &dbv ) ) {
				item->nick = mir_tstrdup(dbv.ptszVal);
				JFreeVariant( &dbv );
			}
			JabberAddEditBookmark( item );
			mir_free(item);
			
			if (nick) mir_free(nick);
		}
		mir_free(roomID);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// contact menu initialization code

void CJabberProto::JabberMenuInit()
{
	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof( CLISTMENUITEM );

	char text[ 200 ];
	strcpy( text, m_szProtoName );
	char* tDest = text + strlen( text );

	//////////////////////////////////////////////////////////////////////////////////////
	// Contact menu initialization

	// "Request authorization"
	JCreateService( "/RequestAuth", &CJabberProto::JabberMenuHandleRequestAuth );
	strcpy( tDest, "/RequestAuth" );
	mi.pszName = LPGEN("Request authorization");
	mi.flags = CMIF_ICONFROMICOLIB;
	mi.position = -2000001000;
	mi.icolibItem = GetIconHandle( IDI_REQUEST );
	mi.pszService = text;
	mi.pszContactOwner = m_szProtoName;
	hMenuRequestAuth = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Grant authorization"
	JCreateService( "/GrantAuth", &CJabberProto::JabberMenuHandleGrantAuth );
	strcpy( tDest, "/GrantAuth" );
	mi.pszName = LPGEN("Grant authorization");
	mi.position = -2000001001;
	mi.icolibItem = GetIconHandle( IDI_GRANT );
	hMenuGrantAuth = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Revoke auth
	JCreateService( "/RevokeAuth", &CJabberProto::JabberMenuRevokeAuth );
	strcpy( tDest, "/RevokeAuth" );
	mi.pszName = LPGEN("Revoke authorization");
	mi.position = -2000001002;
	mi.icolibItem = GetIconHandle( IDI_AUTHREVOKE );
	hMenuRevokeAuth = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Grant authorization"
	JCreateService( "/JoinChat", &CJabberProto::JabberMenuJoinLeave );
	strcpy( tDest, "/JoinChat" );
	mi.pszName = LPGEN("Join chat");
	mi.position = -2000001003;
	mi.icolibItem = GetIconHandle( IDI_GROUP );
	hMenuJoinLeave = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Convert Chat/Contact"
	JCreateService( "/ConvertChatContact", &CJabberProto::JabberMenuConvertChatContact );
	strcpy( tDest, "/ConvertChatContact" );
	mi.pszName = LPGEN("Convert");
	mi.position = -1999901004;
	mi.icolibItem = GetIconHandle( IDI_USER2ROOM );
	hMenuConvert = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Add to roster"
	JCreateService( "/AddToRoster", &CJabberProto::JabberMenuRosterAdd );
	strcpy( tDest, "/AddToRoster" );
	mi.pszName = LPGEN("Add to roster");
	mi.position = -1999901005;
	mi.icolibItem = GetIconHandle( IDI_ADDROSTER );
	hMenuRosterAdd = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Add to Bookmarks"
	JCreateService( "/AddToBookmarks", &CJabberProto::JabberMenuBookmarkAdd );
	strcpy( tDest, "/AddToBookmarks" );
	mi.pszName = LPGEN("Add to Bookmarks");
	mi.position = -1999901006;
	mi.icolibItem = GetIconHandle( IDI_BOOKMARKS);
	hMenuAddBookmark= ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Login/logout
	JCreateService( "/TransportLogin", &CJabberProto::JabberMenuTransportLogin );
	strcpy( tDest, "/TransportLogin" );
	mi.pszName = LPGEN("Login/logout");
	mi.position = -1999901007;
	mi.icolibItem = GetIconHandle( IDI_LOGIN );
	hMenuLogin = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Retrieve nicks
	JCreateService( "/TransportGetNicks", &CJabberProto::JabberMenuTransportResolve );
	strcpy( tDest, "/TransportGetNicks" );
	mi.pszName = LPGEN("Resolve nicks");
	mi.position = -1999901008;
	mi.icolibItem = GetIconHandle( IDI_REFRESH );
	hMenuRefresh = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Run Commands
	JCreateService( "/RunCommands", &CJabberProto::JabberContactMenuRunCommands );
	strcpy( tDest, "/RunCommands" );
	mi.pszName = LPGEN("Commands");
	mi.position = -1999901009;
	mi.icolibItem = GetIconHandle( IDI_COMMAND );
	hMenuCommands = ( HANDLE )JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Resource selector
	mi.pszName = LPGEN("Jabber Resource");
	mi.position = -1999901010;
	mi.pszPopupName = (char *)-1;
	mi.flags |= CMIF_ROOTPOPUP;
	mi.icolibItem = GetIconHandle( IDI_JABBER );
	hMenuResourcesRoot = ( HANDLE )JCallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);

	mi.flags &= ~CMIF_ROOTPOPUP;
	mi.flags |= CMIF_CHILDPOPUP;

	JCreateServiceParam( "/UseResource_last", &CJabberProto::JabberMenuHandleResource, MENUITEM_LASTSEEN );
	strcpy( tDest, "/UseResource_last" );
	mi.pszName = LPGEN("Last Active");
	mi.position = -1999901000;
	mi.pszPopupName = (char *)hMenuResourcesRoot;
	mi.icolibItem = GetIconHandle( IDI_JABBER );
	hMenuResourcesActive = ( HANDLE )CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);

	JCreateServiceParam( "/UseResource_server", &CJabberProto::JabberMenuHandleResource, MENUITEM_SERVER );
	strcpy( tDest, "/UseResource_server" );
	mi.pszName = LPGEN("Server's Choice");
	mi.position = -1999901000;
	mi.pszPopupName = (char *)hMenuResourcesRoot;
	mi.icolibItem = GetIconHandle( IDI_NODE_SERVER );
	hMenuResourcesServer = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);

	//////////////////////////////////////////////////////////////////////////////////////
	// Main menu initialization

	CLISTMENUITEM clmi;
	memset( &clmi, 0, sizeof( CLISTMENUITEM ));
	clmi.cbSize = sizeof( CLISTMENUITEM );
	clmi.flags = CMIM_FLAGS | CMIF_GRAYED;

	mi.popupPosition = 500083000;
	mi.pszService = text;
	mi.pszName = m_szProtoName;
	mi.position = -1999901009;
	mi.pszPopupName = (char *)-1;
	mi.flags = CMIF_ICONFROMICOLIB | CMIF_ROOTPOPUP;
	mi.icolibItem = GetIconHandle( IDI_JABBER );
	hMenuRoot = (HANDLE)CallService( MS_CLIST_ADDMAINMENUITEM,  (WPARAM)0, (LPARAM)&mi);

	// "Service Discovery..."
	JCreateService( "/ServiceDiscovery", &CJabberProto::JabberMenuHandleServiceDiscovery );
	strcpy( tDest, "/ServiceDiscovery" );
	mi.flags = CMIF_ICONFROMICOLIB | CMIF_CHILDPOPUP;
	mi.pszName = LPGEN("Service Discovery");
	mi.position = 2000050001;
	mi.icolibItem = GetIconHandle( IDI_SERVICE_DISCOVERY );
	mi.pszPopupName = (char *)hMenuRoot;
	hMenuServiceDiscovery = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuServiceDiscovery, ( LPARAM )&clmi );

	// "Bookmarks..."
	JCreateService( "/Bookmarks", &CJabberProto::JabberMenuHandleBookmarks );
	strcpy( tDest, "/Bookmarks" );
	mi.pszName = LPGEN("Bookmarks");
	mi.position = 2000050002;
	mi.icolibItem = GetIconHandle( IDI_BOOKMARKS );
	hMenuBookmarks = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuBookmarks, ( LPARAM )&clmi );

	JCreateService( "/SD/MyTransports", &CJabberProto::JabberMenuHandleServiceDiscoveryMyTransports );
	strcpy( tDest, "/SD/MyTransports" );
	mi.pszName = LPGEN("Registered Transports");
	mi.position = 2000050003;
	mi.icolibItem = GetIconHandle( IDI_TRANSPORTL );
	hMenuSDMyTransports = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuSDMyTransports, ( LPARAM )&clmi );

	JCreateService( "/SD/Transports", &CJabberProto::JabberMenuHandleServiceDiscoveryTransports );
	strcpy( tDest, "/SD/Transports" );
	mi.pszName = LPGEN("Local Server Transports");
	mi.position = 2000050004;
	mi.icolibItem = GetIconHandle( IDI_TRANSPORT );
	hMenuSDTransports = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuSDTransports, ( LPARAM )&clmi );

	JCreateService( "/SD/Conferences", &CJabberProto::JabberMenuHandleServiceDiscoveryConferences );
	strcpy( tDest, "/SD/Conferences" );
	mi.pszName = LPGEN("Browse Chatrooms");
	mi.position = 2000050005;
	mi.icolibItem = GetIconHandle( IDI_GROUP );
	hMenuSDConferences = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuSDConferences, ( LPARAM )&clmi );

	JCreateService( "/Groupchat", &CJabberProto::JabberMenuHandleJoinGroupchat );
	strcpy( tDest, "/Groupchat" );
	mi.pszName = LPGEN("Create/Join groupchat");
	mi.position = 2000050006;
	mi.icolibItem = GetIconHandle( IDI_GROUP );
	hMenuGroupchat = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuGroupchat, ( LPARAM )&clmi );

	// "Change Password..."
	JCreateService( "/ChangePassword", &CJabberProto::JabberMenuHandleChangePassword );
	strcpy( tDest, "/ChangePassword" );
	mi.pszName = LPGEN("Change Password");
	mi.position = 2000050007;
	mi.icolibItem = GetIconHandle( IDI_KEYS );
	hMenuChangePassword = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuChangePassword, ( LPARAM )&clmi );

	// "Personal vCard..."
	JCreateService( "/Vcard", &CJabberProto::JabberMenuHandleVcard );
	strcpy( tDest, "/Vcard" );
	mi.pszName = LPGEN("Personal vCard");
	mi.position = 2000050008;
	mi.icolibItem = GetIconHandle( IDI_VCARD );
	JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );

	// "Privacy lists..."
	JCreateService( "/PrivacyLists", &CJabberProto::JabberMenuHandlePrivacyLists );
	strcpy( tDest, "/PrivacyLists" );
	mi.pszName = LPGEN("Privacy Lists");
	mi.position = 2000050009;
	mi.icolibItem = GetIconHandle( IDI_PRIVACY_LISTS );
	hMenuPrivacyLists = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuPrivacyLists, ( LPARAM )&clmi );

	// "Roster editor"
	JCreateService( "/RosterEditor", &CJabberProto::JabberMenuHandleRosterControl );
	strcpy( tDest, "/RosterEditor" );
	mi.pszName = LPGEN("Roster editor");
	mi.position = 2000050009;
	mi.icolibItem = GetIconHandle( IDI_AGENTS );
	hMenuRosterControl = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuRosterControl, ( LPARAM )&clmi );

	// "XML Console"
	JCreateService( "/XMLConsole", &CJabberProto::JabberMenuHandleConsole );
	strcpy( tDest, "/XMLConsole" );
	mi.pszName = LPGEN("XML Console");
	mi.position = 2000050010;
	mi.icolibItem = GetIconHandle( IDI_CONSOLE );
	JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );

	//////////////////////////////////////////////////////////////////////////////////////
	// Hotkeys

	HOTKEYDESC hkd = {0};
	hkd.cbSize = sizeof(hkd);
	hkd.pszName = text;
	hkd.pszService = text;
	hkd.pszSection = m_szModuleName;

	strcpy(tDest, "/Groupchat");
	hkd.pszDescription = "Join conference";
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM)&hkd);

	strcpy(tDest, "/Bookmarks");
	hkd.pszDescription = "Open bookmarks";
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM)&hkd);

	strcpy(tDest, "/PrivacyLists");
	hkd.pszDescription = "Privacy lists";
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM)&hkd);

	strcpy(tDest, "/ServiceDiscovery");
	hkd.pszDescription = "Service discovery";
	CallService(MS_HOTKEY_REGISTER, 0, (LPARAM)&hkd);
}

int CJabberProto::OnModernToolbarInit(WPARAM, LPARAM)
{
	char text[100], tmpbuf[256];
	strcpy( text, m_szProtoName );
	char* tDest = text + strlen( text );

	TBButton button = {0};
	button.cbSize = sizeof(button);
	button.pszButtonID = text;
	button.pszServiceName = text;
	button.pszTooltipUp = button.pszTooltipUp = button.pszButtonName = tmpbuf;
	button.defPos = 1000;
	button.tbbFlags = TBBF_SHOWTOOLTIP|TBBF_VISIBLE;

	JCreateService( "/Groupchat", &CJabberProto::JabberMenuHandleJoinGroupchat );

	strcpy(tDest, "/Groupchat");
	mir_snprintf(tmpbuf, SIZEOF(tmpbuf), "Join conference (%s)", m_szModuleName);
	button.hSecondaryIconHandle = button.hPrimaryIconHandle = (HANDLE)GetIconHandle(IDI_GROUP);
	JCallService(MS_TB_ADDBUTTON, 0, (LPARAM)&button);

	JCreateService( "/Bookmarks", &CJabberProto::JabberMenuHandleBookmarks );

	strcpy(tDest, "/Bookmarks");
	mir_snprintf(tmpbuf, SIZEOF(tmpbuf), "Open bookmarks (%s)", m_szModuleName);
	button.hSecondaryIconHandle = button.hPrimaryIconHandle = (HANDLE)GetIconHandle(IDI_BOOKMARKS);
	button.defPos++;
	JCallService(MS_TB_ADDBUTTON, 0, (LPARAM)&button);

	JCreateService( "/ServiceDiscovery", &CJabberProto::JabberMenuHandleServiceDiscovery );

	strcpy(tDest, "/ServiceDiscovery");
	mir_snprintf(tmpbuf, SIZEOF(tmpbuf), "Service discovery (%s)", m_szModuleName);
	button.hSecondaryIconHandle = button.hPrimaryIconHandle = (HANDLE)GetIconHandle(IDI_SERVICE_DISCOVERY);
	button.defPos++;
	JCallService(MS_TB_ADDBUTTON, 0, (LPARAM)&button);
	return 0;
}

void CJabberProto::JabberMenuUninit()
{
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hMenuRequestAuth, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hMenuGrantAuth, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hMenuRevokeAuth, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hMenuJoinLeave, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hMenuConvert, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hMenuRosterAdd, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hMenuLogin, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hMenuRefresh, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hMenuAgent, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hMenuChangePassword, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hMenuGroupchat, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hMenuBookmarks, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hMenuAddBookmark, 0 );

	JCallService( MS_CLIST_REMOVEMAINMENUITEM, ( WPARAM )hMenuRoot, 0 );

	if ( hMenuResourceItems ) {
		for ( int i=0; i < nMenuResourceItems; i++ )
			JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hMenuResourceItems[i], 0 );
		mir_free(hMenuResourceItems);
		hMenuResourceItems = NULL;
	}
	nMenuResourceItems = 0;
}

void CJabberProto::JabberEnableMenuItems( BOOL bEnable )
{
	CLISTMENUITEM clmi = { 0 };
	clmi.cbSize = sizeof( CLISTMENUITEM );
	clmi.flags = CMIM_FLAGS;
	if ( !bEnable )
		clmi.flags |= CMIF_GRAYED;

	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuAgent, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuChangePassword, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuGroupchat, ( LPARAM )&clmi );

	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuPrivacyLists, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuRosterControl, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuServiceDiscovery, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuSDMyTransports, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuSDTransports, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuSDConferences, ( LPARAM )&clmi );

	if ( jabberThreadInfo && !( jabberThreadInfo->jabberServerCaps & JABBER_CAPS_PRIVATE_STORAGE))
		clmi.flags |= CMIF_GRAYED;
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuBookmarks, ( LPARAM )&clmi );

	JabberUpdateDialogs( bEnable );
}

//////////////////////////////////////////////////////////////////////////
// resource menu

static HANDLE hDialogsList = NULL;

void CJabberProto::JabberMenuHideSrmmIcon(HANDLE hContact)
{
	StatusIconData sid = {0};
	sid.cbSize = sizeof(sid);
	sid.szModule = m_szProtoName;
	sid.flags = MBF_HIDDEN;
	CallService(MS_MSG_MODIFYICON, (WPARAM)hContact, (LPARAM)&sid);
}

void CJabberProto::JabberMenuUpdateSrmmIcon(JABBER_LIST_ITEM *item)
{
	if ( item->list != LIST_ROSTER || !ServiceExists( MS_MSG_MODIFYICON ))
		return;

	HANDLE hContact = JabberHContactFromJID(item->jid);
	if ( !hContact )
		return;

	StatusIconData sid = {0};
	sid.cbSize = sizeof(sid);
	sid.szModule = m_szProtoName;
	sid.flags = item->resourceCount ? 0 : MBF_HIDDEN;
	CallService(MS_MSG_MODIFYICON, (WPARAM)hContact, (LPARAM)&sid);
}

int CJabberProto::OnProcessSrmmEvent( WPARAM wParam, LPARAM lParam )
{
	MessageWindowEventData *event = (MessageWindowEventData *)lParam;

	if ( event->uType == MSG_WINDOW_EVT_OPEN ) {
		if ( !hDialogsList )
			hDialogsList = (HANDLE)CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);
		WindowList_Add(hDialogsList, event->hwndWindow, event->hContact);
	}
	else if ( event->uType == MSG_WINDOW_EVT_CLOSING ) {
		if (hDialogsList)
			WindowList_Remove(hDialogsList, event->hwndWindow);

		DBVARIANT dbv;
		BOOL bSupportTyping = FALSE;
		if ( !DBGetContactSetting( event->hContact, "SRMsg", "SupportTyping", &dbv )) {
			bSupportTyping = dbv.bVal == 1;
			JFreeVariant( &dbv );
		} else if ( !DBGetContactSetting( NULL, "SRMsg", "DefaultTyping", &dbv )) {
			bSupportTyping = dbv.bVal == 1;
			JFreeVariant( &dbv );
		}
		if ( bSupportTyping && !JGetStringT( event->hContact, "jid", &dbv )) {
			TCHAR jid[ 256 ];
			JabberGetClientJID( dbv.ptszVal, jid, SIZEOF( jid ));
			JFreeVariant( &dbv );

			JABBER_RESOURCE_STATUS *r = JabberResourceInfoFromJID( jid );

			if ( r && r->bMessageSessionActive ) {
				r->bMessageSessionActive = FALSE;
				JabberCapsBits jcb = JabberGetResourceCapabilites( jid, TRUE );

				if ( jcb & JABBER_CAPS_CHATSTATES ) {
					int iqId = JabberSerialNext();
					XmlNode msg( "message" );
					msg.addAttr( "to", jid );
					msg.addAttr( "type", "chat" );
					msg.addAttrID( iqId );
					XmlNode *goneNode = msg.addChild( "gone" );
					goneNode->addAttr( "xmlns", JABBER_FEAT_CHATSTATES );

					jabberThreadInfo->send( msg );
	}	}	}	}

	return 0;
}

int CJabberProto::OnProcessSrmmIconClick( WPARAM wParam, LPARAM lParam )
{
	StatusIconClickData *sicd = (StatusIconClickData *)lParam;
	if (lstrcmpA(sicd->szModule, m_szProtoName))
		return 0;

	HANDLE hContact = (HANDLE)wParam;
	if (!hContact)
		return 0;

	DBVARIANT dbv;
	if (JGetStringT(hContact, "jid", &dbv))
		return 0;

	JABBER_LIST_ITEM *LI = JabberListGetItemPtr(LIST_ROSTER, dbv.ptszVal);
	JFreeVariant( &dbv );

	if ( !LI )
		return 0;

	HMENU hMenu = CreatePopupMenu();
	TCHAR buf[256];

	mir_sntprintf(buf, SIZEOF(buf), _T("%s (%s)"), TranslateT("Last active"),
		((LI->lastSeenResource>=0) && (LI->lastSeenResource < LI->resourceCount)) ?
			LI->resource[LI->lastSeenResource].resourceName : TranslateT("No activity yet, use server's choice"));
	AppendMenu(hMenu, MF_STRING, MENUITEM_LASTSEEN, buf);

	AppendMenu(hMenu, MF_STRING, MENUITEM_SERVER, TranslateT("Highest priority (server's choice)"));

	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	for (int i = 0; i < LI->resourceCount; ++i)
		AppendMenu(hMenu, MF_STRING, MENUITEM_RESOURCES+i, LI->resource[i].resourceName);

	if (LI->resourceMode == RSMODE_LASTSEEN)
		CheckMenuItem(hMenu, MENUITEM_LASTSEEN, MF_BYCOMMAND|MF_CHECKED);
	else if (LI->resourceMode == RSMODE_SERVER)
		CheckMenuItem(hMenu, MENUITEM_SERVER, MF_BYCOMMAND|MF_CHECKED);
	else
		CheckMenuItem(hMenu, MENUITEM_RESOURCES+LI->manualResource, MF_BYCOMMAND|MF_CHECKED);

	int res = TrackPopupMenu(hMenu, TPM_RETURNCMD, sicd->clickLocation.x, sicd->clickLocation.y, 0, WindowList_Find(hDialogsList, hContact), NULL);

	if ( res == MENUITEM_LASTSEEN ) {
		LI->manualResource = -1;
		LI->resourceMode = RSMODE_LASTSEEN;
	}
	else if (res == MENUITEM_SERVER) {
		LI->manualResource = -1;
		LI->resourceMode = RSMODE_SERVER;
	}
	else if (res >= MENUITEM_RESOURCES) {
		LI->manualResource = res - MENUITEM_RESOURCES;
		LI->resourceMode = RSMODE_MANUAL;
	}

	JabberUpdateMirVer(LI);
	JabberMenuUpdateSrmmIcon(LI);

	return 0;
}

int __cdecl CJabberProto::JabberMenuHandleResource(WPARAM wParam, LPARAM lParam, LPARAM res)
{
	if ( !jabberOnline || !wParam )
		return 0;

	HANDLE hContact = (HANDLE)wParam;

	DBVARIANT dbv;
	if (JGetStringT(hContact, "jid", &dbv))
		return 0;

	JABBER_LIST_ITEM *LI = JabberListGetItemPtr(LIST_ROSTER, dbv.ptszVal);
	JFreeVariant( &dbv );

	if ( !LI )
		return 0;

	if ( res == MENUITEM_LASTSEEN ) {
		LI->manualResource = -1;
		LI->resourceMode = RSMODE_LASTSEEN;
	}
	else if (res == MENUITEM_SERVER) {
		LI->manualResource = -1;
		LI->resourceMode = RSMODE_SERVER;
	}
	else if (res >= MENUITEM_RESOURCES) {
		LI->manualResource = res - MENUITEM_RESOURCES;
		LI->resourceMode = RSMODE_MANUAL;
	}

	JabberUpdateMirVer(LI);
	JabberMenuUpdateSrmmIcon(LI);
	return 0;
}
