// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004,2005 Martin Öberg, Sam Kothari, Robert Rainwater
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
extern HANDLE hDirectNetlibUser;
extern char gpszICQProtoName[MAX_PATH];



void handleFileAck(PBYTE buf, WORD wLen, DWORD dwUin, WORD wCookie, WORD wStatus, char* pszText)
{

	char* pszFileName = NULL;
	DWORD dwFileSize;
	DWORD dwCookieUin;
	WORD wPort;
	WORD wFilenameLength;
	filetransfer* ft;

	
	// Find the filetransfer that belongs to this response
	if (!FindCookie(wCookie, &dwCookieUin, &ft))
	{
		Netlib_Logf(hDirectNetlibUser, "Error: Received unexpected file transfer request response");
		return;
	}

	FreeCookie(wCookie);
	
	if (dwCookieUin != dwUin)
	{
		Netlib_Logf(hDirectNetlibUser, "Error: UINs do not match in file transfer request response");
		return;
	}

	
	// If status != 0, a request has been denied
	if (wStatus != 0)
	{
		Netlib_Logf(hDirectNetlibUser, "File transfer denied by %u,", dwUin);
		ProtoBroadcastAck(gpszICQProtoName, HContactFromUIN(dwUin, 1), ACKTYPE_FILE, ACKRESULT_DENIED, (HANDLE)ft, 0);

		FreeCookie(wCookie);

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
	
	Netlib_Logf(hDirectNetlibUser, "File transfer ack from %u, port %u, name %s, size %u", dwUin, ft->dwRemotePort, pszFileName, dwFileSize);
	
	OpenDirectConnection(ft->hContact, DIRECTCONN_FILE, ft);

	SAFE_FREE(pszFileName);

}



// pszDescription points to a string with the reason
// buf points to the first data after the string
void handleFileRequest(PBYTE buf, WORD wLen, DWORD dwUin, WORD wCookie, DWORD dwID1, DWORD dwID2, char* pszDescription, int nVersion)
{

	char* pszFileName = NULL;
	DWORD dwFileSize;
	WORD wFilenameLength;


	if (!pszDescription || (strlen(pszDescription) == 0))
		pszDescription = Translate("No description given");

	
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
		Netlib_Logf(hDirectNetlibUser, "Ignoring malformed file send request");
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
		ft->wCookie = wCookie;
		ft->szFilename = _strdup(pszFileName);
		ft->szDescription = _strdup(pszDescription);
		ft->dwUin = dwUin;
		ft->fileId = -1;
		ft->dwTotalSize = dwFileSize;
		ft->nVersion = nVersion;
		ft->TS1 = dwID1;
		ft->TS2 = dwID2;
		
		
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

		SAFE_FREE(szBlob);

	}
	

	SAFE_FREE(pszFileName);
	
}



void handleDirectCancel(directconnect *dc, PBYTE buf, WORD wLen, WORD wCommand, WORD wCookie, WORD wMessageType, WORD wStatus, WORD wFlags, char* pszText)
{

	Netlib_Logf(hDirectNetlibUser, "handleDirectCancel: Unhandled cancel");

}



// *******************************************************************************



void icq_sendFileAcceptDirect(HANDLE hContact, filetransfer* ft)
{

	icq_packet packet;

	
	buildDirectPacketHeader(&packet, 20, DIRECT_ACK, ft->wCookie, MTYPE_FILEREQ, 0, 0);
	packLEWord(&packet, 0);	   // modifier 
	packLEWord(&packet, 1);	  // description
	packByte(&packet, 0);
	packWord(&packet, wListenPort);
	packLEWord(&packet, 0);
	packLEWord(&packet, 1);	  // filename
	packByte(&packet, 0);
	packLEDWord(&packet, 0);  // file size 
	packLEDWord(&packet, wListenPort);		// FIXME: ideally we want to open a new port for this

	OpenDirectConnection(hContact, DIRECTCONN_STANDARD, &packet);

	Netlib_Logf(hDirectNetlibUser, "accepted dc");

}



void icq_CancelFileTransfer(HANDLE hContact, filetransfer* ft)
{

	WORD wCookie;

	if (FindCookieByData(ft, &wCookie, NULL))
		FreeCookie(wCookie);      /* this bit stops a send that's waiting for acceptance */

	if (ft->hConnection)
	{
		ProtoBroadcastAck(gpszICQProtoName, ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0);
		Netlib_CloseHandle(ft->hConnection);
		ft->hConnection = NULL;
	}
	/* FIXME: Do not free ft, or anything therein, it is freed inside DC thread ! */
	#ifdef _DEBUG
		Netlib_Logf(hDirectNetlibUser, "icq_CancelFileTransfer: OK");
	#endif

}



void icq_sendFileSendDirectv7(DWORD dwUin, HANDLE hContact, WORD wCookie, char* pszFiles, char* pszDescription, DWORD dwTotalSize)
{

	icq_packet packet;

	
	buildDirectPacketHeader(&packet, (WORD)(20 + strlen(pszDescription) + strlen(pszFiles)), DIRECT_MESSAGE, wCookie, MTYPE_FILEREQ, 0, 0);
	packLEWord(&packet, 0);	   // modifier
	packLEWord(&packet, (WORD)(strlen(pszDescription) + 1));
	packBuffer(&packet, pszDescription, (WORD)(strlen(pszDescription) + 1));
	packLEDWord(&packet, 0);	 // listen port
	packLEWord(&packet, (WORD)(strlen(pszFiles) + 1));
	packBuffer(&packet, pszFiles, (WORD)(strlen(pszFiles) + 1));
	packLEDWord(&packet, dwTotalSize);
	packLEDWord(&packet, 0);		// listen port (again)

	Netlib_Logf(hDirectNetlibUser, "Sending v7 file request direct");
	OpenDirectConnection(hContact, DIRECTCONN_STANDARD, &packet);

}
