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

extern DWORD dwLocalDirectConnCookie;


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


capstr* MatchCap(char* buf, int bufsize, const capstr* cap, int capsize)
{
  while (bufsize>0) // search the buffer for a capability
  {
    if (!memcmp(buf, cap, capsize))
    {
      return (capstr*)buf; // give found capability for version info
    }
    else
    {
      buf += 0x10;
      bufsize -= 0x10;
    }
  }
  return 0;
}


const capstr capMirandaIm = {'M', 'i', 'r', 'a', 'n', 'd', 'a', 'M', 0, 0, 0, 0, 0, 0, 0, 0};
const capstr capTrillian  = {0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34, 0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x09};
const capstr capTrilCrypt = {0xf2, 0xe7, 0xc7, 0xf4, 0xfe, 0xad, 0x4d, 0xfb, 0xb2, 0x35, 0x36, 0x79, 0x8b, 0xdf, 0x00, 0x00};
const capstr capSim       = {'S', 'I', 'M', ' ', 'c', 'l', 'i', 'e', 'n', 't', ' ', ' ', 0, 0, 0, 0};
const capstr capSimOld    = {0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34, 0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x00};
const capstr capLicq      = {'L', 'i', 'c', 'q', ' ', 'c', 'l', 'i', 'e', 'n', 't', ' ', 0, 0, 0, 0};
const capstr capKopete    = {'K', 'o', 'p', 'e', 't', 'e', ' ', 'I', 'C', 'Q', ' ', ' ', 0, 0, 0, 0};
const capstr capmIcq      = {'m', 'I', 'C', 'Q', ' ', 0xA9, ' ', 'R', '.', 'K', '.', ' ', 0, 0, 0, 0};
const capstr capAndRQ     = {'&', 'R', 'Q', 'i', 'n', 's', 'i', 'd', 'e', 0, 0, 0, 0, 0, 0, 0};
const capstr capQip       = {0x56, 0x3F, 0xC8, 0x09, 0x0B, 0x6F, 0x41, 'Q', 'I', 'P', ' ', '2', '0', '0', '5', 'a'};
const capstr capIm2       = {0x74, 0xED, 0xC3, 0x36, 0x44, 0xDF, 0x48, 0x5B, 0x8B, 0x1C, 0x67, 0x1A, 0x1F, 0x86, 0x09, 0x9F};
const capstr capMacIcq    = {0xdd, 0x16, 0xf2, 0x02, 0x84, 0xe6, 0x11, 0xd4, 0x90, 0xdb, 0x00, 0x10, 0x4b, 0x9b, 0x4b, 0x7d};
const capstr capRichText  = {0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34, 0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x92};
const capstr capIs2001    = {0x2e, 0x7a, 0x64, 0x75, 0xfa, 0xdf, 0x4d, 0xc8, 0x88, 0x6f, 0xea, 0x35, 0x95, 0xfd, 0xb6, 0xdf};
const capstr capIs2002    = {0x10, 0xcf, 0x40, 0xd1, 0x4c, 0x7f, 0x11, 0xd1, 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};
const capstr capStr20012  = {0xa0, 0xe9, 0x3f, 0x37, 0x4f, 0xe9, 0xd3, 0x11, 0xbc, 0xd2, 0x00, 0x04, 0xac, 0x96, 0xdd, 0x96};
const capstr capXtraz     = {0x1A, 0x09, 0x3C, 0x6C, 0xD7, 0xFD, 0x4E, 0xC5, 0x9D, 0x51, 0xA6, 0x47, 0x4E, 0x34, 0xF5, 0xA0};
const capstr capIcq5Extra = {0x09, 0x46, 0x13, 0x43, 0x4C, 0x7F, 0x11, 0xD1, 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}; // CAP_AIM_SENDFILE
const capstr capAimIcon   = {0x09, 0x46, 0x13, 0x46, 0x4c, 0x7f, 0x11, 0xd1, 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}; // CAP_AIM_BUDDYICON

