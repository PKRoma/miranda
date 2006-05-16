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
#include <windowsx.h>
#include <shlwapi.h>

#include "yahoo.h"
#include "resource.h"

#include <m_langpack.h>
#include <m_utils.h>
#include <m_options.h>
#include <m_popup.h>

/*
 * YahooOptInit - initialize/register our Options w/ Miranda.
 */
int YahooOptInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;
	
	ZeroMemory(&odp, sizeof( odp ));
	odp.cbSize						= sizeof(odp);
	odp.position					= -790000000;
	odp.hInstance					= hinstance;
	odp.pszTemplate				    = MAKEINTRESOURCE(IDD_OPT_YAHOO);
	odp.pszTitle					= Translate( yahooProtocolName );
	odp.pszGroup					= Translate("Network");
	odp.flags						= ODPF_BOLDGROUPS;
	odp.nIDBottomSimpleControl      = IDC_STYAHOOGROUP;
	odp.pfnDlgProc					= DlgProcYahooOpts;
	YAHOO_CallService( MS_OPT_ADDPAGE, wParam,( LPARAM )&odp );

	if ( ServiceExists( MS_POPUP_ADDPOPUP ))
	{
	    OPTIONSDIALOGPAGE 	odp2;
		
		ZeroMemory(&odp2, sizeof( odp2 ));
		
		odp2.cbSize			= sizeof( odp2 );
		odp2.position		= 100000000;
		odp2.hInstance		= hinstance;
		odp2.pszTemplate	= MAKEINTRESOURCE( IDD_OPT_YAHOO_POPUP );
		odp2.pszTitle		= Translate( yahooProtocolName );
		odp2.pszGroup		= Translate("PopUps");
		odp2.groupPosition	= 910000000;
		odp2.flags			= ODPF_BOLDGROUPS;
		odp2.pfnDlgProc		= DlgProcYahooPopUpOpts;
		YAHOO_CallService( MS_OPT_ADDPAGE, wParam,( LPARAM )&odp2 );
	}

	return 0;
}

void SetButtonCheck(HWND hwndDlg, int CtrlID, BOOL bCheck)
{
	HWND hwndCtrl = GetDlgItem(hwndDlg, CtrlID);
	
	if (bCheck)
		Button_SetCheck(hwndCtrl, BST_CHECKED);
}

/*
 * DlgProcYahooOpts - Connection Options Dialog
 */
