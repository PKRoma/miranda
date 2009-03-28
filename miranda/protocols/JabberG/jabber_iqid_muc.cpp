/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-09  George Hazan

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_list.h"
#include "jabber_iq.h"
#include "jabber_caps.h"

void CJabberProto::SetMucConfig( HXML node, void *from )
{
	if ( m_ThreadInfo && from ) {
		XmlNodeIq iq( _T("set"), SerialNext(), ( TCHAR* )from );
		HXML query = iq << XQUERY( xmlnsOwner );
		xmlAddChild( query, node );
		m_ThreadInfo->send( iq );
}	}

void LaunchForm(HXML node);

void CJabberProto::OnIqResultGetMuc( HXML iqNode )
{
	HXML queryNode, xNode;
	const TCHAR *type, *from, *str;

	// RECVED: room config form
	// ACTION: show the form
	Log( "<iq/> iqIdGetMuc" );
	if (( type = xmlGetAttrValue( iqNode, _T("type"))) == NULL ) return;
	if (( from = xmlGetAttrValue( iqNode, _T("from"))) == NULL ) return;

	if ( !_tcscmp( type, _T("result"))) {
		if (( queryNode = xmlGetChild( iqNode , "query" )) != NULL ) {
			str = xmlGetAttrValue( queryNode, _T("xmlns"));
			if ( !lstrcmp( str, _T("http://jabber.org/protocol/muc#owner" ))) {
				if (( xNode = xmlGetChild( queryNode , "x" )) != NULL ) {
					str = xmlGetAttrValue( xNode, _T("xmlns"));
					if ( !lstrcmp( str, _T(JABBER_FEAT_DATA_FORMS)))
						//LaunchForm(xNode);
						FormCreateDialog( xNode, _T("Jabber Conference Room Configuration"), &CJabberProto::SetMucConfig, mir_tstrdup( from ));
}	}	}	}	}

static void sttFillJidList(HWND hwndDlg)
{
	JABBER_MUC_JIDLIST_INFO *jidListInfo;
	HXML iqNode, queryNode;
	const TCHAR* from, *jid, *reason, *nick;
	LVITEM lvi;
	HWND hwndList;
	int count, i;

	TCHAR *filter = NULL;
	if (GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_FILTER), GWLP_USERDATA))
	{
		int filterLength = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_FILTER)) + 1;
		filter = (TCHAR *)_alloca(filterLength * sizeof(TCHAR));
		GetDlgItemText(hwndDlg, IDC_FILTER, filter, filterLength);
	}

	jidListInfo = ( JABBER_MUC_JIDLIST_INFO * ) GetWindowLongPtr( hwndDlg, GWLP_USERDATA );
	if ( !jidListInfo )
		return;

	hwndList = GetDlgItem( hwndDlg, IDC_LIST );
	SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);

	count = ListView_GetItemCount( hwndList );
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;
	for ( i=0; i<count; i++ ) {
		lvi.iItem = i;
		if ( ListView_GetItem( hwndList, &lvi ) == TRUE ) {
			if ( lvi.lParam!=( LPARAM )( -1 ) && lvi.lParam!=( LPARAM )( NULL )) {
				mir_free(( void * ) lvi.lParam );
			}
		}
	}
	ListView_DeleteAllItems( hwndList );

	// Populate displayed list from iqNode
	if (( iqNode = jidListInfo->iqNode ) != NULL ) {
		if (( from = xmlGetAttrValue( iqNode, _T("from"))) != NULL ) {
			if (( queryNode = xmlGetChild( iqNode , "query" )) != NULL ) {
				lvi.mask = LVIF_TEXT | LVIF_PARAM;
				lvi.iSubItem = 0;
				lvi.iItem = 0;
				for ( i=0; ; i++ ) {
					HXML itemNode = xmlGetChild( queryNode ,i);
					if ( !itemNode )
						break;

					if (( jid = xmlGetAttrValue( itemNode, _T("jid"))) != NULL ) {
						lvi.pszText = ( TCHAR* )jid;
						if ( jidListInfo->type == MUC_BANLIST ) {										
							if (( reason = xmlGetText(xmlGetChild( itemNode , "reason" ))) != NULL ) {
								TCHAR jidreason[ 200 ];
								mir_sntprintf( jidreason, SIZEOF( jidreason ), _T("%s (%s)") , jid, reason );
								lvi.pszText = jidreason;
						}	}

						if ( jidListInfo->type == MUC_VOICELIST || jidListInfo->type == MUC_MODERATORLIST ) {										
							if (( nick = xmlGetAttrValue( itemNode, _T("nick"))) != NULL ) {
								TCHAR nickjid[ 200 ];
								mir_sntprintf( nickjid, SIZEOF( nickjid ), _T("%s (%s)") , nick, jid );
								lvi.pszText = nickjid;
						}	}

						if (filter && *filter && !JabberStrIStr(lvi.pszText, filter))
							continue;

						lvi.lParam = ( LPARAM )mir_tstrdup( jid );

						ListView_InsertItem( hwndList, &lvi );
						lvi.iItem++;
	}	}	}	}	}

	lvi.mask = LVIF_PARAM;
	lvi.lParam = ( LPARAM )( -1 );
	ListView_InsertItem( hwndList, &lvi );

	SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);
	RedrawWindow(hwndList, NULL, NULL, RDW_INVALIDATE);
}

