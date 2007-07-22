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

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_iq.h,v $
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#ifndef _JABBER_IQ_H_
#define _JABBER_IQ_H_

#include "jabber_xml.h"

class CJabberIqInfo;

typedef enum {
	IQ_PROC_NONE,
	IQ_PROC_GETAGENTS,
	IQ_PROC_GETREGISTER,
	IQ_PROC_SETREGISTER,
	IQ_PROC_GETVCARD,
	IQ_PROC_SETVCARD,
	IQ_PROC_GETSEARCH,
	IQ_PROC_GETSEARCHFIELDS,
	IQ_PROC_BROWSEROOMS,
	IQ_PROC_DISCOROOMSERVER,
	IQ_PROC_DISCOAGENTS,
	IQ_PROC_DISCOBOOKMARKS,
	IQ_PROC_SETBOOKMARKS,
	IQ_PROC_DISCOCOMMANDS,
	IQ_PROC_EXECCOMMANDS,
} JABBER_IQ_PROCID;

typedef void ( *JABBER_IQ_PFUNC )( XmlNode *iqNode, void *usedata );

typedef struct {
	TCHAR* xmlns;
	JABBER_IQ_PFUNC func;
	BOOL allowSubNs;		// e.g. #info in disco#info
} JABBER_IQ_XMLNS_FUNC;

void JabberIqInit();
void JabberIqUninit();
JABBER_IQ_PFUNC JabberIqFetchFunc( int iqId );
void JabberIqAdd( unsigned int iqId, JABBER_IQ_PROCID procId, JABBER_IQ_PFUNC func );

void JabberIqResultBind( XmlNode *iqNode, void *userdata );
void JabberIqResultBrowseRooms( XmlNode *iqNode, void *userdata );
void JabberIqResultDiscoAgentInfo( XmlNode *iqNode, void *userdata );
void JabberIqResultDiscoAgentItems( XmlNode *iqNode, void *userdata );
//void JabberIqResultDiscoClientInfo( XmlNode *iqNode, void *userdata );
void JabberIqResultDiscoRoomItems( XmlNode *iqNode, void *userdata );
void JabberIqResultDiscoBookmarks( XmlNode *iqNode, void *userdata );
void JabberIqResultSetBookmarks( XmlNode *iqNode, void *userdata );
void JabberIqResultExtSearch( XmlNode *iqNode, void *userdata );
void JabberIqResultGetAgents( XmlNode *iqNode, void *userdata );
void JabberIqResultGetAuth( XmlNode *iqNode, void *userdata );
void JabberIqResultGetAvatar( XmlNode *iqNode, void *userdata );
void JabberIqResultGetMuc( XmlNode *iqNode, void *userdata );
void JabberIqResultGetRegister( XmlNode *iqNode, void *userdata );
void JabberIqResultGetRoster( XmlNode *iqNode, void *userdata );
void JabberIqResultGetVcard( XmlNode *iqNode, void *userdata );
void JabberIqResultMucGetAdminList( XmlNode *iqNode, void *userdata );
void JabberIqResultMucGetBanList( XmlNode *iqNode, void *userdata );
void JabberIqResultMucGetMemberList( XmlNode *iqNode, void *userdata );
void JabberIqResultMucGetModeratorList( XmlNode *iqNode, void *userdata );
void JabberIqResultMucGetOwnerList( XmlNode *iqNode, void *userdata );
void JabberIqResultMucGetVoiceList( XmlNode *iqNode, void *userdata );
void JabberIqResultSession( XmlNode *iqNode, void *userdata );
void JabberIqResultSetAuth( XmlNode *iqNode, void *userdata );
void JabberIqResultSetPassword( XmlNode *iqNode, void *userdata );
void JabberIqResultSetRegister( XmlNode *iqNode, void *userdata );
void JabberIqResultSetSearch( XmlNode *iqNode, void *userdata );
void JabberIqResultSetVcard( XmlNode *iqNode, void *userdata );
void JabberIqResultEntityTime( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );
void JabberIqResultLastActivity( XmlNode *iqNode, void *userdata, CJabberIqInfo* pInfo );

void JabberSetBookmarkRequest (XmlNodeIq& iqId);

unsigned int  __stdcall JabberSerialNext( void );
HANDLE        __stdcall JabberHContactFromJID( const TCHAR* jid , BOOL bStripResource );
void          __stdcall JabberLog( const char* fmt, ... );
TCHAR*        a2t( const char* str );

