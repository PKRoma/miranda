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
#ifdef HTTP_GATEWAY

#include <windows.h>
#include "yahoo.h"

char *Bcookie = NULL;

int YAHOO_httpGatewayInit(HANDLE hConn, NETLIBOPENCONNECTION *nloc, NETLIBHTTPREQUEST *nlhr)
{
	char szHttpPostUrl[300];
	NETLIBHTTPPROXYINFO nlhpi;

	YAHOO_DebugLog("YAHOO_httpGatewayInit!!!");
	
	ZeroMemory(&nlhpi, sizeof(nlhpi) );
	nlhpi.cbSize = sizeof(nlhpi);
	nlhpi.flags = 0; /* NO SEQUENCE NUMBERS */
	/*nlhpi.flags = NLHPIF_USEPOSTSEQUENCE;*/
	nlhpi.szHttpGetUrl = NULL;
	nlhpi.szHttpPostUrl = szHttpPostUrl;
	/*nlhpi.firstPostSequence = 1;
	_snprintf(szHttpPostUrl, 300, "http://http.pager.yahoo.com/notify");*/
	lstrcpyn(szHttpPostUrl, "http://http.pager.yahoo.com/notify", 300);
	
	return CallService(MS_NETLIB_SETHTTPPROXYINFO, (WPARAM)hConn, (LPARAM)&nlhpi);
}


PBYTE YAHOO_httpGatewayUnwrapRecv(NETLIBHTTPREQUEST *nlhr, PBYTE buf, int len, int *outBufLen, void *(*NetlibRealloc)(void *, size_t))
{
    YAHOO_DebugLog("YAHOO_httpGatewayUnwrapRecv!!! Len: %d", len);

    YAHOO_DebugLog("Got headers: %d", nlhr->headersCount);
    /* we need to get the first 4 bytes! */
    if (len == 4){
        *outBufLen = 0;
        return buf;
    }

    if (len > 4) {
                        
            if ( (buf[4] == 'Y') && (buf[5] == 'M') && (buf[6] == 'S') && (buf[7] == 'G') ) {
                 int i;
                 
                 if ( (buf[14] == 0) && (buf[15] == 2) ) /* LOGOFF */
                                  return NULL;

                 for (i = 0; i < nlhr->headersCount; i++ ) 
                     if(!lstrcmpi(nlhr->headers[i].szName,"Set-Cookie")) {
                       char *c;
                       YAHOO_DebugLog("Got Cookie: %s", nlhr->headers[i].szValue);
                       Bcookie = strdup(nlhr->headers[i].szValue);
                       
                       c = strchr(Bcookie, ';');
                       if (c != NULL)
                         (*c) = '\0';
                         
                       YAHOO_DebugLog("Bcookie: %s", Bcookie);  
				       break;
                     }
                 
				MoveMemory( buf, buf + 4, len - 4);
                 *outBufLen = len-4;/* we take off 4 bytes from the beginning*/
                 
                 return buf;                 
            } else
                 return NULL; /* Break connection, something went wrong! */
    } else
        return NULL; /* we need our counter!!! */

}

#endif
