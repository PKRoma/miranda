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



typedef struct directthreadstartinfo_t
{
	int type;           // Only valid for outgoing connections
	int incoming;       // 1=incoming, 0=outgoing
	HANDLE hConnection;	// only valid for incoming connections, handle to the connection
	HANDLE hContact;	// Only valid for outgoing connections
	void* pvExtra;      // Only valid for outgoing connections
} directthreadstartinfo;

static unsigned char client_check_data[] = {
  "As part of this software beta version Mirabilis is "
  "granting a limited access to the ICQ network, "
  "servers, directories, listings, information and databases (\""
  "ICQ Services and Information\"). The "
  "ICQ Service and Information may databases (\""
  "ICQ Services and Information\"). The "
  "ICQ Service and Information may\0"
};

static directconnect** directConnList = NULL;
static int directConnCount = 0;
static int mutexesInited = 0;
static CRITICAL_SECTION directConnListMutex;
static CRITICAL_SECTION expectedFileRecvMutex;
static int expectedFileRecvCount = 0;
static filetransfer** expectedFileRecv = NULL;

extern WORD wListenPort;
extern DWORD dwLocalInternalIP, dwLocalExternalIP;
extern DWORD dwLocalDirectConnCookie;
extern DWORD dwLocalUIN;
extern HANDLE hDirectNetlibUser;
extern char gpszICQProtoName[MAX_PATH];

static void handleDirectPacket(directconnect* dc, PBYTE buf, WORD wLen);
static DWORD __stdcall icq_directThread(directthreadstartinfo* dtsi);
static void startHandshake_v78(directconnect* dc);
static int DecryptDirectPacket(directconnect* dc, PBYTE buf, WORD wLen);
static void sendPeerInitAck(directconnect* dc);



void InitDirectConns(void)
{

	if (!mutexesInited)
	{
		mutexesInited = 1;
		InitializeCriticalSection(&directConnListMutex);
		InitializeCriticalSection(&expectedFileRecvMutex);
	}
	directConnCount = 0;
	
}



void UninitDirectConns(void)
{

	int i;
	

	EnterCriticalSection(&directConnListMutex);

	for (i = 0; i < directConnCount; i++)
	{
		if (directConnList[i])
			Netlib_CloseHandle(directConnList[i]->hConnection);
	}

	LeaveCriticalSection(&directConnListMutex);

	for(;;)
	{
		for (i = 0; i < directConnCount; i++)
		{
			if (directConnList[i])
				break;
		}

		if (i == directConnCount)
			break;

		Sleep(10);	   /* yeah, ugly */
	}

	DeleteCriticalSection(&directConnListMutex);
	DeleteCriticalSection(&expectedFileRecvMutex);

	SAFE_FREE(directConnList);

}



static void AddDirectConnToList(directconnect* dc)
{

	int i;


	EnterCriticalSection(&directConnListMutex);

	for (i = 0; i < directConnCount; i++)
	{
		if (directConnList[i] == NULL)
			break;
	}

	if (i == directConnCount)
		directConnList = (directconnect**)realloc(directConnList, sizeof(directconnect*) * ++directConnCount);
	directConnList[i] = dc;

	LeaveCriticalSection(&directConnListMutex);

}



static RemoveDirectConnFromList(directconnect* dc)
{

	int i;


	EnterCriticalSection(&directConnListMutex);

	for (i = 0; i < directConnCount; i++)
	{
		if (directConnList[i] == dc)
		{
			directConnList[i] = NULL;
			break;
		}
	}

	LeaveCriticalSection(&directConnListMutex);

}



void AddExpectedFileRecv(filetransfer* ft)
{

	EnterCriticalSection(&expectedFileRecvMutex);

	expectedFileRecv = (filetransfer** )realloc(expectedFileRecv, sizeof(filetransfer *) * (expectedFileRecvCount + 1));
	expectedFileRecv[expectedFileRecvCount++] = ft;

	LeaveCriticalSection(&expectedFileRecvMutex);

}



