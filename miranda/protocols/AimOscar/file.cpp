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
#include "file.h"

#if _MSC_VER
	#pragma warning( disable: 4706 )
#endif

void CAimProto::sending_file(HANDLE hContact, HANDLE hNewConnection)
{
	LOG("P2P: Entered file sending thread.");
	DBVARIANT dbv;
	char *file, *wd;
	int file_start_point=0;
	unsigned long size;
	if (!getString(hContact, AIM_KEY_FN, &dbv))
	{
		file = mir_strdup(dbv.pszVal);
		DBFreeVariant(&dbv);
		wd = mir_strdup(file);
		char* swd = strrchr(wd,'\\');
		*swd = '\0';
		size = getDword(hContact, AIM_KEY_FS, 0);
		if (!size) return;
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
	memcpy(ft.filename,pszFile,lstrlenA(pszFile));
	char* buf = (char*)&ft;
	if(Netlib_Send(hNewConnection,buf,sizeof(oft2),0)==SOCKET_ERROR)
	{
		sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_FAILED,hContact,0);
		Netlib_CloseHandle(hNewConnection);
		return;
	}
	LOG("Sent file information to buddy.");
	//start listen for packets stuff
	int recvResult=0;
	NETLIBPACKETRECVER packetRecv;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	HANDLE hServerPacketRecver;
	hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hNewConnection, 2048 * 4);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = 100*getWord( AIM_KEY_GP, DEFAULT_GRACE_PERIOD);
	for(;;)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0)
			{
				LOG("P2P: File transfer connection Error: 0");
				sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_FAILED,hContact,0);
                Netlib_CloseHandle(hServerPacketRecver);
                Netlib_CloseHandle(hNewConnection);
				break;
            }
        if (recvResult == SOCKET_ERROR)
			{
				LOG("P2P: File transfer connection Error: -1");
				sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_FAILED,hContact,0);
                Netlib_CloseHandle(hServerPacketRecver);
				Netlib_CloseHandle(hNewConnection);
                break;
            }
		if(recvResult>0)
		{	
			if(recvResult==256)
			{
				packetRecv.bytesUsed=256;
				oft2* recv_ft=(oft2*)&packetRecv.buffer[0];
				unsigned short type=_htons(recv_ft->type);
				if(type==0x0202||type==0x0207)
				{
					LOG("P2P: Buddy Accepts our file transfer.");
					FILE *fd= fopen(file, "rb");
					if (fd)
					{
						if(file_start_point)
							fseek(fd,file_start_point,SEEK_SET);
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
						pfts.workingDir=wd;
						sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_DATA,hContact, (LPARAM) & pfts);
						unsigned int bytes;
						char buffer[1024*4];
						unsigned int lNotify=GetTickCount()-500;
						while ((bytes = (unsigned)fread(buffer, 1, 1024*4, fd)))
						{
							Netlib_Send(hNewConnection,buffer,bytes,MSG_NODUMP);
							pfts.currentFileProgress += bytes;
							pfts.totalProgress += bytes;
							if(GetTickCount()>lNotify+500)
							{
								sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_DATA,hContact, (LPARAM) & pfts);
								lNotify=GetTickCount();
							}
						}
						sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_DATA,hContact, (LPARAM) & pfts);
						LOG("P2P: Finished sending file bytes.");
						fclose(fd);
					}
					sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS,hContact,0);
                    mir_free(wd);
					mir_free(file);
                    Netlib_CloseHandle(hServerPacketRecver);
					return;
				}
				else if(type==0x0204)
				{
					LOG("P2P: Buddy says they got the file successfully");
					sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS,hContact,0);
                    mir_free(wd);
					mir_free(file);
                    Netlib_CloseHandle(hServerPacketRecver);
                    return;
				}
				else if(type==0x0205)
				{
					LOG("P2P: Buddy wants us to start sending at a specified file point.");
					oft2* recv_ft=(oft2*)&packetRecv.buffer[0];
					recv_ft->type=_htons(0x0106);
					file_start_point=_htonl(recv_ft->recv_bytes);
					char* buf = (char*)recv_ft;
					if(Netlib_Send(hNewConnection,buf,sizeof(oft2),0)==SOCKET_ERROR)
					{
						sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_FAILED,hContact,0);
                        Netlib_CloseHandle(hServerPacketRecver);
						Netlib_CloseHandle(hNewConnection);
                        mir_free(wd);
					    mir_free(file);
						return;
					}
				}
			}
		}
	}
}

#if _MSC_VER
	#pragma warning( default: 4706 )
#endif

void CAimProto::receiving_file(HANDLE hContact, HANDLE hNewConnection)
{
	LOG("P2P: Entered file receiving thread.");
	bool accepted_file=0;
	DBVARIANT dbv;
	char* file=0;
	FILE *fd=0;
	oft2 ft;

	PROTOFILETRANSFERSTATUS pfts;
	memset(&pfts, 0, sizeof(PROTOFILETRANSFERSTATUS));
	pfts.hContact=hContact;
	pfts.totalFiles=1;

    unsigned long size;
	if (!getString(hContact, AIM_KEY_FN, &dbv))
	{
		file=mir_strdup(dbv.pszVal);
		pfts.workingDir=mir_strdup(file);
		DBFreeVariant(&dbv);
	}
	//start listen for packets stuff
	int recvResult=0;
	NETLIBPACKETRECVER packetRecv;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	HANDLE hServerPacketRecver=NULL;
	hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hNewConnection, 2048 * 4);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = 100*getWord( AIM_KEY_GP, DEFAULT_GRACE_PERIOD);
	for(;;)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0)
			{
				LOG("P2P: File transfer connection Error: 0");
				sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_FAILED,hContact,0);
                Netlib_CloseHandle(hServerPacketRecver);
				Netlib_CloseHandle(hNewConnection);
                break;
            }
        if (recvResult == SOCKET_ERROR)
			{
				LOG("P2P: File transfer connection Error: -1");
				sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_FAILED,hContact,0);
                Netlib_CloseHandle(hServerPacketRecver);
				Netlib_CloseHandle(hNewConnection);
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
						LOG("P2P: Buddy Ready to begin transfer.");
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
						file = (char*)mir_realloc(file,lstrlenA(file)+1+lstrlenA((char*)&ft.filename)+1);
						lstrcatA(file,(char*)&ft.filename);
						pfts.currentFile=file;
						fd = fopen(file, "wb");
						if(!fd)
						{
                            mir_free(pfts.workingDir);
							mir_free(file);
                            Netlib_CloseHandle(hServerPacketRecver);
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
				sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_DATA,hContact, (LPARAM) & pfts);
				if(pfts.totalBytes==pfts.currentFileProgress)
				{

					ft.type=_htons(0x0204);
					ft.recv_bytes=_htonl(pfts.totalBytes);
					fclose(fd);
					fd=0;
					ft.recv_checksum=_htonl(aim_oft_checksum_file(file));
					LOG("P2P: We got the file successfully");
					Netlib_Send(hNewConnection,(char*)&ft,sizeof(oft2),0);
					sendBroadcast(hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS,hContact,0);
                    Netlib_CloseHandle(hServerPacketRecver);
					break;
				}
			}
		}
	}
	if(accepted_file&&fd)
		fclose(fd);
	mir_free(file);
    mir_free(pfts.workingDir);
}
