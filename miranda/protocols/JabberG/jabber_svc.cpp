/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-11  George Hazan
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

Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"

#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "jabber_list.h"
#include "jabber_iq.h"
#include "jabber_caps.h"
#include "m_file.h"
#include "m_addcontact.h"
#include "jabber_disco.h"
#include "sdk/m_proto_listeningto.h"

/////////////////////////////////////////////////////////////////////////////////////////
// GetMyAwayMsg - obtain the current away message

INT_PTR __cdecl CJabberProto::GetMyAwayMsg(WPARAM wParam, LPARAM lParam)
{
	TCHAR *szStatus = NULL;
	INT_PTR nRetVal = 0;

	EnterCriticalSection( &m_csModeMsgMutex );
	switch ( wParam ? (int)wParam : m_iStatus ) {
	case ID_STATUS_ONLINE:
		szStatus = m_modeMsgs.szOnline;
		break;
	case ID_STATUS_AWAY:
	case ID_STATUS_ONTHEPHONE:
	case ID_STATUS_OUTTOLUNCH:
		szStatus = m_modeMsgs.szAway;
		break;
	case ID_STATUS_NA:
		szStatus = m_modeMsgs.szNa;
		break;
	case ID_STATUS_DND:
	case ID_STATUS_OCCUPIED:
		szStatus = m_modeMsgs.szDnd;
		break;
	case ID_STATUS_FREECHAT:
		szStatus = m_modeMsgs.szFreechat;
		break;
	default:
		// Should not reach here
		break;
	}
	if ( szStatus ) 
		nRetVal = ( lParam & SGMA_UNICODE ) ? ( INT_PTR )mir_t2u( szStatus ) : ( INT_PTR )mir_t2a( szStatus );
	LeaveCriticalSection( &m_csModeMsgMutex );

	return nRetVal;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetAvatar - retrieves the file name of my own avatar

INT_PTR __cdecl CJabberProto::JabberGetAvatar( WPARAM wParam, LPARAM lParam )
{
	TCHAR* buf = ( TCHAR* )wParam;
	int  size = ( int )lParam;

	if ( buf == NULL || size <= 0 )
		return -1;

	if ( !m_options.EnableAvatars )
		return -2;

	GetAvatarFileName( NULL, buf, size );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetAvatarCaps - returns directives how to process avatars

INT_PTR __cdecl CJabberProto::JabberGetAvatarCaps( WPARAM wParam, LPARAM lParam )
{
	switch( wParam ) {
	case AF_MAXSIZE:
		{
			POINT* size = (POINT*)lParam;
			if ( size )
				size->x = size->y = 96;
		}
      return 0;

	case AF_PROPORTION:
		return PIP_NONE;

	case AF_FORMATSUPPORTED: // Jabber supports avatars of virtually all formats
		return 1;

	case AF_ENABLED:
		return m_options.EnableAvatars;
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetAvatarInfo - retrieves the avatar info

INT_PTR __cdecl CJabberProto::JabberGetAvatarInfo( WPARAM wParam, LPARAM lParam )
{
	if ( !m_options.EnableAvatars )
		return GAIR_NOAVATAR;

	PROTO_AVATAR_INFORMATIONT* AI = ( PROTO_AVATAR_INFORMATIONT* )lParam;

	char szHashValue[ MAX_PATH ];
	if ( JGetStaticString( "AvatarHash", AI->hContact, szHashValue, sizeof szHashValue )) {
		Log( "No avatar" );
		return GAIR_NOAVATAR;
	}

	TCHAR tszFileName[ MAX_PATH ];
	GetAvatarFileName( AI->hContact, tszFileName, SIZEOF(tszFileName));
	_tcsncpy( AI->filename, tszFileName, SIZEOF(AI->filename));

	AI->format = ( AI->hContact == NULL ) ? PA_FORMAT_PNG : JGetByte( AI->hContact, "AvatarType", 0 );

	if ( ::_taccess( AI->filename, 0 ) == 0 ) {
		char szSavedHash[ 256 ];
		if ( !JGetStaticString( "AvatarSaved", AI->hContact, szSavedHash, sizeof szSavedHash )) {
			if ( !strcmp( szSavedHash, szHashValue )) {
				Log( "Avatar is Ok: %s == %s", szSavedHash, szHashValue );
				return GAIR_SUCCESS;
	}	}	}

	if (( wParam & GAIF_FORCE ) != 0 && AI->hContact != NULL && m_bJabberOnline ) {
		DBVARIANT dbv;
		if ( !JGetStringT( AI->hContact, "jid", &dbv )) {
			JABBER_LIST_ITEM* item = ListGetItemPtr( LIST_ROSTER, dbv.ptszVal );
			if ( item != NULL ) {
				BOOL isXVcard = JGetByte( AI->hContact, "AvatarXVcard", 0 );

				TCHAR szJid[ JABBER_MAX_JID_LEN ];
				if ( item->resourceCount != NULL && !isXVcard ) {
					TCHAR *bestResName = ListGetBestClientResourceNamePtr(dbv.ptszVal);
					mir_sntprintf( szJid, SIZEOF( szJid ), bestResName?_T("%s/%s"):_T("%s"), dbv.ptszVal, bestResName );
				}
				else lstrcpyn( szJid, dbv.ptszVal, SIZEOF( szJid ));

				Log( "Rereading %s for " TCHAR_STR_PARAM, isXVcard ? JABBER_FEAT_VCARD_TEMP : JABBER_FEAT_AVATAR, szJid );

				int iqId = SerialNext();
				if ( isXVcard )
					IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultGetVCardAvatar );
				else
					IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultGetClientAvatar );

				XmlNodeIq iq( _T("get"), iqId, szJid );
				if ( isXVcard )
					iq << XCHILDNS( _T("vCard"), _T(JABBER_FEAT_VCARD_TEMP));
				else
					iq << XQUERY( isXVcard ? _T("") : _T(JABBER_FEAT_AVATAR));
				m_ThreadInfo->send( iq );

				JFreeVariant( &dbv );
				return GAIR_WAITFOR;
			}
			JFreeVariant( &dbv );
		}
	}

	Log( "No avatar" );
	return GAIR_NOAVATAR;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberGetEventTextChatStates - retrieves a chat state description from an event

INT_PTR __cdecl CJabberProto::OnGetEventTextChatStates( WPARAM, LPARAM lParam )
{
	DBEVENTGETTEXT *pdbEvent = ( DBEVENTGETTEXT * )lParam;

	INT_PTR nRetVal = 0;

	if ( pdbEvent->dbei->cbBlob > 0 ) {
		if ( pdbEvent->dbei->pBlob[0] == JABBER_DB_EVENT_CHATSTATES_GONE ) {
			if ( pdbEvent->datatype == DBVT_WCHAR )
				nRetVal = (INT_PTR)mir_tstrdup(TranslateTS(_T("closed chat session")));
			else if ( pdbEvent->datatype == DBVT_ASCIIZ )
				nRetVal = (INT_PTR)mir_strdup(Translate("closed chat session"));
		}
	}
	
	return nRetVal;
}

////////////////////////////////////////////////////////////////////////////////////////
// OnGetEventTextPresence - retrieves presence state description from an event

INT_PTR __cdecl CJabberProto::OnGetEventTextPresence( WPARAM, LPARAM lParam )
{
	DBEVENTGETTEXT *pdbEvent = ( DBEVENTGETTEXT * )lParam;

	INT_PTR nRetVal = 0;

	if ( pdbEvent->dbei->cbBlob > 0 ) {
		switch ( pdbEvent->dbei->pBlob[0] )
		{
		case JABBER_DB_EVENT_PRESENCE_SUBSCRIBE:
			{
				if ( pdbEvent->datatype == DBVT_WCHAR )
					nRetVal = (INT_PTR)mir_tstrdup(TranslateTS(_T("sent subscription request")));
				else if ( pdbEvent->datatype == DBVT_ASCIIZ )
					nRetVal = (INT_PTR)mir_strdup(Translate("sent subscription request"));
				break;
			}
		case JABBER_DB_EVENT_PRESENCE_SUBSCRIBED:
			{
				if ( pdbEvent->datatype == DBVT_WCHAR )
					nRetVal = (INT_PTR)mir_tstrdup(TranslateTS(_T("approved subscription request")));
				else if ( pdbEvent->datatype == DBVT_ASCIIZ )
					nRetVal = (INT_PTR)mir_strdup(Translate("approved subscription request"));
				break;
			}
		case JABBER_DB_EVENT_PRESENCE_UNSUBSCRIBE:
			{
				if ( pdbEvent->datatype == DBVT_WCHAR )
					nRetVal = (INT_PTR)mir_tstrdup(TranslateTS(_T("declined subscription")));
				else if ( pdbEvent->datatype == DBVT_ASCIIZ )
					nRetVal = (INT_PTR)mir_strdup(Translate("declined subscription"));
				break;
			}
		case JABBER_DB_EVENT_PRESENCE_UNSUBSCRIBED:
			{
				if ( pdbEvent->datatype == DBVT_WCHAR )
					nRetVal = (INT_PTR)mir_tstrdup(TranslateTS(_T("declined subscription")));
				else if ( pdbEvent->datatype == DBVT_ASCIIZ )
					nRetVal = (INT_PTR)mir_strdup(Translate("declined subscription"));
				break;
			}
		case JABBER_DB_EVENT_PRESENCE_ERROR:
			{
				if ( pdbEvent->datatype == DBVT_WCHAR )
					nRetVal = (INT_PTR)mir_tstrdup(TranslateTS(_T("sent error presence")));
				else if ( pdbEvent->datatype == DBVT_ASCIIZ )
					nRetVal = (INT_PTR)mir_strdup(Translate("sent error presence"));
				break;
			}
		default:
			{
				if ( pdbEvent->datatype == DBVT_WCHAR )
					nRetVal = (INT_PTR)mir_tstrdup(TranslateTS(_T("sent unknown presence type")));
				else if ( pdbEvent->datatype == DBVT_ASCIIZ )
					nRetVal = (INT_PTR)mir_strdup(Translate("sent unknown presence type"));
				break;
			}
		}
	}

	return nRetVal;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberSetAvatar - sets an avatar without UI

INT_PTR __cdecl CJabberProto::JabberSetAvatar( WPARAM, LPARAM lParam )
{
	TCHAR* tszFileName = ( TCHAR* )lParam;

	if ( m_bJabberOnline ) {
		SetServerVcard( TRUE, tszFileName );
		SendPresence( m_iDesiredStatus, false );
	}
	else if ( tszFileName == NULL || tszFileName[0] == 0 ) {
		// Remove avatar
		TCHAR tFileName[ MAX_PATH ];
		GetAvatarFileName( NULL, tFileName, MAX_PATH );
		DeleteFile( tFileName );

		JDeleteSetting( NULL, "AvatarSaved" );
		JDeleteSetting( NULL, "AvatarHash" );
	}
	else {
		int fileIn = _topen( tszFileName, O_RDWR | O_BINARY, S_IREAD | S_IWRITE );
		if ( fileIn == -1 ) {
			mir_free(tszFileName);
			return 1;
		}

		long  dwPngSize = _filelength( fileIn );
		char* pResult = new char[ dwPngSize ];
		if ( pResult == NULL ) {
			_close( fileIn );
			mir_free(tszFileName);
			return 2;
		}

		_read( fileIn, pResult, dwPngSize );
		_close( fileIn );

		mir_sha1_byte_t digest[MIR_SHA1_HASH_SIZE];
		mir_sha1_ctx sha1ctx;
		mir_sha1_init( &sha1ctx );
		mir_sha1_append( &sha1ctx, (mir_sha1_byte_t*)pResult, dwPngSize );
		mir_sha1_finish( &sha1ctx, digest );

		TCHAR tFileName[ MAX_PATH ];
		GetAvatarFileName( NULL, tFileName, MAX_PATH );
		DeleteFile( tFileName );

		char buf[MIR_SHA1_HASH_SIZE*2+1];
		for ( int i=0; i<MIR_SHA1_HASH_SIZE; i++ )
			sprintf( buf+( i<<1 ), "%02x", digest[i] );

		m_options.AvatarType = JabberGetPictureType( pResult );

		GetAvatarFileName( NULL, tFileName, MAX_PATH );
		FILE* out = _tfopen( tFileName, _T("wb"));
		if ( out != NULL ) {
			fwrite( pResult, dwPngSize, 1, out );
			fclose( out );
		}
		delete[] pResult;

		JSetString( NULL, "AvatarSaved", buf );
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberSetNickname - sets the user nickname without UI

INT_PTR __cdecl CJabberProto::JabberSetNickname( WPARAM wParam, LPARAM lParam )
{
	TCHAR *nickname = ( wParam & SMNN_UNICODE ) ? mir_u2t( (WCHAR *) lParam ) : mir_a2t( (char *) lParam );

	JSetStringT( NULL, "Nick", nickname );
	SetServerVcard( FALSE, _T(""));
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// "/SendXML" - Allows external plugins to send XML to the server

INT_PTR __cdecl CJabberProto::ServiceSendXML( WPARAM, LPARAM lParam )
{
	return m_ThreadInfo->send( (char*)lParam);
}

INT_PTR __cdecl CJabberProto::JabberGCGetToolTipText( WPARAM wParam, LPARAM lParam )
{
	if ( !wParam || !lParam )
		return 0; //room global tooltip not supported yet

	JABBER_LIST_ITEM* item = ListGetItemPtr( LIST_CHATROOM, (TCHAR*)wParam);
	if ( item == NULL )
		return 0;  //no room found

	JABBER_RESOURCE_STATUS * info = NULL;
	for ( int i=0; i < item->resourceCount; i++ ) {
		JABBER_RESOURCE_STATUS& p = item->resource[i];
		if ( !lstrcmp( p.resourceName, (TCHAR*)lParam )) {
			info = &p;
			break;
	}	}

	if ( info == NULL )
		return 0; //no info found

	// ok process info output will be:
	// JID:			real@jid/resource or
	// Nick:		Nickname
	// Status:		StatusText
	// Role:		Moderator
	// Affiliation:  Affiliation

	TCHAR outBuf[2048];
	outBuf[0]=_T('\0');

	const TCHAR * szSeparator= (IsWinVerMEPlus()) ? _T("\r\n") : _T(" | ");

	static const TCHAR * JabberEnum2AffilationStr[]={ _T("None"), _T("Outcast"), _T("Member"), _T("Admin"), _T("Owner") };

	static const TCHAR * JabberEnum2RoleStr[]={ _T("None"), _T("Visitor"), _T("Participant"), _T("Moderator") };

	//FIXME Table conversion fast but is not safe
	static const TCHAR * JabberEnum2StatusStr[]= {	_T("Offline"), _T("Online"), _T("Away"), _T("DND"),
		_T("NA"), _T("Occupied"), _T("Free for chat"),
		_T("Invisible"), _T("On the phone"), _T("Out to lunch"),
		_T("Idle")  };


	//JID:
	if ( _tcschr(info->resourceName, _T('@') ) != NULL ) {
		_tcsncat( outBuf, TranslateT("JID:\t\t"), SIZEOF(outBuf) );
		_tcsncat( outBuf, info->resourceName, SIZEOF(outBuf) );
	} else if (lParam) { //or simple nick
		_tcsncat( outBuf, TranslateT("Nick:\t\t"), SIZEOF(outBuf) );
		_tcsncat( outBuf, (TCHAR*) lParam, SIZEOF(outBuf) );
	}

	// status
	if ( info->status >= ID_STATUS_OFFLINE && info->status <= ID_STATUS_IDLE  ) {
		_tcsncat( outBuf, szSeparator, SIZEOF(outBuf) );
		_tcsncat( outBuf, TranslateT("Status:\t\t"), SIZEOF(outBuf) );
		_tcsncat( outBuf, TranslateTS( JabberEnum2StatusStr [ info->status-ID_STATUS_OFFLINE ]), SIZEOF(outBuf) );
	}

	// status text
	if ( info->statusMessage ) {
		_tcsncat( outBuf, szSeparator, SIZEOF(outBuf) );
		_tcsncat( outBuf, TranslateT("Status text:\t"), SIZEOF(outBuf) );
		_tcsncat( outBuf, info->statusMessage, SIZEOF(outBuf) );
	}

	// Role???
	//if ( TRUE || info->role ) {
		_tcsncat( outBuf, szSeparator, SIZEOF(outBuf) );
		_tcsncat( outBuf, TranslateT("Role:\t\t"), SIZEOF(outBuf) );
		_tcsncat( outBuf, TranslateTS( JabberEnum2RoleStr[info->role] ), SIZEOF(outBuf) );
	//}

	// Affiliation
	//if ( TRUE || info->affiliation ) {
		_tcsncat( outBuf, szSeparator, SIZEOF(outBuf) );
		_tcsncat( outBuf, TranslateT("Affiliation:\t"), SIZEOF(outBuf) );
		_tcsncat( outBuf, TranslateTS( JabberEnum2AffilationStr[info->affiliation] ), SIZEOF(outBuf) );
	//}

	// real jid
	if ( info->szRealJid ) {
		_tcsncat( outBuf, szSeparator, SIZEOF(outBuf) );
		_tcsncat( outBuf, TranslateT("Real JID:\t"), SIZEOF(outBuf) );
		_tcsncat( outBuf, info->szRealJid, SIZEOF(outBuf) );
	}

	if ( lstrlen( outBuf ) == 0)
		return 0;

	return (INT_PTR) mir_tstrdup( outBuf );
}

// File Association Manager plugin support

INT_PTR __cdecl CJabberProto::JabberServiceParseXmppURI( WPARAM wParam, LPARAM lParam )
{
	UNREFERENCED_PARAMETER( wParam );

	TCHAR *arg = ( TCHAR * )lParam;
	if ( arg == NULL )
		return 1;

	TCHAR szUri[ 1024 ];
	mir_sntprintf( szUri, SIZEOF(szUri), _T("%s"), arg );

	TCHAR *szJid = szUri;

	// skip leading prefix
	szJid = _tcschr( szJid, _T( ':' ));
	if ( szJid == NULL )
		return 1;

	// skip //
	for( ++szJid; *szJid == _T( '/' ); ++szJid );

	// empty jid?
	if ( !*szJid )
		return 1;

	// command code
	TCHAR *szCommand = szJid;
	szCommand = _tcschr( szCommand, _T( '?' ));
	if ( szCommand ) 
		*( szCommand++ ) = 0;

	// parameters
	TCHAR *szSecondParam = szCommand ? _tcschr( szCommand, _T( ';' )) : NULL;
	if ( szSecondParam )
		*( szSecondParam++ ) = 0;

//	TCHAR *szThirdParam = szSecondParam ? _tcschr( szSecondParam, _T( ';' )) : NULL;
//	if ( szThirdParam )
//		*( szThirdParam++ ) = 0;

	// no command or message command
	if ( !szCommand || ( szCommand && !_tcsicmp( szCommand, _T( "message" )))) {
		// message
		if ( ServiceExists( MS_MSG_SENDMESSAGE )) {
			HANDLE hContact = HContactFromJID( szJid, TRUE );
            TCHAR *szMsgBody = NULL;
			if ( !hContact )
				hContact = DBCreateContact( szJid, szJid, TRUE, TRUE );
			if ( !hContact )
				return 1;

			if ( szSecondParam ) { //there are parameters to message
				szMsgBody = _tcsstr(szSecondParam, _T( "body=" ));
				if ( szMsgBody ) {
					szMsgBody += 5;
					TCHAR* szDelim = _tcschr( szMsgBody, _T( ';' )); 
					if ( szDelim )
						szDelim = 0;
					JabberHttpUrlDecode( szMsgBody );
			}	}
			#if defined(_UNICODE)
				CallService(MS_MSG_SENDMESSAGE "W",(WPARAM)hContact, (LPARAM)szMsgBody);
			#else
				CallService( MS_MSG_SENDMESSAGE, (WPARAM)hContact, (LPARAM)szMsgBody );
			#endif

			return 0;
		}
		return 1;
	}
	else if ( !_tcsicmp( szCommand, _T( "roster" )))
	{
		if ( !HContactFromJID( szJid )) {
			JABBER_SEARCH_RESULT jsr = { 0 };
			jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
			jsr.hdr.flags = PSR_TCHAR;
			jsr.hdr.nick = szJid;
			jsr.hdr.id = szJid;
			_tcsncpy( jsr.jid, szJid, SIZEOF(jsr.jid) - 1 );

			ADDCONTACTSTRUCT acs;
			acs.handleType = HANDLE_SEARCHRESULT;
			acs.szProto = m_szModuleName;
			acs.psr = &jsr.hdr;
			CallService( MS_ADDCONTACT_SHOW, (WPARAM)NULL, (LPARAM)&acs );
		}
		return 0;
	}
	else if ( !_tcsicmp( szCommand, _T( "join" )))
	{
		// chat join invitation
		GroupchatJoinRoomByJid( NULL, szJid );
		return 0;
	}
	else if ( !_tcsicmp( szCommand, _T( "disco" )))
	{
		// service discovery request
		OnMenuHandleServiceDiscovery( 0, ( LPARAM )szJid );
		return 0;
	}
	else if ( !_tcsicmp( szCommand, _T( "command" )))
	{
		// ad-hoc commands
		if ( szSecondParam ) {
			if ( !_tcsnicmp( szSecondParam, _T( "node=" ), 5 )) {
				szSecondParam += 5;
				if (!*szSecondParam)
					szSecondParam = NULL;
			}
			else
				szSecondParam = NULL;
		}
		CJabberAdhocStartupParams* pStartupParams = new CJabberAdhocStartupParams( this, szJid, szSecondParam );
		ContactMenuRunCommands( 0, ( LPARAM )pStartupParams );
		return 0;
	}
	else if ( !_tcsicmp( szCommand, _T( "sendfile" )))
	{
		// send file
		if ( ServiceExists( MS_FILE_SENDFILE )) {
			HANDLE hContact = HContactFromJID( szJid, TRUE );
			if ( !hContact )
				hContact = DBCreateContact( szJid, szJid, TRUE, TRUE );
			if ( !hContact )
				return 1;
			CallService( MS_FILE_SENDFILE, ( WPARAM )hContact, ( LPARAM )NULL );
			return 0;
		}
		return 1;
	}

	return 1; /* parse failed */
}

// XEP-0224 support (Attention/Nudge)
INT_PTR __cdecl CJabberProto::JabberSendNudge( WPARAM wParam, LPARAM )
{
	if ( !m_bJabberOnline )
		return 0;

	HANDLE hContact = ( HANDLE )wParam;
	DBVARIANT dbv;
	if ( !JGetStringT( hContact, "jid", &dbv )) {
		TCHAR tszJid[ JABBER_MAX_JID_LEN ];
		TCHAR *szResource = ListGetBestClientResourceNamePtr( dbv.ptszVal );
		if ( szResource )
			mir_sntprintf( tszJid, SIZEOF(tszJid), _T("%s/%s"), dbv.ptszVal, szResource );
		else
			mir_sntprintf( tszJid, SIZEOF(tszJid), _T("%s"), dbv.ptszVal );
		JFreeVariant( &dbv );

		JabberCapsBits jcb = GetResourceCapabilites( tszJid, FALSE );

		m_ThreadInfo->send(
			XmlNode( _T("message")) << XATTR( _T("type"), _T("headline")) << XATTR( _T("to"), tszJid )
				<< XCHILDNS( _T("attention"),
				jcb & JABBER_CAPS_ATTENTION ? _T(JABBER_FEAT_ATTENTION) : _T(JABBER_FEAT_ATTENTION_0 )) );
	}
	return 0;
}

BOOL CJabberProto::SendHttpAuthReply( CJabberHttpAuthParams *pParams, BOOL bAuthorized )
{
	if ( !m_bJabberOnline || !pParams || !m_ThreadInfo )
		return FALSE;

	if ( pParams->m_nType == CJabberHttpAuthParams::IQ ) {
		XmlNodeIq iq( bAuthorized ? _T("result") : _T("error"), pParams->m_szIqId, pParams->m_szFrom );
		if ( !bAuthorized ) {
			iq << XCHILDNS( _T("confirm"), _T(JABBER_FEAT_HTTP_AUTH)) << XATTR( _T("id"), pParams->m_szId )
					<< XATTR( _T("method"), pParams->m_szMethod ) << XATTR( _T("url"), pParams->m_szUrl );
			iq << XCHILD( _T("error")) << XATTRI( _T("code"), 401 ) << XATTR( _T("type"), _T("auth"))
					<< XCHILDNS( _T("not-authorized"), _T("urn:ietf:params:xml:xmpp-stanzas"));
		}
		m_ThreadInfo->send( iq );
	}
	else if ( pParams->m_nType == CJabberHttpAuthParams::MSG ) {
		XmlNode msg( _T("message"));
		msg << XATTR( _T("to"), pParams->m_szFrom );
		if ( !bAuthorized )
			msg << XATTR( _T("type"), _T("error"));
		if ( pParams->m_szThreadId )
			msg << XCHILD( _T("thread"), pParams->m_szThreadId );

		msg << XCHILDNS( _T("confirm"), _T(JABBER_FEAT_HTTP_AUTH)) << XATTR( _T("id"), pParams->m_szId )
					<< XATTR( _T("method"), pParams->m_szMethod ) << XATTR( _T("url"), pParams->m_szUrl );

		if ( !bAuthorized )
			msg << XCHILD( _T("error")) << XATTRI( _T("code"), 401 ) << XATTR( _T("type"), _T("auth"))
					<< XCHILDNS( _T("not-authorized"), _T("urn:ietf:params:xml:xmpp-stanzas"));

		m_ThreadInfo->send( msg );
	}
	else
		return FALSE;


	return TRUE;
}

class CJabberDlgHttpAuth: public CJabberDlgBase
{
	typedef CJabberDlgBase CSuper;

public:
	CJabberDlgHttpAuth(CJabberProto *proto, HWND hwndParent, CJabberHttpAuthParams *pParams):
		CSuper(proto, IDD_HTTP_AUTH, hwndParent, true),
		m_txtInfo(this, IDC_EDIT_HTTP_AUTH_INFO),
		m_btnAuth(this, IDOK),
		m_btnDeny(this, IDCANCEL),
		m_pParams(pParams)
	{
		m_btnAuth.OnClick = Callback( this, &CJabberDlgHttpAuth::btnAuth_OnClick );
		m_btnDeny.OnClick = Callback( this, &CJabberDlgHttpAuth::btnDeny_OnClick );
	}

	void OnInitDialog()
	{
		CSuper::OnInitDialog();

		WindowSetIcon( m_hwnd, m_proto, "openid" );

		SetDlgItemText(m_hwnd, IDC_TXT_URL, m_pParams->m_szUrl);
		SetDlgItemText(m_hwnd, IDC_TXT_FROM, m_pParams->m_szFrom);
		SetDlgItemText(m_hwnd, IDC_TXT_ID, m_pParams->m_szId);
		SetDlgItemText(m_hwnd, IDC_TXT_METHOD, m_pParams->m_szMethod);
	}

	BOOL SendReply( BOOL bAuthorized )
	{
		BOOL bRetVal = m_proto->SendHttpAuthReply( m_pParams, bAuthorized );
		m_pParams->Free();
		mir_free( m_pParams );
		m_pParams = NULL;
		return bRetVal;
	}

	void btnAuth_OnClick(CCtrlButton*)
	{
		SendReply( TRUE );
		Close();
	}
	void btnDeny_OnClick(CCtrlButton*)
	{
		SendReply( FALSE );
		Close();
	}

	UI_MESSAGE_MAP(CJabberDlgHttpAuth, CSuper);
		UI_MESSAGE(WM_CTLCOLORSTATIC, OnCtlColorStatic);
	UI_MESSAGE_MAP_END();

	INT_PTR OnCtlColorStatic(UINT, WPARAM, LPARAM)
	{
		return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
	}

private:
	CCtrlEdit	m_txtInfo;
	CCtrlButton	m_btnAuth;
	CCtrlButton	m_btnDeny;

	CJabberHttpAuthParams *m_pParams;
};

// XEP-0070 support (http auth)
INT_PTR __cdecl CJabberProto::OnHttpAuthRequest( WPARAM wParam, LPARAM lParam )
{
	CLISTEVENT *pCle = (CLISTEVENT *)lParam;
	CJabberHttpAuthParams *pParams = (CJabberHttpAuthParams *)pCle->lParam;
	if ( !pParams )
		return 0;

	CJabberDlgHttpAuth *pDlg = new CJabberDlgHttpAuth( this, (HWND)wParam, pParams );
	if ( !pDlg ) {
		pParams->Free();
		mir_free( pParams );
		return 0;
	}

	pDlg->Show();

	return 0;
}


// Jabber API functions
INT_PTR __cdecl CJabberProto::JabberGetApi( WPARAM wParam, LPARAM lParam )
{
	IJabberInterface **ji = (IJabberInterface**)lParam;
	if ( !ji )
		return -1;
	*ji = &m_JabberApi;
	return 0;
}

DWORD CJabberInterface::GetFlags() const
{
#ifdef _UNICODE
	return JIF_UNICODE;
#else
	return 0;
#endif
}

int CJabberInterface::GetVersion() const
{
	return 1;
}

DWORD CJabberInterface::GetJabberVersion() const
{
	return __VERSION_DWORD;
}

IJabberSysInterface *CJabberInterface::Sys() const
{
	return &m_psProto->m_JabberSysApi;
}

IJabberNetInterface *CJabberInterface::Net() const
{
	return &m_psProto->m_JabberNetApi;
}

int CJabberSysInterface::GetVersion() const
{
	return 1;
}

int CJabberSysInterface::CompareJIDs( LPCTSTR jid1, LPCTSTR jid2 )
{
	if ( !jid1 || !jid2 ) return 0;
	return JabberCompareJids( jid1, jid2 );
}

HANDLE CJabberSysInterface::ContactFromJID( LPCTSTR jid )
{
	if ( !jid ) return NULL;
	return m_psProto->HContactFromJID( jid );
}

LPTSTR CJabberSysInterface::ContactToJID( HANDLE hContact )
{
	return m_psProto->JGetStringT( hContact, m_psProto->JGetByte( hContact, "ChatRoom", 0 ) ? "ChatRoomID" : "jid" );
}

LPTSTR CJabberSysInterface::GetBestResourceName( LPCTSTR jid )
{
	if ( jid == NULL )
		return NULL;
	LPCTSTR p = _tcschr( jid, '/' );
	if ( p == NULL ) {
		m_psProto->ListLock(); // make sure we allow access to the list only after making mir_tstrdup() of resource name
		LPTSTR res = mir_tstrdup( m_psProto->ListGetBestClientResourceNamePtr( jid ));
		m_psProto->ListUnlock();
		return res;
	}
	return mir_tstrdup( jid );
}

LPTSTR CJabberSysInterface::GetResourceList( LPCTSTR jid )
{
	if ( !jid )
		return NULL;

	m_psProto->ListLock();
	JABBER_LIST_ITEM *item = NULL;
	if (( item = m_psProto->ListGetItemPtr( LIST_VCARD_TEMP, jid )) == NULL)
		item = m_psProto->ListGetItemPtr( LIST_ROSTER, jid );
	if ( item == NULL ) {
		m_psProto->ListUnlock();
		return NULL;
	}

	if ( item->resource == NULL ) {
		m_psProto->ListUnlock();
		return NULL;
	}

	int i;
	int iLen = 1; // 1 for extra zero terminator at the end of the string
	// calculate total necessary string length
	for ( i=0; i<item->resourceCount; i++ ) {
		iLen += lstrlen(item->resource[i].resourceName) + 1;
	}

	// allocate memory and fill it
	LPTSTR str = (LPTSTR)mir_alloc(iLen * sizeof(TCHAR));
	LPTSTR p = str;
	for ( i=0; i<item->resourceCount; i++ ) {
		lstrcpy(p, item->resource[i].resourceName);
		p += lstrlen(item->resource[i].resourceName) + 1;
	}
	*p = 0; // extra zero terminator

	m_psProto->ListUnlock();
	return str;
}

char *CJabberSysInterface::GetModuleName() const
{
	return m_psProto->m_szModuleName;
}

int CJabberNetInterface::GetVersion() const
{
	return 1;
}

unsigned int CJabberNetInterface::SerialNext()
{
	return m_psProto->SerialNext();
}

int CJabberNetInterface::SendXmlNode( HXML node )
{
	return m_psProto->m_ThreadInfo->send(node);
}


typedef struct
{
	JABBER_HANDLER_FUNC Func;
	void *pUserData;
} sHandlerData;

void CJabberProto::ExternalTempIqHandler( HXML node, CJabberIqInfo *pInfo )
{
	sHandlerData *d = (sHandlerData*)pInfo->GetUserData();
	d->Func(&m_JabberApi, node, d->pUserData);
	free(d); // free IqHandlerData allocated in CJabberNetInterface::AddIqHandler below
}

BOOL CJabberProto::ExternalIqHandler( HXML node, CJabberIqInfo *pInfo )
{
	sHandlerData *d = (sHandlerData*)pInfo->GetUserData();
	return d->Func(&m_JabberApi, node, d->pUserData);
}

BOOL CJabberProto::ExternalMessageHandler( HXML node, ThreadData *pThreadData, CJabberMessageInfo* pInfo )
{
	sHandlerData *d = (sHandlerData*)pInfo->GetUserData();
	return d->Func(&m_JabberApi, node, d->pUserData);
}

BOOL CJabberProto::ExternalPresenceHandler( HXML node, ThreadData *pThreadData, CJabberPresenceInfo* pInfo )
{
	sHandlerData *d = (sHandlerData*)pInfo->GetUserData();
	return d->Func(&m_JabberApi, node, d->pUserData);
}

BOOL CJabberProto::ExternalSendHandler( HXML node, ThreadData *pThreadData, CJabberSendInfo* pInfo )
{
	sHandlerData *d = (sHandlerData*)pInfo->GetUserData();
	return d->Func(&m_JabberApi, node, d->pUserData);
}

HJHANDLER CJabberNetInterface::AddPresenceHandler( JABBER_HANDLER_FUNC Func, void *pUserData, int iPriority )
{
	sHandlerData *d = (sHandlerData*)malloc(sizeof(sHandlerData));
	d->Func = Func;
	d->pUserData = pUserData;
	return (HJHANDLER)m_psProto->m_presenceManager.AddPermanentHandler( &CJabberProto::ExternalPresenceHandler, d, free, iPriority );
}

HJHANDLER CJabberNetInterface::AddMessageHandler( JABBER_HANDLER_FUNC Func, int iMsgTypes, LPCTSTR szXmlns, LPCTSTR szTag, void *pUserData, int iPriority )
{
	sHandlerData *d = (sHandlerData*)malloc(sizeof(sHandlerData));
	d->Func = Func;
	d->pUserData = pUserData;
	return (HJHANDLER)m_psProto->m_messageManager.AddPermanentHandler( &CJabberProto::ExternalMessageHandler, iMsgTypes, 0, szXmlns, FALSE, szTag, d, free, iPriority );
}

HJHANDLER CJabberNetInterface::AddIqHandler( JABBER_HANDLER_FUNC Func, int iIqTypes, LPCTSTR szXmlns, LPCTSTR szTag, void *pUserData, int iPriority )
{
	sHandlerData *d = (sHandlerData*)malloc(sizeof(sHandlerData));
	d->Func = Func;
	d->pUserData = pUserData;
	return (HJHANDLER)m_psProto->m_iqManager.AddPermanentHandler( &CJabberProto::ExternalIqHandler, iIqTypes, 0, szXmlns, FALSE, szTag, d, free, iPriority );
}

HJHANDLER CJabberNetInterface::AddTemporaryIqHandler( JABBER_HANDLER_FUNC Func, int iIqTypes, int iIqId, void *pUserData, DWORD dwTimeout, int iPriority )
{
	sHandlerData *d = (sHandlerData*)malloc(sizeof(sHandlerData));
	d->Func = Func;
	d->pUserData = pUserData;
	CJabberIqInfo* pInfo = m_psProto->m_iqManager.AddHandler( &CJabberProto::ExternalTempIqHandler, iIqTypes, NULL, 0, iIqId, d, iPriority );
	if ( pInfo && dwTimeout > 0 )
		pInfo->SetTimeout( dwTimeout );
	return (HJHANDLER)pInfo;
}

HJHANDLER CJabberNetInterface::AddSendHandler( JABBER_HANDLER_FUNC Func, void *pUserData, int iPriority )
{
	sHandlerData *d = (sHandlerData*)malloc(sizeof(sHandlerData));
	d->Func = Func;
	d->pUserData = pUserData;
	return (HJHANDLER)m_psProto->m_sendManager.AddPermanentHandler( &CJabberProto::ExternalSendHandler, d, free, iPriority );
}

int CJabberNetInterface::RemoveHandler( HJHANDLER hHandler )
{
	return m_psProto->m_sendManager.DeletePermanentHandler( (CJabberSendPermanentInfo*)hHandler ) ||
		m_psProto->m_presenceManager.DeletePermanentHandler( (CJabberPresencePermanentInfo*)hHandler ) ||
		m_psProto->m_messageManager.DeletePermanentHandler( (CJabberMessagePermanentInfo*)hHandler ) ||
		m_psProto->m_iqManager.DeletePermanentHandler( (CJabberIqPermanentInfo*)hHandler ) ||
		m_psProto->m_iqManager.DeleteHandler( (CJabberIqInfo*)hHandler );
}

JabberFeatCapPairDynamic *CJabberNetInterface::FindFeature(LPCTSTR szFeature)
{
	int i;
	for ( i = 0; i < m_psProto->m_lstJabberFeatCapPairsDynamic.getCount(); i++ )
		if ( !lstrcmp( m_psProto->m_lstJabberFeatCapPairsDynamic[i]->szFeature, szFeature ))
			return m_psProto->m_lstJabberFeatCapPairsDynamic[i];
	return NULL;
}

int CJabberNetInterface::RegisterFeature( LPCTSTR szFeature, LPCTSTR szDescription )
{
	if ( !szFeature ) {
		return false;
	}

	// check for this feature in core features, and return false if it's present, to prevent re-registering a core feature
	int i;
	for ( i = 0; g_JabberFeatCapPairs[i].szFeature; i++ )
	{
		if ( !lstrcmp( g_JabberFeatCapPairs[i].szFeature, szFeature ))
		{
			return false;
		}
	}

	m_psProto->ListLock();
	JabberFeatCapPairDynamic *fcp = FindFeature( szFeature );
	if ( !fcp ) {
	// if the feature is not registered yet, allocate new bit for it
		JabberCapsBits jcb = JABBER_CAPS_OTHER_SPECIAL; // set all bits not included in g_JabberFeatCapPairs

		// set all bits occupied by g_JabberFeatCapPairs
		for ( i = 0; g_JabberFeatCapPairs[i].szFeature; i++ )
			jcb |= g_JabberFeatCapPairs[i].jcbCap;

		// set all bits already occupied by external plugins
		for ( i = 0; i < m_psProto->m_lstJabberFeatCapPairsDynamic.getCount(); i++ )
			jcb |= m_psProto->m_lstJabberFeatCapPairsDynamic[i]->jcbCap;

		// Now get first zero bit. The line below is a fast way to do it. If there are no zero bits, it returns 0.
		jcb = (~jcb) & (JabberCapsBits)(-(__int64)(~jcb));

		if ( !jcb ) {
		// no more free bits
			m_psProto->ListUnlock();
			return false;
		}

		LPTSTR szExt = mir_tstrdup( szFeature );
		LPTSTR pSrc, pDst;
		for ( pSrc = szExt, pDst = szExt; *pSrc; pSrc++ ) {
		// remove unnecessary symbols from szFeature to make the string shorter, and use it as szExt
			if ( _tcschr( _T("bcdfghjklmnpqrstvwxz0123456789"), *pSrc )) {
				*pDst++ = *pSrc;
			}
		}
		*pDst = 0;
		m_psProto->m_clientCapsManager.SetClientCaps( _T(JABBER_CAPS_MIRANDA_NODE), szExt, jcb );

		fcp = new JabberFeatCapPairDynamic();
		fcp->szExt = szExt; // will be deallocated along with other values of JabberFeatCapPairDynamic in CJabberProto destructor
		fcp->szFeature = mir_tstrdup( szFeature );
		fcp->szDescription = szDescription ? mir_tstrdup( szDescription ) : NULL;
		fcp->jcbCap = jcb;
		m_psProto->m_lstJabberFeatCapPairsDynamic.insert( fcp );
	} else if ( szDescription ) {
	// update description
		if ( fcp->szDescription )
			mir_free( fcp->szDescription );
		fcp->szDescription = mir_tstrdup( szDescription );
	}
	m_psProto->ListUnlock();
	return true;
}

int CJabberNetInterface::AddFeatures( LPCTSTR szFeatures )
{
	if ( !szFeatures ) {
		return false;
	}

	m_psProto->ListLock();
	BOOL ret = true;
	LPCTSTR szFeat = szFeatures;
	while ( szFeat[0] ) {
		JabberFeatCapPairDynamic *fcp = FindFeature( szFeat );
		// if someone is trying to add one of core features, RegisterFeature() will return false, so we don't have to perform this check here
		if ( !fcp ) {
		// if the feature is not registered yet
			if ( !RegisterFeature( szFeat, NULL )) {
				ret = false;
			} else {
				fcp = FindFeature( szFeat ); // update fcp after RegisterFeature()
			}
		}
		if ( fcp ) {
			m_psProto->m_uEnabledFeatCapsDynamic |= fcp->jcbCap;
		} else {
			ret = false;
		}
		szFeat += lstrlen( szFeat ) + 1;
	}
	m_psProto->ListUnlock();

	if ( m_psProto->m_bJabberOnline ) {
		m_psProto->SendPresence( m_psProto->m_iStatus, true );
	}
	return ret;
}

int CJabberNetInterface::RemoveFeatures( LPCTSTR szFeatures )
{
	if ( !szFeatures ) {
		return false;
	}

	m_psProto->ListLock();
	BOOL ret = true;
	LPCTSTR szFeat = szFeatures;
	while ( szFeat[0] ) {
		JabberFeatCapPairDynamic *fcp = FindFeature( szFeat );
		if ( fcp ) {
			m_psProto->m_uEnabledFeatCapsDynamic &= ~fcp->jcbCap;
		} else {
			ret = false; // indicate that there was an error removing at least one of the specified features
		}
		szFeat += lstrlen( szFeat ) + 1;
	}
	m_psProto->ListUnlock();

	if ( m_psProto->m_bJabberOnline ) {
		m_psProto->SendPresence( m_psProto->m_iStatus, true );
	}
	return ret;
}

LPTSTR CJabberNetInterface::GetResourceFeatures( LPCTSTR jid )
{
	JabberCapsBits jcb = m_psProto->GetResourceCapabilites( jid, true );
	if ( !( jcb & JABBER_RESOURCE_CAPS_ERROR )) {
		m_psProto->ListLock(); // contents of m_lstJabberFeatCapPairsDynamic must not change from the moment we calculate total length and to the moment when we fill the string
		int i;
		int iLen = 1; // 1 for extra zero terminator at the end of the string
		// calculate total necessary string length
		for ( i = 0; g_JabberFeatCapPairs[i].szFeature; i++ ) {
			if ( jcb & g_JabberFeatCapPairs[i].jcbCap ) {
				iLen += lstrlen( g_JabberFeatCapPairs[i].szFeature ) + 1;
			}
		}
		for ( i = 0; i < m_psProto->m_lstJabberFeatCapPairsDynamic.getCount(); i++ ) {
			if ( jcb & m_psProto->m_lstJabberFeatCapPairsDynamic[i]->jcbCap ) {
				iLen += lstrlen( m_psProto->m_lstJabberFeatCapPairsDynamic[i]->szFeature ) + 1;
			}
		}

		// allocate memory and fill it
		LPTSTR str = (LPTSTR)mir_alloc( iLen * sizeof(TCHAR) );
		LPTSTR p = str;
		for ( i = 0; g_JabberFeatCapPairs[i].szFeature; i++ ) {
			if ( jcb & g_JabberFeatCapPairs[i].jcbCap ) {
				lstrcpy( p, g_JabberFeatCapPairs[i].szFeature );
				p += lstrlen( g_JabberFeatCapPairs[i].szFeature ) + 1;
			}
		}
		for ( i = 0; i < m_psProto->m_lstJabberFeatCapPairsDynamic.getCount(); i++ ) {
			if ( jcb & m_psProto->m_lstJabberFeatCapPairsDynamic[i]->jcbCap ) {
				lstrcpy( p, m_psProto->m_lstJabberFeatCapPairsDynamic[i]->szFeature );
				p += lstrlen( m_psProto->m_lstJabberFeatCapPairsDynamic[i]->szFeature ) + 1;
			}
		}
		*p = 0; // extra zero terminator
		m_psProto->ListUnlock();
		return str;
	}
	return NULL;
}
