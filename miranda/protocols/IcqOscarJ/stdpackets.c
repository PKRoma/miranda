// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin  berg, Sam Kothari, Robert Rainwater
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



extern HANDLE ghServerNetlibUser;
extern HANDLE hDirectNetlibUser;
extern DWORD dwLocalUIN;
extern int gbIdleAllow;
extern int gnCurrentStatus;
extern DWORD dwLocalInternalIP;
extern DWORD dwLocalExternalIP;
extern WORD wListenPort;
extern HANDLE hsmsgrequest;
extern CRITICAL_SECTION modeMsgsMutex;
extern BYTE gbSsiEnabled;
extern char gpszICQProtoName[MAX_PATH];

WORD sendTLVSearchPacket(char *pSearchDataBuf, WORD wInfoLen, BOOL bOnlineUsersOnly);



/*****************************************************************************
 *
 *	 Some handy extra pack functions for basic message type headers
 *
 */



// This is the part of the message header that is common for all message channels
static void packServMsgSendHeader(icq_packet *p, DWORD dwSequence, DWORD dwID1, DWORD dwID2, DWORD dwUin, WORD wFmt, WORD wLen)
{

	unsigned char szUin[10];
	unsigned char nUinLen;


	_snprintf(szUin, 10, "%d", dwUin);
	nUinLen = strlen(szUin);

	p->wLen = 21 + nUinLen + wLen;
	write_flap(p, ICQ_DATA_CHAN);
	packFNACHeader(p, ICQ_MSG_FAMILY, ICQ_MSG_SRV_SEND, 0, dwSequence);
	packLEDWord(p, dwID1);         // Msg ID part 1
	packLEDWord(p, dwID2);         // Msg ID part 2
	packWord(p, wFmt);             // Message channel
	packByte(p, nUinLen);          // Length of user id
	packBuffer(p, szUin, nUinLen); // Receiving user's id

}



static void packServIcqExtensionHeader(icq_packet *p, WORD wLen, WORD wType, WORD wCookie)
{

	p->wLen = 24 + wLen;
	write_flap(p, ICQ_DATA_CHAN);
	packFNACHeader(p, ICQ_EXTENSIONS_FAMILY, CLI_META_REQ, 0, 0);
	packWord(p, 0x01);               // TLV type 1
	packWord(p, (WORD)(10 + wLen));  // TLV len
	packLEWord(p, (WORD)(8 + wLen)); // Data chunk size (TLV.Length-2)
	packLEDWord(p, dwLocalUIN);      // My UIN
	packLEWord(p, wType);              // Request type
	packWord(p, wCookie);

}



static void packServChannel2Header(icq_packet *p, DWORD dwUin, WORD wLen, WORD wCookie, BYTE bMsgType, BYTE bMsgFlags, WORD wPriority, int isAck, int includeDcInfo, BYTE bRequestServerAck)
{

	DWORD dwID1;
	DWORD dwID2;


	dwID1 = time(NULL);
	dwID2 = RandRange(0, 0x00FF);

	packServMsgSendHeader(p, wCookie, dwID1, dwID2, dwUin, 0x0002,
		(WORD)(wLen + 95 + (bRequestServerAck?4:0) + (includeDcInfo?14:0)));

	packWord(p, 0x05);			/* TLV type */
	packWord(p, (WORD)(wLen + 91 + (includeDcInfo?14:0)));	/* TLV len */
	packWord(p, (WORD)(isAck ? 2: 0));	   /* not aborting anything */
	packLEDWord(p, dwID1);   // Msg ID part 1
	packLEDWord(p, dwID2); // Msg ID part 2
	packDWord(p, 0x09461349);		/* capabilities (4 dwords) */
	packDWord(p, 0x4C7F11D1);
	packDWord(p, 0x82224445);
	packDWord(p, 0x53540000);
	packDWord(p, 0x000A0002);	/* TLV: 0x0A WORD: 1 for normal, 2 for ack */
	packWord(p, (WORD)(isAck ? 2 : 1));

	if (includeDcInfo)
	{
		packDWord(p, 0x00050002); // TLV: 0x05 Listen port
		packWord(p, wListenPort);
		packDWord(p, 0x00030004); // TLV: 0x03 DWORD IP
		packDWord(p, dwLocalInternalIP);
	}

	packDWord(p, 0x000F0000);    /* TLV: 0x0F empty */

	packWord(p, 0x2711);	/* TLV: 0x2711 */
	packWord(p, (WORD)(wLen + 51));
	packLEWord(p, 0x1B); // Unknown
	packLEWord(p, ICQ_VERSION); // Client version
	packDWord(p, 0); // Capability (16 bytes)
	packDWord(p, 0); //  -
	packDWord(p, 0); //  -
	packDWord(p, 0); //  -
	packWord(p, 0);
	packLEDWord(p, 3);	   /* unknown */
	packByte(p, 0);
	packLEWord(p, wCookie); // Reference cookie
	packLEWord(p, 0x0E);    // Unknown
	packLEWord(p, wCookie); // Reference cookie again

	packDWord(p, 0); // Unknown (12 bytes)
	packDWord(p, 0); //  -
	packDWord(p, 0); //  -

	packByte(p, bMsgType);  // Message type
	packByte(p, bMsgFlags); // Flags:
                            // 00 - normal message
                            // 80 - multiple recipients
                            // 03 - auto reply message request
	packLEWord(p, (WORD)MirandaStatusToIcq(gnCurrentStatus));
	packWord(p, wPriority);

}



