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
#include "jabber_iq.h"
#include "jabber_caps.h"
#include "jabber_privacy.h"

#include "m_genmenu.h"
#include "m_clistint.h"

void CJabberProto::OnIqResultServerDiscoInfo( XmlNode& iqNode, void* userdata )
{
	if ( !iqNode )
		return;

	const TCHAR *type = iqNode.getAttrValue( _T("type"));
	int i;

	if ( !_tcscmp( type, _T("result"))) {
		XmlNode query = iqNode.getChildByTag( "query", "xmlns", _T(JABBER_FEAT_DISCO_INFO) );
		if ( !query )
			return;

		XmlNode identity;
		for ( i = 1; ( identity = query.getNthChild( _T("identity"), i )) != NULL; i++ ) {
			const TCHAR *identityCategory = identity.getAttrValue( _T("category"));
			const TCHAR *identityType = identity.getAttrValue( _T("type"));
			if ( identityCategory && identityType && !_tcscmp( identityCategory, _T("pubsub") ) && !_tcscmp( identityType, _T("pep")) ) {
				m_bPepSupported = TRUE;

				RebuildStatusMenu();
				RebuildInfoFrame();
				break;
			}
		}
		if ( m_ThreadInfo ) {
			m_ThreadInfo->jabberServerCaps = JABBER_RESOURCE_CAPS_NONE;
			XmlNode feature;
			for ( i = 1; ( feature = query.getNthChild( _T("feature"), i )) != NULL; i++ ) {
				const TCHAR *featureName = feature.getAttrValue( _T("var"));
				if ( featureName ) {
					for ( int j = 0; g_JabberFeatCapPairs[j].szFeature; j++ ) {
						if ( !_tcscmp( g_JabberFeatCapPairs[j].szFeature, featureName )) {
							m_ThreadInfo->jabberServerCaps |= g_JabberFeatCapPairs[j].jcbCap;
							break;
						}	
					}	
				}	
			}	
		}
		OnProcessLoginRq((ThreadData *)userdata, JABBER_LOGIN_SERVERINFO);
	}	
}

void CJabberProto::OnIqResultNestedRosterGroups( XmlNode& iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	const TCHAR *szGroupDelimeter = NULL;
	BOOL bPrivateStorageSupport = FALSE;

	if ( iqNode && pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT ) {
		XmlNode pQuery = iqNode.getChildByTag( "query", "xmlns", _T( JABBER_FEAT_PRIVATE_STORAGE ));
		if ( pQuery ) {
			bPrivateStorageSupport = TRUE;
			XmlNode pRoster = pQuery.getChildByTag( "roster", "xmlns", _T( JABBER_FEAT_NESTED_ROSTER_GROUPS ));
			if ( pRoster && pRoster.getText() && *pRoster.getText() )
				szGroupDelimeter = pRoster.getText();
		}
	}

	// global fuckup
	if ( !m_ThreadInfo )
		return;

	// is our default delimiter?
	if (( !szGroupDelimeter && bPrivateStorageSupport ) || ( szGroupDelimeter && _tcscmp( szGroupDelimeter, _T("\\") ))) {
		XmlNodeIq iqNRG( "set", SerialNext() );
		XmlNode query = iqNRG.addQuery( _T(JABBER_FEAT_PRIVATE_STORAGE));
		XmlNode roster = query.addChild( "roster", _T("\\") );
		roster.addAttr( "xmlns", JABBER_FEAT_NESTED_ROSTER_GROUPS );
		m_ThreadInfo->send( iqNRG );
	}

	// roster request
	TCHAR *szUserData = mir_tstrdup( szGroupDelimeter ? szGroupDelimeter : _T("\\") );
	XmlNodeIq iq( m_iqManager.AddHandler( &CJabberProto::OnIqResultGetRoster, JABBER_IQ_TYPE_GET, NULL, 0, -1, (void *)szUserData ));
	XmlNode query = iq.addChild( "query" ); query.addAttr( "xmlns", JABBER_FEAT_IQ_ROSTER );
	m_ThreadInfo->send( iq );
}

void CJabberProto::OnProcessLoginRq( ThreadData* info, DWORD rq )
{
	info->dwLoginRqs |= rq;

	if ((info->dwLoginRqs & JABBER_LOGIN_ROSTER) && (info->dwLoginRqs & JABBER_LOGIN_BOOKMARKS) &&
		(info->dwLoginRqs & JABBER_LOGIN_SERVERINFO) && !(info->dwLoginRqs & JABBER_LOGIN_BOOKMARKS_AJ))
	{
		if ( jabberChatDllPresent && JGetByte( "AutoJoinBookmarks", TRUE ) ) {
			JABBER_LIST_ITEM* item;
			for ( int i=0; ( i = ListFindNext( LIST_BOOKMARK, i )) >= 0; i++ ) {
				if ((( item = ListGetItemPtrFromIndex( i )) != NULL ) && !lstrcmp( item->type, _T("conference") )) {
					if ( item->bAutoJoin ) {
						TCHAR room[256], *server, *p;
						TCHAR text[128];
						_tcsncpy( text, item->jid, SIZEOF( text ));
						_tcsncpy( room, text, SIZEOF( room ));
						p = _tcstok( room, _T( "@" ));
						server = _tcstok( NULL, _T( "@" ));
						if ( item->nick && item->nick[0] != 0 )
							GroupchatJoinRoom( server, p, item->nick, item->password, true );
						else {
							TCHAR* nick = JabberNickFromJID( m_szJabberJID );
							GroupchatJoinRoom( server, p, nick, item->password, true );
							mir_free( nick );
		}	}	}	}	}

		OnProcessLoginRq( info, JABBER_LOGIN_BOOKMARKS_AJ );
	}
}

void CJabberProto::OnLoggedIn( ThreadData* info )
{
	m_bJabberOnline = TRUE;
	m_tmJabberLoggedInTime = time(0);

	info->dwLoginRqs = 0;

	// XEP-0083 support
	{
		CJabberIqInfo* pIqInfo = m_iqManager.AddHandler( &CJabberProto::OnIqResultNestedRosterGroups, JABBER_IQ_TYPE_GET );
		// ugly hack to prevent hangup during login process
		pIqInfo->SetTimeout( 30000 );
		XmlNodeIq iqNRG( pIqInfo );
		XmlNode query = iqNRG.addQuery( _T(JABBER_FEAT_PRIVATE_STORAGE));
		XmlNode roster = query.addChild( "roster" );
		roster.addAttr( "xmlns", JABBER_FEAT_NESTED_ROSTER_GROUPS );
		info->send( iqNRG );
	}

	int iqId = SerialNext();
	IqAdd( iqId, IQ_PROC_DISCOBOOKMARKS, &CJabberProto::OnIqResultDiscoBookmarks);
	XmlNodeIq biq( "get", iqId);
	XmlNode bquery = biq.addQuery( _T(JABBER_FEAT_PRIVATE_STORAGE));
	XmlNode storage = bquery.addChild( "storage" );
	storage.addAttr( "xmlns", "storage:bookmarks" );
	info->send( biq );

	m_bPepSupported = FALSE;
	info->jabberServerCaps = JABBER_RESOURCE_CAPS_NONE;
	iqId = SerialNext();
	IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultServerDiscoInfo );
	XmlNodeIq diq( "get", iqId, info->server );
	diq.addQuery( _T(JABBER_FEAT_DISCO_INFO));
	info->send( diq );

	QueryPrivacyLists( info );

	char szServerName[ sizeof(info->server) ];
	if ( JGetStaticString( "LastLoggedServer", NULL, szServerName, sizeof(szServerName)))
		SendGetVcard( m_szJabberJID );
	else if ( strcmp( info->server, szServerName ))
		SendGetVcard( m_szJabberJID );
	JSetString( NULL, "LastLoggedServer", info->server );

	//Check if avatar changed
	DBVARIANT dbvSaved = {0};
	DBVARIANT dbvHash = {0};
	int resultSaved = JGetStringT( NULL, "AvatarSaved", &dbvSaved );
	int resultHash  = JGetStringT( NULL, "AvatarHash", &dbvHash );
	if ( resultSaved || resultHash || lstrcmp( dbvSaved.ptszVal, dbvHash.ptszVal ))	{
		char tFileName[ MAX_PATH ];
		GetAvatarFileName( NULL, tFileName, MAX_PATH );
		SetServerVcard( TRUE, tFileName );
	}
	DBFreeVariant(&dbvSaved);
	DBFreeVariant(&dbvHash);
}

