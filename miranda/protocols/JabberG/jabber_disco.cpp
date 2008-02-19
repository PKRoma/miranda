/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
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

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "commctrl.h"
#include "jabber_iq.h"
#include "resource.h"
#include "jabber_disco.h"

CJabberSDManager g_SDManager;

#define SD_FAKEJID_CONFERENCES	"@@conferences"
#define SD_FAKEJID_MYAGENTS		"@@my-transports"
#define SD_FAKEJID_AGENTS		"@@transports"
#define SD_FAKEJID_FAVORITES	"@@favorites"

enum {
	SD_BROWSE_NORMAL,
	SD_BROWSE_MYAGENTS,
	SD_BROWSE_AGENTS,
	SD_BROWSE_CONFERENCES,
	SD_BROWSE_FAVORITES
};
static int sttBrowseMode = SD_BROWSE_NORMAL;

#define REFRESH_TIMEOUT		500
#define REFRESH_TIMER		1607
static DWORD sttLastRefresh = 0;

#define AUTODISCO_TIMEOUT	500
#define AUTODISCO_TIMER		1608
static DWORD sttLastAutoDisco = 0;

enum { SD_OVERLAY_NONE, SD_OVERLAY_FAIL, SD_OVERLAY_PROGRESS, SD_OVERLAY_REGISTERED };

static struct
{
	TCHAR *feature;
	TCHAR *category;
	TCHAR *type;
	char *iconName;
	int iconIndex;
	int listIndex;
} sttNodeIcons[] =
{
//	standard identities: http://www.xmpp.org/registrar/disco-categories.html#directory
//	{NULL,	_T("account"),			_T("admin"),		NULL,			0},
//	{NULL,	_T("account"),			_T("anonymous"),	NULL,			0},
//	{NULL,	_T("account"),			_T("registered"),	NULL,			0},
	{NULL,	_T("account"),			NULL,				NULL,			SKINICON_STATUS_ONLINE},

//	{NULL,	_T("auth"),				_T("cert"),			NULL,			0},
//	{NULL,	_T("auth"),				_T("generic"),		NULL,			0},
//	{NULL,	_T("auth"),				_T("ldap"),			NULL,			0},
//	{NULL,	_T("auth"),				_T("ntlm"),			NULL,			0},
//	{NULL,	_T("auth"),				_T("pam"),			NULL,			0},
//	{NULL,	_T("auth"),				_T("radius"),		NULL,			0},
	{NULL,	_T("auth"),				NULL,				"key",			0},

///	{NULL,	_T("automation"),		_T("command-list"),	NULL,			0},
///	{NULL,	_T("automation"),		_T("command-node"),	NULL,			0},
//	{NULL,	_T("automation"),		_T("rpc"),			NULL,			0},
//	{NULL,	_T("automation"),		_T("soap"),			NULL,			0},
	{NULL,	_T("automation"),		NULL,				"adhoc",		0},

//	{NULL,	_T("client"),			_T("bot"),			NULL,			0},
//	{NULL,	_T("client"),			_T("console"),		NULL,			0},
//	{NULL,	_T("client"),			_T("handheld"),		NULL,			0},
//	{NULL,	_T("client"),			_T("pc"),			NULL,			0},
//	{NULL,	_T("client"),			_T("phone"),		NULL,			0},
//	{NULL,	_T("client"),			_T("web"),			NULL,			0},
	{NULL,	_T("client"),			NULL,				NULL,			SKINICON_STATUS_ONLINE},

//	{NULL,	_T("collaboration"),	_T("whiteboard"),	NULL,			0},
	{NULL,	_T("collaboration"),	NULL,				"group",		0},

//	{NULL,	_T("component"),		_T("archive"),		NULL,			0},
//	{NULL,	_T("component"),		_T("c2s"),			NULL,			0},
//	{NULL,	_T("component"),		_T("generic"),		NULL,			0},
//	{NULL,	_T("component"),		_T("load"),			NULL,			0},
//	{NULL,	_T("component"),		_T("log"),			NULL,			0},
//	{NULL,	_T("component"),		_T("presence"),		NULL,			0},
//	{NULL,	_T("component"),		_T("router"),		NULL,			0},
//	{NULL,	_T("component"),		_T("s2s"),			NULL,			0},
//	{NULL,	_T("component"),		_T("sm"),			NULL,			0},
//	{NULL,	_T("component"),		_T("stats"),		NULL,			0},

//	{NULL,	_T("conference"),		_T("irc"),			NULL,			0},
//	{NULL,	_T("conference"),		_T("text"),			NULL,			0},
	{NULL,	_T("conference"),		NULL,				"group",		0},

	{NULL,	_T("directory"),		_T("chatroom"),		"group",		0},
	{NULL,	_T("directory"),		_T("group"),		"group",		0},
	{NULL,	_T("directory"),		_T("user"),			NULL,			SKINICON_OTHER_FINDUSER},
//	{NULL,	_T("directory"),		_T("waitinglist"),	NULL,			0},
	{NULL,	_T("directory"),		NULL,				NULL,			SKINICON_OTHER_SEARCHALL},

	{NULL,	_T("gateway"),			_T("aim"),			"AIM",			SKINICON_STATUS_ONLINE},
	{NULL,	_T("gateway"),			_T("gadu-gadu"),	"GG",			SKINICON_STATUS_ONLINE},
//	{NULL,	_T("gateway"),			_T("http-ws"),		NUL,			0},
	{NULL,	_T("gateway"),			_T("icq"),			"ICQ",			SKINICON_STATUS_ONLINE},
	{NULL,	_T("gateway"),			_T("msn"),			"MSN",			SKINICON_STATUS_ONLINE},
	{NULL,	_T("gateway"),			_T("qq"),			"QQ",			SKINICON_STATUS_ONLINE},
//	{NULL,	_T("gateway"),			_T("sms"),			NULL,			0},
//	{NULL,	_T("gateway"),			_T("smtp"),			NULL,			0},
	{NULL,	_T("gateway"),			_T("tlen"),			"TLEN",			SKINICON_STATUS_ONLINE},
	{NULL,	_T("gateway"),			_T("yahoo"),		"YAHOO",		SKINICON_STATUS_ONLINE},
	{NULL,	_T("gateway"),			NULL,				"Agents",		0},

//	{NULL,	_T("headline"),			_T("newmail"),		NULL,			0},
	{NULL,	_T("headline"),			_T("rss"),			"node_rss",		0},
	{NULL,	_T("headline"),			_T("weather"),		"node_weather",	0},

//	{NULL,	_T("hierarchy"),		_T("branch"),		NULL,			0},
//	{NULL,	_T("hierarchy"),		_T("leaf"),			NULL,			0},

//	{NULL,	_T("proxy"),			_T("bytestreams"),	NULL,			0},
	{NULL,	_T("proxy"),			NULL,				NULL,			SKINICON_EVENT_FILE},

//	{NULL,	_T("pubsub"),			_T("collection"),	NULL,			0},
//	{NULL,	_T("pubsub"),			_T("leaf"),			NULL,			0},
//	{NULL,	_T("pubsub"),			_T("pep"),			NULL,			0},
//	{NULL,	_T("pubsub"),			_T("service"),		NULL,			0},

//	{NULL,	_T("server"),			_T("im"),			NULL,			0},
	{NULL,	_T("server"),			NULL,				"node_server",	0},

//	{NULL,	_T("store"),			_T("berkeley"),		NULL,			0},
///	{NULL,	_T("store"),			_T("file"),			NULL,			0},
//	{NULL,	_T("store"),			_T("generic"),		NULL,			0},
//	{NULL,	_T("store"),			_T("ldap"),			NULL,			0},
//	{NULL,	_T("store"),			_T("mysql"),		NULL,			0},
//	{NULL,	_T("store"),			_T("oracle"),		NULL,			0},
//	{NULL,	_T("store"),			_T("postgres"),		NULL,			0},
	{NULL,	_T("store"),			NULL,				"node_store",	0},

//	icons for non-standard identities
	{NULL,	_T("x-service"),		_T("x-rss"),		"node_rss",		0},
	{NULL,	_T("application"),		_T("x-weather"),	"node_weather",	0},
	{NULL,	_T("user"),				NULL,				NULL,			SKINICON_STATUS_ONLINE},

//	icon suggestions based on supported features
	{_T("jabber:iq:gateway"),		NULL,NULL,			"Agents",		0},
	{_T("jabber:iq:search"),		NULL,NULL,			NULL,			SKINICON_OTHER_FINDUSER},
	{_T(JABBER_FEAT_COMMANDS),		NULL,NULL,			"adhoc",		0},
	{_T(JABBER_FEAT_REGISTER),		NULL,NULL,			"key",			0},
};

