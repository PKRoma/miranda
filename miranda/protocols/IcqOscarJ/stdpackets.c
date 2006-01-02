// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright � 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001,2002 Jon Keating, Richard Hughes
// Copyright � 2002,2003,2004 Martin  berg, Sam Kothari, Robert Rainwater
// Copyright � 2004,2005 Joe Kucera
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

#include "icqoscar.h"



extern int gbIdleAllow;
extern DWORD dwLocalInternalIP;
extern DWORD dwLocalExternalIP;
extern WORD wListenPort;
extern HANDLE hsmsgrequest;
extern CRITICAL_SECTION modeMsgsMutex;

static DWORD sendTLVSearchPacket(BYTE bType, char *pSearchDataBuf, WORD wSearchType, WORD wInfoLen, BOOL bOnlineUsersOnly);



/*****************************************************************************
 *
 *   Some handy extra pack functions for basic message type headers
 *
 */



// This is the part of the message header that is common for all message channels
static void packServMsgSendHeader(icq_packet *p, DWORD dwSequence, DWORD dwID1, DWORD dwID2, DWORD dwUin, char *szUID, WORD wFmt, WORD wLen)
{
  unsigned char nUinLen;

  nUinLen = getUIDLen(dwUin, szUID);

  serverPacketInit(p, (WORD)(21 + nUinLen + wLen));
  packFNACHeaderFull(p, ICQ_MSG_FAMILY, ICQ_MSG_SRV_SEND, 0, dwSequence | ICQ_MSG_SRV_SEND<<0x10);
  packLEDWord(p, dwID1);         // Msg ID part 1
  packLEDWord(p, dwID2);         // Msg ID part 2
  packWord(p, wFmt);             // Message channel
  packUID(p, dwUin, szUID);      // User ID
}



static void packServIcqExtensionHeader(icq_packet *p, WORD wLen, WORD wType, WORD wSeq)
{
  serverPacketInit(p, (WORD)(24 + wLen));
  packFNACHeaderFull(p, ICQ_EXTENSIONS_FAMILY, CLI_META_REQ, 0, wSeq | CLI_META_REQ<<0x10);
  packWord(p, 0x01);               // TLV type 1
  packWord(p, (WORD)(10 + wLen));  // TLV len
  packLEWord(p, (WORD)(8 + wLen)); // Data chunk size (TLV.Length-2)
  packLEDWord(p, dwLocalUIN);      // My UIN
  packLEWord(p, wType);            // Request type
  packWord(p, wSeq);
}



static void packServTLV5Header(icq_packet *p, WORD wLen, DWORD TS1, DWORD TS2, WORD wAckType)
{
  // TLV(5) header
  packWord(p, 0x05);              // Type
  packWord(p, (WORD)(36 + wLen));  // Len
  // TLV(5) data
  packWord(p, 0);                  // Command
  packLEDWord(p, TS1);            // msgid1
  packLEDWord(p, TS2);            // msgid2
  packGUID(p, MCAP_TLV2711_FMT);  // capabilities (4 dwords)
  packDWord(p, 0x000A0002);       // TLV: 0x0A Acktype: 1 for normal, 2 for ack
  packWord(p, wAckType);
  packDWord(p, 0x000F0000);       // TLV: 0x0F empty
}



static void packServTLV2711Header(icq_packet *packet, WORD wCookie, BYTE bMsgType, BYTE bMsgFlags, WORD X1, WORD X2, int nLen)
{
  packWord(packet, 0x2711);            // Type
  packWord(packet, (WORD)(51 + nLen)); // Len
  // TLV(0x2711) data
  packLEWord(packet, 0x1B);            // Unknown
  packByte(packet, ICQ_VERSION);       // Client version
  packGUID(packet, PSIG_MESSAGE);
  packDWord(packet, CLIENTFEATURES);
  packDWord(packet, DC_TYPE);
  packLEWord(packet, wCookie);         // Reference cookie
  packLEWord(packet, 0x0E);            // Unknown
  packLEWord(packet, wCookie);         // Reference cookie again
  packDWord(packet, 0);                // Unknown (12 bytes)
  packDWord(packet, 0);                //  -
  packDWord(packet, 0);                //  -
  packByte(packet, bMsgType);          // Message type
  packByte(packet, bMsgFlags);         // Flags
  packLEWord(packet, X1);              // Accepted
  packWord(packet, X2);              // Unknown, priority?
}



static void packServChannel2Header(icq_packet *p, DWORD dwUin, WORD wLen, DWORD dwCookie, BYTE bMsgType, BYTE bMsgFlags, WORD wPriority, int isAck, int includeDcInfo, BYTE bRequestServerAck)
{
  DWORD dwID1;
  DWORD dwID2;

  dwID1 = time(NULL);
  dwID2 = RandRange(0, 0x00FF);

  packServMsgSendHeader(p, dwCookie, dwID1, dwID2, dwUin, NULL, 0x0002,
    (WORD)(wLen + 95 + (bRequestServerAck?4:0) + (includeDcInfo?14:0)));

  packWord(p, 0x05);      /* TLV type */
  packWord(p, (WORD)(wLen + 91 + (includeDcInfo?14:0)));  /* TLV len */
  packWord(p, (WORD)(isAck ? 2: 0));     /* not aborting anything */
  packLEDWord(p, dwID1);   // Msg ID part 1
  packLEDWord(p, dwID2);   // Msg ID part 2
  packGUID(p, MCAP_TLV2711_FMT); /* capability (4 dwords) */
  packDWord(p, 0x000A0002);  /* TLV: 0x0A WORD: 1 for normal, 2 for ack */
  packWord(p, (WORD)(isAck ? 2 : 1));

  if (includeDcInfo)
  {
    packDWord(p, 0x00050002); // TLV: 0x05 Listen port
    packWord(p, wListenPort);
    packDWord(p, 0x00030004); // TLV: 0x03 DWORD IP
    packDWord(p, dwLocalInternalIP);
  }

  packDWord(p, 0x000F0000);    /* TLV: 0x0F empty */

  packServTLV2711Header(p, (WORD)dwCookie, bMsgType, bMsgFlags, (WORD)MirandaStatusToIcq(gnCurrentStatus), wPriority, wLen);
}



static void packServAdvancedMsgReply(icq_packet *p, DWORD dwUin, DWORD dwTimestamp, DWORD dwTimestamp2, WORD wCookie, BYTE bMsgType, BYTE bMsgFlags, WORD wLen)
{
  unsigned char nUinLen;

  nUinLen = getUINLen(dwUin);

  serverPacketInit(p, (WORD)(nUinLen + 74 + wLen));
  packFNACHeaderFull(p, ICQ_MSG_FAMILY, ICQ_MSG_RESPONSE, 0, ICQ_MSG_RESPONSE<<0x10 | (wCookie & 0x7FFF));
  packLEDWord(p, dwTimestamp);   // Msg ID part 1
  packLEDWord(p, dwTimestamp2);  // Msg ID part 2
  packWord(p, 0x02);             // Channel
  packUIN(p, dwUin);             // Your UIN
  packWord(p, 0x03);             // Unknown
  packLEWord(p, 0x1B);           // Unknown
  packByte(p, ICQ_VERSION);      // Protocol version
  packGUID(p, PSIG_MESSAGE);
  packDWord(p, CLIENTFEATURES);
  packDWord(p, DC_TYPE);
  packLEWord(p, wCookie);  // Reference
  packLEWord(p, 0x0E);     // Unknown
  packLEWord(p, wCookie);  // Reference
  packDWord(p, 0);         // Unknown
  packDWord(p, 0);         // Unknown
  packDWord(p, 0);         // Unknown
  packByte(p, bMsgType);   // Message type
  packByte(p, bMsgFlags);  // Message flags
  packLEWord(p, 0);        // Ack status code ( 0 = accepted, this is hardcoded because
                           //                   it is only used this way yet)
  packLEWord(p, 0);        // Unused priority field
}



