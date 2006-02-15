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

#include "msn_md5.h"
#include "resource.h"

typedef LONG ( WINAPI pIncrementFunc )( PLONG );

static pIncrementFunc  MyInterlockedIncrement95;
static pIncrementFunc  MyInterlockedIncrementInit;

pIncrementFunc *MyInterlockedIncrement = MyInterlockedIncrementInit;

static CRITICAL_SECTION csInterlocked95;
extern HANDLE msnMainThread;
extern bool msnUseExtendedPopups;

/////////////////////////////////////////////////////////////////////////////////////////
// MirandaStatusToMSN - status helper functions

char* __stdcall MirandaStatusToMSN( int status )
{
	switch(status)
	{
		case ID_STATUS_OFFLINE:		return "FLN";
		case ID_STATUS_NA:			return ( MyOptions.AwayAsBrb ) ? (char *)"AWY" : (char *)"BRB";
		case ID_STATUS_AWAY:			return ( MyOptions.AwayAsBrb ) ? (char *)"BRB" : (char *)"AWY";
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
		case ' YWA': return ( MyOptions.AwayAsBrb ) ? ID_STATUS_NA : ID_STATUS_AWAY;
		case ' LDI': //return ID_STATUS_IDLE;
		case ' BRB': return ( MyOptions.AwayAsBrb ) ? ID_STATUS_AWAY : ID_STATUS_NA;
		case ' YSB': return ID_STATUS_OCCUPIED;
		case ' NHP': return ID_STATUS_ONTHEPHONE;
		case ' NUL': return ID_STATUS_OUTTOLUNCH;
		case ' NDH': return ID_STATUS_INVISIBLE;
		default: return ID_STATUS_OFFLINE;
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_AddUser - adds a e-mail address to one of the MSN server lists

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
				if (( hContact = MSN_HContactFromEmail( email, NULL, 0, 0 )) == NULL )
					return -1;

			char id[ MSN_GUID_LEN ];
			if ( !MSN_GetStaticString( "ID", hContact, id, sizeof id ))
				msgid = msnNsThread->sendPacket( "REM", "%s %s", listName, id );
		}
		else msgid = msnNsThread->sendPacket( "ADC", "%s N=%s F=%s", listName, email, email );
	}
	else {
		if ( flags & LIST_REMOVE )
			msgid = msnNsThread->sendPacket( "REM", "%s %s", listName, email );
		else
			msgid = msnNsThread->sendPacket( "ADC", "%s N=%s", listName, email );
	}

	return msgid;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_AddAuthRequest - adds the authorization event to the database

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

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_AddServerGroup - adds a group to the server list

