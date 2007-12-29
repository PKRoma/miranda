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

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "jabber_iq.h"
#include "jabber_byte.h"
#include "jabber_ibb.h"
#include "jabber_caps.h"

void JabberFtCancel( filetransfer* ft )
{
	JABBER_LIST_ITEM *item;
	JABBER_BYTE_TRANSFER *jbt;
	JABBER_IBB_TRANSFER *jibb;
	int i;

	JabberLog( "Invoking JabberFtCancel()" );

	// For file sending session that is still in si negotiation phase
	if ( g_JabberIqManager.ExpireByUserData( ft ))
		return;
	// For file receiving session that is still in si negotiation phase
	for ( i=0; ( i=JabberListFindNext( LIST_FTRECV, i ))>=0; i++ ) {
		item = JabberListGetItemPtrFromIndex( i );
		if ( item->ft == ft ) {
			JabberLog( "Canceling file receiving session while in si negotiation" );
			JabberListRemoveByIndex( i );
			JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0 );
			delete ft;
			return;
		}
	}
	// For file transfer through bytestream
	if (( jbt=ft->jbt ) != NULL ) {
		JabberLog( "Canceling bytestream session" );
		jbt->state = JBT_ERROR;
		if ( jbt->hConn ) {
			JabberLog( "Force closing bytestream session" );
			Netlib_CloseHandle( jbt->hConn );
			jbt->hConn = NULL;
		}
		if ( jbt->hSendEvent ) SetEvent( jbt->hSendEvent );
		if ( jbt->hEvent ) SetEvent( jbt->hEvent );
		if ( jbt->hProxyEvent ) SetEvent( jbt->hProxyEvent );
	}
	// For file transfer through IBB
	if (( jibb=ft->jibb ) != NULL ) {
		JabberLog( "Canceling IBB session" );
		jibb->state = JIBB_ERROR;
		g_JabberIqManager.ExpireByUserData( jibb );
	}
}

///////////////// File sending using stream initiation /////////////////////////

static void JabberFtSiResult( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
static BOOL JabberFtSend( HANDLE hConn, void *userdata );
static BOOL JabberFtIbbSend( int blocksize, void *userdata );
static void JabberFtSendFinal( BOOL success, void *userdata );

void JabberFtInitiate( TCHAR* jid, filetransfer* ft )
{
	TCHAR *rs;
	char *filename, *p;
	int i;
	TCHAR sid[9];
	XmlNode* option = NULL;

	if ( jid==NULL || ft==NULL || !jabberOnline || ( rs=JabberListGetBestClientResourceNamePtr( jid ))==NULL ) {
		JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0 );
		delete ft;
		return;
	}
	ft->type = FT_SI;
	for ( i=0; i<8; i++ )
		sid[i] = ( rand()%10 ) + '0';
	sid[8] = '\0';
	if ( ft->sid != NULL ) mir_free( ft->sid );
	ft->sid = mir_tstrdup( sid );
	filename = ft->std.files[ ft->std.currentFileNumber ];
	if (( p=strrchr( filename, '\\' )) != NULL )
		filename = p+1;

	TCHAR tszJid[ 512 ];
	mir_sntprintf( tszJid, SIZEOF(tszJid), _T("%s/%s"), jid, rs );

	XmlNodeIq iq( g_JabberIqManager.AddHandler(JabberFtSiResult, JABBER_IQ_TYPE_SET, tszJid, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_TO, -1, ft ));
	XmlNode* si = iq.addChild( "si" ); si->addAttr( "xmlns", JABBER_FEAT_SI ); 
	si->addAttr( "id", sid ); si->addAttr( "mime-type", "binary/octet-stream" );
	si->addAttr( "profile", JABBER_FEAT_SI_FT );
	XmlNode* file = si->addChild( "file" ); file->addAttr( "name", filename ); file->addAttr( "size", ft->fileSize[ ft->std.currentFileNumber ] );
	file->addAttr( "xmlns", JABBER_FEAT_SI_FT );
	file->addChild( "desc", ft->szDescription );
	XmlNode* feature = si->addChild( "feature" ); feature->addAttr( "xmlns", JABBER_FEAT_FEATURE_NEG );
	XmlNode* x = feature->addChild( "x" ); x->addAttr( "xmlns", JABBER_FEAT_DATA_FORMS ); x->addAttr( "type", "form" );
	XmlNode* field = x->addChild( "field" ); field->addAttr( "var", "stream-method" ); field->addAttr( "type", "list-single" );

	BOOL bDirect = JGetByte( "BsDirect", TRUE );
	BOOL bProxy = JGetByte( "BsProxyManual", TRUE );
	
	// bytestreams support?
	if ( bDirect || bProxy ) {
		option = field->addChild( "option" ); option->addChild( "value", JABBER_FEAT_BYTESTREAMS );
	}
	option = field->addChild( "option" ); option->addChild( "value", JABBER_FEAT_IBB );
	jabberThreadInfo->send( iq );
}