void packMsgColorInfo(icq_packet *packet)
{ // TODO: make configurable
  packLEDWord(packet, 0x00000000);   // Foreground colour
  packLEDWord(packet, 0x00FFFFFF);   // Background colour
}



void packEmptyMsg(icq_packet *packet)
{
  packLEWord(packet, 1);
  packByte(packet, 0);
}


/*****************************************************************************
 *
 *   Functions to actually send the stuff
 *
 */

void icq_sendCloseConnection()
{
  icq_packet packet;

  packet.wLen = 0;
  write_flap(&packet, ICQ_CLOSE_CHAN);
  sendServPacket(&packet);
}



void icq_requestnewfamily(WORD wFamily, void (*familyhandler)(HANDLE hConn, char* cookie, WORD cookieLen))
{
  icq_packet packet;
  DWORD dwIdent;
  familyrequest_rec* request;

  request = (familyrequest_rec*)malloc(sizeof(familyrequest_rec));
  request->wFamily = wFamily;
  request->familyhandler = familyhandler;

  dwIdent = AllocateCookie(ICQ_CLIENT_NEW_SERVICE, 0, request); // generate and alloc cookie

  serverPacketInit(&packet, 12);
  packFNACHeaderFull(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_NEW_SERVICE, 0, dwIdent);
  packWord(&packet, wFamily);

  sendServPacket(&packet);
}



void icq_setidle(int bAllow)
{
  icq_packet packet;

  if (bAllow!=gbIdleAllow)
  {
    /* SNAC 1,11 */
    serverPacketInit(&packet, 14);
    packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_SET_IDLE);
    if (bAllow==1)
    {
      packDWord(&packet, 0x0000003C);
    }
    else
    {
      packDWord(&packet, 0x00000000);
    }

    sendServPacket(&packet);
    gbIdleAllow = bAllow;
  }
}



void icq_setstatus(WORD wStatus)
{
  icq_packet packet;

  // Pack data in packet
  serverPacketInit(&packet, 18);
  packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_SET_STATUS);
  packWord(&packet, 0x06);                // TLV 6
  packWord(&packet, 0x04);                // TLV length
  packWord(&packet, GetMyStatusFlags());  // Status flags
  packWord(&packet, wStatus);             // Status

  // Send packet
  sendServPacket(&packet);
}



DWORD icq_SendChannel1Message(DWORD dwUin, char *szUID, HANDLE hContact, char *pszText, message_cookie_data *pCookieData)
{
  icq_packet packet;
  WORD wMessageLen;
  DWORD dwCookie;
  WORD wPacketLength;
  DWORD dwID1, dwID2;


  wMessageLen = strlennull(pszText);
  dwCookie = AllocateCookie(0, dwUin, (void*)pCookieData);
  dwID1 = time(NULL);
  dwID2 = RandRange(0, 0x00FF);

  if (pCookieData->nAckType == ACKTYPE_SERVER)
    wPacketLength = 25;
  else
    wPacketLength = 21;

  // Pack the standard header
  packServMsgSendHeader(&packet, dwCookie, dwID1, dwID2, dwUin, szUID, 1, (WORD)(wPacketLength + wMessageLen));

  // Pack first TLV
  packWord(&packet, 0x0002); // TLV(2)
  packWord(&packet, (WORD)(wMessageLen + 13)); // TLV len

  // Pack client features
  packWord(&packet, 0x0501); // TLV(501)
  packWord(&packet, 0x0001); // TLV len
  packByte(&packet, 0x1);    // Features, meaning unknown, duplicated from ICQ Lite

  // Pack text TLV
  packWord(&packet, 0x0101); // TLV(2)
  packWord(&packet, (WORD)(wMessageLen + 4)); // TLV len
  packWord(&packet, 0x0003); // Message charset number, again copied from ICQ Lite
  packWord(&packet, 0x0000); // Message charset subset
  packBuffer(&packet, pszText, (WORD)(wMessageLen)); // Message text

  // Pack request server ack TLV
  if (pCookieData->nAckType == ACKTYPE_SERVER)
  {
    packDWord(&packet, 0x00030000); // TLV(3)
  }

  // Pack store on server TLV
  packDWord(&packet, 0x00060000); // TLV(6)

  sendServPacket(&packet);

  return dwCookie;
}



DWORD icq_SendChannel1MessageW(DWORD dwUin, char *szUID, HANDLE hContact, wchar_t *pszText, message_cookie_data *pCookieData)
{
  icq_packet packet;
  WORD wMessageLen;
  DWORD dwCookie;
  WORD wPacketLength;
  DWORD dwID1, dwID2;
  wchar_t* ppText;
  int i;


  wMessageLen = wcslen(pszText)*sizeof(wchar_t);
  dwCookie = AllocateCookie(0, dwUin, (void*)pCookieData);
  dwID1 = time(NULL);
  dwID2 = RandRange(0, 0x00FF);

  if (pCookieData->nAckType == ACKTYPE_SERVER)
    wPacketLength = 26;
  else
    wPacketLength = 22;

  // Pack the standard header
  packServMsgSendHeader(&packet, dwCookie, dwID1, dwID2, dwUin, szUID, 1, (WORD)(wPacketLength + wMessageLen));

  // Pack first TLV
  packWord(&packet, 0x0002); // TLV(2)
  packWord(&packet, (WORD)(wMessageLen + 14)); // TLV len

  // Pack client features
  packWord(&packet, 0x0501); // TLV(501)
  packWord(&packet, 0x0002); // TLV len
  packWord(&packet, 0x0106); // Features, meaning unknown, duplicated from ICQ 2003b

  // Pack text TLV
  packWord(&packet, 0x0101); // TLV(2)
  packWord(&packet, (WORD)(wMessageLen + 4)); // TLV len
  packWord(&packet, 0x0002); // Message charset number, again copied from ICQ 2003b
  packWord(&packet, 0x0000); // Message charset subset
  ppText = pszText;   // we must convert the widestring
  for (i = 0; i<wMessageLen; i+=2, ppText++)
  {
    packWord(&packet, *ppText);
  }

  // Pack request server ack TLV
  if (pCookieData->nAckType == ACKTYPE_SERVER)
  {
    packDWord(&packet, 0x00030000); // TLV(3)
  }

  // Pack store on server TLV
  packDWord(&packet, 0x00060000); // TLV(6)

  sendServPacket(&packet);

  return dwCookie;
}



