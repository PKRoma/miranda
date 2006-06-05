/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2003-5 George Hazan.
Copyright (c) 2002-3 Richard Hughes (original version).

Miranda IM: the free icq client for MS Windows
Copyright (C) 2000-2002 Richard Hughes, Roland Rabien & Tristan Van de Vreede

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "msn_global.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Basic SSL operation class

struct SSL_Base
{
				char* m_szEmail;

	virtual	~SSL_Base() {}

	virtual  int init() = 0;
	virtual  char* getSslResult( char* parUrl, char* parAuthInfo ) = 0;
};

/////////////////////////////////////////////////////////////////////////////////////////
// WinInet class
/////////////////////////////////////////////////////////////////////////////////////////

#define ERROR_FLAGS (FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_GENERATE_DATA | FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS )

#include "wininet.h"

#define SSL_BUF_SIZE 8192

typedef BOOL  ( WINAPI *ft_HttpQueryInfo )( HINTERNET, DWORD, LPVOID, LPDWORD, LPDWORD );
typedef BOOL  ( WINAPI *ft_HttpSendRequest )( HINTERNET, LPCSTR, DWORD, LPVOID, DWORD );
typedef BOOL  ( WINAPI *ft_InternetCloseHandle )( HINTERNET );
typedef DWORD ( WINAPI *ft_InternetErrorDlg )( HWND, HINTERNET, DWORD, DWORD, LPVOID* );
typedef BOOL  ( WINAPI *ft_InternetSetOption )( HINTERNET, DWORD, LPVOID, DWORD );
typedef BOOL  ( WINAPI *ft_InternetReadFile )( HINTERNET, LPVOID, DWORD, LPDWORD );

typedef HINTERNET ( WINAPI *ft_HttpOpenRequest )( HINTERNET, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPCSTR*, DWORD, DWORD );
typedef HINTERNET ( WINAPI *ft_InternetConnect )( HINTERNET, LPCSTR, INTERNET_PORT, LPCSTR, LPCSTR, DWORD, DWORD, DWORD );
typedef HINTERNET ( WINAPI *ft_InternetOpen )( LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD );

struct SSL_WinInet : public SSL_Base
{
	virtual ~SSL_WinInet();

	virtual  char* getSslResult( char* parUrl, char* parAuthInfo );
	virtual  int init();

	void applyProxy( HINTERNET );
	void readInput( HINTERNET );

	//-----------------------------------------------------------------------------------
	HMODULE m_dll;

	ft_InternetCloseHandle f_InternetCloseHandle;
	ft_InternetConnect     f_InternetConnect;
	ft_InternetErrorDlg    f_InternetErrorDlg;
	ft_InternetOpen        f_InternetOpen;
	ft_InternetReadFile    f_InternetReadFile;
	ft_InternetSetOption   f_InternetSetOption;
	ft_HttpOpenRequest     f_HttpOpenRequest;
	ft_HttpQueryInfo       f_HttpQueryInfo;
	ft_HttpSendRequest     f_HttpSendRequest;
};

/////////////////////////////////////////////////////////////////////////////////////////

int SSL_WinInet::init()
{
	if (( m_dll = LoadLibraryA( "WinInet.dll" )) == NULL )
		return 10;

	f_InternetCloseHandle = (ft_InternetCloseHandle)GetProcAddress( m_dll, "InternetCloseHandle" );
	f_InternetConnect = (ft_InternetConnect)GetProcAddress( m_dll, "InternetConnectA" );
	f_InternetErrorDlg = (ft_InternetErrorDlg)GetProcAddress( m_dll, "InternetErrorDlg" );
	f_InternetOpen = (ft_InternetOpen)GetProcAddress( m_dll, "InternetOpenA" );
	f_InternetReadFile = (ft_InternetReadFile)GetProcAddress( m_dll, "InternetReadFile" );
	f_InternetSetOption = (ft_InternetSetOption)GetProcAddress( m_dll, "InternetSetOptionA" );
	f_HttpOpenRequest = (ft_HttpOpenRequest)GetProcAddress( m_dll, "HttpOpenRequestA" );
	f_HttpQueryInfo = (ft_HttpQueryInfo)GetProcAddress( m_dll, "HttpQueryInfoA" );
	f_HttpSendRequest = (ft_HttpSendRequest)GetProcAddress( m_dll, "HttpSendRequestA" );
	return 0;
}

