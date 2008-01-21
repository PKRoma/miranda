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

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_iq.h"
#include "jabber_caps.h"
#include "jabber_iq_handlers.h"
#include "jabber_privacy.h"
#include "jabber_ibb.h"
#include "jabber_rc.h"

void CJabberProto::JabberIqInit()
{
	InitializeCriticalSection( &csIqList );
	iqList = NULL;
	iqCount = 0;
	iqAlloced = 0;
}

void CJabberProto::JabberIqUninit()
{
	if ( iqList ) mir_free( iqList );
	iqList = NULL;
	iqCount = 0;
	iqAlloced = 0;
	DeleteCriticalSection( &csIqList );
}

void CJabberProto::JabberIqRemove( int index )
{
	EnterCriticalSection( &csIqList );
	if ( index>=0 && index<iqCount ) {
		memmove( iqList+index, iqList+index+1, sizeof( JABBER_IQ_FUNC )*( iqCount-index-1 ));
		iqCount--;
	}
	LeaveCriticalSection( &csIqList );
}

void CJabberProto::JabberIqExpire()
{
	int i;
	time_t expire;

	EnterCriticalSection( &csIqList );
	expire = time( NULL ) - 120;	// 2 minute
	i = 0;
	while ( i < iqCount ) {
		if ( iqList[i].requestTime < expire )
			JabberIqRemove( i );
		else
			i++;
	}
	LeaveCriticalSection( &csIqList );
}

JABBER_IQ_PFUNC CJabberProto::JabberIqFetchFunc( int iqId )
{
	int i;
	JABBER_IQ_PFUNC res;

	EnterCriticalSection( &csIqList );
	JabberIqExpire();
#ifdef _DEBUG
	for ( i=0; i<iqCount; i++ )
		JabberLog( "  %04d : %02d : 0x%x", iqList[i].iqId, iqList[i].procId, iqList[i].func );
#endif
	for ( i=0; i<iqCount && iqList[i].iqId!=iqId; i++ );
	if ( i < iqCount ) {
		res = iqList[i].func;
		JabberIqRemove( i );
	}
	else {
		res = ( JABBER_IQ_PFUNC ) NULL;
	}
	LeaveCriticalSection( &csIqList );
	return res;
}

void CJabberProto::JabberIqAdd( unsigned int iqId, JABBER_IQ_PROCID procId, JABBER_IQ_PFUNC func )
{
	int i;

	EnterCriticalSection( &csIqList );
	JabberLog( "IqAdd id=%d, proc=%d, func=0x%x", iqId, procId, func );
	if ( procId == IQ_PROC_NONE )
		i = iqCount;
	else
		for ( i=0; i<iqCount && iqList[i].procId!=procId; i++ );

	if ( i>=iqCount && iqCount>=iqAlloced ) {
		iqAlloced = iqCount + 8;
		iqList = ( JABBER_IQ_FUNC * )mir_realloc( iqList, sizeof( JABBER_IQ_FUNC )*iqAlloced );
	}

	if ( iqList != NULL ) {
		iqList[i].iqId = iqId;
		iqList[i].procId = procId;
		iqList[i].func = func;
		iqList[i].requestTime = time( NULL );
		if ( i == iqCount ) iqCount++;
	}
	LeaveCriticalSection( &csIqList );
}

