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

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_privacy.cpp,v $
Revision       : $Revision: 5337 $
Last change on : $Date: 2007-04-28 13:26:31 +0300 (бс, 28 ря№ 2007) $
Last change by : $Author: ghazan $

*/

#include "jabber.h"
#include <commctrl.h>
#include "resource.h"
#include "jabber_iq.h"
#include "jabber_privacy.h"

CPrivacyListManager g_PrivacyListManager;

void JabberProcessIqPrivacyLists( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_SET ) {
		XmlNodeIq iq( "result", pInfo );
		jabberThreadInfo->send( iq );
}	}

void JabberIqResultPrivacyListModify( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if ( !pInfo->m_pUserData )
		return;

	CPrivacyListModifyUserParam *pParam = ( CPrivacyListModifyUserParam * )pInfo->m_pUserData;

	if ( pInfo->m_nIqType != JABBER_IQ_TYPE_RESULT )
		pParam->m_bAllOk = FALSE;

	if ( !g_JabberIqManager.GetGroupPendingIqCount( pInfo->m_dwGroupId )) {
		TCHAR szText[ 512 ];
		if ( !pParam->m_bAllOk )
			mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Error occurred while applying changes"));
		else
			mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Privacy lists successfully saved"));
		SetDlgItemText( hwndPrivacyLists, IDC_STATIC_LISTS_LABEL, szText );
		// FIXME: enable apply button
		delete pParam;
	}
}

void JabberIqResultPrivacyList( XmlNode* iqNode, void* userdata )
{
	if ( !iqNode )
		return;

	TCHAR *type = JabberXmlGetAttrValue( iqNode, "type" );
	if ( !type )
		return;

	if ( !_tcscmp( type, _T("result"))) {
		XmlNode *query = JabberXmlGetChild( iqNode, "query" );
		if ( !query )
			return;
		XmlNode *list = JabberXmlGetChild( query, "list" );
		if ( !list )
			return;
		TCHAR *szListName = JabberXmlGetAttrValue( list, "name" );
		if ( !szListName )
			return;
		g_PrivacyListManager.Lock();
		CPrivacyList* pList = g_PrivacyListManager.FindList( szListName );
		if ( !pList ) {
			g_PrivacyListManager.AddList( szListName );
			pList = g_PrivacyListManager.FindList( szListName );
			if ( !pList ) {
				g_PrivacyListManager.Unlock();
				return;
		}	}

		XmlNode* item;
		for ( int i = 1; ( item = JabberXmlGetNthChild( list, "item", i )) != NULL; i++ ) {
			TCHAR *itemType = JabberXmlGetAttrValue( item, "type" );
			PrivacyListRuleType nItemType = Else;
			if ( itemType ) {
				if ( !_tcsicmp( itemType, _T( "jid" )))
					nItemType = Jid;
				else if ( !_tcsicmp( itemType, _T( "group" )))
					nItemType = Group;
				else if ( !_tcsicmp( itemType, _T( "subscription" )))
					nItemType = Subscription;
			}

			TCHAR *itemValue = JabberXmlGetAttrValue( item, "value" );

			TCHAR *itemAction = JabberXmlGetAttrValue( item, "action" );
			BOOL bAllow = TRUE;
			if ( itemAction && !_tcsicmp( itemAction, _T( "deny" )))
				bAllow = FALSE;

			TCHAR *itemOrder = JabberXmlGetAttrValue( item, "order" );
			DWORD dwOrder = 0;
			if ( itemOrder )
				dwOrder = _ttoi( itemOrder );

			DWORD dwPackets = 0;
			if ( JabberXmlGetChild( item, "message" ))
				dwPackets |= JABBER_PL_RULE_TYPE_MESSAGE;
			if ( JabberXmlGetChild( item, "presence-in" ))
				dwPackets |= JABBER_PL_RULE_TYPE_PRESENCE_IN;
			if ( JabberXmlGetChild( item, "presence-out" ))
				dwPackets |= JABBER_PL_RULE_TYPE_PRESENCE_OUT;
			if ( JabberXmlGetChild( item, "iq" ))
				dwPackets |= JABBER_PL_RULE_TYPE_IQ;

			pList->AddRule( nItemType, itemValue, bAllow, dwOrder, dwPackets );
		}
		pList->Reorder();
		pList->SetLoaded();
		pList->SetModified(FALSE);
		g_PrivacyListManager.Unlock();

		if ( hwndPrivacyLists )
			SendMessage( hwndPrivacyLists, WM_JABBER_REFRESH, 0, 0);
}	}

CPrivacyList* GetSelectedList(HWND hDlg)
{
	int nCurSel = SendDlgItemMessage( hDlg, IDC_LB_LISTS, LB_GETCURSEL, 0, 0 );
	if ( nCurSel == CB_ERR )
		return NULL;

	int nItemData = SendDlgItemMessage( hDlg, IDC_LB_LISTS, LB_GETITEMDATA, nCurSel, 0 );
	if ( nItemData == CB_ERR || nItemData == 0 )
		return NULL;

	return ( CPrivacyList* )nItemData;
}

CPrivacyListRule* GetSelectedRule(HWND hDlg)
{
	int nCurSel = SendDlgItemMessage( hDlg, IDC_PL_RULES_LIST, LB_GETCURSEL, 0, 0 );
	if ( nCurSel == LB_ERR)
		return NULL;

	int nItemData = SendDlgItemMessage( hDlg, IDC_PL_RULES_LIST, LB_GETITEMDATA, nCurSel, 0 );
	if ( nItemData == LB_ERR || nItemData == 0 )
		return NULL;

	return (CPrivacyListRule* )nItemData;
}

void JabberIqResultPrivacyListActive( XmlNode* iqNode, void* userdata )
{
	if ( !hwndPrivacyLists )
		return;

	EnableWindow( GetDlgItem( hwndPrivacyLists, IDC_ACTIVATE ), TRUE );

	if ( !iqNode )
		return;

	TCHAR *type = JabberXmlGetAttrValue( iqNode, "type" );
	if ( !type )
		return;

	TCHAR szText[ 512 ];
	szText[0] = _T('\0');
	g_PrivacyListManager.Lock();
	if ( !_tcscmp( type, _T("result"))) {
		CPrivacyList* pList = (CPrivacyList*)GetWindowLong(GetDlgItem(hwndPrivacyLists, IDC_ACTIVATE), GWL_USERDATA);
		if ( pList ) {
			g_PrivacyListManager.SetActiveListName( pList->GetListName() );
			mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Privacy list %s set as active"), pList->GetListName() );
		}
		else {
			g_PrivacyListManager.SetActiveListName( NULL );
			mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Active privacy list successfully declined"));
		}
	}
	else {
		mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Error occurred while setting active list") );
	}
	SetDlgItemText( hwndPrivacyLists, IDC_STATIC_LISTS_LABEL, szText );
	g_PrivacyListManager.Unlock();

	RedrawWindow(GetDlgItem(hwndPrivacyLists, IDC_LB_LISTS), NULL, NULL, RDW_INVALIDATE);
}

