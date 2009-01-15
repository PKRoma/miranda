/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2008-2009 Boris Krasnovskiy.

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
#include "msn_proto.h"

static const COLORREF crCols[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

int msn_httpGatewayInit(HANDLE hConn,NETLIBOPENCONNECTION *nloc,NETLIBHTTPREQUEST *nlhr);
int msn_httpGatewayBegin(HANDLE hConn,NETLIBOPENCONNECTION *nloc);
int msn_httpGatewayWrapSend(HANDLE hConn,PBYTE buf,int len,int flags,MIRANDASERVICE pfnNetlibSend);
PBYTE msn_httpGatewayUnwrapRecv(NETLIBHTTPREQUEST *nlhr,PBYTE buf,int len,int *outBufLen,void *(*NetlibRealloc)(void*,size_t));

static int CompareLists( const MsnContact* p1, const MsnContact* p2 )
{
	return _stricmp(p1->email, p2->email);
}

template< class T >  int CompareId( const T* p1, const T* p2 )
{
	return _stricmp(p1->id, p2->id);
}

template< class T > int ComparePtr( const T* p1, const T* p2 )
{
	return int( p1 - p2 );
}



CMsnProto::CMsnProto( const char* aProtoName, const TCHAR* aUserName ) :
	contList( 10, CompareLists ),
	grpList( 10, CompareId ),
	sttThreads( 10, ComparePtr ),
	sessionList( 10, ComparePtr ),
	dcList( 10, ComparePtr ),
	msgQueueList(1),
	msgCache(5, CompareId)
{
	char path[ MAX_PATH ];

	m_tszUserName = mir_tstrdup( aUserName );
	m_szModuleName = mir_strdup( aProtoName );
	m_szProtoName = mir_strdup( aProtoName );
	_strlwr( m_szProtoName );
	m_szProtoName[0] = (char)toupper( m_szProtoName[0] );

	mir_snprintf( path, sizeof( path ), "%s:HotmailNotify", m_szModuleName );
	ModuleName = mir_strdup( path );

	mir_snprintf( path, sizeof( path ), "%s/Status", m_szModuleName );
	MSN_CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/IdleTS", m_szModuleName );
	MSN_CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/p2pMsgId", m_szModuleName );
	MSN_CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/MobileEnabled", m_szModuleName );
	MSN_CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/MobileAllowed", m_szModuleName );
	MSN_CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	// Protocol services and events...
	hMSNNudge = CreateProtoEvent( "/Nudge" );

	CreateProtoService( PS_CREATEACCMGRUI,        &CMsnProto::SvcCreateAccMgrUI );

	CreateProtoService( PS_GETNAME,               &CMsnProto::GetName );
	CreateProtoService( PS_GETSTATUS,             &CMsnProto::GetStatus );
	CreateProtoService( PS_GETAVATARINFO,         &CMsnProto::GetAvatarInfo );

	CreateProtoService( PS_LEAVECHAT,             &CMsnProto::OnLeaveChat );

	CreateProtoService( PS_GETMYAVATAR,           &CMsnProto::GetAvatar );
	CreateProtoService( PS_SETMYAVATAR,           &CMsnProto::SetAvatar );
	CreateProtoService( PS_GETAVATARCAPS,         &CMsnProto::GetAvatarCaps );

	CreateProtoService( PS_GET_LISTENINGTO,       &CMsnProto::GetCurrentMedia );
	CreateProtoService( PS_SET_LISTENINGTO,       &CMsnProto::SetCurrentMedia );

	CreateProtoService( MSN_SET_NICKNAME,         &CMsnProto::SetNickName );
	CreateProtoService( MSN_SEND_NUDGE,           &CMsnProto::SendNudge );

	CreateProtoService( MSN_GETUNREAD_EMAILCOUNT, &CMsnProto::GetUnreadEmailCount );

	// service to get from protocol chat buddy info
//	CreateProtoService( MS_GC_PROTO_GETTOOLTIPTEXT, &CMsnProto::GCGetToolTipText );

	HookProtoEvent( ME_DB_CONTACT_DELETED,        &CMsnProto::OnContactDeleted );
	HookProtoEvent( ME_DB_CONTACT_SETTINGCHANGED, &CMsnProto::OnDbSettingChanged );
	HookProtoEvent( ME_MSG_WINDOWEVENT,           &CMsnProto::OnWindowEvent );
	HookProtoEvent( ME_CLIST_PREBUILDCONTACTMENU, &CMsnProto::OnPrebuildContactMenu );
	HookProtoEvent( ME_CLIST_GROUPCHANGE,         &CMsnProto::OnGroupChange );
	HookProtoEvent( ME_OPT_INITIALISE,            &CMsnProto::OnOptionsInit );
    
	LoadOptions();

	tridUrlInbox = -1;

	HANDLE hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) {
		if ( MSN_IsMyContact( hContact ))
		{
			deleteSetting( hContact, "Status" );
			deleteSetting( hContact, "IdleTS" );
			deleteSetting( hContact, "p2pMsgId" );
			deleteSetting( hContact, "AccList" );
//			DBDeleteContactSetting( hContact, "CList", "StatusMsg" );
		}
		hContact = ( HANDLE )MSN_CallService( MS_DB_CONTACT_FINDNEXT,( WPARAM )hContact, 0 );
	}
	deleteSetting( NULL, "MobileEnabled" );
	deleteSetting( NULL, "MobileAllowed" );

	if (getStaticString( NULL, "LoginServer", path, sizeof( path )) == 0 &&
		(strcmp(path, MSN_DEFAULT_LOGIN_SERVER) == 0 ||
		strcmp(path, MSN_DEFAULT_GATEWAY) == 0))
		deleteSetting( NULL, "LoginServer" );

	if (MyOptions.SlowSend)
	{
		if (DBGetContactSettingDword(NULL, "SRMsg", "MessageTimeout", 10000) < 60000) 
			DBWriteContactSettingDword(NULL, "SRMsg", "MessageTimeout", 60000);
		if (DBGetContactSettingDword(NULL, "SRMM", "MessageTimeout", 10000) < 60000) 
			DBWriteContactSettingDword(NULL, "SRMM", "MessageTimeout", 60000);
	}

	mailsoundname = ( char* )mir_alloc( 64 );
	mir_snprintf(mailsoundname, 64, "%s:Hotmail", m_szModuleName);
	SkinAddNewSoundEx(mailsoundname, m_szModuleName, MSN_Translate( "Live Mail" ));

	alertsoundname = ( char* )mir_alloc( 64 );
	mir_snprintf(alertsoundname, 64, "%s:Alerts", m_szModuleName);
	SkinAddNewSoundEx(alertsoundname, m_szModuleName, MSN_Translate( "Live Alert" ));

	m_iStatus = m_iDesiredStatus = ID_STATUS_OFFLINE;

	MSN_InitThreads();
	Lists_Init();
	MsgQueue_Init();
	P2pSessions_Init();
}

