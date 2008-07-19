/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
Copyright ( C ) 2007-08  Maxim Mluhov
Copyright ( C ) 2007-08  Victor Pavlychko

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
#include "jabber_privacy.h"

#include "m_genmenu.h"
#include "m_clistint.h"

#define JABBER_PL_BUSY_MSG	 "Sending request, please wait..."

void CJabberProto::OnIqRequestPrivacyLists( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_SET ) {
		if ( !m_pDlgPrivacyLists )
		{
			m_privacyListManager.RemoveAllLists();
			QueryPrivacyLists();
		}
		else m_pDlgPrivacyLists->SetStatusText(TranslateT("Warning: privacy lists were changed on server."));

		XmlNodeIq iq( "result", pInfo );
		m_ThreadInfo->send( iq );
}	}

void CJabberProto::OnIqResultPrivacyListModify( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if ( !pInfo->m_pUserData )
		return;

	CPrivacyListModifyUserParam *pParam = ( CPrivacyListModifyUserParam * )pInfo->m_pUserData;

	if ( pInfo->m_nIqType != JABBER_IQ_TYPE_RESULT )
		pParam->m_bAllOk = FALSE;

	if ( !m_iqManager.GetGroupPendingIqCount( pInfo->m_dwGroupId )) {
		TCHAR szText[ 512 ];
		if ( !pParam->m_bAllOk )
			mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Error occurred while applying changes"));
		else
			mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Privacy lists successfully saved"));
		if (m_pDlgPrivacyLists)
			m_pDlgPrivacyLists->SetStatusText( szText );
		// FIXME: enable apply button
		delete pParam;
	}
}

