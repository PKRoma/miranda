/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan

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
#include "jabber_caps.h"

#define JABBER_NETWORK_BUFFER_SIZE 2048

void __cdecl CJabberProto::FileReceiveThread( filetransfer* ft )
{
	char* buffer;
	int datalen;
	ThreadData info( this, JABBER_SESSION_NORMAL );

	Log( "Thread started: type=file_receive server='%s' port='%d'", ft->httpHostName, ft->httpPort );

	ft->type = FT_OOB;

	if (( buffer=( char* )mir_alloc( JABBER_NETWORK_BUFFER_SIZE )) == NULL ) {
		Log( "Cannot allocate network buffer, thread ended" );
		JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0 );
		delete ft;
		return;
	}

	NETLIBOPENCONNECTION nloc = { 0 };
	nloc.cbSize = sizeof( nloc );
	nloc.cbSize = sizeof( NETLIBOPENCONNECTION );
	nloc.szHost = ft->httpHostName;
	nloc.wPort = ft->httpPort;
	info.s = ( HANDLE ) JCallService( MS_NETLIB_OPENCONNECTION, ( WPARAM ) m_hNetlibUser, ( LPARAM )&nloc );
	if ( info.s == NULL ) {
		Log( "Connection failed ( %d ), thread ended", WSAGetLastError());
		JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0 );
		mir_free( buffer );
		delete ft;
		return;
	}

	ft->s = info.s;

	info.send( "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", ft->httpPath, ft->httpHostName );
	ft->state = FT_CONNECTING;

	Log( "Entering file_receive recv loop" );
	datalen = 0;

	while ( ft->state != FT_DONE && ft->state != FT_ERROR ) {
		int recvResult, bytesParsed;

		Log( "Waiting for data..." );
		recvResult = info.recv( buffer+datalen, JABBER_NETWORK_BUFFER_SIZE-datalen );
		if ( recvResult <= 0 )
			break;
		datalen += recvResult;

		bytesParsed = FileReceiveParse( ft, buffer, datalen );
		if ( bytesParsed < datalen )
			memmove( buffer, buffer+bytesParsed, datalen-bytesParsed );
		datalen -= bytesParsed;
	}

	ft->s = NULL;

	if ( ft->state==FT_DONE || ( ft->state==FT_RECEIVING && ft->std.currentFileSize < 0 ))
		ft->complete();

	Log( "Thread ended: type=file_receive server='%s'", ft->httpHostName );

	mir_free( buffer );
	delete ft;
}

int CJabberProto::FileReceiveParse( filetransfer* ft, char* buffer, int datalen )
{
	char* p, *q, *s, *eob;
	char* str;
	int num, code;

	eob = buffer + datalen;
	p = buffer;
	num = 0;
	while ( true ) {
		if ( ft->state==FT_CONNECTING || ft->state==FT_INITIALIZING ) {
			for ( q=p; q+1<eob && ( *q!='\r' || *( q+1 )!='\n' ); q++ );
			if ( q+1 < eob ) {
				if (( str=( char* )mir_alloc( q-p+1 )) != NULL ) {
					strncpy( str, p, q-p );
					str[q-p] = '\0';
					Log( "FT Got: %s", str );
					if ( ft->state == FT_CONNECTING ) {
						// looking for "HTTP/1.1 200 OK"
						if ( sscanf( str, "HTTP/%*d.%*d %d %*s", &code )==1 && code==200 ) {
							ft->state = FT_INITIALIZING;
							ft->std.currentFileSize = -1;
							Log( "Change to FT_INITIALIZING" );
							JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0 );
						}
					}
					else {	// FT_INITIALIZING
						if ( str[0] == '\0' ) {
							if (( s=strrchr( ft->httpPath, '/' )) != NULL )
								s++;
							else
								s = ft->httpPath;
							ft->std.currentFile = mir_strdup( s );
							JabberHttpUrlDecode( ft->std.currentFile );
							if ( ft->create() == -1 ) {
								ft->state = FT_ERROR;
								break;
							}
							ft->state = FT_RECEIVING;
							ft->std.currentFileProgress = 0;
							Log( "Change to FT_RECEIVING" );
						}
						else if (( s=strchr( str, ':' )) != NULL ) {
							*s = '\0';
							if ( !strcmp( str, "Content-Length" ))
								ft->std.totalBytes = ft->std.currentFileSize = strtol( s+1, NULL, 10 );
					}	}

					mir_free( str );
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

			if ( ft->std.currentFileSize < 0 || ft->std.currentFileProgress < ft->std.currentFileSize ) {
				bufferSize = eob - p;
				remainingBytes = ft->std.currentFileSize - ft->std.currentFileProgress;
				if ( remainingBytes < bufferSize )
					writeSize = remainingBytes;
				else
					writeSize = bufferSize;
				if ( _write( ft->fileId, p, writeSize ) != writeSize ) {
					Log( "_write() error" );
					ft->state = FT_ERROR;
				}
				else {
					ft->std.currentFileProgress += writeSize;
					ft->std.totalProgress += writeSize;
					JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&ft->std );
					if ( ft->std.currentFileProgress == ft->std.currentFileSize )
						ft->state = FT_DONE;
				}
			}
			num = datalen;
			break;
		}
		else break;
	}

	return num;
}