void CJabberProto::OnIqResultGetAuth( XmlNode& iqNode, void *userdata )
{
	// RECVED: result of the request for authentication method
	// ACTION: send account authentication information to log in
	Log( "<iq/> iqIdGetAuth" );

	ThreadData* info = ( ThreadData* ) userdata;
	XmlNode queryNode;
	const TCHAR* type;
	if (( type=iqNode.getAttrValue( _T("type"))) == NULL ) return;
	if (( queryNode=iqNode.getChild( "query" )) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		int iqId = SerialNext();
		IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultSetAuth );

		XmlNodeIq iq( "set", iqId );
		XmlNode query = iq.addQuery( _T("jabber:iq:auth"));
		query.addChild( "username", info->username );
		if ( queryNode.getChild( "digest" )!=NULL && m_szStreamId ) {
			char* str = mir_utf8encode( info->password );
			char text[200];
			mir_snprintf( text, SIZEOF(text), "%s%s", m_szStreamId, str );
			mir_free( str );
         if (( str=JabberSha1( text )) != NULL ) {
				query.addChild( "digest", str );
				mir_free( str );
			}
		}
		else if ( queryNode.getChild( "password" ) != NULL )
			query.addChild( "password", info->password );
		else {
			Log( "No known authentication mechanism accepted by the server." );

			info->send( "</stream:stream>" );
			return;
		}

		if ( queryNode.getChild( "resource" ) != NULL )
			query.addChild( "resource", info->resource );

		info->send( iq );
	}
	else if ( !lstrcmp( type, _T("error"))) {
 		info->send( "</stream:stream>" );

		TCHAR text[128];
		mir_sntprintf( text, SIZEOF( text ), _T("%s %s."), TranslateT( "Authentication failed for" ), info->username );
		MessageBox( NULL, text, TranslateT( "Jabber Authentication" ), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND );
		JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD );
		m_ThreadInfo = NULL;	// To disallow auto reconnect
}	}

void CJabberProto::OnIqResultSetAuth( XmlNode& iqNode, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;
	const TCHAR* type;

	// RECVED: authentication result
	// ACTION: if successfully logged in, continue by requesting roster list and set my initial status
	Log( "<iq/> iqIdSetAuth" );
	if (( type=iqNode.getAttrValue( _T("type"))) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		DBVARIANT dbv;
		if ( JGetStringT( NULL, "Nick", &dbv ))
			JSetStringT( NULL, "Nick", info->username );
		else
			JFreeVariant( &dbv );

		OnLoggedIn( info );
	}
	// What to do if password error? etc...
	else if ( !lstrcmp( type, _T("error"))) {
		TCHAR text[128];

		info->send( "</stream:stream>" );
		mir_sntprintf( text, SIZEOF( text ), _T("%s %s."), TranslateT( "Authentication failed for" ), info->username );
		MessageBox( NULL, text, TranslateT( "Jabber Authentication" ), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND );
		JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD );
		m_ThreadInfo = NULL;	// To disallow auto reconnect
}	}

void CJabberProto::OnIqResultBind( XmlNode& iqNode, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;
	XmlNode n = iqNode.getChild( "bind" );
	if ( n != NULL ) {
		if ( n = n.getChild( "jid" )) {
			if ( n.getText() ) {
				if ( !_tcsncmp( info->fullJID, n.getText(), SIZEOF( info->fullJID )))
					Log( "Result Bind: "TCHAR_STR_PARAM" %s "TCHAR_STR_PARAM, info->fullJID, "confirmed.", NULL );
				else {
					Log( "Result Bind: "TCHAR_STR_PARAM" %s "TCHAR_STR_PARAM, info->fullJID, "changed to", n.getText());
					_tcsncpy( info->fullJID, n.getText(), SIZEOF( info->fullJID ));
		}	}	}

		if ( info->bIsSessionAvailable ) {
			int iqId = SerialNext();
			IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultSession );

			XmlNodeIq iq( "set" ); iq.addAttrID( iqId );
			iq.addChild( "session" ).addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-session" );
			info->send( iq );
		}
		else OnLoggedIn( info );
	}
   else if ( n = n.getChild( "error" )) {
		//rfc3920 page 39
		info->send( "</stream:stream>" );
		m_ThreadInfo = NULL;	// To disallow auto reconnect
}	}

void CJabberProto::OnIqResultSession( XmlNode& iqNode, void *userdata )
{
	ThreadData* info = ( ThreadData* )userdata;

	const TCHAR* type;
	if (( type=iqNode.getAttrValue( _T("type"))) == NULL ) return;

	if ( !lstrcmp( type, _T("result")))
		OnLoggedIn( info );
}

