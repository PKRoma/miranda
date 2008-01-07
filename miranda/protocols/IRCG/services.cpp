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

#include <algorithm>

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
HANDLE hUMenuShowChannel = NULL;
HANDLE hUMenuJoinLeave = NULL;
HANDLE hUMenuChanSettings = NULL;
HANDLE hUMenuWhois = NULL;
HANDLE hUMenuDisconnect = NULL;
HANDLE hUMenuIgnore = NULL;
HANDLE hMenuRoot = NULL;
HANDLE hMenuQuick = NULL;
HANDLE hMenuJoin = NULL;
HANDLE hMenuNick = NULL;
HANDLE hMenuList = NULL;
HANDLE hMenuServer = NULL;
HANDLE hNetlib = NULL;
HANDLE hNetlibDCC = NULL;

HANDLE hpsAddToList     = NULL;
HANDLE hpsBasicSearch   = NULL;
HANDLE hpsFileResume    = NULL;
HANDLE hpsGetCaps	    = NULL;
HANDLE hpsGetName	    = NULL;
HANDLE hpsGetStatus	    = NULL;
HANDLE hpsLoadIcon	    = NULL;
HANDLE hpsSetAwayMsg    = NULL;
HANDLE hpsSetStatus	    = NULL;

HANDLE hpsRFile		    = NULL;
HANDLE hpsRMessage	    = NULL;
HANDLE hpsSFile		    = NULL;
HANDLE hpsSFileAllow    = NULL;
HANDLE hpsSFileCancel   = NULL;
HANDLE hpsSFileDeny	    = NULL;
HANDLE hpsSGetAwayMsg   = NULL;
HANDLE hpsSMessage	    = NULL;

HANDLE hpsJionChannel   = NULL;
HANDLE hpsQuickConnect  = NULL;
HANDLE hpsChangeNick    = NULL;
HANDLE hpsShowList	    = NULL;
HANDLE hpsShowServer    = NULL;
HANDLE hpsMenuShowChannel = NULL;
HANDLE hpsMenuJoinLeave = NULL;
HANDLE hpsMenuChanSettings = NULL;
HANDLE hpsMenuWhois		= NULL;
HANDLE hpsMenuDisconnect= NULL;
HANDLE hpsMenuIgnore	= NULL;

HANDLE hpsEDblCkick	    = NULL;
HANDLE hpsCInsertRawIn  = NULL;
HANDLE hpsCInsertRawOut = NULL;
HANDLE hpsCInsertGuiIn  = NULL;
HANDLE hpsCInsertGuiOut = NULL;
HANDLE hpsCGetIrcData   = NULL;

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

extern bool				nickflag;

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
		// Root popupmenuitem
		mi.pszName = ALTIRCPROTONAME;
		mi.position = -1999901010;
		mi.pszPopupName = (char *)-1;
		mi.flags |= CMIF_ROOTPOPUP;
		mi.icolibItem = GetIconHandle(IDI_MAIN);
		hMenuRoot = (HANDLE)CallService( MS_CLIST_ADDMAINMENUITEM,  (WPARAM)0, (LPARAM)&mi);
		
		mi.flags &= ~CMIF_ROOTPOPUP;
		mi.flags |= CMIF_CHILDPOPUP;

		mi.pszName = LPGEN("&Quick connect");
		mi.icolibItem = GetIconHandle(IDI_QUICK);
		strcpy( d, IRC_QUICKCONNECT );
		mi.position = 500090000;
		mi.pszPopupName = (char *)hMenuRoot;
		hMenuQuick= (HANDLE)CallService( MS_CLIST_ADDMAINMENUITEM, (WPARAM)0, (LPARAM)&mi);

		mi.pszName = LPGEN("&Join channel");
		mi.icolibItem = GetIconHandle(IDI_JOIN);
		strcpy( d, IRC_JOINCHANNEL );
		mi.position = 500090001;
		mi.pszPopupName = (char *)hMenuRoot;
		hMenuJoin = (HANDLE)CallService( MS_CLIST_ADDMAINMENUITEM, (WPARAM)0, (LPARAM)&mi);

		mi.pszName = LPGEN("&Change your nickname");
		mi.icolibItem = GetIconHandle(IDI_WHOIS);
		strcpy( d, IRC_CHANGENICK );
		mi.position = 500090002;
		mi.pszPopupName = (char *)hMenuRoot;
		hMenuNick = (HANDLE)CallService( MS_CLIST_ADDMAINMENUITEM, (WPARAM)0, (LPARAM)&mi);

		mi.pszName = LPGEN("Show the &list of available channels");
		mi.icolibItem = GetIconHandle(IDI_LIST);
		strcpy( d, IRC_SHOWLIST );
		mi.position = 500090003;
		mi.pszPopupName = (char *)hMenuRoot;
		hMenuList = (HANDLE)CallService( MS_CLIST_ADDMAINMENUITEM, (WPARAM)0, (LPARAM)&mi);

		mi.pszName = LPGEN("&Show the server window");
		mi.icolibItem = GetIconHandle(IDI_SERVER);
		strcpy( d, IRC_SHOWSERVER );
		mi.position = 500090004;
		mi.pszPopupName = (char *)hMenuRoot;
		hMenuServer = (HANDLE)CallService( MS_CLIST_ADDMAINMENUITEM, (WPARAM)0, (LPARAM)&mi);
	}
	
	mi.flags &= ~CMIF_CHILDPOPUP;

	mi.pszName = LPGEN("Sho&w channel");
	mi.icolibItem = GetIconHandle(IDI_SHOW);
	strcpy( d, IRC_UM_SHOWCHANNEL );
	mi.pszContactOwner = IRCPROTONAME;
	mi.popupPosition = 500090000;
	hUMenuShowChannel = (void *)CallService( MS_CLIST_ADDCONTACTMENUITEM, (WPARAM)0, (LPARAM)&mi);

	mi.pszName = LPGEN("&Leave channel");
	mi.icolibItem = GetIconHandle(IDI_PART);
	strcpy( d, IRC_UM_JOINLEAVE );
	mi.pszContactOwner = IRCPROTONAME;
	mi.popupPosition = 500090001;
	hUMenuJoinLeave = (void *)CallService( MS_CLIST_ADDCONTACTMENUITEM, (WPARAM)0, (LPARAM)&mi);

	mi.pszName = LPGEN("Channel &settings");
	mi.icolibItem = GetIconHandle(IDI_MANAGER);
	strcpy( d, IRC_UM_CHANSETTINGS );
	mi.pszContactOwner = IRCPROTONAME;
	mi.popupPosition = 500090002;
	hUMenuChanSettings = (void *)CallService( MS_CLIST_ADDCONTACTMENUITEM, (WPARAM)0, (LPARAM)&mi);

	mi.pszName = LPGEN("&WhoIs info");
	mi.icolibItem = GetIconHandle(IDI_WHOIS);
	strcpy( d, IRC_UM_WHOIS );
	mi.pszContactOwner = IRCPROTONAME;
	mi.popupPosition = 500090001;
	hUMenuWhois = (void *)CallService( MS_CLIST_ADDCONTACTMENUITEM, (WPARAM)0, (LPARAM)&mi);

	mi.pszName = LPGEN("Di&sconnect");
	mi.icolibItem = GetIconHandle(IDI_DELETE);
	strcpy( d, IRC_UM_DISCONNECT );
	mi.pszContactOwner = IRCPROTONAME;
	mi.popupPosition = 500090001;
	hUMenuDisconnect = (void *)CallService( MS_CLIST_ADDCONTACTMENUITEM, (WPARAM)0, (LPARAM)&mi);

	mi.pszName = LPGEN("&Add to ignore list");
	mi.icolibItem = GetIconHandle(IDI_BLOCK);
	strcpy( d, IRC_UM_IGNORE );
	mi.pszContactOwner = IRCPROTONAME;
	mi.popupPosition = 500090002;
	hUMenuIgnore = (void *)CallService( MS_CLIST_ADDCONTACTMENUITEM, (WPARAM)0, (LPARAM)&mi);

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
			TString sFileCorrect = dci->sFile;
			ReplaceString(sFileCorrect, _T("%"), _T("%%"));

			// is it an reverse filetransfer (receiver acts as server)
			if (dci->bReverse) {
				TCHAR szTemp[256];
				PostIrcMessage( _T("/CTCP %s DCC SEND %s 200 0 %u %u"),
					dci->sContactName.c_str(), sFileWithQuotes.c_str(), dci->dwSize, dcc->iToken);

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
	CallService( MS_DB_EVENT_ADD, (WPARAM) ccs->hContact, (LPARAM) &dbei);
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
			char * szTemp = _strdup(pfr->szFilename);
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
				S = SERVERWINDOW;
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

static int Service_MenuShowChannel(WPARAM wp, LPARAM lp)
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
				S = SERVERWINDOW;
			gcd.iType = GC_EVENT_CONTROL;
			gcd.ptszID = ( TCHAR* )S.c_str();
			gce.dwFlags = GC_TCHAR;
			gcd.pszModule = IRCPROTONAME;
			gce.cbSize = sizeof(GCEVENT);
			gce.pDest = &gcd;
			CallChatEvent( WINDOW_VISIBLE, (LPARAM)&gce);
		}
		else CallService( MS_MSG_SENDMESSAGE, (WPARAM)wp, 0);
	}
	return 0;
}

