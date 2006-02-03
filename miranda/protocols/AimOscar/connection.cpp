#include "connection.h"
CRITICAL_SECTION statusMutex;
CRITICAL_SECTION connectionMutex;
HANDLE aim_connect(char* server)
{
	char* server_dup=strdup(server);
    NETLIBOPENCONNECTION ncon = { 0 };
	char* host=strtok(server_dup,":");
	char* port=strtok(NULL,":");
    ncon.cbSize = sizeof(ncon);
    ncon.szHost = host;
    ncon.wPort =atoi(port);
    HANDLE con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) conn.hNetlib, (LPARAM) & ncon);
	free(server_dup);
	load_extra_icons();
	return con;
}
HANDLE aim_peer_connect(char* ip,unsigned short port)
{
    NETLIBOPENCONNECTION ncon = { 0 };
    ncon.cbSize = sizeof(ncon);
    ncon.szHost = ip;
    ncon.wPort =port;
    HANDLE con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) conn.hNetlibPeer, (LPARAM) & ncon);
	return con;
}
void CALLBACK aim_keepalive_connection(HWND hwnd, UINT message, UINT idEvent, DWORD dwTime)
{
    if (conn.state==1)
       aim_keepalive();
}
void __cdecl aim_connection_authorization()
{
	EnterCriticalSection(&connectionMutex);
	NETLIBPACKETRECVER packetRecv;
	DBVARIANT dbv;
	int recvResult=0;
	if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PW, &dbv))
	{
        CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
        conn.password = _strdup(dbv.pszVal);
        DBFreeVariant(&dbv);
	}
	else
	{
		MessageBox( NULL, "Please, enter a password in the options dialog.", AIM_PROTOCOL_NAME, MB_OK );
		LeaveCriticalSection(&connectionMutex);
		return;
	}
	if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
	{
        conn.username = _strdup(dbv.pszVal);
        DBFreeVariant(&dbv);
    }
	else
	{
		MessageBox( NULL, "Please, enter a username in the options dialog.", AIM_PROTOCOL_NAME, MB_OK );
		LeaveCriticalSection(&connectionMutex);
		return;
	}
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	conn.hServerPacketRecver=NULL;
	conn.hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)conn.hServerConn, 2048 * 4);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = INFINITE;
	while(1)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) conn.hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0) {
                break;
            }
        if (recvResult == SOCKET_ERROR) {
                break;
            }
		if(recvResult>0)
		{
			unsigned short flap_length=0;
			for(;packetRecv.bytesUsed<packetRecv.bytesAvailable;packetRecv.bytesUsed=flap_length)
			{
				char buf[MSG_LEN*4];
				struct flap_header* flap=(struct flap_header*)&packetRecv.buffer[packetRecv.bytesUsed];
				flap_length+=FLAP_SIZE;
				if(packetRecv.bytesAvailable<flap_length)//Part of the tcp packet missing go back and get it.
					break;
				ZeroMemory(buf,sizeof(buf));
				if(!packetRecv.buffer)
					break;
				memcpy(buf,&packetRecv.buffer[flap_length],htons(flap->len));
				flap_length+=htons(flap->len);
				if(packetRecv.bytesAvailable<flap_length)//Part of the tcp packet missing go back and get it.
					break;
				if(flap->type==1)
				{
					if(aim_send_connection_packet(buf)==0)//cookie challenge
					{
						aim_authkey_request();//md5 authkey request
					}
				}
				else if(flap->type==2)
				{
					struct snac_header* snac=(struct snac_header*)buf;
					snac->service=htons(snac->service);
					snac->subgroup=htons(snac->subgroup);
					if(snac->service==0x0017)
					{
						snac_md5_authkey(snac->subgroup,buf);
						if(snac_authorization_reply(snac->subgroup,buf,htons(flap->len))==1)
						{
							free(conn.username);
							free(conn.password);
							LeaveCriticalSection(&connectionMutex);
							return;
						}
					}
				}
				if(flap->type==4)
				{
					free(conn.username);
					free(conn.password);
					conn.state=0;
					broadcast_status(ID_STATUS_OFFLINE);
					LeaveCriticalSection(&connectionMutex);
					return;
				}
			}
		}
	}
	free(conn.username);
	free(conn.password);
	LeaveCriticalSection(&connectionMutex);
	conn.state=0;
	broadcast_status(ID_STATUS_OFFLINE);
}
void __cdecl aim_protocol_negotiation()
{
	EnterCriticalSection(&connectionMutex);
	NETLIBPACKETRECVER packetRecv;
	int recvResult=0;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	conn.hServerPacketRecver=NULL;
	conn.hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)conn.hServerConn, 2048 * 8);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = INFINITE;
	while(1)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) conn.hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0)
		{
                break;
		}
        if (recvResult == SOCKET_ERROR)
		{
                break;
		}
		if(recvResult>0)
		{
			unsigned short flap_length=0;
			for(;packetRecv.bytesUsed<packetRecv.bytesAvailable;packetRecv.bytesUsed=flap_length)
			{
				char buf[MSG_LEN*4];
				struct flap_header* flap=(struct flap_header*)&packetRecv.buffer[packetRecv.bytesUsed];
				flap_length+=FLAP_SIZE;
				if(packetRecv.bytesAvailable<flap_length)//Part of the tcp packet missing go back and get it.
					break;
				ZeroMemory(buf,sizeof(buf));
				if(!packetRecv.buffer)
					break;
				memcpy(buf,&packetRecv.buffer[flap_length],htons(flap->len));
				flap_length+=htons(flap->len);
				if(packetRecv.bytesAvailable<flap_length)//Part of the tcp packet missing go back and get it.
					break;
  				if(flap->type==1)
				{
					aim_send_cookie(COOKIE_LENGTH,COOKIE);//cookie challenge
					free(COOKIE);
					COOKIE=NULL;
					COOKIE_LENGTH=0;
				}
				else if(flap->type=2)
				{
					struct snac_header* snac=(struct snac_header*)buf;
					snac->service=htons(snac->service);
					snac->subgroup=htons(snac->subgroup);
					if(snac->service==0x0001)
					{
						snac_supported_families(snac->subgroup);
						snac_supported_family_versions(snac->subgroup);
						snac_rate_limitations(snac->subgroup);
					}
					else if(snac->service==0x0002)
					{
						snac_received_info(snac->subgroup,buf,htons(flap->len));
					}
					else if(snac->service==0x0003)
					{
						snac_user_online(snac->subgroup,buf);
						snac_user_offline(snac->subgroup,buf);
						snac_buddylist_error(snac->subgroup,buf);
					}
					else if(snac->service==0x0004)
					{
						snac_icbm_limitations(snac->subgroup);
						snac_message_accepted(snac->subgroup,buf);
						snac_received_message(snac->subgroup,buf,htons(flap->len));
						snac_typing_notification(snac->subgroup,buf);
					}
					else if(snac->service==0x0013)
					{
						snac_contact_list(snac->subgroup,buf,htons(flap->len));
						snac_buddylist_error(snac->subgroup,buf);
					}
				}
				else if(flap->type==4)
				{
					conn.state=0;
					LeaveCriticalSection(&connectionMutex);
					return;
				}
			}
		}
	}
	conn.state=0;
	conn.buddy_list_received=0;
	broadcast_status(ID_STATUS_OFFLINE);
	LeaveCriticalSection(&connectionMutex);
}