static int sttJidListResizer(HWND, LPARAM, UTILRESIZECONTROL *urc)
{
	switch (urc->wId)
	{
	case IDC_LIST:
		return RD_ANCHORX_LEFT|RD_ANCHORY_TOP|RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;
	case IDC_FILTER:
		return RD_ANCHORX_LEFT|RD_ANCHORY_BOTTOM|RD_ANCHORX_WIDTH;
	case IDC_BTN_FILTERRESET:
	case IDC_BTN_FILTERAPPLY:
		return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
	}
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
}

static INT_PTR CALLBACK JabberMucJidListDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	JABBER_MUC_JIDLIST_INFO* dat = (JABBER_MUC_JIDLIST_INFO*)GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

	switch( msg ) {
	case WM_INITDIALOG:
		{
			LVCOLUMN lvc;
			RECT rc;
			HWND hwndList;

			TranslateDialogDefault( hwndDlg );

			hwndList = GetDlgItem( hwndDlg, IDC_LIST );
			ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
			GetClientRect( hwndList, &rc );
			//rc.right -= GetSystemMetrics( SM_CXVSCROLL );
			lvc.mask = LVCF_WIDTH;
			lvc.cx = rc.right - 20;
			ListView_InsertColumn( hwndList, 0, &lvc );
			lvc.cx = 20;
			ListView_InsertColumn( hwndList, 1, &lvc );
			SendMessage( hwndDlg, WM_JABBER_REFRESH, 0, lParam );
			dat = (JABBER_MUC_JIDLIST_INFO*)lParam;

			static struct
			{
				int idc;
				char *title;
				char *icon;
				bool push;
			} buttons[] =
			{
				{IDC_BTN_FILTERAPPLY,	"Apply filter",		"sd_filter_apply",	false},
				{IDC_BTN_FILTERRESET,	"Reset filter",		"sd_filter_reset",	false},
			};
			for (int i = 0; i < SIZEOF(buttons); ++i)
			{
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BM_SETIMAGE, IMAGE_ICON, (LPARAM)dat->ppro->LoadIconEx(buttons[i].icon));
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONSETASFLATBTN, 0, 0);
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONADDTOOLTIP, (WPARAM)buttons[i].title, 0);
				if (buttons[i].push)
					SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONSETASPUSHBTN, 0, 0);
			}

			Utils_RestoreWindowPosition(hwndDlg, NULL, dat->ppro->m_szModuleName, "jidListWnd_");
		}
		return TRUE;
	case WM_SIZE:
		{
			UTILRESIZEDIALOG urd = {0};
			urd.cbSize = sizeof(urd);
			urd.hInstance = hInst;
			urd.hwndDlg = hwndDlg;
			urd.lpTemplate = MAKEINTRESOURCEA(IDD_JIDLIST);
			urd.pfnResizer = sttJidListResizer;
			CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM)&urd);

			RECT listrc;
			LVCOLUMN lvc;
			HWND hwndList = GetDlgItem( hwndDlg, IDC_LIST );
			GetClientRect( hwndList, &listrc );
			lvc.mask = LVCF_WIDTH;
			//listrc.right -= GetSystemMetrics( SM_CXVSCROLL );
			lvc.cx = listrc.right - 20;
			SendMessage(hwndList, LVM_SETCOLUMN, 0, (LPARAM)&lvc);
			break;
		}
		break;

	case WM_JABBER_REFRESH:
		{
			// lParam is ( JABBER_MUC_JIDLIST_INFO * )
			HXML iqNode, queryNode;
			const TCHAR* from;
			TCHAR title[256];

			// Clear current GWL_USERDATA, if any
			if ( dat != NULL )
				delete dat;

			// Set new GWL_USERDATA
			dat = ( JABBER_MUC_JIDLIST_INFO * ) lParam;
			SetWindowLongPtr( hwndDlg, GWLP_USERDATA, ( LONG_PTR ) dat );

			// Populate displayed list from iqNode
			lstrcpyn( title, TranslateT( "JID List" ), SIZEOF( title ));
			if (( dat=( JABBER_MUC_JIDLIST_INFO * ) lParam ) != NULL ) {
				if (( iqNode = dat->iqNode ) != NULL ) {
					if (( from = xmlGetAttrValue( iqNode, _T("from"))) != NULL ) {
						dat->roomJid = mir_tstrdup( from );
						
						if (( queryNode = xmlGetChild( iqNode , "query" )) != NULL ) {
							TCHAR* localFrom = mir_tstrdup( from );
							mir_sntprintf( title, SIZEOF( title ), _T("%s, %d items (%s)"),
								( dat->type == MUC_VOICELIST ) ? TranslateT( "Voice List" ) :
								( dat->type == MUC_MEMBERLIST ) ? TranslateT( "Member List" ) :
								( dat->type == MUC_MODERATORLIST ) ? TranslateT( "Moderator List" ) :
								( dat->type == MUC_BANLIST ) ? TranslateT( "Ban List" ) :
								( dat->type == MUC_ADMINLIST ) ? TranslateT( "Admin List" ) :
								( dat->type == MUC_OWNERLIST ) ? TranslateT( "Owner List" ) :
								TranslateT( "JID List" ), xmlGetChildCount(queryNode), localFrom );
							mir_free( localFrom );
			}	}	}	}
			SetWindowText( hwndDlg, title );

			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_FILTER), GWLP_USERDATA, 0);
			sttFillJidList(hwndDlg);
		}
		break;
	case WM_NOTIFY:
		if (( ( LPNMHDR )lParam )->idFrom == IDC_LIST ) {
			switch (( ( LPNMHDR )lParam )->code ) {
			case NM_CUSTOMDRAW:
				{
					NMLVCUSTOMDRAW *nm = ( NMLVCUSTOMDRAW * ) lParam;

					switch ( nm->nmcd.dwDrawStage ) {
					case CDDS_PREPAINT:
					case CDDS_ITEMPREPAINT:
						SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, CDRF_NOTIFYSUBITEMDRAW );
						return TRUE;
					case CDDS_SUBITEM|CDDS_ITEMPREPAINT:
						{
							RECT rc;
							HICON hIcon;

							ListView_GetSubItemRect( nm->nmcd.hdr.hwndFrom, nm->nmcd.dwItemSpec, nm->iSubItem, LVIR_LABEL, &rc );
							if ( nm->iSubItem == 1 ) {
								if( nm->nmcd.lItemlParam == ( LPARAM )( -1 ))
									hIcon = ( HICON )LoadImage( hInst, MAKEINTRESOURCE( IDI_ADDCONTACT ), IMAGE_ICON, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );
								else
									hIcon = ( HICON )LoadImage( hInst, MAKEINTRESOURCE( IDI_DELETE ), IMAGE_ICON, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );
								DrawIconEx( nm->nmcd.hdc, ( rc.left+rc.right-GetSystemMetrics( SM_CXSMICON ))/2, ( rc.top+rc.bottom-GetSystemMetrics( SM_CYSMICON ))/2,hIcon, GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0, GetSysColorBrush(COLOR_WINDOW), DI_NORMAL );
								DestroyIcon( hIcon );
								SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, CDRF_SKIPDEFAULT );
								return TRUE;
				}	}	}	}
				break;
			case NM_CLICK:
				{
					NMLISTVIEW *nm = ( NMLISTVIEW * ) lParam;
					LVITEM lvi;
					LVHITTESTINFO hti;
					TCHAR text[128];

					if ( nm->iSubItem < 1 )
						break;

					hti.pt.x = ( short ) LOWORD( GetMessagePos());
					hti.pt.y = ( short ) HIWORD( GetMessagePos());
					ScreenToClient( nm->hdr.hwndFrom, &hti.pt );
					if ( ListView_SubItemHitTest( nm->hdr.hwndFrom, &hti ) == -1 )
						break;

					if ( hti.iSubItem != 1 )
						break;

					lvi.mask = LVIF_PARAM | LVIF_TEXT;
					lvi.iItem = hti.iItem;
					lvi.iSubItem = 0;
					lvi.pszText = text;
					lvi.cchTextMax = sizeof( text );
					ListView_GetItem( nm->hdr.hwndFrom, &lvi );
					if ( lvi.lParam == ( LPARAM )( -1 )) {
						TCHAR szBuffer[ 1024 ];
						_tcscpy( szBuffer, dat->type2str());
						if ( !dat->ppro->EnterString(szBuffer, SIZEOF(szBuffer), NULL, JES_COMBO, "gcAddNick_"))
							break;

						// Trim leading and trailing whitespaces
						TCHAR *p = szBuffer, *q;
						for ( p = szBuffer; *p!='\0' && isspace( BYTE( *p )); p++);
						for ( q = p; *q!='\0' && !isspace( BYTE( *q )); q++);
						if (*q != '\0') *q = '\0';
						if (*p == '\0')
							break;

						dat->ppro->AddMucListItem( dat, p );
					}
					else {
						//delete
						TCHAR msgText[128];

						mir_sntprintf( msgText, SIZEOF( msgText ), _T("%s %s?"), TranslateT( "Removing" ), text );
						if ( MessageBox( hwndDlg, msgText, dat->type2str(), MB_YESNO|MB_SETFOREGROUND ) == IDYES ) {
							dat->ppro->DeleteMucListItem( dat, ( TCHAR* )lvi.lParam );
							mir_free(( void * )lvi.lParam );
							ListView_DeleteItem( nm->hdr.hwndFrom, hti.iItem );
				}	}	}
				break;
			}
			break;
		}
		break;
	case WM_COMMAND:
		if ((LOWORD(wParam) == IDC_BTN_FILTERAPPLY) ||
			((LOWORD(wParam) == IDOK) && (GetFocus() == GetDlgItem(hwndDlg, IDC_FILTER))))
		{
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_FILTER), GWLP_USERDATA, 1);
			sttFillJidList(hwndDlg);
		} else
		if ((LOWORD(wParam) == IDC_BTN_FILTERRESET) ||
			((LOWORD(wParam) == IDCANCEL) && (GetFocus() == GetDlgItem(hwndDlg, IDC_FILTER))))
		{
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_FILTER), GWLP_USERDATA, 0);
			sttFillJidList(hwndDlg);
		}
		break;
