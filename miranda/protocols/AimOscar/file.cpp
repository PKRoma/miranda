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

#pragma pack(push, 1)
struct oft2//oscar file transfer 2 class- See On_Sending_Files_via_OSCAR.pdf
{
    char protocol_version[4];//4
    unsigned short length;//6
    unsigned short type;//8
    unsigned char icbm_cookie[8];//16
    unsigned short encryption;//18
    unsigned short compression;//20
    unsigned short total_files;//22
    unsigned short num_files_left;//24
    unsigned short total_parts;//26
    unsigned short parts_left;//28
    unsigned long total_size;//32
    unsigned long size;//36
    unsigned long mod_time;//40
    unsigned long checksum;//44
    unsigned long recv_RFchecksum;//48
    unsigned long RFsize;//52
    unsigned long creation_time;//56
    unsigned long RFchecksum;//60
    unsigned long recv_bytes;//64
    unsigned long recv_checksum;//68
    unsigned char idstring[32];//100
    unsigned char flags;//101
    unsigned char list_name_offset;//102
    unsigned char list_size_offset;//103
    unsigned char dummy[69];//172
    unsigned char mac_info[16];//188
    unsigned short encoding;//190
    unsigned short sub_encoding;//192
    unsigned char filename[64];//256
 };

#pragma pack(pop)

bool send_init_oft2(file_transfer *ft)
{
    aimString astr(get_fname(ft->file));
    TCHAR *name = mir_utf8decodeT(ft->file);

    size_t len = max(0x100, 0xc0 + astr.getTermSize());

    oft2 *oft = (oft2*)alloca(len);
    memset(oft, 0, len);

    memcpy(oft->protocol_version, "OFT2", 4);
    oft->length           = _htons(len);
    oft->type             = 0x0101;
    oft->total_files      = _htons(1);
    oft->num_files_left   = _htons(1);
    oft->total_parts      = _htons(1);
    oft->parts_left       = _htons(1);
    oft->total_size       = _htonl(ft->total_size);
    oft->size             = _htonl(ft->total_size);
    oft->mod_time         = _htonl(ft->ctime);
    oft->checksum         = _htonl(aim_oft_checksum_file(name));
    oft->recv_RFchecksum  = 0x0000FFFF;
    oft->RFchecksum       = 0x0000FFFF;
    oft->recv_checksum    = 0x0000FFFF;
    memcpy(oft->idstring, "Cool FileXfer", 13);
    oft->flags            = 0x20;
    oft->list_name_offset = 0x1c;
    oft->list_size_offset = 0x11;
    oft->encoding = _htons(astr.isUnicode() ? 2 : 0);
    memcpy(oft->filename, astr.getBuf(), astr.getTermSize());

    mir_free(name);

    return Netlib_Send(ft->hConn, (char*)oft, len, 0) > 0;
}

bool CAimProto::sending_file(file_transfer *ft, HANDLE hServerPacketRecver, NETLIBPACKETRECVER &packetRecv)
{
    LOG("P2P: Entered file sending thread.");

    bool failed = true;

    unsigned __int64 file_start_point = 0;

    send_init_oft2(ft);

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
            if (recvResult < 0x100) continue;
           
            oft2* recv_ft = (oft2*)&packetRecv.buffer[packetRecv.bytesUsed];

            unsigned short pkt_len = _htons(recv_ft->length);
            if (recvResult < pkt_len) continue;

            packetRecv.bytesUsed += pkt_len;
            unsigned short type = _htons(recv_ft->type);
            if (type == 0x0202 || type == 0x0207)
            {
                LOG("P2P: Buddy Accepts our file transfer.");
                TCHAR *fn = mir_utf8decodeT(ft->file);
                FILE *fd = _tfopen(fn, _T("rb"));
                mir_free(fn);

                if (fd == NULL) break;

                if (file_start_point) fseek(fd, file_start_point, SEEK_SET);

                NETLIBSELECT tSelect = {0};
                tSelect.cbSize = sizeof(tSelect);
                tSelect.hReadConns[0] = ft->hConn;

                PROTOFILETRANSFERSTATUS pfts;
                memset(&pfts, 0, sizeof(PROTOFILETRANSFERSTATUS));
                pfts.currentFileNumber      = 0;
                pfts.currentFileProgress    = 0;
                pfts.currentFileSize        = _htonl(recv_ft->size);
                pfts.currentFileTime        = 0;
                pfts.ptszFiles              = NULL;
                pfts.hContact               = ft->hContact;
                pfts.flags                  = PFTS_SENDING | PFTS_TCHAR;
                pfts.totalBytes             = ft->total_size;
                pfts.totalFiles             = 1;
                pfts.totalProgress          = 0;

                pfts.tszCurrentFile         = mir_utf8decodeT(get_fname(ft->file));
                pfts.tszWorkingDir          = mir_utf8decodeT(ft->file);

                TCHAR* swd = _tcsrchr(pfts.tszWorkingDir, '\\'); 
                if (swd) *swd = '\0'; else pfts.tszWorkingDir[0] = 0;

                sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&pfts);

                unsigned int bytes;
                char buffer[1024*4];
                unsigned int lNotify = GetTickCount() - 500;
                while ((bytes = (unsigned)fread(buffer, 1, 1024*4, fd)))
                {
                    if (Netlib_Send(ft->hConn, buffer, bytes, MSG_NODUMP) <= 0) break;
                    pfts.currentFileProgress += bytes;
                    pfts.totalProgress += bytes;
                    if(GetTickCount()>lNotify+500)
                    {
                        sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&pfts);
                        lNotify = GetTickCount();
                        if (CallService(MS_NETLIB_SELECT, 0, (LPARAM)&tSelect)) break;
                    }
                }
                sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&pfts);
                LOG("P2P: Finished sending file bytes.");
                fclose(fd);
                mir_free(pfts.tszWorkingDir);
                mir_free(pfts.tszCurrentFile);
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
                
                file_start_point = _htonl(recv_ft->recv_bytes);

                if (Netlib_Send(ft->hConn, (char*)recv_ft, _htons(recv_ft->length), 0) == SOCKET_ERROR)
                    break;
            }
        }
    }
    return !failed;
}

