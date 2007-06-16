/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-7 Boris Krasnovskiy.
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

static const char defaultPassportUrl[] = "https://login.live.com/RST.srf";

static const char authPacket[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<Envelope xmlns=\"http://schemas.xmlsoap.org/soap/envelope/\""
		" xmlns:wsse=\"http://schemas.xmlsoap.org/ws/2003/06/secext\"" 
		" xmlns:saml=\"urn:oasis:names:tc:SAML:1.0:assertion\"" 
		" xmlns:wsp=\"http://schemas.xmlsoap.org/ws/2002/12/policy\"" 
		" xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\"" 
		" xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/03/addressing\"" 
		" xmlns:wssc=\"http://schemas.xmlsoap.org/ws/2004/04/sc\"" 
		" xmlns:wst=\"http://schemas.xmlsoap.org/ws/2004/04/trust\">"
	"<Header>"
		"<ps:AuthInfo xmlns:ps=\"http://schemas.microsoft.com/Passport/SoapServices/PPCRL\" Id=\"PPAuthInfo\">"
			"<ps:HostingApp>{7108E71A-9926-4FCB-BCC9-9A9D3F32E423}</ps:HostingApp>"
			"<ps:BinaryVersion>3</ps:BinaryVersion>"
			"<ps:UIVersion>1</ps:UIVersion>"
			"<ps:Cookies></ps:Cookies>"
			"<ps:RequestParams>AQAAAAIAAABsYwQAAAAxMDMz</ps:RequestParams>"
		"</ps:AuthInfo>"
		"<wsse:Security>"
			"<wsse:UsernameToken Id=\"user\">"
				"<wsse:Username>%s</wsse:Username>"
				"<wsse:Password>%s</wsse:Password>"
			"</wsse:UsernameToken>"
		"</wsse:Security>"
	"</Header>"
	"<Body>"
		"<ps:RequestMultipleSecurityTokens xmlns:ps=\"http://schemas.microsoft.com/Passport/SoapServices/PPCRL\" Id=\"RSTS\">"
			"<wst:RequestSecurityToken Id=\"RST0\">"
				"<wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>"
				"<wsp:AppliesTo>"
					"<wsa:EndpointReference>"
						"<wsa:Address>http://Passport.NET/tb</wsa:Address>"
					"</wsa:EndpointReference>"
				"</wsp:AppliesTo>"
			"</wst:RequestSecurityToken>"
			"<wst:RequestSecurityToken Id=\"RST1\">"
				"<wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>"
				"<wsp:AppliesTo>"
					"<wsa:EndpointReference>"
						"<wsa:Address>messenger.msn.com</wsa:Address>"
					"</wsa:EndpointReference>"
				"</wsp:AppliesTo>"
				"<wsse:PolicyReference URI=\"?%s\"></wsse:PolicyReference>"
			"</wst:RequestSecurityToken>"
		"</ps:RequestMultipleSecurityTokens>"
	"</Body>"
"</Envelope>";

static const char oimRecvPacket[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>" 
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" 
		" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\""
		" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
	"<soap:Header>"
		"<PassportCookie xmlns=\"http://www.hotmail.msn.com/ws/2004/09/oim/rsi\">" 
			"<t>%s</t>"
			"<p>%s</p>" 
		"</PassportCookie>"
	"</soap:Header>"
	"<soap:Body>"
		"<%s xmlns=\"http://www.hotmail.msn.com/ws/2004/09/oim/rsi\">"
		"%s"
		"</%s>"
	"</soap:Body>"
"</soap:Envelope>";

static const char oimSendPacket[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
	"<soap:Header>"
		"<From memberName=\"%s\" friendlyName=\"=?utf-8?B?%s?=\" xml:lang=\"nl-nl\" proxy=\"MSNMSGR\" xmlns=\"http://messenger.msn.com/ws/2004/09/oim/\" msnpVer=\"MSNP13\" buildVer=\"8.0.0328\"/>"
		"<To memberName=\"%s\" xmlns=\"http://messenger.msn.com/ws/2004/09/oim/\"/>"
		"<Ticket passport=\"%s\" appid=\"PROD01065C%ZFN6F\" lockkey=\"%s\" xmlns=\"http://messenger.msn.com/ws/2004/09/oim/\"/>"
		"<Sequence xmlns=\"http://schemas.xmlsoap.org/ws/2003/03/rm\">"
			"<Identifier xmlns=\"http://schemas.xmlsoap.org/ws/2002/07/utility\">http://messenger.msn.com</Identifier&gt;"
			"<MessageNumber>%d</MessageNumber>"
		"</Sequence>"
	"</soap:Header>"
	"<soap:Body>"
		"<MessageType xmlns=\"http://messenger.msn.com/ws/2004/09/oim/\">text</MessageType>"
		"<Content xmlns=\"http://messenger.msn.com/ws/2004/09/oim/\">"
			"MIME-Version: 1.0\r\n"
			"Content-Type: text/plain; charset=UTF-8\r\n"
			"Content-Transfer-Encoding: base64\r\n"
			"X-OIM-Message-Type: OfflineMessage\r\n"
			"X-OIM-Run-Id: {3A3BE82C-684D-4F4F-8005-CBE8D4F82BAD}\r\n"
			"X-OIM-Sequence-Num: %d\r\n"
			"\r\n%s"
		"</Content>"
	"</soap:Body>"
"</soap:Envelope>";

static const char oimGetAction[] = "<messageId>%s</messageId> <alsoMarkAsRead>%s</alsoMarkAsRead>";
static const char oimDeleteAction[] = "<messageIds>%s</messageIds>";
static const char oimDeleteActionAtom[] = "<messageId>%s</messageId>"; 

char pToken[256], tToken[256]; 


/////////////////////////////////////////////////////////////////////////////////////////
// Basic SSL operation class

struct SSL_Base
{
	virtual	~SSL_Base() {}

	virtual  int init() = 0;
	virtual  char* getSslResult( char* parUrl, char* parAuthInfo, char* hdrs ) = 0;
};

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

struct SSL_WinInet : public SSL_Base
{
	virtual ~SSL_WinInet();

	virtual  char* getSslResult( char* parUrl, char* parAuthInfo, char* hdrs );
	virtual  int init();

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

	MSN_DebugLog( "Applying proxy parameters..." );

	if ( !MSN_GetStaticString( "NLProxyAuthUser", NULL, tBuffer, sizeof( tBuffer )))
		f_InternetSetOption( parHandle, INTERNET_OPTION_PROXY_USERNAME, tBuffer, strlen( tBuffer )+1);
	else
		MSN_DebugLog( "Warning: proxy user name is required but missing" );

	if ( !MSN_GetStaticString( "NLProxyAuthPassword", NULL, tBuffer, sizeof( tBuffer ))) {
		MSN_CallService( MS_DB_CRYPT_DECODESTRING, strlen( tBuffer ), ( LPARAM )tBuffer );
		f_InternetSetOption( parHandle, INTERNET_OPTION_PROXY_PASSWORD, tBuffer, strlen( tBuffer )+1);
	}
	else MSN_DebugLog( "Warning: proxy user password is required but missing" );
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
			f_InternetReadFile( hRequest, tSslAnswer+dwOffset, dwSize - dwOffset, &tBufSize);
			dwOffset += tBufSize;
		}
		while (tBufSize != 0 && dwOffset < dwSize);
		tSslAnswer[dwOffset] = 0;

		MSN_DebugLog( "SSL response:" );
		MSN_CallService( MS_NETLIB_LOG, ( WPARAM )hNetlibUser, ( LPARAM )tSslAnswer );
	}

	return tSslAnswer;
}


char* SSL_WinInet::getSslResult( char* parUrl, char* parAuthInfo, char* hdrs )
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

	if ( MyOptions.UseProxy ) {
		DWORD ptype = MSN_GetByte( "NLProxyType", 0 );
		if ( !MSN_GetByte( "UseIeProxy", 0 ) && ( ptype == PROXYTYPE_HTTP || ptype == PROXYTYPE_HTTPS )) {
			char szProxy[ 100 ];
			if ( MSN_GetStaticString( "NLProxyServer", NULL, szProxy, sizeof( szProxy ))) {
				MSN_DebugLog( "Proxy server name should be set if proxy is used" );
				return NULL;
			}

			int tPortNumber = MSN_GetWord( NULL, "NLProxyPort", -1 );
			if ( tPortNumber == -1 ) {
				MSN_DebugLog( "Proxy server port should be set if proxy is used" );
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
		MSN_DebugLog( "InternetOpen() failed" );
		return NULL;
	}

	MSN_DebugLog( "SSL request (%s): '%s'", MyOptions.UseProxy ? "using proxy": "direct connection", parUrl );

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
		HINTERNET tRequest = f_HttpOpenRequest( tUrlHandle, "POST", tObjectName, NULL, NULL, NULL, tFlags, NULL );
		if ( tRequest != NULL ) {

			unsigned tm = 6000;
			f_InternetSetOption( tRequest, INTERNET_OPTION_CONNECT_TIMEOUT, &tm, sizeof(tm));
			f_InternetSetOption( tRequest, INTERNET_OPTION_SEND_TIMEOUT, &tm, sizeof(tm));
			f_InternetSetOption( tRequest, INTERNET_OPTION_RECEIVE_TIMEOUT, &tm, sizeof(tm));

		if ( MyOptions.UseProxy && MSN_GetByte( "NLUseProxyAuth", 0  ))
			applyProxy( tRequest );

		char headers[2048];
		mir_snprintf(headers, sizeof( headers ), 
			"Accept: txt/xml\r\nContent-Type: text/xml; charset=utf-8\r\n%s", 
			hdrs ? hdrs : "");

		bool restart = false;

LBL_Restart:
			MSN_DebugLog( "Sending request..." );
//			MSN_DebugLog( parAuthInfo );
			DWORD tErrorCode = f_HttpSendRequest( tRequest, headers, strlen( headers ), 
				parAuthInfo, strlen( parAuthInfo ));
			if ( tErrorCode == 0 ) {
				TWinErrorCode errCode;
				MSN_DebugLog( "HttpSendRequest() failed with error %d: %s", errCode.mErrorCode, errCode.getText());

				switch( errCode.mErrorCode ) {
					case 2:
						MSN_ShowError( "Internet Explorer is in the 'Offline' mode. Switch IE to the 'Online' mode and then try to relogin" );
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
						MSN_ShowError( "MSN Passport verification failed with error %d: %s",
							errCode.mErrorCode, errCode.getText());
				}


			}
			else {
				DWORD dwCode;
				DWORD tBufSize = sizeof( dwCode );
				f_HttpQueryInfo( tRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwCode, &tBufSize, 0 );

				switch( dwCode ) {
					case HTTP_STATUS_REDIRECT:
					case HTTP_STATUS_OK:
						tSslAnswer = readData( tRequest );
						break;

					// else fall into the general error handling routine
					default:
						mir_free( readData( tRequest ));
						tSslAnswer = NULL;
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
	virtual  char* getSslResult( char* parUrl, char* parAuthInfo, char* hdrs );
	virtual  int init();

	static	PVOID sslCtx;

	static	HMODULE hLibSSL, hLibEAY;
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
};

/////////////////////////////////////////////////////////////////////////////////////////

PVOID		SSL_OpenSsl::sslCtx = NULL;

HMODULE	SSL_OpenSsl::hLibSSL = NULL,
		SSL_OpenSsl::hLibEAY = NULL;

PFN_SSL_int_void            SSL_OpenSsl::pfn_SSL_library_init;
PFN_SSL_pvoid_void          SSL_OpenSsl::pfn_TLSv1_client_method;
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

	if ( retVal ) {
		FreeLibrary( hLibSSL );
		MSN_ShowError( "Valid %s must be installed to perform the SSL login", "LIBSSL32.DLL" );
		return 1;
	}

	pfn_SSL_library_init();
	sslCtx = pfn_SSL_CTX_new( pfn_TLSv1_client_method());
	MSN_DebugLog( "OpenSSL context successully allocated" );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

char* SSL_OpenSsl::getSslResult( char* parUrl, char* parAuthInfo, char* hdrs )
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

	NETLIBUSERSETTINGS nls = { 0 };
	nls.cbSize = sizeof( nls );
	MSN_CallService(MS_NETLIB_GETUSERSETTINGS,WPARAM(hNetlibUser),LPARAM(&nls));
	int cpType = nls.proxyType;

	if (nls.useProxy && cpType == PROXYTYPE_HTTP)
	{
		nls.proxyType = PROXYTYPE_HTTPS;
		nls.szProxyServer = NEWSTR_ALLOCA(nls.szProxyServer);
		nls.szIncomingPorts = NEWSTR_ALLOCA(nls.szIncomingPorts);
		nls.szOutgoingPorts = NEWSTR_ALLOCA(nls.szOutgoingPorts);
		nls.szProxyAuthPassword = NEWSTR_ALLOCA(nls.szProxyAuthPassword);
		nls.szProxyAuthUser = NEWSTR_ALLOCA(nls.szProxyAuthUser);
		MSN_CallService(MS_NETLIB_SETUSERSETTINGS,WPARAM(hNetlibUser),LPARAM(&nls));
	}

	NETLIBOPENCONNECTION tConn = { 0 };
	tConn.cbSize = sizeof( tConn );
	tConn.szHost = url+8;
	tConn.wPort = 443;
	HANDLE h = ( HANDLE )MSN_CallService( MS_NETLIB_OPENCONNECTION, ( WPARAM )hNetlibUser, ( LPARAM )&tConn );
	
	if (nls.useProxy && cpType == PROXYTYPE_HTTP)
	{
		nls.proxyType = PROXYTYPE_HTTP;
		nls.szProxyServer = NEWSTR_ALLOCA(nls.szProxyServer);
		nls.szIncomingPorts = NEWSTR_ALLOCA(nls.szIncomingPorts);
		nls.szOutgoingPorts = NEWSTR_ALLOCA(nls.szOutgoingPorts);
		nls.szProxyAuthPassword = NEWSTR_ALLOCA(nls.szProxyAuthPassword);
		nls.szProxyAuthUser = NEWSTR_ALLOCA(nls.szProxyAuthUser);
		MSN_CallService(MS_NETLIB_SETUSERSETTINGS,WPARAM(hNetlibUser),LPARAM(&nls));
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
				MSN_DebugLog( "SSL connection succeeded" );

				char headers[2560];
				unsigned nBytes = mir_snprintf( headers, sizeof( headers ),
					"POST /%s HTTP/1.1\r\n"
					"Accept: text/xml\r\n"
					"%s"
					"User-Agent: %s\r\n"
					"Content-Length: %u\r\n"
					"Content-Type: text/xml; charset=utf-8\r\n"
					"Host: %s\r\n"
					"Connection: close\r\n"
					"Cache-Control: no-cache\r\n\r\n", path, hdrs ? hdrs : "",
					MSN_USER_AGENT, strlen( parAuthInfo ), url+8 );

//				MSN_DebugLog( "Sending SSL query:\n%s", buf );
				pfn_SSL_write( ssl, headers, strlen( headers ));
				pfn_SSL_write( ssl, parAuthInfo, strlen( parAuthInfo ));
				
				nBytes = 0;
				unsigned dwSize, dwTotSize = 8192;
				result = ( char* )mir_alloc( dwTotSize );

				do {
					dwSize = pfn_SSL_read( ssl, result+nBytes, dwTotSize - nBytes );
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
				while (dwSize != 0);
				result[nBytes] = 0;

				if ( nBytes > 0 ) {
					if ( strncmp( result, "HTTP/1.1 100", 12 ) == 0 ) {
						char* rest = strstr( result + 12, "HTTP/1.1" );
						memmove(result, rest, nBytes + 1 - ( rest - result )); 
					}

					MSN_DebugLog( "SSL read successfully read %d bytes:", nBytes );
					MSN_CallService( MS_NETLIB_LOG, ( WPARAM )hNetlibUser, ( LPARAM )result );
				}
				else
				{
					mir_free( result );
					result = NULL;
					MSN_DebugLog( "SSL read failed" );
				}
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

	char szPassword[ 100 ];
	MSN_GetStaticString( "Password", NULL, szPassword, sizeof( szPassword ));
	MSN_CallService( MS_DB_CRYPT_DECODESTRING, strlen( szPassword )+1, ( LPARAM )szPassword );
	szPassword[ 16 ] = 0;
	char* szEncPassword = HtmlEncode(szPassword);

	// Replace ',' with '&' 
	char *p = authChallengeInfo;
	while( *p != 0 ) {
		if ( *p == ',' ) *p = '&';
		++p;
	}
	char* szEncAuthInfo = HtmlEncode(authChallengeInfo);

	char* szAuthInfo = ( char* )alloca( 3072 );
	int nBytes = mir_snprintf( szAuthInfo, 3072, authPacket, MyOptions.szEmail, szEncPassword, szEncAuthInfo );

	mir_free( szEncAuthInfo );
	mir_free( szEncPassword );

	char szPassportHost[ 256 ];
	if ( MSN_GetStaticString( "MsnPassportHost", NULL, szPassportHost, sizeof( szPassportHost )) ||
		 strstr( szPassportHost, "/RST.srf" ) == NULL )
		strcpy( szPassportHost, defaultPassportUrl );

	bool defaultUrlAllow = strcmp( szPassportHost, defaultPassportUrl ) != 0;
	char *tResult = NULL;

	for (;;)
	{
		mir_free( tResult );
		tResult = pAgent->getSslResult( szPassportHost, szAuthInfo, NULL );
		if ( tResult == NULL ) {
			if ( defaultUrlAllow ) {
				strcpy( szPassportHost, defaultPassportUrl );
				defaultUrlAllow = false;
				continue;
			}
			else {
				retVal = 4;
				break;
		}	}

		int status = 0;
		sscanf( tResult, "HTTP/1.1 %d", &status );
		if ( status == 302 ) // Handle redirect
		{
			if (txtParseParam(tResult, NULL, "Location:", "\r", szPassportHost, sizeof(szPassportHost))) 
				MSN_DebugLog( "Redirected to '%s'", szPassportHost );
			else break;
		}
		else if (status == 200) 
		{
			if (txtParseParam(tResult, NULL, "<psf:redirectUrl>", "<", szPassportHost, sizeof(szPassportHost))) 
				MSN_DebugLog( "Redirected to '%s'", szPassportHost );
			else break;
		}
		else
		{
			if ( defaultUrlAllow ) {
				strcpy( szPassportHost, defaultPassportUrl );
				defaultUrlAllow = false;
			}
			else {
				retVal = 6;
				break;
	}	}	}

	if ( retVal == 0 ) 	{
		if ( txtParseParam( tResult, "<wsse:BinarySecurityToken", ">", "<", tResult, strlen( tResult ))) {
			HtmlDecode( tResult );
			MSN_SetString( NULL, "MsnPassportHost", szPassportHost );
			parResult = tResult;
		}
		else
			retVal = strstr( tResult, "wsse:FailedAuthentication" ) ? 3 : 5;
	}

	if ( retVal != 0 ) {
		mir_free( tResult );
		MSN_ShowError( retVal == 3 ? "Your username or password is incorrect" : 
			"Unable to contact MS Passport servers check proxy/firewall settings" );
		MSN_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD );
	}
	else
	{
		char * pt = strstr(tResult, "&p=");
		int ptlen = pt - tResult - 2;
		strncpy(tToken, tResult+2, ptlen);
		tToken[ptlen] = 0;
		strcpy(pToken, pt+3);
	}

	MSN_DebugLog( "MSN_CheckRedirector exited with errorCode = %d", retVal );

	delete pAgent;
	return retVal;
}

void MSN_GetOIMs( const char* initxml )
{
	SSL_Base* pAgent;
	if ( MSN_GetByte( "UseOpenSSL", 0 ))
		pAgent = new SSL_OpenSsl();
	else
		pAgent = new SSL_WinInet();
	if ( pAgent == NULL )
		return;

	if ( pAgent->init() ) {
		delete pAgent;
		return;
	}

	const char *xmlst = initxml;

	char* szDelAct = ( char* )mir_calloc( 1 );
	size_t lenDelAct = 1;

	for (;;)
	{
		char szId[128], szEmail[MSN_MAX_EMAIL_LEN];
		char szData1[1024], szData[2048];

		xmlst = strstr( xmlst, "<M>" );
		if ( xmlst == NULL ) break;

		txtParseParam( xmlst, NULL, "<I>", "</I>", szId, sizeof( szId ));
		txtParseParam( xmlst, NULL, "<E>", "</E>", szEmail, sizeof( szEmail ));

		mir_snprintf( szData1, sizeof( szData1 ), oimGetAction, szId, "false");
		mir_snprintf( szData, sizeof( szData ), oimRecvPacket, tToken, pToken, "GetMessage", szData1, "GetMessage" );

		char* tResult = pAgent->getSslResult( "https://rsi.hotmail.com/rsi/rsi.asmx", szData,
			"SOAPAction: \"http://www.hotmail.msn.com/ws/2004/09/oim/rsi/GetMessage\"\r\n" );

		if (tResult != NULL)
		{
			char szTime[32], *p;
			time_t evtm;

			txtParseParam( tResult, "FILETIME", "[", "]", szTime, sizeof( szTime ));
			
			unsigned filetimeLo = strtoul( szTime, &p, 16 );
			if ( *p == ':' ) { 
				unsigned __int64 filetime = strtoul( p+1, &p, 16 );
				filetime <<= 32;
				filetime |= filetimeLo;
				filetime /= 10000000;
				filetime -= 11644473600ui64;
				evtm = ( time_t )filetime;
			}
			else
				evtm = time( NULL );

			txtParseParam( tResult, "<GetMessageResult>", "\r\n\r\n", "</GetMessageResult>", tResult, strlen( tResult ));

			char* ch = tResult;
			size_t len = strlen(tResult) + 1;
			while (*ch != 0)
			{
				if ( *ch == '\n' || *ch == '\r' )
					memmove( ch, ch+1, len-- - ( ch - tResult ));
				else
					++ch;
			}
			
			char* szMsg = MSN_Base64Decode( tResult );

			mir_free( tResult );
			szMsg = ( char* )mir_realloc( szMsg, strlen( szMsg ) * 3 + 4 ); 

			wchar_t* szMsgU = NULL;
			mir_utf8decode( szMsg, &szMsgU );

			size_t alen = strlen( szMsg );

			wcscpy(( wchar_t* )( szMsg+alen+1 ), szMsgU );
			
			mir_free( szMsgU );

			PROTORECVEVENT pre;
			pre.szMessage = szMsg;
			pre.flags = PREF_UNICODE /*+ (( isRtl ) ? PREF_RTL : 0)*/;
			pre.timestamp = evtm;
			pre.lParam = 0;

			CCSDATA ccs;
			ccs.hContact = MSN_HContactFromEmail( szEmail, szEmail, 0, 0 );
			ccs.szProtoService = PSR_MESSAGE;
			ccs.wParam = 0;
			ccs.lParam = ( LPARAM )&pre;
			MSN_CallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );

			lenDelAct += mir_snprintf( szData1, sizeof( szData1 ), oimDeleteActionAtom, szId);
			szDelAct = ( char* )mir_realloc( szDelAct, lenDelAct );
			strcat( szDelAct, szData1 );

			mir_free( szMsg );
		}

		xmlst += 3;
	}
	
	if ( lenDelAct > 1 )
	{
		lenDelAct += sizeof( oimDeleteAction );
		char* szDelActAll = ( char* )alloca ( lenDelAct );
		mir_snprintf( szDelActAll, lenDelAct, oimDeleteAction, szDelAct);
		
		lenDelAct += sizeof( oimRecvPacket ) + sizeof( tToken ) + sizeof( pToken ) + 28;
		char* szDelPack = ( char* )alloca ( lenDelAct );
		mir_snprintf( szDelPack, lenDelAct, oimRecvPacket, tToken, pToken, "DeleteMessages", szDelActAll, "DeleteMessages" );
			
		char* tResult = pAgent->getSslResult( "https://rsi.hotmail.com/rsi/rsi.asmx", szDelPack,
			"SOAPAction: \"http://www.hotmail.msn.com/ws/2004/09/oim/rsi/DeleteMessages\"\r\n" );
		mir_free( tResult );
	}
	mir_free( szDelAct );

	delete pAgent;
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
