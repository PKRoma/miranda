/*
 * $Id$
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

#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_langpack.h>
#include <m_skin.h>
#include <m_utils.h>
#include <m_options.h>
#include <m_popup.h>
#include <m_idle.h>

#include "avatar.h"
#include "resource.h"
#include "file_transfer.h"
#include "im.h"
#include "search.h"

void yahoo_logoff_buddies()
{
		//set all contacts to 'offline'
		HANDLE hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		while ( hContact != NULL ) 
		{
			if ( !lstrcmpA( yahooProtocolName, ( char* )YAHOO_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact,0 ))) {
				YAHOO_SetWord( hContact, "Status", ID_STATUS_OFFLINE );
				DBWriteContactSettingDword(hContact, yahooProtocolName, "IdleTS", 0);
				DBWriteContactSettingDword(hContact, yahooProtocolName, "PictLastCheck", 0);
				DBWriteContactSettingDword(hContact, yahooProtocolName, "PictLoading", 0);
				DBDeleteContactSetting(hContact, "CList", "StatusMsg" );
				DBDeleteContactSetting(hContact, yahooProtocolName, "YMsg" );
				DBDeleteContactSetting(hContact, yahooProtocolName, "YGMsg" );
				//DBDeleteContactSetting(hContact, yahooProtocolName, "MirVer" );
			}

			hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDNEXT,( WPARAM )hContact, 0 );
		}	
}


//=======================================================
//GetCaps
//=======================================================
int GetCaps(WPARAM wParam,LPARAM lParam)
{
    int ret = 0;
    switch (wParam) {        
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
            ret = (int) Translate("ID");
            break;
        case PFLAG_UNIQUEIDSETTING:
            ret = (int) YAHOO_LOGINID;
            break;
        case PFLAG_MAXLENOFMESSAGE:
            ret = 800; /* STUPID YAHOO!!! */
            break;
	
    }
    return ret;
		
}

//=======================================================
//GetName
//=======================================================
int GetName(WPARAM wParam,LPARAM lParam)
{
	lstrcpynA((char*)lParam, yahooProtocolName, wParam);
	return 0;
}

//=======================================================
//YahooLoadIcon
//=======================================================
int YahooLoadIcon(WPARAM wParam,LPARAM lParam)
{
	UINT id;

	switch(wParam&0xFFFF) {
		case PLI_PROTOCOL: id=IDI_YAHOO; break; // IDI_MAIN is the main icon for the protocol
		default: return (int)(HICON)NULL;	
	}
	return (int)LoadImage(hinstance,MAKEINTRESOURCE(id),IMAGE_ICON,GetSystemMetrics(wParam&PLIF_SMALL?SM_CXSMICON:SM_CXICON),GetSystemMetrics(wParam&PLIF_SMALL?SM_CYSMICON:SM_CYICON),0);
}

//=======================================================
//GetStatus
//=======================================================
int GetStatus(WPARAM wParam,LPARAM lParam)
{
	return yahooStatus;
}

extern yahoo_local_account *ylad;

int    gStartStatus = ID_STATUS_ONLINE;
char  *szStartMsg = NULL;

