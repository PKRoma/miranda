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
	pdci( _dci )
{
	SetControlHandler( IDOK, &CMessageBoxDlg::OnCommand_Yes );
}

void CMessageBoxDlg::OnInitDialog()
{
	SendDlgItemMessage( m_hwnd, IDC_LOGO, STM_SETICON, (LPARAM)(HICON)LoadImage(NULL,IDI_QUESTION,IMAGE_ICON,48, 48,LR_SHARED), 0);
}

void CMessageBoxDlg::OnCommand_Yes(HWND hwndCtrl, WORD idCtrl, WORD idCode)
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
	CProtoDlgBase<CIrcProto>( _pro, IDD_INFO, NULL )
{
	SetControlHandler( ID_INFO_GO, &CWhoisDlg::OnGo );
	SetControlHandler( ID_INFO_QUERY, &CWhoisDlg::OnQuery );
	SetControlHandler( IDC_PING, &CWhoisDlg::OnPing );
	SetControlHandler( IDC_USERINFO, &CWhoisDlg::OnUserInfo );
	SetControlHandler( IDC_TIME, &CWhoisDlg::OnTime );
	SetControlHandler( IDC_VERSION, &CWhoisDlg::OnVersion );
}

void CWhoisDlg::OnInitDialog()
{
	LOGFONT lf;
	HFONT hFont = ( HFONT )SendDlgItemMessage( m_hwnd, IDC_CAPTION, WM_GETFONT, 0, 0 );
	GetObject( hFont, sizeof( lf ), &lf );
	lf.lfHeight = (int)(lf.lfHeight*1.6);
	lf.lfWeight = FW_BOLD;
	hFont = CreateFontIndirect( &lf );
	SendDlgItemMessage( m_hwnd, IDC_CAPTION, WM_SETFONT, ( WPARAM )hFont, 0 );

	HFONT hFont2 = ( HFONT )SendDlgItemMessage( m_hwnd, IDC_AWAYTIME, WM_GETFONT, 0, 0 );
	GetObject( hFont2, sizeof( lf ), &lf );
	//lf.lfHeight = (int)(lf.lfHeight*0.6);
	lf.lfWeight = FW_BOLD;
	hFont2 = CreateFontIndirect( &lf );
	SendDlgItemMessage( m_hwnd, IDC_AWAYTIME, WM_SETFONT, ( WPARAM )hFont2, 0 );

	SendDlgItemMessage( m_hwnd, IDC_LOGO, STM_SETICON,(LPARAM)(HICON)m_proto->LoadIconEx(IDI_LOGO), 0);
	SendDlgItemMessage( m_hwnd, IDC_INFO_NAME, EM_SETREADONLY, true, 0);
	SendDlgItemMessage( m_hwnd, IDC_INFO_ID, EM_SETREADONLY, true, 0);
	SendDlgItemMessage( m_hwnd, IDC_INFO_ADDRESS, EM_SETREADONLY, true, 0);
	SendDlgItemMessage( m_hwnd, IDC_INFO_CHANNELS, EM_SETREADONLY, true, 0);
	SendDlgItemMessage( m_hwnd, IDC_INFO_AUTH, EM_SETREADONLY, true, 0);
	SendDlgItemMessage( m_hwnd, IDC_INFO_SERVER, EM_SETREADONLY, true, 0);
	SendDlgItemMessage( m_hwnd, IDC_INFO_AWAY2, EM_SETREADONLY, true, 0);
	SendDlgItemMessage( m_hwnd, IDC_INFO_OTHER, EM_SETREADONLY, true, 0);
	SendMessage( m_hwnd, WM_SETICON, ICON_BIG,(LPARAM)m_proto->LoadIconEx(IDI_WHOIS)); // Tell the dialog to use it
}

void CWhoisDlg::OnClose()
{
	ShowWindow( m_hwnd, SW_HIDE);
	SendMessage( m_hwnd, WM_SETREDRAW, FALSE, 0);
}

void CWhoisDlg::OnDestroy()
{
	HFONT hFont = (HFONT)SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_GETFONT,0,0);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( m_hwnd,IDOK,WM_GETFONT,0,0),0);
	DeleteObject(hFont);				
	HFONT hFont2=(HFONT)SendDlgItemMessage( m_hwnd,IDC_AWAYTIME,WM_GETFONT,0,0);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( m_hwnd,IDOK,WM_GETFONT,0,0),0);
	DeleteObject(hFont2);				
	m_proto->m_whoisDlg = NULL;
}

BOOL CWhoisDlg::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
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

void CWhoisDlg::OnGo(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	TCHAR szTemp[255];
	GetDlgItemText( m_hwnd, IDC_INFO_NICK, szTemp, SIZEOF(szTemp));
	m_proto->PostIrcMessage( _T("/WHOIS %s %s"), szTemp, szTemp);
}

void CWhoisDlg::OnQuery(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	TCHAR szTemp[255];
	GetDlgItemText( m_hwnd, IDC_INFO_NICK, szTemp, SIZEOF(szTemp));
	m_proto->PostIrcMessage( _T("/QUERY %s"), szTemp);
}

void CWhoisDlg::OnPing(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	TCHAR szTemp[255];
	GetDlgItemText( m_hwnd, IDC_INFO_NICK, szTemp, SIZEOF(szTemp));
	SetDlgItemText( m_hwnd, IDC_REPLY, TranslateT("Please wait..."));
	m_proto->PostIrcMessage( _T("/PRIVMSG %s \001PING %u\001"), szTemp, time(0));
}

void CWhoisDlg::OnUserInfo(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	TCHAR szTemp[255];
	GetDlgItemText( m_hwnd, IDC_INFO_NICK, szTemp, SIZEOF(szTemp));
	SetDlgItemText( m_hwnd, IDC_REPLY, TranslateT("Please wait..."));
	m_proto->PostIrcMessage( _T("/PRIVMSG %s \001USERINFO\001"), szTemp);
}

void CWhoisDlg::OnTime(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	TCHAR szTemp[255];
	GetDlgItemText( m_hwnd, IDC_INFO_NICK, szTemp, SIZEOF(szTemp));
	SetDlgItemText( m_hwnd, IDC_REPLY, TranslateT("Please wait..."));
	m_proto->PostIrcMessage( _T("/PRIVMSG %s \001TIME\001"), szTemp);
}

void CWhoisDlg::OnVersion(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	TCHAR szTemp[255];
	GetDlgItemText( m_hwnd, IDC_INFO_NICK, szTemp, SIZEOF(szTemp));
	SetDlgItemText( m_hwnd, IDC_REPLY, TranslateT("Please wait..."));
	m_proto->PostIrcMessage( _T("/PRIVMSG %s \001VERSION\001"), szTemp);
}

/////////////////////////////////////////////////////////////////////////////////////////
// 'Change nickname' dialog

CNickDlg::CNickDlg(CIrcProto *_pro) :
	CProtoDlgBase<CIrcProto>( _pro, IDD_NICK, NULL )
{
	SetControlHandler( IDOK, &CNickDlg::OnCommand_Yes );
}

