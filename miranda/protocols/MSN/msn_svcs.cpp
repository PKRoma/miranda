/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol. 
Copyright(C) 2002-2004 George Hazan (modification) and Richard Hughes (original)

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

#include <io.h>
#include <sys/stat.h>

#include "resource.h"
extern "C"
{
	#include "sha1.h"
}

void __cdecl MSNServerThread( ThreadData* info );

extern	HINSTANCE hInst;

HANDLE msnSetNicknameMenuItem = NULL;

/////////////////////////////////////////////////////////////////////////////////////////
// MsnAddToList - adds contact to the server list

static HANDLE AddToListByEmail( const char *email, DWORD flags )
{
	char urlEmail[ 255 ], *szProto;
	UrlEncode( email, urlEmail, sizeof( urlEmail ));

	//check not already on list
	for ( HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		   hContact != NULL;
			hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 ))
	{
		szProto = ( char* )MSN_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact, 0 );
		if ( szProto != NULL && !strcmp( szProto, msnProtocolName )) {
			char tEmail[ MSN_MAX_EMAIL_LEN ];
			if ( MSN_GetStaticString( "e-mail", hContact, tEmail, sizeof( tEmail )))
				continue;

			if ( strcmp( tEmail, email ))
				continue;

			if ( !( flags & PALF_TEMPORARY ) && DBGetContactSettingByte( hContact, "CList", "NotOnList", 1 )) {
				DBDeleteContactSetting( hContact, "CList", "NotOnList" );
				DBDeleteContactSetting( hContact, "CList", "Hidden" );
LBL_AddContact:
				if ( msnLoggedIn ) {
					MSN_AddUser( hContact, urlEmail, LIST_FL );
					MSN_AddUser( hContact, urlEmail, LIST_BL + LIST_REMOVE );
					MSN_AddUser( hContact, urlEmail, LIST_AL );
				}
				else hContact = NULL;
			}

			return hContact;
	}	}

	//not already there: add
	hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_ADD, 0, 0 );
	MSN_CallService( MS_PROTO_ADDTOCONTACT, ( WPARAM )hContact,( LPARAM )msnProtocolName );
	MSN_SetString( hContact, "e-mail", email );

	if ( !( flags & PALF_TEMPORARY ))
		goto LBL_AddContact;
	
	DBWriteContactSettingByte( hContact, "CList", "NotOnList", 1 );
	DBWriteContactSettingByte( hContact, "CList", "Hidden", 1 );
	return hContact;
}

static int MsnAddToList(WPARAM wParam,LPARAM lParam)
{
	PROTOSEARCHRESULT *psr=(PROTOSEARCHRESULT*)lParam;

	if ( psr->cbSize != sizeof( PROTOSEARCHRESULT ))
		return NULL;

	return ( int )AddToListByEmail( psr->email, wParam );
}

