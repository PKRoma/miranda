#include "thread.h"
void __cdecl forkthread_r(struct FORK_ARG *fa)
{	
	void (*callercode)(void*) = fa->threadcode;
	void *arg = fa->arg;
	CallService(MS_SYSTEM_THREAD_PUSH, 0, 0);
	SetEvent(fa->hEvent);
	__try {
		callercode(arg);
	} __finally {
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
		CallService(MS_SYSTEM_THREAD_POP, 0, 0);
	} 
	return;
}

unsigned long ForkThread(pThreadFunc threadcode,void *arg)
{
	struct FORK_ARG fa;
	fa.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	fa.threadcode = threadcode;
	fa.arg = arg;
	unsigned long rc = _beginthread((pThreadFunc)forkthread_r,0, &fa);
	if ((unsigned long) -1L != rc) {
		WaitForSingleObject(fa.hEvent, INFINITE);
	}
	CloseHandle(fa.hEvent);
	return rc;
}
void __cdecl aim_keepalive_thread(void* /*fa*/)
{
	HANDLE hKeepAliveEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	#if _MSC_VER
	#pragma warning( disable: 4127)
	#endif
	while(1)
	{
		#if _MSC_VER
		#pragma warning( default: 4127)
		#endif
		DWORD dwWait = WaitForSingleObjectEx(hKeepAliveEvent, 1000*DBGetContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_KA, DEFAULT_KEEPALIVE_TIMER), TRUE);
		if (dwWait == WAIT_OBJECT_0) break; // we should end
		else if (dwWait == WAIT_TIMEOUT)
		{
			if (conn.state==1)
				aim_keepalive(conn.hServerConn,conn.seqno);
		}
		//else if (dwWait == WAIT_IO_COMPLETION)
		// Possible shutdown in progress
		if (Miranda_Terminated()) break;
	}
	CloseHandle(hKeepAliveEvent);
	hKeepAliveEvent = NULL;
}
/*void message_box_thread(char* data)
{
	MessageBox( NULL, Translate(data), AIM_PROTOCOL_NAME, MB_OK );
	delete[] data;
}*/
void set_status_thread(int status)
{
	if(conn.shutting_down)
		return;
	EnterCriticalSection(&statusMutex);
	start_connection(status);
	if(conn.state==1)
		switch(status)
		{
			case ID_STATUS_OFFLINE:
				{
					broadcast_status(ID_STATUS_OFFLINE);
					break;
				}
			case ID_STATUS_ONLINE:
			case ID_STATUS_FREECHAT:
				{
					broadcast_status(ID_STATUS_ONLINE);
					aim_set_away(conn.hServerConn,conn.seqno,NULL);//unset away message
					aim_set_invis(conn.hServerConn,conn.seqno,AIM_STATUS_ONLINE,AIM_STATUS_NULL);//online not invis	
					break;
				}
			case ID_STATUS_INVISIBLE:
				{
					broadcast_status(status);
					aim_set_invis(conn.hServerConn,conn.seqno,AIM_STATUS_INVISIBLE,AIM_STATUS_NULL);
					break;
				}
			case ID_STATUS_AWAY:
			case ID_STATUS_OUTTOLUNCH:
			case ID_STATUS_NA:
			case ID_STATUS_DND:
			case ID_STATUS_OCCUPIED:
			case ID_STATUS_ONTHEPHONE:
				{
					start_connection(ID_STATUS_AWAY);// if not started
					if(conn.status!=ID_STATUS_AWAY)
					{
						assign_modmsg((char*)&DEFAULT_AWAY_MSG);
						broadcast_status(ID_STATUS_AWAY);
						aim_set_away(conn.hServerConn,conn.seqno,conn.szModeMsg);//set actual away message
						aim_set_invis(conn.hServerConn,conn.seqno,AIM_STATUS_AWAY,AIM_STATUS_NULL);//away not invis
					}
					//see SetAwayMsg for status away
					break;
				}
		}
	LeaveCriticalSection(&statusMutex);
}
/*void contact_setting_changed_thread(char* data)
{
	HANDLE* hContact=(HANDLE*)data;
	char* group = data + sizeof(HANDLE);
	add_contact_to_group(*hContact,DBGetContactSettingWord(NULL, GROUP_ID_KEY,group,0),group);
}*/
void accept_file_thread(char* data)//buddy sending file
{
	char *szDesc, *szFile, *local_ip, *verified_ip, *proxy_ip,* sn;
	HANDLE* hContact=(HANDLE*)data;
	szFile = data + sizeof(HANDLE);
    szDesc = szFile + lstrlen(szFile) + 1;
	local_ip = szDesc + lstrlen(szDesc) + 1;
	verified_ip = local_ip + lstrlen(local_ip) + 1;
	proxy_ip = verified_ip + lstrlen(verified_ip) + 1;
	DBVARIANT dbv;
	if (!DBGetContactSetting(*hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
	{
		sn= strldup(dbv.pszVal,lstrlen(dbv.pszVal));
		DBFreeVariant(&dbv);
	}
	else
		return;
	int peer_force_proxy=DBGetContactSettingByte(*hContact, AIM_PROTOCOL_NAME, AIM_KEY_FP, 0);
	int force_proxy=DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_FP, 0);
	unsigned short port=(unsigned short)DBGetContactSettingWord(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_PC,0);
	if(peer_force_proxy)//peer is forcing proxy
	{
		HANDLE hProxy=aim_peer_connect(proxy_ip,5190);
		if(hProxy)
		{
			DBWriteContactSettingByte(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_PS,1);
			DBWriteContactSettingDword(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,(DWORD)hProxy);//not really a direct connection
			DBWriteContactSettingWord(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_PC,port);//needed to verify the proxy connection as legit
			DBWriteContactSettingString(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,proxy_ip);
			ForkThread(aim_proxy_helper,*hContact);
		}
	}
	else if(force_proxy)//we are forcing a proxy
	{
		HANDLE hProxy=aim_connect("ars.oscar.aol.com:5190");
		if(hProxy)
		{
			DBWriteContactSettingByte(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_PS,2);
			DBWriteContactSettingDword(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,(DWORD)hProxy);//not really a direct connection
			DBWriteContactSettingString(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,"ars.oscar.aol.com:5190");
			ForkThread(aim_proxy_helper,*hContact);
		}
	}
	else
	{
		char cookie[8];
		read_cookie(*hContact,cookie);
		HANDLE hDirect =aim_peer_connect(verified_ip,port);
		if(hDirect)
		{
			aim_accept_file(conn.hServerConn,conn.seqno,sn,cookie);
			DBWriteContactSettingDword(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,(DWORD)hDirect);
			DBWriteContactSettingString(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,verified_ip);
			ForkThread(aim_dc_helper,*hContact);
		}
		hDirect=aim_peer_connect(local_ip,port);
		if(hDirect)
		{
			aim_accept_file(conn.hServerConn,conn.seqno,sn,cookie);
			DBWriteContactSettingDword(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,(DWORD)hDirect);
			DBWriteContactSettingString(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,local_ip);
			ForkThread(aim_dc_helper,*hContact);
		}
		else
		{
			DBWriteContactSettingString(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,verified_ip);
			aim_file_redirected_request(conn.hServerConn,conn.seqno,sn,cookie);
		}
	}
	delete[] sn;
}

void redirected_file_thread(char* blob)//we are sending file
{
	HANDLE* hContact=(HANDLE*)blob;
	char* icbm_cookie=(char*)blob+sizeof(HANDLE);
	char* sn=(char*)blob+sizeof(HANDLE)+8;
	char* local_ip=(char*)blob+sizeof(HANDLE)+8+lstrlen(sn)+1;
	char* verified_ip=(char*)blob+sizeof(HANDLE)+8+lstrlen(sn)+lstrlen(local_ip)+2;
	char* proxy_ip=(char*)blob+sizeof(HANDLE)+8+lstrlen(sn)+lstrlen(local_ip)+lstrlen(verified_ip)+3;
	unsigned short* port=(unsigned short*)&proxy_ip[lstrlen(proxy_ip)+1];
	bool* force_proxy=(bool*)&proxy_ip[lstrlen(proxy_ip)+1+sizeof(unsigned short)];
	ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_CONNECTING,hContact, 0);
	if(!*force_proxy)
	{
		HANDLE hDirect =aim_peer_connect(verified_ip,*port);
		if(hDirect)
		{
			aim_accept_file(conn.hServerConn,conn.seqno,sn,icbm_cookie);
			DBWriteContactSettingDword(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,(DWORD)hDirect);
			DBWriteContactSettingString(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,verified_ip);
			ForkThread(aim_dc_helper,*hContact);
		}
		else
		{
			hDirect=aim_peer_connect(local_ip,*port);	
			if(hDirect)
			{
				aim_accept_file(conn.hServerConn,conn.seqno,sn,icbm_cookie);
				DBWriteContactSettingDword(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,(DWORD)hDirect);
				DBWriteContactSettingString(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,local_ip);
				ForkThread(aim_dc_helper,*hContact);
			}
			else//stage 3 proxy
			{
				HANDLE hProxy=aim_connect("ars.oscar.aol.com:5190");
				if(hProxy)
				{
					DBWriteContactSettingByte(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_PS,3);
					DBWriteContactSettingDword(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,(DWORD)hProxy);//not really a direct connection
					DBWriteContactSettingString(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,verified_ip);
					ForkThread(aim_proxy_helper,*hContact);
				}
			}
		}
	}
	else//stage 2 proxy
	{
		HANDLE hProxy=aim_peer_connect(proxy_ip,5190);
		if(hProxy)
		{
			DBWriteContactSettingByte(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_PS,2);
			DBWriteContactSettingDword(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,(DWORD)hProxy);//not really a direct connection
			DBWriteContactSettingWord(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_PC,*port);//needed to verify the proxy connection as legit
			DBWriteContactSettingString(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,proxy_ip);
			ForkThread(aim_proxy_helper,*hContact);
		}
	}
	delete[] blob;
}
void proxy_file_thread(char* blob)//buddy sending file here
{
	//stage 3 proxy
	HANDLE* hContact=(HANDLE*)blob;
	char* proxy_ip=(char*)blob+sizeof(HANDLE);
	unsigned short* port=(unsigned short*)&proxy_ip[lstrlen(proxy_ip)+1];
	HANDLE hProxy=aim_peer_connect(proxy_ip,5190);
	if(hProxy)
	{
		DBWriteContactSettingByte(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_PS,3);
		DBWriteContactSettingDword(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,(DWORD)hProxy);//not really a direct connection
		DBWriteContactSettingWord(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_PC,*port);//needed to verify the proxy connection as legit
		DBWriteContactSettingString(*hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,proxy_ip);
		ForkThread(aim_proxy_helper,*hContact);
	}
	delete[] blob;
}
