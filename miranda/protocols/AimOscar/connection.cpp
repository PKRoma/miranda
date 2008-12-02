#include "aim.h"

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

HANDLE CAimProto::aim_connect(const char* server, unsigned short port, bool use_ssl)
{
	NETLIBOPENCONNECTION ncon = { 0 };
	ncon.cbSize = sizeof(ncon);
	ncon.szHost = server;
	ncon.wPort = port;
	ncon.timeout = 5;
	ncon.flags = NLOCF_V2;
    if (use_ssl) ncon.flags |= NLOCF_SSL;
	LOG("%s:%u", server, port);
	HANDLE con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) hNetlib, (LPARAM) & ncon);
	return con;
}

HANDLE CAimProto::aim_peer_connect(const char* ip, unsigned short port)
{ 
	NETLIBOPENCONNECTION ncon = { 0 };
	ncon.cbSize = sizeof(ncon);
	ncon.flags = NLOCF_V2;
	ncon.szHost = ip;
	ncon.wPort = port;
	ncon.timeout = 1;
	HANDLE con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) hNetlibPeer, (LPARAM) & ncon);
	return con;
}

void CAimProto::aim_connection_authorization(void)
{
	EnterCriticalSection(&connectionMutex);
	NETLIBPACKETRECVER packetRecv;
	DBVARIANT dbv;
	int recvResult=0;
	if (!getString(AIM_KEY_PW, &dbv))
	{
		CallService(MS_DB_CRYPT_DECODESTRING, lstrlenA(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
		password = strldup(dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	else
	{
		LeaveCriticalSection(&connectionMutex);
		return;
	}
	if (!getString(AIM_KEY_SN, &dbv))
	{
        if (username) delete[] username;
		username = strldup(dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	else
	{
		LeaveCriticalSection(&connectionMutex);
		return;
	}
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hServerConn, 2048 * 4);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = 5000;
	for(;;)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0)
		{
			LOG("Connection Closed: No Error? during Connection Authorization");
			break;
		}
		else if (recvResult < 0)
		{
			LOG("Connection Closed: Socket Error during Connection Authorization %d", WSAGetLastError());
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
					if( aim_send_connection_packet(hServerConn,seqno,flap.val())==0)//cookie challenge
						aim_authkey_request(hServerConn,seqno);//md5 authkey request
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0017))
					{
						snac_md5_authkey(snac,hServerConn,seqno);
						int authres=snac_authorization_reply(snac);
						if(authres==1)
						{
							delete[] password;
							Netlib_CloseHandle(hServerPacketRecver);
							LOG("Connection Authorization Thread Ending: Negotiation Beginning");
							LeaveCriticalSection(&connectionMutex);
							return;
						}
						else if (authres==2)
							goto exit;
					}
				}
				else if(flap.cmp(0x04))
				{
					LOG("Connection Authorization Thread Ending: Flap 0x04");
					goto exit;
				}
			}
		}
	}

exit:
	delete[] password;
	if (m_iStatus!=ID_STATUS_OFFLINE) broadcast_status(ID_STATUS_OFFLINE);
	Netlib_CloseHandle(hServerPacketRecver); hServerPacketRecver=NULL; 
	Netlib_CloseHandle(hServerConn); hServerConn=NULL;
	LOG("Connection Authorization Thread Ending: End of Thread");
	LeaveCriticalSection(&connectionMutex);
}

void __cdecl CAimProto::aim_protocol_negotiation( void* )
{
	EnterCriticalSection(&connectionMutex);
	NETLIBPACKETRECVER packetRecv;
	int recvResult=0;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hServerConn, 2048 * 8);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = DEFAULT_KEEPALIVE_TIMER*1000;	
	for(;;)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM)hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0)
		{
			LOG("Connection Closed: No Error during Connection Negotiation?");
			break;
		}
		else if (recvResult == SOCKET_ERROR)
		{
            if (WSAGetLastError() == ERROR_TIMEOUT)
                aim_keepalive(hServerConn,seqno);
            else
            {
			    LOG("Connection Closed: Socket Error during Connection Negotiation %d", WSAGetLastError());
			    break;
            }
		}
		else if(recvResult>0)
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
					aim_send_cookie(hServerConn,seqno,COOKIE_LENGTH,COOKIE);//cookie challenge
					delete[] COOKIE;
					COOKIE=NULL;
					COOKIE_LENGTH=0;
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0001))
					{
						snac_supported_families(snac,hServerConn,seqno);
						snac_supported_family_versions(snac,hServerConn,seqno);
						snac_rate_limitations(snac,hServerConn,seqno);
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
						snac_icbm_limitations(snac,hServerConn,seqno);
						snac_message_accepted(snac);
						snac_received_message(snac,hServerConn,seqno);
						snac_typing_notification(snac);
						snac_error(snac);
						snac_busted_payload(snac);
					}
					else if(snac.cmp(0x000A))
					{
						snac_email_search_results(snac);
						/* 
							If there's no match (error 0x14), AIM will pop up a message.
						    Since it's annoying and there's no other errors that'll get
						    generated, I just assume leave this commented out. It's here
							for consistency.
						*/
						//snac_error(snac); 
					}
					else if(snac.cmp(0x0013))
					{
						snac_contact_list(snac,hServerConn,seqno);
						snac_list_modification_ack(snac);
						snac_error(snac);
					}
				}
				else if(flap.cmp(0x04))
				{
					LOG("Connection Negotiation Thread Ending: Flap 0x04");
					goto exit;
				}
			}
		}
	}

