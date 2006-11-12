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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/icq_direct.h,v $
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#ifndef __ICQ_DIRECT_H
#define __ICQ_DIRECT_H

typedef struct {
  message_cookie_data pMessage;
  BYTE ft_magic;
  int status;
  int sending;
  int iCurrentFile;
  int currentIsDir;
  DWORD dwCookie;
  DWORD dwUin;
  DWORD dwRemotePort;
  HANDLE hContact;
  char *szFilename;
  char *szDescription;
  char *szSavePath;
  char *szThisFile;
  char *szThisSubdir;
  char **files;
  DWORD dwThisFileSize;
  DWORD dwThisFileDate;
  DWORD dwTotalSize;
  DWORD dwFileCount;
  DWORD dwTransferSpeed;
  DWORD dwBytesDone, dwFileBytesDone;
  int fileId;
  HANDLE hConnection;
  DWORD dwLastNotify;
  int   nVersion;   // Was this sent with a v7 or a v8 packet?
  BOOL bDC;         // Was this received over a DC or through server?
  BOOL bEmptyDesc;  // Was the description empty ?
} filetransfer;

#define DIRECTCONN_STANDARD   0
#define DIRECTCONN_FILE       1
#define DIRECTCONN_CHAT       2
#define DIRECTCONN_REVERSE    10
#define DIRECTCONN_CLOSING    15

typedef struct {
  HANDLE hContact;
  HANDLE hConnection;
  DWORD dwConnectionCookie;
  int type;
  WORD wVersion;
  int incoming;
  int wantIdleTime;
  int packetPending;
  DWORD dwRemotePort;
  DWORD dwRemoteUin;
  DWORD dwRemoteExternalIP;
  DWORD dwRemoteInternalIP;
  DWORD dwLocalExternalIP;
  DWORD dwLocalInternalIP;
  int initialised;
  int handshake;
  DWORD dwThreadId;
  filetransfer *ft;
  DWORD dwReqId;  // Reverse Connect request cookie
} directconnect;

void OpenDirectConnection(HANDLE hContact, int type, void *pvExtra);
int IsDirectConnectionOpen(HANDLE hContact, int type, int bPassive);
void CloseDirectConnection(directconnect *dc);
void CloseContactDirectConns(HANDLE hContact);
int SendDirectMessage(HANDLE hContact, icq_packet *pkt);
int sendDirectPacket(directconnect *dc, icq_packet *pkt);
void icq_newConnectionReceived(HANDLE hNewConnection, DWORD dwRemoteIP, void *pExtra);
void InitDirectConns(void);
void UninitDirectConns(void);
directconnect* FindFileTransferDC(filetransfer* ft);
void handleDirectMessage(directconnect *dc, PBYTE buf, WORD wLen);
void EncryptDirectPacket(directconnect *dc, icq_packet *p);
void icq_AcceptFileTransfer(HANDLE hContact, filetransfer *ft);
void icq_CancelFileTransfer(HANDLE hContact, filetransfer *ft);
void icq_sendFileResume(filetransfer *ft, int action, const char *szFilename);
void icq_InitFileSend(filetransfer *ft);
void AddExpectedFileRecv(filetransfer *ft);
filetransfer *FindExpectedFileRecv(DWORD dwUin, DWORD dwTotalSize);
void handleFileTransferPacket(directconnect *dc, PBYTE buf, WORD wLen);
void handleFileTransferIdle(directconnect *dc);


void handleDirectCancel(directconnect *dc, PBYTE buf, WORD wLen, WORD wCommand, DWORD dwCookie, WORD wMessageType, WORD wStatus, WORD wFlags, char* pszText);

// Handles all types of file transfer replies
void handleFileAck(PBYTE buf, WORD wLen, DWORD dwUin, DWORD dwCookie, WORD wStatus, char* pszText);

// Handle a received file transfer request (direct & server)
void handleFileRequest(PBYTE buf, WORD wLen, DWORD dwUin, DWORD dwCookie, DWORD dwID1, DWORD dwID2, char* pszDescription, int nVersion, BOOL bDC);


#endif /* __ICQ_DIRECT_H */
