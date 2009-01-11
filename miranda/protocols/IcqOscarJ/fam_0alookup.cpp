// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001-2002 Jon Keating, Richard Hughes
// Copyright © 2002-2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004-2008 Joe Kucera
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
// File name      : $URL$
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

void CIcqProto::handleLookupFam(BYTE *pBuffer, WORD wBufferLength, snac_header* pSnacHeader)
{
	switch (pSnacHeader->wSubtype) {

	case ICQ_LOOKUP_EMAIL_REPLY: // AIM search reply
		handleLookupEmailReply(pBuffer, wBufferLength, pSnacHeader->dwRef);
		break;

	case ICQ_ERROR:
		{ 
			WORD wError;
			cookie_search *pCookie;

			if (wBufferLength >= 2)
				unpackWord(&pBuffer, &wError);
			else 
				wError = 0;

			if (FindCookie(pSnacHeader->dwRef, NULL, (void**)&pCookie))
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

void CIcqProto::ReleaseLookupCookie(DWORD dwCookie, cookie_search *pCookie)
{
	FreeCookie(dwCookie);
	SAFE_FREE((void**)&pCookie->szObject);

	if (pCookie->dwMainId && !pCookie->dwStatus)
	{ // we need to wait for main search
		pCookie->dwStatus = 1;
	}
	else
	{ // finish everything
		if (pCookie->dwMainId)
			BroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)pCookie->dwMainId, 0);
		else // we are single
			BroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)dwCookie, 0);

		SAFE_FREE((void**)&pCookie);
	}
}

void CIcqProto::handleLookupEmailReply(BYTE* buf, WORD wLen, DWORD dwCookie)
{
	ICQSEARCHRESULT sr = {0};
	oscar_tlv_chain* pChain;
	cookie_search *pCookie;

	if (!FindCookie(dwCookie, NULL, (void**)&pCookie))
	{
		NetLog_Server("Error: Received unexpected lookup reply");
		return;
	}
	
	NetLog_Server("SNAC(0x0A,0x3): Lookup reply");

	sr.hdr.cbSize = sizeof(sr);
	sr.hdr.email = pCookie->szObject;

	// Syntax check, read chain
	if (wLen >= 4 && (pChain = readIntoTLVChain(&buf, wLen, 0)))
	{
		for (WORD i = 1; TRUE; i++)
		{ // collect the results
			sr.hdr.nick = pChain->getString(0x01, i);
			if (!sr.hdr.nick) break;
			sr.uid = sr.hdr.nick;
			// broadcast the result
			if (pCookie->dwMainId)
				BroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)pCookie->dwMainId, (LPARAM)&sr);
			else
				BroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)dwCookie, (LPARAM)&sr);
			SAFE_FREE((void**)&sr.hdr.nick);
		}
		disposeChain(&pChain);
	}

	ReleaseLookupCookie(dwCookie, pCookie);
}
