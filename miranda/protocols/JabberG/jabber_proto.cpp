/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $URL: https://miranda.svn.sourceforge.net/svnroot/miranda/trunk/miranda/protocols/JabberG/jabber_svc.cpp $
Revision       : $Revision: 7044 $
Last change on : $Date: 2008-01-04 22:42:50 +0300 (Пт, 04 янв 2008) $
Last change by : $Author: m_mluhov $

*/

#include "jabber.h"

#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <m_addcontact.h>
#include <m_file.h>
#include <m_genmenu.h>
#include <m_icolib.h>

#include "jabber_list.h"
#include "jabber_iq.h"
#include "jabber_caps.h"
#include "jabber_disco.h"

#include "sdk/m_proto_listeningto.h"
#include "sdk/m_modernopt.h"

#pragma warning(disable:4355)

static int compareTransports( const TCHAR* p1, const TCHAR* p2 )
{	return _tcsicmp( p1, p2 );
}

static int compareListItems( const JABBER_LIST_ITEM* p1, const JABBER_LIST_ITEM* p2 )
{
	if ( p1->list != p2->list )
		return p1->list - p2->list;

	// for bookmarks, temporary contacts & groupchat members
	// resource must be used in the comparison
	if (( p1->list == LIST_ROSTER && ( p1->bUseResource == TRUE || p2->bUseResource == TRUE ))
		|| ( p1->list == LIST_BOOKMARK ) || ( p1->list == LIST_VCARD_TEMP ))
		return lstrcmpi( p1->jid, p2->jid );

	TCHAR szp1[ JABBER_MAX_JID_LEN ], szp2[ JABBER_MAX_JID_LEN ];
	JabberStripJid( p1->jid, szp1, SIZEOF( szp1 ));
	JabberStripJid( p2->jid, szp2, SIZEOF( szp2 ));
	return lstrcmpi( szp1, szp2 );
}

CJabberProto::CJabberProto( const char* aProtoName, const TCHAR* aUserName ) :
	m_options( this ),
	m_lstTransports( 50, compareTransports ),
	m_lstRoster( 50, compareListItems ),
	m_iqManager( this ),
	m_adhocManager( this ),
	m_clientCapsManager( this ),
	m_privacyListManager( this ),
	m_privacyMenuServiceAllocated( -1 ),
	m_priorityMenuVal( 0 ),
	m_priorityMenuValSet( false ),
	m_hPrivacyMenuRoot( 0 ),
	m_hPrivacyMenuItems( 10 )
{
	InitializeCriticalSection( &m_csModeMsgMutex );

	InitializeCriticalSection( &m_csLists );

	m_szXmlStreamToBeInitialized = NULL;

	m_tszUserName = mir_tstrdup( aUserName );
	m_szModuleName = mir_strdup( aProtoName );
	m_szProtoName = mir_strdup( aProtoName );
	_strlwr( m_szProtoName );
	m_szProtoName[0] = toupper( m_szProtoName[0] );
	Log( "Setting protocol/module name to '%s/%s'", m_szProtoName, m_szModuleName );

	// Jabber dialog list
	m_windowList = (HANDLE)CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);

	// Protocol services and events...
	m_hEventNudge = JCreateHookableEvent( JE_NUDGE );
	m_hEventRawXMLIn = JCreateHookableEvent( JE_RAWXMLIN );
	m_hEventRawXMLOut = JCreateHookableEvent( JE_RAWXMLOUT );
	m_hEventXStatusIconChanged = JCreateHookableEvent( JE_CUSTOMSTATUS_EXTRAICON_CHANGED );
	m_hEventXStatusChanged = JCreateHookableEvent( JE_CUSTOMSTATUS_CHANGED );

	JCreateService( PS_CREATEACCMGRUI, &CJabberProto::SvcCreateAccMgrUI );

	JCreateService( PS_GETNAME, &CJabberProto::JabberGetName );
	JCreateService( PS_GETAVATARINFO, &CJabberProto::JabberGetAvatarInfo );
	JCreateService( PS_GETSTATUS, &CJabberProto::JabberGetStatus );
	JCreateService( PS_SET_LISTENINGTO, &CJabberProto::OnSetListeningTo );

	JCreateService( JS_GETCUSTOMSTATUSICON, &CJabberProto::OnGetXStatusIcon );
	JCreateService( JS_GETXSTATUS, &CJabberProto::OnGetXStatus );
	JCreateService( JS_SETXSTATUS, &CJabberProto::OnSetXStatus );

	JCreateService( JS_HTTP_AUTH, &CJabberProto::OnHttpAuthRequest );

	JCreateService( JS_SENDXML, &CJabberProto::ServiceSendXML );
	JCreateService( PS_GETMYAVATAR, &CJabberProto::JabberGetAvatar );
	JCreateService( PS_GETAVATARCAPS, &CJabberProto::JabberGetAvatarCaps );
	JCreateService( PS_SETMYAVATAR, &CJabberProto::JabberSetAvatar );

	JCreateService( JS_GETADVANCEDSTATUSICON, &CJabberProto::JGetAdvancedStatusIcon );
	JCreateService( JS_DB_GETEVENTTEXT_CHATSTATES, &CJabberProto::OnGetEventTextChatStates );
	JCreateService( JS_DB_GETEVENTTEXT_PRESENCE, &CJabberProto::OnGetEventTextPresence );

	// XEP-0224 support (Attention/Nudge)
	JCreateService( JS_SEND_NUDGE, &CJabberProto::JabberSendNudge );

	// service to get from protocol chat buddy info
	JCreateService( MS_GC_PROTO_GETTOOLTIPTEXT, &CJabberProto::JabberGCGetToolTipText );

	// XMPP URI parser service for "File Association Manager" plugin
	JCreateService( JS_PARSE_XMPP_URI, &CJabberProto::JabberServiceParseXmppURI );

	JHookEvent( ME_CLIST_PREBUILDSTATUSMENU, &CJabberProto::OnBuildStatusMenu );
	JHookEvent( ME_DB_CONTACT_DELETED, &CJabberProto::OnContactDeleted );
	JHookEvent( ME_DB_CONTACT_SETTINGCHANGED, &CJabberProto::OnDbSettingChanged );
	JHookEvent( ME_CLIST_PREBUILDCONTACTMENU, &CJabberProto::OnPrebuildContactMenu );
	JHookEvent( ME_MODERNOPT_INITIALIZE, &CJabberProto::OnModernOptInit );
	JHookEvent( ME_OPT_INITIALISE, &CJabberProto::OnOptionsInit );
	JHookEvent( ME_SKIN2_ICONSCHANGED, &CJabberProto::OnReloadIcons );

	m_iqManager.FillPermanentHandlers();
	m_iqManager.Start();
	m_adhocManager.FillDefaultNodes();
	m_clientCapsManager.AddDefaultCaps();

	IconsInit();
	MenuInit();
	WsInit();
	IqInit();
	SerialInit();
	ConsoleInit();
	InitCustomFolders();

	m_pepServices.insert(new CPepMood(this));
	m_pepServices.insert(new CPepActivity(this));

	char text[ MAX_PATH ];
	mir_snprintf( text, sizeof( text ), "%s/Status", m_szModuleName );
	JCallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )text );
	mir_snprintf( text, sizeof( text ), "%s/%s", m_szModuleName, DBSETTING_DISPLAY_UID );
	JCallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )text );
}

