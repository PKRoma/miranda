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

#ifndef __ICQ_SERVLIST_H
#define __ICQ_SERVLIST_H

// actions:
// 0 = request serv-list
// 1 = update visibility
// 2 = update contact's nick
// 3 = update contact's comment
// 5 = rename group
// A = add privacy item
// B = remove privacy item
// 10 = add contact w/o auth
// 11 = add contact with auth
// 12 = move to group
// 13 = delete contact
// 15 = create group
// 16 = delete group
// 17 = update group
// cookie struct for SSI actions
typedef struct servlistcookie_t
{
  DWORD dwUin;
  HANDLE hContact;
  WORD wContactId;
  WORD wGroupId;
  WORD wNewGroupId;
  int dwAction; 
} servlistcookie;


DWORD icq_sendUploadContactServ(DWORD dwUin, WORD wGroupId, WORD wContactId, const char *szNick, int authRequired, WORD wItemType);
DWORD icq_sendDeleteServerContactServ(DWORD dwUin, WORD wGroupId, WORD wContactId, WORD wItemType);
void InitServerLists(void);
void UninitServerLists(void);

DWORD icq_sendBuddy(DWORD dwCookie, WORD wAction, DWORD dwUin, WORD wGroupId, WORD wContactId, const char *szNick, const char*szNote, int authRequired, WORD wItemType);
DWORD icq_sendGroup(DWORD dwCookie, WORD wAction, WORD wGroupId, const char *szName, void *pContent, int cbContent);

WORD GenerateServerId(VOID);
WORD GenerateServerIdPair(int wCount);
void ReserveServerID(WORD wID);
void FreeServerID(WORD wID);
BOOL CheckServerID(WORD wID, int wCount);
void FlushServerIDs();

#endif /* __ICQ_SERVLIST_H */