static void packServAdvancedMsgReply(icq_packet *p, DWORD dwUin, DWORD dwTimestamp, DWORD dwTimestamp2, WORD wCookie, BYTE bMsgType, BYTE bMsgFlags, WORD wLen)
{

	unsigned char szUin[10];
	unsigned char nUinLen;


	_snprintf(szUin, 10, "%d", dwUin);
	nUinLen = strlen(szUin);

	p->wLen = nUinLen + 74 + wLen;
	write_flap(p, ICQ_DATA_CHAN);
	packFNACHeader(p, ICQ_MSG_FAMILY, ICQ_MSG_RESPONSE, 0, ICQ_MSG_RESPONSE);
	packLEDWord(p, dwTimestamp);   // Msg ID part 1
	packLEDWord(p, dwTimestamp2);  // Msg ID part 2
	packWord(p, 0x02);			   // Channel
	packByte(p, nUinLen);
	packBuffer(p, szUin, nUinLen); // Your UIN
	packWord(p, 0x03);			   // Unknown
	packLEWord(p, 0x1B);	       // Unknown
	packLEWord(p, ICQ_VERSION);	   // Protocol version
	packDWord(p, 0);
	packDWord(p, 0);
	packDWord(p, 0);
	packDWord(p, 0);
	packWord(p, 0);
	packLEDWord(p, CLIENTFEATURES);
	packByte(p, 0);	            // Firewall info?
	packLEWord(p, wCookie);	 // Reference
	packLEWord(p, 0x0E);	 // Unknown
	packLEWord(p, wCookie);	 // Reference
	packDWord(p, 0);         // Unknown
	packDWord(p, 0);         // Unknown
	packDWord(p, 0);         // Unknown
	packByte(p, bMsgType);   // Message type
	packByte(p, bMsgFlags);  // Message flags
	packLEWord(p, 0);        // Ack status code ( 0 = accepted, this is hardcoded because
	                         //                   it is only used this way yet)
	packLEWord(p, 0);        // Unused priority field

}



/*****************************************************************************
 *
 *	 Functions to actually send the stuff
 *
 */


void icq_setidle(int bAllow)
{
	icq_packet packet;

	if (bAllow!=gbIdleAllow)
	{

		/* SNAC 1,11 */
		packet.wLen = 14;
		write_flap(&packet, 2);
		packFNACHeader(&packet, ICQ_SERVICE_FAMILY, 0x11, 0, 0x11);
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
	WORD wFlags = 0;


	// Webaware setting bit flag
	if (DBGetContactSettingByte(NULL, gpszICQProtoName, "WebAware", 0))
		wFlags = STATUS_WEBAWARE;

	// DC setting bit flag
	switch (DBGetContactSettingByte(NULL, gpszICQProtoName, "DCType", 0))
	{

	case 1:
		wFlags = wFlags | STATUS_DCCONT;
		break;

	case 2:
		wFlags = wFlags | STATUS_DCAUTH;
		break;

	default:
		wFlags = wFlags | STATUS_DCDISABLED;
		break;

	}


	// Pack data in packet
	packet.wLen = 18;
	write_flap(&packet, ICQ_DATA_CHAN);
	packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_SET_STATUS, 0, ICQ_CLIENT_SET_STATUS);
	packWord(&packet, 0x06);    // TLV 6
	packWord(&packet, 0x04);    // TLV length
	packWord(&packet, wFlags);  // Status flags
	packWord(&packet, wStatus); // Status

	// Send packet
	sendServPacket(&packet);

}



WORD icq_SendChannel1Message(DWORD dwUin, HANDLE hContact, char *pszText, message_cookie_data *pCookieData)
{

	icq_packet packet;
	WORD wMessageLen;
	WORD wCookie;
	WORD wPacketLength;
	DWORD dwID1, dwID2;


	wMessageLen = strlen(pszText);
	wCookie = AllocateCookie(dwUin, (void*)pCookieData);
	dwID1 = time(NULL);
	dwID2 = RandRange(0, 0x00FF);

	if (pCookieData->nAckType == ACKTYPE_SERVER)
		wPacketLength = 25;
	else
		wPacketLength = 21;

	// Pack the standard header
	packServMsgSendHeader(&packet, (DWORD)wCookie, dwID1, dwID2, dwUin, 1, (WORD)(wPacketLength + wMessageLen));

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
		packWord(&packet, 0x0003); // TLV(3)
		packWord(&packet, 0x0000); // TLV len
	}

	// Pack store on server TLV
	packWord(&packet, 0x0006); // TLV(6)
	packWord(&packet, 0x0000); // TLV len

	sendServPacket(&packet);

	return wCookie;

}


WORD icq_SendChannel1MessageW(DWORD dwUin, HANDLE hContact, wchar_t *pszText, message_cookie_data *pCookieData)
{

	icq_packet packet;
	WORD wMessageLen;
	WORD wCookie;
	WORD wPacketLength;
	DWORD dwID1, dwID2;
	wchar_t* ppText;
	int i;


	wMessageLen = wcslen(pszText)*sizeof(wchar_t);
	wCookie = AllocateCookie(dwUin, (void*)pCookieData);
	dwID1 = time(NULL);
	dwID2 = RandRange(0, 0x00FF);

	if (pCookieData->nAckType == ACKTYPE_SERVER)
		wPacketLength = 26;
	else
		wPacketLength = 22;

	// Pack the standard header
	packServMsgSendHeader(&packet, (DWORD)wCookie, dwID1, dwID2, dwUin, 1, (WORD)(wPacketLength + wMessageLen));

	// Pack first TLV
	packWord(&packet, 0x0002); // TLV(2)
	packWord(&packet, (WORD)(wMessageLen + 14)); // TLV len

	// Pack client features
	packWord(&packet, 0x0501); // TLV(501)
	packWord(&packet, 0x0002); // TLV len
	packWord(&packet, 0x0106);    // Features, meaning unknown, duplicated from ICQ 2003b

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

	//packBuffer(&packet, (char*)pszText, (WORD)(wMessageLen)); // Message text

	// Pack request server ack TLV
	if (pCookieData->nAckType == ACKTYPE_SERVER)
	{
		packWord(&packet, 0x0003); // TLV(3)
		packWord(&packet, 0x0000); // TLV len
	}

	// Pack store on server TLV
	packWord(&packet, 0x0006); // TLV(6)
	packWord(&packet, 0x0000); // TLV len

	sendServPacket(&packet);

	return wCookie;

}


WORD icq_SendChannel2Message(DWORD dwUin, const char *szMessage, int nBodyLen, WORD wPriority, message_cookie_data *pCookieData)
{

	icq_packet packet;
	WORD wCookie;


	wCookie = AllocateCookie(dwUin, (void*)pCookieData);

	// Pack the standard header
	packServChannel2Header(&packet, dwUin, (WORD)(nBodyLen+11), wCookie, pCookieData->bMessageType, 0,
		wPriority, 0, 0, (BYTE)((pCookieData->nAckType == ACKTYPE_SERVER)?1:0));

	packLEWord(&packet, (WORD)(nBodyLen+1));	        // Length of message
	packBuffer(&packet, szMessage, (WORD)(nBodyLen+1)); // Message
	packLEDWord(&packet, 0x00000000);                   // Foreground colour
	packLEDWord(&packet, 0x00FFFFFF);                   // Background colour

	// Pack request server ack TLV
	if (pCookieData->nAckType == ACKTYPE_SERVER)
	{
		packWord(&packet, 0x0003); // TLV(3)
		packWord(&packet, 0x0000); // TLV len
	}

	sendServPacket(&packet);

	return wCookie;

}


