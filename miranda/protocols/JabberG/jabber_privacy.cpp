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

#include "m_genmenu.h"
#include "m_clistint.h"

#define JABBER_PL_BUSY_MSG				"Sending request, please wait..."

CPrivacyListManager g_PrivacyListManager;

static void sttStatusMessage(TCHAR *msg = NULL)
{
	SendDlgItemMessage(hwndPrivacyLists, IDC_STATUSBAR, SB_SETTEXT, SB_SIMPLEID,
		(LPARAM)(msg ? msg : TranslateT("Ready.")));
}

void JabberProcessIqPrivacyLists( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_SET ) {
		if ( !hwndPrivacyLists )
		{
			g_PrivacyListManager.RemoveAllLists();
			g_PrivacyListManager.QueryLists();
		} else
		{
			sttStatusMessage(TranslateT("Warning: privacy lists were changed on server."));
		}

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
		sttStatusMessage(szText);
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
	if ( nCurSel == LB_ERR )
		return NULL;

	int nItemData = SendDlgItemMessage( hDlg, IDC_LB_LISTS, LB_GETITEMDATA, nCurSel, 0 );
	if ( nItemData == LB_ERR || nItemData == 0 )
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

void JabberIqResultPrivacyListActive( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	CPrivacyList *pList = (CPrivacyList *)pInfo->GetUserData();

	if ( hwndPrivacyLists )
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
	g_PrivacyListManager.Unlock();

	if ( hwndPrivacyLists )
	{
		sttStatusMessage(szText);
		RedrawWindow(GetDlgItem(hwndPrivacyLists, IDC_LB_LISTS), NULL, NULL, RDW_INVALIDATE);
	}

	JabberUtilsRebuildStatusMenu();
}

void JabberIqResultPrivacyListDefault( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	CPrivacyList *pList = (CPrivacyList *)pInfo->GetUserData();

	if ( hwndPrivacyLists )
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
	g_PrivacyListManager.Unlock();

	if ( hwndPrivacyLists )
	{
		sttStatusMessage(szText);
		RedrawWindow(GetDlgItem(hwndPrivacyLists, IDC_LB_LISTS), NULL, NULL, RDW_INVALIDATE);
	}
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

	g_PrivacyListManager.Lock();
	g_PrivacyListManager.RemoveAllLists();

	XmlNode *list;
	for ( int i = 1; ( list = JabberXmlGetNthChild( query, "list", i )) != NULL; i++ ) {
		TCHAR *listName = JabberXmlGetAttrValue( list, "name" );
		if ( listName ) {
			g_PrivacyListManager.AddList(listName);

			// Query contents only if list editior is visible!
			if ( hwndPrivacyLists ) {
				int iqId = JabberSerialNext();
				JabberIqAdd( iqId, IQ_PROC_NONE, JabberIqResultPrivacyList);
				XmlNodeIq iq( "get", iqId);
				XmlNode* queryNode = iq.addQuery( JABBER_FEAT_PRIVACY_LISTS );
				XmlNode* listNode = queryNode->addChild( "list" );
				listNode->addAttr( "name", listName );
				jabberThreadInfo->send( iq );
		}	}	}

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

	if ( hwndPrivacyLists )
		SendMessage( hwndPrivacyLists, WM_JABBER_REFRESH, 0, 0);

	JabberUtilsRebuildStatusMenu();
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

			SendDlgItemMessage(hwndDlg, IDC_ICO_MESSAGE,		STM_SETICON, (WPARAM)LoadIconEx("pl_msg_allow"), 0);
			SendDlgItemMessage(hwndDlg, IDC_ICO_QUERY,			STM_SETICON, (WPARAM)LoadIconEx("pl_iq_allow"), 0);
			SendDlgItemMessage(hwndDlg, IDC_ICO_PRESENCEIN,		STM_SETICON, (WPARAM)LoadIconEx("pl_prin_allow"), 0);
			SendDlgItemMessage(hwndDlg, IDC_ICO_PRESENCEOUT,	STM_SETICON, (WPARAM)LoadIconEx("pl_prout_allow"), 0);

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

static void sttShowAdvancedList(HWND hwndDlg, CPrivacyList *pList)
{
	int nLbSel = SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_GETCURSEL, 0, 0 );
	SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_RESETCONTENT, 0, 0 );

	TCHAR *szListName = pList->GetListName();
	BOOL bListEmpty = TRUE;

	CPrivacyListRule* pRule = pList->GetFirstRule();

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
		LoadIconEx("main"), 16, 16, 0, NULL, DI_NORMAL);

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
	if (lpdis->itemID == -1)
		return;

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

	rc = lpdis->rcItem;
	rc.left +=3;

	bool bActive = false;
	bool bDefault = false;
	TCHAR *szName;

	if (!pList)
	{
		if (!szActive) bActive = true;
		if (!szDefault) bDefault = true;
		szName = TranslateT("<none>");
	} else
	{
		if (!lstrcmp(pList->GetListName(), szActive)) bActive = true;
		if (!lstrcmp(pList->GetListName(), szDefault)) bDefault = true;
		szName = pList->GetListName();
	}

	HFONT hfnt = NULL;
	if (bActive)
	{
		LOGFONT lf;
		GetObject(GetCurrentObject(lpdis->hDC, OBJ_FONT), sizeof(lf), &lf);
		lf.lfWeight = FW_BOLD;
		hfnt = (HFONT)SelectObject(lpdis->hDC, CreateFontIndirect(&lf));
	}

	sttDrawNextRulePart(lpdis->hDC, clLine1, szName, &rc);

	if (bActive && bDefault)
		sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT(" (act., def.)"), &rc);
	else if (bActive)
		sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT(" (active)"), &rc);
	else if (bDefault)
		sttDrawNextRulePart(lpdis->hDC, clLine2, TranslateT(" (default)"), &rc);

	DrawIconEx(lpdis->hDC, lpdis->rcItem.right-16-4, (lpdis->rcItem.top+lpdis->rcItem.bottom-16)/2,
		LoadIconEx(bActive ? "pl_list_active" : "pl_list_any"),
		16, 16, 0, NULL, DI_NORMAL);

	if (bDefault)
		DrawIconEx(lpdis->hDC, lpdis->rcItem.right-16-4, (lpdis->rcItem.top+lpdis->rcItem.bottom-16)/2,
			LoadIconEx("disco_ok"),
			16, 16, 0, NULL, DI_NORMAL);

	if (hfnt)
		DeleteObject(SelectObject(lpdis->hDC, hfnt));

	if (lpdis->itemState & ODS_FOCUS)
	{
		int sel = SendDlgItemMessage(hwndDlg, lpdis->CtlID, LB_GETCURSEL, 0, 0);
		if ((sel == LB_ERR) || (sel == lpdis->itemID))
			DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
	}
}