/*	case WM_SETCURSOR:
		if ( LOWORD( LPARAM )!= HTCLIENT ) break;
		if ( GetForegroundWindow() == GetParent( hwndDlg )) {
			POINT pt;
			GetCursorPos( &pt );
			ScreenToClient( hwndDlg,&pt );
			SetFocus( ChildWindowFromPoint( hwndDlg,pt ));	  //ugly hack because listviews ignore their first click
		}
		break;
*/	case WM_CLOSE:
		{
			HWND hwndList;
			int count, i;
			LVITEM lvi;

			// Free lParam of the displayed list items
			hwndList = GetDlgItem( hwndDlg, IDC_LIST );
			count = ListView_GetItemCount( hwndList );
			lvi.mask = LVIF_PARAM;
			lvi.iSubItem = 0;
			for ( i=0; i<count; i++ ) {
				lvi.iItem = i;
				if ( ListView_GetItem( hwndList, &lvi ) == TRUE ) {
					if ( lvi.lParam!=( LPARAM )( -1 ) && lvi.lParam!=( LPARAM )( NULL )) {
						mir_free(( void * ) lvi.lParam );
					}
				}
			}
			ListView_DeleteAllItems( hwndList );

			CJabberProto* ppro = dat->ppro;
			switch ( dat->type ) {
			case MUC_VOICELIST:
				ppro->m_hwndMucVoiceList = NULL;
				break;
			case MUC_MEMBERLIST:
				ppro->m_hwndMucMemberList = NULL;
				break;
			case MUC_MODERATORLIST:
				ppro->m_hwndMucModeratorList = NULL;
				break;
			case MUC_BANLIST:
				ppro->m_hwndMucBanList = NULL;
				break;
			case MUC_ADMINLIST:
				ppro->m_hwndMucAdminList = NULL;
				break;
			case MUC_OWNERLIST:
				ppro->m_hwndMucOwnerList = NULL;
				break;
			}

			DestroyWindow( hwndDlg );
		}
		break;

	case WM_DESTROY:
		// Clear GWL_USERDATA
		if ( dat != NULL ) {
			Utils_SaveWindowPosition(hwndDlg, NULL, dat->ppro->m_szModuleName, "jidListWnd_");
			delete dat;
		}
		break;
	}
	return FALSE;
}