CJabberProto::~CJabberProto()
{
	WsUninit();
	IqUninit();
	XStatusUninit();
	SerialUninit();
	ConsoleUninit();
	MenuUninit();

	delete m_pInfoFrame;

	DestroyHookableEvent( m_hEventNudge );
	DestroyHookableEvent( m_hEventRawXMLIn );
	DestroyHookableEvent( m_hEventRawXMLOut );
	DestroyHookableEvent( m_hEventXStatusIconChanged );
	DestroyHookableEvent( m_hEventXStatusChanged );
	if ( m_hInitChat )
		DestroyHookableEvent( m_hInitChat );

	ListWipe();
	DeleteCriticalSection( &m_csLists );

	mir_free( m_phIconLibItems );

	DeleteCriticalSection( &m_filterInfo.csPatternLock );

	DeleteCriticalSection( &m_csModeMsgMutex );
	mir_free( m_modeMsgs.szOnline );
	mir_free( m_modeMsgs.szAway );
	mir_free( m_modeMsgs.szNa );
	mir_free( m_modeMsgs.szDnd );
	mir_free( m_modeMsgs.szFreechat );

	mir_free( m_szStreamId );
	mir_free( m_szProtoName );
	mir_free( m_szModuleName );
	mir_free( m_tszUserName );

	for ( int i=0; i < m_lstTransports.getCount(); i++ )
		free( m_lstTransports[i] );
	m_lstTransports.destroy();
}

////////////////////////////////////////////////////////////////////////////////////////
// OnModulesLoadedEx - performs hook registration