static void JabberFtSiResult( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo )
{
	XmlNode *siNode, *featureNode, *xNode, *fieldNode, *valueNode;
	filetransfer *ft = (filetransfer *)pInfo->GetUserData();
	if ( !ft ) return;

	if (( pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT ) && pInfo->m_szFrom && pInfo->m_szTo ) {
		if (( siNode=JabberXmlGetChild( iqNode, "si" )) != NULL ) {

			// fix for very smart clients, like gajim
			BOOL bDirect = JGetByte( "BsDirect", FALSE );
			BOOL bProxy = JGetByte( "BsProxyManual", FALSE );

			if (( featureNode=JabberXmlGetChild( siNode, "feature" )) != NULL ) {
				if (( xNode=JabberXmlGetChildWithGivenAttrValue( featureNode, "x", "xmlns", _T(JABBER_FEAT_DATA_FORMS))) != NULL ) {
					if (( fieldNode=JabberXmlGetChildWithGivenAttrValue( xNode, "field", "var", _T("stream-method"))) != NULL ) {
						if (( valueNode=JabberXmlGetChild( fieldNode, "value" ))!=NULL && valueNode->text!=NULL ) {
							if (( bDirect || bProxy ) && !_tcscmp( valueNode->text, _T(JABBER_FEAT_BYTESTREAMS))) {
								// Start Bytestream session
								JABBER_BYTE_TRANSFER *jbt = ( JABBER_BYTE_TRANSFER * ) mir_alloc( sizeof( JABBER_BYTE_TRANSFER ));
								ZeroMemory( jbt, sizeof( JABBER_BYTE_TRANSFER ));
								jbt->srcJID = mir_tstrdup( pInfo->m_szTo );
								jbt->dstJID = mir_tstrdup( pInfo->m_szFrom );
								jbt->sid = mir_tstrdup( ft->sid );
								jbt->pfnSend = JabberFtSend;
								jbt->pfnFinal = JabberFtSendFinal;
								jbt->userdata = ft;
								ft->type = FT_BYTESTREAM;
								ft->jbt = jbt;
								mir_forkthread(( pThreadFunc )JabberByteSendThread, jbt );
							} else if ( !_tcscmp( valueNode->text, _T(JABBER_FEAT_IBB))) {
								JABBER_IBB_TRANSFER *jibb = (JABBER_IBB_TRANSFER *) mir_alloc( sizeof ( JABBER_IBB_TRANSFER ));
								ZeroMemory( jibb, sizeof( JABBER_IBB_TRANSFER ));
								jibb->srcJID = mir_tstrdup( pInfo->m_szTo );
								jibb->dstJID = mir_tstrdup( pInfo->m_szFrom );
								jibb->sid = mir_tstrdup( ft->sid );
								jibb->pfnSend = JabberFtIbbSend;
								jibb->pfnFinal = JabberFtSendFinal;
								jibb->userdata = ft;
								ft->type = FT_IBB;
								ft->jibb = jibb;
								mir_forkthread(( pThreadFunc )JabberIbbSendThread, jibb );
	}	}	}	}	}	}	}
	else {
		JabberLog( "File transfer stream initiation request denied or failed" );
		JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, pInfo->GetIqType() == JABBER_IQ_TYPE_ERROR ? ACKRESULT_DENIED : ACKRESULT_FAILED, ft, 0 );
		delete ft;
	}
}

