/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan

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
#include <richedit.h>

#include "jabber_list.h"
#include "jabber_caps.h"

#include "m_clistint.h"

extern CRITICAL_SECTION mutex;

extern int bSecureIM;

void CJabberProto::SerialInit( void )
{
	InitializeCriticalSection( &m_csSerial );
	m_nSerial = 0;
}

void CJabberProto::SerialUninit( void )
{
	DeleteCriticalSection( &m_csSerial );
}

int CJabberProto::SerialNext( void )
{
	unsigned int ret;

	EnterCriticalSection( &m_csSerial );
	ret = m_nSerial;
	m_nSerial++;
	LeaveCriticalSection( &m_csSerial );
	return ret;
}

void CJabberProto::Log( const char* fmt, ... )
{
	va_list vararg;
	va_start( vararg, fmt );
	char* str = ( char* )alloca( 32000 );
	mir_vsnprintf( str, 32000, fmt, vararg );
	va_end( vararg );

	JCallService( MS_NETLIB_LOG, ( WPARAM )m_hNetlibUser, ( LPARAM )str );
}

///////////////////////////////////////////////////////////////////////////////
// JabberChatRoomHContactFromJID - looks for the char room HCONTACT with required JID

HANDLE CJabberProto::ChatRoomHContactFromJID( const TCHAR* jid )
{
	if ( jid == NULL )
		return ( HANDLE )NULL;

	JABBER_LIST_ITEM* item = ListGetItemPtr( LIST_CHATROOM, jid );
	
	HANDLE hContactMatched = NULL;
	HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) {
		char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
		if ( szProto != NULL && !strcmp( m_szModuleName, szProto )) {
			DBVARIANT dbv;
			int result = JGetStringT( hContact, "ChatRoomID", &dbv );
			if ( result )
				result = JGetStringT( hContact, "jid", &dbv );	

			if ( !result ) {
				int result;
				result = lstrcmpi( jid, dbv.ptszVal );
				JFreeVariant( &dbv );
				if ( !result && JGetByte( hContact, "ChatRoom", 0 ) != 0 ) {
					hContactMatched = hContact;
					break;
		}	}	}

		hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
	}

	return hContactMatched;
}

///////////////////////////////////////////////////////////////////////////////
// JabberHContactFromJID - looks for the HCONTACT with required JID

HANDLE CJabberProto::HContactFromJID( const TCHAR* jid , BOOL bStripResource )
{
	if ( jid == NULL )
		return ( HANDLE )NULL;

	JABBER_LIST_ITEM* item = ListGetItemPtr( LIST_CHATROOM, jid );

	HANDLE hContactMatched = NULL;
	HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) {
		char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
		if ( szProto != NULL && !strcmp( m_szModuleName, szProto )) {
			DBVARIANT dbv;
			int result = JGetStringT( hContact, "jid", &dbv );
			if ( result )
				result = JGetStringT( hContact, "ChatRoomID", &dbv );

			if ( !result ) {
				int result;
				if ( item != NULL )
					result = lstrcmpi( jid, dbv.ptszVal );
				else {
					if ( bStripResource == 3 ) {
						if (JGetByte(hContact, "ChatRoom", 0))
							result = lstrcmpi( jid, dbv.ptszVal );  // for chat room we have to have full contact matched
						else if ( TRUE )
							result = _tcsnicmp( jid, dbv.ptszVal, _tcslen(dbv.ptszVal));
						else
							result = JabberCompareJids( jid, dbv.ptszVal );
					}
					// most probably it should just look full matching contact
					else
						result = lstrcmpi( jid, dbv.ptszVal );

				}
				JFreeVariant( &dbv );
				if ( !result ) {
					hContactMatched = hContact;
					break;
		}	}	}

		hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
	}

	return hContactMatched;
}

TCHAR* __stdcall JabberNickFromJID( const TCHAR* jid )
{
	const TCHAR* p;
	TCHAR* nick;

	if (( p = _tcschr( jid, '@' )) == NULL )
		p = _tcschr( jid, '/' );

	if ( p != NULL ) {
		if (( nick=( TCHAR* )mir_alloc( sizeof(TCHAR)*( int( p-jid )+1 ))) != NULL ) {
			_tcsncpy( nick, jid, p-jid );
			nick[p-jid] = '\0';
		}
	}
	else nick = mir_tstrdup( jid );
	return nick;
}

JABBER_RESOURCE_STATUS* CJabberProto::ResourceInfoFromJID( TCHAR* jid )
{
	if ( !jid )
		return NULL;

	JABBER_LIST_ITEM *item = NULL;
	if (( item = ListGetItemPtr( LIST_VCARD_TEMP, jid )) == NULL)
		item = ListGetItemPtr( LIST_ROSTER, jid );
	if ( item == NULL ) return NULL;

	TCHAR* p = _tcschr( jid, '/' );
	if ( p == NULL )
		return &item->itemResource;
	if ( *++p == '\0' ) return NULL;

	JABBER_RESOURCE_STATUS *r = item->resource;
	if ( r == NULL ) return NULL;

	int i;
	for ( i=0; i<item->resourceCount && _tcscmp( r->resourceName, p ); i++, r++ );
	if ( i >= item->resourceCount )
		return NULL;

	return r;
}

TCHAR* JabberPrepareJid( TCHAR *jid )
{
	if ( !jid ) return NULL;
	TCHAR* szNewJid = mir_tstrdup( jid );
	if ( !szNewJid ) return NULL;
	TCHAR* pDelimiter = _tcschr( szNewJid, _T('/') );
	if ( pDelimiter ) *pDelimiter = _T('\0');
	CharLower( szNewJid );
	if ( pDelimiter ) *pDelimiter = _T('/');
	return szNewJid;
}

char* __stdcall JabberUrlDecode( char* str )
{
	char* p, *q;

	if ( str == NULL )
		return NULL;

	for ( p=q=str; *p!='\0'; p++,q++ ) {
		if ( *p == '<' ) {
			// skip CDATA
			if ( !strncmp( p, "<![CDATA[", 9 ))
			{
				p += 9;
				char *tail = strstr(p, "]]>");
				int count = tail ? (tail-p) : strlen(p);
				memmove(q, p, count);
				q += count-1;
				p = (tail ? (tail+3) : (p+count)) - 1;
			} else
			{
				*q = *p;
			}
		} else
		if ( *p == '&' ) {
			if ( !strncmp( p, "&amp;", 5 )) {	*q = '&'; p += 4; }
			else if ( !strncmp( p, "&apos;", 6 )) { *q = '\''; p += 5; }
			else if ( !strncmp( p, "&gt;", 4 )) { *q = '>'; p += 3; }
			else if ( !strncmp( p, "&lt;", 4 )) { *q = '<'; p += 3; }
			else if ( !strncmp( p, "&quot;", 6 )) { *q = '"'; p += 5; }
			else { *q = *p;	}
		} else
		{
			*q = *p;
		}
	}
	*q = '\0';
	return str;
}

