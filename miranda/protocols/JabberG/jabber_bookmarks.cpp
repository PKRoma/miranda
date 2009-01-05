/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2007  Michael Stepura, George Hazan

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
#include "jabber_iq.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Bookmarks editor window

struct JabberAddBookmarkDlgParam {
	CJabberProto* ppro;
	JABBER_LIST_ITEM* m_item;
};

static BOOL CALLBACK JabberAddBookmarkDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	JabberAddBookmarkDlgParam* param = (JabberAddBookmarkDlgParam*)GetWindowLong( hwndDlg, GWL_USERDATA );

	TCHAR text[128];
	JABBER_LIST_ITEM *item;

	switch ( msg ) {
	case WM_INITDIALOG:
		param = (JabberAddBookmarkDlgParam*)lParam;
		SetWindowLong( hwndDlg, GWL_USERDATA, lParam );

		param->ppro->m_hwndJabberAddBookmark = hwndDlg;
		TranslateDialogDefault( hwndDlg );
		if ( item = param->m_item ) {
			if ( !lstrcmp( item->type, _T("conference") )) {
				if (!_tcschr( item->jid, _T( '@' ))) {	  //no room name - consider it is transport
					SendDlgItemMessage(hwndDlg, IDC_AGENT_RADIO, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow( GetDlgItem( hwndDlg, IDC_NICK ), FALSE );
					EnableWindow( GetDlgItem( hwndDlg, IDC_PASSWORD ), FALSE );
				}
				else SendDlgItemMessage(hwndDlg, IDC_ROOM_RADIO, BM_SETCHECK, BST_CHECKED, 0);
			}
			else {
				SendDlgItemMessage(hwndDlg, IDC_URL_RADIO, BM_SETCHECK, BST_CHECKED, 0);
				EnableWindow( GetDlgItem( hwndDlg, IDC_NICK ), FALSE );
				EnableWindow( GetDlgItem( hwndDlg, IDC_PASSWORD ), FALSE );
				SendDlgItemMessage(hwndDlg, IDC_CHECK_BM_AUTOJOIN, BM_SETCHECK, BST_UNCHECKED, 0);
				EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_BM_AUTOJOIN), FALSE );
			}

			EnableWindow( GetDlgItem( hwndDlg, IDC_ROOM_RADIO), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_URL_RADIO), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_AGENT_RADIO), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_BM_AUTOJOIN), FALSE );

			if ( item->jid ) SetDlgItemText( hwndDlg, IDC_ROOM_JID, item->jid );
			if ( item->name ) SetDlgItemText( hwndDlg, IDC_NAME, item->name );
			if ( item->nick ) SetDlgItemText( hwndDlg, IDC_NICK, item->nick );
			if ( item->password ) SetDlgItemText( hwndDlg, IDC_PASSWORD, item->password );
			if ( item->bAutoJoin ) SendDlgItemMessage( hwndDlg, IDC_CHECK_BM_AUTOJOIN, BM_SETCHECK, BST_CHECKED, 0 );
			if ( SendDlgItemMessage(hwndDlg, IDC_ROOM_RADIO, BM_GETCHECK,0, 0) == BST_CHECKED )
				EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_BM_AUTOJOIN), TRUE );
		}
		else {
			EnableWindow( GetDlgItem( hwndDlg, IDOK ), FALSE );
			SendDlgItemMessage(hwndDlg, IDC_ROOM_RADIO, BM_SETCHECK, BST_CHECKED, 0);
		}
		return TRUE;

	case WM_COMMAND:
		switch ( HIWORD(wParam) ) {
			case BN_CLICKED:
				switch (LOWORD (wParam) ) {
					case IDC_ROOM_RADIO:
						EnableWindow( GetDlgItem( hwndDlg, IDC_NICK ), TRUE );
						EnableWindow( GetDlgItem( hwndDlg, IDC_PASSWORD ), TRUE );
						EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_BM_AUTOJOIN), TRUE );
						break;
					case IDC_AGENT_RADIO:
					case IDC_URL_RADIO:
						EnableWindow( GetDlgItem( hwndDlg, IDC_NICK ), FALSE );
						EnableWindow( GetDlgItem( hwndDlg, IDC_PASSWORD ), FALSE );
						SendDlgItemMessage(hwndDlg, IDC_CHECK_BM_AUTOJOIN, BM_SETCHECK, BST_UNCHECKED, 0);
						EnableWindow( GetDlgItem( hwndDlg, IDC_CHECK_BM_AUTOJOIN), FALSE );
						break;
				}
		}

		switch ( LOWORD( wParam )) {
		case IDC_ROOM_JID:
			if (( HWND )lParam==GetFocus() && HIWORD( wParam )==EN_CHANGE )
				EnableWindow( GetDlgItem( hwndDlg, IDOK ), GetDlgItemText( hwndDlg, IDC_ROOM_JID, text, SIZEOF( text )));
			break;

		case IDOK:
			{
				GetDlgItemText( hwndDlg, IDC_ROOM_JID, text, SIZEOF( text ));
				TCHAR* roomJID = NEWTSTR_ALLOCA( text );

				if ( param->m_item )
					param->ppro->ListRemove( LIST_BOOKMARK, param->m_item->jid );

				item = param->ppro->ListAdd( LIST_BOOKMARK, roomJID );
				item->bUseResource = TRUE;

				if ( SendDlgItemMessage(hwndDlg, IDC_URL_RADIO, BM_GETCHECK,0, 0) == BST_CHECKED )
					replaceStr( item->type, _T( "url" ));
				else
					replaceStr( item->type, _T( "conference" ));

				GetDlgItemText( hwndDlg, IDC_NICK, text, SIZEOF( text ));
				replaceStr( item->nick, text );

				GetDlgItemText( hwndDlg, IDC_PASSWORD, text, SIZEOF( text ));
				replaceStr( item->password, text );

				GetDlgItemText( hwndDlg, IDC_NAME, text, SIZEOF( text ));
				replaceStr( item->name, ( text[0] == 0 ) ? roomJID : text );

				item->bAutoJoin = (SendDlgItemMessage(hwndDlg, IDC_CHECK_BM_AUTOJOIN, BM_GETCHECK,0, 0) == BST_CHECKED );
				{
					int iqId = param->ppro->SerialNext();
					param->ppro->IqAdd( iqId, IQ_PROC_SETBOOKMARKS, &CJabberProto::OnIqResultSetBookmarks);

					XmlNodeIq iq( _T("set"), iqId);
					param->ppro->SetBookmarkRequest(iq);
					param->ppro->m_ThreadInfo->send( iq );
				}
			}
			// fall through
		case IDCANCEL:
			EndDialog( hwndDlg, 0 );
			break;
		}
		break;

	case WM_JABBER_CHECK_ONLINE:
		if ( !param->ppro->m_bJabberOnline )
			EndDialog( hwndDlg, 0 );
		break;

	case WM_DESTROY:
		param->ppro->m_hwndJabberAddBookmark = NULL;
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Bookmarks manager window

