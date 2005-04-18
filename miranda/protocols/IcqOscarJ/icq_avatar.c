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
//  Manages Avatar connection, provides internal service for handling avatars
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"

BOOL AvatarsReady = FALSE; // states if avatar connection established and ready for requests


typedef struct avatarthreadstartinfo_t
{
  HANDLE hConnection;	// handle to the connection
  char* pCookie;
  WORD wCookieLen;
  HANDLE hAvatarPacketRecver;
  int stopThread; // horrible, but simple - signal for thread to stop
  WORD wLocalSequence;
  CRITICAL_SECTION localSeqMutex;
  int pendingLogin;
  int paused;
} avatarthreadstartinfo;

typedef struct avatarcookie_t
{
  DWORD dwUin;
  HANDLE hContact;
  unsigned int hashlen;
  char *hash;
  unsigned int cbData;
  char *szFile;
} avatarcookie;

typedef struct avatarrequest_t
{
  int type;
  DWORD dwUin;
  HANDLE hContact;
  char *hash;
  unsigned int hashlen;
  char *szFile;
  char *pData;
  unsigned int cbData;
  void * pNext; // invalid, but reused - spare realloc
} avatarrequest;

avatarthreadstartinfo* currentAvatarThread; 
int pendingAvatarsStart = 1;
//static int pendingLogin = 0;
avatarrequest* pendingRequests = NULL;

extern CRITICAL_SECTION cookieMutex;
extern HANDLE ghServerNetlibUser;
extern char gpszICQProtoName[MAX_PATH];

int sendAvatarPacket(icq_packet* pPacket, avatarthreadstartinfo* atsi /*= currentAvatarThread*/);

static DWORD __stdcall icq_avatarThread(avatarthreadstartinfo *atsi);
int handleAvatarPackets(unsigned char* buf, int buflen, avatarthreadstartinfo* atsi);

void handleAvatarLogin(unsigned char *buf, WORD datalen, avatarthreadstartinfo *atsi);
void handleAvatarData(unsigned char *pBuffer, WORD wBufferLength, avatarthreadstartinfo *atsi);

void handleAvatarServiceFam(unsigned char* pBuffer, WORD wBufferLength, snac_header* pSnacHeader, avatarthreadstartinfo *atsi);
void handleAvatarFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader, avatarthreadstartinfo *atsi);


void GetFullAvatarFileName(int dwUin, int dwFormat, char* pszDest, int cbLen)
{
  GetAvatarFileName(dwUin, pszDest, cbLen);
  AddAvatarExt(dwFormat, pszDest);
}

void GetAvatarFileName(int dwUin, char* pszDest, int cbLen)
{
  int tPathLen;

  CallService(MS_DB_GETPROFILEPATH, cbLen, (LPARAM)pszDest);

  tPathLen = strlen(pszDest);
  tPathLen += _snprintf(pszDest + tPathLen, MAX_PATH-tPathLen, "\\%s\\", gpszICQProtoName);
  CreateDirectory(pszDest, NULL);

  if (dwUin != 0) 
  {
    ltoa(dwUin, pszDest + tPathLen, 10);
  }
  else 
  {
    char szBuf[MAX_PATH];

    if (CallService(MS_DB_GETPROFILENAME, 250 - tPathLen, (LPARAM)szBuf))
      strcpy(pszDest + tPathLen, "avatar" );
    else 
    {
      char* szLastDot = strstr(szBuf, ".");
      if (szLastDot) while (strstr(szLastDot+1, ".")) szLastDot = strstr(szLastDot+1, ".");
      if (szLastDot) szLastDot[0] = '\0';
      strcpy(pszDest + tPathLen, szBuf);
      strcat(pszDest + tPathLen, "_avt");
    }
  }
}

void AddAvatarExt(int dwFormat, char* pszDest)
{
  if (dwFormat == PA_FORMAT_JPEG)
    strcat(pszDest, ".jpg");
  else if (dwFormat == PA_FORMAT_GIF)
    strcat(pszDest, ".gif");
  else if (dwFormat == PA_FORMAT_PNG)
    strcat(pszDest, ".png");
  else if (dwFormat == PA_FORMAT_BMP)
    strcat(pszDest, ".bmp");
  else if (dwFormat == PA_FORMAT_XML)
    strcat(pszDest, ".xml");
  else
    strcat(pszDest, ".dat");
}

