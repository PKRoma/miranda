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
//  Handles packets from Buddy family
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


extern const char* cliSpamBot;

void CIcqProto::handleBuddyFam(BYTE *pBuffer, WORD wBufferLength, snac_header *pSnacHeader, serverthread_info *info)
{
	switch (pSnacHeader->wSubtype)
	{
	case ICQ_USER_ONLINE:
		handleUserOnline(pBuffer, wBufferLength, info);
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


void CIcqProto::handleReplyBuddy(BYTE *buf, WORD wPackLen)
{
	oscar_tlv_chain *pChain = readIntoTLVChain(&buf, wPackLen, 0);

	if (pChain)
	{
		DWORD wMaxUins = pChain->getWord(1, 1);
		DWORD wMaxWatchers = pChain->getWord(2, 1);
		DWORD wMaxTemporary = pChain->getWord(4, 1);

		NetLog_Server("MaxUINs %u", wMaxUins);
		NetLog_Server("MaxWatchers %u", wMaxWatchers);
		NetLog_Server("MaxTemporary %u", wMaxTemporary);

		disposeChain(&pChain);
	}
	else
	{
		NetLog_Server("Error: Malformed BuddyReply");
	}
}


int unpackSessionDataItem(oscar_tlv_chain *pChain, WORD wItemType, BYTE **ppItemData, WORD *pwItemSize, BYTE *pbItemFlags)
{
	oscar_tlv *tlv = pChain->getTLV(0x1D, 1);
	int len = 0;
	BYTE *data;

	if (tlv) 
	{
		len = tlv->wLen;
		data = tlv->pData;
	}

	while (len >= 4)
	{ // parse session data items one by one
		WORD itemType;
		BYTE itemFlags;
		BYTE itemLen;

		unpackWord(&data, &itemType);
		unpackByte(&data, &itemFlags);
		unpackByte(&data, &itemLen);
		len -= 4;

		// just some validity check
		if (itemLen > len) 
			itemLen = len;

		if (itemType == wItemType)
		{ // found the requested item
			if (ppItemData)
				*ppItemData = data;
			if (pwItemSize)
				*pwItemSize = itemLen;
			if (pbItemFlags)
				*pbItemFlags = itemFlags;

			return 1; // Success
		}
		data += itemLen;
		len -= itemLen;
	}
	return 0;
}


// TLV(1) User class
// TLV(3) Signon time
// TLV(4) Idle time (in minutes)
// TLV(5) Member since
// TLV(6) New status
// TLV(8) Status Capabilities
// TLV(A) External IP
// TLV(C) DC Info
// TLV(D) Capabilities
// TLV(F) Session timer (in seconds)
// TLV(14) Instance number (AIM only)
// TLV(19) Short capabilities
// TLV(1D) Session Data (Avatar, Mood, etc.)
// TLV(1F) User class (upper bytes)
// TLV(26) AIM Profile update time
// TLV(27) AIM Away message update time
// TLV(29) Away status since
// TLV(2B) URL to protocol icon
// TLV(2F) unknown key
// TLV(30) unknown timestamp

void CIcqProto::handleUserOnline(BYTE *buf, WORD wLen, serverthread_info *info)
{
	DWORD dwPort = 0;
	DWORD dwRealIP = 0;
	DWORD dwUIN;
	uid_str szUID;
	DWORD dwDirectConnCookie = 0;
	DWORD dwWebPort = 0;
	DWORD dwFT1 = 0, dwFT2 = 0, dwFT3 = 0;
	const char *szClient = NULL;
	BYTE bClientId = 0;
	WORD wVersion = 0;
	WORD wTLVCount;
	WORD wWarningLevel;
	WORD wStatusFlags;
	WORD wStatus = 0, wOldStatus = 0;
	BYTE nTCPFlag = 0;
	char szStrBuf[MAX_PATH];

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
	HANDLE hContact = HContactFromUID(dwUIN, szUID, NULL);

	// Ignore status notification if the user is not already on our list
	if (hContact == INVALID_HANDLE_VALUE)
	{
#ifdef _DEBUG
		NetLog_Server("Ignoring user online (%s)", strUID(dwUIN, szUID));
#endif
		return;
	}

	// Read user info TLVs
	oscar_tlv_chain *pChain;
	oscar_tlv *pTLV;

	// Syntax check
	if (wLen < 4)
		return;

	// Get chain
	if (!(pChain = readIntoTLVChain(&buf, wLen, wTLVCount)))
		return;

	// Get Class word
	WORD wClass = pChain->getWord(0x01, 1);
	int nIsICQ = wClass & CLASS_ICQ;

	if (dwUIN)
	{
		// Get DC info TLV
		pTLV = pChain->getTLV(0x0C, 1);
		if (pTLV && (pTLV->wLen >= 15))
		{
			BYTE *pBuffer = pTLV->pData;

			nIsICQ = TRUE;

			unpackDWord(&pBuffer, &dwRealIP);
			unpackDWord(&pBuffer, &dwPort);
			unpackByte(&pBuffer,  &nTCPFlag);
			unpackWord(&pBuffer,  &wVersion);
			unpackDWord(&pBuffer, &dwDirectConnCookie);
			unpackDWord(&pBuffer, &dwWebPort); // Web front port
			pBuffer += 4; // Client features

			// Get faked time signatures, used to identify clients
			if (pTLV->wLen >= 0x23)
			{
				unpackDWord(&pBuffer, &dwFT1);
				unpackDWord(&pBuffer, &dwFT2);
				unpackDWord(&pBuffer, &dwFT3);
			}
		}
		else
		{
			// This client doesnt want DCs
		}

		// Get Status info TLV
		pTLV = pChain->getTLV(0x06, 1);
		if (pTLV && (pTLV->wLen >= 4))
		{
			BYTE *pBuffer = pTLV->pData;

			unpackWord(&pBuffer, &wStatusFlags);
			unpackWord(&pBuffer, &wStatus);
		}
		else if (!nIsICQ)
		{
			// Connected thru AIM client, guess by user class
			if (wClass & CLASS_AWAY)
				wStatus = ID_STATUS_AWAY;
			else if (wClass & CLASS_WIRELESS)
				wStatus = ID_STATUS_ONTHEPHONE;
			else
				wStatus = ID_STATUS_ONLINE;

			wStatusFlags = 0;
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
		nIsICQ = FALSE;

		if (wClass & CLASS_AWAY)
			wStatus = ID_STATUS_AWAY;
		else if (wClass & CLASS_WIRELESS)
			wStatus = ID_STATUS_ONTHEPHONE;
		else
			wStatus = ID_STATUS_ONLINE;

		wStatusFlags = 0;
	}

#ifdef _DEBUG
	NetLog_Server("Flags are %x", wStatusFlags);
	NetLog_Server("Status is %x", wStatus);
#endif

	// Get IP TLV
	DWORD dwIP = pChain->getDWord(0x0A, 1);

	// Get Online Since TLV
	DWORD dwOnlineSince = pChain->getDWord(0x03, 1);

	// Get Away Since TLV
	DWORD dwAwaySince = pChain->getDWord(0x29, 1);

	// Get Member Since TLV
	DWORD dwMemberSince = pChain->getDWord(0x05, 1);

	// Get Idle timer TLV
	WORD wIdleTimer = pChain->getWord(0x04, 1);
	time_t tIdleTS = 0;
	if (wIdleTimer)
	{
		time(&tIdleTS);
		tIdleTS -= (wIdleTimer*60);
	};

#ifdef _DEBUG
	if (wIdleTimer)
		NetLog_Server("Idle timer is %u.", wIdleTimer);
	NetLog_Server("Online since %s", time2text(dwOnlineSince));
	if (dwAwaySince)
		NetLog_Server("Status was set on %s", time2text(dwAwaySince));
#endif

	// Check client capabilities
	if (hContact != NULL)
	{
		wOldStatus = getContactStatus(hContact);

		// Collect all Capability info from TLV chain
		BYTE *capBuf = NULL;
		WORD capLen = 0;

		// Get Location Capability Info TLVs
		oscar_tlv *pFullTLV = pChain->getTLV(0x0D, 1);
		oscar_tlv *pShortTLV = pChain->getTLV(0x19, 1);

		if (pFullTLV && (pFullTLV->wLen >= BINARY_CAP_SIZE))
			capLen += pFullTLV->wLen;

		if (pShortTLV && (pShortTLV->wLen >= 2))
			capLen += (pShortTLV->wLen * 8);

		capBuf = (BYTE*)_alloca(capLen + BINARY_CAP_SIZE);

		if (capLen)
		{
			BYTE *pCapability = capBuf;

			capLen = 0; // we need to recount that

			if (pFullTLV && (pFullTLV->wLen >= BINARY_CAP_SIZE))
			{ // copy classic Capabilities
				BYTE *cData = pFullTLV->pData;
				int cLen = pFullTLV->wLen;

				while (cLen)
				{ // be impervious to duplicates (AOL sends them sometimes)
					if (!capLen || !MatchCapability(capBuf, capLen, (capstr*)cData, BINARY_CAP_SIZE))
					{ // not present, add
						memcpy(pCapability, cData, BINARY_CAP_SIZE);
						capLen += BINARY_CAP_SIZE;
						pCapability += BINARY_CAP_SIZE;
					}
					cData += BINARY_CAP_SIZE;
					cLen -= BINARY_CAP_SIZE;
				}
			}

			if (pShortTLV && (pShortTLV->wLen >= 2))
			{ // copy short Capabilities
				capstr tmp;
				BYTE *cData = pShortTLV->pData;
				int cLen = pShortTLV->wLen;

				memcpy(tmp, capShortCaps, BINARY_CAP_SIZE);
				while (cLen)
				{ // be impervious to duplicates (AOL sends them sometimes)
					tmp[2] = cData[0];
					tmp[3] = cData[1];

					if (!capLen || !MatchCapability(capBuf, capLen, &tmp, BINARY_CAP_SIZE))
					{ // not present, add
						memcpy(pCapability, tmp, BINARY_CAP_SIZE);
						capLen += BINARY_CAP_SIZE;
						pCapability += BINARY_CAP_SIZE;
					}
					cData += 2;
					cLen -= 2;
				}
			}
#ifdef _DEBUG
			NetLog_Server("Detected %d capability items.", capLen / BINARY_CAP_SIZE);
#endif
		}

		if (capLen)
		{ // Update the contact's capabilies if present in packet
			SetCapabilitiesFromBuffer(hContact, capBuf, capLen, wOldStatus == ID_STATUS_OFFLINE);

			char *szCurrentClient = wOldStatus == ID_STATUS_OFFLINE ? NULL : getSettingStringUtf(hContact, "MirVer", NULL);

			szClient = detectUserClient(hContact, nIsICQ, wClass, dwOnlineSince, szCurrentClient, wVersion, dwFT1, dwFT2, dwFT3, nTCPFlag, dwDirectConnCookie, dwWebPort, capBuf, capLen, &bClientId, szStrBuf);
			// Check if the client changed, if not do not change
			if (szCurrentClient && !strcmpnull(szCurrentClient, szClient))
				szClient = (const char*)-1;
			SAFE_FREE(&szCurrentClient);
		}
		else if (wOldStatus == ID_STATUS_OFFLINE)
		{
			// Remove the contact's capabilities if coming from offline
			ClearAllContactCapabilities(hContact);

			// no capability
			NetLog_Server("No capability info TLVs");

			szClient = detectUserClient(hContact, nIsICQ, wClass, dwOnlineSince, NULL, wVersion, dwFT1, dwFT2, dwFT3, nTCPFlag, dwDirectConnCookie, dwWebPort, NULL, capLen, &bClientId, szStrBuf);
		}
		else
		{
			// Capabilities not present in update packet, do not touch
			szClient = (const char*)-1; // we don't want to client be overwritten
		}

		// handle Xtraz status
		char *moodData = NULL;
		WORD moodSize = 0;

		unpackSessionDataItem(pChain, 0x0E, (BYTE**)&moodData, &moodSize, NULL);
		if (capLen || wOldStatus == ID_STATUS_OFFLINE)
			handleXStatusCaps(dwUIN, szUID, hContact, capBuf, capLen, moodData, moodSize);
		else
			handleXStatusCaps(dwUIN, szUID, hContact, NULL, 0, moodData, moodSize);

		// Determine support for extended status messages
		if (pChain->getWord(0x08, 1) == 0x0A06)
			SetContactCapabilities(hContact, CAPF_STATUS_MESSAGES);
		else if (wOldStatus == ID_STATUS_OFFLINE)
			ClearContactCapabilities(hContact, CAPF_STATUS_MESSAGES);

#ifdef _DEBUG
		if (wOldStatus == ID_STATUS_OFFLINE)
		{
			if (CheckContactCapabilities(hContact, CAPF_SRV_RELAY))
				NetLog_Server("Supports advanced messages");
			else
				NetLog_Server("Does NOT support advanced messages");
		}
#endif

		if (!nIsICQ)
		{
			// AIM clients does not advertise these, but do support them
			SetContactCapabilities(hContact, CAPF_UTF | CAPF_TYPING);
			// Server relayed messages are only supported by ICQ clients
			ClearContactCapabilities(hContact, CAPF_SRV_RELAY);

			if (dwUIN && wOldStatus == ID_STATUS_OFFLINE)
				NetLog_Server("Logged in with AIM client");
		}

		if (nIsICQ && wVersion < 8)
		{
			ClearContactCapabilities(hContact, CAPF_SRV_RELAY);
			if (wOldStatus == ID_STATUS_OFFLINE)
				NetLog_Server("Forcing simple messages due to compability issues");
		}

		// Process Avatar Hash
		pTLV = pChain->getTLV(0x1D, 1);
		if (pTLV)
			handleAvatarContactHash(dwUIN, szUID, hContact, pTLV->pData, pTLV->wLen, wOldStatus);
		else
			handleAvatarContactHash(dwUIN, szUID, hContact, NULL, 0, wOldStatus);

		// Process Status Note
		parseStatusNote(dwUIN, szUID, hContact, pChain);
	}
	// Free TLV chain
	disposeChain(&pChain);

	// Save contacts details in database
	if (hContact != NULL)
	{
		setSettingDword(hContact,   "LogonTS",      dwOnlineSince);
		setSettingDword(hContact,   "AwayTS",       dwAwaySince);
		setSettingDword(hContact,   "IdleTS", tIdleTS);

		if (dwMemberSince)
			setSettingDword(hContact, "MemberTS",     dwMemberSince);

		if (nIsICQ)
		{ // on AIM these are not used
			setSettingDword(hContact, "DirectCookie", dwDirectConnCookie);
			setSettingByte(hContact,  "DCType",       (BYTE)nTCPFlag);
			setSettingWord(hContact,  "UserPort",     (WORD)(dwPort & 0xffff));
			setSettingWord(hContact,  "Version",      wVersion);
		}
		else
		{
			deleteSetting(hContact,   "DirectCookie");
			deleteSetting(hContact,   "DCType");
			deleteSetting(hContact,   "UserPort");
			deleteSetting(hContact,   "Version");
		}

		if (!szClient)
		{
			// if no detection, set uknown
			szClient = (nIsICQ ? "Unknown" : "Unknown AIM");		
		}
		if (szClient != (char*)-1)
		{
			setSettingStringUtf(hContact, "MirVer",   szClient);
			setSettingByte(hContact,  "ClientID",     bClientId);
		}

		if (wOldStatus == ID_STATUS_OFFLINE)
		{
			setSettingDword(hContact, "IP",           dwIP);
			setSettingDword(hContact, "RealIP",       dwRealIP);
		}
		else
		{ // if not first notification only write significant information
			if (dwIP)
				setSettingDword(hContact, "IP",         dwIP);
			if (dwRealIP)
				setSettingDword(hContact, "RealIP",     dwRealIP);
		}
		setSettingWord(hContact,  "Status", (WORD)IcqStatusToMiranda(wStatus));

		// Update info?
		if (dwUIN)
		{ // check if the local copy of user details is up-to-date
			if (IsMetaInfoChanged(hContact))
				icq_QueueUser(hContact);
		}
	}

	if (wOldStatus != IcqStatusToMiranda(wStatus))
	{ // And a small log notice... if status was changed
		if (nIsICQ)
			NetLog_Server("%u changed status to %s (v%d).", dwUIN, MirandaStatusToString(IcqStatusToMiranda(wStatus)), wVersion);
		else
			NetLog_Server("%s changed status to %s.", strUID(dwUIN, szUID), MirandaStatusToString(IcqStatusToMiranda(wStatus)));
	}
#ifdef _DEBUG
	else
	{
		if (nIsICQ)
			NetLog_Server("%u has status %s (v%d).", dwUIN, MirandaStatusToString(IcqStatusToMiranda(wStatus)), wVersion);
		else
			NetLog_Server("%s has status %s.", strUID(dwUIN, szUID), MirandaStatusToString(IcqStatusToMiranda(wStatus)));
	}
#endif

	if (szClient == cliSpamBot)
	{
		if (getSettingByte(NULL, "KillSpambots", DEFAULT_KILLSPAM_ENABLED) && DBGetContactSettingByte(hContact, "CList", "NotOnList", 0))
		{ // kill spammer
			icq_DequeueUser(dwUIN);
			icq_sendRemoveContact(dwUIN, NULL);
			AddToSpammerList(dwUIN);
			if (getSettingByte(NULL, "PopupsSpamEnabled", DEFAULT_SPAM_POPUPS_ENABLED))
				ShowPopUpMsg(hContact, LPGEN("Spambot Detected"), LPGEN("Contact deleted & further events blocked."), POPTYPE_SPAM);
			CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);

			NetLog_Server("Contact %u deleted", dwUIN);
		}
	}
}


void CIcqProto::handleUserOffline(BYTE *buf, WORD wLen)
{
	DWORD dwUIN;
	uid_str szUID;

	do {
		oscar_tlv_chain *pChain = NULL;
		WORD wTLVCount;
		DWORD dwAwaySince;

		// Unpack the sender's user ID
		if (!unpackUID(&buf, &wLen, &dwUIN, &szUID)) return;

		// Warning level?
		buf += 2;

		// TLV Count
		unpackWord(&buf, &wTLVCount);
		wLen -= 4;

		// Skip the TLV chain
		while (wTLVCount && wLen >= 4)
		{
			WORD wTLVType;
			WORD wTLVLen;

			unpackWord(&buf, &wTLVType);
			unpackWord(&buf, &wTLVLen);
			wLen -= 4;

			// stop parsing overflowed packet
			if (wTLVLen > wLen) 
			{
				disposeChain(&pChain);
				return;
			}

			if (wTLVType == 0x1D)
			{ // read only TLV with Session data into chain
				BYTE *pTLV = buf - 4;
				disposeChain(&pChain);
				pChain = readIntoTLVChain(&pTLV, wLen + 4, 1);
			}
			else if (wTLVType == 0x29 && wTLVLen == sizeof(DWORD))
			{ // get Away Since value
				BYTE *pData = buf;
				unpackDWord(&pData, &dwAwaySince);
			}

			buf += wTLVLen;
			wLen -= wTLVLen;
			wTLVCount--;
		}

		// Determine contact
		HANDLE hContact = HContactFromUID(dwUIN, szUID, NULL);

		// Skip contacts that are not already on our list or are already offline
		if (hContact != INVALID_HANDLE_VALUE)
		{
			WORD wOldStatus = getContactStatus(hContact);

			// Process Avatar Hash
			oscar_tlv *pAvatarTLV = pChain ? pChain->getTLV(0x1D, 1) : NULL;
			if (pAvatarTLV)
				handleAvatarContactHash(dwUIN, szUID, hContact, pAvatarTLV->pData, pAvatarTLV->wLen, wOldStatus);
			else
				handleAvatarContactHash(dwUIN, szUID, hContact, NULL, 0, wOldStatus);

			// Process Status Note (offline status note)
			parseStatusNote(dwUIN, szUID, hContact, pChain);

			// Update status times
			setSettingDword(hContact, "IdleTS", 0);
			setSettingDword(hContact, "AwayTS", dwAwaySince);

			// Clear custom status & mood
			char tmp = NULL;
			handleXStatusCaps(dwUIN, szUID, hContact, (BYTE*)&tmp, 0, &tmp, 0);

			if (wOldStatus != ID_STATUS_OFFLINE)
			{
				NetLog_Server("%s went offline.", strUID(dwUIN, szUID));

				setSettingWord(hContact, "Status", ID_STATUS_OFFLINE);
				// close Direct Connections to that user
				CloseContactDirectConns(hContact);
				// Reset DC status
				setSettingByte(hContact, "DCStatus", 0);
			}
#ifdef _DEBUG
			else
				NetLog_Server("%s is offline.", strUID(dwUIN, szUID));
#endif
		}

		// Release memory
		disposeChain(&pChain);
	}
	while (wLen >= 1);
}


void CIcqProto::parseStatusNote(DWORD dwUin, char *szUid, HANDLE hContact, oscar_tlv_chain *pChain)
{
	DWORD dwStatusNoteTS = time(NULL);
	BYTE *pStatusNoteTS, *pStatusNote;
	WORD wStatusNoteTSLen, wStatusNoteLen;
	BYTE bStatusNoteFlags;

	if (unpackSessionDataItem(pChain, 0x0D, &pStatusNoteTS, &wStatusNoteTSLen, NULL) && wStatusNoteTSLen == sizeof(DWORD))
		unpackDWord(&pStatusNoteTS, &dwStatusNoteTS);

	// Get Status Note session item
	if (unpackSessionDataItem(pChain, 0x02, &pStatusNote, &wStatusNoteLen, &bStatusNoteFlags))
	{
		char *szStatusNote = NULL;

		if ((bStatusNoteFlags & 4) == 4 && wStatusNoteLen >= 4)
		{
			BYTE *buf = pStatusNote;
			WORD buflen = wStatusNoteLen - 2;
			WORD wTextLen;

			unpackWord(&buf, &wTextLen);
			if (wTextLen > buflen)
				wTextLen = buflen;

			if (wTextLen > 0)
			{
				szStatusNote = (char*)_alloca(wStatusNoteLen + 1);
				unpackString(&buf, szStatusNote, wTextLen);
				szStatusNote[wTextLen] = '\0';
				buflen -= wTextLen;

				WORD wEncodingType = 0;
				char *szEncoding = NULL;

				if (buflen >= 2)
					unpackWord(&buf, &wEncodingType);

				if (wEncodingType == 1 && buflen > 6)
				{ // Encoding specified
					buf += 2;
					buflen -= 2;
					unpackWord(&buf, &wTextLen);
					if (wTextLen > buflen)
						wTextLen = buflen;
					szEncoding = (char*)_alloca(wTextLen + 1);
					unpackString(&buf, szEncoding, wTextLen);
					szEncoding[wTextLen] = '\0';
				}
				else if (UTF8_IsValid(szStatusNote))
					szEncoding = "utf-8";

				szStatusNote = ApplyEncoding(szStatusNote, szEncoding);
			}
		}
		// Check if the status note was changed
		if (dwStatusNoteTS > getSettingDword(hContact, DBSETTING_STATUS_NOTE_TIME, 0))
		{
			DBVARIANT dbv = {DBVT_DELETED};

			if (strlennull(szStatusNote) || (!getSettingString(hContact, DBSETTING_STATUS_NOTE, &dbv) && (dbv.type == DBVT_ASCIIZ || dbv.type == DBVT_UTF8) && strlennull(dbv.pszVal)))
				NetLog_Server("%s changed status note to \"%s\"", strUID(dwUin, szUid), szStatusNote ? szStatusNote : "");

			ICQFreeVariant(&dbv);

			if (szStatusNote)
				setSettingStringUtf(hContact, DBSETTING_STATUS_NOTE, szStatusNote);
			else
				deleteSetting(hContact, DBSETTING_STATUS_NOTE);
			setSettingDword(hContact, DBSETTING_STATUS_NOTE_TIME, dwStatusNoteTS);

			{ // Broadcast a notification
				int nNoteLen = strlennull(szStatusNote);
				char *szNoteAnsi = NULL;

				if (nNoteLen)
				{ // the broadcast does not support unicode
					szNoteAnsi = (char*)_alloca(nNoteLen + 1);
					utf8_decode_static(szStatusNote, szNoteAnsi, strlennull(szStatusNote) + 1);
				}
				if (getContactXStatus(hContact) != 0 || !CheckContactCapabilities(hContact, CAPF_STATUS_MESSAGES))
				{
					setStatusMsgVar(hContact, szStatusNote, false);
					BroadcastAck(hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, NULL, (LPARAM)szNoteAnsi);
				}
			}
		}
		SAFE_FREE(&szStatusNote);
	}
	else
	{
		if (getContactStatus(hContact) == ID_STATUS_OFFLINE)
		{
			setStatusMsgVar(hContact, NULL, false);
			deleteSetting(hContact, DBSETTING_STATUS_NOTE);
			setSettingDword(hContact, DBSETTING_STATUS_NOTE_TIME, dwStatusNoteTS);
		}
	}
}


void CIcqProto::handleNotifyRejected(BYTE *buf, WORD wPackLen)
{
	DWORD dwUIN;
	uid_str szUID;

	while (wPackLen)
		if (unpackUID(&buf, &wPackLen, &dwUIN, &szUID))
			NetLog_Server("%s status notification rejected.", strUID(dwUIN, szUID));
}
