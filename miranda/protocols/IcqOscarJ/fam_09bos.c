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
		Netlib_Logf(ghServerNetlibUser, "Warning: Ignoring SNAC(x09,x%02x) - Unknown SNAC (Flags: %u, Ref: %u", pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
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

			Netlib_Logf(ghServerNetlibUser, "SRV_PRIVACY_RIGHTS_REPLY: Max visible %u, max invisible %u", wMaxVisibleContacts, wMaxInvisibleContacts);

			// Success
			return;
		}
	}

	// Failure
	Netlib_Logf(ghServerNetlibUser, "Warning: Malformed SRV_PRIVACY_RIGHTS_REPLY");
}


void makeContactTemporaryVisible(HANDLE hContact)
{
  icq_packet packet;
  char szUin[UINMAXLEN];
  int nUinLen;

  if (DBGetContactSettingByte(hContact, gpszICQProtoName, "TemporaryVisible", 0))
    return; // already there

  ltoa(DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0), szUin, 10);
  nUinLen = strlen(szUin);

  packet.wLen = nUinLen + 11;
  write_flap(&packet, 2);
  packFNACHeader(&packet, ICQ_BOS_FAMILY, ICQ_CLI_ADDTEMPVISIBLE, 0, ICQ_CLI_ADDTEMPVISIBLE<<0x10);
  packByte(&packet, (BYTE)nUinLen);
  packBuffer(&packet, szUin, (WORD)nUinLen);
  sendServPacket(&packet);

  DBWriteContactSettingByte(hContact, gpszICQProtoName, "TemporaryVisible", 1);

#ifdef _DEBUG
  Netlib_Logf(ghServerNetlibUser, "Added contact %u to temporary visible list");
#endif
}


static char* buildTempVisUinList()
{
	char* szList;
	char* szProto;
	HANDLE hContact;
	WORD wCurrentLen = 0;
	DWORD dwUIN;
	char szUin[UINMAXLEN];
	char szLen[2];


	szList = (char*)calloc(CallService(MS_DB_CONTACT_GETCOUNT, 0, 0), UINMAXLEN);
	szLen[1] = '\0';

	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

	while(hContact != NULL)
	{
		szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (szProto != NULL && !strcmp(szProto, gpszICQProtoName))
		{
			if (DBGetContactSettingByte(hContact, gpszICQProtoName, "TemporaryVisible", 0)
        && (dwUIN = DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0)))
			{
				_itoa(dwUIN, szUin, 10);
				szLen[0] = strlen(szUin);

        wCurrentLen += szLen[0] + 1;

				strcat(szList, szLen); // add to list
				strcat(szList, szUin);

        // clear flag
        DBDeleteContactSetting(hContact, gpszICQProtoName, "TemporaryVisible");
			}
		}

		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}

	return szList;
}


void clearTemporaryVisibleList()
{ // browse clist, remove all temporary visible contacts
	char* szList;
	int nListLen;
	icq_packet packet;


	szList = buildTempVisUinList();
	nListLen = strlennull(szList);

  if (nListLen)
  {
    packet.wLen = nListLen + 10;
	  write_flap(&packet, 2);
	  packFNACHeader(&packet, ICQ_BOS_FAMILY, ICQ_CLI_REMOVETEMPVISIBLE, 0, ICQ_CLI_REMOVETEMPVISIBLE<<0x10);
		packBuffer(&packet, szList, (WORD)nListLen);
		sendServPacket(&packet);
  }

	SAFE_FREE(&szList);
}