void __stdcall JabberUrlDecodeW( WCHAR* str )
{
	if ( str == NULL )
		return;

	WCHAR* p, *q;
	for ( p=q=str; *p!='\0'; p++,q++ ) {
		if ( *p == '&' ) {
			if ( !wcsncmp( p, L"&amp;", 5 )) {	*q = '&'; p += 4; }
			else if ( !wcsncmp( p, L"&apos;", 6 )) { *q = '\''; p += 5; }
			else if ( !wcsncmp( p, L"&gt;", 4 )) { *q = '>'; p += 3; }
			else if ( !wcsncmp( p, L"&lt;", 4 )) { *q = '<'; p += 3; }
			else if ( !wcsncmp( p, L"&quot;", 6 )) { *q = '"'; p += 5; }
			else { *q = *p;	}
		}
		else {
			*q = *p;
		}
	}
	*q = '\0';
}

char* __stdcall JabberUrlEncode( const char* str )
{
	char* s, *p, *q;
	int c;

	if ( str == NULL )
		return NULL;

	for ( c=0,p=( char* )str; *p!='\0'; p++ ) {
		switch ( *p ) {
			case '&': c += 5; break;
			case '\'': c += 6; break;
			case '>': c += 4; break;
			case '<': c += 4; break;
			case '"': c += 6; break;
			default: c++; break;
		}
	}
	if (( s=( char* )mir_alloc( c+1 )) != NULL ) {
		for ( p=( char* )str,q=s; *p!='\0'; p++ ) {
			switch ( *p ) {
			case '&': strcpy( q, "&amp;" ); q += 5; break;
			case '\'': strcpy( q, "&apos;" ); q += 6; break;
			case '>': strcpy( q, "&gt;" ); q += 4; break;
			case '<': strcpy( q, "&lt;" ); q += 4; break;
			case '"': strcpy( q, "&quot;" ); q += 6; break;
			default:
				if ( *p > 0 && *p < 32 ) {
					switch( *p ) {
					case '\r':
					case '\n':
					case '\t':
						*q = *p;
						break;
					default:
						*q = '?';
					}
				}
				else *q = *p; 
				q++; 
				break;
			}
		}
		*q = '\0';
	}

	return s;
}

static void __stdcall sttUtf8Decode( const BYTE* str, wchar_t* tempBuf )
{
	wchar_t* d = tempBuf;
	BYTE* s = ( BYTE* )str;

	while( *s )
	{
		if (( *s & 0x80 ) == 0 ) {
			*d++ = *s++;
			continue;
		}

		if (( s[0] & 0xE0 ) == 0xE0 && ( s[1] & 0xC0 ) == 0x80 && ( s[2] & 0xC0 ) == 0x80 ) {
			*d++ = (( WORD )( s[0] & 0x0F ) << 12 ) + ( WORD )(( s[1] & 0x3F ) << 6 ) + ( WORD )( s[2] & 0x3F );
			s += 3;
			continue;
		}

		if (( s[0] & 0xE0 ) == 0xC0 && ( s[1] & 0xC0 ) == 0x80 ) {
			*d++ = ( WORD )(( s[0] & 0x1F ) << 6 ) + ( WORD )( s[1] & 0x3F );
			s += 2;
			continue;
		}

		*d++ = *s++;
	}

	*d = 0;
}

void __stdcall JabberUtfToTchar( const char* pszValue, size_t cbLen, LPTSTR& dest )
{
	char* pszCopy = NULL;
	bool  bNeedsFree = false;
	__try
	{
		// this code can cause access violation when a stack overflow occurs
		pszCopy = ( char* )alloca( cbLen+1 );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		bNeedsFree = true;
		pszCopy = ( char* )malloc( cbLen+1 );
	}
	if ( pszCopy == NULL )
		return;

	memcpy( pszCopy, pszValue, cbLen );
	pszCopy[ cbLen ] = 0;

	JabberUrlDecode( pszCopy );

	#if defined( _UNICODE )
		mir_utf8decode( pszCopy, &dest );
	#else
		mir_utf8decode( pszCopy, NULL );
		dest = mir_strdup( pszCopy );
	#endif

	if ( bNeedsFree )
		free( pszCopy );
}

char* __stdcall JabberSha1( char* str )
{
	mir_sha1_ctx sha;
	mir_sha1_byte_t digest[20];
	char* result;
	int i;

	if ( str == NULL )
		return NULL;

	mir_sha1_init( &sha );
	mir_sha1_append( &sha, (mir_sha1_byte_t* )str, strlen( str ));
	mir_sha1_finish( &sha, digest );
	if (( result=( char* )mir_alloc( 41 )) == NULL )
		return NULL;

	for ( i=0; i<20; i++ )
		sprintf( result+( i<<1 ), "%02x", digest[i] );
	return result;
}

char* __stdcall JabberUnixToDos( const char* str )
{
	char* p, *q, *res;
	int extra;

	if ( str==NULL || str[0]=='\0' )
		return NULL;

	extra = 0;
	for ( p=( char* )str; *p!='\0'; p++ ) {
		if ( *p == '\n' )
			extra++;
	}
	if (( res=( char* )mir_alloc( strlen( str )+extra+1 )) != NULL ) {
		for ( p=( char* )str,q=res; *p!='\0'; p++,q++ ) {
			if ( *p == '\n' ) {
				*q = '\r';
				q++;
			}
			*q = *p;
		}
		*q = '\0';
	}
	return res;
}

WCHAR* __stdcall JabberUnixToDosW( const WCHAR* str )
{
	if ( str==NULL || str[0]=='\0' )
		return NULL;

	const WCHAR* p;
	WCHAR* q, *res;
	int extra = 0;

	for ( p = str; *p!='\0'; p++ ) {
		if ( *p == '\n' )
			extra++;
	}
	if (( res = ( WCHAR* )mir_alloc( sizeof( WCHAR )*( wcslen( str ) + extra + 1 )) ) != NULL ) {
		for ( p = str,q=res; *p!='\0'; p++,q++ ) {
			if ( *p == '\n' ) {
				*q = '\r';
				q++;
			}
			*q = *p;
		}
		*q = '\0';
	}
	return res;
}

char* __stdcall JabberHttpUrlEncode( const char* str )
{
	unsigned char* p, *q, *res;

	if ( str == NULL ) return NULL;
	res = ( BYTE* ) mir_alloc( 3*strlen( str ) + 1 );
	for ( p=( BYTE* )str,q=res; *p!='\0'; p++,q++ ) {
		if (( *p>='A' && *p<='Z' ) || ( *p>='a' && *p<='z' ) || ( *p>='0' && *p<='9' ) || strchr( "$-_.+!*'(),", *p )!=NULL ) {
			*q = *p;
		}
		else {
			sprintf(( char* )q, "%%%02X", *p );
			q += 2;
		}
	}
	*q = '\0';
	return ( char* )res;
}

void __stdcall JabberHttpUrlDecode( char* str )
{
	unsigned char* p, *q;
	unsigned int code;

	if ( str == NULL ) return;
	for ( p=q=( BYTE* )str; *p!='\0'; p++,q++ ) {
		if ( *p=='%' && *( p+1 )!='\0' && isxdigit( *( p+1 )) && *( p+2 )!='\0' && isxdigit( *( p+2 )) ) {
			sscanf(( char* )p+1, "%2x", &code );
			*q = ( unsigned char ) code;
			p += 2;
		}
		else {
			*q = *p;
	}	}

	*q = '\0';
}