int DetectAvatarFormatBuffer(char* pBuffer)
{
  if (!strncmp(pBuffer, "%PNG", 4))
  {
    return PA_FORMAT_PNG;
  }
  if (!strncmp(pBuffer, "GIF8", 4))
  {
    return PA_FORMAT_GIF;
  }
  if (!strnicmp(pBuffer, "<?xml", 5))
  {
    return PA_FORMAT_XML;
  }
  if (((DWORD*)pBuffer)[0] == 0xE0FFD8FFul)
  {
    return PA_FORMAT_JPEG;
  }
  if (!strncmp(pBuffer, "BM", 2))
  {
    return PA_FORMAT_BMP;
  }
  return PA_FORMAT_UNKNOWN;
}


int DetectAvatarFormat(char* szFile)
{
  char pBuf[32];

  int src = _open(szFile, _O_BINARY | _O_RDONLY);
  _read(src, pBuf, 32);
  _close(src);

  return DetectAvatarFormatBuffer(pBuf);
}


void StartAvatarThread(HANDLE hConn, char* cookie, WORD cookieLen) // called from event
{
  avatarthreadstartinfo* atsi = {0};
  pthread_t tid;

  if (!hConn)
  {
    pendingAvatarsStart = 0;
    Netlib_Logf(ghServerNetlibUser, "Avatar: Connect failed");

    SAFE_FREE(&cookie);

    return;
  }
  if (currentAvatarThread && currentAvatarThread->pendingLogin) // this is not safe...
  {
    Netlib_Logf(ghServerNetlibUser, "Avatar, Multiple start thread attempt, ignored.");
    Netlib_CloseHandle(hConn);
    SAFE_FREE(&cookie);
    return;
  }
  
  AvatarsReady = FALSE; // the old connection should not be used anymore

  atsi = (avatarthreadstartinfo*)malloc(sizeof(avatarthreadstartinfo));
  atsi->pendingLogin = 1;
  atsi->stopThread = 0; // we want thread to run
  atsi->paused = 0;
  // Randomize sequence
  atsi->wLocalSequence = (WORD)RandRange(0, 0x7fff);
  atsi->hConnection = hConn;
  atsi->pCookie = cookie;
  atsi->wCookieLen = cookieLen;
  currentAvatarThread = atsi; // we store only current thread
  tid.hThread = (HANDLE)forkthreadex(NULL, 0, icq_avatarThread, atsi, 0, &tid.dwThreadId);
  CloseHandle(tid.hThread);
  
  return;
}


void StopAvatarThread()
{
  AvatarsReady = FALSE; // the connection are about to close
  
  if (currentAvatarThread)
  {
    currentAvatarThread->stopThread = 1; // make the thread stop
    currentAvatarThread = NULL; // the thread will finish in background
  }
  return;
}


