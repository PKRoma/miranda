/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-07  George Hazan
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

CJabberAdhocManager g_JabberAdhocManager;

BOOL IsRcRequestAllowedByACL( CJabberIqInfo* pInfo )
{
	if ( !pInfo || !pInfo->GetFrom() )
		return FALSE;

	TCHAR* szFrom = JabberPrepareJid( pInfo->GetFrom() );
	if ( !szFrom )
		return FALSE;

	TCHAR* szTo = JabberPrepareJid( jabberThreadInfo->fullJID );
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

void JabberHandleAdhocCommandRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo )
{
	if ( !pInfo->GetChildNode() )
		return;

	if ( !JGetByte( "EnableRemoteControl", FALSE ) || !IsRcRequestAllowedByACL( pInfo )) {
		// FIXME: send error and return
		return;
	}

	TCHAR* szNode = JabberXmlGetAttrValue( pInfo->GetChildNode(), "node" );
	if ( !szNode )
		return;

	g_JabberAdhocManager.HandleCommandRequest( iqNode, userdata, pInfo, szNode );
}

BOOL CJabberAdhocManager::HandleItemsRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo, TCHAR* szNode )
{
	if ( !szNode || !JGetByte( "EnableRemoteControl", FALSE ) || !IsRcRequestAllowedByACL( pInfo ))
		return FALSE;

	if ( !_tcscmp( szNode, _T(JABBER_FEAT_COMMANDS)))
	{
		XmlNodeIq iq( "result", pInfo );
		XmlNode* resultQuery = iq.addChild( "query" );
		resultQuery->addAttr( "xmlns", _T(JABBER_FEAT_DISCO_ITEMS) );
		resultQuery->addAttr( "node", _T(JABBER_FEAT_COMMANDS) );

		Lock();
		CJabberAdhocNode* pNode = GetFirstNode();
		while ( pNode ) {
			XmlNode* item = resultQuery->addChild( "item" );
			TCHAR* szJid = pNode->GetJid();
			if ( !szJid )
				szJid = jabberThreadInfo->fullJID;
			item->addAttr( "jid", szJid );
			item->addAttr( "node", pNode->GetNode() );
			item->addAttr( "name", pNode->GetName() );

			pNode = pNode->GetNext();
		}
		Unlock();

		jabberThreadInfo->send( iq );
		return TRUE;
	}
	return FALSE;
}

BOOL CJabberAdhocManager::HandleInfoRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo, TCHAR* szNode )
{
	if ( !szNode || !JGetByte( "EnableRemoteControl", FALSE ) || !IsRcRequestAllowedByACL( pInfo ))
		return FALSE;

	// FIXME: same code twice
	if ( !_tcscmp( szNode, _T(JABBER_FEAT_COMMANDS))) {
		XmlNodeIq iq( "result", pInfo );
		XmlNode* resultQuery = iq.addChild( "query" );
		resultQuery->addAttr( "xmlns", _T(JABBER_FEAT_DISCO_INFO) );
		resultQuery->addAttr( "node", _T(JABBER_FEAT_COMMANDS) );

		XmlNode* identity = resultQuery->addChild( "identity" );
		identity->addAttr( "name", "Ad-hoc commands" );
		identity->addAttr( "category", "automation" );
		identity->addAttr( "type", "command-node" );

		XmlNode* feature = resultQuery->addChild( "feature" );
		feature->addAttr( "var", _T(JABBER_FEAT_COMMANDS) );

		feature = resultQuery->addChild( "feature" );
		feature->addAttr( "var", _T(JABBER_FEAT_DATA_FORMS) );

		feature = resultQuery->addChild( "feature" );
		feature->addAttr( "var", _T(JABBER_FEAT_DISCO_INFO) );

		feature = resultQuery->addChild( "feature" );
		feature->addAttr( "var", _T(JABBER_FEAT_DISCO_ITEMS) );

		jabberThreadInfo->send( iq );
		return TRUE;
	}

	Lock();
	CJabberAdhocNode *pNode = FindNode( szNode );
	if ( pNode ) {
		XmlNodeIq iq( "result", pInfo );
		XmlNode* resultQuery = iq.addChild( "query" );
		resultQuery->addAttr( "xmlns", _T(JABBER_FEAT_DISCO_INFO) );
		resultQuery->addAttr( "node", szNode );

		XmlNode* identity = resultQuery->addChild( "identity" );
		identity->addAttr( "name", pNode->GetName() );
		identity->addAttr( "category", "automation" );
		identity->addAttr( "type", "command-node" );

		XmlNode* feature = resultQuery->addChild( "feature" );
		feature->addAttr( "var", _T(JABBER_FEAT_COMMANDS) );

		feature = resultQuery->addChild( "feature" );
		feature->addAttr( "var", _T(JABBER_FEAT_DATA_FORMS) );

		feature = resultQuery->addChild( "feature" );
		feature->addAttr( "var", _T(JABBER_FEAT_DISCO_INFO) );

		Unlock();
		jabberThreadInfo->send( iq );
		return TRUE;
	}
	Unlock();
	return FALSE;
}

