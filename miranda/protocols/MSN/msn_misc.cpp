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

#include <stdio.h>
#include <stdarg.h>

#include "msn_global.h"

#include <direct.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "resource.h"

typedef LONG ( WINAPI pIncrementFunc )( PLONG );

static pIncrementFunc  MyInterlockedIncrement95;
static pIncrementFunc  MyInterlockedIncrementInit;

pIncrementFunc *MyInterlockedIncrement = MyInterlockedIncrementInit;

static CRITICAL_SECTION csInterlocked95;
extern HANDLE msnMainThread;
extern bool msnUseExtendedPopups;

//=======================================================================================
// MirandaStatusToMSN - status helper functions
//=======================================================================================

char* __stdcall MirandaStatusToMSN( int status )
{
	switch(status)
	{
		case ID_STATUS_OFFLINE:		return "FLN";
		case ID_STATUS_NA:			return "BRB";
		case ID_STATUS_AWAY:			return "AWY";
		case ID_STATUS_DND:
		case ID_STATUS_OCCUPIED:	return "BSY";
		case ID_STATUS_ONTHEPHONE: return "PHN";
		case ID_STATUS_OUTTOLUNCH: return "LUN";
		case ID_STATUS_INVISIBLE:	return "HDN";
//		case ID_STATUS_IDLE:			return "IDL";
		default:							return "NLN";
}	}

int __stdcall MSNStatusToMiranda(const char *status)
{
	switch((*(PDWORD)status&0x00FFFFFF)|0x20000000) {
		case ' NLN': return ID_STATUS_ONLINE;
		case ' YWA': return ID_STATUS_AWAY;
		case ' LDI': //return ID_STATUS_IDLE;
		case ' BRB': return ID_STATUS_NA;
		case ' YSB': return ID_STATUS_OCCUPIED;
		case ' NHP': return ID_STATUS_ONTHEPHONE;
		case ' NUL': return ID_STATUS_OUTTOLUNCH;
		case ' NDH': return ID_STATUS_INVISIBLE;
		default: return ID_STATUS_OFFLINE;
}	}

//=======================================================================================
// MSN_AddUser - adds a e-mail address to one of the MSN server lists
//=======================================================================================

int __stdcall MSN_AddUser( HANDLE hContact, const char* email, int flags )
{
	if ( flags & LIST_REMOVE )
	{
		if ( !Lists_IsInList( flags & 0xFF, email ))
			return 0;
	}
	else if ( Lists_IsInList( flags, email ))
		return 0;

	char* listName;

	switch( flags & 0xFF )
	{
		case LIST_AL: listName = "AL";	break;
		case LIST_BL: listName = "BL";	break;
		case LIST_FL: listName = "FL";	break;
		case LIST_RL: listName = "RL";	break;
		case LIST_PL: listName = "PL";	break;
		default:
			return -1;
	}

	int msgid;
	if (( flags & 0xFF ) == LIST_FL ) {
		if ( flags & LIST_REMOVE ) {
			if ( hContact == NULL )
				if (( hContact = MSN_HContactFromEmail( email, "", 0, 0 )) == NULL )
					return -1;

			char id[ MSN_GUID_LEN ];
			if ( !MSN_GetStaticString( "ID", hContact, id, sizeof id ))
				msgid = MSN_SendPacket( msnNSSocket, "REM", "%s %s", listName, id );
		}
		else msgid = MSN_SendPacket( msnNSSocket, "ADC", "%s N=%s F=%s", listName, email, email );
	}
	else {
		if ( flags & LIST_REMOVE )
			msgid = MSN_SendPacket( msnNSSocket, "REM", "%s %s", listName, email );
		else
			msgid = MSN_SendPacket( msnNSSocket, "ADC", "%s N=%s", listName, email );
	}

	return msgid;
}

//=======================================================================================
// MSN_AddAuthRequest - adds the authorization event to the database
//=======================================================================================