void CJabberProto::OnIqResultPrivacyList( XmlNode* iqNode, void* userdata )
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
		m_privacyListManager.Lock();
		CPrivacyList* pList = m_privacyListManager.FindList( szListName );
		if ( !pList ) {
			m_privacyListManager.AddList( szListName );
			pList = m_privacyListManager.FindList( szListName );
			if ( !pList ) {
				m_privacyListManager.Unlock();
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
		m_privacyListManager.Unlock();

		UI_SAFE_NOTIFY(m_pDlgPrivacyLists, WM_JABBER_REFRESH);
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

void CJabberProto::OnIqResultPrivacyListActive( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	CPrivacyList *pList = (CPrivacyList *)pInfo->GetUserData();

	if ( m_pDlgPrivacyLists )
		EnableWindow( GetDlgItem( m_pDlgPrivacyLists->GetHwnd(), IDC_ACTIVATE ), TRUE );

	if ( !iqNode )
		return;

	TCHAR *type = JabberXmlGetAttrValue( iqNode, "type" );
	if ( !type )
		return;

	TCHAR szText[ 512 ];
	szText[0] = _T('\0');
	m_privacyListManager.Lock();
	if ( !_tcscmp( type, _T("result"))) {
		if ( pList ) {
			m_privacyListManager.SetActiveListName( pList->GetListName() );
			mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Privacy list %s set as active"), pList->GetListName() );
		}
		else {
			m_privacyListManager.SetActiveListName( NULL );
			mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Active privacy list successfully declined"));
		}
	}
	else {
		mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Error occurred while setting active list") );
	}
	m_privacyListManager.Unlock();

	if ( m_pDlgPrivacyLists )
	{
		m_pDlgPrivacyLists->SetStatusText( szText );
		RedrawWindow(GetDlgItem(m_pDlgPrivacyLists->GetHwnd(), IDC_LB_LISTS), NULL, NULL, RDW_INVALIDATE);
	}

	RebuildStatusMenu();
}

void CJabberProto::OnIqResultPrivacyListDefault( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	CPrivacyList *pList = (CPrivacyList *)pInfo->GetUserData();

	if ( m_pDlgPrivacyLists )
		EnableWindow( GetDlgItem( m_pDlgPrivacyLists->GetHwnd(), IDC_SET_DEFAULT ), TRUE );

	if ( !iqNode )
		return;

	TCHAR *type = JabberXmlGetAttrValue( iqNode, "type" );
	if ( !type )
		return;

	TCHAR szText[ 512 ];
	szText[0] = _T('\0');
	m_privacyListManager.Lock();
	if ( !_tcscmp( type, _T("result"))) {
		if ( pList ) {
			m_privacyListManager.SetDefaultListName( pList->GetListName() );
			mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Privacy list %s set as default"), pList->GetListName() );
		}
		else {
			m_privacyListManager.SetDefaultListName( NULL );
			mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Default privacy list successfully declined"));
		}
	}
	else {
		mir_sntprintf( szText, SIZEOF( szText ), TranslateT("Error occurred while setting default list") );
	}
	m_privacyListManager.Unlock();

	if ( m_pDlgPrivacyLists )
	{
		m_pDlgPrivacyLists->SetStatusText( szText );
		RedrawWindow(GetDlgItem(m_pDlgPrivacyLists->GetHwnd(), IDC_LB_LISTS), NULL, NULL, RDW_INVALIDATE);
	}
}

void CJabberProto::OnIqResultPrivacyLists( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if ( pInfo->m_nIqType != JABBER_IQ_TYPE_RESULT )
		return;

	XmlNode *query = JabberXmlGetChild( iqNode, "query" );
	if ( !query )
		return;

	if ( m_ThreadInfo )
		m_ThreadInfo->jabberServerCaps |= JABBER_CAPS_PRIVACY_LISTS;

	m_privacyListManager.Lock();
	m_privacyListManager.RemoveAllLists();

	XmlNode *list;
	for ( int i = 1; ( list = JabberXmlGetNthChild( query, "list", i )) != NULL; i++ ) {
		TCHAR *listName = JabberXmlGetAttrValue( list, "name" );
		if ( listName ) {
			m_privacyListManager.AddList(listName);

			// Query contents only if list editior is visible!
			if ( m_pDlgPrivacyLists ) {
				int iqId = SerialNext();
				IqAdd( iqId, IQ_PROC_NONE, &CJabberProto::OnIqResultPrivacyList);
				XmlNodeIq iq( "get", iqId);
				XmlNode* queryNode = iq.addQuery( JABBER_FEAT_PRIVACY_LISTS );
				XmlNode* listNode = queryNode->addChild( "list" );
				listNode->addAttr( "name", listName );
				m_ThreadInfo->send( iq );
		}	}	}

	TCHAR *szName = NULL;
	XmlNode *node = JabberXmlGetChild( query, "active" );
	if ( node )
		szName = JabberXmlGetAttrValue( node, "name" );
	m_privacyListManager.SetActiveListName( szName );

	szName = NULL;
	node = JabberXmlGetChild( query, "default" );
	if ( node )
		szName = JabberXmlGetAttrValue( node, "name" );
	m_privacyListManager.SetDefaultListName( szName );

	m_privacyListManager.Unlock();

	UI_SAFE_NOTIFY(m_pDlgPrivacyLists, WM_JABBER_REFRESH);

	RebuildStatusMenu();
}

/////////////////////////////////////////////////////////////////////////////////////////
// Add privacy list box
class CJabberDlgPrivacyAddList: public CJabberDlgBase
{
	typedef CJabberDlgBase CSuper;

public:
	TCHAR szLine[512];

	CJabberDlgPrivacyAddList(CJabberProto *proto, HWND hwndParent):
		CJabberDlgBase(proto, IDD_PRIVACY_ADD_LIST, hwndParent, false),
		m_txtName(this, IDC_EDIT_NAME),
		m_btnOk(this, IDOK),
		m_btnCancel(this, IDCANCEL)
	{
		m_btnOk.OnClick = Callback( this, &CJabberDlgPrivacyAddList::btnOk_OnClick );
		m_btnCancel.OnClick = Callback( this, &CJabberDlgPrivacyAddList::btnCancel_OnClick);
	}

	void btnOk_OnClick(CCtrlButton *btn)
	{
		GetDlgItemText(m_hwnd, IDC_EDIT_NAME, szLine, SIZEOF(szLine));
		EndDialog(m_hwnd, 1);
	}
	void btnCancel_OnClick(CCtrlButton *btn)
	{
		*szLine = 0;
		EndDialog(m_hwnd, 0);
	}

private:
	CCtrlEdit	m_txtName;
	CCtrlButton	m_btnOk;
	CCtrlButton	m_btnCancel;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Privacy rule editor
class CJabberDlgPrivacyRule: public CJabberDlgBase
{
	typedef CJabberDlgBase CSuper;

	CCtrlButton	m_btnOk;
	CCtrlButton	m_btnCancel;
	CCtrlCombo	m_cbType;

public:
	CPrivacyListRule *m_pRule;

	CJabberDlgPrivacyRule(CJabberProto *proto, HWND hwndParent, CPrivacyListRule *pRule):
		CJabberDlgBase(proto, IDD_PRIVACY_RULE, hwndParent, false),
		m_btnOk(this, IDOK),
		m_btnCancel(this, IDCANCEL),
		m_cbType(this, IDC_COMBO_TYPE)
	{
		m_pRule = pRule;
		m_cbType.OnChange = Callback(this, &CJabberDlgPrivacyRule::cbType_OnChange);
		m_btnOk.OnClick = Callback(this, &CJabberDlgPrivacyRule::btnOk_OnClick);
		m_btnCancel.OnClick = Callback(this, &CJabberDlgPrivacyRule::btnCancel_OnClick);
	}

	virtual void OnInitDialog()
	{
		CSuper::OnInitDialog();

		m_proto->m_hwndPrivacyRule = m_hwnd;

		SendDlgItemMessage(m_hwnd, IDC_ICO_MESSAGE,     STM_SETICON, (WPARAM)m_proto->LoadIconEx("pl_msg_allow"), 0);
		SendDlgItemMessage(m_hwnd, IDC_ICO_QUERY,       STM_SETICON, (WPARAM)m_proto->LoadIconEx("pl_iq_allow"), 0);
		SendDlgItemMessage(m_hwnd, IDC_ICO_PRESENCEIN,  STM_SETICON, (WPARAM)m_proto->LoadIconEx("pl_prin_allow"), 0);
		SendDlgItemMessage(m_hwnd, IDC_ICO_PRESENCEOUT, STM_SETICON, (WPARAM)m_proto->LoadIconEx("pl_prout_allow"), 0);

		TCHAR* szTypes[] = { _T("JID"), _T("Group"), _T("Subscription"), _T("Any") };
		int i, nTypes[] = { Jid, Group, Subscription, Else };
		for ( i = 0; i < SIZEOF(szTypes); i++ )
		{
			int nItem = SendDlgItemMessage( m_hwnd, IDC_COMBO_TYPE, CB_ADDSTRING, 0, (LPARAM)TranslateTS( szTypes[i] ));
			SendDlgItemMessage( m_hwnd, IDC_COMBO_TYPE, CB_SETITEMDATA, nItem, nTypes[i] );
			if ( m_pRule->GetType() == nTypes[i] )
				SendDlgItemMessage( m_hwnd, IDC_COMBO_TYPE, CB_SETCURSEL, nItem, 0 );
		}

		SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUE, CB_RESETCONTENT, 0, 0 );
		TCHAR* szSubscriptions[] = { _T("none"), _T("from"), _T("to"), _T("both") };
		for ( i = 0; i < SIZEOF(szSubscriptions); i++ )
		{
			int nItem = SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUE, CB_ADDSTRING, 0, (LPARAM)TranslateTS( szSubscriptions[i] ));
			SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUE, CB_SETITEMDATA, nItem, (LPARAM)szSubscriptions[i] );
		}

		PostMessage( m_hwnd, WM_COMMAND, MAKEWPARAM( IDC_COMBO_TYPE, CBN_SELCHANGE ), 0 );

		SendDlgItemMessage( m_hwnd, IDC_COMBO_ACTION, CB_ADDSTRING, 0, (LPARAM)TranslateTS( _T("Deny" )));
		SendDlgItemMessage( m_hwnd, IDC_COMBO_ACTION, CB_ADDSTRING, 0, (LPARAM)TranslateTS( _T("Allow" )));

		SendDlgItemMessage( m_hwnd, IDC_COMBO_ACTION, CB_SETCURSEL, m_pRule->GetAction() ? 1 : 0, 0 );

		DWORD dwPackets = m_pRule->GetPackets();
		if ( !dwPackets )
			dwPackets = JABBER_PL_RULE_TYPE_ALL;
		if ( dwPackets & JABBER_PL_RULE_TYPE_IQ )
			SendDlgItemMessage( m_hwnd, IDC_CHECK_QUERIES, BM_SETCHECK, BST_CHECKED, 0 );
		if ( dwPackets & JABBER_PL_RULE_TYPE_MESSAGE )
			SendDlgItemMessage( m_hwnd, IDC_CHECK_MESSAGES, BM_SETCHECK, BST_CHECKED, 0 );
		if ( dwPackets & JABBER_PL_RULE_TYPE_PRESENCE_IN )
			SendDlgItemMessage( m_hwnd, IDC_CHECK_PRESENCE_IN, BM_SETCHECK, BST_CHECKED, 0 );
		if ( dwPackets & JABBER_PL_RULE_TYPE_PRESENCE_OUT )
			SendDlgItemMessage( m_hwnd, IDC_CHECK_PRESENCE_OUT, BM_SETCHECK, BST_CHECKED, 0 );

		if ( m_pRule->GetValue() && ( m_pRule->GetType() == Jid || m_pRule->GetType() == Group ))
			SetDlgItemText( m_hwnd, IDC_EDIT_VALUE, m_pRule->GetValue() );
	}

	void cbType_OnChange(CCtrlData *cb)
	{
		if ( !m_pRule ) return;

		int nCurSel = SendDlgItemMessage( m_hwnd, IDC_COMBO_TYPE, CB_GETCURSEL, 0, 0 );
		if ( nCurSel == CB_ERR )
			return;

		int nItemData = SendDlgItemMessage( m_hwnd, IDC_COMBO_TYPE, CB_GETITEMDATA, nCurSel, 0 );
		switch (nItemData)
		{
			case Jid:
			{
				ShowWindow( GetDlgItem( m_hwnd, IDC_COMBO_VALUES ), SW_SHOW );
				ShowWindow( GetDlgItem( m_hwnd, IDC_COMBO_VALUE ), SW_HIDE );

				SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUES, CB_RESETCONTENT, 0, 0 );

				HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
				while ( hContact != NULL )
				{
					char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
					if ( szProto != NULL && !strcmp( szProto, m_proto->m_szModuleName ))
					{
						DBVARIANT dbv;
						if ( !m_proto->JGetStringT( hContact, "jid", &dbv ))
						{
							SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUES, CB_ADDSTRING, 0, (LPARAM)dbv.ptszVal );
							JFreeVariant( &dbv );
						}
					}
					hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
				}

				// append known chatroom jids from bookmarks
				int i = 0;
				while (( i = m_proto->ListFindNext( LIST_BOOKMARK, i )) >= 0 )
				{
					JABBER_LIST_ITEM *item = 0;
					if ( item = m_proto->ListGetItemPtrFromIndex( i ))
						SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUES, CB_ADDSTRING, 0, (LPARAM)item->jid );
					i++;
				}

				// FIXME: ugly code :)
				if ( m_pRule->GetValue() )
				{
					SetDlgItemText( m_hwnd, IDC_COMBO_VALUES, m_pRule->GetValue() );
					int nSelPos = SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUES, CB_FINDSTRINGEXACT , -1, (LPARAM)m_pRule->GetValue() );
					if ( nSelPos != CB_ERR )
						SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUES, CB_SETCURSEL, nSelPos, 0 );
				}
				break;
			}

			case Group:
			{
				ShowWindow( GetDlgItem( m_hwnd, IDC_COMBO_VALUES ), SW_SHOW );
				ShowWindow( GetDlgItem( m_hwnd, IDC_COMBO_VALUE ), SW_HIDE );

				SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUES, CB_RESETCONTENT, 0, 0 );

				char buf[ 20 ];
				DBVARIANT dbv;
				for ( int i = 0; ; i++ )
				{
					mir_snprintf(buf, 20, "%d", i);
					if ( DBGetContactSettingTString(NULL, "CListGroups", buf, &dbv) )
						break;

					SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUES, CB_ADDSTRING, 0, (LPARAM)&dbv.ptszVal[1] );
					DBFreeVariant(&dbv);
				}

				// FIXME: ugly code :)
				if ( m_pRule->GetValue() )
				{
					SetDlgItemText( m_hwnd, IDC_COMBO_VALUES, m_pRule->GetValue() );
					int nSelPos = SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUES, CB_FINDSTRINGEXACT , -1, (LPARAM)m_pRule->GetValue() );
					if ( nSelPos != CB_ERR )
						SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUES, CB_SETCURSEL, nSelPos, 0 );
				}
				break;
			}

			case Subscription:
			{
				ShowWindow( GetDlgItem( m_hwnd, IDC_COMBO_VALUES ), SW_HIDE );
				ShowWindow( GetDlgItem( m_hwnd, IDC_COMBO_VALUE ), SW_SHOW );

				if ( m_pRule->GetValue() )
				{
					int nSelected = SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUE, CB_SELECTSTRING, -1, (LPARAM)TranslateTS(m_pRule->GetValue()) );
					if ( nSelected == CB_ERR )
						SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUE, CB_SETCURSEL, 0, 0 );
				}
				else SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUE, CB_SETCURSEL, 0, 0 );
				break;
			}

			case Else:
			{
				ShowWindow( GetDlgItem( m_hwnd, IDC_COMBO_VALUES ), SW_HIDE );
				ShowWindow( GetDlgItem( m_hwnd, IDC_COMBO_VALUE ), SW_HIDE );
				break;
			}
		}

		return;
	}

	void btnOk_OnClick(CCtrlButton *)
	{
		int nItemData = -1;
		int nCurSel = SendDlgItemMessage( m_hwnd, IDC_COMBO_TYPE, CB_GETCURSEL, 0, 0 );
		if ( nCurSel != CB_ERR )
			nItemData = SendDlgItemMessage( m_hwnd, IDC_COMBO_TYPE, CB_GETITEMDATA, nCurSel, 0 );

		switch ( nItemData )
		{
			case Jid:
			case Group:
			{
				TCHAR szText[ 512 ];
				GetDlgItemText( m_hwnd, IDC_COMBO_VALUES, szText, SIZEOF(szText) );
				m_pRule->SetValue( szText );
				break;
			}
			case Subscription:
			{
				nCurSel = SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUE, CB_GETCURSEL, 0, 0 );
				TCHAR *szSubs = NULL;
				if ( nCurSel != CB_ERR )
				{
					m_pRule->SetValue( ( TCHAR * )SendDlgItemMessage( m_hwnd, IDC_COMBO_VALUE, CB_GETITEMDATA, nCurSel, 0 ));
				} else
				{
					m_pRule->SetValue( _T( "none" ));
				}
				break;
			}

			default:
				m_pRule->SetValue( NULL );
				break;
		}

		m_pRule->SetType( ( PrivacyListRuleType )nItemData );
		nCurSel = SendDlgItemMessage( m_hwnd, IDC_COMBO_ACTION, CB_GETCURSEL, 0, 0 );
		if ( nCurSel == CB_ERR )
			nCurSel = 1;
		m_pRule->SetAction( nCurSel ? TRUE : FALSE );

		DWORD dwPackets = 0;
		if ( BST_CHECKED == SendDlgItemMessage( m_hwnd, IDC_CHECK_MESSAGES, BM_GETCHECK, 0, 0 ))
			dwPackets |= JABBER_PL_RULE_TYPE_MESSAGE;
		if ( BST_CHECKED == SendDlgItemMessage( m_hwnd, IDC_CHECK_PRESENCE_IN, BM_GETCHECK, 0, 0 ))
			dwPackets |= JABBER_PL_RULE_TYPE_PRESENCE_IN;
		if ( BST_CHECKED == SendDlgItemMessage( m_hwnd, IDC_CHECK_PRESENCE_OUT, BM_GETCHECK, 0, 0 ))
			dwPackets |= JABBER_PL_RULE_TYPE_PRESENCE_OUT;
		if ( BST_CHECKED == SendDlgItemMessage( m_hwnd, IDC_CHECK_QUERIES, BM_GETCHECK, 0, 0 ))
			dwPackets |= JABBER_PL_RULE_TYPE_IQ;
		if ( !dwPackets )
			dwPackets = JABBER_PL_RULE_TYPE_ALL;

		m_pRule->SetPackets( dwPackets );

		EndDialog( m_hwnd, 1 );
	}

	void btnCancel_OnClick(CCtrlButton *)
	{
		EndDialog(m_hwnd, 0);
	}

	void OnDestroy()
	{
		m_proto->m_hwndPrivacyRule = NULL;
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// Main privacy list dialog
class CJabberDlgPrivacyLists: public CJabberDlgFancy
{
	typedef CJabberDlgFancy CSuper;

public:
	CJabberDlgPrivacyLists(CJabberProto *proto);

protected:
	static int idSimpleControls[];
	static int idAdvancedControls[];

	void OnInitDialog();
	void OnClose();
	void OnDestroy();
	void OnProtoRefresh(WPARAM, LPARAM);
	int Resizer(UTILRESIZECONTROL *urc);

	UI_MESSAGE_MAP(CJabberDlgPrivacyLists, CSuper);
		UI_MESSAGE(WM_MEASUREITEM, OnWmMeasureItem);
		UI_MESSAGE(WM_DRAWITEM, OnWmDrawItem);
		UI_MESSAGE(WM_GETMINMAXINFO, OnWmGetMinMaxInfo);
	UI_MESSAGE_MAP_END();

	BOOL OnWmMeasureItem(UINT msg, WPARAM wParam, LPARAM lParam);
	BOOL OnWmDrawItem(UINT msg, WPARAM wParam, LPARAM lParam);
	BOOL OnWmGetMinMaxInfo(UINT msg, WPARAM wParam, LPARAM lParam);

	void btnSimple_OnClick(CCtrlButton *);
	void btnAdvanced_OnClick(CCtrlButton *);
	void btnAddJid_OnClick(CCtrlButton *);
	void btnActivate_OnClick(CCtrlButton *);
	void btnSetDefault_OnClick(CCtrlButton *);
	void lbLists_OnSelChange(CCtrlListBox *);
	void lbLists_OnDblClick(CCtrlListBox *);
	void lbRules_OnSelChange(CCtrlListBox *);
	void lbRules_OnDblClick(CCtrlListBox *);
	void btnEditRule_OnClick(CCtrlButton *);
	void btnAddRule_OnClick(CCtrlButton *);
	void btnRemoveRule_OnClick(CCtrlButton *);
	void btnUpRule_OnClick(CCtrlButton *);
	void btnDownRule_OnClick(CCtrlButton *);
	void btnAddList_OnClick(CCtrlButton *);
	void btnRemoveList_OnClick(CCtrlButton *);
	void btnApply_OnClick(CCtrlButton *);
	void clcClist_OnUpdate(CCtrlClc::TEventInfo *evt);
	void clcClist_OnOptionsChanged(CCtrlClc::TEventInfo *evt);
	void clcClist_OnClick(CCtrlClc::TEventInfo *evt);

	void OnCommand_Close(HWND hwndCtrl, WORD idCtrl, WORD idCode);

	void ShowAdvancedList(CPrivacyList *pList);
	void DrawNextRulePart(HDC hdc, COLORREF color, const TCHAR *text, RECT *rc);
	void DrawRuleAction(HDC hdc, COLORREF clLine1, COLORREF clLine2, CPrivacyListRule *pRule, RECT *rc);
	void DrawRulesList(LPDRAWITEMSTRUCT lpdis);
	void DrawLists(LPDRAWITEMSTRUCT lpdis);

	void CListResetOptions(HWND hwndList);
	void CListFilter(HWND hwndList);
	bool CListIsGroup(HANDLE hGroup);
	HANDLE CListFindGroupByName(TCHAR *name);
	void CListResetIcons(HWND hwndList, HANDLE hItem, bool hide=false);
	void CListSetupIcons(HWND hwndList, HANDLE hItem, int iSlot, DWORD dwProcess, BOOL bAction);
	HANDLE CListAddContact(HWND hwndList, TCHAR *jid);
	void CListApplyList(HWND hwndList, CPrivacyList *pList = NULL);
	DWORD CListGetPackets(HWND hwndList, HANDLE hItem, bool bAction);
	void CListBuildList(HWND hwndList, CPrivacyList *pList);

	void EnableEditorControls();
	BOOL CanExit();
	
	static LRESULT CALLBACK LstListsSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK LstRulesSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	struct TCLCInfo
	{
		struct TJidData
		{
			HANDLE hItem;
			TCHAR *jid;

			static int cmp(const TJidData *p1, const TJidData *p2) { return lstrcmp(p1->jid, p2->jid); }
		};

		HANDLE hItemDefault;
		HANDLE hItemSubNone;
		HANDLE hItemSubTo;
		HANDLE hItemSubFrom;
		HANDLE hItemSubBoth;

		LIST<TJidData> newJids;

		bool bChanged;

		CPrivacyList *pList;

		TCLCInfo(): newJids(1, TJidData::cmp), bChanged(false), pList(0) {}
		~TCLCInfo()
		{
			for (int i = 0; i < newJids.getCount(); ++i)
			{
				mir_free(newJids[i]->jid);
				mir_free(newJids[i]);
			}
			newJids.destroy();
		}

		void addJid(HANDLE hItem, TCHAR *jid)
		{
			TJidData *data = (TJidData *)mir_alloc(sizeof(TJidData));
			data->hItem = hItem;
			data->jid = mir_tstrdup(jid);
			newJids.insert(data);
		}

		HANDLE findJid(TCHAR *jid)
		{
			TJidData data = {0};
			data.jid = jid;
			TJidData *found = newJids.find(&data);
			return found ? found->hItem : 0;
		}
	};

	TCLCInfo clc_info;

private:
	CCtrlMButton	m_btnSimple;
	CCtrlMButton	m_btnAdvanced;
	CCtrlMButton	m_btnAddJid;
	CCtrlMButton	m_btnActivate;
	CCtrlMButton	m_btnSetDefault;
	CCtrlMButton	m_btnEditRule;
	CCtrlMButton	m_btnAddRule;
	CCtrlMButton	m_btnRemoveRule;
	CCtrlMButton	m_btnUpRule;
	CCtrlMButton	m_btnDownRule;
	CCtrlMButton	m_btnAddList;
	CCtrlMButton	m_btnRemoveList;
	CCtrlMButton	m_btnApply;
	CCtrlListBox	m_lbLists;
	CCtrlListBox	m_lbRules;
	CCtrlClc		m_clcClist;
};

int CJabberDlgPrivacyLists::idSimpleControls[] =
{
	IDC_CLIST, IDC_CANVAS,
	IDC_TXT_OTHERJID, IDC_NEWJID, IDC_ADDJID,
	IDC_ICO_MESSAGE, IDC_ICO_QUERY, IDC_ICO_INPRESENCE, IDC_ICO_OUTPRESENCE,
	IDC_TXT_MESSAGE, IDC_TXT_QUERY, IDC_TXT_INPRESENCE, IDC_TXT_OUTPRESENCE,
	0
};

int CJabberDlgPrivacyLists::idAdvancedControls[] =
{
	IDC_PL_RULES_LIST,
	IDC_ADD_RULE, IDC_EDIT_RULE, IDC_REMOVE_RULE,
	IDC_UP_RULE, IDC_DOWN_RULE,
	0
};

CJabberDlgPrivacyLists::CJabberDlgPrivacyLists(CJabberProto *proto):
	CJabberDlgFancy(proto, IDD_PRIVACY_LISTS, NULL),
	m_btnSimple(this,     IDC_BTN_SIMPLE,   proto->LoadIconEx("group"),           LPGEN("Simple mode")),
	m_btnAdvanced(this,   IDC_BTN_ADVANCED, proto->LoadIconEx("sd_view_list"),    LPGEN("Advanced mode")),
	m_btnAddJid(this,     IDC_ADDJID,       proto->LoadIconEx("addroster"),       LPGEN("Add JID")),
	m_btnActivate(this,   IDC_ACTIVATE,     proto->LoadIconEx("pl_list_active"),  LPGEN("Activate")),
	m_btnSetDefault(this, IDC_SET_DEFAULT,  proto->LoadIconEx("pl_list_default"),	LPGEN("Set default")),
	m_btnEditRule(this,   IDC_EDIT_RULE,    SKINICON_OTHER_RENAME,                LPGEN("Edit rule")),
	m_btnAddRule(this,    IDC_ADD_RULE,     SKINICON_OTHER_ADDCONTACT,				LPGEN("Add rule")),
	m_btnRemoveRule(this, IDC_REMOVE_RULE,  SKINICON_OTHER_DELETE,                LPGEN("Delete rule")),
	m_btnUpRule(this,     IDC_UP_RULE,      proto->LoadIconEx("arrow_up"),        LPGEN("Move rule up")),
	m_btnDownRule(this,   IDC_DOWN_RULE,    proto->LoadIconEx("arrow_down"),      LPGEN("Move rule down")),
	m_btnAddList(this,    IDC_ADD_LIST,     SKINICON_OTHER_ADDCONTACT,            LPGEN("Add list...")),
	m_btnRemoveList(this, IDC_REMOVE_LIST,  SKINICON_OTHER_DELETE,                LPGEN("Remove list")),
	m_btnApply(this,      IDC_APPLY,        SKINICON_EVENT_FILE,                  LPGEN("Save changes")),
	m_lbLists(this,       IDC_LB_LISTS),
	m_lbRules(this,       IDC_PL_RULES_LIST),
	m_clcClist(this,      IDC_CLIST)
{
	m_btnSimple.OnClick = Callback( this,     &CJabberDlgPrivacyLists::btnSimple_OnClick);
	m_btnAdvanced.OnClick = Callback( this,	&CJabberDlgPrivacyLists::btnAdvanced_OnClick);
	m_btnAddJid.OnClick = Callback( this,		&CJabberDlgPrivacyLists::btnAddJid_OnClick);
	m_btnActivate.OnClick = Callback( this,	&CJabberDlgPrivacyLists::btnActivate_OnClick);
	m_btnSetDefault.OnClick = Callback( this, &CJabberDlgPrivacyLists::btnSetDefault_OnClick);
	m_btnEditRule.OnClick = Callback( this,	&CJabberDlgPrivacyLists::btnEditRule_OnClick);
	m_btnAddRule.OnClick = Callback( this,		&CJabberDlgPrivacyLists::btnAddRule_OnClick);
	m_btnRemoveRule.OnClick = Callback( this, &CJabberDlgPrivacyLists::btnRemoveRule_OnClick);
	m_btnUpRule.OnClick = Callback( this,		&CJabberDlgPrivacyLists::btnUpRule_OnClick);
	m_btnDownRule.OnClick = Callback( this,	&CJabberDlgPrivacyLists::btnDownRule_OnClick);
	m_btnAddList.OnClick = Callback( this,		&CJabberDlgPrivacyLists::btnAddList_OnClick);
	m_btnRemoveList.OnClick = Callback( this, &CJabberDlgPrivacyLists::btnRemoveList_OnClick);
	m_btnApply.OnClick = Callback( this,		&CJabberDlgPrivacyLists::btnApply_OnClick);

	m_lbLists.OnSelChange = Callback(this, &CJabberDlgPrivacyLists::lbLists_OnSelChange);
	m_lbLists.OnDblClick = Callback(this, &CJabberDlgPrivacyLists::lbLists_OnDblClick);
	m_lbRules.OnSelChange = Callback(this, &CJabberDlgPrivacyLists::lbRules_OnSelChange);
	m_lbRules.OnDblClick = Callback(this, &CJabberDlgPrivacyLists::lbRules_OnDblClick);

	m_clcClist.OnNewContact =
	m_clcClist.OnListRebuilt = Callback(this, &CJabberDlgPrivacyLists::clcClist_OnUpdate);
	m_clcClist.OnOptionsChanged = Callback(this, &CJabberDlgPrivacyLists::clcClist_OnOptionsChanged);
	m_clcClist.OnClick = Callback(this, &CJabberDlgPrivacyLists::clcClist_OnClick);
}

void CJabberDlgPrivacyLists::OnInitDialog()
{
	CSuper::OnInitDialog();

	SendMessage( m_hwnd, WM_SETICON, ICON_BIG, ( LPARAM )m_proto->LoadIconEx( "privacylists" ));

	EnableWindow( GetDlgItem( m_hwnd, IDC_ADD_RULE ), FALSE );
	EnableWindow( GetDlgItem( m_hwnd, IDC_EDIT_RULE ), FALSE );
	EnableWindow( GetDlgItem( m_hwnd, IDC_REMOVE_RULE ), FALSE );
	EnableWindow( GetDlgItem( m_hwnd, IDC_UP_RULE ), FALSE );
	EnableWindow( GetDlgItem( m_hwnd, IDC_DOWN_RULE ), FALSE );

	m_proto->QueryPrivacyLists();

	LOGFONT lf;
	GetObject((HFONT)SendDlgItemMessage(m_hwnd, IDC_TXT_LISTS, WM_GETFONT, 0, 0), sizeof(lf), &lf);
	lf.lfWeight = FW_BOLD;
	HFONT hfnt = CreateFontIndirect(&lf);
	SendDlgItemMessage(m_hwnd, IDC_TXT_LISTS, WM_SETFONT, (WPARAM)hfnt, TRUE);
	SendDlgItemMessage(m_hwnd, IDC_TXT_RULES, WM_SETFONT, (WPARAM)hfnt, TRUE);
	SendDlgItemMessage(m_hwnd, IDC_TITLE, WM_SETFONT, (WPARAM)hfnt, TRUE);

	SetWindowLong(GetDlgItem(m_hwnd, IDC_CLIST), GWL_STYLE,
		GetWindowLong(GetDlgItem(m_hwnd, IDC_CLIST), GWL_STYLE)|CLS_HIDEEMPTYGROUPS|CLS_USEGROUPS|CLS_GREYALTERNATE);
	m_clcClist.SetExStyle(CLS_EX_DISABLEDRAGDROP|CLS_EX_TRACKSELECT);

	HIMAGELIST hIml = ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),(IsWinVerXPPlus()?ILC_COLOR32:ILC_COLOR16)|ILC_MASK,9,9);
	ImageList_AddIcon(hIml, LoadSkinnedIcon(SKINICON_OTHER_SMALLDOT));
	ImageList_AddIcon(hIml, m_proto->LoadIconEx("pl_msg_allow"));
	ImageList_AddIcon(hIml, m_proto->LoadIconEx("pl_msg_deny"));
	ImageList_AddIcon(hIml, m_proto->LoadIconEx("pl_prin_allow"));
	ImageList_AddIcon(hIml, m_proto->LoadIconEx("pl_prin_deny"));
	ImageList_AddIcon(hIml, m_proto->LoadIconEx("pl_prout_allow"));
	ImageList_AddIcon(hIml, m_proto->LoadIconEx("pl_prout_deny"));
	ImageList_AddIcon(hIml, m_proto->LoadIconEx("pl_iq_allow"));
	ImageList_AddIcon(hIml, m_proto->LoadIconEx("pl_iq_deny"));
	m_clcClist.SetExtraImageList(hIml);
	m_clcClist.SetExtraColumns(4);

	m_btnSimple.MakePush();
	m_btnAdvanced.MakePush();

	CLCINFOITEM cii = {0};
	cii.cbSize = sizeof(cii);

	cii.flags = CLCIIF_GROUPFONT;
	cii.pszText = TranslateT("** Default **");
	clc_info.hItemDefault = m_clcClist.AddInfoItem(&cii);
	cii.pszText = TranslateT("** Subsription: both **");
	clc_info.hItemSubBoth = m_clcClist.AddInfoItem(&cii);
	cii.pszText = TranslateT("** Subsription: to **");
	clc_info.hItemSubTo = m_clcClist.AddInfoItem(&cii);
	cii.pszText = TranslateT("** Subsription: from **");
	clc_info.hItemSubFrom = m_clcClist.AddInfoItem(&cii);
	cii.pszText = TranslateT("** Subsription: none **");
	clc_info.hItemSubNone = m_clcClist.AddInfoItem(&cii);

	CListResetOptions(GetDlgItem(m_hwnd, IDC_CLIST));
	CListFilter(GetDlgItem(m_hwnd, IDC_CLIST));
	CListApplyList(GetDlgItem(m_hwnd, IDC_CLIST));

	if ( DBGetContactSettingByte(NULL, m_proto->m_szModuleName, "plistsWnd_simpleMode", 1))
	{
		UIShowControls(m_hwnd, idSimpleControls, SW_SHOW);
		UIShowControls(m_hwnd, idAdvancedControls, SW_HIDE);
		CheckDlgButton(m_hwnd, IDC_BTN_SIMPLE, TRUE);
	} else
	{
		UIShowControls(m_hwnd, idSimpleControls, SW_HIDE);
		UIShowControls(m_hwnd, idAdvancedControls, SW_SHOW);
		CheckDlgButton(m_hwnd, IDC_BTN_ADVANCED, TRUE);
	}

	SetWindowLong(GetDlgItem(m_hwnd, IDC_LB_LISTS), GWL_USERDATA,
		SetWindowLong(GetDlgItem(m_hwnd, IDC_LB_LISTS), GWL_WNDPROC, (LONG)LstListsSubclassProc));
	SetWindowLong(GetDlgItem(m_hwnd, IDC_PL_RULES_LIST), GWL_USERDATA,
		SetWindowLong(GetDlgItem(m_hwnd, IDC_PL_RULES_LIST), GWL_WNDPROC, (LONG)LstRulesSubclassProc));

	SetStatusText(TranslateT("Loading..."));

	Utils_RestoreWindowPosition(m_hwnd, NULL, m_proto->m_szModuleName, "plistsWnd_sz");
}

