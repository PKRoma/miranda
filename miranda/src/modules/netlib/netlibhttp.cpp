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

static int RecvWithTimeoutTime(struct NetlibConnection *nlc,DWORD dwTimeoutTime,char *buf,int len,int flags)
{
	DWORD dwTimeNow;

	dwTimeNow=GetTickCount();
	if(dwTimeNow>=dwTimeoutTime
	   || (!si.pending(nlc->hSsl) && !WaitUntilReadable(nlc->s,dwTimeoutTime-dwTimeNow))) 
	{
		if(dwTimeNow>=dwTimeoutTime) SetLastError(ERROR_TIMEOUT);
		return SOCKET_ERROR;
	}
	return NLRecv(nlc,buf,len,flags);
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
	int bytesSent;

	if((nlhr->requestType==REQUEST_POST)) {
		if(sendContentLengthHeader)
			AppendToCharBuffer(httpRequest,"Content-Length: %d\r\n\r\n",nlhr->dataLength);
		else
			AppendToCharBuffer(httpRequest,"\r\n");
		bytesSent=NLSend(nlc,httpRequest->sz,httpRequest->iEnd,MSG_DUMPASTEXT|(nlhr->flags&(NLHRF_NODUMP|NLHRF_NODUMPHEADERS|NLHRF_NODUMPSEND)?MSG_NODUMP:(nlhr->flags&NLHRF_DUMPPROXY?MSG_DUMPPROXY:0)));
		mir_free(httpRequest->sz);
		if (nlhr->dataLength) {
			int sendResult;

			if(bytesSent==SOCKET_ERROR
			|| SOCKET_ERROR==(sendResult=NLSend(nlc,nlhr->pData,nlhr->dataLength,(nlhr->flags&NLHRF_DUMPASTEXT?MSG_DUMPASTEXT:0)|(nlhr->flags&(NLHRF_NODUMP|NLHRF_NODUMPSEND)?MSG_NODUMP:(nlhr->flags&NLHRF_DUMPPROXY?MSG_DUMPPROXY:0))))) {
			return SOCKET_ERROR;
			}
			bytesSent+=sendResult;
		}
	}
	else {
		AppendToCharBuffer(httpRequest,"\r\n");
		bytesSent=NLSend(nlc,httpRequest->sz,httpRequest->iEnd,MSG_DUMPASTEXT|(nlhr->flags&(NLHRF_NODUMP|NLHRF_NODUMPHEADERS|NLHRF_NODUMPSEND)?MSG_NODUMP:(nlhr->flags&NLHRF_DUMPPROXY?MSG_DUMPPROXY:0)));
		mir_free(httpRequest->sz);
	}
	return bytesSent;
}

