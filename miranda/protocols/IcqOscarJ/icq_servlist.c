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
//  Functions that handles list of used server IDs, sends low-level packets for SSI information
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"



extern BYTE gbSsiEnabled;
extern HANDLE ghServerNetlibUser;
extern BOOL bIsSyncingCL;
extern char gpszICQProtoName[MAX_PATH];
extern int gnCurrentStatus;

static HANDLE hHookSettingChanged = NULL;
static HANDLE hHookContactDeleted = NULL;
static WORD* pwIDList = NULL;
static int nIDListCount = 0;
static int nIDListSize = 0;



// Add a server ID to the list of reserved IDs.
// To speed up the process, IDs cannot be removed, and if
// you try to reserve an ID twice, it will be added again.
// You should call CheckServerID before reserving an ID.
void ReserveServerID(WORD wID)
{
	if (nIDListCount >= nIDListSize)
	{
		nIDListSize += 100;
		pwIDList = (WORD*)realloc(pwIDList, nIDListSize * sizeof(WORD));
	}

	pwIDList[nIDListCount] = wID;
	nIDListCount++;
	
}


// Remove a server ID from the list of reserved IDs.
// Used for deleting contacts and other modifications.
void FreeServerID(WORD wID)
{
  int i, j;

  if (pwIDList)
  {
    for (i = 0; i<nIDListCount; i++)
		{
			if (pwIDList[i] == wID)
      { // we found it, so remove
        for (j = i+1; j<nIDListCount; j++)
        {
          pwIDList[j-1] = pwIDList[j];
        }
        nIDListCount--;
      }
		}
	}
  
}


// Returns true if dwID is reserved
BOOL CheckServerID(WORD wID, int wCount)
{
	int i;
	BOOL bFound = FALSE;
	
	if (pwIDList)
	{
		for (i = 0; i<nIDListCount; i++)
		{
			if ((pwIDList[i] >= wID) && (pwIDList[i] <= wID + wCount))
				bFound = TRUE;
		}
	}
	
	return bFound;	
}


void FlushServerIDs()
{
	
	SAFE_FREE(&pwIDList);
	nIDListCount = 0;
	nIDListSize = 0;
	
}


WORD GenerateServerId(VOID)
{

	WORD wId;


	while (TRUE)
	{
		// Randomize a new ID
		// Max value is probably 0x7FFF, lowest value is unknown.
		// We use range 0x1000-0x7FFF.
		wId = (WORD)RandRange(0x1000, 0x7FFF);

		if (!CheckServerID(wId, 0))
			break;
	}
	
	ReserveServerID(wId);

	return wId;
}

// Generate server ID with wCount IDs free after it, for sub-groups.
WORD GenerateServerIdPair(int wCount)
{
  WORD wId;

  while (TRUE)
  {
    // Randomize a new ID
    // Max value is probably 0x7FFF, lowest value is unknown.
    // We use range 0x1000-0x7FFF.
    wId = (WORD)RandRange(0x1000, 0x7FFF);

    if (!CheckServerID(wId, wCount))
      break;
	}
	
	ReserveServerID(wId);

	return wId;
}

// --- Low-level packet sending functions ---