// request avatar data from server
int GetAvatarData(HANDLE hContact, DWORD dwUin, char* hash, unsigned int hashlen, char* file)
{
  avatarthreadstartinfo* atsi;

  atsi = currentAvatarThread; // we take avatar thread - horrible, but realiable
  if (AvatarsReady && !atsi->paused) // check if we are ready
  {
    icq_packet packet;
    char szUin[33];
    BYTE nUinLen;
    DWORD dwCookie;
    avatarcookie* ack;

    _itoa(dwUin, szUin, 10);
    nUinLen = strlen(szUin);
    
    ack = (avatarcookie*)malloc(sizeof(avatarcookie));
    if (!ack) return 0; // out of memory, go away
    ack->dwUin = dwUin;
    ack->hContact = hContact;
    ack->hash = (char*)malloc(hashlen);
    memcpy(ack->hash, hash, hashlen); // copy the data
    ack->hashlen = hashlen;
    ack->szFile = _strdup(file); // we duplicate the string
    dwCookie = AllocateCookie(6, dwUin, ack);

    packet.wLen = 12 + nUinLen + hashlen;
    write_flap(&packet, 2);
    packFNACHeader(&packet, ICQ_AVATAR_FAMILY, ICQ_AVATAR_GET_REQUEST, 0, dwCookie);
    packByte(&packet, nUinLen);
    packBuffer(&packet, szUin, nUinLen);
    packByte(&packet, 1); // unknown, probably type of request: 1 = get icon :)
    packBuffer(&packet, hash, (unsigned short)hashlen);

    if (sendAvatarPacket(&packet, atsi))
    {
      Netlib_Logf(ghServerNetlibUser, "Request to get %d avatar image sent.", dwUin);

      return dwCookie;
    }
    FreeCookie(dwCookie); // sending failed, free resources
    SAFE_FREE(&ack->szFile);
    SAFE_FREE(&ack->hash);
    SAFE_FREE(&ack);
  }
  // we failed to send request, or avatar thread not ready
  EnterCriticalSection(&cookieMutex); // wait for ready queue, reused cs
  { // check if any request for this user is not already in the queue
    avatarrequest* ar;
    int bYet = 0;
    ar = pendingRequests;
    while (ar)
    {
      if (ar->hContact == hContact)
      { // we found it, return error
        LeaveCriticalSection(&cookieMutex);
        Netlib_Logf(ghServerNetlibUser, "Ignoring duplicate get %d avatar request.", dwUin);

        return 0;
      }
      ar = ar->pNext;
    }
    // add request to queue, processed after successful login
    ar = (avatarrequest*)malloc(sizeof(avatarrequest));
    if (!ar)
    { // out of memory, go away
      LeaveCriticalSection(&cookieMutex);
      return 0;
    }
    ar->type = 1; // get avatar
    ar->dwUin = dwUin;
    ar->hContact = hContact;
    ar->hash = (char*)malloc(hashlen);
    memcpy(ar->hash, hash, hashlen); // copy the data
    ar->hashlen = hashlen;
    ar->szFile = _strdup(file); // duplicate the string
    ar->pNext = pendingRequests;
    pendingRequests = ar;
  }
  LeaveCriticalSection(&cookieMutex);

  Netlib_Logf(ghServerNetlibUser, "Request to get %d avatar image added to queue.", dwUin);

  if (!AvatarsReady && !pendingAvatarsStart)
  {
    icq_requestnewfamily(0x10, StartAvatarThread);
    pendingAvatarsStart = 1;
  }

  return -1; // we added to queue
}

// upload avatar data to server
int SetAvatarData(HANDLE hContact, char* data, unsigned int datalen)
{ 
  avatarthreadstartinfo* atsi;

  atsi = currentAvatarThread; // we take avatar thread - horrible, but realiable
  if (AvatarsReady && !atsi->paused) // check if we are ready
  {
    icq_packet packet;
    DWORD dwCookie;
    avatarcookie* ack;

    ack = (avatarcookie*)malloc(sizeof(avatarcookie));
    if (!ack) return 0; // out of memory, go away
    ack->hContact = hContact;
    ack->dwUin = 0;
    ack->cbData = datalen;

    dwCookie = AllocateCookie(2, 0, ack);

    packet.wLen = 14 + datalen;
    write_flap(&packet, 2);
    packFNACHeader(&packet, ICQ_AVATAR_FAMILY, ICQ_AVATAR_UPLOAD_REQUEST, 0, dwCookie);
    packWord(&packet, 1); // unknown, probably reference
    packWord(&packet, (WORD)datalen);
    packBuffer(&packet, data, (unsigned short)datalen);

    if (sendAvatarPacket(&packet, atsi))
    {
      Netlib_Logf(ghServerNetlibUser, "Upload avatar packet sent.");

      return dwCookie;
    }
    FreeCookie(dwCookie); // failed to send, free resources
    SAFE_FREE(&ack);
  }
  // we failed to send request, or avatar thread not ready
  EnterCriticalSection(&cookieMutex); // wait for ready queue, reused cs
  { // check if any request for this user is not already in the queue
    avatarrequest* ar;
    int bYet = 0;
    ar = pendingRequests;
    while (ar)
    {
      if (ar->hContact == hContact)
      { // we found it, return error
        LeaveCriticalSection(&cookieMutex);
        Netlib_Logf(ghServerNetlibUser, "Ignoring duplicate upload avatar request.");

        return 0;
      }
      ar = ar->pNext;
    }
    // add request to queue, processed after successful login
    ar = (avatarrequest*)malloc(sizeof(avatarrequest));
    if (!ar)
    { // out of memory, go away
      LeaveCriticalSection(&cookieMutex);
      return 0;
    }
    ar->type = 2; // upload avatar
    ar->hContact = hContact;
    ar->pData = (char*)malloc(datalen);
    if (!ar->pData)
    { // alloc failed
      LeaveCriticalSection(&cookieMutex);
      SAFE_FREE(&ar);
      return 0;
    }
    memcpy(ar->pData, data, datalen); // copy the data
    ar->cbData = datalen;
    ar->pNext = pendingRequests;
    pendingRequests = ar;
  }
  LeaveCriticalSection(&cookieMutex);

  Netlib_Logf(ghServerNetlibUser, "Request to upload avatar image added to queue.");

  if (!AvatarsReady && !pendingAvatarsStart)
  {
    pendingAvatarsStart = 1;
    icq_requestnewfamily(0x10, StartAvatarThread);
  }

  return -1; // we added to queue
}


