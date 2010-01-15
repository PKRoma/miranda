/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

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
#include "commonheaders.h"
#include "../plugins/zlib/zlib.h"
#include "netlib.h"

#define HTTPRECVHEADERSTIMEOUT   60000  //in ms

struct ResizableCharBuffer {
	char *sz;
	int iEnd,cbAlloced;
};

static void AppendToCharBuffer(struct ResizableCharBuffer *rcb,const char *fmt,...)
{
	va_list va;
	int charsDone;

	if(rcb->cbAlloced==0) {
		rcb->cbAlloced=512;
		rcb->sz=(char*)mir_alloc(rcb->cbAlloced);
	}
	va_start(va,fmt);
	for(;;) {
		charsDone=mir_vsnprintf(rcb->sz+rcb->iEnd,rcb->cbAlloced-rcb->iEnd,fmt,va);
		if(charsDone>=0) break;
		rcb->cbAlloced+=512;
		rcb->sz=(char*)mir_realloc(rcb->sz,rcb->cbAlloced);
	}
	va_end(va);
	rcb->iEnd+=charsDone;
}

static int RecvWithTimeoutTime(struct NetlibConnection *nlc, DWORD dwTimeoutTime, char *buf, int len ,int flags)
{
	DWORD dwTimeNow;

	if (!si.pending(nlc->hSsl))
	{
		while ((dwTimeNow = GetTickCount()) < dwTimeoutTime)
		{
			DWORD dwDeltaTime = min(dwTimeoutTime - dwTimeNow, 1000);
			int res = WaitUntilReadable(nlc->s, dwDeltaTime);

		    if (res > 0) break;
			else if (res == SOCKET_ERROR) return SOCKET_ERROR;
			if (Miranda_Terminated()) return 0;
		}
		if (dwTimeNow >= dwTimeoutTime)
		{
			SetLastError(ERROR_TIMEOUT);
			return SOCKET_ERROR;
		}
	}
	return NLRecv(nlc,buf,len,flags);
}

static char* NetlibHttpFindHeader(NETLIBHTTPREQUEST *nlhrReply, const char *hdr)
{
	for (int i = 0; i < nlhrReply->headersCount; i++) 
	{
		if (_stricmp(nlhrReply->headers[i].szName, hdr) == 0) 
		{
			return nlhrReply->headers[i].szValue;
		}
	}
	return NULL;
}

static bool NetlibHttpProcessUrl(NETLIBHTTPREQUEST *nlhr, NetlibUser *nlu, char* szUrl = NULL)
{
	NETLIBOPENCONNECTION nloc = {0};
	char szHost[1024];
	char *ppath, *phost, *pcolon;
	bool secur;

	if (szUrl == NULL) szUrl = nlhr->szUrl;

	secur = _strnicmp(szUrl, "https", 5) == 0 || (nlhr->flags & NLHRF_SSL);
	phost = strstr(szUrl, "://");
	if (phost == NULL) phost = szUrl;
	else phost += 3;
	lstrcpynA(szHost, phost, SIZEOF(szHost));
	ppath = strchr(szHost, '/');
	if (ppath) *ppath = '\0';

	nloc.cbSize = sizeof(nloc);
	nloc.szHost = szHost;
	pcolon = strrchr(szHost, ':');
	if (pcolon) 
	{
		*pcolon = '\0';
		nloc.wPort = (WORD)strtol(pcolon+1, NULL, 10);
	}
	else nloc.wPort = secur ? 443 : 80;
    if (secur) nlhr->flags |= NLHRF_SSL; else nlhr->flags &= ~NLHRF_SSL;
	nloc.flags = NLOCF_HTTP | (secur ? NLOCF_SSL : 0);

	NetlibConnection* nlc = (NetlibConnection*)nlhr->nlc;
	if (nlc != NULL)
	{
		NETLIBUSERSETTINGS *setgs = &nlc->nlu->settings;
		bool httpProxy = !secur && setgs->useProxy && setgs->proxyType == PROXYTYPE_HTTP && 
			setgs->szProxyServer && setgs->szProxyServer[0];

		bool sameHost = lstrcmpA(nlc->nloc.szHost, szHost) == 0 && nlc->nloc.wPort == nloc.wPort;

		if (!httpProxy && !sameHost)
		{
			if (nlc->hSsl)
			{
				si.shutdown(nlc->hSsl);
				si.sfree(nlc->hSsl);
				nlc->hSsl = NULL;
			}
			if (nlc->s != INVALID_SOCKET) closesocket(nlc->s);
			nlc->s = INVALID_SOCKET;

			mir_free((char*)nlc->nloc.szHost);
			nlc->nloc = nloc;
			nlc->nloc.szHost = mir_strdup(nloc.szHost);
			return NetlibDoConnect(nlc);
		}
	}
	else
		nlhr->nlc = nlc = (NetlibConnection*)NetlibOpenConnection((WPARAM)nlu, (LPARAM)&nloc);

	return nlc != NULL;
}

