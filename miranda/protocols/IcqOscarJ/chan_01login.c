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



extern int isLoginServer;
extern HANDLE ghServerNetlibUser;
extern HANDLE hDirectNetlibUser;
extern DWORD dwLocalUIN;
extern WORD wLocalSequence;

char* cookieData;
int cookieDataLen = 0;



void handleLoginChannel(unsigned char *buf, WORD datalen, serverthread_start_info *info)
{
	icq_packet packet;
	WORD wUinLen;
	char szUin[10];


	// isLoginServer is "1" if we just received SRV_HELLO
	if (isLoginServer)
	{
#ifdef _DEBUG
		Netlib_Logf(ghServerNetlibUser, "Received SRV_HELLO from login server");
#endif
		mir_snprintf(szUin, sizeof(szUin), "%d", dwLocalUIN);
		wUinLen = strlen(szUin);

		packet.wLen = 66 + strlen(CLIENT_ID_STRING) + wUinLen + info->wPassLen;

		write_flap(&packet, ICQ_LOGIN_CHAN);
		packDWord(&packet, 0x00000001);

		packTLV(&packet, 0x0001, wUinLen, szUin);
		packTLV(&packet, 0x0002, info->wPassLen, info->szEncPass);


		// Pack client identification details. We identify ourselves as icq5
		packTLV(&packet, 0x0003, (WORD)strlen(CLIENT_ID_STRING), CLIENT_ID_STRING); // Client ID string
		packTLVWord(&packet, 0x0016, 0x010a);               // Client ID
		packTLVWord(&packet, 0x0017, 0x0014);               // Client major version
		packTLVWord(&packet, 0x0018, 0x0022);               // Client minor version
		packTLVWord(&packet, 0x0019, 0x0000);               // Client lesser version
		packTLVWord(&packet, 0x001a, 0x0911);               // Client build number
		packTLVDWord(&packet, 0x0014, 0x0000043d);          // Client distribution number
		packTLV(&packet, 0x000f, 0x0002, "en");             // Client language
		packTLV(&packet, 0x000e, 0x0002, "us");             // Client country

		sendServPacket(&packet);

#ifdef _DEBUG
		Netlib_Logf(ghServerNetlibUser, "Sent CLI_IDENT to login server");
#endif

		isLoginServer = 0;
		cookieDataLen = 0;
	}
	else 
	{
		if (cookieDataLen)
		{
			packet.wLen = cookieDataLen + 8;
			wLocalSequence = (WORD)RandRange(0, 0xffff);
			
			write_flap(&packet, 1);
			packDWord(&packet, 0x00000001);
			packTLV(&packet, 0x06, (WORD)cookieDataLen, cookieData);
			
			sendServPacket(&packet);
			
#ifdef _DEBUG
			Netlib_Logf(ghServerNetlibUser, "Sent CLI_IDENT to communication server");
#endif
			
			SAFE_FREE(&cookieData);
			cookieDataLen = 0;
		}
		else
		{
			// We need a cookie to identify us to the communication server
			Netlib_Logf(ghServerNetlibUser, "Something went wrong...");
		}
	}
}
