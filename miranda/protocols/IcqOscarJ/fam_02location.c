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


static void handleLocationAwayReply(BYTE* buf, WORD wLen, WORD wCookie);


void handleLocationFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader)
{
  switch (pSnacHeader->wSubtype)
  {

  case ICQ_LOCATION_RIGHTS_REPLY: // Reply to CLI_REQLOCATION
    NetLog_Server("Server sent SNAC(x02,x03) - SRV_LOCATION_RIGHTS_REPLY");
    break;

  case ICQ_LOCATION_USR_INFO_REPLY: // AIM away reply
    handleLocationAwayReply(pBuffer, wBufferLength, (WORD)pSnacHeader->dwRef);
    break;

  case ICQ_ERROR:
  { 
    WORD wError;

    if (wBufferLength >= 2)
      unpackWord(&pBuffer, &wError);
    else 
      wError = 0;

    LogFamilyError(ICQ_LOCATION_FAMILY, wError);
    break;
  }

  default:
    NetLog_Server("Warning: Ignoring SNAC(x%02x,x%02x) - Unknown SNAC (Flags: %u, Ref: %u)", ICQ_LOCATION_FAMILY, pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;
  }
}


void handleLocationAwayReply(BYTE* buf, WORD wLen, WORD wCookie)
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
  if (dwUIN)
    hContact = HContactFromUIN(dwUIN, NULL);
  else
    hContact = HContactFromUID(szUID, NULL);

  // Ignore away status if the user is not already on our list
  if (hContact == INVALID_HANDLE_VALUE)
  {
#ifdef _DEBUG
    NetLog_Server("Ignoring away reply (%u)", dwUIN);
#endif
    return;
  }

  if (!FindCookie(wCookie, &dwCookieUin, &pCookieData))
  {
    NetLog_Server("Error: Received unexpected away reply from %u", dwUIN);
    return;
  }

  if (dwUIN != dwCookieUin)
  {
    NetLog_Server("Error: Away reply UIN does not match Cookie UIN(%u != %u)", dwUIN, dwCookieUin);
    FreeCookie(wCookie);
    SAFE_FREE(&pCookieData); // This could be a bad idea, but I think it is safe
    return;
  }

  status = AwayMsgTypeToStatus(pCookieData->nAckType);
  if (status == ID_STATUS_OFFLINE)
  {
    NetLog_Server("SNAC(2.6) Ignoring unknown status message from %u", dwUIN);
    FreeCookie(wCookie);
    SAFE_FREE(&pCookieData);
    return;
  }

  FreeCookie(wCookie);
  SAFE_FREE(&pCookieData);

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
      // Get Away info TLV
      pTLV = getTLV(pChain, 0x04, 1);
      if (pTLV && (pTLV->wLen >= 1))
      {
        szMsg = malloc(pTLV->wLen + 1);
        memcpy(szMsg, pTLV->pData, pTLV->wLen);
        szMsg[pTLV->wLen] = '\0';
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
    pre.lParam = wCookie;

    CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);

    SAFE_FREE(&szMsg);
  }
}