static BOOL JabberFtSend( HANDLE hConn, void *userdata )
{
	filetransfer* ft = ( filetransfer* ) userdata;

	struct _stat statbuf;
	int fd;
	char* buffer;
	int numRead;

	JabberLog( "Sending [%s]", ft->std.files[ ft->std.currentFileNumber ] );
	_stat( ft->std.files[ ft->std.currentFileNumber ], &statbuf );	// file size in statbuf.st_size
	if (( fd=_open( ft->std.files[ ft->std.currentFileNumber ], _O_BINARY|_O_RDONLY )) < 0 ) {
		JabberLog( "File cannot be opened" );
		return FALSE;
	}

	ft->std.sending = TRUE;
	ft->std.currentFileSize = statbuf.st_size;
	ft->std.currentFileProgress = 0;

	if (( buffer=( char* )mir_alloc( 2048 )) != NULL ) {
		while (( numRead=_read( fd, buffer, 2048 )) > 0 ) {
			if ( Netlib_Send( hConn, buffer, numRead, 0 ) != numRead ) {
				mir_free( buffer );
				_close( fd );
				return FALSE;
			}
			ft->std.currentFileProgress += numRead;
			ft->std.totalProgress += numRead;
			JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&ft->std );
		}
		mir_free( buffer );
	}
	_close( fd );
	return TRUE;
}

static BOOL JabberFtIbbSend( int blocksize, void *userdata )
{
	filetransfer* ft = ( filetransfer* ) userdata;

	struct _stat statbuf;
	int fd;
	char* buffer;
	int numRead;

	JabberLog( "Sending [%s]", ft->std.files[ ft->std.currentFileNumber ] );
	_stat( ft->std.files[ ft->std.currentFileNumber ], &statbuf );	// file size in statbuf.st_size
	if (( fd=_open( ft->std.files[ ft->std.currentFileNumber ], _O_BINARY|_O_RDONLY )) < 0 ) {
		JabberLog( "File cannot be opened" );
		return FALSE;
	}

	ft->std.sending = TRUE;
	ft->std.currentFileSize = statbuf.st_size;
	ft->std.currentFileProgress = 0;

	if (( buffer=( char* )mir_alloc( blocksize )) != NULL ) {
		while (( numRead=_read( fd, buffer, blocksize )) > 0 ) {
			int iqId = JabberSerialNext();
			XmlNode msg( "message" );
			msg.addAttr( "to", ft->jibb->dstJID );
			msg.addAttrID( iqId );

			// let others send data too
			Sleep(2);

			char *encoded = JabberBase64Encode(buffer, numRead);

			XmlNode *dataNode = msg.addChild( "data", encoded );
			dataNode->addAttr( "xmlns", JABBER_FEAT_IBB );
			dataNode->addAttr( "sid", ft->jibb->sid );
			dataNode->addAttr( "seq", ft->jibb->wPacketId );
			XmlNode *ampNode = msg.addChild( "amp" );
			ampNode->addAttr( "xmlns", JABBER_FEAT_AMP );
			XmlNode *rule = ampNode->addChild( "rule" );
			rule->addAttr( "condition", "deliver-at" );
			rule->addAttr( "value", "stored" );
			rule->addAttr( "action", "error" );
			rule = ampNode->addChild( "rule" );
			rule->addAttr( "condition", "match-resource" );
			rule->addAttr( "value", "exact" );
			rule->addAttr( "action", "error" );
			ft->jibb->wPacketId++;

			mir_free( encoded );

			if ( ft->jibb->state == JIBB_ERROR || ft->jibb->bStreamClosed || jabberThreadInfo->send( msg ) == SOCKET_ERROR ) {
				JabberLog( "JabberFtIbbSend unsuccessful exit" );
				mir_free( buffer );
				_close( fd );
				return FALSE;
			}

			ft->jibb->dwTransferredSize += (DWORD)numRead;

			ft->std.currentFileProgress += numRead;
			ft->std.totalProgress += numRead;
			JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&ft->std );
		}
		mir_free( buffer );
	}
	_close( fd );
	return TRUE;
}

static void JabberFtSendFinal( BOOL success, void *userdata )
{
	filetransfer* ft = ( filetransfer* )userdata;

	if ( !success ) {
		JabberLog( "File transfer complete with error" );
		JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0 );
	}
	else {
		if ( ft->std.currentFileNumber < ft->std.totalFiles-1 ) {
			ft->std.currentFileNumber++;
			replaceStr( ft->std.currentFile, ft->std.files[ ft->std.currentFileNumber ] );
			JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, ft, 0 );
			JabberFtInitiate( ft->jid, ft );
			return;
		}

		JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, ft, 0 );
	}

	delete ft;
}

///////////////// File receiving through stream initiation /////////////////////////

static int JabberFtReceive( HANDLE hConn, void *userdata, char* buffer, int datalen );
static void JabberFtReceiveFinal( BOOL success, void *userdata );