DWORD icq_SendChannel2Message(DWORD dwUin, const char *szMessage, int nBodyLen, WORD wPriority, message_cookie_data *pCookieData, char *szCap)
{
  icq_packet packet;
  DWORD dwCookie;


  dwCookie = AllocateCookie(0, dwUin, (void*)pCookieData);

  // Pack the standard header
  packServChannel2Header(&packet, dwUin, (WORD)(nBodyLen + (szCap ? 53:11)), dwCookie, pCookieData->bMessageType, 0,
    wPriority, 0, 0, (BYTE)((pCookieData->nAckType == ACKTYPE_SERVER)?1:0));

  packLEWord(&packet, (WORD)(nBodyLen+1));            // Length of message
  packBuffer(&packet, szMessage, (WORD)(nBodyLen+1)); // Message
  packMsgColorInfo(&packet);

  if (szCap)
  {
    packLEDWord(&packet, 0x00000026);                 // length of GUID
    packBuffer(&packet, szCap, 0x26);                 // UTF-8 GUID
  }

  // Pack request server ack TLV
  if (pCookieData->nAckType == ACKTYPE_SERVER)
  {
    packDWord(&packet, 0x00030000); // TLV(3)
  }

  sendServPacket(&packet);

  return dwCookie;
}



DWORD icq_SendChannel4Message(DWORD dwUin, BYTE bMsgType, WORD wMsgLen, const char *szMsg, message_cookie_data *pCookieData)
{
  icq_packet packet;
  DWORD dwID1;
  DWORD dwID2;
  WORD wPacketLength;
  DWORD dwCookie;


  dwCookie = AllocateCookie(0, dwUin, (void*)pCookieData);

  if (pCookieData->nAckType == ACKTYPE_SERVER)
    wPacketLength = 28;
  else
    wPacketLength = 24;

  dwID1 = time(NULL);
  dwID2 = RandRange(0, 0x00FF);

  // Pack the standard header
  packServMsgSendHeader(&packet, dwCookie, dwID1, dwID2, dwUin, NULL, 4, (WORD)(wPacketLength + wMsgLen));

  // Pack first TLV
  packWord(&packet, 0x05);                 // TLV(5)
  packWord(&packet, (WORD)(wMsgLen + 16)); // TLV len
  packLEDWord(&packet, dwLocalUIN);        // My UIN
  packByte(&packet, bMsgType);             // Message type
  packByte(&packet, 0);                    // Message flags
  packLEWord(&packet, wMsgLen);            // Message length
  packBuffer(&packet, szMsg, wMsgLen);     // Message text
  packMsgColorInfo(&packet);

  // Pack request ack TLV
  if (pCookieData->nAckType == ACKTYPE_SERVER)
  {
    packDWord(&packet, 0x00030000); // TLV(3)
  }

  // Pack store on server TLV
  packDWord(&packet, 0x00060000); // TLV(6)

  sendServPacket(&packet);

  return dwCookie;
}



void sendOwnerInfoRequest(void)
{
  icq_packet packet;
  DWORD dwCookie;
  fam15_cookie_data *pCookieData = NULL;


  pCookieData = malloc(sizeof(fam15_cookie_data));
  pCookieData->bRequestType = REQUESTTYPE_OWNER;
  dwCookie = AllocateCookie(0, dwLocalUIN, (void*)pCookieData);

  packServIcqExtensionHeader(&packet, 6, 0x07D0, (WORD)dwCookie);
  packLEWord(&packet, META_REQUEST_SELF_INFO);
  packLEDWord(&packet, dwLocalUIN);

  sendServPacket(&packet);
}



void sendUserInfoAutoRequest(DWORD dwUin)
{
  icq_packet packet;
  DWORD dwCookie;
  fam15_cookie_data *pCookieData = NULL;


  pCookieData = malloc(sizeof(fam15_cookie_data));
  pCookieData->bRequestType = REQUESTTYPE_USERAUTO;
  dwCookie = AllocateCookie(0, dwUin, (void*)pCookieData);

  packServIcqExtensionHeader(&packet, 6, 0x07D0, (WORD)dwCookie);
  packLEWord(&packet, META_REQUEST_SHORT_INFO);
  packLEDWord(&packet, dwUin);

  sendServPacket(&packet);
}



DWORD icq_sendGetInfoServ(DWORD dwUin, int bMinimal)
{
  icq_packet packet;
  DWORD dwCookie;
  fam15_cookie_data *pCookieData = NULL;

  // we request if we can or 10sec after last request
  if (gbOverRate && (GetTickCount()-gtLastRequest)<10000) return 0;
  gbOverRate = 0;
  gtLastRequest = GetTickCount();

  pCookieData = malloc(sizeof(fam15_cookie_data));
  dwCookie = AllocateCookie(0, dwUin, (void*)pCookieData);

  packServIcqExtensionHeader(&packet, 6, CLI_META_INFO_REQ, (WORD)dwCookie);
  if (bMinimal)
  {
    pCookieData->bRequestType = REQUESTTYPE_USERMINIMAL;
    packLEWord(&packet, META_REQUEST_SHORT_INFO);
  }
  else
  {
    pCookieData->bRequestType = REQUESTTYPE_USERDETAILED;
    packLEWord(&packet, META_REQUEST_FULL_INFO);
  }
  packLEDWord(&packet, dwUin);

  sendServPacket(&packet);

  return dwCookie;
}



DWORD icq_sendGetAimProfileServ(HANDLE hContact, char* szUid)
{
  icq_packet packet;
  DWORD dwCookie;
  fam15_cookie_data *pCookieData = NULL;
  BYTE bUIDlen = strlennull(szUid);

  pCookieData = malloc(sizeof(fam15_cookie_data));
  dwCookie = AllocateCookie(ICQ_LOCATION_REQ_USER_INFO, 0, (void*)pCookieData);
  pCookieData->bRequestType = REQUESTTYPE_PROFILE;
  pCookieData->hContact = hContact;

  serverPacketInit(&packet, (WORD)(13 + bUIDlen));
  packFNACHeaderFull(&packet, ICQ_LOCATION_FAMILY, ICQ_LOCATION_REQ_USER_INFO, 0, dwCookie);
  packWord(&packet, 0x01); // request profile info
  packByte(&packet, bUIDlen);
  packBuffer(&packet, szUid, bUIDlen);

  sendServPacket(&packet);

  return dwCookie;
}



DWORD icq_sendGetAwayMsgServ(DWORD dwUin, int type)
{
  icq_packet packet;
  DWORD dwCookie;
  message_cookie_data *pCookieData = NULL;

  // we request if we can or 10sec after last request
  if (gbOverRate && (GetTickCount()-gtLastRequest)<10000) return 0;
  gbOverRate = 0;
  gtLastRequest = GetTickCount();

  pCookieData = malloc(sizeof(message_cookie_data));
  dwCookie = AllocateCookie(0, dwUin, (void*)pCookieData);
  pCookieData->bMessageType = MTYPE_AUTOAWAY;
  pCookieData->nAckType = (BYTE)type;

  packServChannel2Header(&packet, dwUin, 3, dwCookie, (BYTE)type, 3, 1, 0, 0, 0);
  packEmptyMsg(&packet);    // Message
  sendServPacket(&packet);

  return dwCookie;
}



DWORD icq_sendGetAimAwayMsgServ(char *szUID, int type)
{
  icq_packet packet;
  DWORD dwCookie;
  message_cookie_data *pCookieData = NULL;
  BYTE bUIDlen = strlennull(szUID);

  pCookieData = malloc(sizeof(message_cookie_data));
  dwCookie = AllocateCookie(0, 0, (void*)pCookieData);
  pCookieData->bMessageType = MTYPE_AUTOAWAY;
  pCookieData->nAckType = (BYTE)type;

  serverPacketInit(&packet, (WORD)(13 + bUIDlen));
  packFNACHeaderFull(&packet, ICQ_LOCATION_FAMILY, ICQ_LOCATION_REQ_USER_INFO, 0, dwCookie);
  packWord(&packet, 0x03);
  packByte(&packet, bUIDlen);
  packBuffer(&packet, szUID, bUIDlen);

  sendServPacket(&packet);

  return dwCookie;
}



