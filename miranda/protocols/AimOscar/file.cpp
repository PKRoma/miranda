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

void fill_OFT2(oft2 *oft, file_transfer *ft)
{
    memset(oft, 0, sizeof(oft2));
    memcpy(oft->protocol_version, "OFT2", 4);
	oft->length           = _htons(sizeof(oft2));
	oft->type             = 0x0101;
//    memcpy(oft->icbm_cookie, ft->icbm_cookie, 8);
	oft->total_files      = _htons(1);
	oft->num_files_left   = _htons(1);
	oft->total_parts      = _htons(1);
	oft->parts_left       = _htons(1);
	oft->total_size       = _htonl(ft->total_size);
	oft->size             = _htonl(ft->total_size);
	oft->mod_time         = _htonl(ft->ctime);
	oft->checksum         = _htonl(aim_oft_checksum_file(ft->file));
	oft->recv_RFchecksum  = 0x0000FFFF;
	oft->RFchecksum       = 0x0000FFFF;
	oft->recv_checksum    = 0x0000FFFF;
	memcpy(oft->idstring, "Cool FileXfer", 13);
	oft->flags            = 0x20;
	oft->list_name_offset = 0x1c;
	oft->list_size_offset = 0x11;

	char* pszFile = strrchr(ft->file, '\\');
	if (pszFile) pszFile++; else pszFile = ft->file;

    if (is_utf(pszFile))
    {
        wchar_t *fn = mir_utf8decodeW(pszFile);
        size_t len = wcslen(fn);
        wcs_htons(fn);
	    memcpy(oft->filename, fn, len*sizeof(wchar_t));
        oft->encoding = 2;
    }
    else
	    memcpy(oft->filename, pszFile, strlen(pszFile));
}

void fill_OFT3(oft3 *oft, file_transfer *ft)
{
    memset(oft, 0, sizeof(oft3));
    memcpy(oft->protocol_version, "OFT3", 4);
	oft->length           = _htons(sizeof(oft2));
	oft->type             = 0x0101;
    memcpy(oft->icbm_cookie, ft->icbm_cookie, 8);
	oft->total_files      = _htons(1);
	oft->num_files_left   = _htons(1);
	oft->total_parts      = _htons(1);
	oft->parts_left       = _htons(1);
	oft->total_size       = _htonl64(ft->total_size);
	oft->size             = _htonl64(ft->total_size);
	oft->mod_time         = _htonl(ft->ctime);
	memcpy(oft->idstring, "Cool FileXfer", 13);
	oft->flags            = 0x20;
	oft->list_name_offset = 0x1c;
	oft->list_size_offset = 0x11;

	char* pszFile = strrchr(ft->file, '\\');
	if (pszFile) pszFile++; else pszFile = ft->file;

    if (is_utf(pszFile))
    {
        wchar_t *fn = mir_utf8decodeW(pszFile);
        size_t len = wcslen(fn);
        wcs_htons(fn);
	    memcpy(oft->filename, fn, len*sizeof(wchar_t));
        oft->encoding = 2;
    }
    else
	    memcpy(oft->filename, pszFile, strlen(pszFile));
}