CMsnProto::~CMsnProto()
{
	MsnUninitMenus();

	DestroyHookableEvent( hMSNNudge );

	MSN_FreeGroups();
	Threads_Uninit();
	MsgQueue_Uninit();
	Lists_Uninit();
	P2pSessions_Uninit();
	CachedMsg_Uninit();

	Netlib_CloseHandle( hNetlibUser );
	Netlib_CloseHandle( hNetlibUserHttps );

	mir_free( mailsoundname );
	mir_free( alertsoundname );
	mir_free( m_tszUserName );
	mir_free( m_szModuleName );
	mir_free( ModuleName );

	for ( int i=0; i < MSN_NUM_MODES; i++ )
		mir_free( msnModeMsgs[ i ] );

	mir_free( passport );
	mir_free( rru );
	mir_free( urlId );

	mir_free( msnPreviousUUX );
	mir_free( msnExternalIP );
	mir_free( abchMigrated );

	FreeAuthTokens();
}


int CMsnProto::OnModulesLoaded( WPARAM wParam, LPARAM lParam )
{
	char szBuffer[ MAX_PATH ], szDbsettings[64];;
	char *name;

#ifdef UNICODE
	name = mir_u2a(m_tszUserName);
#else
	name = m_tszUserName;
#endif

	NETLIBUSER nlu1 = {0};
	nlu1.cbSize = sizeof( nlu1 );
	nlu1.flags = NUF_OUTGOING | NUF_HTTPCONNS;
	nlu1.szSettingsModule = szDbsettings;
	nlu1.szDescriptiveName = szBuffer;

	mir_snprintf( szDbsettings, sizeof(szDbsettings), "%s_HTTPS", m_szModuleName );
	mir_snprintf( szBuffer, sizeof(szBuffer), MSN_Translate("%s plugin HTTPS connections"), name );
	hNetlibUserHttps = ( HANDLE )MSN_CallService( MS_NETLIB_REGISTERUSER, 0, ( LPARAM )&nlu1 );

	NETLIBUSER nlu = {0};
	nlu.cbSize = sizeof( nlu );
	nlu.flags = NUF_INCOMING | NUF_OUTGOING | NUF_HTTPCONNS;
	nlu.szSettingsModule = m_szModuleName;
	nlu.szDescriptiveName = szBuffer;

	if ( MyOptions.UseGateway ) {
		nlu.flags |= NUF_HTTPGATEWAY;
		nlu.szHttpGatewayUserAgent = (char*)MSN_USER_AGENT;
		nlu.pfnHttpGatewayInit = msn_httpGatewayInit;
		nlu.pfnHttpGatewayWrapSend = msn_httpGatewayWrapSend;
		nlu.pfnHttpGatewayUnwrapRecv = msn_httpGatewayUnwrapRecv;
	}

	mir_snprintf( szBuffer, sizeof(szBuffer), MSN_Translate("%s plugin connections"), name );
	hNetlibUser = ( HANDLE )MSN_CallService( MS_NETLIB_REGISTERUSER, 0, ( LPARAM )&nlu );

	if ( getByte( "UseIeProxy", 0 )) {
		NETLIBUSERSETTINGS nls = { 0 };
		nls.cbSize = sizeof( nls );
		MSN_CallService(MS_NETLIB_GETUSERSETTINGS, WPARAM(hNetlibUserHttps), LPARAM(&nls));

		HKEY hSettings;
		if ( RegOpenKeyExA( HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings", 
			0, KEY_QUERY_VALUE, &hSettings ))
			return 0;

		char tValue[ 256 ];
		DWORD tType = REG_SZ, tValueLen = sizeof( tValue );
		int tResult = RegQueryValueExA( hSettings, "ProxyServer", NULL, &tType, ( BYTE* )tValue, &tValueLen );
		RegCloseKey( hSettings );

		if ( !tResult )
		{
			char* tDelim = strstr( tValue, "http=" );
			if ( tDelim != 0 ) {
				tDelim += 5;
				memmove(tValue, tDelim, strlen(tDelim)+1);

				tDelim = strchr( tValue, ';' );
				if ( tDelim != NULL )
					*tDelim = '\0';
			}

			tDelim = strchr( tValue, ':' );
			if ( tDelim != NULL ) {
				*tDelim = 0;
				nls.wProxyPort = atol( tDelim+1 );
			}

			rtrim( tValue );
			nls.szProxyServer = tValue;
			nls.useProxy = MyOptions.UseProxy = tValue[0] != 0;
			nls.proxyType = PROXYTYPE_HTTP;
			nls.szIncomingPorts = NEWSTR_ALLOCA(nls.szIncomingPorts);
			nls.szOutgoingPorts = NEWSTR_ALLOCA(nls.szOutgoingPorts);
			nls.szProxyAuthPassword = NEWSTR_ALLOCA(nls.szProxyAuthPassword);
			nls.szProxyAuthUser = NEWSTR_ALLOCA(nls.szProxyAuthUser);
			MSN_CallService(MS_NETLIB_SETUSERSETTINGS, WPARAM(hNetlibUserHttps), LPARAM(&nls));
		}	
	}
#ifdef UNICODE
	mir_free(name);
#endif

	if ( msnHaveChatDll ) 
	{
		GCREGISTER gcr = {0};
		gcr.cbSize = sizeof( GCREGISTER );
		gcr.dwFlags = GC_TYPNOTIF | GC_CHANMGR;
		gcr.iMaxText = 0;
		gcr.nColors = 16;
		gcr.pColors = (COLORREF*)crCols;
		gcr.pszModuleDispName = m_szModuleName;
		gcr.pszModule = m_szModuleName;
		CallServiceSync( MS_GC_REGISTER, 0, ( LPARAM )&gcr );

		HookProtoEvent( ME_GC_EVENT, &CMsnProto::MSN_GCEventHook );
		HookProtoEvent( ME_GC_BUILDMENU, &CMsnProto::MSN_GCMenuHook );

		char szEvent[ 200 ];
		mir_snprintf( szEvent, sizeof szEvent, "%s\\ChatInit", m_szModuleName );
		hInitChat = CreateHookableEvent( szEvent );
		HookProtoEvent( szEvent, &CMsnProto::MSN_ChatInit );
	}

	HookProtoEvent( ME_IDLE_CHANGED,              &CMsnProto::OnIdleChanged );

    MsnInitMenus();

	InitCustomFolders();

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnPreShutdown - prepare a global Miranda shutdown

int CMsnProto::OnPreShutdown( WPARAM wParam, LPARAM lParam )
{
	MSN_CloseThreads();
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////
// MsnAddToList - adds contact to the server list

HANDLE CMsnProto::AddToListByEmail( const char *email, DWORD flags )
{
	HANDLE hContact = MSN_HContactFromEmail(email, email, true, flags & PALF_TEMPORARY);

	if ( flags & PALF_TEMPORARY ) 
	{
		if (DBGetContactSettingByte( hContact, "CList", "NotOnList", 0 ) == 1) 
			DBWriteContactSettingByte( hContact, "CList", "Hidden", 1 );
	}
	else 
	{
		if ( msnLoggedIn ) 
		{
			int netId = Lists_GetNetId(email);
			if (netId == NETID_UNKNOWN || netId == NETID_EMAIL)
				netId = strncmp(email, "tel:", 4) == 0 ? NETID_MOB : NETID_MSN;
			if (MSN_AddUser( hContact, email, netId, LIST_FL ))
			{
				MSN_AddUser( hContact, email, netId, LIST_PL + LIST_REMOVE );
				MSN_AddUser( hContact, email, netId, LIST_BL + LIST_REMOVE );
				MSN_AddUser( hContact, email, netId, LIST_AL );
				DBDeleteContactSetting( hContact, "CList", "Hidden" );
			}
			MSN_SetContactDb(hContact, email);
		}
		else hContact = NULL;
	}
	return hContact;
}

HANDLE __cdecl CMsnProto::AddToList( int flags, PROTOSEARCHRESULT* psr )
{
	return AddToListByEmail( psr->email, flags );
}

HANDLE __cdecl CMsnProto::AddToListByEvent( int flags, int iContact, HANDLE hDbEvent )
{
	DBEVENTINFO dbei = {0};
	dbei.cbSize = sizeof(dbei);
	if (( dbei.cbBlob = MSN_CallService( MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hDbEvent, 0 )) == (DWORD)(-1))
		return NULL;

	dbei.pBlob=(PBYTE) alloca(dbei.cbBlob);
	if ( MSN_CallService(MS_DB_EVENT_GET, ( WPARAM )hDbEvent, ( LPARAM )&dbei))	return NULL;
	if ( strcmp(dbei.szModule, m_szModuleName))					return NULL;
	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST)					return NULL;

	char* nick = (char *) (dbei.pBlob + sizeof(DWORD) + sizeof(HANDLE));
	char* firstName = nick + strlen(nick) + 1;
	char* lastName = firstName + strlen(firstName) + 1;
	char* email = lastName + strlen(lastName) + 1;

	return AddToListByEmail( email, flags );
}

int CMsnProto::AuthRecv( HANDLE hContact, PROTORECVEVENT* pre)
{
	DBEVENTINFO dbei = { 0 };

	dbei.cbSize = sizeof(dbei);
	dbei.szModule = m_szModuleName;
	dbei.timestamp = pre->timestamp;
	dbei.flags = pre->flags & ( PREF_CREATEREAD ? DBEF_READ : 0 );
	dbei.eventType = EVENTTYPE_AUTHREQUEST;

	/* Just copy the Blob from PSR_AUTH event. */
	dbei.cbBlob = pre->lParam;
	dbei.pBlob = (PBYTE)pre->szMessage;
	MSN_CallService( MS_DB_EVENT_ADD, 0,( LPARAM )&dbei );

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AUTHREQUEST

int __cdecl CMsnProto::AuthRequest( HANDLE hContact, const char* szMessage )
{	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// ChangeInfo 

HANDLE __cdecl CMsnProto::ChangeInfo( int iInfoType, void* pInfoData )
{
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnAuthAllow - called after successful authorization

int CMsnProto::Authorize( HANDLE hDbEvent )
{
	if ( !msnLoggedIn )
		return 1;

	DBEVENTINFO dbei = { 0 };
	dbei.cbSize = sizeof( dbei );

	if ((int)( dbei.cbBlob = MSN_CallService( MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hDbEvent, 0 )) == -1 )
		return 1;

	dbei.pBlob = ( PBYTE )alloca( dbei.cbBlob );
	if ( MSN_CallService( MS_DB_EVENT_GET, (WPARAM)hDbEvent, ( LPARAM )&dbei ))
		return 1;

	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST )
		return 1;

	if ( strcmp( dbei.szModule, m_szModuleName ))
		return 1;

	char* nick = ( char* )( dbei.pBlob + sizeof( DWORD )*2 );
	char* firstName = nick + strlen( nick ) + 1;
	char* lastName = firstName + strlen( firstName ) + 1;
	char* email = lastName + strlen( lastName ) + 1;

	HANDLE hContact = MSN_HContactFromEmail(email, nick, true, 0);
	int netId = Lists_GetNetId(email);

	MSN_AddUser( hContact, email, netId, LIST_PL + LIST_REMOVE );
	MSN_AddUser( hContact, email, netId, LIST_BL + LIST_REMOVE );
	MSN_AddUser( hContact, email, netId, LIST_AL );
	MSN_AddUser( hContact, email, netId, LIST_FL );
	MSN_AddUser( hContact, email, netId, LIST_RL );

	MSN_SetContactDb(hContact, email);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnAuthDeny - called after unsuccessful authorization

int CMsnProto::AuthDeny( HANDLE hDbEvent, const char* szReason )
{
	if ( !msnLoggedIn )
		return 1;

	DBEVENTINFO dbei = { 0 };
	dbei.cbSize = sizeof( dbei );

	if ((int)( dbei.cbBlob = MSN_CallService( MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hDbEvent, 0 )) == -1 )
		return 1;

	dbei.pBlob = ( PBYTE )alloca( dbei.cbBlob );
	if ( MSN_CallService( MS_DB_EVENT_GET, (WPARAM)hDbEvent, ( LPARAM )&dbei ))
		return 1;

	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST )
		return 1;

	if ( strcmp( dbei.szModule, m_szModuleName ))
		return 1;

	char* nick = ( char* )( dbei.pBlob + sizeof( DWORD )*2 );
	char* firstName = nick + strlen( nick ) + 1;
	char* lastName = firstName + strlen( firstName ) + 1;
	char* email = lastName + strlen( lastName ) + 1;

	HANDLE hContact = MSN_HContactFromEmail(email, nick, true, false);
	int netId = Lists_GetNetId(email);

	MSN_AddUser( hContact, email, netId, LIST_PL + LIST_REMOVE );
	MSN_AddUser( hContact, email, netId, LIST_BL );
	MSN_AddUser( hContact, email, netId, LIST_RL );

	if (DBGetContactSettingByte( hContact, "CList", "NotOnList", 0 ) == 0)
		MSN_AddUser( hContact, email, netId, LIST_FL );

	MSN_SetContactDb(hContact, email);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnBasicSearch - search contacts by e-mail

void __cdecl CMsnProto::MsnSearchAckThread( void* arg )
{
	const char* email = (char*)arg;

	if (Lists_IsInList(LIST_FL, email) && Lists_GetNetId(email) != NETID_EMAIL)
	{
		TCHAR *title = mir_a2t(email);
		MSN_ShowPopup(title, _T("Contact already in your contact list"), MSN_ALLOW_MSGBOX, NULL);
		SendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, arg, 0 );
		mir_free(title);
		mir_free(arg);
		return;
	}

	unsigned res = MSN_ABContactAdd(email, NULL, 1, true);
	switch(res)
	{
	case 0:
	case 2:
	case 3:
		{
			PROTOSEARCHRESULT isr = {0};
			isr.cbSize = sizeof( isr );
			isr.nick = (char*)arg;
			isr.email = (char*)arg;

			SendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, arg, ( LPARAM )&isr );
			SendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, arg, 0 );
		}
		break;
	
	case 1:
		if (strstr(email, "@yahoo.com") == NULL)
			SendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, arg, 0 );
		else
		{
			msnSearchId = arg;
			MSN_FindYahooUser(email);
		}
		break;
	}
	mir_free(arg);
}