void JabberFileServerConnection( JABBER_SOCKET hConnection, DWORD dwRemoteIP, void* extra )
{
	CJabberProto* ppro = ( CJabberProto* )extra;
	WORD localPort = 0;
	SOCKET s = JCallService( MS_NETLIB_GETSOCKET, ( WPARAM ) hConnection, 0 ); 
	if ( s != INVALID_SOCKET ) {
		SOCKADDR_IN saddr;
		int len = sizeof( saddr );
		if ( getsockname( s, ( SOCKADDR * ) &saddr, &len ) != SOCKET_ERROR ) {
			localPort = ntohs( saddr.sin_port );
		}
	}
	if ( localPort == 0 ) {
		ppro->Log( "Unable to determine the local port, file server connection closed." );
		Netlib_CloseHandle( hConnection );
		return;
	}

	TCHAR szPort[20];
	mir_sntprintf( szPort, SIZEOF( szPort ), _T("%d"), localPort );
	ppro->Log( "File server incoming connection accepted: local_port=" TCHAR_STR_PARAM, szPort );

	JABBER_LIST_ITEM *item = ppro->ListGetItemPtr( LIST_FILE, szPort );
	if ( item == NULL ) {
		ppro->Log( "No file is currently served, file server connection closed." );
		Netlib_CloseHandle( hConnection );
		return;
	}

	filetransfer* ft = item->ft;
	JABBER_SOCKET slisten = ft->s;
	ft->s = hConnection;
	ppro->Log( "Set ft->s to %d ( saving %d )", hConnection, slisten );

	char* buffer = ( char* )mir_alloc( JABBER_NETWORK_BUFFER_SIZE+1 );
	if ( buffer == NULL ) {
		ppro->Log( "Cannot allocate network buffer, file server connection closed." );
		Netlib_CloseHandle( hConnection );
		ft->state = FT_ERROR;
		if ( ft->hFileEvent != NULL )
			SetEvent( ft->hFileEvent );
		return;
	}

	ppro->Log( "Entering recv loop for this file connection... ( ft->s is hConnection )" );
	int datalen = 0;
	while ( ft->state!=FT_DONE && ft->state!=FT_ERROR ) {
		int recvResult, bytesParsed;

		recvResult = Netlib_Recv( hConnection, buffer+datalen, JABBER_NETWORK_BUFFER_SIZE-datalen, 0 );
		if ( recvResult <= 0 )
			break;
		datalen += recvResult;

		buffer[datalen] = '\0';
		ppro->Log( "RECV:%s", buffer );

		bytesParsed = ppro->FileSendParse( hConnection, ft, buffer, datalen );
		if ( bytesParsed < datalen )
			memmove( buffer, buffer+bytesParsed, datalen-bytesParsed );
		datalen -= bytesParsed;
	}

	ppro->Log( "Closing connection for this file transfer... ( ft->s is now hBind )" );
	Netlib_CloseHandle( hConnection );
	ft->s = slisten;
	ppro->Log( "ft->s is restored to %d", ft->s );
	if ( ft->hFileEvent != NULL )
		SetEvent( ft->hFileEvent );
	mir_free( buffer );
}