void CJabberProto::GroupchatJoinByHContact( HANDLE hContact, bool autojoin )
{
	DBVARIANT dbv;
	if( JGetStringT( hContact, "ChatRoomID", &dbv ))
		return;
	if( dbv.type != DBVT_ASCIIZ && dbv.type != DBVT_WCHAR )
		return;

	TCHAR* roomjid = mir_tstrdup( dbv.ptszVal );
	JFreeVariant( &dbv );
	if( !roomjid ) return;

	TCHAR* room = roomjid;
	TCHAR* server = _tcschr( roomjid, '@' );
	if( !server )
		return;
	server[0] = '\0'; server++;

	TCHAR nick[ 256 ];
	if ( JGetStringT( hContact, "MyNick", &dbv )) {
		TCHAR* jidnick = JabberNickFromJID( m_szJabberJID );
		if( !jidnick ) {
			mir_free( jidnick );
			mir_free( roomjid );
			return;
		}
		_tcsncpy( nick, jidnick, SIZEOF( nick ));
		mir_free( jidnick );
	}
	else {
		_tcsncpy( nick, dbv.ptszVal, SIZEOF( nick ));
		JFreeVariant( &dbv );
	}

	GroupchatJoinRoom( server, room, nick, _T(""), autojoin);
	mir_free( roomjid );
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberIqResultGetRoster - populates LIST_ROSTER and creates contact for any new rosters

void CJabberProto::OnIqResultGetRoster( XmlNode& iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	Log( "<iq/> iqIdGetRoster" );
	TCHAR *szGroupDelimeter = (TCHAR *)pInfo->GetUserData();
	if ( pInfo->GetIqType() != JABBER_IQ_TYPE_RESULT ) {
		mir_free( szGroupDelimeter );
		return;
	}

	XmlNode queryNode = iqNode.getChild( "query" );
	if ( queryNode == NULL ) {
		mir_free( szGroupDelimeter );
		return;
	}

	if ( lstrcmp( queryNode.getAttrValue( _T("xmlns")), _T(JABBER_FEAT_IQ_ROSTER))) {
		mir_free( szGroupDelimeter );
		return;
	}

	if ( !_tcscmp( szGroupDelimeter, _T("\\"))) {
		mir_free( szGroupDelimeter );
		szGroupDelimeter = NULL;
	}

	TCHAR* nick;
	int i;
	SortedList chatRooms = {0};
	chatRooms.increment = 10;

	for ( i=0; ; i++ ) {
		BOOL bIsTransport=FALSE;

		XmlNode itemNode = queryNode.getChild(i);
		if ( !itemNode )
			break;

		if ( _tcscmp( itemNode.getName(), _T("item")))
			continue;

		const TCHAR* str = itemNode.getAttrValue( _T("subscription")), *name;

		JABBER_SUBSCRIPTION sub;
		if ( str == NULL ) sub = SUB_NONE;
		else if ( !_tcscmp( str, _T("both"))) sub = SUB_BOTH;
		else if ( !_tcscmp( str, _T("to"))) sub = SUB_TO;
		else if ( !_tcscmp( str, _T("from"))) sub = SUB_FROM;
		else sub = SUB_NONE;

		const TCHAR* jid = itemNode.getAttrValue( _T("jid"));
		if ( jid == NULL )
			continue;
		if ( _tcschr( jid, '@' ) == NULL )
			bIsTransport = TRUE;

		if (( name = itemNode.getAttrValue( _T("name") )) != NULL )
			nick = mir_tstrdup( name );
		else
			nick = JabberNickFromJID( jid );

		if ( nick == NULL )
			continue;

		JABBER_LIST_ITEM* item = ListAdd( LIST_ROSTER, jid );
		item->subscription = sub;

		mir_free( item->nick ); item->nick = nick;

		XmlNode groupNode = itemNode.getChild( "group" );
		replaceStr( item->group, ( groupNode ) ? groupNode.getText() : NULL );

		// check group delimiters:
		if ( item->group && szGroupDelimeter ) {
			TCHAR *szPos = NULL;
			while ( szPos = _tcsstr( item->group, szGroupDelimeter )) {
				*szPos = 0;
				szPos += _tcslen( szGroupDelimeter );
				TCHAR *szNewGroup = (TCHAR *)mir_alloc( sizeof(TCHAR) * ( _tcslen( item->group ) + _tcslen( szPos ) + 2));
				_tcscpy( szNewGroup, item->group );
				_tcscat( szNewGroup, _T("\\"));
				_tcscat( szNewGroup, szPos );
				mir_free( item->group );
				item->group = szNewGroup;
			}
		}

		HANDLE hContact = HContactFromJID( jid );
		if ( hContact == NULL ) {
			// Received roster has a new JID.
			// Add the jid ( with empty resource ) to Miranda contact list.
			hContact = DBCreateContact( jid, nick, FALSE, FALSE );
		}

		if ( name != NULL ) {
			DBVARIANT dbNick;
			if ( !JGetStringT( hContact, "Nick", &dbNick )) {
				if ( lstrcmp( nick, dbNick.ptszVal ) != 0 )
					DBWriteContactSettingTString( hContact, "CList", "MyHandle", nick );
				else
					DBDeleteContactSetting( hContact, "CList", "MyHandle" );

				JFreeVariant( &dbNick );
			}
			else DBWriteContactSettingTString( hContact, "CList", "MyHandle", nick );
		}
		else DBDeleteContactSetting( hContact, "CList", "MyHandle" );

		if ( JGetByte( hContact, "ChatRoom", 0 )) {
			GCSESSION gcw = {0};
			gcw.cbSize = sizeof(GCSESSION);
			gcw.iType = GCW_CHATROOM;
			gcw.pszModule = m_szModuleName;
			gcw.dwFlags = GC_TCHAR;
			gcw.ptszID = jid;
			gcw.ptszName = NEWTSTR_ALLOCA( jid );

			TCHAR* p = (TCHAR*)_tcschr( gcw.ptszName, '@' );
			if ( p )
				*p = 0;

			CallServiceSync( MS_GC_NEWSESSION, 0, ( LPARAM )&gcw );

			DBDeleteContactSetting( hContact, "CList", "Hidden" );
			li.List_Insert( &chatRooms, hContact, chatRooms.realCount );
		}

		if ( item->group != NULL ) {
			JabberContactListCreateGroup( item->group );

			// Don't set group again if already correct, or Miranda may show wrong group count in some case
			DBVARIANT dbv;
			if ( !DBGetContactSettingTString( hContact, "CList", "Group", &dbv )) {
				if ( lstrcmp( dbv.ptszVal, item->group ))
					DBWriteContactSettingTString( hContact, "CList", "Group", item->group );
				JFreeVariant( &dbv );
			}
			else DBWriteContactSettingTString( hContact, "CList", "Group", item->group );
		}
		else DBDeleteContactSetting( hContact, "CList", "Group" );
		if ( hContact != NULL ) {
			if ( bIsTransport)
				JSetByte( hContact, "IsTransport", TRUE );
			else
				JSetByte( hContact, "IsTransport", FALSE );
		}
	}

	// Delete orphaned contacts ( if roster sync is enabled )
	if ( JGetByte( "RosterSync", FALSE ) == TRUE ) {
		int listSize = 0, listAllocSize = 0;
		HANDLE* list = NULL;
		HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		while ( hContact != NULL ) {
			char* str = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
			if ( str != NULL && !strcmp( str, m_szModuleName )) {
				DBVARIANT dbv;
				if ( !JGetStringT( hContact, "jid", &dbv )) {
					if ( !ListExist( LIST_ROSTER, dbv.ptszVal )) {
						Log( "Syncing roster: preparing to delete " TCHAR_STR_PARAM " ( hContact=0x%x )", dbv.ptszVal, hContact );
						if ( listSize >= listAllocSize ) {
							listAllocSize = listSize + 100;
							if (( list=( HANDLE * ) mir_realloc( list, listAllocSize * sizeof( HANDLE ))) == NULL ) {
								listSize = 0;
								break;
						}	}

						list[listSize++] = hContact;
					}
					JFreeVariant( &dbv );
			}	}

			hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
		}

		for ( i=0; i < listSize; i++ ) {
			Log( "Syncing roster: deleting 0x%x", list[i] );
			JCallService( MS_DB_CONTACT_DELETE, ( WPARAM ) list[i], 0 );
		}
		if ( list != NULL )
			mir_free( list );
	}

	EnableMenuItems( TRUE );

	Log( "Status changed via THREADSTART" );
	m_bModeMsgStatusChangePending = FALSE;
	SetServerStatus( m_iDesiredStatus );

	if ( JGetByte( "AutoJoinConferences", 0 )) {
		for ( i=0; i < chatRooms.realCount; i++ )
			GroupchatJoinByHContact(( HANDLE )chatRooms.items[i], true);
	}
	li.List_Destroy( &chatRooms );


	//UI_SAFE_NOTIFY(m_pDlgJabberJoinGroupchat, WM_JABBER_CHECK_ONLINE);
	//UI_SAFE_NOTIFY(m_pDlgBookmarks, WM_JABBER_CHECK_ONLINE);
	UI_SAFE_NOTIFY_HWND(m_hwndJabberAddBookmark, WM_JABBER_CHECK_ONLINE);
	WindowNotify(WM_JABBER_CHECK_ONLINE);

	UI_SAFE_NOTIFY(m_pDlgServiceDiscovery, WM_JABBER_TRANSPORT_REFRESH);

	if ( szGroupDelimeter )
		mir_free( szGroupDelimeter );

	OnProcessLoginRq((ThreadData *)userdata, JABBER_LOGIN_ROSTER);
	RebuildInfoFrame();
}

void CJabberProto::OnIqResultGetRegister( XmlNode& iqNode, void *userdata )
{
	// RECVED: result of the request for ( agent ) registration mechanism
	// ACTION: activate ( agent ) registration input dialog
	Log( "<iq/> iqIdGetRegister" );

	ThreadData* info = ( ThreadData* ) userdata;
	XmlNode queryNode, n;
	const TCHAR *type;
	if (( type = iqNode.getAttrValue( _T("type"))) == NULL ) return;
	if (( queryNode = iqNode.getChild( "query" )) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		if ( m_hwndAgentRegInput ) {
			XmlNode n = iqNode;
			SendMessage( m_hwndAgentRegInput, WM_JABBER_REGINPUT_ACTIVATE, 1 /*success*/, ( LPARAM )&n );
		}
	}
	else if ( !lstrcmp( type, _T("error"))) {
		if ( m_hwndAgentRegInput ) {
			XmlNode errorNode = iqNode.getChild( "error" );
			TCHAR* str = JabberErrorMsg( errorNode );
			SendMessage( m_hwndAgentRegInput, WM_JABBER_REGINPUT_ACTIVATE, 0 /*error*/, ( LPARAM )str );
			mir_free( str );
}	}	}

void CJabberProto::OnIqResultSetRegister( XmlNode& iqNode, void *userdata )
{
	// RECVED: result of registration process
	// ACTION: notify of successful agent registration
	Log( "<iq/> iqIdSetRegister" );

	const TCHAR *type, *from;
	if (( type = iqNode.getAttrValue( _T("type"))) == NULL ) return;
	if (( from = iqNode.getAttrValue( _T("from"))) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		HANDLE hContact = HContactFromJID( from );
		if ( hContact != NULL )
			JSetByte( hContact, "IsTransport", TRUE );

		if ( m_hwndRegProgress )
			SendMessage( m_hwndRegProgress, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )TranslateT( "Registration successful" ));
	}
	else if ( !lstrcmp( type, _T("error"))) {
		if ( m_hwndRegProgress ) {
			XmlNode errorNode = iqNode.getChild( "error" );
			TCHAR* str = JabberErrorMsg( errorNode );
			SendMessage( m_hwndRegProgress, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )str );
			mir_free( str );
}	}	}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberIqResultGetVcard - processes the server-side v-card

