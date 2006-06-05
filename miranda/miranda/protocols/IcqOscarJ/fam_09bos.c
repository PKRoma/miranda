// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006 Joe Kucera
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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/fam_09bos.c,v $
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



static void handlePrivacyRightsReply(unsigned char *pBuffer, WORD wBufferLength);

void handleBosFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader)
{
  switch (pSnacHeader->wSubtype)
  {

  case ICQ_PRIVACY_RIGHTS_REPLY: // Reply to CLI_REQBOS
    handlePrivacyRightsReply(pBuffer, wBufferLength);
    break;

  case ICQ_ERROR:
  {
    WORD wError;

    if (wBufferLength >= 2)
      unpackWord(&pBuffer, &wError);
    else 
      wError = 0;

    LogFamilyError(ICQ_BOS_FAMILY, wError);
    break;
  }

  default:
    NetLog_Server("Warning: Ignoring SNAC(x%02x,x%02x) - Unknown SNAC (Flags: %u, Ref: %u)", ICQ_BOS_FAMILY, pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;

  }
}



static void handlePrivacyRightsReply(unsigned char *pBuffer, WORD wBufferLength)
{
  if (wBufferLength >= 12)
  {
    oscar_tlv_chain* pChain;


    pChain = readIntoTLVChain(&pBuffer, wBufferLength, 0);

    if (pChain)
    {
      WORD wMaxVisibleContacts;
      WORD wMaxInvisibleContacts;


      wMaxVisibleContacts = getWordFromChain(pChain, 0x0001, 1);
      wMaxInvisibleContacts = getWordFromChain(pChain, 0x0001, 1);

      disposeChain(&pChain);

      NetLog_Server("PRIVACY: Max visible %u, max invisible %u items.", wMaxVisibleContacts, wMaxInvisibleContacts);

      // Success
      return;
    }
  }

  // Failure
  NetLog_Server("Warning: Malformed SRV_PRIVACY_RIGHTS_REPLY");
}



void makeContactTemporaryVisible(HANDLE hContact)
{
  DWORD dwUin;
  uid_str szUid;

  if (ICQGetContactSettingByte(hContact, "TemporaryVisible", 0))
    return; // already there

  if (ICQGetContactSettingUID(hContact, &dwUin, &szUid))
    return; // Invalid contact

  icq_sendGenericContact(dwUin, szUid, ICQ_BOS_FAMILY, ICQ_CLI_ADDTEMPVISIBLE);

  ICQWriteContactSettingByte(hContact, "TemporaryVisible", 1);

#ifdef _DEBUG
  NetLog_Server("Added contact %s to temporary visible list", strUID(dwUin, szUid));
#endif
}



void clearTemporaryVisibleList()
{ // remove all temporary visible contacts
  sendEntireListServ(ICQ_BOS_FAMILY, ICQ_CLI_REMOVETEMPVISIBLE, BUL_TEMPVISIBLE);
}