static int Service_MenuJoinLeave(WPARAM wp, LPARAM lp)
{
	DBVARIANT dbv;

	if (!wp )
		return 0;

	if (!DBGetContactSettingTString((HANDLE)wp, IRCPROTONAME, "Nick", &dbv)) {
		int type = DBGetContactSettingByte((HANDLE)wp, IRCPROTONAME, "ChatRoom", 0);
		if ( type != 0 ) {
			if (type == GCW_CHATROOM)
				{
				if (DBGetContactSettingWord((HANDLE)wp, IRCPROTONAME, "Status", ID_STATUS_OFFLINE)== ID_STATUS_OFFLINE)
					PostIrcMessage( _T("/JOIN %s"), dbv.ptszVal);
				else
					{
					PostIrcMessage( _T("/PART %s"), dbv.ptszVal);
					GCEVENT gce = {0};
					GCDEST gcd = {0};
					TString S = MakeWndID(dbv.ptszVal);
					gce.cbSize = sizeof(GCEVENT);
					gce.dwFlags = GC_TCHAR;
					gcd.iType = GC_EVENT_CONTROL;
					gcd.pszModule = IRCPROTONAME;
					gce.pDest = &gcd;
					gcd.ptszID = ( TCHAR* )S.c_str();
					CallChatEvent( SESSION_TERMINATE, (LPARAM)&gce);
					}
				}

		}
		DBFreeVariant(&dbv);
	}
	return 0;
}

static int Service_MenuChanSettings(WPARAM wp, LPARAM lp)
{
	if (!wp )
		return 0;

	HANDLE hContact = (HANDLE) wp;
	DBVARIANT dbv;
	if ( !DBGetContactSettingTString(hContact, IRCPROTONAME, "Nick", &dbv )) {
		PostIrcMessageWnd(dbv.ptszVal, NULL, _T("/CHANNELMANAGER"));
		DBFreeVariant(&dbv);
	}
	return 0;
}

static int Service_MenuWhois(WPARAM wp, LPARAM lp)
{
	if ( !wp )
		return 0;

	DBVARIANT dbv;

	if (!DBGetContactSettingTString((HANDLE)wp, IRCPROTONAME, "Nick", &dbv)) {
		PostIrcMessage( _T("/WHOIS %s %s"), dbv.ptszVal, dbv.ptszVal);
		DBFreeVariant(&dbv);
	}
	return 0;
}

static int Service_MenuDisconnect(WPARAM wp, LPARAM lp)
{
	CDccSession* dcc = g_ircSession.FindDCCSession((HANDLE)wp);
	if ( dcc )
		dcc->Disconnect();
	return 0;
}

