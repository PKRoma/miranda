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
//  Handlers for Family 4 ICBM Messages
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


icq_mode_messages modeMsgs;
CRITICAL_SECTION modeMsgsMutex;

static void handleRecvServMsg(unsigned char *buf, WORD wLen, WORD wFlags, DWORD dwRef);
static void handleRecvServMsgType1(unsigned char *buf, WORD wLen, DWORD dwUin, char *szUID, DWORD dwTS1, DWORD dwTS2);
static void handleRecvServMsgType2(unsigned char *buf, WORD wLen, DWORD dwUin, DWORD dwTS1, DWORD dwTS2);
static void handleRecvServMsgType4(unsigned char *buf, WORD wLen, DWORD dwUin, DWORD dwTS1, DWORD dwTS2);
static void handleRecvServMsgError(unsigned char *buf, WORD wLen, WORD wFlags, DWORD dwRef);
static void handleRecvMsgResponse(unsigned char *buf, WORD wLen, WORD wFlags, DWORD dwRef);
static void handleServerAck(unsigned char *buf, WORD wLen, WORD wFlags, DWORD dwRef);
static void handleTypingNotification(unsigned char *buf, WORD wLen, WORD wFlags, DWORD dwRef);
static void handleMissedMsg(unsigned char *buf, WORD wLen, WORD wFlags, DWORD dwRef);
static void parseTLV2711(DWORD dwUin, HANDLE hContact, DWORD dwID1, DWORD dwID2, WORD wAckType, oscar_tlv* tlv);
static void parseServerGreeting(BYTE* pDataBuf, WORD wLen, WORD wMsgLen, DWORD dwUin, BYTE bFlags, WORD wStatus, WORD wCookie, WORD wAckType, DWORD dwID1, DWORD dwID2);



void handleMsgFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader)
{
  switch (pSnacHeader->wSubtype)
  {

  case ICQ_MSG_SRV_ERROR: // SNAC(4, 0x01)
    handleRecvServMsgError(pBuffer, wBufferLength, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;

  case ICQ_MSG_SRV_RECV: // SNAC(4, 0x07)
    handleRecvServMsg(pBuffer, wBufferLength, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;

  case ICQ_MSG_SRV_MISSED_MESSAGE: // SNAC(4,A)
    handleMissedMsg(pBuffer, wBufferLength, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;

  case ICQ_MSG_RESPONSE: // SNAC(4, 0x0B)
    handleRecvMsgResponse(pBuffer, wBufferLength, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;

  case ICQ_MSG_SRV_ACK: // SNAC(4, 0x0C) Server acknowledgements
    handleServerAck(pBuffer, wBufferLength, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;

  case ICQ_MSG_MTN: // SNAC (4,0x14) Typing notifications
    handleTypingNotification(pBuffer, wBufferLength, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;

    // Stuff we don't care about
  case ICQ_MSG_SRV_REPLYICBM: // SNAC(4,x05) - SRV_REPLYICBM
    break;

  default:
    NetLog_Server("Warning: Ignoring SNAC(x%02x,x%02x) - Unknown SNAC (Flags: %u, Ref: %u)", ICQ_MSG_FAMILY, pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;
  }
}



static void handleRecvServMsg(unsigned char *buf, WORD wLen, WORD wFlags, DWORD dwRef)
{
  DWORD dwUin;
  DWORD dwID1;
  DWORD dwID2;
  WORD wTLVCount;
  WORD wMessageFormat;
  uid_str szUID;


  // These two values are some kind of reference, we need to save
  // them to send file request responses for example
  unpackLEDWord(&buf, &dwID1); // TODO: msg cookies should be main
  wLen -= 4;
  unpackLEDWord(&buf, &dwID2);
  wLen -= 4;

  // The message type used:
  unpackWord(&buf, &wMessageFormat); //  0x0001: Simple message format
  wLen -= 2;                         //  0x0002: Advanced message format
                                     //  0x0004: 'New' message format
  // Sender UIN
  if (wMessageFormat == 1)
  {
    if (!unpackUID(&buf, &wLen, &dwUin, &szUID)) return;
  }
  else 
  {
    szUID[0] = '\0';
    if (!unpackUID(&buf, &wLen, &dwUin, NULL)) return;
  }

  if (dwUin && IsOnSpammerList(dwUin))
  {
    NetLog_Server("Ignored Message from known Spammer");
    return;
  }

  // Warning level?
  buf += 2;
  wLen -= 2;

  // Number of following TLVs, until msg-format dependant TLVs
  unpackWord(&buf, &wTLVCount);
  wLen -= 2;
  if (wTLVCount > 0)
  {
    // Save current buffer pointer so we can calculate
    // how much data we have left after the chain read.
    oscar_tlv_chain* chain = NULL;
    unsigned char* pBufStart = buf;

    chain = readIntoTLVChain(&buf, wLen, wTLVCount);

    // This chain contains info that is filled in by the server.
    // TLV(1): unknown
    // TLV(2): date: on since
    // TLV(3): date: on since
    // TLV(4): unknown, usually 0000. Not in file-req or auto-msg-req
    // TLV(6): sender's status
    // TLV(F): a time in seconds, unknown

    disposeChain(&chain);

    // Update wLen
    wLen = wLen - (buf - pBufStart);
  }


  // This is where the format specific data begins

  switch (wMessageFormat)
  {

  case 1: // Simple message format
    handleRecvServMsgType1(buf, wLen, dwUin, szUID, dwID1, dwID2);
    break;

  case 2:  // Encapsulated messages
    handleRecvServMsgType2(buf, wLen, dwUin, dwID1, dwID2);
    break;

  case 4: // Typed messages
    handleRecvServMsgType4(buf, wLen, dwUin, dwID1, dwID2);
    break;

  default:
    if (dwUin)
      NetLog_Server("Unknown format message thru server - Flags %u, Ref %u, Type: %u, UIN: %u", wFlags, dwRef, wMessageFormat, dwUin);
    else
      NetLog_Server("Unknown format message thru server - Flags %u, Ref %u, Type: %u, UID: %s", wFlags, dwRef, wMessageFormat, szUID);
    break;

  }
}



static void handleRecvServMsgType1(unsigned char *buf, WORD wLen, DWORD dwUin, char *szUID, DWORD dwTS1, DWORD dwTS2)
{
  WORD wTLVType;
  WORD wTLVLen;
  BYTE* pDataBuf;


  // Unpack the first TLV
  unpackTLV(&buf, &wTLVType, &wTLVLen, &pDataBuf);
  if (dwUin)
    NetLog_Server("Message (format 1) - UIN: %u, FirstTLV: %u", dwUin, wTLVType);
  else
    NetLog_Server("Message (format 1) - UID: %s, FirstTLV: %u", szUID, wTLVType);

  // It must be TLV(2)
  if (wTLVType == 2)
  {
    oscar_tlv_chain* pChain;

    pChain = readIntoTLVChain(&pDataBuf, wTLVLen, 0);

    // TLV(2) contains yet another TLV chain with the following TLVs:
    //   TLV(1281): Capability
    //   TLV(257):  This TLV contains the actual message

    if (pChain)
    {
      oscar_tlv* pMessageTLV;
      oscar_tlv* pCapabilityTLV;

      // Find the capability TLV
      pCapabilityTLV = getTLV(pChain, 0x0501, 1);
      if (pCapabilityTLV && (pCapabilityTLV->wLen > 0))
      {
        WORD wDataLen;
        BYTE *pDataBuf;

        wDataLen = pCapabilityTLV->wLen;
        pDataBuf = pCapabilityTLV->pData;

        if (wDataLen > 0)
        {
          NetLog_Server("Message (format 1) - Message has %d caps.", wDataLen);
        }
      }
      else
      {
        NetLog_Server("Message (format 1) - No message cap.");
      }

      // Find the message TLV
      pMessageTLV = getTLV(pChain, 0x0101, 1);
      if (pMessageTLV)
      {
        if (pMessageTLV->wLen > 4)
        {
          WORD wMsgLen;
          BYTE* pMsgBuf;
          WORD wEncoding;
          WORD wCodePage;
          char* szMsg = NULL;
          HANDLE hContact;
          CCSDATA ccs;
          PROTORECVEVENT pre;
          int bAdded;

          // The message begins with a encoding specification
          // The first WORD is believed to have the following meaning:
          //  0x00: US-ASCII
          //  0x02: Unicode UCS-2 Big Endian encoding
          //  0x03: local 8bit encoding
          pMsgBuf = pMessageTLV->pData;
          unpackWord(&pMsgBuf, &wEncoding);
          unpackWord(&pMsgBuf, &wCodePage);

          wMsgLen = pMessageTLV->wLen - 4;
          NetLog_Server("Message (format 1) - Encoding is 0x%X, page is 0x%X", wEncoding, wCodePage);

          pre.flags = 0;

          if (dwUin)
            hContact = HContactFromUIN(dwUin, &bAdded);
          else
            hContact = HContactFromUID(szUID, &bAdded);

          switch (wEncoding)
          {

          case 2:
            {
              WCHAR* usMsg;
              int nStrSize;

              usMsg = malloc(wMsgLen + 2);
              unpackWideString(&pMsgBuf, usMsg, wMsgLen);
              usMsg[wMsgLen/2] = 0;

              if (!dwUin)
              {
                char *utf = make_utf8_string(usMsg);
                int nUtfLen = strlennull(utf);
                char *tmp;
                int i,j;
                BOOL tag = FALSE;

                SAFE_FREE(&usMsg);
                tmp = (char*)malloc(nUtfLen + 1);
                for (i=0,j=0;i<nUtfLen + 1;i++)
                {
                  if (!tag && utf[i] == '<') 
                  {
                    tag = TRUE;
                  }
                  else if (tag && utf[i] == '>')
                  {
                    tag = FALSE;
                  }
                  else if (!tag)
                  {
                    tmp[j] = utf[i];
                    j++;
                  }
                  tmp[j] = '\0';
                }
                SAFE_FREE(&utf);
                utf = DemangleXml(tmp, strlennull(tmp));
                SAFE_FREE(&tmp);
                usMsg = make_unicode_string(utf);
                SAFE_FREE(&utf);
                SetContactCapabilities(hContact, CAPF_UTF);
              }

              nStrSize = WideCharToMultiByte(CP_ACP, 0, usMsg, wMsgLen / sizeof(WCHAR), szMsg, 0, NULL, NULL);
              szMsg = calloc(nStrSize+1, sizeof(wchar_t)+1);
              szMsg[nStrSize - 1] = 0; // ?? redundant as long as calloc is used
              WideCharToMultiByte(CP_ACP, 0, usMsg, wMsgLen / sizeof(WCHAR), szMsg, nStrSize, NULL, NULL);
              nStrSize = strlennull(szMsg); // this is necessary, sometimes it was bad
              memcpy(szMsg+nStrSize+1, usMsg, wMsgLen);

              pre.flags = PREF_UNICODE;

              SAFE_FREE(&usMsg);

              break;
            }

          case 0:
          case 3:
          default:
            {
              // Copy the message text into a new proper string.
              szMsg = (char *)malloc(wMsgLen + 1);
              memcpy(szMsg, pMsgBuf, wMsgLen);
              szMsg[wMsgLen] = '\0';
              if (!dwUin)
              {
                char *tmp;
                int i,j;
                BOOL tag = FALSE;

                tmp = (char*)malloc(wMsgLen + 1);
                for (i=0,j=0;i<wMsgLen + 1;i++)
                {
                  if (!tag && szMsg[i] == '<') 
                  {
                    tag = TRUE;
                  }
                  else if (tag && szMsg[i] == '>')
                  {
                    tag = FALSE;
                  }
                  else if (!tag)
                  {
                    tmp[j] = szMsg[i];
                    j++;
                  }
                  tmp[j] = '\0';
                }
                SAFE_FREE(&szMsg);
                szMsg = DemangleXml(tmp, strlennull(tmp));
                SAFE_FREE(&tmp);
              }

              break;
            }
          }

          if (!pre.flags && !IsUSASCII(szMsg, strlennull(szMsg)))
          { // message is Ansi and contains national characters, create Unicode part by codepage
            WORD wCP = ICQGetContactSettingWord(hContact, "CodePage", gwAnsiCodepage);

            if (wCP != CP_ACP)
            {
              wchar_t* usMsg;

              usMsg = malloc((strlennull(szMsg)+2)*(sizeof(wchar_t)+1));
              memcpy((char*)usMsg, szMsg, strlennull(szMsg)+1);
              MultiByteToWideChar(wCP, 0, szMsg, strlennull(szMsg), (wchar_t*)((char*)usMsg + strlennull(szMsg) + 1), strlennull(szMsg));
              *(wchar_t*)((char*)usMsg + 1 + strlennull(szMsg)*(1 + sizeof(wchar_t))) = '\0'; // trailing zeros
              SAFE_FREE(&szMsg);
              szMsg = (char*)usMsg;
              pre.flags = PREF_UNICODE;
            }
          }
          // Create and send the message event
          ccs.szProtoService = PSR_MESSAGE;
          ccs.hContact = hContact;
          ccs.wParam = 0;
          ccs.lParam = (LPARAM)&pre;
          pre.timestamp = time(NULL);
          pre.szMessage = (char *)szMsg;
          pre.lParam = 0;
          CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);

          SAFE_FREE(&szMsg);

          NetLog_Server("Message (format 1) received");

          // Save tick value
          ICQWriteContactSettingDword(ccs.hContact, "TickTS", time(NULL) - (dwTS1/1000));
        }
        else
        {
          NetLog_Server("Ignoring empty message (format 1)");
        }
      }
      else
      {
        NetLog_Server("Failed to find TLV(1281) message (format 1)");
      }

      // Free the chain memory
      disposeChain(&pChain);
    }
    else
    {
      NetLog_Server("Failed to read TLV chain in message (format 1)");
    }
  }
  else
  {
    NetLog_Server("Unsupported TLV (%u) in message (format 1)", wTLVType);
  }
}



static void handleRecvServMsgType2(unsigned char *buf, WORD wLen, DWORD dwUin, DWORD dwID1, DWORD dwID2)
{
  WORD wTLVType;
  WORD wTLVLen;
  char* pDataBuf = NULL;
  char* pTLV5 = NULL;

  // Unpack the first TLV
  unpackTLV(&buf, &wTLVType, &wTLVLen, &pTLV5);
  if (wTLVType == 4 && wTLVLen == 0)
  { // IM2 sends empty TLV(4) first - OMG!
    SAFE_FREE(&pTLV5);
    unpackTLV(&buf, &wTLVType, &wTLVLen, &pTLV5);
  }
  pDataBuf = pTLV5;
  NetLog_Server("Message (format 2) through server - UIN: %u, FirstTLV: %u", dwUin, wTLVType);

  // It must be TLV(5)
  if (wTLVType == 5)
  {
    WORD wCommand;
    oscar_tlv_chain* chain;
    oscar_tlv* tlv;
    DWORD dwIP;
    DWORD dwExternalIP;
    WORD wPort;
    WORD wAckType;
    DWORD q1,q2,q3,q4;

    unpackWord(&pDataBuf, &wCommand);
    wTLVLen -= 2;                             // Command 0x0000 - Normal message/file send request
#ifdef _DEBUG                                 //         0x0001 - Abort request
    NetLog_Server("Command is %u", wCommand); //         0x0002 - Acknowledge request
#endif

    if (wCommand == 1)
    {
      NetLog_Server("Cannot handle abort messages yet... :(");
      SAFE_FREE(&pTLV5);
      return;
    }

    // Some stuff we don't use
    pDataBuf += 8;  // dwID1 and dwID2 again
    wTLVLen -= 8;
    unpackDWord(&pDataBuf, &q1);
    unpackDWord(&pDataBuf, &q2);
    unpackDWord(&pDataBuf, &q3);
    unpackDWord(&pDataBuf, &q4); // Capability (CAP_SRV_RELAY)
    wTLVLen -= 16;

    if (CompareGUIDs(q1,q2,q3,q4, MCAP_TLV2711_FMT))
    {
      // We need at least 4 bytes to read a chain
      if (wTLVLen > 4)
      {
        int bAdded;
        HANDLE hContact = HContactFromUIN(dwUin, &bAdded);

        // This TLV chain may contain the following TLVs:
        // TLV(A): Acktype 0x0000 - normal message
        //                 0x0001 - file request / abort request
        //                 0x0002 - file ack
        // TLV(F): Unknown
        // TLV(3): External IP
        // TLV(5): DC port (not to use for filetransfers)
        // TLV(0x2711): The next message level

        chain = readIntoTLVChain(&pDataBuf, wTLVLen, 0);

        wAckType = getWordFromChain(chain, 0x0A, 1);

        // Update the saved DC info (if contact already exists)
        if (hContact != INVALID_HANDLE_VALUE)
        {
          if (dwExternalIP = getDWordFromChain(chain, 0x03, 1))
            ICQWriteContactSettingDword(hContact, "RealIP", dwExternalIP);
          if (dwIP = getDWordFromChain(chain, 0x04, 1))
            ICQWriteContactSettingDword(hContact, "IP", dwIP);
          if (wPort = getWordFromChain(chain, 0x05, 1))
            ICQWriteContactSettingWord(hContact, "UserPort", wPort);

          // Save tick value
          ICQWriteContactSettingDword(hContact, "TickTS", time(NULL) - (dwID1/1000));
        }

        // Parse the next message level
        if (tlv = getTLV(chain, 0x2711, 1))
        {
          parseTLV2711(dwUin, hContact, dwID1, dwID2, wAckType, tlv);
        }
        else
        {
          NetLog_Server("Warning, no 0x2711 TLV in message (format 2)");
        }
        // Clean up
        disposeChain(&chain);
      }
      else
      {
        NetLog_Server("Warning, empty message (format 2)");
      }
    }
    else if (CompareGUIDs(q1,q2,q3,q4,MCAP_REVERSE_REQ))
    { // Handle reverse DC request
      // We need at least 4 bytes to read a chain
      if (wTLVLen > 4)
      {
        chain = readIntoTLVChain(&pDataBuf, wTLVLen, 0);

        wAckType = getWordFromChain(chain, 0x0A, 1);
        // Parse the next message level
        if (tlv = getTLV(chain, 0x2711, 1))
        {
          if (tlv->wLen == 0x1B)
          {
            char* buf=tlv->pData;
            DWORD dwUin, dwIp, dwPort, dwId;
            WORD wVersion;
            BYTE bMode;
            HANDLE hContact;
            int bAdded;

            unpackLEDWord(&buf, &dwUin);

            hContact = HContactFromUIN(dwUin, &bAdded);
            if (hContact == INVALID_HANDLE_VALUE)
            {
              NetLog_Server("Error: Reverse Connect Request from unknown contact %u", dwUin);
            }
            else
            {
              unpackDWord(&buf, &dwIp); 
              unpackLEDWord(&buf, &dwPort);
              unpackByte(&buf, &bMode);
              buf += 4; // unknown
              if (dwPort)
                buf += 4;  // port, again?
              else
                unpackLEDWord(&buf, &dwPort);
              unpackLEWord(&buf, &wVersion);

              ICQWriteContactSettingDword(hContact, "IP", dwIp);
              ICQWriteContactSettingWord(hContact,  "UserPort", (WORD)dwPort);
              ICQWriteContactSettingByte(hContact,  "DCType", bMode);
              ICQWriteContactSettingWord(hContact,  "Version", wVersion);
              if (wVersion>6)
              {
                unpackLEDWord(&buf, &dwId);

                OpenDirectConnection(hContact, DIRECTCONN_REVERSE, (void*)dwId);
              }
              else
                NetLog_Server("Warning: Unsupported direct protocol version in Reverse Connect Request");
            }
          }
          else
          {
            NetLog_Server("Malformed Reverse Connect Request");
          }
        }
        else
        {
          NetLog_Server("Warning, no 0x2711 TLV in message (format 2)");
        }
        // Clean up
        disposeChain(&chain);
      }
      else
      {
        NetLog_Server("Warning, empty message (format 2)");
      }
    }
    else
    {
      NetLog_Server("Unknown Message Format Capability");
    }
  }
  else
  {
    NetLog_Server("Unsupported TLV (%u) in message (format 2)", wTLVType);
  }

  SAFE_FREE(&pTLV5);
}



static void parseTLV2711(DWORD dwUin, HANDLE hContact, DWORD dwID1, DWORD dwID2, WORD wAckType, oscar_tlv* tlv)
{
  BYTE* pDataBuf;
  WORD wId;
  WORD wLen;

  pDataBuf = tlv->pData;
  wLen = tlv->wLen;

  unpackLEWord(&pDataBuf, &wId); // Incorrect identification, but working
  wLen -= 2;

  // Only 0x1B are real messages
  if (wId == 0x001B)
  {
    WORD wVersion;
    WORD wCookie;
    WORD wMsgLen;
    BYTE bMsgType;
    BYTE bFlags;
    DWORD dwGuid1,dwGuid2,dwGuid3,dwGuid4;

    unpackLEWord(&pDataBuf, &wVersion);
    wLen -= 2;

    if (hContact != INVALID_HANDLE_VALUE)
      ICQWriteContactSettingWord(hContact, "Version", wVersion);

    unpackDWord(&pDataBuf, &dwGuid1); // plugin type GUID
    unpackDWord(&pDataBuf, &dwGuid2);
    unpackDWord(&pDataBuf, &dwGuid3);
    unpackDWord(&pDataBuf, &dwGuid4);
    wLen -= 16;

    // Skip lots of unused stuff
    pDataBuf += 9; 
    wLen -= 9;

    unpackLEWord(&pDataBuf, &wId); 
    wLen -= 2;

    unpackLEWord(&pDataBuf, &wCookie);
    wLen -= 2;

    if (CompareGUIDs(dwGuid1, dwGuid2, dwGuid3, dwGuid4, PSIG_MESSAGE))
    { // is this a normal message ?
      WORD wPritority;
      WORD wStatus;

      pDataBuf += 12;  /* all zeroes */
      wLen -= 12;
      unpackByte(&pDataBuf, &bMsgType);
      wLen -= 1;
      unpackByte(&pDataBuf, &bFlags);
      wLen -= 1;

      // Status
      unpackLEWord(&pDataBuf, &wStatus);
      wLen -= 2;

      // Priority
      unpackLEWord(&pDataBuf, &wPritority);
      wLen -= 2;
      NetLog_Server("Priority: %u", wPritority);

      // Message
      unpackLEWord(&pDataBuf, &wMsgLen);
      wLen -= 2;

      // HANDLERS
      switch (bMsgType)
      {
        // File messages, handled by the file module
      case MTYPE_FILEREQ:
        {
          char* szMsg;


          szMsg = (char *)malloc(wMsgLen + 1);
          memcpy(szMsg, pDataBuf, wMsgLen);
          szMsg[wMsgLen] = '\0';
          pDataBuf += wMsgLen;
          wLen -= wMsgLen;

          if (wAckType == 0 || wAckType == 1)
          {
            // File requests 7
            handleFileRequest(pDataBuf, wLen, dwUin, wCookie, dwID1, dwID2, szMsg, 7, FALSE);
          }
          else if (wAckType == 2)
          {
            // File reply 7
            handleFileAck(pDataBuf, wLen, dwUin, wCookie, wStatus, szMsg);
          }
          else
          {
            NetLog_Server("Ignored strange file message");
          }

          SAFE_FREE(&szMsg);
          break;
        }

        // Chat messages, handled by the chat module
      case MTYPE_CHAT:
        {
          break;
        }

        // Plugin messages, need further parsing
      case MTYPE_PLUGIN:
        {
          parseServerGreeting(pDataBuf, wLen, wMsgLen, dwUin, bFlags, wStatus, wCookie, wAckType, dwID1, dwID2);
          break;
        }

        // Everything else
      default:
        {
          // Only ack non-status message requests
          // The other will be acked later
          if (bMsgType < 0x80)
            icq_sendAdvancedMsgAck(dwUin, dwID1, dwID2, wCookie, bMsgType, bFlags);
          handleMessageTypes(dwUin, time(NULL), dwID1, dwID2, wCookie, bMsgType, bFlags, wAckType, tlv->wLen - 53, wMsgLen, pDataBuf, FALSE);
          break;
        }
      }
    }
    else if (CompareGUIDs(dwGuid1, dwGuid2, dwGuid3, dwGuid4, PSIG_INFO_PLUGIN))
    { // info manager plugin
      BYTE bLevel;

      pDataBuf += 16;  /* unused stuff */
      wLen -= 16;
      unpackByte(&pDataBuf, &bMsgType);
      wLen -= 1;

      pDataBuf += 3; // unknown
      wLen -= 3;
      unpackByte(&pDataBuf, &bLevel);
      if (bLevel != 0 || wLen < 16)
      {
        NetLog_Server("Invalid Info Manager Plugin message from %u", dwUin);
        return;
      }
      unpackDWord(&pDataBuf, &dwGuid1); // plugin request GUID
      unpackDWord(&pDataBuf, &dwGuid2);
      unpackDWord(&pDataBuf, &dwGuid3);
      unpackDWord(&pDataBuf, &dwGuid4);
      wLen -= 16;

      if (CompareGUIDs(dwGuid1, dwGuid2, dwGuid3, dwGuid4, PMSG_QUERY_INFO))
      {
        NetLog_Server("User %u requests our %s plugin list. NOT SUPPORTED YET", dwUin, "info");
      }
      else
        NetLog_Server("Unknown Info Manager message from %u", dwUin);
    }
    else if (CompareGUIDs(dwGuid1, dwGuid2, dwGuid3, dwGuid4, PSIG_STATUS_PLUGIN))
    { // status manager plugin
      BYTE bLevel;

      pDataBuf += 16;  /* unused stuff */
      wLen -= 16;
      unpackByte(&pDataBuf, &bMsgType);
      wLen -= 1;

      pDataBuf += 3; // unknown
      wLen -= 3;
      unpackByte(&pDataBuf, &bLevel);
      if (bLevel != 0 || wLen < 16)
      {
        NetLog_Server("Invalid Status Manager Plugin message from %u", dwUin);
        return;
      }
      unpackDWord(&pDataBuf, &dwGuid1); // plugin request GUID
      unpackDWord(&pDataBuf, &dwGuid2);
      unpackDWord(&pDataBuf, &dwGuid3);
      unpackDWord(&pDataBuf, &dwGuid4);
      wLen -= 16;

      if (CompareGUIDs(dwGuid1, dwGuid2, dwGuid3, dwGuid4, PMSG_QUERY_STATUS))
      {
        NetLog_Server("User %u requests our %s plugin list. NOT SUPPORTED YET", dwUin, "status");
      }
      else
        NetLog_Server("Unknown Status Manager message from %u", dwUin);
    }
    else
      NetLog_Server("Unknown signature (%08x-%08x-%08x-%08x) in message (format 2)", dwGuid1, dwGuid2, dwGuid3, dwGuid4);
  }
  else
  {
    NetLog_Server("Unknown wId1 (%u) in message (format 2)", wId);
  }
}



void parseServerGreeting(BYTE* pDataBuf, WORD wLen, WORD wMsgLen, DWORD dwUin, BYTE bFlags, WORD wStatus, WORD wCookie, WORD wAckType, DWORD dwID1, DWORD dwID2)
{
  WORD wInfoLen;
  DWORD dwPluginNameLen;
  DWORD dwLengthToEnd;
  DWORD dwDataLen;
  DWORD q1,q2,q3,q4;
  WORD qt;
  char* szPluginName;
  int typeId;

  NetLog_Server("Parsing Greeting message through server");

  pDataBuf += wMsgLen;   // Message
  wLen -= wMsgLen;

  //
  unpackLEWord(&pDataBuf, &wInfoLen);

  unpackDWord(&pDataBuf, &q1); // get data GUID & function id
  unpackDWord(&pDataBuf, &q2);
  unpackDWord(&pDataBuf, &q3);
  unpackDWord(&pDataBuf, &q4);
  unpackLEWord(&pDataBuf, &qt);
  wLen -= 20;

  unpackLEDWord(&pDataBuf, &dwPluginNameLen);
  if (dwPluginNameLen > wLen) 
  { // check for malformed plugin name
    dwPluginNameLen = wLen;
    NetLog_Server("Warning: malformed size of plugin name.");
  }
  szPluginName = (char *)malloc(dwPluginNameLen + 1);
  memcpy(szPluginName, pDataBuf, dwPluginNameLen);
  szPluginName[dwPluginNameLen] = '\0';

  pDataBuf += dwPluginNameLen + 15;

  typeId = TypeGUIDToTypeId(q1, q2, q3, q4, qt);
  if (!typeId)
    NetLog_Server("Error: Unknown type {%08x-%08x-%08x-%08x:%04x}: %s", q1,q2,q3,q4,qt, szPluginName);

  // Length of remaining data
  unpackLEDWord(&pDataBuf, &dwLengthToEnd);

  // Length of message
  unpackLEDWord(&pDataBuf, &dwDataLen);


  if (typeId == MTYPE_FILEREQ && wAckType == 2)
  {
    char* szMsg;


    NetLog_Server("This is file ack");
    szMsg = (char *)malloc(dwDataLen + 1);
    memcpy(szMsg, pDataBuf, dwDataLen);
    szMsg[dwDataLen] = '\0';
    pDataBuf += dwDataLen;
    wLen -= (WORD)dwDataLen;

    handleFileAck(pDataBuf, wLen, dwUin, wCookie, wStatus, szMsg);
    SAFE_FREE(&szMsg);
  }
  else if (typeId == MTYPE_FILEREQ && wAckType == 1)
  {
    char* szMsg;


    NetLog_Server("This is a file request");
    szMsg = (char *)malloc(dwDataLen + 1);
    memcpy(szMsg, pDataBuf, dwDataLen);
    szMsg[dwDataLen] = '\0';
    pDataBuf += dwDataLen;
    wLen -= (WORD)dwDataLen;

    handleFileRequest(pDataBuf, wLen, dwUin, wCookie, dwID1, dwID2, szMsg, 8, FALSE);
    SAFE_FREE(&szMsg);
  }
  else if (typeId == MTYPE_CHAT && wAckType == 1)
  {
    char* szMsg;

    NetLog_Server("This is a chat request");
    szMsg = (char *)malloc(dwDataLen + 1);
    memcpy(szMsg, pDataBuf, dwDataLen);
    szMsg[dwDataLen] = '\0';
    pDataBuf += dwDataLen;
    wLen -= (WORD)dwDataLen;

//    handleChatRequest(pDataBuf, wLen, dwUin, wCookie, dwID1, dwID2, szMsg, 8);
    SAFE_FREE(&szMsg);

  }
  else if (typeId)
  {
    if (typeId == MTYPE_URL || typeId == MTYPE_CONTACTS)
      icq_sendAdvancedMsgAck(dwUin, dwID1, dwID2, wCookie, (BYTE)typeId, bFlags);

    handleMessageTypes(dwUin, time(NULL), dwID1, dwID2, wCookie, typeId, bFlags, wAckType, dwLengthToEnd, (WORD)dwDataLen, pDataBuf, FALSE);
  }
  else
  {
    NetLog_Server("Unsupported plugin message type '%s'", szPluginName);
  }


  SAFE_FREE(&szPluginName);
}



static void handleRecvServMsgType4(unsigned char *buf, WORD wLen, DWORD dwUin, DWORD dwTS1, DWORD dwTS2)
{
  WORD wTLVType;
  WORD wTLVLen;
  BYTE* pDataBuf;
  DWORD dwUin2;

  // Unpack the first TLV
  unpackTLV(&buf, &wTLVType, &wTLVLen, &pDataBuf);
  NetLog_Server("Message (format 4) - UIN: %u, FirstTLV: %u", dwUin, wTLVType);

  // It must be TLV(5)
  if (wTLVType == 5)
  {
    BYTE bMsgType;
    BYTE bFlags;
    unsigned char* pmsg = pDataBuf;
    WORD wMsgLen;


    unpackLEDWord(&pmsg, &dwUin2);

    if (dwUin2 == dwUin)
    {
      unpackByte(&pmsg, &bMsgType);
      unpackByte(&pmsg, &bFlags);
      unpackLEWord(&pmsg, &wMsgLen);

      if (bMsgType == 0 && wMsgLen == 1)
      {
        NetLog_Server("User %u probably checks his ignore state.", dwUin);
      }
      else
      {
        handleMessageTypes(dwUin, time(NULL), dwTS1, dwTS2, 0, bMsgType, bFlags, 0, wTLVLen - 8, wMsgLen, pmsg, FALSE);

        NetLog_Server("TYPE4 message thru server from %d, message type %d", dwUin, bMsgType);
      }
    }
    else
    {
      NetLog_Server("Ignoring spoofed TYPE4 message thru server from %d", dwUin);
    }
  }
  else
  {
    NetLog_Server("Unsupported TLV in Type 4 message (%u)", wTLVType);
  }

  SAFE_FREE(&pDataBuf);
}



//
// Helper functions
//

int TypeGUIDToTypeId(DWORD dwGuid1, DWORD dwGuid2, DWORD dwGuid3, DWORD dwGuid4, WORD wType)
{
  int nTypeID = MTYPE_UNKNOWN;

  if (wType==MGTYPE_STANDARD_SEND)
  {
    if (CompareGUIDs(dwGuid1, dwGuid2, dwGuid3, dwGuid4, MGTYPE_WEBURL))
    {
      nTypeID = MTYPE_URL;
    }
    else if (CompareGUIDs(dwGuid1, dwGuid2, dwGuid3, dwGuid4, MGTYPE_CONTACTS))
    {
      nTypeID = MTYPE_CONTACTS;
    }
    else if (CompareGUIDs(dwGuid1, dwGuid2, dwGuid3, dwGuid4, MGTYPE_CHAT))
    {
      nTypeID = MTYPE_CHAT;
    }
    else if (CompareGUIDs(dwGuid1, dwGuid2, dwGuid3, dwGuid4, MGTYPE_FILE))
    {
      nTypeID = MTYPE_FILEREQ;
    }
    else if (CompareGUIDs(dwGuid1, dwGuid2, dwGuid3, dwGuid4, MGTYPE_GREETING_CARD))
    {
      nTypeID = MTYPE_GREETINGCARD;
    }
    else if (CompareGUIDs(dwGuid1, dwGuid2, dwGuid3, dwGuid4, MGTYPE_MESSAGE))
    {
      nTypeID = MTYPE_MESSAGE;
    }
  }
  else if (wType==MGTYPE_CONTACTS_REQUEST)
  {
    if (CompareGUIDs(dwGuid1, dwGuid2, dwGuid3, dwGuid4, MGTYPE_CONTACTS))
    {
      nTypeID = MTYPE_REQUESTCONTACTS;
    }
    else if (CompareGUIDs(dwGuid1, dwGuid2, dwGuid3, dwGuid4, MGTYPE_XTRAZ_SCRIPT))
    {
      nTypeID = MTYPE_SCRIPT_DATA;
    }
  }
  else if (CompareGUIDs(dwGuid1, dwGuid2, dwGuid3, dwGuid4, MGTYPE_XTRAZ_SCRIPT))
  {
    if (wType==MGTYPE_SCRIPT_INVITATION)
    {
      nTypeID = MTYPE_SCRIPT_INVITATION;
    }
    else if (wType==MGTYPE_SCRIPT_NOTIFY)
    {
      nTypeID = MTYPE_SCRIPT_NOTIFY;
    }
  }

  return nTypeID;
}


int getPluginTypeIdLen(int nTypeID)
{
  switch (nTypeID)
  {
  case MTYPE_SCRIPT_NOTIFY:
    return 0x51;

  case MTYPE_FILEREQ:
    return 0x2B;

  default:
    return 0;
  }
}


void packPluginTypeId(icq_packet *packet, int nTypeID)
{
  switch (nTypeID)
  {
  case MTYPE_SCRIPT_NOTIFY:
    packLEWord(packet, 0x04f);                // Length

    packGUID(packet, MGTYPE_XTRAZ_SCRIPT);    // Message Type GUID
    packLEWord(packet, MGTYPE_SCRIPT_NOTIFY); // Function ID
    packLEDWord(packet, 0x002a);              // Request type string
    packBuffer(packet, "Script Plug-in: Remote Notification Arrive", 0x002a);

    packDWord(packet, 0x00000100);            // Unknown binary stuff
    packDWord(packet, 0x00000000);
    packDWord(packet, 0x00000000);
    packWord(packet, 0x0000);
    packByte(packet, 0x00);

    break;

  case MTYPE_FILEREQ:
    packLEWord(packet, 0x029);     // Length

    packGUID(packet, MGTYPE_FILE); // Message Type GUID
    packWord(packet, 0x0000);      // Unknown
    packLEDWord(packet, 0x0004);   // Request type string
    packBuffer(packet, "File", 0x0004);

    packDWord(packet, 0x00000100); // More unknown binary stuff
    packDWord(packet, 0x00010000);
    packDWord(packet, 0x00000000);
    packWord(packet, 0x0000);
    packByte(packet, 0x00);

    break;

  default:
    return;
  }
}


static void handleSmsReceipt(unsigned char *buf, DWORD dwDataLen)
{
  DWORD dwLen;
  DWORD dwTextLen;
  char* szInfo;


  if (dwDataLen < 36)
    return;

  buf += 20;  // Unknown byte sequence
  dwDataLen -= 20;

  unpackLEDWord(&buf, &dwLen); // Length of message description
  dwDataLen -= 4;
  if (dwLen > dwDataLen)
  {
    NetLog_Server("SMS failed syntax check %d (%d, %d)", 1, dwDataLen, dwLen);
    return;
  }
  buf += dwLen; // Skip message description (usually "ICQSMS\0")
  dwDataLen -= dwLen;

  // Unknown (0,0,0)
  buf += 3;
  dwDataLen -= 3;

  // Remaining bytes
  unpackLEDWord(&buf, &dwLen);
  dwDataLen -= 4;
  if (dwLen > dwDataLen)
  {
    NetLog_Server("SMS failed syntax check %d (%d, %d)", 2, dwDataLen, dwLen);
    return;
  }

  // Length of message
  unpackLEDWord(&buf, &dwTextLen);
  dwDataLen -= 4;
  if ((dwTextLen > dwDataLen) || (dwTextLen+4 != dwLen))
  {
    NetLog_Server("SMS failed syntax check %d (%d, %d, %d)", 3, dwDataLen, dwLen, dwTextLen);
    return;
  }

  // Unpack message
  szInfo = (char *)malloc(dwTextLen + 1);
  memcpy(szInfo, buf, dwTextLen);
  szInfo[dwTextLen] = 0;

  ICQBroadcastAck(NULL, ICQACKTYPE_SMS, ACKRESULT_SUCCESS, NULL, (LPARAM)szInfo);

  SAFE_FREE(&szInfo);
}



static HANDLE handleMessageAck(DWORD dwUin, WORD wCookie, int type, DWORD dwLengthToEnd, WORD wMsgLen, PBYTE buf, BYTE bFlags)
{
  if (bFlags == 3)
  {
    CCSDATA ccs;
    PROTORECVEVENT pre;
    int status;
    HANDLE hContact;
    DWORD dwCookieUin;
    message_cookie_data* pCookieData = NULL;

    hContact = HContactFromUIN(dwUin, NULL);

    if (hContact == INVALID_HANDLE_VALUE)
    {
      NetLog_Server("handleMessageAck: Ignoring status message from unknown contact %u", dwUin);
      return INVALID_HANDLE_VALUE;
    }

    if (!FindCookie(wCookie, &dwCookieUin, &pCookieData))
    {
      NetLog_Server("handleMessageAck: Ignoring unrequested status message from %u", dwUin);
      FreeCookie(wCookie);
      SAFE_FREE(&pCookieData);
      return INVALID_HANDLE_VALUE;
    }

    if (dwUin != dwCookieUin)
    {
      NetLog_Server("handleMessageAck: Ack UIN does not match Cookie UIN(%u != %u)", dwUin, dwCookieUin);
      FreeCookie(wCookie);
      SAFE_FREE(&pCookieData);
      return INVALID_HANDLE_VALUE;
    }
    FreeCookie(wCookie);
    SAFE_FREE(&pCookieData);

    status = AwayMsgTypeToStatus(type);
    if (status == ID_STATUS_OFFLINE)
    {
      NetLog_Server("handleMessageAck: Ignoring unknown status message from %u", dwUin);
      return INVALID_HANDLE_VALUE;
    }

    ccs.szProtoService = PSR_AWAYMSG;
    ccs.hContact = hContact;
    ccs.wParam = status;
    ccs.lParam = (LPARAM)&pre;
    pre.flags = 0;
    pre.szMessage = (char*)buf;
    pre.timestamp = time(NULL);
    pre.lParam = wCookie;

    CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
  }
  else
  {
    // Should not happen
    NetLog_Server("handleMessageAck: Ignored type %u ack message (this should not happen)", type);
  }

  return INVALID_HANDLE_VALUE;
}



/* this function also processes direct packets, so it should be bulletproof */
/* pMsg points to the beginning of the message */
void handleMessageTypes(DWORD dwUin, DWORD dwTimestamp, DWORD dwRecvTimestamp, DWORD dwRecvTimestamp2, WORD wCookie, int type, int flags, WORD wAckType, DWORD dwDataLen, WORD wMsgLen, char *pMsg, BOOL bThruDC)
{
  char* szMsg;
  char* pszMsgField[2*MAX_CONTACTSSEND+1];
  char* pszMsg;
  int nMsgFields;
  HANDLE hContact = INVALID_HANDLE_VALUE;
  int bAdded;


  if (dwDataLen < wMsgLen)
  {
    if (bThruDC)
      NetLog_Direct("Ignoring overflowed message");
    else
      NetLog_Server("Ignoring overflowed message");
    return;
  }

  if (wAckType == 2)
  {
    handleMessageAck(dwUin, wCookie, type, dwDataLen, wMsgLen, pMsg, (BYTE)flags);
    return;
  }

  szMsg = (char *)malloc(wMsgLen + 1);
  if (wMsgLen > 0) {
    memcpy(szMsg, pMsg, wMsgLen);
    pMsg += wMsgLen;
    dwDataLen -= wMsgLen;
  }
  szMsg[wMsgLen] = '\0';


  pszMsgField[0] = szMsg;
  nMsgFields = 0;
  if (type == MTYPE_URL || type == MTYPE_AUTHREQ || type == MTYPE_ADDED || type == MTYPE_CONTACTS || type == MTYPE_EEXPRESS || type == MTYPE_WWP)
  {
    for (pszMsg=szMsg, nMsgFields=1; *pszMsg; pszMsg++)
    {
      if ((unsigned char)*pszMsg == 0xFE)
      {
        *pszMsg = '\0';
        pszMsgField[nMsgFields++] = pszMsg + 1;
        if (nMsgFields >= sizeof(pszMsgField)/sizeof(pszMsgField[0]))
          break;
      }
    }
  }


  switch (type)
  {

  case MTYPE_PLAIN:    /* plain message */
    {
      CCSDATA ccs;
      PROTORECVEVENT pre;

      pre.flags = 0;

      // Check if this message is marked as UTF8 encoded
      if (dwDataLen > 12)
      {
        DWORD dwGuidLen;

        wchar_t* usMsg;
        wchar_t* usMsgW;

        dwGuidLen = (DWORD)*(pMsg+8);
        dwDataLen -= 12;
        pMsg += 12;

        while ((dwGuidLen >= 38) && (dwDataLen >= dwGuidLen))
        {
          if (!strncmp(pMsg, CAP_UTF8MSGS, 38))
          { // Found UTF8 cap, convert message to ansi
            char *szAnsiMessage = NULL;

            if (utf8_decode(szMsg, &szAnsiMessage))
            {
              if (!strcmp(szMsg, szAnsiMessage))
              {
                SAFE_FREE(&szMsg);
                szMsg = szAnsiMessage;
              }
              else
              {
                usMsg = malloc((strlennull(szAnsiMessage)+1)*(sizeof(wchar_t)+1));
                memcpy((char*)usMsg, szAnsiMessage, strlennull(szAnsiMessage)+1);
                usMsgW = make_unicode_string(szMsg);
                memcpy((char*)usMsg+strlennull(szAnsiMessage)+1, (char*)usMsgW, (strlennull(szAnsiMessage)+1)*sizeof(wchar_t));
                SAFE_FREE(&usMsgW);
                SAFE_FREE(&szAnsiMessage);
                SAFE_FREE(&szMsg);
                szMsg = (char*)usMsg;
                pre.flags = PREF_UNICODE;
              }
            }
            else
            {
              if (bThruDC)
                NetLog_Direct("Failed to translate UTF-8 message.");
              else
                NetLog_Server("Failed to translate UTF-8 message.");
            }

            break;
          }
          else if (!strncmp(pMsg, CAP_RTFMSGS, 38))
          { // Found RichText cap
            if (bThruDC)
              NetLog_Direct("Warning: User %u sends us RichText.", dwUin);
            else
              NetLog_Server("Warning: User %u sends us RichText.", dwUin);

            break;
          }

          dwGuidLen -= 38;
          dwDataLen -= 38;
          pMsg += 38;
        }
      }

      hContact = HContactFromUIN(dwUin, &bAdded);

      if (!pre.flags && !IsUSASCII(szMsg, strlennull(szMsg)))
      { // message is Ansi and contains national characters, create Unicode part by codepage
        WORD wCP = ICQGetContactSettingWord(hContact, "CodePage", gwAnsiCodepage);

        if (wCP != CP_ACP)
        {
          wchar_t* usMsg;

          usMsg = malloc((strlennull(szMsg)+2)*(sizeof(wchar_t)+1));
          memcpy((char*)usMsg, szMsg, strlennull(szMsg)+1);
          MultiByteToWideChar(wCP, 0, szMsg, strlennull(szMsg), (wchar_t*)((char*)usMsg + strlennull(szMsg) + 1), strlennull(szMsg));
          *(wchar_t*)((char*)usMsg + 1 + strlennull(szMsg)*(1 + sizeof(wchar_t))) = '\0'; // trailing zeros
          SAFE_FREE(&szMsg);
          szMsg = (char*)usMsg;
          pre.flags = PREF_UNICODE;
        }
      }

      ccs.szProtoService = PSR_MESSAGE;
      ccs.hContact = hContact;
      ccs.wParam = 0;
      ccs.lParam = (LPARAM)&pre;
      pre.timestamp = dwTimestamp;
      pre.szMessage = (char *)szMsg;
      pre.lParam = 0;

      CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
    }
    break;

  case MTYPE_URL:
    {
      CCSDATA ccs;
      PROTORECVEVENT pre;
      char* szBlob;


      if (nMsgFields < 2)
      {
        if (bThruDC)
          NetLog_Direct("Malformed URL message");
        else
          NetLog_Server("Malformed URL message");
        break;
      }

      szBlob = (char *)malloc(strlennull(pszMsgField[0]) + strlennull(pszMsgField[1]) + 2);
      strcpy(szBlob, pszMsgField[1]);
      strcpy(szBlob + strlennull(szBlob) + 1, pszMsgField[0]);

      ccs.szProtoService = PSR_URL;
      ccs.hContact = hContact = HContactFromUIN(dwUin, &bAdded);
      ccs.wParam = 0;
      ccs.lParam = (LPARAM)&pre;
      pre.flags = 0;
      pre.timestamp = dwTimestamp;
      pre.szMessage = (char *)szBlob;
      pre.lParam = 0;

      CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);

      SAFE_FREE(&szBlob);
    }
    break;

  case MTYPE_AUTHREQ:       /* auth request */
    /* format: nick FE first FE last FE email FE unk-char FE msg 00 */
    {
      CCSDATA ccs;
      PROTORECVEVENT pre;
      char* szBlob;
      char* pCurBlob;


      if (nMsgFields < 6)
      {
        NetLog_Server("Malformed auth req message");
        break;
      }

      ccs.szProtoService=PSR_AUTH;
      ccs.hContact=hContact=HContactFromUIN(dwUin, &bAdded);
      ccs.wParam=0;
      ccs.lParam=(LPARAM)&pre;
      pre.flags=0;
      pre.timestamp=dwTimestamp;
      pre.lParam=sizeof(DWORD)+sizeof(HANDLE)+strlennull(pszMsgField[0])+strlennull(pszMsgField[1])+strlennull(pszMsgField[2])+strlennull(pszMsgField[3])+strlennull(pszMsgField[5])+5;

      /*blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)*/
      pCurBlob=szBlob=(char *)malloc(pre.lParam);
      memcpy(pCurBlob,&dwUin,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
      memcpy(pCurBlob,&hContact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
      strcpy((char *)pCurBlob,pszMsgField[0]); pCurBlob+=strlennull((char *)pCurBlob)+1;
      strcpy((char *)pCurBlob,pszMsgField[1]); pCurBlob+=strlennull((char *)pCurBlob)+1;
      strcpy((char *)pCurBlob,pszMsgField[2]); pCurBlob+=strlennull((char *)pCurBlob)+1;
      strcpy((char *)pCurBlob,pszMsgField[3]); pCurBlob+=strlennull((char *)pCurBlob)+1;
      strcpy((char *)pCurBlob,pszMsgField[5]);
      pre.szMessage=(char *)szBlob;

      CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);
    }
    break;

  case MTYPE_ADDED:       /* 'you were added' */
    /* format: nick FE first FE last FE email 00 */
    {
      DBEVENTINFO dbei;
      PBYTE pCurBlob;

      if (nMsgFields < 4)
      {
        NetLog_Server("Malformed 'you were added' message");
        break;
      }

      hContact=HContactFromUIN(dwUin, &bAdded);

      /*blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ) */
      ZeroMemory(&dbei,sizeof(dbei));
      dbei.cbSize=sizeof(dbei);
      dbei.szModule=gpszICQProtoName;
      dbei.timestamp=dwTimestamp;
      dbei.flags=0;
      dbei.eventType=EVENTTYPE_ADDED;
      dbei.cbBlob=sizeof(DWORD)+sizeof(HANDLE)+strlennull(pszMsgField[0])+strlennull(pszMsgField[1])+strlennull(pszMsgField[2])+strlennull(pszMsgField[3])+4;
      pCurBlob=dbei.pBlob=(PBYTE)malloc(dbei.cbBlob);
      memcpy(pCurBlob,&dwUin,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
      memcpy(pCurBlob,&hContact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
      strcpy((char *)pCurBlob,pszMsgField[0]); pCurBlob+=strlennull((char *)pCurBlob)+1;
      strcpy((char *)pCurBlob,pszMsgField[1]); pCurBlob+=strlennull((char *)pCurBlob)+1;
      strcpy((char *)pCurBlob,pszMsgField[2]); pCurBlob+=strlennull((char *)pCurBlob)+1;
      strcpy((char *)pCurBlob,pszMsgField[3]);

      CallService(MS_DB_EVENT_ADD,(WPARAM)(HANDLE)NULL,(LPARAM)&dbei);
    }
    break;

  case MTYPE_CONTACTS:
    {
      CCSDATA ccs;
      PROTORECVEVENT pre;
      ICQSEARCHRESULT** isrList;
      char* pszNContactsEnd;
      int nContacts;
      int i;
      int valid;


      if (nMsgFields < 3
        || (nContacts = strtol(pszMsgField[0], &pszNContactsEnd, 10)) == 0
        || pszNContactsEnd - pszMsgField[0] != (int)strlennull(pszMsgField[0])
        || nMsgFields < nContacts * 2 + 1)
      {
        if (bThruDC)
          NetLog_Direct("Malformed contacts message%s", "");
        else
          NetLog_Server("Malformed contacts message%s", "");
        break;
      }

      valid = 1;
      isrList = (ICQSEARCHRESULT**)malloc(nContacts * sizeof(ICQSEARCHRESULT*));
      for (i = 0; i < nContacts; i++)
      {
        isrList[i] = (ICQSEARCHRESULT*)calloc(1, sizeof(ICQSEARCHRESULT));
        isrList[i]->hdr.cbSize = sizeof(ICQSEARCHRESULT);
        isrList[i]->uin = atoi(pszMsgField[1 + i * 2]);
        if (isrList[i]->uin == 0)
          valid = 0;
        isrList[i]->hdr.nick = pszMsgField[2 + i * 2];
      }

      if (!valid)
      {
        if (bThruDC)
          NetLog_Direct("Malformed contacts message%s", " (most probably contained AIM contacts)");
        else
          NetLog_Server("Malformed contacts message%s", " (most probably contained AIM contacts)");
      }
      else
      {
        ccs.szProtoService = PSR_CONTACTS;
        ccs.hContact = hContact = HContactFromUIN(dwUin, &bAdded);
        ccs.wParam = 0;
        ccs.lParam = (LPARAM)&pre;
        pre.flags = 0;
        pre.timestamp = dwTimestamp;
        pre.szMessage = (char *)isrList;
        pre.lParam = nContacts;
        CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
      }

      for (i = 0; i < nContacts; i++)
        SAFE_FREE(&isrList[i]);

      SAFE_FREE(&(void*)isrList);
    }
    break;

  case MTYPE_PLUGIN:
    hContact = NULL;

    switch(dwUin)
    {
      case 1002:    /* SMS receipt */
        handleSmsReceipt(pMsg, dwDataLen);
        break;

      case 1111:      /* icqmail 'you've got mail' - not processed */
        break;
    }
    break;

  case MTYPE_WWP:
    /* format: fromname FE FE FE fromemail FE unknownbyte FE 'Sender IP: xxx.xxx.xxx.xxx' 0D 0A body */
    {
      DBEVENTINFO dbei;
      PBYTE pCurBlob;


      if (nMsgFields < 6)
      {
        NetLog_Server("Malformed web pager message");
        break;
      }

      /*blob is: body(ASCIIZ), name(ASCIIZ), email(ASCIIZ) */
      ZeroMemory(&dbei,sizeof(dbei));
      dbei.cbSize=sizeof(dbei);
      dbei.szModule=gpszICQProtoName;
      dbei.timestamp=dwTimestamp;
      dbei.flags=0;
      dbei.eventType=ICQEVENTTYPE_WEBPAGER;
      dbei.cbBlob=strlennull(pszMsgField[0])+strlennull(pszMsgField[3])+strlennull(pszMsgField[5])+3;
      pCurBlob=dbei.pBlob=(PBYTE)malloc(dbei.cbBlob);
      strcpy((char *)pCurBlob,pszMsgField[5]); pCurBlob+=strlennull((char *)pCurBlob)+1;
      strcpy((char *)pCurBlob,pszMsgField[0]); pCurBlob+=strlennull((char *)pCurBlob)+1;
      strcpy((char *)pCurBlob,pszMsgField[3]);

      CallService(MS_DB_EVENT_ADD,(WPARAM)(HANDLE)NULL,(LPARAM)&dbei);
    }
    break;

  case MTYPE_EEXPRESS:
    /* format: fromname FE FE FE fromemail FE unknownbyte FE body */
    {
      DBEVENTINFO dbei;
      PBYTE pCurBlob;


      if (nMsgFields < 6)
      {
        NetLog_Server("Malformed e-mail express message");
        break;
      }

      /*blob is: body(ASCIIZ), name(ASCIIZ), email(ASCIIZ) */
      ZeroMemory(&dbei,sizeof(dbei));
      dbei.cbSize=sizeof(dbei);
      dbei.szModule=gpszICQProtoName;
      dbei.timestamp=dwTimestamp;
      dbei.flags=0;
      dbei.eventType=ICQEVENTTYPE_EMAILEXPRESS;
      dbei.cbBlob=strlennull(pszMsgField[0])+strlennull(pszMsgField[3])+strlennull(pszMsgField[5])+3;
      pCurBlob=dbei.pBlob=(PBYTE)malloc(dbei.cbBlob);
      strcpy((char *)pCurBlob,pszMsgField[5]); pCurBlob+=strlennull((char *)pCurBlob)+1;
      strcpy((char *)pCurBlob,pszMsgField[0]); pCurBlob+=strlennull((char *)pCurBlob)+1;
      strcpy((char *)pCurBlob,pszMsgField[3]);

      CallService(MS_DB_EVENT_ADD,(WPARAM)(HANDLE)NULL,(LPARAM)&dbei);
    }
    break;

  case MTYPE_REQUESTCONTACTS:
    /* it's a contacts-request */
    if (bThruDC)
      NetLog_Direct("Received Request for Contacts msg from %u", dwUin);
    else
      NetLog_Server("Received Request for Contacts msg from %u", dwUin);
    break;

  case MTYPE_GREETINGCARD:
    /* it's a greeting card */
    if (bThruDC)
      NetLog_Direct("Received Greeting Card msg from %u", dwUin);
    else
      NetLog_Server("Received Greeting Card msg from %u", dwUin);
    break;

  case MTYPE_SCRIPT_NOTIFY:
    /* it's a xtraz notify request */
    if (bThruDC)
      NetLog_Direct("Received Xtraz Notify Request from %u", dwUin);
    else
      NetLog_Server("Received Xtraz Notify Request from %u", dwUin);
    handleXtrazNotify(dwUin, dwRecvTimestamp, dwRecvTimestamp2, wCookie, szMsg, wMsgLen, bThruDC);
    break;

  case MTYPE_AUTOAWAY:
  case MTYPE_AUTOBUSY:
  case MTYPE_AUTONA:
  case MTYPE_AUTODND:
  case MTYPE_AUTOFFC:
    {
      char** szMsg = MirandaStatusToAwayMsg(AwayMsgTypeToStatus(type));

      if (szMsg)
        icq_sendAwayMsgReplyServ(dwUin, dwRecvTimestamp, dwRecvTimestamp2, wCookie, (BYTE)type, szMsg);

      break;
    }

  case MTYPE_FILEREQ: // Never happens
  default:
    if (bThruDC)
      NetLog_Direct("Unprocessed message type %d", type);
    else
      NetLog_Server("Unprocessed message type %d", type);
    break;

  }

  SAFE_FREE(&szMsg);
}



static void handleRecvMsgResponse(unsigned char *buf, WORD wLen, WORD wFlags, DWORD dwRef)
{
  DWORD dwUin;
  WORD wCookie;
  WORD wMessageFormat;
  WORD wStatus;
  BYTE bMsgType;
  BYTE bFlags;
  WORD wLength;
  HANDLE hContact;
  DWORD dwCookieUin;
  message_cookie_data* pCookieData = NULL;


  buf += 8;  // Message ID
  wLen -= 8;

  unpackWord(&buf, &wMessageFormat);
  wLen -= 2;
  if (wMessageFormat != 2)
  {
    NetLog_Server("SNAC(4.B) Unknown type");
    return;
  }

  if (!unpackUID(&buf, &wLen, &dwUin, NULL)) return;

  hContact = HContactFromUIN(dwUin, NULL);

  buf += 2;   // 3. unknown
  wLen -= 2;

  // Length of sub chunk?
  unpackLEWord(&buf, &wLength);
  wLen -= 2;
  if (wLength != 0x1b)
    NetLog_Server("SNAC(4.B) Subchunk length mismatch");

  buf += 29;  /* unknowns from the msg we sent */
  wLen -= 29;

  // Message sequence (SEQ2)
  unpackLEWord(&buf, &wCookie);
  wLen -= 2;

  // Unknown (12 bytes)
  buf += 12;
  wLen -= 12;

  // Message type
  unpackByte(&buf, &bMsgType);
  unpackByte(&buf, &bFlags);
  wLen -= 2;

  // Status
  unpackLEWord(&buf, &wStatus);
  wLen -= 2;

  // Priority?
  buf += 2;
  wLen -= 2;

  if (!FindCookie(wCookie, &dwCookieUin, &pCookieData))
  {
    NetLog_Server("SNAC(4.B) Received an ack that I did not ask for from (%u)", dwUin);
    return;
  }

  if (dwCookieUin != dwUin)
  {
    NetLog_Server("SNAC(4.B) Ack UIN does not match Cookie UIN(%u != %u)", dwUin, dwCookieUin);
    SAFE_FREE(&pCookieData); // This could be a bad idea, but I think it is safe
    FreeCookie(wCookie);
    return;
  }

  if (bFlags == 3)     // A status message reply
  {
    CCSDATA ccs;
    PROTORECVEVENT pre;
    int status;


    if (hContact == INVALID_HANDLE_VALUE)
    {
      NetLog_Server("SNAC(4.B) Ignoring status message from unknown contact");
      return;
    }

    status = AwayMsgTypeToStatus(bMsgType);
    if (status == ID_STATUS_OFFLINE)
    {
      NetLog_Server("SNAC(4.B) Ignoring unknown status message from %u", dwUin);
      return;
    }

    ccs.szProtoService = PSR_AWAYMSG;
    ccs.hContact = hContact;
    ccs.wParam = status;
    ccs.lParam = (LPARAM)&pre;
    pre.flags = 0;
    pre.szMessage = (char*)(buf + 2);
    pre.timestamp = time(NULL);
    pre.lParam = wCookie;

    CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);

  }
  else
  { // An ack of some kind
    int ackType;


    if (hContact == NULL || hContact == INVALID_HANDLE_VALUE)
    {
      NetLog_Server("SNAC(4.B) Message from unknown contact (%u)", dwUin);
      SAFE_FREE(&pCookieData); // This could be a bad idea, but I think it is safe
      FreeCookie(wCookie);
      return;
    }

    switch (bMsgType)
    {

    case MTYPE_FILEREQ:
      {
        char* szMsg;
        WORD wMsgLen;

        // Message length
        unpackLEWord(&buf, &wMsgLen);
        wLen -= 2;
        szMsg = (char *)malloc(wMsgLen + 1);
        szMsg[wMsgLen] = '\0';
        if (wMsgLen > 0) {
          memcpy(szMsg, buf, wMsgLen);
          buf += wMsgLen;
          wLen -= wMsgLen;
        }
        handleFileAck(buf, wLen, dwUin, wCookie, wStatus, szMsg);
        // No success protoack will be sent here, since all file requests
        // will have been 'sent' when the server returns its ack
        return;
      }

    case MTYPE_PLUGIN:
      {
        DWORD dwLengthToEnd;
        DWORD dwDataLen;
        DWORD dwPluginNameLen;
        int typeId;
        WORD wInfoLen;
        char* szPluginName;
        DWORD q1,q2,q3,q4;
        WORD qt;


        NetLog_Server("Parsing Greeting message response");

        // Message
        unpackLEWord(&buf, &wInfoLen);
        wLen -= 2;
        buf += wInfoLen;
        wLen -= wInfoLen;

        // This packet is malformed. Possibly a file accept from Miranda IM 0.1.2.1
        if (wLen < 20)
          return;

        unpackLEWord(&buf, &wInfoLen);
        wLen -= 2;

        unpackDWord(&buf, &q1); // get data GUID & function id
        unpackDWord(&buf, &q2);
        unpackDWord(&buf, &q3);
        unpackDWord(&buf, &q4);
        unpackLEWord(&buf, &qt);
        wLen -= 18;

        unpackLEDWord(&buf, &dwPluginNameLen);
        wLen -= 4;
        if (dwPluginNameLen > wLen) 
        { // check for malformed plugin name
          dwPluginNameLen = wLen;
          NetLog_Server("Warning: malformed size of plugin name.");
        }
        szPluginName = (char *)malloc(dwPluginNameLen + 1);
        memcpy(szPluginName, buf, dwPluginNameLen);
        szPluginName[dwPluginNameLen] = '\0';

        buf += dwPluginNameLen + 15;
        wLen -= ((WORD)dwPluginNameLen + 15);

        typeId = TypeGUIDToTypeId(q1, q2, q3, q4, qt);
        if (!typeId)
          NetLog_Server("Error: Unknown type {%04x%04x%04x%04x-%02x}: %s", q1,q2,q3,q4,qt);

        SAFE_FREE(&szPluginName);

        // Length of remaining data
        unpackLEDWord(&buf, &dwLengthToEnd);
        wLen -= 4;

        if (dwLengthToEnd > 0)
          unpackLEDWord(&buf, &dwDataLen); // Length of message
        else
          dwDataLen = 0;


        switch (typeId)
        {

        case MTYPE_URL:
          ackType = ACKTYPE_URL;
          break;

        case MTYPE_CONTACTS:
          ackType = ACKTYPE_CONTACTS;
          break;

        case MTYPE_FILEREQ:
          {
            char* szMsg;

            NetLog_Server("This is file ack");
            szMsg = (char *)malloc(dwDataLen + 1);
            if (dwDataLen > 0)
              memcpy(szMsg, buf, dwDataLen);
            szMsg[dwDataLen] = '\0';
            buf += dwDataLen;
            wLen -= (WORD)dwDataLen;

            handleFileAck(buf, wLen, dwUin, wCookie, wStatus, szMsg);
            // No success protoack will be sent here, since all file requests
            // will have been 'sent' when the server returns its ack
          }
          return;

        case MTYPE_SCRIPT_NOTIFY:
          {
            char *szMsg;

            szMsg = (char*)malloc(dwDataLen + 1);
            if (dwDataLen > 0)
              memcpy(szMsg, buf, dwDataLen);
            szMsg[dwDataLen] = '\0';

            handleXtrazNotifyResponse(dwUin, hContact, szMsg, dwDataLen);
            
            SAFE_FREE(&pCookieData);
            FreeCookie(wCookie);
          }
          return;

        default:
          NetLog_Server("I'm confused, received unknown ack in extended channel 2 message response");
          return;
        }
      }
      break;

    case MTYPE_PLAIN:
      ackType = ACKTYPE_MESSAGE;
      break;

    case MTYPE_URL:
      ackType = ACKTYPE_URL;
      break;

    case MTYPE_AUTHOK:
    case MTYPE_AUTHDENY:
      ackType = ACKTYPE_AUTHREQ;
      break;

    case MTYPE_ADDED:
      ackType = ACKTYPE_ADDED;
      break;

    case MTYPE_CONTACTS:
      ackType = ACKTYPE_CONTACTS;
      break;

    case MTYPE_CHAT:
    default:
      NetLog_Server("SNAC(4.B) Unknown message type (%u) in switch", bMsgType);
      return;
    }

    if ((ackType == MTYPE_PLAIN && pCookieData && (pCookieData->nAckType == ACKTYPE_CLIENT)) ||
      ackType != MTYPE_PLAIN)
    {
      ICQBroadcastAck(hContact, ackType, ACKRESULT_SUCCESS, (HANDLE)wCookie, 0);
    }
  }

  SAFE_FREE(&pCookieData);

  FreeCookie(wCookie);
}


// A response to a CLI_SENDMSG
static void handleRecvServMsgError(unsigned char *buf, WORD wLen, WORD wFlags, DWORD dwSequence)
{
  WORD wError;
  char* pszErrorMessage;
  DWORD dwUin;
  HANDLE hContact;
  message_cookie_data* pCookieData = NULL;
  int nMessageType;


  if (wLen < 2)
    return;

  if (FindCookie((WORD)dwSequence, &dwUin, &pCookieData))
  { // all packet cookies from msg family has command 0 in the queue
    hContact = HContactFromUIN(dwUin, NULL);

    // Error code
    unpackWord(&buf, &wError);

    if ((hContact == NULL) || (hContact == INVALID_HANDLE_VALUE))
    {
      NetLog_Server("SNAC(4.1) Received a SENDMSG Error (%u) from invalid contact %u", wError, dwUin);
      SAFE_FREE(&pCookieData);
      FreeCookie((WORD)dwSequence);

      return;
    }
    if (wError == 9 && pCookieData->bMessageType == MTYPE_AUTOAWAY)
    { // we failed to request away message the normal way, try it AIM way
       icq_packet packet;

      packet.wLen = 13 + getUINLen(dwUin);
      write_flap(&packet, ICQ_DATA_CHAN);
      packFNACHeader(&packet, ICQ_LOCATION_FAMILY, ICQ_LOCATION_REQ_USER_INFO, 0, dwSequence);
      packWord(&packet, 0x03);
      packUIN(&packet, dwUin);

      sendServPacket(&packet);

      return;
    }

    // Not all of these are actually used in family 4
    // This will be moved into a special error handling function later
    switch (wError)
    {

    case 0x0002:     // Server rate limit exceeded
      pszErrorMessage = strdup(Translate("You are sending too fast. Wait a while and try again.\nSNAC(4.1) Error x02"));
      break;

    case 0x0003:     // Client rate limit exceeded
      pszErrorMessage = strdup(Translate("You are sending too fast. Wait a while and try again.\nSNAC(4.1) Error x03"));
      break;

    case 0x0004:     // Recipient is not logged in (resend in a offline message)
      if (pCookieData->bMessageType != MTYPE_PLUGIN) // TODO: this needs better solution
        ICQWriteContactSettingWord(hContact, "Status", ID_STATUS_OFFLINE);
      pszErrorMessage = strdup(Translate("The user has logged off. Select 'Retry' to send an offline message.\nSNAC(4.1) Error x04"));
      break;

    case 0x0009:     // Not supported by client (resend in a simpler format)
      pszErrorMessage = strdup(Translate("The receiving client does not support this type of message.\nSNAC(4.1) Error x09"));
      break;

    case 0x000A:     // Refused by client
      pszErrorMessage = strdup(Translate("You sent too long message. The receiving client does not support it.\nSNAC(4.1) Error x0A"));
      break;

    case 0x000E:     // Incorrect SNAC format
      pszErrorMessage = strdup(Translate("The SNAC format was rejected by the server.\nSNAC(4.1) Error x0E"));
      break;

    case 0x0013:     // User temporarily unavailable
      pszErrorMessage = strdup(Translate("The user is temporarily unavailable. Wait a while and try again.\nSNAC(4.1) Error x13"));
      break;

    case 0x0001:     // Invalid SNAC header
    case 0x0005:     // Requested service unavailable
    case 0x0006:     // Requested service not defined
    case 0x0007:     // You sent obsolete SNAC
    case 0x0008:     // Not supported by server
    case 0x000B:     // Reply too big
    case 0x000C:     // Responses lost
    case 0x000D:     // Request denied
    case 0x000F:     // Insufficient rights
    case 0x0010:     // In local permit/deny (recipient blocked)
    case 0x0011:     // Sender too evil
    case 0x0012:     // Receiver too evil
    case 0x0014:     // No match
    case 0x0015:     // List overflow
    case 0x0016:     // Request ambiguous
    case 0x0017:     // Server queue full
    case 0x0018:     // Not while on AOL
    default:
      if (pszErrorMessage = malloc(256))
        null_snprintf(pszErrorMessage, 256, Translate("SNAC(4.1) SENDMSG Error (x%02x)"), wError);
      break;
    }


    switch (pCookieData->bMessageType)
    {

    case MTYPE_PLAIN:
      nMessageType = ACKTYPE_MESSAGE;
      break;

    case MTYPE_CHAT:
      nMessageType = ACKTYPE_CHAT;
      break;

    case MTYPE_FILEREQ:
      nMessageType = ACKTYPE_FILE;
      break;

    case MTYPE_URL:
      nMessageType = ACKTYPE_URL;
      break;

    case MTYPE_CONTACTS:
      nMessageType = ACKTYPE_CONTACTS;
      break;

    default:
      nMessageType = -1;
      break;
    }

    if (nMessageType != -1)
    {
      ICQBroadcastAck(hContact, nMessageType, ACKRESULT_FAILED, (HANDLE)(WORD)dwSequence, (LPARAM)pszErrorMessage);
    }
    else
    {
      NetLog_Server("Error: Message delivery to %u failed: %s", dwUin, pszErrorMessage);
    }

    FreeCookie((WORD)dwSequence);

    if (pCookieData->bMessageType != MTYPE_FILEREQ)
      SAFE_FREE(&pCookieData);

    SAFE_FREE(&pszErrorMessage);
  }
  else
  {
    WORD wError;

    unpackWord(&buf, &wError);

    LogFamilyError(ICQ_MSG_FAMILY, wError);
  }
}



static void handleServerAck(unsigned char *buf, WORD wLen, WORD wFlags, DWORD dwSequence)
{
  DWORD dwUin;
  uid_str szUID;
  WORD wChannel;
  HANDLE hContact;
  message_cookie_data* pCookieData;


  if (wLen < 13)
  {
    NetLog_Server("Ignoring SNAC(4,C) Packet to short");
    return;
  }

  buf += 8; // Skip first 8 bytes
  wLen -= 8;

  // Message channel
  unpackWord(&buf, &wChannel);
  wLen -= 2;

  // Sender
  if (!unpackUID(&buf, &wLen, &dwUin, &szUID)) return;

  if (dwUin)
    hContact = HContactFromUIN(dwUin, NULL);
  else
    hContact = HContactFromUID(szUID, NULL);

  if (FindCookie((WORD)dwSequence, &dwUin, &pCookieData))
  {
    // If the user requested a full ack, the
    // server ack should be ignored here.
    if (pCookieData && (pCookieData->nAckType == ACKTYPE_SERVER))
    {
      if ((hContact != NULL) && (hContact != INVALID_HANDLE_VALUE))
      {
        int ackType;
        int ackRes = ACKRESULT_SUCCESS;

        switch (pCookieData->bMessageType)
        {
        case MTYPE_PLAIN:
          ackType = ACKTYPE_MESSAGE;
          break;

        case MTYPE_CONTACTS:
          ackType = ACKTYPE_CONTACTS;
          break;

        case MTYPE_URL:
          ackType = ACKTYPE_URL;
          break;

        case MTYPE_FILEREQ:
          ackType = ACKTYPE_FILE;
          ackRes = ACKRESULT_SENTREQUEST;
          // Note 1: We are not allowed to free the cookie here because it
          // contains the filetransfer struct that we will need later
          // Note 2: The cookiedata is NOT a message_cookie_data*, it is a
          // filetransfer*. IMPORTANT! (it's one of those silly things)
          break;

        default:
          ackType = -1;
          NetLog_Server("Error: Unknown message type %d in ack", pCookieData->bMessageType);
          break;
        }
        if (ackType != -1)
          ICQBroadcastAck(hContact, ackType, ackRes, (HANDLE)(WORD)dwSequence, 0);

        if (pCookieData->bMessageType != MTYPE_FILEREQ)
          SAFE_FREE(&pCookieData); // this could be a bad idea, but I think it is safe

        FreeCookie((WORD)dwSequence);
      }
    }
    else
    {
      NetLog_Server("Ignored a server ack I did not ask for");
    }
  }

  return;
}



static void handleMissedMsg(unsigned char *buf, WORD wLen, WORD wFlags, DWORD dwRef)
{
  DWORD dwUin;
  WORD wChannel;
  WORD wWarningLevel;
  WORD wCount;
  WORD wError;
  WORD wTLVCount;
  char* pszErrorMsg;
  oscar_tlv_chain* pChain;


  if (wLen < 14)
    return; // Too short

  // Message channel?
  unpackWord(&buf, &wChannel);
  wLen -= 2;

  // Sender
  if (!unpackUID(&buf, &wLen, &dwUin, NULL)) return;

  if (wLen < 8)
    return; // Too short

  // Warning level?
  unpackWord(&buf, &wWarningLevel);
  wLen -= 2;

  // TLV count
  unpackWord(&buf, &wTLVCount);
  wLen -= 2;

  // Read past user info TLVs
  pChain = readIntoTLVChain(&buf, (WORD)(wLen-4), wTLVCount);

  if (pChain)
    disposeChain(&pChain);

  if (wLen < 4)
    return; // Too short


  // Number of missed messages
  unpackWord(&buf, &wCount);
  wLen -= 2;

  // Error code
  unpackWord(&buf, &wError);
  wLen -= 2;

  switch (wError)
  {

  case 0:
    pszErrorMsg = Translate("** This message was blocked by the ICQ server ** The message was invalid.");
    break;

  case 1:
    pszErrorMsg = Translate("** This message was blocked by the ICQ server ** The message was too long.");
    break;

  case 2:
    pszErrorMsg = Translate("** This message was blocked by the ICQ server ** The sender has flooded the server.");
    break;

  case 4:
    pszErrorMsg = Translate("** This message was blocked by the ICQ server ** You are too evil.");
    break;

  default:
    // 3 = Sender too evil (sender warn level > your max_msg_sevil)
    return;
    break;
  }


  // Create message to notify user
  {
    CCSDATA ccs;
    PROTORECVEVENT pre;
    int bAdded;

    ccs.szProtoService = PSR_MESSAGE;
    ccs.hContact = HContactFromUIN(dwUin, &bAdded);
    ccs.wParam = 0;
    ccs.lParam = (LPARAM)&pre;
    pre.flags = 0;
    pre.timestamp = time(NULL);
    pre.szMessage = pszErrorMsg;
    pre.lParam = 0;

    CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
  }
}



static void handleTypingNotification(unsigned char* buf, WORD wLen, WORD wFlags, DWORD dwRef)
{
  DWORD dwID1;
  DWORD dwID2;
  DWORD dwUin;
  uid_str szUID;
  WORD wChannel;
  WORD wNotification;
  HANDLE hContact;

  if (wLen < 14)
  {
    NetLog_Server("Ignoring SNAC(4.x11) Packet to short");
    return;
  }

#ifndef DBG_CAPMTN
  {
    NetLog_Server("Ignoring unexpected typing notification");
    return;
  }
#endif

  // The message ID, unused?
  unpackLEDWord(&buf, &dwID1);
  wLen -= 4;
  unpackLEDWord(&buf, &dwID2);
  wLen -= 4;

  // Message channel, unused?
  unpackWord(&buf, &wChannel);
  wLen -= 2;

  // Sender
  if (!unpackUID(&buf, &wLen, &dwUin, &szUID)) return;

  if (dwUin)
    hContact = HContactFromUIN(dwUin, NULL);
  else
    hContact = HContactFromUID(szUID, NULL);

  if (hContact == INVALID_HANDLE_VALUE) return;

  // Typing notification code
  unpackWord(&buf, &wNotification);
  wLen -= 2;

  SetContactCapabilities(hContact, CAPF_TYPING);

  // Notify user
  switch (wNotification)
  {

  case MTN_FINISHED:
  case MTN_TYPED:
    CallService(MS_PROTO_CONTACTISTYPING, (WPARAM)hContact, (LPARAM)PROTOTYPE_CONTACTTYPING_OFF);
    if (dwUin)
      NetLog_Server("%u has stopped typing (ch %u).", dwUin, wChannel);
    else
      NetLog_Server("%s has stopped typing (ch %u).", szUID, wChannel);
    break;

  case MTN_BEGUN:
    CallService(MS_PROTO_CONTACTISTYPING, (WPARAM)hContact, (LPARAM)60);
    if (dwUin)
      NetLog_Server("%u is typing a message (ch %u).", dwUin, wChannel);
    else
      NetLog_Server("%s is typing a message (ch %u).", szUID, wChannel);
    break;

  default:
    if (dwUin)
      NetLog_Server("Unknown typing notification from %u, type %u (ch %u)", dwUin, wNotification, wChannel);
    else
      NetLog_Server("Unknown typing notification from %s, type %u (ch %u)", szUID, wNotification, wChannel);
    break;
  }

  return;
}



void sendTypingNotification(HANDLE hContact, WORD wMTNCode)
{
  icq_packet p;
  BYTE byUinlen;
  DWORD dwUin;
  uid_str szUID;

  _ASSERTE((wMTNCode == MTN_FINISHED) || (wMTNCode == MTN_TYPED) || (wMTNCode == MTN_BEGUN));

  if (ICQGetContactSettingUID(hContact, &dwUin, &szUID))
    return; // Invalid contact

  byUinlen = getUIDLen(dwUin, szUID);

  p.wLen = 23 + byUinlen;
  write_flap(&p, ICQ_DATA_CHAN);
  packFNACHeader(&p, ICQ_MSG_FAMILY, ICQ_MSG_MTN, 0, ICQ_MSG_MTN<<0x10);
  packLEDWord(&p, 0x0000);          // Msg ID
  packLEDWord(&p, 0x0000);          // Msg ID
  packWord(&p, 0x01);               // Channel
  packUID(&p, dwUin, szUID);        // User ID
  packWord(&p, wMTNCode);           // Notification type

  sendServPacket(&p);
}