void CAimProto::sending_file(file_transfer *ft, HANDLE hServerPacketRecver, NETLIBPACKETRECVER &packetRecv)
{
	LOG("P2P: Entered file sending thread.");

    bool failed = true;

	unsigned __int64 file_start_point = 0;

    oft2 oft;

    if (ft->use_oft3)
        fill_OFT3((oft3*)&oft, ft);
    else
        fill_OFT2(&oft, ft);


	if (Netlib_Send(ft->hConn, (char*)&oft, sizeof(oft2), 0) == SOCKET_ERROR)
        goto exit;

    LOG("Sent file information to buddy.");
	//start listen for packets stuff

    for (;;)
	{
        int recvResult = packetRecv.bytesAvailable - packetRecv.bytesUsed;
        if (recvResult <= 0)
		    recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM)hServerPacketRecver, (LPARAM)&packetRecv);
		if (recvResult == 0)
		{
			LOG("P2P: File transfer connection Error: 0");
		    break;
        }
        if (recvResult == SOCKET_ERROR)
		{
			LOG("P2P: File transfer connection Error: -1");
            break;
        }
		if (recvResult > 0)
		{	
			if (recvResult >= sizeof(oft2))
			{
				oft2* recv_ft = (oft2*)&packetRecv.buffer[packetRecv.bytesUsed];
				packetRecv.bytesUsed += sizeof(oft2);
				unsigned short type = _htons(recv_ft->type);
				if (type == 0x0202 || type == 0x0207)
				{
					LOG("P2P: Buddy Accepts our file transfer.");
                    TCHAR *fn = mir_utf8decodeT(ft->file);
				    FILE *fd = _tfopen(fn, _T("rb"));
                    mir_free(fn);
					if (fd)
					{
						if (file_start_point) fseek(fd, file_start_point, SEEK_SET);

						PROTOFILETRANSFERSTATUS pfts;
						memset(&pfts, 0, sizeof(PROTOFILETRANSFERSTATUS));
						pfts.currentFileNumber      = 0;
						pfts.currentFileProgress    = 0;
						pfts.currentFileSize        = _htonl(oft.size);
						pfts.currentFileTime        = 0;
						pfts.files                  = NULL;
						pfts.hContact               = ft->hContact;
						pfts.sending                = 1;
                        pfts.totalBytes             = ft->total_size;
						pfts.totalFiles             = 1;
						pfts.totalProgress          = 0;

                        pfts.workingDir             = mir_strdup(ft->file);
                        
                        char* swd = strrchr(pfts.workingDir, '\\'); 
	                    if (swd) *swd = '\0'; else pfts.workingDir[0] = 0;
                        pfts.currentFile = mir_utf8decodeA(swd ? swd+1 : ft->file);

						sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&pfts);

						unsigned int bytes;
						char buffer[1024*4];
						unsigned int lNotify = GetTickCount() - 500;
						while ((bytes = (unsigned)fread(buffer, 1, 1024*4, fd)))
						{
							if (Netlib_Send(ft->hConn, buffer, bytes, 0) == SOCKET_ERROR) break;
							pfts.currentFileProgress += bytes;
							pfts.totalProgress += bytes;
							if(GetTickCount()>lNotify+500)
							{
								sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&pfts);
								lNotify = GetTickCount();
							}
						}
						sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&pfts);
						LOG("P2P: Finished sending file bytes.");
						fclose(fd);
                        mir_free(pfts.workingDir);
                        mir_free(pfts.currentFile);
					}
                    else
					    break;
				}
				else if (type == 0x0204)
				{
					LOG("P2P: Buddy says they got the file successfully");
                    failed = false;
                    break;
				}
				else if (type == 0x0205)
				{
					LOG("P2P: Buddy wants us to start sending at a specified file point.");
					oft2* recv_ft = (oft2*)packetRecv.buffer;
					recv_ft->type = _htons(0x0106);
                    
                    if (memcmp(recv_ft->protocol_version, "OFT2", 4) == 0)
					    file_start_point = _htonl(recv_ft->recv_bytes);
                    else
					    file_start_point = _htonl64(((oft3*)recv_ft)->recv_bytes);

					if (Netlib_Send(ft->hConn, (char*)recv_ft, sizeof(oft2), 0) == SOCKET_ERROR)
						break;
				}
			}
		}
	}

exit:
    sendBroadcast(ft->hContact, ACKTYPE_FILE, failed ? ACKRESULT_FAILED : ACKRESULT_SUCCESS, ft, 0);
}