char* cliLibicq2k  = "libicq2000";
char* cliLicqVer   = "Licq %u.%u";
char* cliLicqVerL  = "Licq %u.%u.%u";
char* cliCentericq = "Centericq";
char* cliLibicqUTF = "libicq2000 (Unicode)";
char* cliTrillian  = "Trillian";
char* cliQip       = "QIP 200%c%c";
char* cliIM2       = "IM2";
char* cliSpamBot   = "Spam Bot";


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
	char szClientBuf[64];
	DWORD dwClientId = 0;
	WORD wVersion;
	WORD wTLVCount;
	WORD wWarningLevel;
	WORD wStatusFlags;
	WORD wStatus;
	BYTE nTCPFlag;
	DWORD dwOnlineSince;
	WORD wIdleTimer = 0;
	time_t tIdleTS = 0;


	// Unpack the sender's user ID
  if (!unpackUID(&buf, &wLen, &dwUIN, NULL)) return;

	// Syntax check
	if (wLen < 4)
		return;

	// Warning level?
	unpackWord(&buf, &wWarningLevel);
	wLen -= 2;

	// TLV count
	unpackWord(&buf, &wTLVCount);
	wLen -= 2;

	// Ignore status notification if the user is not already on our list
	if ((hContact = HContactFromUIN(dwUIN, 0)) == INVALID_HANDLE_VALUE)
	{
#ifdef _DEBUG
		NetLog_Server("Ignoring user online (%u)", dwUIN);
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

      // Is this a Miranda IM client?
      if (dwFT1 == 0xffffffff)
      {
        if (dwFT2 == 0xffffffff)
			  { // This is Gaim not Miranda
          szClient = "Gaim";
        }
        else if (!dwFT2 && wVersion == 7)
        { // This is WebICQ not Miranda
          szClient = "WebICQ";
        }
        else if (!dwFT2 && dwFT3 == 0x3B7248ED)
        { // And this is most probably Spam Bot
          szClient = cliSpamBot;
        }
        else 
        { // Yes this is most probably Miranda, get the version info
				  szClient = MirandaVersionToString(dwFT2, 0);
				  dwClientId = 1; // Miranda does not use Tick as msgId
			  }
      }
      else if ((dwFT1 & 0xFF7F0000) == 0x7D000000)
      { // This is probably an Licq client
        DWORD ver = dwFT1 & 0xFFFF;
        if (ver % 10){
          null_snprintf(szClientBuf, 64, cliLicqVerL, ver / 1000, (ver / 10) % 100, ver % 10);
        }
        else
        {
          null_snprintf(szClientBuf, 64, cliLicqVer, ver / 1000, (ver / 10) % 100);
        }
        if (dwFT1 & 0x00800000)
          strcat(szClientBuf, "/SSL");

        szClient = szClientBuf;
      }
      else if (dwFT1 == 0xffffff8f)
      {
        szClient = "StrICQ";
      }
      else if (dwFT1 == 0xffffff42)
      {
        szClient = "mICQ";
      }
      else if (dwFT1 == 0xffffffbe)
      {
        unsigned ver1 = (dwFT2>>24)&0xFF;
        unsigned ver2 = (dwFT2>>16)&0xFF;
        unsigned ver3 = (dwFT2>>8)&0xFF;
        
        if (ver3) 
          null_snprintf(szClientBuf, sizeof(szClientBuf), "Alicq %u.%u.%u", ver1, ver2, ver3);
        else  
          null_snprintf(szClientBuf, sizeof(szClientBuf), "Alicq %u.%u", ver1, ver2);
        szClient = szClientBuf;
      }
      else if (dwFT1 == 0xFFFFFF7F)
      {
        szClient = "&RQ";
      }
      else if (dwFT1 == 0xFFFFFFAB)
      {
        szClient = "YSM";
      }
      else if (dwFT1 == 0x04031980)
      {
        szClient = "vICQ";
      }
      else if ((dwFT1 == 0x3AA773EE) && (dwFT2 == 0x3AA66380))
      {
        szClient = cliLibicq2k;
      }
      else if (dwFT1 == 0x3B75AC09)
      {
        szClient = cliTrillian;
      }
      else if (dwFT1 == 0xFFFFFFFE && dwFT3 == 0xFFFFFFFE)
      {
        szClient = "mobicq/JIMM";
      }
      else if (dwFT1 == 0x3FF19BEB && dwFT3 == 0x3FF19BEB)
      {
        szClient = cliIM2;
      }
      else if (dwFT1 == dwFT2 && dwFT2 == dwFT3 && wVersion == 8)
      {
        DWORD tNow = getDWordFromChain(pChain, 0x03, 1);

        if ((dwFT1 < tNow + 3600) && (dwFT1 > (tNow - 3600)))
        {
          szClient = cliSpamBot;
        }
      }
		}
		else
		{
			// This client doesnt want DCs
			dwRealIP = 0;
			dwPort = 0;
			nTCPFlag = 0;
			wVersion = 0;
			dwDirectConnCookie = 0;
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
			if (pTLV && (pTLV->wLen >= 0x14))
			{
        DBVARIANT dbv;
        int dummy;
        int dwJob = 0;
        char szAvatar[MAX_PATH];
        int dwPaFormat;

        if (pTLV->wLen == 0x18 && pTLV->pData[3] == 0)
        { // icq probably set two avatars, get something from that
          memcpy(pTLV->pData, pTLV->pData+4, 0x14);
        }

        if (gbAvatarsEnabled && ICQGetContactSettingByte(NULL, "AvatarsAutoLoad", 1))
        { // check settings, should we request avatar immediatelly

          if (ICQGetContactSetting(hContact, "AvatarHash", &dbv))
          { // we not found old hash, i.e. get new avatar
            DBVARIANT dbvHashFile;

            if (!ICQGetContactSetting(hContact, "AvatarSaved", &dbvHashFile))
            { // check saved hash and file, if equal only store hash
              if ((dbvHashFile.cpbVal != 0x14) || memcmp(dbvHashFile.pbVal, pTLV->pData, 0x14))
              { // the hash is different, unlink contactphoto
                LinkContactPhotoToFile(hContact, NULL);
                dwJob = 1;
              }
              else
              {
                dwPaFormat = ICQGetContactSettingByte(hContact, "AvatarType", PA_FORMAT_UNKNOWN);

                GetFullAvatarFileName(dwUIN, dwPaFormat, szAvatar, MAX_PATH);
                if (access(szAvatar, 0) == 0)
                { // the file is there, link to contactphoto, save hash
                  NetLog_Server("Avatar is known, hash stored, linked to file.");

                  if (dummy = ICQWriteContactSettingBlob(hContact, "AvatarHash", pTLV->pData, 0x14))
                  {
                    NetLog_Server("Hash saving failed. Error: %d", dummy);
                  }
                  if (dwPaFormat != PA_FORMAT_UNKNOWN && dwPaFormat != PA_FORMAT_XML)
                    LinkContactPhotoToFile(hContact, szAvatar);
                  else  // the format is not supported unlink
                    LinkContactPhotoToFile(hContact, NULL);

                  ICQBroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, (LPARAM)NULL);
                }
                else // the file is lost, request avatar again
                  dwJob = 1;
              }
              DBFreeVariant(&dbvHashFile);
            }
            else
              dwJob = 1;
          }
          else
          { // we found hash check if it changed or not
            NetLog_Server("%s Hash: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", "Old", dbv.pbVal[0], dbv.pbVal[1], dbv.pbVal[2], dbv.pbVal[3], dbv.pbVal[4], dbv.pbVal[5], dbv.pbVal[6], dbv.pbVal[7], dbv.pbVal[8], dbv.pbVal[9], dbv.pbVal[10], dbv.pbVal[11], dbv.pbVal[12], dbv.pbVal[13], dbv.pbVal[14], dbv.pbVal[15], dbv.pbVal[16], dbv.pbVal[17], dbv.pbVal[18], dbv.pbVal[19]);
            if ((dbv.cpbVal != 0x14) || memcmp(dbv.pbVal, pTLV->pData, 0x14))
            { // the hash is different, request new avatar
              LinkContactPhotoToFile(hContact, NULL); // unlink photo
              dwJob = 1;
            }
            else
            { // the hash does not changed, so check if the file exists
              dwPaFormat = ICQGetContactSettingByte(hContact, "AvatarType", PA_FORMAT_UNKNOWN);
              if (dwPaFormat == PA_FORMAT_UNKNOWN)
              { // we do not know the format, get avatar again
                dwJob = 1;
              }
              else
              {
                GetFullAvatarFileName(dwUIN, dwPaFormat, szAvatar, MAX_PATH);
                if (access(szAvatar, 0) == 0)
                { // the file exists, so try to update photo setting
                  if (dwPaFormat != PA_FORMAT_XML && dwPaFormat != PA_FORMAT_UNKNOWN)
                  {
                    LinkContactPhotoToFile(hContact, szAvatar);
                  }
                }
                else
                { // the file was lost, get it again
                  dwJob = 1;
                }
              }
            }
            DBFreeVariant(&dbv);
          }

          if (dwJob)
          {
            NetLog_Server("%s Hash: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", "New", pTLV->pData[0], pTLV->pData[1], pTLV->pData[2], pTLV->pData[3], pTLV->pData[4], pTLV->pData[5], pTLV->pData[6], pTLV->pData[7], pTLV->pData[8], pTLV->pData[9], pTLV->pData[10], pTLV->pData[11], pTLV->pData[12], pTLV->pData[13], pTLV->pData[14], pTLV->pData[15], pTLV->pData[16], pTLV->pData[17], pTLV->pData[18], pTLV->pData[19]);
            ICQBroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, (LPARAM)NULL);

            NetLog_Server("User has Avatar, new hash stored.");

            if (dummy = ICQWriteContactSettingBlob(hContact, "AvatarHash", pTLV->pData, 0x14))
            {
              NetLog_Server("Hash saving failed. Error: %d", dummy);
            }

            GetAvatarFileName(dwUIN, szAvatar, MAX_PATH);
            GetAvatarData(hContact, dwUIN, pTLV->pData, 0x14 /*pTLV->wLen*/, szAvatar);
            // avatar request sent or added to queue
          }
          else
          {
            NetLog_Server("User has Avatar.");
          }
        }
        else if (gbAvatarsEnabled)
        { // we should not request the avatar, so eventually save the hash and go on

          if (ICQGetContactSetting(hContact, "AvatarHash", &dbv))
          { // we not found old hash, i.e. store avatar hash
            DBVARIANT dbvHashFile;

            if (!ICQGetContactSetting(hContact, "AvatarSaved", &dbvHashFile))
            { // check save hash and file, if equal only store hash
              if ((dbvHashFile.cpbVal != 0x14) || memcmp(dbvHashFile.pbVal, pTLV->pData, 0x14))
              { // the hash is different, unlink contactphoto
                LinkContactPhotoToFile(hContact, NULL);
              }
              else
              {
                dwPaFormat = ICQGetContactSettingByte(hContact, "AvatarType", PA_FORMAT_UNKNOWN);

                GetFullAvatarFileName(dwUIN, dwPaFormat, szAvatar, MAX_PATH);
                if (access(szAvatar, 0) == 0)
                { // the file is there, link to contactphoto
                  if (dwPaFormat != PA_FORMAT_UNKNOWN && dwPaFormat != PA_FORMAT_XML)
                    LinkContactPhotoToFile(hContact, szAvatar);
                  else  // the format is not supported unlink
                    LinkContactPhotoToFile(hContact, NULL);
                }
              }
              DBFreeVariant(&dbvHashFile);
            }
            dwJob = 1;
          }
          else
          { // we found hash check if it changed or not
            if ((dbv.cpbVal != 0x14) || memcmp(dbv.pbVal, pTLV->pData, 0x14))
            { // the hash is different, store new avatar hash
              dwJob = 1;
            }
            DBFreeVariant(&dbv);
          }

          if (dwJob)
          {
            ICQBroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, (LPARAM)NULL);

            NetLog_Server("User has Avatar, hash stored.");

            if (dummy = ICQWriteContactSettingBlob(hContact, "AvatarHash", pTLV->pData, 0x14))
            {
              NetLog_Server("Hash saving failed. Error: %d", dummy);
            }
          }
          else
          {
            NetLog_Server("User has Avatar.");
          }
        }
			}
      else if (wOldStatus == ID_STATUS_OFFLINE)
      { // if user were offline, and now hash not found, clear the hash
      //  DBDeleteContactSetting(hContact, gpszICQProtoName, "AvatarHash"); // TODO: need more testing
        // TODO: configure, here should be removed also the mtooltip link if it is ours - in procedure
      }

			// Update the contact's capabilities
			if (wOldStatus == ID_STATUS_OFFLINE)
			{
				// Delete the capabilities we saved the last time this contact came online
				ClearAllContactCapabilities(hContact);

				// Get Capability Info TLV
				pTLV = getTLV(pChain, 0x0D, 1);

				if (pTLV && (pTLV->wLen >= 16))
				{
          capstr* capId;

					AddCapabilitiesFromBuffer(hContact, pTLV->pData, pTLV->wLen);

          // handle Xtraz status
          handleXStatusCaps(hContact, pTLV->pData, pTLV->wLen);

          // check capabilities for client identification
          if (MatchCap(pTLV->pData, pTLV->wLen, &capTrillian, 0x10) || MatchCap(pTLV->pData, pTLV->wLen, &capTrilCrypt, 0x10))
          { // this is Trillian, check for new version
            if (MatchCap(pTLV->pData, pTLV->wLen, &capRichText, 0x10))
              szClient = "Trillian v3";
            else
              szClient = cliTrillian;
          }
          else if ((capId = MatchCap(pTLV->pData, pTLV->wLen, &capSimOld, 0xF)) && ((*capId)[0xF] != 0x92 && (*capId)[0xF] >= 0x20 || (*capId)[0xF] == 0))
          {
            int hiVer = (((*capId)[0xF]) >> 6) - 1;
            unsigned loVer = (*capId)[0xF] & 0x1F;
            if ((hiVer < 0) || ((hiVer == 0) && (loVer == 0))) 
              szClient = "Kopete";
            else
            {
              null_snprintf(szClientBuf, sizeof(szClientBuf), "SIM %u.%u", (unsigned)hiVer, loVer);
              szClient = szClientBuf;
            }
          }
          else if (capId = MatchCap(pTLV->pData, pTLV->wLen, &capSim, 0xC))
          {
            unsigned ver1 = (*capId)[0xC];
            unsigned ver2 = (*capId)[0xD];
            unsigned ver3 = (*capId)[0xE];
            if (ver3) 
              null_snprintf(szClientBuf, sizeof(szClientBuf), "SIM %u.%u.%u", ver1, ver2, ver3);
            else  
              null_snprintf(szClientBuf, sizeof(szClientBuf), "SIM %u.%u", ver1, ver2);
            if ((*capId)[0xF] & 0x80) 
              strcat(szClientBuf,"/Win32");
            else if ((*capId)[0xF] & 0x40) 
              strcat(szClientBuf,"/MacOS X");
            szClient = szClientBuf;
          }
          else if (capId = MatchCap(pTLV->pData, pTLV->wLen, &capLicq, 0xC))
          {
            unsigned ver1 = (*capId)[0xC];
            unsigned ver2 = (*capId)[0xD] % 100;
            unsigned ver3 = (*capId)[0xE];
            if (ver3) 
              null_snprintf(szClientBuf, sizeof(szClientBuf), cliLicqVerL, ver1, ver2, ver3);
            else  
              null_snprintf(szClientBuf, sizeof(szClientBuf), cliLicqVer, ver1, ver2);
            if ((*capId)[0xF]) 
              strcat(szClientBuf,"/SSL");
            szClient = szClientBuf;
          }
          else if (capId = MatchCap(pTLV->pData, pTLV->wLen, &capMirandaIm, 8))
          { // new Miranda Signature
            DWORD iver = (*capId)[0xC] << 0x18 | (*capId)[0xD] << 0x10 | (*capId)[0xE] << 8 | (*capId)[0xF];
            DWORD mver = (*capId)[0x8] << 0x18 | (*capId)[0x9] << 0x10 | (*capId)[0xA] << 8 | (*capId)[0xB];

            szClient = MirandaVersionToString(iver, mver);
            dwClientId = 1; // Miranda does not use Tick as msgId
          }
          else if (capId = MatchCap(pTLV->pData, pTLV->wLen, &capKopete, 0xC))
          {
            unsigned ver1 = (*capId)[0xC];
            unsigned ver2 = (*capId)[0xD];
            unsigned ver3 = (*capId)[0xE];
            unsigned ver4 = (*capId)[0xF];
            if (ver4) 
              null_snprintf(szClientBuf, sizeof(szClientBuf), "Kopete %u.%u.%u.%u", ver1, ver2, ver3, ver4);
            else if (ver3) 
              null_snprintf(szClientBuf, sizeof(szClientBuf), "Kopete %u.%u.%u", ver1, ver2, ver3);
            else
              null_snprintf(szClientBuf, sizeof(szClientBuf), "Kopete %u.%u", ver1, ver2);
            szClient = szClientBuf;
          }
          else if (capId = MatchCap(pTLV->pData, pTLV->wLen, &capmIcq, 0xC))
          { 
            unsigned ver1 = (*capId)[0xC];
            unsigned ver2 = (*capId)[0xD];
            unsigned ver3 = (*capId)[0xE];
            unsigned ver4 = (*capId)[0xF];
            if (ver4) 
              null_snprintf(szClientBuf, sizeof(szClientBuf), "mICQ %u.%u.%u.%u", ver1, ver2, ver3, ver4);
            else if (ver3) 
              null_snprintf(szClientBuf, sizeof(szClientBuf), "mICQ %u.%u.%u", ver1, ver2, ver3);
            else
              null_snprintf(szClientBuf, sizeof(szClientBuf), "mICQ %u.%u", ver1, ver2);
            szClient = szClientBuf;
          }
          else if (MatchCap(pTLV->pData, pTLV->wLen, &capIm2, 0x10))
          {
            szClient = cliIM2;
          }
          else if (capId = MatchCap(pTLV->pData, pTLV->wLen, &capAndRQ, 9))
          {
            unsigned ver1 = (*capId)[0xC];
            unsigned ver2 = (*capId)[0xB];
            unsigned ver3 = (*capId)[0xA];
            unsigned ver4 = (*capId)[9];
            if (ver4) 
              null_snprintf(szClientBuf, sizeof(szClientBuf), "&RQ %u.%u.%u.%u", ver1, ver2, ver3, ver4);
            else if (ver3) 
              null_snprintf(szClientBuf, sizeof(szClientBuf), "&RQ %u.%u.%u", ver1, ver2, ver3);
            else
              null_snprintf(szClientBuf, sizeof(szClientBuf), "&RQ %u.%u", ver1, ver2);
            szClient = szClientBuf;
          }
          else if (capId = MatchCap(pTLV->pData, pTLV->wLen, &capQip, 0xE))
          {
            char v1 = (*capId)[0xE];
            char v2 = (*capId)[0xF];
            null_snprintf(szClientBuf, sizeof(szClientBuf), cliQip, v1, v2);
            szClient = szClientBuf;
          }
          else if (MatchCap(pTLV->pData, pTLV->wLen, &capMacIcq, 0x10))
          {
            szClient = "ICQ for Mac";
          }
          else if (MatchCap(pTLV->pData, pTLV->wLen, &capAimIcon, 0x10))
          { // this is what I really hate, but as it seems to me, there is no other way to detect libgaim
            szClient = "libgaim";
          }
          else if (szClient == cliLibicq2k)
          { // try to determine which client is behind libicq2000
            if (MatchCap(pTLV->pData, pTLV->wLen, &capRichText, 0x10))
              szClient = cliCentericq; // centericq added rtf capability to libicq2000
            else if (CheckContactCapabilities(hContact, CAPF_UTF))
              szClient = cliLibicqUTF; // IcyJuice added unicode capability to libicq2000
            // others - like jabber transport uses unmodified library, thus cannot be detected
          }
          else if (szClient == NULL) // HERE ENDS THE SIGNATURE DETECTION, after this only feature default will be detected
          {
            if (wVersion == 8 && (MatchCap(pTLV->pData, pTLV->wLen, &capStr20012, 0x10) || CheckContactCapabilities(hContact, CAPF_SRV_RELAY)))
            { // try to determine 2001-2003 versions
              if (MatchCap(pTLV->pData, pTLV->wLen, &capIs2001, 0x10))
                if (!dwFT1 && !dwFT2 && !dwFT3)
                  szClient = "ICQ for Pocket PC";
                else
                  szClient = "ICQ 2001";
              else if (MatchCap(pTLV->pData, pTLV->wLen, &capIs2002, 0x10))
                szClient = "ICQ 2002";
              else if (CheckContactCapabilities(hContact, CAPF_SRV_RELAY || CAPF_UTF) && MatchCap(pTLV->pData, pTLV->wLen, &capRichText, 0x10))
                szClient = "ICQ 2002/2003a";
            }
            else if (wVersion == 9)
            { // try to determine lite versions
              if (MatchCap(pTLV->pData, pTLV->wLen, &capXtraz, 0x10))
                if (MatchCap(pTLV->pData, pTLV->wLen, &capIcq5Extra, 0x10))
                  szClient = "icq5";
                else
                  szClient = "ICQ Lite v4";
            }
            else if (wVersion == 7)
              if (MatchCap(pTLV->pData, pTLV->wLen, &capRichText, 0x10))
                szClient = "GnomeICU"; // this is an exception
              else if (CheckContactCapabilities(hContact, CAPF_SRV_RELAY))
                szClient = "ICQ 2000";
              else if (CheckContactCapabilities(hContact, CAPF_TYPING))
                szClient = "Icq2Go! (Java)";
              else 
                szClient = "Icq2Go!";
            else if (wVersion == 0xA)
              if (!MatchCap(pTLV->pData, pTLV->wLen, &capRichText, 0x10) && !CheckContactCapabilities(hContact, CAPF_UTF))
              { // this is bad, but we must do it - try to detect QNext
                ClearContactCapabilities(hContact, CAPF_SRV_RELAY);
                NetLog_Server("Forcing simple messages (QNext client).");
                szClient = "QNext";
              }
          }
				}
				else
				{
					NetLog_Server("No capability info TLV");
          // clear XStatus
          handleXStatusCaps(hContact, NULL, 0);
				}

#ifdef _DEBUG
				if (CheckContactCapabilities(hContact, CAPF_SRV_RELAY))
					NetLog_Server("Supports advanced messages");
				else
					NetLog_Server("Does NOT support advanced messages");
#endif

				if (wVersion < 8)
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

  if (szClient == 0)
  {
    NetLog_Server("No client identification, put default ICQ client for protocol.");

    switch (wVersion)
    {  // client detection failed, provide default clients
      case 1: 
        szClient = "ICQ 1.x";
        break;
      case 2: 
        szClient = "ICQ 2.x";
        break;
      case 4:
        szClient = "ICQ98";
        break;
      case 6:
        szClient = "ICQ99";
        break;
      case 7:
        szClient = "ICQ 2000/Icq2Go";
        break;
      case 8: 
        szClient = "ICQ 2001-2003a";
        break;
      case 9: 
        szClient = "ICQ Lite";
        break;
      case 0xA:
        szClient = "ICQ 2003b";
    }
  }
  else
  {
    if (szClient != (char*)-1)
      NetLog_Server("Client identified as %s", szClient);
  }
  if (szClient == cliSpamBot)
  {
    // TODO: handle
  }

	// Save contacts details in database
  if (hContact != NULL)
  {
    if (szClient == 0) szClient = ""; // if no detection, set uknown
    ICQWriteContactSettingDword(hContact,  "ClientID",     dwClientId);

		ICQWriteContactSettingDword(hContact,  "LogonTS",      dwOnlineSince);
		ICQWriteContactSettingDword(hContact,  "IP",           dwIP);
		ICQWriteContactSettingDword(hContact,  "RealIP",       dwRealIP);
		ICQWriteContactSettingDword(hContact,  "DirectCookie", dwDirectConnCookie);
    ICQWriteContactSettingByte(hContact,   "DCType",       (BYTE)nTCPFlag);
		ICQWriteContactSettingWord(hContact,   "UserPort",     (WORD)(dwPort & 0xffff));
		ICQWriteContactSettingWord(hContact,   "Version",      wVersion);
		if (szClient != (char*)-1) ICQWriteContactSettingString(hContact, "MirVer", szClient);
		ICQWriteContactSettingWord(hContact,   "Status",       (WORD)IcqStatusToMiranda(wStatus));
		ICQWriteContactSettingDword(hContact,  "IdleTS",       tIdleTS);

		// Update info?
		if ((time(NULL) - ICQGetContactSettingDword(hContact, "InfoTS", 0)) > UPDATE_THRESHOLD)
			icq_QueueUser(hContact);
	}
	else
	{
		dwLocalDirectConnCookie = dwDirectConnCookie;
	}

	// And a small log notice...
	NetLog_Server("%u changed status to %s (v%d).",
		dwUIN, MirandaStatusToString(IcqStatusToMiranda(wStatus)), wVersion);
}



static void handleUserOffline(BYTE *buf, WORD wLen)
{
	HANDLE hContact;
	DWORD dwUIN;

	// Unpack the sender's user ID
  if (!unpackUID(&buf, &wLen, &dwUIN, NULL)) return;

	hContact = HContactFromUIN(dwUIN, 0);

	// Skip contacts that are not already on our list
	if (hContact != INVALID_HANDLE_VALUE)
	{
		NetLog_Server("%u went offline.", dwUIN);
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
		NetLog_Server("Error: Malformed BuddyRepyl");
	}
}
