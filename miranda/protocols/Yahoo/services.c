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
#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include <io.h>
#include "pthread.h"
#include "yahoo.h"

#include <m_system.h>
#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_langpack.h>
#include <m_skin.h>
#include <m_utils.h>
#include <m_options.h>
#include <m_popup.h>
#include <m_idle.h>

#include "resource.h"

void yahoo_logoff_buddies()
{
		//set all contacts to 'offline'
		HANDLE hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		while ( hContact != NULL ) 
		{
			if ( !lstrcmp( yahooProtocolName, ( char* )YAHOO_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact,0 ))) {
				YAHOO_SetWord( hContact, "Status", ID_STATUS_OFFLINE );
				DBWriteContactSettingDword(hContact, yahooProtocolName, "IdleTS", 0);
				
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
            ret = PF1_IM  | PF1_ADDED | PF1_AUTHREQ | PF1_MODEMSGRECV |  PF1_BASICSEARCH
			                        | PF1_FILESEND  | PF1_FILERECV| PF1_VISLIST;
//                          | PF1_SERVERCLIST ;
            break;

        case PFLAGNUM_2:
            ret = PF2_ONLINE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_ONTHEPHONE | 
                  PF2_OUTTOLUNCH | PF2_INVISIBLE | PF2_LIGHTDND | PF2_HEAVYDND; 
            break;

        case PFLAGNUM_3:
            ret = PF2_ONLINE | PF2_SHORTAWAY | PF2_LONGAWAY | PF2_ONTHEPHONE | 
                  PF2_OUTTOLUNCH | PF2_INVISIBLE | PF2_LIGHTDND | PF2_HEAVYDND; 
            break;
            
        case PFLAGNUM_4:
            ret = PF4_FORCEAUTH|PF4_FORCEADDED|PF4_SUPPORTTYPING|PF4_SUPPORTIDLE| PF4_AVATARS;
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
	lstrcpyn((char*)lParam, yahooProtocolName, wParam);
	return 0;
}

//=======================================================
//BPLoadIcon
//=======================================================
int YahooLoadIcon(WPARAM wParam,LPARAM lParam)
{
	UINT id;

	switch(wParam&0xFFFF) {
		case PLI_PROTOCOL: id=IDI_MAIN; break; // IDI_MAIN is the main icon for the protocol
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

//=======================================================
//SetStatus
//=======================================================
int SetStatus(WPARAM wParam,LPARAM lParam)
{
    int status = (int) wParam;

    //if (yahooStatus == status)
    //    return 0;
        
    YAHOO_DebugLog("Set Status to %s", (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, status, 0));
    if (status == ID_STATUS_OFFLINE) {
		//stop_timer();
        
		yahoo_logout();
		
        yahoo_util_broadcaststatus(ID_STATUS_OFFLINE);
        yahoo_logoff_buddies();
    }
    else if (!yahooLoggedIn) {
		DBVARIANT dbv;
		char errmsg[80];
		
		//if (hNetlibUser  == 0) {
			//MessageBox(NULL, "ARRRGH NO NETLIB HANDLE!!!", yahooProtocolName, MB_OK);
			//return ;
			//DebugBreak();
			//MessageBox(NULL, "ARRRGH NO NETLIB HANDLE!!!", yahooProtocolName, MB_OK);
			//return 0;
		//}
		
		errmsg[0]='\0';
		
        if (yahooStatus == ID_STATUS_CONNECTING)
			return 0;

		YAHOO_utils_logversion();
		
		ylad = y_new0(yahoo_local_account, 1);

		// Load Yahoo ID form the database.
		if (!DBGetContactSetting(NULL, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
			lstrcpyn(ylad->yahoo_id, dbv.pszVal, 255);
			DBFreeVariant(&dbv);
		} else 
			lstrcpyn(errmsg, Translate("Please enter your yahoo id in Options/Network/Yahoo"), 80);
		

		if (errmsg[0] == '\0') {
			if (!DBGetContactSetting(NULL, yahooProtocolName, YAHOO_PASSWORD, &dbv)) {
				CallService(MS_DB_CRYPT_DECODESTRING, lstrlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
				lstrcpyn(ylad->password, dbv.pszVal, 255);
				DBFreeVariant(&dbv);
			}  else {
				lstrcpyn(errmsg, Translate("Please enter your yahoo password in Options/Network/Yahoo"), 80);
			}
		}

		if (errmsg[0] != '\0'){
			FREE(ylad);
			ylad = NULL;
			yahoo_util_broadcaststatus(ID_STATUS_OFFLINE);
        
			if (YAHOO_hasnotification())
				YAHOO_shownotification(Translate("Yahoo Login Error"), errmsg, NIIF_ERROR);
			else
				MessageBox(NULL, errmsg, Translate("Yahoo Login Error"), MB_OK | MB_ICONINFORMATION);
    
			return 0;
		}

		if (miranda_to_yahoo(status) == YAHOO_STATUS_OFFLINE)
			status = ID_STATUS_ONLINE;
		
		DBWriteContactSettingWord(NULL, yahooProtocolName, "StartupStatus", status);

		yahoo_util_broadcaststatus(ID_STATUS_CONNECTING);
		
		status = (status == ID_STATUS_INVISIBLE) ? YAHOO_STATUS_INVISIBLE: YAHOO_STATUS_AVAILABLE;
        pthread_create(yahoo_server_main,  (void *) status );

		//start_timer();
    } else {
        yahoo_util_broadcaststatus(status);
		yahoo_set_status(yahooStatus,NULL,(yahooStatus != ID_STATUS_ONLINE) ? 1 : 0);
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
        
    yahooStatus = s;

    ProtoBroadcastAck(yahooProtocolName, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE) oldStatus, (LPARAM)yahooStatus);
}


static void __cdecl yahoo_im_sendacksuccess(HANDLE hContact)
{
    ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

static void __cdecl yahoo_im_sendackfail(HANDLE hContact)
{
    SleepEx(1000, TRUE);
    ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, (HANDLE) 1, 0);
}

static void __cdecl yahoo_im_sendackfail_longmsg(HANDLE hContact)
{
    SleepEx(1000, TRUE);
    ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_MESSAGE, ACKRESULT_FAILED, (HANDLE) 1, 
						(LPARAM)Translate("Message is too long: Yahoo messages are limited by 800 UTF8 chars"));
}

//=======================================================
//Send a message
//=======================================================
//#define MSG_LEN                                   2048
int YahooSendMessage(WPARAM wParam, LPARAM lParam)
{
    CCSDATA *ccs = (CCSDATA *) lParam;
    DBVARIANT dbv;
    char *msg = (char *) ccs->lParam;
    
    if (!yahooLoggedIn) {/* don't send message if we not connected! */
        pthread_create(yahoo_im_sendackfail, ccs->hContact);
        return 1;
    }
        
    if (lstrlen(msg) > 800) {/* don't send message if we not connected! */
        pthread_create(	yahoo_im_sendackfail_longmsg, ccs->hContact);
        return 1;
    }

	if (!DBGetContactSetting(ccs->hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
        yahoo_send_msg(dbv.pszVal, msg, 0);
        DBFreeVariant(&dbv);

        pthread_create(yahoo_im_sendacksuccess, ccs->hContact);
    
        return 1;
    }
    
    return 0;
}

int YahooSendMessageW(WPARAM wParam, LPARAM lParam)
{
    CCSDATA *ccs = (CCSDATA *) lParam;
    DBVARIANT dbv;
        
    if (!yahooLoggedIn) {/* don't send message if we not connected! */
        pthread_create(yahoo_im_sendackfail, ccs->hContact);
        return 1;
    }


	if (!DBGetContactSetting(ccs->hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
		char* p = ( char* )ccs->lParam;
		char* msg = Utf8EncodeUcs2(( wchar_t* )&p[ strlen(p)+1 ] );

		if (lstrlen(msg) > 800) {/* don't send message if we not connected! */
			pthread_create(yahoo_im_sendackfail_longmsg, ccs->hContact);
		} else {
			yahoo_send_msg(dbv.pszVal, msg, 1);
		    pthread_create(yahoo_im_sendacksuccess, ccs->hContact);
		}

        DBFreeVariant(&dbv);
		free(msg);
		
        return 1;
    }
    
    return 0;
}

//=======================================================
//Receive a message
//=======================================================
int YahooRecvMessage(WPARAM wParam, LPARAM lParam)
{
    DBEVENTINFO dbei;
    CCSDATA *ccs = (CCSDATA *) lParam;
    PROTORECVEVENT *pre = (PROTORECVEVENT *) ccs->lParam;

    DBDeleteContactSetting(ccs->hContact, "CList", "Hidden");
    ZeroMemory(&dbei, sizeof(dbei));
    dbei.cbSize = sizeof(dbei);
    dbei.szModule = yahooProtocolName;
    dbei.timestamp = pre->timestamp;
    dbei.flags = pre->flags & (PREF_CREATEREAD ? DBEF_READ : 0);
    dbei.eventType = EVENTTYPE_MESSAGE;
    dbei.cbBlob = lstrlen(pre->szMessage) + 1;
	if ( pre->flags & PREF_UNICODE )
		dbei.cbBlob *= ( sizeof( wchar_t )+1 );

	
    dbei.pBlob = (PBYTE) pre->szMessage;
    CallService(MS_DB_EVENT_ADD, (WPARAM) ccs->hContact, (LPARAM) & dbei);
    return 0;
}

//=======================================================
//Search for user
//=======================================================
static void __cdecl yahoo_search_simplethread(void *snsearch)
{
    PROTOSEARCHRESULT psr;

/*    if (aim_util_isme(sn)) {
        ProtoBroadcastAck(AIM_PROTO, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
        return;
    }
    */
    ZeroMemory(&psr, sizeof(psr));
    psr.cbSize = sizeof(psr);
    psr.nick = (char *)snsearch;
    //psr.email = (char *)snsearch;
    
	//void yahoo_search(int id, enum yahoo_search_type t, const char *text, enum yahoo_search_gender g, enum yahoo_search_agerange ar, 
	//	int photo, int yahoo_only)

    YAHOO_SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
    //YAHOO_SendBroadcast(NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

void yahoo_search_simple(const char *nick)
{
    static char m[255];
    char *c;
    
    c = strchr(nick, '@');
    
    if (c != NULL){
        int l =  c - nick;

        strncpy(m, nick, l);
        m[l] = '\0';        
    }else
        strcpy(m, nick);
        
	YAHOO_basicsearch(nick);
    pthread_create(yahoo_search_simplethread, (void *) m);
}

static int YahooBasicSearch(WPARAM wParam,LPARAM lParam)
{
	if ( !yahooLoggedIn )
		return 0;

    yahoo_search_simple((char *) lParam);
    return 1;
}

static int YahooContactDeleted( WPARAM wParam, LPARAM lParam )
{
	char* szProto;
	DBVARIANT dbv;
	
	if ( !yahooLoggedIn )  //should never happen for Yahoo contacts
		return 0;

	szProto = ( char* )YAHOO_CallService( MS_PROTO_GETCONTACTBASEPROTO, wParam, 0 );
	if ( szProto == NULL || strcmp( szProto, yahooProtocolName )) 
		return 0;

	
	if ( !DBGetContactSetting(( HANDLE )wParam, yahooProtocolName, YAHOO_LOGINID, &dbv )) 
	{
		YAHOO_remove_buddy(dbv.pszVal);
		
		DBFreeVariant( &dbv );
	}
	return 0;
}

int YahooSendAuthRequest(WPARAM wParam,LPARAM lParam)
{
	YAHOO_DebugLog("YahooSendAuthRequest");
	
	if (lParam && yahooLoggedIn){
		
		CCSDATA *ccs = (CCSDATA *)lParam;
		
		if (ccs->hContact){
			
			DBVARIANT dbv;
			
			if (!DBGetContactSetting(ccs->hContact, yahooProtocolName, YAHOO_LOGINID, &dbv ))			{
				char *c = NULL;
				
				if ( ccs->lParam )
					c = (char *)ccs->lParam;
				
				YAHOO_DebugLog("Adding buddy:%s Auth:%s", dbv.pszVal, c);
				YAHOO_add_buddy(dbv.pszVal, "miranda", c);
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
	
	YAHOO_DebugLog("[YahooAddToList]");
	
	if (!yahooLoggedIn){
		YAHOO_DebugLog("[YahooAddToList] WARNING: WE ARE OFFLINE!");
		return 0;
	}
	
	if (psr == NULL || psr->cbSize != sizeof( PROTOSEARCHRESULT ))
		return 0;

	if (find_buddy(psr->nick) != NULL) {
		YAHOO_DebugLog("[YahooAddToList] Buddy:%s already on our buddy list", psr->nick);
		return 0;
	}
	
	YAHOO_DebugLog("Adding buddy:%s", psr->nick);
    return (int)add_buddy(psr->nick, psr->nick, wParam);
}


int YahooAuthAllow(WPARAM wParam,LPARAM lParam)
{
    DBEVENTINFO dbei;
    char* nick;
    
    YAHOO_DebugLog("YahooAuthAllow");
	if ( !yahooLoggedIn )
		return 1;

	
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

	nick = ( char* )( dbei.pBlob + sizeof( DWORD )*2 );

    YAHOO_DebugLog("Adding buddy:%s ", nick);
	YAHOO_add_buddy(nick, "miranda", NULL);
	return 0;
}

int YahooAuthDeny(WPARAM wParam,LPARAM lParam)
{
    DBEVENTINFO dbei;
    char* nick;
	char *handle;
	char* reason;
	HANDLE hContact;
	
    YAHOO_DebugLog("YahooAuthDeny");
	if ( !yahooLoggedIn )
		return 1;

	memset( &dbei, 0, sizeof( dbei ));
	dbei.cbSize = sizeof( dbei );

	if (( dbei.cbBlob = YAHOO_CallService( MS_DB_EVENT_GETBLOBSIZE, wParam, 0 )) == -1 ){
	    YAHOO_DebugLog("YahooAuthDeny - Can't get blob size");
		return 1;
	}

	dbei.pBlob = ( PBYTE )alloca( dbei.cbBlob );
	if ( YAHOO_CallService( MS_DB_EVENT_GET, wParam, ( LPARAM )&dbei )){
	    YAHOO_DebugLog("YahooAuthDeny - Can't get db event!");
		return 1;
	}

	if ( dbei.eventType != EVENTTYPE_AUTHREQUEST ){
	    YAHOO_DebugLog("YahooAuthDeny - not Authorizarion event");
		return 1;
	}

	if ( strcmp( dbei.szModule, yahooProtocolName )){
	    YAHOO_DebugLog("YahooAuthDeny - wrong module?");
		return 1;
	}

	nick = ( char* )( dbei.pBlob + sizeof( DWORD )*2 );
	handle = ( char* )( dbei.pBlob + sizeof( DWORD ) );
	reason = (char*)lParam;
		
    memcpy(&hContact,handle, sizeof(HANDLE)); 
    
    /* Need to remove the buddy from our Miranda Lists */
    YAHOO_DebugLog("Rejecting buddy:%s msg: %s", nick, reason);    
    YAHOO_reject(nick,reason);
    
    YAHOO_CallService( MS_DB_CONTACT_DELETE, (WPARAM) hContact, 0);
	return 0;
}

int YahooRecvAuth(WPARAM wParam,LPARAM lParam)
{
	DBEVENTINFO dbei;
	CCSDATA *ccs=(CCSDATA*)lParam;
	PROTORECVEVENT *pre=(PROTORECVEVENT*)ccs->lParam;

    YAHOO_DebugLog("YahooRecvAuth");
	DBDeleteContactSetting(ccs->hContact,"CList","Hidden");

	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.szModule=yahooProtocolName;
	dbei.timestamp=pre->timestamp;
	dbei.flags=pre->flags & (PREF_CREATEREAD?DBEF_READ:0);
	dbei.eventType=EVENTTYPE_AUTHREQUEST;
	dbei.cbBlob=pre->lParam;
	dbei.pBlob=(PBYTE)pre->szMessage;
	CallService(MS_DB_EVENT_ADD,(WPARAM)NULL,(LPARAM)&dbei);

	return 0;
}

static void __cdecl yahoo_get_statusthread(HANDLE hContact)
{
	CCSDATA ccs1;
	PROTORECVEVENT pre;
	int status;
	DBVARIANT dbv;

	status = DBGetContactSettingWord(hContact, yahooProtocolName, "Status", ID_STATUS_OFFLINE);

	if (status == ID_STATUS_OFFLINE)
	   return;
	
	if ( DBGetContactSetting(( HANDLE )hContact, yahooProtocolName, "YMsg", &dbv )) {
		pre.szMessage = yahoo_status_code(DBGetContactSettingWord(hContact, yahooProtocolName, "YStatus", YAHOO_STATUS_OFFLINE));
	} else {
		if (lstrlen(dbv.pszVal) < 1)
			pre.szMessage = yahoo_status_code(DBGetContactSettingWord(hContact, yahooProtocolName, "YStatus", YAHOO_STATUS_OFFLINE));
		else
			pre.szMessage = strdup(dbv.pszVal);
		
		DBFreeVariant( &dbv );
	}

	ccs1.szProtoService = PSR_AWAYMSG;
	ccs1.hContact = hContact;
	ccs1.wParam = status;
	ccs1.lParam = (LPARAM)&pre;
	pre.flags = 0;
	pre.timestamp = time(NULL);
	pre.lParam = 1;

	CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs1);
}

//=======================================================
//Handle away message
//=======================================================
int YahooGetAwayMessage(WPARAM wParam,LPARAM lParam)
{
    YAHOO_DebugLog("YahooGetAwayMessage");
	if (lParam && yahooLoggedIn) {
	    CCSDATA *ccs = (CCSDATA *) lParam;
		 DBVARIANT dbv;
		 int status;
		 
		status = DBGetContactSettingWord(( HANDLE )ccs->hContact, yahooProtocolName, "Status", ID_STATUS_OFFLINE);

		if (status == ID_STATUS_OFFLINE)
			   return 0; /* user offline, what Status message? */

		 if ( DBGetContactSetting(( HANDLE )ccs->hContact, yahooProtocolName, YAHOO_LOGINID, &dbv )) {
		 	 YAHOO_DebugLog("YAHOOGetAwayMessage Can't retrieve buddy id.");
		 	return 0;
	 	}
	 		YAHOO_DebugLog("Buddy %s ", dbv.pszVal);
   				
   			DBFreeVariant( &dbv );

            pthread_create(yahoo_get_statusthread, ccs->hContact);
            return 1; //Success		
		
	}
	
	return 0; // Failure
}

int YahooRecvAwayMessage(WPARAM wParam,LPARAM lParam)
{
    CCSDATA *ccs;
    PROTORECVEVENT *pre;
    
    YAHOO_DebugLog("YahooRecvAwayMessage");
	
	ccs=(CCSDATA*)lParam;
	pre=(PROTORECVEVENT*)ccs->lParam;
	
	YAHOO_DebugLog("Got Msg: %s", pre->szMessage);
	ProtoBroadcastAck(yahooProtocolName,ccs->hContact,ACKTYPE_AWAYMSG,ACKRESULT_SUCCESS,(HANDLE)pre->lParam,(LPARAM)pre->szMessage);
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
	pthread_create(yahoo_get_infothread, ccs->hContact);
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
			SendMessage( hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon( hinstance, MAKEINTRESOURCE( IDI_YAHOO )));

		    if ( !DBGetContactSetting( NULL, yahooProtocolName, YAHOO_CUSTSTATDB, &dbv ))
				    {
				    SetDlgItemText( hwndDlg, IDC_CUSTSTAT, dbv. pszVal );
				    DBFreeVariant( &dbv );
				    }
		    else 
                    SetDlgItemText( hwndDlg, IDC_CUSTSTAT, "" );


            CheckDlgButton( hwndDlg, IDC_CUSTSTATBUSY,  YAHOO_GetByte( "BusyCustStat", 0 ));
			return TRUE;
		}
		case WM_COMMAND:
			switch(wParam) 
			{
				case IDOK:
						{
                        char str[ 255 + 1 ];
						
						GetDlgItemText( hwndDlg, IDC_CUSTSTAT, str, sizeof( str ));
						YAHOO_SetString( NULL, YAHOO_CUSTSTATDB, str );
                        YAHOO_SetByte("BusyCustStat", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_CUSTSTATBUSY ));

                        // Quick hack to reflect the status in miranda while using custome status
                        // Is there a way to force the miranda status without broadcasting....
                        // If only we had two new status for custom ones.....
                        if (( BYTE )IsDlgButtonChecked( hwndDlg, IDC_CUSTSTATBUSY ))
                              SetStatus(ID_STATUS_OCCUPIED,0);
                        else
                              SetStatus(ID_STATUS_ONLINE,0);

						if (yahooLoggedIn)
 				              {
                              yahoo_set_status(YAHOO_CUSTOM_STATUS, str, ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_CUSTSTATBUSY ));
 				              }
						else
			             	  YAHOO_shownotification("ERROR", Translate("You need to be connected to set the custom message"), NIIF_ERROR);
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
//=======================================================
//Show custom status windows
//=======================================================
static int SetCustomStatCommand( WPARAM wParam, LPARAM lParam )
{
	HWND hwndSetCustomStatus;
	
	if ( !yahooLoggedIn ) {
		YAHOO_shownotification("ERROR", Translate("You need to be connected to set the custom message"), NIIF_ERROR);
		return 0;
	}
	
	hwndSetCustomStatus = CreateDialog(hinstance, MAKEINTRESOURCE( IDD_SETCUSTSTAT ), NULL, DlgProcSetCustStat );
	
	SetForegroundWindow( hwndSetCustomStatus );
	SetFocus( hwndSetCustomStatus );
 	ShowWindow( hwndSetCustomStatus, SW_SHOW );
	return 0;
}

//=======================================================
//Show buddy profile
//=======================================================
static int YahooShowProfileCommand( WPARAM wParam, LPARAM lParam )
{
	char tUrl[ 4096 ];
	DBVARIANT dbv;

	if ( !yahooLoggedIn ) 
		return 0;
	
	if ( DBGetContactSetting(( HANDLE )wParam, yahooProtocolName, "yahoo_id", &dbv ))
		return 0;
		
	_snprintf( tUrl, sizeof( tUrl ), "http://profiles.yahoo.com/%s", dbv.pszVal  );
	DBFreeVariant( &dbv );
	CallService( MS_UTILS_OPENURL, TRUE, ( LPARAM )tUrl );    
		
	return 0;
}

//=======================================================
//Show My profile
//=======================================================
static int YahooShowMyProfileCommand( WPARAM wParam, LPARAM lParam )
{
	char tUrl[ 4096 ];
	DBVARIANT dbv;

	if ( !yahooLoggedIn )
		return 0;

	DBGetContactSetting( NULL, yahooProtocolName, YAHOO_LOGINID, &dbv );
		
	_snprintf( tUrl, sizeof( tUrl ), "http://profiles.yahoo.com/%s", dbv.pszVal  );
	DBFreeVariant( &dbv );

	CallService( MS_UTILS_OPENURL, TRUE, ( LPARAM )tUrl );    
	
	return 0;
}

//=======================================================
//Show Goto mailbox
//=======================================================
static int YahooGotoMailboxCommand( WPARAM wParam, LPARAM lParam )
{
	char tUrl[ 4096 ];
	DBVARIANT dbv;

	if ( !yahooLoggedIn )
		return 0;

	if ( DBGetContactSetting(( HANDLE )wParam, yahooProtocolName, "yahoo_id", &dbv ))
		return 0;
		
	_snprintf( tUrl, sizeof( tUrl ), "http://mail.yahoo.com/", dbv.pszVal  );
	DBFreeVariant( &dbv );
	CallService( MS_UTILS_OPENURL, TRUE, ( LPARAM )tUrl );    
		
	return 0;
}

//=======================================================
//Refresh Yahoo
//=======================================================
static int YahooRefreshCommand( WPARAM wParam, LPARAM lParam )
{
	if ( !yahooLoggedIn ){
		YAHOO_shownotification("ERROR", Translate("You need to be connected to refresh your buddy list"), NIIF_ERROR);
		return 0;
	}
//	yahoo_refresh(ylad->id);
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
	if (!DBGetContactSetting(hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
		if (state==PROTOTYPE_SELFTYPING_OFF || state==PROTOTYPE_SELFTYPING_ON) {
			YAHOO_sendtyping(dbv.pszVal, state == PROTOTYPE_SELFTYPING_ON?1:0);
		}
		DBFreeVariant(&dbv);
	}
	return 0;
}

//=======================================================
//Files transfert
//=======================================================
static void __cdecl yahoo_recv_filethread(void *psf) 
{
	y_filetransfer *sf = psf;
	
//    ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
	if (sf == NULL) {
		YAHOO_DebugLog("SF IS NULL!!!");
		return;
	}
	YAHOO_DebugLog("who %s, msg: %s, filename: %s ", sf->who, sf->msg, sf->filename);
	
	YAHOO_RecvFile(sf);
	if ( sf->hWaitEvent != INVALID_HANDLE_VALUE )
		CloseHandle( sf->hWaitEvent );
	
	free(sf->who);
	free(sf->msg);
	free(sf->filename);
	free(sf->url);
	free(sf->savepath);
	free(sf);
	
}

/**************** Receive File ********************/
int YahooFileAllow(WPARAM wParam,LPARAM lParam) 
{
    CCSDATA *ccs = (CCSDATA *) lParam;
    y_filetransfer *ft = (y_filetransfer *) ccs->wParam;
	int len;
	
    //LOG(LOG_INFO, "[%s] Requesting file from %s", ft->cookie, ft->user);
    ft->savepath = _strdup((char *) ccs->lParam);
	
	len = lstrlen(ft->savepath) - 1;
	if (ft->savepath[len] == '\\')
		ft->savepath[len] = '\0';
	
    pthread_create(yahoo_recv_filethread, (void *) ft);
	
    return ccs->wParam;
}

int YahooFileDeny(WPARAM wParam,LPARAM lParam) 
{
	/* deny file receive request.. just ignore it! */
	return 0;
}

int YahooFileResume( WPARAM wParam, LPARAM lParam )
{
    PROTOFILERESUME *pfr;
	y_filetransfer *ft = (y_filetransfer *) wParam;
	
	YAHOO_DebugLog("[YahooFileResume]");
	
	if ( !yahooLoggedIn || ft == NULL ) {
		YAHOO_DebugLog("[YahooFileResume] Not loggedin or some other error!");
		return 1;
	}

	pfr = (PROTOFILERESUME*)lParam;
	
	ft->action = pfr->action;
	
	YAHOO_DebugLog("[YahooFileResume] Action: %d", pfr->action);
	
	if ( pfr->action == FILERESUME_RENAME ) {
		YAHOO_DebugLog("[YahooFileResume] Renamed file!");
		if ( ft->filename != NULL ) {
			free( ft->filename );
			ft->filename = NULL;
		}

		ft->filename = strdup( pfr->szFilename );
	}	
	

	SetEvent( ft->hWaitEvent );
	return 0;
}

/**************** Send File ********************/
static void __cdecl yahoo_send_filethread(void *psf) 
{
	y_filetransfer *sf = psf;
	
//    ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
	if (sf == NULL) {
		YAHOO_DebugLog("SF IS NULL!!!");
		return;
	}
	YAHOO_DebugLog("who %s, msg: %s, filename: %s ", sf->who, sf->msg, sf->filename);
	
	YAHOO_SendFile(sf);
	free(sf->who);
	free(sf->msg);
	free(sf->filename);
	free(sf);
	
}

int YahooFileCancel(WPARAM wParam,LPARAM lParam) 
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	y_filetransfer* ft = (y_filetransfer*)ccs->wParam;
	
	YAHOO_DebugLog("[YahooFileCancel]");
	
	if ( ft->hWaitEvent != INVALID_HANDLE_VALUE )
		SetEvent( ft->hWaitEvent );
	
	ft->action = FILERESUME_CANCEL;
	ft->cancel = 1;
	return 0;
}

/*
 *
 */
int YahooSendFile(WPARAM wParam,LPARAM lParam) 
{
	DBVARIANT dbv;
	y_filetransfer *sf;
	CCSDATA *ccs;
	char** files;
	
	YAHOO_DebugLog("YahooSendFile");
	
	if ( !yahooLoggedIn )
		return 0;

	YAHOO_DebugLog("Gathering Data");
	
	ccs = ( CCSDATA* )lParam;
	//if ( YAHOO_GetWord( ccs->hContact, "Status", ID_STATUS_OFFLINE ) == ID_STATUS_OFFLINE )
	//	return 0;

	YAHOO_DebugLog("Getting Files");
	
	files = ( char** )ccs->lParam;
	if ( files[1] != NULL ){
		MessageBox(NULL, "YAHOO protocol allows only one file to be sent at a time", "Yahoo", MB_OK | MB_ICONINFORMATION);
		return 0;
 	}
	
	YAHOO_DebugLog("Getting Yahoo ID");
	
	if (!DBGetContactSetting(ccs->hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {

		sf = (y_filetransfer*) malloc(sizeof(y_filetransfer));
		sf->who = strdup(dbv.pszVal);
		sf->msg = strdup(( char* )ccs->wParam );
		sf->filename = strdup(files[0]);
		sf->hContact = ccs->hContact;
		sf->cancel = 0;
		
		YAHOO_DebugLog("who: %s, msg: %s, filename: %s", sf->who, sf->msg, sf->filename);
		pthread_create(yahoo_send_filethread, sf);
		
		DBFreeVariant(&dbv);
		YAHOO_DebugLog("Exiting SendRequest...");
		return (int)(HANDLE)sf;
	}
	
	YAHOO_DebugLog("Exiting SendFile");
	return 0;
}

int YahooRecvFile(WPARAM wParam,LPARAM lParam) 
{
    DBEVENTINFO dbei;
    CCSDATA *ccs = (CCSDATA *) lParam;
    PROTORECVEVENT *pre = (PROTORECVEVENT *) ccs->lParam;
    char *szDesc, *szFile;

    DBDeleteContactSetting(ccs->hContact, "CList", "Hidden");
    szFile = pre->szMessage + sizeof(DWORD);
    szDesc = szFile + lstrlen(szFile) + 1;
    ZeroMemory(&dbei, sizeof(dbei));
    dbei.cbSize = sizeof(dbei);
    dbei.szModule = yahooProtocolName;
    dbei.timestamp = pre->timestamp;
    dbei.flags = pre->flags & (PREF_CREATEREAD ? DBEF_READ : 0);
    dbei.eventType = EVENTTYPE_FILE;
    dbei.cbBlob = sizeof(DWORD) + lstrlen(szFile) + lstrlen(szDesc) + 2;
    dbei.pBlob = (PBYTE) pre->szMessage;
    CallService(MS_DB_EVENT_ADD, (WPARAM) ccs->hContact, (LPARAM) & dbei);
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
		yahoo_set_status(yahooStatus,NULL,(bIdle) ? 2 : 0);
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

void GetAvatarFileName(HANDLE hContact, char* pszDest, int cbLen)
{
  int tPathLen;

  DBVARIANT dbv;
  
  CallService(MS_DB_GETPROFILEPATH, cbLen, (LPARAM)pszDest);

  tPathLen = lstrlen(pszDest);
  tPathLen += _snprintf(pszDest + tPathLen, MAX_PATH-tPathLen, "\\%s\\", yahooProtocolName);
  CreateDirectory(pszDest, NULL);

  if (!DBGetContactSetting(hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
		lstrcpy(pszDest + tPathLen, dbv.pszVal);
		tPathLen +=lstrlen(dbv.pszVal);
		DBFreeVariant(&dbv);
  }else {
		lstrcpy(pszDest + tPathLen, "avatar");
		tPathLen +=lstrlen("avatar");
  }
  lstrcpy(pszDest + tPathLen, ".bmp" );
}

int YahooGetAvatarInfo(WPARAM wParam,LPARAM lParam)
{
	PROTO_AVATAR_INFORMATION* AI = ( PROTO_AVATAR_INFORMATION* )lParam;

	DBVARIANT dbv;
	if (!DBGetContactSetting(AI->hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
		YAHOO_DebugLog("[YAHOO_GETAVATARINFO] For: %s", dbv.pszVal);
		DBFreeVariant(&dbv);
	}else {
		YAHOO_DebugLog("[YAHOO_GETAVATARINFO]");
	}

	if (!YAHOO_GetByte( "ShowAvatars", 0 )) {
		if (DBGetContactSettingDword(AI->hContact, yahooProtocolName,"PictCK", 0) != 0) {
			GetAvatarFileName(AI->hContact, AI->filename, sizeof AI->filename);
			DeleteFile(AI->filename);
		}

		return GAIR_NOAVATAR;
	}
	
	if (DBGetContactSettingDword(AI->hContact, yahooProtocolName,"PictCK", 0) != 0) {
		
		GetAvatarFileName(AI->hContact, AI->filename, sizeof AI->filename);
		//if ( access( AI->filename, 0 ) == 0 ) {
		AI->format = PA_FORMAT_BMP;
		YAHOO_DebugLog("[YAHOO_GETAVATARINFO] filename: %s", AI->filename);
		
		if (_access( AI->filename, 0 ) == 0 ) {
			return GAIR_SUCCESS;
		} else {
			/* need to request it again? */
			if (YAHOO_GetWord(AI->hContact, "PictLoading", 0) != 0) {
				return GAIR_WAITFOR;
			} else if ( yahooLoggedIn ) {
				DBVARIANT dbv;
	  
				if (!DBGetContactSetting(AI->hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
					YAHOO_request_avatar(dbv.pszVal/*who */);
					DBFreeVariant(&dbv);
					return GAIR_WAITFOR;
				}
			}
		}
		
	}
	
	YAHOO_DebugLog("[YAHOO_GETAVATARINFO] NO AVATAR???");
	return GAIR_NOAVATAR;
}

int YahooSetAvatarUI( WPARAM wParam, LPARAM lParam )
{
	/*char szFileName[ MAX_PATH ];
	if ( MSN_EnterBitmapFileName( szFileName ) != ERROR_SUCCESS )
		return 1;

	return MsnSetAvatar( 0, ( LPARAM )szFileName );*/
	return 1; /* error for now */
}

extern HANDLE   hHookContactDeleted;
extern HANDLE   hHookIdle;

//=======================================================
//Load the yahoo service/plugin
//=======================================================
int LoadYahooServices( void )
{
    if (!YAHOO_GetByte( "DisableMainMenu", 0 ))
        {
        char servicefunction[ 100 ];
        char* tDest;
        CLISTMENUITEM mi;
        
        lstrcpy( servicefunction, yahooProtocolName );
    	tDest = servicefunction + lstrlen( servicefunction );
    	
    	// Show custom status menu    
    	lstrcpy( tDest, YAHOO_SET_CUST_STAT );
    	CreateServiceFunction( servicefunction, SetCustomStatCommand );
    	memset( &mi, 0, sizeof( mi ));
    	mi.pszPopupName = yahooProtocolName;
    	mi.cbSize = sizeof( mi );
    	mi.popupPosition = 500090000;
    	mi.position = 500090000;
    	mi.hIcon = LoadIcon( hinstance, MAKEINTRESOURCE( IDI_YAHOO ));
    	mi.pszName = Translate( "Set &Custom Status" );
    	mi.pszService = servicefunction;
        YahooMenuItems [ 0 ] = ( HANDLE )CallService( MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi );

    	// Show My profile menu    
    	lstrcpy( tDest, YAHOO_SHOW_MY_PROFILE );
    	CreateServiceFunction( servicefunction, YahooShowMyProfileCommand );
    	mi.position = 500090005;
    	mi.hIcon = LoadIcon( hinstance, MAKEINTRESOURCE( IDI_YAHOO ));
    	mi.pszName = Translate( "S&how My profile" );
    	mi.pszService = servicefunction;
    	YahooMenuItems [ 1 ] = ( HANDLE )CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
    
   	    // Show Yahoo mail menu
    	strcpy( tDest, YAHOO_YAHOO_MAIL );
    	CreateServiceFunction( servicefunction, YahooGotoMailboxCommand );
    	mi.position = 500090010;
    	mi.hIcon = LoadIcon( hinstance, MAKEINTRESOURCE( IDI_INBOX ));
    	mi.pszName = Translate( "&Yahoo Mail" );
    	mi.pszService = servicefunction;
    	YahooMenuItems [ 2 ] = ( HANDLE )CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );

        // Show refresh menu    
    	strcpy( tDest, YAHOO_REFRESH );
    	CreateServiceFunction( servicefunction, YahooRefreshCommand );
    	mi.position = 500090015;
    	mi.hIcon = LoadIcon( hinstance, MAKEINTRESOURCE( IDI_YAHOO ));
    	mi.pszName = Translate( "&Refresh" );
    	mi.pszService = servicefunction;
    	YahooMenuItems [ 3 ] = ( HANDLE )CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );

        // Show show profile menu    
    	strcpy( tDest, YAHOO_SHOW_PROFILE );
    	CreateServiceFunction( servicefunction, YahooShowProfileCommand );
    	mi.position = -2000006000;
    	mi.hIcon = LoadIcon( hinstance, MAKEINTRESOURCE( IDI_YAHOO ));
    	mi.pszName = Translate( "&Show profile" );
    	mi.pszService = servicefunction;
		mi.pszContactOwner = yahooProtocolName;
    	YahooMenuItems [ 4 ] = ( HANDLE )CallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );
    
    	}
	
	//----| Service creation |------------------------------------------------------------
	hHookContactDeleted = HookEvent( ME_DB_CONTACT_DELETED, YahooContactDeleted );
		
	YAHOO_CreateProtoServiceFunction( PS_GETCAPS,	GetCaps );
	YAHOO_CreateProtoServiceFunction( PS_GETNAME,	GetName );
	YAHOO_CreateProtoServiceFunction( PS_LOADICON,	YahooLoadIcon );
	YAHOO_CreateProtoServiceFunction( PS_SETSTATUS,	SetStatus );
	YAHOO_CreateProtoServiceFunction( PS_GETSTATUS,	GetStatus );

	YAHOO_CreateProtoServiceFunction( PS_BASICSEARCH,	YahooBasicSearch );
	YAHOO_CreateProtoServiceFunction( PS_ADDTOLIST,	YahooAddToList );
	
	YAHOO_CreateProtoServiceFunction( PS_AUTHALLOW,	YahooAuthAllow );
	YAHOO_CreateProtoServiceFunction( PS_AUTHDENY,	YahooAuthDeny );
	
	YAHOO_CreateProtoServiceFunction( PS_FILERESUME, YahooFileResume );
	
	
	YAHOO_CreateProtoServiceFunction( PSR_AUTH,	     YahooRecvAuth );
	YAHOO_CreateProtoServiceFunction( PSS_AUTHREQUEST,	YahooSendAuthRequest);
	///
	YAHOO_CreateProtoServiceFunction( PSS_MESSAGE,	YahooSendMessage );
	YAHOO_CreateProtoServiceFunction( PSS_MESSAGE"W",	YahooSendMessageW );
	
	YAHOO_CreateProtoServiceFunction( PSR_MESSAGE,	YahooRecvMessage );
	
	YAHOO_CreateProtoServiceFunction( PSS_GETAWAYMSG,	YahooGetAwayMessage );
	YAHOO_CreateProtoServiceFunction( PSR_AWAYMSG,	YahooRecvAwayMessage );


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
	return 0;
}