class CJabberDlgBookmarks : public CJabberDlgFancy
{
	typedef CJabberDlgFancy CSuper;

public:
	CJabberDlgBookmarks(CJabberProto *proto);

	void UpdateData();

protected:
	void OnInitDialog();
	void OnClose();
	void OnDestroy();
	int Resizer(UTILRESIZECONTROL *urc);

	BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);
	void OnProtoCheckOnline(WPARAM wParam, LPARAM lParam);
	void OnProtoRefresh(WPARAM wParam, LPARAM lParam);
	void OpenBookmark();

private:
	CCtrlMButton	m_btnAdd;
	CCtrlMButton	m_btnEdit;
	CCtrlMButton	m_btnRemove;
	CCtrlFilterListView	m_lvBookmarks;

	void lvBookmarks_OnDoubleClick(CCtrlFilterListView *)
	{
		OpenBookmark();
	}

	void btnAdd_OnClick(CCtrlFilterListView *)
	{
		if (!m_proto->m_bJabberOnline) return;

		JabberAddBookmarkDlgParam param;
		param.ppro = m_proto;
		param.m_item = NULL;
		DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_BOOKMARK_ADD ), m_hwnd, JabberAddBookmarkDlgProc, (LPARAM)&param);
	}

	void btnEdit_OnClick(CCtrlFilterListView *)
	{
		if (!m_proto->m_bJabberOnline) return;

		int iItem = m_lvBookmarks.GetNextItem(-1, LVNI_SELECTED);
		if (iItem < 0) return;

		TCHAR *address = (TCHAR *)m_lvBookmarks.GetItemData(iItem);
		if (!address) return;

		JABBER_LIST_ITEM *item = m_proto->ListGetItemPtr(LIST_BOOKMARK, address);
		if (!item) return;

		JabberAddBookmarkDlgParam param;
		param.ppro = m_proto;
		param.m_item = item;
		DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_BOOKMARK_ADD ), m_hwnd, JabberAddBookmarkDlgProc, (LPARAM)&param);
	}

	void btnRemove_OnClick(CCtrlFilterListView *)
	{
		if (!m_proto->m_bJabberOnline) return;

		int iItem = m_lvBookmarks.GetNextItem(-1, LVNI_SELECTED);
		if (iItem < 0) return;

		TCHAR *address = (TCHAR *)m_lvBookmarks.GetItemData(iItem);
		if (!address) return;

		JABBER_LIST_ITEM *item = m_proto->ListGetItemPtr(LIST_BOOKMARK, address);
		if (!item) return;

		m_btnAdd.Disable();
		m_btnEdit.Disable();
		m_btnRemove.Disable();

		m_proto->ListRemove(LIST_BOOKMARK, address);

		m_lvBookmarks.SetItemState(iItem, 0, LVIS_SELECTED); // Unselect the item

		int iqId = m_proto->SerialNext();
		m_proto->IqAdd(iqId, IQ_PROC_SETBOOKMARKS, &CJabberProto::OnIqResultSetBookmarks);

		XmlNodeIq iq( _T("set"), iqId);
		m_proto->SetBookmarkRequest(iq);
		m_proto->m_ThreadInfo->send(iq);
	}

};

