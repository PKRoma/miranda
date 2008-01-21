/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan
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

#include "resource.h"
#include "jabber_list.h"
#include "jabber_iq.h"
#include "jabber_caps.h"
#include "m_file.h"
#include "m_addcontact.h"
#include "jabber_disco.h"
#include "sdk/m_proto_listeningto.h"

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetAvatar - retrieves the file name of my own avatar

int JabberGetAvatar( WPARAM wParam, LPARAM lParam, CJabberProto* ppro )
{
	char* buf = ( char* )wParam;
	int  size = ( int )lParam;

	if ( buf == NULL || size <= 0 )
		return -1;

	if ( !ppro->JGetByte( "EnableAvatars", TRUE ))
		return -2;

	ppro->JabberGetAvatarFileName( NULL, buf, size );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetAvatarCaps - returns directives how to process avatars

int JabberGetAvatarCaps( WPARAM wParam, LPARAM lParam, CJabberProto* ppro )
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
		return ppro->JGetByte( "EnableAvatars", TRUE );
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetAvatarInfo - retrieves the avatar info

int JabberGetAvatarInfo( WPARAM wParam, LPARAM lParam, CJabberProto* ppro )
{
	if ( !ppro->JGetByte( "EnableAvatars", TRUE ))
		return GAIR_NOAVATAR;

	PROTO_AVATAR_INFORMATION* AI = ( PROTO_AVATAR_INFORMATION* )lParam;

	char szHashValue[ MAX_PATH ];
	if ( ppro->JGetStaticString( "AvatarHash", AI->hContact, szHashValue, sizeof szHashValue )) {
		ppro->JabberLog( "No avatar" );
		return GAIR_NOAVATAR;
	}

	ppro->JabberGetAvatarFileName( AI->hContact, AI->filename, sizeof AI->filename );
	AI->format = ( AI->hContact == NULL ) ? PA_FORMAT_PNG : ppro->JGetByte( AI->hContact, "AvatarType", 0 );

	if ( ::access( AI->filename, 0 ) == 0 ) {
		char szSavedHash[ 256 ];
		if ( !ppro->JGetStaticString( "AvatarSaved", AI->hContact, szSavedHash, sizeof szSavedHash )) {
			if ( !strcmp( szSavedHash, szHashValue )) {
				ppro->JabberLog( "Avatar is Ok: %s == %s", szSavedHash, szHashValue );
				return GAIR_SUCCESS;
	}	}	}

	if (( wParam & GAIF_FORCE ) != 0 && AI->hContact != NULL && ppro->jabberOnline ) {
		DBVARIANT dbv;
		if ( !ppro->JGetStringT( AI->hContact, "jid", &dbv )) {
			JABBER_LIST_ITEM* item = ppro->JabberListGetItemPtr( LIST_ROSTER, dbv.ptszVal );
			if ( item != NULL ) {
				TCHAR szJid[ 512 ];
				BOOL isXVcard = ppro->JGetByte( AI->hContact, "AvatarXVcard", 0 );
				if ( item->resourceCount != NULL && !isXVcard ) {
					TCHAR *bestResName = ppro->JabberListGetBestClientResourceNamePtr(dbv.ptszVal);
					mir_sntprintf( szJid, SIZEOF( szJid ), bestResName?_T("%s/%s"):_T("%s"), dbv.ptszVal, bestResName );
				}
				else lstrcpyn( szJid, dbv.ptszVal, SIZEOF( szJid ));

				ppro->JabberLog( "Rereading %s for " TCHAR_STR_PARAM, isXVcard ? JABBER_FEAT_VCARD_TEMP : JABBER_FEAT_AVATAR, szJid );

				int iqId = ppro->JabberSerialNext();
				ppro->JabberIqAdd( iqId, IQ_PROC_NONE, &CJabberProto::JabberIqResultGetAvatar );

				XmlNodeIq iq( "get", iqId, szJid );
				if ( isXVcard )
					iq.addChild( "vCard" )->addAttr( "xmlns", JABBER_FEAT_VCARD_TEMP );
				else
					iq.addQuery( isXVcard ? "" : JABBER_FEAT_AVATAR );
				ppro->jabberThreadInfo->send( iq );

				JFreeVariant( &dbv );
				return GAIR_WAITFOR;
	}	}	}

	ppro->JabberLog( "No avatar" );
	return GAIR_NOAVATAR;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberGetName - returns the protocol name

int JabberGetName( WPARAM wParam, LPARAM lParam, CJabberProto* ppro )
{
	lstrcpynA(( char* )lParam, ppro->szModuleName, wParam );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberGetStatus - returns the protocol status

int JabberGetStatus( WPARAM wParam, LPARAM lParam, CJabberProto* ppro )
{
	return ppro->iStatus;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberGetEventTextChatStates - retrieves a chat state description from an event

int JabberGetEventTextChatStates( WPARAM wParam, LPARAM lParam, CJabberProto* )
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
// JabberSetAvatar - sets an avatar without UI

int JabberSetAvatar( WPARAM wParam, LPARAM lParam, CJabberProto* ppro )
{
	char* szFileName = ( char* )lParam;

	if ( ppro->jabberConnected )
	{	
		ppro->JabberUpdateVCardPhoto( szFileName );
		ppro->JabberSendPresence( ppro->iDesiredStatus, false );
	}
	else 
	{
		// FIXME OLD CODE: If avatar was changed during Jabber was offline. It should be store and send new vcard on online.

		int fileIn = open( szFileName, O_RDWR | O_BINARY, S_IREAD | S_IWRITE );
		if ( fileIn == -1 )
			return 1;

		long  dwPngSize = filelength( fileIn );
		char* pResult = new char[ dwPngSize ];
		if ( pResult == NULL ) {
			close( fileIn );
			return 2;
		}

		read( fileIn, pResult, dwPngSize );
		close( fileIn );

		mir_sha1_byte_t digest[MIR_SHA1_HASH_SIZE];
		mir_sha1_ctx sha1ctx;
		mir_sha1_init( &sha1ctx );
		mir_sha1_append( &sha1ctx, (mir_sha1_byte_t*)pResult, dwPngSize );
		mir_sha1_finish( &sha1ctx, digest );

		char tFileName[ MAX_PATH ];
		ppro->JabberGetAvatarFileName( NULL, tFileName, MAX_PATH );
		DeleteFileA( tFileName );

		char buf[MIR_SHA1_HASH_SIZE*2+1];
		for ( int i=0; i<MIR_SHA1_HASH_SIZE; i++ )
			sprintf( buf+( i<<1 ), "%02x", digest[i] );

		ppro->JSetByte( "AvatarType", JabberGetPictureType( pResult ));
		ppro->JSetString( NULL, "AvatarSaved", buf );

		ppro->JabberGetAvatarFileName( NULL, tFileName, MAX_PATH );
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

int ServiceSendXML( WPARAM wParam, LPARAM lParam, CJabberProto* ppro )
{
	return ppro->jabberThreadInfo->send( (char*)lParam);
}

int JabberGCGetToolTipText( WPARAM wParam, LPARAM lParam, CJabberProto* ppro )
{
	if (!wParam)
		return 0;

	if (!lParam)
		return 0; //room global tooltip not supported yet

	JABBER_LIST_ITEM* item = ppro->JabberListGetItemPtr( LIST_CHATROOM, (TCHAR*)wParam);
	if ( item == NULL )	return 0;  //no room found

	JABBER_RESOURCE_STATUS * info = NULL;
	for ( int i=0; i < item->resourceCount; i++ ) {
		JABBER_RESOURCE_STATUS& p = item->resource[i];
		if ( !lstrcmp( p.resourceName, (TCHAR*)lParam )) {
			info = &p;
			break;
		}	}
	if ( info==NULL )
		return 0; //no info found

	// ok process info output will be:
	// JID:			real@jid/resource or
	// Nick:		Nickname
	// Status:		StatusText
	// Role:		Moderaror
	// Affiliation:  Affiliation

	TCHAR outBuf[2048];
	outBuf[0]=_T('\0');

	TCHAR * szSeparator= (IsWinVerMEPlus()) ? _T("\r\n") : _T(" | ");

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
int JabberServiceParseXmppURI( WPARAM wParam, LPARAM lParam, CJabberProto* ppro )
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
			HANDLE hContact = ppro->JabberHContactFromJID( szJid, TRUE );
			if ( !hContact )
				hContact = ppro->JabberDBCreateContact( szJid, szJid, TRUE, TRUE );
			if ( !hContact )
				return 1;
			CallService( MS_MSG_SENDMESSAGE, (WPARAM)hContact, (LPARAM)NULL );
			return 0;
		}
		return 1;
	}
	else if ( !_tcsicmp( szCommand, _T( "roster" )))
	{
		if ( !ppro->JabberHContactFromJID( szJid )) {
			JABBER_SEARCH_RESULT jsr = { 0 };
			jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
			jsr.hdr.nick = mir_t2a( szJid );
			_tcsncpy( jsr.jid, szJid, SIZEOF(jsr.jid) - 1 );

			ADDCONTACTSTRUCT acs;
			acs.handleType = HANDLE_SEARCHRESULT;
			acs.szProto = ppro->szProtoName;
			acs.psr = &jsr.hdr;
			CallService( MS_ADDCONTACT_SHOW, (WPARAM)NULL, (LPARAM)&acs );
			mir_free( jsr.hdr.email );
		}
		return 0;
	}
	else if ( !_tcsicmp( szCommand, _T( "join" )))
	{
		// chat join invitation
		ppro->JabberGroupchatJoinRoomByJid( NULL, szJid );
		return 0;
	}
	else if ( !_tcsicmp( szCommand, _T( "disco" )))
	{
		// service discovery request
		JabberMenuHandleServiceDiscovery( 0, ( LPARAM )szJid, ppro );
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
		CJabberAdhocStartupParams* pStartupParams = new CJabberAdhocStartupParams( ppro, szJid, szSecondParam );
		JabberContactMenuRunCommands( 0, ( LPARAM )pStartupParams, ppro );
		return 0;
	}
	else if ( !_tcsicmp( szCommand, _T( "sendfile" )))
	{
		// send file
		if ( ServiceExists( MS_FILE_SENDFILE )) {
			HANDLE hContact = ppro->JabberHContactFromJID( szJid, TRUE );
			if ( !hContact )
				hContact = ppro->JabberDBCreateContact( szJid, szJid, TRUE, TRUE );
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
int JabberSendNudge( WPARAM wParam, LPARAM lParam, CJabberProto* ppro )
{
	if ( !ppro->jabberOnline )
		return 0;

	HANDLE hContact = ( HANDLE )wParam;
	DBVARIANT dbv;
	if ( !ppro->JGetStringT( hContact, "jid", &dbv )) {
		XmlNode m( "message" );
		m.addAttr( "type", "headline" );
		m.addAttr( "to", dbv.ptszVal );
		XmlNode *pAttention = m.addChild( "attention" );
		pAttention->addAttr( "xmlns", JABBER_FEAT_ATTENTION );
		ppro->jabberThreadInfo->send( m );

		JFreeVariant( &dbv );
	}
	return 0;
}