void CJabberProto::OnIqResultGetVcardPhoto( const TCHAR* jid, XmlNode& n, HANDLE hContact, BOOL& hasPhoto )
{
	Log( "JabberIqResultGetVcardPhoto: %d", hasPhoto );
	if ( hasPhoto )
		return;

	XmlNode o = n.getChild( "BINVAL" );
	if ( o == NULL || o.getText() == NULL )
		return;

	int bufferLen;
	char* buffer = JabberBase64Decode( o.getText(), &bufferLen );
	if ( buffer == NULL )
		return;

	char* szPicType;
	XmlNode m = n.getChild( "TYPE" );
	if ( m == NULL || m.getText() == NULL ) {
LBL_NoTypeSpecified:
		switch( JabberGetPictureType( buffer )) {
		case PA_FORMAT_GIF:	szPicType = "image/gif";	break;
		case PA_FORMAT_BMP:  szPicType = "image/bmp";	break;
		case PA_FORMAT_PNG:  szPicType = "image/png";	break;
		case PA_FORMAT_JPEG: szPicType = "image/jpeg";	break;
		default:
			goto LBL_Ret;
		}
	}
	else {
		if ( !_tcscmp( m.getText(), _T("image/jpeg")))
			szPicType = "image/jpeg";
		else if ( !_tcscmp( m.getText(), _T("image/png")))
			szPicType = "image/png";
		else if ( !_tcscmp( m.getText(), _T("image/gif")))
			szPicType = "image/gif";
		else if ( !_tcscmp( m.getText(), _T("image/bmp")))
			szPicType = "image/bmp";
		else
			goto LBL_NoTypeSpecified;
	}

	DWORD nWritten;
	char szTempPath[MAX_PATH], szAvatarFileName[MAX_PATH];
	JABBER_LIST_ITEM *item;
	DBVARIANT dbv;

	if ( hContact ) {
		if ( GetTempPathA( sizeof( szTempPath ), szTempPath ) <= 0 )
			lstrcpyA( szTempPath, ".\\" );
		if ( !GetTempFileNameA( szTempPath, m_szModuleName, GetTickCount(), szAvatarFileName )) {
LBL_Ret:
			mir_free( buffer );
			return;
		}

		char* p = strrchr( szAvatarFileName, '.' );
		if ( p != NULL )
			lstrcpyA( p+1, szPicType + 6 );	
	}
	else GetAvatarFileName( NULL, szAvatarFileName, sizeof( szAvatarFileName ));

	Log( "Picture file name set to %s", szAvatarFileName );
	HANDLE hFile = CreateFileA( szAvatarFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile == INVALID_HANDLE_VALUE )
		goto LBL_Ret;

	Log( "Writing %d bytes", bufferLen );
	if ( !WriteFile( hFile, buffer, bufferLen, &nWritten, NULL ))
		goto LBL_Ret;

	Log( "%d bytes written", nWritten );
	if ( hContact == NULL ) {
		hasPhoto = TRUE;
		Log( "My picture saved to %s", szAvatarFileName );
	}
	else if ( !JGetStringT( hContact, "jid", &dbv )) {
		item = ListGetItemPtr( LIST_ROSTER, jid );
		if ( item == NULL ) {
			item = ListAdd( LIST_VCARD_TEMP, jid ); // adding to the temp list to store information about photo
			item->bUseResource = TRUE;
		}
		if (item != NULL ) {
			hasPhoto = TRUE;
			if ( item->photoFileName )
				DeleteFileA( item->photoFileName );
			replaceStr( item->photoFileName, szAvatarFileName );
			Log( "Contact's picture saved to %s", szAvatarFileName );
		}
		JFreeVariant( &dbv );
	}

	CloseHandle( hFile );

	if ( !hasPhoto )
		DeleteFileA( szAvatarFileName );

	goto LBL_Ret;
}

static char* sttGetText( XmlNode node, char* tag )
{
	XmlNode n = node.getChild( tag );
	if ( n == NULL )
		return NULL;

	return mir_t2a( n.getText() );
}

