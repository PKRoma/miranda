//MSN Messenger Support for Miranda ICQ
//Copyright 2000, Tristan Van de Vreede

//WINSOCK FUNCS

	#include <windows.h>
	#include <winsock.h>

	#include "miranda.h"

	#include "global.h"
	
	#include "msn_global.h"
	
	BOOL MSN_WS_Init()
	{
		WSADATA wsd;
		if (WSAStartup(MAKEWORD(2,2),&wsd)!=0)
		{
			MSN_DebugLog("FAILED TO INIT WINSOCK!!");
			return FALSE;
		}
		
		return TRUE;
	}

	void MSN_WS_CleanUp()
	{
		WSACleanup();
	}

	int MSN_WS_SendData(SOCKET s,char*data)
	{
		int rlen;
		MSN_DebugLogEx("OUT: ",data,NULL);
		if ((rlen=send(s,data,strlen(data),NULL))==SOCKET_ERROR)
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
		shutdown(s,SB_BOTH);
		close(s);
		*s=0;

		if (!opts.MSN.sNS && !transfering)
		{
			MSN_DisconnectAllSessions();//make sure all SS cons are killed
			
			SetStatusTextEx("Offline",STATUS_SLOT_MSN);
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
		if (!select(NULL, &readfds, 0L, 0L, &tv)>0)
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
			if ((rlen=recv(*s,data,datalen,NULL))==SOCKET_ERROR)
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


