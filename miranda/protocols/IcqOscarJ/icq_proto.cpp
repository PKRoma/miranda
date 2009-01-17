// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001-2002 Jon Keating, Richard Hughes
// Copyright © 2002-2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004-2009 Joe Kucera, George Hazan
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// -----------------------------------------------------------------------------
//
// File name      : $URL$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Protocol Interface Implementation
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"

#include "m_cluiframes.h"
#include "m_icolib.h"
#include "m_updater.h"

#include <ctype.h>

extern PLUGININFOEX pluginInfo;

#pragma warning(disable:4355)

static int CompareConns(const directconnect* p1, const directconnect* p2)
{
	if (p1 < p2)
		return -1;

	return (p1 == p2) ? 0 : 1;
}

static int CompareCookies( const icq_cookie_info* p1, const icq_cookie_info* p2 )
{
	if ( p1->dwCookie < p2->dwCookie )
		return -1;

	return ( p1->dwCookie == p2->dwCookie ) ? 0 : 1;
}

static int CompareFT( const filetransfer* p1, const filetransfer* p2 )
{
	if ( p1->dwCookie < p2->dwCookie )
		return -1;

	return ( p1->dwCookie == p2->dwCookie ) ? 0 : 1;
}

static int CompareContactsCache(const icq_contacts_cache *p1, const icq_contacts_cache *p2)
{
  if (p1->dwUin < p2->dwUin)
    return -1;

  if (p1->dwUin > p2->dwUin)
    return 1;

  return stricmpnull(p1->szUid, p2->szUid);
}


CIcqProto::CIcqProto( const char* aProtoName, const TCHAR* aUserName ) :
	cookies(10, CompareCookies),
	directConns(10, CompareConns),
	expectedFileRecvs(10, CompareFT),
  contactsCache(10, CompareContactsCache),
	cheekySearchId( -1 ),
	m_pendingAvatarsStart( 1 )
{
	m_iStatus = ID_STATUS_OFFLINE;
	m_tszUserName = mir_tstrdup( aUserName );
	m_szModuleName = mir_strdup( aProtoName );
	m_szProtoName = mir_strdup( aProtoName );
	_strlwr( m_szProtoName );
	m_szProtoName[0] = toupper( m_szProtoName[0] );
	NetLog_Server( "Setting protocol/module name to '%s/%s'", m_szProtoName, m_szModuleName );

	InitializeCriticalSection(&oftMutex);

	// Initialize direct connections
	InitializeCriticalSection(&directConnListMutex);
	InitializeCriticalSection(&expectedFileRecvMutex);

	// Initialize server lists
	InitializeCriticalSection(&servlistMutex);
	InitializeCriticalSection(&servlistQueueMutex);
	HookProtoEvent(ME_DB_CONTACT_SETTINGCHANGED, &CIcqProto::ServListDbSettingChanged);
	HookProtoEvent(ME_DB_CONTACT_DELETED, &CIcqProto::ServListDbContactDeleted);
	HookProtoEvent(ME_CLIST_GROUPCHANGE, &CIcqProto::ServListCListGroupChange);

	// Initialize status message struct
	ZeroMemory(&m_modeMsgs, sizeof(icq_mode_messages));
	InitializeCriticalSection(&m_modeMsgsMutex);
	InitializeCriticalSection(&connectionHandleMutex);
	InitializeCriticalSection(&localSeqMutex);

	hsmsgrequest = CreateProtoEvent(ME_ICQ_STATUSMSGREQ);
	hxstatuschanged = CreateProtoEvent(ME_ICQ_CUSTOMSTATUS_CHANGED);
	hxstatusiconchanged = CreateProtoEvent(ME_ICQ_CUSTOMSTATUS_EXTRAICON_CHANGED);

	// Initialize cookies
	InitializeCriticalSection(&cookieMutex);
	wCookieSeq = 2;

	// Initialize rates
	InitializeCriticalSection(&ratesMutex);
	InitializeCriticalSection(&ratesListsMutex);

	// Initialize temporary DB settings
	CreateResidentSetting("Status"); // NOTE: XStatus cannot be temporary
	CreateResidentSetting("TemporaryVisible");
	CreateResidentSetting("TickTS");
	CreateResidentSetting("IdleTS");
	CreateResidentSetting("LogonTS");
	CreateResidentSetting("DCStatus");
  CreateResidentSetting(DBSETTING_STATUS_NOTE_TIME);
  CreateResidentSetting(DBSETTING_STATUS_MOOD);

	// Setup services
	CreateProtoService(PS_GETNAME, &CIcqProto::GetName);
	CreateProtoService(PS_GETSTATUS, &CIcqProto::GetStatus);
	CreateProtoService(PS_CREATEACCMGRUI, &CIcqProto::OnCreateAccMgrUI );
	CreateProtoService(MS_ICQ_SENDSMS, &CIcqProto::SendSms);
	CreateProtoService(PS_SET_NICKNAME, &CIcqProto::SetNickName);

  CreateProtoService(PS_GETINFOSETTING, &CIcqProto::GetInfoSetting);

	CreateProtoService(PSS_ADDED, &CIcqProto::SendYouWereAdded);
	// Session password API
	CreateProtoService(PS_ICQ_SETPASSWORD, &CIcqProto::SetPassword);
	// ChangeInfo API
	CreateProtoService(PS_CHANGEINFOEX, &CIcqProto::ChangeInfoEx);
	// Avatar API
	CreateProtoService(PS_GETAVATARINFO, &CIcqProto::GetAvatarInfo);
	CreateProtoService(PS_GETAVATARCAPS, &CIcqProto::GetAvatarCaps);
	CreateProtoService(PS_GETMYAVATAR, &CIcqProto::GetMyAvatar);
	CreateProtoService(PS_SETMYAVATAR, &CIcqProto::SetMyAvatar);
	// Custom Status API
	CreateProtoService(PS_ICQ_SETCUSTOMSTATUS, &CIcqProto::SetXStatus);
	CreateProtoService(PS_ICQ_GETCUSTOMSTATUS, &CIcqProto::GetXStatus);
	CreateProtoService(PS_ICQ_SETCUSTOMSTATUSEX, &CIcqProto::SetXStatusEx);
	CreateProtoService(PS_ICQ_GETCUSTOMSTATUSEX, &CIcqProto::GetXStatusEx);
	CreateProtoService(PS_ICQ_GETCUSTOMSTATUSICON, &CIcqProto::GetXStatusIcon);
	CreateProtoService(PS_ICQ_REQUESTCUSTOMSTATUS, &CIcqProto::RequestXStatusDetails);
	CreateProtoService(PS_ICQ_GETADVANCEDSTATUSICON, &CIcqProto::RequestAdvStatusIconIdx);

	CreateProtoService(MS_ICQ_ADDSERVCONTACT, &CIcqProto::AddServerContact);

	CreateProtoService(MS_REQ_AUTH, &CIcqProto::RequestAuthorization);
	CreateProtoService(MS_GRANT_AUTH, &CIcqProto::GrantAuthorization);
	CreateProtoService(MS_REVOKE_AUTH, &CIcqProto::RevokeAuthorization);

	CreateProtoService(MS_XSTATUS_SHOWDETAILS, &CIcqProto::ShowXStatusDetails);

	HookProtoEvent(ME_SKIN2_ICONSCHANGED, &CIcqProto::OnReloadIcons);

  { // Initialize IconLib icons
    TCHAR lib[MAX_PATH];
    char *szAccountName = mtchar_to_utf8(m_tszUserName);
    char szSectionName[MAX_PATH];
    char szProtocolsBuf[100], szNameBuf[100], szAccountsBuf[100];

    null_snprintf(szSectionName, sizeof(szSectionName), "%s/%s/%s", 
      ICQTranslateUtfStatic(LPGEN("Protocols"), szProtocolsBuf, sizeof(szProtocolsBuf)), 
      ICQTranslateUtfStatic(ICQ_PROTOCOL_NAME, szNameBuf, sizeof(szNameBuf)),
      ICQTranslateUtfStatic(LPGEN("Accounts"), szAccountsBuf, sizeof(szAccountsBuf)));

    GetModuleFileName(hInst, lib, MAX_PATH);
    m_hIconProtocol = IconLibDefine(szAccountName, szSectionName, m_szModuleName, "main", lib, -IDI_ICQ);
    SAFE_FREE((void**)&szSectionName);
  }

	// Reset a bunch of session specific settings
	UpdateGlobalSettings();
	ResetSettingsOnLoad();

  // Initialize Contacts Cache
 	InitContactsCache();

  // Startup Auto Info-Update thread
	icq_InitInfoUpdate();

	// Init extra statuses
 	InitXStatusIcons();

	if (bStatusMenu = ServiceExists(MS_CLIST_ADDSTATUSMENUITEM))
		HookProtoEvent(ME_CLIST_PREBUILDSTATUSMENU, &CIcqProto::OnPreBuildStatusMenu);

	if (HookProtoEvent(ME_CLIST_EXTRA_LIST_REBUILD, &CIcqProto::CListMW_ExtraIconsRebuild))
  { // note if the Hook was successful (e.g. clist_nicer creates them too late)
	  HookProtoEvent(ME_CLIST_EXTRA_IMAGE_APPLY, &CIcqProto::CListMW_ExtraIconsApply);
    bXStatusExtraIconsReady = 1;
  }

  NetLog_Server("%s: Protocol instance '%s' created.", ICQ_PROTOCOL_NAME, m_szModuleName);
}