void CJabberProto::OnIqResultGetVcard( XmlNode& iqNode, void *userdata )
{
	XmlNode vCardNode, m, n, o;
	const TCHAR* type, *jid;
	HANDLE hContact;
	TCHAR text[128];
	DBVARIANT dbv;

	Log( "<iq/> iqIdGetVcard" );
	if (( type = iqNode.getAttrValue( _T("type"))) == NULL ) return;
	if (( jid = iqNode.getAttrValue( _T("from"))) == NULL ) return;
	int id = JabberGetPacketID( iqNode );

	if ( id == m_nJabberSearchID ) {
		m_nJabberSearchID = -1;

		if (( vCardNode = iqNode.getChild( "vCard" )) != NULL ) {
			if ( !lstrcmp( type, _T("result"))) {
				JABBER_SEARCH_RESULT jsr = { 0 };
				jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
				jsr.hdr.nick = sttGetText( vCardNode, "NICKNAME" );
				jsr.hdr.firstName = sttGetText( vCardNode, "FN" );
				jsr.hdr.lastName = "";
				jsr.hdr.email = sttGetText( vCardNode, "EMAIL" );
				_tcsncpy( jsr.jid, jid, SIZEOF( jsr.jid ));
				jsr.jid[ SIZEOF( jsr.jid )-1 ] = '\0';
				JSendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, ( HANDLE )id, ( LPARAM )&jsr );
				JSendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, ( HANDLE )id, 0 );
				mir_free( jsr.hdr.nick );
				mir_free( jsr.hdr.firstName );
				mir_free( jsr.hdr.email );
			}
			else if ( !lstrcmp( type, _T("error")))
				JSendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, ( HANDLE )id, 0 );
		}
		else JSendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, ( HANDLE )id, 0 );
		return;
	}

	int len = _tcslen( m_szJabberJID );
	if ( !_tcsnicmp( jid, m_szJabberJID, len ) && ( jid[len]=='/' || jid[len]=='\0' )) {
		hContact = NULL;
		Log( "Vcard for myself" );
	}
	else {
		if (( hContact = HContactFromJID( jid )) == NULL )
			return;
		Log( "Other user's vcard" );
	}

	if ( !lstrcmp( type, _T("result"))) {
		BOOL hasFn, hasNick, hasGiven, hasFamily, hasMiddle, hasBday, hasGender;
		BOOL hasPhone, hasFax, hasCell, hasUrl;
		BOOL hasHome, hasHomeStreet, hasHomeStreet2, hasHomeLocality, hasHomeRegion, hasHomePcode, hasHomeCtry;
		BOOL hasWork, hasWorkStreet, hasWorkStreet2, hasWorkLocality, hasWorkRegion, hasWorkPcode, hasWorkCtry;
		BOOL hasOrgname, hasOrgunit, hasRole, hasTitle;
		BOOL hasDesc, hasPhoto;
		int nEmail, nPhone, nYear, nMonth, nDay;

		hasFn = hasNick = hasGiven = hasFamily = hasMiddle = hasBday = hasGender = FALSE;
		hasPhone = hasFax = hasCell = hasUrl = FALSE;
		hasHome = hasHomeStreet = hasHomeStreet2 = hasHomeLocality = hasHomeRegion = hasHomePcode = hasHomeCtry = FALSE;
		hasWork = hasWorkStreet = hasWorkStreet2 = hasWorkLocality = hasWorkRegion = hasWorkPcode = hasWorkCtry = FALSE;
		hasOrgname = hasOrgunit = hasRole = hasTitle = FALSE;
		hasDesc = hasPhoto = FALSE;
		nEmail = nPhone = 0;

		if (( vCardNode = iqNode.getChild( "vCard" )) != NULL ) {
			for ( int i=0; ; i++ ) {
				n = vCardNode.getChild(i);
				if ( !n )
					break;
				if ( n.getName() == NULL ) continue;
				if ( !_tcscmp( n.getName(), _T("FN"))) {
					if ( n.getText() != NULL ) {
						hasFn = TRUE;
						JSetStringT( hContact, "FullName", n.getText() );
					}
				}
				else if ( !lstrcmp( n.getName(), _T("NICKNAME"))) {
					if ( n.getText() != NULL ) {
						hasNick = TRUE;
						JSetStringT( hContact, "Nick", n.getText() );
					}
				}
				else if ( !lstrcmp( n.getName(), _T("N"))) {
					// First/Last name
					if ( !hasGiven && !hasFamily && !hasMiddle ) {
						if (( m=n.getChild( "GIVEN" )) != NULL && m.getText()!=NULL ) {
							hasGiven = TRUE;
							JSetStringT( hContact, "FirstName", m.getText() );
						}
						if (( m=n.getChild( "FAMILY" )) != NULL && m.getText()!=NULL ) {
							hasFamily = TRUE;
							JSetStringT( hContact, "LastName", m.getText() );
						}
						if (( m=n.getChild( "MIDDLE" )) != NULL && m.getText() != NULL ) {
							hasMiddle = TRUE;
							JSetStringT( hContact, "MiddleName", m.getText() );
					}	}
				}
				else if ( !lstrcmp( n.getName(), _T("EMAIL"))) {
					// E-mail address( es )
					if (( m=n.getChild( "USERID" )) == NULL )	// Some bad client put e-mail directly in <EMAIL/> instead of <USERID/>
						m = n;
					if ( m.getText() != NULL ) {
						char text[100];
						if ( hContact != NULL ) {
							if ( nEmail == 0 )
								strcpy( text, "e-mail" );
							else
								sprintf( text, "e-mail%d", nEmail-1 );
						}
						else sprintf( text, "e-mail%d", nEmail );
						JSetStringT( hContact, text, m.getText() );

						if ( hContact == NULL ) {
							sprintf( text, "e-mailFlag%d", nEmail );
							int nFlag = 0;
							if ( n.getChild( "HOME" ) != NULL ) nFlag |= JABBER_VCEMAIL_HOME;
							if ( n.getChild( "WORK" ) != NULL ) nFlag |= JABBER_VCEMAIL_WORK;
							if ( n.getChild( "INTERNET" ) != NULL ) nFlag |= JABBER_VCEMAIL_INTERNET;
							if ( n.getChild( "X400" ) != NULL ) nFlag |= JABBER_VCEMAIL_X400;
							JSetWord( NULL, text, nFlag );
						}
						nEmail++;
					}
				}
				else if ( !lstrcmp( n.getName(), _T("BDAY"))) {
					// Birthday
					if ( !hasBday && n.getText()!=NULL ) {
						if ( hContact != NULL ) {
							if ( _stscanf( n.getText(), _T("%d-%d-%d"), &nYear, &nMonth, &nDay ) == 3 ) {
								hasBday = TRUE;
								JSetWord( hContact, "BirthYear", ( WORD )nYear );
								JSetByte( hContact, "BirthMonth", ( BYTE ) nMonth );
								JSetByte( hContact, "BirthDay", ( BYTE ) nDay );
							}
						}
						else {
							hasBday = TRUE;
							JSetStringT( NULL, "BirthDate", n.getText() );
					}	}
				}
				else if ( !lstrcmp( n.getName(), _T("GENDER"))) {
					// Gender
					if ( !hasGender && n.getText()!=NULL ) {
						if ( hContact != NULL ) {
							if ( n.getText()[0] && strchr( "mMfF", n.getText()[0] )!=NULL ) {
								hasGender = TRUE;
								JSetByte( hContact, "Gender", ( BYTE ) toupper( n.getText()[0] ));
							}
						}
						else {
							hasGender = TRUE;
							JSetStringT( NULL, "GenderString", n.getText() );
					}	}
				}
				else if ( !lstrcmp( n.getName(), _T("ADR"))) {
					if ( !hasHome && n.getChild( "HOME" )!=NULL ) {
						// Home address
						hasHome = TRUE;
						if (( m=n.getChild( "STREET" )) != NULL && m.getText() != NULL ) {
							hasHomeStreet = TRUE;
							if ( hContact != NULL ) {
								if (( o=n.getChild( "EXTADR" )) != NULL && o.getText() != NULL )
									mir_sntprintf( text, SIZEOF( text ), _T("%s\r\n%s"), m.getText(), o.getText() );
								else if (( o=n.getChild( "EXTADD" ))!=NULL && o.getText()!=NULL )
									mir_sntprintf( text, SIZEOF( text ), _T("%s\r\n%s"), m.getText(), o.getText() );
								else
									_tcsncpy( text, m.getText(), SIZEOF( text ));
								text[SIZEOF(text)-1] = '\0';
								JSetStringT( hContact, "Street", text );
							}
							else {
								JSetStringT( hContact, "Street", m.getText() );
								if (( m=n.getChild( "EXTADR" )) == NULL )
									m = n.getChild( "EXTADD" );
								if ( m!=NULL && m.getText()!=NULL ) {
									hasHomeStreet2 = TRUE;
									JSetStringT( hContact, "Street2", m.getText() );
						}	}	}

						if (( m=n.getChild( "LOCALITY" ))!=NULL && m.getText()!=NULL ) {
							hasHomeLocality = TRUE;
							JSetStringT( hContact, "City", m.getText() );
						}
						if (( m=n.getChild( "REGION" ))!=NULL && m.getText()!=NULL ) {
							hasHomeRegion = TRUE;
							JSetStringT( hContact, "State", m.getText() );
						}
						if (( m=n.getChild( "PCODE" ))!=NULL && m.getText()!=NULL ) {
							hasHomePcode = TRUE;
							JSetStringT( hContact, "ZIP", m.getText() );
						}
						if (( m=n.getChild( "CTRY" ))==NULL || m.getText()==NULL )	// Some bad client use <COUNTRY/> instead of <CTRY/>
							m = n.getChild( "COUNTRY" );
						if ( m!=NULL && m.getText()!=NULL ) {
							hasHomeCtry = TRUE;
							if ( hContact != NULL )
								JSetWord( hContact, "Country", ( WORD )JabberCountryNameToId( m.getText() ));
							else
								JSetStringT( hContact, "CountryName", m.getText() );
					}	}

					if ( !hasWork && n.getChild( "WORK" )!=NULL ) {
						// Work address
						hasWork = TRUE;
						if (( m=n.getChild( "STREET" ))!=NULL && m.getText()!=NULL ) {
							hasWorkStreet = TRUE;
							if ( hContact != NULL ) {
								if (( o=n.getChild( "EXTADR" ))!=NULL && o.getText()!=NULL )
									mir_sntprintf( text, SIZEOF( text ), _T("%s\r\n%s"), m.getText(), o.getText() );
								else if (( o=n.getChild( "EXTADD" ))!=NULL && o.getText()!=NULL )
									mir_sntprintf( text, SIZEOF( text ), _T("%s\r\n%s"), m.getText(), o.getText() );
								else
									_tcsncpy( text, m.getText(), SIZEOF( text ));
								text[SIZEOF( text )-1] = '\0';
								JSetStringT( hContact, "CompanyStreet", text );
							}
							else {
								JSetStringT( hContact, "CompanyStreet", m.getText() );
								if (( m=n.getChild( "EXTADR" )) == NULL )
									m = n.getChild( "EXTADD" );
								if ( m!=NULL && m.getText()!=NULL ) {
									hasWorkStreet2 = TRUE;
									JSetStringT( hContact, "CompanyStreet2", m.getText() );
						}	}	}

						if (( m=n.getChild( "LOCALITY" ))!=NULL && m.getText()!=NULL ) {
							hasWorkLocality = TRUE;
							JSetStringT( hContact, "CompanyCity", m.getText() );
						}
						if (( m=n.getChild( "REGION" ))!=NULL && m.getText()!=NULL ) {
							hasWorkRegion = TRUE;
							JSetStringT( hContact, "CompanyState", m.getText() );
						}
						if (( m=n.getChild( "PCODE" ))!=NULL && m.getText()!=NULL ) {
							hasWorkPcode = TRUE;
							JSetStringT( hContact, "CompanyZIP", m.getText() );
						}
						if (( m=n.getChild( "CTRY" ))==NULL || m.getText()==NULL )	// Some bad client use <COUNTRY/> instead of <CTRY/>
							m = n.getChild( "COUNTRY" );
						if ( m!=NULL && m.getText()!=NULL ) {
							hasWorkCtry = TRUE;
							if ( hContact != NULL )
								JSetWord( hContact, "CompanyCountry", ( WORD )JabberCountryNameToId( m.getText() ));
							else
								JSetStringT( hContact, "CompanyCountryName", m.getText() );
					}	}
				}
				else if ( !lstrcmp( n.getName(), _T("TEL"))) {
					// Telephone/Fax/Cellular
					if (( m=n.getChild( "NUMBER" ))!=NULL && m.getText()!=NULL ) {
						if ( hContact != NULL ) {
							if ( !hasFax && n.getChild( "FAX" )!=NULL ) {
								hasFax = TRUE;
								JSetStringT( hContact, "Fax", m.getText() );
							}
							if ( !hasCell && n.getChild( "CELL" )!=NULL ) {
								hasCell = TRUE;
								JSetStringT( hContact, "Cellular", m.getText() );
							}
							if ( !hasPhone &&
								( n.getChild( "HOME" )!=NULL ||
								 n.getChild( "WORK" )!=NULL ||
								 n.getChild( "VOICE" )!=NULL ||
								 ( n.getChild( "FAX" )==NULL &&
								  n.getChild( "PAGER" )==NULL &&
								  n.getChild( "MSG" )==NULL &&
								  n.getChild( "CELL" )==NULL &&
								  n.getChild( "VIDEO" )==NULL &&
								  n.getChild( "BBS" )==NULL &&
								  n.getChild( "MODEM" )==NULL &&
								  n.getChild( "ISDN" )==NULL &&
								  n.getChild( "PCS" )==NULL )) ) {
								hasPhone = TRUE;
								JSetStringT( hContact, "Phone", m.getText() );
							}
						}
						else {
							char text[ 100 ];
							sprintf( text, "Phone%d", nPhone );
							JSetStringT( NULL, text, m.getText() );

							sprintf( text, "PhoneFlag%d", nPhone );
							int nFlag = 0;
							if ( n.getChild("HOME" ) != NULL ) nFlag |= JABBER_VCTEL_HOME;
							if ( n.getChild("WORK" ) != NULL ) nFlag |= JABBER_VCTEL_WORK;
							if ( n.getChild("VOICE" ) != NULL ) nFlag |= JABBER_VCTEL_VOICE;
							if ( n.getChild("FAX" ) != NULL ) nFlag |= JABBER_VCTEL_FAX;
							if ( n.getChild("PAGER" ) != NULL ) nFlag |= JABBER_VCTEL_PAGER;
							if ( n.getChild("MSG" ) != NULL ) nFlag |= JABBER_VCTEL_MSG;
							if ( n.getChild("CELL" ) != NULL ) nFlag |= JABBER_VCTEL_CELL;
							if ( n.getChild("VIDEO" ) != NULL ) nFlag |= JABBER_VCTEL_VIDEO;
							if ( n.getChild("BBS" ) != NULL ) nFlag |= JABBER_VCTEL_BBS;
							if ( n.getChild("MODEM" ) != NULL ) nFlag |= JABBER_VCTEL_MODEM;
							if ( n.getChild("ISDN" ) != NULL ) nFlag |= JABBER_VCTEL_ISDN;
							if ( n.getChild("PCS" ) != NULL ) nFlag |= JABBER_VCTEL_PCS;
							JSetWord( NULL, text, nFlag );
							nPhone++;
					}	}
				}
				else if ( !lstrcmp( n.getName(), _T("URL"))) {
					// Homepage
					if ( !hasUrl && n.getText()!=NULL ) {
						hasUrl = TRUE;
						JSetStringT( hContact, "Homepage", n.getText() );
					}
				}
				else if ( !lstrcmp( n.getName(), _T("ORG"))) {
					if ( !hasOrgname && !hasOrgunit ) {
						if (( m=n.getChild("ORGNAME" ))!=NULL && m.getText()!=NULL ) {
							hasOrgname = TRUE;
							JSetStringT( hContact, "Company", m.getText() );
						}
						if (( m=n.getChild("ORGUNIT" ))!=NULL && m.getText()!=NULL ) {	// The real vCard can have multiple <ORGUNIT/> but we will only display the first one
							hasOrgunit = TRUE;
							JSetStringT( hContact, "CompanyDepartment", m.getText() );
					}	}
				}
				else if ( !lstrcmp( n.getName(), _T("ROLE"))) {
					if ( !hasRole && n.getText()!=NULL ) {
						hasRole = TRUE;
						JSetStringT( hContact, "Role", n.getText() );
					}
				}
				else if ( !lstrcmp( n.getName(), _T("TITLE"))) {
					if ( !hasTitle && n.getText()!=NULL ) {
						hasTitle = TRUE;
						JSetStringT( hContact, "CompanyPosition", n.getText() );
					}
				}
				else if ( !lstrcmp( n.getName(), _T("DESC"))) {
					if ( !hasDesc && n.getText()!=NULL ) {
						hasDesc = TRUE;
						TCHAR* szMemo = JabberUnixToDosT( n.getText() );
						JSetStringT( hContact, "About", szMemo );
						mir_free( szMemo );
					}
				}
				else if ( !lstrcmp( n.getName(), _T("PHOTO")))
					OnIqResultGetVcardPhoto( jid, n, hContact, hasPhoto );
		}	}

		if ( !hasFn )
			JDeleteSetting( hContact, "FullName" );
		// We are not deleting "Nick"
//		if ( !hasNick )
//			JDeleteSetting( hContact, "Nick" );
		if ( !hasGiven )
			JDeleteSetting( hContact, "FirstName" );
		if ( !hasFamily )
			JDeleteSetting( hContact, "LastName" );
		if ( !hasMiddle )
			JDeleteSetting( hContact, "MiddleName" );
		if ( hContact != NULL ) {
			while ( true ) {
				if ( nEmail <= 0 )
					JDeleteSetting( hContact, "e-mail" );
				else {
					char text[ 100 ];
					sprintf( text, "e-mail%d", nEmail-1 );
					if ( DBGetContactSettingString( hContact, m_szModuleName, text, &dbv )) break;
					JFreeVariant( &dbv );
					JDeleteSetting( hContact, text );
				}
				nEmail++;
			}
		}
		else {
			while ( true ) {
				char text[ 100 ];
				sprintf( text, "e-mail%d", nEmail );
				if ( DBGetContactSettingString( NULL, m_szModuleName, text, &dbv )) break;
				JFreeVariant( &dbv );
				JDeleteSetting( NULL, text );
				sprintf( text, "e-mailFlag%d", nEmail );
				JDeleteSetting( NULL, text );
				nEmail++;
		}	}

		if ( !hasBday ) {
			JDeleteSetting( hContact, "BirthYear" );
			JDeleteSetting( hContact, "BirthMonth" );
			JDeleteSetting( hContact, "BirthDay" );
			JDeleteSetting( hContact, "BirthDate" );
		}
		if ( !hasGender ) {
			if ( hContact != NULL )
				JDeleteSetting( hContact, "Gender" );
			else
				JDeleteSetting( NULL, "GenderString" );
		}
		if ( hContact != NULL ) {
			if ( !hasPhone )
				JDeleteSetting( hContact, "Phone" );
			if ( !hasFax )
				JDeleteSetting( hContact, "Fax" );
			if ( !hasCell )
				JDeleteSetting( hContact, "Cellular" );
		}
		else {
			while ( true ) {
				char text[ 100 ];
				sprintf( text, "Phone%d", nPhone );
				if ( DBGetContactSettingString( NULL, m_szModuleName, text, &dbv )) break;
				JFreeVariant( &dbv );
				JDeleteSetting( NULL, text );
				sprintf( text, "PhoneFlag%d", nPhone );
				JDeleteSetting( NULL, text );
				nPhone++;
		}	}

		if ( !hasHomeStreet )
			JDeleteSetting( hContact, "Street" );
		if ( !hasHomeStreet2 && hContact==NULL )
			JDeleteSetting( hContact, "Street2" );
		if ( !hasHomeLocality )
			JDeleteSetting( hContact, "City" );
		if ( !hasHomeRegion )
			JDeleteSetting( hContact, "State" );
		if ( !hasHomePcode )
			JDeleteSetting( hContact, "ZIP" );
		if ( !hasHomeCtry ) {
			if ( hContact != NULL )
				JDeleteSetting( hContact, "Country" );
			else
				JDeleteSetting( hContact, "CountryName" );
		}
		if ( !hasWorkStreet )
			JDeleteSetting( hContact, "CompanyStreet" );
		if ( !hasWorkStreet2 && hContact==NULL )
			JDeleteSetting( hContact, "CompanyStreet2" );
		if ( !hasWorkLocality )
			JDeleteSetting( hContact, "CompanyCity" );
		if ( !hasWorkRegion )
			JDeleteSetting( hContact, "CompanyState" );
		if ( !hasWorkPcode )
			JDeleteSetting( hContact, "CompanyZIP" );
		if ( !hasWorkCtry ) {
			if ( hContact != NULL )
				JDeleteSetting( hContact, "CompanyCountry" );
			else
				JDeleteSetting( hContact, "CompanyCountryName" );
		}
		if ( !hasUrl )
			JDeleteSetting( hContact, "Homepage" );
		if ( !hasOrgname )
			JDeleteSetting( hContact, "Company" );
		if ( !hasOrgunit )
			JDeleteSetting( hContact, "CompanyDepartment" );
		if ( !hasRole )
			JDeleteSetting( hContact, "Role" );
		if ( !hasTitle )
			JDeleteSetting( hContact, "CompanyPosition" );
		if ( !hasDesc )
			JDeleteSetting( hContact, "About" );

		if ( id == m_ThreadInfo->resolveID ) {
			const TCHAR* p = _tcschr( jid, '@' );
			ResolveTransportNicks(( p != NULL ) ?  p+1 : jid );
		}
		else if ( hContact != NULL )
			JSendBroadcast( hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, ( HANDLE ) 1, 0 );
		else
			WindowNotify(WM_JABBER_REFRESH_VCARD);
	}
	else if ( !lstrcmp( type, _T("error"))) {
		if ( hContact != NULL )
			JSendBroadcast( hContact, ACKTYPE_GETINFO, ACKRESULT_FAILED, ( HANDLE ) 1, 0 );
}	}