static void sttApplyNodeIcon(HTREELISTITEM hItem, CJabberSDNode *pNode);

void CJabberProto::OnIqResultServiceDiscoveryInfo( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	m_SDManager.Lock();
	CJabberSDNode* pNode = m_SDManager.FindByIqId( pInfo->GetIqId(), TRUE );
	if ( !pNode ) {
		m_SDManager.Unlock();
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
			pNode->SetInfoRequestErrorText( NULL );
		}
	}
	else {
		if ( pInfo->GetIqType() == JABBER_IQ_TYPE_ERROR ) {
			XmlNode *errorNode = JabberXmlGetChild( iqNode, "error" );
			TCHAR* str = JabberErrorMsg( errorNode );
			pNode->SetInfoRequestErrorText( str );
			mir_free( str );
		}
		else {
			pNode->SetInfoRequestErrorText( _T("request timeout.") );
		}
		pNode->SetInfoRequestId( JABBER_DISCO_RESULT_ERROR );
	}

	m_SDManager.Unlock();

	if ( m_hwndServiceDiscovery ) {
		ApplyNodeIcon(pNode->GetTreeItemHandle(), pNode);
		PostMessage( m_hwndServiceDiscovery, WM_JABBER_REFRESH, 0, 0 );
	}
}

void CJabberProto::OnIqResultServiceDiscoveryItems( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	m_SDManager.Lock();
	CJabberSDNode* pNode = m_SDManager.FindByIqId( pInfo->GetIqId(), FALSE );
	if ( !pNode ) {
		m_SDManager.Unlock();
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
			pNode->SetItemsRequestErrorText( NULL );
		}
	}
	else {
		if ( pInfo->GetIqType() == JABBER_IQ_TYPE_ERROR ) {
			XmlNode *errorNode = JabberXmlGetChild( iqNode, "error" );
			TCHAR* str = JabberErrorMsg( errorNode );
			pNode->SetItemsRequestErrorText( str );
			mir_free( str );
		}
		else {
			pNode->SetItemsRequestErrorText( _T("request timeout.") );
		}
		pNode->SetItemsRequestId( JABBER_DISCO_RESULT_ERROR );
	}

	m_SDManager.Unlock();

	if ( m_hwndServiceDiscovery ) {
		ApplyNodeIcon(pNode->GetTreeItemHandle(), pNode);
		PostMessage( m_hwndServiceDiscovery, WM_JABBER_REFRESH, 0, 0 );
	}
}

void CJabberProto::OnIqResultServiceDiscoveryRootInfo( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if (!pInfo->m_pUserData) return;
	m_SDManager.Lock();
	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT ) {
		XmlNode *query = JabberXmlGetChild( iqNode, "query" );
		if ( query ) {
			XmlNode *feature;
			int i;
			for ( i = 1; ( feature = JabberXmlGetNthChild( query, "feature", i )) != NULL; i++ ) {
				if (!lstrcmp(JabberXmlGetAttrValue( feature, "var" ), (TCHAR *)pInfo->m_pUserData)) {
					CJabberSDNode *pNode = m_SDManager.AddPrimaryNode( pInfo->GetReceiver(), JabberXmlGetAttrValue( iqNode, "node" ), NULL);
					SendBothRequests( pNode, NULL );
					break;
	}	}	}	}
	m_SDManager.Unlock();

	if ( m_hwndServiceDiscovery )
		PostMessage( m_hwndServiceDiscovery, WM_JABBER_REFRESH, 0, 0 );
}

void CJabberProto::OnIqResultServiceDiscoveryRootItems( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if (!pInfo->m_pUserData) return;
	m_SDManager.Lock();
	XmlNode* packet = new XmlNode( NULL );
	if ( pInfo->GetIqType() == JABBER_IQ_TYPE_RESULT ) {
		XmlNode *query = JabberXmlGetChild( iqNode, "query" );
		if ( query ) {
			XmlNode *item;
			for ( int i = 1; ( item = JabberXmlGetNthChild( query, "item", i )) != NULL; i++ ) {
				TCHAR *szJid = JabberXmlGetAttrValue( item, "jid" );
				TCHAR *szNode = JabberXmlGetAttrValue( item, "node" );
				CJabberIqInfo* pNewInfo = m_iqManager.AddHandler( &CJabberProto::OnIqResultServiceDiscoveryRootInfo, JABBER_IQ_TYPE_GET, szJid );
				pNewInfo->m_pUserData = pInfo->m_pUserData;
				pNewInfo->SetTimeout( 30000 );
				XmlNodeIq* iq = new XmlNodeIq( pNewInfo );
				XmlNode* query2 = iq->addQuery( JABBER_FEAT_DISCO_INFO );
				if ( szNode ) query2->addAttr( "node", szNode );
				packet->addChild( iq );
	}	}	}
	m_SDManager.Unlock();

	if (packet->numChild)
		m_ThreadInfo->send( *packet );
	delete packet;
}

BOOL CJabberProto::SendInfoRequest(CJabberSDNode* pNode, XmlNode* parent)
{
	if ( !pNode || !m_bJabberOnline )
		return FALSE;

	// disco#info
	if ( !pNode->GetInfoRequestId() ) {
		CJabberIqInfo* pInfo = m_iqManager.AddHandler( &CJabberProto::OnIqResultServiceDiscoveryInfo, JABBER_IQ_TYPE_GET, pNode->GetJid() );
		pInfo->SetTimeout( 30000 );
		pNode->SetInfoRequestId( pInfo->GetIqId() );

		XmlNodeIq* iq = new XmlNodeIq( pInfo );
		XmlNode* query = iq->addQuery( JABBER_FEAT_DISCO_INFO );
		if ( pNode->GetNode() )
			query->addAttr( "node", pNode->GetNode() );

		if ( parent )
			parent->addChild( iq );
		else {
			m_ThreadInfo->send( *iq );
			delete( iq );
	}	}

	if ( m_hwndServiceDiscovery ) {
		ApplyNodeIcon(pNode->GetTreeItemHandle(), pNode);
		PostMessage( m_hwndServiceDiscovery, WM_JABBER_REFRESH, 0, 0 );
	}

	return TRUE;
}

BOOL CJabberProto::SendBothRequests(CJabberSDNode* pNode, XmlNode* parent)
{
	if ( !pNode || !m_bJabberOnline )
		return FALSE;

	// disco#info
	if ( !pNode->GetInfoRequestId() ) {
		CJabberIqInfo* pInfo = m_iqManager.AddHandler( &CJabberProto::OnIqResultServiceDiscoveryInfo, JABBER_IQ_TYPE_GET, pNode->GetJid() );
		pInfo->SetTimeout( 30000 );
		pNode->SetInfoRequestId( pInfo->GetIqId() );

		XmlNodeIq* iq = new XmlNodeIq( pInfo );
		XmlNode* query = iq->addQuery( JABBER_FEAT_DISCO_INFO );
		if ( pNode->GetNode() )
			query->addAttr( "node", pNode->GetNode() );

		if ( parent )
			parent->addChild( iq );
		else {
			m_ThreadInfo->send( *iq );
			delete( iq );
	}	}

	// disco#items
	if ( !pNode->GetItemsRequestId() ) {
		CJabberIqInfo* pInfo = m_iqManager.AddHandler( &CJabberProto::OnIqResultServiceDiscoveryItems, JABBER_IQ_TYPE_GET, pNode->GetJid() );
		pInfo->SetTimeout( 30000 );
		pNode->SetItemsRequestId( pInfo->GetIqId() );

		XmlNodeIq* iq = new XmlNodeIq( pInfo );
		XmlNode* query = iq->addQuery( JABBER_FEAT_DISCO_ITEMS );
		if ( pNode->GetNode() )
			query->addAttr( "node", pNode->GetNode() );

		if ( parent )
			parent->addChild( iq );
		else {
			m_ThreadInfo->send( *iq );
			delete( iq );
	}	}

	if ( m_hwndServiceDiscovery ) {
		ApplyNodeIcon(pNode->GetTreeItemHandle(), pNode);
		PostMessage( m_hwndServiceDiscovery, WM_JABBER_REFRESH, 0, 0 );
	}

	return TRUE;
}

