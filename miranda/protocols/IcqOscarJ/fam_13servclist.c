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

#include "icqoscar.h"


extern HANDLE ghServerNetlibUser;
extern int gnCurrentStatus;
extern char gpszICQProtoName[MAX_PATH];
extern DWORD dwLocalInternalIP;
extern DWORD dwLocalExternalIP;
extern WORD wListenPort;

extern void setUserInfo();
extern int GroupNameExists(const char *name,int skipGroup);

BOOL bIsSyncingCL = FALSE;

static void handleServerCListAck(servlistcookie* sc, WORD wError);
static void handleServerCList(unsigned char *buf, WORD wLen, WORD wFlags);
static void handleRecvAuthRequest(unsigned char *buf, WORD wLen);
static void handleRecvAuthResponse(unsigned char *buf, WORD wLen);
static void handleRecvAdded(unsigned char *buf, WORD wLen);
void sendRosterAck(void);



void handleServClistFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader)
{
  switch (pSnacHeader->wSubtype)
  {

  case ICQ_LISTS_ACK: // UPDATE_ACK
    if (wBufferLength >= 2)
    {
      WORD wError;
      DWORD dwActUin;
      servlistcookie* sc;

      unpackWord(&pBuffer, &wError);

      if (FindCookie(pSnacHeader->dwRef, &dwActUin, &sc))
      { // look for action cookie
#ifdef _DEBUG
        Netlib_Logf(ghServerNetlibUser, "Received expected server list ack, action: %d, result: %d", sc->dwAction, wError);
#endif
        FreeCookie(pSnacHeader->dwRef); // release cookie

        handleServerCListAck(sc, wError);
      }
      else
      {
        Netlib_Logf(ghServerNetlibUser, "Received unexpected server list ack %u", wError);
      }
    }
    break;

  case ICQ_LISTS_SRV_REPLYLISTS:
    /* received list rights, we just skip them */
    /* as the structure is not yet fully understood */
    Netlib_Logf(ghServerNetlibUser, "Server sent SNAC(x13,x03) - SRV_REPLYLISTS");
    break;

  case ICQ_LISTS_LIST: // SRV_REPLYROSTER
  {
    servlistcookie* sc;
    BOOL blWork;

    blWork = bIsSyncingCL;
    bIsSyncingCL = TRUE; // this is not used if cookie takes place

    if (FindCookie(pSnacHeader->dwRef, NULL, &sc))
    { // we do it by reliable cookie
      if (!sc->dwUin)
      { // is this first packet ?
        ResetSettingsOnListReload();
        sc->dwUin = 1;
      }
      handleServerCList(pBuffer, wBufferLength, pSnacHeader->wFlags);
      if (!(pSnacHeader->wFlags & 0x0001))
      { // was that last packet ?
        FreeCookie(pSnacHeader->dwRef); // yes, release cookie
        SAFE_FREE(&sc);
      }
    }
    else
    { // use old fake
      if (!blWork)
      { // this can fail on some crazy situations
        ResetSettingsOnListReload();
      }
      handleServerCList(pBuffer, wBufferLength, pSnacHeader->wFlags);
    }
    break;
  }

	case SRV_SSI_UPTODATE: // SRV_REPLYROSTEROK
  {
    servlistcookie* sc;

		bIsSyncingCL = FALSE;

    if (FindCookie(pSnacHeader->dwRef, NULL, &sc))
    { // we requested servlist check
#ifdef _DEBUG
		  Netlib_Logf(ghServerNetlibUser, "Server stated roster is ok.");
#endif
      FreeCookie(pSnacHeader->dwRef);
      LoadServerIDs();

      SAFE_FREE(&sc);
    }
    else
		  Netlib_Logf(ghServerNetlibUser, "Server sent unexpected SNAC(x13,x0F) - SRV_REPLYROSTEROK");

		// This will activate the server side list
		sendRosterAck(); // this must be here, cause of failures during cookie alloc
		handleServUINSettings(wListenPort, dwLocalInternalIP);
		break;
  }

	case ICQ_LISTS_CLI_MODIFYSTART:
		Netlib_Logf(ghServerNetlibUser, "Server sent SNAC(x13,x11) - Server is modifying contact list");
		break;

	case ICQ_LISTS_CLI_MODIFYEND:
		Netlib_Logf(ghServerNetlibUser, "Server sent SNAC(x13,x12) - End of server modification");
		break;

  case ICQ_LISTS_UPDATEGROUP:
    Netlib_Logf(ghServerNetlibUser, "Server sent SNAC(x13,x09) - Server updated our contact on list");
    break;

  case ICQ_LISTS_REMOVEFROMLIST:
    Netlib_Logf(ghServerNetlibUser, "Server sent SNAC(x13,x0A) - User removed from our contact list");
    break;

	case ICQ_LISTS_AUTHREQUEST:
		handleRecvAuthRequest(pBuffer, wBufferLength);
		break;

	case ICQ_LISTS_SRV_AUTHRESPONSE:
		handleRecvAuthResponse(pBuffer, wBufferLength);
		break;

	case ICQ_LISTS_AUTHGRANTED:
    Netlib_Logf(ghServerNetlibUser, "Server sent SNAC(x13,x15) - User granted us future authorization");
    break;

	case ICQ_LISTS_YOUWEREADDED:
		handleRecvAdded(pBuffer, wBufferLength);
		break;

  case ICQ_LISTS_ERROR:
    if (wBufferLength >= 2)
    {
      WORD wError;
      DWORD dwActUin;
      servlistcookie* sc;

      unpackWord(&pBuffer, &wError);

      if (FindCookie(pSnacHeader->dwRef, &dwActUin, &sc))
      { // look for action cookie
#ifdef _DEBUG
        Netlib_Logf(ghServerNetlibUser, "Received server list error, action: %d, result: %d", sc->dwAction, wError);
#endif
        FreeCookie(pSnacHeader->dwRef); // release cookie

        if (sc->dwAction==SSA_CHECK_ROSTER)
        { // the serv-list is unavailable turn it off
          icq_LogMessage(LOG_ERROR, Translate("Server contact list is unavailable, Miranda will use local contact list."));
          gbSsiEnabled = 0;
          handleServUINSettings(wListenPort, dwLocalInternalIP);
        }
        SAFE_FREE(&sc);
      }
      else
      {
        Netlib_Logf(ghServerNetlibUser, "Received unrecognized server list error %u", wError);
      }
    }
    break;

	default:
		Netlib_Logf(ghServerNetlibUser, "Warning: Ignoring SNAC(x13,x%02x) - Unknown SNAC (Flags: %u, Ref: %u", pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
		break;

	}
}



static void handleServerCListAck(servlistcookie* sc, WORD wError)
{
  switch (sc->dwAction)
  {
  case SSA_VISIBILITY: 
    {
      if (wError)
        Netlib_Logf(ghServerNetlibUser, "Server visibility update failed, error %d", wError);
      break;
    }
  case SSA_CONTACT_RENAME: 
    {
      RemovePendingOperation(sc->hContact, 1);
      if (wError)
      {
        Netlib_Logf(ghServerNetlibUser, "Renaming of server contact failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, Translate("Renaming of server contact failed."));
      }
      break;
    }
  case SSA_CONTACT_COMMENT: 
    {
      if (wError)
      {
        Netlib_Logf(ghServerNetlibUser, "Update of server contact's comment failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, Translate("Update of server contact's comment failed."));
      }
      break;
    }
  case SSA_PRIVACY_ADD: 
    {
      if (wError)
      {
        Netlib_Logf(ghServerNetlibUser, "Adding of privacy item to server list failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, Translate("Adding of privacy item to server list failed."));
      }
      break;
    }
  case SSA_PRIVACY_REMOVE: 
    {
      if (wError)
      {
        Netlib_Logf(ghServerNetlibUser, "Removing of privacy item from server list failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, Translate("Removing of privacy item from server list failed."));
      }
      FreeServerID(sc->wContactId, SSIT_ITEM); // release server id
      break;
    }
  case SSA_CONTACT_ADD:
    {
      if (wError)
      {
        if (wError == 0xE) // server refused to add contact w/o auth, add with
        {
          DWORD dwCookie;

          Netlib_Logf(ghServerNetlibUser, "Contact could not be added without authorisation, add with await auth flag.");

          DBWriteContactSettingByte(sc->hContact, gpszICQProtoName, "Auth", 1); // we need auth
          dwCookie = AllocateCookie(ICQ_LISTS_ADDTOLIST, sc->dwUin, sc);
          icq_sendBuddy(dwCookie, ICQ_LISTS_ADDTOLIST, sc->dwUin, sc->wGroupId, sc->wContactId, sc->szGroupName, NULL, 1, SSI_ITEM_BUDDY);

          sc = NULL; // we do not want it to be freed now
          break;
        }
        FreeServerID(sc->wContactId, SSIT_ITEM);
        SAFE_FREE(&sc->szGroupName); // the the nick
        sendAddEnd(); // end server modifications here
        RemovePendingOperation(sc->hContact, 0);

        Netlib_Logf(ghServerNetlibUser, "Adding of contact to server list failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, Translate("Adding of contact to server list failed."));
      }
      else
      {
        void* groupData;
        int groupSize;
        HANDLE hPend = sc->hContact;

        DBWriteContactSettingWord(sc->hContact, gpszICQProtoName, "ServerId", sc->wContactId);
	      DBWriteContactSettingWord(sc->hContact, gpszICQProtoName, "SrvGroupId", sc->wGroupId);
        SAFE_FREE(&sc->szGroupName); // free the nick

        if (groupData = collectBuddyGroup(sc->wGroupId, &groupSize))
        { // the group is not empty, just update it
          DWORD dwCookie;

          sc->dwAction = SSA_GROUP_UPDATE;
          sc->wContactId = 0;
          sc->hContact = NULL;
          sc->szGroupName = getServerGroupName(sc->wGroupId);
          dwCookie = AllocateCookie(ICQ_LISTS_UPDATEGROUP, 0, sc);

          icq_sendGroup(dwCookie, ICQ_LISTS_UPDATEGROUP, sc->wGroupId, sc->szGroupName, groupData, groupSize);
          sendAddEnd(); // end server modifications here

          sc = NULL; // we do not want it to be freed now
        }
        else
        { // this should never happen
          Netlib_Logf(ghServerNetlibUser, "Group update failed.");
          sendAddEnd(); // end server modifications here
        }
        if (hPend) RemovePendingOperation(hPend, 1);
      }
      break;
    }
  case SSA_GROUP_ADD:
    {
      if (wError)
      {
        FreeServerID(sc->wGroupId, SSIT_GROUP);
        Netlib_Logf(ghServerNetlibUser, "Adding of group to server list failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, Translate("Adding of group to server list failed."));
      }
      else // group added, we need to update master group
      {
        void* groupData;
        int groupSize;
        servlistcookie* ack;
        DWORD dwCookie;

        setServerGroupName(sc->wGroupId, sc->szGroupName); // add group to namelist
        setServerGroupID(makeGroupPath(sc->wGroupId), sc->wGroupId); // add group to known

        groupData = collectGroups(&groupSize);
        groupData = realloc(groupData, groupSize+2);
        *(((WORD*)groupData)+(groupSize>>1)) = sc->wGroupId; // add this new group id
        groupSize += 2;

        ack = (servlistcookie*)malloc(sizeof(servlistcookie));
        if (ack)
        {
          ack->wGroupId = 0;
          ack->dwAction = SSA_GROUP_UPDATE;
          ack->szGroupName = NULL;
          dwCookie = AllocateCookie(ICQ_LISTS_UPDATEGROUP, 0, ack);

          icq_sendGroup(dwCookie, ICQ_LISTS_UPDATEGROUP, 0, ack->szGroupName, groupData, groupSize);
        }
        sendAddEnd(); // end server modifications here

        SAFE_FREE(&groupData);

        if (sc->ofCallback) // is add contact pending
        {
          sc->ofCallback(sc->wGroupId, (LPARAM)sc->lParam);
         // sc = NULL; // we do not want to be freed here
        }
      }
      SAFE_FREE(&sc->szGroupName);
      break;
    }
  case SSA_CONTACT_REMOVE:
    {
      if (!wError)
      {
        void* groupData;
        int groupSize;

        DBWriteContactSettingWord(sc->hContact, gpszICQProtoName, "ServerId", 0); // clear the values
        DBWriteContactSettingWord(sc->hContact, gpszICQProtoName, "SrvGroupId", 0);

        FreeServerID(sc->wContactId, SSIT_ITEM); 
              
        if (groupData = collectBuddyGroup(sc->wGroupId, &groupSize))
        { // the group is still not empty, just update it
          DWORD dwCookie;

          sc->dwAction = SSA_GROUP_UPDATE;
          sc->wContactId = 0;
          sc->hContact = NULL;
          sc->szGroupName = getServerGroupName(sc->wGroupId);
          dwCookie = AllocateCookie(ICQ_LISTS_UPDATEGROUP, 0, sc);

          icq_sendGroup(dwCookie, ICQ_LISTS_UPDATEGROUP, sc->wGroupId, sc->szGroupName, groupData, groupSize);
          sendAddEnd(); // end server modifications here

          sc = NULL; // we do not want it to be freed now
        }
        else // the group is empty, delete it if it does not have sub-groups
        {
          DWORD dwCookie;

          if (!CheckServerID((WORD)(sc->wGroupId+1), 0) || countGroupLevel((WORD)(sc->wGroupId+1)) == 0)
          { // is next id an sub-group, if yes, we cannot delete this group
            sc->dwAction = SSA_GROUP_REMOVE;
            sc->wContactId = 0;
            sc->hContact = NULL;
            sc->szGroupName = getServerGroupName(sc->wGroupId);
            dwCookie = AllocateCookie(ICQ_LISTS_REMOVEFROMLIST, 0, sc);

            icq_sendGroup(dwCookie, ICQ_LISTS_REMOVEFROMLIST, sc->wGroupId, sc->szGroupName, NULL, 0);
            // here the modifications go on
            sc = NULL; // we do not want it to be freed now
          }
        }
        SAFE_FREE(&groupData); // free the memory
      }
      else
      {
        Netlib_Logf(ghServerNetlibUser, "Removing of contact from server list failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, Translate("Removing of contact from server list failed."));
        sendAddEnd(); // end server modifications here
      }
      break;
    }
  case SSA_GROUP_UPDATE:
    {
      if (wError)
      {
        Netlib_Logf(ghServerNetlibUser, "Updating of group on server list failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, Translate("Updating of group on server list failed."));
      }
      SAFE_FREE(&sc->szGroupName);
      break;
    }
  case SSA_GROUP_REMOVE:
    {
      SAFE_FREE(&sc->szGroupName);
      if (wError)
      {
        Netlib_Logf(ghServerNetlibUser, "Removing of group from server list failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, Translate("Removing of group from server list failed."));
      }
      else // group removed, we need to update master group
      {
        void* groupData;
        int groupSize;
        DWORD dwCookie;

        setServerGroupName(sc->wGroupId, NULL); // clear group from namelist
        FreeServerID(sc->wGroupId, SSIT_GROUP);
        removeGroupPathLinks(sc->wGroupId);

        groupData = collectGroups(&groupSize);
        sc->wGroupId = 0;
        sc->dwAction = SSA_GROUP_UPDATE;
        sc->szGroupName = "";
        dwCookie = AllocateCookie(ICQ_LISTS_UPDATEGROUP, 0, sc);

        icq_sendGroup(dwCookie, ICQ_LISTS_UPDATEGROUP, 0, sc->szGroupName, groupData, groupSize);
        sendAddEnd(); // end server modifications here

        sc = NULL; // we do not want to be freed here

        SAFE_FREE(&groupData);
      }
      break;
    }
  case SSA_CONTACT_SET_GROUP:
    { // we moved contact to another group
      if (sc->lParam == -1)
      { // the first was an error
        break;
      }
      if (wError)
      {
        RemovePendingOperation(sc->hContact, 0);
        Netlib_Logf(ghServerNetlibUser, "Moving of user to another group on server list failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, Translate("Moving of user to another group on server list failed."));
        if (!sc->lParam) // is this first ack ?
        {
          sc->lParam = -1;
          sc = NULL; // this can't be freed here
        }
        break;
      }
      if (sc->lParam) // is this the second ack ?
      {
        void* groupData;
        int groupSize;
        int bEnd = 1; // shall we end the sever modifications

        DBWriteContactSettingWord(sc->hContact, gpszICQProtoName, "ServerId", sc->wNewContactId);
	      DBWriteContactSettingWord(sc->hContact, gpszICQProtoName, "SrvGroupId", sc->wNewGroupId);
        FreeServerID(sc->wContactId, SSIT_ITEM); // release old contact id

        if (groupData = collectBuddyGroup(sc->wGroupId, &groupSize)) // update the group we moved from
        { // the group is still not empty, just update it
          DWORD dwCookie;
          servlistcookie* ack;

          ack = (servlistcookie*)malloc(sizeof(servlistcookie));
          if (!ack)
          {
            Netlib_Logf(ghServerNetlibUser, "Updating of group on server list failed (malloc error)");
            break;
          }
          ack->dwAction = SSA_GROUP_UPDATE;
          ack->wContactId = 0;
          ack->hContact = NULL;
          ack->szGroupName = getServerGroupName(sc->wGroupId);
          ack->wGroupId = sc->wGroupId;
          dwCookie = AllocateCookie(ICQ_LISTS_UPDATEGROUP, 0, ack);

          icq_sendGroup(dwCookie, ICQ_LISTS_UPDATEGROUP, ack->wGroupId, ack->szGroupName, groupData, groupSize);
        }
        else if (!CheckServerID((WORD)(sc->wGroupId+1), 0) || countGroupLevel((WORD)(sc->wGroupId+1)) == 0)
        { // the group is empty and is not followed by sub-groups, delete it
          DWORD dwCookie;
          servlistcookie* ack;

          ack = (servlistcookie*)malloc(sizeof(servlistcookie));
          if (!ack)
          {
            Netlib_Logf(ghServerNetlibUser, "Updating of group on server list failed (malloc error)");
            break;
          }
          ack->dwAction = SSA_GROUP_REMOVE;
          ack->wContactId = 0;
          ack->hContact = NULL;
          ack->szGroupName = getServerGroupName(sc->wGroupId);
          ack->wGroupId = sc->wGroupId;
          dwCookie = AllocateCookie(ICQ_LISTS_REMOVEFROMLIST, 0, ack);

          icq_sendGroup(dwCookie, ICQ_LISTS_REMOVEFROMLIST, ack->wGroupId, ack->szGroupName, NULL, 0);
          bEnd = 0; // here the modifications go on
        }
        SAFE_FREE(&groupData); // free the memory

        groupData = collectBuddyGroup(sc->wNewGroupId, &groupSize); // update the group we moved to
        {
          DWORD dwCookie;
          servlistcookie* ack;

          ack = (servlistcookie*)malloc(sizeof(servlistcookie));
          if (!ack)
          {
            Netlib_Logf(ghServerNetlibUser, "Updating of group on server list failed (malloc error)");
            break;
          }
          ack->dwAction = SSA_GROUP_UPDATE;
          ack->wContactId = 0;
          ack->hContact = NULL;
          ack->szGroupName = getServerGroupName(sc->wNewGroupId);
          ack->wGroupId = sc->wNewGroupId;
          dwCookie = AllocateCookie(ICQ_LISTS_UPDATEGROUP, 0, ack);

          icq_sendGroup(dwCookie, ICQ_LISTS_UPDATEGROUP, ack->wGroupId, ack->szGroupName, groupData, groupSize);
        }
        if (bEnd) sendAddEnd();
        if (sc->hContact) RemovePendingOperation(sc->hContact, 1);
      }
      else
      {
        sc->lParam = 1;
        sc = NULL; // wait for second ack
      }
      break;
    }
  case SSA_GROUP_RENAME:
    {
      if (wError)
      {
        Netlib_Logf(ghServerNetlibUser, "Renaming of server group failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, Translate("Renaming of server group failed."));
      }
      else
      { 
        setServerGroupName(sc->wGroupId, sc->szGroupName);
        removeGroupPathLinks(sc->wGroupId);
        setServerGroupID(makeGroupPath(sc->wGroupId), sc->wGroupId);
      }
      RemoveGroupRename(sc->wGroupId);
      break;
    }
  case SSA_SETAVATAR:
    {
      if (wError)
      {
        Netlib_Logf(ghServerNetlibUser, "Uploading of avatar hash failed.");
        if (sc->wGroupId) // is avatar added or updated?
        {
          FreeServerID(sc->wContactId, SSIT_ITEM);
          DBDeleteContactSetting(NULL, gpszICQProtoName, "SrvAvatarID"); // to fix old versions
        }
      }
      else
      {
        DBWriteContactSettingWord(NULL, gpszICQProtoName, "SrvAvatarID", sc->wContactId);
      }
      break;
    }
  case SSA_REMOVEAVATAR:
    {
      if (wError)
        Netlib_Logf(ghServerNetlibUser, "Removing of avatar hash failed.");
      else
      {
        DBDeleteContactSetting(NULL, gpszICQProtoName, "SrvAvatarID");
        FreeServerID(sc->wContactId, SSIT_ITEM);

        setUserInfo();
      }
      break;
    }
  case SSA_SERVLIST_ACK:
    {
      ProtoBroadcastAck(gpszICQProtoName, sc->hContact, ICQACKTYPE_SERVERCLIST, wError?ACKRESULT_FAILED:ACKRESULT_SUCCESS, (HANDLE)sc->lParam, wError);
      break;
    }
  default:
    Netlib_Logf(ghServerNetlibUser, "Server ack cookie type (%d) not recognized.", sc->dwAction);
  }
  SAFE_FREE(&sc); // free the memory

  return;
}



static void handleServerCList(unsigned char *buf, WORD wLen, WORD wFlags)
{
	BYTE bySSIVersion;
	WORD wRecordCount;
	WORD wRecord;
	WORD wRecordNameLen;
	WORD wGroupId;
	WORD wItemId;
	WORD wTlvType;
	WORD wTlvLength;
	BOOL bIsLastPacket;
	char* pszRecordName = NULL;
	oscar_tlv_chain* pChain = NULL;
	oscar_tlv* pTLV = NULL;


	// If flag bit 1 is set, this is not the last
	// packet. If it is 0, this is the last packet
	// and there will be a timestamp at the end.
	if (wFlags & 0x0001)
		bIsLastPacket = FALSE;
	else
		bIsLastPacket = TRUE;


	if (wLen < 3)
		return;

	// Version number of SSI protocol?
	unpackByte(&buf, &bySSIVersion);
	wLen -= 1;

	// Total count of following entries. This is the size of the server
	// side contact list and should be saved and sent with CLI_CHECKROSTER.
	// NOTE: When the entries are split up in several packets, each packet
	// has it's own count and they must be added to get the total size of
	// server list.
	unpackWord(&buf, &wRecordCount);
	wLen -= 2;
	Netlib_Logf(ghServerNetlibUser, "SSI: number of entries is %u, version is %u", wRecordCount, bySSIVersion);


	// Loop over all items in the packet
	for (wRecord = 0; wRecord < wRecordCount; wRecord++)
	{
		Netlib_Logf(ghServerNetlibUser, "SSI: parsing record %u", wRecord + 1);

		if (wLen < 10)
		{
			// minimum: name length (zero), group ID, item ID, empty TLV
			Netlib_Logf(ghServerNetlibUser, "Warning: SSI parsing error (0)");
			break;
		}

		// The name of the entry. If this is a group header, then this
		// is the name of the group. If it is a plain contact list entry,
		// then it's the UIN of the contact.
		unpackWord(&buf, &wRecordNameLen);
		if (wLen < 10 + wRecordNameLen)
		{
			Netlib_Logf(ghServerNetlibUser, "Warning: SSI parsing error (1)");
			break;
		}
		pszRecordName = (char*)realloc(pszRecordName, wRecordNameLen + 1);
		unpackString(&buf, pszRecordName, wRecordNameLen);
		pszRecordName[wRecordNameLen] = '\0';

		// The group identifier this entry belongs to. If 0, this is meta information or
		// a contact without a group
		unpackWord(&buf, &wGroupId);

		// The ID of this entry. Group headers have ID 0. Otherwise, this
		// is a random number generated when the user is added to the
		// contact list, or when the user is ignored. See CLI_ADDBUDDY.
		unpackWord(&buf, &wItemId);

		// This field indicates what type of entry this is
		unpackWord(&buf, &wTlvType);

		// The length in bytes of the following TLV chain
		unpackWord(&buf, &wTlvLength);


		Netlib_Logf(ghServerNetlibUser, "Name: '%s', GroupID: %u, EntryID: %u, EntryType: %u, TLVlength: %u",
			pszRecordName, wGroupId, wItemId, wTlvType, wTlvLength);


		wLen -= (10 + wRecordNameLen);
		if (wLen < wTlvLength)
		{
			Netlib_Logf(ghServerNetlibUser, "Warning: SSI parsing error (2)");
			break;
		}


		// Initialize the tlv chain
		if (wTlvLength > 0)
		{
			pChain = readIntoTLVChain(&buf, (WORD)(wTlvLength), 0);
			wLen -= wTlvLength;
		}
		else
		{
			pChain = NULL;
		}


		switch (wTlvType)
		{

    case SSI_ITEM_BUDDY:
      {
        /* this is a contact */

        if (!IsStringUIN(pszRecordName))
        {
          Netlib_Logf(ghServerNetlibUser, "Ignoring fake AOL contact, message is: \"%s\"", pszRecordName);
        }
        else
        {
          HANDLE hContact;
          char* pszProto;
          DWORD dwUin;
          int bAdded = 0;
          int bRegroup = 0;
          int bNicked = 0;
          WORD wOldGroupId;

          // This looks like a real contact
          // Check if this user already exists in local list
          dwUin = atoi(pszRecordName);
					hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
					while (hContact)
					{
						pszProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
						if (pszProto && !strcmp(pszProto, gpszICQProtoName) && dwUin == DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0))
						{
							Netlib_Logf(ghServerNetlibUser, "SSI ignoring existing contact '%d'", dwUin);
							break;
						}
						hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
					}

					if (hContact == NULL)
          { // Not already on list: add
            char* szGroup;

						Netlib_Logf(ghServerNetlibUser, "SSI adding new contact '%d'", dwUin);
						hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD,0,0);
						if (!hContact)
						{
							Netlib_Logf(ghServerNetlibUser, "Failed to create ICQ contact %u", dwUin);
							if (pChain)
								disposeChain(&pChain);
							continue;
						}
						if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)gpszICQProtoName) != 0)
						{
							// For some reason we failed to register the protocol to this contact
							CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
							Netlib_Logf(ghServerNetlibUser, "Failed to register ICQ contact %u", dwUin);
							if (pChain)
								disposeChain(&pChain);
							continue;
						}

            if (szGroup = makeGroupPath(wGroupId))
            { // try to get Miranda Group path from groupid, if succeeded save to db
              DBWriteContactSettingString(hContact, "CList", "Group", szGroup);

              SAFE_FREE(&szGroup);
            }
            bAdded = 1;
            AddJustAddedContact(hContact);

						DBWriteContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, dwUin);
					}
					else
					{
						// we should add new contacts and this contact was just added, show it
						if (IsContactJustAdded(hContact))
            {
							DBWriteContactSettingByte(hContact, "CList", "Hidden", 0);
              bAdded = 1; // we want details for new contacts
            }
            // Contact on server is always on list
            DBWriteContactSettingByte(hContact, "CList", "NotOnList", 0);
          }

          wOldGroupId = DBGetContactSettingWord(hContact, gpszICQProtoName, "SrvGroupId", 0);
					// Save group and item ID
					DBWriteContactSettingWord(hContact, gpszICQProtoName, "ServerId", wItemId);
					DBWriteContactSettingWord(hContact, gpszICQProtoName, "SrvGroupId", wGroupId);
					ReserveServerID(wItemId, SSIT_ITEM);

          if (!bAdded && (wOldGroupId != wGroupId) && DBGetContactSettingByte(NULL, gpszICQProtoName, "LoadServerDetails", DEFAULT_SS_LOAD))
          { // contact has been moved on the server
            char* szOldGroup = getServerGroupName(wOldGroupId);
            char* szGroup = getServerGroupName(wGroupId);

            if (!szOldGroup)
            { // old group is not known, most probably not created subgroup
              DBVARIANT dbv;

              if (!DBGetContactSetting(hContact, "CList", "Group", &dbv))
              { // get group from CList
                if (dbv.pszVal && strlen(dbv.pszVal) > 0)
                  szOldGroup = _strdup(dbv.pszVal);
                DBFreeVariant(&dbv);
              }
              if (!szOldGroup) szOldGroup = _strdup(DEFAULT_SS_GROUP);
            }

            if (!szGroup || strlennull(szGroup)>=strlennull(szOldGroup) || strnicmp(szGroup, szOldGroup, strlennull(szGroup)))
            { // contact moved to new group or sub-group or not to master group
              bRegroup = 1;
            }
            if (bRegroup && !stricmp(DEFAULT_SS_GROUP, szGroup) && !GroupNameExists(szGroup, -1))
            { // is it the default "General" group ? yes, does it exists in CL ?
              bRegroup = 0; // if no, do not move to it - cause it would hide the contact
            }
            SAFE_FREE(&szGroup);
            SAFE_FREE(&szOldGroup);
          }

          if (bRegroup || bAdded)
          { // if we should load server details or contact was just added, update its group
            char* szGroup;

            if (szGroup = makeGroupPath(wGroupId))
            { // try to get Miranda Group path from groupid, if succeeded save to db
              DBWriteContactSettingString(hContact, "CList", "Group", szGroup);

              SAFE_FREE(&szGroup);
            }
          }

					if (pChain)
					{ // Look for nickname TLV and copy it to the db if necessary
						if (pTLV = getTLV(pChain, 0x0131, 1))
						{
							if (pTLV->pData && (pTLV->wLen > 0))
							{
								char* pszNick;
								WORD wNickLength;

								wNickLength = pTLV->wLen;

								pszNick = (char*)malloc(wNickLength + 1);
								if (pszNick)
								{
									char* pszTempNick = NULL; // Used for UTF-8 conversion

									// Copy buffer to utf-8 buffer
									memcpy(pszNick, pTLV->pData, wNickLength);
									pszNick[wNickLength] = 0; // Terminate string

									// Convert to ansi
									if (utf8_decode(pszNick, &pszTempNick))
									{
										SAFE_FREE(&pszNick);
										pszNick = pszTempNick;
										Netlib_Logf(ghServerNetlibUser, "Nickname is '%s'", pszNick);
									}
									else
									{
										Netlib_Logf(ghServerNetlibUser, "Failed to convert Nickname '%s' from UTF-8", pszNick);
									}

                  bNicked = 1;

									// Write nickname to database
									if (DBGetContactSettingByte(NULL, gpszICQProtoName, "LoadServerDetails", DEFAULT_SS_LOAD) || bAdded)
                  { // if just added contact, save details always - does no harm
										DBVARIANT dbv;

										if (!DBGetContactSetting(hContact,"CList","MyHandle",&dbv))
										{
											if ((lstrcmp(dbv.pszVal, pszNick)) && (strlen(pszNick) > 0))
											{
												// Yes, we really do need to delete it first. Otherwise the CLUI nick
												// cache isn't updated (I'll look into it)
												DBDeleteContactSetting(hContact,"CList","MyHandle");
												DBWriteContactSettingString(hContact, "CList", "MyHandle", pszNick);
											}
											DBFreeVariant(&dbv);
										}
										else if (strlen(pszNick) > 0)
										{
											DBDeleteContactSetting(hContact,"CList","MyHandle");
											DBWriteContactSettingString(hContact, "CList", "MyHandle", pszNick);
										}
									}
									SAFE_FREE(&pszNick);
								}

							}
							else
							{
								Netlib_Logf(ghServerNetlibUser, "Invalid nickname");
							}
						}
            if (bAdded && !bNicked)
              icq_QueueUser(hContact); // queue user without nick for fast auto info update

						// Look for comment TLV and copy it to the db if necessary
						if (pTLV = getTLV(pChain, 0x013C, 1))
						{
							if (pTLV->pData && (pTLV->wLen > 0))
							{

								char* pszComment;
								WORD wCommentLength;


								wCommentLength = pTLV->wLen;

								pszComment = (char*)malloc(wCommentLength + 1);
								if (pszComment)
								{

									char* pszTempComment = NULL; // Used for UTF-8 conversion


									// Copy buffer to utf-8 buffer
									memcpy(pszComment, pTLV->pData, wCommentLength);
									pszComment[wCommentLength] = 0; // Terminate string

									// Convert to ansi
									if (utf8_decode(pszComment, &pszTempComment))
									{
										SAFE_FREE(&pszComment);
										pszComment = pszTempComment;
										Netlib_Logf(ghServerNetlibUser, "Comment is '%s'", pszComment);
									}
									else
									{
										Netlib_Logf(ghServerNetlibUser, "Failed to convert Comment '%s' from UTF-8", pszComment);
									}

									// Write comment to database
									if (DBGetContactSettingByte(NULL, gpszICQProtoName, "LoadServerDetails", DEFAULT_SS_LOAD) || bAdded)
                  { // if just added contact, save details always - does no harm
										DBVARIANT dbv;

										if (!DBGetContactSetting(hContact,"UserInfo","MyNotes",&dbv))
										{
											if ((lstrcmp(dbv.pszVal, pszComment)) && (strlen(pszComment) > 0))
											{
												DBWriteContactSettingString(hContact, "UserInfo", "MyNotes", pszComment);
											}
											DBFreeVariant(&dbv);
										}
										else if (strlen(pszComment) > 0)
										{
											DBWriteContactSettingString(hContact, "UserInfo", "MyNotes", pszComment);
										}
									}

									SAFE_FREE(&pszComment);
								}
							}
							else
							{
								Netlib_Logf(ghServerNetlibUser, "Invalid comment");
							}
						}

						// Look for need-authorization TLV
						if (pTLV = getTLV(pChain, 0x0066, 1))
						{
							DBWriteContactSettingByte(hContact, gpszICQProtoName, "Auth", 1);
							Netlib_Logf(ghServerNetlibUser, "SSI contact need authorization");
						}
						else
						{
							DBWriteContactSettingByte(hContact, gpszICQProtoName, "Auth", 0);
						}
					}
				}
			}
			break;

    case SSI_ITEM_GROUP:
      if ((wGroupId == 0) && (wItemId == 0))
      {
        /* list of groups. wTlvType=1, data is TLV(C8) containing list of WORDs which */
        /* is the group ids
        /* we don't need to use this. Our processing is on-the-fly */
        /* this record is always sent first in the first packet only, */
      }
      else if (wGroupId != 0)
      {
        /* wGroupId != 0: a group record */
        if (wItemId == 0)
        { /* no item ID: this is a group */
          /* pszRecordName is the name of the group */
          char* pszName = NULL;
          WORD wNameLength;

					ReserveServerID(wGroupId, SSIT_GROUP);

          wNameLength = strlen(pszRecordName);

          pszName = (char*)malloc(wNameLength + 1);

          if (pszRecordName)
          {
            char* pszTempName = NULL; // Used for UTF-8 conversion

            // Copy buffer to utf-8 buffer
            memcpy(pszName, pszRecordName, wNameLength);
            pszName[wNameLength] = 0; // Terminate string

            // Convert to ansi
            if (utf8_decode(pszName, &pszTempName))
            {
              SAFE_FREE(&pszName);
              pszName = pszTempName;
            }
            else
            {
              Netlib_Logf(ghServerNetlibUser, "Failed to convert Groupname '%s' from UTF-8", pszName);
            }
          }
          setServerGroupName(wGroupId, pszName);

          Netlib_Logf(ghServerNetlibUser, "Group %s added to known groups.", pszName);

          SAFE_FREE(&pszName);
          /* demangle full grouppath, create groups, set it to known */
          pszName = makeGroupPath(wGroupId); 
          SAFE_FREE(&pszName);

          /* TLV contains a TLV(C8) with a list of WORDs of contained contact IDs */
          /* our processing is good enough that we don't need this duplication */
        }
        else
        {
          Netlib_Logf(ghServerNetlibUser, "Unhandled type 0x01, wItemID != 0");
        }
      }
      else
      {
        Netlib_Logf(ghServerNetlibUser, "Unhandled type 0x01");
      }
      break;

		case SSI_ITEM_PERMIT:
			/* item on visible list */
			/* wItemId not related to contact ID */
			/* pszRecordName is the UIN */
			if (!IsStringUIN(pszRecordName))
			{
				Netlib_Logf(ghServerNetlibUser, "Ignoring fake AOL contact, message is: \"%s\"", pszRecordName);
			}
			else
			{
				DWORD dwUin;
				char* pszProto;
				HANDLE hContact;

				// This looks like a real contact
				// Check if this user already exists in local list
				dwUin = atoi(pszRecordName);
				hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
				while (hContact)
				{
					pszProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
					if (pszProto && !strcmp(pszProto, gpszICQProtoName) && dwUin == DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0))
					{
						Netlib_Logf(ghServerNetlibUser, "Permit contact already exists '%d'", dwUin);
						break;
					}
					hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
				}

				if (hContact == NULL)
				{
					/* not already on list: add */
					Netlib_Logf(ghServerNetlibUser, "SSI adding new Permit contact '%d'", dwUin);
					hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD,0,0);
					if (!hContact)
					{
						Netlib_Logf(ghServerNetlibUser, "Failed to create ICQ contact %u", dwUin);
						if (pChain)
							disposeChain(&pChain);
						continue;
					}
					if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)gpszICQProtoName) != 0)
					{
						// For some reason we failed to register the protocol to this contact
						CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
						Netlib_Logf(ghServerNetlibUser, "Failed to register ICQ contact %u", dwUin);
						if (pChain)
							disposeChain(&pChain);
						continue;
					}
					DBWriteContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, dwUin);
					// It wasn't previously in the list, we hide it so it only appears in the visible list
					DBWriteContactSettingByte(hContact, "CList", "Hidden", 1);
          // Add it to the list, so it can be added properly if proper contact
          AddJustAddedContact(hContact);
				}

				// Save permit ID
				DBWriteContactSettingWord(hContact, gpszICQProtoName, "SrvPermitId", wItemId);
				ReserveServerID(wItemId, SSIT_ITEM);
				// Set apparent mode
				DBWriteContactSettingWord(hContact, gpszICQProtoName, "ApparentMode", ID_STATUS_ONLINE);
				Netlib_Logf(ghServerNetlibUser, "Visible-contact (%u)", dwUin);

			}
			break;

		case SSI_ITEM_DENY: // Item on invisible list
			if (!IsStringUIN(pszRecordName))
			{
				Netlib_Logf(ghServerNetlibUser, "Ignoring fake AOL contact, message is: \"%s\"", pszRecordName);
			}
			else
			{
				DWORD dwUin;
				char* pszProto;
				HANDLE hContact;


				// This looks like a real contact
				// Check if this user already exists in local list
				dwUin = atoi(pszRecordName);
				hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
				while (hContact)
				{
					pszProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
					if (pszProto && !strcmp(pszProto, gpszICQProtoName) && (dwUin == DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0)))
					{
						Netlib_Logf(ghServerNetlibUser, "Deny contact already exists '%d'", dwUin);
						break;
					}
					hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
				}

				if (hContact == NULL)
				{
					/* not already on list: add */
					Netlib_Logf(ghServerNetlibUser, "SSI adding new Deny contact '%d'", dwUin);
					hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD,0,0);
					if (!hContact)
					{
						Netlib_Logf(ghServerNetlibUser, "Failed to create ICQ contact %u", dwUin);
						if (pChain)
							disposeChain(&pChain);
						continue;
					}
					if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)gpszICQProtoName) != 0)
					{
						// For some reason we failed to register the protocol to this contact
						CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
						Netlib_Logf(ghServerNetlibUser, "Failed to register ICQ contact %u", dwUin);
						if (pChain)
							disposeChain(&pChain);
						continue;
					}
					DBWriteContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, dwUin);
					// It wasn't previously in the list, we hide it so it only appears in the visible list
					DBWriteContactSettingByte(hContact, "CList", "Hidden", 1);
          // Add it to the list, so it can be added properly if proper contact
          AddJustAddedContact(hContact);
				}

				// Save Deny ID
				DBWriteContactSettingWord(hContact, gpszICQProtoName, "SrvDenyId", wItemId);
				ReserveServerID(wItemId, SSIT_ITEM);

				// Set apparent mode
				DBWriteContactSettingWord(hContact, gpszICQProtoName, "ApparentMode", ID_STATUS_OFFLINE);
				Netlib_Logf(ghServerNetlibUser, "Invisible-contact (%u)", dwUin);
			}
			break;


    case SSI_ITEM_VISIBILITY: /* My visibility settings */
      {
        BYTE bVisibility;

        ReserveServerID(wItemId, SSIT_ITEM);

        // Look for visibility TLV
        if (bVisibility = getByteFromChain(pChain, 0x00CA, 1))
        { // found it, store the id, we do not need current visibility - we do not rely on it
          DBWriteContactSettingWord(NULL, gpszICQProtoName, "SrvVisibilityID", wItemId);
          Netlib_Logf(ghServerNetlibUser, "Visibility is %u, ID is %u", bVisibility, wItemId);
        }
      }
      break;

    case SSI_ITEM_IGNORE: // item on ignore list 
      /* pszRecordName is the UIN */

      if (!IsStringUIN(pszRecordName))
        Netlib_Logf(ghServerNetlibUser, "Ignoring fake AOL contact, message is: \"%s\"", pszRecordName);
      else
      {
        DWORD dwUin;
        char* pszProto;
        HANDLE hContact;

				// This looks like a real contact
				// Check if this user already exists in local list
				dwUin = atoi(pszRecordName);
				hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
				while (hContact)
				{
					pszProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
					if (pszProto && !strcmp(pszProto, gpszICQProtoName) && (dwUin == DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0)))
					{
						Netlib_Logf(ghServerNetlibUser, "Ignore contact already exists '%d'", dwUin);
						break;
					}
					hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
				}

				if (hContact == NULL)
				{
					/* not already on list: add */
					Netlib_Logf(ghServerNetlibUser, "SSI adding new Ignore contact '%d'", dwUin);
					hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD,0,0);
					if (!hContact)
					{
						Netlib_Logf(ghServerNetlibUser, "Failed to create ICQ contact %u", dwUin);
						if (pChain)
							disposeChain(&pChain);
						continue;
					}
					if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)gpszICQProtoName) != 0)
					{
						// For some reason we failed to register the protocol to this contact
						CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
						Netlib_Logf(ghServerNetlibUser, "Failed to register ICQ contact %u", dwUin);
						if (pChain)
							disposeChain(&pChain);
						continue;
					}
					DBWriteContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, dwUin);
					// It wasn't previously in the list, we hide it
					DBWriteContactSettingByte(hContact, "CList", "Hidden", 1);
          // Add it to the list, so it can be added properly if proper contact
          AddJustAddedContact(hContact);
				}

				// Save Ignore ID
				DBWriteContactSettingWord(hContact, gpszICQProtoName, "SrvIgnoreId", wItemId);
				ReserveServerID(wItemId, SSIT_ITEM);

				// Set apparent mode & ignore
        DBWriteContactSettingWord(hContact, gpszICQProtoName, "ApparentMode", ID_STATUS_OFFLINE);
        // set ignore all events
        DBWriteContactSettingDword(hContact, "Ignore", "Mask1", 0xFFFF);
        Netlib_Logf(ghServerNetlibUser, "Ignore-contact (%u)", dwUin);
      }
			break;

		case SSI_ITEM_UNKNOWN2:
			Netlib_Logf(ghServerNetlibUser, "SSI unknown type 0x11");
			break;

		case SSI_ITEM_IMPORT:
			if ((wGroupId == 0) && (wItemId == 1))
			{
				/* time our list was first imported */
				/* pszRecordName is "Import Time" */
				/* data is TLV(13) {TLV(D4) {time_t importTime}} */
				/* not processed yet (if ever) */
				Netlib_Logf(ghServerNetlibUser, "SSI first import seen");
			}
			break;

    case SSI_ITEM_BUDDYICON:
      if (wGroupId == 0)
      {
        /* our avatar MD5-hash */
        /* pszRecordName is "1" */
        /* data is TLV(D5) hash */
        /* we ignore this, just save the id */
        /* cause we get the hash again after login */
        ReserveServerID(wItemId, SSIT_ITEM);
        DBWriteContactSettingWord(NULL, gpszICQProtoName, "SrvAvatarID", wItemId);
        Netlib_Logf(ghServerNetlibUser, "SSI Avatar item recognized");
      }
      break;

		case SSI_ITEM_UNKNOWN1:
      if (wGroupId == 0)
      {
        /* ICQ2k ShortcutBar Items */
        /* data is TLV(CD) text */
      }

		default:
			Netlib_Logf(ghServerNetlibUser, "SSI unhandled item %2x", wTlvType);
			break;
		}

		if (pChain)
			disposeChain(&pChain);

	} // end for

	Netlib_Logf(ghServerNetlibUser, "Bytes left: %u", wLen);

	SAFE_FREE(&pszRecordName);

	DBWriteContactSettingWord(NULL, gpszICQProtoName, "SrvRecordCount", (WORD)(wRecord + DBGetContactSettingWord(NULL, gpszICQProtoName, "SrvRecordCount", 0)));

	if (bIsLastPacket)
	{
		// No contacts left to sync
		bIsSyncingCL = FALSE;

    icq_RescanInfoUpdate();

		if (wLen >= 4)
		{
			DWORD dwLastUpdateTime;

			/* finally we get a time_t of the last update time */
			unpackDWord(&buf, &dwLastUpdateTime);
			DBWriteContactSettingDword(NULL, gpszICQProtoName, "SrvLastUpdate", dwLastUpdateTime);
			Netlib_Logf(ghServerNetlibUser, "Last update of server list was (%u) %s", dwLastUpdateTime, asctime(localtime(&dwLastUpdateTime)));

			sendRosterAck();
			handleServUINSettings(wListenPort, dwLocalInternalIP);
		}
		else
		{
			Netlib_Logf(ghServerNetlibUser, "Last packet missed update time...");
		}
    if (DBGetContactSettingWord(NULL, gpszICQProtoName, "SrvRecordCount", 0) == 0)
    { // we got empty serv-list, create master group
      servlistcookie* ack = (servlistcookie*)malloc(sizeof(servlistcookie));
      if (ack)
      { 
        DWORD seq;

        ack->dwAction = SSA_GROUP_UPDATE;
        ack->hContact = NULL;
        ack->wContactId = 0;
        ack->wGroupId = 0;
        ack->szGroupName = "";
        seq = AllocateCookie(ICQ_LISTS_ADDTOLIST, 0, ack);
        icq_sendGroup(seq, ICQ_LISTS_ADDTOLIST, 0, ack->szGroupName, NULL, 0);
      }
    }
    // serv-list sync finished, clear just added contacts
    FlushJustAddedContacts(); 
  }
  else
  {
    Netlib_Logf(ghServerNetlibUser, "Waiting for more packets");
  }
}



