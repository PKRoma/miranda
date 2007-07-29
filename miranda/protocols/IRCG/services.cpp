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

CIrcSessionInfo si;

HANDLE g_hModulesLoaded = NULL;
HANDLE g_hSystemPreShutdown = NULL;
HANDLE g_hEventDblClick = NULL;
HANDLE g_hContactDeleted = NULL;
HANDLE g_hUserInfoInit = NULL;
HANDLE g_hMenuCreation = NULL;
HANDLE g_hGCUserEvent = NULL;
HANDLE g_hGCMenuBuild = NULL;
HANDLE g_hOptionsInit = NULL;
HANDLE hContactMenu1 = NULL;
HANDLE hContactMenu2 = NULL;
HANDLE hContactMenu3 = NULL;
HANDLE hMenuQuick = NULL;
HANDLE hMenuJoin = NULL;
HANDLE hMenuNick = NULL;
HANDLE hMenuList = NULL;
HANDLE hMenuServer = NULL;
HANDLE hNetlib = NULL;
HANDLE hNetlibDCC = NULL;

HWND   join_hWnd = NULL;
HWND   quickconn_hWnd = NULL;

volatile bool bChatInstalled = FALSE;

bool      bMbotInstalled = FALSE;
int       RetryCount = 0;
int       PortCount = 0;
DWORD     bConnectRequested = 0;
DWORD     bConnectThreadRunning = 0;
int       iTempCheckTime = 0;
int       OldStatus = ID_STATUS_OFFLINE;
int       GlobalStatus = ID_STATUS_OFFLINE;
UINT_PTR  RetryTimer = 0;
TString   StatusMessage = _T("");

extern HWND nick_hWnd;
extern HWND list_hWnd ;
extern bool bTempDisableCheck ;
extern bool bTempForceCheck ;
extern bool bPerformDone;

extern bool				nickflag;
GETEVENTFUNC			pfnAddEvent = 0;

VOID CALLBACK RetryTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime);

static void InitMenus(void)
{
	char temp[ MAXMODULELABELLENGTH ];
	char *d = temp + sprintf( temp, IRCPROTONAME );

	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof( mi );
	mi.pszService = temp;
	mi.flags = CMIF_ICONFROMICOLIB;

	if ( bChatInstalled ) {
		mi.pszName = LPGEN("&Quick connect");
		mi.icolibItem = GetIconHandle(IDI_QUICK);
		strcpy( d, IRC_QUICKCONNECT );
		mi.popupPosition = 500090000;
		mi.pszPopupName = ALTIRCPROTONAME;
		hMenuQuick= (void *)CallService(MS_CLIST_ADDMAINMENUITEM, (WPARAM)0, (LPARAM)&mi);

		mi.pszName = LPGEN("&Join a channel");
		mi.icolibItem = GetIconHandle(IDI_JOIN);
		strcpy( d, IRC_JOINCHANNEL );
		mi.popupPosition = 500090001;
		mi.pszPopupName = ALTIRCPROTONAME;
		hMenuJoin = (void *)CallService(MS_CLIST_ADDMAINMENUITEM, (WPARAM)0, (LPARAM)&mi);

		mi.pszName = LPGEN("&Change your nickname");
		mi.icolibItem = GetIconHandle(IDI_WHOIS);
		strcpy( d, IRC_CHANGENICK );
		mi.popupPosition = 500090002;
		mi.pszPopupName = ALTIRCPROTONAME;
		hMenuNick = (void *)CallService(MS_CLIST_ADDMAINMENUITEM, (WPARAM)0, (LPARAM)&mi);

		mi.pszName = LPGEN("Show the &list of available channels");
		mi.icolibItem = GetIconHandle(IDI_LIST);
		strcpy( d, IRC_SHOWLIST );
		mi.popupPosition = 500090003;
		mi.pszPopupName = ALTIRCPROTONAME;
		hMenuList = (void *)CallService(MS_CLIST_ADDMAINMENUITEM, (WPARAM)0, (LPARAM)&mi);

		mi.pszName = LPGEN("&Show the server window");
		mi.icolibItem = GetIconHandle(IDI_SERVER);
		strcpy( d, IRC_SHOWSERVER );
		mi.popupPosition = 500090004;
		mi.pszPopupName = ALTIRCPROTONAME;
		hMenuServer = (void *)CallService(MS_CLIST_ADDMAINMENUITEM, (WPARAM)0, (LPARAM)&mi);
	}

	mi.pszName = LPGEN("&Leave the channel");
	mi.icolibItem = GetIconHandle(IDI_DELETE);
	strcpy( d, IRC_MENU1CHANNEL );
	mi.pszContactOwner = IRCPROTONAME;
	mi.popupPosition = 500090000;
	hContactMenu1 = (void *)CallService(MS_CLIST_ADDCONTACTMENUITEM, (WPARAM)0, (LPARAM)&mi);

	mi.pszName = LPGEN("&User details");
	mi.icolibItem = GetIconHandle(IDI_WHOIS);
	strcpy( d, IRC_MENU2CHANNEL );
	mi.pszContactOwner = IRCPROTONAME;
	mi.popupPosition = 500090001;
	hContactMenu2 = (void *)CallService(MS_CLIST_ADDCONTACTMENUITEM, (WPARAM)0, (LPARAM)&mi);

	mi.pszName = LPGEN("&Ignore");
	mi.icolibItem = GetIconHandle(IDI_BLOCK);
	strcpy( d, IRC_MENU3CHANNEL );
	mi.pszContactOwner = IRCPROTONAME;
	mi.popupPosition = 500090002;
	hContactMenu3 = (void *)CallService(MS_CLIST_ADDCONTACTMENUITEM, (WPARAM)0, (LPARAM)&mi);

	CLISTMENUITEM clmi;
	memset( &clmi, 0, sizeof( clmi ));
	clmi.cbSize = sizeof( clmi );
	clmi.flags = CMIM_FLAGS | CMIF_GRAYED;
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuJoin, ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuList, ( LPARAM )&clmi );
	CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuNick, ( LPARAM )&clmi );
	if ( !prefs->UseServer )
		CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuServer, ( LPARAM )&clmi );
}

static int Service_FileAllow(WPARAM wParam,LPARAM lParam)
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	DCCINFO* di = ( DCCINFO* )ccs->wParam;;

	if ( g_ircSession ) {
		TCHAR* ptszFileName = mir_a2t_cp(( char* )ccs->lParam, g_ircSession.getCodepage());
		di->sPath = ptszFileName;
		di->sFileAndPath = di->sPath + di->sFile;
		mir_free( ptszFileName );

		CDccSession* dcc = new CDccSession( di );
		g_ircSession.AddDCCSession( di, dcc );
		dcc->Connect();
	}
	else delete di;

	return ccs->wParam;
}

static int Service_FileDeny(WPARAM wParam,LPARAM lParam)
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	DCCINFO* di = ( DCCINFO* )ccs->wParam;

	delete di;
	return 0;
}

static int Service_FileCancel(WPARAM wParam,LPARAM lParam)
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	DCCINFO* di = ( DCCINFO* )ccs->wParam;

	CDccSession* dcc = g_ircSession.FindDCCSession(di);

	if (dcc) {
		InterlockedExchange(&dcc->dwWhatNeedsDoing, (long)FILERESUME_CANCEL);
		SetEvent(dcc->hEvent);
		dcc->Disconnect();
	}
	return 0;
}

