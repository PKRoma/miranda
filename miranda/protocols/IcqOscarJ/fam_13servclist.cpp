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

#include "icqoscar.h"


extern WORD wListenPort;

extern void setUserInfo();
extern int GroupNameExistsUtf(const char *name,int skipGroup);

BOOL bIsSyncingCL = FALSE;

static HANDLE HContactFromRecordName(char* szRecordName, int *bAdded);

static int getServerDataFromItemTLV(oscar_tlv_chain* pChain, unsigned char *buf);

static int unpackServerListItem(unsigned char** pbuf, WORD* pwLen, char* pszRecordName, WORD* pwGroupId, WORD* pwItemId, WORD* pwItemType, WORD* pwTlvLength);

static void handleServerCListAck(servlistcookie* sc, WORD wError);
static void handleServerCList(unsigned char *buf, WORD wLen, WORD wFlags, serverthread_info *info);
static void handleRecvAuthRequest(unsigned char *buf, WORD wLen);
static void handleRecvAuthResponse(unsigned char *buf, WORD wLen);
static void handleRecvAdded(unsigned char *buf, WORD wLen);
void sendRosterAck(void);


static WORD swapWord(WORD val)
{
  return (val & 0xFF)<<8 | (val>>8);
}



void handleServClistFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader, serverthread_info *info)
{
  switch (pSnacHeader->wSubtype)
  {

  case ICQ_LISTS_ACK: // UPDATE_ACK
    if (wBufferLength >= 2)
    {
      WORD wError;
      servlistcookie* sc;

      unpackWord(&pBuffer, &wError);

      if (FindCookie(pSnacHeader->dwRef, NULL, (void**)&sc))
      { // look for action cookie
#ifdef _DEBUG
        NetLog_Server("Received expected server list ack, action: %d, result: %d", sc->dwAction, wError);
#endif
        FreeCookie(pSnacHeader->dwRef); // release cookie

        handleServerCListAck(sc, wError);
      }
      else
      {
        NetLog_Server("Received unexpected server list ack %u", wError);
      }
    }
    break;

  case ICQ_LISTS_SRV_REPLYLISTS:
    {
      /* received list rights, we just skip them */

      oscar_tlv_chain* chain;

      if (chain = readIntoTLVChain(&pBuffer, wBufferLength, 0))
      {
        oscar_tlv* pTLV;

        if ((pTLV = getTLV(chain, 0x04, 1)) && pTLV->wLen>=30)
        { // limits for item types
          WORD* pMax = (WORD*)pTLV->pData;

          NetLog_Server("SSI: Max %d contacts, %d groups, %d permit, %d deny, %d ignore items.", swapWord(pMax[0]), swapWord(pMax[1]), swapWord(pMax[2]), swapWord(pMax[3]), swapWord(pMax[14]));
        }

        disposeChain(&chain);
      }
#ifdef _DEBUG
      NetLog_Server("Server sent SNAC(x13,x03) - SRV_REPLYLISTS");
#endif
    }
    break;

  case ICQ_LISTS_LIST: // SRV_REPLYROSTER
  {
    servlistcookie* sc;
    BOOL blWork;

    blWork = bIsSyncingCL;
    bIsSyncingCL = TRUE; // this is not used if cookie takes place

    if (FindCookie(pSnacHeader->dwRef, NULL, (void**)&sc))
    { // we do it by reliable cookie
      if (!sc->dwUin)
      { // is this first packet ?
        ResetSettingsOnListReload();
        sc->dwUin = 1;
      }
      handleServerCList(pBuffer, wBufferLength, pSnacHeader->wFlags, info);
      if (!(pSnacHeader->wFlags & 0x0001))
      { // was that last packet ?
        ReleaseCookie(pSnacHeader->dwRef); // yes, release cookie
      }
    }
    else
    { // use old fake
      if (!blWork)
      { // this can fail on some crazy situations
        ResetSettingsOnListReload();
      }
      handleServerCList(pBuffer, wBufferLength, pSnacHeader->wFlags, info);
    }
    break;
  }

  case ICQ_LISTS_UPTODATE: // SRV_REPLYROSTEROK
  {
    servlistcookie* sc;

    bIsSyncingCL = FALSE;

    if (FindCookie(pSnacHeader->dwRef, NULL, (void**)&sc))
    { // we requested servlist check
#ifdef _DEBUG
      NetLog_Server("Server stated roster is ok.");
#endif
      ReleaseCookie(pSnacHeader->dwRef);
      LoadServerIDs();
    }
    else
      NetLog_Server("Server sent unexpected SNAC(x13,x0F) - SRV_REPLYROSTEROK");

    // This will activate the server side list
    sendRosterAck(); // this must be here, cause of failures during cookie alloc
    handleServUINSettings(wListenPort, info);
    break;
  }

  case ICQ_LISTS_CLI_MODIFYSTART:
    NetLog_Server("Server sent SNAC(x13,x%02x) - %s", 0x11, "Server is modifying contact list");
    break;

  case ICQ_LISTS_CLI_MODIFYEND:
    NetLog_Server("Server sent SNAC(x13,x%02x) - %s", 0x12, "End of server modification");
    break;

  case ICQ_LISTS_UPDATEGROUP:
    if (wBufferLength >= 10)
    {
      WORD wGroupId, wItemId, wItemType, wTlvLen;
      uid_str szUID;

      if (unpackServerListItem(&pBuffer, &wBufferLength, szUID, &wGroupId, &wItemId, &wItemType, &wTlvLen))
      {
        HANDLE hContact = HContactFromRecordName(szUID, NULL);

        if (wBufferLength >= wTlvLen && hContact != INVALID_HANDLE_VALUE && wItemType == SSI_ITEM_BUDDY)
        { // a contact was updated on server
          if (wTlvLen > 0)
          { // parse details
            oscar_tlv_chain *pChain = readIntoTLVChain(&pBuffer, (WORD)(wTlvLen), 0);

            if (pChain) 
            {
              oscar_tlv* pAuth = getTLV(pChain, SSI_TLV_AWAITING_AUTH, 1);
              BYTE bAuth = ICQGetContactSettingByte(hContact, "Auth", 0);

              if (bAuth && !pAuth)
              { // server authorized our contact
                char str[MAX_PATH];
                char msg[MAX_PATH];
                char *nick = NickFromHandleUtf(hContact);

                ICQWriteContactSettingByte(hContact, "Auth", 0);
                null_snprintf(str, MAX_PATH, ICQTranslateUtfStatic("Contact \"%s\" was authorized in the server list.", msg, MAX_PATH), nick);
                icq_LogMessage(LOG_WARNING, str);
                SAFE_FREE((void**)&nick);
              }
              else if (!bAuth && pAuth)
              { // server took away authorization of our contact
                char str[MAX_PATH];
                char msg[MAX_PATH];
                char *nick = NickFromHandleUtf(hContact);

                ICQWriteContactSettingByte(hContact, "Auth", 1);
                null_snprintf(str, MAX_PATH, ICQTranslateUtfStatic("Contact \"%s\" lost its authorization in the server list.", msg, MAX_PATH), nick);
                icq_LogMessage(LOG_WARNING, str);
                SAFE_FREE((void**)&nick);
              }

              { // update server's data - otherwise consequent operations can fail with 0x0E
                unsigned char* data = (unsigned char*)_alloca(wTlvLen);
                int datalen = getServerDataFromItemTLV(pChain, data);

                if (datalen > 0)
                  ICQWriteContactSettingBlob(hContact, "ServerData", data, datalen);
                else
                  ICQDeleteContactSetting(hContact, "ServerData");
              }

              disposeChain(&pChain);

              break;
            }
          }
        }
      }
    }
    NetLog_Server("Server sent SNAC(x13,x%02x) - %s", 0x09, "Server updated our contact on list");
    break;

  case ICQ_LISTS_REMOVEFROMLIST:
    if (wBufferLength >= 10)
    {
      WORD wGroupId, wItemId, wItemType;
      uid_str szUID;

      if (unpackServerListItem(&pBuffer, &wBufferLength, szUID, &wGroupId, &wItemId, &wItemType, NULL))
      {
        HANDLE hContact = HContactFromRecordName(szUID, NULL);

        if (hContact != INVALID_HANDLE_VALUE && wItemType == SSI_ITEM_BUDDY)
        { // a contact was removed from our list
          ICQDeleteContactSetting(hContact, "ServerId");
          ICQDeleteContactSetting(hContact, "SrvGroupId");
          ICQDeleteContactSetting(hContact, "Auth");
          icq_sendNewContact(0, szUID); // add to CS to see him
          {
            char str[MAX_PATH];
            char msg[MAX_PATH];
            char *nick = NickFromHandleUtf(hContact);

            null_snprintf(str, MAX_PATH, ICQTranslateUtfStatic("User \"%s\" was removed from server list.", msg, MAX_PATH), nick);
            icq_LogMessage(LOG_WARNING, str);
            SAFE_FREE((void**)&nick);
          }
        }
      }
    }
    NetLog_Server("Server sent SNAC(x13,x%02x) - %s", 0x0A, "Server removed something from our list");
    break;

  case ICQ_LISTS_ADDTOLIST:
    if (wBufferLength >= 10)
    {
      WORD wGroupId, wItemId, wItemType, wTlvLen;

      if (unpackServerListItem(&pBuffer, &wBufferLength, NULL, &wGroupId, &wItemId, &wItemType, &wTlvLen))
      {
        if (wBufferLength >= wTlvLen && wItemType == SSI_ITEM_IMPORTTIME)
        {
          if (wTlvLen > 0)
          { // parse timestamp
            oscar_tlv_chain *pChain = readIntoTLVChain(&pBuffer, (WORD)(wTlvLen), 0);

            if (pChain) 
            {
              ICQWriteContactSettingDword(NULL, "ImportTS", getDWordFromChain(pChain, SSI_TLV_TIMESTAMP, 1));
              ICQWriteContactSettingWord(NULL, "SrvImportID", wItemId);
              disposeChain(&pChain);

              NetLog_Server("Server added Import timestamp to list");

              break;
            }
          }
        }
      }
    }
    NetLog_Server("Server sent SNAC(x13,x%02x) - %s", 0x08, "Server added something to our list");
    break;

  case ICQ_LISTS_AUTHREQUEST:
    handleRecvAuthRequest(pBuffer, wBufferLength);
    break;

  case ICQ_LISTS_SRV_AUTHRESPONSE:
    handleRecvAuthResponse(pBuffer, wBufferLength);
    break;

  case ICQ_LISTS_AUTHGRANTED:
    NetLog_Server("Server sent SNAC(x13,x%02x) - %s", 0x15, "User granted us future authorization");
    break;

  case ICQ_LISTS_YOUWEREADDED:
    handleRecvAdded(pBuffer, wBufferLength);
    break;

  case ICQ_LISTS_ERROR:
    if (wBufferLength >= 2)
    {
      WORD wError;
      servlistcookie* sc;

      unpackWord(&pBuffer, &wError);

      if (FindCookie(pSnacHeader->dwRef, NULL, (void**)&sc))
      { // look for action cookie
#ifdef _DEBUG
        NetLog_Server("Received server list error, action: %d, result: %d", sc->dwAction, wError);
#endif
        FreeCookie(pSnacHeader->dwRef); // release cookie

        if (sc->dwAction==SSA_CHECK_ROSTER)
        { // the serv-list is unavailable turn it off
          icq_LogMessage(LOG_ERROR, "Server contact list is unavailable, Miranda will use local contact list.");
          gbSsiEnabled = 0;
          handleServUINSettings(wListenPort, info);
        }
        SAFE_FREE((void**)&sc);
      }
      else
      {
        LogFamilyError(ICQ_LISTS_FAMILY, wError);
      }
    }
    break;

  default:
    NetLog_Server("Warning: Ignoring SNAC(x%02x,x%02x) - Unknown SNAC (Flags: %u, Ref: %u)", ICQ_LISTS_FAMILY, pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;
  }
}



static int unpackServerListItem(unsigned char** pbuf, WORD* pwLen, char* pszRecordName, WORD* pwGroupId, WORD* pwItemId, WORD* pwItemType, WORD* pwTlvLength)
{
  WORD wRecordNameLen;

  // The name of the entry. If this is a group header, then this
  // is the name of the group. If it is a plain contact list entry,
  // then it's the UIN of the contact.
  unpackWord(pbuf, &wRecordNameLen);
  if (*pwLen < 10 + wRecordNameLen || wRecordNameLen >= MAX_PATH)
    return 0; // Failure

  unpackString(pbuf, pszRecordName, wRecordNameLen);
  if (pszRecordName)
    pszRecordName[wRecordNameLen] = '\0';

  // The group identifier this entry belongs to. If 0, this is meta information or
  // a contact without a group
  unpackWord(pbuf, pwGroupId);

  // The ID of this entry. Group headers have ID 0. Otherwise, this
  // is a random number generated when the user is added to the
  // contact list, or when the user is ignored. See CLI_ADDBUDDY.
  unpackWord(pbuf, pwItemId);

  // This field indicates what type of entry this is
  unpackWord(pbuf, pwItemType);

  // The length in bytes of the following TLV chain
  unpackWord(pbuf, pwTlvLength);

  *pwLen -= wRecordNameLen + 10;

  return 1; // Success
}



static DWORD updateServerGroupData(WORD wGroupId, void *groupData, int groupSize)
{
  DWORD dwCookie;
  servlistcookie* ack;

  ack = (servlistcookie*)SAFE_MALLOC(sizeof(servlistcookie));
  if (!ack)
  {
    NetLog_Server("Updating of group on server list failed (malloc error)");
    return 0;
  }
  ack->dwAction = SSA_GROUP_UPDATE;
  ack->szGroupName = getServerGroupNameUtf(wGroupId);
  ack->wGroupId = wGroupId;
  dwCookie = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_UPDATEGROUP, 0, ack);

  return icq_sendGroupUtf(dwCookie, ICQ_LISTS_UPDATEGROUP, ack->wGroupId, ack->szGroupName, groupData, groupSize);
}



