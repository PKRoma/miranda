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

void CYahooProto::logoff_buddies()
{
	//set all contacts to 'offline'
	HANDLE hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) 
	{
		if ( !lstrcmpA( m_szModuleName, ( char* )YAHOO_CallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM )hContact,0 ))) {
			SetWord( hContact, "Status", ID_STATUS_OFFLINE );
			DBWriteContactSettingDword(hContact, m_szModuleName, "IdleTS", 0);
			DBWriteContactSettingDword(hContact, m_szModuleName, "PictLastCheck", 0);
			DBWriteContactSettingDword(hContact, m_szModuleName, "PictLoading", 0);
			DBDeleteContactSetting(hContact, "CList", "StatusMsg" );
			DBDeleteContactSetting(hContact, m_szModuleName, "YMsg" );
			DBDeleteContactSetting(hContact, m_szModuleName, "YGMsg" );
			//DBDeleteContactSetting(hContact, m_szModuleName, "MirVer" );
		}

		hContact = ( HANDLE )YAHOO_CallService( MS_DB_CONTACT_FINDNEXT,( WPARAM )hContact, 0 );
	}	
}

//=======================================================
//Broadcast the user status
//=======================================================
void CYahooProto::BroadcastStatus(int s)
{
	int oldStatus = m_iStatus;
	if (oldStatus == s)
		return;

	//m_iStatus = s;
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
		m_iStatus = s;
		break;
	case ID_STATUS_DND:
		m_iStatus = ID_STATUS_OCCUPIED;
		break;
	default:
		m_iStatus = ID_STATUS_ONLINE;
	}

	DebugLog("[yahoo_util_broadcaststatus] Old Status: %s (%d), New Status: %s (%d)",
		NEWSTR_ALLOCA((char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, oldStatus, 0)), oldStatus,
		NEWSTR_ALLOCA((char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, m_iStatus, 0)), m_iStatus);	
	ProtoBroadcastAck(m_szModuleName, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE) oldStatus, (LPARAM)m_iStatus);
}

//=======================================================
//Contact deletion event
//=======================================================
int __cdecl CYahooProto::OnContactDeleted( WPARAM wParam, LPARAM lParam )
{
	char* szProto;
	DBVARIANT dbv;
	HANDLE hContact = (HANDLE) wParam;
	
	DebugLog("[YahooContactDeleted]");
	
	if ( !m_bLoggedIn )  {//should never happen for Yahoo contacts
		DebugLog("[YahooContactDeleted] We are not Logged On!!!");
		return 0;
	}

	szProto = ( char* )YAHOO_CallService( MS_PROTO_GETCONTACTBASEPROTO, wParam, 0 );
	if ( szProto == NULL || lstrcmpA( szProto, m_szModuleName ))  {
		DebugLog("[YahooContactDeleted] Not a Yahoo Contact!!!");
		return 0;
	}

	// he is not a permanent contact!
	if (DBGetContactSettingByte(hContact, "CList", "NotOnList", 0) != 0) {
		DebugLog("[YahooContactDeleted] Not a permanent buddy!!!");
		return 0;
	}
	
	if ( !DBGetContactSettingString(hContact, m_szModuleName, YAHOO_LOGINID, &dbv )){
		DebugLog("[YahooContactDeleted] Removing %s", dbv.pszVal);
		remove_buddy(dbv.pszVal, GetWord(hContact, "yprotoid", 0));
		
		DBFreeVariant( &dbv );
	} else {
		DebugLog("[YahooContactDeleted] Can't retrieve contact Yahoo ID");
	}
	return 0;
}

