/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2008 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "msn_global.h"
#include "msn_proto.h"
#include "version.h"

typedef LONG ( WINAPI pIncrementFunc )( PLONG );
static pIncrementFunc  MyInterlockedIncrement95;
static pIncrementFunc  MyInterlockedIncrementInit;

pIncrementFunc *MyInterlockedIncrement = MyInterlockedIncrementInit;

static CRITICAL_SECTION csInterlocked95;

/////////////////////////////////////////////////////////////////////////////////////////
// MirandaStatusToMSN - status helper functions

const char*  CMsnProto::MirandaStatusToMSN( int status )
{
	switch(status)
	{
		case ID_STATUS_OFFLINE:		return "FLN";
		case ID_STATUS_NA:			return ( MyOptions.AwayAsBrb ) ? (char *)"AWY" : (char *)"BRB";
		case ID_STATUS_AWAY:		return ( MyOptions.AwayAsBrb ) ? (char *)"BRB" : (char *)"AWY";
		case ID_STATUS_DND:
		case ID_STATUS_OCCUPIED:	return "BSY";
		case ID_STATUS_ONTHEPHONE:  return "PHN";
		case ID_STATUS_OUTTOLUNCH:  return "LUN";
		case ID_STATUS_INVISIBLE:	return "HDN";
		case ID_STATUS_IDLE:		return "IDL";
		default:					return "NLN";
}	}

WORD  CMsnProto::MSNStatusToMiranda(const char *status)
{
	switch((*(PDWORD)status&0x00FFFFFF)|0x20000000) {
		case ' LDI': return ID_STATUS_ONLINE;
		case ' NLN': return ID_STATUS_ONLINE;
		case ' YWA': return MyOptions.AwayAsBrb ? ID_STATUS_NA : ID_STATUS_AWAY;
		case ' BRB': return MyOptions.AwayAsBrb ? ID_STATUS_AWAY : ID_STATUS_NA;
		case ' YSB': return ID_STATUS_OCCUPIED;
		case ' NHP': return ID_STATUS_ONTHEPHONE;
		case ' NUL': return ID_STATUS_OUTTOLUNCH;
		case ' NDH': return ID_STATUS_INVISIBLE;
		default: return ID_STATUS_OFFLINE;
}	}

