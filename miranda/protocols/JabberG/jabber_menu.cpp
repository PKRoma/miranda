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
#include "jabber_caps.h"
#include "jabber_privacy.h"
#include "jabber_disco.h"

#include <m_genmenu.h>
#include <m_contacts.h>
#include <m_hotkeys.h>
#include <m_icolib.h>

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
	sttEnableMenuItem( m_hMenuRequestAuth, FALSE );
	sttEnableMenuItem( m_hMenuGrantAuth, FALSE );
	sttEnableMenuItem( m_hMenuRevokeAuth, FALSE );
	sttEnableMenuItem( m_hMenuJoinLeave, FALSE );
	sttEnableMenuItem( m_hMenuCommands, FALSE );
	sttEnableMenuItem( m_hMenuConvert, FALSE );
	sttEnableMenuItem( m_hMenuRosterAdd, FALSE );
	sttEnableMenuItem( m_hMenuLogin, FALSE );
	sttEnableMenuItem( m_hMenuRefresh, FALSE );
	sttEnableMenuItem( m_hMenuAddBookmark, FALSE );
	sttEnableMenuItem( m_hMenuResourcesRoot, FALSE );

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
			sttEnableMenuItem( m_hMenuConvert, TRUE );
			clmi.cbSize = sizeof( clmi );
			clmi.pszName = bIsChatRoom ? (char *)LPGEN("&Convert to Contact") : (char *)LPGEN("&Convert to Chat Room");
			clmi.flags = CMIM_NAME | CMIM_FLAGS;
			JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuConvert, ( LPARAM )&clmi );
	}	}

	if (!m_bJabberOnline)
		return 0;

	if ( bIsChatRoom ) {
		DBVARIANT dbv;
		if ( !JGetStringT( hContact, "ChatRoomID", &dbv )) {
			if ( ListGetItemPtr( LIST_ROSTER, dbv.ptszVal ) == NULL )
				sttEnableMenuItem( m_hMenuRosterAdd, TRUE );

			if ( ListGetItemPtr( LIST_BOOKMARK, dbv.ptszVal ) == NULL )
				if ( m_ThreadInfo && m_ThreadInfo->jabberServerCaps & JABBER_CAPS_PRIVATE_STORAGE )
					sttEnableMenuItem( m_hMenuAddBookmark, TRUE );

			JFreeVariant( &dbv );
	}	}

	if ( bIsChatRoom == GCW_CHATROOM ) {
		CLISTMENUITEM clmi = { 0 };
		clmi.cbSize = sizeof( clmi );
		clmi.pszName = ( JGetWord( hContact, "Status", 0 ) == ID_STATUS_ONLINE ) ? (char *)LPGEN("&Leave") : (char *)LPGEN("&Join");
		clmi.flags = CMIM_NAME | CMIM_FLAGS;
		JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuJoinLeave, ( LPARAM )&clmi );
		return 0;
	}

	if ( bIsTransport ) {
		sttEnableMenuItem( m_hMenuLogin, TRUE );
		sttEnableMenuItem( m_hMenuRefresh, TRUE );
	}

	DBVARIANT dbv;
	if ( !JGetStringT( hContact, "jid", &dbv )) {
		JabberCapsBits jcb = GetTotalJidCapabilites(dbv.ptszVal );
		JABBER_LIST_ITEM* item = ListGetItemPtr( LIST_ROSTER, dbv.ptszVal );
		JFreeVariant( &dbv );
		if ( item != NULL ) {
			BOOL bCtrlPressed = ( GetKeyState( VK_CONTROL)&0x8000 ) != 0;
			sttEnableMenuItem( m_hMenuRequestAuth, item->subscription == SUB_FROM || item->subscription == SUB_NONE || bCtrlPressed );
			sttEnableMenuItem( m_hMenuGrantAuth, bCtrlPressed );
			sttEnableMenuItem( m_hMenuRevokeAuth, item->subscription == SUB_FROM || item->subscription == SUB_BOTH || bCtrlPressed );
			sttEnableMenuItem( m_hMenuCommands, (( jcb & JABBER_CAPS_COMMANDS ) != 0) || bCtrlPressed);

			if ( item->resourceCount >= 1 ) {
				sttEnableMenuItem( m_hMenuResourcesRoot, TRUE );

				int nMenuResourceItemsNew = m_nMenuResourceItems;
				if ( m_nMenuResourceItems < item->resourceCount ) {
					m_phMenuResourceItems = (HANDLE *)mir_realloc( m_phMenuResourceItems, item->resourceCount * sizeof(HANDLE) );
					nMenuResourceItemsNew = item->resourceCount;
				}

				char text[ 200 ];

				CLISTMENUITEM mi = { 0 };
				mi.cbSize = sizeof( CLISTMENUITEM );
				mi.flags = CMIF_CHILDPOPUP;
				mi.position = 0;
				mi.icolibItem = GetIconHandle( IDI_REQUEST );
				mi.pszService = text;
				mi.pszContactOwner = m_szModuleName;

				TCHAR szTmp[512];
				for (int i = 0; i < nMenuResourceItemsNew; ++i) {
					mir_snprintf( text, SIZEOF(text), "/UseResource_%d", i );
					if ( i >= m_nMenuResourceItems ) {
						JCreateServiceParam( text, &CJabberProto::OnMenuHandleResource, MENUITEM_RESOURCES+i );
						JCreateService( text, &CJabberProto::OnMenuHandleRequestAuth );
						mi.pszName = "";
						mi.position = i;
						mi.pszPopupName = (char *)m_hMenuResourcesRoot;
						m_phMenuResourceItems[i] = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
					}
					if ( i < item->resourceCount ) {
						CLISTMENUITEM clmi = {0};
						clmi.cbSize = sizeof( CLISTMENUITEM );
						clmi.flags = CMIM_NAME|CMIM_FLAGS | CMIF_CHILDPOPUP|CMIF_TCHAR;
						if ((item->resourceMode == RSMODE_MANUAL) && (item->manualResource == i))
							clmi.flags |= CMIF_CHECKED;
						if (ServiceExists( "Fingerprint/GetClientIcon" )) {
							clmi.flags |= CMIM_ICON;
							FormatMirVer(&item->resource[i], szTmp, SIZEOF(szTmp));
							char *szMirver = mir_t2a(szTmp);
							clmi.hIcon = (HICON)CallService( "Fingerprint/GetClientIcon", (WPARAM)szMirver, 1 );
							mir_free( szMirver );
						}
						mir_sntprintf(szTmp, SIZEOF(szTmp), _T("%s [%s, %d]"),
							item->resource[i].resourceName,
							(TCHAR *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, item->resource[i].status, GCMDF_TCHAR),
							item->resource[i].priority);
						clmi.ptszName = szTmp;
						CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_phMenuResourceItems[i], ( LPARAM )&clmi );
					}
					else sttEnableMenuItem( m_phMenuResourceItems[i], FALSE );
				}

				ZeroMemory(&mi, sizeof(mi));
				mi.cbSize = sizeof( CLISTMENUITEM );

				mi.flags = CMIM_FLAGS | CMIF_CHILDPOPUP|CMIF_ICONFROMICOLIB |
					((item->resourceMode == RSMODE_LASTSEEN) ? CMIF_CHECKED : 0);
				CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuResourcesActive, ( LPARAM )&mi );

				mi.flags = CMIM_FLAGS | CMIF_CHILDPOPUP|CMIF_ICONFROMICOLIB |
					((item->resourceMode == RSMODE_SERVER) ? CMIF_CHECKED : 0);
				CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuResourcesServer, ( LPARAM )&mi );

				m_nMenuResourceItems = nMenuResourceItemsNew;
			}

			return 0;
	}	}

	return 0;
}