struct TCListInfo
{
	struct TJidData
	{
		int hItem;
		TCHAR *jid;

		static int cmp(const TJidData *p1, const TJidData *p2) { return lstrcmp(p1->jid, p2->jid); }
	};

	int hItemDefault;
	int hItemSubNone;
	int hItemSubTo;
	int hItemSubFrom;
	int hItemSubBoth;

	LIST<TJidData> newJids;

	bool bChanged;

	CPrivacyList *pList;

	TCListInfo(): newJids(1, TJidData::cmp), bChanged(false), pList(0) {}
	~TCListInfo()
	{
		for (int i = 0; i < newJids.getCount(); ++i)
		{
			mir_free(newJids[i]->jid);
			mir_free(newJids[i]);
		}
		newJids.destroy();
	}

	void addJid(int hItem, TCHAR *jid)
	{
		TJidData *data = (TJidData *)mir_alloc(sizeof(TJidData));
		data->hItem = hItem;
		data->jid = mir_tstrdup(jid);
		newJids.insert(data);
	}

	int findJid(TCHAR *jid)
	{
		TJidData data = {0};
		data.jid = jid;
		TJidData *found = newJids.find(&data);
		return found ? found->hItem : 0;
	}
};

static void sttCListResetOptions(HWND hwndList)
{
	int i;
	SendMessage(hwndList,CLM_SETBKBITMAP,0,(LPARAM)(HBITMAP)NULL);
	SendMessage(hwndList,CLM_SETBKCOLOR,GetSysColor(COLOR_WINDOW),0);
	SendMessage(hwndList,CLM_SETGREYOUTFLAGS,0,0);
	SendMessage(hwndList,CLM_SETLEFTMARGIN,4,0);
	SendMessage(hwndList,CLM_SETINDENT,10,0);
	SendMessage(hwndList,CLM_SETHIDEEMPTYGROUPS,0,0);
	SendMessage(hwndList,CLM_SETHIDEOFFLINEROOT,0,0);
	for ( i=0; i <= FONTID_MAX; i++ )
		SendMessage( hwndList, CLM_SETTEXTCOLOR, i, GetSysColor( COLOR_WINDOWTEXT ));
}

static void sttCListFilter(HWND hwndList)
{
	for	(HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
			hContact;
			hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0))
	{
		char *proto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (!proto || lstrcmpA(proto, jabberProtoName))
			if (int hItem = SendMessage(hwndList, CLM_FINDCONTACT, (WPARAM)hContact, 0))
				SendMessage(hwndList, CLM_DELETEITEM, (WPARAM)hItem, 0);
	}
}

static bool sttCListIsGroup(int hGroup)
{
	char idstr[33];
	DBVARIANT dbv;

	bool result = true;

	itoa(hGroup-1, idstr, 10);

	if (DBGetContactSettingTString(NULL, "CListGroups", idstr, &dbv))
		result = false;

	DBFreeVariant(&dbv);

	return result;
}

static HANDLE sttCListFindGroupByName(TCHAR *name)
{
	char idstr[33];
	DBVARIANT dbv;

	HANDLE hGroup = 0;

	for (int i= 0; !hGroup; ++i)
	{
		itoa(i, idstr, 10);

		if (DBGetContactSettingTString(NULL, "CListGroups", idstr, &dbv))
			break;

		if (!_tcscmp(dbv.ptszVal + 1, name))
			hGroup = (HANDLE)(i+1);

		DBFreeVariant(&dbv);
	}

	return hGroup;
}

static void sttCListResetIcons(HWND hwndList, int hItem, bool hide=false)
{
	for (int i = 0; i < 4; ++i)
		SendMessage(hwndList, CLM_SETEXTRAIMAGE, hItem, MAKELPARAM(i, hide?0xFF:0));
}

static void sttCListSetupIcons(HWND hwndList, int hItem, int iSlot, DWORD dwProcess, BOOL bAction)
{
	if (dwProcess && !SendMessage(hwndList, CLM_GETEXTRAIMAGE, hItem, MAKELPARAM(iSlot, 0)))
		SendMessage(hwndList, CLM_SETEXTRAIMAGE, hItem, MAKELPARAM(iSlot, iSlot*2 + (bAction?1:2) ));
}

static int sttCListAddContact(HWND hwndList, TCHAR *jid)
{
	int hItem = 0;
	TCListInfo *info = (TCListInfo *)GetWindowLong(hwndList, GWL_USERDATA);

	HANDLE hContact = JabberHContactFromJID(jid);

	if (!hContact)
	{
		hItem = info->findJid(jid);
		if (!hItem)
		{
			CLCINFOITEM cii = {0};
			cii.cbSize = sizeof(cii);
			cii.pszText = jid;
			hItem = SendMessage(hwndList,CLM_ADDINFOITEM,0,(LPARAM)&cii);
			sttCListResetIcons(hwndList, hItem);
			info->addJid(hItem, jid);
		}
	} else
	{
		hItem = SendMessage(hwndList, CLM_FINDCONTACT, (WPARAM)hContact, 0);
	}

	return hItem;
}

