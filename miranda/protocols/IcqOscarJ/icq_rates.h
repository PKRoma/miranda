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

#ifndef __ICQ_RATES_H
#define __ICQ_RATES_H

typedef struct rates_group_s
{
  DWORD dwWindowSize;
  DWORD dwClearLevel;
  DWORD dwAlertLevel;
  DWORD dwLimitLevel;
  DWORD dwMaxLevel;
  // current level
  int rCurrentLevel;
  int tCurrentLevel;
  // links
  WORD* pPairs;
  int nPairs;
} rates_group;

typedef struct rates_s
{
  int nGroups;
  rates_group groups[1];
} rates;

#define RML_CLEAR   1
#define RML_ALERT   2
#define RML_LIMIT   3
#define RML_IDLE_10 0x10
#define RML_IDLE_30 0x11
#define RML_IDLE_50 0x12
#define RML_IDLE_70 0x13

int ratesGetLimitLevel(rates* pRates, WORD wGroup, int nLevel);

// Rates - Level 2

#define RIT_AWAYMSG_RESPONSE 0x01   // response to status msg request

#define RIT_XSTATUS_REQUEST  0x10   // schedule xstatus details requests
#define RIT_XSTATUS_RESPONSE 0x11   // response to xstatus details request

typedef struct rate_record_s
{
  BYTE bType;         // type of request
  WORD wGroup;
  int nRequestType;
  int nMinDelay;
  HANDLE hContact;
  DWORD dwUin;
  DWORD dwMid1;
  DWORD dwMid2;
  WORD wCookie;
  WORD wVersion;
  BOOL bThruDC;
  char *szData;
  BYTE msgType;
} rate_record;

int ratesDelayToLevel(rates* pRates, WORD wGroup, int nLevel);
int ratesNextRateLevel(rates* pRates, WORD wGroup);

#endif /* __ICQ_RATES_H */