static int Service_FileSend(WPARAM wParam,LPARAM lParam)
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	char **files = (char **) ccs->lParam;
	DCCINFO* dci = NULL;
	int iPort = 0;
	int index= 0;
	DWORD size = 0;

	// do not send to channels :-P
	if (DBGetContactSettingByte(ccs->hContact, IRCPROTONAME, "ChatRoom", 0) != 0)
		return 0;

	// stop if it is an active type filetransfer and the user's IP is not known
	unsigned long ulAdr = 0;
	if (prefs->ManualHost)
		ulAdr = ConvertIPToInteger(prefs->MySpecifiedHostIP);
	else
		ulAdr = ConvertIPToInteger(prefs->IPFromServer?prefs->MyHost:prefs->MyLocalHost);

	if (!prefs->DCCPassive && !ulAdr) {
		DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), TranslateT("DCC ERROR: Unable to automatically resolve external IP"), NULL, NULL, NULL, true, false);
		return 0;
	}

	if ( files[index] ) {

		//get file size
		FILE * hFile = NULL;
		while (files[index] && hFile == 0) {
			hFile = fopen ( files[index] , "rb" );
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
			DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), TranslateT("DCC ERROR: No valid files specified"), NULL, NULL, NULL, true, false);
			return 0;
		}

		DBVARIANT dbv;
		if ( !DBGetContactSettingTString( ccs->hContact, IRCPROTONAME, "Nick", &dbv )) {
			// set up a basic DCCINFO struct and pass it to a DCC object
			dci = new DCCINFO;
			dci->sFileAndPath = (TString)_A2T( files[index], g_ircSession.getCodepage());

			int i = dci->sFileAndPath.rfind( _T("\\"), dci->sFileAndPath.length());
			if (i != string::npos) {
				dci->sPath = dci->sFileAndPath.substr(0, i+1);
				dci->sFile = dci->sFileAndPath.substr(i+1, dci->sFileAndPath.length());
			}

			TString sFileWithQuotes = dci->sFile;

			// if spaces in the filename surround witrh quotes
			if ( sFileWithQuotes.find( ' ', 0 ) != string::npos ) {
				sFileWithQuotes.insert( 0, _T("\""));
				sFileWithQuotes.insert( sFileWithQuotes.length(), _T("\""));
			}

			dci->hContact = ccs->hContact;
			dci->sContactName = dbv.ptszVal;
			dci->iType = DCC_SEND;
			dci->bReverse = prefs->DCCPassive?true:false;
			dci->bSender = true;
			dci->dwSize = size;

			// create new dcc object
			CDccSession* dcc = new CDccSession(dci);

			// keep track of all objects created
			g_ircSession.AddDCCSession(dci, dcc);

			// need to make sure that %'s are doubled to avoid having chat interpret as color codes
			TString sFileCorrect = ReplaceString(dci->sFile, _T("%"), _T("%%"));

			// is it an reverse filetransfer (receiver acts as server)
			if (dci->bReverse) {
				TCHAR szTemp[256];
				PostIrcMessage( _T("/CTCP %s DCC SEND ") _T(TCHAR_STR_PARAM) _T(" 200 0 %u %u"), dci->sContactName.c_str(), sFileWithQuotes.c_str(), dci->dwSize, dcc->iToken);

				mir_sntprintf(szTemp, SIZEOF(szTemp), 
					TranslateT("DCC reversed file transfer request sent to %s [%s]"), 
					dci->sContactName.c_str(), sFileCorrect.c_str());
				DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false);

				if (prefs->SendNotice) {
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
					DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), szTemp, NULL, NULL, NULL, true, false);

					if ( prefs->SendNotice ) {
						mir_sntprintf(szTemp, SIZEOF(szTemp), 
							_T("/NOTICE %s I am sending the file \'\002%s\002\' (%u kB) to you, please accept it. [IP: %s]"),
							dci->sContactName.c_str(), sFileCorrect.c_str(), dci->dwSize/1024, (TCHAR*)_A2T(ConvertIntegerToIP(ulAdr)));
						PostIrcMessage(szTemp);
					}
				}
				else DoEvent(GC_EVENT_INFORMATION, 0, g_ircSession.GetInfo().sNick.c_str(), 
					TranslateT("DCC ERROR: Unable to bind local port"), NULL, NULL, NULL, true, false);
			}

			// fix for sending multiple files
			index++;
			while( files[index] ) {
				hFile = NULL;
				hFile = fopen( files[index] , "rb" );
				if ( hFile ) {
					fclose(hFile);
					PostIrcMessage( _T("/DCC SEND %s ") _T(TCHAR_STR_PARAM), dci->sContactName.c_str(), files[index]);
				}
				index++;
			}

			DBFreeVariant( &dbv );
	}	}

	if (dci)
		return (int)(HANDLE) dci;
	return NULL;
}

static int Service_FileReceive(WPARAM wParam,LPARAM lParam)
{
	DBEVENTINFO dbei;
	CCSDATA* ccs = ( CCSDATA* )lParam;
	PROTORECVEVENT *pre = (PROTORECVEVENT *) ccs->lParam;
	char *szDesc, *szFile;

	DBDeleteContactSetting(ccs->hContact, "CList", "Hidden");
	szFile = pre->szMessage + sizeof(DWORD);
	szDesc = szFile + strlen(szFile) + 1;
	ZeroMemory(&dbei, sizeof(dbei));
	dbei.cbSize = sizeof(dbei);
	dbei.szModule = IRCPROTONAME;
	dbei.timestamp = pre->timestamp;
	dbei.flags = pre->flags & (PREF_CREATEREAD ? DBEF_READ : 0);
	dbei.eventType = EVENTTYPE_FILE;
	dbei.cbBlob = sizeof(DWORD) + strlen(szFile) + strlen(szDesc) + 2;
	dbei.pBlob = (PBYTE) pre->szMessage;
	CallService(MS_DB_EVENT_ADD, (WPARAM) ccs->hContact, (LPARAM) &dbei);
	return 0;
}

