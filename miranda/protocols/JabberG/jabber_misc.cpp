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
Last change on : $Date: 2006-07-13 16:11:29 +0400
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_list.h"
#include "jabber_caps.h"

#include "sdk/m_folders.h"

///////////////////////////////////////////////////////////////////////////////
// JabberAddContactToRoster() - adds a contact to the roster

void CJabberProto::JabberAddContactToRoster( const TCHAR* jid, const TCHAR* nick, const TCHAR* grpName, JABBER_SUBSCRIPTION subscription )
{
	XmlNodeIq iq( "set" );
	XmlNode* query = iq.addQuery( JABBER_FEAT_IQ_ROSTER );
	XmlNode* item = query->addChild( "item" ); item->addAttr( "jid", jid );
	if ( nick )
		item->addAttr( "name", nick );
	switch( subscription ) {
		case SUB_BOTH: item->addAttr( "subscription", "both" ); break;
		case SUB_TO:   item->addAttr( "subscription", "to" );   break;
		case SUB_FROM: item->addAttr( "subscription", "from" ); break;
		default:       item->addAttr( "subscription", "none" ); break;
	}

	if ( grpName != NULL )
		item->addChild( "group", grpName );
	jabberThreadInfo->send( iq );

}

///////////////////////////////////////////////////////////////////////////////
// JabberChatDllError() - missing CHAT.DLL

void JabberChatDllError()
{
	MessageBox( NULL,
		TranslateT( "CHAT plugin is required for conferences. Install it before chatting" ),
		TranslateT( "Jabber Error Message" ), MB_OK|MB_SETFOREGROUND );
}

///////////////////////////////////////////////////////////////////////////////
// JabberCompareJids

int JabberCompareJids( const TCHAR* jid1, const TCHAR* jid2 )
{
	if ( !lstrcmpi( jid1, jid2 ))
		return 0;

	// match only node@domain part
	TCHAR szTempJid1[ JABBER_MAX_JID_LEN ], szTempJid2[ JABBER_MAX_JID_LEN ];
	return lstrcmpi(
		JabberStripJid( jid1, szTempJid1, SIZEOF(szTempJid1) ),
		JabberStripJid( jid2, szTempJid2, SIZEOF(szTempJid2)) );
}

///////////////////////////////////////////////////////////////////////////////
// JabberContactListCreateGroup()

static void JabberContactListCreateClistGroup( TCHAR* groupName )
{
	char str[33];
	int i;
	DBVARIANT dbv;

	for ( i=0;;i++ ) {
		itoa( i, str, 10 );
		if ( DBGetContactSettingTString( NULL, "CListGroups", str, &dbv ))
			break;
		TCHAR* name = dbv.ptszVal;
		if ( name[0]!='\0' && !_tcscmp( name+1, groupName )) {
			// Already exists, no need to create
			JFreeVariant( &dbv );
			return;
		}
		JFreeVariant( &dbv );
	}

	// Create new group with id = i ( str is the text representation of i )
	TCHAR newName[128];
	newName[0] = 1 | GROUPF_EXPANDED;
	_tcsncpy( newName+1, groupName, SIZEOF( newName )-1 );
	newName[ SIZEOF( newName )-1] = '\0';
	DBWriteContactSettingTString( NULL, "CListGroups", str, newName );
	JCallService( MS_CLUI_GROUPADDED, i+1, 0 );
}

void JabberContactListCreateGroup( TCHAR* groupName )
{
	TCHAR name[128], *p;

	if ( groupName==NULL || groupName[0]=='\0' || groupName[0]=='\\' ) return;

	_tcsncpy( name, groupName, SIZEOF( name ));
	name[ SIZEOF( name )-1] = '\0';
	for ( p=name; *p!='\0'; p++ ) {
		if ( *p == '\\' ) {
			*p = '\0';
			JabberContactListCreateClistGroup( name );
			*p = '\\';
		}
	}
	JabberContactListCreateClistGroup( name );
}

///////////////////////////////////////////////////////////////////////////////
// JabberDBAddAuthRequest()