void  __stdcall replaceStr( char*& dest, const char* src );
void  __stdcall replaceStr( WCHAR*& dest, const WCHAR* src );

// 2 minutes, milliseconds
#define JABBER_DEFAULT_IQ_REQUEST_TIMEOUT		120000

class CJabberIqRequestManager;

typedef void ( *JABBER_IQ_HANDLER )( XmlNode *iqNode, void *usedata, CJabberIqInfo* pInfo );

#define JABBER_IQ_TYPE_FAIL						0
#define JABBER_IQ_TYPE_RESULT					1
#define JABBER_IQ_TYPE_ERROR					2
#define JABBER_IQ_TYPE_GET						4
#define JABBER_IQ_TYPE_SET						8

#define JABBER_IQ_TYPE_GOOD						(JABBER_IQ_TYPE_RESULT|JABBER_IQ_TYPE_GET|JABBER_IQ_TYPE_SET)

#define JABBER_IQ_PARSE_CHILD_TAG_NODE			(1)
#define JABBER_IQ_PARSE_CHILD_TAG_NAME			((1<<1)|JABBER_IQ_PARSE_CHILD_TAG_NODE)
#define JABBER_IQ_PARSE_CHILD_TAG_XMLNS			((1<<2)|JABBER_IQ_PARSE_CHILD_TAG_NODE)
#define JABBER_IQ_PARSE_FROM					(1<<3)
#define JABBER_IQ_PARSE_HCONTACT				((1<<4)|JABBER_IQ_PARSE_FROM)
#define JABBER_IQ_PARSE_TO						(1<<5)
#define JABBER_IQ_PARSE_ID_STR					(1<<6)

#define JABBER_IQ_PARSE_DEFAULT					(JABBER_IQ_PARSE_CHILD_TAG_NODE|JABBER_IQ_PARSE_CHILD_TAG_NAME|JABBER_IQ_PARSE_CHILD_TAG_XMLNS)

class CJabberIqInfo
{
protected:
	friend class CJabberIqManager;
	JABBER_IQ_HANDLER m_pHandler;
	CJabberIqInfo* m_pNext;
	
	int m_nIqId;
	DWORD m_dwParamsToParse;
	DWORD m_dwRequestTime;
	DWORD m_dwTimeout;
	TCHAR *m_szReceiver;
public:
	void *m_pUserData;
	DWORD m_dwGroupId;
public:// parsed data
	int m_nIqType;
	TCHAR *m_szFrom;
	TCHAR *m_szChildTagXmlns;
	TCHAR *m_szChildTagName;
	XmlNode *m_pChildNode;
	HANDLE m_hContact; 
	TCHAR *m_szTo;
	TCHAR *m_szId;
public:
	CJabberIqInfo()
	{
		ZeroMemory(this, sizeof(CJabberIqInfo));
	}
	~CJabberIqInfo()
	{
		if (m_szReceiver)
			mir_free(m_szReceiver);
	}
	void SetReceiver(TCHAR *szReceiver)
	{
		replaceStr(m_szReceiver, szReceiver);
	}
	TCHAR* GetReceiver()
	{
		return m_szReceiver;
	}
	void SetParamsToParse(DWORD dwParamsToParse)
	{
		m_dwParamsToParse = dwParamsToParse;
	}
	void SetTimeout(DWORD dwTimeout)
	{
		m_dwTimeout = dwTimeout;
	}
	int GetIqId()
	{
		return m_nIqId;
	}
	DWORD GetRequestTime()
	{
		return m_dwRequestTime;
	}
	int GetIqType()
	{
		return m_nIqType;
	}
	void* GetUserData()
	{
		return m_pUserData;
	}
	TCHAR* GetFrom()
	{
		return m_szFrom;
	}
	TCHAR* GetTo()
	{
		return m_szTo;
	}
	TCHAR* GetIdStr()
	{
		return m_szId;
	}
	HANDLE GetHContact()
	{
		return m_hContact;
	}
	XmlNode* GetChildNode()
	{
		return m_pChildNode;
	}
	TCHAR* GetChildNodeName()
	{
		return m_szChildTagName;
	}
	char* GetCharIqType()
	{
		switch (m_nIqType)
		{
		case JABBER_IQ_TYPE_SET: return "set";
		case JABBER_IQ_TYPE_GET: return "get";
		case JABBER_IQ_TYPE_ERROR: return "error";
		case JABBER_IQ_TYPE_RESULT: return "result";
		}
		return NULL;
	}
};

