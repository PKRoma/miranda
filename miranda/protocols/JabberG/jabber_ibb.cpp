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

#include "jabber.h"
#include "jabber_iq.h"
#include "jabber_ibb.h"
#include "jabber_caps.h"

#define JABBER_IBB_BLOCK_SIZE 2048

void JabberIbbFreeJibb( JABBER_IBB_TRANSFER *jibb )
{
	if ( jibb )  {
		filetransfer* pft = (filetransfer *)jibb->userdata;
		if ( pft )
			pft->jibb = NULL;

		mir_free( jibb->srcJID );
		mir_free( jibb->dstJID );
		mir_free( jibb->sid );

		mir_free( jibb );
}	}

void CJabberProto::OnFtHandleIbbIq( HXML iqNode, void *userdata, CJabberIqInfo* pInfo )
{
	if ( !_tcscmp( pInfo->GetChildNodeName(), _T("open")))
		FtHandleIbbRequest( iqNode, TRUE );
	else if ( !_tcscmp( pInfo->GetChildNodeName(), _T("close")))
		FtHandleIbbRequest( iqNode, FALSE );
	else if ( !_tcscmp( pInfo->GetChildNodeName(), _T("data"))) {
		BOOL bOk = FALSE;
		const TCHAR *sid = xmlGetAttrValue( pInfo->GetChildNode(), _T("sid"));
		const TCHAR *seq = xmlGetAttrValue( pInfo->GetChildNode(), _T("seq"));
		if ( sid && seq && xmlGetText( pInfo->GetChildNode())) {
			bOk = OnIbbRecvdData( xmlGetText( pInfo->GetChildNode()), sid, seq );
		}
		if ( bOk ) {
			XmlNodeIq iq( "result", pInfo );
			m_ThreadInfo->send( iq );
		}
		else {
			XmlNodeIq iq( "error", pInfo );
			HXML e = xmlAddChild( iq, _T("error")); xmlAddAttr( e, _T("code"), 404 ); xmlAddAttr( e, "type", _T("cancel"));
			HXML na = xmlAddChild( e, "item-not-found" ); xmlAddAttr( na, "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );
			m_ThreadInfo->send( iq );
		}
	}
}

void CJabberProto::OnIbbInitiateResult( HXML iqNode, void *userdata, CJabberIqInfo* pInfo )
{
	JABBER_IBB_TRANSFER *jibb = ( JABBER_IBB_TRANSFER * )pInfo->GetUserData();
	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT )
		jibb->bStreamInitialized = TRUE;
	if ( jibb->hEvent )
		SetEvent( jibb->hEvent );
}

void CJabberProto::OnIbbCloseResult( HXML iqNode, void *userdata, CJabberIqInfo* pInfo )
{
	JABBER_IBB_TRANSFER *jibb = ( JABBER_IBB_TRANSFER * )pInfo->GetUserData();
	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT )
		jibb->bStreamClosed = TRUE;
	if ( jibb->hEvent )
		SetEvent( jibb->hEvent );
}

void CJabberProto::IbbSendThread( JABBER_IBB_TRANSFER *jibb )
{
	Log( "Thread started: type=ibb_send" );
	
	XmlNodeIq iq( m_iqManager.AddHandler( &CJabberProto::OnIbbInitiateResult, JABBER_IQ_TYPE_SET, jibb->dstJID, 0, -1, jibb ));
	HXML openNode = xmlAddChild( iq, "open" );
	xmlAddAttr( openNode, "sid", jibb->sid );
	xmlAddAttr( openNode, _T("block-size"), JABBER_IBB_BLOCK_SIZE );
	xmlAddAttr( openNode, "xmlns", JABBER_FEAT_IBB );

	jibb->hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	jibb->bStreamInitialized = FALSE;
	jibb->bStreamClosed = FALSE;
	jibb->state = JIBB_SENDING;

	m_ThreadInfo->send( iq );

	WaitForSingleObject( jibb->hEvent, INFINITE );
	CloseHandle( jibb->hEvent );
	jibb->hEvent = NULL;

	if ( jibb->bStreamInitialized ) {

		jibb->wPacketId = 0;

		BOOL bSent = (this->*jibb->pfnSend)( JABBER_IBB_BLOCK_SIZE, jibb->userdata );

		if ( !jibb->bStreamClosed )
		{
			XmlNodeIq iq2( m_iqManager.AddHandler( &CJabberProto::OnIbbCloseResult, JABBER_IQ_TYPE_SET, jibb->dstJID, 0, -1, jibb ));
			HXML closeNode = xmlAddChild( iq2, "close" );
			xmlAddAttr( closeNode, "sid", jibb->sid );
			xmlAddAttr( closeNode, "xmlns", JABBER_FEAT_IBB );

			jibb->hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

			m_ThreadInfo->send( iq2 );

			WaitForSingleObject( jibb->hEvent, INFINITE );
			CloseHandle( jibb->hEvent );
			jibb->hEvent = NULL;

			if ( jibb->bStreamClosed && bSent )
				jibb->state = JIBB_DONE;

		} else {
			jibb->state = JIBB_ERROR;
		}
	}

	(this->*jibb->pfnFinal)(( jibb->state==JIBB_DONE )?TRUE:FALSE, jibb->userdata );
	jibb->userdata = NULL;
	JabberIbbFreeJibb( jibb );
}

void __cdecl CJabberProto::IbbReceiveThread( JABBER_IBB_TRANSFER *jibb )
{
	Log( "Thread started: type=ibb_recv" );

	filetransfer *ft = (filetransfer *)jibb->userdata;

	jibb->hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	jibb->bStreamClosed = FALSE;
	jibb->wPacketId = 0;
	jibb->dwTransferredSize = 0;
	jibb->state = JIBB_RECVING;

	WaitForSingleObject( jibb->hEvent, INFINITE );

	CloseHandle( jibb->hEvent );
	jibb->hEvent = NULL;

	if ( jibb->state == JIBB_ERROR ) {
		int iqId = SerialNext();

		XmlNodeIq iq2( "set", iqId, jibb->dstJID );
		HXML closeNode = xmlAddChild( iq2, "close" );
		xmlAddAttr( closeNode, "sid", jibb->sid );
		xmlAddAttr( closeNode, "xmlns", JABBER_FEAT_IBB );

		m_ThreadInfo->send( iq2 );
	}

	if ( jibb->bStreamClosed && jibb->dwTransferredSize == ft->dwExpectedRecvFileSize )
		jibb->state = JIBB_DONE;

	(this->*jibb->pfnFinal)(( jibb->state==JIBB_DONE )?TRUE:FALSE, jibb->userdata );
	jibb->userdata = NULL;

	ListRemove( LIST_FTRECV, jibb->sid );

	JabberIbbFreeJibb( jibb );
}

BOOL CJabberProto::OnIbbRecvdData( const TCHAR *data, const TCHAR *sid, const TCHAR *seq )
{
	JABBER_LIST_ITEM *item = ListGetItemPtr( LIST_FTRECV, sid );
	if ( !item ) return FALSE;

	WORD wSeq = (WORD)_ttoi(seq);
	if ( wSeq != item->jibb->wPacketId ) {
		if ( item->jibb->hEvent )
			SetEvent( item->jibb->hEvent );
		return FALSE;
	}

	item->jibb->wPacketId++;

	int length = 0;
	char *decodedData = JabberBase64Decode( data, &length );
	if ( !decodedData )
		return FALSE;

	(this->*item->jibb->pfnRecv)( NULL, item->ft, decodedData, length );

	item->jibb->dwTransferredSize += (DWORD)length;

	mir_free( decodedData );

	return TRUE;
}
