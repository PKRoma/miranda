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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/fam_17signon.c,v $
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



static void handleAuthKeyResponse(BYTE *buf, WORD wPacketLen, serverthread_info *info);


void handleAuthorizationFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader, serverthread_info *info)
{
  switch (pSnacHeader->wSubtype)
  {

  case ICQ_SIGNON_ERROR:
  {
    WORD wError;

    if (wBufferLength >= 2)
      unpackWord(&pBuffer, &wError);
    else 
      wError = 0;

    LogFamilyError(ICQ_AUTHORIZATION_FAMILY, wError);
    break;
  }

  case ICQ_SIGNON_AUTH_KEY:
    handleAuthKeyResponse(pBuffer, wBufferLength, info);
    break;

  case ICQ_SIGNON_LOGIN_REPLY:
    handleLoginReply(pBuffer, wBufferLength, info);
    break;

  default:
    NetLog_Server("Warning: Ignoring SNAC(x%02x,x%02x) - Unknown SNAC (Flags: %u, Ref: %u)", ICQ_AUTHORIZATION_FAMILY, pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;
  }
}



static void icq_encryptPassword(const char* szPassword, unsigned char* encrypted)
{
  unsigned int i;
  unsigned char table[] =
  {
    0xf3, 0x26, 0x81, 0xc4,
    0x39, 0x86, 0xdb, 0x92,
    0x71, 0xa3, 0xb9, 0xe6,
    0x53, 0x7a, 0x95, 0x7c
  };

  for (i = 0; szPassword[i]; i++)
  {
    encrypted[i] = (szPassword[i] ^ table[i % 16]);
  }
}



void sendClientAuth(const char* szKey, WORD wKeyLen, BOOL bSecure)
{
  char szUin[UINMAXLEN];
  WORD wUinLen;
  icq_packet packet;

  wUinLen = strlennull(strUID(dwLocalUIN, szUin));

  packet.wLen = 65 + sizeof(CLIENT_ID_STRING) + wUinLen + wKeyLen;

  if (bSecure)
  {
    serverPacketInit(&packet, (WORD)(packet.wLen + 10));
    packFNACHeaderFull(&packet, ICQ_AUTHORIZATION_FAMILY, ICQ_SIGNON_LOGIN_REQUEST, 0, 0);
  }
  else
  {
    write_flap(&packet, ICQ_LOGIN_CHAN);
    packDWord(&packet, 0x00000001);
  }
  packTLV(&packet, 0x0001, wUinLen, szUin);

  if (bSecure)
  { // Pack MD5 auth digest
    packTLV(&packet, 0x0025, wKeyLen, (BYTE*)szKey);
    packDWord(&packet, 0x004C0000); // empty TLV(0x4C): unknown
  }
  else
  { // Pack old style password hash
    char hash[20];

    icq_encryptPassword(szKey, hash);
    packTLV(&packet, 0x0002, wKeyLen, hash);
  }

  // Pack client identification details. We identify ourselves as icq5.1 english
  packTLV(&packet, 0x0003, (WORD)sizeof(CLIENT_ID_STRING)-1, CLIENT_ID_STRING); // Client ID string
  packTLVWord(&packet, 0x0016, 0x010a);               // Client ID
  packTLVWord(&packet, 0x0017, 0x0014);               // Client major version
  packTLVWord(&packet, 0x0018, 0x0034);               // Client minor version
  packTLVWord(&packet, 0x0019, 0x0000);               // Client lesser version
  packTLVWord(&packet, 0x001a, 0x0bb8);               // Client build number
  packTLVDWord(&packet, 0x0014, 0x0000043d);          // Client distribution number
  packTLV(&packet, 0x000f, 0x0002, "en");             // Client language
  packTLV(&packet, 0x000e, 0x0002, "us");             // Client country

  sendServPacket(&packet);
}



static void handleAuthKeyResponse(BYTE *buf, WORD wPacketLen, serverthread_info *info)
{
  WORD wKeyLen;
  char szKey[64] = {0};
	md5_state_t state;
  md5_byte_t digest[16];

#ifdef _DEBUG
  NetLog_Server("Received %s", "ICQ_SIGNON_AUTH_KEY");
#endif

  if (wPacketLen < 2) 
  {
    NetLog_Server("Malformed %s", "ICQ_SIGNON_AUTH_KEY");
    icq_LogMessage(LOG_FATAL, "Secure login failed.\nInvalid server response.");
    SetCurrentStatus(ID_STATUS_OFFLINE);
    return;
  }

  unpackWord(&buf, &wKeyLen);
  wPacketLen -= 2;

  if (!wKeyLen || wKeyLen > wPacketLen || wKeyLen > sizeof(szKey)) 
  {
    NetLog_Server("Invalid length in %s: %u", "ICQ_SIGNON_AUTH_KEY", wKeyLen);
    icq_LogMessage(LOG_FATAL, "Secure login failed.\nInvalid key length.");
    SetCurrentStatus(ID_STATUS_OFFLINE);
    return;
  }

  unpackString(&buf, szKey, wKeyLen);

  {
    char *pwd = info->szAuthKey;

    md5_init(&state);
    md5_append(&state, (const md5_byte_t*)pwd, info->wAuthKeyLen);
    md5_finish(&state, digest);
  }

  md5_init(&state);
	md5_append(&state, szKey, wKeyLen);
	md5_append(&state, digest, 16);
	md5_append(&state, CLIENT_MD5_STRING, sizeof(CLIENT_MD5_STRING)-1);
	md5_finish(&state, digest);

#ifdef _DEBUG
	NetLog_Server("Sending ICQ_SIGNON_LOGIN_REQUEST to login server");
#endif
  sendClientAuth(digest, 0x10, TRUE);
}
