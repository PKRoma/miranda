/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000  Roland Rabien

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

For more information, e-mail figbug@users.sourceforge.net
*/

#ifndef _icqdll_h
#define _icqdll_h

#include <winsock.h>

// structures
typedef struct 
{
  const char *name;
  unsigned short code;
} COUNTRY_CODE;

typedef struct
{
  /* General */
  unsigned long icq_Uin;
  unsigned long icq_OurIP;          /* HOST byteorder */
  unsigned short icq_OurPort;       /* HOST byteorder */
  void *icq_ContactList;
  unsigned long icq_Status;
  char  *icq_Password;

  /* UDP stuff */
  int icq_UDPSok;
  unsigned char icq_UDPServMess[8192]; /* 65536 seqs max, 1 bit per seq -> 65536/8 = 8192 */
  unsigned short icq_UDPSeqNum1, icq_UDPSeqNum2;
  unsigned long icq_UDPSession;
  void *icq_UDPQueue;
  int icq_UDPExpInt;

  /* TCP stuff */
  int icq_TCPSrvSock;
  unsigned short icq_TCPSrvPort;    /* HOST byteorder */
  unsigned long icq_TCPSrvIP;       /* HOST byteorder */
  int icq_TCPSequence;
  void *icq_TCPLinks;
  void *icq_ChatSessions;
  void *icq_FileSessions;
  int TCP_maxfd;
  fd_set TCP_readfds;
  fd_set TCP_writefds;

  /* SOCKS5 Proxy stuff */
  unsigned char icq_UseProxy;
  char *icq_ProxyHost;
  unsigned long icq_ProxyIP;        /* HOST byteorder */
  unsigned short icq_ProxyPort;     /* HOST byteorder */
  int  icq_ProxyAuth;
  char *icq_ProxyName;
  char *icq_ProxyPass;
  int icq_ProxySok;
  unsigned short icq_ProxyOurPort;  /* HOST byteorder */
  unsigned long icq_ProxyDestIP;    /* HOST byteorder */
  unsigned short icq_ProxyDestPort; /* HOST byteorder */
} ICQLINK;

#endif _icqdll_h