// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright � 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001-2002 Jon Keating, Richard Hughes
// Copyright � 2002-2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004-2008 Joe Kucera
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
//  Declaration for handlers of Channel 2 SNAC Families
//
// -----------------------------------------------------------------------------

#ifndef __FAMILIES_H
#define __FAMILIES_H


struct snac_header 
{
  BOOL  bValid;
  WORD  wFamily;
  WORD  wSubtype;
  WORD  wFlags;
  DWORD dwRef;
  WORD  wVersion;
};


struct message_ack_params 
{
  BYTE bType;
  DWORD dwUin;
  DWORD dwMsgID1;
  DWORD dwMsgID2;
  directconnect *pDC;
  WORD wCookie;
  int msgType;
  BYTE bFlags;
};

#define MAT_SERVER_ADVANCED 0
#define MAT_DIRECT          1


struct UserInfoRecordItem 
{
  WORD wTLV;
  int dbType;
  char *szDbSetting;
};

/*---------* Functions *---------------*/

void sendClientAuth(const char* szKey, WORD wKeyLen, BOOL bSecure);
void handleLoginReply(unsigned char *buf, WORD datalen, serverthread_info *info);

void handleServUINSettings(int nPort, serverthread_info *info);
int getPluginTypeIdLen(int nTypeID);
void packPluginTypeId(icq_packet *packet, int nTypeID);
int unpackPluginTypeId(BYTE** pBuffer, WORD* pwLen, int *pTypeId, WORD *pFunctionId, BOOL bThruDC);

void handleMessageTypes(DWORD dwUin, DWORD dwTimestamp, DWORD dwMsgID, DWORD dwMsgID2, WORD wCookie, WORD wVersion, int type, int flags, WORD wAckType, DWORD dwDataLen, WORD wMsgLen, char *pMsg, BOOL bThruDC, message_ack_params *pAckParams);

#define BUL_ALLCONTACTS   0
#define BUL_VISIBLE       1
#define BUL_INVISIBLE     2
#define BUL_TEMPVISIBLE   4


#endif /* __FAMILIES_H */
