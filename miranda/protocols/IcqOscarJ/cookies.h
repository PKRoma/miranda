// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001-2002 Jon Keating, Richard Hughes
// Copyright © 2002-2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004-2008 Joe Kucera
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// -----------------------------------------------------------------------------
//
// File name      : $URL$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#ifndef __COOKIES_H
#define __COOKIES_H


#define CKT_MESSAGE         0x01
#define CKT_FILE            0x02
#define CKT_SEARCH          0x04
#define CKT_SERVERLIST      0x08
#define CKT_SERVICEREQUEST  0x0A
#define CKT_REVERSEDIRECT   0x0C
#define CKT_FAMILYSPECIAL   0x10
#define CKT_OFFLINEMESSAGE  0x12
#define CKT_AVATAR          0x20
#define CKT_CHECKSPAMBOT    0x40

struct CIcqProto;

typedef struct icq_cookie_info_s
{
  DWORD dwCookie;
  HANDLE hContact;
  void *pvExtra;
  time_t dwTime;
  BYTE bType;
} icq_cookie_info;

typedef struct familyrequest_rec_s
{
  WORD wFamily;
  void (CIcqProto::*familyhandler)(HANDLE hConn, char* cookie, WORD cookieLen);
} familyrequest_rec;


typedef struct offline_message_cookie_s
{
  int nMessages;
} offline_message_cookie;

typedef struct message_cookie_data_s
{
  DWORD dwMsgID1;
  DWORD dwMsgID2;
  WORD bMessageType;
  BYTE nAckType;
} message_cookie_data;

#define ACKTYPE_NONE   0
#define ACKTYPE_SERVER 1
#define ACKTYPE_CLIENT 2

typedef struct message_cookie_data_ex_s
{
  message_cookie_data msg;
  BYTE isOffline;
} message_cookie_data_ex;

typedef struct fam15_cookie_data_s
{
  BYTE bRequestType;
} fam15_cookie_data;

#define REQUESTTYPE_OWNER        0
#define REQUESTTYPE_USERAUTO     1
#define REQUESTTYPE_USERMINIMAL  2
#define REQUESTTYPE_USERDETAILED 3
#define REQUESTTYPE_PROFILE      4


typedef struct search_cookie_s
{
  BYTE bSearchType;
  char* szObject;
  DWORD dwMainId;
  DWORD dwStatus;
} search_cookie;

#define SEARCHTYPE_UID     0
#define SEARCHTYPE_EMAIL   1
#define SEARCHTYPE_NAMES   2
#define SEARCHTYPE_DETAILS 4

typedef struct avatarcookie_t
{
  DWORD dwUin;
  HANDLE hContact;
  unsigned int hashlen;
  BYTE *hash;
  unsigned int cbData;
  char *szFile;
} avatarcookie;

typedef struct {
  message_cookie_data pMessage;
  HANDLE hContact;
  DWORD dwUin;
  int type;
  void *ft;
} reverse_cookie;

#endif /* __COOKIES_H */