void CJabberDlgPrivacyLists::OnClose()
{
	if (CanExit()) {
		DestroyWindow(m_hwnd);
		CSuper::OnClose();
	}
	else
		m_lresult = TRUE;
}

void CJabberDlgPrivacyLists::OnDestroy()
{
	m_proto->m_pDlgPrivacyLists = NULL;

	// Wipe all data and query lists without contents
	m_proto->m_privacyListManager.RemoveAllLists();
	m_proto->QueryPrivacyLists();
	m_proto->m_privacyListManager.SetModified( FALSE );

	// Delete custom bold font
	DeleteObject((HFONT)SendDlgItemMessage(m_hwnd, IDC_TXT_LISTS, WM_GETFONT, 0, 0));

	DBWriteContactSettingByte(NULL, m_proto->m_szModuleName, "plistsWnd_simpleMode", IsDlgButtonChecked(m_hwnd, IDC_BTN_SIMPLE));

	Utils_SaveWindowPosition(m_hwnd, NULL, m_proto->m_szModuleName, "plistsWnd_sz");

	CSuper::OnDestroy();
}

void CJabberDlgPrivacyLists::OnProtoRefresh(WPARAM, LPARAM)
{
	int sel = SendDlgItemMessage(m_hwnd, IDC_LB_LISTS, LB_GETCURSEL, 0, 0);
	int len = SendDlgItemMessage(m_hwnd, IDC_LB_LISTS, LB_GETTEXTLEN, sel, 0) + 1;
	TCHAR *szCurrentSelectedList = (TCHAR *)_alloca(len * sizeof(TCHAR));
	SendDlgItemMessage(m_hwnd, IDC_LB_LISTS, LB_GETTEXT, sel, (LPARAM)szCurrentSelectedList);

	SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_RESETCONTENT, 0, 0 );

	int nItemId = SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_ADDSTRING, 0, (LPARAM)TranslateT( "<none>" ));
	SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_SETITEMDATA, nItemId, (LPARAM)NULL );

	m_proto->m_privacyListManager.Lock();
	CPrivacyList* pList = m_proto->m_privacyListManager.GetFirstList();
	while ( pList ) {
		if ( !pList->IsDeleted() ) {
			nItemId = SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_ADDSTRING, 0, (LPARAM)pList->GetListName() );
			SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_SETITEMDATA, nItemId, (LPARAM)pList );
		}
		pList = pList->GetNext();
	}

	if ( SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_SELECTSTRING, -1, (LPARAM)szCurrentSelectedList ) == LB_ERR )
		SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_SETCURSEL, 0, 0 );

	m_proto->m_privacyListManager.Unlock();

	PostMessage( m_hwnd, WM_COMMAND, MAKEWPARAM( IDC_LB_LISTS, LBN_SELCHANGE ), 0 );
	EnableEditorControls();
}