BOOL CJabberAdhocManager::HandleCommandRequest( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo, TCHAR* szNode )
{
	// ATTN: ACL and db settings checked in calling function

	XmlNode* commandNode = pInfo->GetChildNode();

	Lock();
	CJabberAdhocNode* pNode = FindNode( szNode );
	if ( !pNode ) {
		Unlock();

		XmlNodeIq iq( "error", pInfo );

		XmlNode *errorNode = iq.addChild( "error" );
		errorNode->addAttr( "type", "cancel" );
		XmlNode *typeNode = errorNode->addChild( "item-not-found" );
		typeNode->addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );

		jabberThreadInfo->send( iq );
		return FALSE;
	}

	TCHAR* szSessionId = JabberXmlGetAttrValue( commandNode, "sessionid" );

	CJabberAdhocSession* pSession = NULL;
	if ( szSessionId ) {
		pSession = FindSession( szSessionId );
		if ( !pSession ) {
			Unlock();

			XmlNodeIq iq( "error", pInfo );

			XmlNode *errorNode = iq.addChild( "error" );
			errorNode->addAttr( "type", "modify" );
			XmlNode *typeNode = errorNode->addChild( "bad-request" );
			typeNode->addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );

			typeNode = errorNode->addChild( "bad-sessionid" );
			typeNode->addAttr( "xmlns", JABBER_FEAT_COMMANDS );

			jabberThreadInfo->send( iq );
			return FALSE;
		}
	}
	else
		pSession = AddNewSession();

	if ( !pSession ) {
		Unlock();

		XmlNodeIq iq( "error", pInfo );

		XmlNode *errorNode = iq.addChild( "error" );
		errorNode->addAttr( "type", "cancel" );
		XmlNode *typeNode = errorNode->addChild( "forbidden" );
		typeNode->addAttr( "xmlns", "urn:ietf:params:xml:ns:xmpp-stanzas" );

		jabberThreadInfo->send( iq );
		return FALSE;
	}

	// session id and node exits here, call handler

	int nResultCode = pNode->CallHandler( iqNode, userdata, pInfo, pSession );

	if ( nResultCode == JABBER_ADHOC_HANDLER_STATUS_COMPLETED ) {
		XmlNodeIq iq( "result", pInfo );
		XmlNode* commandNode2 = iq.addChild( "command" );
		commandNode2->addAttr( "xmlns", JABBER_FEAT_COMMANDS );
		commandNode2->addAttr( "node", szNode );
		commandNode2->addAttr( "sessionid", pSession->GetSessionId() );
		commandNode2->addAttr( "status", "completed" );

		XmlNode* noteNode = commandNode2->addChild( "note", "Command completed successfully" );
		noteNode->addAttr( "type", "info" );

		jabberThreadInfo->send( iq );

		RemoveSession( pSession );
		pSession = NULL;
	}
	else if ( nResultCode == JABBER_ADHOC_HANDLER_STATUS_CANCEL ) {
		XmlNodeIq iq( "result", pInfo );
		XmlNode* commandNode2 = iq.addChild( "command" );
		commandNode2->addAttr( "xmlns", JABBER_FEAT_COMMANDS );
		commandNode2->addAttr( "node", szNode );
		commandNode2->addAttr( "sessionid", pSession->GetSessionId() );
		commandNode2->addAttr( "status", "canceled" );

		XmlNode* noteNode = commandNode2->addChild( "note", "Error occured during processing command" );
		noteNode->addAttr( "type", "error" );

		jabberThreadInfo->send( iq );

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

int JabberSetAwayMsg( WPARAM wParam, LPARAM lParam );

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

int JabberAdhocSetStatusHandler( XmlNode* iqNode, void* usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession )
{
	if ( pSession->GetStage() == 0 ) {
		// first form
		pSession->SetStage( 1 );

		XmlNodeIq iq( "result", pInfo );
		XmlNode* commandNode = iq.addChild( "command" );
		commandNode->addAttr( "xmlns", JABBER_FEAT_COMMANDS );
		commandNode->addAttr( "node", JABBER_FEAT_RC_SET_STATUS );
		commandNode->addAttr( "sessionid", pSession->GetSessionId() );
		commandNode->addAttr( "status", "executing" );

		XmlNode* xNode = commandNode->addChild( "x" );
		xNode->addAttr( "xmlns", JABBER_FEAT_DATA_FORMS );
		xNode->addAttr( "type", "form" );

		xNode->addChild( "title", "Change Status" );
		xNode->addChild( "instructions", "Choose the status and status message" );

		XmlNode* fieldNode = NULL;

		fieldNode = xNode->addChild( "field" );
		fieldNode->addAttr( "type", "hidden" );
		fieldNode->addAttr( "var", "FORM_TYPE" );
		fieldNode->addChild( "value", JABBER_FEAT_RC );

		// status
		fieldNode = xNode->addChild( "field" );
		fieldNode->addAttr( "label", "Status" );
		fieldNode->addAttr( "type", "list-single" );
		fieldNode->addAttr( "var", "status" );

		fieldNode->addChild( "required" );
		int status = JCallService( MS_CLIST_GETSTATUSMODE, 0, 0 );
		switch ( status ) {
		case ID_STATUS_INVISIBLE:
			fieldNode->addChild( "value", "invisible" );
			break;
		case ID_STATUS_AWAY:
		case ID_STATUS_ONTHEPHONE:
		case ID_STATUS_OUTTOLUNCH:
			fieldNode->addChild( "value", "away" );
			break;
		case ID_STATUS_NA:
			fieldNode->addChild( "value", "xa" );
			break;
		case ID_STATUS_DND:
		case ID_STATUS_OCCUPIED:
			fieldNode->addChild( "value", "dnd" );
			break;
		case ID_STATUS_FREECHAT:
			fieldNode->addChild( "value", "chat" );
			break;
		case ID_STATUS_ONLINE:
		default:
			fieldNode->addChild( "value", "online" );
			break;
		}

		XmlNode* optionNode = NULL;
		optionNode = fieldNode->addChild( "option" );
		optionNode->addAttr( "label", "Free for chat" );
		optionNode->addChild( "value", "chat" );

		optionNode = fieldNode->addChild( "option" );
		optionNode->addAttr( "label", "Online" );
		optionNode->addChild( "value", "online" );

		optionNode = fieldNode->addChild( "option" );
		optionNode->addAttr( "label", "Away" );
		optionNode->addChild( "value", "away" );

		optionNode = fieldNode->addChild( "option" );
		optionNode->addAttr( "label", "Extended Away (N/A)" );
		optionNode->addChild( "value", "xa" );

		optionNode = fieldNode->addChild( "option" );
		optionNode->addAttr( "label", "Do Not Disturb" );
		optionNode->addChild( "value", "dnd" );

		optionNode = fieldNode->addChild( "option" );
		optionNode->addAttr( "label", "Invisible" );
		optionNode->addChild( "value", "invisible" );

		optionNode = fieldNode->addChild( "option" );
		optionNode->addAttr( "label", "Offline" );
		optionNode->addChild( "value", "offline" );

		// priority
		fieldNode = xNode->addChild( "field" );
		fieldNode->addAttr( "label", "Priority" );
		fieldNode->addAttr( "type", "text-single" );
		fieldNode->addAttr( "var", "status-priority" );
		TCHAR szPriority[ 256 ];
		mir_sntprintf( szPriority, SIZEOF(szPriority), _T("%d"), (short)JGetWord( NULL, "Priority", 5 ));
		fieldNode->addChild( "value", szPriority );

		// status message text
		fieldNode = xNode->addChild( "field" );
		fieldNode->addAttr( "label", "Status message" );
		fieldNode->addAttr( "type", "text-multi" );
		fieldNode->addAttr( "var", "status-message" );
		
		// global status
		fieldNode = xNode->addChild( "field" );
		fieldNode->addAttr( "label", "Change global status" );
		fieldNode->addAttr( "type", "boolean" );
		fieldNode->addAttr( "var", "status-global" );

		char* szStatusMsg = (char *)JCallService( MS_AWAYMSG_GETSTATUSMSG, status, 0 );
		if ( szStatusMsg ) {
			fieldNode->addChild( "value", szStatusMsg );
			mir_free( szStatusMsg );
		}

		jabberThreadInfo->send( iq );
		return JABBER_ADHOC_HANDLER_STATUS_EXECUTING;
	}
	else if ( pSession->GetStage() == 1 ) {
		// result form here
		XmlNode* commandNode = pInfo->GetChildNode();
		XmlNode* xNode = JabberXmlGetChildWithGivenAttrValue( commandNode, "x", "xmlns", _T(JABBER_FEAT_DATA_FORMS) );
		if ( !xNode )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		XmlNode* fieldNode = JabberXmlGetChildWithGivenAttrValue( xNode, "field", "var", _T("status") );
		if ( !xNode )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		XmlNode* valueNode = JabberXmlGetChild( fieldNode, "value" );
		if ( !valueNode || !valueNode->text )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		int status = 0;

		if ( !_tcscmp( valueNode->text, _T("away"))) status = ID_STATUS_AWAY;
			else if ( !_tcscmp( valueNode->text, _T("xa"))) status = ID_STATUS_NA;
			else if ( !_tcscmp( valueNode->text, _T("dnd"))) status = ID_STATUS_DND;
			else if ( !_tcscmp( valueNode->text, _T("chat"))) status = ID_STATUS_FREECHAT;
			else if ( !_tcscmp( valueNode->text, _T("online"))) status = ID_STATUS_ONLINE;
			else if ( !_tcscmp( valueNode->text, _T("invisible"))) status = ID_STATUS_INVISIBLE;
			else if ( !_tcscmp( valueNode->text, _T("offline"))) status = ID_STATUS_OFFLINE;

		if ( !status )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		int priority = -9999;

		fieldNode = JabberXmlGetChildWithGivenAttrValue( xNode, "field", "var", _T("status-priority") );
		if ( fieldNode && (valueNode = JabberXmlGetChild( fieldNode, "value" ))) {
			if ( valueNode->text )
				priority = _ttoi( valueNode->text );
		}

		if ( priority >= -128 && priority <= 127 )
			JSetWord( NULL, "Priority", (WORD)priority );

		char* szStatusMessage = NULL;
		fieldNode = JabberXmlGetChildWithGivenAttrValue( xNode, "field", "var", _T("status-message") );
		if ( fieldNode && (valueNode = JabberXmlGetChild( fieldNode, "value" ))) {
			if ( valueNode->text )
				szStatusMessage = mir_t2a(valueNode->text);
		}

		// skip f...ng away dialog
		int nNoDlg = DBGetContactSettingByte( NULL, "SRAway", StatusModeToDbSetting( status, "NoDlg" ), 0 );
		DBWriteContactSettingByte( NULL, "SRAway", StatusModeToDbSetting( status, "NoDlg" ), 1 );

		DBWriteContactSettingString( NULL, "SRAway", StatusModeToDbSetting( status, "Msg" ), szStatusMessage );

		fieldNode = JabberXmlGetChildWithGivenAttrValue( xNode, "field", "var", _T("status-global") );
		if ( fieldNode && (valueNode = JabberXmlGetChild( fieldNode, "value" ))) {
			if ( valueNode->text && _ttoi( valueNode->text ))
				JCallService( MS_CLIST_SETSTATUSMODE, status, NULL );
			else
				CallProtoService( jabberProtoName, PS_SETSTATUS, status, NULL );
		}
		JabberSetAwayMsg( status, (LPARAM)szStatusMessage );

		// return NoDlg setting
		DBWriteContactSettingByte( NULL, "SRAway", StatusModeToDbSetting( status, "NoDlg" ), (BYTE)nNoDlg );

		if ( szStatusMessage )
			mir_free( szStatusMessage );

		return JABBER_ADHOC_HANDLER_STATUS_COMPLETED;
	}
	return JABBER_ADHOC_HANDLER_STATUS_CANCEL;
}

int JabberAdhocOptionsHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession )
{
	if ( pSession->GetStage() == 0 ) {
		// first form
		pSession->SetStage( 1 );

		XmlNodeIq iq( "result", pInfo );
		XmlNode* commandNode = iq.addChild( "command" );
		commandNode->addAttr( "xmlns", JABBER_FEAT_COMMANDS );
		commandNode->addAttr( "node", JABBER_FEAT_RC_SET_OPTIONS );
		commandNode->addAttr( "sessionid", pSession->GetSessionId() );
		commandNode->addAttr( "status", "executing" );

		XmlNode* xNode = commandNode->addChild( "x" );
		xNode->addAttr( "xmlns", JABBER_FEAT_DATA_FORMS );
		xNode->addAttr( "type", "form" );

		xNode->addChild( "title", "Set Options" );
		xNode->addChild( "instructions", "Set the desired options" );

		XmlNode* fieldNode = NULL;
		TCHAR szTmpBuff[ 1024 ];

		fieldNode = xNode->addChild( "field" );
		fieldNode->addAttr( "type", "hidden" );
		fieldNode->addAttr( "var", "FORM_TYPE" );
		fieldNode->addChild( "value", JABBER_FEAT_RC );

		// Automatically Accept File Transfers
		fieldNode = xNode->addChild( "field" );
		fieldNode->addAttr( "label", "Automatically Accept File Transfers" );
		fieldNode->addAttr( "type", "boolean" );
		fieldNode->addAttr( "var", "auto-files" );
		mir_sntprintf( szTmpBuff, SIZEOF(szTmpBuff), _T("%d"), DBGetContactSettingByte( NULL, "SRFile", "AutoAccept", 0 ));
		fieldNode->addChild( "value", szTmpBuff );

		// Use sounds
		fieldNode = xNode->addChild( "field" );
		fieldNode->addAttr( "label", "Play sounds" );
		fieldNode->addAttr( "type", "boolean" );
		fieldNode->addAttr( "var", "sounds" );
		mir_sntprintf( szTmpBuff, SIZEOF(szTmpBuff), _T("%d"), DBGetContactSettingByte( NULL, "Skin", "UseSound", 0 ));
		fieldNode->addChild( "value", szTmpBuff );

		// Disable remote controlling
		fieldNode = xNode->addChild( "field" );
		fieldNode->addAttr( "label", "Disable remote controlling (check twice what you are doing)" );
		fieldNode->addAttr( "type", "boolean" );
		fieldNode->addAttr( "var", "enable-rc" );
		fieldNode->addChild( "value", "0" );

		jabberThreadInfo->send( iq );
		return JABBER_ADHOC_HANDLER_STATUS_EXECUTING;
	}
	else if ( pSession->GetStage() == 1 ) {
		// result form here
		XmlNode* commandNode = pInfo->GetChildNode();
		XmlNode* xNode = JabberXmlGetChildWithGivenAttrValue( commandNode, "x", "xmlns", _T(JABBER_FEAT_DATA_FORMS) );
		if ( !xNode )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		XmlNode* fieldNode = NULL;
		XmlNode* valueNode = NULL;

		// Automatically Accept File Transfers
		fieldNode = JabberXmlGetChildWithGivenAttrValue( xNode, "field", "var", _T("auto-files") );
		if ( fieldNode && (valueNode = JabberXmlGetChild( fieldNode, "value" ))) {
			if ( valueNode->text )
				DBWriteContactSettingByte( NULL, "SRFile", "AutoAccept", (BYTE)_ttoi( valueNode->text ) );
		}

		// Use sounds
		fieldNode = JabberXmlGetChildWithGivenAttrValue( xNode, "field", "var", _T("sounds") );
		if ( fieldNode && (valueNode = JabberXmlGetChild( fieldNode, "value" ))) {
			if ( valueNode->text )
				DBWriteContactSettingByte( NULL, "Skin", "UseSound", (BYTE)_ttoi( valueNode->text ) );
		}

		// Disable remote controlling
		fieldNode = JabberXmlGetChildWithGivenAttrValue( xNode, "field", "var", _T("enable-rc") );
		if ( fieldNode && (valueNode = JabberXmlGetChild( fieldNode, "value" ))) {
			if ( valueNode->text && _ttoi( valueNode->text ))
				JSetByte( "EnableRemoteControl", 0 );
		}

		return JABBER_ADHOC_HANDLER_STATUS_COMPLETED;
	}
	return JABBER_ADHOC_HANDLER_STATUS_CANCEL;
}

