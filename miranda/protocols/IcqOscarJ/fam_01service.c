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
extern int gnCurrentStatus;
extern int icqGoingOnlineStatus;
extern BYTE gbSsiEnabled;
extern BYTE gbAvatarsEnabled;
extern int pendingAvatarsStart;
extern DWORD dwLocalUIN;
extern DWORD dwLocalInternalIP;
extern DWORD dwLocalExternalIP;
extern WORD wListenPort;
extern DWORD dwLocalDirectConnCookie;
extern HANDLE ghServerNetlibUser;
extern char* cookieData;
extern int cookieDataLen;
extern char* migratedServer;
int isMigrating;
extern char gpszICQProtoName[MAX_PATH];

void handleServiceFam(unsigned char* pBuffer, WORD wBufferLength, snac_header* pSnacHeader)
{

  icq_packet packet;


  switch (pSnacHeader->wSubtype)
  {

	case ICQ_SERVER_READY:
#ifdef _DEBUG
		Netlib_Logf(ghServerNetlibUser, "Server is ready and is requesting my Family versions");
		Netlib_Logf(ghServerNetlibUser, "Sending my Families");
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
    Netlib_Logf(ghServerNetlibUser, "Server told me his Family versions");
    Netlib_Logf(ghServerNetlibUser, "Requesting Rate Information");
#endif
    packet.wLen = 10;
    write_flap(&packet, 2);
    packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_REQ_RATE_INFO, 0, ICQ_CLIENT_REQ_RATE_INFO<<0x10);
    sendServPacket(&packet);
    break;

  case ICQ_SERVER_RATE_INFO:
#ifdef _DEBUG
    Netlib_Logf(ghServerNetlibUser, "Server sent Rate Info");
    Netlib_Logf(ghServerNetlibUser, "Sending Rate Info Ack");
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
    Netlib_Logf(ghServerNetlibUser, "Sending CLI_REQINFO");
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

      dwLastUpdate = DBGetContactSettingDword(NULL, gpszICQProtoName, "SrvLastUpdate", 0);
      wRecordCount = (WORD)DBGetContactSettingWord(NULL, gpszICQProtoName, "SrvRecordCount", 0);

      // CLI_REQLISTS - we want to use SSI
      packet.wLen = 10;
      write_flap(&packet, ICQ_DATA_CHAN);
      packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_CLI_REQLISTS, 0, ICQ_LISTS_CLI_REQLISTS<<0x10);
      sendServPacket(&packet);
#ifdef _DEBUG
      Netlib_Logf(ghServerNetlibUser, "Requesting roster check");
#endif
      packet.wLen = 16;
      write_flap(&packet, ICQ_DATA_CHAN);
      ack = (servlistcookie*)malloc(sizeof(servlistcookie));
      if (ack)
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
    packFNACHeader(&packet, ICQ_BOS_FAMILY, 0x02, 0, 0x020000);
    sendServPacket(&packet);
    break;

  case ICQ_SERVER_PAUSE:
    Netlib_Logf(ghServerNetlibUser, "Server is going down in a few seconds... (Flags: %u, Ref: %u", pSnacHeader->wFlags, pSnacHeader->dwRef);
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
    packWord(&packet,0x0a);
    packWord(&packet,0x0b);
    sendServPacket(&packet);
#ifdef _DEBUG
    Netlib_Logf(ghServerNetlibUser, "Sent server pause ack");
