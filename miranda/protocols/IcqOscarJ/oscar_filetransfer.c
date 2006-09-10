// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006 Joe Kucera
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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/oscar_filetransfer.c,v $
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


typedef struct {
  int type;
  int incoming;
  HANDLE hContact;
  HANDLE hConnection;
  DWORD dwRemoteIP;
  oscar_filetransfer *ft;
  oscar_listener *listener;
} oscarthreadstartinfo;


// small utility function
extern void NormalizeBackslash(char* path);


static DWORD __stdcall oft_connectionThread(oscarthreadstartinfo *otsi);
static void sendOscarPacket(oscar_connection *oc, icq_packet *packet);
static int oft_handlePackets(oscar_connection *oc, unsigned char *buf, int len);
static int oft_handleFileData(oscar_connection *oc, unsigned char *buf, int len);
static void handleOFT2FramePacket(oscar_connection *oc, WORD datatype, BYTE *pBuffer, WORD wLen);
static void sendOFT2FramePacket(oscar_connection *oc, WORD datatype);

static void oft_sendPeerInit(oscar_connection *oc);

static CRITICAL_SECTION oftMutex;
static int oftTransferCount = 0;
static oscar_filetransfer** oftTransferList = NULL;

static oscar_filetransfer* CreateOscarTransfer();
static void SafeReleaseOscarTransfer(oscar_filetransfer *ft);
static int IsValidOscarTransfer(void *ft);
static oscar_filetransfer* FindOscarTransfer(HANDLE hContact, DWORD dwID1, DWORD dwID2);


//
// Utility functions
/////////////////////////////


static oscar_filetransfer* CreateOscarTransfer()
{
  oscar_filetransfer* ft = (oscar_filetransfer*)SAFE_MALLOC(sizeof(oscar_filetransfer));

  EnterCriticalSection(&oftMutex);

  oftTransferList = (oscar_filetransfer**)realloc(oftTransferList, sizeof(oscar_filetransfer*)*(oftTransferCount + 1));
  oftTransferList[oftTransferCount++] = ft;

  LeaveCriticalSection(&oftMutex);

  return ft;
}



static void SafeReleaseOscarTransfer(oscar_filetransfer *ft)
{
  int i;

  EnterCriticalSection(&oftMutex);

  for (i = 0; i < oftTransferCount; i++)
  {
    if (oftTransferList[i] == ft)
    {
      oftTransferCount--;
      SafeReleaseFileTransfer(&oftTransferList[i]);
      oftTransferList[i] = oftTransferList[oftTransferCount];
      oftTransferList = (oscar_filetransfer**)realloc(oftTransferList, sizeof(oscar_filetransfer*)*oftTransferCount);
      break;
    }
  }

  LeaveCriticalSection(&oftMutex);
}



static int IsValidOscarTransfer(void *ft)
{
  int i;

  EnterCriticalSection(&oftMutex);

  for (i = 0; i < oftTransferCount; i++)
  {
    if (oftTransferList[i] == ft)
    {
      LeaveCriticalSection(&oftMutex);

      return 1;
    }
  }

  LeaveCriticalSection(&oftMutex);

  return 0;
}



static oscar_filetransfer* FindOscarTransfer(HANDLE hContact, DWORD dwID1, DWORD dwID2)
{
  int i;

  EnterCriticalSection(&oftMutex);

  for (i = 0; i < oftTransferCount; i++)
  {
    if (oftTransferList[i]->hContact == hContact && oftTransferList[i]->pMessage.dwMsgID1 == dwID1 && oftTransferList[i]->pMessage.dwMsgID2 == dwID2)
    {
      LeaveCriticalSection(&oftMutex);

      return oftTransferList[i];
    }
  }

  LeaveCriticalSection(&oftMutex);

  return NULL;
}



// Release file transfer structure
void SafeReleaseFileTransfer(void **ft)
{
  basic_filetransfer **bft = (basic_filetransfer**)ft;

  if (*bft)
  {
    if ((*bft)->ft_magic == FT_MAGIC_ICQ)
    { // release ICQ filetransfer structure and its contents
      filetransfer *ift = (filetransfer*)(*bft);

      SAFE_FREE(&ift->szFilename);
      SAFE_FREE(&ift->szDescription);
      SAFE_FREE(&ift->szSavePath);
      SAFE_FREE(&ift->szThisFile);
      SAFE_FREE(&ift->szThisSubdir);
      if (ift->files)
      {
        int i;

        for (i = 0; i < (int)ift->dwFileCount; i++)
          SAFE_FREE(&ift->files[i]);
        SAFE_FREE((char**)&ift->files);
      }
      SAFE_FREE(ft);
    }
    else
    { // release oscar filetransfer structure and its contents
      oscar_filetransfer *oft = (oscar_filetransfer*)(*bft);
      // FIX ME: release all dynamic members
      if (oft->listener)
        ReleaseOscarListener((oscar_listener**)&oft->listener);
      SAFE_FREE(&oft->rawFileName);
      SAFE_FREE(&oft->szSavePath);
      SAFE_FREE(&oft->szThisFile);
      if (oft->files)
      {
        int i;

        for (i = 0; i < oft->wFilesCount; i++)
          SAFE_FREE(&oft->files[i]);
        SAFE_FREE((char**)&oft->files);
      }
      SAFE_FREE(ft);
    }
  }
}




