/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

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

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_disco.cpp,v $
Revision       : $Revision: 5337 $
Last change on : $Date: 2007-04-28 13:26:31 +0300 (бс, 28 ря№ 2007) $
Last change by : $Author: ghazan $

*/

#include "jabber.h"
#include "commctrl.h"
#include "jabber_iq.h"
#include "resource.h"
#include "jabber_disco.h"

CJabberSDManager g_SDManager;

#define REFRESH_TIMEOUT		500
#define REFRESH_TIMER		1607
static DWORD sttLastRefresh = 0;

#define INFO_REQUEST_LIMIT	25

void JabberServiceDiscoveryShowMenu(CJabberSDNode *node, HTREELISTITEM hItem, POINT pt);

void JabberIqResultServiceDiscoveryItems( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );

void JabberIqResultServiceDiscoveryInfo( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	g_SDManager.Lock();
	CJabberSDNode* pNode = g_SDManager.GetPrimaryNode()->FindByIqId( pInfo->GetIqId(), TRUE );
	if ( !pNode ) {
		g_SDManager.Unlock();
		return;
	}

	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT ) {
		XmlNode *query = JabberXmlGetChild( iqNode, "query" );
		if ( !query )
			pNode->SetInfoRequestId( JABBER_DISCO_RESULT_ERROR );
		else {
			XmlNode *feature;
			int i;
			for ( i = 1; ( feature = JabberXmlGetNthChild( query, "feature", i )) != NULL; i++ )
				pNode->AddFeature( JabberXmlGetAttrValue( feature, "var" ) );
			XmlNode *identity;
			for ( i = 1; ( identity = JabberXmlGetNthChild( query, "identity", i )) != NULL; i++ )
				pNode->AddIdentity( JabberXmlGetAttrValue( identity, "category" ),
									JabberXmlGetAttrValue( identity, "type" ),
									JabberXmlGetAttrValue( identity, "name" ));

			pNode->SetInfoRequestId( JABBER_DISCO_RESULT_OK );
		}
	}
	else pNode->SetInfoRequestId( JABBER_DISCO_RESULT_ERROR );

	g_SDManager.Unlock();

	if ( hwndServiceDiscovery )
		PostMessage( hwndServiceDiscovery, WM_JABBER_REFRESH, 0, 0 );
}

void JabberIqResultServiceDiscoveryItems( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	g_SDManager.Lock();
	CJabberSDNode* pNode = g_SDManager.GetPrimaryNode()->FindByIqId( pInfo->GetIqId(), FALSE );
	if ( !pNode ) {
		g_SDManager.Unlock();
		return;
	}

	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT ) {
		XmlNode *query = JabberXmlGetChild( iqNode, "query" );
		if ( !query )
			pNode->SetItemsRequestId( JABBER_DISCO_RESULT_ERROR );
		else {
			XmlNode *item;
			for ( int i = 1; ( item = JabberXmlGetNthChild( query, "item", i )) != NULL; i++ ) {
				pNode->AddChildNode( JabberXmlGetAttrValue( item, "jid" ),
					JabberXmlGetAttrValue( item, "node" ),
					JabberXmlGetAttrValue( item, "name" ));
			}

			pNode->SetItemsRequestId( JABBER_DISCO_RESULT_OK );
		}
	}
	else pNode->SetItemsRequestId( JABBER_DISCO_RESULT_ERROR );

	g_SDManager.Unlock();

	if ( hwndServiceDiscovery )
		PostMessage( hwndServiceDiscovery, WM_JABBER_REFRESH, 0, 0 );
}