void __cdecl CJabberProto::FileServerThread( filetransfer* ft )
{
	Log( "Thread started: type=file_send" );

	ThreadData info( this, JABBER_SESSION_NORMAL );
	ft->type = FT_OOB;

	NETLIBBIND nlb = {0};
	nlb.cbSize = sizeof( NETLIBBIND );
	nlb.pfnNewConnectionV2 = JabberFileServerConnection;
	nlb.pExtra = this;
	nlb.wPort = 0;	// Use user-specified incoming port ranges, if available
	info.s = ( HANDLE ) JCallService( MS_NETLIB_BINDPORT, ( WPARAM ) m_hNetlibUser, ( LPARAM )&nlb );
	if ( info.s == NULL ) {
		Log( "Cannot allocate port to bind for file server thread, thread ended." );
		JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0 );
		delete ft;
		return;
	}

	ft->s = info.s;
	Log( "ft->s = %d", info.s );

	HANDLE hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	ft->hFileEvent = hEvent;

	TCHAR szPort[20];
	mir_sntprintf( szPort, SIZEOF( szPort ), _T("%d"), nlb.wPort );
	JABBER_LIST_ITEM *item = ListAdd( LIST_FILE, szPort );
	item->ft = ft;

	TCHAR* ptszResource = ListGetBestClientResourceNamePtr( ft->jid );
	if ( ptszResource != NULL ) {
		ft->state = FT_CONNECTING;
		for ( int i=0; i < ft->std.totalFiles && ft->state != FT_ERROR && ft->state != FT_DENIED; i++ ) {
			ft->std.currentFileNumber = i;
			ft->state = FT_CONNECTING;
			if ( ft->httpPath ) mir_free( ft->httpPath );
			ft->httpPath = NULL;

			char* p;
			if (( p=strrchr( ft->std.files[i], '\\' )) != NULL )
				p++;
			else
				p = ft->std.files[i];

			in_addr in;
			in.S_un.S_addr = m_dwJabberLocalIP;
		
			char* pFileName = JabberHttpUrlEncode( p );
			if ( pFileName != NULL ) {
				int id = SerialNext();
				if ( ft->iqId ) mir_free( ft->iqId );
				ft->iqId = ( TCHAR* )mir_alloc( sizeof(TCHAR)*( strlen( JABBER_IQID )+20 ));
				wsprintf( ft->iqId, _T(JABBER_IQID)_T("%d"), id );

				char *myAddr;
				DBVARIANT dbv;
				if ( JGetByte( "BsDirect", FALSE ) && JGetByte( "BsDirectManual", FALSE )) {
					if ( !DBGetContactSettingString( NULL, m_szModuleName, "BsDirectAddr", &dbv )) {
						myAddr = NEWSTR_ALLOCA( dbv.pszVal );
						JFreeVariant( &dbv );
					}
					else myAddr = inet_ntoa( in );
				}
				else myAddr = inet_ntoa( in );

				char szAddr[ 256 ];
				mir_snprintf( szAddr, sizeof(szAddr), "http://%s:%d/%s", myAddr, nlb.wPort, pFileName );

				int len = lstrlen(ptszResource) + lstrlen(ft->jid) + 2;
				TCHAR* fulljid = ( TCHAR* )alloca( sizeof( TCHAR )*len );
				wsprintf( fulljid, _T("%s/%s"), ft->jid, ptszResource );
				XmlNodeIq iq( "set", id, fulljid );
				HXML query = iq.addQuery( _T(JABBER_FEAT_OOB));
				xmlAddChild( query, "url", szAddr );
				xmlAddChild( query, "desc", ft->szDescription );
				m_ThreadInfo->send( iq );

				Log( "Waiting for the file to be sent..." );
				WaitForSingleObject( hEvent, INFINITE );
				mir_free( pFileName );
			}
			Log( "File sent, advancing to the next file..." );
			JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, ft, 0 );
		}
		CloseHandle( hEvent );
		ft->hFileEvent = NULL;
		Log( "Finish all files" );
	}

	ft->s = NULL;
	Log( "ft->s is NULL" );

	ListRemove( LIST_FILE, szPort );

	switch ( ft->state ) {
	case FT_DONE:
		Log( "Finish successfully" );
		JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, ft, 0 );
		break;
	case FT_DENIED:
		JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DENIED, ft, 0 );
		break;
	default: // FT_ERROR:
		Log( "Finish with errors" );
		JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0 );
		break;
	}

	Log( "Thread ended: type=file_send" );
	delete ft;
}