DWORD icq_sendBuddy(DWORD dwCookie, WORD wAction, DWORD dwUin, WORD wGroupId, WORD wContactId, const char *szNick, const char*szNote, int authRequired, WORD wItemType)
{
  icq_packet packet;
  char szUin[10];
  int nUinLen;
  int nNickLen;
  int nNoteLen;
  char* szUtfNick = NULL;
  char* szUtfNote = NULL;
  WORD wTLVlen;

  // Prepare UIN
  _itoa(dwUin, szUin, 10);
  nUinLen = strlen(szUin);

  // Prepare custom utf-8 nick name
  if (szNick && (strlen(szNick) > 0))
  {
    int nResult;

    nResult = utf8_encode(szNick, &szUtfNick);
    nNickLen = strlen(szUtfNick);
  }
  else
  {
    nNickLen = 0;
  }

  // Prepare custom utf-8 note
  if (szNote && (strlen(szNote) > 0))
  {
    int nResult;

    nResult = utf8_encode(szNote, &szUtfNote);
    nNoteLen = strlen(szUtfNote);
  }
  else
  {
    nNoteLen = 0;
  }


  // Build the packet
  packet.wLen = nUinLen + 20;	
  if (nNickLen > 0)
    packet.wLen += nNickLen + 4;
  if (nNoteLen > 0)
    packet.wLen += nNoteLen + 4;
  if (authRequired)
    packet.wLen += 4;

  write_flap(&packet, ICQ_DATA_CHAN);
  packFNACHeader(&packet, ICQ_LISTS_FAMILY, wAction, 0, dwCookie);
  packWord(&packet, (WORD)nUinLen);
  packBuffer(&packet, szUin, (WORD)nUinLen);
  packWord(&packet, wGroupId);
  packWord(&packet, wContactId);
  packWord(&packet, wItemType);

  wTLVlen = ((nNickLen>0) ? 4+nNickLen : 0) + ((nNoteLen>0) ? 4+nNoteLen : 0) + (authRequired?4:0);
  packWord(&packet, wTLVlen);
  if (authRequired)
    packDWord(&packet, 0x00660000);  // "Still waiting for auth" TLV
  if (nNickLen > 0)
  {
    packWord(&packet, 0x0131);	// Nickname TLV
    packWord(&packet, (WORD)nNickLen);
    packBuffer(&packet, szUtfNick, (WORD)nNickLen);
  }
  if (nNoteLen > 0)
  {
    packWord(&packet, 0x013C);	// Comment TLV
    packWord(&packet, (WORD)nNoteLen);
    packBuffer(&packet, szUtfNote, (WORD)nNoteLen);
  }

	// Send the packet and return the cookie
  sendServPacket(&packet);

  SAFE_FREE(&szUtfNick);
  SAFE_FREE(&szUtfNote);

  return dwCookie;
}

DWORD icq_sendGroup(DWORD dwCookie, WORD wAction, WORD wGroupId, const char *szName, void *pContent, int cbContent)
{
  icq_packet packet;
  int nNameLen;
  char* szUtfName = NULL;
  WORD wTLVlen;

  // Prepare custom utf-8 group name
  if (szName && (strlen(szName) > 0))
  {
    int nResult;

    nResult = utf8_encode(szName, &szUtfName);
    nNameLen = strlen(szUtfName);
  }
  else
  {
    nNameLen = 0;
  }
  if (nNameLen == 0 && wGroupId != 0) return 0; // without name we could not change the group

  // Build the packet
  packet.wLen = nNameLen + 20;	
  if (cbContent > 0)
    packet.wLen += cbContent + 4;

  write_flap(&packet, ICQ_DATA_CHAN);
  packFNACHeader(&packet, ICQ_LISTS_FAMILY, wAction, 0, dwCookie);
  packWord(&packet, (WORD)nNameLen);
  packBuffer(&packet, szUtfName, (WORD)nNameLen);
  packWord(&packet, wGroupId);
  packWord(&packet, 0); // ItemId is always 0 for groups
  packWord(&packet, 1); // ItemType 1 = group

  wTLVlen = ((cbContent>0) ? 4+cbContent : 0);
  packWord(&packet, wTLVlen);
  if (cbContent > 0)
  {
    packWord(&packet, 0x0C8);	// Groups TLV
    packWord(&packet, (WORD)cbContent);
    packBuffer(&packet, pContent, (WORD)cbContent);
  }

	// Send the packet and return the cookie
  sendServPacket(&packet);

  SAFE_FREE(&szUtfName);

  return dwCookie;
}