BOOL CALLBACK DlgProcYahooOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DBVARIANT dbv;
	YList *l;
	
	switch ( msg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg );

		if ( !DBGetContactSetting( NULL, yahooProtocolName, YAHOO_LOGINID, &dbv )) {
			SetDlgItemText(hwndDlg,IDC_HANDLE,dbv.pszVal);
			DBFreeVariant(&dbv);
		}

		if ( !DBGetContactSetting( NULL, yahooProtocolName, YAHOO_PASSWORD, &dbv )) {
			//bit of a security hole here, since it's easy to extract a password from an edit box
			YAHOO_CallService( MS_DB_CRYPT_DECODESTRING, strlen( dbv.pszVal )+1, ( LPARAM )dbv.pszVal );
			SetDlgItemText( hwndDlg, IDC_PASSWORD, dbv.pszVal );
			DBFreeVariant( &dbv );
		}

		if ( !DBGetContactSetting( NULL, yahooProtocolName, YAHOO_LOGINSERVER, &dbv )){
			SetDlgItemText( hwndDlg, IDC_LOGINSERVER, dbv. pszVal );
			DBFreeVariant( &dbv );
		}
		else SetDlgItemText( hwndDlg, IDC_LOGINSERVER, YAHOO_DEFAULT_LOGIN_SERVER );

		SetDlgItemInt( hwndDlg, IDC_YAHOOPORT, YAHOO_GetWord( NULL, YAHOO_LOGINPORT, 5050 ), FALSE );
		
		SetButtonCheck( hwndDlg, IDC_YAHOO_JAPAN, YAHOO_GetByte( "YahooJapan", 0 ) );
		SetButtonCheck( hwndDlg, IDC_DISMAINMENU, YAHOO_GetByte( "DisableMainMenu", 0 ) );
		SetButtonCheck( hwndDlg, IDC_DISABLE_UTF8, YAHOO_GetByte( "DisableUTF8", 0 )); 
		SetButtonCheck( hwndDlg, IDC_USE_YAB, YAHOO_GetByte( "UseYAB", 1 )); 
		SetButtonCheck( hwndDlg, IDC_SHOW_AVATARS, YAHOO_GetByte( "ShowAvatars", 0 )); 
		SetButtonCheck( hwndDlg, IDC_MAIL_AUTOLOGIN, YAHOO_GetByte( "MailAutoLogin", 0 )); 
		SetButtonCheck( hwndDlg, IDC_DISABLEYAHOOMAIL, !YAHOO_GetByte( "DisableYahoomail", 0 ));
		SetButtonCheck( hwndDlg, IDC_SHOW_ERRORS, YAHOO_GetByte( "ShowErrors", 1 )); 

		/* show our current ignore list */
		l = (YList *)YAHOO_GetIgnoreList();
		while (l != NULL) {
			struct yahoo_buddy *b = (struct yahoo_buddy *) l->data;
			
			//MessageBox(NULL, b->id, "ID", MB_OK);
			SendMessage(GetDlgItem(hwndDlg,IDC_YIGN_LIST), LB_INSERTSTRING, 0, (LPARAM)b->id);
			l = l->next;
		}
		return TRUE;

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
    		case IDC_NEWYAHOOACCOUNTLINK:
    			YAHOO_CallService( MS_UTILS_OPENURL, 1, 
				YAHOO_GetByte( "YahooJapan", 0 ) 
				?(LPARAM)"http://edit.yahoo.co.jp/config/eval_register"
				:(LPARAM)"http://edit.yahoo.com/config/eval_register" );
    			return TRUE;
    
    		case IDC_RESETSERVER:
				SetDlgItemText( hwndDlg, IDC_LOGINSERVER, YAHOO_DEFAULT_LOGIN_SERVER );
    			SetDlgItemInt(  hwndDlg, IDC_YAHOOPORT,  5050, FALSE );
    			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
    			break;
    		
			case IDC_IGN_ADD: 
							{
							  char id[128];
							  int i;
							  
							  if (!yahooLoggedIn) {
								MessageBox(hwndDlg, Translate("You need to be connected to Yahoo to add to Ignore List."), Translate("Yahoo Ignore"), MB_OK| MB_ICONINFORMATION);
								break;
							  }

							  
							  i = GetDlgItemText( hwndDlg, IDC_YIGN_EDIT, id, sizeof( id ));
							  
							  if (i < 3) {
								MessageBox(hwndDlg, Translate("Please enter a valid buddy name to ignore."), Translate("Yahoo Ignore"), MB_OK| MB_ICONINFORMATION);
								break;
							  }
							  
							  i = SendMessage(GetDlgItem(hwndDlg,IDC_YIGN_LIST), LB_FINDSTRINGEXACT, -1, (LPARAM)id);
							  if (i != LB_ERR ) {
								MessageBox(hwndDlg, Translate("The buddy is already on your ignore list. "), Translate("Yahoo Ignore"), MB_OK | MB_ICONINFORMATION);
								break;
							  }
							   YAHOO_IgnoreBuddy(id, 0);
							   SendMessage(GetDlgItem(hwndDlg,IDC_YIGN_LIST), LB_INSERTSTRING, -1, (LPARAM)id);
							   SetDlgItemText( hwndDlg, IDC_YIGN_EDIT, "" );
							}  
							break;
			
			case IDC_IGN_REMOVE:
							{
								int i;
								char id[128];
								
								if (!yahooLoggedIn) {
									MessageBox(hwndDlg, Translate("You need to be connected to Yahoo to remove from the Ignore List."), Translate("Yahoo Ignore"), MB_OK| MB_ICONINFORMATION);
									break;
								}
							  
								i = SendMessage(GetDlgItem(hwndDlg,IDC_YIGN_LIST), LB_GETCURSEL, 0, 0);
								if (i == LB_ERR) {
									MessageBox(hwndDlg, Translate("Please select a buddy on the ignore list to remove."), Translate("Yahoo Ignore"), MB_OK| MB_ICONINFORMATION);
									break;
								}
								
								SendMessage(GetDlgItem(hwndDlg,IDC_YIGN_LIST), LB_GETTEXT, i, (LPARAM)id);
								
								YAHOO_IgnoreBuddy(id, 1);
							    SendMessage(GetDlgItem(hwndDlg,IDC_YIGN_LIST), LB_DELETESTRING, i, 0);
							}	
								
							break;
			case IDC_YAHOO_JAPAN:
					SetDlgItemText( hwndDlg, IDC_LOGINSERVER, 
										(IsDlgButtonChecked(hwndDlg,IDC_YAHOO_JAPAN)==BST_CHECKED)
										?YAHOO_DEFAULT_JAPAN_LOGIN_SERVER
										:YAHOO_DEFAULT_LOGIN_SERVER );
					// fall through and enable apply button
				
    		case IDC_DISMAINMENU:
			case IDC_DISABLE_UTF8: 
			case IDC_USE_YAB:	
			case IDC_SHOW_AVATARS:
			case IDC_MAIL_AUTOLOGIN:
			case IDC_SHOW_ERRORS:
			case IDC_DISABLEYAHOOMAIL:
    		    SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
    		    break;
    		}    

    		if ( HIWORD( wParam ) == EN_CHANGE && ( HWND )lParam == GetFocus())
  			switch( LOWORD( wParam )) {
  			case IDC_HANDLE:			
			case IDC_PASSWORD:			
			case IDC_LOGINSERVER:
  			case IDC_YAHOOPORT:			
			
  				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
  			}

		break;



	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == PSN_APPLY )
		{
			BOOL	    reconnectRequired = FALSE, restartRequired = FALSE;
			char	    str[128];
			char		id[128];
			DBVARIANT dbv;

			GetDlgItemText( hwndDlg, IDC_HANDLE, id, sizeof( id ));
			dbv.pszVal = NULL;
			
			if ( DBGetContactSetting( NULL, yahooProtocolName, YAHOO_LOGINID, &dbv ) || 
                  lstrcmp( id, dbv.pszVal ))
				reconnectRequired = TRUE;
				
			if ( dbv.pszVal != NULL ) DBFreeVariant( &dbv );
			
			YAHOO_SetString( NULL, YAHOO_LOGINID, id );

			GetDlgItemText( hwndDlg, IDC_PASSWORD, str, sizeof( str ));
			YAHOO_CallService( MS_DB_CRYPT_ENCODESTRING, sizeof( str ),( LPARAM )str );
			dbv.pszVal = NULL;
			if ( DBGetContactSetting( NULL, yahooProtocolName, YAHOO_PASSWORD, &dbv ) || 
				lstrcmp( str, dbv.pszVal ))
				reconnectRequired = TRUE;
			if ( dbv.pszVal != NULL ) DBFreeVariant( &dbv );
			
			YAHOO_SetString( NULL, YAHOO_PASSWORD, str );

			GetDlgItemText( hwndDlg, IDC_LOGINSERVER, str, sizeof( str ));
			YAHOO_SetString( NULL, YAHOO_LOGINSERVER, str );

			YAHOO_SetWord( NULL, YAHOO_LOGINPORT, GetDlgItemInt( hwndDlg, IDC_YAHOOPORT, NULL, FALSE ));

			YAHOO_SetByte("YahooJapan", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_YAHOO_JAPAN ));
	        YAHOO_SetByte("DisableMainMenu", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_DISMAINMENU ));
			YAHOO_SetByte("DisableUTF8", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_DISABLE_UTF8 )); 
			YAHOO_SetByte("UseYAB", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_USE_YAB )); 
			YAHOO_SetByte("ShowAvatars", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_SHOW_AVATARS )); 
			YAHOO_SetByte("MailAutoLogin", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_MAIL_AUTOLOGIN )); 
	        YAHOO_SetByte("DisableYahoomail", ( BYTE )!IsDlgButtonChecked( hwndDlg, IDC_DISABLEYAHOOMAIL ));
			YAHOO_SetByte("ShowErrors", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_SHOW_ERRORS )); 

			if ( restartRequired )
				MessageBox( hwndDlg, Translate( "The changes you have made require you to restart Miranda IM before they take effect"), Translate("YAHOO Options"), MB_OK );
			else if ( reconnectRequired && yahooLoggedIn )
				MessageBox( hwndDlg, Translate( "The changes you have made require you to reconnect to the Yahoo network before they take effect"), Translate("YAHOO Options"), MB_OK );

			return TRUE;
		}
		break;
	}
	return FALSE;
}