static void sttCListApplyList(HWND hwndList, CPrivacyList *pList = NULL)
{
	TCListInfo *info = (TCListInfo *)GetWindowLong(hwndList, GWL_USERDATA);
	if (!info) return;

	info->pList = pList;

	SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);

	bool bHideIcons = pList ? false : true;

	sttCListResetIcons(hwndList, info->hItemDefault, bHideIcons);
	sttCListResetIcons(hwndList, info->hItemSubBoth, bHideIcons);
	sttCListResetIcons(hwndList, info->hItemSubFrom, bHideIcons);
	sttCListResetIcons(hwndList, info->hItemSubNone, bHideIcons);
	sttCListResetIcons(hwndList, info->hItemSubTo, bHideIcons);

	// group handles start with 1 (0 is "root") 
	for (int hGroup = 1; sttCListIsGroup(hGroup); ++hGroup)
	{
		int hItem = SendMessage(hwndList, CLM_FINDGROUP, hGroup, 0);
		if (!hItem) continue;
		sttCListResetIcons(hwndList, hItem, bHideIcons);
	}

	for (HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0); hContact;
			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0))
	{
		int hItem = SendMessage(hwndList, CLM_FINDCONTACT, (WPARAM)hContact, 0);
		if (!hItem) continue;
		sttCListResetIcons(hwndList, hItem, bHideIcons);
	}

	for (int iJid = 0; iJid < info->newJids.getCount(); ++iJid)
		sttCListResetIcons(hwndList, info->newJids[iJid]->hItem, bHideIcons);

	if (!pList)
		goto lbl_return;

	for (CPrivacyListRule *pRule = pList->GetFirstRule(); pRule; pRule = pRule->GetNext())
	{
		int hItem = 0;
		switch (pRule->GetType())
		{
			case Jid:
			{
				hItem = sttCListAddContact(hwndList, pRule->GetValue());
				break;
			}
			case Group:
			{
				HANDLE hGroup = sttCListFindGroupByName(pRule->GetValue());
				hItem = SendMessage(hwndList, CLM_FINDGROUP, (WPARAM)hGroup, 0);
				break;
			}
			case Subscription:
			{
				     if (!lstrcmp(pRule->GetValue(), _T("none")))	hItem = info->hItemSubNone;
				else if (!lstrcmp(pRule->GetValue(), _T("from")))	hItem = info->hItemSubFrom;
				else if (!lstrcmp(pRule->GetValue(), _T("to")))		hItem = info->hItemSubTo;
				else if (!lstrcmp(pRule->GetValue(), _T("both")))	hItem = info->hItemSubBoth;
				break;
			}
			case Else:
			{
				hItem = info->hItemDefault;
				break;
			}
		}

		if (!hItem) continue;

		DWORD dwPackets = pRule->GetPackets();
		if (!dwPackets) dwPackets = JABBER_PL_RULE_TYPE_ALL;
		sttCListSetupIcons(hwndList, hItem, 0, dwPackets & JABBER_PL_RULE_TYPE_MESSAGE, pRule->GetAction());
		sttCListSetupIcons(hwndList, hItem, 1, dwPackets & JABBER_PL_RULE_TYPE_PRESENCE_IN, pRule->GetAction());
		sttCListSetupIcons(hwndList, hItem, 2, dwPackets & JABBER_PL_RULE_TYPE_PRESENCE_OUT, pRule->GetAction());
		sttCListSetupIcons(hwndList, hItem, 3, dwPackets & JABBER_PL_RULE_TYPE_IQ, pRule->GetAction());
	}

lbl_return:
	info->bChanged = false;
	SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);
	RedrawWindow(hwndList, NULL, NULL, RDW_INVALIDATE);
}

static DWORD sttCListGetPackets(HWND hwndList, int hItem, bool bAction)
{
	DWORD result = 0;

	int iIcon = 0;

	iIcon = SendMessage(hwndList, CLM_GETEXTRAIMAGE, hItem, MAKELPARAM(0, 0));
	     if ( bAction && (iIcon == 1)) result |= JABBER_PL_RULE_TYPE_MESSAGE;
	else if (!bAction && (iIcon == 2)) result |= JABBER_PL_RULE_TYPE_MESSAGE;

	iIcon = SendMessage(hwndList, CLM_GETEXTRAIMAGE, hItem, MAKELPARAM(1, 0));
	     if ( bAction && (iIcon == 3)) result |= JABBER_PL_RULE_TYPE_PRESENCE_IN;
	else if (!bAction && (iIcon == 4)) result |= JABBER_PL_RULE_TYPE_PRESENCE_IN;

	iIcon = SendMessage(hwndList, CLM_GETEXTRAIMAGE, hItem, MAKELPARAM(2, 0));
	     if ( bAction && (iIcon == 5)) result |= JABBER_PL_RULE_TYPE_PRESENCE_OUT;
	else if (!bAction && (iIcon == 6)) result |= JABBER_PL_RULE_TYPE_PRESENCE_OUT;

	iIcon = SendMessage(hwndList, CLM_GETEXTRAIMAGE, hItem, MAKELPARAM(3, 0));
	     if ( bAction && (iIcon == 7)) result |= JABBER_PL_RULE_TYPE_IQ;
	else if (!bAction && (iIcon == 8)) result |= JABBER_PL_RULE_TYPE_IQ;

	return result;
}

static void sttCListBuildList(HWND hwndList, CPrivacyList *pList)
{
	if (!pList) return;

	TCListInfo *info = (TCListInfo *)GetWindowLong(hwndList, GWL_USERDATA);
	if (!info || !info->bChanged) return;

	info->bChanged = false;

	DWORD dwOrder = 0;
	DWORD dwPackets = 0;

	int hItem;
	TCHAR *szJid;

	pList->RemoveAllRules();

	for (int iJid = 0; iJid < info->newJids.getCount(); ++iJid)
	{
		hItem = info->newJids[iJid]->hItem;
		szJid = info->newJids[iJid]->jid;

		if (dwPackets = sttCListGetPackets(hwndList, hItem, true))
			pList->AddRule(Jid, szJid, TRUE, dwOrder++, dwPackets);
		if (dwPackets = sttCListGetPackets(hwndList, hItem, false))
			pList->AddRule(Jid, szJid, FALSE, dwOrder++, dwPackets);
	}

	for (HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0); hContact;
			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0))
	{
		hItem = SendMessage(hwndList, CLM_FINDCONTACT, (WPARAM)hContact, 0);

		DBVARIANT dbv;
		if (JGetStringT(hContact, "jid", &dbv))
		{
			JFreeVariant(&dbv);
			continue;
		}

		szJid = dbv.ptszVal;

		if (dwPackets = sttCListGetPackets(hwndList, hItem, true))
			pList->AddRule(Jid, szJid, TRUE, dwOrder++, dwPackets);
		if (dwPackets = sttCListGetPackets(hwndList, hItem, false))
			pList->AddRule(Jid, szJid, FALSE, dwOrder++, dwPackets);

		JFreeVariant(&dbv);
	}

	// group handles start with 1 (0 is "root") 
	for (int hGroup = 1; ; ++hGroup)
	{
		char idstr[33];
		itoa(hGroup-1, idstr, 10);
		DBVARIANT dbv;
		if (DBGetContactSettingTString(NULL, "CListGroups", idstr, &dbv))
		{
			DBFreeVariant(&dbv);
			break;
		}

		hItem = SendMessage(hwndList, CLM_FINDGROUP, (WPARAM)hGroup, 0);
		szJid = dbv.ptszVal+1;

		if (dwPackets = sttCListGetPackets(hwndList, hItem, true))
			pList->AddRule(Group, szJid, TRUE, dwOrder++, dwPackets);
		if (dwPackets = sttCListGetPackets(hwndList, hItem, false))
			pList->AddRule(Group, szJid, FALSE, dwOrder++, dwPackets);

		DBFreeVariant(&dbv);
	}

	hItem = info->hItemSubBoth;
	szJid = _T("both");
	if (dwPackets = sttCListGetPackets(hwndList, hItem, true))
		pList->AddRule(Subscription, szJid, TRUE, dwOrder++, dwPackets);
	if (dwPackets = sttCListGetPackets(hwndList, hItem, false))
		pList->AddRule(Subscription, szJid, FALSE, dwOrder++, dwPackets);

	hItem = info->hItemSubFrom;
	szJid = _T("from");
	if (dwPackets = sttCListGetPackets(hwndList, hItem, true))
		pList->AddRule(Subscription, szJid, TRUE, dwOrder++, dwPackets);
	if (dwPackets = sttCListGetPackets(hwndList, hItem, false))
		pList->AddRule(Subscription, szJid, FALSE, dwOrder++, dwPackets);

	hItem = info->hItemSubNone;
	szJid = _T("none");
	if (dwPackets = sttCListGetPackets(hwndList, hItem, true))
		pList->AddRule(Subscription, szJid, TRUE, dwOrder++, dwPackets);
	if (dwPackets = sttCListGetPackets(hwndList, hItem, false))
		pList->AddRule(Subscription, szJid, FALSE, dwOrder++, dwPackets);

	hItem = info->hItemSubTo;
	szJid = _T("to");
	if (dwPackets = sttCListGetPackets(hwndList, hItem, true))
		pList->AddRule(Subscription, szJid, TRUE, dwOrder++, dwPackets);
	if (dwPackets = sttCListGetPackets(hwndList, hItem, false))
		pList->AddRule(Subscription, szJid, FALSE, dwOrder++, dwPackets);

	hItem = info->hItemDefault;
	szJid = NULL;
	if (dwPackets = sttCListGetPackets(hwndList, hItem, true))
		pList->AddRule(Else, szJid, TRUE, dwOrder++, dwPackets);
	if (dwPackets = sttCListGetPackets(hwndList, hItem, false))
		pList->AddRule(Else, szJid, FALSE, dwOrder++, dwPackets);

	pList->Reorder();
	pList->SetModified();
}