INT_PTR NetlibHttpSendRequest(WPARAM wParam,LPARAM lParam)
{
	struct NetlibConnection *nlc=(struct NetlibConnection*)wParam;
	NETLIBHTTPREQUEST *nlhr=(NETLIBHTTPREQUEST*)lParam;
	struct ResizableCharBuffer httpRequest={0};
	char *pszRequest,*szHost,*pszUrl;
	char *pszProxyAuthorizationHeader;
	int i,doneHostHeader,doneContentLengthHeader,doneProxyAuthHeader,doneConnectionHeader;
	int useProxyHttpAuth,usingNtlmAuthentication,bytesSent;

	if(nlhr==NULL || nlhr->cbSize!=sizeof(NETLIBHTTPREQUEST) || nlhr->szUrl==NULL || nlhr->szUrl[0]=='\0') {
		SetLastError(ERROR_INVALID_PARAMETER);
		return SOCKET_ERROR;
	}
	switch(nlhr->requestType) {
		case REQUEST_GET: pszRequest="GET"; break;
		case REQUEST_POST: pszRequest="POST"; break;
		case REQUEST_CONNECT: pszRequest="CONNECT"; break;
		case REQUEST_HEAD:pszRequest="HEAD"; break;
		default:
			SetLastError(ERROR_INVALID_PARAMETER);
			return SOCKET_ERROR;
	}

	if(!NetlibEnterNestedCS(nlc,NLNCS_SEND)) return SOCKET_ERROR;

	//first line: "GET /index.html HTTP/1.0\r\n"
	szHost=NULL;
	if(nlhr->flags&(NLHRF_SMARTREMOVEHOST|NLHRF_REMOVEHOST|NLHRF_GENERATEHOST)){
		char *ppath,*phost;
		phost=strstr(nlhr->szUrl,"://");
		if(phost==NULL) phost=nlhr->szUrl;
		else phost+=3;
		ppath=strchr(phost,'/');
		if(ppath==NULL) ppath=phost+lstrlenA(phost);
		if(nlhr->flags&NLHRF_GENERATEHOST) {
			szHost=(char*)mir_alloc(ppath-phost+1);
			lstrcpynA(szHost,phost,ppath-phost+1);
		}
		if(nlhr->flags&NLHRF_REMOVEHOST
		   || (nlhr->flags&NLHRF_SMARTREMOVEHOST
		       && (!nlc->nlu->settings.useProxy
			       || !(nlc->nlu->settings.proxyType==PROXYTYPE_HTTP || nlc->nlu->settings.proxyType==PROXYTYPE_HTTPS)))) {
           pszUrl = ppath[0] ? ppath : "/";
		}
		else pszUrl=nlhr->szUrl;
	}
	else pszUrl=nlhr->szUrl;
	AppendToCharBuffer(&httpRequest, "%s %s HTTP/1.%d\r\n", pszRequest, pszUrl, (nlhr->flags & NLHRF_HTTP11) != 0);

	//if (nlhr->dataLength > 0)
	//	AppendToCharBuffer(&httpRequest,"Content-Length: %d\r\n",nlhr->dataLength);

	//proxy auth initialization
	useProxyHttpAuth=nlhr->flags&NLHRF_SMARTAUTHHEADER && nlc->nlu->settings.useProxy && nlc->nlu->settings.useProxyAuth && (nlc->nlu->settings.proxyType==PROXYTYPE_HTTP || nlc->nlu->settings.proxyType==PROXYTYPE_HTTPS);
	usingNtlmAuthentication=0;
	if(useProxyHttpAuth) {
		if(nlc->nlu->settings.useProxyAuthNtlm) {
			char *pszNtlmAuth;
			pszNtlmAuth=NtlmCreateResponseFromChallenge(nlc->hNtlmSecurity, "", 
				nlc->nlu->settings.szProxyAuthUser, nlc->nlu->settings.szProxyAuthPassword);

			if(pszNtlmAuth==NULL) {useProxyHttpAuth=0; pszProxyAuthorizationHeader=NULL;}
			else {
				pszProxyAuthorizationHeader=(char*)mir_alloc(lstrlenA(pszNtlmAuth)+6);
				lstrcpyA(pszProxyAuthorizationHeader,"NTLM ");
				lstrcatA(pszProxyAuthorizationHeader,pszNtlmAuth);
				mir_free(pszNtlmAuth);
				usingNtlmAuthentication=1;
			}
		}
		else {
			NETLIBBASE64 nlb64;
			char szAuth[512];
			mir_snprintf(szAuth,SIZEOF(szAuth),"%s:%s",nlc->nlu->settings.szProxyAuthUser,nlc->nlu->settings.szProxyAuthPassword);
			nlb64.cbDecoded=lstrlenA(szAuth);
			nlb64.pbDecoded=(PBYTE)szAuth;
			nlb64.cchEncoded=Netlib_GetBase64EncodedBufferSize(nlb64.cbDecoded);
			nlb64.pszEncoded=(char*)mir_alloc(nlb64.cchEncoded);
			NetlibBase64Encode(0,(LPARAM)&nlb64);
			pszProxyAuthorizationHeader=(char*)mir_alloc(lstrlenA(nlb64.pszEncoded)+7);
			lstrcpyA(pszProxyAuthorizationHeader,"Basic ");
			lstrcatA(pszProxyAuthorizationHeader,nlb64.pszEncoded);
			mir_free(nlb64.pszEncoded);
		}
	}
	else pszProxyAuthorizationHeader=NULL;

	//HTTP headers
	doneHostHeader=doneContentLengthHeader=doneProxyAuthHeader=doneConnectionHeader=0;
	for(i=0;i<nlhr->headersCount;i++) {
		if(!lstrcmpiA(nlhr->headers[i].szName,"Host")) doneHostHeader=1;
		else if(!lstrcmpiA(nlhr->headers[i].szName,"Content-Length")) doneContentLengthHeader=1;
		else if(!lstrcmpiA(nlhr->headers[i].szName,"Proxy-Authorization")) doneProxyAuthHeader=1;
		else if(!lstrcmpiA(nlhr->headers[i].szName,"Connection")) doneConnectionHeader=1;
		else if(!lstrcmpiA(nlhr->headers[i].szName,"Connection") && usingNtlmAuthentication) continue;
		if(nlhr->headers[i].szValue==NULL) continue;
		AppendToCharBuffer(&httpRequest,"%s: %s\r\n",nlhr->headers[i].szName,nlhr->headers[i].szValue);
	}
	if(szHost && !doneHostHeader) AppendToCharBuffer(&httpRequest,"%s: %s\r\n","Host",szHost);
	if(pszProxyAuthorizationHeader) {
		if(!doneProxyAuthHeader) AppendToCharBuffer(&httpRequest,"%s: %s\r\n","Proxy-Authorization",pszProxyAuthorizationHeader);
		mir_free(pszProxyAuthorizationHeader);
		if (usingNtlmAuthentication) {
			AppendToCharBuffer(&httpRequest,"%s: %s\r\n","Connection","Keep-Alive");
			doneConnectionHeader=1;
		}
	}
	if (!doneConnectionHeader && (nlhr->flags & NLHRF_HTTP11) && nlhr->requestType != REQUEST_CONNECT) 
		AppendToCharBuffer(&httpRequest,"%s: %s\r\n","Connection","close");

	// Add Sticky Headers
	if (nlc->nlu->szStickyHeaders != NULL) {
		AppendToCharBuffer(&httpRequest,"%s\r\n", nlc->nlu->szStickyHeaders);
	}

	//send it
	bytesSent=SendHttpRequestAndData(nlc,&httpRequest,nlhr,!doneContentLengthHeader);
	if(bytesSent==SOCKET_ERROR) {
		if(szHost) mir_free(szHost);
		NetlibLeaveNestedCS(&nlc->ncsSend);
		return SOCKET_ERROR;
	}

	//ntlm reply
	if(usingNtlmAuthentication) {
		int resultCode=0;

		if(!HttpPeekFirstResponseLine(nlc,GetTickCount()+5000,MSG_PEEK|MSG_DUMPASTEXT|(nlhr->flags&(NLHRF_NODUMP|NLHRF_NODUMPHEADERS)?MSG_NODUMP:(nlhr->flags&NLHRF_DUMPPROXY?MSG_DUMPPROXY:0)),&resultCode,NULL,NULL)
		   || ((resultCode<200 || resultCode>=300) && resultCode!=407)) {
			if(szHost) mir_free(szHost);
			NetlibLeaveNestedCS(&nlc->ncsSend);
			if(resultCode) NetlibHttpSetLastErrorUsingHttpResult(resultCode);
			return SOCKET_ERROR;
		}
		if(resultCode==407) {	//proxy auth required
			NETLIBHTTPREQUEST *nlhrReply;
			int i,error;

			DWORD hflags = nlhr->flags&(NLHRF_NODUMP|NLHRF_NODUMPHEADERS|NLHRF_NODUMPSEND)?MSG_NODUMP:(nlhr->flags&NLHRF_DUMPPROXY?MSG_DUMPPROXY:0);
			DWORD dflags = nlhr->flags&(NLHRF_NODUMP|NLHRF_NODUMPSEND)?MSG_NODUMP:MSG_DUMPASTEXT|MSG_DUMPPROXY;

			if (nlhr->requestType == REQUEST_HEAD)
				nlhrReply = (NETLIBHTTPREQUEST*)NetlibHttpRecvHeaders((WPARAM)nlc, hflags);
			else
				nlhrReply = NetlibHttpRecv(nlc, hflags, dflags);

			if(nlhrReply==NULL) {
				if(szHost) mir_free(szHost);
				NetlibLeaveNestedCS(&nlc->ncsSend);
				if(resultCode) NetlibHttpSetLastErrorUsingHttpResult(resultCode);
				return SOCKET_ERROR;
			}
			pszProxyAuthorizationHeader=NULL;
			error=ERROR_SUCCESS;
			for(i=0;i<nlhrReply->headersCount;i++) {
				if(!lstrcmpiA(nlhrReply->headers[i].szName,"Proxy-Authenticate")) {
					if(!_strnicmp(nlhrReply->headers[i].szValue,"NTLM ",5)) {
						pszProxyAuthorizationHeader=NtlmCreateResponseFromChallenge(nlc->hNtlmSecurity, nlhrReply->headers[i].szValue+5,
							nlc->nlu->settings.szProxyAuthUser, nlc->nlu->settings.szProxyAuthPassword);
					}
					else error=ERROR_ACCESS_DENIED;
				}
			}
			NetlibHttpFreeRequestStruct(0,(LPARAM)nlhrReply);
			if(pszProxyAuthorizationHeader==NULL) {
				if(error!=ERROR_SUCCESS) SetLastError(error);
				if(szHost) mir_free(szHost);
				NetlibLeaveNestedCS(&nlc->ncsSend);
				return SOCKET_ERROR;
			}

			httpRequest.cbAlloced=httpRequest.iEnd=0;
			httpRequest.sz=NULL;
			AppendToCharBuffer(&httpRequest,"%s %s HTTP/1.%d\r\n",pszRequest,pszUrl,(nlhr->flags & NLHRF_HTTP11) != 0);

			//HTTP headers
			doneHostHeader=doneContentLengthHeader=doneConnectionHeader=0;
			for(i=0;i<nlhr->headersCount;i++) {
				if(!lstrcmpiA(nlhr->headers[i].szName,"Host")) doneHostHeader=1;
				else if(!lstrcmpiA(nlhr->headers[i].szName,"Content-Length")) doneContentLengthHeader=1;
				else if(!lstrcmpiA(nlhr->headers[i].szName,"Connection")) doneConnectionHeader=1;
				if(nlhr->headers[i].szValue==NULL) continue;
				AppendToCharBuffer(&httpRequest,"%s: %s\r\n",nlhr->headers[i].szName,nlhr->headers[i].szValue);
			}
			if(szHost) {
				if(!doneHostHeader) AppendToCharBuffer(&httpRequest,"%s: %s\r\n","Host",szHost);
				mir_free(szHost); szHost=NULL;
			}
			if(!doneConnectionHeader && (nlhr->flags & NLHRF_HTTP11)) AppendToCharBuffer(&httpRequest,"%s: %s\r\n","Connection","close");
			AppendToCharBuffer(&httpRequest,"%s: NTLM %s\r\n","Proxy-Authorization",pszProxyAuthorizationHeader);
			mir_free(pszProxyAuthorizationHeader);

			//send it
			bytesSent=SendHttpRequestAndData(nlc,&httpRequest,nlhr,!doneContentLengthHeader);
			if(bytesSent==SOCKET_ERROR) {
				NetlibLeaveNestedCS(&nlc->ncsSend);
				return SOCKET_ERROR;
			}
		}
	}

	//clean up
	if(szHost) mir_free(szHost);
	NetlibLeaveNestedCS(&nlc->ncsSend);
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
		return (INT_PTR)(NETLIBHTTPREQUEST*)NULL;
	}
	bytesPeeked=NLRecv(nlc,buffer,firstLineLength,lParam|MSG_DUMPASTEXT);
	if(bytesPeeked<firstLineLength) {
		NetlibLeaveNestedCS(&nlc->ncsRecv);
		NetlibHttpFreeRequestStruct(0,(LPARAM)nlhr);
		if(bytesPeeked!=SOCKET_ERROR) SetLastError(ERROR_HANDLE_EOF);
		return (INT_PTR)(NETLIBHTTPREQUEST*)NULL;
	}
	for(;;) {
		bytesPeeked=RecvWithTimeoutTime(nlc,dwRequestTimeoutTime,buffer,SIZEOF(buffer)-1,MSG_PEEK|lParam);
        if (bytesPeeked==0) break;

		if(bytesPeeked==SOCKET_ERROR) {
			NetlibLeaveNestedCS(&nlc->ncsRecv);
			NetlibHttpFreeRequestStruct(0,(LPARAM)nlhr);
			if(bytesPeeked==0) SetLastError(ERROR_HANDLE_EOF);
			return (INT_PTR)(NETLIBHTTPREQUEST*)NULL;
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
				return (INT_PTR)(NETLIBHTTPREQUEST*)NULL;
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

INT_PTR NetlibHttpTransaction(WPARAM wParam,LPARAM lParam)
{
	struct NetlibUser *nlu=(struct NetlibUser*)wParam;
	NETLIBHTTPREQUEST *nlhr=(NETLIBHTTPREQUEST*)lParam,*nlhrReply;
	HANDLE hConnection = nlhr->nlc;
	DWORD dflags;

	if(GetNetlibHandleType(nlu)!=NLH_USER || !(nlu->user.flags&NUF_OUTGOING) || nlhr==NULL || nlhr->cbSize!=sizeof(NETLIBHTTPREQUEST) || nlhr->szUrl==NULL || nlhr->szUrl[0]=='\0') {
		SetLastError(ERROR_INVALID_PARAMETER);
		return (INT_PTR)(HANDLE)NULL;
	}

	if (hConnection)
	{
		if (!WaitUntilWritable(((struct NetlibConnection*)hConnection)->s, 0))
		{
			NetlibCloseHandle((WPARAM)hConnection,0);
			hConnection = NULL;
		}
	}

	if (hConnection==NULL)
	{
		NETLIBOPENCONNECTION nloc={0};
		char szHost[128];
		char *ppath,*phost,*pcolon;
		BOOL secur;

		secur = _strnicmp(nlhr->szUrl, "https", 5) == 0;
		phost=strstr(nlhr->szUrl,"://");
		if(phost==NULL) phost=nlhr->szUrl;
		else phost+=3;
		lstrcpynA(szHost,phost,SIZEOF(szHost));
		ppath=strchr(szHost,'/');
		if(ppath) *ppath='\0';
		nloc.cbSize=sizeof(nloc);
		nloc.szHost=szHost;
		pcolon=strrchr(szHost,':');
		if(pcolon) {
			*pcolon='\0';
			nloc.wPort=(WORD)strtol(pcolon+1,NULL,10);
		}
		else nloc.wPort = secur ? 443 : 80;
		nloc.flags = NLOCF_HTTP | (secur ? NLOCF_SSL : 0);
		hConnection=(HANDLE)NetlibOpenConnection((WPARAM)nlu,(LPARAM)&nloc);
		if(hConnection==NULL) return (INT_PTR)(HANDLE)NULL;
	}

	{
		NETLIBHTTPREQUEST nlhrSend;
		int i,doneUserAgentHeader=0,doneAcceptEncoding=0;
		char szUserAgent[64];

		nlhrSend=*nlhr;
		nlhrSend.flags&=~NLHRF_REMOVEHOST;
		nlhrSend.flags|=NLHRF_GENERATEHOST|NLHRF_SMARTREMOVEHOST|NLHRF_SMARTAUTHHEADER;
		for(i=0;i<nlhr->headersCount;i++) {
			if(!lstrcmpiA(nlhr->headers[i].szName,"User-Agent"))
				doneUserAgentHeader=1;
			else if(!lstrcmpiA(nlhr->headers[i].szName,"Accept-Encoding"))
				doneAcceptEncoding=1;
		}
		if(!doneUserAgentHeader||!doneAcceptEncoding) {
			nlhrSend.headers=(NETLIBHTTPHEADER*)mir_alloc(sizeof(NETLIBHTTPHEADER)*(nlhrSend.headersCount+2));
			memcpy(nlhrSend.headers,nlhr->headers,sizeof(NETLIBHTTPHEADER)*nlhr->headersCount);
        }
		if(!doneUserAgentHeader) {
			char *pspace,szMirandaVer[32];

			nlhrSend.headers[nlhrSend.headersCount].szName="User-Agent";
			nlhrSend.headers[nlhrSend.headersCount].szValue=szUserAgent;
			++nlhrSend.headersCount;
			CallService(MS_SYSTEM_GETVERSIONTEXT,SIZEOF(szMirandaVer),(LPARAM)szMirandaVer);
			pspace=strchr(szMirandaVer,' ');
			if(pspace) {
				*pspace++='\0';
				mir_snprintf(szUserAgent,SIZEOF(szUserAgent),"Miranda/%s (%s)",szMirandaVer,pspace);
			}
			else mir_snprintf(szUserAgent,SIZEOF(szUserAgent),"Miranda/%s",szMirandaVer);
		}
        if (!doneAcceptEncoding) {
			nlhrSend.headers[nlhrSend.headersCount].szName="Accept-Encoding";
			nlhrSend.headers[nlhrSend.headersCount].szValue="deflate, gzip";
			++nlhrSend.headersCount;
        }
		if(NetlibHttpSendRequest((WPARAM)hConnection,(LPARAM)&nlhrSend)==SOCKET_ERROR) {
			if(!doneUserAgentHeader||!doneAcceptEncoding) mir_free(nlhrSend.headers);
			NetlibCloseHandle((WPARAM)hConnection,0);
			return (INT_PTR)(HANDLE)NULL;
		}
		if(!doneUserAgentHeader||!doneAcceptEncoding) mir_free(nlhrSend.headers);
	}

	dflags = (nlhr->flags&NLHRF_DUMPASTEXT?MSG_DUMPASTEXT:0) |
		(nlhr->flags&NLHRF_NODUMP?MSG_NODUMP:(nlhr->flags&NLHRF_DUMPPROXY?MSG_DUMPPROXY:0));

	if (nlhr->requestType == REQUEST_HEAD)
		nlhrReply = (NETLIBHTTPREQUEST*)NetlibHttpRecvHeaders((WPARAM)hConnection, 0);
	else
		nlhrReply = NetlibHttpRecv(hConnection, 0, dflags);

    if ((nlhr->flags&NLHRF_PERSISTENT) == 0)
	    NetlibCloseHandle((WPARAM)hConnection,0);
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

int NetlibHttpRecvChunkHeader(HANDLE hConnection, BOOL first)
{
	char data[32], *peol1;

	for (;;)
	{
		int recvResult = NLRecv((struct NetlibConnection*)hConnection, data, 31, MSG_PEEK);
		if (recvResult <= 0) return SOCKET_ERROR;

		data[recvResult] = 0;

		peol1 = strstr(data, "\r\n");
		if (peol1 != NULL)
		{
			char *peol2 = first ? peol1 : strstr(peol1 + 2, "\r\n");
			if (peol2 != NULL)
			{
				NLRecv((struct NetlibConnection*)hConnection, data, peol2 - data + 2, 0);
				return strtol(first ? data : peol1+2, NULL, 16);
			}
			else
				if (recvResult >= 31) return SOCKET_ERROR;

		}
	}
}

NETLIBHTTPREQUEST* NetlibHttpRecv(HANDLE hConnection, DWORD hflags, DWORD dflags)
{
	int dataLen = -1, i, chunkhdr = 0;
	int chunked = FALSE;
	int cenc = 0, cenctype = 0;

next:
	NETLIBHTTPREQUEST *nlhrReply = (NETLIBHTTPREQUEST*)NetlibHttpRecvHeaders((WPARAM)hConnection, hflags);
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
			chunked = TRUE;
			chunkhdr = i;
		}
	}

	if (nlhrReply->resultCode >= 200 && dataLen != 0)
	{
		int recvResult, chunksz = 0;
		int dataBufferAlloced;

		if (chunked)
		{
			chunksz = NetlibHttpRecvChunkHeader(hConnection, TRUE);
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
				recvResult = NLRecv((struct NetlibConnection*)hConnection, nlhrReply->pData+nlhrReply->dataLength,
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
				chunksz = NetlibHttpRecvChunkHeader(hConnection, FALSE);
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
			NetlibDumpData( (NetlibConnection*)hConnection, ( PBYTE )szData, bufsz, 0, dflags );
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
