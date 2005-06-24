// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
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
//  Manages main server connection, low-level communication
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"



extern CRITICAL_SECTION connectionHandleMutex;
extern WORD wLocalSequence;
extern CRITICAL_SECTION localSeqMutex;
extern HANDLE hKeepAliveEvent;
HANDLE hServerConn;
DWORD dwLocalInternalIP, dwLocalExternalIP;
WORD wListenPort;
WORD wLocalSequence;
DWORD dwLocalDirectConnCookie;
HANDLE hServerPacketRecver;
static pthread_t serverThreadId;
HANDLE hDirectBoundPort;
int bReinitRecver = 0;

static int handleServerPackets(unsigned char* buf, int len, serverthread_start_info* info);
static void icq_encryptPassword(const char* szPassword, unsigned char* encrypted);



static DWORD __stdcall icq_serverThread(serverthread_start_info* infoParam)
{
  serverthread_start_info info;

  info = *infoParam;
  SAFE_FREE(&infoParam);

  srand(time(NULL));

  dwLocalDirectConnCookie = rand() ^ (rand() << 16);

  ResetSettingsOnConnect();

	// Connect to the login server
	Netlib_Logf(ghServerNetlibUser, "Authenticating to server");
	hServerConn = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)ghServerNetlibUser, (LPARAM)&info.nloc);
  if (!hServerConn && (GetLastError() == 87))
  {
    info.nloc.cbSize = NETLIBOPENCONNECTION_V1_SIZE;
    hServerConn = (HANDLE)CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)ghServerNetlibUser, (LPARAM)&info.nloc);
  }

	SAFE_FREE(&(void*)info.nloc.szHost);


	// Login error
	if (hServerConn == NULL)
	{
		int oldStatus = gnCurrentStatus;
		DWORD dwError = GetLastError();

		hServerConn = NULL;
		gnCurrentStatus = ID_STATUS_OFFLINE;
		ProtoBroadcastAck(gpszICQProtoName, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS,
			(HANDLE)oldStatus, gnCurrentStatus);

		icq_LogUsingErrorCode(LOG_ERROR, dwError, "Unable to connect to ICQ login server");

		return 0;
	}


	// Initialize direct connection ports
	{
		NETLIBBIND nlb = {0};

		nlb.cbSize = sizeof(nlb);
		nlb.pfnNewConnection = icq_newConnectionReceived;
    hDirectBoundPort = (HANDLE)CallService(MS_NETLIB_BINDPORT, (WPARAM)ghDirectNetlibUser, (LPARAM)&nlb);
    if (!hDirectBoundPort && (GetLastError() == 87))
    { // this ensures old Miranda also can bind a port for a dc
      nlb.cbSize = NETLIBBIND_SIZEOF_V1;
      hDirectBoundPort = (HANDLE)CallService(MS_NETLIB_BINDPORT, (WPARAM)ghDirectNetlibUser, (LPARAM)&nlb);
    }
		if (hDirectBoundPort == NULL)
		{
			icq_LogUsingErrorCode(LOG_WARNING, GetLastError(), "Miranda was unable to allocate a port to listen for direct peer-to-peer connections between clients. You will be able to use most of the ICQ network without problems but you may be unable to send or receive files.\n\nIf you have a firewall this may be blocking Miranda, in which case you should configure your firewall to leave some ports open and tell Miranda which ports to use in M->Options->ICQ->Network.");
			wListenPort = 0;
		}
		wListenPort = nlb.wPort;
		dwLocalInternalIP = nlb.dwInternalIP;
	}


  // This is the "infinite" loop that receives the packets from the ICQ server
  {
    int recvResult;
    NETLIBPACKETRECVER packetRecv = {0};

    hServerPacketRecver = (HANDLE)CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)hServerConn, 8192);
    packetRecv.cbSize = sizeof(packetRecv);
    packetRecv.dwTimeout = INFINITE;
    bReinitRecver = 0;
    for(;;)
    {
      if (bReinitRecver)
      { // we reconnected, reinit struct
        bReinitRecver = 0;
        ZeroMemory(&packetRecv, sizeof(packetRecv));
        packetRecv.cbSize = sizeof(packetRecv);
        packetRecv.dwTimeout = INFINITE;
      }

			recvResult = CallService(MS_NETLIB_GETMOREPACKETS,(WPARAM)hServerPacketRecver, (LPARAM)&packetRecv);

			if (recvResult == 0)
			{
				Netlib_Logf(ghServerNetlibUser, "Clean closure of server socket");
				break;
			}

			if (recvResult == SOCKET_ERROR)
			{
				Netlib_Logf(ghServerNetlibUser, "Abortive closure of server socket, error: %d", GetLastError());
				break;
			}

			// Deal with the packet
			packetRecv.bytesUsed = handleServerPackets(packetRecv.buffer, packetRecv.bytesAvailable, &info);
		}

		// Close the packet receiver (connection may still be open)
		Netlib_CloseHandle(hServerPacketRecver);
		hServerPacketRecver = 0;

		// Close DC port
		Netlib_CloseHandle(hDirectBoundPort);
		hDirectBoundPort = 0;
	}

	// Time to shutdown
	icq_serverDisconnect();
	if (gnCurrentStatus != ID_STATUS_OFFLINE)
	{
		int oldStatus = gnCurrentStatus;

		gnCurrentStatus = ID_STATUS_OFFLINE;
		ProtoBroadcastAck(gpszICQProtoName, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS,
			(HANDLE)oldStatus, gnCurrentStatus);
	}

  // Offline all contacts
	{
		HANDLE hContact;
		char* szProto;

		hContact= (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

		while (hContact)
		{
			szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
			if (szProto != NULL && !strcmp(szProto, gpszICQProtoName))
			{
				if (DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0))
				{
					if (DBGetContactSettingWord(hContact, gpszICQProtoName, "Status", ID_STATUS_OFFLINE)
						!= ID_STATUS_OFFLINE)
					{
						DBWriteContactSettingWord(hContact, gpszICQProtoName, "Status", ID_STATUS_OFFLINE);
					}
				}
			}

			hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
		}
	}
  DBWriteContactSettingDword(NULL, gpszICQProtoName, "LogonTS", 0); // clear logon time

  FlushServerIDs(); // clear server IDs list
  FlushPendingOperations(); // clear pending operations list
  FlushGroupRenames(); // clear group rename in progress list

  Netlib_Logf(ghServerNetlibUser, "Server thread ended.");

  return 0;
}