void CNickDlg::OnInitDialog()
{
	HFONT hFont;
	LOGFONT lf;
	hFont=(HFONT)SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_GETFONT,0,0);
	GetObject(hFont,sizeof(lf),&lf);
	lf.lfHeight=(int)(lf.lfHeight*1.2);
	lf.lfWeight=FW_BOLD;
	hFont=CreateFontIndirect(&lf);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
	SendDlgItemMessage( m_hwnd, IDC_LOGO,STM_SETICON,(LPARAM)(HICON)m_proto->LoadIconEx(IDI_LOGO), 0);

	DBVARIANT dbv;
	if ( !DBGetContactSettingTString(NULL, m_proto->m_szModuleName, "RecentNicks", &dbv)) {
		for (int i = 0; i<10; i++)
			if ( !GetWord( dbv.ptszVal, i).empty())
				SendDlgItemMessage( m_hwnd, IDC_ENICK, CB_ADDSTRING, 0, (LPARAM)GetWord(dbv.ptszVal, i).c_str());

		DBFreeVariant(&dbv);
	}
}

void CNickDlg::OnDestroy()
{
	HFONT hFont = ( HFONT )SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_GETFONT,0,0);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( m_hwnd,IDOK,WM_GETFONT,0,0),0);
	DeleteObject(hFont);			
	m_proto->m_nickDlg = NULL;
}

BOOL CNickDlg::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	if ( msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORSTATIC ) {
		switch( GetDlgCtrlID(( HWND )lParam )) {
		case IDC_WHITERECT: case IDC_TEXT: case IDC_CAPTION: case IDC_LOGO:
			SetTextColor((HDC)wParam,RGB(0,0,0));
			SetBkColor((HDC)wParam,RGB(255,255,255));
			return (BOOL)GetStockObject(WHITE_BRUSH);
	}	}

	return CDlgBase::DlgProc(msg, wParam, lParam);
}

void CNickDlg::OnCommand_Yes(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	TCHAR szTemp[255];
	GetDlgItemText( m_hwnd, IDC_ENICK, szTemp, SIZEOF(szTemp));
	m_proto->PostIrcMessage( _T("/NICK %s"), szTemp);

	TString S = szTemp; 
	DBVARIANT dbv;
	if ( !DBGetContactSettingTString( NULL, m_proto->m_szModuleName, "RecentNicks", &dbv )) {
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
	CProtoDlgBase<CIrcProto>( _pro, IDD_LIST, NULL )
{
	SetControlHandler( IDC_JOIN, &CListDlg::OnJoin );
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
	
	SetWindowPos( m_hwnd, HWND_TOP, (screen.right-screen.left)/2- (m_proto->ListSize.x)/2,(screen.bottom-screen.top)/2- (m_proto->ListSize.y)/2, (m_proto->ListSize.x), (m_proto->ListSize.y), 0);
	SendMessage( m_hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(m_proto->ListSize.x, m_proto->ListSize.y));
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
			m_proto->ListSize.x = winRect.right-winRect.left;
			m_proto->ListSize.y = winRect.bottom-winRect.top;
			m_proto->setDword("SizeOfListBottom", m_proto->ListSize.y);
			m_proto->setDword("SizeOfListRight", m_proto->ListSize.x);
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

void CListDlg::OnJoin(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	TCHAR szTemp[255];
	int i = ListView_GetSelectionMark( GetDlgItem( m_hwnd, IDC_INFO_LISTVIEW));
	ListView_GetItemText( GetDlgItem( m_hwnd, IDC_INFO_LISTVIEW), i, 0, szTemp, 255);
	m_proto->PostIrcMessage( _T("/JOIN %s"), szTemp );
}

/////////////////////////////////////////////////////////////////////////////////////////
// 'Join' dialog

CJoinDlg::CJoinDlg(CIrcProto *_pro) :
	CProtoDlgBase<CIrcProto>( _pro, IDD_NICK, NULL )
{
	SetControlHandler( IDOK, &CJoinDlg::OnCommand_Yes );
}

void CJoinDlg::OnInitDialog()
{
	HFONT hFont=(HFONT)SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_GETFONT,0,0);
	LOGFONT lf;
	GetObject(hFont,sizeof(lf),&lf);
	lf.lfHeight=(int)(lf.lfHeight*1.2);
	lf.lfWeight=FW_BOLD;
	hFont=CreateFontIndirect(&lf);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
	SendDlgItemMessage( m_hwnd, IDC_LOGO,STM_SETICON,(LPARAM)(HICON)m_proto->LoadIconEx(IDI_LOGO), 0);

	DBVARIANT dbv;
	if ( !DBGetContactSettingTString( NULL, m_proto->m_szModuleName, "RecentChannels", &dbv)) {
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
	HFONT hFont = (HFONT)SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_GETFONT,0,0);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( m_hwnd,IDOK,WM_GETFONT,0,0),0);
	DeleteObject( hFont );				
	m_proto->m_joinDlg = NULL;
}

BOOL CJoinDlg::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	if ( msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORSTATIC ) {
		switch( GetDlgCtrlID(( HWND )lParam )) {
		case IDC_WHITERECT:  case IDC_TEXT:  case IDC_CAPTION:  case IDC_LOGO:
			SetTextColor((HDC)wParam,RGB(0,0,0));
			SetBkColor((HDC)wParam,RGB(255,255,255));
			return (BOOL)GetStockObject(WHITE_BRUSH);
	}	}

	return CDlgBase::DlgProc(msg, wParam, lParam);
}

void CJoinDlg::OnCommand_Yes(HWND hwndCtrl, WORD idCtrl, WORD idCode)
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
	if ( !DBGetContactSettingTString( NULL, m_proto->m_szModuleName, "RecentChannels", &dbv)) {
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
	CProtoDlgBase<CIrcProto>( _pro, IDD_INIT, NULL )
{
	SetControlHandler( IDOK, &CInitDlg::OnCommand_Yes );
}

void CInitDlg::OnInitDialog()
{
	HFONT hFont=(HFONT)SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_GETFONT,0,0);
	LOGFONT lf;
	GetObject(hFont,sizeof(lf),&lf);
	lf.lfHeight=(int)(lf.lfHeight*1.2);
	lf.lfWeight=FW_BOLD;
	hFont=CreateFontIndirect(&lf);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
	SendDlgItemMessage( m_hwnd, IDC_LOGO,STM_SETICON,(LPARAM)(HICON)m_proto->LoadIconEx(IDI_LOGO), 0);
}

void CInitDlg::OnDestroy()
{
	HFONT hFont = (HFONT)SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_GETFONT,0,0);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( m_hwnd,IDOK,WM_GETFONT,0,0),0);
	DeleteObject(hFont);				
}

BOOL CInitDlg::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	if ( msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORSTATIC ) {
		switch( GetDlgCtrlID(( HWND )lParam )) {
		case IDC_WHITERECT:  case IDC_TEXT:  case IDC_CAPTION:  case IDC_LOGO:
			SetTextColor((HDC)wParam,RGB(0,0,0));
			SetBkColor((HDC)wParam,RGB(255,255,255));
			return (BOOL)GetStockObject(WHITE_BRUSH);
	}	}

	return CDlgBase::DlgProc(msg, wParam, lParam);
}