CIcqProto::~CIcqProto()
{
	if (m_bXStatusEnabled)
		m_bXStatusEnabled = 10; // block clist changing

  // Make sure all connections are closed
	CloseContactDirectConns(NULL);
	while ( true ) {
		if ( !directConns.getCount())
			break;

		Sleep(10);     /* yeah, ugly */
	}

  // Delete account items from contact menu
  for (int i = 0; i < SIZEOF(m_hContactMenuItems); i++)
	  CallService(MS_CLIST_REMOVECONTACTMENUITEM, (WPARAM)m_hContactMenuItems[i], 0);

  // Serv-list update board clean-up
	FlushServerIDs();
  /// TODO: make sure server-list handler thread is not running
  /// TODO: save state of server-list update board to DB
  servlistPendingFlushOperations();

  // NetLib clean-up
	NetLib_SafeCloseHandle(&m_hDirectNetlibUser);
	NetLib_SafeCloseHandle(&m_hServerNetlibUser);

  // Destroy hookable events
	if (hsmsgrequest)
		DestroyHookableEvent(hsmsgrequest);

	if (hxstatuschanged)
		DestroyHookableEvent(hxstatuschanged);

	if (hxstatusiconchanged)
		DestroyHookableEvent(hxstatusiconchanged);

  // Clean-up remaining protocol instance members
	cookies.destroy();

	UninitContactsCache();

	SAFE_FREE((void**)&pendingList1);
	SAFE_FREE((void**)&pendingList2);
	DeleteCriticalSection(&ratesMutex);
	DeleteCriticalSection(&ratesListsMutex);

	DeleteCriticalSection(&servlistMutex);
	DeleteCriticalSection(&servlistQueueMutex);

	DeleteCriticalSection(&m_modeMsgsMutex);
	DeleteCriticalSection(&localSeqMutex);
	DeleteCriticalSection(&connectionHandleMutex);
	DeleteCriticalSection(&oftMutex);
	DeleteCriticalSection(&directConnListMutex);
	DeleteCriticalSection(&expectedFileRecvMutex);
	DeleteCriticalSection(&cookieMutex);

	SAFE_FREE((void**)&m_modeMsgs.szAway);
	SAFE_FREE((void**)&m_modeMsgs.szNa);
	SAFE_FREE((void**)&m_modeMsgs.szOccupied);
	SAFE_FREE((void**)&m_modeMsgs.szDnd);
	SAFE_FREE((void**)&m_modeMsgs.szFfc);

  // Remove account icons
  UninitXStatusIcons();

  IconLibRemove(&m_hIconProtocol);

  NetLog_Server("%s: Protocol instance '%s' destroyed.", ICQ_PROTOCOL_NAME, m_szModuleName);

	mir_free( m_szProtoName );
	mir_free( m_szModuleName );
	mir_free( m_tszUserName );
}

////////////////////////////////////////////////////////////////////////////////////////
// OnModulesLoadedEx - performs hook registration