void __stdcall MSN_AddAuthRequest( HANDLE hContact, const char *email, const char *nick )
{
	//blob is: UIN=0(DWORD), hContact(DWORD), nick(ASCIIZ), ""(ASCIIZ), ""(ASCIIZ), email(ASCIIZ), ""(ASCIIZ)

	DBEVENTINFO dbei = { 0 };
	dbei.cbSize    = sizeof( dbei );
	dbei.szModule  = msnProtocolName;
	dbei.timestamp = (DWORD)time(NULL);
	dbei.flags     = 0;
	dbei.eventType = EVENTTYPE_AUTHREQUEST;
	dbei.cbBlob    = sizeof(DWORD)*2+strlen(nick)+strlen(email)+5;

	PBYTE pCurBlob = dbei.pBlob = ( PBYTE )malloc( dbei.cbBlob );
	*( PDWORD )pCurBlob = 0; pCurBlob+=sizeof( DWORD );
	*( PDWORD )pCurBlob = ( DWORD )hContact; pCurBlob+=sizeof( DWORD );
	strcpy(( char* )pCurBlob, nick); pCurBlob += strlen( nick )+1;
	*pCurBlob = '\0'; pCurBlob++;	   //firstName
	*pCurBlob = '\0'; pCurBlob++;	   //lastName
	strcpy(( char* )pCurBlob, email ); pCurBlob += strlen( email )+1;
	*pCurBlob = '\0';         	   //reason
	MSN_CallService( MS_DB_EVENT_ADD, NULL,( LPARAM )&dbei );
}

//=======================================================================================
// MSN_DebugLog - writes a line of comments to the network log
//=======================================================================================

void __stdcall	MSN_DebugLog( const char *fmt, ... )
{
	char		str[ 4096 ];
	va_list	vararg;

	va_start( vararg, fmt );
	int tBytes = _vsnprintf( str, sizeof( str ), fmt, vararg );
	if ( tBytes > 0 )
		str[ tBytes ] = 0;

	MSN_CallService( MS_NETLIB_LOG, ( WPARAM )hNetlibUser, ( LPARAM )str );
	va_end( vararg );
}

//=======================================================================================
// MSN_GetAvatarFileName - gets a file name for an contact's avatar
//=======================================================================================

void __stdcall MSN_DumpMemory( const char* buffer, int bufSize )
{
   char TmpBuffer[ 256 ];
   long Ptr = 0;

   while ( Ptr < bufSize ) {
		char* bufferPtr = TmpBuffer + sprintf( TmpBuffer, "%04X ", Ptr );
      int i;

		for ( i=0; Ptr+i < bufSize && i < 16; i++ )
         bufferPtr += sprintf( bufferPtr, "%02X ", BYTE( buffer[Ptr+i] ));

		while ( i++ < 17 ) {
			strcat( bufferPtr, "   " );
			bufferPtr += 3;
		}

      for ( i=0; Ptr < bufSize && i < 16; i++, Ptr++ )
			*bufferPtr++ = ( BYTE( buffer[ Ptr ]) >= ' ' ) ? buffer[ Ptr ] : '.';

		*bufferPtr = 0;

		MSN_CallService( MS_NETLIB_LOG, ( WPARAM )hNetlibUser, ( LPARAM )TmpBuffer );
}	}

//=======================================================================================
// MSN_GetAvatarFileName - gets a file name for an contact's avatar
//=======================================================================================

void __stdcall MSN_GetAvatarFileName( HANDLE hContact, char* pszDest, int cbLen )
{
	MSN_CallService( MS_DB_GETPROFILEPATH, cbLen, LPARAM( pszDest ));

	int tPathLen = strlen( pszDest );
	tPathLen += _snprintf( pszDest + tPathLen, MAX_PATH - tPathLen, "\\%s\\", msnProtocolName );
	CreateDirectory( pszDest, NULL );

	if ( hContact != NULL ) {
		ltoa(( long )hContact, pszDest + tPathLen, 10 );
		strcat( pszDest + tPathLen, ".bmp" );
	}
	else strcpy( pszDest + tPathLen, "avatar.png" );
}

//=======================================================================================
// MSN_GoOffline - performs several actions when a server goes offline
//=======================================================================================

void __stdcall	MSN_GoOffline()
{
	if ( msnLoggedIn )
		MSN_SendPacket( msnNSSocket, "OUT", NULL );

	int msnOldStatus = msnStatusMode; msnStatusMode = ID_STATUS_OFFLINE;
	MSN_SendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)msnOldStatus, ID_STATUS_OFFLINE );

	msnLoggedIn = false;

	MSN_EnableMenuItems( FALSE );
	MSN_CloseConnections();

	HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL )
	{
		if ( !lstrcmp( msnProtocolName, (char*)MSN_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact, 0 )))
			if ( ID_STATUS_OFFLINE != MSN_GetWord( hContact, "Status", ID_STATUS_OFFLINE ))
				MSN_SetWord( hContact, "Status", ID_STATUS_OFFLINE );

		hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 );
}	}