DWORD icq_sendUploadContactServ(DWORD dwUin, WORD wGroupId, WORD wContactId, const char *szNick, int authRequired, WORD wItemType)
{

 /* icq_packet packet;
	char szUin[10];
	int nUinLen;
	int nNickLen;*/
	DWORD dwSequence;
	/*char* szUtfNick = NULL;
	WORD wTLVlen;


	// Prepare UIN
	_itoa(dwUin, szUin, 10);
	nUinLen = strlen(szUin);


	// Prepare custom nick name
	if (szNick && (strlen(szNick) > 0))
	{

		int nResult;

		nResult = utf8_encode(szNick, &szUtfNick);
		nNickLen = strlen(szUtfNick);

	}
	else
	{
		nNickLen = 0;
	}
*/

	// Cookie
	dwSequence = GenerateCookie(0);
/*
	// Build the packet
	packet.wLen = nUinLen + 20;	
	if (nNickLen > 0)
		packet.wLen += nNickLen + 4;
	if (authRequired)
		packet.wLen += 4;

	write_flap(&packet, ICQ_DATA_CHAN);
	packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_ADDTOLIST, 0, dwSequence);
	packWord(&packet, (WORD)nUinLen);
	packBuffer(&packet, szUin, (WORD)nUinLen);
	packWord(&packet, wGroupId);
	packWord(&packet, wContactId);
	packWord(&packet, wItemType);

	wTLVlen = ((nNickLen>0) ? 4+nNickLen : 0) + (authRequired?4:0);
	packWord(&packet, wTLVlen);
	if (authRequired)
		packDWord(&packet, 0x00660000);  // "Still waiting for auth" TLV
	if (nNickLen > 0)
	{
		packWord(&packet, 0x0131);	// Nickname TLV
		packWord(&packet, (WORD)nNickLen);
		packBuffer(&packet, szUtfNick, (WORD)nNickLen);
	}

	// Send the packet and return the cookie
	sendServPacket(&packet);

	SAFE_FREE(&szUtfNick);
*/
  icq_sendBuddy(dwSequence, ICQ_LISTS_ADDTOLIST, dwUin, wGroupId, wContactId, szNick, NULL, authRequired, wItemType);

	return dwSequence;
	
}


DWORD icq_sendDeleteServerContactServ(DWORD dwUin, WORD wGroupId, WORD wContactId, WORD wItemType)
{
/*
  	icq_packet packet;
	char szUin[10];
	int nUinLen;*/
	DWORD dwSequence;
/*
	_itoa(dwUin, szUin, 10);
	nUinLen = strlen(szUin);
*/
	dwSequence = GenerateCookie(0);
/*
	packet.wLen = nUinLen + 20;
	write_flap(&packet, ICQ_DATA_CHAN);
	packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_REMOVEFROMLIST, 0, dwSequence);
	packWord(&packet, (WORD)nUinLen);
	packBuffer(&packet, szUin, (WORD)nUinLen);
	packWord(&packet, wGroupId);
	packWord(&packet, wContactId);
	packWord(&packet, wItemType);
	packWord(&packet, 0x0000);     // Length of additional data

	sendServPacket(&packet);
*/
  icq_sendBuddy(dwSequence, ICQ_LISTS_REMOVEFROMLIST, dwUin, wGroupId, wContactId, NULL, NULL, 0, wItemType);
	return dwSequence;

}


// --- Server-List Operations ---


