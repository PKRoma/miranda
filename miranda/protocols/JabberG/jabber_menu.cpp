/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-09  George Hazan
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

static int hChooserMenu;
static int iChooserMenuPos = 30000;

static int JabberMenuChooseService( WPARAM wParam, LPARAM lParam )
{
	if ( lParam )
		*( void** )lParam = ( void* )wParam;
	return 0;
}

void g_MenuInit( void )
{
	CreateServiceFunction( "Jabber/MenuChoose", JabberMenuChooseService );

	TMenuParam mnu = {0};
	mnu.cbSize = sizeof(mnu);
	mnu.name = "JabberAccountChooser";
	mnu.ExecService = "Jabber/MenuChoose";
	hChooserMenu = JCallService( MO_CREATENEWMENUOBJECT, 0, (LPARAM)&mnu );

	TMO_MenuItem tmi = { 0 };
	tmi.cbSize = sizeof( tmi );
	tmi.flags = CMIF_ICONFROMICOLIB;
	tmi.pszName = "Cancel";
	tmi.position = 9999999;
	tmi.hIcolibItem = LoadSkinnedIconHandle(SKINICON_OTHER_DELETE);
	JCallService( MO_ADDNEWMENUITEM, hChooserMenu, ( LPARAM )&tmi );
}

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