// Calculate oft checksum of buffer
// --------------------------------
// Information was gathered from Gaim's sources, thanks
//
DWORD oft_calc_checksum(int offset, const BYTE *buffer, int len, DWORD dwChecksum)
{
  DWORD checksum;
  int i;

  checksum = (dwChecksum >> 16) & 0xffff;
  for (i = 0; i < len; i++)
  {
    WORD val = buffer[i];
    DWORD oldchecksum = checksum;

    if (((i + offset) & 1) == 0)
      val = val << 8;

    if (checksum < val)
      checksum -= val + 1;
    else // simulate carry
      checksum -= val;
  }
  checksum = ((checksum & 0x0000ffff) + (checksum >> 16));
  checksum = ((checksum & 0x0000ffff) + (checksum >> 16));
  return checksum << 16;
}



oscar_listener* CreateOscarListener(oscar_filetransfer *ft, NETLIBNEWCONNECTIONPROC_V2 handler)
{
  oscar_listener *listener = (oscar_listener*)SAFE_MALLOC(sizeof(oscar_listener));

  listener->ft = ft;
  if (listener->hBoundPort = NetLib_BindPort(handler, listener, &listener->wPort, NULL))
    return listener; // Success

  SAFE_FREE(&listener);

  return NULL; // Failure
}



void ReleaseOscarListener(oscar_listener **pListener)
{
  oscar_listener *listener = *pListener;

  if (listener)
  { // Close listening port
    if (listener->hBoundPort)
      NetLib_SafeCloseHandle(&listener->hBoundPort, FALSE);
  }
  SAFE_FREE(pListener);
}


//
// Miranda FT interface handlers & services
/////////////////////////////

void InitOscarFileTransfer()
{
  InitializeCriticalSection(&oftMutex);
}



void UninitOscarFileTransfer()
{
  DeleteCriticalSection(&oftMutex);
}