static int HttpPeekFirstResponseLine(struct NetlibConnection *nlc,DWORD dwTimeoutTime,DWORD recvFlags,int *resultCode,char **ppszResultDescr,int *length)
{
	int bytesPeeked=0;
	char buffer[1024];
	char *peol;

	for(;;) {
		bytesPeeked=RecvWithTimeoutTime(nlc,dwTimeoutTime,buffer,SIZEOF(buffer)-1,MSG_PEEK|recvFlags);
		if(bytesPeeked==0 || bytesPeeked==SOCKET_ERROR) {
			if(bytesPeeked==0) SetLastError(ERROR_HANDLE_EOF);
			return 0;
		}
		buffer[bytesPeeked]='\0';
		peol=strchr(buffer,'\n');
		if(peol==NULL) {
			if(lstrlenA(buffer)<bytesPeeked) {
				SetLastError(ERROR_BAD_FORMAT);
				return 0;
			}
			if(bytesPeeked==SIZEOF(buffer)-1) {
				SetLastError(ERROR_BUFFER_OVERFLOW);
				return 0;
			}
			Sleep(10);
			continue;
		}
		if(peol==buffer || *--peol!='\r') {
			SetLastError(ERROR_BAD_FORMAT);
			return 0;
		}
		*peol='\0';
		{
			char *pResultCode,*pResultDescr,*pHttpMajor,*pHttpMinor;
			size_t tokenLen;
			int httpMajorVer,httpMinorVer;
			if(peol==buffer
			   || _strnicmp(buffer,"http/",5)
			   || (httpMajorVer=strtol(pHttpMajor=buffer+5,&pHttpMinor,10))<0
			   || pHttpMajor==pHttpMinor
			   || httpMajorVer>1
			   || *pHttpMinor++!='.'
			   || (httpMinorVer=strtol(pHttpMinor,&pResultCode,10))<0
			   || pResultCode==pHttpMinor
			   || (tokenLen=strspn(pResultCode," \t\n\0"))==0	//by FYR: Some proxy may not return code description but mostly 'HTTP/1.0 200' is able to work result
			   || (*resultCode=strtol(pResultCode+=tokenLen,&pResultDescr,10))==0
			// || pResultDescr==pResultCode
			// || (tokenLen=strspn(pResultDescr," \t"))==0
			// || *(pResultDescr+=tokenLen)=='\0' 
			   ) {
				SetLastError(peol==buffer?ERROR_BAD_FORMAT:ERROR_INVALID_DATA);
				return 0;
			}
			if(ppszResultDescr) *ppszResultDescr=mir_strdup(pResultDescr);
			if(length) *length=peol-buffer+2;
		}
		return 1;
	}
}

static int SendHttpRequestAndData(struct NetlibConnection *nlc,struct ResizableCharBuffer *httpRequest,NETLIBHTTPREQUEST *nlhr,int sendContentLengthHeader)
{
	bool sendData = (nlhr->requestType==REQUEST_POST || nlhr->requestType==REQUEST_PUT);

	if(sendContentLengthHeader && sendData)
		AppendToCharBuffer(httpRequest,"Content-Length: %d\r\n\r\n", nlhr->dataLength);
	else
		AppendToCharBuffer(httpRequest,"\r\n");

	DWORD hflags = (nlhr->flags & NLHRF_DUMPASTEXT ? MSG_DUMPASTEXT : 0) | 
		(nlhr->flags & (NLHRF_NODUMP | NLHRF_NODUMPSEND | NLHRF_NODUMPHEADERS) ? 
			MSG_NODUMP : (nlhr->flags & NLHRF_DUMPPROXY ? MSG_DUMPPROXY : 0)) |
		(nlhr->flags & NLHRF_NOPROXY ? MSG_RAW : 0);

	int bytesSent=NLSend(nlc,httpRequest->sz,httpRequest->iEnd, hflags);
	if (bytesSent != SOCKET_ERROR && sendData && nlhr->dataLength) 
	{
		DWORD sflags = (nlhr->flags & NLHRF_DUMPASTEXT ? MSG_DUMPASTEXT : 0) | 
			(nlhr->flags & (NLHRF_NODUMP | NLHRF_NODUMPSEND) ? 
				MSG_NODUMP : (nlhr->flags & NLHRF_DUMPPROXY ? MSG_DUMPPROXY : 0)) |
			(nlhr->flags & NLHRF_NOPROXY ? MSG_RAW : 0);

		int sendResult = NLSend(nlc,nlhr->pData,nlhr->dataLength, sflags);

		bytesSent = sendResult != SOCKET_ERROR ? bytesSent + sendResult : SOCKET_ERROR;
	}
	mir_free(httpRequest->sz);
	memset(httpRequest, 0, sizeof(*httpRequest));

	return bytesSent;
}

