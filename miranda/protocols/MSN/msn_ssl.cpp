/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2008 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "msn_global.h"
#include "msn_proto.h"

/////////////////////////////////////////////////////////////////////////////////////////
// WinInet class
/////////////////////////////////////////////////////////////////////////////////////////

#define ERROR_FLAGS (FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_GENERATE_DATA | FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS )

#include "wininet.h"

typedef BOOL  ( WINAPI *ft_HttpQueryInfo )( HINTERNET, DWORD, LPVOID, LPDWORD, LPDWORD );
typedef BOOL  ( WINAPI *ft_HttpSendRequest )( HINTERNET, LPCSTR, DWORD, LPVOID, DWORD );
typedef BOOL  ( WINAPI *ft_InternetCloseHandle )( HINTERNET );
typedef DWORD ( WINAPI *ft_InternetErrorDlg )( HWND, HINTERNET, DWORD, DWORD, LPVOID* );
typedef BOOL  ( WINAPI *ft_InternetSetOption )( HINTERNET, DWORD, LPVOID, DWORD );
typedef BOOL  ( WINAPI *ft_InternetReadFile )( HINTERNET, LPVOID, DWORD, LPDWORD );
typedef BOOL  ( WINAPI *ft_InternetCrackUrl )( LPCSTR, DWORD, DWORD, LPURL_COMPONENTSA );

typedef HINTERNET ( WINAPI *ft_HttpOpenRequest )( HINTERNET, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPCSTR*, DWORD, DWORD );
typedef HINTERNET ( WINAPI *ft_InternetConnect )( HINTERNET, LPCSTR, INTERNET_PORT, LPCSTR, LPCSTR, DWORD, DWORD, DWORD );
typedef HINTERNET ( WINAPI *ft_InternetOpen )( LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD );

class SSL_WinInet : public SSL_Base
{
public:
	SSL_WinInet(CMsnProto* prt) : SSL_Base(prt) {}
	virtual ~SSL_WinInet();

	virtual  char* getSslResult( const char* parUrl, const char* parAuthInfo, const char* hdrs );
	virtual  int init(void);

private:
	void applyProxy( HINTERNET );
	char* readData( HINTERNET );

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
	ft_InternetCrackUrl    f_InternetCrackUrl;
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
	f_InternetCrackUrl = (ft_InternetCrackUrl)GetProcAddress( m_dll, "InternetCrackUrlA" );
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

	proto->MSN_DebugLog( "Applying proxy parameters..." );

	if ( !proto->getStaticString( NULL, "NLProxyAuthUser", tBuffer, sizeof( tBuffer )))
		f_InternetSetOption( parHandle, INTERNET_OPTION_PROXY_USERNAME, tBuffer, strlen( tBuffer )+1);
	else
		proto->MSN_DebugLog( "Warning: proxy user name is required but missing" );

	if ( !proto->getStaticString( NULL, "NLProxyAuthPassword", tBuffer, sizeof( tBuffer ))) {
		MSN_CallService( MS_DB_CRYPT_DECODESTRING, strlen( tBuffer ), ( LPARAM )tBuffer );
		f_InternetSetOption( parHandle, INTERNET_OPTION_PROXY_PASSWORD, tBuffer, strlen( tBuffer )+1);
	}
	else proto->MSN_DebugLog( "Warning: proxy user password is required but missing" );
}

/////////////////////////////////////////////////////////////////////////////////////////

char* SSL_WinInet::readData( HINTERNET hRequest )
{
	char bufQuery[32] ;
	DWORD tBufSize = sizeof( bufQuery );
	f_HttpQueryInfo( hRequest, HTTP_QUERY_CONTENT_LENGTH, bufQuery, &tBufSize, NULL );

	tBufSize = 0; 
	f_HttpQueryInfo( hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, NULL, &tBufSize, NULL );

	DWORD dwSize = tBufSize + atol( bufQuery );
	char* tSslAnswer = (char*)mir_alloc( dwSize + 1 );

	if ( tSslAnswer )
	{
		f_HttpQueryInfo( hRequest, HTTP_QUERY_RAW_HEADERS_CRLF, tSslAnswer, &tBufSize, NULL );

		DWORD dwOffset = tBufSize;
		do {
			if (!f_InternetReadFile( hRequest, tSslAnswer+dwOffset, dwSize - dwOffset, &tBufSize))
			{
				mir_free( tSslAnswer );
				return NULL;
			}
			dwOffset += tBufSize;
		}
		while (tBufSize != 0 && dwOffset < dwSize);
		tSslAnswer[dwOffset] = 0;

		proto->MSN_DebugLog( "SSL response:" );
		MSN_CallService( MS_NETLIB_LOG, ( WPARAM )proto->hNetlibUser, ( LPARAM )tSslAnswer );
	}

	return tSslAnswer;
}


