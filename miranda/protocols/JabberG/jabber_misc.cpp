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

void CJabberProto::AddContactToRoster( const TCHAR* jid, const TCHAR* nick, const TCHAR* grpName )
{
	XmlNodeIq iq( "set", SerialNext() );
	XmlNode query = iq.addQuery( _T(JABBER_FEAT_IQ_ROSTER));
	XmlNode item = query.addChild( "item" ); item.addAttr( "jid", jid );
	if ( nick )
		item.addAttr( "name", nick );
	if ( grpName )
		item.addChild( "group", grpName );
	m_ThreadInfo->send( iq );
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
		_itoa( i, str, 10 );
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

void CJabberProto::DBAddAuthRequest( const TCHAR* jid, const TCHAR* nick )
{
	HANDLE hContact = DBCreateContact( jid, NULL, FALSE, TRUE );
	JDeleteSetting( hContact, "Hidden" );
	//JSetStringT( hContact, "Nick", nick );

	char* szJid = mir_t2a( jid );
	char* szNick = mir_t2a( nick );

	//blob is: uin( DWORD ), hContact( HANDLE ), nick( ASCIIZ ), first( ASCIIZ ), last( ASCIIZ ), email( ASCIIZ ), reason( ASCIIZ )
	//blob is: 0( DWORD ), hContact( HANDLE ), nick( ASCIIZ ), ""( ASCIIZ ), ""( ASCIIZ ), email( ASCIIZ ), ""( ASCIIZ )
	DBEVENTINFO dbei = {0};
	dbei.cbSize = sizeof( DBEVENTINFO );
	dbei.szModule = m_szModuleName;
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
	Log( "Setup DBAUTHREQUEST with nick='" TCHAR_STR_PARAM "' jid='" TCHAR_STR_PARAM "'", szNick, szJid );

	mir_free( szJid );
	mir_free( szNick );
}

///////////////////////////////////////////////////////////////////////////////
// JabberDBCreateContact()

HANDLE CJabberProto::DBCreateContact( const TCHAR* jid, const TCHAR* nick, BOOL temporary, BOOL stripResource )
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
		if ( szProto!=NULL && !strcmp( m_szModuleName, szProto )) {
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
		JCallService( MS_PROTO_ADDTOCONTACT, ( WPARAM ) hContact, ( LPARAM )m_szModuleName );
		JSetStringT( hContact, "jid", s );
		if ( nick != NULL && *nick != '\0' )
			JSetStringT( hContact, "Nick", nick );
		if ( temporary )
			DBWriteContactSettingByte( hContact, "CList", "NotOnList", 1 );
		else
			SendGetVcard( s );
		Log( "Create Jabber contact jid=" TCHAR_STR_PARAM ", nick=" TCHAR_STR_PARAM, s, nick );
		DBCheckIsTransportedContact(s,hContact);
	}

	mir_free( s );
	return hContact;
}

BOOL CJabberProto::AddDbPresenceEvent(HANDLE hContact, BYTE btEventType)
{
	if ( !hContact )
		return FALSE;

	switch ( btEventType )
	{
	case JABBER_DB_EVENT_PRESENCE_SUBSCRIBE:
	case JABBER_DB_EVENT_PRESENCE_SUBSCRIBED:
	case JABBER_DB_EVENT_PRESENCE_UNSUBSCRIBE:
	case JABBER_DB_EVENT_PRESENCE_UNSUBSCRIBED:
		{
			if ( !JGetByte( "LogPresence", TRUE ))
				return FALSE;
			break;
		}
	case JABBER_DB_EVENT_PRESENCE_ERROR:
		{
			if ( !JGetByte( "LogPresenceErrors", TRUE ))
				return FALSE;
			break;
		}
	}

	DBEVENTINFO dbei;
	dbei.cbSize = sizeof( dbei );
	dbei.pBlob = &btEventType;
	dbei.cbBlob = sizeof( btEventType );
	dbei.eventType = JABBER_DB_EVENT_TYPE_PRESENCE;
	dbei.flags = 0;
	dbei.timestamp = time( NULL );
	dbei.szModule = m_szModuleName;
	CallService( MS_DB_EVENT_ADD, (WPARAM)hContact, (LPARAM)&dbei );

	return TRUE;
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
		hJabberAvatarsFolder = FoldersRegisterCustomPath(m_szModuleName, "Avatars", AvatarsFolder);	// title!!!!!!!!!!!
}	}

