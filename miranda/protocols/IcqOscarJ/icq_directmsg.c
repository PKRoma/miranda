// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005 Joe Kucera
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



extern icq_mode_messages modeMsgs;
extern CRITICAL_SECTION modeMsgsMutex;
extern WORD wListenPort;
extern HANDLE hDirectNetlibUser, hsmsgrequest;
extern char gpszICQProtoName[MAX_PATH];

void icq_sendAwayMsgReplyDirect(directconnect *dc, WORD wCookie, BYTE msgType, const char **szMsg);
void handleDirectGreetingMessage(directconnect *dc, PBYTE buf, WORD wLen, WORD wCommand, WORD wCookie, WORD wMessageType, WORD wStatus, WORD wFlags, char* pszText);
void handleDirectNormalMessage(directconnect *dc, PBYTE buf, WORD wLen, WORD wCommand, WORD wCookie, WORD wMessageType, WORD wStatus, WORD wFlags, char* pszText, WORD wTextLen);



void buildDirectPacketHeader(icq_packet* packet, WORD wDataLen, WORD wCommand, DWORD dwCookie, BYTE bMsgType, BYTE bMsgFlags, WORD wX1)
{

	directPacketInit(packet, 27 + wDataLen);
	packByte(packet, 2);	   /* channel */
	packLEDWord(packet, 0);   /* space for crypto */
	packLEWord(packet, wCommand);
	packLEWord(packet, 14);		/* unknown */
	packLEWord(packet, (WORD)dwCookie);
	packLEDWord(packet, 0);	  /* unknown */
	packLEDWord(packet, 0);	  /* unknown */
	packLEDWord(packet, 0);	  /* unknown */
	packByte(packet, bMsgType);
	packByte(packet, bMsgFlags);
	packLEWord(packet, wX1);	   /* unknown. Is 1 for getawaymsg, 0 otherwise */

}



void handleDirectMessage(directconnect* dc, PBYTE buf, WORD wLen)
{

	WORD wCommand;
	WORD wCookie;
	WORD wMessageType;
	WORD wStatus;
	WORD wFlags;
	WORD wTextLen;
	char* pszText = NULL;


	// The first part of the packet should always be at least 31 bytes
	if (wLen < 31)
	{
		Netlib_Logf(hDirectNetlibUser, "Error during parsing of DC packet 2 PEER_MSG (too short)");
		return;
	}
	
	// Skip packet checksum
	buf += 4;
	wLen -= 4;

	// Command:
	//   0x07d0 = 2000 - cancel given message.
	//   0x07da = 2010 - acknowledge message.
	//   0x07ee = 2030 - normal message/request.
	unpackLEWord(&buf, &wCommand);
	wLen -= 2;

	// Unknown, always 0xe (14)
	buf += 2;
	wLen -= 2;

	// Sequence number
	unpackLEWord(&buf, &wCookie);
	wLen -=2;

	// Unknown, always zeroes
	buf += 12;
	wLen -= 12;
	
	// Peer message type
	unpackLEWord(&buf, &wMessageType);
	wLen -= 2;

	// The current status of the user, or whether the message was accepted or not.
	//   0x00 - user is online, or message was receipt, or file transfer accepted
	//   0x01 - refused
	//   0x04 - auto-refused, because of away
	//   0x09 - auto-refused, because of occupied
	//   0x0a - auto-refused, because of dnd
	//   0x0e - auto-refused, because of na
	unpackLEWord(&buf, &wStatus);
	wLen -= 2;

	// Flags
	// Seen: 1 - Chat request
	//       0 - File auto accept (type 3)
	unpackLEWord(&buf, &wFlags);
	wLen -= 2;

	// Messagetext. This is either the status message or the actual message
	// when this is a PEER_MSG_MSG packet
	unpackLEWord(&buf, &wTextLen);
	if (wTextLen > 0)
	{
		pszText = malloc(wTextLen+1);
		unpackString(&buf, pszText, wTextLen);
		pszText[wTextLen] = '\0';
	}
	wLen = (wLen - 2) - wTextLen;


	Netlib_Logf(hDirectNetlibUser, "Handling PEER_MSG '%s', command %u, cookie %u, messagetype %u, status %u, flags %u", pszText, wCommand, wCookie, wMessageType, wStatus, wFlags);


	// The remaining actual message is handled either as a status message request,
	// a greeting message, a acknowledge or a normal (text, url, file) message
	switch (wMessageType)
	{

	case MTYPE_FILEREQ: // File inits and acks
		if (wCommand == DIRECT_MESSAGE)
			handleFileRequest(buf, wLen, dc->dwRemoteUin, wCookie, 0, 0, pszText, 7);
		else if (wCommand == DIRECT_ACK)
			handleFileAck(buf, wLen, dc->dwRemoteUin, wCookie, wStatus, pszText);
		else 
			Netlib_Logf(hDirectNetlibUser, "Skipped FILE message from direct connection");
		break;

	case 0x03e8:
		if (wCommand == DIRECT_MESSAGE)
			icq_sendAwayMsgReplyDirect(dc, wCookie, MTYPE_AUTOAWAY, &modeMsgs.szAway);
		break;
		
	case 0x03e9:
		if (wCommand == DIRECT_MESSAGE)
			icq_sendAwayMsgReplyDirect(dc, wCookie, MTYPE_AUTOBUSY, &modeMsgs.szOccupied);
		break;
		
	case 0x03ea:
		if (wCommand == DIRECT_MESSAGE)
			icq_sendAwayMsgReplyDirect(dc, wCookie, MTYPE_AUTONA, &modeMsgs.szNa);
		break;
		
	case 0x03eb:
		if (wCommand == DIRECT_MESSAGE)
			icq_sendAwayMsgReplyDirect(dc, wCookie, MTYPE_AUTODND, &modeMsgs.szDnd);
		break;
		
	case 0x03ec:
		if (wCommand == DIRECT_MESSAGE)
			icq_sendAwayMsgReplyDirect(dc, wCookie, MTYPE_AUTOFFC, &modeMsgs.szFfc);
		break;

	case MTYPE_PLUGIN: // Greeting
		if (wCommand == DIRECT_MESSAGE || wCommand == DIRECT_ACK)
			handleDirectGreetingMessage(dc, buf, wLen, wCommand, wCookie, wMessageType, wStatus, wFlags, pszText);
		else
			Netlib_Logf(hDirectNetlibUser, "Skipped packet from direct connection");
		break;

	default:
		if (wCommand == DIRECT_MESSAGE)
			handleDirectNormalMessage(dc, buf, wLen, wCommand, wCookie, wMessageType, wStatus, wFlags, pszText, wTextLen);
		else
			Netlib_Logf(hDirectNetlibUser, "Skipped packet from direct connection");
		break;

	}

	// Clean up allocated memory
	SAFE_FREE(&pszText);
	
}