INT_PTR NetlibHttpSendRequest(WPARAM wParam,LPARAM lParam)
{
	struct NetlibConnection *nlc=(struct NetlibConnection*)wParam;
	NETLIBHTTPREQUEST *nlhr=(NETLIBHTTPREQUEST*)lParam;
	NETLIBHTTPREQUEST *nlhrReply = NULL;

	struct ResizableCharBuffer httpRequest={0};
	char *pszRequest, *szHost, *pszUrl;
	char *pszProxyAuthorizationHeader = NULL;
	int i,doneHostHeader,doneContentLengthHeader,doneProxyAuthHeader;
	int bytesSent;

	if(nlhr==NULL || nlhr->cbSize!=sizeof(NETLIBHTTPREQUEST) || nlhr->szUrl==NULL || nlhr->szUrl[0]=='\0') 
    {
		SetLastError(ERROR_INVALID_PARAMETER);
		return SOCKET_ERROR;
	}
	switch(nlhr->requestType) 
    {
		case REQUEST_GET:       pszRequest="GET";       break;
		case REQUEST_POST:      pszRequest="POST";      break;
		case REQUEST_CONNECT:   pszRequest="CONNECT";   break;
		case REQUEST_HEAD:      pszRequest="HEAD";      break;
        case REQUEST_PUT:       pszRequest="PUT";       break;
        case REQUEST_DELETE:    pszRequest="DELETE";    break;
		default:
			SetLastError(ERROR_INVALID_PARAMETER);
			return SOCKET_ERROR;
	}

	if (nlc->nlhpi.szHttpGetUrl == NULL)
	{
		if(!NetlibEnterNestedCS(nlc,NLNCS_SEND)) return SOCKET_ERROR;
	}

	//first line: "GET /index.html HTTP/1.0\r\n"
	szHost = NULL;
	pszUrl = nlhr->szUrl;

	HANDLE hNtlmSecurity = NULL;
	unsigned complete = false;
	for (int count = 6; --count; )
	{
		if (nlhr->flags & (NLHRF_SMARTREMOVEHOST | NLHRF_REMOVEHOST | NLHRF_GENERATEHOST))
		{
			mir_free(szHost); szHost = NULL;

			char *ppath, *phost;
			phost = strstr(pszUrl, "://");
			if (phost == NULL) phost = pszUrl;
			else phost += 3;
			ppath = strchr(phost, '/');
			if (ppath == NULL) ppath = phost + lstrlenA(phost);
			if (nlhr->flags & NLHRF_GENERATEHOST) 
			{
				int hostLen = ppath - phost + 1;
				szHost = (char*)mir_alloc(hostLen);
				lstrcpynA(szHost, phost, hostLen);
			}

			NETLIBUSERSETTINGS *setgs = &nlc->nlu->settings;
			if (nlhr->flags & NLHRF_REMOVEHOST || (nlhr->flags & NLHRF_SMARTREMOVEHOST &&
				!(setgs->useProxy && (setgs->proxyType == PROXYTYPE_HTTP || setgs->proxyType == PROXYTYPE_HTTPS)))) 
			{
			   pszUrl = ppath[0] ? ppath : "/";
			}
		}

		if (!NetlibReconnect(nlc)) { bytesSent = SOCKET_ERROR; break; }
		AppendToCharBuffer(&httpRequest, "%s %s HTTP/1.%d\r\n", pszRequest, pszUrl, (nlhr->flags & NLHRF_HTTP11) != 0);

		//HTTP headers
		doneHostHeader=doneContentLengthHeader=doneProxyAuthHeader=0;
		for(i=0;i<nlhr->headersCount;i++) {
			if(!lstrcmpiA(nlhr->headers[i].szName,"Host")) doneHostHeader=1;
			else if(!lstrcmpiA(nlhr->headers[i].szName,"Content-Length")) doneContentLengthHeader=1;
			else if(!lstrcmpiA(nlhr->headers[i].szName,"Proxy-Authorization")) doneProxyAuthHeader=1;
			else if(!lstrcmpiA(nlhr->headers[i].szName,"Connection")) continue;
			if(nlhr->headers[i].szValue==NULL) continue;
			AppendToCharBuffer(&httpRequest,"%s: %s\r\n",nlhr->headers[i].szName,nlhr->headers[i].szValue);
		}
		if(szHost && !doneHostHeader) AppendToCharBuffer(&httpRequest,"%s: %s\r\n","Host",szHost);
		if(pszProxyAuthorizationHeader) {
			if(!doneProxyAuthHeader) AppendToCharBuffer(&httpRequest,"%s: %s\r\n","Proxy-Authorization",pszProxyAuthorizationHeader);
		}
		AppendToCharBuffer(&httpRequest,"%s: %s\r\n","Connection","Keep-Alive");
		AppendToCharBuffer(&httpRequest,"%s: %s\r\n","Proxy-Connection","Keep-Alive");

		// Add Sticky Headers
		if (nlc->nlu->szStickyHeaders != NULL) {
			AppendToCharBuffer(&httpRequest,"%s\r\n", nlc->nlu->szStickyHeaders);
		}

		//send it
		bytesSent=SendHttpRequestAndData(nlc,&httpRequest,nlhr,!doneContentLengthHeader);
		if (bytesSent == SOCKET_ERROR) break;

		//ntlm reply
		{
			int resultCode = 0;

			DWORD fflags = MSG_PEEK | MSG_NODUMP | ((nlhr->flags & NLHRF_NOPROXY) ? MSG_RAW : 0);
			DWORD dwTimeOutTime = nlc->usingHttpGateway && nlhr->requestType == REQUEST_GET ? -1 : GetTickCount() + HTTPRECVHEADERSTIMEOUT;
			if(!HttpPeekFirstResponseLine(nlc, dwTimeOutTime, fflags, &resultCode, NULL, NULL))
			{
				if (NetlibReconnect(nlc)) continue;

				if (resultCode) NetlibHttpSetLastErrorUsingHttpResult(resultCode);
				bytesSent = SOCKET_ERROR;
				break;
			}

			DWORD hflags = (nlhr->flags & (NLHRF_NODUMP|NLHRF_NODUMPHEADERS|NLHRF_NODUMPSEND) ? 
				MSG_NODUMP : (nlhr->flags & NLHRF_DUMPPROXY ? MSG_DUMPPROXY : 0)) |
				(nlhr->flags & NLHRF_NOPROXY ? MSG_RAW : 0);

			DWORD dflags = (nlhr->flags & (NLHRF_NODUMP | NLHRF_NODUMPSEND) ? 
				MSG_NODUMP : MSG_DUMPASTEXT | MSG_DUMPPROXY) |
				(nlhr->flags & NLHRF_NOPROXY ? MSG_RAW : 0);

			if (resultCode == 100)
			{
				nlhrReply = (NETLIBHTTPREQUEST*)NetlibHttpRecvHeaders((WPARAM)nlc, hflags);
			}
			else if ((resultCode == 301 || resultCode == 302 || resultCode == 307) // redirect
				&& (nlhr->flags & NLHRF_REDIRECT))
			{
				pszUrl = NULL;

				if (nlhr->requestType == REQUEST_HEAD)
					nlhrReply = (NETLIBHTTPREQUEST*)NetlibHttpRecvHeaders((WPARAM)nlc, hflags);
				else
					nlhrReply = NetlibHttpRecv(nlc, hflags, dflags);

				if (nlhrReply) 
				{
					char* tmpUrl = NetlibHttpFindHeader(nlhrReply, "Location");
					if (tmpUrl)
					{
						size_t rlen = 0;
						if (tmpUrl[0] == '/')
						{
							char* szPath;
							char* szPref = strstr(pszUrl, "://");
							szPref = szPref ? szPref + 3 : pszUrl;
							szPath = strchr(szPref, '/');
							rlen = szPath != NULL ? szPath - pszUrl : strlen(pszUrl); 
						}

						nlc->szNewUrl = (char*)mir_realloc(nlc->szNewUrl, rlen + strlen(tmpUrl) * 3 + 1);

						strncpy(nlc->szNewUrl, pszUrl, rlen);
						strcpy(nlc->szNewUrl + rlen, tmpUrl); 
						pszUrl = nlc->szNewUrl;

						if (!NetlibHttpProcessUrl(nlhr, nlc->nlu, pszUrl))
						{
							bytesSent = SOCKET_ERROR;
							break;
						}
						nlc = (NetlibConnection*)nlhr->nlc;
					}
					else
					{
						NetlibHttpSetLastErrorUsingHttpResult(resultCode);
						bytesSent = SOCKET_ERROR;
						break;
					}
				}
				else
				{
					NetlibHttpSetLastErrorUsingHttpResult(resultCode);
					bytesSent = SOCKET_ERROR;
					break;
				}
			}
			else if (resultCode == 407) 	//proxy auth required
			{
				int error;

				if (nlhr->requestType == REQUEST_HEAD)
					nlhrReply = (NETLIBHTTPREQUEST*)NetlibHttpRecvHeaders((WPARAM)nlc, hflags);
				else
					nlhrReply = NetlibHttpRecv(nlc, hflags, dflags);

				if (nlhrReply == NULL || complete) 
				{
					NetlibHttpSetLastErrorUsingHttpResult(resultCode);
					bytesSent = SOCKET_ERROR;
					break;
				}

				error = ERROR_SUCCESS;
				char *szAuthStr = NetlibHttpFindHeader(nlhrReply, "Proxy-Authenticate");
				if (szAuthStr)
				{
					char *szChallenge = strchr(szAuthStr, ' '); 
					if (szChallenge) { *szChallenge = 0; ++szChallenge; }

					if (hNtlmSecurity == NULL)
					{
						TCHAR *szAuthMethod = mir_a2t(szAuthStr);	
						hNtlmSecurity = NetlibInitSecurityProvider(szAuthMethod, NULL);
						mir_free(szAuthMethod);
					}

					if (hNtlmSecurity)
					{
						mir_free(pszProxyAuthorizationHeader);

						TCHAR *szLogin = NULL, *szPassw = NULL;

						if (nlc->nlu->settings.useProxyAuth)
						{
							szLogin = mir_a2t(nlc->nlu->settings.szProxyAuthUser);
							szPassw = mir_a2t(nlc->nlu->settings.szProxyAuthPassword);
						}

						pszProxyAuthorizationHeader = NtlmCreateResponseFromChallenge(hNtlmSecurity, 
							szChallenge, szLogin, szPassw, true, complete);

						mir_free(szLogin);
						mir_free(szPassw);
					}
					else
						error = ERROR_ACCESS_DENIED;
				}
				if (pszProxyAuthorizationHeader == NULL) 
				{
					if (error != ERROR_SUCCESS) SetLastError(error);
					bytesSent = SOCKET_ERROR;
					break;
				}
			}
			else
				break;
		
			if (nlhrReply)
			{
				NetlibHttpFreeRequestStruct(0, (LPARAM)nlhrReply);
				nlhrReply = NULL;
			}
		}
	}
	if (nlhrReply) NetlibHttpFreeRequestStruct(0, (LPARAM)nlhrReply);

	//clean up
	if (hNtlmSecurity) NetlibDestroySecurityProvider(hNtlmSecurity);
	mir_free(pszProxyAuthorizationHeader);
	mir_free(szHost);
	if (nlc->nlhpi.szHttpGetUrl == NULL)
	{
		NetlibLeaveNestedCS(&nlc->ncsSend);
	}
	return bytesSent;
}