int JabberRcGetUnreadEventsCount()
{
	int nEventsSent = 0;
	HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) {
		char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
		if ( szProto != NULL && !strcmp( szProto, jabberProtoName )) {
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

int JabberAdhocForwardHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession )
{
	TCHAR szMsg[ 1024 ];
	if ( pSession->GetStage() == 0 ) {
		int nUnreadEvents = JabberRcGetUnreadEventsCount();
		if ( !nUnreadEvents ) {
			XmlNodeIq iq( "result", pInfo );
			XmlNode* commandNode = iq.addChild( "command" );
			commandNode->addAttr( "xmlns", JABBER_FEAT_COMMANDS );
			commandNode->addAttr( "node", JABBER_FEAT_RC_FORWARD );
			commandNode->addAttr( "sessionid", pSession->GetSessionId() );
			commandNode->addAttr( "status", "completed" );

			mir_sntprintf( szMsg, SIZEOF(szMsg), _T("There is no messages to forward") );
			XmlNode* noteNode = commandNode->addChild( "note", szMsg );
			noteNode->addAttr( "type", "info" );

			jabberThreadInfo->send( iq );

			return JABBER_ADHOC_HANDLER_STATUS_REMOVE_SESSION;
		}

		// first form
		pSession->SetStage( 1 );

		XmlNodeIq iq( "result", pInfo );
		XmlNode* commandNode = iq.addChild( "command" );
		commandNode->addAttr( "xmlns", JABBER_FEAT_COMMANDS );
		commandNode->addAttr( "node", JABBER_FEAT_RC_FORWARD );
		commandNode->addAttr( "sessionid", pSession->GetSessionId() );
		commandNode->addAttr( "status", "executing" );

		XmlNode* xNode = commandNode->addChild( "x" );
		xNode->addAttr( "xmlns", JABBER_FEAT_DATA_FORMS );
		xNode->addAttr( "type", "form" );

		xNode->addChild( "title", "Forward options" );
		mir_sntprintf( szMsg, SIZEOF(szMsg), _T("%d message(s) to be forwarded"), nUnreadEvents );
		xNode->addChild( "instructions", szMsg );

		XmlNode* fieldNode = NULL;

		fieldNode = xNode->addChild( "field" );
		fieldNode->addAttr( "type", "hidden" );
		fieldNode->addAttr( "var", "FORM_TYPE" );
		fieldNode->addChild( "value", JABBER_FEAT_RC );

		// remove clist events
		fieldNode = xNode->addChild( "field" );
		fieldNode->addAttr( "label", "Mark messages as read" );
		fieldNode->addAttr( "type", "boolean" );
		fieldNode->addAttr( "var", "remove-clist-events" );
		fieldNode->addChild( "value", "1" );

		jabberThreadInfo->send( iq );
		return JABBER_ADHOC_HANDLER_STATUS_EXECUTING;
	}
	else if ( pSession->GetStage() == 1 ) {
		// result form here
		XmlNode* commandNode = pInfo->GetChildNode();
		XmlNode* xNode = JabberXmlGetChildWithGivenAttrValue( commandNode, "x", "xmlns", _T(JABBER_FEAT_DATA_FORMS) );
		if ( !xNode )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		XmlNode* fieldNode = NULL;
		XmlNode* valueNode = NULL;

		BOOL bRemoveCListEvents = TRUE;

		// remove clist events
		fieldNode = JabberXmlGetChildWithGivenAttrValue( xNode, "field", "var", _T("remove-clist-events") );
		if ( fieldNode && (valueNode = JabberXmlGetChild( fieldNode, "value" ))) {
			if ( valueNode->text && !_ttoi( valueNode->text ) ) {
				bRemoveCListEvents = FALSE;
			}
		}

		int nEventsSent = 0;
		HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
		while ( hContact != NULL ) {
			char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
			if ( szProto != NULL && !strcmp( szProto, jabberProtoName )) {
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
									XmlNode msg( "message" );
									msg.addAttr( "to", pInfo->GetFrom() );
									msg.addAttrID( JabberSerialNext() );

									XmlNode* bodyNode = msg.addChild( "body", szEventText );
									XmlNode* addressesNode = msg.addChild( "addresses" );
									addressesNode->addAttr( "xmlns", JABBER_FEAT_EXT_ADDRESSING );

									XmlNode* addressNode = addressesNode->addChild( "address" );
									addressNode->addAttr( "type", "ofrom" );
									addressNode->addAttr( "jid", dbv.ptszVal );

									addressNode = addressesNode->addChild( "address" );
									addressNode->addAttr( "type", "oto" );
									addressNode->addAttr( "jid", jabberThreadInfo->fullJID );

									time_t ltime = ( time_t )dbei.timestamp;
									struct tm *gmt = gmtime( &ltime );
									char stime[ 512 ];
									sprintf(stime, "%.4i-%.2i-%.2iT%.2i:%.2i:%.2iZ", gmt->tm_year + 1900, gmt->tm_mon + 1, gmt->tm_mday,
										gmt->tm_hour, gmt->tm_min, gmt->tm_sec);

									XmlNode* delayNode = msg.addChild( "delay" );
									delayNode->addAttr( "xmlns", "urn:xmpp:delay" );
									delayNode->addAttr( "stamp", stime );

									jabberThreadInfo->send( msg );

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
		commandNode = iq.addChild( "command" );
		commandNode->addAttr( "xmlns", JABBER_FEAT_COMMANDS );
		commandNode->addAttr( "node", JABBER_FEAT_RC_FORWARD );
		commandNode->addAttr( "sessionid", pSession->GetSessionId() );
		commandNode->addAttr( "status", "completed" );

		mir_sntprintf( szMsg, SIZEOF(szMsg), _T("%d message(s) forwarded"), nEventsSent );
		XmlNode* noteNode = commandNode->addChild( "note", szMsg );
		noteNode->addAttr( "type", "info" );

		jabberThreadInfo->send( iq );

		return JABBER_ADHOC_HANDLER_STATUS_REMOVE_SESSION;
	}

	return JABBER_ADHOC_HANDLER_STATUS_CANCEL;
}

typedef BOOL (WINAPI *LWS )( VOID );

int JabberAdhocLockWSHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession )
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
	XmlNode* commandNode = iq.addChild( "command" );
	commandNode->addAttr( "xmlns", JABBER_FEAT_COMMANDS );
	commandNode->addAttr( "node", JABBER_FEAT_RC_WS_LOCK );
	commandNode->addAttr( "sessionid", pSession->GetSessionId() );
	commandNode->addAttr( "status", "completed" );

	TCHAR szMsg[ 1024 ];
	if ( bOk )
		mir_sntprintf( szMsg, SIZEOF(szMsg), _T("Workstation successfully locked") );
	else
		mir_sntprintf( szMsg, SIZEOF(szMsg), _T("Error %d occured during workstation lock"), GetLastError() );

	XmlNode* noteNode = commandNode->addChild( "note", szMsg );
	noteNode->addAttr( "type", bOk ? "info" : "error" );

	jabberThreadInfo->send( iq );

	return JABBER_ADHOC_HANDLER_STATUS_REMOVE_SESSION;
}

static void __cdecl JabberQuitMirandaIMThread( void* pParam )
{
	JabberLog( "JabberQuitMirandaIMThread start" );
	SleepEx( 2000, TRUE );
	JCallService( "CloseAction", 0, 0 );
	JabberLog( "JabberQuitMirandaIMThread exit" );
}

int JabberAdhocQuitMirandaHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession )
{
	if ( pSession->GetStage() == 0 ) {
		// first form
		pSession->SetStage( 1 );

		XmlNodeIq iq( "result", pInfo );
		XmlNode* commandNode = iq.addChild( "command" );
		commandNode->addAttr( "xmlns", JABBER_FEAT_COMMANDS );
		commandNode->addAttr( "node", JABBER_FEAT_RC_QUIT_MIRANDA );
		commandNode->addAttr( "sessionid", pSession->GetSessionId() );
		commandNode->addAttr( "status", "executing" );

		XmlNode* xNode = commandNode->addChild( "x" );
		xNode->addAttr( "xmlns", JABBER_FEAT_DATA_FORMS );
		xNode->addAttr( "type", "form" );

		xNode->addChild( "title", "Confirmation needed" );
		xNode->addChild( "instructions", "Please confirm Miranda IM shutdown" );

		XmlNode* fieldNode = NULL;

		fieldNode = xNode->addChild( "field" );
		fieldNode->addAttr( "type", "hidden" );
		fieldNode->addAttr( "var", "FORM_TYPE" );
		fieldNode->addChild( "value", JABBER_FEAT_RC );

		// I Agree checkbox
		fieldNode = xNode->addChild( "field" );
		fieldNode->addAttr( "label", "I agree" );
		fieldNode->addAttr( "type", "boolean" );
		fieldNode->addAttr( "var", "allow-shutdown" );
		fieldNode->addChild( "value", "0" );

		jabberThreadInfo->send( iq );
		return JABBER_ADHOC_HANDLER_STATUS_EXECUTING;
	}
	else if ( pSession->GetStage() == 1 ) {
		// result form here
		XmlNode* commandNode = pInfo->GetChildNode();
		XmlNode* xNode = JabberXmlGetChildWithGivenAttrValue( commandNode, "x", "xmlns", _T(JABBER_FEAT_DATA_FORMS) );
		if ( !xNode )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		XmlNode* fieldNode = NULL;
		XmlNode* valueNode = NULL;

		// I Agree checkbox
		fieldNode = JabberXmlGetChildWithGivenAttrValue( xNode, "field", "var", _T("allow-shutdown") );
		if ( fieldNode && (valueNode = JabberXmlGetChild( fieldNode, "value" ))) {
			if ( valueNode->text && _ttoi( valueNode->text ) ) {
				mir_forkthread( JabberQuitMirandaIMThread, NULL );
			}
		}

		return JABBER_ADHOC_HANDLER_STATUS_COMPLETED;
	}
	return JABBER_ADHOC_HANDLER_STATUS_CANCEL;
}