filetransfer *FindExpectedFileRecv(DWORD dwUin, DWORD dwTotalSize)
{

	int i;
	filetransfer* pFt = NULL;


	EnterCriticalSection(&expectedFileRecvMutex);

	for (i = 0; i < expectedFileRecvCount; i++)
	{
		if (expectedFileRecv[i]->dwUin == dwUin && expectedFileRecv[i]->dwTotalSize == dwTotalSize)
		{

			pFt = expectedFileRecv[i];
			expectedFileRecvCount--;
			memmove(expectedFileRecv + i, expectedFileRecv + i + 1, sizeof(filetransfer*) * (expectedFileRecvCount - i));
			expectedFileRecv = (filetransfer**)realloc(expectedFileRecv, sizeof(filetransfer*) * expectedFileRecvCount);

			// Filereceive found, exit loop
			break;
		}
	}

	LeaveCriticalSection(&expectedFileRecvMutex);

	return pFt;

}



int sendDirectPacket(HANDLE hConnection, icq_packet* pkt)
{

	int nResult;


	nResult = Netlib_Send(hConnection, (const char*)pkt->pData, pkt->wLen + 2, 0);

	if (nResult == SOCKET_ERROR)
	{
		Netlib_Logf(hDirectNetlibUser, "Direct %p socket error: %d, closing", hConnection, GetLastError());
		Netlib_CloseHandle(hConnection);
	}
	
	SAFE_FREE(pkt->pData);

	return nResult;

}



// Check if we have an open and initialized DC with type
// 'type' to the specified contact
BOOL IsDirectConnectionOpen(HANDLE hContact, int type)
{
	
	int i;
	BOOL bIsOpen = FALSE;
	
	
	EnterCriticalSection(&directConnListMutex);
	
	for (i = 0; i < directConnCount; i++)
	{
		if (directConnList[i] && (directConnList[i]->type == type))
		{
			if ((HContactFromUIN(directConnList[i]->dwRemoteUin, 0) == hContact)
				&& (directConnList[i]->initialised))
			{
				// Connection is OK
				bIsOpen = TRUE;
				break;
			}
		}
	}
	
	LeaveCriticalSection(&directConnListMutex);
	
	return bIsOpen;
	
}



// This function is called from the Netlib when someone is connecting to
// one of our incomming DC ports
void icq_newConnectionReceived(HANDLE hNewConnection, DWORD dwRemoteIP)
{

	directthreadstartinfo* dtsi;
	pthread_t tid;


	// Start a new thread for the incomming connection
	dtsi = (directthreadstartinfo*)malloc(sizeof(directthreadstartinfo));
	dtsi->hConnection = hNewConnection;
	dtsi->incoming = 1;

	tid.hThread = (HANDLE)forkthreadex(NULL, 0, icq_directThread, dtsi, 0, &tid.dwThreadId);
	CloseHandle(tid.hThread);

}



// Called from icq_AcceptFileTransfer to open a standard DC and send a 'file accept'
// Called from icq_InitFileSend to open a standard DC and send a 'file send request'
// Called from handleMessageAck to open a file DC when a file send request has been accepted
void OpenDirectConnection(HANDLE hContact, int type, void* pvExtra)
{

	pthread_t tid;
	directthreadstartinfo* dtsi;


	// If a standard connection is requested, we try to reuse exising ones
	if (type == DIRECTCONN_STANDARD)
	{

		int i;


		Netlib_Logf(hDirectNetlibUser, "OpenDirectConnection: Connection is standard");

		EnterCriticalSection(&directConnListMutex);

		for (i = 0; i < directConnCount; i++)
		{

			if (directConnList[i] == NULL)
				continue;

			if (HContactFromUIN(directConnList[i]->dwRemoteUin, 0) == hContact)
			{
				if (directConnList[i]->initialised)
				{

					// This connection can be reused, send packet and exit
					Netlib_Logf(hDirectNetlibUser, "OpenDirectConnection: Using existing connection");
					if (pvExtra)
					{
						if (((icq_packet*)pvExtra)->pData[2] == 2)
							EncryptDirectPacket(directConnList[i], (icq_packet*)pvExtra);

						sendDirectPacket(directConnList[i]->hConnection, (icq_packet*)pvExtra);
					}

					LeaveCriticalSection(&directConnListMutex);
					return;

				}

				/// lazy: connection is not inited yet, open a new one
				break;

			}
		}
		
		LeaveCriticalSection(&directConnListMutex);

	}


	// Create a new connection
	dtsi = (directthreadstartinfo*)malloc(sizeof(directthreadstartinfo));
	dtsi->type = type;
	dtsi->incoming = 0;
	dtsi->hContact = hContact;
	if (dtsi->type == DIRECTCONN_STANDARD)
	{
		dtsi->pvExtra = malloc(sizeof(icq_packet));
		memcpy(dtsi->pvExtra, pvExtra, sizeof(icq_packet));
	}
	else
	{
		dtsi->pvExtra = pvExtra;
	}

	tid.hThread = (HANDLE)forkthreadex(NULL, 0, icq_directThread, dtsi, 0, &tid.dwThreadId);
	CloseHandle(tid.hThread);

}