static void handleServerCListAck(servlistcookie* sc, WORD wError)
{
  switch (sc->dwAction)
  {
  case SSA_VISIBILITY: 
    {
      if (wError)
        NetLog_Server("Server visibility update failed, error %d", wError);
      break;
    }
  case SSA_CONTACT_UPDATE:
    {
      RemovePendingOperation(sc->hContact, 1);
      if (wError)
      {
        NetLog_Server("Updating of server contact failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, "Updating of server contact failed.");
      }
      break;
    }
  case SSA_PRIVACY_ADD: 
    {
      if (wError)
      {
        NetLog_Server("Adding of privacy item to server list failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, "Adding of privacy item to server list failed.");
      }
      break;
    }
  case SSA_PRIVACY_REMOVE: 
    {
      if (wError)
      {
        NetLog_Server("Removing of privacy item from server list failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, "Removing of privacy item from server list failed.");
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

          NetLog_Server("Contact could not be added without authorization, add with await auth flag.");

          ICQWriteContactSettingByte(sc->hContact, "Auth", 1); // we need auth
          dwCookie = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_ADDTOLIST, sc->hContact, sc);
          icq_sendServerContact(sc->hContact, dwCookie, ICQ_LISTS_ADDTOLIST, sc->wGroupId, sc->wContactId);

          sc = NULL; // we do not want it to be freed now
          break;
        }
        FreeServerID(sc->wContactId, SSIT_ITEM);
        sendAddEnd(); // end server modifications here
        RemovePendingOperation(sc->hContact, 0);

        NetLog_Server("Adding of contact to server list failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, "Adding of contact to server list failed.");
      }
      else
      {
        void* groupData;
        int groupSize;
        HANDLE hPend = sc->hContact;

        ICQWriteContactSettingWord(sc->hContact, "ServerId", sc->wContactId);
        ICQWriteContactSettingWord(sc->hContact, "SrvGroupId", sc->wGroupId);

        if (groupData = collectBuddyGroup(sc->wGroupId, &groupSize))
        { // the group is not empty, just update it
          updateServerGroupData(sc->wGroupId, groupData, groupSize);
          SAFE_FREE((void**)&groupData);
        }
        else
        { // this should never happen
          NetLog_Server("Group update failed.");
        }
        sendAddEnd(); // end server modifications here

        if (hPend) RemovePendingOperation(hPend, 1);
      }
      break;
    }
  case SSA_GROUP_ADD:
    {
      if (wError)
      {
        FreeServerID(sc->wGroupId, SSIT_GROUP);
        NetLog_Server("Adding of group to server list failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, "Adding of group to server list failed.");
      }
      else // group added, we need to update master group
      {
        void* groupData;
        int groupSize;
        servlistcookie* ack;
        DWORD dwCookie;

        setServerGroupNameUtf(sc->wGroupId, sc->szGroupName); // add group to namelist
        setServerGroupIDUtf(makeGroupPathUtf(sc->wGroupId), sc->wGroupId); // add group to known

        groupData = collectGroups(&groupSize);
        groupData = SAFE_REALLOC(groupData, groupSize+2);
        *(((WORD*)groupData)+(groupSize>>1)) = sc->wGroupId; // add this new group id
        groupSize += 2;

        ack = (servlistcookie*)SAFE_MALLOC(sizeof(servlistcookie));
        if (ack)
        {
          ack->dwAction = SSA_GROUP_UPDATE;
          dwCookie = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_UPDATEGROUP, 0, ack);

          icq_sendGroupUtf(dwCookie, ICQ_LISTS_UPDATEGROUP, 0, ack->szGroupName, groupData, groupSize);
        }
        sendAddEnd(); // end server modifications here

        SAFE_FREE((void**)&groupData);

        if (sc->ofCallback) // is add contact pending
        {
          sc->ofCallback(sc->wGroupId, (LPARAM)sc->lParam);
         // sc = NULL; // we do not want to be freed here
        }
      }
      SAFE_FREE((void**)&sc->szGroupName);
      break;
    }
  case SSA_CONTACT_REMOVE:
    {
      if (!wError)
      {
        void* groupData;
        int groupSize;

        ICQWriteContactSettingWord(sc->hContact, "ServerId", 0); // clear the values
        ICQWriteContactSettingWord(sc->hContact, "SrvGroupId", 0);

        FreeServerID(sc->wContactId, SSIT_ITEM); 
              
        if (groupData = collectBuddyGroup(sc->wGroupId, &groupSize))
        { // the group is still not empty, just update it
          updateServerGroupData(sc->wGroupId, groupData, groupSize);

          sendAddEnd(); // end server modifications here
        }
        else // the group is empty, delete it if it does not have sub-groups
        {
          DWORD dwCookie;

          if (!CheckServerID((WORD)(sc->wGroupId+1), 0) || countGroupLevel((WORD)(sc->wGroupId+1)) == 0)
          { // is next id an sub-group, if yes, we cannot delete this group
            sc->dwAction = SSA_GROUP_REMOVE;
            sc->wContactId = 0;
            sc->hContact = NULL;
            sc->szGroupName = getServerGroupNameUtf(sc->wGroupId);
            dwCookie = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_REMOVEFROMLIST, 0, sc);

            icq_sendGroupUtf(dwCookie, ICQ_LISTS_REMOVEFROMLIST, sc->wGroupId, sc->szGroupName, NULL, 0);
            // here the modifications go on
            sc = NULL; // we do not want it to be freed now
          }
        }
        SAFE_FREE((void**)&groupData); // free the memory
      }
      else
      {
        NetLog_Server("Removing of contact from server list failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, "Removing of contact from server list failed.");
        sendAddEnd(); // end server modifications here
      }
      break;
    }
  case SSA_GROUP_UPDATE:
    {
      if (wError)
      {
        NetLog_Server("Updating of group on server list failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, "Updating of group on server list failed.");
      }
      SAFE_FREE((void**)&sc->szGroupName);
      break;
    }
  case SSA_GROUP_REMOVE:
    {
      SAFE_FREE((void**)&sc->szGroupName);
      if (wError)
      {
        NetLog_Server("Removing of group from server list failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, "Removing of group from server list failed.");
      }
      else // group removed, we need to update master group
      {
        void* groupData;
        int groupSize;
        DWORD dwCookie;

        setServerGroupNameUtf(sc->wGroupId, NULL); // clear group from namelist
        FreeServerID(sc->wGroupId, SSIT_GROUP);
        removeGroupPathLinks(sc->wGroupId);

        groupData = collectGroups(&groupSize);
        sc->wGroupId = 0;
        sc->dwAction = SSA_GROUP_UPDATE;
        sc->szGroupName = NULL;
        dwCookie = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_UPDATEGROUP, 0, sc);

        icq_sendGroupUtf(dwCookie, ICQ_LISTS_UPDATEGROUP, 0, sc->szGroupName, groupData, groupSize);
        sendAddEnd(); // end server modifications here

        sc = NULL; // we do not want to be freed here

        SAFE_FREE((void**)&groupData);
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
        if (wError == 0x0E && sc->lParam == 1)
        { // second ack - adding failed with error 0x0E, try to add with AVAIT_AUTH flag
          DWORD dwCookie;

          if (!ICQGetContactSettingByte(sc->hContact, "Auth", 0))
          { // we tried without AWAIT_AUTH, try again with it
            NetLog_Server("Contact could not be added without authorization, add with await auth flag.");

            ICQWriteContactSettingByte(sc->hContact, "Auth", 1); // we need auth
          }
          else
          { // we tried with AWAIT_AUTH, try again without
            NetLog_Server("Contact count not be added awaiting authorization, try authorized.");

            ICQWriteContactSettingByte(sc->hContact, "Auth", 0);
          }
          dwCookie = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_ADDTOLIST, sc->hContact, sc);
          icq_sendServerContact(sc->hContact, dwCookie, ICQ_LISTS_ADDTOLIST, sc->wNewGroupId, sc->wNewContactId);

          sc->lParam = 2; // do not cycle
          sc = NULL; // we do not want to be freed here
          break;
        }
        RemovePendingOperation(sc->hContact, 0);
        NetLog_Server("Moving of user to another group on server list failed, error %d", wError);
        icq_LogMessage(LOG_ERROR, "Moving of user to another group on server list failed.");
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

        ICQWriteContactSettingWord(sc->hContact, "ServerId", sc->wNewContactId);
        ICQWriteContactSettingWord(sc->hContact, "SrvGroupId", sc->wNewGroupId);
        FreeServerID(sc->wContactId, SSIT_ITEM); // release old contact id

        if (groupData = collectBuddyGroup(sc->wGroupId, &groupSize)) // update the group we moved from
        { // the group is still not empty, just update it
          updateServerGroupData(sc->wGroupId, groupData, groupSize);
          SAFE_FREE((void**)&groupData); // free the memory
        }
        else if (!CheckServerID((WORD)(sc->wGroupId+1), 0) || countGroupLevel((WORD)(sc->wGroupId+1)) == 0)
        { // the group is empty and is not followed by sub-groups, delete it
          DWORD dwCookie;
          servlistcookie* ack;

          ack = (servlistcookie*)SAFE_MALLOC(sizeof(servlistcookie));
          if (!ack)
          {
            NetLog_Server("Updating of group on server list failed (malloc error)");
            break;
          }
          ack->dwAction = SSA_GROUP_REMOVE;
          ack->szGroupName = getServerGroupNameUtf(sc->wGroupId);
          ack->wGroupId = sc->wGroupId;
          dwCookie = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_REMOVEFROMLIST, 0, ack);

          icq_sendGroupUtf(dwCookie, ICQ_LISTS_REMOVEFROMLIST, ack->wGroupId, ack->szGroupName, NULL, 0);
          bEnd = 0; // here the modifications go on
        }

        groupData = collectBuddyGroup(sc->wNewGroupId, &groupSize); // update the group we moved to
        updateServerGroupData(sc->wNewGroupId, groupData, groupSize);
        SAFE_FREE((void**)&groupData);

        if (bEnd) sendAddEnd();
        if (sc->hContact) RemovePendingOperation(sc->hContact, 1);
      }
      else // contact was deleted from server-list
      {
        ICQDeleteContactSetting(sc->hContact, "ServerId");
        ICQDeleteContactSetting(sc->hContact, "SrvGroupId");
        sc->lParam = 1;
        sc = NULL; // wait for second ack
      }
      break;
    }
  case SSA_CONTACT_FIX_AUTH:
    {
      if (wError)
      { // FIXME: something failed, we should handle it properly
      }
      break;
    }
  case SSA_GROUP_RENAME:
    {
      if (wError)
      {
        NetLog_Server("Renaming of server group failed, error %d", wError);
        icq_LogMessage(LOG_WARNING, "Renaming of server group failed.");
      }
      else
      { 
        setServerGroupNameUtf(sc->wGroupId, sc->szGroupName);
        removeGroupPathLinks(sc->wGroupId);
        setServerGroupIDUtf(makeGroupPathUtf(sc->wGroupId), sc->wGroupId);
      }
      RemoveGroupRename(sc->wGroupId);
      SAFE_FREE((void**)&sc->szGroupName);
      break;
    }
  case SSA_SETAVATAR:
    {
      if (wError)
      {
        NetLog_Server("Uploading of avatar hash failed.");
        if (sc->wGroupId) // is avatar added or updated?
        {
          FreeServerID(sc->wContactId, SSIT_ITEM);
          ICQDeleteContactSetting(NULL, "SrvAvatarID"); // to fix old versions
        }
      }
      else
      {
        ICQWriteContactSettingWord(NULL, "SrvAvatarID", sc->wContactId);
      }
      break;
    }
  case SSA_REMOVEAVATAR:
    {
      if (wError)
        NetLog_Server("Removing of avatar hash failed.");
      else
      {
        ICQDeleteContactSetting(NULL, "SrvAvatarID");
        FreeServerID(sc->wContactId, SSIT_ITEM);
      }
      break;
    }
  case SSA_SERVLIST_ACK:
    {
      ICQBroadcastAck(sc->hContact, ICQACKTYPE_SERVERCLIST, wError?ACKRESULT_FAILED:ACKRESULT_SUCCESS, (HANDLE)sc->lParam, wError);
      break;
    }
  case SSA_IMPORT:
    {
      if (wError)
        NetLog_Server("Re-starting import sequence failed, error %d", wError);
      else
      {
        ICQWriteContactSettingWord(NULL, "SrvImportID", 0);
        ICQDeleteContactSetting(NULL, "ImportTS");
      }
      break;
    }
  default:
    NetLog_Server("Server ack cookie type (%d) not recognized.", sc->dwAction);
  }
  SAFE_FREE((void**)&sc); // free the memory

  return;
}



static HANDLE HContactFromRecordName(char* szRecordName, int *bAdded)
{
  HANDLE hContact = INVALID_HANDLE_VALUE;

  if (!IsStringUIN(szRecordName))
  { // probably AIM contact
    hContact = HContactFromUID(0, szRecordName, bAdded);
  }
  else
  { // this should be ICQ number
    DWORD dwUin;

    dwUin = atoi(szRecordName);
    hContact = HContactFromUIN(dwUin, bAdded);
  }
  return hContact;
}



static int getServerDataFromItemTLV(oscar_tlv_chain* pChain, unsigned char *buf)
{ // get server-list item's TLV data
  oscar_tlv_chain* list = pChain;
  int datalen = 0;
  icq_packet pBuf;

  // Initialize our handy data buffer
  pBuf.wPlace = 0;
  pBuf.pData = buf;

  while (list)
  { // collect non-standard TLVs and save them to DB
    if (list->tlv.wType != SSI_TLV_AWAITING_AUTH &&
        list->tlv.wType != SSI_TLV_NAME &&
        list->tlv.wType != SSI_TLV_COMMENT)
    { // only TLVs which we do not handle on our own
      packTLV(&pBuf, list->tlv.wType, list->tlv.wLen, list->tlv.pData);

      datalen += list->tlv.wLen + 4;
    }
    list = list->next;
  }
  return datalen;
}


static void handleServerCList(unsigned char *buf, WORD wLen, WORD wFlags, serverthread_info *info)
{
  BYTE bySSIVersion;
  WORD wRecordCount;
  WORD wRecord;
  WORD wGroupId;
  WORD wItemId;
  WORD wTlvType;
  WORD wTlvLength;
  BOOL bIsLastPacket;
  uid_str szRecordName;
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
  NetLog_Server("SSI: number of entries is %u, version is %u", wRecordCount, bySSIVersion);


  // Loop over all items in the packet
  for (wRecord = 0; wRecord < wRecordCount; wRecord++)
  {
    NetLog_Server("SSI: parsing record %u", wRecord + 1);

    if (wLen < 10)
    { // minimum: name length (zero), group ID, item ID, empty TLV
      NetLog_Server("Warning: SSI parsing error (%d)", 0);
      break;
    }

    if (!unpackServerListItem(&buf, &wLen, szRecordName, &wGroupId, &wItemId, &wTlvType, &wTlvLength))
    { // unpack basic structure
      NetLog_Server("Warning: SSI parsing error (%d)", 1);
      break;
    }

    NetLog_Server("Name: '%s', GroupID: %u, EntryID: %u, EntryType: %u, TLVlength: %u",
      szRecordName, wGroupId, wItemId, wTlvType, wTlvLength);

    if (wLen < wTlvLength)
    {
      NetLog_Server("Warning: SSI parsing error (%d)", 2);
      break;
    }

    // Initialize the tlv chain
    if (wTlvLength > 0)
    {
      pChain = readIntoTLVChain(&buf, wTlvLength, 0);
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
        HANDLE hContact;
        int bAdded;

        hContact = HContactFromRecordName(szRecordName, &bAdded);

        if (hContact != INVALID_HANDLE_VALUE)
        {
          int bRegroup = 0;
          int bNicked = 0;
          WORD wOldGroupId;

          if (bAdded)
          { // Not already on list: added
            char* szGroup;

            NetLog_Server("SSI added new %s contact '%s'", "ICQ", szRecordName);

            if (szGroup = makeGroupPathUtf(wGroupId))
            { // try to get Miranda Group path from groupid, if succeeded save to db
              UniWriteContactSettingUtf(hContact, "CList", "Group", szGroup);

              SAFE_FREE((void**)&szGroup);
            }
            AddJustAddedContact(hContact);
          }
          else
          { // we should add new contacts and this contact was just added, show it
            if (IsContactJustAdded(hContact))
            {
              SetContactHidden(hContact, 0);
              bAdded = 1; // we want details for new contacts
            }
            else
              NetLog_Server("SSI ignoring existing contact '%s'", szRecordName);
            // Contact on server is always on list
            DBWriteContactSettingByte(hContact, "CList", "NotOnList", 0);
          }

          wOldGroupId = ICQGetContactSettingWord(hContact, "SrvGroupId", 0);
          // Save group and item ID
          ICQWriteContactSettingWord(hContact, "ServerId", wItemId);
          ICQWriteContactSettingWord(hContact, "SrvGroupId", wGroupId);
          ReserveServerID(wItemId, SSIT_ITEM);

          if (!bAdded && (wOldGroupId != wGroupId) && ICQGetContactSettingByte(NULL, "LoadServerDetails", DEFAULT_SS_LOAD))
          { // contact has been moved on the server
            char* szOldGroup = getServerGroupNameUtf(wOldGroupId);
            char* szGroup = getServerGroupNameUtf(wGroupId);

            if (!szOldGroup)
            { // old group is not known, most probably not created subgroup
              char* szTmp = UniGetContactSettingUtf(hContact, "CList", "Group", "");

              if (strlennull(szTmp))
              { // got group from CList
                SAFE_FREE((void**)&szOldGroup);
                szOldGroup = szTmp;
              }
              else
                SAFE_FREE((void**)&szTmp);

              if (!szOldGroup) szOldGroup = null_strdup(DEFAULT_SS_GROUP);
            }

            if (!szGroup || strlennull(szGroup)>=strlennull(szOldGroup) || strnicmp(szGroup, szOldGroup, strlennull(szGroup)))
            { // contact moved to new group or sub-group or not to master group
              bRegroup = 1;
            }
            if (bRegroup && !stricmp(DEFAULT_SS_GROUP, szGroup) && !GroupNameExistsUtf(szGroup, -1))
            { // is it the default "General" group ? yes, does it exists in CL ?
              bRegroup = 0; // if no, do not move to it - cause it would hide the contact
            }
            SAFE_FREE((void**)&szGroup);
            SAFE_FREE((void**)&szOldGroup);
          }

          if (bRegroup || bAdded)
          { // if we should load server details or contact was just added, update its group
            char* szGroup;

            if (szGroup = makeGroupPathUtf(wGroupId))
            { // try to get Miranda Group path from groupid, if succeeded save to db
              UniWriteContactSettingUtf(hContact, "CList", "Group", szGroup);

              SAFE_FREE((void**)&szGroup);
            }
          }

          if (pChain)
          { // Look for nickname TLV and copy it to the db if necessary
            if (pTLV = getTLV(pChain, SSI_TLV_NAME, 1))
            {
              if (pTLV->pData && (pTLV->wLen > 0))
              {
                char* pszNick;
                WORD wNickLength;

                wNickLength = pTLV->wLen;

                pszNick = (char*)SAFE_MALLOC(wNickLength + 1);
                // Copy buffer to utf-8 buffer
                memcpy(pszNick, pTLV->pData, wNickLength);
                pszNick[wNickLength] = 0; // Terminate string

                NetLog_Server("Nickname is '%s'", pszNick);

                bNicked = 1;

                // Write nickname to database
                if (ICQGetContactSettingByte(NULL, "LoadServerDetails", DEFAULT_SS_LOAD) || bAdded)
                { // if just added contact, save details always - does no harm
                  char *szOldNick;

                  if (szOldNick = UniGetContactSettingUtf(hContact,"CList","MyHandle",""))
                  {
                    if ((strcmpnull(szOldNick, pszNick)) && (strlennull(pszNick) > 0))
                    { // check if the truncated nick changed, i.e. do not overwrite locally stored longer nick
                      if (strlennull(szOldNick) <= strlennull(pszNick) || strncmp(szOldNick, pszNick, null_strcut(szOldNick, MAX_SSI_TLV_NAME_SIZE)))
                      {
                        // Yes, we really do need to delete it first. Otherwise the CLUI nick
                        // cache isn't updated (I'll look into it)
                        DBDeleteContactSetting(hContact,"CList","MyHandle");
                        UniWriteContactSettingUtf(hContact, "CList", "MyHandle", pszNick);
                      }
                    }
                    SAFE_FREE((void**)&szOldNick);
                  }
                  else if (strlennull(pszNick) > 0)
                  {
                    DBDeleteContactSetting(hContact,"CList","MyHandle");
                    UniWriteContactSettingUtf(hContact, "CList", "MyHandle", pszNick);
                  }
                }
                SAFE_FREE((void**)&pszNick);
              }
              else
              {
                NetLog_Server("Invalid nickname");
              }
            }
            if (bAdded && !bNicked)
              icq_QueueUser(hContact); // queue user without nick for fast auto info update

            // Look for comment TLV and copy it to the db if necessary
            if (pTLV = getTLV(pChain, SSI_TLV_COMMENT, 1))
            {
              if (pTLV->pData && (pTLV->wLen > 0))
              {
                char* pszComment;
                WORD wCommentLength;


                wCommentLength = pTLV->wLen;

                pszComment = (char*)SAFE_MALLOC(wCommentLength + 1);
                // Copy buffer to utf-8 buffer
                memcpy(pszComment, pTLV->pData, wCommentLength);
                pszComment[wCommentLength] = 0; // Terminate string

                NetLog_Server("Comment is '%s'", pszComment);

                // Write comment to database
                if (ICQGetContactSettingByte(NULL, "LoadServerDetails", DEFAULT_SS_LOAD) || bAdded)
                { // if just added contact, save details always - does no harm
                  char *szOldComment;

                  if (szOldComment = UniGetContactSettingUtf(hContact,"UserInfo","MyNotes",""))
                  {
                    if ((strcmpnull(szOldComment, pszComment)) && (strlennull(pszComment) > 0))
                    { // check if the truncated comment changed, i.e. do not overwrite locally stored longer comment
                      if (strlennull(szOldComment) <= strlennull(pszComment) || strncmp(szOldComment, pszComment, null_strcut(szOldComment, MAX_SSI_TLV_COMMENT_SIZE)))
                      {
                        UniWriteContactSettingUtf(hContact, "UserInfo", "MyNotes", pszComment);
                      }
                    }
                    SAFE_FREE((void**)&szOldComment);
                  }
                  else if (strlennull(pszComment) > 0)
                  {
                    UniWriteContactSettingUtf(hContact, "UserInfo", "MyNotes", pszComment);
                  }
                }
                SAFE_FREE((void**)&pszComment);
              }
              else
              {
                NetLog_Server("Invalid comment");
              }
            }

            // Look for need-authorization TLV
            if (getTLV(pChain, SSI_TLV_AWAITING_AUTH, 1))
            {
              ICQWriteContactSettingByte(hContact, "Auth", 1);
              NetLog_Server("SSI contact need authorization");
            }
            else
            {
              ICQWriteContactSettingByte(hContact, "Auth", 0);
            }

            { // store server-list item's TLV data
              unsigned char* data = (unsigned char*)SAFE_MALLOC(wTlvLength);
              int datalen = getServerDataFromItemTLV(pChain, data);

              if (datalen > 0)
                ICQWriteContactSettingBlob(hContact, "ServerData", data, datalen);
              else
                ICQDeleteContactSetting(hContact, "ServerData");

              SAFE_FREE((void**)&data);
            }
          }
        }
        else
        { // failed to add or other error
          NetLog_Server("SSI failed to handle %s Item '%s'", "Buddy", szRecordName);
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

          ReserveServerID(wGroupId, SSIT_GROUP);

          setServerGroupNameUtf(wGroupId, szRecordName);

          NetLog_Server("Group %s added to known groups.", szRecordName);

          /* demangle full grouppath, create groups, set it to known */
          pszName = makeGroupPathUtf(wGroupId); 
          SAFE_FREE((void**)&pszName);

          /* TLV contains a TLV(C8) with a list of WORDs of contained contact IDs */
          /* our processing is good enough that we don't need this duplication */
        }
        else
        {
          NetLog_Server("Unhandled type 0x01, wItemID != 0");
        }
      }
      else
      {
        NetLog_Server("Unhandled type 0x01");
      }
      break;

    case SSI_ITEM_PERMIT:
      {
        /* item on visible list */
        /* wItemId not related to contact ID */
        /* pszRecordName is the UIN */
        HANDLE hContact;
        int bAdded;

        hContact = HContactFromRecordName(szRecordName, &bAdded);

        if (hContact != INVALID_HANDLE_VALUE)
        {
          if (bAdded)
          {
            NetLog_Server("SSI added new %s contact '%s'", "Permit", szRecordName);
            // It wasn't previously in the list, we hide it so it only appears in the visible list
            SetContactHidden(hContact, 1);
            // Add it to the list, so it can be added properly if proper contact
            AddJustAddedContact(hContact);
          }
          else
            NetLog_Server("SSI %s contact already exists '%s'", "Permit", szRecordName);

          // Save permit ID
          ICQWriteContactSettingWord(hContact, "SrvPermitId", wItemId);
          ReserveServerID(wItemId, SSIT_ITEM);
          // Set apparent mode
          ICQWriteContactSettingWord(hContact, "ApparentMode", ID_STATUS_ONLINE);
          NetLog_Server("Visible-contact (%s)", szRecordName);
        }
        else
        { // failed to add or other error
          NetLog_Server("SSI failed to handle %s Item '%s'", "Permit", szRecordName);
        }
      }
      break;

    case SSI_ITEM_DENY:
      {
        /* Item on invisible list */
        /* wItemId not related to contact ID */
        /* pszRecordName is the UIN */
        HANDLE hContact;
        int bAdded;

        hContact = HContactFromRecordName(szRecordName, &bAdded);

        if (hContact != INVALID_HANDLE_VALUE)
        {
          if (bAdded)
          {
            /* not already on list: added */
            NetLog_Server("SSI added new %s contact '%s'", "Deny", szRecordName);
            // It wasn't previously in the list, we hide it so it only appears in the visible list
            SetContactHidden(hContact, 1);
            // Add it to the list, so it can be added properly if proper contact
            AddJustAddedContact(hContact);
          }
          else
            NetLog_Server("SSI %s contact already exists '%s'", "Deny", szRecordName);

          // Save Deny ID
          ICQWriteContactSettingWord(hContact, "SrvDenyId", wItemId);
          ReserveServerID(wItemId, SSIT_ITEM);

          // Set apparent mode
          ICQWriteContactSettingWord(hContact, "ApparentMode", ID_STATUS_OFFLINE);
          NetLog_Server("Invisible-contact (%s)", szRecordName);
        }
        else
        { // failed to add or other error
          NetLog_Server("SSI failed to handle %s Item '%s'", "Deny", szRecordName);
        }
      }
      break;

    case SSI_ITEM_VISIBILITY: /* My visibility settings */
      {
        BYTE bVisibility;

        ReserveServerID(wItemId, SSIT_ITEM);

        // Look for visibility TLV
        if (bVisibility = getByteFromChain(pChain, SSI_TLV_VISIBILITY, 1))
        { // found it, store the id, we do not need current visibility - we do not rely on it
          ICQWriteContactSettingWord(NULL, "SrvVisibilityID", wItemId);
          NetLog_Server("Visibility is %u", bVisibility);
        }
      }
      break;

    case SSI_ITEM_IGNORE: 
      {
        /* item on ignore list */
        /* wItemId not related to contact ID */
        /* pszRecordName is the UIN */
        HANDLE hContact;
        int bAdded;

        hContact = HContactFromRecordName(szRecordName, &bAdded);

        if (hContact != INVALID_HANDLE_VALUE)
        {
          if (bAdded)
          {
            /* not already on list: add */
            NetLog_Server("SSI added new %s contact '%s'", "Ignore", szRecordName);
            // It wasn't previously in the list, we hide it
            SetContactHidden(hContact, 1);
            // Add it to the list, so it can be added properly if proper contact
            AddJustAddedContact(hContact);
          }
          else
            NetLog_Server("SSI %s contact already exists '%s'", "Ignore", szRecordName);

          // Save Ignore ID
          ICQWriteContactSettingWord(hContact, "SrvIgnoreId", wItemId);
          ReserveServerID(wItemId, SSIT_ITEM);

          // Set apparent mode & ignore
          ICQWriteContactSettingWord(hContact, "ApparentMode", ID_STATUS_OFFLINE);
          // set ignore all events
          CallService(MS_IGNORE_IGNORE, (WPARAM)hContact, IGNOREEVENT_ALL);
          NetLog_Server("Ignore-contact (%s)", szRecordName);
        }
        else
        { // failed to add or other error
          NetLog_Server("SSI failed to handle %s Item '%s'", "Ignore", szRecordName);
        }
      }
      break;

    case SSI_ITEM_UNKNOWN2:
      NetLog_Server("SSI unknown type 0x11");
      break;

    case SSI_ITEM_IMPORTTIME:
      if (wGroupId == 0)
      {
        /* time our list was first imported */
        /* pszRecordName is "Import Time" */
        /* data is TLV(13) {TLV(D4) {time_t importTime}} */
        ICQWriteContactSettingDword(NULL, "ImportTS", getDWordFromChain(pChain, SSI_TLV_TIMESTAMP, 1));
        ICQWriteContactSettingWord(NULL, "SrvImportID", wItemId);
        NetLog_Server("SSI %s item recognized", "first import");
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
        if (!strcmpnull(szRecordName, "12"))
        { // need to handle Photo Item separately
          ICQWriteContactSettingWord(NULL, "SrvPhotoID", wItemId);
          NetLog_Server("SSI %s item recognized", "Photo");
        }
        else
        {
          ICQWriteContactSettingWord(NULL, "SrvAvatarID", wItemId);
          NetLog_Server("SSI %s item recognized", "Avatar");
        }
        ReserveServerID(wItemId, SSIT_ITEM);
      }
      break;

    case SSI_ITEM_UNKNOWN1:
      if (wGroupId == 0)
      {
        /* ICQ2k ShortcutBar Items */
        /* data is TLV(CD) text */
      }

    default:
      NetLog_Server("SSI unhandled item %2x", wTlvType);
      break;
    }

    if (pChain)
      disposeChain(&pChain);

  } // end for

  NetLog_Server("Bytes left: %u", wLen);

  ICQWriteContactSettingWord(NULL, "SrvRecordCount", (WORD)(wRecord + ICQGetContactSettingWord(NULL, "SrvRecordCount", 0)));

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
      ICQWriteContactSettingDword(NULL, "SrvLastUpdate", dwLastUpdateTime);
      NetLog_Server("Last update of server list was (%u) %s", dwLastUpdateTime, asctime(localtime((time_t*)&dwLastUpdateTime)));

      sendRosterAck();
      handleServUINSettings(wListenPort, info);
    }
    else
    {
      NetLog_Server("Last packet missed update time...");
    }
    if (ICQGetContactSettingWord(NULL, "SrvRecordCount", 0) == 0)
    { // we got empty serv-list, create master group
      servlistcookie* ack = (servlistcookie*)SAFE_MALLOC(sizeof(servlistcookie));
      if (ack)
      { 
        DWORD seq;

        ack->dwAction = SSA_GROUP_UPDATE;
        ack->szGroupName = "";
        seq = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_ADDTOLIST, 0, ack);
        icq_sendGroupUtf(seq, ICQ_LISTS_ADDTOLIST, 0, ack->szGroupName, NULL, 0);
      }
    }
    // serv-list sync finished, clear just added contacts
    FlushJustAddedContacts(); 
  }
  else
  {
    NetLog_Server("Waiting for more packets");
  }
}