void icq_serverDisconnect()
{
	EnterCriticalSection(&connectionHandleMutex);

	if (hServerConn)
	{
    int sck = CallService(MS_NETLIB_GETSOCKET, (WPARAM)hServerConn, (LPARAM)0);
    if (sck!=INVALID_SOCKET) shutdown(sck, 2); // close gracefully
		Netlib_CloseHandle(hServerConn);
    FreeGatewayIndex(hServerConn);
		hServerConn = NULL;
		LeaveCriticalSection(&connectionHandleMutex);
    
		// Not called from network thread?
		if (GetCurrentThreadId() != serverThreadId.dwThreadId)
    {
			while (WaitForSingleObjectEx(serverThreadId.hThread, INFINITE, TRUE) != WAIT_OBJECT_0);
		  CloseHandle(serverThreadId.hThread);
    }
	}
	else
		LeaveCriticalSection(&connectionHandleMutex);

  if (hKeepAliveEvent) SetEvent(hKeepAliveEvent); // signal keep-alive thread to stop
}



static int handleServerPackets(unsigned char* buf, int len, serverthread_start_info* info)
{
	BYTE channel;
	WORD sequence;
	WORD datalen;
	int bytesUsed = 0;

	while (len > 0)
	{
		// All FLAPS begin with 0x2a
		if (*buf++ != FLAP_MARKER)
			break;

		if (len < 6)
			break;

		unpackByte(&buf, &channel);
		unpackWord(&buf, &sequence);
		unpackWord(&buf, &datalen);

		if (len < 6 + datalen)
			break;


#ifdef _DEBUG
		Netlib_Logf(ghServerNetlibUser, "Server FLAP: Channel %u, Seq %u, Length %u bytes", channel, sequence, datalen);
#endif

		switch (channel)
		{
		case ICQ_LOGIN_CHAN:
			handleLoginChannel(buf, datalen, info);
			break;

		case ICQ_DATA_CHAN:
			handleDataChannel(buf, datalen);
			break;

		case ICQ_ERROR_CHAN:
			handleErrorChannel(buf, datalen);
			break;

		case ICQ_CLOSE_CHAN:
			handleCloseChannel(buf, datalen);
      break; // we need this for walking thru proxy

		case ICQ_PING_CHAN:
			handlePingChannel(buf, datalen);
			return 0;

		default:
			Netlib_Logf(ghServerNetlibUser, "Warning: Unhandled Server FLAP Channel: Channel %u, Seq %u, Length %u bytes", channel, sequence, datalen);
			break;
		}

		/* Increase pointers so we can check for more FLAPs */
		buf += datalen;
		len -= (datalen + 6);
		bytesUsed += (datalen + 6);
	}

	return bytesUsed;
}



