/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2003-5 George Hazan.
Copyright (c) 2002-3 Richard Hughes (original version).

Miranda IM: the free icq client for MS Windows
Copyright (C) 2000-2002 Richard Hughes, Roland Rabien & Tristan Van de Vreede

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "msn_global.h"

/////////////////////////////////////////////////////////////////////////////////////////
// constructors and destructor

MimeHeaders::MimeHeaders() :
	mCount( 0 ),
	mVals( NULL )
{
}

MimeHeaders::MimeHeaders( int iInitCount ) :
	mCount( 0 )
{
	mVals = ( MimeHeader* )malloc( iInitCount * sizeof( MimeHeader ));
}

MimeHeaders::~MimeHeaders()
{
	if ( mCount == NULL )
		return;

	for ( int i=0; i < mCount; i++ ) {
		MimeHeader& H = mVals[ i ];
		free( H.name );
		free( H.value );
	}

	free( mVals );
}

/////////////////////////////////////////////////////////////////////////////////////////
// add various values

void MimeHeaders::addString( const char* name, const char* szValue )
{
	MimeHeader& H = mVals[ mCount++ ];
	H.name = strdup( name );
	H.value = strdup( szValue );
}

void MimeHeaders::addLong( const char* name, long lValue )
{
	MimeHeader& H = mVals[ mCount++ ];
	H.name = strdup( name );

	char szBuffer[ 20 ];
	ltoa( lValue, szBuffer, 10 );
	H.value = strdup( szBuffer );
}

/////////////////////////////////////////////////////////////////////////////////////////
// write all values to a buffer

int MimeHeaders::getLength()
{
	int iResult = 0;
	for ( int i=0; i < mCount; i++ ) {
		MimeHeader& H = mVals[ i ];
		iResult += strlen( H.name ) + strlen( H.value ) + 4;
	}

	return iResult;
}

char* MimeHeaders::writeToBuffer( char* pDest )
{
	for ( int i=0; i < mCount; i++ ) {
		MimeHeader& H = mVals[ i ];
		pDest += sprintf( pDest, "%s: %s\r\n", H.name, H.value );
	}

	return pDest;
}

/////////////////////////////////////////////////////////////////////////////////////////
// read set of values from buffer

char* MimeHeaders::readFromBuffer( const char* parString )
{
	char  line[ 4096 ];
	const char* msgBody = parString;

	int        headerCount = 0;
	MimeHeader headers[ 100 ];

	while ( true ) {
		lstrcpyn( line, msgBody, sizeof( line ));
		char* peol = strchr( line, '\r' );
		if ( peol == NULL )
			break;

		msgBody = peol;
		if ( *++msgBody=='\n' )
			msgBody++;

		parString += int( msgBody - line );

		if ( line[0] == '\r' )
			break;

		*peol='\0';
		peol = strchr( line,':' );
		if ( peol == NULL )
		{
			MSN_DebugLog( "MSG: Invalid MIME header: '%s'",line);
			continue;
		}

		*peol='\0'; peol++;
		while ( *peol==' ' || *peol=='\t' )
			peol++;

		MimeHeader& H = headers[ headerCount ];
		H.name = strdup( line );
		H.value = strdup( peol );
		headerCount++;
	}

	mCount = headerCount;
	mVals = ( MimeHeader* )malloc( sizeof( MimeHeader )*headerCount );
	memcpy( mVals, headers, sizeof( MimeHeader )*headerCount );
	return ( char* )parString;
}

const char* MimeHeaders::operator[]( const char* szFieldName )
{
	for ( int i=0; i < mCount; i++ ) {
		MimeHeader& MH = mVals[i];
		if ( !strcmp( MH.name, szFieldName ))
			return MH.value;
	}

	return NULL;
}
