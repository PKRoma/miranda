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

#include <io.h>
#include "version.h"
#include "jabber_iq.h"
#include "jabber_rc.h"

extern int bSecureIM;


static int JGetMirandaProductText(WPARAM wParam,LPARAM lParam)
{
	char filename[MAX_PATH],*productName = NULL;
	DWORD unused;
	DWORD verInfoSize;
	UINT blockSize;
	PVOID pVerInfo;
	GetModuleFileNameA(NULL,filename,SIZEOF(filename));
	verInfoSize=GetFileVersionInfoSizeA(filename,&unused);
	pVerInfo=mir_alloc(verInfoSize);
	if( GetFileVersionInfoA(filename,0,verInfoSize,pVerInfo) ) {
		VerQueryValueA(pVerInfo,"\\StringFileInfo\\000004b0\\ProductName",(void**)&productName,&blockSize);
		if( productName )
		#if defined( _UNICODE )
			mir_snprintf(( char* )lParam, wParam, "%s", productName );
		#else
			lstrcpynA((char*)lParam,productName,wParam);
		#endif
		else
			*(char*)lParam = 0;
		mir_free(pVerInfo);
	} else
		*(char*)lParam = 0;
	return 0;
}

void CJabberProto::OnIqRequestVersion( HXML node, void* userdata, CJabberIqInfo* pInfo )
{
	if ( !pInfo->GetFrom() )
		return;

	TCHAR* os = NULL;

	if ( m_options.ShowOSVersion ) {
		OSVERSIONINFO osvi = { 0 };
		osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
		if ( GetVersionEx( &osvi )) {
			switch ( osvi.dwPlatformId ) {
			case VER_PLATFORM_WIN32_NT:
				if ( osvi.dwMajorVersion == 6 )
					os = TranslateT( "Windows Vista" );
				else if ( osvi.dwMajorVersion == 5 ) {
					if ( osvi.dwMinorVersion == 2 ) os = TranslateT( "Windows Server 2003" );
					else if ( osvi.dwMinorVersion == 1 ) os = TranslateT( "Windows XP" );
					else if ( osvi.dwMinorVersion == 0 ) os = TranslateT( "Windows 2000" );
				}
				else if ( osvi.dwMajorVersion <= 4 ) {
					os = TranslateT( "Windows NT" );
				}
				break;
			case VER_PLATFORM_WIN32_WINDOWS:
				if ( osvi.dwMajorVersion == 4 ) {
					if ( osvi.dwMinorVersion == 0 ) os = TranslateT( "Windows 95" );
					if ( osvi.dwMinorVersion == 10 ) os = TranslateT( "Windows 98" );
					if ( osvi.dwMinorVersion == 90 ) os = TranslateT( "Windows ME" );
				}
				break;
		}	}

		if ( os == NULL ) os = TranslateT( "Windows" );
	}

	XmlNodeIq iq( _T("result"), pInfo );
	HXML query = iq << XQUERY( _T(JABBER_FEAT_VERSION));
	query << XCHILD( _T("name"), _T("Miranda IM Jabber") );
	query << XCHILD( _T("version"), _T(__VERSION_STRING) );
	if ( os ) 
		query << XCHILD( _T("os"), os );
	m_ThreadInfo->send( iq );
}

// last activity (XEP-0012) support
void CJabberProto::OnIqRequestLastActivity( HXML node, void* userdata, CJabberIqInfo *pInfo )
{
	m_ThreadInfo->send(
		XmlNodeIq( _T("result"), pInfo ) << XQUERY( _T(JABBER_FEAT_LAST_ACTIVITY))
			<< XATTRI( _T("seconds"), m_tmJabberIdleStartTime ? time( 0 ) - m_tmJabberIdleStartTime : 0 ));
}

// XEP-0199: XMPP Ping support
void CJabberProto::OnIqRequestPing( HXML node, void* userdata, CJabberIqInfo *pInfo )
{
	m_ThreadInfo->send( XmlNodeIq( _T("result"), pInfo ));
}