INT_PTR NetlibHttpFreeRequestStruct(WPARAM, LPARAM lParam)
{
	NETLIBHTTPREQUEST *nlhr=(NETLIBHTTPREQUEST*)lParam;

	if(nlhr==NULL || nlhr->cbSize!=sizeof(NETLIBHTTPREQUEST) || nlhr->requestType!=REQUEST_RESPONSE) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}
	if(nlhr->headers) {
		int i;
		for(i=0;i<nlhr->headersCount;i++) {
			if(nlhr->headers[i].szName) mir_free(nlhr->headers[i].szName);
			if(nlhr->headers[i].szValue) mir_free(nlhr->headers[i].szValue);
		}
		mir_free(nlhr->headers);
	}
	if(nlhr->pData) mir_free(nlhr->pData);
	if(nlhr->szResultDescr) mir_free(nlhr->szResultDescr);
	if(nlhr->szUrl) mir_free(nlhr->szUrl);
	mir_free(nlhr);
	return 1;
}

INT_PTR NetlibHttpRecvHeaders(WPARAM wParam,LPARAM lParam)
{
	struct NetlibConnection *nlc=(struct NetlibConnection*)wParam;
	NETLIBHTTPREQUEST *nlhr;
	char buffer[8192];
	int bytesPeeked;
	DWORD dwRequestTimeoutTime;
	char *peol,*pbuffer;
	int headersDone=0,firstLineLength;

	if(!NetlibEnterNestedCS(nlc,NLNCS_RECV))
		return (INT_PTR)(NETLIBHTTPREQUEST*)NULL;
	dwRequestTimeoutTime=GetTickCount()+HTTPRECVHEADERSTIMEOUT;
	nlhr=(NETLIBHTTPREQUEST*)mir_calloc(sizeof(NETLIBHTTPREQUEST));
	nlhr->cbSize=sizeof(NETLIBHTTPREQUEST);
	nlhr->nlc=nlc;
	nlhr->requestType=REQUEST_RESPONSE;
	if(!HttpPeekFirstResponseLine(nlc,dwRequestTimeoutTime,lParam|MSG_PEEK,&nlhr->resultCode,&nlhr->szResultDescr,&firstLineLength)) {
		NetlibLeaveNestedCS(&nlc->ncsRecv);
		NetlibHttpFreeRequestStruct(0,(LPARAM)nlhr);
		return 0;
	}
	bytesPeeked=NLRecv(nlc,buffer,firstLineLength,lParam|MSG_DUMPASTEXT);
	if(bytesPeeked<firstLineLength) {
		NetlibLeaveNestedCS(&nlc->ncsRecv);
		NetlibHttpFreeRequestStruct(0,(LPARAM)nlhr);
		if(bytesPeeked!=SOCKET_ERROR) SetLastError(ERROR_HANDLE_EOF);
		return 0;
	}
	for(;;) {
		bytesPeeked=RecvWithTimeoutTime(nlc,dwRequestTimeoutTime,buffer,SIZEOF(buffer)-1,MSG_PEEK|lParam);
        if (bytesPeeked==0) break;

		if(bytesPeeked==SOCKET_ERROR) {
			NetlibLeaveNestedCS(&nlc->ncsRecv);
			NetlibHttpFreeRequestStruct(0,(LPARAM)nlhr);
			if(bytesPeeked==0) SetLastError(ERROR_HANDLE_EOF);
			return 0;
		}
		buffer[bytesPeeked]='\0';
		for(pbuffer=buffer;;) {
			peol=strchr(pbuffer,'\n');
			if(peol==NULL) {
                if((bytesPeeked == SIZEOF(buffer)-1 && pbuffer==buffer)	//buffer overflow
				   || (pbuffer!=buffer && NLRecv(nlc,buffer,pbuffer-buffer,lParam|MSG_DUMPASTEXT)==SOCKET_ERROR)) {	 //error removing read bytes from buffer
					NetlibLeaveNestedCS(&nlc->ncsRecv);
					NetlibHttpFreeRequestStruct(0,(LPARAM)nlhr);
					if(pbuffer==buffer) SetLastError(ERROR_BUFFER_OVERFLOW);
					return (INT_PTR)(NETLIBHTTPREQUEST*)NULL;
				}
				Sleep(100);
				break;
			}
			if(peol==pbuffer || *--peol!='\r') {
				NetlibLeaveNestedCS(&nlc->ncsRecv);
				NetlibHttpFreeRequestStruct(0,(LPARAM)nlhr);
				SetLastError(ERROR_BAD_FORMAT);
				return 0;
			}
			*peol='\0';
			{
				char *pColon;
				int len;
				if(peol==pbuffer) {	   //blank line: end of headers
					if(NLRecv(nlc,buffer,peol+2-buffer,lParam|MSG_DUMPASTEXT)==SOCKET_ERROR) {
						NetlibLeaveNestedCS(&nlc->ncsRecv);
						NetlibHttpFreeRequestStruct(0,(LPARAM)nlhr);
						return (INT_PTR)(NETLIBHTTPREQUEST*)NULL;
					}
					headersDone=1;
					break;
				}
				pColon=strchr(pbuffer,':');
				if(pColon==NULL) {
					NetlibLeaveNestedCS(&nlc->ncsRecv);
					NetlibHttpFreeRequestStruct(0,(LPARAM)nlhr);
					SetLastError(ERROR_INVALID_DATA);
					return (INT_PTR)(NETLIBHTTPREQUEST*)NULL;
				}
				nlhr->headersCount++;
				nlhr->headers=(NETLIBHTTPHEADER*)mir_realloc(nlhr->headers,sizeof(NETLIBHTTPHEADER)*nlhr->headersCount);
				nlhr->headers[nlhr->headersCount-1].szName=(char*)mir_alloc(pColon-pbuffer+1);
				lstrcpynA(nlhr->headers[nlhr->headersCount-1].szName,pbuffer,pColon-pbuffer+1);
				len=lstrlenA(nlhr->headers[nlhr->headersCount-1].szName);
				while(len && (nlhr->headers[nlhr->headersCount-1].szName[len-1]==' ' || nlhr->headers[nlhr->headersCount-1].szName[len-1]=='\t'))
					nlhr->headers[nlhr->headersCount-1].szName[--len]='\0';
				pColon++;
				while(*pColon==' ' || *pColon=='\t') pColon++;
				nlhr->headers[nlhr->headersCount-1].szValue=mir_strdup(pColon);
			}
			pbuffer=peol+2;
		}
		if(headersDone) break;
	}
	NetlibLeaveNestedCS(&nlc->ncsRecv);
	return (INT_PTR)nlhr;
}