int __cdecl CJabberProto::OnBuildStatusMenu( WPARAM wParam, LPARAM lParam )
{
	BuildPrivacyMenu(wParam, lParam);
	BuildPriorityMenu(wParam, lParam);
	m_pepServices.RebuildMenu();
	return 0;
}

int __cdecl CJabberProto::OnMenuConvertChatContact( WPARAM wParam, LPARAM lParam )
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

int __cdecl CJabberProto::OnMenuRosterAdd( WPARAM wParam, LPARAM lParam )
{
	DBVARIANT dbv;
	if ( !wParam ) return 0; // we do not add ourself to the roster. (buggy situation - should not happen)
	if ( !JGetStringT( ( HANDLE ) wParam, "ChatRoomID", &dbv )) {
		TCHAR *roomID = mir_tstrdup(dbv.ptszVal);
		JFreeVariant( &dbv );
		if ( ListGetItemPtr( LIST_ROSTER, roomID ) == NULL ) {
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
			AddContactToRoster( roomID, nick, group );
			if ( JGetByte( "AddRoster2Bookmarks", TRUE ) == TRUE ) {

				JABBER_LIST_ITEM* item = NULL;
				
				item = ListGetItemPtr(LIST_BOOKMARK, roomID);
				if (!item) {
					item = ( JABBER_LIST_ITEM* )mir_alloc( sizeof( JABBER_LIST_ITEM ));
					ZeroMemory( item, sizeof( JABBER_LIST_ITEM ));
					item->jid = mir_tstrdup(roomID);
					item->name = mir_tstrdup(nick);
					if ( !JGetStringT( ( HANDLE ) wParam, "MyNick", &dbv ) ) {
						item->nick = mir_tstrdup(dbv.ptszVal);
						JFreeVariant( &dbv );
					}
					AddEditBookmark( item );
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

int __cdecl CJabberProto::OnMenuHandleRequestAuth( WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact;
	DBVARIANT dbv;

	if (( hContact=( HANDLE ) wParam )!=NULL && m_bJabberOnline ) {
		if ( !JGetStringT( hContact, "jid", &dbv )) {
			XmlNode presence( _T("presence")); xmlAddAttr( presence, "to", dbv.ptszVal ); xmlAddAttr( presence, "type", "subscribe" );
			m_ThreadInfo->send( presence );
			JFreeVariant( &dbv );
	}	}

	return 0;
}

int __cdecl CJabberProto::OnMenuHandleGrantAuth( WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact;
	DBVARIANT dbv;

	if (( hContact=( HANDLE ) wParam )!=NULL && m_bJabberOnline ) {
		if ( !JGetStringT( hContact, "jid", &dbv )) {
			XmlNode presence( _T("presence")); xmlAddAttr( presence, "to", dbv.ptszVal ); xmlAddAttr( presence, "type", "subscribed" );
			m_ThreadInfo->send( presence );
			JFreeVariant( &dbv );
	}	}

	return 0;
}

int __cdecl CJabberProto::OnMenuRevokeAuth( WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact;
	DBVARIANT dbv;

	if (( hContact=( HANDLE ) wParam ) != NULL && m_bJabberOnline ) {
		if ( !JGetStringT( hContact, "jid", &dbv )) {
			XmlNode presence( _T("presence")); xmlAddAttr( presence, "to", dbv.ptszVal ); xmlAddAttr( presence, "type", "unsubscribed" );
			m_ThreadInfo->send( presence );
			JFreeVariant( &dbv );
	}	}

	return 0;
}

int __cdecl CJabberProto::OnMenuJoinLeave( WPARAM wParam, LPARAM lParam )
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
		GroupchatJoinRoom( p, jid.ptszVal, dbv.ptszVal, _T(""));
	}
	else {
		JABBER_LIST_ITEM* item = ListGetItemPtr( LIST_CHATROOM, jid.ptszVal );
		if ( item != NULL )
			GcQuit( item, 0, NULL );
	}

LBL_Return:
	JFreeVariant( &dbv );
	JFreeVariant( &jid );
	return 0;
}

int __cdecl CJabberProto::OnMenuTransportLogin( WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact = ( HANDLE )wParam;
	if ( !JGetByte( hContact, "IsTransport", 0 ))
		return 0;

	DBVARIANT jid;
	if ( JGetStringT( hContact, "jid", &jid ))
		return 0;

	JABBER_LIST_ITEM* item = ListGetItemPtr( LIST_ROSTER, jid.ptszVal );
	if ( item != NULL ) {
		XmlNode p( _T("presence")); xmlAddAttr( p, "to", item->jid );
		if ( item->itemResource.status == ID_STATUS_ONLINE )
			xmlAddAttr( p, "type", "unavailable" );
		m_ThreadInfo->send( p );
	}

	JFreeVariant( &jid );
	return 0;
}

int __cdecl CJabberProto::OnMenuTransportResolve( WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact = ( HANDLE )wParam;
	if ( !JGetByte( hContact, "IsTransport", 0 ))
		return 0;

	DBVARIANT jid;
	if ( !JGetStringT( hContact, "jid", &jid )) {
		ResolveTransportNicks( jid.ptszVal );
		JFreeVariant( &jid );
	}
	return 0;
}

int __cdecl CJabberProto::OnMenuBookmarkAdd( WPARAM wParam, LPARAM lParam )
{
	DBVARIANT dbv;
	if ( !wParam ) return 0; // we do not add ourself to the roster. (buggy situation - should not happen)
	if ( !JGetStringT( ( HANDLE ) wParam, "ChatRoomID", &dbv )) {
		TCHAR *roomID = mir_tstrdup(dbv.ptszVal);
		JFreeVariant( &dbv );
		if ( ListGetItemPtr( LIST_BOOKMARK, roomID ) == NULL ) {
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
			AddEditBookmark( item );
			mir_free(item);
			
			if (nick) mir_free(nick);
		}
		mir_free(roomID);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// contact menu initialization code

void CJabberProto::MenuInit()
{
	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof( CLISTMENUITEM );

	char text[ 200 ];
	strcpy( text, m_szModuleName );
	char* tDest = text + strlen( text );

	//////////////////////////////////////////////////////////////////////////////////////
	// Contact menu initialization

	// "Request authorization"
	JCreateService( "/RequestAuth", &CJabberProto::OnMenuHandleRequestAuth );
	strcpy( tDest, "/RequestAuth" );
	mi.pszName = LPGEN("Request authorization");
	mi.flags = CMIF_ICONFROMICOLIB;
	mi.position = -2000001000;
	mi.icolibItem = GetIconHandle( IDI_REQUEST );
	mi.pszService = text;
	mi.pszContactOwner = m_szModuleName;
	m_hMenuRequestAuth = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Grant authorization"
	JCreateService( "/GrantAuth", &CJabberProto::OnMenuHandleGrantAuth );
	strcpy( tDest, "/GrantAuth" );
	mi.pszName = LPGEN("Grant authorization");
	mi.position = -2000001001;
	mi.icolibItem = GetIconHandle( IDI_GRANT );
	m_hMenuGrantAuth = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Revoke auth
	JCreateService( "/RevokeAuth", &CJabberProto::OnMenuRevokeAuth );
	strcpy( tDest, "/RevokeAuth" );
	mi.pszName = LPGEN("Revoke authorization");
	mi.position = -2000001002;
	mi.icolibItem = GetIconHandle( IDI_AUTHREVOKE );
	m_hMenuRevokeAuth = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Grant authorization"
	JCreateService( "/JoinChat", &CJabberProto::OnMenuJoinLeave );
	strcpy( tDest, "/JoinChat" );
	mi.pszName = LPGEN("Join chat");
	mi.position = -2000001003;
	mi.icolibItem = GetIconHandle( IDI_GROUP );
	m_hMenuJoinLeave = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Convert Chat/Contact"
	JCreateService( "/ConvertChatContact", &CJabberProto::OnMenuConvertChatContact );
	strcpy( tDest, "/ConvertChatContact" );
	mi.pszName = LPGEN("Convert");
	mi.position = -1999901004;
	mi.icolibItem = GetIconHandle( IDI_USER2ROOM );
	m_hMenuConvert = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Add to roster"
	JCreateService( "/AddToRoster", &CJabberProto::OnMenuRosterAdd );
	strcpy( tDest, "/AddToRoster" );
	mi.pszName = LPGEN("Add to roster");
	mi.position = -1999901005;
	mi.icolibItem = GetIconHandle( IDI_ADDROSTER );
	m_hMenuRosterAdd = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Add to Bookmarks"
	JCreateService( "/AddToBookmarks", &CJabberProto::OnMenuBookmarkAdd );
	strcpy( tDest, "/AddToBookmarks" );
	mi.pszName = LPGEN("Add to Bookmarks");
	mi.position = -1999901006;
	mi.icolibItem = GetIconHandle( IDI_BOOKMARKS);
	m_hMenuAddBookmark= ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Login/logout
	JCreateService( "/TransportLogin", &CJabberProto::OnMenuTransportLogin );
	strcpy( tDest, "/TransportLogin" );
	mi.pszName = LPGEN("Login/logout");
	mi.position = -1999901007;
	mi.icolibItem = GetIconHandle( IDI_LOGIN );
	m_hMenuLogin = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Retrieve nicks
	JCreateService( "/TransportGetNicks", &CJabberProto::OnMenuTransportResolve );
	strcpy( tDest, "/TransportGetNicks" );
	mi.pszName = LPGEN("Resolve nicks");
	mi.position = -1999901008;
	mi.icolibItem = GetIconHandle( IDI_REFRESH );
	m_hMenuRefresh = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Run Commands
	JCreateService( "/RunCommands", &CJabberProto::ContactMenuRunCommands );
	strcpy( tDest, "/RunCommands" );
	mi.pszName = LPGEN("Commands");
	mi.position = -1999901009;
	mi.icolibItem = GetIconHandle( IDI_COMMAND );
	m_hMenuCommands = ( HANDLE )JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Resource selector
	mi.pszName = LPGEN("Jabber Resource");
	mi.position = -1999901010;
	mi.pszPopupName = (char *)-1;
	mi.flags |= CMIF_ROOTPOPUP;
	mi.icolibItem = GetIconHandle( IDI_JABBER );
	m_hMenuResourcesRoot = ( HANDLE )JCallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);

	mi.flags &= ~CMIF_ROOTPOPUP;
	mi.flags |= CMIF_CHILDPOPUP;

	JCreateServiceParam( "/UseResource_last", &CJabberProto::OnMenuHandleResource, MENUITEM_LASTSEEN );
	strcpy( tDest, "/UseResource_last" );
	mi.pszName = LPGEN("Last Active");
	mi.position = -1999901000;
	mi.pszPopupName = (char *)m_hMenuResourcesRoot;
	mi.icolibItem = GetIconHandle( IDI_JABBER );
	m_hMenuResourcesActive = ( HANDLE )CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);

	JCreateServiceParam( "/UseResource_server", &CJabberProto::OnMenuHandleResource, MENUITEM_SERVER );
	strcpy( tDest, "/UseResource_server" );
	mi.pszName = LPGEN("Server's Choice");
	mi.position = -1999901000;
	mi.pszPopupName = (char *)m_hMenuResourcesRoot;
	mi.icolibItem = GetIconHandle( IDI_NODE_SERVER );
	m_hMenuResourcesServer = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);

	//////////////////////////////////////////////////////////////////////////////////////
	// Main menu initialization

	CLISTMENUITEM clmi;
	memset( &clmi, 0, sizeof( CLISTMENUITEM ));
	clmi.cbSize = sizeof( CLISTMENUITEM );
	clmi.flags = CMIM_FLAGS | CMIF_GRAYED;

	mi.popupPosition = 500083000;
	mi.pszService = text;
	mi.ptszName = m_tszUserName;
	mi.position = -1999901009;
	mi.pszPopupName = (char *)-1;
	mi.flags = CMIF_ICONFROMICOLIB | CMIF_ROOTPOPUP | CMIF_TCHAR;
	mi.icolibItem = GetIconHandle( IDI_JABBER );
	m_hMenuRoot = (HANDLE)CallService( MS_CLIST_ADDMAINMENUITEM,  (WPARAM)0, (LPARAM)&mi);

	// "Service Discovery..."
	JCreateService( "/ServiceDiscovery", &CJabberProto::OnMenuHandleServiceDiscovery );
	strcpy( tDest, "/ServiceDiscovery" );
	mi.flags = CMIF_ICONFROMICOLIB | CMIF_CHILDPOPUP;
	mi.pszName = LPGEN("Service Discovery");
	mi.position = 2000050001;
	mi.icolibItem = GetIconHandle( IDI_SERVICE_DISCOVERY );
	mi.pszPopupName = (char *)m_hMenuRoot;
	m_hMenuServiceDiscovery = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) m_hMenuServiceDiscovery, ( LPARAM )&clmi );

	// "Bookmarks..."
	JCreateService( "/Bookmarks", &CJabberProto::OnMenuHandleBookmarks );
	strcpy( tDest, "/Bookmarks" );
	mi.pszName = LPGEN("Bookmarks");
	mi.position = 2000050002;
	mi.icolibItem = GetIconHandle( IDI_BOOKMARKS );
	m_hMenuBookmarks = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) m_hMenuBookmarks, ( LPARAM )&clmi );

	JCreateService( "/SD/MyTransports", &CJabberProto::OnMenuHandleServiceDiscoveryMyTransports );
	strcpy( tDest, "/SD/MyTransports" );
	mi.pszName = LPGEN("Registered Transports");
	mi.position = 2000050003;
	mi.icolibItem = GetIconHandle( IDI_TRANSPORTL );
	m_hMenuSDMyTransports = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) m_hMenuSDMyTransports, ( LPARAM )&clmi );

	JCreateService( "/SD/Transports", &CJabberProto::OnMenuHandleServiceDiscoveryTransports );
	strcpy( tDest, "/SD/Transports" );
	mi.pszName = LPGEN("Local Server Transports");
	mi.position = 2000050004;
	mi.icolibItem = GetIconHandle( IDI_TRANSPORT );
	m_hMenuSDTransports = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) m_hMenuSDTransports, ( LPARAM )&clmi );

	JCreateService( "/SD/Conferences", &CJabberProto::OnMenuHandleServiceDiscoveryConferences );
	strcpy( tDest, "/SD/Conferences" );
	mi.pszName = LPGEN("Browse Chatrooms");
	mi.position = 2000050005;
	mi.icolibItem = GetIconHandle( IDI_GROUP );
	m_hMenuSDConferences = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) m_hMenuSDConferences, ( LPARAM )&clmi );

	JCreateService( "/Groupchat", &CJabberProto::OnMenuHandleJoinGroupchat );
	strcpy( tDest, "/Groupchat" );
	mi.pszName = LPGEN("Create/Join groupchat");
	mi.position = 2000050006;
	mi.icolibItem = GetIconHandle( IDI_GROUP );
	m_hMenuGroupchat = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) m_hMenuGroupchat, ( LPARAM )&clmi );

	// "Change Password..."
	JCreateService( "/ChangePassword", &CJabberProto::OnMenuHandleChangePassword );
	strcpy( tDest, "/ChangePassword" );
	mi.pszName = LPGEN("Change Password");
	mi.position = 2000050007;
	mi.icolibItem = GetIconHandle( IDI_KEYS );
	m_hMenuChangePassword = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) m_hMenuChangePassword, ( LPARAM )&clmi );

	// "Privacy lists..."
	JCreateService( "/PrivacyLists", &CJabberProto::OnMenuHandlePrivacyLists );
	strcpy( tDest, "/PrivacyLists" );
	mi.pszName = LPGEN("Privacy Lists");
	mi.position = 2000050009;
	mi.icolibItem = GetIconHandle( IDI_PRIVACY_LISTS );
	m_hMenuPrivacyLists = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) m_hMenuPrivacyLists, ( LPARAM )&clmi );

	// "Roster editor"
	JCreateService( "/RosterEditor", &CJabberProto::OnMenuHandleRosterControl );
	strcpy( tDest, "/RosterEditor" );
	mi.pszName = LPGEN("Roster editor");
	mi.position = 2000050009;
	mi.icolibItem = GetIconHandle( IDI_AGENTS );
	m_hMenuRosterControl = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) m_hMenuRosterControl, ( LPARAM )&clmi );

	// "XML Console"
	JCreateService( "/XMLConsole", &CJabberProto::OnMenuHandleConsole );
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
	hkd.pszSection = m_szModuleName;	// title!!!!!!!!!!!

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