BOOL CJabberDlgPrivacyLists::OnWmMeasureItem(UINT msg, WPARAM wParam, LPARAM lParam)
{
	LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
	if ((lpmis->CtlID != IDC_PL_RULES_LIST) && (lpmis->CtlID != IDC_LB_LISTS))
		return FALSE;

	TEXTMETRIC tm = {0};
	HDC hdc = GetDC(GetDlgItem(m_hwnd, lpmis->CtlID));
	GetTextMetrics(hdc, &tm);
	ReleaseDC(GetDlgItem(m_hwnd, lpmis->CtlID), hdc);

	if (lpmis->CtlID == IDC_PL_RULES_LIST)
		lpmis->itemHeight = tm.tmHeight * 2;
	else if (lpmis->CtlID == IDC_LB_LISTS)
		lpmis->itemHeight = tm.tmHeight;

	if (lpmis->itemHeight < 18) lpmis->itemHeight = 18;

	return TRUE;
}

BOOL CJabberDlgPrivacyLists::OnWmDrawItem(UINT msg, WPARAM wParam, LPARAM lParam)
{
	LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

	if (lpdis->CtlID == IDC_PL_RULES_LIST)
		DrawRulesList(lpdis);
	else if (lpdis->CtlID == IDC_LB_LISTS)
		DrawLists(lpdis);
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
		rc.left = (rc.left + rc.right - totalWidth)/2;

		for (i = 0; i < SIZEOF(items); ++i)
		{
			DrawIconEx(lpdis->hDC, rc.left, (rc.top+rc.bottom-16)/2, m_proto->LoadIconEx(items[i].icon),
				16, 16, 0, NULL, DI_NORMAL);
			rc.left += 18;
			DrawNextRulePart(lpdis->hDC, clText, items[i].text, &rc);
			rc.left += 5;
		}
	}
	else return FALSE;

	return TRUE;
}

BOOL CJabberDlgPrivacyLists::OnWmGetMinMaxInfo(UINT msg, WPARAM wParam, LPARAM lParam)
{
	LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
	lpmmi->ptMinTrackSize.x = 550;
	lpmmi->ptMinTrackSize.y = 390;
	return 0;
}

void CJabberDlgPrivacyLists::ShowAdvancedList(CPrivacyList *pList)
{
	int nLbSel = SendDlgItemMessage(m_hwnd, IDC_PL_RULES_LIST, LB_GETCURSEL, 0, 0);
	SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_RESETCONTENT, 0, 0 );

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

		int nItemId = SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_ADDSTRING, 0, (LPARAM)szListItem );
		SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_SETITEMDATA, nItemId, (LPARAM)pRule );

		pRule = pRule->GetNext();
	}

	EnableWindow( GetDlgItem( m_hwnd, IDC_PL_RULES_LIST ), !bListEmpty );
	if ( bListEmpty )
		SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_ADDSTRING, 0, (LPARAM)TranslateTS(_T("List has no rules, empty lists will be deleted then changes applied")));
	else
		SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_SETCURSEL, nLbSel, 0 );

	PostMessage( m_hwnd, WM_COMMAND, MAKEWPARAM( IDC_PL_RULES_LIST, LBN_SELCHANGE ), 0 );
}

void CJabberDlgPrivacyLists::DrawNextRulePart(HDC hdc, COLORREF color, const TCHAR *text, RECT *rc)
{
	SetTextColor(hdc, color);
	DrawText(hdc, text, lstrlen(text), rc, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE|DT_VCENTER|DT_WORD_ELLIPSIS);

	SIZE sz;
	GetTextExtentPoint32(hdc, text, lstrlen(text), &sz);
	rc->left += sz.cx;
}

void CJabberDlgPrivacyLists::DrawRuleAction(HDC hdc, COLORREF clLine1, COLORREF clLine2, CPrivacyListRule *pRule, RECT *rc)
{
	DrawNextRulePart(hdc, clLine1, pRule->GetAction() ? TranslateT("allow ") : TranslateT("deny "), rc);
	if (!pRule->GetPackets() || (pRule->GetPackets() == JABBER_PL_RULE_TYPE_ALL))
	{
		DrawNextRulePart(hdc, clLine1, TranslateT("all."), rc);
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
			DrawNextRulePart(hdc, clLine1, TranslateT("messages"), rc);
		}
		if (pRule->GetPackets() & JABBER_PL_RULE_TYPE_PRESENCE_IN)
		{
			--itemCount;
			if (needComma)
				DrawNextRulePart(hdc, clLine1, itemCount ? _T(", ") : TranslateT(" and "), rc);
			needComma = true;
			DrawNextRulePart(hdc, clLine1, TranslateT("incoming presences"), rc);
		}
		if (pRule->GetPackets() & JABBER_PL_RULE_TYPE_PRESENCE_OUT)
		{
			--itemCount;
			if (needComma)
				DrawNextRulePart(hdc, clLine1, itemCount ? _T(", ") : TranslateT(" and "), rc);
			needComma = true;
			DrawNextRulePart(hdc, clLine1, TranslateT("outgoing presences"), rc);
		}
		if (pRule->GetPackets() & JABBER_PL_RULE_TYPE_IQ)
		{
			--itemCount;
			if (needComma)
				DrawNextRulePart(hdc, clLine1, itemCount ? _T(", ") : TranslateT(" and "), rc);
			needComma = true;
			DrawNextRulePart(hdc, clLine1, TranslateT("queries"), rc);
		}
		DrawNextRulePart(hdc, clLine1, _T("."), rc);
	}
}

