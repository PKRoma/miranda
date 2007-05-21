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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/channels.h,v $
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#ifndef __CHANNELS_H
#define __CHANNELS_H

int unpackSnacHeader(snac_header* pSnacHeader, unsigned char **pBuffer, WORD* pwBufferLength);
void handleLoginChannel(unsigned char *buf, WORD datalen, serverthread_info *info);
void handleErrorChannel(unsigned char *buf, WORD datalen);
void handleDataChannel(unsigned char *buf, WORD wLen, serverthread_info *info);
void handlePingChannel(unsigned char *buf, WORD wLen);
void handleCloseChannel(unsigned char *buf, WORD datalen, serverthread_info *info);

void LogFamilyError(WORD wFamily, WORD wError);

void StartKeepAlive(serverthread_info* info);
void StopKeepAlive(serverthread_info* info);

#endif /* __CHANNELS_H */