SSL_WinInet::~SSL_WinInet()
{
	#if defined( _UNICODE ) 
		FreeLibrary( m_dll );   // we free WININET.DLL only if we're under NT
	#endif
}

/////////////////////////////////////////////////////////////////////////////////////////

void SSL_WinInet::applyProxy( HINTERNET parHandle )
{
	char tBuffer[ 100 ];

	MSN_DebugLog( "Applying proxy parameters..." );

	if ( !MSN_GetStaticString( "NLProxyAuthUser", NULL, tBuffer, SSL_BUF_SIZE ))
		f_InternetSetOption( parHandle, INTERNET_OPTION_PROXY_USERNAME, tBuffer, strlen( tBuffer ));
	else
		MSN_DebugLog( "Warning: proxy user name is required but missing" );

	if ( !MSN_GetStaticString( "NLProxyAuthPassword", NULL, tBuffer, SSL_BUF_SIZE )) {
		MSN_CallService( MS_DB_CRYPT_DECODESTRING, strlen( tBuffer ), ( LPARAM )tBuffer );
		f_InternetSetOption( parHandle, INTERNET_OPTION_PROXY_PASSWORD, tBuffer, strlen( tBuffer ));
	}
	else MSN_DebugLog( "Warning: proxy user password is required but missing" );
}

/////////////////////////////////////////////////////////////////////////////////////////

void SSL_WinInet::readInput( HINTERNET hRequest )
{
	DWORD dwSize;

	do {
		char tmpbuf[100];
		f_InternetReadFile( hRequest, tmpbuf, 50, &dwSize);
	}
		while (dwSize != 0);
}