// Called from icq_newConnectionReceived when a new incomming dc is done
// Called from OpenDirectConnection when a new outgoing dc is done
static DWORD __stdcall icq_directThread(directthreadstartinfo *dtsi)
{

	WORD wLen;
	int i;
	directconnect dc;
	NETLIBPACKETRECVER packetRecv={0};
	HANDLE hPacketRecver;
	int recvResult;


	srand(time(NULL));
	AddDirectConnToList(&dc);


	// Initialize DC struct
	dc.dwThreadId = GetCurrentThreadId();
	dc.incoming = dtsi->incoming;
	dc.hConnection = dtsi->hConnection;
	dc.packetToSend = NULL;
	dc.ft = NULL;

	if (!dc.incoming)
	{

		dc.type = dtsi->type;
		dc.dwRemoteExternalIP = DBGetContactSettingDword(dtsi->hContact, gpszICQProtoName, "IP", 0);
		dc.dwRemoteInternalIP = DBGetContactSettingDword(dtsi->hContact, gpszICQProtoName, "RealIP", 0);
		dc.dwRemotePort = DBGetContactSettingWord(dtsi->hContact, gpszICQProtoName, "UserPort", 0);
		dc.dwRemoteUin = DBGetContactSettingDword(dtsi->hContact, gpszICQProtoName, UNIQUEIDSETTING, 0);
		dc.wVersion = DBGetContactSettingWord(dtsi->hContact, gpszICQProtoName, "Version", 0);
		dc.dwConnCookie = DBGetContactSettingDword(dtsi->hContact, gpszICQProtoName, "DirectCookie", 0);

		if (dc.type == DIRECTCONN_STANDARD)
		{
			dc.packetToSend = (icq_packet*)dtsi->pvExtra;
		}
		else if (dc.type == DIRECTCONN_FILE)
		{
			dc.ft = (filetransfer*)dtsi->pvExtra;
			dc.dwRemotePort = dc.ft->dwRemotePort;
		}

	}
	else
	{
		dc.type = DIRECTCONN_STANDARD;
	}


	SAFE_FREE(dtsi);
	dc.initialised = 0;
	dc.wantIdleTime = 0;


	// Create outgoing DC
	if (!dc.incoming)
	{

		NETLIBOPENCONNECTION nloc = {0};
		IN_ADDR addr;


		nloc.cbSize = sizeof(nloc);
		nloc.flags = 0;
		if (dc.dwRemoteExternalIP == dwLocalExternalIP)
			addr.S_un.S_addr = htonl(dc.dwRemoteInternalIP);
		else
			addr.S_un.S_addr = htonl(dc.dwRemoteExternalIP);
		nloc.szHost = inet_ntoa(addr);
		nloc.wPort = (WORD)dc.dwRemotePort;
		Netlib_Logf(hDirectNetlibUser, "Direct: connecting to %s:%u", nloc.szHost, nloc.wPort);

		dc.hConnection = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)hDirectNetlibUser, (LPARAM)&nloc);

		if (dc.hConnection == NULL)
		{
			Netlib_Logf(hDirectNetlibUser, "Direct: connect() failed (%d)", GetLastError());
			RemoveDirectConnFromList(&dc);
			if (dc.type==DIRECTCONN_FILE) ProtoBroadcastAck(gpszICQProtoName, dc.ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, dc.ft, 0);
			return 0;
		}

		if (dc.type == DIRECTCONN_FILE)
			dc.ft->hConnection = dc.hConnection;


		if (dc.wVersion > 6)
		{
			startHandshake_v78(&dc);
		}
		else
		{
			Netlib_Logf(hDirectNetlibUser, "Unsupported direct protocol: %d, closing connection", dc.wVersion);
			Netlib_CloseHandle(dc.hConnection);
			RemoveDirectConnFromList(&dc);

			return 0;
		}
	}


	hPacketRecver = (HANDLE)CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)dc.hConnection, 8192);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.bytesUsed = 0;


	// Packet receiving loop

	while(dc.hConnection)
	{
		packetRecv.dwTimeout = dc.wantIdleTime ? 0 : 600000;

		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM)hPacketRecver, (LPARAM)&packetRecv);
		if (recvResult == 0)
		{
			Netlib_Logf(hDirectNetlibUser, "Clean closure of direct socket (%p)", dc.hConnection);
			break;
		}

		if (recvResult == SOCKET_ERROR)
		{
			if (GetLastError() == ERROR_TIMEOUT)
			{
				if (dc.wantIdleTime)
				{
					switch (dc.type)
					{
						case DIRECTCONN_FILE:
							handleFileTransferIdle(&dc);
							break;
					}
				}
				else
				{
					Netlib_Logf(hDirectNetlibUser, "Direct: connection inactive for 10 minutes. Closing.");
					break;
				}
			}
			else
			{
				Netlib_Logf(hDirectNetlibUser, "Abortive closure of direct socket (%p) (%d)", dc.hConnection, GetLastError());
				break;
			}
		}


		if (dc.type == DIRECTCONN_CLOSING)
			packetRecv.bytesUsed = packetRecv.bytesAvailable;
		else
		{
			for (i = 0; i + 2 <= packetRecv.bytesAvailable;)
			{
				wLen = *(WORD*)(packetRecv.buffer + i);
				if (wLen + 2 + i > packetRecv.bytesAvailable)
					break;

				if (dc.type == DIRECTCONN_STANDARD && wLen && packetRecv.buffer[i + 2] == 2)
				{
					if (!DecryptDirectPacket(&dc, packetRecv.buffer + i + 3, (WORD)(wLen - 1)))
					{
						Netlib_Logf(hDirectNetlibUser, "Direct: Corrupted packet encryption, ignoring packet");
						i += wLen + 2;
						continue;
					}
				}

				Netlib_Logf(hDirectNetlibUser, "Direct: new direct package");

				if (dc.type == DIRECTCONN_FILE && dc.initialised)
					handleFileTransferPacket(&dc, packetRecv.buffer + i + 2, wLen);
				else
					handleDirectPacket(&dc, packetRecv.buffer + i + 2, wLen);

				i += wLen + 2;
			}
			packetRecv.bytesUsed = i;
		}
	}

	// End of packet receiving loop


	Netlib_CloseHandle(hPacketRecver);
	Netlib_Logf(hDirectNetlibUser, "Direct conn closed (%p)", dc.hConnection);

	if(dc.hConnection)
		Netlib_CloseHandle(dc.hConnection);

	if (dc.ft)
	{
		if (dc.ft->fileId != -1)
		{
			_close(dc.ft->fileId);
			ProtoBroadcastAck(gpszICQProtoName, dc.ft->hContact, ACKTYPE_FILE, dc.ft->dwBytesDone==dc.ft->dwTotalSize ? ACKRESULT_SUCCESS : ACKRESULT_FAILED, dc.ft, 0);
		}
		else if(dc.ft->hConnection)
			ProtoBroadcastAck(gpszICQProtoName, dc.ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, dc.ft, 0);
		SAFE_FREE(dc.ft->szFilename);
		SAFE_FREE(dc.ft->szDescription);
		SAFE_FREE(dc.ft->szSavePath);
		SAFE_FREE(dc.ft->szThisFile);
		SAFE_FREE(dc.ft->szThisSubdir);
		if (dc.ft->files)
		{
			int i;
			for (i = 0; i < (int)dc.ft->dwFileCount; i++)
				SAFE_FREE(dc.ft->files[i]);
			SAFE_FREE(dc.ft->files);
		}
		SAFE_FREE(dc.ft);
		_chdir("\\");		/* so we don't leave a subdir handle open so it can't be deleted */
	}

	RemoveDirectConnFromList(&dc);

	return 0;

}