CJabberDlgBookmarks::CJabberDlgBookmarks(CJabberProto *proto) :
	CJabberDlgFancy(proto, IDD_BOOKMARKS, NULL),
	m_btnAdd(this,      IDC_ADD,    SKINICON_OTHER_ADDCONTACT, LPGEN("Add")),
	m_btnEdit(this,     IDC_EDIT,   SKINICON_OTHER_RENAME,     LPGEN("Edit")),
	m_btnRemove(this,   IDC_REMOVE, SKINICON_OTHER_DELETE,     LPGEN("Remove")),
	m_lvBookmarks(this, IDC_BM_LIST, true, true)
{
	m_lvBookmarks.OnItemActivate = Callback(this, &CJabberDlgBookmarks::lvBookmarks_OnDoubleClick);
	m_btnAdd.OnClick = Callback(this, &CJabberDlgBookmarks::btnAdd_OnClick);
	m_btnEdit.OnClick = Callback(this, &CJabberDlgBookmarks::btnEdit_OnClick);
	m_btnRemove.OnClick = Callback(this, &CJabberDlgBookmarks::btnRemove_OnClick);
}

void CJabberDlgBookmarks::UpdateData()
{
	if (!m_proto->m_bJabberOnline) return;

	int iqId = m_proto->SerialNext();
	m_proto->IqAdd( iqId, IQ_PROC_DISCOBOOKMARKS, &CJabberProto::OnIqResultDiscoBookmarks);
	m_proto->m_ThreadInfo->send( XmlNodeIq( _T("get"), iqId ) << XQUERY( _T(JABBER_FEAT_PRIVATE_STORAGE)) 
		<< XCHILDNS( _T("storage"), _T("storage:bookmarks")));
}

void CJabberDlgBookmarks::OnInitDialog()
{
	CSuper::OnInitDialog();

	SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)m_proto->LoadIconEx("bookmarks"));

	m_btnAdd.Disable();
	m_btnEdit.Disable();
	m_btnRemove.Disable();

	m_lvBookmarks.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_HEADERDRAGDROP | (IsWinVerXPPlus() ? LVS_EX_DOUBLEBUFFER : 0));

	HIMAGELIST hIml = m_lvBookmarks.CreateImageList(LVSIL_SMALL);
	ImageList_AddIcon(hIml, m_proto->LoadIconEx("group"));
	ImageList_AddIcon(hIml, LoadSkinnedIcon(SKINICON_EVENT_URL));

	m_lvBookmarks.AddColumn(0, TranslateT("Bookmark Name"),			m_proto->JGetWord(NULL, "bookmarksWnd_cx0", 120));
	m_lvBookmarks.AddColumn(1, TranslateT("Address (JID or URL)"),	m_proto->JGetWord(NULL, "bookmarksWnd_cx1", 210));
	m_lvBookmarks.AddColumn(2, TranslateT("Nickname"),				m_proto->JGetWord(NULL, "bookmarksWnd_cx2", 90));

	m_lvBookmarks.EnableGroupView(TRUE);
	m_lvBookmarks.AddGroup(0, TranslateT("Conferences"));
	m_lvBookmarks.AddGroup(1, TranslateT("Links"));

	Utils_RestoreWindowPosition(m_hwnd, NULL, m_proto->m_szModuleName, "bookmarksWnd_");
}

void CJabberDlgBookmarks::OnClose()
{
	LVCOLUMN lvc = {0};
	lvc.mask = LVCF_WIDTH;
	m_lvBookmarks.GetColumn(0, &lvc);
	m_proto->JSetWord(NULL, "bookmarksWnd_cx0", lvc.cx);
	m_lvBookmarks.GetColumn(1, &lvc);
	m_proto->JSetWord(NULL, "bookmarksWnd_cx1", lvc.cx);
	m_lvBookmarks.GetColumn(2, &lvc);
	m_proto->JSetWord(NULL, "bookmarksWnd_cx2", lvc.cx);

	Utils_SaveWindowPosition(m_hwnd, NULL, m_proto->m_szModuleName, "bookmarksWnd_");

	CSuper::OnClose();
}

