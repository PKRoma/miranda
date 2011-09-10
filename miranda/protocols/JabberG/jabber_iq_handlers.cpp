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

#include <io.h>
#include "version.h"
#include "jabber_iq.h"
#include "jabber_rc.h"

extern int bSecureIM;

#ifndef VER_SUITE_WH_SERVER
	#define VER_SUITE_WH_SERVER                     0x00008000
#endif

#ifndef PRODUCT_ULTIMATE
	#define PRODUCT_UNDEFINED                       0x00000000
	#define PRODUCT_ULTIMATE                        0x00000001
	#define PRODUCT_HOME_BASIC                      0x00000002
	#define PRODUCT_HOME_PREMIUM                    0x00000003
	#define PRODUCT_ENTERPRISE                      0x00000004
	#define PRODUCT_HOME_BASIC_N                    0x00000005
	#define PRODUCT_BUSINESS                        0x00000006
	#define PRODUCT_STANDARD_SERVER                 0x00000007
	#define PRODUCT_DATACENTER_SERVER               0x00000008
	#define PRODUCT_SMALLBUSINESS_SERVER            0x00000009
	#define PRODUCT_ENTERPRISE_SERVER               0x0000000A
	#define PRODUCT_STARTER                         0x0000000B
	#define PRODUCT_DATACENTER_SERVER_CORE          0x0000000C
	#define PRODUCT_STANDARD_SERVER_CORE            0x0000000D
	#define PRODUCT_ENTERPRISE_SERVER_CORE          0x0000000E
	#define PRODUCT_ENTERPRISE_SERVER_IA64          0x0000000F
	#define PRODUCT_BUSINESS_N                      0x00000010
	#define PRODUCT_WEB_SERVER                      0x00000011
	#define PRODUCT_CLUSTER_SERVER                  0x00000012
	#define PRODUCT_HOME_SERVER                     0x00000013
	#define PRODUCT_STORAGE_EXPRESS_SERVER          0x00000014
	#define PRODUCT_STORAGE_STANDARD_SERVER         0x00000015
	#define PRODUCT_STORAGE_WORKGROUP_SERVER        0x00000016
	#define PRODUCT_STORAGE_ENTERPRISE_SERVER       0x00000017
	#define PRODUCT_SERVER_FOR_SMALLBUSINESS        0x00000018
	#define PRODUCT_SMALLBUSINESS_SERVER_PREMIUM    0x00000019
	#define PRODUCT_UNLICENSED                      0xABCDABCD
#endif

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

#define StringCchCopy(x,y,z)      lstrcpyn((x),(z),(y))
#define StringCchCat(x,y,z)       lstrcat((x),(z))
#define StringCchPrintf           mir_sntprintf