void CJabberProto::JabberDBAddAuthRequest( TCHAR* jid, TCHAR* nick )
{
	HANDLE hContact = JabberDBCreateContact( jid, NULL, FALSE, TRUE );
	JDeleteSetting( hContact, "Hidden" );
	//JSetStringT( hContact, "Nick", nick );

	char* szJid = mir_t2a( jid );
	char* szNick = mir_t2a( nick );

	//blob is: uin( DWORD ), hContact( HANDLE ), nick( ASCIIZ ), first( ASCIIZ ), last( ASCIIZ ), email( ASCIIZ ), reason( ASCIIZ )
	//blob is: 0( DWORD ), hContact( HANDLE ), nick( ASCIIZ ), ""( ASCIIZ ), ""( ASCIIZ ), email( ASCIIZ ), ""( ASCIIZ )
	DBEVENTINFO dbei = {0};
	dbei.cbSize = sizeof( DBEVENTINFO );
	dbei.szModule = m_szProtoName;
	dbei.timestamp = ( DWORD )time( NULL );
	dbei.flags = 0;
	dbei.eventType = EVENTTYPE_AUTHREQUEST;
	dbei.cbBlob = sizeof( DWORD )+ sizeof( HANDLE ) + strlen( szNick ) + strlen( szJid ) + 5;
	PBYTE pCurBlob = dbei.pBlob = ( PBYTE ) mir_alloc( dbei.cbBlob );
	*(( PDWORD ) pCurBlob ) = 0; pCurBlob += sizeof( DWORD );
	*(( PHANDLE ) pCurBlob ) = hContact; pCurBlob += sizeof( HANDLE );
	strcpy(( char* )pCurBlob, szNick ); pCurBlob += strlen( szNick )+1;
	*pCurBlob = '\0'; pCurBlob++;		//firstName
	*pCurBlob = '\0'; pCurBlob++;		//lastName
	strcpy(( char* )pCurBlob, szJid ); pCurBlob += strlen( szJid )+1;
	*pCurBlob = '\0';					//reason

	JCallService( MS_DB_EVENT_ADD, ( WPARAM ) ( HANDLE ) NULL, ( LPARAM )&dbei );
	JabberLog( "Setup DBAUTHREQUEST with nick='" TCHAR_STR_PARAM "' jid='" TCHAR_STR_PARAM "'", szNick, szJid );

	mir_free( szJid );
	mir_free( szNick );
}

///////////////////////////////////////////////////////////////////////////////
// JabberDBCreateContact()

HANDLE CJabberProto::JabberDBCreateContact( TCHAR* jid, TCHAR* nick, BOOL temporary, BOOL stripResource )
{
	TCHAR* s, *p, *q;
	int len;
	char* szProto;

	if ( jid==NULL || jid[0]=='\0' )
		return NULL;

	s = mir_tstrdup( jid );
	q = NULL;
	// strip resource if present
	if (( p = _tcschr( s, '@' )) != NULL )
		if (( q = _tcschr( p, '/' )) != NULL )
			*q = '\0';

	if ( !stripResource && q!=NULL )	// so that resource is not stripped
		*q = '/';
	len = _tcslen( s );

	// We can't use JabberHContactFromJID() here because of the stripResource option
	HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) {
		szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
		if ( szProto!=NULL && !strcmp( m_szProtoName, szProto )) {
			DBVARIANT dbv;
			if ( !JGetStringT( hContact, "jid", &dbv )) {
				p = dbv.ptszVal;
				if ( p && ( int )_tcslen( p )>=len && ( p[len]=='\0'||p[len]=='/' ) && !_tcsnicmp( p, s, len )) {
					JFreeVariant( &dbv );
					break;
				}
				JFreeVariant( &dbv );
		}	}
		hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
	}

	if ( hContact == NULL ) {
		hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_ADD, 0, 0 );
		JCallService( MS_PROTO_ADDTOCONTACT, ( WPARAM ) hContact, ( LPARAM )m_szProtoName );
		JSetStringT( hContact, "jid", s );
		if ( nick != NULL && *nick != '\0' )
			JSetStringT( hContact, "Nick", nick );
		if ( temporary )
			DBWriteContactSettingByte( hContact, "CList", "NotOnList", 1 );
		else
			JabberSendGetVcard( s );
		JabberLog( "Create Jabber contact jid=" TCHAR_STR_PARAM ", nick=" TCHAR_STR_PARAM, s, nick );
		JabberDBCheckIsTransportedContact(s,hContact);
	}

	mir_free( s );
	return hContact;
}

