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
//  Handles packets from Buddy family
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"



static void handleUserOffline(BYTE* buf, WORD wPackLen);
static void handleUserOnline(BYTE* buf, WORD wPackLen);
static void handleReplyBuddy(BYTE* buf, WORD wPackLen);

extern DWORD dwLocalDirectConnCookie;
extern HANDLE ghServerNetlibUser;
extern char gpszICQProtoName[MAX_PATH];

extern byte gbAvatarsEnabled;


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

	default:
		Netlib_Logf(ghServerNetlibUser, "Warning: Ignoring SNAC(x03,x%02x) - Unknown SNAC (Flags: %u, Ref: %u", pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
		break;

	}

}

typedef unsigned char capstr[0x10];

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

const capstr capTrillian  = {0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34, 0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x09};
const capstr capTrilNew   = {0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34, 0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x92};
const capstr capTrilCrypt = {0xf2, 0xe7, 0xc7, 0xf4, 0xfe, 0xad, 0x4d, 0xfb, 0xb2, 0x35, 0x36, 0x79, 0x8b, 0xdf, 0x00, 0x00};
const capstr capSim       = {'S', 'I', 'M', ' ', 'c', 'l', 'i', 'e', 'n', 't', ' ', ' ', 0, 0, 0, 0};
const capstr capSimOld    = {0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34, 0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x00};
const capstr capLicq      = {'L', 'i', 'c', 'q', ' ', 'c', 'l', 'i', 'e', 'n', 't', ' ', 0, 0, 0, 0};
const capstr capKopete    = {'K', 'o', 'p', 'e', 't', 'e', ' ', 'I', 'C', 'Q', ' ', ' ', 0, 0, 0, 0};
const capstr capmIcq      = {'m', 'I', 'C', 'Q', ' ', 0xA9, ' ', 'R', '.', 'K', '.', ' ', 0, 0, 0, 0};
const capstr capAndRQ     = {'&', 'R', 'Q', 'i', 'n', 's', 'i', 'd', 'e', 0, 0, 0, 0, 0, 0, 0};
const capstr capIm2       = {0x74, 0xED, 0xC3, 0x36, 0x44, 0xDF, 0x48, 0x5B, 0x8B, 0x1C, 0x67, 0x1A, 0x1F, 0x86, 0x09, 0x9F};
const capstr capMacIcq    = {0xdd, 0x16, 0xf2, 0x02, 0x84, 0xe6, 0x11, 0xd4, 0x90, 0xdb, 0x00, 0x10, 0x4b, 0x9b, 0x4b, 0x7d};
const capstr capRichText  = {0x97, 0xb1, 0x27, 0x51, 0x24, 0x3c, 0x43, 0x34, 0xad, 0x22, 0xd6, 0xab, 0xf7, 0x3f, 0x14, 0x92};
const capstr capIs2001    = {0x2e, 0x7a, 0x64, 0x75, 0xfa, 0xdf, 0x4d, 0xc8, 0x88, 0x6f, 0xea, 0x35, 0x95, 0xfd, 0xb6, 0xdf};
const capstr capIs2002    = {0x10, 0xcf, 0x40, 0xd1, 0x4c, 0x7f, 0x11, 0xd1, 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};
const capstr capStr20012  = {0xa0, 0xe9, 0x3f, 0x37, 0x4f, 0xe9, 0xd3, 0x11, 0xbc, 0xd2, 0x00, 0x04, 0xac, 0x96, 0xdd, 0x96};
const capstr capXtraz     = {0x1A, 0x09, 0x3C, 0x6C, 0xD7, 0xFD, 0x4E, 0xC5, 0x9D, 0x51, 0xA6, 0x47, 0x4E, 0x34, 0xF5, 0xA0};
const capstr capIcq5Extra = {0x09, 0x46, 0x13, 0x43, 0x4C, 0x7F, 0x11, 0xD1, 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}; // CAP_AIM_SENDFILE
const capstr capAimIcon   = {0x09, 0x46, 0x13, 0x46, 0x4c, 0x7f, 0x11, 0xd1, 0x82, 0x22, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}; // CAP_AIM_BUDDYICON


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
//	DWORD dwMirVer = 0;
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
	{
		BYTE nUIDLen;
		char* pszUID;

		unpackByte(&buf, &nUIDLen);
		wLen -= 1;

		if (nUIDLen > wLen)
		{
			Netlib_Logf(ghServerNetlibUser, "SNAC(3.B) Invalid UIN 1");
			return;
		}

		if (!(pszUID = malloc(nUIDLen+1)))
			return; // Memory failure
		unpackString(&buf, pszUID, nUIDLen);
		wLen -= nUIDLen;
		pszUID[nUIDLen] = '\0';

		if (!IsStringUIN(pszUID))
		{
			Netlib_Logf(ghServerNetlibUser, "SNAC(3.B) Invalid UIN 2");
			SAFE_FREE(&pszUID);
			return;
		}

		dwUIN = atoi(pszUID);
		SAFE_FREE(&pszUID);
	}


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
		Netlib_Logf(ghServerNetlibUser, "Ignoring user online (%u)", dwUIN);
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
        } else 
        { // Yes this is most probably Miranda, get the version info
				  szClient = MirandaVersionToString(dwFT2);
				  dwClientId = 1; // Miranda does not use Tick as msgId
			  }
      }
      else if ((dwFT1 & 0xFF7F0000) == 0x7D800000)
      { // This is probably an Licq client
        DWORD ver = dwFT1 & 0xFFFF;
        if (ver % 10){
          _snprintf(szClientBuf, 64, "Licq %u.%u.%u", ver / 1000, (ver / 10) % 100, ver % 10);
        }
        else
        {
          _snprintf(szClientBuf, 64, "Licq %u.%u", ver / 1000, (ver / 10) % 100);
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
        unsigned ver1 = (dwFT1>>24)&0xFF;
        unsigned ver2 = (dwFT1>>16)&0xFF;
        unsigned ver3 = (dwFT1>>8)&0xFF;
        
        if (ver3) 
          _snprintf(szClientBuf, sizeof(szClientBuf), "Alicq %u.%u.%u", ver1, ver2, ver3);
        else  
          _snprintf(szClientBuf, sizeof(szClientBuf), "Alicq %u.%u", ver1, ver2);
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
        szClient = "libicq2000";
      }
      else if (dwFT1 == 0xFFFFFFFE && dwFT3 == 0xFFFFFFFE)
      {
        szClient = "mobicq/JIMM";
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
		Netlib_Logf(ghServerNetlibUser, "Flags are %x", wStatusFlags);
		Netlib_Logf(ghServerNetlibUser, "Status is %x", wStatus);
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
			Netlib_Logf(ghServerNetlibUser, "Idle timer is %u.", wIdleTimer);
		Netlib_Logf(ghServerNetlibUser, "Online since %s", asctime(localtime(&dwOnlineSince)));
#endif


		// Check client capabilities
		if (hContact != NULL)
		{

			WORD wOldStatus;

			wOldStatus = DBGetContactSettingWord(hContact, gpszICQProtoName, "Status", ID_STATUS_OFFLINE);

			// Get Avatar Hash TLV
			pTLV = getTLV(pChain, 0x1D, 1);
			if (pTLV && (pTLV->wLen >= 0x14))
			{
        DBCONTACTWRITESETTING cws;
        DBVARIANT dbv;
        int dummy;
        int dwJob = 0;
        char szAvatar[256];
        int dwPaFormat;

        if (gbAvatarsEnabled && DBGetContactSettingByte(NULL, gpszICQProtoName, "AvatarsAutoLoad", 1))
        { // check settings, should we request avatar immediatelly

          if (DBGetContactSetting(hContact, gpszICQProtoName, "AvatarHash", &dbv))
          { // we not found old hash, i.e. get new avatar
            dwJob = 1;
          }
          else
          { // we found hash check if it changed or not
            Netlib_Logf(ghServerNetlibUser, "Old Hash: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", dbv.pbVal[0], dbv.pbVal[1], dbv.pbVal[2], dbv.pbVal[3], dbv.pbVal[4], dbv.pbVal[5], dbv.pbVal[6], dbv.pbVal[7], dbv.pbVal[8], dbv.pbVal[9], dbv.pbVal[10], dbv.pbVal[11], dbv.pbVal[12], dbv.pbVal[13], dbv.pbVal[14], dbv.pbVal[15], dbv.pbVal[16], dbv.pbVal[17], dbv.pbVal[18], dbv.pbVal[19], dbv.pbVal[20]);
            if ((dbv.cpbVal != 0x14) || memcmp(dbv.pbVal, pTLV->pData, 0x14))
            { // the hash is different, request new avatar
              dwJob = 1;
            }
            else
            { // the hash does not changed, so check if the file exists
              if (pTLV->pData[1] == 8)
                dwPaFormat = PA_FORMAT_XML;
              else 
                dwPaFormat = PA_FORMAT_JPEG;
              GetAvatarFileName(dwUIN, dwPaFormat, szAvatar, 255);
              if (access(szAvatar, 0) == 0)
              { // the file exists, so try to update photo setting
                if (dwPaFormat == PA_FORMAT_JPEG)
                {
                  DBDeleteContactSetting(hContact, "ContactPhoto", "File"); // delete that setting
                  DBDeleteContactSetting(hContact, "ContactPhoto", "Link");
                  if (DBWriteContactSettingString(hContact, "ContactPhoto", "File", szAvatar))
                    Netlib_Logf(ghServerNetlibUser, "Avatar file could not be linked to mToolTip.");
                }
              }
              else
              { // the file was lost, get it again
                dwJob = 1;
              }
            }
            DBFreeVariant(&dbv);
          }

          if (dwJob)
          {
            Netlib_Logf(ghServerNetlibUser, "New Hash: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", pTLV->pData[0], pTLV->pData[1], pTLV->pData[2], pTLV->pData[3], pTLV->pData[4], pTLV->pData[5], pTLV->pData[6], pTLV->pData[7], pTLV->pData[8], pTLV->pData[9], pTLV->pData[10], pTLV->pData[11], pTLV->pData[12], pTLV->pData[13], pTLV->pData[14], pTLV->pData[15], pTLV->pData[16], pTLV->pData[17], pTLV->pData[18], pTLV->pData[19], pTLV->pData[20]);
            ProtoBroadcastAck(gpszICQProtoName, hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, (LPARAM)NULL);

            Netlib_Logf(ghServerNetlibUser, "User has Avatar, new hash stored.");

            cws.szModule = gpszICQProtoName;
            cws.szSetting = "AvatarHash";
            cws.value.type = DBVT_BLOB;
            cws.value.pbVal = pTLV->pData;
            cws.value.cpbVal = 0x14; //pTLV->wLen; // only 20 bytes useful
				    dummy = CallService(MS_DB_CONTACT_WRITESETTING, (WPARAM)hContact, (LPARAM)&cws);
            if (dummy)
            {
              Netlib_Logf(ghServerNetlibUser, "Hash saving failed. Error: %d", dummy);
            }

            if (pTLV->pData[1] == 8)
              dwPaFormat = PA_FORMAT_XML;
            else 
              dwPaFormat = PA_FORMAT_JPEG;
            GetAvatarFileName(dwUIN, dwPaFormat, szAvatar, 255);
            if (GetAvatarData(hContact, dwUIN, pTLV->pData, 0x14 /*pTLV->wLen*/, szAvatar)) 
            { // avatar request sent or added to queue
              if (dwPaFormat == PA_FORMAT_JPEG)
              { // TODO: move to separate function for linking avatar file to mToolTip and others
                DBDeleteContactSetting(hContact, "ContactPhoto", "File"); // delete that setting
                DBDeleteContactSetting(hContact, "ContactPhoto", "Link");
                if (DBWriteContactSettingString(hContact, "ContactPhoto", "File", szAvatar))
                  Netlib_Logf(ghServerNetlibUser, "Avatar file could not be linked to mToolTip.");
              }
            } 
          }
          else
          {
            Netlib_Logf(ghServerNetlibUser, "User has Avatar.");
          }
        }
        else if (gbAvatarsEnabled)
        { // we should not request the avatar, so eventually save the hash and go on

          if (DBGetContactSetting(hContact, gpszICQProtoName, "AvatarHash", &dbv))
          { // we not found old hash, i.e. store avatar hash
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
            ProtoBroadcastAck(gpszICQProtoName, hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, NULL, (LPARAM)NULL);

            Netlib_Logf(ghServerNetlibUser, "User has Avatar, hash stored.");

            cws.szModule = gpszICQProtoName;
            cws.szSetting = "AvatarHash";
            cws.value.type = DBVT_BLOB;
            cws.value.pbVal = pTLV->pData;
            cws.value.cpbVal = 0x14; //pTLV->wLen; // only 20 bytes useful
				    dummy = CallService(MS_DB_CONTACT_WRITESETTING, (WPARAM)hContact, (LPARAM)&cws);
            if (dummy)
            {
              Netlib_Logf(ghServerNetlibUser, "Hash saving failed. Error: %d", dummy);
            }
          }
          else
          {
            Netlib_Logf(ghServerNetlibUser, "User has Avatar.");
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
          // check capabilities for client identification
          if (MatchCap(pTLV->pData, pTLV->wLen, &capTrillian, 0x10) || MatchCap(pTLV->pData, pTLV->wLen, &capTrilCrypt, 0x10))
          { // this is Trillian, check for new version
            if (MatchCap(pTLV->pData, pTLV->wLen, &capTrilNew, 0x10))
              szClient = "Trillian v3";
            else
              szClient = "Trillian";
          }
          else if ((capId = MatchCap(pTLV->pData, pTLV->wLen, &capSimOld, 0xF)) && ((*capId)[0xF] != 0x92 && (*capId)[0xF] >= 0x20 || (*capId)[0xF] == 0))
          {
            int hiVer = (((*capId)[0xF]) >> 6) - 1;
            unsigned loVer = (*capId)[0xF] & 0x1F;
            if ((hiVer < 0) || ((hiVer == 0) && (loVer == 0))) 
              szClient = "Kopete";
            else
            {
              _snprintf(szClientBuf, sizeof(szClientBuf), "SIM %u.%u", (unsigned)hiVer, loVer);
              szClient = szClientBuf;
            }
          }
          else if (capId = MatchCap(pTLV->pData, pTLV->wLen, &capSim, 0xC))
          {
            unsigned ver1 = (*capId)[0xC];
            unsigned ver2 = (*capId)[0xD];
            unsigned ver3 = (*capId)[0xE];
            if (ver3) 
              _snprintf(szClientBuf, sizeof(szClientBuf), "SIM %u.%u.%u", ver1, ver2, ver3);
            else  
              _snprintf(szClientBuf, sizeof(szClientBuf), "SIM %u.%u", ver1, ver2);
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
              _snprintf(szClientBuf, sizeof(szClientBuf), "Licq %u.%u.%u", ver1, ver2, ver3);
            else  
              _snprintf(szClientBuf, sizeof(szClientBuf), "Licq %u.%u", ver1, ver2);
            if ((*capId)[0xF]) 
              strcat(szClientBuf,"/SSL");
            szClient = szClientBuf;
          }
          else if (capId = MatchCap(pTLV->pData, pTLV->wLen, &capKopete, 0xC))
          {
            unsigned ver1 = (*capId)[0xC];
            unsigned ver2 = (*capId)[0xD];
            unsigned ver3 = (*capId)[0xE];
            unsigned ver4 = (*capId)[0xF];
            if (ver4) 
              _snprintf(szClientBuf, sizeof(szClientBuf), "Kopete %u.%u.%u.%u", ver1, ver2, ver3, ver4);
            else if (ver3) 
              _snprintf(szClientBuf, sizeof(szClientBuf), "Kopete %u.%u.%u", ver1, ver2, ver3);
            else
              _snprintf(szClientBuf, sizeof(szClientBuf), "Kopete %u.%u", ver1, ver2);
            szClient = szClientBuf;
          }
          else if (capId = MatchCap(pTLV->pData, pTLV->wLen, &capmIcq, 0xC))
          { 
            unsigned ver1 = (*capId)[0xC];
            unsigned ver2 = (*capId)[0xD];
            unsigned ver3 = (*capId)[0xE];
            unsigned ver4 = (*capId)[0xF];
            if (ver4) 
              _snprintf(szClientBuf, sizeof(szClientBuf), "mICQ %u.%u.%u.%u", ver1, ver2, ver3, ver4);
            else if (ver3) 
              _snprintf(szClientBuf, sizeof(szClientBuf), "mICQ %u.%u.%u", ver1, ver2, ver3);
            else
              _snprintf(szClientBuf, sizeof(szClientBuf), "mICQ %u.%u", ver1, ver2);
            szClient = szClientBuf;
          }
          else if (MatchCap(pTLV->pData, pTLV->wLen, &capIm2, 0x10))
          {
            szClient = "IM2";
          }
          else if (capId = MatchCap(pTLV->pData, pTLV->wLen, &capAndRQ, 9))
          {
            unsigned ver1 = (*capId)[0xC];
            unsigned ver2 = (*capId)[0xB];
            unsigned ver3 = (*capId)[0xA];
            unsigned ver4 = (*capId)[9];
            if (ver4) 
              _snprintf(szClientBuf, sizeof(szClientBuf), "&RQ %u.%u.%u.%u", ver1, ver2, ver3, ver4);
            else if (ver3) 
              _snprintf(szClientBuf, sizeof(szClientBuf), "&RQ %u.%u.%u", ver1, ver2, ver3);
            else
              _snprintf(szClientBuf, sizeof(szClientBuf), "&RQ %u.%u", ver1, ver2);
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
          else // HERE ENDS THE SIGNATURE DETECTION, after this only feature default will be detected
          {
            if (wVersion == 8 && MatchCap(pTLV->pData, pTLV->wLen, &capStr20012, 0x10))
            { // try to determine 2001-2003 versions
              if (MatchCap(pTLV->pData, pTLV->wLen, &capIs2001, 0x10))
                if (!dwFT1 && !dwFT2 && !dwFT3)
                  szClient = "ICQ for Pocket PC";
                else
                  szClient = "ICQ 2001";
              else if (MatchCap(pTLV->pData, pTLV->wLen, &capIs2002, 0x10))
                szClient = "ICQ 2002";
              else if (CheckContactCapabilities(hContact, CAPF_SRV_RELAY || CAPF_UTF) && MatchCap(pTLV->pData, pTLV->wLen, &capRichText, 0x10))
                szClient = "ICQ 2003a";
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
          }
				}
				else
				{
					Netlib_Logf(ghServerNetlibUser, "No capability info TLV");
				}

#ifdef _DEBUG
				if (CheckContactCapabilities(hContact, CAPF_SRV_RELAY))
					Netlib_Logf(ghServerNetlibUser, "Supports advanced messages");
				else
					Netlib_Logf(ghServerNetlibUser, "Does NOT support advanced messages");
#endif

				if (wVersion < 8)
				{
					ClearContactCapabilities(hContact, CAPF_SRV_RELAY);
					Netlib_Logf(ghServerNetlibUser, "Forcing simple messages due to compability issues");
				}

			}
      else
        if (szClient == 0) szClient = (char*)-1; // we don't want to client be overwritten if no capabilities received

		}


		// Free TLV chain
		disposeChain(&pChain);
	}

  if (szClient == 0)
  {
    Netlib_Logf(ghServerNetlibUser, "No client identification, put default ICQ client for protocol.");

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
      Netlib_Logf(ghServerNetlibUser, "Client identified as %s", szClient);
  }

	// Save contacts details in database
  if (hContact != NULL)
  {
    if (szClient == 0) szClient = ""; // if no detection, set uknown
    DBWriteContactSettingDword(hContact,  gpszICQProtoName, "ClientID",     dwClientId);

		DBWriteContactSettingDword(hContact,  gpszICQProtoName, "LogonTS",      dwOnlineSince);
		DBWriteContactSettingDword(hContact,  gpszICQProtoName, "IP",           dwIP);
		DBWriteContactSettingDword(hContact,  gpszICQProtoName, "RealIP",       dwRealIP);
		DBWriteContactSettingDword(hContact,  gpszICQProtoName, "DirectCookie", dwDirectConnCookie);
		DBWriteContactSettingWord(hContact,   gpszICQProtoName, "UserPort",     (WORD)(dwPort & 0xffff));
		DBWriteContactSettingWord(hContact,   gpszICQProtoName, "Version",      wVersion);
		if (szClient != (char*)-1) DBWriteContactSettingString(hContact, gpszICQProtoName, "MirVer",       szClient);
		DBWriteContactSettingWord(hContact,   gpszICQProtoName, "Status",       (WORD)IcqStatusToMiranda(wStatus));
		DBWriteContactSettingDword(hContact,  gpszICQProtoName, "IdleTS",       tIdleTS);


		// Update info?
		if ((time(NULL) - DBGetContactSettingDword(hContact, gpszICQProtoName, "InfoTS", 0)) > UPDATE_THRESHOLD)
			icq_QueueUser(hContact);
	}
	else
	{
		dwLocalDirectConnCookie = dwDirectConnCookie;
	}


	// And a small log notice...
	Netlib_Logf(ghServerNetlibUser, "%u changed status to %s (v%d).",
		dwUIN, MirandaStatusToString(IcqStatusToMiranda(wStatus)), wVersion);

}



static void handleUserOffline(BYTE *buf, WORD wLen)
{

	HANDLE hContact;
	DWORD dwUIN;


	// Unpack the sender's user ID
	{
		BYTE nUIDLen;
		char* pszUID;

		unpackByte(&buf, &nUIDLen);
		wLen -= 1;

		if (nUIDLen > wLen)
		{
			Netlib_Logf(ghServerNetlibUser, "SNAC(3.C) Invalid UIN 1");
			return;
		}

		if (!(pszUID = malloc(nUIDLen+1)))
			return; // Memory failure
		unpackString(&buf, pszUID, nUIDLen);
		wLen -= nUIDLen;
		pszUID[nUIDLen] = '\0';

		if (!IsStringUIN(pszUID))
		{
			Netlib_Logf(ghServerNetlibUser, "SNAC(3.C) Invalid UIN 2");
			SAFE_FREE(&pszUID);
			return;
		}

		dwUIN = atoi(pszUID);
		SAFE_FREE(&pszUID);
	}


	hContact = HContactFromUIN(dwUIN, 0);

	// Skip contacts that are not already on our list
	if (hContact != INVALID_HANDLE_VALUE)
	{
		Netlib_Logf(ghServerNetlibUser, "%u went offline.", dwUIN);
		DBWriteContactSettingWord(hContact, gpszICQProtoName, "Status", ID_STATUS_OFFLINE);
		DBWriteContactSettingDword(hContact, gpszICQProtoName, "IdleTS", 0);
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

		Netlib_Logf(ghServerNetlibUser, "MaxUINs %u", wMaxUins);
		Netlib_Logf(ghServerNetlibUser, "MaxWatchers %u", wMaxWatchers);

		disposeChain(&pChain);

	}
	else
	{
		Netlib_Logf(ghServerNetlibUser, "Error: Malformed BuddyRepyl");
	}

}
