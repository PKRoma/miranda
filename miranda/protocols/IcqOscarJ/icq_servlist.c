// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
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



extern BYTE gbSsiEnabled;
extern HANDLE ghServerNetlibUser;
extern BOOL bIsSyncingCL;
extern char gpszICQProtoName[MAX_PATH];
extern int gnCurrentStatus;

static HANDLE hHookSettingChanged = NULL;
static HANDLE hHookContactDeleted = NULL;
static WORD* pwIDList = NULL;
static int nIDListCount = 0;
static int nIDListSize = 0;



// Add a server ID to the list of reserved IDs.
// To speed up the process, IDs cannot be removed, and if
// you try to reserve an ID twice, it will be added again.
// You should call CheckServerID before reserving an ID.
void ReserveServerID(WORD wID)
{
	
	if (nIDListCount >= nIDListSize)
	{
		nIDListSize += 100;
		pwIDList = (WORD*)realloc(pwIDList, nIDListSize * sizeof(WORD));
	}

	pwIDList[nIDListCount] = wID;
	nIDListCount++;
	
}



// Returns true if dwID is reserved
BOOL CheckServerID(WORD wID)
{
	
	int i;
	BOOL bFound = FALSE;
	
	
	if (pwIDList)
	{
		for (i = 0; i<nIDListCount; i++)
		{
			if (pwIDList[i] == wID)
				bFound = TRUE;
		}
	}
	
	return bFound;
	
}



void FlushServerIDs()
{
	
	SAFE_FREE(pwIDList);
	nIDListCount = 0;
	nIDListSize = 0;
	
}



WORD GenerateServerId(VOID)
{

	WORD wId;


	while (TRUE)
	{
		// Randomize a new ID
		// Max value is probably 0x7FFF, lowest value is unknown.
		// We use range 0x1000-0x7FFF.
		wId = (WORD)RandRange(0x1000, 0x7FFF);

		if (!CheckServerID(wId))
			break;
		
		break;
	}
	
	ReserveServerID(wId);

	return wId;

}



WORD icq_sendUploadContactServ(DWORD dwUin, WORD wGroupId, WORD wContactId, const char *szNick, int authRequired, WORD wItemType)
{

   	icq_packet packet;
	char szUin[10];
	int nUinLen;
	int nNickLen;
	WORD wSequence;
	char* szUtfNick = NULL;
	WORD wTLVlen;


	// Prepare UIN
	_itoa(dwUin, szUin, 10);
	nUinLen = strlen(szUin);


	// Prepare custom nick name
	if (szNick && (strlen(szNick) > 0))
	{

		int nResult;

		nResult = utf8_encode(szNick, &szUtfNick);
		nNickLen = strlen(szUtfNick);

	}
	else
	{
		nNickLen = 0;
	}


	// Cookie
	wSequence = GenerateCookie();

	// Build the packet
	packet.wLen = nUinLen + 20;	
	if (nNickLen > 0)
		packet.wLen += nNickLen + 4;
	if (authRequired)
		packet.wLen += 4;

	write_flap(&packet, ICQ_DATA_CHAN);
	packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_ADDTOLIST, 0, wSequence);
	packWord(&packet, (WORD)nUinLen);
	packBuffer(&packet, szUin, (WORD)nUinLen);
	packWord(&packet, wGroupId);
	packWord(&packet, wContactId);
	packWord(&packet, wItemType);

	wTLVlen = ((nNickLen>0) ? 4+nNickLen : 0) + (authRequired?4:0);
	packWord(&packet, wTLVlen);
	if (authRequired)
		packDWord(&packet, 0x00660000);  // "Still waiting for auth" TLV
	if (nNickLen > 0)
	{
		packWord(&packet, 0x0131);	// Nickname TLV
		packWord(&packet, (WORD)nNickLen);
		packBuffer(&packet, szUtfNick, (WORD)nNickLen);
	}

	// Send the packet and return the cookie
	sendServPacket(&packet);

	SAFE_FREE(szUtfNick);

	return wSequence;
	
}


WORD icq_sendDeleteServerContactServ(DWORD dwUin, WORD wGroupId, WORD wContactId, WORD wItemType)
{

  	icq_packet packet;
	char szUin[10];
	int nUinLen;
	WORD wSequence;

	_itoa(dwUin, szUin, 10);
	nUinLen = strlen(szUin);

	wSequence = GenerateCookie();

	packet.wLen = nUinLen + 20;
	write_flap(&packet, ICQ_DATA_CHAN);
	packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_REMOVEFROMLIST, 0, wSequence);
	packWord(&packet, (WORD)nUinLen);
	packBuffer(&packet, szUin, (WORD)nUinLen);
	packWord(&packet, wGroupId);
	packWord(&packet, wContactId);
	packWord(&packet, wItemType);
	packWord(&packet, 0x0000);     // Length of additional data

	sendServPacket(&packet);

	return wSequence;

}