static int g_ToolbarHandleJoinGroupchat(WPARAM w, LPARAM l)
{
	if (CJabberProto *ppro = JabberChooseInstance())
		return ppro->OnMenuHandleJoinGroupchat(w, l);
	return 0;
}

static int g_ToolbarHandleBookmarks(WPARAM w, LPARAM l)
{
	if (CJabberProto *ppro = JabberChooseInstance())
		return ppro->OnMenuHandleBookmarks(w, l);
	return 0;
}

static int g_ToolbarHandleServiceDiscovery(WPARAM w, LPARAM l)
{
	if (CJabberProto *ppro = JabberChooseInstance())
		return ppro->OnMenuHandleServiceDiscovery(w, l);
	return 0;
}

int g_OnModernToolbarInit(WPARAM, LPARAM)
{
	TBButton button = {0};
	button.cbSize = sizeof(button);
	button.defPos = 1000;
	button.tbbFlags = TBBF_SHOWTOOLTIP|TBBF_VISIBLE;

	CreateServiceFunction("JABBER/*/Groupchat", g_ToolbarHandleJoinGroupchat);
	button.pszButtonID = button.pszServiceName = "JABBER/*/Groupchat";
	button.pszTooltipUp = button.pszTooltipUp = button.pszButtonName = "Join conference";
	button.hSecondaryIconHandle = button.hPrimaryIconHandle = (HANDLE)g_GetIconHandle(IDI_GROUP);
	JCallService(MS_TB_ADDBUTTON, 0, (LPARAM)&button);

	CreateServiceFunction("JABBER/*/Bookmarks", g_ToolbarHandleBookmarks);
	button.pszButtonID = button.pszServiceName = "JABBER/*/Bookmarks";
	button.pszTooltipUp = button.pszTooltipUp = button.pszButtonName = "Open bookmarks";
	button.hSecondaryIconHandle = button.hPrimaryIconHandle = (HANDLE)g_GetIconHandle(IDI_BOOKMARKS);
	button.defPos++;
	JCallService(MS_TB_ADDBUTTON, 0, (LPARAM)&button);

	CreateServiceFunction("JABBER/*/ServiceDiscovery", g_ToolbarHandleServiceDiscovery);
	button.pszButtonID = button.pszServiceName = "JABBER/*/ServiceDiscovery";
	button.pszTooltipUp = button.pszTooltipUp = button.pszButtonName = "Service discovery";
	button.hSecondaryIconHandle = button.hPrimaryIconHandle = (HANDLE)g_GetIconHandle(IDI_SERVICE_DISCOVERY);
	button.defPos++;
	JCallService(MS_TB_ADDBUTTON, 0, (LPARAM)&button);
	return 0;
}