INT_PTR NetlibHttpTransaction(WPARAM wParam, LPARAM lParam)
{
	NetlibUser *nlu =  (NetlibUser*)wParam;
	NETLIBHTTPREQUEST *nlhr = (NETLIBHTTPREQUEST*)lParam, *nlhrReply;
	DWORD dflags;

	if (GetNetlibHandleType(nlu) != NLH_USER || !(nlu->user.flags & NUF_OUTGOING) || 
		nlhr == NULL || nlhr->cbSize != sizeof(NETLIBHTTPREQUEST) || 
		nlhr->szUrl == NULL || nlhr->szUrl[0] == 0) 
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	if (!NetlibHttpProcessUrl(nlhr, nlu)) return 0;
	HANDLE hConnection = nlhr->nlc;

	{
		NETLIBHTTPREQUEST nlhrSend;
		char szUserAgent[64];

		nlhrSend = *nlhr;
		nlhrSend.flags &= ~NLHRF_REMOVEHOST;
		nlhrSend.flags |= NLHRF_GENERATEHOST | NLHRF_SMARTREMOVEHOST | NLHRF_SMARTAUTHHEADER;

		bool doneUserAgentHeader = NetlibHttpFindHeader(nlhr, "User-Agent") != NULL;
		bool doneAcceptEncoding = NetlibHttpFindHeader(nlhr, "Accept-Encoding") != NULL;

		if(!doneUserAgentHeader||!doneAcceptEncoding) {
			nlhrSend.headers=(NETLIBHTTPHEADER*)mir_alloc(sizeof(NETLIBHTTPHEADER)*(nlhrSend.headersCount+2));
			memcpy(nlhrSend.headers,nlhr->headers,sizeof(NETLIBHTTPHEADER)*nlhr->headersCount);
        }
		if (!doneUserAgentHeader) 
		{
			char *pspace,szMirandaVer[64];

			nlhrSend.headers[nlhrSend.headersCount].szName="User-Agent";
			nlhrSend.headers[nlhrSend.headersCount].szValue=szUserAgent;
			++nlhrSend.headersCount;
			CallService(MS_SYSTEM_GETVERSIONTEXT,SIZEOF(szMirandaVer),(LPARAM)szMirandaVer);
			pspace=strchr(szMirandaVer,' ');
			if(pspace) 
			{
				*pspace++ = '\0';
				mir_snprintf(szUserAgent, SIZEOF(szUserAgent), "Miranda/%s (%s)", szMirandaVer, pspace);
			}
			else 
				mir_snprintf(szUserAgent, SIZEOF(szUserAgent), "Miranda/%s", szMirandaVer);
		}
        if (!doneAcceptEncoding) {
			nlhrSend.headers[nlhrSend.headersCount].szName="Accept-Encoding";
			nlhrSend.headers[nlhrSend.headersCount].szValue="deflate, gzip";
			++nlhrSend.headersCount;
        }
		if(NetlibHttpSendRequest((WPARAM)hConnection, (LPARAM)&nlhrSend) == SOCKET_ERROR) 
		{
			if(!doneUserAgentHeader||!doneAcceptEncoding) mir_free(nlhrSend.headers);
			NetlibCloseHandle((WPARAM)hConnection, 0);
			return 0;
		}
		if (!doneUserAgentHeader || !doneAcceptEncoding) mir_free(nlhrSend.headers);
	}

	dflags = (nlhr->flags & NLHRF_DUMPASTEXT ? MSG_DUMPASTEXT:0) |
		(nlhr->flags & NLHRF_NODUMP?MSG_NODUMP : (nlhr->flags & NLHRF_DUMPPROXY ? MSG_DUMPPROXY : 0)) |
		(nlhr->flags & NLHRF_NOPROXY ? MSG_RAW : 0);

	if (nlhr->requestType == REQUEST_HEAD)
		nlhrReply = (NETLIBHTTPREQUEST*)NetlibHttpRecvHeaders((WPARAM)hConnection, 0);
	else
		nlhrReply = NetlibHttpRecv((NetlibConnection*)hConnection, 0, dflags);

	if (nlhrReply)
	{
		nlhrReply->szUrl = ((NetlibConnection*)hConnection)->szNewUrl;
		((NetlibConnection*)hConnection)->szNewUrl = NULL;
	}

	if ((nlhr->flags & NLHRF_PERSISTENT) == 0 || nlhrReply == NULL)
	    NetlibCloseHandle((WPARAM)hConnection, 0);
	else
		nlhrReply->nlc = hConnection;

	return (INT_PTR)nlhrReply;
}

