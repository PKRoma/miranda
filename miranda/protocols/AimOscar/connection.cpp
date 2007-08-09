#include "connection.h"
CRITICAL_SECTION statusMutex;
CRITICAL_SECTION connectionMutex;
int LOG(const char *fmt, ...)
{
	va_list va;
	char szText[1024];
	if (!conn.hNetlib)
		return 0;
	va_start(va, fmt);
	mir_vsnprintf(szText, sizeof(szText), fmt, va);
	va_end(va);
	return CallService(MS_NETLIB_LOG, (WPARAM) conn.hNetlib, (LPARAM) szText);
}
HANDLE aim_connect(char* server)
{
	char* server_dup=strldup(server,lstrlen(server));
    NETLIBOPENCONNECTION ncon = { 0 };
	char* host=strtok(server_dup,":");
	char* port=strtok(NULL,":");
    ncon.cbSize = sizeof(ncon);
    ncon.szHost = host;
    ncon.wPort =(short)atoi(port);
	conn.port=ncon.wPort;
	ncon.timeout=5;
    HANDLE con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) conn.hNetlib, (LPARAM) & ncon);
	delete[] server_dup;
	return con;
}
HANDLE aim_peer_connect(char* ip,unsigned short port)
{
    NETLIBOPENCONNECTION ncon = { 0 };
    ncon.cbSize = sizeof(ncon);
	ncon.flags = NLOCF_V2;
    ncon.szHost = ip;
    ncon.wPort =port;
	ncon.timeout=1;
    HANDLE con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) conn.hNetlibPeer, (LPARAM) & ncon);
	return con;
}
void __cdecl aim_connection_authorization()
{
	EnterCriticalSection(&connectionMutex);
	NETLIBPACKETRECVER packetRecv;
	DBVARIANT dbv;
	int recvResult=0;
	if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PW, &dbv))
	{
        CallService(MS_DB_CRYPT_DECODESTRING, lstrlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
        conn.password = strldup(dbv.pszVal,lstrlen(dbv.pszVal));
        DBFreeVariant(&dbv);
	}
	else
	{
		LeaveCriticalSection(&connectionMutex);
		return;
	}
	if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
	{
        conn.username = strldup(dbv.pszVal,lstrlen(dbv.pszVal));
        DBFreeVariant(&dbv);
    }
	else
	{
		LeaveCriticalSection(&connectionMutex);
		return;
	}
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	conn.hServerPacketRecver=NULL;
	conn.hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)conn.hServerConn, 2048 * 4);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = 5000;
	#if _MSC_VER
	#pragma warning( disable: 4127)
	#endif
	while(1)
	{
		#if _MSC_VER
		#pragma warning( default: 4127)
		#endif
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) conn.hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0)
		{
			LOG("Connection Closed: No Error? during Connection Authorization");
			break;
		}
        if (recvResult == SOCKET_ERROR)
		{
			LOG("Connection Closed: Socket Error during Connection Authorization");
			break;
		}
		if(recvResult>0)
		{
			unsigned short flap_length=0;
			for(;packetRecv.bytesUsed<packetRecv.bytesAvailable;packetRecv.bytesUsed=flap_length)
			{
				if(!packetRecv.buffer)
					break;
				FLAP flap((char*)&packetRecv.buffer[packetRecv.bytesUsed],(unsigned short)(packetRecv.bytesAvailable-packetRecv.bytesUsed));
				if(!flap.len())
					break;
				flap_length+=FLAP_SIZE+flap.len();
				if(flap.cmp(0x01))
				{
					if(aim_send_connection_packet(conn.hServerConn,conn.seqno,flap.val())==0)//cookie challenge
					{
						aim_authkey_request(conn.hServerConn,conn.seqno);//md5 authkey request
					}
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0017))
					{
						snac_md5_authkey(snac,conn.hServerConn,conn.seqno);
						if(snac_authorization_reply(snac)==1)
						{
							delete[] conn.username;
							delete[] conn.password;
							Netlib_CloseHandle(conn.hServerPacketRecver);
							LOG("Connection Authorization Thread Ending: Negotiation Beginning");
							LeaveCriticalSection(&connectionMutex);
							return;
						}
					}
				}
				if(flap.cmp(0x04))
				{
					delete[] conn.username;
					delete[] conn.password;
					broadcast_status(ID_STATUS_OFFLINE);
					LOG("Connection Authorization Thread Ending: Flap 0x04");
					LeaveCriticalSection(&connectionMutex);
					return;
				}
			}
		}
	}
	delete[] conn.username;
	delete[] conn.password;
	broadcast_status(ID_STATUS_OFFLINE);
	LOG("Connection Authorization Thread Ending: End of Thread");
	LeaveCriticalSection(&connectionMutex);
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
	#if _MSC_VER
	#pragma warning( disable: 4127)
	#endif
	while(1)
	{
		#if _MSC_VER
		#pragma warning( default: 4127)
		#endif
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) conn.hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0)
		{
			LOG("Connection Closed: No Error during Connection Negotiation?");
			break;
		}
		if (recvResult == SOCKET_ERROR)
		{
			LOG("Connection Closed: Socket Error during Connection Negotiation");
			break;
		}
		if(recvResult>0)
		{
			unsigned short flap_length=0;
			for(;packetRecv.bytesUsed<packetRecv.bytesAvailable;packetRecv.bytesUsed=flap_length)
			{
				if(!packetRecv.buffer)
					break;
				FLAP flap((char*)&packetRecv.buffer[packetRecv.bytesUsed],(unsigned short)(packetRecv.bytesAvailable-packetRecv.bytesUsed));
				if(!flap.len())
					break;
				flap_length+=FLAP_SIZE+flap.len();
  				if(flap.cmp(0x01))
				{
					aim_send_cookie(conn.hServerConn,conn.seqno,COOKIE_LENGTH,COOKIE);//cookie challenge
					delete[] COOKIE;
					COOKIE=NULL;
					COOKIE_LENGTH=0;
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0001))
					{
						snac_supported_families(snac,conn.hServerConn,conn.seqno);
						snac_supported_family_versions(snac,conn.hServerConn,conn.seqno);
						snac_rate_limitations(snac,conn.hServerConn,conn.seqno);
						snac_service_redirect(snac);
						snac_error(snac);
					}
					else if(snac.cmp(0x0002))
					{
						snac_received_info(snac);
						snac_error(snac);
					}
					else if(snac.cmp(0x0003))
					{
						snac_user_online(snac);
						snac_user_offline(snac);
						snac_error(snac);
					}
					else if(snac.cmp(0x0004))
					{
						snac_icbm_limitations(snac,conn.hServerConn,conn.seqno);
						snac_message_accepted(snac);
						snac_received_message(snac,conn.hServerConn,conn.seqno);
						snac_typing_notification(snac);
						snac_error(snac);
						snac_busted_payload(snac);
					}
					else if(snac.cmp(0x0013))
					{
						snac_contact_list(snac,conn.hServerConn,conn.seqno);
						snac_list_modification_ack(snac);
						snac_error(snac);
					}
				}
				else if(flap.cmp(0x04))
				{
					offline_contacts();
					broadcast_status(ID_STATUS_OFFLINE);
					LOG("Connection Negotiation Thread Ending: Flap 0x04");
					SetEvent(conn.hAvatarEvent);
					LeaveCriticalSection(&connectionMutex);
					return;
				}
			}
		}
	}
	offline_contacts();
	broadcast_status(ID_STATUS_OFFLINE);
	SetEvent(conn.hAvatarEvent);
	LOG("Connection Negotiation Thread Ending: End of Thread");
	LeaveCriticalSection(&connectionMutex);
}
void __cdecl aim_mail_negotiation()
{
	NETLIBPACKETRECVER packetRecv;
	int recvResult=0;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	HANDLE hServerPacketRecver=NULL;
	hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)conn.hMailConn, 2048 * 8);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = INFINITE;
	#if _MSC_VER
	#pragma warning( disable: 4127)
	#endif
	while(1)
	{
		#if _MSC_VER
		#pragma warning( default: 4127)
		#endif
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) hServerPacketRecver, (LPARAM) & packetRecv);
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
				if(!packetRecv.buffer)
					break;
				FLAP flap((char*)&packetRecv.buffer[packetRecv.bytesUsed],(unsigned short)(packetRecv.bytesAvailable-packetRecv.bytesUsed));
				if(!flap.len())
					break;
				flap_length+=FLAP_SIZE+flap.len();
  				if(flap.cmp(0x01))
				{
					aim_send_cookie(conn.hMailConn,conn.mail_seqno,MAIL_COOKIE_LENGTH,MAIL_COOKIE);//cookie challenge
					delete[] MAIL_COOKIE;
					MAIL_COOKIE=NULL;
					MAIL_COOKIE_LENGTH=0;
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0001))
					{
						snac_supported_families(snac,conn.hMailConn,conn.mail_seqno);
						snac_supported_family_versions(snac,conn.hMailConn,conn.mail_seqno);
						snac_mail_rate_limitations(snac,conn.hMailConn,conn.mail_seqno);
						snac_error(snac);
					}
					else if(snac.cmp(0x0018))
					{
						snac_mail_response(snac);
					}
				}
				else if(flap.cmp(0x04))
				{
					Netlib_CloseHandle(hServerPacketRecver);
					LOG("Mail Server Connection has ended");
					return;
				}
			}
		}
	}
	LOG("Mail Server Connection has ended");
	Netlib_CloseHandle(hServerPacketRecver);
}
void __cdecl aim_avatar_negotiation()
{
	NETLIBPACKETRECVER packetRecv;
	int recvResult=0;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	HANDLE hServerPacketRecver=NULL;
	hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)conn.hAvatarConn, 2048 * 8);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = 300000;//5 minutes connected
	#if _MSC_VER
	#pragma warning( disable: 4127)
	#endif
	while(1)
	{
		#if _MSC_VER
		#pragma warning( default: 4127)
		#endif
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) hServerPacketRecver, (LPARAM) & packetRecv);
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
				if(!packetRecv.buffer)
					break;
				FLAP flap((char*)&packetRecv.buffer[packetRecv.bytesUsed],(unsigned short)(packetRecv.bytesAvailable-packetRecv.bytesUsed));
				if(!flap.len())
					break;
				flap_length+=FLAP_SIZE+flap.len();
  				if(flap.cmp(0x01))
				{
					aim_send_cookie(conn.hAvatarConn,conn.avatar_seqno,AVATAR_COOKIE_LENGTH,AVATAR_COOKIE);//cookie challenge
					delete[] AVATAR_COOKIE;
					AVATAR_COOKIE=NULL;
					AVATAR_COOKIE_LENGTH=0;
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0001))
					{
						snac_supported_families(snac,conn.hAvatarConn,conn.avatar_seqno);
						snac_supported_family_versions(snac,conn.hAvatarConn,conn.avatar_seqno);
						snac_avatar_rate_limitations(snac,conn.hAvatarConn,conn.avatar_seqno);
						snac_error(snac);
					}
					if(snac.cmp(0x0010))
					{
						snac_retrieve_avatar(snac);
					}
				}
				else if(flap.cmp(0x04))
				{
					Netlib_CloseHandle(hServerPacketRecver);
					conn.hAvatarConn=0;
					LOG("Avatar Server Connection has ended");
					conn.AvatarLimitThread=0;
					return;
				}
			}
		}
	}
	Netlib_CloseHandle(hServerPacketRecver);
	LOG("Avatar Server Connection has ended");
	conn.hAvatarConn=0;
	conn.AvatarLimitThread=0;
}