void CJabberProto::MenuUninit()
{
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )m_hMenuRequestAuth, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )m_hMenuGrantAuth, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )m_hMenuRevokeAuth, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )m_hMenuJoinLeave, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )m_hMenuConvert, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )m_hMenuRosterAdd, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )m_hMenuLogin, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )m_hMenuRefresh, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )m_hMenuAgent, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )m_hMenuChangePassword, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )m_hMenuGroupchat, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )m_hMenuBookmarks, 0 );
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )m_hMenuAddBookmark, 0 );

	JCallService( MS_CLIST_REMOVEMAINMENUITEM, ( WPARAM )m_hMenuRoot, 0 );

	if ( m_phMenuResourceItems ) {
		for ( int i=0; i < m_nMenuResourceItems; i++ )
			JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )m_phMenuResourceItems[i], 0 );
		mir_free(m_phMenuResourceItems);
		m_phMenuResourceItems = NULL;
	}
	m_nMenuResourceItems = 0;
}

void CJabberProto::EnableMenuItems( BOOL bEnable )
{
	CLISTMENUITEM clmi = { 0 };
	clmi.cbSize = sizeof( CLISTMENUITEM );
	clmi.flags = CMIM_FLAGS;
	if ( !bEnable )
		clmi.flags |= CMIF_GRAYED;

	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuAgent, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuChangePassword, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuGroupchat, ( LPARAM )&clmi );

	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuPrivacyLists, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuRosterControl, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuServiceDiscovery, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuSDMyTransports, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuSDTransports, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuSDConferences, ( LPARAM )&clmi );

	if ( m_ThreadInfo && !( m_ThreadInfo->jabberServerCaps & JABBER_CAPS_PRIVATE_STORAGE))
		clmi.flags |= CMIF_GRAYED;
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuBookmarks, ( LPARAM )&clmi );

	JabberUpdateDialogs( bEnable );
}