static void sttEnableEditorControls(HWND hwndDlg)
{
	TCListInfo *info = (TCListInfo *)GetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_USERDATA);

	g_PrivacyListManager.Lock();
	BOOL bListsLoaded = g_PrivacyListManager.IsAllListsLoaded();
	BOOL bListsModified = g_PrivacyListManager.IsModified() || info->bChanged;
	g_PrivacyListManager.Unlock();

	int nCurSel = SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_GETCURSEL, 0, 0 );
	int nItemCount = SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_GETCOUNT, 0, 0 );
	BOOL bSelected = nCurSel != CB_ERR;
	BOOL bListSelected = SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_GETCOUNT, 0, 0);
	bListSelected = bListSelected && (SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_GETCURSEL, 0, 0) != LB_ERR);
	bListSelected = bListSelected && SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_GETITEMDATA, SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_GETCURSEL, 0, 0), 0);

	EnableWindow( GetDlgItem( hwndDlg, IDC_TXT_OTHERJID ), bListsLoaded && bListSelected );
	EnableWindow( GetDlgItem( hwndDlg, IDC_NEWJID ), bListsLoaded && bListSelected );
	EnableWindow( GetDlgItem( hwndDlg, IDC_ADDJID ), bListsLoaded && bListSelected );

	EnableWindow( GetDlgItem( hwndDlg, IDC_ADD_RULE ), bListsLoaded && bListSelected );
	EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT_RULE ), bListsLoaded && bSelected );
	EnableWindow( GetDlgItem( hwndDlg, IDC_REMOVE_RULE ), bListsLoaded && bSelected );
	EnableWindow( GetDlgItem( hwndDlg, IDC_UP_RULE ), bListsLoaded && bSelected && nCurSel != 0 );
	EnableWindow( GetDlgItem( hwndDlg, IDC_DOWN_RULE ), bListsLoaded && bSelected && nCurSel != ( nItemCount - 1 ));
	EnableWindow( GetDlgItem( hwndDlg, IDC_REMOVE_LIST ), bListsLoaded && bListSelected );
	EnableWindow( GetDlgItem( hwndDlg, IDC_APPLY ), bListsLoaded && bListsModified );

	if (bListsLoaded)
		sttStatusMessage( NULL );
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
	case IDC_BTN_SIMPLE:
	case IDC_BTN_ADVANCED:
		return RD_ANCHORX_RIGHT|RD_ANCHORY_TOP;
	case IDC_LB_LISTS:
		return RD_ANCHORX_LEFT|RD_ANCHORY_TOP|RD_ANCHORY_HEIGHT;
	case IDC_PL_RULES_LIST:
	case IDC_CLIST:
		return RD_ANCHORX_LEFT|RD_ANCHORY_TOP|RD_ANCHORY_HEIGHT|RD_ANCHORX_WIDTH;
	case IDC_NEWJID:
	case IDC_CANVAS:
		return RD_ANCHORX_LEFT|RD_ANCHORX_WIDTH|RD_ANCHORY_BOTTOM;
	case IDC_APPLY:
	case IDC_ADD_LIST:
	case IDC_ACTIVATE:
	case IDC_REMOVE_LIST:
	case IDC_SET_DEFAULT:
	case IDC_TXT_OTHERJID:
		return RD_ANCHORX_LEFT|RD_ANCHORY_BOTTOM;
	case IDC_ADDJID:
	case IDC_ADD_RULE:
	case IDC_UP_RULE:
	case IDC_EDIT_RULE:
	case IDC_DOWN_RULE:
	case IDC_REMOVE_RULE:
		return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
	case IDC_STATUSBAR:
		return RD_ANCHORX_LEFT|RD_ANCHORX_WIDTH|RD_ANCHORY_BOTTOM;
	}
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
}

static int __forceinline sttEmulateBtnClick(HWND hwndDlg, UINT idcButton)
{
	if (IsWindowEnabled(GetDlgItem(hwndDlg, idcButton)))
		PostMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(idcButton, BN_CLICKED), (LPARAM)GetDlgItem(hwndDlg, idcButton));
	return 0;
}