int CJabberProto::FileSendParse( JABBER_SOCKET s, filetransfer* ft, char* buffer, int datalen )
{
	char* p, *q, *t, *eob;
	char* str;
	int num;
	int currentFile;
	int fileId;
	int numRead;

	eob = buffer + datalen;
	p = buffer;
	num = 0;
	while ( ft->state==FT_CONNECTING || ft->state==FT_INITIALIZING ) {
		for ( q=p; q+1<eob && ( *q!='\r' || *( q+1 )!='\n' ); q++ );
		if ( q+1 >= eob )
			break;
		if (( str=( char* )mir_alloc( q-p+1 )) == NULL ) {
			ft->state = FT_ERROR;
			break;
		}
		strncpy( str, p, q-p );
		str[q-p] = '\0';
		Log( "FT Got: %s", str );
		if ( ft->state == FT_CONNECTING ) {
			// looking for "GET filename.ext HTTP/1.1"
			if ( !strncmp( str, "GET ", 4 )) {
				for ( t=str+4; *t!='\0' && *t!=' '; t++ );
				*t = '\0';
				for ( t=str+4; *t!='\0' && *t=='/'; t++ );
				ft->httpPath = mir_strdup( t );
				JabberHttpUrlDecode( ft->httpPath );
				ft->state = FT_INITIALIZING;
				Log( "Change to FT_INITIALIZING" );
			}
		}
		else {	// FT_INITIALIZING
			if ( str[0] == '\0' ) {
				struct _stat statbuf;

				mir_free( str );
				num += 2;

				currentFile = ft->std.currentFileNumber;
				if (( t=strrchr( ft->std.files[ currentFile ], '\\' )) != NULL )
					t++;
				else
					t = ft->std.files[currentFile];
				if ( ft->httpPath==NULL || strcmp( ft->httpPath, t )) {
					if ( ft->httpPath == NULL )
						Log( "Requested file name does not matched ( httpPath==NULL )" );
					else
						Log( "Requested file name does not matched ( '%s' vs. '%s' )", ft->httpPath, t );
					ft->state = FT_ERROR;
					break;
				}
				Log( "Sending [%s]", ft->std.files[ currentFile ] );
				_stat( ft->std.files[ currentFile ], &statbuf );	// file size in statbuf.st_size
				if (( fileId=_open( ft->std.files[currentFile], _O_BINARY|_O_RDONLY )) < 0 ) {
					Log( "File cannot be opened" );
					ft->state = FT_ERROR;
					mir_free( ft->httpPath );
					ft->httpPath = NULL;
					break;
				}

				char fileBuffer[ 2048 ];
				int bytes = mir_snprintf( fileBuffer, sizeof(fileBuffer), "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", statbuf.st_size );
				WsSend( s, fileBuffer, bytes, MSG_DUMPASTEXT );

				ft->std.sending = TRUE;
				ft->std.currentFileProgress = 0;
				Log( "Sending file data..." );

				while (( numRead = _read( fileId, fileBuffer, 2048 )) > 0 ) {
					if ( Netlib_Send( s, fileBuffer, numRead, 0 ) != numRead ) {
						ft->state = FT_ERROR;
						break;
					}
					ft->std.currentFileProgress += numRead;
					ft->std.totalProgress += numRead;
					JSendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&ft->std );
				}
				_close( fileId );
				if ( ft->state != FT_ERROR )
					ft->state = FT_DONE;
				Log( "Finishing this file..." );
				mir_free( ft->httpPath );
				ft->httpPath = NULL;
				break;
		}	}

		mir_free( str );
		q += 2;
		num += ( q-p );
		p = q;
	}

	return num;
}

