/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-2  Richard Hughes, Roland Rabien & Tristan Van de Vreede

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
//MSN Messenger Support for Miranda ICQ
//Copyright 2000, Tristan Van de Vreede

//WINSOCK FUNCS

#include "msn_global.h"
//	#include "resource.h"
#include "../../miranda32/ui/contactlist/m_clist.h"

BOOL MSN_WS_Init()
{
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2,2),&wsd)!=0) return FALSE;
	
	return TRUE;
}

void MSN_WS_CleanUp()
{
	WSACleanup();
}

int MSN_WS_Send(SOCKET s,char*data,int datalen)
{
	int rlen;
	MSN_DebugLog(MSN_LOG_RAWDATA,"SEND:%s",data);
	if ((rlen=send(s,data,datalen,0))==SOCKET_ERROR)
	{//should really also check if sendlen is the same as datalen
		MSN_DebugLog(MSN_LOG_WARNING,"Send failed: %d",WSAGetLastError());
		return FALSE;
	}
	return TRUE;
}

unsigned long MSN_WS_ResolveName(char*name,WORD *port,int defaultPort)
{
	HOSTENT *lk;
	char *pcolon,*nameCopy;
	DWORD ret;

	nameCopy=_strdup(name);
	if(port!=NULL) *port=defaultPort;
	pcolon=strchr(nameCopy,':');
	if(pcolon!=NULL) {
		if(port!=NULL) *port=atoi(pcolon+1);
		*pcolon=0;
	}
	if (inet_addr(nameCopy)==INADDR_NONE) {//DNS NAME
		lk=gethostbyname(nameCopy);
		if(lk==0) {//bad name, failed
			MSN_DebugLog(MSN_LOG_ERROR,"Can't resolve name '%s' (%d)",name,WSAGetLastError());
			return SOCKET_ERROR;	
		}
		else {free(nameCopy); return *(u_long*)lk->h_addr_list[0];}
	}
	ret=inet_addr(nameCopy);
	free(nameCopy);
	return ret;
}

int MSN_WS_Recv(SOCKET s,char*data,long datalen)
{
	int ret;

	ret=recv(s,data,datalen,0);
	MSN_DebugLog(MSN_LOG_RAWDATA,"RAW(%d,%d):%*s",ret,WSAGetLastError(),ret,data);
	if(ret==SOCKET_ERROR) {
		MSN_DebugLog(MSN_LOG_FATAL,"recv() failed, error=%d",WSAGetLastError());
		return 0;
	}
	if(ret==0) {
		MSN_DebugLog(MSN_LOG_MESSAGE,"Connection closed gracefully");
		return 0;
	}
	return ret;
}