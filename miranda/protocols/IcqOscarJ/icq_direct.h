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

#ifndef __ICQ_DIRECT_H
#define __ICQ_DIRECT_H

typedef struct {
	BYTE bMessageType;
	BYTE nAckType;
	int status;
	int sending;
	int iCurrentFile;
	int currentIsDir;
	WORD wCookie;
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
	DWORD TS1;
	DWORD TS2;
	int   nVersion; // Was this sent with a v7 or a v8 packet?
	BOOL bDC;       // Was this received over a DC or through server?
} filetransfer;

#define DIRECTCONN_STANDARD   0
#define DIRECTCONN_FILE       1
#define DIRECTCONN_CHAT       2
#define DIRECTCONN_CLOSING    10
typedef struct {
	HANDLE hConnection;
	int type;
	WORD wVersion;
	int incoming;
	int wantIdleTime;
	DWORD dwRemotePort;
	DWORD dwRemoteUin;
	DWORD dwRemoteExternalIP;
	DWORD dwRemoteInternalIP;
	DWORD dwConnCookie;
	int initialised;
	DWORD dwThreadId;
	icq_packet *packetToSend;
	filetransfer *ft;
} directconnect;

void OpenDirectConnection(HANDLE hContact, int type, void *pvExtra);
int IsDirectConnectionOpen(HANDLE hContact, int type);
int sendDirectPacket(HANDLE hConnection, icq_packet *pkt);
void icq_newConnectionReceived(HANDLE hNewConnection, DWORD dwRemoteIP);
void InitDirectConns(void);
void UninitDirectConns(void);
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
void icq_sendFileSendDirectv7(DWORD dwUin, HANDLE hContact, WORD wCookie, char *pszFiles, char *szDescription, DWORD dwTotalSize);
void icq_sendFileSendDirectv8(DWORD dwUin, HANDLE hContact, WORD wCookie, char *pszFiles, char *szDescription, DWORD dwTotalSize);
void buildDirectPacketHeader(icq_packet *packet, WORD wDataLen, WORD wCommand, WORD wCookie, BYTE bMsgType, BYTE bMsgFlags, WORD wX1);

void icq_sendFileAcceptDirect(HANDLE hContact, filetransfer *ft);

void handleDirectCancel(directconnect *dc, PBYTE buf, WORD wLen, WORD wCommand, WORD wCookie, WORD wMessageType, WORD wStatus, WORD wFlags, char* pszText);

// Handles all types of file transfer replies
void handleFileAck(PBYTE buf, WORD wLen, DWORD dwUin, WORD wCookie, WORD wStatus, char* pszText);

// Handle a received file transfer request (direct & server)
void handleFileRequest(PBYTE buf, WORD wLen, DWORD dwUin, WORD wCookie, DWORD dwID1, DWORD dwID2, char* pszDescription, int nVersion);


#endif /* __ICQ_DIRECT_H */