void handleRecvServMsgOFT(unsigned char *buf, WORD wLen, DWORD dwUin, char *szUID, DWORD dwID1, DWORD dwID2, WORD wCommand)
{
  if (wCommand == 0)
  { // this is OFT request
    oscar_tlv_chain* chain = readIntoTLVChain(&buf, wLen, 0);

    if (chain)
    {
      HANDLE hContact = HContactFromUID(dwUin, szUID, NULL);
      WORD wAckType = getWordFromChain(chain, 0x0A, 1);

      if (wAckType == 1)
      { // This is first request in this OFT
        oscar_filetransfer *ft = CreateOscarTransfer();
        char* pszFileName = NULL;
        char* pszDescription = NULL;
        WORD wFilenameLength;

        NetLog_Server("This is a file request");

        // This TLV chain may contain the following TLVs:
        // TLV(A): Acktype 0x0001 - file request / abort request
        //                 0x0002 - file ack
        // TLV(F): Unknown
        // TLV(E): Language ?
        // TLV(2): Proxy IP
        // TLV(16): Proxy IP Check
        // TLV(3): External IP
        // TLV(4): Internal IP
        // TLV(5): Port
        // TLV(17): Port Check
        // TLV(10): Proxy Flag
        // TLV(D): Charset of User Message
        // TLV(C): User Message (ICQ_COOL_FT)
        // TLV(2711): FT info
        // TLV(2712): Charset of file name

        ft->ft_magic = FT_MAGIC_OSCAR;
        // init filetransfer structure
        ft->pMessage.dwMsgID1 = dwID1;
        ft->pMessage.dwMsgID2 = dwID2;
        ft->bUseProxy = getTLV(chain, 0x10, 1) ? 1 : 0;
        ft->dwProxyIP = getDWordFromChain(chain, 0x02, 1);
        ft->dwRemoteExternalIP = getDWordFromChain(chain, 0x03, 1);
        ft->dwRemoteInternalIP = getDWordFromChain(chain, 0x04, 1);
        ft->dwRemotePort = getWordFromChain(chain, 0x05, 1);

        { // User Message
          oscar_tlv* tlv = getTLV(chain, 0x0C, 1);

          if (tlv)
          { // parse User Message
            BYTE* tBuf = tlv->pData;

            pszDescription = (char*)_alloca(tlv->wLen + 1);
            unpackString(&tBuf, pszDescription, tlv->wLen);
            pszDescription[tlv->wLen] = '\0';
            { // apply User Message encoding
              oscar_tlv *charset = getTLV(chain, 0x0D, 1);
              char *str = pszDescription;
              char *bTag,*eTag;

              if (charset)
              { // decode charset
                char *szEnc = (char*)_alloca(charset->wLen + 1);

                strncpy(szEnc, charset->pData, charset->wLen);
                szEnc[charset->wLen] = '\0';
                str = ApplyEncoding(pszDescription, szEnc);
              }
              // decode XML tags
              str = DemangleXml(str, strlennull(str));
              if (charset) SAFE_FREE(&pszDescription);
              pszDescription = str;

              bTag = strstr(str, "<FS>");
              if (bTag)
              { // take only <FS> - Description tag if present
                eTag = strstr(bTag, "</FS>");
                if (eTag)
                {
                  *eTag = '\0';
                  pszDescription = null_strdup(bTag + 4);
                  SAFE_FREE(&str);
                }
              }
            }

          }
          if (!strlennull(pszDescription)) pszDescription = ICQTranslate("No description given");
        }
        { // parse File Transfer Info block
          oscar_tlv* tlv = getTLV(chain, 0x2711, 1);
          BYTE* tBuf = tlv->pData;
          WORD tLen = tlv->wLen;

          tBuf += 2; // FT flag
          unpackWord(&tBuf, &ft->wFilesCount);
          unpackDWord(&tBuf, &ft->dwTotalSize);
          tLen -= 8;
          // Filename
          wFilenameLength = tLen - 1;
          pszFileName = (char*)_alloca(tLen);
          unpackString(&tBuf, pszFileName, wFilenameLength);
          pszFileName[wFilenameLength] = '\0';
          { // apply Filename encoding
            oscar_tlv* charset = getTLV(chain, 0x2712, 1);

            if (charset)
            {
              char* szEnc = (char*)_alloca(charset->wLen + 1);

              strncpy(szEnc, charset->pData, charset->wLen);
              szEnc[charset->wLen] = '\0';
              pszFileName = ApplyEncoding(pszFileName, szEnc);
            }
            else // needs to be alloced
              pszFileName = null_strdup(pszFileName);
          }
        }
        {
          CCSDATA ccs;
          PROTORECVEVENT pre;
          char* szBlob;
          int bAdded;
          HANDLE hContact = HContactFromUID(dwUin, szUID, &bAdded);

          ft->hContact = hContact;
          ft->szFilename = pszFileName;
          ft->szDescription = pszDescription;
          ft->fileId = -1;

          // Send chain event
          szBlob = (char*)_alloca(sizeof(DWORD) + strlennull(pszFileName) + strlennull(pszDescription) + 2);
          *(PDWORD)szBlob = (DWORD)ft;
          strcpy(szBlob + sizeof(DWORD), pszFileName);
          strcpy(szBlob + sizeof(DWORD) + strlennull(pszFileName) + 1, pszDescription);
          ccs.szProtoService = PSR_FILE;
          ccs.hContact = hContact;
          ccs.wParam = 0;
          ccs.lParam = (LPARAM)&pre;
          pre.flags = 0;
          pre.timestamp = time(NULL);
          pre.szMessage = szBlob;
          pre.lParam = 0;

          CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
        }
      }
      else if (wAckType == 2)
      { // First attempt failed, reverse requested
      }
      else if (wAckType == 3)
      { // Reverse attempt failed, trying proxy
      }

      disposeChain(&chain);
    }
    else
      NetLog_Server("Error: Missing TLV chain in OFT request");
  }
  else
  { // TODO: handle other commands / cancels
  }
}



DWORD oftFileAllow(HANDLE hContact, WPARAM wParam, LPARAM lParam)
{
  oscar_filetransfer* ft = (oscar_filetransfer*)wParam;

  ft->szSavePath = null_strdup((char *)lParam);

  OpenOscarConnection(hContact, ft, ft->bUseProxy ? OCT_PROXY: OCT_NORMAL);

  return wParam; // Success
}



DWORD oftFileDeny(HANDLE hContact, WPARAM wParam, LPARAM lParam)
{
  oscar_filetransfer* ft = (oscar_filetransfer*)wParam;
  DWORD dwUin;
  uid_str szUid;

  if (ICQGetContactSettingUID(hContact, &dwUin, &szUid))
    return 1; // Invalid contact

  oft_sendFileDeny(dwUin, szUid, ft);

  SafeReleaseOscarTransfer(ft);

  return 0; // Success
}



DWORD oftFileCancel(HANDLE hContact, WPARAM wParam, LPARAM lParam)
{
  oscar_filetransfer* ft = (oscar_filetransfer*)wParam;
  DWORD dwUin;
  uid_str szUid;

  if (ft->hContact != hContact)
    return 1; // Bad contact or hTransfer

  if (ICQGetContactSettingUID(hContact, &dwUin, &szUid))
    return 1; // Invalid contact

  oft_sendFileDeny(dwUin, szUid, ft);

  ICQBroadcastAck(hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0);

  if (ft->connection)
  {
    CloseOscarConnection((oscar_connection*)ft->connection);
  }
  // Release structure
  SafeReleaseOscarTransfer(ft);

  return 0; // Success
}