//=======================================================================================
// MSN_SendMessage - formats and sends a MSG packet through the server
//=======================================================================================

char sttHeaderStart[] = "MIME-Version: 1.0\r\n";

LONG ThreadData::sendMessage( const char* parMsg, int parFlags )
{
	char tHeader[ 1024 ];
	strcpy( tHeader, sttHeaderStart );

	if (( parFlags & MSG_DISABLE_HDR ) == 0 ) {
		char  tFontName[ 100 ], tFontStyle[ 3 ];
		DWORD tFontColor;

		strcpy( tFontName, "Arial" );

		if ( MSN_GetByte( "SendFontInfo", 1 )) {
			char* p;

			DBVARIANT dbv;
			if ( !DBGetContactSetting( NULL, "SRMsg", "Font0", &dbv )) {
				for ( p = dbv.pszVal; *p; p++ )
					if ( BYTE( *p ) >= 128 || *p < 32 )
						break;

				if ( *p == 0 ) {
					UrlEncode( dbv.pszVal, tFontName, sizeof( tFontName ));
					MSN_FreeVariant( &dbv );
			}	}

			{	BYTE  tStyle = DBGetContactSettingByte( NULL, "SRMsg", "Font0Sty", 0 );
				p = tFontStyle;
				if ( tStyle & 1 ) *p++ = 'B';
				if ( tStyle & 2 ) *p++ = 'I';
				*p = 0;
			}

			tFontColor = DBGetContactSettingDword( NULL, "SRMsg", "Font0Col", 0 );
		}
		else {
			tFontColor = 0;
			tFontStyle[ 0 ] = 0;
		}

		_snprintf( tHeader + sizeof( sttHeaderStart )-1, sizeof( tHeader )-sizeof( sttHeaderStart ),
			"Content-Type: text/plain; charset=UTF-8\r\n"
			"X-MMS-IM-Format: FN=%s; EF=%s; CO=%06X; CS=0; PF=31\r\n\r\n",
			tFontName, tFontStyle, tFontColor );
	}

	return sendPacket( "MSG", "%c %d\r\n%s%s",
		( parFlags & MSG_REQUIRE_ACK ) ? 'A' : 'N',
		strlen( parMsg )+strlen( tHeader ), tHeader, parMsg );
}

//=======================================================================================
// MSN_SendRawPacket - sends a packet accordingly to the MSN protocol
//=======================================================================================

LONG ThreadData::sendRawMessage( int msgType, const char* data, int datLen )
{
	if ( data == NULL )
		data = "";

	if ( datLen == -1 )
		datLen = strlen( data );

	char* buf = ( char* )alloca( datLen + 100 );

	LONG thisTrid = MyInterlockedIncrement( &mTrid );
	int nBytes = _snprintf( buf, 100, "MSG %d %c %d\r\n%s",
		thisTrid, msgType, datLen + sizeof(sttHeaderStart)-1, sttHeaderStart );
	memcpy( buf + nBytes, data, datLen );
	MSN_WS_Send( s, buf, nBytes + datLen );

	return thisTrid;
}

//=======================================================================================
// MsnSendNickname - update our own nickname on the server
//=======================================================================================

int __stdcall MSN_SendNickname(char *email, char *nickname)
{
	char* nickutf = Utf8Encode( nickname );

	char urlNick[ 387 ];
	UrlEncode( nickutf, urlNick,  sizeof( urlNick ));
	MSN_SendPacket( msnNSSocket, "PRP", "MFN %s", urlNick );

	free( nickutf );
	return 0;
}

//=======================================================================================
// MSN_SendPacket - sends a packet accordingly to the MSN protocol
//=======================================================================================

LONG __stdcall	MSN_SendPacket( HANDLE s, const char* cmd, const char* fmt,...)
{
	ThreadData* T = MSN_GetThreadByConnection( s );
	if ( T == NULL )
		return 0;

	va_list vararg;
	va_start( vararg, fmt );
	return T->vsendPacket( cmd, fmt, vararg );
}

LONG ThreadData::sendPacket( const char* cmd, const char* fmt,...)
{
	va_list vararg;
	va_start( vararg, fmt );
	return vsendPacket( cmd, fmt, vararg );
}