int JabberAdhocLeaveGroupchatsHandler( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo, CJabberAdhocSession* pSession )
{
	int i = 0;
	if ( pSession->GetStage() == 0 ) {
		// first form
		TCHAR szMsg[ 1024 ];

		JabberListLock();
		int nChatsCount = 0;
		for ( i = 0; ( i=JabberListFindNext( LIST_CHATROOM, i )) >= 0; i++ ) {
			JABBER_LIST_ITEM *item = JabberListGetItemPtrFromIndex( i );
			if ( item != NULL )
				nChatsCount++;
		}
		JabberListUnlock();

		if ( !nChatsCount ) {
			XmlNodeIq iq( "result", pInfo );
			XmlNode* commandNode = iq.addChild( "command" );
			commandNode->addAttr( "xmlns", JABBER_FEAT_COMMANDS );
			commandNode->addAttr( "node", JABBER_FEAT_RC_LEAVE_GROUPCHATS );
			commandNode->addAttr( "sessionid", pSession->GetSessionId() );
			commandNode->addAttr( "status", "completed" );

			mir_sntprintf( szMsg, SIZEOF(szMsg), _T("There is no groupchats to leave") );
			XmlNode* noteNode = commandNode->addChild( "note", szMsg );
			noteNode->addAttr( "type", "info" );

			jabberThreadInfo->send( iq );

			return JABBER_ADHOC_HANDLER_STATUS_REMOVE_SESSION;
		}

		pSession->SetStage( 1 );

		XmlNodeIq iq( "result", pInfo );
		XmlNode* commandNode = iq.addChild( "command" );
		commandNode->addAttr( "xmlns", JABBER_FEAT_COMMANDS );
		commandNode->addAttr( "node", JABBER_FEAT_RC_LEAVE_GROUPCHATS );
		commandNode->addAttr( "sessionid", pSession->GetSessionId() );
		commandNode->addAttr( "status", "executing" );

		XmlNode* xNode = commandNode->addChild( "x" );
		xNode->addAttr( "xmlns", JABBER_FEAT_DATA_FORMS );
		xNode->addAttr( "type", "form" );

		xNode->addChild( "title", "Leave groupchats" );
		xNode->addChild( "instructions", "Choose the groupchats you want to leave" );

		XmlNode* fieldNode = NULL;

		fieldNode = xNode->addChild( "field" );
		fieldNode->addAttr( "type", "hidden" );
		fieldNode->addAttr( "var", "FORM_TYPE" );
		fieldNode->addChild( "value", JABBER_FEAT_RC );

		// Groupchats
		fieldNode = xNode->addChild( "field" );
		fieldNode->addAttr( "label", "" );
		fieldNode->addAttr( "type", "list-multi" );
		fieldNode->addAttr( "var", "groupchats" );
		fieldNode->addChild( "required" );

		JabberListLock();
		for ( i = 0; ( i=JabberListFindNext( LIST_CHATROOM, i )) >= 0; i++ ) {
			JABBER_LIST_ITEM *item = JabberListGetItemPtrFromIndex( i );
			if ( item != NULL ) {
				XmlNode* optionNode = fieldNode->addChild( "option" );
				optionNode->addAttr( "label", item->jid );
				optionNode->addChild( "value", item->jid );
			}
		}
		JabberListUnlock();

		jabberThreadInfo->send( iq );
		return JABBER_ADHOC_HANDLER_STATUS_EXECUTING;
	}
	else if ( pSession->GetStage() == 1 ) {
		// result form here
		XmlNode* commandNode = pInfo->GetChildNode();
		XmlNode* xNode = JabberXmlGetChildWithGivenAttrValue( commandNode, "x", "xmlns", _T(JABBER_FEAT_DATA_FORMS) );
		if ( !xNode )
			return JABBER_ADHOC_HANDLER_STATUS_CANCEL;

		XmlNode* fieldNode = NULL;
		XmlNode* valueNode = NULL;

		// Groupchat list here:
		fieldNode = JabberXmlGetChildWithGivenAttrValue( xNode, "field", "var", _T("groupchats") );
		if ( fieldNode ) {
			for ( i = 0; i < fieldNode->numChild; i++ ) {
				valueNode = fieldNode->child[i];
				if ( valueNode && valueNode->name && valueNode->text && !strcmp( valueNode->name, "value" )) {
					JABBER_LIST_ITEM* item = JabberListGetItemPtr( LIST_CHATROOM, valueNode->text );
					if ( item )
						JabberGcQuit( item, 0, NULL );
				}
			}
		}

		return JABBER_ADHOC_HANDLER_STATUS_COMPLETED;
	}
	return JABBER_ADHOC_HANDLER_STATUS_CANCEL;
}
