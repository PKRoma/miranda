// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright � 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001-2002 Jon Keating, Richard Hughes
// Copyright � 2002-2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004-2010 Joe Kucera
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
//  Handles packets from Service family
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"

extern capstr capXStatus[];

void CIcqProto::handleServiceFam(BYTE *pBuffer, WORD wBufferLength, snac_header *pSnacHeader, serverthread_info *info)
{
	icq_packet packet;

	switch (pSnacHeader->wSubtype) {

	case ICQ_SERVER_READY:
#ifdef _DEBUG
		NetLog_Server("Server is ready and is requesting my Family versions");
		NetLog_Server("Sending my Families");
#endif

		// This packet is a response to SRV_FAMILIES SNAC(1,3).
		// This tells the server which SNAC families and their corresponding
		// versions which the client understands. This also seems to identify
		// the client as an ICQ vice AIM client to the server.
		// Miranda mimics the behaviour of ICQ 6
		serverPacketInit(&packet, 54);
		packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_FAMILIES);
		packDWord(&packet, 0x00220001);
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
		serverPacketInit(&packet, 10);
		packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_REQ_RATE_INFO);
		sendServPacket(&packet);
		break;

	case ICQ_SERVER_RATE_INFO:
#ifdef _DEBUG
		NetLog_Server("Server sent Rate Info");
#endif
		/* init rates management */
		m_rates = new rates(this, pBuffer, wBufferLength);
		/* ack rate levels */
#ifdef _DEBUG
		NetLog_Server("Sending Rate Info Ack");
#endif
    m_rates->initAckPacket(&packet);
		sendServPacket(&packet);

		/* CLI_REQINFO - This command requests from the server certain information about the client that is stored on the server. */
#ifdef _DEBUG
		NetLog_Server("Sending CLI_REQINFO");