void CInitDlg::OnCommand_Yes(HWND hwndCtrl, WORD idCtrl, WORD idCode)
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
		lstrcpyn( m_proto->Nick, l, 30 );
		lstrcpyn( m_proto->Name, m, 200 );
		if ( lstrlen( m_proto->AlternativeNick ) == 0) {
			TCHAR szTemp[30];
			mir_sntprintf(szTemp, SIZEOF(szTemp), _T("%s%u"), l, rand()%9999);
			m_proto->setTString("AlernativeNick", szTemp);
			lstrcpyn(m_proto->AlternativeNick, szTemp, 30);					
}	}	}

/////////////////////////////////////////////////////////////////////////////////////////
// 'Quick' dialog

CQuickDlg::CQuickDlg(CIrcProto *_pro) :
	CProtoDlgBase<CIrcProto>( _pro, IDD_QUICKCONN, NULL )
{
	SetControlHandler( IDOK, &CQuickDlg::OnCommand_Yes );
	SetControlHandler( IDC_SERVERCOMBO, &CQuickDlg::OnServerCombo );
}

void CQuickDlg::OnInitDialog()
{
	HFONT hFont;
	LOGFONT lf;
	hFont=(HFONT)SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_GETFONT,0,0);
	GetObject(hFont,sizeof(lf),&lf);
	lf.lfHeight=(int)(lf.lfHeight*1.2);
	lf.lfWeight=FW_BOLD;
	hFont=CreateFontIndirect(&lf);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
	SendDlgItemMessage( m_hwnd, IDC_LOGO,STM_SETICON,(LPARAM)(HICON)m_proto->LoadIconEx(IDI_LOGO), 0);
	
	char * p1 = m_proto->pszServerFile;
	char * p2 = m_proto->pszServerFile;
	if ( m_proto->pszServerFile ) {
		while (strchr(p2, 'n')) {
			SERVER_INFO* pData = new SERVER_INFO;
			p1 = strchr(p2, '=');
			++p1;
			p2 = strstr(p1, "SERVER:");
			pData->Name = ( char* )mir_alloc( p2-p1+1 );
			lstrcpynA( pData->Name, p1, p2-p1+1 );
			
			p1 = strchr(p2, ':');
			++p1;
			pData->iSSL = 0;
			if ( strstr(p1, "SSL") == p1 ) {
				p1 +=3;
				if(*p1 == '1')
					pData->iSSL = 1;
				else if(*p1 == '2')
					pData->iSSL = 2;
				p1++;
			}

			p2 = strchr(p1, ':');
			pData->Address = ( char* )mir_alloc( p2-p1+1 );
			lstrcpynA( pData->Address, p1, p2-p1+1 );
			
			p1 = p2;
			p1++;
			while (*p2 !='G' && *p2 != '-')
				p2++;
			pData->PortStart = ( char* )mir_alloc( p2-p1+1 );
			lstrcpynA( pData->PortStart, p1, p2-p1+1 );

			if (*p2 == 'G'){
				pData->PortEnd = ( char* )mir_alloc( p2-p1+1 );
				lstrcpyA(pData->PortEnd, pData->PortStart);
			} 
			else {
				p1 = p2;
				p1++;
				p2 = strchr(p1, 'G');
				pData->PortEnd = ( char* )mir_alloc( p2-p1+1 );
				lstrcpynA(pData->PortEnd, p1, p2-p1+1);
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
			int iItem = SendDlgItemMessageA( m_hwnd, IDC_SERVERCOMBO, CB_ADDSTRING,0,(LPARAM) pData->Name);
			SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_SETITEMDATA, iItem,(LPARAM) pData);
		}
	}
	else EnableWindow(GetDlgItem( m_hwnd, IDOK), false);
	
	SERVER_INFO* pData = new SERVER_INFO;
	pData->Group = mir_strdup( "" );
	pData->Name = mir_strdup( Translate("---- Not listed server ----"));

	DBVARIANT dbv;
	if ( !DBGetContactSettingString( NULL, m_proto->m_szModuleName, "ServerName", &dbv )) {
		pData->Address = mir_strdup( dbv.pszVal );
		DBFreeVariant(&dbv);
	}
	else pData->Address = mir_strdup( Translate("Type new server address here"));

	if ( !DBGetContactSettingString( NULL, m_proto->m_szModuleName, "PortStart", &dbv )) {
		pData->PortStart = mir_strdup( dbv.pszVal );
		DBFreeVariant(&dbv);
	}
	else pData->PortStart = mir_strdup( "6667" );

	if ( !DBGetContactSettingString( NULL, m_proto->m_szModuleName, "PortEnd", &dbv )) {
		pData->PortEnd = mir_strdup( dbv.pszVal );
		DBFreeVariant(&dbv);
	}
	else pData->PortEnd = mir_strdup( "6667" );

	pData->iSSL = DBGetContactSettingByte( NULL, m_proto->m_szModuleName, "UseSSL", 0 );
	
	int iItem = SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_ADDSTRING,0,(LPARAM)  mir_a2t(pData->Name));
	SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_SETITEMDATA, iItem,(LPARAM) pData);

	if ( m_proto->QuickComboSelection != -1 ) {
		SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_SETCURSEL, m_proto->QuickComboSelection,0);
		SendMessage( m_hwnd, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO, CBN_SELCHANGE), 0);
	}
	else EnableWindow(GetDlgItem( m_hwnd, IDOK), false);
}

void CQuickDlg::OnDestroy()
{
	HFONT hFont = ( HFONT )SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_GETFONT,0,0);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( m_hwnd,IDOK,WM_GETFONT,0,0),0);
	DeleteObject(hFont);				
	
	int j = (int) SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_GETCOUNT, 0, 0);
	for ( int index2 = 0; index2 < j; index2++ )
		delete ( SERVER_INFO* )SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_GETITEMDATA, index2, 0);
	
	m_proto->m_quickDlg = NULL;
}

BOOL CQuickDlg::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	if ( msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORSTATIC ) {
		switch( GetDlgCtrlID(( HWND )lParam )) {
		case IDC_WHITERECT:  case IDC_TEXT:  case IDC_CAPTION:  case IDC_LOGO:
			SetTextColor((HDC)wParam,RGB(0,0,0));
			SetBkColor((HDC)wParam,RGB(255,255,255));
			return (BOOL)GetStockObject(WHITE_BRUSH);
	}	}

	return CDlgBase::DlgProc(msg, wParam, lParam);
}