static int Service_FileResume(WPARAM wParam,LPARAM lParam)
{
	DCCINFO* di = ( DCCINFO* )wParam;
	PROTOFILERESUME * pfr = (PROTOFILERESUME *) lParam;

	long i = (long)pfr->action;

	CDccSession* dcc = g_ircSession.FindDCCSession(di);
	if (dcc) {
		InterlockedExchange(&dcc->dwWhatNeedsDoing, i);
		if (pfr->action == FILERESUME_RENAME) {
			char * szTemp = strdup(pfr->szFilename);
			InterlockedExchange((long*)&dcc->NewFileName, (long)szTemp);
		}

		if (pfr->action == FILERESUME_RESUME) {
			DWORD dwPos = 0;
			String sFile;
			char * pszTemp = NULL;;
			FILE * hFile = NULL;

			hFile = _tfopen(di->sFileAndPath.c_str(), _T("rb"));
			if (hFile) {
				fseek(hFile,0,SEEK_END);
				dwPos = ftell(hFile);
				rewind (hFile);
				fclose(hFile); hFile = NULL;
			}

			TString sFileWithQuotes = di->sFile;

			// if spaces in the filename surround witrh quotes
			if (sFileWithQuotes.find( ' ', 0 ) != string::npos ) {
				sFileWithQuotes.insert( 0, _T("\""));
				sFileWithQuotes.insert( sFileWithQuotes.length(), _T("\""));
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

static int Service_EventDoubleclicked(WPARAM wParam,LPARAM lParam)
{
	if (!lParam)
		return 0;

	CLISTEVENT* pcle = (CLISTEVENT*)lParam;

	if (DBGetContactSettingByte((HANDLE) pcle->hContact, IRCPROTONAME, "DCC", 0) != 0) {
		DCCINFO* pdci = ( DCCINFO* )pcle->lParam;
		HWND hWnd = CreateDialogParam(g_hInstance,MAKEINTRESOURCE(IDD_MESSAGEBOX),NULL,MessageboxWndProc, (LPARAM)pdci);
		TCHAR szTemp[500];
		mir_sntprintf( szTemp, SIZEOF(szTemp), TranslateT("%s (%s) is requesting a client-to-client chat connection."), 
			pdci->sContactName.c_str(), pdci->sHostmask.c_str());
		SetDlgItemText( hWnd, IDC_TEXT, szTemp );
		ShowWindow( hWnd, SW_SHOW );
		return 1;
	}
	return 0;
}

int Service_UserDeletedContact(WPARAM wp, LPARAM lp)
{
	HANDLE hContact = ( HANDLE )wp;
	if ( !hContact )
		return 0;

	DBVARIANT dbv;
	if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "Nick", &dbv )) {
		int type = DBGetContactSettingByte( hContact, IRCPROTONAME, "ChatRoom", 0 );
		if ( type != 0 ) {
			GCEVENT gce = {0};
			GCDEST gcd = {0};
			TString S = _T("");
			if (type == GCW_CHATROOM)
				S = MakeWndID( dbv.ptszVal );
			if (type == GCW_SERVER)
				S = _T("Network log");
			gce.cbSize = sizeof(GCEVENT);
			gce.dwItemData = 0;
			gcd.iType = GC_EVENT_CONTROL;
			gcd.pszModule = IRCPROTONAME;
			gce.dwFlags = GC_TCHAR;
			gce.pDest = &gcd;
			gcd.ptszID = ( TCHAR* )S.c_str();
			int i = CallChatEvent( SESSION_TERMINATE, (LPARAM)&gce);
			if (i && type == GCW_CHATROOM)
				PostIrcMessage( _T("/PART %s"), dbv.ptszVal);
		}
		else {
			BYTE bDCC = DBGetContactSettingByte((HANDLE)wp, IRCPROTONAME, "DCC", 0) ;
			if ( bDCC ) {
				CDccSession* dcc = g_ircSession.FindDCCSession((HANDLE)wp);
				if ( dcc )
					dcc->Disconnect();
		}	}

		DBFreeVariant(&dbv);
	}
	return 0;
}

static int Service_Menu1Command(WPARAM wp, LPARAM lp)
{
	if ( !wp )
		return 0;

	DBVARIANT dbv;
	if ( !DBGetContactSettingTString(( HANDLE )wp, IRCPROTONAME, "Nick", &dbv )) {
		int type = DBGetContactSettingByte((HANDLE)wp, IRCPROTONAME, "ChatRoom", 0);
		if ( type != 0) {
			GCEVENT gce = {0};
			GCDEST gcd = {0};
			TString S = _T("");
			if ( type == GCW_CHATROOM)
				S = MakeWndID( dbv.ptszVal );
			if ( type == GCW_SERVER )
				S = _T("Network log");
			gcd.iType = GC_EVENT_CONTROL;
			gcd.ptszID = ( TCHAR* )S.c_str();
			gce.dwFlags = GC_TCHAR;
			gcd.pszModule = IRCPROTONAME;
			gce.cbSize = sizeof(GCEVENT);
			gce.pDest = &gcd;
			CallChatEvent( WINDOW_VISIBLE, (LPARAM)&gce);
		}
		else CallService(MS_MSG_SENDMESSAGE, (WPARAM)wp, 0);
	}
	return 0;
}

static int Service_Menu2Command(WPARAM wp, LPARAM lp)
{
	DBVARIANT dbv;

	if (!wp )
		return 0;

	if (!DBGetContactSettingTString((HANDLE)wp, IRCPROTONAME, "Nick", &dbv)) {
		int type = DBGetContactSettingByte((HANDLE)wp, IRCPROTONAME, "ChatRoom", 0);
		if ( type != 0 ) {
			if (type == GCW_CHATROOM)
				PostIrcMessage( _T("/PART %s"), dbv.ptszVal);

			GCEVENT gce = {0};
			GCDEST gcd = {0};
			TString S = _T("");
			if (type == GCW_CHATROOM)
				S = MakeWndID(dbv.ptszVal);
			if (type == GCW_SERVER)
				S = _T("Network log");
			gce.cbSize = sizeof(GCEVENT);
			gce.dwFlags = GC_TCHAR;
			gcd.iType = GC_EVENT_CONTROL;
			gcd.pszModule = IRCPROTONAME;
			gce.pDest = &gcd;
			gcd.ptszID = ( TCHAR* )S.c_str();
			CallChatEvent( SESSION_TERMINATE, (LPARAM)&gce);
		}
		else {
			BYTE bDCC = DBGetContactSettingByte((HANDLE)wp, IRCPROTONAME, "DCC", 0) ;
			if (bDCC) {
				CDccSession* dcc = g_ircSession.FindDCCSession((HANDLE)wp);
				if ( dcc )
					dcc->Disconnect();
			}
			else PostIrcMessage( _T("/WHOIS %s"), dbv.ptszVal);
		}
		DBFreeVariant(&dbv);
	}
	return 0;
}

static int Service_Menu3Command(WPARAM wp, LPARAM lp)
{
	if (!wp )
		return 0;

	HANDLE hContact = (HANDLE) wp;
	DBVARIANT dbv;
	if ( !DBGetContactSettingTString(hContact, IRCPROTONAME, "Nick", &dbv )) {
		if ( DBGetContactSettingByte((HANDLE)wp, IRCPROTONAME, "ChatRoom", 0) == 0 ) {
			char* host = NULL;
			DBVARIANT dbv1;
			if (!DBGetContactSettingString((HANDLE) wp, IRCPROTONAME, "Host", &dbv1))
				host = dbv1.pszVal;

			if ( host ) {
				String S;
				if (prefs->IgnoreChannelDefault)
					S = "+qnidcm";
				else
					S = "+qnidc";
				PostIrcMessage( _T("/IGNORE %%question=\"%s\",\"%s\",\"*!*@") _T(TCHAR_STR_PARAM) _T("\" %s"),
					TranslateT("Please enter the hostmask (nick!user@host) \nNOTE! Contacts on your contact list are never ignored"), 
					TranslateT("Ignore"), host, S.c_str());
				DBFreeVariant(&dbv1);
			}
		}
		else PostIrcMessageWnd(dbv.ptszVal, NULL, _T("/CHANNELMANAGER"));

		DBFreeVariant(&dbv);
	}
	return 0;
}

static int Service_JoinMenuCommand(WPARAM wp, LPARAM lp)
{
	if ( !join_hWnd )
		join_hWnd = CreateDialog(g_hInstance, MAKEINTRESOURCE(IDD_NICK), NULL, JoinWndProc);

	SetDlgItemText(join_hWnd, IDC_CAPTION, TranslateT("Join channel"));
	SetWindowText(GetDlgItem(join_hWnd, IDC_TEXT), TranslateT("Please enter a channel to join"));
	SendMessage(GetDlgItem(join_hWnd, IDC_ENICK), EM_SETSEL, 0,MAKELPARAM(0,-1));
	ShowWindow(join_hWnd, SW_SHOW);
	SetActiveWindow(join_hWnd);
	return 0;
}

static int Service_QuickConnectMenuCommand(WPARAM wp, LPARAM lp)
{
	if (!quickconn_hWnd)
		quickconn_hWnd = CreateDialog(g_hInstance, MAKEINTRESOURCE(IDD_QUICKCONN), NULL, QuickWndProc);

	SetWindowText( quickconn_hWnd, TranslateT( "Quick connect" ));
	SetDlgItemText( quickconn_hWnd, IDC_TEXT, TranslateT( "Please select IRC network and enter the password if needed" ));
	SetDlgItemText( quickconn_hWnd, IDC_CAPTION, TranslateT( "Quick connect" ));
	SendMessage( quickconn_hWnd, WM_SETICON, ICON_BIG, ( LPARAM )LoadIconEx( IDI_QUICK ));
	ShowWindow( quickconn_hWnd, SW_SHOW );
	SetActiveWindow( quickconn_hWnd );
	return 0;
}

static int Service_ShowListMenuCommand(WPARAM wp, LPARAM lp)
{
	PostIrcMessage( _T("/LIST"));
	return 0;
}

static int Service_ShowServerMenuCommand(WPARAM wp, LPARAM lp)
{
	GCEVENT gce = {0};
	GCDEST gcd = {0};
	gcd.iType = GC_EVENT_CONTROL;
	gcd.ptszID = _T("Network log");
	gce.dwFlags = GC_TCHAR;
	gcd.pszModule = IRCPROTONAME;
	gce.cbSize = sizeof(GCEVENT);
	gce.pDest = &gcd;
	CallChatEvent( WINDOW_VISIBLE, (LPARAM)&gce);
	return 0;
}

static int Service_ChangeNickMenuCommand(WPARAM wp, LPARAM lp)
{
	if ( !nick_hWnd )
		nick_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_NICK), NULL, NickWndProc);

	SetDlgItemText(nick_hWnd, IDC_CAPTION, TranslateT("Change nick name"));
	SetWindowText(GetDlgItem(nick_hWnd, IDC_TEXT), TranslateT("Please enter a unique nickname"));
	SetWindowText(GetDlgItem(nick_hWnd, IDC_ENICK), g_ircSession.GetInfo().sNick.c_str());
	SendMessage(GetDlgItem(nick_hWnd, IDC_ENICK), CB_SETEDITSEL, 0,MAKELPARAM(0,-1));
	ShowWindow(nick_hWnd, SW_SHOW);
	SetActiveWindow(nick_hWnd);
	return 0;
}

static void DoChatFormatting( TCHAR* pszText )
{
	TCHAR* p1 = pszText;
	int iFG = -1;
	int iRemoveChars;
	TCHAR InsertThis[50];

	while (*p1 != '\0') {
		iRemoveChars = 0;
		InsertThis[0] = 0;

		if ( *p1 == '%' ) {
			switch ( p1[1] ) {
			case 'B':
			case 'b':
				lstrcpy(InsertThis, _T("\002"));
				iRemoveChars = 2;
				break;
			case 'I':
			case 'i':
				lstrcpy(InsertThis, _T("\026"));
				iRemoveChars = 2;
				break;
			case 'U':
			case 'u':
				lstrcpy(InsertThis, _T("\037"));
				iRemoveChars = 2;
				break;
			case 'c':
				{
					lstrcpy(InsertThis, _T("\003"));
					iRemoveChars = 2;

					TCHAR szTemp[3];
					lstrcpyn(szTemp, p1 + 2, 3);
					iFG = _ttoi(szTemp);
				}
				break;
			case 'C':
				if ( p1[2] == '%' && p1[3] == 'F') {
					lstrcpy(InsertThis, _T("\00399,99"));
					iRemoveChars = 4;
				}
				else {
					lstrcpy(InsertThis, _T("\00399"));
					iRemoveChars = 2;
				}
				iFG = -1;
				break;
			case 'f':
				if (p1 - 3 >= pszText && p1[-3] == '\003')
					lstrcpy(InsertThis, _T(","));
				else if ( iFG >= 0 )
					mir_sntprintf(InsertThis, SIZEOF(InsertThis), _T("\003%u,"), iFG);
				else
					lstrcpy(InsertThis, _T("\00399,"));

				iRemoveChars = 2;
				break;

			case 'F':
				if (iFG >= 0)
					mir_sntprintf(InsertThis, SIZEOF(InsertThis), _T("\003%u,99"), iFG);
				else
					lstrcpy(InsertThis, _T("\00399,99"));
				iRemoveChars = 2;
				break;

			case '%':
				lstrcpy(InsertThis, _T("%"));
				iRemoveChars = 2;
				break;

			default:
				iRemoveChars = 2;
				break;
			}

			MoveMemory(p1 + lstrlen(InsertThis), p1 + iRemoveChars, sizeof(TCHAR)*(lstrlen(p1) - iRemoveChars + 1));
			CopyMemory(p1, InsertThis, sizeof(TCHAR)*lstrlen(InsertThis));
			if (iRemoveChars || lstrlen(InsertThis))
				p1 += lstrlen(InsertThis);
			else
				p1++;
		}
		else p1++;
}	}