void icq_sendSetAimAwayMsgServ(char *szMsg)
{
  icq_packet packet;
  DWORD dwCookie;
  WORD wMsgLen = strlennull(szMsg);

  dwCookie = GenerateCookie(ICQ_LOCATION_SET_USER_INFO);

  if (wMsgLen > 0x1000) wMsgLen = 0x1000; // limit length
  serverPacketInit(&packet, (WORD)(51 + wMsgLen));
  packFNACHeaderFull(&packet, ICQ_LOCATION_FAMILY, ICQ_LOCATION_SET_USER_INFO, 0, dwCookie);
  packTLV(&packet, 0x03, 0x21, "text/x-aolrtf; charset=\"us-ascii\"");
  packTLV(&packet, 0x04, wMsgLen, szMsg);

  sendServPacket(&packet);
}



void icq_sendFileSendServv7(DWORD dwUin, DWORD dwCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize)
{
  icq_packet packet;
  WORD wDescrLen,wFilesLen;

  wDescrLen = strlennull(szDescr);
  wFilesLen = strlennull(szFiles);

  packServChannel2Header(&packet, dwUin, (WORD)(18 + wDescrLen + wFilesLen), dwCookie, MTYPE_FILEREQ, 0, 1, 0, 1, 1);

  packLEWord(&packet, (WORD)(wDescrLen + 1));
  packBuffer(&packet, szDescr, (WORD)(wDescrLen + 1));
  packLEDWord(&packet, 0);   // unknown
  packLEWord(&packet, (WORD)(wFilesLen + 1));
  packBuffer(&packet, szFiles, (WORD)(wFilesLen + 1));
  packLEDWord(&packet, dwTotalSize);
  packLEDWord(&packet, 0);   // unknown

  sendServPacket(&packet);
}



void icq_sendFileSendServv8(DWORD dwUin, DWORD dwCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize, int nAckType)
{
  icq_packet packet;
  DWORD dwMsgID1;
  DWORD dwMsgID2;
  WORD wFlapLen;
  WORD wDescrLen,wFilesLen;

  wDescrLen = strlennull(szDescr);
  wFilesLen = strlennull(szFiles);

  dwMsgID1 = time(NULL);
  dwMsgID2 = RandRange(0, 0x00FF);

  // 202 + UIN len + file description (no null) + file name (null included)
  // Packet size = Flap length + 4
  wFlapLen = 178 + wDescrLen + wFilesLen + (nAckType == ACKTYPE_SERVER?4:0);
  packServMsgSendHeader(&packet, dwCookie, dwMsgID1, dwMsgID2, dwUin, NULL, 2, wFlapLen);

  // TLV(5) header
  packServTLV5Header(&packet, (WORD)(138 + wDescrLen + wFilesLen), dwMsgID1, dwMsgID2, 1); 

  packDWord(&packet, 0x00030004); // TLV: 0x03 DWORD IP
  packDWord(&packet, dwLocalInternalIP);
  packDWord(&packet, 0x00050002); // TLV: 0x05 Listen port
  packWord(&packet, wListenPort);

  // TLV(0x2711) header
  packServTLV2711Header(&packet, (WORD)dwCookie, MTYPE_PLUGIN, 0, (WORD)MirandaStatusToIcq(gnCurrentStatus), 0x100, 69 + wDescrLen + wFilesLen);

  packEmptyMsg(&packet);  // Message (unused)

  packPluginTypeId(&packet, MTYPE_FILEREQ);

  packLEDWord(&packet, (WORD)(18 + wDescrLen + wFilesLen + 1)); // Remaining length
  packLEDWord(&packet, wDescrLen);          // Description
  packBuffer(&packet, szDescr, wDescrLen);
  packWord(&packet, 0x8c82); // Unknown (port?), seen 0x80F6
  packWord(&packet, 0x0222); // Unknown, seen 0x2e01
  packLEWord(&packet, (WORD)(wFilesLen + 1));
  packBuffer(&packet, szFiles, (WORD)(wFilesLen + 1));
  packLEDWord(&packet, dwTotalSize);
  packLEDWord(&packet, 0x0008c82); // Unknown, (seen 0xf680 ~33000)

  // Pack request server ack TLV
  if (nAckType == ACKTYPE_SERVER)
  {
    packDWord(&packet, 0x00030000); // TLV(3)
  }

  // Send the monster
  sendServPacket(&packet);
}



/* also sends rejections */
void icq_sendFileAcceptServv8(DWORD dwUin, DWORD TS1, DWORD TS2, DWORD dwCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize, WORD wPort, BOOL accepted, int nAckType)
{
  icq_packet packet;
  WORD wFlapLen;
  WORD wDescrLen,wFilesLen;

  /* if !accepted, szDescr == szReason, szFiles = "" */

  if (!accepted) szFiles = "";

  wDescrLen = strlennull(szDescr);
  wFilesLen = strlennull(szFiles);

  // 202 + UIN len + file description (no null) + file name (null included)
  // Packet size = Flap length + 4
  wFlapLen = 178 + wDescrLen + wFilesLen + (nAckType == ACKTYPE_SERVER?4:0);
  packServMsgSendHeader(&packet, dwCookie, TS1, TS2, dwUin, NULL, 2, wFlapLen);

  // TLV(5) header
  packServTLV5Header(&packet, (WORD)(138 + wDescrLen + wFilesLen), TS1, TS2, 2); 

  packDWord(&packet, 0x00030004); // TLV: 0x03 DWORD IP
  packDWord(&packet, accepted ? dwLocalInternalIP : 0);
  packDWord(&packet, 0x00050002); // TLV: 0x05 Listen port
  packWord(&packet, (WORD) (accepted ? wListenPort : 0));

  // TLV(0x2711) header
  packServTLV2711Header(&packet, (WORD)dwCookie, MTYPE_PLUGIN, 0, (WORD)(accepted ? 0:1), 0, 69 + wDescrLen + wFilesLen);
  //
  packEmptyMsg(&packet);    // Message (unused)

  packPluginTypeId(&packet, MTYPE_FILEREQ);

  packLEDWord(&packet, (WORD)(18 + wDescrLen + wFilesLen + 1)); // Remaining length
  packLEDWord(&packet, wDescrLen);          // Description
  packBuffer(&packet, szDescr, wDescrLen);
  packWord(&packet, wPort); // Port
  packWord(&packet, 0x00);  // Unknown
  packLEWord(&packet, (WORD)(wFilesLen + 1));
  packBuffer(&packet, szFiles, (WORD)(wFilesLen + 1));
  packLEDWord(&packet, dwTotalSize);
  packLEDWord(&packet, (DWORD)wPort); // Unknown

  // Pack request server ack TLV
  if (nAckType == ACKTYPE_SERVER)
  {
    packDWord(&packet, 0x00030000); // TLV(3)
  }

  // Send the monster
  sendServPacket(&packet);
}