//=======================================================
//Custom status message windows handling
//=======================================================
static BOOL CALLBACK DlgProcSetCustStat(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DBVARIANT dbv;

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg );
		{
			CYahooProto* ppro = ( CYahooProto* )lParam;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, lParam );

			SendMessage( hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)ppro->LoadIconEx( "yahoo" ) );

			if ( !DBGetContactSettingString( NULL, ppro->m_szModuleName, YAHOO_CUSTSTATDB, &dbv )) {
				SetDlgItemTextA( hwndDlg, IDC_CUSTSTAT, dbv. pszVal );

				EnableWindow( GetDlgItem( hwndDlg, IDOK ), lstrlenA(dbv.pszVal) > 0 );
				DBFreeVariant( &dbv );
			}
			else {
				SetDlgItemTextA( hwndDlg, IDC_CUSTSTAT, "" );
				EnableWindow( GetDlgItem( hwndDlg, IDOK ), FALSE );
			}

			CheckDlgButton( hwndDlg, IDC_CUSTSTATBUSY,  ppro->GetByte( "BusyCustStat", 0 ));
		}
		return TRUE;

	case WM_COMMAND:
		switch(wParam) {
		case IDOK:
			{
				char str[ 255 + 1 ];
				CYahooProto* ppro = ( CYahooProto* )GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

				/* Get String from dialog */
				GetDlgItemTextA( hwndDlg, IDC_CUSTSTAT, str, sizeof( str ));

				/* Save it for later use */
				ppro->SetString( NULL, YAHOO_CUSTSTATDB, str );
				ppro->SetByte("BusyCustStat", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_CUSTSTATBUSY ));

				/* set for Idle/AA */
				if (ppro->m_startMsg) free(ppro->m_startMsg);
				ppro->m_startMsg = strdup(str);

				/* notify Server about status change */
				ppro->set_status(YAHOO_CUSTOM_STATUS, str, ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_CUSTSTATBUSY ));

				/* change local/miranda status */
				ppro->BroadcastStatus(( BYTE )IsDlgButtonChecked( hwndDlg, IDC_CUSTSTATBUSY ) ? ID_STATUS_AWAY : ID_STATUS_ONLINE);
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

INT_PTR __cdecl CYahooProto::SetCustomStatCommand( WPARAM wParam, LPARAM lParam )
{
	if ( !m_bLoggedIn ) {
		ShowNotification(Translate("Yahoo Error"), Translate("You need to be connected to set the custom message"), NIIF_ERROR);
		return 0;
	}
	
	HWND hwndSetCustomStatus = CreateDialogParam(hInstance, MAKEINTRESOURCE( IDD_SETCUSTSTAT ), NULL, DlgProcSetCustStat, ( LPARAM )this );
	SetForegroundWindow( hwndSetCustomStatus );
	SetFocus( hwndSetCustomStatus );
 	ShowWindow( hwndSetCustomStatus, SW_SHOW );
	return 0;
}