// Is called when a contact has been renamed locally to update
// the server side nick name.
DWORD renameServContact(HANDLE hContact, const char *pszNick)
{
  WORD wGroupID;
  WORD wItemID;
  DWORD dwUin;
  BOOL bAuthRequired;
  DBVARIANT dbvNote;
  char* pszNote;
  servlistcookie* ack;
  DWORD dwCookie;

  // Get the contact's group ID
  if (!(wGroupID = DBGetContactSettingWord(hContact, gpszICQProtoName, "SrvGroupId", 0)))
  {
    // Could not find a usable group ID
    Netlib_Logf(ghServerNetlibUser, "Failed to upload new nick name to server side list (no group ID)");
    return 0;
  }

  // Get the contact's item ID
  if (!(wItemID = DBGetContactSettingWord(hContact, gpszICQProtoName, "ServerId", 0)))
  {
    // Could not find usable item ID
    Netlib_Logf(ghServerNetlibUser, "Failed to upload new nick name to server side list (no item ID)");
    return 0;
  }

  // Check if contact is authorized
  bAuthRequired = (DBGetContactSettingByte(hContact, gpszICQProtoName, "Auth", 0) == 1);

  // Read comment from DB
  if (DBGetContactSetting(hContact, "UserInfo", "MyNotes", &dbvNote))
    pszNote = NULL; // if not read, no note
  else
    pszNote = dbvNote.pszVal;

	// Get UIN
	if (!(dwUin = DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0)))
  {
    // Could not set nickname on server without uin
    Netlib_Logf(ghServerNetlibUser, "Failed to upload new nick name to server side list (no UIN)");

    DBFreeVariant(&dbvNote);
    return 0;
  }
  
  if (!(ack = (servlistcookie*)malloc(sizeof(servlistcookie))))
  {
    // Could not allocate cookie - use old fake
    Netlib_Logf(ghServerNetlibUser, "Failed to allocate cookie");

    dwCookie = GenerateCookie(ICQ_LISTS_UPDATEGROUP);
  }
  else
  {
    ack->dwAction = 2; // rename
    ack->wContactId = wItemID;
    ack->wGroupId = wGroupID;
    ack->dwUin = dwUin;
    ack->hContact = hContact;

    dwCookie = AllocateCookie(ICQ_LISTS_UPDATEGROUP, dwUin, ack);
  }

  // There is no need to send ICQ_LISTS_CLI_MODIFYSTART or
  // ICQ_LISTS_CLI_MODIFYEND when just changing nick name
  icq_sendBuddy(dwCookie, ICQ_LISTS_UPDATEGROUP, dwUin, wGroupID, wItemID, pszNick, pszNote, bAuthRequired, 0 /* contact */);

  DBFreeVariant(&dbvNote);

  return dwCookie;
}



// Is called when a contact's note was changed to update
// the server side comment.
DWORD setServContactComment(HANDLE hContact, const char *pszNote)
{
  WORD wGroupID;
  WORD wItemID;
  DWORD dwUin;
  BOOL bAuthRequired;
  DBVARIANT dbvNick;
  char* pszNick;
  servlistcookie* ack;
  DWORD dwCookie;

  // Get the contact's group ID
  if (!(wGroupID = DBGetContactSettingWord(hContact, gpszICQProtoName, "SrvGroupId", 0)))
  {
    // Could not find a usable group ID
    Netlib_Logf(ghServerNetlibUser, "Failed to upload new comment to server side list (no group ID)");
    return 0;
  }

  // Get the contact's item ID
  if (!(wItemID = DBGetContactSettingWord(hContact, gpszICQProtoName, "ServerId", 0)))
  {
    // Could not find usable item ID
    Netlib_Logf(ghServerNetlibUser, "Failed to upload new comment to server side list (no item ID)");
    return 0;
  }

  // Check if contact is authorized
  bAuthRequired = (DBGetContactSettingByte(hContact, gpszICQProtoName, "Auth", 0) == 1);

  // Read nick name from DB
  if (DBGetContactSetting(hContact, "CList", "MyHandle", &dbvNick))
    pszNick = NULL; // if not read, no nick
  else
    pszNick = dbvNick.pszVal;

	// Get UIN
	if (!(dwUin = DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0)))
  {
    // Could not set comment on server without uin
    Netlib_Logf(ghServerNetlibUser, "Failed to upload new comment to server side list (no UIN)");

    DBFreeVariant(&dbvNick);
    return 0;
  }
  
  if (!(ack = (servlistcookie*)malloc(sizeof(servlistcookie))))
  {
    // Could not allocate cookie - use old fake
    Netlib_Logf(ghServerNetlibUser, "Failed to allocate cookie");

    dwCookie = GenerateCookie(ICQ_LISTS_UPDATEGROUP);
  }
  else
  {
    ack->dwAction = 3; // update comment
    ack->wContactId = wItemID;
    ack->wGroupId = wGroupID;
    ack->dwUin = dwUin;
    ack->hContact = hContact;

    dwCookie = AllocateCookie(ICQ_LISTS_UPDATEGROUP, dwUin, ack);
  }

  // There is no need to send ICQ_LISTS_CLI_MODIFYSTART or
  // ICQ_LISTS_CLI_MODIFYEND when just changing nick name
  icq_sendBuddy(dwCookie, ICQ_LISTS_UPDATEGROUP, dwUin, wGroupID, wItemID, pszNick, pszNote, bAuthRequired, 0 /* contact */);

  DBFreeVariant(&dbvNick);

  return dwCookie;
}