void CJabberProto::PerformBrowse(HWND hwndDlg)
{
	TCHAR szJid[ 512 ];
	TCHAR szNode[ 512 ];
	if ( !GetDlgItemText( hwndDlg, IDC_COMBO_JID, szJid, SIZEOF( szJid )))
		szJid[ 0 ] = _T('\0');
	if ( !GetDlgItemText( hwndDlg, IDC_COMBO_NODE, szNode, SIZEOF( szNode )))
		szNode[ 0 ] = _T('\0');
	
	ComboAddRecentString(hwndDlg, IDC_COMBO_JID, "discoWnd_rcJid", szJid);
	ComboAddRecentString(hwndDlg, IDC_COMBO_NODE, "discoWnd_rcNode", szNode);

	if ( _tcslen( szJid )) {
		HWND hwndList = GetDlgItem(hwndDlg, IDC_TREE_DISCO);
		TreeList_Reset(hwndList);

		m_SDManager.Lock();
		m_SDManager.RemoveAll();
		if (!lstrcmp(szJid, _T(SD_FAKEJID_MYAGENTS))) {
			sttBrowseMode = SD_BROWSE_MYAGENTS;
			int i = 0;
			JABBER_LIST_ITEM *item = NULL;
			while (( i=ListFindNext( LIST_ROSTER, i )) >= 0 ) {
				if (( item=ListGetItemPtrFromIndex( i )) != NULL ) {
					if ( _tcschr( item->jid, '@' )==NULL && _tcschr( item->jid, '/' )==NULL && item->subscription!=SUB_NONE ) {
						HANDLE hContact = HContactFromJID( item->jid );
						if ( hContact != NULL )
							JSetByte( hContact, "IsTransport", TRUE );

						if ( m_lstTransports.getIndex( item->jid ) == -1 )
							m_lstTransports.insert( _tcsdup( item->jid ));

						CJabberSDNode* pNode = m_SDManager.AddPrimaryNode(item->jid, NULL, NULL);
						SendBothRequests( pNode, NULL );
				}	}
				i++;
		}	}
		else if (!lstrcmp(szJid, _T(SD_FAKEJID_CONFERENCES))) {
			sttBrowseMode = SD_BROWSE_CONFERENCES;
			TCHAR *szServerJid = mir_a2t(m_ThreadInfo->server);
			CJabberIqInfo* pInfo = m_iqManager.AddHandler( &CJabberProto::OnIqResultServiceDiscoveryRootItems, JABBER_IQ_TYPE_GET, szServerJid );
			pInfo->m_pUserData = _T(JABBER_FEAT_MUC);
			pInfo->SetTimeout( 30000 );
			XmlNodeIq* iq = new XmlNodeIq( pInfo );
			XmlNode* query = iq->addQuery( JABBER_FEAT_DISCO_ITEMS );
			m_ThreadInfo->send( *iq );
			delete( iq );
			mir_free(szServerJid);
		}
		else if (!lstrcmp(szJid, _T(SD_FAKEJID_AGENTS))) {
			sttBrowseMode = SD_BROWSE_AGENTS;
			TCHAR *szServerJid = mir_a2t(m_ThreadInfo->server);
			CJabberIqInfo* pInfo = m_iqManager.AddHandler( &CJabberProto::OnIqResultServiceDiscoveryRootItems, JABBER_IQ_TYPE_GET, szServerJid );
			pInfo->m_pUserData = _T("jabber:iq:gateway");
			pInfo->SetTimeout( 30000 );
			XmlNodeIq* iq = new XmlNodeIq( pInfo );
			XmlNode* query = iq->addQuery( JABBER_FEAT_DISCO_ITEMS );
			m_ThreadInfo->send( *iq );
			delete( iq );
			mir_free(szServerJid);
		}
		else if (!lstrcmp(szJid, _T(SD_FAKEJID_FAVORITES))) {
			sttBrowseMode = SD_BROWSE_FAVORITES;
			int count = JGetDword(NULL, "discoWnd_favCount", 0);
			for (int i = 0; i < count; ++i)
			{
				DBVARIANT dbv;
				char setting[MAXMODULELABELLENGTH];
				mir_snprintf(setting, sizeof(setting), "discoWnd_favName_%d", i);
				if (!JGetStringT(NULL, setting, &dbv)) {
					DBVARIANT dbvJid, dbvNode;
					mir_snprintf(setting, sizeof(setting), "discoWnd_favJID_%d", i);
					JGetStringT(NULL, setting, &dbvJid);
					mir_snprintf(setting, sizeof(setting), "discoWnd_favNode_%d", i);
					JGetStringT(NULL, setting, &dbvNode);
					CJabberSDNode* pNode = m_SDManager.AddPrimaryNode(dbvJid.ptszVal, dbvNode.ptszVal, dbv.ptszVal);
					SendBothRequests( pNode, NULL );
					JFreeVariant(&dbv);
					JFreeVariant(&dbvJid);
					JFreeVariant(&dbvNode);
		}	}	}
		else {
			sttBrowseMode = SD_BROWSE_NORMAL;
			CJabberSDNode* pNode = m_SDManager.AddPrimaryNode(szJid, _tcslen( szNode ) ? szNode : NULL, NULL);
			SendBothRequests( pNode, NULL );
		}
		m_SDManager.Unlock();

		PostMessage( hwndDlg, WM_JABBER_REFRESH, 0, 0 );
	}
}

BOOL CJabberProto::IsNodeRegistered(CJabberSDNode *pNode)
{
	if (pNode->GetNode())
		return FALSE;

	JABBER_LIST_ITEM *item;
	if (item = ListGetItemPtr(LIST_ROSTER, pNode->GetJid()))
		return (item->subscription != SUB_NONE) ? TRUE : FALSE;

	if (item = ListGetItemPtr(LIST_BOOKMARK, pNode->GetJid()))
		return TRUE;

	return FALSE;
}

void CJabberProto::ApplyNodeIcon(HTREELISTITEM hItem, CJabberSDNode *pNode)
{
	if (!hItem || !pNode) return;

	int iIcon = -1, iOverlay = -1;

	if ((pNode->GetInfoRequestId() > 0) || (pNode->GetItemsRequestId() > 0))
		iOverlay = SD_OVERLAY_PROGRESS;
	else if (pNode->GetInfoRequestId() == JABBER_DISCO_RESULT_ERROR)
		iOverlay = SD_OVERLAY_FAIL;
	else if (pNode->GetInfoRequestId() == JABBER_DISCO_RESULT_NOT_REQUESTED) {
		if (IsNodeRegistered(pNode))
			iOverlay = SD_OVERLAY_REGISTERED;
		else
			iOverlay = SD_OVERLAY_NONE;
	} else if (pNode->GetInfoRequestId() == JABBER_DISCO_RESULT_OK) {
		if (IsNodeRegistered(pNode))
			iOverlay = SD_OVERLAY_REGISTERED;
		else if (pNode->GetInfoRequestId() == JABBER_DISCO_RESULT_ERROR)
			iOverlay = SD_OVERLAY_FAIL;
		else
			iOverlay = SD_OVERLAY_NONE;
	}

	for (int i = 0; i < SIZEOF(sttNodeIcons); ++i)
	{
		if (!sttNodeIcons[i].iconIndex && !sttNodeIcons[i].iconName) continue;

		if (sttNodeIcons[i].category)
		{
			CJabberSDIdentity *iIdentity;
			for (iIdentity = pNode->GetFirstIdentity(); iIdentity; iIdentity = iIdentity->GetNext())
				if (!lstrcmp(iIdentity->GetCategory(), sttNodeIcons[i].category) &&
					(!sttNodeIcons[i].type || !lstrcmp(iIdentity->GetType(), sttNodeIcons[i].type)))
				{
					iIcon = sttNodeIcons[i].listIndex;
					break;
				}
			if (iIdentity) break;
		}

		if (sttNodeIcons[i].feature)
		{
			CJabberSDFeature *iFeature;
			for (iFeature = pNode->GetFirstFeature(); iFeature; iFeature = iFeature->GetNext())
				if (!lstrcmp(iFeature->GetVar(), sttNodeIcons[i].feature))
				{
					iIcon = sttNodeIcons[i].listIndex;
					break;
				}
			if (iFeature) break;
		}
	}

	TreeList_SetIcon(pNode->GetTreeItemHandle(), iIcon, iOverlay);
}