void CJabberProto::OnIqResultSetVcard( XmlNode& iqNode, void *userdata )
{
	Log( "<iq/> iqIdSetVcard" );
	const TCHAR* type = iqNode.getAttrValue( _T("type"));
	if ( type == NULL )
		return;

	WindowNotify(WM_JABBER_REFRESH_VCARD);
}

void CJabberProto::OnIqResultSetSearch( XmlNode& iqNode, void *userdata )
{
	XmlNode queryNode, n;
	const TCHAR* type, *jid;
	int i, id;
	JABBER_SEARCH_RESULT jsr;

	Log( "<iq/> iqIdGetSearch" );
	if (( type = iqNode.getAttrValue( _T("type"))) == NULL ) return;
	if (( id = JabberGetPacketID( iqNode )) == -1 ) return;

	if ( !lstrcmp( type, _T("result"))) {
		if (( queryNode=iqNode.getChild( "query" )) == NULL ) return;
		jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
		for ( i=0; ; i++ ) {
			XmlNode itemNode = queryNode.getChild(i);
			if ( !itemNode )
				break;

			if ( !lstrcmp( itemNode.getName(), _T("item"))) {
				if (( jid=itemNode.getAttrValue( _T("jid"))) != NULL ) {
					_tcsncpy( jsr.jid, jid, SIZEOF( jsr.jid ));
					jsr.jid[ SIZEOF( jsr.jid )-1] = '\0';
					Log( "Result jid = " TCHAR_STR_PARAM, jid );
					if (( n=itemNode.getChild( "nick" ))!=NULL && n.getText()!=NULL )
						jsr.hdr.nick = mir_t2a( n.getText() );
					else
						jsr.hdr.nick = mir_strdup( "" );
					if (( n=itemNode.getChild( "first" ))!=NULL && n.getText()!=NULL )
						jsr.hdr.firstName = mir_t2a( n.getText() );
					else
						jsr.hdr.firstName = mir_strdup( "" );
					if (( n=itemNode.getChild( "last" ))!=NULL && n.getText()!=NULL )
						jsr.hdr.lastName = mir_t2a( n.getText() );
					else
						jsr.hdr.lastName = mir_strdup( "" );
					if (( n=itemNode.getChild( "email" ))!=NULL && n.getText()!=NULL )
						jsr.hdr.email = mir_t2a( n.getText() );
					else
						jsr.hdr.email = mir_strdup( "" );
					JSendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, ( HANDLE ) id, ( LPARAM )&jsr );
					mir_free( jsr.hdr.nick );
					mir_free( jsr.hdr.firstName );
					mir_free( jsr.hdr.lastName );
					mir_free( jsr.hdr.email );
		}	}	}

		JSendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, ( HANDLE ) id, 0 );
	}
	else if ( !lstrcmp( type, _T("error")))
		JSendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, ( HANDLE ) id, 0 );
}

