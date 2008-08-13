/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
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

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#ifndef _JABBER_BYTE_H_
#define _JABBER_BYTE_H_

typedef enum { JBT_INIT, JBT_AUTH, JBT_CONNECT, JBT_SOCKSERR, JBT_SENDING, JBT_RECVING, JBT_DONE, JBT_ERROR } JABBER_BYTE_STATE;

struct CJabberProto;

typedef struct
{
	TCHAR* sid;
	TCHAR* srcJID;
	TCHAR* dstJID;
	TCHAR* streamhostJID;
	TCHAR* iqId;
	JABBER_BYTE_STATE state;
	HANDLE hConn;
	HANDLE hEvent;
	XmlNode iqNode;
	BOOL ( CJabberProto::*pfnSend )( HANDLE hConn, void *userdata );
	int ( CJabberProto::*pfnRecv )( HANDLE hConn, void *userdata, char* buffer, int datalen );
	void ( CJabberProto::*pfnFinal )( BOOL success, void *userdata );
	void *userdata;

	// XEP-0065 proxy support
	BOOL bProxyDiscovered;
	HANDLE hProxyEvent;
	TCHAR* szProxyHost;
	TCHAR* szProxyPort;
	TCHAR* szProxyJid;
	TCHAR* szStreamhostUsed;
	BOOL bStreamActivated;
	HANDLE hSendEvent;

} JABBER_BYTE_TRANSFER;

#endif