BOOL CJabberProto::SyncTree(HTREELISTITEM hIndex, CJabberSDNode* pNode)
{
	CJabberSDNode* pTmp = pNode;
	while (pTmp) {
		if ( !pTmp->GetTreeItemHandle() ) {
			HTREELISTITEM hNewItem = TreeList_AddItem(
				GetDlgItem(m_hwndServiceDiscovery, IDC_TREE_DISCO), hIndex,
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

		ApplyNodeIcon(pNode->GetTreeItemHandle(), pNode);

		if ( pTmp->GetFirstChildNode() )
			SyncTree( pTmp->GetTreeItemHandle(), pTmp->GetFirstChildNode() );

		pTmp = pTmp->GetNext();
	}
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
		case IDC_TXT_NODELABEL:
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

struct JabberServiceDiscoveryDlgProcParam {
	CJabberProto* ppro;
	TCHAR* m_jid;
};

BOOL CALLBACK JabberServiceDiscoveryDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	CJabberProto* ppro = (CJabberProto*)GetWindowLong( hwndDlg, GWL_USERDATA );
	BOOL result;
	if (TreeList_ProcessMessage(hwndDlg, msg, wParam, lParam, IDC_TREE_DISCO, &result))
		return result;

	switch ( msg ) {
	case WM_INITDIALOG: {
		JabberServiceDiscoveryDlgProcParam* param = (JabberServiceDiscoveryDlgProcParam*)lParam;
		ppro = param->ppro;
		SetWindowLong( hwndDlg, GWL_USERDATA, (LPARAM)ppro );

		TranslateDialogDefault( hwndDlg );
		SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )ppro->LoadIconEx( "servicediscovery" ));
		ppro->m_hwndServiceDiscovery = hwndDlg;
		{
			int i;

			if ( param->m_jid ) {
				SetDlgItemText( hwndDlg, IDC_COMBO_JID, (TCHAR *)param->m_jid );
				SetDlgItemText( hwndDlg, IDC_COMBO_NODE, _T("") );
			} else {
				SetDlgItemTextA( hwndDlg, IDC_COMBO_JID, ppro->m_ThreadInfo->server );
				SetDlgItemText( hwndDlg, IDC_COMBO_NODE, _T("") );
			}

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
				{IDC_BTN_FAVORITE,		_T("Favorites"),		"bookmarks",		true},	// this is checked during menu loop
				{IDC_BTN_FILTERAPPLY,	_T("Apply filter"),		"sd_filter_apply",	false},
				{IDC_BTN_FILTERRESET,	_T("Reset filter"),		"sd_filter_reset",	false},
			};
			for (i = 0; i < SIZEOF(buttons); ++i)
			{
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BM_SETIMAGE, IMAGE_ICON, (LPARAM)ppro->LoadIconEx(buttons[i].icon));
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONSETASFLATBTN, 0, 0);
				SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONADDTOOLTIP, (WPARAM)TranslateTS(buttons[i].title), BATF_TCHAR);
				if (buttons[i].push)
					SendDlgItemMessage(hwndDlg, buttons[i].idc, BUTTONSETASPUSHBTN, 0, 0);
			}
			CheckDlgButton(hwndDlg,
				DBGetContactSettingByte(NULL, ppro->m_szModuleName, "discoWnd_useTree", 1) ?
					IDC_BTN_VIEWTREE : IDC_BTN_VIEWLIST,
				TRUE);

			EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_FILTERRESET), FALSE);

			SendDlgItemMessage(hwndDlg, IDC_COMBO_JID, CB_ADDSTRING, 0, (LPARAM)_T(SD_FAKEJID_CONFERENCES));
			SendDlgItemMessage(hwndDlg, IDC_COMBO_JID, CB_ADDSTRING, 0, (LPARAM)_T(SD_FAKEJID_MYAGENTS));
			SendDlgItemMessage(hwndDlg, IDC_COMBO_JID, CB_ADDSTRING, 0, (LPARAM)_T(SD_FAKEJID_AGENTS));
			SendDlgItemMessage(hwndDlg, IDC_COMBO_JID, CB_ADDSTRING, 0, (LPARAM)_T(SD_FAKEJID_FAVORITES));
			ppro->ComboLoadRecentStrings(hwndDlg, IDC_COMBO_JID, "doscoWnd_rcJid");
			ppro->ComboLoadRecentStrings(hwndDlg, IDC_COMBO_NODE, "doscoWnd_rcNode");

			HWND hwndList = GetDlgItem(hwndDlg, IDC_TREE_DISCO);
			LVCOLUMN lvc = {0};
			lvc.mask = LVCF_SUBITEM|LVCF_WIDTH|LVCF_TEXT;
			lvc.cx = DBGetContactSettingWord(NULL, ppro->m_szModuleName, "discoWnd_cx0", 200);
			lvc.iSubItem = 0;
			lvc.pszText = _T("Node hierarchy");
			ListView_InsertColumn(hwndList, 0, &lvc);
			lvc.cx = DBGetContactSettingWord(NULL, ppro->m_szModuleName, "discoWnd_cx1", 200);
			lvc.iSubItem = 1;
			lvc.pszText = _T("JID");
			ListView_InsertColumn(hwndList, 1, &lvc);
			lvc.cx = DBGetContactSettingWord(NULL, ppro->m_szModuleName, "discoWnd_cx2", 200);
			lvc.iSubItem = 2;
			lvc.pszText = _T("Node");
			ListView_InsertColumn(hwndList, 2, &lvc);

			TreeList_Create(hwndList);
			TreeList_AddIcon(hwndList, ppro->LoadIconEx("main"), 0);
			for (i = 0; i < SIZEOF(sttNodeIcons); ++i)
			{
				HICON hIcon;
				if ((sttNodeIcons[i].iconIndex == SKINICON_STATUS_ONLINE) && sttNodeIcons[i].iconName)
					hIcon = (HICON)CallProtoService(sttNodeIcons[i].iconName, PS_LOADICON, PLI_PROTOCOL|PLIF_SMALL, 0);
				else if (sttNodeIcons[i].iconName)
					hIcon = ppro->LoadIconEx(sttNodeIcons[i].iconName);
				else if (sttNodeIcons[i].iconIndex)
					hIcon = LoadSkinnedIcon(sttNodeIcons[i].iconIndex);
				else continue;
				sttNodeIcons[i].listIndex = TreeList_AddIcon(hwndList, hIcon, 0);
			}
			TreeList_AddIcon(hwndList, ppro->LoadIconEx("disco_fail"),	SD_OVERLAY_FAIL);
			TreeList_AddIcon(hwndList, ppro->LoadIconEx("disco_progress"),SD_OVERLAY_PROGRESS);
			TreeList_AddIcon(hwndList, ppro->LoadIconEx("disco_ok"),		SD_OVERLAY_REGISTERED);

			TreeList_SetMode(hwndList, DBGetContactSettingByte(NULL, ppro->m_szModuleName, "discoWnd_useTree", 1) ? TLM_TREE : TLM_REPORT);

			PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_BUTTON_BROWSE, 0 ), 0 );
		}
		Utils_RestoreWindowPosition(hwndDlg, NULL, ppro->m_szModuleName, "discoWnd_");
		return TRUE;
	}
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

	case WM_JABBER_TRANSPORT_REFRESH:
		if (ppro->m_nSDBrowseMode == SD_BROWSE_MYAGENTS) {
			SetDlgItemText(hwndDlg, IDC_COMBO_JID, _T(SD_FAKEJID_MYAGENTS));
			SetDlgItemText(hwndDlg, IDC_COMBO_NODE, _T(""));
			PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_BUTTON_BROWSE, 0 ), 0 );
		}
		break;

	case WM_JABBER_REFRESH:
		KillTimer(hwndDlg, REFRESH_TIMER);
		if (GetTickCount() - ppro->m_dwSDLastRefresh < REFRESH_TIMEOUT) {
			SetTimer(hwndDlg, REFRESH_TIMER, REFRESH_TIMEOUT, NULL);
			return TRUE;
		} 
		
		wParam = REFRESH_TIMER;
		// fall through

	case WM_TIMER:
		if (wParam == REFRESH_TIMER) {
			ppro->m_SDManager.Lock();

			CJabberSDNode* pNode = ppro->m_SDManager.GetPrimaryNode();
			while (pNode)
			{
				if ( pNode->GetJid() ) {
					if ( !pNode->GetTreeItemHandle() ) {
						HTREELISTITEM hNewItem = TreeList_AddItem(
							GetDlgItem( ppro->m_hwndServiceDiscovery, IDC_TREE_DISCO), NULL,
							pNode->GetName() ? pNode->GetName() : pNode->GetJid(),
							(LPARAM)pNode);
						TreeList_AppendColumn(hNewItem, pNode->GetJid());
						TreeList_AppendColumn(hNewItem, pNode->GetNode());
						pNode->SetTreeItemHandle( hNewItem );
				}	}
				ppro->SyncTree( NULL, pNode );
				pNode = pNode->GetNext();
			}
			ppro->m_SDManager.Unlock();
			TreeList_Update(GetDlgItem(hwndDlg, IDC_TREE_DISCO));
			KillTimer(hwndDlg, REFRESH_TIMER);
			ppro->m_dwSDLastRefresh = GetTickCount();
			return TRUE;
		}
		else if (wParam == AUTODISCO_TIMER) {
			HWND hwndList = GetDlgItem(hwndDlg, IDC_TREE_DISCO);
			RECT rcCtl; GetClientRect(hwndList, &rcCtl);
			RECT rcHdr; GetClientRect(ListView_GetHeader(hwndList), &rcHdr);
			LVHITTESTINFO lvhti = {0};
			lvhti.pt.x = rcCtl.left + 5;
			lvhti.pt.y = rcHdr.bottom + 5;
			int iFirst = ListView_HitTest(hwndList, &lvhti);
			ZeroMemory(&lvhti, sizeof(lvhti));
			lvhti.pt.x = rcCtl.left + 5;
			lvhti.pt.y = rcCtl.bottom - 5;
			int iLast = ListView_HitTest(hwndList, &lvhti);
			if (iFirst < 0) return CDRF_DODEFAULT;
			if (iLast < 0) iLast = ListView_GetItemCount(hwndList) - 1;

			ppro->m_SDManager.Lock();
			XmlNode* packet = new XmlNode( NULL );
			for (int i = iFirst; i <= iLast; ++i)
			{
				LVITEM lvi = {0};
				lvi.mask = LVIF_PARAM;
				lvi.iItem = i;
				ListView_GetItem(hwndList, &lvi);
				if (!lvi.lParam) continue;
				CJabberSDNode *pNode = (CJabberSDNode *)TreeList_GetData((HTREELISTITEM)lvi.lParam);
				if (!pNode || pNode->GetInfoRequestId()) continue;
				ppro->SendInfoRequest(pNode, packet);
			}
			ppro->m_SDManager.Unlock();
			if ( packet->numChild )
				ppro->m_ThreadInfo->send( *packet );
			delete packet;

			KillTimer(hwndDlg, AUTODISCO_TIMER);
			ppro->m_dwSDLastRefresh = GetTickCount();
			return TRUE;
		}
		return FALSE;

	case WM_CONTEXTMENU:
		if (GetWindowLong((HWND)wParam, GWL_ID) == IDC_TREE_DISCO)
		{
			HWND hwndList = (HWND)wParam;
			POINT pt = { (signed short)LOWORD( lParam ), (signed short)HIWORD( lParam ) };

			if (( pt.x == -1 ) && ( pt.y == -1 )) {
				LVITEM lvi = {0};
				lvi.iItem = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
				if (lvi.iItem < 0) return FALSE;

				RECT rc;
				ListView_GetItemRect(hwndList, lvi.iItem, &rc, LVIR_LABEL);
				pt.x = rc.left;
				pt.y = rc.bottom;
				ClientToScreen(hwndList, &pt);
			}

			HTREELISTITEM hItem = TreeList_GetActiveItem(hwndList);
			if (!hItem) break;
			CJabberSDNode *pNode = (CJabberSDNode *)TreeList_GetData(hItem);
			if (!pNode) break;

			ppro->ServiceDiscoveryShowMenu(pNode, hItem, pt);
		}
		return FALSE;

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
				ppro->m_SDManager.Lock();
				CJabberSDNode* pNode = (CJabberSDNode* )TreeList_GetData(hItem);
				if ( pNode ) {
					pNode->GetTooltipText( pInfoTip->pszText, pInfoTip->cchTextMax );
				}
				ppro->m_SDManager.Unlock();
			}
			else if ( pHeader->code == TVN_ITEMEXPANDED ) {
				NMTREEVIEW *pNmTreeView = (NMTREEVIEW *)lParam;
				HTREELISTITEM hItem = (HTREELISTITEM)pNmTreeView->itemNew.hItem;

				ppro->m_SDManager.Lock();
				XmlNode* packet = new XmlNode( NULL );
				CJabberSDNode* pNode;
				pNode = (CJabberSDNode* )TreeList_GetData(hItem);
				if ( pNode ) 
				{
					ppro->SendBothRequests( pNode, packet );
					TreeList_MakeFakeParent(hItem, FALSE);
				}
				ppro->m_SDManager.Unlock();

				if ( packet->numChild )
					ppro->m_ThreadInfo->send( *packet );
				delete packet;
			}
			else if ( pHeader->code == NM_CUSTOMDRAW ) {
				LPNMLVCUSTOMDRAW lpnmlvcd = (LPNMLVCUSTOMDRAW)lParam;
				if (lpnmlvcd->nmcd.dwDrawStage != CDDS_PREPAINT)
					return CDRF_DODEFAULT;

				KillTimer(hwndDlg, AUTODISCO_TIMER);
				if (GetTickCount() - sttLastAutoDisco < AUTODISCO_TIMEOUT) {
					SetTimer(hwndDlg, AUTODISCO_TIMER, AUTODISCO_TIMEOUT, NULL);
					return CDRF_DODEFAULT;
				}

				SendMessage(hwndDlg, WM_TIMER, AUTODISCO_TIMER, 0);

				return CDRF_DODEFAULT;
		}	}

		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDOK:
			{
				HWND hwndFocus = GetFocus();
				if (!hwndFocus) return TRUE;
				if (GetWindowLong(hwndFocus, GWL_ID) == IDC_TXT_FILTERTEXT)
					PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_BTN_FILTERAPPLY, 0 ), 0 );
				else if (hwndDlg == (hwndFocus = GetParent(hwndFocus)))
					break;
				else if ((GetWindowLong(hwndFocus, GWL_ID) == IDC_COMBO_NODE) || (GetWindowLong(hwndFocus, GWL_ID) == IDC_COMBO_JID))
					PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_BUTTON_BROWSE, 0 ), 0 );
				return TRUE;
			}
			case IDCANCEL:
			{
				PostMessage(hwndDlg, WM_CLOSE, 0, 0);
				return TRUE;
			}
			case IDC_BUTTON_BROWSE:
			{
				ppro->PerformBrowse(hwndDlg);
				return TRUE;
			}
			case IDC_BTN_REFRESH:
			{
				HTREELISTITEM hItem = (HTREELISTITEM)TreeList_GetActiveItem(GetDlgItem(hwndDlg, IDC_TREE_DISCO));
				if (!hItem) return TRUE;

				ppro->m_SDManager.Lock();
				XmlNode* packet = new XmlNode( NULL );
				CJabberSDNode* pNode = (CJabberSDNode* )TreeList_GetData(hItem);
				if ( pNode ) {
					TreeList_ResetItem(GetDlgItem(hwndDlg, IDC_TREE_DISCO), hItem);
					pNode->ResetInfo();
					ppro->SendBothRequests( pNode, packet );
					TreeList_MakeFakeParent(hItem, FALSE);
				}
				ppro->m_SDManager.Unlock();

				if ( packet->numChild )
					ppro->m_ThreadInfo->send( *packet );
				delete packet;
				return TRUE;
			}
			case IDC_BTN_NAVHOME:
			{
				SetDlgItemTextA( hwndDlg, IDC_COMBO_JID, ppro->m_ThreadInfo->server );
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
			case IDC_BTN_FAVORITE:
			{
				HMENU hMenu = CreatePopupMenu();
				int count = ppro->JGetDword(NULL, "discoWnd_favCount", 0);
				for (int i = 0; i < count; ++i)
				{
					DBVARIANT dbv;
					char setting[MAXMODULELABELLENGTH];
					mir_snprintf(setting, sizeof(setting), "discoWnd_favName_%d", i);
					if (!ppro->JGetStringT(NULL, setting, &dbv))
					{
						HMENU hSubMenu = CreatePopupMenu();
						AppendMenu(hSubMenu, MF_STRING, 100+i*10+0, TranslateT("Navigate"));
						AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
						AppendMenu(hSubMenu, MF_STRING, 100+i*10+1, TranslateT("Remove"));
						AppendMenu(hMenu, MF_POPUP|MF_STRING, (UINT_PTR)hSubMenu, dbv.ptszVal);
					}
					JFreeVariant(&dbv);
				}
				int res = 0;
				if (GetMenuItemCount(hMenu)) {
					AppendMenu(hMenu, MF_SEPARATOR, 1, NULL);
					AppendMenu(hMenu, MF_STRING, 10+SD_BROWSE_FAVORITES, TranslateT("Browse all favorites"));
					AppendMenu(hMenu, MF_STRING, 1, TranslateT("Remove all favorites"));
				}
				if (GetMenuItemCount(hMenu))
					AppendMenu(hMenu, MF_SEPARATOR, 1, NULL);

				AppendMenu(hMenu, MF_STRING, 10+SD_BROWSE_MYAGENTS, TranslateT("Registered transports"));
				AppendMenu(hMenu, MF_STRING, 10+SD_BROWSE_AGENTS, TranslateT("Browse local transports"));
				AppendMenu(hMenu, MF_STRING, 10+SD_BROWSE_CONFERENCES, TranslateT("Browse chatrooms"));

				RECT rc; GetWindowRect(GetDlgItem(hwndDlg, IDC_BTN_FAVORITE), &rc);
				CheckDlgButton(hwndDlg, IDC_BTN_FAVORITE, TRUE);
				res = TrackPopupMenu(hMenu, TPM_RETURNCMD, rc.left, rc.bottom, 0, ppro->m_hwndServiceDiscovery, NULL);
				CheckDlgButton(hwndDlg, IDC_BTN_FAVORITE, FALSE);
				DestroyMenu(hMenu);

				if (res >= 100)
				{
					res -= 100;
					if (res % 10)
					{
						res /= 10;
						char setting[MAXMODULELABELLENGTH];
						mir_snprintf(setting, sizeof(setting), "discoWnd_favName_%d", res);
						ppro->JDeleteSetting(NULL, setting);
						mir_snprintf(setting, sizeof(setting), "discoWnd_favJID_%d", res);
						ppro->JDeleteSetting(NULL, setting);
						mir_snprintf(setting, sizeof(setting), "discoWnd_favNode_%d", res);
						ppro->JDeleteSetting(NULL, setting);
					} else
					{
						res /= 10;

						SetDlgItemText(hwndDlg, IDC_COMBO_JID, _T(""));
						SetDlgItemText(hwndDlg, IDC_COMBO_NODE, _T(""));

						DBVARIANT dbv;
						char setting[MAXMODULELABELLENGTH];
						mir_snprintf(setting, sizeof(setting), "discoWnd_favJID_%d", res);
						if (!ppro->JGetStringT(NULL, setting, &dbv)) SetDlgItemText(hwndDlg, IDC_COMBO_JID, dbv.ptszVal);
						JFreeVariant(&dbv);
						mir_snprintf(setting, sizeof(setting), "discoWnd_favNode_%d", res);
						if (!ppro->JGetStringT(NULL, setting, &dbv)) SetDlgItemText(hwndDlg, IDC_COMBO_NODE, dbv.ptszVal);
						JFreeVariant(&dbv);
						
						PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_BUTTON_BROWSE, 0 ), 0 );
					}
				} else
				if (res == 1)
				{
					int count = ppro->JGetDword(NULL, "discoWnd_favCount", 0);
					for (int i = 0; i < count; ++i)
					{
						char setting[MAXMODULELABELLENGTH];
						mir_snprintf(setting, sizeof(setting), "discoWnd_favName_%d", i);
						ppro->JDeleteSetting(NULL, setting);
						mir_snprintf(setting, sizeof(setting), "discoWnd_favJID_%d", i);
						ppro->JDeleteSetting(NULL, setting);
						mir_snprintf(setting, sizeof(setting), "discoWnd_favNode_%d", i);
						ppro->JDeleteSetting(NULL, setting);
					}
					ppro->JDeleteSetting(NULL, "discoWnd_favCount");
				} else
				if ((res >= 10) && (res <= 20))
				{
					switch (res-10) {
					case SD_BROWSE_FAVORITES:
						SetDlgItemText(hwndDlg, IDC_COMBO_JID, _T(SD_FAKEJID_FAVORITES));
						break;
					case SD_BROWSE_MYAGENTS:
						SetDlgItemText(hwndDlg, IDC_COMBO_JID, _T(SD_FAKEJID_MYAGENTS));
						break;
					case SD_BROWSE_AGENTS:
						SetDlgItemText(hwndDlg, IDC_COMBO_JID, _T(SD_FAKEJID_AGENTS));
						break;
					case SD_BROWSE_CONFERENCES:
						SetDlgItemText(hwndDlg, IDC_COMBO_JID, _T(SD_FAKEJID_CONFERENCES));
						break;
					}
					SetDlgItemText(hwndDlg, IDC_COMBO_NODE, _T(""));
					PostMessage( hwndDlg, WM_COMMAND, MAKEWPARAM( IDC_BUTTON_BROWSE, 0 ), 0 );
				}

				CheckDlgButton(hwndDlg, IDC_BTN_FAVORITE, FALSE);
				return TRUE;
			}
		}
		return FALSE;

	case WM_MEASUREITEM:
		return CallService(MS_CLIST_MENUMEASUREITEM, wParam, lParam);
	case WM_DRAWITEM:
		return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);

	case WM_CLOSE:
	{
		DBWriteContactSettingByte(NULL, ppro->m_szModuleName, "discoWnd_useTree", IsDlgButtonChecked(hwndDlg, IDC_BTN_VIEWTREE));

		HWND hwndList = GetDlgItem(hwndDlg, IDC_TREE_DISCO);
		LVCOLUMN lvc = {0};
		lvc.mask = LVCF_WIDTH;
		ListView_GetColumn(hwndList, 0, &lvc);
		DBWriteContactSettingWord(NULL, ppro->m_szModuleName, "discoWnd_cx0", lvc.cx);
		ListView_GetColumn(hwndList, 1, &lvc);
		DBWriteContactSettingWord(NULL, ppro->m_szModuleName, "discoWnd_cx1", lvc.cx);
		ListView_GetColumn(hwndList, 2, &lvc);
		DBWriteContactSettingWord(NULL, ppro->m_szModuleName, "discoWnd_cx2", lvc.cx);

		Utils_SaveWindowPosition(hwndDlg, NULL, ppro->m_szModuleName, "discoWnd_");
		DestroyWindow( hwndDlg );
		return TRUE;
	}

	case WM_DESTROY:
		ppro->m_hwndServiceDiscovery = NULL;
		ppro->m_SDManager.Lock();
		ppro->m_SDManager.RemoveAll();
		ppro->m_SDManager.Unlock();
		TreeList_Destroy(GetDlgItem(hwndDlg, IDC_TREE_DISCO));
		break;
	}
	return FALSE;
}

