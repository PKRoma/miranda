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

#ifndef __ICQ_SERVLIST_H
#define __ICQ_SERVLIST_H

// actions:
#define SSA_CHECK_ROSTER      0     // request serv-list
#define SSA_VISIBILITY        1     // update visibility
#define SSA_CONTACT_UPDATE    2     // update contact's details
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
#define SSA_SERVLIST_ACK      0x20  // send proto ack only (UploadUI)
#define SSA_SETAVATAR         0x30
#define SSA_REMOVEAVATAR      0x31
#define SSA_IMPORT            7

typedef void (*GROUPADDCALLBACK)(WORD wGroupId, LPARAM lParam);

// cookie struct for SSI actions
typedef struct servlistcookie_t
{
  DWORD dwUin;
  HANDLE hContact;
  WORD wContactId;
  WORD wGroupId;
  char* szGroupName;
  WORD wNewContactId;
  WORD wNewGroupId;
  int dwAction; 
  GROUPADDCALLBACK ofCallback;
  LPARAM lParam;
} servlistcookie;


void InitServerLists(void);
void UninitServerLists(void);

void* collectGroups(int *count);
void* collectBuddyGroup(WORD wGroupID, int *count);
char* getServerGroupNameUtf(WORD wGroupID);
void setServerGroupNameUtf(WORD wGroupID, const char* szGroupNameUtf);
WORD getServerGroupIDUtf(const char* szPath);
void setServerGroupIDUtf(const char* szPath, WORD wGroupID);
int IsServerGroupsDefined();
char* makeGroupPathUtf(WORD wGroupId);
WORD makeGroupId(const char* szGroupPath, GROUPADDCALLBACK ofCallback, servlistcookie* lParam);
void removeGroupPathLinks(WORD wGroupID);
int countGroupLevel(WORD wGroupId);

void FlushSrvGroupsCache();

DWORD icq_sendServerContact(HANDLE hContact, DWORD dwCookie, WORD wAction, WORD wGroupId, WORD wContactId);
DWORD icq_sendSimpleItem(DWORD dwCookie, WORD wAction, DWORD dwUin, char* szUID, WORD wGroupId, WORD wItemId, WORD wItemType);
DWORD icq_sendGroupUtf(DWORD dwCookie, WORD wAction, WORD wGroupId, const char *szName, void *pContent, int cbContent);

DWORD icq_removeServerPrivacyItem(HANDLE hContact, DWORD dwUin, char* szUid, WORD wItemId, WORD wType);
DWORD icq_addServerPrivacyItem(HANDLE hContact, DWORD dwUin, char* szUid, WORD wItemId, WORD wType);

// id type groups
#define SSIT_ITEM 0
#define SSIT_GROUP 1

WORD GenerateServerId(int bGroupId);
WORD GenerateServerIdPair(int bGroupId, int wCount);
void ReserveServerID(WORD wID, int bGroupId);
void FreeServerID(WORD wID, int bGroupId);
BOOL CheckServerID(WORD wID, unsigned int wCount);
void FlushServerIDs();
void LoadServerIDs();

void FlushPendingOperations();
void RemovePendingOperation(HANDLE hContact, int nResult);
void AddGroupRename(WORD wGroupID);
void RemoveGroupRename(WORD wGroupID);
void FlushGroupRenames();

void AddJustAddedContact(HANDLE hContact);
BOOL IsContactJustAdded(HANDLE hContact);
void FlushJustAddedContacts();

#endif /* __ICQ_SERVLIST_H */
