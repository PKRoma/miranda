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

void JabberIqResultServiceDiscoveryItems( XmlNode* iqNode, void* userdata, CJabberIqRequestInfo *pInfo );

void JabberIqResultServiceDiscoveryInfo( XmlNode* iqNode, void* userdata, CJabberIqRequestInfo *pInfo )
{
	g_SDManager.Lock();
	CJabberSDNode *pNode = g_SDManager.GetPrimaryNode()->FindByIqId( pInfo->GetIqId(), TRUE );
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

void JabberIqResultServiceDiscoveryItems( XmlNode* iqNode, void* userdata, CJabberIqRequestInfo *pInfo )
{
	g_SDManager.Lock();
	CJabberSDNode *pNode = g_SDManager.GetPrimaryNode()->FindByIqId( pInfo->GetIqId(), FALSE );
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

BOOL SyncTree(HTREEITEM hIndex, CJabberSDNode *pNode)
{
	CJabberSDNode *pTmp = pNode;
	while (pTmp) {
		if ( !pTmp->GetTreeItemHandle() ) {
			TVINSERTSTRUCT tvi;
			ZeroMemory( &tvi, sizeof( tvi ));
			tvi.hParent = hIndex;
			tvi.hInsertAfter = TVI_LAST;
			tvi.itemex.mask = TVIF_PARAM | TVIF_TEXT | TVIF_STATE;
			tvi.itemex.state = TVIS_EXPANDED;
			tvi.itemex.lParam = (LPARAM)pTmp;
			tvi.itemex.pszText = pTmp->GetName() ? pTmp->GetName() : pTmp->GetJid();
			LRESULT lIndex = SendDlgItemMessage( hwndServiceDiscovery, IDC_TREE_DISCO, TVM_INSERTITEM, 0, (LPARAM)&tvi );
			pTmp->SetTreeItemHandle( (HTREEITEM)lIndex );
		}

		if ( pTmp->GetFirstChildNode() )
			SyncTree( pTmp->GetTreeItemHandle(), pTmp->GetFirstChildNode() );

		pTmp = pTmp->GetNext();
	}
	return TRUE;
}

BOOL SendBothRequests(CJabberSDNode *pNode)
{
	if ( !pNode || !jabberOnline )
		return FALSE;

	// disco#info
	if ( !pNode->GetInfoRequestId() ) {
		CJabberIqRequestInfo *pInfo = g_JabberIqRequestManager.AddHandler( JabberIqResultServiceDiscoveryInfo, JABBER_IQ_TYPE_GET, pNode->GetJid() );
		pInfo->SetTimeout( 30000 );
		pNode->SetInfoRequestId( pInfo->GetIqId() );

		XmlNodeIq iq( pInfo );
		XmlNode* query = iq.addQuery( JABBER_FEAT_DISCO_INFO );
		if ( pNode->GetNode() )
			query->addAttr( "node", pNode->GetNode() );

		jabberThreadInfo->send( iq );
	}

	// disco#items
	if ( !pNode->GetItemsRequestId() ) {
		CJabberIqRequestInfo *pInfo = g_JabberIqRequestManager.AddHandler( JabberIqResultServiceDiscoveryItems, JABBER_IQ_TYPE_GET, pNode->GetJid() );
		pInfo->SetTimeout( 30000 );
		pNode->SetItemsRequestId( pInfo->GetIqId() );

		XmlNodeIq iq( pInfo );
		XmlNode* query = iq.addQuery( JABBER_FEAT_DISCO_ITEMS );
		if ( pNode->GetNode() )
			query->addAttr( "node", pNode->GetNode() );

		jabberThreadInfo->send( iq );
	}

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
			PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_BUTTON_BROWSE, 0 ), 0 );
		}
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
		SendDlgItemMessage( hwndDlg, IDC_TREE_DISCO, WM_SETREDRAW, FALSE, 0 );
		g_SDManager.Lock();
		{
			CJabberSDNode *pNode = g_SDManager.GetPrimaryNode();
			if ( pNode->GetJid() ) {
				int nNodeCount = SendDlgItemMessage( hwndDlg, IDC_TREE_DISCO, TVM_GETCOUNT, 0, 0 );
				if ( !nNodeCount ) {
					TVINSERTSTRUCT tvi;
					ZeroMemory( &tvi, sizeof( tvi ));
					tvi.hParent = TVI_ROOT;
					tvi.hInsertAfter = TVI_ROOT;
					tvi.itemex.mask = TVIF_PARAM | TVIF_TEXT | TVIF_STATE;
					tvi.itemex.state = TVIS_EXPANDED;
					tvi.itemex.lParam = (LPARAM)pNode;
					tvi.itemex.pszText = pNode->GetName() ? pNode->GetName() : pNode->GetJid();
					LRESULT lIndex = SendDlgItemMessage( hwndDlg, IDC_TREE_DISCO, TVM_INSERTITEM, 0, (LPARAM)&tvi );
					pNode->SetTreeItemHandle( (HTREEITEM)lIndex );
			}	}

			SyncTree( TVI_ROOT, pNode );
		}
		g_SDManager.Unlock();
		SendDlgItemMessage( hwndDlg, IDC_TREE_DISCO, WM_SETREDRAW, TRUE, 0 );
		RedrawWindow( GetDlgItem( hwndDlg, IDC_TREE_DISCO ), NULL, NULL, RDW_INVALIDATE );
		return TRUE;

	case WM_NOTIFY:
		if ( wParam == IDC_TREE_DISCO ) {
			NMHDR *pHeader = (NMHDR *)lParam;
			if ( pHeader->code == TVN_SELCHANGED ) {
				NMTREEVIEW *pNmTreeView = (NMTREEVIEW *)lParam;
				g_SDManager.Lock();
				CJabberSDNode *pNode = (CJabberSDNode *)pNmTreeView->itemNew.lParam;
				if ( pNode && !pNode->GetInfoRequestId() )
					SendBothRequests( pNode );
				g_SDManager.Unlock();
			}
			else if ( pHeader->code == TVN_GETINFOTIP ) {
				NMTVGETINFOTIP *pInfoTip = (NMTVGETINFOTIP *)lParam;
				g_SDManager.Lock();
				CJabberSDNode *pNode = (CJabberSDNode *)pInfoTip->lParam;
				if ( pNode ) {
					pNode->GetTooltipText( pInfoTip->pszText, pInfoTip->cchTextMax );
				}
				g_SDManager.Unlock();
			}
			else if ( pHeader->code == TVN_ITEMEXPANDED ) {
				NMTREEVIEW *pNmTreeView = (NMTREEVIEW *)lParam;
				g_SDManager.Lock();
				LPARAM lChild = SendDlgItemMessage( hwndDlg, IDC_TREE_DISCO, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)pNmTreeView->itemNew.hItem );
				while ( lChild ) {
					TVITEM tvi;
					ZeroMemory( &tvi, sizeof( tvi ));
					tvi.mask = TVIF_HANDLE | TVIF_PARAM;
					tvi.hItem = (HTREEITEM)lChild;

					if ( SendDlgItemMessage( hwndDlg, IDC_TREE_DISCO, TVM_GETITEM, 0, (LPARAM)&tvi )) {
						CJabberSDNode *pNode = (CJabberSDNode *)tvi.lParam;
						if ( pNode )
							SendBothRequests( pNode );
					}
					lChild = SendDlgItemMessage( hwndDlg, IDC_TREE_DISCO, TVM_GETNEXTITEM, TVGN_NEXT, lChild );
				}
				g_SDManager.Unlock();
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
				SendDlgItemMessage( hwndDlg, IDC_TREE_DISCO, TVM_DELETEITEM, 0, 0 );

				g_SDManager.Lock();

				CJabberSDNode *pNode = g_SDManager.GetPrimaryNode();

				pNode->RemoveAll();
				pNode->SetJid( szJid );
				pNode->SetNode( _tcslen( szNode ) ? szNode : NULL );

				SendBothRequests( pNode );

				g_SDManager.Unlock();

				PostMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
			}
			return TRUE;
		}
		return FALSE;

	case WM_CLOSE:
		DestroyWindow( hwndDlg );
		return TRUE;

	case WM_DESTROY:
		hwndServiceDiscovery = NULL;
		g_SDManager.Lock();
		g_SDManager.GetPrimaryNode()->RemoveAll();
		g_SDManager.Unlock();
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