WORD icq_SendChannel2MessageW(DWORD dwUin, const char *szMessage, int nBodyLen, WORD wPriority, message_cookie_data *pCookieData)
{

	icq_packet packet;
	WORD wCookie;


	wCookie = AllocateCookie(dwUin, (void*)pCookieData);

	// Pack the standard header
	packServChannel2Header(&packet, dwUin, (WORD)(nBodyLen+53), wCookie, pCookieData->bMessageType, 0,
		wPriority, 0, 0, (BYTE)((pCookieData->nAckType == ACKTYPE_SERVER)?1:0));

	packLEWord(&packet, (WORD)(nBodyLen+1));	        // Length of message
	packBuffer(&packet, szMessage, (WORD)(nBodyLen+1)); // Message
	packLEDWord(&packet, 0x00000000);                   // Foreground colour
	packLEDWord(&packet, 0x00FFFFFF);                   // Background colour
	packLEDWord(&packet, 0x00000026);                   // length of GUID
	packDWord(&packet, 0x7B303934);                     // UTF-8 GUID
	packDWord(&packet, 0x36313334);
	packDWord(&packet, 0x452D3443);
	packDWord(&packet, 0x37462D31);
	packDWord(&packet, 0x3144312D);
	packDWord(&packet, 0x38323232);
	packDWord(&packet, 0x2D343434);
	packDWord(&packet, 0x35353335);
	packDWord(&packet, 0x34303030);
	packWord(&packet, 0x307D);

	// Pack request server ack TLV
	if (pCookieData->nAckType == ACKTYPE_SERVER)
	{
		packWord(&packet, 0x0003); // TLV(3)
		packWord(&packet, 0x0000); // TLV len
	}

	sendServPacket(&packet);

	return wCookie;

}


WORD icq_SendChannel4Message(DWORD dwUin, BYTE bMsgType, WORD wMsgLen, const char *szMsg, message_cookie_data *pCookieData)
{

	icq_packet packet;
	DWORD dwID1;
	DWORD dwID2;
	WORD wPacketLength;
	WORD wCookie;


	wCookie = AllocateCookie(dwUin, (void*)pCookieData);

	if (pCookieData->nAckType == ACKTYPE_SERVER)
		wPacketLength = 28;
	else
		wPacketLength = 24;

	dwID1 = time(NULL);
	dwID2 = RandRange(0, 0x00FF);

	// Pack the standard header
	packServMsgSendHeader(&packet, wCookie, dwID1, dwID2, dwUin, 4, (WORD)(wPacketLength + wMsgLen));

	// Pack first TLV
	packWord(&packet, 0x05);                 // TLV(5)
	packWord(&packet, (WORD)(wMsgLen + 16)); // TLV len
	packLEDWord(&packet, dwLocalUIN);        // My UIN
	packByte(&packet, bMsgType);             // Message type
	packByte(&packet, 0);                    // Message flags
	packLEWord(&packet, wMsgLen);            // Message length
	packBuffer(&packet, szMsg, wMsgLen);     // Message text
	packLEDWord(&packet, 0x000000);	         // Foreground colour
	packLEDWord(&packet, 0xFFFFFF);          // Background colour

	// Pack request ack TLV
	if (pCookieData->nAckType == ACKTYPE_SERVER)
	{
		packWord(&packet, 0x0003); // TLV(3)
		packWord(&packet, 0x0000); // TLV len
	}

	// Pack store on server TLV
	packWord(&packet, 0x0006); // TLV(6)
	packWord(&packet, 0x0000); // TLV len

	sendServPacket(&packet);

	return wCookie;

}



void sendOwnerInfoRequest(void)
{

	icq_packet packet;
	WORD wCookie;
	fam15_cookie_data *pCookieData = NULL;


	pCookieData = malloc(sizeof(fam15_cookie_data));
	pCookieData->bRequestType = REQUESTTYPE_OWNER;
	wCookie = AllocateCookie(dwLocalUIN, (void*)pCookieData);

	packServIcqExtensionHeader(&packet, 6, 0x07D0, wCookie);
	packLEWord(&packet, 0x04D0);
	packLEDWord(&packet, dwLocalUIN);

	sendServPacket(&packet);

}



void sendUserInfoAutoRequest(DWORD dwUin)
{

	icq_packet packet;
	WORD wCookie;
	fam15_cookie_data *pCookieData = NULL;


	pCookieData = malloc(sizeof(fam15_cookie_data));
	pCookieData->bRequestType = REQUESTTYPE_USERAUTO;
	wCookie = AllocateCookie(dwUin, (void*)pCookieData);

	packServIcqExtensionHeader(&packet, 6, 0x07D0, wCookie);
	packLEWord(&packet, 0x04BA);
	packLEDWord(&packet, dwUin);

	sendServPacket(&packet);

}



WORD icq_sendGetInfoServ(DWORD dwUin, int bMinimal)
{

	icq_packet packet;
	WORD wCookie;
	fam15_cookie_data *pCookieData = NULL;


	pCookieData = malloc(sizeof(fam15_cookie_data));
	wCookie = AllocateCookie(dwUin, (void*)pCookieData);

	packServIcqExtensionHeader(&packet, 6, CLI_META_INFO_REQ, wCookie);
	if (bMinimal)
	{
		pCookieData->bRequestType = REQUESTTYPE_USERMINIMAL;
		packLEWord(&packet, 0x04BA);
	}
	else
	{
		pCookieData->bRequestType = REQUESTTYPE_USERDETAILED;
		packLEWord(&packet, 0x04B2);
	}
	packLEDWord(&packet, dwUin);

	sendServPacket(&packet);

	return wCookie;

}



WORD icq_sendGetAwayMsgServ(DWORD dwUin, int type)
{

	icq_packet packet;
	WORD wCookie;


	wCookie = GenerateCookie();

	packServChannel2Header(&packet, dwUin, 3, wCookie, (BYTE)type, 3, 1, 0, 0, 0);
	packLEWord(&packet, 1);	/* length of message */
	packByte(&packet, 0);      /* message */
	sendServPacket(&packet);

	return wCookie;

}



