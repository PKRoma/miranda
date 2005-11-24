// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin  berg, Sam Kothari, Robert Rainwater
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


extern int gbIdleAllow;
extern int icqGoingOnlineStatus;
extern BYTE gbOverRate;
extern int pendingAvatarsStart;
extern DWORD dwLocalInternalIP;
extern DWORD dwLocalExternalIP;
extern WORD wListenPort;
extern DWORD dwLocalDirectConnCookie;
extern char* cookieData;
extern int cookieDataLen;
extern char* migratedServer;
extern CRITICAL_SECTION modeMsgsMutex;

extern const capstr capXStatus[];

int isMigrating;

void setUserInfo();

char* calcMD5Hash(char* szFile);


void handleServiceFam(unsigned char* pBuffer, WORD wBufferLength, snac_header* pSnacHeader)
{
  icq_packet packet;

  switch (pSnacHeader->wSubtype)
  {

  case ICQ_SERVER_READY:
#ifdef _DEBUG
    NetLog_Server("Server is ready and is requesting my Family versions");
    NetLog_Server("Sending my Families");
#endif

    // This packet is a response to SRV_FAMILIES SNAC(1,3).
    // This tells the server which SNAC families and their corresponding
    // versions which the client understands. This also seems to identify
    // the client as an ICQ vice AIM client to the server.
    // Miranda mimics the behaviour of icq5 (haven't changed since at least 2002a)
    packet.wLen = 50;
    write_flap(&packet, 2);
    packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_FAMILIES, 0, ICQ_CLIENT_FAMILIES<<0x10);
    packDWord(&packet, 0x00010004);
    packDWord(&packet, 0x00130004);
    packDWord(&packet, 0x00020001);
    packDWord(&packet, 0x00030001);
    packDWord(&packet, 0x00150001);
    packDWord(&packet, 0x00040001);
    packDWord(&packet, 0x00060001);
    packDWord(&packet, 0x00090001);
    packDWord(&packet, 0x000a0001);
    packDWord(&packet, 0x000b0001);
    sendServPacket(&packet);
    break;

  case ICQ_SERVER_FAMILIES2:
    /* This is a reply to CLI_FAMILIES and it tells the client which families and their versions that this server understands.
     * We send a rate request packet */
#ifdef _DEBUG
    NetLog_Server("Server told me his Family versions");
    NetLog_Server("Requesting Rate Information");
#endif
    packet.wLen = 10;
    write_flap(&packet, 2);
    packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_REQ_RATE_INFO, 0, ICQ_CLIENT_REQ_RATE_INFO<<0x10);
    sendServPacket(&packet);
    break;

  case ICQ_SERVER_RATE_INFO:
#ifdef _DEBUG
    NetLog_Server("Server sent Rate Info");
    NetLog_Server("Sending Rate Info Ack");
#endif
    /* Don't really care about this now, just send the ack */
    packet.wLen = 20;
    write_flap(&packet, 2);
    packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_RATE_ACK, 0, ICQ_CLIENT_RATE_ACK<<0x10);
    packDWord(&packet, 0x00010002);
    packDWord(&packet, 0x00030004);
    packWord(&packet, 0x0005);
    sendServPacket(&packet);

    /* CLI_REQINFO - This command requests from the server certain information about the client that is stored on the server. */
#ifdef _DEBUG
    NetLog_Server("Sending CLI_REQINFO");
