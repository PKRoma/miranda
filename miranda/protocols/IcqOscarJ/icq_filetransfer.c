// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001,2002 Jon Keating, Richard Hughes
// Copyright � 2002,2003,2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004,2005 Joe Kucera
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


static void file_buildProtoFileTransferStatus(filetransfer* ft, PROTOFILETRANSFERSTATUS* pfts)
{
  memset(pfts, 0, sizeof(PROTOFILETRANSFERSTATUS));
  pfts->cbSize = sizeof(PROTOFILETRANSFERSTATUS);
  pfts->hContact = ft->hContact;
  pfts->sending = ft->sending;
  if(ft->sending)
    pfts->files = ft->files;
  else
    pfts->files = NULL;  /* FIXME */
  pfts->totalFiles = ft->dwFileCount;
  pfts->currentFileNumber = ft->iCurrentFile;
  pfts->totalBytes = ft->dwTotalSize;
  pfts->totalProgress = ft->dwBytesDone;
  pfts->workingDir = ft->szSavePath;
  pfts->currentFile = ft->szThisFile;
  pfts->currentFileSize = ft->dwThisFileSize;
  pfts->currentFileTime = ft->dwThisFileDate;
  pfts->currentFileProgress = ft->dwFileBytesDone;
}



static void file_sendTransferSpeed(directconnect* dc)
{
  icq_packet packet;

  directPacketInit(&packet, 5);
  packByte(&packet, PEER_FILE_SPEED);       /* Ident */
  packLEDWord(&packet, dc->ft->dwTransferSpeed);
  sendDirectPacket(dc->hConnection, &packet);
}



static void file_sendNick(directconnect* dc)
{
  icq_packet packet;
  char* szNick;
  WORD wNickLen;
  DBVARIANT dbv;


  dbv.type = DBVT_DELETED;
  if (ICQGetContactSetting(NULL, "Nick", &dbv))
    szNick = "";
  else
    szNick = dbv.pszVal;

  wNickLen = strlennull(szNick);

  directPacketInit(&packet, (WORD)(8 + wNickLen));
  packByte(&packet, PEER_FILE_INIT_ACK);        /* Ident */
  packLEDWord(&packet, dc->ft->dwTransferSpeed);
  packLEWord(&packet, (WORD)(wNickLen + 1));
  packBuffer(&packet, szNick, (WORD)(wNickLen + 1));
  sendDirectPacket(dc->hConnection, &packet);
  ICQFreeVariant(&dbv);
}



