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



extern HANDLE ghServerNetlibUser;
extern int gnCurrentStatus;
extern char gpszICQProtoName[MAX_PATH];
extern DWORD dwLocalInternalIP;
extern DWORD dwLocalExternalIP;
extern WORD wListenPort;

BOOL bIsSyncingCL = FALSE;

static void handleServerCList(unsigned char *buf, WORD wLen, WORD wFlags);
static void handleRecvAuthRequest(unsigned char *buf, WORD wLen);
static void handleRecvAuthResponse(unsigned char *buf, WORD wLen);
static void handleRecvAdded(unsigned char *buf, WORD wLen);
void sendAddStart(void);
void sendAddEnd(void);
void sendRosterAck(void);



void handleServClistFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader)
{

	switch (pSnacHeader->wSubtype)
	{

	case ICQ_LISTS_LIST: // SRV_REPLYROSTER
		bIsSyncingCL = TRUE;
		handleServerCList(pBuffer, wBufferLength, pSnacHeader->wFlags);
		break;

	case ICQ_LISTS_SRV_REPLYLISTS:
		Netlib_Logf(ghServerNetlibUser, "Server sent SNAC(x13,x03) - SRV_REPLYLISTS");
		break;

	case ICQ_LISTS_ACK: // UPDATE_ACK
		if (wBufferLength >= 2)
		{

			WORD wError;


			unpackWord(&pBuffer, &wError);
			Netlib_Logf(ghServerNetlibUser, "Received server list ack %u", wError);
			ProtoBroadcastAck(gpszICQProtoName, NULL, ICQACKTYPE_SERVERCLIST, wError?ACKRESULT_FAILED:ACKRESULT_SUCCESS, (HANDLE)pSnacHeader->dwRef, wError);

		}
		// Todo: We will receive rename and move acks too, so it could be a good idea to reset
		// a future 'in sync' contact flag here.
		break;

	case SRV_SSI_UPTODATE: // SRV_REPLYROSTEROK
		bIsSyncingCL = FALSE;
		Netlib_Logf(ghServerNetlibUser, "Server sent SNAC(x13,x0F) - SRV_REPLYROSTEROK");

		// This will activate the server side list
		sendRosterAck();
		handleServUINSettings(wListenPort, dwLocalInternalIP);
		break;

	case ICQ_LISTS_CLI_MODIFYSTART:
		Netlib_Logf(ghServerNetlibUser, "Server sent SNAC(x13,x11) - Server is modifying contact list");
		break;

	case ICQ_LISTS_CLI_MODIFYEND:
		Netlib_Logf(ghServerNetlibUser, "Server sent SNAC(x13,x12) - End of server modification");
		break;

	case ICQ_LISTS_AUTHREQUEST:
		handleRecvAuthRequest(pBuffer, wBufferLength);
		break;

	case ICQ_LISTS_AUTHRESPONSE2:
		handleRecvAuthResponse(pBuffer, wBufferLength);
		break;

	case ICQ_LISTS_YOUWEREADDED:
	case ICQ_LISTS_YOUWEREADDED2:
		handleRecvAdded(pBuffer, wBufferLength);
		break;

	default:
		Netlib_Logf(ghServerNetlibUser, "Warning: Ignoring SNAC(x13,x%02x) - Unknown SNAC (Flags: %u, Ref: %u", pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
		break;

	}

}



static void handleServerCList(unsigned char *buf, WORD wLen, WORD wFlags)
{

	BYTE bySSIVersion;
	WORD wRecordCount;
	WORD wRecord;
	WORD wRecordNameLen;
	WORD wGroupId;
	WORD wItemId;
	WORD wTlvType;
	WORD wTlvLength;
	WORD wDefaultGroupId = 0;
	BOOL bIsLastPacket;
	char* pszRecordName = NULL;
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
	Netlib_Logf(ghServerNetlibUser, "SSI: number of entries is %u, version is %u", wRecordCount, bySSIVersion);


	// Loop over all items in the packet
	for (wRecord = 0; wRecord < wRecordCount; wRecord++)
	{

		Netlib_Logf(ghServerNetlibUser, "SSI: parsing record %u", wRecord + 1);

		if (wLen < 10)
		{
			// minimum: name length (zero), group ID, item ID, empty TLV
			Netlib_Logf(ghServerNetlibUser, "Warning: SSI parsing error (0)");
			break;
		}

		// The name of the entry. If this is a group header, then this
		// is the name of the group. If it is a plain contact list entry,
		// then it's the UIN of the contact.
		unpackWord(&buf, &wRecordNameLen);
		if (wLen < 10 + wRecordNameLen)
		{
			Netlib_Logf(ghServerNetlibUser, "Warning: SSI parsing error (1)");
			break;
		}
		pszRecordName = (char*)realloc(pszRecordName, wRecordNameLen + 1);
		unpackString(&buf, pszRecordName, wRecordNameLen);
		pszRecordName[wRecordNameLen] = '\0';

		// The group identifier this entry belongs to. If 0, this is meta information or
		// a contact without a group
		unpackWord(&buf, &wGroupId);

		// The ID of this entry. Group headers have ID 0. Otherwise, this
		// is a random number generated when the user is added to the
		// contact list, or when the user is ignored. See CLI_ADDBUDDY.
		unpackWord(&buf, &wItemId);

		// This field indicates what type of entry this is
		unpackWord(&buf, &wTlvType);

		// The length in bytes of the following TLV chain
		unpackWord(&buf, &wTlvLength);


		Netlib_Logf(ghServerNetlibUser, "Name: '%s', GroupID: %u, EntryID: %u, EntryType: %u, TLVlength: %u",
			pszRecordName, wGroupId, wItemId, wTlvType, wTlvLength);


		wLen -= (10 + wRecordNameLen);
		if (wLen < wTlvLength)
		{
			Netlib_Logf(ghServerNetlibUser, "Warning: SSI parsing error (2)");
			break;
		}


		// Initialize the tlv chain
		if (wTlvLength > 0)
		{
			pChain = readIntoTLVChain(&buf, (WORD)(wTlvLength), 0);
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

				if (!IsStringUIN(pszRecordName))
				{

					Netlib_Logf(ghServerNetlibUser, "Ignoring fake AOL contact, message is: \"%s\"", pszRecordName);

				}
				else
				{

					HANDLE hContact;
					char* pszProto;
					DWORD dwUin;


					// This looks like a real contact
					// Check if this user already exists in local list
					dwUin = atoi(pszRecordName);
					hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
					while (hContact)
					{
						pszProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
						if (pszProto && !strcmp(pszProto, gpszICQProtoName) && dwUin == DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0))
						{
							Netlib_Logf(ghServerNetlibUser, "SSI ignoring existing contact '%d'", dwUin);
							break;
						}
						hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
					}

					if (hContact == NULL)
					{
						// Not already on list: add
						Netlib_Logf(ghServerNetlibUser, "SSI adding new contact '%d'", dwUin);
						hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD,0,0);
						if (!hContact)
						{
							Netlib_Logf(ghServerNetlibUser, "Failed to create ICQ contact %u", dwUin);
							if (pChain)
								disposeChain(&pChain);
							continue;
						}
						if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)gpszICQProtoName) != 0)
						{
							// For some reason we failed to register the protocol to this contact
							CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
							Netlib_Logf(ghServerNetlibUser, "Failed to register ICQ contact %u", dwUin);
							if (pChain)
								disposeChain(&pChain);
							continue;
						}

						DBWriteContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, dwUin);
						if (!DBGetContactSettingByte(NULL, gpszICQProtoName, "AddServerNew", DEFAULT_SS_ADD))
							DBWriteContactSettingByte(hContact, "CList", "Hidden", 1);
					}
					else
					{
						// Make sure it's not hidden or temporary
						if (!DBGetContactSettingByte(NULL, gpszICQProtoName, "AddServerNew", DEFAULT_SS_ADD))
						{
							DBWriteContactSettingByte(hContact, "CList", "Hidden", 0);
							DBWriteContactSettingByte(hContact, "CList", "NotOnList", 0);
						}
					}

					// Save group and item ID
					DBWriteContactSettingWord(hContact, gpszICQProtoName, "ServerId", wItemId);
					DBWriteContactSettingWord(hContact, gpszICQProtoName, "SrvGroupId", wGroupId);
					ReserveServerID(wItemId);
					ReserveServerID(wGroupId);


					if (pChain)
					{
						// Look for nickname TLV and copy it to the db is necessary
						if (pTLV = getTLV(pChain, 0x0131, 1))
						{
							if (pTLV->pData && (pTLV->wLen > 0))
							{

								char* pszNick;
								WORD wNickLength;


								wNickLength = pTLV->wLen;

								pszNick = (char*)malloc(wNickLength + 1);
								if (pszNick)
								{

									char* pszTempNick = NULL; // Used for UTF-8 conversion


									// Copy buffer to utf-8 buffer
									memcpy(pszNick, pTLV->pData, wNickLength);
									pszNick[wNickLength] = 0; // Terminate string

									// Convert to ansi
									if (utf8_decode(pszNick, &pszTempNick))
									{
										SAFE_FREE(pszNick);
										pszNick = pszTempNick;
										Netlib_Logf(ghServerNetlibUser, "Nickname is '%s'", pszNick);
									}
									else
									{
										Netlib_Logf(ghServerNetlibUser, "Failed to convert Nickname '%s' from UTF-8", pszNick);
									}

									// Write nickname to database
									if (DBGetContactSettingByte(NULL, gpszICQProtoName, "UseServerNicks", DEFAULT_SS_NICKS))
									{
										DBVARIANT dbv;

										if (!DBGetContactSetting(hContact,"CList","MyHandle",&dbv))
										{
											if ((lstrcmp(dbv.pszVal, pszNick)) && (strlen(pszNick) > 0))
											{
												// Yes, we really do need to delete it first. Otherwise the CLUI nick
												// cache isn't updated (I'll look into it)
												DBDeleteContactSetting(hContact,"CList","MyHandle");
												DBWriteContactSettingString(hContact, "CList", "MyHandle", pszNick);
											}
											DBFreeVariant(&dbv);
										}
										else if (strlen(pszNick) > 0)
										{
											DBDeleteContactSetting(hContact,"CList","MyHandle");
											DBWriteContactSettingString(hContact, "CList", "MyHandle", pszNick);
										}
									}

									SAFE_FREE(pszNick);

								}

							}
							else
							{
								Netlib_Logf(ghServerNetlibUser, "Invalid nickname");
							}
						}


						// Look for need-authorization TLV
						if (pTLV = getTLV(pChain, 0x0066, 1))
						{
							DBWriteContactSettingByte(hContact, gpszICQProtoName, "Auth", 1);
							Netlib_Logf(ghServerNetlibUser, "SSI contact need authorization");
						}
						else
						{
							DBWriteContactSettingByte(hContact, gpszICQProtoName, "Auth", 0);
						}

					}

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
				{
					/* no item ID: this is a group */
					Netlib_Logf(ghServerNetlibUser, "Group %s was ignored, server groups not implemented", pszRecordName);
					/* pszRecordName is the name of the group */
					if (wDefaultGroupId == 0 || !strlen(pszRecordName) > 0)
						wDefaultGroupId = wGroupId;	  /* need to save one group ID so that we can do valid upload packets */
					/* TODO */
					/* wTlvType == 1 */
					/* TLV contains a TLV(C8) with a list of WORDs of contained contact IDs */
					/* our processing is good enough that we don't need this duplication */
				}
				else
				{
					Netlib_Logf(ghServerNetlibUser, "Unhandled type 0x01, wItemID != 0");
				}
			}
			else
			{
				Netlib_Logf(ghServerNetlibUser, "Unhandled type 0x01");
			}
			break;

		case SSI_ITEM_PERMIT:
			/* item on visible list */
			/* wItemId not related to contact ID */
			/* pszRecordName is the UIN */
			if (!IsStringUIN(pszRecordName))
			{
				Netlib_Logf(ghServerNetlibUser, "Ignoring fake AOL contact, message is: \"%s\"", pszRecordName);
			}
			else
			{

				DWORD dwUin;
				char* pszProto;
				HANDLE hContact;


				// This looks like a real contact
				// Check if this user already exists in local list
				dwUin = atoi(pszRecordName);
				hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
				while (hContact)
				{
					pszProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
					if (pszProto && !strcmp(pszProto, gpszICQProtoName) && dwUin == DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0))
					{
						Netlib_Logf(ghServerNetlibUser, "Permit contact already exists '%d'", dwUin);
						break;
					}
					hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
				}

				if (hContact == NULL)
				{
					/* not already on list: add */
					Netlib_Logf(ghServerNetlibUser, "SSI adding new Permit contact '%d'", dwUin);
					hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD,0,0);
					if (!hContact)
					{
						Netlib_Logf(ghServerNetlibUser, "Failed to create ICQ contact %u", dwUin);
						if (pChain)
							disposeChain(&pChain);
						continue;
					}
					if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)gpszICQProtoName) != 0)
					{
						// For some reason we failed to register the protocol to this contact
						CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
						Netlib_Logf(ghServerNetlibUser, "Failed to register ICQ contact %u", dwUin);
						if (pChain)
							disposeChain(&pChain);
						continue;
					}
					DBWriteContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, dwUin);
					// It wasn't previously in the list, we hide it so it only appears in the visible list
					DBWriteContactSettingByte(hContact, "CList", "Hidden", 1);
				}

				// Save permit ID
				DBWriteContactSettingWord(hContact, gpszICQProtoName, "SrvPermitId", wItemId);
				ReserveServerID(wItemId);
				// Set apparent mode
				DBWriteContactSettingWord(hContact, gpszICQProtoName, "ApparentMode", ID_STATUS_ONLINE);
				Netlib_Logf(ghServerNetlibUser, "Visible-contact (%u)", dwUin);

			}
			break;

		case SSI_ITEM_DENY: // Item on invisible list
			if (!IsStringUIN(pszRecordName))
			{
				Netlib_Logf(ghServerNetlibUser, "Ignoring fake AOL contact, message is: \"%s\"", pszRecordName);
			}
			else
			{

				DWORD dwUin;
				char* pszProto;
				HANDLE hContact;


				// This looks like a real contact
				// Check if this user already exists in local list
				dwUin = atoi(pszRecordName);
				hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
				while (hContact)
				{
					pszProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
					if (pszProto && !strcmp(pszProto, gpszICQProtoName) && (dwUin == DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0)))
					{
						Netlib_Logf(ghServerNetlibUser, "Deny contact already exists '%d'", dwUin);
						break;
					}
					hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
				}

				if (hContact == NULL)
				{
					/* not already on list: add */
					Netlib_Logf(ghServerNetlibUser, "SSI adding new Deny contact '%d'", dwUin);
					hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD,0,0);
					if (!hContact)
					{
						Netlib_Logf(ghServerNetlibUser, "Failed to create ICQ contact %u", dwUin);
						if (pChain)
							disposeChain(&pChain);
						continue;
					}
					if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)gpszICQProtoName) != 0)
					{
						// For some reason we failed to register the protocol to this contact
						CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
						Netlib_Logf(ghServerNetlibUser, "Failed to register ICQ contact %u", dwUin);
						if (pChain)
							disposeChain(&pChain);
						continue;
					}
					DBWriteContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, dwUin);
					// It wasn't previously in the list, we hide it so it only appears in the visible list
					DBWriteContactSettingByte(hContact, "CList", "Hidden", 1);
				}

				// Save Deny ID
				DBWriteContactSettingWord(hContact, gpszICQProtoName, "SrvDenyId", wItemId);
				ReserveServerID(wItemId);

				// Set apparent mode
				DBWriteContactSettingWord(hContact, gpszICQProtoName, "ApparentMode", ID_STATUS_OFFLINE);
				Netlib_Logf(ghServerNetlibUser, "Invisible-contact (%u)", dwUin);
			}
			break;


		case SSI_ITEM_VISIBILITY: /* My visibility settings */
			{

				BYTE bVisibility;


				ReserveServerID(wItemId);

				// Look for visibility TLV
				if (bVisibility = getByteFromChain(pChain, 0x00CA, 1))
				{
					DBWriteContactSettingWord(NULL, gpszICQProtoName, "SrvVisibilityID", wItemId);
					Netlib_Logf(ghServerNetlibUser, "Visibility is %u, ID is %u", bVisibility, wItemId);
				}
			}
			break;

		case SSI_ITEM_IGNORE:
			/* item on ignore list */
			/* pszRecordName is the UIN */
			/* TODO */

			ReserveServerID(wItemId);

			if (!IsStringUIN(pszRecordName))
				Netlib_Logf(ghServerNetlibUser, "Ignoring fake AOL contact, message is: \"%s\"", pszRecordName);
			else
				Netlib_Logf(ghServerNetlibUser, "Ignored-contact ignored (%s)", pszRecordName);
			break;

		case SSI_ITEM_UNKNOWN2:
			Netlib_Logf(ghServerNetlibUser, "SSI unknown type 0x11");
			break;

		case SSI_ITEM_IMPORT:
			if ((wGroupId == 0) && (wItemId == 1))
			{
				/* time our list was first imported */
				/* pszRecordName is "Import Time" */
				/* data is TLV(13) {TLV(D4) {time_t importTime}} */
				/* not processed yet (if ever) */
				Netlib_Logf(ghServerNetlibUser, "SSI first import seen");
			}
			break;

		case SSI_ITEM_UNKNOWN1:
		default:
			Netlib_Logf(ghServerNetlibUser, "SSI unhandled item %2x", wTlvType);
			break;

		}

		if (pChain)
			disposeChain(&pChain);

	} // end for


	Netlib_Logf(ghServerNetlibUser, "Bytes left: %u", wLen);

	SAFE_FREE(pszRecordName);

	if (wDefaultGroupId)
	{
		DBWriteContactSettingWord(NULL, gpszICQProtoName, "SrvDefGroupId", wDefaultGroupId);
#ifdef _DEBUG
		Netlib_Logf(ghServerNetlibUser, "Default group is %u", wDefaultGroupId);
#endif
	}

	DBWriteContactSettingWord(NULL, gpszICQProtoName, "SrvRecordCount", (WORD)(wRecord + DBGetContactSettingWord(NULL, gpszICQProtoName, "SrvRecordCount", 0)));


	if (bIsLastPacket)
	{

		// No contacts left to sync
		bIsSyncingCL = FALSE;

		if (wLen >= 4)
		{

			DWORD dwLastUpdateTime;


			/* finally we get a time_t of the last update time */
			unpackDWord(&buf, &dwLastUpdateTime);
			DBWriteContactSettingDword(NULL, gpszICQProtoName, "SrvLastUpdate", dwLastUpdateTime);
			Netlib_Logf(ghServerNetlibUser, "Last update of server list was (%u) %s", dwLastUpdateTime, asctime(localtime(&dwLastUpdateTime)));

			sendRosterAck();
			handleServUINSettings(wListenPort, dwLocalInternalIP);
		}
		else
		{
			Netlib_Logf(ghServerNetlibUser, "Last packet missed update time...");
		}
	}
	else
	{
		Netlib_Logf(ghServerNetlibUser, "Waiting for more packets");
	}

}



