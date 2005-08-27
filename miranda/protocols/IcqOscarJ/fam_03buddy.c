// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright � 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001,2002 Jon Keating, Richard Hughes
// Copyright � 2002,2003,2004 Martin  berg, Sam Kothari, Robert Rainwater
// Copyright � 2004,2005 Joe Kucera
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
//  Handles packets from Buddy family
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"
#include "m_cluiframes.h"

static void handleUserOffline(BYTE* buf, WORD wPackLen);
static void handleUserOnline(BYTE* buf, WORD wPackLen);
static void handleReplyBuddy(BYTE* buf, WORD wPackLen);
static void handleNotifyRejected(BYTE* buf, WORD wPackLen);

extern DWORD dwLocalDirectConnCookie;

extern const capstr capAimIcon;
extern const char* cliSpamBot;
extern char* detectUserClient(HANDLE hContact, DWORD dwUin, WORD wVersion, DWORD dwFT1, DWORD dwFT2, DWORD dwFT3, DWORD dwOnlineSince, BYTE* caps, WORD wLen, DWORD* dwClientId);


void handleBuddyFam(unsigned char* pBuffer, WORD wBufferLength, snac_header* pSnacHeader)
{
	switch (pSnacHeader->wSubtype)
	{
	case ICQ_USER_ONLINE:
		handleUserOnline(pBuffer, wBufferLength);
		break;

	case ICQ_USER_OFFLINE:
		handleUserOffline(pBuffer, wBufferLength);
		break;

	case ICQ_USER_SRV_REPLYBUDDY:
		handleReplyBuddy(pBuffer, wBufferLength);
		break;

	case ICQ_USER_NOTIFY_REJECTED:
    handleNotifyRejected(pBuffer, wBufferLength);
		break;

  case ICQ_ERROR:
  {
    WORD wError;

    if (wBufferLength >= 2)
      unpackWord(&pBuffer, &wError);
    else 
      wError = 0;

    LogFamilyError(ICQ_BUDDY_FAMILY, wError);
    break;
  }

	default:
		NetLog_Server("Warning: Ignoring SNAC(x%02x,x%02x) - Unknown SNAC (Flags: %u, Ref: %u)", ICQ_BUDDY_FAMILY, pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
		break;
	}
}


// TLV(1) Unknown (x50)
// TLV(2) Member since (not sent)
// TLV(3) Online since
// TLV(4) Idle time (not sent)
// TLV(6) New status
// TLV(A) External IP
// TLV(C) DC Info
// TLV(D) Capabilities
// TLV(F) Session timer (in seconds)
// TLV(1D) Avatar Hash (20 bytes)
static void handleUserOnline(BYTE* buf, WORD wLen)
{
	HANDLE hContact;
	DWORD dwPort;
	DWORD dwRealIP;
	DWORD dwIP;
	DWORD dwUIN;
	DWORD dwDirectConnCookie;
	DWORD dwFT1, dwFT2, dwFT3;
	LPSTR szClient = 0;
	DWORD dwClientId = 0;
	WORD wVersion = 0;
	WORD wTLVCount;
	WORD wWarningLevel;
	WORD wStatusFlags;
	WORD wStatus;
	BYTE nTCPFlag;
	DWORD dwOnlineSince;
	WORD wIdleTimer = 0;
	time_t tIdleTS = 0;
  char *szUID;

	// Unpack the sender's user ID
  if (!unpackUID(&buf, &wLen, &dwUIN, &szUID)) return;

	// Syntax check
	if (wLen < 4)
		return;

	// Warning level?
	unpackWord(&buf, &wWarningLevel);
	wLen -= 2;

	// TLV count
	unpackWord(&buf, &wTLVCount);
	wLen -= 2;

  // Determine contact
  if (dwUIN)
    hContact = HContactFromUIN(dwUIN, NULL);
  else
    hContact = HContactFromUID(szUID, NULL);

	// Ignore status notification if the user is not already on our list
	if (hContact == INVALID_HANDLE_VALUE)
	{
#ifdef _DEBUG
    if (dwUIN)
		  NetLog_Server("Ignoring user online (%u)", dwUIN);
    else
		  NetLog_Server("Ignoring user online (%s)", szUID);
#endif
		return;
	}

	// Read user info TLVs
	{
		oscar_tlv_chain* pChain;
		oscar_tlv* pTLV;

		// Syntax check
		if (wLen < 4)
			return;

		// Get chain
		if (!(pChain = readIntoTLVChain(&buf, wLen, wTLVCount)))
			return;

    if (dwUIN)
    {
		  // Get DC info TLV
		  pTLV = getTLV(pChain, 0x0C, 1);
		  if (pTLV && (pTLV->wLen >= 15))
		  {
			  unsigned char* pBuffer;

			  pBuffer = pTLV->pData;
			  unpackDWord(&pBuffer, &dwRealIP);
			  unpackDWord(&pBuffer, &dwPort);
			  unpackByte(&pBuffer,  &nTCPFlag);
			  unpackWord(&pBuffer,  &wVersion);
			  unpackDWord(&pBuffer, &dwDirectConnCookie);
			  pBuffer += 4; // Web front port
			  pBuffer += 4; // Client features

        // Get faked time signatures, used to identify clients
        if (pTLV->wLen >= 0x23)
        {
          unpackDWord(&pBuffer, &dwFT1);
          unpackDWord(&pBuffer, &dwFT2);
          unpackDWord(&pBuffer, &dwFT3);
        }
        else
        {  // just for the case the timestamps are not there
          dwFT1 = dwFT2 = dwFT3 = 0;
        }
		  }
		  else
		  {
			  // This client doesnt want DCs
			  dwRealIP = 0;
			  dwPort = 0;
			  nTCPFlag = 0;
			  dwDirectConnCookie = 0;
        dwFT1 = dwFT2 = dwFT3 = 0;
		  }

		  // Get Status info TLV
		  pTLV = getTLV(pChain, 0x06, 1);
		  if (pTLV && (pTLV->wLen >= 4))
		  {
			  unsigned char* pBuffer;

			  pBuffer = pTLV->pData;
			  unpackWord(&pBuffer, &wStatusFlags);
			  unpackWord(&pBuffer, &wStatus);
		  }
		  else
		  {
			  // Huh? No status TLV? Lets guess then...
			  wStatusFlags = 0;
			  wStatus = ICQ_STATUS_ONLINE;
		  }
    }
    else
    {
      // Get Class word
      WORD wClass = getWordFromChain(pChain, 0x01, 1);

      if (wClass & CLASS_AWAY)
        wStatus = ID_STATUS_AWAY;
      else
        wStatus = ID_STATUS_ONLINE;

      wStatusFlags = 0;
  	  dwRealIP = 0;
		  dwPort = 0;
		  nTCPFlag = 0;
		  dwDirectConnCookie = 0;
      dwFT1 = dwFT2 = dwFT3 = 0;
    }

#ifdef _DEBUG
		NetLog_Server("Flags are %x", wStatusFlags);
		NetLog_Server("Status is %x", wStatus);
#endif

		// Get IP TLV
		dwIP = getDWordFromChain(pChain, 0x0a, 1);

		// Get Online Since TLV
		dwOnlineSince = getDWordFromChain(pChain, 0x03, 1);

		// Get Idle timer TLV
		wIdleTimer = getWordFromChain(pChain, 0x04, 1);
		if (wIdleTimer)
		{
			time(&tIdleTS);
			tIdleTS -= (wIdleTimer*60);
		};

#ifdef _DEBUG
		if (wIdleTimer)
			NetLog_Server("Idle timer is %u.", wIdleTimer);
		NetLog_Server("Online since %s", asctime(localtime(&dwOnlineSince)));
#endif

		// Check client capabilities
		if (hContact != NULL)
		{
			WORD wOldStatus;

			wOldStatus = ICQGetContactSettingWord(hContact, "Status", ID_STATUS_OFFLINE);

			// Get Avatar Hash TLV
			pTLV = getTLV(pChain, 0x1D, 1);
      if (pTLV)
        handleAvatarContactHash(dwUIN, hContact, pTLV->pData, pTLV->wLen, wOldStatus);
      else
        handleAvatarContactHash(dwUIN, hContact, NULL, 0, wOldStatus);

			// Update the contact's capabilities
			if (wOldStatus == ID_STATUS_OFFLINE)
			{
				// Delete the capabilities we saved the last time this contact came online
				ClearAllContactCapabilities(hContact);

        {
          BYTE capBufDat[0x200]; // max 0x20 caps, currently used max 0x12
          BYTE* capBuf;
          WORD capLen = 0;
          oscar_tlv* pNewTLV;

				  // Get Location Capability Info TLVs
				  pTLV = getTLV(pChain, 0x0D, 1);
          pNewTLV = getTLV(pChain, 0x19, 1);

				  if (pTLV && (pTLV->wLen >= 16))
            capLen = pTLV->wLen;

          if (pNewTLV && (pNewTLV->wLen >= 2))
            capLen += (pNewTLV->wLen * 8);

          if (capLen)
          {
            int i;
            BYTE* pCap;

            capBuf = pCap = capBufDat;

            if (pTLV && (pTLV->wLen >= 16))
            { // copy classic Capabilities
              pCap += pTLV->wLen;
              memcpy(capBuf, pTLV->pData, pTLV->wLen);
            }

            if (pNewTLV && (pNewTLV->wLen >= 2))
            { // get new Capabilities
              capstr tmp;
              BYTE* capNew = pNewTLV->pData;

              memcpy(tmp, capAimIcon, 0x10); 

              for (i = 0; i<pNewTLV->wLen; i+=2)
              {
                BYTE capVal;

                capVal = capNew[0];
                tmp[2] = capVal;
                capVal = capNew[1];
                tmp[3] = capVal;
  
                capNew += 2;

                memcpy(pCap, tmp, 0x10);
                pCap += 0x10;
              }
            }
  					AddCapabilitiesFromBuffer(hContact, capBuf, capLen);
          }
          else
          { // no capability
            NetLog_Server("No capability info TLVs");
          }

          // handle Xtraz status
          handleXStatusCaps(hContact, capBuf, capLen);

          szClient = detectUserClient(hContact, dwUIN, wVersion, dwFT1, dwFT2, dwFT3, dwOnlineSince, capBuf, capLen, &dwClientId);
				}

#ifdef _DEBUG
				if (CheckContactCapabilities(hContact, CAPF_SRV_RELAY))
					NetLog_Server("Supports advanced messages");
				else
					NetLog_Server("Does NOT support advanced messages");
#endif

				if (dwUIN && wVersion < 8)
				{
					ClearContactCapabilities(hContact, CAPF_SRV_RELAY);
					NetLog_Server("Forcing simple messages due to compability issues");
				}
			}
      else
      {
        szClient = (char*)-1; // we don't want to client be overwritten if no capabilities received

				// Get Capability Info TLV
				pTLV = getTLV(pChain, 0x0D, 1);

				if (pTLV && (pTLV->wLen >= 16))
				{
          // handle Xtraz status
          handleXStatusCaps(hContact, pTLV->pData, pTLV->wLen);
        }
      }
		}

		// Free TLV chain
		disposeChain(&pChain);
	}

	// Save contacts details in database
  if (hContact != NULL)
  {
    if (szClient == 0) szClient = ""; // if no detection, set uknown
    ICQWriteContactSettingDword(hContact,  "ClientID",     dwClientId);

		ICQWriteContactSettingDword(hContact,  "LogonTS",      dwOnlineSince);
    if (dwUIN)
    { // on AIM these are not used
		  ICQWriteContactSettingDword(hContact,  "IP",           dwIP);
		  ICQWriteContactSettingDword(hContact,  "RealIP",       dwRealIP);
		  ICQWriteContactSettingDword(hContact,  "DirectCookie", dwDirectConnCookie);
      ICQWriteContactSettingByte(hContact,   "DCType",       (BYTE)nTCPFlag);
		  ICQWriteContactSettingWord(hContact,   "UserPort",     (WORD)(dwPort & 0xffff));
		  ICQWriteContactSettingWord(hContact,   "Version",      wVersion);
    }
		if (szClient != (char*)-1) ICQWriteContactSettingString(hContact, "MirVer", szClient);
		ICQWriteContactSettingWord(hContact,   "Status",       (WORD)IcqStatusToMiranda(wStatus));
		ICQWriteContactSettingDword(hContact,  "IdleTS",       tIdleTS);

		// Update info?
		if (dwUIN && ((time(NULL) - ICQGetContactSettingDword(hContact, "InfoTS", 0)) > UPDATE_THRESHOLD))
			icq_QueueUser(hContact);
	}
	else
	{
		dwLocalDirectConnCookie = dwDirectConnCookie;
	}

	// And a small log notice...
  if (dwUIN)
	  NetLog_Server("%u changed status to %s (v%d).",
		  dwUIN, MirandaStatusToString(IcqStatusToMiranda(wStatus)), wVersion);
  else
	  NetLog_Server("%s changed status to %s (v%d).",
		  szUID, MirandaStatusToString(IcqStatusToMiranda(wStatus)), wVersion);

  if (szClient == cliSpamBot)
  {
    if (ICQGetContactSettingByte(NULL, "KillSpambots", DEFAULT_KILLSPAM_ENABLED))
    { // TODO: show popup informing that user was blocked
      AddToSpammerList(dwUIN);
			CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);

      NetLog_Server("Contact %u deleted", dwUIN);
    }
  }
}