void CJabberDlgPrivacyLists::DrawRulesList(LPDRAWITEMSTRUCT lpdis)
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

	if ( !pRule ) {
		rc = lpdis->rcItem;
		rc.left += 25;

		int len = SendDlgItemMessage(m_hwnd, lpdis->CtlID, LB_GETTEXTLEN, lpdis->itemID, 0) + 1;
		TCHAR *str = (TCHAR *)_alloca(len * sizeof(TCHAR));
		SendDlgItemMessage(m_hwnd, lpdis->CtlID, LB_GETTEXT, lpdis->itemID, (LPARAM)str);
		DrawNextRulePart(lpdis->hDC, clLine1, str, &rc);
	}
	else if (pRule->GetType() == Else) {
		rc = lpdis->rcItem;
		rc.left += 25;

		DrawNextRulePart(lpdis->hDC, clLine2, TranslateT("Else "), &rc);
		DrawRuleAction(lpdis->hDC, clLine1, clLine2, pRule, &rc);
	}
	else {
		rc = lpdis->rcItem;
		rc.bottom -= (rc.bottom - rc.top) / 2;
		rc.left += 25;

		switch ( pRule->GetType() )
		{
			case Jid:
			{
				DrawNextRulePart(lpdis->hDC, clLine2, TranslateT("If jabber id is '"), &rc);
				DrawNextRulePart(lpdis->hDC, clLine1, pRule->GetValue(), &rc);
				DrawNextRulePart(lpdis->hDC, clLine2, TranslateT("'"), &rc);

				if (HANDLE hContact = m_proto->HContactFromJID(pRule->GetValue())) {
					TCHAR *szName = (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR);
					if ( szName ) {
						DrawNextRulePart(lpdis->hDC, clLine2, TranslateT(" (nickname: "), &rc);
						DrawNextRulePart(lpdis->hDC, clLine1, szName, &rc);
						DrawNextRulePart(lpdis->hDC, clLine2, TranslateT(")"), &rc);
				}	}
				break;
			}

			case Group:
				DrawNextRulePart(lpdis->hDC, clLine2, TranslateT("If group is '"), &rc);
				DrawNextRulePart(lpdis->hDC, clLine1, pRule->GetValue(), &rc);
				DrawNextRulePart(lpdis->hDC, clLine2, TranslateT("'"), &rc);
				break;
			case Subscription:
				DrawNextRulePart(lpdis->hDC, clLine2, TranslateT("If subscription is '"), &rc);
				DrawNextRulePart(lpdis->hDC, clLine1, pRule->GetValue(), &rc);
				DrawNextRulePart(lpdis->hDC, clLine2, TranslateT("'"), &rc);
				break;
		}

		rc = lpdis->rcItem;
		rc.top += (rc.bottom - rc.top) / 2;
		rc.left += 25;

		DrawNextRulePart(lpdis->hDC, clLine2, TranslateT("then "), &rc);
		DrawRuleAction(lpdis->hDC, clLine1, clLine2, pRule, &rc);
	}

	DrawIconEx(lpdis->hDC, lpdis->rcItem.left+4, (lpdis->rcItem.top+lpdis->rcItem.bottom-16)/2,
		m_proto->LoadIconEx("main"), 16, 16, 0, NULL, DI_NORMAL);

	if (pRule)
	{
		DrawIconEx(lpdis->hDC, lpdis->rcItem.left+4, (lpdis->rcItem.top+lpdis->rcItem.bottom-16)/2,
			m_proto->LoadIconEx(pRule->GetAction() ? "disco_ok" : "disco_fail"),
			16, 16, 0, NULL, DI_NORMAL);
	}

	if (lpdis->itemState & ODS_FOCUS)
	{
		int sel = SendDlgItemMessage(m_hwnd, lpdis->CtlID, LB_GETCURSEL, 0, 0);
		if ((sel == LB_ERR) || (sel == (int)lpdis->itemID))
			DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
	}
}

void CJabberDlgPrivacyLists::DrawLists(LPDRAWITEMSTRUCT lpdis)
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
	m_proto->m_privacyListManager.Lock();
	TCHAR *szDefault = NEWTSTR_ALLOCA(m_proto->m_privacyListManager.GetDefaultListName());
	TCHAR *szActive = NEWTSTR_ALLOCA(m_proto->m_privacyListManager.GetActiveListName());
	m_proto->m_privacyListManager.Unlock();

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

	DrawNextRulePart(lpdis->hDC, clLine1, szName, &rc);

	if (bActive && bDefault)
		DrawNextRulePart(lpdis->hDC, clLine2, TranslateT(" (act., def.)"), &rc);
	else if (bActive)
		DrawNextRulePart(lpdis->hDC, clLine2, TranslateT(" (active)"), &rc);
	else if (bDefault)
		DrawNextRulePart(lpdis->hDC, clLine2, TranslateT(" (default)"), &rc);

	DrawIconEx(lpdis->hDC, lpdis->rcItem.right-16-4, (lpdis->rcItem.top+lpdis->rcItem.bottom-16)/2,
		m_proto->LoadIconEx(bActive ? "pl_list_active" : "pl_list_any"),
		16, 16, 0, NULL, DI_NORMAL);

	if (bDefault)
		DrawIconEx(lpdis->hDC, lpdis->rcItem.right-16-4, (lpdis->rcItem.top+lpdis->rcItem.bottom-16)/2,
			m_proto->LoadIconEx("disco_ok"),
			16, 16, 0, NULL, DI_NORMAL);

	if (hfnt)
		DeleteObject(SelectObject(lpdis->hDC, hfnt));

	if (lpdis->itemState & ODS_FOCUS)
	{
		int sel = SendDlgItemMessage(m_hwnd, lpdis->CtlID, LB_GETCURSEL, 0, 0);
		if ((sel == LB_ERR) || (sel == (int)lpdis->itemID))
			DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
	}
}

void CJabberDlgPrivacyLists::CListResetOptions(HWND hwndList)
{
	m_clcClist.SetBkBitmap(0, NULL);
	m_clcClist.SetBkColor(GetSysColor(COLOR_WINDOW));
	m_clcClist.SetGreyoutFlags(0);
	m_clcClist.SetLeftMargin(4);
	m_clcClist.SetIndent(10);
	m_clcClist.SetHideEmptyGroups(false);
	m_clcClist.SetHideOfflineRoot(false);
	for (int i = 0; i <= FONTID_MAX; i++)
		m_clcClist.SetTextColor(i, GetSysColor(COLOR_WINDOWTEXT));
}

void CJabberDlgPrivacyLists::CListFilter(HWND hwndList)
{
	for	(HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
			hContact;
			hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0))
	{
		char *proto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (!proto || lstrcmpA(proto, m_proto->m_szModuleName))
			if (HANDLE hItem = m_clcClist.FindContact(hContact))
				m_clcClist.DeleteItem(hItem);
	}
}

bool CJabberDlgPrivacyLists::CListIsGroup(HANDLE hGroup)
{
	char idstr[33];
	DBVARIANT dbv;

	bool result = true;

	_itoa((int)hGroup-1, idstr, 10);

	if (DBGetContactSettingTString(NULL, "CListGroups", idstr, &dbv))
		result = false;

	DBFreeVariant(&dbv);

	return result;
}

HANDLE CJabberDlgPrivacyLists::CListFindGroupByName(TCHAR *name)
{
	char idstr[33];
	DBVARIANT dbv;

	HANDLE hGroup = 0;

	for (int i= 0; !hGroup; ++i)
	{
		_itoa(i, idstr, 10);

		if (DBGetContactSettingTString(NULL, "CListGroups", idstr, &dbv))
			break;

		if (!_tcscmp(dbv.ptszVal + 1, name))
			hGroup = (HANDLE)(i+1);

		DBFreeVariant(&dbv);
	}

	return hGroup;
}

void CJabberDlgPrivacyLists::CListResetIcons(HWND hwndList, HANDLE hItem, bool hide)
{
	for (int i = 0; i < 4; ++i)
		m_clcClist.SetExtraImage(hItem, i, hide ? 0xFF : 0);
}

void CJabberDlgPrivacyLists::CListSetupIcons(HWND hwndList, HANDLE hItem, int iSlot, DWORD dwProcess, BOOL bAction)
{
	if (dwProcess && !m_clcClist.GetExtraImage(hItem, iSlot))
		m_clcClist.SetExtraImage(hItem, iSlot, iSlot*2 + (bAction?1:2));
}

HANDLE CJabberDlgPrivacyLists::CListAddContact(HWND hwndList, TCHAR *jid)
{
	HANDLE hItem = 0;

	HANDLE hContact = m_proto->HContactFromJID(jid);
	if ( !hContact ) {
		hItem = clc_info.findJid(jid);
		if (!hItem)
		{
			CLCINFOITEM cii = {0};
			cii.cbSize = sizeof(cii);
			cii.pszText = jid;
			hItem = m_clcClist.AddInfoItem(&cii);
			CListResetIcons(hwndList, hItem);
			clc_info.addJid(hItem, jid);
		}
	} else
	{
		hItem = m_clcClist.FindContact(hContact);
	}

	return hItem;
}

void CJabberDlgPrivacyLists::CListApplyList(HWND hwndList, CPrivacyList *pList)
{
	clc_info.pList = pList;

	bool bHideIcons = pList ? false : true;

	CListResetIcons(hwndList, clc_info.hItemDefault, bHideIcons);
	CListResetIcons(hwndList, clc_info.hItemSubBoth, bHideIcons);
	CListResetIcons(hwndList, clc_info.hItemSubFrom, bHideIcons);
	CListResetIcons(hwndList, clc_info.hItemSubNone, bHideIcons);
	CListResetIcons(hwndList, clc_info.hItemSubTo, bHideIcons);

	// group handles start with 1 (0 is "root")
	for (int iGroup = 1; CListIsGroup((HANDLE)iGroup); ++iGroup)
	{
		HANDLE hItem = m_clcClist.FindGroup((HANDLE)iGroup);
		if (!hItem) continue;
		CListResetIcons(hwndList, hItem, bHideIcons);
	}

	for (HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0); hContact;
			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0))
	{
		HANDLE hItem = m_clcClist.FindContact(hContact);
		if (!hItem) continue;
		CListResetIcons(hwndList, hItem, bHideIcons);
	}

	for (int iJid = 0; iJid < clc_info.newJids.getCount(); ++iJid)
		CListResetIcons(hwndList, clc_info.newJids[iJid]->hItem, bHideIcons);

	if (!pList)
		goto lbl_return;

	CPrivacyListRule *pRule;
	for (pRule = pList->GetFirstRule(); pRule; pRule = pRule->GetNext())
	{
		HANDLE hItem = 0;
		switch (pRule->GetType())
		{
			case Jid:
			{
				hItem = CListAddContact(hwndList, pRule->GetValue());
				break;
			}
			case Group:
			{
				HANDLE hGroup = CListFindGroupByName(pRule->GetValue());
				hItem = m_clcClist.FindGroup(hGroup);
				break;
			}
			case Subscription:
			{
				     if (!lstrcmp(pRule->GetValue(), _T("none")))	hItem = clc_info.hItemSubNone;
				else if (!lstrcmp(pRule->GetValue(), _T("from")))	hItem = clc_info.hItemSubFrom;
				else if (!lstrcmp(pRule->GetValue(), _T("to")))		hItem = clc_info.hItemSubTo;
				else if (!lstrcmp(pRule->GetValue(), _T("both")))	hItem = clc_info.hItemSubBoth;
				break;
			}
			case Else:
			{
				hItem = clc_info.hItemDefault;
				break;
			}
		}

		if (!hItem) continue;

		DWORD dwPackets = pRule->GetPackets();
		if (!dwPackets) dwPackets = JABBER_PL_RULE_TYPE_ALL;
		CListSetupIcons(hwndList, hItem, 0, dwPackets & JABBER_PL_RULE_TYPE_MESSAGE, pRule->GetAction());
		CListSetupIcons(hwndList, hItem, 1, dwPackets & JABBER_PL_RULE_TYPE_PRESENCE_IN, pRule->GetAction());
		CListSetupIcons(hwndList, hItem, 2, dwPackets & JABBER_PL_RULE_TYPE_PRESENCE_OUT, pRule->GetAction());
		CListSetupIcons(hwndList, hItem, 3, dwPackets & JABBER_PL_RULE_TYPE_IQ, pRule->GetAction());
	}