// slightly modified sample from MSDN
BOOL GetOSDisplayString(LPTSTR pszOS, int BUFSIZE)
{
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	PGNSI pGNSI;
	PGPI pGPI;
	BOOL bOsVersionInfoEx;
	DWORD dwType;

	ZeroMemory(&si, sizeof(SYSTEM_INFO));
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
		return FALSE;

	// Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.

	pGNSI = (PGNSI) GetProcAddress(
		GetModuleHandle(TEXT("kernel32.dll")), 
		"GetNativeSystemInfo");
	if(NULL != pGNSI)
		pGNSI(&si);
	else GetSystemInfo(&si);

	if ( VER_PLATFORM_WIN32_NT==osvi.dwPlatformId && 
		osvi.dwMajorVersion > 4 )
	{
		StringCchCopy(pszOS, BUFSIZE, TEXT("Microsoft "));

		// Test for the specific product.

		if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1 )
		{
			if( osvi.wProductType == VER_NT_WORKSTATION )
				StringCchCat(pszOS, BUFSIZE, TEXT("Windows 7 "));
			else StringCchCat(pszOS, BUFSIZE, TEXT("Windows Server 2008 R2 " ));
		}

		if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 )
		{
			if( osvi.wProductType == VER_NT_WORKSTATION )
				StringCchCat(pszOS, BUFSIZE, TEXT("Windows Vista "));
			else StringCchCat(pszOS, BUFSIZE, TEXT("Windows Server 2008 " ));

			pGPI = (PGPI) GetProcAddress(
				GetModuleHandle(TEXT("kernel32.dll")), 
				"GetProductInfo");

			pGPI( 6, 0, 0, 0, &dwType);

			switch( dwType )
			{
			case PRODUCT_ULTIMATE:
				StringCchCat(pszOS, BUFSIZE, TEXT("Ultimate Edition" ));
				break;
			case PRODUCT_HOME_PREMIUM:
				StringCchCat(pszOS, BUFSIZE, TEXT("Home Premium Edition" ));
				break;
			case PRODUCT_HOME_BASIC:
				StringCchCat(pszOS, BUFSIZE, TEXT("Home Basic Edition" ));
				break;
			case PRODUCT_ENTERPRISE:
				StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition" ));
				break;
			case PRODUCT_BUSINESS:
				StringCchCat(pszOS, BUFSIZE, TEXT("Business Edition" ));
				break;
			case PRODUCT_STARTER:
				StringCchCat(pszOS, BUFSIZE, TEXT("Starter Edition" ));
				break;
			case PRODUCT_CLUSTER_SERVER:
				StringCchCat(pszOS, BUFSIZE, TEXT("Cluster Server Edition" ));
				break;
			case PRODUCT_DATACENTER_SERVER:
				StringCchCat(pszOS, BUFSIZE, TEXT("Datacenter Edition" ));
				break;
			case PRODUCT_DATACENTER_SERVER_CORE:
				StringCchCat(pszOS, BUFSIZE, TEXT("Datacenter Edition (core installation)" ));
				break;
			case PRODUCT_ENTERPRISE_SERVER:
				StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition" ));
				break;
			case PRODUCT_ENTERPRISE_SERVER_CORE:
				StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition (core installation)" ));
				break;
			case PRODUCT_ENTERPRISE_SERVER_IA64:
				StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition for Itanium-based Systems" ));
				break;
			case PRODUCT_SMALLBUSINESS_SERVER:
				StringCchCat(pszOS, BUFSIZE, TEXT("Small Business Server" ));
				break;
			case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
				StringCchCat(pszOS, BUFSIZE, TEXT("Small Business Server Premium Edition" ));
				break;
			case PRODUCT_STANDARD_SERVER:
				StringCchCat(pszOS, BUFSIZE, TEXT("Standard Edition" ));
				break;
			case PRODUCT_STANDARD_SERVER_CORE:
				StringCchCat(pszOS, BUFSIZE, TEXT("Standard Edition (core installation)" ));
				break;
			case PRODUCT_WEB_SERVER:
				StringCchCat(pszOS, BUFSIZE, TEXT("Web Server Edition" ));
				break;
			}
			if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
				StringCchCat(pszOS, BUFSIZE, TEXT( ", 64-bit" ));
			else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
				StringCchCat(pszOS, BUFSIZE, TEXT(", 32-bit"));
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
		{
			if( GetSystemMetrics(SM_SERVERR2) )
				StringCchCat(pszOS, BUFSIZE, TEXT( "Windows Server 2003 R2, "));
			else if ( osvi.wSuiteMask==VER_SUITE_STORAGE_SERVER )
				StringCchCat(pszOS, BUFSIZE, TEXT( "Windows Storage Server 2003"));
			else if ( osvi.wSuiteMask==VER_SUITE_WH_SERVER )
				StringCchCat(pszOS, BUFSIZE, TEXT( "Windows Home Server"));
			else if( osvi.wProductType == VER_NT_WORKSTATION &&
				si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
			{
				StringCchCat(pszOS, BUFSIZE, TEXT( "Windows XP Professional x64 Edition"));
			}
			else StringCchCat(pszOS, BUFSIZE, TEXT("Windows Server 2003, "));

			// Test for the server type.
			if ( osvi.wProductType != VER_NT_WORKSTATION )
			{
				if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 )
				{
					if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
						StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter Edition for Itanium-based Systems" ));
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						StringCchCat(pszOS, BUFSIZE, TEXT( "Enterprise Edition for Itanium-based Systems" ));
				}

				else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
				{
					if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
						StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter x64 Edition" ));
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						StringCchCat(pszOS, BUFSIZE, TEXT( "Enterprise x64 Edition" ));
					else StringCchCat(pszOS, BUFSIZE, TEXT( "Standard x64 Edition" ));
				}

				else
				{
					if ( osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER )
						StringCchCat(pszOS, BUFSIZE, TEXT( "Compute Cluster Edition" ));
					else if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
						StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter Edition" ));
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						StringCchCat(pszOS, BUFSIZE, TEXT( "Enterprise Edition" ));
					else if ( osvi.wSuiteMask & VER_SUITE_BLADE )
						StringCchCat(pszOS, BUFSIZE, TEXT( "Web Edition" ));
					else StringCchCat(pszOS, BUFSIZE, TEXT( "Standard Edition" ));
				}
			}
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
		{
			StringCchCat(pszOS, BUFSIZE, TEXT("Windows XP "));
			if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
				StringCchCat(pszOS, BUFSIZE, TEXT( "Home Edition" ));
			else StringCchCat(pszOS, BUFSIZE, TEXT( "Professional" ));
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
		{
			StringCchCat(pszOS, BUFSIZE, TEXT("Windows 2000 "));

			if ( osvi.wProductType == VER_NT_WORKSTATION )
			{
				StringCchCat(pszOS, BUFSIZE, TEXT( "Professional" ));
			}
			else 
			{
				if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter Server" ));
				else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					StringCchCat(pszOS, BUFSIZE, TEXT( "Advanced Server" ));
				else StringCchCat(pszOS, BUFSIZE, TEXT( "Server" ));
			}
		}

		// Include service pack (if any) and build number.

		if( _tcslen(osvi.szCSDVersion) > 0 )
		{
			StringCchCat(pszOS, BUFSIZE, TEXT(" ") );
			StringCchCat(pszOS, BUFSIZE, osvi.szCSDVersion);
		}

		TCHAR buf[80];

		StringCchPrintf( buf, 80, TEXT(" (build %d)"), osvi.dwBuildNumber);
		StringCchCat(pszOS, BUFSIZE, buf);

		return TRUE; 
	}
	else
	{  
		return FALSE;
	}
}


BOOL CJabberProto::OnIqRequestVersion( HXML, CJabberIqInfo* pInfo )
{
	if ( !pInfo->GetFrom() )
		return TRUE;

	if ( !m_options.AllowVersionRequests )
		return FALSE;

	XmlNodeIq iq( _T("result"), pInfo );
	HXML query = iq << XQUERY( _T(JABBER_FEAT_VERSION));
	#if defined( _UNICODE )
		query << XCHILD( _T("name"), _T("Miranda IM Jabber (Unicode)") );
	#else
		query << XCHILD( _T("name"), _T("Miranda IM Jabber") );
	#endif
	query << XCHILD( _T("version"), _T(__VERSION_STRING) );

	if ( m_options.ShowOSVersion )
	{
		TCHAR os[256] = {0};
		if (!GetOSDisplayString(os, SIZEOF(os)))
			lstrcpyn(os, _T("Microsoft Windows"), SIZEOF(os));
		query << XCHILD( _T("os"), os );
	}

	m_ThreadInfo->send( iq );
	return TRUE;
}

// last activity (XEP-0012) support
BOOL CJabberProto::OnIqRequestLastActivity( HXML, CJabberIqInfo *pInfo )
{
	m_ThreadInfo->send(
		XmlNodeIq( _T("result"), pInfo ) << XQUERY( _T(JABBER_FEAT_LAST_ACTIVITY))
			<< XATTRI( _T("seconds"), m_tmJabberIdleStartTime ? time( 0 ) - m_tmJabberIdleStartTime : 0 ));
	return TRUE;
}

// XEP-0199: XMPP Ping support
BOOL CJabberProto::OnIqRequestPing( HXML, CJabberIqInfo *pInfo )
{
	m_ThreadInfo->send( XmlNodeIq( _T("result"), pInfo ));
	return TRUE;
}

// Returns the current GMT offset in seconds
int GetGMTOffset(void)
{
	TIME_ZONE_INFORMATION tzinfo;
	int nOffset = 0;

	DWORD dwResult= GetTimeZoneInformation(&tzinfo);

	switch(dwResult) {
	case TIME_ZONE_ID_STANDARD:
		nOffset = tzinfo.Bias + tzinfo.StandardBias;
		break;
	case TIME_ZONE_ID_DAYLIGHT:
		nOffset = tzinfo.Bias + tzinfo.DaylightBias;
		break;
	case TIME_ZONE_ID_UNKNOWN:
		nOffset = tzinfo.Bias;
		break;
	case TIME_ZONE_ID_INVALID:
	default:
		nOffset = 0;
		break;
	}

	return -nOffset;
}

// entity time (XEP-0202) support
BOOL CJabberProto::OnIqRequestTime( HXML, CJabberIqInfo *pInfo )
{
	TCHAR stime[100];
	TCHAR szTZ[10];

	tmi.printDateTime(UTC_TIME_HANDLE, _T("I"), stime, SIZEOF(stime), 0);

	int nGmtOffset = GetGMTOffset();
	mir_sntprintf(szTZ, SIZEOF(szTZ), _T("%+03d:%02d"), nGmtOffset / 60, nGmtOffset % 60 );

	XmlNodeIq iq( _T("result"), pInfo );
	HXML timeNode = iq << XCHILDNS( _T("time"), _T(JABBER_FEAT_ENTITY_TIME));
	timeNode << XCHILD( _T("utc"), stime); timeNode << XCHILD( _T("tzo"), szTZ );
	LPCTSTR szTZName = tmi.getTzName( NULL );
	if ( szTZName )
		timeNode << XCHILD( _T("tz"), szTZName );
	m_ThreadInfo->send( iq );
	return TRUE;
}

BOOL CJabberProto::OnIqProcessIqOldTime( HXML, CJabberIqInfo *pInfo )
{
	struct tm *gmt;
	time_t ltime;
	TCHAR stime[ 100 ], *dtime;

	_tzset();
	time( &ltime );
	gmt = gmtime( &ltime );
	mir_sntprintf( stime, SIZEOF(stime), _T("%.4i%.2i%.2iT%.2i:%.2i:%.2i"),
		gmt->tm_year + 1900, gmt->tm_mon + 1,
		gmt->tm_mday, gmt->tm_hour, gmt->tm_min, gmt->tm_sec );
	dtime = _tctime( &ltime );
	dtime[ 24 ] = 0;

	XmlNodeIq iq( _T("result"), pInfo );
	HXML queryNode = iq << XQUERY( _T(JABBER_FEAT_ENTITY_TIME_OLD) );
	queryNode << XCHILD( _T("utc"), stime );
	LPCTSTR szTZName = tmi.getTzName( NULL );
	if ( szTZName )
		queryNode << XCHILD( _T("tz"), szTZName );
	queryNode << XCHILD( _T("display"), dtime );
	m_ThreadInfo->send( iq );
	return TRUE;
}

BOOL CJabberProto::OnIqRequestAvatar( HXML, CJabberIqInfo *pInfo )
{
	if ( !m_options.EnableAvatars )
		return TRUE;

	int pictureType = m_options.AvatarType;
	if ( pictureType == PA_FORMAT_UNKNOWN )
		return TRUE;

	TCHAR* szMimeType;
	switch( pictureType ) {
		case PA_FORMAT_JPEG:	 szMimeType = _T("image/jpeg");   break;
		case PA_FORMAT_GIF:	 szMimeType = _T("image/gif");    break;
		case PA_FORMAT_PNG:	 szMimeType = _T("image/png");    break;
		case PA_FORMAT_BMP:	 szMimeType = _T("image/bmp");    break;
		default:	return TRUE;
	}

	TCHAR szFileName[ MAX_PATH ];
	GetAvatarFileName( NULL, szFileName, SIZEOF(szFileName));

	FILE* in = _tfopen( szFileName, _T("rb"));
	if ( in == NULL )
		return TRUE;

	long bytes = _filelength( _fileno( in ));
	char* buffer = ( char* )mir_alloc( bytes*4/3 + bytes + 1000 );
	if ( buffer == NULL ) {
		fclose( in );
		return TRUE;
	}

	fread( buffer, bytes, 1, in );
	fclose( in );

	char* str = JabberBase64Encode( buffer, bytes );
	m_ThreadInfo->send( XmlNodeIq( _T("result"), pInfo ) << XQUERY( _T(JABBER_FEAT_AVATAR)) << XCHILD( _T("query"), _A2T(str)) << XATTR( _T("mimetype"), szMimeType ));
	mir_free( str );
	mir_free( buffer );
	return TRUE;
}

BOOL CJabberProto::OnSiRequest( HXML node, CJabberIqInfo *pInfo )
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
	return TRUE;
}

