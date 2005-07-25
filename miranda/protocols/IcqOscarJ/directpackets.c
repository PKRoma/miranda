// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001,2002 Jon Keating, Richard Hughes
// Copyright � 2002,2003,2004 Martin �berg, Sam Kothari, Robert Rainwater
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


extern HANDLE hsmsgrequest;
extern CRITICAL_SECTION modeMsgsMutex;
extern WORD wListenPort;


void packDirectMsgHeader(icq_packet* packet, WORD wDataLen, WORD wCommand, DWORD dwCookie, BYTE bMsgType, BYTE bMsgFlags, WORD wX1, WORD wX2)
{
	directPacketInit(packet, 29 + wDataLen);
	packByte(packet, 2);	      /* channel */
	packLEDWord(packet, 0);     /* space for crypto */
	packLEWord(packet, wCommand);
	packLEWord(packet, 14);		  /* unknown */
	packLEWord(packet, (WORD)dwCookie);
	packLEDWord(packet, 0);	    /* unknown */
	packLEDWord(packet, 0);	    /* unknown */
	packLEDWord(packet, 0);	    /* unknown */
	packByte(packet, bMsgType);
	packByte(packet, bMsgFlags);
	packLEWord(packet, wX1);	  /* unknown. Is 1 for getawaymsg, 0 otherwise */
  packLEWord(packet, wX2); // this is probably priority
}


void icq_sendDirectMsgAck(directconnect* dc, WORD wCookie, BYTE bMsgType, BYTE bMsgFlags, char* szCap)
{
  icq_packet packet;

  packDirectMsgHeader(&packet, (WORD)(bMsgType==MTYPE_PLAIN ? (szCap ? 53 : 11) : 3), DIRECT_ACK, wCookie, bMsgType, bMsgFlags, 0, 0);
  packLEWord(&packet, 1);	 /* empty message */
  packByte(&packet, 0);    /* message */
  if (bMsgType == MTYPE_PLAIN)
  {
    packMsgColorInfo(&packet);

    if (szCap)
    {
      packLEDWord(&packet, 0x26);     /* CLSID length */
      packBuffer(&packet, szCap, 0x26); /* GUID */
    }
  }
  EncryptDirectPacket(dc, &packet);
  sendDirectPacket(dc->hConnection, &packet);

  NetLog_Direct("Sent acknowledgement thru direct connection");
}


DWORD icq_sendGetAwayMsgDirect(HANDLE hContact, int type)
{
  icq_packet packet;
	DWORD dwCookie;
	message_cookie_data *pCookieData = NULL;

	pCookieData = malloc(sizeof(message_cookie_data));
	dwCookie = AllocateCookie(0, ICQGetContactSettingDword(hContact, UNIQUEIDSETTING, 0), (void*)pCookieData);
  pCookieData->bMessageType = MTYPE_AUTOAWAY;
  pCookieData->nAckType = (BYTE)type;

  packDirectMsgHeader(&packet, 3, DIRECT_MESSAGE, dwCookie, (BYTE)type, 3, 1, 0);
  packLEWord(&packet, 1);	    /* length of message */
  packByte(&packet, 0);       /* message */

  if (SendDirectMessage(hContact, &packet))
    return dwCookie; // Success
  else 
    return 0;
}


void icq_sendAwayMsgReplyDirect(directconnect* dc, WORD wCookie, BYTE msgType, const char** szMsg)
{
	icq_packet packet;
	WORD wMsgLen;


	if (validateStatusMessageRequest(dc->hContact, msgType))
	{
		NotifyEventHooks(hsmsgrequest, (WPARAM)msgType, (LPARAM)dc->dwRemoteUin);
		
		EnterCriticalSection(&modeMsgsMutex);

		if (*szMsg != NULL)
		{
			wMsgLen = strlen(*szMsg);
			packDirectMsgHeader(&packet, (WORD)(3 + wMsgLen), DIRECT_ACK, wCookie, msgType, 3, 0, 0);
			packLEWord(&packet, (WORD)(wMsgLen + 1));
			packBuffer(&packet, *szMsg, (WORD)(wMsgLen + 1));
			EncryptDirectPacket(dc, &packet);

			sendDirectPacket(dc->hConnection, &packet);
		}

		LeaveCriticalSection(&modeMsgsMutex);
	}
}


void icq_sendFileAcceptDirect(HANDLE hContact, filetransfer* ft)
{ // v7 packet
	icq_packet packet;

	packDirectMsgHeader(&packet, 18, DIRECT_ACK, ft->dwCookie, MTYPE_FILEREQ, 0, 0, 0);
	packLEWord(&packet, 1);	  // description
	packByte(&packet, 0);
	packWord(&packet, wListenPort);
	packLEWord(&packet, 0);
	packLEWord(&packet, 1);	  // filename
	packByte(&packet, 0);     // TODO: really send filename
	packLEDWord(&packet, ft->dwTotalSize);  // file size 
	packLEDWord(&packet, wListenPort);		// FIXME: ideally we want to open a new port for this

	SendDirectMessage(hContact, &packet);

  NetLog_Direct("Sent file accept direct, port %u", wListenPort);
}


void icq_sendFileDenyDirect(HANDLE hContact, filetransfer* ft, char *szReason)
{ // v7 packet
	icq_packet packet;

	packDirectMsgHeader(&packet, (WORD)(18+strlennull(szReason)), DIRECT_ACK, ft->dwCookie, MTYPE_FILEREQ, 0, 1, 0);
	packLEWord(&packet, (WORD)(1+strlennull(szReason)));  // description
  if (szReason) packBuffer(&packet, szReason, (WORD)strlen(szReason));
	packByte(&packet, 0);
	packWord(&packet, 0);
	packLEWord(&packet, 0);
	packLEWord(&packet, 1);   // filename
	packByte(&packet, 0);     // TODO: really send filename
	packLEDWord(&packet, 0);  // file size 
	packLEDWord(&packet, 0);	

	SendDirectMessage(hContact, &packet);

  NetLog_Direct("Sent file deny direct.");
}