HANDLE __cdecl CMsnProto::SearchBasic( const char* id )
{
	if (!msnLoggedIn) return 0;
	
	char* email = mir_strdup(id); 
	ForkThread(&CMsnProto::MsnSearchAckThread, email);

	return email;
}

HANDLE __cdecl CMsnProto::SearchByEmail( const char* email )
{
	return SearchBasic(email);
}


HANDLE __cdecl CMsnProto::SearchByName( const char* nick, const char* firstName, const char* lastName )
{
	return NULL;
}

HWND __cdecl CMsnProto::SearchAdvanced( HWND hwndDlg )
{
	return NULL;
}

HWND __cdecl CMsnProto::CreateExtendedSearchUI( HWND parent )
{
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnFileAllow - starts the file transfer

void __cdecl CMsnProto::MsnFileAckThread( void* arg )
{
	filetransfer* ft = (filetransfer*)arg;
	
	char filefull[ MAX_PATH ];
	mir_snprintf( filefull, sizeof( filefull ), "%s\\%s", ft->std.workingDir, ft->std.currentFile );
	replaceStr( ft->std.currentFile, filefull );

	if ( SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_FILERESUME, ft, ( LPARAM )&ft->std ))
		return;

	if ( ft->wszFileName != NULL ) 
	{
		mir_free( ft->wszFileName );
		ft->wszFileName = NULL;
	}	

	bool fcrt = ft->create() != -1;

	if ( ft->p2p_appID != 0) {
		p2p_sendStatus( ft, fcrt ? 200 : 603 );
		if ( fcrt )
			p2p_sendFeedStart( ft );
	}
	else
		msnftp_sendAcceptReject (ft, fcrt);

	SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0);
}