// Returns the current GMT offset in seconds
int GetGMTOffset(void)
{
	TIME_ZONE_INFORMATION tzinfo;
	int nOffset = 0;

	DWORD dwResult= GetTimeZoneInformation(&tzinfo);

	switch(dwResult) {
	case TIME_ZONE_ID_STANDARD:
		nOffset = -(tzinfo.Bias + tzinfo.StandardBias) * 60;
		break;
	case TIME_ZONE_ID_DAYLIGHT:
		nOffset = -(tzinfo.Bias + tzinfo.DaylightBias) * 60;
		break;
	case TIME_ZONE_ID_UNKNOWN:
		nOffset = -(tzinfo.Bias) * 60;
		break;
	case TIME_ZONE_ID_INVALID:
	default:
		nOffset = 0;
		break;
	}

	return nOffset;
}

// entity time (XEP-0202) support
void CJabberProto::OnIqRequestTime( HXML node, void* userdata, CJabberIqInfo *pInfo )
{
	struct tm *gmt;
	time_t ltime;
	TCHAR stime[100];
	TCHAR szTZ[10];

	_tzset();
	time(&ltime);
	gmt = gmtime(&ltime);
	wsprintf(stime,_T("%.4i-%.2i-%.2iT%.2i:%.2i:%.2iZ"), 
		gmt->tm_year + 1900, gmt->tm_mon + 1,
		gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec);

	int nGmtOffset = GetGMTOffset();
	ltime = abs(nGmtOffset);

	gmt = gmtime( &ltime );
	wsprintf(szTZ, _T("%s%.2i:%.2i"), nGmtOffset > 0 ? "+" : "-", gmt->tm_hour, gmt->tm_min );

	XmlNodeIq iq( _T("result"), pInfo );
	HXML timeNode = iq << XCHILDNS( _T("time"), _T(JABBER_FEAT_ENTITY_TIME));
	timeNode << XCHILD( _T("utc"), stime); timeNode << XCHILD( _T("tzo"), szTZ );
	m_ThreadInfo->send( iq );
}

void CJabberProto::OnIqRequestAvatar( HXML node, void* userdata, CJabberIqInfo *pInfo )
{
	if ( !m_options.EnableAvatars )
		return;

	int pictureType = m_options.AvatarType;
	if ( pictureType == PA_FORMAT_UNKNOWN )
		return;

	TCHAR* szMimeType;
	switch( pictureType ) {
		case PA_FORMAT_JPEG:	 szMimeType = _T("image/jpeg");   break;
		case PA_FORMAT_GIF:	 szMimeType = _T("image/gif");    break;
		case PA_FORMAT_PNG:	 szMimeType = _T("image/png");    break;
		case PA_FORMAT_BMP:	 szMimeType = _T("image/bmp");    break;
		default:	return;
	}

	char szFileName[ MAX_PATH ];
	GetAvatarFileName( NULL, szFileName, MAX_PATH );

	FILE* in = fopen( szFileName, "rb" );
	if ( in == NULL )
		return;

	long bytes = _filelength( _fileno( in ));
	char* buffer = ( char* )mir_alloc( bytes*4/3 + bytes + 1000 );
	if ( buffer == NULL ) {
		fclose( in );
		return;
	}

	fread( buffer, bytes, 1, in );
	fclose( in );

	char* str = JabberBase64Encode( buffer, bytes );
	m_ThreadInfo->send( XmlNodeIq( _T("result"), pInfo ) << XQUERY( _T(JABBER_FEAT_AVATAR)) << XCHILD( _T("query"), _A2T(str)) << XATTR( _T("mimetype"), szMimeType ));
	mir_free( str );
	mir_free( buffer );
}

void CJabberProto::OnSiRequest( HXML node, void* userdata, CJabberIqInfo *pInfo )
{
	const TCHAR* szProfile = xmlGetAttrValue( pInfo->GetChildNode(), _T("profile"));

	if ( szProfile && !_tcscmp( szProfile, _T(JABBER_FEAT_SI_FT)))
		FtHandleSiRequest( node );
	else {
		XmlNodeIq iq( _T("error"), pInfo );
		HXML error = iq << XCHILD( _T("error")) << XATTRI( _T("code"), 400 ) << XATTR( _T("type"), _T("cancel"));
		error << XCHILDNS( _T("bad-request"), _T("urn:ietf:params:xml:ns:xmpp-stanzas"));
		error << XCHILD( _T("bad-profile")); 
		m_ThreadInfo->send( iq );
	}
}