char* SSL_WinInet::getSslResult( char* parUrl, char* parAuthInfo )
{
	DWORD tFlags =
		INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS |
		INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP |
		INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
		INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
      INTERNET_FLAG_KEEP_CONNECTION |
		INTERNET_FLAG_NO_AUTO_REDIRECT |
      INTERNET_FLAG_NO_CACHE_WRITE |
		INTERNET_FLAG_NO_COOKIES |
		INTERNET_FLAG_RELOAD |
		INTERNET_FLAG_SECURE;

	const DWORD tInternetFlags =
		INTERNET_FLAG_NO_COOKIES |
		INTERNET_FLAG_NO_UI |
		INTERNET_FLAG_PRAGMA_NOCACHE |
		INTERNET_FLAG_SECURE;

	HINTERNET tNetHandle;
	char* tBuffer = ( char* )alloca( SSL_BUF_SIZE );

	int tUsesProxy = MSN_GetByte( "NLUseProxy", 0 );
	if ( tUsesProxy ) {
		DWORD ptype = MSN_GetByte( "NLProxyType", 0 );
		if ( !MSN_GetByte( "UseIeProxy", 0 ) && ( ptype == PROXYTYPE_HTTP || ptype == PROXYTYPE_HTTPS )) {
			char szProxy[ 100 ];
			if ( MSN_GetStaticString( "NLProxyServer", NULL, szProxy, sizeof szProxy )) {
				MSN_DebugLog( "Proxy server name should be set if proxy is used" );
				return NULL;
			}

			int tPortNumber = MSN_GetWord( NULL, "NLProxyPort", -1 );
			if ( tPortNumber == -1 ) {
				MSN_DebugLog( "Proxy server port should be set if proxy is used" );
				return NULL;
			}

			mir_snprintf( tBuffer, SSL_BUF_SIZE, "%s:%d", szProxy, tPortNumber );

			tNetHandle = f_InternetOpen( "MSMSGS", INTERNET_OPEN_TYPE_PROXY, tBuffer, NULL, tInternetFlags );
		}
		else tNetHandle = f_InternetOpen( "MSMSGS", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, tInternetFlags );
	}
	else tNetHandle = f_InternetOpen( "MSMSGS", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, tInternetFlags );

	if ( tNetHandle == NULL ) {
		MSN_DebugLog( "InternetOpen() failed" );
		return NULL;
	}

	MSN_DebugLog( "SSL request (%s): '%s'", ( tUsesProxy ) ? "using proxy": "direct connection", parUrl );

	char* urlStart = strstr( parUrl, "://" );
	if ( urlStart == NULL )
		urlStart = parUrl;
	else
		urlStart += 3;

	{	int tLen = strlen( urlStart )+1;
		parUrl = ( char* )alloca( tLen );
		memcpy( parUrl, urlStart, tLen );
	}

	char* tObjectName = ( char* )strchr( parUrl, '/' );
	if ( tObjectName != NULL ) {
		int tLen = strlen( tObjectName )+1;
		char* newBuf = ( char* )alloca( tLen );
		memcpy( newBuf, tObjectName, tLen );

		*tObjectName = 0;
		tObjectName = newBuf;
	}
	else tObjectName = "/";

	char* tSslAnswer = NULL;

	HINTERNET tUrlHandle = f_InternetConnect( tNetHandle, parUrl, INTERNET_DEFAULT_HTTPS_PORT, "", "", INTERNET_SERVICE_HTTP, INTERNET_FLAG_NO_AUTO_REDIRECT + INTERNET_FLAG_NO_COOKIES, 0 );
	if ( tUrlHandle != NULL ) {
		HINTERNET tRequest = f_HttpOpenRequest( tUrlHandle, "GET", tObjectName, NULL, "", NULL, tFlags, NULL );
		if ( tRequest != NULL ) {
			DWORD tBufSize;
			bool  bProxyParamsSubstituted = false;

			if ( tUsesProxy )
				applyProxy( tRequest );

LBL_Restart:
			MSN_DebugLog( "Sending request..." );
			DWORD tErrorCode = f_HttpSendRequest( tRequest, ( parAuthInfo != NULL ) ? parAuthInfo : "", DWORD(-1), NULL, 0 );
			if ( tErrorCode == 0 ) {
				TWinErrorCode errCode;
				MSN_DebugLog( "HttpSendRequest() failed with error %ld", errCode.mErrorCode );

				if ( errCode.mErrorCode == 2 )
					MSN_ShowError( "Internet Explorer is in the 'Offline' mode. Switch IE to the 'Online' mode and then try to relogin" );
				else
					MSN_ShowError( "MSN Passport verification failed with error %d: %s",
						errCode.mErrorCode, errCode.getText());
			}
			else {
				DWORD dwCode;
				tBufSize = sizeof( dwCode );
				f_HttpQueryInfo( tRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwCode, &tBufSize, 0 );

				switch( dwCode ) {
				case HTTP_STATUS_REDIRECT:
					tBufSize = SSL_BUF_SIZE;
					f_HttpQueryInfo( tRequest, HTTP_QUERY_LOCATION, tBuffer, &tBufSize, NULL );
					MSN_DebugLog( "Redirected to '%s'", tBuffer );
					tSslAnswer = getSslResult( tBuffer, parAuthInfo );
					break;

				case HTTP_STATUS_DENIED:
				case HTTP_STATUS_PROXY_AUTH_REQ:
					if ( tUsesProxy && !bProxyParamsSubstituted ) {
						bProxyParamsSubstituted = true;
						applyProxy( tUrlHandle );
						readInput( tRequest );
						goto LBL_Restart;
					}

					// else fall down to display the error dialog

				case ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR:
				case ERROR_INTERNET_INCORRECT_PASSWORD:
				case ERROR_INTERNET_INVALID_CA:
				case ERROR_INTERNET_POST_IS_NON_SECURE:
				case ERROR_INTERNET_SEC_CERT_CN_INVALID:
				case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
				case ERROR_INTERNET_SEC_CERT_ERRORS:
				case ERROR_INTERNET_SEC_CERT_NO_REV:
				case ERROR_INTERNET_SEC_CERT_REV_FAILED:
					MSN_DebugLog( "HttpSendRequest returned error code %d", tErrorCode );
					if ( ERROR_INTERNET_FORCE_RETRY == f_InternetErrorDlg( GetDesktopWindow(), tRequest, tErrorCode, ERROR_FLAGS, NULL )) {
						readInput( tRequest );
						goto LBL_Restart;
					}

					// else fall into the general error handling routine

				case HTTP_STATUS_OK:
				LBL_PrintHeaders:
					tBufSize = SSL_BUF_SIZE;
					f_HttpQueryInfo( tRequest, HTTP_QUERY_RAW_HEADERS_CRLF, tBuffer, &tBufSize, NULL );
					MSN_DebugLog( "SSL answer: '%s'", tBuffer );
					tSslAnswer = strdup( tBuffer );
					break;

				default:
					tBufSize = SSL_BUF_SIZE;
					if ( !f_HttpQueryInfo( tRequest, HTTP_QUERY_STATUS_TEXT, tBuffer, &tBufSize, NULL ))
						strcpy( tBuffer, "unknown error" );

					MSN_ShowError( "Internet secure connection (SSL) failed with error %d: %s", dwCode, tBuffer );
					MSN_DebugLog( "SSL error %d: '%s'", dwCode, tBuffer );
					goto LBL_PrintHeaders;
			}	}

			f_InternetCloseHandle( tRequest );
		}

		f_InternetCloseHandle( tUrlHandle );
	}
	else MSN_DebugLog( "InternetOpenUrl() failed" );

	f_InternetCloseHandle( tNetHandle );
	return tSslAnswer;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Performs the MSN Passport login via SSL3 using the OpenSSL library

typedef int ( *PFN_SSL_int_void ) ( void );
typedef PVOID ( *PFN_SSL_pvoid_void ) ( void );
typedef PVOID ( *PFN_SSL_pvoid_pvoid ) ( PVOID );
typedef void ( *PFN_SSL_void_pvoid ) ( PVOID );
typedef int ( *PFN_SSL_int_pvoid_int ) ( PVOID, int );
typedef int ( *PFN_SSL_int_pvoid ) ( PVOID );
typedef int ( *PFN_SSL_int_pvoid_pvoid_int ) ( PVOID, PVOID, int );

struct SSL_OpenSsl : public SSL_Base
{
	virtual  char* getSslResult( char* parUrl, char* parAuthInfo );
	virtual  int init();

	static	PVOID sslCtx;

	static	HMODULE hLibSSL, hLibEAY;
	static	PFN_SSL_int_void            pfn_SSL_library_init;
	static	PFN_SSL_pvoid_void          pfn_SSLv23_client_method;
	static	PFN_SSL_pvoid_pvoid         pfn_SSL_CTX_new;
	static	PFN_SSL_void_pvoid          pfn_SSL_CTX_free;
	static	PFN_SSL_pvoid_pvoid         pfn_SSL_new;
	static	PFN_SSL_void_pvoid          pfn_SSL_free;
	static	PFN_SSL_int_pvoid_int       pfn_SSL_set_fd;
	static	PFN_SSL_int_pvoid           pfn_SSL_connect;
	static	PFN_SSL_int_pvoid_pvoid_int pfn_SSL_read;
	static	PFN_SSL_int_pvoid_pvoid_int pfn_SSL_write;
};

/////////////////////////////////////////////////////////////////////////////////////////

PVOID		SSL_OpenSsl::sslCtx = NULL;

HMODULE	SSL_OpenSsl::hLibSSL = NULL,
			SSL_OpenSsl::hLibEAY = NULL;

PFN_SSL_int_void            SSL_OpenSsl::pfn_SSL_library_init;
PFN_SSL_pvoid_void          SSL_OpenSsl::pfn_SSLv23_client_method;
PFN_SSL_pvoid_pvoid         SSL_OpenSsl::pfn_SSL_CTX_new;
PFN_SSL_void_pvoid          SSL_OpenSsl::pfn_SSL_CTX_free;
PFN_SSL_pvoid_pvoid         SSL_OpenSsl::pfn_SSL_new;
PFN_SSL_void_pvoid          SSL_OpenSsl::pfn_SSL_free;
PFN_SSL_int_pvoid_int       SSL_OpenSsl::pfn_SSL_set_fd;
PFN_SSL_int_pvoid           SSL_OpenSsl::pfn_SSL_connect;
PFN_SSL_int_pvoid_pvoid_int SSL_OpenSsl::pfn_SSL_read;
PFN_SSL_int_pvoid_pvoid_int SSL_OpenSsl::pfn_SSL_write;

int SSL_OpenSsl::init()
{
	if ( sslCtx != NULL )
		return 0;

	if ( hLibSSL == NULL ) {
		if (( hLibEAY = LoadLibraryA( "LIBEAY32.DLL" )) == NULL ) {
			MSN_ShowError( "Valid %s must be installed to perform the SSL login", "LIBEAY32.DLL" );
			return 1;
		}

		if (( hLibSSL = LoadLibraryA( "LIBSSL32.DLL" )) == NULL ) {
			MSN_ShowError( "Valid %s must be installed to perform the SSL login", "LIBSSL32.DLL" );
			return 1;
	}	}

	int retVal = 0;
	if (( pfn_SSL_library_init = ( PFN_SSL_int_void )GetProcAddress( hLibSSL, "SSL_library_init" )) == NULL )
		retVal = TRUE;
	if (( pfn_SSLv23_client_method = ( PFN_SSL_pvoid_void )GetProcAddress( hLibSSL, "SSLv23_client_method" )) == NULL )
		retVal = TRUE;
	if (( pfn_SSL_CTX_new = ( PFN_SSL_pvoid_pvoid )GetProcAddress( hLibSSL, "SSL_CTX_new" )) == NULL )
		retVal = TRUE;
	if (( pfn_SSL_CTX_free = ( PFN_SSL_void_pvoid )GetProcAddress( hLibSSL, "SSL_CTX_free" )) == NULL )
		retVal = TRUE;
	if (( pfn_SSL_new = ( PFN_SSL_pvoid_pvoid )GetProcAddress( hLibSSL, "SSL_new" )) == NULL )
		retVal = TRUE;
	if (( pfn_SSL_free = ( PFN_SSL_void_pvoid )GetProcAddress( hLibSSL, "SSL_free" )) == NULL )
		retVal = TRUE;
	if (( pfn_SSL_set_fd = ( PFN_SSL_int_pvoid_int )GetProcAddress( hLibSSL, "SSL_set_fd" )) == NULL )
		retVal = TRUE;
	if (( pfn_SSL_connect = ( PFN_SSL_int_pvoid )GetProcAddress( hLibSSL, "SSL_connect" )) == NULL )
		retVal = TRUE;
	if (( pfn_SSL_read = ( PFN_SSL_int_pvoid_pvoid_int )GetProcAddress( hLibSSL, "SSL_read" )) == NULL )
		retVal = TRUE;
	if (( pfn_SSL_write = ( PFN_SSL_int_pvoid_pvoid_int )GetProcAddress( hLibSSL, "SSL_write" )) == NULL )
		retVal = TRUE;

	if ( retVal ) {
		FreeLibrary( hLibSSL );
		MSN_ShowError( "Valid %s must be installed to perform the SSL login", "LIBSSL32.DLL" );
		return 1;
	}

	pfn_SSL_library_init();
	sslCtx = pfn_SSL_CTX_new( pfn_SSLv23_client_method());
	MSN_DebugLog( "OpenSSL context successully allocated" );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

char* SSL_OpenSsl::getSslResult( char* parUrl, char* parAuthInfo )
{
	if ( strnicmp( parUrl, "https://", 8 ) != 0 )
		return NULL;

	char* url = NEWSTR_ALLOCA( parUrl );
	char* path = strchr( url+9, '//' );
	if ( path == NULL ) {
		MSN_DebugLog( "Invalid URL passed: '%s'", parUrl );
		return NULL;
	}
	*path++ = 0;

	NETLIBOPENCONNECTION tConn = { 0 };
	tConn.cbSize = sizeof( tConn );
	tConn.szHost = url+8;
	tConn.wPort = 443;
	HANDLE h = ( HANDLE )MSN_CallService( MS_NETLIB_OPENCONNECTION, ( WPARAM )hNetlibUser, ( LPARAM )&tConn );
	if ( h == NULL )
		return NULL;

	char* result = NULL;
	PVOID ssl = pfn_SSL_new( sslCtx );
	if ( ssl != NULL ) {
		SOCKET s = MSN_CallService( MS_NETLIB_GETSOCKET, ( WPARAM )h, 0 );
		if ( s != INVALID_SOCKET ) {
			pfn_SSL_set_fd( ssl, s );
			if ( pfn_SSL_connect( ssl ) > 0 ) {
				MSN_DebugLog( "SSL connection succeeded" );

				char *buf = ( char* )alloca( SSL_BUF_SIZE );

				int nBytes = mir_snprintf( buf, SSL_BUF_SIZE,
					"GET /%s HTTP/1.1\r\n"
					"Accept: */*\r\n"
					"Accept-Language: en;q=0.5\r\n"
					"User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322)\r\n"
					"Host: %s\r\n"
					"Connection: Keep-Alive\r\n", path, url+8 );

				if ( parAuthInfo != NULL ) {
					strcat( buf+nBytes, parAuthInfo );
					strcat( buf+nBytes, "\r\n" );
				}

				strcat( buf+nBytes, "\r\n" );

//				MSN_DebugLog( "Sending SSL query:\n%s", buf );
				pfn_SSL_write( ssl, buf, strlen( buf ));

				nBytes = pfn_SSL_read( ssl, buf, SSL_BUF_SIZE );
				if ( nBytes > 0 ) {
					result = ( char* )malloc( nBytes+1 );
					memcpy( result, buf, nBytes+1 );
					result[ nBytes ] = 0;

					MSN_DebugLog( "SSL read successfully read %d bytes:\n%s", nBytes, result );
				}
				else MSN_DebugLog( "SSL read failed" );
			}
			else MSN_DebugLog( "SSL connection failed" );
		}
		else MSN_DebugLog( "pfn_SSL_connect failed" );

		pfn_SSL_free( ssl );
	}
	else MSN_DebugLog( "pfn_SSL_new failed" );

	MSN_CallService( MS_NETLIB_CLOSEHANDLE, ( WPARAM )h, 0 );
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Checks the MSN Passport redirector site

int MSN_CheckRedirector()
{
	int retVal = 0;
	SSL_Base* pAgent;
	if ( MSN_GetByte( "UseOpenSSL", 0 ))
		pAgent = new SSL_OpenSsl();
	else
		pAgent = new SSL_WinInet();
	if ( pAgent == NULL )
		return 1;

	if ( pAgent->init() ) {
		delete pAgent;
		return 2;
	}

	char* msnLoginHost = pAgent->getSslResult( "https://nexus.passport.com/rdr/pprdr.asp", NULL );
	if ( msnLoginHost == NULL ) {
		retVal = 1;
LBL_Exit:
		delete pAgent;
		MSN_DebugLog( "MSN_CheckRedirector exited with errorCode = %d", retVal );
		return retVal;
	}

	char* p = strstr( msnLoginHost, "DALogin=" );
	if ( p == NULL ) {
		free( msnLoginHost ); msnLoginHost = NULL;
		retVal = 2;
		goto LBL_Exit;
	}

	strcpy( msnLoginHost, "https://" );
	strdel( msnLoginHost+8, int( p-msnLoginHost ));
	if (( p = strchr( msnLoginHost, ',' )) != 0 )
		*p = 0;

	MSN_SetString( NULL, "MsnPassportHost", msnLoginHost );
	MSN_DebugLog( "MSN Passport login host is set to '%s'", msnLoginHost );
	free( msnLoginHost );
	goto LBL_Exit;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Performs the MSN Passport login via SSL3

int MSN_GetPassportAuth( char* authChallengeInfo, char*& parResult )
{
	int retVal = 0;
	parResult = NULL;
	SSL_Base* pAgent;
	if ( MSN_GetByte( "UseOpenSSL", 0 ))
		pAgent = new SSL_OpenSsl();
	else
		pAgent = new SSL_WinInet();
	if ( pAgent == NULL )
		return 1;

	if ( pAgent->init() ) {
		delete pAgent;
		return 2;
	}

	char szEmail[ MSN_MAX_EMAIL_LEN ];
	MSN_GetStaticString( "e-mail", NULL, szEmail, MSN_MAX_EMAIL_LEN );
	char* p = strchr( szEmail, '@' );
	if ( p != NULL ) {
		memmove( p+3, p+1, strlen( p ));
		memcpy( p, "%40", 3 );
	}

	char szPassword[ 100 ];
	MSN_GetStaticString( "Password", NULL, szPassword, sizeof szPassword );
	MSN_CallService( MS_DB_CRYPT_DECODESTRING, strlen( szPassword )+1, ( LPARAM )szPassword );
	szPassword[ 16 ] = 0;

	char* szAuthInfo = ( char* )alloca( 1024 );
	int nBytes = mir_snprintf( szAuthInfo, 1024,
		"Authorization: Passport1.4 OrgVerb=GET,OrgURL=http%%3A%%2F%%2Fmessenger%%2Emsn%%2Ecom,sign-in=%s,pwd=",
		szEmail );
	UrlEncode( szPassword, szAuthInfo+nBytes, 1024-nBytes );
	strcat( szAuthInfo+nBytes, "," );
	strcat( szAuthInfo+nBytes, authChallengeInfo );

	char szPassportHost[ 256 ];
	if ( MSN_GetStaticString( "MsnPassportHost", NULL, szPassportHost, sizeof( szPassportHost ))) {
		retVal = 3;
LBL_Exit:
		delete pAgent;
		MSN_DebugLog( "MSN_CheckRedirector exited with errorCode = %d", retVal );
		return retVal;
	}

	char* tResult = pAgent->getSslResult( szPassportHost, szAuthInfo );
	if ( tResult == NULL ) {
		retVal = 4;
		goto LBL_Exit;
	}

	if (( p = strstr( tResult, "from-PP=" )) == NULL )	{
		free( tResult );
		retVal = 5;
		goto LBL_Exit;
	}

	strdel( tResult, int( p-tResult )+9 );

	if (( p = strchr( tResult, '\'' )) != NULL )
		*p = 0;

	parResult = tResult;
	goto LBL_Exit;
}

void UninitSsl( void )
{
	if ( SSL_OpenSsl::hLibEAY )
		FreeLibrary( SSL_OpenSsl::hLibEAY );

	if ( SSL_OpenSsl::hLibSSL ) {
		SSL_OpenSsl::pfn_SSL_CTX_free( SSL_OpenSsl::sslCtx );

		MSN_DebugLog( "Free SSL library" );
		FreeLibrary( SSL_OpenSsl::hLibSSL );
}	}