exit:
	if (m_iStatus!=ID_STATUS_OFFLINE) broadcast_status(ID_STATUS_OFFLINE);
	Netlib_CloseHandle(hServerPacketRecver); hServerPacketRecver=NULL; 
	Netlib_CloseHandle(hServerConn); hServerConn=NULL;
	LOG("Connection Negotiation Thread Ending: End of Thread");
	LeaveCriticalSection(&connectionMutex);
	offline_contacts();
}

void __cdecl CAimProto::aim_mail_negotiation( void* )
{
	NETLIBPACKETRECVER packetRecv;
	int recvResult=0;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	HANDLE hServerPacketRecver;
	hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hMailConn, 2048 * 8);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = DEFAULT_KEEPALIVE_TIMER*1000;
	while(m_iStatus!=ID_STATUS_OFFLINE)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0)
		{
			break;
		}
		if (recvResult == SOCKET_ERROR)
		{
            if (WSAGetLastError() == ERROR_TIMEOUT)
                aim_keepalive(hMailConn, mail_seqno);
            else
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
					aim_send_cookie(hMailConn,mail_seqno,MAIL_COOKIE_LENGTH,MAIL_COOKIE);//cookie challenge
					delete[] MAIL_COOKIE;
					MAIL_COOKIE=NULL;
					MAIL_COOKIE_LENGTH=0;
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0001))
					{
						snac_supported_families(snac,hMailConn,mail_seqno);
						snac_supported_family_versions(snac,hMailConn,mail_seqno);
						snac_mail_rate_limitations(snac,hMailConn,mail_seqno);
						snac_error(snac);
					}
					else if(snac.cmp(0x0018))
						snac_mail_response(snac);
				}
				else if(flap.cmp(0x04))
					goto exit;
			}
		}
	}

exit:
	LOG("Mail Server Connection has ended");
	Netlib_CloseHandle(hServerPacketRecver);
	Netlib_CloseHandle(hMailConn);
	hMailConn=NULL;
}

void __cdecl CAimProto::aim_avatar_negotiation( void* )
{
	NETLIBPACKETRECVER packetRecv;
	int recvResult=0;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	HANDLE hServerPacketRecver=NULL;
	hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hAvatarConn, 2048 * 8);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = 300000;//5 minutes connected
	for(;;)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0)
			break;

		if (recvResult == SOCKET_ERROR)
			break;

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
					aim_send_cookie(hAvatarConn,avatar_seqno,AVATAR_COOKIE_LENGTH,AVATAR_COOKIE);//cookie challenge
					delete[] AVATAR_COOKIE;
					AVATAR_COOKIE=NULL;
					AVATAR_COOKIE_LENGTH=0;
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0001))
					{
						snac_supported_families(snac,hAvatarConn,avatar_seqno);
						snac_supported_family_versions(snac,hAvatarConn,avatar_seqno);
						snac_avatar_rate_limitations(snac,hAvatarConn,avatar_seqno);
						snac_error(snac);
					}
					if(snac.cmp(0x0010))
						snac_retrieve_avatar(snac);
				}
				else if(flap.cmp(0x04))
					goto exit;
			}
		}
	}

exit:
	Netlib_CloseHandle(hServerPacketRecver);
	Netlib_CloseHandle(hAvatarConn);
	hAvatarConn=NULL;
	ResetEvent(hAvatarEvent);
	LOG("Avatar Server Connection has ended");
}

