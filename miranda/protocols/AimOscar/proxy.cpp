/*
Plugin of Miranda IM for communicating with users of the AIM protocol.
Copyright (c) 2008-2009 Boris Krasnovskiy
Copyright (C) 2005-2006 Aaron Myles Landwehr

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "aim.h"
#include "proxy.h"

void __cdecl CAimProto::aim_proxy_helper( void* hContact )
{
	HANDLE Connection=(HANDLE)getDword( hContact, AIM_KEY_DH, 0 );//really just the proxy handle not a dc handle
	int stage = getByte( hContact, AIM_KEY_PS, 0 );
	int sender = getByte( hContact, AIM_KEY_FT, 0 );
	if ( Connection ) {
		char cookie[8];
		read_cookie(hContact,cookie);
		DBVARIANT dbv;//CHECK FOR FREE VARIANT!
		if ( !getString(AIM_KEY_SN, &dbv)) {
			if ( stage == 1 && !sender || stage == 2 && sender || stage==3 && !sender ) {
				unsigned short port_check = (unsigned short)getWord( hContact, AIM_KEY_PC, 0 );
				if ( proxy_initialize_recv( Connection, dbv.pszVal, cookie, port_check )) {
					DBFreeVariant(&dbv);
					return;//error
				}
			}
			else if(stage==1&&sender||stage==2&&!sender||stage==3&&sender)
			{
				if(proxy_initialize_send(Connection,dbv.pszVal,cookie))
				{
					DBFreeVariant(&dbv);
					return;//error
				}
			}
			DBFreeVariant(&dbv);
			//start listen for packets stuff
			int recvResult=0;
			NETLIBPACKETRECVER packetRecv;
			ZeroMemory(&packetRecv, sizeof(packetRecv));
			HANDLE hServerPacketRecver;
			hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)Connection, 2048 * 4);
			packetRecv.cbSize = sizeof(packetRecv);
			packetRecv.dwTimeout = INFINITE;
			for(;;)
			{
				recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) hServerPacketRecver, (LPARAM) & packetRecv);
				if ( recvResult == 0) {
					sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_FAILED,(HANDLE)hContact,0);
					break;
				}
				if ( recvResult == SOCKET_ERROR) {
					sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_FAILED,(HANDLE)hContact,0);
					break;
				}
				if ( recvResult > 0 ) {
					unsigned short* length=(unsigned short*)&packetRecv.buffer[0];
					*length=_htons(*length);
					packetRecv.bytesUsed=*length+2;
					unsigned short* type=(unsigned short*)&packetRecv.buffer[4];
					*type=_htons(*type);
					if(*type==0x0001)
					{
						unsigned short* error=(unsigned short*)&packetRecv.buffer[12];
						*error=_htons(*error);
						if(*error==0x000D)
						{
                            ShowPopup("Proxy Server File Transfer Error: Bad Request.", ERROR_POPUP);
						}
						else if(*error==0x0010)
						{
                            ShowPopup("Proxy Server File Transfer Error: Initial Request Timed Out.", ERROR_POPUP);
						}
						else if(*error==0x001A)
						{
                            ShowPopup("Proxy Server File Transfer Error: Accept Period Timed Out.", ERROR_POPUP);
						}
						else if(*error==0x000e)
						{
                            ShowPopup("Proxy Server File Transfer Error: Incorrect command syntax.", ERROR_POPUP);
						}
						else if(*error==0x0016)
						{
                            ShowPopup("Proxy Server File Transfer Error: Unknown command issued.", ERROR_POPUP);
						}
					}
					else if(*type==0x0003)
					{
						unsigned short* port=(unsigned short*)&packetRecv.buffer[12];
						unsigned long* ip=(unsigned long*)&packetRecv.buffer[14];
						*port=_htons(*port);
						*ip=_htonl(*ip);
						
                        char* sn = getSetting(hContact, AIM_KEY_SN);
						if (sn) 
                        {
							if ( stage == 1 && sender ) {
								LOG("Stage 1 Proxy ft and we are the sender.");
								char vip[20];
								long_ip_to_char_ip(*ip,vip);
								setString( hContact, AIM_KEY_IP, vip );

                                char *file = getSetting(hContact, AIM_KEY_FN);
                                if (file)
                                {
                                    char *descr = getSetting(hContact, AIM_KEY_FD);
									if (descr) 
                                    {
										unsigned size = getDword(hContact, AIM_KEY_FS, 0);
                                        char* pszfile = strrchr(file, '\\');
								        pszfile++;
								        aim_send_file(hServerConn,seqno,sn,cookie,*ip,*port,1,1,pszfile,size,descr);
								        mir_free(descr);
									}
							        mir_free(file);
								}
							}
							else if(stage==2&&!sender)
							{
								LOG("Stage 2 Proxy ft and we are not the sender.");
								aim_file_proxy_request(hServerConn,seqno,dbv.pszVal,cookie,0x02,*ip,*port);
							}
							else if(stage==3&&sender)
							{
								LOG("Stage 3 Proxy ft and we are the sender.");
								aim_file_proxy_request(hServerConn,seqno,dbv.pszVal,cookie,0x03,*ip,*port);
							}
						}
						mir_free(sn);
					}
					else if(*type==0x0005) {
						if ( !getString(hContact, AIM_KEY_IP, &dbv )) {
							unsigned long ip=char_ip_to_long_ip(dbv.pszVal);
							DBWriteContactSettingDword(NULL,FILE_TRANSFER_KEY,dbv.pszVal,(DWORD)hContact);
							DBFreeVariant(&dbv);
							if ( stage == 1 && !sender || stage == 2 && sender || stage == 3 && !sender ) {
								if ( !getString(hContact, AIM_KEY_SN, &dbv)) {
									aim_file_ad(hServerConn,seqno,dbv.pszVal,cookie,false);
									DBFreeVariant(&dbv);
								}
							}
							aim_direct_connection_initiated(Connection, ip, this);
						}
					}
				}
			}
            Netlib_CloseHandle(hServerPacketRecver);
		}
	}

	deleteSetting(hContact, AIM_KEY_FT);
	deleteSetting(hContact, AIM_KEY_DH);
	deleteSetting(hContact, AIM_KEY_IP);
}

int proxy_initialize_send(HANDLE connection,char* sn, char* cookie)
{
	char sn_length=(char)lstrlenA(sn);
	unsigned short length = _htons(39+sn_length);
	char* msg_frag= (char*)alloca(25+sn_length+sizeof(AIM_CAP_FILE_TRANSFER));
	memcpy(msg_frag,(char*)&length,2);
	memcpy(&msg_frag[2],"\x04\x4a\0\x02\0\0\0\0\0\0",10);
	memcpy(&msg_frag[12],(char*)&sn_length,1);
	memcpy(&msg_frag[13],sn,sn_length);
	memcpy(&msg_frag[13+sn_length],cookie,8);
	memcpy(&msg_frag[21+sn_length],"\0\x01\0\x10",4);
	memcpy(&msg_frag[25+sn_length],AIM_CAP_FILE_TRANSFER,sizeof(AIM_CAP_FILE_TRANSFER));
    return Netlib_Send(connection,msg_frag,(26+sn_length+sizeof(AIM_CAP_FILE_TRANSFER)),0) >= 0 ? 0 : -1; 
}

int proxy_initialize_recv(HANDLE connection,char* sn, char* cookie,unsigned short port_check)
{
	char sn_length=(char)lstrlenA(sn);
	unsigned short length = _htons(41+sn_length);
	char* clength =(char*)&length;
	char* msg_frag= (char*)alloca(27+sn_length+sizeof(AIM_CAP_FILE_TRANSFER));
	memcpy(msg_frag,clength,2);
	memcpy(&msg_frag[2],"\x04\x4a\0\x04\0\0\0\0\0\0",10);
	memcpy(&msg_frag[12],(char*)&sn_length,1);
	memcpy(&msg_frag[13],sn,sn_length);
	port_check=_htons(port_check);
	memcpy(&msg_frag[13+sn_length],(char*)&port_check,2);
	memcpy(&msg_frag[15+sn_length],cookie,8);
	memcpy(&msg_frag[23+sn_length],"\0\x01\0\x10",4);
	memcpy(&msg_frag[27+sn_length],AIM_CAP_FILE_TRANSFER,sizeof(AIM_CAP_FILE_TRANSFER));
    return Netlib_Send(connection,msg_frag,(26+sn_length+sizeof(AIM_CAP_FILE_TRANSFER)),0) >= 0 ? 0 : -1; 
}