BOOL SyncTree(HTREELISTITEM hIndex, CJabberSDNode* pNode)
{
	CJabberSDNode* pTmp = pNode;
	while (pTmp) {
		if ( !pTmp->GetTreeItemHandle() ) {
			HTREELISTITEM hNewItem = TreeList_AddItem(
				GetDlgItem(hwndServiceDiscovery, IDC_TREE_DISCO), hIndex,
				pTmp->GetName() ? pTmp->GetName() : pTmp->GetJid(),
				(LPARAM)pTmp);
			TreeList_AppendColumn(hNewItem, pTmp->GetJid());
			TreeList_AppendColumn(hNewItem, pTmp->GetNode());
			if (!pTmp->GetInfoRequestId())
				TreeList_MakeFakeParent(hNewItem, TRUE);
			else
				TreeList_MakeFakeParent(hNewItem, FALSE);
			pTmp->SetTreeItemHandle( hNewItem );
		}

		if ( pTmp->GetFirstChildNode() )
			SyncTree( pTmp->GetTreeItemHandle(), pTmp->GetFirstChildNode() );

		pTmp = pTmp->GetNext();
	}
	return TRUE;
}

BOOL SendBothRequests(CJabberSDNode* pNode, XmlNode* parent)
{
	if ( !pNode || !jabberOnline )
		return FALSE;

	// disco#info
	if ( !pNode->GetInfoRequestId() ) {
		CJabberIqInfo* pInfo = g_JabberIqManager.AddHandler( JabberIqResultServiceDiscoveryInfo, JABBER_IQ_TYPE_GET, pNode->GetJid() );
		pInfo->SetTimeout( 30000 );
		pNode->SetInfoRequestId( pInfo->GetIqId() );

		XmlNodeIq* iq = new XmlNodeIq( pInfo );
		XmlNode* query = iq->addQuery( JABBER_FEAT_DISCO_INFO );
		if ( pNode->GetNode() )
			query->addAttr( "node", pNode->GetNode() );

		if ( parent )
			parent->addChild( iq );
		else {
			jabberThreadInfo->send( *iq );
			delete( iq );
	}	}

	// disco#items
	if ( !pNode->GetItemsRequestId() ) {
		CJabberIqInfo* pInfo = g_JabberIqManager.AddHandler( JabberIqResultServiceDiscoveryItems, JABBER_IQ_TYPE_GET, pNode->GetJid() );
		pInfo->SetTimeout( 30000 );
		pNode->SetItemsRequestId( pInfo->GetIqId() );

		XmlNodeIq* iq = new XmlNodeIq( pInfo );
		XmlNode* query = iq->addQuery( JABBER_FEAT_DISCO_ITEMS );
		if ( pNode->GetNode() )
			query->addAttr( "node", pNode->GetNode() );

		if ( parent )
			parent->addChild( iq );
		else {
			jabberThreadInfo->send( *iq );
			delete( iq );
	}	}

	if ( hwndServiceDiscovery )
		PostMessage( hwndServiceDiscovery, WM_JABBER_REFRESH, 0, 0 );

	return TRUE;
}

int JabberServiceDiscoveryDlgResizer(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL *urc)
{
	RECT rc;

	switch ( urc->wId ) {
		case IDC_COMBO_JID:
		{
			GetWindowRect(GetDlgItem(hwndDlg, urc->wId), &rc);
			urc->rcItem.right += (urc->dlgNewSize.cx - urc->dlgOriginalSize.cx) / 2;
			urc->rcItem.bottom = urc->rcItem.top + rc.bottom - rc.top;
			return 0;
		}
		case IDC_TXT_NODE:
		{
			urc->rcItem.left += (urc->dlgNewSize.cx - urc->dlgOriginalSize.cx) / 2;
			urc->rcItem.right += (urc->dlgNewSize.cx - urc->dlgOriginalSize.cx) / 2;
			return 0;
		}
		case IDC_COMBO_NODE:
		{
			GetWindowRect(GetDlgItem(hwndDlg, urc->wId), &rc);
			urc->rcItem.left += (urc->dlgNewSize.cx - urc->dlgOriginalSize.cx) / 2;
			urc->rcItem.right += urc->dlgNewSize.cx - urc->dlgOriginalSize.cx;
			urc->rcItem.bottom = urc->rcItem.top + rc.bottom - rc.top;
			return 0;
		}
		case IDC_BUTTON_BROWSE:
			return RD_ANCHORX_RIGHT|RD_ANCHORY_TOP;

		case IDC_TREE_DISCO:
			return RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;

		case IDC_TXT_FILTER:
			return RD_ANCHORX_LEFT|RD_ANCHORY_BOTTOM;
		case IDC_TXT_FILTERTEXT:
			return RD_ANCHORX_WIDTH|RD_ANCHORY_BOTTOM;
		case IDC_BTN_FILTERAPPLY:
		case IDC_BTN_FILTERRESET:
			return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
	}
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
}