static void handleRecvAuthRequest(unsigned char *buf, WORD wLen)
{
  WORD wReasonLen;
  DWORD dwUin;
  uid_str szUid;
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
  int bAdded;

  if (!unpackUID(&buf, &wLen, &dwUin, &szUid)) return;

  if (dwUin && IsOnSpammerList(dwUin))
  {
    NetLog_Server("Ignored Message from known Spammer");
    return;
  }

  unpackWord(&buf, &wReasonLen);
  wLen -= 2;
  if (wReasonLen > wLen)
    return;

  hcontact = HContactFromUID(dwUin, szUid, &bAdded);

  ccs.szProtoService=PSR_AUTH;
  ccs.hContact=hcontact;
  ccs.wParam=0;
  ccs.lParam=(LPARAM)&pre;
  pre.flags=0;
  pre.timestamp=time(NULL);
  pre.lParam=sizeof(DWORD)+sizeof(HANDLE)+wReasonLen+5;
  szReason = (char*)SAFE_MALLOC(wReasonLen+1);
  if (szReason)
  {
    memcpy(szReason, buf, wReasonLen);
    szReason[wReasonLen] = '\0';
    szReason = detect_decode_utf8(szReason); // detect & decode UTF-8
  }
  nReasonLen = strlennull(szReason);
  // Read nick name from DB
  if (dwUin)
  {
    if (ICQGetContactSettingString(hcontact, "Nick", &dbv))
      nNickLen = 0;
    else
    {
      szNick = dbv.pszVal;
      nNickLen = strlennull(szNick);
    }
  }
  else
    nNickLen = strlennull(szUid);
  pre.lParam += nNickLen + nReasonLen;

  ICQWriteContactSettingByte(ccs.hContact, "Grant", 1);

  /*blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)*/
  pCurBlob=szBlob=(char *)_alloca(pre.lParam);
  memcpy(pCurBlob,&dwUin,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
  memcpy(pCurBlob,&hcontact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
  if (nNickLen && dwUin) 
  { // if we have nick we add it, otherwise keep trailing zero
    memcpy(pCurBlob, szNick, nNickLen);
    pCurBlob+=nNickLen;
  }
  else
  {
    memcpy(pCurBlob, szUid, nNickLen);
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

  SAFE_FREE((void**)&szReason);
  ICQFreeVariant(&dbv);
  return;
}



static void handleRecvAdded(unsigned char *buf, WORD wLen)
{
  DWORD dwUin;
  uid_str szUid;
  DWORD cbBlob;
  PBYTE pBlob,pCurBlob;
  HANDLE hContact;
  int bAdded;
  char* szNick;
  int nNickLen;
  DBVARIANT dbv = {0};

  if (!unpackUID(&buf, &wLen, &dwUin, &szUid)) return;

  if (dwUin && IsOnSpammerList(dwUin))
  {
    NetLog_Server("Ignored Message from known Spammer");
    return;
  }

  hContact=HContactFromUID(dwUin, szUid, &bAdded);

  ICQDeleteContactSetting(hContact, "Grant");

  cbBlob=sizeof(DWORD)+sizeof(HANDLE)+4;

  if (dwUin)
  {
    if (ICQGetContactSettingString(hContact, "Nick", &dbv))
      nNickLen = 0;
    else
    {
      szNick = dbv.pszVal;
      nNickLen = strlennull(szNick);
    }
  }
  else
    nNickLen = strlennull(szUid);
  
  cbBlob += nNickLen;
  
  pCurBlob=pBlob=(PBYTE)_alloca(cbBlob);
  /*blob is: uin(DWORD), hContact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ) */
  memcpy(pCurBlob,&dwUin,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
  memcpy(pCurBlob,&hContact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
  if (nNickLen && dwUin) 
  { // if we have nick we add it, otherwise keep trailing zero
    memcpy(pCurBlob, szNick, nNickLen);
    pCurBlob+=nNickLen;
  }
  else
  {
    memcpy(pCurBlob, szUid, nNickLen);
    pCurBlob+=nNickLen;
  }
  *(char *)pCurBlob = 0; pCurBlob++;
  *(char *)pCurBlob = 0; pCurBlob++;
  *(char *)pCurBlob = 0; pCurBlob++;
  *(char *)pCurBlob = 0;
// TODO: Change for new auth system

  ICQAddEvent(NULL, EVENTTYPE_ADDED, time(NULL), 0, cbBlob, pBlob);
}



static void handleRecvAuthResponse(unsigned char *buf, WORD wLen)
{
  BYTE bResponse;
  DWORD dwUin;
  uid_str szUid;
  HANDLE hContact;
  char* szNick;
  WORD nReasonLen;
  char* szReason;
  int bAdded;

  bResponse = 0xFF;

  if (!unpackUID(&buf, &wLen, &dwUin, &szUid)) return;

  if (dwUin && IsOnSpammerList(dwUin))
  {
    NetLog_Server("Ignored Message from known Spammer");
    return;
  }

  hContact = HContactFromUID(dwUin, szUid, &bAdded);

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
      szReason = (char*)_alloca(nReasonLen+1);
      unpackString(&buf, szReason, nReasonLen);
      szReason[nReasonLen] = '\0';
    }
  }

  switch (bResponse)
  {

  case 0:
    NetLog_Server("Authorization request %s by %s", "denied", strUID(dwUin, szUid));
    // TODO: Add to system history as soon as new auth system is ready
    break;

  case 1:
    ICQWriteContactSettingByte(hContact, "Auth", 0);
    NetLog_Server("Authorization request %s by %s", "granted", strUID(dwUin, szUid));
    // TODO: Add to system history as soon as new auth system is ready
    break;

  default:
    NetLog_Server("Unknown Authorization request response (%u) from %s", bResponse, strUID(dwUin, szUid));
    break;

  }
  SAFE_FREE((void**)&szNick);
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
    BYTE bVisibility = ICQGetContactSettingByte(NULL, "SrvVisibility", 0);

    if (bVisibility == bCode) // if no change was made, not necescary to update that
      return;
    ICQWriteContactSettingByte(NULL, "SrvVisibility", bCode);

    // Do we have a known server visibility ID? We should, unless we just subscribed to the serv-list for the first time
    if ((wVisibilityID = ICQGetContactSettingWord(NULL, "SrvVisibilityID", 0)) == 0)
    {
      // No, create a new random ID
      wVisibilityID = GenerateServerId(SSIT_ITEM);
      ICQWriteContactSettingWord(NULL, "SrvVisibilityID", wVisibilityID);
      wCommand = ICQ_LISTS_ADDTOLIST;
      NetLog_Server("Made new srvVisibilityID, id is %u, code is %u", wVisibilityID, bCode);
    }
    else
    {
      NetLog_Server("Reused srvVisibilityID, id is %u, code is %u", wVisibilityID, bCode);
      wCommand = ICQ_LISTS_UPDATEGROUP;
    }

    ack = (servlistcookie*)SAFE_MALLOC(sizeof(servlistcookie));
    if (!ack) 
    {
      NetLog_Server("Cookie alloc failure.");
      return; // out of memory, go away
    }
    ack->dwAction = SSA_VISIBILITY; // update visibility
    dwCookie = AllocateCookie(CKT_SERVERLIST, wCommand, 0, ack); // take cookie

    // Build and send packet
    serverPacketInit(&packet, 25);
    packFNACHeaderFull(&packet, ICQ_LISTS_FAMILY, wCommand, 0, dwCookie);
    packWord(&packet, 0);                   // Name (null)
    packWord(&packet, 0);                   // GroupID (0 if not relevant)
    packWord(&packet, wVisibilityID);       // EntryID
    packWord(&packet, SSI_ITEM_VISIBILITY); // EntryType
    packWord(&packet, 5);                   // Length in bytes of following TLV
    packTLV(&packet, SSI_TLV_VISIBILITY, 1, &bCode);  // TLV (Visibility)
    sendServPacket(&packet);
    // There is no need to send ICQ_LISTS_CLI_MODIFYSTART or
    // ICQ_LISTS_CLI_MODIFYEND when modifying the visibility code
  }
}



// Updates the avatar hash used while in SSI mode. If a server ID is
// not stored in the local DB, a new ID will be added to the server list.
void updateServAvatarHash(BYTE *pHash, int size)
{
  icq_packet packet;
  WORD wAvatarID;
  WORD wCommand;
  DBVARIANT dbvHash;
  int bResetHash = 0;
  BYTE bName = 0;

  if (!ICQGetContactSetting(NULL, "AvatarHash", &dbvHash))
  {
    bName = 0x30 + dbvHash.pbVal[1];

    if (memcmp(pHash, dbvHash.pbVal, 2) != 0)
    {
      /** add code to remove old hash from server */
      bResetHash = 1;
    }
    ICQFreeVariant(&dbvHash);
  }

  if (bResetHash) // start update session
    sendAddStart(FALSE);

  if (bResetHash || !pHash)
  {
    servlistcookie* ack;
    DWORD dwCookie;

    // Do we have a known server avatar ID?
    if (wAvatarID = ICQGetContactSettingWord(NULL, "SrvAvatarID", 0))
    {
      ack = (servlistcookie*)SAFE_MALLOC(sizeof(servlistcookie));
      if (!ack) 
      {
        NetLog_Server("Cookie alloc failure.");
        return; // out of memory, go away
      }
      ack->dwAction = SSA_REMOVEAVATAR; // update avatar hash
      ack->wContactId = wAvatarID;
      dwCookie = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_REMOVEFROMLIST, 0, ack); // take cookie

      // Build and send packet
      serverPacketInit(&packet, (WORD)(20 + (bName ? 1 : 0)));
      packFNACHeaderFull(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_REMOVEFROMLIST, 0, dwCookie);
      if (bName)
      { // name
        packWord(&packet, 1);
        packByte(&packet, bName);             // Name
      }
      else
        packWord(&packet, 0);                 // Name (null)
      packWord(&packet, 0);                   // GroupID (0 if not relevant)
      packWord(&packet, wAvatarID);           // EntryID
      packWord(&packet, SSI_ITEM_BUDDYICON);  // EntryType
      packWord(&packet, 0);                   // Length in bytes of following TLV
      sendServPacket(&packet);
    }
  }

  if (pHash)
  {
    servlistcookie* ack;
    DWORD dwCookie;
    WORD hashsize = size - 2;

    // Do we have a known server avatar ID? We should, unless we just subscribed to the serv-list for the first time
    if (bResetHash || (wAvatarID = ICQGetContactSettingWord(NULL, "SrvAvatarID", 0)) == 0)
    {
      // No, create a new random ID
      wAvatarID = GenerateServerId(SSIT_ITEM);
      wCommand = ICQ_LISTS_ADDTOLIST;
      NetLog_Server("Made new srvAvatarID, id is %u", wAvatarID);
    }
    else
    {
      NetLog_Server("Reused srvAvatarID, id is %u", wAvatarID);
      wCommand = ICQ_LISTS_UPDATEGROUP;
    }

    ack = (servlistcookie*)SAFE_MALLOC(sizeof(servlistcookie));
    if (!ack) 
    {
      NetLog_Server("Cookie alloc failure.");
      return; // out of memory, go away
    }
    ack->dwAction = SSA_SETAVATAR; // update avatar hash
    ack->wContactId = wAvatarID;
    dwCookie = AllocateCookie(CKT_SERVERLIST, wCommand, 0, ack); // take cookie

    // Build and send packet
    serverPacketInit(&packet, (WORD)(29 + hashsize));
    packFNACHeaderFull(&packet, ICQ_LISTS_FAMILY, wCommand, 0, dwCookie);
    packWord(&packet, 1);                       // Name length
    packByte(&packet, (BYTE)(0x30 + pHash[1])); // Name
    packWord(&packet, 0);                       // GroupID (0 if not relevant)
    packWord(&packet, wAvatarID);               // EntryID
    packWord(&packet, SSI_ITEM_BUDDYICON);      // EntryType
    packWord(&packet, (WORD)(0x8 + hashsize));  // Length in bytes of following TLV
    packTLV(&packet, SSI_TLV_NAME, 0, NULL);                    // TLV (Name)
    packTLV(&packet, SSI_TLV_AVATARHASH, hashsize, (LPBYTE)pHash + 2);  // TLV (Hash)
    sendServPacket(&packet);
    // There is no need to send ICQ_LISTS_CLI_MODIFYSTART or
    // ICQ_LISTS_CLI_MODIFYEND when modifying the avatar hash
  }

  if (bResetHash) // finish update session
    sendAddEnd();
}



// Should be called before the server list is modified. When all
// modifications are done, call sendAddEnd().
void sendAddStart(int bImport)
{
  icq_packet packet;
  WORD wImportID = ICQGetContactSettingWord(NULL, "SrvImportID", 0);

  if (bImport && wImportID)
  { // we should be importing, check if already have import item
    if (ICQGetContactSettingDword(NULL, "ImportTS", 0) + 604800 < ICQGetContactSettingDword(NULL, "LogonTS", 0))
    { // is the timestamp week older, clear it and begin new import
      DWORD dwCookie;
      servlistcookie* ack;

      if (ack = (servlistcookie*)SAFE_MALLOC(sizeof(servlistcookie)))
      { // we have cookie good, go on
        ack->dwAction = SSA_IMPORT;
        dwCookie = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_REMOVEFROMLIST, 0, ack);

        icq_sendSimpleItem(dwCookie, ICQ_LISTS_REMOVEFROMLIST, 0, "ImportTime", 0, wImportID, SSI_ITEM_IMPORTTIME);
      }
    }
  }

  serverPacketInit(&packet, (WORD)(bImport?14:10));
  packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_CLI_MODIFYSTART);
  if (bImport) packDWord(&packet, 1<<0x10); 
  sendServPacket(&packet);
}



// Should be called after the server list has been modified to inform
// the server that we are done.
void sendAddEnd(void)
{
  icq_packet packet;

  serverPacketInit(&packet, 10);
  packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_CLI_MODIFYEND);
  sendServPacket(&packet);
}



// Sent when the last roster packet has been received
void sendRosterAck(void)
{
  icq_packet packet;

  serverPacketInit(&packet, 10);
  packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_GOTLIST);
  sendServPacket(&packet);

#ifdef _DEBUG
  NetLog_Server("Sent SNAC(x13,x07) - CLI_ROSTERACK");
#endif
}