static void handleUserOffline(BYTE *buf, WORD wLen)
{
	HANDLE hContact;
	DWORD dwUIN;
  char *szUID;

	// Unpack the sender's user ID
  if (!unpackUID(&buf, &wLen, &dwUIN, &szUID)) return;

  if (dwUIN)
	  hContact = HContactFromUIN(dwUIN, NULL);
  else
    hContact = HContactFromUID(szUID, NULL);

	// Skip contacts that are not already on our list
	if (hContact != INVALID_HANDLE_VALUE)
	{
    if (dwUIN)
		  NetLog_Server("%u went offline.", dwUIN);
    else
      NetLog_Server("%s went offline.", szUID);

		ICQWriteContactSettingWord(hContact, "Status", ID_STATUS_OFFLINE);
		ICQWriteContactSettingDword(hContact, "IdleTS", 0);
    // close Direct Connections to that user
    CloseContactDirectConns(hContact);
    // clear Xtraz status
    handleXStatusCaps(hContact, NULL, 0);
	}
}


static void handleReplyBuddy(BYTE *buf, WORD wPackLen)
{
	oscar_tlv_chain *pChain;

	pChain = readIntoTLVChain(&buf, wPackLen, 0);

	if (pChain)
	{
		DWORD wMaxUins;
		DWORD wMaxWatchers;

		wMaxUins = getWordFromChain(pChain, 1, 1);
		wMaxWatchers = getWordFromChain(pChain, 2, 1);

		NetLog_Server("MaxUINs %u", wMaxUins);
		NetLog_Server("MaxWatchers %u", wMaxWatchers);

		disposeChain(&pChain);
	}
	else
	{
		NetLog_Server("Error: Malformed BuddyReply");
	}
}


static void handleNotifyRejected(BYTE *buf, WORD wPackLen)
{
	DWORD dwUIN;
  char *szUID;

  if (!unpackUID(&buf, &wPackLen, &dwUIN, &szUID))
    return;

  if (dwUIN)
    NetLog_Server("SNAC(x03,x0a) - SRV_NOTIFICATION_REJECTED for %u", dwUIN);
  else
    NetLog_Server("SNAC(x03,x0a) - SRV_NOTIFICATION_REJECTED for %s", szUID);
}