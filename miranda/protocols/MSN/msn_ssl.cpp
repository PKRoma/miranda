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
// Performs the MSN Passport login via SSL3 using the OpenSSL library

class SSL_OpenSsl : public SSL_Base
{
public:
	SSL_OpenSsl(CMsnProto* prt) : SSL_Base(prt) {}

	virtual  char* getSslResult( const char* parUrl, const char* parAuthInfo, const char* hdrs );
};


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
	tConn.flags = NLOCF_SSL;
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
		
	if ( h == NULL ) return NULL;

	char* result = NULL;

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

	int flags = 0;

	Netlib_Send( h, headers, strlen( headers ), flags);

#ifndef _DEBUG
	if (strstr(parUrl, "login")) flags |= MSG_NODUMP;
#endif

	Netlib_Send( h, parAuthInfo, strlen( parAuthInfo ), flags);
	proto->MSN_DebugLog( "SSL All data sent" );

	nBytes = 0;
	size_t dwTotSize = 8192;
	result = ( char* )mir_alloc( dwTotSize );

	for (;;) 
	{
		int dwSize = Netlib_Recv( h, result+nBytes, dwTotSize - nBytes, 0 );
		if (dwSize  < 0) { nBytes = 0; break; }
		if (dwSize == 0) break;

		nBytes += dwSize;
		if ( nBytes >= dwTotSize ) 
		{
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

	Netlib_CloseHandle( h );
	return result;
}

SSLAgent::SSLAgent(CMsnProto* proto)
{
	pAgent = new SSL_OpenSsl(proto);
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