void oftFileResume(oscar_filetransfer *ft, int action, const char *szFilename)
{
  oscar_connection *oc;
  int openFlags;

  if (ft->connection == NULL)
    return;

  oc = (oscar_connection*)ft->connection;

  switch (action)
  {
    case FILERESUME_RESUME: // FIX ME: this needs to request resume!!!
      openFlags = _O_BINARY | _O_WRONLY;
      break;

    case FILERESUME_OVERWRITE:
      openFlags = _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY;
      ft->dwFileBytesDone = 0;
      break;

    case FILERESUME_SKIP:
      openFlags = _O_BINARY | _O_WRONLY;
      ft->dwFileBytesDone = ft->dwThisFileSize;
      break;

    case FILERESUME_RENAME:
      openFlags = _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY;
      SAFE_FREE(&ft->szThisFile);
      ft->szThisFile = null_strdup(szFilename);
      ft->dwFileBytesDone = 0;
      break;
  }

  // TODO: unicode support
  ft->fileId = _open(ft->szThisFile, openFlags, _S_IREAD | _S_IWRITE);
  if (ft->fileId == -1)
  {
    icq_LogMessage(LOG_ERROR, "Your file receive has been aborted because Miranda could not open the destination file in order to write to it. You may be trying to save to a read-only folder.");

    CloseOscarConnection(oc);
    return;
  }

  if (action == FILERESUME_RESUME)
    ft->dwFileBytesDone = _lseek(ft->fileId, 0, SEEK_END);
  else
    _lseek(ft->fileId, ft->dwFileBytesDone, SEEK_SET);

  ft->dwBytesDone += ft->dwFileBytesDone;

  // Send "we are ready"
  oc->status = OCS_DATA;

  sendOFT2FramePacket(oc, OFT_TYPE_READY);
  ICQBroadcastAck(ft->hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, ft, 0);

  if (!ft->dwThisFileSize)
  { // if the file is empty we will not receive any data
    BYTE buf;
    oft_handleFileData(oc, &buf, 0);
  }
}



static void oft_buildProtoFileTransferStatus(oscar_filetransfer* ft, PROTOFILETRANSFERSTATUS* pfts)
{
  ZeroMemory(pfts, sizeof(PROTOFILETRANSFERSTATUS));
  pfts->cbSize = sizeof(PROTOFILETRANSFERSTATUS);
  pfts->hContact = ft->hContact;
  pfts->sending = ft->sending;
  if (ft->sending)
    pfts->files = ft->files;
  else
    pfts->files = NULL;  /* FIXME */
  pfts->totalFiles = ft->wFilesCount;
  pfts->currentFileNumber = ft->iCurrentFile;
  pfts->totalBytes = ft->dwTotalSize;
  pfts->totalProgress = ft->dwBytesDone;
  pfts->workingDir = ft->szSavePath;
  pfts->currentFile = ft->szThisFile;
  pfts->currentFileSize = ft->dwThisFileSize;
  pfts->currentFileTime = ft->dwThisFileDate;
  pfts->currentFileProgress = ft->dwFileBytesDone;
}



void CloseOscarConnection(oscar_connection *oc)
{
  EnterCriticalSection(&oftMutex);

  if (oc)
  {
    oc->type = OCT_CLOSING;

    if (oc->hConnection)
    { // we need this for Netlib handle consistency
      NetLib_SafeCloseHandle(&oc->hConnection, FALSE);
    }
  }
  LeaveCriticalSection(&oftMutex);
}



void OpenOscarConnection(HANDLE hContact, oscar_filetransfer *ft, int type)
{
  pthread_t tid;
  oscarthreadstartinfo *otsi = (oscarthreadstartinfo*)SAFE_MALLOC(sizeof(oscarthreadstartinfo));

  otsi->hContact = hContact;
  otsi->type = type;
  otsi->ft = ft;

  tid.hThread = (HANDLE)forkthreadex(NULL, 0, oft_connectionThread, otsi, 0, &tid.dwThreadId);
  CloseHandle(tid.hThread);
}



// This function is called from the Netlib when someone is connecting to our oscar_listener
void oft_newConnectionReceived(HANDLE hNewConnection, DWORD dwRemoteIP, void *pExtra)
{
  pthread_t tid;
  oscarthreadstartinfo *otsi = (oscarthreadstartinfo*)SAFE_MALLOC(sizeof(oscarthreadstartinfo));
  oscar_listener* listener = (oscar_listener*)pExtra;

  otsi->type = listener->ft->sending ? OCT_NORMAL : OCT_REVERSE;
  otsi->incoming = 1;
  otsi->hConnection = hNewConnection;
  otsi->dwRemoteIP = dwRemoteIP;
  otsi->listener = listener;

  // Start a new thread for the incomming connection
  tid.hThread = (HANDLE)forkthreadex(NULL, 0, oft_connectionThread, otsi, 0, &tid.dwThreadId);
  CloseHandle(tid.hThread);
}