BOOL CJabberIqManager::FillPermanentHandlers()
{
	// version requests (XEP-0092)
	AddPermanentHandler( &CJabberProto::JabberProcessIqVersion, JABBER_IQ_TYPE_GET, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_ID_STR, _T(JABBER_FEAT_VERSION), FALSE, _T("query"));

	// last activity (XEP-0012)
	AddPermanentHandler( &CJabberProto::JabberProcessIqLast, JABBER_IQ_TYPE_GET, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_ID_STR, _T(JABBER_FEAT_LAST_ACTIVITY), FALSE, _T("query"));

	// ping requests (XEP-0199)
	AddPermanentHandler( &CJabberProto::JabberProcessIqPing, JABBER_IQ_TYPE_GET, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_ID_STR, _T(JABBER_FEAT_PING), FALSE, _T("ping"));

	// entity time (XEP-0202)
	AddPermanentHandler( &CJabberProto::JabberProcessIqTime202, JABBER_IQ_TYPE_GET, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_ID_STR, _T(JABBER_FEAT_ENTITY_TIME), FALSE, _T("time"));

	// old avatars support (deprecated XEP-0008)
	AddPermanentHandler( &CJabberProto::JabberProcessIqAvatar, JABBER_IQ_TYPE_GET, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_ID_STR, _T(JABBER_FEAT_AVATAR), FALSE, _T("query"));

	// privacy lists (XEP-0016)
	AddPermanentHandler( &CJabberProto::JabberProcessIqPrivacyLists, JABBER_IQ_TYPE_SET, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_ID_STR, _T(JABBER_FEAT_PRIVACY_LISTS), FALSE, _T("query"));

	// in band bytestreams (XEP-0047)
	AddPermanentHandler( &CJabberProto::JabberFtHandleIbbIq, JABBER_IQ_TYPE_SET, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_CHILD_TAG_NODE | JABBER_IQ_PARSE_CHILD_TAG_NAME | JABBER_IQ_PARSE_CHILD_TAG_XMLNS, _T(JABBER_FEAT_IBB), FALSE, NULL);

	// socks5-bytestreams (XEP-0065)
	AddPermanentHandler( &CJabberProto::JabberFtHandleBytestreamRequest, JABBER_IQ_TYPE_SET, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_ID_STR | JABBER_IQ_PARSE_CHILD_TAG_NODE, _T(JABBER_FEAT_BYTESTREAMS), FALSE, _T("query"));

	// session initiation (XEP-0095)
	AddPermanentHandler( &CJabberProto::JabberHandleSiRequest, JABBER_IQ_TYPE_SET, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_ID_STR | JABBER_IQ_PARSE_CHILD_TAG_NODE, _T(JABBER_FEAT_SI), FALSE, _T("si"));

	// roster push requests
	AddPermanentHandler( &CJabberProto::JabberHandleRosterPushRequest, JABBER_IQ_TYPE_SET, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_ID_STR | JABBER_IQ_PARSE_CHILD_TAG_NODE, _T(JABBER_FEAT_IQ_ROSTER), FALSE, _T("query"));

	// OOB file transfers
	AddPermanentHandler( &CJabberProto::JabberHandleIqRequestOOB, JABBER_IQ_TYPE_SET, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_HCONTACT | JABBER_IQ_PARSE_ID_STR | JABBER_IQ_PARSE_CHILD_TAG_NODE, _T(JABBER_FEAT_OOB), FALSE, _T("query"));

	// disco#items requests (XEP-0030, XEP-0050)
	AddPermanentHandler( &CJabberProto::JabberHandleDiscoItemsRequest, JABBER_IQ_TYPE_GET, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_TO | JABBER_IQ_PARSE_ID_STR | JABBER_IQ_PARSE_CHILD_TAG_NODE, _T(JABBER_FEAT_DISCO_ITEMS), FALSE, _T("query"));

	// disco#info requests (XEP-0030, XEP-0050, XEP-0115)
	AddPermanentHandler( &CJabberProto::JabberHandleDiscoInfoRequest, JABBER_IQ_TYPE_GET, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_TO | JABBER_IQ_PARSE_ID_STR | JABBER_IQ_PARSE_CHILD_TAG_NODE, _T(JABBER_FEAT_DISCO_INFO), FALSE, _T("query"));

	// ad-hoc commands (XEP-0050) for remote controlling (XEP-0146)
	AddPermanentHandler( &CJabberProto::JabberHandleAdhocCommandRequest, JABBER_IQ_TYPE_SET, JABBER_IQ_PARSE_FROM | JABBER_IQ_PARSE_TO | JABBER_IQ_PARSE_ID_STR | JABBER_IQ_PARSE_CHILD_TAG_NODE, _T(JABBER_FEAT_COMMANDS), FALSE, _T("command"));

	return TRUE;
}