static void handleDirectPacket(directconnect* dc, PBYTE buf, WORD wLen)
{

	if (wLen < 1)
		return;


	switch (buf[0])
	{

		case 0:   /* first packet of a file transfer */
			Netlib_Logf(hDirectNetlibUser, "Received PEER_FILE_INIT from %u",dc->dwRemoteUin);
			handleFileTransferPacket(dc, buf, wLen);
			break;

		case 1:
			/* PEER_INIT_ACK. 4 bytes: 01 00 00 00. Sent during connect */
			// This is sent as a response to our PEER_INIT packet
			if (wLen != 4)
			{
				Netlib_Logf(hDirectNetlibUser, "Error: Received malformed PEER_INITACK from %u", dc->dwRemoteUin);
				break;
			}


			if (!dc->incoming)
			{
				Netlib_Logf(hDirectNetlibUser, "Received PEER_INITACK DC from %u on outgoing DC", dc->dwRemoteUin);
				sendPeerInitAck(dc);
			}
			else
			{
				Netlib_Logf(hDirectNetlibUser, "Error: Received PEER_INITACK DC from %u on INCOMING DC", dc->dwRemoteUin);
				Netlib_Logf(hDirectNetlibUser, "Connection established?");
			}
			break;

		case 0xFF:	   /* connect packet */
			Netlib_Logf(hDirectNetlibUser, "Received PEER_INIT from %u",dc->dwRemoteUin);
			buf++;

			if (wLen < 3)
				return;
			
			unpackLEWord(&buf, &dc->wVersion);

			if (dc->wVersion > 6)
			{

				WORD wSecondLen;
				DWORD dwUin;
				DWORD dwPort;
				DWORD dwCookie;
				icq_packet packet;
				HANDLE hContact;


				if (wLen != 0x30)
					return;

				unpackLEWord(&buf, &wSecondLen);
				if (wSecondLen != 0x2b)
					return;
				
				unpackLEDWord(&buf, &dwUin);
				if (dwUin != dwLocalUIN)
					return;
				
				buf += 2;    /* 00 00 */
				unpackLEDWord(&buf, &dc->dwRemotePort);
				unpackLEDWord(&buf, &dc->dwRemoteUin);
				unpackDWord(&buf, &dc->dwRemoteExternalIP);
				unpackDWord(&buf, &dc->dwRemoteInternalIP);
				buf ++;     /* 04: accept direct connections */
				unpackLEDWord(&buf, &dwPort);
				if (dwPort != dc->dwRemotePort)
					return;
				unpackLEDWord(&buf, &dwCookie);

				/* 12 more bytes of unknown stuff */

				hContact = HContactFromUIN(dc->dwRemoteUin, 0);
				if (hContact == INVALID_HANDLE_VALUE)
					return;	   /* don't allow direct connection with people not on my clist */


				if (dwCookie != DBGetContactSettingDword(hContact, gpszICQProtoName, "DirectCookie", 0))
					return;
				

				if (dc->incoming)
				{
					directPacketInit(&packet, 4);
					packLEDWord(&packet, 1);
					sendDirectPacket(dc->hConnection, &packet);

					directPacketInit(&packet, 0x30);
					packByte(&packet, 0xff);
					packLEWord(&packet, dc->wVersion);
					packLEWord(&packet, 0x2b);      /* length again */
					packLEDWord(&packet, dc->dwRemoteUin);
					packWord(&packet, 0);
					packLEDWord(&packet, wListenPort);
					packLEDWord(&packet, dwLocalUIN);
					packDWord(&packet, dwLocalExternalIP);
					packDWord(&packet, dwLocalInternalIP);
					packByte(&packet, 4);       /* accept direct conn */
					packLEDWord(&packet, wListenPort);
					packLEDWord(&packet, dwCookie);
					packLEDWord(&packet, WEBFRONTPORT);
					packLEDWord(&packet, 3);
					packDWord(&packet, 0);
					sendDirectPacket(dc->hConnection, &packet);
				}
				else	 /* outgoing */
				{
					if (dc->type == DIRECTCONN_STANDARD)
					{
						directPacketInit(&packet, 33);
						packByte(&packet, 3);
						packLEDWord(&packet, 10);
						packLEDWord(&packet, 1);
						packLEDWord(&packet, 0);
						packLEDWord(&packet, 0);
						packLEDWord(&packet, 0);
						packLEDWord(&packet, 0);
						packLEDWord(&packet, 0);
						packLEWord(&packet, 1);
						packLEWord(&packet, 4);
						sendDirectPacket(dc->hConnection, &packet);
					}
					else if(dc->type == DIRECTCONN_FILE)
					{

						DBVARIANT dbv;
						char* szNick;


						dbv.type = DBVT_DELETED;
						if (DBGetContactSetting(NULL, gpszICQProtoName, "Nick", &dbv))
							szNick = "";
						else
							szNick = dbv.pszVal;

						directPacketInit(&packet, (WORD)(20 + strlen(szNick)));
						packByte(&packet, 0);		 /* packet type */
						packLEDWord(&packet, 0);	 /* unknown */
						packLEDWord(&packet, dc->ft->dwFileCount);
						packLEDWord(&packet, dc->ft->dwTotalSize);
						packLEDWord(&packet, dc->ft->dwTransferSpeed);
						packLEWord(&packet, (WORD)(strlen(szNick) + 1));
						packBuffer(&packet, szNick, (WORD)(strlen(szNick) + 1));
						sendDirectPacket(dc->hConnection, &packet);
						DBFreeVariant(&dbv);
						dc->initialised = 1;
					}
				}
			}
			else
			{
				Netlib_Logf(hDirectNetlibUser, "Unsupported direct protocol: %d, closing connection", dc->wVersion);
				Netlib_CloseHandle(dc->hConnection);
				dc->hConnection = NULL;
			}
			break;

		case 2:    /* messaging packets */
			Netlib_Logf(hDirectNetlibUser, "Received PEER_MSG from %u", dc->dwRemoteUin);
			handleDirectMessage(dc, buf + 1, (WORD)(wLen - 1));
			break;

		case 3:
			/* Last packet sent during connect
			 * This is an additional packet in the peer-to-peer handshake. It is only used in the message connection,
			 * _not_ for file transfer. So it kinda distinguishes what this connection is for. It is sent by the originator
			 * of the connection after he has acknowledged the peer's PEER_INIT and by the peer as a reply to the originator's
			 * PEER_MSGINIT.
			 */
			{

				DWORD dw;
				icq_packet packet;


				Netlib_Logf(hDirectNetlibUser, "Received PEER_MSGINIT from %u",dc->dwRemoteUin);

				buf++;
				if (wLen != 0x21)
					break;

				unpackLEDWord(&buf, &dw);   /* always 10 */
				unpackLEDWord(&buf, &dw);
				if (dw != 1)
				{   /* this is some sort of 'check it's working' connection. Useless so kill. */
					Netlib_CloseHandle(dc->hConnection);
					dc->hConnection = NULL;
					break;
				}

				if (dc->incoming)
				{
					/* reply */
					directPacketInit(&packet, 33);
					packByte(&packet, 3);
					packLEDWord(&packet, 10);
					packLEDWord(&packet, 1);
					packLEDWord(&packet, 1);
					packLEDWord(&packet, 0);
					packLEDWord(&packet, 0);
					packLEWord(&packet, 1);
					packLEWord(&packet, 4);
					packLEDWord(&packet, 0);
					packLEDWord(&packet, 0);
					sendDirectPacket(dc->hConnection, &packet);
				}
				else
				{
					if (dc->packetToSend != NULL)
					{
						if (dc->packetToSend->pData[2] == 2)
							EncryptDirectPacket(dc, dc->packetToSend);
						sendDirectPacket(dc->hConnection, dc->packetToSend);
						SAFE_FREE(dc->packetToSend);
						dc->packetToSend = NULL;
					}
				}

				dc->initialised = 1;
			}
			break;

		default:
			Netlib_Logf(hDirectNetlibUser, "Unknown direct packet ignored.");
			break;

	}

}



