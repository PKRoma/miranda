/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_ibb.h,v $
Revision       : $Revision: 5336 $
Last change on : $Date: 2007-04-28 13:14:46 +0300 (бс, 28 ря№ 2007) $
Last change by : $Author: ghazan $

*/

#ifndef _JABBER_IBB_H_
#define _JABBER_IBB_H_

typedef enum { JIBB_INIT, JIBB_CONNECT, JIBB_SENDING, JIBB_RECVING, JIBB_DONE, JIBB_ERROR } JABBER_IBB_STATE;

typedef struct {
	TCHAR* sid;
	TCHAR* srcJID;
	TCHAR* dstJID;
	DWORD dwTransferredSize;
	JABBER_IBB_STATE state;
	HANDLE hEvent;
	BOOL bStreamInitialized;
	BOOL bStreamClosed;
	WORD wPacketId;
	BOOL ( *pfnSend )( int blocksize, void *userdata );
	int ( *pfnRecv )( HANDLE hConn, void *userdata, char* buffer, int datalen );
	void ( *pfnFinal )( BOOL success, void *userdata );
	void *userdata;
} JABBER_IBB_TRANSFER;

void __cdecl JabberIbbSendThread( JABBER_IBB_TRANSFER *jibb );
void __cdecl JabberIbbReceiveThread( JABBER_IBB_TRANSFER *jibb );
BOOL JabberIbbProcessRecvdData( TCHAR *data, TCHAR *sid, TCHAR *seq );
void JabberFtHandleIbbIq( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );

#endif