static DWORD __stdcall icq_avatarThread(avatarthreadstartinfo *atsi)
{
  // This is the "infinite" loop that receives the packets from the ICQ avatar server
  {
    int recvResult;
    NETLIBPACKETRECVER packetRecv = {0};

    InitializeCriticalSection(&atsi->localSeqMutex);

    atsi->hAvatarPacketRecver = (HANDLE)CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)atsi->hConnection, 8192);
    packetRecv.cbSize = sizeof(packetRecv);
    packetRecv.dwTimeout = 60000; // timeout every minute - for stopThread to work
    while(!atsi->stopThread)
    {
      recvResult = CallService(MS_NETLIB_GETMOREPACKETS,(WPARAM)atsi->hAvatarPacketRecver, (LPARAM)&packetRecv);

      if (recvResult == 0)
      {
        Netlib_Logf(ghServerNetlibUser, "Clean closure of avatar socket");
        break;
      }

      if (recvResult == SOCKET_ERROR)
      {
        if (GetLastError() == ERROR_TIMEOUT)
        {  // timeout, check if we should be still running
          if (Miranda_Terminated())
            atsi->stopThread = 1; // we must stop here, cause due to a hack in netlib, we always get timeout, even if the connection is already dead
#ifdef _DEBUG
          else
            Netlib_Logf(ghServerNetlibUser, "Avatar Thread is Idle.");
#endif
          if (DBGetContactSettingByte(NULL, gpszICQProtoName, "KeepAlive", 0))
          { // send keep-alive packet
            icq_packet packet;

            packet.wLen = 0;
            write_flap(&packet, ICQ_PING_CHAN);
            sendAvatarPacket(&packet, atsi);
          }
          continue; 
        }
        Netlib_Logf(ghServerNetlibUser, "Abortive closure of avatar socket");
        break;
      }

      // Deal with the packet
      packetRecv.bytesUsed = handleAvatarPackets(packetRecv.buffer, packetRecv.bytesAvailable, atsi);
      
      if ((AvatarsReady == TRUE) && (packetRecv.bytesAvailable == packetRecv.bytesUsed) && !atsi->paused) // no packets pending
      { // process request queue
        EnterCriticalSection(&cookieMutex);

        while (pendingRequests) // pick up an request and send it - happens immediatelly after login
        {
          avatarrequest* reqdata = pendingRequests;
          pendingRequests = reqdata->pNext;

#ifdef _DEBUG
          Netlib_Logf(ghServerNetlibUser, "Picked up the %d request from queue.", reqdata->dwUin);
#endif
          switch (reqdata->type)
          {
          case 1: // get avatar
            GetAvatarData(reqdata->hContact, reqdata->dwUin, reqdata->hash, reqdata->hashlen, reqdata->szFile);

            SAFE_FREE(&reqdata->szFile);
            SAFE_FREE(&reqdata->hash); // as soon as it will be copied
            break;
          case 2: // set avatar
            SetAvatarData(reqdata->hContact, reqdata->pData, reqdata->cbData);

            SAFE_FREE(&reqdata->pData);
            break;
          }

          SAFE_FREE(&reqdata);
        }

        LeaveCriticalSection(&cookieMutex);
      }
    }
    Netlib_CloseHandle(atsi->hAvatarPacketRecver); // Close the packet receiver 
  }
  Netlib_CloseHandle(atsi->hConnection); // Close the connection
  
  DeleteCriticalSection(&atsi->localSeqMutex);

  SAFE_FREE(&atsi->pCookie);
  if (currentAvatarThread == atsi) // if we stoped by error or unexpectedly, clear global variable
  {
    AvatarsReady = FALSE; // we are not ready
    pendingAvatarsStart = 0;
    currentAvatarThread = NULL; // this is horrible, rewrite
  }
  SAFE_FREE(&atsi);
  return 0;
}