class CJabberIqPermanentInfo
{
	friend class CJabberIqManager;

	CJabberIqPermanentInfo* m_pNext;

	JABBER_IQ_HANDLER m_pHandler;
	DWORD m_dwParamsToParse;
	int m_nIqTypes;
	TCHAR* m_szXmlns;
	TCHAR* m_szTag;
	BOOL m_bAllowPartialNs;
public:
	CJabberIqPermanentInfo()
	{
		ZeroMemory(this, sizeof(CJabberIqPermanentInfo));
	}
	~CJabberIqPermanentInfo()
	{
		mir_free(m_szXmlns);
		mir_free(m_szTag);
		if ( m_pNext )
			delete m_pNext;
	}
};

class CJabberIqManager
{
protected:
	CRITICAL_SECTION m_cs;
	DWORD m_dwLastUsedHandle;
	CJabberIqInfo* m_pIqs;
	HANDLE m_hExpirerThread;
	BOOL m_bExpirerThreadShutdownRequest;

	CJabberIqPermanentInfo* m_pPermanentHandlers;

	CJabberIqInfo* DetachInfo(int nIqId, DWORD dwGroupId)
	{
		if (!m_pIqs)
			return NULL;

		CJabberIqInfo* pInfo = m_pIqs;
		if (nIqId == -1 ? m_pIqs->m_dwGroupId == dwGroupId : m_pIqs->m_nIqId == nIqId)
		{
			m_pIqs = pInfo->m_pNext;
			pInfo->m_pNext = NULL;
			return pInfo;
		}

		while (pInfo->m_pNext)
		{
			if (nIqId == -1 ? pInfo->m_pNext->m_dwGroupId == dwGroupId : pInfo->m_pNext->m_nIqId == nIqId)
			{
				CJabberIqInfo* pRetVal = pInfo->m_pNext;
				pInfo->m_pNext = pInfo->m_pNext->m_pNext;
				pRetVal->m_pNext = NULL;
				return pRetVal;
			}
			pInfo = pInfo->m_pNext;
		}
		return NULL;
	}
	CJabberIqInfo* DetachInfo(void *pUserData)
	{
		if (!m_pIqs)
			return NULL;

		CJabberIqInfo* pInfo = m_pIqs;
		if (m_pIqs->m_pUserData == pUserData)
		{
			m_pIqs = pInfo->m_pNext;
			pInfo->m_pNext = NULL;
			return pInfo;
		}

		while (pInfo->m_pNext)
		{
			if (pInfo->m_pNext->m_pUserData == pUserData)
			{
				CJabberIqInfo* pRetVal = pInfo->m_pNext;
				pInfo->m_pNext = pInfo->m_pNext->m_pNext;
				pRetVal->m_pNext = NULL;
				return pRetVal;
			}
			pInfo = pInfo->m_pNext;
		}
		return NULL;
	}
	CJabberIqInfo* DetachExpired()
	{
		if (!m_pIqs)
			return NULL;

		DWORD dwCurrentTime = GetTickCount();

		CJabberIqInfo* pInfo = m_pIqs;
		if (pInfo->m_dwRequestTime + pInfo->m_dwTimeout < dwCurrentTime)
		{
			m_pIqs = pInfo->m_pNext;
			pInfo->m_pNext = NULL;
			return pInfo;
		}

		while (pInfo->m_pNext)
		{
			if (pInfo->m_pNext->m_dwRequestTime + pInfo->m_pNext->m_dwTimeout < dwCurrentTime)
			{
				CJabberIqInfo* pRetVal = pInfo->m_pNext;
				pInfo->m_pNext = pInfo->m_pNext->m_pNext;
				pRetVal->m_pNext = NULL;
				return pRetVal;
			}
			pInfo = pInfo->m_pNext;
		}
		return NULL;
	}
	static DWORD WINAPI _ExpirerThread(LPVOID pParam)
	{
		CJabberIqManager *pManager = ( CJabberIqManager * )pParam;
		pManager->ExpirerThread();
		if ( !pManager->m_bExpirerThreadShutdownRequest ) {
			CloseHandle( pManager->m_hExpirerThread );
			pManager->m_hExpirerThread = NULL;
		}
		return 0;
	}
	void ExpirerThread();
	void ExpireInfo( CJabberIqInfo* pInfo, void *pUserData = NULL )
	{
		if ( !pInfo )
			return;
		
		if ( pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_FROM )
			pInfo->m_szFrom = pInfo->m_szReceiver;
		if (( pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_HCONTACT ) && ( pInfo->m_szFrom ))
			pInfo->m_hContact = JabberHContactFromJID( pInfo->m_szFrom , 3);

		JabberLog( "Expiring iq id %d, sent to %S", pInfo->m_nIqId, pInfo->m_szReceiver ? pInfo->m_szReceiver : _T("unknown") );

		pInfo->m_nIqType = JABBER_IQ_TYPE_FAIL;
		pInfo->m_pHandler( NULL, NULL, pInfo );
	}
	BOOL AppendIq(CJabberIqInfo* pInfo)
	{
		Lock();
		if (!m_pIqs)
			m_pIqs = pInfo;
		else
		{
			CJabberIqInfo* pTmp = m_pIqs;
			while (pTmp->m_pNext)
				pTmp = pTmp->m_pNext;
			pTmp->m_pNext = pInfo;
		}
		Unlock();
		return TRUE;
	}
public:
	CJabberIqManager()
	{
		InitializeCriticalSection(&m_cs);
		m_dwLastUsedHandle = 0;
		m_pIqs = NULL;
		m_hExpirerThread = NULL;
		m_pPermanentHandlers = NULL;
	}
	~CJabberIqManager()
	{
		ExpireAll();
		if ( m_pPermanentHandlers )
			delete m_pPermanentHandlers;
		DeleteCriticalSection(&m_cs);
	}
	BOOL Start()
	{
		if ( m_hExpirerThread || m_bExpirerThreadShutdownRequest )
			return FALSE;

		DWORD dwThreadId = 0;
		m_hExpirerThread = CreateThread( NULL, 0, _ExpirerThread, this, NULL, &dwThreadId );
		if (!m_hExpirerThread)
			return FALSE;

		return TRUE;
	}
	BOOL Shutdown()
	{
		if ( m_bExpirerThreadShutdownRequest || !m_hExpirerThread )
			return TRUE;

		m_bExpirerThreadShutdownRequest = TRUE;

		WaitForSingleObject( m_hExpirerThread, INFINITE );
		CloseHandle( m_hExpirerThread );
		m_hExpirerThread = NULL;

		return TRUE;
	}
	void Lock()
	{
		EnterCriticalSection(&m_cs);
	}
	void Unlock()
	{
		LeaveCriticalSection(&m_cs);
	}
	DWORD GetNextFreeGroupId()
	{
		Lock();
		DWORD dwRetVal = ++m_dwLastUsedHandle;
		Unlock();
		return dwRetVal;
	}
	DWORD GetGroupPendingIqCount(DWORD dwGroup)
	{
		Lock();
		DWORD dwCount = 0;
		CJabberIqInfo* pInfo = m_pIqs;
		while (pInfo)
		{
			if (pInfo->m_dwGroupId == dwGroup)
				dwCount++;
			pInfo = pInfo->m_pNext;
		}
		Unlock();
		return dwCount;
	}
	// fucking params, maybe just return CJabberIqRequestInfo pointer ?
	CJabberIqInfo* AddHandler(JABBER_IQ_HANDLER pHandler, int nIqType = JABBER_IQ_TYPE_GET, TCHAR *szReceiver = NULL, DWORD dwParamsToParse = 0, int nIqId = -1, void *pUserData = NULL, DWORD dwGroupId = 0, DWORD dwTimeout = JABBER_DEFAULT_IQ_REQUEST_TIMEOUT)
	{
		CJabberIqInfo* pInfo = new CJabberIqInfo();
		if (!pInfo)
			return NULL;

		pInfo->m_pHandler = pHandler;
		if (nIqId == -1)
			nIqId = JabberSerialNext();
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
	CJabberIqPermanentInfo* AddPermanentHandler(JABBER_IQ_HANDLER pHandler, int nIqTypes, DWORD dwParamsToParse, TCHAR* szXmlns, BOOL bAllowPartialNs, TCHAR* szTag)
	{
		CJabberIqPermanentInfo* pInfo = new CJabberIqPermanentInfo();
		if (!pInfo)
			return NULL;

		pInfo->m_pHandler = pHandler;
		pInfo->m_nIqTypes = nIqTypes;
		replaceStr( pInfo->m_szXmlns, szXmlns );
		pInfo->m_bAllowPartialNs = bAllowPartialNs;
		replaceStr( pInfo->m_szTag, szTag );
		pInfo->m_dwParamsToParse = dwParamsToParse;

		Lock();
		if (!m_pPermanentHandlers)
			m_pPermanentHandlers = pInfo;
		else
		{
			CJabberIqPermanentInfo* pTmp = m_pPermanentHandlers;
			while (pTmp->m_pNext)
				pTmp = pTmp->m_pNext;
			pTmp->m_pNext = pInfo;
		}
		Unlock();

		return pInfo;
	}
	BOOL HandleIq(int nIqId, XmlNode *pNode, void *pUserData)
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
					pInfo->m_szChildTagName = a2t( pInfo->m_pChildNode->name );
				if (pInfo->m_pChildNode && (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_CHILD_TAG_XMLNS))
					pInfo->m_szChildTagXmlns = JabberXmlGetAttrValue( pInfo->m_pChildNode, "xmlns" );
			}