static void file_sendNextFile(directconnect* dc)
{
  icq_packet packet;
  struct _stat statbuf;
  char *pszThisFileName;
  char szThisSubDir[MAX_PATH];


  if (dc->ft->iCurrentFile >= (int)dc->ft->dwFileCount)
  {
    Netlib_CloseHandle(dc->hConnection);
    dc->hConnection = dc->ft->hConnection = NULL;
    ICQBroadcastAck(dc->ft->hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, dc->ft, 0);
    return;
  }

  dc->ft->szThisFile = _strdup(dc->ft->files[dc->ft->iCurrentFile]);
  if (_stat(dc->ft->szThisFile, &statbuf))
  {
    icq_LogMessage(LOG_ERROR, Translate("Your file transfer has been aborted because one of the files that you selected to send is no longer readable from the disk. You may have deleted or moved it."));
    Netlib_CloseHandle(dc->hConnection);
    dc->hConnection = dc->ft->hConnection = NULL;
    return;
  }

  szThisSubDir[0] = '\0';

  if (((pszThisFileName = strrchr(dc->ft->szThisFile, '\\')) == NULL) &&
    ((pszThisFileName = strrchr(dc->ft->szThisFile, '/')) == NULL))
  {
    pszThisFileName = dc->ft->szThisFile;
  }
  else
  {
    int i;
    int len;


    /* find an earlier subdirectory to be used as a container */
    for (i = dc->ft->iCurrentFile - 1; i >= 0; i--)
    {
      len = strlennull(dc->ft->files[i]);
      if (!_strnicmp(dc->ft->files[i], dc->ft->szThisFile, len) &&
        (dc->ft->szThisFile[len] == '\\' || dc->ft->szThisFile[len] == '/'))
      {
        char* pszLastBackslash;

        if (((pszLastBackslash = strrchr(dc->ft->files[i], '\\')) == NULL) &&
          ((pszLastBackslash = strrchr(dc->ft->files[i], '/')) == NULL))
        {
          strcpy(szThisSubDir, dc->ft->files[i]);
        }
        else
        {
          len = pszLastBackslash - dc->ft->files[i] + 1;
          strncpy(szThisSubDir, dc->ft->szThisFile + len,
            pszThisFileName - dc->ft->szThisFile - len);
          szThisSubDir[pszThisFileName - dc->ft->szThisFile - len] = '\0';
        }
      }
    }
    pszThisFileName++; // skip backslash
  }

  if (statbuf.st_mode&_S_IFDIR)
  {
    dc->ft->currentIsDir = 1;
  }
  else
  {
    dc->ft->currentIsDir = 0;
    dc->ft->fileId = _open(dc->ft->szThisFile, _O_BINARY | _O_RDONLY);
    if (dc->ft->fileId == -1)
    {
      icq_LogMessage(LOG_ERROR, Translate("Your file transfer has been aborted because one of the files that you selected to send is no longer readable from the disk. You may have deleted or moved it."));
      Netlib_CloseHandle(dc->hConnection);
      dc->hConnection = dc->ft->hConnection = NULL;
      return;
    }

  }
  dc->ft->dwThisFileSize = statbuf.st_size;
  dc->ft->dwThisFileDate = statbuf.st_mtime;
  dc->ft->dwFileBytesDone = 0;


  directPacketInit(&packet, (WORD)(20 + strlennull(pszThisFileName) + strlennull(szThisSubDir)));
  packByte(&packet, PEER_FILE_NEXTFILE);      /* Ident */
  packByte(&packet, (BYTE)((statbuf.st_mode & _S_IFDIR) != 0)); // Is subdir
  packLEWord(&packet, (WORD)(strlennull(pszThisFileName) + 1));
  packBuffer(&packet, pszThisFileName, (WORD)(strlennull(pszThisFileName) + 1));
  packLEWord(&packet, (WORD)(strlennull(szThisSubDir) + 1));
  packBuffer(&packet, szThisSubDir, (WORD)(strlennull(szThisSubDir) + 1));
  packLEDWord(&packet, dc->ft->dwThisFileSize);
  packLEDWord(&packet, statbuf.st_mtime);
  packLEDWord(&packet, dc->ft->dwTransferSpeed);

  sendDirectPacket(dc->hConnection, &packet);

  ICQBroadcastAck(dc->ft->hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, dc->ft, 0);
}



static void file_sendResume(filetransfer* ft)
{
  icq_packet packet;

  directPacketInit(&packet, 17);
  packByte(&packet, PEER_FILE_RESUME);        /* Ident */
  packLEDWord(&packet, ft->dwFileBytesDone);  /* file resume */
  packLEDWord(&packet, 0);                    /* unknown */
  packLEDWord(&packet, ft->dwTransferSpeed);
  packLEDWord(&packet, ft->iCurrentFile + 1); /* file number */
  sendDirectPacket(ft->hConnection, &packet);
}