static int ServListDbSettingChanged(WPARAM wParam, LPARAM lParam)
{

	DBCONTACTWRITESETTING* cws = (DBCONTACTWRITESETTING*)lParam;


	// We can't upload changes to NULL contact
	if ((HANDLE)wParam == NULL)
		return 0;

	// TODO: Queue changes that occur while offline
	if (!icqOnline || !gbSsiEnabled || bIsSyncingCL)
		return 0;
	

	if (!strcmp(cws->szModule, "CList"))
	{
		
		// Has a temporary contact just been added permanently?
		if (!strcmp(cws->szSetting, "NotOnList") &&
			(cws->value.type == DBVT_DELETED || (cws->value.type == DBVT_BYTE && cws->value.bVal == 0)) &&
			DBGetContactSettingByte(NULL, gpszICQProtoName, "ServerAddRemove", DEFAULT_SS_ADDREMOVE))
		{
			
			char* szProto;
			DWORD dwUin;
			
			
			dwUin = DBGetContactSettingDword((HANDLE)wParam, gpszICQProtoName, UNIQUEIDSETTING, 0);
			szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)wParam, 0);
			
			// Is this a ICQ contact and does it have a UIN?
			if (szProto && !strcmp(szProto, gpszICQProtoName) && dwUin)
			{
				
				WORD wGroupId; // TODO: rework for server-groups support
				WORD wContactId;

				
				wGroupId = (WORD)DBGetContactSettingWord(NULL, gpszICQProtoName, "SrvDefGroupId", 0);
				wContactId = DBGetContactSettingWord(NULL, gpszICQProtoName, "ServerId", 0);

				// Only add the contact if it doesnt already have an ID
				if (!wContactId)
				{
					wContactId = GenerateServerId();
					sendAddStart();
					icq_sendUploadContactServ(DBGetContactSettingDword((HANDLE)wParam, gpszICQProtoName, UNIQUEIDSETTING, 0), wGroupId, wContactId, "", 1, SSI_ITEM_BUDDY);
					sendAddEnd();
					DBWriteContactSettingWord((HANDLE)wParam, gpszICQProtoName, "ServerId", wContactId);
					DBWriteContactSettingWord((HANDLE)wParam, gpszICQProtoName, "SrvGroupId", wGroupId);
				}
				
			}
			
		}

    // Has contact been renamed?
    if (!strcmp(cws->szSetting, "MyHandle") &&
      DBGetContactSettingByte(NULL, gpszICQProtoName, "StoreServerDetails", DEFAULT_SS_STORE))
    {
      if (cws->value.type == DBVT_ASCIIZ && cws->value.pszVal != 0)
      {
        renameServContact((HANDLE)wParam, cws->value.pszVal);
      }
      else
      {
        renameServContact((HANDLE)wParam, NULL);
      }
    }		
  }

  if (!strcmp(cws->szModule, "UserInfo"))
  {
    if (!strcmp(cws->szSetting, "MyNotes") &&
      DBGetContactSettingByte(NULL, gpszICQProtoName, "StoreServerDetails", DEFAULT_SS_STORE))
    {
      if (cws->value.type == DBVT_ASCIIZ && cws->value.pszVal != 0)
      {
        setServContactComment((HANDLE)wParam, cws->value.pszVal);
      }
      else
      {
        setServContactComment((HANDLE)wParam, NULL);
      }
    }
  }

  return 0;
}



