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

#define ERROR_FLAGS (FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_GENERATE_DATA | FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS )

#include "wininet.h"

#define SSL_BUF_SIZE 4096

typedef BOOL  ( WINAPI *ft_HttpQueryInfo )( HINTERNET, DWORD, LPVOID, LPDWORD, LPDWORD );
typedef BOOL  ( WINAPI *ft_HttpSendRequest )( HINTERNET, LPCSTR, DWORD, LPVOID, DWORD );
typedef BOOL  ( WINAPI *ft_InternetCloseHandle )( HINTERNET );
typedef DWORD ( WINAPI *ft_InternetErrorDlg )( HWND, HINTERNET, DWORD, DWORD, LPVOID* );
typedef BOOL  ( WINAPI *ft_InternetSetOption )( HINTERNET, DWORD, LPVOID, DWORD );
typedef BOOL  ( WINAPI *ft_InternetReadFile )( HINTERNET, LPVOID, DWORD, LPDWORD );

typedef HINTERNET ( WINAPI *ft_HttpOpenRequest )( HINTERNET, LPCSTR, LPCSTR, LPCSTR, LPCSTR, LPCSTR*, DWORD, DWORD );
typedef HINTERNET ( WINAPI *ft_InternetConnect )( HINTERNET, LPCSTR, INTERNET_PORT, LPCSTR, LPCSTR, DWORD, DWORD, DWORD );
typedef HINTERNET ( WINAPI *ft_InternetOpen )( LPCSTR, DWORD, LPCSTR, LPCSTR, DWORD );

static ft_InternetCloseHandle f_InternetCloseHandle;
static ft_InternetConnect     f_InternetConnect;
static ft_InternetErrorDlg    f_InternetErrorDlg;
static ft_InternetOpen        f_InternetOpen;
static ft_InternetReadFile    f_InternetReadFile;
static ft_InternetSetOption   f_InternetSetOption;
static ft_HttpOpenRequest     f_HttpOpenRequest;
static ft_HttpQueryInfo       f_HttpQueryInfo;
static ft_HttpSendRequest     f_HttpSendRequest;

static void  sttApplyProxy( HINTERNET parHandle )
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

static void sttSuckMethan( HINTERNET hRequest )
{
	DWORD dwSize;

	do {
		char tmpbuf[100];
		f_InternetReadFile( hRequest, tmpbuf, 50, &dwSize);
	}
		while (dwSize != 0);
}

static char* sttSslGet( char* parUrl, char* parChallenge )
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

	char tUserEmail[ MSN_MAX_EMAIL_LEN ];

	MSN_GetStaticString( "e-mail", NULL, tUserEmail, sizeof tUserEmail );
	char* p = strchr( tUserEmail, '@' );
	if ( p != NULL )
	{	*p = 0;
		mir_snprintf( tBuffer, SSL_BUF_SIZE, "%s%%40%s", tUserEmail, p+1 );
		strcpy( tUserEmail, tBuffer );
	}

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
				sttApplyProxy( tRequest );

			if ( parChallenge != NULL ) {
				char tPassword[ 100 ];
				MSN_GetStaticString( "Password", NULL, tPassword, sizeof tPassword );
				MSN_CallService( MS_DB_CRYPT_DECODESTRING, strlen( tPassword )+1, ( LPARAM )tPassword );

				int tBytes = mir_snprintf( tBuffer, SSL_BUF_SIZE,
					"Authorization: Passport1.4 OrgVerb=GET,OrgURL=http%%3A%%2F%%2Fmessenger%%2Emsn%%2Ecom,sign-in=%s,pwd=",
					tUserEmail );

				UrlEncode( tPassword, tBuffer + tBytes, SSL_BUF_SIZE - tBytes );
				strcat( tBuffer, "," );
				strcat( tBuffer, parChallenge );
			}
			else tBuffer[ 0 ] = 0;

LBL_Restart:
			MSN_DebugLog( "Sending request..." );
			DWORD tErrorCode = f_HttpSendRequest( tRequest, tBuffer, DWORD(-1), NULL, 0 );
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
					tSslAnswer = sttSslGet( tBuffer, parChallenge );
					break;

				case HTTP_STATUS_DENIED:
				case HTTP_STATUS_PROXY_AUTH_REQ:
					if ( tUsesProxy && !bProxyParamsSubstituted ) {
						bProxyParamsSubstituted = true;
						sttApplyProxy( tUrlHandle );
						sttSuckMethan( tRequest );
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
						sttSuckMethan( tRequest );
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

int MSN_Auth8( char* authChallengeInfo, char*& parResult )
{
	parResult = NULL;
	int retVal = 0;

	HMODULE tDll = LoadLibrary( "WinInet.dll" );
	if ( tDll == NULL )
		return 10;

	f_InternetCloseHandle = (ft_InternetCloseHandle)GetProcAddress( tDll, "InternetCloseHandle" );
	f_InternetConnect = (ft_InternetConnect)GetProcAddress( tDll, "InternetConnectA" );
	f_InternetErrorDlg = (ft_InternetErrorDlg)GetProcAddress( tDll, "InternetErrorDlg" );
	f_InternetOpen = (ft_InternetOpen)GetProcAddress( tDll, "InternetOpenA" );
	f_InternetReadFile = (ft_InternetReadFile)GetProcAddress( tDll, "InternetReadFile" );
	f_InternetSetOption = (ft_InternetSetOption)GetProcAddress( tDll, "InternetSetOptionA" );
	f_HttpOpenRequest = (ft_HttpOpenRequest)GetProcAddress( tDll, "HttpOpenRequestA" );
	f_HttpQueryInfo = (ft_HttpQueryInfo)GetProcAddress( tDll, "HttpQueryInfoA" );
	f_HttpSendRequest = (ft_HttpSendRequest)GetProcAddress( tDll, "HttpSendRequestA" );

	if ( msnLoginHost == NULL )
	{
		msnLoginHost = sttSslGet( "https://nexus.passport.com/rdr/pprdr.asp", NULL );
		if ( msnLoginHost == NULL )
		{
			retVal = 1;
LBL_Exit:
			if ( msnRunningUnderNT )	// we free WININET.DLL only if we're under NT
				FreeLibrary( tDll );
			return retVal;
		}

		char* p = strstr( msnLoginHost, "DALogin=" );
		if ( p == NULL )
		{	free( msnLoginHost ); msnLoginHost = NULL;
			retVal = 2;
			goto LBL_Exit;
		}

		strcpy( msnLoginHost, "https://" );
		strdel( msnLoginHost+8, int( p-msnLoginHost ));
		if (( p = strchr( msnLoginHost, ',' )) != 0 )
			*p = 0;
	}

	char* tResult = sttSslGet( msnLoginHost, authChallengeInfo );
	if ( tResult == NULL )
	{	retVal = 3;
		goto LBL_Exit;
	}

	char* p = strstr( tResult, "from-PP=" );
	if ( p == NULL )
	{	free( tResult );
		retVal = 4;
		goto LBL_Exit;
	}

	strdel( tResult, int( p-tResult )+9 );

	if (( p = strchr( tResult, '\'' )) != NULL )
		*p = 0;

	parResult = tResult;
	goto LBL_Exit;
}
