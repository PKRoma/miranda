// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006,2007 Joe Kucera
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
// File name      : $URL$
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
  HANDLE hConnection; // only valid for incoming connections, handle to the connection
  HANDLE hContact;    // Only valid for outgoing connections
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
static int directConnSize = 0;
static int directConnCount = 0;
static int mutexesInited = 0;
static CRITICAL_SECTION directConnListMutex;
static CRITICAL_SECTION expectedFileRecvMutex;
static int expectedFileRecvCount = 0;
static filetransfer** expectedFileRecv = NULL;

extern WORD wListenPort;

static void handleDirectPacket(directconnect* dc, PBYTE buf, WORD wLen);
static DWORD __stdcall icq_directThread(directthreadstartinfo* dtsi);
static void sendPeerInit_v78(directconnect* dc);
static int DecryptDirectPacket(directconnect* dc, PBYTE buf, WORD wLen);
static void sendPeerInitAck(directconnect* dc);
static void sendPeerMsgInit(directconnect* dc, DWORD dwSeq);
static void sendPeerFileInit(directconnect* dc);



void InitDirectConns(void)
{
  if (!mutexesInited)
  {
    mutexesInited = 1;
    InitializeCriticalSection(&directConnListMutex);
    InitializeCriticalSection(&expectedFileRecvMutex);
  }
  directConnCount = 0;
  directConnSize = 0;
}



void UninitDirectConns(void)
{
  CloseContactDirectConns(NULL);

  for(;;)
  {
    if (!directConnCount)
      break;

    Sleep(10);     /* yeah, ugly */
  }

  DeleteCriticalSection(&directConnListMutex);
  DeleteCriticalSection(&expectedFileRecvMutex);

  SAFE_FREE((void**)&directConnList);
}



void CloseContactDirectConns(HANDLE hContact)
{
  int i;

  EnterCriticalSection(&directConnListMutex);

  for (i = 0; i < directConnCount; i++)
  {
    if (!hContact || directConnList[i]->hContact == hContact)
    {
      HANDLE hConnection = directConnList[i]->hConnection;
      int sck = CallService(MS_NETLIB_GETSOCKET, (WPARAM)hConnection, 0);

      directConnList[i]->hConnection = NULL; // do not allow reuse
      if (sck!=INVALID_SOCKET) shutdown(sck, 2); // close gracefully
      NetLib_SafeCloseHandle(&hConnection, FALSE);
    }
  }

  LeaveCriticalSection(&directConnListMutex);
}



static void ResizeDirectList(int nSize)
{
  if ((directConnSize < nSize) || ((directConnSize > nSize + 6) && nSize))
  {
    if (directConnSize < nSize)
      directConnSize += 4;
    else
      directConnSize -= 4;

    directConnList = (directconnect**)realloc(directConnList, sizeof(directconnect*) * directConnSize);
  }
}



static void AddDirectConnToList(directconnect* dc)
{
  EnterCriticalSection(&directConnListMutex);

  ResizeDirectList(directConnCount + 1);
  directConnList[directConnCount] = dc;
  directConnCount++;

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
      directConnCount--;
      directConnList[i] = directConnList[directConnCount];
      ResizeDirectList(directConnCount);
      break;
    }
  }

  LeaveCriticalSection(&directConnListMutex);
}



