// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006 Joe Kucera
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
// File name      : $Source$
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


typedef struct icq_cookie_info_s
{
  DWORD dwCookie;
  DWORD dwUin;
  void *pvExtra;
  DWORD dwTime;
} icq_cookie_info;

typedef struct familyrequest_rec_s
{
  WORD wFamily;
  void (*familyhandler)(HANDLE hConn, char* cookie, WORD cookieLen);
} familyrequest_rec;


typedef struct message_cookie_data_s
{
  BYTE bMessageType;
  BYTE nAckType;
} message_cookie_data;

#define ACKTYPE_NONE   0
#define ACKTYPE_SERVER 1
#define ACKTYPE_CLIENT 2


typedef struct fam15_cookie_data_s
{
  BYTE bRequestType;
  HANDLE hContact;
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


void InitCookies(void);
void UninitCookies(void);
DWORD AllocateCookie(WORD wIdent, DWORD dwUin, void *pvExtra);
int FindCookie(DWORD wCookie, DWORD *pdwUin, void **ppvExtra);
int FindCookieByData(void *pvExtra,DWORD *pdwCookie, DWORD *pdwUin);
void FreeCookie(DWORD wCookie);
DWORD GenerateCookie(WORD wIdent);


#endif /* __COOKIES_H */