int __stdcall JabberCombineStatus( int status1, int status2 )
{
	// Combine according to the following priority ( high to low )
	// ID_STATUS_FREECHAT
	// ID_STATUS_ONLINE
	// ID_STATUS_DND
	// ID_STATUS_AWAY
	// ID_STATUS_NA
	// ID_STATUS_INVISIBLE ( valid only for TLEN_PLUGIN )
	// ID_STATUS_OFFLINE
	// other ID_STATUS in random order ( actually return status1 )
	if ( status1==ID_STATUS_FREECHAT || status2==ID_STATUS_FREECHAT )
		return ID_STATUS_FREECHAT;
	if ( status1==ID_STATUS_ONLINE || status2==ID_STATUS_ONLINE )
		return ID_STATUS_ONLINE;
	if ( status1==ID_STATUS_DND || status2==ID_STATUS_DND )
		return ID_STATUS_DND;
	if ( status1==ID_STATUS_AWAY || status2==ID_STATUS_AWAY )
		return ID_STATUS_AWAY;
	if ( status1==ID_STATUS_NA || status2==ID_STATUS_NA )
		return ID_STATUS_NA;
	if ( status1==ID_STATUS_INVISIBLE || status2==ID_STATUS_INVISIBLE )
		return ID_STATUS_INVISIBLE;
	if ( status1==ID_STATUS_OFFLINE || status2==ID_STATUS_OFFLINE )
		return ID_STATUS_OFFLINE;
	return status1;
}

struct tagErrorCodeToStr {
	int code;
	TCHAR* str;
}
static JabberErrorCodeToStrMapping[] = {
	{ JABBER_ERROR_REDIRECT,               _T("Redirect") },
	{ JABBER_ERROR_BAD_REQUEST,            _T("Bad request") },
	{ JABBER_ERROR_UNAUTHORIZED,           _T("Unauthorized") },
	{ JABBER_ERROR_PAYMENT_REQUIRED,       _T("Payment required") },
	{ JABBER_ERROR_FORBIDDEN,              _T("Forbidden") },
	{ JABBER_ERROR_NOT_FOUND,              _T("Not found") },
	{ JABBER_ERROR_NOT_ALLOWED,            _T("Not allowed") },
	{ JABBER_ERROR_NOT_ACCEPTABLE,         _T("Not acceptable") },
	{ JABBER_ERROR_REGISTRATION_REQUIRED,  _T("Registration required") },
	{ JABBER_ERROR_REQUEST_TIMEOUT,        _T("Request timeout") },
	{ JABBER_ERROR_CONFLICT,               _T("Conflict") },
	{ JABBER_ERROR_INTERNAL_SERVER_ERROR,  _T("Internal server error") },
	{ JABBER_ERROR_NOT_IMPLEMENTED,        _T("Not implemented") },
	{ JABBER_ERROR_REMOTE_SERVER_ERROR,    _T("Remote server error") },
	{ JABBER_ERROR_SERVICE_UNAVAILABLE,    _T("Service unavailable") },
	{ JABBER_ERROR_REMOTE_SERVER_TIMEOUT,  _T("Remote server timeout") },
	{ -1,                                  _T("Unknown error") }
};

TCHAR* __stdcall JabberErrorStr( int errorCode )
{
	int i;

	for ( i=0; JabberErrorCodeToStrMapping[i].code!=-1 && JabberErrorCodeToStrMapping[i].code!=errorCode; i++ );
	return JabberErrorCodeToStrMapping[i].str;
}

TCHAR* __stdcall JabberErrorMsg( XmlNode *errorNode )
{
	TCHAR* errorStr, *str;
	int errorCode;

	errorStr = ( TCHAR* )mir_alloc( 256 * sizeof( TCHAR ));
	if ( errorNode == NULL ) {
		mir_sntprintf( errorStr, 256, _T("%s -1: %s"), TranslateT( "Error" ), TranslateT( "Unknown error message" ));
		return errorStr;
	}

	errorCode = -1;
	if (( str=JabberXmlGetAttrValue( errorNode, "code" )) != NULL )
		errorCode = _ttoi( str );
	if (( str=errorNode->text ) != NULL )
		mir_sntprintf( errorStr, 256, _T("%s %d: %s\r\n%s"), TranslateT( "Error" ), errorCode, TranslateTS( JabberErrorStr( errorCode )), str );
	else
		mir_sntprintf( errorStr, 256, _T("%s %d: %s"), TranslateT( "Error" ), errorCode, TranslateTS( JabberErrorStr( errorCode )) );
	return errorStr;
}

