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
// JabberGetAvatar - retrieves the file name of my own avatar

int __cdecl CJabberProto::JabberGetAvatar( WPARAM wParam, LPARAM lParam )
{
	char* buf = ( char* )wParam;
	int  size = ( int )lParam;

	if ( buf == NULL || size <= 0 )
		return -1;

	if ( !JGetByte( "EnableAvatars", TRUE ))
		return -2;

	GetAvatarFileName( NULL, buf, size );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetAvatarCaps - returns directives how to process avatars

int __cdecl CJabberProto::JabberGetAvatarCaps( WPARAM wParam, LPARAM lParam )
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
		return JGetByte( "EnableAvatars", TRUE );
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetAvatarInfo - retrieves the avatar info

int __cdecl CJabberProto::JabberGetAvatarInfo( WPARAM wParam, LPARAM lParam )
{
	if ( !JGetByte( "EnableAvatars", TRUE ))
		return GAIR_NOAVATAR;

	PROTO_AVATAR_INFORMATION* AI = ( PROTO_AVATAR_INFORMATION* )lParam;

	char szHashValue[ MAX_PATH ];
	if ( JGetStaticString( "AvatarHash", AI->hContact, szHashValue, sizeof szHashValue )) {
		Log( "No avatar" );
		return GAIR_NOAVATAR;
	}

	GetAvatarFileName( AI->hContact, AI->filename, sizeof AI->filename );
	AI->format = ( AI->hContact == NULL ) ? PA_FORMAT_PNG : JGetByte( AI->hContact, "AvatarType", 0 );

	if ( ::_access( AI->filename, 0 ) == 0 ) {
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
				TCHAR szJid[ 512 ];
				BOOL isXVcard = JGetByte( AI->hContact, "AvatarXVcard", 0 );
				if ( item->resourceCount != NULL && !isXVcard ) {
					TCHAR *bestResName = ListGetBestClientResourceNamePtr(dbv.ptszVal);
					mir_sntprintf( szJid, SIZEOF( szJid ), bestResName?_T("%s/%s"):_T("%s"), dbv.ptszVal, bestResName );
				}
				else lstrcpyn( szJid, dbv.ptszVal, SIZEOF( szJid ));

				Log( "Rereading %s for " TCHAR_STR_PARAM, isXVcard ? JABBER_FEAT_VCARD_TEMP : JABBER_FEAT_AVATAR, szJid );

				int iqId = SerialNext();
				IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultGetAvatar );

				XmlNodeIq iq( _T("get"), iqId, szJid );
				if ( isXVcard )
					iq << XCHILDNS( _T("vCard"), _T(JABBER_FEAT_VCARD_TEMP));
				else
					iq.addQuery( isXVcard ? _T("") : _T(JABBER_FEAT_AVATAR));
				m_ThreadInfo->send( iq );

				JFreeVariant( &dbv );
				return GAIR_WAITFOR;
	}	}	}

	Log( "No avatar" );
	return GAIR_NOAVATAR;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberGetName - returns the protocol name

