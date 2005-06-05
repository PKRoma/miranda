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
#include "jabber_iq.h"
#include "resource.h"

extern HANDLE hInitChat;

/////////////////////////////////////////////////////////////////////////////////////////
// One string entry dialog

struct JabberEnterStringParams
{	char* result;
	size_t resultLen;
};

BOOL CALLBACK JabberEnterStringDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
	{
		TranslateDialogDefault( hwndDlg );
		JabberEnterStringParams* params = ( JabberEnterStringParams* )lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG )params );
		SetWindowText( hwndDlg, params->result );
		return TRUE;
	}

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDOK:
		{	JabberEnterStringParams* params = ( JabberEnterStringParams* )GetWindowLong( hwndDlg, GWL_USERDATA );
			GetDlgItemText( hwndDlg, IDC_TOPIC, params->result, params->resultLen );
			params->result[ params->resultLen-1 ] = 0;
			EndDialog( hwndDlg, 1 );
			break;
		}
		case IDCANCEL:
			EndDialog( hwndDlg, 0 );
			break;
	}	}

	return FALSE;
}

BOOL JabberEnterString( char* result, size_t resultLen )
{
	JabberEnterStringParams params = { result, resultLen };
	return DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_GROUPCHAT_INPUT ), NULL, JabberEnterStringDlgProc, LPARAM( &params ));
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetUserData - get an item by the room name

JABBER_LIST_ITEM* JabberGetUserData( char* szRoomName )
{
	GCDEST gcd = { jabberProtoName, szRoomName, GC_EVENT_GETITEMDATA };
	GCEVENT gce = {0};
	gce.cbSize = sizeof( gce );
	gce.pDest = &gcd;
	return ( JABBER_LIST_ITEM* )JCallService( MS_GC_EVENT, NULL, (LPARAM)&gce );
}

static char* sttRoles[] = { "Other", "Visitors", "Participants", "Moderators" };

int JabberGcInit( WPARAM wParam, LPARAM lParam )
{
	JABBER_LIST_ITEM* item = ( JABBER_LIST_ITEM* )wParam;
	GCWINDOW gcw = {0};
	GCEVENT gce = {0};

	char* szNick = JabberNickFromJID( item->jid );
	gcw.cbSize = sizeof(GCWINDOW);
	gcw.iType = GCW_CHATROOM;
	gcw.pszModule = jabberProtoName;
	gcw.pszName = szNick;
	gcw.pszID = item->jid;
	gcw.pszStatusbarText = NULL;
	gcw.bDisableNickList = FALSE;
	JCallService(MS_GC_NEWCHAT, NULL, (LPARAM)&gcw);
	free( szNick );

	item->bChatActive = TRUE;

	GCDEST gcd = { jabberProtoName, item->jid, GC_EVENT_ADDGROUP };
	gce.cbSize = sizeof(GCEVENT);
	gce.pDest = &gcd;
	for ( int i=sizeof(sttRoles)/sizeof(char*)-1; i >= 0; i-- ) {
		gce.pszStatus = Translate( sttRoles[i] );
		JCallService(MS_GC_EVENT, NULL, ( LPARAM )&gce );
	}

	gce.cbSize = sizeof(GCEVENT);
	gce.dwItemData = wParam;
	gce.pDest = &gcd;

	gcd.iType = GC_EVENT_SETITEMDATA;
	JCallService(MS_GC_EVENT, 0, (LPARAM)&gce);
	
	gcd.iType = GC_EVENT_CONTROL;
	JCallService(MS_GC_EVENT, WINDOW_INITDONE, (LPARAM)&gce);
	JCallService(MS_GC_EVENT, WINDOW_ONLINE, (LPARAM)&gce);
	JCallService(MS_GC_EVENT, WINDOW_VISIBLE, (LPARAM)&gce);
	return 0;
}

void JabberGcLogCreate( JABBER_LIST_ITEM* item )
{
	if ( !item->bChatActive )
		NotifyEventHooks( hInitChat, (WPARAM)item, 0 );
}

void JabberGcLogUpdateMemberStatus( JABBER_LIST_ITEM* item, const char* jid, char* nick )
{
	GCDEST gcd = { jabberProtoName, item->jid, GC_EVENT_PART };
	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.pszNick = gce.pszUID = nick;
	gce.pDest = &gcd;
	gce.bAddToLog = TRUE;
	gce.time = time(0);

	for ( int i=0; i < item->resourceCount; i++ ) {
		JABBER_RESOURCE_STATUS& JS = item->resource[i];
		if ( !strcmp( nick, JS.resourceName )) {
			gcd.iType = GC_EVENT_JOIN;
			gce.pszStatus = JTranslate( sttRoles[ JS.role ] );
			gce.bIsMe = JabberCompareJids( jid, jabberJID ) == 0;
			break;
	}	}

	JCallService( MS_GC_EVENT, NULL, ( LPARAM )&gce );
}