int Service_GCEventHook(WPARAM wParam,LPARAM lParam)
{
	GCHOOK *gchook= (GCHOOK*) lParam;
	GCHOOK *gchtemp = NULL;
	GCHOOK *gch = NULL;
	TString S = _T("");

	EnterCriticalSection(&m_gchook);

	// handle the hook
	if ( gchook ) {
		if (!lstrcmpiA(gchook->pDest->pszModule, IRCPROTONAME)) {

			// first see if the scripting module should modify or stop this event
			if (bMbotInstalled && prefs->ScriptingEnabled && wParam == NULL) {
				gchtemp = (GCHOOK *)mir_alloc(sizeof(GCHOOK));
				gchtemp->pDest = (GCDEST *)mir_alloc(sizeof(GCDEST));
				gchtemp->pDest->iType = gchook->pDest->iType;
				gchtemp->dwData = gchook->dwData;

				if ( gchook->pDest->ptszID ) {
					gchtemp->pDest->ptszID = mir_tstrdup( gchook->pDest->ptszID );
					TCHAR* pTemp = _tcschr(gchtemp->pDest->ptszID, ' ');
					if ( pTemp )
						*pTemp = '\0';
				}
				else gchtemp->pDest->ptszID = NULL;

				gchook->pDest->pszModule = mir_strdup( gchook->pDest->pszModule );
				gchook->ptszText = mir_tstrdup( gchook->ptszText );
				gchook->ptszUID = mir_tstrdup( gchook->ptszUID );

				if ( Scripting_TriggerMSPGuiOut(gchtemp) && gchtemp)
					gch = gchtemp;
				else
					gch = NULL;
			}
			else gch = gchook;

			if ( gch ) {
				TCHAR* p1 = mir_tstrdup( gch->pDest->ptszID );
				TCHAR* p2 = _tcsstr( p1, _T(" - "));
				if ( p2 )
					*p2 = '\0';

				switch( gch->pDest->iType ) {
				case GC_SESSION_TERMINATE:
					FreeWindowItemData(p1, (CHANNELINFO*)gch->dwData);
					break;

				case GC_USER_MESSAGE:
					if (gch && gch->pszText && lstrlen(gch->ptszText) > 0 ) {
						TCHAR* pszText = new TCHAR[lstrlen(gch->ptszText)+1000];
						lstrcpy(pszText, gch->ptszText);
						DoChatFormatting(pszText);
						PostIrcMessageWnd(p1, NULL, pszText);
						delete []pszText;
					}
					break;

				case GC_USER_CHANMGR:
					PostIrcMessageWnd(p1, NULL, _T("/CHANNELMANAGER"));
					break;

				case GC_USER_PRIVMESS:
					{
						TCHAR szTemp[4000];
						mir_sntprintf(szTemp, SIZEOF(szTemp), _T("/QUERY %s"), gch->ptszUID );
						PostIrcMessageWnd(p1, NULL, szTemp);
					}
					break;

				case GC_USER_LOGMENU:
					switch( gch->dwData ) {
					case 1:
						PostIrcMessageWnd(p1, NULL, _T("/CHANNELMANAGER"));
						break;

					case 2:
						PostIrcMessage( _T("/PART %s"), p1 );
						{	GCEVENT gce = {0};
							GCDEST gcd = {0};
							S = MakeWndID(p1);
							gce.cbSize = sizeof(GCEVENT);
							gcd.iType = GC_EVENT_CONTROL;
							gcd.pszModule = IRCPROTONAME;
							gce.dwFlags = GC_TCHAR;
							gce.pDest = &gcd;
							gcd.ptszID = ( TCHAR* )S.c_str();
							CallChatEvent( SESSION_TERMINATE, (LPARAM)&gce);
						}
						break;
					case 3:
						PostIrcMessageWnd(p1, NULL, _T("/SERVERSHOW"));
						break;
					}
					break;

				case GC_USER_NICKLISTMENU:
					switch(gch->dwData) {
					case 1:
						PostIrcMessage( _T("/MODE %s +o %s"), p1, gch->ptszUID );
						break;
					case 2:
						PostIrcMessage( _T("/MODE %s -o %s"), p1, gch->ptszUID );
						break;
					case 3:
						PostIrcMessage( _T("/MODE %s +v %s"), p1, gch->ptszUID );
						break;
					case 4:
						PostIrcMessage( _T("/MODE %s -v %s"), p1, gch->ptszUID );
						break;
					case 5:
						PostIrcMessage( _T("/KICK %s %s"), p1, gch->ptszUID );
						break;
					case 6:
						PostIrcMessage( _T("/KICK %s %s %%question=\"%s\",\"%s\",\"%s\""), 
							p1, gch->ptszUID, TranslateT("Please enter the reason"), TranslateT("Kick"), TranslateT("Jerk") );
						break;
					case 7:
						DoUserhostWithReason(1, _T("B") + (TString)p1, true, _T("%s"), gch->ptszUID );
						break;
					case 8:
						DoUserhostWithReason(1, _T("K") + (TString)p1, true, _T("%s"), gch->ptszUID );
						break;
					case 9:
						DoUserhostWithReason(1, _T("L") + (TString)p1, true, _T("%s"), gch->ptszUID );
						break;
					case 10:
						PostIrcMessage( _T("/WHOIS %s"), gch->ptszUID );
						break;
				//	case 11:
				//		DoUserhostWithReason(1, "I", true, "%s", gch->ptszUID );
				//		break;
				//	case 12:
				//		DoUserhostWithReason(1, "J", true, "%s", gch->ptszUID );
				//		break;
					case 13:
						PostIrcMessage( _T("/DCC CHAT %s"), gch->ptszUID );
						break;
					case 14:
						PostIrcMessage( _T("/DCC SEND %s"), gch->ptszUID );
						break;
					case 15:
						DoUserhostWithReason(1, _T("I"), true, _T("%s"), gch->ptszUID );
						break;
					case 16:
						PostIrcMessage( _T("/MODE %s +h %s"), p1, gch->ptszUID );
						break;
					case 17:
						PostIrcMessage( _T("/MODE %s -h %s"), p1, gch->ptszUID );
						break;
					case 30:
						{
							PROTOSEARCHRESULT psr;
							ZeroMemory(&psr, sizeof(psr));
							psr.cbSize = sizeof(psr);
							psr.nick = ( char* )gch->ptszUID;
							ADDCONTACTSTRUCT acs;
							ZeroMemory(&acs, sizeof(acs));
							acs.handleType = HANDLE_SEARCHRESULT;
							acs.szProto = IRCPROTONAME;
							acs.psr = &psr;
							CallService(MS_ADDCONTACT_SHOW, (WPARAM)NULL, (LPARAM)&acs);
						}
						break;
					}
					break;
				}
				mir_free( p1 );
	}	}	}

	if ( gchtemp ) {
		mir_free(gchtemp->pszUID);
		mir_free(gchtemp->pszText);
		mir_free(gchtemp->pDest->ptszID);
		mir_free(gchtemp->pDest->pszModule);
		mir_free(gchtemp->pDest);
		mir_free(gchtemp);
	}
	LeaveCriticalSection(&m_gchook);
	return 0;
}

static int Service_GCMenuHook(WPARAM wParam,LPARAM lParam)
{
	GCMENUITEMS *gcmi= (GCMENUITEMS*) lParam;
	if (gcmi) {
		if (!lstrcmpiA(gcmi->pszModule, IRCPROTONAME)) {
			if (gcmi->Type == MENU_ON_LOG) {
				if (lstrcmpi(gcmi->pszID, _T("Network log"))) {
					static gc_item Item[] = {
						{ TranslateT("Channel &settings" ), 1, MENU_ITEM, FALSE},
						{ TranslateT("&Leave the channel" ), 2, MENU_ITEM, FALSE},
						{ TranslateT("Show the server &window" ), 3, MENU_ITEM, FALSE}};
						gcmi->nItems = SIZEOF(Item);
						gcmi->Item = &Item[0];
				}
				else gcmi->nItems = 0;
			}

			if (gcmi->Type == MENU_ON_NICKLIST) {
				CONTACT user ={ (TCHAR*)gcmi->pszUID, NULL, NULL, false, false, false};
				HANDLE hContact = CList_FindContact(&user);
				BOOL bFlag = FALSE;

				if (hContact && DBGetContactSettingByte(hContact, "CList", "NotOnList", 0) == 0)
					bFlag = TRUE;

				static gc_item Item[] = {
					{ TranslateT("&WhoIs info"),         10, MENU_ITEM,           FALSE },
					{ TranslateT("&Control"),             0, MENU_NEWPOPUP,       FALSE },
					{ TranslateT("Give &Op"),             1, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Take O&p"),             2, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Give &Halfop"),        16, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Take H&alfop"),        17, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Give &Voice"),          3, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Take V&oice"),          4, MENU_POPUPITEM,      FALSE },
					{ _T(""),                             0, MENU_POPUPSEPARATOR, FALSE },
					{ TranslateT("&Kick"),                5, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Ki&ck (reason)"),       6, MENU_POPUPITEM,      FALSE },
					{ TranslateT("&Ban"),                 7, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Ban'&n kick"),          8, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Ban'n kick (&reason)"), 9, MENU_POPUPITEM,      FALSE },
					{ TranslateT("&Direct Connection"),   0, MENU_NEWPOPUP,       FALSE },
					{ TranslateT("Request &Chat"),       13, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Send &File"),          14, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Add to &ignore list"), 15, MENU_ITEM,           FALSE },
					{ _T(""),                            12, MENU_SEPARATOR,      FALSE },
					{ TranslateT("&Add User"),           30, MENU_ITEM,           bFlag }
				};
				gcmi->nItems = SIZEOF(Item);
				gcmi->Item = &Item[0];
				gcmi->Item[gcmi->nItems-1].bDisabled = bFlag;

				unsigned long ulAdr = 0;
				if (prefs->ManualHost)
					ulAdr = ConvertIPToInteger(prefs->MySpecifiedHostIP);
				else
					ulAdr = ConvertIPToInteger(prefs->IPFromServer?prefs->MyHost:prefs->MyLocalHost);

				bool bDcc = ulAdr == 0 ?false:true;

				gcmi->Item[14].bDisabled = !bDcc;
				gcmi->Item[15].bDisabled = !bDcc;
				gcmi->Item[16].bDisabled = !bDcc;

				bool bHalfop = strchr(sUserModes.c_str(), 'h') == NULL?false:true;
				gcmi->Item[5].bDisabled = !bHalfop;
				gcmi->Item[4].bDisabled = !bHalfop;
	}	}	}

	return 0;
}