int __cdecl CJabberProto::JabberGetName( WPARAM wParam, LPARAM lParam )
{
	lstrcpynA(( char* )lParam, m_szModuleName, wParam );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberGetStatus - returns the protocol status

int __cdecl CJabberProto::JabberGetStatus( WPARAM wParam, LPARAM lParam )
{
	return m_iStatus;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberGetEventTextChatStates - retrieves a chat state description from an event

int __cdecl CJabberProto::OnGetEventTextChatStates( WPARAM wParam, LPARAM lParam )
{
	DBEVENTGETTEXT *pdbEvent = ( DBEVENTGETTEXT * )lParam;

	int nRetVal = 0;

	if ( pdbEvent->dbei->cbBlob > 0 ) {
		if ( pdbEvent->dbei->pBlob[0] == JABBER_DB_EVENT_CHATSTATES_GONE ) {
			if ( pdbEvent->datatype == DBVT_WCHAR )
				nRetVal = (int)mir_tstrdup(TranslateTS(_T("closed chat session")));
			else if ( pdbEvent->datatype == DBVT_ASCIIZ )
				nRetVal = (int)mir_strdup(Translate("closed chat session"));
		}
	}
	
	return nRetVal;
}

////////////////////////////////////////////////////////////////////////////////////////
// OnGetEventTextPresence - retrieves presence state description from an event

int __cdecl CJabberProto::OnGetEventTextPresence( WPARAM wParam, LPARAM lParam )
{
	DBEVENTGETTEXT *pdbEvent = ( DBEVENTGETTEXT * )lParam;

	int nRetVal = 0;

	if ( pdbEvent->dbei->cbBlob > 0 ) {
		switch ( pdbEvent->dbei->pBlob[0] )
		{
		case JABBER_DB_EVENT_PRESENCE_SUBSCRIBE:
			{
				if ( pdbEvent->datatype == DBVT_WCHAR )
					nRetVal = (int)mir_tstrdup(TranslateTS(_T("sent subscription request")));
				else if ( pdbEvent->datatype == DBVT_ASCIIZ )
					nRetVal = (int)mir_strdup(Translate("sent subscription request"));
				break;
			}
		case JABBER_DB_EVENT_PRESENCE_SUBSCRIBED:
			{
				if ( pdbEvent->datatype == DBVT_WCHAR )
					nRetVal = (int)mir_tstrdup(TranslateTS(_T("approved subscription request")));
				else if ( pdbEvent->datatype == DBVT_ASCIIZ )
					nRetVal = (int)mir_strdup(Translate("approved subscription request"));
				break;
			}
		case JABBER_DB_EVENT_PRESENCE_UNSUBSCRIBE:
			{
				if ( pdbEvent->datatype == DBVT_WCHAR )
					nRetVal = (int)mir_tstrdup(TranslateTS(_T("declined subscription")));
				else if ( pdbEvent->datatype == DBVT_ASCIIZ )
					nRetVal = (int)mir_strdup(Translate("declined subscription"));
				break;
			}
		case JABBER_DB_EVENT_PRESENCE_UNSUBSCRIBED:
			{
				if ( pdbEvent->datatype == DBVT_WCHAR )
					nRetVal = (int)mir_tstrdup(TranslateTS(_T("declined subscription")));
				else if ( pdbEvent->datatype == DBVT_ASCIIZ )
					nRetVal = (int)mir_strdup(Translate("declined subscription"));
				break;
			}
		case JABBER_DB_EVENT_PRESENCE_ERROR:
			{
				if ( pdbEvent->datatype == DBVT_WCHAR )
					nRetVal = (int)mir_tstrdup(TranslateTS(_T("sent error presence")));
				else if ( pdbEvent->datatype == DBVT_ASCIIZ )
					nRetVal = (int)mir_strdup(Translate("sent error presence"));
				break;
			}
		default:
			{
				if ( pdbEvent->datatype == DBVT_WCHAR )
					nRetVal = (int)mir_tstrdup(TranslateTS(_T("sent unknown presence type")));
				else if ( pdbEvent->datatype == DBVT_ASCIIZ )
					nRetVal = (int)mir_strdup(Translate("sent unknown presence type"));
				break;
			}
		}
	}

	return nRetVal;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberSetAvatar - sets an avatar without UI

int __cdecl CJabberProto::JabberSetAvatar( WPARAM wParam, LPARAM lParam )
{
	char* szFileName = ( char* )lParam;

	if ( m_bJabberOnline ) {	
		SetServerVcard( TRUE, szFileName );
		SendPresence( m_iDesiredStatus, false );
	}
	else {
		int fileIn = _open( szFileName, O_RDWR | O_BINARY, S_IREAD | S_IWRITE );
		if ( fileIn == -1 )
			return 1;

		long  dwPngSize = _filelength( fileIn );
		char* pResult = new char[ dwPngSize ];
		if ( pResult == NULL ) {
			_close( fileIn );
			return 2;
		}

		_read( fileIn, pResult, dwPngSize );
		_close( fileIn );

		mir_sha1_byte_t digest[MIR_SHA1_HASH_SIZE];
		mir_sha1_ctx sha1ctx;
		mir_sha1_init( &sha1ctx );
		mir_sha1_append( &sha1ctx, (mir_sha1_byte_t*)pResult, dwPngSize );
		mir_sha1_finish( &sha1ctx, digest );

		char tFileName[ MAX_PATH ];
		GetAvatarFileName( NULL, tFileName, MAX_PATH );
		DeleteFileA( tFileName );

		char buf[MIR_SHA1_HASH_SIZE*2+1];
		for ( int i=0; i<MIR_SHA1_HASH_SIZE; i++ )
			sprintf( buf+( i<<1 ), "%02x", digest[i] );

		JSetByte( "AvatarType", JabberGetPictureType( pResult ));
		JSetString( NULL, "AvatarSaved", buf );

		GetAvatarFileName( NULL, tFileName, MAX_PATH );
		FILE* out = fopen( tFileName, "wb" );
		if ( out != NULL ) {
			fwrite( pResult, dwPngSize, 1, out );
			fclose( out );
		}
		delete[] pResult;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// "/SendXML" - Allows external plugins to send XML to the server

int __cdecl CJabberProto::ServiceSendXML( WPARAM wParam, LPARAM lParam )
{
	return m_ThreadInfo->send( (char*)lParam);
}

int __cdecl CJabberProto::JabberGCGetToolTipText( WPARAM wParam, LPARAM lParam )
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
	// Role:		Moderaror
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
	if ( _tcschr(info->resourceName,_T('@') != NULL ) ) {
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

	// Role
	if ( TRUE || info->role ) {
		_tcsncat( outBuf, szSeparator, SIZEOF(outBuf) );
		_tcsncat( outBuf, TranslateT("Role:\t\t"), SIZEOF(outBuf) );
		_tcsncat( outBuf, TranslateTS( JabberEnum2RoleStr[info->role] ), SIZEOF(outBuf) );
	}

	// Affiliation
	if ( TRUE || info->affiliation ) {
		_tcsncat( outBuf, szSeparator, SIZEOF(outBuf) );
		_tcsncat( outBuf, TranslateT("Affiliation:\t"), SIZEOF(outBuf) );
		_tcsncat( outBuf, TranslateTS( JabberEnum2AffilationStr[info->affiliation] ), SIZEOF(outBuf) );
	}

	// real jid
	if ( info->szRealJid ) {
		_tcsncat( outBuf, szSeparator, SIZEOF(outBuf) );
		_tcsncat( outBuf, TranslateT("Real JID:\t"), SIZEOF(outBuf) );
		_tcsncat( outBuf, info->szRealJid, SIZEOF(outBuf) );
	}

	if ( lstrlen( outBuf ) == 0)
		return 0;

	return (int) mir_tstrdup( outBuf );
}

// File Association Manager plugin support

int __cdecl CJabberProto::JabberServiceParseXmppURI( WPARAM wParam, LPARAM lParam )
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
			if ( !hContact )
				hContact = DBCreateContact( szJid, szJid, TRUE, TRUE );
			if ( !hContact )
				return 1;
			CallService( MS_MSG_SENDMESSAGE, (WPARAM)hContact, (LPARAM)NULL );
			return 0;
		}
		return 1;
	}
	else if ( !_tcsicmp( szCommand, _T( "roster" )))
	{
		if ( !HContactFromJID( szJid )) {
			JABBER_SEARCH_RESULT jsr = { 0 };
			jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
			jsr.hdr.nick = mir_t2a( szJid );
			_tcsncpy( jsr.jid, szJid, SIZEOF(jsr.jid) - 1 );

			ADDCONTACTSTRUCT acs;
			acs.handleType = HANDLE_SEARCHRESULT;
			acs.szProto = m_szModuleName;
			acs.psr = &jsr.hdr;
			CallService( MS_ADDCONTACT_SHOW, (WPARAM)NULL, (LPARAM)&acs );
			mir_free( jsr.hdr.nick );
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
int __cdecl CJabberProto::JabberSendNudge( WPARAM wParam, LPARAM lParam )
{
	if ( !m_bJabberOnline )
		return 0;

	HANDLE hContact = ( HANDLE )wParam;
	DBVARIANT dbv;
	if ( !JGetStringT( hContact, "jid", &dbv )) {
		m_ThreadInfo->send(
			XmlNode( _T("message")) << XATTR( _T("type"), _T("headline")) << XATTR( _T("to"), dbv.ptszVal )
				<< XCHILDNS( _T("attention"), _T(JABBER_FEAT_ATTENTION)));

		JFreeVariant( &dbv );
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
		xmlAddAttr( msg, _T("to"), pParams->m_szFrom );
		if ( !bAuthorized )
			xmlAddAttr( msg, _T("type"), _T("error"));
		if ( pParams->m_szThreadId )
			xmlAddChild( msg, "thread", pParams->m_szThreadId );

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

class CJabberDlgHttpAuth: public CJabberDlgFancy
{
	typedef CJabberDlgFancy CSuper;

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

		SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)m_proto->LoadIconEx("openid"));
		SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_proto->LoadIconEx("openid"));

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

	void btnAuth_OnClick(CCtrlButton *btn)
	{
		SendReply( TRUE );
		Close();
	}
	void btnDeny_OnClick(CCtrlButton *btn)
	{
		SendReply( FALSE );
		Close();
	}

	UI_MESSAGE_MAP(CJabberDlgHttpAuth, CSuper);
		UI_MESSAGE(WM_CTLCOLORSTATIC, OnCtlColorStatic);
	UI_MESSAGE_MAP_END();

	BOOL OnCtlColorStatic(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return (BOOL)GetSysColorBrush(COLOR_WINDOW);
	}

private:
	CCtrlEdit	m_txtInfo;
	CCtrlButton	m_btnAuth;
	CCtrlButton	m_btnDeny;

	CJabberHttpAuthParams *m_pParams;
};

// XEP-0070 support (http auth)
int __cdecl CJabberProto::OnHttpAuthRequest( WPARAM wParam, LPARAM lParam )
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