bool CAimProto::receiving_file(file_transfer *ft, HANDLE hServerPacketRecver, NETLIBPACKETRECVER &packetRecv)
{
    LOG("P2P: Entered file receiving thread.");
    bool failed = true;
    bool accepted_file = false;
    FILE *fd = NULL;

    oft2 *oft = NULL;

    PROTOFILETRANSFERSTATUS pfts;
    memset(&pfts, 0, sizeof(PROTOFILETRANSFERSTATUS));
    pfts.hContact   = ft->hContact;
    pfts.flags      = PFTS_TCHAR;
    pfts.totalFiles = 1;
    pfts.tszWorkingDir = mir_utf8decodeT(ft->file);

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
                if (recvResult < 0x100) continue;
               
                oft2* recv_ft = (oft2*)&packetRecv.buffer[packetRecv.bytesUsed];
                unsigned short pkt_len = _htons(recv_ft->length);

                if (recvResult < pkt_len) continue;
                packetRecv.bytesUsed += pkt_len;

                unsigned short type = _htons(recv_ft->type);
                if (type == 0x0101)
                {
                    LOG("P2P: Buddy Ready to begin transfer.");
                    oft = (oft2*)mir_realloc(oft, pkt_len);
                    memcpy(oft, recv_ft, pkt_len);
                    memcpy(oft->icbm_cookie, ft->icbm_cookie, 8);
                    oft->type = _htons(ft->start_offset ? 0x0205 : 0x0202);

                    int buflen = pkt_len - 0x100 + 64;
                    char *buf = (char*)mir_calloc(buflen+2);
                    unsigned short enc;

                    const unsigned long size = _htonl(recv_ft->size);
                    pfts.currentFileSize = size;
                    pfts.totalBytes = size;
                    pfts.currentFileTime = recv_ft->mod_time;
                    memcpy(buf, recv_ft->filename, buflen);
                    enc = _htons(recv_ft->encoding);

                    TCHAR *name;
                    if (enc == 2)
                    {
                        wcs_htons((wchar_t*)buf);
                        name = mir_u2t((wchar_t*)buf);
                    }
                    else
                        name = mir_a2t(buf);

                    TCHAR fname[256];
                    mir_sntprintf(fname, SIZEOF(fname), _T("%s%s"), pfts.tszWorkingDir, name);
                    mir_free(name);
                    pfts.tszCurrentFile = mir_tstrdup(fname);
                    ft->pfts = &pfts;
//                  sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_FILERESUME, ft, (LPARAM)&pfts);
                    fd = _tfopen(fname, _T("wb"));

                    mir_free(buf);
                    accepted_file = fd != NULL;
                    if (!accepted_file)	break;

                    if (ft->start_offset)
                    {
                        oft->recv_bytes = _htonl(ft->start_offset);
                        oft->checksum = _htonl(aim_oft_checksum_file(pfts.tszCurrentFile));
                    }

                    if (Netlib_Send(ft->hConn, (char*)oft, pkt_len, 0) == SOCKET_ERROR)
                        break;
                }
                else
                    break;
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
                    oft->type = _htons(0x0204);
                    oft->recv_bytes = _htonl(pfts.totalBytes);
                    oft->recv_checksum = _htonl(aim_oft_checksum_file(pfts.tszCurrentFile));
                    oft->flags = 0x24;

                    LOG("P2P: We got the file successfully");
                    Netlib_Send(ft->hConn, (char*)oft, _htons(oft->length), 0);
                    fclose(fd);
                    fd = 0;
                    failed = false;
                    break;
                }
            }
        }
    }

    if (accepted_file && fd) fclose(fd);
    mir_free(pfts.tszWorkingDir);
    mir_free(pfts.tszCurrentFile);
    mir_free(oft);

    return !failed;
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
        if (ft->accepted && ft->requester && (ft->local_ip == ip || ft->verified_ip == ip))
            return ft;
    }
    return NULL;
}

file_transfer* ft_list_type::find_suitable(void)
{
    for (int i = getCount(); i--; )
    {
        file_transfer *ft = items[i];
        if (ft->accepted && ft->requester)
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