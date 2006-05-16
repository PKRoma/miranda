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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/chan_05ping.c,v $
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



extern HANDLE hServerConn;
static HANDLE hKeepAliveEvent = NULL;


void handlePingChannel(unsigned char* buf, WORD datalen)
{
  NetLog_Server("Warning: Ignoring server packet on PING channel");
}



static void __cdecl icq_keepAliveThread(void* fa)
{
  icq_packet packet;

  NetLog_Server("Keep alive thread starting.");

  hKeepAliveEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

  for(;;)
  {
    DWORD dwWait = WaitForSingleObjectEx(hKeepAliveEvent, 57000, TRUE);

    if (dwWait == WAIT_OBJECT_0) break; // we should end
    else if (dwWait == WAIT_TIMEOUT)
    {
      // Send a keep alive packet to server
      packet.wLen = 0;
      write_flap(&packet, ICQ_PING_CHAN);
      if (hServerConn) // connection lost, end
        sendServPacket(&packet);
      else 
        break;
    }
    else if (dwWait == WAIT_IO_COMPLETION)
      // Possible shutdown in progress
      if (Miranda_Terminated()) break;
  }

  NetLog_Server("Keep alive thread shutting down.");

  CloseHandle(hKeepAliveEvent);
  hKeepAliveEvent = NULL;

  return;
}



void StartKeepAlive()
{
  if (hKeepAliveEvent) // start only once
    return;

  if (ICQGetContactSettingByte(NULL, "KeepAlive", 0))
    forkthread(icq_keepAliveThread, 0, NULL);
}



void StopKeepAlive()
{ // finish keep alive thread
  if (hKeepAliveEvent)
    SetEvent(hKeepAliveEvent);
}