int handleAvatarPackets(unsigned char* buf, int buflen, avatarthreadstartinfo* atsi)
{
  BYTE channel;
  WORD sequence;
  WORD datalen;
  int bytesUsed = 0;

  while (buflen > 0)
  {
    // All FLAPS begin with 0x2a
    if (*buf++ != FLAP_MARKER)
      break;

    if (buflen < 6)
      break;

    unpackByte(&buf, &channel);
    unpackWord(&buf, &sequence);
    unpackWord(&buf, &datalen);

    if (buflen < 6 + datalen)
      break;

#ifdef _DEBUG
    Netlib_Logf(ghServerNetlibUser, "Avatar FLAP: Channel %u, Seq %u, Length %u bytes", channel, sequence, datalen);
#endif

    switch (channel)
    {
    case ICQ_LOGIN_CHAN:
      handleAvatarLogin(buf, datalen, atsi);
      break;

    case ICQ_DATA_CHAN:
      handleAvatarData(buf, datalen, atsi);
      break;

    default:
      Netlib_Logf(ghServerNetlibUser, "Warning: Unhandled Avatar FLAP Channel: Channel %u, Seq %u, Length %u bytes", channel, sequence, datalen);
      break;
    }

    /* Increase pointers so we can check for more FLAPs */
    buf += datalen;
    buflen -= (datalen + 6);
    bytesUsed += (datalen + 6);
  }

  return bytesUsed;
}


int sendAvatarPacket(icq_packet* pPacket, avatarthreadstartinfo* atsi)
{
  int lResult = 0;

  // This critsec makes sure that the sequence order doesn't get screwed up
  EnterCriticalSection(&atsi->localSeqMutex);

  if (atsi->hConnection)
  {
    int nRetries;
    int nSendResult;

    // :IMPORTANT:
    // The FLAP sequence must be a WORD. When it reaches 0xFFFF it should wrap to
    // 0x0000, otherwise we'll get kicked by server.
    atsi->wLocalSequence++;

    // Pack sequence number
    pPacket->pData[2] = ((atsi->wLocalSequence & 0xff00) >> 8);
    pPacket->pData[3] = (atsi->wLocalSequence & 0x00ff);

    for (nRetries = 3; nRetries >= 0; nRetries--)
    {
      nSendResult = Netlib_Send(atsi->hConnection, (const char *)pPacket->pData, pPacket->wLen, 0);

      if (nSendResult != SOCKET_ERROR)
        break;

      Sleep(1000);
    }

    // Send error
    if (nSendResult == SOCKET_ERROR)
    { // thread stops automatically
      Netlib_Logf(ghServerNetlibUser, "Your connection with the ICQ avatar server was abortively closed");
    }
    else
    {
      lResult = 1; // packet sent successfully
    }
  }
  else
  {
    Netlib_Logf(ghServerNetlibUser, "Error: Failed to send packet (no connection)");
  }

  LeaveCriticalSection(&atsi->localSeqMutex);

  SAFE_FREE(&pPacket->pData);

  return lResult;
}