void CJabberProto::SendVisibleInvisiblePresence( BOOL invisible )
{
	if ( !m_bJabberOnline ) return;

	for ( int i = 0; ( i=ListFindNext( LIST_ROSTER, i )) >= 0; i++ ) {
		JABBER_LIST_ITEM *item = ListGetItemPtrFromIndex( i );
		if ( item == NULL )
			continue;

		HANDLE hContact = HContactFromJID( item->jid );
		if ( hContact == NULL )
			continue;

		WORD apparentMode = JGetWord( hContact, "ApparentMode", 0 );
		if ( invisible==TRUE && apparentMode==ID_STATUS_OFFLINE ) {
			XmlNode p( "presence" ); p.addAttr( "to", item->jid ); p.addAttr( "type", "invisible" );
			m_ThreadInfo->send( p );
		}
		else if ( invisible==FALSE && apparentMode==ID_STATUS_ONLINE )
			SendPresenceTo( m_iStatus, item->jid, NULL );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberTextEncodeW - prepare a string for transmission

char* __stdcall JabberTextEncode( const char* str )
{
	if ( str == NULL )
		return NULL;

	char* s1 = JabberUrlEncode( str );
	if ( s1 == NULL )
		return NULL;

	// Convert invalid control characters to space
	if ( *s1 ) {
		char* p, *q;

		for ( p = s1; *p != '\0'; p++ )
			if ( *p > 0 && *p < 0x20 && *p != 0x09 && *p != 0x0a && *p != 0x0d )
				*p = ( char )0x20;

		for ( p = q = s1; *p!='\0'; p++ ) {
			if ( *p != '\r' ) {
				*q = *p;
				q++;
		}	}

		*q = '\0';
	}

	char* s2 = mir_utf8encode( s1 );
	mir_free( s1 );
	return s2;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberTextEncodeW - prepare a string for transmission

char* __stdcall JabberTextEncodeW( const wchar_t* str )
{
	if ( str == NULL )
		return NULL;

	const wchar_t *s;
	int resLen = 1;

	for ( s = str; *s; s++ ) {
		switch( *s ) {
			case '\r':	continue;
			case '&':   resLen += 5;	break;
			case '\'':  resLen += 6;	break;
			case '>':
			case '<':	resLen += 6;	break;
			case '\"':  resLen += 6;	break;
			default:		resLen++;		break;
	}	}

	wchar_t* tmp = ( wchar_t* )alloca( resLen * sizeof( wchar_t )), *d;

	for ( s = str, d = tmp; *s; s++ ) {
		switch( *s ) {
		case '\r':	continue;
		case '&':   wcscpy( d, L"&amp;" );  d += 5;	break;
		case '\'':  wcscpy( d, L"&apos;" );	d += 6;	break;
		case '>':   wcscpy( d, L"&gt;" );   d += 4;	break;
		case '<':	wcscpy( d, L"&lt;" );   d += 4;	break;
		case '\"':  wcscpy( d, L"&quot;" ); d += 6;	break;
		default:
			if ( *s > 0 && *s < 0x20 && *s != 0x09 && *s != 0x0a && *s != 0x0d )
				*d++ = ' ';
			else
				*d++ = *s;
	}	}

	*d = 0;

	return mir_utf8encodeW( tmp );
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberTextDecode - retrieve a text from the encoded string

char* __stdcall JabberTextDecode( const char* str )
{
	if ( str == NULL )
		return NULL;

	char* s1 = ( char* )alloca( strlen( str )+1 ), *s2;
	strcpy( s1, str );

	mir_utf8decode( s1, NULL );
	JabberUrlDecode( s1 );
	if (( s2 = JabberUnixToDos( s1 )) == NULL )
		return NULL;

	return s2;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberBase64Encode

static char b64table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* __stdcall JabberBase64Encode( const char* buffer, int bufferLen )
{
	if ( buffer==NULL || bufferLen<=0 )
		return NULL;

	char* res = (char*)mir_alloc(((( bufferLen+2 )*4 )/3 ) + 1);
	if ( res == NULL )
		return NULL;

	unsigned char igroup[3];
	int nGroups = 0;
	char *r = res;
	const char* peob = buffer + bufferLen;
	for ( const char* p = buffer; p < peob; ) {
		igroup[ 0 ] = igroup[ 1 ] = igroup[ 2 ] = 0;
		int n;
		for ( n=0; n<3; n++ ) {
			if ( p >= peob ) break;
			igroup[n] = ( unsigned char ) *p;
			p++;
		}

		if ( n > 0 ) {
			r[0] = b64table[ igroup[0]>>2 ];
			r[1] = b64table[ (( igroup[0]&3 )<<4 ) | ( igroup[1]>>4 ) ];
			r[2] = b64table[ (( igroup[1]&0xf )<<2 ) | ( igroup[2]>>6 ) ];
			r[3] = b64table[ igroup[2]&0x3f ];

			if ( n < 3 ) {
				r[3] = '=';
				if ( n < 2 )
					r[2] = '=';
			}
			r += 4;
	}	}

	*r = '\0';

	return res;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberBase64Decode

static unsigned char b64rtable[256];

char* __stdcall JabberBase64Decode( const TCHAR* str, int *resultLen )
{
	char* res;
	unsigned char* r, igroup[4], a[4];
	int n, num, count;

	if ( str==NULL || resultLen==NULL ) return NULL;
	if (( res=( char* )mir_alloc(( ( _tcslen( str )+3 )/4 )*3 )) == NULL ) return NULL;

	for ( n=0; n<256; n++ )
		b64rtable[n] = ( unsigned char ) 0x80;
	for ( n=0; n<26; n++ )
		b64rtable['A'+n] = n;
	for ( n=0; n<26; n++ )
		b64rtable['a'+n] = n + 26;
	for ( n=0; n<10; n++ )
		b64rtable['0'+n] = n + 52;
	b64rtable['+'] = 62;
	b64rtable['/'] = 63;
	b64rtable['='] = 0;
	count = 0;
	for ( r=( unsigned char* )res; *str != '\0'; ) {
		for ( n=0; n<4; n++ ) {
			if ( BYTE(*str) == '\r' || BYTE(*str) == '\n' ) {
				n--; str++;
				continue;
			}

			if ( BYTE(*str)=='\0' ) {
				if ( n == 0 )
					goto LBL_Exit;
				mir_free( res );
				return NULL;
			}

			if ( b64rtable[BYTE(*str)]==0x80 ) {
				mir_free( res );
				return NULL;
			}

			a[n] = BYTE(*str);
			igroup[n] = b64rtable[BYTE(*str)];
			str++;
		}
		r[0] = igroup[0]<<2 | igroup[1]>>4;
		r[1] = igroup[1]<<4 | igroup[2]>>2;
		r[2] = igroup[2]<<6 | igroup[3];
		r += 3;
		num = ( a[2]=='='?1:( a[3]=='='?2:3 ));
		count += num;
		if ( num < 3 ) break;
	}
LBL_Exit:
	*resultLen = count;
	return res;
}

time_t __stdcall JabberIsoToUnixTime( TCHAR* stamp )
{
	struct tm timestamp;
	TCHAR date[9];
	TCHAR* p;
	int i, y;
	time_t t;

	if ( stamp == NULL ) return ( time_t ) 0;

	p = stamp;

	// Get the date part
	for ( i=0; *p!='\0' && i<8 && isdigit( *p ); p++,i++ )
		date[i] = *p;

	// Parse year
	if ( i == 6 ) {
		// 2-digit year ( 1970-2069 )
		y = ( date[0]-'0' )*10 + ( date[1]-'0' );
		if ( y < 70 ) y += 100;
	}
	else if ( i == 8 ) {
		// 4-digit year
		y = ( date[0]-'0' )*1000 + ( date[1]-'0' )*100 + ( date[2]-'0' )*10 + date[3]-'0';
		y -= 1900;
	}
	else
		return ( time_t ) 0;
	timestamp.tm_year = y;
	// Parse month
	timestamp.tm_mon = ( date[i-4]-'0' )*10 + date[i-3]-'0' - 1;
	// Parse date
	timestamp.tm_mday = ( date[i-2]-'0' )*10 + date[i-1]-'0';

	// Skip any date/time delimiter
	for ( ; *p!='\0' && !isdigit( *p ); p++ );

	// Parse time
	if ( _stscanf( p, _T("%d:%d:%d"), &timestamp.tm_hour, &timestamp.tm_min, &timestamp.tm_sec ) != 3 )
		return ( time_t ) 0;

	timestamp.tm_isdst = 0;	// DST is already present in _timezone below
	t = mktime( &timestamp );

	_tzset();
	t -= _timezone;

	if ( t >= 0 )
		return t;
	else
		return ( time_t ) 0;
}

struct MyCountryListEntry
{
	int id;
	TCHAR* szName;
}
static extraCtry[] =
{
	{ 1,	_T("United States") },
	{ 1,	_T("United States of America") },
	{ 1,	_T("US") },
	{ 44,	_T("England") }
};

int __stdcall JabberCountryNameToId( TCHAR* ptszCountryName )
{
	int ctryCount, i;

	// Check for some common strings not present in the country list
	ctryCount = sizeof( extraCtry )/sizeof( extraCtry[0] );
	for ( i=0; i < ctryCount && _tcsicmp( extraCtry[i].szName, ptszCountryName ); i++ );
	if ( i < ctryCount )
		return extraCtry[i].id;

	// Check Miranda country list
	{
		const char* szName;
		struct CountryListEntry *countries;
		JCallService( MS_UTILS_GETCOUNTRYLIST, ( WPARAM )&ctryCount, ( LPARAM )&countries );

		#if defined ( _UNICODE )
			const char* p = ( const char* )mir_t2a( ptszCountryName );
			szName = NEWSTR_ALLOCA( p );
			mir_free(( void* )p );
		#else
			szName = ptszCountryName;
		#endif

		for( i=0; i < ctryCount; i++ )
			if ( !strcmp( countries[i].szName, szName ))
				return countries[i].id;
	}

	return 0xffff;
}

void CJabberProto::SendPresenceTo( int status, TCHAR* to, XmlNode* extra )
{
	if ( !m_bJabberOnline ) return;

	// Send <presence/> update for status ( we won't handle ID_STATUS_OFFLINE here )
	EnterCriticalSection( &m_csModeMsgMutex );

	short iPriority = (short)JGetWord( NULL, "Priority", 0 );
	UpdatePriorityMenu(iPriority);

	char szPriority[40];
	_itoa( iPriority, szPriority, 10 );

	XmlNode p( "presence" ); p.addChild( "priority", szPriority );
	if ( to != NULL )
		p.addAttr( "to", to );

	if ( extra )
		p.addChild( extra );

	// XEP-0115:Entity Capabilities
	XmlNode *c = p.addChild( "c" );
	c->addAttr( "xmlns", JABBER_FEAT_ENTITY_CAPS );
	c->addAttr( "node", JABBER_CAPS_MIRANDA_NODE );
	c->addAttr( "ver", __VERSION_STRING );

	TCHAR szExtCaps[ 512 ];
	szExtCaps[ 0 ] = _T('\0');

	if ( bSecureIM ) {
		if ( _tcslen( szExtCaps ))
			_tcscat( szExtCaps, _T(" "));
		_tcscat( szExtCaps, _T(JABBER_EXT_SECUREIM) );
	}

	if ( JGetByte( "EnableRemoteControl", FALSE )) {
		if ( _tcslen( szExtCaps ))
			_tcscat( szExtCaps, _T(" "));
		_tcscat( szExtCaps, _T(JABBER_EXT_COMMANDS) );
	}

	if ( JGetByte( "EnableUserMood", TRUE )) {
		if ( _tcslen( szExtCaps ))
			_tcscat( szExtCaps, _T(" "));
		_tcscat( szExtCaps, _T(JABBER_EXT_USER_MOOD) );
	}

	if ( JGetByte( "EnableUserTune", FALSE )) {
		if ( _tcslen( szExtCaps ))
			_tcscat( szExtCaps, _T(" "));
		_tcscat( szExtCaps, _T(JABBER_EXT_USER_TUNE) );
	}

	if ( JGetByte( "EnableUserActivity", TRUE )) {
		if ( _tcslen( szExtCaps ))
			_tcscat( szExtCaps, _T(" "));
		_tcscat( szExtCaps, _T(JABBER_EXT_USER_ACTIVITY) );
	}

	if ( _tcslen( szExtCaps ))
		c->addAttr( "ext", szExtCaps );

	if ( JGetByte( "EnableAvatars", TRUE )) {
		char hashValue[ 50 ];
		if ( !JGetStaticString( "AvatarHash", NULL, hashValue, sizeof( hashValue ))) {
			XmlNode* x;

			// deprecated XEP-0008
//			x = p.addChild( "x" ); x->addAttr( "xmlns", "jabber:x:avatar" );
//			x->addChild( "hash", hashValue );

			// XEP-0153: vCard-Based Avatars
			x = p.addChild( "x" ); x->addAttr( "xmlns", "vcard-temp:x:update" );
			x->addChild( "photo", hashValue );
	}	}

	switch ( status ) {
	case ID_STATUS_ONLINE:
		if ( m_modeMsgs.szOnline )
			p.addChild( "status", m_modeMsgs.szOnline );
		break;
	case ID_STATUS_INVISIBLE:
		p.addAttr( "type", "invisible" );
		break;
	case ID_STATUS_AWAY:
	case ID_STATUS_ONTHEPHONE:
	case ID_STATUS_OUTTOLUNCH:
		p.addChild( "show", "away" );
		if ( m_modeMsgs.szAway )
			p.addChild( "status", m_modeMsgs.szAway );
		break;
	case ID_STATUS_NA:
		p.addChild( "show", "xa" );
		if ( m_modeMsgs.szNa )
			p.addChild( "status", m_modeMsgs.szNa );
		break;
	case ID_STATUS_DND:
	case ID_STATUS_OCCUPIED:
		p.addChild( "show", "dnd" );
		if ( m_modeMsgs.szDnd )
			p.addChild( "status", m_modeMsgs.szDnd );
		break;
	case ID_STATUS_FREECHAT:
		p.addChild( "show", "chat" );
		if ( m_modeMsgs.szFreechat )
			p.addChild( "status", m_modeMsgs.szFreechat );
		break;
	default:
		// Should not reach here
		break;
	}
	m_ThreadInfo->send( p );
	LeaveCriticalSection( &m_csModeMsgMutex );
}

void CJabberProto::SendPresence( int status, bool bSendToAll )
{
	SendPresenceTo( status, NULL, NULL );
	SendVisibleInvisiblePresence( status == ID_STATUS_INVISIBLE );

	// Also update status in all chatrooms
	if ( bSendToAll ) {
		for ( int i = 0; ( i=ListFindNext( LIST_CHATROOM, i )) >= 0; i++ ) {
			JABBER_LIST_ITEM *item = ListGetItemPtrFromIndex( i );
			if ( item != NULL )
				SendPresenceTo( status == ID_STATUS_INVISIBLE ? ID_STATUS_ONLINE : status, item->jid, NULL );
}	}	}

void __stdcall JabberStringAppend( char* *str, int *sizeAlloced, const char* fmt, ... )
{
	va_list vararg;
	char* p;
	int size, len;

	if ( str == NULL ) return;

	if ( *str==NULL || *sizeAlloced<=0 ) {
		*sizeAlloced = size = 2048;
		*str = ( char* )mir_alloc( size );
		len = 0;
	}
	else {
		len = strlen( *str );
		size = *sizeAlloced - strlen( *str );
	}

	p = *str + len;
	va_start( vararg, fmt );
	while ( _vsnprintf( p, size, fmt, vararg ) == -1 ) {
		size += 2048;
		( *sizeAlloced ) += 2048;
		*str = ( char* )mir_realloc( *str, *sizeAlloced );
		p = *str + len;
	}
	va_end( vararg );
}

///////////////////////////////////////////////////////////////////////////////
// JabberGetPacketID - converts the xml id attribute into an integer

int __stdcall JabberGetPacketID( XmlNode* n )
{
	int result = -1;

	TCHAR* str = JabberXmlGetAttrValue( n, "id" );
	if ( str )
		if ( !_tcsncmp( str, _T(JABBER_IQID), SIZEOF( JABBER_IQID )-1 ))
			result = _ttoi( str + SIZEOF( JABBER_IQID )-1 );

	return result;
}

///////////////////////////////////////////////////////////////////////////////
// JabberGetClientJID - adds a resource postfix to a JID

TCHAR* CJabberProto::GetClientJID( const TCHAR* jid, TCHAR* dest, size_t destLen )
{
	if ( jid == NULL )
		return NULL;

	size_t len = _tcslen( jid );
	if ( len >= destLen )
		len = destLen-1;

	_tcsncpy( dest, jid, len );
	dest[ len ] = '\0';

	TCHAR* p = _tcschr( dest, '/' );
	if ( p == NULL ) {
		TCHAR* resource = ListGetBestResourceNamePtr( jid );
		if ( resource != NULL )
			mir_sntprintf( dest+len, destLen-len-1, _T("/%s"), resource );
	}

	return dest;
}

///////////////////////////////////////////////////////////////////////////////
// JabberStripJid - strips a resource postfix from a JID

TCHAR* __stdcall JabberStripJid( const TCHAR* jid, TCHAR* dest, size_t destLen )
{
	if ( jid == NULL )
		*dest = 0;
	else {
		size_t len = _tcslen( jid );
		if ( len >= destLen )
			len = destLen-1;

		memcpy( dest, jid, len * sizeof( TCHAR ));
		dest[ len ] = 0;

		TCHAR* p = _tcschr( dest, '/' );
		if ( p != NULL )
			*p = 0;
	}

	return dest;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetXmlLang() - returns language code for xml:lang attribute, caller must free return value

TCHAR* CJabberProto::GetXmlLang()
{
	DBVARIANT dbv;
	TCHAR *szSelectedLang = NULL;
	if ( !JGetStringT( NULL, "XmlLang", &dbv )) {
		szSelectedLang = mir_tstrdup( dbv.ptszVal );
		JFreeVariant( &dbv );
	}
	else
		szSelectedLang = mir_tstrdup( _T( "en" ));
	return szSelectedLang;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGetPictureType - tries to autodetect the picture type from the buffer

int __stdcall JabberGetPictureType( const char* buf )
{
	if ( buf != NULL ) {
		if ( memcmp( buf, "GIF8", 4 ) == 0 )     return PA_FORMAT_GIF;
		if ( memcmp( buf, "\x89PNG", 4 ) == 0 )  return PA_FORMAT_PNG;
		if ( memcmp( buf, "BM", 2 ) == 0 )       return PA_FORMAT_BMP;
		if ( memcmp( buf, "\xFF\xD8", 2 ) == 0 ) return PA_FORMAT_JPEG;
	}

	return PA_FORMAT_UNKNOWN;
}

/////////////////////////////////////////////////////////////////////////////////////////
// TStringPairs class members

TStringPairs::TStringPairs( char* buffer ) :
	elems( NULL )
{
   TStringPairsElem tempElem[ 100 ];

	for ( numElems=0; *buffer; numElems++ ) {
		char* p = strchr( buffer, '=' );
		if ( p == NULL )
			break;

		tempElem[ numElems ].name = rtrim( buffer );
		*p = 0;
		if (( p = strchr( ++p, '\"' )) == NULL )
			break;

		char* p1 = strchr( p+1, '\"' );
		if ( p1 == NULL )
			break;

		*p1++ = 0;
		tempElem[ numElems ].value = rtrim( p+1 );
		p1 = skipSpaces( p1 );
		if ( *p1 == ',' )
			p1++;
		
		buffer = skipSpaces( p1 );
	}

	if ( numElems ) {
		elems = new TStringPairsElem[ numElems ];
		memcpy( elems, tempElem, sizeof(tempElem[0]) * numElems );
}	}

TStringPairs::~TStringPairs()
{
	delete[] elems;
}

const char* TStringPairs::operator[]( const char* key ) const
{
	for ( int i = 0; i < numElems; i++ )
		if ( !strcmp( elems[i].name, key ))
			return elems[i].value;

	return "";
}

////////////////////////////////////////////////////////////////////////
// Manage combo boxes with recent item list

void CJabberProto::ComboLoadRecentStrings(HWND hwndDlg, UINT idcCombo, char *param, int recentCount)
{
	for (int i = 0; i < recentCount; ++i) {
		DBVARIANT dbv;
		char setting[MAXMODULELABELLENGTH];
		mir_snprintf(setting, sizeof(setting), "%s%d", param, i);
		if (!JGetStringT(NULL, setting, &dbv)) {
			SendDlgItemMessage(hwndDlg, idcCombo, CB_ADDSTRING, 0, (LPARAM)dbv.ptszVal);
			JFreeVariant(&dbv);
	}	}
	if (!SendDlgItemMessage(hwndDlg, idcCombo, CB_GETCOUNT, 0, 0))
		SendDlgItemMessage(hwndDlg, idcCombo, CB_ADDSTRING, 0, (LPARAM)_T(""));
}

void CJabberProto::ComboAddRecentString(HWND hwndDlg, UINT idcCombo, char *param, TCHAR *string, int recentCount)
{
	if (!string || !*string)
		return;
	if (SendDlgItemMessage(hwndDlg, idcCombo, CB_FINDSTRING, (WPARAM)-1, (LPARAM)string) != CB_ERR)
		return;

	int id;
	SendDlgItemMessage(hwndDlg, idcCombo, CB_ADDSTRING, 0, (LPARAM)string);
	if ((id = SendDlgItemMessage(hwndDlg, idcCombo, CB_FINDSTRING, (WPARAM)-1, (LPARAM)_T(""))) != CB_ERR)
		SendDlgItemMessage(hwndDlg, idcCombo, CB_DELETESTRING, id, 0);

	id = JGetByte(NULL, param, 0);
	char setting[MAXMODULELABELLENGTH];
	mir_snprintf(setting, sizeof(setting), "%s%d", param, id);
	JSetStringT(NULL, setting, string);
	JSetByte(NULL, param, (id+1)%recentCount);
}

////////////////////////////////////////////////////////////////////////
// Rebuild status menu
static VOID CALLBACK sttRebuildMenusApcProc( DWORD param )
{
	CJabberProto *ppro = (CJabberProto *)param;

	CLIST_INTERFACE* pcli = ( CLIST_INTERFACE* )CallService( MS_CLIST_RETRIEVE_INTERFACE, 0, 0 );
	if ( pcli && pcli->version > 4 )
		pcli->pfnReloadProtoMenus();
}

static VOID CALLBACK sttRebuildInfoFrameApcProc( DWORD param )
{
	CJabberProto *ppro = (CJabberProto *)param;

	if (!ppro->m_pInfoFrame)
		return;

	ppro->m_pInfoFrame->LockUpdates();
	if (!ppro->m_bJabberOnline)
	{
		ppro->m_pInfoFrame->RemoveInfoItem("$/PEP");
		ppro->m_pInfoFrame->RemoveInfoItem("$/Transports");
		ppro->m_pInfoFrame->UpdateInfoItem("$/JID", LoadSkinnedIconHandle(SKINICON_OTHER_USERDETAILS), _T("Offline"));
	} else
	{
		ppro->m_pInfoFrame->UpdateInfoItem("$/JID", LoadSkinnedIconHandle(SKINICON_OTHER_USERDETAILS), ppro->m_szJabberJID);

		if (!ppro->m_bPepSupported)
		{
			ppro->m_pInfoFrame->RemoveInfoItem("$/PEP");
		} else
		{
			ppro->m_pInfoFrame->RemoveInfoItem("$/PEP/");
			ppro->m_pInfoFrame->CreateInfoItem("$/PEP", false);
			ppro->m_pInfoFrame->UpdateInfoItem("$/PEP", ppro->GetIconHandle(IDI_PL_LIST_ANY), TranslateT("Advanced Status"));

			ppro->m_pInfoFrame->CreateInfoItem("$/PEP/mood", true);
			ppro->m_pInfoFrame->SetInfoItemCallback("$/PEP/mood", &CJabberProto::InfoFrame_OnUserMood);
			ppro->m_pInfoFrame->UpdateInfoItem("$/PEP/mood", LoadSkinnedIconHandle(SKINICON_OTHER_SMALLDOT), TranslateT("Set mood..."));

			ppro->m_pInfoFrame->CreateInfoItem("$/PEP/activity", true);
			ppro->m_pInfoFrame->SetInfoItemCallback("$/PEP/activity", &CJabberProto::InfoFrame_OnUserActivity);
			ppro->m_pInfoFrame->UpdateInfoItem("$/PEP/activity", LoadSkinnedIconHandle(SKINICON_OTHER_SMALLDOT), TranslateT("Set activity..."));
		}

		ppro->m_pInfoFrame->RemoveInfoItem("$/Transports/");
		ppro->m_pInfoFrame->CreateInfoItem("$/Transports", false);
		ppro->m_pInfoFrame->UpdateInfoItem("$/Transports", ppro->GetIconHandle(IDI_TRANSPORT), TranslateT("Transports"));

		int i = 0;
		JABBER_LIST_ITEM *item = NULL;
		while (( i=ppro->ListFindNext( LIST_ROSTER, i )) >= 0 ) {
			if (( item=ppro->ListGetItemPtrFromIndex( i )) != NULL ) {
				if ( _tcschr( item->jid, '@' )==NULL && _tcschr( item->jid, '/' )==NULL && item->subscription!=SUB_NONE ) {
					HANDLE hContact = ppro->HContactFromJID( item->jid );
					if ( hContact == NULL ) continue;

					char name[128];
					char *jid_copy = mir_t2a(item->jid);
					mir_snprintf(name, SIZEOF(name), "$/Transports/%s", jid_copy);
					ppro->m_pInfoFrame->CreateInfoItem(name, true, (LPARAM)hContact);
					ppro->m_pInfoFrame->UpdateInfoItem(name, ppro->GetIconHandle(IDI_TRANSPORTL), (TCHAR *)item->jid);
					ppro->m_pInfoFrame->SetInfoItemCallback(name, &CJabberProto::InfoFrame_OnTransport);
					mir_free(jid_copy);
			}	}
			i++;
		}
	}
	ppro->m_pInfoFrame->Update();
}

void CJabberProto::RebuildStatusMenu()
{
	QueueUserAPC(sttRebuildMenusApcProc, hMainThread, (ULONG_PTR)this);
}

void CJabberProto::RebuildInfoFrame()
{
	QueueUserAPC(sttRebuildInfoFrameApcProc, hMainThread, (ULONG_PTR)this);
}

////////////////////////////////////////////////////////////////////////
// case-insensitive _tcsstr
TCHAR *JabberStrIStr(TCHAR *str, TCHAR *substr)
{
	TCHAR *str_up = NEWTSTR_ALLOCA(str);
	TCHAR *substr_up = NEWTSTR_ALLOCA(substr);

	CharUpperBuff(str_up, lstrlen(str_up));
	CharUpperBuff(substr_up, lstrlen(substr_up));

	TCHAR *p = _tcsstr(str_up, substr_up);
	return p ? (str + (p - str_up)) : NULL;
}

////////////////////////////////////////////////////////////////////////
// clipboard processing
void JabberCopyText(HWND hwnd, TCHAR *text)
{
	if (!hwnd || !text) return;

	OpenClipboard(hwnd);
	EmptyClipboard();
	int a = lstrlen(text);
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sizeof(TCHAR)*(lstrlen(text)+1));
	TCHAR *s = (TCHAR *)GlobalLock(hMem);
	lstrcpy(s, text);
	GlobalUnlock(hMem);
#ifdef UNICODE
	SetClipboardData(CF_UNICODETEXT, hMem);
#else
	SetClipboardData(CF_TEXT, hMem);
#endif
	CloseClipboard();
}

/////////////////////////////////////////////////////////////////////////////////////////
// One string entry dialog

struct JabberEnterStringParams
{
	CJabberProto* ppro;

	int type;
	TCHAR* caption;
	TCHAR* result;
	size_t resultLen;
	char *windowName;
	int recentCount;
	int timeout;

	int idcControl;
	int height;
};

static int sttEnterStringResizer(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL *urc)
{
	switch (urc->wId)
	{
	case IDC_TXT_MULTILINE:
	case IDC_TXT_COMBO:
	case IDC_TXT_RICHEDIT:
		return RD_ANCHORX_LEFT|RD_ANCHORY_TOP|RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;
	case IDOK:
	case IDCANCEL:
		return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
	}
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
}

static BOOL CALLBACK sttEnterStringDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	JabberEnterStringParams *params = (JabberEnterStringParams *)GetWindowLong( hwndDlg, GWL_USERDATA );

	switch ( msg ) {
	case WM_INITDIALOG:
	{
		//SetWindowPos( hwndDlg, HWND_TOPMOST ,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE );
		TranslateDialogDefault( hwndDlg );
		SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_OTHER_RENAME));
		SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadSkinnedIcon(SKINICON_OTHER_RENAME));
		JabberEnterStringParams *params = (JabberEnterStringParams *)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG )params );
		SetWindowText( hwndDlg, params->caption );

		RECT rc; GetWindowRect(hwndDlg, &rc);
		switch (params->type)
		{
			case JES_MULTINE:
			{
				params->idcControl = IDC_TXT_MULTILINE;
				params->height = 0;
				rc.bottom += (rc.bottom-rc.top) * 2;
				SetWindowPos(hwndDlg, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top, SWP_NOMOVE|SWP_NOREPOSITION);
				break;
			}
			case JES_COMBO:
			{
				params->idcControl = IDC_TXT_COMBO;
				params->height = rc.bottom-rc.top;
				if (params->windowName && params->recentCount)
					params->ppro->ComboLoadRecentStrings(hwndDlg, IDC_TXT_COMBO, params->windowName, params->recentCount);
				break;
			}
			case JES_RICHEDIT:
			{
				params->idcControl = IDC_TXT_RICHEDIT;
				SendDlgItemMessage(hwndDlg, IDC_TXT_RICHEDIT, EM_AUTOURLDETECT, TRUE, 0);
				SendDlgItemMessage(hwndDlg, IDC_TXT_RICHEDIT, EM_SETEVENTMASK, 0, ENM_LINK);
				params->height = 0;
				rc.bottom += (rc.bottom-rc.top) * 2;
				SetWindowPos(hwndDlg, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top, SWP_NOMOVE|SWP_NOREPOSITION);
				break;
			}
		}

		ShowWindow(GetDlgItem(hwndDlg, params->idcControl), SW_SHOW);
		SetDlgItemText( hwndDlg, params->idcControl, params->result );

		if (params->windowName)
			Utils_RestoreWindowPosition(hwndDlg, NULL, params->ppro->m_szModuleName, params->windowName);

		SetTimer(hwndDlg, 1000, 50, NULL);

		if (params->timeout > 0)
		{
			SetTimer(hwndDlg, 1001, 1000, NULL);
			TCHAR buf[128];
			mir_sntprintf(buf, SIZEOF(buf), _T("%s (%d)"), TranslateT("OK"), params->timeout);
			SetDlgItemText(hwndDlg, IDOK, buf);
		}

		return TRUE;
	}
	case WM_TIMER:
	{
		switch (wParam)
		{
			case 1000:
				KillTimer(hwndDlg,1000);
				EnableWindow(GetParent(hwndDlg), TRUE);
				return TRUE;

			case 1001:
			{
				TCHAR buf[128];
				mir_sntprintf(buf, SIZEOF(buf), _T("%s (%d)"), TranslateT("OK"), --params->timeout);
				SetDlgItemText(hwndDlg, IDOK, buf);

				if (params->timeout < 0)
				{
					KillTimer(hwndDlg, 1001);
					UIEmulateBtnClick(hwndDlg, IDOK);
				}

				return TRUE;
			}
		}
	}
	case WM_SIZE:
	{
		UTILRESIZEDIALOG urd = {0};
		urd.cbSize = sizeof(urd);
		urd.hInstance = hInst;
		urd.hwndDlg = hwndDlg;
		urd.lpTemplate = MAKEINTRESOURCEA(IDD_GROUPCHAT_INPUT);
		urd.pfnResizer = sttEnterStringResizer;
		CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM)&urd);
		break;
	}
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
		if (params && params->height)
			lpmmi->ptMaxSize.y = lpmmi->ptMaxTrackSize.y = params->height;
		break;
	}
	case WM_NOTIFY:
	{
		ENLINK *param = (ENLINK *)lParam;
		if (param->nmhdr.idFrom != IDC_TXT_RICHEDIT) break;
		if (param->nmhdr.code != EN_LINK) break;
		if (param->msg != WM_LBUTTONUP) break;

		CHARRANGE sel;
		SendMessage(param->nmhdr.hwndFrom, EM_EXGETSEL, 0, (LPARAM) & sel);
		if (sel.cpMin != sel.cpMax) break; // allow link selection

		TEXTRANGE tr;
		tr.chrg = param->chrg;
		tr.lpstrText = (TCHAR *)mir_alloc(sizeof(TCHAR)*(tr.chrg.cpMax - tr.chrg.cpMin + 2));
        SendMessage(param->nmhdr.hwndFrom, EM_GETTEXTRANGE, 0, (LPARAM) & tr);

		char *tmp = mir_t2a(tr.lpstrText);
		CallService(MS_UTILS_OPENURL, 1, (LPARAM)tmp);
        mir_free(tmp);
        mir_free(tr.lpstrText);
        return TRUE;
	}
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDOK:
			GetDlgItemText( hwndDlg, params->idcControl, params->result, params->resultLen );
			params->result[ params->resultLen-1 ] = 0;

			if ((params->type == JES_COMBO) && params->windowName && params->recentCount)
				params->ppro->ComboAddRecentString(hwndDlg, IDC_TXT_COMBO, params->windowName, params->result, params->recentCount);
			if (params->windowName)
				Utils_SaveWindowPosition(hwndDlg, NULL, params->ppro->m_szModuleName, params->windowName);

			EndDialog( hwndDlg, 1 );
			break;

		case IDCANCEL:
			if (params->windowName)
				Utils_SaveWindowPosition(hwndDlg, NULL, params->ppro->m_szModuleName, params->windowName);

			EndDialog( hwndDlg, 0 );
			break;

		case IDC_TXT_MULTILINE:
		case IDC_TXT_RICHEDIT:
			if ((HIWORD(wParam) != EN_SETFOCUS) && (HIWORD(wParam) != EN_KILLFOCUS))
			{
				SetDlgItemText(hwndDlg, IDOK, TranslateT("OK"));
				KillTimer(hwndDlg, 1001);
			}
			break;

		case IDC_TXT_COMBO:
			if ((HIWORD(wParam) != CBN_SETFOCUS) && (HIWORD(wParam) != CBN_KILLFOCUS))
			{
				SetDlgItemText(hwndDlg, IDOK, TranslateT("OK"));
				KillTimer(hwndDlg, 1001);
			}
			break;
	}	}

	return FALSE;
}

