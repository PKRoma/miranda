#include "file.h"
#if _MSC_VER
#pragma warning( disable: 4706 )
#endif
void sending_file(HANDLE hContact, HANDLE hNewConnection)
{
	DBVARIANT dbv;
	char* file;
	unsigned long size;
	if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_FN, &dbv))
	{
		file=strldup(dbv.pszVal,lstrlen(dbv.pszVal));
		DBFreeVariant(&dbv);
		size=DBGetContactSettingDword(hContact, AIM_PROTOCOL_NAME, AIM_KEY_FS, 0);
		if(!size)
			return;
	}
	else 
		return;
	char cookie[8];
	read_cookie(hContact,cookie);
	oft2 ft;
	ZeroMemory(&ft,sizeof(oft2));
	memcpy(ft.protocol_version,"OFT2",4);
	ft.length=0x0001;
	ft.type=0x0101;
	memcpy(ft.icbm_cookie,cookie,8);
	ft.total_files=_htons(1);
	ft.num_files_left=_htons(1);
	ft.total_parts=_htons(1);
	ft.parts_left=_htons(1);
	ft.total_size=_htonl(size);
	ft.size=_htonl(size);
	ft.mod_time=0;
	ft.checksum=_htonl(aim_oft_checksum_file(file));
	ft.recv_RFchecksum=0x0000FFFF;
	ft.RFchecksum=0x0000FFFF;
	ft.recv_checksum=0x0000FFFF;
	memcpy(ft.idstring,"Cool FileXfer",13);
	ft.flags=0x20;
	ft.list_name_offset=0x1c;
	ft.list_size_offset=0x11;
	char* pszFile = strrchr(file, '\\');
	pszFile++;
	memcpy(ft.filename,pszFile,lstrlen(pszFile));
	char* buf = (char*)&ft;
	if(Netlib_Send(hNewConnection,buf,sizeof(oft2),0)==SOCKET_ERROR)
	{
		ProtoBroadcastAck(AIM_PROTOCOL_NAME,hContact, ACKTYPE_FILE, ACKRESULT_FAILED,hContact,0);
		return;
	}
	//start listen for packets stuff
	int recvResult=0;
	NETLIBPACKETRECVER packetRecv;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	HANDLE hServerPacketRecver=NULL;
	hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hNewConnection, 2048 * 4);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = 100*DBGetContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_GP, DEFAULT_GRACE_PERIOD);
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
				ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_FAILED,hContact,0);
                break;
            }
        if (recvResult == SOCKET_ERROR)
			{
				ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_FAILED,hContact,0);
                break;
            }
		if(recvResult>0)
		{	
			if(recvResult==256)
			{
				packetRecv.bytesUsed=256;
				oft2* recv_ft=(oft2*)&packetRecv.buffer[0];
				unsigned short type=_htons(recv_ft->type);
				if(type==0x0202)
				{
					FILE *fd= fopen(file, "rb");
					if (fd)
					{
						PROTOFILETRANSFERSTATUS pfts;
						memset(&pfts, 0, sizeof(PROTOFILETRANSFERSTATUS));
						pfts.currentFile=pszFile;
						pfts.currentFileNumber=0;
						pfts.currentFileProgress=0;
						pfts.currentFileSize=_htonl(ft.size);
						pfts.currentFileTime=0;
						pfts.files=NULL;
						pfts.hContact=hContact;
						pfts.sending=1;
						pfts.totalBytes=size;
						pfts.totalFiles=1;
						pfts.totalProgress=0;
						ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_DATA,hContact, (LPARAM) & pfts);
						int bytes;
						unsigned char buffer[1024];
						while ((bytes = fread(buffer, 1, 1024, fd)))
						{
							Netlib_Send(hNewConnection,(char*)&buffer,bytes,0);
							pfts.currentFileProgress+=bytes;
							pfts.totalProgress+=bytes;
							ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_DATA,hContact, (LPARAM) & pfts);
						}	
						fclose(fd);
					}
					ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS,hContact,0);
					delete[] file;
					return;
				}
				else if(type==0x0204)
				{
					ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS,hContact,0);
					delete[] file;
					return;
				}
			}
		}
	}
}
#if _MSC_VER
#pragma warning( default: 4706 )
#endif
void receiving_file(HANDLE hContact, HANDLE hNewConnection)
{
	bool accepted_file=0;
	DBVARIANT dbv;
	char* file=0;
	FILE *fd=0;
	oft2 ft;
	PROTOFILETRANSFERSTATUS pfts;
	memset(&pfts, 0, sizeof(PROTOFILETRANSFERSTATUS));
	pfts.currentFileNumber=0;
	pfts.currentFileProgress=0;
	pfts.currentFileTime=0;
	pfts.files=NULL;
	pfts.hContact=hContact;
	pfts.sending=0;
	pfts.totalFiles=1;
	pfts.totalProgress=0;
	unsigned long size;
	if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_FN, &dbv))
	{
		file=strldup(dbv.pszVal,lstrlen(dbv.pszVal));
		DBFreeVariant(&dbv);
	}
	//start listen for packets stuff
	int recvResult=0;
	NETLIBPACKETRECVER packetRecv;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	HANDLE hServerPacketRecver=NULL;
	hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hNewConnection, 2048 * 4);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = 100*DBGetContactSettingWord(NULL, AIM_PROTOCOL_NAME, AIM_KEY_GP, DEFAULT_GRACE_PERIOD);
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
				ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_FAILED,hContact,0);
                break;
            }
        if (recvResult == SOCKET_ERROR)
			{
				ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_FAILED,hContact,0);
                break;
            }
		if(recvResult>0)
		{	
			if(!accepted_file)
			{
				if(recvResult==256)
				{
					packetRecv.bytesUsed=256;
					oft2* recv_ft=(oft2*)&packetRecv.buffer[0];
					unsigned short type=_htons(recv_ft->type);
					if(type==0x0101)
					{
						memcpy(&ft,recv_ft,sizeof(oft2));
						char cookie[8];
						read_cookie(hContact,cookie);
						memcpy(ft.icbm_cookie,cookie,8);
						ft.type=0x0202;//turning it to acknowledge type and sending it back;)
						size=_htonl(recv_ft->size);
						pfts.currentFileSize=size;
						pfts.totalBytes=size;
						char* buf = (char*)&ft;
						Netlib_Send(hNewConnection,buf,sizeof(oft2),0);
						file=renew(file,lstrlen(file)+1,lstrlen((char*)&ft.filename)+1);
						lstrcat(file,(char*)&ft.filename);
						pfts.currentFile=file;
						fd = fopen(file, "wb");
						if(!fd)
						{
							delete[] file;
							return;
						}
						else
						{
							accepted_file=1;
						}
					}
				}
			}
			else
			{
				packetRecv.bytesUsed=packetRecv.bytesAvailable;
				fwrite(packetRecv.buffer,1,packetRecv.bytesAvailable,fd);
				pfts.currentFileProgress+=packetRecv.bytesAvailable;
				pfts.totalProgress+=packetRecv.bytesAvailable;
				ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_DATA,hContact, (LPARAM) & pfts);
				if(pfts.totalBytes==pfts.currentFileProgress)
				{

					ft.type=_htons(0x0204);
					Netlib_Send(hNewConnection,(char*)&ft,sizeof(oft2),0);
					ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS,hContact,0);
					break;
				}
			}
		}
	}
	if(accepted_file)
		fclose(fd);
	delete[] file;
}