void CAimProto::receiving_file(file_transfer *ft, HANDLE hServerPacketRecver, NETLIBPACKETRECVER &packetRecv)
{
	LOG("P2P: Entered file receiving thread.");
    bool failed = true;
	bool accepted_file = false;
	FILE *fd = NULL;
	oft2 oft;

	PROTOFILETRANSFERSTATUS pfts;
	memset(&pfts, 0, sizeof(PROTOFILETRANSFERSTATUS));
	pfts.hContact   = ft->hContact;
	pfts.totalFiles = 1;
	pfts.workingDir = mir_strdup(ft->file);

    //start listen for packets stuff
	for (;;)
	{
        int recvResult = packetRecv.bytesAvailable - packetRecv.bytesUsed;
        if (recvResult <= 0)
		    recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM)hServerPacketRecver, (LPARAM)&packetRecv);
		if (recvResult == 0)
		{
			LOG("P2P: File transfer connection Error: 0");
            break;
        }
        if (recvResult == SOCKET_ERROR)
		{
			LOG("P2P: File transfer connection Error: -1");
            break;
        }
		if (recvResult > 0)
		{	
			if (!accepted_file)
			{
				if (recvResult >= sizeof(oft2))
				{
					oft2* recv_ft = (oft2*)&packetRecv.buffer[packetRecv.bytesUsed];
					packetRecv.bytesUsed += sizeof(oft2);
					unsigned short type = _htons(recv_ft->type);
					if (type == 0x0101)
					{
						LOG("P2P: Buddy Ready to begin transfer.");
						memcpy(&oft, recv_ft, sizeof(oft2));
                        memcpy(oft.icbm_cookie, ft->icbm_cookie, 8);
                        oft.type = _htons(ft->start_offset ? 0x0205 : 0x0202);

                        char *buf = (char*)mir_calloc(70);
                        unsigned short enc;

                        if (memcmp(oft.protocol_version, "OFT2", 4) == 0)
                        {
						    const unsigned long size = _htonl(recv_ft->size);
						    pfts.currentFileSize = size;
						    pfts.totalBytes = size;
                            pfts.currentFileTime = recv_ft->mod_time;
                            memcpy(buf, recv_ft->filename, 64);
                            enc = _htons(recv_ft->encoding);
                        }
                        else
                        { 
                            oft3* nft = (oft3*)recv_ft;
						    const unsigned long size = (unsigned long)_htonl64(nft->size);
						    pfts.currentFileSize = size;
						    pfts.totalBytes = size;
                            pfts.currentFileTime = nft->mod_time;
                            memcpy(buf, nft->filename, 56);
                            enc = _htons(nft->encoding);
                        }

                        if (enc == 2)
                        {
                            TCHAR *path = mir_a2t(pfts.workingDir);
                            wcs_htons((wchar_t*)buf);
                            TCHAR *name = mir_u2t((wchar_t*)buf);
                            TCHAR fname[256];
                            mir_sntprintf(fname, SIZEOF(fname), _T("%s%s"), path, name);
                            mir_free(name);
                            mir_free(path);
                            pfts.currentFile = mir_t2a(fname);
						    fd = _tfopen(fname, _T("wb"));
                        }
                        else
                        {
                            char fname[256];
                            mir_snprintf(fname, SIZEOF(fname), "%s%s", pfts.workingDir, buf);
                            pfts.currentFile = mir_strdup(fname);
                            fd = fopen(fname, "wb");
                        }
                        mir_free(buf);
                        accepted_file = fd != NULL;
						if (!accepted_file)	break;

                        if (ft->start_offset)
                        {
                            if (memcmp(oft.protocol_version, "OFT2", 4) == 0)
                            {
                                oft.recv_bytes = _htonl(ft->start_offset);
					            oft.checksum = _htonl(aim_oft_checksum_file(pfts.currentFile));
                            }
                            else
                            {

                                ((oft3*)&oft)->recv_bytes = _htonl(ft->start_offset);
					            ((oft3*)&oft)->checksum = _htonl(aim_oft_checksum_file(pfts.currentFile));
                            }
                        }

                        if (Netlib_Send(ft->hConn, (char*)&oft, sizeof(oft2), 0) == SOCKET_ERROR)
                            break;
					}
                    else
                        break;
				}
			}
			else
			{
				packetRecv.bytesUsed = packetRecv.bytesAvailable;
				fwrite(packetRecv.buffer, 1, packetRecv.bytesAvailable, fd);
				pfts.currentFileProgress += packetRecv.bytesAvailable;
				pfts.totalProgress       += packetRecv.bytesAvailable;
				sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&pfts);
				if (pfts.totalBytes == pfts.currentFileProgress)
				{
					oft.type = _htons(0x0204);
                    if (memcmp(oft.protocol_version, "OFT2", 4) == 0)
                    {
					    oft.recv_bytes = _htonl(pfts.totalBytes);
					    oft.recv_checksum = _htonl(aim_oft_checksum_file(pfts.currentFile));
                    }
                    else
                    {
                        oft3* nft = (oft3*)&oft;
					    nft->recv_bytes = _htonl(pfts.totalBytes);
                    }
					LOG("P2P: We got the file successfully");
					Netlib_Send(ft->hConn, (char*)&oft, sizeof(oft2), 0);
					fclose(fd);
					fd = 0;
                    failed = false;
					break;
				}
			}
		}
	}

	if (accepted_file && fd) fclose(fd);
    sendBroadcast(ft->hContact, ACKTYPE_FILE, failed ? ACKRESULT_FAILED : ACKRESULT_SUCCESS, ft, 0);
    if (failed) aim_file_ad(hServerConn, seqno, ft->sn, ft->icbm_cookie, true);
    mir_free(pfts.workingDir);
    mir_free(pfts.currentFile);
}

ft_list_type::ft_list_type() :  OBJLIST <file_transfer>(10) {};

file_transfer* ft_list_type::find_by_cookie(char* cookie, HANDLE hContact)
{
    for (int i = 0; i < getCount(); ++i)
    {
        file_transfer *ft = items[i];
        if (ft->hContact == hContact && memcmp(ft->icbm_cookie, cookie, 8) == 0)
            return ft;
    }
    return NULL;
}

file_transfer* ft_list_type::find_by_ip(unsigned long ip)
{
    for (int i = getCount(); i--; )
    {
        file_transfer *ft = items[i];
        if (ft->accepted && ft->listen && (ft->local_ip == ip || ft->verified_ip == ip))
            return ft;
    }
    return NULL;
}

file_transfer* ft_list_type::find_suitable(void)
{
    for (int i = getCount(); i--; )
    {
        file_transfer *ft = items[i];
        if (ft->accepted && ft->listen && ft->local_ip == 0 && ft->verified_ip == 0 && ft->sending)
            return ft;
    }
    return NULL;
}


bool ft_list_type::find_by_ft(file_transfer *ft)
{
    for (int i = 0; i < getCount(); ++i)
    {
        if (items[i] == ft) return true;
    }
    return false;
}

void ft_list_type::remove_by_ft(file_transfer *ft)
{
    for (int i = 0; i < getCount(); ++i)
    {
        if (items[i] == ft)
        {
            remove(i);
            break;
        }
    }
}