//=======================================================
//SetStatus
//=======================================================
int SetStatus(WPARAM wParam,LPARAM lParam)
{
    int status = (int) wParam;

    //if (yahooStatus == status)
    //    return 0;
        
    YAHOO_DebugLog("[SetStatus] New status %s", (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, status, 0));
    if (status == ID_STATUS_OFFLINE) {
		yahoo_logout();
		
        /*yahoo_util_broadcaststatus(ID_STATUS_OFFLINE);
        yahoo_logoff_buddies();*/
    }
    else if (!yahooLoggedIn) {
		DBVARIANT dbv;
		int err = 0;
		char errmsg[80];
		
        if (yahooStatus == ID_STATUS_CONNECTING)
			return 0;

		YAHOO_utils_logversion();
		
		/*
		 * Load Yahoo ID form the database.
		 */
		if (!DBGetContactSettingString(NULL, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
			if (lstrlenA(dbv.pszVal) > 0) {
				lstrcpynA(ylad->yahoo_id, dbv.pszVal, 255);
			} else
				err++;
			DBFreeVariant(&dbv);
		} else {
			ProtoBroadcastAck(yahooProtocolName, NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_BADUSERID);
			err++;
		}
		
		if (err) {
			lstrcpynA(errmsg, Translate("Please enter your yahoo id in Options/Network/Yahoo"), 80);
		} else {
			if (!DBGetContactSettingString(NULL, yahooProtocolName, YAHOO_PASSWORD, &dbv)) {
				CallService(MS_DB_CRYPT_DECODESTRING, lstrlenA(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
				if (lstrlenA(dbv.pszVal) > 0) {
					lstrcpynA(ylad->password, dbv.pszVal, 255);
				} else
					err++;
				
				DBFreeVariant(&dbv);
			}  else  {
				ProtoBroadcastAck(yahooProtocolName, NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD);
				err++;
			}
			
			if (err) {
				lstrcpynA(errmsg, Translate("Please enter your yahoo password in Options/Network/Yahoo"), 80);
			}
		}

		if (err != 0){
			yahoo_util_broadcaststatus(ID_STATUS_OFFLINE);
        
			YAHOO_ShowError(Translate("Yahoo Login Error"), errmsg);
			return 0;
		}

		if (status == ID_STATUS_OFFLINE)
			status = ID_STATUS_ONLINE;
		
		//DBWriteContactSettingWord(NULL, yahooProtocolName, "StartupStatus", status);
		gStartStatus = status;

		//reset the unread email count. We'll get a new packet since we are connecting.
		mUnreadMessages = 0;

		yahoo_util_broadcaststatus(ID_STATUS_CONNECTING);
		
		status = (status == ID_STATUS_INVISIBLE) ? YAHOO_STATUS_INVISIBLE: YAHOO_STATUS_AVAILABLE;
        mir_forkthread(yahoo_server_main,  (void *) status );

		//start_timer();
    } else if (status == ID_STATUS_INVISIBLE){ /* other normal away statuses are set via setaway */
        yahoo_util_broadcaststatus(status);
		yahoo_set_status(yahooStatus,NULL,(yahooStatus != ID_STATUS_ONLINE) ? 1 : 0);
    } else {
		/* clear out our message just in case, STUPID AA! */
		FREE(szStartMsg);
			
		/* now tell miranda that we are Online, don't tell Yahoo server yet though! */
		yahoo_util_broadcaststatus(status);
	}
	
    return 0;
}

//=======================================================
//Broadcast the user status
//=======================================================
void yahoo_util_broadcaststatus(int s)
{
	int oldStatus = yahooStatus;
	if (oldStatus == s)
		return;

	//yahooStatus = s;
	switch (s) {
	case ID_STATUS_OFFLINE:
	case ID_STATUS_CONNECTING:
	case ID_STATUS_ONLINE:
	case ID_STATUS_AWAY:
	case ID_STATUS_NA:
	case ID_STATUS_OCCUPIED:
	case ID_STATUS_ONTHEPHONE:
	case ID_STATUS_OUTTOLUNCH:
	case ID_STATUS_INVISIBLE:
		yahooStatus = s;
		break;
	case ID_STATUS_DND:
		yahooStatus = ID_STATUS_OCCUPIED;
		break;
	default:
		yahooStatus = ID_STATUS_ONLINE;
	}

	YAHOO_DebugLog("[yahoo_util_broadcaststatus] Old Status: %s (%d), New Status: %s (%d)",
		NEWSTR_ALLOCA((char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, oldStatus, 0)), oldStatus,
		NEWSTR_ALLOCA((char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, yahooStatus, 0)), yahooStatus);	
	ProtoBroadcastAck(yahooProtocolName, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE) oldStatus, (LPARAM)yahooStatus);
}

static int YahooContactDeleted( WPARAM wParam, LPARAM lParam )
{
	char* szProto;
	DBVARIANT dbv;
	HANDLE hContact = (HANDLE) wParam;
	
	YAHOO_DebugLog("[YahooContactDeleted]");
	
	if ( !yahooLoggedIn )  {//should never happen for Yahoo contacts
		YAHOO_DebugLog("[YahooContactDeleted] We are not Logged On!!!");
		return 0;
	}

	szProto = ( char* )YAHOO_CallService( MS_PROTO_GETCONTACTBASEPROTO, wParam, 0 );
	if ( szProto == NULL || lstrcmpA( szProto, yahooProtocolName ))  {
		YAHOO_DebugLog("[YahooContactDeleted] Not a Yahoo Contact!!!");
		return 0;
	}

	// he is not a permanent contact!
	if (DBGetContactSettingByte(hContact, "CList", "NotOnList", 0) != 0) {
		YAHOO_DebugLog("[YahooContactDeleted] Not a permanent buddy!!!");
		return 0;
	}
	
	if ( !DBGetContactSettingString(hContact, yahooProtocolName, YAHOO_LOGINID, &dbv )){
		YAHOO_DebugLog("[YahooContactDeleted] Removing %s", dbv.pszVal);
		YAHOO_remove_buddy(dbv.pszVal, YAHOO_GetWord(hContact, "yprotoid", 0));
		
		DBFreeVariant( &dbv );
	} else {
		YAHOO_DebugLog("[YahooContactDeleted] Can't retrieve contact Yahoo ID");
	}
	return 0;
}

int YahooSendAuthRequest(WPARAM wParam,LPARAM lParam)
{
	YAHOO_DebugLog("[YahooSendAuthRequest]");
	
	if (lParam && yahooLoggedIn){
		
		CCSDATA *ccs = (CCSDATA *)lParam;
		
		if (ccs->hContact){
			
			DBVARIANT dbv;
			
			if (!DBGetContactSettingString(ccs->hContact, yahooProtocolName, YAHOO_LOGINID, &dbv ))			{
				char *c = NULL;
				
				if ( ccs->lParam )
					c = (char *)ccs->lParam;
				
				YAHOO_DebugLog("Adding buddy:%s Auth:%s", dbv.pszVal, c);
				YAHOO_add_buddy(dbv.pszVal, YAHOO_GetWord(ccs->hContact, "yprotoid", 0), "miranda", c);
				YAHOO_SetString(ccs->hContact, "YGroup", "miranda");
				DBFreeVariant( &dbv );
				
				return 0; // Success
				
			}
			
		}
		
	}
	
	return 1; // Failure
}

int YahooAddToList(WPARAM wParam,LPARAM lParam)
{
    PROTOSEARCHRESULT *psr=(PROTOSEARCHRESULT*)lParam;
	HANDLE hContact;
	int protocol;
	
	YAHOO_DebugLog("[YahooAddToList] Flags: %d", wParam);
	
	if (!yahooLoggedIn){
		YAHOO_DebugLog("[YahooAddToList] WARNING: WE ARE OFFLINE!");
		return 0;
	}
	
	if (psr == NULL || psr->cbSize != sizeof( PROTOSEARCHRESULT )) {
		YAHOO_DebugLog("[YahooAddToList] Empty data passed?");
		return 0;
	}
	
	hContact = getbuddyH(psr->nick);
	if (hContact != NULL) {
		if (DBGetContactSettingByte(hContact, "CList", "NotOnList", 0)) {
			YAHOO_DebugLog("[YahooAddToList] Temporary Buddy:%s already on our buddy list", psr->nick);
			//return 0;
		} else {
			YAHOO_DebugLog("[YahooAddToList] Buddy:%s already on our buddy list", psr->nick);
			return 0;
		}
	} else if (wParam & PALF_TEMPORARY ) { /* not on our list */
		YAHOO_DebugLog("[YahooAddToList] Adding Temporary Buddy:%s ", psr->nick);
	}
	
	protocol = psr->reserved[0];
	YAHOO_DebugLog("Adding buddy:%s", psr->nick);
    return (int)add_buddy(psr->nick, psr->nick, protocol, wParam);
}

int YahooAddToListByEvent(WPARAM wParam,LPARAM lParam)
{
    DBEVENTINFO dbei;
	HANDLE hContact;
    
    YAHOO_DebugLog("[YahooAddToListByEvent]");
	if ( !yahooLoggedIn )
		return 0;
	
	memset( &dbei, 0, sizeof( dbei ));
	dbei.cbSize = sizeof( dbei );

	if (( dbei.cbBlob = YAHOO_CallService( MS_DB_EVENT_GETBLOBSIZE, lParam, 0 )) == -1 ) {
		YAHOO_DebugLog("[YahooAddToListByEvent] ERROR: Can't get blob size.");
		return 0;
	}

	YAHOO_DebugLog("[YahooAddToListByEvent] Got blob size: %lu", dbei.cbBlob);
	dbei.pBlob = ( PBYTE )_alloca( dbei.cbBlob );
	if ( YAHOO_CallService( MS_DB_EVENT_GET, lParam, ( LPARAM )&dbei )) {
		YAHOO_DebugLog("[YahooAddToListByEvent] ERROR: Can't get event.");
		return 0;
	}

	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST ) {
		YAHOO_DebugLog("[YahooAddToListByEvent] ERROR: Not an authorization request.");
		return 0;
	}

	if ( strcmp( dbei.szModule, yahooProtocolName )) {
		YAHOO_DebugLog("[YahooAddToListByEvent] ERROR: Not Yahoo protocol.");
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
		YAHOO_DebugLog("Temp Buddy found at: %p ", hContact);
	} else
		YAHOO_DebugLog("hContact NULL???");
	
	return (int)hContact;
}

int YahooAuthAllow(WPARAM wParam,LPARAM lParam)
{
    DBEVENTINFO dbei;
	HANDLE hContact;
	DBVARIANT dbv;
    
    YAHOO_DebugLog("[YahooAuthAllow]");
	if ( !yahooLoggedIn ) {
		YAHOO_DebugLog("[YahooAuthAllow] Not Logged In!");
		return 1;
	}

	memset( &dbei, 0, sizeof( dbei ));
	dbei.cbSize = sizeof( dbei );

	if (( dbei.cbBlob = YAHOO_CallService( MS_DB_EVENT_GETBLOBSIZE, wParam, 0 )) == -1 )
		return 1;

	dbei.pBlob = ( PBYTE )_alloca( dbei.cbBlob );
	if ( YAHOO_CallService( MS_DB_EVENT_GET, wParam, ( LPARAM )&dbei ))
		return 1;

	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST )
		return 1;

	if ( strcmp( dbei.szModule, yahooProtocolName ))
		return 1;

	memcpy(&hContact,( char* )( dbei.pBlob + sizeof( DWORD ) ), sizeof(HANDLE)); 
    
    /* Need to remove the buddy from our Miranda Lists */
    if (hContact != NULL && !DBGetContactSettingString( hContact, yahooProtocolName, YAHOO_LOGINID, &dbv )){
		YAHOO_DebugLog("Accepting buddy:%s", dbv.pszVal);    
	    YAHOO_accept(dbv.pszVal, YAHOO_GetWord(hContact, "yprotoid", 0));
		DBFreeVariant(&dbv);
	}

	return 0;
}

