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

void CJabberProto::OnIqResultServerDiscoInfo( XmlNode* iqNode, void* userdata )
{
	if ( !iqNode )
		return;

	TCHAR *type = JabberXmlGetAttrValue( iqNode, "type" );
	int i;

	if ( !_tcscmp( type, _T("result"))) {
		XmlNode *query = JabberXmlGetChildWithGivenAttrValue( iqNode, "query", "xmlns", _T(JABBER_FEAT_DISCO_INFO) );
		if ( !query )
			return;

		XmlNode *identity;
		for ( i = 1; ( identity = JabberXmlGetNthChild( query, "identity", i )) != NULL; i++ ) {
			TCHAR *identityCategory = JabberXmlGetAttrValue( identity, "category" );
			TCHAR *identityType = JabberXmlGetAttrValue( identity, "type" );
			if ( identityCategory && identityType && !_tcscmp( identityCategory, _T("pubsub") ) && !_tcscmp( identityType, _T("pep")) ) {
				m_bPepSupported = TRUE;

				RebuildStatusMenu();
				RebuildInfoFrame();
				break;
			}
		}
		if ( m_ThreadInfo ) {
			m_ThreadInfo->jabberServerCaps = JABBER_RESOURCE_CAPS_NONE;
			XmlNode *feature;
			for ( i = 1; ( feature = JabberXmlGetNthChild( query, "feature", i )) != NULL; i++ ) {
				TCHAR *featureName = JabberXmlGetAttrValue( feature, "var" );
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

void CJabberProto::OnIqResultNestedRosterGroups( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	TCHAR *szGroupDelimeter = NULL;
	BOOL bPrivateStorageSupport = FALSE;

	if ( iqNode && pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT ) {
		XmlNode *pQuery = JabberXmlGetChildWithGivenAttrValue( iqNode, "query", "xmlns", _T( JABBER_FEAT_PRIVATE_STORAGE ));
		if ( pQuery ) {
			bPrivateStorageSupport = TRUE;
			XmlNode *pRoster = JabberXmlGetChildWithGivenAttrValue( pQuery, "roster", "xmlns", _T( JABBER_FEAT_NESTED_ROSTER_GROUPS ));
			if ( pRoster && pRoster->text && *pRoster->text )
				szGroupDelimeter = pRoster->text;
		}
	}

	// global fuckup
	if ( !m_ThreadInfo )
		return;

	// is our default delimiter?
	if (( !szGroupDelimeter && bPrivateStorageSupport ) || ( szGroupDelimeter && _tcscmp( szGroupDelimeter, _T("\\") ))) {
		XmlNodeIq iqNRG( "set", SerialNext() );
		XmlNode* query = iqNRG.addQuery( JABBER_FEAT_PRIVATE_STORAGE );
		XmlNode* roster = query->addChild( "roster", _T("\\") );
		roster->addAttr( "xmlns", JABBER_FEAT_NESTED_ROSTER_GROUPS );
		m_ThreadInfo->send( iqNRG );
	}

	// roster request
	TCHAR *szUserData = mir_tstrdup( szGroupDelimeter ? szGroupDelimeter : _T("\\") );
	XmlNodeIq iq( m_iqManager.AddHandler( &CJabberProto::OnIqResultGetRoster, JABBER_IQ_TYPE_GET, NULL, 0, -1, (void *)szUserData ));
	XmlNode* query = iq.addChild( "query" ); query->addAttr( "xmlns", JABBER_FEAT_IQ_ROSTER );
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
		XmlNode* query = iqNRG.addQuery( JABBER_FEAT_PRIVATE_STORAGE );
		XmlNode* roster = query->addChild( "roster" );
		roster->addAttr( "xmlns", JABBER_FEAT_NESTED_ROSTER_GROUPS );
		info->send( iqNRG );
	}

	int iqId = SerialNext();
	IqAdd( iqId, IQ_PROC_DISCOBOOKMARKS, &CJabberProto::OnIqResultDiscoBookmarks);
	XmlNodeIq biq( "get", iqId);
	XmlNode* bquery = biq.addQuery( JABBER_FEAT_PRIVATE_STORAGE );
	XmlNode* storage = bquery->addChild( "storage" );
	storage->addAttr( "xmlns", "storage:bookmarks" );
	info->send( biq );

	m_bPepSupported = FALSE;
	info->jabberServerCaps = JABBER_RESOURCE_CAPS_NONE;
	iqId = SerialNext();
	IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultServerDiscoInfo );
	XmlNodeIq diq( "get", iqId, info->server );
	diq.addQuery( JABBER_FEAT_DISCO_INFO );
	info->send( diq );

	QueryPrivacyLists( info );

	BOOL bSendVcardRequest = TRUE;
	DBVARIANT dbvLastLoggedJID = {0};
	if ( !JGetStringT( NULL, "LastLoggedJID", &dbvLastLoggedJID )) {
		if ( !_tcsicmp( m_szJabberJID, dbvLastLoggedJID.ptszVal ))
			bSendVcardRequest = FALSE;
		JFreeVariant( &dbvLastLoggedJID );
	}
	JSetStringT( NULL, "LastLoggedJID", m_szJabberJID );

	if ( bSendVcardRequest )
		SendGetVcard( m_szJabberJID );
	else {
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
}

void CJabberProto::OnIqResultGetAuth( XmlNode *iqNode, void *userdata )
{
	// RECVED: result of the request for authentication method
	// ACTION: send account authentication information to log in
	Log( "<iq/> iqIdGetAuth" );

	ThreadData* info = ( ThreadData* ) userdata;
	XmlNode *queryNode;
	TCHAR* type;
	if (( type=JabberXmlGetAttrValue( iqNode, "type" )) == NULL ) return;
	if (( queryNode=JabberXmlGetChild( iqNode, "query" )) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		int iqId = SerialNext();
		IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultSetAuth );

		XmlNodeIq iq( "set", iqId );
		XmlNode* query = iq.addQuery( "jabber:iq:auth" );
		query->addChild( "username", info->username );
		if ( JabberXmlGetChild( queryNode, "digest" )!=NULL && m_szStreamId ) {
			char* str = mir_utf8encode( info->password );
			char text[200];
			mir_snprintf( text, SIZEOF(text), "%s%s", m_szStreamId, str );
			mir_free( str );
         if (( str=JabberSha1( text )) != NULL ) {
				query->addChild( "digest", str );
				mir_free( str );
			}
		}
		else if ( JabberXmlGetChild( queryNode, "password" ) != NULL )
			query->addChild( "password", info->password );
		else {
			Log( "No known authentication mechanism accepted by the server." );

			info->send( "</stream:stream>" );
			return;
		}

		if ( JabberXmlGetChild( queryNode, "resource" ) != NULL )
			query->addChild( "resource", info->resource );

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

void CJabberProto::OnIqResultSetAuth( XmlNode *iqNode, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;
	TCHAR* type;

	// RECVED: authentication result
	// ACTION: if successfully logged in, continue by requesting roster list and set my initial status
	Log( "<iq/> iqIdSetAuth" );
	if (( type=JabberXmlGetAttrValue( iqNode, "type" )) == NULL ) return;

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

void CJabberProto::OnIqResultBind( XmlNode *iqNode, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;
	XmlNode* n = JabberXmlGetChild( iqNode, "bind" );
	if ( n != NULL ) {
		if ( n = JabberXmlGetChild( n, "jid" )) {
			if ( n->text ) {
				if ( !_tcsncmp( info->fullJID, n->text, SIZEOF( info->fullJID )))
					Log( "Result Bind: "TCHAR_STR_PARAM" %s "TCHAR_STR_PARAM, info->fullJID, "confirmed.", NULL );
				else {
					Log( "Result Bind: "TCHAR_STR_PARAM" %s "TCHAR_STR_PARAM, info->fullJID, "changed to", n->text);
					_tcsncpy( info->fullJID, n->text, SIZEOF( info->fullJID ));
		}	}	}

		if ( info->bIsSessionAvailable ) {
			int iqId = SerialNext();
			IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultSession );

			XmlNodeIq iq( "set" ); iq.addAttrID( iqId );
			iq.addChild( "session" )->addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-session" );
			info->send( iq );
		}
		else OnLoggedIn( info );
	}
   else if ( n = JabberXmlGetChild( n, "error" )) {
		//rfc3920 page 39
		info->send( "</stream:stream>" );
		m_ThreadInfo = NULL;	// To disallow auto reconnect
}	}

void CJabberProto::OnIqResultSession( XmlNode *iqNode, void *userdata )
{
	ThreadData* info = ( ThreadData* )userdata;

	TCHAR* type;
	if (( type=JabberXmlGetAttrValue( iqNode, "type" )) == NULL ) return;

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

void CJabberProto::OnIqResultGetRoster( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	Log( "<iq/> iqIdGetRoster" );
	TCHAR *szGroupDelimeter = (TCHAR *)pInfo->GetUserData();
	if ( pInfo->GetIqType() != JABBER_IQ_TYPE_RESULT ) {
		mir_free( szGroupDelimeter );
		return;
	}

	XmlNode* queryNode = JabberXmlGetChild( iqNode, "query" );
	if ( queryNode == NULL ) {
		mir_free( szGroupDelimeter );
		return;
	}

	if ( lstrcmp( JabberXmlGetAttrValue( queryNode, "xmlns" ), _T(JABBER_FEAT_IQ_ROSTER))) {
		mir_free( szGroupDelimeter );
		return;
	}

	if ( !_tcscmp( szGroupDelimeter, _T("\\"))) {
		mir_free( szGroupDelimeter );
		szGroupDelimeter = NULL;
	}

	TCHAR* name, *nick;
	int i;
	SortedList chatRooms = {0};
	chatRooms.increment = 10;

	for ( i=0; i < queryNode->numChild; i++ ) {
		BOOL bIsTransport=FALSE;

		XmlNode* itemNode = queryNode->child[i];
		if ( strcmp( itemNode->name, "item" ))
			continue;

		TCHAR* str = JabberXmlGetAttrValue( itemNode, "subscription" );

		JABBER_SUBSCRIPTION sub;
		if ( str == NULL ) sub = SUB_NONE;
		else if ( !_tcscmp( str, _T("both"))) sub = SUB_BOTH;
		else if ( !_tcscmp( str, _T("to"))) sub = SUB_TO;
		else if ( !_tcscmp( str, _T("from"))) sub = SUB_FROM;
		else sub = SUB_NONE;

		TCHAR* jid = JabberXmlGetAttrValue( itemNode, "jid" );
		if ( jid == NULL )
			continue;
		if ( _tcschr( jid, '@' ) == NULL )
			bIsTransport = TRUE;

		if (( name = JabberXmlGetAttrValue( itemNode, "name" )) != NULL )
			nick = mir_tstrdup( name );
		else
			nick = JabberNickFromJID( jid );

		if ( nick == NULL )
			continue;

		JABBER_LIST_ITEM* item = ListAdd( LIST_ROSTER, jid );
		item->subscription = sub;

		mir_free( item->nick ); item->nick = nick;

		XmlNode* groupNode = JabberXmlGetChild( itemNode, "group" );
		replaceStr( item->group, ( groupNode ) ? groupNode->text : NULL );

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

void CJabberProto::OnIqResultGetRegister( XmlNode *iqNode, void *userdata )
{
	// RECVED: result of the request for ( agent ) registration mechanism
	// ACTION: activate ( agent ) registration input dialog
	Log( "<iq/> iqIdGetRegister" );

	ThreadData* info = ( ThreadData* ) userdata;
	XmlNode *queryNode, *n;
	TCHAR *type;
	if (( type=JabberXmlGetAttrValue( iqNode, "type" )) == NULL ) return;
	if (( queryNode=JabberXmlGetChild( iqNode, "query" )) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		if ( m_hwndAgentRegInput )
			if (( n = JabberXmlCopyNode( iqNode )) != NULL )
				SendMessage( m_hwndAgentRegInput, WM_JABBER_REGINPUT_ACTIVATE, 1 /*success*/, ( LPARAM )n );
	}
	else if ( !lstrcmp( type, _T("error"))) {
		if ( m_hwndAgentRegInput ) {
			XmlNode *errorNode = JabberXmlGetChild( iqNode, "error" );
			TCHAR* str = JabberErrorMsg( errorNode );
			SendMessage( m_hwndAgentRegInput, WM_JABBER_REGINPUT_ACTIVATE, 0 /*error*/, ( LPARAM )str );
			mir_free( str );
}	}	}

void CJabberProto::OnIqResultSetRegister( XmlNode *iqNode, void *userdata )
{
	// RECVED: result of registration process
	// ACTION: notify of successful agent registration
	Log( "<iq/> iqIdSetRegister" );

	TCHAR *type, *from;
	if (( type = JabberXmlGetAttrValue( iqNode, "type" )) == NULL ) return;
	if (( from = JabberXmlGetAttrValue( iqNode, "from" )) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		HANDLE hContact = HContactFromJID( from );
		if ( hContact != NULL )
			JSetByte( hContact, "IsTransport", TRUE );

		if ( m_hwndRegProgress )
			SendMessage( m_hwndRegProgress, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )TranslateT( "Registration successful" ));
	}
	else if ( !lstrcmp( type, _T("error"))) {
		if ( m_hwndRegProgress ) {
			XmlNode *errorNode = JabberXmlGetChild( iqNode, "error" );
			TCHAR* str = JabberErrorMsg( errorNode );
			SendMessage( m_hwndRegProgress, WM_JABBER_REGDLG_UPDATE, 100, ( LPARAM )str );
			mir_free( str );
}	}	}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberIqResultGetVcard - processes the server-side v-card

void CJabberProto::OnIqResultGetVcardPhoto( const TCHAR* jid, XmlNode* n, HANDLE hContact, BOOL& hasPhoto )
{
	Log( "JabberIqResultGetVcardPhoto: %d", hasPhoto );
	if ( hasPhoto )
		return;

	XmlNode* o = JabberXmlGetChild( n, "BINVAL" );
	if ( o == NULL || o->text == NULL )
		return;

	int bufferLen;
	char* buffer = JabberBase64Decode( o->text, &bufferLen );
	if ( buffer == NULL )
		return;

	char* szPicType;
	XmlNode* m = JabberXmlGetChild( n, "TYPE" );
	if ( m == NULL || m->text == NULL ) {
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
		if ( !_tcscmp( m->text, _T("image/jpeg")))
			szPicType = "image/jpeg";
		else if ( !_tcscmp( m->text, _T("image/png")))
			szPicType = "image/png";
		else if ( !_tcscmp( m->text, _T("image/gif")))
			szPicType = "image/gif";
		else if ( !_tcscmp( m->text, _T("image/bmp")))
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

static char* sttGetText( XmlNode* node, char* tag )
{
	XmlNode* n = JabberXmlGetChild( node, tag );
	if ( n == NULL )
		return NULL;

	return mir_t2a( n->text );
}

void CJabberProto::OnIqResultGetVcard( XmlNode *iqNode, void *userdata )
{
	XmlNode *vCardNode, *m, *n, *o;
	TCHAR* type, *jid;
	HANDLE hContact;
	TCHAR text[128];
	DBVARIANT dbv;

	Log( "<iq/> iqIdGetVcard" );
	if (( type=JabberXmlGetAttrValue( iqNode, "type" )) == NULL ) return;
	if (( jid=JabberXmlGetAttrValue( iqNode, "from" )) == NULL ) return;
	int id = JabberGetPacketID( iqNode );

	if ( id == m_nJabberSearchID ) {
		m_nJabberSearchID = -1;

		if (( vCardNode = JabberXmlGetChild( iqNode, "vCard" )) != NULL ) {
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

		if (( vCardNode=JabberXmlGetChild( iqNode, "vCard" )) != NULL ) {
			for ( int i=0; i<vCardNode->numChild; i++ ) {
				n = vCardNode->child[i];
				if ( n==NULL || n->name==NULL ) continue;
				if ( !strcmp( n->name, "FN" )) {
					if ( n->text != NULL ) {
						hasFn = TRUE;
						JSetStringT( hContact, "FullName", n->text );
					}
				}
				else if ( !strcmp( n->name, "NICKNAME" )) {
					if ( n->text != NULL ) {
						hasNick = TRUE;
						JSetStringT( hContact, "Nick", n->text );
					}
				}
				else if ( !strcmp( n->name, "N" )) {
					// First/Last name
					if ( !hasGiven && !hasFamily && !hasMiddle ) {
						if (( m=JabberXmlGetChild( n, "GIVEN" )) != NULL && m->text!=NULL ) {
							hasGiven = TRUE;
							JSetStringT( hContact, "FirstName", m->text );
						}
						if (( m=JabberXmlGetChild( n, "FAMILY" )) != NULL && m->text!=NULL ) {
							hasFamily = TRUE;
							JSetStringT( hContact, "LastName", m->text );
						}
						if (( m=JabberXmlGetChild( n, "MIDDLE" )) != NULL && m->text != NULL ) {
							hasMiddle = TRUE;
							JSetStringT( hContact, "MiddleName", m->text );
					}	}
				}
				else if ( !strcmp( n->name, "EMAIL" )) {
					// E-mail address( es )
					if (( m=JabberXmlGetChild( n, "USERID" )) == NULL )	// Some bad client put e-mail directly in <EMAIL/> instead of <USERID/>
						m = n;
					if ( m->text != NULL ) {
						char text[100];
						if ( hContact != NULL ) {
							if ( nEmail == 0 )
								strcpy( text, "e-mail" );
							else
								sprintf( text, "e-mail%d", nEmail-1 );
						}
						else sprintf( text, "e-mail%d", nEmail );
						JSetStringT( hContact, text, m->text );

						if ( hContact == NULL ) {
							sprintf( text, "e-mailFlag%d", nEmail );
							int nFlag = 0;
							if ( JabberXmlGetChild( n, "HOME" ) != NULL ) nFlag |= JABBER_VCEMAIL_HOME;
							if ( JabberXmlGetChild( n, "WORK" ) != NULL ) nFlag |= JABBER_VCEMAIL_WORK;
							if ( JabberXmlGetChild( n, "INTERNET" ) != NULL ) nFlag |= JABBER_VCEMAIL_INTERNET;
							if ( JabberXmlGetChild( n, "X400" ) != NULL ) nFlag |= JABBER_VCEMAIL_X400;
							JSetWord( NULL, text, nFlag );
						}
						nEmail++;
					}
				}
				else if ( !strcmp( n->name, "BDAY" )) {
					// Birthday
					if ( !hasBday && n->text!=NULL ) {
						if ( hContact != NULL ) {
							if ( _stscanf( n->text, _T("%d-%d-%d"), &nYear, &nMonth, &nDay ) == 3 ) {
								hasBday = TRUE;
								JSetWord( hContact, "BirthYear", ( WORD )nYear );
								JSetByte( hContact, "BirthMonth", ( BYTE ) nMonth );
								JSetByte( hContact, "BirthDay", ( BYTE ) nDay );
							}
						}
						else {
							hasBday = TRUE;
							JSetStringT( NULL, "BirthDate", n->text );
					}	}
				}
				else if ( !lstrcmpA( n->name, "GENDER" )) {
					// Gender
					if ( !hasGender && n->text!=NULL ) {
						if ( hContact != NULL ) {
							if ( n->text[0] && strchr( "mMfF", n->text[0] )!=NULL ) {
								hasGender = TRUE;
								JSetByte( hContact, "Gender", ( BYTE ) toupper( n->text[0] ));
							}
						}
						else {
							hasGender = TRUE;
							JSetStringT( NULL, "GenderString", n->text );
					}	}
				}
				else if ( !strcmp( n->name, "ADR" )) {
					if ( !hasHome && JabberXmlGetChild( n, "HOME" )!=NULL ) {
						// Home address
						hasHome = TRUE;
						if (( m=JabberXmlGetChild( n, "STREET" )) != NULL && m->text != NULL ) {
							hasHomeStreet = TRUE;
							if ( hContact != NULL ) {
								if (( o=JabberXmlGetChild( n, "EXTADR" )) != NULL && o->text != NULL )
									mir_sntprintf( text, SIZEOF( text ), _T("%s\r\n%s"), m->text, o->text );
								else if (( o=JabberXmlGetChild( n, "EXTADD" ))!=NULL && o->text!=NULL )
									mir_sntprintf( text, SIZEOF( text ), _T("%s\r\n%s"), m->text, o->text );
								else
									_tcsncpy( text, m->text, SIZEOF( text ));
								text[SIZEOF(text)-1] = '\0';
								JSetStringT( hContact, "Street", text );
							}
							else {
								JSetStringT( hContact, "Street", m->text );
								if (( m=JabberXmlGetChild( n, "EXTADR" )) == NULL )
									m = JabberXmlGetChild( n, "EXTADD" );
								if ( m!=NULL && m->text!=NULL ) {
									hasHomeStreet2 = TRUE;
									JSetStringT( hContact, "Street2", m->text );
						}	}	}

						if (( m=JabberXmlGetChild( n, "LOCALITY" ))!=NULL && m->text!=NULL ) {
							hasHomeLocality = TRUE;
							JSetStringT( hContact, "City", m->text );
						}
						if (( m=JabberXmlGetChild( n, "REGION" ))!=NULL && m->text!=NULL ) {
							hasHomeRegion = TRUE;
							JSetStringT( hContact, "State", m->text );
						}
						if (( m=JabberXmlGetChild( n, "PCODE" ))!=NULL && m->text!=NULL ) {
							hasHomePcode = TRUE;
							JSetStringT( hContact, "ZIP", m->text );
						}
						if (( m=JabberXmlGetChild( n, "CTRY" ))==NULL || m->text==NULL )	// Some bad client use <COUNTRY/> instead of <CTRY/>
							m = JabberXmlGetChild( n, "COUNTRY" );
						if ( m!=NULL && m->text!=NULL ) {
							hasHomeCtry = TRUE;
							if ( hContact != NULL )
								JSetWord( hContact, "Country", ( WORD )JabberCountryNameToId( m->text ));
							else
								JSetStringT( hContact, "CountryName", m->text );
					}	}

					if ( !hasWork && JabberXmlGetChild( n, "WORK" )!=NULL ) {
						// Work address
						hasWork = TRUE;
						if (( m=JabberXmlGetChild( n, "STREET" ))!=NULL && m->text!=NULL ) {
							hasWorkStreet = TRUE;
							if ( hContact != NULL ) {
								if (( o=JabberXmlGetChild( n, "EXTADR" ))!=NULL && o->text!=NULL )
									mir_sntprintf( text, SIZEOF( text ), _T("%s\r\n%s"), m->text, o->text );
								else if (( o=JabberXmlGetChild( n, "EXTADD" ))!=NULL && o->text!=NULL )
									mir_sntprintf( text, SIZEOF( text ), _T("%s\r\n%s"), m->text, o->text );
								else
									_tcsncpy( text, m->text, SIZEOF( text ));
								text[SIZEOF( text )-1] = '\0';
								JSetStringT( hContact, "CompanyStreet", text );
							}
							else {
								JSetStringT( hContact, "CompanyStreet", m->text );
								if (( m=JabberXmlGetChild( n, "EXTADR" )) == NULL )
									m = JabberXmlGetChild( n, "EXTADD" );
								if ( m!=NULL && m->text!=NULL ) {
									hasWorkStreet2 = TRUE;
									JSetStringT( hContact, "CompanyStreet2", m->text );
						}	}	}

						if (( m=JabberXmlGetChild( n, "LOCALITY" ))!=NULL && m->text!=NULL ) {
							hasWorkLocality = TRUE;
							JSetStringT( hContact, "CompanyCity", m->text );
						}
						if (( m=JabberXmlGetChild( n, "REGION" ))!=NULL && m->text!=NULL ) {
							hasWorkRegion = TRUE;
							JSetStringT( hContact, "CompanyState", m->text );
						}
						if (( m=JabberXmlGetChild( n, "PCODE" ))!=NULL && m->text!=NULL ) {
							hasWorkPcode = TRUE;
							JSetStringT( hContact, "CompanyZIP", m->text );
						}
						if (( m=JabberXmlGetChild( n, "CTRY" ))==NULL || m->text==NULL )	// Some bad client use <COUNTRY/> instead of <CTRY/>
							m = JabberXmlGetChild( n, "COUNTRY" );
						if ( m!=NULL && m->text!=NULL ) {
							hasWorkCtry = TRUE;
							if ( hContact != NULL )
								JSetWord( hContact, "CompanyCountry", ( WORD )JabberCountryNameToId( m->text ));
							else
								JSetStringT( hContact, "CompanyCountryName", m->text );
					}	}
				}
				else if ( !strcmp( n->name, "TEL" )) {
					// Telephone/Fax/Cellular
					if (( m=JabberXmlGetChild( n, "NUMBER" ))!=NULL && m->text!=NULL ) {
						if ( hContact != NULL ) {
							if ( !hasFax && JabberXmlGetChild( n, "FAX" )!=NULL ) {
								hasFax = TRUE;
								JSetStringT( hContact, "Fax", m->text );
							}
							if ( !hasCell && JabberXmlGetChild( n, "CELL" )!=NULL ) {
								hasCell = TRUE;
								JSetStringT( hContact, "Cellular", m->text );
							}
							if ( !hasPhone &&
								( JabberXmlGetChild( n, "HOME" )!=NULL ||
								 JabberXmlGetChild( n, "WORK" )!=NULL ||
								 JabberXmlGetChild( n, "VOICE" )!=NULL ||
								 ( JabberXmlGetChild( n, "FAX" )==NULL &&
								  JabberXmlGetChild( n, "PAGER" )==NULL &&
								  JabberXmlGetChild( n, "MSG" )==NULL &&
								  JabberXmlGetChild( n, "CELL" )==NULL &&
								  JabberXmlGetChild( n, "VIDEO" )==NULL &&
								  JabberXmlGetChild( n, "BBS" )==NULL &&
								  JabberXmlGetChild( n, "MODEM" )==NULL &&
								  JabberXmlGetChild( n, "ISDN" )==NULL &&
								  JabberXmlGetChild( n, "PCS" )==NULL )) ) {
								hasPhone = TRUE;
								JSetStringT( hContact, "Phone", m->text );
							}
						}
						else {
							char text[ 100 ];
							sprintf( text, "Phone%d", nPhone );
							JSetStringT( NULL, text, m->text );

							sprintf( text, "PhoneFlag%d", nPhone );
							int nFlag = 0;
							if ( JabberXmlGetChild( n, "HOME" ) != NULL ) nFlag |= JABBER_VCTEL_HOME;
							if ( JabberXmlGetChild( n, "WORK" ) != NULL ) nFlag |= JABBER_VCTEL_WORK;
							if ( JabberXmlGetChild( n, "VOICE" ) != NULL ) nFlag |= JABBER_VCTEL_VOICE;
							if ( JabberXmlGetChild( n, "FAX" ) != NULL ) nFlag |= JABBER_VCTEL_FAX;
							if ( JabberXmlGetChild( n, "PAGER" ) != NULL ) nFlag |= JABBER_VCTEL_PAGER;
							if ( JabberXmlGetChild( n, "MSG" ) != NULL ) nFlag |= JABBER_VCTEL_MSG;
							if ( JabberXmlGetChild( n, "CELL" ) != NULL ) nFlag |= JABBER_VCTEL_CELL;
							if ( JabberXmlGetChild( n, "VIDEO" ) != NULL ) nFlag |= JABBER_VCTEL_VIDEO;
							if ( JabberXmlGetChild( n, "BBS" ) != NULL ) nFlag |= JABBER_VCTEL_BBS;
							if ( JabberXmlGetChild( n, "MODEM" ) != NULL ) nFlag |= JABBER_VCTEL_MODEM;
							if ( JabberXmlGetChild( n, "ISDN" ) != NULL ) nFlag |= JABBER_VCTEL_ISDN;
							if ( JabberXmlGetChild( n, "PCS" ) != NULL ) nFlag |= JABBER_VCTEL_PCS;
							JSetWord( NULL, text, nFlag );
							nPhone++;
					}	}
				}
				else if ( !strcmp( n->name, "URL" )) {
					// Homepage
					if ( !hasUrl && n->text!=NULL ) {
						hasUrl = TRUE;
						JSetStringT( hContact, "Homepage", n->text );
					}
				}
				else if ( !strcmp( n->name, "ORG" )) {
					if ( !hasOrgname && !hasOrgunit ) {
						if (( m=JabberXmlGetChild( n, "ORGNAME" ))!=NULL && m->text!=NULL ) {
							hasOrgname = TRUE;
							JSetStringT( hContact, "Company", m->text );
						}
						if (( m=JabberXmlGetChild( n, "ORGUNIT" ))!=NULL && m->text!=NULL ) {	// The real vCard can have multiple <ORGUNIT/> but we will only display the first one
							hasOrgunit = TRUE;
							JSetStringT( hContact, "CompanyDepartment", m->text );
					}	}
				}
				else if ( !strcmp( n->name, "ROLE" )) {
					if ( !hasRole && n->text!=NULL ) {
						hasRole = TRUE;
						JSetStringT( hContact, "Role", n->text );
					}
				}
				else if ( !strcmp( n->name, "TITLE" )) {
					if ( !hasTitle && n->text!=NULL ) {
						hasTitle = TRUE;
						JSetStringT( hContact, "CompanyPosition", n->text );
					}
				}
				else if ( !strcmp( n->name, "DESC" )) {
					if ( !hasDesc && n->text!=NULL ) {
						hasDesc = TRUE;
						TCHAR* szMemo = JabberUnixToDosT( n->text );
						JSetStringT( hContact, "About", szMemo );
						mir_free( szMemo );
					}
				}
				else if ( !strcmp( n->name, "PHOTO" ))
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
			TCHAR* p = _tcschr( jid, '@' );
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

void CJabberProto::OnIqResultSetVcard( XmlNode *iqNode, void *userdata )
{
	Log( "<iq/> iqIdSetVcard" );
	TCHAR* type = JabberXmlGetAttrValue( iqNode, "type" );
	if ( type == NULL )
		return;

	WindowNotify(WM_JABBER_REFRESH_VCARD);
}

void CJabberProto::OnIqResultSetSearch( XmlNode *iqNode, void *userdata )
{
	XmlNode *queryNode, *itemNode, *n;
	TCHAR* type, *jid;
	int i, id;
	JABBER_SEARCH_RESULT jsr;

	Log( "<iq/> iqIdGetSearch" );
	if (( type=JabberXmlGetAttrValue( iqNode, "type" )) == NULL ) return;
	if (( id = JabberGetPacketID( iqNode )) == -1 ) return;

	if ( !lstrcmp( type, _T("result"))) {
		if (( queryNode=JabberXmlGetChild( iqNode, "query" )) == NULL ) return;
		jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
		for ( i=0; i<queryNode->numChild; i++ ) {
			itemNode = queryNode->child[i];
			if ( !lstrcmpA( itemNode->name, "item" )) {
				if (( jid=JabberXmlGetAttrValue( itemNode, "jid" )) != NULL ) {
					_tcsncpy( jsr.jid, jid, SIZEOF( jsr.jid ));
					jsr.jid[ SIZEOF( jsr.jid )-1] = '\0';
					Log( "Result jid = " TCHAR_STR_PARAM, jid );
					if (( n=JabberXmlGetChild( itemNode, "nick" ))!=NULL && n->text!=NULL )
						jsr.hdr.nick = mir_t2a( n->text );
					else
						jsr.hdr.nick = mir_strdup( "" );
					if (( n=JabberXmlGetChild( itemNode, "first" ))!=NULL && n->text!=NULL )
						jsr.hdr.firstName = mir_t2a( n->text );
					else
						jsr.hdr.firstName = mir_strdup( "" );
					if (( n=JabberXmlGetChild( itemNode, "last" ))!=NULL && n->text!=NULL )
						jsr.hdr.lastName = mir_t2a( n->text );
					else
						jsr.hdr.lastName = mir_strdup( "" );
					if (( n=JabberXmlGetChild( itemNode, "email" ))!=NULL && n->text!=NULL )
						jsr.hdr.email = mir_t2a( n->text );
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

void CJabberProto::OnIqResultExtSearch( XmlNode *iqNode, void *userdata )
{
	XmlNode *queryNode;
	TCHAR* type;
	int id;

	Log( "<iq/> iqIdGetExtSearch" );
	if (( type=JabberXmlGetAttrValue( iqNode, "type" )) == NULL ) return;
	if (( id = JabberGetPacketID( iqNode )) == -1 ) return;

	if ( !lstrcmp( type, _T("result"))) {
		if (( queryNode=JabberXmlGetChild( iqNode, "query" )) == NULL ) return;
		if (( queryNode=JabberXmlGetChild( queryNode, "x" )) == NULL ) return;
		for ( int i=0; i<queryNode->numChild; i++ ) {
			XmlNode* itemNode = queryNode->child[i];
			if ( strcmp( itemNode->name, "item" ))
				continue;

			JABBER_SEARCH_RESULT jsr = { 0 };
			jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
//			jsr.hdr.firstName = "";

			for ( int j=0; j < itemNode->numChild; j++ ) {
				XmlNode* fieldNode = itemNode->child[j];
				if ( strcmp( fieldNode->name, "field" ))
					continue;

				TCHAR* fieldName = JabberXmlGetAttrValue( fieldNode, "var" );
				if ( fieldName == NULL )
					continue;

				XmlNode* n = JabberXmlGetChild( fieldNode, "value" );
				if ( n == NULL )
					continue;

				if ( !lstrcmp( fieldName, _T("jid"))) {
					_tcsncpy( jsr.jid, n->text, SIZEOF( jsr.jid ));
					jsr.jid[SIZEOF( jsr.jid )-1] = '\0';
					Log( "Result jid = " TCHAR_STR_PARAM, jsr.jid );
				}
				else if ( !lstrcmp( fieldName, _T("nickname")))
					jsr.hdr.nick = ( n->text != NULL ) ? mir_t2a( n->text ) : mir_strdup( "" );
				else if ( !lstrcmp( fieldName, _T("fn"))) {
					mir_free( jsr.hdr.firstName );
					jsr.hdr.firstName = ( n->text != NULL ) ? mir_t2a(n->text) : mir_strdup( "" );
				}
				else if ( !lstrcmp( fieldName, _T("given"))) {
					mir_free( jsr.hdr.firstName );
					jsr.hdr.firstName = ( n->text != NULL ) ? mir_t2a(n->text) : mir_strdup( "" );
				}
				else if ( !lstrcmp( fieldName, _T("family")))
					jsr.hdr.lastName = ( n->text != NULL ) ? mir_t2a(n->text) : mir_strdup( "" );
				else if ( !lstrcmp( fieldName, _T("email")))
					jsr.hdr.email = ( n->text != NULL ) ? mir_t2a(n->text) : mir_strdup( "" );
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

void CJabberProto::OnIqResultSetPassword( XmlNode *iqNode, void *userdata )
{
	Log( "<iq/> iqIdSetPassword" );

	TCHAR* type = JabberXmlGetAttrValue( iqNode, "type" );
	if ( type == NULL )
		return;

	if ( !lstrcmp( type, _T("result"))) {
		strncpy( m_ThreadInfo->password, m_ThreadInfo->newPassword, SIZEOF( m_ThreadInfo->password ));
		MessageBox( NULL, TranslateT( "Password is successfully changed. Don't forget to update your password in the Jabber protocol option." ), TranslateT( "Change Password" ), MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND );
	}
	else if ( !lstrcmp( type, _T("error")))
		MessageBox( NULL, TranslateT( "Password cannot be changed." ), TranslateT( "Change Password" ), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND );
}

void CJabberProto::OnIqResultGetAvatar( XmlNode *iqNode, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;
	TCHAR* type;

	Log( "<iq/> iqIdResultGetAvatar" );
	if (( type=JabberXmlGetAttrValue( iqNode, "type" )) == NULL )   return;
	if ( _tcscmp( type, _T("result")))                              return;

	TCHAR* from = JabberXmlGetAttrValue( iqNode, "from" );
	if ( from == NULL )
		return;
	HANDLE hContact = HContactFromJID( from );
	if ( hContact == NULL )
		return;
	XmlNode* n = NULL;
	TCHAR* mimeType = NULL;
	if ( JGetByte( hContact, "AvatarXVcard", 0 )) {
		XmlNode *vCard = JabberXmlGetChild( iqNode, "vCard" );
		if (vCard == NULL) return;
		vCard = JabberXmlGetChild( vCard, "PHOTO" );
		if (vCard == NULL) return;
		XmlNode *typeNode = JabberXmlGetChild( vCard, "TYPE" );
		if (typeNode != NULL) mimeType = typeNode->text;
		n = JabberXmlGetChild( vCard, "BINVAL" );
	}
	else {
		XmlNode *queryNode = JabberXmlGetChild( iqNode, "query" );
		if ( queryNode == NULL )
			return;

		TCHAR* xmlns = JabberXmlGetAttrValue( queryNode, "xmlns" );
		if ( lstrcmp( xmlns, _T(JABBER_FEAT_AVATAR)))
			return;

		n = JabberXmlGetChild( queryNode, "data" );
		if ( n )
			mimeType = JabberXmlGetAttrValue( n, "mimetype" );
	}
	if ( n == NULL )
		return;

	int resultLen = 0;
	char* body = JabberBase64Decode( n->text, &resultLen );

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

void CJabberProto::OnIqResultDiscoBookmarks( XmlNode *iqNode, void *userdata )
{
	ThreadData* info = ( ThreadData* ) userdata;
	XmlNode *queryNode, *itemNode, *storageNode, *nickNode, *passNode;
	TCHAR* type, *jid;

	// RECVED: list of bookmarks
	// ACTION: refresh bookmarks dialog
	Log( "<iq/> iqIdGetBookmarks" );
	if (( type=JabberXmlGetAttrValue( iqNode, "type" )) == NULL ) return;

	if ( !lstrcmp( type, _T("result"))) {
		if ( m_ThreadInfo && !( m_ThreadInfo->jabberServerCaps & JABBER_CAPS_PRIVATE_STORAGE )) {
			m_ThreadInfo->jabberServerCaps |= JABBER_CAPS_PRIVATE_STORAGE;
			EnableMenuItems( TRUE );
		}

		if (( queryNode = JabberXmlGetChild( iqNode, "query" )) != NULL ) {
			if ((storageNode = JabberXmlGetChild( queryNode, "storage" )) != NULL) {
				ListRemoveList( LIST_BOOKMARK );
				for ( int i=0; i<storageNode->numChild; i++ ) {
					if (( itemNode = storageNode->child[i] ) != NULL && itemNode->name != NULL) {
						if ( !strcmp( itemNode->name, "conference" )) {
							if (( jid = JabberXmlGetAttrValue( itemNode, "jid" )) != NULL ) {
								JABBER_LIST_ITEM* item = ListAdd( LIST_BOOKMARK, jid );
								item->name = mir_tstrdup( JabberXmlGetAttrValue( itemNode, "name" ));
								item->type = mir_tstrdup( _T( "conference" ));
								item->bUseResource = TRUE;
								if ( JabberXmlGetAttrValue( itemNode, "autojoin" ) != NULL ) {
									TCHAR* autoJ = JabberXmlGetAttrValue( itemNode, "autojoin" );
									item->bAutoJoin = ( !lstrcmp( autoJ, _T("true")) || !lstrcmp( autoJ, _T("1"))) ? true : false;
								}
								if (( nickNode = JabberXmlGetChild( itemNode, "nick" )) != NULL && nickNode->text != NULL )
									replaceStr( item->nick, nickNode->text );
								if (( passNode = JabberXmlGetChild( itemNode, "password" )) != NULL && passNode->text != NULL )
									replaceStr( item->password, passNode->text );
						}	}

						if ( !strcmp( itemNode->name, "url" )) {
							if (( jid = JabberXmlGetAttrValue( itemNode, "url" )) != NULL ) {
								JABBER_LIST_ITEM* item = ListAdd( LIST_BOOKMARK, jid );
								item->bUseResource = TRUE;
								item->name = mir_tstrdup( JabberXmlGetAttrValue( itemNode, "name" ));
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
	XmlNode* query = iq.addQuery( JABBER_FEAT_PRIVATE_STORAGE );
	XmlNode* storage = query->addChild( "storage" );
	storage->addAttr( "xmlns", "storage:bookmarks" );

	for ( int i=0; ( i=ListFindNext( LIST_BOOKMARK, i )) >= 0; i++ ) {
		JABBER_LIST_ITEM* item = ListGetItemPtrFromIndex( i );
		if ( item == NULL )
			continue;

		if ( item->jid == NULL )
			continue;
		if (!lstrcmp( item->type, _T("conference") )) {
			XmlNode* itemNode = storage->addChild("conference");
			itemNode->addAttr( "jid", item->jid );
			if ( item->name )
				itemNode->addAttr( "name", item->name );
			if ( item->bAutoJoin )
				itemNode->addAttr( "autojoin", _T("1") );
			if ( item->nick )
				itemNode->addChild( "nick", item->nick );
			if ( item->password )
				itemNode->addChild( "password", item->password );
		}
		if (!lstrcmp( item->type, _T("url") )) {
			XmlNode* itemNode = storage->addChild("url");
			itemNode->addAttr( "url", item->jid );
			if ( item->name )
				itemNode->addAttr( "name", item->name );
		}
	}
}

void CJabberProto::OnIqResultSetBookmarks( XmlNode *iqNode, void *userdata )
{
	// RECVED: server's response
	// ACTION: refresh bookmarks list dialog

	Log( "<iq/> iqIdSetBookmarks" );

	TCHAR* type = JabberXmlGetAttrValue( iqNode, "type" );
	if ( type == NULL )
		return;

	if ( !lstrcmp( type, _T("result"))) {
		UI_SAFE_NOTIFY(m_pDlgBookmarks, WM_JABBER_REFRESH);
	}
	else if ( !lstrcmp( type, _T("error"))) {
		XmlNode* errorNode = JabberXmlGetChild( iqNode, "error" );
		TCHAR* str = JabberErrorMsg( errorNode );
		MessageBox( NULL, str, TranslateT( "Jabber Bookmarks Error" ), MB_OK|MB_SETFOREGROUND );
		mir_free( str );
		UI_SAFE_NOTIFY(m_pDlgBookmarks, WM_JABBER_ACTIVATE);
}	}

// last activity (XEP-0012) support
void CJabberProto::OnIqResultLastActivity( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo )
{
	JABBER_RESOURCE_STATUS *r = ResourceInfoFromJID( pInfo->m_szFrom );
	if ( !r )
		return;

	time_t lastActivity = -1;
	if ( pInfo->m_nIqType == JABBER_IQ_TYPE_RESULT ) {
		XmlNode *queryNode = JabberXmlGetChild( iqNode, "query" );
		if ( queryNode ) {
			TCHAR *seconds = JabberXmlGetAttrValue( queryNode, "seconds" );
			if ( seconds ) {
				int nSeconds = _ttoi( seconds );
				if ( nSeconds > 0 )
					lastActivity = time( 0 ) - nSeconds;
			}
			if ( queryNode->text && *queryNode->text )
				replaceStr( r->statusMessage, queryNode->text );
		}
	}

	r->idleStartTime = lastActivity;

	JabberUserInfoUpdate(pInfo->GetHContact());
}

// entity time (XEP-0202) support
void CJabberProto::OnIqResultEntityTime( XmlNode* pIqNode, void* pUserdata, CJabberIqInfo* pInfo )
{
	if ( !pInfo->m_hContact )
		return;

	if ( pInfo->m_nIqType == JABBER_IQ_TYPE_RESULT ) {
		XmlNode* pTimeNode = JabberXmlGetChildWithGivenAttrValue(pIqNode, "time", "xmlns", _T( JABBER_FEAT_ENTITY_TIME ));
		if ( pTimeNode ) {
			XmlNode* pTzo = JabberXmlGetChild(pTimeNode, "tzo");
			if ( pTzo ) {
				if ( _tcslen( pTzo->text ) == 6 ) { // +00:00
					bool bNegative = pTzo->text[0] == _T('-');
					int nTz = ( _ttoi( pTzo->text+1 ) * 60 + _ttoi( pTzo->text + 4 )) / 30;

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