void handleAvatarLogin(unsigned char *buf, WORD datalen, avatarthreadstartinfo *atsi)
{
  icq_packet packet;

  if (*(DWORD*)buf == 0x1000000)
  {  // here check if we received SRV_HELLO
    packet.wLen = atsi->wCookieLen + 8;

    atsi->wLocalSequence = (WORD)RandRange(0, 0xffff); 

    write_flap(&packet, 1);
    packDWord(&packet, 0x00000001);
    packTLV(&packet, 0x06, (WORD)atsi->wCookieLen, atsi->pCookie);

    sendAvatarPacket(&packet, atsi);
    
#ifdef _DEBUG
    Netlib_Logf(ghServerNetlibUser, "Sent CLI_IDENT to avatar server");
#endif

    SAFE_FREE(&atsi->pCookie);
    atsi->wCookieLen = 0;
  }
  else
  {
    Netlib_Logf(ghServerNetlibUser, "Invalid Avatar Server response, Ch1.");
  }
}

void handleAvatarData(unsigned char *pBuffer, WORD wBufferLength, avatarthreadstartinfo *atsi)
{
  snac_header* pSnacHeader = NULL;

  pSnacHeader = unpackSnacHeader(&pBuffer, &wBufferLength);

  if (!pSnacHeader || !pSnacHeader->bValid)
  {
    Netlib_Logf(ghServerNetlibUser, "Error: Failed to parse SNAC header");
  }
  else
  {
#ifdef _DEBUG
    Netlib_Logf(ghServerNetlibUser, " Received SNAC(x%02X,x%02X)", pSnacHeader->wFamily, pSnacHeader->wSubtype);
#endif

    switch (pSnacHeader->wFamily)
    {

    case ICQ_SERVICE_FAMILY:
      handleAvatarServiceFam(pBuffer, wBufferLength, pSnacHeader, atsi);
      break;

    case ICQ_AVATAR_FAMILY:
      handleAvatarFam(pBuffer, wBufferLength, pSnacHeader, atsi);
      break;

    default:
      Netlib_Logf(ghServerNetlibUser, "Ignoring SNAC(x%02X,x%02X) - FAMILYx%02X not expected", pSnacHeader->wFamily, pSnacHeader->wSubtype, pSnacHeader->wFamily);
      break;
    }
  }
  // Clean up and exit
  SAFE_FREE(&pSnacHeader);
}


void handleAvatarServiceFam(unsigned char* pBuffer, WORD wBufferLength, snac_header* pSnacHeader, avatarthreadstartinfo *atsi)
{
  icq_packet packet;

  switch (pSnacHeader->wSubtype)
  {

  case ICQ_SERVER_READY:
#ifdef _DEBUG
    Netlib_Logf(ghServerNetlibUser, "Avatar server is ready and is requesting my Family versions");
    Netlib_Logf(ghServerNetlibUser, "Sending my Families");
#endif

    // Miranda mimics the behaviour of Icq5
    packet.wLen = 18;
    write_flap(&packet, 2);
    packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_FAMILIES, 0, ICQ_CLIENT_FAMILIES<<0x10);
    packDWord(&packet, 0x00010004);
    packDWord(&packet, 0x00100001);
    sendAvatarPacket(&packet, atsi);
    break;

  case ICQ_SERVER_FAMILIES2:
    /* This is a reply to CLI_FAMILIES and it tells the client which families and their versions that this server understands.
     * We send a rate request packet */
#ifdef _DEBUG
    Netlib_Logf(ghServerNetlibUser, "Server told me his Family versions");
    Netlib_Logf(ghServerNetlibUser, "Requesting Rate Information");
#endif
    packet.wLen = 10;
    write_flap(&packet, 2);
    packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_REQ_RATE_INFO, 0, ICQ_CLIENT_REQ_RATE_INFO<<0x10);
    sendAvatarPacket(&packet, atsi);
    break;

  case ICQ_SERVER_RATE_INFO:
#ifdef _DEBUG
    Netlib_Logf(ghServerNetlibUser, "Server sent Rate Info");
    Netlib_Logf(ghServerNetlibUser, "Sending Rate Info Ack");
#endif
    /* Don't really care about this now, just send the ack */
    packet.wLen = 20;
    write_flap(&packet, 2);
    packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_RATE_ACK, 0, ICQ_CLIENT_RATE_ACK<<0x10);
    packDWord(&packet, 0x00010002);
    packDWord(&packet, 0x00030004);
    packWord(&packet, 0x0005);
    sendAvatarPacket(&packet, atsi);

    // send cli_ready
    packet.wLen = 26;
    write_flap(&packet, 2);
    packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_READY, 0, ICQ_CLIENT_READY<<0x10);
    packDWord(&packet, 0x00010004);
    packDWord(&packet, 0x001008E4);
    packDWord(&packet, 0x00100001);
    packDWord(&packet, 0x001008E4);
    sendAvatarPacket(&packet, atsi);
    
    AvatarsReady = TRUE; // we are ready to process requests
    pendingAvatarsStart = 0;
    atsi->pendingLogin = 0;

    Netlib_Logf(ghServerNetlibUser, " *** Yeehah, avatar login sequence complete");
    break;

  case ICQ_SERVER_PAUSE:
    Netlib_Logf(ghServerNetlibUser, "Avatar server is going down in a few seconds... (Flags: %u, Ref: %u", pSnacHeader->wFlags, pSnacHeader->dwRef);
    // This is the list of groups that we want to have on the next server
    packet.wLen = 14;
    write_flap(&packet, ICQ_DATA_CHAN);
    packFNACHeader(&packet, ICQ_SERVICE_FAMILY, ICQ_CLIENT_PAUSE_ACK, 0, ICQ_CLIENT_PAUSE_ACK<<0x10);
    packWord(&packet,ICQ_SERVICE_FAMILY);
    packWord(&packet,0x10);
    sendAvatarPacket(&packet, atsi);
#ifdef _DEBUG
    Netlib_Logf(ghServerNetlibUser, "Sent server pause ack");
#endif
    break;

  default:
    Netlib_Logf(ghServerNetlibUser, "Warning: Ignoring SNAC(x01,%2x) - Unknown SNAC (Flags: %u, Ref: %u)", pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;
  }
}