static void handleRecvAuthRequest(unsigned char *buf, WORD wLen)
{
	BYTE nUinLen;
	BYTE szUin[10];
	WORD wReasonLen;
	DWORD dwUin;
	HANDLE hcontact;
	CCSDATA ccs;
	PROTORECVEVENT pre;
  char* szReason;
  int nReasonLen;
  char* szNick;
  int nNickLen;
	char* szBlob;
	char* pCurBlob;
  DBVARIANT dbv;

	unpackByte(&buf, &nUinLen);
	wLen -= 1;

	if (nUinLen > wLen)
		return;

	unpackString(&buf, szUin, nUinLen);
	wLen -= nUinLen;
	szUin[nUinLen] = '\0';

	if (!IsStringUIN(szUin))
		return;

	dwUin = atoi(szUin);

	unpackWord(&buf, &wReasonLen);
	wLen -= 2;
	if (wReasonLen > wLen)
		return;

	ccs.szProtoService=PSR_AUTH;
	ccs.hContact=hcontact=HContactFromUIN(dwUin,1);
	ccs.wParam=0;
	ccs.lParam=(LPARAM)&pre;
	pre.flags=0;
	pre.timestamp=time(NULL);
	pre.lParam=sizeof(DWORD)+sizeof(HANDLE)+wReasonLen+5;
  szReason = (char*)malloc(wReasonLen+1);
  if (szReason)
  {
    memcpy(szReason, buf, wReasonLen);
    szReason[wReasonLen] = '\0';
    szReason = detect_decode_utf8(szReason); // detect & decode UTF-8
  }
  nReasonLen = strlennull(szReason);
  // Read nick name from DB
  if (DBGetContactSetting(ccs.hContact, gpszICQProtoName, "Nick", &dbv))
    nNickLen = 0;
  else
  {
    szNick = dbv.pszVal;
    nNickLen = strlennull(szNick);
  }
  pre.lParam += nNickLen + nReasonLen;

  DBWriteContactSettingByte(ccs.hContact, gpszICQProtoName, "Grant", 1);

	/*blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)*/
	pCurBlob=szBlob=(char *)malloc(pre.lParam);
	memcpy(pCurBlob,&dwUin,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
	memcpy(pCurBlob,&hcontact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
  if (nNickLen) 
  { // if we have nick we add it, otherwise keep trailing zero
    memcpy(pCurBlob, szNick, nNickLen);
    pCurBlob+=nNickLen;
  }
	*(char *)pCurBlob = 0; pCurBlob++;
	*(char *)pCurBlob = 0; pCurBlob++;
	*(char *)pCurBlob = 0; pCurBlob++;
	*(char *)pCurBlob = 0; pCurBlob++;
  if (nReasonLen)
  {
    memcpy(pCurBlob, szReason, nReasonLen);
    pCurBlob += nReasonLen;
  }
  else
  {
	  memcpy(pCurBlob, buf, wReasonLen); 
    pCurBlob += wReasonLen;
  }
	*(char *)pCurBlob = 0;
	pre.szMessage=(char *)szBlob;

// TODO: Change for new auth system, include all known informations
	CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);

  SAFE_FREE(&szReason);
  DBFreeVariant(&dbv);
}



static void handleRecvAdded(unsigned char *buf, WORD wLen)
{
	BYTE nUinLen;
	BYTE szUin[10];
	DWORD dwUin;
	DBEVENTINFO dbei;
	PBYTE pCurBlob;
	HANDLE hContact;


	unpackByte(&buf, &nUinLen);
	wLen -= 1;

	if (nUinLen > wLen)
		return;

	unpackString(&buf, szUin, nUinLen);
	wLen -= nUinLen;
	szUin[nUinLen] = '\0';

	if (!IsStringUIN(szUin))
		return;

	dwUin = atoi(szUin);
	hContact=HContactFromUIN(dwUin,1);

  DBDeleteContactSetting(hContact, gpszICQProtoName, "Grant");

	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.szModule=gpszICQProtoName;
	dbei.timestamp=time(NULL);
	dbei.flags=0;
	dbei.eventType=EVENTTYPE_ADDED;
	dbei.cbBlob=sizeof(DWORD)+sizeof(HANDLE)+4;
	pCurBlob=dbei.pBlob=(PBYTE)malloc(dbei.cbBlob);
	/*blob is: uin(DWORD), hContact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ) */
	memcpy(pCurBlob,&dwUin,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
	memcpy(pCurBlob,&hContact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
	*(char *)pCurBlob = 0; pCurBlob++;
	*(char *)pCurBlob = 0; pCurBlob++;
	*(char *)pCurBlob = 0; pCurBlob++;
	*(char *)pCurBlob = 0;
// TODO: Change for new auth system

	CallService(MS_DB_EVENT_ADD,(WPARAM)(HANDLE)NULL,(LPARAM)&dbei);
}



static void handleRecvAuthResponse(unsigned char *buf, WORD wLen)
{
	BYTE nUinLen;
	BYTE bResponse;
	BYTE szUin[10];
	DWORD dwUin;
	HANDLE hContact;
	char* szNick;
	WORD nReasonLen;
	char* szReason;

	bResponse = 0xFF;

	unpackByte(&buf, &nUinLen);
	wLen -= 1;

	if (nUinLen > wLen)
		return;

	unpackString(&buf, szUin, nUinLen);
	wLen -= nUinLen;
	szUin[nUinLen] = '\0';

	if (!IsStringUIN(szUin))
		return;

	dwUin = atoi(szUin);
	hContact = HContactFromUIN(dwUin, 1);

	if (hContact != INVALID_HANDLE_VALUE) szNick = NickFromHandle(hContact);

	if (wLen > 0)
	{
	  unpackByte(&buf, &bResponse);
	  wLen -= 1;
	}
	if (wLen >= 2)
	{
	  unpackWord(&buf, &nReasonLen);
	  wLen -= 2;
	  if (wLen >= nReasonLen)
		{
			szReason = malloc(nReasonLen+1);
			unpackString(&buf, szReason, nReasonLen);
			szReason[nReasonLen] = '\0';
		}
	}

	switch (bResponse)
	{

	case 0:
		DBWriteContactSettingByte(hContact, gpszICQProtoName, "Auth", 1);
		Netlib_Logf(ghServerNetlibUser, "Authorisation request denied by %u", dwUin);
		// TODO: Add to system history as soon as new auth system is ready
		break;

	case 1:
		DBWriteContactSettingByte(hContact, gpszICQProtoName, "Auth", 0);
		Netlib_Logf(ghServerNetlibUser, "Authorisation request granted by %u", dwUin);
		// TODO: Add to system history as soon as new auth system is ready
		break;

	default:
		Netlib_Logf(ghServerNetlibUser, "Unknown Authorisation request response (%u) from %u", bResponse, dwUin);
		break;

	}
	SAFE_FREE(&szNick);
	SAFE_FREE(&szReason);
}



// Updates the visibility code used while in SSI mode. If a server ID is
// not stored in the local DB, a new ID will be added to the server list.
//
// Possible values are:
//   01 - Allow all users to see you
//   02 - Block all users from seeing you
//   03 - Allow only users in the permit list to see you
//   04 - Block only users in the invisible list from seeing you
//   05 - Allow only users in the buddy list to see you
//
void updateServVisibilityCode(BYTE bCode)
{
  icq_packet packet;
  WORD wVisibilityID;
  WORD wCommand;

  if ((bCode > 0) && (bCode < 6))
  {
    servlistcookie* ack;
    DWORD dwCookie;
    BYTE bVisibility = DBGetContactSettingByte(NULL, gpszICQProtoName, "SrvVisibility", 0);

    if (bVisibility == bCode) // if no change was made, not necescary to update that
      return;
    DBWriteContactSettingByte(NULL, gpszICQProtoName, "SrvVisibility", bCode);

    // Do we have a known server visibility ID? We should, unless we just subscribed to the serv-list for the first time
    if ((wVisibilityID = DBGetContactSettingWord(NULL, gpszICQProtoName, "SrvVisibilityID", 0)) == 0)
    {
      // No, create a new random ID
      wVisibilityID = GenerateServerId(SSIT_ITEM);
      DBWriteContactSettingWord(NULL, gpszICQProtoName, "SrvVisibilityID", wVisibilityID);
      wCommand = ICQ_LISTS_ADDTOLIST;
      Netlib_Logf(ghServerNetlibUser, "Made new srvVisibilityID, id is %u, code is %u", wVisibilityID, bCode);
    }
    else
    {
      Netlib_Logf(ghServerNetlibUser, "Reused srvVisibilityID, id is %u, code is %u", wVisibilityID, bCode);
      wCommand = ICQ_LISTS_UPDATEGROUP;
    }

    ack = (servlistcookie*)malloc(sizeof(servlistcookie));
    if (!ack) 
    {
      Netlib_Logf(ghServerNetlibUser, "Cookie alloc failure.");
      return; // out of memory, go away
    }
    ack->dwAction = 1; // update visibility
    ack->dwUin = 0; // this is ours
    dwCookie = AllocateCookie(wCommand, 0, ack); // take cookie

    // Build and send packet
    packet.wLen = 25;
    write_flap(&packet, ICQ_DATA_CHAN);
    packFNACHeader(&packet, ICQ_LISTS_FAMILY, wCommand, 0, dwCookie);
    packWord(&packet, 0);                   // Name (null)
    packWord(&packet, 0);                   // GroupID (0 if not relevant)
    packWord(&packet, wVisibilityID);       // EntryID
    packWord(&packet, SSI_ITEM_VISIBILITY); // EntryType
    packWord(&packet, 5);                   // Length in bytes of following TLV
    packWord(&packet, 0xCA);                // TLV Type
    packWord(&packet, 0x1);                 // TLV Length
    packByte(&packet, bCode);               // TLV Value (visibility code)
    sendServPacket(&packet);
    // There is no need to send ICQ_LISTS_CLI_MODIFYSTART or
    // ICQ_LISTS_CLI_MODIFYEND when modifying the visibility code
  }
}



// Updates the avatar hash used while in SSI mode. If a server ID is
// not stored in the local DB, a new ID will be added to the server list.
void updateServAvatarHash(char* pHash, int size)
{
  icq_packet packet;
  WORD wAvatarID;
  WORD wCommand;

  if (pHash)
  {
    servlistcookie* ack;
    DWORD dwCookie;

    // Do we have a known server avatar ID? We should, unless we just subscribed to the serv-list for the first time
    if ((wAvatarID = DBGetContactSettingWord(NULL, gpszICQProtoName, "SrvAvatarID", 0)) == 0)
    {
      // No, create a new random ID
      wAvatarID = GenerateServerId(SSIT_ITEM);
      wCommand = ICQ_LISTS_ADDTOLIST;
      Netlib_Logf(ghServerNetlibUser, "Made new srvAvatarID, id is %u", wAvatarID);
    }
    else
    {
      Netlib_Logf(ghServerNetlibUser, "Reused srvAvatarID, id is %u", wAvatarID);
      wCommand = ICQ_LISTS_UPDATEGROUP;
    }

    ack = (servlistcookie*)malloc(sizeof(servlistcookie));
    if (!ack) 
    {
      Netlib_Logf(ghServerNetlibUser, "Cookie alloc failure.");
      return; // out of memory, go away
    }
    ack->dwAction = SSA_SETAVATAR; // update avatar hash
    ack->dwUin = 0; // this is ours
    ack->wContactId = wAvatarID;
    dwCookie = AllocateCookie(wCommand, 0, ack); // take cookie

    // Build and send packet
    packet.wLen = 29 + size;
    write_flap(&packet, ICQ_DATA_CHAN);
    packFNACHeader(&packet, ICQ_LISTS_FAMILY, wCommand, 0, dwCookie);
    packWord(&packet, 1);                   // Name length
    packByte(&packet, '1');                 // Name
    packWord(&packet, 0);                   // GroupID (0 if not relevant)
    packWord(&packet, wAvatarID);           // EntryID
    packWord(&packet, SSI_ITEM_BUDDYICON);  // EntryType
    packWord(&packet, (WORD)(0x8 + size));          // Length in bytes of following TLV
    packWord(&packet, 0x131);               // TLV Type (Name)
    packWord(&packet, 0);                   // TLV Length (empty)
    packWord(&packet, 0xD5);                // TLV Type
    packWord(&packet, (WORD)size);                // TLV Length
    packBuffer(&packet, pHash, (WORD)size);       // TLV Value (avatar hash)
    sendServPacket(&packet);
    // There is no need to send ICQ_LISTS_CLI_MODIFYSTART or
    // ICQ_LISTS_CLI_MODIFYEND when modifying the avatar hash
  }
  else
  {
    servlistcookie* ack;
    DWORD dwCookie;

    // Do we have a known server avatar ID?
    if ((wAvatarID = DBGetContactSettingWord(NULL, gpszICQProtoName, "SrvAvatarID", 0)) == 0)
    {
      return;
    }
    ack = (servlistcookie*)malloc(sizeof(servlistcookie));
    if (!ack) 
    {
      Netlib_Logf(ghServerNetlibUser, "Cookie alloc failure.");
      return; // out of memory, go away
    }
    ack->dwAction = SSA_REMOVEAVATAR; // update avatar hash
    ack->dwUin = 0; // this is ours
    ack->wContactId = wAvatarID;
    dwCookie = AllocateCookie(ICQ_LISTS_REMOVEFROMLIST, 0, ack); // take cookie

    // Build and send packet
    packet.wLen = 20;
    write_flap(&packet, ICQ_DATA_CHAN);
    packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_REMOVEFROMLIST, 0, dwCookie);
    packWord(&packet, 0);                   // Name (null)
    packWord(&packet, 0);                   // GroupID (0 if not relevant)
    packWord(&packet, wAvatarID);           // EntryID
    packWord(&packet, SSI_ITEM_BUDDYICON);  // EntryType
    packWord(&packet, 0);                   // Length in bytes of following TLV
    sendServPacket(&packet);
    // There is no need to send ICQ_LISTS_CLI_MODIFYSTART or
    // ICQ_LISTS_CLI_MODIFYEND when modifying the avatar hash
  }
}



// Should be called before the server list is modified. When all
// modifications are done, call sendAddEnd().
void sendAddStart(int bImport)
{
  icq_packet packet;

  packet.wLen = bImport?14:10;
  write_flap(&packet, ICQ_DATA_CHAN);
  packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_CLI_MODIFYSTART, 0, ICQ_LISTS_CLI_MODIFYSTART<<0x10);
  if (bImport) packDWord(&packet, 1<<0x10); 
  sendServPacket(&packet);
}



// Should be called after the server list has been modified to inform
// the server that we are done.
void sendAddEnd(void)
{
  icq_packet packet;

  packet.wLen = 10;
  write_flap(&packet, ICQ_DATA_CHAN);
  packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_CLI_MODIFYEND, 0, ICQ_LISTS_CLI_MODIFYEND<<0x10);
  sendServPacket(&packet);
}



// Sent when the last roster packet has been received
void sendRosterAck(void)
{
  icq_packet packet;

  packet.wLen = 10;
  write_flap(&packet, ICQ_DATA_CHAN);
  packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_GOTLIST, 0, ICQ_LISTS_GOTLIST<<0x10);
  sendServPacket(&packet);

#ifdef _DEBUG
  Netlib_Logf(ghServerNetlibUser, "Sent SNAC(x13,x07) - CLI_ROSTERACK");
#endif
}