static void file_sendData(directconnect* dc)
{
  icq_packet packet;
  BYTE buf[2048];
  int bytesRead = 0;


  if (!dc->ft->currentIsDir)
  {
    if (dc->ft->fileId == -1)
      return;
    bytesRead = _read(dc->ft->fileId, buf, sizeof(buf));
    if (bytesRead == -1)
      return;

    directPacketInit(&packet, (WORD)(1 + bytesRead));
    packByte(&packet, 6);      /* Ident */
    packBuffer(&packet, buf, (WORD)bytesRead);
    sendDirectPacket(dc->hConnection, &packet);
  }

  dc->ft->dwBytesDone += bytesRead;
  dc->ft->dwFileBytesDone += bytesRead;

  if(GetTickCount() > dc->ft->dwLastNotify + 500 || bytesRead == 0)
  {
    PROTOFILETRANSFERSTATUS pfts;

    file_buildProtoFileTransferStatus(dc->ft, &pfts);
    ICQBroadcastAck(dc->ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, dc->ft, (LPARAM)&pfts);
    dc->ft->dwLastNotify = GetTickCount();
  }

  if (bytesRead == 0)
  {
    if (!dc->ft->currentIsDir) _close(dc->ft->fileId);
    dc->ft->fileId = -1;
    dc->wantIdleTime = 0;
    dc->ft->iCurrentFile++;
    file_sendNextFile(dc);   /* this will close the socket if no more files */
  }
}



void handleFileTransferIdle(directconnect* dc)
{
  file_sendData(dc);
}



void icq_sendFileResume(filetransfer* ft, int action, const char* szFilename)
{
  int openFlags;


  if (ft->hConnection == NULL)
    return;

  switch (action)
  {
    case FILERESUME_RESUME:
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
      ft->szThisFile = _strdup(szFilename);
      ft->dwFileBytesDone = 0;
      break;
  }

  ft->fileId = _open(ft->szThisFile, openFlags, _S_IREAD | _S_IWRITE);
  if (ft->fileId == -1)
  {
    icq_LogMessage(LOG_ERROR, Translate("Your file receive has been aborted because Miranda could not open the destination file in order to write to it. You may be trying to save to a read-only folder."));
    Netlib_CloseHandle(ft->hConnection);
    ft->hConnection = NULL;
    return;
  }

  if (action == FILERESUME_RESUME)
    ft->dwFileBytesDone = _lseek(ft->fileId, 0, SEEK_END);
  else
    _lseek(ft->fileId, ft->dwFileBytesDone, SEEK_SET);

  ft->dwBytesDone += ft->dwFileBytesDone;

  file_sendResume(ft);

  ICQBroadcastAck(ft->hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, ft, 0);
}