///////////////////////////////////////////////////////////////////////////////
// JabberGetAvatarFileName() - gets a file name for the avatar image

static HANDLE hJabberAvatarsFolder = NULL;
static bool bInitDone = false;

void CJabberProto::InitCustomFolders( void )
{
	if ( bInitDone )
		return;

	bInitDone = true;
	if ( ServiceExists( MS_FOLDERS_REGISTER_PATH )) {
		char AvatarsFolder[MAX_PATH]; AvatarsFolder[0] = 0;
		CallService( MS_DB_GETPROFILEPATH, ( WPARAM )MAX_PATH, ( LPARAM )AvatarsFolder );
		strcat( AvatarsFolder, "\\Jabber" );
		hJabberAvatarsFolder = FoldersRegisterCustomPath(m_szProtoName, "Avatars", AvatarsFolder);
}	}

void CJabberProto::JabberGetAvatarFileName( HANDLE hContact, char* pszDest, int cbLen )
{
	size_t tPathLen;
	char* path = ( char* )alloca( cbLen );

	InitCustomFolders();

	if ( hJabberAvatarsFolder == NULL || FoldersGetCustomPath( hJabberAvatarsFolder, path, cbLen, "" )) {
		JCallService( MS_DB_GETPROFILEPATH, cbLen, LPARAM( pszDest ));
		
		tPathLen = strlen( pszDest );
		tPathLen += mir_snprintf(pszDest + tPathLen, cbLen - tPathLen, "\\Jabber" );
	}
	else tPathLen = mir_snprintf( pszDest, cbLen, "%s", path );

	DWORD dwAttributes = GetFileAttributesA( pszDest );
	if ( dwAttributes == 0xffffffff || ( dwAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 )
		JCallService( MS_UTILS_CREATEDIRTREE, 0, ( LPARAM )pszDest );

	pszDest[ tPathLen++ ] = '\\';

	char* szFileType = NULL;
	switch( JGetByte( hContact, "AvatarType", PA_FORMAT_PNG )) {
		case PA_FORMAT_JPEG: szFileType = "jpg";   break;
		case PA_FORMAT_PNG:  szFileType = "png";   break;
		case PA_FORMAT_GIF:  szFileType = "gif";   break;
		case PA_FORMAT_BMP:  szFileType = "bmp";   break;
	}

	if ( hContact != NULL ) {
		char str[ 256 ];
		DBVARIANT dbv;
		if ( !JGetStringUtf( hContact, "jid", &dbv )) {
			strncpy( str, dbv.pszVal, sizeof str );
			str[ sizeof(str)-1 ] = 0;
			JFreeVariant( &dbv );
		}
		else ltoa(( long )hContact, str, 10 );

		char* hash = JabberSha1( str );
		mir_snprintf( pszDest + tPathLen, MAX_PATH - tPathLen, "%s.%s", hash, szFileType );
		mir_free( hash );
	}
	else if ( jabberThreadInfo != NULL ) {
		mir_snprintf( pszDest + tPathLen, MAX_PATH - tPathLen, TCHAR_STR_PARAM "@%s avatar.%s", 
			jabberThreadInfo->username, jabberThreadInfo->server, szFileType );
	}
	else {
		DBVARIANT dbv1, dbv2;
		BOOL res1 = DBGetContactSettingString( NULL, m_szProtoName, "LoginName", &dbv1 );
		BOOL res2 = DBGetContactSettingString( NULL, m_szProtoName, "LoginServer", &dbv2 );
		mir_snprintf( pszDest + tPathLen, MAX_PATH - tPathLen, "%s@%s avatar.%s",
			res1 ? "noname" : dbv1.pszVal,
			res2 ? m_szProtoName : dbv2.pszVal,
			szFileType );
		if (!res1) JFreeVariant( &dbv1 );
		if (!res2) JFreeVariant( &dbv2 );
	}
}

///////////////////////////////////////////////////////////////////////////////
// JabberResolveTransportNicks - massive vcard update

void CJabberProto::JabberResolveTransportNicks( TCHAR* jid )
{
	// Set all contacts to offline
	HANDLE hContact = jabberThreadInfo->resolveContact;
	if ( hContact == NULL )
		hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );

	for ( ; hContact != NULL; hContact = ( HANDLE )JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 )) {
		char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
		if ( lstrcmpA( szProto, m_szProtoName ))
			continue;

		if ( !JGetByte( hContact, "IsTransported", 0 ))
			continue;

		DBVARIANT dbv, nick;
		if ( JGetStringT( hContact, "jid", &dbv ))
			continue;
		if ( JGetStringT( hContact, "Nick", &nick )) {
			JFreeVariant( &dbv );
			continue;
		}

		TCHAR* p = _tcschr( dbv.ptszVal, '@' );
		if ( p ) {
			*p = 0;
			if ( !lstrcmp( jid, p+1 ) && !lstrcmp( dbv.ptszVal, nick.ptszVal )) {
				*p = '@';
				jabberThreadInfo->resolveID = JabberSendGetVcard( dbv.ptszVal );
				jabberThreadInfo->resolveContact = hContact;
				JFreeVariant( &dbv );
				JFreeVariant( &nick );
				return;
		}	}

		JFreeVariant( &dbv );
		JFreeVariant( &nick );
	}

	jabberThreadInfo->resolveID = -1;
	jabberThreadInfo->resolveContact = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// JabberSetServerStatus()