static int Service_MenuIgnore(WPARAM wp, LPARAM lp)
{
	if ( !wp )
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
	gcd.ptszID = SERVERWINDOW;
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
				//MBOT CORRECTIONS
//*
				gchook->pDest->pszModule = mir_strdup( gchook->pDest->pszModule );
				gchook->ptszText = mir_tstrdup( gchook->ptszText );
				gchook->ptszUID = mir_tstrdup( gchook->ptszUID );
/*/
				gchtemp->pDest->pszModule = mir_strdup( gchook->pDest->pszModule );
				gchtemp->ptszText = mir_tstrdup( gchook->ptszText );
				gchtemp->ptszUID = mir_tstrdup( gchook->ptszUID );
*/
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
						Service_ChangeNickMenuCommand(NULL, NULL);
						break;
					case 2:
						PostIrcMessageWnd(p1, NULL, _T("/CHANNELMANAGER"));
						break;

					case 3:
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
					case 4:		// show server window
						PostIrcMessageWnd(p1, NULL, _T("/SERVERSHOW"));
						break;
/*					case 5:		// nickserv register nick
						PostIrcMessage( _T("/nickserv REGISTER %%question=\"%s\",\"%s\""),
							TranslateT("Please enter your authentification code"), TranslateT("Authentificate nick") );
						break;
*/					case 6:		// nickserv Identify
						PostIrcMessage( _T("/nickserv AUTH %%question=\"%s\",\"%s\""),
							TranslateT("Please enter your authentification code"), TranslateT("Authentificate nick") );
						break;
					case 7:		// nickserv drop nick
						if (MessageBox(0, TranslateT("Are you sure you want to unregister your current nick?"), TranslateT("Delete nick"),
								MB_ICONERROR + MB_YESNO + MB_DEFBUTTON2) == IDYES)
							PostIrcMessage( _T("/nickserv DROP"));
						break;
					case 8:		// nickserv Identify
						{
						HWND question_hWnd = CreateDialog( g_hInstance, MAKEINTRESOURCE(IDD_QUESTION), NULL, QuestionWndProc );
						HWND hEditCtrl = GetDlgItem( question_hWnd, IDC_EDIT);
						SetDlgItemText( question_hWnd, IDC_CAPTION, TranslateT("Identify nick"));
						SetWindowText( GetDlgItem( question_hWnd, IDC_TEXT), TranslateT("Please enter your password") );
						SetDlgItemText( question_hWnd, IDC_HIDDENEDIT, _T("/nickserv IDENTIFY %question=\"%s\",\"%s\""));
						SetWindowLong(GetDlgItem( question_hWnd, IDC_EDIT), GWL_STYLE,
							(LONG)GetWindowLong(GetDlgItem( question_hWnd, IDC_EDIT), GWL_STYLE) | ES_PASSWORD);
						SendMessage(hEditCtrl, EM_SETPASSWORDCHAR,(WPARAM)_T('*'),0 );
						SetFocus(hEditCtrl);
						PostMessage( question_hWnd, IRC_ACTIVATE, 0, 0);
						}
						break;
					case 9:		// nickserv remind password
						{
						DBVARIANT dbv;
						if ( !DBGetContactSettingTString( NULL, IRCPROTONAME, "Nick", &dbv ))
							{
							PostIrcMessage( _T("/nickserv SENDPASS %s"), dbv.ptszVal);
							DBFreeVariant( &dbv );
							}
						}
						break;
					case 10:		// nickserv set new password
						PostIrcMessage( _T("/nickserv SET PASSWORD %%question=\"%s\",\"%s\""),
							TranslateT("Please enter your new password"), TranslateT("Set new password") );
						break;
					case 11:		// nickserv set language
						PostIrcMessage( _T("/nickserv SET LANGUAGE %%question=\"%s\",\"%s\""),
							TranslateT("Please enter desired languageID (numeric value, depends on server)"), TranslateT("Change language of NickServ messages") );
						break;
					case 12:		// nickserv set homepage
						PostIrcMessage( _T("/nickserv SET URL %%question=\"%s\",\"%s\""),
						TranslateT("Please enter URL that will be linked to your nick"), TranslateT("Set URL, linked to nick") );
						break;
					case 13:		// nickserv set email
						PostIrcMessage( _T("/nickserv SET EMAIL %%question=\"%s\",\"%s\""),
							TranslateT("Please enter your e-mail, that will be linked to your nick"), TranslateT("Set e-mail, linked to nick") );
						break;
					case 14:		// nickserv set info
						PostIrcMessage( _T("/nickserv SET INFO %%question=\"%s\",\"%s\""),
							TranslateT("Please enter some information about your nick"), TranslateT("Set information for nick") );
						break;
					case 15:		// nickserv kill unauth off
						PostIrcMessage( _T("/nickserv SET KILL OFF"));
							break;
					case 16:		// nickserv kill unauth on
						PostIrcMessage( _T("/nickserv SET KILL ON"));
							break;
					case 17:		// nickserv kill unauth quick
						PostIrcMessage( _T("/nickserv SET KILL QUICK"));
							break;
					case 18:		// nickserv hide nick from /LIST
						PostIrcMessage( _T("/nickserv SET PRIVATE ON"));
						break;
					case 19:		// nickserv show nick to /LIST
						PostIrcMessage( _T("/nickserv SET PRIVATE OFF"));
						break;
					case 20:		// nickserv Hide e-mail from info
						PostIrcMessage( _T("/nickserv SET HIDE EMAIL ON"));
							break;
					case 21:		// nickserv Show e-mail in info
						PostIrcMessage( _T("/nickserv SET HIDE EMAIL OFF"));
							break;
					case 22:		// nickserv Set security for nick
						PostIrcMessage( _T("/nickserv SET SECURE ON"));
							break;
					case 23:		// nickserv Remove security for nick
						PostIrcMessage( _T("/nickserv SET SECURE OFF"));
							break;
					case 24:		// nickserv Link nick to current
						PostIrcMessage( _T("/nickserv LINK %%question=\"%s\",\"%s\""),
							TranslateT("Please enter nick you want to link to your current nick"), TranslateT("Link another nick to current nick") );
						break;
					case 25:		// nickserv Unlink nick from current
						PostIrcMessage( _T("/nickserv LINK %%question=\"%s\",\"%s\""),
							TranslateT("Please enter nick you want to unlink from your current nick"), TranslateT("Unlink another nick from current nick") );
						break;
					case 26:		// nickserv Set main nick
						PostIrcMessage( _T("/nickserv LINK %%question=\"%s\",\"%s\""),
							TranslateT("Please enter nick you want to set as your main nick"), TranslateT("Set main nick") );
						break;
					case 27:		// nickserv list all linked nicks
						PostIrcMessage( _T("/nickserv LISTLINKS"));
							break;
					case 28:		// nickserv list all channels owned
						PostIrcMessage( _T("/nickserv LISTCHANS"));
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
						PostIrcMessage( _T("/WHOIS %s %s"), gch->ptszUID, gch->ptszUID );
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
					case 18:
						PostIrcMessage( _T("/MODE %s +q %s"), p1, gch->ptszUID );
						break;
					case 19:
						PostIrcMessage( _T("/MODE %s -q %s"), p1, gch->ptszUID );
						break;
					case 20:
						PostIrcMessage( _T("/MODE %s +a %s"), p1, gch->ptszUID );
						break;
					case 21:
						PostIrcMessage( _T("/MODE %s -a %s"), p1, gch->ptszUID );
						break;
					case 22:
						PostIrcMessage( _T("/NOTICE %s %%question=\"%s\",\"%s\""),
							gch->ptszUID, TranslateT("Please enter the notice text"), TranslateT("Send notice") );
						break;
					case 23:
						PostIrcMessage( _T("/INVITE %s %%question=\"%s\",\"%s\""),
							gch->ptszUID, TranslateT("Please enter the channel name to invite to"), TranslateT("Invite to channel") );
						break;
					case 30:
						{
							PROTOSEARCHRESULT psr;
							ZeroMemory(&psr, sizeof(psr));
							psr.cbSize = sizeof(psr);
							psr.nick = mir_t2a_cp( gch->ptszUID, g_ircSession.getCodepage());
							ADDCONTACTSTRUCT acs;
							ZeroMemory(&acs, sizeof(acs));
							acs.handleType = HANDLE_SEARCHRESULT;
							acs.szProto = IRCPROTONAME;
							acs.psr = &psr;
							CallService( MS_ADDCONTACT_SHOW, (WPARAM)NULL, (LPARAM)&acs);
							mir_free( psr.nick );
						}
						break;
					case 31:	//slap
						{
							TCHAR tszTemp[4000];
							mir_sntprintf(tszTemp, SIZEOF(tszTemp), _T("/slap %s"), gch->ptszUID);
							PostIrcMessageWnd(p1, NULL, tszTemp);
						}
						break;
					case 32:  //nickserv info
						{
							TCHAR tszTemp[4000];
							mir_sntprintf(tszTemp, SIZEOF(tszTemp), _T("/nickserv INFO %s ALL"), gch->ptszUID);
							PostIrcMessageWnd(p1, NULL, tszTemp);
						}
						break;
					case 33:  //nickserv ghost
						{
							TCHAR tszTemp[4000];
							mir_sntprintf(tszTemp, SIZEOF(tszTemp), _T("/nickserv GHOST %s"), gch->ptszUID);
							PostIrcMessageWnd(p1, NULL, tszTemp);
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
	if ( gcmi ) {
		if ( !lstrcmpiA( gcmi->pszModule, IRCPROTONAME )) {
			if ( gcmi->Type == MENU_ON_LOG ) {
				if ( lstrcmpi( gcmi->pszID, SERVERWINDOW)) {
					static gc_item Item[] = {
						{ TranslateT("&Change your nickname" ),		1, MENU_ITEM,		FALSE},
						{ TranslateT("Channel &settings" ),			2, MENU_ITEM,		FALSE},
						{ _T(""),									0, MENU_SEPARATOR,	FALSE},
						{ TranslateT("NickServ"),					0, MENU_NEWPOPUP,	FALSE},
						{ TranslateT("Register nick" ),				5, MENU_POPUPITEM,	TRUE},
						{ TranslateT("Auth nick" ),					6, MENU_POPUPITEM,	FALSE},
						{ TranslateT("Delete nick" ),				7, MENU_POPUPITEM,	FALSE},
						{ TranslateT("Identify nick" ),				8, MENU_POPUPITEM,	FALSE},
						{ TranslateT("Remind password " ),			9, MENU_POPUPITEM,	FALSE},
						{ TranslateT("Set new password" ),			10, MENU_POPUPITEM,	TRUE},
						{ TranslateT("Set language" ),				11, MENU_POPUPITEM,	FALSE},
						{ TranslateT("Set homepage" ),				12, MENU_POPUPITEM,	FALSE},
						{ TranslateT("Set e-mail" ),				13, MENU_POPUPITEM,	FALSE},
						{ TranslateT("Set info" ),					14, MENU_POPUPITEM,	FALSE},
						{ _T("" ),									0,	MENU_POPUPSEPARATOR,FALSE},
						{ TranslateT("Hide e-mail from info" ),		20, MENU_POPUPITEM,	FALSE},
						{ TranslateT("Show e-mail in info" ),		21, MENU_POPUPITEM,	FALSE},
						{ _T("" ),									0,	MENU_POPUPSEPARATOR,FALSE},
						{ TranslateT("Set security for nick" ),		22, MENU_POPUPITEM,	FALSE},
						{ TranslateT("Remove security for nick" ),	23, MENU_POPUPITEM,	FALSE},
						{ _T("" ),									0,	MENU_POPUPSEPARATOR,FALSE},
						{ TranslateT("Link nick to current" ),		24, MENU_POPUPITEM,	FALSE},
						{ TranslateT("Unlink nick from current" ),	25, MENU_POPUPITEM,	FALSE},
						{ TranslateT("Set main nick" ),				26, MENU_POPUPITEM,	FALSE},
						{ TranslateT("List all your nicks" ),		27, MENU_POPUPITEM,	FALSE},
						{ TranslateT("List your channels" ),		28, MENU_POPUPITEM,	FALSE},
						{ _T("" ),									0,	MENU_POPUPSEPARATOR,FALSE},
						{ TranslateT("Kill unauthorized: off" ),	15, MENU_POPUPITEM,	FALSE},
						{ TranslateT("Kill unauthorized: on" ),		16, MENU_POPUPITEM,	FALSE},
						{ TranslateT("Kill unauthorized: quick" ),	17, MENU_POPUPITEM,	FALSE},
						{ _T("" ),									0,	MENU_POPUPSEPARATOR,FALSE},
						{ TranslateT("Hide nick from list" ),		18, MENU_POPUPITEM,	FALSE},
						{ TranslateT("Show nick to list" ),			19, MENU_POPUPITEM,	FALSE},
						{ TranslateT("Show the server &window" ),	4, MENU_ITEM,		FALSE},
						{ _T(""),									0, MENU_SEPARATOR,	FALSE},
						{ TranslateT("&Leave the channel" ),		3, MENU_ITEM,		FALSE}
						};
						gcmi->nItems = SIZEOF(Item);
						gcmi->Item = &Item[0];
				}
				else gcmi->nItems = 0;
			}

			if (gcmi->Type == MENU_ON_NICKLIST) {
				CONTACT user ={ (TCHAR*)gcmi->pszUID, NULL, NULL, false, false, false};
				HANDLE hContact = CList_FindContact(&user);

				static gc_item Item[] = {
					{ TranslateT("&WhoIs info"),         10, MENU_ITEM,           FALSE },		//0
					{ TranslateT("&Invite to channel"),  23, MENU_ITEM,           FALSE },
					{ TranslateT("Send &notice"),        22, MENU_ITEM,           FALSE },
					{ TranslateT("&Slap"),		         31, MENU_ITEM,           FALSE },
					{ TranslateT("Nickserv info"),		 32, MENU_ITEM,           FALSE },
					{ TranslateT("Nickserv kill ghost"), 33, MENU_ITEM,           FALSE },		//5
					{ TranslateT("&Control"),             0, MENU_NEWPOPUP,       FALSE },
					{ TranslateT("Give Owner"),          18, MENU_POPUPITEM,      FALSE },		//7
					{ TranslateT("Take Owner"),          19, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Give Admin"),          20, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Take Admin"),          21, MENU_POPUPITEM,      FALSE },		//10
					{ TranslateT("Give &Op"),             1, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Take O&p"),             2, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Give &Halfop"),        16, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Take H&alfop"),        17, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Give &Voice"),          3, MENU_POPUPITEM,      FALSE },		//15
					{ TranslateT("Take V&oice"),          4, MENU_POPUPITEM,      FALSE },
					{ _T(""),                             0, MENU_POPUPSEPARATOR, FALSE },
					{ TranslateT("&Kick"),                5, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Ki&ck (reason)"),       6, MENU_POPUPITEM,      FALSE },
					{ TranslateT("&Ban"),                 7, MENU_POPUPITEM,      FALSE },		//20
					{ TranslateT("Ban'&n kick"),          8, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Ban'n kick (&reason)"), 9, MENU_POPUPITEM,      FALSE },
					{ TranslateT("&Direct Connection"),   0, MENU_NEWPOPUP,       FALSE },
					{ TranslateT("Request &Chat"),       13, MENU_POPUPITEM,      FALSE },
					{ TranslateT("Send &File"),          14, MENU_POPUPITEM,      FALSE },		//25
					{ TranslateT("Add to &ignore list"), 15, MENU_ITEM,           FALSE },
					{ _T(""),                            12, MENU_SEPARATOR,      FALSE },
					{ TranslateT("&Add User"),           30, MENU_ITEM,           FALSE }
				};

				gcmi->nItems = SIZEOF(Item);
				gcmi->Item = &Item[0];
				BOOL bIsInList = (hContact && DBGetContactSettingByte(hContact, "CList", "NotOnList", 0) == 0);
				gcmi->Item[gcmi->nItems-1].bDisabled = bIsInList;

				unsigned long ulAdr = 0;
				if (prefs->ManualHost)
					ulAdr = ConvertIPToInteger(prefs->MySpecifiedHostIP);
				else
					ulAdr = ConvertIPToInteger(prefs->IPFromServer?prefs->MyHost:prefs->MyLocalHost);
				gcmi->Item[23].bDisabled = ulAdr == 0?TRUE:FALSE;		//DCC submenu

				TCHAR stzChanName[100];
				const TCHAR* temp = _tcschr( gcmi->pszID, ' ' );
				int len = min((( temp == NULL ) ? lstrlen( gcmi->pszID ) : ( int )( temp - gcmi->pszID + 1 )), SIZEOF(stzChanName)-1 );
				lstrcpyn( stzChanName, gcmi->pszID, len );
				stzChanName[ len ] = 0;
				CHANNELINFO* wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA,stzChanName, NULL, NULL, NULL, NULL, NULL, false, false, 0);
				BOOL bServOwner  = strchr(sUserModes.c_str(), 'q') == NULL?FALSE:TRUE;
				BOOL bServAdmin  = strchr(sUserModes.c_str(), 'a') == NULL?FALSE:TRUE;
				BOOL bOwner  = bServOwner?((wi->OwnMode>>4)&01):FALSE;
				BOOL bAdmin  = bServAdmin?((wi->OwnMode>>3)&01):FALSE;
				BOOL bOp	 = strchr(sUserModes.c_str(), 'o') == NULL?FALSE:((wi->OwnMode>>2)&01);
				BOOL bHalfop = strchr(sUserModes.c_str(), 'h') == NULL?FALSE:((wi->OwnMode>>1)&01);

				BOOL bForceEnable = GetAsyncKeyState(VK_CONTROL);

				gcmi->Item[6].bDisabled /* "Control" submenu */ = !(bForceEnable|| bHalfop || bOp || bAdmin || bOwner);
				gcmi->Item[7].uType = gcmi->Item[8].uType  = /* +/- Owner */ bServOwner?MENU_POPUPITEM:0;
				gcmi->Item[9].uType = gcmi->Item[10].uType = /* +/- Admin */ bServAdmin?MENU_POPUPITEM:0;
				gcmi->Item[7].bDisabled  = gcmi->Item[8].bDisabled  = gcmi->Item[9].bDisabled  = gcmi->Item[10].bDisabled = /* +/- Owner/Admin */
					!(bForceEnable || bOwner);
				gcmi->Item[11].bDisabled = gcmi->Item[12].bDisabled = gcmi->Item[13].bDisabled = gcmi->Item[14].bDisabled = /* +/- Op/hop */
					!(bForceEnable || bOp || bAdmin || bOwner);
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

	char *szProto = ( char* ) CallService( MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) wParam, 0);
	if (szProto && !lstrcmpiA(szProto, IRCPROTONAME)) {
		bool bIsOnline = DBGetContactSettingWord(hContact, IRCPROTONAME, "Status", ID_STATUS_OFFLINE)== ID_STATUS_OFFLINE ? false : true;
		if (DBGetContactSettingByte(hContact, IRCPROTONAME, "ChatRoom", 0) == GCW_CHATROOM) {
			// context menu for chatrooms
			clmi.flags |= CMIF_NOTOFFLINE;
			clmi.icolibItem = GetIconHandle(IDI_SHOW);
			clmi.pszName = LPGEN("Show channel");
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuShowChannel, ( LPARAM )&clmi );

			clmi.flags = CMIM_FLAGS | CMIF_NOTOFFLINE;
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuChanSettings, ( LPARAM )&clmi );

			clmi.flags = CMIM_FLAGS | CMIM_NAME | CMIM_ICON;

			if (bIsOnline)
				{ // for online chatrooms
				clmi.icolibItem = GetIconHandle(IDI_PART);
				clmi.pszName = LPGEN("&Leave channel");
				}
			else
				{ // for offline chatrooms
				clmi.icolibItem = GetIconHandle(IDI_JOIN);
				clmi.pszName = LPGEN("&Join channel");
				}
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuJoinLeave, ( LPARAM )&clmi );

			clmi.flags = CMIM_FLAGS | CMIF_HIDDEN;
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuWhois,			( LPARAM )&clmi );
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuDisconnect,		( LPARAM )&clmi );
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuIgnore,			( LPARAM )&clmi );
		}
		else if ( DBGetContactSettingByte(hContact, IRCPROTONAME, "ChatRoom", 0) == GCW_SERVER ) {
			//context menu for server window
			clmi.icolibItem = GetIconHandle(IDI_SERVER);
			clmi.pszName = LPGEN("&Show server");
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuShowChannel,		( LPARAM )&clmi );

			clmi.flags = CMIM_FLAGS | CMIF_HIDDEN;
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuJoinLeave,		( LPARAM )&clmi );
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuChanSettings,		( LPARAM )&clmi );
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuWhois,			( LPARAM )&clmi );
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuDisconnect,		( LPARAM )&clmi );
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuIgnore,			( LPARAM )&clmi );
		}
		else if ( !DBGetContactSettingTString( hContact, IRCPROTONAME, "Default", &dbv )) {
			// context menu for contact
			BYTE bDcc = DBGetContactSettingByte( hContact, IRCPROTONAME, "DCC", 0) ;

			clmi.flags = CMIM_FLAGS | CMIF_HIDDEN;
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuShowChannel,		( LPARAM )&clmi );
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuJoinLeave,		( LPARAM )&clmi );
			CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuChanSettings,		( LPARAM )&clmi );

			clmi.flags = CMIM_FLAGS;
			if ( bDcc ) {
				// for DCC contact
				clmi.flags = CMIM_FLAGS | CMIF_NOTOFFLINE;
				CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuDisconnect,		( LPARAM )&clmi );

				clmi.flags = CMIM_FLAGS | CMIF_HIDDEN;
				CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuWhois,			( LPARAM )&clmi );
				CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuIgnore,			( LPARAM )&clmi );
			}
			else {
				// for normal contact
				clmi.flags = CMIM_FLAGS | CMIF_HIDDEN;
				CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuDisconnect,		( LPARAM )&clmi );

				clmi.flags = CMIM_FLAGS | CMIF_NOTOFFLINE;
				if ( !g_ircSession )
					clmi.flags = CMIM_FLAGS | CMIF_HIDDEN;
				CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuWhois, ( LPARAM )&clmi );

				if (bIsOnline) {
					DBVARIANT dbv3;
					if ( !DBGetContactSettingString( hContact, IRCPROTONAME, "Host", &dbv3) ) {
						if (dbv3.pszVal[0] == 0)  
							clmi.flags = CMIM_FLAGS | CMIF_HIDDEN;
						DBFreeVariant( &dbv3 );
					}
				}
				CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hUMenuIgnore, ( LPARAM )&clmi );
			}
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
	if ( lParam ) {
		lstrcpynA(buf, (const char *)lParam, 50);
		if (OldStatus != ID_STATUS_OFFLINE && OldStatus != ID_STATUS_CONNECTING && lstrlenA(buf) >0 && !IsChannel(buf)) {
			mir_forkthread(AckBasicSearch, &buf );
			return 1;
	}	}

	return 0;
}