void CJabberIqManager::ExpirerThread()
{
	while (!m_bExpirerThreadShutdownRequest)
	{
		Lock();
		CJabberIqInfo* pInfo = DetachExpired();
		Unlock();
		if (!pInfo)
		{
			for (int i = 0; !m_bExpirerThreadShutdownRequest && (i < 10); i++)
				Sleep(50);

			// -1 thread :)
			ppro->m_adhocManager.ExpireSessions();

			continue;
		}
		ExpireInfo(pInfo);
		delete pInfo;
	}
}

void CJabberIqManager::ExpireInfo( CJabberIqInfo* pInfo, void *pUserData )
{
	if ( !pInfo )
		return;
	
	if ( pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_FROM )
		pInfo->m_szFrom = pInfo->m_szReceiver;
	if (( pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_HCONTACT ) && ( pInfo->m_szFrom ))
		pInfo->m_hContact = ppro->JabberHContactFromJID( pInfo->m_szFrom , 3);

	ppro->JabberLog( "Expiring iq id %d, sent to " TCHAR_STR_PARAM, pInfo->m_nIqId, pInfo->m_szReceiver ? pInfo->m_szReceiver : _T("unknown") );

	pInfo->m_nIqType = JABBER_IQ_TYPE_FAIL;
	(ppro->*(pInfo->m_pHandler))( NULL, NULL, pInfo );
}

CJabberIqInfo* CJabberIqManager::AddHandler(JABBER_IQ_HANDLER pHandler, int nIqType, TCHAR *szReceiver, DWORD dwParamsToParse, int nIqId, void *pUserData, DWORD dwGroupId, DWORD dwTimeout)
{
	CJabberIqInfo* pInfo = new CJabberIqInfo();
	if (!pInfo)
		return NULL;

	pInfo->m_pHandler = pHandler;
	if (nIqId == -1)
		nIqId = ppro->JabberSerialNext();
	pInfo->m_nIqId = nIqId;
	pInfo->m_nIqType = nIqType;
	pInfo->m_dwParamsToParse = dwParamsToParse;
	pInfo->m_pUserData = pUserData;
	pInfo->m_dwGroupId = dwGroupId;
	pInfo->m_dwRequestTime = GetTickCount();
	pInfo->m_dwTimeout = dwTimeout;
	pInfo->SetReceiver(szReceiver);

	AppendIq(pInfo);

	return pInfo;
}

BOOL CJabberIqManager::HandleIq(int nIqId, XmlNode *pNode, void *pUserData)
{
	if (nIqId == -1 || pNode == NULL)
		return FALSE;

	TCHAR *szType = JabberXmlGetAttrValue(pNode, "type");
	if ( !szType )
		return FALSE;

	int nIqType = JABBER_IQ_TYPE_FAIL;
	if (!_tcsicmp(szType, _T("result")))
		nIqType = JABBER_IQ_TYPE_RESULT;
	else if (!_tcsicmp(szType, _T("error")))
		nIqType = JABBER_IQ_TYPE_ERROR;
	else
		return FALSE;

	Lock();
	CJabberIqInfo* pInfo = DetachInfo(nIqId, 0);
	Unlock();
	if (pInfo)
	{
		pInfo->m_nIqType = nIqType;
		if (nIqType == JABBER_IQ_TYPE_RESULT) {
			if (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_CHILD_TAG_NODE)
				pInfo->m_pChildNode = JabberXmlGetFirstChild( pNode );
			
			if (pInfo->m_pChildNode && (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_CHILD_TAG_NAME))
				pInfo->m_szChildTagName = mir_a2t( pInfo->m_pChildNode->name );
			if (pInfo->m_pChildNode && (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_CHILD_TAG_XMLNS))
				pInfo->m_szChildTagXmlns = JabberXmlGetAttrValue( pInfo->m_pChildNode, "xmlns" );
		}

		if (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_TO)
			pInfo->m_szTo = JabberXmlGetAttrValue( pNode, "to" );

		if (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_FROM)
			pInfo->m_szFrom = JabberXmlGetAttrValue( pNode, "from" );
		if (pInfo->m_szFrom && (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_HCONTACT))
			pInfo->m_hContact = ppro->JabberHContactFromJID( pInfo->m_szFrom, 3 );

		if (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_ID_STR)
			pInfo->m_szId = JabberXmlGetAttrValue( pNode, "id" );

		(ppro->*(pInfo->m_pHandler))(pNode, pUserData, pInfo);
		mir_free( pInfo->m_szChildTagName );
		delete pInfo;
		return TRUE;
	}
	return FALSE;
}