void CJabberProto::JabberSetServerStatus( int iNewStatus )
{
	if ( !jabberConnected )
		return;

	// change status
	int oldStatus = m_iStatus;
	switch ( iNewStatus ) {
	case ID_STATUS_ONLINE:
	case ID_STATUS_NA:
	case ID_STATUS_FREECHAT:
	case ID_STATUS_INVISIBLE:
		m_iStatus = iNewStatus;
		break;
	case ID_STATUS_AWAY:
	case ID_STATUS_ONTHEPHONE:
	case ID_STATUS_OUTTOLUNCH:
		m_iStatus = ID_STATUS_AWAY;
		break;
	case ID_STATUS_DND:
	case ID_STATUS_OCCUPIED:
		m_iStatus = ID_STATUS_DND;
		break;
	default:
		return;
	}

	// send presence update
	JabberSendPresence( m_iStatus, true );
	JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, m_iStatus );
}

// Process a string, and double all % characters, according to chat.dll's restrictions
// Returns a pointer to the new string (old one is not freed)
TCHAR* EscapeChatTags(TCHAR* pszText)
{
	int nChars = 0;
	for ( TCHAR* p = pszText; ( p = _tcschr( p, '%' )) != NULL; p++ )
		nChars++;

	if ( nChars == 0 )
		return mir_tstrdup( pszText );

	TCHAR* pszNewText = (TCHAR*)mir_alloc( sizeof(TCHAR)*(_tcslen( pszText ) + 1 + nChars )), *s, *d;
	if ( pszNewText == NULL )
		return mir_tstrdup( pszText );

	for ( s = pszText, d = pszNewText; *s; s++ ) {
		if ( *s == '%' )
			*d++ = '%';
		*d++ = *s;
	}
	*d = 0;
	return pszNewText;
}

char* UnEscapeChatTags(char* str_in)
{
	char* s = str_in, *d = str_in;
	while ( *s ) {
		if (( *s == '%' && s[1] == '%' ) || ( *s == '\n' && s[1] == '\n' ))
			s++;
		*d++ = *s++;
	}
	*d = 0;
	return str_in;
}

//////////////////////////////////////////////////////////////////////////
// update MirVer with data for active resource

void CJabberProto::JabberUpdateMirVer(JABBER_LIST_ITEM *item)
{
	HANDLE hContact = JabberHContactFromJID(item->jid);
	if (!hContact)
		return;

	JabberLog("JabberUpdateMirVer: for jid " TCHAR_STR_PARAM, item->jid);

	int resource = -1;
	if (item->resourceMode == RSMODE_LASTSEEN)
		resource = item->lastSeenResource;
	else if (item->resourceMode == RSMODE_MANUAL)
		resource = item->manualResource;
	if ((resource < 0) || (resource >= item->resourceCount))
		return;

	JabberUpdateMirVer( hContact, &item->resource[resource] );
}