int icq_sendFileSendDirectv7(DWORD dwUin, HANDLE hContact, WORD wCookie, char* pszFiles, char* pszDescription, DWORD dwTotalSize)
{
	icq_packet packet;

	packDirectMsgHeader(&packet, (WORD)(18 + strlen(pszDescription) + strlen(pszFiles)), DIRECT_MESSAGE, wCookie, MTYPE_FILEREQ, 0, 0, 0);
	packLEWord(&packet, (WORD)(strlen(pszDescription) + 1));
	packBuffer(&packet, pszDescription, (WORD)(strlen(pszDescription) + 1));
	packLEDWord(&packet, 0);	 // listen port
	packLEWord(&packet, (WORD)(strlen(pszFiles) + 1));
	packBuffer(&packet, pszFiles, (WORD)(strlen(pszFiles) + 1));
	packLEDWord(&packet, dwTotalSize);
	packLEDWord(&packet, 0);		// listen port (again)

	NetLog_Direct("Sending v%u file transfer request direct", 7);

	return SendDirectMessage(hContact, &packet);
}


int icq_sendFileSendDirectv8(DWORD dwUin, HANDLE hContact, WORD wCookie, char *pszFiles, char *szDescription, DWORD dwTotalSize)
{
  icq_packet packet;

  packDirectMsgHeader(&packet, (WORD)(0x2E + 22 + strlen(szDescription) + strlen(pszFiles)+1), DIRECT_MESSAGE, wCookie, MTYPE_PLUGIN, 0, 0, 0);
  packLEWord(&packet, 1); // message
  packByte(&packet, 0);
  packPluginTypeId(&packet, MTYPE_FILEREQ);

	packLEDWord(&packet, (WORD)(18 + strlen(szDescription) + strlen(pszFiles)+1)); // Remaining length
	packLEDWord(&packet, (WORD)(strlen(szDescription)));          // Description
	packBuffer(&packet, szDescription, (WORD)(strlen(szDescription)));
	packWord(&packet, 0x8c82); // Unknown (port?), seen 0x80F6
	packWord(&packet, 0x0222); // Unknown, seen 0x2e01
	packLEWord(&packet, (WORD)(strlen(pszFiles)+1));
	packBuffer(&packet, pszFiles, (WORD)(strlen(pszFiles)+1));
	packLEDWord(&packet, dwTotalSize);
	packLEDWord(&packet, 0x0008c82); // Unknown, (seen 0xf680 ~33000)

  NetLog_Direct("Sending v%u file transfer request direct", 8);

  return SendDirectMessage(hContact, &packet);
}


DWORD icq_SendDirectMessage(DWORD dwUin, HANDLE hContact, const char *szMessage, int nBodyLength, WORD wPriority, message_cookie_data *pCookieData, char *szCap)
{
	icq_packet packet;
	DWORD dwCookie;


	dwCookie = AllocateCookie(0, dwUin, (void*)pCookieData);

	// Pack the standard header
  packDirectMsgHeader(&packet, (WORD)(nBodyLength + (szCap ? 53:11)), DIRECT_MESSAGE, dwCookie, pCookieData->bMessageType, 0, 0, 0);

	packLEWord(&packet, (WORD)(nBodyLength+1));	           // Length of message
	packBuffer(&packet, szMessage, (WORD)(nBodyLength+1)); // Message
  packMsgColorInfo(&packet);
  if (szCap)
  {
    packLEDWord(&packet, 0x00000026);                    // length of GUID
    packBuffer(&packet, szCap, 0x26);                    // UTF-8 GUID
  }

  if (SendDirectMessage(hContact, &packet))
    return dwCookie; // Success
  else
  {
    FreeCookie(dwCookie); // release cookie

    return 0; // Failure
  }
}


void icq_sendXtrazRequestDirect(DWORD dwUin, HANDLE hContact, DWORD dwCookie, char* szBody, int nBodyLen, WORD wType)
{
  icq_packet packet;

  packDirectMsgHeader(&packet, (WORD)(11 + getPluginTypeIdLen(wType) + nBodyLen), DIRECT_MESSAGE, dwCookie, MTYPE_PLUGIN, 0, 0, 1);
  packLEWord(&packet, 1); // message
  packByte(&packet, 0);
  packPluginTypeId(&packet, wType);

  packLEDWord(&packet, nBodyLen + 4);
  packLEDWord(&packet, nBodyLen);
  packBuffer(&packet, szBody, (WORD)nBodyLen);

	SendDirectMessage(hContact, &packet);
}


void icq_sendXtrazResponseDirect(DWORD dwUin, HANDLE hContact, WORD wCookie, char* szBody, int nBodyLen, WORD wType)
{
	icq_packet packet;

	packDirectMsgHeader(&packet, (WORD)(getPluginTypeIdLen(wType) + 11 + nBodyLen), DIRECT_ACK, wCookie, MTYPE_PLUGIN, 0, 0, 0);
	//
	packLEWord(&packet, 1); // Message len
	packByte(&packet, 0);   // Message (unused)

  packPluginTypeId(&packet, wType);

  packLEDWord(&packet, nBodyLen + 4);
  packLEDWord(&packet, nBodyLen);
  packBuffer(&packet, szBody, (WORD)nBodyLen);

	SendDirectMessage(hContact, &packet);
}
