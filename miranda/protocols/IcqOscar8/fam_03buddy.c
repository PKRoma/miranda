// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004,2005 Martin Öberg, Sam Kothari, Robert Rainwater
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



static void handleUserOffline(BYTE* buf, WORD wPackLen);
static void handleUserOnline(BYTE* buf, WORD wPackLen);
static void handleReplyBuddy(BYTE* buf, WORD wPackLen);

extern DWORD dwLocalDirectConnCookie;
extern HANDLE ghServerNetlibUser;
extern char gpszICQProtoName[MAX_PATH];



void handleBuddyFam(unsigned char* pBuffer, WORD wBufferLength, snac_header* pSnacHeader)
{

	switch (pSnacHeader->wSubtype)
	{

	case SRV_USERONLINE:
		handleUserOnline(pBuffer, wBufferLength);
		break;

	case SRV_USEROFFLINE:
		handleUserOffline(pBuffer, wBufferLength);
		break;

	case SRV_REPLYBUDDY:
		handleReplyBuddy(pBuffer, wBufferLength);
		break;

	default:
		Netlib_Logf(ghServerNetlibUser, "Warning: Ignoring SNAC(x03,x%02x) - Unknown SNAC (Flags: %u, Ref: %u", pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
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
static void handleUserOnline(BYTE* buf, WORD wLen)
{

	HANDLE hContact;
	DWORD dwPort;
	DWORD dwRealIP;
	DWORD dwIP;
	DWORD dwUIN;
	DWORD dwDirectConnCookie;
	DWORD dwMirVer = 0;
	WORD wVersion;
	WORD wTLVCount;
	WORD wWarningLevel;
	WORD wStatusFlags;
	WORD wStatus;
	BYTE nTCPFlag;
	DWORD dwOnlineSince;
	WORD wIdleTimer = 0;


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
			SAFE_FREE(pszUID);
			return;
		}

		dwUIN = atoi(pszUID);
		SAFE_FREE(pszUID);
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
			DWORD dwMirandaSignature;

			pBuffer = pTLV->pData;
			unpackDWord(&pBuffer, &dwRealIP);
			unpackDWord(&pBuffer, &dwPort);
			unpackByte(&pBuffer,  &nTCPFlag);
			unpackWord(&pBuffer,  &wVersion);
			unpackDWord(&pBuffer, &dwDirectConnCookie);
			pBuffer += 4; // Web front port
			pBuffer += 4; // Client features

			// Is this a Miranda IM client?
			unpackDWord(&pBuffer, &dwMirandaSignature);
			if (dwMirandaSignature == 0xffffffff)
			{
				// Yes, get the version info
				unpackDWord(&pBuffer, &dwMirVer);
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
			dwMirVer = 0;
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


			// Update the contact's capabilities
			if (wOldStatus == ID_STATUS_OFFLINE)
			{

				// Delete the capabilities we saved the last time this contact came online
				ClearAllContactCapabilities(hContact);

				// Get Capability Info TLV
				pTLV = getTLV(pChain, 0x0D, 1);

				if (pTLV && (pTLV->wLen >= 16))
				{
					AddCapabilitiesFromBuffer(hContact, pTLV->pData, pTLV->wLen);
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

		}

		
		// Free TLV chain
		disposeChain(&pChain);
	}



	// Save contacts details in database
	if (hContact != NULL)
	{
		if (dwMirVer)
			DBWriteContactSettingDword(hContact, gpszICQProtoName, "ClientID", 1);
		else
			DBWriteContactSettingDword(hContact, gpszICQProtoName, "ClientID", 0);

		DBWriteContactSettingDword(hContact,  gpszICQProtoName, "LogonTS",      dwOnlineSince);
		DBWriteContactSettingDword(hContact,  gpszICQProtoName, "IP",           dwIP);
		DBWriteContactSettingDword(hContact,  gpszICQProtoName, "RealIP",       dwRealIP);
		DBWriteContactSettingDword(hContact,  gpszICQProtoName, "DirectCookie", dwDirectConnCookie);
		DBWriteContactSettingWord(hContact,   gpszICQProtoName, "UserPort",     (WORD)(dwPort & 0xffff));
		DBWriteContactSettingWord(hContact,   gpszICQProtoName, "Version",      wVersion);
		DBWriteContactSettingString(hContact, gpszICQProtoName, "MirVer",       MirandaVersionToString(dwMirVer));
		DBWriteContactSettingWord(hContact,   gpszICQProtoName, "Status",       (WORD)IcqStatusToMiranda(wStatus));


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
			SAFE_FREE(pszUID);
			return;
		}

		dwUIN = atoi(pszUID);
		SAFE_FREE(pszUID);
	}


	hContact = HContactFromUIN(dwUIN, 0);

	// Skip contacts that are not already on our list
	if (hContact != INVALID_HANDLE_VALUE)
	{
		Netlib_Logf(ghServerNetlibUser, "%u went offline.", dwUIN);
		DBWriteContactSettingWord(hContact, gpszICQProtoName, "Status", ID_STATUS_OFFLINE);
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