//=======================================================
//Open URL
//=======================================================
void CYahooProto::OpenURL(const char *url, int autoLogin)
{
	char tUrl[ 4096 ];

	DebugLog("[YahooOpenURL] url: %s Auto Login: %d", url, autoLogin);
	
	if (autoLogin && GetByte( "MailAutoLogin", 0 ) && m_bLoggedIn && m_id > 0) {
		char  *y, *t, *u;
		
		y = yahoo_urlencode(yahoo_get_cookie(m_id, "y"));
		t = yahoo_urlencode(yahoo_get_cookie(m_id, "t"));
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

	DebugLog("[YahooOpenURL] url: %s Final URL: %s", url, tUrl);
	
	CallService( MS_UTILS_OPENURL, TRUE, ( LPARAM )tUrl );    
}

//=======================================================
//Show buddy profile
//=======================================================
INT_PTR __cdecl CYahooProto::OnShowProfileCommand( WPARAM wParam, LPARAM lParam )
{
	char tUrl[ 4096 ];
	DBVARIANT dbv;

	if ( DBGetContactSettingString(( HANDLE )wParam, m_szModuleName, "yahoo_id", &dbv ))
		return 0;
		
	_snprintf( tUrl, sizeof( tUrl ), "http://profiles.yahoo.com/%s", dbv.pszVal  );
	DBFreeVariant( &dbv );
	
	OpenURL(tUrl, 0);
	return 0;
}

INT_PTR __cdecl CYahooProto::OnEditMyProfile( WPARAM wParam, LPARAM lParam )
{
	OpenURL("http://edit.yahoo.com/config/eval_profile", 1);
	return 0;
}

//=======================================================
//Show My profile
//=======================================================
INT_PTR __cdecl CYahooProto::OnShowMyProfileCommand( WPARAM wParam, LPARAM lParam )
{
	DBVARIANT dbv;
	
	if ( getString( YAHOO_LOGINID, &dbv ) != 0)	{
		ShowError(Translate("Yahoo Error"), Translate("Please enter your yahoo id in Options/Network/Yahoo"));
		return 0;
	}

	char tUrl[ 4096 ];
	_snprintf( tUrl, sizeof( tUrl ), "http://profiles.yahoo.com/%s", dbv.pszVal  );
	DBFreeVariant( &dbv );

	OpenURL(tUrl, 0);
	return 0;
}

//=======================================================
//Show Goto mailbox
//=======================================================
INT_PTR __cdecl CYahooProto::OnGotoMailboxCommand( WPARAM wParam, LPARAM lParam )
{
	if (GetByte( "YahooJapan", 0 ))
		OpenURL("http://mail.yahoo.co.jp/", 1);
	else
		OpenURL("http://mail.yahoo.com/", 1);
	
	return 0;
}

INT_PTR __cdecl CYahooProto::OnABCommand( WPARAM wParam, LPARAM lParam )
{
	OpenURL("http://address.yahoo.com/yab/", 1);
	return 0;
}

INT_PTR __cdecl CYahooProto::OnCalendarCommand( WPARAM wParam, LPARAM lParam )
{
	OpenURL("http://calendar.yahoo.com/", 1);		
	return 0;
}

//=======================================================
//Refresh Yahoo
//=======================================================
INT_PTR __cdecl CYahooProto::OnRefreshCommand( WPARAM wParam, LPARAM lParam )
{
	if ( !m_bLoggedIn ){
		ShowNotification(Translate("Yahoo Error"), Translate("You need to be connected to refresh your buddy list"), NIIF_ERROR);
		return 0;
	}

	yahoo_refresh(m_id);
	return 0;
}

int __cdecl CYahooProto::OnIdleEvent(WPARAM wParam, LPARAM lParam)
{
	BOOL bIdle = (lParam & IDF_ISIDLE);

	DebugLog("[YAHOO_IDLE_EVENT] Idle: %s", bIdle ?"Yes":"No");

	if ( lParam & IDF_PRIVACY ) 
		return 0; /* we support Privacy settings */

	if (m_bLoggedIn) {
		/* set me to idle or back */
		if (m_iStatus == ID_STATUS_INVISIBLE)
			DebugLog("[YAHOO_IDLE_EVENT] WARNING: INVISIBLE! Don't change my status!");
		else
			set_status(m_iStatus,m_startMsg,(bIdle) ? 2 : (m_iStatus == ID_STATUS_ONLINE) ? 0 : 1);
	} else {
		DebugLog("[YAHOO_IDLE_EVENT] WARNING: NOT LOGGED IN???");
	}

	return 0;
}

INT_PTR __cdecl CYahooProto::GetUnreadEmailCount(WPARAM wParam, LPARAM lParam)
{
	if ( !m_bLoggedIn )
		return 0;

	return m_unreadMessages;
}

/////////////////////////////////////////////////////////////////////////////////////////

void CYahooProto::MenuInit( void )
{
	char servicefunction[ 200 ];
	lstrcpyA( servicefunction, m_szModuleName );
	char* tDest = servicefunction + lstrlenA( servicefunction );
	
	// Show custom status menu    
	lstrcpyA( tDest, YAHOO_SET_CUST_STAT );
	YCreateService( YAHOO_SET_CUST_STAT, &CYahooProto::SetCustomStatCommand );

	CLISTMENUITEM mi = { 0 };
	mi.ptszName = m_tszUserName;
	mi.flags = CMIF_ROOTPOPUP | CMIF_TCHAR;
	mi.cbSize = sizeof( mi );
	mi.position = -1999901011;
	mi.pszPopupName = (char *)-1;
	mi.hIcon = LoadIconEx( "yahoo" );
	HGENMENU hMenuRoot = (HGENMENU)CallService( MS_CLIST_ADDMAINMENUITEM,  (WPARAM)0, (LPARAM)&mi);
		
	mi.flags &= ~(CMIF_ROOTPOPUP | CMIF_TCHAR);
	mi.flags |= CMIF_CHILDPOPUP;

	mi.hParentMenu = hMenuRoot;
	mi.popupPosition = 500090000;
	mi.position = 500090000;
	mi.hIcon = LoadIconEx( "set_status" );
	mi.pszName = LPGEN( "Set &Custom Status" );
	mi.pszService = servicefunction;
	CallService( MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi );

	// Edit My profile
	lstrcpyA( tDest, YAHOO_EDIT_MY_PROFILE );
	YCreateService( YAHOO_EDIT_MY_PROFILE, &CYahooProto::OnEditMyProfile );

	mi.position = 500090005;
	mi.hIcon = LoadIconEx( "profile" );
	mi.pszName = LPGEN( "&Edit My Profile" );
	mi.pszService = servicefunction;
	CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );

	// Show My profile
	lstrcpyA( tDest, YAHOO_SHOW_MY_PROFILE );
	YCreateService( YAHOO_SHOW_MY_PROFILE, &CYahooProto::OnShowMyProfileCommand );

	mi.position = 500090005;
	mi.hIcon = LoadIconEx( "profile" );
	mi.pszName = LPGEN( "&My Profile" );
	mi.pszService = servicefunction;
	CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );

	// Show Yahoo mail 
	strcpy( tDest, YAHOO_YAHOO_MAIL );
	YCreateService( YAHOO_YAHOO_MAIL, &CYahooProto::OnGotoMailboxCommand );

	mi.position = 500090010;
	mi.hIcon = LoadIconEx( "mail" );
	mi.pszName = LPGEN( "&Yahoo Mail" );
	mi.pszService = servicefunction;
	CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );

	// Show Address Book    
	strcpy( tDest, YAHOO_AB );
	YCreateService( YAHOO_AB, &CYahooProto::OnABCommand );

	mi.position = 500090015;
	mi.hIcon = LoadIconEx( "yab" );
	mi.pszName = LPGEN( "&Address Book" );
	mi.pszService = servicefunction;
	CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );

	// Show Calendar
	strcpy( tDest, YAHOO_CALENDAR );
	YCreateService( YAHOO_CALENDAR, &CYahooProto::OnCalendarCommand );

	mi.position = 500090015;
	mi.hIcon = LoadIconEx( "calendar" );
	mi.pszName = LPGEN( "&Calendar" );
	mi.pszService = servicefunction;
	CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );

	// Show Refresh     
	strcpy( tDest, YAHOO_REFRESH );
	YCreateService( YAHOO_REFRESH, &CYahooProto::OnRefreshCommand );

	mi.position = 500090015;
	mi.hIcon = LoadIconEx( "refresh" );
	mi.pszName = LPGEN( "&Refresh" );
	mi.pszService = servicefunction;
	CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );
	
	// Show Profile 
	strcpy( tDest, YAHOO_SHOW_PROFILE );
	YCreateService( YAHOO_SHOW_PROFILE, &CYahooProto::OnShowProfileCommand );

	mi.position = -2000006000;
	mi.hIcon = LoadIconEx( "profile" );
	mi.pszName = LPGEN( "&Show Profile" );
	mi.pszService = servicefunction;
	mi.pszContactOwner = m_szModuleName;
	CallService( MS_CLIST_ADDCONTACTMENUITEM, 0, ( LPARAM )&mi );
}