//////////////////////////////////////////////////////////////////////////
// resource menu

static HANDLE hDialogsList = NULL;

void CJabberProto::MenuHideSrmmIcon(HANDLE hContact)
{
	StatusIconData sid = {0};
	sid.cbSize = sizeof(sid);
	sid.szModule = m_szModuleName;
	sid.flags = MBF_HIDDEN;
	CallService(MS_MSG_MODIFYICON, (WPARAM)hContact, (LPARAM)&sid);
}

void CJabberProto::MenuUpdateSrmmIcon(JABBER_LIST_ITEM *item)
{
	if ( item->list != LIST_ROSTER || !ServiceExists( MS_MSG_MODIFYICON ))
		return;

	HANDLE hContact = HContactFromJID(item->jid);
	if ( !hContact )
		return;

	StatusIconData sid = {0};
	sid.cbSize = sizeof(sid);
	sid.szModule = m_szModuleName;
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
			GetClientJID( dbv.ptszVal, jid, SIZEOF( jid ));
			JFreeVariant( &dbv );

			JABBER_RESOURCE_STATUS *r = ResourceInfoFromJID( jid );

			if ( r && r->bMessageSessionActive ) {
				r->bMessageSessionActive = FALSE;
				JabberCapsBits jcb = GetResourceCapabilites( jid, TRUE );

				if ( jcb & JABBER_CAPS_CHATSTATES ) {
					int iqId = SerialNext();
					XmlNode msg( _T("message"));
					xmlAddAttr( msg, "to", jid );
					xmlAddAttr( msg, "type", "chat" );
					msg.addAttrID( iqId );
					HXML goneNode = xmlAddChild( msg, "gone" );
					xmlAddAttr( goneNode, "xmlns", JABBER_FEAT_CHATSTATES );

					m_ThreadInfo->send( msg );
	}	}	}	}

	return 0;
}