void icq_sendFileSendServv7(DWORD dwUin, WORD wCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize)
{

	icq_packet packet;


	packServChannel2Header(&packet, dwUin, (WORD)(18 + strlen(szDescr) + strlen(szFiles)), wCookie, MTYPE_FILEREQ, 0, 1, 0, 1, 1);

	packLEWord(&packet, (WORD)(strlen(szDescr) + 1));
	packBuffer(&packet, szDescr, (WORD)(strlen(szDescr) + 1));
	packLEDWord(&packet, 0);	 // unknown
	packLEWord(&packet, (WORD)(strlen(szFiles) + 1));
	packBuffer(&packet, szFiles, (WORD)(strlen(szFiles) + 1));
	packLEDWord(&packet, dwTotalSize);
	packLEDWord(&packet, 0);	 // unknown

	sendServPacket(&packet);

}



void icq_sendFileSendServv8(DWORD dwUin, WORD wCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize)
{

	icq_packet packet;
	unsigned char szUin[10];
	unsigned char nUinLen;
	DWORD dwMsgID1;
	DWORD dwMsgID2;
	WORD wFlapLen;


	dwMsgID1 = time(NULL);
	dwMsgID2 = RandRange(0, 0x00FF);

	_snprintf(szUin, 10, "%d", dwUin);
	nUinLen = strlen(szUin);


	// 202 + UIN len + file description (no null) + file name (null included)
	// Packet size = Flap length + 4
	wFlapLen = 202 + nUinLen + strlen(szDescr) + strlen(szFiles)+1;
	packet.wLen = wFlapLen;
	write_flap(&packet, ICQ_DATA_CHAN);
	packFNACHeader(&packet, ICQ_MSG_FAMILY, ICQ_MSG_SRV_SEND, 0, wCookie);
	packLEDWord(&packet, dwMsgID1); // Msg ID part 1
	packLEDWord(&packet, dwMsgID2); // Msg ID part 2
	packWord(&packet, 0x0002);	    // Channel
	packByte(&packet, nUinLen);
	packBuffer(&packet, szUin, nUinLen);

	// TLV(5) header
	packWord(&packet, 0x05);                                              // Type
	packWord(&packet, (WORD)(173 + strlen(szDescr) + strlen(szFiles)+1)); // Len
	// TLV(5) data
	packWord(&packet, 0);	        // Command
	packLEDWord(&packet, dwMsgID1); // Msg ID part 1
	packLEDWord(&packet, dwMsgID2); // Msg ID part 2
	packDWord(&packet, 0x09461349); // capabilities (4 dwords)
	packDWord(&packet, 0x4C7F11D1);
	packDWord(&packet, 0x82224445);
	packDWord(&packet, 0x53540000);
	packDWord(&packet, 0x000A0002); // TLV: 0x0A Acktype: 1 for normal, 2 for ack
	packWord(&packet, 0x0001);
	packDWord(&packet, 0x000F0000); // TLV: 0x0F empty
	packDWord(&packet, 0x00030004); // TLV: 0x03 DWORD IP
	packDWord(&packet, dwLocalInternalIP);
	packDWord(&packet, 0x00050002); // TLV: 0x05 Listen port
	packWord(&packet, wListenPort);

	// TLV(0x2711) header
	packWord(&packet, 0x2711);	                                          // Type
	packWord(&packet, (WORD)(119 + strlen(szDescr) + strlen(szFiles)+1)); // Len
	// TLV(0x2711) data
	packLEWord(&packet, 0x1B); // Unknown
	packByte(&packet, ICQ_VERSION); // Client version
	packDWord(&packet, 0); // Unknown (16 bytes)
	packDWord(&packet, 0); //  -
	packDWord(&packet, 0); //  -
	packDWord(&packet, 0); //  -
	packDWord(&packet, CLIENTFEATURES);
	packDWord(&packet, DC_TYPE);
	packLEWord(&packet, wCookie); // Reference cookie
	packLEWord(&packet, 0x0E);    // Unknown
	packLEWord(&packet, wCookie); // Reference cookie again
	packDWord(&packet, 0); // Unknown (12 bytes)
	packDWord(&packet, 0); //  -
	packDWord(&packet, 0); //  -
	packByte(&packet, MTYPE_PLUGIN); // Message type
	packByte(&packet, 0);  // Flags
	packLEWord(&packet, (WORD)MirandaStatusToIcq(gnCurrentStatus));
	packLEWord(&packet, 1); // Unknown, priority?

	packLEWord(&packet, 1); // Message len
	packByte(&packet, 0);   // Message (unused)
	packLEWord(&packet, 0x029); // Command

	packDWord(&packet, 0xf02d12d9); // Unknown binary string (4 dwords)
	packDWord(&packet, 0x3091d311);
	packDWord(&packet, 0x8dd70010);
	packDWord(&packet, 0x4b06462e);
	packWord(&packet, 0x0000);      // Unknown
	packLEDWord(&packet, 0x0004);                           // Request type string
	packBuffer(&packet, "File", 4);

	packDWord(&packet, 0x00000100); // More unknown binary stuff
	packDWord(&packet, 0x00010000);
	packDWord(&packet, 0x00000000);
	packWord(&packet, 0x0000);
	packByte(&packet, 0x00);

	packLEDWord(&packet, (WORD)(18 + strlen(szDescr) + strlen(szFiles)+1)); // Remaining length
	packLEDWord(&packet, (WORD)(strlen(szDescr)));          // Description
	packBuffer(&packet, szDescr, (WORD)(strlen(szDescr)));
	packWord(&packet, 0x8c82); // Unknown (port?), seen 0x80F6
	packWord(&packet, 0x0222); // Unknown, seen 0x2e01
	packLEWord(&packet, (WORD)(strlen(szFiles)+1));
	packBuffer(&packet, szFiles, (WORD)(strlen(szFiles)+1));
	packLEDWord(&packet, dwTotalSize);
	packLEDWord(&packet, 0x0008c82); // Unknown, (seen 0xf680 ~33000)

	// Finally, an empty TLV(3)
	packDWord(&packet, 0x00030000);

	// Send the monster
	sendServPacket(&packet);

}


