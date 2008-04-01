/*
IRC plugin for Miranda IM

Copyright (C) 2003-05 Jurgen Persson
Copyright (C) 2007-08 George Hazan

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

#include <algorithm>

#include <m_genmenu.h>

static int CompareServers( const SERVER_INFO* p1, const SERVER_INFO* p2 )
{
	return lstrcmpA( p1->m_name, p2->m_name );
}

static int CompareSessions( const CDccSession* p1, const CDccSession* p2 )
{
	return int( p1->di->hContact ) - int( p2->di->hContact );
}

CIrcProto::CIrcProto( const char* szModuleName, const TCHAR* tszUserName ) :
	m_servers( 20, CompareServers ),
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

	IrcHookEvent( ME_SYSTEM_MODULESLOADED, &CIrcProto::OnModulesLoaded);
	IrcHookEvent( ME_SYSTEM_PRESHUTDOWN,   &CIrcProto::OnPreShutdown);
	IrcHookEvent( ME_DB_CONTACT_DELETED,   &CIrcProto::OnDeletedContact );

	CreateProtoService( PS_GETNAME,        &CIrcProto::GetName );
	CreateProtoService( PS_GETSTATUS,      &CIrcProto::GetStatus );
	CreateProtoService( PS_CREATEACCMGRUI, &CIrcProto::SvcCreateAccMgrUI );

	CreateProtoService( IRC_JOINCHANNEL,   &CIrcProto::OnJoinMenuCommand );
	CreateProtoService( IRC_QUICKCONNECT,  &CIrcProto::OnQuickConnectMenuCommand);
	CreateProtoService( IRC_CHANGENICK,    &CIrcProto::OnChangeNickMenuCommand );
	CreateProtoService( IRC_SHOWLIST,      &CIrcProto::OnShowListMenuCommand );
	CreateProtoService( IRC_SHOWSERVER,    &CIrcProto::OnShowServerMenuCommand );
	CreateProtoService( IRC_UM_SHOWCHANNEL, &CIrcProto::OnMenuShowChannel );
	CreateProtoService( IRC_UM_JOINLEAVE,  &CIrcProto::OnMenuJoinLeave );
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

	IRC_MAP_ENTRY("PING", OnIrc_PING)
	IRC_MAP_ENTRY("JOIN", OnIrc_JOIN)
	IRC_MAP_ENTRY("QUIT", OnIrc_QUIT)
	IRC_MAP_ENTRY("KICK", OnIrc_KICK)
	IRC_MAP_ENTRY("MODE", OnIrc_MODE)
	IRC_MAP_ENTRY("NICK", OnIrc_NICK)
	IRC_MAP_ENTRY("PART", OnIrc_PART)
	IRC_MAP_ENTRY("PRIVMSG", OnIrc_PRIVMSG)
	IRC_MAP_ENTRY("TOPIC", OnIrc_TOPIC)
	IRC_MAP_ENTRY("NOTICE", OnIrc_NOTICE)
	IRC_MAP_ENTRY("PING", OnIrc_PINGPONG)
	IRC_MAP_ENTRY("PONG", OnIrc_PINGPONG)
	IRC_MAP_ENTRY("INVITE", OnIrc_INVITE)
	IRC_MAP_ENTRY("ERROR", OnIrc_ERROR)
	IRC_MAP_ENTRY("001", OnIrc_WELCOME)
	IRC_MAP_ENTRY("002", OnIrc_YOURHOST)
	IRC_MAP_ENTRY("005", OnIrc_SUPPORT)
	IRC_MAP_ENTRY("223", OnIrc_WHOIS_OTHER)			//CodePage info
	IRC_MAP_ENTRY("254", OnIrc_NOOFCHANNELS)
	IRC_MAP_ENTRY("263", OnIrc_TRYAGAIN)
	IRC_MAP_ENTRY("264", OnIrc_WHOIS_OTHER)			//Encryption info (SSL connect)
	IRC_MAP_ENTRY("301", OnIrc_WHOIS_AWAY)
	IRC_MAP_ENTRY("302", OnIrc_USERHOST_REPLY)
	IRC_MAP_ENTRY("305", OnIrc_BACKFROMAWAY)
	IRC_MAP_ENTRY("306", OnIrc_SETAWAY)
	IRC_MAP_ENTRY("307", OnIrc_WHOIS_AUTH)
	IRC_MAP_ENTRY("310", OnIrc_WHOIS_OTHER)
	IRC_MAP_ENTRY("311", OnIrc_WHOIS_NAME)
	IRC_MAP_ENTRY("312", OnIrc_WHOIS_SERVER)
	IRC_MAP_ENTRY("313", OnIrc_WHOIS_OTHER)
	IRC_MAP_ENTRY("315", OnIrc_WHO_END)
	IRC_MAP_ENTRY("317", OnIrc_WHOIS_IDLE)
	IRC_MAP_ENTRY("318", OnIrc_WHOIS_END)
	IRC_MAP_ENTRY("319", OnIrc_WHOIS_CHANNELS)
	IRC_MAP_ENTRY("320", OnIrc_WHOIS_AUTH)
	IRC_MAP_ENTRY("321", OnIrc_LISTSTART)
	IRC_MAP_ENTRY("322", OnIrc_LIST)
	IRC_MAP_ENTRY("323", OnIrc_LISTEND)
	IRC_MAP_ENTRY("324", OnIrc_MODEQUERY)
	IRC_MAP_ENTRY("330", OnIrc_WHOIS_AUTH)
	IRC_MAP_ENTRY("332", OnIrc_INITIALTOPIC)
	IRC_MAP_ENTRY("333", OnIrc_INITIALTOPICNAME)
	IRC_MAP_ENTRY("352", OnIrc_WHO_REPLY)
	IRC_MAP_ENTRY("353", OnIrc_NAMES)
	IRC_MAP_ENTRY("366", OnIrc_ENDNAMES)
	IRC_MAP_ENTRY("367", OnIrc_BANLIST)
	IRC_MAP_ENTRY("368", OnIrc_BANLISTEND)
	IRC_MAP_ENTRY("346", OnIrc_BANLIST)
	IRC_MAP_ENTRY("347", OnIrc_BANLISTEND)
	IRC_MAP_ENTRY("348", OnIrc_BANLIST)
	IRC_MAP_ENTRY("349", OnIrc_BANLISTEND)
	IRC_MAP_ENTRY("371", OnIrc_WHOIS_OTHER)
	IRC_MAP_ENTRY("376", OnIrc_ENDMOTD)
	IRC_MAP_ENTRY("401", OnIrc_WHOIS_NO_USER)
	IRC_MAP_ENTRY("403", OnIrc_JOINERROR)
	IRC_MAP_ENTRY("416", OnIrc_WHOTOOLONG)
	IRC_MAP_ENTRY("421", OnIrc_UNKNOWN)
	IRC_MAP_ENTRY("422", OnIrc_ENDMOTD)
	IRC_MAP_ENTRY("433", OnIrc_NICK_ERR)
	IRC_MAP_ENTRY("471", OnIrc_JOINERROR)
	IRC_MAP_ENTRY("473", OnIrc_JOINERROR)
	IRC_MAP_ENTRY("474", OnIrc_JOINERROR)
	IRC_MAP_ENTRY("475", OnIrc_JOINERROR)
	IRC_MAP_ENTRY("671", OnIrc_WHOIS_OTHER)			//Encryption info (SSL connect)
}

CIrcProto::~CIrcProto()
{
	Netlib_CloseHandle(hNetlib);
	Netlib_CloseHandle(hNetlibDCC);

	DeleteCriticalSection( &cs );
	DeleteCriticalSection( &m_gchook );

	CallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hUMenuShowChannel, 0 );
	CallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hUMenuJoinLeave, 0 );
	CallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hUMenuChanSettings, 0 );
	CallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hUMenuWhois, 0 );
	CallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hUMenuDisconnect, 0 );
	CallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hUMenuIgnore, 0 );

	CallService( MS_CLIST_REMOVECONTACTMENUITEM, ( WPARAM )hMenuRoot, 0 );

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
	if ( !_memicmp( szSetting, "PERFORM:", 8 )) {
		String s = szSetting;
		s.MakeUpper();
		if ( s != szSetting ) {
			OBJLIST<String>* p = ( OBJLIST<String>* )lParam;
			p->insert( new String( szSetting ));
	}	}
	return 0;
}

int sttServerEnum( const char* szSetting, LPARAM lParam )
{
	DBVARIANT dbv;
	if ( DBGetContactSettingString( NULL, SERVERSMODULE, szSetting, &dbv ))
		return 0;

	SERVER_INFO* pData = new SERVER_INFO;
	pData->m_name = mir_strdup( szSetting );

	char* p1 = strchr( dbv.pszVal, ':' )+1;
	pData->m_iSSL = 0;
	if ( !memicmp( p1, "SSL", 3 )) {
		p1 +=3;
		if ( *p1 == '1' )
			pData->m_iSSL = 1;
		else if ( *p1 == '2' )
			pData->m_iSSL = 2;
		p1++;
	}
	char* p2 = strchr(p1, ':');
	pData->m_address = ( char* )mir_alloc( p2-p1+1 );
	lstrcpynA( pData->m_address, p1, p2-p1+1 );

	p1 = p2+1;
	while (*p2 !='G' && *p2 != '-')
		p2++;

	char* buf = ( char* )alloca( p2-p1+1 );
	lstrcpynA( buf, p1, p2-p1+1 );
	pData->m_portStart = atoi( buf );

	if ( *p2 == 'G' )
		pData->m_portEnd = pData->m_portStart;
	else {
		p1 = p2+1;
		p2 = strchr(p1, 'G');
		buf = ( char* )alloca( p2-p1+1 );
		lstrcpynA( buf, p1, p2-p1+1 );
		pData->m_portEnd = atoi( buf );
	}
   
   p1 = strchr(p2, ':')+1;
	p2 = strchr(p1, '\0');
	pData->m_group = ( char* )mir_alloc( p2-p1+1 );
	lstrcpynA( pData->m_group, p1, p2-p1+1 );

	CIrcProto* ppro = ( CIrcProto* )lParam;
	ppro->m_servers.insert( pData );
	DBFreeVariant( &dbv );
	return 0;
}

static void sttImportIni( const char* szIniFile )
{
	FILE* serverFile = fopen( szIniFile, "r" );
	if ( serverFile == NULL )
		return;

	char buf1[ 500 ], buf2[ 200 ];
	while ( fgets( buf1, sizeof( buf1 ), serverFile )) {
		char* p = strchr( buf1, '=' );
		if ( !p )
			continue;

		p++;
		rtrim( p );
		char* p1 = strstr( p, "SERVER:" );
		if ( !p1 )
			continue;

		memcpy( buf2, p, int(p1-p));
		buf2[ int(p1-p) ] = 0;
		DBWriteContactSettingString( NULL, SERVERSMODULE, buf2, p1 );
	}
	fclose( serverFile );
	::remove( szIniFile );
}

int CIrcProto::OnModulesLoaded( WPARAM wParam, LPARAM lParam )
{
	char szTemp[MAX_PATH];
	char szTemp3[256];
	NETLIBUSER nlu = {0};

	DBDeleteContactSetting( NULL, m_szModuleName, "JTemp" );

	AddIcons();

	nlu.cbSize = sizeof(nlu);
	nlu.flags = NUF_OUTGOING|NUF_INCOMING|NUF_HTTPCONNS;
	nlu.szSettingsModule = m_szModuleName;
	mir_snprintf(szTemp, sizeof(szTemp), Translate("%s server connection"), m_szModuleName);
	nlu.szDescriptiveName = szTemp;
	hNetlib=(HANDLE)CallService( MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

	char szTemp2[256];
	nlu.flags = NUF_OUTGOING|NUF_INCOMING|NUF_HTTPCONNS;
	mir_snprintf(szTemp2, sizeof(szTemp2), "%s DCC", m_szModuleName);
	nlu.szSettingsModule = szTemp2;
	mir_snprintf(szTemp, sizeof(szTemp), Translate("%s client-to-client connections"), m_szModuleName);
	nlu.szDescriptiveName = szTemp;
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

	mir_snprintf(szTemp, sizeof(szTemp), "%s\\%s_servers.ini", mirandapath, m_szModuleName);
	sttImportIni( szTemp );

	mir_snprintf(szTemp, sizeof(szTemp), "%s\\IRC_servers.ini", mirandapath, m_szModuleName);
	sttImportIni( szTemp );

	DBCONTACTENUMSETTINGS dbces;
	dbces.pfnEnumProc = sttServerEnum;
	dbces.lParam = ( LPARAM )this;
	dbces.szModule = SERVERSMODULE;
	CallService( MS_DB_CONTACT_ENUMSETTINGS, NULL, (LPARAM)&dbces );

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
			String* s = performToConvert[i];
			DBVARIANT dbv;
			if ( !getTString( s->c_str(), &dbv )) {
				DBDeleteContactSetting( NULL, m_szModuleName, s->c_str());
				s->MakeUpper();
				setTString( s->c_str(), dbv.ptszVal );
				DBFreeVariant( &dbv );
			}
			delete s;
		}

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

HANDLE __cdecl CIrcProto::AddToList( int flags, PROTOSEARCHRESULT* psr )
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

HANDLE __cdecl CIrcProto::AddToListByEvent( int flags, int iContact, HANDLE hDbEvent )
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// AuthAllow - processes the successful authorization

int __cdecl CIrcProto::Authorize( HANDLE hContact )
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// AuthDeny - handles the unsuccessful authorization

int __cdecl CIrcProto::AuthDeny( HANDLE hContact, const char* szReason )
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_ADDED

int __cdecl CIrcProto::AuthRecv( HANDLE hContact, PROTORECVEVENT* evt )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AUTHREQUEST

int __cdecl CIrcProto::AuthRequest( HANDLE hContact, const char* szMessage )
{	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// ChangeInfo 

HANDLE __cdecl CIrcProto::ChangeInfo( int iInfoType, void* pInfoData )
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// FileAllow - starts a file transfer

int __cdecl CIrcProto::FileAllow( HANDLE hContact, HANDLE hTransfer, const char* szPath )
{
	DCCINFO* di = ( DCCINFO* )hTransfer;

	if ( IsConnected() ) {
		TCHAR* ptszFileName = mir_a2t_cp( szPath, getCodepage());
		di->sPath = ptszFileName;
		di->sFileAndPath = di->sPath + di->sFile;
		mir_free( ptszFileName );

		CDccSession* dcc = new CDccSession( this, di );
		AddDCCSession( di, dcc );
		dcc->Connect();
	}
	else delete di;

	return ( int )szPath;
}

////////////////////////////////////////////////////////////////////////////////////////
// FileCancel - cancels a file transfer

int __cdecl CIrcProto::FileCancel( HANDLE hContact, HANDLE hTransfer )
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

int __cdecl CIrcProto::FileDeny( HANDLE hContact, HANDLE hTransfer, const char* szReason )
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
			char * pszTemp = NULL;
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

DWORD __cdecl CIrcProto::GetCaps( int type, HANDLE hContact )
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
		return (int) Translate("Nickname");

	case PFLAG_MAXLENOFMESSAGE:
		return 400;

	case PFLAG_UNIQUEIDSETTING:
		return (int) "Nick";
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

int __cdecl CIrcProto::GetInfo( HANDLE hContact, int infoType )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchBasic - searches the contact by JID

struct AckBasicSearchParam
{
	CIrcProto* ppro;
	char buf[ 50 ];
};

static void __cdecl AckBasicSearch( AckBasicSearchParam* param )
{
	PROTOSEARCHRESULT psr;
	ZeroMemory(&psr, sizeof(psr));
	psr.cbSize = sizeof(psr);
	psr.nick = param->buf;
	ProtoBroadcastAck( param->ppro->m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
	ProtoBroadcastAck( param->ppro->m_szModuleName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
	delete param;
}

HANDLE __cdecl CIrcProto::SearchBasic( const char* szId )
{
	if ( szId ) {
		if (m_iDesiredStatus != ID_STATUS_OFFLINE && m_iDesiredStatus != ID_STATUS_CONNECTING && lstrlenA(szId) > 0 && !IsChannel(szId)) {
			AckBasicSearchParam* param = new AckBasicSearchParam;
			param->ppro = this;
			lstrcpynA( param->buf, szId, 50 );
			mir_forkthread(( pThreadFunc )AckBasicSearch, param );
			return ( HANDLE )1;
	}	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchByEmail - searches the contact by its e-mail

HANDLE __cdecl CIrcProto::SearchByEmail( const char* email )
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// upsupported search functions

HANDLE __cdecl CIrcProto::SearchByName( const char* nick, const char* firstName, const char* lastName )
{
	return NULL;
}

HWND __cdecl CIrcProto::CreateExtendedSearchUI( HWND parent )
{
	return NULL;
}

HWND __cdecl CIrcProto::SearchAdvanced( HWND hwndDlg )
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvContacts

int __cdecl CIrcProto::RecvContacts( HANDLE hContact, PROTORECVEVENT* )
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

int __cdecl CIrcProto::RecvUrl( HANDLE hContact, PROTORECVEVENT* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendContacts

int __cdecl CIrcProto::SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendFile - sends a file

int __cdecl CIrcProto::SendFile( HANDLE hContact, const char* szDescription, char** ppszFiles )
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
			dci->sFileAndPath = (CMString)_A2T( ppszFiles[index], getCodepage());

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
		return (int)(HANDLE) dci;
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendMessage - sends a message

struct AckMessageThreadParam
{
	__inline AckMessageThreadParam( CIrcProto* p, void* i ) : ppro( p ), info( i ) {}

	CIrcProto* ppro;
	void* info;
};

static void __cdecl AckMessageFail( AckMessageThreadParam* param )
{
	ProtoBroadcastAck(param->ppro->m_szModuleName, param->info, ACKTYPE_MESSAGE, ACKRESULT_FAILED, (HANDLE) 1, (LONG)Translate("The protocol is not online"));
	delete param;
}

static void __cdecl AckMessageFailDcc( AckMessageThreadParam* param )
{
	ProtoBroadcastAck(param->ppro->m_szModuleName, param->info, ACKTYPE_MESSAGE, ACKRESULT_FAILED, (HANDLE) 1, (LONG)Translate("The dcc chat connection is not active"));
	delete param;
}

static void __cdecl AckMessageSuccess( AckMessageThreadParam* param )
{
	ProtoBroadcastAck(param->ppro->m_szModuleName, param->info, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
	delete param;
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
		mir_forkthread(( pThreadFunc )AckMessageSuccess, new AckMessageThreadParam( this, hContact ));
	}
	else {
		if ( bDcc )
			mir_forkthread(( pThreadFunc )AckMessageFailDcc, new AckMessageThreadParam( this, hContact ));
		else
			mir_forkthread(( pThreadFunc )AckMessageFail, new AckMessageThreadParam( this, hContact ));
	}

	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendUrl

int __cdecl CIrcProto::SendUrl( HANDLE hContact, int flags, const char* url )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SetApparentMode - sets the visibility status

int __cdecl CIrcProto::SetApparentMode( HANDLE hContact, int mode )
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// SetStatus - sets the protocol status

int __cdecl CIrcProto::SetStatus( int iNewStatus )
{
	if ( !bChatInstalled ) {
		MIRANDASYSTRAYNOTIFY msn;
		msn.cbSize = sizeof(MIRANDASYSTRAYNOTIFY);
		msn.szProto = m_szModuleName;
		msn.tszInfoTitle = TranslateT( "Information" );
		msn.tszInfo = TranslateT( "This protocol is dependent on another plugin named \'Chat\'\nPlease download it from the Miranda IM website!" );
		msn.dwInfoFlags = NIIF_ERROR | NIIF_INTERN_UNICODE;
		msn.uTimeout = 15000;
		CallService( MS_CLIST_SYSTRAY_NOTIFY, 0, ( LPARAM )&msn );
		return 0;
	}

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

	if ( iNewStatus == ID_STATUS_FREECHAT && m_perform && IsConnected() )
		DoPerform( "Event: Free for chat" );
	if ( iNewStatus == ID_STATUS_ONTHEPHONE && m_perform && IsConnected() )
		DoPerform( "Event: On the phone" );
	if ( iNewStatus == ID_STATUS_OUTTOLUNCH && m_perform && IsConnected() )
		DoPerform( "Event: Out for lunch" );
	if ( iNewStatus == ID_STATUS_ONLINE && m_perform && IsConnected() && (m_iStatus ==ID_STATUS_ONTHEPHONE ||m_iStatus  ==ID_STATUS_OUTTOLUNCH) && m_iDesiredStatus  !=ID_STATUS_AWAY)
		DoPerform( "Event: Available" );
	if ( iNewStatus != 1 )
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
		SetStatus(ID_STATUS_AWAY);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetAwayMsg - returns a contact's away message

int __cdecl CIrcProto::GetAwayMsg( HANDLE hContact )
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
				*this << CIrcMessage( this, S.c_str(), getCodepage(), false, false);
			DBFreeVariant( &dbv);
	}	}

	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSR_AWAYMSG

int __cdecl CIrcProto::RecvAwayMsg( HANDLE hContact, int statusMode, PROTORECVEVENT* evt )
{	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AWAYMSG

int __cdecl CIrcProto::SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg )
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

int __cdecl CIrcProto::UserIsTyping( HANDLE hContact, int type )
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