void CJabberProto::JabberFormatMirVer(JABBER_RESOURCE_STATUS *resource, TCHAR *buf, int bufSize)
{
	if ( !buf || !bufSize ) return;
	buf[ 0 ] = _T('\0');
	if ( !resource ) return;

	// jabber:iq:version info requested and exists?
	if ( resource->dwVersionRequestTime && resource->software ) {
		JabberLog("JabberUpdateMirVer: for iq:version rc " TCHAR_STR_PARAM ": " TCHAR_STR_PARAM, resource->resourceName, resource->software);
		lstrcpyn(buf, resource->software, bufSize);
		return;
	}

	// no version info and no caps info? set MirVer = resource name
	if ( !resource->szCapsNode || !resource->szCapsVer ) {
		JabberLog("JabberUpdateMirVer: for rc " TCHAR_STR_PARAM ": " TCHAR_STR_PARAM, resource->resourceName, resource->resourceName);
		if ( resource->resourceName )
			lstrcpyn(buf, resource->resourceName, bufSize);
		return;
	}
	JabberLog("JabberUpdateMirVer: for rc " TCHAR_STR_PARAM ": " TCHAR_STR_PARAM "#" TCHAR_STR_PARAM, resource->resourceName, resource->szCapsNode, resource->szCapsVer);

	// XEP-0115 caps mode
	if ( _tcsstr( resource->szCapsNode, _T("miranda-im.org"))) {
		if ( resource->szCapsExt ) {
			mir_sntprintf( buf, bufSize, _T("Miranda IM %s (Jabber %s [%s]) (%s)"), resource->szCapsVer, resource->szCapsVer, resource->resourceName, resource->szCapsExt );

			if (TCHAR *pszSecureIm = _tcsstr( buf, _T(JABBER_EXT_SECUREIM)))
				_tcsncpy(pszSecureIm, _T("SecureIM"), 8);
		}
		else
			mir_sntprintf( buf, bufSize, _T("Miranda IM %s (Jabber %s [%s])"), resource->szCapsVer, resource->szCapsVer, resource->resourceName );
	}
	else if ( !resource->szCapsExt )
		mir_sntprintf( buf, bufSize, _T("%s#%s"), resource->szCapsNode, resource->szCapsVer );
	else if ( _tcsstr( resource->szCapsExt, _T(JABBER_EXT_SECUREIM) ))
		mir_sntprintf( buf, bufSize, _T("%s#%s#%s (SecureIM)"), resource->szCapsNode, resource->szCapsVer, resource->szCapsExt );
	else if ( _tcsstr( resource->szCapsNode, _T("www.google.com") ))
		mir_sntprintf( buf, bufSize, _T("%s#%s#%s gtalk"), resource->szCapsNode, resource->szCapsVer, resource->szCapsExt );
	else
		mir_sntprintf( buf, bufSize, _T("%s#%s#%s"), resource->szCapsNode, resource->szCapsVer, resource->szCapsExt );
}


void CJabberProto::JabberUpdateMirVer(HANDLE hContact, JABBER_RESOURCE_STATUS *resource)
{
	TCHAR szMirVer[ 512 ];
	JabberFormatMirVer(resource, szMirVer, SIZEOF(szMirVer));
	JSetStringT( hContact, "MirVer", szMirVer );
}


void CJabberProto::JabberSetContactOfflineStatus( HANDLE hContact )
{
	if ( JGetWord( hContact, "Status", ID_STATUS_OFFLINE ) != ID_STATUS_OFFLINE )
		JSetWord( hContact, "Status", ID_STATUS_OFFLINE );

	JDeleteSetting( hContact, DBSETTING_XSTATUSID );
	JDeleteSetting( hContact, DBSETTING_XSTATUSNAME );
	JDeleteSetting( hContact, DBSETTING_XSTATUSMSG );
	JabberUpdateContactExtraIcon(hContact);
}
