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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/oscar_filetransfer.c,v $
// Revision       : $Revision: 3474 $
// Last change on : $Date: 2006-08-08 01:05:47 +0200 (Ãºt, 08 VIII 2006) $
// Last change by : $Author: jokusoftware $
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"



void handleRecvServMsgOFT(unsigned char *buf, WORD wLen, DWORD dwUin, char *szUID, DWORD dwID1, DWORD dwID2, WORD wCommand)
{
  if (wCommand == 0)
  { // this is OFT request
    oscar_tlv_chain* chain = readIntoTLVChain(&buf, wLen, 0);

    if (chain)
    {
      // TODO: parse info, create filetransfer_oscar, add event
      disposeChain(&chain);
    }
    else
      NetLog_Server("Error: Missing TLV chain in OFT request");
  }
}
