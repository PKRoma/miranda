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
#include "jabber_iq.h"
#include "jabber_caps.h"
#include "jabber_privacy.h"

#include "m_genmenu.h"
#include "m_clistint.h"

void CJabberProto::OnIqResultServerDiscoInfo( HXML iqNode )
{
	if ( !iqNode )
		return;

	const TCHAR *type = xmlGetAttrValue( iqNode, _T("type"));
	int i;

	if ( !_tcscmp( type, _T("result"))) {
		HXML query = xmlGetChildByTag( iqNode, "query", "xmlns", _T(JABBER_FEAT_DISCO_INFO) );
		if ( !query )
			return;

		HXML identity;
		for ( i = 1; ( identity = xmlGetNthChild( query, _T("identity"), i )) != NULL; i++ ) {
			const TCHAR *identityCategory = xmlGetAttrValue( identity, _T("category"));
			const TCHAR *identityType = xmlGetAttrValue( identity, _T("type"));
			if ( identityCategory && identityType && !_tcscmp( identityCategory, _T("pubsub") ) && !_tcscmp( identityType, _T("pep")) ) {
				m_bPepSupported = TRUE;

				EnableMenuItems( TRUE );
				RebuildInfoFrame();
				break;
			}
		}
		if ( m_ThreadInfo ) {
			HXML feature;
			for ( i = 1; ( feature = xmlGetNthChild( query, _T("feature"), i )) != NULL; i++ ) {
				const TCHAR *featureName = xmlGetAttrValue( feature, _T("var"));
				if ( featureName ) {
					for ( int j = 0; g_JabberFeatCapPairs[j].szFeature; j++ ) {
						if ( !_tcscmp( g_JabberFeatCapPairs[j].szFeature, featureName )) {
							m_ThreadInfo->jabberServerCaps |= g_JabberFeatCapPairs[j].jcbCap;
							break;
		}	}	}	}	}

		OnProcessLoginRq( m_ThreadInfo, JABBER_LOGIN_SERVERINFO);
}	}

void CJabberProto::OnIqResultNestedRosterGroups( HXML iqNode, CJabberIqInfo* pInfo )
{
	const TCHAR *szGroupDelimeter = NULL;
	BOOL bPrivateStorageSupport = FALSE;

	if ( iqNode && pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT ) {
		bPrivateStorageSupport = TRUE;
		szGroupDelimeter = XPathFmt( iqNode, _T("query[@xmlns='%s']/roster[@xmlns='%s']"), _T(JABBER_FEAT_PRIVATE_STORAGE), _T( JABBER_FEAT_NESTED_ROSTER_GROUPS ) );
		if ( szGroupDelimeter && !szGroupDelimeter[0] )
			szGroupDelimeter = NULL; // "" as roster delimeter is not supported :)
	}

	// global fuckup
	if ( !m_ThreadInfo )
		return;

	// is our default delimiter?
	if (( !szGroupDelimeter && bPrivateStorageSupport ) || ( szGroupDelimeter && _tcscmp( szGroupDelimeter, _T("\\") )))
		m_ThreadInfo->send(
			XmlNodeIq( _T("set"), SerialNext()) << XQUERY( _T(JABBER_FEAT_PRIVATE_STORAGE))
				<< XCHILD( _T("roster"), _T("\\")) << XATTR( _T("xmlns"), _T(JABBER_FEAT_NESTED_ROSTER_GROUPS)));

	// roster request
	TCHAR *szUserData = mir_tstrdup( szGroupDelimeter ? szGroupDelimeter : _T("\\") );
	m_ThreadInfo->send(
		XmlNodeIq( m_iqManager.AddHandler( &CJabberProto::OnIqResultGetRoster, JABBER_IQ_TYPE_GET, NULL, 0, -1, (void *)szUserData ))
			<< XCHILDNS( _T("query"), _T(JABBER_FEAT_IQ_ROSTER)));
}

void CJabberProto::OnIqResultNotes( HXML iqNode, CJabberIqInfo* pInfo )
{
	if ( iqNode && pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT ) {
		HXML hXmlData = XPathFmt( iqNode, _T("query[@xmlns='%s']/storage[@xmlns='%s']"),
			_T(JABBER_FEAT_PRIVATE_STORAGE), _T(JABBER_FEAT_MIRANDA_NOTES));
		if (hXmlData) m_notes.LoadXml(hXmlData);
	}
}