/* also sends rejections */
void icq_sendFileAcceptServv8(DWORD dwUin, DWORD TS1, DWORD TS2, WORD wCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize, WORD wPort, BOOL accepted)
{

	icq_packet packet;
	unsigned char szUin[10];
	unsigned char nUinLen;
	WORD wFlapLen;

	/* if !accepted, szDescr == szReason, szFiles = "" */

	if (!accepted) szFiles = "";

	_snprintf(szUin, 10, "%d", dwUin);
	nUinLen = strlen(szUin);

	// 202 + UIN len + file description (no null) + file name (null included)
	// Packet size = Flap length + 4
	wFlapLen = 202 + nUinLen + strlen(szDescr) + strlen(szFiles)+1;
	packet.wLen = wFlapLen;
	write_flap(&packet, ICQ_DATA_CHAN);
	packFNACHeader(&packet, ICQ_MSG_FAMILY, ICQ_MSG_SRV_SEND, 0, wCookie);
	packLEDWord(&packet, TS1);		   // timestamp1
	packLEDWord(&packet, TS2);		   // timestamp2
	packWord(&packet, 0x0002);	           // message format
	packByte(&packet, nUinLen);
	packBuffer(&packet, szUin, nUinLen);

	// TLV(5) header
	packWord(&packet, 0x05);                                              // Type
	packWord(&packet, (WORD)(173 + strlen(szDescr) + strlen(szFiles)+1)); // Len
	// TLV(5) data
	packWord(&packet, 0);	       // Command
	packLEDWord(&packet, TS1);       // timestamp1
	packLEDWord(&packet, TS2);       // timestamp2
	packDWord(&packet, 0x09461349); // capabilities (4 dwords)
	packDWord(&packet, 0x4C7F11D1);
	packDWord(&packet, 0x82224445);
	packDWord(&packet, 0x53540000);
	packDWord(&packet, 0x000A0002); // TLV: 0x0A Acktype: 1 for normal, 2 for ack
	packWord(&packet, 0x0002);
	packDWord(&packet, 0x000F0000); // TLV: 0x0F empty
	packDWord(&packet, 0x00030004); // TLV: 0x03 DWORD IP
	packDWord(&packet, accepted ? dwLocalInternalIP : 0);
	packDWord(&packet, 0x00050002); // TLV: 0x05 Listen port
	packWord(&packet, (WORD) (accepted ? wListenPort : 0));

	// TLV(0x2711) header
	packWord(&packet, 0x2711);	                                          // Type
	packWord(&packet, (WORD)(119 + strlen(szDescr) + strlen(szFiles)+1)); // Len
	// TLV(0x2711) data
	packLEWord(&packet, 0x1B); // Unknown
	packByte(&packet, ICQ_VERSION); // Client version
	packDWord(&packet, 0); // Unknown (16 bytes)
	packDWord(&packet, 0); //  -
	packDWord(&packet, 0); //  -
	packDWord(&packet, 0); //  -
	packDWord(&packet, CLIENTFEATURES);
	packDWord(&packet, DC_TYPE);
	packLEWord(&packet, wCookie); // Reference cookie
	packLEWord(&packet, 0x0E);    // Unknown
	packLEWord(&packet, wCookie); // Reference cookie again
	packDWord(&packet, 0); // Unknown (12 bytes)
	packDWord(&packet, 0); //  -
	packDWord(&packet, 0); //  -
	packByte(&packet, MTYPE_PLUGIN); // Message type
	packByte(&packet, 0);  // Flags
	packLEWord(&packet, (WORD) (accepted ? 0:1)); // Accepted or not?
	packLEWord(&packet, 0); // Unknown, priority?
	//
	packLEWord(&packet, 1); // Message len
	packByte(&packet, 0);   // Message (unused)
	packLEWord(&packet, 0x029); // Command

	packDWord(&packet, 0xf02d12d9); // Unknown binary string (4 dwords)
	packDWord(&packet, 0x3091d311);
	packDWord(&packet, 0x8dd70010);
	packDWord(&packet, 0x4b06462e);
	packWord(&packet, 0x0000);      // Unknown
	packLEDWord(&packet, 0x0004);                           // Request type string
	packBuffer(&packet, "File", 0x0004);

	packDWord(&packet, 0x00000100); // More unknown binary stuff
	packDWord(&packet, 0x00010000);
	packDWord(&packet, 0x00000000);
	packWord(&packet, 0x0000);
	packByte(&packet, 0x00);

	packLEDWord(&packet, (WORD)(18 + strlen(szDescr) + strlen(szFiles)+1)); // Remaining length
	packLEDWord(&packet, (WORD)(strlen(szDescr)));          // Description
	packBuffer(&packet, szDescr, (WORD)(strlen(szDescr)));
	packWord(&packet, wPort); // Port
	packWord(&packet, 0x00);  // Unknown
	packLEWord(&packet, (WORD)(strlen(szFiles)+1));
	packBuffer(&packet, szFiles, (WORD)(strlen(szFiles)+1));
	packLEDWord(&packet, dwTotalSize);
	packLEDWord(&packet, (DWORD)wPort); // Unknown

	// Finally, an empty TLV(3)
	packDWord(&packet, 0x00030000);

	// Send the monster
	sendServPacket(&packet);

}