/* a file transfer looks like this:
S: 0
R: 5
R: 1
S: 2
R: 3
S: 6 * many
(for more files, send 2, 3, 6*many)
*/
void handleFileTransferPacket(directconnect* dc, PBYTE buf, WORD wLen)
{
  if (wLen < 1)
    return;

  NetLog_Direct("Handling file packet");

  switch (buf[0])
  {
    case PEER_FILE_INIT:   /* first packet of a file transfer */
      if (dc->initialised)
        return;
      if (wLen < 19)
        return;
      buf += 5;  /* id, and unknown 0 */
      dc->type = DIRECTCONN_FILE;
      {
        DWORD dwFileCount;
        DWORD dwTotalSize;
        DWORD dwTransferSpeed;
        WORD wNickLength;
        int bAdded;

        unpackLEDWord(&buf, &dwFileCount);
        unpackLEDWord(&buf, &dwTotalSize);
        unpackLEDWord(&buf, &dwTransferSpeed);
        unpackLEWord(&buf, &wNickLength);

        dc->ft = FindExpectedFileRecv(dc->dwRemoteUin, dwTotalSize);
        if (dc->ft == NULL)
        {
          NetLog_Direct("Unexpected file receive");
          Netlib_CloseHandle(dc->hConnection);
          dc->hConnection = NULL;
          return;
        }
        dc->ft->dwFileCount = dwFileCount;
        dc->ft->dwTransferSpeed = dwTransferSpeed;
        dc->ft->hContact = HContactFromUIN(dc->ft->dwUin, &bAdded);
        dc->ft->dwBytesDone = 0;
        dc->ft->iCurrentFile = -1;
        dc->ft->fileId = -1;        
        dc->ft->hConnection = dc->hConnection;
        dc->ft->dwLastNotify = GetTickCount();

        dc->initialised = 1;

        file_sendTransferSpeed(dc);
        file_sendNick(dc);
      }
      ICQBroadcastAck(dc->ft->hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, dc->ft, 0);
      break;

    case PEER_FILE_INIT_ACK:
      if (wLen < 8)
        return;
      buf++;
      unpackLEDWord(&buf, &dc->ft->dwTransferSpeed);
      /* followed by nick */
      file_sendNextFile(dc);
      break;

    case PEER_FILE_NEXTFILE:
      if (wLen < 20)
        return;
      buf++;  /* id */
      {
        WORD wThisFilenameLen, wSubdirLen;
        BYTE isDirectory;
        char *szFullPath;

        unpackByte(&buf, &isDirectory);
        unpackLEWord(&buf, &wThisFilenameLen);
        if (wLen < 19 + wThisFilenameLen)
          return;
        SAFE_FREE(&dc->ft->szThisFile);
        dc->ft->szThisFile = (char *)malloc(wThisFilenameLen + 1);
        memcpy(dc->ft->szThisFile, buf, wThisFilenameLen);
        dc->ft->szThisFile[wThisFilenameLen] = '\0';
        buf += wThisFilenameLen;

        unpackLEWord(&buf, &wSubdirLen);
        if (wLen < 18 + wThisFilenameLen + wSubdirLen)
          return;
        SAFE_FREE(&dc->ft->szThisSubdir);
        dc->ft->szThisSubdir = (char *)malloc(wSubdirLen + 1);
        memcpy(dc->ft->szThisSubdir, buf, wSubdirLen);
        dc->ft->szThisSubdir[wSubdirLen] = '\0';
        buf += wSubdirLen;

        unpackLEDWord(&buf, &dc->ft->dwThisFileSize);
        unpackLEDWord(&buf,  &dc->ft->dwThisFileDate);
        unpackLEDWord(&buf,  &dc->ft->dwTransferSpeed);

        /* no cheating with paths */
        if (strstr(dc->ft->szThisFile, "..\\") || strstr(dc->ft->szThisFile, "../") ||
          strstr(dc->ft->szThisFile, ":\\") || strstr(dc->ft->szThisFile, ":/") ||
          dc->ft->szThisFile[0] == '\\' || dc->ft->szThisFile[0] == '/')
        {
          NetLog_Direct("Invalid path information");
          break;
        }
        if (strstr(dc->ft->szThisSubdir, "..\\") || strstr(dc->ft->szThisSubdir, "../") ||
          strstr(dc->ft->szThisSubdir, ":\\") || strstr(dc->ft->szThisSubdir, ":/") ||
          dc->ft->szThisSubdir[0] == '\\' || dc->ft->szThisSubdir[0] == '/')
        {
          NetLog_Direct("Invalid path information");
          break;
        }

        szFullPath = (char*)malloc(strlennull(dc->ft->szSavePath)+strlennull(dc->ft->szThisSubdir)+strlennull(dc->ft->szThisFile)+3);
        szFullPath[0] = '\0';
        strcpy(szFullPath, dc->ft->szSavePath);
        if (strlennull(szFullPath) && szFullPath[strlennull(szFullPath)-1] != '\\') strcat(szFullPath, "\\");
        strcat(szFullPath, dc->ft->szThisSubdir);
        if (strlennull(szFullPath) && szFullPath[strlennull(szFullPath)-1] != '\\') strcat(szFullPath, "\\");
        _chdir(szFullPath); // set current dir - not very useful
        strcat(szFullPath, dc->ft->szThisFile);
        // we joined the full path to dest file
        SAFE_FREE(&dc->ft->szThisFile);
        dc->ft->szThisFile = szFullPath;

        dc->ft->dwFileBytesDone = 0;
        dc->ft->iCurrentFile++;

        if (isDirectory)
        {
          _mkdir(dc->ft->szThisFile);
          dc->ft->fileId = -1;
        }
        else
        {
          /* file resume */
          PROTOFILETRANSFERSTATUS pfts = {0};

          file_buildProtoFileTransferStatus(dc->ft, &pfts);
          if (ICQBroadcastAck(dc->ft->hContact, ACKTYPE_FILE, ACKRESULT_FILERESUME, dc->ft, (LPARAM)&pfts))
          break;   /* UI supports resume: it will call PS_FILERESUME */

          dc->ft->fileId = _open(dc->ft->szThisFile, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
          if (dc->ft->fileId == -1)
          {
            icq_LogMessage(LOG_ERROR, Translate("Your file receive has been aborted because Miranda could not open the destination file in order to write to it. You may be trying to save to a read-only folder."));
            Netlib_CloseHandle(dc->hConnection);
            dc->hConnection = dc->ft->hConnection = NULL;
            break;
          }
        }
      }
      file_sendResume(dc->ft);
      ICQBroadcastAck(dc->ft->hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, dc->ft, 0);
      break;

    case PEER_FILE_RESUME:
      if (dc->ft->fileId == -1 && !dc->ft->currentIsDir)
        return;
      if (wLen < 13)
        return;
      if (wLen < 17)
        NetLog_Direct("Warning: Received short PEER_FILE_RESUME");
      buf++;
      {
        DWORD dwRestartFrom;

        unpackLEDWord(&buf, &dwRestartFrom);
        if (dwRestartFrom > dc->ft->dwThisFileSize)
          return;
        buf += 4;  /* unknown. 0 */
        unpackLEDWord(&buf, &dc->ft->dwTransferSpeed);
        buf += 4;  /* unknown. 1 */
        if (!dc->ft->currentIsDir)
          _lseek(dc->ft->fileId, dwRestartFrom, 0);
        dc->wantIdleTime = 1;
        dc->ft->dwBytesDone += dwRestartFrom;
        dc->ft->dwFileBytesDone += dwRestartFrom;
      }
      break;

    case PEER_FILE_SPEED:
      if (wLen < 5)
        return;
      buf++;
      unpackLEDWord(&buf, &dc->ft->dwTransferSpeed);
      dc->ft->dwLastNotify = GetTickCount();
      break;

    case PEER_FILE_DATA:
      if (!dc->ft->currentIsDir)
      {
        if (dc->ft->fileId == -1)
          break;
        buf++; wLen--;
        _write(dc->ft->fileId, buf, wLen);
      }
      else
        wLen = 0;
      dc->ft->dwBytesDone += wLen;
      dc->ft->dwFileBytesDone += wLen;
      if(GetTickCount() > dc->ft->dwLastNotify + 500 || wLen < 2048) 
      {
        PROTOFILETRANSFERSTATUS pfts;

        file_buildProtoFileTransferStatus(dc->ft, &pfts);
        ICQBroadcastAck(dc->ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, dc->ft, (LPARAM)&pfts);
        dc->ft->dwLastNotify = GetTickCount();
      }
      if (wLen < 2048)
      {
        /* EOF */
        if (!dc->ft->currentIsDir)
          _close(dc->ft->fileId);
        dc->ft->fileId = -1;
        if ((DWORD)dc->ft->iCurrentFile == dc->ft->dwFileCount - 1)
        {
          dc->type = DIRECTCONN_CLOSING;     /* this guarantees that we won't accept any more data but that the sender is still free to closesocket() neatly */
          ICQBroadcastAck(dc->ft->hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, dc->ft, 0);
        }
      }
      break;

    default:
      NetLog_Direct("Unknown file transfer packet ignored.");
      break;
  }
}