lbl_return:
	clc_info.bChanged = false;
}

DWORD CJabberDlgPrivacyLists::CListGetPackets(HWND hwndList, HANDLE hItem, bool bAction)
{
	DWORD result = 0;

	int iIcon = 0;

	iIcon = m_clcClist.GetExtraImage(hItem, 0);
	     if ( bAction && (iIcon == 1)) result |= JABBER_PL_RULE_TYPE_MESSAGE;
	else if (!bAction && (iIcon == 2)) result |= JABBER_PL_RULE_TYPE_MESSAGE;

	iIcon = m_clcClist.GetExtraImage(hItem, 1);
	     if ( bAction && (iIcon == 3)) result |= JABBER_PL_RULE_TYPE_PRESENCE_IN;
	else if (!bAction && (iIcon == 4)) result |= JABBER_PL_RULE_TYPE_PRESENCE_IN;

	iIcon = m_clcClist.GetExtraImage(hItem, 2);
	     if ( bAction && (iIcon == 5)) result |= JABBER_PL_RULE_TYPE_PRESENCE_OUT;
	else if (!bAction && (iIcon == 6)) result |= JABBER_PL_RULE_TYPE_PRESENCE_OUT;

	iIcon = m_clcClist.GetExtraImage(hItem, 3);
	     if ( bAction && (iIcon == 7)) result |= JABBER_PL_RULE_TYPE_IQ;
	else if (!bAction && (iIcon == 8)) result |= JABBER_PL_RULE_TYPE_IQ;

	return result;
}

void CJabberDlgPrivacyLists::CListBuildList(HWND hwndList, CPrivacyList *pList)
{
	if (!pList) return;

	if (!clc_info.bChanged) return;

	clc_info.bChanged = false;

	DWORD dwOrder = 0;
	DWORD dwPackets = 0;

	HANDLE hItem;
	TCHAR *szJid;

	pList->RemoveAllRules();

	for (int iJid = 0; iJid < clc_info.newJids.getCount(); ++iJid)
	{
		hItem = clc_info.newJids[iJid]->hItem;
		szJid = clc_info.newJids[iJid]->jid;

		if (dwPackets = CListGetPackets(hwndList, hItem, true))
			pList->AddRule(Jid, szJid, TRUE, dwOrder++, dwPackets);
		if (dwPackets = CListGetPackets(hwndList, hItem, false))
			pList->AddRule(Jid, szJid, FALSE, dwOrder++, dwPackets);
	}

	for (HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0); hContact;
			hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0))
	{
		hItem = m_clcClist.FindContact(hContact);

		DBVARIANT dbv;
		if ( m_proto->JGetStringT(hContact, "jid", &dbv)) {
			if ( m_proto->JGetStringT(hContact, "ChatRoomID", &dbv)) {
				JFreeVariant(&dbv);
				continue;
			}
		}

		szJid = dbv.ptszVal;

		if (dwPackets = CListGetPackets(hwndList, hItem, true))
			pList->AddRule(Jid, szJid, TRUE, dwOrder++, dwPackets);
		if (dwPackets = CListGetPackets(hwndList, hItem, false))
			pList->AddRule(Jid, szJid, FALSE, dwOrder++, dwPackets);

		JFreeVariant(&dbv);
	}

	// group handles start with 1 (0 is "root")
	for (int iGroup = 1; ; ++iGroup)
	{
		char idstr[33];
		_itoa(iGroup-1, idstr, 10);
		DBVARIANT dbv;
		if (DBGetContactSettingTString(NULL, "CListGroups", idstr, &dbv))
		{
			DBFreeVariant(&dbv);
			break;
		}

		hItem = m_clcClist.FindGroup((HANDLE)iGroup);
		szJid = dbv.ptszVal+1;

		if (dwPackets = CListGetPackets(hwndList, hItem, true))
			pList->AddRule(Group, szJid, TRUE, dwOrder++, dwPackets);
		if (dwPackets = CListGetPackets(hwndList, hItem, false))
			pList->AddRule(Group, szJid, FALSE, dwOrder++, dwPackets);

		DBFreeVariant(&dbv);
	}

	hItem = clc_info.hItemSubBoth;
	szJid = _T("both");
	if (dwPackets = CListGetPackets(hwndList, hItem, true))
		pList->AddRule(Subscription, szJid, TRUE, dwOrder++, dwPackets);
	if (dwPackets = CListGetPackets(hwndList, hItem, false))
		pList->AddRule(Subscription, szJid, FALSE, dwOrder++, dwPackets);

	hItem = clc_info.hItemSubFrom;
	szJid = _T("from");
	if (dwPackets = CListGetPackets(hwndList, hItem, true))
		pList->AddRule(Subscription, szJid, TRUE, dwOrder++, dwPackets);
	if (dwPackets = CListGetPackets(hwndList, hItem, false))
		pList->AddRule(Subscription, szJid, FALSE, dwOrder++, dwPackets);

	hItem = clc_info.hItemSubNone;
	szJid = _T("none");
	if (dwPackets = CListGetPackets(hwndList, hItem, true))
		pList->AddRule(Subscription, szJid, TRUE, dwOrder++, dwPackets);
	if (dwPackets = CListGetPackets(hwndList, hItem, false))
		pList->AddRule(Subscription, szJid, FALSE, dwOrder++, dwPackets);

	hItem = clc_info.hItemSubTo;
	szJid = _T("to");
	if (dwPackets = CListGetPackets(hwndList, hItem, true))
		pList->AddRule(Subscription, szJid, TRUE, dwOrder++, dwPackets);
	if (dwPackets = CListGetPackets(hwndList, hItem, false))
		pList->AddRule(Subscription, szJid, FALSE, dwOrder++, dwPackets);

	hItem = clc_info.hItemDefault;
	szJid = NULL;
	if (dwPackets = CListGetPackets(hwndList, hItem, true))
		pList->AddRule(Else, szJid, TRUE, dwOrder++, dwPackets);
	if (dwPackets = CListGetPackets(hwndList, hItem, false))
		pList->AddRule(Else, szJid, FALSE, dwOrder++, dwPackets);

	pList->Reorder();
	pList->SetModified();
}

void CJabberDlgPrivacyLists::EnableEditorControls()
{
	m_proto->m_privacyListManager.Lock();
	BOOL bListsLoaded = m_proto->m_privacyListManager.IsAllListsLoaded();
	BOOL bListsModified = m_proto->m_privacyListManager.IsModified() || clc_info.bChanged;
	m_proto->m_privacyListManager.Unlock();

	int nCurSel = SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_GETCURSEL, 0, 0 );
	int nItemCount = SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_GETCOUNT, 0, 0 );
	BOOL bSelected = nCurSel != CB_ERR;
	BOOL bListSelected = SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_GETCOUNT, 0, 0);
	bListSelected = bListSelected && (SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_GETCURSEL, 0, 0) != LB_ERR);
	bListSelected = bListSelected && SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_GETITEMDATA, SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_GETCURSEL, 0, 0), 0);

	EnableWindow( GetDlgItem( m_hwnd, IDC_TXT_OTHERJID ), bListsLoaded && bListSelected );
	EnableWindow( GetDlgItem( m_hwnd, IDC_NEWJID ), bListsLoaded && bListSelected );
	EnableWindow( GetDlgItem( m_hwnd, IDC_ADDJID ), bListsLoaded && bListSelected );

	EnableWindow( GetDlgItem( m_hwnd, IDC_ADD_RULE ), bListsLoaded && bListSelected );
	EnableWindow( GetDlgItem( m_hwnd, IDC_EDIT_RULE ), bListsLoaded && bSelected );
	EnableWindow( GetDlgItem( m_hwnd, IDC_REMOVE_RULE ), bListsLoaded && bSelected );
	EnableWindow( GetDlgItem( m_hwnd, IDC_UP_RULE ), bListsLoaded && bSelected && nCurSel != 0 );
	EnableWindow( GetDlgItem( m_hwnd, IDC_DOWN_RULE ), bListsLoaded && bSelected && nCurSel != ( nItemCount - 1 ));
	EnableWindow( GetDlgItem( m_hwnd, IDC_REMOVE_LIST ), bListsLoaded && bListSelected );
	EnableWindow( GetDlgItem( m_hwnd, IDC_APPLY ), bListsLoaded && bListsModified );

	if (bListsLoaded)
		SetStatusText( TranslateT("Ready.") );
}

LRESULT CALLBACK CJabberDlgPrivacyLists::LstListsSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC sttOldWndProc = (WNDPROC)GetWindowLong(hwnd, GWL_USERDATA);
	switch (msg)
	{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			if (wParam == VK_INSERT)
				return UIEmulateBtnClick(GetParent(hwnd), IDC_ADD_LIST);
			if (wParam == VK_DELETE)
				return UIEmulateBtnClick(GetParent(hwnd), IDC_REMOVE_LIST);
			if (wParam == VK_SPACE)
			{
				if (GetAsyncKeyState(VK_CONTROL))
					return UIEmulateBtnClick(GetParent(hwnd), IDC_SET_DEFAULT);
				return UIEmulateBtnClick(GetParent(hwnd), IDC_ACTIVATE);
			}

			break;
		}
	}
	return CallWindowProc(sttOldWndProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK CJabberDlgPrivacyLists::LstRulesSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC sttOldWndProc = (WNDPROC)GetWindowLong(hwnd, GWL_USERDATA);
	switch (msg)
	{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			if (wParam == VK_INSERT)
				return UIEmulateBtnClick(GetParent(hwnd), IDC_ADD_RULE);
			if (wParam == VK_DELETE)
				return UIEmulateBtnClick(GetParent(hwnd), IDC_REMOVE_RULE);
			if ((wParam == VK_UP) && (lParam & (1UL << 29)))
				return UIEmulateBtnClick(GetParent(hwnd), IDC_UP_RULE);
			if ((wParam == VK_DOWN) && (lParam & (1UL << 29)))
				return UIEmulateBtnClick(GetParent(hwnd), IDC_DOWN_RULE);
			if (wParam == VK_F2)
				return UIEmulateBtnClick(GetParent(hwnd), IDC_EDIT_RULE);

			break;
		}
	}
	return CallWindowProc(sttOldWndProc, hwnd, msg, wParam, lParam);
}