void handleAvatarFam(unsigned char *pBuffer, WORD wBufferLength, snac_header* pSnacHeader, avatarthreadstartinfo *atsi)
{

  switch (pSnacHeader->wSubtype)
  {
    case ICQ_AVATAR_GET_REPLY:  // received avatar data, store to file
    { // handle new avatar, notify
      avatarcookie* ac;
      if (FindCookie(pSnacHeader->dwRef, NULL, &ac))
      {
        BYTE len;
        WORD datalen;
        int out;
        char* szMyFile = (char*)malloc(strlen(ac->szFile)+10);
        PROTO_AVATAR_INFORMATION ai;

        strcpy(szMyFile, ac->szFile);

        ai.cbSize = sizeof ai;
        ai.format = PA_FORMAT_JPEG; // this is for error only
        ai.hContact = ac->hContact;
        strcpy(ai.filename, ac->szFile);
        AddAvatarExt(PA_FORMAT_JPEG, ai.filename); 

        FreeCookie(pSnacHeader->dwRef);
        unpackByte(&pBuffer, &len);
        if (wBufferLength < ((ac->hashlen)<<1)+4+len)
        {
          Netlib_Logf(ghServerNetlibUser, "Received invalid avatar reply.");

          ProtoBroadcastAck(gpszICQProtoName, ac->hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, (LPARAM)NULL);

          SAFE_FREE(&ac->szFile);
          SAFE_FREE(&ac->hash);
          SAFE_FREE(&ac);

          break;
        }

        pBuffer += len;
        pBuffer += (ac->hashlen<<1) + 1;
        unpackWord(&pBuffer, &datalen);

        if (datalen > 4)
        { // store to file...
          int dwPaFormat;

          Netlib_Logf(ghServerNetlibUser, "Received user avatar, storing (%d bytes).", datalen);

          dwPaFormat = DetectAvatarFormatBuffer(pBuffer);
          DBWriteContactSettingByte(ac->hContact, gpszICQProtoName, "AvatarType", (BYTE)dwPaFormat);
          ai.format = dwPaFormat; // set the format
          AddAvatarExt(dwPaFormat, szMyFile);
          strcpy(ai.filename, szMyFile);

          out = _open(szMyFile, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
			    if (out) 
          {
            DBVARIANT dbv;

            _write(out, pBuffer, datalen);
            _close(out);
            
            if (dwPaFormat != PA_FORMAT_XML)
              LinkContactPhotoToFile(ac->hContact, szMyFile); // this should not be here, but no other simple solution available

            if (!DBGetContactSetting(ac->hContact, gpszICQProtoName, "AvatarHash", &dbv))
            {
              if (DBWriteContactSettingBlob(ac->hContact, gpszICQProtoName, "AvatarSaved", dbv.pbVal, dbv.cpbVal))
                Netlib_Logf(ghServerNetlibUser, "Failed to set file hash.");

              DBFreeVariant(&dbv);
            }
            else
            {
              Netlib_Logf(ghServerNetlibUser, "Warning: DB error (no hash in DB).");
              // the hash was lost, try to fix that
              if (DBWriteContactSettingBlob(ac->hContact, gpszICQProtoName, "AvatarSaved", ac->hash, ac->hashlen) ||
                DBWriteContactSettingBlob(ac->hContact, gpszICQProtoName, "AvatarHash", ac->hash, ac->hashlen))
              {
                Netlib_Logf(ghServerNetlibUser, "Failed to save avatar hash to DB");
              }
            }

            ProtoBroadcastAck(gpszICQProtoName, ac->hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, (HANDLE)&ai, (LPARAM)NULL);
          }
        }
        else
        { // the avatar is empty, delete the hash
          Netlib_Logf(ghServerNetlibUser, "Received empty avatar, hash deleted.", datalen);

          DBDeleteContactSetting(ac->hContact, gpszICQProtoName, "AvatarSaved");
          DBDeleteContactSetting(ac->hContact, gpszICQProtoName, "AvatarHash");
          ProtoBroadcastAck(gpszICQProtoName, ac->hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, (HANDLE)&ai, (LPARAM)NULL);
        }
        SAFE_FREE(&ac->szFile);
        SAFE_FREE(&ac->hash);
        SAFE_FREE(&ac);
      }
      else
      {
        Netlib_Logf(ghServerNetlibUser, "Warning: Received unexpected Avatar Reply SNAC(x10,x07).");
      }

      break;
    }
    case ICQ_AVATAR_UPLOAD_ACK:
    {
      // upload completed, notify
      BYTE res;
      unpackByte(&pBuffer, &res);
      if (!res && (wBufferLength == 0x15))
      {
        avatarcookie* ac;
        if (FindCookie(pSnacHeader->dwRef, NULL, &ac))
        {
          // here we store the local hash
          SAFE_FREE(&ac);
        }
        else
        {
          Netlib_Logf(ghServerNetlibUser, "Warning: Received unexpected Upload Avatar Reply SNAC(x10,x03).");
        }
      }
      else if (res)
      {
        Netlib_Logf(ghServerNetlibUser, "Error uploading avatar to server, #%d", res);
        icq_LogMessage(LOG_WARNING, "Error uploading avatar to server, server refused to accept the image.");
      }
      else
        Netlib_Logf(ghServerNetlibUser, "Received invalid upload avatar ack.");

      break;
    }
  default:
    Netlib_Logf(ghServerNetlibUser, "Warning: Ignoring SNAC(x02,%2x) - Unknown SNAC (Flags: %u, Ref: %u)", pSnacHeader->wSubtype, pSnacHeader->wFlags, pSnacHeader->dwRef);
    break;

  }
}
