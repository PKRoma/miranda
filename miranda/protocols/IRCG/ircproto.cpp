/*
IRC plugin for Miranda IM

Copyright (C) 2003-05 Jurgen Persson
Copyright (C) 2007-09 George Hazan

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

#include "irc.h"
#include "version.h"

#include <m_genmenu.h>

static int CompareSessions( const CDccSession* p1, const CDccSession* p2 )
{
	return int( p1->di->hContact ) - int( p2->di->hContact );
}

CIrcProto::CIrcProto( const char* szModuleName, const TCHAR* tszUserName ) :
	m_dcc_chats( 10, CompareSessions ),
	m_dcc_xfers( 10, CompareSessions ),
	m_ignoreItems( 10, NULL ),
	vUserhostReasons( 10, NULL ),
	vWhoInProgress( 10, NULL )
{
	m_tszUserName = mir_tstrdup( tszUserName );
	m_szModuleName = mir_strdup( szModuleName );

	InitializeCriticalSection(&cs);
	InitializeCriticalSection(&m_gchook);
	m_evWndCreate = ::CreateEvent( NULL, FALSE, FALSE, NULL );

	IrcHookEvent( ME_DB_CONTACT_DELETED,   &CIrcProto::OnDeletedContact );

	CreateProtoService( PS_GETSTATUS,      &CIrcProto::GetStatus );
	CreateProtoService( PS_CREATEACCMGRUI, &CIrcProto::SvcCreateAccMgrUI );
	CreateProtoService( PS_JOINCHAT,       &CIrcProto::OnJoinChat );
	CreateProtoService( PS_LEAVECHAT,      &CIrcProto::OnLeaveChat );

	CreateProtoService( IRC_JOINCHANNEL,   &CIrcProto::OnJoinMenuCommand );
	CreateProtoService( IRC_QUICKCONNECT,  &CIrcProto::OnQuickConnectMenuCommand);
	CreateProtoService( IRC_CHANGENICK,    &CIrcProto::OnChangeNickMenuCommand );
	CreateProtoService( IRC_SHOWLIST,      &CIrcProto::OnShowListMenuCommand );
	CreateProtoService( IRC_SHOWSERVER,    &CIrcProto::OnShowServerMenuCommand );
	CreateProtoService( IRC_UM_CHANSETTINGS, &CIrcProto::OnMenuChanSettings );
	CreateProtoService( IRC_UM_WHOIS,      &CIrcProto::OnMenuWhois );
	CreateProtoService( IRC_UM_DISCONNECT, &CIrcProto::OnMenuDisconnect );
	CreateProtoService( IRC_UM_IGNORE,     &CIrcProto::OnMenuIgnore );

	CreateProtoService( "/DblClickEvent",  &CIrcProto::OnDoubleclicked );
	CreateProtoService( "/InsertRawIn",    &CIrcProto::Scripting_InsertRawIn );
	CreateProtoService( "/InsertRawOut",   &CIrcProto::Scripting_InsertRawOut );
	CreateProtoService( "/InsertGuiIn",    &CIrcProto::Scripting_InsertGuiIn );
	CreateProtoService( "/InsertGuiOut",   &CIrcProto::Scripting_InsertGuiOut);
	CreateProtoService( "/GetIrcData",     &CIrcProto::Scripting_GetIrcData);

	codepage = CP_ACP;
	InitializeCriticalSection(&m_resolve);
	InitializeCriticalSection(&m_dcc);

	InitPrefs();

	char text[ MAX_PATH ];
	mir_snprintf( text, sizeof( text ), "%s/Status", m_szProtoName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )text );

	CList_SetAllOffline(true);

	IRC_MAP_ENTRY("PING", PING)
	IRC_MAP_ENTRY("JOIN", JOIN)
	IRC_MAP_ENTRY("QUIT", QUIT)
	IRC_MAP_ENTRY("KICK", KICK)
	IRC_MAP_ENTRY("MODE", MODE)
	IRC_MAP_ENTRY("NICK", NICK)
	IRC_MAP_ENTRY("PART", PART)
	IRC_MAP_ENTRY("PRIVMSG", PRIVMSG)
	IRC_MAP_ENTRY("TOPIC", TOPIC)
	IRC_MAP_ENTRY("NOTICE", NOTICE)
	IRC_MAP_ENTRY("PING", PINGPONG)
	IRC_MAP_ENTRY("PONG", PINGPONG)
	IRC_MAP_ENTRY("INVITE", INVITE)
	IRC_MAP_ENTRY("ERROR", ERROR)
	IRC_MAP_ENTRY("001", WELCOME)
	IRC_MAP_ENTRY("002", YOURHOST)
	IRC_MAP_ENTRY("005", SUPPORT)
	IRC_MAP_ENTRY("223", WHOIS_OTHER)			//CodePage info
	IRC_MAP_ENTRY("254", NOOFCHANNELS)
	IRC_MAP_ENTRY("263", TRYAGAIN)
	IRC_MAP_ENTRY("264", WHOIS_OTHER)			//Encryption info (SSL connect)
	IRC_MAP_ENTRY("301", WHOIS_AWAY)
	IRC_MAP_ENTRY("302", USERHOST_REPLY)
	IRC_MAP_ENTRY("305", BACKFROMAWAY)
	IRC_MAP_ENTRY("306", SETAWAY)
	IRC_MAP_ENTRY("307", WHOIS_AUTH)
	IRC_MAP_ENTRY("310", WHOIS_OTHER)
	IRC_MAP_ENTRY("311", WHOIS_NAME)
	IRC_MAP_ENTRY("312", WHOIS_SERVER)
	IRC_MAP_ENTRY("313", WHOIS_OTHER)
	IRC_MAP_ENTRY("315", WHO_END)
	IRC_MAP_ENTRY("317", WHOIS_IDLE)
	IRC_MAP_ENTRY("318", WHOIS_END)
	IRC_MAP_ENTRY("319", WHOIS_CHANNELS)
	IRC_MAP_ENTRY("320", WHOIS_AUTH)
	IRC_MAP_ENTRY("321", LISTSTART)
	IRC_MAP_ENTRY("322", LIST)
	IRC_MAP_ENTRY("323", LISTEND)
	IRC_MAP_ENTRY("324", MODEQUERY)
	IRC_MAP_ENTRY("330", WHOIS_AUTH)
	IRC_MAP_ENTRY("332", INITIALTOPIC)
	IRC_MAP_ENTRY("333", INITIALTOPICNAME)
	IRC_MAP_ENTRY("352", WHO_REPLY)
	IRC_MAP_ENTRY("353", NAMES)
	IRC_MAP_ENTRY("366", ENDNAMES)
	IRC_MAP_ENTRY("367", BANLIST)
	IRC_MAP_ENTRY("368", BANLISTEND)
	IRC_MAP_ENTRY("346", BANLIST)
	IRC_MAP_ENTRY("347", BANLISTEND)
	IRC_MAP_ENTRY("348", BANLIST)
	IRC_MAP_ENTRY("349", BANLISTEND)
	IRC_MAP_ENTRY("371", WHOIS_OTHER)
	IRC_MAP_ENTRY("376", ENDMOTD)
	IRC_MAP_ENTRY("401", WHOIS_NO_USER)
	IRC_MAP_ENTRY("403", JOINERROR)
	IRC_MAP_ENTRY("416", WHOTOOLONG)
	IRC_MAP_ENTRY("421", UNKNOWN)
	IRC_MAP_ENTRY("422", ENDMOTD)
	IRC_MAP_ENTRY("433", NICK_ERR)
	IRC_MAP_ENTRY("471", JOINERROR)
	IRC_MAP_ENTRY("473", JOINERROR)
	IRC_MAP_ENTRY("474", JOINERROR)
	IRC_MAP_ENTRY("475", JOINERROR)
	IRC_MAP_ENTRY("671", WHOIS_OTHER)			//Encryption info (SSL connect)
}

CIrcProto::~CIrcProto()
{
	if ( con ) {
		Netlib_CloseHandle( con );
		con = NULL;
	}

	Netlib_CloseHandle(hNetlib); hNetlib = NULL;
	Netlib_CloseHandle(hNetlibDCC); hNetlibDCC = NULL;

	m_dcc_chats.destroy();
	m_dcc_xfers.destroy();

	DeleteCriticalSection( &cs );
	DeleteCriticalSection( &m_gchook );

	CallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hUMenuShowChannel, 0 );
	CallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hUMenuJoinLeave, 0 );
	CallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hUMenuChanSettings, 0 );
	CallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hUMenuWhois, 0 );
	CallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hUMenuDisconnect, 0 );
	CallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hUMenuIgnore, 0 );

	CallService( MS_CLIST_REMOVEMAINMENUITEM, ( WPARAM )hMenuRoot, 0 );

	mir_free( m_alias );
	mir_free( m_szModuleName );
	mir_free( m_tszUserName );

	CloseHandle( m_evWndCreate );
	DeleteCriticalSection(&m_resolve);
	DeleteCriticalSection(&m_dcc);
	KillChatTimer(OnlineNotifTimer);
	KillChatTimer(OnlineNotifTimer3);

	delete[] hIconLibItems;
}

////////////////////////////////////////////////////////////////////////////////////////
// OnModulesLoaded - performs hook registration

static COLORREF crCols[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

static int sttCheckPerform( const char *szSetting, LPARAM lParam )
{
	if ( !_strnicmp( szSetting, "PERFORM:", 8 )) {
		String s = szSetting;
		s.MakeUpper();
		if ( s != szSetting ) {
			OBJLIST<String>* p = ( OBJLIST<String>* )lParam;
			p->insert( new String( szSetting ));
	}	}
	return 0;
}

int CIrcProto::OnModulesLoaded( WPARAM, LPARAM )
{
	char szTemp[MAX_PATH];
	char szTemp3[256];
	NETLIBUSER nlu = {0};
	TCHAR name[128];

	DBDeleteContactSetting( NULL, m_szModuleName, "JTemp" );

	AddIcons();

	nlu.cbSize = sizeof(nlu);
	nlu.flags = NUF_OUTGOING|NUF_INCOMING|NUF_HTTPCONNS|NUF_TCHAR;
	nlu.szSettingsModule = m_szModuleName;
	mir_sntprintf( name, SIZEOF(name), TranslateT("%s server connection"), m_tszUserName);
	nlu.ptszDescriptiveName = name;
	hNetlib=(HANDLE)CallService( MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

	nlu.flags = NUF_OUTGOING|NUF_INCOMING|NUF_HTTPCONNS|NUF_TCHAR;
	char szTemp2[256];
	mir_snprintf(szTemp2, sizeof(szTemp2), "%s DCC", m_szModuleName);
	nlu.szSettingsModule = szTemp2;
	mir_sntprintf( name, SIZEOF(name), TranslateT("%s client-to-client connections"), m_tszUserName);
	nlu.ptszDescriptiveName = name;
	hNetlibDCC=(HANDLE)CallService( MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

	//add as a known module in DB Editor ++
	CallService( "DBEditorpp/RegisterSingleModule",(WPARAM)m_szModuleName,0);
	mir_snprintf(szTemp3, sizeof(szTemp3), "%s DCC", m_szModuleName);
	CallService( "DBEditorpp/RegisterSingleModule",(WPARAM)szTemp3,0);

	if ( ServiceExists("MBot/GetFcnTable")) {
		CallService( MS_MBOT_REGISTERIRC, 0, (LPARAM)m_szModuleName);
		m_bMbotInstalled = TRUE;
	}

	if ( ServiceExists( MS_GC_REGISTER )) {
		GCREGISTER gcr = {0};
		gcr.cbSize = sizeof(GCREGISTER);
		gcr.dwFlags = GC_CHANMGR|GC_BOLD|GC_ITALICS|GC_UNDERLINE|GC_COLOR|GC_BKGCOLOR;
		gcr.iMaxText = 0;
		gcr.nColors = 16;
		gcr.pColors = colors;
		gcr.pszModuleDispName = m_szModuleName;
		gcr.pszModule = m_szModuleName;
		CallServiceSync( MS_GC_REGISTER, NULL, (LPARAM)&gcr );
		IrcHookEvent( ME_GC_EVENT, &CIrcProto::GCEventHook );
		IrcHookEvent( ME_GC_BUILDMENU, &CIrcProto::GCMenuHook );

		GCSESSION gcw = {0};
		GCDEST gcd = {0};
		GCEVENT gce = {0};

		gcw.cbSize = sizeof(GCSESSION);
		gcw.dwFlags = GC_TCHAR;
		gcw.iType = GCW_SERVER;
		gcw.ptszID = SERVERWINDOW;
		gcw.pszModule = m_szModuleName;
		gcw.ptszName = NEWTSTR_ALLOCA(( TCHAR* )_A2T( m_network ));
		CallServiceSync( MS_GC_NEWSESSION, 0, (LPARAM)&gcw );

		gce.cbSize = sizeof(GCEVENT);
		gce.dwFlags = GC_TCHAR;
		gce.pDest = &gcd;
		gcd.ptszID = SERVERWINDOW;
		gcd.pszModule = m_szModuleName;
		gcd.iType = GC_EVENT_CONTROL;

		gce.pDest = &gcd;
		if ( m_useServer && !m_hideServerWindow )
			CallChatEvent( WINDOW_VISIBLE, (LPARAM)&gce);
		else
			CallChatEvent( WINDOW_HIDDEN, (LPARAM)&gce);
		bChatInstalled = TRUE;
	}
	else {
		if ( IDYES == MessageBox(0,TranslateT("The IRC protocol depends on another plugin called \'Chat\'\n\nDo you want to download it from the Miranda IM web site now?"),TranslateT("Information"),MB_YESNO|MB_ICONINFORMATION ))
			CallService( MS_UTILS_OPENURL, 1, (LPARAM) "http://www.miranda-im.org/download/");
	}
    
	mir_snprintf(szTemp, sizeof(szTemp), "%s\\%s_perform.ini", mirandapath, m_szModuleName);
	char* pszPerformData = IrcLoadFile( szTemp );
	if ( pszPerformData != NULL ) {
		char *p1 = pszPerformData, *p2 = pszPerformData;
		while (( p1 = strstr( p2, "NETWORK: " )) != NULL ) {
			p1 += 9;
			p2 = strchr(p1, '\n');
			String sNetwork( p1, int( p2-p1-1 ));
			sNetwork.MakeUpper();
			p1 = p2;
			p2 = strstr( ++p1, "\nNETWORK: " );
			if ( !p2 )
				p2 = p1 + lstrlenA( p1 )-1;
			if ( p1 == p2 )
				break;

			*p2++ = 0;
			setString(("PERFORM:" + sNetwork).c_str(), rtrim( p1 ));
		}
		delete[] pszPerformData;
		::remove( szTemp );
	}

	if ( !getByte( "PerformConversionDone", 0 )) {
		OBJLIST<String> performToConvert( 10, NULL );
		DBCONTACTENUMSETTINGS dbces;
		dbces.pfnEnumProc = sttCheckPerform;
		dbces.lParam = ( LPARAM )&performToConvert;
		dbces.szModule = m_szModuleName;
		CallService( MS_DB_CONTACT_ENUMSETTINGS, NULL, (LPARAM)&dbces );

		for ( int i = 0; i < performToConvert.getCount(); i++ ) {
			String s = performToConvert[i];
			DBVARIANT dbv;
			if ( !getTString( s, &dbv )) {
				DBDeleteContactSetting( NULL, m_szModuleName, s );
				s.MakeUpper();
				setTString( s, dbv.ptszVal );
				DBFreeVariant( &dbv );
		}	}

		setByte( "PerformConversionDone", 1 );
	}

	InitIgnore();
	InitMenus();

	IrcHookEvent( ME_USERINFO_INITIALISE, &CIrcProto::OnInitUserInfo );
	IrcHookEvent( ME_CLIST_PREBUILDCONTACTMENU, &CIrcProto::OnMenuPreBuild );
	IrcHookEvent( ME_OPT_INITIALISE, &CIrcProto::OnInitOptionsPages );

	if ( m_nick[0] ) {
		TCHAR szBuf[ 40 ];
		if ( lstrlen( m_alternativeNick ) == 0 ) {
			mir_sntprintf( szBuf, SIZEOF(szBuf), _T("%s%u"), m_nick, rand()%9999);
			setTString("AlernativeNick", szBuf);
			lstrcpyn(m_alternativeNick, szBuf, 30);
		}

		if ( lstrlen( m_name ) == 0 ) {
			mir_sntprintf( szBuf, SIZEOF(szBuf), _T("Miranda%u"), rand()%9999);
			setTString("Name", szBuf);
			lstrcpyn( m_name, szBuf, 200 );
	}	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// AddToList - adds a contact to the contact list

HANDLE __cdecl CIrcProto::AddToList( int, PROTOSEARCHRESULT* psr )
{
	if ( m_iDesiredStatus == ID_STATUS_OFFLINE || m_iDesiredStatus == ID_STATUS_CONNECTING )
		return 0;

	CONTACT user = { mir_a2t( psr->nick ), NULL, NULL, true, false, false };
	HANDLE hContact = CList_AddContact( &user, true, false );

	if ( hContact ) {
		DBVARIANT dbv1;
		CMString S = _T("S");

		if ( getByte( hContact, "AdvancedMode", 0 ) == 0 ) {
			S += user.name;
			DoUserhostWithReason( 1, S, true, user.name );
		}
		else {
			if ( !getTString(hContact, "UWildcard", &dbv1 )) {
				S += dbv1.ptszVal;
				DoUserhostWithReason(2, S, true, dbv1.ptszVal);
				DBFreeVariant( &dbv1 );
			}
			else {
				S += user.name;
				DoUserhostWithReason( 2, S, true, user.name );
			}
		}
		if (getByte( "MirVerAutoRequest", 1))
			PostIrcMessage( _T("/PRIVMSG %s \001VERSION\001"), user.name);
	}

	mir_free( user.name );
	return hContact;
}

////////////////////////////////////////////////////////////////////////////////////////
// AddToList - adds a contact to the contact list

HANDLE __cdecl CIrcProto::AddToListByEvent( int, int, HANDLE )
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// AuthAllow - processes the successful authorization

int __cdecl CIrcProto::Authorize( HANDLE )
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// AuthDeny - handles the unsuccessful authorization

int __cdecl CIrcProto::AuthDeny( HANDLE, const char* )
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSR_AUTH

int __cdecl CIrcProto::AuthRecv( HANDLE, PROTORECVEVENT* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AUTHREQUEST

int __cdecl CIrcProto::AuthRequest( HANDLE, const char* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// ChangeInfo

HANDLE __cdecl CIrcProto::ChangeInfo( int, void* )
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// FileAllow - starts a file transfer

HANDLE __cdecl CIrcProto::FileAllow( HANDLE, HANDLE hTransfer, const char* szPath )
{
	DCCINFO* di = ( DCCINFO* )hTransfer;

	if ( !IsConnected() ) {
		delete di;
		return (HANDLE)szPath;
	}

	TCHAR* ptszFileName = mir_a2t_cp( szPath, getCodepage());
	di->sPath = ptszFileName;
	di->sFileAndPath = di->sPath + di->sFile;
	mir_free( ptszFileName );

	CDccSession* dcc = new CDccSession( this, di );
	AddDCCSession( di, dcc );
	dcc->Connect();
	return di;
}

////////////////////////////////////////////////////////////////////////////////////////
// FileCancel - cancels a file transfer

int __cdecl CIrcProto::FileCancel( HANDLE, HANDLE hTransfer )
{
	DCCINFO* di = ( DCCINFO* )hTransfer;

	CDccSession* dcc = FindDCCSession(di);
	if (dcc) {
		InterlockedExchange(&dcc->dwWhatNeedsDoing, (long)FILERESUME_CANCEL);
		SetEvent(dcc->hEvent);
		dcc->Disconnect();
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// FileDeny - denies a file transfer

int __cdecl CIrcProto::FileDeny( HANDLE, HANDLE hTransfer, const char* )
{
	DCCINFO* di = ( DCCINFO* )hTransfer;
	delete di;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// FileResume - processes file renaming etc

int __cdecl CIrcProto::FileResume( HANDLE hTransfer, int* action, const char** szFilename )
{
	DCCINFO* di = ( DCCINFO* )hTransfer;

	long i = (long)*action;

	CDccSession* dcc = FindDCCSession(di);
	if (dcc) {
		InterlockedExchange(&dcc->dwWhatNeedsDoing, i);
		if (*action == FILERESUME_RENAME) {
			char * szTemp = _strdup(*szFilename);
			InterlockedExchange((long*)&dcc->NewFileName, (long)szTemp);
		}

		if (*action == FILERESUME_RESUME) {
			DWORD dwPos = 0;
			String sFile;
			FILE * hFile = NULL;

			hFile = _tfopen(di->sFileAndPath.c_str(), _T("rb"));
			if (hFile) {
				fseek(hFile,0,SEEK_END);
				dwPos = ftell(hFile);
				rewind (hFile);
				fclose(hFile); hFile = NULL;
			}

			CMString sFileWithQuotes = di->sFile;

			// if spaces in the filename surround witrh quotes
			if (sFileWithQuotes.Find( ' ', 0 ) != -1 ) {
				sFileWithQuotes.Insert( 0, _T("\""));
				sFileWithQuotes.Insert( sFileWithQuotes.GetLength(), _T("\""));
			}

			if (di->bReverse)
				PostIrcMessage( _T("/PRIVMSG %s \001DCC RESUME %s 0 %u %s\001"), di->sContactName.c_str(), sFileWithQuotes.c_str(), dwPos, dcc->di->sToken.c_str());
			else
				PostIrcMessage( _T("/PRIVMSG %s \001DCC RESUME %s %u %u\001"), di->sContactName.c_str(), sFileWithQuotes.c_str(), di->iPort, dwPos);

			return 0;
		}

		SetEvent(dcc->hEvent);
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetCaps - return protocol capabilities bits

DWORD_PTR __cdecl CIrcProto::GetCaps( int type, HANDLE )
{
	switch( type ) {
	case PFLAGNUM_1:
		return PF1_BASICSEARCH | PF1_MODEMSG | PF1_FILE | PF1_CHAT | PF1_CANRENAMEFILE | PF1_PEER2PEER | PF1_IM;

	case PFLAGNUM_2:
		return PF2_ONLINE | PF2_SHORTAWAY;

	case PFLAGNUM_3:
		return PF2_SHORTAWAY;

	case PFLAGNUM_4:
		return PF4_NOCUSTOMAUTH | PF4_IMSENDUTF;

	case PFLAG_UNIQUEIDTEXT:
		return (DWORD_PTR) Translate("Nickname");

	case PFLAG_MAXLENOFMESSAGE:
		return 400;

	case PFLAG_UNIQUEIDSETTING:
		return (DWORD_PTR) "Nick";
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetIcon - loads an icon for the contact list

HICON __cdecl CIrcProto::GetIcon( int iconIndex )
{
	if (( iconIndex & 0xFFFF ) == PLI_PROTOCOL )
		return ( HICON )LoadImage(hInst,MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,16,16,LR_SHARED);

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetInfo - retrieves a contact info

int __cdecl CIrcProto::GetInfo( HANDLE, int )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchBasic - searches the contact by JID

struct AckBasicSearchParam
{
	char buf[ 50 ];
};

void __cdecl CIrcProto::AckBasicSearch( void* param )
{
	PROTOSEARCHRESULT psr = { 0 };
	psr.cbSize = sizeof(psr);
	psr.nick = (( AckBasicSearchParam* )param )->buf;
	ProtoBroadcastAck( m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
	ProtoBroadcastAck( m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
	delete param;
}

HANDLE __cdecl CIrcProto::SearchBasic( const char* szId )
{
	if ( szId ) {
		if (m_iDesiredStatus != ID_STATUS_OFFLINE && m_iDesiredStatus != ID_STATUS_CONNECTING && lstrlenA(szId) > 0 && !IsChannel(szId)) {
			AckBasicSearchParam* param = new AckBasicSearchParam;
			lstrcpynA( param->buf, szId, 50 );
			ircFork( &CIrcProto::AckBasicSearch, param );
			return ( HANDLE )1;
	}	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchByEmail - searches the contact by its e-mail

HANDLE __cdecl CIrcProto::SearchByEmail( const char* )
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// upsupported search functions

HANDLE __cdecl CIrcProto::SearchByName( const char*, const char*, const char* )
{
	return NULL;
}

HWND __cdecl CIrcProto::CreateExtendedSearchUI( HWND )
{
	return NULL;
}

HWND __cdecl CIrcProto::SearchAdvanced( HWND )
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvContacts

int __cdecl CIrcProto::RecvContacts( HANDLE, PROTORECVEVENT* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvFile

int __cdecl CIrcProto::RecvFile( HANDLE hContact, PROTORECVFILE* evt )
{
	CCSDATA ccs = { hContact, PSR_FILE, 0, ( LPARAM )evt };
	return CallService( MS_PROTO_RECVFILE, 0, ( LPARAM )&ccs );
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvMsg

int __cdecl CIrcProto::RecvMsg( HANDLE hContact, PROTORECVEVENT* evt )
{
	CCSDATA ccs = { hContact, PSR_MESSAGE, 0, ( LPARAM )evt };
	return CallService( MS_PROTO_RECVMSG, 0, ( LPARAM )&ccs );
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvUrl

int __cdecl CIrcProto::RecvUrl( HANDLE, PROTORECVEVENT* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendContacts

int __cdecl CIrcProto::SendContacts( HANDLE, int, int, HANDLE* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendFile - sends a file

HANDLE __cdecl CIrcProto::SendFile( HANDLE hContact, const char*, char** ppszFiles )
{
	DCCINFO* dci = NULL;
	int iPort = 0;
	int index= 0;
	DWORD size = 0;

	// do not send to channels :-P
	if ( getByte(hContact, "ChatRoom", 0) != 0)
		return 0;

	// stop if it is an active type filetransfer and the user's IP is not known
	unsigned long ulAdr = 0;
	if (m_manualHost)
		ulAdr = ConvertIPToInteger(m_mySpecifiedHostIP);
	else
		ulAdr = ConvertIPToInteger(m_IPFromServer?m_myHost:m_myLocalHost);

	if (!m_DCCPassive && !ulAdr) {
		DoEvent(GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), TranslateT("DCC ERROR: Unable to automatically resolve external IP"), NULL, NULL, NULL, true, false);
		return 0;
	}

	if ( ppszFiles[index] ) {

		//get file size
		FILE * hFile = NULL;
		while (ppszFiles[index] && hFile == 0) {
			hFile = fopen ( ppszFiles[index] , "rb" );
			if (hFile) {
				fseek (hFile , 0 , SEEK_END);
				size = ftell (hFile);
				rewind (hFile);
				fclose(hFile);
				break;
			}
			index++;
		}

		if (size == 0) {
			DoEvent(GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), TranslateT("DCC ERROR: No valid files specified"), NULL, NULL, NULL, true, false);
			return 0;
		}

		DBVARIANT dbv;
		if ( !getTString( hContact, "Nick", &dbv )) {
			// set up a basic DCCINFO struct and pass it to a DCC object
			dci = new DCCINFO;
			dci->sFileAndPath = (TCHAR *)_A2T( ppszFiles[index], getCodepage());

			int i = dci->sFileAndPath.ReverseFind( '\\' );
			if (i != -1) {
				dci->sPath = dci->sFileAndPath.Mid(0, i+1);
				dci->sFile = dci->sFileAndPath.Mid(i+1, dci->sFileAndPath.GetLength());
			}

			CMString sFileWithQuotes = dci->sFile;

			// if spaces in the filename surround witrh quotes
			if ( sFileWithQuotes.Find( ' ', 0 ) != -1) {
				sFileWithQuotes.Insert( 0, _T("\""));
				sFileWithQuotes.Insert( sFileWithQuotes.GetLength(), _T("\""));
			}

			dci->hContact = hContact;
			dci->sContactName = dbv.ptszVal;
			dci->iType = DCC_SEND;
			dci->bReverse = m_DCCPassive?true:false;
			dci->bSender = true;
			dci->dwSize = size;

			// create new dcc object
			CDccSession* dcc = new CDccSession(this,dci);

			// keep track of all objects created
			AddDCCSession(dci, dcc);

			// need to make sure that %'s are doubled to avoid having chat interpret as color codes
			CMString sFileCorrect = dci->sFile;
			ReplaceString(sFileCorrect, _T("%"), _T("%%"));

			// is it an reverse filetransfer (receiver acts as server)
			if (dci->bReverse) {
				TCHAR szTemp[256];
				PostIrcMessage( _T("/CTCP %s DCC SEND %s 200 0 %u %u"),
					dci->sContactName.c_str(), sFileWithQuotes.c_str(), dci->dwSize, dcc->iToken);

				mir_sntprintf(szTemp, SIZEOF(szTemp),
					TranslateT("DCC reversed file transfer request sent to %s [%s]"),
					dci->sContactName.c_str(), sFileCorrect.c_str());
				DoEvent(GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), szTemp, NULL, NULL, NULL, true, false);

				if (m_sendNotice) {
					mir_sntprintf(szTemp, SIZEOF(szTemp),
						_T("/NOTICE %s I am sending the file \'\002%s\002\' (%u kB) to you, please accept it. [Reverse transfer]"),
						dci->sContactName.c_str(), sFileCorrect.c_str(), dci->dwSize/1024);
					PostIrcMessage(szTemp);
				}
			}
			else { // ... normal filetransfer.
				iPort = dcc->Connect();
				if ( iPort ) {
					TCHAR szTemp[256];
					PostIrcMessage( _T("/CTCP %s DCC SEND %s %u %u %u"),
						dci->sContactName.c_str(), sFileWithQuotes.c_str(), ulAdr, iPort, dci->dwSize);

					mir_sntprintf(szTemp, SIZEOF(szTemp),
						TranslateT("DCC file transfer request sent to %s [%s]"),
						dci->sContactName.c_str(), sFileCorrect.c_str());
					DoEvent(GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(), szTemp, NULL, NULL, NULL, true, false);

					if ( m_sendNotice ) {
						mir_sntprintf(szTemp, SIZEOF(szTemp),
							_T("/NOTICE %s I am sending the file \'\002%s\002\' (%u kB) to you, please accept it. [IP: %s]"),
							dci->sContactName.c_str(), sFileCorrect.c_str(), dci->dwSize/1024, (TCHAR*)_A2T(ConvertIntegerToIP(ulAdr)));
						PostIrcMessage(szTemp);
					}
				}
				else DoEvent(GC_EVENT_INFORMATION, 0, m_info.sNick.c_str(),
					TranslateT("DCC ERROR: Unable to bind local port"), NULL, NULL, NULL, true, false);
			}

			// fix for sending multiple files
			index++;
			while( ppszFiles[index] ) {
				hFile = NULL;
				hFile = fopen( ppszFiles[index] , "rb" );
				if ( hFile ) {
					fclose(hFile);
					PostIrcMessage( _T("/DCC SEND %s ") _T(TCHAR_STR_PARAM), dci->sContactName.c_str(), ppszFiles[index]);
				}
				index++;
			}

			DBFreeVariant( &dbv );
	}	}

	if (dci)
		return dci;
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendMessage - sends a message

void __cdecl CIrcProto::AckMessageFail( void* info )
{
	ProtoBroadcastAck( m_szModuleName, info, ACKTYPE_MESSAGE, ACKRESULT_FAILED, (HANDLE) 1, (LONG)Translate("The protocol is not online"));
}

void __cdecl CIrcProto::AckMessageFailDcc( void* info )
{
	ProtoBroadcastAck( m_szModuleName, info, ACKTYPE_MESSAGE, ACKRESULT_FAILED, (HANDLE) 1, (LONG)Translate("The dcc chat connection is not active"));
}

void __cdecl CIrcProto::AckMessageSuccess( void* info )
{
	ProtoBroadcastAck( m_szModuleName, info, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

int __cdecl CIrcProto::SendMsg( HANDLE hContact, int flags, const char* pszSrc )
{
	BYTE bDcc = getByte( hContact, "DCC", 0) ;
	WORD wStatus = getWord( hContact, "Status", ID_STATUS_OFFLINE) ;
	if ( m_iDesiredStatus != ID_STATUS_OFFLINE && m_iDesiredStatus != ID_STATUS_CONNECTING && !bDcc || bDcc && wStatus == ID_STATUS_ONLINE ) {
		int codepage = getCodepage();

		TCHAR* result;
		if ( flags & PREF_UNICODE ) {
			const char* p = strchr( pszSrc, '\0' );
			if ( p != pszSrc ) {
				while ( *(++p) == '\0' )
					;
				result = mir_u2t_cp(( wchar_t* )p, codepage );
			}
			else result = mir_a2t_cp( pszSrc, codepage );
		}
		else if ( flags & PREF_UTF ) {
			#if defined( _UNICODE )
				mir_utf8decode( NEWSTR_ALLOCA(pszSrc), &result );
			#else
				result = mir_strdup( pszSrc );
				mir_utf8decodecp( result, codepage, NULL );
			#endif
		}
		else result = mir_a2t_cp( pszSrc, codepage );

		PostIrcMessageWnd(NULL, hContact, result );
		mir_free( result );
		ircFork( &CIrcProto::AckMessageSuccess, hContact );
	}
	else {
		if ( bDcc )
			ircFork( &CIrcProto::AckMessageFailDcc, hContact );
		else
			ircFork( &CIrcProto::AckMessageFail, hContact );
	}

	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendUrl

int __cdecl CIrcProto::SendUrl( HANDLE, int, const char* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SetApparentMode - sets the visibility status

int __cdecl CIrcProto::SetApparentMode( HANDLE, int )
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// SetStatus - sets the protocol status

int __cdecl CIrcProto::SetStatus( int iNewStatus )
{
	return SetStatusInternal( iNewStatus, false );
}

int CIrcProto::SetStatusInternal( int iNewStatus, bool bIsInternal )
{
	if ( !bChatInstalled )
		return 0;

	if ( iNewStatus != ID_STATUS_OFFLINE && lstrlenA(m_network) < 1 ) {
		if (lstrlen(m_nick) > 0 && !m_disableDefaultServer) {
			CQuickDlg* dlg = new CQuickDlg( this );
			dlg->Show();
			HWND hwnd = dlg->GetHwnd();
			SetWindowTextA(hwnd, "Miranda IRC");
			SetWindowText(GetDlgItem(hwnd, IDC_TEXT), TranslateT("Please choose an IRC-network to go online. This network will be the default."));
			SetWindowText(GetDlgItem(hwnd, IDC_CAPTION), TranslateT("Default network"));
			HICON hIcon = LoadIconEx(IDI_MAIN);
			SendMessage(hwnd, WM_SETICON, ICON_BIG,(LPARAM)hIcon);
			ShowWindow(hwnd, SW_SHOW);
			SetActiveWindow(hwnd);
		}
		return 0;
	}

	if ( iNewStatus != ID_STATUS_OFFLINE && (lstrlen(m_nick) <1 || lstrlen(m_userID) < 1 || lstrlen(m_name) < 1)) {
		MIRANDASYSTRAYNOTIFY msn;
		msn.cbSize = sizeof( MIRANDASYSTRAYNOTIFY );
		msn.szProto = m_szModuleName;
		msn.tszInfoTitle = TranslateT( "IRC error" );
		msn.tszInfo = TranslateT( "Connection can not be established! You have not completed all necessary fields (Nickname, User ID and m_name)." );
		msn.dwInfoFlags = NIIF_ERROR | NIIF_INTERN_UNICODE;
		msn.uTimeout = 15000;
		CallService( MS_CLIST_SYSTRAY_NOTIFY, (WPARAM)NULL,(LPARAM) &msn);
		return 0;
	}

	if ( m_perform && IsConnected())
		switch( iNewStatus ) {
			case ID_STATUS_AWAY:       DoPerform( "Event: Away" );            break;
			case ID_STATUS_NA:         DoPerform( "Event: N/A" );             break;
			case ID_STATUS_DND:        DoPerform( "Event: DND" );             break;
			case ID_STATUS_OCCUPIED:   DoPerform( "Event: Occupied" );        break;
			case ID_STATUS_FREECHAT:   DoPerform( "Event: Free for chat" );   break;
			case ID_STATUS_ONTHEPHONE: DoPerform( "Event: On the phone" );    break;
			case ID_STATUS_OUTTOLUNCH: DoPerform( "Event: Out for lunch" );   break;
			case ID_STATUS_ONLINE:     DoPerform( "Event: Available" );       break;
      }

	if ( !bIsInternal )
		m_iStatus = iNewStatus;

	if (( iNewStatus == ID_STATUS_ONLINE || iNewStatus == ID_STATUS_AWAY || iNewStatus == ID_STATUS_FREECHAT) && !IsConnected() ) //go from offline to online
		ConnectToServer();
	else if (( iNewStatus == ID_STATUS_ONLINE || iNewStatus == ID_STATUS_FREECHAT) && IsConnected() && m_iDesiredStatus == ID_STATUS_AWAY) //go to online while connected
	{
		m_statusMessage = _T("");
		PostIrcMessage( _T("/AWAY"));
		return 0;
	}
	else if ( iNewStatus == ID_STATUS_OFFLINE && IsConnected()) //go from online/away to offline
		DisconnectFromServer();
	else if ( iNewStatus == ID_STATUS_OFFLINE && !IsConnected()) //offline to offline
	{
		KillChatTimer( RetryTimer);
		return 0;
	}
	else if ( iNewStatus == ID_STATUS_AWAY && IsConnected()) //go to away while connected
		return 0;
	else if ( iNewStatus == ID_STATUS_ONLINE && IsConnected()) //already online
		return 0;
	else
		SetStatusInternal(ID_STATUS_AWAY, true);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetAwayMsg - returns a contact's away message

HANDLE __cdecl CIrcProto::GetAwayMsg( HANDLE hContact )
{
	WhoisAwayReply = _T("");
	DBVARIANT dbv;

	// bypass chat contacts.
	if ( getByte( hContact, "ChatRoom", 0 ) == 0) {
		if ( hContact && !getTString( hContact, "Nick", &dbv)) {
			int i = getWord( hContact, "Status", ID_STATUS_OFFLINE );
			if ( i != ID_STATUS_AWAY) {
				DBFreeVariant( &dbv);
				return 0;
			}
			CMString S = _T("WHOIS ");
			S += dbv.ptszVal;
			if (IsConnected())
				SendIrcMessage( S.c_str(), false);
			DBFreeVariant( &dbv);
	}	}

	return (HANDLE)1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSR_AWAYMSG

int __cdecl CIrcProto::RecvAwayMsg( HANDLE, int, PROTORECVEVENT* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AWAYMSG

int __cdecl CIrcProto::SendAwayMsg( HANDLE, HANDLE, const char* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SetAwayMsg - sets the away status message

int __cdecl CIrcProto::SetAwayMsg( int status, const char* msg )
{
	switch( status ) {
	case ID_STATUS_ONLINE:     case ID_STATUS_INVISIBLE:   case ID_STATUS_FREECHAT:
	case ID_STATUS_CONNECTING: case ID_STATUS_OFFLINE:
		break;

	default:
		CMString newStatus = _A2T( msg, getCodepage());
		ReplaceString( newStatus, _T("\r\n"), _T(" "));
		if ( m_statusMessage.IsEmpty() || msg == NULL || m_statusMessage != newStatus ) {
			if ( msg == NULL || *( char* )msg == '\0')
				m_statusMessage = _T(STR_AWAYMESSAGE);
			else
				m_statusMessage = newStatus;

			PostIrcMessage( _T("/AWAY %s"), m_statusMessage.Mid(0,450).c_str());
	}	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// UserIsTyping - sends a UTN notification

int __cdecl CIrcProto::UserIsTyping( HANDLE, int )
{
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnEvent - maintain protocol events

int __cdecl CIrcProto::OnEvent( PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam )
{
	switch( eventType ) {
	case EV_PROTO_ONLOAD:    return OnModulesLoaded( 0, 0 );
	case EV_PROTO_ONEXIT:    return OnPreShutdown( 0, 0 );
	case EV_PROTO_ONOPTIONS: return OnInitOptionsPages( wParam, lParam );
	case EV_PROTO_ONRENAME:
		{
			CLISTMENUITEM clmi = { 0 };
			clmi.cbSize = sizeof( CLISTMENUITEM );
			clmi.flags = CMIM_NAME | CMIF_TCHAR;
			clmi.ptszName = m_tszUserName;
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuRoot, ( LPARAM )&clmi );
	}	}
	return 1;
}