void CJabberProto::GetAvatarFileName( HANDLE hContact, char* pszDest, int cbLen )
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
		else _ltoa(( long )hContact, str, 10 );

		char* hash = JabberSha1( str );
		mir_snprintf( pszDest + tPathLen, MAX_PATH - tPathLen, "%s.%s", hash, szFileType );
		mir_free( hash );
	}
	else if ( m_ThreadInfo != NULL ) {
		mir_snprintf( pszDest + tPathLen, MAX_PATH - tPathLen, TCHAR_STR_PARAM "@%s avatar.%s", 
			m_ThreadInfo->username, m_ThreadInfo->server, szFileType );
	}
	else {
		DBVARIANT dbv1, dbv2;
		BOOL res1 = DBGetContactSettingString( NULL, m_szModuleName, "LoginName", &dbv1 );
		BOOL res2 = DBGetContactSettingString( NULL, m_szModuleName, "LoginServer", &dbv2 );
		mir_snprintf( pszDest + tPathLen, MAX_PATH - tPathLen, "%s@%s avatar.%s",
			res1 ? "noname" : dbv1.pszVal,
			res2 ? m_szModuleName : dbv2.pszVal,
			szFileType );
		if (!res1) JFreeVariant( &dbv1 );
		if (!res2) JFreeVariant( &dbv2 );
	}
}

///////////////////////////////////////////////////////////////////////////////
// JabberResolveTransportNicks - massive vcard update

void CJabberProto::ResolveTransportNicks( const TCHAR* jid )
{
	// Set all contacts to offline
	HANDLE hContact = m_ThreadInfo->resolveContact;
	if ( hContact == NULL )
		hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );

	for ( ; hContact != NULL; hContact = ( HANDLE )JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 )) {
		char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
		if ( lstrcmpA( szProto, m_szModuleName ))
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
				m_ThreadInfo->resolveID = SendGetVcard( dbv.ptszVal );
				m_ThreadInfo->resolveContact = hContact;
				JFreeVariant( &dbv );
				JFreeVariant( &nick );
				return;
		}	}

		JFreeVariant( &dbv );
		JFreeVariant( &nick );
	}

	m_ThreadInfo->resolveID = -1;
	m_ThreadInfo->resolveContact = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// JabberSetServerStatus()

