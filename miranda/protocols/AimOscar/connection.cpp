#include "aim.h"
#include "connection.h"

int CAimProto::LOG(const char *fmt, ...)
{
	va_list va;
	char szText[1024];
	if (!hNetlib)
		return 0;
	va_start(va, fmt);
	mir_vsnprintf(szText, sizeof(szText), fmt, va);
	va_end(va);
	return CallService(MS_NETLIB_LOG, (WPARAM) hNetlib, (LPARAM) szText);
}

HANDLE CAimProto::aim_connect(char* server)
{
	char* server_dup=strldup(server,lstrlenA(server));
	NETLIBOPENCONNECTION ncon = { 0 };
	char* szPort = strchr(server_dup,':');
	if (szPort) *szPort++ = 0; else szPort = "5190";
	ncon.cbSize = sizeof(ncon);
	ncon.szHost = server_dup;
	port = ncon.wPort = (WORD)atol(szPort);
	ncon.timeout=5;
	LOG("%s:%u", server_dup, ncon.wPort);
	HANDLE con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) hNetlib, (LPARAM) & ncon);
	delete[] server_dup;
	return con;
}

HANDLE CAimProto::aim_peer_connect(char* ip,unsigned short port)
{
	NETLIBOPENCONNECTION ncon = { 0 };
	ncon.cbSize = sizeof(ncon);
	ncon.flags = NLOCF_V2;
	ncon.szHost = ip;
	ncon.wPort =port;
	ncon.timeout=1;
	HANDLE con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) hNetlibPeer, (LPARAM) & ncon);
	return con;
}

void __cdecl aim_connection_authorization( CAimProto* ppro )
{
	EnterCriticalSection(&ppro->connectionMutex);
	NETLIBPACKETRECVER packetRecv;
	DBVARIANT dbv;
	int recvResult=0;
	if (!ppro->getString(AIM_KEY_PW, &dbv))
	{
		CallService(MS_DB_CRYPT_DECODESTRING, lstrlenA(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
		ppro->password = strldup(dbv.pszVal,lstrlenA(dbv.pszVal));
		DBFreeVariant(&dbv);
	}
	else
	{
		LeaveCriticalSection(&ppro->connectionMutex);
		return;
	}
	if (!ppro->getString(AIM_KEY_SN, &dbv))
	{
		ppro->username = strldup(dbv.pszVal,lstrlenA(dbv.pszVal));
		DBFreeVariant(&dbv);
	}
	else
	{
		LeaveCriticalSection(&ppro->connectionMutex);
		return;
	}
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	ppro->hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)ppro->hServerConn, 2048 * 4);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = 5000;
	for(;;)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) ppro->hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0)
		{
			ppro->LOG("Connection Closed: No Error? during Connection Authorization");
			break;
		}
		else if (recvResult < 0)
		{
			ppro->LOG("Connection Closed: Socket Error during Connection Authorization %d", WSAGetLastError());
			break;
		}
		else
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
					if( ppro->aim_send_connection_packet(ppro->hServerConn,ppro->seqno,flap.val())==0)//cookie challenge
						ppro->aim_authkey_request(ppro->hServerConn,ppro->seqno);//md5 authkey request
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0017))
					{
						ppro->snac_md5_authkey(snac,ppro->hServerConn,ppro->seqno);
						if(ppro->snac_authorization_reply(snac)==1)
						{
							delete[] ppro->username;
							delete[] ppro->password;
							Netlib_CloseHandle(ppro->hServerPacketRecver);
							ppro->LOG("Connection Authorization Thread Ending: Negotiation Beginning");
							LeaveCriticalSection(&ppro->connectionMutex);
							return;
						}
					}
				}
				if(flap.cmp(0x04))
				{
					delete[] ppro->username;
					delete[] ppro->password;
					ppro->broadcast_status(ID_STATUS_OFFLINE);
					ppro->LOG("Connection Authorization Thread Ending: Flap 0x04");
					LeaveCriticalSection(&ppro->connectionMutex);
					return;
				}
			}
		}
	}
	delete[] ppro->username;
	delete[] ppro->password;
	ppro->broadcast_status(ID_STATUS_OFFLINE);
	ppro->LOG("Connection Authorization Thread Ending: End of Thread");
	LeaveCriticalSection(&ppro->connectionMutex);
}