LONG ThreadData::vsendPacket( const char* cmd, const char* fmt, va_list vararg )
{
	int strsize = 512;
	char* str = (char*)malloc( strsize );

	LONG thisTrid = MyInterlockedIncrement( &mTrid );

	if ( fmt == NULL || fmt[0] == '\0' )
		sprintf( str, "%s %d", cmd, thisTrid );
	else  {
		int paramStart = sprintf( str, "%s %d ", cmd, thisTrid );
		while ( _vsnprintf( str+paramStart, strsize-paramStart-2, fmt, vararg ) == -1 )
			str = (char*)realloc( str, strsize += 512 );
	}

	if (( strncmp( str, "MSG", 3 ) !=0 ) && ( strncmp( str, "QRY", 3 ) != 0 ))
		strcat( str,"\r\n" );

	MSN_WS_Send( s, str, strlen( str ));
	free( str );
	return thisTrid;
}

//=======================================================================================
// MSN_SetServerStatus - changes plugins status at the server
//=======================================================================================

void __stdcall MSN_SetServerStatus( int newStatus )
{
	if ( !msnLoggedIn )
		return;

	char* szStatusName = MirandaStatusToMSN( newStatus );

	if ( newStatus != ID_STATUS_OFFLINE ) {
		char szMsnObject[ 1000 ];
		if ( MSN_GetStaticString( "PictObject", NULL, szMsnObject, sizeof szMsnObject ))
			szMsnObject[ 0 ] = 0;

		MSN_SendPacket( msnNSSocket, "CHG", "%s 805306404 %s", szStatusName, szMsnObject );
	}
	else MSN_SendPacket( msnNSSocket, "CHG", szStatusName );
}

//=======================================================================================
// MSN_ShowError - shows an error
//=======================================================================================

void __cdecl MSN_ShowError( const char* msgtext, ... )
{
	char    tBuffer[ 4096 ];
	va_list tArgs;

	va_start( tArgs, msgtext );
	_vsnprintf( tBuffer, sizeof( tBuffer ), MSN_Translate( msgtext ), tArgs );
	va_end( tArgs );

	if ( MyOptions.ShowErrorsAsPopups )
		MSN_ShowPopup( msnProtocolName, tBuffer, MSN_ALLOW_MSGBOX | MSN_SHOW_ERROR );
	else
		MessageBox( NULL, tBuffer, msnProtocolName, MB_OK );
}

//=======================================================================================
// MSN_ShowPopup - popup plugin support
//=======================================================================================

void CALLBACK sttMainThreadCallback( ULONG dwParam )
{
	POPUPDATAEX* ppd = ( POPUPDATAEX* )dwParam;

	if ( msnUseExtendedPopups )
		MSN_CallService( MS_POPUP_ADDPOPUPEX, ( WPARAM )ppd, 0 );
	else
		MSN_CallService( MS_POPUP_ADDPOPUP, ( WPARAM )ppd, 0 );

	free( ppd );
}

void __stdcall	MSN_ShowPopup( const char* nickname, const char* msg, int flags )
{
	if ( !ServiceExists( MS_POPUP_ADDPOPUP )) {
		if ( flags & MSN_ALLOW_MSGBOX )
			MessageBox( NULL, msg, "MSN Protocol", MB_OK + ( flags & MSN_SHOW_ERROR ) ? MB_ICONERROR : MB_ICONINFORMATION );

		return;
	}

	POPUPDATAEX* ppd = ( POPUPDATAEX* )calloc( sizeof( POPUPDATAEX ), 1 );

	ppd->lchContact = NULL;
	ppd->lchIcon = LoadIcon( hInst, MAKEINTRESOURCE( IDI_MSN ));
	strcpy( ppd->lpzContactName, nickname );
	strcpy( ppd->lpzText, msg );

	if ( flags & MSN_SHOW_ERROR ) {
		ppd->lchIcon   = LoadIcon( NULL, IDI_WARNING );
		if ( ServiceExists( MS_POPUP_ADDCLASS ))
			ppd->lpzClass  = POPUP_CLASS_WARNING;
		else {
			ppd->colorBack = RGB(191,0,0); //Red
			ppd->colorText = RGB(255,245,225); //Yellow
		}

		ppd->iSeconds  = 60;
	}
	else {
		ppd->colorBack = ( MyOptions.UseWinColors ) ? GetSysColor( COLOR_BTNFACE ) : MyOptions.BGColour;
		ppd->colorText = ( MyOptions.UseWinColors ) ? GetSysColor( COLOR_WINDOWTEXT ) : MyOptions.TextColour;
		if ( msnUseExtendedPopups )
			ppd->iSeconds = ( flags & MSN_HOTMAIL_POPUP ) ? MyOptions.PopupTimeoutHotmail : MyOptions.PopupTimeoutOther;
	}

	ppd->PluginWindowProc = ( WNDPROC )NullWindowProc;
	ppd->PluginData = ( flags & MSN_ALLOW_ENTER ) ? &ppd : NULL;

	QueueUserAPC( sttMainThreadCallback , msnMainThread, ( ULONG )ppd );
}