void CJabberProto::SetServerStatus( int iNewStatus )
{
	if ( !m_bJabberOnline )
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
	SendPresence( m_iStatus, true );
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

TCHAR* UnEscapeChatTags(TCHAR* str_in)
{
	TCHAR* s = str_in, *d = str_in;
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

struct
{
	TCHAR *node;
	TCHAR *name;
}
static sttCapsNodeToName_Map[] =
{
	{ _T("http://miranda-im.org"), _T("Miranda IM Jabber") },
	{ _T("http://www.google.com"), _T("GTalk") },
};

void CJabberProto::UpdateMirVer(JABBER_LIST_ITEM *item)
{
	HANDLE hContact = HContactFromJID(item->jid);
	if (!hContact)
		return;

	Log("JabberUpdateMirVer: for jid " TCHAR_STR_PARAM, item->jid);

	int resource = -1;
	if (item->resourceMode == RSMODE_LASTSEEN)
		resource = item->lastSeenResource;
	else if (item->resourceMode == RSMODE_MANUAL)
		resource = item->manualResource;
	if ((resource < 0) || (resource >= item->resourceCount))
		return;

	UpdateMirVer( hContact, &item->resource[resource] );
}

void CJabberProto::FormatMirVer(JABBER_RESOURCE_STATUS *resource, TCHAR *buf, int bufSize)
{
	if ( !buf || !bufSize ) return;
	buf[ 0 ] = _T('\0');
	if ( !resource ) return;

	// jabber:iq:version info requested and exists?
	if ( resource->dwVersionRequestTime && resource->software ) {
		Log("JabberUpdateMirVer: for iq:version rc " TCHAR_STR_PARAM ": " TCHAR_STR_PARAM, resource->resourceName, resource->software);
		if (_tcsstr(resource->software, resource->version))
			lstrcpyn(buf, resource->software, bufSize);
		else
			mir_sntprintf(buf, bufSize, _T("%s %s"), resource->software, resource->version);
	}
	else

	// no version info and no caps info? set MirVer = resource name
	if ( !resource->szCapsNode || !resource->szCapsVer ) {
		Log("JabberUpdateMirVer: for rc " TCHAR_STR_PARAM ": " TCHAR_STR_PARAM, resource->resourceName, resource->resourceName);
		if ( resource->resourceName )
			lstrcpyn(buf, resource->resourceName, bufSize);
	}
	else

	// XEP-0115 caps mode
	{
		Log("JabberUpdateMirVer: for rc " TCHAR_STR_PARAM ": " TCHAR_STR_PARAM "#" TCHAR_STR_PARAM, resource->resourceName, resource->szCapsNode, resource->szCapsVer);

		int i;

		// search throught known software list
		for (i = 0; i < SIZEOF(sttCapsNodeToName_Map); ++i)
			if ( _tcsstr( resource->szCapsNode, sttCapsNodeToName_Map[i].node ) )
			{
				mir_sntprintf( buf, bufSize, _T("%s %s"), sttCapsNodeToName_Map[i].name, resource->szCapsVer );
				break;
			}

		// unknown software
		if (i == SIZEOF(sttCapsNodeToName_Map))
			mir_sntprintf( buf, bufSize, _T("%s %s"), resource->szCapsNode, resource->szCapsVer );
	}

	// attach additional info for fingerprint plguin
	if (resource->resourceName && !_tcsstr(buf, resource->resourceName))
	{
		if (_tcsstr(buf, _T("Miranda IM")) || JGetByte("ShowForeignResourceInMirVer", FALSE))
		{
			int offset = lstrlen(buf);
			mir_sntprintf(buf + offset, bufSize - offset, _T(" [%s]"), resource->resourceName);
		}
	}

	if (resource->szCapsExt && _tcsstr(resource->szCapsExt, _T(JABBER_EXT_SECUREIM)) && !_tcsstr(buf, _T("(SecureIM)")))
	{
		int offset = lstrlen(buf);
		mir_sntprintf(buf + offset, bufSize - offset, _T(" (SecureIM)"));
	}
}


void CJabberProto::UpdateMirVer(HANDLE hContact, JABBER_RESOURCE_STATUS *resource)
{
	TCHAR szMirVer[ 512 ];
	FormatMirVer(resource, szMirVer, SIZEOF(szMirVer));
	JSetStringT( hContact, "MirVer", szMirVer );
}


void CJabberProto::SetContactOfflineStatus( HANDLE hContact )
{
	if ( JGetWord( hContact, "Status", ID_STATUS_OFFLINE ) != ID_STATUS_OFFLINE )
		JSetWord( hContact, "Status", ID_STATUS_OFFLINE );

	JDeleteSetting( hContact, DBSETTING_XSTATUSID );
	JDeleteSetting( hContact, DBSETTING_XSTATUSNAME );
	JDeleteSetting( hContact, DBSETTING_XSTATUSMSG );

	ResetAdvStatus( hContact, ADVSTATUS_MOOD );
	ResetAdvStatus( hContact, ADVSTATUS_TUNE );

	//JabberUpdateContactExtraIcon(hContact);
}
