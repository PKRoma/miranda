#include "proxy.h"
void __cdecl aim_proxy_helper(HANDLE hContact)
{
	HANDLE Connection=(HANDLE)DBGetContactSettingDword(hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH,0);//really just the proxy handle not a dc handle
	int stage= DBGetContactSettingByte(hContact,AIM_PROTOCOL_NAME,AIM_KEY_PS,0);
	int sender= DBGetContactSettingByte(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FT,0);
	if(Connection)
	{
		char cookie[8];
		read_cookie(hContact,cookie);
		DBVARIANT dbv;//CHECK FOR FREE VARIANT!
		if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
		{
			if(stage==1&&!sender||stage==2&&sender||stage==3&&!sender)
			{
				unsigned short port_check=DBGetContactSettingWord(hContact,AIM_PROTOCOL_NAME,AIM_KEY_PC,0);
				if(proxy_initialize_recv(Connection,dbv.pszVal,cookie,port_check))
					return;//error
			}
			else if(stage==1&&sender||stage==2&&!sender||stage==3&&sender)
			{
				if(proxy_initialize_send(Connection,dbv.pszVal,cookie))
					return;//error
			}
			//start listen for packets stuff
			int recvResult=0;
			NETLIBPACKETRECVER packetRecv;
			ZeroMemory(&packetRecv, sizeof(packetRecv));
			HANDLE hServerPacketRecver=NULL;
			hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)Connection, 2048 * 4);
			packetRecv.cbSize = sizeof(packetRecv);
			packetRecv.dwTimeout = INFINITE;
			while(1)
			{
				recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) hServerPacketRecver, (LPARAM) & packetRecv);
				if (recvResult == 0)
				{
					ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_FAILED,(HANDLE)hContact,0);
					break;
				}
				if (recvResult == SOCKET_ERROR)
				{
					ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_FAILED,(HANDLE)hContact,0);
					break;
				}
				if(recvResult>0)
				{
					unsigned short* length=(unsigned short*)&packetRecv.buffer[0];
					*length=htons(*length);
					packetRecv.bytesUsed=*length+2;
					unsigned short* type=(unsigned short*)&packetRecv.buffer[4];
					*type=htons(*type);
					if(*type==0x0001)
					{
						unsigned short* error=(unsigned short*)&packetRecv.buffer[12];
						*error=htons(*error);
						if(*error==0x000D)
						{
							MessageBox( NULL, "Bad Request", Translate("Proxy Server File Transfer Error"), MB_OK );
						}
						else if(*error==0x0010)
						{
							MessageBox( NULL, "Initial Request Timed Out.", Translate("Proxy Server File Transfer Error"), MB_OK );
						}
						else if(*error==0x001A)
						{
							MessageBox( NULL, "Accept Period Timed Out.", Translate("Proxy Server File Transfer Error"), MB_OK );
						}
						else if(*error==0x000e)
						{
							MessageBox( NULL, "Incorrect command syntax.", Translate("Proxy Server File Transfer Error"), MB_OK );
						}
						else if(*error==0x0016)
						{
							MessageBox( NULL, "Unknown command issued.", Translate("Proxy Server File Transfer Error"), MB_OK );
						}
					}
					else if(*type==0x0003)
					{
						unsigned short* port=(unsigned short*)&packetRecv.buffer[12];
						unsigned long* ip=(unsigned long*)&packetRecv.buffer[14];
						*port=htons(*port);
						*ip=htonl(*ip);
						DBVARIANT dbv;
						if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
						{
							if(stage==1&&sender)
							{
								char* sn=strldup(dbv.pszVal,strlen(dbv.pszVal));
								char vip[20];
								char *file, *descr;
								unsigned long size;
								long_ip_to_char_ip(*ip,vip);
								DBWriteContactSettingString(hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP,vip);
								DBVARIANT dbv;
								if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_FN, &dbv))
								{
									file=strldup(dbv.pszVal,strlen(dbv.pszVal));
									DBFreeVariant(&dbv);
									DBVARIANT dbv;
									if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_FD, &dbv))
									{
										descr=strldup(dbv.pszVal,strlen(dbv.pszVal));
										DBFreeVariant(&dbv);
										size=DBGetContactSettingDword(hContact, AIM_PROTOCOL_NAME, AIM_KEY_FS, 0);
										if(!size)
											return;
									}
									else
										return;
								}
								else 
									return;
								char* pszfile = strrchr(file, '\\');
								pszfile++;
								aim_send_file_proxy(sn,cookie,pszfile,size,descr,*ip,*port);
								delete[] file;
								delete[] sn;
								delete[] descr;
							}
							else if(stage==2&&!sender)
								aim_file_proxy_request(dbv.pszVal,cookie,0x02,*ip,*port);
							else if(stage==3&&sender)
								aim_file_proxy_request(dbv.pszVal,cookie,0x03,*ip,*port);
							DBFreeVariant(&dbv);
						}
					}
					else if(*type==0x0005)
					{
						DBVARIANT dbv;
						if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_IP, &dbv))
						{
							unsigned long ip=char_ip_to_long_ip(dbv.pszVal);
							DBWriteContactSettingDword(NULL,FILE_TRANSFER_KEY,dbv.pszVal,(DWORD)hContact);
							DBFreeVariant(&dbv);
							if(stage==1&&!sender||stage==2&&sender||stage==3&&!sender)
							{
								if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
								{
									aim_accept_file(dbv.pszVal,cookie);
									DBFreeVariant(&dbv);
								}
							}
							aim_direct_connection_initiated(Connection, ip,NULL);
						}
					}
				}
			}
			DBFreeVariant(&dbv);
		}
	}
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_FT);
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_DH);
	DBDeleteContactSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_IP);
}
int proxy_initialize_send(HANDLE connection,char* sn, char* cookie)
{
	char msg_frag[MSG_LEN*2];
	char sn_length=strlen(sn);
	unsigned short length = htons(39+sn_length);
	char* clength =(char*)&length;
	memcpy(msg_frag,clength,2);
	memcpy(&msg_frag[2],"\x04\x4a\0\x02\0\0\0\0\0\0",10);
	memcpy(&msg_frag[12],(char*)&sn_length,1);
	memcpy(&msg_frag[13],sn,sn_length);
	memcpy(&msg_frag[13+sn_length],cookie,8);
	memcpy(&msg_frag[21+sn_length],"\0\x01\0\x10",4);
	memcpy(&msg_frag[25+sn_length],AIM_CAP_SEND_FILES,sizeof(AIM_CAP_SEND_FILES));
	if(Netlib_Send(connection,msg_frag,(24+sn_length+sizeof(AIM_CAP_SEND_FILES)),0)==SOCKET_ERROR)
		return -1;
	return 0;
}
int proxy_initialize_recv(HANDLE connection,char* sn, char* cookie,unsigned short port_check)
{
	char msg_frag[MSG_LEN*2];
	char sn_length=strlen(sn);
	unsigned short length = htons(41+sn_length);
	char* clength =(char*)&length;
	memcpy(msg_frag,clength,2);
	memcpy(&msg_frag[2],"\x04\x4a\0\x04\0\0\0\0\0\0",10);
	memcpy(&msg_frag[12],(char*)&sn_length,1);
	memcpy(&msg_frag[13],sn,sn_length);
	port_check=htons(port_check);
	memcpy(&msg_frag[13+sn_length],(char*)&port_check,2);
	memcpy(&msg_frag[15+sn_length],cookie,8);
	memcpy(&msg_frag[23+sn_length],"\0\x01\0\x10",4);
	memcpy(&msg_frag[27+sn_length],AIM_CAP_SEND_FILES,sizeof(AIM_CAP_SEND_FILES));
	if(Netlib_Send(connection,msg_frag,(26+sn_length+sizeof(AIM_CAP_SEND_FILES)),0)==SOCKET_ERROR)
		return -1;
	return 0;
}