void CJabberProto::OnRosterPushRequest( HXML node, void* userdata, CJabberIqInfo *pInfo )
{
	HXML queryNode = pInfo->GetChildNode();

	// RFC 3921 #7.2 Business Rules
	if ( pInfo->GetFrom() ) {
		TCHAR* szFrom = JabberPrepareJid( pInfo->GetFrom() );
		if ( !szFrom )
			return;

		TCHAR* szTo = JabberPrepareJid( m_ThreadInfo->fullJID );
		if ( !szTo ) {
			mir_free( szFrom );
			return;
		}

		TCHAR* pDelimiter = _tcschr( szFrom, _T('/') );
		if ( pDelimiter ) *pDelimiter = _T('\0');

		pDelimiter = _tcschr( szTo, _T('/') );
		if ( pDelimiter ) *pDelimiter = _T('\0');

		BOOL bRetVal = _tcscmp( szFrom, szTo ) == 0;

		mir_free( szFrom );
		mir_free( szTo );

		// invalid JID
		if ( !bRetVal ) {
			Log( "<iq/> attempt to hack via roster push from " TCHAR_STR_PARAM, pInfo->GetFrom() );
			return;
		}
	}

	JABBER_LIST_ITEM *item;
	HANDLE hContact = NULL;
	const TCHAR *jid, *str, *name;
	TCHAR* nick;

	Log( "<iq/> Got roster push, query has %d children", xmlGetChildCount( queryNode ));
	for ( int i=0; ; i++ ) {
		HXML itemNode = xmlGetChild( queryNode ,i);
		if ( !itemNode )
			break;

		if ( _tcscmp( xmlGetName( itemNode ), _T("item")) != 0 )
			continue;
		if (( jid = xmlGetAttrValue( itemNode, _T("jid"))) == NULL )
			continue;
		if (( str = xmlGetAttrValue( itemNode, _T("subscription"))) == NULL )
			continue;

		// we will not add new account when subscription=remove
		if ( !_tcscmp( str, _T("to")) || !_tcscmp( str, _T("both")) || !_tcscmp( str, _T("from")) || !_tcscmp( str, _T("none"))) {
			if (( name = xmlGetAttrValue( itemNode, _T("name"))) != NULL )
				nick = mir_tstrdup( name );
			else
				nick = JabberNickFromJID( jid );

			if ( nick != NULL ) {
				if (( item=ListAdd( LIST_ROSTER, jid )) != NULL ) {
					replaceStr( item->nick, nick );

					HXML groupNode = xmlGetChild( itemNode , "group" );
					replaceStr( item->group, ( groupNode ) ? xmlGetText( groupNode ) : NULL );

					if (( hContact=HContactFromJID( jid, 0 )) == NULL ) {
						// Received roster has a new JID.
						// Add the jid ( with empty resource ) to Miranda contact list.
						hContact = DBCreateContact( jid, nick, FALSE, FALSE );
					}
					else JSetStringT( hContact, "jid", jid );

					if ( name != NULL ) {
						DBVARIANT dbnick;
						if ( !JGetStringT( hContact, "Nick", &dbnick )) {
							if ( _tcscmp( nick, dbnick.ptszVal ) != 0 )
								DBWriteContactSettingTString( hContact, "CList", "MyHandle", nick );
							else
								DBDeleteContactSetting( hContact, "CList", "MyHandle" );

							JFreeVariant( &dbnick );
						}
						else DBWriteContactSettingTString( hContact, "CList", "MyHandle", nick );
					}
					else DBDeleteContactSetting( hContact, "CList", "MyHandle" );

					if ( item->group != NULL ) {
						JabberContactListCreateGroup( item->group );
						DBWriteContactSettingTString( hContact, "CList", "Group", item->group );
					}
					else DBDeleteContactSetting( hContact, "CList", "Group" );
				}
				mir_free( nick );
		}	}

		if (( item=ListGetItemPtr( LIST_ROSTER, jid )) != NULL ) {
			if ( !_tcscmp( str, _T("both"))) item->subscription = SUB_BOTH;
			else if ( !_tcscmp( str, _T("to"))) item->subscription = SUB_TO;
			else if ( !_tcscmp( str, _T("from"))) item->subscription = SUB_FROM;
			else item->subscription = SUB_NONE;
			Log( "Roster push for jid=" TCHAR_STR_PARAM ", set subscription to " TCHAR_STR_PARAM, jid, str );
			// subscription = remove is to remove from roster list
			// but we will just set the contact to offline and not actually
			// remove, so that history will be retained.
			if ( !_tcscmp( str, _T("remove"))) {
				if (( hContact=HContactFromJID( jid )) != NULL ) {
					SetContactOfflineStatus( hContact );
					ListRemove( LIST_ROSTER, jid );
			}	}
			else if ( JGetByte( hContact, "ChatRoom", 0 ))
				DBDeleteContactSetting( hContact, "CList", "Hidden" );
	}	}

	UI_SAFE_NOTIFY(m_pDlgServiceDiscovery, WM_JABBER_TRANSPORT_REFRESH);
	RebuildInfoFrame();
}