#endif
    break;

	case ICQ_SERVER_MIGRATIONREQ:
		{
			oscar_tlv_chain *chain = NULL;

#ifdef _DEBUG
			Netlib_Logf(ghServerNetlibUser, "Server migration requested (Flags: %u, Ref: %u", pSnacHeader->wFlags, pSnacHeader->dwRef);
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
			Netlib_Logf(ghServerNetlibUser, "Migration has started. New server will be %s", migratedServer);
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

			if (pSnacHeader->dwRef == 0x0e0000) // This is during the login sequence
			{

				// TLV(x01) User type?
				// TLV(x0C) Empty CLI2CLI Direct connection info
				// TLV(x0A) External IP
				// TLV(x0F) Number of seconds that user has been online
				// TLV(x02) TIME MEMBERTIME The member since time (not sent)
				// TLV(x03) The online since time.
				// TLV(x05) Some unknown time. (not sent)
				// TLV(x0A) External IP again
				// TLV(x06) The current online status.
				// TLV(x1E) Unknown: empty.
				chain = readIntoTLVChain(&pBuffer, wBufferLength, 0);

				dwLocalExternalIP = getDWordFromChain(chain, 10, 1);
				disposeChain(&chain);

				// If we are in SSI mode, this is sent after the list is acked instead
				// to make sure that we don't set status before seing the visibility code
				if (!gbSsiEnabled)
					handleServUINSettings(wListenPort, dwLocalInternalIP);

				// Change status
				gnCurrentStatus = icqGoingOnlineStatus;
				ProtoBroadcastAck(gpszICQProtoName, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS,
					(HANDLE)ID_STATUS_CONNECTING, gnCurrentStatus);


				Netlib_Logf(ghServerNetlibUser, " *** Yeehah, login sequence complete");


				/* Get Offline Messages Reqeust */
				packet.wLen = 24;
				write_flap(&packet, 2);
				packFNACHeader(&packet, ICQ_EXTENSIONS_FAMILY, CLI_META_REQ, 0, 0x00010002);
				packDWord(&packet, 0x0001000a);	  /* TLV */
				packLEWord(&packet, 8);		      /* bytes remaining */
				packLEDWord(&packet, dwLocalUIN);
				packDWord(&packet, 0x3c000200);	  /* get offline msgs */

				sendServPacket(&packet);

				// Update our information from the server
				sendOwnerInfoRequest();

				// Request info updates on all contacts
				icq_RescanInfoUpdate();

				// Start sending Keep-Alive packets
				if (DBGetContactSettingByte(NULL, gpszICQProtoName, "KeepAlive", 0))
					forkthread(icq_keepAliveThread, 0, NULL);

			}

		}
		break;

	case ICQ_SERVER_RATE_CHANGE:

		if (wBufferLength >= 2)
		{
			WORD wStatus;
			// This is a horrible simplification, but the only
			// area where we have rate control is in the user info
			// auto request part.
			unpackWord(&pBuffer, &wStatus);
			if (wStatus == 2 || wStatus == 3)
			{
				icq_EnableUserLookup(FALSE);
			}
			else if (wStatus == 4)
			{
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
      Netlib_Logf(ghServerNetlibUser, "Received Broken Redirect Service SNAC(1,5).");
      break;
    }
    wFamily = getWordFromChain(pChain, 0x0D, 1);

    // pick request data
    if ((!FindCookie(pSnacHeader->dwRef, NULL, &reqdata)) || (reqdata->wFamily != wFamily))
    {
      Netlib_Logf(ghServerNetlibUser, "Received unexpected SNAC(1,5), skipping.");
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
        Netlib_Logf(ghServerNetlibUser, "Server returned invalid data, family unavailable.");

        SAFE_FREE(&pServer);
        SAFE_FREE(&pCookie);
        break;
      }

      nloc.cbSize = sizeof(nloc); // establish connection
      nloc.flags = 0;
      nloc.szHost = pServer; // this is horrible assumption - there should not be port
      nloc.wPort = (WORD)DBGetContactSettingWord(NULL, gpszICQProtoName, "OscarPort", DEFAULT_SERVER_PORT);

      hConnection = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)ghServerNetlibUser, (LPARAM)&nloc);
      if (!hConnection && (GetLastError() == 87))
      { // this ensures, an old Miranda will be able to connect also
        nloc.cbSize = NETLIBOPENCONNECTION_V1_SIZE;
        hConnection = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)ghServerNetlibUser, (LPARAM)&nloc);
      }
      
      if (hConnection == NULL)
      {
        Netlib_Logf(ghServerNetlibUser, "Unable to connect to ICQ new family server.");
      } // we want the handler to be called even if the connecting failed
      reqdata->familyhandler(hConnection, pCookie, wCookieLen);

      // Free allocated memory
      // NOTE: "cookie" will get freed when we have connected to the avatar server.
      SAFE_FREE(&pServer);
      SAFE_FREE(&reqdata);
    }

    break;
  }

  case ICQ_SERVER_EXTSTATUS: // our avatar
  {
    DBCONTACTWRITESETTING cws;

    Netlib_Logf(ghServerNetlibUser, "Received our avatar hash & status.");

    if ((wBufferLength >= 0x14) && gbAvatarsEnabled)
    {
      switch (pBuffer[2])
      {
        case 1:
        { // informational packet, just store the hash
          int dummy;
          cws.szModule = gpszICQProtoName;
          cws.szSetting = "AvatarHash";
          cws.value.type = DBVT_BLOB;
          cws.value.pbVal = pBuffer;
          cws.value.cpbVal = 0x14;
          dummy = CallService(MS_DB_CONTACT_WRITESETTING, 0, (LPARAM)&cws);

          break;
        }
        case 0x41: // request to upload avatar data
        case 0x81:
        { // request to re-upload avatar data
          Netlib_Logf(ghServerNetlibUser, "We are requested to post our avatar data.");
          break;
        }
      }
    }
		break;
  }

  case ICQ_ERROR:
  { // Something went wrong, probably the request for avatar family failed
    WORD wError;

    unpackWord(&pBuffer, &wError);
    Netlib_Logf(ghServerNetlibUser, "Server error: %d", wError);
    break;
  }

		// Stuff we don't care about
	case ICQ_SERVER_MOTD:
#ifdef _DEBUG
		Netlib_Logf(ghServerNetlibUser, "Server message of the day");
#endif
		break;

	default:
		Netlib_Logf(ghServerNetlibUser, "Warning: Ignoring SNAC(x01,%2x) - Unknown SNAC (Flags: %u, Ref: %u", pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
		break;

	}

}



static char* buildUinList(int subtype, WORD wMaxLen, HANDLE* hContactResume)
{

	char* szList;
	char* szProto;
	HANDLE hContact;
	WORD wCurrentLen = 0;
	DWORD dwUIN;
	char szUin[33];
	char szLen[2];
	int add;


	szList = (char*)calloc(CallService(MS_DB_CONTACT_GETCOUNT, 0, 0), 11);
	szLen[1] = '\0';

	if (*hContactResume)
		hContact = *hContactResume;
	else
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);


	while(hContact != NULL)
	{
		szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (szProto != NULL && !strcmp(szProto, gpszICQProtoName))
		{
			if (dwUIN = DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0))
			{
				_itoa(dwUIN, szUin, 10);
				szLen[0] = strlen(szUin);

				switch (subtype)
				{

				case BUL_VISIBLE:
					add = ID_STATUS_ONLINE == DBGetContactSettingWord(hContact, gpszICQProtoName, "ApparentMode", 0);
					break;

				case BUL_INVISIBLE:
					add = ID_STATUS_OFFLINE == DBGetContactSettingWord(hContact, gpszICQProtoName, "ApparentMode", 0);
					break;

				default:
					add = 1;

					// If we are in SS mode, we only add those contacts that are
					// not in our SS list, or are awaiting authorization, to our
					// client side list

// This is temporarily disabled cause we cant rely on the Auth flag. The negative thing is that all status updates
// will arrive twice, sometimes contradicting themselves
//					if (icqUsingServerCList && DBGetContactSettingWord(hContact, gpszICQProtoName, "ServerId", 0) &&
//						!DBGetContactSettingByte(hContact, gpszICQProtoName, "Auth", 0))
//						add = 0;

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
					strcat(szList, szUin);
				}
			}
		}

		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
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
	{
		//server doesn't seem to be able to cope with packets larger than 8k
		szList = buildUinList(listType, 0x1800, &hResumeContact);
		nListLen = strlen(szList);

		packet.wLen = nListLen + 10;
		write_flap(&packet, 2);
		packFNACHeader(&packet, wFamily, wSubtype, 0, wFlags<<0x10);
		packBuffer(&packet, szList, (WORD)nListLen);
		sendServPacket(&packet);

		SAFE_FREE(&szList);
	}
	while (hResumeContact);

}