void JabberIqResultPrivacyListDefault( XmlNode* iqNode, void* userdata )
{
	if ( !hwndPrivacyLists )
		return;

	EnableWindow( GetDlgItem( hwndPrivacyLists, IDC_SET_DEFAULT ), TRUE );

	if ( !iqNode )
		return;

	TCHAR *type = JabberXmlGetAttrValue( iqNode, "type" );
	if ( !type )
		return;

	TCHAR szText[ 512 ];
	szText[0] = _T('\0');
	g_PrivacyListManager.Lock();
	if ( !_tcscmp( type, _T("result"))) {
		CPrivacyList* pList = (CPrivacyList*)GetWindowLong(GetDlgItem(hwndPrivacyLists, IDC_SET_DEFAULT), GWL_USERDATA);
		if ( pList ) {
			g_PrivacyListManager.SetDefaultListName( pList->GetListName() );
			mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Privacy list %s set as default"), pList->GetListName() );
		}
		else {
			g_PrivacyListManager.SetDefaultListName( NULL );
			mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Default privacy list successfully declined"));
		}
	}
	else {
		mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Error occurred while setting default list") );
	}
	SetDlgItemText( hwndPrivacyLists, IDC_STATIC_LISTS_LABEL, szText );
	g_PrivacyListManager.Unlock();

	RedrawWindow(GetDlgItem(hwndPrivacyLists, IDC_LB_LISTS), NULL, NULL, RDW_INVALIDATE);
}

void JabberIqResultPrivacyLists( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if ( pInfo->m_nIqType != JABBER_IQ_TYPE_RESULT )
		return;

	XmlNode *query = JabberXmlGetChild( iqNode, "query" );
	if ( !query )
		return;

	if ( jabberThreadInfo )
		jabberThreadInfo->jabberServerCaps |= JABBER_CAPS_PRIVACY_LISTS;

	if ( !hwndPrivacyLists )
		return;

	g_PrivacyListManager.Lock();
	g_PrivacyListManager.RemoveAllLists();

	XmlNode *list;
	for ( int i = 1; ( list = JabberXmlGetNthChild( query, "list", i )) != NULL; i++ ) {
		TCHAR *listName = JabberXmlGetAttrValue( list, "name" );
		if ( listName ) {
			g_PrivacyListManager.AddList(listName);

			int iqId = JabberSerialNext();
			JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultPrivacyList);
			XmlNodeIq iq( "get", iqId);
			XmlNode* queryNode = iq.addQuery( JABBER_FEAT_PRIVACY_LISTS );
			XmlNode* listNode = queryNode->addChild( "list" );
			listNode->addAttr( "name", listName );
			jabberThreadInfo->send( iq );
	}	}

	TCHAR *szName = NULL;
	XmlNode *node = JabberXmlGetChild( query, "active" );
	if ( node )
		szName = JabberXmlGetAttrValue( node, "name" );
	g_PrivacyListManager.SetActiveListName( szName );

	szName = NULL;
	node = JabberXmlGetChild( query, "default" );
	if ( node )
		szName = JabberXmlGetAttrValue( node, "name" );
	g_PrivacyListManager.SetDefaultListName( szName );

	g_PrivacyListManager.Unlock();

	SendMessage( hwndPrivacyLists, WM_JABBER_REFRESH, 0, 0);
}

static BOOL CALLBACK JabberPrivacyAddListDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
		hwndPrivacyRule = hwndDlg;
		TranslateDialogDefault( hwndDlg );
		SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG )lParam );
		return TRUE;

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDOK:
			{
				TCHAR *szText = ( TCHAR * )GetWindowLong( hwndDlg, GWL_USERDATA );
				// FIXME: hard coded line length
				GetDlgItemText( hwndDlg, IDC_EDIT_NAME, szText, 511 );
				EndDialog( hwndDlg, 1 );
			}
			return TRUE;

		case IDCANCEL:
			EndDialog( hwndDlg, 0 );
			return TRUE;
		}
		return FALSE;

	case WM_CLOSE:
		EndDialog( hwndDlg, 0 );
		return TRUE;

	case WM_DESTROY:
		hwndPrivacyRule = NULL;
		break;
	}
	return FALSE;
}