BOOL CJabberDlgPrivacyLists::CanExit()
{
	m_proto->m_privacyListManager.Lock();
	BOOL bModified = m_proto->m_privacyListManager.IsModified();
	m_proto->m_privacyListManager.Unlock();

	if (clc_info.bChanged)
		bModified = TRUE;

	if ( !bModified )
		return TRUE;

	if ( IDYES == MessageBox( m_hwnd, TranslateT("Privacy lists are not saved, discard any changes and exit?"), TranslateT("Are you sure?"), MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2 ))
		return TRUE;

	return FALSE;
}

void CJabberDlgPrivacyLists::btnSimple_OnClick(CCtrlButton *)
{
	CheckDlgButton(m_hwnd, IDC_BTN_SIMPLE, TRUE);
	CheckDlgButton(m_hwnd, IDC_BTN_ADVANCED, FALSE);
	UIShowControls(m_hwnd, idSimpleControls, SW_SHOW);
	UIShowControls(m_hwnd, idAdvancedControls, SW_HIDE);
	CListApplyList(GetDlgItem(m_hwnd, IDC_CLIST), GetSelectedList(m_hwnd));
}

void CJabberDlgPrivacyLists::btnAdvanced_OnClick(CCtrlButton *)
{
	CheckDlgButton(m_hwnd, IDC_BTN_SIMPLE, FALSE);
	CheckDlgButton(m_hwnd, IDC_BTN_ADVANCED, TRUE);
	UIShowControls(m_hwnd, idSimpleControls, SW_HIDE);
	UIShowControls(m_hwnd, idAdvancedControls, SW_SHOW);
	CListBuildList(GetDlgItem(m_hwnd, IDC_CLIST), GetSelectedList(m_hwnd));
	PostMessage(m_hwnd, WM_COMMAND, MAKEWPARAM(IDC_LB_LISTS, LBN_SELCHANGE), 0);
}

void CJabberDlgPrivacyLists::btnAddJid_OnClick(CCtrlButton *)
{
	int len = GetWindowTextLength(GetDlgItem(m_hwnd, IDC_NEWJID))+1;
	TCHAR *buf = (TCHAR *)_alloca(sizeof(TCHAR) * len);
	GetWindowText(GetDlgItem(m_hwnd, IDC_NEWJID), buf, len);
	SetWindowText(GetDlgItem(m_hwnd, IDC_NEWJID), _T(""));
	CListAddContact(GetDlgItem(m_hwnd, IDC_CLIST), buf);
}

void CJabberDlgPrivacyLists::btnActivate_OnClick(CCtrlButton *)
{
	if ( m_proto->m_bJabberOnline )
	{
		m_proto->m_privacyListManager.Lock();
		EnableWindow( GetDlgItem( m_hwnd, IDC_ACTIVATE ), FALSE );
		CPrivacyList* pList = GetSelectedList(m_hwnd);
		SetWindowLong( GetDlgItem( m_hwnd, IDC_ACTIVATE ), GWL_USERDATA, (DWORD)pList );
		XmlNodeIq iq( m_proto->m_iqManager.AddHandler( &CJabberProto::OnIqResultPrivacyListActive, JABBER_IQ_TYPE_SET, NULL, 0, -1, pList ) );
		XmlNode* query = iq.addQuery( JABBER_FEAT_PRIVACY_LISTS );
		XmlNode* active = query->addChild( "active" );
		if ( pList )
			active->addAttr( "name", pList->GetListName() );
		m_proto->m_privacyListManager.Unlock();

		SetStatusText(TranslateT( JABBER_PL_BUSY_MSG ));

		m_proto->m_ThreadInfo->send( iq );
	}
}

void CJabberDlgPrivacyLists::btnSetDefault_OnClick(CCtrlButton *)
{
	if ( m_proto->m_bJabberOnline )
	{
		m_proto->m_privacyListManager.Lock();
		EnableWindow( GetDlgItem( m_hwnd, IDC_SET_DEFAULT ), FALSE );
		CPrivacyList* pList = GetSelectedList(m_hwnd);
		SetWindowLong( GetDlgItem( m_hwnd, IDC_SET_DEFAULT ), GWL_USERDATA, (DWORD)pList );
		int iqId = m_proto->SerialNext();
		XmlNodeIq iq( m_proto->m_iqManager.AddHandler( &CJabberProto::OnIqResultPrivacyListDefault, JABBER_IQ_TYPE_SET, NULL, 0, -1, pList ) );
		XmlNode* query = iq.addQuery( JABBER_FEAT_PRIVACY_LISTS );
		XmlNode* defaultTag = query->addChild( "default" );
		if ( pList )
			defaultTag->addAttr( "name", pList->GetListName() );
		m_proto->m_privacyListManager.Unlock();

		SetStatusText(TranslateT( JABBER_PL_BUSY_MSG ));

		m_proto->m_ThreadInfo->send( iq );
	}
}

void CJabberDlgPrivacyLists::lbLists_OnSelChange(CCtrlListBox *)
{
	int nCurSel = SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_GETCURSEL, 0, 0 );
	if ( nCurSel == LB_ERR )
		return;

	int nErr = SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_GETITEMDATA, nCurSel, 0 );
	if ( nErr == LB_ERR )
		return;
	if ( nErr == 0 )
	{
		if (IsWindowVisible(GetDlgItem(m_hwnd, IDC_CLIST)))
		{
			CListBuildList(GetDlgItem(m_hwnd, IDC_CLIST), clc_info.pList);
			CListApplyList(GetDlgItem(m_hwnd, IDC_CLIST), NULL);
		} else
		{
			EnableWindow( GetDlgItem( m_hwnd, IDC_PL_RULES_LIST ), FALSE );
			SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_RESETCONTENT, 0, 0 );
			SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_ADDSTRING, 0, (LPARAM)TranslateTS(_T("No list selected")));
		}
		EnableEditorControls();
		return;
	}

	m_proto->m_privacyListManager.Lock();
	if (IsWindowVisible(GetDlgItem(m_hwnd, IDC_CLIST)))
	{
		CListBuildList(GetDlgItem(m_hwnd, IDC_CLIST), clc_info.pList);
		CListApplyList(GetDlgItem(m_hwnd, IDC_CLIST), (CPrivacyList* )nErr);
	}
	else ShowAdvancedList((CPrivacyList* )nErr);

	m_proto->m_privacyListManager.Unlock();

	EnableEditorControls();
}

void CJabberDlgPrivacyLists::lbLists_OnDblClick(CCtrlListBox *)
{
	PostMessage( m_hwnd, WM_COMMAND, MAKEWPARAM( IDC_ACTIVATE, 0 ), 0 );
}

void CJabberDlgPrivacyLists::lbRules_OnSelChange(CCtrlListBox *)
{
	EnableEditorControls();
}

void CJabberDlgPrivacyLists::lbRules_OnDblClick(CCtrlListBox *)
{
	PostMessage( m_hwnd, WM_COMMAND, MAKEWPARAM( IDC_EDIT_RULE, 0 ), 0 );
}

void CJabberDlgPrivacyLists::btnEditRule_OnClick(CCtrlButton *)
{
	// FIXME: potential deadlock due to PLM lock while editing rule
	m_proto->m_privacyListManager.Lock();
	{
		CPrivacyListRule* pRule = GetSelectedRule( m_hwnd );
		CPrivacyList* pList = GetSelectedList(m_hwnd);
		if ( pList && pRule ) {
			CJabberDlgPrivacyRule dlgPrivacyRule(m_proto, m_hwnd, pRule);
			int nResult = dlgPrivacyRule.DoModal();
			if ( nResult ) {
				pList->SetModified();
				PostMessage( m_hwnd, WM_JABBER_REFRESH, 0, 0 );
	}	}	}
	m_proto->m_privacyListManager.Unlock();
}

void CJabberDlgPrivacyLists::btnAddRule_OnClick(CCtrlButton *)
{
	// FIXME: potential deadlock due to PLM lock while editing rule
	m_proto->m_privacyListManager.Lock();
	{
		CPrivacyList* pList = GetSelectedList(m_hwnd);
		if ( pList ) {
			CPrivacyListRule* pRule = new CPrivacyListRule( m_proto, Jid, _T(""), FALSE );
			CJabberDlgPrivacyRule dlgPrivacyRule(m_proto, m_hwnd, pRule);
			int nResult = dlgPrivacyRule.DoModal();
			if ( nResult ) {
				pList->AddRule(pRule);
				pList->Reorder();
				pList->SetModified();
				PostMessage( m_hwnd, WM_JABBER_REFRESH, 0, 0 );
			}
			else delete pRule;
	}	}
	m_proto->m_privacyListManager.Unlock();
}

void CJabberDlgPrivacyLists::btnRemoveRule_OnClick(CCtrlButton *)
{
	m_proto->m_privacyListManager.Lock();
	{
		CPrivacyList* pList = GetSelectedList(m_hwnd);
		CPrivacyListRule* pRule = GetSelectedRule( m_hwnd );

		if ( pList && pRule ) {
			pList->RemoveRule( pRule );
			int nCurSel = SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_GETCURSEL, 0, 0 );
			int nItemCount = SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_GETCOUNT, 0, 0 );
			SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_SETCURSEL, nCurSel != nItemCount - 1 ? nCurSel : nCurSel - 1, 0 );
			pList->Reorder();
			pList->SetModified();
			PostMessage( m_hwnd, WM_JABBER_REFRESH, 0, 0 );
	}	}

	m_proto->m_privacyListManager.Unlock();
}

void CJabberDlgPrivacyLists::btnUpRule_OnClick(CCtrlButton *)
{
	m_proto->m_privacyListManager.Lock();
	{
		CPrivacyList* pList = GetSelectedList(m_hwnd);
		CPrivacyListRule* pRule = GetSelectedRule( m_hwnd );
		int nCurSel = SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_GETCURSEL, 0, 0 );

		if ( pList && pRule && nCurSel ) {
			pRule->SetOrder( pRule->GetOrder() - 11 );
			SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_SETCURSEL, nCurSel - 1, 0 );
			pList->Reorder();
			pList->SetModified();
			PostMessage( m_hwnd, WM_JABBER_REFRESH, 0, 0 );
	}	}

	m_proto->m_privacyListManager.Unlock();
}

void CJabberDlgPrivacyLists::btnDownRule_OnClick(CCtrlButton *)
{
	m_proto->m_privacyListManager.Lock();
	{
		CPrivacyList* pList = GetSelectedList(m_hwnd);
		CPrivacyListRule* pRule = GetSelectedRule( m_hwnd );
		int nCurSel = SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_GETCURSEL, 0, 0 );
		int nItemCount = SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_GETCOUNT, 0, 0 );

		if ( pList && pRule && ( nCurSel != ( nItemCount - 1 ))) {
			pRule->SetOrder( pRule->GetOrder() + 11 );
			SendDlgItemMessage( m_hwnd, IDC_PL_RULES_LIST, LB_SETCURSEL, nCurSel + 1, 0 );
			pList->Reorder();
			pList->SetModified();
			PostMessage( m_hwnd, WM_JABBER_REFRESH, 0, 0 );
	}	}

	m_proto->m_privacyListManager.Unlock();
}