void MSN_AddServerGroup( const char* pszGroupName )
{
	char szBuf[ 200 ];
	UrlEncode( pszGroupName, szBuf, sizeof szBuf );

	if ( hGroupAddEvent == NULL )
		hGroupAddEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	msnNsThread->sendPacket( "ADG", "%s", szBuf );

	WaitForSingleObject( hGroupAddEvent, INFINITE );
	CloseHandle( hGroupAddEvent ); hGroupAddEvent = NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_DebugLog - writes a line of comments to the network log

void __stdcall	MSN_DebugLog( const char *fmt, ... )
{
	char		str[ 4096 ];
	va_list	vararg;

	va_start( vararg, fmt );
	int tBytes = _vsnprintf( str, sizeof(str)-1, fmt, vararg );
	if ( tBytes == 0 )
		return;

	if ( tBytes > 0 )
		str[ tBytes ] = 0;
	else
		str[ sizeof(str)-1 ] = 0;

	MSN_CallService( MS_NETLIB_LOG, ( WPARAM )hNetlibUser, ( LPARAM )str );
	va_end( vararg );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_DumpMemory - dumps a memory block to the network log

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

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_GetAvatarFileName - gets a file name for an contact's avatar

void __stdcall MSN_GetAvatarFileName( HANDLE hContact, char* pszDest, int cbLen )
{
	MSN_CallService( MS_DB_GETPROFILEPATH, cbLen, LPARAM( pszDest ));

	int tPathLen = strlen( pszDest );
	tPathLen += mir_snprintf( pszDest + tPathLen, MAX_PATH - tPathLen, "\\MSN\\"  );
	CreateDirectoryA( pszDest, NULL );

	if ( hContact != NULL ) {
		char szEmail[ MSN_MAX_EMAIL_LEN ];
		if ( MSN_GetStaticString( "e-mail", hContact, szEmail, sizeof( szEmail )))
			ltoa(( long )hContact, szEmail, 10 );

		long digest[ 4 ];
		MD5_CTX ctx;
		MD5Init( &ctx );
		MD5Update( &ctx, ( BYTE* )szEmail, strlen( szEmail ));
		MD5Final(( BYTE* )digest, &ctx );

		tPathLen += mir_snprintf( pszDest + tPathLen, MAX_PATH - tPathLen, "%08lX%08lX%08lX%08lX",
			digest[0], digest[1], digest[2], digest[3] );

		strcat( pszDest + tPathLen, ".bmp" );
	}
	else mir_snprintf( pszDest + tPathLen, MAX_PATH - tPathLen, "%s avatar.png", msnProtocolName );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_GoOffline - performs several actions when a server goes offline

void __stdcall	MSN_GoOffline()
{
	if ( msnLoggedIn )
		msnNsThread->sendPacket( "OUT", NULL );

	int msnOldStatus = msnStatusMode; msnStatusMode = ID_STATUS_OFFLINE;
	MSN_SendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)msnOldStatus, ID_STATUS_OFFLINE );

	msnLoggedIn = false;

	if ( !Miranda_Terminated() )
		MSN_EnableMenuItems( FALSE );
	MSN_CloseConnections();
	MSN_FreeGroups();

	HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL )
	{
		if ( !lstrcmpA( msnProtocolName, (char*)MSN_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact, 0 )))
			if ( ID_STATUS_OFFLINE != MSN_GetWord( hContact, "Status", ID_STATUS_OFFLINE ))
				MSN_SetWord( hContact, "Status", ID_STATUS_OFFLINE );

		hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_SendMessage - formats and sends a MSG packet through the server

char sttHeaderStart[] = "MIME-Version: 1.0\r\n";

LONG ThreadData::sendMessage( int msgType, const char* parMsg, int parFlags )
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

		mir_snprintf( tHeader + sizeof( sttHeaderStart )-1, sizeof( tHeader )-sizeof( sttHeaderStart ),
			"Content-Type: text/plain; charset=UTF-8\r\n"
			"X-MMS-IM-Format: FN=%s; EF=%s; CO=%x; CS=0; PF=31\r\n\r\n",
			tFontName, tFontStyle, tFontColor );
	}

	return sendPacket( "MSG", "%c %d\r\n%s%s", msgType, strlen( parMsg )+strlen( tHeader ), tHeader, parMsg );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_SendRawPacket - sends a packet accordingly to the MSN protocol

