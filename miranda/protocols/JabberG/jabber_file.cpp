/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005     George Hazan

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

*/

#include "jabber.h"
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

static char* fileBaseName;
static char* filePathName;

static int JabberFileReceiveParse( JABBER_FILE_TRANSFER *ft, char* buffer, int datalen );
static void JabberFileServerConnection( HANDLE hNewConnection, DWORD dwRemoteIP );
static int JabberFileSendParse( JABBER_SOCKET s, JABBER_FILE_TRANSFER *ft, char* buffer, int datalen );

void JabberFileFreeFt( JABBER_FILE_TRANSFER *ft )
{
	int i;

	if ( ft->jid ) free( ft->jid );
	if ( ft->iqId ) free( ft->iqId );
	if ( ft->httpHostName ) free( ft->httpHostName );
	if ( ft->httpPath ) free( ft->httpPath );
	if ( ft->szSavePath ) free( ft->szSavePath );
	if ( ft->szDescription ) free( ft->szDescription );
	if ( ft->fullFileName ) free( ft->fullFileName );
	if ( ft->files ) {
		for ( i=0; i<ft->fileCount; i++ ) {
			if ( ft->files[i] ) free( ft->files[i] );
		}
		free( ft->files );
	}
	if ( ft->fileSize ) free( ft->fileSize );
	if ( ft->fileName ) free( ft->fileName );
	if ( ft->sid ) free( ft->sid );
	free( ft );
}

#define JABBER_NETWORK_BUFFER_SIZE 2048