void icq_sendFileAcceptServv7(DWORD dwUin, DWORD TS1, DWORD TS2, WORD wCookie, const char* szFiles, const char* szDescr, DWORD dwTotalSize, WORD wPort, BOOL accepted)
{

	icq_packet packet;
	unsigned char szUin[10];
	unsigned char nUinLen;
	WORD wFlapLen;

	/* if !accepted, szDescr == szReason, szFiles = "" */

	if (!accepted) szFiles = "";


	_snprintf(szUin, 10, "%d", dwUin);
	nUinLen = strlen(szUin);

	// 150 + UIN len + file description (with null) + file name (2 nulls)
	// Packet size = Flap length + 4
	wFlapLen = 150 + nUinLen + strlen(szDescr)+1 + strlen(szFiles)+2;
	packet.wLen = wFlapLen;
	write_flap(&packet, ICQ_DATA_CHAN);
	packFNACHeader(&packet, ICQ_MSG_FAMILY, ICQ_MSG_SRV_SEND, 0, wCookie);
	packLEDWord(&packet, TS1);		   // timestamp1
	packLEDWord(&packet, TS2);		   // timestamp2
	packWord(&packet, 0x0002);         // message format
	packByte(&packet, nUinLen);
	packBuffer(&packet, szUin, nUinLen);

	// TLV(5) header
	packWord(&packet, 0x05);                                              // Type
	packWord(&packet, (WORD)(121 + strlen(szDescr)+1 + strlen(szFiles)+2)); // Len
	// TLV(5) data
	packWord(&packet, 0);	        // Command
	packLEDWord(&packet, TS1);        // timestamp1
	packLEDWord(&packet, TS2);        // timestamp2
	packDWord(&packet, 0x09461349); // capabilities (4 dwords)
	packDWord(&packet, 0x4C7F11D1);
	packDWord(&packet, 0x82224445);
	packDWord(&packet, 0x53540000);
	packDWord(&packet, 0x000A0002); // TLV: 0x0A Acktype: 1 for normal, 2 for ack
	packWord(&packet, 0x0002);
	packDWord(&packet, 0x000F0000); // TLV: 0x0F empty
	packDWord(&packet, 0x00030004); // TLV: 0x03 DWORD IP
	packDWord(&packet, (accepted ? dwLocalInternalIP : 0));
	packDWord(&packet, 0x00050002); // TLV: 0x05 Listen port
	packWord(&packet, (WORD)(accepted ? wListenPort : 0));

	// TLV(0x2711) header
	packWord(&packet, 0x2711);	                                          // Type
	packWord(&packet, (WORD)(67 + strlen(szDescr)+1 + strlen(szFiles)+2)); // Len
	// TLV(0x2711) data
	packLEWord(&packet, 0x1B); // Unknown
	packByte(&packet, 0x8); // Client version
	packDWord(&packet, 0); // Unknown (16 bytes)
	packDWord(&packet, 0); //  -
	packDWord(&packet, 0); //  -
	packDWord(&packet, 0); //  -
	packDWord(&packet, 0x0003); // Unknown
	packDWord(&packet, 0x0004); // Unknown
	packLEWord(&packet, wCookie); // Reference cookie
	packLEWord(&packet, 0x0E);    // Unknown
	packLEWord(&packet, wCookie); // Reference cookie again
	packDWord(&packet, 0); // Unknown (12 bytes)
	packDWord(&packet, 0); //  -
	packDWord(&packet, 0); //  -
	packByte(&packet, 0x03); // Message type
	packByte(&packet, 0);  // Flags
	packLEWord(&packet, (WORD)(accepted ? 0:1)); // Accepted or not?
	packLEWord(&packet, 0); // Unknown, priority?
	//
	packLEWord(&packet, (WORD)(strlen(szDescr)+1));          // Description
	packBuffer(&packet, szDescr, (WORD)(strlen(szDescr)+1));
	packWord(&packet, wPort); // Port
	packWord(&packet, 0x00);  // Unknown
	packLEWord(&packet, (WORD)(strlen(szFiles)+2));
	packBuffer(&packet, szFiles, (WORD)(strlen(szFiles)+1));
	packByte(&packet, 0);
	packLEDWord(&packet, dwTotalSize);
	packLEDWord(&packet, (DWORD)wPort); // Unknown

	// Finally, an empty TLV(3)
	packDWord(&packet, 0x00030000);

	// Send the monster
	sendServPacket(&packet);

}



void icq_sendFileAcceptServ(DWORD dwUin, filetransfer* ft)
{

	if (ft->nVersion >= 8)
	{
		icq_sendFileAcceptServv8(dwUin, ft->TS1, ft->TS2, ft->wCookie, ft->szFilename, ft->szDescription, ft->dwTotalSize, wListenPort, TRUE);
		Netlib_Logf(ghServerNetlibUser, "Sent file accept v8 through server, port %u", wListenPort);
	}
	else
	{
		icq_sendFileAcceptServv7(dwUin, ft->TS1, ft->TS2, ft->wCookie, ft->szFilename, ft->szDescription, ft->dwTotalSize, wListenPort, TRUE);
		Netlib_Logf(ghServerNetlibUser, "Sent file accept v7 through server, port %u", wListenPort);
	}

}

void icq_sendFileDenyServ(DWORD dwUin, filetransfer* ft, char *szReason)
{

	if (ft->nVersion >= 8)
	{
		icq_sendFileAcceptServv8(dwUin, ft->TS1, ft->TS2, ft->wCookie, ft->szFilename, szReason, ft->dwTotalSize, wListenPort, FALSE);
		Netlib_Logf(ghServerNetlibUser, "Sent file deny v8 through server, port %u", wListenPort);
	}
	else
	{
		icq_sendFileAcceptServv7(dwUin, ft->TS1, ft->TS2, ft->wCookie, ft->szFilename, szReason, ft->dwTotalSize, wListenPort, FALSE);
		Netlib_Logf(ghServerNetlibUser, "Sent file deny v7 through server, port %u", wListenPort);
	}

}