//=======================================================================================
// MSN_StoreLen - stores a message's length in a buffer
//=======================================================================================

char* __stdcall MSN_StoreLen( char* dest, char* last )
{
	char tBuffer[ 20 ];
	ltoa( short( last-dest )-7, tBuffer, 10 );
	int cbDigits = strlen( tBuffer );
	memcpy( dest, tBuffer, cbDigits );
	memmove( dest + cbDigits, dest + 5, int( last-dest )-7 );
	return last - ( 5 - cbDigits );
}

//=======================================================================================
// UrlDecode - converts URL chars like %20 into printable characters
//=======================================================================================

static int SingleHexToDecimal(char c)
{
	if ( c >= '0' && c <= '9' ) return c-'0';
	if ( c >= 'a' && c <= 'f' ) return c-'a'+10;
	if ( c >= 'A' && c <= 'F' ) return c-'A'+10;
	return -1;
}

void __stdcall UrlDecode( char* str )
{
	char* s = str, *d = str;

	while( *s )
	{
		if ( *s != '%' ) {
			*d++ = *s++;
			continue;
		}

		int digit1 = SingleHexToDecimal( s[1] ), digit2 = SingleHexToDecimal( s[2] );
		if ( digit1 == -1 || digit2 == -1 ) {
			*d++ = *s++;
			continue;
		}

		s += 3;
		*d++ = ( digit1 << 4 ) + digit2;
	}

	*d = 0;
}

//=======================================================================================
// UrlEncode - converts printable characters into URL chars like %20
//=======================================================================================

void __stdcall UrlEncode( const char* src,char* dest, int cbDest )
{
	BYTE* d = ( BYTE* )dest;
	int   i = 0;

	for( const BYTE* s = ( const BYTE* )src; *s; s++ ) {
		if (( *s < '0' && *s != '.' && *s != '-' ) ||
			 ( *s >= ':' && *s <= '?' ) ||
			 ( *s >= '[' && *s <= '`' && *s != '_' ) ||
			 ( *s >= '{' ))
		{
			if ( i >= cbDest-4 )
				break;

			*d++ = '%';
			_itoa( *s, ( char* )d, 16 );
			d += 2;
			i += 3;
		}
		else
		{
			*d++ = *s;
			if ( i++ == cbDest-1 )
				break;
	}	}

	*d = '\0';
}

//=======================================================================================
// Utf8Decode - converts UTF8-encoded string to the UCS2/MBCS format
//=======================================================================================

void __stdcall Utf8Decode( char* str, int maxSize, wchar_t** ucs2 )
{
	wchar_t* tempBuf;

	int len = strlen( str );
	if ( len < 2 ) {
		if ( ucs2 != NULL ) {
			*ucs2 = ( wchar_t* )malloc(( len+1 )*sizeof( wchar_t ));
			MultiByteToWideChar( CP_ACP, 0, str, len, *ucs2, len );
			( *ucs2 )[ len ] = 0;
		}
		return;
	}

	tempBuf = ( wchar_t* )alloca(( len+1 )*sizeof( wchar_t ));
	{
		wchar_t* d = tempBuf;
		BYTE* s = ( BYTE* )str;

		while( *s )
		{
			if (( *s & 0x80 ) == 0 ) {
				*d++ = *s++;
				continue;
			}

			if (( s[0] & 0xE0 ) == 0xE0 && ( s[1] & 0xC0 ) == 0x80 && ( s[2] & 0xC0 ) == 0x80 ) {
				*d++ = (( WORD )( s[0] & 0x0F) << 12 ) + ( WORD )(( s[1] & 0x3F ) << 6 ) + ( WORD )( s[2] & 0x3F );
				s += 3;
				continue;
			}

			if (( s[0] & 0xE0 ) == 0xC0 && ( s[1] & 0xC0 ) == 0x80 ) {
				*d++ = ( WORD )(( s[0] & 0x1F ) << 6 ) + ( WORD )( s[1] & 0x3F );
				s += 2;
				continue;
			}

			*d++ = *s++;
		}

		*d = 0;
	}

	if ( ucs2 != NULL ) {
		int fullLen = ( len+1 )*sizeof( wchar_t );
		*ucs2 = ( wchar_t* )malloc( fullLen );
		memcpy( *ucs2, tempBuf, fullLen );
	}

	if ( maxSize == 0 )
		maxSize = len;

   WideCharToMultiByte( CP_ACP, 0, tempBuf, -1, str, maxSize, NULL, NULL );
}