void icq_sendFileAcceptServv7(DWORD dwUin, DWORD TS1, DWORD TS2, DWORD dwCookie, const char* szFiles, const char* szDescr, DWORD dwTotalSize, WORD wPort, BOOL accepted, int nAckType)
{
  icq_packet packet;
  WORD wFlapLen;
  WORD wDescrLen,wFilesLen;

  /* if !accepted, szDescr == szReason, szFiles = "" */

  if (!accepted) szFiles = "";

  wDescrLen = strlennull(szDescr);
  wFilesLen = strlennull(szFiles);

  // 150 + UIN len + file description (with null) + file name (2 nulls)
  // Packet size = Flap length + 4
  wFlapLen = 127 + wDescrLen + 1 + wFilesLen + (nAckType == ACKTYPE_SERVER?4:0);
  packServMsgSendHeader(&packet, dwCookie, TS1, TS2, dwUin, NULL, 2, wFlapLen);

  // TLV(5) header
  packServTLV5Header(&packet, (WORD)(88 + wDescrLen + wFilesLen), TS1, TS2, 2); 

  packDWord(&packet, 0x00030004); // TLV: 0x03 DWORD IP
  packDWord(&packet, (accepted ? dwLocalInternalIP : 0));
  packDWord(&packet, 0x00050002); // TLV: 0x05 Listen port
  packWord(&packet, (WORD)(accepted ? wListenPort : 0));

  // TLV(0x2711) header
  packServTLV2711Header(&packet, (WORD)dwCookie, MTYPE_FILEREQ, 0, (WORD)(accepted ? 0:1), 0, 19 + wDescrLen + wFilesLen);
  //
  packLEWord(&packet, (WORD)(wDescrLen + 1));     // Description
  packBuffer(&packet, szDescr, (WORD)(wDescrLen + 1));
  packWord(&packet, wPort);   // Port
  packWord(&packet, 0x00);    // Unknown
  packLEWord(&packet, (WORD)(wFilesLen + 2));
  packBuffer(&packet, szFiles, (WORD)(wFilesLen + 1));
  packByte(&packet, 0);
  packLEDWord(&packet, dwTotalSize);
  packLEDWord(&packet, (DWORD)wPort); // Unknown

  // Pack request server ack TLV
  if (nAckType == ACKTYPE_SERVER)
  {
    packDWord(&packet, 0x00030000); // TLV(3)
  }

  // Send the monster
  sendServPacket(&packet);
}



void icq_sendFileAcceptServ(DWORD dwUin, filetransfer* ft, int nAckType)
{
  char *szDesc = ft->szDescription;

  if (ft->bEmptyDesc) szDesc = ""; // keep empty if it originally was (Trillian workaround)

  if (ft->nVersion >= 8)
  {
    icq_sendFileAcceptServv8(dwUin, ft->TS1, ft->TS2, ft->dwCookie, ft->szFilename, szDesc, ft->dwTotalSize, wListenPort, TRUE, nAckType);
    NetLog_Server("Sent file accept v%u through server, port %u", 8, wListenPort);
  }
  else
  {
    icq_sendFileAcceptServv7(dwUin, ft->TS1, ft->TS2, ft->dwCookie, ft->szFilename, szDesc, ft->dwTotalSize, wListenPort, TRUE, nAckType);
    NetLog_Server("Sent file accept v%u through server, port %u", 7, wListenPort);
  }
}



void icq_sendFileDenyServ(DWORD dwUin, filetransfer* ft, char *szReason, int nAckType)
{
  if (ft->nVersion >= 8)
  {
    icq_sendFileAcceptServv8(dwUin, ft->TS1, ft->TS2, ft->dwCookie, ft->szFilename, szReason, ft->dwTotalSize, wListenPort, FALSE, nAckType);
    NetLog_Server("Sent file deny v%u through server", 8);
  }
  else
  {
    icq_sendFileAcceptServv7(dwUin, ft->TS1, ft->TS2, ft->dwCookie, ft->szFilename, szReason, ft->dwTotalSize, wListenPort, FALSE, nAckType);
    NetLog_Server("Sent file deny v%u through server", 7);
  }
}



void icq_sendAwayMsgReplyServ(DWORD dwUin, DWORD dwTimestamp, DWORD dwTimestamp2, WORD wCookie, BYTE msgType, const char** szMsg)
{
  icq_packet packet;
  WORD wMsgLen;
  HANDLE hContact;


  hContact = HContactFromUIN(dwUin, NULL);

  if (validateStatusMessageRequest(hContact, msgType))
  {
    NotifyEventHooks(hsmsgrequest, (WPARAM)msgType, (LPARAM)dwUin);

    EnterCriticalSection(&modeMsgsMutex);

    if (szMsg && *szMsg)
    {
      wMsgLen = strlennull(*szMsg);

      // limit msg len to max snac size - we get disconnected if exceeded
      if (wMsgLen > MAX_MESSAGESNACSIZE)
        wMsgLen = MAX_MESSAGESNACSIZE;

      packServAdvancedMsgReply(&packet, dwUin, dwTimestamp, dwTimestamp2, wCookie, msgType, 3, (WORD)(wMsgLen + 3));
      packLEWord(&packet, (WORD)(wMsgLen + 1));
      packBuffer(&packet, *szMsg, wMsgLen);
      packByte(&packet, 0);

      sendServPacket(&packet);
    }

    LeaveCriticalSection(&modeMsgsMutex);
  }
}



void icq_sendAdvancedMsgAck(DWORD dwUin, DWORD dwTimestamp, DWORD dwTimestamp2, WORD wCookie, BYTE bMsgType, BYTE bMsgFlags)
{
  icq_packet packet;

  packServAdvancedMsgReply(&packet, dwUin, dwTimestamp, dwTimestamp2, wCookie, bMsgType, bMsgFlags, 11);
  packEmptyMsg(&packet);       // Status message
  packMsgColorInfo(&packet);

  sendServPacket(&packet);
}



// Searches

DWORD SearchByUin(DWORD dwUin)
{
  DWORD dwCookie;
  WORD wInfoLen;
  icq_packet pBuffer; // I reuse the ICQ packet type as a generic buffer
                      // I should be ashamed! ;)

  // Calculate data size
  wInfoLen = 8;

  // Initialize our handy data buffer
  pBuffer.wPlace = 0;
  pBuffer.pData = (BYTE *)_alloca(wInfoLen);
  pBuffer.wLen = wInfoLen;

  // Initialize our handy data buffer
  packLEWord(&pBuffer, 0x0136);
  packLEWord(&pBuffer, 0x0004);
  packLEDWord(&pBuffer, dwUin);

  // Send it off for further packing
  dwCookie = sendTLVSearchPacket(SEARCHTYPE_UID, pBuffer.pData, META_SEARCH_UIN, wInfoLen, FALSE);

  return dwCookie;
}