static void CALLBACK JabberMucJidListCreateDialogApcProc( JABBER_MUC_JIDLIST_INFO *jidListInfo )
{
	HXML iqNode, queryNode;
	const TCHAR* from;
	HWND *pHwndJidList;

	if ( jidListInfo == NULL )                                        return;
	if (( iqNode = jidListInfo->iqNode ) == NULL )                      return;
	if (( from = xmlGetAttrValue( iqNode, _T("from"))) == NULL )     return;
	if (( queryNode = xmlGetChild( iqNode , "query" )) == NULL )   return;

	CJabberProto* ppro = jidListInfo->ppro;
	switch ( jidListInfo->type ) {
	case MUC_VOICELIST:
		pHwndJidList = &ppro->m_hwndMucVoiceList;
		break;
	case MUC_MEMBERLIST:
		pHwndJidList = &ppro->m_hwndMucMemberList;
		break;
	case MUC_MODERATORLIST:
		pHwndJidList = &ppro->m_hwndMucModeratorList;
		break;
	case MUC_BANLIST:
		pHwndJidList = &ppro->m_hwndMucBanList;
		break;
	case MUC_ADMINLIST:
		pHwndJidList = &ppro->m_hwndMucAdminList;
		break;
	case MUC_OWNERLIST:
		pHwndJidList = &ppro->m_hwndMucOwnerList;
		break;
	default:
		mir_free( jidListInfo );
		return;
	}

	if ( *pHwndJidList!=NULL && IsWindow( *pHwndJidList )) {
		SetForegroundWindow( *pHwndJidList );
		SendMessage( *pHwndJidList, WM_JABBER_REFRESH, 0, ( LPARAM )jidListInfo );
	}
	else *pHwndJidList = CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_JIDLIST ), GetForegroundWindow(), JabberMucJidListDlgProc, ( LPARAM )jidListInfo );
}