static int MsnAddToListByEvent(WPARAM wParam, LPARAM lParam)
{
	DBEVENTINFO dbei = {0};
	dbei.cbSize = sizeof(dbei);
	if (( dbei.cbBlob = MSN_CallService( MS_DB_EVENT_GETBLOBSIZE, lParam, 0 )) == (DWORD)(-1))
		return 0;

	dbei.pBlob=(PBYTE) alloca(dbei.cbBlob);
	if ( CallService(MS_DB_EVENT_GET, lParam, (LPARAM) &dbei))	return 0;
	if ( strcmp(dbei.szModule, msnProtocolName))						return 0;
	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST)					return 0;

	char* nick = (char *) (dbei.pBlob + sizeof(DWORD) + sizeof(HANDLE));
	char* firstName = nick + strlen(nick) + 1;
	char* lastName = firstName + strlen(firstName) + 1;
	char* email = lastName + strlen(lastName) + 1;

	return ( int ) AddToListByEmail( email, 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnAuthAllow - called after successful authorization

static int MsnAuthAllow(WPARAM wParam,LPARAM lParam)
{
	if ( !msnLoggedIn )
		return 1;

	DBEVENTINFO dbei;
	memset( &dbei, 0, sizeof( dbei ));
	dbei.cbSize = sizeof( dbei );

	if (( dbei.cbBlob = MSN_CallService( MS_DB_EVENT_GETBLOBSIZE, wParam, 0 )) == -1 )
		return 1;

	dbei.pBlob = ( PBYTE )alloca( dbei.cbBlob );
	if ( MSN_CallService( MS_DB_EVENT_GET, wParam, ( LPARAM )&dbei ))
		return 1;

	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST )
		return 1;

	if ( strcmp( dbei.szModule, msnProtocolName ))
		return 1;

	char* nick = ( char* )( dbei.pBlob + sizeof( DWORD )*2 );
	char* firstName = nick + strlen( nick ) + 1;
	char* lastName = firstName + strlen( firstName ) + 1;
	char* email = lastName + strlen( lastName ) + 1;

	char urlNick[388],urlEmail[130];

	char* nickutf = Utf8Encode( nick );
	UrlEncode( nickutf, urlNick, sizeof( urlNick ));
	UrlEncode( email, urlEmail, sizeof( urlEmail ));

	free( nickutf );
	AddToListByEmail( email, 0 );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnAuthDeny - called after unsuccessful authorization

static int MsnAuthDeny(WPARAM wParam,LPARAM lParam)
{
	if ( !msnLoggedIn )
		return 1;

	DBEVENTINFO dbei;
	memset( &dbei, 0, sizeof( dbei ));
	dbei.cbSize = sizeof( dbei );

	if (( dbei.cbBlob = MSN_CallService( MS_DB_EVENT_GETBLOBSIZE, wParam, 0 )) == -1 )
		return 1;

	dbei.pBlob = ( PBYTE )alloca( dbei.cbBlob );
	if ( MSN_CallService( MS_DB_EVENT_GET, wParam, ( LPARAM )&dbei ))
		return 1;

	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST )
		return 1;

	if ( strcmp( dbei.szModule, msnProtocolName ))
		return 1;

	char* nick = ( char* )( dbei.pBlob + sizeof( DWORD )*2 );
	char* firstName = nick + strlen( nick ) + 1;
	char* lastName = firstName + strlen( firstName ) + 1;
	char* email = lastName + strlen( lastName ) + 1;

	char urlNick[388],urlEmail[130];

	char* nickutf = Utf8Encode( nick );
	UrlEncode( nickutf, urlNick, sizeof( urlNick ));
	UrlEncode( email, urlEmail, sizeof( urlEmail ));

	MSN_AddUser( NULL, urlEmail, LIST_BL );
	free( nickutf );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnBasicSearch - search contacts by e-mail

static int MsnBasicSearch(WPARAM wParam,LPARAM lParam)
{
	if ( !msnLoggedIn || msnSearchID != -1 )
		return 0;

	char tEmail[ 256 ];
	if ( !MSN_GetStaticString( "e-mail", NULL, tEmail, sizeof tEmail )) {
		if ( !stricmp(( char* )lParam, tEmail )) {
			MSN_ShowError( "You cannot add yourself to the contact list" );
			return 0;
	}	}

	UrlEncode(( char* )lParam, tEmail, sizeof( tEmail ));

	if ( msnProtVersion < 10 ) 
		msnSearchID = MSN_SendPacket( msnNSSocket, "ADD", "BL %s %s", tEmail, tEmail );
	else
		msnSearchID = MSN_SendPacket( msnNSSocket, "ADC", "BL N=%s", tEmail );
	return msnSearchID;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Block command callback function

static int MsnBlockCommand( WPARAM wParam, LPARAM lParam )
{
	if ( msnLoggedIn ) {
		char tEmail[ MSN_MAX_EMAIL_LEN ];
		if ( !MSN_GetStaticString( "e-mail", ( HANDLE )wParam, tEmail, sizeof( tEmail )))
			MSN_SetWord(( HANDLE )wParam, "ApparentMode", ( Lists_IsInList( LIST_BL, tEmail )) ? 0 : ID_STATUS_OFFLINE );
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnContactDeleted - called when a contact is deleted from list

static int MsnContactDeleted( WPARAM wParam, LPARAM lParam )
{
	if ( !msnLoggedIn )  //should never happen for MSN contacts
		return 0;

	char* szProto = ( char* )MSN_CallService( MS_PROTO_GETCONTACTBASEPROTO, wParam, 0 );
	if ( szProto == NULL || strcmp( szProto, msnProtocolName )) 
		return 0;

	HANDLE hContact = ( HANDLE )wParam;
	char tEmail[ MSN_MAX_EMAIL_LEN ];
	if ( !MSN_GetStaticString( "e-mail", hContact, tEmail, sizeof( tEmail ))) {
		MSN_AddUser( hContact, tEmail, LIST_FL | LIST_REMOVE );
		MSN_AddUser( hContact, tEmail, LIST_AL | LIST_REMOVE );

		if ( !Lists_IsInList( LIST_RL, tEmail )) {
			MSN_AddUser( hContact, tEmail, LIST_BL | LIST_REMOVE );
			Lists_Remove( 0xFF, tEmail );
		}
		else MSN_AddUser( hContact, tEmail, LIST_BL );
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnDbSettingChanged - look for contact's settings changes

static int MsnDbSettingChanged(WPARAM wParam,LPARAM lParam)
{
	HANDLE hContact = ( HANDLE )wParam;
	DBCONTACTWRITESETTING* cws = ( DBCONTACTWRITESETTING* )lParam;

	if ( hContact == NULL || !msnLoggedIn )
		return 0;

	if ( !strcmp( cws->szSetting, "ApparentMode" )) {
		char tEmail[ MSN_MAX_EMAIL_LEN ];
		if ( !MSN_GetStaticString( "e-mail", hContact, tEmail, sizeof( tEmail ))) {
			int isBlocked = Lists_IsInList( LIST_BL, tEmail );

			if ( !isBlocked && cws->value.wVal == ID_STATUS_OFFLINE ) {
				MSN_AddUser( hContact, tEmail, LIST_AL + LIST_REMOVE );
				MSN_AddUser( hContact, tEmail, LIST_BL );
			}
			else if ( isBlocked && cws->value.wVal == 0 ) {
				MSN_AddUser( hContact, tEmail, LIST_BL + LIST_REMOVE );
				MSN_AddUser( hContact, tEmail, LIST_AL );
	}	}	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnEditProfile - goes to the Profile section at the Hotmail.com

static int MsnEditProfile( WPARAM, LPARAM )
{
	char tUrl[ 4096 ];
	_snprintf( tUrl, sizeof( tUrl ), "http://members.msn.com/Edit.asp?did=1&t=%s&js=yes", MSPAuth );
	MSN_CallService( MS_UTILS_OPENURL, TRUE, ( LPARAM )tUrl );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnFileAllow - starts the file transfer

int MsnFileAllow(WPARAM wParam, LPARAM lParam)
{
	if ( !msnLoggedIn )
		return 0;

	CCSDATA* ccs = ( CCSDATA* )lParam;
	filetransfer* ft = ( filetransfer* )ccs->wParam;

	ThreadData* thread = MSN_GetThreadByContact( ccs->hContact );
	if ( thread != NULL && thread->ft != NULL ) {
		if ( ft->p2p_appID == 0 ) {
			MSN_SendPacket( thread->s, "MSG",
				"U %d\r\nMIME-Version: 1.0\r\n"
				"Content-Type: text/x-msmsgsinvite; charset=UTF-8\r\n\r\n"
				"Invitation-Command: ACCEPT\r\n"
				"Invitation-Cookie: %s\r\n"
				"Launch-Application: FALSE\r\n"
				"Request-Data: IP-Address:\r\n\r\n",
				172+4+strlen( thread->ft->szInvcookie ), thread->ft->szInvcookie );
		}
		else {
			p2p_sendAck( ft, &ft->p2p_hdr );
			ft->p2p_msgid -= 3;

			//---- send 200 OK Message 
			p2p_sendStatus( ft, 200 );
	}	}

	ft->std.workingDir = strdup((char *)ccs->lParam);
	return int( ft );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnFileCancel - cancels the active file transfer

int MsnFileCancel(WPARAM wParam, LPARAM lParam)
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	filetransfer* ft = ( filetransfer* )ccs->wParam;

	ft->bCanceled = true;
	if ( ft->p2p_appID != 0 )
		p2p_sendStatus( ft, -1 );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnFileDeny - rejects the file transfer request

int MsnFileDeny( WPARAM wParam, LPARAM lParam )
{
	if ( !msnLoggedIn )
		return 1;
	
	CCSDATA* ccs = ( CCSDATA* )lParam;
	filetransfer* ft = ( filetransfer* )ccs->wParam;

	ThreadData* thread = MSN_GetThreadByContact( ccs->hContact );
	if ( thread != NULL && thread->ft != NULL ) {
		if ( ft->p2p_appID == 0 ) {
			MSN_SendPacket( thread->s,"MSG",
				"U %d\r\nMIME-Version: 1.0\r\n"
				"Content-Type: text/x-msmsgsinvite; charset=UTF-8\r\n\r\n"
				"Invitation-Command: CANCEL\r\n"
				"Invitation-Cookie: %s\r\n"
				"Cancel-Code: REJECT\r\n\r\n",
				172-33+4+strlen( thread->ft->szInvcookie ), thread->ft->szInvcookie );
		}
		else {
			p2p_sendAck( ft, &ft->p2p_hdr );
			ft->p2p_msgid -= 3;

			//---- send 603 DECLINE Message 
			p2p_sendStatus( ft, 603 );
	}	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetAvatarInfo - retrieve the avatar info

static int MsnGetAvatarInfo(WPARAM wParam,LPARAM lParam)
{
	if ( msnProtVersion < 10 )
		return GAIR_NOAVATAR;

	PROTO_AVATAR_INFORMATION* AI = ( PROTO_AVATAR_INFORMATION* )lParam;

	char szContext[ MAX_PATH ];
	if ( MSN_GetStaticString(( AI->hContact == NULL ) ? "PictObject" : "PictContext", AI->hContact, szContext, sizeof szContext ))
		return GAIR_NOAVATAR;

	MSN_GetAvatarFileName( AI->hContact, AI->filename, sizeof AI->filename );
	AI->format = ( AI->hContact == NULL ) ? PA_FORMAT_PNG : PA_FORMAT_BMP;

	if ( ::access( AI->filename, 0 ) == 0 ) {
		char szSavedContext[ 256 ];
		if ( !MSN_GetStaticString( "PictSavedContext", AI->hContact, szSavedContext, sizeof szSavedContext ))
			if ( !strcmp( szSavedContext, szContext ))
				return GAIR_SUCCESS;
	}

	if (( wParam & GAIF_FORCE ) != 0 && AI->hContact != NULL ) {
		p2p_session( AI->hContact );
		return GAIR_WAITFOR;
	}

	return GAIR_NOAVATAR;	
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetCaps - obtain the protocol capabilities

static int MsnGetCaps(WPARAM wParam,LPARAM lParam)
{
	switch( wParam ) {
	case PFLAGNUM_1:
		return PF1_IM | PF1_SERVERCLIST | PF1_AUTHREQ | PF1_BASICSEARCH | 
				 PF1_ADDSEARCHRES | PF1_SEARCHBYEMAIL | PF1_USERIDISEMAIL | 
				 PF1_FILESEND | PF1_FILERECV | PF1_URLRECV | PF1_VISLIST;

	case PFLAGNUM_2:
		return PF2_ONLINE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_LIGHTDND | 
				 PF2_ONTHEPHONE | PF2_OUTTOLUNCH | PF2_INVISIBLE;

	case PFLAGNUM_3:
		return PF2_SHORTAWAY | PF2_LONGAWAY | PF2_LIGHTDND | PF2_HEAVYDND | 
				 PF2_FREECHAT | PF2_ONTHEPHONE | PF2_OUTTOLUNCH;

	case PFLAGNUM_4:
		return PF4_SUPPORTTYPING | PF4_AVATARS;

	case PFLAG_UNIQUEIDTEXT:
		return ( int )MSN_Translate( "E-mail address" );
	
	case PFLAG_UNIQUEIDSETTING:
		return ( int )"e-mail";
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetInfo - nothing to do, cause we cannot obtain information from the server

static int MsnGetInfo(WPARAM wParam,LPARAM lParam)
{
	if ( !msnLoggedIn )
		return 0;

	CCSDATA *ccs=(CCSDATA*)lParam;
	msnGetInfoContact = ccs->hContact;
	MSN_WS_Send( msnNSSocket, "PNG\r\n", 5 );
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetName - obtain the protocol name

static int MsnGetName( WPARAM wParam, LPARAM lParam )
{
	lstrcpyn(( char* )lParam, msnProtocolName, wParam );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetStatus - obtain the protocol status

static int MsnGetStatus(WPARAM wParam,LPARAM lParam)
{
	return msnStatusMode;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGotoInbox - goes to the Inbox folder at the Hotmail.com

static int MsnGotoInbox( WPARAM, LPARAM )
{
	DWORD tThreadID;
	CreateThread( NULL, 0, MsnShowMailThread, NULL, 0, &tThreadID );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnInviteCommand - invite command callback function

static int MsnInviteCommand( WPARAM wParam, LPARAM lParam )
{
	ThreadData* tActiveThreads[ 64 ];
	int tThreads = MSN_GetActiveThreads( tActiveThreads ), tChosenThread;
	switch( tThreads )
	{
		case 0:	return 0;

		case 1:	tChosenThread = 0;
					break;

		default:
			HMENU tMenu = ::CreatePopupMenu();

			for ( int i=0; i < tThreads; i++ )
				::AppendMenu( tMenu, MF_STRING, ( UINT_PTR )( i+1 ), MSN_GetContactName( *tActiveThreads[i]->mJoinedContacts ));

			HWND tWindow = CreateWindow("EDIT","",0,1,1,1,1,NULL,NULL,hInst,NULL);

			POINT pt;
			::GetCursorPos ( &pt );
			tChosenThread = ::TrackPopupMenu( tMenu, TPM_NONOTIFY | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, tWindow, NULL );
			::DestroyMenu( tMenu );
			::DestroyWindow( tWindow );
			if ( !tChosenThread )
				return 0;

			tChosenThread--;
	}

	char tEmail[ MSN_MAX_EMAIL_LEN ];
	if ( !MSN_GetStaticString( "e-mail", ( HANDLE )wParam, tEmail, sizeof( tEmail )))
		MSN_SendPacket( tActiveThreads[ tChosenThread ]->s, "CAL", tEmail );

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnLoadIcon - obtain the protocol icon

static int MsnLoadIcon(WPARAM wParam,LPARAM lParam)
{
	UINT id;

	switch(wParam&0xFFFF) {
		case PLI_PROTOCOL: id=IDI_MSN; break;
		default: return (int)(HICON)NULL;	
	}

	bool tIsSmall = ( wParam & PLIF_SMALL ) != 0;
	return (int)LoadImage( hInst, MAKEINTRESOURCE(id), IMAGE_ICON,
									GetSystemMetrics(tIsSmall ? SM_CXSMICON : SM_CXICON),
									GetSystemMetrics(tIsSmall ? SM_CYSMICON : SM_CYICON), 0 );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnRecvFile - creates a database event from the file request been received

int MsnRecvFile( WPARAM wParam, LPARAM lParam )
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	DBDeleteContactSetting( ccs->hContact, "CList", "Hidden" );

	ThreadData* thread = MSN_GetThreadByContact( ccs->hContact );
	if ( thread == NULL )
	{	MSN_SendPacket( msnNSSocket, "XFR", "SB" );
		return 0;
	}

	PROTORECVEVENT* pre = ( PROTORECVEVENT* )ccs->lParam;
	char* szFile = pre->szMessage + sizeof( DWORD );
	char* szDescr = szFile + strlen( szFile ) + 1;

	DBEVENTINFO dbei;
	memset( &dbei, 0, sizeof( dbei ));
	dbei.cbSize = sizeof( dbei );
	dbei.szModule = msnProtocolName;
	dbei.timestamp = pre->timestamp;
	dbei.flags = ( pre->flags & PREF_CREATEREAD ) ? DBEF_READ : 0;
	dbei.eventType = EVENTTYPE_FILE;
	dbei.cbBlob = sizeof( DWORD ) + strlen( szFile ) + strlen( szDescr ) + 2;
	dbei.pBlob = ( PBYTE )pre->szMessage;
	MSN_CallService( MS_DB_EVENT_ADD, ( WPARAM )ccs->hContact, ( LPARAM )&dbei );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnRecvMessage - creates a database event from the message been received

static int MsnRecvMessage(WPARAM wParam,LPARAM lParam)
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	PROTORECVEVENT* pre = ( PROTORECVEVENT* )ccs->lParam;

	DBDeleteContactSetting( ccs->hContact, "CList", "Hidden" );

	DBEVENTINFO dbei = { 0 };
	dbei.cbSize = sizeof( dbei );
	dbei.szModule = msnProtocolName;
	dbei.timestamp = pre->timestamp;
	dbei.flags = ( pre->flags & PREF_CREATEREAD ) ? DBEF_READ : 0;
	dbei.eventType = EVENTTYPE_MESSAGE;
	dbei.cbBlob = strlen( pre->szMessage )+1;
	if ( pre->flags & PREF_UNICODE )
		dbei.cbBlob *= ( sizeof( wchar_t )+1 );

	dbei.pBlob = ( PBYTE )pre->szMessage;
	MSN_CallService( MS_DB_EVENT_ADD, ( WPARAM )ccs->hContact, ( LPARAM )&dbei );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnSendFile - initiates a file transfer

static int MsnSendFile( WPARAM wParam, LPARAM lParam )
{
	if ( !msnLoggedIn )
		return 0;

	CCSDATA *ccs = ( CCSDATA* )lParam;
	if ( MSN_GetWord( ccs->hContact, "Status", ID_STATUS_OFFLINE ) == ID_STATUS_OFFLINE )
		return 0;

	char** files = ( char** )ccs->lParam;
	if ( files[1] != NULL )
	{
		MSN_ShowError( "MSN protocol allows only one file to be sent at a time" );
		return 0;
 	}

	long tFileSize = 0;
	{	struct _stat statbuf;
		if ( _stat( files[0], &statbuf ) == 0 )
			tFileSize += statbuf.st_size;
	}

	filetransfer* sft = new filetransfer( NULL );
	sft->std.totalFiles = 1;
	sft->std.currentFile = strdup( files[0] );
	sft->std.totalBytes = tFileSize;
	sft->std.hContact = ccs->hContact;
	sft->std.sending = 1;
	sft->std.currentFileNumber = 0;
	sft->fileId = -1;

	ThreadData* thread = MSN_GetThreadByContact( ccs->hContact );
	if ( thread != NULL )
	{	sft->mOwner = thread;
		thread->ft = sft;
	}
	else MSN_SendPacket( msnNSSocket, "XFR", "SB" );

	char* pszFiles = strrchr( *files, '\\' ), msg[ 1024 ];
	if ( pszFiles )
		pszFiles++;
	else
		pszFiles = *files;

	char* pszFilesUTF = Utf8Encode( pszFiles );
	_snprintf( msg, sizeof( msg ),
		"Content-Type: text/x-msmsgsinvite; charset=UTF-8\r\n\r\n"
		"Application-Name: File Transfer\r\n"
		"Application-GUID: {5D3E02AB-6190-11d3-BBBB-00C04F795683}\r\n"
		"Invitation-Command: INVITE\r\n"
		"Invitation-Cookie: %i\r\n"
		"Application-File: %s\r\n"
		"Application-FileSize: %i\r\n\r\n",
		( WORD )(((double)rand()/(double)RAND_MAX)*4294967295), pszFilesUTF, tFileSize );
	free( pszFilesUTF );

	if ( thread == NULL )
		MsgQueue_Add( ccs->hContact, msg, -1, sft );
	else
		MSN_SendMessage( thread->s, msg, MSG_DISABLE_HDR );
		
	MSN_SendBroadcast( ccs->hContact, ACKTYPE_FILE, ACKRESULT_SENTREQUEST, sft, 0 );
	return (int)(HANDLE)sft;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnSendMessage - sends the message to a server

struct TFakeAckParams
{
	inline TFakeAckParams( HANDLE p1, HANDLE p2, LONG p3 ) :
		hEvent( p1 ),
		hContact( p2 ),
		id( p3 )
		{}

	HANDLE	hEvent;
	HANDLE	hContact;
	LONG		id;
};

static DWORD CALLBACK sttFakeAck( LPVOID param )
{
	TFakeAckParams* tParam = ( TFakeAckParams* )param;
	WaitForSingleObject( tParam->hEvent, INFINITE );

	Sleep( 100 );
	MSN_SendBroadcast( tParam->hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE )tParam->id, 0 );

	CloseHandle( tParam->hEvent );
	delete tParam;
	return 0;
}

static int sttSendMessage( CCSDATA* ccs, char* msg )
{
	int seq;

	ThreadData* thread = MSN_GetThreadByContact( ccs->hContact );
	if ( thread == NULL )
	{
		WORD wStatus = MSN_GetWord( ccs->hContact, "Status", ID_STATUS_OFFLINE );
		if ( wStatus == ID_STATUS_OFFLINE || msnStatusMode == ID_STATUS_INVISIBLE )
		{	free( msg );
			return 0;
		}

		if ( MsgQueue_CheckContact( ccs->hContact ) == NULL )
			MSN_SendPacket( msnNSSocket, "XFR", "SB" );

		seq = MsgQueue_Add( ccs->hContact, msg, 0, 0 );
	}
	else
	{	if ( thread->mJoinedCount == 0 )
			seq = MsgQueue_Add( ccs->hContact, msg, 0, 0 );
		else
		{
			seq = MSN_SendMessage( thread->s, msg, MSG_REQUIRE_ACK );
			free( msg );

			HANDLE hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

			DWORD dwThreadId;
			CloseHandle( CreateThread( NULL, 0, sttFakeAck, new TFakeAckParams( hEvent, ccs->hContact, seq ), 0, &dwThreadId ));
			SetEvent( hEvent );
	}	}

	return seq;
}

static int MsnSendMessage(WPARAM wParam,LPARAM lParam)
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	char* msg = Utf8Encode(( char* )ccs->lParam );
	return sttSendMessage( ccs, msg );
}

static int MsnSendMessageW(WPARAM wParam,LPARAM lParam)
{
	CCSDATA* ccs = ( CCSDATA* )lParam;

	char* p = ( char* )ccs->lParam;
	char* msg = Utf8EncodeUcs2(( wchar_t* )&p[ strlen(p)+1 ] );
	return sttSendMessage( ccs, msg );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnSendNetMeeting - Netmeeting callback function

static int MsnSendNetMeeting( WPARAM wParam, LPARAM lParam )
{
	if ( !msnLoggedIn ) return 0;

	ThreadData* thread = MSN_GetThreadByContact( HANDLE(wParam) );

	if ( thread == NULL ) {
		MessageBox( NULL, MSN_Translate( "You must be talking to start Netmeeting" ), "MSN Protocol", MB_OK | MB_ICONERROR );
		return 0;
	}

	char msg[ 1024 ];

	_snprintf( msg, sizeof( msg ),
		"Content-Type: text/x-msmsgsinvite; charset=UTF-8\r\n\r\n"
		"Application-Name: NetMeeting\r\n"
		"Application-GUID: {44BBA842-CC51-11CF-AAFA-00AA00B6015C}\r\n"
		"Session-Protocol: SM1\r\n"
		"Invitation-Command: INVITE\r\n"
		"Invitation-Cookie: %i\r\n"
		"Session-ID: {1A879604-D1B8-11D7-9066-0003FF431510}\r\n\r\n",
		( WORD )(((double)rand()/(double)RAND_MAX)*4294967295));

	MSN_SendMessage( thread->s, msg, MSG_DISABLE_HDR );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	MsnSetApparentMode - controls contact visibility

static int MsnSetApparentMode(WPARAM wParam, LPARAM lParam)
{
	CCSDATA *ccs = (CCSDATA *) lParam;
	if ( ccs->wParam && ccs->wParam != ID_STATUS_OFFLINE )
	  return 1;

	WORD oldMode = MSN_GetWord( ccs->hContact, "ApparentMode", 0 );
	if ( ccs->wParam != oldMode )
		MSN_SetWord( ccs->hContact, "ApparentMode", ( WORD )ccs->wParam );

	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	SetSetAvatar - sets our own picture

static int MsnSetAvatar(WPARAM wParam, LPARAM lParam)
{
	if ( !MSN_LoadPngModule() )
		return 5;

	if ( MSN_GetByte( "UseMsnp10", 0 ) == 0 ) {	
		MessageBox( NULL, 
			MSN_Translate( "Avatar assignment is available only for MSN protocol v.10.\nPlease turn on the appropriate option" ),
			MSN_Translate( "Error" ),
			MB_OK | MB_ICONERROR );
		return 1;
	}

	char filter[ 512 ];
	MSN_CallService( MS_UTILS_GETBITMAPFILTERSTRINGS, sizeof filter, ( LPARAM )filter );

	char str[ MAX_PATH ]; str[0] = 0;
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof( OPENFILENAME );
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = str;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.nMaxFile = sizeof str;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrDefExt = "bmp";
	if ( !GetOpenFileName( &ofn ))
		return 2;
	
	HBITMAP hBitmap = ( HBITMAP )MSN_CallService( MS_UTILS_LOADBITMAP, 0, ( LPARAM )str );
	if ( hBitmap == NULL )
		return 3;

	BITMAP bmp;
	HDC hDC = CreateCompatibleDC( NULL );
	SelectObject( hDC, hBitmap );
	GetObject( hBitmap, sizeof( BITMAP ), &bmp );

	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = 0x28;

	HDC hBmpDC = CreateCompatibleDC( hDC );
	HBITMAP hStretchedBitmap = CreateBitmap( 96, 96, 1, GetDeviceCaps( hDC, BITSPIXEL ), NULL );
	SelectObject( hBmpDC, hStretchedBitmap );
	int side, dx, dy;

	if ( bmp.bmWidth > bmp.bmHeight ) {
		side = bmp.bmHeight;
		dx = ( bmp.bmWidth - bmp.bmHeight )/2;
		dy = 0;
	}
	else {
		side = bmp.bmWidth;
		dx = 0;
		dy = ( bmp.bmHeight - bmp.bmWidth )/2;
	}

	SetStretchBltMode( hBmpDC, HALFTONE );
	StretchBlt( hBmpDC, 0, 0, 96, 96, hDC, dx, dy, side, side, SRCCOPY );
	DeleteObject( hBitmap );	
	DeleteDC( hDC );

	if ( GetDIBits( hBmpDC, hStretchedBitmap, 0, 96, NULL, &bmi, DIB_RGB_COLORS ) == 0 ) {
		TWinErrorCode errCode;
		MSN_ShowError( "Unable to get the bitmap: error %d (%s)", errCode.mErrorCode, errCode.getText() );
		return 6;
	}

	BYTE* pBits = new BYTE[ bmi.bmiHeader.biSizeImage ];
	GetDIBits( hBmpDC, hStretchedBitmap, 0, bmi.bmiHeader.biHeight, pBits, &bmi, DIB_RGB_COLORS );
	DeleteObject( hStretchedBitmap );	
	DeleteDC( hBmpDC );

	long dwPngSize = 0;
	if ( !dib2pngConvertor( &bmi, pBits, NULL, &dwPngSize )) {
		delete pBits;
		return 7;
	}

	BYTE* pPngMemBuffer = new BYTE[ dwPngSize ];
	dib2pngConvertor( &bmi, pBits, pPngMemBuffer, &dwPngSize );
	delete pBits;

	SHA1Context sha1ctx;
	BYTE sha1c[ SHA1HashSize ], sha1d[ SHA1HashSize ];
	char szSha1c[ 40 ], szSha1d[ 40 ];
	SHA1Reset( &sha1ctx );
	SHA1Input( &sha1ctx, pPngMemBuffer, dwPngSize );
	SHA1Result( &sha1ctx, sha1d );
	{	NETLIBBASE64 nlb = { szSha1d, sizeof szSha1d, ( PBYTE )sha1d, sizeof sha1d };
		MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb ));
	}

	SHA1Reset( &sha1ctx );

	char szEmail[ MSN_MAX_EMAIL_LEN ];
	MSN_GetStaticString( "e-mail", NULL, szEmail, sizeof szEmail );
	SHA1Input( &sha1ctx, ( PBYTE )"Creator", 7 );
	SHA1Input( &sha1ctx, ( PBYTE )szEmail, strlen( szEmail ));

	char szFileSize[ 20 ];
	ltoa( dwPngSize, szFileSize, 10 );
	SHA1Input( &sha1ctx, ( PBYTE )"Size", 4 );
	SHA1Input( &sha1ctx, ( PBYTE )szFileSize, strlen( szFileSize ));

	SHA1Input( &sha1ctx, ( PBYTE )"Type", 4 );
	SHA1Input( &sha1ctx, ( PBYTE )"3", 1 );

	SHA1Input( &sha1ctx, ( PBYTE )"Location", 8 );
	SHA1Input( &sha1ctx, ( PBYTE )"TFR43.dat", 9 );

	SHA1Input( &sha1ctx, ( PBYTE )"Friendly", 8 );
	SHA1Input( &sha1ctx, ( PBYTE )"AAA=", 4 );

	SHA1Input( &sha1ctx, ( PBYTE )"SHA1D", 5 );
	SHA1Input( &sha1ctx, ( PBYTE )szSha1d, strlen( szSha1d ));
	SHA1Result( &sha1ctx, sha1c );
	{	NETLIBBASE64 nlb = { szSha1c, sizeof szSha1c, ( PBYTE )sha1c, sizeof sha1c };
		MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb ));
	}
	{	char* szBuffer = ( char* )alloca( 1000 );
		_snprintf( szBuffer, 1000,
			"<msnobj Creator=\"%s\" Size=\"%ld\" Type=\"3\" Location=\"TFR43.dat\" Friendly=\"AAA=\" SHA1D=\"%s\" SHA1C=\"%s\"/>",
			szEmail, dwPngSize, szSha1d, szSha1c );

		char* szEncodedBuffer = ( char* )alloca( 1000 );
		UrlEncode( szBuffer, szEncodedBuffer, 1000 );

		MSN_SetString( NULL, "PictObject", szEncodedBuffer );
		if ( msnLoggedIn )
			MSN_SetServerStatus( msnStatusMode );
	}
	{	char tFileName[ MAX_PATH ];
		MSN_GetAvatarFileName( NULL, tFileName, sizeof tFileName );
		FILE* out = fopen( tFileName, "wb" );
		if ( out == NULL ) {
			delete pPngMemBuffer;
			return 8;
		}

		fwrite( pPngMemBuffer, dwPngSize, 1, out );
		fclose( out );
	}
	delete pPngMemBuffer;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	SetNicknameCommand - sets nick name

static BOOL CALLBACK DlgProcSetNickname(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg )
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault( hwndDlg );
			SendMessage( hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon( hInst, MAKEINTRESOURCE( IDI_MSN )));
			SendMessage( GetDlgItem( hwndDlg, IDC_NICKNAME ), EM_LIMITTEXT, 130, 0 );

			char tNick[ 130 ];
			if ( !MSN_GetStaticString( "Nick", NULL, tNick, sizeof tNick ))
				SetDlgItemText( hwndDlg, IDC_NICKNAME, tNick );

			return TRUE;
		}
		case WM_COMMAND:
			switch(wParam)
			{
				case IDOK:
					if ( msnLoggedIn )
					{
						char str[ 130+1 ];
						GetDlgItemText( hwndDlg, IDC_NICKNAME, str, sizeof( str ));

						char tEmail[ MSN_MAX_EMAIL_LEN ];
						MSN_GetStaticString( "e-mail", NULL, tEmail, sizeof( tEmail ));
						MSN_SendNickname( tEmail, str );
					}	

				case IDCANCEL:
 					DestroyWindow( hwndDlg );
					break;
			}
			break;

		case WM_CLOSE:
			DestroyWindow( hwndDlg );
			break;
	}
	return FALSE;
}

static int SetNicknameCommand( WPARAM wParam, LPARAM lParam )
{
	HWND hwndSetNickname = CreateDialog(hInst, MAKEINTRESOURCE( IDD_SETNICKNAME ), NULL, DlgProcSetNickname );
	
	SetForegroundWindow( hwndSetNickname );
	SetFocus( hwndSetNickname );
 	ShowWindow( hwndSetNickname, SW_SHOW );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnSetStatus - set the plugin's connection status

static int MsnSetStatus( WPARAM wParam, LPARAM lParam )
{
	msnDesiredStatus = wParam;
	MSN_DebugLog( "PS_SETSTATUS(%d,0)", wParam );

	if ( msnDesiredStatus == ID_STATUS_OFFLINE )
	{
		MSN_GoOffline();
	}
	else if (!msnLoggedIn && !(msnStatusMode>=ID_STATUS_CONNECTING && msnStatusMode<ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES)) 
	{
		ThreadData* newThread = new ThreadData;

		WORD tServerPort = MSN_GetWord( NULL, "MSNMPort", 1863 );

		char tServer[ sizeof( newThread->mServer ) ];
		if ( !MSN_GetStaticString( "LoginServer", NULL, tServer, sizeof( tServer )))
			lstrcpyn( newThread->mServer, tServer, sizeof( newThread->mServer ));
		else 
			strcpy( newThread->mServer, MSN_DEFAULT_LOGIN_SERVER );
		
		sprintf( newThread->mServer + strlen( newThread->mServer ), ":%i", tServerPort );

		newThread->mType = SERVER_DISPATCH;
		{	int oldMode = msnStatusMode;
			msnStatusMode = ID_STATUS_CONNECTING;
			MSN_SendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE )oldMode, msnStatusMode );
		}
		newThread->startThread(( pThreadFunc )MSNServerThread );
	}
	else MSN_SetServerStatus( msnDesiredStatus );

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnUserIsTyping - notify another contact that we're typing a message

static int MsnUserIsTyping(WPARAM wParam, LPARAM lParam)
{
	if ( !msnLoggedIn || lParam == PROTOTYPE_SELFTYPING_OFF )
		return 0;

	char tEmail[ MSN_MAX_EMAIL_LEN ];
	if ( MSN_GetStaticString( "e-mail", NULL, tEmail, sizeof tEmail ))
		return 0;

	HANDLE hContact = ( HANDLE )wParam;
	WORD wStatus = MSN_GetWord( hContact, "Status", ID_STATUS_OFFLINE );
	if ( wStatus == ID_STATUS_OFFLINE || msnStatusMode == ID_STATUS_INVISIBLE )
		return 0;

	char tCommand[ 1024 ];
	_snprintf( tCommand, sizeof( tCommand ),
		"Content-Type: text/x-msmsgscontrol\r\n"
		"TypingUser: %s\r\n\r\n\r\n", tEmail );

	ThreadData* T = MSN_GetThreadByContact( hContact );
	if ( T == NULL ) {
		if ( MsgQueue_CheckContact( hContact ) == NULL ) {
			MsgQueue_Add( hContact, tCommand, -1 );
			MSN_SendPacket( msnNSSocket, "XFR", "SB" );
		}
	}
	else MSN_SendMessage( T->s, tCommand, MSG_DISABLE_HDR );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnViewProfile - view a contact's profile at http://members.msn.com

static char sttUrlPrefix[] = "http://members.msn.com/";

static int MsnViewProfile( WPARAM wParam, LPARAM lParam )
{
	char tUrl[ MSN_MAX_EMAIL_LEN + sizeof sttUrlPrefix ];
	strcpy( tUrl, sttUrlPrefix );

	if ( !MSN_GetStaticString( "e-mail", ( HANDLE )wParam, tUrl + sizeof sttUrlPrefix - 1, MSN_MAX_EMAIL_LEN ))
		MSN_CallService( MS_UTILS_OPENURL, 1, ( LPARAM )tUrl );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnViewServiceStatus - display MSN services status

static int MsnViewServiceStatus( WPARAM wParam, LPARAM lParam )
{
	MSN_CallService( MS_UTILS_OPENURL, 1, ( LPARAM )"http://messenger.msn.com/Status.aspx" );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Services initialization and destruction

static HANDLE hHookSettingChanged = NULL;
static HANDLE hHookContactDeleted = NULL;

int LoadMsnServices( void )
{
	//////////////////////////////////////////////////////////////////////////////////////
	// Main menu initialization 

	char servicefunction[ 100 ];
	strcpy( servicefunction, msnProtocolName );
	char* tDest = servicefunction + strlen( servicefunction );
	CLISTMENUITEM mi;

	if ( !MSN_GetByte( "DisableSetNickname", 0 ))
	{
		strcpy( tDest, MS_SET_NICKNAME );
		CreateServiceFunction( servicefunction, SetNicknameCommand );

		memset( &mi, 0, sizeof( mi ));
		mi.popupPosition = 500085000;
		mi.pszPopupName = msnProtocolName;
		mi.cbSize = sizeof( mi );
		mi.position = 2000060000;
		mi.hIcon = LoadIcon( hInst, MAKEINTRESOURCE( IDI_MSN ));
		mi.pszName = MSN_Translate( "Set &Nickname" );
		mi.pszService = servicefunction;
		msnMenuItems[ 0 ] = ( HANDLE )MSN_CallService( MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi );

		strcpy( tDest, MS_GOTO_INBOX );
		CreateServiceFunction( servicefunction, MsnGotoInbox );

		mi.position = 2000060001;
		mi.hIcon = LoadIcon( hInst, MAKEINTRESOURCE( IDI_INBOX ));
		mi.pszName = MSN_Translate( "Display Hotmail &Inbox" );
		mi.pszService = servicefunction;
		MSN_CallService( MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi );

		strcpy( tDest, MS_EDIT_PROFILE );
		CreateServiceFunction( servicefunction, MsnEditProfile );

		mi.position = 2000060002;
		mi.hIcon = LoadIcon( hInst, MAKEINTRESOURCE( IDI_PROFILE ));
		mi.pszName = MSN_Translate( "Edit MSN &Profile" );
		mi.pszService = servicefunction;
		msnMenuItems[ 1 ] = ( HANDLE )MSN_CallService( MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi );

		strcpy( tDest, MS_SET_AVATAR );
		CreateServiceFunction( servicefunction, MsnSetAvatar );

		mi.position = 2000060003;
		mi.hIcon = LoadIcon( hInst, MAKEINTRESOURCE( IDI_AVATAR ));
		mi.pszName = MSN_Translate( "Set &Avatar" );
		mi.pszService = servicefunction;
		MSN_CallService( MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi );

		strcpy( tDest, MS_VIEW_STATUS );
		CreateServiceFunction( servicefunction, MsnViewServiceStatus );

		mi.position = 2000060004;
		mi.hIcon = LoadIcon( hInst, MAKEINTRESOURCE( IDI_SERVICES ));
		mi.pszName = MSN_Translate( "View MSN Services &Status" );
		mi.pszService = servicefunction;
		MSN_CallService( MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi );
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// Contact menu initialization 

	strcpy( tDest, MSN_BLOCK );
	CreateServiceFunction( servicefunction, MsnBlockCommand );

	memset( &mi, 0, sizeof( mi ));
	mi.cbSize = sizeof( mi );
	mi.position = -500050000;
	mi.hIcon = LoadIcon( hInst, MAKEINTRESOURCE( IDI_MSNBLOCK ));
	mi.pszContactOwner = msnProtocolName;
	mi.pszName = MSN_Translate( "&Block/Unblock" );
	mi.pszService = servicefunction;
	msnSetNicknameMenuItem = ( HANDLE )MSN_CallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	strcpy( tDest, MSN_INVITE );
	CreateServiceFunction( servicefunction, MsnInviteCommand );

	mi.flags = CMIF_NOTOFFLINE;
	mi.position = -500050001;
	mi.hIcon = LoadIcon( hInst, MAKEINTRESOURCE( IDI_INVITE ));
	mi.pszName = MSN_Translate( "&Invite to chat" );
	mi.pszService = servicefunction;
	MSN_CallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	strcpy( tDest, MSN_NETMEETING );
	CreateServiceFunction( servicefunction, MsnSendNetMeeting );

	mi.flags = CMIF_NOTOFFLINE;
	mi.position = -500050002;
	mi.hIcon = LoadIcon( hInst, MAKEINTRESOURCE( IDI_NETMEETING ));
	mi.pszName = MSN_Translate( "&Start Netmeeting" );
	mi.pszService = servicefunction;
	MSN_CallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	strcpy( tDest, MSN_VIEW_PROFILE );
	CreateServiceFunction( servicefunction, MsnViewProfile );

	mi.flags = 0;
	mi.position = -500050003;
	mi.hIcon = LoadIcon( hInst, MAKEINTRESOURCE( IDI_PROFILE ));
	mi.pszName = MSN_Translate( "&View Profile" );
	mi.pszService = servicefunction;
	MSN_CallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );

	MSN_EnableMenuItems( FALSE );

	//////////////////////////////////////////////////////////////////////////////////////
	// Service creation 

	hHookContactDeleted = HookEvent( ME_DB_CONTACT_DELETED, MsnContactDeleted );
	hHookSettingChanged = HookEvent( ME_DB_CONTACT_SETTINGCHANGED, MsnDbSettingChanged );

	MSN_CreateProtoServiceFunction( PS_ADDTOLIST,			MsnAddToList );
	MSN_CreateProtoServiceFunction( PS_ADDTOLISTBYEVENT,	MsnAddToListByEvent );
	MSN_CreateProtoServiceFunction( PS_AUTHALLOW,			MsnAuthAllow );
	MSN_CreateProtoServiceFunction( PS_AUTHDENY,				MsnAuthDeny );
	MSN_CreateProtoServiceFunction( PS_BASICSEARCH,			MsnBasicSearch );
	MSN_CreateProtoServiceFunction( PS_GETAVATARINFO,		MsnGetAvatarInfo );
	MSN_CreateProtoServiceFunction( PS_GETCAPS,				MsnGetCaps );
	MSN_CreateProtoServiceFunction( PS_GETNAME,				MsnGetName );
	MSN_CreateProtoServiceFunction( PS_GETSTATUS,			MsnGetStatus );
	MSN_CreateProtoServiceFunction( PS_LOADICON,				MsnLoadIcon );
	MSN_CreateProtoServiceFunction( PS_SEARCHBYEMAIL,		MsnBasicSearch );
	MSN_CreateProtoServiceFunction( PS_SETSTATUS,			MsnSetStatus );

	MSN_CreateProtoServiceFunction( PSR_FILE,					MsnRecvFile );
	MSN_CreateProtoServiceFunction( PSR_MESSAGE,				MsnRecvMessage );

	MSN_CreateProtoServiceFunction( PSS_FILE,					MsnSendFile );
	MSN_CreateProtoServiceFunction( PSS_FILEALLOW,			MsnFileAllow );
	MSN_CreateProtoServiceFunction( PSS_FILECANCEL,			MsnFileCancel );
	MSN_CreateProtoServiceFunction( PSS_FILEDENY,			MsnFileDeny );
	MSN_CreateProtoServiceFunction( PSS_GETINFO,				MsnGetInfo );
	MSN_CreateProtoServiceFunction( PSS_MESSAGE,				MsnSendMessage );
	MSN_CreateProtoServiceFunction( PSS_MESSAGE"W",			MsnSendMessageW );
	MSN_CreateProtoServiceFunction( PSS_SETAPPARENTMODE,  MsnSetApparentMode );
	MSN_CreateProtoServiceFunction( PSS_USERISTYPING,     MsnUserIsTyping );
	return 0;
}

void UnloadMsnServices( void )
{
	if ( hHookSettingChanged )
		UnhookEvent( hHookSettingChanged );

	if ( hHookContactDeleted )
		UnhookEvent( hHookContactDeleted );
}