BOOL CJabberProto::OnRosterPushRequest( HXML, CJabberIqInfo *pInfo )
{
	HXML queryNode = pInfo->GetChildNode();

	// RFC 3921 #7.2 Business Rules
	if ( pInfo->GetFrom() ) {
		TCHAR* szFrom = JabberPrepareJid( pInfo->GetFrom() );
		if ( !szFrom )
			return TRUE;

		TCHAR* szTo = JabberPrepareJid( m_ThreadInfo->fullJID );
		if ( !szTo ) {
			mir_free( szFrom );
			return TRUE;
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
			return TRUE;
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
					else if (!m_options.IgnoreRosterGroups)
						DBDeleteContactSetting( hContact, "CList", "Group" );
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
			else
				UpdateSubscriptionInfo( hContact, item );
	}	}

	UI_SAFE_NOTIFY(m_pDlgServiceDiscovery, WM_JABBER_TRANSPORT_REFRESH);
	RebuildInfoFrame();
	return TRUE;
}

BOOL CJabberProto::OnIqRequestOOB( HXML, CJabberIqInfo *pInfo )
{
	if ( !pInfo->GetFrom() || !pInfo->GetHContact() )
		return TRUE;

	HXML n = xmlGetChild( pInfo->GetChildNode(), "url" );
	if ( !n || !xmlGetText( n ))
		return TRUE;

	if ( m_options.BsOnlyIBB ) {
		// reject
		XmlNodeIq iq( _T("error"), pInfo );
		HXML e = xmlAddChild( iq, _T("error"), _T("File transfer refused")); xmlAddAttr( e, _T("code"), 406 );
		m_ThreadInfo->send( iq );
		return TRUE;
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
		TCHAR* desc = NULL;

		Log( "Host=%s Port=%d Path=%s", ft->httpHostName, ft->httpPort, ft->httpPath );
		if (( n = xmlGetChild( pInfo->GetChildNode(), "desc" )) != NULL )
			desc = ( TCHAR* )xmlGetText( n );

		TCHAR* str2;
		Log( "description = %s", desc );
		if (( str2 = _tcsrchr( ft->httpPath, '/' )) != NULL )
			str2++;
		else
			str2 = ft->httpPath;
		str2 = mir_tstrdup( str2 );
		JabberHttpUrlDecode( str2 );

		PROTORECVFILET pre;
		pre.flags = PREF_TCHAR;
		pre.timestamp = time( NULL );
		pre.tszDescription = desc;
		pre.ptszFiles = &str2;
		pre.fileCount = 1;
		pre.lParam = ( LPARAM )ft;

		CCSDATA ccs = { ft->std.hContact, PSR_FILE, 0, ( LPARAM )&pre };
		JCallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );
		mir_free( str2 );
	}
	else {
		// reject
		XmlNodeIq iq( _T("error"), pInfo );
		HXML e = xmlAddChild( iq, _T("error"), _T("File transfer refused")); xmlAddAttr( e, _T("code"), 406 );
		m_ThreadInfo->send( iq );
		delete ft;
	}
	return TRUE;
}

