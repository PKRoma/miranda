/*
AOL Instant Messenger Plugin for Miranda IM

Copyright © 2003-2005 Robert Rainwater

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef TOC_H
#define TOC_H

#include <windows.h>

#define FLAPON 					"FLAPON\r\n\r\n"
#define ROAST 					"Tic/Toc"
#define REVISION 				"\"TIC:\\$Revision$\" 160 US \"\" \"\" 3 0 30303 -kentucky -utf8"
#define LANGUAGE 				"English"

#define STATE_OFFLINE 			0
#define STATE_FLAPON 			1
#define STATE_SIGNON		 	2
#define STATE_ONLINE 			3
#define STATE_PAUSE 			4

#define TYPE_SIGNON    			1
#define TYPE_DATA      			2
#define TYPE_ERROR     			3
#define TYPE_SIGNOFF   			4
#define TYPE_KEEPALIVE 			5

#define USER_AOL				0x02
#define USER_ADMIN				0x04
#define USER_UNCONFIRMED		0x08
#define USER_NORMAL				0x10
#define USER_WIRELESS     		0x20
#define USER_UNAVAILABLE		0x40

#define MSG_LEN					2048
#define BUF_LONG                (MSG_LEN*2)
#define MAX_SN_LEN              16

#define UID_ICQ_SUPPORT         "0946134D-4C7F-11D1-8222-444553540000"
#define UID_AIM_CHAT            "748F2420-6287-11D1-8222-444553540000"
#define UID_AIM_FILE_RECV       "09461343-4C7F-11D1-8222-444553540000"

struct signon
{
    unsigned int ver;
    unsigned short tag;
    unsigned short namelen;
    char username[80];
};

struct toc_data
{
    char *username;
    char *password;
    int seqno;
    int state;
};

struct toc_sflap_hdr
{
    unsigned char ast;
    unsigned char type;
    unsigned short seqno;
    unsigned short len;
};

extern struct toc_data *tdt;

// toc.c
HANDLE aim_toc_connect();
void aim_toc_disconnect();
int aim_toc_login(HANDLE hConn);
int aim_toc_parse(char *buffer, int len);
int aim_toc_sflapsend(char *buf, int olen, int type);

#endif