void handleDirectNormalMessage(directconnect* dc, PBYTE buf, WORD wLen, WORD wCommand, WORD wCookie, WORD wMessageType, WORD wStatus, WORD wFlags, char* pszText, WORD wTextLen)
{

	icq_packet packet;
	BYTE bMsgSubtype;
	BYTE bMsgFlags;


	Netlib_Logf(hDirectNetlibUser, "Normal message thru direct connection");


	// TEMP - restore old variables
	if (wMessageType > 999)
		bMsgFlags = 3;
	else
		bMsgFlags = 0;
	if (wMessageType > 999)
		bMsgSubtype = wMessageType-0x300;
	else
		bMsgSubtype = (BYTE)wMessageType;
	// !TEMP

	if (bMsgSubtype == MTYPE_FILEREQ)
	{
		handleFileRequest(buf, wLen, dc->dwRemoteUin, wCookie, 0, 0, pszText, 7);
	}
	else
	{

		buf -= wTextLen;
		wLen += wTextLen;

		handleMessageTypes(dc->dwRemoteUin, time(NULL), 0, 0, wCookie, (int)bMsgSubtype, (int)bMsgFlags, 0, (DWORD)wLen, wTextLen, buf);
		
		// Send acknowledgement
		if (bMsgSubtype == MTYPE_PLAIN || bMsgSubtype == MTYPE_URL)
		{
			buildDirectPacketHeader(&packet, (WORD)(bMsgSubtype==MTYPE_PLAIN ? 55 : 5), DIRECT_ACK, wCookie, bMsgSubtype, bMsgFlags, 0);
			packLEWord(&packet, 0);	   /* modifier: 0 for an ack */
			packLEWord(&packet, 1);	 /* empty message */
			packByte(&packet, 0);    /* message */
			if (bMsgSubtype == MTYPE_PLAIN)
			{
				packLEDWord(&packet, 0);   /* foreground colour */
				packLEDWord(&packet, RGB(255,255,255));   /* background colour */
				packLEDWord(&packet, 0x26);   /* CLSID length */
				packBuffer(&packet, "{97B12751-243C-4334-AD22-D6ABF73F1492}", 0x26);
			}
			EncryptDirectPacket(dc, &packet);
			sendDirectPacket(dc->hConnection, &packet);
			Netlib_Logf(hDirectNetlibUser, "Send acknowledgement thru direct connection");
		}

	}

}



