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
#include "jabber_iq_handlers.h"

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

void CJabberProto::OnIqRequestVersion( XmlNode& node, void* userdata, CJabberIqInfo* pInfo )
{
	if ( !pInfo->GetFrom() )
		return;

	TCHAR* os = NULL;

	if ( JGetByte( "ShowOSVersion", TRUE )) {
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

	XmlNodeIq iq( "result", pInfo );
	XmlNode query = iq.addQuery( _T(JABBER_FEAT_VERSION));
	query.addChild( "name", _T("Miranda IM Jabber") );
	query.addChild( "version", _T(__VERSION_STRING) );
	if (os) query.addChild( "os", os );
	m_ThreadInfo->send( iq );
}

// last activity (XEP-0012) support
void CJabberProto::OnIqRequestLastActivity( XmlNode& node, void* userdata, CJabberIqInfo *pInfo )
{
	XmlNodeIq iq( "result", pInfo );
	XmlNode query = iq.addQuery( _T(JABBER_FEAT_LAST_ACTIVITY));
	query.addAttr( _T("seconds"), m_tmJabberIdleStartTime ? time( 0 ) - m_tmJabberIdleStartTime : 0 );
	m_ThreadInfo->send( iq );
}

// XEP-0199: XMPP Ping support
void CJabberProto::OnIqRequestPing( XmlNode& node, void* userdata, CJabberIqInfo *pInfo )
{
	XmlNodeIq iq( "result", pInfo );
	m_ThreadInfo->send( iq );
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
	case TIME_ZONE_ID_INVALID:
	default:
		nOffset = 0;
		break;
	}

	return nOffset;
}

// entity time (XEP-0202) support
void CJabberProto::OnIqRequestTime( XmlNode& node, void* userdata, CJabberIqInfo *pInfo )
{
	struct tm *gmt;
	time_t ltime;
	char stime[100];
	char szTZ[10];

	_tzset();
	time(&ltime);
	gmt = gmtime(&ltime);
	sprintf(stime,"%.4i-%.2i-%.2iT%.2i:%.2i:%.2iZ", gmt->tm_year + 1900, gmt->tm_mon + 1,
		gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec);

	int nGmtOffset = GetGMTOffset();
	ltime = abs(nGmtOffset);

	gmt = gmtime( &ltime );
	sprintf(szTZ, "%s%.2i:%.2i", nGmtOffset > 0 ? "+" : "-", gmt->tm_hour, gmt->tm_min );

	XmlNodeIq iq( "result", pInfo );
	XmlNode timeNode = iq.addChild( "time" );
	timeNode.addAttr( "xmlns", JABBER_FEAT_ENTITY_TIME );
	timeNode.addChild( "utc", stime);
	timeNode.addChild( "tzo", szTZ );
	m_ThreadInfo->send(iq);
}

void CJabberProto::OnIqRequestAvatar( XmlNode& node, void* userdata, CJabberIqInfo *pInfo )
{
	if ( !JGetByte( "EnableAvatars", TRUE ))
		return;

	int pictureType = JGetByte( "AvatarType", PA_FORMAT_UNKNOWN );
	if ( pictureType == PA_FORMAT_UNKNOWN )
		return;

	char* szMimeType;
	switch( pictureType ) {
		case PA_FORMAT_JPEG:	 szMimeType = "image/jpeg";   break;
		case PA_FORMAT_GIF:	 szMimeType = "image/gif";    break;
		case PA_FORMAT_PNG:	 szMimeType = "image/png";    break;
		case PA_FORMAT_BMP:	 szMimeType = "image/bmp";    break;
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
	XmlNodeIq iq( "result", pInfo );
	XmlNode query = iq.addQuery( _T(JABBER_FEAT_AVATAR));
	XmlNode data = query.addChild( "data", str ); data.addAttr( "mimetype", szMimeType );
	m_ThreadInfo->send( iq );
	mir_free( str );
	mir_free( buffer );
}

void CJabberProto::OnSiRequest( XmlNode& node, void* userdata, CJabberIqInfo *pInfo )
{
	const TCHAR* szProfile = pInfo->GetChildNode().getAttrValue( _T("profile"));

	if ( szProfile && !_tcscmp( szProfile, _T(JABBER_FEAT_SI_FT)))
		FtHandleSiRequest( node );
	else {
		XmlNodeIq iq( "error", pInfo );
		XmlNode error = iq.addChild( "error" ); error.addAttr( "code", "400" ); error.addAttr( "type", "cancel" );
		XmlNode brq = error.addChild( "bad-request" ); brq.addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );
		XmlNode bp = error.addChild( "bad-profile" ); 
		m_ThreadInfo->send( iq );
	}
}