void CJabberProto::OnIqRequestOOB( HXML node, void* userdata, CJabberIqInfo *pInfo )
{
	if ( !pInfo->GetFrom() || !pInfo->GetHContact() )
		return;

	HXML n = xmlGetChild( pInfo->GetChildNode(), "url" );
	if ( !n || !xmlGetText( n ))
		return;

	if ( m_options.BsOnlyIBB ) {
		// reject
		XmlNodeIq iq( _T("error"), pInfo );
		HXML e = xmlAddChild( iq, _T("error"), _T("File transfer refused")); xmlAddAttr( e, _T("code"), 406 );
		m_ThreadInfo->send( iq );
		return;
	}

	TCHAR text[ 1024 ];
	TCHAR *str, *p, *q;
	
	str = ( TCHAR* )xmlGetText( n );	// URL of the file to get
	filetransfer* ft = new filetransfer( this );
	ft->std.totalFiles = 1;
	ft->jid = mir_tstrdup( pInfo->GetFrom() );
	ft->std.hContact = pInfo->GetHContact();
	ft->type = FT_OOB;
	ft->httpHostName = NULL;
	ft->httpPort = 80;
	ft->httpPath = NULL;

	// Parse the URL
	if ( !_tcsnicmp( str, _T("http://"), 7 )) {
		p = str + 7;
		if (( q = _tcschr( p, '/' )) != NULL ) {
			if ( q-p < SIZEOF( text )) {
				_tcsncpy( text, p, q-p );
				text[q-p] = '\0';
				if (( p = _tcschr( text, ':' )) != NULL ) {
					ft->httpPort = ( WORD )_ttoi( p+1 );
					*p = '\0';
				}
				ft->httpHostName = mir_t2a( text );
	}	}	}

	if ( pInfo->GetIdStr() )
		ft->iqId = mir_tstrdup( pInfo->GetIdStr() );

	if ( ft->httpHostName && ft->httpPath ) {
		CCSDATA ccs;
		PROTORECVEVENT pre;
		char* szBlob, *desc;

		Log( "Host=%s Port=%d Path=%s", ft->httpHostName, ft->httpPort, ft->httpPath );
		if (( n = xmlGetChild( pInfo->GetChildNode(), "desc" ))!=NULL && xmlGetText( n )!=NULL )
			desc = mir_t2a( xmlGetText( n ) );
		else
			desc = mir_strdup( "" );

		if ( desc != NULL ) {
			char* str2;
			Log( "description = %s", desc );
			if (( str2 = strrchr( ft->httpPath, '/' )) != NULL )
				str2++;
			else
				str2 = ft->httpPath;
			str2 = mir_strdup( str2 );
			JabberHttpUrlDecode( str2 );
			szBlob = ( char* )mir_alloc( sizeof( DWORD )+ strlen( str2 ) + strlen( desc ) + 2 );
			*(( PDWORD ) szBlob ) = ( DWORD )ft;
			strcpy( szBlob + sizeof( DWORD ), str2 );
			strcpy( szBlob + sizeof( DWORD )+ strlen( str2 ) + 1, desc );
			pre.flags = 0;
			pre.timestamp = time( NULL );
			pre.szMessage = szBlob;
			pre.lParam = 0;
			ccs.szProtoService = PSR_FILE;
			ccs.hContact = ft->std.hContact;
			ccs.wParam = 0;
			ccs.lParam = ( LPARAM )&pre;
			JCallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );
			mir_free( szBlob );
			mir_free( str2 );
			mir_free( desc );
		}
	}
	else {
		// reject
		XmlNodeIq iq( _T("error"), pInfo );
		HXML e = xmlAddChild( iq, _T("error"), _T("File transfer refused")); xmlAddAttr( e, _T("code"), 406 );
		m_ThreadInfo->send( iq );
		delete ft;
	}
}

