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
#ifndef FILE_H
#define FILE_H

class oft2//oscar file transfer 2 class- See On_Sending_Files_via_OSCAR.pdf
 {
 public:
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

class file_transfer
{
public:
	HANDLE hContact;
    char* sn;
	char icbm_cookie[8];
	char* file;
	unsigned long total_size;

	//below is for when receiving only
	unsigned long local_ip;
	unsigned long verified_ip;
	unsigned long proxy_ip;
	unsigned short port;
	char* message;

	bool force_proxy;

	file_transfer()  { memset(this, 0, sizeof(*this)); }
	~file_transfer() { mir_free(file); mir_free(sn); }
};

#endif