static DWORD __stdcall oft_connectionThread(oscarthreadstartinfo *otsi)
{
  oscar_connection oc = {0};
  oscar_listener *source;
  NETLIBPACKETRECVER packetRecv={0};
  HANDLE hPacketRecver;

  oc.hContact = otsi->hContact;
  oc.hConnection = otsi->hConnection;
  oc.type = otsi->type;
  oc.incoming = otsi->incoming;
  oc.ft = otsi->ft;
  source = otsi->listener;
  if (oc.incoming)
  {
    if (IsValidOscarTransfer(source->ft))
    {
      oc.ft = source->ft;
      oc.ft->dwRemoteExternalIP = otsi->dwRemoteIP;
      oc.hContact = oc.ft->hContact;
      oc.ft->connection = &oc;
    }
    else
    { // FT is already over, kill listener
      NetLog_Direct("Received unexpected connection, closing.");

      CloseOscarConnection(&oc);
      ReleaseOscarListener(&source);

      return 0;
    }
  }
  SAFE_FREE(&otsi);

  if (oc.hContact)
  { // Load contact information
    ICQGetContactSettingUID(oc.hContact, &oc.dwUin, &oc.szUid);
  }

  // Load local IP information
  oc.dwLocalExternalIP = ICQGetContactSettingDword(NULL, "IP", 0);
  oc.dwLocalInternalIP = ICQGetContactSettingDword(NULL, "RealIP", 0);

  if (!oc.incoming)
  { // FIX ME: this needs more work for proxy & sending
    if (oc.type == OCT_NORMAL)
    { // create outgoing connection to peer
      NETLIBOPENCONNECTION nloc = {0};
      IN_ADDR addr = {0};

      nloc.cbSize = sizeof(nloc);

      // FIX ME: Just for testing reverse
      if (oc.ft->dwRemoteExternalIP == oc.dwLocalExternalIP && oc.ft->dwRemoteInternalIP)
        addr.S_un.S_addr = htonl(oc.ft->dwRemoteInternalIP);
      else
        addr.S_un.S_addr = htonl(oc.ft->dwRemoteExternalIP);

      // Inform UI that we will attempt to connect
      ICQBroadcastAck(oc.ft->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTING, oc.ft, 0);

      if (!addr.S_un.S_addr)
      { // IP to connect to is empty, request reverse
        oscar_listener* listener = CreateOscarListener(oc.ft, oft_newConnectionReceived);

        if (listener)
        { // we got listening port, fine send request
          oc.ft->listener = listener;
          oft_sendFileRedirect(oc.dwUin, oc.szUid, oc.ft, oc.dwLocalInternalIP, listener->wPort, FALSE);
// FIX ME: FT UI should support this
//          ICQBroadcastAck(oc.ft->hContact, ACKTYPE_FILE, ACKRESULT_LISTENING, oc.ft, 0);
        }
        // FIX ME: try proxy
        return 0;
      }
      nloc.szHost = inet_ntoa(addr);
      nloc.wPort = (WORD)oc.ft->dwRemotePort;
      NetLog_Direct("%sConnecting to %s:%u", oc.type==OCT_REVERSE?"Reverse ":"", nloc.szHost, nloc.wPort);

      oc.hConnection = NetLib_OpenConnection(ghDirectNetlibUser, &nloc);
      if (!oc.hConnection)
      { // connection failed, try reverse
        oscar_listener* listener = CreateOscarListener(oc.ft, oft_newConnectionReceived);

        if (listener)
        { // we got listening port, fine send request
          oc.ft->listener = listener;
          oft_sendFileRedirect(oc.dwUin, oc.szUid, oc.ft, oc.dwLocalInternalIP, listener->wPort, FALSE);
// FIX ME: FT UI should support this
//          ICQBroadcastAck(oc.ft->hContact, ACKTYPE_FILE, ACKRESULT_LISTENING, oc.ft, 0);
        }
        // FIX ME: try proxy
        return 0;
      }
      oc.ft->connection = &oc;
      // acknowledge OFT - connection is ready
      oft_sendFileAccept(oc.dwUin, oc.szUid, oc.ft);
    }
  }
  if (!oc.hConnection)
  { // one more sanity chech
    NetLog_Direct("Error: No OFT connection.");
    return 0;
  }
  // Connected, notify FT UI
  ICQBroadcastAck(oc.ft->hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, oc.ft, 0);
  // Init connection
  if (oc.ft->sending)
    oft_sendPeerInit(&oc); // only if sending file...

  hPacketRecver = (HANDLE)CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)oc.hConnection, 8192);
  packetRecv.cbSize = sizeof(packetRecv);

  // Packet receiving loop

  while (oc.hConnection)
  {
    int recvResult;

    packetRecv.dwTimeout = oc.wantIdleTime ? 0 : 60000;

    recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM)hPacketRecver, (LPARAM)&packetRecv);
    if (!recvResult)
    {
      NetLog_Direct("Clean closure of oscar socket (%p)", oc.hConnection);
      break;
    }

    if (recvResult == SOCKET_ERROR)
    {
      if (GetLastError() == ERROR_TIMEOUT)
      { // TODO: this will not work on some systems
        if (oc.wantIdleTime)
        {
          // here we should want to send file data packets
        }
        else
        {
          NetLog_Direct("Connection timeouted, closing.");
          break;
        }
      }
      else
      {
        NetLog_Direct("Abortive closure of oscar socket (%p) (%d)", oc.hConnection, GetLastError());
        break;
      }
    }

    if (oc.type == OCT_CLOSING)
      packetRecv.bytesUsed = packetRecv.bytesAvailable;
    else
      packetRecv.bytesUsed = oft_handlePackets(&oc, packetRecv.buffer, packetRecv.bytesAvailable);
  }

  // End of packet receiving loop

  NetLib_SafeCloseHandle(&hPacketRecver, FALSE);

  CloseOscarConnection(&oc);

  // FIX ME: Clean up, error handling
  if (IsValidOscarTransfer(oc.ft))
  {
    oc.ft->connection = NULL; // release link
  }

  return 0;
}