//=======================================================================================
// Utf8Encode - converts MBCS string to the UTF8-encoded format
//=======================================================================================

char* __stdcall Utf8Encode( const char* src )
{
	wchar_t* tempBuf;

	int len = strlen( src );
	char* result = ( char* )malloc( len*3 + 1 );
	if ( result == NULL )
		return NULL;

	tempBuf = ( wchar_t* )alloca(( len+1 )*sizeof( wchar_t ));
	MultiByteToWideChar( CP_ACP, 0, src, -1, tempBuf, len );
	tempBuf[ len ] = 0;
	{
		wchar_t* s = tempBuf;
		BYTE*		d = ( BYTE* )result;

		while( *s ) {
			int U = *s++;

			if ( U < 0x80 ) {
				*d++ = ( BYTE )U;
			}
			else if ( U < 0x800 ) {
				*d++ = 0xC0 + (( U >> 6 ) & 0x3F );
				*d++ = 0x80 + ( U & 0x003F );
			}
			else {
				*d++ = 0xE0 + ( U >> 12 );
				*d++ = 0x80 + (( U >> 6 ) & 0x3F );
				*d++ = 0x80 + ( U & 0x3F );
		}	}

		*d = 0;
	}

	return result;
}

//=======================================================================================
// Utf8Encode - converts UCS2 string to the UTF8-encoded format
//=======================================================================================

char* __stdcall Utf8EncodeUcs2( const wchar_t* src )
{
	int len = wcslen( src );
	char* result = ( char* )malloc( len*3 + 1 );
	if ( result == NULL )
		return NULL;

	{	const wchar_t* s = src;
		BYTE*	d = ( BYTE* )result;

		while( *s ) {
			int U = *s++;

			if ( U < 0x80 ) {
				*d++ = ( BYTE )U;
			}
			else if ( U < 0x800 ) {
				*d++ = 0xC0 + (( U >> 6 ) & 0x3F );
				*d++ = 0x80 + ( U & 0x003F );
			}
			else {
				*d++ = 0xE0 + ( U >> 12 );
				*d++ = 0x80 + (( U >> 6 ) & 0x3F );
				*d++ = 0x80 + ( U & 0x3F );
		}	}

		*d = 0;
	}

	return result;
}

//=======================================================================================
// filetransfer class members
//=======================================================================================

filetransfer::filetransfer()
{
	memset( this, 0, sizeof( filetransfer ));
	fileId = -1;
	hWaitEvent = INVALID_HANDLE_VALUE;
	std.cbSize = sizeof( std );
}

filetransfer::~filetransfer()
{
	MSN_DebugLog( "Destroying file transfer session %ld", p2p_sessionid );

	if ( !bCompleted )
		MSN_SendBroadcast( std.hContact, ACKTYPE_FILE, ACKRESULT_FAILED, this, 0);

	if ( inmemTransfer ) {
		if ( fileBuffer != NULL )
			LocalFree( fileBuffer );
	}
	else if ( fileId != -1 )
		_close( fileId );

	if ( mIncomingBoundPort != NULL )
		Netlib_CloseHandle( mIncomingBoundPort );

	if ( hWaitEvent != INVALID_HANDLE_VALUE )
		CloseHandle( hWaitEvent );

	if ( p2p_branch != NULL ) free( p2p_branch );
	if ( p2p_callID != NULL ) free( p2p_callID );
	if ( p2p_dest   != NULL ) free( p2p_dest );

	if ( std.currentFile != NULL ) free( std.currentFile );
	if ( std.workingDir != NULL ) free( std.workingDir );

	if ( wszFileName != NULL ) free( wszFileName );
	if ( szInvcookie != NULL ) free( szInvcookie );
}

