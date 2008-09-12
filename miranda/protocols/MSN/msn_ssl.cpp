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

static int findHeader(NETLIBHTTPREQUEST *nlhrReply, char *hdr)
{
	int res = -1, i; 
	for (i=0; i<nlhrReply->headersCount; i++) 
	{
		if (_stricmp(nlhrReply->headers[i].szName, hdr) == 0) 
		{
			res = i;
			break;
		}
	}
	return res;
}


char* CMsnProto::getSslResult(char** parUrl, const char* parAuthInfo, const char* hdrs, unsigned& status) 
{
	char* result = NULL;
	NETLIBHTTPREQUEST nlhr = {0};

	// initialize the netlib request
	nlhr.cbSize = sizeof(nlhr);
	nlhr.requestType = REQUEST_POST;
	nlhr.flags = NLHRF_HTTP11 | NLHRF_DUMPASTEXT;
	nlhr.szUrl = *parUrl;
	nlhr.dataLength = strlen(parAuthInfo);
	nlhr.pData = (char*)parAuthInfo;

#ifndef _DEBUG
	if (strstr(*parUrl, "login")) nlhr.flags |= NLHRF_NODUMP;
#endif

	nlhr.headersCount = 5;
	nlhr.headers=(NETLIBHTTPHEADER*)mir_alloc(sizeof(NETLIBHTTPHEADER)*(nlhr.headersCount+5));
	nlhr.headers[0].szName   = "User-Agent";
	nlhr.headers[0].szValue = (char*)MSN_USER_AGENT;
	nlhr.headers[1].szName  = "Connection";
	nlhr.headers[1].szValue = "close";
	nlhr.headers[2].szName  = "Accept";
	nlhr.headers[2].szValue = "text/*";
	nlhr.headers[3].szName  = "Content-Type";
	nlhr.headers[3].szValue = "text/xml; charset=utf-8";
	nlhr.headers[4].szName  = "Cache-Control";
	nlhr.headers[4].szValue = "no-cache";

	if (hdrs)
	{
		char* hdrprs = NEWSTR_ALLOCA(hdrs);
		for (;;)
		{
			char* fnd = strchr(hdrprs, ':');
			if (fnd == NULL) break;
			*fnd = 0; 
			fnd += 2;

			nlhr.headers[nlhr.headersCount].szName  = hdrprs;
			nlhr.headers[nlhr.headersCount].szValue = fnd;

			fnd = strchr(fnd, '\r');
			*fnd = 0;
			hdrprs = fnd + 2;
			nlhr.headersCount++;
		}
	}

retry:
	// download the page
	NETLIBHTTPREQUEST *nlhrReply = (NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,
		(WPARAM)hNetlibUserHttps,(LPARAM)&nlhr);

	if (nlhrReply) 
	{
		status = nlhrReply->resultCode;
		// if the recieved code is 302 Moved, Found, etc
		// workaround for url forwarding
		if(nlhrReply->resultCode == 302 || nlhrReply->resultCode == 301 || nlhrReply->resultCode == 307) // page moved
		{
			// get the url for the new location and save it to szInfo
			// look for the reply header "Location"
			int i = findHeader(nlhrReply, "Location");
			if (i != -1) 
			{
				size_t rlen = 0;
				if (nlhrReply->headers[i].szValue[0] == '/')
				{
					char* szPath;
					char* szPref = strstr(*parUrl, "://");
					szPref = szPref ? szPref + 3 : *parUrl;
					szPath = strchr(szPref, '/');
					rlen = szPath != NULL ? szPath - *parUrl : strlen(*parUrl); 
				}

				nlhr.szUrl = (char*)mir_realloc(*parUrl, 
					rlen + strlen(nlhrReply->headers[i].szValue)*3 + 1);

				strncpy(nlhr.szUrl, *parUrl, rlen);
				strcpy(nlhr.szUrl+rlen, nlhrReply->headers[i].szValue); 
				*parUrl = nlhr.szUrl;
				goto retry;
			}
		}
		else 
		{
			const int len = nlhrReply->dataLength;
			result = (char*)mir_alloc(len+1);
			memcpy(result, nlhrReply->pData, len);
			result[len] = 0;
		}
	}

	CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,0,(LPARAM)nlhrReply);
	mir_free(nlhr.headers);

	return result;
}