static BOOL CALLBACK JabberPrivacyRuleDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
		{
			hwndPrivacyRule = hwndDlg;
			TranslateDialogDefault( hwndDlg );

			SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG )lParam );

			CPrivacyListRule* pRule = (CPrivacyListRule* )lParam;

			TCHAR* szTypes[] = { _T("JID"), _T("Group"), _T("Subscription"), _T("Any") };
			int i, nTypes[] = { Jid, Group, Subscription, Else };
			for ( i = 0; i < SIZEOF(szTypes); i++ ) {
				int nItem = SendDlgItemMessage( hwndDlg, IDC_COMBO_TYPE, CB_ADDSTRING, 0, (LPARAM)TranslateTS( szTypes[i] ));
				SendDlgItemMessage( hwndDlg, IDC_COMBO_TYPE, CB_SETITEMDATA, nItem, nTypes[i] );
				if ( pRule->GetType() == nTypes[i] )
					SendDlgItemMessage( hwndDlg, IDC_COMBO_TYPE, CB_SETCURSEL, nItem, 0 );
			}

			SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUE, CB_RESETCONTENT, 0, 0 );
			TCHAR* szSubscriptions[] = { _T("none"), _T("from"), _T("to"), _T("both") };
			for ( i = 0; i < SIZEOF(szSubscriptions); i++ ) {
				int nItem = SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUE, CB_ADDSTRING, 0, (LPARAM)TranslateTS( szSubscriptions[i] ));
				SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUE, CB_SETITEMDATA, nItem, (LPARAM)szSubscriptions[i] );
			}

			PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_COMBO_TYPE, CBN_SELCHANGE ), 0 );

			SendDlgItemMessage( hwndDlg, IDC_COMBO_ACTION, CB_ADDSTRING, 0, (LPARAM)TranslateTS( _T("Deny" )));
			SendDlgItemMessage( hwndDlg, IDC_COMBO_ACTION, CB_ADDSTRING, 0, (LPARAM)TranslateTS( _T("Allow" )));

			SendDlgItemMessage( hwndDlg, IDC_COMBO_ACTION, CB_SETCURSEL, pRule->GetAction() ? 1 : 0, 0 );

			DWORD dwPackets = pRule->GetPackets();
			if ( !dwPackets )
				dwPackets = JABBER_PL_RULE_TYPE_ALL;
			if ( dwPackets & JABBER_PL_RULE_TYPE_IQ )
				SendDlgItemMessage( hwndDlg, IDC_CHECK_QUERIES, BM_SETCHECK, BST_CHECKED, 0 );
			if ( dwPackets & JABBER_PL_RULE_TYPE_MESSAGE )
				SendDlgItemMessage( hwndDlg, IDC_CHECK_MESSAGES, BM_SETCHECK, BST_CHECKED, 0 );
			if ( dwPackets & JABBER_PL_RULE_TYPE_PRESENCE_IN )
				SendDlgItemMessage( hwndDlg, IDC_CHECK_PRESENCE_IN, BM_SETCHECK, BST_CHECKED, 0 );
			if ( dwPackets & JABBER_PL_RULE_TYPE_PRESENCE_OUT )
				SendDlgItemMessage( hwndDlg, IDC_CHECK_PRESENCE_OUT, BM_SETCHECK, BST_CHECKED, 0 );

			if ( pRule->GetValue() && ( pRule->GetType() == Jid || pRule->GetType() == Group ))
				SetDlgItemText( hwndDlg, IDC_EDIT_VALUE, pRule->GetValue() );

			return TRUE;
		}

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_COMBO_TYPE:
			if ( HIWORD( wParam ) == CBN_SELCHANGE ) {
				CPrivacyListRule* pRule = (CPrivacyListRule* )GetWindowLong( hwndDlg, GWL_USERDATA );
				if ( !pRule )
					return TRUE;

				int nCurSel = SendDlgItemMessage( hwndDlg, IDC_COMBO_TYPE, CB_GETCURSEL, 0, 0 );
				if ( nCurSel == CB_ERR )
					return TRUE;

				int nItemData = SendDlgItemMessage( hwndDlg, IDC_COMBO_TYPE, CB_GETITEMDATA, nCurSel, 0 );
				switch (nItemData) {
				case Jid:
					{
						ShowWindow( GetDlgItem( hwndDlg, IDC_COMBO_VALUES ), SW_SHOW );
						ShowWindow( GetDlgItem( hwndDlg, IDC_COMBO_VALUE ), SW_HIDE );

						SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUES, CB_RESETCONTENT, 0, 0 );

						HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
						while ( hContact != NULL ) {
							char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
							if ( szProto != NULL && !strcmp( szProto, jabberProtoName )) {
								DBVARIANT dbv;
								if ( !JGetStringT( hContact, "jid", &dbv )) {
									SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUES, CB_ADDSTRING, 0, (LPARAM)dbv.ptszVal );
									JFreeVariant( &dbv );
								}
							}
							hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
						}

						// FIXME: ugly code :)
						if ( pRule->GetValue() ) {
							SetDlgItemText( hwndDlg, IDC_COMBO_VALUES, pRule->GetValue() );
							int nSelPos = SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUES, CB_FINDSTRINGEXACT , -1, (LPARAM)pRule->GetValue() );
							if ( nSelPos != CB_ERR )
								SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUES, CB_SETCURSEL, nSelPos, 0 );
						}
						break;
					}

				case Group:
					{
						ShowWindow( GetDlgItem( hwndDlg, IDC_COMBO_VALUES ), SW_SHOW );
						ShowWindow( GetDlgItem( hwndDlg, IDC_COMBO_VALUE ), SW_HIDE );

						SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUES, CB_RESETCONTENT, 0, 0 );

						char buf[ 20 ];
						DBVARIANT dbv;
						for ( int i = 0; ; i++ ) {
							mir_snprintf(buf, 20, "%d", i);
							if ( DBGetContactSettingTString(NULL, "CListGroups", buf, &dbv) )
								break;

							SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUES, CB_ADDSTRING, 0, (LPARAM)&dbv.ptszVal[1] );
							DBFreeVariant(&dbv);
						}

						// FIXME: ugly code :)
						if ( pRule->GetValue() ) {
							SetDlgItemText( hwndDlg, IDC_COMBO_VALUES, pRule->GetValue() );
							int nSelPos = SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUES, CB_FINDSTRINGEXACT , -1, (LPARAM)pRule->GetValue() );
							if ( nSelPos != CB_ERR )
								SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUES, CB_SETCURSEL, nSelPos, 0 );
						}
						break;
					}

				case Subscription:
					ShowWindow( GetDlgItem( hwndDlg, IDC_COMBO_VALUES ), SW_HIDE );
					ShowWindow( GetDlgItem( hwndDlg, IDC_COMBO_VALUE ), SW_SHOW );

					if ( pRule->GetValue() ) {
						int nSelected = SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUE, CB_SELECTSTRING, -1, (LPARAM)TranslateTS(pRule->GetValue()) );
						if ( nSelected == CB_ERR )
							SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUE, CB_SETCURSEL, 0, 0 );
					}
					else SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUE, CB_SETCURSEL, 0, 0 );
					break;

				case Else:
					ShowWindow( GetDlgItem( hwndDlg, IDC_COMBO_VALUES ), SW_HIDE );
					ShowWindow( GetDlgItem( hwndDlg, IDC_COMBO_VALUE ), SW_HIDE );
					break;
			}	}
			return TRUE;

		case IDOK:
			{
				CPrivacyListRule* pRule = (CPrivacyListRule* )GetWindowLong( hwndDlg, GWL_USERDATA );
				int nItemData = -1;
				int nCurSel = SendDlgItemMessage( hwndDlg, IDC_COMBO_TYPE, CB_GETCURSEL, 0, 0 );
				if ( nCurSel != CB_ERR )
					nItemData = SendDlgItemMessage( hwndDlg, IDC_COMBO_TYPE, CB_GETITEMDATA, nCurSel, 0 );

				switch ( nItemData ) {
				case Jid:
				case Group:
					{
						TCHAR szText[ 512 ];
						GetDlgItemText( hwndDlg, IDC_COMBO_VALUES, szText, SIZEOF(szText) );
						pRule->SetValue( szText );
					}
					break;

				case Subscription:
					{
						nCurSel = SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUE, CB_GETCURSEL, 0, 0 );
						TCHAR *szSubs = NULL;
						if ( nCurSel != CB_ERR ) {
							pRule->SetValue( ( TCHAR * )SendDlgItemMessage( hwndDlg, IDC_COMBO_VALUE, CB_GETITEMDATA, nCurSel, 0 ));
						}
						else
							pRule->SetValue( _T( "none" ));
					}
					break;

				default:
					pRule->SetValue( NULL );
					break;
				}
				pRule->SetType( ( PrivacyListRuleType )nItemData );
				nCurSel = SendDlgItemMessage( hwndDlg, IDC_COMBO_ACTION, CB_GETCURSEL, 0, 0 );
				if ( nCurSel == CB_ERR )
					nCurSel = 1;
				pRule->SetAction( nCurSel ? TRUE : FALSE );

				DWORD dwPackets = 0;
				if ( BST_CHECKED == SendDlgItemMessage( hwndDlg, IDC_CHECK_MESSAGES, BM_GETCHECK, 0, 0 ))
					dwPackets |= JABBER_PL_RULE_TYPE_MESSAGE;
				if ( BST_CHECKED == SendDlgItemMessage( hwndDlg, IDC_CHECK_PRESENCE_IN, BM_GETCHECK, 0, 0 ))
					dwPackets |= JABBER_PL_RULE_TYPE_PRESENCE_IN;
				if ( BST_CHECKED == SendDlgItemMessage( hwndDlg, IDC_CHECK_PRESENCE_OUT, BM_GETCHECK, 0, 0 ))
					dwPackets |= JABBER_PL_RULE_TYPE_PRESENCE_OUT;
				if ( BST_CHECKED == SendDlgItemMessage( hwndDlg, IDC_CHECK_QUERIES, BM_GETCHECK, 0, 0 ))
					dwPackets |= JABBER_PL_RULE_TYPE_IQ;
				if ( !dwPackets )
					dwPackets = JABBER_PL_RULE_TYPE_ALL;

				pRule->SetPackets( dwPackets );

				EndDialog( hwndDlg, 1 );
			}
			return TRUE;

		case IDCANCEL:
			EndDialog( hwndDlg, 0 );
			return TRUE;
		}
		return FALSE;

	case WM_CLOSE:
		EndDialog( hwndDlg, 0 );
		return TRUE;

	case WM_DESTROY:
		hwndPrivacyRule = NULL;
		break;
	}
	return FALSE;
}