DWORD SearchByNames(char* pszNick, char* pszFirstName, char* pszLastName)
{ // use generic TLV search like icq5 does
  DWORD dwCookie;
  WORD wInfoLen = 0;
  WORD wNickLen,wFirstLen,wLastLen;
  icq_packet pBuffer; // I reuse the ICQ packet type as a generic buffer
                      // I should be ashamed! ;)

  wNickLen = strlennull(pszNick);
  wFirstLen = strlennull(pszFirstName);
  wLastLen = strlennull(pszLastName);

  _ASSERTE(wFirstLen || wLastLen || wNickLen);


  // Calculate data size
  if (wFirstLen > 0)
    wInfoLen = wFirstLen + 7;
  if (wLastLen > 0)
    wInfoLen += wLastLen + 7;
  if (wNickLen > 0)
    wInfoLen += wNickLen + 7;

  // Initialize our handy data buffer
  pBuffer.wPlace = 0;
  pBuffer.pData = (BYTE*)_alloca(wInfoLen);
  pBuffer.wLen = wInfoLen;


  // Pack the search details
  if (wFirstLen > 0)
  {
    packLEWord(&pBuffer, 0x0140);
    packLEWord(&pBuffer, (WORD)(wFirstLen+3));
    packLEWord(&pBuffer, (WORD)(wFirstLen+1));
    packBuffer(&pBuffer, pszFirstName, wFirstLen);
    packByte(&pBuffer, 0);
  }

  if (wLastLen > 0)
  {
    packLEWord(&pBuffer, 0x014a);
    packLEWord(&pBuffer, (WORD)(wLastLen+3));
    packLEWord(&pBuffer, (WORD)(wLastLen+1));
    packBuffer(&pBuffer, pszLastName, wLastLen);
    packByte(&pBuffer, 0);
  }

  if (wNickLen > 0)
  {
    packLEWord(&pBuffer, 0x0154);
    packLEWord(&pBuffer, (WORD)(wNickLen+3));
    packLEWord(&pBuffer, (WORD)(wNickLen+1));
    packBuffer(&pBuffer, pszNick, wNickLen);
    packByte(&pBuffer, 0);
  }

  // Send it off for further packing
  dwCookie = sendTLVSearchPacket(SEARCHTYPE_NAMES, pBuffer.pData, META_SEARCH_GENERIC, wInfoLen, FALSE);

  return dwCookie;
}



DWORD SearchByEmail(char* pszEmail)
{
  DWORD dwCookie;
  WORD wInfoLen = 0;
  WORD wEmailLen;
  icq_packet pBuffer; // I reuse the ICQ packet type as a generic buffer
                      // I should be ashamed! ;)

  wEmailLen = strlennull(pszEmail);

  _ASSERTE(wEmailLen);

  if (wEmailLen > 0)
  {
    // Calculate data size
    wInfoLen = wEmailLen + 7;

    // Initialize our handy data buffer
    pBuffer.wPlace = 0;
    pBuffer.pData = (BYTE *)_alloca(wInfoLen);
    pBuffer.wLen = wInfoLen;

    // Pack the search details
    packLEWord(&pBuffer, 0x015E);
    packLEWord(&pBuffer, (WORD)(wEmailLen+3));
    packLEWord(&pBuffer, (WORD)(wEmailLen+1));
    packBuffer(&pBuffer, pszEmail, wEmailLen);
    packByte(&pBuffer, 0);

    // Send it off for further packing
    dwCookie = sendTLVSearchPacket(SEARCHTYPE_EMAIL, pBuffer.pData, META_SEARCH_EMAIL, wInfoLen, FALSE);
  }

  return dwCookie;
}



DWORD sendTLVSearchPacket(BYTE bType, char* pSearchDataBuf, WORD wSearchType, WORD wInfoLen, BOOL bOnlineUsersOnly)
{
  icq_packet packet;
  DWORD dwCookie;
  search_cookie* pCookie;

  _ASSERTE(pSearchDataBuf);
  _ASSERTE(wInfoLen >= 4);

  pCookie = (search_cookie*)malloc(sizeof(search_cookie));
  if (pCookie)
  {
    pCookie->bSearchType = bType;
    pCookie->szObject = NULL;
    pCookie->dwStatus = 0; // in progress
    pCookie->dwMainId = 0;
    dwCookie = AllocateCookie(0, 0, pCookie);
  }
  else
    return 0;

  // Pack headers
  packServIcqExtensionHeader(&packet, (WORD)(wInfoLen + (wSearchType==META_SEARCH_GENERIC?7:2)), CLI_META_INFO_REQ, (WORD)dwCookie);

  // Pack search type
  packLEWord(&packet, wSearchType);

  // Pack search data
  packBuffer(&packet, pSearchDataBuf, wInfoLen);

  if (wSearchType == META_SEARCH_GENERIC)
  { // Pack "Online users only" flag - only for generic search
    packLEWord(&packet, 0x0230);
    packLEWord(&packet, 0x0001);
    if (bOnlineUsersOnly)
      packByte(&packet, 0x0001);
    else
      packByte(&packet, 0x0000);
  }

  // Go!
  sendServPacket(&packet);

  return dwCookie;
}



DWORD icq_sendAdvancedSearchServ(BYTE* fieldsBuffer,int bufferLen)
{
  icq_packet packet;
  DWORD dwCookie;
  search_cookie* pCookie;

  pCookie = (search_cookie*)malloc(sizeof(search_cookie));
  if (pCookie)
  {
    pCookie->bSearchType = SEARCHTYPE_DETAILS;
    pCookie->szObject = NULL;
    pCookie->dwMainId = 0;
    dwCookie = AllocateCookie(0, 0, pCookie);
  }
  else
    return 0;

  packServIcqExtensionHeader(&packet, (WORD)(2 + bufferLen), CLI_META_INFO_REQ, (WORD)dwCookie);
  packLEWord(&packet, META_SEARCH_GENERIC);       /* subtype: full search */
  packBuffer(&packet, (const char*)fieldsBuffer, (WORD)bufferLen);

  sendServPacket(&packet);

  return dwCookie;
}



DWORD icq_searchAimByEmail(char* pszEmail, DWORD dwSearchId)
{
  icq_packet packet;
  DWORD dwCookie;
  search_cookie* pCookie;
  search_cookie* pMainCookie = NULL;
  WORD wEmailLen;

  if (!FindCookie(dwSearchId, NULL, &pCookie))
  {
    dwSearchId = 0;
    pCookie = (search_cookie*)malloc(sizeof(search_cookie));
    pCookie->bSearchType = SEARCHTYPE_EMAIL;
  }

  if (pCookie)
  {
    pCookie->dwMainId = dwSearchId;
    pCookie->szObject = null_strdup(pszEmail);
    dwCookie = AllocateCookie(ICQ_LOOKUP_REQUEST, 0, pCookie);
  }
  else
    return 0;

  wEmailLen = strlennull(pszEmail);
  serverPacketInit(&packet, (WORD)(10 + wEmailLen));
  packFNACHeaderFull(&packet, ICQ_LOOKUP_FAMILY, ICQ_LOOKUP_REQUEST, 0, dwCookie);
  packBuffer(&packet, pszEmail, wEmailLen);

  sendServPacket(&packet);

  return dwCookie;
}



DWORD icq_changeUserDetailsServ(WORD type, const unsigned char *pData, WORD wDataLen)
{
  icq_packet packet;
  DWORD dwCookie;


  dwCookie = GenerateCookie(0);

  packServIcqExtensionHeader(&packet, (WORD)(wDataLen + 2), 0x07D0, (WORD)dwCookie);
  packWord(&packet, type);
  packBuffer(&packet, pData, wDataLen);

  sendServPacket(&packet);

  return dwCookie;
}



