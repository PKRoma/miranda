/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
Copyright ( C ) 2007     Maxim Mluhov
Copyright ( C ) 2008-09  Dmitriy Chervov

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

File name      : $URL: https://miranda.svn.sourceforge.net/svnroot/miranda/trunk/miranda/protocols/JabberG/jabber_message_handlers.cpp $
Revision       : $Revision: 7392 $
Last change on : $Date: 2008-03-04 23:50:26 +0200 $
Last change by : $Author: dchervov $

*/

#include "jabber.h"
#include "jabber_message_manager.h"

BOOL CJabberProto::OnMessageError( HXML node, ThreadData *pThreadData, CJabberMessageInfo* pInfo )
{
	// we check if is message delivery failure
	int id = JabberGetPacketID( node );
	JABBER_LIST_ITEM* item = ListGetItemPtr( LIST_ROSTER, pInfo->GetFrom() );
	if ( item != NULL ) { // yes, it is
		TCHAR *szErrText = JabberErrorMsg( pInfo->GetChildNode() );
		char *errText = mir_t2a(szErrText);
		JSendBroadcast( pInfo->GetHContact(), ACKTYPE_MESSAGE, ACKRESULT_FAILED, ( HANDLE ) id, (LPARAM)errText );
		mir_free(errText);
		mir_free(szErrText);
	}
	return TRUE;
}

BOOL CJabberProto::OnMessageIbb( HXML node, ThreadData *pThreadData, CJabberMessageInfo* pInfo )
{
	BOOL bOk = FALSE;
	const TCHAR *sid = xmlGetAttrValue( pInfo->GetChildNode(), _T("sid"));
	const TCHAR *seq = xmlGetAttrValue( pInfo->GetChildNode(), _T("seq"));
	if ( sid && seq && xmlGetText( pInfo->GetChildNode() ) ) {
		bOk = OnIbbRecvdData( xmlGetText( pInfo->GetChildNode() ), sid, seq );
	}
	return TRUE;
}

BOOL CJabberProto::OnMessagePubsubEvent( HXML node, ThreadData *pThreadData, CJabberMessageInfo* pInfo )
{
	OnProcessPubsubEvent( node );
	return TRUE;
}

BOOL CJabberProto::OnMessageGroupchat( HXML node, ThreadData *pThreadData, CJabberMessageInfo* pInfo )
{
	JABBER_LIST_ITEM *chatItem = ListGetItemPtr( LIST_CHATROOM, pInfo->GetFrom() );
	if ( chatItem )
	{	// process GC message
		GroupchatProcessMessage( node );
	} else
	{	// got message from unknown conference... let's leave it :)
//			TCHAR *conference = NEWTSTR_ALLOCA(from);
//			if (TCHAR *s = _tcschr(conference, _T('/'))) *s = 0;
//			XmlNode p( "presence" ); xmlAddAttr( p, "to", conference ); xmlAddAttr( p, "type", "unavailable" );
//			info->send( p );
	}
	return TRUE;
}