static int Service_AddToList(WPARAM wParam,LPARAM lParam)
{
	if ( OldStatus == ID_STATUS_OFFLINE || OldStatus == ID_STATUS_CONNECTING )
		return 0;

	PROTOSEARCHRESULT *psr = ( PROTOSEARCHRESULT* ) lParam;
	CONTACT user = { mir_a2t( psr->nick ), NULL, NULL, true, false, false };
	HANDLE hContact = CList_AddContact( &user, true, false );

	if ( hContact ) {
		DBVARIANT dbv1;

		if ( DBGetContactSettingByte( hContact, IRCPROTONAME, "AdvancedMode", 0 ) == 0 )
			DoUserhostWithReason( 1, ((TString)_T("S") + user.name).c_str(), true, user.name );
		else {
			if ( !DBGetContactSettingTString(hContact, IRCPROTONAME, "UWildcard", &dbv1 )) {
				DoUserhostWithReason(2, ((TString)_T("S") + dbv1.ptszVal).c_str(), true, dbv1.ptszVal);
				DBFreeVariant( &dbv1 );
				}
			else DoUserhostWithReason( 2, ((TString)_T("S") + user.name).c_str(), true, user.name );
			}
			if (DBGetContactSettingByte(NULL, IRCPROTONAME,"MirVerAutoRequest", 1))
				PostIrcMessage( _T("/PRIVMSG %s \001VERSION\001"), user.name);
		}

	mir_free( user.name );
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

			DoEvent(GC_EVENT_CHANGESESSIONAME, SERVERWINDOW, NULL, g_ircSession.GetInfo().sNetwork.c_str(), NULL, NULL, NULL, FALSE, TRUE);
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
	DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, szTemp, NULL, NULL, NULL, true, false);
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
		CallService( MS_CLIST_SYSTRAY_NOTIFY, (WPARAM)NULL,(LPARAM) &msn);
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
		ReplaceString( newStatus, _T("\r\n"), _T(" "));
		if ( StatusMessage.empty() || lParam == NULL || StatusMessage != newStatus ) {
			if (lParam == NULL ||  *(char*)lParam == '\0')
				StatusMessage = _T(STR_AWAYMESSAGE);
			else
				StatusMessage = newStatus;

			PostIrcMessage( _T("/AWAY %s"), StatusMessage.substr(0,450).c_str());
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
	char* szProto = ( char* )CallService( MS_PROTO_GETCONTACTBASEPROTO, lParam, 0);
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
	CallService( MS_USERINFO_ADDPAGE, wParam, (LPARAM) & odp);
	return 0;
}