int YahooAuthDeny(WPARAM wParam,LPARAM lParam)
{
    DBEVENTINFO dbei;
	DBVARIANT dbv;
	HANDLE hContact;
	char* reason= (char*)lParam;
	
    YAHOO_DebugLog("[YahooAuthDeny]");
	if ( !yahooLoggedIn )
		return 1;

	memset( &dbei, 0, sizeof( dbei ));
	dbei.cbSize = sizeof( dbei );

	if (( dbei.cbBlob = YAHOO_CallService( MS_DB_EVENT_GETBLOBSIZE, wParam, 0 )) == -1 ){
	    YAHOO_DebugLog("[YahooAuthDeny] ERROR: Can't get blob size");
		return 1;
	}

	dbei.pBlob = ( PBYTE )alloca( dbei.cbBlob );
	if ( YAHOO_CallService( MS_DB_EVENT_GET, wParam, ( LPARAM )&dbei )){
	    YAHOO_DebugLog("YahooAuthDeny - Can't get db event!");
		return 1;
	}

	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST ){
	    YAHOO_DebugLog("YahooAuthDeny - not Authorization event");
		return 1;
	}

	if ( strcmp( dbei.szModule, yahooProtocolName )){
	    YAHOO_DebugLog("YahooAuthDeny - wrong module?");
		return 1;
	}

    memcpy(&hContact,( char* )( dbei.pBlob + sizeof( DWORD ) ), sizeof(HANDLE)); 
    
    /* Need to remove the buddy from our Miranda Lists */
    if (hContact != NULL && !DBGetContactSettingString( hContact, yahooProtocolName, YAHOO_LOGINID, &dbv )){
		YAHOO_DebugLog("Rejecting buddy:%s msg: %s", dbv.pszVal, reason);    
	    YAHOO_reject(dbv.pszVal, YAHOO_GetWord(hContact, "yprotoid", 0), reason);
		DBFreeVariant(&dbv);
		YAHOO_CallService( MS_DB_CONTACT_DELETE, (WPARAM) hContact, 0);
	}
	return 0;
}