void NetlibHttpSetLastErrorUsingHttpResult(int result)
{
	if(result>=200 && result<300) {
		SetLastError(ERROR_SUCCESS);
		return;
	}
	switch(result) {
		case 400: SetLastError(ERROR_BAD_FORMAT); break;
		case 401:
		case 402:
		case 403:
		case 407: SetLastError(ERROR_ACCESS_DENIED); break;
		case 404: SetLastError(ERROR_FILE_NOT_FOUND); break;
		case 405:
		case 406: SetLastError(ERROR_INVALID_FUNCTION); break;
		case 408: SetLastError(ERROR_TIMEOUT); break;
		default: SetLastError(ERROR_GEN_FAILURE); break;
	}
}

char* gzip_decode(char *gzip_data, int *len_ptr, int window)
{
    int gzip_len = *len_ptr * 5;
    char* output_data = NULL;

    int gzip_err;
    z_stream zstr;

    do
    {
        output_data = (char*)mir_realloc(output_data, gzip_len+1);

        zstr.next_in = (Bytef*)gzip_data;
        zstr.avail_in = *len_ptr;
        zstr.zalloc = Z_NULL;
        zstr.zfree = Z_NULL;
        zstr.opaque = Z_NULL;
        inflateInit2_(&zstr, window, ZLIB_VERSION, sizeof(z_stream));

        zstr.next_out = (Bytef*)output_data;
        zstr.avail_out = gzip_len;

        gzip_err = inflate(&zstr, Z_FINISH);

        inflateEnd(&zstr);
        gzip_len *= 2;
    }
    while (gzip_err == Z_BUF_ERROR);
    
    gzip_len = gzip_err == Z_STREAM_END ? zstr.total_out : -1;
    
    if (gzip_len <= 0)
    {
        mir_free(output_data);
        output_data = NULL;
    }
    else
        output_data[gzip_len] = 0;

    *len_ptr = gzip_len;
    return output_data;
}

