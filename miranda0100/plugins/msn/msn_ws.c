/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000-1  Richard Hughes, Roland Rabien & Tristan Van de Vreede

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
/*
#ifndef MODULAR
	#include "../../core/miranda.h"

	#include "../../global.h"

    void SetStatusTextEx(char*buff,int slot);
#endif
	void MSN_DisconnectAllSessions();
*/	
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

	int MSN_WS_SendData(SOCKET s,char*data)
	{
		int rlen;
		if ((rlen=send(s,data,strlen(data),0))==SOCKET_ERROR)
		{//should really also check if sendlen is the same as datalen
			return FALSE;
		}
		return TRUE;
	}


	unsigned long MSN_WS_ResolveName(char*name)
	{
		HOSTENT *lk;

		if (inet_addr(name)==INADDR_NONE)
		{//DNS NAME
			lk=gethostbyname(name);
			if (lk==0)
			{//bad name, failed
				MSN_DebugLog("Can't resolve name.");
				return SOCKET_ERROR;	
			}
			else
			{
				return *(u_long*)lk->h_addr_list[0];
			}
		}
		else
		{//IP ADDR
			return inet_addr(name);
		}
	}

	void MSN_WS_Close(SOCKET *s,BOOL transfering)
	{
		shutdown(*s,SB_BOTH);
		*s=0;

		if (!msnSock && !transfering)
		{
			//MSN_DisconnectAllSessions();//make sure all SS cons are killed
			//CallService(MS_CLIST_MSNSTATUSCHANGED,ID_STATUS_OFFLINE,0);
		}
	}

	int MSN_WS_Recv(SOCKET *s,char*data,long datalen)
	{
		int rlen;
		
		//check for data, return FALSE if no data,other wise return the recv ret
		struct timeval tv;
		fd_set readfds;

		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&readfds);
		FD_SET(*s, &readfds);
		if (!select(0, &readfds, 0L, 0L, &tv)>0)
		{//data not waiting
			return FALSE;
		}

		/*if (!FD_ISSET(s,&readfds))
		{//DATA is NOT waiting
			return FALSE;
		}
		else
		{	//data there*/

			data[0]=0;
			if ((rlen=recv(*s,data,datalen-1,0))==SOCKET_ERROR)
			{
				DWORD err;
				err=GetLastError();
				if (err==10053)
				{//Connection Aborted, Terminate it
					MSN_DebugLog("TCP Connection closed\n");
					MSN_WS_Close(s,FALSE);
				}
				else
				{
					MSN_DebugLog("TCP Recv failed\n");
				}

				
				return FALSE;
				
			}
			else
			{
				if (rlen==0) 
					return FALSE;

				data[rlen]=0;
				return rlen;
			}
		//}
	}

 