int YahooRecvAuth(WPARAM wParam,LPARAM lParam)
{
	DBEVENTINFO dbei = { 0 };
	CCSDATA *ccs=(CCSDATA*)lParam;
	PROTORECVEVENT *pre=(PROTORECVEVENT*)ccs->lParam;
	
    YAHOO_DebugLog("[YahooRecvAuth] ");
	DBDeleteContactSetting(ccs->hContact,"CList","Hidden");

	dbei.cbSize = sizeof(dbei);
	dbei.szModule = yahooProtocolName;
	dbei.timestamp = pre->timestamp;
	dbei.flags = pre->flags & (PREF_CREATEREAD?DBEF_READ:0);
	dbei.eventType = EVENTTYPE_AUTHREQUEST;
	
	/* Just copy the Blob from PSR_AUTH event. */
	dbei.cbBlob = pre->lParam;
	dbei.pBlob = (PBYTE)pre->szMessage;
	
	CallService(MS_DB_EVENT_ADD,(WPARAM)NULL,(LPARAM)&dbei);

	return 0;
}

static void __cdecl yahoo_get_statusthread(HANDLE hContact)
{
	int l;
	DBVARIANT dbv;
	char *gm = NULL, *sm = NULL, *fm;
	
	Sleep( 150 );
	
	/* Check Yahoo Games Message */
	if (! DBGetContactSettingString(( HANDLE )hContact, yahooProtocolName, "YGMsg", &dbv )) {
		gm = strdup(dbv.pszVal);
		
		DBFreeVariant( &dbv );
	}
	
	if (! DBGetContactSettingString(hContact, "CList", "StatusMsg", &dbv )) {
		if (lstrlenA(dbv.pszVal) >= 1)
			sm = strdup(dbv.pszVal);
		
		DBFreeVariant( &dbv );
	} else {
		WORD status = DBGetContactSettingWord(hContact, yahooProtocolName, "YStatus", YAHOO_STATUS_OFFLINE);
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
	
	YAHOO_SendBroadcast( hContact, ACKTYPE_AWAYMSG, ACKRESULT_SUCCESS, ( HANDLE )1, ( LPARAM ) fm );
}

//=======================================================
//Handle away message
//=======================================================
int YahooGetAwayMessage(WPARAM wParam,LPARAM lParam)
{
    YAHOO_DebugLog("[YahooGetAwayMessage] ");
	if (lParam && yahooLoggedIn) {
	    CCSDATA *ccs = (CCSDATA *) lParam;
		
		if (DBGetContactSettingWord(( HANDLE )ccs->hContact, 
									yahooProtocolName, 
									"Status", 
									ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE)
			return 0; /* user offline, what Status message? */

        mir_forkthread(yahoo_get_statusthread, ccs->hContact);
        return 1; //Success		
	}
	
	return 0; // Failure
}

//=======================================================
//SetStatusMessage
//=======================================================
int YahooSetAwayMessage(WPARAM wParam, LPARAM lParam)
{
	char *c;
	
	c = (char *) lParam;
	if (c != NULL) 
		if (*c == '\0')
			c = NULL;
		
	YAHOO_DebugLog("[YahooSetAwayMessage] Status: %s, Msg: %s",(char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, wParam, 0), (char*) c);
	
    if(!yahooLoggedIn){
		if (yahooStatus == ID_STATUS_OFFLINE) {
			YAHOO_DebugLog("[YahooSetAwayMessage] WARNING: WE ARE OFFLINE!"); 
			return 1;
		} else {
			if (szStartMsg) free(szStartMsg);
			
			if (c != NULL) 
				szStartMsg = strdup(c);
			else
				szStartMsg = NULL;
			
			return 0;
		}
	}              
	
	/* need to tell ALL plugins that we are changing status */
	yahoo_util_broadcaststatus(wParam);
	
	if (szStartMsg) free(szStartMsg);
	
	/* now decide what we tell the server */
	if (c != 0) {
		szStartMsg = strdup(c);
		if(wParam == ID_STATUS_ONLINE) {
			yahoo_set_status(YAHOO_CUSTOM_STATUS, c, 0);
		} else if(wParam != ID_STATUS_INVISIBLE){ 
			yahoo_set_status(YAHOO_CUSTOM_STATUS, c, 1);
		}
    } else {
		yahoo_set_status(wParam, NULL, 0);
		szStartMsg = NULL;
	}
	
	
	return 0;
}

static void __cdecl yahoo_get_infothread(HANDLE hContact) 
{
	SleepEx(500, TRUE);
    ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

int YahooGetInfo(WPARAM wParam,LPARAM lParam)
{
	CCSDATA *ccs = (CCSDATA *) lParam;
	mir_forkthread(yahoo_get_infothread, ccs->hContact);
	return 0;
}

//=======================================================
//Custom status message windows handling
//=======================================================
static BOOL CALLBACK DlgProcSetCustStat(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
//    char		str[ 4096 ];
	DBVARIANT dbv;

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault( hwndDlg );
			SendMessage( hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIconEx( "yahoo" ) );

		    if ( !DBGetContactSettingString( NULL, yahooProtocolName, YAHOO_CUSTSTATDB, &dbv ))
				    {
				    SetDlgItemTextA( hwndDlg, IDC_CUSTSTAT, dbv. pszVal );
					
					EnableWindow( GetDlgItem( hwndDlg, IDOK ), lstrlenA(dbv.pszVal) > 0 );
				    DBFreeVariant( &dbv );
				    }
		    else {
                    SetDlgItemTextA( hwndDlg, IDC_CUSTSTAT, "" );
					EnableWindow( GetDlgItem( hwndDlg, IDOK ), FALSE );
			}


            CheckDlgButton( hwndDlg, IDC_CUSTSTATBUSY,  YAHOO_GetByte( "BusyCustStat", 0 ));
			return TRUE;
		}
		case WM_COMMAND:
			switch(wParam) 
			{
				case IDOK:
					{
                        char str[ 255 + 1 ];
						
						/* Get String from dialog */
						GetDlgItemTextA( hwndDlg, IDC_CUSTSTAT, str, sizeof( str ));
						
						/* Save it for later use */
						YAHOO_SetString( NULL, YAHOO_CUSTSTATDB, str );
                        YAHOO_SetByte("BusyCustStat", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_CUSTSTATBUSY ));
						
						/* set for Idle/AA */
						if (szStartMsg) free(szStartMsg);
						szStartMsg = strdup(str);
						
						/* notify Server about status change */
						yahoo_set_status(YAHOO_CUSTOM_STATUS, str, ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_CUSTSTATBUSY ));
						
						/* change local/miranda status */
                        yahoo_util_broadcaststatus(( BYTE )IsDlgButtonChecked( hwndDlg, IDC_CUSTSTATBUSY ) ? 
													ID_STATUS_AWAY : ID_STATUS_ONLINE);
					}
				case IDCANCEL:
 					DestroyWindow( hwndDlg );
					break;
			}
			
			if ( HIWORD( wParam ) == EN_CHANGE && ( HWND )lParam == GetFocus()) {
				if (LOWORD( wParam ) == IDC_CUSTSTAT) {
					char str[ 255 + 1 ];
					
					BOOL toSet;
					
					toSet = GetDlgItemTextA( hwndDlg, IDC_CUSTSTAT, str, sizeof( str )) != 0;
					
					EnableWindow( GetDlgItem( hwndDlg, IDOK ), toSet );
				}			
			}
			break; /* case WM_COMMAND */

		case WM_CLOSE:
			DestroyWindow( hwndDlg );
			break;
	}
	return FALSE;
}