#endif
    packet.wLen = 10;
    write_flap(&packet, ICQ_DATA_CHAN);
    packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_REQINFO, 0, ICQ_CLIENT_REQINFO<<0x10);
    sendServPacket(&packet);

    if (gbSsiEnabled)
    {
      DWORD dwLastUpdate;
      WORD wRecordCount;
      servlistcookie* ack;
      DWORD dwCookie;

      dwLastUpdate = ICQGetContactSettingDword(NULL, "SrvLastUpdate", 0);
      wRecordCount = ICQGetContactSettingWord(NULL, "SrvRecordCount", 0);

      // CLI_REQLISTS - we want to use SSI
      packet.wLen = 10;
      write_flap(&packet, ICQ_DATA_CHAN);
      packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_CLI_REQLISTS, 0, ICQ_LISTS_CLI_REQLISTS<<0x10);
      sendServPacket(&packet);

      if (!wRecordCount) // CLI_REQROSTER
      { // we do not have any data - request full list
#ifdef _DEBUG
        NetLog_Server("Requesting full roster");
#endif
        packet.wLen = 10;
        write_flap(&packet, ICQ_DATA_CHAN);
        ack = (servlistcookie*)malloc(sizeof(servlistcookie));
        if (ack)
        { // we try to use standalone cookie if available
          ack->dwAction = SSA_CHECK_ROSTER; // loading list
          ack->dwUin = 0; // init content
          dwCookie = AllocateCookie(ICQ_LISTS_CLI_REQUEST, 0, ack);
        }
        else // if not use that old fake
          dwCookie = ICQ_LISTS_CLI_REQUEST<<0x10;

        packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_CLI_REQUEST, 0, dwCookie);
        sendServPacket(&packet);
      }
      else // CLI_CHECKROSTER
      {
#ifdef _DEBUG
        NetLog_Server("Requesting roster check");
#endif
        packet.wLen = 16;
        write_flap(&packet, ICQ_DATA_CHAN);
        ack = (servlistcookie*)malloc(sizeof(servlistcookie));
        if (ack)  // TODO: rewrite - use get list service for empty list
        { // we try to use standalone cookie if available
          ack->dwAction = SSA_CHECK_ROSTER; // loading list
          ack->dwUin = 0; // init content
          dwCookie = AllocateCookie(ICQ_LISTS_CLI_CHECK, 0, ack);
        }
        else // if not use that old fake
          dwCookie = ICQ_LISTS_CLI_CHECK<<0x10;

        packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_CLI_CHECK, 0, dwCookie);
        // check if it was not changed elsewhere (force reload, set that setting to zero)
        if (IsServerGroupsDefined())
        {
          packDWord(&packet, dwLastUpdate);  // last saved time
          packWord(&packet, wRecordCount);   // number of records saved
        }
        else
        { // we need to get groups info into DB, force receive list
          packDWord(&packet, 0);  // last saved time
          packWord(&packet, 0);   // number of records saved
        }
        sendServPacket(&packet);
      }
    }

    // CLI_REQLOCATION
    packet.wLen = 10;
    write_flap(&packet, ICQ_DATA_CHAN);
    packFNACHeader(&packet, ICQ_LOCATION_FAMILY, 0x02, 0, 0x020000);
    sendServPacket(&packet);

    // CLI_REQBUDDY
    packet.wLen = 10;
    write_flap(&packet, ICQ_DATA_CHAN);
    packFNACHeader(&packet, ICQ_BUDDY_FAMILY, ICQ_USER_CLI_REQBUDDY, 0, ICQ_USER_CLI_REQBUDDY<<0x10);
    sendServPacket(&packet);

    // CLI_REQICBM
    packet.wLen = 10;
    write_flap(&packet, ICQ_DATA_CHAN);
    packFNACHeader(&packet, ICQ_MSG_FAMILY, ICQ_MSG_CLI_REQICBM, 0, ICQ_MSG_CLI_REQICBM<<0x10);
    sendServPacket(&packet);

    // CLI_REQBOS
    packet.wLen = 10;
    write_flap(&packet, ICQ_DATA_CHAN);
    packFNACHeader(&packet, ICQ_BOS_FAMILY, ICQ_PRIVACY_REQ_RIGHTS, 0, ICQ_PRIVACY_REQ_RIGHTS<<0x10);
    sendServPacket(&packet);
    break;

  case ICQ_SERVER_PAUSE:
    NetLog_Server("Server is going down in a few seconds... (Flags: %u)", pSnacHeader->wFlags);
    // This is the list of groups that we want to have on the next server
    packet.wLen = 30;
    write_flap(&packet, ICQ_DATA_CHAN);
    packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_PAUSE_ACK, 0, ICQ_CLIENT_PAUSE_ACK<<0x10);
    packWord(&packet,ICQ_SERVICE_FAMILY);
    packWord(&packet,ICQ_LISTS_FAMILY);
    packWord(&packet,ICQ_LOCATION_FAMILY);
    packWord(&packet,ICQ_BUDDY_FAMILY);
    packWord(&packet,ICQ_EXTENSIONS_FAMILY);
    packWord(&packet,ICQ_MSG_FAMILY);
    packWord(&packet,0x06);
    packWord(&packet,ICQ_BOS_FAMILY);
    packWord(&packet,ICQ_LOOKUP_FAMILY);
    packWord(&packet,ICQ_STATS_FAMILY);
    sendServPacket(&packet);
#ifdef _DEBUG
    NetLog_Server("Sent server pause ack");