static int Service_SystemPreShutdown(WPARAM wParam,LPARAM lParam)
{
	EnterCriticalSection(&cs);

	if ( prefs->Perform && g_ircSession )
		if ( DoPerform( "Event: Disconnect" ))
			Sleep( 200 );

	g_ircSession.DisconnectAllDCCSessions( true );

	if (g_ircSession)
		g_ircSession.Disconnect();
	if (list_hWnd)
		SendMessage(list_hWnd, WM_CLOSE, 0, 0);
	if (nick_hWnd)
		SendMessage(nick_hWnd, WM_CLOSE, 0, 0);
	if (join_hWnd)
		SendMessage(join_hWnd, WM_CLOSE, 0, 0);
	LeaveCriticalSection(&cs);

	return 0;
}

static int Service_MenuPreBuild(WPARAM wParam,LPARAM lParam)
{
	DBVARIANT dbv;

	HANDLE hContact = ( HANDLE )wParam;
	if ( !hContact )
		return 0;

	CLISTMENUITEM clmi = { 0 };
	clmi.cbSize = sizeof( clmi );
	clmi.flags = CMIM_FLAGS | CMIM_NAME | CMIM_ICON;

	char *szProto = ( char* ) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) wParam, 0);
	if (szProto && !lstrcmpiA(szProto, IRCPROTONAME)) {
		if (DBGetContactSettingByte(hContact, IRCPROTONAME, "ChatRoom", 0) == GCW_CHATROOM) {
			clmi.icolibItem = GetIconHandle(IDI_PART);
			clmi.pszName = LPGEN("&Leave channel");
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hContactMenu2, ( LPARAM )&clmi );

			clmi.icolibItem = GetIconHandle(IDI_MANAGER);
			clmi.pszName = LPGEN("Channel &settings");
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hContactMenu3, ( LPARAM )&clmi );

			clmi.icolibItem = GetIconHandle(IDI_SHOW);
			clmi.pszName = LPGEN("Show channel");
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hContactMenu1, ( LPARAM )&clmi );
		}
		else if ( DBGetContactSettingByte(hContact, IRCPROTONAME, "ChatRoom", 0) == GCW_SERVER ) {
			clmi.icolibItem = GetIconHandle(IDI_SERVER);
			clmi.pszName = LPGEN("&Show server");
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hContactMenu1, ( LPARAM )&clmi );

			clmi.flags = CMIM_FLAGS | CMIF_HIDDEN;
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hContactMenu2, ( LPARAM )&clmi );
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hContactMenu3, ( LPARAM )&clmi );
		}
		else if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "Default", &dbv )) {
			BYTE bDcc = DBGetContactSettingByte( hContact, IRCPROTONAME, "DCC", 0) ;

			clmi.flags = CMIM_FLAGS | CMIF_HIDDEN;
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hContactMenu1, ( LPARAM )&clmi );

			clmi.flags = CMIM_NAME | CMIM_ICON | CMIM_FLAGS;
			if ( bDcc ) {
				if (DBGetContactSettingWord( hContact, IRCPROTONAME, "Status", ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE)
					clmi.flags = CMIM_NAME|CMIM_ICON | CMIM_FLAGS |CMIF_HIDDEN;
				clmi.icolibItem = GetIconHandle(IDI_DELETE);
				clmi.pszName = LPGEN("Di&sconnect");
				CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hContactMenu2, ( LPARAM )&clmi );
			}
			else {
				if ( !g_ircSession )
					clmi.flags = CMIM_NAME | CMIM_ICON | CMIM_FLAGS |CMIF_HIDDEN;
				clmi.icolibItem = GetIconHandle(IDI_WHOIS);
				clmi.pszName = LPGEN("&WhoIs info");
				CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hContactMenu2, ( LPARAM )&clmi );
			}

			if (!g_ircSession || bDcc)
				clmi.flags = CMIM_NAME | CMIM_ICON | CMIM_FLAGS |CMIF_HIDDEN;
			else
				clmi.flags = CMIM_NAME | CMIM_ICON | CMIM_FLAGS;

			clmi.icolibItem = GetIconHandle(IDI_BLOCK);
			if (DBGetContactSettingWord( hContact, IRCPROTONAME, "Status", ID_STATUS_OFFLINE) != ID_STATUS_OFFLINE) {
				char * host = NULL;
				DBVARIANT dbv3;
				if ( !DBGetContactSetting( hContact, IRCPROTONAME, "Host", &dbv3) && dbv3.type == DBVT_ASCIIZ ) {
					host = dbv3.pszVal;
					clmi.pszName = LPGEN("&Add to ignore list");
					DBFreeVariant( &dbv3 );
				}
				else clmi.flags = CMIM_NAME | CMIM_ICON | CMIM_FLAGS |CMIF_HIDDEN;
			}
			else clmi.flags = CMIM_NAME | CMIM_ICON | CMIM_FLAGS |CMIF_HIDDEN;

			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hContactMenu3, ( LPARAM )&clmi );
			DBFreeVariant( &dbv );
	}	}

	return 0;
}

static int Service_GetCaps(WPARAM wParam,LPARAM lParam)
{
	switch( wParam ) {
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
		return (int) "Default";
	}

	return 0;
}

static int Service_GetName(WPARAM wParam,LPARAM lParam)
{
	lstrcpynA(( char* ) lParam, ALTIRCPROTONAME, wParam);
	return 0;
}

static int Service_LoadIcon(WPARAM wParam,LPARAM lParam)
{
	switch(wParam&0xFFFF) {
		case PLI_PROTOCOL:
			return (int)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_MAIN),IMAGE_ICON,16,16,LR_SHARED);
		default: return (int)(HICON)NULL;
	}
	return 0;
}