static LRESULT CALLBACK sttLstListsSubclassProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	WNDPROC sttOldWndProc = (WNDPROC)GetWindowLong(hwnd, GWL_USERDATA);
	switch (msg)
	{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			if (wParam == VK_INSERT)
				return sttEmulateBtnClick(GetParent(hwnd), IDC_ADD_LIST);
			if (wParam == VK_DELETE)
				return sttEmulateBtnClick(GetParent(hwnd), IDC_REMOVE_LIST);
			if (wParam == VK_SPACE)
			{
				if (GetAsyncKeyState(VK_CONTROL))
					return sttEmulateBtnClick(GetParent(hwnd), IDC_SET_DEFAULT);
				return sttEmulateBtnClick(GetParent(hwnd), IDC_ACTIVATE);
			}

			break;
		}
	}
	return CallWindowProc(sttOldWndProc, hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK sttLstRulesSubclassProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	WNDPROC sttOldWndProc = (WNDPROC)GetWindowLong(hwnd, GWL_USERDATA);
	switch (msg)
	{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			if (wParam == VK_INSERT)
				return sttEmulateBtnClick(GetParent(hwnd), IDC_ADD_RULE);
			if (wParam == VK_DELETE)
				return sttEmulateBtnClick(GetParent(hwnd), IDC_REMOVE_RULE);
			if ((wParam == VK_UP) && (lParam & (1UL << 29)))
				return sttEmulateBtnClick(GetParent(hwnd), IDC_UP_RULE);
			if ((wParam == VK_DOWN) && (lParam & (1UL << 29)))
				return sttEmulateBtnClick(GetParent(hwnd), IDC_DOWN_RULE);
			if (wParam == VK_F2)
				return sttEmulateBtnClick(GetParent(hwnd), IDC_EDIT_RULE);

			break;
		}
	}
	return CallWindowProc(sttOldWndProc, hwnd, msg, wParam, lParam);
}

static void sttShowControls(HWND hwndDlg, int *idList, int nCmdShow)
{
	for (; *idList; ++idList)
		ShowWindow(GetDlgItem(hwndDlg, *idList), nCmdShow);
}

