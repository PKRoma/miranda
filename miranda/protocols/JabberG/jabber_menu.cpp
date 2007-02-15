/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-06  George Hazan

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

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_menu.cpp,v $
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_list.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////////////////
// module data

static HANDLE hMenuRequestAuth = NULL;
static HANDLE hMenuGrantAuth = NULL;
static HANDLE hMenuRevokeAuth = NULL;
static HANDLE hMenuJoinLeave = NULL;
static HANDLE hMenuConvert = NULL;
static HANDLE hMenuRosterAdd = NULL;
static HANDLE hMenuLogin = NULL;
static HANDLE hMenuRefresh = NULL;
static HANDLE hMenuAgent = NULL;
static HANDLE hMenuChangePassword = NULL;
static HANDLE hMenuGroupchat = NULL;

int JabberMenuHandleAgents( WPARAM wParam, LPARAM lParam );
int JabberMenuHandleChangePassword( WPARAM wParam, LPARAM lParam );
int JabberMenuHandleVcard( WPARAM wParam, LPARAM lParam );
int JabberMenuHandleRequestAuth( WPARAM wParam, LPARAM lParam );
int JabberMenuHandleGrantAuth( WPARAM wParam, LPARAM lParam );

static int sttCompareHandles( const void* p1, const void* p2 )
{	return (long)p1 - (long)p2;
}
static LIST<void> arHooks( 20, sttCompareHandles ); 

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

int JabberMenuPrebuildContactMenu( WPARAM wParam, LPARAM lParam )
{
	sttEnableMenuItem( hMenuRequestAuth, FALSE );
	sttEnableMenuItem( hMenuGrantAuth, FALSE );
	sttEnableMenuItem( hMenuRevokeAuth, FALSE );
	sttEnableMenuItem( hMenuJoinLeave, FALSE );
	sttEnableMenuItem( hMenuConvert, FALSE );
	sttEnableMenuItem( hMenuRosterAdd, FALSE );
	sttEnableMenuItem( hMenuLogin, FALSE );
	sttEnableMenuItem( hMenuRefresh, FALSE );

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
			clmi.pszName = bIsChatRoom ? "&Convert to Contact" : "&Convert to Chat Room";
			clmi.flags = CMIM_NAME | CMIM_FLAGS;
			JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuConvert, ( LPARAM )&clmi );
	}	}

	if (!jabberOnline)
		return 0;

	if ( bIsChatRoom ) {
		DBVARIANT dbv;
		if ( !JGetStringT( hContact, "ChatRoomID", &dbv )) {
			if ( JabberListGetItemPtr( LIST_ROSTER, dbv.ptszVal ) == NULL ) {
				sttEnableMenuItem( hMenuRosterAdd, TRUE );
			}
			JFreeVariant( &dbv );
	}	}

	if ( bIsChatRoom == GCW_CHATROOM ) {
		CLISTMENUITEM clmi = { 0 };
		clmi.cbSize = sizeof( clmi );
		clmi.pszName = ( JGetWord( hContact, "Status", 0 ) == ID_STATUS_ONLINE ) ? "&Leave" : "&Join";
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
		JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_ROSTER, dbv.ptszVal );
		JFreeVariant( &dbv );
		if ( item != NULL ) {
			BOOL bCtrlPressed = ( GetKeyState( VK_CONTROL)&0x8000 ) != 0;
			sttEnableMenuItem( hMenuRequestAuth, item->subscription == SUB_FROM || item->subscription == SUB_NONE || bCtrlPressed );
			sttEnableMenuItem( hMenuGrantAuth, item->subscription == SUB_TO || item->subscription == SUB_NONE || bCtrlPressed );
			sttEnableMenuItem( hMenuRevokeAuth, item->subscription == SUB_FROM || item->subscription == SUB_BOTH || bCtrlPressed );
			return 0;
	}	}

	return 0;
}

int JabberMenuConvertChatContact( WPARAM wParam, LPARAM lParam )
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

int JabberMenuRosterAdd( WPARAM wParam, LPARAM lParam )
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
			if (nick) mir_free(nick);
			if (nick) mir_free(group);
		}
		mir_free(roomID);
	}
	return 0;
}

int JabberMenuHandleRequestAuth( WPARAM wParam, LPARAM lParam )
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

int JabberMenuHandleGrantAuth( WPARAM wParam, LPARAM lParam )
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

int JabberMenuRevokeAuth( WPARAM wParam, LPARAM lParam )
{
	HANDLE hContact;
	DBVARIANT dbv;

	if (( hContact=( HANDLE ) wParam )!=NULL && jabberOnline ) {
		if ( !JGetStringT( hContact, "jid", &dbv )) {
			XmlNode presence( "presence" ); presence.addAttr( "to", dbv.ptszVal ); presence.addAttr( "type", "unsubscribed" );
			jabberThreadInfo->send( presence );
			JFreeVariant( &dbv );
	}	}

	return 0;
}

int JabberMenuJoinLeave( WPARAM wParam, LPARAM lParam )
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

int JabberMenuTransportLogin( WPARAM wParam, LPARAM lParam )
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
		if ( item->status == ID_STATUS_ONLINE )
			p.addAttr( "type", "unavailable" );
		jabberThreadInfo->send( p );
	}

	JFreeVariant( &jid );
	return 0;
}

