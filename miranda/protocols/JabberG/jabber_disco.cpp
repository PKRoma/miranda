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

void JabberIqResultServiceDiscoveryItems( XmlNode* iqNode, void* userdata, CJabberIqRequestInfo* pInfo );

void JabberIqResultServiceDiscoveryInfo( XmlNode* iqNode, void* userdata, CJabberIqRequestInfo* pInfo )
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

void JabberIqResultServiceDiscoveryItems( XmlNode* iqNode, void* userdata, CJabberIqRequestInfo* pInfo )
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
		CJabberIqRequestInfo* pInfo = g_JabberIqRequestManager.AddHandler( JabberIqResultServiceDiscoveryInfo, JABBER_IQ_TYPE_GET, pNode->GetJid() );
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
		CJabberIqRequestInfo* pInfo = g_JabberIqRequestManager.AddHandler( JabberIqResultServiceDiscoveryItems, JABBER_IQ_TYPE_GET, pNode->GetJid() );
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

int JabberServiceDiscoveryDlg(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL *urc)
{
	switch ( urc->wId ) {
	case IDC_TREE_DISCO:
		return RD_ANCHORX_WIDTH|RD_ANCHORY_HEIGHT;
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
		TranslateDialogDefault( hwndDlg );
		SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )LoadIconEx( "servicediscovery" ));
		hwndServiceDiscovery = hwndDlg;
		{
			TCHAR *szServer = a2t( jabberThreadInfo->server );
			SetDlgItemText( hwndDlg, IDC_COMBO_JID, szServer );
			mir_free(szServer);
			SetDlgItemText( hwndDlg, IDC_COMBO_NODE, _T("") );

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

			PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_BUTTON_BROWSE, 0 ), 0 );
		}
		Utils_RestoreWindowPosition(hwndDlg, NULL, jabberProtoName, "discoWnd_");
		return TRUE;

	case WM_SIZE:
		{
			UTILRESIZEDIALOG urd;
			urd.cbSize = sizeof(urd);
			urd.hwndDlg = hwndDlg;
			urd.hInstance = hInst;
			urd.lpTemplate = MAKEINTRESOURCEA( IDD_SERVICE_DISCOVERY );
			urd.lParam = 0;
			urd.pfnResizer = JabberServiceDiscoveryDlg;
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
				for (int iChild = TreeList_GetChildrenCount(hItem); iChild--; ) {
					CJabberSDNode* pNode = (CJabberSDNode* )TreeList_GetData(TreeList_GetChild(hItem, iChild));
					if ( pNode ) 
						SendBothRequests( pNode, packet );

					if ( packet->numChild >= 50 ) {
						jabberThreadInfo->send( *packet );
						delete packet;
						packet = new XmlNode( NULL );
				}	}
				g_SDManager.Unlock();

				if ( packet->numChild )
					jabberThreadInfo->send( *packet );
				delete packet;
		}	}

		return TRUE;

	case WM_COMMAND:
		if ( LOWORD( wParam ) == IDC_BUTTON_BROWSE ) {
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
		return FALSE;

	case WM_CLOSE:
	{
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

int JabberMenuHandleServiceDiscovery( WPARAM wParam, LPARAM lParam )
{
	if ( hwndServiceDiscovery && IsWindow( hwndServiceDiscovery ))
		SetForegroundWindow( hwndServiceDiscovery );
	else
		CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_SERVICE_DISCOVERY ), NULL, JabberServiceDiscoveryDlgProc, lParam );

	return 0;
}