void CJabberProto::OnHandleDiscoInfoRequest( HXML iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if ( !pInfo->GetChildNode() )
		return;

	const TCHAR* szNode = xmlGetAttrValue( pInfo->GetChildNode(), _T("node"));
	// caps hack
	if ( m_clientCapsManager.HandleInfoRequest( iqNode, userdata, pInfo, szNode ))
		return;

	// ad-hoc hack:
	if ( szNode && m_adhocManager.HandleInfoRequest( iqNode, userdata, pInfo, szNode ))
		return;

	// another request, send empty result
	m_ThreadInfo->send(
		XmlNodeIq( _T("error"), pInfo )
			<< XCHILD( _T("error")) << XATTRI( _T("code"), 404 ) << XATTR( _T("type"), _T("cancel"))
				<< XCHILDNS( _T("item-not-found"), _T("urn:ietf:params:xml:ns:xmpp-stanzas")));
}

void CJabberProto::OnHandleDiscoItemsRequest( HXML iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if ( !pInfo->GetChildNode() )
		return;

	// ad-hoc commands check:
	const TCHAR* szNode = xmlGetAttrValue( pInfo->GetChildNode(), _T("node"));
	if ( szNode && m_adhocManager.HandleItemsRequest( iqNode, userdata, pInfo, szNode ))
		return;

	// another request, send empty result
	XmlNodeIq iq( _T("result"), pInfo );
	HXML resultQuery = iq << XQUERY( _T(JABBER_FEAT_DISCO_ITEMS));
	if ( szNode )
		xmlAddAttr( resultQuery, _T("node"), szNode );

	if ( !szNode && m_options.EnableRemoteControl )
		resultQuery << XCHILD( _T("item")) << XATTR( _T("jid"), m_ThreadInfo->fullJID ) 
			<< XATTR( _T("node"), _T(JABBER_FEAT_COMMANDS)) << XATTR( _T("name"), _T("Ad-hoc commands"));

	m_ThreadInfo->send( iq );
}

BOOL CJabberProto::AddClistHttpAuthEvent( CJabberHttpAuthParams *pParams )
{
	CLISTEVENT cle = {0};
	char szService[256];
	mir_snprintf( szService, sizeof(szService),"%s%s", m_szModuleName, JS_HTTP_AUTH );
	cle.cbSize = sizeof(CLISTEVENT);
	cle.hIcon = (HICON) LoadIconEx("openid");
	cle.flags = CLEF_PROTOCOLGLOBAL | CLEF_TCHAR;
	cle.hDbEvent = (HANDLE)("test");
	cle.lParam = (LPARAM) pParams;
	cle.pszService = szService;
	cle.ptszTooltip = TranslateT("Http authentication request received");
	CallService(MS_CLIST_ADDEVENT, 0, (LPARAM)&cle);
	
	return TRUE;
}

void CJabberProto::OnIqHttpAuth( HXML node, void* userdata, CJabberIqInfo* pInfo )
{
	if ( !m_options.AcceptHttpAuth )
		return;

	if ( !node || !pInfo->GetChildNode() || !pInfo->GetFrom() || !pInfo->GetIdStr() )
		return;

	HXML pConfirm = xmlGetChild( node , "confirm" );
	if ( !pConfirm )
		return;

	const TCHAR *szId = xmlGetAttrValue( pConfirm, _T("id"));
	const TCHAR *szMethod = xmlGetAttrValue( pConfirm, _T("method"));
	const TCHAR *szUrl = xmlGetAttrValue( pConfirm, _T("url"));

	if ( !szId || !szMethod || !szUrl )
		return;

	CJabberHttpAuthParams *pParams = (CJabberHttpAuthParams *)mir_alloc( sizeof( CJabberHttpAuthParams ));
	if ( !pParams )
		return;
	ZeroMemory( pParams, sizeof( CJabberHttpAuthParams ));
	pParams->m_nType = CJabberHttpAuthParams::IQ;
	pParams->m_szFrom = mir_tstrdup( pInfo->GetFrom() );
	pParams->m_szId = mir_tstrdup( szId );
	pParams->m_szMethod = mir_tstrdup( szMethod );
	pParams->m_szUrl = mir_tstrdup( szUrl );

	AddClistHttpAuthEvent( pParams );

	return;
}