// extern references to used functions:
void SearchAddToRecent( TCHAR* szAddr, HWND hwndDialog = NULL );

void CJabberProto::ServiceDiscoveryShowMenu(CJabberSDNode *pNode, HTREELISTITEM hItem, POINT pt)
{
	//ClientToScreen(GetDlgItem(hwndServiceDiscovery, IDC_TREE_DISCO), &pt);

	enum { // This values are below CLISTMENUIDMAX and won't overlap
		SD_ACT_REFRESH = 1, SD_ACT_REFRESHCHILDREN, SD_ACT_FAVORITE,
		SD_ACT_ROSTER, SD_ACT_COPYJID, SD_ACT_COPYNODE, SD_ACT_USERMENU,
		SD_ACT_COPYINFO,

		SD_ACT_LOGON = 100, SD_ACT_LOGOFF, SD_ACT_UNREGISTER,

		SD_ACT_REGISTER = 200, SD_ACT_ADHOC, SD_ACT_ADDDIRECTORY,
		SD_ACT_JOIN, SD_ACT_BOOKMARK, SD_ACT_PROXY, SD_ACT_VCARD
	};

	enum {
		SD_FLG_NONODE			= 0x001,
		SD_FLG_NOTONROSTER		= 0x002,
		SD_FLG_ONROSTER			= 0x004,
		SD_FLG_SUBSCRIBED		= 0x008,
		SD_FLG_NOTSUBSCRIBED	= 0x010,
		SD_FLG_ONLINE			= 0x020,
		SD_FLG_NOTONLINE		= 0x040,
		SD_FLG_NORESOURCE		= 0x080,
		SD_FLG_HASUSER			= 0x100
	};

	static struct
	{
		TCHAR *feature;
		TCHAR *title;
		int action;
		DWORD flags;
	} items[] =
	{
		{NULL,							_T("Contact Menu..."),		SD_ACT_USERMENU,			SD_FLG_NONODE},
		{NULL,							_T("View vCard"),			SD_ACT_VCARD,				SD_FLG_NONODE},
		{_T(JABBER_FEAT_MUC),			_T("Join chatroom"),		SD_ACT_JOIN,				SD_FLG_NORESOURCE},
		{0},
		{NULL,							_T("Refresh Info"),			SD_ACT_REFRESH},
		{NULL,							_T("Refresh Children"),		SD_ACT_REFRESHCHILDREN},
		{0},
		{NULL,							_T("Add to favorites"),		SD_ACT_FAVORITE},
		{NULL,							_T("Add to roster"),		SD_ACT_ROSTER,				SD_FLG_NONODE|SD_FLG_NOTONROSTER},
		{_T(JABBER_FEAT_MUC),			_T("Bookmark chatroom"),	SD_ACT_BOOKMARK,			SD_FLG_NORESOURCE|SD_FLG_HASUSER},
		{_T("jabber:iq:search"),		_T("Add search directory"),	SD_ACT_ADDDIRECTORY},
		{_T(JABBER_FEAT_BYTESTREAMS),	_T("Use this proxy"),		SD_ACT_PROXY},
		{0},
		{_T(JABBER_FEAT_REGISTER),		_T("Register"),				SD_ACT_REGISTER},
		{_T("jabber:iq:gateway"),		_T("Unregister"),			SD_ACT_UNREGISTER,			SD_FLG_ONROSTER|SD_FLG_SUBSCRIBED},
		{_T(JABBER_FEAT_COMMANDS),		_T("Commands..."),			SD_ACT_ADHOC},
		{0},
		{_T("jabber:iq:gateway"),		_T("Logon"),				SD_ACT_LOGON,				SD_FLG_ONROSTER|SD_FLG_SUBSCRIBED|SD_FLG_ONLINE},
		{_T("jabber:iq:gateway"),		_T("Logoff"),				SD_ACT_LOGOFF,				SD_FLG_ONROSTER|SD_FLG_SUBSCRIBED|SD_FLG_NOTONLINE},
		{0},
		{NULL,							_T("Copy JID"),				SD_ACT_COPYJID},
		{NULL,							_T("Copy node name"),		SD_ACT_COPYNODE},
		{NULL,							_T("Copy node information"),SD_ACT_COPYINFO},
	};

	HMENU hMenu = CreatePopupMenu();
	BOOL lastSeparator = TRUE;
	for (int i = 0; i < SIZEOF(items); ++i)
	{
		JABBER_LIST_ITEM *rosterItem = NULL;

		if ((items[i].flags&SD_FLG_NONODE) && pNode->GetNode())
			continue;
		if ((items[i].flags&SD_FLG_NOTONROSTER) && (rosterItem = ListGetItemPtr(LIST_ROSTER, pNode->GetJid())))
			continue;
		if ((items[i].flags&SD_FLG_ONROSTER) && !(rosterItem = ListGetItemPtr(LIST_ROSTER, pNode->GetJid())))
			continue;
		if ((items[i].flags&SD_FLG_SUBSCRIBED) && (!rosterItem || (rosterItem->subscription == SUB_NONE)))
			continue;
		if ((items[i].flags&SD_FLG_NOTSUBSCRIBED) && (rosterItem && (rosterItem->subscription != SUB_NONE)))
			continue;
		if ((items[i].flags&SD_FLG_ONLINE) && rosterItem && (rosterItem->itemResource.status != ID_STATUS_OFFLINE))
			continue;
		if ((items[i].flags&SD_FLG_NOTONLINE) && rosterItem && (rosterItem->itemResource.status == ID_STATUS_OFFLINE))
			continue;
		if ((items[i].flags&SD_FLG_NORESOURCE) && _tcschr(pNode->GetJid(), _T('/')))
			continue;
		if ((items[i].flags&SD_FLG_HASUSER) && !_tcschr(pNode->GetJid(), _T('@')))
			continue;

		if (!items[i].feature)
		{
			if (items[i].title)
			{
				HANDLE hContact;
				if ((items[i].action == SD_ACT_USERMENU) && (hContact = HContactFromJID(pNode->GetJid()))) {
					HMENU hContactMenu = (HMENU)CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM)hContact, 0);
					AppendMenu(hMenu, MF_STRING|MF_POPUP, (UINT_PTR)hContactMenu, TranslateTS(items[i].title));
				} else
					AppendMenu(hMenu, MF_STRING, items[i].action, TranslateTS(items[i].title));
				lastSeparator = FALSE;
			} else
			if (!lastSeparator)
			{
				AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
				lastSeparator = TRUE;
			}
			continue;
		}
		for (CJabberSDFeature *iFeature = pNode->GetFirstFeature(); iFeature; iFeature = iFeature->GetNext())
			if (!lstrcmp(iFeature->GetVar(), items[i].feature))
			{
				if (items[i].title)
				{
					AppendMenu(hMenu, MF_STRING, items[i].action, TranslateTS(items[i].title));
					lastSeparator = FALSE;
				} else
				if (!lastSeparator)
				{
					AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
					lastSeparator = TRUE;
				}
				break;
			}
	}

	if (!GetMenuItemCount(hMenu))
	{
		DestroyMenu(hMenu);
		return;
	}

	int res = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hwndServiceDiscovery, NULL);
	DestroyMenu(hMenu);

	switch (res)
	{
		case SD_ACT_REFRESH:
		{
			m_SDManager.Lock();
			XmlNode* packet = new XmlNode( NULL );
			if ( pNode ) 
			{
				TreeList_ResetItem(GetDlgItem(m_hwndServiceDiscovery, IDC_TREE_DISCO), hItem);
				pNode->ResetInfo();
				SendBothRequests( pNode, packet );
				TreeList_MakeFakeParent(hItem, FALSE);
			}
			m_SDManager.Unlock();

			if ( packet->numChild )
				m_ThreadInfo->send( *packet );
			delete packet;
			break;
		}

		case SD_ACT_REFRESHCHILDREN:
		{
			m_SDManager.Lock();
			XmlNode* packet = new XmlNode( NULL );
			for (int iChild = TreeList_GetChildrenCount(hItem); iChild--; ) {
				HTREELISTITEM hNode = TreeList_GetChild(hItem, iChild);
				CJabberSDNode *pNode = (CJabberSDNode *)TreeList_GetData(hNode);
				if ( pNode )
				{
					TreeList_ResetItem(GetDlgItem(m_hwndServiceDiscovery, IDC_TREE_DISCO), hNode);
					pNode->ResetInfo();
					SendBothRequests( pNode, packet );
					TreeList_MakeFakeParent(hNode, FALSE);
				}

				if ( packet->numChild >= 50 ) {
					m_ThreadInfo->send( *packet );
					delete packet;
					packet = new XmlNode( NULL );
			}	}
			m_SDManager.Unlock();

			if ( packet->numChild )
				m_ThreadInfo->send( *packet );
			delete packet;
			break;
		}

		case SD_ACT_COPYJID:
			JabberCopyText(m_hwndServiceDiscovery, pNode->GetJid());
			break;

		case SD_ACT_COPYNODE:
			JabberCopyText(m_hwndServiceDiscovery, pNode->GetNode());
			break;

		case SD_ACT_COPYINFO:
		{
			TCHAR buf[8192];
			pNode->GetTooltipText(buf, SIZEOF(buf));
			JabberCopyText(m_hwndServiceDiscovery, buf);
			break;
		}

		case SD_ACT_FAVORITE:
		{
			char setting[MAXMODULELABELLENGTH];
			int count = JGetDword(NULL, "discoWnd_favCount", 0);
			mir_snprintf(setting, sizeof(setting), "discoWnd_favName_%d", count);
			JSetStringT(NULL, setting, pNode->GetName() ? pNode->GetName() : pNode->GetJid());
			mir_snprintf(setting, sizeof(setting), "discoWnd_favJID_%d", count);
			JSetStringT(NULL, setting, pNode->GetJid());
			mir_snprintf(setting, sizeof(setting), "discoWnd_favNode_%d", count);
			JSetStringT(NULL, setting, pNode->GetNode() ? pNode->GetNode() : _T(""));
			JSetDword(NULL, "discoWnd_favCount", ++count);
			break;
		}

		case SD_ACT_REGISTER:
			RegisterAgent(m_hwndServiceDiscovery, pNode->GetJid());
			break;

		case SD_ACT_ADHOC:
		{
			CJabberAdhocStartupParams* pStartupParams = new CJabberAdhocStartupParams( this, pNode->GetJid(), pNode->GetNode() );
			ContactMenuAdhocCommands( pStartupParams );
			break;
		}

		case SD_ACT_ADDDIRECTORY:
			SearchAddToRecent(pNode->GetJid());
			break;

		case SD_ACT_PROXY:
		{
			JSetByte( "BsDirect", FALSE );
			JSetByte( "BsProxyManual", TRUE );
			JSetStringT( NULL, "BsProxyServer", pNode->GetJid());
			break;
		}

		case SD_ACT_JOIN:
			if ( jabberChatDllPresent )
				GroupchatJoinRoomByJid(m_hwndServiceDiscovery, pNode->GetJid());
			else
				JabberChatDllError();
			break;

		case SD_ACT_BOOKMARK:
		{
			JABBER_LIST_ITEM* item = ListGetItemPtr( LIST_BOOKMARK, pNode->GetJid() );
			if ( item == NULL ) {
				item = ListGetItemPtr( LIST_ROOM, pNode->GetJid() );
				if ( item == NULL ) {
					item = ListAdd( LIST_ROOM, pNode->GetJid() );
					item->name = mir_tstrdup( pNode->GetName() );
				}
				if ( item != NULL ) {
					item->type = _T("conference");
					AddEditBookmark( item );
			}	}
			break;
		}

		case SD_ACT_USERMENU:
		{
			HANDLE hContact = HContactFromJID( pNode->GetJid() );
			if ( !hContact ) {
				hContact = DBCreateContact( pNode->GetJid(), pNode->GetName(), TRUE, FALSE );
				JABBER_LIST_ITEM* item = ListAdd( LIST_VCARD_TEMP, pNode->GetJid() );
				item->bUseResource = TRUE;
			}
			HMENU hContactMenu = (HMENU)CallService(MS_CLIST_MENUBUILDCONTACT, (WPARAM)hContact, 0);
			GetCursorPos(&pt);
			int res = TrackPopupMenu(hContactMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hwndServiceDiscovery, NULL);
			CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(res, MPCF_CONTACTMENU), (LPARAM)hContact);
			break;
		}

		case SD_ACT_VCARD:
		{
			TCHAR * jid = pNode->GetJid();
			HANDLE hContact = HContactFromJID(pNode->GetJid());
			if ( !hContact ) {	
				JABBER_SEARCH_RESULT jsr={0};
				mir_sntprintf( jsr.jid, SIZEOF(jsr.jid), _T("%s"), jid );
				jsr.hdr.cbSize = sizeof( JABBER_SEARCH_RESULT );
				hContact = ( HANDLE )CallProtoService( m_szModuleName, PS_ADDTOLIST, PALF_TEMPORARY, ( LPARAM )&jsr );
			}
			if ( ListGetItemPtr( LIST_VCARD_TEMP, pNode->GetJid()) == NULL ) {
				JABBER_LIST_ITEM* item = ListAdd( LIST_VCARD_TEMP, pNode->GetJid() );
				item->bUseResource = TRUE;
				if ( item->resource == NULL )
					ListAddResource( LIST_VCARD_TEMP, jid, ID_STATUS_OFFLINE, NULL, 0);
			}
			CallService(MS_USERINFO_SHOWDIALOG, (WPARAM)hContact, 0);
			break;
		}

		case SD_ACT_ROSTER:
		{
			HANDLE hContact = DBCreateContact(pNode->GetJid(), pNode->GetName(), FALSE, FALSE);
			DBDeleteContactSetting( hContact, "CList", "NotOnList" );
			JABBER_LIST_ITEM* item = ListAdd( LIST_VCARD_TEMP, pNode->GetJid() );
			item->bUseResource = TRUE;
			break;
		}

		case SD_ACT_LOGON:
		case SD_ACT_LOGOFF:
		{
			XmlNode p( "presence" ); p.addAttr( "to", pNode->GetJid() );
			if ( res != SD_ACT_LOGON )
				p.addAttr( "type", "unavailable" );
			m_ThreadInfo->send( p );
			break;
		}

		case SD_ACT_UNREGISTER:
			{	XmlNodeIq iq( "set", NOID, pNode->GetJid() );
				XmlNode* query = iq.addQuery( JABBER_FEAT_REGISTER );
				query->addChild( "remove" );
				m_ThreadInfo->send( iq );
			}
			{	XmlNodeIq iq( "set" );
				XmlNode* query = iq.addQuery( JABBER_FEAT_IQ_ROSTER );
				XmlNode* itm = query->addChild( "item" ); itm->addAttr( "jid", pNode->GetJid() ); itm->addAttr( "subscription", "remove" );
				m_ThreadInfo->send( iq );
			}
			break;

		default:
			if ((res >= CLISTMENUIDMIN) && (res <= CLISTMENUIDMAX)) {
				HANDLE hContact = HContactFromJID(pNode->GetJid());
				if (hContact)
					CallService(MS_CLIST_MENUPROCESSCOMMAND, MAKEWPARAM(res, MPCF_CONTACTMENU), (LPARAM)hContact);
			}
			break;
	}
}

