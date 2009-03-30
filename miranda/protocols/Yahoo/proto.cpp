/*
 * $Id: options.h 8425 2008-10-15 16:02:48Z gena01 $
 *
 * myYahoo Miranda Plugin 
 *
 * Authors: Gennady Feldman (aka Gena01) 
 *          Laurent Marechal (aka Peorth)
 *
 * This code is under GPL and is based on AIM, MSN and Miranda source code.
 * I want to thank Robert Rainwater and George Hazan for their code and support
 * and for answering some of my questions during development of this plugin.
 */

#include "yahoo.h"

#include <m_idle.h>
#include <m_options.h>
#include <m_skin.h>

#include "resource.h"
#include "file_transfer.h"

#pragma warning(disable:4355)

CYahooProto::CYahooProto( const char* aProtoName, const TCHAR* aUserName ) :
	poll_loop( 1 )
{
	m_tszUserName = mir_tstrdup( aUserName );
	m_szModuleName = mir_strdup( aProtoName );

	m_startStatus = ID_STATUS_ONLINE;

	logoff_buddies();

	SkinAddNewSoundEx(Translate( "mail" ), m_szModuleName, "New E-mail available in Inbox" );
	
	LoadYahooServices();
	IconsInit();
}

CYahooProto::~CYahooProto()
{
	if (m_bLoggedIn)
		logout();
	DebugLog("Logged out");

	DestroyHookableEvent(hYahooNudge);

	mir_free( m_szModuleName );
	mir_free( m_tszUserName );

	FREE(m_startMsg);
}

////////////////////////////////////////////////////////////////////////////////////////
// OnModulesLoadedEx - performs hook registration

