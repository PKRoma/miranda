// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004,2005 Martin Öberg, Sam Kothari, Robert Rainwater
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


extern HANDLE ghServerNetlibUser, hDirectNetlibUser;

void handleDataChannel(unsigned char *pBuffer, WORD wBufferLength)
{

	snac_header* pSnacHeader = NULL;


	pSnacHeader = unpackSnacHeader(&pBuffer, &wBufferLength);

	if (!pSnacHeader || !pSnacHeader->bValid)
	{
		Netlib_Logf(ghServerNetlibUser, "Error: Failed to parse SNAC header");
	}
	else
	{
#ifdef _DEBUG
		Netlib_Logf(ghServerNetlibUser, " Received SNAC(x%02X,x%02X)", pSnacHeader->wFamily, pSnacHeader->wSubtype);
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
			
		case ICQ_STATUS_FAMILY:
			handleStatusFam(pBuffer, wBufferLength, pSnacHeader);
			break;
			
		case ICQ_LISTS_FAMILY:
			handleServClistFam(pBuffer, wBufferLength, pSnacHeader);
			break;
			
		case ICQ_EXTENSIONS_FAMILY:
			handleIcqExtensionsFam(pBuffer, wBufferLength, pSnacHeader);
			break;
			
		default:
			Netlib_Logf(ghServerNetlibUser, "Ignoring SNAC(x%02X,x%02X) - FAMILYx%02X not implemented", pSnacHeader->wFamily, pSnacHeader->wSubtype, pSnacHeader->wFamily);
			break;

		}
		
	}
	
	// Clean up and exit
	SAFE_FREE(pSnacHeader);

}


snac_header* unpackSnacHeader(unsigned char **pBuffer, WORD* pwBufferLength)
{

	snac_header* pSnacHeader = NULL;


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
	unpackDWord(pBuffer, &(pSnacHeader->dwRef));
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