static COLORREF crCols[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

int CJabberProto::OnModulesLoadedEx( WPARAM wParam, LPARAM lParam )
{
	JHookEvent( ME_USERINFO_INITIALISE, &CJabberProto::OnUserInfoInit );
	XStatusInit();
	m_pepServices.InitGui();

	m_pInfoFrame = new CJabberInfoFrame(this);

	if ( ServiceExists( MS_GC_REGISTER )) {
		jabberChatDllPresent = true;

		GCREGISTER gcr = {0};
		gcr.cbSize = sizeof( GCREGISTER );
		gcr.dwFlags = GC_TYPNOTIF|GC_CHANMGR;
		gcr.iMaxText = 0;
		gcr.nColors = 16;
		gcr.pColors = &crCols[0];
		gcr.pszModuleDispName = m_szModuleName;
		gcr.pszModule = m_szModuleName;
		CallServiceSync( MS_GC_REGISTER, NULL, ( LPARAM )&gcr );

		JHookEvent( ME_GC_EVENT, &CJabberProto::JabberGcEventHook );
		JHookEvent( ME_GC_BUILDMENU, &CJabberProto::JabberGcMenuHook );

		char szEvent[ 200 ];
		mir_snprintf( szEvent, sizeof szEvent, "%s\\ChatInit", m_szModuleName );
		m_hInitChat = CreateHookableEvent( szEvent );
		JHookEvent( szEvent, &CJabberProto::JabberGcInit );
	}

	if ( ServiceExists( MS_MSG_ADDICON )) {
		StatusIconData sid = {0};
		sid.cbSize = sizeof(sid);
		sid.szModule = m_szModuleName;
		sid.hIcon = LoadIconEx("main");
		sid.hIconDisabled = LoadIconEx("main");
		sid.flags = MBF_HIDDEN;
		sid.szTooltip = "Jabber Resource";
		CallService(MS_MSG_ADDICON, 0, (LPARAM) &sid);
		JHookEvent( ME_MSG_ICONPRESSED, &CJabberProto::OnProcessSrmmIconClick );
		JHookEvent( ME_MSG_WINDOWEVENT, &CJabberProto::OnProcessSrmmEvent );

		HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		while ( hContact != NULL ) {
			char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
			if ( szProto != NULL && !strcmp( szProto, m_szModuleName ))
				MenuHideSrmmIcon(hContact);
			hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
	}	}

	DBEVENTTYPEDESCR dbEventType = {0};
	dbEventType.cbSize = sizeof(DBEVENTTYPEDESCR);
	dbEventType.eventType = JABBER_DB_EVENT_TYPE_CHATSTATES;
	dbEventType.module = m_szModuleName;
	dbEventType.descr = "Chat state notifications";
	JCallService( MS_DB_EVENT_REGISTERTYPE, 0, (LPARAM)&dbEventType );

	dbEventType.eventType = JABBER_DB_EVENT_TYPE_PRESENCE;
	dbEventType.module = m_szModuleName;
	dbEventType.descr = "Presence notifications";
	JCallService( MS_DB_EVENT_REGISTERTYPE, 0, (LPARAM)&dbEventType );

	JHookEvent( ME_IDLE_CHANGED, &CJabberProto::OnIdleChanged );

	CheckAllContactsAreTransported();

	// Set all contacts to offline
	HANDLE hContact = ( HANDLE )CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) {
		char* szProto = ( char* )CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
		if ( szProto != NULL && !strcmp( szProto, m_szModuleName )) {
			SetContactOfflineStatus( hContact );

			if ( JGetByte( hContact, "IsTransport", 0 )) {
				DBVARIANT dbv;
				if ( !JGetStringT( hContact, "jid", &dbv )) {
					TCHAR* domain = NEWTSTR_ALLOCA(dbv.ptszVal);
					TCHAR* resourcepos = _tcschr( domain, '/' );
					if ( resourcepos != NULL )
						*resourcepos = '\0';
					m_lstTransports.insert( _tcsdup( domain ));
					JFreeVariant( &dbv );
		}	}	}

		hContact = ( HANDLE )CallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberAddToList - adds a contact to the contact list

HANDLE CJabberProto::AddToListByJID( const TCHAR* newJid, DWORD flags )
{
	HANDLE hContact;
	TCHAR* jid, *nick;

	Log( "AddToListByJID jid = " TCHAR_STR_PARAM, newJid );

	if (( hContact=HContactFromJID( newJid )) == NULL ) {
		// not already there: add
		jid = mir_tstrdup( newJid );
		Log( "Add new jid to contact jid = " TCHAR_STR_PARAM, jid );
		hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_ADD, 0, 0 );
		JCallService( MS_PROTO_ADDTOCONTACT, ( WPARAM ) hContact, ( LPARAM )m_szModuleName );
		JSetStringT( hContact, "jid", jid );
		if (( nick=JabberNickFromJID( newJid )) == NULL )
			nick = mir_tstrdup( newJid );
//		JSetStringT( hContact, "Nick", nick );
		mir_free( nick );
		mir_free( jid );

		// Note that by removing or disable the "NotOnList" will trigger
		// the plugin to add a particular contact to the roster list.
		// See DBSettingChanged hook at the bottom part of this source file.
		// But the add module will delete "NotOnList". So we will not do it here.
		// Also because we need "MyHandle" and "Group" info, which are set after
		// PS_ADDTOLIST is called but before the add dialog issue deletion of
		// "NotOnList".
		// If temporary add, "NotOnList" won't be deleted, and that's expected.
		DBWriteContactSettingByte( hContact, "CList", "NotOnList", 1 );
		if ( flags & PALF_TEMPORARY )
			DBWriteContactSettingByte( hContact, "CList", "Hidden", 1 );
	}
	else {
		// already exist
		// Set up a dummy "NotOnList" when adding permanently only
		if ( !( flags & PALF_TEMPORARY ))
			DBWriteContactSettingByte( hContact, "CList", "NotOnList", 1 );
	}

	if (hContact && newJid)
		DBCheckIsTransportedContact( newJid, hContact );
	return hContact;
}

HANDLE CJabberProto::AddToList( int flags, PROTOSEARCHRESULT* psr )
{
	if ( psr->cbSize != sizeof( JABBER_SEARCH_RESULT ))
		return NULL;

	JABBER_SEARCH_RESULT* jsr = ( JABBER_SEARCH_RESULT* )psr;
	return AddToListByJID( jsr->jid, flags );
}

HANDLE __cdecl CJabberProto::AddToListByEvent( int flags, int iContact, HANDLE hDbEvent )
{
	DBEVENTINFO dbei;
	HANDLE hContact;
	char* nick, *firstName, *lastName, *jid;

	Log( "AddToListByEvent" );
	ZeroMemory( &dbei, sizeof( dbei ));
	dbei.cbSize = sizeof( dbei );
	if (( dbei.cbBlob = JCallService( MS_DB_EVENT_GETBLOBSIZE, ( WPARAM )hDbEvent, 0 )) == ( DWORD )( -1 ))
		return NULL;
	if (( dbei.pBlob=( PBYTE ) alloca( dbei.cbBlob )) == NULL )
		return NULL;
	if ( JCallService( MS_DB_EVENT_GET, ( WPARAM )hDbEvent, ( LPARAM )&dbei ))
		return NULL;
	if ( strcmp( dbei.szModule, m_szModuleName ))
		return NULL;

/*
	// EVENTTYPE_CONTACTS is when adding from when we receive contact list ( not used in Jabber )
	// EVENTTYPE_ADDED is when adding from when we receive "You are added" ( also not used in Jabber )
	// Jabber will only handle the case of EVENTTYPE_AUTHREQUEST
	// EVENTTYPE_AUTHREQUEST is when adding from the authorization request dialog
*/

	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST )
		return NULL;

	nick = ( char* )( dbei.pBlob + sizeof( DWORD )+ sizeof( HANDLE ));
	firstName = nick + strlen( nick ) + 1;
	lastName = firstName + strlen( firstName ) + 1;
	jid = lastName + strlen( lastName ) + 1;

	TCHAR* newJid = mir_a2t( jid );
	hContact = ( HANDLE ) AddToListByJID( newJid, flags );
	mir_free( newJid );
	return hContact;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberAuthAllow - processes the successful authorization

int CJabberProto::Authorize( HANDLE hContact )
{
	DBEVENTINFO dbei;
	char* nick, *firstName, *lastName, *jid;

	if ( !m_bJabberOnline )
		return 1;

	memset( &dbei, 0, sizeof( dbei ) );
	dbei.cbSize = sizeof( dbei );
	if (( dbei.cbBlob=JCallService( MS_DB_EVENT_GETBLOBSIZE, ( WPARAM )hContact, 0 )) == ( DWORD )( -1 ))
		return 1;
	if (( dbei.pBlob=( PBYTE )alloca( dbei.cbBlob )) == NULL )
		return 1;
	if ( JCallService( MS_DB_EVENT_GET, ( WPARAM )hContact, ( LPARAM )&dbei ))
		return 1;
	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST )
		return 1;
	if ( strcmp( dbei.szModule, m_szModuleName ))
		return 1;

	nick = ( char* )( dbei.pBlob + sizeof( DWORD )+ sizeof( HANDLE ));
	firstName = nick + strlen( nick ) + 1;
	lastName = firstName + strlen( firstName ) + 1;
	jid = lastName + strlen( lastName ) + 1;

	Log( "Send 'authorization allowed' to " TCHAR_STR_PARAM, jid );

	m_ThreadInfo->send( XmlNode( _T("presence")) << XATTR( _T("to"), _A2T(jid)) << XATTR( _T("type"), _T("subscribed")));

	TCHAR* newJid = mir_a2t( jid );

	// Automatically add this user to my roster if option is enabled
	if ( m_options.AutoAdd == TRUE ) {
		HANDLE hContact;
		JABBER_LIST_ITEM *item;

		if (( item = ListGetItemPtr( LIST_ROSTER, newJid )) == NULL || ( item->subscription != SUB_BOTH && item->subscription != SUB_TO )) {
			Log( "Try adding contact automatically jid = " TCHAR_STR_PARAM, jid );
			if (( hContact=AddToListByJID( newJid, 0 )) != NULL ) {
				// Trigger actual add by removing the "NotOnList" added by AddToListByJID()
				// See AddToListByJID() and JabberDbSettingChanged().
				DBDeleteContactSetting( hContact, "CList", "NotOnList" );
	}	}	}

	mir_free( newJid );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberAuthDeny - handles the unsuccessful authorization

int CJabberProto::AuthDeny( HANDLE hContact, const char* szReason )
{
	DBEVENTINFO dbei;
	char* nick, *firstName, *lastName, *jid;

	if ( !m_bJabberOnline )
		return 1;

	Log( "Entering AuthDeny" );
	memset( &dbei, 0, sizeof( dbei ) );
	dbei.cbSize = sizeof( dbei );
	if (( dbei.cbBlob=JCallService( MS_DB_EVENT_GETBLOBSIZE, ( WPARAM )hContact, 0 )) == ( DWORD )( -1 ))
		return 1;
	if (( dbei.pBlob=( PBYTE ) mir_alloc( dbei.cbBlob )) == NULL )
		return 1;
	if ( JCallService( MS_DB_EVENT_GET, ( WPARAM )hContact, ( LPARAM )&dbei )) {
		mir_free( dbei.pBlob );
		return 1;
	}
	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST ) {
		mir_free( dbei.pBlob );
		return 1;
	}
	if ( strcmp( dbei.szModule, m_szModuleName )) {
		mir_free( dbei.pBlob );
		return 1;
	}

	nick = ( char* )( dbei.pBlob + sizeof( DWORD )+ sizeof( HANDLE ));
	firstName = nick + strlen( nick ) + 1;
	lastName = firstName + strlen( firstName ) + 1;
	jid = lastName + strlen( lastName ) + 1;

	Log( "Send 'authorization denied' to " TCHAR_STR_PARAM, jid );

	m_ThreadInfo->send( XmlNode( _T("presence")) << XATTR( _T("to"), _A2T(jid)) << XATTR( _T("type"), _T("unsubscribed")));

	mir_free( dbei.pBlob );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSR_AUTH

int __cdecl CJabberProto::AuthRecv( HANDLE hContact, PROTORECVEVENT* evt )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AUTHREQUEST

int __cdecl CJabberProto::AuthRequest( HANDLE hContact, const char* szMessage )
{	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// ChangeInfo 

HANDLE __cdecl CJabberProto::ChangeInfo( int iInfoType, void* pInfoData )
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberFileAllow - starts a file transfer

int __cdecl CJabberProto::FileAllow( HANDLE hContact, HANDLE hTransfer, const char* szPath )
{
	if ( !m_bJabberOnline )
		return 0;

	filetransfer* ft = ( filetransfer* )hTransfer;
	ft->std.workingDir = mir_strdup( szPath );
	int len = strlen( ft->std.workingDir )-1;
	if ( ft->std.workingDir[len] == '/' || ft->std.workingDir[len] == '\\' )
		ft->std.workingDir[len] = 0;

	switch ( ft->type ) {
	case FT_OOB:
		JForkThread(( JThreadFunc )&CJabberProto::FileReceiveThread, ft );
		break;
	case FT_BYTESTREAM:
		FtAcceptSiRequest( ft );
		break;
	case FT_IBB:
		FtAcceptIbbRequest( ft );
		break;
	}
	return int( hTransfer );
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberFileCancel - cancels a file transfer

int __cdecl CJabberProto::FileCancel( HANDLE hContact, HANDLE hTransfer )
{
	filetransfer* ft = ( filetransfer* )hTransfer;
	HANDLE hEvent;

	Log( "Invoking FileCancel()" );
	if ( ft->type == FT_OOB ) {
		if ( ft->s ) {
			Log( "FT canceled" );
			Log( "Closing ft->s = %d", ft->s );
			ft->state = FT_ERROR;
			Netlib_CloseHandle( ft->s );
			ft->s = NULL;
			if ( ft->hFileEvent != NULL ) {
				hEvent = ft->hFileEvent;
				ft->hFileEvent = NULL;
				SetEvent( hEvent );
			}
			Log( "ft->s is now NULL, ft->state is now FT_ERROR" );
		}
	}
	else FtCancel( ft );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberFileDeny - denies a file transfer

int __cdecl CJabberProto::FileDeny( HANDLE hContact, HANDLE hTransfer, const char* szReason )
{
	if ( !m_bJabberOnline )
		return 1;

	filetransfer* ft = ( filetransfer* )hTransfer;

	switch ( ft->type ) {
	case FT_OOB:
		m_ThreadInfo->send( XmlNodeIq( _T("error"), ft->iqId, ft->jid ) << XCHILD( _T("error"), _T("File transfer refused")) << XATTRI( _T("code"), 406 ));
		break;

	case FT_BYTESTREAM:
	case FT_IBB:
		m_ThreadInfo->send(
			XmlNodeIq( _T("error"), ft->iqId, ft->jid )
				<< XCHILD( _T("error"), _T("File transfer refused")) << XATTRI( _T("code"), 403 ) << XATTR( _T("type"), _T("cancel"))
					<< XCHILDNS( _T("forbidden"), _T("urn:ietf:params:xml:ns:xmpp-stanzas"))
						<< XCHILD( _T("text"), _T("File transfer refused")) << XATTR( _T("xmlns"), _T("urn:ietf:params:xml:ns:xmpp-stanzas")));
		break;
	}
	delete ft;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberFileResume - processes file renaming etc

int __cdecl CJabberProto::FileResume( HANDLE hTransfer, int* action, const char** szFilename )
{
	filetransfer* ft = ( filetransfer* )hTransfer;
	if ( !m_bJabberOnline || ft == NULL )
		return 1;

	if ( *action == FILERESUME_RENAME ) {
		if ( ft->wszFileName != NULL ) {
			mir_free( ft->wszFileName );
			ft->wszFileName = NULL;
		}

		replaceStr( ft->std.currentFile, *szFilename );
	}

	SetEvent( ft->hWaitEvent );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetCaps - return protocol capabilities bits

DWORD __cdecl CJabberProto::GetCaps( int type, HANDLE hContact )
{
	switch( type ) {
	case PFLAGNUM_1:
		return PF1_IM | PF1_AUTHREQ | PF1_CHAT | PF1_SERVERCLIST | PF1_MODEMSG|PF1_BASICSEARCH | PF1_EXTSEARCH | PF1_FILE;
	case PFLAGNUM_2:
		return PF2_ONLINE | PF2_INVISIBLE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_HEAVYDND | PF2_FREECHAT;
	case PFLAGNUM_3:
		return PF2_ONLINE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_HEAVYDND | PF2_FREECHAT;
	case PFLAGNUM_4:
		return PF4_FORCEAUTH | PF4_NOCUSTOMAUTH | PF4_SUPPORTTYPING | PF4_AVATARS | PF4_IMSENDUTF;
	case PFLAG_UNIQUEIDTEXT:
		return ( DWORD ) JTranslate( "JID" );
	case PFLAG_UNIQUEIDSETTING:
		return ( DWORD ) "jid";
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetIcon - loads an icon for the contact list

HICON __cdecl CJabberProto::GetIcon( int iconIndex )
{
	if (( iconIndex & 0xffff ) == PLI_PROTOCOL )
		return CopyIcon( LoadIconEx( "main" ));

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetInfo - retrieves a contact info

int __cdecl CJabberProto::GetInfo( HANDLE hContact, int infoType )
{
	if ( !m_bJabberOnline )
		return 1;

	int result = 1;
	DBVARIANT dbv;
	if ( JGetByte( hContact, "ChatRoom" , 0) )
		return 1;

	if ( !JGetStringT( hContact, "jid", &dbv )) {
		if ( m_ThreadInfo ) {
			TCHAR jid[ 256 ];
			GetClientJID( dbv.ptszVal, jid, SIZEOF( jid ));

			m_ThreadInfo->send(
				XmlNodeIq( m_iqManager.AddHandler( &CJabberProto::OnIqResultEntityTime, JABBER_IQ_TYPE_GET, jid, JABBER_IQ_PARSE_HCONTACT ))
					<< XCHILDNS( _T("time"), _T(JABBER_FEAT_ENTITY_TIME)));

			// XEP-0012, last logoff time
			XmlNodeIq iq2( m_iqManager.AddHandler( &CJabberProto::OnIqResultLastActivity, JABBER_IQ_TYPE_GET, dbv.ptszVal, JABBER_IQ_PARSE_FROM ));
			iq2 << XQUERY( _T(JABBER_FEAT_LAST_ACTIVITY));
			m_ThreadInfo->send( iq2 );

			JABBER_LIST_ITEM *item = NULL;

			if (( item = ListGetItemPtr( LIST_VCARD_TEMP, dbv.ptszVal )) == NULL)
				item = ListGetItemPtr( LIST_ROSTER, dbv.ptszVal );

			if ( !item ) {
				TCHAR szBareJid[ 1024 ];
				_tcsncpy( szBareJid, dbv.ptszVal, 1023 );
				TCHAR* pDelimiter = _tcschr( szBareJid, _T('/') );
				if ( pDelimiter ) {
					*pDelimiter = 0;
					pDelimiter++;
					if ( !*pDelimiter )
						pDelimiter = NULL;
				}
				JABBER_LIST_ITEM *tmpItem = NULL;
				if ( pDelimiter && ( tmpItem  = ListGetItemPtr( LIST_CHATROOM, szBareJid ))) {
					JABBER_RESOURCE_STATUS *him = NULL;
					for ( int i=0; i < tmpItem->resourceCount; i++ ) {
						JABBER_RESOURCE_STATUS& p = tmpItem->resource[i];
						if ( !lstrcmp( p.resourceName, pDelimiter )) him = &p;
					}
					if ( him ) {
						item = ListAdd( LIST_VCARD_TEMP, dbv.ptszVal );
						ListAddResource( LIST_VCARD_TEMP, dbv.ptszVal, him->status, him->statusMessage, him->priority );
					}
				}
				else
					item = ListAdd( LIST_VCARD_TEMP, dbv.ptszVal );
			}

			if ( item ) {
				if ( item->resource ) {
					for ( int i = 0; i < item->resourceCount; i++ ) {
						TCHAR szp1[ JABBER_MAX_JID_LEN ];
						JabberStripJid( dbv.ptszVal, szp1, SIZEOF( szp1 ));
						mir_sntprintf( jid, 256, _T("%s/%s"), szp1, item->resource[i].resourceName );

						XmlNodeIq iq3( m_iqManager.AddHandler( &CJabberProto::OnIqResultLastActivity, JABBER_IQ_TYPE_GET, jid, JABBER_IQ_PARSE_FROM ));
						iq3 << XQUERY( _T(JABBER_FEAT_LAST_ACTIVITY));
						m_ThreadInfo->send( iq3 );

						if ( !item->resource[i].dwVersionRequestTime ) {
							XmlNodeIq iq4( m_iqManager.AddHandler( &CJabberProto::OnIqResultVersion, JABBER_IQ_TYPE_GET, jid, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_HCONTACT | JABBER_IQ_PARSE_CHILD_TAG_NODE ));
							HXML query = iq4 << XQUERY( _T(JABBER_FEAT_VERSION));
							m_ThreadInfo->send( iq4 );
					}	}
				}
				else if ( !item->itemResource.dwVersionRequestTime ) {
					XmlNodeIq iq4( m_iqManager.AddHandler( &CJabberProto::OnIqResultVersion, JABBER_IQ_TYPE_GET, item->jid, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_HCONTACT | JABBER_IQ_PARSE_CHILD_TAG_NODE ));
					HXML query = iq4 << XQUERY( _T(JABBER_FEAT_VERSION));
					m_ThreadInfo->send( iq4 );
		}	}	}

		SendGetVcard( dbv.ptszVal );
		JFreeVariant( &dbv );
		result = 0;
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchBasic - searches the contact by JID

struct JABBER_SEARCH_BASIC
{	int hSearch;
	char jid[128];
};

void __cdecl CJabberProto::BasicSearchThread( JABBER_SEARCH_BASIC *jsb )
{
	SleepEx( 100, TRUE );

	JABBER_SEARCH_RESULT jsr = { 0 };
	jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
	jsr.hdr.nick = "";
	jsr.hdr.firstName = "";
	jsr.hdr.lastName = "";
	jsr.hdr.email = jsb->jid;

	TCHAR* jid = mir_a2t(jsb->jid);
	_tcsncpy( jsr.jid, jid, SIZEOF( jsr.jid ));
	mir_free( jid );

	jsr.jid[SIZEOF( jsr.jid )-1] = '\0';
	JSendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, ( HANDLE ) jsb->hSearch, ( LPARAM )&jsr );
	JSendBroadcast( NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, ( HANDLE ) jsb->hSearch, 0 );
	mir_free( jsb );
}

HANDLE __cdecl CJabberProto::SearchBasic( const char* szJid )
{
	Log( "JabberBasicSearch called with lParam = '%s'", szJid );

	JABBER_SEARCH_BASIC *jsb;
	if ( !m_bJabberOnline || ( jsb=( JABBER_SEARCH_BASIC * ) mir_alloc( sizeof( JABBER_SEARCH_BASIC )) )==NULL )
		return 0;

	if ( strchr( szJid, '@' ) == NULL ) {
		const char* p = strstr( szJid, m_ThreadInfo->server );
		if ( !p ) {
			char szServer[ 100 ];
			if ( JGetStaticString( "LoginServer", NULL, szServer, sizeof szServer ))
				strcpy( szServer, "jabber.org" );

			mir_snprintf( jsb->jid, SIZEOF(jsb->jid), "%s@%s", szJid, szServer );
		}
		else strncpy( jsb->jid, szJid, SIZEOF(jsb->jid));
	}
	else strncpy( jsb->jid, szJid, SIZEOF(jsb->jid));

	Log( "Adding '%s' without validation", jsb->jid );
	jsb->hSearch = SerialNext();
	JForkThread(( JThreadFunc )&CJabberProto::BasicSearchThread, jsb );
	return ( HANDLE )jsb->hSearch;
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchByEmail - searches the contact by its e-mail

HANDLE __cdecl CJabberProto::SearchByEmail( const char* email )
{
	if ( !m_bJabberOnline ) return 0;
	if ( email == NULL ) return 0;

	char szServerName[100];
	if ( JGetStaticString( "Jud", NULL, szServerName, sizeof szServerName ))
		strcpy( szServerName, "users.jabber.org" );

	int iqId = SerialNext();
	IqAdd( iqId, IQ_PROC_GETSEARCH, &CJabberProto::OnIqResultSetSearch );
	m_ThreadInfo->send( XmlNodeIq( _T("set"), iqId, _A2T(szServerName)) << XQUERY( _T("jabber:iq:search")) 
		<< XCHILD( _T("email"), _A2T(email)));
	return ( HANDLE )iqId;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberSearchByName - searches the contact by its first or last name, or by a nickname

HANDLE __cdecl CJabberProto::SearchByName( const char* nick, const char* firstName, const char* lastName )
{
	if ( !m_bJabberOnline )
		return NULL;

	BOOL bIsExtFormat = m_options.ExtendedSearch;

	char szServerName[100];
	if ( JGetStaticString( "Jud", NULL, szServerName, sizeof szServerName ))
		strcpy( szServerName, "users.jabber.org" );

	int iqId = SerialNext();
	XmlNodeIq iq( _T("set"), iqId, _A2T(szServerName));
	HXML query = iq << XQUERY( _T("jabber:iq:search")), x;

	if ( bIsExtFormat ) {
		IqAdd( iqId, IQ_PROC_GETSEARCH, &CJabberProto::OnIqResultExtSearch );

		TCHAR *szXmlLang = GetXmlLang();
		if ( szXmlLang ) {
			iq << XATTR( _T("xml:lang"), szXmlLang );
			mir_free( szXmlLang );
		}
		x = query << XCHILDNS( _T("x"), _T(JABBER_FEAT_DATA_FORMS)) << XATTR( _T("type"), _T("submit"));
	}
	else IqAdd( iqId, IQ_PROC_GETSEARCH, &CJabberProto::OnIqResultSetSearch );

	if ( nick[0] != '\0' ) {
		if ( bIsExtFormat )
			x << XCHILD( _T("field")) << XATTR( _T("var"), _T("user")) << XATTR( _T("value"), _A2T(nick));
		else 
			query << XCHILD( _T("nick"), _A2T(nick));
	}

	if ( firstName[0] != '\0' ) {
		if ( bIsExtFormat )
			x << XCHILD( _T("field")) << XATTR( _T("var"), _T("fn")) << XATTR( _T("value"), _A2T(firstName));
		else 
			query << XCHILD( _T("first"), _A2T(firstName));
	}

	if ( lastName[0] != '\0' ) {
		if ( bIsExtFormat )
			x << XCHILD( _T("field")) << XATTR( _T("var"), _T("given")) << XATTR( _T("value"), _A2T(lastName));
		else
			query << XCHILD( _T("last"), _A2T(lastName));
	}

	m_ThreadInfo->send( iq );
	return ( HANDLE )iqId;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvContacts

int __cdecl CJabberProto::RecvContacts( HANDLE hContact, PROTORECVEVENT* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvFile

int __cdecl CJabberProto::RecvFile( HANDLE hContact, PROTORECVFILE* evt )
{
	CCSDATA ccs = { hContact, PSR_FILE, 0, ( LPARAM )evt };
	return JCallService( MS_PROTO_RECVFILE, 0, ( LPARAM )&ccs );
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvMsg

int __cdecl CJabberProto::RecvMsg( HANDLE hContact, PROTORECVEVENT* evt )
{
	CCSDATA ccs = { hContact, PSR_MESSAGE, 0, ( LPARAM )evt };
	return JCallService( MS_PROTO_RECVMSG, 0, ( LPARAM )&ccs );
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvUrl

int __cdecl CJabberProto::RecvUrl( HANDLE hContact, PROTORECVEVENT* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendContacts

int __cdecl CJabberProto::SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendFile - sends a file

int __cdecl CJabberProto::SendFile( HANDLE hContact, const char* szDescription, char** ppszFiles )
{
	if ( !m_bJabberOnline ) return 0;

	if ( JGetWord( hContact, "Status", ID_STATUS_OFFLINE ) == ID_STATUS_OFFLINE )
		return 0;

	DBVARIANT dbv;
	if ( JGetStringT( hContact, "jid", &dbv ))
		return 0;

	int i, j;
	struct _stat statbuf;
	JABBER_LIST_ITEM* item = ListGetItemPtr( LIST_ROSTER, dbv.ptszVal );
	if ( item == NULL ) {
		JFreeVariant( &dbv );
		return 0;
	}

	// Check if another file transfer session request is pending ( waiting for disco result )
	if ( item->ft != NULL ) {
		JFreeVariant( &dbv );
		return 0;
	}

	JabberCapsBits jcb = GetResourceCapabilites( item->jid, TRUE );

	// fix for very smart clients, like gajim
	if ( !m_options.BsDirect && !m_options.BsProxyManual ) {
		// disable bytestreams
		jcb &= ~JABBER_CAPS_BYTESTREAMS;
	}

	// if only JABBER_CAPS_SI_FT feature set (without BS or IBB), disable JABBER_CAPS_SI_FT
	if (( jcb & (JABBER_CAPS_SI_FT | JABBER_CAPS_IBB | JABBER_CAPS_BYTESTREAMS)) == JABBER_CAPS_SI_FT)
		jcb &= ~JABBER_CAPS_SI_FT;

	if (
		// can't get caps
		( jcb & JABBER_RESOURCE_CAPS_ERROR )
		// caps not already received
		|| ( jcb == JABBER_RESOURCE_CAPS_NONE )
		// XEP-0096 and OOB not supported?
		|| !(jcb & ( JABBER_CAPS_SI_FT | JABBER_CAPS_OOB ) )
		) {
		JFreeVariant( &dbv );
		return 0;
	}

	filetransfer* ft = new filetransfer(this);
	ft->std.hContact = hContact;
	while( ppszFiles[ ft->std.totalFiles ] != NULL )
		ft->std.totalFiles++;

	ft->std.files = ( char** ) mir_alloc( sizeof( char* )* ft->std.totalFiles );
	ft->fileSize = ( long* ) mir_alloc( sizeof( long ) * ft->std.totalFiles );
	for( i=j=0; i < ft->std.totalFiles; i++ ) {
		if ( _stat( ppszFiles[i], &statbuf ))
			Log( "'%s' is an invalid filename", ppszFiles[i] );
		else {
			ft->std.files[j] = mir_strdup( ppszFiles[i] );
			ft->fileSize[j] = statbuf.st_size;
			j++;
			ft->std.totalBytes += statbuf.st_size;
	}	}

	ft->std.currentFile = mir_strdup( ppszFiles[0] );
	ft->szDescription = mir_strdup( szDescription );
	ft->jid = mir_tstrdup( dbv.ptszVal );
	JFreeVariant( &dbv );

	if ( jcb & JABBER_CAPS_SI_FT )
		FtInitiate( item->jid, ft );
	else if ( jcb & JABBER_CAPS_OOB )
		JForkThread(( JThreadFunc )&CJabberProto::FileServerThread, ft );

	return ( int )( HANDLE ) ft;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberSendMessage - sends a message

void __cdecl CJabberProto::SendMessageAckThread( void* hContact )
{
	SleepEx( 10, TRUE );
	Log( "Broadcast ACK" );
	JSendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, ( HANDLE ) 1, 0 );
	Log( "Returning from thread" );
}

static char PGP_PROLOG[] = "-----BEGIN PGP MESSAGE-----\r\n\r\n";
static char PGP_EPILOG[] = "\r\n-----END PGP MESSAGE-----\r\n";

int __cdecl CJabberProto::SendMsg( HANDLE hContact, int flags, const char* pszSrc )
{
	int id;

	DBVARIANT dbv;
	if ( !m_bJabberOnline || JGetStringT( hContact, "jid", &dbv )) {
		JSendBroadcast( hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, ( HANDLE ) 1, 0 );
		return 0;
	}

	TCHAR *msg;
	int  isEncrypted;

	if ( !strncmp( pszSrc, PGP_PROLOG, strlen( PGP_PROLOG ))) {
		const char* szEnd = strstr( pszSrc, PGP_EPILOG );
		char* tempstring = ( char* )alloca( strlen( pszSrc ) + 1 );
		int nStrippedLength = strlen(pszSrc) - strlen(PGP_PROLOG) - (szEnd ? strlen(szEnd) : 0);
		strncpy( tempstring, pszSrc + strlen(PGP_PROLOG), nStrippedLength );
		tempstring[ nStrippedLength ] = 0;
		pszSrc = tempstring;
		isEncrypted = 1;
		flags &= ~PREF_UNICODE;
	}
	else isEncrypted = 0;

	if ( flags & PREF_UTF ) {
		#if defined( _UNICODE )
			mir_utf8decode( NEWSTR_ALLOCA( pszSrc ), &msg );
		#else
			msg = mir_strdup( mir_utf8decode( NEWSTR_ALLOCA( pszSrc ), 0 ));
		#endif
	}
	else if ( flags & PREF_UNICODE )
		msg = mir_u2t(( wchar_t* )&pszSrc[ strlen( pszSrc )+1 ] );
	else
		msg = mir_a2t( pszSrc );

	int nSentMsgId = 0;

	if ( msg != NULL ) {
		TCHAR* msgType;
		if ( ListExist( LIST_CHATROOM, dbv.ptszVal ) && _tcschr( dbv.ptszVal, '/' )==NULL )
			msgType = _T("groupchat");
		else
			msgType = _T("chat");

		XmlNode m( _T("message" )); xmlAddAttr( m, _T("type"), msgType );
		if ( !isEncrypted )
			m << XCHILD( _T("body"), msg );
		else {
			m << XCHILD( _T("body"), _T("[This message is encrypted.]" ));
			m << XCHILDNS( _T("x"), _T("jabber:x:encrypted"));
		}
		mir_free( msg );

		TCHAR szClientJid[ 256 ];
		GetClientJID( dbv.ptszVal, szClientJid, SIZEOF( szClientJid ));

		JABBER_RESOURCE_STATUS *r = ResourceInfoFromJID( szClientJid );
		if ( r )
			r->bMessageSessionActive = TRUE;

		JabberCapsBits jcb = GetResourceCapabilites( szClientJid, TRUE );

		if ( jcb & JABBER_RESOURCE_CAPS_ERROR )
			jcb = JABBER_RESOURCE_CAPS_NONE;

		if ( jcb & JABBER_CAPS_CHATSTATES )
			m << XCHILDNS( _T("active"), _T(JABBER_FEAT_CHATSTATES));

		if (
			// if message delivery check disabled by entity caps manager
			( jcb & JABBER_CAPS_MESSAGE_EVENTS_NO_DELIVERY ) ||
			// if client knows nothing about delivery
			!( jcb & ( JABBER_CAPS_MESSAGE_EVENTS | JABBER_CAPS_MESSAGE_RECEIPTS )) ||
			// if message sent to groupchat
			!lstrcmp( msgType, _T("groupchat")) ||
			// if message delivery check disabled in settings
			!m_options.MsgAck || !JGetByte( hContact, "MsgAck", TRUE )) {
			if ( !lstrcmp( msgType, _T("groupchat")))
				xmlAddAttr( m, _T("to"), dbv.ptszVal );
			else {
				id = SerialNext();
				xmlAddAttr( m, _T("to"), szClientJid ); xmlAddAttrID( m, id );
			}
			m_ThreadInfo->send( m );

			JForkThread( &CJabberProto::SendMessageAckThread, hContact );

			nSentMsgId = 1;
		}
		else {
			id = SerialNext();
			xmlAddAttr( m, _T("to"), szClientJid ); xmlAddAttrID( m, id );

			// message receipts XEP priority
			if ( jcb & JABBER_CAPS_MESSAGE_RECEIPTS )
				m << XCHILDNS( _T("request"), _T(JABBER_FEAT_MESSAGE_RECEIPTS));
			else if ( jcb & JABBER_CAPS_MESSAGE_EVENTS ) {
				HXML x = m << XCHILDNS( _T("x"), _T(JABBER_FEAT_MESSAGE_EVENTS));
				x << XCHILD( _T("delivered")); x << XCHILD( _T("offline"));
			}
			else
				id = 1;

			m_ThreadInfo->send( m );
			nSentMsgId = id;
	}	}

	JFreeVariant( &dbv );
	return nSentMsgId;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendUrl

int __cdecl CJabberProto::SendUrl( HANDLE hContact, int flags, const char* url )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberSetApparentMode - sets the visibility status

int __cdecl CJabberProto::SetApparentMode( HANDLE hContact, int mode )
{
	if ( mode != 0 && mode != ID_STATUS_ONLINE && mode != ID_STATUS_OFFLINE )
		return 1;
	
	int oldMode = JGetWord( hContact, "ApparentMode", 0 );
	if ( mode == oldMode )
		return 1;

	JSetWord( hContact, "ApparentMode", ( WORD )mode );
	if ( !m_bJabberOnline )
		return 0;

	DBVARIANT dbv;
	if ( !JGetStringT( hContact, "jid", &dbv )) {
		TCHAR* jid = dbv.ptszVal;
		switch ( mode ) {
		case ID_STATUS_ONLINE:
			if ( m_iStatus == ID_STATUS_INVISIBLE || oldMode == ID_STATUS_OFFLINE )
				m_ThreadInfo->send( XmlNode( _T("presence")) << XATTR( _T("to"), jid ));
			break;
		case ID_STATUS_OFFLINE:
			if ( m_iStatus != ID_STATUS_INVISIBLE || oldMode == ID_STATUS_ONLINE )
				SendPresenceTo( ID_STATUS_INVISIBLE, jid, NULL );
			break;
		case 0:
			if ( oldMode == ID_STATUS_ONLINE && m_iStatus == ID_STATUS_INVISIBLE )
				SendPresenceTo( ID_STATUS_INVISIBLE, jid, NULL );
			else if ( oldMode == ID_STATUS_OFFLINE && m_iStatus != ID_STATUS_INVISIBLE )
				SendPresenceTo( m_iStatus, jid, NULL );
			break;
		}
		JFreeVariant( &dbv );
	}

	// TODO: update the zebra list ( jabber:iq:privacy )

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberSetStatus - sets the protocol status

int __cdecl CJabberProto::SetStatus( int iNewStatus )
{
	Log( "PS_SETSTATUS( %d )", iNewStatus );
	m_iDesiredStatus = iNewStatus;

 	if ( iNewStatus == ID_STATUS_OFFLINE ) {
		if ( m_ThreadInfo ) {
			m_ThreadInfo->send( "</stream:stream>" );
			m_ThreadInfo->close();
			m_ThreadInfo = NULL;
			if ( m_bJabberConnected ) {
				m_bJabberConnected = m_bJabberOnline = FALSE;
				RebuildInfoFrame();
			}
		}

		int oldStatus = m_iStatus;
		m_iStatus = m_iDesiredStatus = ID_STATUS_OFFLINE;
		JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, m_iStatus );
	}
	else if ( !m_bJabberConnected && !( m_iStatus >= ID_STATUS_CONNECTING && m_iStatus < ID_STATUS_CONNECTING + MAX_CONNECT_RETRIES )) {
		ThreadData* thread = new ThreadData( this, JABBER_SESSION_NORMAL );
		int oldStatus = m_iStatus;
		m_iStatus = ID_STATUS_CONNECTING;
		JSendBroadcast( NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, ( HANDLE ) oldStatus, m_iStatus );
		thread->hThread = JForkThreadEx(( JThreadFunc )&CJabberProto::ServerThread, thread );

		RebuildInfoFrame();
	}
	else if ( m_bJabberOnline )
		SetServerStatus( iNewStatus );

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberGetAwayMsg - returns a contact's away message

void __cdecl CJabberProto::GetAwayMsgThread( void* hContact )
{
	DBVARIANT dbv;
	JABBER_LIST_ITEM *item;
	JABBER_RESOURCE_STATUS *r;
	int i, len, msgCount;

	if ( !JGetStringT( hContact, "jid", &dbv )) {
		if (( item = ListGetItemPtr( LIST_ROSTER, dbv.ptszVal )) != NULL ) {
			JFreeVariant( &dbv );
			if ( item->resourceCount > 0 ) {
				Log( "resourceCount > 0" );
				r = item->resource;
				len = msgCount = 0;
				for ( i=0; i<item->resourceCount; i++ ) {
					if ( r[i].statusMessage ) {
						msgCount++;
						len += ( _tcslen( r[i].resourceName ) + _tcslen( r[i].statusMessage ) + 8 );
				}	}

				TCHAR* str = ( TCHAR* )alloca( sizeof( TCHAR )*( len+1 ));
				str[0] = str[len] = '\0';
				for ( i=0; i < item->resourceCount; i++ ) {
					if ( r[i].statusMessage ) {
						if ( str[0] != '\0' ) _tcscat( str, _T("\r\n" ));
						if ( msgCount > 1 ) {
							_tcscat( str, _T("( "));
							_tcscat( str, r[i].resourceName );
							_tcscat( str, _T(" ): "));
						}
						_tcscat( str, r[i].statusMessage );
				}	}

				char* msg = mir_t2a(str);
				char* msg2 = JabberUnixToDos(msg);
				JSendBroadcast( hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, ( HANDLE ) 1, ( LPARAM )msg2 );
				mir_free(msg);
				mir_free(msg2);
				return;
			}

			if ( item->itemResource.statusMessage != NULL ) {
				char* msg = mir_t2a(item->itemResource.statusMessage);
				JSendBroadcast( hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, ( HANDLE ) 1, ( LPARAM )msg );
				mir_free(msg);
				return;
			}
		}
		else JFreeVariant( &dbv );
	}

	JSendBroadcast( hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, ( HANDLE ) 1, ( LPARAM )"" );
}

int __cdecl CJabberProto::GetAwayMsg( HANDLE hContact )
{
	Log( "GetAwayMsg called, hContact=%08X", hContact );

	JForkThread( &CJabberProto::GetAwayMsgThread, hContact );
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSR_AWAYMSG

int __cdecl CJabberProto::RecvAwayMsg( HANDLE hContact, int statusMode, PROTORECVEVENT* evt )
{	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AWAYMSG

int __cdecl CJabberProto::SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg )
{	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// JabberSetAwayMsg - sets the away status message

int __cdecl CJabberProto::SetAwayMsg( int status, const char* msg )
{
	Log( "SetAwayMsg called, wParam=%d lParam=%s", status, msg );

	EnterCriticalSection( &m_csModeMsgMutex );

	TCHAR **szMsg;

	switch ( status ) {
	case ID_STATUS_ONLINE:
		szMsg = &m_modeMsgs.szOnline;
		break;
	case ID_STATUS_AWAY:
	case ID_STATUS_ONTHEPHONE:
	case ID_STATUS_OUTTOLUNCH:
		szMsg = &m_modeMsgs.szAway;
		status = ID_STATUS_AWAY;
		break;
	case ID_STATUS_NA:
		szMsg = &m_modeMsgs.szNa;
		break;
	case ID_STATUS_DND:
	case ID_STATUS_OCCUPIED:
		szMsg = &m_modeMsgs.szDnd;
		status = ID_STATUS_DND;
		break;
	case ID_STATUS_FREECHAT:
		szMsg = &m_modeMsgs.szFreechat;
		break;
	default:
		LeaveCriticalSection( &m_csModeMsgMutex );
		return 1;
	}

	TCHAR* newModeMsg = mir_a2t( msg );

	if (( *szMsg == NULL && newModeMsg == NULL ) ||
		( *szMsg != NULL && newModeMsg != NULL && !lstrcmp( *szMsg, newModeMsg )) ) {
		// Message is the same, no update needed
		mir_free( newModeMsg );
		LeaveCriticalSection( &m_csModeMsgMutex );
	}
	else {
		// Update with the new mode message
		if ( *szMsg != NULL )
			mir_free( *szMsg );
		*szMsg = newModeMsg;
		// Send a presence update if needed
		LeaveCriticalSection( &m_csModeMsgMutex );
		if ( status == m_iStatus ) {
			SendPresence( m_iStatus, true );
	}	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberUserIsTyping - sends a UTN notification

int __cdecl CJabberProto::UserIsTyping( HANDLE hContact, int type )
{
	if ( !m_bJabberOnline ) return 0;

	DBVARIANT dbv;
	if ( JGetStringT( hContact, "jid", &dbv )) return 0;

	JABBER_LIST_ITEM *item;
	if (( item = ListGetItemPtr( LIST_ROSTER, dbv.ptszVal )) != NULL ) {
		TCHAR szClientJid[ 256 ];
		GetClientJID( dbv.ptszVal, szClientJid, SIZEOF( szClientJid ));

		JabberCapsBits jcb = GetResourceCapabilites( szClientJid, TRUE );

		if ( jcb & JABBER_RESOURCE_CAPS_ERROR )
			jcb = JABBER_RESOURCE_CAPS_NONE;

		XmlNode m( _T("message")); xmlAddAttr( m, _T("to"), szClientJid );

		if ( jcb & JABBER_CAPS_CHATSTATES ) {
			m << XATTR( _T("type"), _T("chat")) << XATTRID( SerialNext());
			switch ( type ){
			case PROTOTYPE_SELFTYPING_OFF:
				m << XCHILDNS( _T("paused"), _T(JABBER_FEAT_CHATSTATES));
				m_ThreadInfo->send( m );
				break;
			case PROTOTYPE_SELFTYPING_ON:
				m << XCHILDNS( _T("composing"), _T(JABBER_FEAT_CHATSTATES));
				m_ThreadInfo->send( m );
				break;
			}
		}
		else if ( jcb & JABBER_CAPS_MESSAGE_EVENTS ) {
			HXML x = m << XCHILDNS( _T("x"), _T(JABBER_FEAT_MESSAGE_EVENTS));
			if ( item->messageEventIdStr != NULL )
				x << XCHILD( _T("id"), item->messageEventIdStr );

			switch ( type ){
			case PROTOTYPE_SELFTYPING_OFF:
				m_ThreadInfo->send( m );
				break;
			case PROTOTYPE_SELFTYPING_ON:
				x << XCHILD( _T("composing"));
				m_ThreadInfo->send( m );
				break;
	}	}	}

	JFreeVariant( &dbv );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Notify dialogs

void CJabberProto::WindowSubscribe(HWND hwnd)
{
	WindowList_Add(m_windowList, hwnd, NULL);
}

void CJabberProto::WindowUnsubscribe(HWND hwnd)
{
	WindowList_Remove(m_windowList, hwnd);
}

void CJabberProto::WindowNotify(UINT msg, bool async)
{
	if (async)
		WindowList_BroadcastAsync(m_windowList, msg, 0, 0);
	else
		WindowList_Broadcast(m_windowList, msg, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////
// InfoFrame events

void CJabberProto::InfoFrame_OnSetup(CJabberInfoFrame_Event *evt)
{
	OPENOPTIONSDIALOG ood = {0};
	ood.cbSize = sizeof(ood);
	ood.pszGroup = "Network";
	ood.pszPage = mir_t2a(m_tszUserName);
	ood.pszTab = "Account";
	CallService(MS_OPT_OPENOPTIONS, 0, (LPARAM)&ood);

	mir_free((void *)ood.pszPage);
}

void CJabberProto::InfoFrame_OnTransport(CJabberInfoFrame_Event *evt)
{
	if (evt->m_event == CJabberInfoFrame_Event::CLICK)
	{
		HANDLE hContact = (HANDLE)evt->m_pUserData;
		POINT pt;

		HMENU hContactMenu = (HMENU)CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM)hContact, 0);
		GetCursorPos(&pt);
		int res = TrackPopupMenu(hContactMenu, TPM_RETURNCMD, pt.x, pt.y, 0, (HWND)CallService(MS_CLUI_GETHWND, 0, 0), NULL);
		CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(res, MPCF_CONTACTMENU), (LPARAM)hContact);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnEvent - maintain protocol events

int __cdecl CJabberProto::OnEvent( PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam )
{
	switch( eventType ) {
	case EV_PROTO_ONLOAD:    return OnModulesLoadedEx( 0, 0 );
	case EV_PROTO_ONEXIT:    return OnPreShutdown( 0, 0 );
	case EV_PROTO_ONOPTIONS: return OnOptionsInit( wParam, lParam );
	case EV_PROTO_ONRENAME:
		{	
			CLISTMENUITEM clmi = { 0 };
			clmi.cbSize = sizeof( CLISTMENUITEM );
			clmi.flags = CMIM_NAME | CMIF_TCHAR;
			clmi.ptszName = m_tszUserName;
			JCallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )m_hMenuRoot, ( LPARAM )&clmi );
	}	}	
	return 1;
}
