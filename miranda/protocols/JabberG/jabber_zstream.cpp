/*

Jabber Protocol Plugin for Miranda IM
XEP-0138 (Stream Compression) implementation

Copyright ( C ) 2005-08  George Hazan
Copyright ( C ) 2007     Kostya Chukavin, Taras Zackrepa

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

BOOL ThreadData::zlibInit( void )
{
	proto->Log( "Zlib init..." );
	zStreamIn.zalloc = Z_NULL;
	zStreamIn.zfree = Z_NULL;
	zStreamIn.opaque = Z_NULL;
	zStreamIn.next_in = Z_NULL;
	zStreamIn.avail_in = 0;

	zStreamOut.zalloc = Z_NULL;
	zStreamOut.zfree = Z_NULL;
	zStreamOut.opaque = Z_NULL;

	if ( deflateInit( &zStreamOut, Z_BEST_COMPRESSION) != Z_OK ) return FALSE;
	if ( inflateInit( &zStreamIn ) != Z_OK ) return FALSE;

	zRecvReady = true;
	return TRUE;
}

void ThreadData::zlibUninit( void )
{
	deflateEnd( &zStreamOut );
	inflateEnd( &zStreamIn );
}

int ThreadData::zlibSend( char* data, int datalen )
{
	char send_data[ ZLIB_CHUNK_SIZE ];
	int bytesOut = 0;

	zStreamOut.avail_in = datalen;
	zStreamOut.next_in = ( unsigned char* )data;

	do {
		zStreamOut.avail_out = ZLIB_CHUNK_SIZE;
		zStreamOut.next_out = ( unsigned char* )send_data;

		switch ( deflate( &zStreamOut, Z_SYNC_FLUSH )) {
			case Z_OK:         proto->Log( "Deflate: Z_OK" );         break;
			case Z_BUF_ERROR:  proto->Log( "Deflate: Z_BUF_ERROR" );  break;
			case Z_DATA_ERROR: proto->Log( "Deflate: Z_DATA_ERROR" ); break;
			case Z_MEM_ERROR:  proto->Log( "Deflate: Z_MEM_ERROR" );  break;
		}

		int len, send_datalen = ZLIB_CHUNK_SIZE - zStreamOut.avail_out;

		if (( len = sendws( send_data, send_datalen, MSG_NODUMP )) == SOCKET_ERROR || len != send_datalen ) {
			proto->Log( "Netlib_Send() failed, error=%d", WSAGetLastError());
			return FALSE;
		}

		bytesOut += len;
	}
		while ( zStreamOut.avail_out == 0 );

	if ( DBGetContactSettingByte( NULL, "Netlib", "DumpSent", TRUE ) == TRUE )
		proto->Log( "(ZLIB) Data sent\n%s\n===OUT: %d(%d) bytes", data, datalen, bytesOut );

	return TRUE;
}

int ThreadData::zlibRecv( char* data, long datalen )
{
	if ( zRecvReady ) {
		zRecvDatalen = recvws( zRecvData, ZLIB_CHUNK_SIZE, MSG_NODUMP );
		if ( zRecvDatalen == SOCKET_ERROR ) {
			proto->Log( "Netlib_Recv() failed, error=%d", WSAGetLastError());
			return SOCKET_ERROR;
		}
		if ( zRecvDatalen == 0 )
			return 0;

		zStreamIn.avail_in = zRecvDatalen;
		zStreamIn.next_in = ( Bytef* )zRecvData;
	}

	zStreamIn.avail_out = datalen;
	zStreamIn.next_out = ( BYTE* )data;

	switch ( inflate( &zStreamIn, Z_NO_FLUSH )) {
		case Z_OK:         proto->Log( "Inflate: Z_OK" );         break;
		case Z_BUF_ERROR:  proto->Log( "Inflate: Z_BUF_ERROR" );  break;
		case Z_DATA_ERROR: proto->Log( "Inflate: Z_DATA_ERROR" ); break;
		case Z_MEM_ERROR:  proto->Log( "Inflate: Z_MEM_ERROR" );  break;
	}

	int len = datalen - zStreamIn.avail_out;

	if ( DBGetContactSettingByte( NULL, "Netlib", "DumpRecv", TRUE ) == TRUE ) {
		char* szLogBuffer = ( char* )alloca( len+32 );
		memcpy( szLogBuffer, data, len );
		szLogBuffer[ len ]='\0';
		proto->Log( "(ZLIB) Data received\n%s\n===IN: %d(%d) bytes", szLogBuffer, len, zRecvDatalen );
	}

	zRecvReady = ( zStreamIn.avail_out != 0 );
	return len;
}