void CJabberDlgPrivacyLists::btnAddList_OnClick(CCtrlButton *)
{
	// FIXME: line length is hard coded in dialog procedure
	CJabberDlgPrivacyAddList dlgPrivacyAddList(m_proto, m_hwnd);
	int nRetVal = dlgPrivacyAddList.DoModal();
	if ( nRetVal && _tcslen( dlgPrivacyAddList.szLine ) ) {
		m_proto->m_privacyListManager.Lock();
		CPrivacyList* pList = m_proto->m_privacyListManager.FindList( dlgPrivacyAddList.szLine );
		if ( !pList ) {
			m_proto->m_privacyListManager.AddList( dlgPrivacyAddList.szLine );
			pList = m_proto->m_privacyListManager.FindList( dlgPrivacyAddList.szLine );
			if ( pList ) {
				pList->SetModified( TRUE );
				pList->SetLoaded( TRUE );
			}
		}
		if ( pList )
			pList->SetDeleted( FALSE );
		int nSelected = SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_SELECTSTRING, -1, (LPARAM)dlgPrivacyAddList.szLine );
		if ( nSelected == CB_ERR ) {
			nSelected = SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_ADDSTRING, 0, (LPARAM)dlgPrivacyAddList.szLine );
			SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_SETITEMDATA, nSelected, (LPARAM)pList );
			SendDlgItemMessage( m_hwnd, IDC_LB_LISTS, LB_SETCURSEL, nSelected, 0 );
		}
		m_proto->m_privacyListManager.Unlock();
		PostMessage( m_hwnd, WM_JABBER_REFRESH, 0, 0 );
	}
}

void CJabberDlgPrivacyLists::btnRemoveList_OnClick(CCtrlButton *)
{
	m_proto->m_privacyListManager.Lock();
	{
		CPrivacyList* pList = GetSelectedList(m_hwnd);
		if ( pList ) {
			TCHAR *szListName = pList->GetListName();
			if ( ( m_proto->m_privacyListManager.GetActiveListName() && !_tcscmp( szListName, m_proto->m_privacyListManager.GetActiveListName()))
				|| ( m_proto->m_privacyListManager.GetDefaultListName() && !_tcscmp( szListName, m_proto->m_privacyListManager.GetDefaultListName()) )) {
				m_proto->m_privacyListManager.Unlock();
				MessageBox( m_hwnd, TranslateTS(_T("Can't remove active or default list")), TranslateTS(_T("Sorry")), MB_OK | MB_ICONSTOP );
				return;
			}
			pList->SetDeleted();
			pList->SetModified();
	}	}

	m_proto->m_privacyListManager.Unlock();
	PostMessage( m_hwnd, WM_JABBER_REFRESH, 0, 0 );
}

void CJabberDlgPrivacyLists::btnApply_OnClick(CCtrlButton *)
{
	if ( !m_proto->m_bJabberOnline ) {
		SetStatusText(TranslateT("Unable to save list because you are currently offline."));
		return;
	}

	m_proto->m_privacyListManager.Lock();
	{
		if (IsWindowVisible(GetDlgItem(m_hwnd, IDC_CLIST)))
			CListBuildList(GetDlgItem(m_hwnd, IDC_CLIST), clc_info.pList);

		DWORD dwGroupId = 0;
		CPrivacyListModifyUserParam *pUserData = NULL;
		CPrivacyList* pList = m_proto->m_privacyListManager.GetFirstList();
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
					dwGroupId = m_proto->m_iqManager.GetNextFreeGroupId();
					pUserData = new CPrivacyListModifyUserParam;
					pUserData->m_bAllOk = TRUE;
				}

				XmlNodeIq iq( m_proto->m_iqManager.AddHandler( &CJabberProto::OnIqResultPrivacyListModify, JABBER_IQ_TYPE_SET, NULL, 0, -1, pUserData, dwGroupId ));
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

				m_proto->m_ThreadInfo->send( iq );
			}
			pList = pList->GetNext();
	}	}
	m_proto->m_privacyListManager.Unlock();
	SetStatusText(TranslateT( JABBER_PL_BUSY_MSG ));
	PostMessage( m_hwnd, WM_JABBER_REFRESH, 0, 0 );
}

void CJabberDlgPrivacyLists::OnCommand_Close(HWND hwndCtrl, WORD idCtrl, WORD idCode)
{
	if (IsWindowVisible(GetDlgItem(m_hwnd, IDC_CLIST)))
		CListBuildList(GetDlgItem(m_hwnd, IDC_CLIST), clc_info.pList);

	if (CanExit())
		DestroyWindow(m_hwnd);
}

void CJabberDlgPrivacyLists::clcClist_OnUpdate(CCtrlClc::TEventInfo *evt)
{
	CListFilter(GetDlgItem(m_hwnd, IDC_CLIST));
	CListApplyList(GetDlgItem(m_hwnd, IDC_CLIST), GetSelectedList(m_hwnd));
}

void CJabberDlgPrivacyLists::clcClist_OnOptionsChanged(CCtrlClc::TEventInfo *evt)
{
	CListResetOptions(GetDlgItem(m_hwnd, IDC_CLIST));
	CListApplyList(GetDlgItem(m_hwnd, IDC_CLIST), GetSelectedList(m_hwnd));
}

void CJabberDlgPrivacyLists::clcClist_OnClick(CCtrlClc::TEventInfo *evt)
{
	HANDLE hItem;
	DWORD hitFlags;
	int iImage;

	if(evt->info->iColumn==-1) return;
	hItem = m_clcClist.HitTest(evt->info->pt.x, evt->info->pt.y, &hitFlags);
	if(hItem==NULL) return;
	if(!(hitFlags&CLCHT_ONITEMEXTRA)) return;

	iImage = m_clcClist.GetExtraImage(hItem, evt->info->iColumn);
	if (iImage != 0xFF)
	{
		if (iImage == 0)
			iImage = evt->info->iColumn * 2 + 2;
		else if (iImage == evt->info->iColumn * 2 + 2)
			iImage = evt->info->iColumn * 2 + 1;
		else
			iImage = 0;

		m_clcClist.SetExtraImage(hItem, evt->info->iColumn, iImage);

		clc_info.bChanged = true;

		EnableEditorControls();
	}
}

int CJabberDlgPrivacyLists::Resizer(UTILRESIZECONTROL *urc)
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
	case IDC_ADD_LIST:
	case IDC_ACTIVATE:
	case IDC_REMOVE_LIST:
	case IDC_SET_DEFAULT:
	case IDC_TXT_OTHERJID:
	case IDC_ADD_RULE:
	case IDC_UP_RULE:
	case IDC_EDIT_RULE:
	case IDC_DOWN_RULE:
	case IDC_REMOVE_RULE:
		return RD_ANCHORX_LEFT|RD_ANCHORY_BOTTOM;
	case IDC_ADDJID:
	case IDC_APPLY:
	case IDCANCEL:
		return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
	}
	return CSuper::Resizer(urc);
}

int __cdecl CJabberProto::OnMenuHandlePrivacyLists( WPARAM wParam, LPARAM lParam )
{
	UI_SAFE_OPEN(CJabberDlgPrivacyLists, m_pDlgPrivacyLists);
	return 0;
}

void CJabberProto::QueryPrivacyLists( ThreadData *pThreadInfo )
{
	XmlNodeIq iq( m_iqManager.AddHandler( &CJabberProto::OnIqResultPrivacyLists ) );
	XmlNode* query = iq.addQuery( JABBER_FEAT_PRIVACY_LISTS );
	if ( pThreadInfo )
		pThreadInfo->send( iq );
	else if ( m_ThreadInfo )
		m_ThreadInfo->send( iq );
}

/////////////////////////////////////////////////////////////////////////////////////////
// builds privacy menu

int __cdecl CJabberProto::menuSetPrivacyList( WPARAM wParam, LPARAM lParam, LPARAM iList )
{
	m_privacyListManager.Lock();
	CPrivacyList *pList = NULL;

	if (iList)
	{
		pList = m_privacyListManager.GetFirstList();
		for (int i = 1; pList && (i < iList); ++i)
			pList = pList->GetNext();
	}

	XmlNodeIq iq( m_iqManager.AddHandler( &CJabberProto::OnIqResultPrivacyListActive, JABBER_IQ_TYPE_SET, NULL, 0, -1, pList ) );
	XmlNode* query = iq.addQuery( JABBER_FEAT_PRIVACY_LISTS );
	XmlNode* active = query->addChild( "active" );
	if ( pList )
		active->addAttr( "name", pList->GetListName() );
	m_privacyListManager.Unlock();

	m_ThreadInfo->send( iq );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// init privacy menu

void CJabberProto::BuildPrivacyMenu( WPARAM wParam, LPARAM lParam )
{
	if ( !m_bJabberOnline )
		return;

	CLISTMENUITEM mi = { 0 };
	int i=0;
	char srvFce[MAX_PATH + 64], *svcName = srvFce+strlen( m_szModuleName );
	char szItem[MAX_PATH + 64];
	HANDLE hPrivacyRoot;
	HANDLE hRoot = ( HANDLE )szItem;

	mir_snprintf( szItem, SIZEOF(szItem), LPGEN("Privacy Lists") );

	mi.cbSize = sizeof(mi);
	mi.popupPosition= 500084000;
	mi.pszContactOwner = m_szModuleName;
	mi.pszService = srvFce;
	mi.pszPopupName = (char *)hRoot;

	mi.position = 3000040000;
	mi.flags = CMIF_TCHAR|CMIF_ICONFROMICOLIB;

	mi.icolibItem = GetIconHandle(IDI_PRIVACY_LISTS);
	mi.ptszName = LPGENT("List Editor...");
	mir_snprintf( srvFce, SIZEOF(srvFce), "%s/PrivacyLists", m_szModuleName );
	CallService( MS_CLIST_ADDSTATUSMENUITEM, ( WPARAM )&hPrivacyRoot, ( LPARAM )&mi );

	CLISTMENUITEM miTmp = {0};
	miTmp.cbSize = sizeof(miTmp);
	miTmp.flags = CMIM_ICON|CMIM_FLAGS | CMIF_ICONFROMICOLIB;
	miTmp.icolibItem = GetIconHandle(IDI_PRIVACY_LISTS);
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hPrivacyRoot, (LPARAM)&miTmp);

	mi.position = 2000040000;
	mi.flags = CMIF_ICONFROMICOLIB|CMIF_TCHAR;

	m_privacyListManager.Lock();

	mir_snprintf( srvFce, SIZEOF(srvFce), "%s/menuPrivacy%d", m_szModuleName, i );
	if ( i > m_privacyMenuServiceAllocated ) {
		JCreateServiceParam( svcName, &CJabberProto::menuSetPrivacyList, i );
		m_privacyMenuServiceAllocated = i;
	}
	mi.position++;
	mi.icolibItem = LoadSkinnedIconHandle(
		m_privacyListManager.GetActiveListName() ?
			SKINICON_OTHER_SMALLDOT :
			SKINICON_OTHER_EMPTYBLOB);
	mi.ptszName = LPGENT("<none>");
	CallService( MS_CLIST_ADDSTATUSMENUITEM, ( WPARAM )&hPrivacyRoot, ( LPARAM )&mi );

	for ( CPrivacyList *pList = m_privacyListManager.GetFirstList(); pList; pList = pList->GetNext()) {
		++i;
		mir_snprintf( srvFce, SIZEOF(srvFce), "%s/menuPrivacy%d", m_szModuleName, i );

		if ( i > m_privacyMenuServiceAllocated ) {
			JCreateServiceParam( svcName, &CJabberProto::menuSetPrivacyList, i );
			m_privacyMenuServiceAllocated = i;
		}

		mi.position++;
		mi.icolibItem = LoadSkinnedIconHandle(
			lstrcmp( m_privacyListManager.GetActiveListName(), pList->GetListName()) ?
				SKINICON_OTHER_SMALLDOT :
				SKINICON_OTHER_EMPTYBLOB);
		mi.ptszName = pList->GetListName();
		CallService( MS_CLIST_ADDSTATUSMENUITEM, ( WPARAM )&hPrivacyRoot, ( LPARAM )&mi );
	}

	m_privacyListManager.Unlock();
}