/////////////////////////////////////////////////////////////////////////////////////////
// filetransfer class members

filetransfer::filetransfer( CJabberProto* proto )
{
	memset( this, 0, sizeof( filetransfer ));
	ppro = proto;
	fileId = -1;
	std.cbSize = sizeof( std );
}

filetransfer::~filetransfer()
{
	ppro->Log( "Destroying file transfer session %08p", this );

	if ( !bCompleted )
		ppro->JSendBroadcast( std.hContact, ACKTYPE_FILE, ACKRESULT_FAILED, this, 0 );

	close();

	if ( hWaitEvent != INVALID_HANDLE_VALUE )
		CloseHandle( hWaitEvent );

	if ( jid ) mir_free( jid );
	if ( sid ) mir_free( sid );
	if ( iqId ) mir_free( iqId );
	if ( fileSize ) mir_free( fileSize );
	if ( httpHostName ) mir_free( httpHostName );
	if ( httpPath ) mir_free( httpPath );
	if ( szDescription ) mir_free( szDescription );

	if ( std.workingDir ) mir_free( std.workingDir );
	if ( std.currentFile ) mir_free( std.currentFile );

	if ( std.files ) {
		for ( int i=0; i < std.totalFiles; i++ )
			if ( std.files[i] ) mir_free( std.files[i] );

		mir_free( std.files );
}	}

void filetransfer::close()
{
	if ( fileId != -1 ) {
		_close( fileId );
		fileId = -1;
}	}

void filetransfer::complete()
{
	close();

	bCompleted = true;
	ppro->JSendBroadcast( std.hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, this, 0);
}

int filetransfer::create()
{
	if ( fileId != -1 )
		return fileId;

	char filefull[ MAX_PATH ];
	mir_snprintf( filefull, sizeof filefull, "%s\\%s", std.workingDir, std.currentFile );
	replaceStr( std.currentFile, filefull );

	if ( hWaitEvent != INVALID_HANDLE_VALUE )
		CloseHandle( hWaitEvent );
	hWaitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	if ( ppro->JSendBroadcast( std.hContact, ACKTYPE_FILE, ACKRESULT_FILERESUME, this, ( LPARAM )&std ))
		WaitForSingleObject( hWaitEvent, INFINITE );

	if ( IsWinVerNT() && wszFileName != NULL ) {
		WCHAR wszTemp[ MAX_PATH ];
		_snwprintf( wszTemp, SIZEOF(wszTemp), L"%S\\%s", std.workingDir, wszFileName );
		wszTemp[ MAX_PATH-1 ] = 0;
		fileId = _wopen( wszTemp, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
		if ( fileId != -1 ) {
			WIN32_FIND_DATAW data;
			HANDLE hFind = FindFirstFileW( wszFileName, &data );
			if ( hFind != INVALID_HANDLE_VALUE ) {
				mir_free( std.currentFile );

            char tShortName[ 20 ];
				WideCharToMultiByte( CP_ACP, 0,
					( data.cAlternateFileName[0] != 0 ) ? data.cAlternateFileName : data.cFileName,
					-1, tShortName, sizeof tShortName, 0, 0 );
				mir_snprintf( filefull, sizeof( filefull ), "%s\\%s", std.workingDir, tShortName );
				std.currentFile = mir_strdup( filefull );
				ppro->Log( "Saving to [%s]", std.currentFile );
				FindClose( hFind );
	}	}	}

	if ( fileId == -1 ) {
		ppro->Log( "Saving to [%s]", std.currentFile );
		fileId = _open( std.currentFile, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE );
	}

	if ( fileId == -1 )
		ppro->Log( "Cannot create file '%s' during a file transfer", filefull );
	else if ( std.currentFileSize != 0 )
		_chsize( fileId, std.currentFileSize );

	return fileId;
}