BOOL CALLBACK JabberServiceDiscoveryDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	BOOL result;
	if (TreeList_ProcessMessage(hwndDlg, msg, wParam, lParam, IDC_TREE_DISCO, &result))
		return result;

	switch ( msg ) {
	case WM_INITDIALOG:
		mir_alloc(666);

		TranslateDialogDefault( hwndDlg );
		SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )LoadIconEx( "servicediscovery" ));
		hwndServiceDiscovery = hwndDlg;
		{
			int i;
			SetDlgItemTextA( hwndDlg, IDC_COMBO_JID, jabberThreadInfo->server );
			SetDlgItemText( hwndDlg, IDC_COMBO_NODE, _T("") );

			static struct
			{
				int idc;
				TCHAR *title;
				char *icon;
				bool push;
			} buttons[] =
			{
				{IDC_BTN_NAVHOME,		_T("Navigate home"),	"sd_nav_home",		false},
				{IDC_BTN_REFRESH,		_T("Refresh node"),		"sd_nav_refresh",	false},
				{IDC_BTN_VIEWLIST,		_T("View as list"),		"sd_view_list",		true},
				{IDC_BTN_VIEWTREE,		_T("View as tree"),		"sd_view_tree",		true},
				{IDC_BUTTON_BROWSE,		_T("Browse"),			"sd_browse",		false},
				{IDC_BTN_FILTERAPPLY,	_T("Apply filter"),		"sd_filter_apply",	false},
				{IDC_BTN_FILTERRESET,	_T("Reset filter"),		"sd_filter_reset",	false},
			};
			for (i = 0; i < SIZEOF(buttons); ++i)
			{
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIconEx(buttons[i].icon));
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONSETASFLATBTN, 0, 0);
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONADDTOOLTIP, (WPARAM)TranslateTS(buttons[i].title), BATF_TCHAR);
				if (buttons[i].push)
					SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONSETASPUSHBTN, 0, 0);
			}
			CheckDlgButton(hwndDlg,
				DBGetContactSettingByte(NULL, jabberProtoName, "discoWnd_useTree", 0) ?
					IDC_BTN_VIEWTREE : IDC_BTN_VIEWLIST,
				TRUE);

			EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_FILTERRESET), FALSE);

			HWND hwndList = GetDlgItem(hwndDlg, IDC_TREE_DISCO);
			LVCOLUMN lvc = {0};
			lvc.mask = LVCF_SUBITEM|LVCF_WIDTH|LVCF_TEXT;
			lvc.cx = DBGetContactSettingWord(NULL, jabberProtoName, "discoWnd_cx0", 200);
			lvc.iSubItem = 0;
			lvc.pszText = _T("Node hierarchy");
			ListView_InsertColumn(hwndList, 0, &lvc);
			lvc.cx = DBGetContactSettingWord(NULL, jabberProtoName, "discoWnd_cx1", 200);
			lvc.iSubItem = 1;
			lvc.pszText = _T("JID");
			ListView_InsertColumn(hwndList, 1, &lvc);
			lvc.cx = DBGetContactSettingWord(NULL, jabberProtoName, "discoWnd_cx2", 200);
			lvc.iSubItem = 2;
			lvc.pszText = _T("Node");
			ListView_InsertColumn(hwndList, 2, &lvc);

			TreeList_Create(hwndList);
			TreeList_AddIcon(hwndList, LoadIconEx("main"), 0);
			TreeList_AddIcon(hwndList, LoadIconEx("disco_fail"), 1);
			TreeList_AddIcon(hwndList, LoadIconEx("disco_ok"), 2);

			TreeList_SetMode(hwndList, DBGetContactSettingByte(NULL, jabberProtoName, "discoWnd_useTree", 0) ? TLM_TREE : TLM_REPORT);

			PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_BUTTON_BROWSE, 0 ), 0 );
		}
		Utils_RestoreWindowPosition(hwndDlg, NULL, jabberProtoName, "discoWnd_");
		return TRUE;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
			lpmmi->ptMinTrackSize.x = 538;
			lpmmi->ptMinTrackSize.y = 374;
			return 0;
		}

	case WM_SIZE:
		{
			UTILRESIZEDIALOG urd;
			urd.cbSize = sizeof(urd);
			urd.hwndDlg = hwndDlg;
			urd.hInstance = hInst;
			urd.lpTemplate = MAKEINTRESOURCEA( IDD_SERVICE_DISCOVERY );
			urd.lParam = 0;
			urd.pfnResizer = JabberServiceDiscoveryDlgResizer;
			CallService( MS_UTILS_RESIZEDIALOG, 0, (LPARAM)&urd );
			return TRUE;
		}

	case WM_JABBER_REFRESH:
		KillTimer(hwndDlg, REFRESH_TIMER);
		if (GetTickCount() - sttLastRefresh < REFRESH_TIMEOUT) {
			SetTimer(hwndDlg, REFRESH_TIMER, REFRESH_TIMEOUT, NULL);
			return TRUE;
		} 
		
		wParam = REFRESH_TIMER;
		// fall through

	case WM_TIMER:
		if (wParam == REFRESH_TIMER) {
			g_SDManager.Lock();

			CJabberSDNode* pNode = g_SDManager.GetPrimaryNode();
			if ( pNode->GetJid() ) {
				int nNodeCount = ListView_GetItemCount(GetDlgItem(hwndDlg, IDC_TREE_DISCO));
				if ( !nNodeCount ) {
					HTREELISTITEM hNewItem = TreeList_AddItem(
						GetDlgItem(hwndServiceDiscovery, IDC_TREE_DISCO), NULL,
						pNode->GetName() ? pNode->GetName() : pNode->GetJid(),
						(LPARAM)pNode);
					TreeList_AppendColumn(hNewItem, pNode->GetJid());
					TreeList_AppendColumn(hNewItem, pNode->GetNode());
					pNode->SetTreeItemHandle( hNewItem );
			}	}

			SyncTree( NULL, pNode );
			g_SDManager.Unlock();
			TreeList_Update(GetDlgItem(hwndDlg, IDC_TREE_DISCO));
			KillTimer(hwndDlg, REFRESH_TIMER);
			sttLastRefresh = GetTickCount();
			return TRUE;
		}

	case WM_NOTIFY:
		if ( wParam == IDC_TREE_DISCO ) {
			NMHDR* pHeader = (NMHDR* )lParam;
			if ( pHeader->code == LVN_GETINFOTIP ) {
				NMLVGETINFOTIP *pInfoTip = (NMLVGETINFOTIP *)lParam;
				LVITEM lvi;
				lvi.mask = LVIF_PARAM;
				lvi.iItem = pInfoTip->iItem;
				ListView_GetItem(pHeader->hwndFrom, &lvi);
				HTREELISTITEM hItem = (HTREELISTITEM)lvi.lParam;
				g_SDManager.Lock();
				CJabberSDNode* pNode = (CJabberSDNode* )TreeList_GetData(hItem);
				if ( pNode ) {
					pNode->GetTooltipText( pInfoTip->pszText, pInfoTip->cchTextMax );
				}
				g_SDManager.Unlock();
			}
			else if ( pHeader->code == TVN_ITEMEXPANDED ) {
				NMTREEVIEW *pNmTreeView = (NMTREEVIEW *)lParam;
				HTREELISTITEM hItem = (HTREELISTITEM)pNmTreeView->itemNew.hItem;

				g_SDManager.Lock();
				XmlNode* packet = new XmlNode( NULL );
				CJabberSDNode* pNode;
				pNode = (CJabberSDNode* )TreeList_GetData(hItem);
				if ( pNode ) 
				{
					SendBothRequests( pNode, packet );
					TreeList_MakeFakeParent(hItem, FALSE);
				}
				g_SDManager.Unlock();

				if ( packet->numChild )
					jabberThreadInfo->send( *packet );
				delete packet;
			}
			else if ( pHeader->code == NM_RCLICK ) {
				HTREELISTITEM hItem = TreeList_GetActiveItem(GetDlgItem(hwndDlg, IDC_TREE_DISCO));
				if (!hItem) break;
				CJabberSDNode *pNode = (CJabberSDNode *)TreeList_GetData(hItem);
				if (!pNode) break;
				JabberServiceDiscoveryShowMenu(pNode, hItem, ((LPNMITEMACTIVATE)lParam)->ptAction);
		}	}

		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDC_BUTTON_BROWSE:
			{
				TCHAR szJid[ 512 ];
				TCHAR szNode[ 512 ];
				if ( !GetDlgItemText( hwndDlg, IDC_COMBO_JID, szJid, SIZEOF( szJid )))
					szJid[ 0 ] = _T('\0');
				if ( !GetDlgItemText( hwndDlg, IDC_COMBO_NODE, szNode, SIZEOF( szNode )))
					szNode[ 0 ] = _T('\0');

				if ( _tcslen( szJid )) {
					HWND hwndList = GetDlgItem(hwndDlg, IDC_TREE_DISCO);
					TreeList_Destroy(hwndList);
					TreeList_Create(hwndList);
					TreeList_AddIcon(hwndList, LoadIconEx("main"), 0);
					TreeList_AddIcon(hwndList, LoadIconEx("disco_fail"), 1);
					TreeList_AddIcon(hwndList, LoadIconEx("disco_ok"), 2);
					TreeList_SetMode(hwndList, IsDlgButtonChecked(hwndDlg, IDC_BTN_VIEWTREE) ? TLM_TREE : TLM_REPORT);

					g_SDManager.Lock();

					CJabberSDNode* pNode = g_SDManager.GetPrimaryNode();

					pNode->RemoveAll();
					pNode->SetJid( szJid );
					pNode->SetNode( _tcslen( szNode ) ? szNode : NULL );

					SendBothRequests( pNode, NULL );

					g_SDManager.Unlock();

					PostMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
				}
				return TRUE;
			}
			case IDC_BTN_REFRESH:
			{
				HTREELISTITEM hItem = (HTREELISTITEM)TreeList_GetActiveItem(GetDlgItem(hwndDlg, IDC_TREE_DISCO));
				if (!hItem) return TRUE;

				g_SDManager.Lock();
				XmlNode* packet = new XmlNode( NULL );
				CJabberSDNode* pNode = (CJabberSDNode* )TreeList_GetData(hItem);
				if ( pNode )
				{
					SendBothRequests( pNode, packet );
					TreeList_MakeFakeParent(hItem, FALSE);
				}
				for (int iChild = TreeList_GetChildrenCount(hItem); iChild--; ) {
					pNode = (CJabberSDNode* )TreeList_GetData(TreeList_GetChild(hItem, iChild));
					if ( pNode )
					{
						SendBothRequests( pNode, packet );
						TreeList_MakeFakeParent(TreeList_GetChild(hItem, iChild), FALSE);
					}

					if ( packet->numChild >= 50 ) {
						jabberThreadInfo->send( *packet );
						delete packet;
						packet = new XmlNode( NULL );
				}	}
				g_SDManager.Unlock();

				if ( packet->numChild )
					jabberThreadInfo->send( *packet );
				delete packet;
				return TRUE;
			}
			case IDC_BTN_NAVHOME:
			{
				SetDlgItemTextA( hwndDlg, IDC_COMBO_JID, jabberThreadInfo->server );
				SetDlgItemText( hwndDlg, IDC_COMBO_NODE, _T("") );
				PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_BUTTON_BROWSE, 0 ), 0 );
				return TRUE;
			}
			case IDC_BTN_VIEWLIST:
			{
				CheckDlgButton(hwndDlg, IDC_BTN_VIEWLIST, TRUE);
				CheckDlgButton(hwndDlg, IDC_BTN_VIEWTREE, FALSE);
				TreeList_SetMode(GetDlgItem(hwndDlg, IDC_TREE_DISCO), TLM_REPORT);
				return TRUE;
			}
			case IDC_BTN_VIEWTREE:
			{
				CheckDlgButton(hwndDlg, IDC_BTN_VIEWLIST, FALSE);
				CheckDlgButton(hwndDlg, IDC_BTN_VIEWTREE, TRUE);
				TreeList_SetMode(GetDlgItem(hwndDlg, IDC_TREE_DISCO), TLM_TREE);
				return TRUE;
			}
			case IDC_BTN_FILTERAPPLY:
			{
				int length = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_TXT_FILTERTEXT))+1;
				TCHAR *txt = (TCHAR *)mir_alloc(sizeof(TCHAR)*length);
				GetDlgItemText(hwndDlg, IDC_TXT_FILTERTEXT, txt, length);
				TreeList_SetFilter(GetDlgItem(hwndDlg, IDC_TREE_DISCO), txt);
				mir_free(txt);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_FILTERRESET), TRUE);
				return TRUE;
			}
			case IDC_BTN_FILTERRESET:
			{
				TreeList_SetFilter(GetDlgItem(hwndDlg, IDC_TREE_DISCO), NULL);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_FILTERRESET), FALSE);
				return TRUE;
			}
		}
		return FALSE;

	case WM_CLOSE:
	{
		DBWriteContactSettingByte(NULL, jabberProtoName, "discoWnd_useTree", IsDlgButtonChecked(hwndDlg, IDC_BTN_VIEWTREE));

		HWND hwndList = GetDlgItem(hwndDlg, IDC_TREE_DISCO);
		LVCOLUMN lvc = {0};
		lvc.mask = LVCF_WIDTH;
		ListView_GetColumn(hwndList, 0, &lvc);
		DBWriteContactSettingWord(NULL, jabberProtoName, "discoWnd_cx0", lvc.cx);
		ListView_GetColumn(hwndList, 1, &lvc);
		DBWriteContactSettingWord(NULL, jabberProtoName, "discoWnd_cx1", lvc.cx);
		ListView_GetColumn(hwndList, 2, &lvc);
		DBWriteContactSettingWord(NULL, jabberProtoName, "discoWnd_cx2", lvc.cx);

		Utils_SaveWindowPosition(hwndDlg, NULL, jabberProtoName, "discoWnd_");
		DestroyWindow( hwndDlg );
		return TRUE;
	}

	case WM_DESTROY:
		hwndServiceDiscovery = NULL;
		g_SDManager.Lock();
		g_SDManager.GetPrimaryNode()->RemoveAll();
		g_SDManager.Unlock();
		TreeList_Destroy(GetDlgItem(hwndDlg, IDC_TREE_DISCO));
		break;
	}
	return FALSE;
}