static int NetlibHttpRecvChunkHeader(NetlibConnection* nlc, BOOL first)
{
	char data[64], *peol1;

	for (;;)
	{
		int recvResult = NLRecv(nlc, data, 31, MSG_PEEK);
		if (recvResult <= 0) return SOCKET_ERROR;

		data[recvResult] = 0;

		peol1 = strstr(data, "\r\n");
		if (peol1 != NULL)
		{
			char *peol2 = first ? peol1 : strstr(peol1 + 2, "\r\n");
			if (peol2 != NULL)
			{
				int sz = peol2 - data + 2;
				int r = strtol(first ? data : peol1 + 2, NULL, 16);
				if (r == 0)
				{
					char *peol3 = strstr(peol2 + 2, "\r\n");
					if (peol3 == NULL) continue;
					sz = peol3 - data + 2;
				}
				NLRecv(nlc, data, sz, 0);
				return r;
			}
			else
				if (recvResult >= 31) return SOCKET_ERROR;
		}
	}
}

NETLIBHTTPREQUEST* NetlibHttpRecv(NetlibConnection* nlc, DWORD hflags, DWORD dflags)
{
	int dataLen = -1, i, chunkhdr = 0;
	bool chunked = false;
	int cenc = 0, cenctype = 0;

//	DWORD dwCompleteTime = GetTickCount() + HTTPGETTIMEOUT;

next:
	NETLIBHTTPREQUEST *nlhrReply = (NETLIBHTTPREQUEST*)NetlibHttpRecvHeaders((WPARAM)nlc, hflags);
	if (nlhrReply == NULL) 
		return NULL;

	if (nlhrReply->resultCode == 100)
	{
		NetlibHttpFreeRequestStruct(0, (LPARAM)nlhrReply);
		goto next;
	} 

	for(i=0; i<nlhrReply->headersCount; i++) 
	{
		if(!lstrcmpiA(nlhrReply->headers[i].szName, "Content-Length")) 
			dataLen = atoi(nlhrReply->headers[i].szValue);

		if(!lstrcmpiA(nlhrReply->headers[i].szName, "Content-Encoding")) 
		{
			cenc = i;
			if (strstr(nlhrReply->headers[i].szValue, "gzip"))
				cenctype = 1;
			else if (strstr(nlhrReply->headers[i].szValue, "deflate"))
				cenctype = 2;
		}

		if(!lstrcmpiA(nlhrReply->headers[i].szName, "Transfer-Encoding") && 
			!lstrcmpiA(nlhrReply->headers[i].szValue, "chunked"))
		{
			chunked = true;
			chunkhdr = i;
		}
	}

	if (nlhrReply->resultCode >= 200 && dataLen != 0)
	{
		int recvResult, chunksz = 0;
		int dataBufferAlloced;

		if (chunked)
		{
			chunksz = NetlibHttpRecvChunkHeader(nlc, TRUE);
			if (chunksz == SOCKET_ERROR) 
			{
				NetlibHttpFreeRequestStruct(0, (LPARAM)nlhrReply);
				return NULL;
			}
			dataLen = chunksz;
		}
		dataBufferAlloced = dataLen < 0 ? 2048 : dataLen + 1;
		nlhrReply->pData = (char*)mir_realloc(nlhrReply->pData, dataBufferAlloced);

		do {
			for(;;) 
			{
/*
				unsigned dwTimeNow = GetTickCount();
				if (dwTimeNow > dwCompleteTime || (!si.pending(nlc->hSsl) && 
					WaitUntilReadable(nlc->s, dwCompleteTime - dwTimeNow) <= 0)) 
					return NULL;
*/
				recvResult = NLRecv(nlc, nlhrReply->pData+nlhrReply->dataLength,
					dataBufferAlloced-nlhrReply->dataLength-1, dflags | (cenctype ? MSG_NODUMP : 0));

				if (recvResult == 0) break;
				if (recvResult == SOCKET_ERROR) {
					NetlibHttpFreeRequestStruct(0,(LPARAM)nlhrReply);
					return NULL;
				}
				nlhrReply->dataLength += recvResult;

				if (dataLen >= 0)
				{
					if (nlhrReply->dataLength >= dataLen) break;
				}
				else
				{
					if ((dataBufferAlloced - nlhrReply->dataLength) < 256) 
					{
						dataBufferAlloced += 2048;
						nlhrReply->pData = (char*)mir_realloc(nlhrReply->pData, dataBufferAlloced);
						if(nlhrReply->pData == NULL) 
						{
							SetLastError(ERROR_OUTOFMEMORY);
							NetlibHttpFreeRequestStruct(0, (LPARAM)nlhrReply);
							return NULL;
						}
					}
				}
			}

			if (chunked)
			{
				chunksz = NetlibHttpRecvChunkHeader(nlc, FALSE);
				if (chunksz == SOCKET_ERROR) 
				{
					NetlibHttpFreeRequestStruct(0, (LPARAM)nlhrReply);
					return NULL;
				}
				dataLen += chunksz;
				dataBufferAlloced += chunksz;

				nlhrReply->pData = (char*)mir_realloc(nlhrReply->pData, dataBufferAlloced);
			}
		} while (chunksz != 0);

		nlhrReply->pData[nlhrReply->dataLength]='\0';
	}

	if (chunked)
	{
		nlhrReply->headers[chunkhdr].szName = ( char* )mir_realloc(nlhrReply->headers[chunkhdr].szName, 16);
		lstrcpyA(nlhrReply->headers[chunkhdr].szName, "Content-Length");

		nlhrReply->headers[chunkhdr].szValue = ( char* )mir_realloc(nlhrReply->headers[chunkhdr].szValue, 16);
		mir_snprintf(nlhrReply->headers[chunkhdr].szValue, 16, "%u", nlhrReply->dataLength);
	}

	if (cenctype)
	{
		int bufsz = nlhrReply->dataLength;
		char* szData = NULL;

		switch (cenctype)
		{
        case 1:
			szData = gzip_decode(nlhrReply->pData, &bufsz, 0x10 | MAX_WBITS);
            break;

        case 2:
			szData = gzip_decode(nlhrReply->pData, &bufsz, -MAX_WBITS);
			if (bufsz < 0)
			{
				bufsz = nlhrReply->dataLength;
				szData = gzip_decode(nlhrReply->pData, &bufsz, MAX_WBITS);
			}
            break;
		}

		if (bufsz > 0)
		{
			NetlibDumpData(nlc, (PBYTE)szData, bufsz, 0, dflags);
			mir_free(nlhrReply->pData);
			nlhrReply->pData = szData;
			nlhrReply->dataLength = bufsz;

			mir_free(nlhrReply->headers[cenc].szName);
			mir_free(nlhrReply->headers[cenc].szValue);
            memmove(&nlhrReply->headers[cenc], &nlhrReply->headers[cenc+1], (--nlhrReply->headersCount-cenc)*sizeof(nlhrReply->headers[0]));
		}
        else if (bufsz == 0)
        {
			mir_free(nlhrReply->pData);
			nlhrReply->pData = NULL;
			nlhrReply->dataLength = 0; 
        }
	}

	return nlhrReply;
}