int CJabberProto::OnProcessSrmmIconClick( WPARAM wParam, LPARAM lParam )
{
	StatusIconClickData *sicd = (StatusIconClickData *)lParam;
	if (lstrcmpA(sicd->szModule, m_szModuleName))
		return 0;

	HANDLE hContact = (HANDLE)wParam;
	if (!hContact)
		return 0;

	DBVARIANT dbv;
	if (JGetStringT(hContact, "jid", &dbv))
		return 0;

	JABBER_LIST_ITEM *LI = ListGetItemPtr(LIST_ROSTER, dbv.ptszVal);
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

	UpdateMirVer(LI);
	MenuUpdateSrmmIcon(LI);

	return 0;
}

int __cdecl CJabberProto::OnMenuHandleResource(WPARAM wParam, LPARAM lParam, LPARAM res)
{
	if ( !m_bJabberOnline || !wParam )
		return 0;

	HANDLE hContact = (HANDLE)wParam;

	DBVARIANT dbv;
	if (JGetStringT(hContact, "jid", &dbv))
		return 0;

	JABBER_LIST_ITEM *LI = ListGetItemPtr(LIST_ROSTER, dbv.ptszVal);
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

	UpdateMirVer(LI);
	MenuUpdateSrmmIcon(LI);
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// priority menu
int CJabberProto::OnMenuSetPriority(WPARAM wParam, LPARAM lParam, LPARAM dwDelta)
{
	int iDelta = (int)dwDelta;
	short priority = 0;
	priority = (short)JGetWord(NULL, "Priority", 0) + iDelta;
	if (priority > 127) priority = 127;
	else if (priority < -128) priority = -128;
	JSetWord(NULL, "Priority", priority);
	SendPresence(m_iStatus, true);
	return 0;
}

void CJabberProto::BuildPriorityMenu(WPARAM, LPARAM)
{
	m_hMenuPriorityRoot = NULL;
	m_priorityMenuVal = 0;
	m_priorityMenuValSet = false;

	if ( !m_bJabberOnline )
		return;

	HANDLE hMenuPriorityRoot = NULL;

	CLISTMENUITEM mi = { 0 };
	char srvFce[MAX_PATH + 64], *svcName = srvFce+strlen( m_szModuleName );
	char szItem[MAX_PATH + 64];
	char szName[128];
	HANDLE hRoot = ( HANDLE )szItem;

	mir_snprintf( szItem, SIZEOF(szItem), LPGEN("Resource priority") );

	mi.cbSize = sizeof(mi);
	mi.popupPosition= 500084000;
	mi.pszContactOwner = m_szModuleName;
	mi.pszService = srvFce;
	mi.pszPopupName = (char *)hRoot;
	mi.pszName = szName;
	mi.position = 2000040000;
	mi.flags = CMIF_ICONFROMICOLIB;

	mir_snprintf(srvFce, sizeof(srvFce), "%s/menuSetPriority/0", m_szModuleName);
	bool needServices = !ServiceExists(srvFce);
	if (needServices) JCreateServiceParam(svcName, &CJabberProto::OnMenuSetPriority, (LPARAM)0);

	int steps[] = { 10, 5, 1, 0, -1, -5, -10 };
	for (int i = 0; i < SIZEOF(steps); ++i)
	{
		if (!steps[i])
		{
			mi.position += 100000;
			continue;
		}

		mi.icolibItem = (steps[i] > 0) ? GetIconHandle(IDI_ARROW_UP) : GetIconHandle(IDI_ARROW_DOWN);

		mir_snprintf(srvFce, sizeof(srvFce), "%s/menuSetPriority/%d", m_szModuleName, steps[i]);
		mir_snprintf(szName, sizeof(szName), (steps[i] > 0) ? "Increase priority by %d" : "Decrease priority by %d", abs(steps[i]));

		if (needServices) JCreateServiceParam(svcName, &CJabberProto::OnMenuSetPriority, (LPARAM)steps[i]);

		mi.position++;
		CallService( MS_CLIST_ADDSTATUSMENUITEM, ( WPARAM )&hMenuPriorityRoot, ( LPARAM )&mi );
	}

	m_hMenuPriorityRoot = hMenuPriorityRoot;
	UpdatePriorityMenu((short)JGetWord(NULL, "Priority", 0));
}

void CJabberProto::UpdatePriorityMenu(short priority)
{
	if (!m_hMenuPriorityRoot || m_priorityMenuValSet && (priority == m_priorityMenuVal))
		return;

	TCHAR szName[128];
	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof(mi);
	mi.flags = CMIF_TCHAR|CMIF_ICONFROMICOLIB|CMIM_ICON|CMIM_NAME;
	mi.icolibItem = GetIconHandle(IDI_AGENTS);
	mi.ptszName = szName;
	mir_sntprintf(szName, SIZEOF(szName), TranslateT("Resource priority [%d]"), (int)priority);
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)m_hMenuPriorityRoot, (LPARAM)&mi);

	m_priorityMenuVal = priority;
	m_priorityMenuValSet = true;
}

//////////////////////////////////////////////////////////////////////////
// custom dynamic menus
struct TMenuItemData
{
	bool bIcolib;
	HANDLE hIcon;
	LPARAM lParam;
};

static int g_nextMenuItemId = 0;
static HWND g_hwndMenuHost = NULL;

static HBITMAP sttCreateVistaMenuBitmap(HANDLE hIcon, bool bIcolib);
static LRESULT CALLBACK sttJabberMenuHostWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

HMENU JMenuCreate(bool bRoot)
{
	if (bRoot) g_nextMenuItemId = 1;
	return CreatePopupMenu();
}