void CJabberProto::OnIqResultExtSearch( XmlNode& iqNode, void *userdata )
{
	XmlNode queryNode;
	const TCHAR* type;
	int id;

	Log( "<iq/> iqIdGetExtSearch" );
	if (( type=iqNode.getAttrValue( _T("type"))) == NULL ) return;
	if (( id = JabberGetPacketID( iqNode )) == -1 ) return;

	if ( !lstrcmp( type, _T("result"))) {
		if (( queryNode=iqNode.getChild( "query" )) == NULL ) return;
		if (( queryNode=queryNode.getChild( "x" )) == NULL ) return;
		for ( int i=0; ; i++ ) {
			XmlNode itemNode = queryNode.getChild(i);
			if ( !itemNode )
				break;
			if ( lstrcmp( itemNode.getName(), _T("item")))
				continue;

			JABBER_SEARCH_RESULT jsr = { 0 };
			jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
//			jsr.hdr.firstName = "";

			for ( int j=0; ; j++ ) {
				XmlNode fieldNode = itemNode.getChild(j);
				if ( !fieldNode )
					break;

				if ( lstrcmp( fieldNode.getName(), _T("field")))
					continue;

				const TCHAR* fieldName = fieldNode.getAttrValue( _T("var"));
				if ( fieldName == NULL )
					continue;

				XmlNode n = fieldNode.getChild( "value" );
				if ( n == NULL )
					continue;

				if ( !lstrcmp( fieldName, _T("jid"))) {
					_tcsncpy( jsr.jid, n.getText(), SIZEOF( jsr.jid ));
					jsr.jid[SIZEOF( jsr.jid )-1] = '\0';
					Log( "Result jid = " TCHAR_STR_PARAM, jsr.jid );
				}
				else if ( !lstrcmp( fieldName, _T("nickname")))
					jsr.hdr.nick = ( n.getText() != NULL ) ? mir_t2a( n.getText() ) : mir_strdup( "" );
				else if ( !lstrcmp( fieldName, _T("fn"))) {
					mir_free( jsr.hdr.firstName );
					jsr.hdr.firstName = ( n.getText() != NULL ) ? mir_t2a(n.getText()) : mir_strdup( "" );
				}
				else if ( !lstrcmp( fieldName, _T("given"))) {
					mir_free( jsr.hdr.firstName );
					jsr.hdr.firstName = ( n.getText() != NULL ) ? mir_t2a(n.getText()) : mir_strdup( "" );
				}
				else if ( !lstrcmp( fieldName, _T("family")))
					jsr.hdr.lastName = ( n.getText() != NULL ) ? mir_t2a(n.getText()) : mir_strdup( "" );
				else if ( !lstrcmp( fieldName, _T("email")))
					jsr.hdr.email = ( n.getText() != NULL ) ? mir_t2a(n.getText()) : mir_strdup( "" );
			}

			JSendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, ( HANDLE ) id, ( LPARAM )&jsr );
			mir_free( jsr.hdr.nick );
			mir_free( jsr.hdr.firstName );
			mir_free( jsr.hdr.lastName );
			mir_free( jsr.hdr.email );
		}

		JSendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, ( HANDLE ) id, 0 );
	}
	else if ( !lstrcmp( type, _T("error")))
		JSendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, ( HANDLE ) id, 0 );
}

void CJabberProto::OnIqResultSetPassword( XmlNode& iqNode, void *userdata )
{
	Log( "<iq/> iqIdSetPassword" );

	const TCHAR* type = iqNode.getAttrValue( _T("type"));
	if ( type == NULL )
		return;

	if ( !lstrcmp( type, _T("result"))) {
		strncpy( m_ThreadInfo->password, m_ThreadInfo->newPassword, SIZEOF( m_ThreadInfo->password ));
		MessageBox( NULL, TranslateT( "Password is successfully changed. Don't forget to update your password in the Jabber protocol option." ), TranslateT( "Change Password" ), MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND );
	}
	else if ( !lstrcmp( type, _T("error")))
		MessageBox( NULL, TranslateT( "Password cannot be changed." ), TranslateT( "Change Password" ), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND );
}
/*
void CJabberProto::OnIqResultDiscoAgentItems( XmlNode& iqNode, void *userdata )
{
	if ( !JGetByte( "EnableAvatars", TRUE ))
		return;
}
*/
void CJabberProto::OnIqResultGetAvatar( XmlNode& iqNode, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;
	const TCHAR* type;

	// RECVED: agent list
	// ACTION: refresh agent list dialog
	Log( "<iq/> iqIdResultGetAvatar" );
	if (( type = iqNode.getAttrValue( _T("type"))) == NULL ) return;
	if ( _tcscmp( type, _T("result")))                       return;

	const TCHAR* from = iqNode.getAttrValue( _T("from"));
	if ( from == NULL )
		return;
	HANDLE hContact = HContactFromJID( from );
	if ( hContact == NULL )
		return;
	XmlNode n;
	const TCHAR* mimeType = NULL;
	if ( JGetByte( hContact, "AvatarXVcard", 0 )) {
		XmlNode vCard = iqNode.getChild( "vCard" );
		if (vCard == NULL) return;
		vCard = vCard.getChild( "PHOTO" );
		if (vCard == NULL) return;
		XmlNode typeNode = vCard.getChild( "TYPE" );
		if (typeNode != NULL) mimeType = typeNode.getText();
		n = vCard.getChild( "BINVAL" );
	}
	else {
		XmlNode queryNode = iqNode.getChild( "query" );
		if ( queryNode == NULL )
			return;

		const TCHAR* xmlns = queryNode.getAttrValue( _T("xmlns"));
		if ( lstrcmp( xmlns, _T(JABBER_FEAT_AVATAR)))
			return;

		n = queryNode.getChild( "data" );
		if ( n )
			mimeType = n.getAttrValue( _T("mimetype"));
	}
	if ( n == NULL )
		return;

	int resultLen = 0;
	char* body = JabberBase64Decode( n.getText(), &resultLen );

	int pictureType;
	if ( mimeType != NULL ) {
		if ( !lstrcmp( mimeType, _T("image/jpeg")))     pictureType = PA_FORMAT_JPEG;
		else if ( !lstrcmp( mimeType, _T("image/png"))) pictureType = PA_FORMAT_PNG;
		else if ( !lstrcmp( mimeType, _T("image/gif"))) pictureType = PA_FORMAT_GIF;
		else if ( !lstrcmp( mimeType, _T("image/bmp"))) pictureType = PA_FORMAT_BMP;
		else {
LBL_ErrFormat:
			Log( "Invalid mime type specified for picture: " TCHAR_STR_PARAM, mimeType );
			mir_free( body );
			return;
	}	}
	else if (( pictureType = JabberGetPictureType( body )) == PA_FORMAT_UNKNOWN )
		goto LBL_ErrFormat;

	PROTO_AVATAR_INFORMATION AI;
	AI.cbSize = sizeof AI;
	AI.format = pictureType;
	AI.hContact = hContact;

	if ( JGetByte( hContact, "AvatarType", PA_FORMAT_UNKNOWN ) != (unsigned char)pictureType ) {
		GetAvatarFileName( hContact, AI.filename, sizeof AI.filename );
		DeleteFileA( AI.filename );
	}

	JSetByte( hContact, "AvatarType", pictureType );

	char buffer[ 41 ];
	mir_sha1_byte_t digest[20];
	mir_sha1_ctx sha;
	mir_sha1_init( &sha );
	mir_sha1_append( &sha, ( mir_sha1_byte_t* )body, resultLen );
	mir_sha1_finish( &sha, digest );
	for ( int i=0; i<20; i++ )
		sprintf( buffer+( i<<1 ), "%02x", digest[i] );
	JSetString( hContact, "AvatarSaved", buffer );

	GetAvatarFileName( hContact, AI.filename, sizeof AI.filename );

	FILE* out = fopen( AI.filename, "wb" );
	if ( out != NULL ) {
		fwrite( body, resultLen, 1, out );
		fclose( out );
		JSendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, HANDLE( &AI ), NULL );
		Log("Broadcast new avatar: %s",AI.filename);
	}
	else JSendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, HANDLE( &AI ), NULL );

	mir_free( body );
}

