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

extern int avsPresent;

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetName - obtain the protocol name

int CMsnProto::GetName( WPARAM wParam, LPARAM lParam )
{
	mir_snprintf(( char* )lParam, wParam, "%s", m_szProtoName );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetStatus - obtain the protocol status

int CMsnProto::GetStatus(WPARAM wParam,LPARAM lParam)
{
	return m_iStatus;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetAvatar - retrieves the file name of my own avatar

int CMsnProto::GetAvatar(WPARAM wParam, LPARAM lParam)
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

void CMsnProto::sttFakeAvatarAck( void* arg )
{
	Sleep( 100 );
	SendBroadcast((( PROTO_AVATAR_INFORMATION* )arg )->hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, arg, 0 );
}

int CMsnProto::GetAvatarInfo(WPARAM wParam,LPARAM lParam)
{
	if (avsPresent < 0) avsPresent = ServiceExists(MS_AV_SETMYAVATAR) != 0;
	if (!avsPresent) return GAIR_NOAVATAR;

	PROTO_AVATAR_INFORMATION* AI = ( PROTO_AVATAR_INFORMATION* )lParam;

	if (( getDword( AI->hContact, "FlagBits", 0 ) & 0xf0000000 ) == 0 )
		return GAIR_NOAVATAR;

	char *szContext;
	DBVARIANT dbv;
	if (DBGetContactSettingString(AI->hContact, m_szProtoName, AI->hContact ? "PictContext" : "PictObject", &dbv) == 0)
	{
		szContext = (char*)NEWSTR_ALLOCA(dbv.pszVal);
		MSN_FreeVariant( &dbv );
	}
	else
		return GAIR_NOAVATAR;

	if ( MSN_IsMeByContact( AI->hContact ))
	{
		MSN_GetAvatarFileName( NULL, AI->filename, sizeof( AI->filename ));
		AI->format = PA_FORMAT_PNG;
		return 	(_access(AI->filename, 4) ? GAIR_NOAVATAR : GAIR_SUCCESS);
	}

	MSN_GetAvatarFileName( AI->hContact, AI->filename, sizeof( AI->filename ));
	AI->format = PA_FORMAT_UNKNOWN;

	if ( AI->hContact ) 
	{
		size_t len = strlen( AI->filename );
		strcpy( AI->filename + len, "*" );

		_finddata_t c_file;
		long hFile = _findfirst(AI->filename, &c_file);

		// Find first .c file in current directory 
		if (hFile == -1L)
		{
			strcpy( AI->filename + len, "unk" );
		}
		else
		{
			char *ext = strrchr(c_file.name, '.');
			if (ext != NULL) 
			{
				_strlwr(++ext);
				
				if (strcmp(ext, "png") == 0)
				{
					AI->format = PA_FORMAT_PNG;
					strcpy( AI->filename + len, "png" );
				}
				else if (strcmp(ext, "jpg") == 0)
				{
					AI->format = PA_FORMAT_JPEG;
					strcpy( AI->filename + len, "jpg" );
				}
				else if (strcmp(ext, "gif") == 0)
				{
					AI->format = PA_FORMAT_GIF;
					strcpy( AI->filename + len, "gif" );
				}
				else if (strcmp(ext, "bmp") == 0)
				{
					AI->format = PA_FORMAT_BMP;
					strcpy( AI->filename + len, "bmp" );
				}
				else
					strcpy( AI->filename + len, "unk" );
			}
			else
				strcpy( AI->filename + len, "unk" );

			_findclose(hFile);
		}
	}
	else {
		if ( _access( AI->filename, 0 ) == 0 )
			AI->format = PA_FORMAT_PNG;
	}

	if ( AI->format != PA_FORMAT_UNKNOWN ) 
	{
		bool needupdate = true;
		if (DBGetContactSettingString(AI->hContact, m_szProtoName, "PictSavedContext", &dbv) == 0)
		{
			needupdate = strcmp(dbv.pszVal, szContext) != 0;
			MSN_FreeVariant( &dbv );
		}
		if (needupdate)
		{
			setString( AI->hContact, "PictSavedContext", szContext );

			// Store also avatar hash
			char* szAvatarHash = MSN_GetAvatarHash(szContext);
			if (szAvatarHash != NULL)
			{
				setString( AI->hContact, "AvatarSavedHash", szAvatarHash );
				mir_free(szAvatarHash);
			}
		}
		return GAIR_SUCCESS;
	}

	if (( wParam & GAIF_FORCE ) != 0 && AI->hContact != NULL )
	{
		WORD wStatus = getWord( AI->hContact, "Status", ID_STATUS_OFFLINE );
		if ( wStatus == ID_STATUS_OFFLINE ) 
		{
			deleteSetting( AI->hContact, "AvatarHash" );
			PROTO_AVATAR_INFORMATION* fakeAI = new PROTO_AVATAR_INFORMATION;
			*fakeAI = *AI;
			ForkThread( &CMsnProto::sttFakeAvatarAck, fakeAI );
		}
		else 
		{
			if ( p2p_getAvatarSession( AI->hContact ) == NULL )
			{
				filetransfer* ft = new filetransfer(this);
				ft->std.hContact = AI->hContact;
				ft->p2p_object = mir_strdup(szContext);
				ft->std.currentFile = mir_strdup(AI->filename);

				p2p_invite(AI->hContact, MSN_APPID_AVATAR, ft);
			}
		}
		return GAIR_WAITFOR;
	}
	return GAIR_NOAVATAR;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetAvatarCaps - retrieves avatar capabilities

int CMsnProto::GetAvatarCaps(WPARAM wParam, LPARAM lParam)
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
//	MsnSetAvatar - sets an avatar without UI

int CMsnProto::SetAvatar( WPARAM wParam, LPARAM lParam )
{
	if (avsPresent < 0) avsPresent = ServiceExists(MS_AV_SETMYAVATAR) != 0;
	if (!avsPresent) return 1;

	char* szFileName = ( char* )lParam;

	if (szFileName == NULL)
	{
		char tFileName[ MAX_PATH ];
		MSN_GetAvatarFileName( NULL, tFileName, sizeof( tFileName ));
		remove( tFileName );
		deleteSetting( NULL, "PictObject" );
	}
	else
	{
		int fileId = _open( szFileName, _O_RDONLY | _O_BINARY, _S_IREAD );
		if ( fileId == -1 )
			return 1;

		long  dwPngSize = _filelength( fileId );
		BYTE* pResult = (BYTE*)mir_alloc(dwPngSize);
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

			setString( NULL, "PictObject", szEncodedBuffer );
		}
		{	char tFileName[ MAX_PATH ];
			MSN_GetAvatarFileName( NULL, tFileName, sizeof( tFileName ));
			int fileId = _open( tFileName, _O_CREAT | _O_TRUNC | _O_WRONLY | O_BINARY,  _S_IREAD | _S_IWRITE );
			if ( fileId > -1 ) {
				_write( fileId, pResult, dwPngSize );
				_close( fileId );
			}
			else
				MSN_ShowError("Cannot set avatar. File '%s' could not be created/overwritten", tFileName);

		}

		mir_free(pResult);
	}

	MSN_SetServerStatus( m_iStatus );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	SetNickname - sets a nick name without UI

int CMsnProto::SetNickName( WPARAM wParam, LPARAM lParam )
{
	MSN_SendNicknameA(( char* )lParam );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnSendNudge - Sending a nudge

int CMsnProto::SendNudge( WPARAM wParam, LPARAM lParam )
{
	if ( !msnLoggedIn ) return 0;

	HANDLE hContact = ( HANDLE )wParam;

	char tEmail[ MSN_MAX_EMAIL_LEN ];
	if ( MSN_IsMeByContact( hContact, tEmail )) return 0;

	static const char nudgemsg[] = 
		"MIME-Version: 1.0\r\n"
		"Content-Type: text/x-msnmsgr-datacast\r\n\r\n"
		"ID: 1\r\n\r\n";

	int netId = Lists_GetNetId(tEmail);

	switch (netId)
	{
	case NETID_UNKNOWN:
		hContact = MSN_GetChatInernalHandle(hContact);

	case NETID_MSN:
	case NETID_LCS:
		{
			bool isOffline;
			ThreadData* thread = MSN_StartSB(hContact, isOffline);
			if ( thread == NULL )
			{
				if (isOffline) return 0; 
				MsgQueue_Add( hContact, 'N', nudgemsg, -1 );
			}
			else
			{
				int tNnetId = netId == NETID_UNKNOWN ? NETID_MSN : netId;
				thread->sendMessage( 'N', tEmail, tNnetId, nudgemsg, MSG_DISABLE_HDR );
			}
		}
		break;

	case NETID_YAHOO:
		msnNsThread->sendMessage( '3', tEmail, netId, nudgemsg, MSG_DISABLE_HDR );
		break;

	default:
		break;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	GetCurrentMedia - get current media

int CMsnProto::GetCurrentMedia(WPARAM wParam, LPARAM lParam)
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

int CMsnProto::SetCurrentMedia(WPARAM wParam, LPARAM lParam)
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
		deleteSetting( NULL, "ListeningTo" );
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
		setTString("ListeningTo", text);
		mir_free(text);
	}

	// Send it
	char** msgptr = GetStatusMsgLoc(m_iDesiredStatus);
	if ( msgptr != NULL )
		MSN_SendStatusMessage( *msgptr );

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnContactDeleted - called when a contact is deleted from list

int CMsnProto::OnContactDeleted( WPARAM wParam, LPARAM lParam )
{
	const HANDLE hContact = ( HANDLE )wParam;

	if ( !msnLoggedIn )  //should never happen for MSN contacts
		return 0;

	if ( !MSN_IsMyContact( hContact ))
		return 0;

	int type = DBGetContactSettingByte( hContact, m_szProtoName, "ChatRoom", 0 );
	if ( type != 0 ) 
	{
		DBVARIANT dbv;
		if ( !getTString( hContact, "ChatRoomID", &dbv )) {
			MSN_KillChatSession( dbv.ptszVal );
			MSN_FreeVariant( &dbv );
		}	
	}
	else
	{
		char tEmail[ MSN_MAX_EMAIL_LEN ];
		if ( !getStaticString( hContact, "e-mail", tEmail, sizeof( tEmail ))) {
			MSN_DebugLog("Deleted Handler Email");
			MSN_AddUser( hContact, tEmail, 0, LIST_FL | LIST_REMOVE );
			MSN_AddUser( hContact, tEmail, 0, LIST_AL | LIST_REMOVE );

			if ( Lists_IsInList( LIST_RL, tEmail ))
				MSN_AddUser( hContact, tEmail, 0, LIST_BL );
			else 
				MSN_AddUser( hContact, tEmail, 0, LIST_BL | LIST_REMOVE );
		}
	}

	return 0;
}


int CMsnProto::OnGroupChange(WPARAM wParam,LPARAM lParam)
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
			if (szId != NULL) MSN_DeleteServerGroup(szId);	
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

int CMsnProto::OnDbSettingChanged(WPARAM wParam,LPARAM lParam)
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

	if ( !strcmp( cws->szSetting, "ApparentMode" )) 
	{
		if ( !MSN_IsMyContact( hContact ))
			return 0;

		char tEmail[ MSN_MAX_EMAIL_LEN ];
		if ( !getStaticString( hContact, "e-mail", tEmail, sizeof( tEmail ))) 
		{
			bool isBlocked = Lists_IsInList( LIST_BL, tEmail );

			if ( !isBlocked && cws->value.wVal == ID_STATUS_OFFLINE ) 
			{
				MSN_AddUser( hContact, tEmail, 0, LIST_AL + LIST_REMOVE );
				MSN_AddUser( hContact, tEmail, 0, LIST_BL );
			}
			else if ( isBlocked && cws->value.wVal == 0 ) 
			{
				MSN_AddUser( hContact, tEmail, 0, LIST_BL + LIST_REMOVE );
				MSN_AddUser( hContact, tEmail, 0, LIST_AL );
			}	
		}	
	}

	if ( !strcmp( cws->szModule, "CList" )) 
	{
		if ( !strcmp( cws->szSetting, "MyHandle" )) 
		{
			if ( !MSN_IsMyContact( hContact ))
				return 0;

			char szContactID[ 100 ];
			if ( !getStaticString( hContact, "ID", szContactID, sizeof( szContactID ))) 
			{
				if ( cws->value.type != DBVT_DELETED ) 
				{
					if ( cws->value.type == DBVT_UTF8 )
						MSN_ABUpdateNick(cws->value.pszVal, szContactID);
					else
						MSN_ABUpdateNick(UTF8(cws->value.pszVal), szContactID);
				}
				else
					MSN_ABUpdateNick(NULL, szContactID);
			}	
			return 0;
		}	
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnIdleChanged - transitions to Idle

int CMsnProto::OnIdleChanged(WPARAM wParam, LPARAM lParam)
{
	if ( !msnLoggedIn || m_iDesiredStatus != ID_STATUS_ONLINE )
		return 0;

    if ( lParam & IDF_PRIVACY ) {
		if ( m_iStatus == ID_STATUS_IDLE )
			MSN_SetServerStatus( ID_STATUS_ONLINE );
	}
    else
		MSN_SetServerStatus( lParam & IDF_ISIDLE ? ID_STATUS_IDLE : ID_STATUS_ONLINE );

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnWindowEvent - creates session on window open

int CMsnProto::OnWindowEvent(WPARAM wParam, LPARAM lParam)
{
	MessageWindowEventData* msgEvData  = (MessageWindowEventData*)lParam;

	if ( msgEvData->uType == MSG_WINDOW_EVT_OPENING ) {
		if ( !MSN_IsMyContact( msgEvData->hContact )) return 0;
		
		char tEmail[ MSN_MAX_EMAIL_LEN ];
		if ( MSN_IsMeByContact( msgEvData->hContact, tEmail )) return 0;

		int netId = Lists_GetNetId(tEmail);
		if (netId != NETID_MSN && netId != NETID_LCS) return 0;

		if (m_iStatus == ID_STATUS_OFFLINE || m_iStatus == ID_STATUS_INVISIBLE)
			return 0;

		bool isOffline;
		ThreadData* thread = MSN_StartSB(msgEvData->hContact, isOffline);
		
		if (thread == NULL && !isOffline)
			MsgQueue_Add( msgEvData->hContact, 'X', NULL, 0, NULL );
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetUnread - returns the actual number of unread emails in the INBOX

int CMsnProto::GetUnreadEmailCount(WPARAM wParam, LPARAM lParam)
{
	if ( !msnLoggedIn )
		return 0;
	return mUnreadMessages;
}
