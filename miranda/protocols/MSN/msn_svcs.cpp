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

extern int avsPresent;

void __cdecl MSNServerThread( ThreadData* info );

void msnftp_sendAcceptReject( filetransfer *ft, bool acc );
void msnftp_invite( filetransfer *ft );

/////////////////////////////////////////////////////////////////////////////////////////
// MsnAddToList - adds contact to the server list

static HANDLE AddToListByEmail( const char *email, DWORD flags )
{
	HANDLE hContact;

	//check not already on list
	for ( hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		   hContact != NULL;
			hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM )hContact, 0 ))
	{
		if ( MSN_IsMyContact( hContact )) {
			char tEmail[ MSN_MAX_EMAIL_LEN ];
			if ( MSN_GetStaticString( "e-mail", hContact, tEmail, sizeof( tEmail )))
				continue;

			if ( strcmp( tEmail, email ))
				continue;

			if ( !( flags & PALF_TEMPORARY ) && DBGetContactSettingByte( hContact, "CList", "NotOnList", 1 )) {
				DBDeleteContactSetting( hContact, "CList", "NotOnList" );
				DBDeleteContactSetting( hContact, "CList", "Hidden" );
				goto LBL_AddContact;
			}

			return hContact;
	}	}

	//not already there: add
	hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_ADD, 0, 0 );
	MSN_CallService( MS_PROTO_ADDTOCONTACT, ( WPARAM )hContact,( LPARAM )msnProtocolName );
	MSN_SetString( hContact, "e-mail", email );

	if ( flags & PALF_TEMPORARY ) {
   		DBWriteContactSettingByte( hContact, "CList", "NotOnList", 1 );
		DBWriteContactSettingByte( hContact, "CList", "Hidden", 1 );
	}
	else {
LBL_AddContact:
		if ( msnLoggedIn ) {
			MSN_AddUser( hContact, email, LIST_FL );
			MSN_AddUser( hContact, email, LIST_BL + LIST_REMOVE );
			MSN_AddUser( hContact, email, LIST_AL );
			if (strncmp(email, "tel:", 4) == 0 )
			{
				MSN_SetWord( hContact, "Status", ID_STATUS_ONTHEPHONE );
				MSN_SetString( hContact, "MirVer", "SMS" );
			}
		}
		else hContact = NULL;
	}
	return hContact;
}

static int MsnAddToList(WPARAM wParam,LPARAM lParam)
{
	PROTOSEARCHRESULT *psr=(PROTOSEARCHRESULT*)lParam;

	if ( psr->cbSize != sizeof( PROTOSEARCHRESULT ))
		return 0;

	return ( int )AddToListByEmail( psr->email, wParam );
}

static int MsnAddToListByEvent(WPARAM wParam, LPARAM lParam)
{
	DBEVENTINFO dbei = {0};
	dbei.cbSize = sizeof(dbei);
	if (( dbei.cbBlob = MSN_CallService( MS_DB_EVENT_GETBLOBSIZE, lParam, 0 )) == (DWORD)(-1))
		return 0;

	dbei.pBlob=(PBYTE) alloca(dbei.cbBlob);
	if ( MSN_CallService(MS_DB_EVENT_GET, lParam, ( LPARAM )&dbei))	return 0;
	if ( strcmp(dbei.szModule, msnProtocolName))						      return 0;
	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST)					      return 0;

	char* nick = (char *) (dbei.pBlob + sizeof(DWORD) + sizeof(HANDLE));
	char* firstName = nick + strlen(nick) + 1;
	char* lastName = firstName + strlen(firstName) + 1;
	char* email = lastName + strlen(lastName) + 1;

	return ( int ) AddToListByEmail( email, 0 );
}