void CQuickDlg::OnCommand_Yes(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	GetDlgItemTextA( m_hwnd, IDC_SERVER, m_proto->ServerName, SIZEOF(m_proto->ServerName));				 
	GetDlgItemTextA( m_hwnd, IDC_PORT,   m_proto->PortStart,  SIZEOF(m_proto->PortStart));
	GetDlgItemTextA( m_hwnd, IDC_PORT2,  m_proto->PortEnd,    SIZEOF(m_proto->PortEnd));
	GetDlgItemTextA( m_hwnd, IDC_PASS,   m_proto->Password,   SIZEOF(m_proto->Password));
	
	int i = SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_GETCURSEL, 0, 0);
	SERVER_INFO* pData = ( SERVER_INFO* )SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_GETITEMDATA, i, 0);
	if ( pData && (int)pData != CB_ERR ) {
		lstrcpyA( m_proto->Network, pData->Group ); 
		if( m_ssleay32 ) {
			pData->iSSL = 0;
			if(IsDlgButtonChecked( m_hwnd, IDC_SSL_ON))
				pData->iSSL = 2;
			if(IsDlgButtonChecked( m_hwnd, IDC_SSL_AUTO))
				pData->iSSL = 1;
			m_proto->iSSL = pData->iSSL;
		}
		else m_proto->iSSL = 0;
	}
	
	TCHAR windowname[20];
	GetWindowText( m_hwnd, windowname, 20);
	if ( lstrcmpi(windowname, _T("Miranda IRC")) == 0 ) {
		m_proto->ServerComboSelection = SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_GETCURSEL, 0, 0);
		m_proto->setDword("ServerComboSelection",m_proto->ServerComboSelection);
		m_proto->setString("ServerName",m_proto->ServerName);
		m_proto->setString("PortStart",m_proto->PortStart);
		m_proto->setString("PortEnd",m_proto->PortEnd);
		CallService( MS_DB_CRYPT_ENCODESTRING, 499, (LPARAM)m_proto->Password);
		m_proto->setString("Password",m_proto->Password);
		CallService( MS_DB_CRYPT_DECODESTRING, 499, (LPARAM)m_proto->Password);
		m_proto->setString("Network",m_proto->Network);
		m_proto->setByte("UseSSL",m_proto->iSSL);
	}
	m_proto->QuickComboSelection = SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_GETCURSEL, 0, 0);
	m_proto->setDword("QuickComboSelection",m_proto->QuickComboSelection);
	m_proto->DisconnectFromServer();
	m_proto->ConnectToServer();
}

void CQuickDlg::OnServerCombo(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	if ( idCode == CBN_SELCHANGE ) {
		int i = SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_GETCURSEL, 0, 0);
		SERVER_INFO* pData = ( SERVER_INFO* )SendDlgItemMessage( m_hwnd, IDC_SERVERCOMBO, CB_GETITEMDATA, i, 0);
		if ( i != CB_ERR ) {
			SetDlgItemTextA( m_hwnd,IDC_SERVER, pData->Address );
			SetDlgItemTextA( m_hwnd,IDC_PORT,   pData->PortStart );
			SetDlgItemTextA( m_hwnd,IDC_PORT2,  pData->PortEnd );
			SetDlgItemTextA( m_hwnd,IDC_PASS,   "" );
			if ( m_ssleay32 ) {
				if ( pData->iSSL == 0 ) {
					CheckDlgButton( m_hwnd, IDC_SSL_OFF,  BST_CHECKED );
					CheckDlgButton( m_hwnd, IDC_SSL_AUTO, BST_UNCHECKED );
					CheckDlgButton( m_hwnd, IDC_SSL_ON,   BST_UNCHECKED );
				}
				if ( pData->iSSL == 1 ) {
					CheckDlgButton( m_hwnd, IDC_SSL_AUTO, BST_CHECKED );
					CheckDlgButton( m_hwnd, IDC_SSL_OFF,  BST_UNCHECKED );
					CheckDlgButton( m_hwnd, IDC_SSL_ON,   BST_UNCHECKED );
				}
				if ( pData->iSSL == 2 ) {
					CheckDlgButton( m_hwnd, IDC_SSL_ON,   BST_CHECKED );
					CheckDlgButton( m_hwnd, IDC_SSL_OFF,  BST_UNCHECKED );
					CheckDlgButton( m_hwnd, IDC_SSL_AUTO, BST_UNCHECKED );
			}	}

			if ( !strcmp( pData->Name, Translate("---- Not listed server ----" ))) {
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
}	}	}

/////////////////////////////////////////////////////////////////////////////////////////
// 'Question' dialog

CQuestionDlg::CQuestionDlg(CIrcProto *_pro, HWND parent ) :
	CProtoDlgBase<CIrcProto>( _pro, IDD_QUESTION, parent )
{
	SetControlHandler( IDOK, &CQuestionDlg::OnCommand_Yes );
}

void CQuestionDlg::OnInitDialog()
{
	HFONT hFont = (HFONT)SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_GETFONT,0,0);
	LOGFONT lf;
	GetObject(hFont,sizeof(lf),&lf);
	lf.lfHeight=(int)(lf.lfHeight*1.2);
	lf.lfWeight=FW_BOLD;
	hFont=CreateFontIndirect(&lf);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
	SendDlgItemMessage( m_hwnd, IDC_LOGO,STM_SETICON,(LPARAM)(HICON)m_proto->LoadIconEx(IDI_LOGO), 0);
}

void CQuestionDlg::OnClose()
{
	HWND hwnd = GetParent( m_hwnd);
	if ( hwnd )
		SendMessage(GetParent( m_hwnd), IRC_QUESTIONCLOSE, 0, 0);
	DestroyWindow( m_hwnd );
}

void CQuestionDlg::OnDestroy()
{
	HFONT hFont = (HFONT)SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_GETFONT,0,0);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( m_hwnd,IDOK,WM_GETFONT,0,0),0);
	DeleteObject(hFont);				
}

BOOL CQuestionDlg::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	if ( msg == IRC_ACTIVATE ) {
		ShowWindow( m_hwnd, SW_SHOW);
		SetActiveWindow( m_hwnd);
	}
	else if ( msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORSTATIC ) {
		switch( GetDlgCtrlID(( HWND )lParam )) {
		case IDC_WHITERECT:  case IDC_TEXT:  case IDC_CAPTION:  case IDC_LOGO:
			SetTextColor((HDC)wParam,RGB(0,0,0));
			SetBkColor((HDC)wParam,RGB(255,255,255));
			return (BOOL)GetStockObject(WHITE_BRUSH);
	}	}

	return CDlgBase::DlgProc(msg, wParam, lParam);
}

void CQuestionDlg::OnCommand_Yes(HWND hwndCtrl, WORD idCtrl, WORD idCode)
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

		HWND hwnd = GetParent( m_hwnd);
		if( hwnd )
			SendMessage(hwnd, IRC_QUESTIONAPPLY, 0, 0);
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// 'Channel Manager' dialog

