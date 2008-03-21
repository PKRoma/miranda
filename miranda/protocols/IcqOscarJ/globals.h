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
extern HINSTANCE hInst;
extern char gpszICQProtoName[MAX_PATH];

extern HANDLE ghServerNetlibUser;
extern HANDLE ghDirectNetlibUser;

// from init.h
extern const int moodXStatus[];

extern BYTE gbGatewayMode;
extern BYTE gbSecureLogin;
extern BYTE gbAimEnabled;
extern BYTE gbUtfEnabled;
extern WORD gwAnsiCodepage;
extern BYTE gbDCMsgEnabled;
extern BYTE gbTempVisListEnabled;
extern BYTE gbSsiEnabled;
extern BYTE gbSsiSimpleGroups;
extern BYTE gbAvatarsEnabled;
extern BYTE gbXStatusEnabled;
extern DWORD MIRANDA_VERSION;

// from icqosc_svcs.c
extern int gnCurrentStatus;
extern DWORD dwLocalUIN;

extern char gpszPassword[16];
extern BYTE gbRememberPwd;

extern BYTE gbUnicodeCore;

// from fam_04message.c
struct icq_mode_messages
{
  char *szAway;
  char *szNa;
  char *szDnd;
  char *szOccupied;
  char *szFfc;
};

extern icq_mode_messages modeMsgs;
extern CRITICAL_SECTION modeMsgsMutex;


#endif /* __GLOBALS_H */