/*
 * DlgProcYahooPopUpOpts - Connection Yahoo Popup Options Dialog
 */

BOOL CALLBACK DlgProcYahooPopUpOpts( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    BOOL usewincolorflag;
    
	switch( msg ) {
	case WM_INITDIALOG:
	{
		BOOL toSet;
		int tTimeout;
		
        TranslateDialogDefault(hwndDlg);
		
		//Colors. First step is configuring the colours.
		SendDlgItemMessage( hwndDlg, IDC_BGCOLOUR, CPM_SETCOLOUR, 0, 
              YAHOO_GetDword( "BackgroundColour", STYLE_DEFAULTBGCOLOUR ));
		SendDlgItemMessage( hwndDlg, IDC_TEXTCOLOUR, CPM_SETCOLOUR, 0, 
              YAHOO_GetDword( "TextColour", GetSysColor( COLOR_WINDOWTEXT )));

		//Second step is disabling them if we want to use default Windows ones.
		toSet = YAHOO_GetByte( "UseWinColors", 0 );
		SetButtonCheck( hwndDlg, IDC_USEWINCOLORS, toSet );
		
		EnableWindow( GetDlgItem( hwndDlg, IDC_BGCOLOUR), !toSet );
		EnableWindow( GetDlgItem( hwndDlg, IDC_TEXTCOLOUR), !toSet );

		tTimeout = YAHOO_GetDword( "PopupTimeout", 3 );
		SetDlgItemInt( hwndDlg, IDC_POPUP_TIMEOUT, tTimeout, FALSE );
		return TRUE;
	}
	case WM_COMMAND:
		if ( (LOWORD(wParam) == IDC_BGCOLOUR || LOWORD(wParam) == IDC_TEXTCOLOUR) && 
			( HIWORD( wParam ) == CPN_COLOURCHANGED ) ) {
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}

		if ((HWND) lParam != GetFocus())
                return 0;
		switch( LOWORD( wParam )) {
    		case IDC_USEWINCOLORS:
                usewincolorflag=IsDlgButtonChecked(hwndDlg, IDC_USEWINCOLORS);
    			EnableWindow( GetDlgItem( hwndDlg, IDC_BGCOLOUR ), !usewincolorflag);
    			EnableWindow( GetDlgItem( hwndDlg, IDC_TEXTCOLOUR ), !usewincolorflag);
    			SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
                break;
    
    		case IDC_PREVIEW:
    			YAHOO_ShowPopup( Translate( "New Mail (99 msgs)" ), Translate( "From: Sample User\nSubject: Testing123." ), YAHOO_MAIL_POPUP );
    			break;
    
		}
		if (HIWORD(wParam) == EN_CHANGE) // Valid the Apply button if any change are done.
    		SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);

		break;
    
	case WM_NOTIFY: //Here we have pressed either the OK or the APPLY button.
			switch (((LPNMHDR)lParam)->code) {
			   case PSN_APPLY:
					YAHOO_SetDword("PopupTimeout", GetDlgItemInt( hwndDlg, IDC_POPUP_TIMEOUT, NULL, FALSE ) );
				    YAHOO_SetDword("TextColour",SendDlgItemMessage(hwndDlg,IDC_TEXTCOLOUR,CPM_GETCOLOUR,0,0));
				    YAHOO_SetDword("BackgroundColour",SendDlgItemMessage(hwndDlg,IDC_BGCOLOUR,CPM_GETCOLOUR,0,0));
    			    YAHOO_SetByte("UseWinColors",IsDlgButtonChecked(hwndDlg, IDC_USEWINCOLORS));
 			        break;
			}
			break;
	}
    return FALSE;
}
