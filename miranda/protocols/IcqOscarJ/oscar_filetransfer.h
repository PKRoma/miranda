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

#ifndef __OSCAR_FILETRANSFER_H
#define __OSCAR_FILETRANSFER_H


void InitOscarFileTransfer();
void UninitOscarFileTransfer();


#define FT_MAGIC_ICQ    0x00
#define FT_MAGIC_OSCAR  0x4F

#define OFT_BUFFER_SIZE 8192

typedef struct {
  char* szContainer;
  char* szFile;
} oft_file_record;

char* FindFilePathContainer(const char** files, int iFile, char* szContainer);


typedef struct {
  message_cookie_data pMessage;
  BYTE ft_magic;
} basic_filetransfer;

typedef struct {
  message_cookie_data pMessage;
  BYTE ft_magic;
  HANDLE hContact;
  int initialized;
  int sending;
  int containerCount;
  char **file_containers;
  oft_file_record* files;
  char **files_ansi; // sending only 
  int iCurrentFile;
  int currentIsDir;
  int bUseProxy;
  DWORD dwProxyIP;
  DWORD dwRemoteInternalIP;
  DWORD dwRemoteExternalIP;
  WORD wRemotePort;
  char *szSavePath;
  char *szDescription;
  char *szThisFile; 
  char *szThisPath;
  // Request sequence
  DWORD dwCookie;
  WORD wReqNum;
  // OFT2 header data
  WORD wEncrypt, wCompress;
  WORD wFilesCount,wFilesLeft;
  WORD wPartsCount, wPartsLeft;
  DWORD64 qwTotalSize;
  DWORD64 qwThisFileSize;
  DWORD dwThisFileDate; // modification date
  DWORD dwThisFileCheck;
  DWORD dwRecvForkCheck, dwThisForkSize;
  DWORD dwThisFileCreation; // creation date (not used)
  DWORD dwThisForkCheck;
  DWORD64 qwBytesDone;
  DWORD dwRecvFileCheck;
  char rawIDString[32];
  BYTE bHeaderFlags;
  BYTE bNameOff, bSizeOff;
  BYTE rawDummy[69];
  BYTE rawMacInfo[16];
  WORD wEncoding, wSubEncoding;
  WORD cbRawFileName;
  char *rawFileName;
  // helper data
  DWORD64 qwFileBytesDone;
  int fileId;
  struct oscar_connection* connection;
  void* listener;
  DWORD dwLastNotify;
  int resumeAction;
} oscar_filetransfer;

#define OFT_TYPE_REQUEST        0x0101 // I am going to send you this file, is that ok?
#define OFT_TYPE_READY          0x0202 // Yes, it is ok for you to send me that file
#define OFT_TYPE_DONE           0x0204 // I received that file with no problems
#define OFT_TYPE_RESUMEREQUEST  0x0205 // Resume transferring from position
#define OFT_TYPE_RESUMEREADY    0x0106 // Ok, I am ready to send it
#define OFT_TYPE_RESUMEACK      0x0207 // Fine, ready to receive

void SafeReleaseFileTransfer(void **ft);

struct oscar_connection
{
  HANDLE hContact;
  HANDLE hConnection;
  int status;
  DWORD dwUin;
  uid_str szUid;
  DWORD dwLocalInternalIP;
  DWORD dwLocalExternalIP;
  int type;
  int incoming;
  oscar_filetransfer *ft;
  int wantIdleTime;
};

#define OCT_NORMAL      0
#define OCT_REVERSE     1
#define OCT_PROXY       2
#define OCT_PROXY_INIT  3
#define OCT_PROXY_RECV  4
#define OCT_CLOSING     10

#define OCS_READY       0
#define OCS_CONNECTED   1
#define OCS_NEGOTIATION 2
#define OCS_RESUME      3
#define OCS_DATA        4
#define OCS_PROXY       8
#define OCS_WAITING     10

typedef struct {
  WORD wPort;
  HANDLE hBoundPort;
  oscar_filetransfer* ft;
} oscar_listener;

oscar_listener* CreateOscarListener(oscar_filetransfer *ft, NETLIBNEWCONNECTIONPROC_V2 handler);
void ReleaseOscarListener(oscar_listener **pListener);

filetransfer *CreateIcqFileTransfer();

int IsValidFileTransfer(void *ft);
int IsValidOscarTransfer(void *ft);

void OpenOscarConnection(HANDLE hContact, oscar_filetransfer *ft, int type);
void CloseOscarConnection(oscar_connection *oc);

void handleRecvServMsgOFT(unsigned char *buf, WORD wLen, DWORD dwUin, char *szUID, DWORD dwID1, DWORD dwID2, WORD wCommand);
void handleRecvServResponseOFT(unsigned char *buf, WORD wLen, DWORD dwUin, char *szUID, void* ft);

int oftInitTransfer(HANDLE hContact, DWORD dwUin, char *szUid, char** files, char* pszDesc);

DWORD oftFileAllow(HANDLE hContact, WPARAM wParam, LPARAM lParam);
DWORD oftFileDeny(HANDLE hContact, WPARAM wParam, LPARAM lParam);
DWORD oftFileCancel(HANDLE hContact, WPARAM wParam, LPARAM lParam);

void oftFileResume(oscar_filetransfer *ft, int action, const char *szFilename);

#endif /* __OSCAR_FILETRANSFER_H */