int JabberMenuTransportResolve( WPARAM wParam, LPARAM lParam )
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

/////////////////////////////////////////////////////////////////////////////////////////
// contact menu initialization code

void JabberMenuInit()
{
	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof( CLISTMENUITEM );

	char text[ 200 ];
	strcpy( text, jabberProtoName );
	char* tDest = text + strlen( text );

	// "Request authorization"
	strcpy( tDest, "/RequestAuth" );
	CreateServiceFunction( text, JabberMenuHandleRequestAuth );
	mi.pszName = "Request authorization";
	mi.position = -2000001000;
	mi.hIcon = LoadIconEx( "Request" );
	mi.pszService = text;
	mi.pszContactOwner = jabberProtoName;
	hMenuRequestAuth = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Grant authorization"
	strcpy( tDest, "/GrantAuth" );
	CreateServiceFunction( text, JabberMenuHandleGrantAuth );
	mi.pszName = "Grant authorization";
	mi.position = -2000001001;
	mi.hIcon = LoadIconEx( "Grant" );
	hMenuGrantAuth = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Revoke auth
	strcpy( tDest, "/RevokeAuth" );
	CreateServiceFunction( text, JabberMenuRevokeAuth );
	mi.pszName = "Revoke authorization";
	mi.position = -2000001002;
	mi.hIcon = LoadIconEx( "Revoke" );
	hMenuRevokeAuth = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Grant authorization"
	strcpy( tDest, "/JoinChat" );
	CreateServiceFunction( text, JabberMenuJoinLeave );
	mi.pszName = "Join chat";
	mi.position = -2000001003;
	mi.hIcon = LoadIconEx( "group" );
	hMenuJoinLeave = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Convert Chat/Contact"
	strcpy( tDest, "/ConvertChatContact" );
	CreateServiceFunction( text, JabberMenuConvertChatContact );
	mi.pszName = "Convert";
	mi.position = -1999901004;
	mi.hIcon = LoadIconEx( "convert" );
	hMenuConvert = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// "Add to roster"
	strcpy( tDest, "/AddToRoster" );
	CreateServiceFunction( text, JabberMenuRosterAdd );
	mi.pszName = "Add to roster";
	mi.position = -1999901005;
	mi.hIcon = LoadIconEx( "addroster" );
	hMenuRosterAdd = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Login/logout
	strcpy( tDest, "/TransportLogin" );
	CreateServiceFunction( text, JabberMenuTransportLogin );
	mi.pszName = "Login/logout";
	mi.position = -1999901006;
	mi.hIcon = LoadIconEx( "trlogonoff" );
	hMenuLogin = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Retrieve nicks
	strcpy( tDest, "/TransportGetNicks" );
	CreateServiceFunction( text, JabberMenuTransportResolve );
	mi.pszName = "Resolve nicks";
	mi.position = -1999901007;
	mi.hIcon = LoadIconEx( "trresolve" );
	hMenuRefresh = ( HANDLE ) JCallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	// Add Jabber menu to the main menu
	if ( !JGetByte( "DisableMainMenu", FALSE )) {
		CLISTMENUITEM clmi;
		memset( &clmi, 0, sizeof( CLISTMENUITEM ));
		clmi.cbSize = sizeof( CLISTMENUITEM );
		clmi.flags = CMIM_FLAGS | CMIF_GRAYED;

		// "Agents..."
		strcpy( tDest, "/Agents" );
		CreateServiceFunction( text, JabberMenuHandleAgents );

		mi.pszPopupName = jabberModuleName;
		mi.popupPosition = 500090000;
		mi.pszName = "Agents...";
		mi.position = 2000050000;
		mi.hIcon = LoadIconEx( "main" );
		mi.pszService = text;
		hMenuAgent = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
		JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuAgent, ( LPARAM )&clmi );

		// "Change Password..."
		strcpy( tDest, "/ChangePassword" );
		CreateServiceFunction( text, JabberMenuHandleChangePassword );
		mi.pszName = "Change Password...";
		mi.position = 2000050001;
		mi.hIcon = LoadIconEx( "key" );
		hMenuChangePassword = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
		JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuChangePassword, ( LPARAM )&clmi );

		// "Multi-User Conference..."
		strcpy( tDest, "/Groupchat" );
		CreateServiceFunction( text, JabberMenuHandleGroupchat );
		mi.pszName = "Multi-User Conference...";
		mi.position = 2000050002;
		mi.hIcon = LoadIconEx( "group" );
		hMenuGroupchat = ( HANDLE ) JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
		JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM ) hMenuGroupchat, ( LPARAM )&clmi );

		// "Personal vCard..."
		strcpy( tDest,  "/Vcard" );
		CreateServiceFunction( text, JabberMenuHandleVcard );
		mi.pszName = "Personal vCard...";
		mi.position = 2000050003;
		mi.hIcon = LoadIconEx( "vcard" );
		JCallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
}	}

void JabberEnableMenuItems( BOOL bEnable )
{
	CLISTMENUITEM clmi = { 0 };
	clmi.cbSize = sizeof( CLISTMENUITEM );
	clmi.flags = CMIM_FLAGS;
	if ( !bEnable )
		clmi.flags += CMIF_GRAYED;

	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuAgent, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuChangePassword, ( LPARAM )&clmi );
	JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuGroupchat, ( LPARAM )&clmi );
}