void JabberFtHandleSiRequest( XmlNode *iqNode )
{
	TCHAR* from, *sid, *str, *szId, *filename;
	XmlNode *siNode, *fileNode, *featureNode, *xNode, *fieldNode, *optionNode, *n;
	int filesize, i;
	JABBER_FT_TYPE ftType;

	if ( iqNode==NULL ||
		  ( from=JabberXmlGetAttrValue( iqNode, "from" ))==NULL ||
		  ( str=JabberXmlGetAttrValue( iqNode, "type" ))==NULL || _tcscmp( str, _T("set")) ||
		  ( siNode=JabberXmlGetChildWithGivenAttrValue( iqNode, "si", "xmlns", _T(JABBER_FEAT_SI))) == NULL )
		return;

	szId = JabberXmlGetAttrValue( iqNode, "id" );
	if (( sid=JabberXmlGetAttrValue( siNode, "id" ))!=NULL &&
		( fileNode=JabberXmlGetChildWithGivenAttrValue( siNode, "file", "xmlns", _T(JABBER_FEAT_SI_FT)))!=NULL &&
		( filename=JabberXmlGetAttrValue( fileNode, "name" ))!=NULL &&
		( str=JabberXmlGetAttrValue( fileNode, "size" ))!=NULL ) {

		filesize = _ttoi( str );
		if (( featureNode=JabberXmlGetChildWithGivenAttrValue( siNode, "feature", "xmlns", _T(JABBER_FEAT_FEATURE_NEG))) != NULL &&
			( xNode=JabberXmlGetChildWithGivenAttrValue( featureNode, "x", "xmlns", _T(JABBER_FEAT_DATA_FORMS)))!=NULL &&
			( fieldNode=JabberXmlGetChildWithGivenAttrValue( xNode, "field", "var", _T("stream-method")))!=NULL ) {

			BOOL bIbbOnly = JGetByte( "BsOnlyIBB", FALSE );

			if ( !bIbbOnly ) {
				for ( i=0; i<fieldNode->numChild; i++ ) {
					optionNode = fieldNode->child[i];
					if ( optionNode->name && !strcmp( optionNode->name, "option" )) {
						if (( n=JabberXmlGetChild( optionNode, "value" ))!=NULL && n->text ) {
							if ( !_tcscmp( n->text, _T(JABBER_FEAT_BYTESTREAMS))) {
								ftType = FT_BYTESTREAM;
								break;
			}	}	}	}	}

			// try IBB only if bytestreams support not found or BsOnlyIBB flag exists
			if ( bIbbOnly || (i >= fieldNode->numChild) ) {
				for ( i=0; i<fieldNode->numChild; i++ ) {
					optionNode = fieldNode->child[i];
					if ( optionNode->name && !strcmp( optionNode->name, "option" )) {
						if (( n=JabberXmlGetChild( optionNode, "value" ))!=NULL && n->text ) {
							if ( !_tcscmp( n->text, _T(JABBER_FEAT_IBB))) {
								ftType = FT_IBB;
								break;
			}	}	}	}	}

			if ( i < fieldNode->numChild ) {
				// Found known stream mechanism
				CCSDATA ccs;
				PROTORECVEVENT pre;

				char *localFilename = mir_t2a( filename );
				char *desc = (( n=JabberXmlGetChild( fileNode, "desc" ))!=NULL && n->text!=NULL ) ? mir_t2a( n->text ) : mir_strdup( "" );

				filetransfer* ft = new filetransfer;
				ft->dwExpectedRecvFileSize = (DWORD)filesize;
				ft->jid = mir_tstrdup( from );
				ft->std.hContact = JabberHContactFromJID( from );
				ft->sid = mir_tstrdup( sid );
				ft->iqId = mir_tstrdup( szId );
				ft->type = ftType;
				ft->std.totalFiles = 1;
				ft->std.currentFile = localFilename;
				ft->std.totalBytes = ft->std.currentFileSize = filesize;
				char* szBlob = ( char* )alloca( sizeof( DWORD )+ strlen( localFilename ) + strlen( desc ) + 2 );
				*(( PDWORD ) szBlob ) = ( DWORD )ft;
				strcpy( szBlob + sizeof( DWORD ), localFilename );
				strcpy( szBlob + sizeof( DWORD )+ strlen( localFilename ) + 1, desc );
				pre.flags = 0;
				pre.timestamp = time( NULL );
				pre.szMessage = szBlob;
				pre.lParam = 0;
				ccs.szProtoService = PSR_FILE;
				ccs.hContact = ft->std.hContact;
				ccs.wParam = 0;
				ccs.lParam = ( LPARAM )&pre;
				JCallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );
				mir_free( desc );
				return;
			}
			else {
				// Unknown stream mechanism
				XmlNodeIq iq( "error", szId, from );
				XmlNode* e = iq.addChild( "error" ); e->addAttr( "code", 400 ); e->addAttr( "type", "cancel" );
				XmlNode* br = e->addChild( "bad-request" ); br->addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );
				XmlNode* nvs = e->addChild( "no-valid-streams" ); nvs->addAttr( "xmlns", JABBER_FEAT_SI );
				jabberThreadInfo->send( iq );
				return;
	}	}	}

	// Bad stream initiation, reply with bad-profile
	XmlNodeIq iq( "error", szId, from );
	XmlNode* e = iq.addChild( "error" ); e->addAttr( "code", 400 ); e->addAttr( "type", "cancel" );
	XmlNode* br = e->addChild( "bad-request" ); br->addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );
	XmlNode* nvs = e->addChild( "bad-profile" ); nvs->addAttr( "xmlns", JABBER_FEAT_SI );
	jabberThreadInfo->send( iq );
}