static void sendOscarPacket(oscar_connection *oc, icq_packet *packet)
{
  if (oc->hConnection)
  {
    int nResult;

    nResult = Netlib_Send(oc->hConnection, (const char*)packet->pData, packet->wLen, 0);

    if (nResult == SOCKET_ERROR)
    {
      NetLog_Direct("Oscar %p socket error: %d, closing", oc->hConnection, GetLastError());
      CloseOscarConnection(oc);
    }
  }

  SAFE_FREE(&packet->pData);
}



static int oft_handlePackets(oscar_connection *oc, unsigned char *buf, int len)
{
  BYTE *pBuf;
  DWORD dwHead;
  WORD datalen;
  WORD datatype;
  int bytesUsed = 0;

  while (len > 0)
  {
    if (oc->status == OCS_DATA)
    {
      return oft_handleFileData(oc, buf, len);
    }
    if (len < 6)
      break;

    pBuf = buf;
    unpackDWord(&pBuf, &dwHead);
    if (dwHead != 0x4F465432)
    { // bad packet
      CloseOscarConnection(oc);
      break;
    }

    unpackWord(&pBuf, &datalen);

    if (len < datalen) // wait for whole packet
      break;

    unpackWord(&pBuf, &datatype);
#ifdef _DEBUG
    NetLog_Direct("OFT2: Type %u, Length %u bytes", datatype, datalen);
#endif
    handleOFT2FramePacket(oc, datatype, pBuf, (WORD)(datalen - 8));

    /* Increase pointers so we can check for more data */
    buf += datalen;
    len -= datalen;
    bytesUsed += datalen;
  }

  return bytesUsed;
}



static int oft_handleFileData(oscar_connection *oc, unsigned char *buf, int len)
{
  oscar_filetransfer *ft = oc->ft;
  DWORD dwLen = len;
  int bytesUsed = 0;

  // do not accept more data than expected
  if (ft->dwThisFileSize - ft->dwFileBytesDone < dwLen)
    dwLen = ft->dwThisFileSize - ft->dwFileBytesDone;

  if (ft->fileId == -1)
  { // something went terribly bad
    CloseOscarConnection(oc);
    return 0;
  }
  _write(ft->fileId, buf, dwLen);
  // update checksum
  ft->dwRecvFileCheck = oft_calc_checksum(ft->dwFileBytesDone, buf, dwLen, ft->dwRecvFileCheck);
  bytesUsed += dwLen;
  ft->dwBytesDone += dwLen;
  ft->dwFileBytesDone += dwLen;

  if (GetTickCount() > ft->dwLastNotify + 700 || ft->dwFileBytesDone == ft->dwThisFileSize)
  { // notify FT UI of our progress, at most every 700ms - do not be faster than Miranda
    PROTOFILETRANSFERSTATUS pfts;

    oft_buildProtoFileTransferStatus(ft, &pfts);
    ICQBroadcastAck(ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&pfts);
    oc->ft->dwLastNotify = GetTickCount();
  }
  if (ft->dwFileBytesDone == ft->dwThisFileSize)
  {
    /* EOF */
    _close(ft->fileId);
    ft->fileId = -1;

    if (ft->dwRecvFileCheck != ft->dwThisFileCheck)
    {
      NetLog_Direct("Error: File checksums does not match!");
      // FIX ME: here we should inform user and end transfer
    }

    if ((DWORD)(ft->iCurrentFile + 1) == ft->wFilesCount)
    {
      ft->bHeaderFlags = 0x01; // the whole process is over
      // ack received file
      sendOFT2FramePacket(oc, OFT_TYPE_DONE);
      oc->type = OCT_CLOSING;
      CloseOscarConnection(oc); // close connection
      NetLog_Direct("File Transfer completed successfully.");
      ICQBroadcastAck(ft->hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, ft, 0);
      // Release structure
      SafeReleaseOscarTransfer(ft);
    }
    else
    { // ack received file
      sendOFT2FramePacket(oc, OFT_TYPE_DONE);
      oc->status = OCS_NEGOTIATION;
    }

  }
  return bytesUsed;
}