void CJabberDlgBookmarks::OnDestroy()
{
	m_proto->m_pDlgBookmarks = NULL;

	CSuper::OnDestroy();
}

void CJabberDlgBookmarks::OpenBookmark()
{
	int iItem = m_lvBookmarks.GetNextItem(-1, LVNI_SELECTED);
	if (iItem < 0) return;

	TCHAR *address = (TCHAR *)m_lvBookmarks.GetItemData(iItem);
	if (!address) return;

	JABBER_LIST_ITEM *item = m_proto->ListGetItemPtr(LIST_BOOKMARK, address);
	if (!item) return;

	if (!lstrcmpi(item->type, _T("conference")))
	{
		if (!jabberChatDllPresent)
		{
			JabberChatDllError();
			return;
		}

		m_lvBookmarks.SetItemState(iItem, 0, LVIS_SELECTED); // Unselect the item

		/* some hack for using bookmark to transport not under XEP-0048 */
		if (!_tcschr(item->jid, _T('@')))
		{	//the room name is not provided let consider that it is transport and send request to registration
			m_proto->RegisterAgent(NULL, item->jid);
		} else
		{
			TCHAR *room = NEWTSTR_ALLOCA(item->jid);
			TCHAR *server = _tcschr(room, _T('@'));
			*(server++) = 0;

			if (item->nick && *item->nick)
			{
				m_proto->GroupchatJoinRoom(server, room, item->nick, item->password);
			} else
			{
				TCHAR* nick = JabberNickFromJID(m_proto->m_szJabberJID);
				m_proto->GroupchatJoinRoom(server, room, nick, item->password);
				mir_free(nick);
			}
		}
	} else
	{
		char *szUrl = mir_t2a(item->jid);
		JCallService(MS_UTILS_OPENURL, 1, (LPARAM)szUrl);
		mir_free(szUrl);
	}
}

BOOL CJabberDlgBookmarks::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
			lpmmi->ptMinTrackSize.x = 451;
			lpmmi->ptMinTrackSize.y = 320;
			return 0;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDOK:
			{
				OpenBookmark();
				return TRUE;
			}
		}
		break;
	}

	return CSuper::DlgProc(msg, wParam, lParam);
}

void CJabberDlgBookmarks::OnProtoCheckOnline(WPARAM, LPARAM)
{
	if (!m_proto->m_bJabberOnline)
	{
		m_btnAdd.Disable();
		m_btnEdit.Disable();
		m_btnRemove.Disable();
	} else
	{
		UpdateData();
	}

}

void CJabberDlgBookmarks::OnProtoRefresh(WPARAM, LPARAM)
{
	m_lvBookmarks.DeleteAllItems();

	JABBER_LIST_ITEM *item = NULL;
	for (int i = 0; (i = m_proto->ListFindNext(LIST_BOOKMARK, i)) >= 0; ++i)
	{
		if (item = m_proto->ListGetItemPtrFromIndex(i))
		{
			int itemType = lstrcmpi(item->type, _T("conference")) ? 1 : 0;
			int iItem = m_lvBookmarks.AddItem(item->name, itemType, (LPARAM)item->jid, itemType);
			m_lvBookmarks.SetItem(iItem, 1, item->jid);
			if (itemType == 0)
				m_lvBookmarks.SetItem(iItem, 2, item->nick);
		}
	}

	if (item)
	{
		m_btnEdit.Enable();
		m_btnRemove.Enable();
	}

	m_btnAdd.Enable();
}

int CJabberDlgBookmarks::Resizer(UTILRESIZECONTROL *urc)
{
	switch ( urc->wId ) {
		case IDC_BM_LIST:
			return RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;

		case IDCANCEL:
			return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;

		case IDC_ADD:
		case IDC_EDIT:
		case IDC_REMOVE:
			return RD_ANCHORX_LEFT|RD_ANCHORY_BOTTOM;
	}
	return CSuper::Resizer(urc);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Launches the Bookmarks manager window

int __cdecl CJabberProto::OnMenuHandleBookmarks( WPARAM, LPARAM)
{
	UI_SAFE_OPEN_EX(CJabberDlgBookmarks, m_pDlgBookmarks, pDlg);
	pDlg->UpdateData();
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Launches the Bookmark details window, lParam is JABBER_BOOKMARK_ITEM*
int CJabberProto::AddEditBookmark( JABBER_LIST_ITEM* item )
{
	if ( m_bJabberOnline) {
		JabberAddBookmarkDlgParam param;
		param.ppro = this;
		param.m_item = item;//(JABBER_LIST_ITEM*)lParam;
		DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_BOOKMARK_ADD ), NULL, JabberAddBookmarkDlgProc, (LPARAM)&param );
	}
	return 0;
}