static void __cdecl AckBasicSearch(void * pszNick)
{
	PROTOSEARCHRESULT psr;
	ZeroMemory(&psr, sizeof(psr));
	psr.cbSize = sizeof(psr);
	psr.nick = ( char* )pszNick;
	ProtoBroadcastAck(IRCPROTONAME, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) 1, (LPARAM) & psr);
	ProtoBroadcastAck(IRCPROTONAME, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

static int Service_BasicSearch(WPARAM wParam,LPARAM lParam)
{
	static char buf[50];
	if (lParam) {
		lstrcpynA(buf, (const char *)lParam, 50);
		if (OldStatus != ID_STATUS_OFFLINE && OldStatus != ID_STATUS_CONNECTING && lstrlenA(buf) >0 && !IsChannel(buf)) {
			mir_forkthread(AckBasicSearch, &buf );
			return 1;
	}	}

	return 0;
}

static int Service_AddToList(WPARAM wParam,LPARAM lParam)
{
	HANDLE hContact;
	PROTOSEARCHRESULT *psr = (PROTOSEARCHRESULT *) lParam;
	CONTACT user = { (TCHAR *)psr->nick, NULL, NULL, true, false, false};

	if (OldStatus == ID_STATUS_OFFLINE || OldStatus == ID_STATUS_CONNECTING)
		return 0;
	hContact = CList_AddContact(&user, true, false);

	if ( hContact ) {
		DBVARIANT dbv1;

		if (DBGetContactSettingByte(hContact, IRCPROTONAME, "AdvancedMode", 0) == 0) {
			TCHAR* p = mir_a2t( psr->nick );
			DoUserhostWithReason( 1, ((TString)_T("S") + p).c_str(), true, p );
			mir_free( p );
		}
		else {
			if (!DBGetContactSettingTString(hContact, IRCPROTONAME, "UWildcard", &dbv1)) {
				DoUserhostWithReason(2, ((TString)_T("S") + dbv1.ptszVal).c_str(), true, dbv1.ptszVal);
				DBFreeVariant(&dbv1);
			}
			else {
				TCHAR* p = mir_a2t( psr->nick );
				DoUserhostWithReason( 2, ((TString)_T("S") + p).c_str(), true, p );
				mir_free( p );
	}	}	}

	return (int) hContact;
}

static void __cdecl ConnectServerThread(LPVOID di)
{
	EnterCriticalSection(&cs);
	InterlockedIncrement((long *) &bConnectThreadRunning);
	InterlockedIncrement((long *) &bConnectRequested);
	while ( !Miranda_Terminated() && bConnectRequested > 0 ) {
		while(bConnectRequested > 0)
			InterlockedDecrement((long *) &bConnectRequested);
		if (g_ircSession) {
			Sleep(200);
			g_ircSession.Disconnect();
		}

		g_ircSession.GetInfo().bNickFlag = false;
		int Temp = OldStatus;
		OldStatus = ID_STATUS_CONNECTING;
		nickflag = false;
		ProtoBroadcastAck(IRCPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)Temp,ID_STATUS_CONNECTING);
		Sleep(100);
		g_ircSession.Connect(si);
		if (g_ircSession) {
			KillChatTimer( RetryTimer );

			if ( lstrlenA( prefs->MySpecifiedHost ))
				mir_forkthread( ResolveIPThread, new IPRESOLVE( prefs->MySpecifiedHost, IP_MANUAL ));
		}
		else {
			Temp = OldStatus;
			OldStatus = ID_STATUS_OFFLINE;
			ProtoBroadcastAck(IRCPROTONAME,NULL,ACKTYPE_STATUS,ACKRESULT_SUCCESS,(HANDLE)Temp,ID_STATUS_OFFLINE);
			Sleep(100);
	}	}

	InterlockedDecrement((long *) &bConnectThreadRunning);
	LeaveCriticalSection(&cs);
	return;
}

static void __cdecl DisconnectServerThread(LPVOID di)
{
	EnterCriticalSection(&cs);
	KillChatTimer(RetryTimer);
	if (g_ircSession)
		g_ircSession.Disconnect();
	LeaveCriticalSection(&cs);
	return;
}

void ConnectToServer(void)
{
	PortCount = StrToIntA(prefs->PortStart);
	si.sServer = GetWord(prefs->ServerName, 0);
	si.iPort = PortCount;
	si.sNick = prefs->Nick;
	si.sUserID = prefs->UserID;
	si.sFullName = prefs->Name;
	si.sPassword = prefs->Password;
	si.bIdentServer =  ((prefs->Ident) ? (true) : (false));
	si.iIdentServerPort = StrToInt(prefs->IdentPort);
	si.sIdentServerType = prefs->IdentSystem;
	si.iSSL = prefs->iSSL;
	{	TCHAR* p = mir_a2t( prefs->Network );
		si.sNetwork = p;
		mir_free(p);
	}
	RetryCount = 1;
	KillChatTimer(RetryTimer);
	if (prefs->Retry) {
		if (StrToInt(prefs->RetryWait)<10)
			lstrcpy(prefs->RetryWait, _T("10"));
		SetChatTimer(RetryTimer, StrToInt(prefs->RetryWait)*1000, RetryTimerProc);
	}

	bPerformDone = false;
	bTempDisableCheck = false;
	bTempForceCheck = false;
	iTempCheckTime = 0;
	sChannelPrefixes = _T("&#");
	sUserModes = "ov";
	sUserModePrefixes = _T("@+");
	sChannelModes = "btnimklps";

	if (!bConnectThreadRunning)
		mir_forkthread(ConnectServerThread, NULL );
	else if (bConnectRequested < 1)
		InterlockedIncrement((long *) &bConnectRequested);

	TCHAR szTemp[300];
	mir_sntprintf(szTemp, SIZEOF(szTemp), _T("\0033%s \002%s\002 (") _T(TCHAR_STR_PARAM) _T(": %u)"), 
		TranslateT("Connecting to"), si.sNetwork.c_str(), si.sServer.c_str(), si.iPort);
	DoEvent(GC_EVENT_INFORMATION, _T("Network log"), NULL, szTemp, NULL, NULL, NULL, true, false);
}

void DisconnectFromServer(void)
{
	GCEVENT gce = {0};
	GCDEST gcd = {0};

	if ( prefs->Perform && g_ircSession )
		DoPerform( "Event: Disconnect" );

	gcd.iType = GC_EVENT_CONTROL;
	gcd.ptszID = NULL; // all windows
	gcd.pszModule = IRCPROTONAME;
	gce.cbSize = sizeof(GCEVENT);
	gce.pDest = &gcd;

	CallChatEvent( SESSION_TERMINATE, (LPARAM)&gce);
	mir_forkthread(DisconnectServerThread, NULL );
}

static int Service_SetStatus(WPARAM wParam,LPARAM lParam)
{
	if ( !bChatInstalled ) {
		MIRANDASYSTRAYNOTIFY msn;
		msn.cbSize = sizeof(MIRANDASYSTRAYNOTIFY);
		msn.szProto = IRCPROTONAME;
		msn.tszInfoTitle = TranslateT( "Information" );
		msn.tszInfo = TranslateT( "This protocol is dependent on another plugin named \'Chat\'\nPlease download it from the Miranda IM website!" );
		msn.dwInfoFlags = NIIF_ERROR | NIIF_INTERN_UNICODE;
		msn.uTimeout = 15000;
		CallService( MS_CLIST_SYSTRAY_NOTIFY, 0, ( LPARAM )&msn );
		return 0;
	}

	if ( wParam != ID_STATUS_OFFLINE && lstrlenA(prefs->Network) < 1 ) {
		if (lstrlen(prefs->Nick) > 0 && !prefs->DisableDefaultServer) {
			HWND hwnd=CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_QUICKCONN),NULL,QuickWndProc);
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

	if (wParam != ID_STATUS_OFFLINE && (lstrlen(prefs->Nick) <1 || lstrlen(prefs->UserID) < 1 || lstrlen(prefs->Name) < 1)) {
		MIRANDASYSTRAYNOTIFY msn;
		msn.cbSize = sizeof( MIRANDASYSTRAYNOTIFY );
		msn.szProto = IRCPROTONAME;
		msn.tszInfoTitle = TranslateT( "IRC error" );
		msn.tszInfo = TranslateT( "Connection can not be established! You have not completed all necessary fields (Nickname, User ID and Name)." );
		msn.dwInfoFlags = NIIF_ERROR | NIIF_INTERN_UNICODE;
		msn.uTimeout = 15000;
		CallService(MS_CLIST_SYSTRAY_NOTIFY, (WPARAM)NULL,(LPARAM) &msn);
		return 0;
	}

	if ( wParam == ID_STATUS_FREECHAT && prefs->Perform && g_ircSession )
		DoPerform( "Event: Free for chat" );
	if ( wParam == ID_STATUS_ONTHEPHONE && prefs->Perform && g_ircSession )
		DoPerform( "Event: On the phone" );
	if ( wParam == ID_STATUS_OUTTOLUNCH && prefs->Perform && g_ircSession )
		DoPerform( "Event: Out for lunch" );
	if ( wParam == ID_STATUS_ONLINE && prefs->Perform && g_ircSession && (GlobalStatus ==ID_STATUS_ONTHEPHONE ||GlobalStatus  ==ID_STATUS_OUTTOLUNCH) && OldStatus  !=ID_STATUS_AWAY)
		DoPerform( "Event: Available" );
	if ( lParam != 1 )
		GlobalStatus = wParam;

	if ((wParam == ID_STATUS_ONLINE || wParam == ID_STATUS_AWAY || wParam == ID_STATUS_FREECHAT) && !g_ircSession ) //go from offline to online
		ConnectToServer();
	else if ((wParam == ID_STATUS_ONLINE || wParam == ID_STATUS_FREECHAT) && g_ircSession && OldStatus == ID_STATUS_AWAY) //go to online while connected
	{
		StatusMessage = _T("");
		PostIrcMessage( _T("/AWAY"));
		return 0;
	}
	else if (wParam == ID_STATUS_OFFLINE && g_ircSession) //go from online/away to offline
		DisconnectFromServer();
	else if (wParam == ID_STATUS_OFFLINE && !g_ircSession) //offline to offline
	{
		KillChatTimer( RetryTimer);
		return 0;
	}
	else if (wParam == ID_STATUS_AWAY && g_ircSession) //go to away while connected
		return 0;
	else if (wParam == ID_STATUS_ONLINE && g_ircSession) //already online
		return 0;
	else
		Service_SetStatus(ID_STATUS_AWAY, 1);
	return 0;
}