void handleDirectGreetingMessage(directconnect* dc, PBYTE buf, WORD wLen, WORD wCommand, WORD wCookie, WORD wMessageType, WORD wStatus, WORD wFlags, char* pszText)
{

	DWORD dwMsgTypeLen;
	DWORD dwLengthToEnd;
	DWORD dwDataLength;
	char* szMsgType = NULL;
	char* pszFileName = NULL;
	int typeId;
	WORD wPacketCommand;


	Netlib_Logf(hDirectNetlibUser, "Handling PEER_MSG_GREETING, command %u, cookie %u, messagetype %u, status %u, flags %u", wCommand, wCookie, wMessageType, wStatus, wFlags);


	// The command in this packet. Seen values:
	//          0x0029 =  41 - file request
	//          0x002d =  45 - chat request
	//          0x0032 =  50 - file request granted/refused
	//                    55 - Greeting card
	//          0x0040 =  64 - URL
	unpackLEWord(&buf, &wPacketCommand);
	wLen -= 2;
	
	// Some binary id string
	buf += 16;
	wLen -= 16;

	// Unknown
	buf += 2;
	wLen -= 2;

	// A text string
	// "ICQ Chat" for chat request, "File" for file request,
	// "File Transfer" for file request grant/refusal. This text is
	// displayed in the requester opened by Windows.
	unpackLEDWord(&buf, &dwMsgTypeLen);
	wLen -= 4;
	if (dwMsgTypeLen == 0 || dwMsgTypeLen>256)
	{
		Netlib_Logf(hDirectNetlibUser, "Error: Sanity checking failed (1) in handleDirectGreetingMessage, len is %u", dwMsgTypeLen);
		return;
	}
	szMsgType = (char *)malloc(dwMsgTypeLen + 1);
	memcpy(szMsgType, buf, dwMsgTypeLen);
	szMsgType[dwMsgTypeLen] = '\0';
	typeId = TypeStringToTypeId(szMsgType);
	buf += dwMsgTypeLen;
	wLen -= (WORD)dwMsgTypeLen;


	Netlib_Logf(hDirectNetlibUser, "PEER_MSG_GREETING, command: %u, type: %s, typeID: %u", wPacketCommand, szMsgType, typeId);


	// Unknown
	buf += 15;
	wLen -= 15;
	
	// Length of remaining data
	unpackLEDWord(&buf, &dwLengthToEnd);
	if (dwLengthToEnd < 4 || dwLengthToEnd > wLen)
	{
		Netlib_Logf(hDirectNetlibUser, "Error: Sanity checking failed (2) in handleDirectGreetingMessage, datalen %u wLen %u", dwLengthToEnd, wLen);
		return;
	}
	
	// Length of message/reason
	unpackLEDWord(&buf, &dwDataLength);
	wLen -= 4;
	if (dwDataLength > wLen)
	{
		Netlib_Logf(hDirectNetlibUser, "Error: Sanity checking failed (3) in handleDirectGreetingMessage, dwDataLength %u wLen %u", dwDataLength, wLen);
		return;
	}


	if (typeId == MTYPE_FILEREQ && wPacketCommand == 0x0029)
	{

		char* szMsg;

		
		Netlib_Logf(hDirectNetlibUser, "This is file ack");
		szMsg = malloc(dwDataLength+1);
		unpackString(&buf, szMsg, (WORD)dwDataLength);
		szMsg[dwDataLength] = '\0';
		wLen = wLen - (WORD)dwDataLength;

		handleFileRequest(buf, wLen, dc->dwRemoteUin, wCookie, 0, 0, szMsg, 8);
		SAFE_FREE(&szMsg);
	}
	else if (typeId == MTYPE_FILEREQ && wPacketCommand == 0x0032)
	{

		char* szMsg;

		
		Netlib_Logf(hDirectNetlibUser, "This is file ack");
		szMsg = malloc(dwDataLength+1);
		unpackString(&buf, szMsg, (WORD)dwDataLength);
		szMsg[dwDataLength] = '\0';
		wLen = wLen - (WORD)dwDataLength;

		// 50 - file request granted/refused
		handleFileAck(buf, wLen, dc->dwRemoteUin, wCookie, wStatus, szMsg);
		SAFE_FREE(&szMsg);
	}
	else if (typeId)
	{
		if (typeId == MTYPE_URL)
		{
			icq_sendAdvancedMsgAck(dc->dwRemoteUin, 0, 0, wCookie, (BYTE)typeId, 0);
		}
		handleMessageTypes(dc->dwRemoteUin, time(NULL), 0, 0, wCookie, (BYTE)typeId, 0, 0, dwLengthToEnd, (WORD)dwDataLength, buf);
	}
	else
	{
		Netlib_Logf(hDirectNetlibUser, "Unsupported plugin message type '%s'", szMsgType);
	}

	SAFE_FREE(&szMsgType);
	
}



void icq_sendAwayMsgReplyDirect(directconnect* dc, WORD wCookie, BYTE msgType, const char** szMsg)
{

	icq_packet packet;
	WORD wMsgLen;
	HANDLE hContact;


	hContact = HContactFromUIN(dc->dwRemoteUin, 0);

	if (validateStatusMessageRequest(hContact, msgType))
	{
	
		NotifyEventHooks(hsmsgrequest, (WPARAM)msgType, (LPARAM)dc->dwRemoteUin);
		
		EnterCriticalSection(&modeMsgsMutex);

		if (*szMsg != NULL)
		{
			wMsgLen = strlen(*szMsg);
			buildDirectPacketHeader(&packet, (WORD)(5 + wMsgLen), DIRECT_ACK, wCookie, msgType, 3, 0);
			packLEWord(&packet, 0); // 0 for an ack
			packLEWord(&packet, (WORD)(wMsgLen + 1));
			packBuffer(&packet, *szMsg, (WORD)(wMsgLen + 1));
			EncryptDirectPacket(dc, &packet);

			sendDirectPacket(dc->hConnection, &packet);
		}

		LeaveCriticalSection(&modeMsgsMutex);
		
	}

}