void JMenuAddItem(HMENU hMenu, LPARAM lParam, TCHAR *szText, HANDLE hIcon, bool bIcolib, bool bChecked)
{
	TMenuItemData *dat = new TMenuItemData;
	dat->hIcon = hIcon;
	dat->bIcolib = bIcolib;
	dat->lParam = lParam;

	int idx = GetMenuItemCount(hMenu)+1;

	MENUITEMINFO mii = {0};
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_BITMAP|MIIM_FTYPE|MIIM_ID|MIIM_STATE|MIIM_STRING|MIIM_DATA;
	mii.dwItemData = (ULONG_PTR)dat;
	mii.fState = bChecked ? MFS_CHECKED : 0;
	mii.fType = MFT_STRING;
	mii.wID = g_nextMenuItemId++;
	mii.hbmpItem = IsWinVerVistaPlus() ? sttCreateVistaMenuBitmap(hIcon, bIcolib) : HBMMENU_CALLBACK;
	mii.dwTypeData = szText;

	if (!IsWinVerVistaPlus())
	{
		// this breaks vista syle for menu :(
		if ((idx % 35) == 34)
		{
			mii.fMask |= MIIM_FTYPE;
			mii.fType |= MFT_MENUBARBREAK;
		}
	}

	InsertMenuItem(hMenu, idx, TRUE, &mii);
}

void JMenuAddPopup(HMENU hMenu, HMENU hPopup, TCHAR *szText, HANDLE hIcon, bool bIcolib)
{
	TMenuItemData *dat = new TMenuItemData;
	dat->hIcon = hIcon;
	dat->bIcolib = bIcolib;
	dat->lParam = 0;

	int idx = GetMenuItemCount(hMenu)+1;

	MENUITEMINFO mii = {0};
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_BITMAP|MIIM_FTYPE|MIIM_ID|MIIM_STATE|MIIM_STRING|MIIM_DATA|MIIM_SUBMENU;
	mii.dwItemData = (ULONG_PTR)dat;
	mii.hSubMenu = hPopup;
	mii.fType = MFT_STRING;
	mii.wID = g_nextMenuItemId++;
	mii.hbmpItem = IsWinVerVistaPlus() ? sttCreateVistaMenuBitmap(hIcon, bIcolib) : HBMMENU_CALLBACK;
	mii.dwTypeData = szText;

	if (!IsWinVerVistaPlus())
	{
		// this breaks vista syle for menu :(
		if ((idx % 35) == 34)
		{
			mii.fMask |= MIIM_FTYPE;
			mii.fType |= MFT_MENUBARBREAK;
		}
	}

	InsertMenuItem(hMenu, idx, TRUE, &mii);
}

void JMenuAddSeparator(HMENU hMenu)
{
	int idx = GetMenuItemCount(hMenu)+1;

	MENUITEMINFO mii = {0};
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_FTYPE;
	mii.fType = MFT_SEPARATOR;

	if (!IsWinVerVistaPlus())
	{
		// this breaks vista syle for menu :(
		if ((idx % 35) == 34)
		{
			mii.fMask |= MIIM_FTYPE;
			mii.fType |= MFT_MENUBARBREAK;
		}
	}

	InsertMenuItem(hMenu, idx, TRUE, &mii);
}

int JMenuShow(HMENU hMenu)
{
	POINT pt; GetCursorPos(&pt);
	return JMenuShow(hMenu, pt.x, pt.y);
}

static HMENU sttGetMenuItemInfoRecursive(HMENU hMenu, int id, MENUITEMINFO *mii)
{
	if (GetMenuItemInfo(hMenu, id, FALSE, mii))
		return hMenu;

	int count = GetMenuItemCount(hMenu);
	for (int i = 0; i < count; ++i)
	{
		MENUITEMINFO mii2 = {0};
		mii2.cbSize = sizeof(mii);
		mii2.fMask = MIIM_SUBMENU;
		GetMenuItemInfo(hMenu, i, TRUE, &mii2);

		if (mii2.hSubMenu)
			if (HMENU hMenuRes = sttGetMenuItemInfoRecursive(mii2.hSubMenu, id, mii))
				return hMenuRes;
	}

	return NULL;
}

int JMenuShow(HMENU hMenu, int x, int y)
{
	if (!g_hwndMenuHost)
	{
		WNDCLASSEX wcl = {0};
		wcl.cbSize = sizeof(wcl);
		wcl.lpfnWndProc = sttJabberMenuHostWndProc;
		wcl.style = 0;
		wcl.cbClsExtra = 0;
		wcl.cbWndExtra = 0;
		wcl.hInstance = hInst;
		wcl.hIcon = NULL;
		wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcl.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
		wcl.lpszMenuName = NULL;
		wcl.lpszClassName = _T("JabberDynMenuHostClass");
		wcl.hIconSm = NULL;
		RegisterClassEx(&wcl);

		g_hwndMenuHost = CreateWindow(_T("JabberDynMenuHostClass"), NULL, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_DESKTOP, NULL, hInst, NULL);
		SetWindowPos(g_hwndMenuHost, 0, 0, 0, 0, 0, SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_DEFERERASE|SWP_NOSENDCHANGING|SWP_HIDEWINDOW);
	}

	int res = TrackPopupMenu(hMenu, TPM_RETURNCMD, x, y, 0, g_hwndMenuHost, NULL);
	if (!res) return 0;

	MENUITEMINFO mii = {0};
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_DATA;
	sttGetMenuItemInfoRecursive(hMenu, res, &mii);
	//GetMenuItemInfo(hMenu, res, FALSE, &mii);

	if (mii.dwItemData)
	{
		TMenuItemData *dat = (TMenuItemData *)mii.dwItemData;
		return dat->lParam;
	}

	return 0;
}

void JMenuDestroy(HMENU hMenu, CJabberProto *ppro, void (CJabberProto::*pfnDestructor)(LPARAM lParam))
{
	int count = GetMenuItemCount(hMenu);
	for (int i = 0; i < count; ++i)
	{
		MENUITEMINFO mii = {0};
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_DATA|MIIM_SUBMENU|MIIM_BITMAP;
		GetMenuItemInfo(hMenu, i, TRUE, &mii);

		if (mii.dwItemData)
		{
			TMenuItemData *dat = (TMenuItemData *)mii.dwItemData;
			if (ppro && pfnDestructor)
				(ppro->*pfnDestructor)(dat->lParam);
			delete dat;
		}

		if (mii.hbmpItem && (mii.hbmpItem != HBMMENU_CALLBACK))
			DeleteObject(mii.hbmpItem);

		if (mii.hSubMenu)
			JMenuDestroy(mii.hSubMenu, ppro, pfnDestructor);
	}

	DestroyMenu(hMenu);
}