static int ServListDbContactDeleted(WPARAM wParam, LPARAM lParam)
{
	
	if (!icqOnline || !gbSsiEnabled)
		return 0;
	
	if (DBGetContactSettingByte(NULL, gpszICQProtoName, "ServerAddRemove", DEFAULT_SS_ADDREMOVE))
	{

		WORD wContactID;
		WORD wGroupID;
		WORD wVisibleID;
		WORD wInvisibleID;
		DWORD dwUIN;

		
		wContactID = DBGetContactSettingWord((HANDLE)wParam, gpszICQProtoName, "ServerId", 0);
		wGroupID = DBGetContactSettingWord((HANDLE)wParam, gpszICQProtoName, "SrvGroupId", 0);
		wVisibleID = DBGetContactSettingWord((HANDLE)wParam, gpszICQProtoName, "SrvPermitId", 0);
		wInvisibleID = DBGetContactSettingWord((HANDLE)wParam, gpszICQProtoName, "SrvDenyId", 0);
		dwUIN = DBGetContactSettingDword((HANDLE)wParam, gpszICQProtoName, UNIQUEIDSETTING, 0);

		if ((wGroupID && wContactID) || wVisibleID || wInvisibleID)
		{

			if (wContactID)
      { // TODO: rewrite for global function
			  sendAddStart();
				icq_sendDeleteServerContactServ(dwUIN, wGroupID, wContactID, SSI_ITEM_BUDDY);
			  sendAddEnd();
      }

      if (wVisibleID)
      {
        servlistcookie* ack;
        DWORD dwCookie;

        if (!(ack = (servlistcookie*)malloc(sizeof(servlistcookie))))
        { // cookie failed, use old fake
          dwCookie = GenerateCookie(ICQ_LISTS_ADDTOLIST);
        }
        else
        {
          ack->dwAction = 0xB; // remove privacy item
          ack->dwUin = dwUIN;
          ack->hContact = (HANDLE)wParam;
          ack->wGroupId = 0;
          ack->wContactId = wVisibleID;

          dwCookie = AllocateCookie(ICQ_LISTS_REMOVEFROMLIST, dwUIN, ack);
        }

        icq_sendBuddy(dwCookie, ICQ_LISTS_REMOVEFROMLIST, dwUIN, 0, wVisibleID, NULL, NULL, 0, SSI_ITEM_PERMIT);
      }

      if (wInvisibleID)
      {
        servlistcookie* ack;
        DWORD dwCookie;

        if (!(ack = (servlistcookie*)malloc(sizeof(servlistcookie))))
        { // cookie failed, use old fake
          dwCookie = GenerateCookie(ICQ_LISTS_ADDTOLIST);
        }
        else
        {
          ack->dwAction = 0xB; // remove privacy item
          ack->dwUin = dwUIN;
          ack->hContact = (HANDLE)wParam;
          ack->wGroupId = 0;
          ack->wContactId = wInvisibleID;

          dwCookie = AllocateCookie(ICQ_LISTS_REMOVEFROMLIST, dwUIN, ack);
        }

        icq_sendBuddy(dwCookie, ICQ_LISTS_REMOVEFROMLIST, dwUIN, 0, wInvisibleID, NULL, NULL, 0, SSI_ITEM_DENY);
      }
    }
  }

	return 0;

}



void InitServerLists(void)
{

	hHookSettingChanged = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, ServListDbSettingChanged);
	hHookContactDeleted = HookEvent(ME_DB_CONTACT_DELETED, ServListDbContactDeleted);

}



void UninitServerLists(void)
{

	if (hHookSettingChanged)
		UnhookEvent(hHookSettingChanged);

	if (hHookContactDeleted)
		UnhookEvent(hHookContactDeleted);

	FlushServerIDs();

}