void JabberFtAcceptSiRequest( filetransfer* ft )
{
	if ( !jabberOnline || ft==NULL || ft->jid==NULL || ft->sid==NULL ) return;

	JABBER_LIST_ITEM *item;
	if (( item=JabberListAdd( LIST_FTRECV, ft->sid )) != NULL ) {
		item->ft = ft;

		XmlNodeIq iq( "result", ft->iqId, ft->jid );
		XmlNode* si = iq.addChild( "si" ); si->addAttr( "xmlns", JABBER_FEAT_SI );
		XmlNode* f = si->addChild( "feature" ); f->addAttr( "xmlns", JABBER_FEAT_FEATURE_NEG );
		XmlNode* x = f->addChild( "x" ); x->addAttr( "xmlns", JABBER_FEAT_DATA_FORMS ); x->addAttr( "type", "submit" );
		XmlNode* fl = x->addChild( "field" ); fl->addAttr( "var", "stream-method" );
		fl->addChild( "value", JABBER_FEAT_BYTESTREAMS );
		jabberThreadInfo->send( iq );
}	}

void JabberFtAcceptIbbRequest( filetransfer* ft )
{
	if ( !jabberOnline || ft==NULL || ft->jid==NULL || ft->sid==NULL ) return;

	JABBER_LIST_ITEM *item;
	if (( item=JabberListAdd( LIST_FTRECV, ft->sid )) != NULL ) {
		item->ft = ft;

		XmlNodeIq iq( "result", ft->iqId, ft->jid );
		XmlNode* si = iq.addChild( "si" ); si->addAttr( "xmlns", JABBER_FEAT_SI );
		XmlNode* f = si->addChild( "feature" ); f->addAttr( "xmlns", JABBER_FEAT_FEATURE_NEG );
		XmlNode* x = f->addChild( "x" ); x->addAttr( "xmlns", JABBER_FEAT_DATA_FORMS ); x->addAttr( "type", "submit" );
		XmlNode* fl = x->addChild( "field" ); fl->addAttr( "var", "stream-method" );
		fl->addChild( "value", JABBER_FEAT_IBB );
		jabberThreadInfo->send( iq );
}	}

void JabberFtHandleBytestreamRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	XmlNode *queryNode = pInfo->GetChildNode();

	TCHAR* sid;
	JABBER_LIST_ITEM *item;

	if (( sid = JabberXmlGetAttrValue( queryNode, "sid" )) != NULL && ( item = JabberListGetItemPtr( LIST_FTRECV, sid )) != NULL ) {
		// Start Bytestream session
		JABBER_BYTE_TRANSFER *jbt = ( JABBER_BYTE_TRANSFER * ) mir_alloc( sizeof( JABBER_BYTE_TRANSFER ));
		ZeroMemory( jbt, sizeof( JABBER_BYTE_TRANSFER ));
		jbt->iqNode = JabberXmlCopyNode( iqNode );
		jbt->pfnRecv = JabberFtReceive;
		jbt->pfnFinal = JabberFtReceiveFinal;
		jbt->userdata = item->ft;
		item->ft->jbt = jbt;
		mir_forkthread(( pThreadFunc )JabberByteReceiveThread, jbt );
		JabberListRemove( LIST_FTRECV, sid );
		return;
	}

	JabberLog( "File transfer invalid bytestream initiation request received" );
	return;
}