int CJabberProto::OnPrebuildContactMenu( WPARAM wParam, LPARAM )
{
	sttEnableMenuItem( m_hMenuRequestAuth, FALSE );
	sttEnableMenuItem( m_hMenuGrantAuth, FALSE );
	sttEnableMenuItem( m_hMenuRevokeAuth, FALSE );
	sttEnableMenuItem( m_hMenuJoinLeave, FALSE );
	sttEnableMenuItem( m_hMenuCommands, FALSE );
	sttEnableMenuItem( m_hMenuSendNote, FALSE );
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
		clmi.flags = CMIM_NAME | CMIM_FLAGS;
		if ( JGetWord( hContact, "Status", 0 ) == ID_STATUS_ONLINE )
			clmi.pszName = (char *)LPGEN("&Leave");
		else {
			clmi.pszName = (char *)LPGEN("&Join");
			clmi.flags |= CMIF_DEFAULT;
		}

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
			sttEnableMenuItem( m_hMenuSendNote, TRUE );

			if ( item->resourceCount >= 1 ) {
				sttEnableMenuItem( m_hMenuResourcesRoot, TRUE );

				int nMenuResourceItemsNew = m_nMenuResourceItems;
				if ( m_nMenuResourceItems < item->resourceCount ) {
					m_phMenuResourceItems = (HANDLE *)mir_realloc( m_phMenuResourceItems, item->resourceCount * sizeof(HANDLE) );
					nMenuResourceItemsNew = item->resourceCount;
				}

				char text[ 256 ];
				strcpy( text, m_szModuleName );
				int nModuleNameLength = strlen( text );
				char* tDest = text + nModuleNameLength;

				CLISTMENUITEM mi = { 0 };
				mi.cbSize = sizeof( CLISTMENUITEM );
				mi.flags = CMIF_CHILDPOPUP;
				mi.position = 0;
				mi.icolibItem = GetIconHandle( IDI_REQUEST );
				mi.pszService = text;
				mi.pszContactOwner = m_szModuleName;

				TCHAR szTmp[512];
				for (int i = 0; i < nMenuResourceItemsNew; ++i) {
					mir_snprintf( tDest, SIZEOF(text) - nModuleNameLength, "/UseResource_%d", i );
					if ( i >= m_nMenuResourceItems ) {
						JCreateServiceParam( tDest, &CJabberProto::OnMenuHandleResource, MENUITEM_RESOURCES+i );
						JCreateService( tDest, &CJabberProto::OnMenuHandleRequestAuth );
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

int __cdecl CJabberProto::OnMenuConvertChatContact( WPARAM wParam, LPARAM )
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

int __cdecl CJabberProto::OnMenuRosterAdd( WPARAM wParam, LPARAM )
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
			if ( m_options.AddRoster2Bookmarks == TRUE ) {

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

int __cdecl CJabberProto::OnMenuHandleRequestAuth( WPARAM wParam, LPARAM )
{
	HANDLE hContact;
	DBVARIANT dbv;

	if (( hContact=( HANDLE ) wParam )!=NULL && m_bJabberOnline ) {
		if ( !JGetStringT( hContact, "jid", &dbv )) {
			m_ThreadInfo->send( XmlNode( _T("presence")) << XATTR( _T("to"), dbv.ptszVal ) << XATTR( _T("type"), _T("subscribe")));
			JFreeVariant( &dbv );
	}	}

	return 0;
}

int __cdecl CJabberProto::OnMenuHandleGrantAuth( WPARAM wParam, LPARAM )
{
	HANDLE hContact;
	DBVARIANT dbv;

	if (( hContact=( HANDLE ) wParam )!=NULL && m_bJabberOnline ) {
		if ( !JGetStringT( hContact, "jid", &dbv )) {
			m_ThreadInfo->send( XmlNode( _T("presence")) << XATTR( _T("to"), dbv.ptszVal ) << XATTR( _T("type"), _T("subscribed")));
			JFreeVariant( &dbv );
	}	}

	return 0;
}

int __cdecl CJabberProto::OnMenuRevokeAuth( WPARAM wParam, LPARAM )
{
	HANDLE hContact;
	DBVARIANT dbv;

	if (( hContact=( HANDLE ) wParam ) != NULL && m_bJabberOnline ) {
		if ( !JGetStringT( hContact, "jid", &dbv )) {
			m_ThreadInfo->send( XmlNode( _T("presence")) << XATTR( _T("to"), dbv.ptszVal ) << XATTR( _T("type"), _T("unsubscribed")));
			JFreeVariant( &dbv );
	}	}

	return 0;
}

int __cdecl CJabberProto::OnMenuTransportLogin( WPARAM wParam, LPARAM )
{
	HANDLE hContact = ( HANDLE )wParam;
	if ( !JGetByte( hContact, "IsTransport", 0 ))
		return 0;

	DBVARIANT jid;
	if ( JGetStringT( hContact, "jid", &jid ))
		return 0;

	JABBER_LIST_ITEM* item = ListGetItemPtr( LIST_ROSTER, jid.ptszVal );
	if ( item != NULL ) {
		XmlNode p( _T("presence")); xmlAddAttr( p, _T("to"), item->jid );
		if ( item->itemResource.status == ID_STATUS_ONLINE )
			xmlAddAttr( p, _T("type"), _T("unavailable"));
		m_ThreadInfo->send( p );
	}

	JFreeVariant( &jid );
	return 0;
}

int __cdecl CJabberProto::OnMenuTransportResolve( WPARAM wParam, LPARAM )
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

int __cdecl CJabberProto::OnMenuBookmarkAdd( WPARAM wParam, LPARAM )
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
// status menu

int __cdecl CJabberProto::OnBuildStatusMenu( WPARAM, LPARAM )
{
	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof( CLISTMENUITEM );
	mi.flags = CMIM_FLAGS | CMIF_GRAYED;

	char text[ 200 ];
	strcpy( text, m_szModuleName );
	char* tDest = text + strlen( text );
	mi.pszService = text;

	mi.hParentMenu = pcli->pfnGetProtocolMenu( m_szModuleName );
	mi.flags = CMIF_ICONFROMICOLIB | CMIF_ROOTHANDLE;
	mi.position = 1001;

	m_hMenuRoot = (HGENMENU)CallService( MS_CLIST_ADDSTATUSMENUITEM,  (WPARAM)0, (LPARAM)&mi);

	// "Bookmarks..."
	JCreateService( "/Bookmarks", &CJabberProto::OnMenuHandleBookmarks );
	strcpy( tDest, "/Bookmarks" );
	mi.pszName = LPGEN("Bookmarks");
	mi.popupPosition = 200001;
	mi.icolibItem = GetIconHandle( IDI_BOOKMARKS );
	m_hMenuBookmarks = ( HGENMENU ) JCallService( MS_CLIST_ADDSTATUSMENUITEM, 0, ( LPARAM )&mi );

	// "Options..."
	JCreateService( "/Options", &CJabberProto::OnMenuOptions );
	strcpy( tDest, "/Options" );
	mi.pszName = LPGEN("Options...");
	mi.popupPosition = 200002;
	mi.icolibItem = GetIconHandle( IDI_BOOKMARKS );
	JCallService( MS_CLIST_ADDSTATUSMENUITEM, 0, ( LPARAM )&mi );

	// "Services..."
	mi.pszName = LPGEN("Services...");
	strcpy( tDest, "/Services" );
	mi.popupPosition = 200003;
	mi.icolibItem = GetIconHandle( IDI_SERVICE_DISCOVERY );
	m_hMenuRoot = ( HGENMENU ) JCallService( MS_CLIST_ADDSTATUSMENUITEM, 0, ( LPARAM )&mi );

	// "Service Discovery..."
	JCreateService( "/ServiceDiscovery", &CJabberProto::OnMenuHandleServiceDiscovery );
	strcpy( tDest, "/ServiceDiscovery" );
	mi.flags = CMIF_ICONFROMICOLIB | CMIF_ROOTHANDLE;
	mi.pszName = LPGEN("Service Discovery");
	mi.position = 2000050001;
	mi.icolibItem = GetIconHandle( IDI_SERVICE_DISCOVERY );
	mi.hParentMenu = m_hMenuRoot;
	m_hMenuServiceDiscovery = ( HGENMENU ) JCallService( MS_CLIST_ADDSTATUSMENUITEM, 0, ( LPARAM )&mi );

	JCreateService( "/SD/MyTransports", &CJabberProto::OnMenuHandleServiceDiscoveryMyTransports );
	strcpy( tDest, "/SD/MyTransports" );
	mi.pszName = LPGEN("Registered Transports");
	mi.position = 2000050003;
	mi.icolibItem = GetIconHandle( IDI_TRANSPORTL );
	m_hMenuSDMyTransports = ( HGENMENU ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );

	JCreateService( "/SD/Transports", &CJabberProto::OnMenuHandleServiceDiscoveryTransports );
	strcpy( tDest, "/SD/Transports" );
	mi.pszName = LPGEN("Local Server Transports");
	mi.position = 2000050004;
	mi.icolibItem = GetIconHandle( IDI_TRANSPORT );
	m_hMenuSDTransports = ( HGENMENU ) JCallService( MS_CLIST_ADDSTATUSMENUITEM, 0, ( LPARAM )&mi );

	JCreateService( "/SD/Conferences", &CJabberProto::OnMenuHandleServiceDiscoveryConferences );
	strcpy( tDest, "/SD/Conferences" );
	mi.pszName = LPGEN("Browse Chatrooms");
	mi.position = 2000050005;
	mi.icolibItem = GetIconHandle( IDI_GROUP );
	m_hMenuSDConferences = ( HGENMENU ) JCallService( MS_CLIST_ADDSTATUSMENUITEM, 0, ( LPARAM )&mi );

	JCreateService( "/Groupchat", &CJabberProto::OnMenuHandleJoinGroupchat );
	strcpy( tDest, "/Groupchat" );
	mi.pszName = LPGEN("Create/Join groupchat");
	mi.position = 2000050006;
	mi.icolibItem = GetIconHandle( IDI_GROUP );
	m_hMenuGroupchat = ( HGENMENU ) JCallService( MS_CLIST_ADDSTATUSMENUITEM, 0, ( LPARAM )&mi );

	// "Change Password..."
	JCreateService( "/ChangePassword", &CJabberProto::OnMenuHandleChangePassword );
	strcpy( tDest, "/ChangePassword" );
	mi.pszName = LPGEN("Change Password");
	mi.position = 2000050007;
	mi.icolibItem = GetIconHandle( IDI_KEYS );
	m_hMenuChangePassword = ( HGENMENU ) JCallService( MS_CLIST_ADDSTATUSMENUITEM, 0, ( LPARAM )&mi );

	// "Roster editor"
	JCreateService( "/RosterEditor", &CJabberProto::OnMenuHandleRosterControl );
	strcpy( tDest, "/RosterEditor" );
	mi.pszName = LPGEN("Roster editor");
	mi.position = 2000050009;
	mi.icolibItem = GetIconHandle( IDI_AGENTS );
	m_hMenuRosterControl = ( HGENMENU ) JCallService( MS_CLIST_ADDSTATUSMENUITEM, 0, ( LPARAM )&mi );

	// "XML Console"
	JCreateService( "/XMLConsole", &CJabberProto::OnMenuHandleConsole );
	strcpy( tDest, "/XMLConsole" );
	mi.pszName = LPGEN("XML Console");
	mi.position = 2000050010;
	mi.icolibItem = GetIconHandle( IDI_CONSOLE );
	JCallService( MS_CLIST_ADDSTATUSMENUITEM, 0, ( LPARAM )&mi );

	JCreateService( "/Notes", &CJabberProto::OnMenuHandleNotes );
	strcpy( tDest, "/Notes" );
	mi.pszName = LPGEN("Notes");
	mi.position = 2000050011;
	mi.icolibItem = GetIconHandle( IDI_NOTES);
	m_hMenuNotes = ( HGENMENU ) JCallService( MS_CLIST_ADDSTATUSMENUITEM, 0, ( LPARAM )&mi );

	BuildPrivacyMenu();
	if ( m_menuItemsStatus )
		BuildPrivacyListsMenu( false );
	BuildPriorityMenu();
	m_pepServices.RebuildMenu();
	CheckMenuItems();
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// priority popup in status menu

int CJabberProto::OnMenuSetPriority(WPARAM, LPARAM, LPARAM dwDelta)
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

void CJabberProto::BuildPriorityMenu()
{
	m_priorityMenuVal = 0;
	m_priorityMenuValSet = false;

	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof(mi);
	mi.position = 1006;
	mi.pszContactOwner = m_szModuleName;
	mi.hParentMenu = pcli->pfnGetProtocolMenu( m_szModuleName );
	mi.pszName = LPGEN("Resource priority");
	mi.flags = CMIF_ROOTPOPUP | CMIF_HIDDEN;
	m_hMenuPriorityRoot = ( HGENMENU )CallService( MS_CLIST_ADDSTATUSMENUITEM, 0, ( LPARAM )&mi );

	char szName[128], srvFce[MAX_PATH + 64], *svcName = srvFce+strlen( m_szModuleName );
	mi.pszService = srvFce;
	mi.pszName = szName;
	mi.position = 2000040000;
	mi.flags = CMIF_CHILDPOPUP | CMIF_ICONFROMICOLIB;
	mi.hParentMenu = m_hMenuPriorityRoot;

	mir_snprintf(srvFce, sizeof(srvFce), "%s/menuSetPriority/0", m_szModuleName);
	bool needServices = !ServiceExists(srvFce);
	if ( needServices )
		JCreateServiceParam(svcName, &CJabberProto::OnMenuSetPriority, (LPARAM)0);

	int steps[] = { 10, 5, 1, 0, -1, -5, -10 };
	for (int i = 0; i < SIZEOF(steps); ++i) {
		if ( !steps[i] ) {
			mi.position += 100000;
			continue;
		}

		mi.icolibItem = (steps[i] > 0) ? GetIconHandle(IDI_ARROW_UP) : GetIconHandle(IDI_ARROW_DOWN);

		mir_snprintf(srvFce, sizeof(srvFce), "%s/menuSetPriority/%d", m_szModuleName, steps[i]);
		mir_snprintf(szName, sizeof(szName), (steps[i] > 0) ? "Increase priority by %d" : "Decrease priority by %d", abs(steps[i]));

		if ( needServices )
			JCreateServiceParam(svcName, &CJabberProto::OnMenuSetPriority, (LPARAM)steps[i]);

		mi.position++;
		CallService( MS_CLIST_ADDSTATUSMENUITEM, 0, ( LPARAM )&mi );
	}

	UpdatePriorityMenu((short)JGetWord(NULL, "Priority", 0));
}

void CJabberProto::UpdatePriorityMenu(short priority)
{
	if (!m_hMenuPriorityRoot || m_priorityMenuValSet && (priority == m_priorityMenuVal))
		return;

	TCHAR szName[128];
	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof(mi);
	mi.flags = CMIF_TCHAR | CMIM_NAME;
	mi.ptszName = szName;
	mir_sntprintf(szName, SIZEOF(szName), TranslateT("Resource priority [%d]"), (int)priority);
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)m_hMenuPriorityRoot, (LPARAM)&mi);

	m_priorityMenuVal = priority;
	m_priorityMenuValSet = true;
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
	JCreateService( "/ReqAuth", &CJabberProto::OnMenuHandleRequestAuth );
	strcpy( tDest, "/ReqAuth" );
	mi.pszName = LPGEN("Request authorization");
	mi.flags = CMIF_ICONFROMICOLIB;
	mi.position = -2000001000;
	mi.icolibItem = GetIconHandle( IDI_REQUEST );
	mi.pszService = text;
	mi.pszContactOwner = m_szModuleName;
	m_hMenuRequestAuth = ( HGENMENU ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Grant authorization"
	JCreateService( "/GrantAuth", &CJabberProto::OnMenuHandleGrantAuth );
	strcpy( tDest, "/GrantAuth" );
	mi.pszName = LPGEN("Grant authorization");
	mi.position = -2000001001;
	mi.icolibItem = GetIconHandle( IDI_GRANT );
	m_hMenuGrantAuth = ( HGENMENU ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Revoke auth
	JCreateService( "/RevokeAuth", &CJabberProto::OnMenuRevokeAuth );
	strcpy( tDest, "/RevokeAuth" );
	mi.pszName = LPGEN("Revoke authorization");
	mi.position = -2000001002;
	mi.icolibItem = GetIconHandle( IDI_AUTHREVOKE );
	m_hMenuRevokeAuth = ( HGENMENU ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Convert Chat/Contact"
	JCreateService( "/ConvertChatContact", &CJabberProto::OnMenuConvertChatContact );
	strcpy( tDest, "/ConvertChatContact" );
	mi.pszName = LPGEN("Convert");
	mi.position = -1999901004;
	mi.icolibItem = GetIconHandle( IDI_USER2ROOM );
	m_hMenuConvert = ( HGENMENU ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Add to roster"
	JCreateService( "/AddToRoster", &CJabberProto::OnMenuRosterAdd );
	strcpy( tDest, "/AddToRoster" );
	mi.pszName = LPGEN("Add to roster");
	mi.position = -1999901005;
	mi.icolibItem = GetIconHandle( IDI_ADDROSTER );
	m_hMenuRosterAdd = ( HGENMENU ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Add to Bookmarks"
	JCreateService( "/AddToBookmarks", &CJabberProto::OnMenuBookmarkAdd );
	strcpy( tDest, "/AddToBookmarks" );
	mi.pszName = LPGEN("Add to Bookmarks");
	mi.position = -1999901006;
	mi.icolibItem = GetIconHandle( IDI_BOOKMARKS);
	m_hMenuAddBookmark= ( HGENMENU ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Login/logout
	JCreateService( "/TransportLogin", &CJabberProto::OnMenuTransportLogin );
	strcpy( tDest, "/TransportLogin" );
	mi.pszName = LPGEN("Login/logout");
	mi.position = -1999901007;
	mi.icolibItem = GetIconHandle( IDI_LOGIN );
	m_hMenuLogin = ( HGENMENU ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Retrieve nicks
	JCreateService( "/TransportGetNicks", &CJabberProto::OnMenuTransportResolve );
	strcpy( tDest, "/TransportGetNicks" );
	mi.pszName = LPGEN("Resolve nicks");
	mi.position = -1999901008;
	mi.icolibItem = GetIconHandle( IDI_REFRESH );
	m_hMenuRefresh = ( HGENMENU ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Run Commands
	JCreateService( "/RunCommands", &CJabberProto::ContactMenuRunCommands );
	strcpy( tDest, "/RunCommands" );
	mi.pszName = LPGEN("Commands");
	mi.position = -1999901009;
	mi.icolibItem = GetIconHandle( IDI_COMMAND );
	m_hMenuCommands = ( HGENMENU )JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Send Note
	JCreateService( "/SendNote", &CJabberProto::OnMenuSendNote );
	strcpy( tDest, "/SendNote" );
	mi.pszName = LPGEN("Send Note");
	mi.position = -1999901010;
	mi.icolibItem = GetIconHandle( IDI_SEND_NOTE);
	m_hMenuSendNote = ( HGENMENU )JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Resource selector
	strcpy( tDest, "/ResourceSelectorDummySvc" );
	mi.pszName = LPGEN("Jabber Resource");
	mi.position = -1999901011;
	mi.pszPopupName = (char *)-1;
	mi.flags |= CMIF_ROOTPOPUP;
	mi.icolibItem = GetIconHandle( IDI_JABBER );
	m_hMenuResourcesRoot = ( HGENMENU )JCallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);

	mi.flags &= ~CMIF_ROOTPOPUP;
	mi.flags |= CMIF_CHILDPOPUP;

	JCreateServiceParam( "/UseResource_last", &CJabberProto::OnMenuHandleResource, MENUITEM_LASTSEEN );
	strcpy( tDest, "/UseResource_last" );
	mi.pszName = LPGEN("Last Active");
	mi.position = -1999901000;
	mi.pszPopupName = (char *)m_hMenuResourcesRoot;
	mi.icolibItem = GetIconHandle( IDI_JABBER );
	m_hMenuResourcesActive = ( HGENMENU )CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);

	JCreateServiceParam( "/UseResource_server", &CJabberProto::OnMenuHandleResource, MENUITEM_SERVER );
	strcpy( tDest, "/UseResource_server" );
	mi.pszName = LPGEN("Server's Choice");
	mi.position = -1999901000;
	mi.pszPopupName = (char *)m_hMenuResourcesRoot;
	mi.icolibItem = GetIconHandle( IDI_NODE_SERVER );
	m_hMenuResourcesServer = ( HGENMENU )CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);

	//////////////////////////////////////////////////////////////////////////////////////
	// Account chooser menu

	TMO_MenuItem tmi = { 0 };
	tmi.cbSize = sizeof(tmi);
	tmi.flags = CMIF_TCHAR;
	tmi.ownerdata = this;
	tmi.position = m_hChooseMenuPos = iChooserMenuPos++;
	tmi.ptszName = m_tszUserName;
	m_hChooseMenuItem = JCallService( MO_ADDNEWMENUITEM, hChooserMenu, ( LPARAM )&tmi );

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
	if ( g_Instances.getCount() == 0 )
		return 0;

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
	JCallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )m_hMenuNotes, 0 );
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
	m_menuItemsStatus = bEnable;
	CheckMenuItems();
}

void CJabberProto::CheckMenuItems()
{
	CLISTMENUITEM clmi = { 0 };
	clmi.cbSize = sizeof( CLISTMENUITEM );
	clmi.flags = CMIM_FLAGS;
	if ( !m_menuItemsStatus )
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
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuNotes, ( LPARAM )&clmi );

	clmi.flags = CMIM_FLAGS | (( m_menuItemsStatus ) ? 0 : CMIF_HIDDEN);
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hPrivacyMenuRoot, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuPriorityRoot, ( LPARAM )&clmi );

	if ( !m_bPepSupported )
		clmi.flags |= CMIF_HIDDEN;
	for ( int i=0; i < m_pepServices.getCount(); i++ )
		JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_pepServices[i].GetMenu(), ( LPARAM )&clmi );
	
	JabberUpdateDialogs( m_menuItemsStatus );
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

int CJabberProto::OnProcessSrmmEvent( WPARAM, LPARAM lParam )
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
					m_ThreadInfo->send( 
						XmlNode( _T("message")) << XATTR( _T("to"), jid ) << XATTR( _T("type"), _T("chat")) << XATTRID( iqId )
							<< XCHILDNS( _T("gone"), _T(JABBER_FEAT_CHATSTATES)));
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

int __cdecl CJabberProto::OnMenuHandleResource(WPARAM wParam, LPARAM, LPARAM res)
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

////////////////////////////////////////////////////////////////////////
// Choose protocol instance
CJabberProto *JabberChooseInstance(bool bAllowOffline, bool /*atCursor*/)
{
	if (g_Instances.getCount() == 0)
		return NULL;

	if (g_Instances.getCount() == 1) {
		if (bAllowOffline || ((g_Instances[0]->m_iStatus != ID_STATUS_OFFLINE) && (g_Instances[0]->m_iStatus != ID_STATUS_CONNECTING)))
			return g_Instances[0];
		return NULL;
	}

	CLISTMENUITEM clmi = {0};
	clmi.cbSize = sizeof( CLISTMENUITEM );

	int nItems = 0, lastItemId = 0;
	for (int i = 0; i < g_Instances.getCount(); ++i) {
		clmi.flags = CMIM_FLAGS;

		CJabberProto* ppro = g_Instances[i];
		if (bAllowOffline || (ppro->m_iStatus != ID_STATUS_OFFLINE && ppro->m_iStatus != ID_STATUS_CONNECTING))
		{
			++nItems;
			lastItemId = i+1;
			clmi.flags |= CMIM_ICON;
			clmi.hIcon = LoadSkinnedProtoIcon(ppro->m_szModuleName, ppro->m_iStatus);
		}
		else clmi.flags |= CMIF_HIDDEN;

		JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )ppro->m_hChooseMenuItem, ( LPARAM )&clmi );
	}

	if (nItems > 1) {
		ListParam param = { 0 };
		param.MenuObjectHandle = hChooserMenu;
		HMENU hMenu = CreatePopupMenu();
		JCallService( MO_BUILDMENU, ( WPARAM )hMenu, ( LPARAM )&param );

		POINT pt;
		GetCursorPos(&pt);

		int res = TrackPopupMenu( hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, pcli->hwndContactList, NULL );
		DestroyMenu( hMenu );

		if ( res ) {
			CJabberProto* pro = NULL;
			JCallService( MO_PROCESSCOMMANDBYMENUIDENT, res, ( LPARAM )&pro );
			return pro;
		}

		return NULL;
	}

	return lastItemId ? g_Instances[lastItemId-1] : NULL;
}