//=======================================================
//Show custom status windows
//=======================================================
static int SetCustomStatCommand( WPARAM wParam, LPARAM lParam )
{
	HWND hwndSetCustomStatus;
	
	if ( !yahooLoggedIn ) {
		YAHOO_shownotification(Translate("Yahoo Error"), Translate("You need to be connected to set the custom message"), NIIF_ERROR);
		return 0;
	}
	
	hwndSetCustomStatus = CreateDialog(hinstance, MAKEINTRESOURCE( IDD_SETCUSTSTAT ), NULL, DlgProcSetCustStat );
	
	SetForegroundWindow( hwndSetCustomStatus );
	SetFocus( hwndSetCustomStatus );
 	ShowWindow( hwndSetCustomStatus, SW_SHOW );
	return 0;
}

//=======================================================
//Open URL
//=======================================================
void YahooOpenURL(const char *url, int autoLogin)
{
	char tUrl[ 4096 ];

	if (autoLogin && YAHOO_GetByte( "MailAutoLogin", 0 ) && yahooLoggedIn) {
		int   id = 1;
		char  *y, *t, *u;
		
		y = yahoo_urlencode(yahoo_get_cookie(id, "y"));
		t = yahoo_urlencode(yahoo_get_cookie(id, "t"));
		u = yahoo_urlencode(url);
		_snprintf( tUrl, sizeof( tUrl ), 
				"http://msg.edit.yahoo.com/config/reset_cookies?&.y=Y=%s&.t=T=%s&.ver=2&.done=http%%3a//us.rd.yahoo.com/messenger/client/%%3f%s",
				y, t, u);
				
		FREE(y);
		FREE(t);
		FREE(u);
	} else {
		_snprintf( tUrl, sizeof( tUrl ), url );
	}

	YAHOO_DebugLog("[YahooOpenURL] url: %s Final URL: %s", url, tUrl);
	
	CallService( MS_UTILS_OPENURL, TRUE, ( LPARAM )tUrl );    
}