CManagerDlg::CManagerDlg(CIrcProto *_pro) :
	CProtoDlgBase<CIrcProto>( _pro, IDD_CHANMANAGER, NULL )
{
	SetControlHandler( IDC_ADD, &CManagerDlg::OnAdd );
	SetControlHandler( IDC_APPLYTOPIC, &CManagerDlg::OnApplyTopic );
	SetControlHandler( IDC_CHECK1, &CManagerDlg::OnCheck );
	SetControlHandler( IDC_CHECK2, &CManagerDlg::OnCheck );
	SetControlHandler( IDC_CHECK3, &CManagerDlg::OnCheck );
	SetControlHandler( IDC_CHECK4, &CManagerDlg::OnCheck );
	SetControlHandler( IDC_CHECK5, &CManagerDlg::OnCheck5 );
	SetControlHandler( IDC_CHECK6, &CManagerDlg::OnCheck6 );
	SetControlHandler( IDC_CHECK7, &CManagerDlg::OnCheck );
	SetControlHandler( IDC_CHECK8, &CManagerDlg::OnCheck );
	SetControlHandler( IDC_CHECK9, &CManagerDlg::OnCheck );
	SetControlHandler( IDC_EDIT, &CManagerDlg::OnEdit );
	SetControlHandler( IDC_KEY, &CManagerDlg::OnChangeModes );
	SetControlHandler( IDC_LIMIT, &CManagerDlg::OnChangeModes );
	SetControlHandler( IDC_LIST, &CManagerDlg::OnList );
	SetControlHandler( IDC_RADIO1, &CManagerDlg::OnRadio );
	SetControlHandler( IDC_RADIO2, &CManagerDlg::OnRadio );
	SetControlHandler( IDC_RADIO3, &CManagerDlg::OnRadio );
	SetControlHandler( IDC_REMOVE, &CManagerDlg::OnRemove );
	SetControlHandler( IDC_TOPIC, &CManagerDlg::OnChangeTopic );
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
	HFONT hFont;
	LOGFONT lf;
	hFont=(HFONT)SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_GETFONT,0,0);
	GetObject(hFont,sizeof(lf),&lf);
	lf.lfHeight=(int)(lf.lfHeight*1.2);
	lf.lfWeight=FW_BOLD;
	hFont=CreateFontIndirect(&lf);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);

	POINT pt;
	pt.x = 3; 
	pt.y = 3; 
	HWND hwndEdit = ChildWindowFromPoint(GetDlgItem( m_hwnd, IDC_TOPIC), pt); 
	
	OldMgrEditProc = (WNDPROC)SetWindowLong(hwndEdit, GWL_WNDPROC,(LONG)MgrEditSubclassProc); 
	
	SendDlgItemMessage( m_hwnd,IDC_ADD,BM_SETIMAGE,IMAGE_ICON,(LPARAM)(HICON)m_proto->LoadIconEx(IDI_ADD));
	SendDlgItemMessage( m_hwnd,IDC_REMOVE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)m_proto->LoadIconEx(IDI_DELETE));
	SendDlgItemMessage( m_hwnd,IDC_EDIT,BM_SETIMAGE,IMAGE_ICON,(LPARAM)m_proto->LoadIconEx(IDI_RENAME));
	SendDlgItemMessage( m_hwnd,IDC_ADD, BUTTONADDTOOLTIP, (WPARAM)LPGEN("Add ban/invite/exception"), 0);
	SendDlgItemMessage( m_hwnd,IDC_EDIT, BUTTONADDTOOLTIP, (WPARAM)LPGEN("Edit selected ban/invite/exception"), 0);
	SendDlgItemMessage( m_hwnd,IDC_REMOVE, BUTTONADDTOOLTIP, (WPARAM)LPGEN("Delete selected ban/invite/exception"), 0);

	SendMessage( m_hwnd,WM_SETICON,ICON_BIG,(LPARAM)m_proto->LoadIconEx(IDI_MANAGER));
	SendDlgItemMessage( m_hwnd, IDC_LOGO,STM_SETICON,(LPARAM)(HICON)m_proto->LoadIconEx(IDI_LOGO), 0);
	SendDlgItemMessage( m_hwnd,IDC_APPLYTOPIC,BM_SETIMAGE,IMAGE_ICON,(LPARAM)m_proto->LoadIconEx( IDI_GO ));
	SendDlgItemMessage( m_hwnd,IDC_APPLYMODES,BM_SETIMAGE,IMAGE_ICON,(LPARAM)m_proto->LoadIconEx( IDI_GO ));
	SendDlgItemMessage( m_hwnd,IDC_APPLYTOPIC, BUTTONADDTOOLTIP, (WPARAM)LPGEN("Set this topic for the channel"), 0);
	SendDlgItemMessage( m_hwnd,IDC_APPLYMODES, BUTTONADDTOOLTIP, (WPARAM)LPGEN("Set these modes for the channel"), 0);

	SendDlgItemMessage( m_hwnd,IDC_LIST,LB_SETHORIZONTALEXTENT,750,NULL);
	CheckDlgButton( m_hwnd, IDC_RADIO1, BST_CHECKED);
	if ( !strchr(m_proto->sChannelModes.c_str(), 't'))
		EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK1), false);
	if ( !strchr(m_proto->sChannelModes.c_str(), 'n'))
		EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK2), false);
	if ( !strchr(m_proto->sChannelModes.c_str(), 'i'))
		EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK3), false);
	if ( !strchr(m_proto->sChannelModes.c_str(), 'm'))
		EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK4), false);
	if ( !strchr(m_proto->sChannelModes.c_str(), 'k'))
		EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK5), false);
	if ( !strchr(m_proto->sChannelModes.c_str(), 'l'))
		EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK6), false);
	if ( !strchr(m_proto->sChannelModes.c_str(), 'p'))
		EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK7), false);
	if ( !strchr(m_proto->sChannelModes.c_str(), 's'))
		EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK8), false);
	if ( !strchr(m_proto->sChannelModes.c_str(), 'c'))
		EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK9), false);
}

void CManagerDlg::OnClose()
{
//	if ( lParam != 3 ) {
	if ( IsWindowEnabled(GetDlgItem( m_hwnd, IDC_APPLYMODES)) || IsWindowEnabled(GetDlgItem( m_hwnd, IDC_APPLYTOPIC))) {
		int i = MessageBox( NULL, TranslateT("You have not applied all changes!\n\nApply before exiting?"), TranslateT("IRC warning"), MB_YESNOCANCEL|MB_ICONWARNING|MB_DEFBUTTON3);
		if ( i == IDCANCEL ) {
			m_lresult = TRUE;
			return;
		}

		if ( i == IDYES ) {
			if ( IsWindowEnabled( GetDlgItem( m_hwnd, IDC_APPLYMODES )))
				SendMessage( m_hwnd, WM_COMMAND, MAKEWPARAM( IDC_APPLYMODES, BN_CLICKED), 0);
			if ( IsWindowEnabled( GetDlgItem( m_hwnd, IDC_APPLYTOPIC )))
				SendMessage( m_hwnd, WM_COMMAND, MAKEWPARAM( IDC_APPLYTOPIC, BN_CLICKED), 0);
	}	}

	TCHAR window[256];
	GetDlgItemText( m_hwnd, IDC_CAPTION, window, 255 );
	TString S = _T("");
	TCHAR temp[1000];
	for ( int i = 0; i < 5; i++ ) {
		if ( SendDlgItemMessage( m_hwnd, IDC_TOPIC, CB_GETLBTEXT, i, (LPARAM)temp) != LB_ERR) {
			TString S1 = temp;
			ReplaceString( S1, _T(" "), _T("%¤"));
			S += _T(" ");
			S += S1;
	}	}

	if ( !S.empty() && m_proto->IsConnected() ) {
		mir_sntprintf( temp, SIZEOF(temp), _T("Topic%s%s"), window, m_proto->GetInfo().sNetwork.c_str());
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
	HFONT hFont = (HFONT)SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_GETFONT,0,0);
	SendDlgItemMessage( m_hwnd,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage( m_hwnd,IDOK,WM_GETFONT,0,0),0);
	DeleteObject(hFont);
	m_proto->m_managerDlg = NULL;
}

