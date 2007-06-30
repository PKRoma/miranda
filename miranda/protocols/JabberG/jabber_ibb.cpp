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

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_ibb.cpp,v $
Revision       : $Revision: 5337 $
Last change on : $Date: 2007-04-28 13:26:31 +0300 (��, 28 ��� 2007) $
Last change by : $Author: ghazan $

*/

#include "jabber.h"
#include "jabber_iq.h"
#include "jabber_ibb.h"
#include "jabber_caps.h"

#define JABBER_IBB_BLOCK_SIZE 2048

void JabberIbbFreeJibb( JABBER_IBB_TRANSFER *jibb )
{
	if ( jibb )  {
		mir_free( jibb->srcJID );
		mir_free( jibb->dstJID );
		mir_free( jibb->sid );

		mir_free( jibb );
}	}

static void JabberIbbInitiateResult( XmlNode *iqNode, void *userdata, CJabberIqRequestInfo *pInfo )
{
	JABBER_IBB_TRANSFER *jibb = ( JABBER_IBB_TRANSFER * )pInfo->GetUserData();
	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT )
		jibb->bStreamInitialized = TRUE;
	if ( jibb->hEvent )
		SetEvent( jibb->hEvent );
}

static void JabberIbbCloseResult( XmlNode *iqNode, void *userdata, CJabberIqRequestInfo *pInfo )
{
	JABBER_IBB_TRANSFER *jibb = ( JABBER_IBB_TRANSFER * )pInfo->GetUserData();
	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT )
		jibb->bStreamClosed = TRUE;
	if ( jibb->hEvent )
		SetEvent( jibb->hEvent );
}

void __cdecl JabberIbbSendThread( JABBER_IBB_TRANSFER *jibb )
{
	JabberLog( "Thread started: type=ibb_send" );
	
	XmlNodeIq iq( g_JabberIqRequestManager.AddHandler( JabberIbbInitiateResult, JABBER_IQ_TYPE_SET, jibb->dstJID, 0, -1, jibb ));
	XmlNode* openNode = iq.addChild( "open" );
	openNode->addAttr( "sid", jibb->sid );
	openNode->addAttr( "block-size", JABBER_IBB_BLOCK_SIZE );
	openNode->addAttr( "xmlns", JABBER_FEAT_IBB );

	jibb->hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	jibb->bStreamInitialized = FALSE;
	jibb->bStreamClosed = FALSE;
	jibb->state = JIBB_SENDING;

	jabberThreadInfo->send( iq );

	WaitForSingleObject( jibb->hEvent, INFINITE );
	CloseHandle( jibb->hEvent );
	jibb->hEvent = NULL;

	if ( jibb->bStreamInitialized ) {

		jibb->wPacketId = 0;

		BOOL bSent = jibb->pfnSend( JABBER_IBB_BLOCK_SIZE, jibb->userdata );

		if ( !jibb->bStreamClosed )
		{
			XmlNodeIq iq2( g_JabberIqRequestManager.AddHandler( JabberIbbCloseResult, JABBER_IQ_TYPE_SET, jibb->dstJID, 0, -1, jibb ));
			XmlNode* closeNode = iq2.addChild( "close" );
			closeNode->addAttr( "sid", jibb->sid );
			closeNode->addAttr( "xmlns", JABBER_FEAT_IBB );

			jibb->hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

			jabberThreadInfo->send( iq2 );

			WaitForSingleObject( jibb->hEvent, INFINITE );
			CloseHandle( jibb->hEvent );
			jibb->hEvent = NULL;

			if ( jibb->bStreamClosed && bSent )
				jibb->state = JIBB_DONE;

		} else {
			jibb->state = JIBB_ERROR;
		}
	}

	jibb->pfnFinal(( jibb->state==JIBB_DONE )?TRUE:FALSE, jibb->userdata );
	JabberIbbFreeJibb( jibb );
}

void __cdecl JabberIbbReceiveThread( JABBER_IBB_TRANSFER *jibb )
{
	JabberLog( "Thread started: type=ibb_recv" );

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
		int iqId = JabberSerialNext();

		XmlNodeIq iq2( "set", iqId, jibb->dstJID );
		XmlNode* closeNode = iq2.addChild( "close" );
		closeNode->addAttr( "sid", jibb->sid );
		closeNode->addAttr( "xmlns", JABBER_FEAT_IBB );

		jabberThreadInfo->send( iq2 );
	}

	if ( jibb->bStreamClosed && jibb->dwTransferredSize == ft->dwExpectedRecvFileSize )
		jibb->state = JIBB_DONE;

	jibb->pfnFinal(( jibb->state==JIBB_DONE )?TRUE:FALSE, jibb->userdata );

	JabberListRemove( LIST_FTRECV, jibb->sid );

	JabberIbbFreeJibb( jibb );
}

BOOL JabberIbbProcessRecvdData( TCHAR *data, TCHAR *sid, TCHAR *seq )
{
	JABBER_LIST_ITEM *item = JabberListGetItemPtr( LIST_FTRECV, sid );
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

	item->jibb->pfnRecv( NULL, item->ft, decodedData, length );

	item->jibb->dwTransferredSize += (DWORD)length;

	mir_free( decodedData );

	return TRUE;
}
