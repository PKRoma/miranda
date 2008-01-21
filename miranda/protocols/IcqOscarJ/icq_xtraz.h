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
// File name      : $URL: https://miranda.svn.sourceforge.net/svnroot/miranda/trunk/miranda/protocols/IcqOscarJ/icq_xtraz.h $
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#ifndef __ICQ_XTRAZ_H
#define __ICQ_XTRAZ_H

void handleXtrazNotify(DWORD dwUin, DWORD dwMID, DWORD dwMID2, WORD wCookie, char* szMsg, int nMsgLen, BOOL bThruDC);
void handleXtrazNotifyResponse(DWORD dwUin, HANDLE hContact, WORD wCookie, char* szMsg, int nMsgLen);

void handleXtrazInvitation(DWORD dwUin, DWORD dwMID, DWORD dwMID2, WORD wCookie, char* szMsg, int nMsgLen, BOOL bThruDC);
void handleXtrazData(DWORD dwUin, DWORD dwMID, DWORD dwMID2, WORD wCookie, char* szMsg, int nMsgLen, BOOL bThruDC);

DWORD SendXtrazNotifyRequest(HANDLE hContact, char* szQuery, char* szNotify, int bForced);
void SendXtrazNotifyResponse(DWORD dwUin, DWORD dwMID, DWORD dwMID2, WORD wCookie, char* szResponse, int nResponseLen, BOOL bThruDC);

// helper functions
BYTE __stdcall ICQGetContactXStatus(HANDLE hContact);

// custom status support
void InitXStatusItems(BOOL bAllowStatus);
void InitXStatusEvents();
void UninitXStatusEvents();

void InitXStatusIcons();
void ChangedIconsXStatus();
HICON GetXStatusIcon(int bStatus, UINT flags);

void handleXStatusCaps(HANDLE hContact, char* caps, int capsize, char* moods, int moodsize);

int IcqShowXStatusDetails(WPARAM wParam, LPARAM lParam);

// custom status public services
int IcqSetXStatus(WPARAM wParam, LPARAM lParam);
int IcqGetXStatus(WPARAM wParam, LPARAM lParam);
int IcqSetXStatusEx(WPARAM wParam, LPARAM lParam);
int IcqGetXStatusEx(WPARAM wParam, LPARAM lParam);
int IcqGetXStatusIcon(WPARAM wParam, LPARAM lParam);
int IcqRequestXStatusDetails(WPARAM wParam, LPARAM lParam);
int IcqRequestAdvStatusIconIdx(WPARAM wParam, LPARAM lParam);



#endif /* __ICQ_XTRAZ_H */