void CJabberProto::OnProcessLoginRq( ThreadData* info, DWORD rq )
{
	if ( info == NULL )
		return;

	info->dwLoginRqs |= rq;

	if ((info->dwLoginRqs & JABBER_LOGIN_ROSTER) && (info->dwLoginRqs & JABBER_LOGIN_BOOKMARKS) &&
		(info->dwLoginRqs & JABBER_LOGIN_SERVERINFO) && !(info->dwLoginRqs & JABBER_LOGIN_BOOKMARKS_AJ))
	{
		if ( jabberChatDllPresent && m_options.AutoJoinBookmarks) {
			JABBER_LIST_ITEM* item;
			LISTFOREACH(i, this, LIST_BOOKMARK)
			{
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

void CJabberProto::OnLoggedIn()
{
	m_bJabberOnline = TRUE;
	m_tmJabberLoggedInTime = time(0);

	m_ThreadInfo->dwLoginRqs = 0;

	// XEP-0083 support
	{
		CJabberIqInfo* pIqInfo = m_iqManager.AddHandler( &CJabberProto::OnIqResultNestedRosterGroups, JABBER_IQ_TYPE_GET );
		// ugly hack to prevent hangup during login process
		pIqInfo->SetTimeout( 30000 );
		m_ThreadInfo->send(
			XmlNodeIq( pIqInfo ) << XQUERY( _T(JABBER_FEAT_PRIVATE_STORAGE))
				<< XCHILDNS( _T("roster"), _T(JABBER_FEAT_NESTED_ROSTER_GROUPS)));
	}

	// Server-side notes
	{
		m_ThreadInfo->send(
			XmlNodeIq(m_iqManager.AddHandler(&CJabberProto::OnIqResultNotes, JABBER_IQ_TYPE_GET))
				<< XQUERY( _T(JABBER_FEAT_PRIVATE_STORAGE))
				<< XCHILDNS( _T("storage"), _T(JABBER_FEAT_MIRANDA_NOTES)));
	}

	int iqId = SerialNext();
	IqAdd( iqId, IQ_PROC_DISCOBOOKMARKS, &CJabberProto::OnIqResultDiscoBookmarks);
	m_ThreadInfo->send(
		XmlNodeIq( _T("get"), iqId) << XQUERY( _T(JABBER_FEAT_PRIVATE_STORAGE))
			<< XCHILDNS( _T("storage"), _T("storage:bookmarks")));

	m_bPepSupported = FALSE;
	m_ThreadInfo->jabberServerCaps = JABBER_RESOURCE_CAPS_NONE;
	iqId = SerialNext();
	IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultServerDiscoInfo );
	m_ThreadInfo->send( XmlNodeIq( _T("get"), iqId, _A2T(m_ThreadInfo->server)) << XQUERY( _T(JABBER_FEAT_DISCO_INFO)));

	QueryPrivacyLists( m_ThreadInfo );

	char szServerName[ sizeof(m_ThreadInfo->server) ];
	if ( JGetStaticString( "LastLoggedServer", NULL, szServerName, sizeof(szServerName)))
		SendGetVcard( m_szJabberJID );
	else if ( strcmp( m_ThreadInfo->server, szServerName ))
		SendGetVcard( m_szJabberJID );
	JSetString( NULL, "LastLoggedServer", m_ThreadInfo->server );

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

	m_pepServices.ResetPublishAll();
}

void CJabberProto::OnIqResultGetAuth( HXML iqNode )
{
	// RECVED: result of the request for authentication method
	// ACTION: send account authentication information to log in
	Log( "<iq/> iqIdGetAuth" );

	HXML queryNode;
	const TCHAR* type;
	if (( type=xmlGetAttrValue( iqNode, _T("type"))) == NULL ) return;
	if (( queryNode=xmlGetChild( iqNode , "query" )) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		int iqId = SerialNext();
		IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultSetAuth );

		XmlNodeIq iq( _T("set"), iqId );
		HXML query = iq << XQUERY( _T("jabber:iq:auth"));
		query << XCHILD( _T("username"), m_ThreadInfo->username );
		if ( xmlGetChild( queryNode, "digest" ) != NULL && m_szStreamId ) {
			char* str = mir_utf8encode( m_ThreadInfo->password );
			char text[200];
			mir_snprintf( text, SIZEOF(text), "%s%s", m_szStreamId, str );
			mir_free( str );
         if (( str=JabberSha1( text )) != NULL ) {
				query << XCHILD( _T("digest"), _A2T(str));
				mir_free( str );
			}
		}
		else if ( xmlGetChild( queryNode, "password" ) != NULL )
			query << XCHILD( _T("password"), _A2T(m_ThreadInfo->password));
		else {
			Log( "No known authentication mechanism accepted by the server." );

			m_ThreadInfo->send( "</stream:stream>" );
			return;
		}

		if ( xmlGetChild( queryNode , "resource" ) != NULL )
			query << XCHILD( _T("resource"), m_ThreadInfo->resource );

		m_ThreadInfo->send( iq );
	}
	else if ( !lstrcmp( type, _T("error"))) {
 		m_ThreadInfo->send( "</stream:stream>" );

		TCHAR text[128];
		mir_sntprintf( text, SIZEOF( text ), _T("%s %s."), TranslateT( "Authentication failed for" ), m_ThreadInfo->username );
		MessageBox( NULL, text, TranslateT( "Jabber Authentication" ), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND );
		JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD );
		m_ThreadInfo = NULL;	// To disallow auto reconnect
}	}

void CJabberProto::OnIqResultSetAuth( HXML iqNode )
{
	const TCHAR* type;

	// RECVED: authentication result
	// ACTION: if successfully logged in, continue by requesting roster list and set my initial status
	Log( "<iq/> iqIdSetAuth" );
	if (( type=xmlGetAttrValue( iqNode, _T("type"))) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		DBVARIANT dbv;
		if ( JGetStringT( NULL, "Nick", &dbv ))
			JSetStringT( NULL, "Nick", m_ThreadInfo->username );
		else
			JFreeVariant( &dbv );

		OnLoggedIn();
	}
	// What to do if password error? etc...
	else if ( !lstrcmp( type, _T("error"))) {
		TCHAR text[128];

		m_ThreadInfo->send( "</stream:stream>" );
		mir_sntprintf( text, SIZEOF( text ), _T("%s %s."), TranslateT( "Authentication failed for" ), m_ThreadInfo->username );
		MessageBox( NULL, text, TranslateT( "Jabber Authentication" ), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND );
		JSendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD );
		m_ThreadInfo = NULL;	// To disallow auto reconnect
}	}

void CJabberProto::OnIqResultBind( HXML iqNode, CJabberIqInfo* pInfo )
{
	if ( !m_ThreadInfo || !iqNode )
		return;
	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT) {
		LPCTSTR szJid = XPathT( iqNode, "bind[@xmlns='urn:ietf:params:xml:ns:xmpp-bind']/jid" );
		if ( szJid ) {
			if ( !_tcsncmp( m_ThreadInfo->fullJID, szJid, SIZEOF( m_ThreadInfo->fullJID )))
				Log( "Result Bind: " TCHAR_STR_PARAM " confirmed ", m_ThreadInfo->fullJID );
			else {
				Log( "Result Bind: " TCHAR_STR_PARAM " changed to " TCHAR_STR_PARAM, m_ThreadInfo->fullJID, szJid);
				_tcsncpy( m_ThreadInfo->fullJID, szJid, SIZEOF( m_ThreadInfo->fullJID ));
			}
		}
		if ( m_ThreadInfo->bIsSessionAvailable )
			m_ThreadInfo->send(
				XmlNodeIq( m_iqManager.AddHandler( &CJabberProto::OnIqResultSession, JABBER_IQ_TYPE_SET ))
				<< XCHILDNS( _T("session"), _T("urn:ietf:params:xml:ns:xmpp-session" )));
		else
			OnLoggedIn();
	}
	else {
		//rfc3920 page 39
		m_ThreadInfo->send( "</stream:stream>" );
		m_ThreadInfo = NULL;	// To disallow auto reconnect
	}
}

void CJabberProto::OnIqResultSession( HXML iqNode, CJabberIqInfo* pInfo )
{
	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT)
		OnLoggedIn();
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
	if( !server ) {
		mir_free( roomjid );
		return;
	}
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