void __cdecl aim_protocol_negotiation( CAimProto* ppro )
{
	EnterCriticalSection(&ppro->connectionMutex);
	NETLIBPACKETRECVER packetRecv;
	int recvResult=0;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	ppro->hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)ppro->hServerConn, 2048 * 8);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = INFINITE;	
	for(;;)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM)ppro->hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0)
		{
			ppro->LOG("Connection Closed: No Error during Connection Negotiation?");
			break;
		}
		if (recvResult == SOCKET_ERROR)
		{
			ppro->LOG("Connection Closed: Socket Error during Connection Negotiation %d", WSAGetLastError());
			break;
		}
		if(recvResult>0)
		{
			unsigned short flap_length=0;
			for(;packetRecv.bytesUsed<packetRecv.bytesAvailable;packetRecv.bytesUsed=flap_length)
			{
				if(!packetRecv.buffer)
					break;
				FLAP flap((char*)&packetRecv.buffer[packetRecv.bytesUsed],packetRecv.bytesAvailable-packetRecv.bytesUsed);
				if(!flap.len())
					break;
				flap_length+=FLAP_SIZE+flap.len();
				if(flap.cmp(0x01))
				{
					ppro->aim_send_cookie(ppro->hServerConn,ppro->seqno,ppro->COOKIE_LENGTH,ppro->COOKIE);//cookie challenge
					delete[] ppro->COOKIE;
					ppro->COOKIE=NULL;
					ppro->COOKIE_LENGTH=0;
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0001))
					{
						ppro->snac_supported_families(snac,ppro->hServerConn,ppro->seqno);
						ppro->snac_supported_family_versions(snac,ppro->hServerConn,ppro->seqno);
						ppro->snac_rate_limitations(snac,ppro->hServerConn,ppro->seqno);
						ppro->snac_service_redirect(snac);
						ppro->snac_error(snac);
					}
					else if(snac.cmp(0x0002))
					{
						ppro->snac_received_info(snac);
						ppro->snac_error(snac);
					}
					else if(snac.cmp(0x0003))
					{
						ppro->snac_user_online(snac);
						ppro->snac_user_offline(snac);
						ppro->snac_error(snac);
					}
					else if(snac.cmp(0x0004))
					{
						ppro->snac_icbm_limitations(snac,ppro->hServerConn,ppro->seqno);
						ppro->snac_message_accepted(snac);
						ppro->snac_received_message(snac,ppro->hServerConn,ppro->seqno);
						ppro->snac_typing_notification(snac);
						ppro->snac_error(snac);
						ppro->snac_busted_payload(snac);
					}
					else if(snac.cmp(0x0013))
					{
						ppro->snac_contact_list(snac,ppro->hServerConn,ppro->seqno);
						ppro->snac_list_modification_ack(snac);
						ppro->snac_error(snac);
					}
				}
				else if(flap.cmp(0x04))
				{
					ppro->offline_contacts();
					ppro->broadcast_status(ID_STATUS_OFFLINE);
					ppro->LOG("Connection Negotiation Thread Ending: Flap 0x04");
					SetEvent(ppro->hAvatarEvent);
					LeaveCriticalSection(&ppro->connectionMutex);
					return;
				}
			}
		}
	}
	ppro->offline_contacts();
	ppro->broadcast_status(ID_STATUS_OFFLINE);
	SetEvent(ppro->hAvatarEvent);
	ppro->LOG("Connection Negotiation Thread Ending: End of Thread");
	LeaveCriticalSection(&ppro->connectionMutex);
}