void EncryptDirectPacket(directconnect* dc, icq_packet* p)
{

  unsigned long B1;
  unsigned long M1;
  unsigned long check;
  unsigned int i;
  unsigned char X1;
  unsigned char X2;
  unsigned char X3;
  unsigned char* buf = (unsigned char*)(p->pData + 3);
  unsigned char bak[6];
  unsigned long offset;
  unsigned long key;
  unsigned long hex;
  unsigned long size = p->wLen - 1;


  if (dc->wVersion < 4)
    return;  // no encryption necessary.


  switch (dc->wVersion)
  {

    case 4:
    case 5:
      offset = 6;
      break;

    case 6:
    case 7:
	case 8:
	case 9:
	case 10:
    default:
      offset = 0;

  }


  // calculate verification data
  M1 = (rand() % ((size < 255 ? size : 255)-10))+10;
  X1 = buf[M1] ^ 0xFF;
  X2 = rand() % 220;
  X3 = client_check_data[X2] ^ 0xFF;
  if(offset)
  {
    memcpy(bak, buf, sizeof(bak));
    B1 = (buf[offset+4]<<24) | (buf[offset+6]<<16) | (buf[2]<<8) | buf[0];
  }
  else
  {
    B1 = (buf[4]<<24) | (buf[6]<<16) | (buf[4]<<8) | (buf[6]);
  }

  // calculate checkcode
  check = (M1<<24) | (X1<<16) | (X2<<8) | X3;
  check ^= B1;

  // main XOR key
  key = 0x67657268 * size + check;

  // XORing the actual data
  for (i = 0; i<(size+3)/4; i+=4)
  {
    hex = key + client_check_data[i&0xFF];
	*(PDWORD)(buf + i) ^= hex;
  }

  // in TCPv4 are the first 6 bytes unencrypted
  // so restore them
  if (offset)
    memcpy(buf, bak, sizeof(bak));

  // storing the checkcode
  *(PDWORD)(buf + offset) = check;

}