static int Service_GetStatus(WPARAM wParam,LPARAM lParam)
{
	if (OldStatus == ID_STATUS_CONNECTING)
		return ID_STATUS_CONNECTING;
	else if (g_ircSession && OldStatus == ID_STATUS_ONLINE)
		return ID_STATUS_ONLINE;
	else if (g_ircSession && OldStatus == ID_STATUS_AWAY)
		return ID_STATUS_AWAY;
	else
		return ID_STATUS_OFFLINE;
}

static int Service_SetAwayMsg(WPARAM wParam, LPARAM lParam)
{
	if ( wParam != ID_STATUS_ONLINE && wParam != ID_STATUS_INVISIBLE && wParam != ID_STATUS_FREECHAT && wParam != ID_STATUS_CONNECTING && wParam != ID_STATUS_OFFLINE) {
		TString newStatus = _A2T(( char* )lParam, g_ircSession.getCodepage());
		if ( StatusMessage.empty() || lParam == NULL || StatusMessage != ReplaceString( newStatus, _T("\r\n"), _T(" "))) {
			if (lParam == NULL ||  *(char*)lParam == '\0')
				StatusMessage = _T(STR_AWAYMESSAGE);
			else
				StatusMessage =  ReplaceString( newStatus, _T("\r\n"), _T(" "));

			PostIrcMessage( _T("/AWAY ") _T(TCHAR_STR_PARAM), StatusMessage.substr(0,450).c_str());
	}	}

	return 0;
}

static int Service_AddIncMessToDB(WPARAM wParam, LPARAM lParam)
{
	return CallService( MS_PROTO_RECVMSG, wParam, lParam );
}

static void __cdecl AckMessageFail(void * lParam)
{
	ProtoBroadcastAck(IRCPROTONAME, (void*)lParam, ACKTYPE_MESSAGE, ACKRESULT_FAILED, (HANDLE) 1, (LONG)Translate("The protocol is not online"));
}

static void __cdecl AckMessageFailDcc(void * lParam)
{
	ProtoBroadcastAck(IRCPROTONAME, (void*)lParam, ACKTYPE_MESSAGE, ACKRESULT_FAILED, (HANDLE) 1, (LONG)Translate("The dcc chat connection is not active"));
}

static void __cdecl AckMessageSuccess(void * lParam)
{
	ProtoBroadcastAck(IRCPROTONAME, (void*)lParam, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

static int Service_GetMessFromSRMM(WPARAM wParam, LPARAM lParam)
{
	CCSDATA* ccs = ( CCSDATA* )lParam;

	BYTE bDcc = DBGetContactSettingByte(ccs->hContact, IRCPROTONAME, "DCC", 0) ;
	WORD wStatus = DBGetContactSettingWord(ccs->hContact, IRCPROTONAME, "Status", ID_STATUS_OFFLINE) ;
	if ( OldStatus != ID_STATUS_OFFLINE && OldStatus != ID_STATUS_CONNECTING && !bDcc || bDcc && wStatus == ID_STATUS_ONLINE ) {
		int codepage = g_ircSession.getCodepage();
		char*  msg = ( char* )ccs->lParam;
		TCHAR* result;
		if ( ccs->wParam & PREF_UNICODE ) {
			char* p = strchr( msg, '\0' );
			if ( p != msg ) {
				while ( *(++p) == '\0' )
					;
				result = mir_u2t_cp(( wchar_t* )p, codepage );
			}
			else result = mir_a2t_cp( msg, codepage );
		}
		else if ( ccs->wParam & PREF_UTF ) {
			#if defined( _UNICODE )
				mir_utf8decode( NEWSTR_ALLOCA(msg), &result );
			#else
				result = mir_strdup( msg );
				mir_utf8decodecp( result, codepage, NULL );
			#endif
		}
		else result = mir_a2t_cp( msg, codepage );

		PostIrcMessageWnd(NULL, ccs->hContact, result );
		mir_free( result );
		mir_forkthread(AckMessageSuccess, ccs->hContact);
	}
	else {
		if ( bDcc )
			mir_forkthread(AckMessageFailDcc, ccs->hContact);
		else
			mir_forkthread(AckMessageFail, ccs->hContact);
	}

	return 1;
}

static int Service_GetAwayMessage(WPARAM wParam, LPARAM lParam)
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	WhoisAwayReply = _T("");
	DBVARIANT dbv;

	// bypass chat contacts.
	if (DBGetContactSettingByte(ccs->hContact, IRCPROTONAME, "ChatRoom", 0) == 0) {
		if (ccs->hContact && !DBGetContactSettingTString(ccs->hContact, IRCPROTONAME, "Nick", &dbv)) {
			int i = DBGetContactSettingWord(ccs->hContact,IRCPROTONAME, "Status", ID_STATUS_OFFLINE);
			if ( i != ID_STATUS_AWAY) {
				DBFreeVariant( &dbv);
				return 0;
			}
			TString S = _T("WHOIS ") + (TString)dbv.ptszVal;
			if (g_ircSession)
				g_ircSession << CIrcMessage(S.c_str(), g_ircSession.getCodepage(), false, false);
			DBFreeVariant( &dbv);
	}	}

	return 1;
}

static int Service_InitUserInfo(WPARAM wParam, LPARAM lParam)
{
	char* szProto = ( char* )CallService(MS_PROTO_GETCONTACTBASEPROTO, lParam, 0);
	HANDLE hContact = (HANDLE) lParam;
	if ( !hContact || !szProto || lstrcmpiA( szProto, IRCPROTONAME ))
		return 0;

	if ( DBGetContactSettingByte( hContact, IRCPROTONAME, "ChatRoom", 0 ) != 0 )
		return 0;

	if ( DBGetContactSettingByte( hContact, IRCPROTONAME, "DCC", 0 ) != 0 )
		return 0;

	DBVARIANT dbv;
	if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "Default", &dbv )) {
		if ( IsChannel( dbv.ptszVal )) {
			DBFreeVariant( &dbv );
			return 0;
		}
		DBFreeVariant(&dbv);
	}

	OPTIONSDIALOGPAGE odp = { 0 };
	odp.cbSize = sizeof(odp);
	odp.pszTitle = ALTIRCPROTONAME;
	odp.hIcon = NULL;
	odp.hInstance = g_hInstance;
	odp.position = -1900000000;
	odp.pfnDlgProc = UserDetailsDlgProc;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_USERINFO);
	odp.pszTitle = ALTIRCPROTONAME;
	CallService(MS_USERINFO_ADDPAGE, wParam, (LPARAM) & odp);
	return 0;
}

