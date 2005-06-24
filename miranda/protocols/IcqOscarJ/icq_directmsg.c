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
extern WORD wListenPort;

void icq_sendAwayMsgReplyDirect(directconnect *dc, WORD wCookie, BYTE msgType, const char **szMsg);

void handleDirectGreetingMessage(directconnect *dc, PBYTE buf, WORD wLen, WORD wCommand, WORD wCookie, BYTE bMsgType, BYTE bMsgFlags, WORD wStatus, WORD wFlags, char* pszText);



void handleDirectMessage(directconnect* dc, PBYTE buf, WORD wLen)
{
	WORD wCommand;
	WORD wCookie;
	BYTE bMsgType,bMsgFlags;
	WORD wStatus;
	WORD wFlags;
	WORD wTextLen;
	char* pszText = NULL;


	// The first part of the packet should always be at least 31 bytes
	if (wLen < 31)
	{
		Netlib_Logf(ghDirectNetlibUser, "Error during parsing of DC packet 2 PEER_MSG (too short)");
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
	unpackByte(&buf, &bMsgType);
  // Peer message flags
  unpackByte(&buf, &bMsgFlags);
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

	// Flags, or priority
	// Seen: 1 - Chat request
	//       0 - File auto accept (type 3)
  //       33 - priority ?
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


  Netlib_Logf(ghDirectNetlibUser, "Handling PEER_MSG '%s', command %u, cookie %u, messagetype %u, messageflags %u, status %u, flags %u", pszText, wCommand, wCookie, bMsgType, bMsgFlags, wStatus, wFlags);

  // The remaining actual message is handled either as a status message request,
  // a greeting message, a acknowledge or a normal (text, url, file) message
  if (wCommand == DIRECT_MESSAGE)
    switch (bMsgType)
    {
      case MTYPE_FILEREQ: // File inits
        handleFileRequest(buf, wLen, dc->dwRemoteUin, wCookie, 0, 0, pszText, 7, TRUE);
        break;

      case MTYPE_AUTOAWAY: // TODO: let this handle handleMessageTypes
        icq_sendAwayMsgReplyDirect(dc, wCookie, MTYPE_AUTOAWAY, &modeMsgs.szAway);
        break;

      case MTYPE_AUTOBUSY:
        icq_sendAwayMsgReplyDirect(dc, wCookie, MTYPE_AUTOBUSY, &modeMsgs.szOccupied);
        break;

      case MTYPE_AUTONA:
        icq_sendAwayMsgReplyDirect(dc, wCookie, MTYPE_AUTONA, &modeMsgs.szNa);
        break;

      case MTYPE_AUTODND:
        icq_sendAwayMsgReplyDirect(dc, wCookie, MTYPE_AUTODND, &modeMsgs.szDnd);
        break;

      case MTYPE_AUTOFFC:
        icq_sendAwayMsgReplyDirect(dc, wCookie, MTYPE_AUTOFFC, &modeMsgs.szFfc);
        break;

      case MTYPE_PLUGIN: // Greeting
        handleDirectGreetingMessage(dc, buf, wLen, wCommand, wCookie, bMsgType, bMsgFlags, wStatus, wFlags, pszText);
        break;

      default: 
        {
	        buf -= wTextLen;
	        wLen += wTextLen;

	        handleMessageTypes(dc->dwRemoteUin, time(NULL), 0, 0, wCookie, (int)bMsgType, (int)bMsgFlags, 0, (DWORD)wLen, wTextLen, buf, TRUE);
		
	        // Send acknowledgement
	        if (bMsgType == MTYPE_PLAIN || bMsgType == MTYPE_URL || bMsgType == MTYPE_CONTACTS)
	        {
            icq_sendDirectMsgAck(dc, wCookie, bMsgType, bMsgFlags, CAP_RTFMSGS);
	        }
          break;
        }
    }
  else if (wCommand == DIRECT_ACK)
  {
    if (bMsgFlags == 3)
    { // this is status reply
      buf -= wTextLen;
      wLen += wTextLen;

      handleMessageTypes(dc->dwRemoteUin, time(NULL), 0, 0, wCookie, (int)bMsgType, (int)bMsgFlags, 2, (DWORD)wLen, wTextLen, buf, TRUE);
    }
    else
    {
      DWORD dwCookieUin;
      message_cookie_data* pCookieData = NULL;

      if (!FindCookie(wCookie, &dwCookieUin, &pCookieData))
      {
        Netlib_Logf(ghServerNetlibUser, "Received an unexpected direct ack");
      }
      else if (dwCookieUin != dc->dwRemoteUin)
      {
        Netlib_Logf(ghServerNetlibUser, "Direct UIN does not match Cookie UIN(%u != %u)", dc->dwRemoteUin, dwCookieUin);
        SAFE_FREE(&pCookieData); // This could be a bad idea, but I think it is safe
        FreeCookie(wCookie);
      }
      else
      { // the ack is correct
        int ackType = -1;

        switch (bMsgType)
        { 
          case MTYPE_PLAIN:
            ackType = ACKTYPE_MESSAGE;
            break;
          case MTYPE_URL:
            ackType = ACKTYPE_URL;
            break;
          case MTYPE_CONTACTS:
            ackType = ACKTYPE_CONTACTS;
            break;

          case MTYPE_FILEREQ: // File acks
            handleFileAck(buf, wLen, dc->dwRemoteUin, wCookie, wStatus, pszText);
            break;

          case MTYPE_PLUGIN: // Greeting
            handleDirectGreetingMessage(dc, buf, wLen, wCommand, wCookie, bMsgType, bMsgFlags, wStatus, wFlags, pszText);
            break;

          default: 
            Netlib_Logf(ghDirectNetlibUser, "Skipped packet from direct connection");
            break;
        }
        if (ackType != -1)
        { // was a good ack to broadcast ?
   		    ProtoBroadcastAck(gpszICQProtoName, dc->hContact, ackType, ACKRESULT_SUCCESS, (HANDLE)wCookie, 0);
          SAFE_FREE(&pCookieData);
          FreeCookie(wCookie);
        }
      }
    }
  }
  else
    Netlib_Logf(ghDirectNetlibUser, "Unknown wCommand, packet skipped");

	// Clean up allocated memory
	SAFE_FREE(&pszText);
}


void handleDirectGreetingMessage(directconnect* dc, PBYTE buf, WORD wLen, WORD wCommand, WORD wCookie, BYTE bMsgType, BYTE bMsgFlags, WORD wStatus, WORD wFlags, char* pszText)
{
	DWORD dwMsgTypeLen;
	DWORD dwLengthToEnd;
	DWORD dwDataLength;
	char* szMsgType = NULL;
	char* pszFileName = NULL;
	int typeId;
	WORD wPacketCommand;
  DWORD q1,q2,q3,q4;
  WORD qt;


	Netlib_Logf(ghDirectNetlibUser, "Handling PEER_MSG_GREETING, command %u, cookie %u, messagetype %u, messageflags %u, status %u, flags %u", wCommand, wCookie, bMsgType, bMsgFlags, wStatus, wFlags);


	// The command in this packet. Seen values:
	//          0x0029 =  41 - file request
	//          0x002d =  45 - chat request
	//          0x0032 =  50 - file request granted/refused
	//                    55 - Greeting card
	//          0x0040 =  64 - URL
	unpackLEWord(&buf, &wPacketCommand); // TODO: this is most probably length...
	wLen -= 2;
	
	// Data type GUID
  unpackDWord(&buf, &q1);
  unpackDWord(&buf, &q2);
  unpackDWord(&buf, &q3);
  unpackDWord(&buf, &q4);
	wLen -= 16;

	// Data type function id
  unpackLEWord(&buf, &qt);
	wLen -= 2;

	// A text string
	// "ICQ Chat" for chat request, "File" for file request,
	// "File Transfer" for file request grant/refusal. This text is
	// displayed in the requester opened by Windows.
	unpackLEDWord(&buf, &dwMsgTypeLen);
	wLen -= 4;
	if (dwMsgTypeLen == 0 || dwMsgTypeLen>256)
	{
		Netlib_Logf(ghDirectNetlibUser, "Error: Sanity checking failed (1) in handleDirectGreetingMessage, len is %u", dwMsgTypeLen);
		return;
	}
	szMsgType = (char *)malloc(dwMsgTypeLen + 1);
	memcpy(szMsgType, buf, dwMsgTypeLen);
	szMsgType[dwMsgTypeLen] = '\0';
	//typeId = TypeStringToTypeId(szMsgType);
  typeId = TypeGUIDToTypeId(q1,q2,q3,q4,qt);
  if (!typeId)
    Netlib_Logf(ghServerNetlibUser, "Error: Unknown type {%04x%04x%04x%04x-%02x}: %s", q1,q2,q3,q4,qt);

	buf += dwMsgTypeLen;
	wLen -= (WORD)dwMsgTypeLen;


	Netlib_Logf(ghDirectNetlibUser, "PEER_MSG_GREETING, command: %u, type: %s, typeID: %u", wPacketCommand, szMsgType, typeId);


	// Unknown
	buf += 15;
	wLen -= 15;
	
	// Length of remaining data
	unpackLEDWord(&buf, &dwLengthToEnd);
	if (dwLengthToEnd < 4 || dwLengthToEnd > wLen)
	{
		Netlib_Logf(ghDirectNetlibUser, "Error: Sanity checking failed (2) in handleDirectGreetingMessage, datalen %u wLen %u", dwLengthToEnd, wLen);
		return;
	}
	
	// Length of message/reason
	unpackLEDWord(&buf, &dwDataLength);
	wLen -= 4;
	if (dwDataLength > wLen)
	{
		Netlib_Logf(ghDirectNetlibUser, "Error: Sanity checking failed (3) in handleDirectGreetingMessage, dwDataLength %u wLen %u", dwDataLength, wLen);
		return;
	}


	if (typeId == MTYPE_FILEREQ && wCommand == DIRECT_MESSAGE)
	{
		char* szMsg;

		
		Netlib_Logf(ghDirectNetlibUser, "This is file request");
		szMsg = malloc(dwDataLength+1);
		unpackString(&buf, szMsg, (WORD)dwDataLength);
		szMsg[dwDataLength] = '\0';
		wLen = wLen - (WORD)dwDataLength;

		handleFileRequest(buf, wLen, dc->dwRemoteUin, wCookie, 0, 0, szMsg, 8, TRUE);
		SAFE_FREE(&szMsg);
	}
	else if (typeId == MTYPE_FILEREQ && wCommand == DIRECT_ACK)
	{
		char* szMsg;

		
		Netlib_Logf(ghDirectNetlibUser, "This is file ack");
		szMsg = malloc(dwDataLength+1);
		unpackString(&buf, szMsg, (WORD)dwDataLength);
		szMsg[dwDataLength] = '\0';
		wLen = wLen - (WORD)dwDataLength;

		// 50 - file request granted/refused
		handleFileAck(buf, wLen, dc->dwRemoteUin, wCookie, wStatus, szMsg);
		SAFE_FREE(&szMsg);
	}
	else if (typeId && wCommand == DIRECT_MESSAGE)
	{
		if (typeId == MTYPE_URL || typeId == MTYPE_CONTACTS)
    { 
      icq_sendDirectMsgAck(dc, wCookie, (BYTE)typeId, 0, CAP_RTFMSGS);
		}
		handleMessageTypes(dc->dwRemoteUin, time(NULL), 0, 0, wCookie, typeId, 0, 0, dwLengthToEnd, (WORD)dwDataLength, buf, TRUE);
	}
  else if (typeId && wCommand == DIRECT_ACK)
  {
    DWORD dwCookieUin;
    message_cookie_data* pCookieData = NULL;

    if (!FindCookie(wCookie, &dwCookieUin, &pCookieData))
    {
      Netlib_Logf(ghServerNetlibUser, "Received an unexpected direct ack");
    }
    else if (dwCookieUin != dc->dwRemoteUin)
    {
      Netlib_Logf(ghServerNetlibUser, "Direct UIN does not match Cookie UIN(%u != %u)", dc->dwRemoteUin, dwCookieUin);
      SAFE_FREE(&pCookieData); // This could be a bad idea, but I think it is safe
      FreeCookie(wCookie);
    }
    else
    {
      int ackType = -1;

      switch (typeId)
      {
      case MTYPE_URL:
        ackType = ACKTYPE_URL;
        break;
      case MTYPE_CONTACTS:
        ackType = ACKTYPE_CONTACTS;
        break;
      default:
        Netlib_Logf(ghDirectNetlibUser, "Skipped packet from direct connection");
        break;
      }

      if (ackType != -1)
      { // was a good ack to broadcast ?
	      ProtoBroadcastAck(gpszICQProtoName, dc->hContact, ackType, ACKRESULT_SUCCESS, (HANDLE)wCookie, 0);
        SAFE_FREE(&pCookieData);
        FreeCookie(wCookie);
      }
    }
  }
	else
	{
		Netlib_Logf(ghDirectNetlibUser, "Unsupported plugin message type '%s'", szMsgType);
	}

	SAFE_FREE(&szMsgType);
}
