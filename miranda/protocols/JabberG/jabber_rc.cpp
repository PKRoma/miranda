/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

XEP-0146 support for Miranda IM

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
#include "jabber_rc.h"
#include "m_awaymsg.h"

CJabberAdhocSession::CJabberAdhocSession( CJabberProto* global )
{
	m_pNext = NULL;
	m_pUserData = NULL;
	m_bAutofreeUserData = FALSE;
	m_dwStage = 0;
	ppro = global;
	m_szSessionId.Format(_T("%u%u"), ppro->SerialNext(), GetTickCount());
	m_dwStartTime = GetTickCount();
}

BOOL CJabberProto::IsRcRequestAllowedByACL( CJabberIqInfo* pInfo )
{
	if ( !pInfo || !pInfo->GetFrom() )
		return FALSE;

	TCHAR* szFrom = JabberPrepareJid( pInfo->GetFrom() );
	if ( !szFrom )
		return FALSE;

	TCHAR* szTo = JabberPrepareJid( m_ThreadInfo->fullJID );
	if ( !szTo ) {
		mir_free( szFrom );
		return FALSE;
	}

	TCHAR* pDelimiter = _tcschr( szFrom, _T('/') );
	if ( pDelimiter ) *pDelimiter = _T('\0');

	pDelimiter = _tcschr( szTo, _T('/') );
	if ( pDelimiter ) *pDelimiter = _T('\0');

	BOOL bRetVal = _tcscmp( szFrom, szTo ) == 0;
	
	mir_free( szFrom );
	mir_free( szTo );
	
	return bRetVal;
}

void CJabberProto::HandleAdhocCommandRequest( HXML iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if ( !pInfo->GetChildNode() )
		return;

	if ( !JGetByte( "EnableRemoteControl", FALSE ) || !IsRcRequestAllowedByACL( pInfo )) {
		// FIXME: send error and return
		return;
	}

	const TCHAR* szNode = xmlGetAttrValue( pInfo->GetChildNode(), _T("node"));
	if ( !szNode )
		return;

	m_adhocManager.HandleCommandRequest( iqNode, userdata, pInfo, ( TCHAR* )szNode );
}

BOOL CJabberAdhocManager::HandleItemsRequest( HXML iqNode, void* userdata, CJabberIqInfo* pInfo, const TCHAR* szNode )
{
	if ( !szNode || !m_pProto->JGetByte( "EnableRemoteControl", FALSE ) || !m_pProto->IsRcRequestAllowedByACL( pInfo ))
		return FALSE;

	if ( !_tcscmp( szNode, _T(JABBER_FEAT_COMMANDS)))
	{
		XmlNodeIq iq( "result", pInfo );
		HXML resultQuery = xmlAddChild( iq, "query" );
		xmlAddAttr( resultQuery, "xmlns", _T(JABBER_FEAT_DISCO_ITEMS) );
		xmlAddAttr( resultQuery, "node", _T(JABBER_FEAT_COMMANDS) );

		Lock();
		CJabberAdhocNode* pNode = GetFirstNode();
		while ( pNode ) {
			HXML item = xmlAddChild( resultQuery, "item" );
			TCHAR* szJid = pNode->GetJid();
			if ( !szJid )
				szJid = m_pProto->m_ThreadInfo->fullJID;
			xmlAddAttr( item, "jid", szJid );
			xmlAddAttr( item, "node", pNode->GetNode() );
			xmlAddAttr( item, "name", pNode->GetName() );

			pNode = pNode->GetNext();
		}
		Unlock();

		m_pProto->m_ThreadInfo->send( iq );
		return TRUE;
	}
	return FALSE;
}

BOOL CJabberAdhocManager::HandleInfoRequest( HXML iqNode, void* userdata, CJabberIqInfo* pInfo, const TCHAR* szNode )
{
	if ( !szNode || !m_pProto->JGetByte( "EnableRemoteControl", FALSE ) || !m_pProto->IsRcRequestAllowedByACL( pInfo ))
		return FALSE;

	// FIXME: same code twice
	if ( !_tcscmp( szNode, _T(JABBER_FEAT_COMMANDS))) {
		XmlNodeIq iq( "result", pInfo );
		HXML resultQuery = xmlAddChild( iq, "query" );
		xmlAddAttr( resultQuery, "xmlns", _T(JABBER_FEAT_DISCO_INFO) );
		xmlAddAttr( resultQuery, "node", _T(JABBER_FEAT_COMMANDS) );

		HXML identity = xmlAddChild( resultQuery, "identity" );
		xmlAddAttr( identity, "name", "Ad-hoc commands" );
		xmlAddAttr( identity, "category", "automation" );
		xmlAddAttr( identity, "type", "command-node" );

		HXML feature = xmlAddChild( resultQuery, "feature" );
		xmlAddAttr( feature, "var", _T(JABBER_FEAT_COMMANDS) );

		feature = xmlAddChild( resultQuery, "feature" );
		xmlAddAttr( feature, "var", _T(JABBER_FEAT_DATA_FORMS) );

		feature = xmlAddChild( resultQuery, "feature" );
		xmlAddAttr( feature, "var", _T(JABBER_FEAT_DISCO_INFO) );

		feature = xmlAddChild( resultQuery, "feature" );
		xmlAddAttr( feature, "var", _T(JABBER_FEAT_DISCO_ITEMS) );

		m_pProto->m_ThreadInfo->send( iq );
		return TRUE;
	}

	Lock();
	CJabberAdhocNode *pNode = FindNode( szNode );
	if ( pNode ) {
		XmlNodeIq iq( "result", pInfo );
		HXML resultQuery = xmlAddChild( iq, "query" );
		xmlAddAttr( resultQuery, "xmlns", _T(JABBER_FEAT_DISCO_INFO) );
		xmlAddAttr( resultQuery, "node", szNode );

		HXML identity = xmlAddChild( resultQuery, "identity" );
		xmlAddAttr( identity, "name", pNode->GetName() );
		xmlAddAttr( identity, "category", "automation" );
		xmlAddAttr( identity, "type", "command-node" );

		HXML feature = xmlAddChild( resultQuery, "feature" );
		xmlAddAttr( feature, "var", _T(JABBER_FEAT_COMMANDS) );

		feature = xmlAddChild( resultQuery, "feature" );
		xmlAddAttr( feature, "var", _T(JABBER_FEAT_DATA_FORMS) );

		feature = xmlAddChild( resultQuery, "feature" );
		xmlAddAttr( feature, "var", _T(JABBER_FEAT_DISCO_INFO) );

		Unlock();
		m_pProto->m_ThreadInfo->send( iq );
		return TRUE;
	}
	Unlock();
	return FALSE;
}