static int Service_ModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	char szTemp[MAX_PATH];
	char szTemp3[256];
	NETLIBUSER nlu = {0};

	DBDeleteContactSetting( NULL, IRCPROTONAME, "JTemp" );

	AddIcons();

	nlu.cbSize = sizeof(nlu);
	nlu.flags = NUF_OUTGOING|NUF_INCOMING|NUF_HTTPCONNS;
	nlu.szSettingsModule = IRCPROTONAME;
	mir_snprintf(szTemp, sizeof(szTemp), Translate("%s server connection"), ALTIRCPROTONAME);
	nlu.szDescriptiveName = szTemp;
	hNetlib=(HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

	char szTemp2[256];
	nlu.flags = NUF_OUTGOING|NUF_INCOMING|NUF_HTTPCONNS;
	mir_snprintf(szTemp2, sizeof(szTemp2), "%s DCC", IRCPROTONAME);
	nlu.szSettingsModule = szTemp2;
	mir_snprintf(szTemp, sizeof(szTemp), Translate("%s client-to-client connections"), ALTIRCPROTONAME);
	nlu.szDescriptiveName = szTemp;
	hNetlibDCC=(HANDLE)CallService(MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

	//add as a known module in DB Editor ++
	CallService("DBEditorpp/RegisterSingleModule",(WPARAM)IRCPROTONAME,0);
	mir_snprintf(szTemp3, sizeof(szTemp3), "%s DCC", IRCPROTONAME);
	CallService("DBEditorpp/RegisterSingleModule",(WPARAM)szTemp3,0);

	if ( ServiceExists("MBot/GetFcnTable")) {
		CallService(MS_MBOT_REGISTERIRC, 0, (LPARAM)IRCPROTONAME);
		bMbotInstalled = TRUE;
	}

	if ( ServiceExists( MS_GC_REGISTER )) {
		GCREGISTER gcr = {0};

		GCPTRS gp = {0};
		CallService(MS_GC_GETEVENTPTR, 0, (LPARAM)&gp);
		pfnAddEvent = gp.pfnAddEvent;

		gcr.cbSize = sizeof(GCREGISTER);
		gcr.dwFlags = GC_CHANMGR|GC_BOLD|GC_ITALICS|GC_UNDERLINE|GC_COLOR|GC_BKGCOLOR;
		gcr.iMaxText = 0;
		gcr.nColors = 16;
		gcr.pColors = prefs->colors;
		gcr.pszModuleDispName = IRCPROTONAME;
		gcr.pszModule = IRCPROTONAME;
		CallServiceSync(MS_GC_REGISTER, NULL, (LPARAM)&gcr);
		g_hGCUserEvent = HookEvent(ME_GC_EVENT, Service_GCEventHook);
		g_hGCMenuBuild = HookEvent(ME_GC_BUILDMENU, Service_GCMenuHook );

		GCSESSION gcw = {0};
		GCDEST gcd = {0};
		GCEVENT gce = {0};

		gcw.cbSize = sizeof(GCSESSION);
		gcw.iType = GCW_SERVER;
		gcw.ptszID = _T("Network log");
		gcw.pszModule = IRCPROTONAME;
		gcw.ptszName = TranslateT("Offline");
		gcw.dwFlags = GC_TCHAR;
		CallServiceSync(MS_GC_NEWSESSION, 0, (LPARAM)&gcw);

		gce.cbSize = sizeof(GCEVENT);
		gce.dwFlags = GC_TCHAR;
		gce.pDest = &gcd;
		gcd.ptszID = _T("Network log");
		gcd.pszModule = IRCPROTONAME;
		gcd.iType = GC_EVENT_CONTROL;

		gce.pDest = &gcd;
		if ( prefs->UseServer && !prefs->HideServerWindow )
			CallChatEvent( WINDOW_VISIBLE, (LPARAM)&gce);
		else
			CallChatEvent( WINDOW_HIDDEN, (LPARAM)&gce);
		bChatInstalled = TRUE;
	}
	else {
		if ( IDYES == MessageBox(0,TranslateT("The IRC protocol depends on another plugin called \'Chat\'\n\nDo you want to download it from the Miranda IM web site now?"),TranslateT("Information"),MB_YESNO|MB_ICONINFORMATION ))
			CallService(MS_UTILS_OPENURL, 1, (LPARAM) "http://www.miranda-im.org/download/details.php?action=viewfile&id=1309");
	}

	mir_snprintf(szTemp, sizeof(szTemp), "%s\\%s_servers.ini", mirandapath, IRCPROTONAME);
	pszServerFile = IrcLoadFile(szTemp);

	mir_snprintf(szTemp, sizeof(szTemp), "%s\\%s_perform.ini", mirandapath, IRCPROTONAME);
	char* pszPerformData = IrcLoadFile( szTemp );
	if ( pszPerformData != NULL ) {
		char *p1 = pszPerformData, *p2 = pszPerformData;
		while (( p1 = strstr( p2, "NETWORK: " )) != NULL ) {
			p1 += 9;
			p2 = strchr(p1, '\n');
			String sNetwork( p1, int( p2-p1-1 ));
			p1 = p2;
			p2 = strstr( ++p1, "\nNETWORK: " );
			if ( !p2 )
				p2 = p1 + lstrlenA( p1 )-1;
			if ( p1 == p2 )
				break;

			*p2++ = 0;
			DBWriteContactSettingString( NULL, IRCPROTONAME, ("PERFORM:" + sNetwork).c_str(), p1 );
		}
		delete[] pszPerformData;
		::remove( szTemp );
	}

	InitIgnore();
	InitMenus();

	g_hUserInfoInit =	HookEvent(ME_USERINFO_INITIALISE, Service_InitUserInfo);
	g_hMenuCreation =	HookEvent(ME_CLIST_PREBUILDCONTACTMENU, Service_MenuPreBuild );
	g_hOptionsInit  = HookEvent(ME_OPT_INITIALISE, InitOptionsPages);

	if (lstrlen(prefs->Nick) == 0)
		CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_INIT),NULL, InitWndProc);
	else {
		TCHAR szBuf[ 40 ];
		if ( lstrlen( prefs->AlternativeNick ) == 0 ) {
			mir_sntprintf( szBuf, SIZEOF(szBuf), _T("%s%u"), prefs->Nick, rand()%9999);
			DBWriteContactSettingTString(NULL, IRCPROTONAME, "AlernativeNick", szBuf);
			lstrcpyn(prefs->AlternativeNick, szBuf, 30);
		}

		if ( lstrlen( prefs->Name ) == 0 ) {
			mir_sntprintf( szBuf, SIZEOF(szBuf), _T("Miranda%u"), rand()%9999);
			DBWriteContactSettingTString(NULL, IRCPROTONAME, "Name", szBuf);
			lstrcpyn( prefs->Name, szBuf, 200 );
	}	}

	return 0;
}

void HookEvents(void)
{
	g_hModulesLoaded =     HookEvent(ME_SYSTEM_MODULESLOADED, Service_ModulesLoaded);
	g_hSystemPreShutdown = HookEvent(ME_SYSTEM_PRESHUTDOWN, Service_SystemPreShutdown);
	g_hContactDeleted =    HookEvent(ME_DB_CONTACT_DELETED, Service_UserDeletedContact );
}

void UnhookEvents(void)
{
	UnhookEvent(g_hModulesLoaded);
	UnhookEvent(g_hSystemPreShutdown);
	UnhookEvent(g_hContactDeleted);
	UnhookEvent(g_hUserInfoInit);
	UnhookEvent(g_hMenuCreation);
	UnhookEvent(g_hGCUserEvent);
	UnhookEvent(g_hGCMenuBuild);
	Netlib_CloseHandle(hNetlib);
	Netlib_CloseHandle(hNetlibDCC);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Service function creation

static void CreateProtoService( const char* serviceName, MIRANDASERVICE pFunc )
{
	char temp[MAXMODULELABELLENGTH];
	mir_snprintf( temp, sizeof(temp), "%s%s", IRCPROTONAME, serviceName );
		CreateServiceFunction( temp, pFunc );
}

void CreateServiceFunctions( void )
{
	CreateProtoService( PS_ADDTOLIST,     Service_AddToList );
	CreateProtoService( PS_BASICSEARCH,   Service_BasicSearch );
	CreateProtoService( PS_FILERESUME,    Service_FileResume );
	CreateProtoService( PS_GETCAPS,       Service_GetCaps );
	CreateProtoService( PS_GETNAME,       Service_GetName );
	CreateProtoService( PS_GETSTATUS,     Service_GetStatus );
	CreateProtoService( PS_LOADICON,      Service_LoadIcon );
	CreateProtoService( PS_SETAWAYMSG,    Service_SetAwayMsg );
	CreateProtoService( PS_SETSTATUS,     Service_SetStatus );

	CreateProtoService( PSR_FILE,         Service_FileReceive);
	CreateProtoService( PSR_MESSAGE,      Service_AddIncMessToDB );
	CreateProtoService( PSS_FILE,         Service_FileSend );
	CreateProtoService( PSS_FILEALLOW,    Service_FileAllow );
	CreateProtoService( PSS_FILECANCEL,   Service_FileCancel );
	CreateProtoService( PSS_FILEDENY,     Service_FileDeny );
	CreateProtoService( PSS_GETAWAYMSG,   Service_GetAwayMessage );
	CreateProtoService( PSS_MESSAGE,      Service_GetMessFromSRMM );

	CreateProtoService( IRC_JOINCHANNEL,  Service_JoinMenuCommand );
	CreateProtoService( IRC_QUICKCONNECT, Service_QuickConnectMenuCommand);
	CreateProtoService( IRC_CHANGENICK,   Service_ChangeNickMenuCommand );
	CreateProtoService( IRC_SHOWLIST,     Service_ShowListMenuCommand );
	CreateProtoService( IRC_SHOWSERVER,   Service_ShowServerMenuCommand );
	CreateProtoService( IRC_MENU1CHANNEL, Service_Menu1Command );
	CreateProtoService( IRC_MENU2CHANNEL, Service_Menu2Command );
	CreateProtoService( IRC_MENU3CHANNEL, Service_Menu3Command );

	CreateProtoService( "/DblClickEvent", Service_EventDoubleclicked );
	CreateProtoService( "/InsertRawIn",   Scripting_InsertRawIn );
	CreateProtoService( "/InsertRawOut",  Scripting_InsertRawOut );
	CreateProtoService( "/InsertGuiIn",   Scripting_InsertGuiIn );
	CreateProtoService( "/InsertGuiOut",  Scripting_InsertGuiOut);
	CreateProtoService( "/GetIrcData",    Scripting_GetIrcData);
}

VOID CALLBACK RetryTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{
	if (RetryCount <= StrToInt(prefs->RetryCount) && prefs->Retry) {
		PortCount++;
		if (PortCount > StrToIntA(prefs->PortEnd) || StrToIntA(prefs->PortEnd) ==0)
			PortCount = StrToIntA(prefs->PortStart);
		si.iPort = PortCount;

		TCHAR szTemp[300];
		mir_sntprintf(szTemp, SIZEOF(szTemp), _T("\0033%s \002%s\002 (") _T(TCHAR_STR_PARAM) _T(": %u, try %u)"),
			TranslateT("Reconnecting to"), si.sNetwork.c_str(), si.sServer.c_str(), si.iPort, RetryCount);

		DoEvent(GC_EVENT_INFORMATION, _T("Network log"), NULL, szTemp, NULL, NULL, NULL, true, false);

		if (!bConnectThreadRunning)
			mir_forkthread(ConnectServerThread, NULL  );
		else
			bConnectRequested = true;

		RetryCount++;
	}
	else KillChatTimer(RetryTimer);
}
