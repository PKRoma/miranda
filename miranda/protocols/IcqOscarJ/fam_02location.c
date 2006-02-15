// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001,2002 Jon Keating, Richard Hughes
// Copyright � 2002,2003,2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004,2005,2006 Joe Kucera
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


static void handleLocationAwayReply(BYTE* buf, WORD wLen, DWORD dwCookie);


void handleLocationFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader)
{
  switch (pSnacHeader->wSubtype)
  {

  case ICQ_LOCATION_RIGHTS_REPLY: // Reply to CLI_REQLOCATION
    NetLog_Server("Server sent SNAC(x02,x03) - SRV_LOCATION_RIGHTS_REPLY");
    break;

  case ICQ_LOCATION_USR_INFO_REPLY: // AIM user info reply
    handleLocationAwayReply(pBuffer, wBufferLength, pSnacHeader->dwRef);
    break;

  case ICQ_ERROR:
  { 
    WORD wError;
    DWORD dwCookieUin;
    fam15_cookie_data *pCookieData;


    if (wBufferLength >= 2)
      unpackWord(&pBuffer, &wError);
    else 
      wError = 0;

    if (wError == 4)
    {
      if (FindCookie(pSnacHeader->dwRef, &dwCookieUin, &pCookieData) && !dwCookieUin && pCookieData->bRequestType == REQUESTTYPE_PROFILE)
      {
        ICQBroadcastAck(pCookieData->hContact, ACKTYPE_GETINFO, ACKRESULT_FAILED, (HANDLE)1 ,0);

        ReleaseCookie(pSnacHeader->dwRef);
      }
    }

    LogFamilyError(ICQ_LOCATION_FAMILY, wError);
    break;
  }

  default:
    NetLog_Server("Warning: Ignoring SNAC(x%02x,x%02x) - Unknown SNAC (Flags: %u, Ref: %u)", ICQ_LOCATION_FAMILY, pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;
  }
}



static char* AimApplyEncoding(char* pszStr, const char* pszEncoding)
{ // decode encoding to ANSI only
  if (pszStr && pszEncoding)
  {
    char *szEnc = strstr(pszEncoding, "charset=");

    if (szEnc)
    {
      szEnc = szEnc + 9; // get charset string

      if (!strnicmp(szEnc, "utf-8", 5))
      { // it is utf-8 encoded
        char *szRes = NULL;

        utf8_decode(pszStr, &szRes);
        SAFE_FREE(&pszStr);

        return szRes;
      }
      if (!strnicmp(szEnc, "unicode-2-0", 11))
      { // it is UCS-2 encoded
        int wLen = wcslen((wchar_t*)pszStr) + 1;
        wchar_t *szStr = (wchar_t*)_alloca(wLen*2);
        char *szRes = (char*)malloc(wLen);
        char *tmp = pszStr;

        unpackWideString(&tmp, szStr, (WORD)(wLen*2));
        szRes[0] = '\0';
        WideCharToMultiByte(CP_ACP, 0, szStr, -1, szRes, wLen, NULL, NULL);
        SAFE_FREE(&pszStr);

        return szRes;
      }
    }
  }
  return pszStr;
}



void handleLocationAwayReply(BYTE* buf, WORD wLen, DWORD dwCookie)
{
  HANDLE hContact;
  DWORD dwUIN;
  uid_str szUID;
  WORD wTLVCount;
  WORD wWarningLevel;
  DWORD dwCookieUin;
  WORD status;
  message_cookie_data *pCookieData;

  // Unpack the sender's user ID
  if (!unpackUID(&buf, &wLen, &dwUIN, &szUID)) return;

  // Syntax check
  if (wLen < 4)
    return;

  // Warning level?
  unpackWord(&buf, &wWarningLevel);
  wLen -= 2;

  // TLV count
  unpackWord(&buf, &wTLVCount);
  wLen -= 2;

  // Determine contact
  hContact = HContactFromUID(dwUIN, szUID, NULL);

  // Ignore away status if the user is not already on our list
  if (hContact == INVALID_HANDLE_VALUE)
  {
#ifdef _DEBUG
    NetLog_Server("Ignoring away reply (%s)", strUID(dwUIN, szUID));
#endif
    return;
  }

  if (!FindCookie(dwCookie, &dwCookieUin, &pCookieData))
  {
    NetLog_Server("Error: Received unexpected away reply from %s", strUID(dwUIN, szUID));
    return;
  }

  if (dwUIN != dwCookieUin)
  {
    NetLog_Server("Error: Away reply UIN does not match Cookie UIN(%u != %u)", dwUIN, dwCookieUin);

    ReleaseCookie(dwCookie); // This could be a bad idea, but I think it is safe
    return;
  }

  if (pCookieData->bMessageType == REQUESTTYPE_PROFILE)
  { 
    ReleaseCookie(dwCookie);

    // Read user info TLVs
    {
      oscar_tlv_chain* pChain;
      oscar_tlv* pTLV;
      BYTE *tmp;
      char *szMsg = NULL;

      // Syntax check
      if (wLen < 4)
        return;

      tmp = buf;
      // Get general chain
      if (!(pChain = readIntoTLVChain(&buf, wLen, wTLVCount)))
        return;

      disposeChain(&pChain);

      wLen -= (buf - tmp);

      // Get extra chain
      if (pChain = readIntoTLVChain(&buf, wLen, 2))
      {
        char* szEncoding = NULL;

        // Get Profile encoding TLV
        pTLV = getTLV(pChain, 0x01, 1);
        if (pTLV && (pTLV->wLen >= 1))
        {
          szEncoding = (char*)_alloca(pTLV->wLen + 1);
          memcpy(szEncoding, pTLV->pData, pTLV->wLen);
          szEncoding[pTLV->wLen] = '\0';
        }
        // Get Profile info TLV
        pTLV = getTLV(pChain, 0x02, 1);
        if (pTLV && (pTLV->wLen >= 1))
        {
          szMsg = malloc(pTLV->wLen + 2);
          memcpy(szMsg, pTLV->pData, pTLV->wLen);
          szMsg[pTLV->wLen] = '\0';
          szMsg[pTLV->wLen + 1] = '\0';
          szMsg = AimApplyEncoding(szMsg, szEncoding);
          szMsg = EliminateHtml(szMsg, pTLV->wLen);
        }
        // Free TLV chain
        disposeChain(&pChain);
      }

      ICQWriteContactSettingString(hContact, "About", szMsg);
      ICQBroadcastAck(hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE)1 ,0);

      SAFE_FREE(&szMsg);
    }
  }
  else
  {
    status = AwayMsgTypeToStatus(pCookieData->nAckType);
    if (status == ID_STATUS_OFFLINE)
    {
      NetLog_Server("SNAC(2.6) Ignoring unknown status message from %s", strUID(dwUIN, szUID));

      ReleaseCookie(dwCookie);
      return;
    }

    ReleaseCookie(dwCookie);

    // Read user info TLVs
    {
      oscar_tlv_chain* pChain;
      oscar_tlv* pTLV;
      BYTE *tmp;
      char *szMsg = NULL;
      CCSDATA ccs;
      PROTORECVEVENT pre;

      // Syntax check
      if (wLen < 4)
        return;

      tmp = buf;
      // Get general chain
      if (!(pChain = readIntoTLVChain(&buf, wLen, wTLVCount)))
        return;

      disposeChain(&pChain);

      wLen -= (buf - tmp);

      // Get extra chain
      if (pChain = readIntoTLVChain(&buf, wLen, 2))
      {
        char* szEncoding = NULL;

        // Get Away encoding TLV
        pTLV = getTLV(pChain, 0x03, 1);
        if (pTLV && (pTLV->wLen >= 1))
        {
          szEncoding = (char*)_alloca(pTLV->wLen + 1);
          memcpy(szEncoding, pTLV->pData, pTLV->wLen);
          szEncoding[pTLV->wLen] = '\0';
        }
        // Get Away info TLV
        pTLV = getTLV(pChain, 0x04, 1);
        if (pTLV && (pTLV->wLen >= 1))
        {
          szMsg = malloc(pTLV->wLen + 2);
          memcpy(szMsg, pTLV->pData, pTLV->wLen);
          szMsg[pTLV->wLen] = '\0';
          szMsg[pTLV->wLen + 1] = '\0';
          szMsg = AimApplyEncoding(szMsg, szEncoding);
          szMsg = EliminateHtml(szMsg, pTLV->wLen);
        }
        // Free TLV chain
        disposeChain(&pChain);
      }

      ccs.szProtoService = PSR_AWAYMSG;
      ccs.hContact = hContact;
      ccs.wParam = status;
      ccs.lParam = (LPARAM)&pre;
      pre.flags = 0;
      pre.szMessage = szMsg?szMsg:"";
      pre.timestamp = time(NULL);
      pre.lParam = dwCookie;

      CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);

      SAFE_FREE(&szMsg);
    }
  }
}
