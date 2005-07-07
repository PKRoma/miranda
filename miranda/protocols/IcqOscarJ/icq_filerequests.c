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
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"



extern WORD wListenPort;

void handleFileAck(PBYTE buf, WORD wLen, DWORD dwUin, DWORD dwCookie, WORD wStatus, char* pszText)
{
	char* pszFileName = NULL;
	DWORD dwFileSize;
	DWORD dwCookieUin;
	WORD wPort;
	WORD wFilenameLength;
	filetransfer* ft;

	
	// Find the filetransfer that belongs to this response
	if (!FindCookie(dwCookie, &dwCookieUin, &ft))
	{
		Netlib_Logf(ghDirectNetlibUser, "Error: Received unexpected file transfer request response");
		return;
	}

	FreeCookie(dwCookie);
	
	if (dwCookieUin != dwUin)
	{
		Netlib_Logf(ghDirectNetlibUser, "Error: UINs do not match in file transfer request response");
		return;
	}

	
	// If status != 0, a request has been denied
	if (wStatus != 0)
	{
		Netlib_Logf(ghDirectNetlibUser, "File transfer denied by %u,", dwUin);
		ProtoBroadcastAck(gpszICQProtoName, HContactFromUIN(dwUin, 1), ACKTYPE_FILE, ACKRESULT_DENIED, (HANDLE)ft, 0);

		FreeCookie(dwCookie);

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
		pszFileName = malloc(wFilenameLength+1);
		unpackString(&buf, pszFileName, wFilenameLength);
		pszFileName[wFilenameLength] = '\0';
	}
	wLen = wLen - 2 - wFilenameLength;
	
	// Total filesize
	unpackLEDWord(&buf, &dwFileSize);
	wLen -= 4;
	
	Netlib_Logf(ghDirectNetlibUser, "File transfer ack from %u, port %u, name %s, size %u", dwUin, ft->dwRemotePort, pszFileName, dwFileSize);
	
	OpenDirectConnection(ft->hContact, DIRECTCONN_FILE, ft);

	SAFE_FREE(&pszFileName);
}



// pszDescription points to a string with the reason
// buf points to the first data after the string
void handleFileRequest(PBYTE buf, WORD wLen, DWORD dwUin, DWORD dwCookie, DWORD dwID1, DWORD dwID2, char* pszDescription, int nVersion, BOOL bDC)
{
	char* pszFileName = NULL;
	DWORD dwFileSize;
	WORD wFilenameLength;
  BOOL bEmptyDesc = FALSE;


	if (!pszDescription || (strlen(pszDescription) == 0))
  {
		pszDescription = Translate("No description given");
    bEmptyDesc = TRUE;
  }
	
	// Empty port+pad
	buf += 4;
	wLen -= 4;

	
	// Filename
	unpackLEWord(&buf, &wFilenameLength);
	if (wFilenameLength > 0)
	{
		pszFileName = malloc(wFilenameLength + 1);
		unpackString(&buf, pszFileName, wFilenameLength);
		pszFileName[wFilenameLength] = '\0';
	}
	else
	{
		Netlib_Logf(ghDirectNetlibUser, "Ignoring malformed file send request");
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
		
		// Initialize a filetransfer struct
		ft = (filetransfer*)malloc(sizeof(filetransfer));
		memset(ft, 0, sizeof(filetransfer));
		ft->status = 0;
		ft->dwCookie = dwCookie;
		ft->szFilename = _strdup(pszFileName);
		ft->szDescription = _strdup(pszDescription);
		ft->dwUin = dwUin;
		ft->fileId = -1;
		ft->dwTotalSize = dwFileSize;
		ft->nVersion = nVersion;
		ft->TS1 = dwID1;
		ft->TS2 = dwID2;
    ft->bDC = bDC;
    ft->bEmptyDesc = bEmptyDesc;
		
		
		// Send chain event
		szBlob = (char*)malloc(sizeof(DWORD) + strlen(pszFileName) + strlen(pszDescription) + 2);
		*(PDWORD)szBlob = (DWORD)ft;
		strcpy(szBlob + sizeof(DWORD), pszFileName);
		strcpy(szBlob + sizeof(DWORD) + strlen(pszFileName) + 1, pszDescription);
		ccs.szProtoService = PSR_FILE;
		ccs.hContact = HContactFromUIN(dwUin, 1);
		ccs.wParam = 0;
		ccs.lParam = (LPARAM)&pre;
		pre.flags = 0;
		pre.timestamp = time(NULL);
		pre.szMessage = szBlob;
		pre.lParam = 0;

		CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);

		SAFE_FREE(&szBlob);
	}

	SAFE_FREE(&pszFileName);
}



void handleDirectCancel(directconnect *dc, PBYTE buf, WORD wLen, WORD wCommand, DWORD dwCookie, WORD wMessageType, WORD wStatus, WORD wFlags, char* pszText)
{
	Netlib_Logf(ghDirectNetlibUser, "handleDirectCancel: Unhandled cancel");
}



// *******************************************************************************



void icq_CancelFileTransfer(HANDLE hContact, filetransfer* ft)
{
	DWORD dwCookie;

	if (FindCookieByData(ft, &dwCookie, NULL))
		FreeCookie(dwCookie);      /* this bit stops a send that's waiting for acceptance */

	if (ft->hConnection)
	{
		ProtoBroadcastAck(gpszICQProtoName, ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0);
		Netlib_CloseHandle(ft->hConnection);
		ft->hConnection = NULL;
	}
	/* FIXME: Do not free ft, or anything therein, it is freed inside DC thread ! */
	#ifdef _DEBUG
		Netlib_Logf(ghDirectNetlibUser, "icq_CancelFileTransfer: OK");
	#endif
}