//=======================================================
//Show buddy profile
//=======================================================
static int YahooShowProfileCommand( WPARAM wParam, LPARAM lParam )
{
	char tUrl[ 4096 ];
	DBVARIANT dbv;

	if ( DBGetContactSettingString(( HANDLE )wParam, yahooProtocolName, "yahoo_id", &dbv ))
		return 0;
		
	_snprintf( tUrl, sizeof( tUrl ), "http://profiles.yahoo.com/%s", dbv.pszVal  );
	DBFreeVariant( &dbv );
	
	YahooOpenURL(tUrl, 0);
	return 0;
}

static int YahooEditMyProfile( WPARAM wParam, LPARAM lParam )
{
	YahooOpenURL("http://edit.yahoo.com/config/eval_profile", 1);

	return 0;
}

//=======================================================
//Show My profile
//=======================================================
static int YahooShowMyProfileCommand( WPARAM wParam, LPARAM lParam )
{
	char tUrl[ 4096 ];
	DBVARIANT dbv;

	DBGetContactSettingString( NULL, yahooProtocolName, YAHOO_LOGINID, &dbv );
		
	_snprintf( tUrl, sizeof( tUrl ), "http://profiles.yahoo.com/%s", dbv.pszVal  );
	DBFreeVariant( &dbv );

	YahooOpenURL(tUrl, 0);
	
	return 0;
}

//=======================================================
//Show Goto mailbox
//=======================================================
int YahooGotoMailboxCommand( WPARAM wParam, LPARAM lParam )
{
	if (YAHOO_GetByte( "YahooJapan", 0 ))
		YahooOpenURL("http://mail.yahoo.co.jp/", 1);
	else
		YahooOpenURL("http://mail.yahoo.com/", 1);
	
	return 0;
}

static int YahooABCommand( WPARAM wParam, LPARAM lParam )
{
	YahooOpenURL("http://address.yahoo.com/yab/", 1);

	return 0;
}

static int YahooCalendarCommand( WPARAM wParam, LPARAM lParam )
{
	YahooOpenURL("http://calendar.yahoo.com/", 1);		
	
	return 0;
}

//=======================================================
//Refresh Yahoo
//=======================================================
static int YahooRefreshCommand( WPARAM wParam, LPARAM lParam )
{
	if ( !yahooLoggedIn ){
		YAHOO_shownotification(Translate("Yahoo Error"), Translate("You need to be connected to refresh your buddy list"), NIIF_ERROR);
		return 0;
	}

	YAHOO_refresh();
	return 0;
}

