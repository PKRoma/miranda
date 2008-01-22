// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001-2002 Jon Keating, Richard Hughes
// Copyright © 2002-2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004-2008 Joe Kucera
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



extern WORD wListenPort;

void handleFileAck(PBYTE buf, WORD wLen, DWORD dwUin, DWORD dwCookie, WORD wStatus, char* pszText)
{
  char* pszFileName = NULL;
  DWORD dwFileSize;
  HANDLE hCookieContact;
  WORD wPort;
  WORD wFilenameLength;
  filetransfer* ft;

  
  // Find the filetransfer that belongs to this response
  if (!FindCookie(dwCookie, &hCookieContact, &ft))
  {
    NetLog_Direct("Error: Received unexpected file transfer request response");
    return;
  }

  FreeCookie(dwCookie);
  
  if (hCookieContact != HContactFromUIN(dwUin, NULL))
  {
    NetLog_Direct("Error: UINs do not match in file transfer request response");
    return;
  }

  // If status != 0, a request has been denied
  if (wStatus != 0)
  {
    NetLog_Direct("File transfer denied by %u.", dwUin);
    ICQBroadcastAck(ft->hContact, ACKTYPE_FILE, ACKRESULT_DENIED, (HANDLE)ft, 0);

    FreeCookie(dwCookie);

    return;
  }

  if (wLen < 6) 
  { // sanity check
    NetLog_Direct("Ignoring malformed file transfer request response");
    return;
  }

  // Port to connect to
  unpackWord(&buf, &wPort);
  ft->dwRemotePort = wPort;
  wLen -= 2;
  
  // Unknown
  buf += 2;
  wLen -= 2;
  
  // Filename
  unpackLEWord(&buf, &wFilenameLength);
  if (wFilenameLength > 0)
  {
    if (wFilenameLength > wLen - 2)
      wFilenameLength = wLen - 2;
    pszFileName = _alloca(wFilenameLength+1);
    unpackString(&buf, pszFileName, wFilenameLength);
    pszFileName[wFilenameLength] = '\0';
  }
  wLen = wLen - 2 - wFilenameLength;
  
  if (wLen >= 4)
  { // Total filesize
    unpackLEDWord(&buf, &dwFileSize);
    wLen -= 4;
  }
  else
    dwFileSize = 0;
  
  NetLog_Direct("File transfer ack from %u, port %u, name %s, size %u", dwUin, ft->dwRemotePort, pszFileName, dwFileSize);

  ICQBroadcastAck(ft->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTING, (HANDLE)ft, 0);

  OpenDirectConnection(ft->hContact, DIRECTCONN_FILE, ft);
}



filetransfer *CreateFileTransfer(HANDLE hContact, DWORD dwUin, int nVersion)
{
  filetransfer *ft = CreateIcqFileTransfer();

  ft->dwUin = dwUin;
  ft->hContact = hContact;
  ft->nVersion = nVersion;
  ft->pMessage.bMessageType = MTYPE_FILEREQ;
  InitMessageCookie(&ft->pMessage);

  return ft;
}

// pszDescription points to a string with the reason
// buf points to the first data after the string
void handleFileRequest(PBYTE buf, WORD wLen, DWORD dwUin, DWORD dwCookie, DWORD dwID1, DWORD dwID2, char* pszDescription, int nVersion, BOOL bDC)
{
  char* pszFileName = NULL;
  DWORD dwFileSize;
  WORD wFilenameLength;
  BOOL bEmptyDesc = FALSE;


  if (strlennull(pszDescription) == 0)
  {
    pszDescription = ICQTranslate("No description given");
    bEmptyDesc = TRUE;
  }
  
  // Empty port+pad
  buf += 4;
  wLen -= 4;

  
  // Filename
  unpackLEWord(&buf, &wFilenameLength);
  if (wFilenameLength > 0)
  {
    pszFileName = _alloca(wFilenameLength + 1);
    unpackString(&buf, pszFileName, wFilenameLength);
    pszFileName[wFilenameLength] = '\0';
  }
  else
  {
    NetLog_Direct("Ignoring malformed file send request");
    return;
  }

  wLen = wLen - 2 - wFilenameLength;

  // Total filesize
  unpackLEDWord(&buf, &dwFileSize);
  wLen -= 4;

  {
    CCSDATA ccs;
    PROTORECVEVENT pre;
    char* szBlob;
    filetransfer* ft;
    int bAdded;
    HANDLE hContact = HContactFromUIN(dwUin, &bAdded);
    
    // Initialize a filetransfer struct
    ft = CreateFileTransfer(hContact, dwUin, nVersion);
    ft->dwCookie = dwCookie;
    ft->szFilename = null_strdup(pszFileName);
    ft->szDescription = null_strdup(pszDescription);
    ft->fileId = -1;
    ft->dwTotalSize = dwFileSize;
    ft->pMessage.dwMsgID1 = dwID1;
    ft->pMessage.dwMsgID2 = dwID2;
    ft->bDC = bDC;
    ft->bEmptyDesc = bEmptyDesc;
    
    
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



void handleDirectCancel(directconnect *dc, PBYTE buf, WORD wLen, WORD wCommand, DWORD dwCookie, WORD wMessageType, WORD wStatus, WORD wFlags, char* pszText)
{
  NetLog_Direct("handleDirectCancel: Unhandled cancel");
}



// *******************************************************************************



void icq_CancelFileTransfer(HANDLE hContact, filetransfer* ft)
{
  DWORD dwCookie;

  if (FindCookieByData(ft, &dwCookie, NULL))
    FreeCookie(dwCookie);      /* this bit stops a send that's waiting for acceptance */

  if (IsValidFileTransfer(ft))
  { // Transfer still out there, end it
    NetLib_CloseConnection(&ft->hConnection, FALSE);

    ICQBroadcastAck(ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0);

    if (!FindFileTransferDC(ft))
    { // Release orphan structure only
      SafeReleaseFileTransfer(&ft);
    }
  }
}