BOOL CJabberAdhocManager::HandleCommandRequest( HXML iqNode, void* userdata, CJabberIqInfo* pInfo, const TCHAR* szNode )
{
	// ATTN: ACL and db settings checked in calling function

	HXML commandNode = pInfo->GetChildNode();

	Lock();
	CJabberAdhocNode* pNode = FindNode( szNode );
	if ( !pNode ) {
		Unlock();

		XmlNodeIq iq( "error", pInfo );

		HXML errorNode = xmlAddChild( iq, "error" );
		xmlAddAttr( errorNode, "type", "cancel" );
		HXML typeNode = xmlAddChild( errorNode, "item-not-found" );
		xmlAddAttr( typeNode, "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );

		m_pProto->m_ThreadInfo->send( iq );
		return FALSE;
	}

	const TCHAR* szSessionId = xmlGetAttrValue( commandNode, _T("sessionid"));

	CJabberAdhocSession* pSession = NULL;
	if ( szSessionId ) {
		pSession = FindSession( szSessionId );
		if ( !pSession ) {
			Unlock();

			XmlNodeIq iq( "error", pInfo );

			HXML errorNode = xmlAddChild( iq, "error" );
			xmlAddAttr( errorNode, "type", "modify" );
			HXML typeNode = xmlAddChild( errorNode, "bad-request" );
			xmlAddAttr( typeNode, "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );

			typeNode = xmlAddChild( errorNode, "bad-sessionid" );
			xmlAddAttr( typeNode, "xmlns", JABBER_FEAT_COMMANDS );

			m_pProto->m_ThreadInfo->send( iq );
			return FALSE;
		}
	}
	else
		pSession = AddNewSession();

	if ( !pSession ) {
		Unlock();

		XmlNodeIq iq( "error", pInfo );

		HXML errorNode = xmlAddChild( iq, "error" );
		xmlAddAttr( errorNode, "type", "cancel" );
		HXML typeNode = xmlAddChild( errorNode, "forbidden" );
		xmlAddAttr( typeNode, "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );

		m_pProto->m_ThreadInfo->send( iq );
		return FALSE;
	}

	// session id and node exits here, call handler

	int nResultCode = pNode->CallHandler( iqNode, userdata, pInfo, pSession );

	if ( nResultCode == JABBER_ADHOC_HANDLER_STATUS_COMPLETED ) {
		XmlNodeIq iq( "result", pInfo );
		HXML commandNode2 = xmlAddChild( iq, "command" );
		xmlAddAttr( commandNode2, "xmlns", JABBER_FEAT_COMMANDS );
		xmlAddAttr( commandNode2, "node", szNode );
		xmlAddAttr( commandNode2, "sessionid", pSession->GetSessionId() );
		xmlAddAttr( commandNode2, "status", "completed" );

		HXML noteNode = xmlAddChild( commandNode2, "note", "Command completed successfully" );
		xmlAddAttr( noteNode, "type", "info" );

		m_pProto->m_ThreadInfo->send( iq );

		RemoveSession( pSession );
		pSession = NULL;
	}
	else if ( nResultCode == JABBER_ADHOC_HANDLER_STATUS_CANCEL ) {
		XmlNodeIq iq( "result", pInfo );
		HXML commandNode2 = xmlAddChild( iq, "command" );
		xmlAddAttr( commandNode2, "xmlns", JABBER_FEAT_COMMANDS );
		xmlAddAttr( commandNode2, "node", szNode );
		xmlAddAttr( commandNode2, "sessionid", pSession->GetSessionId() );
		xmlAddAttr( commandNode2, "status", "canceled" );

		HXML noteNode = xmlAddChild( commandNode2, "note", "Error occured during processing command" );
		xmlAddAttr( noteNode, "type", "error" );

		m_pProto->m_ThreadInfo->send( iq );

		RemoveSession( pSession );
		pSession = NULL;
	}
	else if ( nResultCode == JABBER_ADHOC_HANDLER_STATUS_REMOVE_SESSION ) {
		RemoveSession( pSession );
		pSession = NULL;
	}
	Unlock();
	return TRUE;
}

BOOL CJabberAdhocManager::FillDefaultNodes()
{
	AddNode( NULL, _T(JABBER_FEAT_RC_SET_STATUS), _T("Set status"), &CJabberProto::AdhocSetStatusHandler );
	AddNode( NULL, _T(JABBER_FEAT_RC_SET_OPTIONS), _T("Set options"), &CJabberProto::AdhocOptionsHandler );
	AddNode( NULL, _T(JABBER_FEAT_RC_FORWARD), _T("Forward unread messages"), &CJabberProto::AdhocForwardHandler );
	AddNode( NULL, _T(JABBER_FEAT_RC_LEAVE_GROUPCHATS), _T("Leave groupchats"), &CJabberProto::AdhocLeaveGroupchatsHandler );
	AddNode( NULL, _T(JABBER_FEAT_RC_WS_LOCK), _T("Lock workstation"), &CJabberProto::AdhocLockWSHandler );
	AddNode( NULL, _T(JABBER_FEAT_RC_QUIT_MIRANDA), _T("Quit Miranda IM"), &CJabberProto::AdhocQuitMirandaHandler );
	return TRUE;
}


static char *StatusModeToDbSetting(int status,const char *suffix)
{
	char *prefix;
	static char str[64];

	switch(status) {
		case ID_STATUS_AWAY: prefix="Away";	break;
		case ID_STATUS_NA: prefix="Na";	break;
		case ID_STATUS_DND: prefix="Dnd"; break;
		case ID_STATUS_OCCUPIED: prefix="Occupied"; break;
		case ID_STATUS_FREECHAT: prefix="FreeChat"; break;
		case ID_STATUS_ONLINE: prefix="On"; break;
		case ID_STATUS_OFFLINE: prefix="Off"; break;
		case ID_STATUS_INVISIBLE: prefix="Inv"; break;
		case ID_STATUS_ONTHEPHONE: prefix="Otp"; break;
		case ID_STATUS_OUTTOLUNCH: prefix="Otl"; break;
		case ID_STATUS_IDLE: prefix="Idl"; break;
		default: return NULL;
	}
	lstrcpyA(str,prefix); lstrcatA(str,suffix);
	return str;
}

int CJabberProto::AdhocSetStatusHandler( HXML iqNode, void* usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession )
{
	if ( pSession->GetStage() == 0 ) {
		// first form
		pSession->SetStage( 1 );

		XmlNodeIq iq( "result", pInfo );
		HXML commandNode = xmlAddChild( iq, "command" );
		xmlAddAttr( commandNode, "xmlns", JABBER_FEAT_COMMANDS );
		xmlAddAttr( commandNode, "node", JABBER_FEAT_RC_SET_STATUS );
		xmlAddAttr( commandNode, "sessionid", pSession->GetSessionId() );
		xmlAddAttr( commandNode, "status", "executing" );

		HXML xNode = xmlAddChild( commandNode, "x" );
		xmlAddAttr( xNode, "xmlns", JABBER_FEAT_DATA_FORMS );
		xmlAddAttr( xNode, "type", "form" );

		xmlAddChild( xNode, "title", "Change Status" );
		xmlAddChild( xNode, "instructions", "Choose the status and status message" );

		HXML fieldNode = xmlAddChild( xNode, "field" );
		xmlAddAttr( fieldNode, "type", "hidden" );
		xmlAddAttr( fieldNode, "var", "FORM_TYPE" );
		xmlAddChild( fieldNode, "value", JABBER_FEAT_RC );

		// status
		fieldNode = xmlAddChild( xNode, "field" );
		xmlAddAttr( fieldNode, "label", "Status" );
		xmlAddAttr( fieldNode, "type", "list-single" );
		xmlAddAttr( fieldNode, "var", "status" );

		xmlAddChild( fieldNode, "required" );
		int status = JCallService( MS_CLIST_GETSTATUSMODE, 0, 0 );
		switch ( status ) {
		case ID_STATUS_INVISIBLE:
			xmlAddChild( fieldNode, "value", "invisible" );
			break;
		case ID_STATUS_AWAY:
		case ID_STATUS_ONTHEPHONE:
		case ID_STATUS_OUTTOLUNCH:
			xmlAddChild( fieldNode, "value", "away" );
			break;
		case ID_STATUS_NA:
			xmlAddChild( fieldNode, "value", "xa" );
			break;
		case ID_STATUS_DND:
		case ID_STATUS_OCCUPIED:
			xmlAddChild( fieldNode, "value", "dnd" );
			break;
		case ID_STATUS_FREECHAT:
			xmlAddChild( fieldNode, "value", "chat" );
			break;
		case ID_STATUS_ONLINE:
		default:
			xmlAddChild( fieldNode, "value", "online" );
			break;
		}

		HXML optionNode = xmlAddChild( fieldNode, "option" );
		xmlAddAttr( optionNode, "label", "Free for chat" );
		xmlAddAttr( optionNode, "value", "chat" );

		optionNode = xmlAddChild( fieldNode, "option" );
		xmlAddAttr( optionNode, "label", "Online" );
		xmlAddChild( optionNode, "value", "online" );

		optionNode = xmlAddChild( fieldNode, "option" );
		xmlAddAttr( optionNode, "label", "Away" );
		xmlAddChild( optionNode, "value", "away" );

		optionNode = xmlAddChild( fieldNode, "option" );
		xmlAddAttr( optionNode, "label", "Extended Away (N/A)" );
		xmlAddChild( optionNode, "value", "xa" );

		optionNode = xmlAddChild( fieldNode, "option" );
		xmlAddAttr( optionNode, "label", "Do Not Disturb" );
		xmlAddChild( optionNode, "value", "dnd" );

		optionNode = xmlAddChild( fieldNode, "option" );
		xmlAddAttr( optionNode, "label", "Invisible" );
		xmlAddChild( optionNode, "value", "invisible" );

		optionNode = xmlAddChild( fieldNode, "option" );
		xmlAddAttr( optionNode, "label", "Offline" );
		xmlAddChild( optionNode, "value", "offline" );

		// priority
		fieldNode = xmlAddChild( xNode, "field" );
		xmlAddAttr( fieldNode, "label", "Priority" );
		xmlAddAttr( fieldNode, "type", "text-single" );
		xmlAddAttr( fieldNode, "var", "status-priority" );
		TCHAR szPriority[ 256 ];
		mir_sntprintf( szPriority, SIZEOF(szPriority), _T("%d"), (short)JGetWord( NULL, "Priority", 5 ));
		xmlAddChild( fieldNode, "value", szPriority );

		// status message text
		fieldNode = xmlAddChild( xNode, "field" );
		xmlAddAttr( fieldNode, "label", "Status message" );
		xmlAddAttr( fieldNode, "type", "text-multi" );
		xmlAddAttr( fieldNode, "var", "status-message" );
		
		// global status
		fieldNode = xmlAddChild( xNode, "field" );
		xmlAddAttr( fieldNode, "label", "Change global status" );
		xmlAddAttr( fieldNode, "type", "boolean" );
		xmlAddAttr( fieldNode, "var", "status-global" );

		char* szStatusMsg = (char *)JCallService( MS_AWAYMSG_GETSTATUSMSG, status, 0 );
		if ( szStatusMsg ) {
			xmlAddChild( fieldNode, "value", szStatusMsg );
			mir_free( szStatusMsg );
		}

		m_ThreadInfo->send( iq );
		return JABBER_ADHOC_HANDLER_STATUS_EXECUTING;
	}
	else if ( pSession->GetStage() == 1 ) {
		// result form here
		HXML commandNode = pInfo->GetChildNode();
		HXML xNode = xmlGetChildByTag( commandNode, "x", "xmlns", _T(JABBER_FEAT_DATA_FORMS) );
		if ( !xNode )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		HXML fieldNode = xmlGetChildByTag( xNode, "field", "var", _T("status") );
		if ( !xNode )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		HXML valueNode = xmlGetChild( fieldNode , "value" );
		if ( !valueNode || !xmlGetText( valueNode ) )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		int status = 0;

		if ( !_tcscmp( xmlGetText( valueNode ), _T("away"))) status = ID_STATUS_AWAY;
			else if ( !_tcscmp( xmlGetText( valueNode ), _T("xa"))) status = ID_STATUS_NA;
			else if ( !_tcscmp( xmlGetText( valueNode ), _T("dnd"))) status = ID_STATUS_DND;
			else if ( !_tcscmp( xmlGetText( valueNode ), _T("chat"))) status = ID_STATUS_FREECHAT;
			else if ( !_tcscmp( xmlGetText( valueNode ), _T("online"))) status = ID_STATUS_ONLINE;
			else if ( !_tcscmp( xmlGetText( valueNode ), _T("invisible"))) status = ID_STATUS_INVISIBLE;
			else if ( !_tcscmp( xmlGetText( valueNode ), _T("offline"))) status = ID_STATUS_OFFLINE;

		if ( !status )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		int priority = -9999;

		fieldNode = xmlGetChildByTag( xNode, "field", "var", _T("status-priority") );
		if ( fieldNode && (valueNode = xmlGetChild( fieldNode , "value" ))) {
			if ( xmlGetText( valueNode ) )
				priority = _ttoi( xmlGetText( valueNode ) );
		}

		if ( priority >= -128 && priority <= 127 )
			JSetWord( NULL, "Priority", (WORD)priority );

		char* szStatusMessage = NULL;
		fieldNode = xmlGetChildByTag( xNode, "field", "var", _T("status-message") );
		if ( fieldNode && (valueNode = xmlGetChild( fieldNode , "value" ))) {
			if ( xmlGetText( valueNode ) )
				szStatusMessage = mir_t2a(xmlGetText( valueNode ));
		}

		// skip f...ng away dialog
		int nNoDlg = DBGetContactSettingByte( NULL, "SRAway", StatusModeToDbSetting( status, "NoDlg" ), 0 );
		DBWriteContactSettingByte( NULL, "SRAway", StatusModeToDbSetting( status, "NoDlg" ), 1 );

		DBWriteContactSettingString( NULL, "SRAway", StatusModeToDbSetting( status, "Msg" ), szStatusMessage ? szStatusMessage : "" );
		
		fieldNode = xmlGetChildByTag( xNode, "field", "var", _T("status-global") );
		if ( fieldNode && (valueNode = xmlGetChild( fieldNode , "value" ))) {
			if ( xmlGetText( valueNode ) && _ttoi( xmlGetText( valueNode ) ))
				JCallService( MS_CLIST_SETSTATUSMODE, status, NULL );
			else
				CallProtoService( m_szModuleName, PS_SETSTATUS, status, NULL );
		}
		SetAwayMsg( status, szStatusMessage );

		// return NoDlg setting
		DBWriteContactSettingByte( NULL, "SRAway", StatusModeToDbSetting( status, "NoDlg" ), (BYTE)nNoDlg );

		if ( szStatusMessage )
			mir_free( szStatusMessage );

		return JABBER_ADHOC_HANDLER_STATUS_COMPLETED;
	}
	return JABBER_ADHOC_HANDLER_STATUS_CANCEL;
}

int CJabberProto::AdhocOptionsHandler( HXML iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession )
{
	if ( pSession->GetStage() == 0 ) {
		// first form
		pSession->SetStage( 1 );

		XmlNodeIq iq( "result", pInfo );
		HXML commandNode = xmlAddChild( iq, "command" );
		xmlAddAttr( commandNode, "xmlns", JABBER_FEAT_COMMANDS );
		xmlAddAttr( commandNode, "node", JABBER_FEAT_RC_SET_OPTIONS );
		xmlAddAttr( commandNode, "sessionid", pSession->GetSessionId() );
		xmlAddAttr( commandNode, "status", "executing" );

		HXML xNode = xmlAddChild( commandNode, "x" );
		xmlAddAttr( xNode, "xmlns", JABBER_FEAT_DATA_FORMS );
		xmlAddAttr( xNode, "type", "form" );

		xmlAddChild( xNode, "title", "Set Options" );
		xmlAddChild( xNode, "instructions", "Set the desired options" );

		TCHAR szTmpBuff[ 1024 ];
		HXML fieldNode = xmlAddChild( xNode, "field" );
		xmlAddAttr( fieldNode, "type", "hidden" );
		xmlAddAttr( fieldNode, "var", "FORM_TYPE" );
		xmlAddChild( fieldNode, "value", JABBER_FEAT_RC );

		// Automatically Accept File Transfers
		fieldNode = xmlAddChild( xNode, "field" );
		xmlAddAttr( fieldNode, "label", "Automatically Accept File Transfers" );
		xmlAddAttr( fieldNode, "type", "boolean" );
		xmlAddAttr( fieldNode, "var", "auto-files" );
		mir_sntprintf( szTmpBuff, SIZEOF(szTmpBuff), _T("%d"), DBGetContactSettingByte( NULL, "SRFile", "AutoAccept", 0 ));
		xmlAddChild( fieldNode, "value", szTmpBuff );

		// Use sounds
		fieldNode = xmlAddChild( xNode, "field" );
		xmlAddAttr( fieldNode, "label", "Play sounds" );
		xmlAddAttr( fieldNode, "type", "boolean" );
		xmlAddAttr( fieldNode, "var", "sounds" );
		mir_sntprintf( szTmpBuff, SIZEOF(szTmpBuff), _T("%d"), DBGetContactSettingByte( NULL, "Skin", "UseSound", 0 ));
		xmlAddChild( fieldNode, "value", szTmpBuff );

		// Disable remote controlling
		fieldNode = xmlAddChild( xNode, "field" );
		xmlAddAttr( fieldNode, "label", "Disable remote controlling (check twice what you are doing)" );
		xmlAddAttr( fieldNode, "type", "boolean" );
		xmlAddAttr( fieldNode, "var", "enable-rc" );
		xmlAddChild( fieldNode, "value", "0" );

		m_ThreadInfo->send( iq );
		return JABBER_ADHOC_HANDLER_STATUS_EXECUTING;
	}
	else if ( pSession->GetStage() == 1 ) {
		// result form here
		HXML commandNode = pInfo->GetChildNode();
		HXML xNode = xmlGetChildByTag( commandNode, "x", "xmlns", _T(JABBER_FEAT_DATA_FORMS) );
		if ( !xNode )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		HXML fieldNode, valueNode;

		// Automatically Accept File Transfers
		fieldNode = xmlGetChildByTag( xNode, "field", "var", _T("auto-files") );
		if ( fieldNode && (valueNode = xmlGetChild( fieldNode , "value" ))) {
			if ( xmlGetText( valueNode ) )
				DBWriteContactSettingByte( NULL, "SRFile", "AutoAccept", (BYTE)_ttoi( xmlGetText( valueNode ) ) );
		}

		// Use sounds
		fieldNode = xmlGetChildByTag( xNode, "field", "var", _T("sounds") );
		if ( fieldNode && (valueNode = xmlGetChild( fieldNode , "value" ))) {
			if ( xmlGetText( valueNode ) )
				DBWriteContactSettingByte( NULL, "Skin", "UseSound", (BYTE)_ttoi( xmlGetText( valueNode ) ) );
		}

		// Disable remote controlling
		fieldNode = xmlGetChildByTag( xNode, "field", "var", _T("enable-rc") );
		if ( fieldNode && (valueNode = xmlGetChild( fieldNode , "value" ))) {
			if ( xmlGetText( valueNode ) && _ttoi( xmlGetText( valueNode ) ))
				JSetByte( "EnableRemoteControl", 0 );
		}

		return JABBER_ADHOC_HANDLER_STATUS_COMPLETED;
	}
	return JABBER_ADHOC_HANDLER_STATUS_CANCEL;
}

int CJabberProto::RcGetUnreadEventsCount()
{
	int nEventsSent = 0;
	HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) {
		char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
		if ( szProto != NULL && !strcmp( szProto, m_szModuleName )) {
			DBVARIANT dbv;
			if ( !JGetStringT( hContact, "jid", &dbv )) {
				HANDLE hDbEvent = (HANDLE)JCallService( MS_DB_EVENT_FINDFIRSTUNREAD, (WPARAM)hContact, 0 );
				while ( hDbEvent ) {
					DBEVENTINFO dbei = { 0 };
					dbei.cbSize = sizeof(dbei);
					dbei.cbBlob = CallService( MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hDbEvent, 0 );
					if ( dbei.cbBlob != -1 ) {
						dbei.pBlob = (PBYTE)mir_alloc( dbei.cbBlob + 1 );
						int nGetTextResult = JCallService( MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei );
						if ( !nGetTextResult && dbei.eventType == EVENTTYPE_MESSAGE && !(dbei.flags & DBEF_READ) && !(dbei.flags & DBEF_SENT)) {
							TCHAR* szEventText = DbGetEventTextT( &dbei, CP_ACP );
							if ( szEventText ) {
								nEventsSent++;
								mir_free( szEventText );
							}
						}
						mir_free( dbei.pBlob );
					}
					hDbEvent = (HANDLE)JCallService( MS_DB_EVENT_FINDNEXT, (WPARAM)hDbEvent, 0 );
				}
				JFreeVariant( &dbv );
			}
		}
		hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
	}
	return nEventsSent;
}

int CJabberProto::AdhocForwardHandler( HXML iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession )
{
	TCHAR szMsg[ 1024 ];
	if ( pSession->GetStage() == 0 ) {
		int nUnreadEvents = RcGetUnreadEventsCount();
		if ( !nUnreadEvents ) {
			XmlNodeIq iq( "result", pInfo );
			HXML commandNode = xmlAddChild( iq, "command" );
			xmlAddAttr( commandNode, "xmlns", JABBER_FEAT_COMMANDS );
			xmlAddAttr( commandNode, "node", JABBER_FEAT_RC_FORWARD );
			xmlAddAttr( commandNode, "sessionid", pSession->GetSessionId() );
			xmlAddAttr( commandNode, "status", "completed" );

			mir_sntprintf( szMsg, SIZEOF(szMsg), _T("There is no messages to forward") );
			HXML noteNode = xmlAddChild( commandNode, "note", szMsg );
			xmlAddAttr( noteNode, "type", "info" );

			m_ThreadInfo->send( iq );

			return JABBER_ADHOC_HANDLER_STATUS_REMOVE_SESSION;
		}

		// first form
		pSession->SetStage( 1 );

		XmlNodeIq iq( "result", pInfo );
		HXML commandNode = xmlAddChild( iq, "command" );
		xmlAddAttr( commandNode, "xmlns", JABBER_FEAT_COMMANDS );
		xmlAddAttr( commandNode, "node", JABBER_FEAT_RC_FORWARD );
		xmlAddAttr( commandNode, "sessionid", pSession->GetSessionId() );
		xmlAddAttr( commandNode, "status", "executing" );

		HXML xNode = xmlAddChild( commandNode, "x" );
		xmlAddAttr( xNode, "xmlns", JABBER_FEAT_DATA_FORMS );
		xmlAddAttr( xNode, "type", "form" );

		xmlAddChild( xNode, "title", "Forward options" );
		mir_sntprintf( szMsg, SIZEOF(szMsg), _T("%d message(s) to be forwarded"), nUnreadEvents );
		xmlAddChild( xNode, "instructions", szMsg );

		HXML fieldNode = xmlAddChild( xNode, "field" );
		xmlAddAttr( fieldNode, "type", "hidden" );
		xmlAddAttr( fieldNode, "var", "FORM_TYPE" );
		xmlAddChild( fieldNode, "value", JABBER_FEAT_RC );

		// remove clist events
		fieldNode = xmlAddChild( xNode, "field" );
		xmlAddAttr( fieldNode, "label", "Mark messages as read" );
		xmlAddAttr( fieldNode, "type", "boolean" );
		xmlAddAttr( fieldNode, "var", "remove-clist-events" );
		xmlAddChild( fieldNode, "value", "1" );

		m_ThreadInfo->send( iq );
		return JABBER_ADHOC_HANDLER_STATUS_EXECUTING;
	}
	else if ( pSession->GetStage() == 1 ) {
		// result form here
		HXML commandNode = pInfo->GetChildNode();
		HXML xNode = xmlGetChildByTag( commandNode, "x", "xmlns", _T(JABBER_FEAT_DATA_FORMS) );
		if ( !xNode )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		HXML fieldNode, valueNode;

		BOOL bRemoveCListEvents = TRUE;

		// remove clist events
		fieldNode = xmlGetChildByTag( xNode,"field", "var", _T("remove-clist-events") );
		if ( fieldNode && (valueNode = xmlGetChild( fieldNode , "value" ))) {
			if ( xmlGetText( valueNode ) && !_ttoi( xmlGetText( valueNode ) ) ) {
				bRemoveCListEvents = FALSE;
			}
		}

		int nEventsSent = 0;
		HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		while ( hContact != NULL ) {
			char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
			if ( szProto != NULL && !strcmp( szProto, m_szModuleName )) {
				DBVARIANT dbv;
				if ( !JGetStringT( hContact, "jid", &dbv )) {
					
					HANDLE hDbEvent = (HANDLE)JCallService( MS_DB_EVENT_FINDFIRSTUNREAD, (WPARAM)hContact, 0 );
					while ( hDbEvent ) {
						DBEVENTINFO dbei = { 0 };
						dbei.cbSize = sizeof(dbei);
						dbei.cbBlob = CallService( MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hDbEvent, 0 );
						if ( dbei.cbBlob != -1 ) {
							dbei.pBlob = (PBYTE)mir_alloc( dbei.cbBlob + 1 );
							int nGetTextResult = JCallService( MS_DB_EVENT_GET, (WPARAM)hDbEvent, (LPARAM)&dbei );
							if ( !nGetTextResult && dbei.eventType == EVENTTYPE_MESSAGE && !(dbei.flags & DBEF_READ) && !(dbei.flags & DBEF_SENT)) {
								TCHAR* szEventText = DbGetEventTextT( &dbei, CP_ACP );
								if ( szEventText ) {
									XmlNode msg( _T("message"));
									xmlAddAttr( msg, "to", pInfo->GetFrom() );
									xmlAddAttrID( msg, SerialNext() );

									HXML bodyNode = xmlAddChild( msg, "body", szEventText );
									HXML addressesNode = xmlAddChild( msg, "addresses" );
									xmlAddAttr( addressesNode, "xmlns", JABBER_FEAT_EXT_ADDRESSING );

									HXML addressNode = xmlAddChild( addressesNode, "address" );
									xmlAddAttr( addressNode, "type", "ofrom" );
									xmlAddAttr( addressNode, "jid", dbv.ptszVal );

									addressNode = xmlAddChild( addressesNode, "address" );
									xmlAddAttr( addressNode, "type", "oto" );
									xmlAddAttr( addressNode, "jid", m_ThreadInfo->fullJID );

									time_t ltime = ( time_t )dbei.timestamp;
									struct tm *gmt = gmtime( &ltime );
									char stime[ 512 ];
									sprintf(stime, "%.4i-%.2i-%.2iT%.2i:%.2i:%.2iZ", gmt->tm_year + 1900, gmt->tm_mon + 1, gmt->tm_mday,
										gmt->tm_hour, gmt->tm_min, gmt->tm_sec);

									HXML delayNode = xmlAddChild( msg, "delay" );
									xmlAddAttr( delayNode, "xmlns", "urn:xmpp:delay" );
									xmlAddAttr( delayNode, "stamp", stime );

									m_ThreadInfo->send( msg );

									nEventsSent++;

									JCallService( MS_DB_EVENT_MARKREAD, (WPARAM)hContact, (LPARAM)hDbEvent );
									if ( bRemoveCListEvents )
										JCallService( MS_CLIST_REMOVEEVENT, (WPARAM)hContact, (LPARAM)hDbEvent );

									mir_free( szEventText );
								}
							}
							mir_free( dbei.pBlob );
						}
						hDbEvent = (HANDLE)JCallService( MS_DB_EVENT_FINDNEXT, (WPARAM)hDbEvent, 0 );
					}
					JFreeVariant( &dbv );
				}
			}
			hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
		}

		XmlNodeIq iq( "result", pInfo );
		commandNode = xmlAddChild( iq, "command" );
		xmlAddAttr( commandNode, "xmlns", JABBER_FEAT_COMMANDS );
		xmlAddAttr( commandNode, "node", JABBER_FEAT_RC_FORWARD );
		xmlAddAttr( commandNode, "sessionid", pSession->GetSessionId() );
		xmlAddAttr( commandNode, "status", "completed" );

		mir_sntprintf( szMsg, SIZEOF(szMsg), _T("%d message(s) forwarded"), nEventsSent );
		HXML noteNode = xmlAddChild( commandNode, "note", szMsg );
		xmlAddAttr( noteNode, "type", "info" );

		m_ThreadInfo->send( iq );

		return JABBER_ADHOC_HANDLER_STATUS_REMOVE_SESSION;
	}

	return JABBER_ADHOC_HANDLER_STATUS_CANCEL;
}

typedef BOOL (WINAPI *LWS )( VOID );

int CJabberProto::AdhocLockWSHandler( HXML iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession )
{
	BOOL bOk = FALSE;
	HMODULE hLibrary = LoadLibrary( _T("user32.dll") );
	if ( hLibrary ) {
		LWS pLws = GetProcAddress( hLibrary, "LockWorkStation" );
		if ( pLws )
			bOk = pLws();
		FreeLibrary( hLibrary );
	}

	XmlNodeIq iq( "result", pInfo );
	HXML commandNode = xmlAddChild( iq, "command" );
	xmlAddAttr( commandNode, "xmlns", JABBER_FEAT_COMMANDS );
	xmlAddAttr( commandNode, "node", JABBER_FEAT_RC_WS_LOCK );
	xmlAddAttr( commandNode, "sessionid", pSession->GetSessionId() );
	xmlAddAttr( commandNode, "status", "completed" );

	TCHAR szMsg[ 1024 ];
	if ( bOk )
		mir_sntprintf( szMsg, SIZEOF(szMsg), _T("Workstation successfully locked") );
	else
		mir_sntprintf( szMsg, SIZEOF(szMsg), _T("Error %d occured during workstation lock"), GetLastError() );

	HXML noteNode = xmlAddChild( commandNode, "note", szMsg );
	xmlAddAttr( noteNode, "type", bOk ? "info" : "error" );

	m_ThreadInfo->send( iq );

	return JABBER_ADHOC_HANDLER_STATUS_REMOVE_SESSION;
}

static void __cdecl JabberQuitMirandaIMThread( void* pParam )
{
	SleepEx( 2000, TRUE );
	JCallService( "CloseAction", 0, 0 );
}

int CJabberProto::AdhocQuitMirandaHandler( HXML iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession )
{
	if ( pSession->GetStage() == 0 ) {
		// first form
		pSession->SetStage( 1 );

		XmlNodeIq iq( "result", pInfo );
		HXML commandNode = xmlAddChild( iq, "command" );
		xmlAddAttr( commandNode, "xmlns", JABBER_FEAT_COMMANDS );
		xmlAddAttr( commandNode, "node", JABBER_FEAT_RC_QUIT_MIRANDA );
		xmlAddAttr( commandNode, "sessionid", pSession->GetSessionId() );
		xmlAddAttr( commandNode, "status", "executing" );

		HXML xNode = xmlAddChild( commandNode, "x" );
		xmlAddAttr( xNode, "xmlns", JABBER_FEAT_DATA_FORMS );
		xmlAddAttr( xNode, "type", "form" );

		xmlAddChild( xNode, "title", "Confirmation needed" );
		xmlAddChild( xNode, "instructions", "Please confirm Miranda IM shutdown" );

		HXML fieldNode = xmlAddChild( xNode, "field" );
		xmlAddAttr( fieldNode, "type", "hidden" );
		xmlAddAttr( fieldNode, "var", "FORM_TYPE" );
		xmlAddChild( fieldNode, "value", JABBER_FEAT_RC );

		// I Agree checkbox
		fieldNode = xmlAddChild( xNode, "field" );
		xmlAddAttr( fieldNode, "label", "I agree" );
		xmlAddAttr( fieldNode, "type", "boolean" );
		xmlAddAttr( fieldNode, "var", "allow-shutdown" );
		xmlAddChild( fieldNode, "value", "0" );

		m_ThreadInfo->send( iq );
		return JABBER_ADHOC_HANDLER_STATUS_EXECUTING;
	}
	else if ( pSession->GetStage() == 1 ) {
		// result form here
		HXML commandNode = pInfo->GetChildNode();
		HXML xNode = xmlGetChildByTag( commandNode, "x", "xmlns", _T(JABBER_FEAT_DATA_FORMS) );
		if ( !xNode )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		HXML fieldNode, valueNode;

		// I Agree checkbox
		fieldNode = xmlGetChildByTag( xNode,"field", "var", _T("allow-shutdown") );
		if ( fieldNode && (valueNode = xmlGetChild( fieldNode , "value" )))
			if ( xmlGetText( valueNode ) && _ttoi( xmlGetText( valueNode ) ))
				mir_forkthread( JabberQuitMirandaIMThread, NULL );

		return JABBER_ADHOC_HANDLER_STATUS_COMPLETED;
	}
	return JABBER_ADHOC_HANDLER_STATUS_CANCEL;
}

int CJabberProto::AdhocLeaveGroupchatsHandler( HXML iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession )
{
	int i = 0;
	if ( pSession->GetStage() == 0 ) {
		// first form
		TCHAR szMsg[ 1024 ];

		ListLock();
		int nChatsCount = 0;
		for ( i = 0; ( i=ListFindNext( LIST_CHATROOM, i )) >= 0; i++ ) {
			JABBER_LIST_ITEM *item = ListGetItemPtrFromIndex( i );
			if ( item != NULL )
				nChatsCount++;
		}
		ListUnlock();

		if ( !nChatsCount ) {
			XmlNodeIq iq( "result", pInfo );
			HXML commandNode = xmlAddChild( iq, "command" );
			xmlAddAttr( commandNode, "xmlns", JABBER_FEAT_COMMANDS );
			xmlAddAttr( commandNode, "node", JABBER_FEAT_RC_LEAVE_GROUPCHATS );
			xmlAddAttr( commandNode, "sessionid", pSession->GetSessionId() );
			xmlAddAttr( commandNode, "status", "completed" );

			mir_sntprintf( szMsg, SIZEOF(szMsg), _T("There is no groupchats to leave") );
			HXML noteNode = xmlAddChild( commandNode, "note", szMsg );
			xmlAddAttr( noteNode, "type", "info" );

			m_ThreadInfo->send( iq );

			return JABBER_ADHOC_HANDLER_STATUS_REMOVE_SESSION;
		}

		pSession->SetStage( 1 );

		XmlNodeIq iq( "result", pInfo );
		HXML commandNode = xmlAddChild( iq, "command" );
		xmlAddAttr( commandNode, "xmlns", JABBER_FEAT_COMMANDS );
		xmlAddAttr( commandNode, "node", JABBER_FEAT_RC_LEAVE_GROUPCHATS );
		xmlAddAttr( commandNode, "sessionid", pSession->GetSessionId() );
		xmlAddAttr( commandNode, "status", "executing" );

		HXML xNode = xmlAddChild( commandNode, "x" );
		xmlAddAttr( xNode, "xmlns", JABBER_FEAT_DATA_FORMS );
		xmlAddAttr( xNode, "type", "form" );

		xmlAddChild( xNode, "title", "Leave groupchats" );
		xmlAddChild( xNode, "instructions", "Choose the groupchats you want to leave" );

		HXML fieldNode = xmlAddChild( xNode, "field" );
		xmlAddAttr( fieldNode, "type", "hidden" );
		xmlAddAttr( fieldNode, "var", "FORM_TYPE" );
		xmlAddAttr( fieldNode, "value", JABBER_FEAT_RC );

		// Groupchats
		fieldNode = xmlAddChild( xNode, "field" );
		xmlAddAttr( fieldNode, "label", "" );
		xmlAddAttr( fieldNode, "type", "list-multi" );
		xmlAddAttr( fieldNode, "var", "groupchats" );
		xmlAddChild( fieldNode, "required" );

		ListLock();
		for ( i = 0; ( i=ListFindNext( LIST_CHATROOM, i )) >= 0; i++ ) {
			JABBER_LIST_ITEM *item = ListGetItemPtrFromIndex( i );
			if ( item != NULL ) {
				HXML optionNode = xmlAddChild( fieldNode, "option" );
				xmlAddAttr( optionNode, "label", item->jid );
				xmlAddChild( optionNode, "value", item->jid );
			}
		}
		ListUnlock();

		m_ThreadInfo->send( iq );
		return JABBER_ADHOC_HANDLER_STATUS_EXECUTING;
	}
	else if ( pSession->GetStage() == 1 ) {
		// result form here
		HXML commandNode = pInfo->GetChildNode();
		HXML xNode = xmlGetChildByTag( commandNode, "x", "xmlns", _T(JABBER_FEAT_DATA_FORMS) );
		if ( !xNode )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		// Groupchat list here:
		HXML fieldNode = xmlGetChildByTag( xNode,"field", "var", _T("groupchats") );
		if ( fieldNode ) {
			for ( i = 0; ; i++ ) {
				HXML valueNode = xmlGetChild( fieldNode ,i);
				if ( valueNode && xmlGetName( valueNode ) && xmlGetText( valueNode ) && !_tcscmp( xmlGetName( valueNode ), _T("value"))) {
					JABBER_LIST_ITEM* item = ListGetItemPtr( LIST_CHATROOM, xmlGetText( valueNode ) );
					if ( item )
						GcQuit( item, 0, NULL );
				}
			}
		}

		return JABBER_ADHOC_HANDLER_STATUS_COMPLETED;
	}
	return JABBER_ADHOC_HANDLER_STATUS_CANCEL;
}