void icq_sendAwayMsgReplyServ(DWORD dwUin, DWORD dwTimestamp, DWORD dwTimestamp2, WORD wCookie, BYTE msgType, const char** szMsg)
{

	icq_packet packet;
	WORD wMsgLen;
	HANDLE hContact;


	hContact = HContactFromUIN(dwUin, 0);

	if (validateStatusMessageRequest(hContact, msgType))
	{

		NotifyEventHooks(hsmsgrequest, (WPARAM)msgType, (LPARAM)dwUin);

		EnterCriticalSection(&modeMsgsMutex);

		if (*szMsg != NULL)
		{
			wMsgLen = strlen(*szMsg);
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
	packLEWord(&packet, 1);	          // Status message length
	packByte(&packet, 0);	          // Status message
	packLEDWord(&packet, 0x00000000); // Foreground colour
	packLEDWord(&packet, 0x00FFFFFF); // Background colour

	sendServPacket(&packet);

}



// Searches

WORD SearchByUin(DWORD dwUin)
{

	WORD wCookie, wInfoLen = 0;
	icq_packet pBuffer; // I reuse the ICQ packet type as a generic buffer
	                    // I should be ashamed! ;)


	// Calculate data size
	wInfoLen = 8;

	// Initialize our handy data buffer
	pBuffer.wPlace = 0;
	pBuffer.pData = (BYTE *)calloc(1, wInfoLen);
	pBuffer.wLen = wInfoLen;

	// Initialize our handy data buffer
	packLEWord(&pBuffer, 0x0136);
	packLEWord(&pBuffer, 0x0004);
	packLEDWord(&pBuffer, dwUin);

	// Send it off for further packing
	wCookie = sendTLVSearchPacket(pBuffer.pData, wInfoLen, FALSE);
	SAFE_FREE(pBuffer.pData);

	return wCookie;

}



WORD SearchByNames(char* pszNick, char* pszFirstName, char* pszLastName)
{

	WORD wCookie, wInfoLen = 0;
	icq_packet pBuffer; // I reuse the ICQ packet type as a generic buffer
	                    // I should be ashamed! ;)

	_ASSERTE(strlennull(pszFirstName) || strlennull(pszLastName) || strlennull(pszNick));


	// Calculate data size
	if (strlennull(pszFirstName) > 0)
		wInfoLen = strlen(pszFirstName) + 7;
	if (strlennull(pszLastName) > 0)
		wInfoLen += strlen(pszLastName) + 7;
	if (strlennull(pszNick) > 0)
		wInfoLen += strlen(pszNick) + 7;

	// Initialize our handy data buffer
	pBuffer.wPlace = 0;
	pBuffer.pData = (BYTE*)calloc(1, wInfoLen);
	pBuffer.wLen = wInfoLen;


	// Pack the search details
	if (strlennull(pszFirstName) > 0)
	{
		packLEWord(&pBuffer, 0x0140);
		packLEWord(&pBuffer, (WORD)(strlen(pszFirstName)+3));
		packLEWord(&pBuffer, (WORD)(strlen(pszFirstName)+1));
		packBuffer(&pBuffer, pszFirstName, (WORD)(strlen(pszFirstName)));
		packByte(&pBuffer, 0);
	}

	if (strlennull(pszLastName) > 0)
	{
		packLEWord(&pBuffer, 0x014a);
		packLEWord(&pBuffer, (WORD)(strlen(pszLastName)+3));
		packLEWord(&pBuffer, (WORD)(strlen(pszLastName)+1));
		packBuffer(&pBuffer, pszLastName, (WORD)(strlen(pszLastName)));
		packByte(&pBuffer, 0);
	}

	if (strlennull(pszNick) > 0)
	{
		packLEWord(&pBuffer, 0x0154);
		packLEWord(&pBuffer, (WORD)(strlen(pszNick)+3));
		packLEWord(&pBuffer, (WORD)(strlen(pszNick)+1));
		packBuffer(&pBuffer, pszNick, (WORD)(strlen(pszNick)));
		packByte(&pBuffer, 0);
	}


	// Send it off for further packing
	wCookie = sendTLVSearchPacket(pBuffer.pData, wInfoLen, FALSE);
	SAFE_FREE(pBuffer.pData);


	return wCookie;

}



WORD SearchByEmail(char* pszEmail)
{

	WORD wCookie;
	WORD wInfoLen = 0;
	WORD wEmailLen;
	icq_packet pBuffer; // I reuse the ICQ packet type as a generic buffer
	                    // I should be ashamed! ;)

	_ASSERTE(strlennull(pszEmail));


	wEmailLen = strlennull(pszEmail);
	if (wEmailLen > 0)
	{

		// Calculate data size
		wInfoLen = wEmailLen + 7;

		// Initialize our handy data buffer
		pBuffer.wPlace = 0;
		pBuffer.pData = (BYTE *)calloc(1, wInfoLen);
		pBuffer.wLen = wInfoLen;

		// Pack the search details
		packLEWord(&pBuffer, 0x015E);
		packLEWord(&pBuffer, (WORD)(wEmailLen+3));
		packLEWord(&pBuffer, (WORD)(wEmailLen+1));
		packBuffer(&pBuffer, pszEmail, wEmailLen);
		packByte(&pBuffer, 0);

		// Send it off for further packing
		wCookie = sendTLVSearchPacket(pBuffer.pData, wInfoLen, FALSE);
		SAFE_FREE(pBuffer.pData);
	}

	return wCookie;

}



WORD sendTLVSearchPacket(char* pSearchDataBuf, WORD wInfoLen, BOOL bOnlineUsersOnly)
{

	icq_packet packet;
	WORD wCookie;

	_ASSERTE(pSearchDataBuf);
	_ASSERTE(wInfoLen >= 4);


	wCookie = GenerateCookie();

	// Pack headers
	packServIcqExtensionHeader(&packet, (WORD)(wInfoLen + 7), CLI_META_INFO_REQ, wCookie);

	// Pack search type
	packLEWord(&packet, 0x055f);

	// Pack search data
	packBuffer(&packet, pSearchDataBuf, wInfoLen);

	// Pack "Online users only" flag
	packLEWord(&packet, 0x0230);
	packLEWord(&packet, 0x0001);
	if (bOnlineUsersOnly)
		packByte(&packet, 0x0001);
	else
		packByte(&packet, 0x0000);


	// Go!
	sendServPacket(&packet);

	return wCookie;

}



WORD icq_sendAdvancedSearchServ(BYTE* fieldsBuffer,int bufferLen)
{

	icq_packet packet;
	WORD wCookie;


	wCookie = GenerateCookie();

	packServIcqExtensionHeader(&packet, (WORD)(2 + bufferLen), CLI_META_INFO_REQ, wCookie);
	packWord(&packet, 0x3305);		   /* subtype: full search */
	packBuffer(&packet, (const char*)fieldsBuffer, (WORD)bufferLen);

	sendServPacket(&packet);

	return wCookie;

}



WORD icq_changeUserDetailsServ(WORD type, const unsigned char *pData, WORD wDataLen)
{

	icq_packet packet;
	WORD wCookie;


	wCookie = GenerateCookie();

	packServIcqExtensionHeader(&packet, (WORD)(wDataLen + 2), 0x07D0, wCookie);
	packWord(&packet, type);
	packBuffer(&packet, pData, wDataLen);

	sendServPacket(&packet);

	return wCookie;

}



WORD icq_sendSMSServ(const char *szPhoneNumber, const char *szMsg)
{

	icq_packet packet;
	WORD wCookie;
	WORD wBufferLen;
	char* szBuffer = NULL;
	char* szMyNick = NULL;
	char szTime[30];
	time_t now;
	int nBufferSize;


	now = time(NULL);
	strftime(szTime, sizeof(szTime), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
	                            /* Sun, 00 Jan 0000 00:00:00 GMT */

	szMyNick = _strdup((char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)(HANDLE)NULL, 0));
	nBufferSize = 1 + strlen(szMyNick) + strlen(szPhoneNumber) + strlen(szMsg) + sizeof("<icq_sms_message><destination></destination><text></text><codepage>1252</codepage><encoding>utf8</encoding><senders_UIN>0000000000</senders_UIN><senders_name></senders_name><delivery_receipt>Yes</delivery_receipt><time>Sun, 00 Jan 0000 00:00:00 GMT</time></icq_sms_message>");

	if (szBuffer = (char *)malloc(nBufferSize))
	{

		wBufferLen = _snprintf(szBuffer, nBufferSize,
			"<icq_sms_message>"
			"<destination>"
            "%s"   /* phone number */
			"</destination>"
			"<text>"
            "%s"	 /* body */
			"</text>"
			"<codepage>"
            "1252"
			"</codepage>"
			"<encoding>"
            "utf8"
			"</encoding>"
			"<senders_UIN>"
            "%u"	/* my UIN */
			"</senders_UIN>"
			"<senders_name>"
            "%s"	/* my nick */
			"</senders_name>"
			"<delivery_receipt>"
            "Yes"
			"</delivery_receipt>"
			"<time>"
            "%s"	/* time */
			"</time>"
			"</icq_sms_message>",
			szPhoneNumber, szMsg, dwLocalUIN, szMyNick, szTime);

		wCookie = GenerateCookie();

		packServIcqExtensionHeader(&packet, (WORD)(wBufferLen + 27), 0x07D0, wCookie);
		packWord(&packet, 0x8214);	   /* send sms */
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
		wCookie = 0;
	}


	SAFE_FREE(szMyNick);
	SAFE_FREE(szBuffer);


	return wCookie;

}



void icq_sendNewContact(DWORD dwUin)
{

	icq_packet packet;
	char szUin[10];
	int nUinLen;

	_ltoa(dwUin, szUin, 10);
	nUinLen = strlen(szUin);

	packet.wLen = nUinLen + 11;
	write_flap(&packet, ICQ_DATA_CHAN);
	packFNACHeader(&packet, ICQ_BUDDY_FAMILY, ICQ_USER_ADDTOLIST, 0, 0);
	packByte(&packet, (BYTE)nUinLen);
	packBuffer(&packet, szUin, (BYTE)nUinLen);

	sendServPacket(&packet);

}



// list==0: visible list
// list==1: invisible list
void icq_sendChangeVisInvis(HANDLE hContact, DWORD dwUin, int list, int add)
{

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

			// Add
			wContactId = GenerateServerId();

			icq_sendUploadContactServ(dwUin, 0, wContactId, NULL, FALSE, wType);

			if (wType == SSI_ITEM_DENY)
				DBWriteContactSettingWord(hContact, gpszICQProtoName, "SrvDenyId", wContactId);
			else
				DBWriteContactSettingWord(hContact, gpszICQProtoName, "SrvPermitId", wContactId);

		}
		else
		{

			// Remove
			if (wType == SSI_ITEM_DENY)
			{

				wContactId = DBGetContactSettingWord(hContact, gpszICQProtoName, "SrvDenyId", 0);

				if (wContactId)
					icq_sendDeleteServerContactServ(dwUin, 0, wContactId, SSI_ITEM_DENY);

				DBDeleteContactSetting(hContact, gpszICQProtoName, "SrvDenyId");
			}
			else
			{

				wContactId = DBGetContactSettingWord(hContact, gpszICQProtoName, "SrvPermitId", 0);

				if (wContactId)
					icq_sendDeleteServerContactServ(dwUin, 0, wContactId, SSI_ITEM_PERMIT);

				DBDeleteContactSetting(hContact, gpszICQProtoName, "SrvPermitId");

			}

		}

	}


	// Notify server that we have changed
	// our client side visibility list
	{

		int nUinLen;
		char szUin[10];
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


		_itoa(dwUin, szUin, 10);
		nUinLen = strlen(szUin);

		packet.wLen = nUinLen + 1 + 10;
		write_flap(&packet, ICQ_DATA_CHAN);
		packFNACHeader(&packet, ICQ_BOS_FAMILY, wSnac, 0, wSnac);
		packByte(&packet, (BYTE)nUinLen);
		packBuffer(&packet, szUin, (BYTE)nUinLen);

		sendServPacket(&packet);

	}

}