void CManagerDlg::OnRemove(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	int i = SendDlgItemMessage( m_hwnd, IDC_LIST, LB_GETCURSEL, 0, 0);
	if ( i != LB_ERR ) {
		EnableWindow(GetDlgItem( m_hwnd, IDC_REMOVE), false);
		EnableWindow(GetDlgItem( m_hwnd, IDC_EDIT), false);
		EnableWindow(GetDlgItem( m_hwnd, IDC_ADD), false);
		TCHAR* m = new TCHAR[ SendDlgItemMessage( m_hwnd, IDC_LIST, LB_GETTEXTLEN, i, 0)+2 ];
		SendDlgItemMessage( m_hwnd, IDC_LIST, LB_GETTEXT, i, (LPARAM)m);
		TString user = GetWord(m, 0);
		delete[]m;
		TCHAR temp[100];
		TCHAR mode[3];
		if ( IsDlgButtonChecked( m_hwnd, IDC_RADIO1 )) {
			lstrcpy(mode, _T("-b"));
			lstrcpyn(temp, TranslateT( "Remove ban?" ), 100 );
		}
		if ( IsDlgButtonChecked( m_hwnd, IDC_RADIO2 )) {
			lstrcpy(mode, _T("-I"));
			lstrcpyn(temp, TranslateT( "Remove invite?" ), 100 );
		}
		if ( IsDlgButtonChecked( m_hwnd, IDC_RADIO3 )) {
			lstrcpy(mode, _T("-e"));
			lstrcpyn(temp, TranslateT( "Remove exception?" ), 100 );
		}
		
		TCHAR window[256];
		GetDlgItemText( m_hwnd, IDC_CAPTION, window, SIZEOF(window));
		if ( MessageBox( m_hwnd, user.c_str(), temp, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 ) == IDYES ) {
			m_proto->PostIrcMessage( _T("/MODE %s %s %s"), window, mode, user.c_str());
			SendMessage( m_hwnd, IRC_QUESTIONAPPLY, 0, 0);
		}
		SendMessage( m_hwnd, IRC_QUESTIONCLOSE, 0, 0);
}	}

void CManagerDlg::OnAdd(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	TCHAR temp[100];
	TCHAR mode[3];
	if ( IsDlgButtonChecked( m_hwnd, IDC_RADIO1 )) {
		lstrcpy( mode, _T("+b"));
		lstrcpyn( temp, TranslateT("Add ban"), 100 );
	}
	if ( IsDlgButtonChecked( m_hwnd, IDC_RADIO2 )) {
		lstrcpy( mode, _T("+I"));
		lstrcpyn( temp, TranslateT("Add invite"), 100 );
	}
	if ( IsDlgButtonChecked( m_hwnd, IDC_RADIO3 )) {
		lstrcpy( mode, _T("+e"));
		lstrcpyn( temp, TranslateT("Add exception"), 100);
	}

	EnableWindow(GetDlgItem( m_hwnd, IDC_ADD), false);
	EnableWindow(GetDlgItem( m_hwnd, IDC_REMOVE), false);
	EnableWindow(GetDlgItem( m_hwnd, IDC_EDIT), false);

	CQuestionDlg* dlg = new CQuestionDlg( m_proto, m_hwnd );
	dlg->Show();
	HWND addban_hWnd = dlg->GetHwnd();
	SetDlgItemText(addban_hWnd, IDC_CAPTION, temp);
	SetWindowText(GetDlgItem(addban_hWnd, IDC_TEXT), TranslateT("Please enter the hostmask (nick!user@host)"));
	
	TCHAR temp2[450];
	TCHAR window[256];
	GetDlgItemText( m_hwnd, IDC_CAPTION, window, SIZEOF(window));
	mir_sntprintf(temp2, 450, _T("/MODE %s %s %s"), window, mode, _T("%question"));
	SetDlgItemText(addban_hWnd, IDC_HIDDENEDIT, temp2);
	PostMessage(addban_hWnd, IRC_ACTIVATE, 0, 0);
}

void CManagerDlg::OnEdit(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	if ( !IsDlgButtonChecked( m_hwnd, IDC_NOTOP )) {
		int i = SendDlgItemMessage( m_hwnd, IDC_LIST, LB_GETCURSEL, 0, 0);
		if ( i != LB_ERR ) {
			TCHAR* m = new TCHAR[ SendDlgItemMessage( m_hwnd, IDC_LIST, LB_GETTEXTLEN, i, 0)+2 ];
			SendDlgItemMessage( m_hwnd, IDC_LIST, LB_GETTEXT, i, (LPARAM)m );
			TString user = GetWord(m, 0);
			delete[]m;
			
			TCHAR temp[100];
			TCHAR mode[3];
			if ( IsDlgButtonChecked( m_hwnd, IDC_RADIO1 )) {
				lstrcpy( mode, _T("b"));
				lstrcpyn( temp, TranslateT("Edit ban"), 100 );
			}
			if ( IsDlgButtonChecked( m_hwnd, IDC_RADIO2 )) {
				lstrcpy( mode, _T("I"));
				lstrcpyn( temp, TranslateT("Edit invite?"), 100 );
			}
			if (IsDlgButtonChecked( m_hwnd, IDC_RADIO3 )) {
				lstrcpy( mode, _T("e"));
				lstrcpyn( temp, TranslateT("Edit exception?"), 100 );
			}
				
			CQuestionDlg* dlg = new CQuestionDlg( m_proto, m_hwnd );
			dlg->Show();
			HWND addban_hWnd = dlg->GetHwnd();
			EnableWindow(GetDlgItem( m_hwnd, IDC_ADD), false);
			EnableWindow(GetDlgItem( m_hwnd, IDC_REMOVE), false);
			EnableWindow(GetDlgItem( m_hwnd, IDC_EDIT), false);
			SetDlgItemText(addban_hWnd, IDC_CAPTION, temp);
			SetWindowText(GetDlgItem(addban_hWnd, IDC_TEXT), TranslateT("Please enter the hostmask (nick!user@host)"));
			SetWindowText(GetDlgItem(addban_hWnd, IDC_EDIT), user.c_str());

			TCHAR temp2[450];
			TCHAR window[256];
			GetDlgItemText( m_hwnd, IDC_CAPTION, window, SIZEOF(window));
			mir_sntprintf(temp2, 450, _T("/MODE %s -%s %s%s/MODE %s +%s %s"), window, mode, user.c_str(), _T("%newl"), window, mode, _T("%question"));
			SetDlgItemText(addban_hWnd, IDC_HIDDENEDIT, temp2);
			PostMessage(addban_hWnd, IRC_ACTIVATE, 0, 0);
}	}	}