static int sttCheckPerform( const char *szSetting, LPARAM lParam )
{
	if ( !_memicmp( szSetting, "PERFORM:", 8 )) {
		String s = szSetting;
		transform( s.begin(), s.end(), s.begin(), toupper );
		if ( s != szSetting ) {
			vector<String>* p = ( vector<String>* )lParam;
			p->push_back( String( szSetting ));
	}	}
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
	hNetlib=(HANDLE)CallService( MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

	char szTemp2[256];
	nlu.flags = NUF_OUTGOING|NUF_INCOMING|NUF_HTTPCONNS;
	mir_snprintf(szTemp2, sizeof(szTemp2), "%s DCC", IRCPROTONAME);
	nlu.szSettingsModule = szTemp2;
	mir_snprintf(szTemp, sizeof(szTemp), Translate("%s client-to-client connections"), ALTIRCPROTONAME);
	nlu.szDescriptiveName = szTemp;
	hNetlibDCC=(HANDLE)CallService( MS_NETLIB_REGISTERUSER, 0, (LPARAM)&nlu);

	//add as a known module in DB Editor ++
	CallService( "DBEditorpp/RegisterSingleModule",(WPARAM)IRCPROTONAME,0);
	mir_snprintf(szTemp3, sizeof(szTemp3), "%s DCC", IRCPROTONAME);
	CallService( "DBEditorpp/RegisterSingleModule",(WPARAM)szTemp3,0);

	if ( ServiceExists("MBot/GetFcnTable")) {
		CallService( MS_MBOT_REGISTERIRC, 0, (LPARAM)IRCPROTONAME);
		bMbotInstalled = TRUE;
	}

	if ( ServiceExists( MS_GC_REGISTER )) {
		GCREGISTER gcr = {0};
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
		gcw.dwFlags = GC_TCHAR;
		gcw.iType = GCW_SERVER;
		gcw.ptszID = SERVERWINDOW;
		gcw.pszModule = IRCPROTONAME;
		gcw.ptszName = NEWTSTR_ALLOCA( _A2T( prefs->Network ));
		CallServiceSync( MS_GC_NEWSESSION, 0, (LPARAM)&gcw );

		gce.cbSize = sizeof(GCEVENT);
		gce.dwFlags = GC_TCHAR;
		gce.pDest = &gcd;
		gcd.ptszID = SERVERWINDOW;
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
			CallService( MS_UTILS_OPENURL, 1, (LPARAM) "http://www.miranda-im.org/download/");
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
			transform(sNetwork.begin(), sNetwork.end(), sNetwork.begin(), toupper);
			p1 = p2;
			p2 = strstr( ++p1, "\nNETWORK: " );
			if ( !p2 )
				p2 = p1 + lstrlenA( p1 )-1;
			if ( p1 == p2 )
				break;

			*p2++ = 0;
			DBWriteContactSettingString( NULL, IRCPROTONAME, ("PERFORM:" + sNetwork).c_str(), rtrim( p1 ));
		}
		delete[] pszPerformData;
		::remove( szTemp );
	}

	if ( !DBGetContactSettingByte( NULL, IRCPROTONAME, "PerformConversionDone", 0 )) {
		vector<String> performToConvert;
		DBCONTACTENUMSETTINGS dbces;
		dbces.pfnEnumProc = sttCheckPerform;
		dbces.lParam = ( LPARAM )&performToConvert;
		dbces.szModule = IRCPROTONAME;
		CallService( MS_DB_CONTACT_ENUMSETTINGS, NULL, (LPARAM)&dbces );

		for ( size_t i = 0; i < performToConvert.size(); i++ ) {
			String s = performToConvert[i];
			DBVARIANT dbv;
			if ( !DBGetContactSettingTString( NULL, IRCPROTONAME, s.c_str(), &dbv )) {
				DBDeleteContactSetting( NULL, IRCPROTONAME, s.c_str());
				transform( s.begin(), s.end(), s.begin(), toupper );
				DBWriteContactSettingTString( NULL, IRCPROTONAME, s.c_str(), dbv.ptszVal );
				DBFreeVariant( &dbv );
		}	}

		DBWriteContactSettingByte( NULL, IRCPROTONAME, "PerformConversionDone", 1 );
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
	UnhookEvent(g_hOptionsInit);
	Netlib_CloseHandle(hNetlib);
	Netlib_CloseHandle(hNetlibDCC);

	DestroyServiceFunction(hpsAddToList    );
	DestroyServiceFunction(hpsBasicSearch  );
	DestroyServiceFunction(hpsFileResume   );
	DestroyServiceFunction(hpsGetCaps	   );
	DestroyServiceFunction(hpsGetName	   );
	DestroyServiceFunction(hpsGetStatus	   );
	DestroyServiceFunction(hpsLoadIcon	   );
	DestroyServiceFunction(hpsSetAwayMsg   );
	DestroyServiceFunction(hpsSetStatus	   );

	DestroyServiceFunction(hpsRFile		   );
	DestroyServiceFunction(hpsRMessage	   );
	DestroyServiceFunction(hpsSFile		   );
	DestroyServiceFunction(hpsSFileAllow   );
	DestroyServiceFunction(hpsSFileCancel  );
	DestroyServiceFunction(hpsSFileDeny	   );
	DestroyServiceFunction(hpsSGetAwayMsg  );
	DestroyServiceFunction(hpsSMessage	   );

	DestroyServiceFunction(hpsJionChannel  );
	DestroyServiceFunction(hpsQuickConnect );
	DestroyServiceFunction(hpsChangeNick   );
	DestroyServiceFunction(hpsShowList	   );
	DestroyServiceFunction(hpsShowServer   );
	DestroyServiceFunction(hpsMenuShowChannel );
	DestroyServiceFunction(hpsMenuJoinLeave);
	DestroyServiceFunction(hpsMenuChanSettings );
	DestroyServiceFunction(hpsMenuWhois	   );
	DestroyServiceFunction(hpsMenuDisconnect );
	DestroyServiceFunction(hpsMenuIgnore   );

	DestroyServiceFunction(hpsEDblCkick	   );
	DestroyServiceFunction(hpsCInsertRawIn );
	DestroyServiceFunction(hpsCInsertRawOut);
	DestroyServiceFunction(hpsCInsertGuiIn );
	DestroyServiceFunction(hpsCInsertGuiOut);
	DestroyServiceFunction(hpsCGetIrcData  );
}

/////////////////////////////////////////////////////////////////////////////////////////
// Service function creation

static HANDLE CreateProtoService( const char* serviceName, MIRANDASERVICE pFunc )
{
	char temp[MAXMODULELABELLENGTH];
	mir_snprintf( temp, sizeof(temp), "%s%s", IRCPROTONAME, serviceName );
	return CreateServiceFunction( temp, pFunc );
}

void CreateServiceFunctions( void )
{
	hpsAddToList     = (HANDLE)CreateProtoService( PS_ADDTOLIST,     Service_AddToList );
	hpsBasicSearch   = (HANDLE)CreateProtoService( PS_BASICSEARCH,   Service_BasicSearch );
	hpsFileResume	 = (HANDLE)CreateProtoService( PS_FILERESUME,    Service_FileResume );
	hpsGetCaps		 = (HANDLE)CreateProtoService( PS_GETCAPS,       Service_GetCaps );
	hpsGetName		 = (HANDLE)CreateProtoService( PS_GETNAME,       Service_GetName );
	hpsGetStatus	 = (HANDLE)CreateProtoService( PS_GETSTATUS,     Service_GetStatus );
	hpsLoadIcon		 = (HANDLE)CreateProtoService( PS_LOADICON,      Service_LoadIcon );
	hpsSetAwayMsg	 = (HANDLE)CreateProtoService( PS_SETAWAYMSG,    Service_SetAwayMsg );
	hpsSetStatus	 = (HANDLE)CreateProtoService( PS_SETSTATUS,     Service_SetStatus );

	hpsRFile		 = (HANDLE)CreateProtoService( PSR_FILE,         Service_FileReceive);
	hpsRMessage		 = (HANDLE)CreateProtoService( PSR_MESSAGE,      Service_AddIncMessToDB );
	hpsSFile		 = (HANDLE)CreateProtoService( PSS_FILE,         Service_FileSend );
	hpsSFileAllow	 = (HANDLE)CreateProtoService( PSS_FILEALLOW,    Service_FileAllow );
	hpsSFileCancel	 = (HANDLE)CreateProtoService( PSS_FILECANCEL,   Service_FileCancel );
	hpsSFileDeny	 = (HANDLE)CreateProtoService( PSS_FILEDENY,     Service_FileDeny );
	hpsSGetAwayMsg	 = (HANDLE)CreateProtoService( PSS_GETAWAYMSG,   Service_GetAwayMessage );
	hpsSMessage		 = (HANDLE)CreateProtoService( PSS_MESSAGE,      Service_GetMessFromSRMM );

	hpsJionChannel	 = (HANDLE)CreateProtoService( IRC_JOINCHANNEL,  Service_JoinMenuCommand );
	hpsQuickConnect  = (HANDLE)CreateProtoService( IRC_QUICKCONNECT, Service_QuickConnectMenuCommand);
	hpsChangeNick	 = (HANDLE)CreateProtoService( IRC_CHANGENICK,   Service_ChangeNickMenuCommand );
	hpsShowList		 = (HANDLE)CreateProtoService( IRC_SHOWLIST,     Service_ShowListMenuCommand );
	hpsShowServer	 = (HANDLE)CreateProtoService( IRC_SHOWSERVER,   Service_ShowServerMenuCommand );
	hpsMenuShowChannel  = (HANDLE)CreateProtoService( IRC_UM_SHOWCHANNEL,  Service_MenuShowChannel );
	hpsMenuJoinLeave = (HANDLE)CreateProtoService( IRC_UM_JOINLEAVE, Service_MenuJoinLeave );
	hpsMenuChanSettings = (HANDLE)CreateProtoService( IRC_UM_CHANSETTINGS, Service_MenuChanSettings );
	hpsMenuWhois	 = (HANDLE)CreateProtoService( IRC_UM_WHOIS,     Service_MenuWhois );
	hpsMenuDisconnect= (HANDLE)CreateProtoService( IRC_UM_DISCONNECT,Service_MenuDisconnect );
	hpsMenuIgnore    = (HANDLE)CreateProtoService( IRC_UM_IGNORE,    Service_MenuIgnore );

	hpsEDblCkick	 = (HANDLE)CreateProtoService( "/DblClickEvent", Service_EventDoubleclicked );
	hpsCInsertRawIn  = (HANDLE)CreateProtoService( "/InsertRawIn",   Scripting_InsertRawIn );
	hpsCInsertRawOut = (HANDLE)CreateProtoService( "/InsertRawOut",  Scripting_InsertRawOut );
	hpsCInsertGuiIn  = (HANDLE)CreateProtoService( "/InsertGuiIn",   Scripting_InsertGuiIn );
	hpsCInsertGuiOut = (HANDLE)CreateProtoService( "/InsertGuiOut",  Scripting_InsertGuiOut);
	hpsCGetIrcData	 = (HANDLE)CreateProtoService( "/GetIrcData",    Scripting_GetIrcData);
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

		DoEvent(GC_EVENT_INFORMATION, SERVERWINDOW, NULL, szTemp, NULL, NULL, NULL, true, false);

		if (!bConnectThreadRunning)
			mir_forkthread(ConnectServerThread, NULL  );
		else
			bConnectRequested = true;

		RetryCount++;
	}
	else KillChatTimer(RetryTimer);
}
