// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
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

#ifndef __ICQ_PACKET_H
#define __ICQ_PACKET_H

typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned long	DWORD;

/*---------* Structures *--------------*/

typedef struct icq_packet_s
{
	WORD wPlace;
	BYTE nChannel;
	WORD wSequence;
	WORD wLen;
	BYTE *pData;
} icq_packet;

/*---------* Functions *---------------*/

void write_httphdr(icq_packet *d, WORD wType);
void write_flap(icq_packet *, BYTE);
void directPacketInit(icq_packet *, DWORD);

void packByte(icq_packet *, BYTE);
void packWord(icq_packet *, WORD);
void packDWord(icq_packet *, DWORD);
void packTLV(icq_packet* pPacket, WORD wType, WORD wLength, BYTE* pbyValue);
void packTLVWord(icq_packet *d, unsigned short nType, WORD wData);
void packTLVDWord(icq_packet *d, unsigned short nType, DWORD dwData);

void packBuffer(icq_packet* pPacket, const BYTE* pbyBuffer, WORD wLength);
//void packLEWordSizedBuffer(icq_packet* pPacket, const BYTE* pbyBuffer, WORD wLength);
void packFNACHeader(icq_packet *d, WORD wFamily, WORD wSubtype, WORD wFlags, DWORD wSeq);

void packLEWord(icq_packet *, WORD);
void packLEDWord(icq_packet *, DWORD);

void unpackByte(unsigned char **, BYTE *);
void unpackWord(unsigned char **, WORD *);
void unpackDWord(unsigned char **, DWORD *);
void unpackString(unsigned char **buf, char *string, WORD len);
void unpackWideString(unsigned char **buf, WCHAR *string, WORD len);
void unpackTLV(unsigned char **, WORD *, WORD *, char **);
BOOL unpackUID(unsigned char** ppBuf, WORD* pwLen, char** ppszUID);

void unpackLEWord(unsigned char **, WORD *);
void unpackLEDWord(unsigned char **, DWORD *);

#endif /* __ICQ_PACKET_H */