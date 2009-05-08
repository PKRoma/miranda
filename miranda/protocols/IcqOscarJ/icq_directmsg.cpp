// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001-2002 Jon Keating, Richard Hughes
// Copyright � 2002-2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004-2009 Joe Kucera
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

#include "icqoscar.h"

void CIcqProto::handleDirectMessage(directconnect* dc, PBYTE buf, WORD wLen)
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
		NetLog_Direct("Error during parsing of DC packet 2 PEER_MSG (too short)");
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
		pszText = (char*)_alloca(wTextLen+1);
		unpackString(&buf, pszText, wTextLen);
		pszText[wTextLen] = '\0';
	}
	wLen = (wLen - 2) - wTextLen;

#ifdef _DEBUG
	NetLog_Direct("Handling PEER_MSG '%s', command %u, cookie %u, messagetype %u, messageflags %u, status %u, flags %u", pszText, wCommand, wCookie, bMsgType, bMsgFlags, wStatus, wFlags);
#else
	NetLog_Direct("Message through direct - UID: %u", dc->dwRemoteUin);
#endif

	// The remaining actual message is handled either as a status message request,
	// a greeting message, a acknowledge or a normal (text, url, file) message
	if (wCommand == DIRECT_MESSAGE)
		switch (bMsgType)
	{
		case MTYPE_FILEREQ: // File inits
			handleFileRequest(buf, wLen, dc->dwRemoteUin, wCookie, 0, 0, pszText, 7, TRUE);
			break;

		case MTYPE_AUTOAWAY:
		case MTYPE_AUTOBUSY:
		case MTYPE_AUTONA:
		case MTYPE_AUTODND:
		case MTYPE_AUTOFFC:
			{
				char **szMsg = MirandaStatusToAwayMsg(AwayMsgTypeToStatus(bMsgType));
				if (szMsg)
					icq_sendAwayMsgReplyDirect(dc, wCookie, bMsgType, ( const char** )szMsg);
			}
			break;

		case MTYPE_PLUGIN: // Greeting
			handleDirectGreetingMessage(dc, buf, wLen, wCommand, wCookie, bMsgType, bMsgFlags, wStatus, wFlags, pszText);
			break;

		default: 
			{
				message_ack_params pMsgAck = {0};

				buf -= wTextLen;
				wLen += wTextLen;

				pMsgAck.bType = MAT_DIRECT;
				pMsgAck.pDC = dc;
				pMsgAck.wCookie = wCookie;
				pMsgAck.msgType = bMsgType;
				pMsgAck.bFlags = bMsgFlags;
				handleMessageTypes(dc->dwRemoteUin, time(NULL), 0, 0, wCookie, dc->wVersion, (int)bMsgType, (int)bMsgFlags, 0, (DWORD)wLen, wTextLen, (char*)buf, MTF_DIRECT, &pMsgAck);
				break;
			}
	}
	else if (wCommand == DIRECT_ACK)
	{
		if (bMsgFlags == 3)
		{ // this is status reply
			buf -= wTextLen;
			wLen += wTextLen;

			handleMessageTypes(dc->dwRemoteUin, time(NULL), 0, 0, wCookie, dc->wVersion, (int)bMsgType, (int)bMsgFlags, 2, (DWORD)wLen, wTextLen, (char*)buf, MTF_DIRECT, NULL);
		}
		else
		{
			HANDLE hCookieContact;
			cookie_message_data *pCookieData = NULL;

			if (!FindCookie(wCookie, &hCookieContact, (void**)&pCookieData))
			{
				NetLog_Direct("Received an unexpected direct ack");
			}
			else if (hCookieContact != dc->hContact)
			{
				NetLog_Direct("Direct Contact does not match Cookie Contact(0x%x != 0x%x)", dc->hContact, hCookieContact);
				ReleaseCookie(wCookie); // This could be a bad idea, but I think it is safe
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
					NetLog_Direct("Skipped packet from direct connection");
					break;
				}
				if (ackType != -1)
				{ // was a good ack to broadcast ?
					BroadcastAck(dc->hContact, ackType, ACKRESULT_SUCCESS, (HANDLE)wCookie, 0);
					ReleaseCookie(wCookie);
				}
			}
		}
	}
	else if (wCommand == DIRECT_CANCEL)
	{
		NetLog_Direct("Cannot handle abort messages yet... :(");
	}
	else
		NetLog_Direct("Unknown wCommand, packet skipped");
}