void filetransfer::close()
{
	if ( !inmemTransfer && fileId != -1 ) {
		_close( fileId );
		fileId = -1;
}	}

void filetransfer::complete()
{
	close();

	bCompleted = true;
	MSN_SendBroadcast( std.hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, this, 0);
}

int filetransfer::create()
{
	if ( inmemTransfer ) {
		if ( fileBuffer == NULL ) {
			if (( fileBuffer = ( char* )LocalAlloc( LPTR, DWORD( std.totalBytes ))) == NULL ) {
				MSN_DebugLog( "Not enough memory to receive file '%s'", std.currentFile );
				return -1;
		}	}

		return ( int )fileBuffer;
	}

	if ( fileId != -1 )
		return fileId;

	_chdir( std.workingDir );

	if ( hWaitEvent != INVALID_HANDLE_VALUE )
		CloseHandle( hWaitEvent );
   hWaitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	if ( MSN_SendBroadcast( std.hContact, ACKTYPE_FILE, ACKRESULT_FILERESUME, this, ( LPARAM )&std ))
		WaitForSingleObject( hWaitEvent, INFINITE );

	char filefull[ MAX_PATH ];
	_snprintf( filefull, sizeof( filefull ), "%s\\%s", std.workingDir, std.currentFile );
	std.currentFile = strdup( filefull );

	if ( msnRunningUnderNT && wszFileName != NULL )
		fileId = _wopen( wszFileName, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
	else
		fileId = _open( std.currentFile, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);

	if ( fileId == -1 )
		MSN_DebugLog( "Cannot create file '%s' during a file transfer", filefull );

	return fileId;
}

//=======================================================================================
// TWinErrorCode class
//=======================================================================================

TWinErrorCode::TWinErrorCode() :
	mErrorText( NULL )
{
	mErrorCode = ::GetLastError();
};

TWinErrorCode::~TWinErrorCode()
{
	if ( mErrorText != NULL )
		::LocalFree( mErrorText );
};

char* TWinErrorCode::getText()
{
	if ( mErrorText == NULL )
	{
		int tBytes = 0;

		if ( mErrorCode >= 12000 && mErrorCode < 12500 )
			tBytes = FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
				::GetModuleHandle( "WININET.DLL" ),
				mErrorCode, LANG_NEUTRAL, (LPTSTR)&mErrorText, 0, NULL );

		if ( tBytes == 0 )
			tBytes = FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
				mErrorCode, LANG_NEUTRAL, (LPTSTR)&mErrorText, 0, NULL );

		if ( tBytes == 0 )
		{
			mErrorText = ( LPTSTR )LocalAlloc( LMEM_FIXED, 100 );
			tBytes = _snprintf( mErrorText, 100, "unknown Windows error code %d", mErrorCode );
		}

		*mErrorText = tolower( *mErrorText );

		if ( mErrorText[ tBytes-1 ] == '\n' )
			mErrorText[ --tBytes ] = 0;
		if ( mErrorText[ tBytes-1 ] == '\r' )
			mErrorText[ --tBytes ] = 0;
		if ( mErrorText[ tBytes-1 ] == '.' )
			mErrorText[ tBytes-1 ] = 0;
	}

	return mErrorText;
}

//=======================================================================================
// InterlockedIncrement emulation
//=======================================================================================

//I hate Microsoft (c) cyreve

static LONG WINAPI MyInterlockedIncrement95(PLONG pVal)
{
	DWORD ret;
	EnterCriticalSection(&csInterlocked95);
	ret=++*pVal;
	LeaveCriticalSection(&csInterlocked95);
	return ret;
}

//there's a possible hole here if too many people call this at the same time, but that doesn't happen

static LONG WINAPI MyInterlockedIncrementInit(PLONG pVal)
{
	DWORD ver = GetVersion();
   if (( ver & 0x80000000 ) && LOWORD( ver ) == 0x0004 )
	{
		InitializeCriticalSection( &csInterlocked95 );
		MyInterlockedIncrement = MyInterlockedIncrement95;
	}
   else  MyInterlockedIncrement = ( pIncrementFunc* )InterlockedIncrement;

	return MyInterlockedIncrement( pVal );
}