#endif
		serverPacketInit(&packet, 10);
		packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_REQINFO);
		sendServPacket(&packet);

		if (m_bSsiEnabled)
		{
			cookie_servlist_action* ack;
			DWORD dwCookie;

			DWORD dwLastUpdate = getSettingDword(NULL, "SrvLastUpdate", 0);
			WORD wRecordCount = getSettingWord(NULL, "SrvRecordCount", 0);

			// CLI_REQLISTS - we want to use SSI
#ifdef _DEBUG
			NetLog_Server("Requesting roster rights");
#endif
			serverPacketInit(&packet, 16);
			packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_CLI_REQLISTS);
			packTLVWord(&packet, 0x0B, 0x000F); // mimic ICQ 6
			sendServPacket(&packet);

			if (!wRecordCount) // CLI_REQROSTER
			{ // we do not have any data - request full list
#ifdef _DEBUG
				NetLog_Server("Requesting full roster");
#endif
				serverPacketInit(&packet, 10);
				ack = (cookie_servlist_action*)SAFE_MALLOC(sizeof(cookie_servlist_action));
				if (ack)
				{ // we try to use standalone cookie if available
					ack->dwAction = SSA_CHECK_ROSTER; // loading list
					dwCookie = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_CLI_REQUEST, 0, ack);
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
				serverPacketInit(&packet, 16);
				ack = (cookie_servlist_action*)SAFE_MALLOC(sizeof(cookie_servlist_action));
				if (ack)  // TODO: rewrite - use get list service for empty list
				{ // we try to use standalone cookie if available
					ack->dwAction = SSA_CHECK_ROSTER; // loading list
					dwCookie = AllocateCookie(CKT_SERVERLIST, ICQ_LISTS_CLI_CHECK, 0, ack);
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
#ifdef _DEBUG
		NetLog_Server("Requesting Location rights");
#endif
		serverPacketInit(&packet, 10);
		packFNACHeader(&packet, ICQ_LOCATION_FAMILY, ICQ_LOCATION_CLI_REQ_RIGHTS);
		sendServPacket(&packet);

		// CLI_REQBUDDY
#ifdef _DEBUG
		NetLog_Server("Requesting Client-side contactlist rights");
#endif
		serverPacketInit(&packet, 16);
		packFNACHeader(&packet, ICQ_BUDDY_FAMILY, ICQ_USER_CLI_REQBUDDY);
		// Query flags: 1 = Enable Avatars
		//              2 = Enable offline status message notification
		//              4 = Enable Avatars for offline contacts
    //              8 = Use reject for not authorized contacts
		packTLVWord(&packet, 0x05, 0x0007);
		sendServPacket(&packet);

		// CLI_REQICBM
#ifdef _DEBUG
		NetLog_Server("Sending CLI_REQICBM");
#endif
		serverPacketInit(&packet, 10);
		packFNACHeader(&packet, ICQ_MSG_FAMILY, ICQ_MSG_CLI_REQICBM);
		sendServPacket(&packet);

		// CLI_REQBOS
#ifdef _DEBUG
		NetLog_Server("Sending CLI_REQBOS");
#endif
		serverPacketInit(&packet, 10);
		packFNACHeader(&packet, ICQ_BOS_FAMILY, ICQ_PRIVACY_REQ_RIGHTS);
		sendServPacket(&packet);
		break;

	case ICQ_SERVER_PAUSE:
		NetLog_Server("Server is going down in a few seconds... (Flags: %u)", pSnacHeader->wFlags);
		// This is the list of groups that we want to have on the next server
		serverPacketInit(&packet, 30);
		packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_PAUSE_ACK);
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
#ifdef _DEBUG
			NetLog_Server("Server migration requested (Flags: %u)", pSnacHeader->wFlags);
#endif
			pBuffer += 2; // Unknown, seen: 0
			wBufferLength -= 2;

			oscar_tlv_chain *chain = readIntoTLVChain(&pBuffer, wBufferLength, 0);

			if (info->cookieDataLen > 0)
				SAFE_FREE((void**)&info->cookieData);

			info->newServer = chain->getString(0x05, 1);
			info->newServerSSL = chain->getNumber(0x8E, 1);
			info->cookieData = (BYTE*)chain->getString(0x06, 1);
			info->cookieDataLen = chain->getLength(0x06, 1);

			disposeChain(&chain);

			if (!info->newServer || !info->cookieData)
			{
				icq_LogMessage(LOG_FATAL, LPGEN("A server migration has failed because the server returned invalid data. You must reconnect manually."));
				SAFE_FREE(&info->newServer);
				SAFE_FREE((void**)&info->cookieData);
				info->cookieDataLen = 0;
				info->newServerReady = 0;
				return;
			}

			NetLog_Server("Migration has started. New server will be %s", info->newServer);

			m_iDesiredStatus = m_iStatus;
			SetCurrentStatus(ID_STATUS_CONNECTING); // revert to connecting state

			info->newServerReady = 1;
			info->isMigrating = 1;
		}
		break;

	case ICQ_SERVER_NAME_INFO: // This is the reply to CLI_REQINFO
		{
			BYTE bUinLen;
			oscar_tlv_chain *chain;

#ifdef _DEBUG
			NetLog_Server("Received self info");
#endif
			unpackByte(&pBuffer, &bUinLen);
			pBuffer += bUinLen;
			pBuffer += 4;      /* warning level & user class */
			wBufferLength -= 5 + bUinLen;

			if (pSnacHeader->dwRef == ICQ_CLIENT_REQINFO<<0x10)
			{ // This is during the login sequence
				DWORD dwValue;

				// TLV(x01) User type?
				// TLV(x0C) Empty CLI2CLI Direct connection info
				// TLV(x0A) External IP
				// TLV(x0F) Number of seconds that user has been online
				// TLV(x03) The online since time.
				// TLV(x0A) External IP again
				// TLV(x22) Unknown
				// TLV(x1E) Unknown: empty.
				// TLV(x05) Member of ICQ since.
				// TLV(x14) Unknown
				chain = readIntoTLVChain(&pBuffer, wBufferLength, 0);

				// Save external IP
				dwValue = chain->getDWord(0x0A, 1); 
				setSettingDword(NULL, "IP", dwValue);

				// Save member since timestamp
				dwValue = chain->getDWord(0x05, 1); 
				if (dwValue) setSettingDword(NULL, "MemberTS", dwValue);

				dwValue = chain->getDWord(0x03, 1);
				setSettingDword(NULL, "LogonTS", dwValue ? dwValue : time(NULL));

				disposeChain(&chain);

				// If we are in SSI mode, this is sent after the list is acked instead
				// to make sure that we don't set status before seing the visibility code
				if (!m_bSsiEnabled || info->isMigrating)
					handleServUINSettings(wListenPort, info);
			}
			else if (m_hNotifyNameInfoEvent)
				// Just notify that the set status note & mood process is finished
				SetEvent(m_hNotifyNameInfoEvent);
		}
		break;

	case ICQ_SERVER_RATE_CHANGE:

		if (wBufferLength >= 2)
		{
			WORD wStatus;
			WORD wClass;
			DWORD dwLevel;
			// We now have global rate management, although controlled are only some
			// areas. This should not arrive in most cases. If it does, update our
			// local rate levels & issue broadcast.
			unpackWord(&pBuffer, &wStatus);
			unpackWord(&pBuffer, &wClass);
			pBuffer += 20;
			unpackDWord(&pBuffer, &dwLevel);

			m_ratesMutex->Enter();
			m_rates->updateLevel(wClass, dwLevel);
			m_ratesMutex->Leave();

			if (wStatus == 2 || wStatus == 3)
			{ // this is only the simplest solution, needs rate management to every section
				BroadcastAck(NULL, ICQACKTYPE_RATEWARNING, ACKRESULT_STATUS, (HANDLE)wClass, wStatus);
				if (wStatus == 2)
					NetLog_Server("Rates #%u: Alert", wClass);
				else
					NetLog_Server("Rates #%u: Limit", wClass);
			}
			else if (wStatus == 4)
			{
				BroadcastAck(NULL, ICQACKTYPE_RATEWARNING, ACKRESULT_STATUS, (HANDLE)wClass, wStatus);
				NetLog_Server("Rates #%u: Clear", wClass);
			}
		}

		break;

	case ICQ_SERVER_REDIRECT_SERVICE: // reply to family request, got new connection point
		{
			oscar_tlv_chain *pChain = NULL;
			cookie_family_request *pCookieData;

			if (!(pChain = readIntoTLVChain(&pBuffer, wBufferLength, 0)))
			{
				NetLog_Server("Received Broken Redirect Service SNAC(1,5).");
				break;
			}
			WORD wFamily = pChain->getWord(0x0D, 1);

			// pick request data
			if ((!FindCookie(pSnacHeader->dwRef, NULL, (void**)&pCookieData)) || (pCookieData->wFamily != wFamily))
			{
				disposeChain(&pChain);
				NetLog_Server("Received unexpected SNAC(1,5), skipping.");
				break;
			}

			FreeCookie(pSnacHeader->dwRef);

			{ // new family entry point received
				char *pServer = pChain->getString(0x05, 1);
				BYTE bServerSSL = pChain->getNumber(0x8E, 1);
				char *pCookie = pChain->getString(0x06, 1);
				WORD wCookieLen = pChain->getLength(0x06, 1);

				if (!pServer || !pCookie)
				{
					NetLog_Server("Server returned invalid data, family unavailable.");

					SAFE_FREE(&pServer);
					SAFE_FREE(&pCookie);
					SAFE_FREE((void**)&pCookieData);
					disposeChain(&pChain);
					break;
				}

				// Get new family server ip and port
				WORD wPort = info->wServerPort; // get default port
				parseServerAddress(pServer, &wPort);

				// establish connection
				NETLIBOPENCONNECTION nloc = {0};
				if (m_bGatewayMode)
					nloc.flags |= NLOCF_HTTPGATEWAY;
				nloc.szHost = pServer; 
				nloc.wPort = wPort;

				HANDLE hConnection = NetLib_OpenConnection(m_hServerNetlibUser, wFamily == ICQ_AVATAR_FAMILY ? "Avatar " : NULL, &nloc);

				if (hConnection == NULL)
				{
					NetLog_Server("Unable to connect to ICQ new family server.");
				} // we want the handler to be called even if the connecting failed
				else if (bServerSSL)
				{ /* Start SSL session if requested */
#ifdef _DEBUG
					NetLog_Server("(%d) Starting SSL negotiation", CallService(MS_NETLIB_GETSOCKET, (WPARAM)hConnection, 0));
#endif
					if (!CallService(MS_NETLIB_STARTSSL, (WPARAM)hConnection, 0))
					{
						NetLog_Server("Unable to connect to ICQ new family server, SSL could not be negotiated");
						NetLib_CloseConnection(&hConnection, FALSE);
					}
				}

				(this->*pCookieData->familyHandler)(hConnection, pCookie, wCookieLen);

				// Free allocated memory
				// NOTE: "cookie" will get freed when we have connected to the avatar server.
				disposeChain(&pChain);
				SAFE_FREE(&pServer);
				SAFE_FREE((void**)&pCookieData);
			}

			break;
		}

	case ICQ_SERVER_EXTSTATUS: // our session data
		{
#ifdef _DEBUG
			NetLog_Server("Received owner session data.");
#endif
      while (wBufferLength > 4)
      { // loop thru all items
        WORD itemType = pBuffer[0] * 0x10 | pBuffer[1];
        BYTE itemFlags = pBuffer[2];
        BYTE itemLen = pBuffer[3];

			  if (itemType == AVATAR_HASH_PHOTO) /// TODO: handle photo item
			  { // skip photo item
#ifdef _DEBUG
          NetLog_Server("Photo item recognized");
#endif
			  }
			  else if ((itemType == AVATAR_HASH_STATIC || itemType == AVATAR_HASH_FLASH) && (itemLen >= 0x10))
			  {
#ifdef _DEBUG
          NetLog_Server("Avatar item recognized");
#endif
				  if (m_bAvatarsEnabled && !info->bMyAvatarInited) // signal the server after login
				  { // this refreshes avatar state - it used to work automatically, but now it does not
					  if (getSettingByte(NULL, "ForceOurAvatar", 0))
					  { // keep our avatar
						  TCHAR *file = GetOwnAvatarFileName();
						  SetMyAvatar(0, (LPARAM)file);
						  SAFE_FREE(&file);
					  }
					  else // only change avatar hash to the same one
					  {
						  BYTE hash[0x14];

						  memcpy(hash, pBuffer, 0x14);
						  hash[2] = 1; // update image status
						  updateServAvatarHash(hash, 0x14);
					  }
					  info->bMyAvatarInited = TRUE;
					  break;
				  }
          // process owner avatar hash changed notification
          handleAvatarOwnerHash(itemType, itemFlags, pBuffer, itemLen + 4);
        }
        else if (itemType == 0x02)
        {
#ifdef _DEBUG
          NetLog_Server("Status message item recognized");
#endif
        }
        else if (itemType == 0x0E)
        {
#ifdef _DEBUG
          NetLog_Server("Status mood item recognized");
#endif
        }

        // move to next item
			  if (wBufferLength >= itemLen + 4)
			  {
 					wBufferLength -= itemLen + 4;
  				pBuffer += itemLen + 4;
	  		}
		  	else
			  {
				  pBuffer += wBufferLength;
				  wBufferLength = 0;
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


char* CIcqProto::buildUinList(int subtype, WORD wMaxLen, HANDLE* hContactResume)
{
	char* szList;
	HANDLE hContact;
	WORD wCurrentLen = 0;
	DWORD dwUIN;
	uid_str szUID;
	char szLen[2];
	int add;

	szList = (char*)SAFE_MALLOC(CallService(MS_DB_CONTACT_GETCOUNT, 0, 0) * UINMAXLEN);
	szLen[1] = '\0';

	if (*hContactResume)
		hContact = *hContactResume;
	else
		hContact = FindFirstContact();

	while (hContact != NULL)
	{
		if (!getContactUid(hContact, &dwUIN, &szUID))
		{
			szLen[0] = strlennull(strUID(dwUIN, szUID));

			switch (subtype)
			{

			case BUL_VISIBLE:
				add = ID_STATUS_ONLINE == getSettingWord(hContact, "ApparentMode", 0);
				break;

			case BUL_INVISIBLE:
				add = ID_STATUS_OFFLINE == getSettingWord(hContact, "ApparentMode", 0);
				break;

			case BUL_TEMPVISIBLE:
				add = getSettingByte(hContact, "TemporaryVisible", 0);
				// clear temporary flag
				// Here we assume that all temporary contacts will be in one packet
				setSettingByte(hContact, "TemporaryVisible", 0);
				break;

			default:
				add = 1;

				// If we are in SS mode, we only add those contacts that are
				// not in our SS list, or are awaiting authorization, to our
				// client side list
				if (m_bSsiEnabled && getSettingWord(hContact, DBSETTING_SERVLIST_ID, 0) &&
					!getSettingByte(hContact, "Auth", 0))
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
				strcat(szList, szUID);
			}
		}

		hContact = FindNextContact(hContact);
	}
	*hContactResume = NULL;

	return szList;
}


void CIcqProto::sendEntireListServ(WORD wFamily, WORD wSubtype, int listType)
{
	HANDLE hResumeContact = NULL;

	do
	{ // server doesn't seem to be able to cope with packets larger than 8k
		// send only about 100contacts per packet
		char *szList = buildUinList(listType, 0x3E8, &hResumeContact);
		int nListLen = strlennull(szList);

		if (nListLen)
		{
			icq_packet packet;

			serverPacketInit(&packet, (WORD)(nListLen + 10));
			packFNACHeader(&packet, wFamily, wSubtype);
			packBuffer(&packet, (LPBYTE)szList, (WORD)nListLen);
			sendServPacket(&packet);
		}

		SAFE_FREE((void**)&szList);
	}
	while (hResumeContact);
}


static void packShortCapability(icq_packet *packet, WORD wCapability)
{ // pack standard capability
	DWORD dwQ1 = 0x09460000 | wCapability;

	packDWord(packet, dwQ1); 
	packDWord(packet, 0x4c7f11d1);
	packDWord(packet, 0x82224445);
	packDWord(packet, 0x53540000);
}


// CLI_SETUSERINFO
void CIcqProto::setUserInfo()
{ 
	icq_packet packet;
	WORD wAdditionalData = 0;
	BYTE bXStatus = getContactXStatus(NULL);

	if (m_bAimEnabled)
		wAdditionalData += 16;
#ifdef DBG_CAPMTN
	wAdditionalData += 16;
#endif
	if (m_bUtfEnabled)
		wAdditionalData += 16;
#ifdef DBG_NEWCAPS
	wAdditionalData += 16;
#endif
#ifdef DBG_CAPXTRAZ
	wAdditionalData += 16;
#endif
#ifdef DBG_OSCARFT
	wAdditionalData += 16;
#endif
	if (m_bAvatarsEnabled)
		wAdditionalData += 16;
	if (m_bXStatusEnabled && bXStatus != 0)
		wAdditionalData += 16;
#ifdef DBG_CAPHTML
	wAdditionalData += 16;
#endif
#ifdef DBG_AIMCONTACTSEND
	wAdditionalData += 16;
#endif

	//MIM/PackName
	bool bHasPackName = false;
	DBVARIANT dbv;
	if ( !DBGetContactSettingString(NULL, "ICQCaps", "PackName", &dbv )) {
		//MIM/PackName
		bHasPackName = true;
		wAdditionalData += 16;
	}

	serverPacketInit(&packet, (WORD)(62 + wAdditionalData));
	packFNACHeader(&packet, ICQ_LOCATION_FAMILY, ICQ_LOCATION_SET_USER_INFO);

	/* TLV(5): capability data */
	packWord(&packet, 0x0005);
	packWord(&packet, (WORD)(48 + wAdditionalData));

#ifdef DBG_CAPMTN
	{
		packDWord(&packet, 0x563FC809); // CAP_TYPING
		packDWord(&packet, 0x0B6F41BD);
		packDWord(&packet, 0x9F794226);
		packDWord(&packet, 0x09DFA2F3);
	}
#endif
	{
		packShortCapability(&packet, 0x1349);  // AIM_CAPS_ICQSERVERRELAY
	}
	if (m_bUtfEnabled)
	{
		packShortCapability(&packet, 0x134E);  // CAP_UTF8MSGS
	} // Broadcasts the capability to receive UTF8 encoded messages
#ifdef DBG_NEWCAPS
	{
		packShortCapability(&packet, 0x0000);  // CAP_SHORTCAPS
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
	if (m_bAvatarsEnabled)
	{
		packShortCapability(&packet, 0x134C);  // CAP_DEVILS
	}
#ifdef DBG_OSCARFT
	{
		packShortCapability(&packet, 0x1343);  // CAP_AIM_FILE
	} // Broadcasts the capability to receive Oscar File Transfers
#endif
	if (m_bAimEnabled)
	{
		packShortCapability(&packet, 0x134D);  // CAP_AIM_COMPATIBLE
  } // Tells the server we can speak to AIM
#ifdef DBG_AIMCONTACTSEND
	{
		packShortCapability(&packet, 0x134B);  // CAP_SENDBUDDYLIST
	}
#endif
	if (m_bXStatusEnabled && bXStatus != 0)
	{
		packBuffer(&packet, capXStatus[bXStatus-1], BINARY_CAP_SIZE);
	}

	packShortCapability(&packet, 0x1344);      // CAP_ICQDIRECT

#ifdef DBG_CAPHTML
	packShortCapability(&packet, 0x0002);      // CAP_HTMLMSGS
#endif

	packDWord(&packet, 0x4D697261);   // Miranda Signature
	packDWord(&packet, 0x6E64614D);
	packDWord(&packet, MIRANDA_VERSION);
	packDWord(&packet, ICQ_PLUG_VERSION);

	//MIM/PackName
	if ( bHasPackName ) {
		packBuffer(&packet, (BYTE*)dbv.pszVal, 0x10);
		ICQFreeVariant(&dbv);
	}

	sendServPacket(&packet);
}


void CIcqProto::handleServUINSettings(int nPort, serverthread_info *info)
{
	icq_packet packet;

	setUserInfo();

	/* SNAC 3,4: Tell server who's on our list (deprecated) */
  /* SNAC 3,15: Try to add unauthorised contacts to temporary list */
	sendEntireListServ(ICQ_BUDDY_FAMILY, ICQ_USER_ADDTOTEMPLIST, BUL_ALLCONTACTS);

	if (m_iDesiredStatus == ID_STATUS_INVISIBLE)
	{
		/* Tell server who's on our visible list (deprecated) */
		if (!m_bSsiEnabled)
			sendEntireListServ(ICQ_BOS_FAMILY, ICQ_CLI_ADDVISIBLE, BUL_VISIBLE);
		else
			updateServVisibilityCode(3);
	}

	if (m_iDesiredStatus != ID_STATUS_INVISIBLE)
	{
		/* Tell server who's on our invisible list (deprecated) */
		if (!m_bSsiEnabled)
			sendEntireListServ(ICQ_BOS_FAMILY, ICQ_CLI_ADDINVISIBLE, BUL_INVISIBLE);
		else
			updateServVisibilityCode(4);
	}

	// SNAC 1,1E: Set status
	{
		DWORD dwDirectCookie = rand() ^ (rand() << 16);

		// Get status
		WORD wStatus = MirandaStatusToIcq(m_iDesiredStatus);

		// Get status note & mood
		char *szStatusNote = PrepareStatusNote(m_iDesiredStatus);
		BYTE bXStatus = getContactXStatus(NULL);
		char szMoodData[32];

		// prepare mood id
		if (m_bMoodsEnabled && bXStatus && moodXStatus[bXStatus-1] != -1)
			null_snprintf(szMoodData, SIZEOF(szMoodData), "icqmood%d", moodXStatus[bXStatus-1]);
		else
			szMoodData[0] = '\0';

		//! Tricky code, this ensures that the status note will be saved to the directory
		SetStatusNote(szStatusNote, m_bGatewayMode ? 5000 : 2500, TRUE);

		WORD wStatusNoteLen = strlennull(szStatusNote);
		WORD wStatusMoodLen = strlennull(szMoodData);
		WORD wSessionDataLen = (wStatusNoteLen ? wStatusNoteLen + 4 : 0) + 4 + wStatusMoodLen + 4;

		serverPacketInit(&packet, (WORD)(71 + (wSessionDataLen ? wSessionDataLen + 4 : 0)));
		packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_SET_STATUS);
		packDWord(&packet, 0x00060004);             // TLV 6: Status mode and security flags
		packWord(&packet, GetMyStatusFlags());      // Status flags
		packWord(&packet, wStatus);                 // Status
		packTLVWord(&packet, 0x0008, 0x0A06);       // TLV 8: Independent Status Messages
		packDWord(&packet, 0x000c0025);             // TLV C: Direct connection info
		packDWord(&packet, getSettingDword(NULL, "RealIP", 0));
		packDWord(&packet, nPort);
		packByte(&packet, DC_TYPE);                 // TCP/FLAG firewall settings
		packWord(&packet, ICQ_VERSION);
		packDWord(&packet, dwDirectCookie);         // DC Cookie
		packDWord(&packet, WEBFRONTPORT);           // Web front port
		packDWord(&packet, CLIENTFEATURES);         // Client features
#if defined( _UNICODE )
		packDWord(&packet, 0x7fffffff); // Abused timestamp
#else
		packDWord(&packet, 0xffffffff); // Abused timestamp
#endif
		packDWord(&packet, ICQ_PLUG_VERSION);       // Abused timestamp
		if (ServiceExists("SecureIM/IsContactSecured"))
			packDWord(&packet, 0x5AFEC0DE);           // SecureIM Abuse
		else
			packDWord(&packet, 0x00000000);           // Timestamp
		packWord(&packet, 0x0000);                  // Unknown
		packTLVWord(&packet, 0x001F, 0x0000);

		if (wSessionDataLen)
		{ // Pack session data
			packWord(&packet, 0x1D);                  // TLV 1D
			packWord(&packet, wSessionDataLen);       // TLV length
			packWord(&packet, 0x02);                  // Item Type
			if (wStatusNoteLen)
			{
				packWord(&packet, 0x400 | (WORD)(wStatusNoteLen + 4)); // Flags + Item Length
				packWord(&packet, wStatusNoteLen);      // Text Length
				packBuffer(&packet, (LPBYTE)szStatusNote, wStatusNoteLen);
				packWord(&packet, 0);                   // Encoding not specified (utf-8 is default)
			}
			else
				packWord(&packet, 0);                   // Flags + Item Length
			packWord(&packet, 0x0E);                  // Item Type
			packWord(&packet, wStatusMoodLen);        // Flags + Item Length
			if (wStatusMoodLen)
				packBuffer(&packet, (LPBYTE)szMoodData, wStatusMoodLen); // Mood

			// Save current status note & mood
			setSettingStringUtf(NULL, DBSETTING_STATUS_NOTE, szStatusNote);
			setSettingString(NULL, DBSETTING_STATUS_MOOD, szMoodData);
		}
		// Release memory
		SAFE_FREE(&szStatusNote);

		sendServPacket(&packet);
	}

	/* SNAC 1,11 */
	serverPacketInit(&packet, 14);
	packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_SET_IDLE);
	packDWord(&packet, 0x00000000);

	sendServPacket(&packet);
	m_bIdleAllow = 0;

	// Change status
	SetCurrentStatus(m_iDesiredStatus);

	// Finish Login sequence
	serverPacketInit(&packet, 98);
	packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_READY);
	packDWord(&packet, 0x00220001); // imitate ICQ 6 behaviour
	packDWord(&packet, 0x0110164f);
	packDWord(&packet, 0x00010004);
	packDWord(&packet, 0x0110164f);
	packDWord(&packet, 0x00130004);
	packDWord(&packet, 0x0110164f);
	packDWord(&packet, 0x00020001);
	packDWord(&packet, 0x0110164f);
	packDWord(&packet, 0x00030001);
	packDWord(&packet, 0x0110164f);
	packDWord(&packet, 0x00150001);
	packDWord(&packet, 0x0110164f);
	packDWord(&packet, 0x00040001);
	packDWord(&packet, 0x0110164f);
	packDWord(&packet, 0x00060001);
	packDWord(&packet, 0x0110164f);
	packDWord(&packet, 0x00090001);
	packDWord(&packet, 0x0110164f);
	packDWord(&packet, 0x000A0001);
	packDWord(&packet, 0x0110164f);
	packDWord(&packet, 0x000B0001);
	packDWord(&packet, 0x0110164f);

	sendServPacket(&packet);

	NetLog_Server(" *** Yeehah, login sequence complete");

	// login sequence is complete enter logged-in mode
	info->bLoggedIn = 1;
	m_bConnectionLost = FALSE;

	// enable auto info-update routine
	icq_EnableUserLookup(TRUE);

	if (!info->isMigrating)
	{ /* Get Offline Messages Reqeust */
		cookie_offline_messages *ack = (cookie_offline_messages*)SAFE_MALLOC(sizeof(cookie_offline_messages));
		if (ack)
		{
			DWORD dwCookie = AllocateCookie(CKT_OFFLINEMESSAGE, ICQ_MSG_CLI_REQ_OFFLINE, 0, ack);

			serverPacketInit(&packet, 10);
			packFNACHeader(&packet, ICQ_MSG_FAMILY, ICQ_MSG_CLI_REQ_OFFLINE, 0, dwCookie);

			sendServPacket(&packet);
		}
		else
			icq_LogMessage(LOG_WARNING, LPGEN("Failed to request offline messages. They may be received next time you log in."));

		// Update our information from the server
		sendOwnerInfoRequest();

		// Request info updates on all contacts
		icq_RescanInfoUpdate();

		// Start sending Keep-Alive packets
		StartKeepAlive(info);

		if (m_bAvatarsEnabled)
		{ // Send SNAC 1,4 - request avatar family 0x10 connection
			icq_requestnewfamily(ICQ_AVATAR_FAMILY, &CIcqProto::StartAvatarThread);

			m_avatarsConnectionPending = TRUE;
			NetLog_Server("Requesting Avatar family entry point.");
		}
	}
	info->isMigrating = 0;

	if (m_bAimEnabled)
	{
		char **szAwayMsg = NULL;
		icq_lock l(m_modeMsgsMutex);

		szAwayMsg = MirandaStatusToAwayMsg(m_iStatus);
		if (szAwayMsg)
			icq_sendSetAimAwayMsgServ(*szAwayMsg);
	}
}