int YAHOOSendTyping(WPARAM wParam, LPARAM lParam)
{
	DBVARIANT dbv;
	char *szProto;
	HANDLE hContact = (HANDLE)wParam;
	int state = (int)lParam;

	if (!yahooLoggedIn)
        return 0;
	
	szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
    if (szProto==NULL || strcmp(szProto, yahooProtocolName)) return 0;
	if (!DBGetContactSettingString(hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
		if (state==PROTOTYPE_SELFTYPING_OFF || state==PROTOTYPE_SELFTYPING_ON) {
			YAHOO_sendtyping(dbv.pszVal, YAHOO_GetWord(hContact, "yprotoid", 0), state == PROTOTYPE_SELFTYPING_ON?1:0);
		}
		DBFreeVariant(&dbv);
	}
	return 0;
}

int YahooIdleEvent(WPARAM wParam, LPARAM lParam)
{
	BOOL bIdle = (lParam & IDF_ISIDLE);
	
	YAHOO_DebugLog("[YAHOO_IDLE_EVENT] Idle: %s", bIdle ?"Yes":"No");
	
	if ( lParam & IDF_PRIVACY ) 
		return 0; /* we support Privacy settings */

	if (yahooLoggedIn) {
		/* set me to idle or back */
		if (yahooStatus == ID_STATUS_INVISIBLE)
			YAHOO_DebugLog("[YAHOO_IDLE_EVENT] WARNING: INVISIBLE! Don't change my status!");
		else
			yahoo_set_status(yahooStatus,szStartMsg,(bIdle) ? 2 : (yahooStatus == ID_STATUS_ONLINE) ? 0 : 1);
	} else {
		YAHOO_DebugLog("[YAHOO_IDLE_EVENT] WARNING: NOT LOGGED IN???");
	}
	
	return 0;
}

int YahooSetApparentMode(WPARAM wParam, LPARAM lParam)
{
    int oldMode;
    CCSDATA *ccs = (CCSDATA *) lParam;

    if (ccs->wParam && ccs->wParam != ID_STATUS_OFFLINE)
        return 1;
	
    oldMode = DBGetContactSettingWord(ccs->hContact, yahooProtocolName, "ApparentMode", 0);
	
    if ((int) ccs->wParam == oldMode)
        return 1;
	
    DBWriteContactSettingWord(ccs->hContact, yahooProtocolName, "ApparentMode", (WORD) ccs->wParam);
    return 1;
}

int YahooGetUnreadEmailCount(WPARAM wParam, LPARAM lParam)
{
    if ( !yahooLoggedIn )
        return 0;
	
    return mUnreadMessages;
}

extern HANDLE   hHookContactDeleted;
extern HANDLE   hHookIdle;

void YahooMenuInit( void )
{
	char servicefunction[ 100 ];
	char* tDest;
	CLISTMENUITEM mi;
	
	lstrcpyA( servicefunction, yahooProtocolName );
	tDest = servicefunction + lstrlenA( servicefunction );
	
	// Show custom status menu    
	lstrcpyA( tDest, YAHOO_SET_CUST_STAT );
	CreateServiceFunction( servicefunction, SetCustomStatCommand );
	memset( &mi, 0, sizeof( mi ));
	mi.pszPopupName = yahooProtocolName;
	mi.cbSize = sizeof( mi );
	mi.popupPosition = 500090000;
	mi.position = 500090000;
	mi.hIcon = LoadIconEx( "set_status" );
	mi.pszName = LPGEN( "Set &Custom Status" );
	mi.pszService = servicefunction;
	YahooMenuItems [ 0 ] = ( HANDLE )CallService( MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi );

	// Edit My profile
	lstrcpyA( tDest, YAHOO_EDIT_MY_PROFILE );
	CreateServiceFunction( servicefunction, YahooEditMyProfile );
	mi.position = 500090005;
	mi.hIcon = LoadIconEx( "profile" );
	mi.pszName = LPGEN( "&Edit My Profile" );
	mi.pszService = servicefunction;
	YahooMenuItems [ 1 ] = ( HANDLE )CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );


	// Show My profile
	lstrcpyA( tDest, YAHOO_SHOW_MY_PROFILE );
	CreateServiceFunction( servicefunction, YahooShowMyProfileCommand );
	mi.position = 500090005;
	mi.hIcon = LoadIconEx( "profile" );
	mi.pszName = LPGEN( "&My Profile" );
	mi.pszService = servicefunction;
	YahooMenuItems [ 1 ] = ( HANDLE )CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );

	// Show Yahoo mail 
	strcpy( tDest, YAHOO_YAHOO_MAIL );
	CreateServiceFunction( servicefunction, YahooGotoMailboxCommand );
	mi.position = 500090010;
	mi.hIcon = LoadIconEx( "mail" );
	mi.pszName = LPGEN( "&Yahoo Mail" );
	mi.pszService = servicefunction;
	YahooMenuItems [ 2 ] = ( HANDLE )CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );

	// Show Address Book    
	strcpy( tDest, YAHOO_AB );
	CreateServiceFunction( servicefunction, YahooABCommand );
	mi.position = 500090015;
	mi.hIcon = LoadIconEx( "yab" );
	mi.pszName = LPGEN( "&Address Book" );
	mi.pszService = servicefunction;
	YahooMenuItems [ 3 ] = ( HANDLE )CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );

	// Show Calendar
	strcpy( tDest, YAHOO_CALENDAR );
	CreateServiceFunction( servicefunction, YahooCalendarCommand );
	mi.position = 500090015;
	mi.hIcon = LoadIconEx( "calendar" );
	mi.pszName = LPGEN( "&Calendar" );
	mi.pszService = servicefunction;
	YahooMenuItems [ 4 ] = ( HANDLE )CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );

	// Show Refresh     
	strcpy( tDest, YAHOO_REFRESH );
	CreateServiceFunction( servicefunction, YahooRefreshCommand );
	mi.position = 500090015;
	mi.hIcon = LoadIconEx( "refresh" );
	mi.pszName = LPGEN( "&Refresh" );
	mi.pszService = servicefunction;
	YahooMenuItems [ 5 ] = ( HANDLE )CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	
	// Show Profile 
	strcpy( tDest, YAHOO_SHOW_PROFILE );
	CreateServiceFunction( servicefunction, YahooShowProfileCommand );
	mi.position = -2000006000;
	mi.hIcon = LoadIconEx( "profile" );
	mi.pszName = LPGEN( "&Show Profile" );
	mi.pszService = servicefunction;
	mi.pszContactOwner = yahooProtocolName;
	YahooMenuItems [ 6 ] = ( HANDLE )CallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );
}
//=======================================================
//Load the yahoo service/plugin
//=======================================================
int LoadYahooServices( void )
{
	//----| Service creation |------------------------------------------------------------
	hHookContactDeleted = HookEvent( ME_DB_CONTACT_DELETED, YahooContactDeleted );

	// Send Nudge
	YAHOO_CreateProtoServiceFunction( YAHOO_SEND_NUDGE, 	YahooSendNudge );
	
	YAHOO_CreateProtoServiceFunction( YAHOO_GETUNREAD_EMAILCOUNT, YahooGetUnreadEmailCount);
		
	YAHOO_CreateProtoServiceFunction( PS_GETCAPS,	GetCaps );
	YAHOO_CreateProtoServiceFunction( PS_GETNAME,	GetName );
	YAHOO_CreateProtoServiceFunction( PS_LOADICON,	YahooLoadIcon );
	YAHOO_CreateProtoServiceFunction( PS_SETSTATUS,	SetStatus );
	YAHOO_CreateProtoServiceFunction( PS_GETSTATUS,	GetStatus );

	YAHOO_CreateProtoServiceFunction( PS_BASICSEARCH,	YahooBasicSearch );
	YAHOO_CreateProtoServiceFunction( PS_ADDTOLIST,	YahooAddToList );
	
	YAHOO_CreateProtoServiceFunction( PS_AUTHALLOW,	YahooAuthAllow );
	YAHOO_CreateProtoServiceFunction( PS_AUTHDENY,	YahooAuthDeny );
	YAHOO_CreateProtoServiceFunction( PS_ADDTOLISTBYEVENT,	YahooAddToListByEvent );
	
	YAHOO_CreateProtoServiceFunction( PS_FILERESUME, YahooFileResume );
	
	
	YAHOO_CreateProtoServiceFunction( PSR_AUTH,	     YahooRecvAuth );
	YAHOO_CreateProtoServiceFunction( PSS_AUTHREQUEST,	YahooSendAuthRequest);
	///
	YAHOO_CreateProtoServiceFunction( PSS_MESSAGE,	YahooSendMessage );
	YAHOO_CreateProtoServiceFunction( PSR_MESSAGE,	YahooRecvMessage );
	 
	YAHOO_CreateProtoServiceFunction( PSS_GETAWAYMSG,	YahooGetAwayMessage );

	YAHOO_CreateProtoServiceFunction( PS_SETAWAYMSG, YahooSetAwayMessage );

	YAHOO_CreateProtoServiceFunction( PSS_GETINFO,	YahooGetInfo );

	YAHOO_CreateProtoServiceFunction( PSS_USERISTYPING,	YAHOOSendTyping );
	
	//Allows a file transfer to begin
	YAHOO_CreateProtoServiceFunction( PSS_FILEALLOW,		YahooFileAllow );
	//Refuses a file transfer request
	YAHOO_CreateProtoServiceFunction( PSS_FILEDENY,		YahooFileDeny );
	
	//Cancel an in-progress file transfer
	YAHOO_CreateProtoServiceFunction( PSS_FILECANCEL,		YahooFileCancel );
	//Initiate a file send
	YAHOO_CreateProtoServiceFunction( PSS_FILE,				YahooSendFile );
	//File(s) have been received
	YAHOO_CreateProtoServiceFunction( PSR_FILE,				YahooRecvFile );
	
	YAHOO_CreateProtoServiceFunction( PSS_SETAPPARENTMODE,	YahooSetApparentMode);
	
	YAHOO_CreateProtoServiceFunction( PS_GETAVATARINFO,	YahooGetAvatarInfo);
	YAHOO_CreateProtoServiceFunction( PS_GETAVATARCAPS, YahooGetAvatarCaps);
	YAHOO_CreateProtoServiceFunction( PS_GETMYAVATAR, YahooGetMyAvatar);
	YAHOO_CreateProtoServiceFunction( PS_SETMYAVATAR, YahooSetMyAvatar);

	YAHOO_CreateProtoServiceFunction( PS_CREATEADVSEARCHUI, YahooCreateAdvancedSearchUI );
	YAHOO_CreateProtoServiceFunction( PS_SEARCHBYADVANCED, YahooAdvancedSearch );
	
	return 0;
}