void CJabberProto::OnIqResultMucGetJidList( HXML iqNode, JABBER_MUC_JIDLIST_TYPE listType )
{
	const TCHAR* type;
	JABBER_MUC_JIDLIST_INFO *jidListInfo;

	if (( type = xmlGetAttrValue( iqNode, _T("type"))) == NULL ) return;

	if ( !lstrcmp( type, _T("result" ))) {
		if (( jidListInfo = new JABBER_MUC_JIDLIST_INFO ) != NULL ) {
			jidListInfo->type = listType;
			jidListInfo->ppro = this;
			jidListInfo->roomJid = NULL;	// Set in the dialog procedure
			if (( jidListInfo->iqNode = xi.copyNode( iqNode )) != NULL ) {
				if ( GetCurrentThreadId() != jabberMainThreadId )
					QueueUserAPC(( PAPCFUNC )JabberMucJidListCreateDialogApcProc, hMainThread, ( DWORD )jidListInfo );
				else
					JabberMucJidListCreateDialogApcProc( jidListInfo );
			}
			else mir_free( jidListInfo );
}	}	}

void CJabberProto::OnIqResultMucGetVoiceList( HXML iqNode )
{
	Log( "<iq/> iqResultMucGetVoiceList" );
	OnIqResultMucGetJidList( iqNode, MUC_VOICELIST );
}