//=======================================================================================
// Load the yahoo service/plugin
//=======================================================================================
void CYahooProto::LoadYahooServices( void )
{
	//----| Events hooking |--------------------------------------------------------------
	YHookEvent( ME_OPT_INITIALISE, &CYahooProto::OnOptionsInit );
	YHookEvent( ME_DB_CONTACT_DELETED, &CYahooProto::OnContactDeleted );

	//----| Service creation |------------------------------------------------------------
	YCreateService( PS_CREATEACCMGRUI, &CYahooProto::SvcCreateAccMgrUI);
	
	YCreateService( PS_GETAVATARINFO,  &CYahooProto::GetAvatarInfo );
	YCreateService( PS_GETMYAVATAR,    &CYahooProto::GetMyAvatar );
	YCreateService( PS_SETMYAVATAR,    &CYahooProto::SetMyAvatar );
	YCreateService( PS_GETAVATARCAPS,  &CYahooProto::GetAvatarCaps );

	YCreateService(	PS_GETMYAWAYMSG, &CYahooProto::GetMyAwayMsg);
	YCreateService( YAHOO_SEND_NUDGE, &CYahooProto::SendNudge );
	
	YCreateService( YAHOO_GETUNREAD_EMAILCOUNT, &CYahooProto::GetUnreadEmailCount);

	//----| Set resident variables |------------------------------------------------------
	char path[MAX_PATH];
	mir_snprintf( path, sizeof( path ), "%s/Status", m_szModuleName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/YStatus", m_szModuleName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/YAway", m_szModuleName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/Mobile", m_szModuleName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );
	
	mir_snprintf( path, sizeof( path ), "%s/YGMsg", m_szModuleName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );
	
	mir_snprintf( path, sizeof( path ), "%s/IdleTS", m_szModuleName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );
	
	mir_snprintf( path, sizeof( path ), "%s/PictLastCheck", m_szModuleName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	mir_snprintf( path, sizeof( path ), "%s/PictLoading", m_szModuleName );
	CallService( MS_DB_SETSETTINGRESIDENT, TRUE, ( LPARAM )path );

	//----| Create nudge event |----------------------------------------------------------
	mir_snprintf( path, sizeof( path ), "%s/Nudge");
	hYahooNudge = CreateHookableEvent( path );
}