BOOL JabberPrivacyListDlgCanExit( HWND hDlg )
{
	g_PrivacyListManager.Lock();
	BOOL bModified = g_PrivacyListManager.IsModified();
	g_PrivacyListManager.Unlock();
	
	if ( !bModified )
		return TRUE;

	if ( IDYES == MessageBox( hDlg, TranslateT("Privacy lists are not saved, discard any changes and exit?"), TranslateT("Are you sure?"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2 ))
		return TRUE;

	return FALSE;
}

static void sttDrawNextRulePart(HDC hdc, COLORREF color, TCHAR *text, RECT *rc)
{
	SetTextColor(hdc, color);
	DrawText(hdc, text, lstrlen(text), rc, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER|DT_WORD_ELLIPSIS);

	SIZE sz;
	GetTextExtentPoint32(hdc, text, lstrlen(text), &sz);
	rc->left += sz.cx;
}

static void sttDrawRuleAction(HDC hdc, COLORREF clLine1, COLORREF clLine2, CPrivacyListRule *pRule, RECT *rc)
{
	sttDrawNextRulePart(hdc, clLine1, pRule->GetAction() ? TranslateT("allow ") : TranslateT("deny "), rc);
	if (!pRule->GetPackets() || (pRule->GetPackets() == JABBER_PL_RULE_TYPE_ALL))
	{
		sttDrawNextRulePart(hdc, clLine1, TranslateT("all."), rc);
	} else
	{
		bool needComma = false;
		int itemCount = 
			((pRule->GetPackets() & JABBER_PL_RULE_TYPE_MESSAGE) ? 1 : 0) +
			((pRule->GetPackets() & JABBER_PL_RULE_TYPE_PRESENCE_IN) ? 1 : 0) +
			((pRule->GetPackets() & JABBER_PL_RULE_TYPE_PRESENCE_OUT) ? 1 : 0) +
			((pRule->GetPackets() & JABBER_PL_RULE_TYPE_IQ) ? 1 : 0);

		if (pRule->GetPackets() & JABBER_PL_RULE_TYPE_MESSAGE)
		{
			--itemCount;
			needComma = true;
			sttDrawNextRulePart(hdc, clLine1, TranslateT("messages"), rc);
		}
		if (pRule->GetPackets() & JABBER_PL_RULE_TYPE_PRESENCE_IN)
		{
			--itemCount;
			if (needComma)
				sttDrawNextRulePart(hdc, clLine1, itemCount ? _T(", ") : TranslateT(" and "), rc);
			needComma = true;
			sttDrawNextRulePart(hdc, clLine1, TranslateT("incoming presences"), rc);
		}
		if (pRule->GetPackets() & JABBER_PL_RULE_TYPE_PRESENCE_OUT)
		{
			--itemCount;
			if (needComma)
				sttDrawNextRulePart(hdc, clLine1, itemCount ? _T(", ") : TranslateT(" and "), rc);
			needComma = true;
			sttDrawNextRulePart(hdc, clLine1, TranslateT("outgoing presences"), rc);
		}
		if (pRule->GetPackets() & JABBER_PL_RULE_TYPE_IQ)
		{
			--itemCount;
			if (needComma)
				sttDrawNextRulePart(hdc, clLine1, itemCount ? _T(", ") : TranslateT(" and "), rc);
			needComma = true;
			sttDrawNextRulePart(hdc, clLine1, TranslateT("queries"), rc);
		}
		sttDrawNextRulePart(hdc, clLine1, _T("."), rc);
	}
}

static void sttDrawRulesList(HWND hwndDlg, LPDRAWITEMSTRUCT lpdis)
{
	if (lpdis->itemID == -1)
		return;

	CPrivacyListRule *pRule = (CPrivacyListRule *)lpdis->itemData;

	COLORREF clLine1, clLine2, clBack;
	if (lpdis->itemState & ODS_SELECTED)
	{
		FillRect(lpdis->hDC, &lpdis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
		clBack = GetSysColor(COLOR_HIGHLIGHT);
		clLine1 = GetSysColor(COLOR_HIGHLIGHTTEXT);
	} else
	{
		FillRect(lpdis->hDC, &lpdis->rcItem, GetSysColorBrush(COLOR_WINDOW));
		clBack = GetSysColor(COLOR_WINDOW);
		clLine1 = GetSysColor(COLOR_WINDOWTEXT);
	}
	clLine2 = RGB(
			GetRValue(clLine1) * 0.66 + GetRValue(clBack) * 0.34,
			GetGValue(clLine1) * 0.66 + GetGValue(clBack) * 0.34,
			GetBValue(clLine1) * 0.66 + GetBValue(clBack) * 0.34
		);

	SetBkMode(lpdis->hDC, TRANSPARENT);

	RECT rc;

	if (!pRule)
	{
		rc = lpdis->rcItem;
		rc.left += 25;

		int len = SendDlgItemMessage(hwndDlg, lpdis->CtlID, LB_GETTEXTLEN, lpdis->itemID, 0) + 1;
		TCHAR *str = (TCHAR *)_alloca(len * sizeof(TCHAR));
		SendDlgItemMessage(hwndDlg, lpdis->CtlID, LB_GETTEXT, lpdis->itemID, (LPARAM)str);
		sttDrawNextRulePart(lpdis->hDC, clLine1, str, &rc);
	} else
	if (pRule->GetType() == Else)
	{
		rc = lpdis->rcItem;
		rc.left += 25;

		sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT("Else "), &rc);
		sttDrawRuleAction(lpdis->hDC, clLine1, clLine2, pRule, &rc);
	} else
	{
		rc = lpdis->rcItem;
		rc.bottom -= (rc.bottom - rc.top) / 2;
		rc.left += 25;

		switch ( pRule->GetType() )
		{
			case Jid:
			{
				sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT("If jabber id is '"), &rc);
				sttDrawNextRulePart(lpdis->hDC, clLine1, pRule->GetValue(), &rc);
				sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT("'"), &rc);
				
				if (HANDLE hContact = JabberHContactFromJID(pRule->GetValue()))
				{
					TCHAR *szName = (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR);
					if (szName)
					{
						sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT(" (nickname: "), &rc);
						sttDrawNextRulePart(lpdis->hDC, clLine1, szName, &rc);
						sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT(")"), &rc);
					}
				}
				break;
			}

			case Group:
				sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT("If group is '"), &rc);
				sttDrawNextRulePart(lpdis->hDC, clLine1, pRule->GetValue(), &rc);
				sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT("'"), &rc);
				break;
			case Subscription:
				sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT("If subscription is '"), &rc);
				sttDrawNextRulePart(lpdis->hDC, clLine1, pRule->GetValue(), &rc);
				sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT("'"), &rc);
				break;
		}

		rc = lpdis->rcItem;
		rc.top += (rc.bottom - rc.top) / 2;
		rc.left += 25;

		sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT("then "), &rc);
		sttDrawRuleAction(lpdis->hDC, clLine1, clLine2, pRule, &rc);
	}

	DrawIconEx(lpdis->hDC, lpdis->rcItem.left+4, (lpdis->rcItem.top+lpdis->rcItem.bottom-16)/2,
		LoadIconEx("main"),
		16, 16, 0, NULL, DI_NORMAL);

	if (pRule)
	{
		DrawIconEx(lpdis->hDC, lpdis->rcItem.left+4, (lpdis->rcItem.top+lpdis->rcItem.bottom-16)/2,
			LoadIconEx(pRule->GetAction() ? "disco_ok" : "disco_fail"),
			16, 16, 0, NULL, DI_NORMAL);
	}

	if (lpdis->itemState & ODS_FOCUS)
	{
		int sel = SendDlgItemMessage(hwndDlg, lpdis->CtlID, LB_GETCURSEL, 0, 0);
		if ((sel == LB_ERR) || (sel == lpdis->itemID))
			DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
	}
}