static int ServListDbSettingChanged(WPARAM wParam, LPARAM lParam)
{

	DBCONTACTWRITESETTING* cws = (DBCONTACTWRITESETTING*)lParam;


	// We can't upload changes to NULL contact
	if ((HANDLE)wParam == NULL)
		return 0;

	// TODO: Queue changes that occur while offline
	if (!icqOnline || !gbSsiEnabled || bIsSyncingCL)
		return 0;
	

	if (!strcmp(cws->szModule, "CList"))
	{
		
		// Has a temporary contact just been added permanently?
		if (!strcmp(cws->szSetting, "NotOnList") &&
			(cws->value.type == DBVT_DELETED || (cws->value.type == DBVT_BYTE && cws->value.bVal == 0)) &&
			DBGetContactSettingByte(NULL, gpszICQProtoName, "ServerAddRemove", DEFAULT_SS_ADDREMOVE))
		{
			
			char* szProto;
			DWORD dwUin;
			
			
			dwUin = DBGetContactSettingDword((HANDLE)wParam, gpszICQProtoName, UNIQUEIDSETTING, 0);
			szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)wParam, 0);
			
			// Is this a ICQ contact and does it have a UIN?
			if (szProto && !strcmp(szProto, gpszICQProtoName) && dwUin)
			{
				
				WORD wGroupId;
				WORD wContactId;

				
				wGroupId = (WORD)DBGetContactSettingWord(NULL, gpszICQProtoName, "SrvDefGroupId", 0);
				wContactId = DBGetContactSettingWord(NULL, gpszICQProtoName, "ServerId", 0);

				// Only add the contact if it doesnt already have an ID
				if (!wContactId)
				{
					wContactId = GenerateServerId();
					sendAddStart();
					icq_sendUploadContactServ(DBGetContactSettingDword((HANDLE)wParam, gpszICQProtoName, UNIQUEIDSETTING, 0), wGroupId, wContactId, "", 1, SSI_ITEM_BUDDY);
					sendAddEnd();
					DBWriteContactSettingWord((HANDLE)wParam, gpszICQProtoName, "ServerId", wContactId);
					DBWriteContactSettingWord((HANDLE)wParam, gpszICQProtoName, "SrvGroupId", wGroupId);
				}
				
			}
			
		}

		// Has contact been renamed?
		if (!strcmp(cws->szSetting, "MyHandle") &&
			DBGetContactSettingByte(NULL, gpszICQProtoName, "UseServerNicks", DEFAULT_SS_NICKS))
		{
			if (cws->value.type == DBVT_ASCIIZ && cws->value.pszVal != 0)
			{
				renameServContact((HANDLE)wParam, cws->value.pszVal);
			}
			else
			{
				renameServContact((HANDLE)wParam, NULL);
			}
		}
		
	}


	return 0;

}



static int ServListDbContactDeleted(WPARAM wParam, LPARAM lParam)
{
	
	if (!icqOnline || !gbSsiEnabled)
		return 0;
	
	if (DBGetContactSettingByte(NULL, gpszICQProtoName, "ServerAddRemove", DEFAULT_SS_ADDREMOVE))
	{

		WORD wContactID;
		WORD wGroupID;
		WORD wVisibleID;
		WORD wInvisibleID;
		DWORD dwUIN;

		
		wContactID = DBGetContactSettingWord((HANDLE)wParam, gpszICQProtoName, "ServerId", 0);
		wGroupID = DBGetContactSettingWord((HANDLE)wParam, gpszICQProtoName, "SrvGroupId", 0);
		wVisibleID = DBGetContactSettingWord((HANDLE)wParam, gpszICQProtoName, "SrvPermitId", 0);
		wInvisibleID = DBGetContactSettingWord((HANDLE)wParam, gpszICQProtoName, "SrvDenyId", 0);
		dwUIN = DBGetContactSettingDword((HANDLE)wParam, gpszICQProtoName, UNIQUEIDSETTING, 0);

		if ((wGroupID && wContactID) || wVisibleID || wInvisibleID)
		{
			sendAddStart();

			if (wContactID)
				icq_sendDeleteServerContactServ(dwUIN, wGroupID, wContactID, SSI_ITEM_BUDDY);

			if (wVisibleID)
				icq_sendDeleteServerContactServ(dwUIN, 0, wVisibleID, SSI_ITEM_PERMIT);

			if (wInvisibleID)
				icq_sendDeleteServerContactServ(dwUIN, 0, wInvisibleID, SSI_ITEM_DENY);

			sendAddEnd();
		}

	}

	return 0;

}



void InitServerLists(void)
{

	hHookSettingChanged = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, ServListDbSettingChanged);
	hHookContactDeleted = HookEvent(ME_DB_CONTACT_DELETED, ServListDbContactDeleted);

}



void UninitServerLists(void)
{

	if (hHookSettingChanged)
		UnhookEvent(hHookSettingChanged);

	if (hHookContactDeleted)
		UnhookEvent(hHookContactDeleted);

	FlushServerIDs();

}