void __cdecl JabberFileReceiveThread( JABBER_FILE_TRANSFER *ft )
{
	char* buffer;
	int datalen;
	JABBER_SOCKET s;

	JabberLog( "Thread started: type=file_receive server='%s' port='%d'", ft->httpHostName, ft->httpPort );

	ft->type = FT_OOB;

	if (( buffer=( char* )malloc( JABBER_NETWORK_BUFFER_SIZE )) == NULL ) {
		JabberLog( "Cannot allocate network buffer, thread ended" );
		ProtoBroadcastAck( jabberProtoName, ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0 );
		JabberFileFreeFt( ft );
		return;
	}

	NETLIBOPENCONNECTION nloc = { 0 };
	nloc.cbSize = sizeof( nloc );
	nloc.cbSize = sizeof( NETLIBOPENCONNECTION );
	nloc.szHost = ft->httpHostName;
	nloc.wPort = ft->httpPort;
	s = ( HANDLE ) JCallService( MS_NETLIB_OPENCONNECTION, ( WPARAM ) hNetlibUser, ( LPARAM )&nloc );
	if ( s == NULL ) {
		JabberLog( "Connection failed ( %d ), thread ended", WSAGetLastError());
		ProtoBroadcastAck( jabberProtoName, ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0 );
		free( buffer );
		JabberFileFreeFt( ft );
		return;
	}

	ft->s = s;

	JabberSend( s, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", ft->httpPath, ft->httpHostName );
	ft->state = FT_CONNECTING;

	JabberLog( "Entering file_receive recv loop" );
	datalen = 0;

	while ( ft->state!=FT_DONE && ft->state!=FT_ERROR ) {
		int recvResult, bytesParsed;

		JabberLog( "Waiting for data..." );
		recvResult = Netlib_Recv( s, buffer+datalen, JABBER_NETWORK_BUFFER_SIZE-datalen, MSG_NODUMP );
		if ( recvResult <= 0 )
			break;
		datalen += recvResult;

		bytesParsed = JabberFileReceiveParse( ft, buffer, datalen );
		if ( bytesParsed < datalen )
			memmove( buffer, buffer+bytesParsed, datalen-bytesParsed );
		datalen -= bytesParsed;
	}

	if ( ft->s )
		Netlib_CloseHandle( s );
	ft->s = NULL;

	if ( ft->state==FT_RECEIVING || ft->state==FT_DONE )
		_close( ft->fileId );

	if ( ft->state==FT_DONE || ( ft->state==FT_RECEIVING && ft->fileTotalSize<0 ))
		ProtoBroadcastAck( jabberProtoName, ft->hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, ft, 0 );
	else
		ProtoBroadcastAck( jabberProtoName, ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0 );

	JabberLog( "Thread ended: type=file_receive server='%s'", ft->httpHostName );

	free( buffer );
	JabberFileFreeFt( ft );
}

static int JabberFileReceiveParse( JABBER_FILE_TRANSFER *ft, char* buffer, int datalen )
{
	char* p, *q, *s, *eob;
	char* str;
	int num, code;

	eob = buffer + datalen;
	p = buffer;
	num = 0;
	for ( ;; ) {
		if ( ft->state==FT_CONNECTING || ft->state==FT_INITIALIZING ) {
			for ( q=p; q+1<eob && ( *q!='\r' || *( q+1 )!='\n' ); q++ );
			if ( q+1 < eob ) {
				if (( str=( char* )malloc( q-p+1 )) != NULL ) {
					strncpy( str, p, q-p );
					str[q-p] = '\0';
					JabberLog( "FT Got: %s", str );
					if ( ft->state == FT_CONNECTING ) {
						// looking for "HTTP/1.1 200 OK"
						if ( sscanf( str, "HTTP/%*d.%*d %d %*s", &code )==1 && code==200 ) {
							ft->state = FT_INITIALIZING;
							ft->fileTotalSize = -1;
							JabberLog( "Change to FT_INITIALIZING" );
							ProtoBroadcastAck( jabberProtoName, ft->hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0 );
						}
					}
					else {	// FT_INITIALIZING
						if ( str[0] == '\0' ) {
							ft->fullFileName = ( char* )malloc( strlen( ft->szSavePath ) + strlen( ft->httpPath ) + 2 );
							strcpy( ft->fullFileName, ft->szSavePath );
							if ( ft->fullFileName[strlen( ft->fullFileName )-1] != '\\' )
								strcat( ft->fullFileName, "\\" );
							if (( s=strrchr( ft->httpPath, '/' )) != NULL )
								s++;
							else
								s = ft->httpPath;
							s = _strdup( s );
							JabberHttpUrlDecode( s );
							strcat( ft->fullFileName, s );
							free( s );
							JabberLog( "Saving to [%s]", ft->fullFileName );
							ft->fileId = _open( ft->fullFileName, _O_BINARY|_O_WRONLY|_O_CREAT|_O_TRUNC, _S_IREAD|_S_IWRITE );
							if ( ft->fileId < 0 ) {
								ft->state = FT_ERROR;
								break;
							}
							ft->state = FT_RECEIVING;
							ft->fileReceivedBytes = 0;
							JabberLog( "Change to FT_RECEIVING" );
						}
						else if (( s=strchr( str, ':' )) != NULL ) {
							*s = '\0';
							if ( !strcmp( str, "Content-Length" )) {
								ft->fileTotalSize = strtol( s+1, NULL, 10 );
							}
						}
					}
					free( str );
					q += 2;
					num += ( q-p );
					p = q;
				}
				else {
					ft->state = FT_ERROR;
					break;
				}
			}
			else {
				break;
			}
		}
		else if ( ft->state == FT_RECEIVING ) {
			int bufferSize, remainingBytes, writeSize;
			PROTOFILETRANSFERSTATUS pfts;

			if ( ft->fileTotalSize<0 || ft->fileReceivedBytes<ft->fileTotalSize ) {
				bufferSize = eob - p;
				remainingBytes = ft->fileTotalSize - ft->fileReceivedBytes;
				if ( remainingBytes < bufferSize )
					writeSize = remainingBytes;
				else
					writeSize = bufferSize;
				if ( _write( ft->fileId, p, writeSize ) != writeSize ) {
					JabberLog( "_write() error" );
					ft->state = FT_ERROR;
				}
				else {
					ft->fileReceivedBytes += writeSize;
					memset( &pfts, 0, sizeof( PROTOFILETRANSFERSTATUS ));
					pfts.cbSize = sizeof( PROTOFILETRANSFERSTATUS );
					pfts.hContact = ft->hContact;
					pfts.sending = FALSE;
					pfts.files = NULL;
					pfts.totalFiles = 1;
					pfts.currentFileNumber = 0;
					pfts.totalBytes = ft->fileTotalSize;
					pfts.totalProgress = ft->fileReceivedBytes;
					pfts.workingDir = ft->szSavePath;
					pfts.currentFile = ft->fullFileName;
					pfts.currentFileSize = ft->fileTotalSize;
					pfts.currentFileProgress = ft->fileReceivedBytes;
					pfts.currentFileTime = 0;
					ProtoBroadcastAck( jabberProtoName, ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&pfts );
					if ( ft->fileReceivedBytes == ft->fileTotalSize )
						ft->state = FT_DONE;
				}
			}
			num = datalen;
			break;
		}
		else {
			break;
		}
	}
	
	return num;
}

void __cdecl JabberFileServerThread( JABBER_FILE_TRANSFER *ft )
{
	NETLIBBIND nlb = {0};
	JABBER_SOCKET s;
	int i;
	JABBER_LIST_ITEM *item;
	struct in_addr in;
	HANDLE hEvent;
	char* p, *myAddr;
	char* resource;
	char* pFileName, *pDescription;
	DBVARIANT dbv;
	char szPort[20];
	int id;

	JabberLog( "Thread started: type=file_send" );

	ft->type = FT_OOB;

	nlb.cbSize = sizeof( NETLIBBIND );
	nlb.pfnNewConnection = JabberFileServerConnection;
	nlb.wPort = 0;	// Use user-specified incoming port ranges, if available
	s = ( HANDLE ) JCallService( MS_NETLIB_BINDPORT, ( WPARAM ) hNetlibUser, ( LPARAM )&nlb );
	if ( s == NULL ) {
		JabberLog( "Cannot allocate port to bind for file server thread, thread ended." );
		ProtoBroadcastAck( jabberProtoName, ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0 );
		JabberFileFreeFt( ft );
		return;
	}

	ft->s = s;
	JabberLog( "ft->s = %d", s );

	hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	ft->hFileEvent = hEvent;

	_snprintf( szPort, sizeof( szPort ), "%d", nlb.wPort );
	item = JabberListAdd( LIST_FILE, szPort );
	item->ft = ft;

	if (( p=JabberListGetBestClientResourceNamePtr( ft->jid )) == NULL )
		resource = NULL;
	else
		resource = _strdup( p );

	if ( resource != NULL ) {
		ft->state = FT_CONNECTING;
		for ( i=0; i<ft->fileCount && ft->state!=FT_ERROR && ft->state!=FT_DENIED; i++ ) {
			ft->currentFile = i;
			ft->state = FT_CONNECTING;
			if ( ft->httpPath ) free( ft->httpPath );
			ft->httpPath = NULL;
			if (( p=strrchr( ft->files[i], '\\' )) != NULL )
				p++;
			else
				p = ft->files[i];
			in.S_un.S_addr = jabberLocalIP;
			if (( pFileName=JabberHttpUrlEncode( p )) != NULL ) {
				id = JabberSerialNext();
				if ( ft->iqId ) free( ft->iqId );
				ft->iqId = ( char* )malloc( strlen( JABBER_IQID )+20 );
				sprintf( ft->iqId, JABBER_IQID"%d", id );
				if (( pDescription=JabberTextEncode( ft->szDescription )) != NULL ) {
					if ( JGetByte( "BsDirect", TRUE ) && JGetByte( "BsDirectManual", FALSE )) {
						if ( !DBGetContactSetting( NULL, jabberProtoName, "BsDirectAddr", &dbv )) {
							myAddr = JabberTextEncode( dbv.pszVal );
							JFreeVariant( &dbv );
						}
						else myAddr = _strdup( inet_ntoa( in ));
					}
					else myAddr = _strdup( inet_ntoa( in ));
					JabberSend( jabberThreadInfo->s, "<iq type='set' to='%s/%s' id='"JABBER_IQID"%d'><query xmlns='jabber:iq:oob'><url>http://%s:%d/%s</url><desc>%s</desc></query></iq>", ft->jid, resource, id, myAddr, nlb.wPort, pFileName, pDescription );
					free( myAddr );
					free( pDescription );
				}
				else {
					id = JabberSerialNext();
					JabberSend( jabberThreadInfo->s, "<iq type='set' to='%s/%s' id='"JABBER_IQID"%d'><query xmlns='jabber:iq:oob'><url>http://%s:%d/%s</url><desc/></query></iq>", ft->jid, resource, id, inet_ntoa( in ), nlb.wPort, pFileName );
				}
				JabberLog( "Waiting for the file to be sent..." );
				WaitForSingleObject( hEvent, INFINITE );
				free( pFileName );
			}
			JabberLog( "File sent, advancing to the next file..." );
			ProtoBroadcastAck( jabberProtoName, ft->hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, ft, 0 );
		}
		CloseHandle( hEvent );
		ft->hFileEvent = NULL;
		JabberLog( "Finish all files" );

		Netlib_CloseHandle( s );

		free( resource );
	}

	ft->s = NULL;
	JabberLog( "ft->s is NULL" );

	JabberListRemove( LIST_FILE, szPort );

	switch ( ft->state ) {
	case FT_DONE:
		JabberLog( "Finish successfully" );
		ProtoBroadcastAck( jabberProtoName, ft->hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, ft, 0 );
		break;
	case FT_DENIED:
		ProtoBroadcastAck( jabberProtoName, ft->hContact, ACKTYPE_FILE, ACKRESULT_DENIED, ft, 0 );
		break;
	default: // FT_ERROR:
		JabberLog( "Finish with errors" );
		ProtoBroadcastAck( jabberProtoName, ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0 );
		break;
	}

	JabberLog( "Thread ended: type=file_send" );

	JabberFileFreeFt( ft );
}

static void JabberFileServerConnection( JABBER_SOCKET hConnection, DWORD dwRemoteIP )
{
	SOCKET s;
	JABBER_SOCKET slisten;
	SOCKADDR_IN saddr;
	int len;
	WORD localPort;
	JABBER_LIST_ITEM *item;
	char* buffer;
	int datalen;
	JABBER_FILE_TRANSFER *ft;
	char szPort[20];

	localPort = 0;
	if (( s=JCallService( MS_NETLIB_GETSOCKET, ( WPARAM ) hConnection, 0 )) != INVALID_SOCKET ) {
		len = sizeof( saddr );
		if ( getsockname( s, ( SOCKADDR * ) &saddr, &len ) != SOCKET_ERROR ) {
			localPort = ntohs( saddr.sin_port );
		}
	}
	if ( localPort == 0 ) {
		JabberLog( "Unable to determine the local port, file server connection closed." );
		Netlib_CloseHandle( hConnection );
		return;
	}

	_snprintf( szPort, sizeof( szPort ), "%d", localPort );
	JabberLog( "File server incoming connection accepted: local_port=%s", szPort );

	if (( item=JabberListGetItemPtr( LIST_FILE, szPort )) == NULL ) {
		JabberLog( "No file is currently served, file server connection closed." );
		Netlib_CloseHandle( hConnection );
		return;
	}

	ft = item->ft;
	slisten = ft->s;
	ft->s = hConnection;
	JabberLog( "Set ft->s to %d ( saving %d )", hConnection, slisten );

	if (( buffer=( char* )malloc( JABBER_NETWORK_BUFFER_SIZE+1 )) == NULL ) {
		JabberLog( "Cannot allocate network buffer, file server connection closed." );
		Netlib_CloseHandle( hConnection );
		ft->state = FT_ERROR;
		if ( ft->hFileEvent != NULL )
			SetEvent( ft->hFileEvent );
		return;
	}
	JabberLog( "Entering recv loop for this file connection... ( ft->s is hConnection )" );
	datalen = 0;
	while ( ft->state!=FT_DONE && ft->state!=FT_ERROR ) {
		int recvResult, bytesParsed;

		//recvResult = JabberWsRecv( hConnection, buffer+datalen, JABBER_NETWORK_BUFFER_SIZE-datalen );
		recvResult = Netlib_Recv( hConnection, buffer+datalen, JABBER_NETWORK_BUFFER_SIZE-datalen, MSG_NODUMP );
		if ( recvResult <= 0 )
			break;
		datalen += recvResult;

		buffer[datalen] = '\0';
		JabberLog( "RECV:%s", buffer );

		bytesParsed = JabberFileSendParse( hConnection, ft, buffer, datalen );
		if ( bytesParsed < datalen )
			memmove( buffer, buffer+bytesParsed, datalen-bytesParsed );
		datalen -= bytesParsed;
	}

	JabberLog( "Closing connection for this file transfer... ( ft->s is now hBind )" );
	Netlib_CloseHandle( hConnection );
	ft->s = slisten;
	JabberLog( "ft->s is restored to %d", ft->s );
	if ( ft->hFileEvent != NULL )
		SetEvent( ft->hFileEvent );
	free( buffer );
}

static int JabberFileSendParse( JABBER_SOCKET s, JABBER_FILE_TRANSFER *ft, char* buffer, int datalen )
{
	char* p, *q, *t, *eob;
	char* str;
	int num;
	int currentFile;
	int fileId;
	int numRead;
	char* fileBuffer;

	eob = buffer + datalen;
	p = buffer;
	num = 0;
	while ( ft->state==FT_CONNECTING || ft->state==FT_INITIALIZING ) {
		for ( q=p; q+1<eob && ( *q!='\r' || *( q+1 )!='\n' ); q++ );
		if ( q+1 >= eob )
			break;
		if (( str=( char* )malloc( q-p+1 )) == NULL ) {
			ft->state = FT_ERROR;
			break;
		}
		strncpy( str, p, q-p );
		str[q-p] = '\0';
		JabberLog( "FT Got: %s", str );
		if ( ft->state == FT_CONNECTING ) {
			// looking for "GET filename.ext HTTP/1.1"
			if ( !strncmp( str, "GET ", 4 )) {
				for ( t=str+4; *t!='\0' && *t!=' '; t++ );
				*t = '\0';
				for ( t=str+4; *t!='\0' && *t=='/'; t++ );
				ft->httpPath = _strdup( t );
				JabberHttpUrlDecode( ft->httpPath );
				ft->state = FT_INITIALIZING;
				JabberLog( "Change to FT_INITIALIZING" );
			}
		}
		else {	// FT_INITIALIZING
			if ( str[0] == '\0' ) {
				PROTOFILETRANSFERSTATUS pfts;
				struct _stat statbuf;

				free( str );
				num += 2;

				currentFile = ft->currentFile;
				if (( t=strrchr( ft->files[currentFile], '\\' )) != NULL )
					t++;
				else
					t = ft->files[currentFile];
				if ( ft->httpPath==NULL || strcmp( ft->httpPath, t )) {
					if ( ft->httpPath == NULL )
						JabberLog( "Requested file name does not matched ( httpPath==NULL )" );
					else
						JabberLog( "Requested file name does not matched ( '%s' vs. '%s' )", ft->httpPath, t );
					ft->state = FT_ERROR;
					break;
				}
				JabberLog( "Sending [%s]", ft->files[currentFile] );
				_stat( ft->files[currentFile], &statbuf );	// file size in statbuf.st_size
				if (( fileId=_open( ft->files[currentFile], _O_BINARY|_O_RDONLY )) < 0 ) {
					JabberLog( "File cannot be opened" );
					ft->state = FT_ERROR;
					free( ft->httpPath );
					ft->httpPath = NULL;
					break;
				}
				JabberSend( s, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", statbuf.st_size );
				memset( &pfts, 0, sizeof( PROTOFILETRANSFERSTATUS ));
				pfts.cbSize = sizeof( PROTOFILETRANSFERSTATUS );
				pfts.hContact = ft->hContact;
				pfts.sending = TRUE;
				pfts.files = ft->files;
				pfts.totalFiles = ft->fileCount;
				pfts.currentFileNumber = ft->currentFile;
				pfts.totalBytes = ft->allFileTotalSize;
				pfts.workingDir = NULL;
				pfts.currentFile = ft->files[ft->currentFile];
				pfts.currentFileSize = statbuf.st_size;
				pfts.currentFileTime = 0;
				ft->fileReceivedBytes = 0;
				fileBuffer = ( char* )malloc( 2048 );
				JabberLog( "Sending file data..." );
				while (( numRead=_read( fileId, fileBuffer, 2048 )) > 0 ) {
					if ( Netlib_Send( s, fileBuffer, numRead, MSG_NODUMP ) != numRead ) {
						ft->state = FT_ERROR;
						break;
					}
					ft->fileReceivedBytes += numRead;
					ft->allFileReceivedBytes += numRead;
					pfts.totalProgress = ft->allFileReceivedBytes;
					pfts.currentFileProgress = ft->fileReceivedBytes;
					ProtoBroadcastAck( jabberProtoName, ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&pfts );
				}
				free( fileBuffer );
				_close( fileId );
				if ( ft->state != FT_ERROR )
					ft->state = FT_DONE;
				JabberLog( "Finishing this file..." );
				free( ft->httpPath );
				ft->httpPath = NULL;
				break;
			}
		}
		free( str );
		q += 2;
		num += ( q-p );
		p = q;
	}
	
	return num;
}
