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
//  Contains global variables declarations.
//
// -----------------------------------------------------------------------------


#ifndef __GLOBALS_H
#define __GLOBALS_H

typedef char uid_str[MAX_PATH];

// from init.c
HINSTANCE hInst;
char gpszICQProtoName[MAX_PATH];

HANDLE ghServerNetlibUser;
HANDLE ghDirectNetlibUser;

// from init.h
BYTE gbGatewayMode;
BYTE gbSecureLogin;
BYTE gbAimEnabled;
BYTE gbUtfEnabled;
WORD gwAnsiCodepage;
BYTE gbDCMsgEnabled;
BYTE gbTempVisListEnabled;
BYTE gbSsiEnabled;
BYTE gbAvatarsEnabled;
BYTE gbXStatusEnabled;
DWORD MIRANDA_VERSION;

// from icqosc_svcs.c
int gnCurrentStatus;
DWORD dwLocalUIN;

char gpszPassword[16];
BYTE gbRememberPwd;

BYTE gbUnicodeAPI;
BYTE gbUnicodeCore;

// from fam_04message.c
typedef struct icq_mode_messages_s
{
  char* szAway;
  char* szNa;
  char* szDnd;
  char* szOccupied;
  char* szFfc;
} icq_mode_messages;

icq_mode_messages modeMsgs;
CRITICAL_SECTION modeMsgsMutex;


#endif /* __GLOBALS_H */
