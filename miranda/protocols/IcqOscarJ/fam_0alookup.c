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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/fam_0alookup.c,v $
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


static void handleLookupEmailReply(BYTE* buf, WORD wLen, DWORD dwCookie);
static void ReleaseLookupCookie(DWORD dwCookie, search_cookie* pCookie);


void handleLookupFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader)
{
  switch (pSnacHeader->wSubtype)
  {

  case ICQ_LOOKUP_EMAIL_REPLY: // AIM search reply
    handleLookupEmailReply(pBuffer, wBufferLength, pSnacHeader->dwRef);
    break;

  case ICQ_ERROR:
  { 
    WORD wError;
    search_cookie* pCookie;

    if (wBufferLength >= 2)
      unpackWord(&pBuffer, &wError);
    else 
      wError = 0;

    if (FindCookie(pSnacHeader->dwRef, NULL, &pCookie))
    {
      if (wError == 0x14)
        NetLog_Server("Lookup: No results");

      ReleaseLookupCookie(pSnacHeader->dwRef, pCookie);

      if (wError == 0x14) return;
    }

    LogFamilyError(ICQ_LOOKUP_FAMILY, wError);
    break;
  }

  default:
    NetLog_Server("Warning: Ignoring SNAC(x%02x,x%02x) - Unknown SNAC (Flags: %u, Ref: %u)", ICQ_LOOKUP_FAMILY, pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;
  }
}



static void ReleaseLookupCookie(DWORD dwCookie, search_cookie* pCookie)
{
  FreeCookie(dwCookie);
  SAFE_FREE(&pCookie->szObject);

  if (pCookie->dwMainId && !pCookie->dwStatus)
  { // we need to wait for main search
    pCookie->dwStatus = 1;
  }
  else
  { // finish everything
    if (pCookie->dwMainId)
      ICQBroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)pCookie->dwMainId, 0);
    else // we are single
      ICQBroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)dwCookie, 0);

    SAFE_FREE(&pCookie);
  }
}



void handleLookupEmailReply(BYTE* buf, WORD wLen, DWORD dwCookie)
{
  ICQSEARCHRESULT sr = {0};
  oscar_tlv_chain* pChain;
  search_cookie* pCookie;
  int i;

  if (!FindCookie(dwCookie, NULL, &pCookie))
  {
    NetLog_Server("Error: Received unexpected lookup reply");
    return;
  }
  else
    NetLog_Server("SNAC(0x0A,0x3): Lookup reply");

  sr.hdr.cbSize = sizeof(sr);
  sr.hdr.email = pCookie->szObject;

  // Syntax check, read chain
  if (wLen >= 4 && (pChain = readIntoTLVChain(&buf, wLen, 0)))
  {
    for (i = 1; TRUE; i++)
    { // collect the results
      sr.hdr.nick = getStrFromChain(pChain, 1, (WORD)i);
      if (!sr.hdr.nick) break;
      sr.uid = sr.hdr.nick;
      // broadcast the result
      if (pCookie->dwMainId)
        ICQBroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)pCookie->dwMainId, (LPARAM)&sr);
      else
        ICQBroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)dwCookie, (LPARAM)&sr);
      SAFE_FREE(&sr.hdr.nick);
    }
    disposeChain(&pChain);
  }

  ReleaseLookupCookie(dwCookie, pCookie);
}