BOOL JabberFtHandleIbbRequest( XmlNode *iqNode, BOOL bOpen )
{
	if ( !iqNode ) return FALSE;

	TCHAR *id = JabberXmlGetAttrValue( iqNode, "id" );
	TCHAR *from = JabberXmlGetAttrValue( iqNode, "from" );
	TCHAR *to = JabberXmlGetAttrValue( iqNode, "to" );
	if ( !id || !from || !to ) return FALSE;

	XmlNode *ibbNode = JabberXmlGetChildWithGivenAttrValue( iqNode, bOpen ? "open" : "close", "xmlns", _T(JABBER_FEAT_IBB));
	if ( !ibbNode ) return FALSE;

	TCHAR *sid = JabberXmlGetAttrValue( ibbNode, "sid" );
	if ( !sid ) return FALSE;

	// already closed?
	JABBER_LIST_ITEM *item = JabberListGetItemPtr( LIST_FTRECV, sid );
	if ( !item ) {
		XmlNodeIq iq( "error", id, from );
		XmlNode* e = iq.addChild( "error" ); e->addAttr( "code", 404 ); e->addAttr( "type", _T("cancel"));
		XmlNode* na = e->addChild( "item-not-found" ); na->addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );
		jabberThreadInfo->send( iq );
		return FALSE;
	}

	// open event
	if ( bOpen ) {
		if ( !item->jibb ) {
			JABBER_IBB_TRANSFER *jibb = ( JABBER_IBB_TRANSFER * ) mir_alloc( sizeof( JABBER_IBB_TRANSFER ));
			ZeroMemory( jibb, sizeof( JABBER_IBB_TRANSFER ));
			jibb->srcJID = mir_tstrdup( from );
			jibb->dstJID = mir_tstrdup( to );
			jibb->sid = mir_tstrdup( sid );
			jibb->pfnRecv = JabberFtReceive;
			jibb->pfnFinal = JabberFtReceiveFinal;
			jibb->userdata = item->ft;
			item->ft->jibb = jibb;
			item->jibb = jibb;
			mir_forkthread(( pThreadFunc )JabberIbbReceiveThread, jibb );
			XmlNodeIq iq( "result", id, from );
			jabberThreadInfo->send( iq );
			return TRUE;
		}
		// stream already open
		XmlNodeIq iq( "error", id, from );
		XmlNode* e = iq.addChild( "error" ); e->addAttr( "code", 404 ); e->addAttr( "type", _T("cancel"));
		XmlNode* na = e->addChild( "item-not-found" ); na->addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );
		jabberThreadInfo->send( iq );
		return FALSE;
	}
	
	// close event && stream already open
	if ( item->jibb && item->jibb->hEvent ) {
		item->jibb->bStreamClosed = TRUE;
		SetEvent( item->jibb->hEvent );

		XmlNodeIq iq( "result", id, from );
		jabberThreadInfo->send( iq );
		return TRUE;
	}

	JabberListRemove( LIST_FTRECV, sid );

	return FALSE;
}

static int JabberFtReceive( HANDLE hConn, void *userdata, char* buffer, int datalen )
{
	filetransfer* ft = ( filetransfer* )userdata;
	if ( ft->create() == -1 )
		return -1;

	int remainingBytes = ft->std.currentFileSize - ft->std.currentFileProgress;
	if ( remainingBytes > 0 ) {
		int writeSize = ( remainingBytes<datalen ) ? remainingBytes : datalen;
		if ( _write( ft->fileId, buffer, writeSize ) != writeSize ) {
			JabberLog( "_write() error" );
			return -1;
		}

		ft->std.currentFileProgress += writeSize;
		ft->std.totalProgress += writeSize;
		JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&ft->std );
		return ( ft->std.currentFileSize == ft->std.currentFileProgress ) ? 0 : writeSize;
	}

	return 0;
}

static void JabberFtReceiveFinal( BOOL success, void *userdata )
{
	filetransfer* ft = ( filetransfer* )userdata;

	if ( success ) {
		JabberLog( "File transfer complete successfully" );
		ft->complete();
	}
	else JabberLog( "File transfer complete with error" );

	delete ft;
}