static LRESULT CALLBACK sttJabberMenuHostWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_MEASUREITEM:
		{
			LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
			TMenuItemData *dat = (TMenuItemData *)lpmis->itemData;
			if (lpmis->CtlType != ODT_MENU) return FALSE;
			if (!dat) return FALSE;

			lpmis->itemWidth = max(0, GetSystemMetrics(SM_CXSMICON) - GetSystemMetrics(SM_CXMENUCHECK) + 4);
			lpmis->itemHeight = GetSystemMetrics(SM_CXSMICON) + 2;

			return TRUE;
		}

		case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
			TMenuItemData *dat = (TMenuItemData *)lpdis->itemData;
			if (lpdis->CtlType != ODT_MENU) return FALSE;
			if (!dat) return FALSE;

			HICON hIcon = dat->bIcolib ? (HICON)CallService(MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)dat->hIcon) : (HICON)dat->hIcon;

			if (lpdis->itemState & ODS_CHECKED)
			{
				RECT rc;
				rc.left = rc.right = 2 /*lpdis->rcItem.left - GetSystemMetrics(SM_CXMENUCHECK)*/ - 1;
				rc.top = rc.bottom = (lpdis->rcItem.top + lpdis->rcItem.bottom - GetSystemMetrics(SM_CYSMICON))/2 - 1;
				rc.right += 18; rc.bottom += 18;
				FillRect(lpdis->hDC, &rc, GetSysColorBrush(COLOR_MENUHILIGHT));
				rc.left++; rc.top++;
				rc.right--; rc.bottom--;

				COLORREF cl1 = GetSysColor(COLOR_MENUHILIGHT);
				COLORREF cl2 = GetSysColor(COLOR_MENU);
				HBRUSH hbr = CreateSolidBrush(RGB((GetRValue(cl1) + GetRValue(cl2)) / 2, (GetGValue(cl1) + GetGValue(cl2)) / 2, (GetBValue(cl1) + GetBValue(cl2)) / 2));
				FillRect(lpdis->hDC, &rc, hbr);
				DeleteObject(hbr);
			}

			DrawIconEx(lpdis->hDC,
				2/*lpdis->rcItem.left - GetSystemMetrics(SM_CXMENUCHECK)*/,
				(lpdis->rcItem.top + lpdis->rcItem.bottom - GetSystemMetrics(SM_CYSMICON))/2,
				hIcon, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
				0, NULL, DI_NORMAL);

			if (dat->bIcolib) CallService(MS_SKIN2_RELEASEICON, (WPARAM)hIcon, 0);

			return TRUE;
		}
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

static HBITMAP sttCreateVistaMenuBitmap(HANDLE hIcon, bool bIcolib)
{
	int i, j;

	BITMAPINFO bi;
	HDC dcBmp;
	HBITMAP hBmp, hBmpSave;
	BYTE *bits = NULL;

	HICON hic = bIcolib ? (HICON)CallService(MS_SKIN2_GETICONBYHANDLE, 0, (LPARAM)hIcon) : (HICON)hIcon;
	ICONINFO info;
	BITMAP bmpColor, bmpMask;
	BYTE *cbit, *mbit;

	int width = GetSystemMetrics(SM_CXSMICON);
	int height = GetSystemMetrics(SM_CYSMICON);

	if (!hic) return NULL;

    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    hBmp = (HBITMAP)CreateDIBSection(0, &bi, DIB_RGB_COLORS, (void **)&bits, 0, 0);
    dcBmp = CreateCompatibleDC(0);
    hBmpSave = (HBITMAP)SelectObject(dcBmp, hBmp);

	GdiFlush();

	GetIconInfo(hic, &info);
	GetObject(info.hbmMask, sizeof(bmpMask), &bmpMask);
	GetObject(info.hbmColor, sizeof(bmpColor), &bmpColor);

	cbit = (BYTE *)mir_alloc(bmpColor.bmWidthBytes*bmpColor.bmHeight);
	mbit = (BYTE *)mir_alloc(bmpMask.bmWidthBytes*bmpMask.bmHeight);
	GetBitmapBits(info.hbmColor, bmpColor.bmWidthBytes*bmpColor.bmHeight, cbit);
	GetBitmapBits(info.hbmMask, bmpMask.bmWidthBytes*bmpMask.bmHeight, mbit);

	GdiFlush();

	for (i = 0; i < width; i++)
	{
		for (j = 0; j < height; j++)
		{
			BYTE *pixel = cbit + i*bmpColor.bmWidthBytes + j*4;
			BYTE *pixel_bmp = bits + (i*width + j)*4;
			if (!pixel[3])
			{
				pixel[3] = (*(mbit + i*bmpMask.bmWidthBytes + j*bmpMask.bmBitsPixel/8) & (1<<(7-j%8))) ? 0 : 255;
			}

			pixel_bmp[3] = pixel[3];
			if (pixel[3] != 255)
			{
				pixel_bmp[0] = (BYTE)((long)(pixel[0]) * pixel[3]) / 255;
				pixel_bmp[1] = (BYTE)((long)(pixel[1]) * pixel[3]) / 255;
				pixel_bmp[2] = (BYTE)((long)(pixel[2]) * pixel[3]) / 255;
			} else
			{
				pixel_bmp[0] = pixel[0];
				pixel_bmp[1] = pixel[1];
				pixel_bmp[2] = pixel[2];
			}
		}
	}

	GdiFlush();

	mir_free(mbit);
	mir_free(cbit);

	SelectObject(dcBmp, hBmpSave);
	DeleteDC(dcBmp);

	DeleteObject(info.hbmColor);
	DeleteObject(info.hbmMask);
	if (bIcolib) CallService(MS_SKIN2_RELEASEICON, (WPARAM)hic, 0);

	return hBmp;
}