/////////////////////////////////////////////////////////////////////////////////////////
// Bookmarks

void CJabberProto::OnIqResultDiscoBookmarks( XmlNode& iqNode, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;
	XmlNode queryNode, itemNode, storageNode, nickNode, passNode;
	const TCHAR* type, *jid;

	// RECVED: list of bookmarks
	// ACTION: refresh bookmarks dialog
	Log( "<iq/> iqIdGetBookmarks" );
	if (( type = iqNode.getAttrValue( _T("type"))) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		if ( m_ThreadInfo && !( m_ThreadInfo->jabberServerCaps & JABBER_CAPS_PRIVATE_STORAGE )) {
			m_ThreadInfo->jabberServerCaps |= JABBER_CAPS_PRIVATE_STORAGE;
			EnableMenuItems( TRUE );
		}

		if (( queryNode = iqNode.getChild( "query" )) != NULL ) {
			if ((storageNode = queryNode.getChild( "storage" )) != NULL) {
				ListRemoveList( LIST_BOOKMARK );
				for ( int i=0; ; i++ ) {
					XmlNode itemNode = storageNode.getChild(i);
					if ( !itemNode )
						break;

					if ( itemNode.getName() != NULL) {
						if ( !lstrcmp( itemNode.getName(), _T("conference"))) {
							if (( jid = itemNode.getAttrValue( _T("jid"))) != NULL ) {
								JABBER_LIST_ITEM* item = ListAdd( LIST_BOOKMARK, jid );
								item->name = mir_tstrdup( itemNode.getAttrValue( _T("name")));
								item->type = mir_tstrdup( _T( "conference" ));
								item->bUseResource = TRUE;
								
								const TCHAR* autoJ = itemNode.getAttrValue( _T("autojoin"));
								if ( autoJ != NULL )
									item->bAutoJoin = ( !lstrcmp( autoJ, _T("true")) || !lstrcmp( autoJ, _T("1"))) ? true : false;

								if (( nickNode = itemNode.getChild( "nick" )) != NULL && nickNode.getText() != NULL )
									replaceStr( item->nick, nickNode.getText() );
								if (( passNode = itemNode.getChild( "password" )) != NULL && passNode.getText() != NULL )
									replaceStr( item->password, passNode.getText() );
						}	}

						if ( !lstrcmp( itemNode.getName(), _T("url"))) {
							if (( jid = itemNode.getAttrValue( _T("url" ))) != NULL ) {
								JABBER_LIST_ITEM* item = ListAdd( LIST_BOOKMARK, jid );
								item->bUseResource = TRUE;
								item->name = mir_tstrdup( itemNode.getAttrValue( _T("name")));
								item->type = mir_tstrdup( _T("url") );
			}	}	}	}	}

			UI_SAFE_NOTIFY(m_pDlgBookmarks, WM_JABBER_REFRESH);
			info->bBookmarksLoaded = TRUE;
			OnProcessLoginRq(info, JABBER_LOGIN_BOOKMARKS);
		}
	}
	else if ( !lstrcmp( type, _T("error"))) {
		if ( info->jabberServerCaps & JABBER_CAPS_PRIVATE_STORAGE ) {
			info->jabberServerCaps &= ~JABBER_CAPS_PRIVATE_STORAGE;
			EnableMenuItems( TRUE );
			UI_SAFE_NOTIFY(m_pDlgBookmarks, WM_JABBER_ACTIVATE);
			return;
		}
}	}

void CJabberProto::SetBookmarkRequest (XmlNodeIq& iq)
{
	XmlNode query = iq.addQuery( _T(JABBER_FEAT_PRIVATE_STORAGE));
	XmlNode storage = query.addChild( "storage" );
	storage.addAttr( "xmlns", "storage:bookmarks" );

	for ( int i=0; ( i=ListFindNext( LIST_BOOKMARK, i )) >= 0; i++ ) {
		JABBER_LIST_ITEM* item = ListGetItemPtrFromIndex( i );
		if ( item == NULL )
			continue;

		if ( item->jid == NULL )
			continue;
		if (!lstrcmp( item->type, _T("conference") )) {
			XmlNode itemNode = storage.addChild("conference");
			itemNode.addAttr( "jid", item->jid );
			if ( item->name )
				itemNode.addAttr( "name", item->name );
			if ( item->bAutoJoin )
				itemNode.addAttr( "autojoin", _T("1") );
			if ( item->nick )
				itemNode.addChild( "nick", item->nick );
			if ( item->password )
				itemNode.addChild( "password", item->password );
		}
		if (!lstrcmp( item->type, _T("url") )) {
			XmlNode itemNode = storage.addChild("url");
			itemNode.addAttr( "url", item->jid );
			if ( item->name )
				itemNode.addAttr( "name", item->name );
		}
	}
}

void CJabberProto::OnIqResultSetBookmarks( XmlNode& iqNode, void *userdata )
{
	// RECVED: server's response
	// ACTION: refresh bookmarks list dialog

	Log( "<iq/> iqIdSetBookmarks" );

	const TCHAR* type = iqNode.getAttrValue( _T("type"));
	if ( type == NULL )
		return;

	if ( !lstrcmp( type, _T("result"))) {
		UI_SAFE_NOTIFY(m_pDlgBookmarks, WM_JABBER_REFRESH);
	}
	else if ( !lstrcmp( type, _T("error"))) {
		XmlNode errorNode = iqNode.getChild( "error" );
		TCHAR* str = JabberErrorMsg( errorNode );
		MessageBox( NULL, str, TranslateT( "Jabber Bookmarks Error" ), MB_OK|MB_SETFOREGROUND );
		mir_free( str );
		UI_SAFE_NOTIFY(m_pDlgBookmarks, WM_JABBER_ACTIVATE);
}	}

// last activity (XEP-0012) support
void CJabberProto::OnIqResultLastActivity( XmlNode& iqNode, void *userdata, CJabberIqInfo* pInfo )
{
	JABBER_RESOURCE_STATUS *r = ResourceInfoFromJID( pInfo->m_szFrom );
	if ( !r )
		return;

	time_t lastActivity = -1;
	if ( pInfo->m_nIqType == JABBER_IQ_TYPE_RESULT ) {
		XmlNode queryNode = iqNode.getChild( "query" );
		if ( queryNode ) {
			const TCHAR *seconds = queryNode.getAttrValue( _T("seconds"));
			if ( seconds ) {
				int nSeconds = _ttoi( seconds );
				if ( nSeconds > 0 )
					lastActivity = time( 0 ) - nSeconds;
			}
			if ( queryNode.getText() && *queryNode.getText() )
				replaceStr( r->statusMessage, queryNode.getText() );
		}
	}

	r->idleStartTime = lastActivity;

	JabberUserInfoUpdate(pInfo->GetHContact());
}

// entity time (XEP-0202) support
void CJabberProto::OnIqResultEntityTime( XmlNode& pIqNode, void* pUserdata, CJabberIqInfo* pInfo )
{
	if ( !pInfo->m_hContact )
		return;

	if ( pInfo->m_nIqType == JABBER_IQ_TYPE_RESULT ) {
		XmlNode pTimeNode = pIqNode.getChildByTag( "time", "xmlns", _T( JABBER_FEAT_ENTITY_TIME ));
		if ( pTimeNode ) {
			XmlNode pTzo = pTimeNode.getChild( "tzo");
			if ( pTzo ) {
				if ( _tcslen( pTzo.getText() ) == 6 ) { // +00:00
					bool bNegative = pTzo.getText()[0] == _T('-');
					int nTz = ( _ttoi( pTzo.getText()+1 ) * 60 + _ttoi( pTzo.getText() + 4 )) / 30;

					TIME_ZONE_INFORMATION tzinfo;
					if ( GetTimeZoneInformation( &tzinfo ) == TIME_ZONE_ID_DAYLIGHT )
						nTz += ( bNegative ? -tzinfo.DaylightBias : tzinfo.DaylightBias ) / 30;

					if ( !bNegative )
						nTz = 256 - nTz;
					JSetByte( pInfo->m_hContact, "Timezone", nTz );
					return;
	}	}	}	}

	JDeleteSetting( pInfo->m_hContact, "Timezone" );
	return;
}
