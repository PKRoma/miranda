/*
IRC plugin for Miranda IM

Copyright (C) 2003-2005 Jurgen Persson
Copyright (C) 2007 George Hazan

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

extern HWND nick_hWnd;
extern HWND list_hWnd;
extern HWND whois_hWnd;
extern HWND join_hWnd;
extern HWND quickconn_hWnd;
extern char * pszServerFile;
static WNDPROC OldMgrEditProc;
extern HWND manager_hWnd;
extern HMODULE m_ssleay32;

// Callback for the 'CTCP accept' dialog
BOOL CALLBACK MessageboxWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static DCCINFO* pdci = NULL;
	switch ( uMsg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg);
		pdci = (DCCINFO *) lParam;
		SendMessage(GetDlgItem( hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadImage(NULL,IDI_QUESTION,IMAGE_ICON,48, 48,LR_SHARED), 0);
		break;
		
	case WM_COMMAND:
		if ( HIWORD(wParam) == BN_CLICKED ) {
			switch ( LOWORD( wParam )) {
			case IDN_YES:
				{
					CDccSession * dcc = new CDccSession(pdci);

					CDccSession * olddcc = g_ircSession.FindDCCSession(pdci->hContact);
					if (olddcc)
						olddcc->Disconnect();
					g_ircSession.AddDCCSession(pdci->hContact, dcc);

					dcc->Connect();

					PostMessage ( hwndDlg, WM_CLOSE, 0,0);
				}
				break;
			case IDN_NO:
				PostMessage ( hwndDlg, WM_CLOSE, 0,0);
				break;
		}	}
		break;
		
	case WM_CLOSE:
		DestroyWindow( hwndDlg);
		break;
	}

	return FALSE;
}

// Callback for the /WHOIS dialog
BOOL CALLBACK InfoWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg);
		{
			HFONT hFont;
			LOGFONT lf;
			hFont = ( HFONT )SendDlgItemMessage( hwndDlg, IDC_CAPTION, WM_GETFONT, 0, 0 );
			GetObject( hFont, sizeof( lf ), &lf );
			lf.lfHeight = (int)(lf.lfHeight*1.2);
			lf.lfWeight = FW_BOLD;
			hFont = CreateFontIndirect( &lf );
			SendDlgItemMessage( hwndDlg, IDC_CAPTION, WM_SETFONT, ( WPARAM )hFont, 0 );
		}
		SendMessage(GetDlgItem( hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadIconEx(IDI_LOGO), 0);
		SendMessage(GetDlgItem( hwndDlg, IDC_INFO_NAME), EM_SETREADONLY, true, 0);
		SendMessage(GetDlgItem( hwndDlg, IDC_INFO_ID), EM_SETREADONLY, true, 0);
		SendMessage(GetDlgItem( hwndDlg, IDC_INFO_ADDRESS), EM_SETREADONLY, true, 0);
		SendMessage(GetDlgItem( hwndDlg, IDC_INFO_CHANNELS), EM_SETREADONLY, true, 0);
		SendMessage(GetDlgItem( hwndDlg, IDC_INFO_AUTH), EM_SETREADONLY, true, 0);
		SendMessage(GetDlgItem( hwndDlg, IDC_INFO_SERVER), EM_SETREADONLY, true, 0);
		SendMessage(GetDlgItem( hwndDlg, IDC_INFO_AWAY2), EM_SETREADONLY, true, 0);
		SendMessage(GetDlgItem( hwndDlg, IDC_INFO_OTHER), EM_SETREADONLY, true, 0);
		SendMessage( hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)LoadIconEx(IDI_WHOIS)); // Tell the dialog to use it
		break;

	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		if (( HWND )lParam == GetDlgItem( hwndDlg, IDC_WHITERECT ) ||
			 ( HWND )lParam == GetDlgItem( hwndDlg, IDC_TEXT ) ||
			 ( HWND )lParam == GetDlgItem( hwndDlg, IDC_CAPTION ) ||
			 ( HWND )lParam == GetDlgItem( hwndDlg, IDC_LOGO ))
			{
				SetTextColor((HDC)wParam,RGB(0,0,0));
				SetBkColor((HDC)wParam,RGB(255,255,255));
				return (BOOL)GetStockObject(WHITE_BRUSH);
			}
		break;
		
	case WM_COMMAND:
		if( HIWORD( wParam ) == BN_CLICKED ) {
			TCHAR szTemp[255];
			GetDlgItemText( hwndDlg, IDC_INFO_NICK, szTemp, SIZEOF(szTemp));
			switch ( LOWORD( wParam )) {
			case ID_INFO_OK:
				PostMessage ( hwndDlg, WM_CLOSE, 0,0);
				break;
			case ID_INFO_GO:
				PostIrcMessage( _T("/WHOIS %s"), szTemp);
				break;
			case ID_INFO_QUERY:
				PostIrcMessage( _T("/QUERY %s"), szTemp);
				break;
			case IDC_PING:
				SetDlgItemText( hwndDlg, IDC_REPLY, TranslateT("Please wait..."));
				PostIrcMessage( _T("/PRIVMSG %s \001PING %u\001"), szTemp, time(0));
				break;
			case IDC_USERINFO:
				SetDlgItemText( hwndDlg, IDC_REPLY, TranslateT("Please wait..."));
				PostIrcMessage( _T("/PRIVMSG %s \001USERINFO\001"), szTemp);
				break;
			case IDC_TIME:
				SetDlgItemText( hwndDlg, IDC_REPLY, TranslateT("Please wait..."));
				PostIrcMessage( _T("/PRIVMSG %s \001TIME\001"), szTemp);
				break;
			case IDC_VERSION:
				SetDlgItemText( hwndDlg, IDC_REPLY, TranslateT("Please wait..."));
				PostIrcMessage( _T("/PRIVMSG %s \001VERSION\001"), szTemp);
				break;
		}	}
		break;
	
	case WM_CLOSE:
		ShowWindow( hwndDlg, SW_HIDE);
		SendMessage( hwndDlg, WM_SETREDRAW, FALSE, 0);
		break;
		
	case WM_DESTROY:
		{
			HFONT hFont;
			hFont=(HFONT)SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( hwndDlg,IDNOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				
			whois_hWnd = NULL;
		}
		break;
	}
	return FALSE;
}

// Callback for the 'Change nickname' dialog
BOOL CALLBACK NickWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg);
		{
			HFONT hFont;
			LOGFONT lf;
			hFont=(HFONT)SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			GetObject(hFont,sizeof(lf),&lf);
			lf.lfHeight=(int)(lf.lfHeight*1.2);
			lf.lfWeight=FW_BOLD;
			hFont=CreateFontIndirect(&lf);
			SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem( hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadIconEx(IDI_LOGO), 0);

			DBVARIANT dbv;
  			if ( !DBGetContactSettingTString(NULL, IRCPROTONAME, "RecentNicks", &dbv)) {
				for (int i = 0; i<10; i++)
					if ( !GetWord( dbv.ptszVal, i).empty())
						SendDlgItemMessage( hwndDlg, IDC_ENICK, CB_ADDSTRING, 0, (LPARAM)GetWord(dbv.ptszVal, i).c_str());

				DBFreeVariant(&dbv);
		}	}
		break;

	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem( hwndDlg,IDC_WHITERECT) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_TEXT) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_CAPTION) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_LOGO ))
		{
			SetTextColor((HDC)wParam,RGB(0,0,0));
			SetBkColor((HDC)wParam,RGB(255,255,255));
			return (BOOL)GetStockObject(WHITE_BRUSH);
		}
		break;
		
	case WM_COMMAND:
		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNCANCEL )
			PostMessage( hwndDlg, WM_CLOSE, 0, 0 );

		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNOK ) {
			TCHAR szTemp[255];
			GetDlgItemText( hwndDlg, IDC_ENICK, szTemp, SIZEOF(szTemp));
			PostIrcMessage( _T("/NICK %s"), szTemp);

			TString S = szTemp; 
			DBVARIANT dbv;
	  		if ( !DBGetContactSettingTString( NULL, IRCPROTONAME, "RecentNicks", &dbv )) {
				for ( int i = 0; i<10; i++ ) {
					TString s = GetWord(dbv.ptszVal, i);
					if ( !s.empty() && s != szTemp) {
						S += _T(" ");
						S += s;
				}	}
				DBFreeVariant(&dbv);
			}
			DBWriteContactSettingTString( NULL, IRCPROTONAME, "RecentNicks", S.c_str());
			SendMessage( hwndDlg, WM_CLOSE, 0,0);
		}
		break;
		
	case WM_CLOSE:
		DestroyWindow( hwndDlg);
		break;
		
	case WM_DESTROY:
		{
			HFONT hFont;
			hFont=(HFONT)SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( hwndDlg,IDNOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				
			nick_hWnd = NULL;
		}
		break;
	}

	return FALSE;
}

BOOL CALLBACK ListWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch( uMsg ) {
	case IRC_UPDATELIST:
		{
			int j = ListView_GetItemCount(GetDlgItem( hwndDlg, IDC_INFO_LISTVIEW));
			if ( j > 0 ) {
				LVITEM lvm;
				lvm.mask= LVIF_PARAM;
				lvm.iSubItem = 0;
				for ( int i =0; i < j; i++ ) {
					lvm.iItem = i;
					lvm.lParam = i;
					ListView_SetItem(GetDlgItem( hwndDlg, IDC_INFO_LISTVIEW),&lvm);
		}	}	}
		break;

	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg);
		{
			RECT screen;

			SystemParametersInfo(SPI_GETWORKAREA, 0,  &screen, 0); 
			LVCOLUMN lvC;
			int COLUMNS_SIZES[4] ={200, 50,50,2000};
			TCHAR szBuffer[32];

			lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvC.fmt = LVCFMT_LEFT;
			for ( int index=0;index < 4; index++ ) {
				lvC.iSubItem = index;
				lvC.cx = COLUMNS_SIZES[index];

				switch (index)
				{
					case 0: lstrcpy( szBuffer, TranslateT("Channel")); break;
					case 1: lstrcpy( szBuffer, _T("#"));               break;
					case 2: lstrcpy( szBuffer, TranslateT("Mode"));    break;
					case 3: lstrcpy( szBuffer, TranslateT("Topic"));   break;
				}
				lvC.pszText = szBuffer;
				ListView_InsertColumn(GetDlgItem( hwndDlg, IDC_INFO_LISTVIEW),index,&lvC);
			}
			
			SetWindowPos( hwndDlg, HWND_TOP, (screen.right-screen.left)/2- (prefs->ListSize.x)/2,(screen.bottom-screen.top)/2- (prefs->ListSize.y)/2, (prefs->ListSize.x), (prefs->ListSize.y), 0);
			SendMessage( hwndDlg, WM_SIZE, SIZE_RESTORED, MAKELPARAM(prefs->ListSize.x, prefs->ListSize.y));
			ListView_SetExtendedListViewStyle(GetDlgItem( hwndDlg, IDC_INFO_LISTVIEW), LVS_EX_FULLROWSELECT);
			SendMessage( hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)LoadIconEx(IDI_LIST)); // Tell the dialog to use it
		}
		break;

	case WM_SIZE:
		{
			RECT winRect;
			GetClientRect( hwndDlg, &winRect);
			RECT buttRect;
			GetWindowRect(GetDlgItem( hwndDlg, IDC_JOIN), &buttRect);
			SetWindowPos(GetDlgItem( hwndDlg, IDC_INFO_LISTVIEW), HWND_TOP, 4, 4, winRect.right-8, winRect.bottom-36, 0 );
			SetWindowPos(GetDlgItem( hwndDlg, IDC_CLOSE), HWND_TOP, winRect.right-84, winRect.bottom-28, buttRect.right- buttRect.left, buttRect.bottom- buttRect.top, 0 );
			SetWindowPos(GetDlgItem( hwndDlg, IDC_JOIN), HWND_TOP, winRect.right-174,  winRect.bottom-28, buttRect.right- buttRect.left, buttRect.bottom- buttRect.top, 0 );
			SetWindowPos(GetDlgItem( hwndDlg, IDC_TEXT), HWND_TOP, 4,  winRect.bottom-28, winRect.right-200, buttRect.bottom- buttRect.top, 0 );

			GetWindowRect( hwndDlg, &winRect);
			prefs->ListSize.x = winRect.right-winRect.left;
			prefs->ListSize.y = winRect.bottom-winRect.top;
			DBWriteContactSettingDword(NULL,IRCPROTONAME, "SizeOfListBottom", prefs->ListSize.y);
			DBWriteContactSettingDword(NULL,IRCPROTONAME, "SizeOfListRight", prefs->ListSize.x);
		}
		return 0;		

	case WM_COMMAND:
		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_CLOSE )
			PostMessage ( hwndDlg, WM_CLOSE, 0,0);

		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_JOIN ) {
			TCHAR szTemp[255];
			int i = ListView_GetSelectionMark( GetDlgItem( hwndDlg, IDC_INFO_LISTVIEW));
			ListView_GetItemText( GetDlgItem( hwndDlg, IDC_INFO_LISTVIEW), i, 0, szTemp, 255);
			PostIrcMessage( _T("/JOIN %s"), szTemp );
		}
		break;
		
	case WM_NOTIFY :
		switch (((NMHDR*)lParam)->code) {
		case NM_DBLCLK:
			{
				TCHAR szTemp[255];
				int i = ListView_GetSelectionMark(GetDlgItem( hwndDlg, IDC_INFO_LISTVIEW));
				ListView_GetItemText(GetDlgItem( hwndDlg, IDC_INFO_LISTVIEW), i, 0, szTemp, SIZEOF(szTemp));
				PostIrcMessage( _T("/JOIN %s"), szTemp);
			}
			break;
				
		case LVN_COLUMNCLICK:
			{
				LPNMLISTVIEW lv = (LPNMLISTVIEW)lParam;
				SendMessage(GetDlgItem( hwndDlg, IDC_INFO_LISTVIEW), LVM_SORTITEMS, (WPARAM)lv->iSubItem, (LPARAM)ListViewSort);
				SendMessage(list_hWnd, IRC_UPDATELIST, 0, 0);
			}
			break;
		}
		break;
		
	case WM_CLOSE:
		DestroyWindow( hwndDlg);
		break;

   case WM_DESTROY:
		list_hWnd = NULL;
		break;
	}

	return FALSE;
}

BOOL CALLBACK JoinWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch( uMsg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg);					
		{
			HFONT hFont;
			LOGFONT lf;
			hFont=(HFONT)SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			GetObject(hFont,sizeof(lf),&lf);
			lf.lfHeight=(int)(lf.lfHeight*1.2);
			lf.lfWeight=FW_BOLD;
			hFont=CreateFontIndirect(&lf);
			SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem( hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadIconEx(IDI_LOGO), 0);

			DBVARIANT dbv;
  			if ( !DBGetContactSettingTString( NULL, IRCPROTONAME, "RecentChannels", &dbv)) {
				for ( int i = 0; i < 20; i++ ) {
					if ( !GetWord( dbv.ptszVal, i).empty()) {
						TString S = ReplaceString( GetWord(dbv.ptszVal, i), _T("%newl"), _T(" "));
						SendDlgItemMessage( hwndDlg, IDC_ENICK, CB_ADDSTRING, 0, (LPARAM)S.c_str());
				}	}
				DBFreeVariant(&dbv);
		}	}
		break;
		
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem( hwndDlg,IDC_WHITERECT) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_TEXT) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_CAPTION) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_LOGO))
			{
				SetTextColor((HDC)wParam,RGB(0,0,0));
				SetBkColor((HDC)wParam,RGB(255,255,255));
				return (BOOL)GetStockObject(WHITE_BRUSH);
			}
		break;
		
	case WM_COMMAND:
		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNCANCEL )
			PostMessage( hwndDlg, WM_CLOSE, 0, 0 );

		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNOK ) {
			TCHAR szTemp[255];
			GetDlgItemText( hwndDlg, IDC_ENICK, szTemp, SIZEOF(szTemp));
			if ( IsChannel( szTemp ))
				PostIrcMessage( _T("/JOIN %s"), szTemp );
			else
				PostIrcMessage( _T("/JOIN #%s"), szTemp );

			TString S = ReplaceString( szTemp, _T(" "), _T("%newl"));
			TString SL = S;
			
			DBVARIANT dbv;
			if ( !DBGetContactSettingTString( NULL, IRCPROTONAME, "RecentChannels", &dbv)) {
				for (int i = 0; i < 20; i++ ) {
					if ( !GetWord(dbv.ptszVal, i).empty() && GetWord(dbv.ptszVal, i) != SL) {
						S += _T(" ");
						S += GetWord(dbv.ptszVal, i);
				}	}
				DBFreeVariant(&dbv);
			}
			DBWriteContactSettingTString(NULL, IRCPROTONAME, "RecentChannels", S.c_str());
			PostMessage ( hwndDlg, WM_CLOSE, 0,0);
		}
		break;

	case WM_CLOSE:
		DestroyWindow( hwndDlg);
		break;

	case WM_DESTROY:
		{
			HFONT hFont;
			hFont=(HFONT)SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( hwndDlg,IDNOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				
			join_hWnd = NULL;
		}
		break;
	}

	return FALSE;
}

BOOL CALLBACK InitWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch( uMsg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg);					
		{
			HFONT hFont;
			LOGFONT lf;
			hFont=(HFONT)SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			GetObject(hFont,sizeof(lf),&lf);
			lf.lfHeight=(int)(lf.lfHeight*1.2);
			lf.lfWeight=FW_BOLD;
			hFont=CreateFontIndirect(&lf);
			SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem( hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadIconEx(IDI_LOGO), 0);
		}
		break;

	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem( hwndDlg,IDC_WHITERECT) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_TEXT) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_CAPTION) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_LOGO))
		{
			SetTextColor((HDC)wParam,RGB(0,0,0));
			SetBkColor((HDC)wParam,RGB(255,255,255));
			return (BOOL)GetStockObject(WHITE_BRUSH);
		}
		break;

	case WM_COMMAND:
		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNCANCEL )
			PostMessage( hwndDlg, WM_CLOSE, 0, 0 );

		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNOK ) {
			int i = SendMessage( GetDlgItem( hwndDlg, IDC_EDIT), WM_GETTEXTLENGTH, 0, 0);
			int j = SendMessage( GetDlgItem( hwndDlg, IDC_EDIT2), WM_GETTEXTLENGTH, 0, 0);
			if (i >0 && j > 0) {
				TCHAR l[30], m[200];
				GetDlgItemText( hwndDlg, IDC_EDIT, l, SIZEOF(l));
				GetDlgItemText( hwndDlg, IDC_EDIT2, m, SIZEOF(m));

				DBWriteContactSettingTString(NULL, IRCPROTONAME, "PNick", l);
				DBWriteContactSettingTString(NULL, IRCPROTONAME, "Nick", l);
				DBWriteContactSettingTString(NULL, IRCPROTONAME, "Name", m);
				lstrcpyn( prefs->Nick, l, 30 );
				lstrcpyn( prefs->Name, m, 200 );
				if ( lstrlen( prefs->AlternativeNick ) == 0) {
					TCHAR szTemp[30];
					mir_sntprintf(szTemp, SIZEOF(szTemp), _T("%s%u"), l, rand()%9999);
					DBWriteContactSettingTString(NULL, IRCPROTONAME, "AlernativeNick", szTemp);
					lstrcpyn(prefs->AlternativeNick, szTemp, 30);					
			}	}

			PostMessage( hwndDlg, WM_CLOSE, 0, 0 );
		}
		break;
		
	case WM_CLOSE:
		DestroyWindow( hwndDlg);
		break;

	case WM_DESTROY:
		{
			HFONT hFont;
			hFont=(HFONT)SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( hwndDlg,IDNOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				
		}
		break;
	}

	return FALSE;
}

BOOL CALLBACK QuickWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch( uMsg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg);
		{
			HFONT hFont;
			LOGFONT lf;
			hFont=(HFONT)SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			GetObject(hFont,sizeof(lf),&lf);
			lf.lfHeight=(int)(lf.lfHeight*1.2);
			lf.lfWeight=FW_BOLD;
			hFont=CreateFontIndirect(&lf);
			SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem( hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadIconEx(IDI_LOGO), 0);
			
			char * p1 = pszServerFile;
			char * p2 = pszServerFile;
			if ( pszServerFile ) 
				while (strchr(p2, 'n'))
				{
					SERVER_INFO* pData = new SERVER_INFO;
					p1 = strchr(p2, '=');
					++p1;
					p2 = strstr(p1, "SERVER:");
					pData->Name = new char[ p2-p1+1 ];
					lstrcpynA(pData->Name, p1, p2-p1+1);
					
					p1 = strchr(p2, ':');
					++p1;
					pData->iSSL = 0;
					if(strstr(p1, "SSL") == p1)
					{
						p1 +=3;
						if(*p1 == '1')
							pData->iSSL = 1;
						else if(*p1 == '2')
							pData->iSSL = 2;
						p1++;
					}

					p2 = strchr(p1, ':');
					pData->Address=new char[p2-p1+1];
					lstrcpynA(pData->Address, p1, p2-p1+1);
					
					p1 = p2;
					p1++;
					while (*p2 !='G' && *p2 != '-')
						p2++;
					pData->PortStart = new char[p2-p1+1];
					lstrcpynA(pData->PortStart, p1, p2-p1+1);

					if (*p2 == 'G'){
						pData->PortEnd = new char[p2-p1+1];
						lstrcpyA(pData->PortEnd, pData->PortStart);
					} else {
						p1 = p2;
						p1++;
						p2 = strchr(p1, 'G');
						pData->PortEnd = new char[p2-p1+1];
						lstrcpynA(pData->PortEnd, p1, p2-p1+1);
					}

					p1 = strchr(p2, ':');
					p1++;
					p2 = strchr(p1, '\r');
					if (!p2)
						p2 = strchr(p1, '\n');
					if (!p2)
						p2 = strchr(p1, '\0');
					pData->Group = new char[p2-p1+1];
					lstrcpynA(pData->Group, p1, p2-p1+1);
					int iItem = SendDlgItemMessage( hwndDlg, IDC_SERVERCOMBO, CB_ADDSTRING,0,(LPARAM) pData->Name);
					SendDlgItemMessage( hwndDlg, IDC_SERVERCOMBO, CB_SETITEMDATA, iItem,(LPARAM) pData);
				}
				else
					EnableWindow(GetDlgItem( hwndDlg, IDNOK), false);
					
				SendMessage(GetDlgItem( hwndDlg, IDC_SERVER), EM_SETREADONLY, true, 0);
				SendMessage(GetDlgItem( hwndDlg, IDC_PORT), EM_SETREADONLY, true, 0);
				SendMessage(GetDlgItem( hwndDlg, IDC_PORT2), EM_SETREADONLY, true, 0);

				if (prefs->QuickComboSelection != -1)
				{
					SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_SETCURSEL, prefs->QuickComboSelection,0);				
					SendMessage( hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO, CBN_SELCHANGE), 0);
				}
				else
					EnableWindow(GetDlgItem( hwndDlg, IDNOK), false);
			
		} 
		break;

	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem( hwndDlg,IDC_WHITERECT) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_TEXT) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_CAPTION) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_LOGO))
		{
			SetTextColor((HDC)wParam,RGB(0,0,0));
			SetBkColor((HDC)wParam,RGB(255,255,255));
			return (BOOL)GetStockObject(WHITE_BRUSH);
		}
		break;

	case WM_COMMAND:
		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNCANCEL )
			PostMessage ( hwndDlg, WM_CLOSE, 0,0);

		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNOK ) {
			GetDlgItemTextA( hwndDlg, IDC_SERVER, prefs->ServerName, SIZEOF(prefs->ServerName));				 
			GetDlgItemTextA( hwndDlg, IDC_PORT,   prefs->PortStart,  SIZEOF(prefs->PortStart));
			GetDlgItemTextA( hwndDlg, IDC_PORT2,  prefs->PortEnd,    SIZEOF(prefs->PortEnd));
			GetDlgItemTextA( hwndDlg, IDC_PASS,   prefs->Password,   SIZEOF(prefs->Password));
			
			int i = SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
			SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
			if ( pData && (int)pData != CB_ERR ) {
				lstrcpyA( prefs->Network, pData->Group ); 
				if( m_ssleay32 )
					prefs->iSSL = pData->iSSL;
				else
					prefs->iSSL = 0;
			}
			
			TCHAR windowname[20];
			GetWindowText( hwndDlg, windowname, 20);
			if ( lstrcmpi(windowname, _T("Miranda IRC")) == 0 ) {
				prefs->ServerComboSelection = SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
				DBWriteContactSettingDword(NULL,IRCPROTONAME,"ServerComboSelection",prefs->ServerComboSelection);
				DBWriteContactSettingString(NULL,IRCPROTONAME,"ServerName",prefs->ServerName);
				DBWriteContactSettingString(NULL,IRCPROTONAME,"PortStart",prefs->PortStart);
				DBWriteContactSettingString(NULL,IRCPROTONAME,"PortEnd",prefs->PortEnd);
				CallService(MS_DB_CRYPT_ENCODESTRING, 499, (LPARAM)prefs->Password);
				DBWriteContactSettingString(NULL,IRCPROTONAME,"Password",prefs->Password);
				CallService(MS_DB_CRYPT_DECODESTRING, 499, (LPARAM)prefs->Password);
				DBWriteContactSettingString(NULL,IRCPROTONAME,"Network",prefs->Network);
				DBWriteContactSettingByte(NULL,IRCPROTONAME,"UseSSL",prefs->iSSL);
			}
			prefs->QuickComboSelection = SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
			DBWriteContactSettingDword(NULL,IRCPROTONAME,"QuickComboSelection",prefs->QuickComboSelection);
			DisconnectFromServer();
			ConnectToServer();
			PostMessage ( hwndDlg, WM_CLOSE, 0,0);
		}

		if ( HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_SERVERCOMBO ) {
			int i = SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
			SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
			if ( i != CB_ERR ) {
				SetDlgItemTextA( hwndDlg,IDC_SERVER,pData->Address);
				SetDlgItemTextA( hwndDlg,IDC_PORT,pData->PortStart);
				SetDlgItemTextA( hwndDlg,IDC_PORT2,pData->PortEnd);
				SetDlgItemTextA( hwndDlg,IDC_PASS,"");
				if ( m_ssleay32 ) {
					if ( pData->iSSL == 0 )
						SetDlgItemText( hwndDlg, IDC_SSL, TranslateT("Off"));
					if ( pData->iSSL == 1 )
						SetDlgItemText( hwndDlg, IDC_SSL, TranslateT("Auto"));
					if ( pData->iSSL == 2 )
						SetDlgItemText( hwndDlg, IDC_SSL, TranslateT("On"));
				}
				else SetDlgItemText( hwndDlg, IDC_SSL, TranslateT("N/A"));

				EnableWindow(GetDlgItem( hwndDlg, IDNOK), true);
		}	}
		break;
		
	case WM_CLOSE:
		DestroyWindow( hwndDlg);
		break;
		
	case WM_DESTROY:
		{			
			HFONT hFont = ( HFONT )SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( hwndDlg,IDNOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				
			
			int j = (int) SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETCOUNT, 0, 0);
			for ( int index2 = 0; index2 < j; index2++ ) {
				SERVER_INFO* pData = ( SERVER_INFO* )SendMessage(GetDlgItem( hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, index2, 0);
				delete []pData->Name;
				delete []pData->Address;
				delete []pData->PortStart;
				delete []pData->PortEnd;
				delete []pData->Group;
				delete pData;						
			}
			
			quickconn_hWnd = NULL;
		}
		break;
	}

	return FALSE;
}

int CALLBACK ListViewSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	if (!list_hWnd)
		return 0;

	TCHAR temp1[512];
	TCHAR temp2[512];
	LVITEM lvm;
	lvm.mask = LVIF_TEXT;
	lvm.iItem = lParam1;
	lvm.iSubItem = lParamSort;
	lvm.pszText = temp1;
	lvm.cchTextMax = 511;
	SendMessage(GetDlgItem(list_hWnd, IDC_INFO_LISTVIEW), LVM_GETITEM, 0, (LPARAM)&lvm);
	lvm.iItem = lParam2;
	lvm.pszText = temp2;
	SendMessage(GetDlgItem(list_hWnd, IDC_INFO_LISTVIEW), LVM_GETITEM, 0, (LPARAM)&lvm);
	if (lParamSort != 1){
		if (lstrlen(temp1) != 0 && lstrlen(temp2) !=0)
			return lstrcmpi(temp1, temp2);
		
		return ( *temp1 == 0 ) ? 1 : -1;
	}

	return ( StrToInt(temp1) < StrToInt(temp2)) ? 1 : -1;
}

static void __cdecl AckUserInfoSearch(void * lParam)
{
	ProtoBroadcastAck(IRCPROTONAME, (void*)lParam, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

#define STR_BASIC "Faster! Searches the network for an exact match of the nickname only. The hostmask is optional and provides further security if used. Wildcards (? and *) are allowed."		
#define STR_ADVANCED "Slower! Searches the network for nicknames matching a wildcard string. The hostmask is mandatory and a minimum of 4 characters is necessary in the \"Nick\" field. Wildcards (? and *) are allowed."		
#define STR_ERROR "Settings could not be saved!\n\nThe \"Nick\" field must contain at least four characters including wildcards,\n and it must also match the default nickname for this contact."		
#define STR_ERROR2 "Settings could not be saved!\n\nA full hostmask must be set for this online detection mode to work."		

BOOL CALLBACK UserDetailsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact;
	hContact = (HANDLE) GetWindowLong( hwndDlg, GWL_USERDATA);
	switch (msg) {
	case WM_INITDIALOG:
		{
			DBVARIANT dbv;
			hContact = (HANDLE) lParam;
			BYTE bAdvanced = DBGetContactSettingByte( hContact, IRCPROTONAME, "AdvancedMode", 0);

			TranslateDialogDefault( hwndDlg);

			SetWindowLong( hwndDlg, GWL_USERDATA, (LONG) hContact);

			CheckDlgButton( hwndDlg, IDC_RADIO1, bAdvanced?BST_UNCHECKED:BST_CHECKED);
			CheckDlgButton( hwndDlg, IDC_RADIO2, bAdvanced?BST_CHECKED:BST_UNCHECKED);
			EnableWindow(GetDlgItem( hwndDlg, IDC_WILDCARD), bAdvanced);

			if ( !bAdvanced ) {
				SetDlgItemTextA( hwndDlg, IDC_DEFAULT, STR_BASIC);
				if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "Default", &dbv)) {
					SetDlgItemText( hwndDlg, IDC_WILDCARD, dbv.ptszVal);
					DBFreeVariant(&dbv);
				}
			}
			else {
				SetDlgItemTextA( hwndDlg, IDC_DEFAULT, STR_ADVANCED);
				if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "UWildcard", &dbv)) {
					SetDlgItemText( hwndDlg, IDC_WILDCARD, dbv.ptszVal);
					DBFreeVariant(&dbv);
			}	}

			if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "UUser", &dbv)) {
				SetDlgItemText( hwndDlg, IDC_USER, dbv.ptszVal);
				DBFreeVariant(&dbv);
			}

			if (!DBGetContactSettingTString( hContact, IRCPROTONAME, "UHost", &dbv)) {
				SetDlgItemText( hwndDlg, IDC_HOST, dbv.ptszVal);
				DBFreeVariant(&dbv);
			}
			mir_forkthread(AckUserInfoSearch, hContact);
		}
		break;

	case WM_COMMAND:
		if (( LOWORD(wParam) == IDC_WILDCARD || LOWORD(wParam) == IDC_USER || LOWORD(wParam) == IDC_HOST ) &&
			 ( HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()))
			return true;		

		EnableWindow(GetDlgItem( hwndDlg, IDC_BUTTON), true);
		EnableWindow(GetDlgItem( hwndDlg, IDC_BUTTON2), true);

		if( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_BUTTON ) {
			TCHAR temp[500];
			GetDlgItemText( hwndDlg, IDC_WILDCARD, temp, SIZEOF(temp));
			DBVARIANT dbv;

			BYTE bAdvanced = IsDlgButtonChecked( hwndDlg, IDC_RADIO1)?0:1;
			if ( bAdvanced ) {
				if ( GetWindowTextLength(GetDlgItem( hwndDlg, IDC_WILDCARD)) == 0 ||
					  GetWindowTextLength(GetDlgItem( hwndDlg, IDC_USER)) == 0 ||
					  GetWindowTextLength(GetDlgItem( hwndDlg, IDC_HOST)) == 0)
				{
					MessageBox( NULL, TranslateT(STR_ERROR2), TranslateT("IRC error"), MB_OK|MB_ICONERROR);
					return FALSE;
				}

				if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "Default", &dbv )) {
					TString S = _T(STR_ERROR) + (TString)_T(" (") + dbv.ptszVal + (TString)_T(")");
					if (( lstrlen(temp) < 4 && lstrlen(temp)) || !WCCmp(CharLower(temp), CharLower(dbv.ptszVal))) {
						MessageBox( NULL, TranslateTS( S.c_str()), TranslateT( "IRC error" ), MB_OK | MB_ICONERROR );
						DBFreeVariant( &dbv );
						return FALSE;
					}
					DBFreeVariant( &dbv );
				}
				
				GetDlgItemText( hwndDlg, IDC_WILDCARD, temp, SIZEOF(temp));
				if ( lstrlen( GetWord(temp, 0).c_str()))
					DBWriteContactSettingTString( hContact, IRCPROTONAME, "UWildcard", GetWord(temp, 0).c_str());
				else
					DBDeleteContactSetting( hContact, IRCPROTONAME, "UWildcard");
			}

			DBWriteContactSettingByte( hContact, IRCPROTONAME, "AdvancedMode", bAdvanced);

			GetDlgItemText( hwndDlg, IDC_USER, temp, SIZEOF(temp));
			if (lstrlen(GetWord(temp, 0).c_str()))
				DBWriteContactSettingTString( hContact, IRCPROTONAME, "UUser", GetWord(temp, 0).c_str());
			else
				DBDeleteContactSetting( hContact, IRCPROTONAME, "UUser");

			GetDlgItemText( hwndDlg, IDC_HOST, temp, SIZEOF(temp));
			if (lstrlen(GetWord(temp, 0).c_str()))
				DBWriteContactSettingTString( hContact, IRCPROTONAME, "UHost", GetWord(temp, 0).c_str());
			else
				DBDeleteContactSetting( hContact, IRCPROTONAME, "UHost");

			EnableWindow(GetDlgItem( hwndDlg, IDC_BUTTON), FALSE);
		}

		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_BUTTON2 ) {
			if ( IsDlgButtonChecked( hwndDlg, IDC_RADIO2 ))
				SetDlgItemTextA( hwndDlg, IDC_WILDCARD, "");
			SetDlgItemTextA( hwndDlg, IDC_HOST, "" );
			SetDlgItemTextA( hwndDlg, IDC_USER, "" );
			DBDeleteContactSetting( hContact, IRCPROTONAME, "UWildcard");
			DBDeleteContactSetting( hContact, IRCPROTONAME, "UUser");
			DBDeleteContactSetting( hContact, IRCPROTONAME, "UHost");
			EnableWindow(GetDlgItem( hwndDlg, IDC_BUTTON), FALSE );
			EnableWindow(GetDlgItem( hwndDlg, IDC_BUTTON2), FALSE );
		}		

		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_RADIO1 ) {
			SetDlgItemTextA( hwndDlg, IDC_DEFAULT, STR_BASIC );

			DBVARIANT dbv;
			if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "Default", &dbv )) {
				SetDlgItemText( hwndDlg, IDC_WILDCARD, dbv.ptszVal );
				DBFreeVariant( &dbv );
			}
			EnableWindow(GetDlgItem( hwndDlg, IDC_WILDCARD), FALSE );
		}	
		
		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_RADIO2 ) {
			DBVARIANT dbv;
			SetDlgItemTextA( hwndDlg, IDC_DEFAULT, STR_ADVANCED );
			if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "UWildcard", &dbv )) {
				SetDlgItemText( hwndDlg, IDC_WILDCARD, dbv.ptszVal );
				DBFreeVariant( &dbv );
			}
			EnableWindow(GetDlgItem( hwndDlg, IDC_WILDCARD), true);
		}	
		break;
	}
	return FALSE;
}

BOOL CALLBACK QuestionWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg);
		{
			HFONT hFont;
			LOGFONT lf;
			hFont=(HFONT)SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			GetObject(hFont,sizeof(lf),&lf);
			lf.lfHeight=(int)(lf.lfHeight*1.2);
			lf.lfWeight=FW_BOLD;
			hFont=CreateFontIndirect(&lf);
			SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem( hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadIconEx(IDI_LOGO), 0);
		}
		break;
		
	case WM_COMMAND:
		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNCANCEL )
			PostMessage ( hwndDlg, WM_CLOSE, 0,0);

		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNOK ) {
			int i = GetWindowTextLength( GetDlgItem( hwndDlg, IDC_EDIT ));
			if ( i > 0 ) {
				TCHAR* l = new TCHAR[ i+2 ];
				GetDlgItemText( hwndDlg, IDC_EDIT, l, i+1 );

				int j = GetWindowTextLength(GetDlgItem( hwndDlg, IDC_HIDDENEDIT));
				TCHAR* m = new TCHAR[ j+2 ];
				GetDlgItemText( hwndDlg, IDC_HIDDENEDIT, m, j+1 );

				TCHAR* text = _tcsstr( m, _T("%question"));
				TCHAR* p1 = text;
				TCHAR* p2 = NULL;
				if ( p1 ) {
					p1 += 9;
					if ( *p1 == '=' && p1[1] == '\"' ) {
						p1 += 2;
						for ( int k =0; k < 3; k++ ) {
							p2 = _tcschr( p1, '\"' );
							if ( p2 ) {
								p2++;
								if ( k == 2 || (*p2 != ',' || (*p2 == ',' && p2[1] != '\"')))
									*p2 = '\0';	
								else
									p2 += 2;
								p1 = p2;
						}	}
					}
					else *p1 = '\0';
				}
				
				TCHAR* n = new TCHAR[ j+2 ];
				GetDlgItemText( hwndDlg, IDC_HIDDENEDIT, n, j+1 );
			
				TString S = ReplaceString(n, text, l);

				PostIrcMessageWnd( NULL, NULL, (TCHAR*)S.c_str());

				delete []m;
				delete []l;
				delete []n;
				HWND hwnd = GetParent( hwndDlg);
				if( hwnd )
					SendMessage(hwnd, IRC_QUESTIONAPPLY, 0, 0);
			}
			SendMessage ( hwndDlg, WM_CLOSE, 0,0);
		}
		break;

	case IRC_ACTIVATE:
		ShowWindow( hwndDlg, SW_SHOW);
		SetActiveWindow( hwndDlg);
		break;

	case WM_CLOSE:
		{
			HWND hwnd = GetParent( hwndDlg);
			if ( hwnd )
				SendMessage(GetParent( hwndDlg), IRC_QUESTIONCLOSE, 0, 0);
			DestroyWindow( hwndDlg );

		}
		break;

	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem( hwndDlg,IDC_WHITERECT) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_TEXT) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_CAPTION) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_LOGO))
		{
			SetTextColor((HDC)wParam,RGB(0,0,0));
			SetBkColor((HDC)wParam,RGB(255,255,255));
			return (BOOL)GetStockObject(WHITE_BRUSH);
		}
		break;

	case WM_DESTROY:
		{			
			HFONT hFont;
			hFont=(HFONT)SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( hwndDlg,IDNOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				
		}
		break;	
	}

	return FALSE;
}

BOOL CALLBACK ManagerWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg);
		{
			HFONT hFont;
			LOGFONT lf;
			hFont=(HFONT)SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			GetObject(hFont,sizeof(lf),&lf);
			lf.lfHeight=(int)(lf.lfHeight*1.2);
			lf.lfWeight=FW_BOLD;
			hFont=CreateFontIndirect(&lf);
			SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);

			POINT pt;
			pt.x = 3; 
			pt.y = 3; 
			HWND hwndEdit = ChildWindowFromPoint(GetDlgItem( hwndDlg, IDC_TOPIC), pt); 
			
			OldMgrEditProc = (WNDPROC)SetWindowLong(hwndEdit, GWL_WNDPROC,(LONG)MgrEditSubclassProc); 
			
			SendDlgItemMessage( hwndDlg,IDC_ADD,BM_SETIMAGE,IMAGE_ICON,(LPARAM)(HICON)LoadIconEx(IDI_ADD));
			SendDlgItemMessage( hwndDlg,IDC_REMOVE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_DELETE));
			SendDlgItemMessage( hwndDlg,IDC_EDIT,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_RENAME));
			SendMessage(GetDlgItem( hwndDlg,IDC_ADD), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Add ban/invite/exception"), 0);
			SendMessage(GetDlgItem( hwndDlg,IDC_EDIT), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Edit selected ban/invite/exception"), 0);
			SendMessage(GetDlgItem( hwndDlg,IDC_REMOVE), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Delete selected ban/invite/exception"), 0);

			SendMessage( hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)LoadIconEx(IDI_MANAGER));
			SendMessage(GetDlgItem( hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadIconEx(IDI_LOGO), 0);
			SendDlgItemMessage( hwndDlg,IDC_APPLYTOPIC,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx( IDI_GO ));
			SendDlgItemMessage( hwndDlg,IDC_APPLYMODES,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx( IDI_GO ));
			SendMessage(GetDlgItem( hwndDlg,IDC_APPLYTOPIC), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Set this topic for the channel"), 0);
			SendMessage(GetDlgItem( hwndDlg,IDC_APPLYMODES), BUTTONADDTOOLTIP, (WPARAM)LPGEN("Set these modes for the channel"), 0);

			SendDlgItemMessage( hwndDlg,IDC_LIST,LB_SETHORIZONTALEXTENT,750,NULL);
			CheckDlgButton( hwndDlg, IDC_RADIO1, BST_CHECKED);
			if ( strchr(sChannelModes.c_str(), 't'))
				EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK1), false);
			if ( strchr(sChannelModes.c_str(), 'n'))
				EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK2), false);
			if ( strchr(sChannelModes.c_str(), 'i'))
				EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK3), false);
			if ( strchr(sChannelModes.c_str(), 'm'))
				EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK4), false);
			if ( strchr(sChannelModes.c_str(), 'k'))
				EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK5), false);
			if ( strchr(sChannelModes.c_str(), 'l'))
				EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK6), false);
			if ( strchr(sChannelModes.c_str(), 'p'))
				EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK7), false);
			if ( strchr(sChannelModes.c_str(), 's'))
				EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK8), false);
		}
		break;

	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem( hwndDlg,IDC_WHITERECT) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_TEXT) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_CAPTION) ||
			 (HWND)lParam == GetDlgItem( hwndDlg,IDC_LOGO))
		{
			SetTextColor((HDC)wParam,RGB(0,0,0));
			SetBkColor((HDC)wParam,RGB(255,255,255));
			return (BOOL)GetStockObject(WHITE_BRUSH);
		}
		break;

	case IRC_INITMANAGER:
		{
			TCHAR* window = (TCHAR*) lParam;

			CHANNELINFO* wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, window, NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
			if ( wi ) {
				if ( g_ircSession ) {
					TCHAR temp[1000];
					mir_sntprintf(temp, SIZEOF(temp), _T("Topic%s%s"), window, g_ircSession.GetInfo().sNetwork.c_str());

					#if defined( _UNICODE )
						char* p = mir_t2a(temp);
					#else
						char* p = temp;
					#endif
					DBVARIANT dbv;
  					if ( !DBGetContactSettingTString(NULL, IRCPROTONAME, p, &dbv )) {
						for ( int i = 0; i<5; i++ ) {
							if ( !GetWord(dbv.ptszVal, i).empty()) {
								TString S = ReplaceString(GetWord(dbv.ptszVal, i), _T("%¤"), _T(" "));
								SendDlgItemMessage( hwndDlg, IDC_TOPIC, CB_ADDSTRING, 0, (LPARAM)S.c_str());
						}	}
						DBFreeVariant(&dbv);
					}
					#if defined( _UNICODE )
						mir_free(p);
					#endif
				}

				if ( wi->pszTopic )
					SetDlgItemText( hwndDlg, IDC_TOPIC, wi->pszTopic);

				if ( !IsDlgButtonChecked( manager_hWnd, IDC_NOTOP ))
					EnableWindow(GetDlgItem( hwndDlg, IDC_ADD), true);


				bool add = false;
				TCHAR* p1= wi->pszMode;
				if ( p1 ) {
					while ( *p1 != '\0' && *p1 != ' ' ) {
						if (*p1 == '+')
							add = true;
						if (*p1 == '-')
							add = false;
						if (*p1 == 't')
							CheckDlgButton( hwndDlg, IDC_CHECK1, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'n')
							CheckDlgButton( hwndDlg, IDC_CHECK2, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'i')
							CheckDlgButton( hwndDlg, IDC_CHECK3, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'm')
							CheckDlgButton( hwndDlg, IDC_CHECK4, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'p')
							CheckDlgButton( hwndDlg, IDC_CHECK7, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 's')
							CheckDlgButton( hwndDlg, IDC_CHECK8, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'k' && add) {
							CheckDlgButton( hwndDlg, IDC_CHECK5, add?(BST_CHECKED) : (BST_UNCHECKED));
							EnableWindow(GetDlgItem( hwndDlg, IDC_KEY), add?(true) : (false));
							if(wi->pszPassword)
								SetDlgItemText( hwndDlg, IDC_KEY, wi->pszPassword);
						}
						if (*p1 == 'l' && add) {
							CheckDlgButton( hwndDlg, IDC_CHECK6, add?(BST_CHECKED) : (BST_UNCHECKED));
							EnableWindow(GetDlgItem( hwndDlg, IDC_LIMIT), add?(true) : (false));
							if(wi->pszLimit)
								SetDlgItemText( hwndDlg, IDC_LIMIT, wi->pszLimit);
						}
						p1++;
						if (wParam == 0 ) {
							EnableWindow(GetDlgItem( hwndDlg, IDC_LIMIT), (false));
							EnableWindow(GetDlgItem( hwndDlg, IDC_KEY), (false));
							EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK1),  (false));
							EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK2),  (false));
							EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK3), (false));
							EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK4), (false));
							EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK5),  (false));
							EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK6),  (false));
							EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK7),  (false));
							EnableWindow(GetDlgItem( hwndDlg, IDC_CHECK8), (false));
							EnableWindow(GetDlgItem( hwndDlg, IDC_ADD), (false));
							if(IsDlgButtonChecked( hwndDlg, IDC_CHECK1))
								EnableWindow(GetDlgItem( hwndDlg, IDC_TOPIC), (false));
							CheckDlgButton( hwndDlg, IDC_NOTOP, BST_CHECKED);
						}
						ShowWindow( hwndDlg, SW_SHOW);
			}	}	}

			if ( strchr( sChannelModes.c_str(), 'b' )) {
				CheckDlgButton( hwndDlg, IDC_RADIO1, BST_CHECKED);
				PostIrcMessage( _T("/MODE %s +b"), window);
		}	}
		break;		

	case WM_COMMAND:
		if ((	LOWORD(wParam) == IDC_LIMIT || LOWORD(wParam) == IDC_KEY ) &&
			 ( HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()))
			return true;		
		
		TCHAR window[256];
		GetDlgItemText( hwndDlg, IDC_CAPTION, window, SIZEOF(window));

		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDCLOSE )
			PostMessage ( hwndDlg, WM_CLOSE, 0,0);

		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_REMOVE ) {
			int i = SendDlgItemMessage( hwndDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
			if ( i != LB_ERR ) {
				EnableWindow(GetDlgItem( hwndDlg, IDC_REMOVE), false);
				EnableWindow(GetDlgItem( hwndDlg, IDC_EDIT), false);
				EnableWindow(GetDlgItem( hwndDlg, IDC_ADD), false);
				TCHAR* m = new TCHAR[ SendDlgItemMessage( hwndDlg, IDC_LIST, LB_GETTEXTLEN, i, 0)+2 ];
				SendDlgItemMessage( hwndDlg, IDC_LIST, LB_GETTEXT, i, (LPARAM)m);
				TString user = GetWord(m, 0);
				delete[]m;
				TCHAR temp[100];
				TCHAR mode[3];
				if ( IsDlgButtonChecked( hwndDlg, IDC_RADIO1 )) {
					lstrcpy(mode, _T("-b"));
					lstrcpyn(temp, TranslateT( "Remove ban?" ), 100 );
				}
				if ( IsDlgButtonChecked( hwndDlg, IDC_RADIO2 )) {
					lstrcpy(mode, _T("-I"));
					lstrcpyn(temp, TranslateT( "Remove invite?" ), 100 );
				}
				if ( IsDlgButtonChecked( hwndDlg, IDC_RADIO3 )) {
					lstrcpy(mode, _T("-e"));
					lstrcpyn(temp, TranslateT( "Remove exception?" ), 100 );
				}
				
				if ( MessageBox( hwndDlg, user.c_str(), temp, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2 ) == IDYES ) {
					PostIrcMessage( _T("/MODE %s %s %s"), window, mode, user.c_str());
					SendMessage( hwndDlg, IRC_QUESTIONAPPLY, 0, 0);
				}
				SendMessage( hwndDlg, IRC_QUESTIONCLOSE, 0, 0);
		}	}

		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_EDIT ) {
			if ( IsDlgButtonChecked( hwndDlg, IDC_NOTOP ))
				break;	

			int i = SendDlgItemMessage( hwndDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
			if ( i != LB_ERR ) {
				TCHAR* m = new TCHAR[ SendDlgItemMessage( hwndDlg, IDC_LIST, LB_GETTEXTLEN, i, 0)+2 ];
				SendDlgItemMessage( hwndDlg, IDC_LIST, LB_GETTEXT, i, (LPARAM)m );
				TString user = GetWord(m, 0);
				delete[]m;
				
				TCHAR temp[100];
				TCHAR mode[3];
				if ( IsDlgButtonChecked( hwndDlg, IDC_RADIO1 )) {
					lstrcpy( mode, _T("b"));
					lstrcpyn( temp, TranslateT("Edit ban"), 100 );
				}
				if ( IsDlgButtonChecked( hwndDlg, IDC_RADIO2 )) {
					lstrcpy( mode, _T("I"));
					lstrcpyn( temp, TranslateT("Edit invite?"), 100 );
				}
				if (IsDlgButtonChecked( hwndDlg, IDC_RADIO3 )) {
					lstrcpy( mode, _T("e"));
					lstrcpyn( temp, TranslateT("Edit exception?"), 100 );
				}
					
				HWND addban_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_QUESTION),hwndDlg, QuestionWndProc);
				EnableWindow(GetDlgItem( hwndDlg, IDC_ADD), false);
				EnableWindow(GetDlgItem( hwndDlg, IDC_REMOVE), false);
				EnableWindow(GetDlgItem( hwndDlg, IDC_EDIT), false);
				SetDlgItemText(addban_hWnd, IDC_CAPTION, temp);
				SetWindowText(GetDlgItem(addban_hWnd, IDC_TEXT), TranslateT("Please enter the hostmask (nick!user@host)"));
				SetWindowText(GetDlgItem(addban_hWnd, IDC_EDIT), user.c_str());

				TCHAR temp2[450];
				mir_sntprintf(temp2, 450, _T("/MODE %s -%s %s%s/MODE %s +%s %s"), window, mode, user.c_str(), _T("%newl"), window, mode, _T("%question"));
				SetDlgItemText(addban_hWnd, IDC_HIDDENEDIT, temp2);
				PostMessage(addban_hWnd, IRC_ACTIVATE, 0, 0);
		}	}			

		if ( HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == IDC_LIST ) {
			if ( !IsDlgButtonChecked( hwndDlg, IDC_NOTOP )) {
				EnableWindow(GetDlgItem( hwndDlg, IDC_EDIT), true);
				EnableWindow(GetDlgItem( hwndDlg, IDC_REMOVE), true);
		}	}
		
		if ( HIWORD(wParam) == LBN_DBLCLK && LOWORD(wParam) == IDC_LIST )
			SendMessage( hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_EDIT, BN_CLICKED), 0);

		if ( HIWORD(wParam) == EN_CHANGE && (LOWORD(wParam) == IDC_LIMIT || LOWORD(wParam) == IDC_KEY ))
			EnableWindow(GetDlgItem( hwndDlg, IDC_APPLYMODES), true);

		if (( HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE) && LOWORD(wParam) == IDC_TOPIC ) {
			EnableWindow(GetDlgItem( hwndDlg, IDC_APPLYTOPIC), true);
			return false;
		}

		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_APPLYTOPIC ) {
			TCHAR temp[470];
			GetDlgItemText( hwndDlg, IDC_TOPIC, temp, SIZEOF(temp));
			PostIrcMessage( _T("/TOPIC %s %s"), window, temp);
			int i = SendDlgItemMessage( hwndDlg, IDC_TOPIC, CB_FINDSTRINGEXACT, -1, (LPARAM)temp);
			if ( i != LB_ERR )
				SendDlgItemMessage( hwndDlg, IDC_TOPIC, CB_DELETESTRING, i, 0);
			SendDlgItemMessage( hwndDlg, IDC_TOPIC, CB_INSERTSTRING, 0, (LPARAM)temp);
			SetDlgItemText( hwndDlg, IDC_TOPIC, temp);
			EnableWindow(GetDlgItem( hwndDlg, IDC_APPLYTOPIC), false);
		}

		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_CHECK5 ) {
			EnableWindow(GetDlgItem( hwndDlg, IDC_KEY), IsDlgButtonChecked( hwndDlg, IDC_CHECK5) == BST_CHECKED);				
			EnableWindow(GetDlgItem( hwndDlg, IDC_APPLYMODES), true);
		}

		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_ADD ) {
			TCHAR temp[100];
			TCHAR mode[3];
			if ( IsDlgButtonChecked( hwndDlg, IDC_RADIO1 )) {
				lstrcpy( mode, _T("+b"));
				lstrcpyn( temp, TranslateT("Add ban"), 100 );
			}
			if ( IsDlgButtonChecked( hwndDlg, IDC_RADIO2 )) {
				lstrcpy( mode, _T("+I"));
				lstrcpyn( temp, TranslateT("Add invite"), 100 );
			}
			if ( IsDlgButtonChecked( hwndDlg, IDC_RADIO3 )) {
				lstrcpy( mode, _T("+e"));
				lstrcpyn( temp, TranslateT("Add exception"), 100);
			}
			HWND addban_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_QUESTION),hwndDlg, QuestionWndProc);
			EnableWindow(GetDlgItem( hwndDlg, IDC_ADD), false);
			EnableWindow(GetDlgItem( hwndDlg, IDC_REMOVE), false);
			EnableWindow(GetDlgItem( hwndDlg, IDC_EDIT), false);
			SetDlgItemText(addban_hWnd, IDC_CAPTION, temp);
			SetWindowText(GetDlgItem(addban_hWnd, IDC_TEXT), TranslateT("Please enter the hostmask (nick!user@host)"));
			
			TCHAR temp2[450];
			mir_sntprintf(temp2, 450, _T("/MODE %s %s %s"), window, mode, _T("%question"));
			SetDlgItemText(addban_hWnd, IDC_HIDDENEDIT, temp2);
			PostMessage(addban_hWnd, IRC_ACTIVATE, 0, 0);
		}
		
		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_CHECK6 ) {
			EnableWindow(GetDlgItem( hwndDlg, IDC_LIMIT), IsDlgButtonChecked( hwndDlg, IDC_CHECK6) == BST_CHECKED );				
			EnableWindow(GetDlgItem( hwndDlg, IDC_APPLYMODES), true);
		}
		
		if ( HIWORD(wParam) == BN_CLICKED && (LOWORD(wParam) == IDC_RADIO1 || LOWORD(wParam) == IDC_RADIO3 || LOWORD(wParam) == IDC_RADIO2))
			SendMessage( hwndDlg, IRC_QUESTIONAPPLY, 0, 0);

		if ( HIWORD(wParam) == BN_CLICKED && (LOWORD(wParam) == IDC_CHECK1
				|| LOWORD(wParam) == IDC_CHECK2 || LOWORD(wParam) == IDC_CHECK3 || LOWORD(wParam) == IDC_CHECK4
				|| LOWORD(wParam) == IDC_CHECK7 || LOWORD(wParam) == IDC_CHECK8))
			EnableWindow(GetDlgItem( hwndDlg, IDC_APPLYMODES), true);				

		if ( HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_APPLYMODES ) {
			CHANNELINFO* wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, window, NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
			if ( wi ) {
				TCHAR toadd[10]; *toadd = '\0';
				TCHAR toremove[10]; *toremove = '\0';
				TString appendixadd = _T("");
				TString appendixremove = _T("");
				if ( wi->pszMode && _tcschr( wi->pszMode, 't' )) {
					if ( !IsDlgButtonChecked( hwndDlg, IDC_CHECK1 ))
						lstrcat( toremove, _T("t"));
				}
				else if ( IsDlgButtonChecked( hwndDlg, IDC_CHECK1 ))
					lstrcat( toadd, _T("t"));

				if ( wi->pszMode && _tcschr( wi->pszMode, 'n' )) {
					if( ! IsDlgButtonChecked( hwndDlg, IDC_CHECK2 ))
						lstrcat( toremove, _T("n"));
				}
				else if ( IsDlgButtonChecked( hwndDlg, IDC_CHECK2 ))
					lstrcat( toadd, _T("n"));

				if ( wi->pszMode && _tcschr( wi->pszMode, 'i' )) {
					if ( !IsDlgButtonChecked( hwndDlg, IDC_CHECK3 ))
						lstrcat( toremove, _T("i"));
				}
				else if ( IsDlgButtonChecked( hwndDlg, IDC_CHECK3 ))
					lstrcat( toadd, _T("i"));

				if ( wi->pszMode && _tcschr( wi->pszMode, 'm' )) {
					if( !IsDlgButtonChecked( hwndDlg, IDC_CHECK4 ))
						lstrcat( toremove, _T("m"));
				}
				else if ( IsDlgButtonChecked( hwndDlg, IDC_CHECK4 ))
					lstrcat( toadd, _T("m"));

				if ( wi->pszMode && _tcschr( wi->pszMode, 'p' )) {
					if ( !IsDlgButtonChecked( hwndDlg, IDC_CHECK7 ))
						lstrcat( toremove, _T("p"));
				}
				else if ( IsDlgButtonChecked( hwndDlg, IDC_CHECK7 ))
					lstrcat( toadd, _T("p"));

				if ( wi->pszMode && _tcschr( wi->pszMode, 's' )) {
					if ( !IsDlgButtonChecked( hwndDlg, IDC_CHECK8 ))
						lstrcat( toremove, _T("s"));
				}
				else if ( IsDlgButtonChecked( hwndDlg, IDC_CHECK8 ))
					lstrcat( toadd, _T("s"));

				TString Key = _T("");
				TString Limit = _T("");
				if ( wi->pszMode && wi->pszPassword && _tcschr( wi->pszMode, 'k' )) {
					if ( !IsDlgButtonChecked( hwndDlg, IDC_CHECK5 )) {
						lstrcat( toremove, _T("k"));
						appendixremove += _T(" ");
						appendixremove += wi->pszPassword;
					}
					else if( GetWindowTextLength( GetDlgItem( hwndDlg, IDC_KEY ))) {
						TCHAR temp[400];
						GetDlgItemText( hwndDlg, IDC_KEY, temp, 14);

						if ( Key != temp ) {
							lstrcat( toremove, _T("k"));
							lstrcat( toadd, _T("k"));
							appendixadd += _T(" ");
							appendixadd += temp;
							appendixremove += _T(" ");
							appendixremove += wi->pszPassword;
					}	}
				}
				else if ( IsDlgButtonChecked( hwndDlg, IDC_CHECK5) && GetWindowTextLength( GetDlgItem( hwndDlg, IDC_KEY ))) {
					lstrcat( toadd, _T("k"));
					appendixadd += _T(" ");
					
					TCHAR temp[400];
					GetDlgItemText( hwndDlg, IDC_KEY, temp, SIZEOF(temp));
					appendixadd += temp;
				}

				if ( _tcschr( wi->pszMode, 'l' )) {
					if ( !IsDlgButtonChecked( hwndDlg, IDC_CHECK6 ))
						lstrcat( toremove, _T("l"));
					else if ( GetWindowTextLength( GetDlgItem( hwndDlg, IDC_LIMIT ))) {
						TCHAR temp[15];
						GetDlgItemText( hwndDlg, IDC_LIMIT, temp, SIZEOF(temp));
						if ( wi->pszLimit && lstrcmpi( wi->pszLimit, temp )) {
							lstrcat( toadd, _T("l"));
							appendixadd += _T(" ");
							appendixadd += temp;
					}	}
				}
				else if ( IsDlgButtonChecked( hwndDlg, IDC_CHECK6 ) && GetWindowTextLength( GetDlgItem( hwndDlg, IDC_LIMIT ))) {
					lstrcat( toadd, _T("l"));
					appendixadd += _T(" ");
					
					TCHAR temp[15];
					GetDlgItemText( hwndDlg, IDC_LIMIT, temp, SIZEOF(temp));
					appendixadd += temp;
				}

				if ( lstrlen(toadd) || lstrlen( toremove )) {
					TCHAR temp[500];
					lstrcpy(temp, _T("/mode "));
					lstrcat(temp, window);
					lstrcat(temp, _T(" "));
					if ( lstrlen( toremove ))
						mir_sntprintf( temp, 499, _T("%s-%s"), temp, toremove );
					if ( lstrlen( toadd ))
						mir_sntprintf( temp, 499, _T("%s+%s"), temp, toadd );
					if (!appendixremove.empty())
						lstrcat(temp, appendixremove.c_str());
					if (!appendixadd.empty())
						lstrcat(temp, appendixadd.c_str());
					PostIrcMessage( temp);
			}	}
			EnableWindow(GetDlgItem( hwndDlg, IDC_APPLYMODES), false);				
		}
		break;
	
	case IRC_QUESTIONCLOSE:
		EnableWindow(GetDlgItem( hwndDlg, IDC_ADD), true);
		if ( SendDlgItemMessage( hwndDlg, IDC_LIST, LB_GETCURSEL, 0, 0) != LB_ERR) {
			EnableWindow(GetDlgItem( hwndDlg, IDC_REMOVE), true);
			EnableWindow(GetDlgItem( hwndDlg, IDC_EDIT), true);
		}
		break;

	case IRC_QUESTIONAPPLY:
		{
			TCHAR window[256];
			GetDlgItemText( hwndDlg, IDC_CAPTION, window, 255);

			TCHAR mode[3];
			lstrcpy( mode, _T("+b"));
			if ( IsDlgButtonChecked( hwndDlg, IDC_RADIO2 ))
				lstrcpy( mode, _T("+I"));
			if ( IsDlgButtonChecked( hwndDlg, IDC_RADIO3 ))
				lstrcpy( mode, _T("+e"));
			SendDlgItemMessage( hwndDlg, IDC_LIST, LB_RESETCONTENT, 0, 0);
			EnableWindow(GetDlgItem( hwndDlg, IDC_RADIO1), false);
			EnableWindow(GetDlgItem( hwndDlg, IDC_RADIO2), false);
			EnableWindow(GetDlgItem( hwndDlg, IDC_RADIO3), false);
			EnableWindow(GetDlgItem( hwndDlg, IDC_ADD), false);
			EnableWindow(GetDlgItem( hwndDlg, IDC_EDIT), false);
			EnableWindow(GetDlgItem( hwndDlg, IDC_REMOVE), false);
			PostIrcMessage( _T("%s %s %s"), _T("/MODE"), window, mode); //wrong overloaded operator if three args
		}
		break;		

	case WM_CLOSE:
		{
			if ( lParam != 3 ) {
				if ( IsWindowEnabled(GetDlgItem( hwndDlg, IDC_APPLYMODES)) || IsWindowEnabled(GetDlgItem( hwndDlg, IDC_APPLYTOPIC))) {
					int i = MessageBox( hwndDlg, TranslateT("You have not applied all changes!\n\nApply before exiting?"), TranslateT("IRC warning"), MB_YESNOCANCEL|MB_ICONWARNING|MB_DEFBUTTON3);
					if ( i == IDCANCEL )
						return false;

					if ( i == IDYES ) {
						if ( IsWindowEnabled( GetDlgItem( hwndDlg, IDC_APPLYMODES )))
							SendMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_APPLYMODES, BN_CLICKED), 0);
						if ( IsWindowEnabled( GetDlgItem( hwndDlg, IDC_APPLYTOPIC )))
							SendMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_APPLYTOPIC, BN_CLICKED), 0);
			}	}	}

			TCHAR window[256];
			GetDlgItemText( hwndDlg, IDC_CAPTION, window, 255 );
			TString S = _T(""); 
			TCHAR temp[1000];
			for ( int i = 0; i < 5; i++ ) {
				if ( SendDlgItemMessage( hwndDlg, IDC_TOPIC, CB_GETLBTEXT, i, (LPARAM)temp) != LB_ERR) {
					S += _T(" ");
					S += ReplaceString( temp, _T(" "), _T("%¤"));
			}	}

			if ( !S.empty() && g_ircSession ) {
				mir_sntprintf( temp, SIZEOF(temp), _T("Topic%s%s"), window, g_ircSession.GetInfo().sNetwork.c_str());
				#if defined( _UNICODE )
					char* p = mir_t2a(temp);
					DBWriteContactSettingTString(NULL, IRCPROTONAME, p, S.c_str());
					mir_free(p);
				#else
					DBWriteContactSettingString(NULL, IRCPROTONAME, temp, S.c_str());
				#endif
			}
			DestroyWindow( hwndDlg);
		}
		break;

	case WM_DESTROY:
		{			
			HFONT hFont;
			hFont=(HFONT)SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			SendDlgItemMessage( hwndDlg,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( hwndDlg,IDNOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);
			manager_hWnd = NULL;
		}
		break;	
	}
	return FALSE;
}

LRESULT CALLBACK MgrEditSubclassProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) 
{ 
	switch( msg ) { 
	case WM_CHAR :
		if ( wParam == 21 || wParam == 11 || wParam == 2 ) {
			char w[2];
			if ( wParam == 11 ) {
				w[0] = 3;
				w[1] = '\0';
			}
			if ( wParam == 2 ) {
				w[0] = 2;
				w[1] = '\0';
			}
			if ( wParam == 21 ) {
				w[0] = 31;
				w[1] = '\0';
			}
			SendMessage( hwndDlg, EM_REPLACESEL, false, (LPARAM) w);
			SendMessage( hwndDlg, EM_SCROLLCARET, 0, 0 );
			return 0;
		}
		break;
	} 

	return CallWindowProc(OldMgrEditProc, hwndDlg, msg, wParam, lParam); 
} 