int __cdecl CMsnProto::FileAllow( HANDLE hContact, HANDLE hTransfer, const char* szPath )
{
	filetransfer* ft = ( filetransfer* )hTransfer;

	if ( !msnLoggedIn || !p2p_sessionRegistered( ft ) )
		return 0;

	if (( ft->std.workingDir = mir_strdup( szPath )) == NULL ) {
		char szCurrDir[ MAX_PATH ];
		GetCurrentDirectoryA( sizeof( szCurrDir ), szCurrDir );
		ft->std.workingDir = mir_strdup( szCurrDir );
	}
	else {
		int len = strlen( ft->std.workingDir )-1;
		if ( ft->std.workingDir[ len ] == '\\' )
			ft->std.workingDir[ len ] = 0;
	}

	ForkThread( &CMsnProto::MsnFileAckThread, ft );

	return int( ft );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnFileCancel - cancels the active file transfer

int __cdecl CMsnProto::FileCancel( HANDLE hContact, HANDLE hTransfer )
{
	filetransfer* ft = ( filetransfer* )hTransfer;

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

int __cdecl CMsnProto::FileDeny( HANDLE hContact, HANDLE hTransfer, const char* /*szReason*/ )
{
	filetransfer* ft = ( filetransfer* )hTransfer;

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

int __cdecl CMsnProto::FileResume( HANDLE hTransfer, int* action, const char** szFilename )
{
	filetransfer* ft = ( filetransfer* )hTransfer;

	if ( !msnLoggedIn || !p2p_sessionRegistered( ft ))
		return 1;

	switch (*action) {
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
		replaceStr( ft->std.currentFile, *szFilename );

	default:
		bool fcrt = ft->create() != -1;
		if ( ft->p2p_appID != 0 ) {
			p2p_sendStatus( ft, fcrt ? 200 : 603 );
			if ( fcrt )
				p2p_sendFeedStart( ft );
		}
		else
			msnftp_sendAcceptReject (ft, fcrt);

		SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0);
		break;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetAwayMsg - reads the current status message for a user

typedef struct AwayMsgInfo_tag
{
	int id;
	HANDLE hContact;
} AwayMsgInfo;

void __cdecl CMsnProto::MsnGetAwayMsgThread( void* arg )
{
	Sleep( 150 );

	AwayMsgInfo *inf = ( AwayMsgInfo* )arg;
	DBVARIANT dbv;
	if ( !DBGetContactSettingString( inf->hContact, "CList", "StatusMsg", &dbv )) {
		SendBroadcast( inf->hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, ( HANDLE )inf->id, ( LPARAM )dbv.pszVal );
		MSN_FreeVariant( &dbv );
	}
	else SendBroadcast( inf->hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, ( HANDLE )inf->id, ( LPARAM )0 );

	mir_free( inf );
}

int __cdecl CMsnProto::GetAwayMsg( HANDLE hContact )
{
	AwayMsgInfo* inf = (AwayMsgInfo*)mir_alloc( sizeof( AwayMsgInfo ));
	inf->hContact = hContact;
	inf->id = MSN_GenRandom();

	ForkThread( &CMsnProto::MsnGetAwayMsgThread, inf );
	return inf->id;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetCaps - obtain the protocol capabilities

DWORD __cdecl CMsnProto::GetCaps( int type, HANDLE hContact )
{
	switch( type ) {
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
		return PF4_SUPPORTTYPING | PF4_AVATARS | PF4_SUPPORTIDLE | PF4_IMSENDUTF;

	case PFLAG_UNIQUEIDTEXT:
		return ( int )MSN_Translate( "Live ID" );

	case PFLAG_UNIQUEIDSETTING:
		return ( int )"e-mail";

	case PFLAG_MAXLENOFMESSAGE:
		return 1202;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnGetInfo - nothing to do, cause we cannot obtain information from the server

int __cdecl CMsnProto::GetInfo( HANDLE hContact, int infoType )
{
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnLoadIcon - obtain the protocol icon

HICON __cdecl CMsnProto::GetIcon( int iconIndex )
{
	if ( LOWORD( iconIndex ) == PLI_PROTOCOL )
		return CopyIcon( LoadIconEx( "main" ));

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvContacts

int __cdecl CMsnProto::RecvContacts( HANDLE hContact, PROTORECVEVENT* )
{
	return 1;
}


/////////////////////////////////////////////////////////////////////////////////////////
// MsnRecvFile - creates a database event from the file request been received

int __cdecl CMsnProto::RecvFile( HANDLE hContact, PROTORECVFILE* evt )
{
	CCSDATA ccs = { hContact, PSR_FILE, 0, ( LPARAM )evt };
	return MSN_CallService( MS_PROTO_RECVFILE, 0, ( LPARAM )&ccs );
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnRecvMessage - creates a database event from the message been received

int __cdecl CMsnProto::RecvMsg( HANDLE hContact, PROTORECVEVENT* pre )
{
	char tEmail[ MSN_MAX_EMAIL_LEN ];
	getStaticString( hContact, "e-mail", tEmail, sizeof( tEmail ));

	if (Lists_IsInList( LIST_FL, tEmail ))
		DBDeleteContactSetting( hContact, "CList", "Hidden" );

	CCSDATA ccs = { hContact, PSR_MESSAGE, 0, ( LPARAM )pre };
	return MSN_CallService( MS_PROTO_RECVMSG, 0, ( LPARAM )&ccs );
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvUrl

int __cdecl CMsnProto::RecvUrl( HANDLE hContact, PROTORECVEVENT* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendContacts

int __cdecl CMsnProto::SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList )
{
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnSendFile - initiates a file transfer

int __cdecl CMsnProto::SendFile( HANDLE hContact, const char* szDescription, char** ppszFiles )
{
	if ( !msnLoggedIn )
		return 0;

	if ( getWord( hContact, "Status", ID_STATUS_OFFLINE ) == ID_STATUS_OFFLINE )
		return 0;

	char tEmail[ MSN_MAX_EMAIL_LEN ];
	if ( MSN_IsMeByContact( hContact, tEmail )) return 0;

	DWORD dwFlags = getDword( hContact, "FlagBits", 0 );

	int netId = Lists_GetNetId( tEmail );
	if ((dwFlags & 0xf0000000) == 0 && netId != NETID_MSN) return 0;

	filetransfer* sft = new filetransfer(this);
	sft->std.files = ppszFiles;
	sft->std.hContact = hContact;
	sft->std.sending = true;

	while ( ppszFiles[sft->std.totalFiles] != NULL ) {
		struct _stat statbuf;
		if ( _stat( ppszFiles[sft->std.totalFiles], &statbuf ) == 0 )
			sft->std.totalBytes += statbuf.st_size;

		++sft->std.totalFiles;
	}

	if ( sft->openNext() == -1 ) {
		delete sft;
		return 0;
	}

	if ( dwFlags & 0xf0000000 )
		p2p_invite( hContact, MSN_APPID_FILE, sft );
	else
		msnftp_invite( sft );

	SendBroadcast( hContact, ACKTYPE_FILE, ACKRESULT_SENTREQUEST, sft, 0 );
	return (int)(HANDLE)sft;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnSendMessage - sends the message to a server

struct TFakeAckParams
{
	inline TFakeAckParams( HANDLE p2, long p3, const char* p4, CMsnProto *p5 ) :
		hContact( p2 ),
		id( p3 ),
		msg( p4 ),
		proto(p5)
		{}

	HANDLE	hContact;
	long	id;
	const char*	msg;
	CMsnProto *proto;
};

void CMsnProto::MsnFakeAck( void* arg )
{
	TFakeAckParams* tParam = ( TFakeAckParams* )arg;

	Sleep( 150 );
	tParam->proto->SendBroadcast( tParam->hContact, ACKTYPE_MESSAGE, 
		tParam->msg ? ACKRESULT_FAILED : ACKRESULT_SUCCESS,
		( HANDLE )tParam->id, LPARAM( tParam->msg ));

	delete tParam;
}

void CMsnProto::MsnSendOim( void* arg )
{
	TFakeAckParams* tParam = ( TFakeAckParams* )arg;

	char tEmail[ MSN_MAX_EMAIL_LEN ];
	tParam->proto->getStaticString( tParam->hContact, "e-mail", tEmail, sizeof( tEmail ));

	int seq = tParam->proto->MSN_SendOIM(tEmail, tParam->msg);

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
	tParam->proto->SendBroadcast( tParam->hContact, ACKTYPE_MESSAGE, errMsg ? ACKRESULT_FAILED : ACKRESULT_SUCCESS,
		( HANDLE )tParam->id, ( LPARAM )errMsg );

	mir_free(( void* )tParam->msg );
	delete tParam;
}

int __cdecl CMsnProto::SendMsg( HANDLE hContact, int flags, const char* pszSrc )
{
	if ( !msnLoggedIn ) return 0;

	char *errMsg = NULL;

	char tEmail[ MSN_MAX_EMAIL_LEN ];
	if ( MSN_IsMeByContact( hContact, tEmail )) 
	{
		errMsg = MSN_Translate( "You cannot send message to yourself" );
		ForkThread( &CMsnProto::MsnFakeAck, new TFakeAckParams( hContact, 999999, errMsg, this ));
		return 999999;
	}

	char *msg = ( char* )pszSrc;
	if ( msg == NULL ) return 0;

	if ( flags & PREF_UNICODE ) {
		char* p = strchr(msg, '\0');
		if (p != msg) {
            while ( *(++p) == '\0' ) {}
			msg = mir_utf8encodeW(( wchar_t* )p );
		}
		else
			msg = mir_strdup( msg );
	}
	else
		msg = (flags & PREF_UTF) ? mir_strdup(msg) : mir_utf8encode(msg);

	int rtlFlag = (flags & PREF_RTL ) ? MSG_RTL : 0;

	int seq = 0;
	int netId  = Lists_GetNetId(tEmail);

	switch (netId)
	{
	case NETID_EMAIL:
		seq = 999994;
		errMsg = MSN_Translate( "Cannot send messages to E-mail only contacts" );
		ForkThread( &CMsnProto::MsnFakeAck, new TFakeAckParams( hContact, seq, errMsg, this ));
		break;

	case NETID_MOB:
		if ( strlen( msg ) > 133 ) {
			errMsg = MSN_Translate( "Message is too long: SMS page limited to 133 UTF8 chars" );
			seq = 999997;
		}
		else
		{
			errMsg = NULL;
			seq = MSN_SendSMS(tEmail, msg);
		}
		ForkThread( &CMsnProto::MsnFakeAck, new TFakeAckParams( hContact, seq, errMsg, this ));
		break;

	case NETID_MSN:
	case NETID_LCS:
		if ( strlen( msg ) > 1202 ) 
		{
			seq = 999996;
			errMsg = MSN_Translate( "Message is too long: MSN messages are limited by 1202 UTF8 chars" );
			ForkThread( &CMsnProto::MsnFakeAck, new TFakeAckParams( hContact, seq, errMsg, this ));
		}
		else
		{
			const char msgType = MyOptions.SlowSend ? 'A' : 'N';
			bool isOffline;
			ThreadData* thread = MSN_StartSB(hContact, isOffline);
			if ( thread == NULL )
			{
				if ( isOffline ) 
				{
					if (netId == NETID_MSN)
					{
						seq = MSN_GenRandom();
						ForkThread( &CMsnProto::MsnSendOim, new TFakeAckParams( hContact, seq, mir_strdup( msg ), this));
					}
					else
					{
						seq = 999993;
						errMsg = MSN_Translate( "Offline messaging is not allowed for LCS contacts" );
						ForkThread( &CMsnProto::MsnFakeAck, new TFakeAckParams( hContact, seq, errMsg, this ));
					}
				}
				else
					seq = MsgQueue_Add( hContact, msgType, msg, 0, 0, rtlFlag );
			}
			else
			{
				seq = thread->sendMessage( msgType, tEmail, netId, msg, rtlFlag );
				if ( !MyOptions.SlowSend )
					ForkThread( &CMsnProto::MsnFakeAck, new TFakeAckParams( hContact, seq, NULL, this ));
			}
		}
		break;
		
	case NETID_YAHOO:
		if ( strlen( msg ) > 1202 ) 
		{
			seq = 999996;
			errMsg = MSN_Translate( "Message is too long: MSN messages are limited by 1202 UTF8 chars" );
			ForkThread( &CMsnProto::MsnFakeAck, new TFakeAckParams( hContact, seq, errMsg, this ));
		}
		else
		{
			seq = msnNsThread->sendMessage( '1', tEmail, netId, msg, rtlFlag );
			ForkThread( &CMsnProto::MsnFakeAck, new TFakeAckParams( hContact, seq, NULL, this ));
		}
		break;

	default:
		break;
	}

	mir_free( msg );
	return seq;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnSetAwayMsg - sets the current status message for a user

int __cdecl CMsnProto::SetAwayMsg( int status, const char* msg )
{
	char** msgptr = GetStatusMsgLoc(status);

	if ( msgptr == NULL )
		return 1;

	replaceStr( *msgptr, msg );

	if ( status == m_iDesiredStatus )
		MSN_SendStatusMessage( msg );

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSR_AWAYMSG

int __cdecl CMsnProto::RecvAwayMsg( HANDLE hContact, int statusMode, PROTORECVEVENT* evt )
{	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AWAYMSG

int __cdecl CMsnProto::SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg )
{	
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnSetStatus - set the plugin's connection status

int __cdecl CMsnProto::SetStatus( int iNewStatus )
{
	if (m_iDesiredStatus == iNewStatus) return 0;

	m_iDesiredStatus = iNewStatus;
	MSN_DebugLog( "PS_SETSTATUS(%d,0)", iNewStatus );

	if ( m_iDesiredStatus == ID_STATUS_OFFLINE )
	{
		MSN_GoOffline();
	}
	else if (!msnLoggedIn && !(m_iStatus>=ID_STATUS_CONNECTING && m_iStatus<ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES))
	{
		char szPassword[ 100 ];
		int ps = getStaticString( NULL, "Password", szPassword, sizeof( szPassword ));
		if (ps != 0  || *szPassword == 0) {
			SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD );
			m_iDesiredStatus = m_iStatus = ID_STATUS_OFFLINE;
			return 0;
		}	
		 
		if (*MyOptions.szEmail == 0) {
			SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_BADUSERID );
			m_iDesiredStatus = m_iStatus = ID_STATUS_OFFLINE;
			return 0;
		}	

		MyOptions.UseProxy = getByte( "NLUseProxy", FALSE ) != 0;

		ThreadData* newThread = new ThreadData;

		newThread->mType = SERVER_DISPATCH;
		newThread->mIsMainThread = true;
		{	int oldMode = m_iStatus;
			m_iStatus = ID_STATUS_CONNECTING;
			SendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE )oldMode, m_iStatus );
		}
		newThread->startThread( &CMsnProto::MSNServerThread, this );
	}
	else MSN_SetServerStatus( m_iDesiredStatus );

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MsnUserIsTyping - notify another contact that we're typing a message

int __cdecl CMsnProto::UserIsTyping( HANDLE hContact, int type )
{
	if ( !msnLoggedIn ) return 0;

	char tEmail[ MSN_MAX_EMAIL_LEN ];
	if ( MSN_IsMeByContact( hContact, tEmail )) return 0;

	bool typing = type == PROTOTYPE_SELFTYPING_ON;

	int netId = Lists_GetNetId(tEmail);
	switch (netId)
	{
	case NETID_UNKNOWN:
	case NETID_MSN:
	case NETID_LCS:
		{
			bool isOffline;
			ThreadData* thread = MSN_StartSB(hContact, isOffline);

			if ( thread == NULL ) 
			{
				if (isOffline) return 0;
				MsgQueue_Add( hContact, 2571, NULL, 0, NULL, typing );
			}
			else
				MSN_StartStopTyping( thread, typing );
		}
		break;

	case NETID_YAHOO:
		if (typing) MSN_SendTyping(msnNsThread, tEmail, netId);
		break;

	default:
		break;
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendUrl

int __cdecl CMsnProto::SendUrl( HANDLE hContact, int flags, const char* url )
{
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	MsnSetApparentMode - controls contact visibility

int __cdecl CMsnProto::SetApparentMode( HANDLE hContact, int mode )
{
	if ( mode && mode != ID_STATUS_OFFLINE )
	  return 1;

	WORD oldMode = getWord( hContact, "ApparentMode", 0 );
	if ( mode != oldMode )
		setWord( hContact, "ApparentMode", ( WORD )mode );

	return 1;
}

int __cdecl CMsnProto::OnEvent( PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam )
{
	switch( eventType ) {
		case EV_PROTO_ONLOAD:    return OnModulesLoaded( 0, 0 );
		case EV_PROTO_ONEXIT:    return OnPreShutdown( 0, 0 );
		case EV_PROTO_ONOPTIONS: return OnOptionsInit( wParam, lParam );
		case EV_PROTO_ONRENAME:
		{	
			CLISTMENUITEM clmi = { 0 };
			clmi.cbSize = sizeof( CLISTMENUITEM );
			clmi.flags = CMIM_NAME | CMIF_TCHAR;
			clmi.ptszName = m_tszUserName;
			MSN_CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )mainMenuRoot, ( LPARAM )&clmi );
            break;
		}
        default: break;              
	}
	return 1;
}