BOOL CJabberIqManager::HandleIqPermanent(XmlNode *pNode, void *pUserData)
{
	TCHAR *szType = JabberXmlGetAttrValue(pNode, "type");
	if ( !szType )
		return FALSE;
	
	CJabberIqInfo iqInfo;

	iqInfo.m_nIqType = JABBER_IQ_TYPE_FAIL;
	if ( !_tcsicmp( szType, _T("get")))
		iqInfo.m_nIqType = JABBER_IQ_TYPE_GET;
	else if ( !_tcsicmp( szType, _T("set")))
		iqInfo.m_nIqType = JABBER_IQ_TYPE_SET;
	else
		return FALSE;

	XmlNode *pFirstChild = JabberXmlGetFirstChild( pNode );
	if ( !pFirstChild || !pFirstChild->name )
		return FALSE;
	
	TCHAR *szTagName = mir_a2t( pFirstChild->name );
	TCHAR *szXmlns = JabberXmlGetAttrValue( pFirstChild, "xmlns" );

	BOOL bHandled = FALSE;
	Lock();
	CJabberIqPermanentInfo *pInfo = m_pPermanentHandlers;
	while ( pInfo ) {
		BOOL bAllow = TRUE;
		if ( !(pInfo->m_nIqTypes & iqInfo.m_nIqType ))
			bAllow = FALSE;
		if ( bAllow && pInfo->m_szXmlns && ( !szXmlns || _tcscmp( pInfo->m_szXmlns, szXmlns )))
			bAllow = FALSE;
		if ( bAllow && pInfo->m_szTag && _tcscmp( pInfo->m_szTag, szTagName ))
			bAllow = FALSE;
		if ( bAllow ) {
			iqInfo.m_pChildNode = pFirstChild;
			iqInfo.m_szChildTagName = szTagName;
			iqInfo.m_szChildTagXmlns = szXmlns;
			iqInfo.m_szId = JabberXmlGetAttrValue( pNode, "id" );

			if (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_TO)
				iqInfo.m_szTo = JabberXmlGetAttrValue( pNode, "to" );

			if (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_FROM)
				iqInfo.m_szFrom = JabberXmlGetAttrValue( pNode, "from" );

			if ((pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_HCONTACT) && (iqInfo.m_szFrom))
				iqInfo.m_hContact = ppro->JabberHContactFromJID( iqInfo.m_szFrom, 3 );

			ppro->JabberLog( "Handling iq id " TCHAR_STR_PARAM ", type " TCHAR_STR_PARAM ", from " TCHAR_STR_PARAM, iqInfo.m_szId, szType, iqInfo.m_szFrom );
			(ppro->*(pInfo->m_pHandler))(pNode, pUserData, &iqInfo);
			bHandled = TRUE;
		}
		pInfo = pInfo->m_pNext;
	}
	Unlock();

	mir_free( szTagName );
	return bHandled;
}