void __cdecl aim_mail_negotiation( CAimProto* ppro )
{
	NETLIBPACKETRECVER packetRecv;
	int recvResult=0;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	HANDLE hServerPacketRecver=NULL;
	hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)ppro->hMailConn, 2048 * 8);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = INFINITE;
	while(ppro->m_iStatus!=ID_STATUS_OFFLINE)
	{
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
				FLAP flap((char*)&packetRecv.buffer[packetRecv.bytesUsed],packetRecv.bytesAvailable-packetRecv.bytesUsed);
				if(!flap.len())
					break;
				flap_length+=FLAP_SIZE+flap.len();
				if(flap.cmp(0x01))
				{
					ppro->aim_send_cookie(ppro->hMailConn,ppro->mail_seqno,ppro->MAIL_COOKIE_LENGTH,ppro->MAIL_COOKIE);//cookie challenge
					delete[] ppro->MAIL_COOKIE;
					ppro->MAIL_COOKIE=NULL;
					ppro->MAIL_COOKIE_LENGTH=0;
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0001))
					{
						ppro->snac_supported_families(snac,ppro->hMailConn,ppro->mail_seqno);
						ppro->snac_supported_family_versions(snac,ppro->hMailConn,ppro->mail_seqno);
						ppro->snac_mail_rate_limitations(snac,ppro->hMailConn,ppro->mail_seqno);
						ppro->snac_error(snac);
					}
					else if(snac.cmp(0x0018))
					{
						ppro->snac_mail_response(snac);
					}
				}
				else if(flap.cmp(0x04))
				{
					Netlib_CloseHandle(hServerPacketRecver);
					Netlib_CloseHandle(ppro->hMailConn);
					ppro->hMailConn=0;
					ppro->LOG("Mail Server Connection has ended");
					return;
				}
			}
		}
	}
	ppro->LOG("Mail Server Connection has ended");
	Netlib_CloseHandle(hServerPacketRecver);
	Netlib_CloseHandle(ppro->hMailConn);
	ppro->hMailConn=0;
}

void __cdecl aim_avatar_negotiation( CAimProto* ppro )
{
	NETLIBPACKETRECVER packetRecv;
	int recvResult=0;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	HANDLE hServerPacketRecver=NULL;
	hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)ppro->hAvatarConn, 2048 * 8);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = 300000;//5 minutes connected
	for(;;)
	{
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
				FLAP flap((char*)&packetRecv.buffer[packetRecv.bytesUsed],packetRecv.bytesAvailable-packetRecv.bytesUsed);
				if(!flap.len())
					break;
				flap_length+=FLAP_SIZE+flap.len();
				if(flap.cmp(0x01))
				{
					ppro->aim_send_cookie(ppro->hAvatarConn,ppro->avatar_seqno,ppro->AVATAR_COOKIE_LENGTH,ppro->AVATAR_COOKIE);//cookie challenge
					delete[] ppro->AVATAR_COOKIE;
					ppro->AVATAR_COOKIE=NULL;
					ppro->AVATAR_COOKIE_LENGTH=0;
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0001))
					{
						ppro->snac_supported_families(snac,ppro->hAvatarConn,ppro->avatar_seqno);
						ppro->snac_supported_family_versions(snac,ppro->hAvatarConn,ppro->avatar_seqno);
						ppro->snac_avatar_rate_limitations(snac,ppro->hAvatarConn,ppro->avatar_seqno);
						ppro->snac_error(snac);
					}
					if(snac.cmp(0x0010))
					{
						ppro->snac_retrieve_avatar(snac);
					}
				}
				else if(flap.cmp(0x04))
				{
					Netlib_CloseHandle(hServerPacketRecver);
					ppro->hAvatarConn=0;
					ppro->LOG("Avatar Server Connection has ended");
					ppro->AvatarLimitThread=0;
					return;
				}
			}
		}
	}
	Netlib_CloseHandle(hServerPacketRecver);
	ppro->LOG("Avatar Server Connection has ended");
	ppro->hAvatarConn=0;
	ppro->AvatarLimitThread=0;
}