static COLORREF crCols[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

int CYahooProto::OnModulesLoadedEx( WPARAM, LPARAM )
{
	MenuInit();

	YHookEvent(ME_DB_CONTACT_SETTINGCHANGED, &CYahooProto::OnSettingChanged);
	YHookEvent(ME_IDLE_CHANGED, &CYahooProto::OnIdleEvent);

	char tModuleDescr[ 100 ];
	wsprintfA(tModuleDescr, Translate( "%s plugin connections" ), m_szModuleName);
	
	NETLIBUSER nlu = {0};
	nlu.cbSize = sizeof(nlu);
#ifdef HTTP_GATEWAY
	nlu.flags = NUF_OUTGOING | NUF_HTTPGATEWAY| NUF_HTTPCONNS;
#else
  	nlu.flags = NUF_OUTGOING | NUF_HTTPCONNS;
#endif
	nlu.szSettingsModule = m_szModuleName;
	nlu.szDescriptiveName = tModuleDescr;
	
#ifdef HTTP_GATEWAY
	// Here comes the Gateway Code! 
	nlu.szHttpGatewayHello = NULL;
	nlu.szHttpGatewayUserAgent = "User-Agent: Mozilla/4.01 [en] (Win95; I)";
 	nlu.pfnHttpGatewayInit = YAHOO_httpGatewayInit;
	nlu.pfnHttpGatewayBegin = NULL;
	nlu.pfnHttpGatewayWrapSend = YAHOO_httpGatewayWrapSend;
	nlu.pfnHttpGatewayUnwrapRecv = YAHOO_httpGatewayUnwrapRecv;
#endif	
	
	m_hNetlibUser = ( HANDLE )YAHOO_CallService( MS_NETLIB_REGISTERUSER, 0, ( LPARAM )&nlu );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// AddToList - adds a contact to the contact list

HANDLE CYahooProto::AddToList( int flags, PROTOSEARCHRESULT* psr )
{
	DebugLog("[YahooAddToList] Flags: %d", flags);

	if (!m_bLoggedIn){
		DebugLog("[YahooAddToList] WARNING: WE ARE OFFLINE!");
		return 0;
	}

	if (psr == NULL || psr->cbSize != sizeof( PROTOSEARCHRESULT )) {
		DebugLog("[YahooAddToList] Empty data passed?");
		return 0;
	}

	HANDLE hContact = getbuddyH(psr->nick);
	if (hContact != NULL) {
		if (DBGetContactSettingByte(hContact, "CList", "NotOnList", 0)) {
			DebugLog("[YahooAddToList] Temporary Buddy:%s already on our buddy list", psr->nick);
			//return 0;
		} else {
			DebugLog("[YahooAddToList] Buddy:%s already on our buddy list", psr->nick);
			return 0;
		}
	} else if (flags & PALF_TEMPORARY ) { /* not on our list */
		DebugLog("[YahooAddToList] Adding Temporary Buddy:%s ", psr->nick);
	}

	int protocol = psr->reserved[0];
	DebugLog("Adding buddy:%s", psr->nick);
	return add_buddy(psr->nick, psr->nick, protocol, flags);
}

HANDLE __cdecl CYahooProto::AddToListByEvent( int flags, int /*iContact*/, HANDLE hDbEvent )
{
	DBEVENTINFO dbei;
	HANDLE hContact;

	DebugLog("[YahooAddToListByEvent]");
	if ( !m_bLoggedIn )
		return 0;

	memset( &dbei, 0, sizeof( dbei ));
	dbei.cbSize = sizeof( dbei );

	if (( dbei.cbBlob = YAHOO_CallService( MS_DB_EVENT_GETBLOBSIZE, ( LPARAM )hDbEvent, 0 )) == -1 ) {
		DebugLog("[YahooAddToListByEvent] ERROR: Can't get blob size.");
		return 0;
	}

	DebugLog("[YahooAddToListByEvent] Got blob size: %lu", dbei.cbBlob);
	dbei.pBlob = ( PBYTE )_alloca( dbei.cbBlob );
	if ( YAHOO_CallService( MS_DB_EVENT_GET, ( WPARAM )hDbEvent, ( LPARAM )&dbei )) {
		DebugLog("[YahooAddToListByEvent] ERROR: Can't get event.");
		return 0;
	}

	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST ) {
		DebugLog("[YahooAddToListByEvent] ERROR: Not an authorization request.");
		return 0;
	}

	if ( strcmp( dbei.szModule, m_szModuleName )) {
		DebugLog("[YahooAddToListByEvent] ERROR: Not Yahoo protocol.");
		return 0;
	}

	//Adds a contact to the contact list given an auth, added or contacts event
//wParam=MAKEWPARAM(flags,iContact)
//lParam=(LPARAM)(HANDLE)hDbEvent
//Returns a HANDLE to the new contact, or NULL on failure
//hDbEvent must be either EVENTTYPE_AUTHREQ or EVENTTYPE_ADDED
//flags are the same as for PS_ADDTOLIST.
//iContact is only used for contacts events. It is the 0-based index of the
//contact in the event to add. There is no way to add two or more contacts at
//once, you should just do lots of calls.

	/* TYPE ADDED
		blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), 
		last(ASCIIZ), email(ASCIIZ) 
	
	   TYPE AUTH REQ
		blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), 
		last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)
	*/
	memcpy(&hContact,( char* )( dbei.pBlob + sizeof( DWORD ) ), sizeof(HANDLE)); 

	if (hContact != NULL) {
		DebugLog("Temp Buddy found at: %p ", hContact);
	} else
		DebugLog("hContact NULL???");

	return hContact;
}

////////////////////////////////////////////////////////////////////////////////////////
// AuthAllow - processes the successful authorization

int CYahooProto::Authorize( HANDLE hdbe )
{
	DebugLog("[YahooAuthAllow]");
	if ( !m_bLoggedIn ) {
		DebugLog("[YahooAuthAllow] Not Logged In!");
		return 1;
	}

	DBEVENTINFO dbei;
	memset( &dbei, 0, sizeof( dbei ));
	dbei.cbSize = sizeof( dbei );
	if (( dbei.cbBlob = YAHOO_CallService( MS_DB_EVENT_GETBLOBSIZE, ( WPARAM )hdbe, 0 )) == -1 )
		return 1;

	dbei.pBlob = ( PBYTE )_alloca( dbei.cbBlob );
	if ( YAHOO_CallService( MS_DB_EVENT_GET, ( WPARAM )hdbe, ( LPARAM )&dbei ))
		return 1;

	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST )
		return 1;

	if ( strcmp( dbei.szModule, m_szModuleName ))
		return 1;

	HANDLE hContact;
	memcpy(&hContact,( char* )( dbei.pBlob + sizeof( DWORD ) ), sizeof(HANDLE)); 

	/* Need to remove the buddy from our Miranda Lists */
	DBVARIANT dbv;
	if (hContact != NULL && !DBGetContactSettingString( hContact, m_szModuleName, YAHOO_LOGINID, &dbv )){
		DebugLog("Accepting buddy:%s", dbv.pszVal);    
		accept(dbv.pszVal, GetWord(hContact, "yprotoid", 0));
		DBFreeVariant(&dbv);
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// AuthDeny - handles the unsuccessful authorization

int CYahooProto::AuthDeny( HANDLE hdbe, const char* reason )
{
	DebugLog("[YahooAuthDeny]");
	if ( !m_bLoggedIn )
		return 1;

	DBEVENTINFO dbei;
	memset( &dbei, 0, sizeof( dbei ));
	dbei.cbSize = sizeof( dbei );

	if (( dbei.cbBlob = YAHOO_CallService( MS_DB_EVENT_GETBLOBSIZE, ( WPARAM )hdbe, 0 )) == -1 ){
		DebugLog("[YahooAuthDeny] ERROR: Can't get blob size");
		return 1;
	}

	dbei.pBlob = ( PBYTE )alloca( dbei.cbBlob );
	if ( YAHOO_CallService( MS_DB_EVENT_GET, ( WPARAM )hdbe, ( LPARAM )&dbei )){
		DebugLog("YahooAuthDeny - Can't get db event!");
		return 1;
	}

	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST ){
		DebugLog("YahooAuthDeny - not Authorization event");
		return 1;
	}

	if ( strcmp( dbei.szModule, m_szModuleName )){
		DebugLog("YahooAuthDeny - wrong module?");
		return 1;
	}

	HANDLE hContact;
	memcpy(&hContact,( char* )( dbei.pBlob + sizeof( DWORD ) ), sizeof(HANDLE)); 

	/* Need to remove the buddy from our Miranda Lists */
	DBVARIANT dbv;
	if (hContact != NULL && !DBGetContactSettingString( hContact, m_szModuleName, YAHOO_LOGINID, &dbv )){
		DebugLog("Rejecting buddy:%s msg: %s", dbv.pszVal, reason);    
		reject(dbv.pszVal, GetWord(hContact, "yprotoid", 0), reason);
		DBFreeVariant(&dbv);
		YAHOO_CallService( MS_DB_CONTACT_DELETE, (WPARAM) hContact, 0);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSR_AUTH

int __cdecl CYahooProto::AuthRecv( HANDLE hContact, PROTORECVEVENT* pre )
{
	DebugLog("[YahooRecvAuth] ");
	DBDeleteContactSetting(hContact,"CList","Hidden");

	DBEVENTINFO dbei = { 0 };
	dbei.cbSize = sizeof(dbei);
	dbei.szModule = m_szModuleName;
	dbei.timestamp = pre->timestamp;
	dbei.flags = pre->flags & (PREF_CREATEREAD?DBEF_READ:0);
	dbei.eventType = EVENTTYPE_AUTHREQUEST;

	/* Just copy the Blob from PSR_AUTH event. */
	dbei.cbBlob = pre->lParam;
	dbei.pBlob = (PBYTE)pre->szMessage;

	CallService(MS_DB_EVENT_ADD,(WPARAM)NULL,(LPARAM)&dbei);

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AUTHREQUEST

int __cdecl CYahooProto::AuthRequest( HANDLE hContact, const char* msg )
{	
	DebugLog("[YahooSendAuthRequest]");
	
	if (hContact && m_bLoggedIn) {
		if (hContact) {
			DBVARIANT dbv;
			if (!DBGetContactSettingString(hContact, m_szModuleName, YAHOO_LOGINID, &dbv )) {
				DebugLog("Adding buddy:%s Auth:%s", dbv.pszVal, msg);
				AddBuddy( dbv.pszVal, GetWord(hContact, "yprotoid", 0), "miranda", msg );
				SetString(hContact, "YGroup", "miranda");
				DBFreeVariant( &dbv );
				
				return 0; // Success
			}
		}
	}
	
	return 1; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// ChangeInfo 

HANDLE __cdecl CYahooProto::ChangeInfo( int /*iInfoType*/, void* )
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
// FileAllow - starts a file transfer

HANDLE __cdecl CYahooProto::FileAllow( HANDLE /*hContact*/, HANDLE hTransfer, const char* szPath )
{
	y_filetransfer *ft = (y_filetransfer *)hTransfer;
	int len;

	DebugLog("[YahooFileAllow]");

	//LOG(LOG_INFO, "[%s] Requesting file from %s", ft->cookie, ft->user);
	ft->savepath = strdup( szPath );

	len = lstrlenA(ft->savepath) - 1;
	if (ft->savepath[len] == '\\')
		ft->savepath[len] = '\0';

	if (ft->y7) {
		DebugLog("[YahooFileAllow] We don't handle y7 stuff yet.");
		//void yahoo_ft7dc_accept(int id, const char *buddy, const char *ft_token);
		yahoo_ft7dc_accept(ft->id, ft->who, ft->ftoken);

		return hTransfer;
	}

	YForkThread(&CYahooProto::recv_filethread, ft);
	return hTransfer;
}

////////////////////////////////////////////////////////////////////////////////////////
// FileCancel - cancels a file transfer

int __cdecl CYahooProto::FileCancel( HANDLE /*hContact*/, HANDLE hTransfer )
{
	DebugLog("[YahooFileCancel]");

	y_filetransfer* ft = (y_filetransfer*)hTransfer;
	if ( ft->hWaitEvent != INVALID_HANDLE_VALUE )
		SetEvent( ft->hWaitEvent );

	ft->action = FILERESUME_CANCEL;
	ft->cancel = 1;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// FileDeny - denies a file transfer

int __cdecl CYahooProto::FileDeny( HANDLE /*hContact*/, HANDLE hTransfer, const char* )
{
	/* deny file receive request.. just ignore it! */
	y_filetransfer *ft = (y_filetransfer *)hTransfer;

	DebugLog("[YahooFileDeny]");

	if ( !m_bLoggedIn || ft == NULL ) {
		DebugLog("[YahooFileDeny] Not logged-in or some other error!");
		return 1;
	}

	if (ft->y7) {
		DebugLog("[YahooFileDeny] We don't handle y7 stuff yet.");
		//void yahoo_ft7dc_accept(int id, const char *buddy, const char *ft_token);
		yahoo_ft7dc_deny(ft->id, ft->who, ft->ftoken);
		return 0;
	}

	if (ft->ftoken != NULL) {
		struct yahoo_file_info *fi = (struct yahoo_file_info *)ft->files->data;

		DebugLog("[] DC Detected: Denying File Transfer!");
		yahoo_ftdc_deny(m_id, ft->who, fi->filename, ft->ftoken, 2);	
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// FileResume - processes file renaming etc

int __cdecl CYahooProto::FileResume( HANDLE hTransfer, int* action, const char** szFilename )
{
	y_filetransfer *ft = (y_filetransfer *)hTransfer;

	DebugLog("[YahooFileResume]");

	if ( !m_bLoggedIn || ft == NULL ) {
		DebugLog("[YahooFileResume] Not loggedin or some other error!");
		return 1;
	}

	ft->action = *action;

	DebugLog("[YahooFileResume] Action: %d", *action);

	if ( *action == FILERESUME_RENAME ) {
		struct yahoo_file_info *fi = (struct yahoo_file_info *)ft->files->data;

		DebugLog("[YahooFileResume] Renamed file!");
		FREE( fi->filename );
		fi->filename = strdup( *szFilename );
	}	

	SetEvent( ft->hWaitEvent );
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetCaps - return protocol capabilities bits

DWORD_PTR __cdecl CYahooProto::GetCaps( int type, HANDLE /*hContact*/ )
{
	int ret = 0;
	switch ( type ) {        
	case PFLAGNUM_1:
		ret = PF1_IM  | PF1_ADDED | PF1_AUTHREQ | PF1_MODEMSGRECV | PF1_MODEMSGSEND |  PF1_BASICSEARCH |
			PF1_EXTSEARCH | PF1_FILESEND  | PF1_FILERECV| PF1_VISLIST | PF1_SERVERCLIST ;
		break;

	case PFLAGNUM_2:
		ret = PF2_ONLINE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_ONTHEPHONE | 
			PF2_OUTTOLUNCH | PF2_INVISIBLE | PF2_LIGHTDND /*| PF2_HEAVYDND*/;
		break;

	case PFLAGNUM_3:
		ret = PF2_ONLINE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_ONTHEPHONE | 
			PF2_OUTTOLUNCH | PF2_LIGHTDND ;
		break;

	case PFLAGNUM_4:
		ret = PF4_FORCEAUTH | PF4_FORCEADDED | PF4_SUPPORTTYPING | PF4_SUPPORTIDLE
			|PF4_AVATARS | PF4_OFFLINEFILES | PF4_IMSENDUTF;
		break;
	case PFLAG_UNIQUEIDTEXT:
		ret = (DWORD_PTR) Translate("ID");
		break;
	case PFLAG_UNIQUEIDSETTING:
		ret = (DWORD_PTR) YAHOO_LOGINID;
		break;
	case PFLAG_MAXLENOFMESSAGE:
		ret = 800; /* STUPID YAHOO!!! */
		break;
	}
	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetIcon - loads an icon for the contact list

HICON __cdecl CYahooProto::GetIcon( int iconIndex )
{
	UINT id;

	switch( iconIndex & 0xFFFF ) {
		case PLI_PROTOCOL: id=IDI_YAHOO; break; // IDI_MAIN is the main icon for the protocol
		default: return NULL;	
	}
	return ( HICON )LoadImage(hInstance, MAKEINTRESOURCE(id), IMAGE_ICON,
		GetSystemMetrics( iconIndex & PLIF_SMALL ? SM_CXSMICON : SM_CXICON ),
		GetSystemMetrics( iconIndex & PLIF_SMALL ? SM_CYSMICON : SM_CYICON ),0);
}

////////////////////////////////////////////////////////////////////////////////////////
// GetInfo - retrieves a contact info

void __cdecl CYahooProto::get_info_thread(HANDLE hContact) 
{
	SleepEx(500, TRUE);
	ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

int __cdecl CYahooProto::GetInfo( HANDLE hContact, int /*infoType*/ )
{
	YForkThread(&CYahooProto::get_info_thread, hContact);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchByEmail - searches the contact by its e-mail

HANDLE __cdecl CYahooProto::SearchByEmail( const char* email )
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// SearchByName - searches the contact by its first or last name, or by a nickname

HANDLE __cdecl CYahooProto::SearchByName( const char* nick, const char* firstName, const char* lastName )
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvContacts

int __cdecl CYahooProto::RecvContacts( HANDLE /*hContact*/, PROTORECVEVENT* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvFile

int __cdecl CYahooProto::RecvFile( HANDLE hContact, PROTORECVFILE* evt )
{
	DBDeleteContactSetting(hContact, "CList", "Hidden");

	CCSDATA ccs = { hContact, PSR_FILE, 0, ( LPARAM )evt };
	return CallService( MS_PROTO_RECVFILE, 0, ( LPARAM )&ccs );
}

////////////////////////////////////////////////////////////////////////////////////////
// RecvUrl

int __cdecl CYahooProto::RecvUrl( HANDLE /*hContact*/, PROTORECVEVENT* )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendContacts

int __cdecl CYahooProto::SendContacts( HANDLE /*hContact*/, int /*flags*/, int /*nContacts*/, HANDLE* /*hContactsList*/ )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SendUrl

int __cdecl CYahooProto::SendUrl( HANDLE /*hContact*/, int /*flags*/, const char* /*url*/ )
{
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SetApparentMode - sets the visibility status

int __cdecl CYahooProto::SetApparentMode( HANDLE hContact, int mode )
{
	if (mode && mode != ID_STATUS_OFFLINE)
		return 1;

	int oldMode = DBGetContactSettingWord(hContact, m_szModuleName, "ApparentMode", 0);
	if (mode != oldMode)
		DBWriteContactSettingWord(hContact, m_szModuleName, "ApparentMode", (WORD)mode);
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SetStatus - sets the protocol status

int __cdecl CYahooProto::SetStatus( int iNewStatus )
{
	DebugLog("[SetStatus] New status %s", (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, iNewStatus, 0));
	if (iNewStatus == ID_STATUS_OFFLINE) {
		logout();
	}
	else if (!m_bLoggedIn) {
		DBVARIANT dbv;
		int err = 0;
		char errmsg[80];

		if (m_iStatus == ID_STATUS_CONNECTING)
			return 0;

		YAHOO_utils_logversion();

		/*
		* Load Yahoo ID from the database.
		*/
		if (!DBGetContactSettingString(NULL, m_szModuleName, YAHOO_LOGINID, &dbv)) {
			if (lstrlenA(dbv.pszVal) > 0) {
				lstrcpynA(m_yahoo_id, dbv.pszVal, 255);
			} else
				err++;
			DBFreeVariant(&dbv);
		} else {
			ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_BADUSERID);
			err++;
		}

		if (err) {
			lstrcpynA(errmsg, Translate("Please enter your yahoo id in Options/Network/Yahoo"), 80);
		} else {
			if (!DBGetContactSettingString(NULL, m_szModuleName, YAHOO_PASSWORD, &dbv)) {
				CallService(MS_DB_CRYPT_DECODESTRING, lstrlenA(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
				if (lstrlenA(dbv.pszVal) > 0) {
					lstrcpynA(m_password, dbv.pszVal, 255);
				} else
					err++;

				DBFreeVariant(&dbv);
			}  else  {
				ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD);
				err++;
			}

			if (err)
				lstrcpynA(errmsg, Translate("Please enter your yahoo password in Options/Network/Yahoo"), 80);
		}

		if (err != 0){
			BroadcastStatus(ID_STATUS_OFFLINE);

			ShowError(Translate("Yahoo Login Error"), errmsg);
			return 0;
		}

		if (iNewStatus == ID_STATUS_OFFLINE)
			iNewStatus = ID_STATUS_ONLINE;

		//DBWriteContactSettingWord(NULL, m_szModuleName, "StartupStatus", status);
		m_startStatus = iNewStatus;

		//reset the unread email count. We'll get a new packet since we are connecting.
		m_unreadMessages = 0;

		BroadcastStatus(ID_STATUS_CONNECTING);

		iNewStatus = (iNewStatus == ID_STATUS_INVISIBLE) ? YAHOO_STATUS_INVISIBLE: YAHOO_STATUS_AVAILABLE;
		YForkThread(&CYahooProto::server_main, (void *)iNewStatus);
	}
	else if (iNewStatus == ID_STATUS_INVISIBLE){ /* other normal away statuses are set via setaway */
		BroadcastStatus(iNewStatus);
		set_status(m_iStatus,NULL,(m_iStatus != ID_STATUS_ONLINE) ? 1 : 0);
	}
	else {
		/* clear out our message just in case, STUPID AA! */
		FREE(m_startMsg);

		/* now tell miranda that we are Online, don't tell Yahoo server yet though! */
		BroadcastStatus(iNewStatus);
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
// GetAwayMsg - returns a contact's away message

void __cdecl CYahooProto::get_status_thread(HANDLE hContact)
{
	int l;
	DBVARIANT dbv;
	char *gm = NULL, *sm = NULL, *fm;

	Sleep( 150 );

	/* Check Yahoo Games Message */
	if (! DBGetContactSettingString(( HANDLE )hContact, m_szModuleName, "YGMsg", &dbv )) {
		gm = strdup(dbv.pszVal);

		DBFreeVariant( &dbv );
	}

	if (! DBGetContactSettingString(hContact, "CList", "StatusMsg", &dbv )) {
		if (lstrlenA(dbv.pszVal) >= 1)
			sm = strdup(dbv.pszVal);

		DBFreeVariant( &dbv );
	} else {
		WORD status = DBGetContactSettingWord(hContact, m_szModuleName, "YStatus", YAHOO_STATUS_OFFLINE);
		sm = yahoo_status_code( yahoo_status( status ));
		if (sm) sm = strdup(sm); /* we need this to go global FREE later */
	}

	l = 0;
	if (gm)
		l += lstrlenA(gm) + 3;

	l += lstrlenA(sm) + 1;
	fm = (char *) malloc(l);

	fm[0] ='\0';
	if (gm && lstrlenA(gm) > 0) {
		/* BAH YAHOO SUCKS! WHAT A PAIN!
		find first carriage return add status message then add the rest */
		char *c = strchr(gm, '\r');

		if (c != NULL) {
			lstrcpynA(fm,gm, c - gm + 1);
			fm[c - gm + 1] = '\0';
		} else
			lstrcpyA(fm, gm);

		if (sm) {
			lstrcatA(fm, ": ");
			lstrcatA(fm, sm);
		}

		if (c != NULL)
			lstrcatA(fm, c);
	} else if (sm) {
		lstrcatA(fm, sm);
	}

	FREE(sm);

	SendBroadcast( hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, ( HANDLE )1, ( LPARAM ) fm );
}

HANDLE __cdecl CYahooProto::GetAwayMsg( HANDLE hContact )
{
	DebugLog("[YahooGetAwayMessage] ");

	if (hContact && m_bLoggedIn) {
		if (DBGetContactSettingWord(hContact, m_szModuleName, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE)
			return 0; /* user offline, what Status message? */

		YForkThread(&CYahooProto::get_status_thread, hContact);
		return (HANDLE)1; //Success		
	}

	return 0; // Failure
}

////////////////////////////////////////////////////////////////////////////////////////
// PSR_AWAYMSG

int __cdecl CYahooProto::RecvAwayMsg( HANDLE /*hContact*/, int /*statusMode*/, PROTORECVEVENT* )
{	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// PSS_AWAYMSG

int __cdecl CYahooProto::SendAwayMsg( HANDLE /*hContact*/, HANDLE /*hProcess*/, const char* )
{	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
// SetAwayMsg - sets the away status message

int __cdecl CYahooProto::SetAwayMsg( int status, const char* msg )
{
	char *c = (char *)msg;
	if (c != NULL) 
		if (*c == '\0')
			c = NULL;
		
	DebugLog("[YahooSetAwayMessage] Status: %s, Msg: %s",(char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, status, 0), (char*) c);
	
    if(!m_bLoggedIn){
		if (m_iStatus == ID_STATUS_OFFLINE) {
			DebugLog("[YahooSetAwayMessage] WARNING: WE ARE OFFLINE!"); 
			return 1;
		} else {
			if (m_startMsg) free(m_startMsg);
			
			if (c != NULL) 
				m_startMsg = strdup(c);
			else
				m_startMsg = NULL;
			
			return 0;
		}
	}              
	
	/* need to tell ALL plugins that we are changing status */
	BroadcastStatus(status);
	
	if (m_startMsg) free(m_startMsg);
	
	/* now decide what we tell the server */
	if (c != 0) {
		m_startMsg = strdup(c);
		if(status == ID_STATUS_ONLINE) {
			set_status(YAHOO_CUSTOM_STATUS, c, 0);
		} else if(status != ID_STATUS_INVISIBLE){ 
			set_status(YAHOO_CUSTOM_STATUS, c, 1);
		}
    } else {
		set_status(status, NULL, 0);
		m_startMsg = NULL;
	}
	
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// UserIsTyping - sends a UTN notification

int __cdecl CYahooProto::UserIsTyping( HANDLE hContact, int type )
{
	if (!m_bLoggedIn)
		return 0;

	char *szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
	if (szProto==NULL || strcmp(szProto, m_szModuleName))
		return 0;

	DBVARIANT dbv;
	if (!DBGetContactSettingString(hContact, m_szModuleName, YAHOO_LOGINID, &dbv)) {
		if (type == PROTOTYPE_SELFTYPING_OFF || type == PROTOTYPE_SELFTYPING_ON) {
			sendtyping(dbv.pszVal, GetWord(hContact, "yprotoid", 0), type == PROTOTYPE_SELFTYPING_ON?1:0);
		}
		DBFreeVariant(&dbv);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnEvent - maintain protocol events

int __cdecl CYahooProto::OnEvent( PROTOEVENTTYPE eventType, WPARAM wParam, LPARAM lParam )
{
	switch( eventType ) {
		case EV_PROTO_ONLOAD:    return OnModulesLoadedEx( 0, 0 );
		//case EV_PROTO_ONEXIT:    return OnPreShutdown( 0, 0 );
		case EV_PROTO_ONOPTIONS: return OnOptionsInit( wParam, lParam );
	}	
	return 1;
}
