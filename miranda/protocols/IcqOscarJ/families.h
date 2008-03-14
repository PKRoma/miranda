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
//  Declaration for handlers of Channel 2 SNAC Families
//
// -----------------------------------------------------------------------------

#ifndef __FAMILIES_H
#define __FAMILIES_H


typedef struct snac_header_s
{
  BOOL  bValid;
  WORD  wFamily;
  WORD  wSubtype;
  WORD  wFlags;
  DWORD dwRef;
  WORD  wVersion;
} snac_header;


typedef struct message_ack_params_s
{
  BYTE bType;
  DWORD dwUin;
  DWORD dwMsgID1;
  DWORD dwMsgID2;
  directconnect *pDC;
  WORD wCookie;
  int msgType;
  BYTE bFlags;
} message_ack_params;

#define MAT_SERVER_ADVANCED 0
#define MAT_DIRECT          1

/*---------* Functions *---------------*/

void handleServiceFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader, serverthread_info *info);
void handleLocationFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader);
void handleBuddyFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader, serverthread_info *info);
void handleMsgFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader);
void handleBosFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader);
void handleLookupFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader);
void handleStatusFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader);
void handleServClistFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader, serverthread_info *info);
void handleIcqExtensionsFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader);
void handleAuthorizationFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader, serverthread_info *info);

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
void sendEntireListServ(WORD wFamily, WORD wSubtype, int listType);
void updateServVisibilityCode(BYTE bCode);
void updateServAvatarHash(BYTE *pHash, int size);
void sendAddStart(int bImport);
void sendAddEnd(void);
void sendTypingNotification(HANDLE hContact, WORD wMTNCode);

void makeContactTemporaryVisible(HANDLE hContact);
void clearTemporaryVisibleList();


#endif /* __FAMILIES_H */
