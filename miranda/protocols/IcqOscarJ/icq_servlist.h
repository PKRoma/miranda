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
#define SSA_CHECK_ROSTER      0     // request serv-list
#define SSA_VISIBILITY        1     // update visibility
#define SSA_CONTACT_RENAME    2     // update contact's nick
#define SSA_CONTACT_COMMENT   3     // update contact's comment
#define SSA_GROUP_RENAME      5     // rename group
#define SSA_PRIVACY_ADD       0xA   // add privacy item
#define SSA_PRIVACY_REMOVE    0xB   // remove privacy item
#define SSA_CONTACT_PRE_ADD   0x14  // add contact to new group, group added
#define SSA_CONTACT_ADD       0x10  // add contact w/o auth
#define SSA_CONTACT_ADD_AUTH  0x11  // add contact with auth
#define SSA_CONTACT_SET_GROUP 0x12  // move to group
#define SSA_CONTACT_REMOVE    0x13  // delete contact
#define SSA_GROUP_ADD         0x15  // create group
#define SSA_GROUP_REMOVE      0x16  // delete group
#define SSA_GROUP_UPDATE      0x17  // update group

typedef void (*GROUPADDCALLBACK)(const char *szGroupPath, WORD wGroupId, LPARAM lParam);

// cookie struct for SSI actions
typedef struct servlistcookie_t
{
  DWORD dwUin;
  HANDLE hContact;
  WORD wContactId;
  WORD wGroupId;
  char* szGroupName;
  WORD wNewGroupId;
  int dwAction; 
  GROUPADDCALLBACK ofCallback;
  LPARAM lParam;
} servlistcookie;


DWORD icq_sendUploadContactServ(DWORD dwUin, WORD wGroupId, WORD wContactId, const char *szNick, int authRequired, WORD wItemType);
DWORD icq_sendDeleteServerContactServ(DWORD dwUin, WORD wGroupId, WORD wContactId, WORD wItemType);
void InitServerLists(void);
void UninitServerLists(void);

void* collectGroups(int *count);
void* collectBuddyGroup(WORD wGroupID, int *count);
char* getServerGroupName(WORD wGroupID);
void setServerGroupName(WORD wGroupID, const char* szGroupName);
WORD getServerGroupID(const char* szPath);
void setServerGroupID(const char* szPath, WORD wGroupID);
int IsServerGroupsDefined();
char* makeGroupPath(WORD wGroupId);
WORD makeGroupId(const char* szGroupPath, GROUPADDCALLBACK ofCallback, servlistcookie* lParam);

DWORD addServContact(HANDLE hContact, const char *pszNick, const char *pszGroup);
DWORD removeServContact(HANDLE hContact);
DWORD moveServContactGroup(HANDLE hContact, const char *pszNewGroup);

DWORD icq_sendBuddy(DWORD dwCookie, WORD wAction, DWORD dwUin, WORD wGroupId, WORD wContactId, const char *szNick, const char*szNote, int authRequired, WORD wItemType);
DWORD icq_sendGroup(DWORD dwCookie, WORD wAction, WORD wGroupId, const char *szName, void *pContent, int cbContent);

WORD GenerateServerId(VOID);
WORD GenerateServerIdPair(int wCount);
void ReserveServerID(WORD wID);
void FreeServerID(WORD wID);
BOOL CheckServerID(WORD wID, int wCount);
void FlushServerIDs();
void LoadServerIDs();

#endif /* __ICQ_SERVLIST_H */