void handleServUINSettings(int nPort, int nIP)
{

	icq_packet packet;


	// CLI_SETUSERINFO
	{

		WORD wAdditionalData = 0;


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
#ifdef DBG_CAPUTF
		wAdditionalData += 16;
#endif
#ifdef DBG_CAPAVATAR
		wAdditionalData += 16;
#endif


		packet.wLen = 30 + wAdditionalData;
		write_flap(&packet, 2);
		packFNACHeader(&packet, ICQ_LOCATION_FAMILY, 0x04, 0, 0x040000);

		/* TLV(5): capability data */
		packWord(&packet, 0x0005);
		packWord(&packet, (WORD)(16 + wAdditionalData));


#ifdef DBG_CAPMTN
        {
			packDWord(&packet, 0x563FC809);
			packDWord(&packet, 0x0B6F41BD);
			packDWord(&packet, 0x9F794226);
			packDWord(&packet, 0x09DFA2F3);
        }
#endif
#ifdef DBG_CAPCH2
		{
			packDWord(&packet, 0x09461349); // AIM_CAPS_ICQSERVERRELAY
			packDWord(&packet, 0x4c7f11d1);
			packDWord(&packet, 0x82224445);
			packDWord(&packet, 0x53540000);
		}
#endif
#ifdef DBG_CAPRTF
		{
			packDWord(&packet, 0x97B12751);	// AIM_CAPS_ICQRTF
			packDWord(&packet, 0x243C4334); // Broadcasts the capability to receive
			packDWord(&packet, 0xAD22D6AB); // RTF messages
			packDWord(&packet, 0xF73F1492);
		}
#endif
#ifdef DBG_CAPUTF
		{
			packDWord(&packet, 0x0946134e);	// CAP_UTF8MSGS
			packDWord(&packet, 0x4c7f11d1); // Broadcasts the capability to receive
			packDWord(&packet, 0x82224445); // UTF8 encoded messages
			packDWord(&packet, 0x53540000);
		}
#endif
#ifdef DBG_CAPAVATAR
		{
			packDWord(&packet, 0x0946134c);	// CAP_EXTRAZ
			packDWord(&packet, 0x4c7f11d1); // Broadcasts the capability to receive
			packDWord(&packet, 0x82224445); // Buddy Avatars
			packDWord(&packet, 0x53540000);
		}
#endif
		if (gbAimEnabled)
		{
			packDWord(&packet, 0x0946134D); // Tells the server we can speak to AIM
			packDWord(&packet, 0x4C7F11D1); // This also change how permit/deny lists work
			packDWord(&packet, 0x82224445);
			packDWord(&packet, 0x53540000);
		}
		packDWord(&packet, 0x09461344); // AIM_CAPS_ICQ
		packDWord(&packet, 0x4c7f11d1);
		packDWord(&packet, 0x82224445);
		packDWord(&packet, 0x53540000);

		sendServPacket(&packet);

	}


	// Set message parameters for channel 1 (CLI_SET_ICBM_PARAMS)
	packet.wLen = 26;
	write_flap(&packet, ICQ_DATA_CHAN);
	packFNACHeader(&packet, ICQ_MSG_FAMILY, 0x02, 0, 0x020000);
	packWord(&packet, 0x0001);              // Channel
#ifdef DBG_CAPMTN
		packDWord(&packet, 0x0000000B);     // Flags
#else
		packDWord(&packet, 0x00000003);     // Flags
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
	packFNACHeader(&packet, ICQ_MSG_FAMILY, 0x02, 0, 0x020000);
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
	packFNACHeader(&packet, ICQ_MSG_FAMILY, 0x02, 0, 0x020000);
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
		if (DBGetContactSettingByte(NULL, gpszICQProtoName, "WebAware", 0))
			wFlags = STATUS_WEBAWARE;

		// DC setting bit flag
		switch (DBGetContactSettingByte(NULL, gpszICQProtoName, "DCType", 0))
		{

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

		packet.wLen = 65;
		write_flap(&packet, 2);
		packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_SET_STATUS, 0, ICQ_CLIENT_SET_STATUS<<0x10);
		packDWord(&packet, 0x00060004);	// TLV 6: Status mode and security flags
		packWord(&packet, wFlags);      // Status flags
		packWord(&packet, wStatus);     // Status
		packDWord(&packet, 0x00080002);	// TLV 8: Error code
		packWord(&packet, 0x0000);
		packDWord(&packet, 0x000c0025); // TLV C: Direct connection info
		packDWord(&packet, nIP);
		packDWord(&packet, nPort);
		packByte(&packet, DC_TYPE);         // TCP/FLAG firewall settings
		packWord(&packet, ICQ_VERSION);
		packDWord(&packet, 0x00000000);     // DC Cookie (TODO)
		packDWord(&packet, WEBFRONTPORT);   // Web front port
		packDWord(&packet, CLIENTFEATURES); // Client features
		packDWord(&packet, 0xffffffff);     // Abused timestamp
		packDWord(&packet, 0x80030403);     // Abused timestamp
		packDWord(&packet, 0x00000000);     // Timestamp
		packWord(&packet, 0x0000);          // Unknown

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
	packDWord(&packet, 0x00010003);
	packDWord(&packet, 0x0110047B);
	packDWord(&packet, 0x00130002);
	packDWord(&packet, 0x0110047B);
	packDWord(&packet, 0x00020001);
	packDWord(&packet, 0x0101047B);
	packDWord(&packet, 0x00030001);
	packDWord(&packet, 0x0110047B);
	packDWord(&packet, 0x00150001);
	packDWord(&packet, 0x0110047B);
	packDWord(&packet, 0x00040001);
	packDWord(&packet, 0x0110047B);
	packDWord(&packet, 0x00060001);
	packDWord(&packet, 0x0110047B);
	packDWord(&packet, 0x00090001);
	packDWord(&packet, 0x0110047B);
	packDWord(&packet, 0x000A0001);
	packDWord(&packet, 0x0110047B);
	packDWord(&packet, 0x000B0001);
	packDWord(&packet, 0x0110047B);

  sendServPacket(&packet);

  if (gbAvatarsEnabled)
  { // Send SNAC 1,4 - request avatar family 0x10 connection

    icq_requestnewfamily(0x10, StartAvatarThread);

    pendingAvatarsStart = 1;
    Netlib_Logf(ghServerNetlibUser, "Requesting Avatar family entry point.");
  }


}