void sendServPacket(icq_packet* pPacket)
{
	// This critsec makes sure that the sequence order doesn't get screwed up
	EnterCriticalSection(&localSeqMutex);

	if (hServerConn)
	{
		int nRetries;
		int nSendResult;


    // :IMPORTANT:
		// The FLAP sequence must be a WORD. When it reaches 0xFFFF it should wrap to
		// 0x0000, otherwise we'll get kicked by server.
		wLocalSequence++;

		// Pack sequence number
		pPacket->pData[2] = ((wLocalSequence & 0xff00) >> 8);
		pPacket->pData[3] = (wLocalSequence & 0x00ff);

		for (nRetries = 3; nRetries >= 0; nRetries--)
		{
			nSendResult = Netlib_Send(hServerConn, (const char *)pPacket->pData, pPacket->wLen, 0);

			if (nSendResult != SOCKET_ERROR)
				break;

			Sleep(1000);
		}


		// Send error
		if (nSendResult == SOCKET_ERROR)
		{
			icq_LogUsingErrorCode(LOG_ERROR, GetLastError(), "Your connection with the ICQ server was abortively closed");
			icq_serverDisconnect(0);

			if (gnCurrentStatus != ID_STATUS_OFFLINE)
			{
				int oldStatus = gnCurrentStatus;

				gnCurrentStatus = ID_STATUS_OFFLINE;
				ProtoBroadcastAck(gpszICQProtoName, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS,
					(HANDLE)oldStatus, gnCurrentStatus);
			}
		}
	}
	else
	{
		Netlib_Logf(ghServerNetlibUser, "Error: Failed to send packet (no connection)");
	}

	LeaveCriticalSection(&localSeqMutex);

	SAFE_FREE(&pPacket->pData);
}



void icq_login(const char* szPassword)
{
	DBVARIANT dbvServer = {DBVT_DELETED};
	serverthread_start_info* stsi;
	DWORD dwUin;


	dwUin = DBGetContactSettingDword(NULL, gpszICQProtoName, UNIQUEIDSETTING, 0);
	stsi = (serverthread_start_info*)calloc(sizeof(serverthread_start_info), 1);
	stsi->nloc.cbSize = sizeof(NETLIBOPENCONNECTION);
	stsi->nloc.flags = 0;


	// Server host name
	if (DBGetContactSetting(NULL, gpszICQProtoName, "OscarServer", &dbvServer))
		stsi->nloc.szHost = _strdup(DEFAULT_SERVER_HOST);
	else
		stsi->nloc.szHost = _strdup(dbvServer.pszVal);

	// Server port
	stsi->nloc.wPort = (WORD)DBGetContactSettingWord(NULL, gpszICQProtoName, "OscarPort", DEFAULT_SERVER_PORT);
	if (stsi->nloc.wPort == 0)
		stsi->nloc.wPort = RandRange(1024, 65535);

	// User password
	stsi->wPassLen = strlen(szPassword);
	if (stsi->wPassLen > 9) stsi->wPassLen = 9;
	icq_encryptPassword(szPassword, stsi->szEncPass);
	stsi->szEncPass[stsi->wPassLen] = '\0';

	// Randomize sequence
	wLocalSequence = (WORD)RandRange(0, 0x7fff);

	dwLocalUIN = dwUin;

	isLoginServer = 1;

	serverThreadId.hThread = (HANDLE)forkthreadex(NULL, 0, icq_serverThread, stsi, 0, &serverThreadId.dwThreadId);

	DBFreeVariant(&dbvServer);
}



static void icq_encryptPassword(const char* szPassword, unsigned char* encrypted)
{
	unsigned int i;
	unsigned char table[] =
	{
		0xf3, 0x26, 0x81, 0xc4,
		0x39, 0x86, 0xdb, 0x92,
		0x71, 0xa3, 0xb9, 0xe6,
		0x53, 0x7a, 0x95, 0x7c
	};

	for (i = 0; szPassword[i]; i++)
	{
		encrypted[i] = (szPassword[i] ^ table[i % 16]);
	}
}