static void sttDrawLists(HWND hwndDlg, LPDRAWITEMSTRUCT lpdis)
{
	CPrivacyList *pList = (CPrivacyList *)lpdis->itemData;

	COLORREF clLine1, clLine2, clBack;
	if (lpdis->itemState & ODS_SELECTED)
	{
		FillRect(lpdis->hDC, &lpdis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
		clBack = GetSysColor(COLOR_HIGHLIGHT);
		clLine1 = GetSysColor(COLOR_HIGHLIGHTTEXT);
	} else
	{
		FillRect(lpdis->hDC, &lpdis->rcItem, GetSysColorBrush(COLOR_WINDOW));
		clBack = GetSysColor(COLOR_WINDOW);
		clLine1 = GetSysColor(COLOR_WINDOWTEXT);
	}
	clLine2 = RGB(
			GetRValue(clLine1) * 0.66 + GetRValue(clBack) * 0.34,
			GetGValue(clLine1) * 0.66 + GetGValue(clBack) * 0.34,
			GetBValue(clLine1) * 0.66 + GetBValue(clBack) * 0.34
		);

	SetBkMode(lpdis->hDC, TRANSPARENT);

	RECT rc;

	g_PrivacyListManager.Lock();
	TCHAR *szDefault = NEWTSTR_ALLOCA(g_PrivacyListManager.GetDefaultListName());
	TCHAR *szActive = NEWTSTR_ALLOCA(g_PrivacyListManager.GetActiveListName());
	g_PrivacyListManager.Unlock();

	HFONT hfnt = NULL;

	rc = lpdis->rcItem;
	rc.left +=3;

	if (!pList)
	{
		if (!szActive)
		{
			LOGFONT lf;
			GetObject(GetCurrentObject(lpdis->hDC, OBJ_FONT), sizeof(lf), &lf);
			lf.lfWeight = FW_BOLD;
			hfnt = (HFONT)SelectObject(lpdis->hDC, CreateFontIndirect(&lf));
		}

		int len = SendDlgItemMessage(hwndDlg, lpdis->CtlID, LB_GETTEXTLEN, lpdis->itemID, 0) + 1;
		TCHAR *str = (TCHAR *)_alloca(len * sizeof(TCHAR));
		SendDlgItemMessage(hwndDlg, lpdis->CtlID, LB_GETTEXT, lpdis->itemID, (LPARAM)str);
		sttDrawNextRulePart(lpdis->hDC, clLine1, str, &rc);

		if (!szActive && !szDefault)
			sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT(" (act., def.)"), &rc);
		else if (!szActive)
			sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT(" (active)"), &rc);
		else if (!szDefault)
			sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT(" (default)"), &rc);

		if (!szDefault)
			DrawIconEx(lpdis->hDC, lpdis->rcItem.right-16-4, (lpdis->rcItem.top+lpdis->rcItem.bottom-16)/2,
				LoadSkinnedIcon(SKINICON_OTHER_EMPTYBLOB),
				16, 16, 0, NULL, DI_NORMAL);
	} else
	{
		if (!lstrcmp(pList->GetListName(), szActive))
		{
			LOGFONT lf;
			GetObject(GetCurrentObject(lpdis->hDC, OBJ_FONT), sizeof(lf), &lf);
			lf.lfWeight = FW_BOLD;
			hfnt = (HFONT)SelectObject(lpdis->hDC, CreateFontIndirect(&lf));
		}

		sttDrawNextRulePart(lpdis->hDC, clLine1, pList->GetListName(), &rc);

		if (!lstrcmp(pList->GetListName(), szActive) && !lstrcmp(pList->GetListName(), szDefault))
			sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT(" (act., def.)"), &rc);
		else if (!lstrcmp(pList->GetListName(), szActive))
			sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT(" (active)"), &rc);
		else if (!lstrcmp(pList->GetListName(), szDefault))
			sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT(" (default)"), &rc);

		if (!lstrcmp(pList->GetListName(), szDefault))
			DrawIconEx(lpdis->hDC, lpdis->rcItem.right-16-4, (lpdis->rcItem.top+lpdis->rcItem.bottom-16)/2,
				LoadSkinnedIcon(SKINICON_OTHER_EMPTYBLOB),
				16, 16, 0, NULL, DI_NORMAL);
	}

	if (hfnt)
	{
		DeleteObject(SelectObject(lpdis->hDC, hfnt));
	}

	if (lpdis->itemState & ODS_FOCUS)
	{
		int sel = SendDlgItemMessage(hwndDlg, lpdis->CtlID, LB_GETCURSEL, 0, 0);
		if ((sel == LB_ERR) || (sel == lpdis->itemID))
			DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
	}
}

static int sttPrivacyListsResizer(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL *urc)
{
	switch (urc->wId)
	{
	case IDC_WHITERECT:
	case IDC_TITLE:
	case IDC_DESCRIPTION:
	case IDC_FRAME1:
		return RD_ANCHORX_LEFT|RD_ANCHORY_TOP|RD_ANCHORX_WIDTH;
	case IDC_LB_LISTS:
		return RD_ANCHORX_LEFT|RD_ANCHORY_TOP|RD_ANCHORY_HEIGHT;
	case IDC_PL_RULES_LIST:
		return RD_ANCHORX_LEFT|RD_ANCHORY_TOP|RD_ANCHORY_HEIGHT|RD_ANCHORX_WIDTH;
	case IDC_STATIC_LISTS_LABEL:
		return RD_ANCHORX_RIGHT|RD_ANCHORY_TOP|RD_ANCHORX_WIDTH;
	case IDC_ADD_LIST:
	case IDC_ACTIVATE:
	case IDC_REMOVE_LIST:
	case IDC_SET_DEFAULT:
	case IDC_ADD_RULE:
	case IDC_UP_RULE:
	case IDC_EDIT_RULE:
	case IDC_DOWN_RULE:
	case IDC_REMOVE_RULE:
		return RD_ANCHORX_LEFT|RD_ANCHORY_BOTTOM;
	case IDC_APPLY:
	case IDCANCEL:
		return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
	}
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
}