directconnect* FindFileTransferDC(filetransfer* ft)
{
  int i;
  directconnect* dc = NULL;

  EnterCriticalSection(&directConnListMutex);

  for (i = 0; i < directConnCount; i++)
  {
    if (directConnList[i] && directConnList[i]->ft == ft)
    {
      dc = directConnList[i];
      break;
    }
  }

  LeaveCriticalSection(&directConnListMutex);
  return dc;
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



int sendDirectPacket(directconnect* dc, icq_packet* pkt)
{
  int nResult;

  nResult = Netlib_Send(dc->hConnection, (const char*)pkt->pData, pkt->wLen + 2, 0);

  if (nResult == SOCKET_ERROR)
  {
    NetLog_Direct("Direct %p socket error: %d, closing", dc->hConnection, GetLastError());
    CloseDirectConnection(dc);
  }
  
  SAFE_FREE(&pkt->pData);

  return nResult;
}



directthreadstartinfo* CreateDTSI(HANDLE hContact, HANDLE hConnection, int type)
{
  directthreadstartinfo* dtsi;

  dtsi = (directthreadstartinfo*)SAFE_MALLOC(sizeof(directthreadstartinfo));
  dtsi->hContact = hContact;
  dtsi->hConnection = hConnection;
  if (type == -1)
  {
    dtsi->incoming = 1;
  }
  else
  {
    dtsi->type = type;
  }

  return dtsi;
}



// Check if we have an open and initialized DC with type
// 'type' to the specified contact
BOOL IsDirectConnectionOpen(HANDLE hContact, int type, int bPassive)
{
  int i;
  BOOL bIsOpen = FALSE, bIsCreated = FALSE;


  EnterCriticalSection(&directConnListMutex);

  for (i = 0; i < directConnCount; i++)
  {
    if (directConnList[i] && (directConnList[i]->type == type))
    {
      if (directConnList[i]->hContact == hContact)
        if (directConnList[i]->initialised)
        {
          // Connection is OK
          bIsOpen = TRUE;
          // we are going to use the conn, so prevent timeout
          directConnList[i]->packetPending = 1;
          break;
        }
        else
          bIsCreated = TRUE; // we found pending connection
    }
  }
  
  LeaveCriticalSection(&directConnListMutex);
  
  if (!bPassive && !bIsCreated && !bIsOpen && type == DIRECTCONN_STANDARD && gbDCMsgEnabled == 2)
  { // do not try to open DC to offline contact
    if (ICQGetContactStatus(hContact) == ID_STATUS_OFFLINE) return FALSE;
    // do not try to open DC if previous attempt was not successfull
    if (ICQGetContactSettingByte(hContact, "DCStatus", 0)) return FALSE;

    // Set DC status as tried
    ICQWriteContactSettingByte(hContact, "DCStatus", 1);
    // Create a new connection
    OpenDirectConnection(hContact, DIRECTCONN_STANDARD, NULL);
  }

  return bIsOpen;
}



// This function is called from the Netlib when someone is connecting to
// one of our incomming DC ports
void icq_newConnectionReceived(HANDLE hNewConnection, DWORD dwRemoteIP, void *pExtra)
{
  // Start a new thread for the incomming connection
  ICQCreateThread(icq_directThread, CreateDTSI(NULL, hNewConnection, -1));
}



// Opens direct connection of specified type to specified contact
void OpenDirectConnection(HANDLE hContact, int type, void* pvExtra)
{
  directthreadstartinfo* dtsi;

  // Create a new connection
  dtsi = CreateDTSI(hContact, NULL, type);
  dtsi->pvExtra = pvExtra;

  ICQCreateThread(icq_directThread, dtsi);
}



// Safely close NetLib connection - do not corrupt direct connection list
void CloseDirectConnection(directconnect *dc)
{
  EnterCriticalSection(&directConnListMutex);

  NetLib_SafeCloseHandle(&dc->hConnection, FALSE);
#ifdef _DEBUG
  if (dc->hConnection)
    NetLog_Direct("Direct conn closed (%p)", dc->hConnection);
#endif

  LeaveCriticalSection(&directConnListMutex);
}



// This should be called only if connection already exists
int SendDirectMessage(HANDLE hContact, icq_packet *pkt)
{
  int i;

  EnterCriticalSection(&directConnListMutex);

  for (i = 0; i < directConnCount; i++)
  {
    if (directConnList[i] == NULL)
      continue;

    if (directConnList[i]->hContact == hContact)
    {
      if (directConnList[i]->initialised)
      {
        // This connection can be reused, send packet and exit
        NetLog_Direct("Sending direct message");

        if (pkt->pData[2] == 2)
          EncryptDirectPacket(directConnList[i], pkt);

        sendDirectPacket(directConnList[i], pkt);
        directConnList[i]->packetPending = 0; // packet done

        LeaveCriticalSection(&directConnListMutex);

        return TRUE; // Success
      }
      break; // connection not ready, use server instead
    }
  }
    
  LeaveCriticalSection(&directConnListMutex);

  return FALSE; // connection pending, we failed, use server instead
}


// Called from icq_newConnectionReceived when a new incomming dc is done
// Called from OpenDirectConnection when a new outgoing dc is done
// Called from SendDirectMessage when a new outgoing dc is done
static DWORD __stdcall icq_directThread(directthreadstartinfo *dtsi)
{
  directconnect dc = {0};
  NETLIBPACKETRECVER packetRecv={0};
  HANDLE hPacketRecver;
  BOOL bFirstPacket = TRUE;
  int nSkipPacketBytes = 0;
  DWORD dwReqMsgID1;
  DWORD dwReqMsgID2;


  srand(time(NULL));
  AddDirectConnToList(&dc);

  // Initialize DC struct
  dc.hContact = dtsi->hContact;
  dc.dwThreadId = GetCurrentThreadId();
  dc.incoming = dtsi->incoming;
  dc.hConnection = dtsi->hConnection;
  dc.ft = NULL;

  if (!dc.incoming)
  {
    dc.type = dtsi->type;
    dc.dwRemoteExternalIP = ICQGetContactSettingDword(dtsi->hContact, "IP", 0);
    dc.dwRemoteInternalIP = ICQGetContactSettingDword(dtsi->hContact, "RealIP", 0);
    dc.dwRemotePort = ICQGetContactSettingWord(dtsi->hContact, "UserPort", 0);
    dc.dwRemoteUin = ICQGetContactSettingUIN(dtsi->hContact);
    dc.dwConnectionCookie = ICQGetContactSettingDword(dtsi->hContact, "DirectCookie", 0);
    dc.wVersion = ICQGetContactSettingWord(dtsi->hContact, "Version", 0);

    if (!dc.dwRemoteExternalIP && !dc.dwRemoteInternalIP)
    { // we do not have any ip, do not try to connect
      RemoveDirectConnFromList(&dc);
      SAFE_FREE(&dtsi);
      return 0; 
    }
    if (!dc.dwRemotePort)
    { // we do not have port, do not try to connect
      RemoveDirectConnFromList(&dc);
      SAFE_FREE(&dtsi);
      return 0; 
    }

    if (dc.type == DIRECTCONN_STANDARD)
    {
      // do nothing - some specific init for msg sessions
    }
    else if (dc.type == DIRECTCONN_FILE)
    {
      dc.ft = (filetransfer*)dtsi->pvExtra;
      dc.dwRemotePort = dc.ft->dwRemotePort;
    }
    else if (dc.type == DIRECTCONN_REVERSE)
    {
      reverse_cookie *pCookie = (reverse_cookie*)dtsi->pvExtra;

      dwReqMsgID1 = pCookie->pMessage.dwMsgID1;
      dwReqMsgID2 = pCookie->pMessage.dwMsgID2;
      dc.dwReqId = (DWORD)pCookie->ft;
      SAFE_FREE(&pCookie);
    }
  }
  else
  {
    dc.type = DIRECTCONN_STANDARD;
  }

  SAFE_FREE(&dtsi);

  // Load local IP information
  dc.dwLocalExternalIP = ICQGetContactSettingDword(NULL, "IP", 0);
  dc.dwLocalInternalIP = ICQGetContactSettingDword(NULL, "RealIP", 0);

  // Create outgoing DC
  if (!dc.incoming)
  {
    NETLIBOPENCONNECTION nloc = {0};
    IN_ADDR addr = {0}, addr2 = {0};

    if (dc.dwRemoteExternalIP == dc.dwLocalExternalIP && dc.dwRemoteInternalIP)
      addr.S_un.S_addr = htonl(dc.dwRemoteInternalIP);
    else
    {
      addr.S_un.S_addr = htonl(dc.dwRemoteExternalIP);
      // for different internal, try it also (for LANs with multiple external IP, VPNs, etc.)
      if (dc.dwRemoteInternalIP != dc.dwRemoteExternalIP)
        addr2.S_un.S_addr = htonl(dc.dwRemoteInternalIP);
    }

    if (!addr.S_un.S_addr)
    { // IP to connect to is empty, go away
      RemoveDirectConnFromList(&dc);
      return 0;
    }
    nloc.szHost = inet_ntoa(addr);
    nloc.wPort = (WORD)dc.dwRemotePort;
    nloc.timeout = 8; // 8 secs to connect
    dc.hConnection = NetLib_OpenConnection(ghDirectNetlibUser, dc.type==DIRECTCONN_REVERSE?"Reverse ":NULL, &nloc);
    if (!dc.hConnection && addr2.S_un.S_addr)
    { // first address failed, try second one if available
      nloc.szHost = inet_ntoa(addr2);
      dc.hConnection = NetLib_OpenConnection(ghDirectNetlibUser, dc.type==DIRECTCONN_REVERSE?"Reverse ":NULL, &nloc);
    }
    if (!dc.hConnection)
    {
      if (CheckContactCapabilities(dc.hContact, CAPF_ICQDIRECT))
      { // only if the contact support ICQ DC connections
        if (dc.type != DIRECTCONN_REVERSE)
        { // try reverse connect
          reverse_cookie *pCookie = (reverse_cookie*)SAFE_MALLOC(sizeof(reverse_cookie));
          DWORD dwCookie;

          NetLog_Direct("connect() failed (%d), trying reverse.", GetLastError());

          if (pCookie)
          { // init cookie
            InitMessageCookie(&pCookie->pMessage);
            pCookie->pMessage.bMessageType = MTYPE_REVERSE_REQUEST;
            pCookie->hContact = dc.hContact;
            pCookie->dwUin = dc.dwRemoteUin;
            pCookie->type = dc.type;
            pCookie->ft = dc.ft;
            dwCookie = AllocateCookie(CKT_REVERSEDIRECT, 0, dc.hContact, pCookie);
            icq_sendReverseReq(&dc, dwCookie, (message_cookie_data*)pCookie);
            RemoveDirectConnFromList(&dc);

            return 0;
          }
          else
            NetLog_Direct("Reverse failed (%s)", "malloc failed");
        }
      }
      else // Set DC status to failed
        ICQWriteContactSettingByte(dc.hContact, "DCStatus", 2);

      if (dc.type == DIRECTCONN_REVERSE) // failed reverse connection
      { // announce we failed
        icq_sendReverseFailed(&dc, dwReqMsgID1, dwReqMsgID2, dc.dwReqId);
      }
      NetLog_Direct("connect() failed (%d)", GetLastError());
      RemoveDirectConnFromList(&dc);
      if (dc.type == DIRECTCONN_FILE)
      {
        ICQBroadcastAck(dc.ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, dc.ft, 0);
        // Release transfer
        SafeReleaseFileTransfer(&dc.ft);
      }
      return 0;
    }

    if (dc.type == DIRECTCONN_FILE)
      dc.ft->hConnection = dc.hConnection;

    if (dc.wVersion > 6)
    {
      sendPeerInit_v78(&dc);
    }
    else
    {
      NetLog_Direct("Error: Unsupported direct protocol: %d, closing.", dc.wVersion);
      CloseDirectConnection(&dc);
      RemoveDirectConnFromList(&dc);

      return 0;
    }
  }

  hPacketRecver = (HANDLE)CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)dc.hConnection, 8192);
  packetRecv.cbSize = sizeof(packetRecv);
  packetRecv.bytesUsed = 0;

  // Packet receiving loop

  while (dc.hConnection)
  {
    int recvResult;

    packetRecv.dwTimeout = dc.wantIdleTime ? 0 : 600000;

    recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM)hPacketRecver, (LPARAM)&packetRecv);
    if (recvResult == 0)
    {
      NetLog_Direct("Clean closure of direct socket (%p)", dc.hConnection);
      break;
    }

    if (recvResult == SOCKET_ERROR)
    {
      if (GetLastError() == ERROR_TIMEOUT)
      { // TODO: this will not work on some systems
        if (dc.wantIdleTime)
        {
          switch (dc.type)
          {
            case DIRECTCONN_FILE:
              handleFileTransferIdle(&dc);
              break;
          }
        }
        else if (dc.packetPending)
        { // do we expect packet soon?
          NetLog_Direct("Keeping connection, packet pending.");
        }
        else
        {
          NetLog_Direct("Connection inactive for 10 minutes, closing.");
          break;
        }
      }
      else
      {
        NetLog_Direct("Abortive closure of direct socket (%p) (%d)", dc.hConnection, GetLastError());
        break;
      }
    }

    if (dc.type == DIRECTCONN_CLOSING)
      packetRecv.bytesUsed = packetRecv.bytesAvailable;
    else if (packetRecv.bytesAvailable < nSkipPacketBytes)
    { // the whole buffer needs to be skipped
      nSkipPacketBytes -= packetRecv.bytesAvailable;
      packetRecv.bytesUsed = packetRecv.bytesAvailable;
    }
    else
    {
      int i;

      for (i = nSkipPacketBytes, nSkipPacketBytes = 0; i + 2 <= packetRecv.bytesAvailable;)
      { 
        WORD wLen = *(WORD*)(packetRecv.buffer + i);

        if (bFirstPacket)
        {
          if (wLen > 64)
          { // roughly check first packet size
            NetLog_Direct("Error: Overflowed packet, closing connection.");
            CloseDirectConnection(&dc);
            break;
          }
          bFirstPacket = FALSE;
        }
        else
        {
          if (packetRecv.bytesAvailable >= i + 2 && wLen > 8190)
          { // check for too big packages
            NetLog_Direct("Error: Package too big: %d bytes, skipping.");
            nSkipPacketBytes = wLen;
            packetRecv.bytesUsed = i + 2;
            break;
          }
        }

        if (wLen + 2 + i > packetRecv.bytesAvailable)
          break;

        if (dc.type == DIRECTCONN_STANDARD && wLen && packetRecv.buffer[i + 2] == 2)
        {
          if (!DecryptDirectPacket(&dc, packetRecv.buffer + i + 3, (WORD)(wLen - 1)))
          {
            NetLog_Direct("Error: Corrupted packet encryption, ignoring packet");
            i += wLen + 2;
            continue;
          }
        }
#ifdef _DEBUG
        NetLog_Direct("New direct package");
#endif
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

  NetLib_SafeCloseHandle(&hPacketRecver, FALSE);
  CloseDirectConnection(&dc);

  if (dc.ft)
  {
    if (dc.ft->fileId != -1)
    {
      _close(dc.ft->fileId);
      ICQBroadcastAck(dc.ft->hContact, ACKTYPE_FILE, dc.ft->dwBytesDone==dc.ft->dwTotalSize ? ACKRESULT_SUCCESS : ACKRESULT_FAILED, dc.ft, 0);
    }
    else if (dc.ft->hConnection)
      ICQBroadcastAck(dc.ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, dc.ft, 0);

    SafeReleaseFileTransfer(&dc.ft);
    _chdir("\\");    /* so we don't leave a subdir handle open so it can't be deleted */
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
    case PEER_FILE_INIT: // first packet of a file transfer
#ifdef _DEBUG
      NetLog_Direct("Received PEER_FILE_INIT from %u",dc->dwRemoteUin);
#endif
      if (dc->handshake)
        handleFileTransferPacket(dc, buf, wLen);
      else
        NetLog_Direct("Received %s on uninitialised DC, ignoring.", "PEER_FILE_INIT");

      break;

    case PEER_INIT_ACK: // This is sent as a response to our PEER_INIT packet
      if (wLen != 4)
      {
        NetLog_Direct("Error: Received malformed PEER_INITACK from %u", dc->dwRemoteUin);
        break;
      }
#ifdef _DEBUG
      NetLog_Direct("Received PEER_INITACK from %u on %s DC", dc->dwRemoteUin, dc->incoming?"incoming":"outgoing");
#endif
      if (dc->incoming) dc->handshake = 1;

      if (dc->incoming && dc->type == DIRECTCONN_REVERSE)
      {
        reverse_cookie* pCookie;

        dc->incoming = 0;

        if (FindCookie(dc->dwReqId, NULL, &pCookie) && pCookie)
        { // valid reverse DC, check and init session
          FreeCookie(dc->dwReqId);
          if (pCookie->dwUin == dc->dwRemoteUin)
          { // valid connection
            dc->type = pCookie->type;
            dc->ft = pCookie->ft;
            dc->hContact = pCookie->hContact;
            if (dc->type == DIRECTCONN_STANDARD)
            { // init message session
              sendPeerMsgInit(dc, 0);
            }
            else if (dc->type == DIRECTCONN_FILE)
            { // init file session
              sendPeerFileInit(dc);
              dc->initialised = 1;
            }
            SAFE_FREE(&pCookie);
            break;
          }
          else
          {
            SAFE_FREE(&pCookie);
            NetLog_Direct("Error: Invalid connection (UINs does not match).");
            CloseDirectConnection(dc);
            return;
          }
        }
        else
        {
          NetLog_Direct("Error: Received unexpected reverse DC, closing.");
          CloseDirectConnection(dc);
          return; 
        }
      }
      break;

    case PEER_INIT:       /* connect packet */
#ifdef _DEBUG
      NetLog_Direct("Received PEER_INIT");
#endif
      buf++;

      if (wLen < 3)
        return;
      
      unpackLEWord(&buf, &dc->wVersion);

      if (dc->wVersion > 6)
      { // we support only versions 7 and up
        WORD wSecondLen;
        DWORD dwUin;
        DWORD dwPort;
        DWORD dwCookie;
        HANDLE hContact;

        if (wLen != 0x30)
        {
          NetLog_Direct("Error: Received malformed PEER_INIT");
          return;
        }

        unpackLEWord(&buf, &wSecondLen);
        if (wSecondLen && wSecondLen != 0x2b)
        { // OMG? GnomeICU sets this to zero
          NetLog_Direct("Error: Received malformed PEER_INIT");
          return;
        }
        
        unpackLEDWord(&buf, &dwUin);
        if (dwUin != dwLocalUIN)
        {
          NetLog_Direct("Error: Received PEER_INIT targeted to %u", dwUin);
          CloseDirectConnection(dc);
          return;
        }
        
        buf += 2;   /* 00 00 */
        unpackLEDWord(&buf, &dc->dwRemotePort);
        unpackLEDWord(&buf, &dc->dwRemoteUin);
        unpackDWord(&buf, &dc->dwRemoteExternalIP);
        unpackDWord(&buf, &dc->dwRemoteInternalIP);
        buf ++;     /* 04: accept direct connections */
        unpackLEDWord(&buf, &dwPort);
        if (dwPort != dc->dwRemotePort)
        {
          NetLog_Direct("Error: Received malformed PEER_INIT (invalid port)");
          return;
        }
        unpackLEDWord(&buf, &dwCookie);

        buf += 8; // Unknown stuff
        unpackLEDWord(&buf, &dc->dwReqId);

        if (dc->dwRemoteUin || !dc->dwReqId)
        { // OMG! Licq sends on reverse connection empty uin
          hContact = HContactFromUIN(dc->dwRemoteUin, NULL);
          if (hContact == INVALID_HANDLE_VALUE)
          {
            NetLog_Direct("Error: Received PEER_INIT from %u not on my list", dwUin);
            CloseDirectConnection(dc);
            return;   /* don't allow direct connection with people not on my clist */
          }

          if (dc->incoming)
          { // this is the first PEER_INIT with our cookie
            if (dwCookie != ICQGetContactSettingDword(hContact, "DirectCookie", 0))
            {
              NetLog_Direct("Error: Received PEER_INIT with broken cookie");
              CloseDirectConnection(dc);
              return;
            }
          }
          else
          { // this is the second PEER_INIT with peer cookie
            if (dwCookie != dc->dwConnectionCookie)
            {
              NetLog_Direct("Error: Received PEER_INIT with broken cookie");
              CloseDirectConnection(dc);
              return;
            }
          }
        }

        if (dc->incoming && dc->dwReqId)
        { // this is reverse connection
          dc->type = DIRECTCONN_REVERSE;
          if (!dc->dwRemoteUin)
          { // we need to load cookie (licq)
            reverse_cookie* pCookie;

            if (FindCookie(dc->dwReqId, NULL, &pCookie) && pCookie)
            { // valid reverse DC, check and init session
              dc->dwRemoteUin = pCookie->dwUin;
              dc->hContact = pCookie->hContact;
            }
            else
            {
              NetLog_Direct("Error: Received unexpected reverse DC, closing.");
              CloseDirectConnection(dc);
              return; 
            }
          }
        }

        sendPeerInitAck(dc); // ack good PEER_INIT packet

        if (dc->incoming)
        { // store good IP info
          dc->hContact = hContact;
          dc->dwConnectionCookie = dwCookie;
          ICQWriteContactSettingDword(dc->hContact, "IP", dc->dwRemoteExternalIP); 
          ICQWriteContactSettingDword(dc->hContact, "RealIP", dc->dwRemoteInternalIP);
          sendPeerInit_v78(dc); // reply with our PEER_INIT
        }
        else // outgoing
        {
          dc->handshake = 1;

          if (dc->type == DIRECTCONN_REVERSE)
          {
            dc->incoming = 1; // this is incoming reverse connection
            dc->type = DIRECTCONN_STANDARD; // we still do not know type
          }
          else if (dc->type == DIRECTCONN_STANDARD)
          { // send PEER_MSGINIT
            sendPeerMsgInit(dc, 0);
          }
          else if (dc->type == DIRECTCONN_FILE)
          {
            sendPeerFileInit(dc);
            dc->initialised = 1;
          }
        }
        // Set DC Status to successful
        ICQWriteContactSettingByte(dc->hContact, "DCStatus", 0);
      }
      else
      {
        NetLog_Direct("Unsupported direct protocol: %d, closing connection", dc->wVersion);
        CloseDirectConnection(dc);
      }
      break;

    case PEER_MSG:        /* messaging packets */
#ifdef _DEBUG
      NetLog_Direct("Received PEER_MSG from %u", dc->dwRemoteUin);
#endif
      if (dc->initialised)
        handleDirectMessage(dc, buf + 1, (WORD)(wLen - 1));
      else
        NetLog_Direct("Received %s on uninitialised DC, ignoring.", "PEER_MSG");

      break;

    case PEER_MSG_INIT:   /* init message connection */
      { // it is sent by both contains GUID of message channel
        DWORD q1,q2,q3,q4;

        if (!gbDCMsgEnabled)
        { // DC messaging disabled, close connection
          NetLog_Direct("Messaging DC requested, denied");
          CloseDirectConnection(dc);
          break;
        }

#ifdef _DEBUG
        NetLog_Direct("Received PEER_MSG_INIT from %u",dc->dwRemoteUin);
#endif
        buf++;
        if (wLen != 0x21)
          break;

        if (!dc->handshake)
        {
          NetLog_Direct("Received %s on unitialised DC, ignoring.", "PEER_MSG_INIT");
          break;
        }

        buf += 4;   /* always 10 */
        buf += 4;   /* some id */
        buf += 4;   /* sequence - always 0 on incoming */
        unpackDWord(&buf, &q1);   // session type GUID
        unpackDWord(&buf, &q2);
        if (!dc->incoming)
        { // skip marker on sequence 1
          buf += 4;
        }
        unpackDWord(&buf, &q3);
        unpackDWord(&buf, &q4);
        if (!CompareGUIDs(q1,q2,q3,q4,PSIG_MESSAGE))
        { // This is not for normal messages, useless so kill.
          if (CompareGUIDs(q1,q2,q3,q4,PSIG_STATUS_PLUGIN))
          {
            NetLog_Direct("Status Manager Plugin connections not supported, closing.");
          }
          else if (CompareGUIDs(q1,q2,q3,q4,PSIG_INFO_PLUGIN))
          {
            NetLog_Direct("Info Manager Plugin connection not supported, closing.");
          }
          else
          {
            NetLog_Direct("Unknown connection type init, closing.");
          }
          CloseDirectConnection(dc);
          break;
        }

        if (dc->incoming)
        { // reply with our PEER_MSG_INIT
          sendPeerMsgInit(dc, 1);
        }
        else
        { // connection initialized, ready to send message packet
        }
        NetLog_Direct("Direct message session ready.");
        dc->initialised = 1;
      }
      break;

    default:
      NetLog_Direct("Unknown direct packet ignored.");
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
  if (offset)
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
#ifdef _DEBUG
  { // log decrypted data
    char szTitleLine[128];
    char* szBuf;
    int titleLineLen;
    int line;
    int col;
    int colsInLine;
    char* pszBuf;


    titleLineLen = null_snprintf(szTitleLine, 128, "DECRYPTED\n");
    szBuf = (char*)_alloca(titleLineLen + ((wLen+15)>>4) * 76 + 1);
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

    CallService(MS_NETLIB_LOG,(WPARAM)ghDirectNetlibUser, (LPARAM)szBuf);
  }
#endif

  return 1;
}



// Sends a PEER_INIT packet through a DC
// -----------------------------------------------------------------------
// This packet is sent during direct connection initialization between two
// ICQ clients. It is sent by the originator of the connection to start
// the handshake and by the receiver directly after it has sent the
// PEER_ACK packet as a reply to the originator's PEER_INIT. The values
// after the COOKIE field have been added for v7.
static void sendPeerInit_v78(directconnect* dc)
{
  icq_packet packet;

  directPacketInit(&packet, 48);             // Full packet length
  packByte(&packet, PEER_INIT);              // Command
  packLEWord(&packet, dc->wVersion);         // Version
  packLEWord(&packet, 43);                   // Data length
  packLEDWord(&packet, dc->dwRemoteUin);     // UIN of remote user
  packWord(&packet, 0);                      // Unknown
  packLEDWord(&packet, wListenPort);         // Our port
  packLEDWord(&packet, dwLocalUIN);          // Our UIN
  packDWord(&packet, dc->dwLocalExternalIP); // Our external IP
  packDWord(&packet, dc->dwLocalInternalIP); // Our internal IP
  packByte(&packet, DC_TYPE);                // TCP connection flags
  packLEDWord(&packet, wListenPort);         // Our port
  packLEDWord(&packet, dc->dwConnectionCookie); // DC cookie
  packLEDWord(&packet, WEBFRONTPORT);        // Unknown
  packLEDWord(&packet, CLIENTFEATURES);      // Unknown
  if (dc->type == DIRECTCONN_REVERSE)
    packLEDWord(&packet, dc->dwReqId);       // Reverse Request Cookie
  else
    packDWord(&packet, 0);                   // Unknown

  sendDirectPacket(dc, &packet);
#ifdef _DEBUG
  NetLog_Direct("Sent PEER_INIT to %u on %s DC", dc->dwRemoteUin, dc->incoming?"incoming":"outgoing");
#endif
}



// Sends a PEER_INIT packet through a DC
// -----------------------------------------------------------------------
// This is sent to acknowledge a PEER_INIT packet.
static void sendPeerInitAck(directconnect* dc)
{
  icq_packet packet;

  directPacketInit(&packet, 4);              // Packet length
  packLEDWord(&packet, PEER_INIT_ACK);       // 

  sendDirectPacket(dc, &packet);
#ifdef _DEBUG
  NetLog_Direct("Sent PEER_INIT_ACK to %u on %s DC", dc->dwRemoteUin, dc->incoming?"incoming":"outgoing");
#endif
}



// Sends a PEER_MSG_INIT packet through a DC
// -----------------------------------------------------------------------
// This packet starts message session.
static void sendPeerMsgInit(directconnect* dc, DWORD dwSeq)
{
  icq_packet packet;

  directPacketInit(&packet, 33);
  packByte(&packet, PEER_MSG_INIT);
  packLEDWord(&packet, 10);           // unknown
  packLEDWord(&packet, 1);            // message connection
  packLEDWord(&packet, dwSeq);        // sequence is 0,1
  if (!dwSeq)
  {
    packGUID(&packet, PSIG_MESSAGE);  // message type GUID
    packLEWord(&packet, 1);           // delimiter
    packLEWord(&packet, 4);
  }
  else
  {
    packDWord(&packet, 0);            // first part of Message GUID
    packDWord(&packet, 0);
    packLEWord(&packet, 1);           // delimiter
    packLEWord(&packet, 4);
    packDWord(&packet, 0);            // second part of Message GUID
    packDWord(&packet, 0);
  }
  sendDirectPacket(dc, &packet);
#ifdef _DEBUG
  NetLog_Direct("Sent PEER_MSG_INIT to %u on %s DC", dc->dwRemoteUin, dc->incoming?"incoming":"outgoing");
#endif
}



// Sends a PEER_FILE_INIT packet through a DC
// -----------------------------------------------------------------------
// This packet configures file-transfer session.
static void sendPeerFileInit(directconnect* dc)
{
  icq_packet packet;
  DBVARIANT dbv;
  char* szNick;
  int nNickLen;

  dbv.type = DBVT_DELETED;
  if (ICQGetContactSettingString(NULL, "Nick", &dbv))
    szNick = "";
  else
    szNick = dbv.pszVal;
  nNickLen = strlennull(szNick);

  directPacketInit(&packet, (WORD)(20 + nNickLen));
  packByte(&packet, PEER_FILE_INIT);  /* packet type */
  packLEDWord(&packet, 0);            /* unknown */
  packLEDWord(&packet, dc->ft->dwFileCount);
  packLEDWord(&packet, dc->ft->dwTotalSize);
  packLEDWord(&packet, dc->ft->dwTransferSpeed);
  packLEWord(&packet, (WORD)(nNickLen + 1));
  packBuffer(&packet, szNick, (WORD)(nNickLen + 1));
  sendDirectPacket(dc, &packet);
#ifdef _DEBUG
  NetLog_Direct("Sent PEER_FILE_INIT to %u on %s DC", dc->dwRemoteUin, dc->incoming?"incoming":"outgoing");
#endif
  ICQFreeVariant(&dbv);
}