void CJabberProto::OnIqResultGetRoster( HXML iqNode, CJabberIqInfo* pInfo )
{
	Log( "<iq/> iqIdGetRoster" );
	TCHAR *szGroupDelimeter = (TCHAR *)pInfo->GetUserData();
	if ( pInfo->GetIqType() != JABBER_IQ_TYPE_RESULT ) {
		mir_free( szGroupDelimeter );
		return;
	}

	HXML queryNode = xmlGetChild( iqNode , "query" );
	if ( queryNode == NULL ) {
		mir_free( szGroupDelimeter );
		return;
	}

	if ( lstrcmp( xmlGetAttrValue( queryNode, _T("xmlns")), _T(JABBER_FEAT_IQ_ROSTER))) {
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

		HXML itemNode = xmlGetChild( queryNode ,i);
		if ( !itemNode )
			break;

		if ( _tcscmp( xmlGetName( itemNode ), _T("item")))
			continue;

		const TCHAR* str = xmlGetAttrValue( itemNode, _T("subscription")), *name;

		JABBER_SUBSCRIPTION sub;
		if ( str == NULL ) sub = SUB_NONE;
		else if ( !_tcscmp( str, _T("both"))) sub = SUB_BOTH;
		else if ( !_tcscmp( str, _T("to"))) sub = SUB_TO;
		else if ( !_tcscmp( str, _T("from"))) sub = SUB_FROM;
		else sub = SUB_NONE;

		const TCHAR* jid = xmlGetAttrValue( itemNode, _T("jid"));
		if ( jid == NULL )
			continue;
		if ( _tcschr( jid, '@' ) == NULL )
			bIsTransport = TRUE;

		if (( name = xmlGetAttrValue( itemNode, _T("name") )) != NULL )
			nick = mir_tstrdup( name );
		else
			nick = JabberNickFromJID( jid );

		if ( nick == NULL )
			continue;

		JABBER_LIST_ITEM* item = ListAdd( LIST_ROSTER, jid );
		item->subscription = sub;

		mir_free( item->nick ); item->nick = nick;

		HXML groupNode = xmlGetChild( itemNode , "group" );
		replaceStr( item->group, ( groupNode ) ? xmlGetText( groupNode ) : NULL );

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
		} else
		{
			UpdateSubscriptionInfo(hContact, item);
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
	if ( m_options.RosterSync == TRUE ) {
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

	if ( m_options.AutoJoinConferences ) {
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

	OnProcessLoginRq(m_ThreadInfo, JABBER_LOGIN_ROSTER);
	RebuildInfoFrame();
}

void CJabberProto::OnIqResultGetRegister( HXML iqNode )
{
	// RECVED: result of the request for ( agent ) registration mechanism
	// ACTION: activate ( agent ) registration input dialog
	Log( "<iq/> iqIdGetRegister" );

	HXML queryNode;
	const TCHAR *type;
	if (( type = xmlGetAttrValue( iqNode, _T("type"))) == NULL ) return;
	if (( queryNode = xmlGetChild( iqNode , "query" )) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		if ( m_hwndAgentRegInput )
			SendMessage( m_hwndAgentRegInput, WM_JABBER_REGINPUT_ACTIVATE, 1 /*success*/, ( LPARAM )xi.copyNode( iqNode ));
	}
	else if ( !lstrcmp( type, _T("error"))) {
		if ( m_hwndAgentRegInput ) {
			HXML errorNode = xmlGetChild( iqNode , "error" );
			TCHAR* str = JabberErrorMsg( errorNode );
			SendMessage( m_hwndAgentRegInput, WM_JABBER_REGINPUT_ACTIVATE, 0 /*error*/, ( LPARAM )str );
			mir_free( str );
}	}	}

void CJabberProto::OnIqResultSetRegister( HXML iqNode )
{
	// RECVED: result of registration process
	// ACTION: notify of successful agent registration
	Log( "<iq/> iqIdSetRegister" );

	const TCHAR *type, *from;
	if (( type = xmlGetAttrValue( iqNode, _T("type"))) == NULL ) return;
	if (( from = xmlGetAttrValue( iqNode, _T("from"))) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		HANDLE hContact = HContactFromJID( from );
		if ( hContact != NULL )
			JSetByte( hContact, "IsTransport", TRUE );

		if ( m_hwndRegProgress )
			SendMessage( m_hwndRegProgress, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )TranslateT( "Registration successful" ));
	}
	else if ( !lstrcmp( type, _T("error"))) {
		if ( m_hwndRegProgress ) {
			HXML errorNode = xmlGetChild( iqNode , "error" );
			TCHAR* str = JabberErrorMsg( errorNode );
			SendMessage( m_hwndRegProgress, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )str );
			mir_free( str );
}	}	}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberIqResultGetVcard - processes the server-side v-card

void CJabberProto::OnIqResultGetVcardPhoto( const TCHAR* jid, HXML n, HANDLE hContact, BOOL& hasPhoto )
{
	Log( "JabberIqResultGetVcardPhoto: %d", hasPhoto );
	if ( hasPhoto )
		return;

	HXML o = xmlGetChild( n , "BINVAL" );
	if ( o == NULL || xmlGetText( o ) == NULL )
		return;

	int bufferLen;
	char* buffer = JabberBase64Decode( xmlGetText( o ), &bufferLen );
	if ( buffer == NULL )
		return;

	char* szPicType;
	HXML m = xmlGetChild( n , "TYPE" );
	if ( m == NULL || xmlGetText( m ) == NULL ) {
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
		if ( !_tcscmp( xmlGetText( m ), _T("image/jpeg")))
			szPicType = "image/jpeg";
		else if ( !_tcscmp( xmlGetText( m ), _T("image/png")))
			szPicType = "image/png";
		else if ( !_tcscmp( xmlGetText( m ), _T("image/gif")))
			szPicType = "image/gif";
		else if ( !_tcscmp( xmlGetText( m ), _T("image/bmp")))
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

static char* sttGetText( HXML node, char* tag )
{
	HXML n = xmlGetChild( node , tag );
	if ( n == NULL )
		return NULL;

	return mir_t2a( xmlGetText( n ) );
}

void CJabberProto::OnIqResultGetVcard( HXML iqNode )
{
	HXML vCardNode, m, n, o;
	const TCHAR* type, *jid;
	HANDLE hContact;
	TCHAR text[128];
	DBVARIANT dbv;

	Log( "<iq/> iqIdGetVcard" );
	if (( type = xmlGetAttrValue( iqNode, _T("type"))) == NULL ) return;
	if (( jid = xmlGetAttrValue( iqNode, _T("from"))) == NULL ) return;
	int id = JabberGetPacketID( iqNode );

	if ( id == m_nJabberSearchID ) {
		m_nJabberSearchID = -1;

		if (( vCardNode = xmlGetChild( iqNode , "vCard" )) != NULL ) {
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

	size_t len = _tcslen( m_szJabberJID );
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

		if (( vCardNode = xmlGetChild( iqNode , "vCard" )) != NULL ) {
			for ( int i=0; ; i++ ) {
				n = xmlGetChild( vCardNode ,i);
				if ( !n )
					break;
				if ( xmlGetName( n ) == NULL ) continue;
				if ( !_tcscmp( xmlGetName( n ), _T("FN"))) {
					if ( xmlGetText( n ) != NULL ) {
						hasFn = TRUE;
						JSetStringT( hContact, "FullName", xmlGetText( n ) );
					}
				}
				else if ( !lstrcmp( xmlGetName( n ), _T("NICKNAME"))) {
					if ( xmlGetText( n ) != NULL ) {
						hasNick = TRUE;
						JSetStringT( hContact, "Nick", xmlGetText( n ) );
					}
				}
				else if ( !lstrcmp( xmlGetName( n ), _T("N"))) {
					// First/Last name
					if ( !hasGiven && !hasFamily && !hasMiddle ) {
						if (( m=xmlGetChild( n , "GIVEN" )) != NULL && xmlGetText( m )!=NULL ) {
							hasGiven = TRUE;
							JSetStringT( hContact, "FirstName", xmlGetText( m ) );
						}
						if (( m=xmlGetChild( n , "FAMILY" )) != NULL && xmlGetText( m )!=NULL ) {
							hasFamily = TRUE;
							JSetStringT( hContact, "LastName", xmlGetText( m ) );
						}
						if (( m=xmlGetChild( n , "MIDDLE" )) != NULL && xmlGetText( m ) != NULL ) {
							hasMiddle = TRUE;
							JSetStringT( hContact, "MiddleName", xmlGetText( m ) );
					}	}
				}
				else if ( !lstrcmp( xmlGetName( n ), _T("EMAIL"))) {
					// E-mail address( es )
					if (( m=xmlGetChild( n , "USERID" )) == NULL )	// Some bad client put e-mail directly in <EMAIL/> instead of <USERID/>
						m = n;
					if ( xmlGetText( m ) != NULL ) {
						char text[100];
						if ( hContact != NULL ) {
							if ( nEmail == 0 )
								strcpy( text, "e-mail" );
							else
								sprintf( text, "e-mail%d", nEmail-1 );
						}
						else sprintf( text, "e-mail%d", nEmail );
						JSetStringT( hContact, text, xmlGetText( m ) );

						if ( hContact == NULL ) {
							sprintf( text, "e-mailFlag%d", nEmail );
							int nFlag = 0;
							if ( xmlGetChild( n , "HOME" ) != NULL ) nFlag |= JABBER_VCEMAIL_HOME;
							if ( xmlGetChild( n , "WORK" ) != NULL ) nFlag |= JABBER_VCEMAIL_WORK;
							if ( xmlGetChild( n , "INTERNET" ) != NULL ) nFlag |= JABBER_VCEMAIL_INTERNET;
							if ( xmlGetChild( n , "X400" ) != NULL ) nFlag |= JABBER_VCEMAIL_X400;
							JSetWord( NULL, text, nFlag );
						}
						nEmail++;
					}
				}
				else if ( !lstrcmp( xmlGetName( n ), _T("BDAY"))) {
					// Birthday
					if ( !hasBday && xmlGetText( n )!=NULL ) {
						if ( hContact != NULL ) {
							if ( _stscanf( xmlGetText( n ), _T("%d-%d-%d"), &nYear, &nMonth, &nDay ) == 3 ) {
								hasBday = TRUE;
								JSetWord( hContact, "BirthYear", ( WORD )nYear );
								JSetByte( hContact, "BirthMonth", ( BYTE ) nMonth );
								JSetByte( hContact, "BirthDay", ( BYTE ) nDay );
							}
						}
						else {
							hasBday = TRUE;
							JSetStringT( NULL, "BirthDate", xmlGetText( n ) );
					}	}
				}
				else if ( !lstrcmp( xmlGetName( n ), _T("GENDER"))) {
					// Gender
					if ( !hasGender && xmlGetText( n )!=NULL ) {
						if ( hContact != NULL ) {
							if ( xmlGetText( n )[0] && strchr( "mMfF", xmlGetText( n )[0] )!=NULL ) {
								hasGender = TRUE;
								JSetByte( hContact, "Gender", ( BYTE ) toupper( xmlGetText( n )[0] ));
							}
						}
						else {
							hasGender = TRUE;
							JSetStringT( NULL, "GenderString", xmlGetText( n ) );
					}	}
				}
				else if ( !lstrcmp( xmlGetName( n ), _T("ADR"))) {
					if ( !hasHome && xmlGetChild( n , "HOME" )!=NULL ) {
						// Home address
						hasHome = TRUE;
						if (( m=xmlGetChild( n , "STREET" )) != NULL && xmlGetText( m ) != NULL ) {
							hasHomeStreet = TRUE;
							if ( hContact != NULL ) {
								if (( o=xmlGetChild( n , "EXTADR" )) != NULL && xmlGetText( o ) != NULL )
									mir_sntprintf( text, SIZEOF( text ), _T("%s\r\n%s"), xmlGetText( m ), xmlGetText( o ) );
								else if (( o=xmlGetChild( n , "EXTADD" ))!=NULL && xmlGetText( o )!=NULL )
									mir_sntprintf( text, SIZEOF( text ), _T("%s\r\n%s"), xmlGetText( m ), xmlGetText( o ) );
								else
									_tcsncpy( text, xmlGetText( m ), SIZEOF( text ));
								text[SIZEOF(text)-1] = '\0';
								JSetStringT( hContact, "Street", text );
							}
							else {
								JSetStringT( hContact, "Street", xmlGetText( m ) );
								if (( m=xmlGetChild( n , "EXTADR" )) == NULL )
									m = xmlGetChild( n , "EXTADD" );
								if ( m!=NULL && xmlGetText( m )!=NULL ) {
									hasHomeStreet2 = TRUE;
									JSetStringT( hContact, "Street2", xmlGetText( m ) );
						}	}	}

						if (( m=xmlGetChild( n , "LOCALITY" ))!=NULL && xmlGetText( m )!=NULL ) {
							hasHomeLocality = TRUE;
							JSetStringT( hContact, "City", xmlGetText( m ) );
						}
						if (( m=xmlGetChild( n , "REGION" ))!=NULL && xmlGetText( m )!=NULL ) {
							hasHomeRegion = TRUE;
							JSetStringT( hContact, "State", xmlGetText( m ) );
						}
						if (( m=xmlGetChild( n , "PCODE" ))!=NULL && xmlGetText( m )!=NULL ) {
							hasHomePcode = TRUE;
							JSetStringT( hContact, "ZIP", xmlGetText( m ) );
						}
						if (( m=xmlGetChild( n , "CTRY" ))==NULL || xmlGetText( m )==NULL )	// Some bad client use <COUNTRY/> instead of <CTRY/>
							m = xmlGetChild( n , "COUNTRY" );
						if ( m!=NULL && xmlGetText( m )!=NULL ) {
							hasHomeCtry = TRUE;
							if ( hContact != NULL )
								JSetWord( hContact, "Country", ( WORD )JabberCountryNameToId( xmlGetText( m ) ));
							else
								JSetStringT( hContact, "CountryName", xmlGetText( m ) );
					}	}

					if ( !hasWork && xmlGetChild( n , "WORK" )!=NULL ) {
						// Work address
						hasWork = TRUE;
						if (( m=xmlGetChild( n , "STREET" ))!=NULL && xmlGetText( m )!=NULL ) {
							hasWorkStreet = TRUE;
							if ( hContact != NULL ) {
								if (( o=xmlGetChild( n , "EXTADR" ))!=NULL && xmlGetText( o )!=NULL )
									mir_sntprintf( text, SIZEOF( text ), _T("%s\r\n%s"), xmlGetText( m ), xmlGetText( o ) );
								else if (( o=xmlGetChild( n , "EXTADD" ))!=NULL && xmlGetText( o )!=NULL )
									mir_sntprintf( text, SIZEOF( text ), _T("%s\r\n%s"), xmlGetText( m ), xmlGetText( o ) );
								else
									_tcsncpy( text, xmlGetText( m ), SIZEOF( text ));
								text[SIZEOF( text )-1] = '\0';
								JSetStringT( hContact, "CompanyStreet", text );
							}
							else {
								JSetStringT( hContact, "CompanyStreet", xmlGetText( m ) );
								if (( m=xmlGetChild( n , "EXTADR" )) == NULL )
									m = xmlGetChild( n , "EXTADD" );
								if ( m!=NULL && xmlGetText( m )!=NULL ) {
									hasWorkStreet2 = TRUE;
									JSetStringT( hContact, "CompanyStreet2", xmlGetText( m ) );
						}	}	}

						if (( m=xmlGetChild( n , "LOCALITY" ))!=NULL && xmlGetText( m )!=NULL ) {
							hasWorkLocality = TRUE;
							JSetStringT( hContact, "CompanyCity", xmlGetText( m ) );
						}
						if (( m=xmlGetChild( n , "REGION" ))!=NULL && xmlGetText( m )!=NULL ) {
							hasWorkRegion = TRUE;
							JSetStringT( hContact, "CompanyState", xmlGetText( m ) );
						}
						if (( m=xmlGetChild( n , "PCODE" ))!=NULL && xmlGetText( m )!=NULL ) {
							hasWorkPcode = TRUE;
							JSetStringT( hContact, "CompanyZIP", xmlGetText( m ) );
						}
						if (( m=xmlGetChild( n , "CTRY" ))==NULL || xmlGetText( m )==NULL )	// Some bad client use <COUNTRY/> instead of <CTRY/>
							m = xmlGetChild( n , "COUNTRY" );
						if ( m!=NULL && xmlGetText( m )!=NULL ) {
							hasWorkCtry = TRUE;
							if ( hContact != NULL )
								JSetWord( hContact, "CompanyCountry", ( WORD )JabberCountryNameToId( xmlGetText( m ) ));
							else
								JSetStringT( hContact, "CompanyCountryName", xmlGetText( m ) );
					}	}
				}
				else if ( !lstrcmp( xmlGetName( n ), _T("TEL"))) {
					// Telephone/Fax/Cellular
					if (( m=xmlGetChild( n , "NUMBER" ))!=NULL && xmlGetText( m )!=NULL ) {
						if ( hContact != NULL ) {
							if ( !hasFax && xmlGetChild( n , "FAX" )!=NULL ) {
								hasFax = TRUE;
								JSetStringT( hContact, "Fax", xmlGetText( m ) );
							}
							if ( !hasCell && xmlGetChild( n , "CELL" )!=NULL ) {
								hasCell = TRUE;
								JSetStringT( hContact, "Cellular", xmlGetText( m ) );
							}
							if ( !hasPhone &&
								( xmlGetChild( n , "HOME" )!=NULL ||
								 xmlGetChild( n , "WORK" )!=NULL ||
								 xmlGetChild( n , "VOICE" )!=NULL ||
								 ( xmlGetChild( n , "FAX" )==NULL &&
								  xmlGetChild( n , "PAGER" )==NULL &&
								  xmlGetChild( n , "MSG" )==NULL &&
								  xmlGetChild( n , "CELL" )==NULL &&
								  xmlGetChild( n , "VIDEO" )==NULL &&
								  xmlGetChild( n , "BBS" )==NULL &&
								  xmlGetChild( n , "MODEM" )==NULL &&
								  xmlGetChild( n , "ISDN" )==NULL &&
								  xmlGetChild( n , "PCS" )==NULL )) ) {
								hasPhone = TRUE;
								JSetStringT( hContact, "Phone", xmlGetText( m ) );
							}
						}
						else {
							char text[ 100 ];
							sprintf( text, "Phone%d", nPhone );
							JSetStringT( NULL, text, xmlGetText( m ) );

							sprintf( text, "PhoneFlag%d", nPhone );
							int nFlag = 0;
							if ( xmlGetChild( n ,"HOME" ) != NULL ) nFlag |= JABBER_VCTEL_HOME;
							if ( xmlGetChild( n ,"WORK" ) != NULL ) nFlag |= JABBER_VCTEL_WORK;
							if ( xmlGetChild( n ,"VOICE" ) != NULL ) nFlag |= JABBER_VCTEL_VOICE;
							if ( xmlGetChild( n ,"FAX" ) != NULL ) nFlag |= JABBER_VCTEL_FAX;
							if ( xmlGetChild( n ,"PAGER" ) != NULL ) nFlag |= JABBER_VCTEL_PAGER;
							if ( xmlGetChild( n ,"MSG" ) != NULL ) nFlag |= JABBER_VCTEL_MSG;
							if ( xmlGetChild( n ,"CELL" ) != NULL ) nFlag |= JABBER_VCTEL_CELL;
							if ( xmlGetChild( n ,"VIDEO" ) != NULL ) nFlag |= JABBER_VCTEL_VIDEO;
							if ( xmlGetChild( n ,"BBS" ) != NULL ) nFlag |= JABBER_VCTEL_BBS;
							if ( xmlGetChild( n ,"MODEM" ) != NULL ) nFlag |= JABBER_VCTEL_MODEM;
							if ( xmlGetChild( n ,"ISDN" ) != NULL ) nFlag |= JABBER_VCTEL_ISDN;
							if ( xmlGetChild( n ,"PCS" ) != NULL ) nFlag |= JABBER_VCTEL_PCS;
							JSetWord( NULL, text, nFlag );
							nPhone++;
					}	}
				}
				else if ( !lstrcmp( xmlGetName( n ), _T("URL"))) {
					// Homepage
					if ( !hasUrl && xmlGetText( n )!=NULL ) {
						hasUrl = TRUE;
						JSetStringT( hContact, "Homepage", xmlGetText( n ) );
					}
				}
				else if ( !lstrcmp( xmlGetName( n ), _T("ORG"))) {
					if ( !hasOrgname && !hasOrgunit ) {
						if (( m=xmlGetChild( n ,"ORGNAME" ))!=NULL && xmlGetText( m )!=NULL ) {
							hasOrgname = TRUE;
							JSetStringT( hContact, "Company", xmlGetText( m ) );
						}
						if (( m=xmlGetChild( n ,"ORGUNIT" ))!=NULL && xmlGetText( m )!=NULL ) {	// The real vCard can have multiple <ORGUNIT/> but we will only display the first one
							hasOrgunit = TRUE;
							JSetStringT( hContact, "CompanyDepartment", xmlGetText( m ) );
					}	}
				}
				else if ( !lstrcmp( xmlGetName( n ), _T("ROLE"))) {
					if ( !hasRole && xmlGetText( n )!=NULL ) {
						hasRole = TRUE;
						JSetStringT( hContact, "Role", xmlGetText( n ) );
					}
				}
				else if ( !lstrcmp( xmlGetName( n ), _T("TITLE"))) {
					if ( !hasTitle && xmlGetText( n )!=NULL ) {
						hasTitle = TRUE;
						JSetStringT( hContact, "CompanyPosition", xmlGetText( n ) );
					}
				}
				else if ( !lstrcmp( xmlGetName( n ), _T("DESC"))) {
					if ( !hasDesc && xmlGetText( n )!=NULL ) {
						hasDesc = TRUE;
						TCHAR* szMemo = JabberUnixToDosT( xmlGetText( n ) );
						JSetStringT( hContact, "About", szMemo );
						mir_free( szMemo );
					}
				}
				else if ( !lstrcmp( xmlGetName( n ), _T("PHOTO")))
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
		else {
			if (( hContact = HContactFromJID( jid )) != NULL )
				JSendBroadcast( hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, ( HANDLE ) 1, 0 );
			WindowNotify(WM_JABBER_REFRESH_VCARD);
		}
	}
	else if ( !lstrcmp( type, _T("error"))) {
		if (( hContact = HContactFromJID( jid )) != NULL )
			JSendBroadcast( hContact, ACKTYPE_GETINFO, ACKRESULT_FAILED, ( HANDLE ) 1, 0 );
	}
}

void CJabberProto::OnIqResultSetVcard( HXML iqNode )
{
	Log( "<iq/> iqIdSetVcard" );
	if ( !xmlGetAttrValue( iqNode, _T("type") ))
		return;

	WindowNotify(WM_JABBER_REFRESH_VCARD);
}

void CJabberProto::OnIqResultSetSearch( HXML iqNode )
{
	HXML queryNode, n;
	const TCHAR* type, *jid;
	int i, id;
	JABBER_SEARCH_RESULT jsr;

	Log( "<iq/> iqIdGetSearch" );
	if (( type = xmlGetAttrValue( iqNode, _T("type"))) == NULL ) return;
	if (( id = JabberGetPacketID( iqNode )) == -1 ) return;

	if ( !lstrcmp( type, _T("result"))) {
		if (( queryNode=xmlGetChild( iqNode , "query" )) == NULL ) return;
		jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
		for ( i=0; ; i++ ) {
			HXML itemNode = xmlGetChild( queryNode ,i);
			if ( !itemNode )
				break;

			if ( !lstrcmp( xmlGetName( itemNode ), _T("item"))) {
				if (( jid=xmlGetAttrValue( itemNode, _T("jid"))) != NULL ) {
					_tcsncpy( jsr.jid, jid, SIZEOF( jsr.jid ));
					jsr.jid[ SIZEOF( jsr.jid )-1] = '\0';
					Log( "Result jid = " TCHAR_STR_PARAM, jid );
					if (( n=xmlGetChild( itemNode , "nick" ))!=NULL && xmlGetText( n )!=NULL )
						jsr.hdr.nick = mir_t2a( xmlGetText( n ) );
					else
						jsr.hdr.nick = mir_strdup( "" );
					if (( n=xmlGetChild( itemNode , "first" ))!=NULL && xmlGetText( n )!=NULL )
						jsr.hdr.firstName = mir_t2a( xmlGetText( n ) );
					else
						jsr.hdr.firstName = mir_strdup( "" );
					if (( n=xmlGetChild( itemNode , "last" ))!=NULL && xmlGetText( n )!=NULL )
						jsr.hdr.lastName = mir_t2a( xmlGetText( n ) );
					else
						jsr.hdr.lastName = mir_strdup( "" );
					if (( n=xmlGetChild( itemNode , "email" ))!=NULL && xmlGetText( n )!=NULL )
						jsr.hdr.email = mir_t2a( xmlGetText( n ) );
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

void CJabberProto::OnIqResultExtSearch( HXML iqNode )
{
	HXML queryNode;
	const TCHAR* type;
	int id;

	Log( "<iq/> iqIdGetExtSearch" );
	if (( type=xmlGetAttrValue( iqNode, _T("type"))) == NULL ) return;
	if (( id = JabberGetPacketID( iqNode )) == -1 ) return;

	if ( !lstrcmp( type, _T("result"))) {
		if (( queryNode=xmlGetChild( iqNode , "query" )) == NULL ) return;
		if (( queryNode=xmlGetChild( queryNode , "x" )) == NULL ) return;
		for ( int i=0; ; i++ ) {
			HXML itemNode = xmlGetChild( queryNode ,i);
			if ( !itemNode )
				break;
			if ( lstrcmp( xmlGetName( itemNode ), _T("item")))
				continue;

			JABBER_SEARCH_RESULT jsr = { 0 };
			jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
//			jsr.hdr.firstName = "";

			for ( int j=0; ; j++ ) {
				HXML fieldNode = xmlGetChild( itemNode ,j);
				if ( !fieldNode )
					break;

				if ( lstrcmp( xmlGetName( fieldNode ), _T("field")))
					continue;

				const TCHAR* fieldName = xmlGetAttrValue( fieldNode, _T("var"));
				if ( fieldName == NULL )
					continue;

				HXML n = xmlGetChild( fieldNode , "value" );
				if ( n == NULL )
					continue;

				if ( !lstrcmp( fieldName, _T("jid"))) {
					_tcsncpy( jsr.jid, xmlGetText( n ), SIZEOF( jsr.jid ));
					jsr.jid[SIZEOF( jsr.jid )-1] = '\0';
					Log( "Result jid = " TCHAR_STR_PARAM, jsr.jid );
				}
				else if ( !lstrcmp( fieldName, _T("nickname")))
					jsr.hdr.nick = ( xmlGetText( n ) != NULL ) ? mir_t2a( xmlGetText( n ) ) : mir_strdup( "" );
				else if ( !lstrcmp( fieldName, _T("fn"))) {
					mir_free( jsr.hdr.firstName );
					jsr.hdr.firstName = ( xmlGetText( n ) != NULL ) ? mir_t2a(xmlGetText( n )) : mir_strdup( "" );
				}
				else if ( !lstrcmp( fieldName, _T("given"))) {
					mir_free( jsr.hdr.firstName );
					jsr.hdr.firstName = ( xmlGetText( n ) != NULL ) ? mir_t2a(xmlGetText( n )) : mir_strdup( "" );
				}
				else if ( !lstrcmp( fieldName, _T("family")))
					jsr.hdr.lastName = ( xmlGetText( n ) != NULL ) ? mir_t2a(xmlGetText( n )) : mir_strdup( "" );
				else if ( !lstrcmp( fieldName, _T("email")))
					jsr.hdr.email = ( xmlGetText( n ) != NULL ) ? mir_t2a(xmlGetText( n )) : mir_strdup( "" );
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

void CJabberProto::OnIqResultSetPassword( HXML iqNode )
{
	Log( "<iq/> iqIdSetPassword" );

	const TCHAR* type = xmlGetAttrValue( iqNode, _T("type"));
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
void CJabberProto::OnIqResultDiscoAgentItems( HXML iqNode, void *userdata )
{
	if ( !m_options.EnableAvatars )
		return;
}
*/
void CJabberProto::OnIqResultGetVCardAvatar( HXML iqNode )
{
	const TCHAR* type;

	Log( "<iq/> OnIqResultGetVCardAvatar" );

	const TCHAR* from = xmlGetAttrValue( iqNode, _T("from"));
	if ( from == NULL )
		return;
	HANDLE hContact = HContactFromJID( from );
	if ( hContact == NULL )
		return;

	if (( type = xmlGetAttrValue( iqNode, _T("type"))) == NULL ) return;
	if ( _tcscmp( type, _T("result"))) return;

	HXML vCard = xmlGetChild( iqNode , "vCard" );
	if (vCard == NULL) return;
	vCard = xmlGetChild( vCard , "PHOTO" );
	if (vCard == NULL) return;

	if ( xmlGetChildCount( vCard ) == 0 ) {
		JDeleteSetting( hContact, "AvatarHash" );
		DBVARIANT dbv = {0};
		if ( !JGetStringT( hContact, "AvatarSaved", &dbv ) ) {
			JFreeVariant( &dbv );
			JDeleteSetting( hContact, "AvatarSaved" );
			JSendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, NULL, NULL );
		}

		return;
	}

	HXML typeNode = xmlGetChild( vCard , "TYPE" );
	const TCHAR* mimeType = NULL;
	if (typeNode != NULL) mimeType = xmlGetText( typeNode );
	HXML n = xmlGetChild( vCard , "BINVAL" );
	if ( n == NULL ) 
		return;

	JSetByte( hContact, "AvatarXVcard", 1 );
	OnIqResultGotAvatar( hContact, n, mimeType);
}

void CJabberProto::OnIqResultGetClientAvatar( HXML iqNode )
{
	const TCHAR* type;

	Log( "<iq/> iqIdResultGetClientAvatar" );

	const TCHAR* from = xmlGetAttrValue( iqNode, _T("from"));
	if ( from == NULL )
		return;
	HANDLE hContact = HContactFromJID( from );
	if ( hContact == NULL )
		return;

	HXML n = NULL;
	if (( type = xmlGetAttrValue( iqNode, _T("type"))) != NULL && !_tcscmp( type, _T("result")) ) {
		HXML queryNode = xmlGetChild( iqNode , "query" );
		if ( queryNode != NULL ) {
			const TCHAR* xmlns = xmlGetAttrValue( queryNode, _T("xmlns"));
			if ( !lstrcmp( xmlns, _T(JABBER_FEAT_AVATAR))) {
				n = xmlGetChild( queryNode , "data" );
			}
		}
	}

	if ( n == NULL ) {
		TCHAR szJid[ 512 ];
		lstrcpyn(szJid, from, 512);
		TCHAR *res = _tcschr(szJid, _T('/'));
		if ( res != NULL )
			*res = 0;

		// Try server stored avatar
		int iqId = SerialNext();
		IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultGetServerAvatar );

		XmlNodeIq iq( _T("get"), iqId, szJid );
		iq << XQUERY( _T(JABBER_FEAT_SERVER_AVATAR));
		m_ThreadInfo->send( iq );

		return;
	}

	const TCHAR* mimeType = mimeType = xmlGetAttrValue( n, _T("mimetype"));

	OnIqResultGotAvatar( hContact, n, mimeType);
}


void CJabberProto::OnIqResultGetServerAvatar( HXML iqNode )
{
	const TCHAR* type;

	Log( "<iq/> iqIdResultGetServerAvatar" );

	const TCHAR* from = xmlGetAttrValue( iqNode, _T("from"));
	if ( from == NULL )
		return;
	HANDLE hContact = HContactFromJID( from );
	if ( hContact == NULL )
		return;

	HXML n = NULL;
	if (( type = xmlGetAttrValue( iqNode, _T("type"))) != NULL && !_tcscmp( type, _T("result")) ) {
		HXML queryNode = xmlGetChild( iqNode , "query" );
		if ( queryNode != NULL ) {
			const TCHAR* xmlns = xmlGetAttrValue( queryNode, _T("xmlns"));
			if ( !lstrcmp( xmlns, _T(JABBER_FEAT_SERVER_AVATAR)) ) {
				n = xmlGetChild( queryNode , "data" );
			}
		}
	}

	if ( n == NULL ) {
		TCHAR szJid[ 512 ];
		lstrcpyn(szJid, from, 512);
		TCHAR *res = _tcschr(szJid, _T('/'));
		if ( res != NULL )
			*res = 0;

		// Try VCard photo
		int iqId = SerialNext();
		IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultGetVCardAvatar );

		XmlNodeIq iq( _T("get"), iqId, szJid );
		iq << XCHILDNS( _T("vCard"), _T(JABBER_FEAT_VCARD_TEMP));
		m_ThreadInfo->send( iq );

		return;
	}

	const TCHAR* mimeType = xmlGetAttrValue( n, _T("mimetype"));

	OnIqResultGotAvatar( hContact, n, mimeType);
}


void CJabberProto::OnIqResultGotAvatar( HANDLE hContact, HXML n, const TCHAR* mimeType ) 
{
	int resultLen = 0;
	char* body = JabberBase64Decode( xmlGetText( n ), &resultLen );

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

	GetAvatarFileName( hContact, AI.filename, sizeof AI.filename );

	FILE* out = fopen( AI.filename, "wb" );
	if ( out != NULL ) {
		fwrite( body, resultLen, 1, out );
		fclose( out );
		JSetString( hContact, "AvatarSaved", buffer );
		JSendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, HANDLE( &AI ), NULL );
		Log("Broadcast new avatar: %s",AI.filename);
	}
	else JSendBroadcast( hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, HANDLE( &AI ), NULL );

	mir_free( body );
}

/////////////////////////////////////////////////////////////////////////////////////////
// Bookmarks

void CJabberProto::OnIqResultDiscoBookmarks( HXML iqNode )
{
	HXML storageNode;//, nickNode, passNode;
	const TCHAR* type, *jid, *name;

	// RECVED: list of bookmarks
	// ACTION: refresh bookmarks dialog
	Log( "<iq/> iqIdGetBookmarks" );
	if (( type = xmlGetAttrValue( iqNode, _T("type"))) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		if ( m_ThreadInfo && !( m_ThreadInfo->jabberServerCaps & JABBER_CAPS_PRIVATE_STORAGE )) {
			m_ThreadInfo->jabberServerCaps |= JABBER_CAPS_PRIVATE_STORAGE;
			EnableMenuItems( TRUE );
		}

		if ( storageNode = XPathT( iqNode, "query/storage[@xmlns='storage:bookmarks']" )) {
			ListRemoveList( LIST_BOOKMARK );

			HXML itemNode;
			for ( int i = 0; itemNode = xmlGetChild( storageNode, i ); i++ ) {
				if ( name = xmlGetName( itemNode)) {
					if ( !_tcscmp( name, _T("conference") ) && (jid = xmlGetAttrValue( itemNode, _T("jid") ))) {
						JABBER_LIST_ITEM* item = ListAdd( LIST_BOOKMARK, jid );
						item->name = mir_tstrdup( xmlGetAttrValue( itemNode, _T("name")));
						item->type = mir_tstrdup( _T( "conference" ));
						item->bUseResource = TRUE;
						item->nick = mir_tstrdup( XPathT( itemNode, "nick" ));
						item->password = mir_tstrdup( XPathT( itemNode, "password" ));

						const TCHAR* autoJ = xmlGetAttrValue( itemNode, _T("autojoin"));
						if ( autoJ != NULL )
							item->bAutoJoin = ( !lstrcmp( autoJ, _T("true")) || !lstrcmp( autoJ, _T("1"))) ? true : false;
					}
					else if ( !_tcscmp( name, _T("url")) && (jid = xmlGetAttrValue( itemNode, _T("url")  ))) {
						JABBER_LIST_ITEM* item = ListAdd( LIST_BOOKMARK, jid );
						item->bUseResource = TRUE;
						item->name = mir_tstrdup( xmlGetAttrValue( itemNode, _T("name")));
						item->type = mir_tstrdup( _T("url") );
					}
				}
			}

			UI_SAFE_NOTIFY(m_pDlgBookmarks, WM_JABBER_REFRESH);
			m_ThreadInfo->bBookmarksLoaded = TRUE;
			OnProcessLoginRq(m_ThreadInfo, JABBER_LOGIN_BOOKMARKS);
		}
	}
	else if ( !lstrcmp( type, _T("error"))) {
		if ( m_ThreadInfo->jabberServerCaps & JABBER_CAPS_PRIVATE_STORAGE ) {
			m_ThreadInfo->jabberServerCaps &= ~JABBER_CAPS_PRIVATE_STORAGE;
			EnableMenuItems( TRUE );
			UI_SAFE_NOTIFY(m_pDlgBookmarks, WM_JABBER_ACTIVATE);
			return;
		}
}	}

void CJabberProto::SetBookmarkRequest (XmlNodeIq& iq)
{
	HXML query = iq << XQUERY( _T(JABBER_FEAT_PRIVATE_STORAGE));
	HXML storage = query << XCHILDNS( _T("storage"), _T("storage:bookmarks"));

	LISTFOREACH(i, this, LIST_BOOKMARK)
	{
		JABBER_LIST_ITEM* item = ListGetItemPtrFromIndex( i );
		if ( item == NULL )
			continue;

		if ( item->jid == NULL )
			continue;
		if ( !lstrcmp( item->type, _T("conference"))) {
			HXML itemNode = storage << XCHILD( _T("conference")) << XATTR( _T("jid"), item->jid );
			if ( item->name )
				itemNode << XATTR( _T("name"), item->name );
			if ( item->bAutoJoin )
				itemNode << XATTRI( _T("autojoin"), 1 );
			if ( item->nick )
				itemNode << XCHILD( _T("nick"), item->nick );
			if ( item->password )
				itemNode << XCHILD( _T("password"), item->password );
		}
		if ( !lstrcmp( item->type, _T("url"))) {
			HXML itemNode = storage << XCHILD( _T("url")) << XATTR( _T("url"), item->jid );
			if ( item->name )
				itemNode << XATTR( _T("name"), item->name );
		}
	}
}

void CJabberProto::OnIqResultSetBookmarks( HXML iqNode )
{
	// RECVED: server's response
	// ACTION: refresh bookmarks list dialog

	Log( "<iq/> iqIdSetBookmarks" );

	const TCHAR* type = xmlGetAttrValue( iqNode, _T("type"));
	if ( type == NULL )
		return;

	if ( !lstrcmp( type, _T("result"))) {
		UI_SAFE_NOTIFY(m_pDlgBookmarks, WM_JABBER_REFRESH);
	}
	else if ( !lstrcmp( type, _T("error"))) {
		HXML errorNode = xmlGetChild( iqNode , "error" );
		TCHAR* str = JabberErrorMsg( errorNode );
		MessageBox( NULL, str, TranslateT( "Jabber Bookmarks Error" ), MB_OK|MB_SETFOREGROUND );
		mir_free( str );
		UI_SAFE_NOTIFY(m_pDlgBookmarks, WM_JABBER_ACTIVATE);
}	}

// last activity (XEP-0012) support
void CJabberProto::OnIqResultLastActivity( HXML iqNode, CJabberIqInfo* pInfo )
{
	JABBER_RESOURCE_STATUS *r = ResourceInfoFromJID( pInfo->m_szFrom );
	if ( !r )
		return;

	time_t lastActivity = -1;
	if ( pInfo->m_nIqType == JABBER_IQ_TYPE_RESULT ) {
		LPCTSTR szSeconds = XPathT( iqNode, "query[@xmlns='jabber:iq:last']/@seconds" );
		if ( szSeconds ) {
			int nSeconds = _ttoi( szSeconds );
			if ( nSeconds > 0 )
				lastActivity = time( 0 ) - nSeconds;
		}

		LPCTSTR szLastStatusMessage = XPathT( iqNode, "query[@xmlns='jabber:iq:last']" );
		if ( szLastStatusMessage ) // replace only if it exists
			replaceStr( r->statusMessage, szLastStatusMessage );
	}

	r->idleStartTime = lastActivity;

	JabberUserInfoUpdate(pInfo->GetHContact());
}

// entity time (XEP-0202) support
void CJabberProto::OnIqResultEntityTime( HXML pIqNode, CJabberIqInfo* pInfo )
{
	if ( !pInfo->m_hContact )
		return;

	if ( pInfo->m_nIqType == JABBER_IQ_TYPE_RESULT ) {
		LPCTSTR szTzo = XPathFmt( pIqNode, _T("time[@xmlns='%s']/tzo"), _T( JABBER_FEAT_ENTITY_TIME ));
		if ( szTzo && _tcslen( szTzo ) == 6 ) {
			bool bNegative = szTzo[0] == _T('-');
			int nTz = ( _ttoi( szTzo + 1 ) * 60 + _ttoi( szTzo + 4 )) / 30;

			TIME_ZONE_INFORMATION tzinfo;
			if ( GetTimeZoneInformation( &tzinfo ) == TIME_ZONE_ID_DAYLIGHT )
				nTz += ( bNegative ? -tzinfo.DaylightBias : tzinfo.DaylightBias ) / 30;

			if ( !bNegative )
				nTz = 256 - nTz;
			JSetByte( pInfo->m_hContact, "Timezone", nTz );
			return;
		}
	}

	JDeleteSetting( pInfo->m_hContact, "Timezone" );
	return;
}