char**  CMsnProto::GetStatusMsgLoc( int status )
{
	static const int modes[ MSN_NUM_MODES ] = {
		ID_STATUS_ONLINE,
		ID_STATUS_AWAY,
		ID_STATUS_DND, 
		ID_STATUS_NA,
		ID_STATUS_OCCUPIED, 
		ID_STATUS_INVISIBLE,
		ID_STATUS_ONTHEPHONE,
		ID_STATUS_OUTTOLUNCH, 
	};

	for ( int i=0; i < MSN_NUM_MODES; i++ ) 
		if ( modes[i] == status ) return &msnModeMsgs[i];

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_AddAuthRequest - adds the authorization event to the database

void  CMsnProto::MSN_AddAuthRequest( HANDLE hContact, const char *email, const char *nick )
{
	//blob is: UIN=0(DWORD), hContact(DWORD), nick(ASCIIZ), ""(ASCIIZ), ""(ASCIIZ), email(ASCIIZ), ""(ASCIIZ)

	CCSDATA ccs = { 0 };
	PROTORECVEVENT pre = { 0 };

    pre.timestamp = ( DWORD )time( NULL );
	pre.lParam = sizeof( DWORD ) * 2 + strlen( nick ) + strlen( email ) + 5;

	ccs.szProtoService = PSR_AUTH;
    ccs.hContact = hContact;
    ccs.lParam = ( LPARAM )&pre;

	PBYTE pCurBlob = ( PBYTE )alloca( pre.lParam );
	pre.szMessage = ( char* )pCurBlob;

	*( PDWORD )pCurBlob = 0; pCurBlob+=sizeof( DWORD );
	*( PDWORD )pCurBlob = ( DWORD )hContact; pCurBlob+=sizeof( DWORD );
	strcpy(( char* )pCurBlob, nick); pCurBlob += strlen( nick )+1;
	*pCurBlob = '\0'; pCurBlob++;	   //firstName
	*pCurBlob = '\0'; pCurBlob++;	   //lastName
	strcpy(( char* )pCurBlob, email ); pCurBlob += strlen( email )+1;
	*pCurBlob = '\0';         	   //reason

	MSN_CallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_DebugLog - writes a line of comments to the network log

void CMsnProto::MSN_DebugLog( const char *fmt, ... )
{
	char	str[ 4096 ];
	va_list	vararg;

	va_start( vararg, fmt );
	if ( _vsnprintf( str, sizeof(str), fmt, vararg ) != 0 )
	{
		str[ sizeof(str)-1 ] = 0;
		MSN_CallService( MS_NETLIB_LOG, ( WPARAM )hNetlibUser, ( LPARAM )str );
	}
	va_end( vararg );
}

void CMsnProto::InitCustomFolders(void)
{
	if ( InitCstFldRan ) return; 

	char AvatarsFolder[MAX_PATH]= "";
	CallService(MS_DB_GETPROFILEPATH, (WPARAM) MAX_PATH, (LPARAM)AvatarsFolder);
	strcat(AvatarsFolder, "\\");
	strcat(AvatarsFolder, m_szProtoName);
	hMSNAvatarsFolder = FoldersRegisterCustomPath(m_szProtoName, "Avatars", AvatarsFolder);
	strcat(AvatarsFolder, "\\CustomSmiley");
	hCustomSmileyFolder = FoldersRegisterCustomPath(m_szProtoName, "Custom Smiley", AvatarsFolder);

	InitCstFldRan = true;
}


char* MSN_GetAvatarHash(char* szContext)
{
	char* res  = NULL;

	ezxml_t xmli = ezxml_parse_str(NEWSTR_ALLOCA(szContext), strlen(szContext));
	const char* szAvatarHash = ezxml_attr(xmli, "SHA1D");
	if (szAvatarHash != NULL)
	{
		BYTE szActHash[MIR_SHA1_HASH_SIZE+2];
		const size_t len = strlen( szAvatarHash );

		NETLIBBASE64 nlb = { (char*)szAvatarHash, len, szActHash, sizeof(szActHash) };
		int decod = MSN_CallService( MS_NETLIB_BASE64DECODE, 0, LPARAM( &nlb ));
		if (decod != 0 && nlb.cbDecoded > 0)
			res = arrayToHex(szActHash, MIR_SHA1_HASH_SIZE);
	}
	ezxml_free(xmli);

	return res;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_GetAvatarFileName - gets a file name for an contact's avatar

void  CMsnProto::MSN_GetAvatarFileName( HANDLE hContact, char* pszDest, size_t cbLen )
{
	size_t tPathLen;

	InitCustomFolders();

	char* path = ( char* )alloca( cbLen );
	if ( hMSNAvatarsFolder == NULL || FoldersGetCustomPath( hMSNAvatarsFolder, path, cbLen, "" ))
	{
		MSN_CallService( MS_DB_GETPROFILEPATH, cbLen, LPARAM( pszDest ));
		
		tPathLen = strlen( pszDest );
		tPathLen += mir_snprintf(pszDest + tPathLen, cbLen - tPathLen,"\\%s", m_szProtoName);
	}
	else {
		strcpy( pszDest, path );
		tPathLen = strlen( pszDest );
	}

	if (_access(pszDest, 0))
		MSN_CallService(MS_UTILS_CREATEDIRTREE, 0, (LPARAM)pszDest);

	if ( hContact != NULL ) 
	{
		DBVARIANT dbv;
		if (getString(hContact, "PictContext", &dbv) == 0)
		{
			char* szAvatarHash = MSN_GetAvatarHash(dbv.pszVal);
			if (szAvatarHash != NULL)
			{
				tPathLen += mir_snprintf(pszDest + tPathLen, cbLen - tPathLen, "\\%s.", szAvatarHash );
				mir_free(szAvatarHash);
			}
			MSN_FreeVariant( &dbv );
		}
	}
	else 
		tPathLen += mir_snprintf(pszDest + tPathLen, cbLen - tPathLen, "\\%s avatar.png", m_szProtoName );
}

int MSN_GetImageFormat(void* buf, const char** ext)
{
	int res;
	if ( *(unsigned short*)buf == 0xd8ff )
	{
		res =  PA_FORMAT_JPEG;
		*ext = "jpg"; 
	}
	else if ( *(unsigned short*)buf == 0x4d42 )
	{
		res = PA_FORMAT_BMP;
		*ext = "bmp"; 
	}
	else if ( *(unsigned*)buf == 0x474e5089 )
	{
		res = PA_FORMAT_PNG;
		*ext = "png"; 
	}
	else if ( *(unsigned*)buf == 0x38464947 )
	{
		res = PA_FORMAT_GIF;
		*ext = "gif"; 
	}
	else 
	{
		res = PA_FORMAT_UNKNOWN;
		*ext = "unk"; 
	}
	return res;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_GetCustomSmileyFileName - gets a file name for an contact's custom smiley

void  CMsnProto::MSN_GetCustomSmileyFileName( HANDLE hContact, char* pszDest, size_t cbLen, const char* SmileyName, int type )
{
	size_t tPathLen;

	InitCustomFolders();

	char* path = ( char* )alloca( cbLen );
	if ( hCustomSmileyFolder == NULL || FoldersGetCustomPath(hCustomSmileyFolder, path, cbLen, "" )) 
	{
		CallService(MS_DB_GETPROFILEPATH, (WPARAM) cbLen, (LPARAM)pszDest);
		tPathLen = strlen( pszDest );
		tPathLen += mir_snprintf(pszDest + tPathLen, cbLen - tPathLen, "\\%s\\CustomSmiley", m_szProtoName);
	}
	else {
		strcpy( pszDest, path );
		tPathLen = strlen( pszDest );
	}

	if ( hContact != NULL ) {
		char szEmail[ MSN_MAX_EMAIL_LEN ];
		if ( getStaticString( hContact, "e-mail", szEmail, sizeof( szEmail )))
			_ltoa(( long )hContact, szEmail, 10 );
		
		tPathLen += mir_snprintf( pszDest + tPathLen, cbLen - tPathLen, "\\%s", szEmail );
	}
	else 
		tPathLen += mir_snprintf( pszDest + tPathLen, cbLen - tPathLen, "\\%s", m_szProtoName );
		
	bool exist = _access(pszDest, 0) == 0;
	
	if (type == 0)
	{
		if (!exist) pszDest[0] = 0;
		return;
	}

	if (!exist)
		MSN_CallService( MS_UTILS_CREATEDIRTREE, 0, ( LPARAM )pszDest );

	mir_snprintf( pszDest + tPathLen, cbLen - tPathLen, "\\%s.%s", SmileyName,
		type == MSN_APPID_CUSTOMSMILEY ? "png" : "gif");
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_GoOffline - performs several actions when a server goes offline

void CMsnProto::MSN_GoOffline(void)
{
	if (m_iStatus == ID_STATUS_OFFLINE) return;

	if ( !Miranda_Terminated() )
	{
		int msnOldStatus = m_iStatus; m_iStatus = m_iDesiredStatus = ID_STATUS_OFFLINE; 
		SendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE)msnOldStatus, ID_STATUS_OFFLINE );
	}

	msnLoggedIn = false;

	mir_free(msnPreviousUUX);
	msnPreviousUUX = NULL;

	if ( !Miranda_Terminated() )
		MSN_EnableMenuItems( false );
	MSN_CloseConnections();
	MSN_FreeGroups();
	MsgQueue_Clear();
	clearCachedMsg();

	HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL )
	{
		if ( MSN_IsMyContact( hContact ))
			if ( ID_STATUS_OFFLINE != getWord( hContact, "Status", ID_STATUS_OFFLINE )) {
				setWord( hContact, "Status", ID_STATUS_OFFLINE );
				setDword( hContact, "IdleTS", 0 );
			}

		hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_SendMessage - formats and sends a MSG packet through the server

static const char sttHeaderStart[] = "MIME-Version: 1.0\r\n";

LONG ThreadData::sendMessage( int msgType, const char* email, int netId, const char* parMsg, int parFlags )
{
	char tHeader[ 1024 ];
	strcpy( tHeader, sttHeaderStart );

	if (( parFlags & MSG_DISABLE_HDR ) == 0 ) {
		char  tFontName[ 100 ], tFontStyle[ 3 ];
		DWORD tFontColor;

		strcpy( tFontName, "Arial" );

		if ( proto->getByte( "SendFontInfo", 1 )) {
			char* p;

			DBVARIANT dbv;
			if ( !DBGetContactSettingString( NULL, "SRMsg", "Font0", &dbv )) {
				for ( p = dbv.pszVal; *p; p++ )
					if ( BYTE( *p ) >= 128 || *p < 32 )
						break;

				if ( *p == 0 ) {
					UrlEncode( dbv.pszVal, tFontName, sizeof( tFontName ));
					MSN_FreeVariant( &dbv );
			}	}

			{	int  tStyle = DBGetContactSettingByte( NULL, "SRMsg", "Font0Sty", 0 );
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
			"X-MMS-IM-Format: FN=%s; EF=%s; CO=%x; CS=0; PF=31%s\r\n\r\n",
			tFontName, tFontStyle, tFontColor, (parFlags & MSG_RTL) ? ";RL=1" : "" );
	}

	int seq;
	if (netId == NETID_YAHOO)
		seq = sendPacket( "UUM", "%s %d %c %d\r\n%s%s", email, netId, msgType, 
			strlen( parMsg )+strlen( tHeader ), tHeader, parMsg );
	else
		seq = sendPacket( "MSG", "%c %d\r\n%s%s", msgType, 
			strlen( parMsg )+strlen( tHeader ), tHeader, parMsg );

	return seq;
}

void ThreadData::sendCaps( void )
{
	char mversion[100], capMsg[1000];
	MSN_CallService( MS_SYSTEM_GETVERSIONTEXT, sizeof( mversion ), ( LPARAM )mversion );

	mir_snprintf( capMsg, sizeof( capMsg ),
		"Content-Type: text/x-clientcaps\r\n\r\n"
		"Client-Name: Miranda IM %s (MSN v.%s)\r\n",
		mversion, __VERSION_STRING );

	sendMessage( 'U', NULL, 1, capMsg, MSG_DISABLE_HDR );
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

// Typing notifications support

void CMsnProto::MSN_SendTyping( ThreadData* info, const char* email, int netId  )
{
	char tCommand[ 1024 ];
	mir_snprintf( tCommand, sizeof( tCommand ),
		"Content-Type: text/x-msmsgscontrol\r\n"
		"TypingUser: %s\r\n\r\n\r\n", MyOptions.szEmail );

	info->sendMessage( netId == NETID_MSN ? 'U' : '2', email, netId, tCommand, MSG_DISABLE_HDR );
}


static ThreadData* FindThreadTimer(UINT timerId)
{
	ThreadData* res = NULL;
	for (int i = 0; i < g_Instances.getCount() && res == NULL; ++i)
		res = g_Instances[i].MSN_GetThreadByTimer( timerId );

	return res;
}

static VOID CALLBACK TypingTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) 
{
	ThreadData* T = FindThreadTimer( idEvent );
	if ( T != NULL )
		T->proto->MSN_SendTyping( T, NULL, 1 );
	else
		KillTimer( NULL, idEvent );
}	


void  CMsnProto::MSN_StartStopTyping( ThreadData* info, bool start )
{
	if ( start && info->mTimerId == 0 ) {
		info->mTimerId = SetTimer(NULL, 0, 5000, TypingTimerProc);
		MSN_SendTyping( info, NULL, 1 );
	}
	else if ( !start && info->mTimerId != 0 ) {
			KillTimer( NULL, info->mTimerId );
			info->mTimerId = 0;
	}
}


long CMsnProto::MSN_SendSMS(char* tel, char* txt)
{
	static const char *pgd = 
		"<TEXT xml:space=\"preserve\" enc=\"utf-8\">%s</TEXT>"
		"<LCID>%u</LCID><CS>iso-8859-1</CS>";

	char* etxt = HtmlEncode(txt);
	char pgda[1024];
	size_t sz = mir_snprintf(pgda, sizeof(pgda), pgd, etxt, langpref);
	mir_free(etxt);

	return msnNsThread->sendPacket("PGD", "%s 1 %u\r\n%s", tel, sz, pgda);
}


/////////////////////////////////////////////////////////////////////////////////////////
// MSN_SendStatusMessage - notify a server about the status message change

// Helper to process texts
static char * HtmlEncodeUTF8T( const TCHAR *src )
{
	if (src == NULL)
		return mir_strdup("");

	char *tmp = mir_utf8encodeT(src);
	char *ret = HtmlEncode(tmp);
	mir_free(tmp);
	return ret;
}

void  CMsnProto::MSN_SendStatusMessage( const char* msg )
{
	if ( !msnLoggedIn )
		return;

	char *tmp = mir_utf8encode(msg);
	char* msgEnc = HtmlEncode(( tmp == NULL ) ? "" : tmp );
	mir_free(tmp);

	size_t sz;
	char  szMsg[2048];
	if (msnCurrentMedia.cbSize == 0) 
		sz = mir_snprintf(szMsg, sizeof(szMsg), "<Data><PSM>%s</PSM><MachineGuid></MachineGuid></Data>", msgEnc);
	else 
	{
		char *szFormatEnc;
		if (ServiceExists(MS_LISTENINGTO_GETPARSEDTEXT)) {
			LISTENINGTOINFO lti = {0};
			lti.cbSize = sizeof(lti);
			if (msnCurrentMedia.ptszTitle != NULL) lti.ptszTitle = _T("{0}");
			if (msnCurrentMedia.ptszArtist != NULL) lti.ptszArtist = _T("{1}");
			if (msnCurrentMedia.ptszAlbum != NULL) lti.ptszAlbum = _T("{2}");
			if (msnCurrentMedia.ptszTrack != NULL) lti.ptszTrack = _T("{3}");
			if (msnCurrentMedia.ptszYear != NULL) lti.ptszYear = _T("{4}");
			if (msnCurrentMedia.ptszGenre != NULL) lti.ptszGenre = _T("{5}");
			if (msnCurrentMedia.ptszLength != NULL) lti.ptszLength = _T("{6}");
			if (msnCurrentMedia.ptszPlayer != NULL) lti.ptszPlayer = _T("{7}");
			if (msnCurrentMedia.ptszType != NULL) lti.ptszType = _T("{8}");

			TCHAR *tmp = (TCHAR *)CallService(MS_LISTENINGTO_GETPARSEDTEXT, (WPARAM) _T("%title% - %artist%"), (LPARAM) &lti );
			szFormatEnc = HtmlEncodeUTF8T(tmp);
			mir_free(tmp);
		} else {
			szFormatEnc = HtmlEncodeUTF8T(_T("{0} - {1}"));
		}

		char *szArtist = HtmlEncodeUTF8T( msnCurrentMedia.ptszArtist );
		char *szAlbum = HtmlEncodeUTF8T( msnCurrentMedia.ptszAlbum );
		char *szTitle = HtmlEncodeUTF8T( msnCurrentMedia.ptszTitle );
		char *szTrack = HtmlEncodeUTF8T( msnCurrentMedia.ptszTrack );
		char *szYear = HtmlEncodeUTF8T( msnCurrentMedia.ptszYear );
		char *szGenre = HtmlEncodeUTF8T( msnCurrentMedia.ptszGenre );
		char *szLength = HtmlEncodeUTF8T( msnCurrentMedia.ptszLength );
		char *szPlayer = HtmlEncodeUTF8T( msnCurrentMedia.ptszPlayer );
		char *szType = HtmlEncodeUTF8T( msnCurrentMedia.ptszType );

		sz = mir_snprintf( szMsg, sizeof szMsg, 
			"<Data>"
				"<PSM>%s</PSM>"
				"<CurrentMedia>%s\\0%s\\01\\0%s\\0%s\\0%s\\0%s\\0%s\\0%s\\0%s\\0%s\\0%s\\0%s\\0\\0</CurrentMedia>"
				"<MachineGuid></MachineGuid>"
			"</Data>", 
			msgEnc, szPlayer, szType, szFormatEnc, szTitle, szArtist, szAlbum, szTrack, szYear, szGenre, szLength, szPlayer, szType);

		mir_free( szArtist );
		mir_free( szAlbum );
		mir_free( szTitle );
		mir_free( szTrack );
		mir_free( szYear );
		mir_free( szGenre );
		mir_free( szLength );
		mir_free( szPlayer );
		mir_free( szType );
	}
	mir_free( msgEnc );

	if ( msnPreviousUUX == NULL || strcmp( msnPreviousUUX, szMsg ))
	{
		replaceStr( msnPreviousUUX, szMsg );
		msnNsThread->sendPacket( "UUX", "%d\r\n%s", sz, szMsg );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_SendPacket - sends a packet accordingly to the MSN protocol

LONG ThreadData::sendPacket( const char* cmd, const char* fmt,...)
{
	if (this == NULL) return 0;

	size_t strsize = 512;
	char* str = ( char* )mir_alloc( strsize );

	LONG thisTrid = 0;

	if ( fmt == NULL )
		mir_snprintf( str, strsize, "%s", cmd );
	else {
		thisTrid = MyInterlockedIncrement( &mTrid );
		if ( fmt[0] == '\0' )
			mir_snprintf( str, strsize, "%s %d", cmd, thisTrid );
		else {
			va_list vararg;
			va_start( vararg, fmt );

			int paramStart = mir_snprintf( str, strsize, "%s %d ", cmd, thisTrid );
			while ( _vsnprintf( str+paramStart, strsize-paramStart-3, fmt, vararg ) == -1 )
				str = (char*)mir_realloc( str, strsize += 512 );

			str[strsize-3] = 0;
			va_end( vararg );
		}
	}

	if ( strchr( str, '\r' ) == NULL )
		strcat( str,"\r\n" );

	int result = send( str, strlen( str ));
	mir_free( str );
	return ( result > 0 ) ? thisTrid : -1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_SetServerStatus - changes plugins status at the server

void  CMsnProto::MSN_SetServerStatus( int newStatus )
{
	MSN_DebugLog( "Setting MSN server status %d, logged in = %d", newStatus, msnLoggedIn );

	if ( !msnLoggedIn )
		return;

	const char* szStatusName = MirandaStatusToMSN( newStatus  );

	if ( newStatus != ID_STATUS_OFFLINE ) {
		char szMsnObject[ 1000 ];
		if ( !ServiceExists( MS_AV_SETMYAVATAR ) || 
			 getStaticString( NULL, "PictObject", szMsnObject, sizeof( szMsnObject )))
			szMsnObject[ 0 ] = 0;

		// Capabilties: WLM 8.1, Chunking 
		unsigned flags = 0x80000020;
		if (getByte( "MobileEnabled", 0) && getByte( "MobileAllowed", 0))
			flags |= 0x40;

		msnNsThread->sendPacket( "CHG", "%s %u %s", szStatusName, flags, szMsnObject );

		unsigned status = newStatus == ID_STATUS_IDLE ? ID_STATUS_ONLINE : newStatus;

		char** msgptr = GetStatusMsgLoc(status);
		if ( msgptr != NULL )
			MSN_SendStatusMessage( *msgptr );
	}
	else msnNsThread->sendPacket( "CHG", szStatusName );
}


/////////////////////////////////////////////////////////////////////////////////////////
// Display Hotmail Inbox thread

void CMsnProto::MsnInvokeMyURL( bool ismail, char* url )
{
	const char* requrl = url ? url : (ismail ? rru : "http://spaces.live.com");  
	const char* id = ismail ? urlId : "73625";

	char* hippy = NULL;
	if (passport && requrl && id)
	{
		static const char postdataM[] = "ct=%u&bver=4&id=%s&rru=%s&svc=mail&js=yes&pl=%%3Fid%%3D%s";
		static const char postdataS[] = "ct=%u&bver=4&id=%s&ru=%s&js=yes&pl=%%3Fid%%3D%s";
		const char *postdata = ismail ? postdataM : postdataS;
	  
		char rruenc[256];
		UrlEncode(requrl, rruenc, sizeof(rruenc));
 
		const size_t fnpstlen = strlen(postdata) +  strlen(rruenc) + 2*strlen(id) + 30;
		char* fnpst = (char*)alloca(fnpstlen);

		mir_snprintf(fnpst, fnpstlen, postdata, time(NULL), id, rruenc, id);

		char* post = HotmailLogin(fnpst);
		if (post)
		{
			size_t hipsz = strlen(passport) + 3*strlen(post) + 50;
			hippy = (char*)alloca(hipsz);

			strcpy(hippy, passport);
			char* ch = strstr(hippy, "md5auth");
			if (ch)
			{
				memmove(ch + 1, ch, strlen(ch)+1);
				memcpy(ch, "sha1", 4);  
			}
			strcat(hippy, "&token="); 
			size_t hiplen = strlen(hippy);
			UrlEncode(post, hippy+hiplen, hipsz-hiplen);
			mir_free(post);
		}
	}
	if (hippy == NULL) hippy = (char*)(ismail ? "http://mail.live.com" : "http://spaces.live.com");

	MSN_DebugLog( "Starting URL: '%s'", hippy );
	MSN_CallService( MS_UTILS_OPENURL, 1, ( LPARAM )hippy );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_ShowError - shows an error

void CMsnProto::MSN_ShowError( const char* msgtext, ... )
{
	char    tBuffer[ 4096 ];
	va_list tArgs;

	va_start( tArgs, msgtext );
	mir_vsnprintf( tBuffer, sizeof( tBuffer ), MSN_Translate( msgtext ), tArgs );
	va_end( tArgs );

	TCHAR* buf1 = mir_a2t( m_szProtoName );
	TCHAR* buf2 = mir_a2t( tBuffer );

	MSN_ShowPopup( buf1, buf2, MSN_ALLOW_MSGBOX | MSN_SHOW_ERROR, NULL );

	mir_free(buf1);
	mir_free(buf2);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Popup plugin window proc

LRESULT CALLBACK NullWindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message ) {
		case WM_COMMAND: {
			PopupData* tData = ( PopupData* )PUGetPluginData( hWnd );
			if ( tData != NULL )
			{
				if ( tData->flags & MSN_HOTMAIL_POPUP )
				{
					if ( tData->flags & MSN_ALLOW_ENTER )
						tData->proto->MsnInvokeMyURL( true, tData->url );
				}
				else
				{
					if ( tData->url != NULL )
						MSN_CallService( MS_UTILS_OPENURL, 1, ( LPARAM )tData->url );
				}
			}

			PUDeletePopUp( hWnd );
			break;
		}

		case WM_CONTEXTMENU:
			PUDeletePopUp( hWnd );
			break;

		case UM_FREEPLUGINDATA:	{
			PopupData* tData = ( PopupData* )PUGetPluginData( hWnd );
			if ( tData != NULL && tData != (void*)CALLSERVICE_NOTFOUND)
			{
				CallService( MS_SKIN2_RELEASEICON, (WPARAM)tData->hIcon, 0 );
				mir_free( tData->url );
				mir_free( tData );
			}
			break;
		}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////////////////
// MSN_ShowPopup - popup plugin support

void CALLBACK sttMainThreadCallback( PVOID dwParam )
{
	LPPOPUPDATAT ppd = ( LPPOPUPDATAT )dwParam;
	PopupData* pud = ( PopupData* )ppd->PluginData;

	bool iserr = (pud->flags & MSN_SHOW_ERROR) != 0;
	if ((iserr && !pud->proto->MyOptions.ShowErrorsAsPopups) || PUAddPopUpT(ppd) == CALLSERVICE_NOTFOUND) 
	{
		if ( pud->flags & MSN_ALLOW_MSGBOX ) 
		{
			TCHAR szMsg[ MAX_SECONDLINE + MAX_CONTACTNAME ];
			mir_sntprintf( szMsg, SIZEOF( szMsg ), _T("%s:\n%s"), ppd->lptzContactName, ppd->lptzText );
			MessageBox( NULL, szMsg, _T("MSN Protocol"), 
				MB_OK | (iserr ? MB_ICONERROR : MB_ICONINFORMATION));
		}
		CallService( MS_SKIN2_RELEASEICON, (WPARAM)pud->hIcon, 0 );
		mir_free( pud->url );
		mir_free( pud );
	}
	mir_free( ppd );
}

void CMsnProto::MSN_ShowPopup( const TCHAR* nickname, const TCHAR* msg, int flags, const char* url, HANDLE hContact )
{
	LPPOPUPDATAT ppd = ( LPPOPUPDATAT )mir_calloc( sizeof( POPUPDATAT ));

	ppd->lchContact = hContact;
	_tcsncpy( ppd->lptzContactName, nickname, MAX_CONTACTNAME );
	ppd->lptzContactName[MAX_CONTACTNAME-1] = 0;
	_tcsncpy( ppd->lptzText, msg, MAX_SECONDLINE );
	ppd->lptzText[MAX_SECONDLINE-1] = 0;

	if ( flags & MSN_SHOW_ERROR ) {
		ppd->lchIcon = ( HICON )LoadImage( NULL, IDI_WARNING, IMAGE_ICON, 0, 0, LR_SHARED );
		ppd->colorBack = RGB(191, 0, 0); //Red
		ppd->colorText = RGB(255, 245, 225); //Yellow
		ppd->iSeconds  = 60;
	}
	else {
		ppd->lchIcon = LoadIconEx( "main" );
		ppd->colorBack = ( MyOptions.UseWinColors ) ? GetSysColor( COLOR_BTNFACE ) : MyOptions.BGColour;
		ppd->colorText = ( MyOptions.UseWinColors ) ? GetSysColor( COLOR_WINDOWTEXT ) : MyOptions.TextColour;
		ppd->iSeconds = ( flags & ( MSN_HOTMAIL_POPUP | MSN_ALERT_POPUP )) ? 
			MyOptions.PopupTimeoutHotmail : MyOptions.PopupTimeoutOther;
	}

	ppd->PluginWindowProc = ( WNDPROC )NullWindowProc;
	ppd->PluginData = mir_alloc( sizeof( PopupData ));

	PopupData* pud = ( PopupData* )ppd->PluginData;
	pud->flags = flags;
	pud->hIcon = ppd->lchIcon;
	pud->url = mir_strdup( url );
	pud->proto = this;

	CallFunctionAsync( sttMainThreadCallback, ppd );
}


void CMsnProto::MSN_ShowPopup( const HANDLE hContact, const TCHAR* msg, int flags )
{
	const TCHAR* nickname = hContact ? MSN_GetContactNameT( hContact ) : _T("Me");
	MSN_ShowPopup( nickname, msg, flags, NULL, hContact );
}


/////////////////////////////////////////////////////////////////////////////////////////
// filetransfer class members

filetransfer::filetransfer(CMsnProto* prt)
{
	memset( this, 0, sizeof( filetransfer ));
	fileId = -1;
	std.cbSize = sizeof( std );
	proto = prt;

	hLockHandle = CreateMutex( NULL, FALSE, NULL );
}

filetransfer::~filetransfer( void )
{
	proto->MSN_DebugLog( "Destroying file transfer session %08X", p2p_sessionid );

	WaitForSingleObject( hLockHandle, 2000 );
	CloseHandle( hLockHandle );

	if ( !bCompleted ) {
		std.files = NULL;
		std.totalFiles = 0;
		proto->SendBroadcast( std.hContact, ACKTYPE_FILE, ACKRESULT_FAILED, this, 0);
	}

	if ( fileId != -1 )
		_close( fileId );

	mir_free( p2p_branch );
	mir_free( p2p_callID );
	mir_free( p2p_dest );
	mir_free( p2p_object );

	mir_free( std.currentFile );
	mir_free( std.workingDir );
	if ( std.files != NULL ) {
		for ( int i=0; i < std.totalFiles; i++ )
			mir_free( std.files[ i ] );
		mir_free( std.files );
	}

	mir_free( wszFileName );
	mir_free( szInvcookie );
}

void filetransfer::close( void )
{
	if ( fileId != -1 ) _close( fileId );
	fileId = -1;
}

void filetransfer::complete( void )
{
	close();

	bCompleted = true;
	proto->SendBroadcast( std.hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, this, 0);
}

int filetransfer::create( void )
{
	#if defined( _UNICODE )	
		if ( wszFileName != NULL ) {
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
					char filefull[ MAX_PATH ];
					mir_snprintf( filefull, sizeof( filefull ), "%s\\%s", std.workingDir, tShortName );
					std.currentFile = mir_strdup( filefull );
					FindClose( hFind );
			}	}
		}
		else fileId = _open( std.currentFile, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE );
	#else
		fileId = _open( std.currentFile, _O_BINARY | _O_CREAT | _O_TRUNC | _O_WRONLY, _S_IREAD | _S_IWRITE );
	#endif

	if ( fileId == -1 )
		proto->MSN_ShowError( "Cannot create file '%s' during a file transfer", std.currentFile );
//	else if ( std.currentFileSize != 0 )
//		_chsize( fileId, std.currentFileSize );

	return fileId;
}

int filetransfer::openNext( void )
{
	if ( fileId != -1 ) {
		close();
		++std.currentFileNumber;
	}

	if ( std.currentFileNumber < std.totalFiles) {
		bCompleted = false;
		replaceStr(std.currentFile, std.files[std.currentFileNumber] );
		fileId = _open( std.currentFile, _O_BINARY | _O_RDONLY, _S_IREAD );
		if ( fileId != -1 ) {
			std.currentFileSize = _filelength( fileId );
			std.currentFileProgress = 0;
			
			p2p_sendmsgid = 0;
			p2p_byemsgid = 0;
			tType = SERVER_DISPATCH;

			mir_free( p2p_branch ); p2p_branch = NULL;
			mir_free( p2p_callID ); p2p_callID = NULL;
		}
		else
			proto->MSN_ShowError( "Unable to open file '%s' for the file transfer, error %d", std.currentFile, errno );
	}

	return fileId;
}

directconnection::directconnection(filetransfer* ft)
{
	memset( this, 0, sizeof( directconnection ));

	callId = mir_strdup( ft->p2p_callID );
	mNonce = ( UUID* )mir_alloc( sizeof( UUID ));
	UuidCreate( mNonce );
	ts = time( NULL );
}

directconnection::~directconnection()
{
	mir_free( callId );
	mir_free( mNonce );
	mir_free( xNonce );
}


char* directconnection::calcHashedNonce(UUID* nonce)
{
	mir_sha1_ctx sha1ctx;
	BYTE sha[ MIR_SHA1_HASH_SIZE ];

	mir_sha1_init( &sha1ctx );
	mir_sha1_append( &sha1ctx, ( BYTE* )nonce, sizeof( UUID ));
	mir_sha1_finish( &sha1ctx, sha );

	char* p;
	UuidToStringA(( UUID* )&sha, ( BYTE** )&p );
	size_t len = strlen( p ) + 3;
	char* result = ( char* )mir_alloc( len );
	mir_snprintf(result, len, "{%s}", p );
	_strupr( result );
	RpcStringFreeA(( BYTE** )&p );

	return result;
}

char* directconnection::mNonceToText( void )
{
	char* p;
	UuidToStringA( mNonce, ( BYTE** )&p );
	size_t len = strlen( p ) + 3;
	char* result = ( char* )mir_alloc( len );
	mir_snprintf(result, len, "{%s}", p );
	_strupr( result );
	RpcStringFreeA(( BYTE** )&p );

	return result;
}


void directconnection::xNonceToBin( UUID* nonce )
{
	size_t len = strlen( xNonce );
	char *p = ( char* )alloca( len );
	strcpy( p, xNonce + 1 );
	p[len-2] = 0;
	UuidFromStringA(( BYTE* )p, nonce );
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
	mir_free( mErrorText );
}

char* TWinErrorCode::getText()
{
	if ( mErrorText == NULL )
	{
		int tBytes = 0;
		mErrorText = (char*)mir_alloc(256);

		if ( mErrorCode >= 12000 && mErrorCode < 12500 )
			tBytes = FormatMessageA(
				FORMAT_MESSAGE_FROM_HMODULE,
				GetModuleHandleA( "WININET.DLL" ),
				mErrorCode, LANG_NEUTRAL, mErrorText, 256, NULL );

		if ( tBytes == 0 )
			tBytes = FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM, NULL,
				mErrorCode, LANG_NEUTRAL, mErrorText, 256, NULL );

		if ( tBytes == 0 )
		{
			tBytes = mir_snprintf( mErrorText, 256, "unknown Windows error code %d", mErrorCode );
		}

		*mErrorText = (char)tolower( *mErrorText );

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

// Process a string, and double all % characters, according to chat.dll's restrictions
// Returns a pointer to the new string (old one is not freed)
TCHAR* EscapeChatTags(const TCHAR* pszText)
{
	int nChars = 0;
	for ( const TCHAR* p = pszText; ( p = _tcschr( p, '%' )) != NULL; p++ )
		nChars++;

	if ( nChars == 0 )
		return mir_tstrdup( pszText );

	TCHAR *pszNewText = (TCHAR*)mir_alloc( sizeof(TCHAR)*(lstrlen( pszText ) + 1 + nChars));
	if ( pszNewText == NULL )
		return mir_tstrdup( pszText );

	const TCHAR *s = pszText;
	TCHAR *d = pszNewText;
	while ( *s ) {
		if ( *s == '%' )
			*d++ = '%';
		*d++ = *s++;
	}
	*d = 0;
	return pszNewText;
}

TCHAR* UnEscapeChatTags(TCHAR* str_in)
{
	TCHAR *s = str_in, *d = str_in;
	while ( *s ) {
		if (( *s == '%' && s[1] == '%' ) || ( *s == '\n' && s[1] == '\n' ))
			s++;
		*d++ = *s++;
	}
	*d = 0;
	return str_in;
}

char* MSN_Base64Decode( const char* str )
{
	if ( str == NULL ) return NULL; 

	size_t len = strlen( str );
	size_t reslen = Netlib_GetBase64DecodedBufferSize(len) + 4;
	char* res = ( char* )mir_alloc( reslen );

	char* p = const_cast< char* >( str );
	if ( len & 3 ) { // fix for stupid Kopete's base64 encoder
		char* p1 = ( char* )alloca( len+5 );
		memcpy( p1, p, len );
		p = p1;
		p1 += len; 
		for ( int i = 4 - (len & 3); i > 0; i--, p1++, len++ )
			*p1 = '=';
		*p1 = 0;
	}

	NETLIBBASE64 nlb = { p, len, ( PBYTE )res, reslen };
	MSN_CallService( MS_NETLIB_BASE64DECODE, 0, LPARAM( &nlb ));
	res[nlb.cbDecoded] = 0;

	return res;
}

bool CMsnProto::MSN_IsMyContact( HANDLE hContact )
{
	const char* szProto = ( char* )MSN_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact, 0 );
	return szProto != NULL && strcmp( m_szProtoName, szProto ) == 0;
}

bool CMsnProto::MSN_IsMeByContact( HANDLE hContact, char* szEmail )
{
	char tEmail[ MSN_MAX_EMAIL_LEN ];
	char *emailPtr = szEmail ? szEmail : tEmail;

	*emailPtr = 0;
	if (getStaticString( hContact, "e-mail", emailPtr, sizeof( tEmail ))) 
		return false;
	
	return strcmp(emailPtr, MyOptions.szEmail) == 0;
}


void MSN_MakeDigest(const char* chl, char* dgst)
{
	//Digest it
	DWORD md5hash[ 4 ], md5hashOr[ 4 ];
	mir_md5_state_t context;
	mir_md5_init( &context );
	mir_md5_append( &context, ( BYTE* )chl, strlen( chl ));
	mir_md5_append( &context, ( BYTE* )msnProtChallenge,  strlen( msnProtChallenge ));
	mir_md5_finish( &context, ( BYTE* )md5hash );

	memcpy(md5hashOr, md5hash, sizeof(md5hash));

	size_t i;
	for ( i=0; i < 4; i++ )
		md5hash[i] &= 0x7FFFFFFF;

	char chlString[128];
	mir_snprintf( chlString, sizeof( chlString ), "%s%s00000000", chl, msnProductID );
	chlString[ (strlen(chl)+strlen(msnProductID)+7) & 0xF8 ] = 0;

	LONGLONG high=0, low=0;
	int* chlStringArray = ( int* )chlString;
	for ( i=0; i < strlen( chlString )/4; i += 2) {
		LONGLONG temp = chlStringArray[i];

		temp = (0x0E79A9C1 * temp) % 0x7FFFFFFF;
		temp += high;
		temp = md5hash[0] * temp + md5hash[1];
		temp = temp % 0x7FFFFFFF;

		high = chlStringArray[i + 1];
		high = (high + temp) % 0x7FFFFFFF;
		high = md5hash[2] * high + md5hash[3];
		high = high % 0x7FFFFFFF;

		low = low + high + temp;
	}
	high = (high + md5hash[1]) % 0x7FFFFFFF;
	low = (low + md5hash[3]) % 0x7FFFFFFF;

	md5hashOr[0] ^= high;
	md5hashOr[1] ^= low;
	md5hashOr[2] ^= high;
	md5hashOr[3] ^= low;

	char* str = arrayToHex((PBYTE)md5hashOr, sizeof(md5hashOr));
	strcpy(dgst, str);
	mir_free(str);
}