int __cdecl CJabberProto::OnMenuHandleServiceDiscovery( WPARAM wParam, LPARAM lParam )
{
	if ( m_hwndServiceDiscovery && IsWindow( m_hwndServiceDiscovery ))
		SetForegroundWindow( m_hwndServiceDiscovery );
	else {
		JabberServiceDiscoveryDlgProcParam param = { this, NULL };
		CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_SERVICE_DISCOVERY ), NULL, JabberServiceDiscoveryDlgProc, (LPARAM)&param );
	}

	return 0;
}

int __cdecl CJabberProto::OnMenuHandleServiceDiscoveryMyTransports( WPARAM wParam, LPARAM lParam )
{
	if ( m_hwndServiceDiscovery && IsWindow( m_hwndServiceDiscovery )) {
		SetForegroundWindow( m_hwndServiceDiscovery );
		SetDlgItemText( m_hwndServiceDiscovery, IDC_COMBO_JID, _T(SD_FAKEJID_MYAGENTS));
		SetDlgItemText( m_hwndServiceDiscovery, IDC_COMBO_NODE, _T(""));
		PostMessage( m_hwndServiceDiscovery, WM_COMMAND, MAKEWPARAM( IDC_BUTTON_BROWSE, 0 ), 0 );
	}
	else {
		JabberServiceDiscoveryDlgProcParam param = { this, _T(SD_FAKEJID_MYAGENTS) };
		CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_SERVICE_DISCOVERY ), NULL, JabberServiceDiscoveryDlgProc, (LPARAM)&param );
	}

	return 0;
}