DWORD icq_sendSMSServ(const char *szPhoneNumber, const char *szMsg)
{
  icq_packet packet;
  DWORD dwCookie;
  WORD wBufferLen;
  char* szBuffer = NULL;
  char* szMyNick = NULL;
  char szTime[30];
  time_t now;
  int nBufferSize;


  now = time(NULL);
  strftime(szTime, sizeof(szTime), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
                              /* Sun, 00 Jan 0000 00:00:00 GMT */

  szMyNick = null_strdup((char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)(HANDLE)NULL, 0));
  nBufferSize = 1 + strlennull(szMyNick) + strlennull(szPhoneNumber) + strlennull(szMsg) + sizeof("<icq_sms_message><destination></destination><text></text><codepage>1252</codepage><encoding>utf8</encoding><senders_UIN>0000000000</senders_UIN><senders_name></senders_name><delivery_receipt>Yes</delivery_receipt><time>Sun, 00 Jan 0000 00:00:00 GMT</time></icq_sms_message>");

  if (szBuffer = (char *)malloc(nBufferSize))
  {

    wBufferLen = null_snprintf(szBuffer, nBufferSize,
      "<icq_sms_message>"
      "<destination>"
            "%s"   /* phone number */
      "</destination>"
      "<text>"
            "%s"   /* body */
      "</text>"
      "<codepage>"
            "1252"
      "</codepage>"
      "<encoding>"
            "utf8"
      "</encoding>"
      "<senders_UIN>"
            "%u"  /* my UIN */
      "</senders_UIN>"
      "<senders_name>"
            "%s"  /* my nick */
      "</senders_name>"
      "<delivery_receipt>"
            "Yes"
      "</delivery_receipt>"
      "<time>"
            "%s"  /* time */
      "</time>"
      "</icq_sms_message>",
      szPhoneNumber, szMsg, dwLocalUIN, szMyNick, szTime);

    dwCookie = GenerateCookie(0);

    packServIcqExtensionHeader(&packet, (WORD)(wBufferLen + 27), 0x07D0, (WORD)dwCookie);
    packWord(&packet, 0x8214);     /* send sms */
    packWord(&packet, 1);
    packWord(&packet, 0x16);
    packDWord(&packet, 0);
    packDWord(&packet, 0);
    packDWord(&packet, 0);
    packDWord(&packet, 0);
    packWord(&packet, 0);
    packWord(&packet, (WORD)(wBufferLen + 1));
    packBuffer(&packet, szBuffer, (WORD)(1 + wBufferLen));

    sendServPacket(&packet);
  }
  else
  {
    dwCookie = 0;
  }

  SAFE_FREE(&szMyNick);
  SAFE_FREE(&szBuffer);


  return dwCookie;
}



void icq_sendNewContact(DWORD dwUin, char* szUid)
{
  icq_packet packet;
  int nUinLen;

  nUinLen = getUIDLen(dwUin, szUid);

  serverPacketInit(&packet, (WORD)(nUinLen + 11));
  packFNACHeader(&packet, ICQ_BUDDY_FAMILY, ICQ_USER_ADDTOLIST);
  packUID(&packet, dwUin, szUid);

  sendServPacket(&packet);
}



// list==0: visible list
// list==1: invisible list
void icq_sendChangeVisInvis(HANDLE hContact, DWORD dwUin, char* szUID, int list, int add)
{ // TODO: This needs grouping & rate management
  // Tell server to change our server-side contact visbility list
  if (gbSsiEnabled)
  {
    WORD wContactId;
    WORD wType;

    if (list == 0)
      wType = SSI_ITEM_PERMIT;
    else
      wType = SSI_ITEM_DENY;

    if (add)
    {
      servlistcookie* ack;
      DWORD dwCookie;
      
      if (wType == SSI_ITEM_DENY) 
      { // check if we should make the changes, this is 2nd level check
        if (ICQGetContactSettingWord(hContact, "SrvDenyId", 0) != 0)
          return;
      }
      else
      {
        if (ICQGetContactSettingWord(hContact, "SrvPermitId", 0) != 0)
          return;
      }

      // Add
      wContactId = GenerateServerId(SSIT_ITEM);

      if (!(ack = (servlistcookie*)malloc(sizeof(servlistcookie))))
      { // cookie failed, use old fake
        dwCookie = GenerateCookie(ICQ_LISTS_ADDTOLIST);
      }
      else
      {
        ack->dwAction = SSA_PRIVACY_ADD;
        ack->dwUin = dwUin;
        ack->szUID = null_strdup(szUID);
        ack->hContact = hContact;
        ack->wGroupId = 0;
        ack->wContactId = wContactId;

        dwCookie = AllocateCookie(ICQ_LISTS_ADDTOLIST, dwUin, ack);
      }
      
      icq_sendBuddyUtf(dwCookie, ICQ_LISTS_ADDTOLIST, dwUin, szUID, 0, wContactId, NULL, NULL, 0, wType);

      if (wType == SSI_ITEM_DENY)
        ICQWriteContactSettingWord(hContact, "SrvDenyId", wContactId);
      else
        ICQWriteContactSettingWord(hContact, "SrvPermitId", wContactId);
    }
    else
    {
      // Remove
      if (wType == SSI_ITEM_DENY)
        wContactId = ICQGetContactSettingWord(hContact, "SrvDenyId", 0);
      else
        wContactId = ICQGetContactSettingWord(hContact, "SrvPermitId", 0);

      if (wContactId)
      {
        servlistcookie* ack;
        DWORD dwCookie;

        if (!(ack = (servlistcookie*)malloc(sizeof(servlistcookie))))
        { // cookie failed, use old fake
          dwCookie = GenerateCookie(ICQ_LISTS_REMOVEFROMLIST);
        }
        else
        {
          ack->dwAction = SSA_PRIVACY_REMOVE; // remove privacy item
          ack->dwUin = dwUin;
          ack->szUID = null_strdup(szUID);
          ack->hContact = hContact;
          ack->wGroupId = 0;
          ack->wContactId = wContactId;

          dwCookie = AllocateCookie(ICQ_LISTS_REMOVEFROMLIST, dwUin, ack);
        }
        icq_sendBuddyUtf(dwCookie, ICQ_LISTS_REMOVEFROMLIST, dwUin, szUID, 0, wContactId, NULL, NULL, 0, wType);

        if (wType == SSI_ITEM_DENY)
          ICQDeleteContactSetting(hContact, "SrvDenyId");
        else
          ICQDeleteContactSetting(hContact, "SrvPermitId");
      }
    }
  }

  // Notify server that we have changed
  // our client side visibility list
  {
    int nUinLen;
    icq_packet packet;
    WORD wSnac;

    if (list && gnCurrentStatus == ID_STATUS_INVISIBLE)
      return;

    if (!list && gnCurrentStatus != ID_STATUS_INVISIBLE)
      return;


    if (list && add)
      wSnac = ICQ_CLI_ADDINVISIBLE;
    else if (list && !add)
      wSnac = ICQ_CLI_REMOVEINVISIBLE;
    else if (!list && add)
      wSnac = ICQ_CLI_ADDVISIBLE;
    else if (!list && !add)
      wSnac = ICQ_CLI_REMOVEVISIBLE;

    nUinLen = getUIDLen(dwUin, szUID);

    serverPacketInit(&packet, (WORD)(nUinLen + 11));
    packFNACHeader(&packet, ICQ_BOS_FAMILY, wSnac);
    packUID(&packet, dwUin, szUID);

    sendServPacket(&packet);
  }
}



void icq_sendEntireVisInvisList(int list)
{
  if (list)
    sendEntireListServ(ICQ_BOS_FAMILY, ICQ_CLI_ADDINVISIBLE, BUL_INVISIBLE);
  else
    sendEntireListServ(ICQ_BOS_FAMILY, ICQ_CLI_ADDVISIBLE, BUL_VISIBLE);
}



