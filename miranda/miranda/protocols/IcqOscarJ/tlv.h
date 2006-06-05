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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/tlv.h,v $
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#ifndef __TLV_H
#define __TLV_H

/*---------* Structures *--------------*/

typedef struct oscar_tlv_s
{
  WORD wType;
  WORD wLen;
  BYTE *pData;
} oscar_tlv;

typedef struct oscar_tlv_chain_s
{
  oscar_tlv tlv;
  struct oscar_tlv_chain_s *next;
} oscar_tlv_chain;

/*---------* Functions *---------------*/

oscar_tlv_chain* readIntoTLVChain(BYTE **buf, WORD wLen, int maxTlvs);
oscar_tlv* getTLV(oscar_tlv_chain *, WORD, WORD);

WORD getLenFromChain(oscar_tlv_chain* chain, WORD wType, WORD wIndex);
DWORD getDWordFromChain(oscar_tlv_chain* chain, WORD wType, WORD wIndex);
WORD  getWordFromChain(oscar_tlv_chain* chain, WORD wType, WORD wIndex);
BYTE getByteFromChain(oscar_tlv_chain* chain, WORD wType, WORD wIndex);
BYTE *getStrFromChain(oscar_tlv_chain* chain, WORD wType, WORD wIndex);

void disposeChain(oscar_tlv_chain** chain);

#endif /* __TLV_H */