void __cdecl CAimProto::aim_chatnav_negotiation( void* )
{
	NETLIBPACKETRECVER packetRecv;
	int recvResult=0;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	HANDLE hServerPacketRecver=NULL;
	hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hChatNavConn, 2048 * 8);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = 300000;//5 minutes connected
	for(;;)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0)
			break;

		if (recvResult == SOCKET_ERROR)
			break;

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
					aim_send_cookie(hChatNavConn,chatnav_seqno,CHATNAV_COOKIE_LENGTH,CHATNAV_COOKIE);//cookie challenge
					delete[] CHATNAV_COOKIE;
					CHATNAV_COOKIE=NULL;
					CHATNAV_COOKIE_LENGTH=0;
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0001))
					{
						snac_supported_families(snac,hChatNavConn,chatnav_seqno);
						snac_supported_family_versions(snac,hChatNavConn,chatnav_seqno);
						snac_chatnav_rate_limitations(snac,hChatNavConn,chatnav_seqno);
						snac_error(snac);
					}
					if(snac.cmp(0x000D))
					{
						snac_chatnav_info_response(snac,hChatNavConn,chatnav_seqno);
						snac_error(snac);
					}
				}
				else if(flap.cmp(0x04))
					goto exit;
			}
		}
	}

exit:
	Netlib_CloseHandle(hServerPacketRecver);
	Netlib_CloseHandle(hChatNavConn);
	hChatNavConn=NULL;
    ResetEvent(hChatNavEvent);
	LOG("Chat Navigation Server Connection has ended");
}

void __cdecl CAimProto::aim_chat_negotiation( void* param )
{
    chat_list_item *item = (chat_list_item*)param;
	NETLIBPACKETRECVER packetRecv;
	int recvResult=0;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	HANDLE hServerPacketRecver=NULL;
	hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)item->hconn, 2048 * 8);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = DEFAULT_KEEPALIVE_TIMER*1000;
	for(;;)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0)
			break;

		if (recvResult == SOCKET_ERROR)
        {
            if (WSAGetLastError() == ERROR_TIMEOUT)
                aim_keepalive(item->hconn,item->seqno);
            else
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
					aim_send_cookie(item->hconn,item->seqno,item->CHAT_COOKIE_LENGTH,item->CHAT_COOKIE);//cookie challenge
					delete[] item->CHAT_COOKIE;
					item->CHAT_COOKIE=NULL;
					item->CHAT_COOKIE_LENGTH=0;
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0001))
					{
						snac_supported_families(snac,item->hconn,item->seqno);
						snac_supported_family_versions(snac,item->hconn,item->seqno);
						snac_chat_rate_limitations(snac,item->hconn,item->seqno);
						snac_error(snac);

					}
					if(snac.cmp(0x000E))
					{
						snac_chat_received_message(snac, item);
						snac_chat_joined_left_users(snac, item);
						snac_error(snac);
					}
				}
				else if(flap.cmp(0x04))
					goto exit;
			}
		}
	}

exit:
	Netlib_CloseHandle(hServerPacketRecver);
	Netlib_CloseHandle(item->hconn);
    chat_leave(item->id);
    remove_chat_by_ptr(item);
	LOG("Chat Server Connection has ended");
}

void __cdecl CAimProto::aim_admin_negotiation( void* )
{
	NETLIBPACKETRECVER packetRecv;
	int recvResult=0;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	HANDLE hServerPacketRecver=NULL;
	hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hAdminConn, 2048 * 8);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = 300000;//5 minutes connected
	for(;;)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0)
			break;

		if (recvResult == SOCKET_ERROR)
			break;

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
					aim_send_cookie(hAdminConn,admin_seqno,ADMIN_COOKIE_LENGTH,ADMIN_COOKIE);//cookie challenge
					delete[] ADMIN_COOKIE;
					ADMIN_COOKIE=NULL;
					ADMIN_COOKIE_LENGTH=0;
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0001))
					{
						snac_supported_families(snac,hAdminConn,admin_seqno);
						snac_supported_family_versions(snac,hAdminConn,admin_seqno);
						snac_admin_rate_limitations(snac,hAdminConn,admin_seqno);
						snac_error(snac);
					}
					if(snac.cmp(0x0007))
					{
						snac_admin_account_infomod(snac);
						snac_admin_account_confirm(snac);
						snac_error(snac);
					}
				}
				else if(flap.cmp(0x04))
					goto exit;
			}
		}
	}

exit:
	Netlib_CloseHandle(hServerPacketRecver);
	Netlib_CloseHandle(hAdminConn);
	hAdminConn=NULL;
    ResetEvent(hAdminEvent);
	LOG("Admin Server Connection has ended");
}