int __cdecl CJabberProto::OnMenuHandleServiceDiscoveryTransports( WPARAM wParam, LPARAM lParam )
{
	if ( m_hwndServiceDiscovery && IsWindow( m_hwndServiceDiscovery )) {
		SetForegroundWindow( m_hwndServiceDiscovery );
		SetDlgItemText( m_hwndServiceDiscovery, IDC_COMBO_JID, _T(SD_FAKEJID_AGENTS));
		SetDlgItemText( m_hwndServiceDiscovery, IDC_COMBO_NODE, _T(""));
		PostMessage( m_hwndServiceDiscovery, WM_COMMAND, MAKEWPARAM( IDC_BUTTON_BROWSE, 0 ), 0 );
	}
	else {
		JabberServiceDiscoveryDlgProcParam param = { this, _T(SD_FAKEJID_AGENTS) };
		CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_SERVICE_DISCOVERY ), NULL, JabberServiceDiscoveryDlgProc, (LPARAM)&param );
	}

	return 0;
}

int __cdecl CJabberProto::OnMenuHandleServiceDiscoveryConferences( WPARAM wParam, LPARAM lParam )
{
	if ( m_hwndServiceDiscovery && IsWindow( m_hwndServiceDiscovery )) {
		SetForegroundWindow( m_hwndServiceDiscovery );
		SetDlgItemText( m_hwndServiceDiscovery, IDC_COMBO_JID, _T(SD_FAKEJID_CONFERENCES));
		SetDlgItemText( m_hwndServiceDiscovery, IDC_COMBO_NODE, _T(""));
		PostMessage(  m_hwndServiceDiscovery, WM_COMMAND, MAKEWPARAM( IDC_BUTTON_BROWSE, 0 ), 0 );
	}
	else {
		JabberServiceDiscoveryDlgProcParam param = { this, _T(SD_FAKEJID_CONFERENCES) };
		CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_SERVICE_DISCOVERY ), NULL, JabberServiceDiscoveryDlgProc, (LPARAM)&param );
	}

	return 0;
}