void JabberGcQuit( JABBER_LIST_ITEM* item, int code, XmlNode* reason )
{
	GCDEST gcd = { jabberProtoName, item->jid, GC_EVENT_CONTROL };
	GCEVENT gce = {0};
	gce.cbSize = sizeof(GCEVENT);
	gce.pszUID = item->jid;
	gce.pDest = &gcd;
	gce.pszText = ( reason != NULL ) ? reason->text : NULL;
	JCallService( MS_GC_EVENT, WINDOW_OFFLINE, ( LPARAM )&gce );
	JCallService( MS_GC_EVENT, WINDOW_HIDDEN, ( LPARAM )&gce );
	JCallService( MS_GC_EVENT, WINDOW_CLEARLOG, ( LPARAM )&gce );

	item->bChatActive = FALSE;

	if ( jabberOnline ) {
		JabberSend( jabberThreadInfo->s, "<presence to='%s' type='unavailable'/>", UTF8(item->jid));
		JabberListRemove( LIST_CHATROOM, item->jid );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// Context menu hooks

#define IDM_LEAVE       10
#define IDM_TOPIC       12

int JabberGcMenuHook( WPARAM wParam, LPARAM lParam )
{
	GCMENUITEMS* gcmi= ( GCMENUITEMS* )lParam;
	if ( gcmi == NULL )
		return 0;

	if ( lstrcmpi( gcmi->pszModule, jabberProtoName ))
		return 0;

	JABBER_LIST_ITEM* item = JabberGetUserData( gcmi->pszID );
	if ( item == NULL )
		return 0;

	JABBER_RESOURCE_STATUS *me = NULL, *him = NULL;
	for ( int i=0; i < item->resourceCount; i++ ) {
		JABBER_RESOURCE_STATUS& p = item->resource[i];
		if ( !lstrcmp( p.resourceName, item->nick   ))  me = &p;
		if ( !lstrcmp( p.resourceName, gcmi->pszUID ))  him = &p;
	}

	if ( gcmi->Type == MENU_ON_LOG ) {
		static struct gc_item sttLogListItems[] = {
			{ JTranslate( "&Leave chat session" ),    IDM_LEAVE,     MENU_ITEM, FALSE },
			{ NULL, 0, MENU_SEPARATOR, FALSE },
			{ JTranslate( "&Voice List..." ),         IDM_VOICE,     MENU_ITEM, TRUE  },
			{ JTranslate( "&Ban List..." ),           IDM_BAN,       MENU_ITEM, TRUE  },
			{ NULL, 0, MENU_SEPARATOR, FALSE },
			{ JTranslate( "&Member List..." ),        IDM_MEMBER,    MENU_ITEM, TRUE  },
			{ JTranslate( "Mo&derator List..." ),     IDM_MODERATOR, MENU_ITEM, TRUE  },
			{ JTranslate( "&Admin List..." ),         IDM_ADMIN,     MENU_ITEM, TRUE  },
			{ JTranslate( "&Owner List..." ),         IDM_OWNER,     MENU_ITEM, TRUE  },
			{ NULL, 0, MENU_SEPARATOR, FALSE },
			{ JTranslate( "Change &Nickname..." ),    IDM_NICK,      MENU_ITEM, FALSE },
			{ JTranslate( "Set &Topic..." ),          IDM_TOPIC,     MENU_ITEM, FALSE },
			{ JTranslate( "&Invite a User..." ),      IDM_INVITE,    MENU_ITEM, FALSE },
			{ JTranslate( "Room Con&figuration..." ), IDM_CONFIG,    MENU_ITEM, TRUE  },
			{ NULL, 0, MENU_SEPARATOR, FALSE },
			{ JTranslate( "Destroy Room..." ),        IDM_DESTROY,   MENU_ITEM, TRUE  }};

		gcmi->nItems = sizeof( sttLogListItems ) / sizeof( sttLogListItems[0] );
		gcmi->Item = sttLogListItems;

		if ( me != NULL ) {
			if ( me->role == ROLE_MODERATOR )
				sttLogListItems[2].bDisabled = FALSE;

			if ( me->affiliation == AFFILIATION_ADMIN )
				sttLogListItems[3].bDisabled = sttLogListItems[5].bDisabled = FALSE;
			else if ( me->affiliation == AFFILIATION_OWNER )
				sttLogListItems[3].bDisabled = sttLogListItems[5].bDisabled = 
				sttLogListItems[6].bDisabled = sttLogListItems[7].bDisabled = 
				sttLogListItems[8].bDisabled = sttLogListItems[13].bDisabled = 
				sttLogListItems[15].bDisabled = FALSE;
		}
	}
	else if ( gcmi->Type == MENU_ON_NICKLIST ) {
		static struct gc_item sttListItems[] = {
			{ JTranslate( "&Leave chat session" ), IDM_LEAVE, MENU_ITEM, FALSE },
			{ NULL, 0, MENU_SEPARATOR, FALSE },
			{ JTranslate( "Kick" ), IDM_KICK, MENU_ITEM, TRUE },
			{ JTranslate( "Ban" ),  IDM_BAN,  MENU_ITEM, TRUE },
			{ NULL, 0, MENU_SEPARATOR, FALSE },
			{ JTranslate( "Toggle &Voice" ),     IDM_VOICE,      MENU_ITEM, TRUE },
			{ JTranslate( "Toggle Moderator" ),  IDM_MODERATOR,  MENU_ITEM, TRUE },
			{ JTranslate( "Toggle Admin" ),      IDM_ADMIN,      MENU_ITEM, TRUE },
			{ JTranslate( "Toggle Owner" ),      IDM_OWNER,      MENU_ITEM, TRUE }};

		gcmi->nItems = sizeof( sttListItems )/sizeof( sttListItems[0] );
		gcmi->Item = sttListItems;

		if ( me != NULL && him != NULL ) {
			if ( me->role == ROLE_MODERATOR )
				if ( him->affiliation != AFFILIATION_ADMIN && him->affiliation != AFFILIATION_OWNER )
					sttListItems[2].bDisabled = sttListItems[3].bDisabled = FALSE;

			if ( me->affiliation == AFFILIATION_ADMIN ) {
				if ( him->affiliation != AFFILIATION_ADMIN && him->affiliation != AFFILIATION_OWNER )
					sttListItems[5].bDisabled = sttListItems[6].bDisabled = FALSE;
			}
			else if ( me->affiliation == AFFILIATION_OWNER )
				sttListItems[5].bDisabled = sttListItems[6].bDisabled = 
				sttListItems[7].bDisabled = sttListItems[8].bDisabled = FALSE;
	}	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Conference invitation dialog

static BOOL CALLBACK JabberGcLogInviteDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault( hwndDlg );
			SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )LoadIcon( hInst, MAKEINTRESOURCE( IDI_GROUP )) );
			SetDlgItemText( hwndDlg, IDC_ROOM, ( char* )lParam );
			HWND hwndComboBox = GetDlgItem( hwndDlg, IDC_USER );
			int index = 0;
			while (( index=JabberListFindNext( LIST_ROSTER, index )) >= 0 ) {
				JABBER_LIST_ITEM* item = JabberListGetItemPtrFromIndex( index );
				if ( item->status != ID_STATUS_OFFLINE ) {
					// Add every non-offline users to the combobox
					int n = SendMessage( hwndComboBox, CB_ADDSTRING, 0, ( LPARAM )item->jid );
					SendMessage( hwndComboBox, CB_SETITEMDATA, n, ( LPARAM )item->jid );
				}
				index++;
			}
			SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG ) _strdup(( char* )lParam ));
		}
		return TRUE;
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_INVITE:
			{
				char text[256], user[256], *pUser;
				char* room;
				HWND hwndComboBox;
				int iqId, n;

				hwndComboBox = GetDlgItem( hwndDlg, IDC_USER );
				if (( room=( char* )GetWindowLong( hwndDlg, GWL_USERDATA )) != NULL ) {
					n = SendMessage( hwndComboBox, CB_GETCURSEL, 0, 0 );
					if ( n < 0 ) {
						GetWindowText( hwndComboBox, user, sizeof( user ));
						pUser = user;
					}
					else pUser = ( char* )SendMessage( hwndComboBox, CB_GETITEMDATA, n, 0 );

					if ( pUser != NULL ) {
						GetDlgItemText( hwndDlg, IDC_REASON, text, sizeof( text ));
						iqId = JabberSerialNext();
						JabberSend( jabberThreadInfo->s, "<message id='"JABBER_IQID"%d' to='%s'><x xmlns='http://jabber.org/protocol/muc#user'><invite to='%s'><reason>%s</reason></invite></x></message>", 
							iqId, UTF8(room), UTF8(pUser), UTF8(text));
			}	}	}
			// Fall through
		case IDCANCEL:
		case IDCLOSE:
			DestroyWindow( hwndDlg );
			return TRUE;
		}
		break;
	case WM_CLOSE:
		DestroyWindow( hwndDlg );
		break;
	case WM_DESTROY:
		{
			char* str;

			if (( str=( char* )GetWindowLong( hwndDlg, GWL_USERDATA )) != NULL )
				free( str );
		}
		break;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Context menu processing