static void handleRecvAuthRequest(unsigned char *buf, WORD wLen)
{

	BYTE nUinLen;
	BYTE szUin[10];
	WORD wReasonLen;
	DWORD dwUin;
	HANDLE hcontact;
	CCSDATA ccs;
	PROTORECVEVENT pre;
	char* szBlob;
	char* pCurBlob;


	unpackByte(&buf, &nUinLen);
	wLen -= 1;

	if (nUinLen > wLen)
		return;

	unpackString(&buf, szUin, nUinLen);
	wLen -= nUinLen;
	szUin[nUinLen] = '\0';

	if (!IsStringUIN(szUin))
		return;

	dwUin = atoi(szUin);

	unpackWord(&buf, &wReasonLen);
	wLen -= 2;
	if (wReasonLen > wLen)
		return;

	ccs.szProtoService=PSR_AUTH;
	ccs.hContact=hcontact=HContactFromUIN(dwUin,1);
	ccs.wParam=0;
	ccs.lParam=(LPARAM)&pre;
	pre.flags=0;
	pre.timestamp=time(NULL);
	pre.lParam=sizeof(DWORD)+sizeof(HANDLE)+wReasonLen+5;

	/*blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)*/
	pCurBlob=szBlob=(char *)malloc(pre.lParam);
	memcpy(pCurBlob,&dwUin,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
	memcpy(pCurBlob,&hcontact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
	*(char *)pCurBlob = 0; pCurBlob++;
	*(char *)pCurBlob = 0; pCurBlob++;
	*(char *)pCurBlob = 0; pCurBlob++;
	*(char *)pCurBlob = 0; pCurBlob++;
	memcpy(pCurBlob, buf, wReasonLen); pCurBlob += wReasonLen;
	*(char *)pCurBlob = 0;
	pre.szMessage=(char *)szBlob;
// TODO: Change for new auth system, include all known informations

	CallService(MS_PROTO_CHAINRECV,0,(LPARAM)&ccs);

}



static void handleRecvAdded(unsigned char *buf, WORD wLen)
{

	BYTE nUinLen;
	BYTE szUin[10];
	DWORD dwUin;
	DBEVENTINFO dbei;
	PBYTE pCurBlob;
	HANDLE hContact;


	unpackByte(&buf, &nUinLen);
	wLen -= 1;

	if (nUinLen > wLen)
		return;

	unpackString(&buf, szUin, nUinLen);
	wLen -= nUinLen;
	szUin[nUinLen] = '\0';

	if (!IsStringUIN(szUin))
		return;

	dwUin = atoi(szUin);
	hContact=HContactFromUIN(dwUin,1);


	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.szModule=gpszICQProtoName;
	dbei.timestamp=time(NULL);
	dbei.flags=0;
	dbei.eventType=EVENTTYPE_ADDED;
	dbei.cbBlob=sizeof(DWORD)+sizeof(HANDLE)+4;
	pCurBlob=dbei.pBlob=(PBYTE)malloc(dbei.cbBlob);
	/*blob is: uin(DWORD), hContact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ) */
	memcpy(pCurBlob,&dwUin,sizeof(DWORD)); pCurBlob+=sizeof(DWORD);
	memcpy(pCurBlob,&hContact,sizeof(HANDLE)); pCurBlob+=sizeof(HANDLE);
	*(char *)pCurBlob = 0; pCurBlob++;
	*(char *)pCurBlob = 0; pCurBlob++;
	*(char *)pCurBlob = 0; pCurBlob++;
	*(char *)pCurBlob = 0;
// TODO: Change for new auth system

	CallService(MS_DB_EVENT_ADD,(WPARAM)(HANDLE)NULL,(LPARAM)&dbei);

}



static void handleRecvAuthResponse(unsigned char *buf, WORD wLen)
{

	BYTE nUinLen;
	BYTE bResponse;
	BYTE szUin[10];
	DWORD dwUin;
	HANDLE hContact;
	char* szNick;
	WORD nReasonLen;
	char* szReason;

	bResponse = 0xFF;

	unpackByte(&buf, &nUinLen);
	wLen -= 1;

	if (nUinLen > wLen)
		return;

	unpackString(&buf, szUin, nUinLen);
	wLen -= nUinLen;
	szUin[nUinLen] = '\0';

	if (!IsStringUIN(szUin))
		return;

	dwUin = atoi(szUin);
	hContact = HContactFromUIN(dwUin, 1);

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
			szReason = malloc(nReasonLen+1);
			unpackString(&buf, szReason, nReasonLen);
			szReason[nReasonLen] = '\0';
		}
	}

	switch (bResponse)
	{

	case 0:
		DBWriteContactSettingByte(hContact, gpszICQProtoName, "Auth", 1);
		Netlib_Logf(ghServerNetlibUser, "Authorisation request denied by %u", dwUin);
		// TODO: Add to system history as soon as new auth system is ready
		break;

	case 1:
		DBWriteContactSettingByte(hContact, gpszICQProtoName, "Auth", 0);
		Netlib_Logf(ghServerNetlibUser, "Authorisation request granted by %u", dwUin);
		// TODO: Add to system history as soon as new auth system is ready
		break;

	default:
		Netlib_Logf(ghServerNetlibUser, "Unknown Authorisation request response (%u) from %u", bResponse, dwUin);
		break;

	}
	SAFE_FREE(szNick);
	SAFE_FREE(szReason);
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

		// Do we have a known server visibility ID?
		if ((wVisibilityID = DBGetContactSettingWord(NULL, gpszICQProtoName, "SrvVisibilityID", 0)) == 0)
		{
			// No, create a new random ID
			wVisibilityID = GenerateServerId();
			DBWriteContactSettingWord(NULL, gpszICQProtoName, "SrvVisibilityID", wVisibilityID);
			wCommand = ICQ_LISTS_ADDTOLIST;
			Netlib_Logf(ghServerNetlibUser, "Made new srvvisID, id is %u, code is %u", wVisibilityID, bCode);
		}
		else
		{
			Netlib_Logf(ghServerNetlibUser, "Reused srvvisID, id is %u, code is %u", wVisibilityID, bCode);
			wCommand = ICQ_LISTS_UPDATEGROUP;
		}


		// Build and send packet
		packet.wLen = 25;
		write_flap(&packet, ICQ_DATA_CHAN);
		packFNACHeader(&packet, ICQ_LISTS_FAMILY, wCommand, 0, wCommand);
		packWord(&packet, 0);                   // Name (null)
		packWord(&packet, 0);                   // GroupID (0 if not relevant)
		packWord(&packet, wVisibilityID);       // EntryID
		packWord(&packet, SSI_ITEM_VISIBILITY); // EntryType
		packWord(&packet, 5);                   // Length in bytes of following TLV
		packWord(&packet, 0xCA);                // TLV Type
		packWord(&packet, 0x1);                 // TLV Length
		packByte(&packet, bCode);               // TLV Value (visibility code)
		sendServPacket(&packet);
		// There is no need to send ICQ_LISTS_CLI_MODIFYSTART or
		// ICQ_LISTS_CLI_MODIFYEND when modifying the visibility code

	}

}