static void handleOFT2FramePacket(oscar_connection *oc, WORD datatype, BYTE *pBuffer, WORD wLen)
{
  DWORD dwID1;
  DWORD dwID2;

  if (wLen < 248)
  {
    NetLog_Direct("Error: Malformed OFT2 Frame, ignoring.");
    return;
  }

  unpackLEDWord(&pBuffer, &dwID1);
  wLen -= 4;
  unpackLEDWord(&pBuffer, &dwID2);
  wLen -= 4;

  if (datatype == OFT_TYPE_REQUEST && !oc->ft->initalized)
  { // first request does not contain MsgIDs we need to send them in ready packet
    dwID1 = oc->ft->pMessage.dwMsgID1;
    dwID2 = oc->ft->pMessage.dwMsgID2;
  }

  if (oc->ft->pMessage.dwMsgID1 != dwID1 || oc->ft->pMessage.dwMsgID2 != dwID2)
  { // this is not the right packet - bad Message IDs
    NetLog_Direct("Error: Invalid Packet Cookie, closing.");
    CloseOscarConnection(oc);
    return;
  }

  switch (datatype)
  {
  case OFT_TYPE_REQUEST:
    { // Sender ready
      oscar_filetransfer *ft = oc->ft;
      char *szFullPath;

      // Read Frame data
      if (!ft->initalized)
      {
        unpackWord(&pBuffer, &ft->wEncrypt);
        unpackWord(&pBuffer, &ft->wCompress);
        unpackWord(&pBuffer, &ft->wFilesCount);
      }
      else
        pBuffer += 6;
      unpackWord(&pBuffer, &ft->wFilesLeft);
      ft->iCurrentFile = ft->wFilesCount - oc->ft->wFilesLeft;
      if (!ft->initalized)
        unpackWord(&pBuffer, &ft->wPartsCount);
      else
        pBuffer += 2;
      unpackWord(&pBuffer, &ft->wPartsLeft);
      if (!ft->initalized)
        unpackDWord(&pBuffer, &ft->dwTotalSize);
      else
        pBuffer += 4;
      unpackDWord(&pBuffer, &ft->dwThisFileSize);
      unpackDWord(&pBuffer, &ft->dwThisFileDate);
      unpackDWord(&pBuffer, &ft->dwThisFileCheck);
      unpackDWord(&pBuffer, &ft->dwRecvForkCheck);
      unpackDWord(&pBuffer, &ft->dwThisForkSize);
      unpackDWord(&pBuffer, &ft->dwThisFileCreation);
      unpackDWord(&pBuffer, &ft->dwThisForkCheck);
      unpackDWord(&pBuffer, &ft->dwBytesDone);
      unpackDWord(&pBuffer, &ft->dwRecvFileCheck);
      if (!ft->initalized)
        unpackString(&pBuffer, ft->rawIDString, 32);
      else
        pBuffer += 32;
      unpackByte(&pBuffer, &ft->bHeaderFlags);
      unpackByte(&pBuffer, &ft->bNameOff);
      unpackByte(&pBuffer, &ft->bSizeOff);
      if (!ft->initalized)
      {
        unpackString(&pBuffer, ft->rawDummy, 69);
        unpackString(&pBuffer, ft->rawMacInfo, 16);
      }
      else
        pBuffer += 85;
      unpackWord(&pBuffer, &ft->wEncoding);
      unpackWord(&pBuffer, &ft->wSubEncoding);
      ft->cbRawFileName = wLen - 184;
      SAFE_FREE(&ft->rawFileName); // release previous buffers
      SAFE_FREE(&ft->szThisFile);
      ft->rawFileName = (char*)SAFE_MALLOC(ft->cbRawFileName);
      unpackString(&pBuffer, ft->rawFileName, ft->cbRawFileName);
      // Prepare file
      if (ft->wEncoding == 2)
      { // FIX ME: this is bad
        ft->szThisFile = ApplyEncoding(ft->rawFileName, "unicode-2-0");
        // FIX ME: need to convert dir markings
      }
      else
      {
        DWORD i;

        ft->szThisFile = null_strdup(ft->rawFileName);

        for (i = 0; i < strlennull(ft->szThisFile); i++)
        { // convert dir markings to normal backslashes
          if (ft->szThisFile[i] == 0x01) ft->szThisFile[i] = '\\';
        }
      }

      ft->initalized = 1; // First Frame Processed

      NetLog_Direct("File '%s', %d Bytes", ft->szThisFile, ft->dwThisFileSize);

      { // Prepare Path Information
        char* szFile = strrchr(ft->szThisFile, '\\');

        SAFE_FREE(&ft->szThisSubdir); // release previous path
        if (szFile)
        {
          char *szNewDir;

          ft->szThisSubdir = ft->szThisFile;
          szFile[0] = '\0'; // split that strings
          ft->szThisFile = null_strdup(szFile + 1);
          // no cheating with paths
          if (strstr(ft->szThisSubdir, "..\\") || strstr(ft->szThisSubdir, "../") ||
              strstr(ft->szThisSubdir, ":\\") || strstr(ft->szThisSubdir, ":/") ||
              ft->szThisSubdir[0] == '\\' || ft->szThisSubdir[0] == '/')
          {
            NetLog_Direct("Invalid path information");
            break;
          }
          szNewDir = (char*)_alloca(strlennull(ft->szSavePath)+strlennull(ft->szThisSubdir)+2);
          strcpy(szNewDir, ft->szSavePath);
          NormalizeBackslash(szNewDir);
          strcat(szNewDir, ft->szThisSubdir);
          _mkdir(szNewDir); // FIX ME: this will fail for multi sub-sub-sub-dirs at once
        }
        else
          ft->szThisSubdir = null_strdup("");
      }

      /* no cheating with paths */
      if (strstr(ft->szThisFile, "..\\") || strstr(ft->szThisFile, "../") ||
        strstr(ft->szThisFile, ":\\") || strstr(ft->szThisFile, ":/") ||
        ft->szThisFile[0] == '\\' || ft->szThisFile[0] == '/')
      {
        NetLog_Direct("Invalid path information");
        break;
      }
      szFullPath = (char*)SAFE_MALLOC(strlennull(ft->szSavePath)+strlennull(ft->szThisSubdir)+strlennull(ft->szThisFile)+3);
      strcpy(szFullPath, ft->szSavePath);
      NormalizeBackslash(szFullPath);
      strcat(szFullPath, ft->szThisSubdir);
      NormalizeBackslash(szFullPath);
      _chdir(szFullPath); // set current dir - not very useful
      strcat(szFullPath, ft->szThisFile);
      // we joined the full path to dest file
      SAFE_FREE(&ft->szThisFile);
      ft->szThisFile = szFullPath;

      ft->dwFileBytesDone = 0;

      {
        /* file resume */
        PROTOFILETRANSFERSTATUS pfts = {0};

        oft_buildProtoFileTransferStatus(ft, &pfts);
        if (ICQBroadcastAck(ft->hContact, ACKTYPE_FILE, ACKRESULT_FILERESUME, ft, (LPARAM)&pfts))
          break; /* UI supports resume: it will call PS_FILERESUME */

        // TODO: Unicode support
        ft->fileId = _open(ft->szThisFile, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
        if (ft->fileId == -1)
        { // FIX ME: this needs some UI interaction
          icq_LogMessage(LOG_ERROR, "Your file receive has been aborted because Miranda could not open the destination file in order to write to it. You may be trying to save to a read-only folder.");
          CloseOscarConnection(oc);
          return;
        }
      }
      // Send "we are ready"
      oc->status = OCS_DATA;

      sendOFT2FramePacket(oc, OFT_TYPE_READY);
      ICQBroadcastAck(ft->hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, ft, 0);
      if (!ft->dwThisFileSize)
      { // if the file is empty we will not receive any data
        BYTE buf;

        oft_handleFileData(oc, &buf, 0);
      }
      return;
    }
  }
}


//
// Direct packets
/////////////////////////////

static void oft_sendPeerInit(oscar_connection *oc)
{
  // prepare init frame

  // sendOFT2FramePacket(oc, OFT_TYPE_REQUEST);
}



static void sendOFT2FramePacket(oscar_connection *oc, WORD datatype)
{
  oscar_filetransfer *ft = oc->ft;
  icq_packet packet;

  packet.wLen = 192 + ft->cbRawFileName;
  init_generic_packet(&packet, 0);
  // Basic Oscar Frame
  packDWord(&packet, 0x4F465432); // Magic
  packWord(&packet, packet.wLen);
  packWord(&packet, datatype);
  // Cookie
  packLEDWord(&packet, ft->pMessage.dwMsgID1);
  packLEDWord(&packet, ft->pMessage.dwMsgID2);
  packWord(&packet, ft->wEncrypt);
  packWord(&packet, ft->wCompress);
  packWord(&packet, ft->wFilesCount);
  packWord(&packet, ft->wFilesLeft);
  packWord(&packet, ft->wPartsCount);
  packWord(&packet, ft->wPartsLeft);
  packDWord(&packet, ft->dwTotalSize);
  packDWord(&packet, ft->dwThisFileSize);
  packDWord(&packet, ft->dwThisFileDate);
  packDWord(&packet, ft->dwThisFileCheck);
  packDWord(&packet, ft->dwRecvForkCheck);
  packDWord(&packet, ft->dwThisForkSize);
  packDWord(&packet, ft->dwThisFileCreation);
  packDWord(&packet, ft->dwThisForkCheck);
  packDWord(&packet, ft->dwBytesDone);
  packDWord(&packet, ft->dwRecvFileCheck);
  packBuffer(&packet, ft->rawIDString, 32);
  packByte(&packet, ft->bHeaderFlags);
  packByte(&packet, ft->bNameOff);
  packByte(&packet, ft->bSizeOff);
  packBuffer(&packet, ft->rawDummy, 69);
  packBuffer(&packet, ft->rawMacInfo, 16);
  packWord(&packet, ft->wEncoding);
  packWord(&packet, ft->wSubEncoding);
  packBuffer(&packet, ft->rawFileName, ft->cbRawFileName);

  sendOscarPacket(oc, &packet);
}