static char sttAdminXml[] = "<iq type='set' to='%s'>%s<item nick='%s' %s='%s'/></query></iq>";

static void sttNickListHook( JABBER_LIST_ITEM* item, GCHOOK* gch )
{
	JABBER_RESOURCE_STATUS *me = NULL, *him = NULL;
	for ( int i=0; i < item->resourceCount; i++ ) {
		JABBER_RESOURCE_STATUS& p = item->resource[i];
		if ( !lstrcmp( p.resourceName, item->nick  )) me = &p;
		if ( !lstrcmp( p.resourceName, gch->pszUID )) him = &p;
	}

	if ( him == NULL || me == NULL )
		return;

	char szBuffer[ 1024 ];

	switch( gch->dwData ) {
	case IDM_LEAVE: 
		JabberGcQuit( item, 0, 0 );
		break;

	case IDM_KICK:
		mir_snprintf( szBuffer, sizeof szBuffer, "%s %s", JTranslate( "Reason to kick" ), him->resourceName );
		if ( JabberEnterString( szBuffer, sizeof szBuffer ))
			JabberSend( jabberThreadInfo->s, "<iq type='set' to='%s'>%s<item nick='%s' role='none'><reason>%s</reason></item></query></iq>",
				xmlnsAdmin, UTF8(item->jid), UTF8(him->resourceName), UTF8(szBuffer));
		break;

	case IDM_BAN:
		mir_snprintf( szBuffer, sizeof szBuffer, "%s %s", JTranslate( "Reason to ban" ), him->resourceName );
		if ( JabberEnterString( szBuffer, sizeof szBuffer ))
			JabberSend( jabberThreadInfo->s, "<iq type='set' to='%s'>%s<item nick='%s' affiliation='outcast'><reason>%s</reason></item></query></iq>",
				xmlnsAdmin, UTF8(item->jid), UTF8(him->resourceName), UTF8(szBuffer));
		break;

	case IDM_VOICE:
		JabberSend( jabberThreadInfo->s, sttAdminXml, UTF8(item->jid), xmlnsAdmin, UTF8(item->nick), "role", 
			( him->role == ROLE_PARTICIPANT ) ? "visitor" : "participant" );
		break;

	case IDM_MODERATOR:
		JabberSend( jabberThreadInfo->s, sttAdminXml, UTF8(item->jid), xmlnsAdmin, UTF8(item->nick), "role", 
			( him->affiliation == ROLE_MODERATOR ) ? "participant" : "moderator" );
		break;

	case IDM_ADMIN:
		JabberSend( jabberThreadInfo->s, sttAdminXml, UTF8(item->jid), xmlnsAdmin, UTF8(item->nick), "affiliation", 
			( him->affiliation==AFFILIATION_ADMIN )? "member" : "admin" );
		break;

	case IDM_OWNER:
		JabberSend( jabberThreadInfo->s, sttAdminXml, UTF8(item->jid), xmlnsAdmin, UTF8(item->nick), "affiliation", 
			( him->affiliation==AFFILIATION_OWNER ) ? "admin" : "owner" );
		break;
}	}