BOOL CALLBACK JabberPrivacyListsDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	static int idSimpleControls[] =
	{
		IDC_CLIST, IDC_CANVAS,
		IDC_TXT_OTHERJID, IDC_NEWJID, IDC_ADDJID,
		IDC_ICO_MESSAGE, IDC_ICO_QUERY, IDC_ICO_INPRESENCE, IDC_ICO_OUTPRESENCE,
		IDC_TXT_MESSAGE, IDC_TXT_QUERY, IDC_TXT_INPRESENCE, IDC_TXT_OUTPRESENCE,
		0
	};
	static int idAdvancedControls[] =
	{
		IDC_PL_RULES_LIST,
		IDC_ADD_RULE, IDC_EDIT_RULE, IDC_REMOVE_RULE,
		IDC_UP_RULE, IDC_DOWN_RULE,
		0
	};

	switch ( msg ) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault( hwndDlg );
			SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )LoadIconEx( "privacylists" ));
			hwndPrivacyLists = hwndDlg;

			EnableWindow( GetDlgItem( hwndDlg, IDC_ADD_RULE ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_EDIT_RULE ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_REMOVE_RULE ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_UP_RULE ), FALSE );
			EnableWindow( GetDlgItem( hwndDlg, IDC_DOWN_RULE ), FALSE );

			g_PrivacyListManager.QueryLists();

			LOGFONT lf;
			GetObject((HFONT)SendDlgItemMessage(hwndDlg, IDC_TXT_LISTS, WM_GETFONT, 0, 0), sizeof(lf), &lf);
			lf.lfWeight = FW_BOLD;
			HFONT hfnt = CreateFontIndirect(&lf);
			SendDlgItemMessage(hwndDlg, IDC_TXT_LISTS, WM_SETFONT, (WPARAM)hfnt, TRUE);
			SendDlgItemMessage(hwndDlg, IDC_TXT_RULES, WM_SETFONT, (WPARAM)hfnt, TRUE);
			SendDlgItemMessage(hwndDlg, IDC_TITLE, WM_SETFONT, (WPARAM)hfnt, TRUE);

			TCListInfo *info = new TCListInfo;
			SetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_USERDATA, (LONG_PTR)info);

			SetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_STYLE,
				GetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_STYLE)|CLS_HIDEEMPTYGROUPS|CLS_USEGROUPS|CLS_GREYALTERNATE);
			SendMessage(GetDlgItem(hwndDlg, IDC_CLIST), CLM_SETEXSTYLE, CLS_EX_DISABLEDRAGDROP|CLS_EX_TRACKSELECT, 0);

			HIMAGELIST hIml = ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),(IsWinVerXPPlus()?ILC_COLOR32:ILC_COLOR16)|ILC_MASK,9,9);
			ImageList_AddIcon(hIml, LoadSkinnedIcon(SKINICON_OTHER_SMALLDOT));
			ImageList_AddIcon(hIml, LoadIconEx("pl_msg_allow"));
			ImageList_AddIcon(hIml, LoadIconEx("pl_msg_deny"));
			ImageList_AddIcon(hIml, LoadIconEx("pl_prin_allow"));
			ImageList_AddIcon(hIml, LoadIconEx("pl_prin_deny"));
			ImageList_AddIcon(hIml, LoadIconEx("pl_prout_allow"));
			ImageList_AddIcon(hIml, LoadIconEx("pl_prout_deny"));
			ImageList_AddIcon(hIml, LoadIconEx("pl_iq_allow"));
			ImageList_AddIcon(hIml, LoadIconEx("pl_iq_deny"));
			SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETEXTRAIMAGELIST, 0, (LPARAM)hIml);
			SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETEXTRACOLUMNS, 4, 0);

			CLCINFOITEM cii = {0};
			cii.cbSize = sizeof(cii);

			cii.flags = CLCIIF_GROUPFONT;
			cii.pszText = TranslateT("** Default **");
			info->hItemDefault = SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_ADDINFOITEM,0,(LPARAM)&cii);
			cii.pszText = TranslateT("** Subsription: both **");
			info->hItemSubBoth = SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_ADDINFOITEM,0,(LPARAM)&cii);
			cii.pszText = TranslateT("** Subsription: to **");
			info->hItemSubTo = SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_ADDINFOITEM,0,(LPARAM)&cii);
			cii.pszText = TranslateT("** Subsription: from **");
			info->hItemSubFrom = SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_ADDINFOITEM,0,(LPARAM)&cii);
			cii.pszText = TranslateT("** Subsription: none **");
			info->hItemSubNone = SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_ADDINFOITEM,0,(LPARAM)&cii);

			sttCListResetOptions(GetDlgItem(hwndDlg, IDC_CLIST));
			sttCListFilter(GetDlgItem(hwndDlg, IDC_CLIST));
			sttCListApplyList(GetDlgItem(hwndDlg, IDC_CLIST));

			static struct
			{
				int idc;
				TCHAR *title;
				char *icon;
				int coreIcon;
				bool push;
			} buttons[] =
			{
				{IDC_ADD_LIST,		_T("Add list..."),		NULL, SKINICON_OTHER_ADDCONTACT,	false},
				{IDC_ACTIVATE,		_T("Activate"),			"pl_list_active", 0,				false},
				{IDC_SET_DEFAULT,	_T("Set default"),		"pl_list_default", 0,				false},
				{IDC_REMOVE_LIST,	_T("Remove list"),		NULL, SKINICON_OTHER_DELETE,		false},
				{IDC_ADD_RULE,		_T("Add rule"),			NULL, SKINICON_OTHER_ADDCONTACT,	false},
				{IDC_EDIT_RULE,		_T("Edit rule"),		NULL, SKINICON_OTHER_RENAME,		false},
				{IDC_UP_RULE,		_T("Move rule up"),		"arrow_up", 0,						false},
				{IDC_DOWN_RULE,		_T("Move rule down"),	"arrow_down", 0,					false},
				{IDC_REMOVE_RULE,	_T("Delete rule"),		NULL, SKINICON_OTHER_DELETE,		false},
				{IDC_APPLY,			_T("Save changes"),		NULL, SKINICON_EVENT_FILE,			false},
				{IDC_ADDJID,		_T("Add JID"),			"addroster", 0,						false},
				{IDC_BTN_SIMPLE,	_T("Simple mode"),		"group", 0,							true},
				{IDC_BTN_ADVANCED,	_T("Advanced mode"),	"sd_view_list", 0,					true},
			};
			for (int i = 0; i < SIZEOF(buttons); ++i)
			{
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BM_SETIMAGE, IMAGE_ICON,
					(LPARAM)(buttons[i].icon ?
						LoadIconEx(buttons[i].icon) :
						LoadSkinnedIcon(buttons[i].coreIcon)));
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONSETASFLATBTN, 0, 0);
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONADDTOOLTIP, (WPARAM)TranslateTS(buttons[i].title), BATF_TCHAR);
				if (buttons[i].push)
					SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONSETASPUSHBTN, 0, 0);
			}

			if (DBGetContactSettingByte(NULL, jabberProtoName, "plistsWnd_simpleMode", 1))
			{
				sttShowControls(hwndDlg, idSimpleControls, SW_SHOW);
				sttShowControls(hwndDlg, idAdvancedControls, SW_HIDE);
				CheckDlgButton(hwndDlg, IDC_BTN_SIMPLE, TRUE);
			} else
			{
				sttShowControls(hwndDlg, idSimpleControls, SW_HIDE);
				sttShowControls(hwndDlg, idAdvancedControls, SW_SHOW);
				CheckDlgButton(hwndDlg, IDC_BTN_ADVANCED, TRUE);
			}

			SetWindowLong(GetDlgItem(hwndDlg, IDC_LB_LISTS), GWL_USERDATA,
				SetWindowLong(GetDlgItem(hwndDlg, IDC_LB_LISTS), GWL_WNDPROC, (LONG)sttLstListsSubclassProc));
			SetWindowLong(GetDlgItem(hwndDlg, IDC_PL_RULES_LIST), GWL_USERDATA,
				SetWindowLong(GetDlgItem(hwndDlg, IDC_PL_RULES_LIST), GWL_WNDPROC, (LONG)sttLstRulesSubclassProc));

			SendDlgItemMessage(hwndDlg, IDC_STATUSBAR, SB_SETMINHEIGHT, GetSystemMetrics(SM_CYSMICON), 0);
			SendDlgItemMessage(hwndDlg, IDC_STATUSBAR, SB_SIMPLE, TRUE, 0);
			sttStatusMessage(TranslateT("Loading..."));

			Utils_RestoreWindowPosition(hwndDlg, NULL, jabberProtoName, "plistsWnd_sz");

			return TRUE;
		}

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
			sttEnableEditorControls(hwndDlg);
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

		if (lpdis->CtlID == IDC_PL_RULES_LIST)
			sttDrawRulesList(hwndDlg, lpdis);
		else if (lpdis->CtlID == IDC_LB_LISTS)
			sttDrawLists(hwndDlg, lpdis);
		else if (lpdis->CtlID == IDC_CANVAS)
		{
			static struct
			{
				TCHAR *textEng;
				char *icon;
				TCHAR *text;
			} items[] = 
			{
				{ _T("Message"),		"pl_msg_allow" },
				{ _T("Presence (in)"),	"pl_prin_allow" },
				{ _T("Presence (out)"),	"pl_prout_allow" },
				{ _T("Query"),			"pl_iq_allow" },
			};

			int i, totalWidth = -5; // spacing for last item
			for (i = 0; i < SIZEOF(items); ++i)
			{
				SIZE sz = {0};
				if (!items[i].text) items[i].text = TranslateTS(items[i].textEng);
				GetTextExtentPoint32(lpdis->hDC, items[i].text, lstrlen(items[i].text), &sz);
				totalWidth += sz.cx + 18 + 5; // 18 pixels for icon, 5 pixel spacing
			}

			COLORREF clText = GetSysColor(COLOR_BTNTEXT);
			RECT rc = lpdis->rcItem;
			rc.left = rc.right - totalWidth;

			for (i = 0; i < SIZEOF(items); ++i)
			{
				DrawIconEx(lpdis->hDC, rc.left, (rc.top+rc.bottom-16)/2, LoadIconEx(items[i].icon),
					16, 16, 0, NULL, DI_NORMAL);
				rc.left += 18;
				sttDrawNextRulePart(lpdis->hDC, clText, items[i].text, &rc);
				rc.left += 5;
			}
		} else
		{
			return FALSE;
		}

		return TRUE;
	}

	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_BTN_SIMPLE:
			{
				CheckDlgButton(hwndDlg, IDC_BTN_SIMPLE, TRUE);
				CheckDlgButton(hwndDlg, IDC_BTN_ADVANCED, FALSE);
				sttShowControls(hwndDlg, idSimpleControls, SW_SHOW);
				sttShowControls(hwndDlg, idAdvancedControls, SW_HIDE);

				sttCListApplyList(GetDlgItem(hwndDlg, IDC_CLIST), GetSelectedList(hwndDlg));

				return TRUE;
			}
			break;

		case IDC_BTN_ADVANCED:
			{
				CheckDlgButton(hwndDlg, IDC_BTN_SIMPLE, FALSE);
				CheckDlgButton(hwndDlg, IDC_BTN_ADVANCED, TRUE);
				sttShowControls(hwndDlg, idSimpleControls, SW_HIDE);
				sttShowControls(hwndDlg, idAdvancedControls, SW_SHOW);

				sttCListBuildList(GetDlgItem(hwndDlg, IDC_CLIST), GetSelectedList(hwndDlg));
				PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_LB_LISTS, LBN_SELCHANGE ), 0 );

				return TRUE;
			}
			break;

		case IDC_ADDJID:
			{
				int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_NEWJID))+1;
				TCHAR *buf = (TCHAR *)_alloca(sizeof(TCHAR) * len);
				GetWindowText(GetDlgItem(hwndDlg, IDC_NEWJID), buf, len);
				SetWindowText(GetDlgItem(hwndDlg, IDC_NEWJID), _T(""));
				sttCListAddContact(GetDlgItem(hwndDlg, IDC_CLIST), buf);
			}
			break;

		case IDC_ACTIVATE:
			if ( jabberOnline ) {
				g_PrivacyListManager.Lock();
				EnableWindow( GetDlgItem( hwndDlg, IDC_ACTIVATE ), FALSE );
				CPrivacyList* pList = GetSelectedList(hwndDlg);
				SetWindowLong( GetDlgItem( hwndDlg, IDC_ACTIVATE ), GWL_USERDATA, (DWORD)pList );
				XmlNodeIq iq( g_JabberIqManager.AddHandler( JabberIqResultPrivacyListActive, JABBER_IQ_TYPE_SET, NULL, 0, -1, pList ) );
				XmlNode* query = iq.addQuery( JABBER_FEAT_PRIVACY_LISTS );
				XmlNode* active = query->addChild( "active" );
				if ( pList )
					active->addAttr( "name", pList->GetListName() );
				g_PrivacyListManager.Unlock();

				sttStatusMessage( TranslateT( JABBER_PL_BUSY_MSG ));

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
				XmlNodeIq iq( g_JabberIqManager.AddHandler( JabberIqResultPrivacyListDefault, JABBER_IQ_TYPE_SET, NULL, 0, -1, pList ) );
				XmlNode* query = iq.addQuery( JABBER_FEAT_PRIVACY_LISTS );
				XmlNode* defaultTag = query->addChild( "default" );
				if ( pList )
					defaultTag->addAttr( "name", pList->GetListName() );
				g_PrivacyListManager.Unlock();

				sttStatusMessage( TranslateT( JABBER_PL_BUSY_MSG ));

				jabberThreadInfo->send( iq );
			}
			return TRUE;

		case IDC_LB_LISTS:
			if ( HIWORD( wParam ) == LBN_SELCHANGE ) {
				int nCurSel = SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_GETCURSEL, 0, 0 );
				if ( nCurSel == LB_ERR )
					return TRUE;

				int nErr = SendDlgItemMessage( hwndDlg, IDC_LB_LISTS, LB_GETITEMDATA, nCurSel, 0 );
				if ( nErr == LB_ERR )
					return TRUE;
				if ( nErr == 0 )
				{
					if (IsWindowVisible(GetDlgItem(hwndDlg, IDC_CLIST)))
					{
						TCListInfo *info = (TCListInfo *)GetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_USERDATA);
						sttCListBuildList(GetDlgItem(hwndDlg, IDC_CLIST), info->pList);
						sttCListApplyList(GetDlgItem(hwndDlg, IDC_CLIST), NULL);
					} else
					{
						EnableWindow( GetDlgItem( hwndDlg, IDC_PL_RULES_LIST ), FALSE );
						SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_RESETCONTENT, 0, 0 );
						SendDlgItemMessage( hwndDlg, IDC_PL_RULES_LIST, LB_ADDSTRING, 0, (LPARAM)TranslateTS(_T("No list selected")));
					}
					sttEnableEditorControls(hwndDlg);
					return TRUE;
				}

				g_PrivacyListManager.Lock();
				if (IsWindowVisible(GetDlgItem(hwndDlg, IDC_CLIST)))
				{
					TCListInfo *info = (TCListInfo *)GetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_USERDATA);
					sttCListBuildList(GetDlgItem(hwndDlg, IDC_CLIST), info->pList);
					sttCListApplyList(GetDlgItem(hwndDlg, IDC_CLIST), (CPrivacyList* )nErr);
				} else
				{
					sttShowAdvancedList(hwndDlg, (CPrivacyList* )nErr);
				}
				g_PrivacyListManager.Unlock();

				sttEnableEditorControls(hwndDlg);
			}
			else if ( HIWORD( wParam ) == LBN_DBLCLK )
				PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_ACTIVATE, 0 ), 0 );
			return TRUE;

		case IDC_PL_RULES_LIST:
			if ( HIWORD( wParam ) == LBN_SELCHANGE ) {
				sttEnableEditorControls(hwndDlg);
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
			if ( !jabberOnline ) {
				sttStatusMessage(TranslateT("Unable to save list because you are currently offline."));
				return TRUE;
			}

			g_PrivacyListManager.Lock();
			{
				if (IsWindowVisible(GetDlgItem(hwndDlg, IDC_CLIST)))
				{
					TCListInfo *info = (TCListInfo *)GetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_USERDATA);
					sttCListBuildList(GetDlgItem(hwndDlg, IDC_CLIST), info->pList);
				}

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
			sttStatusMessage( TranslateT( JABBER_PL_BUSY_MSG ));
			PostMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
			return TRUE;

		case IDCLOSE:
		case IDCANCEL:
			if (IsWindowVisible(GetDlgItem(hwndDlg, IDC_CLIST)))
			{
				TCListInfo *info = (TCListInfo *)GetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_USERDATA);
				sttCListBuildList(GetDlgItem(hwndDlg, IDC_CLIST), info->pList);
			}
			if ( JabberPrivacyListDlgCanExit( hwndDlg ))
				DestroyWindow( hwndDlg );
			return TRUE;
		}
		break;

	case WM_NOTIFY:
	{
		switch(((LPNMHDR)lParam)->idFrom)
		{
			case IDC_CLIST:
			{
				switch (((LPNMHDR)lParam)->code)
				{
					case CLN_NEWCONTACT:
					case CLN_LISTREBUILT:
						sttCListFilter(GetDlgItem(hwndDlg, IDC_CLIST));
						sttCListApplyList(GetDlgItem(hwndDlg, IDC_CLIST), GetSelectedList(hwndDlg));
						break;
					case CLN_OPTIONSCHANGED:
						sttCListResetOptions(GetDlgItem(hwndDlg, IDC_CLIST));
						sttCListApplyList(GetDlgItem(hwndDlg, IDC_CLIST), GetSelectedList(hwndDlg));
						break;
					case NM_CLICK:
					{
						HANDLE hItem;
						NMCLISTCONTROL *nm=(NMCLISTCONTROL*)lParam;
						DWORD hitFlags;
						int iImage;

						if(nm->iColumn==-1) break;
						hItem=(HANDLE)SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_HITTEST,(WPARAM)&hitFlags,MAKELPARAM(nm->pt.x,nm->pt.y));
						if(hItem==NULL) break;
						if(!(hitFlags&CLCHT_ONITEMEXTRA)) break;

						iImage = SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_GETEXTRAIMAGE,(WPARAM)hItem,MAKELPARAM(nm->iColumn,0));
						if (iImage != 0xFF)
						{
							if (iImage == 0)
								iImage = nm->iColumn * 2 + 2;
							else if (iImage == nm->iColumn * 2 + 2)
								iImage = nm->iColumn * 2 + 1;
							else
								iImage = 0;

							SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM(nm->iColumn, iImage));

							TCListInfo *info = (TCListInfo *)GetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_USERDATA);
							info->bChanged = true;
							g_PrivacyListManager.SetModified();

							sttEnableEditorControls(hwndDlg);
						}

						break;
					}
				}
				break;
			}
		}
		break;
	}

	case WM_CLOSE:
		if ( JabberPrivacyListDlgCanExit( hwndDlg ))
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

		// Wipe all data and query lists without contents
		g_PrivacyListManager.RemoveAllLists();
		g_PrivacyListManager.QueryLists();
		g_PrivacyListManager.SetModified( FALSE );

		// Delete custom bold font
		DeleteObject((HFONT)SendDlgItemMessage(hwndDlg, IDC_TXT_LISTS, WM_GETFONT, 0, 0));

		{
			TCListInfo *info = (TCListInfo *)GetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_USERDATA);
			delete info;
		}

		DBWriteContactSettingByte(NULL, jabberProtoName, "plistsWnd_simpleMode", IsDlgButtonChecked(hwndDlg, IDC_BTN_SIMPLE));

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