// Should be called before the server list is modified. When all
// modifications are done, call sendAddEnd().
void sendAddStart(void)
{

	icq_packet packet;


	packet.wLen = 10;
	write_flap(&packet, ICQ_DATA_CHAN);
	packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_CLI_MODIFYSTART, 0, ICQ_LISTS_CLI_MODIFYSTART);
	sendServPacket(&packet);

}



// Should be called after the server list has been modified to inform
// the server that we are done.
void sendAddEnd(void)
{

	icq_packet packet;


	packet.wLen = 10;
	write_flap(&packet, ICQ_DATA_CHAN);
	packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_CLI_MODIFYEND, 0, ICQ_LISTS_CLI_MODIFYEND);
	sendServPacket(&packet);

}



// Is called when a contact has been renamed locally to update
// the server side nick name.
WORD renameServContact(HANDLE hContact, const char *pszNick)
{

	icq_packet packet;
	WORD wGroupID;
	WORD wItemID;
	DWORD dwUin;
	char szUin[10];
	int nUinLen;
	int nNickLen = 0;
	char* pszUtfNick = NULL;
	WORD wSequence = 0;
	BOOL bAuthRequired;


	// Get the contact's group ID
	if (!(wGroupID = DBGetContactSettingWord(hContact, gpszICQProtoName, "SrvGroupId", 0)))
	{
		if (!(wGroupID = DBGetContactSettingWord(NULL, gpszICQProtoName, "SrvDefGroupId", 0)))
		{
			// Could not find a usable group ID
			Netlib_Logf(ghServerNetlibUser, "Failed to upload new nick name to server side list (no group ID");
			return 0;
		}
	}

	// Get the contact's item ID
	if (!(wItemID = DBGetContactSettingWord(hContact, gpszICQProtoName, "ServerId", 0)))
	{
		if (!(wItemID = DBGetContactSettingWord(hContact, gpszICQProtoName, "SrvDenyId", 0)))
		{
			if (!(wItemID = DBGetContactSettingWord(hContact, gpszICQProtoName, "SrvPermitId", 0)))
			{
				// Could not find usable item ID
				Netlib_Logf(ghServerNetlibUser, "Failed to upload new nick name to server side list (no item ID");
				return 0;
			}
		}
	}


	// Check if contact is authorized
	bAuthRequired = (DBGetContactSettingByte(hContact, gpszICQProtoName, "Auth", 0) == 1);


	// Make UIN string
	dwUin = DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0);
	_itoa(dwUin, szUin, 10);
	nUinLen = strlen(szUin);


	// Convert to utf
	if (pszNick && (strlen(pszNick) > 0))
	{
		utf8_encode(pszNick, &pszUtfNick);
		if (pszUtfNick)
			nNickLen = strlen(pszUtfNick);
	}


	// Pack packet header
	wSequence = GenerateCookie();
	packet.wLen = nUinLen + 20;
	if (nNickLen > 0)
		packet.wLen += 4 + nNickLen;
	if (bAuthRequired)
		packet.wLen += 4;
	write_flap(&packet, ICQ_DATA_CHAN);
	// TODO:
	// The sequence we pack here should be a combination of ICQ_LISTS_UPDATEGROUP & wSequence.
	// For example, ICQ2003b sends 0x00030009
	packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_UPDATEGROUP, 0, wSequence);
	packWord(&packet, (WORD)nUinLen);
	packBuffer(&packet, szUin, (WORD)nUinLen);
	packWord(&packet, wGroupID);
	packWord(&packet, wItemID);
	packWord(&packet, 0);	     // Entry type (0 = contact)


	// Pack contact data
	{

		WORD wBytesToPack = 0;


		if (nNickLen > 0)
			wBytesToPack += 4 + nNickLen;

		if (bAuthRequired)
			wBytesToPack += 4;

		if (wBytesToPack > 0)
		{
			packWord(&packet, wBytesToPack);    // Payload length
			if (bAuthRequired)
				packDWord(&packet, 0x00660000); // TLV(0x66)
			if (nNickLen > 0)
			{
				packWord(&packet, 0x0131);	    // TLV(0x0131)
				packWord(&packet, (WORD)nNickLen);
				packBuffer(&packet, pszUtfNick, (WORD)nNickLen);
			}
		}
	}

	// There is no need to send ICQ_LISTS_CLI_MODIFYSTART or
	// ICQ_LISTS_CLI_MODIFYEND when just changing nick name

	sendServPacket(&packet);

	if (nNickLen > 0)
		Netlib_Logf(ghServerNetlibUser, "Sent SNAC(x13,x09) - CLI_UPDATEGROUP, renaming %u to %s (seq: %u)", dwUin, pszNick, wSequence);
	else
		Netlib_Logf(ghServerNetlibUser, "Sent SNAC(x13,x09) - CLI_UPDATEGROUP, removed nickname for %u (seq: %u)", dwUin, wSequence);

	SAFE_FREE(pszUtfNick);

	return wSequence;

}



// Sent when the last roster packet has been received
void sendRosterAck(void)
{

	icq_packet packet;


	packet.wLen = 10;
	write_flap(&packet, ICQ_DATA_CHAN);
	packFNACHeader(&packet, ICQ_LISTS_FAMILY, ICQ_LISTS_GOTLIST, 0, ICQ_LISTS_GOTLIST);
	sendServPacket(&packet);

#ifdef _DEBUG
	Netlib_Logf(ghServerNetlibUser, "Sent SNAC(x13,x07) - CLI_ROSTERACK");
#endif

}
