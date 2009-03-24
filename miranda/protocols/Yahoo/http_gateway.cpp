/*
 * $Id$
 *
 * myYahoo Miranda Plugin 
 *
 * Authors: Gennady Feldman (aka Gena01) 
 *          Laurent Marechal (aka Peorth)
 *
 * This code is under GPL and is based on AIM, MSN and Miranda source code.
 * I want to thank Robert Rainwater and George Hazan for their code and support
 * and for answering some of my questions during development of this plugin.
 */

#include "yahoo.h"

#ifdef HTTP_GATEWAY

extern yahoo_local_account * ylad;

int YAHOO_httpGatewayInit(HANDLE hConn, NETLIBOPENCONNECTION *nloc, NETLIBHTTPREQUEST *nlhr)
{
	NETLIBHTTPPROXYINFO nlhpi;

	YAHOO_DebugLog("YAHOO_httpGatewayInit!!!");
	
	ZeroMemory(&nlhpi, sizeof(nlhpi) );
	nlhpi.cbSize = sizeof(nlhpi);
	nlhpi.szHttpPostUrl = "http://shttp.msg.yahoo.com/notify/";
	
	//CallService( MS_NETLIB_SETPOLLINGTIMEOUT, (WPARAM) hConn, 15 );
	
	return CallService(MS_NETLIB_SETHTTPPROXYINFO, (WPARAM)hConn, (LPARAM)&nlhpi);
}

int YAHOO_httpGatewayWrapSend(HANDLE hConn, PBYTE buf, int len, int flags, MIRANDASERVICE pfnNetlibSend)
{
	YAHOO_DebugLog("YAHOO_httpGatewayWrapSend!!! Len: %d", len);

	if (len == 0 && ylad->id > 0) { // we need to send something!!!
		int n;
		char *z = yahoo_webmessenger_idle_packet(ylad->id, &n);
		int ret = 0;
		
		if (z != NULL) {
			YAHOO_DebugLog("YAHOO_httpGatewayWrapSend!!! Got Len: %d", n);
			NETLIBBUFFER tBuf = { ( char* )z, n, flags };
			ret = pfnNetlibSend(( LPARAM )hConn, (WPARAM) &tBuf );
			FREE(z);
		} else {
			YAHOO_DebugLog("YAHOO_httpGatewayWrapSend!!! GOT NULL???");
		}
		
		return ret;
	} else {
		NETLIBBUFFER tBuf = { ( char* )buf, len, flags };
		
		return pfnNetlibSend(( LPARAM )hConn, (WPARAM) &tBuf );
	}
}

PBYTE YAHOO_httpGatewayUnwrapRecv(NETLIBHTTPREQUEST *nlhr, PBYTE buf, int len, int *outBufLen, void *(*NetlibRealloc)(void *, size_t))
{
    YAHOO_DebugLog("YAHOO_httpGatewayUnwrapRecv!!! Len: %d", len);

    YAHOO_DebugLog("Got headers: %d", nlhr->headersCount);
    /* we need to get the first 4 bytes! */
	if (len < 4) 
		return NULL;

	ylad->rpkts = buf[0] + buf[1] *256;
	YAHOO_DebugLog("Got packets: %d", ylad->rpkts);
	
    if (len == 4){
        *outBufLen = 0;
        return buf;
    } else  if ( (buf[4] == 'Y') && (buf[5] == 'M') && (buf[6] == 'S') && (buf[7] == 'G') ) {
		MoveMemory( buf, buf + 4, len - 4);
		*outBufLen = len-4;// we take off 4 bytes from the beginning
		 
		return buf;                 
    } else
        return NULL; /* Break connection, something went wrong! */
     
}

#endif