BOOL CJabberProto::EnterString(TCHAR *result, size_t resultLen, TCHAR *caption, int type, char *windowName, int recentCount, int timeout)
{
	bool free_caption = false;
	if (!caption || (caption==result))
	{
		free_caption = true;
		caption = mir_tstrdup( result );
		result[ 0 ] = _T('\0');
	}

	JabberEnterStringParams params = { this, type, caption, result, resultLen, windowName, recentCount, timeout };

	BOOL bRetVal = DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_GROUPCHAT_INPUT ), GetForegroundWindow(), sttEnterStringDlgProc, LPARAM( &params ));

	if (free_caption) mir_free( caption );

	return bRetVal;
}

////////////////////////////////////////////////////////////////////////
// Choose protocol instance
CJabberProto *JabberChooseInstance(bool bAllowOffline, bool atCursor)
{
	if (g_Instances.getCount() == 0) return NULL;
	if (g_Instances.getCount() == 1)
	{
		if (bAllowOffline || ((g_Instances[0]->m_iStatus != ID_STATUS_OFFLINE) && (g_Instances[0]->m_iStatus != ID_STATUS_CONNECTING)))
			return g_Instances[0];
		return NULL;
	}

	HMENU hMenu = JMenuCreate(true);

	int nItems = 0;
	int lastItemId = 0;
	for (int i = 0; i < g_Instances.getCount(); ++i)
	{
		if (bAllowOffline || ((g_Instances[i]->m_iStatus != ID_STATUS_OFFLINE) && (g_Instances[i]->m_iStatus != ID_STATUS_CONNECTING)))
		{
			++nItems;
			lastItemId = i+1;
			JMenuAddItem(hMenu, lastItemId,
				g_Instances[i]->m_tszUserName,
				LoadSkinnedProtoIcon(g_Instances[i]->m_szModuleName, g_Instances[i]->m_iStatus), false);
		}
	}

	int res = lastItemId;
	if (nItems > 1)
	{
		JMenuAddSeparator(hMenu);

		JMenuAddItem(hMenu, 0,
			TranslateT("Cancel"),
			LoadSkinnedIconHandle(SKINICON_OTHER_DELETE), true);

		res = JMenuShow(hMenu);
	}

	JMenuDestroy(hMenu, NULL, NULL);

	return res ? g_Instances[res-1] : NULL;
};

////////////////////////////////////////////////////////////////////////
// Premultiply bitmap channels for 32-bit bitmaps
void JabberBitmapPremultiplyChannels(HBITMAP hBitmap)
{
	BITMAP bmp;
	DWORD dwLen;
	BYTE *p;
	int x, y;

	GetObject(hBitmap, sizeof(bmp), &bmp);

	if (bmp.bmBitsPixel != 32)
		return;

	dwLen = bmp.bmWidth * bmp.bmHeight * (bmp.bmBitsPixel / 8);
	p = (BYTE *)malloc(dwLen);
	if (p == NULL)
		return;
	memset(p, 0, dwLen);

	GetBitmapBits(hBitmap, dwLen, p);

	for (y = 0; y < bmp.bmHeight; ++y)
	{
		BYTE *px = p + bmp.bmWidth * 4 * y;

		for (x = 0; x < bmp.bmWidth; ++x)
		{
			px[0] = px[0] * px[3] / 255;
			px[1] = px[1] * px[3] / 255;
			px[2] = px[2] * px[3] / 255;

			px += 4;
		}
	}

	SetBitmapBits(hBitmap, dwLen, p);

	free(p);
}