#endif
    break;

  case ICQ_SERVER_MIGRATIONREQ:
    {
      oscar_tlv_chain *chain = NULL;

#ifdef _DEBUG
      NetLog_Server("Server migration requested (Flags: %u)", pSnacHeader->wFlags);
#endif
      pBuffer += 2; // Unknown, seen: 0
      wBufferLength -= 2;
      chain = readIntoTLVChain(&pBuffer, wBufferLength, 0);

      if (cookieDataLen > 0 && cookieData != 0)
        SAFE_FREE(&cookieData);

      migratedServer = getStrFromChain(chain, 0x05, 1);
      cookieData = getStrFromChain(chain, 0x06, 1);
      cookieDataLen = getLenFromChain(chain, 0x06, 1);

      if (!migratedServer || !cookieData)
      {
        icq_LogMessage(LOG_FATAL, Translate("A server migration has failed because the server returned invalid data. You must reconnect manually."));
        SAFE_FREE(&migratedServer);
        SAFE_FREE(&cookieData);
        cookieDataLen = 0;
        return;
      }

      disposeChain(&chain);
      NetLog_Server("Migration has started. New server will be %s", migratedServer);

      icqGoingOnlineStatus = gnCurrentStatus;
      SetCurrentStatus(ID_STATUS_CONNECTING); // revert to connecting state

      isMigrating = 1;
    }
    break;

  case ICQ_SERVER_NAME_INFO: // This is the reply to CLI_REQINFO
    {
      BYTE bUinLen;
      oscar_tlv_chain *chain;

      unpackByte(&pBuffer, &bUinLen);
      pBuffer += bUinLen;
      pBuffer += 4;      /* warning level & user class */
      wBufferLength -= 5 + bUinLen;

      if (pSnacHeader->dwRef == ICQ_CLIENT_REQINFO<<0x10) // This is during the login sequence
      {
        DWORD dwValue;

        // TLV(x01) User type?
        // TLV(x0C) Empty CLI2CLI Direct connection info
        // TLV(x0A) External IP
        // TLV(x0F) Number of seconds that user has been online
        // TLV(x02) TIME MEMBERTIME The member since time (not sent)
        // TLV(x03) The online since time.
        // TLV(x05) Member of ICQ since. (not sent)
        // TLV(x0A) External IP again
        // TLV(x06) The current online status.
        // TLV(x1E) Unknown: empty.
        chain = readIntoTLVChain(&pBuffer, wBufferLength, 0);

        dwLocalExternalIP = getDWordFromChain(chain, 10, 1); 

        dwValue = getDWordFromChain(chain, 5, 1); 
        if (dwValue) ICQWriteContactSettingDword(NULL, "MemberTS", dwValue);

        ICQWriteContactSettingDword(NULL, "LogonTS", time(NULL));

        disposeChain(&chain);

        // If we are in SSI mode, this is sent after the list is acked instead
        // to make sure that we don't set status before seing the visibility code
        if (!gbSsiEnabled)
          handleServUINSettings(wListenPort, dwLocalInternalIP);

        // Change status
        SetCurrentStatus(icqGoingOnlineStatus);

        NetLog_Server(" *** Yeehah, login sequence complete");


        /* Get Offline Messages Reqeust */
        packet.wLen = 24;
        write_flap(&packet, 2);
        packFNACHeader(&packet, ICQ_EXTENSIONS_FAMILY, CLI_META_REQ, 0, 0x00010002);
        packDWord(&packet, 0x0001000a);    /* TLV */
        packLEWord(&packet, 8);            /* bytes remaining */
        packLEDWord(&packet, dwLocalUIN);
        packDWord(&packet, 0x3c000200);    /* get offline msgs */

        sendServPacket(&packet);

        // Update our information from the server
        sendOwnerInfoRequest();

        // Request info updates on all contacts
        icq_RescanInfoUpdate();

        // Start sending Keep-Alive packets
        if (ICQGetContactSettingByte(NULL, "KeepAlive", 0))
          forkthread(icq_keepAliveThread, 0, NULL);
      }
    }
    break;

  case ICQ_SERVER_RATE_CHANGE:

    if (wBufferLength >= 2)
    {
      WORD wStatus;
      WORD wClass;
      // This is a horrible simplification, but the only
      // area where we have rate control is in the user info
      // auto request part.
      unpackWord(&pBuffer, &wStatus);
      unpackWord(&pBuffer, &wClass);

      if (wStatus == 2 || wStatus == 3)
      { // this is only the simplest solution, needs rate management to every section
        ICQBroadcastAck(NULL, ICQACKTYPE_RATEWARNING, ACKRESULT_STATUS, (HANDLE)wClass, wStatus);
        gbOverRate = 1; // block user requests (user info, status messages, etc.)
        icq_PauseUserLookup(); // pause auto-info update thread
      }
      else if (wStatus == 4)
      {
        ICQBroadcastAck(NULL, ICQACKTYPE_RATEWARNING, ACKRESULT_STATUS, (HANDLE)wClass, wStatus);
        gbOverRate = 0; // enable user requests
        icq_EnableUserLookup(TRUE);
      }
    }

    break;

  case ICQ_SERVER_REDIRECT_SERVICE: // reply to family request, got new connection point
  {
     oscar_tlv_chain* pChain = NULL;
     WORD wFamily;
     familyrequest_rec* reqdata;

    if (!(pChain = readIntoTLVChain(&pBuffer, wBufferLength, 0)))
    {
      NetLog_Server("Received Broken Redirect Service SNAC(1,5).");
      break;
    }
    wFamily = getWordFromChain(pChain, 0x0D, 1);

    // pick request data
    if ((!FindCookie(pSnacHeader->dwRef, NULL, &reqdata)) || (reqdata->wFamily != wFamily))
    {
      disposeChain(&pChain);
      NetLog_Server("Received unexpected SNAC(1,5), skipping.");
      break;
    }

    FreeCookie(pSnacHeader->dwRef);

    { // new family entry point received
      char* pServer;
      char* pCookie;
      WORD wCookieLen;
      NETLIBOPENCONNECTION nloc = {0};
      HANDLE hConnection;

      pServer = getStrFromChain(pChain, 0x05, 1);
      pCookie = getStrFromChain(pChain, 0x06, 1);
      wCookieLen = getLenFromChain(pChain, 0x06, 1);

      if (!pServer || !pCookie)
      {
        NetLog_Server("Server returned invalid data, family unavailable.");

        SAFE_FREE(&pServer);
        SAFE_FREE(&pCookie);
        break;
      }

      nloc.cbSize = sizeof(nloc); // establish connection
      nloc.flags = 0;
      nloc.szHost = pServer; // this is horrible assumption - there should not be port
      nloc.wPort = ICQGetContactSettingWord(NULL, "OscarPort", DEFAULT_SERVER_PORT);

      hConnection = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)ghServerNetlibUser, (LPARAM)&nloc);
      if (!hConnection && (GetLastError() == 87))
      { // this ensures, an old Miranda will be able to connect also
        nloc.cbSize = NETLIBOPENCONNECTION_V1_SIZE;
        hConnection = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)ghServerNetlibUser, (LPARAM)&nloc);
      }
      
      if (hConnection == NULL)
      {
        NetLog_Server("Unable to connect to ICQ new family server.");
      } // we want the handler to be called even if the connecting failed
      reqdata->familyhandler(hConnection, pCookie, wCookieLen);

      // Free allocated memory
      // NOTE: "cookie" will get freed when we have connected to the avatar server.
      disposeChain(&pChain);
      SAFE_FREE(&pServer);
      SAFE_FREE(&reqdata);
    }

    break;
  }

  case ICQ_SERVER_EXTSTATUS: // our avatar
  {
    NetLog_Server("Received our avatar hash & status.");

    if ((wBufferLength >= 0x14) && gbAvatarsEnabled)
    {
      switch (pBuffer[2])
      {
        case 1: // our avatar is on the server, store hash
        {
          ICQWriteContactSettingBlob(NULL, "AvatarHash", pBuffer, 0x14);
          
          setUserInfo();

          // TODO: here we need to find a file, check its hash, if invalid get avatar from server
          // TODO: check if we had set any avatar if yes set our, if not download from server

          break;
        }
        case 0x41: // request to upload avatar data
        case 0x81:
        { // request to re-upload avatar data
          DBVARIANT dbv;
          char* hash;

          if (!gbSsiEnabled) break; // we could not change serv-list if it is disabled...

          if (ICQGetContactSetting(NULL, "AvatarFile", &dbv))
          { // we have no file to upload, remove hash from server
            NetLog_Server("We do not have avatar, removing hash.");
            updateServAvatarHash(NULL, 0);
            LinkContactPhotoToFile(NULL, NULL);
            break;
          }
          hash = calcMD5Hash(dbv.pszVal);
          if (!hash)
          { // the hash could not be calculated, remove from server
            NetLog_Server("We could not obtain hash, removing hash.");
            updateServAvatarHash(NULL, 0);
            LinkContactPhotoToFile(NULL, NULL);
          }
          else if (!memcmp(hash, pBuffer+4, 0x10))
          { // we have the right file
            HANDLE hFile = NULL, hMap = NULL;
            BYTE* ppMap = NULL;
            long cbFileSize = 0;

            NetLog_Server("Uploading our avatar data.");

            if ((hFile = CreateFile(dbv.pszVal, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL )) != INVALID_HANDLE_VALUE)
              if ((hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL)
                if ((ppMap = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL)
                  cbFileSize = GetFileSize( hFile, NULL );

            if (cbFileSize != 0)
            {
              SetAvatarData(NULL, ppMap, cbFileSize);
              LinkContactPhotoToFile(NULL, dbv.pszVal);
            }

            if (ppMap != NULL) UnmapViewOfFile(ppMap);
            if (hMap  != NULL) CloseHandle(hMap);
            if (hFile != NULL) CloseHandle(hFile);
            SAFE_FREE(&hash);
          }
          else
          {
            char* pHash = malloc(0x12);
            if (pHash)
            {
              NetLog_Server("Our file is different, set our new hash.");

              pHash[0] = 1; // state of the hash
              pHash[1] = 0x10; // len of the hash
              memcpy(pHash+2, hash, 0x10);
              updateServAvatarHash(pHash, 0x12);
              SAFE_FREE(&pHash);
            }
            else
            {
              NetLog_Server("We could not set hash, removing hash.");
              updateServAvatarHash(NULL, 0);
            }
            SAFE_FREE(&hash);
          }

          ICQFreeVariant(&dbv);
          break;
        default:
          NetLog_Server("Reiceived UNKNOWN Avatar Status.");
        }
      }
    }
    break;
  }

  case ICQ_ERROR:
  { // Something went wrong, probably the request for avatar family failed
    WORD wError;

    if (wBufferLength >= 2)
      unpackWord(&pBuffer, &wError);
    else 
      wError = 0;

    LogFamilyError(ICQ_SERVICE_FAMILY, wError);
    break;
  }

    // Stuff we don't care about
  case ICQ_SERVER_MOTD:
#ifdef _DEBUG
    NetLog_Server("Server message of the day");
#endif
    break;

  default:
    NetLog_Server("Warning: Ignoring SNAC(x%02x,x%02x) - Unknown SNAC (Flags: %u, Ref: %u)", ICQ_SERVICE_FAMILY, pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;

  }
}



char* calcMD5Hash(char* szFile)
{
  md5_state_t state;
  md5_byte_t digest[16];

  if (szFile)
  {
    HANDLE hFile = NULL, hMap = NULL;
    BYTE* ppMap = NULL;
    long cbFileSize = 0;
    char* res;

    if ((hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL )) != INVALID_HANDLE_VALUE)
      if ((hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != NULL)
        if ((ppMap = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL)
          cbFileSize = GetFileSize( hFile, NULL );

    res = malloc(16*sizeof(char));
    if (cbFileSize != 0 && res)
    {
      md5_init(&state);
      md5_append(&state, (const md5_byte_t *)ppMap, cbFileSize);
      md5_finish(&state, digest);
      memcpy(res, digest, 16);
    }

    if (ppMap != NULL) UnmapViewOfFile(ppMap);
    if (hMap  != NULL) CloseHandle(hMap);
    if (hFile != NULL) CloseHandle(hFile);

    if (res) return res;
  }
  
  return NULL;
}



static char* buildUinList(int subtype, WORD wMaxLen, HANDLE* hContactResume)
{
  char* szList;
  HANDLE hContact;
  WORD wCurrentLen = 0;
  DWORD dwUIN;
  uid_str szUID;
  char szUin[UINMAXLEN];
  char szLen[2];
  int add;


  szList = (char*)calloc(CallService(MS_DB_CONTACT_GETCOUNT, 0, 0), 11);
  szLen[1] = '\0';

  if (*hContactResume)
    hContact = *hContactResume;
  else
    hContact = ICQFindFirstContact();

  while (hContact != NULL)
  {
    if (!ICQGetContactSettingUID(hContact, &dwUIN, &szUID))
    {
      if (dwUIN)
      {
        _itoa(dwUIN, szUin, 10);
        szLen[0] = strlennull(szUin);
      }
      else
        szLen[0] = strlennull(szUID);

      switch (subtype)
      {

      case BUL_VISIBLE:
        add = ID_STATUS_ONLINE == ICQGetContactSettingWord(hContact, "ApparentMode", 0);
        break;

      case BUL_INVISIBLE:
        add = ID_STATUS_OFFLINE == ICQGetContactSettingWord(hContact, "ApparentMode", 0);
        break;

      default:
        add = 1;

        // If we are in SS mode, we only add those contacts that are
        // not in our SS list, or are awaiting authorization, to our
        // client side list

        // TODO: re-enabled, should work properly, needs more testing
        // This is temporarily disabled cause we cant rely on the Auth flag. 
        // The negative thing is that some status updates will arrive twice, sometimes contradicting themselves
        if (gbSsiEnabled && ICQGetContactSettingWord(hContact, "ServerId", 0) &&
          !ICQGetContactSettingByte(hContact, "Auth", 0))
          add = 0;

        // Never add hidden contacts to CS list
        if (DBGetContactSettingByte(hContact, "CList", "Hidden", 0))
          add = 0;

        break;
      }

      if (add)
      {
        wCurrentLen += szLen[0] + 1;
        if (wCurrentLen > wMaxLen)
        {
          *hContactResume = hContact;
          return szList;
        }

        strcat(szList, szLen);
        if (dwUIN)
          strcat(szList, szUin);
        else
          strcat(szList, szUID);
      }
    }

    hContact = ICQFindNextContact(hContact);
  }
  *hContactResume = NULL;


  return szList;
}



void sendEntireListServ(WORD wFamily, WORD wSubtype, WORD wFlags, int listType)
{
  HANDLE hResumeContact;
  char* szList;
  int nListLen;
  icq_packet packet;


  hResumeContact = NULL;

  do
  { // server doesn't seem to be able to cope with packets larger than 8k
    // send only about 100contacts per packet
    szList = buildUinList(listType, 0x3E8, &hResumeContact);
    nListLen = strlennull(szList);

    if (nListLen)
    {
      packet.wLen = nListLen + 10;
      write_flap(&packet, 2);
      packFNACHeader(&packet, wFamily, wSubtype, 0, wFlags<<0x10);
      packBuffer(&packet, szList, (WORD)nListLen);
      sendServPacket(&packet);
    }

    SAFE_FREE(&szList);
  }
  while (hResumeContact);
}



static void packNewCap(icq_packet* packet, WORD wNewCap)
{ // pack standard capability
  DWORD dwQ1 = 0x09460000 | wNewCap;

  packDWord(packet, dwQ1); 
  packDWord(packet, 0x4c7f11d1);
  packDWord(packet, 0x82224445);
  packDWord(packet, 0x53540000);
}



void setUserInfo()
{ // CLI_SETUSERINFO
  icq_packet packet;
  WORD wAdditionalData = 0;
  BYTE bXStatus = gbXStatusEnabled?ICQGetContactSettingByte(NULL, "XStatusId", 0):0;

  if (gbAimEnabled)
    wAdditionalData += 16;
#ifdef DBG_CAPMTN
  wAdditionalData += 16;
#endif
#ifdef DBG_CAPCH2
  wAdditionalData += 16;
#endif
#ifdef DBG_CAPRTF
  wAdditionalData += 16;
#endif
  if (gbUtfEnabled)
    wAdditionalData += 16;
#ifdef DBG_NEWCAPS
  wAdditionalData += 16;
#endif
#ifdef DBG_CAPXTRAZ
  wAdditionalData += 16;
#endif
  if (gbAvatarsEnabled)
    wAdditionalData += 16;
  if (bXStatus)
    wAdditionalData += 16;

  packet.wLen = 46 + wAdditionalData;
  write_flap(&packet, 2);
  packFNACHeader(&packet, ICQ_LOCATION_FAMILY, ICQ_LOCATION_SET_USER_INFO, 0, ICQ_LOCATION_SET_USER_INFO<<0x10);

  /* TLV(5): capability data */
  packWord(&packet, 0x0005);
  packWord(&packet, (WORD)(32 + wAdditionalData));


#ifdef DBG_CAPMTN
  {
    packDWord(&packet, 0x563FC809); // CAP_TYPING
    packDWord(&packet, 0x0B6F41BD);
    packDWord(&packet, 0x9F794226);
    packDWord(&packet, 0x09DFA2F3);
  }
#endif
#ifdef DBG_CAPCH2
  {
    packNewCap(&packet, 0x1349);    // AIM_CAPS_ICQSERVERRELAY
  }
#endif
#ifdef DBG_CAPRTF
  {
    packDWord(&packet, 0x97B12751); // AIM_CAPS_ICQRTF
    packDWord(&packet, 0x243C4334); // Broadcasts the capability to receive
    packDWord(&packet, 0xAD22D6AB); // RTF messages
    packDWord(&packet, 0xF73F1492);
  }
#endif
  if (gbUtfEnabled)
  {
    packNewCap(&packet, 0x134E);    // CAP_UTF8MSGS
  } // Broadcasts the capability to receive UTF8 encoded messages
#ifdef DBG_NEWCAPS
  {
    packNewCap(&packet, 0x0000);    // CAP_NEWCAPS
  } // Tells server we understand to new format of caps
#endif
#ifdef DBG_CAPXTRAZ
  {
    packDWord(&packet, 0x1a093c6c); // CAP_XTRAZ
    packDWord(&packet, 0xd7fd4ec5); // Broadcasts the capability to handle
    packDWord(&packet, 0x9d51a647); // Xtraz
    packDWord(&packet, 0x4e34f5a0);
  }
#endif
  if (gbAvatarsEnabled)
  {
    packNewCap(&packet, 0x134C);    // CAP_AVATAR
  } // Broadcasts the capability to provide Avatar
  if (gbAimEnabled)
  {
    packNewCap(&packet, 0x134D);    // Tells the server we can speak to AIM
  }
  if (bXStatus)
  {
    packBuffer(&packet, capXStatus[bXStatus-1], 0x10);
  }

  packNewCap(&packet, 0x1344);      // AIM_CAPS_ICQ

  packDWord(&packet, 0x4D697261);   // Miranda Signature
  packDWord(&packet, 0x6E64614D);
  packDWord(&packet, MIRANDA_VERSION);
  packDWord(&packet, ICQ_PLUG_VERSION);

  sendServPacket(&packet);
}


void handleServUINSettings(int nPort, int nIP)
{
  icq_packet packet;

  setUserInfo();

  // Set message parameters for channel 1 (CLI_SET_ICBM_PARAMS)
  packet.wLen = 26;
  write_flap(&packet, ICQ_DATA_CHAN);
  packFNACHeader(&packet, ICQ_MSG_FAMILY, ICQ_MSG_CLI_SETPARAMS, 0, ICQ_MSG_CLI_SETPARAMS<<0x10);
  packWord(&packet, 0x0001);              // Channel
#ifdef DBG_CAPMTN
  packDWord(&packet, 0x0000000B);         // Flags
#else
  packDWord(&packet, 0x00000003);         // Flags
#endif
  packWord(&packet, MAX_MESSAGESNACSIZE); // Max message snac size
  packWord(&packet, 0x03E7);              // Max sender warning level
  packWord(&packet, 0x03E7);              // Max receiver warning level
  packWord(&packet, CLIENTRATELIMIT);     // Minimum message interval in seconds
  packWord(&packet, 0x0000);              // Unknown
  sendServPacket(&packet);

  // Set message parameters for channel 2 (CLI_SET_ICBM_PARAMS)
  packet.wLen = 26;
  write_flap(&packet, ICQ_DATA_CHAN);
  packFNACHeader(&packet, ICQ_MSG_FAMILY, ICQ_MSG_CLI_SETPARAMS, 0, ICQ_MSG_CLI_SETPARAMS<<0x10);
  packWord(&packet, 0x0002);              // Channel
  packDWord(&packet, 0x00000003);         // Flags
  packWord(&packet, MAX_MESSAGESNACSIZE); // Max message snac size
  packWord(&packet, 0x03E7);              // Max sender warning level
  packWord(&packet, 0x03E7);              // Max receiver warning level
  packWord(&packet, CLIENTRATELIMIT);     // Minimum message interval in seconds
  packWord(&packet, 0x0000);              // Unknown
  sendServPacket(&packet);

  // Set message parameters for channel 4 (CLI_SET_ICBM_PARAMS)
  packet.wLen = 26;
  write_flap(&packet, ICQ_DATA_CHAN);
  packFNACHeader(&packet, ICQ_MSG_FAMILY, ICQ_MSG_CLI_SETPARAMS, 0, ICQ_MSG_CLI_SETPARAMS<<0x10);
  packWord(&packet, 0x0004);              // Channel
  packDWord(&packet, 0x00000003);         // Flags
  packWord(&packet, MAX_MESSAGESNACSIZE); // Max message snac size
  packWord(&packet, 0x03E7);              // Max sender warning level
  packWord(&packet, 0x03E7);              // Max receiver warning level
  packWord(&packet, CLIENTRATELIMIT);     // Minimum message interval in seconds
  packWord(&packet, 0x0000);              // Unknown
  sendServPacket(&packet);


  /* SNAC 3,4: Tell server who's on our list */
  sendEntireListServ(ICQ_BUDDY_FAMILY, ICQ_USER_ADDTOLIST, ICQ_USER_ADDTOLIST, BUL_ALLCONTACTS);

  if (icqGoingOnlineStatus == ID_STATUS_INVISIBLE)
  {
    /* Tell server who's on our visible list */
    if (!gbSsiEnabled)
      sendEntireListServ(ICQ_BOS_FAMILY, ICQ_CLI_ADDVISIBLE, 7, BUL_VISIBLE);
    else
      updateServVisibilityCode(3);
  }

  if (icqGoingOnlineStatus != ID_STATUS_INVISIBLE)
  {
    /* Tell server who's on our invisible list */
    if (!gbSsiEnabled)
      sendEntireListServ(ICQ_BOS_FAMILY, ICQ_CLI_ADDINVISIBLE, 7, BUL_INVISIBLE);
    else
      updateServVisibilityCode(4);
  }

  // SNAC 1,1E: Set status
  {
    WORD wFlags = 0;
    WORD wStatus;

    // Webaware setting bit flag
    if (ICQGetContactSettingByte(NULL, "WebAware", 0))
      wFlags = STATUS_WEBAWARE;

    // DC setting bit flag
    switch (ICQGetContactSettingByte(NULL, "DCType", 0))
    {
    case 0:
      break;

    case 1:
      wFlags = wFlags | STATUS_DCCONT;
      break;

    case 2:
      wFlags = wFlags | STATUS_DCAUTH;
      break;

    default:
      wFlags = wFlags | STATUS_DCDISABLED;
      break;
    }

    // Get status
    wStatus = MirandaStatusToIcq(icqGoingOnlineStatus);

    packet.wLen = 71;
    write_flap(&packet, 2);
    packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_SET_STATUS, 0, ICQ_CLIENT_SET_STATUS<<0x10);
    packDWord(&packet, 0x00060004);             // TLV 6: Status mode and security flags
    packWord(&packet, wFlags);                  // Status flags
    packWord(&packet, wStatus);                 // Status
    packTLVWord(&packet, 0x0008, 0x0000);       // TLV 8: Error code
    packDWord(&packet, 0x000c0025);             // TLV C: Direct connection info
    packDWord(&packet, nIP);
    packDWord(&packet, nPort);
    packByte(&packet, DC_TYPE);                 // TCP/FLAG firewall settings
    packWord(&packet, ICQ_VERSION);
    packDWord(&packet, dwLocalDirectConnCookie);// DC Cookie
    packDWord(&packet, WEBFRONTPORT);           // Web front port
    packDWord(&packet, CLIENTFEATURES);         // Client features
    packDWord(&packet, 0xffffffff);             // Abused timestamp
    packDWord(&packet, ICQ_PLUG_VERSION);       // Abused timestamp
    packDWord(&packet, 0x00000000);             // Timestamp
    packWord(&packet, 0x0000);                  // Unknown
    packTLVWord(&packet, 0x001F, 0x0000);

    sendServPacket(&packet);
  }


  /* SNAC 1,11 */
  packet.wLen = 14;
  write_flap(&packet, 2);
  packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_SET_IDLE, 0, ICQ_CLIENT_SET_IDLE<<0x10);
  packDWord(&packet, 0x00000000);

  sendServPacket(&packet);
  gbIdleAllow = 0;


  packet.wLen = 90;
  write_flap(&packet, 2);
  packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_READY, 0, ICQ_CLIENT_READY<<0x10);
  packDWord(&packet, 0x00010004); // imitate icq5 behaviour
  packDWord(&packet, 0x011008E4);
  packDWord(&packet, 0x00130004);
  packDWord(&packet, 0x011008E4);
  packDWord(&packet, 0x00020001);
  packDWord(&packet, 0x011008E4);
  packDWord(&packet, 0x00030001);
  packDWord(&packet, 0x011008E4);
  packDWord(&packet, 0x00150001);
  packDWord(&packet, 0x011008E4);
  packDWord(&packet, 0x00040001);
  packDWord(&packet, 0x011008E4);
  packDWord(&packet, 0x00060001);
  packDWord(&packet, 0x011008E4);
  packDWord(&packet, 0x00090001);
  packDWord(&packet, 0x011008E4);
  packDWord(&packet, 0x000A0001);
  packDWord(&packet, 0x011008E4);
  packDWord(&packet, 0x000B0001);
  packDWord(&packet, 0x011008E4);

  sendServPacket(&packet);

  if (gbAvatarsEnabled)
  { // Send SNAC 1,4 - request avatar family 0x10 connection
    icq_requestnewfamily(ICQ_AVATAR_FAMILY, StartAvatarThread);

    pendingAvatarsStart = 1;
    NetLog_Server("Requesting Avatar family entry point.");
  }

  if (gbAimEnabled)
  {
    char** szMsg = MirandaStatusToAwayMsg(gnCurrentStatus);

    EnterCriticalSection(&modeMsgsMutex);
    if (szMsg)
      icq_sendSetAimAwayMsgServ(*szMsg);
    LeaveCriticalSection(&modeMsgsMutex);
  }
}