char* SSL_WinInet::getSslResult( const char* parUrl, const char* parAuthInfo, const char* hdrs )
{
	const DWORD tFlags =
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

	HINTERNET tNetHandle;

	if ( proto->MyOptions.UseProxy ) {
		DWORD ptype = proto->getByte( "NLProxyType", 0 );
		if ( !proto->getByte( "UseIeProxy", 0 ) && ( ptype == PROXYTYPE_HTTP || ptype == PROXYTYPE_HTTPS )) {
			char szProxy[ 100 ];
			if ( proto->getStaticString( NULL, "NLProxyServer", szProxy, sizeof( szProxy ))) {
				proto->MSN_DebugLog( "Proxy server name should be set if proxy is used" );
				return NULL;
			}

			int tPortNumber = proto->getWord( NULL, "NLProxyPort", -1 );
			if ( tPortNumber == -1 ) {
				proto->MSN_DebugLog( "Proxy server port should be set if proxy is used" );
				return NULL;
			}

			char proxystr[1024];
			mir_snprintf( proxystr, sizeof( proxystr ), "https=http://%s:%d http=http://%s:%d", 
				szProxy, tPortNumber, szProxy, tPortNumber );

			tNetHandle = f_InternetOpen( MSN_USER_AGENT, INTERNET_OPEN_TYPE_PROXY, proxystr, NULL, 0 );
		}
		else tNetHandle = f_InternetOpen( MSN_USER_AGENT, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	}
	else 
		tNetHandle = f_InternetOpen( MSN_USER_AGENT, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0 );

	if ( tNetHandle == NULL ) {
		proto->MSN_DebugLog( "InternetOpen() failed" );
		return NULL;
	}

	proto->MSN_DebugLog( "SSL request (%s): '%s'", proto->MyOptions.UseProxy ? "using proxy": "direct connection", parUrl );

	URL_COMPONENTSA urlComp = {0};
	urlComp.dwStructSize = sizeof( urlComp );
	urlComp.dwUrlPathLength = 1;
	urlComp.dwHostNameLength = 1;

	f_InternetCrackUrl( parUrl, 0, 0, &urlComp);

	char* url = ( char* )alloca( urlComp.dwHostNameLength + 1 );
	memcpy( url, urlComp.lpszHostName, urlComp.dwHostNameLength );
	url[urlComp.dwHostNameLength] = 0;

	char* tObjectName = ( char* )alloca( urlComp.dwUrlPathLength + 1 );
	memcpy( tObjectName, urlComp.lpszUrlPath, urlComp.dwUrlPathLength );
	tObjectName[urlComp.dwUrlPathLength] = 0;

	char* tSslAnswer = NULL;

	HINTERNET tUrlHandle = f_InternetConnect( tNetHandle, url, INTERNET_DEFAULT_HTTPS_PORT, "", "", INTERNET_SERVICE_HTTP, 0, 0 );
	if ( tUrlHandle != NULL ) 
	{
		HINTERNET tRequest = f_HttpOpenRequest( tUrlHandle, "POST", tObjectName, NULL, NULL, NULL, tFlags, 0 );
		if ( tRequest != NULL ) {

			unsigned tm = 6000;
			f_InternetSetOption( tRequest, INTERNET_OPTION_CONNECT_TIMEOUT, &tm, sizeof(tm));
			f_InternetSetOption( tRequest, INTERNET_OPTION_SEND_TIMEOUT, &tm, sizeof(tm));
			f_InternetSetOption( tRequest, INTERNET_OPTION_RECEIVE_TIMEOUT, &tm, sizeof(tm));

		if ( proto->MyOptions.UseProxy && proto->getByte( "NLUseProxyAuth", 0  ))
			applyProxy( tRequest );

		char headers[2048];
		mir_snprintf(headers, sizeof( headers ), 
			"Accept: text/*\r\nContent-Type: text/xml; charset=utf-8\r\n%s", 
			hdrs ? hdrs : "");

		bool restart = false;

LBL_Restart:
			proto->MSN_DebugLog( "Sending request..." );
#ifndef _DEBUG
			if (strstr(parUrl, "login") == NULL)
#endif
				MSN_CallService( MS_NETLIB_LOG, ( WPARAM )proto->hNetlibUser, ( LPARAM )parAuthInfo );

			DWORD tErrorCode = f_HttpSendRequest( tRequest, headers, strlen( headers ), 
				(void*)parAuthInfo, strlen( parAuthInfo ));
			if ( tErrorCode == 0 ) {
				TWinErrorCode errCode;
				proto->MSN_DebugLog( "HttpSendRequest() failed with error %d: %s", errCode.mErrorCode, errCode.getText());

				switch( errCode.mErrorCode ) {
					case 2:
						proto->MSN_ShowError( "Internet Explorer is in the 'Offline' mode. Switch IE to the 'Online' mode and then try to relogin" );
						break;

					case ERROR_INTERNET_INVALID_CA:
					case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
					case ERROR_INTERNET_SEC_CERT_NO_REV:
					case ERROR_INTERNET_SEC_CERT_REV_FAILED:
						if (!restart)
						{
							DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA  |
								  			SECURITY_FLAG_IGNORE_REVOCATION  |   
											SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

							f_InternetSetOption( tRequest, INTERNET_OPTION_SECURITY_FLAGS, 
								&dwFlags, sizeof( dwFlags ));
							mir_free( readData( tRequest ));
							restart = true;
							goto LBL_Restart;
						}

					default:
						proto->MSN_ShowError( "MSN Passport verification failed with error %d: %s",
							errCode.mErrorCode, errCode.getText());
				}
			}
			else {
				DWORD dwCode;
				DWORD tBufSize = sizeof( dwCode );
				f_HttpQueryInfo( tRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwCode, &tBufSize, 0 );

				tSslAnswer = readData( tRequest );
			}

			f_InternetCloseHandle( tRequest );
		}

		f_InternetCloseHandle( tUrlHandle );
	}
	else proto->MSN_DebugLog( "InternetOpenUrl() failed" );

	f_InternetCloseHandle( tNetHandle );
	return tSslAnswer;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Performs the MSN Passport login via SSL3 using the OpenSSL library

class SSL_OpenSsl : public SSL_Base
{
public:
	SSL_OpenSsl(CMsnProto* prt) : SSL_Base(prt) {}

	virtual  char* getSslResult( const char* parUrl, const char* parAuthInfo, const char* hdrs );
	virtual  int init(void);
};

typedef int ( *PFN_SSL_int_void ) ( void );
typedef PVOID ( *PFN_SSL_pvoid_void ) ( void );
typedef PVOID ( *PFN_SSL_pvoid_pvoid ) ( PVOID );
typedef void ( *PFN_SSL_void_pvoid ) ( PVOID );
typedef int ( *PFN_SSL_int_pvoid_int ) ( PVOID, int );
typedef int ( *PFN_SSL_int_pvoid ) ( PVOID );
typedef int ( *PFN_SSL_int_pvoid_pvoid_int ) ( PVOID, PVOID, int );
typedef int ( *PFN_SSL_int_pvoid_int_pvoid ) ( PVOID, int, PVOID );

static	HMODULE hLibSSL;
static	PVOID sslCtx;

static	PFN_SSL_int_void            pfn_SSL_library_init;
static	PFN_SSL_pvoid_void          pfn_TLSv1_client_method;
static	PFN_SSL_pvoid_pvoid         pfn_SSL_CTX_new;
static	PFN_SSL_void_pvoid          pfn_SSL_CTX_free;
static	PFN_SSL_pvoid_pvoid         pfn_SSL_new;
static	PFN_SSL_void_pvoid          pfn_SSL_free;
static	PFN_SSL_int_pvoid_int       pfn_SSL_set_fd;
static	PFN_SSL_int_pvoid           pfn_SSL_connect;
static	PFN_SSL_int_pvoid_pvoid_int pfn_SSL_read;
static	PFN_SSL_int_pvoid_pvoid_int pfn_SSL_write;
static  PFN_SSL_int_pvoid_int_pvoid pfn_SSL_CTX_set_verify;

/////////////////////////////////////////////////////////////////////////////////////////

int SSL_OpenSsl::init(void)
{
	if ( sslCtx != NULL )
		return 0;

	if ( hLibSSL == NULL ) 
	{
		hLibSSL = LoadLibraryA( "WINSSL.DLL" );
		if ( hLibSSL == NULL )
			hLibSSL = LoadLibraryA( "CYASSL.DLL" );
		if ( hLibSSL == NULL )
			hLibSSL = LoadLibraryA( "SSLEAY32.DLL" );
		if ( hLibSSL == NULL )
			hLibSSL = LoadLibraryA( "LIBSSL32.DLL" );
		if ( hLibSSL == NULL ) {
			proto->MSN_ShowError( "Valid %s must be installed to perform the SSL login", "SSLEAY32.DLL" );
			return 1;
		}

		int retVal = 0;
		if (( pfn_SSL_library_init = ( PFN_SSL_int_void )GetProcAddress( hLibSSL, "SSL_library_init" )) == NULL )
			retVal = TRUE;
		if (( pfn_TLSv1_client_method = ( PFN_SSL_pvoid_void )GetProcAddress( hLibSSL, "TLSv1_client_method" )) == NULL )
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
		if (( pfn_SSL_CTX_set_verify = ( PFN_SSL_int_pvoid_int_pvoid )GetProcAddress( hLibSSL, "SSL_CTX_set_verify" )) == NULL )
			retVal = TRUE;

		if ( retVal ) {
			FreeLibrary( hLibSSL );
			proto->MSN_ShowError( "Valid %s must be installed to perform the SSL login", "SSLEAY32.DLL" );
			return 1;
		}

		pfn_SSL_library_init();
		sslCtx = pfn_SSL_CTX_new( pfn_TLSv1_client_method());
		pfn_SSL_CTX_set_verify(sslCtx, 0, NULL);
		proto->MSN_DebugLog( "OpenSSL context successully allocated" );
	}
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////

char* SSL_OpenSsl::getSslResult( const char* parUrl, const char* parAuthInfo, const char* hdrs )
{
	if ( _strnicmp( parUrl, "https://", 8 ) != 0 )
		return NULL;

	char* url = NEWSTR_ALLOCA(parUrl);
	char* path  = strchr(url+9, '/');
	char* path1 = strchr(url+9, ':');
	if (path == NULL) 
	{
		proto->MSN_DebugLog( "Invalid URL passed: '%s'", parUrl );
		return NULL;
	}
	if (path < path1 || path1 == NULL)
		*path = 0;
	else
		*path1 = 0;

	++path;

	NETLIBUSERSETTINGS nls = { 0 };
	nls.cbSize = sizeof( nls );
	MSN_CallService(MS_NETLIB_GETUSERSETTINGS,WPARAM(proto->hNetlibUser),LPARAM(&nls));
	int cpType = nls.proxyType;

	if (nls.useProxy && cpType == PROXYTYPE_HTTP)
	{
		nls.proxyType = PROXYTYPE_HTTPS;
		nls.szProxyServer = NEWSTR_ALLOCA(nls.szProxyServer);
		nls.szIncomingPorts = NEWSTR_ALLOCA(nls.szIncomingPorts);
		nls.szOutgoingPorts = NEWSTR_ALLOCA(nls.szOutgoingPorts);
		nls.szProxyAuthPassword = NEWSTR_ALLOCA(nls.szProxyAuthPassword);
		nls.szProxyAuthUser = NEWSTR_ALLOCA(nls.szProxyAuthUser);
		MSN_CallService(MS_NETLIB_SETUSERSETTINGS,WPARAM(proto->hNetlibUser),LPARAM(&nls));
	}

	NETLIBOPENCONNECTION tConn = { 0 };
	tConn.cbSize = sizeof( tConn );
	tConn.szHost = url+8;
	tConn.wPort = 443;
	tConn.timeout = 8;
	HANDLE h = ( HANDLE )MSN_CallService( MS_NETLIB_OPENCONNECTION, ( WPARAM )proto->hNetlibUser, ( LPARAM )&tConn );
	
	if (nls.useProxy && cpType == PROXYTYPE_HTTP)
	{
		nls.proxyType = PROXYTYPE_HTTP;
		nls.szProxyServer = NEWSTR_ALLOCA(nls.szProxyServer);
		nls.szIncomingPorts = NEWSTR_ALLOCA(nls.szIncomingPorts);
		nls.szOutgoingPorts = NEWSTR_ALLOCA(nls.szOutgoingPorts);
		nls.szProxyAuthPassword = NEWSTR_ALLOCA(nls.szProxyAuthPassword);
		nls.szProxyAuthUser = NEWSTR_ALLOCA(nls.szProxyAuthUser);
		MSN_CallService(MS_NETLIB_SETUSERSETTINGS,WPARAM(proto->hNetlibUser),LPARAM(&nls));
	}
		
	if ( h == NULL )
		return NULL;

	char* result = NULL;
	PVOID ssl = pfn_SSL_new( sslCtx );
	if ( ssl != NULL ) {
		SOCKET s = MSN_CallService( MS_NETLIB_GETSOCKET, ( WPARAM )h, 0 );
		if ( s != INVALID_SOCKET ) {
			pfn_SSL_set_fd( ssl, s );
			if ( pfn_SSL_connect( ssl ) > 0 ) {
				proto->MSN_DebugLog( "SSL connection succeeded" );

				const char* chdrs = hdrs ? hdrs : "";
				size_t hlen = strlen(chdrs) + 1024;
				char *headers = (char*)alloca(hlen);
				
				unsigned nBytes = mir_snprintf( headers, hlen,
					"POST /%s HTTP/1.1\r\n"
					"Accept: text/*\r\n"
					"%s"
					"User-Agent: %s\r\n"
					"Content-Length: %u\r\n"
					"Content-Type: text/xml; charset=utf-8\r\n"
					"Host: %s\r\n"
					"Connection: close\r\n"
					"Cache-Control: no-cache\r\n\r\n", path, chdrs,
					MSN_USER_AGENT, strlen( parAuthInfo ), url+8 );

					proto->MSN_DebugLog( "Sending SSL query:\n%s", headers );
#ifndef _DEBUG
				if (strstr(parUrl, "login") == NULL)
#endif
					proto->MSN_DebugLog( "Sending SSL query:\n%s", parAuthInfo );

				pfn_SSL_write( ssl, headers, strlen( headers ));
				pfn_SSL_write( ssl, (void*)parAuthInfo, strlen( parAuthInfo ));
				
				nBytes = 0;
				size_t dwTotSize = 8192;
				result = ( char* )mir_alloc( dwTotSize );

				for (;;) 
				{
					int dwSize = pfn_SSL_read( ssl, result+nBytes, dwTotSize - nBytes );
					if (dwSize  < 0) { nBytes = 0; break; }
					if (dwSize == 0) break;

					nBytes += dwSize;
					if ( nBytes >= dwTotSize ) {
						dwTotSize += 4096;
						char* rest = (char*)mir_realloc( result, dwTotSize );
						if ( rest == NULL )
							nBytes = 0;
						else 
							result = rest;
					}
				}
				result[nBytes] = 0;

				if ( nBytes > 0 ) 
				{
					proto->MSN_DebugLog( "SSL read successfully read %d bytes:", nBytes );
					MSN_CallService( MS_NETLIB_LOG, ( WPARAM )proto->hNetlibUser, ( LPARAM )result );

					if ( strncmp( result, "HTTP/1.1 100", 12 ) == 0 ) 
					{
						char* rest = strstr( result + 12, "HTTP/1.1" );
						if (rest) memmove(result, rest, nBytes + 1 - ( rest - result )); 
						else nBytes = 0;
					}
				}
				if (nBytes == 0)
				{
					mir_free( result );
					result = NULL;
					proto->MSN_DebugLog( "SSL read failed" );
				}
			}
			else proto->MSN_DebugLog( "SSL connection failed" );
		}
		else proto->MSN_DebugLog( "pfn_SSL_connect failed" );

		pfn_SSL_free( ssl );
	}
	else proto->MSN_DebugLog( "pfn_SSL_new failed" );

	Netlib_CloseHandle( h );
	return result;
}

SSLAgent::SSLAgent(CMsnProto* proto)
{
	unsigned useOpenSSL = proto->getByte( "UseOpenSSL", false );

	if ( useOpenSSL )
		pAgent = new SSL_OpenSsl(proto);
	else
		pAgent = new SSL_WinInet(proto);

	if ( pAgent->init() ) {
		delete pAgent;
		pAgent = NULL;
	}
}


SSLAgent::~SSLAgent()
{
	if (pAgent) delete pAgent;
}


char* SSLAgent::getSslResult(char** parUrl, const char* parAuthInfo, const char* hdrs, 
							 unsigned& status, char*& htmlbody)
{
	status = 0;
	char* tResult = NULL;
	if (pAgent != NULL)
	{
		MimeHeaders httpinfo;

lbl_retry:
		tResult = pAgent->getSslResult(*parUrl, parAuthInfo, hdrs);
		if (tResult != NULL)
		{
			char* htmlhdr = httpParseHeader( tResult, status );
			htmlbody = httpinfo.readFromBuffer( htmlhdr );
			if (status == 301 || status == 302)
			{
				const char* loc = httpinfo[ "Location" ];
				if (loc != NULL)
				{
					pAgent->proto->MSN_DebugLog( "Redirected to '%s'", loc );
					mir_free(*parUrl);
					*parUrl = mir_strdup(loc);
					mir_free(tResult);
					goto lbl_retry;
				}
			}
		}
	}
	return tResult;
}


void UninitSsl( void )
{
	if ( hLibSSL ) 
	{
		pfn_SSL_CTX_free( sslCtx );

//		proto->MSN_DebugLog( "Free SSL library" );
		FreeLibrary( hLibSSL );
	}
}