BOOL CJabberProto::OnHandleDiscoInfoRequest( HXML iqNode, CJabberIqInfo* pInfo )
{
	if ( !pInfo->GetChildNode() )
		return TRUE;

	const TCHAR* szNode = xmlGetAttrValue( pInfo->GetChildNode(), _T("node"));
	// caps hack
	if ( m_clientCapsManager.HandleInfoRequest( iqNode, pInfo, szNode ))
		return TRUE;

	// ad-hoc hack:
	if ( szNode && m_adhocManager.HandleInfoRequest( iqNode, pInfo, szNode ))
		return TRUE;

	// another request, send empty result
	m_ThreadInfo->send(
		XmlNodeIq( _T("error"), pInfo )
			<< XCHILD( _T("error")) << XATTRI( _T("code"), 404 ) << XATTR( _T("type"), _T("cancel"))
				<< XCHILDNS( _T("item-not-found"), _T("urn:ietf:params:xml:ns:xmpp-stanzas")));
	return TRUE;
}

BOOL CJabberProto::OnHandleDiscoItemsRequest( HXML iqNode, CJabberIqInfo* pInfo )
{
	if ( !pInfo->GetChildNode() )
		return TRUE;

	// ad-hoc commands check:
	const TCHAR* szNode = xmlGetAttrValue( pInfo->GetChildNode(), _T("node"));
	if ( szNode && m_adhocManager.HandleItemsRequest( iqNode, pInfo, szNode ))
		return TRUE;

	// another request, send empty result
	XmlNodeIq iq( _T("result"), pInfo );
	HXML resultQuery = iq << XQUERY( _T(JABBER_FEAT_DISCO_ITEMS));
	if ( szNode )
		xmlAddAttr( resultQuery, _T("node"), szNode );

	if ( !szNode && m_options.EnableRemoteControl )
		resultQuery << XCHILD( _T("item")) << XATTR( _T("jid"), m_ThreadInfo->fullJID ) 
			<< XATTR( _T("node"), _T(JABBER_FEAT_COMMANDS)) << XATTR( _T("name"), _T("Ad-hoc commands"));

	m_ThreadInfo->send( iq );
	return TRUE;
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

BOOL CJabberProto::OnIqHttpAuth( HXML node, CJabberIqInfo* pInfo )
{
	if ( !m_options.AcceptHttpAuth )
		return TRUE;

	if ( !node || !pInfo->GetChildNode() || !pInfo->GetFrom() || !pInfo->GetIdStr() )
		return TRUE;

	HXML pConfirm = xmlGetChild( node , "confirm" );
	if ( !pConfirm )
		return TRUE;

	const TCHAR *szId = xmlGetAttrValue( pConfirm, _T("id"));
	const TCHAR *szMethod = xmlGetAttrValue( pConfirm, _T("method"));
	const TCHAR *szUrl = xmlGetAttrValue( pConfirm, _T("url"));

	if ( !szId || !szMethod || !szUrl )
		return TRUE;

	CJabberHttpAuthParams *pParams = (CJabberHttpAuthParams *)mir_alloc( sizeof( CJabberHttpAuthParams ));
	if ( !pParams )
		return TRUE;
	ZeroMemory( pParams, sizeof( CJabberHttpAuthParams ));
	pParams->m_nType = CJabberHttpAuthParams::IQ;
	pParams->m_szFrom = mir_tstrdup( pInfo->GetFrom() );
	pParams->m_szId = mir_tstrdup( szId );
	pParams->m_szMethod = mir_tstrdup( szMethod );
	pParams->m_szUrl = mir_tstrdup( szUrl );

	AddClistHttpAuthEvent( pParams );

	return TRUE;
}