static int DecryptDirectPacket(directconnect* dc, PBYTE buf, WORD wLen)
{

  unsigned long hex;
  unsigned long key;
  unsigned long B1;
  unsigned long M1;
  unsigned long check;
  unsigned int i;
  unsigned char X1;
  unsigned char X2;
  unsigned char X3;
  unsigned char bak[6];
  unsigned long size = wLen;
  unsigned long offset;


  if (dc->wVersion < 4)
    return 1;  // no decryption necessary.

  if (size < 4)
	return 1;

  if (dc->wVersion < 4)
	  return 1;

  if (dc->wVersion == 4 || dc->wVersion == 5)
  {
    offset = 6;
  }
  else
  {
    offset = 0;
  }

  // backup the first 6 bytes
  if (offset)
    memcpy(bak, buf, sizeof(bak));

  // retrieve checkcode
  check = *(PDWORD)(buf+offset);

  // main XOR key
  key = 0x67657268 * size + check;

  for (i=4; i<(size+3)/4; i+=4)
  {
    hex = key + client_check_data[i&0xFF];
	*(PDWORD)(buf + i) ^= hex;
  }

  // retrive validate data
  if (offset)
  {
    // in TCPv4 are the first 6 bytes unencrypted
    // so restore them
    memcpy(buf, bak, sizeof(bak));
    B1 = (buf[offset+4]<<24) | (buf[offset+6]<<16) | (buf[2]<<8) | buf[0];
  }
  else
  {
    B1 = (buf[4]<<24) | (buf[6]<<16) | (buf[4]<<8) | (buf[6]<<0);
  }

  // special decryption
  B1 ^= check;

  // validate packet
  M1 = (B1>>24) & 0xFF;
  if (M1 < 10 || M1 >= size)
  {
    return 0;
  }

  X1 = buf[M1] ^ 0xFF;
  if(((B1 >> 16) & 0xFF) != X1)
  {
    return 0;
  }

  X2 = (BYTE)((B1 >> 8) & 0xFF);
  if (X2 < 220)
  {
    X3 = client_check_data[X2] ^ 0xFF;
    if ((B1 & 0xFF) != X3)
	{
      return 0;
    }
  }


  {

	  char szTitleLine[128];
	  char* szBuf;
	  int titleLineLen;
	  int line;
	  int col;
	  int colsInLine;
	  char* pszBuf;


	  titleLineLen = _snprintf(szTitleLine, 128, "DECRYPTED\n");
	  szBuf = (char*)malloc(titleLineLen + ((wLen+15)>>4) * 76 + 1);
	  CopyMemory(szBuf, szTitleLine, titleLineLen);
	  pszBuf = szBuf + titleLineLen;
	  
	  for (line = 0; ; line += 16)
	  {

		  colsInLine = min(16, wLen - line);
		  pszBuf += wsprintf(pszBuf, "%08X: ", line);

		  for (col = 0; col<colsInLine; col++)
			  pszBuf += wsprintf(pszBuf, "%02X%c", buf[line+col], (col&3)==3 && col!=15?'-':' ');

		  for (; col<16; col++)
		  {
			  lstrcpy(pszBuf,"   ");
			  pszBuf+=3;
		  }

		  *pszBuf++ = ' ';
		  for (col = 0; col<colsInLine; col++)
			  *pszBuf++ = buf[line+col]<' ' ? '.' : (char)buf[line+col];
		  if(wLen-line<=16) break;
		  *pszBuf++='\n';

	  }
	  *pszBuf='\0';

	  Netlib_Logf(hDirectNetlibUser, szBuf);

	  SAFE_FREE(szBuf);

  }

  return 1;

}



