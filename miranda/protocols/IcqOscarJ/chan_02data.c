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
//  Handle channel 2 (Data) packets
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


void handleDataChannel(unsigned char *pBuffer, WORD wBufferLength)
{
	snac_header* pSnacHeader = NULL;


	pSnacHeader = unpackSnacHeader(&pBuffer, &wBufferLength);

	if (!pSnacHeader || !pSnacHeader->bValid)
	{
		NetLog_Server("Error: Failed to parse SNAC header");
	}
	else
	{
#ifdef _DEBUG
		NetLog_Server(" Received SNAC(x%02X,x%02X)", pSnacHeader->wFamily, pSnacHeader->wSubtype);
#endif

		
		switch (pSnacHeader->wFamily)
		{
			
		case ICQ_SERVICE_FAMILY:
			handleServiceFam(pBuffer, wBufferLength, pSnacHeader);
			break;
			
		case ICQ_LOCATION_FAMILY:
			handleLocationFam(pBuffer, wBufferLength, pSnacHeader);
			break;
			
		case ICQ_BUDDY_FAMILY:
			handleBuddyFam(pBuffer, wBufferLength, pSnacHeader);
			break;
			
		case ICQ_MSG_FAMILY:
			handleMsgFam(pBuffer, wBufferLength, pSnacHeader);
			break;
			
		case ICQ_BOS_FAMILY:
			handleBosFam(pBuffer, wBufferLength, pSnacHeader);
			break;
			
		case ICQ_STATS_FAMILY:
			handleStatusFam(pBuffer, wBufferLength, pSnacHeader);
			break;
			
		case ICQ_LISTS_FAMILY:
			handleServClistFam(pBuffer, wBufferLength, pSnacHeader);
			break;
			
		case ICQ_EXTENSIONS_FAMILY:
			handleIcqExtensionsFam(pBuffer, wBufferLength, pSnacHeader);
			break;
			
		default:
			NetLog_Server("Ignoring SNAC(x%02X,x%02X) - FAMILYx%02X not implemented", pSnacHeader->wFamily, pSnacHeader->wSubtype, pSnacHeader->wFamily);
			break;

		}
  }
	
	// Clean up and exit
	SAFE_FREE(&pSnacHeader);
}


snac_header* unpackSnacHeader(unsigned char **pBuffer, WORD* pwBufferLength)
{
	snac_header* pSnacHeader = NULL;
  WORD wRef1, wRef2;


	// Create a empty header
	if (!(pSnacHeader = calloc(1, sizeof(snac_header))))
		return FALSE;

	// 10 bytes is the minimum size of a header
	if (*pwBufferLength < 10)
	{
		// Buffer overflow
		pSnacHeader->bValid = FALSE;
		return pSnacHeader;
	}

	// Unpack all the standard data
	unpackWord(pBuffer,  &(pSnacHeader->wFamily));
	unpackWord(pBuffer,  &(pSnacHeader->wSubtype));
	unpackWord(pBuffer,  &(pSnacHeader->wFlags));
  unpackWord(pBuffer,  &wRef1); // unpack reference id (sequence)
  unpackWord(pBuffer,  &wRef2); // command
  pSnacHeader->dwRef = wRef1 | (wRef2<<0x10);

	//unpackDWord(pBuffer, &(pSnacHeader->dwRef));
	*pwBufferLength -= 10;
	
	// If flag bit 15 is set, we also have a version tag
	// (...at least that is what I think it is)
	if (pSnacHeader->wFlags & 0x8000)
	{
		if (*pwBufferLength >= 2)
		{
			WORD wExtraBytes = 0;
			
			unpackWord(pBuffer, &wExtraBytes);
			*pwBufferLength -= 2;
			
			if (*pwBufferLength >= wExtraBytes)
			{
				if (wExtraBytes == 6)
				{
					*pBuffer += 4; // TLV type and length?
					unpackWord(pBuffer,  &(pSnacHeader->wVersion));
					*pwBufferLength -= 6;
					pSnacHeader->bValid = TRUE;
				}
				else
				{
					*pwBufferLength -= wExtraBytes;
					*pBuffer += wExtraBytes;
					pSnacHeader->bValid = TRUE;
				}
			}
			else
			{
				// Buffer overflow
				pSnacHeader->bValid = FALSE;
			}
			
		}
		else
		{
			// Buffer overflow
			pSnacHeader->bValid = FALSE;
		}
	}
	else
	{
		pSnacHeader->bValid = TRUE;
	}

	return pSnacHeader;
}


void LogFamilyError(WORD wFamily, WORD wError)
{
	char *msg;

	switch(wError) 
  {
	  case 0x01: msg = "Invalid SNAC header"; break;
	  case 0x02: msg = "Server rate limit exceeded"; break;
	  case 0x03: msg = "Client rate limit exceeded"; break;
	  case 0x04: msg = "Recipient is not logged in"; break;
	  case 0x05: msg = "Requested service unavailable"; break;
	  case 0x06: msg = "Requested service not defined"; break;
	  case 0x07: msg = "You sent obsolete SNAC"; break;
	  case 0x08: msg = "Not supported by server"; break;
	  case 0x09: msg = "Not supported by client"; break;
	  case 0x0A: msg = "Refused by client"; break;
	  case 0x0B: msg = "Reply too big"; break;
	  case 0x0C: msg = "Responses lost"; break;
	  case 0x0D: msg = "Request denied"; break;
	  case 0x0E: msg = "Incorrect SNAC format"; break;
	  case 0x0F: msg = "Insufficient rights"; break;
	  case 0x10: msg = "In local permit/deny (recipient blocked)"; break;
	  case 0x11: msg = "Sender too evil"; break;
	  case 0x12: msg = "Receiver too evil"; break;
	  case 0x13: msg = "User temporarily unavailable"; break;
	  case 0x14: msg = "No match"; break;
	  case 0x15: msg = "List overflow"; break;
	  case 0x16: msg = "Request ambiguous"; break;
	  case 0x17: msg = "Server queue full"; break;
	  case 0x18: msg = "Not while on AOL"; break;
	  default:	 msg = ""; break;
	}

	NetLog_Server("SNAC(x%02X,x01) - Error(%u): %s", wFamily, wError, msg);
}