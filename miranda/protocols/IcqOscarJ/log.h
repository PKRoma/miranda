// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001,2002 Jon Keating, Richard Hughes
// Copyright � 2002,2003,2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004,2005,2006 Joe Kucera
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

#ifndef __LOG_H
#define __LOG_H

#define LOG_NOTE       0   //trivial problems or problems that will already have been reported elsewhere
#define LOG_WARNING    1   //problems that may have caused data loss
#define LOG_ERROR      2   //problems that cause a disconnection from the network
#define LOG_FATAL      3   //problems requiring user intervention: password wrong, rate exceeded, etc.

/*---------* Functions *---------------*/

void icq_LogMessage(int level, const char *szMsg);
void icq_LogUsingErrorCode(int level, DWORD dwError, const char *szMsg);  //szMsg is optional
void icq_LogFatalParam(const char* szMsg, WORD wError);

#endif /* __LOG_H */