LONG ThreadData::sendRawMessage( int msgType, const char* data, int datLen )
{
	if ( data == NULL )
		data = "";

	if ( datLen == -1 )
		datLen = strlen( data );

	char* buf = ( char* )alloca( datLen + 100 );

	LONG thisTrid = MyInterlockedIncrement( &mTrid );
	int nBytes = mir_snprintf( buf, 100, "MSG %d %c %d\r\n%s",
		thisTrid, msgType, datLen + sizeof(sttHeaderStart)-1, sttHeaderStart );
	memcpy( buf + nBytes, data, datLen );
	send( buf, nBytes + datLen );

	return thisTrid;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_RenameServerGroup - renames a group at the server

void MSN_RenameServerGroup( int iNumber, LPCSTR szId, const char* newName )
{
	LPCSTR oldId = MSN_GetGroupByName( newName );
	if ( oldId == NULL ) {
		char szNewName[ 256 ];
		UrlEncode( newName, szNewName, sizeof szNewName );
		msnNsThread->sendPacket( "REG", "%s %s", szId, szNewName );
	}
	else MSN_SetGroupNumber( oldId, iNumber );
}

/////////////////////////////////////////////////////////////////////////////////////////
// Msn_SendNickname - update our own nickname on the server

int __stdcall MSN_SendNickname(char *nickname)
{
	char urlNick[ 387 ];
	UrlEncode( UTF8(nickname), urlNick,  sizeof( urlNick ));
	msnNsThread->sendPacket( "PRP", "MFN %s", urlNick );
	return 0;
}

int __stdcall MSN_SendNicknameW( WCHAR* nickname)
{
	char* nickutf = Utf8EncodeUcs2( nickname );

	char urlNick[ 387 ];
	UrlEncode( nickutf, urlNick,  sizeof( urlNick ));
	msnNsThread->sendPacket( "PRP", "MFN %s", urlNick );

	free( nickutf );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_SendStatusMessage - notify a server about the status message change

extern char* msnPreviousUUX;

void __stdcall MSN_SendStatusMessage( const char* msg )
{
	if ( !msnLoggedIn || !MyOptions.UseMSNP11 )
		return;

	char* msgEnc = HtmlEncode(( msg == NULL ) ? "" : msg );
	char  szMsg[ 1024 ], szEmail[ MSN_MAX_EMAIL_LEN ];
	mir_snprintf( szMsg, sizeof szMsg, "<Data><PSM>%s</PSM><CurrentMedia></CurrentMedia></Data>", UTF8(msgEnc));
	free( msgEnc );

	if ( !lstrcmpA( msnPreviousUUX, szMsg ))
		return;

	replaceStr( msnPreviousUUX, szMsg );
	if ( !MSN_GetStaticString( "e-mail", NULL, szEmail, sizeof szEmail ))
		msnNsThread->sendPacket( "UUX", "%d\r\n%s", strlen( szMsg ), szMsg );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_SendPacket - sends a packet accordingly to the MSN protocol

LONG ThreadData::sendPacket( const char* cmd, const char* fmt,...)
{
	if ( this == NULL )  // :)
		return 0;

	if ( !strcmp( cmd, "CAL" ) && mIsCalSent )
		return 0;

	va_list vararg;
	va_start( vararg, fmt );

	int strsize = 512;
	char* str = ( char* )malloc( strsize );

	LONG thisTrid = MyInterlockedIncrement( &mTrid );

	if ( fmt == NULL || fmt[0] == '\0' )
		sprintf( str, "%s %d", cmd, thisTrid );
	else  {
		int paramStart = sprintf( str, "%s %d ", cmd, thisTrid );
		while ( _vsnprintf( str+paramStart, strsize-paramStart-2, fmt, vararg ) == -1 )
			str = (char*)realloc( str, strsize += 512 );
	}

	if ( strcmp( cmd, "MSG" ) && strcmp( cmd, "QRY" ) && strcmp( cmd, "UUX" ))
		strcat( str,"\r\n" );

	if ( !strcmp( cmd, "CAL" ))
		mIsCalSent = true;

	int result = send( str, strlen( str ));
	free( str );
	return ( result > 0 ) ? thisTrid : -1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_SetServerStatus - changes plugins status at the server

void __stdcall MSN_SetServerStatus( int newStatus )
{
	MSN_DebugLog( "Setting MSN server status %d, logged in = %d", newStatus, msnLoggedIn );

	if ( !msnLoggedIn )
		return;

	char* szStatusName = MirandaStatusToMSN( newStatus );

	if ( newStatus != ID_STATUS_OFFLINE ) {
		char szMsnObject[ 1000 ];
		if ( MSN_GetStaticString( "PictObject", NULL, szMsnObject, sizeof szMsnObject ))
			szMsnObject[ 0 ] = 0;

		//here we say what functions can be used with this plugins : http://siebe.bot2k3.net/docs/?url=clientid.html
		//msnNsThread->sendPacket( "CHG", "%s 805306404 %s", szStatusName, szMsnObject );
		msnNsThread->sendPacket( "CHG", "%s 1342177280 %s", szStatusName, szMsnObject );

		if ( MyOptions.UseMSNP11 ) {
			for ( int i=0; i < MSN_NUM_MODES; i++ ) { 
				if ( msnModeMsgs[ i ].m_mode == newStatus ) {
					if ( msnModeMsgs[ i ].m_msg != NULL )
                  MSN_SendStatusMessage( msnModeMsgs[ i ].m_msg );
					break;
		}	}	}
	}
	else msnNsThread->sendPacket( "CHG", szStatusName );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_ShowError - shows an error

void __cdecl MSN_ShowError( const char* msgtext, ... )
{
	char    tBuffer[ 4096 ];
	va_list tArgs;

	va_start( tArgs, msgtext );
	mir_vsnprintf( tBuffer, sizeof( tBuffer ), MSN_Translate( msgtext ), tArgs );
	va_end( tArgs );

	if ( MyOptions.ShowErrorsAsPopups )
		MSN_ShowPopup( msnProtocolName, tBuffer, MSN_ALLOW_MSGBOX | MSN_SHOW_ERROR );
	else
		MessageBoxA( NULL, tBuffer, msnProtocolName, MB_OK );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_ShowPopup - popup plugin support

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
			MessageBoxA( NULL, msg, "MSN Protocol", MB_OK + ( flags & MSN_SHOW_ERROR ) ? MB_ICONERROR : MB_ICONINFORMATION );

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
			ppd->lpzClass  = _T(POPUP_CLASS_WARNING);
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

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_StoreLen - stores a message's length in a buffer

char* __stdcall MSN_StoreLen( char* dest, char* last )
{
	char tBuffer[ 20 ];
	ltoa( short( last-dest )-7, tBuffer, 10 );
	int cbDigits = strlen( tBuffer );
	memcpy( dest, tBuffer, cbDigits );
	memmove( dest + cbDigits, dest + 5, int( last-dest )-7 );
	return last - ( 5 - cbDigits );
}

/////////////////////////////////////////////////////////////////////////////////////////
// UrlDecode - converts URL chars like %20 into printable characters

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

void __stdcall HtmlDecode( char* str )
{
	char* p, *q;

	if ( str == NULL )
		return;

	for ( p=q=str; *p!='\0'; p++,q++ ) {
		if ( *p == '&' ) {
			if ( !strncmp( p, "&amp;", 5 )) {	*q = '&'; p += 4; }
			else if ( !strncmp( p, "&apos;", 6 )) { *q = '\''; p += 5; }
			else if ( !strncmp( p, "&gt;", 4 )) { *q = '>'; p += 3; }
			else if ( !strncmp( p, "&lt;", 4 )) { *q = '<'; p += 3; }
			else if ( !strncmp( p, "&quot;", 6 )) { *q = '"'; p += 5; }
			else { *q = *p;	}
		}
		else {
			*q = *p;
		}
	}
	*q = '\0';
}

/////////////////////////////////////////////////////////////////////////////////////////
// HtmlEncode - replaces special HTML chars

char* __stdcall HtmlEncode( const char* str )
{
	char* s, *p, *q;
	int c;

	if ( str == NULL )
		return NULL;

	for ( c=0,p=( char* )str; *p!='\0'; p++ ) {
		switch ( *p ) {
		case '&': c += 5; break;
		case '\'': c += 6; break;
		case '>': c += 4; break;
		case '<': c += 4; break;
		case '"': c += 6; break;
		default: c++; break;
		}
	}
	if (( s=( char* )malloc( c+1 )) != NULL ) {
		for ( p=( char* )str,q=s; *p!='\0'; p++ ) {
			switch ( *p ) {
			case '&': strcpy( q, "&amp;" ); q += 5; break;
			case '\'': strcpy( q, "&apos;" ); q += 6; break;
			case '>': strcpy( q, "&gt;" ); q += 4; break;
			case '<': strcpy( q, "&lt;" ); q += 4; break;
			case '"': strcpy( q, "&quot;" ); q += 6; break;
			default: *q = *p; q++; break;
			}
		}
		*q = '\0';
	}

	return s;
}

/////////////////////////////////////////////////////////////////////////////////////////
// UrlEncode - converts printable characters into URL chars like %20

void __stdcall UrlEncode( const char* src,char* dest, int cbDest )
{
	BYTE* d = ( BYTE* )dest;
	int   i = 0;

	for( const BYTE* s = ( const BYTE* )src; *s; s++ ) {
		if (( *s < '0' && *s != '.' && *s != '-' ) ||
			 ( *s >= ':' && *s <= '?' ) ||
			 ( *s >= '[' && *s <= '`' && *s != '_' ))
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

/////////////////////////////////////////////////////////////////////////////////////////
// Utf8Decode - converts UTF8-encoded string to the UCS2/MBCS format

void __stdcall Utf8Decode( char* str, wchar_t** ucs2 )
{
	if ( str == NULL )
		return;

	int len = strlen( str );
	if ( len < 2 ) {
		if ( ucs2 != NULL ) {
			*ucs2 = ( wchar_t* )malloc(( len+1 )*sizeof( wchar_t ));
			MultiByteToWideChar( CP_ACP, 0, str, len, *ucs2, len );
			( *ucs2 )[ len ] = 0;
		}
		return;
	}

	wchar_t* tempBuf = ( wchar_t* )alloca(( len+1 )*sizeof( wchar_t ));
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

   WideCharToMultiByte( CP_ACP, 0, tempBuf, -1, str, len, NULL, NULL );
}

/////////////////////////////////////////////////////////////////////////////////////////
// Utf8Encode - converts MBCS string to the UTF8-encoded format

char* __stdcall Utf8Encode( const char* src )
{
	if ( src == NULL )
		return NULL;

	int len = strlen( src );
	char* result = ( char* )malloc( len*3 + 1 );
	if ( result == NULL )
		return NULL;

	wchar_t* tempBuf = ( wchar_t* )alloca(( len+1 )*sizeof( wchar_t ));
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

/////////////////////////////////////////////////////////////////////////////////////////
// Utf8Encode - converts UCS2 string to the UTF8-encoded format

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

/////////////////////////////////////////////////////////////////////////////////////////
// filetransfer class members

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
	if ( std.files != NULL ) {
		for ( int i=0; i < std.totalFiles; i++ )
			mir_free( std.files[ i ] );
		mir_free( std.files );
	}

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

	char filefull[ MAX_PATH ];
	mir_snprintf( filefull, sizeof filefull, "%s\\%s", std.workingDir, std.currentFile );
	replaceStr( std.currentFile, filefull );

	if ( hWaitEvent != INVALID_HANDLE_VALUE )
		CloseHandle( hWaitEvent );
   hWaitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	if ( MSN_SendBroadcast( std.hContact, ACKTYPE_FILE, ACKRESULT_FILERESUME, this, ( LPARAM )&std ))
		WaitForSingleObject( hWaitEvent, INFINITE );

	#if defined( _UNICODE )	
		if ( wszFileName != NULL ) {
			WCHAR wszTemp[ MAX_PATH ];
			_snwprintf( wszTemp, sizeof wszTemp, L"%S\\%s", std.workingDir, wszFileName );
			wszTemp[ MAX_PATH-1 ] = 0;
			fileId = _wopen( wszTemp, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE);
			if ( fileId != -1 ) {
				WIN32_FIND_DATAW data;
				HANDLE hFind = FindFirstFileW( wszFileName, &data );
				if ( hFind != INVALID_HANDLE_VALUE ) {
					free( std.currentFile );

					char tShortName[ 20 ];
					WideCharToMultiByte( CP_ACP, 0, 
						( data.cAlternateFileName[0] != 0 ) ? data.cAlternateFileName : data.cFileName, 
						-1, tShortName, sizeof tShortName, 0, 0 );
					mir_snprintf( filefull, sizeof( filefull ), "%s\\%s", std.workingDir, tShortName );
					std.currentFile = strdup( filefull );
					FindClose( hFind );
			}	}
		}
		else fileId = _open( std.currentFile, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE );
	#else
		fileId = _open( std.currentFile, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE );
	#endif

	if ( fileId == -1 )
		MSN_DebugLog( "Cannot create file '%s' during a file transfer", filefull );
	else if ( std.currentFileSize != 0 )
		_chsize( fileId, std.currentFileSize );

	return fileId;
}

/////////////////////////////////////////////////////////////////////////////////////////
// TWinErrorCode class

TWinErrorCode::TWinErrorCode() :
	mErrorText( NULL )
{
	mErrorCode = ::GetLastError();
}

TWinErrorCode::~TWinErrorCode()
{
	if ( mErrorText != NULL )
		::LocalFree( mErrorText );
}

char* TWinErrorCode::getText()
{
	if ( mErrorText == NULL )
	{
		int tBytes = 0;

		if ( mErrorCode >= 12000 && mErrorCode < 12500 )
			tBytes = FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
				::GetModuleHandleA( "WININET.DLL" ),
				mErrorCode, LANG_NEUTRAL, (LPSTR)&mErrorText, 0, NULL );

		if ( tBytes == 0 )
			tBytes = FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
				mErrorCode, LANG_NEUTRAL, (LPSTR)&mErrorText, 0, NULL );

		if ( tBytes == 0 )
		{
			mErrorText = ( LPSTR )LocalAlloc( LMEM_FIXED, 100 );
			tBytes = mir_snprintf( mErrorText, 100, "unknown Windows error code %d", mErrorCode );
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

/////////////////////////////////////////////////////////////////////////////////////////
// InterlockedIncrement emulation

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