/////////////////////////////////////////////////////////////////////////////////////////
// builds privacy menu
static int menuSetPrivacyList( WPARAM wParam, LPARAM lParam, LPARAM iList )
{
	g_PrivacyListManager.Lock();
	CPrivacyList *pList = NULL;

	if (iList)
	{
		pList = g_PrivacyListManager.GetFirstList();
		for (int i = 1; pList && (i < iList); ++i)
			pList = pList->GetNext();
	}

	XmlNodeIq iq( g_JabberIqManager.AddHandler( JabberIqResultPrivacyListActive, JABBER_IQ_TYPE_SET, NULL, 0, -1, pList ) );
	XmlNode* query = iq.addQuery( JABBER_FEAT_PRIVACY_LISTS );
	XmlNode* active = query->addChild( "active" );
	if ( pList )
		active->addAttr( "name", pList->GetListName() );
	g_PrivacyListManager.Unlock();

	jabberThreadInfo->send( iq );

	return 0;
}

static int CListMW_BuildPrivacyMenu( WPARAM wParam, LPARAM lParam )
{
	if ( !jabberOnline )
		return 0;

	static int serviceAllocated = -1;

	CLISTMENUITEM mi = { 0 };
	int i=0;
	char srvFce[MAX_PATH + 64];
	char szItem[MAX_PATH + 64];
	HANDLE hPrivacyRoot;
	HANDLE hRoot = ( HANDLE )szItem;

	mir_snprintf( szItem, sizeof(szItem), LPGEN("Privacy Lists") );

	mi.cbSize = sizeof(mi);
	mi.popupPosition= 500084000;
	mi.pszContactOwner = jabberProtoName;
	mi.pszService = srvFce;
	mi.pszPopupName = (char *)hRoot;

	mi.position = 3000040000;
	mi.flags = CMIF_TCHAR|CMIF_ICONFROMICOLIB;

	mi.icolibItem = GetIconHandle(IDI_PRIVACY_LISTS);
	mi.ptszName = LPGENT("List Editor...");
	mir_snprintf( srvFce, sizeof(srvFce), "%s/PrivacyLists", jabberProtoName );
	CallService( MS_CLIST_ADDSTATUSMENUITEM, ( WPARAM )&hPrivacyRoot, ( LPARAM )&mi );

	{
		CLISTMENUITEM miTmp = {0};
		miTmp.cbSize = sizeof(miTmp);
		miTmp.flags = CMIM_ICON|CMIM_FLAGS | CMIF_ICONFROMICOLIB;
		miTmp.icolibItem = GetIconHandle(IDI_PRIVACY_LISTS);
		CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hPrivacyRoot, (LPARAM)&miTmp);
	}

	mi.position = 2000040000;
	mi.flags = CMIF_ICONFROMICOLIB|CMIF_TCHAR;

	g_PrivacyListManager.Lock();

	mir_snprintf( srvFce, sizeof(srvFce), "%s/menuPrivacy%d", jabberProtoName, i );
	if ( i > serviceAllocated )
	{
		CreateServiceFunctionParam( srvFce, menuSetPrivacyList, i );
		serviceAllocated = i;
	}
	mi.position++;
	mi.icolibItem = LoadSkinnedIconHandle(
		g_PrivacyListManager.GetActiveListName() ?
			SKINICON_OTHER_SMALLDOT :
			SKINICON_OTHER_EMPTYBLOB);
	mi.ptszName = LPGENT("<none>");
	CallService( MS_CLIST_ADDSTATUSMENUITEM, ( WPARAM )&hPrivacyRoot, ( LPARAM )&mi );

	for (CPrivacyList *pList = g_PrivacyListManager.GetFirstList(); pList; pList = pList->GetNext())
	{
		++i;
		mir_snprintf( srvFce, sizeof(srvFce), "%s/menuPrivacy%d", jabberProtoName, i );

		if ( i > serviceAllocated )
		{
			CreateServiceFunctionParam( srvFce, menuSetPrivacyList, i );
			serviceAllocated = i;
		}

		mi.position++;
		mi.icolibItem = LoadSkinnedIconHandle(
			lstrcmp(g_PrivacyListManager.GetActiveListName(), pList->GetListName()) ?
				SKINICON_OTHER_SMALLDOT :
				SKINICON_OTHER_EMPTYBLOB);
		mi.ptszName = pList->GetListName();
		CallService( MS_CLIST_ADDSTATUSMENUITEM, ( WPARAM )&hPrivacyRoot, ( LPARAM )&mi );
	}

	g_PrivacyListManager.Unlock();

	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////
// init privacy menu

static HANDLE hhkPrebuildStatusMenu = NULL;
void JabberPrivacyInit()
{
	hhkPrebuildStatusMenu = HookEvent( ME_CLIST_PREBUILDSTATUSMENU, CListMW_BuildPrivacyMenu );
}

void JabberPrivacyUninit()
{
	UnhookEvent(hhkPrebuildStatusMenu);
}