void CIcqProto::handleDirectGreetingMessage(directconnect* dc, PBYTE buf, WORD wLen, WORD wCommand, WORD wCookie, BYTE bMsgType, BYTE bMsgFlags, WORD wStatus, WORD wFlags, char* pszText)
{
	DWORD dwLengthToEnd;
	DWORD dwDataLength;
	char* pszFileName = NULL;
	int typeId;
	WORD qt;

#ifdef _DEBUG
	NetLog_Direct("Handling PEER_MSG_GREETING, command %u, cookie %u, messagetype %u, messageflags %u, status %u, flags %u", wCommand, wCookie, bMsgType, bMsgFlags, wStatus, wFlags);
#endif

	NetLog_Direct("Parsing Greeting message through direct");

	if (!unpackPluginTypeId(&buf, &wLen, &typeId, &qt, TRUE)) return;

	// Length of remaining data
	unpackLEDWord(&buf, &dwLengthToEnd);
	if (dwLengthToEnd < 4 || dwLengthToEnd > wLen)
	{
		NetLog_Direct("Error: Sanity checking failed (%d) in handleDirectGreetingMessage, datalen %u wLen %u", 2, dwLengthToEnd, wLen);
		return;
	}

	// Length of message/reason
	unpackLEDWord(&buf, &dwDataLength);
	wLen -= 4;
	if (dwDataLength > wLen)
	{
		NetLog_Direct("Error: Sanity checking failed (%d) in handleDirectGreetingMessage, datalen %u wLen %u", 3, dwDataLength, wLen);
		return;
	}

	if (typeId == MTYPE_FILEREQ && wCommand == DIRECT_MESSAGE)
	{
		char* szMsg;

		NetLog_Direct("This is file request");
		szMsg = (char*)_alloca(dwDataLength+1);
		unpackString(&buf, szMsg, (WORD)dwDataLength);
		szMsg[dwDataLength] = '\0';
		wLen = wLen - (WORD)dwDataLength;

		handleFileRequest(buf, wLen, dc->dwRemoteUin, wCookie, 0, 0, szMsg, 8, TRUE);
	}
	else if (typeId == MTYPE_FILEREQ && wCommand == DIRECT_ACK)
	{
		char* szMsg;

		NetLog_Direct("This is file ack");
		szMsg = (char*)_alloca(dwDataLength+1);
		unpackString(&buf, szMsg, (WORD)dwDataLength);
		szMsg[dwDataLength] = '\0';
		wLen = wLen - (WORD)dwDataLength;

		// 50 - file request granted/refused
		handleFileAck(buf, wLen, dc->dwRemoteUin, wCookie, wStatus, szMsg);
	}
	else if (typeId && wCommand == DIRECT_MESSAGE)
	{
		message_ack_params pMsgAck = {0};

		pMsgAck.bType = MAT_DIRECT;
		pMsgAck.pDC = dc;
		pMsgAck.wCookie = wCookie;
		pMsgAck.msgType = typeId;
		handleMessageTypes(dc->dwRemoteUin, time(NULL), 0, 0, wCookie, dc->wVersion, typeId, 0, 0, dwLengthToEnd, (WORD)dwDataLength, (char*)buf, MTF_PLUGIN | MTF_DIRECT, &pMsgAck);
	}
	else if (typeId == MTYPE_STATUSMSGEXT && wCommand == DIRECT_ACK)
	{ // especially for icq2003b
		char* szMsg;

		NetLog_Direct("This is extended status reply");
		szMsg = (char*)_alloca(dwDataLength+1);
		unpackString(&buf, szMsg, (WORD)dwDataLength);
		szMsg[dwDataLength] = '\0';

		handleMessageTypes(dc->dwRemoteUin, time(NULL), 0, 0, wCookie, dc->wVersion, (int)(qt + 0xE7), 3, 2, (DWORD)wLen, (WORD)dwDataLength, szMsg, MTF_PLUGIN | MTF_DIRECT, NULL);
	}
	else if (typeId && wCommand == DIRECT_ACK)
	{
		HANDLE hCookieContact;
		cookie_message_data *pCookieData = NULL;

		if (!FindCookie(wCookie, &hCookieContact, (void**)&pCookieData))
		{
			NetLog_Direct("Received an unexpected direct ack");
		}
		else if (hCookieContact != dc->hContact)
		{
			NetLog_Direct("Direct Contact does not match Cookie Contact(0x%x != 0x%x)", dc->hContact, hCookieContact);
			ReleaseCookie(wCookie); // This could be a bad idea, but I think it is safe
		}
		else
		{
			int ackType = -1;

			switch (typeId)
			{
			case MTYPE_MESSAGE:
				ackType = ACKTYPE_MESSAGE;
				break;
			case MTYPE_URL:
				ackType = ACKTYPE_URL;
				break;
			case MTYPE_CONTACTS:
				ackType = ACKTYPE_CONTACTS;
				break;
			case MTYPE_SCRIPT_NOTIFY:
				{
					char *szMsg;

					szMsg = (char*)_alloca(dwDataLength + 1);
					if (dwDataLength > 0)
						memcpy(szMsg, buf, dwDataLength);
					szMsg[dwDataLength] = '\0';

					handleXtrazNotifyResponse(dc->dwRemoteUin, dc->hContact, wCookie, szMsg, dwDataLength);
				}
				break;

			default:
				NetLog_Direct("Skipped packet from direct connection");
				break;
			}

			if (ackType != -1)
			{ // was a good ack to broadcast ?
				BroadcastAck(dc->hContact, ackType, ACKRESULT_SUCCESS, (HANDLE)wCookie, 0);
			}
			// Release cookie
			ReleaseCookie(wCookie);
		}
	}
	else
		NetLog_Direct("Unsupported plugin message type %s", typeId);
}