static int MsnRecvAuth(WPARAM wParam, LPARAM lParam)
{
	DBEVENTINFO dbei = { 0 };
	CCSDATA *ccs = ( CCSDATA* )lParam;
	PROTORECVEVENT *pre = ( PROTORECVEVENT* )ccs->lParam;

	dbei.cbSize = sizeof(dbei);
	dbei.szModule = msnProtocolName;
	dbei.timestamp = pre->timestamp;
	dbei.flags = pre->flags & ( PREF_CREATEREAD ? DBEF_READ : 0 );
	dbei.eventType = EVENTTYPE_AUTHREQUEST;

	/* Just copy the Blob from PSR_AUTH event. */
	dbei.cbBlob = pre->lParam;
	dbei.pBlob = (PBYTE)pre->szMessage;
	CallService( MS_DB_EVENT_ADD, 0,( LPARAM )&dbei );

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnAuthAllow - called after successful authorization

static int MsnAuthAllow(WPARAM wParam,LPARAM lParam)
{
	if ( !msnLoggedIn )
		return 1;

	DBEVENTINFO dbei = { 0 };
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

	UrlEncode( UTF8( nick ), urlNick, sizeof( urlNick ));
	UrlEncode( email, urlEmail, sizeof( urlEmail ));

	AddToListByEmail( email, 0 );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnAuthDeny - called after unsuccessful authorization

static int MsnAuthDeny(WPARAM wParam,LPARAM lParam)
{
	if ( !msnLoggedIn )
		return 1;

	DBEVENTINFO dbei = { 0 };
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

	UrlEncode( UTF8(nick), urlNick, sizeof( urlNick ));
	UrlEncode( email, urlEmail, sizeof( urlEmail ));

	MSN_AddUser( NULL, urlEmail, LIST_BL );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnBasicSearch - search contacts by e-mail

static int MsnBasicSearch(WPARAM wParam,LPARAM lParam)
{
	if ( !msnLoggedIn /* || msnSearchID != -1 */ )
		return 0;

	return msnSearchID = msnNsThread->sendPacket( "ADC", "BL N=%s", ( char* )lParam );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnContactDeleted - called when a contact is deleted from list

int MsnContactDeleted( WPARAM wParam, LPARAM lParam )
{
	const HANDLE hContact = ( HANDLE )wParam;

	if ( !msnLoggedIn )  //should never happen for MSN contacts
		return 0;

	if ( !MSN_IsMyContact( hContact ))
		return 0;

	char tEmail[ MSN_MAX_EMAIL_LEN ];
	if ( !MSN_GetStaticString( "e-mail", hContact, tEmail, sizeof( tEmail ))) {
		MSN_AddUser( hContact, tEmail, LIST_FL | LIST_REMOVE );
		MSN_AddUser( hContact, tEmail, LIST_AL | LIST_REMOVE );

		if ( !Lists_IsInList( LIST_RL, hContact )) {
			MSN_AddUser( hContact, tEmail, LIST_BL | LIST_REMOVE );
			Lists_Remove( 0xFF, hContact );
		}
		else {
			MSN_AddUser( hContact, tEmail, LIST_BL );
			Lists_Remove( LIST_FL, hContact );
		}
	}

	int type = DBGetContactSettingByte( hContact, msnProtocolName, "ChatRoom", 0 );
	if ( type != 0 ) {
		DBVARIANT dbv;
		if ( !MSN_GetStringT( "ChatRoomID", hContact, &dbv )) {
			MSN_KillChatSession( dbv.ptszVal );
			MSN_FreeVariant( &dbv );
	}	}

	return 0;
}


int MsnGroupChange(WPARAM wParam,LPARAM lParam)
{
	if (!msnLoggedIn || !MyOptions.ManageServer) return 0;

	const HANDLE hContact = (HANDLE)wParam;
	const CLISTGROUPCHANGE* grpchg = (CLISTGROUPCHANGE*)lParam;

	if (hContact == NULL)
	{
		if (grpchg->pszNewName == NULL && grpchg->pszOldName != NULL)
		{
			char* szOldName = mir_utf8encodeT(grpchg->pszOldName);
			LPCSTR szId = MSN_GetGroupByName(szOldName);
			if (szId != NULL) msnNsThread->sendPacket("RMG", szId);	
			mir_free(szOldName);
		}
		else if (grpchg->pszNewName != NULL && grpchg->pszOldName != NULL)
		{
			char* szNewName = mir_utf8encodeT(grpchg->pszNewName);
			char* szOldName = mir_utf8encodeT(grpchg->pszOldName);
			LPCSTR szId = MSN_GetGroupByName(szOldName);
			if (szId != NULL) MSN_RenameServerGroup(szId, szNewName);
			mir_free(szOldName);
			mir_free(szNewName);
		}
	}
	else
	{
		if (MSN_IsMyContact(hContact))
		{
			char* szNewName = grpchg->pszNewName ? mir_utf8encodeT(grpchg->pszNewName) : NULL;
			MSN_MoveContactToGroup(hContact, szNewName);
			mir_free(szNewName);
		}
	}
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////
// MsnDbSettingChanged - look for contact's settings changes

int MsnDbSettingChanged(WPARAM wParam,LPARAM lParam)
{
	HANDLE hContact = ( HANDLE )wParam;
	DBCONTACTWRITESETTING* cws = ( DBCONTACTWRITESETTING* )lParam;

	if (!msnLoggedIn)
		return 0;

	if (hContact == NULL) 
	{
		if (MyOptions.SlowSend && strcmp(cws->szSetting, "MessageTimeout") == 0 &&
		   (strcmp(cws->szModule, "SRMM") == 0 || strcmp(cws->szModule, "SRMsg") == 0))
		{ 
			if (cws->value.dVal < 60000)
				MessageBox(NULL, TranslateT("MSN Protocol requires messages timeout to be not less then 60 sec. Correct the timeout value."), TranslateT("MSN"), MB_OK|MB_ICONINFORMATION);
		}
		return 0;
	}

	if ( !strcmp( cws->szSetting, "ApparentMode" )) {
		if ( !MSN_IsMyContact( hContact ))
			return 0;

		char tEmail[ MSN_MAX_EMAIL_LEN ];
		if ( !MSN_GetStaticString( "e-mail", hContact, tEmail, sizeof( tEmail ))) {
			int isBlocked = Lists_IsInList( LIST_BL, hContact );

			if ( !isBlocked && cws->value.wVal == ID_STATUS_OFFLINE ) {
				MSN_AddUser( hContact, tEmail, LIST_AL + LIST_REMOVE );
				MSN_AddUser( hContact, tEmail, LIST_BL );
			}
			else if ( isBlocked && cws->value.wVal == 0 ) {
				MSN_AddUser( hContact, tEmail, LIST_BL + LIST_REMOVE );
				MSN_AddUser( hContact, tEmail, LIST_AL );
	}	}	}

	if ( !strcmp( cws->szModule, "CList" )) {
		if ( !MSN_IsMyContact( hContact ))
			return 0;

		if ( !strcmp( cws->szSetting, "MyHandle" )) {
			char szContactID[ 100 ], szNewNick[ 387 ];
			if ( !MSN_GetStaticString( "ID", hContact, szContactID, sizeof( szContactID ))) {
				MSN_GetStaticString( "e-mail", hContact, szNewNick, sizeof( szNewNick ));
				if ( strcmp( szNewNick, MyOptions.szEmail )) {
					if ( cws->value.type != DBVT_DELETED ) {
						if ( cws->value.type == DBVT_UTF8 )
							UrlEncode( cws->value.pszVal, szNewNick, sizeof( szNewNick ));
						else
							UrlEncode( UTF8(cws->value.pszVal), szNewNick, sizeof( szNewNick ));
					}
					msnNsThread->sendPacket( "SBP", "%s MFN %s", szContactID, szNewNick );
				}
				return 0;
	}	}	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnWindowEvent - creates session on window open

int MsnIdleChanged(WPARAM wParam, LPARAM lParam)
{
	if ( !msnLoggedIn || msnDesiredStatus != ID_STATUS_ONLINE )
		return 0;

    if ( lParam & IDF_PRIVACY ) {
		if ( msnStatusMode == ID_STATUS_IDLE )
			MSN_SetServerStatus( ID_STATUS_ONLINE );
	}
    else
		MSN_SetServerStatus( lParam & IDF_ISIDLE ? ID_STATUS_IDLE : ID_STATUS_ONLINE );

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnWindowEvent - creates session on window open

int MsnWindowEvent(WPARAM wParam, LPARAM lParam)
{
	MessageWindowEventData* msgEvData  = (MessageWindowEventData*)lParam;

	if ( msgEvData->uType == MSG_WINDOW_EVT_OPENING ) {
		if ( !MSN_IsMyContact( msgEvData->hContact )) return 0;
		if ( MSN_IsMeByContact( msgEvData->hContact )) return 0;

		bool isOffline;
		ThreadData* thread = MSN_StartSB(msgEvData->hContact, isOffline);
		
		if (thread == NULL && !isOffline)
			MsgQueue_Add( msgEvData->hContact, 'X', NULL, 0, NULL );
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnFileAllow - starts the file transfer

static void __cdecl MsnFileAckThread( void* arg )
{
	filetransfer* ft = (filetransfer*)arg;
	if ( !ft->inmemTransfer ) {
		char filefull[ MAX_PATH ];
		mir_snprintf( filefull, sizeof( filefull ), "%s\\%s", ft->std.workingDir, ft->std.currentFile );
		replaceStr( ft->std.currentFile, filefull );

		if ( MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_FILERESUME, ft, ( LPARAM )&ft->std ))
			return;

		if ( ft->wszFileName != NULL ) {
			mir_free( ft->wszFileName );
			ft->wszFileName = NULL;
	}	}

	bool fcrt = ft->create() != -1;

	if ( ft->p2p_appID != 0) {
		p2p_sendStatus( ft, fcrt ? 200 : 603 );
		if ( fcrt )
			p2p_sendFeedStart( ft );
	}
	else
		msnftp_sendAcceptReject (ft, fcrt);

	MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0);
}

int MsnFileAllow(WPARAM wParam, LPARAM lParam)
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	filetransfer* ft = ( filetransfer* )ccs->wParam;

	if ( !msnLoggedIn || !p2p_sessionRegistered( ft ) )
		return 0;

	if (( ft->std.workingDir = mir_strdup(( char* )ccs->lParam )) == NULL ) {
		char szCurrDir[ MAX_PATH ];
		GetCurrentDirectoryA( sizeof( szCurrDir ), szCurrDir );
		ft->std.workingDir = mir_strdup( szCurrDir );
	}
	else {
		int len = strlen( ft->std.workingDir )-1;
		if ( ft->std.workingDir[ len ] == '\\' )
			ft->std.workingDir[ len ] = 0;
	}

	mir_forkthread( MsnFileAckThread, ft );

	return int( ft );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnFileCancel - cancels the active file transfer

int MsnFileCancel(WPARAM wParam, LPARAM lParam)
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	filetransfer* ft = ( filetransfer* )ccs->wParam;

	if ( !msnLoggedIn || !p2p_sessionRegistered( ft ))
		return 0;

	if  ( !ft->std.sending && ft->fileId == -1 ) {
		if ( ft->p2p_appID != 0 )
			p2p_sendStatus(ft, 603);
		else
			msnftp_sendAcceptReject (ft, false);
	}
	else {
		ft->bCanceled = true;
		if ( ft->p2p_appID != 0 )
			p2p_sendCancel( ft );
	}

	ft->std.files = NULL;
	ft->std.totalFiles = 0;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnFileDeny - rejects the file transfer request

int MsnFileDeny( WPARAM wParam, LPARAM lParam )
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	filetransfer* ft = ( filetransfer* )ccs->wParam;

	if ( !msnLoggedIn || !p2p_sessionRegistered( ft ))
		return 1;

	if ( !ft->std.sending && ft->fileId == -1 ) {
		if ( ft->p2p_appID != 0 )
			p2p_sendStatus(ft, 603);
		else
			msnftp_sendAcceptReject (ft, false);
	}
	else {
		if ( ft->p2p_appID != 0 )
			p2p_sendCancel( ft );
		else
			ft->bCanceled = true;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnFileResume - renames a file

int MsnFileResume( WPARAM wParam, LPARAM lParam )
{
	filetransfer* ft = ( filetransfer* )wParam;

	if ( !msnLoggedIn || !p2p_sessionRegistered( ft ))
		return 1;

	PROTOFILERESUME *pfr = (PROTOFILERESUME*)lParam;
	switch (pfr->action) {
	case FILERESUME_SKIP:
		if ( ft->p2p_appID != 0 )
			p2p_sendStatus( ft, 603 );
		else
			msnftp_sendAcceptReject (ft, false);
		break;

	case FILERESUME_RENAME:
		if ( ft->wszFileName != NULL ) {
			mir_free( ft->wszFileName );
			ft->wszFileName = NULL;
		}
		replaceStr( ft->std.currentFile, pfr->szFilename );

	default:
		bool fcrt = ft->create() != -1;
		if ( ft->p2p_appID != 0 ) {
			p2p_sendStatus( ft, fcrt ? 200 : 603 );
			if ( fcrt )
				p2p_sendFeedStart( ft );
		}
		else
			msnftp_sendAcceptReject (ft, fcrt);

		MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0);
		break;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetAvatar - retrieves the file name of my own avatar

int MsnGetAvatar(WPARAM wParam, LPARAM lParam)
{
	if (avsPresent < 0) avsPresent = ServiceExists(MS_AV_SETMYAVATAR) != 0;
	if (!avsPresent) return 1;

	char* buf = ( char* )wParam;
	int  size = ( int )lParam;

	if ( buf == NULL || size <= 0 )
		return -1;

	MSN_GetAvatarFileName( NULL, buf, size );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetAvatarInfo - retrieve the avatar info

static void sttFakeAvatarAck( LPVOID param )
{
	Sleep( 100 );
	MSN_SendBroadcast((( PROTO_AVATAR_INFORMATION* )param )->hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, param, 0 );
}

static int MsnGetAvatarInfo(WPARAM wParam,LPARAM lParam)
{
	if (avsPresent < 0) avsPresent = ServiceExists(MS_AV_SETMYAVATAR) != 0;
	if (!avsPresent) return GAIR_NOAVATAR;

	PROTO_AVATAR_INFORMATION* AI = ( PROTO_AVATAR_INFORMATION* )lParam;

	if (( MSN_GetDword( AI->hContact, "FlagBits", 0 ) & 0xf0000000 ) == 0 )
		return GAIR_NOAVATAR;

	char szContext[ MAX_PATH ];
	if ( MSN_GetStaticString(( AI->hContact == NULL ) ? "PictObject" : "PictContext", AI->hContact,
		szContext, sizeof( szContext )))
		return GAIR_NOAVATAR;

	if ( MSN_IsMeByContact( AI->hContact ))
	{
		MSN_GetAvatarFileName( NULL, AI->filename, sizeof( AI->filename ));
		AI->format = PA_FORMAT_PNG;
		return 	(_access(AI->filename, 4) ? GAIR_NOAVATAR : GAIR_SUCCESS);
	}

	MSN_GetAvatarFileName( AI->hContact, AI->filename, sizeof( AI->filename ));
	AI->format = PA_FORMAT_UNKNOWN;

	if ( AI->hContact ) {
		size_t len = strlen( AI->filename );
		strcpy( AI->filename + len, "png" );
		if ( _access( AI->filename, 0 ) == 0 )
			AI->format = PA_FORMAT_PNG;
		else {
			strcpy( AI->filename + len, "jpg" );
			if ( _access( AI->filename, 0 ) == 0 )
				AI->format = PA_FORMAT_JPEG;
			else {
				strcpy( AI->filename + len, "gif" );
				if ( _access( AI->filename, 0 ) == 0 )
					AI->format = PA_FORMAT_GIF;
			}
		}
	}
	else {
		if ( _access( AI->filename, 0 ) == 0 )
			AI->format = PA_FORMAT_PNG;
	}

	if ( AI->format != PA_FORMAT_UNKNOWN ) {
		char szSavedContext[ 256 ];
		if ( !MSN_GetStaticString( "PictSavedContext", AI->hContact, szSavedContext, sizeof( szSavedContext )))
			if ( strcmp( szSavedContext, szContext ) == 0)
				return GAIR_SUCCESS;

		MSN_SetString( AI->hContact, "PictSavedContext", szContext );

		// Store also avatar hash
		char* szAvatarHash = MSN_GetAvatarHash(szContext);
		if (szAvatarHash != NULL)
		{
			MSN_SetString( AI->hContact, "AvatarSavedHash", szAvatarHash );
			mir_free(szAvatarHash);
		}
		return GAIR_SUCCESS;
	}

	if (( wParam & GAIF_FORCE ) != 0 && AI->hContact != NULL )
	{
		WORD wStatus = MSN_GetWord( AI->hContact, "Status", ID_STATUS_OFFLINE );
		if ( wStatus == ID_STATUS_OFFLINE || msnStatusMode == ID_STATUS_INVISIBLE ) {
			MSN_DeleteSetting( AI->hContact, "AvatarHash" );
			PROTO_AVATAR_INFORMATION* fakeAI = new PROTO_AVATAR_INFORMATION;
			*fakeAI = *AI;
			mir_forkthread( sttFakeAvatarAck, fakeAI );
		}
		else {
			if ( p2p_getAvatarSession( AI->hContact ) == NULL )
				p2p_invite( AI->hContact, MSN_APPID_AVATAR );
		}

		return GAIR_WAITFOR;
	}

	return GAIR_NOAVATAR;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetAvatarCaps - retrieves avatar capabilities

static int MsnGetAvatarCaps(WPARAM wParam, LPARAM lParam)
{
	int res = 0;

	switch (wParam)
	{
	case AF_MAXSIZE:
		((POINT*)lParam)->x = 96;
		((POINT*)lParam)->y = 96;
		break;

	case AF_PROPORTION:
		res = PIP_NONE;
		break;

	case AF_FORMATSUPPORTED:
		res = lParam == PA_FORMAT_PNG;
		break;

	case AF_ENABLED:
		res = 1;
		break;
	}

	return res;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetAwayMsg - reads the current status message for a user

typedef struct AwayMsgInfo_tag
{
	int id;
	HANDLE hContact;
} AwayMsgInfo;

static void __cdecl MsnGetAwayMsgThread( void* param )
{
	Sleep( 150 );

	AwayMsgInfo *inf = ( AwayMsgInfo* )param;
	DBVARIANT dbv;
	if ( !DBGetContactSettingString( inf->hContact, "CList", "StatusMsg", &dbv )) {
		MSN_SendBroadcast( inf->hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, ( HANDLE )inf->id, ( LPARAM )dbv.pszVal );
		MSN_FreeVariant( &dbv );
	}
	else MSN_SendBroadcast( inf->hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, ( HANDLE )inf->id, ( LPARAM )0 );

	mir_free( inf );
}

static int MsnGetAwayMsg(WPARAM wParam,LPARAM lParam)
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	if ( ccs == NULL ) return 0;
	
	AwayMsgInfo* inf = (AwayMsgInfo*)mir_alloc( sizeof( AwayMsgInfo ));
	inf->hContact = ccs->hContact;
	inf->id = rand();

	mir_forkthread( MsnGetAwayMsgThread, inf );
	return inf->id;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetCaps - obtain the protocol capabilities

static int MsnGetCaps(WPARAM wParam,LPARAM lParam)
{
	switch( wParam ) {
	case PFLAGNUM_1:
	{	int result = PF1_IM | PF1_SERVERCLIST | PF1_AUTHREQ | PF1_BASICSEARCH |
				 PF1_ADDSEARCHRES | PF1_SEARCHBYEMAIL | PF1_USERIDISEMAIL | PF1_CHAT |
				 PF1_FILESEND | PF1_FILERECV | PF1_URLRECV | PF1_VISLIST | PF1_MODEMSG;
		return result;
	}
	case PFLAGNUM_2:
		return PF2_ONLINE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_LIGHTDND |
				 PF2_ONTHEPHONE | PF2_OUTTOLUNCH | PF2_INVISIBLE | PF2_IDLE;

	case PFLAGNUM_3:
		return PF2_ONLINE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_LIGHTDND |
				 PF2_ONTHEPHONE | PF2_OUTTOLUNCH;

	case PFLAGNUM_4:
		return PF4_SUPPORTTYPING | PF4_AVATARS | PF4_IMSENDUTF;

	case PFLAG_UNIQUEIDTEXT:
		return ( int )MSN_Translate( "E-mail address" );

	case PFLAG_UNIQUEIDSETTING:
		return ( int )"e-mail";

	case PFLAG_MAXLENOFMESSAGE:
		return 1202;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetInfo - nothing to do, cause we cannot obtain information from the server
/*
static int MsnGetInfo( WPARAM wParam, LPARAM lParam )
{
	CCSDATA *ccs = ( CCSDATA* )lParam;
	return 1;
}
*/
/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetName - obtain the protocol name

static int MsnGetName( WPARAM wParam, LPARAM lParam )
{
	mir_snprintf(( char* )lParam, wParam, "%s", msnProtocolName );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetStatus - obtain the protocol status

static int MsnGetStatus(WPARAM wParam,LPARAM lParam)
{
	return msnStatusMode;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnLoadIcon - obtain the protocol icon

static int MsnLoadIcon(WPARAM wParam,LPARAM lParam)
{
	if ( LOWORD( wParam ) == PLI_PROTOCOL )
		return (int)CopyIcon( LoadIconEx( "main" ));

	return (int)(HICON)NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnRecvFile - creates a database event from the file request been received

int MsnRecvFile( WPARAM wParam, LPARAM lParam )
{
	return MSN_CallService( MS_PROTO_RECVFILE, wParam, lParam );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnRecvMessage - creates a database event from the message been received

static int MsnRecvMessage(WPARAM wParam,LPARAM lParam)
{
	CCSDATA *ccs = ( CCSDATA* )lParam;
	if (Lists_IsInList( LIST_FL, ( HANDLE )ccs->hContact ))
		DBDeleteContactSetting( ccs->hContact, "CList", "Hidden" );

	return MSN_CallService( MS_PROTO_RECVMSG, wParam, lParam );
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

	if ( MSN_IsMeByContact( ccs->hContact )) return 0;

	char** files = ( char** )ccs->lParam;

	filetransfer* sft = new filetransfer();
	sft->std.files = files;
	sft->std.hContact = ccs->hContact;
	sft->std.sending = true;

	while ( files[sft->std.totalFiles] != NULL ) {
		struct _stat statbuf;
		if ( _stat( files[sft->std.totalFiles], &statbuf ) == 0 )
			sft->std.totalBytes += statbuf.st_size;

		++sft->std.totalFiles;
	}

	if ( sft->openNext() == -1 ) {
		delete sft;
		return 0;
	}

	DWORD dwFlags = MSN_GetDword( ccs->hContact, "FlagBits", 0 );
	if ( dwFlags & 0xf0000000 )
		p2p_invite( ccs->hContact, MSN_APPID_FILE, sft );
	else
		msnftp_invite( sft );

	MSN_SendBroadcast( ccs->hContact, ACKTYPE_FILE, ACKRESULT_SENTREQUEST, sft, 0 );
	return (int)(HANDLE)sft;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnSendMessage - sends the message to a server

struct TFakeAckParams
{
	inline TFakeAckParams( HANDLE p2, long p3, const char* p4 ) :
		hContact( p2 ),
		id( p3 ),
		msg( p4 )
		{}

	HANDLE	hContact;
	long	id;
	const char*	msg;
};

static void sttFakeAck( LPVOID param )
{
	TFakeAckParams* tParam = ( TFakeAckParams* )param;

	Sleep( 150 );
	MSN_SendBroadcast( tParam->hContact, ACKTYPE_MESSAGE, tParam->msg ? ACKRESULT_FAILED : ACKRESULT_SUCCESS,
		( HANDLE )tParam->id, LPARAM( tParam->msg ));

	delete tParam;
}

static void sttSendOim( LPVOID param )
{
	TFakeAckParams* tParam = ( TFakeAckParams* )param;

	char tEmail[ MSN_MAX_EMAIL_LEN ];
	MSN_GetStaticString( "e-mail", tParam->hContact, tEmail, sizeof( tEmail ));

	int seq = MSN_SendOIM(tEmail, tParam->msg);

	char* errMsg = NULL;
	switch (seq)
	{
		case -1:
			errMsg = MSN_Translate( "Offline messages could not be sent to this contact" );
			break;

		case -2:
			errMsg = MSN_Translate( "You sent too many offline messages and have been locked out" );
			break;

		case -3:
			errMsg = MSN_Translate( "You are not allowed to send offline messages to this user" );
			break;
	}
	MSN_SendBroadcast( tParam->hContact, ACKTYPE_MESSAGE, errMsg ? ACKRESULT_FAILED : ACKRESULT_SUCCESS,
		( HANDLE )tParam->id, ( LPARAM )errMsg );

	mir_free(( void* )tParam->msg );
	delete tParam;
}

static int MsnSendMessage( WPARAM wParam, LPARAM lParam )
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	char *errMsg = NULL;

	char tEmail[ MSN_MAX_EMAIL_LEN ];
	if ( MSN_IsMeByContact( ccs->hContact, tEmail )) 
	{
		errMsg = MSN_Translate( "You cannot send message to yourself" );
		mir_forkthread( sttFakeAck, new TFakeAckParams( ccs->hContact, 999999, errMsg ));
		return 999999;
	}

	char *msg = ( char* )ccs->lParam;
	if ( msg == NULL ) return 0;

	if ( ccs->wParam & PREF_UNICODE ) {
		char* p = strchr(msg, '\0');
		if (p != msg) {
			while ( *(++p) == '\0' );
			msg = mir_utf8encodeW(( wchar_t* )p );
		}
		else
			msg = mir_strdup( msg );
	}
	else if ( ccs->wParam & PREF_UTF )
		msg = mir_strdup( msg );
	else
		msg = mir_utf8encode( msg );

	if (strncmp(tEmail, "tel:", 4) == 0)
	{
		long id;
		if ( strlen( msg ) > 133 ) {
			errMsg = MSN_Translate( "Message is too long: SMS page limited to 133 UTF8 chars" );
			id = 999997;
		}
		else
		{
			errMsg = NULL;
			id = MSN_SendSMS(tEmail, msg);
		}
		mir_free( msg );
		mir_forkthread( sttFakeAck, new TFakeAckParams( ccs->hContact, id, errMsg ));
		return id;
	}

	if ( strlen( msg ) > 1202 ) {
		errMsg = MSN_Translate( "Message is too long: MSN messages are limited by 1202 UTF8 chars" );
		mir_free( msg );

		mir_forkthread( sttFakeAck, new TFakeAckParams( ccs->hContact, 999999, errMsg ));
		return 999996;
	}

	int seq, msgType = ( MyOptions.SlowSend ) ? 'A' : 'N';
	int rtlFlag = ( ccs->wParam & PREF_RTL ) ? MSG_RTL : 0;
	
	bool isOffline;
	ThreadData* thread = MSN_StartSB(ccs->hContact, isOffline);
	if ( thread == NULL )
	{
		if ( isOffline ) 
		{
			seq = rand();
			mir_forkthread( sttSendOim, new TFakeAckParams( ccs->hContact, seq, msg ));
			return seq;
		}
		else
			seq = MsgQueue_Add( ccs->hContact, msgType, msg, 0, 0, rtlFlag );
	}
	else
	{
		seq = thread->sendMessage( msgType, msg, rtlFlag );
		if ( !MyOptions.SlowSend )
			mir_forkthread( sttFakeAck, new TFakeAckParams( ccs->hContact, seq, 0 ));
	}

	mir_free( msg );
	return seq;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnSendNudge - Sending a nudge

static int MsnSendNudge( WPARAM wParam, LPARAM lParam )
{
	if ( !msnLoggedIn ) return 0;

	HANDLE hContact = ( HANDLE )wParam;

	if ( MSN_IsMeByContact( hContact )) return 0;

	static const char nudgemsg[] = 
		"MIME-Version: 1.0\r\n"
		"Content-Type: text/x-msnmsgr-datacast\r\n\r\n"
		"ID: 1\r\n\r\n";

	bool isOffline;
	ThreadData* thread = MSN_StartSB(hContact, isOffline);
	if ( thread == NULL )
	{
		if (isOffline) return 0; 
		MsgQueue_Add( hContact, 'N', nudgemsg, -1 );
	}
	else
		thread->sendMessage( 'N', nudgemsg, MSG_DISABLE_HDR );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	MsnSetApparentMode - controls contact visibility

static int MsnSetApparentMode( WPARAM wParam, LPARAM lParam )
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	if ( ccs->wParam && ccs->wParam != ID_STATUS_OFFLINE )
	  return 1;

	WORD oldMode = MSN_GetWord( ccs->hContact, "ApparentMode", 0 );
	if ( ccs->wParam != oldMode )
		MSN_SetWord( ccs->hContact, "ApparentMode", ( WORD )ccs->wParam );

	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	MsnSetAvatar - sets an avatar without UI

static int MsnSetAvatar( WPARAM wParam, LPARAM lParam )
{
	if (avsPresent < 0) avsPresent = ServiceExists(MS_AV_SETMYAVATAR) != 0;
	if (!avsPresent) return 1;

	char* szFileName = ( char* )lParam;

	if (szFileName == NULL)
	{
		char tFileName[ MAX_PATH ];
		MSN_GetAvatarFileName( NULL, tFileName, sizeof( tFileName ));
		remove( tFileName );
		MSN_DeleteSetting( NULL, "PictObject" );
	}
	else
	{
		int fileId = _open( szFileName, _O_RDONLY | _O_BINARY, _S_IREAD );
		if ( fileId == -1 )
			return 1;

		long  dwPngSize = _filelength( fileId );
		BYTE* pResult = new BYTE[ dwPngSize ];
		if ( pResult == NULL )
			return 2;

		_read( fileId, pResult, dwPngSize );
		_close( fileId );

		mir_sha1_ctx sha1ctx;
		BYTE sha1c[ MIR_SHA1_HASH_SIZE ], sha1d[ MIR_SHA1_HASH_SIZE ];
		char szSha1c[ 40 ], szSha1d[ 40 ];
		mir_sha1_init( &sha1ctx );
		mir_sha1_append( &sha1ctx, pResult, dwPngSize );
		mir_sha1_finish( &sha1ctx, sha1d );
		{	NETLIBBASE64 nlb = { szSha1d, sizeof( szSha1d ), ( PBYTE )sha1d, sizeof( sha1d ) };
			MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb ));
		}
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		_splitpath( szFileName, drive, dir, fname, ext );
		mir_sha1_init( &sha1ctx );
		ezxml_t xmlp = ezxml_new("msnobj");

		mir_sha1_append( &sha1ctx, ( PBYTE )"Creator", 7 );
		mir_sha1_append( &sha1ctx, ( PBYTE )MyOptions.szEmail, strlen( MyOptions.szEmail ));
		ezxml_set_attr(xmlp, "Creator", MyOptions.szEmail);

		char szFileSize[ 20 ];
		_ltoa( dwPngSize, szFileSize, 10 );
		mir_sha1_append( &sha1ctx, ( PBYTE )"Size", 4 );
		mir_sha1_append( &sha1ctx, ( PBYTE )szFileSize, strlen( szFileSize ));
		ezxml_set_attr(xmlp, "Size", szFileSize);

		mir_sha1_append( &sha1ctx, ( PBYTE )"Type", 4 );
		mir_sha1_append( &sha1ctx, ( PBYTE )"3", 1 );  // MSN_TYPEID_DISPLAYPICT
		ezxml_set_attr(xmlp, "Type", "3");

		mir_sha1_append( &sha1ctx, ( PBYTE )"Location", 8 );
		mir_sha1_append( &sha1ctx, ( PBYTE )fname, sizeof( fname ));
		ezxml_set_attr(xmlp, "Location", fname);

		mir_sha1_append( &sha1ctx, ( PBYTE )"Friendly", 8 );
		mir_sha1_append( &sha1ctx, ( PBYTE )"AAA=", 4 );
		ezxml_set_attr(xmlp, "Friendly", "AAA=");

		mir_sha1_append( &sha1ctx, ( PBYTE )"SHA1D", 5 );
		mir_sha1_append( &sha1ctx, ( PBYTE )szSha1d, strlen( szSha1d ));
		ezxml_set_attr(xmlp, "SHA1D", szSha1d);
		
		mir_sha1_finish( &sha1ctx, sha1c );

		{	NETLIBBASE64 nlb = { szSha1c, sizeof( szSha1c ), ( PBYTE )sha1c, sizeof( sha1c ) };
			MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb ));
			ezxml_set_attr(xmlp, "SHA1C", szSha1c);
		}
		{
			char* szBuffer = ezxml_toxml(xmlp, false);
			char szEncodedBuffer[ 2000 ];
			UrlEncode( szBuffer, szEncodedBuffer, sizeof( szEncodedBuffer ));
			free(szBuffer);
			ezxml_free(xmlp);

			MSN_SetString( NULL, "PictObject", szEncodedBuffer );
		}
		{	char tFileName[ MAX_PATH ];
			MSN_GetAvatarFileName( NULL, tFileName, sizeof( tFileName ));
			int fileId = _open( tFileName, _O_CREAT | _O_TRUNC | _O_WRONLY | O_BINARY,  _S_IREAD | _S_IWRITE );
			if ( fileId > -1 ) {
				_write( fileId, pResult, dwPngSize );
				_close( fileId );
		}	}

		delete pResult;
	}

	MSN_SetServerStatus( msnStatusMode );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnSetAwayMsg - sets the current status message for a user

static int MsnSetAwayMsg(WPARAM wParam,LPARAM lParam)
{
	int i;

	for ( i=0; i < MSN_NUM_MODES; i++ )
		if ( msnModeMsgs[i].m_mode == (int)wParam )
			break;

	if ( i == MSN_NUM_MODES )
		return 1;

	replaceStr( msnModeMsgs[i].m_msg, ( char* )lParam );

	if ( (int)wParam == msnDesiredStatus )
		MSN_SendStatusMessage(( char* )lParam );

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	SetNickname - sets a nick name without UI

static int MsnSetNickName( WPARAM wParam, LPARAM lParam )
{
	MSN_SendNickname(( char* )lParam );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	GetCurrentMedia - get current media

static int MsnGetCurrentMedia(WPARAM wParam, LPARAM lParam)
{
	LISTENINGTOINFO *cm = (LISTENINGTOINFO *)lParam;

	if (cm == NULL || cm->cbSize != sizeof(LISTENINGTOINFO))
		return -1;

	cm->ptszArtist = mir_tstrdup( msnCurrentMedia.ptszArtist );
	cm->ptszAlbum = mir_tstrdup( msnCurrentMedia.ptszAlbum );
	cm->ptszTitle = mir_tstrdup( msnCurrentMedia.ptszTitle );
	cm->ptszTrack = mir_tstrdup( msnCurrentMedia.ptszTrack );
	cm->ptszYear = mir_tstrdup( msnCurrentMedia.ptszYear );
	cm->ptszGenre = mir_tstrdup( msnCurrentMedia.ptszGenre );
	cm->ptszLength = mir_tstrdup( msnCurrentMedia.ptszLength );
	cm->ptszPlayer = mir_tstrdup( msnCurrentMedia.ptszPlayer );
	cm->ptszType = mir_tstrdup( msnCurrentMedia.ptszType );
	cm->dwFlags = msnCurrentMedia.dwFlags;

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	SetCurrentMedia - set current media

static int MsnSetCurrentMedia(WPARAM wParam, LPARAM lParam)
{
	// Clear old info
	mir_free( msnCurrentMedia.ptszArtist );
	mir_free( msnCurrentMedia.ptszAlbum );
	mir_free( msnCurrentMedia.ptszTitle );
	mir_free( msnCurrentMedia.ptszTrack );
	mir_free( msnCurrentMedia.ptszYear );
	mir_free( msnCurrentMedia.ptszGenre );
	mir_free( msnCurrentMedia.ptszLength );
	mir_free( msnCurrentMedia.ptszPlayer );
	mir_free( msnCurrentMedia.ptszType );
	memset(&msnCurrentMedia, 0, sizeof(msnCurrentMedia));

	// Copy new info
	LISTENINGTOINFO *cm = (LISTENINGTOINFO *)lParam;
	if (cm != NULL && cm->cbSize == sizeof(LISTENINGTOINFO) && (cm->ptszArtist != NULL || cm->ptszTitle != NULL))
	{
		bool unicode = (cm->dwFlags & LTI_UNICODE) != 0;

		msnCurrentMedia.cbSize = sizeof(msnCurrentMedia);	// Marks that there is info set
		msnCurrentMedia.dwFlags = LTI_TCHAR;

		overrideStr( msnCurrentMedia.ptszType, cm->ptszType, unicode, _T("Music") );
		overrideStr( msnCurrentMedia.ptszArtist, cm->ptszArtist, unicode );
		overrideStr( msnCurrentMedia.ptszAlbum, cm->ptszAlbum, unicode );
		overrideStr( msnCurrentMedia.ptszTitle, cm->ptszTitle, unicode, _T("No Title") );
		overrideStr( msnCurrentMedia.ptszTrack, cm->ptszTrack, unicode );
		overrideStr( msnCurrentMedia.ptszYear, cm->ptszYear, unicode );
		overrideStr( msnCurrentMedia.ptszGenre, cm->ptszGenre, unicode );
		overrideStr( msnCurrentMedia.ptszLength, cm->ptszLength, unicode );
		overrideStr( msnCurrentMedia.ptszPlayer, cm->ptszPlayer, unicode );
	}

	// Set user text
	if (msnCurrentMedia.cbSize == 0)
		MSN_DeleteSetting( NULL, "ListeningTo" );
	else
	{
		TCHAR *text;
		if (ServiceExists(MS_LISTENINGTO_GETPARSEDTEXT))
			text = (TCHAR *) CallService(MS_LISTENINGTO_GETPARSEDTEXT, (WPARAM) _T("%title% - %artist%"), (LPARAM) &msnCurrentMedia );
		else
		{
			text = (TCHAR *) mir_alloc( 128 * sizeof(TCHAR) );
			mir_sntprintf( text, 128, _T("%s - %s"), ( msnCurrentMedia.ptszTitle ? msnCurrentMedia.ptszTitle : _T("") ),
													 ( msnCurrentMedia.ptszArtist ? msnCurrentMedia.ptszArtist : _T("") ) );
		}
		MSN_SetStringT(NULL, "ListeningTo", text);
		mir_free(text);
	}

	// Send it
	int i;
	for ( i=0; i < MSN_NUM_MODES; i++ ) {
		if ( msnModeMsgs[i].m_mode == msnDesiredStatus ) {
			MSN_SendStatusMessage( msnModeMsgs[i].m_msg );
			break;
	}	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnSetStatus - set the plugin's connection status

static int MsnSetStatus( WPARAM wParam, LPARAM lParam )
{
	if (msnDesiredStatus == wParam) return 0;

	msnDesiredStatus = wParam;
	MSN_DebugLog( "PS_SETSTATUS(%d,0)", wParam );

	if ( msnDesiredStatus == ID_STATUS_OFFLINE )
	{
		MSN_GoOffline();
	}
	else if (!msnLoggedIn && !(msnStatusMode>=ID_STATUS_CONNECTING && msnStatusMode<ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES))
	{
		char szPassword[ 100 ];
		int ps = MSN_GetStaticString( "Password", NULL, szPassword, sizeof( szPassword ));
		if (ps != 0  || *szPassword == 0) {
			MSN_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD );
			msnDesiredStatus = msnStatusMode;
			return 0;
		}	
		 
		if (*MyOptions.szEmail == 0) {
			MSN_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_BADUSERID );
			msnDesiredStatus = msnStatusMode;
			return 0;
		}	

		MyOptions.UseProxy = MSN_GetByte( "NLUseProxy", FALSE ) != 0;

		ThreadData* newThread = new ThreadData;

		newThread->mType = SERVER_DISPATCH;
		newThread->mIsMainThread = true;
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
	if ( !msnLoggedIn )
		return 0;

	HANDLE hContact = ( HANDLE )wParam;

	if ( MSN_IsMeByContact( hContact )) return 0;

	bool typing = lParam == PROTOTYPE_SELFTYPING_ON;

	bool isOffline;
	ThreadData* thread = MSN_StartSB(hContact, isOffline);

	if ( thread == NULL ) 
	{
		if (isOffline) return 0;
		MsgQueue_Add( hContact, 2571, NULL, 0, NULL, typing );
	}
	else
		MSN_StartStopTyping( thread, typing );

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetUnread - returns the actual number of unread emails in the INBOX
extern int mUnreadMessages;

static int MsnGetUnreadEmailCount(WPARAM wParam, LPARAM lParam)
{
	if ( !msnLoggedIn )
		return 0;
	return mUnreadMessages;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Services initialization and destruction

int CompareHandles( const void* p1, const void* p2 );
LIST<void> arServices( 20, CompareHandles );

int LoadMsnServices( void )
{
	arServices.insert( MSN_CreateProtoServiceFunction( PS_ADDTOLIST,        MsnAddToList ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_ADDTOLISTBYEVENT, MsnAddToListByEvent ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_AUTHALLOW,        MsnAuthAllow ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_AUTHDENY,         MsnAuthDeny ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_BASICSEARCH,      MsnBasicSearch ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_FILERESUME,       MsnFileResume ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_GETAVATARINFO,    MsnGetAvatarInfo ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_GETCAPS,          MsnGetCaps ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_GETNAME,          MsnGetName ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_GETSTATUS,        MsnGetStatus ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_LOADICON,         MsnLoadIcon ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_SEARCHBYEMAIL,    MsnBasicSearch ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_SETSTATUS,        MsnSetStatus ));

	arServices.insert( MSN_CreateProtoServiceFunction( PSR_AUTH,            MsnRecvAuth ));
	arServices.insert( MSN_CreateProtoServiceFunction( PSR_FILE,            MsnRecvFile ));
	arServices.insert( MSN_CreateProtoServiceFunction( PSR_MESSAGE,         MsnRecvMessage ));

	arServices.insert( MSN_CreateProtoServiceFunction( PSS_FILE,            MsnSendFile ));
	arServices.insert( MSN_CreateProtoServiceFunction( PSS_FILEALLOW,       MsnFileAllow ));
	arServices.insert( MSN_CreateProtoServiceFunction( PSS_FILECANCEL,      MsnFileCancel ));
	arServices.insert( MSN_CreateProtoServiceFunction( PSS_FILEDENY,        MsnFileDeny ));
//	arServices.insert( MSN_CreateProtoServiceFunction( PSS_GETINFO,         MsnGetInfo ));
	arServices.insert( MSN_CreateProtoServiceFunction( PSS_MESSAGE,         MsnSendMessage ));
	arServices.insert( MSN_CreateProtoServiceFunction( PSS_SETAPPARENTMODE, MsnSetApparentMode ));
	arServices.insert( MSN_CreateProtoServiceFunction( PSS_USERISTYPING,    MsnUserIsTyping ));

	arServices.insert( MSN_CreateProtoServiceFunction( PSS_GETAWAYMSG,		MsnGetAwayMsg ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_SETAWAYMSG,		MsnSetAwayMsg ));

	arServices.insert( MSN_CreateProtoServiceFunction( PS_GETMYAVATAR,      MsnGetAvatar ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_SETMYAVATAR,      MsnSetAvatar ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_GETAVATARCAPS,    MsnGetAvatarCaps ));

	arServices.insert( MSN_CreateProtoServiceFunction( PS_SET_LISTENINGTO,  MsnSetCurrentMedia ));
	arServices.insert( MSN_CreateProtoServiceFunction( PS_GET_LISTENINGTO,  MsnGetCurrentMedia ));

	arServices.insert( MSN_CreateProtoServiceFunction( MSN_SET_NICKNAME,    MsnSetNickName ));
	arServices.insert( MSN_CreateProtoServiceFunction( MSN_SEND_NUDGE,      MsnSendNudge ));

	arServices.insert( MSN_CreateProtoServiceFunction( MSN_GETUNREAD_EMAILCOUNT, MsnGetUnreadEmailCount ));
	return 0;
}

void UnloadMsnServices( void )
{
	for ( int i=0; i < arServices.getCount(); i++ )
		DestroyServiceFunction( arServices[i] );
	arServices.destroy();
}