void CJabberProto::OnRosterPushRequest( XmlNode& node, void* userdata, CJabberIqInfo *pInfo )
{
	XmlNode queryNode = pInfo->GetChildNode();

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

	Log( "<iq/> Got roster push, query has %d children", queryNode.getChildCount() );
	for ( int i=0; ; i++ ) {
		XmlNode itemNode = queryNode.getChild(i);
		if ( !itemNode )
			break;

		if ( _tcscmp( itemNode.getName(), _T("item")) != 0 )
			continue;
		if (( jid = itemNode.getAttrValue( _T("jid"))) == NULL )
			continue;
		if (( str = itemNode.getAttrValue( _T("subscription"))) == NULL )
			continue;

		// we will not add new account when subscription=remove
		if ( !_tcscmp( str, _T("to")) || !_tcscmp( str, _T("both")) || !_tcscmp( str, _T("from")) || !_tcscmp( str, _T("none"))) {
			if (( name = itemNode.getAttrValue( _T("name"))) != NULL )
				nick = mir_tstrdup( name );
			else
				nick = JabberNickFromJID( jid );

			if ( nick != NULL ) {
				if (( item=ListAdd( LIST_ROSTER, jid )) != NULL ) {
					replaceStr( item->nick, nick );

					XmlNode groupNode = itemNode.getChild( "group" );
					replaceStr( item->group, ( groupNode ) ? groupNode.getText() : NULL );

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

void CJabberProto::OnIqRequestOOB( XmlNode& node, void* userdata, CJabberIqInfo *pInfo )
{
	if ( !pInfo->GetFrom() || !pInfo->GetHContact() )
		return;

	XmlNode n = pInfo->GetChildNode().getChild( "url" );
	if ( !n || !n.getText())
		return;

	if ( JGetByte( "BsOnlyIBB", FALSE )) {
		// reject
		XmlNodeIq iq( "error", pInfo );
		XmlNode e = iq.addChild( _T("error"), _T("File transfer refused")); e.addAttr( _T("code"), 406 );
		m_ThreadInfo->send( iq );
		return;
	}

	TCHAR text[ 1024 ];
	TCHAR *str, *p, *q;
	
	str = ( TCHAR* )n.getText();	// URL of the file to get
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
		if (( n = pInfo->GetChildNode().getChild( "desc" ))!=NULL && n.getText()!=NULL )
			desc = mir_t2a( n.getText() );
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
		XmlNodeIq iq( "error", pInfo );
		XmlNode e = iq.addChild( _T("error"), _T("File transfer refused")); e.addAttr( _T("code"), 406 );
		m_ThreadInfo->send( iq );
		delete ft;
	}
}

void CJabberProto::OnHandleDiscoInfoRequest( XmlNode& iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if ( !pInfo->GetChildNode() )
		return;

	const TCHAR* szNode = pInfo->GetChildNode().getAttrValue( _T("node"));
	// caps hack
	if ( m_clientCapsManager.HandleInfoRequest( iqNode, userdata, pInfo, szNode ))
		return;

	// ad-hoc hack:
	if ( szNode && m_adhocManager.HandleInfoRequest( iqNode, userdata, pInfo, szNode ))
		return;

	// another request, send empty result
	XmlNodeIq iq( "error", pInfo );

	XmlNode errorNode = iq.addChild( "error" );
	errorNode.addAttr( "code", "404" );
	errorNode.addAttr( "type", "cancel" );
	XmlNode notfoundNode = errorNode.addChild( "item-not-found" );
	notfoundNode.addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );

	m_ThreadInfo->send( iq );
}

void CJabberProto::OnHandleDiscoItemsRequest( XmlNode& iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if ( !pInfo->GetChildNode() )
		return;

	// ad-hoc commands check:
	const TCHAR* szNode = pInfo->GetChildNode().getAttrValue( _T("node"));
	if ( szNode && m_adhocManager.HandleItemsRequest( iqNode, userdata, pInfo, szNode ))
		return;

	// another request, send empty result
	XmlNodeIq iq( "result", pInfo );
	XmlNode resultQuery = iq.addChild( "query" );
	resultQuery.addAttr( "xmlns", _T(JABBER_FEAT_DISCO_ITEMS));
	if ( szNode )
		resultQuery.addAttr( "node", szNode );

	if ( !szNode && JGetByte( "EnableRemoteControl", FALSE )) {
		XmlNode item = resultQuery.addChild( "item" );
		item.addAttr( "jid", m_ThreadInfo->fullJID );
		item.addAttr( "node", _T(JABBER_FEAT_COMMANDS) );
		item.addAttr( "name", "Ad-hoc commands" );
	}

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

void CJabberProto::OnIqHttpAuth( XmlNode& node, void* userdata, CJabberIqInfo* pInfo )
{
	if ( !JGetByte( "AcceptHttpAuth", TRUE ))
		return;

	if ( !node || !pInfo->GetChildNode() || !pInfo->GetFrom() || !pInfo->GetIdStr() )
		return;

	XmlNode pConfirm = node.getChild( "confirm" );
	if ( !pConfirm )
		return;

	const TCHAR *szId = pConfirm.getAttrValue( _T("id"));
	const TCHAR *szMethod = pConfirm.getAttrValue( _T("method"));
	const TCHAR *szUrl = pConfirm.getAttrValue( _T("url"));

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