void CManagerDlg::OnList(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	if ( idCode == LBN_SELCHANGE ) {
		if ( !IsDlgButtonChecked( m_hwnd, IDC_NOTOP )) {
			EnableWindow(GetDlgItem( m_hwnd, IDC_EDIT), true);
			EnableWindow(GetDlgItem( m_hwnd, IDC_REMOVE), true);
	}	}
	
	if ( idCode == LBN_DBLCLK )
		SendMessage( m_hwnd, WM_COMMAND, MAKEWPARAM(IDC_EDIT, BN_CLICKED), 0);
}

void CManagerDlg::OnChangeModes(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	if ( idCode == EN_CHANGE || idCode == CBN_EDITCHANGE || idCode == CBN_SELCHANGE )
		EnableWindow(GetDlgItem( m_hwnd, IDC_APPLYMODES), true);
}

void CManagerDlg::OnChangeTopic(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	if ( idCode == EN_CHANGE || idCode == CBN_EDITCHANGE || idCode == CBN_SELCHANGE )
		EnableWindow(GetDlgItem( m_hwnd, IDC_APPLYTOPIC), true);
}

void CManagerDlg::OnApplyModes(HWND hwndCtrl, WORD idCtrl, WORD idCode)
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
			if ( !IsDlgButtonChecked( m_hwnd, IDC_CHECK1 ))
				lstrcat( toremove, _T("t"));
		}
		else if ( IsDlgButtonChecked( m_hwnd, IDC_CHECK1 ))
			lstrcat( toadd, _T("t"));

		if ( wi->pszMode && _tcschr( wi->pszMode, 'n' )) {
			if( ! IsDlgButtonChecked( m_hwnd, IDC_CHECK2 ))
				lstrcat( toremove, _T("n"));
		}
		else if ( IsDlgButtonChecked( m_hwnd, IDC_CHECK2 ))
			lstrcat( toadd, _T("n"));

		if ( wi->pszMode && _tcschr( wi->pszMode, 'i' )) {
			if ( !IsDlgButtonChecked( m_hwnd, IDC_CHECK3 ))
				lstrcat( toremove, _T("i"));
		}
		else if ( IsDlgButtonChecked( m_hwnd, IDC_CHECK3 ))
			lstrcat( toadd, _T("i"));

		if ( wi->pszMode && _tcschr( wi->pszMode, 'm' )) {
			if( !IsDlgButtonChecked( m_hwnd, IDC_CHECK4 ))
				lstrcat( toremove, _T("m"));
		}
		else if ( IsDlgButtonChecked( m_hwnd, IDC_CHECK4 ))
			lstrcat( toadd, _T("m"));

		if ( wi->pszMode && _tcschr( wi->pszMode, 'p' )) {
			if ( !IsDlgButtonChecked( m_hwnd, IDC_CHECK7 ))
				lstrcat( toremove, _T("p"));
		}
		else if ( IsDlgButtonChecked( m_hwnd, IDC_CHECK7 ))
			lstrcat( toadd, _T("p"));

		if ( wi->pszMode && _tcschr( wi->pszMode, 's' )) {
			if ( !IsDlgButtonChecked( m_hwnd, IDC_CHECK8 ))
				lstrcat( toremove, _T("s"));
		}
		else if ( IsDlgButtonChecked( m_hwnd, IDC_CHECK8 ))
			lstrcat( toadd, _T("s"));

		if ( wi->pszMode && _tcschr( wi->pszMode, 'c' )) {
			if ( !IsDlgButtonChecked( m_hwnd, IDC_CHECK9 ))
				lstrcat( toremove, _T("c"));
		}
		else if ( IsDlgButtonChecked( m_hwnd, IDC_CHECK9 ))
			lstrcat( toadd, _T("c"));

		TString Key = _T("");
		TString Limit = _T("");
		if ( wi->pszMode && wi->pszPassword && _tcschr( wi->pszMode, 'k' )) {
			if ( !IsDlgButtonChecked( m_hwnd, IDC_CHECK5 )) {
				lstrcat( toremove, _T("k"));
				appendixremove += _T(" ");
				appendixremove += wi->pszPassword;
			}
			else if( GetWindowTextLength( GetDlgItem( m_hwnd, IDC_KEY ))) {
				TCHAR temp[400];
				GetDlgItemText( m_hwnd, IDC_KEY, temp, 14);

				if ( Key != temp ) {
					lstrcat( toremove, _T("k"));
					lstrcat( toadd, _T("k"));
					appendixadd += _T(" ");
					appendixadd += temp;
					appendixremove += _T(" ");
					appendixremove += wi->pszPassword;
			}	}
		}
		else if ( IsDlgButtonChecked( m_hwnd, IDC_CHECK5) && GetWindowTextLength( GetDlgItem( m_hwnd, IDC_KEY ))) {
			lstrcat( toadd, _T("k"));
			appendixadd += _T(" ");
			
			TCHAR temp[400];
			GetDlgItemText( m_hwnd, IDC_KEY, temp, SIZEOF(temp));
			appendixadd += temp;
		}

		if ( _tcschr( wi->pszMode, 'l' )) {
			if ( !IsDlgButtonChecked( m_hwnd, IDC_CHECK6 ))
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
		else if ( IsDlgButtonChecked( m_hwnd, IDC_CHECK6 ) && GetWindowTextLength( GetDlgItem( m_hwnd, IDC_LIMIT ))) {
			lstrcat( toadd, _T("l"));
			appendixadd += _T(" ");
			
			TCHAR temp[15];
			GetDlgItemText( m_hwnd, IDC_LIMIT, temp, SIZEOF(temp));
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
	EnableWindow(GetDlgItem( m_hwnd, IDC_APPLYMODES), false);				
}

void CManagerDlg::OnApplyTopic(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	TCHAR temp[470];
	TCHAR window[256];
	GetDlgItemText( m_hwnd, IDC_CAPTION, window, SIZEOF(window));
	GetDlgItemText( m_hwnd, IDC_TOPIC, temp, SIZEOF(temp));
	m_proto->PostIrcMessage( _T("/TOPIC %s %s"), window, temp);
	int i = SendDlgItemMessage( m_hwnd, IDC_TOPIC, CB_FINDSTRINGEXACT, -1, (LPARAM)temp);
	if ( i != LB_ERR )
		SendDlgItemMessage( m_hwnd, IDC_TOPIC, CB_DELETESTRING, i, 0);
	SendDlgItemMessage( m_hwnd, IDC_TOPIC, CB_INSERTSTRING, 0, (LPARAM)temp);
	SetDlgItemText( m_hwnd, IDC_TOPIC, temp);
	EnableWindow(GetDlgItem( m_hwnd, IDC_APPLYTOPIC), false);
}

void CManagerDlg::OnCheck5(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	EnableWindow(GetDlgItem( m_hwnd, IDC_KEY), IsDlgButtonChecked( m_hwnd, IDC_CHECK5) == BST_CHECKED);				
	EnableWindow(GetDlgItem( m_hwnd, IDC_APPLYMODES), true);
}

void CManagerDlg::OnCheck6(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	EnableWindow(GetDlgItem( m_hwnd, IDC_LIMIT), IsDlgButtonChecked( m_hwnd, IDC_CHECK6) == BST_CHECKED );				
	EnableWindow(GetDlgItem( m_hwnd, IDC_APPLYMODES), true);
}

void CManagerDlg::OnRadio(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	if ( idCode == BN_CLICKED )
		SendMessage( m_hwnd, IRC_QUESTIONAPPLY, 0, 0);
}

void CManagerDlg::OnCheck(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	if ( idCode == BN_CLICKED )
		EnableWindow(GetDlgItem( m_hwnd, IDC_APPLYMODES), true);				
}

BOOL CManagerDlg::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg ) {
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		switch( GetDlgCtrlID(( HWND )lParam )) {
		case IDC_WHITERECT: case IDC_TEXT: case IDC_CAPTION: case IDC_AWAYTIME: case IDC_LOGO:
			SetTextColor((HDC)wParam,RGB(0,0,0));
			SetBkColor((HDC)wParam,RGB(255,255,255));
			return (BOOL)GetStockObject(WHITE_BRUSH);
		}
		break;

	case IRC_QUESTIONCLOSE:
		EnableWindow(GetDlgItem( m_hwnd, IDC_ADD), true);
		if ( SendDlgItemMessage( m_hwnd, IDC_LIST, LB_GETCURSEL, 0, 0) != LB_ERR) {
			EnableWindow(GetDlgItem( m_hwnd, IDC_REMOVE), true);
			EnableWindow(GetDlgItem( m_hwnd, IDC_EDIT), true);
		}
		break;

	case IRC_QUESTIONAPPLY:
		{
			TCHAR window[256];
			GetDlgItemText( m_hwnd, IDC_CAPTION, window, 255);

			TCHAR mode[3];
			lstrcpy( mode, _T("+b"));
			if ( IsDlgButtonChecked( m_hwnd, IDC_RADIO2 ))
				lstrcpy( mode, _T("+I"));
			if ( IsDlgButtonChecked( m_hwnd, IDC_RADIO3 ))
				lstrcpy( mode, _T("+e"));
			SendDlgItemMessage( m_hwnd, IDC_LIST, LB_RESETCONTENT, 0, 0);
			EnableWindow(GetDlgItem( m_hwnd, IDC_RADIO1), false);
			EnableWindow(GetDlgItem( m_hwnd, IDC_RADIO2), false);
			EnableWindow(GetDlgItem( m_hwnd, IDC_RADIO3), false);
			EnableWindow(GetDlgItem( m_hwnd, IDC_ADD), false);
			EnableWindow(GetDlgItem( m_hwnd, IDC_EDIT), false);
			EnableWindow(GetDlgItem( m_hwnd, IDC_REMOVE), false);
			m_proto->PostIrcMessage( _T("%s %s %s"), _T("/MODE"), window, mode); //wrong overloaded operator if three args
		}
		break;		

	case IRC_INITMANAGER:
		{
			TCHAR* window = (TCHAR*) lParam;

			CHANNELINFO* wi = (CHANNELINFO *)m_proto->DoEvent(GC_EVENT_GETITEMDATA, window, NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
			if ( wi ) {
				if ( m_proto->IsConnected() ) {
					TCHAR temp[1000];
					mir_sntprintf(temp, SIZEOF(temp), _T("Topic%s%s"), window, m_proto->GetInfo().sNetwork.c_str());

					#if defined( _UNICODE )
						char* p = mir_t2a(temp);
					#else
						char* p = temp;
					#endif
					DBVARIANT dbv;
  					if ( !DBGetContactSettingTString(NULL, m_proto->m_szModuleName, p, &dbv )) {
						for ( int i = 0; i<5; i++ ) {
							if ( !GetWord(dbv.ptszVal, i).empty()) {
								TString S = GetWord(dbv.ptszVal, i);
								ReplaceString( S, _T("%¤"), _T(" "));
								SendDlgItemMessage( m_hwnd, IDC_TOPIC, CB_ADDSTRING, 0, (LPARAM)S.c_str());
						}	}
						DBFreeVariant(&dbv);
					}
					#if defined( _UNICODE )
						mir_free(p);
					#endif
				}

				if ( wi->pszTopic )
					SetDlgItemText( m_hwnd, IDC_TOPIC, wi->pszTopic);

				if ( !IsDlgButtonChecked( m_proto->m_managerDlg->GetHwnd(), IDC_NOTOP ))
					EnableWindow(GetDlgItem( m_hwnd, IDC_ADD), true);


				bool add = false;
				TCHAR* p1= wi->pszMode;
				if ( p1 ) {
					while ( *p1 != '\0' && *p1 != ' ' ) {
						if (*p1 == '+')
							add = true;
						if (*p1 == '-')
							add = false;
						if (*p1 == 't')
							CheckDlgButton( m_hwnd, IDC_CHECK1, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'n')
							CheckDlgButton( m_hwnd, IDC_CHECK2, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'i')
							CheckDlgButton( m_hwnd, IDC_CHECK3, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'm')
							CheckDlgButton( m_hwnd, IDC_CHECK4, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'p')
							CheckDlgButton( m_hwnd, IDC_CHECK7, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 's')
							CheckDlgButton( m_hwnd, IDC_CHECK8, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'c')
							CheckDlgButton( m_hwnd, IDC_CHECK9, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'k' && add) {
							CheckDlgButton( m_hwnd, IDC_CHECK5, add?(BST_CHECKED) : (BST_UNCHECKED));
							EnableWindow(GetDlgItem( m_hwnd, IDC_KEY), add?(true) : (false));
							if(wi->pszPassword)
								SetDlgItemText( m_hwnd, IDC_KEY, wi->pszPassword);
						}
						if (*p1 == 'l' && add) {
							CheckDlgButton( m_hwnd, IDC_CHECK6, add?(BST_CHECKED) : (BST_UNCHECKED));
							EnableWindow(GetDlgItem( m_hwnd, IDC_LIMIT), add?(true) : (false));
							if(wi->pszLimit)
								SetDlgItemText( m_hwnd, IDC_LIMIT, wi->pszLimit);
						}
						p1++;
						if (wParam == 0 ) {
							EnableWindow(GetDlgItem( m_hwnd, IDC_LIMIT),  (false));
							EnableWindow(GetDlgItem( m_hwnd, IDC_KEY),    (false));
							EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK1), (false));
							EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK2), (false));
							EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK3), (false));
							EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK4), (false));
							EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK5), (false));
							EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK6), (false));
							EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK7), (false));
							EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK8), (false));
							EnableWindow(GetDlgItem( m_hwnd, IDC_CHECK9), (false));
							EnableWindow(GetDlgItem( m_hwnd, IDC_ADD), (false));
							if(IsDlgButtonChecked( m_hwnd, IDC_CHECK1))
								EnableWindow(GetDlgItem( m_hwnd, IDC_TOPIC), (false));
							CheckDlgButton( m_hwnd, IDC_NOTOP, BST_CHECKED);
						}
						ShowWindow( m_hwnd, SW_SHOW);
			}	}	}

			if ( strchr( m_proto->sChannelModes.c_str(), 'b' )) {
				CheckDlgButton( m_hwnd, IDC_RADIO1, BST_CHECKED);
				m_proto->PostIrcMessage( _T("/MODE %s +b"), window);
		}	}
		break;		
	}
	return CDlgBase::DlgProc(msg, wParam, lParam);
}