static void sttLogListHook( JABBER_LIST_ITEM* item, GCHOOK* gch )
{
	int iqId;
	char szBuffer[ 1024 ];

	switch( gch->dwData ) {
	case IDM_VOICE:
		iqId = JabberSerialNext();
		JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultMucGetVoiceList );
		JabberSend( jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><item role='participant'/></query></iq>", 
			iqId, UTF8(gch->pDest->pszID));
		break;

	case IDM_MEMBER:
		iqId = JabberSerialNext();
		JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultMucGetMemberList );
		JabberSend( jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'>%s<item affiliation='member'/></query></iq>", 
			iqId, UTF8(gch->pDest->pszID), xmlnsAdmin );
		break;

	case IDM_MODERATOR:
		iqId = JabberSerialNext();
		JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultMucGetModeratorList );
		JabberSend( jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'>%s<item role='moderator'/></query></iq>", 
			iqId, UTF8(gch->pDest->pszID), xmlnsAdmin );
		break;

	case IDM_BAN:
		iqId = JabberSerialNext();
		JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultMucGetBanList );
		JabberSend( jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'>%s<item affiliation='outcast'/></query></iq>",
			iqId, UTF8(gch->pDest->pszID), xmlnsAdmin );
		break;

	case IDM_ADMIN:
		iqId = JabberSerialNext();
		JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultMucGetAdminList );
		JabberSend( jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'>%s<item affiliation='admin'/></query></iq>",
			iqId, UTF8(gch->pDest->pszID), xmlnsOwner );
		break;

	case IDM_OWNER:
		iqId = JabberSerialNext();
		JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultMucGetOwnerList );
		JabberSend( jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'>%s<item affiliation='owner'/></query></iq>", 
			iqId, UTF8(gch->pDest->pszID), xmlnsOwner );
		break;

	case IDM_TOPIC:
		mir_snprintf( szBuffer, sizeof szBuffer, "%s %s", JTranslate( "Set topic for" ), gch->pDest->pszID );
		if ( JabberEnterString( szBuffer, sizeof szBuffer ))
			JabberSend( jabberThreadInfo->s, "<message to='%s' type='groupchat'><subject>%s</subject></message>", 
				UTF8(gch->pDest->pszID), UTF8(szBuffer));
		break;

	case IDM_NICK:
		mir_snprintf( szBuffer, sizeof szBuffer, "%s %s", JTranslate( "Change nickname in" ), gch->pDest->pszID );
		if ( JabberEnterString( szBuffer, sizeof szBuffer )) {
			JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_CHATROOM, gch->pDest->pszID );
			if ( item != NULL ) {
				char text[ 1024 ];
				mir_snprintf( text, sizeof( text ), "%s/%s", gch->pDest->pszID, szBuffer );
				JabberSendPresenceTo( jabberStatus, text, NULL );
				if ( item->newNick != NULL )
					free( item->newNick );
				item->newNick = strdup( szBuffer );
		}	}
		break;

	case IDM_INVITE:
		CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_GROUPCHAT_INVITE ), NULL, JabberGcLogInviteDlgProc, ( LPARAM )gch->pDest->pszID );
		break;

	case IDM_CONFIG:
		iqId = JabberSerialNext();
		JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultGetMuc );
		JabberSend( jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'>%s</query></iq>", 
			iqId, UTF8(gch->pDest->pszID), xmlnsOwner );
		break;

	case IDM_DESTROY:
		mir_snprintf( szBuffer, sizeof szBuffer, "%s %s", JTranslate( "Reason to destroy" ), gch->pDest->pszID );
		if ( !JabberEnterString( szBuffer, sizeof szBuffer ))
			break;

		JabberSend( jabberThreadInfo->s, "<iq type='set' to='%s'>%s<destroy><reason>%s</reason></destroy></query></iq>", 
			UTF8(gch->pDest->pszID), xmlnsOwner, UTF8(szBuffer));

	case IDM_LEAVE: 
		JabberGcQuit( item, 0, 0 );
		break;
}	}

int JabberGcEventHook(WPARAM wParam,LPARAM lParam) 
{
	GCHOOK* gch = ( GCHOOK* )lParam;
	if ( gch == NULL )
		return 0;

	if ( lstrcmpi( gch->pDest->pszModule, jabberProtoName ))
		return 0;

	JABBER_LIST_ITEM* item = JabberGetUserData( gch->pDest->pszID );
	if ( item == NULL )
		return 0;

	switch ( gch->pDest->iType ) {
	case GC_USER_MESSAGE:
		if ( gch->pszText && lstrlen( gch->pszText) > 0 ) {
			rtrim( gch->pszText );

			if ( jabberOnline ) {
				char* str = JabberTextEncode( gch->pszText );
				if ( str != NULL ) {
					JabberSend( jabberThreadInfo->s, "<message to='%s' type='groupchat'><body>%s</body></message>", UTF8(item->jid), str );
					free( str );
		}	}	}
		break;
		
	case GC_USER_LOGMENU:
		sttLogListHook( item, gch );
		break;

	case GC_USER_NICKLISTMENU:
		sttNickListHook( item, gch );
		break;
	}

	return 0;
}