/*
void startHandshake_v6(const char *szHost, DWORD dwPort, DWORD dwUin)
{
	icq_packet packet;
	DWORD dwSessionId;

	dwSessionId = rand();

	directPacketInit(&packet, 44);
	packByte(&packet, 0xff);			// Ident
	packLEDWord(&packet, ICQ_VERSION);	// Our version
	packLEDWord(&packet, dwUin);
	
	packWord(&packet, 0);

	packLEDWord(&packet, dwListenPort);
	packLEDWord(&packet, dwLocalUin);
	packDWord(&packet, dwExternalIP);
	packDWord(&packet, dwInteralIP);
	packByte(&packet, nTCPFlag);
	packLEDWord(&packet, dwSessionId);
	packLEDWord(&packet, dwListenPort);
	packLEDWord(&packet, dwSessionId);
	packLEDWord(&packet, WEBFRONTPORT);
	packLEDWord(&packet, 0x00000003);
}
*/



// Sends a PEER_INIT packet through a DC
// -----------------------------------------------------------------------
// This packet is sent during direct connection initialization between two
// ICQ clients. It is sent by the originator of the connection to start
// the handshake and by the receiver directly after it has sent the
// PEER_ACK packet as a reply to the originator's PEER_INIT. The values
// after the COOKIE field have been added for v7.
static void startHandshake_v78(directconnect* dc)
{

	icq_packet packet;


	directPacketInit(&packet, 48);             // Full packet length

	packByte(&packet, 0xff);			       // Command, 0xFF = Connect
	packLEWord(&packet, dc->wVersion);	       // Version
	packLEWord(&packet, 43);	               // Data length
	packLEDWord(&packet, dc->dwRemoteUin);     // UIN of remote user
	packWord(&packet, 0);                      // Unknown
	packLEDWord(&packet, wListenPort);         // Our port
	packLEDWord(&packet, dwLocalUIN);          // Our UIN
	packDWord(&packet, dwLocalExternalIP);     // Our external IP
	packDWord(&packet, dwLocalInternalIP);     // Our internal IP
	packByte(&packet, DC_TYPE);	               // TCP connection flags
	packLEDWord(&packet, wListenPort);         // Our port
	packLEDWord(&packet, dc->dwConnCookie);    // DC cookie
	packLEDWord(&packet, WEBFRONTPORT);        // Unknown
	packLEDWord(&packet, CLIENTFEATURES);      // Unknown
	packDWord(&packet, 0);                     // Unknown

	sendDirectPacket(dc->hConnection, &packet);

}



// Sends a PEER_INIT packet through a DC
// -----------------------------------------------------------------------
// This is sent to acknowledge a PEER_INIT packet.
static void sendPeerInitAck(directconnect* dc)
{

	icq_packet packet;


	// The PEER_INITACK packet is just a DWORD 1
	directPacketInit(&packet, 4);  // Packet length
	packLEDWord(&packet, 1);       // 

	sendDirectPacket(dc->hConnection, &packet);

	if (dc->incoming)
		Netlib_Logf(hDirectNetlibUser, "Sent PEER_INIT_ACK DC from %u on incoming DC", dc->dwRemoteUin);
	else
		Netlib_Logf(hDirectNetlibUser, "Sent PEER_INIT_ACK DC from %u on outgoing DC", dc->dwRemoteUin);

}