void icq_sendEntireVisInvisList(int list)
{

	if (list)
		sendEntireListServ(9, 7, 7, BUL_INVISIBLE);
	else
		sendEntireListServ(9, 5, 7, BUL_VISIBLE);

}



void icq_sendAuthReqServ(DWORD dwUin, char *szMsg)
{

	icq_packet packet;
	unsigned char szUin[10];
	unsigned char nUinlen;
	WORD nMsglen;


	ltoa(dwUin, szUin, 10);
	nUinlen = strlen(szUin);
	nMsglen = strlen(szMsg);

	packet.wLen = 15 + nUinlen + nMsglen;

	write_flap(&packet, ICQ_DATA_CHAN);
	packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_REQUESTAUTH, 0, 0);
	packByte(&packet, nUinlen);
	packBuffer(&packet, szUin, nUinlen);
	packWord(&packet, (WORD)nMsglen);
	packBuffer(&packet, szMsg, nMsglen);
	packWord(&packet, 0);

	sendServPacket(&packet);

}



void icq_sendAuthResponseServ(DWORD dwUin, int auth, char *szReason)
{

	icq_packet p;
	WORD nReasonlen;
	unsigned char szUin[10], nUinlen;


	ltoa(dwUin, szUin, 10);
	nUinlen = strlen(szUin);
	nReasonlen = strlen(szReason);

	p.wLen = 16 + nUinlen + nReasonlen;
	write_flap(&p, ICQ_DATA_CHAN);
	packFNACHeader(&p, ICQ_LISTS_FAMILY, ICQ_LISTS_AUTHRESPONSE, 0, 0x1A);
	packByte(&p, nUinlen);
	packBuffer(&p, szUin, nUinlen);
	packByte(&p, (BYTE)auth);
	packWord(&p, nReasonlen);
	packBuffer(&p, szReason, nReasonlen);
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

	packServMsgSendHeader(&packet, 0, dwID1, dwID2, dwUin, 0x0004, 17);
	packWord(&packet, 0x0005);		// TLV(5)
	packWord(&packet, 0x0009);
	packLEDWord(&packet, dwMyUin);
	packByte(&packet, MTYPE_ADDED);
	packByte(&packet, 0);			// msg-flags
	packLEWord(&packet, 0x0001);	// len of NTS
	packByte(&packet, 0);			// NTS
	packWord(&packet, 0x0006);		// TLV(6)
	packWord(&packet, 0);

	sendServPacket(&packet);

}