// extern references to used functions:
void JabberRegisterAgent( HWND hwndDlg, TCHAR* jid );
void JabberGroupchatJoinRoomByJid(HWND hwndParent, TCHAR *jid);
void JabberSearchAddToRecent( TCHAR* szAddr, HWND hwndDialog = NULL );

void JabberServiceDiscoveryShowMenu(CJabberSDNode *pNode, HTREELISTITEM hItem, POINT pt)
{
	ClientToScreen(GetDlgItem(hwndServiceDiscovery, IDC_TREE_DISCO), &pt);

	enum {
		SD_ACT_REFRESH = 1, SD_ACT_REFRESHCHILDREN, SD_ACT_FAVORITE, SD_ACT_ROSTER,
		SD_ACT_REGISTER = 50, SD_ACT_ADHOC, SD_ACT_ADDDIRECTORY, SD_ACT_JOIN, SD_ACT_PROXY, SD_ACT_VCARD
	};

	static struct
	{
		TCHAR *feature;
		TCHAR *title;
		int action;
	} items[] =
	{
		{NULL,						_T("Refresh Info"),			SD_ACT_REFRESH},
		{NULL,						_T("Refresh Children"),		SD_ACT_REFRESHCHILDREN},
		{NULL,						NULL,						0},
		{_T(JABBER_FEAT_COMMANDS),	_T("Commands..."),			SD_ACT_ADHOC},
		{_T(JABBER_FEAT_REGISTER),	_T("Register"),				SD_ACT_REGISTER},
		{_T("jabber:iq:search"),	_T("Add search directory"),	SD_ACT_ADDDIRECTORY},
		{_T(JABBER_FEAT_MUC),		_T("Join chatroom"),		SD_ACT_JOIN},
	};

	HMENU hMenu = CreatePopupMenu();
	for (int i = 0; i < SIZEOF(items); ++i)
	{
		if (!items[i].feature)
		{
			AppendMenu(hMenu, items[i].title ? MF_STRING : MF_SEPARATOR, items[i].action,
				items[i].title ? TranslateTS(items[i].title) : NULL);
			continue;
		}
		for (CJabberSDFeature *iFeature = pNode->GetFirstFeature(); iFeature; iFeature = iFeature->GetNext())
			if (!lstrcmp(iFeature->GetVar(), items[i].feature))
			{
				AppendMenu(hMenu, items[i].title ? MF_STRING : MF_SEPARATOR, items[i].action,
					items[i].title ? TranslateTS(items[i].title) : NULL);
				break;
			}
	}

	if (!GetMenuItemCount(hMenu))
	{
		DestroyMenu(hMenu);
		return;
	}

	int res = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndServiceDiscovery, NULL);
	DestroyMenu(hMenu);

	switch (res)
	{
		case SD_ACT_REFRESH:
		{
			g_SDManager.Lock();
			XmlNode* packet = new XmlNode( NULL );
			if ( pNode ) 
			{
				SendBothRequests( pNode, packet );
				TreeList_MakeFakeParent(hItem, FALSE);
			}
			g_SDManager.Unlock();

			if ( packet->numChild )
				jabberThreadInfo->send( *packet );
			delete packet;
			break;
		}

		case SD_ACT_REFRESHCHILDREN:
		{
			g_SDManager.Lock();
			XmlNode* packet = new XmlNode( NULL );
			for (int iChild = TreeList_GetChildrenCount(hItem); iChild--; ) {
				CJabberSDNode *pNode = (CJabberSDNode *)TreeList_GetData(TreeList_GetChild(hItem, iChild));
				if ( pNode )
				{
					SendBothRequests( pNode, packet );
					TreeList_MakeFakeParent(TreeList_GetChild(hItem, iChild), FALSE);
				}

				if ( packet->numChild >= 50 ) {
					jabberThreadInfo->send( *packet );
					delete packet;
					packet = new XmlNode( NULL );
			}	}
			g_SDManager.Unlock();

			if ( packet->numChild )
				jabberThreadInfo->send( *packet );
			delete packet;
			break;
		}

		case SD_ACT_REGISTER:
			if (pNode->GetJid())
				JabberRegisterAgent(hwndServiceDiscovery, pNode->GetJid());
			break;

		case SD_ACT_ADHOC:
			if (pNode->GetJid())
				JabberContactMenuRunCommands(0, (LPARAM)pNode->GetJid());
			break;

		case SD_ACT_ADDDIRECTORY:
			if (pNode->GetJid())
				JabberSearchAddToRecent(pNode->GetJid());
			break;

		case SD_ACT_JOIN:
			if (pNode->GetJid())
				JabberGroupchatJoinRoomByJid(hwndServiceDiscovery, pNode->GetJid());
			break;
	}
}

int JabberMenuHandleServiceDiscovery( WPARAM wParam, LPARAM lParam )
{
	if ( hwndServiceDiscovery && IsWindow( hwndServiceDiscovery ))
		SetForegroundWindow( hwndServiceDiscovery );
	else
		CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_SERVICE_DISCOVERY ), NULL, JabberServiceDiscoveryDlgProc, lParam );

	return 0;
}
