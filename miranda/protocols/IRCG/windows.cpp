/*
IRC plugin for Miranda IM

Copyright (C) 2003-05 Jurgen Persson
Copyright (C) 2007-08 George Hazan

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

static WNDPROC OldMgrEditProc;

/////////////////////////////////////////////////////////////////////////////////////////
// Message Box

CMessageBoxDlg::CMessageBoxDlg( CIrcProto* _pro, DCCINFO* _dci ) :
	CProtoDlgBase<CIrcProto>( _pro, IDD_MESSAGEBOX, NULL ),
	pdci( _dci ),
	m_Ok( this, IDOK )
{
	m_Ok.OnClick = Callback( this, &CMessageBoxDlg::OnOk );
}

void CMessageBoxDlg::OnInitDialog()
{
	SendDlgItemMessage( m_hwnd, IDC_LOGO, STM_SETICON, (LPARAM)(HICON)LoadImage(NULL,IDI_QUESTION,IMAGE_ICON,48, 48,LR_SHARED), 0);
}

void CMessageBoxDlg::OnOk( CCtrlButton* )
{
	CDccSession* dcc = new CDccSession(m_proto, pdci);

	CDccSession* olddcc = m_proto->FindDCCSession(pdci->hContact);
	if (olddcc)
		olddcc->Disconnect();
	m_proto->AddDCCSession(pdci->hContact, dcc);

	dcc->Connect();
}

/////////////////////////////////////////////////////////////////////////////////////////
// Whois dialog

CWhoisDlg::CWhoisDlg( CIrcProto* _pro ) :
	CCoolIrcDlg( _pro, IDD_INFO ),
	m_InfoNick( this, IDC_INFO_NICK ),
	m_Reply( this, IDC_REPLY ),
	m_Caption( this, IDC_CAPTION ),
	m_AwayTime( this, IDC_AWAYTIME ),
	m_InfoName( this, IDC_INFO_NAME ),
	m_InfoId( this, IDC_INFO_ID ),
	m_InfoAddress( this, IDC_INFO_ADDRESS ),
	m_InfoChannels( this, IDC_INFO_CHANNELS ),
	m_InfoAuth( this, IDC_INFO_AUTH ),
	m_InfoServer( this, IDC_INFO_SERVER ),
	m_InfoAway2( this, IDC_INFO_AWAY2 ),
	m_InfoOther( this, IDC_INFO_OTHER ),
	m_Ping( this, IDC_PING ),
	m_Version( this, IDC_VERSION ),
	m_Time( this, IDC_TIME ),
	m_userInfo( this, IDC_USERINFO ),
	m_Refresh( this, ID_INFO_GO ),
	m_Query( this, ID_INFO_QUERY )
{
	m_Ping.OnClick = Callback( this, &CWhoisDlg::OnPing );
	m_Version.OnClick = Callback( this, &CWhoisDlg::OnVersion );
	m_Time.OnClick = Callback( this, &CWhoisDlg::OnTime );
	m_userInfo.OnClick = Callback( this, &CWhoisDlg::OnUserInfo );
	m_Refresh.OnClick = Callback( this, &CWhoisDlg::OnGo );
	m_Query.OnClick = Callback( this, &CWhoisDlg::OnQuery );
}

void CWhoisDlg::OnInitDialog()
{
	LOGFONT lf;
	HFONT hFont = ( HFONT )m_AwayTime.SendMsg( WM_GETFONT, 0, 0 );
	GetObject( hFont, sizeof( lf ), &lf );
	lf.lfWeight = FW_BOLD;
	hFont = CreateFontIndirect( &lf );
	m_AwayTime.SendMsg( WM_SETFONT, ( WPARAM )hFont, 0 );

	CCoolIrcDlg::OnInitDialog();

	SendMessage( m_hwnd, WM_SETICON, ICON_BIG,(LPARAM)m_proto->LoadIconEx(IDI_WHOIS)); // Tell the dialog to use it
}

void CWhoisDlg::OnClose()
{
	ShowWindow( m_hwnd, SW_HIDE);
	SendMessage( m_hwnd, WM_SETREDRAW, FALSE, 0);
}

void CWhoisDlg::OnDestroy()
{
	CCoolIrcDlg::OnDestroy();

	HFONT hFont2=(HFONT)SendDlgItemMessage( m_hwnd,IDC_AWAYTIME,WM_GETFONT,0,0);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( m_hwnd,IDOK,WM_GETFONT,0,0),0);
	DeleteObject(hFont2);				

	m_proto->m_whoisDlg = NULL;
}

void __cdecl CWhoisDlg::OnGo( CCtrlButton* )
{
	TCHAR szTemp[255];
	m_InfoNick.GetText( szTemp, SIZEOF(szTemp));
	m_proto->PostIrcMessage( _T("/WHOIS %s %s"), szTemp, szTemp );
}

void __cdecl CWhoisDlg::OnQuery( CCtrlButton* )
{
	TCHAR szTemp[255];
	m_InfoNick.GetText( szTemp, SIZEOF(szTemp));
	m_proto->PostIrcMessage( _T("/QUERY %s"), szTemp );
}

void __cdecl CWhoisDlg::OnPing( CCtrlButton* )
{
	TCHAR szTemp[255];
	m_InfoNick.GetText( szTemp, SIZEOF(szTemp));
	m_Reply.SetText( TranslateT("Please wait..."));
	m_proto->PostIrcMessage( _T("/PRIVMSG %s \001PING %u\001"), szTemp, time(0));
}

void __cdecl CWhoisDlg::OnUserInfo( CCtrlButton* )
{
	TCHAR szTemp[255];
	m_InfoNick.GetText( szTemp, SIZEOF(szTemp));
	m_Reply.SetText( TranslateT("Please wait..."));
	m_proto->PostIrcMessage( _T("/PRIVMSG %s \001USERINFO\001"), szTemp);
}

void __cdecl CWhoisDlg::OnTime( CCtrlButton* )
{
	TCHAR szTemp[255];
	m_InfoNick.GetText( szTemp, SIZEOF(szTemp));
	m_Reply.SetText( TranslateT("Please wait..."));
	m_proto->PostIrcMessage( _T("/PRIVMSG %s \001TIME\001"), szTemp);
}

void __cdecl CWhoisDlg::OnVersion( CCtrlButton* )
{
	TCHAR szTemp[255];
	m_InfoNick.GetText( szTemp, SIZEOF(szTemp));
	m_Reply.SetText( TranslateT("Please wait..."));
	m_proto->PostIrcMessage( _T("/PRIVMSG %s \001VERSION\001"), szTemp);
}

void CWhoisDlg::ShowMessage( const CIrcMessage* pmsg )
{
	if ( m_InfoNick.SendMsg( CB_FINDSTRINGEXACT, -1, (LPARAM) pmsg->parameters[1].c_str()) == CB_ERR)	
		m_InfoNick.SendMsg( CB_ADDSTRING, 0, (LPARAM) pmsg->parameters[1].c_str());	
	int i = m_InfoNick.SendMsg( CB_FINDSTRINGEXACT, -1, (LPARAM) pmsg->parameters[1].c_str());
	m_InfoNick.SendMsg( CB_SETCURSEL, i, 0);
	m_Caption.SetText( pmsg->parameters[1].c_str());
	m_InfoName.SetText( pmsg->parameters[5].c_str());	
	m_InfoAddress.SetText( pmsg->parameters[3].c_str());
	m_InfoId.SetText( pmsg->parameters[2].c_str());
	m_InfoChannels.SetText( _T("") );
	m_InfoServer.SetText( _T("") );
	m_InfoAway2.SetText( _T("") );
	m_InfoAuth.SetText( _T("") );
	m_InfoOther.SetText( _T("") );
	m_Reply.SetText( _T("") );
	SetWindowText( m_hwnd, TranslateT("User information"));
	EnableWindow( GetDlgItem( m_hwnd, ID_INFO_QUERY), true );
	ShowWindow( m_hwnd, SW_SHOW);
	if ( IsIconic( m_hwnd ))
		ShowWindow( m_hwnd, SW_SHOWNORMAL );
	SendMessage( m_hwnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect( m_hwnd, NULL, TRUE);
}

void CWhoisDlg::ShowMessageNoUser( const CIrcMessage* pmsg )
{
	m_InfoNick.SetText( pmsg->parameters[2].c_str());
	m_InfoNick.SendMsg( CB_SETEDITSEL, 0,MAKELPARAM(0,-1));
	m_Caption.SetText( pmsg->parameters[2].c_str());	
	m_InfoName.SetText(  _T("") );
	m_InfoAddress.SetText(  _T("") );
	m_InfoId.SetText(  _T("") );
	m_InfoChannels.SetText( _T("") );
	m_InfoServer.SetText( _T("") );
	m_InfoAway2.SetText( _T("") );
	m_InfoAuth.SetText( _T("") );
	m_Reply.SetText( _T(""));
	EnableWindow(GetDlgItem(m_hwnd, ID_INFO_QUERY), false);
}

/////////////////////////////////////////////////////////////////////////////////////////
// 'Change nickname' dialog

CNickDlg::CNickDlg(CIrcProto *_pro) :
	CCoolIrcDlg( _pro, IDD_NICK ),
	m_Ok( this, IDOK ),
	m_Enick( this, IDC_ENICK )
{
	m_Ok.OnClick = Callback( this, &CNickDlg::OnOk );
}

void CNickDlg::OnInitDialog()
{
	CCoolIrcDlg::OnInitDialog();

	DBVARIANT dbv;
	if ( !m_proto->getTString( "RecentNicks", &dbv)) {
		for (int i = 0; i<10; i++)
			if ( !GetWord( dbv.ptszVal, i).empty())
				SendDlgItemMessage( m_hwnd, IDC_ENICK, CB_ADDSTRING, 0, (LPARAM)GetWord(dbv.ptszVal, i).c_str());

		DBFreeVariant(&dbv);
}	}

void CNickDlg::OnDestroy()
{
	CCoolIrcDlg::OnDestroy();
	m_proto->m_nickDlg = NULL;
}

void CNickDlg::OnOk( CCtrlButton* )
{
	TCHAR szTemp[255];
	m_Enick.GetText( szTemp, SIZEOF(szTemp));
	m_proto->PostIrcMessage( _T("/NICK %s"), szTemp);

	TString S = szTemp; 
	DBVARIANT dbv;
	if ( !m_proto->getTString( "RecentNicks", &dbv )) {
		for ( int i = 0; i<10; i++ ) {
			TString s = GetWord(dbv.ptszVal, i);
			if ( !s.empty() && s != szTemp) {
				S += _T(" ");
				S += s;
		}	}
		DBFreeVariant(&dbv);
	}
	m_proto->setTString( "RecentNicks", S.c_str());
}

/////////////////////////////////////////////////////////////////////////////////////////
// 'Change nickname' dialog

CListDlg::CListDlg(CIrcProto *_pro) :
	CProtoDlgBase<CIrcProto>( _pro, IDD_LIST, NULL ),
	m_Join( this, IDC_JOIN )
{
}

void CListDlg::OnInitDialog()
{
	RECT screen;

	SystemParametersInfo(SPI_GETWORKAREA, 0,  &screen, 0); 
	LVCOLUMN lvC;
	int COLUMNS_SIZES[4] ={200, 50,50,2000};
	TCHAR szBuffer[32];

	lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvC.fmt = LVCFMT_LEFT;
	for ( int index = 0;index < 4; index++ ) {
		lvC.iSubItem = index;
		lvC.cx = COLUMNS_SIZES[index];

		switch( index ) {
			case 0: lstrcpy( szBuffer, TranslateT("Channel")); break;
			case 1: lstrcpy( szBuffer, _T("#"));               break;
			case 2: lstrcpy( szBuffer, TranslateT("Mode"));    break;
			case 3: lstrcpy( szBuffer, TranslateT("Topic"));   break;
		}
		lvC.pszText = szBuffer;
		ListView_InsertColumn(GetDlgItem( m_hwnd, IDC_INFO_LISTVIEW),index,&lvC);
	}
	
	SetWindowPos( m_hwnd, HWND_TOP, (screen.right-screen.left)/2- (m_proto->m_listSize.x)/2,(screen.bottom-screen.top)/2- (m_proto->m_listSize.y)/2, (m_proto->m_listSize.x), (m_proto->m_listSize.y), 0);
	SendMessage( m_hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(m_proto->m_listSize.x, m_proto->m_listSize.y));
	ListView_SetExtendedListViewStyle(GetDlgItem( m_hwnd, IDC_INFO_LISTVIEW), LVS_EX_FULLROWSELECT);
	SendMessage( m_hwnd,WM_SETICON,ICON_BIG,(LPARAM)m_proto->LoadIconEx(IDI_LIST)); // Tell the dialog to use it
}

void CListDlg::OnDestroy()
{
	m_proto->m_listDlg = NULL;
}

struct ListViewSortParam
{
	HWND hwnd;
	int  iSubItem;
};

static int CALLBACK ListViewSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	ListViewSortParam* param = ( ListViewSortParam* )lParamSort;
	if ( !param->hwnd )
		return 0;

	TCHAR temp1[512];
	TCHAR temp2[512];
	LVITEM lvm;
	lvm.mask = LVIF_TEXT;
	lvm.iItem = lParam1;
	lvm.iSubItem = param->iSubItem;
	lvm.pszText = temp1;
	lvm.cchTextMax = 511;
	SendDlgItemMessage(param->hwnd, IDC_INFO_LISTVIEW, LVM_GETITEM, 0, (LPARAM)&lvm);
	lvm.iItem = lParam2;
	lvm.pszText = temp2;
	SendDlgItemMessage(param->hwnd, IDC_INFO_LISTVIEW, LVM_GETITEM, 0, (LPARAM)&lvm);
	if (param->iSubItem != 1){
		if (lstrlen(temp1) != 0 && lstrlen(temp2) !=0)
			return lstrcmpi(temp1, temp2);
		
		return ( *temp1 == 0 ) ? 1 : -1;
	}

	return ( StrToInt(temp1) < StrToInt(temp2)) ? 1 : -1;
}

BOOL CListDlg::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg ) {
	case IRC_UPDATELIST:
		{
			int j = ListView_GetItemCount(GetDlgItem( m_hwnd, IDC_INFO_LISTVIEW));
			if ( j > 0 ) {
				LVITEM lvm;
				lvm.mask= LVIF_PARAM;
				lvm.iSubItem = 0;
				for ( int i =0; i < j; i++ ) {
					lvm.iItem = i;
					lvm.lParam = i;
					ListView_SetItem(GetDlgItem( m_hwnd, IDC_INFO_LISTVIEW),&lvm);
		}	}	}
		break;

	case WM_SIZE:
		{
			RECT winRect;
			GetClientRect( m_hwnd, &winRect);
			RECT buttRect;
			GetWindowRect(GetDlgItem( m_hwnd, IDC_JOIN), &buttRect);
			SetWindowPos(GetDlgItem( m_hwnd, IDC_INFO_LISTVIEW), HWND_TOP, 4, 4, winRect.right-8, winRect.bottom-36, 0 );
			SetWindowPos(GetDlgItem( m_hwnd, IDC_CLOSE), HWND_TOP, winRect.right-84, winRect.bottom-28, buttRect.right- buttRect.left, buttRect.bottom- buttRect.top, 0 );
			SetWindowPos(GetDlgItem( m_hwnd, IDC_JOIN), HWND_TOP, winRect.right-174,  winRect.bottom-28, buttRect.right- buttRect.left, buttRect.bottom- buttRect.top, 0 );
			SetWindowPos(GetDlgItem( m_hwnd, IDC_TEXT), HWND_TOP, 4,  winRect.bottom-28, winRect.right-200, buttRect.bottom- buttRect.top, 0 );

			GetWindowRect( m_hwnd, &winRect);
			m_proto->m_listSize.x = winRect.right-winRect.left;
			m_proto->m_listSize.y = winRect.bottom-winRect.top;
			m_proto->setDword("SizeOfListBottom", m_proto->m_listSize.y);
			m_proto->setDword("SizeOfListRight", m_proto->m_listSize.x);
		}
		return 0;		

	case WM_NOTIFY :
		switch (((NMHDR*)lParam)->code) {
		case NM_DBLCLK:
			{
				TCHAR szTemp[255];
				int i = ListView_GetSelectionMark(GetDlgItem( m_hwnd, IDC_INFO_LISTVIEW));
				ListView_GetItemText(GetDlgItem( m_hwnd, IDC_INFO_LISTVIEW), i, 0, szTemp, SIZEOF(szTemp));
				m_proto->PostIrcMessage( _T("/JOIN %s"), szTemp);
			}
			break;
				
		case LVN_COLUMNCLICK:
			{
				LPNMLISTVIEW lv = (LPNMLISTVIEW)lParam;
				ListViewSortParam param = { m_proto->m_listDlg->GetHwnd(), lv->iSubItem };
				SendDlgItemMessage( m_hwnd, IDC_INFO_LISTVIEW, LVM_SORTITEMS, (WPARAM)&param, (LPARAM)ListViewSort);
				SendMessage(m_proto->m_listDlg->GetHwnd(), IRC_UPDATELIST, 0, 0);
			}
			break;
		}
		break;
	}
	return CDlgBase::DlgProc(msg, wParam, lParam);
}

void CListDlg::OnJoin( CCtrlButton* )
{
	TCHAR szTemp[255];
	int i = ListView_GetSelectionMark( GetDlgItem( m_hwnd, IDC_INFO_LISTVIEW));
	ListView_GetItemText( GetDlgItem( m_hwnd, IDC_INFO_LISTVIEW), i, 0, szTemp, 255);
	m_proto->PostIrcMessage( _T("/JOIN %s"), szTemp );
}

/////////////////////////////////////////////////////////////////////////////////////////
// 'Join' dialog

CJoinDlg::CJoinDlg(CIrcProto *_pro) :
	CCoolIrcDlg( _pro, IDD_NICK, NULL ),
	m_Ok( this, IDOK )
{
	m_Ok.OnClick = Callback( this, &CJoinDlg::OnOk );
}

void CJoinDlg::OnInitDialog()
{
	CCoolIrcDlg::OnInitDialog();

	DBVARIANT dbv;
	if ( !m_proto->getTString( "RecentChannels", &dbv)) {
		for ( int i = 0; i < 20; i++ ) {
			if ( !GetWord( dbv.ptszVal, i).empty()) {
				TString S = GetWord(dbv.ptszVal, i);
				ReplaceString( S, _T("%newl"), _T(" "));
				SendDlgItemMessage( m_hwnd, IDC_ENICK, CB_ADDSTRING, 0, (LPARAM)S.c_str());
		}	}
		DBFreeVariant(&dbv);
}	}

void CJoinDlg::OnDestroy()
{
	CCoolIrcDlg::OnDestroy();
	m_proto->m_joinDlg = NULL;
}

void CJoinDlg::OnOk( CCtrlButton* )
{
	TCHAR szTemp[255];
	GetDlgItemText( m_hwnd, IDC_ENICK, szTemp, SIZEOF(szTemp));
	if ( m_proto->IsChannel( szTemp ))
		m_proto->PostIrcMessage( _T("/JOIN %s"), szTemp );
	else
		m_proto->PostIrcMessage( _T("/JOIN #%s"), szTemp );

	TString S = szTemp;
	ReplaceString( S, _T(" "), _T("%newl"));
	TString SL = S;
	
	DBVARIANT dbv;
	if ( !m_proto->getTString( "RecentChannels", &dbv)) {
		for (int i = 0; i < 20; i++ ) {
			if ( !GetWord(dbv.ptszVal, i).empty() && GetWord(dbv.ptszVal, i) != SL) {
				S += _T(" ");
				S += GetWord(dbv.ptszVal, i);
		}	}
		DBFreeVariant(&dbv);
	}
	m_proto->setTString("RecentChannels", S.c_str());
}

/////////////////////////////////////////////////////////////////////////////////////////
// 'Init' dialog

CInitDlg::CInitDlg(CIrcProto *_pro) :
	CCoolIrcDlg( _pro, IDD_INIT ),
	m_Ok( this, IDOK )
{
	m_Ok.OnClick = Callback( this, &CInitDlg::OnOk );
}

void CInitDlg::OnOk( CCtrlButton* )
{
	int i = SendMessage( GetDlgItem( m_hwnd, IDC_EDIT), WM_GETTEXTLENGTH, 0, 0);
	int j = SendMessage( GetDlgItem( m_hwnd, IDC_EDIT2), WM_GETTEXTLENGTH, 0, 0);
	if (i >0 && j > 0) {
		TCHAR l[30], m[200];
		GetDlgItemText( m_hwnd, IDC_EDIT, l, SIZEOF(l));
		GetDlgItemText( m_hwnd, IDC_EDIT2, m, SIZEOF(m));

		m_proto->setTString("PNick", l);
		m_proto->setTString("Nick", l);
		m_proto->setTString("Name", m);
		lstrcpyn( m_proto->m_nick, l, 30 );
		lstrcpyn( m_proto->m_name, m, 200 );
		if ( lstrlen( m_proto->m_alternativeNick ) == 0) {
			TCHAR szTemp[30];
			mir_sntprintf(szTemp, SIZEOF(szTemp), _T("%s%u"), l, rand()%9999);
			m_proto->setTString("AlernativeNick", szTemp);
			lstrcpyn(m_proto->m_alternativeNick, szTemp, 30);					
}	}	}

/////////////////////////////////////////////////////////////////////////////////////////
// 'Quick' dialog

CQuickDlg::CQuickDlg(CIrcProto *_pro) :
	CCoolIrcDlg( _pro, IDD_QUICKCONN ),
	m_Ok( this, IDOK ),
	m_serverCombo( this, IDC_SERVERCOMBO )
{
	m_Ok.OnClick = Callback( this, &CQuickDlg::OnOk );
	m_serverCombo.OnChange = Callback( this, &CQuickDlg::OnServerCombo );
}

void CQuickDlg::OnInitDialog()
{
	CCoolIrcDlg::OnInitDialog();

	char * p1 = m_proto->m_pszServerFile;
	char * p2 = m_proto->m_pszServerFile;
	if ( m_proto->m_pszServerFile ) {
		while (strchr(p2, 'n')) {
			SERVER_INFO* pData = new SERVER_INFO;
			p1 = strchr(p2, '=');
			++p1;
			p2 = strstr(p1, "SERVER:");
			pData->m_name = ( char* )mir_alloc( p2-p1+1 );
			lstrcpynA( pData->m_name, p1, p2-p1+1 );
			
			p1 = strchr(p2, ':');
			++p1;
			pData->m_iSSL = 0;
			if ( strstr(p1, "SSL") == p1 ) {
				p1 +=3;
				if(*p1 == '1')
					pData->m_iSSL = 1;
				else if(*p1 == '2')
					pData->m_iSSL = 2;
				p1++;
			}

			p2 = strchr(p1, ':');
			pData->Address = ( char* )mir_alloc( p2-p1+1 );
			lstrcpynA( pData->Address, p1, p2-p1+1 );
			
			p1 = p2;
			p1++;
			while (*p2 !='G' && *p2 != '-')
				p2++;
			pData->m_portStart = ( char* )mir_alloc( p2-p1+1 );
			lstrcpynA( pData->m_portStart, p1, p2-p1+1 );

			if (*p2 == 'G'){
				pData->m_portEnd = ( char* )mir_alloc( p2-p1+1 );
				lstrcpyA(pData->m_portEnd, pData->m_portStart);
			} 
			else {
				p1 = p2;
				p1++;
				p2 = strchr(p1, 'G');
				pData->m_portEnd = ( char* )mir_alloc( p2-p1+1 );
				lstrcpynA(pData->m_portEnd, p1, p2-p1+1);
			}

			p1 = strchr(p2, ':');
			p1++;
			p2 = strchr(p1, '\r');
			if (!p2)
				p2 = strchr(p1, '\n');
			if (!p2)
				p2 = strchr(p1, '\0');
			pData->Group = ( char* )mir_alloc( p2-p1+1 );
			lstrcpynA(pData->Group, p1, p2-p1+1);
			m_serverCombo.AddStringA( pData->m_name, ( LPARAM )pData );
		}
	}
	else EnableWindow(GetDlgItem( m_hwnd, IDOK), false);
	
	SERVER_INFO* pData = new SERVER_INFO;
	pData->Group = mir_strdup( "" );
	pData->m_name = mir_strdup( Translate("---- Not listed server ----"));

	DBVARIANT dbv;
	if ( !m_proto->getString( "ServerName", &dbv )) {
		pData->Address = mir_strdup( dbv.pszVal );
		DBFreeVariant(&dbv);
	}
	else pData->Address = mir_strdup( Translate("Type new server address here"));

	if ( !m_proto->getString( "PortStart", &dbv )) {
		pData->m_portStart = mir_strdup( dbv.pszVal );
		DBFreeVariant(&dbv);
	}
	else pData->m_portStart = mir_strdup( "6667" );

	if ( !m_proto->getString( "PortEnd", &dbv )) {
		pData->m_portEnd = mir_strdup( dbv.pszVal );
		DBFreeVariant(&dbv);
	}
	else pData->m_portEnd = mir_strdup( "6667" );

	pData->m_iSSL = m_proto->getByte( "UseSSL", 0 );
	
	int iItem = m_serverCombo.AddStringA( pData->m_name, ( LPARAM )pData );

	if ( m_proto->m_quickComboSelection != -1 ) {
		m_serverCombo.SetCurSel( m_proto->m_quickComboSelection );
		OnServerCombo( NULL );
	}
	else EnableWindow(GetDlgItem( m_hwnd, IDOK), false);
}

void CQuickDlg::OnDestroy()
{
	CCoolIrcDlg::OnDestroy();
	
	int j = m_serverCombo.GetCount();
	for ( int index2 = 0; index2 < j; index2++ )
		delete ( SERVER_INFO* )m_serverCombo.GetItemData( index2 );
	
	m_proto->m_quickDlg = NULL;
}

void CQuickDlg::OnOk( CCtrlButton* )
{
	GetDlgItemTextA( m_hwnd, IDC_SERVER, m_proto->m_serverName, SIZEOF(m_proto->m_serverName));				 
	GetDlgItemTextA( m_hwnd, IDC_PORT,   m_proto->m_portStart,  SIZEOF(m_proto->m_portStart));
	GetDlgItemTextA( m_hwnd, IDC_PORT2,  m_proto->m_portEnd,    SIZEOF(m_proto->m_portEnd));
	GetDlgItemTextA( m_hwnd, IDC_PASS,   m_proto->m_password,   SIZEOF(m_proto->m_password));
	
	int i = m_serverCombo.GetCurSel();
	SERVER_INFO* pData = ( SERVER_INFO* )m_serverCombo.GetItemData( i );
	if ( pData && (int)pData != CB_ERR ) {
		lstrcpyA( m_proto->m_network, pData->Group ); 
		if( m_ssleay32 ) {
			pData->m_iSSL = 0;
			if(IsDlgButtonChecked( m_hwnd, IDC_SSL_ON))
				pData->m_iSSL = 2;
			if(IsDlgButtonChecked( m_hwnd, IDC_SSL_AUTO))
				pData->m_iSSL = 1;
			m_proto->m_iSSL = pData->m_iSSL;
		}
		else m_proto->m_iSSL = 0;
	}
	
	TCHAR windowname[20];
	GetWindowText( m_hwnd, windowname, 20);
	if ( lstrcmpi(windowname, _T("Miranda IRC")) == 0 ) {
		m_proto->m_serverComboSelection = m_serverCombo.GetCurSel();
		m_proto->setDword("ServerComboSelection",m_proto->m_serverComboSelection);
		m_proto->setString("ServerName",m_proto->m_serverName);
		m_proto->setString("PortStart",m_proto->m_portStart);
		m_proto->setString("PortEnd",m_proto->m_portEnd);
		CallService( MS_DB_CRYPT_ENCODESTRING, 499, (LPARAM)m_proto->m_password);
		m_proto->setString("Password",m_proto->m_password);
		CallService( MS_DB_CRYPT_DECODESTRING, 499, (LPARAM)m_proto->m_password);
		m_proto->setString("Network",m_proto->m_network);
		m_proto->setByte("UseSSL",m_proto->m_iSSL);
	}
	m_proto->m_quickComboSelection = m_serverCombo.GetCurSel();
	m_proto->setDword("QuickComboSelection",m_proto->m_quickComboSelection);
	m_proto->DisconnectFromServer();
	m_proto->ConnectToServer();
}

void CQuickDlg::OnServerCombo( CCtrlData* )
{
	int i = m_serverCombo.GetCurSel();
	SERVER_INFO* pData = ( SERVER_INFO* )m_serverCombo.GetItemData( i );
	if ( i == CB_ERR )
		return;

	SetDlgItemTextA( m_hwnd,IDC_SERVER, pData->Address );
	SetDlgItemTextA( m_hwnd,IDC_PORT,   pData->m_portStart );
	SetDlgItemTextA( m_hwnd,IDC_PORT2,  pData->m_portEnd );
	SetDlgItemTextA( m_hwnd,IDC_PASS,   "" );
	if ( m_ssleay32 ) {
		if ( pData->m_iSSL == 0 ) {
			CheckDlgButton( m_hwnd, IDC_SSL_OFF,  BST_CHECKED );
			CheckDlgButton( m_hwnd, IDC_SSL_AUTO, BST_UNCHECKED );
			CheckDlgButton( m_hwnd, IDC_SSL_ON,   BST_UNCHECKED );
		}
		if ( pData->m_iSSL == 1 ) {
			CheckDlgButton( m_hwnd, IDC_SSL_AUTO, BST_CHECKED );
			CheckDlgButton( m_hwnd, IDC_SSL_OFF,  BST_UNCHECKED );
			CheckDlgButton( m_hwnd, IDC_SSL_ON,   BST_UNCHECKED );
		}
		if ( pData->m_iSSL == 2 ) {
			CheckDlgButton( m_hwnd, IDC_SSL_ON,   BST_CHECKED );
			CheckDlgButton( m_hwnd, IDC_SSL_OFF,  BST_UNCHECKED );
			CheckDlgButton( m_hwnd, IDC_SSL_AUTO, BST_UNCHECKED );
	}	}

	if ( !strcmp( pData->m_name, Translate("---- Not listed server ----" ))) {
		SendDlgItemMessage( m_hwnd, IDC_SERVER, EM_SETREADONLY, false, 0);
		SendDlgItemMessage( m_hwnd, IDC_PORT,   EM_SETREADONLY, false, 0);
		SendDlgItemMessage( m_hwnd, IDC_PORT2,  EM_SETREADONLY, false, 0);
		EnableWindow(GetDlgItem( m_hwnd, IDC_SSL_OFF), TRUE);
		EnableWindow(GetDlgItem( m_hwnd, IDC_SSL_AUTO),TRUE);
		EnableWindow(GetDlgItem( m_hwnd, IDC_SSL_ON),  TRUE);
	}
	else {
		SendDlgItemMessage( m_hwnd, IDC_SERVER, EM_SETREADONLY, true, 0);
		SendDlgItemMessage( m_hwnd, IDC_PORT,   EM_SETREADONLY, true, 0);
		SendDlgItemMessage( m_hwnd, IDC_PORT2,  EM_SETREADONLY, true, 0);
		EnableWindow(GetDlgItem( m_hwnd, IDC_SSL_OFF), FALSE);
		EnableWindow(GetDlgItem( m_hwnd, IDC_SSL_AUTO),FALSE);
		EnableWindow(GetDlgItem( m_hwnd, IDC_SSL_ON),  FALSE);
	}

	EnableWindow(GetDlgItem( m_hwnd, IDOK), true);
}

/////////////////////////////////////////////////////////////////////////////////////////
// 'Question' dialog

CQuestionDlg::CQuestionDlg(CIrcProto *_pro, CManagerDlg* owner ) :
	CCoolIrcDlg( _pro, IDD_QUESTION, ( owner == NULL ) ? NULL : owner->GetHwnd()),
	m_Ok( this, IDOK ),
	m_owner( owner )
{
	m_Ok.OnClick = Callback( this, &CQuestionDlg::OnOk );
}

void CQuestionDlg::OnClose()
{
	if ( m_owner )
		m_owner->CloseQuestion();
}

void CQuestionDlg::OnOk( CCtrlButton* )
{
	int i = GetWindowTextLength( GetDlgItem( m_hwnd, IDC_EDIT ));
	if ( i > 0 ) {
		TCHAR* l = new TCHAR[ i+2 ];
		GetDlgItemText( m_hwnd, IDC_EDIT, l, i+1 );

		int j = GetWindowTextLength(GetDlgItem( m_hwnd, IDC_HIDDENEDIT));
		TCHAR* m = new TCHAR[ j+2 ];
		GetDlgItemText( m_hwnd, IDC_HIDDENEDIT, m, j+1 );

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
		
		TCHAR* n = ( TCHAR* )alloca( sizeof( TCHAR )*( j+2 ));
		GetDlgItemText( m_hwnd, IDC_HIDDENEDIT, n, j+1 );
		TString S( n );
		ReplaceString( S, text, l );
		m_proto->PostIrcMessageWnd( NULL, NULL, (TCHAR*)S.c_str());

		delete []m;
		delete []l;

		if ( m_owner )
			m_owner->ApplyQuestion();
}	}

void CQuestionDlg::Activate()
{
	ShowWindow( m_hwnd, SW_SHOW );
	SetActiveWindow( m_hwnd );
}

/////////////////////////////////////////////////////////////////////////////////////////
// 'Channel Manager' dialog

CManagerDlg::CManagerDlg(CIrcProto *_pro) :
	CCoolIrcDlg( _pro, IDD_CHANMANAGER ),
	m_list( this, IDC_LIST ),

	m_check1( this, IDC_CHECK1 ),
	m_check2( this, IDC_CHECK2 ),
	m_check3( this, IDC_CHECK3 ),
	m_check4( this, IDC_CHECK4 ),
	m_check5( this, IDC_CHECK5 ),
	m_check6( this, IDC_CHECK6 ),
	m_check7( this, IDC_CHECK7 ),
	m_check8( this, IDC_CHECK8 ),
	m_check9( this, IDC_CHECK9 ),

	m_key( this, IDC_KEY ),
	m_limit( this, IDC_LIMIT ),
	m_topic( this, IDC_TOPIC ),

	m_add( this, IDC_ADD, _pro->LoadIconEx(IDI_ADD), LPGEN("Add ban/invite/exception")),
	m_edit( this, IDC_EDIT, _pro->LoadIconEx(IDI_RENAME), LPGEN("Edit selected ban/invite/exception")),
	m_remove( this, IDC_REMOVE, _pro->LoadIconEx(IDI_DELETE), LPGEN("Delete selected ban/invite/exception")),
	m_applyModes( this, IDC_APPLYMODES, _pro->LoadIconEx( IDI_GO ), LPGEN("Set these modes for the channel")),
	m_applyTopic( this, IDC_APPLYTOPIC, _pro->LoadIconEx( IDI_GO ), LPGEN("Set this topic for the channel")),

	m_radio1( this, IDC_RADIO1 ),
	m_radio2( this, IDC_RADIO2 ),
	m_radio3( this, IDC_RADIO3 )
{
	m_add.OnClick = Callback( this, &CManagerDlg::OnAdd );
	m_edit.OnClick = Callback( this, &CManagerDlg::OnEdit );
	m_remove.OnClick = Callback( this, &CManagerDlg::OnRemove );

	m_applyModes.OnClick = Callback( this, &CManagerDlg::OnApplyModes );
	m_applyTopic.OnClick = Callback( this, &CManagerDlg::OnApplyTopic );

	m_check1.OnChange = Callback( this, &CManagerDlg::OnCheck );
	m_check2.OnChange = Callback( this, &CManagerDlg::OnCheck );
	m_check3.OnChange = Callback( this, &CManagerDlg::OnCheck );
	m_check4.OnChange = Callback( this, &CManagerDlg::OnCheck );
	m_check5.OnChange = Callback( this, &CManagerDlg::OnCheck5 );
	m_check6.OnChange = Callback( this, &CManagerDlg::OnCheck6 );
	m_check7.OnChange = Callback( this, &CManagerDlg::OnCheck );
	m_check8.OnChange = Callback( this, &CManagerDlg::OnCheck );
	m_check9.OnChange = Callback( this, &CManagerDlg::OnCheck );

	m_key.OnChange = Callback( this, &CManagerDlg::OnChangeModes );
	m_limit.OnChange = Callback( this, &CManagerDlg::OnChangeModes );
	m_topic.OnChange = Callback( this, &CManagerDlg::OnChangeTopic );

	m_radio1.OnChange = Callback( this, &CManagerDlg::OnRadio );
	m_radio2.OnChange = Callback( this, &CManagerDlg::OnRadio );
	m_radio3.OnChange = Callback( this, &CManagerDlg::OnRadio );

	m_list.OnDblClick = Callback( this, &CManagerDlg::OnListDblClick );
	m_list.OnChange = Callback( this, &CManagerDlg::OnChangeList );
}

LRESULT CALLBACK MgrEditSubclassProc(HWND m_hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
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
			SendMessage( m_hwnd, EM_REPLACESEL, false, (LPARAM) w);
			SendMessage( m_hwnd, EM_SCROLLCARET, 0, 0 );
			return 0;
		}
		break;
	} 

	return CallWindowProc(OldMgrEditProc, m_hwnd, msg, wParam, lParam); 
} 

void CManagerDlg::OnInitDialog()
{
	CCoolIrcDlg::OnInitDialog();

	POINT pt;
	pt.x = 3; 
	pt.y = 3; 
	HWND hwndEdit = ChildWindowFromPoint( m_topic.GetHwnd(), pt); 
	OldMgrEditProc = (WNDPROC)SetWindowLong(hwndEdit, GWL_WNDPROC,(LONG)MgrEditSubclassProc); 
	
	SendMessage( m_hwnd,WM_SETICON,ICON_BIG,(LPARAM)m_proto->LoadIconEx(IDI_MANAGER));

	m_list.SendMsg( LB_SETHORIZONTALEXTENT, 750, NULL );
	m_radio1.SetState( true );

	const char* modes = m_proto->sChannelModes.c_str();
	if ( !strchr( modes, 't')) m_check1.Disable();
	if ( !strchr( modes, 'n')) m_check2.Disable();
	if ( !strchr( modes, 'i')) m_check3.Disable();
	if ( !strchr( modes, 'm')) m_check4.Disable();
	if ( !strchr( modes, 'k')) m_check5.Disable();
	if ( !strchr( modes, 'l')) m_check6.Disable();
	if ( !strchr( modes, 'p')) m_check7.Disable();
	if ( !strchr( modes, 's')) m_check8.Disable();
	if ( !strchr( modes, 'c')) m_check9.Disable();
}

void CManagerDlg::OnClose()
{
	if ( m_applyModes.Enabled() || m_applyTopic.Enabled()) {
		int i = MessageBox( NULL, TranslateT("You have not applied all changes!\n\nApply before exiting?"), TranslateT("IRC warning"), MB_YESNOCANCEL|MB_ICONWARNING|MB_DEFBUTTON3);
		if ( i == IDCANCEL ) {
			m_lresult = TRUE;
			return;
		}

		if ( i == IDYES ) {
			if ( m_applyModes.Enabled())
				OnApplyModes( NULL );
			if ( m_applyTopic.Enabled())
				OnApplyTopic( NULL );
	}	}

	TCHAR window[256];
	GetDlgItemText( m_hwnd, IDC_CAPTION, window, 255 );
	TString S = _T("");
	TCHAR temp[1000];
	for ( int i = 0; i < 5; i++ ) {
		if ( m_topic.SendMsg( CB_GETLBTEXT, i, (LPARAM)temp) != LB_ERR) {
			TString S1 = temp;
			ReplaceString( S1, _T(" "), _T("%¤"));
			S += _T(" ");
			S += S1;
	}	}

	if ( !S.empty() && m_proto->IsConnected() ) {
		mir_sntprintf( temp, SIZEOF(temp), _T("Topic%s%s"), window, m_proto->m_info.sNetwork.c_str());
		#if defined( _UNICODE )
			char* p = mir_t2a(temp);
			m_proto->setTString(p, S.c_str());
			mir_free(p);
		#else
			m_proto->setString(temp, S.c_str());
		#endif
	}
	DestroyWindow( m_hwnd);
}

void CManagerDlg::OnDestroy()
{
	CCoolIrcDlg::OnDestroy();
	m_proto->m_managerDlg = NULL;
}

void __cdecl CManagerDlg::OnAdd( CCtrlButton* )
{
	TCHAR temp[100];
	TCHAR mode[3];
	if ( m_radio1.GetState()) {
		lstrcpy( mode, _T("+b"));
		lstrcpyn( temp, TranslateT("Add ban"), 100 );
	}
	if ( m_radio2.GetState()) {
		lstrcpy( mode, _T("+I"));
		lstrcpyn( temp, TranslateT("Add invite"), 100 );
	}
	if ( m_radio3.GetState()) {
		lstrcpy( mode, _T("+e"));
		lstrcpyn( temp, TranslateT("Add exception"), 100);
	}

	m_add.Disable();
	m_edit.Disable();
	m_remove.Disable();

	CQuestionDlg* dlg = new CQuestionDlg( m_proto, this );
	dlg->Show();
	HWND addban_hWnd = dlg->GetHwnd();
	SetDlgItemText(addban_hWnd, IDC_CAPTION, temp);
	SetWindowText(GetDlgItem(addban_hWnd, IDC_TEXT), TranslateT("Please enter the hostmask (nick!user@host)"));
	
	TCHAR temp2[450];
	TCHAR window[256];
	GetDlgItemText( m_hwnd, IDC_CAPTION, window, SIZEOF(window));
	mir_sntprintf(temp2, 450, _T("/MODE %s %s %s"), window, mode, _T("%question"));
	SetDlgItemText(addban_hWnd, IDC_HIDDENEDIT, temp2);
	dlg->Activate();
}

void __cdecl CManagerDlg::OnEdit( CCtrlButton* )
{
	if ( !IsDlgButtonChecked( m_hwnd, IDC_NOTOP )) {
		int i = m_list.GetCurSel();
		if ( i != LB_ERR ) {
			TCHAR* m = m_list.GetItemText( i );
			TString user = GetWord(m, 0);
			mir_free( m );
			
			TCHAR temp[100];
			TCHAR mode[3];
			if ( m_radio1.GetState()) {
				lstrcpy( mode, _T("b"));
				lstrcpyn( temp, TranslateT("Edit ban"), 100 );
			}
			if ( m_radio2.GetState()) {
				lstrcpy( mode, _T("I"));
				lstrcpyn( temp, TranslateT("Edit invite?"), 100 );
			}
			if ( m_radio3.GetState()) {
				lstrcpy( mode, _T("e"));
				lstrcpyn( temp, TranslateT("Edit exception?"), 100 );
			}
				
			CQuestionDlg* dlg = new CQuestionDlg( m_proto, this );
			dlg->Show();
			HWND addban_hWnd = dlg->GetHwnd();
			SetDlgItemText(addban_hWnd, IDC_CAPTION, temp);
			SetWindowText(GetDlgItem(addban_hWnd, IDC_TEXT), TranslateT("Please enter the hostmask (nick!user@host)"));
			SetWindowText(GetDlgItem(addban_hWnd, IDC_EDIT), user.c_str());

			m_add.Disable();
			m_edit.Disable();
			m_remove.Disable();

			TCHAR temp2[450];
			TCHAR window[256];
			GetDlgItemText( m_hwnd, IDC_CAPTION, window, SIZEOF(window));
			mir_sntprintf(temp2, 450, _T("/MODE %s -%s %s%s/MODE %s +%s %s"), window, mode, user.c_str(), _T("%newl"), window, mode, _T("%question"));
			SetDlgItemText(addban_hWnd, IDC_HIDDENEDIT, temp2);
			dlg->Activate();
}	}	}

void __cdecl CManagerDlg::OnRemove( CCtrlButton* )
{
	int i = m_list.GetCurSel();
	if ( i != LB_ERR ) {
		m_add.Disable();
		m_edit.Disable();
		m_remove.Disable();

		TCHAR temp[100], mode[3];
		TCHAR* m = m_list.GetItemText( i, temp, SIZEOF( temp ));
		TString user = GetWord(m, 0);
		
		if ( m_radio1.GetState()) {
			lstrcpy(mode, _T("-b"));
			lstrcpyn(temp, TranslateT( "Remove ban?" ), 100 );
		}
		if ( m_radio2.GetState()) {
			lstrcpy(mode, _T("-I"));
			lstrcpyn(temp, TranslateT( "Remove invite?" ), 100 );
		}
		if ( m_radio3.GetState()) {
			lstrcpy(mode, _T("-e"));
			lstrcpyn(temp, TranslateT( "Remove exception?" ), 100 );
		}
		
		TCHAR window[256];
		GetDlgItemText( m_hwnd, IDC_CAPTION, window, SIZEOF(window));
		if ( MessageBox( m_hwnd, user.c_str(), temp, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 ) == IDYES ) {
			m_proto->PostIrcMessage( _T("/MODE %s %s %s"), window, mode, user.c_str());
			ApplyQuestion();
		}
		CloseQuestion();
}	}

void CManagerDlg::OnListDblClick( CCtrlListBox* )
{
	OnEdit( NULL );
}

void CManagerDlg::OnChangeList( CCtrlData* )
{
	if ( !IsDlgButtonChecked( m_hwnd, IDC_NOTOP )) {
		m_edit.Enable();
		m_remove.Enable();
}	}

void CManagerDlg::OnChangeModes( CCtrlData* )
{
	m_applyModes.Enable();
}

void CManagerDlg::OnChangeTopic( CCtrlData* )
{
	m_applyTopic.Enable();
}

void CManagerDlg::OnApplyModes( CCtrlButton* )
{
	TCHAR window[256];
	GetDlgItemText( m_hwnd, IDC_CAPTION, window, SIZEOF(window));
	CHANNELINFO* wi = (CHANNELINFO *)m_proto->DoEvent(GC_EVENT_GETITEMDATA, window, NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
	if ( wi ) {
		TCHAR toadd[10]; *toadd = '\0';
		TCHAR toremove[10]; *toremove = '\0';
		TString appendixadd = _T("");
		TString appendixremove = _T("");
		if ( wi->pszMode && _tcschr( wi->pszMode, 't' )) {
			if ( !m_check1.GetState())
				lstrcat( toremove, _T("t"));
		}
		else if ( m_check1.GetState())
			lstrcat( toadd, _T("t"));

		if ( wi->pszMode && _tcschr( wi->pszMode, 'n' )) {
			if( !m_check2.GetState())
				lstrcat( toremove, _T("n"));
		}
		else if ( m_check2.GetState())
			lstrcat( toadd, _T("n"));

		if ( wi->pszMode && _tcschr( wi->pszMode, 'i' )) {
			if ( !m_check3.GetState())
				lstrcat( toremove, _T("i"));
		}
		else if ( m_check3.GetState())
			lstrcat( toadd, _T("i"));

		if ( wi->pszMode && _tcschr( wi->pszMode, 'm' )) {
			if( !m_check4.GetState())
				lstrcat( toremove, _T("m"));
		}
		else if ( m_check4.GetState())
			lstrcat( toadd, _T("m"));

		if ( wi->pszMode && _tcschr( wi->pszMode, 'p' )) {
			if ( !m_check7.GetState())
				lstrcat( toremove, _T("p"));
		}
		else if ( m_check7.GetState())
			lstrcat( toadd, _T("p"));

		if ( wi->pszMode && _tcschr( wi->pszMode, 's' )) {
			if ( !m_check8.GetState())
				lstrcat( toremove, _T("s"));
		}
		else if ( m_check8.GetState())
			lstrcat( toadd, _T("s"));

		if ( wi->pszMode && _tcschr( wi->pszMode, 'c' )) {
			if ( !m_check9.GetState())
				lstrcat( toremove, _T("c"));
		}
		else if ( m_check9.GetState())
			lstrcat( toadd, _T("c"));

		TString Key = _T("");
		TString Limit = _T("");
		if ( wi->pszMode && wi->pszPassword && _tcschr( wi->pszMode, 'k' )) {
			if ( !m_check5.GetState()) {
				lstrcat( toremove, _T("k"));
				appendixremove += _T(" ");
				appendixremove += wi->pszPassword;
			}
			else if( GetWindowTextLength( m_key.GetHwnd())) {
				TCHAR temp[400];
				m_key.GetText( temp, 14);

				if ( Key != temp ) {
					lstrcat( toremove, _T("k"));
					lstrcat( toadd, _T("k"));
					appendixadd += _T(" ");
					appendixadd += temp;
					appendixremove += _T(" ");
					appendixremove += wi->pszPassword;
			}	}
		}
		else if ( m_check5.GetState() && GetWindowTextLength( m_key.GetHwnd())) {
			lstrcat( toadd, _T("k"));
			appendixadd += _T(" ");
			
			TCHAR temp[400];
			m_key.GetText( temp, SIZEOF(temp));
			appendixadd += temp;
		}

		if ( _tcschr( wi->pszMode, 'l' )) {
			if ( !m_check6.GetState())
				lstrcat( toremove, _T("l"));
			else if ( GetWindowTextLength( GetDlgItem( m_hwnd, IDC_LIMIT ))) {
				TCHAR temp[15];
				GetDlgItemText( m_hwnd, IDC_LIMIT, temp, SIZEOF(temp));
				if ( wi->pszLimit && lstrcmpi( wi->pszLimit, temp )) {
					lstrcat( toadd, _T("l"));
					appendixadd += _T(" ");
					appendixadd += temp;
			}	}
		}
		else if ( m_check6.GetState() && GetWindowTextLength( m_limit.GetHwnd())) {
			lstrcat( toadd, _T("l"));
			appendixadd += _T(" ");
			
			TCHAR temp[15];
			m_limit.GetText( temp, SIZEOF(temp));
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
			m_proto->PostIrcMessage( temp);
	}	}

	m_applyModes.Disable();
}

void __cdecl CManagerDlg::OnApplyTopic( CCtrlButton* )
{
	TCHAR temp[470];
	TCHAR window[256];
	GetDlgItemText( m_hwnd, IDC_CAPTION, window, SIZEOF(window));
	m_topic.GetText( temp, SIZEOF(temp));
	m_proto->PostIrcMessage( _T("/TOPIC %s %s"), window, temp);
	int i = m_topic.SendMsg( CB_FINDSTRINGEXACT, -1, (LPARAM)temp);
	if ( i != LB_ERR )
		m_topic.SendMsg( CB_DELETESTRING, i, 0);
	m_topic.SendMsg( CB_INSERTSTRING, 0, (LPARAM)temp);
	m_topic.SetText( temp );
	m_applyTopic.Disable();
}

void __cdecl CManagerDlg::OnCheck( CCtrlData* )
{
	m_applyModes.Enable();
}

void __cdecl CManagerDlg::OnCheck5( CCtrlData* )
{
	m_key.Enable( m_check5.GetState());
	m_applyModes.Enable();
}

void __cdecl CManagerDlg::OnCheck6( CCtrlData* )
{
	m_limit.Enable( m_check6.GetState() );				
	m_applyModes.Enable();
}

void __cdecl CManagerDlg::OnRadio( CCtrlData* )
{
	ApplyQuestion();
}

void CManagerDlg::ApplyQuestion()
{
	TCHAR window[256];
	GetDlgItemText( m_hwnd, IDC_CAPTION, window, 255);

	TCHAR mode[3];
	lstrcpy( mode, _T("+b"));
	if ( m_radio2.GetState())
		lstrcpy( mode, _T("+I"));
	if ( m_radio3.GetState())
		lstrcpy( mode, _T("+e"));
	m_list.ResetContent();
	m_radio1.Disable();
	m_radio2.Disable();
	m_radio3.Disable();
	m_add.Disable();
	m_edit.Disable();
	m_remove.Disable();
	m_proto->PostIrcMessage( _T("%s %s %s"), _T("/MODE"), window, mode); //wrong overloaded operator if three args
}

void CManagerDlg::CloseQuestion()
{
	m_add.Enable();
	if ( m_list.GetCurSel() != LB_ERR) {
		m_edit.Enable();
		m_remove.Enable();
}	}

void CManagerDlg::InitManager( int mode, const TCHAR* window )
{
	SetWindowText( GetDlgItem( m_hwnd, IDC_CAPTION ), window );

	CHANNELINFO* wi = (CHANNELINFO *)m_proto->DoEvent(GC_EVENT_GETITEMDATA, window, NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
	if ( wi ) {
		if ( m_proto->IsConnected() ) {
			TCHAR temp[1000];
			mir_sntprintf(temp, SIZEOF(temp), _T("Topic%s%s"), window, m_proto->m_info.sNetwork.c_str());

			#if defined( _UNICODE )
				char* p = mir_t2a(temp);
			#else
				char* p = temp;
			#endif
			DBVARIANT dbv;
			if ( !m_proto->getTString( p, &dbv )) {
				for ( int i = 0; i<5; i++ ) {
					if ( !GetWord(dbv.ptszVal, i).empty()) {
						TString S = GetWord(dbv.ptszVal, i);
						ReplaceString( S, _T("%¤"), _T(" "));
						m_topic.SendMsg( CB_ADDSTRING, 0, (LPARAM)S.c_str());
				}	}
				DBFreeVariant(&dbv);
			}
			#if defined( _UNICODE )
				mir_free(p);
			#endif
		}

		if ( wi->pszTopic )
			m_topic.SetText( wi->pszTopic );

		if ( !IsDlgButtonChecked( m_proto->m_managerDlg->GetHwnd(), IDC_NOTOP ))
			m_add.Enable();

		bool add = false;
		TCHAR* p1= wi->pszMode;
		if ( p1 ) {
			while ( *p1 != '\0' && *p1 != ' ' ) {
				if (*p1 == '+')
					add = true;
				if (*p1 == '-')
					add = false;
				if (*p1 == 't')
					m_check1.SetState( add );
				if (*p1 == 'n')
					m_check2.SetState( add );
				if (*p1 == 'i')
					m_check3.SetState( add );
				if (*p1 == 'm')
					m_check4.SetState( add );
				if (*p1 == 'p')
					m_check7.SetState( add );
				if (*p1 == 's')
					m_check8.SetState( add );
				if (*p1 == 'c')
					m_check9.SetState( add );
				if (*p1 == 'k' && add) {
					m_check5.SetState( add );
					m_key.Enable( add );
					if ( wi->pszPassword )
						m_key.SetText( wi->pszPassword );
				}
				if (*p1 == 'l' && add) {
					m_check6.SetState( add );
					m_limit.Enable( add );
					if ( wi->pszLimit )
						m_limit.SetText( wi->pszLimit );
				}
				p1++;
				if ( mode == 0 ) {
					m_limit.Disable();
					m_key.Disable();
					m_check1.Disable();
					m_check2.Disable();
					m_check3.Disable();
					m_check4.Disable();
					m_check5.Disable();
					m_check6.Disable();
					m_check7.Disable();
					m_check8.Disable();
					m_check9.Disable();
					m_add.Disable();
					if ( m_check1.GetState())
						m_topic.Disable();
					CheckDlgButton( m_hwnd, IDC_NOTOP, BST_CHECKED);
				}
				ShowWindow( m_hwnd, SW_SHOW );
	}	}	}

	if ( strchr( m_proto->sChannelModes.c_str(), 'b' )) {
		m_radio1.SetState( true );
		m_proto->PostIrcMessage( _T("/MODE %s +b"), window);
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// 'cool' dialog

CCoolIrcDlg::CCoolIrcDlg( CIrcProto* _pro, int dlgId, HWND parent ) :
	CProtoDlgBase<CIrcProto>( _pro, dlgId, parent )
{
}

void CCoolIrcDlg::OnInitDialog()
{
	HFONT hFont;
	LOGFONT lf;
	hFont=(HFONT)SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_GETFONT,0,0);
	GetObject(hFont,sizeof(lf),&lf);
	lf.lfHeight=(int)(lf.lfHeight*1.2);
	lf.lfWeight=FW_BOLD;
	hFont=CreateFontIndirect(&lf);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);

	SendDlgItemMessage( m_hwnd, IDC_LOGO, STM_SETICON,(LPARAM)(HICON)m_proto->LoadIconEx(IDI_LOGO), 0);
}

void CCoolIrcDlg::OnDestroy()
{
	HFONT hFont = (HFONT)SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_GETFONT,0,0);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( m_hwnd,IDOK,WM_GETFONT,0,0),0);
	DeleteObject(hFont);
}

BOOL CCoolIrcDlg::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	if ( msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORSTATIC ) {
		switch( GetDlgCtrlID(( HWND )lParam )) {
		case IDC_WHITERECT: case IDC_TEXT: case IDC_CAPTION: case IDC_AWAYTIME: case IDC_LOGO:
			SetTextColor((HDC)wParam,RGB(0,0,0));
			SetBkColor((HDC)wParam,RGB(255,255,255));
			return (BOOL)GetStockObject(WHITE_BRUSH);
	}	}

	return CDlgBase::DlgProc(msg, wParam, lParam);
}