static HANDLE CListAddContactMenuItem(const char *szName, const IcqIconHandle hIcon, int nPosition, const char *szOwnerModule, const char *szServiceName, int flags)
{
  CLISTMENUITEM mi = {0};
	char pszServiceName[MAX_PATH+30];

	strcpy(pszServiceName, szOwnerModule);
	strcat(pszServiceName, szServiceName);

	mi.cbSize = sizeof(mi);
	mi.position = nPosition;
	mi.flags = CMIF_ICONFROMICOLIB | flags;
	mi.icolibItem = hIcon->Handle();
	mi.pszContactOwner = (char*)szOwnerModule;
	mi.pszName = (char*)szName;
	mi.pszService = pszServiceName;

	return (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
}

int CIcqProto::OnModulesLoaded( WPARAM wParam, LPARAM lParam )
{
	NETLIBUSER nlu = {0};
	char pszP2PName[MAX_PATH+3];
	char pszGroupsName[MAX_PATH+10];
	char pszSrvGroupsName[MAX_PATH+10];
	char* modules[5] = {0,0,0,0,0};

	strcpy(pszP2PName, m_szModuleName);
	strcat(pszP2PName, "P2P");

	strcpy(pszGroupsName, m_szModuleName);
	strcat(pszGroupsName, "Groups");
	strcpy(pszSrvGroupsName, m_szModuleName);
	strcat(pszSrvGroupsName, "SrvGroups");
	modules[0] = m_szModuleName;
	modules[1] = pszP2PName;
	modules[2] = pszGroupsName;
	modules[3] = pszSrvGroupsName;
	CallService("DBEditorpp/RegisterModule",(WPARAM)modules,(LPARAM)4);

	TCHAR szBuffer[MAX_PATH + 64];
	null_snprintf(szBuffer, SIZEOF(szBuffer), TranslateT("%s server connection"), m_tszUserName);
	nlu.cbSize = sizeof(nlu);
	nlu.flags = NUF_OUTGOING | NUF_HTTPGATEWAY | NUF_TCHAR;
	nlu.ptszDescriptiveName = szBuffer;
	nlu.szSettingsModule = m_szModuleName;
	nlu.szHttpGatewayHello = "http://http.proxy.icq.com/hello";
	nlu.szHttpGatewayUserAgent = "Mozilla/4.08 [en] (WinNT; U ;Nav)";
	nlu.pfnHttpGatewayInit = icq_httpGatewayInit;
	nlu.pfnHttpGatewayBegin = icq_httpGatewayBegin;
	nlu.pfnHttpGatewayWrapSend = icq_httpGatewayWrapSend;
	nlu.pfnHttpGatewayUnwrapRecv = icq_httpGatewayUnwrapRecv;

	m_hServerNetlibUser = (HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

	null_snprintf(szBuffer, SIZEOF(szBuffer), TranslateT("%s client-to-client connections"), m_tszUserName);
	nlu.flags = NUF_OUTGOING | NUF_INCOMING | NUF_TCHAR;
	nlu.ptszDescriptiveName = szBuffer;
	nlu.szSettingsModule = pszP2PName;
	nlu.minIncomingPorts = 1;
	m_hDirectNetlibUser = (HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

	HookProtoEvent(ME_OPT_INITIALISE, &CIcqProto::OnOptionsInit);
	HookProtoEvent(ME_USERINFO_INITIALISE, &CIcqProto::OnUserInfoInit);
	HookProtoEvent(ME_CLIST_PREBUILDCONTACTMENU, &CIcqProto::OnPreBuildContactMenu);
	HookProtoEvent(ME_IDLE_CHANGED, &CIcqProto::OnIdleChanged);

  // Register custom database events
	DBEVENTTYPEDESCR eventType = {0};
	eventType.cbSize = DBEVENTTYPEDESCR_SIZE;
	eventType.eventType = ICQEVENTTYPE_MISSEDMESSAGE;
	eventType.module = m_szModuleName;
	eventType.descr = "Missed message notifications";
	eventType.textService = ICQ_DB_GETEVENTTEXT_MISSEDMESSAGE;
	eventType.flags = DETF_HISTORY | DETF_MSGWINDOW;
	// for now keep default "message" icon
	CallService(MS_DB_EVENT_REGISTERTYPE, 0, (LPARAM)&eventType);

	InitAvatars();

	// Init extra optional modules
	InitPopUps();
	InitXStatusItems(FALSE);

  // Add account items to contact menu
	m_hContactMenuItems[ICMI_AUTH_REQUEST] = CListAddContactMenuItem(LPGEN("Request authorization"), hStaticIcons[ISI_AUTH_REQUEST], 1000030000, m_szModuleName, MS_REQ_AUTH, 0);
  m_hContactMenuItems[ICMI_AUTH_GRANT] = CListAddContactMenuItem(LPGEN("Grant authorization"), hStaticIcons[ISI_AUTH_GRANT], 1000029999, m_szModuleName, MS_GRANT_AUTH, 0);
  m_hContactMenuItems[ICMI_AUTH_REVOKE] = CListAddContactMenuItem(LPGEN("Revoke authorization"), hStaticIcons[ISI_AUTH_REVOKE], 1000029998, m_szModuleName, MS_REVOKE_AUTH, 0);
  m_hContactMenuItems[ICMI_ADD_TO_SERVLIST] = CListAddContactMenuItem(LPGEN("Add to server list"), hStaticIcons[ISI_ADD_TO_SERVLIST], -2049999999, m_szModuleName, MS_ICQ_ADDSERVCONTACT, 0);

  m_hContactMenuItems[ICMI_XSTATUS_DETAILS] = CListAddContactMenuItem(LPGEN("Show custom status details"), NULL, -2000004999, m_szModuleName, MS_XSTATUS_SHOWDETAILS, CMIF_NOTOFFLINE);

	// TODO: add beta builds support to devel builds :)
	CallService(MS_UPDATE_REGISTERFL, 1683, (WPARAM)&pluginInfo);
	return 0;
}

int CIcqProto::OnPreShutdown(WPARAM wParam,LPARAM lParam)
{
	// all threads should be terminated here
	if (hServerConn) {
		icq_sendCloseConnection();
		icq_serverDisconnect(TRUE);
	}

	icq_InfoUpdateCleanup();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// PS_AddToList - adds a contact to the contact list

HANDLE CIcqProto::AddToList( int flags, PROTOSEARCHRESULT* psr )
{
	if (psr)
	{	// this should be only from search
		ICQSEARCHRESULT *isr = (ICQSEARCHRESULT*)psr;
		if (isr->hdr.cbSize == sizeof(ICQSEARCHRESULT))
		{
			if (!isr->uin)
				return AddToListByUID(isr->hdr.nick, flags);
			else
				return AddToListByUIN(isr->uin, flags);
		}
	}

	return 0; // Failure
}

HANDLE __cdecl CIcqProto::AddToListByEvent( int flags, int iContact, HANDLE hDbEvent )
{
	DWORD uin = 0;
	uid_str uid = {0};

	DBEVENTINFO dbei = {0};
	dbei.cbSize = sizeof(dbei);
	if ((dbei.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hDbEvent, 0)) == -1)
		return 0;

	dbei.pBlob = (PBYTE)_alloca(dbei.cbBlob + 1);
	dbei.pBlob[dbei.cbBlob] = '\0';

	if (CallService(MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei))
		return 0; // failed to get event

	if (strcmpnull(dbei.szModule, m_szModuleName))
		return 0; // this event is not ours

	if (dbei.eventType == EVENTTYPE_CONTACTS)
	{
		int i;
		char* pbOffset, *pbEnd;

		for (i = 0, pbOffset = (char*)dbei.pBlob, pbEnd = pbOffset + dbei.cbBlob; i <= iContact; i++)
		{
			pbOffset += strlennull(pbOffset) + 1;  // Nick
			if (pbOffset >= pbEnd) break;
			if (i == iContact)
			{ // we found the contact, get uid
				if (IsStringUIN((char*)pbOffset))
					uin = atoi((char*)pbOffset);
				else
				{
					uin = 0;
					strcpy(uid, (char*)pbOffset);
				}
			}
			pbOffset += strlennull(pbOffset) + 1;  // Uin
			if (pbOffset >= pbEnd) break;
		}
	}
	else if (dbei.eventType != EVENTTYPE_AUTHREQUEST && dbei.eventType != EVENTTYPE_ADDED)
	{
		return 0;
	}
	else // auth req or added event
	{
		HANDLE hContact = ((HANDLE*)dbei.pBlob)[1]; // this sucks - awaiting new auth system
		if (getContactUid(hContact, &uin, &uid))
			return 0;
	}

	if (uin != 0)
		return AddToListByUIN(uin, flags); // Success

	// add aim contact
	if (strlennull(uid))
		return AddToListByUID(uid, flags); // Success

	return NULL; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// PS_AuthAllow - processes the successful authorization

int CIcqProto::Authorize( HANDLE hDbEvent )
{
	if (icqOnline() && hDbEvent)
	{
		HANDLE hContact = HContactFromAuthEvent( hDbEvent );
		if (hContact == INVALID_HANDLE_VALUE)
			return 1;

		DWORD uin;
		uid_str uid;
		if (getContactUid(hContact, &uin, &uid))
			return 1;

		icq_sendAuthResponseServ(uin, uid, 1, "");

		return 0; // Success
	}

	return 1; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// PS_AuthDeny - handles the unsuccessful authorization

int CIcqProto::AuthDeny( HANDLE hDbEvent, const char* szReason )
{
	if (icqOnline() && hDbEvent)
	{
		HANDLE hContact = HContactFromAuthEvent(hDbEvent);
		if (hContact == INVALID_HANDLE_VALUE)
			return 1;

		DWORD uin;
		uid_str uid;
		if (getContactUid(hContact, &uin, &uid))
			return 1;

		icq_sendAuthResponseServ(uin, uid, 0, szReason);

		return 0; // Success
	}

	return 1; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// PSR_AUTH

int __cdecl CIcqProto::AuthRecv( HANDLE hContact, PROTORECVEVENT* pre )
{
	setContactHidden( hContact, 0 );
	ICQAddRecvEvent( NULL, EVENTTYPE_AUTHREQUEST, pre, pre->lParam, (PBYTE)pre->szMessage, 0 );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AUTHREQUEST

int __cdecl CIcqProto::AuthRequest( HANDLE hContact, const char* szMessage )
{
	if ( !icqOnline())
		return 1;

	if (hContact)
	{
		DWORD dwUin;
		uid_str szUid;
		if (getContactUid(hContact, &dwUin, &szUid))
			return 1; // Invalid contact

		if (dwUin && szMessage)
		{
			char *utf = ansi_to_utf8(szMessage); // Miranda is ANSI only here
			icq_sendAuthReqServ(dwUin, szUid, utf);
			SAFE_FREE((void**)&utf);
			return 0; // Success
		}
	}

	return 1; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// ChangeInfo

HANDLE __cdecl CIcqProto::ChangeInfo( int iInfoType, void* pInfoData )
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// PS_FileAllow - starts a file transfer

int __cdecl CIcqProto::FileAllow( HANDLE hContact, HANDLE hTransfer, const char* szPath )
{
	DWORD dwUin;
	uid_str szUid;

	if ( getContactUid(hContact, &dwUin, &szUid))
		return 0; // Invalid contact

	if (icqOnline() && hContact && szPath && hTransfer)
	{ // approve old fashioned file transfer
		basic_filetransfer* ft = (basic_filetransfer *)hTransfer;

    if (!IsValidFileTransfer(ft))
      return 0; // Invalid transfer

		if (dwUin && ft->ft_magic == FT_MAGIC_ICQ)
		{
			filetransfer* ft = (filetransfer *)hTransfer;

			ft->szSavePath = null_strdup(szPath);

			EnterCriticalSection(&expectedFileRecvMutex);
			expectedFileRecvs.insert( ft );
			LeaveCriticalSection(&expectedFileRecvMutex);

			// Was request received thru DC and have we a open DC, send through that
			if (ft->bDC && IsDirectConnectionOpen(hContact, DIRECTCONN_STANDARD, 0))
				icq_sendFileAcceptDirect(hContact, ft);
			else
				icq_sendFileAcceptServ(dwUin, ft, 0);

			return (int)hTransfer; // Success
		}
		else if (ft->ft_magic == FT_MAGIC_OSCAR)
		{ // approve oscar file transfer
			return oftFileAllow(hContact, hTransfer, szPath);
		}
	}

	return 0; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// PS_FileCancel - cancels a file transfer

int __cdecl CIcqProto::FileCancel( HANDLE hContact, HANDLE hTransfer )
{
	DWORD dwUin;
	uid_str szUid;
	if ( getContactUid(hContact, &dwUin, &szUid))
		return 1; // Invalid contact

	if (hContact && hTransfer)
	{
		basic_filetransfer *ft = (basic_filetransfer *)hTransfer;

    if (!IsValidFileTransfer(ft))
      return 1; // Invalid transfer

		if (dwUin && ft->ft_magic == FT_MAGIC_ICQ)
		{ // cancel old fashioned file transfer
			filetransfer * ft = (filetransfer*)hTransfer;
			icq_CancelFileTransfer(hContact, ft);
			return 0; // Success
		}
		else if (ft->ft_magic = FT_MAGIC_OSCAR)
		{ // cancel oscar file transfer
			return oftFileCancel(hContact, hTransfer);
		}
	}

	return 1; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// PS_FileDeny - denies a file transfer

int __cdecl CIcqProto::FileDeny( HANDLE hContact, HANDLE hTransfer, const char* szReason )
{
	int nReturnValue = 1;
	DWORD dwUin;
	uid_str szUid;
	basic_filetransfer *ft = (basic_filetransfer*)hTransfer;

	if (getContactUid(hContact, &dwUin, &szUid))
		return 1; // Invalid contact

	if (icqOnline() && hTransfer && hContact)
	{
    if (!IsValidFileTransfer(hTransfer))
      return 1; // Invalid transfer

		if (dwUin && ft->ft_magic == FT_MAGIC_ICQ)
		{ // deny old fashioned file transfer
			filetransfer *ft = (filetransfer*)hTransfer;
			// Was request received thru DC and have we a open DC, send through that
			if (ft->bDC && IsDirectConnectionOpen(hContact, DIRECTCONN_STANDARD, 0))
				icq_sendFileDenyDirect(hContact, ft, szReason);
			else
				icq_sendFileDenyServ(dwUin, ft, szReason, 0);

			nReturnValue = 0; // Success
		}
		else if (ft->ft_magic == FT_MAGIC_OSCAR)
		{ // deny oscar file transfer
			return oftFileDeny(hContact, hTransfer, szReason);
		}
	}
	// Release possible orphan structure
	SafeReleaseFileTransfer((void**)&ft);

	return nReturnValue;
}

////////////////////////////////////////////////////////////////////////////////////////
// PS_FileResume - processes file renaming etc

int __cdecl CIcqProto::FileResume( HANDLE hTransfer, int* action, const char** szFilename )
{
	if (icqOnline() && hTransfer)
	{
		basic_filetransfer *ft = (basic_filetransfer *)hTransfer;

    if (!IsValidFileTransfer(ft))
      return 1; // Invalid transfer

		if (ft->ft_magic == FT_MAGIC_ICQ)
		{
			icq_sendFileResume((filetransfer *)hTransfer, *action, *szFilename);
		}
		else if (ft->ft_magic == FT_MAGIC_OSCAR)
		{
			oftFileResume((oscar_filetransfer *)hTransfer, *action, *szFilename);
		}
		else
			return 1; // Failure

		return 0; // Success
	}

	return 1; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// GetCaps - return protocol capabilities bits

DWORD __cdecl CIcqProto::GetCaps( int type, HANDLE hContact )
{
	int nReturn = 0;

	switch ( type ) {

	case PFLAGNUM_1:
		nReturn = PF1_IM | PF1_URL | PF1_AUTHREQ | PF1_BASICSEARCH | PF1_ADDSEARCHRES |
			PF1_VISLIST | PF1_INVISLIST | PF1_MODEMSG | PF1_FILE | PF1_EXTSEARCH |
			PF1_EXTSEARCHUI | PF1_SEARCHBYEMAIL | PF1_SEARCHBYNAME |
			PF1_ADDED | PF1_CONTACT;
		if (!m_bAimEnabled)
			nReturn |= PF1_NUMERICUSERID;
		if (m_bSsiEnabled && getSettingByte(NULL, "ServerAddRemove", DEFAULT_SS_ADDSERVER))
			nReturn |= PF1_SERVERCLIST;
		break;

	case PFLAGNUM_2:
		nReturn = PF2_ONLINE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_LIGHTDND | PF2_HEAVYDND |
			PF2_FREECHAT | PF2_INVISIBLE;
		if (m_bAimEnabled)
			nReturn |= PF2_ONTHEPHONE;
		break;

	case PFLAGNUM_3:
		nReturn = PF2_ONLINE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_LIGHTDND | PF2_HEAVYDND |
			PF2_FREECHAT | PF2_INVISIBLE;
		break;

	case PFLAGNUM_4:
		nReturn = PF4_SUPPORTIDLE | PF4_IMSENDUTF | PF4_INFOSETTINGSVC;
		if (m_bAvatarsEnabled)
			nReturn |= PF4_AVATARS;
#ifdef DBG_CAPMTN
		nReturn |= PF4_SUPPORTTYPING;
#endif
		break;

	case PFLAGNUM_5:
		if (m_bAimEnabled)
			nReturn |= PF2_ONTHEPHONE;
		break;

	case PFLAG_UNIQUEIDTEXT:
		nReturn = (int)ICQTranslate("User ID");
		break;

	case PFLAG_UNIQUEIDSETTING:
		nReturn = (int)UNIQUEIDSETTING;
		break;

	case PFLAG_MAXCONTACTSPERPACKET:
		if ( hContact )
		{ // determine per contact
			BYTE bClientId = getSettingByte(hContact, "ClientID", CLID_GENERIC);

			if (bClientId == CLID_MIRANDA)
			{
				if (CheckContactCapabilities(hContact, CAPF_CONTACTS) && getContactStatus(hContact) != ID_STATUS_OFFLINE)
					nReturn = 0x100; // limited only by packet size
				else
					nReturn = MAX_CONTACTSSEND;
			}
			else if (bClientId == CLID_ICQ6)
			{
				if (CheckContactCapabilities(hContact, CAPF_CONTACTS))
					nReturn = 1; // crapy ICQ6 cannot handle multiple contacts in the transfer
				else
					nReturn = 0; // this version does not support contacts transfer at all
			}
			else
				nReturn = MAX_CONTACTSSEND;
		}
		else // return generic limit
			nReturn = MAX_CONTACTSSEND;
		break;

	case PFLAG_MAXLENOFMESSAGE:
		nReturn = MAX_MESSAGESNACSIZE-102;
	}

	return nReturn;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetIcon - loads an icon for the contact list

HICON __cdecl CIcqProto::GetIcon( int iconIndex )
{
	IcqIconHandle hIcon = NULL;

	switch (iconIndex & 0xFFFF) {
	case PLI_PROTOCOL:
		hIcon = m_hIconProtocol;
		break;

	default:
		return 0; // Failure
	}

	if ( iconIndex & PLIF_ICOLIBHANDLE )
		return ( HICON )hIcon->Handle();
	
	HICON icon = hIcon->GetIcon();
	return (iconIndex & PLIF_ICOLIB) ? icon : CopyIcon(icon);
}

////////////////////////////////////////////////////////////////////////////////////////
// GetInfo - retrieves a contact info

int __cdecl CIcqProto::GetInfo(HANDLE hContact, int infoType)
{
	if (icqOnline())
	{
		DWORD dwUin;
		uid_str szUid;

		if (getContactUid(hContact, &dwUin, &szUid))
			return 1; // Invalid contact

		DWORD dwCookie;
		if (dwUin)
			dwCookie = icq_sendGetInfoServ(hContact, dwUin, (infoType & SGIF_ONOPEN) != 0);
		else // TODO: this needs something better
			dwCookie = icq_sendGetAimProfileServ(hContact, szUid);

		return (dwCookie) ? 0 : 1;
	}

	return 1; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchBasic - searches the contact by UID

void CIcqProto::CheekySearchThread( void* )
{
	ICQSEARCHRESULT isr = {0};

	isr.hdr.cbSize = sizeof(isr);
	if ( cheekySearchUin )
	{
		isr.hdr.nick = "";
		isr.uid = NULL;
	}
	else
	{
		isr.hdr.nick = cheekySearchUid;
		isr.uid = cheekySearchUid;
	}
	isr.hdr.firstName = "";
	isr.hdr.lastName = "";
	isr.hdr.email = "";
	isr.uin = cheekySearchUin;

	BroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE)cheekySearchId, (LPARAM)&isr);
	BroadcastAck(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE)cheekySearchId, 0);
	cheekySearchId = -1;
}

HANDLE __cdecl CIcqProto::SearchBasic( const char* pszSearch )
{
	if (strlennull(pszSearch) == 0)
		return 0;

	char pszUIN[255];
	int nHandle = 0;
	unsigned int i, j;

	if (!m_bAimEnabled)
	{
		for (i=j=0; (i<strlennull(pszSearch)) && (j<255); i++)
		{ // we take only numbers
			if ((pszSearch[i]>=0x30) && (pszSearch[i]<=0x39))
			{
				pszUIN[j] = pszSearch[i];
				j++;
			}
		}
	}
	else
	{
		for (i=j=0; (i<strlennull(pszSearch)) && (j<255); i++)
		{ // we remove spaces and slashes
			if ((pszSearch[i]!=0x20) && (pszSearch[i]!='-'))
			{
				pszUIN[j] = pszSearch[i];
				j++;
			}
		}
	}
	pszUIN[j] = 0;

	if (strlennull(pszUIN))
	{
		DWORD dwUin;
		if (IsStringUIN(pszUIN))
			dwUin = atoi(pszUIN);
		else
			dwUin = 0;

		// Cheeky instant UIN search
		if (!dwUin || GetKeyState(VK_CONTROL)&0x8000)
		{
			cheekySearchId = GenerateCookie(0);
			cheekySearchUin = dwUin;
			cheekySearchUid = null_strdup(pszUIN);
			ForkThread(&CIcqProto::CheekySearchThread, 0); // The caller needs to get this return value before the results
			nHandle = cheekySearchId;
		}
		else if (icqOnline())
		{
			nHandle = SearchByUin(dwUin);
		}

		// Success
		return (HANDLE)nHandle;
	}

	// Failure
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchByEmail - searches the contact by its e-mail

HANDLE __cdecl CIcqProto::SearchByEmail( const char* email )
{
	if (email && icqOnline() && strlennull(email) > 0)
	{
		DWORD dwSearchId, dwSecId;

		// Success
		dwSearchId = SearchByMail(email);
		if (m_bAimEnabled)
			dwSecId = icq_searchAimByEmail(email, dwSearchId);
		else
			dwSecId = 0;
		if (dwSearchId)
			return ( HANDLE )dwSearchId;
		else
			return ( HANDLE )dwSecId;
	}

	return 0; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// PS_SearchByName - searches the contact by its first or last name, or by a nickname

HANDLE __cdecl CIcqProto::SearchByName(const char *nick, const char *firstName, const char *lastName)
{
	if (icqOnline())
	{
		if (nick || firstName || lastName)
		{
      char *nickUtf = ansi_to_utf8(nick);
      char *firstNameUtf = ansi_to_utf8(firstName);
      char *lastNameUtf = ansi_to_utf8(lastName);

			// Success
			HANDLE dwCookie = (HANDLE)SearchByNames(nickUtf, firstNameUtf, lastNameUtf, 0);

      SAFE_FREE((void**)&nickUtf);
      SAFE_FREE((void**)&firstNameUtf);
      SAFE_FREE((void**)&lastNameUtf);

      return dwCookie;
		}
	}

	return 0; // Failure
}


HWND __cdecl CIcqProto::CreateExtendedSearchUI( HWND parent )
{
	if (parent && hInst)
		return CreateDialog(hInst, MAKEINTRESOURCE(IDD_ICQADVANCEDSEARCH), parent, AdvancedSearchDlgProc);

	return NULL; // Failure
}

HWND __cdecl CIcqProto::SearchAdvanced( HWND hwndDlg )
{
	if (icqOnline() && IsWindow(hwndDlg))
	{
		int nDataLen;
		BYTE* bySearchData;

		if (bySearchData = createAdvancedSearchStructure(hwndDlg, &nDataLen))
		{
			int result = icq_sendAdvancedSearchServ(bySearchData, nDataLen);
			SAFE_FREE((void**)&bySearchData);
			return ( HWND )result; // Success
		}
	}

	return NULL; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvContacts

int __cdecl CIcqProto::RecvContacts( HANDLE hContact, PROTORECVEVENT* pre )
{
	ICQSEARCHRESULT** isrList = (ICQSEARCHRESULT**)pre->szMessage;
	int i;
	char szUin[UINMAXLEN];
	DWORD cbBlob = 0;
	PBYTE pBlob,pCurBlob;
	DWORD flags = 0;

	for (i = 0; i < pre->lParam; i++)
	{
		cbBlob += strlennull(isrList[i]->hdr.nick) + 2; // both trailing zeros
		if (isrList[i]->uin)
			cbBlob += getUINLen(isrList[i]->uin);
		else
			cbBlob += strlennull(isrList[i]->uid);
	}
	pBlob = (PBYTE)_alloca(cbBlob);
	for (i = 0, pCurBlob = pBlob; i < pre->lParam; i++)
	{
		strcpy((char*)pCurBlob, isrList[i]->hdr.nick);
		pCurBlob += strlennull((char*)pCurBlob) + 1;
		if (isrList[i]->uin)
		{
			_itoa(isrList[i]->uin, szUin, 10);
			strcpy((char*)pCurBlob, szUin);
		}
		else // aim contact
			strcpy((char*)pCurBlob, isrList[i]->uid);
		pCurBlob += strlennull((char*)pCurBlob) + 1;
	}
	if (pre->flags & PREF_UTF)
		flags |= DBEF_UTF;

	ICQAddRecvEvent(hContact, EVENTTYPE_CONTACTS, pre, cbBlob, pBlob, flags);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvFile

int __cdecl CIcqProto::RecvFile( HANDLE hContact, PROTORECVFILE* evt )
{
	CCSDATA ccs = { hContact, PSR_FILE, 0, ( LPARAM )evt };
	return CallService( MS_PROTO_RECVFILE, 0, ( LPARAM )&ccs );
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvMsg

int __cdecl CIcqProto::RecvMsg( HANDLE hContact, PROTORECVEVENT* pre )
{
	DWORD cbBlob;
	DWORD flags = 0;

	cbBlob = strlennull(pre->szMessage) + 1;
	// process utf-8 encoded messages
	if ((pre->flags & PREF_UTF) && !IsUSASCII(pre->szMessage, strlennull(pre->szMessage)))
		flags |= DBEF_UTF;
	// process unicode ucs-2 messages
	if ((pre->flags & PREF_UNICODE) && !IsUnicodeAscii((WCHAR*)(pre->szMessage+cbBlob), strlennull((WCHAR*)(pre->szMessage+cbBlob))))
		cbBlob *= (sizeof(WCHAR)+1);

	ICQAddRecvEvent(hContact, EVENTTYPE_MESSAGE, pre, cbBlob, (PBYTE)pre->szMessage, flags);

	// stop contact from typing - some clients do not sent stop notify
	if (CheckContactCapabilities(hContact, CAPF_TYPING))
		CallService(MS_PROTO_CONTACTISTYPING, (WPARAM)hContact, PROTOTYPE_CONTACTTYPING_OFF);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvUrl

int __cdecl CIcqProto::RecvUrl( HANDLE hContact, PROTORECVEVENT* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendContacts

int __cdecl CIcqProto::SendContacts( HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList )
{
	if (hContact && hContactsList)
	{
		int i;
		DWORD dwUin;
		uid_str szUid;
		WORD wRecipientStatus;
		DWORD dwCookie;

		if (getContactUid(hContact, &dwUin, &szUid))
		{ // Invalid contact
			return ReportGenericSendError(hContact, ACKTYPE_CONTACTS, "The receiver has an invalid user ID.");
		}

		wRecipientStatus = getContactStatus(hContact);

		// Failures
		if (!icqOnline())
		{
			dwCookie = ReportGenericSendError(hContact, ACKTYPE_CONTACTS, "You cannot send messages when you are offline.");
		}
		else if (!hContactsList || (nContacts < 1) || (nContacts > MAX_CONTACTSSEND))
		{
			dwCookie = ReportGenericSendError(hContact, ACKTYPE_CONTACTS, "Bad data (internal error #1)");
		}
		// OK
		else
		{
			if (CheckContactCapabilities(hContact, CAPF_CONTACTS) && wRecipientStatus != ID_STATUS_OFFLINE)
			{ // Use the new format if possible
				int nDataLen, nNamesLen;
				struct icq_contactsend_s* contacts = NULL;

				// Format the data part and the names part
				// This is kinda messy, but there is no simple way to do it. First
				// we need to calculate the length of the packet.
				contacts = (struct icq_contactsend_s*)_alloca(sizeof(struct icq_contactsend_s)*nContacts);
				ZeroMemory(contacts, sizeof(struct icq_contactsend_s)*nContacts);
				{
					nDataLen = 0; nNamesLen = 0;
					for (i = 0; i < nContacts; i++)
					{
						uid_str szContactUid;

						if (!IsICQContact(hContactsList[i]))
							break; // Abort if a non icq contact is found
						if (getContactUid(hContactsList[i], &contacts[i].uin, &szContactUid))
							break; // Abort if invalid contact
						contacts[i].uid = contacts[i].uin?NULL:null_strdup(szContactUid);
						contacts[i].szNick = NickFromHandleUtf(hContactsList[i]);
						nDataLen += getUIDLen(contacts[i].uin, contacts[i].uid) + 4;
						nNamesLen += strlennull(contacts[i].szNick) + 8;
					}

					if (i == nContacts)
					{
						icq_packet mData, mNames;

#ifdef _DEBUG
						NetLog_Server("Sending contacts to %s.", strUID(dwUin, szUid));
#endif
						// Do not calculate the exact size of the data packet - only the maximal size (easier)
						// Sumarize size of group information
						// - we do not utilize the full power of the protocol and send all contacts with group "General"
						//   just like ICQ6 does
						nDataLen += 9;
						nNamesLen += 9;

						// Create data structures
						mData.wPlace = 0;
						mData.pData = (LPBYTE)SAFE_MALLOC(nDataLen);
						mData.wLen = nDataLen;
						mNames.wPlace = 0;
						mNames.pData = (LPBYTE)SAFE_MALLOC(nNamesLen);

						// pack Group Name
						packWord(&mData, 7);
						packBuffer(&mData, (LPBYTE)"General", 7);
						packWord(&mNames, 7);
						packBuffer(&mNames, (LPBYTE)"General", 7);

						// all contacts in one group
						packWord(&mData, (WORD)nContacts);
						packWord(&mNames, (WORD)nContacts);
						for (i = 0; i < nContacts; i++)
						{
							uid_str szContactUid;
							WORD wLen;

							if (contacts[i].uin)
								strUID(contacts[i].uin, szContactUid);
							else
								strcpy(szContactUid, contacts[i].uid);

							// prepare UID
							wLen = strlennull(szContactUid);
							packWord(&mData, wLen);
							packBuffer(&mData, (LPBYTE)szContactUid, wLen);

							// prepare Nick
							wLen = strlennull(contacts[i].szNick);
							packWord(&mNames, (WORD)(wLen + 4));
							packTLV(&mNames, 0x01, wLen, (LPBYTE)contacts[i].szNick);
						}

						// Cleanup temporary list
						for(i = 0; i < nContacts; i++)
						{
							SAFE_FREE((void**)&contacts[i].szNick);
							SAFE_FREE((void**)&contacts[i].uid);
						}

						// Rate check
						if (IsServerOverRate(ICQ_MSG_FAMILY, ICQ_MSG_SRV_SEND, RML_LIMIT))
						{ // rate is too high, the message will not go thru...
							SAFE_FREE((void**)&mData.pData);
							SAFE_FREE((void**)&mNames.pData);

							return ReportGenericSendError(hContact, ACKTYPE_CONTACTS, "The message could not be delivered. You are sending too fast. Wait a while and try again.");
						}

						// Set up the ack type
						cookie_message_data *pCookieData = CreateMessageCookieData(MTYPE_CONTACTS, hContact, dwUin, FALSE);

						// AIM clients do not send acknowledgement
						if (!dwUin && pCookieData->nAckType == ACKTYPE_CLIENT)
							pCookieData->nAckType = ACKTYPE_SERVER;
						// Send the message
						dwCookie = icq_SendChannel2Contacts(dwUin, szUid, hContact, (char*)mData.pData, mData.wPlace, (char*)mNames.pData, mNames.wPlace, pCookieData);

						// This will stop the message dialog from waiting for the real message delivery ack
						if (pCookieData->nAckType == ACKTYPE_NONE)
						{
							SendProtoAck(hContact, dwCookie, ACKRESULT_SUCCESS, ACKTYPE_CONTACTS, NULL);
							// We need to free this here since we will never see the real ack
							// The actual cookie value will still have to be returned to the message dialog though
							ReleaseCookie(dwCookie);
						}
						// Release our buffers
						SAFE_FREE((void**)&mData.pData);
						SAFE_FREE((void**)&mNames.pData);
					}
					else
					{
						dwCookie = ReportGenericSendError(hContact, ACKTYPE_CONTACTS, "Bad data (internal error #2)");
					}

					for(i = 0; i < nContacts; i++)
					{
						SAFE_FREE((void**)&contacts[i].szNick);
						SAFE_FREE((void**)&contacts[i].uid);
					}
				}
			}
			else if (dwUin)
			{ // old format is only understood by ICQ clients
				int nBodyLength;
				char szContactUin[UINMAXLEN];
				char szCount[17];
				struct icq_contactsend_s* contacts = NULL;
				uid_str szContactUid;


				// Format the body
				// This is kinda messy, but there is no simple way to do it. First
				// we need to calculate the length of the packet.
				contacts = (struct icq_contactsend_s*)_alloca(sizeof(struct icq_contactsend_s)*nContacts);
				ZeroMemory(contacts, sizeof(struct icq_contactsend_s)*nContacts);
				{
					nBodyLength = 0;
					for (i = 0; i < nContacts; i++)
					{
						if (!IsICQContact(hContactsList[i]))
							break; // Abort if a non icq contact is found
						if (getContactUid(hContactsList[i], &contacts[i].uin, &szContactUid))
							break; // Abort if invalid contact
						contacts[i].uid = contacts[i].uin?NULL:null_strdup(szContactUid);
						contacts[i].szNick = NickFromHandle(hContactsList[i]);
						// Compute this contact's length
						nBodyLength += getUIDLen(contacts[i].uin, contacts[i].uid) + 1;
						nBodyLength += strlennull(contacts[i].szNick) + 1;
					}

					if (i == nContacts)
					{
						char* pBody;
						char* pBuffer;

#ifdef _DEBUG
						NetLog_Server("Sending contacts to %d.", dwUin);
#endif
						// Compute count record's length
						_itoa(nContacts, szCount, 10);
						nBodyLength += strlennull(szCount) + 1;

						// Finally we need to copy the contact data into the packet body
						pBuffer = pBody = (char *)SAFE_MALLOC(nBodyLength);
						strncpy(pBuffer, szCount, nBodyLength);
						pBuffer += strlennull(pBuffer);
						*pBuffer++ = (char)0xFE;
						for (i = 0; i < nContacts; i++)
						{
							if (contacts[i].uin)
							{
								_itoa(contacts[i].uin, szContactUin, 10);
								strcpy(pBuffer, szContactUin);
							}
							else
								strcpy(pBuffer, contacts[i].uid);
							pBuffer += strlennull(pBuffer);
							*pBuffer++ = (char)0xFE;
							strcpy(pBuffer, contacts[i].szNick);
							pBuffer += strlennull(pBuffer);
							*pBuffer++ = (char)0xFE;
						}

						for (i = 0; i < nContacts; i++)
						{ // release memory
							SAFE_FREE((void**)&contacts[i].szNick);
							SAFE_FREE((void**)&contacts[i].uid);
						}

						// Set up the ack type
						cookie_message_data *pCookieData = CreateMessageCookieData(MTYPE_CONTACTS, hContact, dwUin, TRUE);

						if (m_bDCMsgEnabled && IsDirectConnectionOpen(hContact, DIRECTCONN_STANDARD, 0))
						{
							int iRes = icq_SendDirectMessage(hContact, pBody, nBodyLength, 1, pCookieData, NULL);

							if (iRes)
							{
								SAFE_FREE((void**)&pBody);

								return iRes; // we succeded, return
							}
						}

						// Rate check
						if (IsServerOverRate(ICQ_MSG_FAMILY, ICQ_MSG_SRV_SEND, RML_LIMIT))
						{ // rate is too high, the message will not go thru...
							SAFE_FREE((void**)&pCookieData);
							SAFE_FREE((void**)&pBody);

							return ReportGenericSendError(hContact, ACKTYPE_CONTACTS, "The message could not be delivered. You are sending too fast. Wait a while and try again.");
						}
						// Select channel and send
						if (!CheckContactCapabilities(hContact, CAPF_SRV_RELAY) || wRecipientStatus == ID_STATUS_OFFLINE)
						{
							dwCookie = icq_SendChannel4Message(dwUin, hContact, MTYPE_CONTACTS, (WORD)nBodyLength, pBody, pCookieData);
						}
						else
						{
							WORD wPriority;

							if (wRecipientStatus == ID_STATUS_ONLINE || wRecipientStatus == ID_STATUS_FREECHAT)
								wPriority = 0x0001;
							else
								wPriority = 0x0021;

							dwCookie = icq_SendChannel2Message(dwUin, hContact, pBody, nBodyLength, wPriority, pCookieData, NULL);
						}

						// This will stop the message dialog from waiting for the real message delivery ack
						if (pCookieData->nAckType == ACKTYPE_NONE)
						{
							SendProtoAck(hContact, dwCookie, ACKRESULT_SUCCESS, ACKTYPE_CONTACTS, NULL);
							// We need to free this here since we will never see the real ack
							// The actual cookie value will still have to be returned to the message dialog though
							ReleaseCookie(dwCookie);
						}
						SAFE_FREE((void**)&pBody);
					}
					else
					{
						dwCookie = ReportGenericSendError(hContact, ACKTYPE_CONTACTS, "Bad data (internal error #2)");
					}
				}
			}
			else
			{
				dwCookie = ReportGenericSendError(hContact, ACKTYPE_CONTACTS, "The reciever does not support receiving of contacts.");
			}
		}
		return dwCookie;
	}

	// Exit with Failure
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendFile - sends a file

int __cdecl CIcqProto::SendFile( HANDLE hContact, const char* szDescription, char** ppszFiles )
{
	if ( !icqOnline())
		return 0;

	if (hContact && szDescription && ppszFiles)
	{
		DWORD dwUin;
		uid_str szUid;

		if (getContactUid(hContact, &dwUin, &szUid))
			return 0; // Invalid contact

		if (getContactStatus(hContact) != ID_STATUS_OFFLINE)
		{
			if (CheckContactCapabilities(hContact, CAPF_OSCAR_FILE))
				return oftInitTransfer(hContact, dwUin, szUid, ppszFiles, szDescription);

			if (dwUin)
			{
				WORD wClientVersion;

				wClientVersion = getSettingWord(hContact, "Version", 7);
				if (wClientVersion < 7)
					NetLog_Server("IcqSendFile() can't send to version %u", wClientVersion);
				else
				{
					int i;
					filetransfer* ft;
					struct _stat statbuf;

					// Initialize filetransfer struct
					ft = CreateFileTransfer(hContact, dwUin, (wClientVersion == 7) ? 7: 8);

					for (ft->dwFileCount = 0; ppszFiles[ft->dwFileCount]; ft->dwFileCount++);
					ft->files = (char **)SAFE_MALLOC(sizeof(char *) * ft->dwFileCount);
					ft->dwTotalSize = 0;
					for (i = 0; i < (int)ft->dwFileCount; i++)
					{
						ft->files[i] = null_strdup(ppszFiles[i]);

						if (_stat(ppszFiles[i], &statbuf))
							NetLog_Server("IcqSendFile() was passed invalid filename(s)");
						else
							ft->dwTotalSize += statbuf.st_size;
					}
					ft->szDescription = null_strdup(szDescription);
					ft->dwTransferSpeed = 100;
					ft->sending = 1;
					ft->fileId = -1;
					ft->iCurrentFile = 0;
					ft->dwCookie = AllocateCookie(CKT_FILE, 0, hContact, ft);
					ft->hConnection = NULL;

					// Send file transfer request
					{
						char szFiles[64];
						char* pszFiles;


						NetLog_Server("Init file send");

						if (ft->dwFileCount == 1)
						{
							pszFiles = strrchr(ft->files[0], '\\');
							if (pszFiles)
								pszFiles++;
							else
								pszFiles = ft->files[0];
						}
						else
						{
							null_snprintf(szFiles, 64, ICQTranslate("%d Files"), ft->dwFileCount);
							pszFiles = szFiles;
						}

						// Send packet
						{
							if (ft->nVersion == 7)
							{
								if (m_bDCMsgEnabled && IsDirectConnectionOpen(hContact, DIRECTCONN_STANDARD, 0))
								{
									int iRes = icq_sendFileSendDirectv7(ft, pszFiles);
									if (iRes) return (int)(HANDLE)ft; // Success
								}
								NetLog_Server("Sending v%u file transfer request through server", 7);
								icq_sendFileSendServv7(ft, pszFiles);
							}
							else
							{
								if (m_bDCMsgEnabled && IsDirectConnectionOpen(hContact, DIRECTCONN_STANDARD, 0))
								{
									int iRes = icq_sendFileSendDirectv8(ft, pszFiles);
									if (iRes) return (int)(HANDLE)ft; // Success
								}
								NetLog_Server("Sending v%u file transfer request through server", 8);
								icq_sendFileSendServv8(ft, pszFiles, ACKTYPE_NONE);
							}
						}
					}

					return (int)(HANDLE)ft; // Success
				}
			}
		}
	}

	return 0; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// PS_SendMessage - sends a message

int __cdecl CIcqProto::SendMsg( HANDLE hContact, int flags, const char* pszSrc )
{
	if (hContact && pszSrc)
	{
		WORD wRecipientStatus;
		DWORD dwCookie;
		char* pszText = NULL;
		char* puszText = NULL;
		int bNeedFreeA = 0, bNeedFreeU = 0;

		// Invalid contact
		DWORD dwUin;
		uid_str szUID;
		if (getContactUid(hContact, &dwUin, &szUID))
			return ReportGenericSendError(hContact, ACKTYPE_MESSAGE, "The receiver has an invalid user ID.");

		if ((flags & PREF_UTF) == PREF_UTF)
			puszText = (char*)pszSrc;
		else
			pszText = (char*)pszSrc;

		if ((flags & PREF_UNICODE) == PREF_UNICODE)
		{
			puszText = make_utf8_string((WCHAR*)((char*)pszSrc+strlennull(pszText)+1)); // get the UTF-16 part
			bNeedFreeU = 1;
		}

		wRecipientStatus = getContactStatus(hContact);

		if (puszText)
		{ // we have unicode message, check if it is possible and reasonable to send it as unicode
			BOOL plain_ascii = IsUSASCII(puszText, strlennull(puszText));

			if (plain_ascii || !m_bUtfEnabled || !CheckContactCapabilities(hContact, CAPF_UTF) ||
				!getSettingByte(hContact, "UnicodeSend", 1))
			{ // unicode is not available for target contact, convert to good codepage
				if (!plain_ascii)
				{ // do not convert plain ascii messages
					char* szUserAnsi = ConvertMsgToUserSpecificAnsi(hContact, puszText); 

					if (szUserAnsi)
					{ // we have special encoding, use it
						pszText = szUserAnsi; 
						bNeedFreeA = 1;
					}
					else if (!pszText)
					{ // no ansi available, create basic
						utf8_decode(puszText, &pszText);
						bNeedFreeA = 1;
					}
				}
				else if (!pszText)
				{ // plain ascii unicode message, take as ansi if no ansi available
					pszText = (char*)puszText;
					bNeedFreeA = bNeedFreeU;
					puszText = NULL;
				}
				// dispose unicode message
				if (bNeedFreeU)
					SAFE_FREE((void**)&puszText);
				else
					puszText = NULL;
			}
		}

		if (m_bTempVisListEnabled && m_iStatus == ID_STATUS_INVISIBLE)
			makeContactTemporaryVisible(hContact);  // make us temporarily visible to contact

		// Failure scenarios
		if (!icqOnline())
		{
			dwCookie = ReportGenericSendError(hContact, ACKTYPE_MESSAGE, "You cannot send messages when you are offline.");
		}
		else if ((wRecipientStatus == ID_STATUS_OFFLINE) && (strlennull(pszText) > 4096))
		{
			dwCookie = ReportGenericSendError(hContact, ACKTYPE_MESSAGE, "Messages to offline contacts must be shorter than 4096 characters.");
		}
		// Looks OK
		else
		{
			if (!puszText && m_bUtfEnabled == 2 && !IsUSASCII(pszText, strlennull(pszText))
				&& CheckContactCapabilities(hContact, CAPF_UTF) && getSettingByte(hContact, "UnicodeSend", 1))
			{ // text is not unicode and contains national chars and we should send all this as Unicode, so do it
				puszText = ansi_to_utf8(pszText);
				bNeedFreeU = 1;
			}

			if (!dwUin)
			{ // prepare AIM Html message
				char *src, *mng;

				if (puszText)
					src = (char*)puszText;
				else
					src = pszText;
				mng = MangleXml(src, strlennull(src));
				src = (char*)SAFE_MALLOC(strlennull(mng) + 28);
				strcpy(src, "<HTML><BODY>");
				strcat(src, mng);
				SAFE_FREE((void**)&mng);
				strcat(src, "</BODY></HTML>");
				if (puszText)
				{ // convert to UCS-2
					if (bNeedFreeU) SAFE_FREE((void**)&puszText);
					puszText = src;
					bNeedFreeU = 1;
				}
				else
				{
					if (bNeedFreeA) SAFE_FREE((void**)&pszText);
					pszText = src;
					bNeedFreeA = 1;
				}
			}
			// Set up the ack type
			cookie_message_data *pCookieData = CreateMessageCookieData(MTYPE_PLAIN, hContact, dwUin, TRUE);

#ifdef _DEBUG
			NetLog_Server("Send %s message - Message cap is %u", puszText ? "unicode" : "", CheckContactCapabilities(hContact, CAPF_SRV_RELAY));
			NetLog_Server("Send %s message - Contact status is %u", puszText ? "unicode" : "", wRecipientStatus);
#endif
			if (dwUin && m_bDCMsgEnabled && IsDirectConnectionOpen(hContact, DIRECTCONN_STANDARD, 0))
			{ // send thru direct
				char* dc_msg = pszText;
				char* dc_cap = NULL;

				if (puszText)
				{ // direct connection uses utf-8, prepare
					dc_msg = (char*)puszText;
					dc_cap = CAP_UTF8MSGS;
				}
				dwCookie = icq_SendDirectMessage(hContact, dc_msg, strlennull(dc_msg), 1, pCookieData, dc_cap);

				if (dwCookie)
				{ // free the buffers if alloced
					if (bNeedFreeA) SAFE_FREE((void**)&pszText);
					if (bNeedFreeU) SAFE_FREE((void**)&puszText);

					return dwCookie; // we succeded, return
				}
				// on failure, fallback to send thru server
			}
			if (!dwUin || !CheckContactCapabilities(hContact, CAPF_SRV_RELAY) || wRecipientStatus == ID_STATUS_OFFLINE)
			{
				WCHAR* pwszText = NULL;

				if (puszText) pwszText = make_unicode_string(puszText);
				if ((pwszText ? strlennull(pwszText) * sizeof(WCHAR) : strlennull(pszText)) > MAX_MESSAGESNACSIZE)
				{ // max length check // TLV(2) is currently limited to 0xA00 bytes in online mode
					// only limit to not get disconnected, all other will be handled by error 0x0A
					dwCookie = ReportGenericSendError(hContact, ACKTYPE_MESSAGE, "The message could not be delivered, it is too long.");

					SAFE_FREE((void**)&pCookieData);
					// free the buffers if alloced
					SAFE_FREE((void**)&pwszText);
					if (bNeedFreeA) SAFE_FREE((void**)&pszText);
					if (bNeedFreeU) SAFE_FREE((void**)&puszText);

					return dwCookie;
				}
				// Rate check
				if (IsServerOverRate(ICQ_MSG_FAMILY, ICQ_MSG_SRV_SEND, RML_LIMIT))
				{ // rate is too high, the message will not go thru...
					dwCookie = ReportGenericSendError(hContact, ACKTYPE_MESSAGE, "The message could not be delivered. You are sending too fast. Wait a while and try again.");

					SAFE_FREE((void**)&pCookieData);
					// free the buffers if alloced
					SAFE_FREE((void**)&pwszText);
					if (bNeedFreeA) SAFE_FREE((void**)&pszText);
					if (bNeedFreeU) SAFE_FREE((void**)&puszText);

					return dwCookie;
				}
				// set flag for offline messages - to allow proper error handling
				if (wRecipientStatus == ID_STATUS_OFFLINE) ((cookie_message_data_ext*)pCookieData)->isOffline = TRUE;

				if (pwszText)
					dwCookie = icq_SendChannel1MessageW(dwUin, szUID, hContact, pwszText, pCookieData);
				else
					dwCookie = icq_SendChannel1Message(dwUin, szUID, hContact, pszText, pCookieData);
				// free the unicode message
				SAFE_FREE((void**)&pwszText);
			}
			else
			{
				WORD wPriority;
				char* srv_msg = pszText;
				char* srv_cap = NULL;

				if (wRecipientStatus == ID_STATUS_ONLINE || wRecipientStatus == ID_STATUS_FREECHAT)
					wPriority = 0x0001;
				else
					wPriority = 0x0021;

				if (puszText)
				{ // type-2 messages are utf-8 encoded, prepare
					srv_msg = (char*)puszText;
					srv_cap = CAP_UTF8MSGS;
				}
				if (strlennull(srv_msg) + (puszText ? 144 : 102) > MAX_MESSAGESNACSIZE)
				{ // max length check
					dwCookie = ReportGenericSendError(hContact, ACKTYPE_MESSAGE, "The message could not be delivered, it is too long.");

					SAFE_FREE((void**)&pCookieData);
					// free the buffers if alloced
					if (bNeedFreeA) SAFE_FREE((void**)&pszText);
					if (bNeedFreeU) SAFE_FREE((void**)&puszText);

					return dwCookie;
				}
				// Rate check
				if (IsServerOverRate(ICQ_MSG_FAMILY, ICQ_MSG_SRV_SEND, RML_LIMIT))
				{ // rate is too high, the message will not go thru...
					dwCookie = ReportGenericSendError(hContact, ACKTYPE_MESSAGE, "The message could not be delivered. You are sending too fast. Wait a while and try again.");

					SAFE_FREE((void**)&pCookieData);
					// free the buffers if alloced
					if (bNeedFreeA) SAFE_FREE((void**)&pszText);
					if (bNeedFreeU) SAFE_FREE((void**)&puszText);

					return dwCookie;
				}
				// WORKAROUND!!
				// Nasty workaround for ICQ6 client's bug - it does not send acknowledgement when in temporary visible mode
				// - This uses only server ack, but also for visible invisible contact!
				if (wRecipientStatus == ID_STATUS_INVISIBLE && pCookieData->nAckType == ACKTYPE_CLIENT && getSettingByte(hContact, "ClientID", CLID_GENERIC) == CLID_ICQ6)
					pCookieData->nAckType = ACKTYPE_SERVER;

				dwCookie = icq_SendChannel2Message(dwUin, hContact, srv_msg, strlennull(srv_msg), wPriority, pCookieData, srv_cap);
			}

			// This will stop the message dialog from waiting for the real message delivery ack
			if (pCookieData->nAckType == ACKTYPE_NONE)
			{
				SendProtoAck(hContact, dwCookie, ACKRESULT_SUCCESS, ACKTYPE_MESSAGE, NULL);
				// We need to free this here since we will never see the real ack
				// The actual cookie value will still have to be returned to the message dialog though
				ReleaseCookie(dwCookie);
			}
		}
		// free the buffers if alloced
		if (bNeedFreeA) SAFE_FREE((void**)&pszText);
		if (bNeedFreeU) SAFE_FREE((void**)&puszText);

		return dwCookie; // Success
	}

	return 0; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// SendUrl

int __cdecl CIcqProto::SendUrl( HANDLE hContact, int flags, const char* url )
{
	if (hContact && url)
	{
		DWORD dwCookie;
		WORD wRecipientStatus;
		DWORD dwUin;

		if (getContactUid(hContact, &dwUin, NULL))
		{ // Invalid contact
			return ReportGenericSendError(hContact, ACKTYPE_URL, "The receiver has an invalid user ID.");
		}

		wRecipientStatus = getContactStatus(hContact);

		// Failure
		if (!icqOnline())
		{
			dwCookie = ReportGenericSendError(hContact, ACKTYPE_URL, "You cannot send messages when you are offline.");
		}
		// Looks OK
		else
		{
			char* szDesc;
			char* szBody;
			int nBodyLen;
			int nDescLen;
			int nUrlLen;


			// Set up the ack type
			cookie_message_data *pCookieData = CreateMessageCookieData(MTYPE_URL, hContact, dwUin, TRUE);

			// Format the body
			nUrlLen = strlennull(url);
			szDesc = (char *)url + nUrlLen + 1;
			nDescLen = strlennull(szDesc);
			nBodyLen = nUrlLen + nDescLen + 2;
			szBody = (char *)_alloca(nBodyLen);
			strcpy(szBody, szDesc);
			szBody[nDescLen] = (char)0xFE; // Separator
			strcpy(szBody + nDescLen + 1, url);

			if (m_bDCMsgEnabled && IsDirectConnectionOpen(hContact, DIRECTCONN_STANDARD, 0))
			{
				int iRes = icq_SendDirectMessage(hContact, szBody, nBodyLen, 1, pCookieData, NULL);
				if (iRes) return iRes; // we succeded, return
			}

			// Rate check
			if (IsServerOverRate(ICQ_MSG_FAMILY, ICQ_MSG_SRV_SEND, RML_LIMIT))
			{ // rate is too high, the message will not go thru...
				SAFE_FREE((void**)&pCookieData);

				return ReportGenericSendError(hContact, ACKTYPE_URL, "The message could not be delivered. You are sending too fast. Wait a while and try again.");
			}
			// Select channel and send
			if (!CheckContactCapabilities(hContact, CAPF_SRV_RELAY) ||
				wRecipientStatus == ID_STATUS_OFFLINE)
			{
				dwCookie = icq_SendChannel4Message(dwUin, hContact, MTYPE_URL,
					(WORD)nBodyLen, szBody, pCookieData);
			}
			else
			{
				WORD wPriority;

				if (wRecipientStatus == ID_STATUS_ONLINE || wRecipientStatus == ID_STATUS_FREECHAT)
					wPriority = 0x0001;
				else
					wPriority = 0x0021;

				dwCookie = icq_SendChannel2Message(dwUin, hContact, szBody, nBodyLen, wPriority, pCookieData, NULL);
			}

			// This will stop the message dialog from waiting for the real message delivery ack
			if (pCookieData->nAckType == ACKTYPE_NONE)
			{
				SendProtoAck(hContact, dwCookie, ACKRESULT_SUCCESS, ACKTYPE_URL, NULL);
				// We need to free this here since we will never see the real ack
				// The actual cookie value will still have to be returned to the message dialog though
				ReleaseCookie(dwCookie);
			}
		}

		return dwCookie; // Success
	}

	return 0; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// PS_SetApparentMode - sets the visibility status

int __cdecl CIcqProto::SetApparentMode( HANDLE hContact, int mode )
{
	DWORD uin;
	uid_str uid;

	if (getContactUid(hContact, &uin, &uid))
		return 1; // Invalid contact

	if (hContact)
	{
		// Only 3 modes are supported
		if (mode == 0 || mode == ID_STATUS_ONLINE || mode == ID_STATUS_OFFLINE)
		{
			int oldMode = getSettingWord(hContact, "ApparentMode", 0);

			// Don't send redundant updates
			if (mode != oldMode)
			{
				setSettingWord(hContact, "ApparentMode", (WORD)mode);

				// Not being online is only an error when in SS mode. This is not handled
				// yet so we just ignore this for now.
				if (icqOnline())
				{
					if (oldMode != 0)
					{ // Remove from old list
						if (oldMode == ID_STATUS_OFFLINE && getSettingWord(hContact, DBSETTING_SERVLIST_IGNORE, 0))
						{ // Need to remove Ignore item as well
							icq_removeServerPrivacyItem(hContact, uin, uid, getSettingWord(hContact, DBSETTING_SERVLIST_IGNORE, 0), SSI_ITEM_IGNORE);

							setSettingWord(hContact, DBSETTING_SERVLIST_IGNORE, 0);
						}
						icq_sendChangeVisInvis(hContact, uin, uid, oldMode==ID_STATUS_OFFLINE, 0);
					}
					if (mode != 0)
					{ // Add to new list
						if (mode==ID_STATUS_OFFLINE && getSettingWord(hContact, DBSETTING_SERVLIST_IGNORE, 0))
							return 0; // Success: offline by ignore item

						icq_sendChangeVisInvis(hContact, uin, uid, mode==ID_STATUS_OFFLINE, 1);
					}
				}

				return 0; // Success
			}
		}
	}

	return 1; // Failure
}


////////////////////////////////////////////////////////////////////////////////////////
// PrepareStatusNote - returns correct status note for given status

char* CIcqProto::PrepareStatusNote(int nStatus)
{
  char *szStatusNote = NULL;
        
  if (getSettingByte(NULL, "XStatusMsgAsNote", DEFAULT_XSTATUS_STATUS_NOTE))
  {
    BYTE bXStatus = getContactXStatus(NULL);

    if (bXStatus && moodXStatus[bXStatus-1] != -1)
    { // use custom status message as status note
      szStatusNote = getSettingStringUtf(NULL, DBSETTING_XSTATUS_MSG, "");
    }
  }

  if (!szStatusNote)
  { // get standard status message
    EnterCriticalSection(&m_modeMsgsMutex);

    char **pszStatusNote = MirandaStatusToAwayMsg(nStatus);
    if (pszStatusNote)
      szStatusNote = null_strdup(*pszStatusNote);
    LeaveCriticalSection(&m_modeMsgsMutex);
  }

  if (!szStatusNote)
    // nothing available set empty status note
    szStatusNote = null_strdup("");

  return szStatusNote;
}


////////////////////////////////////////////////////////////////////////////////////////
// PS_SetStatus - sets the protocol status

int __cdecl CIcqProto::SetStatus(int iNewStatus)
{
	int nNewStatus = MirandaStatusToSupported(iNewStatus);

	// check if netlib handles are ready
	if (!m_hServerNetlibUser)
		return 0;

	if (m_bTempVisListEnabled && icqOnline()) // remove temporary visible users
		sendEntireListServ(ICQ_BOS_FAMILY, ICQ_CLI_REMOVETEMPVISIBLE, BUL_TEMPVISIBLE);

	if (nNewStatus != m_iStatus)
	{
		// clear custom status on status change
		if (getSettingByte(NULL, "XStatusReset", DEFAULT_XSTATUS_RESET))
		 	setXStatusEx(0, 0);

		// New status is OFFLINE
		if (nNewStatus == ID_STATUS_OFFLINE)
    { // for quick logoff
      if (icqOnline())
      { // set offline status note (otherwise the old will remain)
        char *szOfflineNote = PrepareStatusNote(nNewStatus);

        // Create unnamed event to wait until the status note change process is completed
        m_hNotifyNameInfoEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        int bNoteChanged = SetStatusNote(szOfflineNote, 0, FALSE);

        SAFE_FREE((void**)&szOfflineNote);

        // Note was changed, wait until the process is over
        if (bNoteChanged)
          ICQWaitForSingleObject(m_hNotifyNameInfoEvent, 4000, TRUE);

        // Release the event
        CloseHandle(m_hNotifyNameInfoEvent);
        m_hNotifyNameInfoEvent = NULL;
      }

			m_iDesiredStatus = nNewStatus;

      if (hServerConn)
      { // Connected, Send disconnect packet
			  icq_sendCloseConnection();

			  icq_serverDisconnect(FALSE);

  			SetCurrentStatus(ID_STATUS_OFFLINE);

	  		NetLog_Server("Logged off.");
      }
		}
		else
		{
			switch (m_iStatus) {

			// We are offline and need to connect
			case ID_STATUS_OFFLINE:
				{
					// Update user connection settings
					UpdateGlobalSettings();

					// Read UIN from database
					m_dwLocalUIN = getContactUin(NULL);
					if (m_dwLocalUIN == 0)
					{
						SetCurrentStatus(ID_STATUS_OFFLINE);

						icq_LogMessage(LOG_FATAL, LPGEN("You have not entered a ICQ number.\nConfigure this in Options->Network->ICQ and try again."));
						return 0;
					}

					// Set status to 'Connecting'
					m_iDesiredStatus = nNewStatus;
					SetCurrentStatus(ID_STATUS_CONNECTING);

					// Read password from database
					char *pszPwd = GetUserPassword(FALSE);

					if (pszPwd)
						icq_login(pszPwd);
					else
						RequestPassword();

					break;
				}

				// We are connecting... We only need to change the going online status
			case ID_STATUS_CONNECTING:
				m_iDesiredStatus = nNewStatus;
				break;

				// We are already connected so we should just change status
			default:
				SetCurrentStatus(nNewStatus);

        char *szStatusNote = PrepareStatusNote(nNewStatus);
        
        //! This is a bit tricky, we do trigger status note change thread and then
        // change the status note right away (this spares one packet) - so SetStatusNote()
        // will only change User Details Directory
        SetStatusNote(szStatusNote, 6000, FALSE);

				if (m_iStatus == ID_STATUS_INVISIBLE)
				{
					if (m_bSsiEnabled)
						updateServVisibilityCode(3);
					icq_setstatus(MirandaStatusToIcq(m_iStatus), szStatusNote);
				}
				else
				{
					icq_setstatus(MirandaStatusToIcq(m_iStatus), szStatusNote);
					if (m_bSsiEnabled)
						updateServVisibilityCode(4);
				}
				if (m_bAimEnabled)
        {
          EnterCriticalSection(&m_modeMsgsMutex);

          char **pszStatusNote = MirandaStatusToAwayMsg(nNewStatus);
          if (pszStatusNote)
        		icq_sendSetAimAwayMsgServ(*pszStatusNote);
          LeaveCriticalSection(&m_modeMsgsMutex);
        }

        SAFE_FREE((void**)&szStatusNote);
			}
		}
	}

	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////
// RecvAuth - returns a contact's away message

int __cdecl CIcqProto::RecvAuth(WPARAM wParam, LPARAM lParam)
{
	CCSDATA* ccs = (CCSDATA*)lParam;
	PROTORECVEVENT* pre = (PROTORECVEVENT*)ccs->lParam;

	setContactHidden(ccs->hContact, 0);

	ICQAddRecvEvent(NULL, EVENTTYPE_AUTHREQUEST, pre, pre->lParam, (PBYTE)pre->szMessage, 0);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetAwayMsgThread - return a contact's status message

struct status_message_thread_data
{
  HANDLE hContact;
  char *szMessage;
  HANDLE hProcess;
};

void __cdecl CIcqProto::GetAwayMsgThread( void *pStatusData )
{
  status_message_thread_data *pThreadData = (status_message_thread_data*)pStatusData;
	
  if (pThreadData)
  {
    char *szAnsiMsg = NULL;

    // wait a little
    Sleep(100);

    if (utf8_decode(pThreadData->szMessage, &szAnsiMsg))
      BroadcastAck(pThreadData->hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, pThreadData->hProcess, (LPARAM)szAnsiMsg);

    SAFE_FREE((void**)&szAnsiMsg);
    SAFE_FREE((void**)&pThreadData);
  }
}

////////////////////////////////////////////////////////////////////////////////////////
// PS_GetAwayMsg - returns a contact's away message

int __cdecl CIcqProto::GetAwayMsg( HANDLE hContact )
{
	DWORD dwUin;
	uid_str szUID;

	if (getContactUid(hContact, &dwUin, &szUID))
		return 0; // Invalid contact

  // First check if the contact has Status Note, if yes give it
  char *szStatusNote = getSettingStringUtf(hContact, DBSETTING_STATUS_NOTE, NULL);

  if (strlennull(szStatusNote) > 0)
  { // Give Status Note
    status_message_thread_data *pThreadData = (status_message_thread_data*)SAFE_MALLOC(sizeof(status_message_thread_data));

    pThreadData->hContact = hContact;
    pThreadData->szMessage = szStatusNote;
    pThreadData->hProcess = (HANDLE)GenerateCookie(0);
	 ForkThread(&CIcqProto::GetAwayMsgThread, pThreadData);

    return (int)pThreadData->hProcess;
  }
  SAFE_FREE((void**)&szStatusNote);

  if (!icqOnline())
		return 0;

	WORD wStatus = getContactStatus(hContact);

	if (dwUin)
	{
		int wMessageType = 0;

		switch(wStatus)
		{

		case ID_STATUS_AWAY:
			wMessageType = MTYPE_AUTOAWAY;
			break;

		case ID_STATUS_NA:
			wMessageType = MTYPE_AUTONA;
			break;

		case ID_STATUS_OCCUPIED:
			wMessageType = MTYPE_AUTOBUSY;
			break;

		case ID_STATUS_DND:
			wMessageType = MTYPE_AUTODND;
			break;

		case ID_STATUS_FREECHAT:
			wMessageType = MTYPE_AUTOFFC;
			break;

		default:
			break;
		}

		if (wMessageType)
		{
			if (m_bDCMsgEnabled && IsDirectConnectionOpen(hContact, DIRECTCONN_STANDARD, 0))
			{
				int iRes = icq_sendGetAwayMsgDirect(hContact, wMessageType);
				if (iRes) return iRes; // we succeded, return
			}
			if (CheckContactCapabilities(hContact, CAPF_STATUSMSG_EXT))
				return icq_sendGetAwayMsgServExt(hContact, dwUin, wMessageType,
				(WORD)(getSettingWord(hContact, "Version", 0)==9?9:ICQ_VERSION)); // Success
			else
				return icq_sendGetAwayMsgServ(hContact, dwUin, wMessageType,
				(WORD)(getSettingWord(hContact, "Version", 0)==9?9:ICQ_VERSION)); // Success
		}
	}
	else if (wStatus == ID_STATUS_AWAY)
		return icq_sendGetAimAwayMsgServ(hContact, szUID, MTYPE_AUTOAWAY);

	return 0; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// PSR_AWAYMSG

int __cdecl CIcqProto::RecvAwayMsg( HANDLE hContact, int statusMode, PROTORECVEVENT* evt )
{
	BroadcastAck(hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, (HANDLE)evt->lParam, (LPARAM)evt->szMessage);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AWAYMSG

int __cdecl CIcqProto::SendAwayMsg( HANDLE hContact, HANDLE hProcess, const char* msg )
{
	return 1;
}


////////////////////////////////////////////////////////////////////////////////////////
// PS_SetAwayMsg - sets the away status message

int __cdecl CIcqProto::SetAwayMsg(int status, const char* msg)
{
	EnterCriticalSection(&m_modeMsgsMutex);

	char **ppszMsg = MirandaStatusToAwayMsg(status);
	if (!ppszMsg)
	{
		LeaveCriticalSection(&m_modeMsgsMutex);
		return 1; // Failure
	}

	// Prepare UTF-8 status message
	char *szNewUtf = ansi_to_utf8(msg);

	if (strcmpnull(szNewUtf, *ppszMsg))
	{
		// Free old message
		SAFE_FREE((void**)ppszMsg);

		// Set new message
		*ppszMsg = szNewUtf;
		szNewUtf = NULL;

    if (icqOnline())
    { // update current status note
      char *szNote = *ppszMsg ? *ppszMsg : "";

      if (getSettingByte(NULL, "XStatusMsgAsNote", DEFAULT_XSTATUS_STATUS_NOTE))
      {
        BYTE bXStatus = getContactXStatus(NULL);
    
        if (!bXStatus || moodXStatus[bXStatus-1] == -1)
          SetStatusNote(szNote, 1000, FALSE);
      }
      else
        SetStatusNote(szNote, 1000, FALSE);

  		if (m_bAimEnabled && (m_iStatus == status))
	  		icq_sendSetAimAwayMsgServ(*ppszMsg);
    }
	}
	SAFE_FREE((void**)&szNewUtf);

	LeaveCriticalSection(&m_modeMsgsMutex);

	return 0; // Success
}


/////////////////////////////////////////////////////////////////////////////////////////
// PS_UserIsTyping - sends a UTN notification

int __cdecl CIcqProto::UserIsTyping( HANDLE hContact, int type )
{
	if (hContact && icqOnline())
	{
		if (CheckContactCapabilities(hContact, CAPF_TYPING))
		{
			switch (type) {
			case PROTOTYPE_SELFTYPING_ON:
				sendTypingNotification(hContact, MTN_BEGUN);
				return 0;

			case PROTOTYPE_SELFTYPING_OFF:
				sendTypingNotification(hContact, MTN_FINISHED);
				return 0;
			}
		}
	}

	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnEvent - maintain protocol events

int __cdecl CIcqProto::OnEvent( PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam )
{
	switch( eventType ) {
		case EV_PROTO_ONLOAD:    return OnModulesLoaded( 0, 0 );
		case EV_PROTO_ONEXIT:    return OnPreShutdown( 0, 0 );
		case EV_PROTO_ONOPTIONS: return OnOptionsInit( wParam, lParam );
	}
	return 1;
}