void CJabberProto::OnIqResultMucGetMemberList( HXML iqNode )
{
	Log( "<iq/> iqResultMucGetMemberList" );
	OnIqResultMucGetJidList( iqNode, MUC_MEMBERLIST );
}

void CJabberProto::OnIqResultMucGetModeratorList( HXML iqNode )
{
	Log( "<iq/> iqResultMucGetModeratorList" );
	OnIqResultMucGetJidList( iqNode, MUC_MODERATORLIST );
}

void CJabberProto::OnIqResultMucGetBanList( HXML iqNode )
{
	Log( "<iq/> iqResultMucGetBanList" );
	OnIqResultMucGetJidList( iqNode, MUC_BANLIST );
}

void CJabberProto::OnIqResultMucGetAdminList( HXML iqNode )
{
	Log( "<iq/> iqResultMucGetAdminList" );
	OnIqResultMucGetJidList( iqNode, MUC_ADMINLIST );
}

void CJabberProto::OnIqResultMucGetOwnerList( HXML iqNode )
{
	Log( "<iq/> iqResultMucGetOwnerList" );
	OnIqResultMucGetJidList( iqNode, MUC_OWNERLIST );
}

/////////////////////////////////////////////////////////////////////////////////////////

JABBER_MUC_JIDLIST_INFO::~JABBER_MUC_JIDLIST_INFO()
{
	xi.destroyNode( iqNode );
	mir_free( roomJid );
}

TCHAR* JABBER_MUC_JIDLIST_INFO::type2str() const
{
	switch( type ) {
		case MUC_VOICELIST:     return TranslateT( "Voice List" );
		case MUC_MEMBERLIST:    return TranslateT( "Member List" );
		case MUC_MODERATORLIST: return TranslateT( "Moderator List" );
		case MUC_BANLIST:       return TranslateT( "Ban List" );
		case MUC_ADMINLIST:     return TranslateT( "Admin List" );
		case MUC_OWNERLIST:     return TranslateT( "Owner List" );
	}

	return TranslateT( "JID List" );
}