BOOL CALLBACK JabberPrivacyListsDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
		TranslateDialogDefault( hwndDlg );
		SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )LoadIconEx( "privacylists" ));
		hwndPrivacyLists = hwndDlg;

		EnableWindow( GetDlgItem( hwndDlg, IDC_ADD_RULE ), FALSE );
		EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT_RULE ), FALSE );
		EnableWindow( GetDlgItem( hwndDlg, IDC_REMOVE_RULE ), FALSE );
		EnableWindow( GetDlgItem( hwndDlg, IDC_UP_RULE ), FALSE );
		EnableWindow( GetDlgItem( hwndDlg, IDC_DOWN_RULE ), FALSE );
		{
			XmlNodeIq iq( g_JabberIqManager.AddHandler( JabberIqResultPrivacyLists ));
			XmlNode* query = iq.addQuery( JABBER_FEAT_PRIVACY_LISTS );
			jabberThreadInfo->send( iq );
		}

		{
			LOGFONT lf;
			GetObject((HFONT)SendDlgItemMessage(hwndDlg, IDC_TXT_LISTS, WM_GETFONT, 0, 0), sizeof(lf), &lf);
			lf.lfWeight = FW_BOLD;
			HFONT hfnt = CreateFontIndirect(&lf);
			SendDlgItemMessage(hwndDlg, IDC_TXT_LISTS, WM_SETFONT, (WPARAM)hfnt, TRUE);
			SendDlgItemMessage(hwndDlg, IDC_TXT_RULES, WM_SETFONT, (WPARAM)hfnt, TRUE);
			SendDlgItemMessage(hwndDlg, IDC_TITLE, WM_SETFONT, (WPARAM)hfnt, TRUE);
		}

		Utils_RestoreWindowPosition(hwndDlg, NULL, jabberProtoName, "plistsWnd_sz");

		return TRUE;

	case WM_JABBER_REFRESH:
		{
			int sel = SendDlgItemMessage(hwndDlg, IDC_LB_LISTS, LB_GETCURSEL, 0, 0);
			int len = SendDlgItemMessage(hwndDlg, IDC_LB_LISTS, LB_GETTEXTLEN, sel, 0) + 1;
			TCHAR *szCurrentSelectedList = (TCHAR *)_alloca(len * sizeof(TCHAR));
			SendDlgItemMessage(hwndDlg, IDC_LB_LISTS, LB_GETTEXT, sel, (LPARAM)szCurrentSelectedList);

			SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_RESETCONTENT, 0, 0 );

			int nItemId;

			nItemId = SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_ADDSTRING, 0, (LPARAM)TranslateT( "<none>" ));
			SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_SETITEMDATA, nItemId, (LPARAM)NULL );

			g_PrivacyListManager.Lock();
			CPrivacyList* pList = g_PrivacyListManager.GetFirstList();
			while ( pList ) {
				if ( !pList->IsDeleted() ) {
					nItemId = SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_ADDSTRING, 0, (LPARAM)pList->GetListName() );
					SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_SETITEMDATA, nItemId, (LPARAM)pList );
				}
				pList = pList->GetNext();
			}

			if ( SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_SELECTSTRING, -1, (LPARAM)szCurrentSelectedList ) == LB_ERR )
				SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_SETCURSEL, 0, 0 );

			g_PrivacyListManager.Unlock();

			PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_LB_LISTS, LBN_SELCHANGE ), 0 );
			PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_PL_RULES_LIST, LBN_SELCHANGE ), 0 );
		}
		return TRUE;

	case WM_MEASUREITEM:
	{
		LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
		if ((lpmis->CtlID != IDC_PL_RULES_LIST) && (lpmis->CtlID != IDC_LB_LISTS))
			break;

		TEXTMETRIC tm = {0};
		HDC hdc = GetDC(GetDlgItem(hwndDlg, lpmis->CtlID));
		GetTextMetrics(hdc, &tm);
		ReleaseDC(GetDlgItem(hwndDlg, lpmis->CtlID), hdc);

		if (lpmis->CtlID == IDC_PL_RULES_LIST)
			lpmis->itemHeight = tm.tmHeight * 2;
		else if (lpmis->CtlID == IDC_LB_LISTS)
			lpmis->itemHeight = tm.tmHeight;

		if (lpmis->itemHeight < 18) lpmis->itemHeight = 18;

		return TRUE;
	}

	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
		if ((lpdis->CtlID != IDC_PL_RULES_LIST) && (lpdis->CtlID != IDC_LB_LISTS))
			break;

		if (lpdis->CtlID == IDC_PL_RULES_LIST)
			sttDrawRulesList(hwndDlg, lpdis);
		else if (lpdis->CtlID == IDC_LB_LISTS)
			sttDrawLists(hwndDlg, lpdis);

		return TRUE;
	}

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_ACTIVATE:
			if ( jabberOnline ) {
				g_PrivacyListManager.Lock();
				EnableWindow( GetDlgItem( hwndDlg, IDC_ACTIVATE ), FALSE );
				CPrivacyList* pList = GetSelectedList(hwndDlg);
				SetWindowLong( GetDlgItem( hwndDlg, IDC_ACTIVATE ), GWL_USERDATA, (DWORD)pList );
				int iqId = JabberSerialNext();
				JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultPrivacyListActive );
				XmlNodeIq iq( "set", iqId );
				XmlNode* query = iq.addQuery( JABBER_FEAT_PRIVACY_LISTS );
				XmlNode* active = query->addChild( "active" );
				if ( pList )
					active->addAttr( "name", pList->GetListName() );
				g_PrivacyListManager.Unlock();

				jabberThreadInfo->send( iq );
			}
			return TRUE;

		case IDC_SET_DEFAULT:
			if ( jabberOnline ) {
				g_PrivacyListManager.Lock();
				EnableWindow( GetDlgItem( hwndDlg, IDC_SET_DEFAULT ), FALSE );
				CPrivacyList* pList = GetSelectedList(hwndDlg);
				SetWindowLong( GetDlgItem( hwndDlg, IDC_SET_DEFAULT ), GWL_USERDATA, (DWORD)pList );
				int iqId = JabberSerialNext();
				JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultPrivacyListDefault );
				XmlNodeIq iq( "set", iqId);
				XmlNode* query = iq.addQuery( JABBER_FEAT_PRIVACY_LISTS );
				XmlNode* defaultTag = query->addChild( "default" );
				if ( pList )
					defaultTag->addAttr( "name", pList->GetListName() );
				g_PrivacyListManager.Unlock();

				jabberThreadInfo->send( iq );
			}
			return TRUE;

		case IDC_LB_LISTS:
			if ( HIWORD( wParam ) == LBN_SELCHANGE ) {
				int nLbSel = SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_GETCURSEL, 0, 0 );
				SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_RESETCONTENT, 0, 0 );

				int nCurSel = SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_GETCURSEL, 0, 0 );
				if ( nCurSel == LB_ERR )
					return TRUE;

				int nErr = SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_GETITEMDATA, nCurSel, 0 );
				if ( nErr == LB_ERR )
					return TRUE;
				if ( nErr == 0 )
				{
					EnableWindow( GetDlgItem( hwndDlg, IDC_PL_RULES_LIST ), FALSE );
					SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_ADDSTRING, 0, (LPARAM)TranslateTS(_T("No list selected")));
					PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_PL_RULES_LIST, LBN_SELCHANGE ), 0 );
					return TRUE;
				}

				g_PrivacyListManager.Lock();
				TCHAR *szListName = ((CPrivacyList* )nErr)->GetListName();
				BOOL bListEmpty = TRUE;

				CPrivacyListRule* pRule = ((CPrivacyList* )nErr)->GetFirstRule();

				while ( pRule ) {
					bListEmpty = FALSE;
					TCHAR szTypeValue[ 512 ];
					switch ( pRule->GetType() ) {
					case Jid:
						mir_sntprintf( szTypeValue, SIZEOF( szTypeValue ), _T( "If jabber id is '%s' then" ), pRule->GetValue() );
						break;
					case Group:
						mir_sntprintf( szTypeValue, SIZEOF( szTypeValue ), _T( "If group is '%s' then" ), pRule->GetValue() );
						break;
					case Subscription:
						mir_sntprintf( szTypeValue, SIZEOF( szTypeValue ), _T( "If subscription is '%s' then" ), pRule->GetValue() );
						break;
					case Else:
						mir_sntprintf( szTypeValue, SIZEOF( szTypeValue ), _T( "Else"));
						break;
					}

					TCHAR szPackets[ 512 ];
					szPackets[ 0 ] = '\0';

					DWORD dwPackets = pRule->GetPackets();
					if ( !dwPackets )
						dwPackets = JABBER_PL_RULE_TYPE_ALL;
					if ( dwPackets == JABBER_PL_RULE_TYPE_ALL )
						_tcscpy( szPackets, _T("all") );
					else {
						if ( dwPackets & JABBER_PL_RULE_TYPE_MESSAGE )
							_tcscat( szPackets, _T("messages") );
						if ( dwPackets & JABBER_PL_RULE_TYPE_PRESENCE_IN ) {
							if ( _tcslen( szPackets ))
								_tcscat( szPackets, _T(", "));
							_tcscat( szPackets, _T("presence-in") );
						}
						if ( dwPackets & JABBER_PL_RULE_TYPE_PRESENCE_OUT ) {
							if ( _tcslen( szPackets ))
								_tcscat( szPackets, _T(", "));
							_tcscat( szPackets, _T("presence-out") );
						}
						if ( dwPackets & JABBER_PL_RULE_TYPE_IQ ) {
							if ( _tcslen( szPackets ))
								_tcscat( szPackets, _T(", "));
							_tcscat( szPackets, _T("queries") );
					}	}

					TCHAR szListItem[ 512 ];
					mir_sntprintf( szListItem, SIZEOF( szListItem ), _T("%s %s %s"), szTypeValue, pRule->GetAction() ? _T("allow") : _T("deny"), szPackets );

					int nItemId = SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_ADDSTRING, 0, (LPARAM)szListItem );
					SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_SETITEMDATA, nItemId, (LPARAM)pRule );

					pRule = pRule->GetNext();
				}

				EnableWindow( GetDlgItem( hwndDlg, IDC_PL_RULES_LIST ), !bListEmpty );
				if ( bListEmpty )
					SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_ADDSTRING, 0, (LPARAM)TranslateTS(_T("List has no rules, empty lists will be deleted then changes applied")));
				else
					SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_SETCURSEL, nLbSel, 0 );
				
				PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_PL_RULES_LIST, LBN_SELCHANGE ), 0 );

				g_PrivacyListManager.Unlock();
			}
			else if ( HIWORD( wParam ) == LBN_DBLCLK )
				PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_ACTIVATE, 0 ), 0 );
			return TRUE;

		case IDC_PL_RULES_LIST:
			if ( HIWORD( wParam ) == LBN_SELCHANGE ) {
				g_PrivacyListManager.Lock();
				BOOL bListsLoaded = g_PrivacyListManager.IsAllListsLoaded();
				g_PrivacyListManager.Unlock();

				int nCurSel = SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_GETCURSEL, 0, 0 );
				int nItemCount = SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_GETCOUNT, 0, 0 );
				BOOL bSelected = nCurSel != CB_ERR;
				BOOL bListSelected = SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_GETCOUNT, 0, 0);
				bListSelected = bListSelected && (SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_GETCURSEL, 0, 0) != LB_ERR);
				bListSelected = bListSelected && SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_GETITEMDATA, SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_GETCURSEL, 0, 0), 0);

				EnableWindow( GetDlgItem( hwndDlg, IDC_ADD_RULE ), bListsLoaded && bListSelected );
				EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT_RULE ), bListsLoaded && bSelected );
				EnableWindow( GetDlgItem( hwndDlg, IDC_REMOVE_RULE ), bListsLoaded && bSelected );
				EnableWindow( GetDlgItem( hwndDlg, IDC_UP_RULE ), bListsLoaded && bSelected && nCurSel != 0 );
				EnableWindow( GetDlgItem( hwndDlg, IDC_DOWN_RULE ), bListsLoaded && bSelected && nCurSel != ( nItemCount - 1 ));
				EnableWindow( GetDlgItem( hwndDlg, IDC_REMOVE_LIST ), bListsLoaded && bListSelected );
				g_PrivacyListManager.Lock();
				EnableWindow( GetDlgItem( hwndDlg, IDC_APPLY ), bListsLoaded && g_PrivacyListManager.IsModified() );
				g_PrivacyListManager.Unlock();
			}
			else if ( HIWORD( wParam ) == LBN_DBLCLK )
				PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_EDIT_RULE, 0 ), 0 );

			return TRUE;

		case IDC_EDIT_RULE:
			// FIXME: potential deadlock due to PLM lock while editing rule
			g_PrivacyListManager.Lock();
			{
				CPrivacyListRule* pRule = GetSelectedRule( hwndDlg );
				CPrivacyList* pList = GetSelectedList(hwndDlg);
				if ( pList && pRule ) {
					int nResult = DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_PRIVACY_RULE ), hwndDlg, JabberPrivacyRuleDlgProc, (LPARAM)pRule );
					if ( nResult ) {
						pList->SetModified();
						PostMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
			}	}	}
			g_PrivacyListManager.Unlock();
			return TRUE;

		case IDC_ADD_RULE:
			// FIXME: potential deadlock due to PLM lock while editing rule
			g_PrivacyListManager.Lock();
			{
				CPrivacyList* pList = GetSelectedList(hwndDlg);
				if ( pList ) {
					CPrivacyListRule* pRule = new CPrivacyListRule( Jid, _T(""), FALSE );
					int nResult = DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_PRIVACY_RULE ), hwndDlg, JabberPrivacyRuleDlgProc, (LPARAM)pRule );
					if ( nResult ) {
						pList->AddRule(pRule);
						pList->Reorder();
						pList->SetModified();
						PostMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
					}
					else delete pRule;
			}	}
			g_PrivacyListManager.Unlock();
			return TRUE;

		case IDC_REMOVE_RULE:
			g_PrivacyListManager.Lock();
			{
				CPrivacyList* pList = GetSelectedList(hwndDlg);
				CPrivacyListRule* pRule = GetSelectedRule( hwndDlg );

				if ( pList && pRule ) {
					pList->RemoveRule( pRule );
					int nCurSel = SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_GETCURSEL, 0, 0 );
					int nItemCount = SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_GETCOUNT, 0, 0 );
					SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_SETCURSEL, nCurSel != nItemCount - 1 ? nCurSel : nCurSel - 1, 0 );
					pList->Reorder();
					pList->SetModified();
					PostMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
			}	}

			g_PrivacyListManager.Unlock();
			return TRUE;

		case IDC_UP_RULE:
			g_PrivacyListManager.Lock();
			{
				CPrivacyList* pList = GetSelectedList(hwndDlg);
				CPrivacyListRule* pRule = GetSelectedRule( hwndDlg );
				int nCurSel = SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_GETCURSEL, 0, 0 );

				if ( pList && pRule && nCurSel ) {
					pRule->SetOrder( pRule->GetOrder() - 11 );
					SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_SETCURSEL, nCurSel - 1, 0 );
					pList->Reorder();
					pList->SetModified();
					PostMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
			}	}

			g_PrivacyListManager.Unlock();
			return TRUE;

		case IDC_DOWN_RULE:
			g_PrivacyListManager.Lock();
			{
				CPrivacyList* pList = GetSelectedList(hwndDlg);
				CPrivacyListRule* pRule = GetSelectedRule( hwndDlg );
				int nCurSel = SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_GETCURSEL, 0, 0 );
				int nItemCount = SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_GETCOUNT, 0, 0 );

				if ( pList && pRule && ( nCurSel != ( nItemCount - 1 ))) {
					pRule->SetOrder( pRule->GetOrder() + 11 );
					SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_SETCURSEL, nCurSel + 1, 0 );
					pList->Reorder();
					pList->SetModified();
					PostMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
			}	}

			g_PrivacyListManager.Unlock();
			return TRUE;

		case IDC_ADD_LIST:
			{
				// FIXME: line length is hard coded in dialog procedure
				TCHAR szLine[ 512 ];
				szLine[0] = _T( '\0' );
				int nRetVal = DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_PRIVACY_ADD_LIST ), hwndDlg, JabberPrivacyAddListDlgProc, (LPARAM)szLine );
				if ( nRetVal && _tcslen( szLine ) ) {
					g_PrivacyListManager.Lock();
					CPrivacyList* pList = g_PrivacyListManager.FindList( szLine );
					if ( !pList ) {
						g_PrivacyListManager.AddList( szLine );
						pList = g_PrivacyListManager.FindList( szLine );
						if ( pList ) {
							pList->SetModified( TRUE );
							pList->SetLoaded( TRUE );
						}
					}
					if ( pList )
						pList->SetDeleted( FALSE );
					int nSelected = SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_SELECTSTRING, -1, (LPARAM)szLine );
					if ( nSelected == CB_ERR ) {
						nSelected = SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_ADDSTRING, 0, (LPARAM)szLine );
						SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_SETITEMDATA, nSelected, (LPARAM)pList );
						SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_SETCURSEL, nSelected, 0 );
					}
					g_PrivacyListManager.Unlock();
					PostMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
				}
				return TRUE;
			}

		case IDC_REMOVE_LIST:
			g_PrivacyListManager.Lock();
			{
				CPrivacyList* pList = GetSelectedList(hwndDlg);
				if ( pList ) {
					TCHAR *szListName = pList->GetListName();
					if ( ( g_PrivacyListManager.GetActiveListName() && !_tcscmp( szListName, g_PrivacyListManager.GetActiveListName()))
						|| ( g_PrivacyListManager.GetDefaultListName() && !_tcscmp( szListName, g_PrivacyListManager.GetDefaultListName()) )) {
						g_PrivacyListManager.Unlock();
						MessageBox( hwndDlg, TranslateTS(_T("Can't remove active or default list")), TranslateTS(_T("Sorry")), MB_OK | MB_ICONSTOP );
						return TRUE;
					}
					pList->SetDeleted();
					pList->SetModified();
			}	}

			g_PrivacyListManager.Unlock();
			PostMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
			return TRUE;

		case IDC_APPLY:
			if ( !jabberOnline )
				return TRUE;

			g_PrivacyListManager.Lock();
			{
				DWORD dwGroupId = 0;
				CPrivacyListModifyUserParam *pUserData = NULL;
				CPrivacyList* pList = g_PrivacyListManager.GetFirstList();
				while ( pList ) {
					if ( pList->IsModified() ) {
						CPrivacyListRule* pRule = pList->GetFirstRule();
						if ( !pRule )
							pList->SetDeleted();
						if ( pList->IsDeleted() ) {
							pList->RemoveAllRules();
							pRule = NULL;
						}
						pList->SetModified( FALSE );

						if ( !dwGroupId ) {
							dwGroupId = g_JabberIqManager.GetNextFreeGroupId();
							pUserData = new CPrivacyListModifyUserParam;
							pUserData->m_bAllOk = TRUE;
						}

						XmlNodeIq iq( g_JabberIqManager.AddHandler( JabberIqResultPrivacyListModify, JABBER_IQ_TYPE_SET, NULL, 0, -1, pUserData, dwGroupId ));
						XmlNode* query = iq.addQuery( JABBER_FEAT_PRIVACY_LISTS );
						XmlNode* listTag = query->addChild( "list" );
						listTag->addAttr( "name", pList->GetListName() );

						while ( pRule ) {
							XmlNode* itemTag = listTag->addChild( "item" );
							switch ( pRule->GetType() ) {
							case Jid:
								itemTag->addAttr( "type", "jid" );
								break;
							case Group:
								itemTag->addAttr( "type", "group" );
								break;
							case Subscription:
								itemTag->addAttr( "type", "subscription" );
								break;
							}
							if ( pRule->GetType() != Else )
								itemTag->addAttr( "value", pRule->GetValue() );
							if ( pRule->GetAction() )
								itemTag->addAttr( "action", "allow" );
							else
								itemTag->addAttr( "action", "deny" );
							itemTag->addAttr( "order", pRule->GetOrder() );
							DWORD dwPackets = pRule->GetPackets();
							if ( dwPackets != JABBER_PL_RULE_TYPE_ALL ) {
								if ( dwPackets & JABBER_PL_RULE_TYPE_IQ )
									itemTag->addChild( "iq" );
								if ( dwPackets & JABBER_PL_RULE_TYPE_PRESENCE_IN )
									itemTag->addChild( "presence-in" );
								if ( dwPackets & JABBER_PL_RULE_TYPE_PRESENCE_OUT )
									itemTag->addChild( "presence-out" );
								if ( dwPackets & JABBER_PL_RULE_TYPE_MESSAGE )
									itemTag->addChild( "message" );
							}
							pRule = pRule->GetNext();
						}

						jabberThreadInfo->send( iq );
					}
					pList = pList->GetNext();
			}	}
			g_PrivacyListManager.Unlock();
			PostMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
			return TRUE;

		case IDCLOSE:
		case IDCANCEL:
			if ( jabberOnline && JabberPrivacyListDlgCanExit( hwndDlg ))
				DestroyWindow( hwndDlg );
			return TRUE;
		}
		break;

	case WM_CLOSE:
		if ( jabberOnline && JabberPrivacyListDlgCanExit( hwndDlg ))
			DestroyWindow( hwndDlg );
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if ( ((HWND)lParam == GetDlgItem(hwndDlg, IDC_WHITERECT)) ||
			 ((HWND)lParam == GetDlgItem(hwndDlg, IDC_TITLE)) ||
			 ((HWND)lParam == GetDlgItem(hwndDlg, IDC_DESCRIPTION)) )
			return (BOOL)GetStockObject(WHITE_BRUSH);
		return FALSE;

	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
			lpmmi->ptMinTrackSize.x = 550;
			lpmmi->ptMinTrackSize.y = 390;
			return 0;
		}
	case WM_SIZE:
	{
		UTILRESIZEDIALOG urd = {0};
		urd.cbSize = sizeof(urd);
		urd.hInstance = hInst;
		urd.hwndDlg = hwndDlg;
		urd.lpTemplate = MAKEINTRESOURCEA(IDD_PRIVACY_LISTS);
		urd.pfnResizer = sttPrivacyListsResizer;
		CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM)&urd);
		break;
	}

	case WM_DESTROY:
		hwndPrivacyLists = NULL;
		g_PrivacyListManager.RemoveAllLists();
		g_PrivacyListManager.SetModified( FALSE );

		// Delete custom bold font
		DeleteObject((HFONT)SendDlgItemMessage(hwndDlg, IDC_TXT_LISTS, WM_GETFONT, 0, 0));

		Utils_SaveWindowPosition(hwndDlg, NULL, jabberProtoName, "plistsWnd_sz");

		break;
	}
	return FALSE;
}

int JabberMenuHandlePrivacyLists( WPARAM wParam, LPARAM lParam )
{
	if ( hwndPrivacyLists && IsWindow( hwndPrivacyLists ))
		SetForegroundWindow( hwndPrivacyLists );
	else
		CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_PRIVACY_LISTS ), NULL, JabberPrivacyListsDlgProc, lParam );

	return 0;
}