			if (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_TO)
				pInfo->m_szTo = JabberXmlGetAttrValue( pNode, "to" );

			if (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_FROM)
				pInfo->m_szFrom = JabberXmlGetAttrValue( pNode, "from" );
			if (pInfo->m_szFrom && (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_HCONTACT))
				pInfo->m_hContact = JabberHContactFromJID( pInfo->m_szFrom, 3 );

			if (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_ID_STR)
				pInfo->m_szId = JabberXmlGetAttrValue( pNode, "id" );

			pInfo->m_pHandler(pNode, pUserData, pInfo);
			mir_free( pInfo->m_szChildTagName );
			delete pInfo;
			return TRUE;
		}
		return FALSE;
	}
	BOOL HandleIqPermanent(XmlNode *pNode, void *pUserData)
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
		
		TCHAR *szTagName = a2t( pFirstChild->name );
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
					iqInfo.m_hContact = JabberHContactFromJID( iqInfo.m_szFrom, 3 );

				JabberLog( "Handling iq id %S, type %S, from %S", iqInfo.m_szId, szType, iqInfo.m_szFrom );
				pInfo->m_pHandler(pNode, pUserData, &iqInfo);
				bHandled = TRUE;
			}
			pInfo = pInfo->m_pNext;
		}
		Unlock();

		mir_free( szTagName );
		return bHandled;
	}
	BOOL ExpireIq(int nIqId, void *pUserData = NULL)
	{
		Lock();
		CJabberIqInfo* pInfo = DetachInfo(nIqId, 0);
		Unlock();
		if (pInfo)
		{
			ExpireInfo(pInfo, pUserData);
			delete pInfo;
			return TRUE;
		}
		return FALSE;
	}
	BOOL ExpireGroup(DWORD dwGroupId, void *pUserData = NULL)
	{
		BOOL bRetVal = FALSE;
		while (1)
		{
			Lock();
			CJabberIqInfo* pInfo = DetachInfo(-1, dwGroupId);
			Unlock();
			if (!pInfo)
				break;
			ExpireInfo(pInfo, pUserData);
			delete pInfo;
			bRetVal = TRUE;
		}
		return bRetVal;
	}
	BOOL ExpireByUserData(void *pUserData)
	{
		BOOL bRetVal = FALSE;
		while (1)
		{
			Lock();
			CJabberIqInfo* pInfo = DetachInfo(pUserData);
			Unlock();
			if (!pInfo)
				break;
			ExpireInfo(pInfo, NULL);
			delete pInfo;
			bRetVal = TRUE;
		}
		return bRetVal;
	}
	BOOL ExpireAll(void *pUserData = NULL)
	{
		while (1)
		{
			Lock();
			CJabberIqInfo* pInfo = m_pIqs;
			if (pInfo)
				m_pIqs = m_pIqs->m_pNext;
			Unlock();
			if (!pInfo)
				break;
			pInfo->m_pNext = NULL;
			ExpireInfo(pInfo, pUserData);
			delete pInfo;
		}
		return TRUE;
	}
	BOOL FillPermanentHandlers();
};


extern CJabberIqManager g_JabberIqManager;

#endif