void icq_sendGrantAuthServ(DWORD dwUin, char *szUid, char *szMsg)
{
  icq_packet packet;
  unsigned char nUinlen;
  char* szUtfMsg = NULL;
  WORD nMsglen;

  nUinlen = getUIDLen(dwUin, szUid);

  // Prepare custom utf-8 message
  if (strlennull(szMsg) > 0)
  {
    int nResult;

    nResult = utf8_encode(szMsg, &szUtfMsg);
    nMsglen = strlennull(szUtfMsg);
  }
  else
  {
    nMsglen = 0;
  }

  serverPacketInit(&packet, (WORD)(15 + nUinlen + nMsglen));
  packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_GRANTAUTH);
  packUID(&packet, dwUin, szUid);
  packWord(&packet, (WORD)nMsglen);
  packBuffer(&packet, szUtfMsg, nMsglen);
  packWord(&packet, 0);

  sendServPacket(&packet);
}



void icq_sendAuthReqServ(DWORD dwUin, char *szMsg)
{
  icq_packet packet;
  unsigned char nUinlen;
  char* szUtfMsg = NULL;
  WORD nMsglen;

  nUinlen = getUINLen(dwUin);

  // Prepare custom utf-8 message
  if (strlennull(szMsg) > 0)
  {
    int nResult;

    nResult = utf8_encode(szMsg, &szUtfMsg);
    nMsglen = strlennull(szUtfMsg);
  }
  else
  {
    nMsglen = 0;
  }

  serverPacketInit(&packet, (WORD)(15 + nUinlen + nMsglen));
  packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_REQUESTAUTH);
  packUIN(&packet, dwUin);
  packWord(&packet, (WORD)nMsglen);
  packBuffer(&packet, szUtfMsg, nMsglen);
  packWord(&packet, 0);

  sendServPacket(&packet);
}



void icq_sendAuthResponseServ(DWORD dwUin, char* szUid, int auth, char *szReason)
{
  icq_packet p;
  WORD nReasonlen;
  unsigned char nUinlen;
  char* szUtfReason = NULL;

  nUinlen = getUINLen(dwUin);

  // Prepare custom utf-8 reason
  if (strlennull(szReason) > 0)
  {
    int nResult;

    nResult = utf8_encode(szReason, &szUtfReason);
    nReasonlen = strlennull(szUtfReason);
  }
  else
  {
    nReasonlen = 0;
  }

  serverPacketInit(&p, (WORD)(16 + nUinlen + nReasonlen));
  packFNACHeader(&p, ICQ_LISTS_FAMILY, ICQ_LISTS_CLI_AUTHRESPONSE);
  packUIN(&p, dwUin);
  packByte(&p, (BYTE)auth);
  packWord(&p, nReasonlen);
  packBuffer(&p, szUtfReason, nReasonlen);
  packWord(&p, 0);

  sendServPacket(&p);
}



void icq_sendYouWereAddedServ(DWORD dwUin, DWORD dwMyUin)
{
  icq_packet packet;
  DWORD dwID1;
  DWORD dwID2;

  dwID1 = time(NULL);
  dwID2 = RandRange(0, 0x00FF);

  packServMsgSendHeader(&packet, 0, dwID1, dwID2, dwUin, NULL, 0x0004, 17);
  packWord(&packet, 0x0005);      // TLV(5)
  packWord(&packet, 0x0009);
  packLEDWord(&packet, dwMyUin);
  packByte(&packet, MTYPE_ADDED);
  packByte(&packet, 0);           // msg-flags
  packEmptyMsg(&packet);          // NTS
  packDWord(&packet, 0x00060000); // TLV(6)

  sendServPacket(&packet);
}



void icq_sendXtrazRequestServ(DWORD dwUin, DWORD dwCookie, char* szBody, int nBodyLen, int nType)
{
  icq_packet packet;
  WORD wFlapLen;
  DWORD dwID1;
  DWORD dwID2;

  dwID1 = time(NULL);
  dwID2 = RandRange(0, 0x00FF);

  wFlapLen = getPluginTypeIdLen(nType) + 106 + nBodyLen + 4;
  packServMsgSendHeader(&packet, dwCookie, dwID1, dwID2, dwUin, NULL, 2, wFlapLen);

  // TLV(5) header
  packServTLV5Header(&packet, (WORD)(getPluginTypeIdLen(nType) + 66 + nBodyLen), dwID1, dwID2, 1); 

  // TLV(0x2711) header
  packServTLV2711Header(&packet, (WORD)dwCookie, MTYPE_PLUGIN, 0, 0, 0x100, 11 + getPluginTypeIdLen(nType) + nBodyLen);
  //
  packEmptyMsg(&packet);

  packPluginTypeId(&packet, nType);

  packLEDWord(&packet, nBodyLen + 4);
  packLEDWord(&packet, nBodyLen);
  packBuffer(&packet, szBody, (WORD)nBodyLen);

  // Pack request server ack TLV
  packDWord(&packet, 0x00030000); // TLV(3)

  // Send the monster
  sendServPacket(&packet);
}



void icq_sendXtrazResponseServ(DWORD dwUin, DWORD dwMID, DWORD dwMID2, WORD wCookie, char* szBody, int nBodyLen, int nType)
{
  icq_packet packet;

  packServAdvancedMsgReply(&packet, dwUin, dwMID, dwMID2, wCookie, MTYPE_PLUGIN, 0, (WORD)(getPluginTypeIdLen(nType) + 11 + nBodyLen));
  //
  packEmptyMsg(&packet);

  packPluginTypeId(&packet, nType);

  packLEDWord(&packet, nBodyLen + 4);
  packLEDWord(&packet, nBodyLen);
  packBuffer(&packet, szBody, (WORD)nBodyLen);

  // Send the monster
  sendServPacket(&packet);
}



void icq_sendReverseReq(DWORD dwUin, DWORD dwCookie, DWORD dwRemotePort)
{
  icq_packet packet;
  DWORD dwID1;
  DWORD dwID2;

  dwID1 = time(NULL);
  dwID2 = RandRange(0, 0x00FF);

  packServMsgSendHeader(&packet, dwCookie, dwID1, dwID2, dwUin, NULL, 2, 0x47);

  // TLV(5) header
  packWord(&packet, 0x05);              // Type
  packWord(&packet, 0x43);              // Len
  // TLV(5) data
  packWord(&packet, 0);                 // Command
  packLEDWord(&packet, dwID1);          // msgid1
  packLEDWord(&packet, dwID2);          // msgid2
  packGUID(&packet, MCAP_REVERSE_REQ);  // capabilities (4 dwords)
  packDWord(&packet, 0x000A0002);       // TLV: 0x0A Acktype: 1 for normal, 2 for ack
  packWord(&packet, 1);
  packDWord(&packet, 0x000F0000);       // TLV: 0x0F empty
  packDWord(&packet, 0x2711001B);       // TLV: 0x2711 Content
  // TLV(0x2711) data
  packLEDWord(&packet, dwLocalUIN);     // Our UIN
  packDWord(&packet, dwLocalExternalIP);// IP to connect to
  packLEDWord(&packet, wListenPort);    // Port to connect to
  packByte(&packet, DC_TYPE);           // generic DC type
  packDWord(&packet, dwRemotePort);     // unknown
  packDWord(&packet, wListenPort);      // port again ?
  packLEWord(&packet, ICQ_VERSION);     // DC Version
  packLEDWord(&packet, dwCookie);       // Req Cookie

  // Send the monster
  sendServPacket(&packet);
}
