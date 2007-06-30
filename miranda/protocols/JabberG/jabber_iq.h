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


class CJabberIqRequestInfo;


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
JABBER_IQ_PFUNC JabberIqFetchXmlnsFunc( TCHAR* xmlns );

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
void JabberIqResultEntityTime( XmlNode *iqNode, void *userdata, CJabberIqRequestInfo *pInfo );
void JabberIqResultLastActivity( XmlNode *iqNode, void *userdata, CJabberIqRequestInfo *pInfo );

void JabberSetBookmarkRequest (XmlNodeIq& iqId);


unsigned int  __stdcall JabberSerialNext( void );
HANDLE        __stdcall JabberHContactFromJID( const TCHAR* jid );
void __stdcall replaceStr( WCHAR*& dest, const WCHAR* src );
void          __stdcall JabberLog( const char* fmt, ... );

// 2 minutes, milliseconds
#define JABBER_DEFAULT_IQ_REQUEST_TIMEOUT		120000

class CJabberIqRequestManager;

typedef void ( *JABBER_IQ_HANDLER )( XmlNode *iqNode, void *usedata, CJabberIqRequestInfo *pInfo );

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

#define JABBER_IQ_PARSE_DEFAULT					(JABBER_IQ_PARSE_CHILD_TAG_NODE|JABBER_IQ_PARSE_CHILD_TAG_NAME|JABBER_IQ_PARSE_CHILD_TAG_XMLNS)

class CJabberIqRequestInfo
{
protected:
	friend class CJabberIqRequestManager;
	JABBER_IQ_HANDLER m_pHandler;
	CJabberIqRequestInfo *m_pNext;
	
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
	char *m_szChildTagName;
	XmlNode *m_pChildNode;
	HANDLE m_hContact; 
	TCHAR *m_szTo;
public:
	CJabberIqRequestInfo()
	{
		ZeroMemory(this, sizeof(CJabberIqRequestInfo));
	}
	~CJabberIqRequestInfo()
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

class CJabberIqRequestManager
{
protected:
	CRITICAL_SECTION m_cs;
	DWORD m_dwLastUsedHandle;
	CJabberIqRequestInfo *m_pIqs;
	HANDLE m_hExpirerThread;
	BOOL m_bExpirerThreadShutdownRequest;

	CJabberIqRequestInfo* DetachInfo(int nIqId, DWORD dwGroupId)
	{
		if (!m_pIqs)
			return NULL;

		CJabberIqRequestInfo *pInfo = m_pIqs;
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
				CJabberIqRequestInfo *pRetVal = pInfo->m_pNext;
				pInfo->m_pNext = pInfo->m_pNext->m_pNext;
				pRetVal->m_pNext = NULL;
				return pRetVal;
			}
			pInfo = pInfo->m_pNext;
		}
		return NULL;
	}
	CJabberIqRequestInfo* DetachInfo(void *pUserData)
	{
		if (!m_pIqs)
			return NULL;

		CJabberIqRequestInfo *pInfo = m_pIqs;
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
				CJabberIqRequestInfo *pRetVal = pInfo->m_pNext;
				pInfo->m_pNext = pInfo->m_pNext->m_pNext;
				pRetVal->m_pNext = NULL;
				return pRetVal;
			}
			pInfo = pInfo->m_pNext;
		}
		return NULL;
	}
	CJabberIqRequestInfo* DetachExpired()
	{
		if (!m_pIqs)
			return NULL;

		DWORD dwCurrentTime = GetTickCount();

		CJabberIqRequestInfo *pInfo = m_pIqs;
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
				CJabberIqRequestInfo *pRetVal = pInfo->m_pNext;
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
		CJabberIqRequestManager *pManager = ( CJabberIqRequestManager * )pParam;
		pManager->ExpirerThread();
		if ( !pManager->m_bExpirerThreadShutdownRequest ) {
			CloseHandle( pManager->m_hExpirerThread );
			pManager->m_hExpirerThread = NULL;
		}
		return 0;
	}
	void ExpirerThread()
	{
		while (!m_bExpirerThreadShutdownRequest)
		{
			Lock();
			CJabberIqRequestInfo *pInfo = DetachExpired();
			Unlock();
			if (!pInfo)
			{
				for (int i = 0; !m_bExpirerThreadShutdownRequest && (i < 10); i++)
					Sleep(50);
				continue;
			}
			ExpireInfo(pInfo);
			delete pInfo;
		}
	}
	void ExpireInfo( CJabberIqRequestInfo *pInfo, void *pUserData = NULL )
	{
		if ( !pInfo )
			return;
		
		if ( pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_FROM )
			pInfo->m_szFrom = pInfo->m_szReceiver;
		if (( pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_HCONTACT ) && ( pInfo->m_szFrom ))
			pInfo->m_hContact = JabberHContactFromJID( pInfo->m_szFrom );

		JabberLog( "Expiring iq id %d, sent to %S", pInfo->m_nIqId, pInfo->m_szReceiver ? pInfo->m_szReceiver : _T("unknown") );

		pInfo->m_nIqType = JABBER_IQ_TYPE_FAIL;
		pInfo->m_pHandler( NULL, NULL, pInfo );
	}
	BOOL AppendIq(CJabberIqRequestInfo *pInfo)
	{
		Lock();
		if (!m_pIqs)
			m_pIqs = pInfo;
		else
		{
			CJabberIqRequestInfo *pTmp = m_pIqs;
			while (pTmp->m_pNext)
				pTmp = pTmp->m_pNext;
			pTmp->m_pNext = pInfo;
		}
		Unlock();
		return TRUE;
	}
public:
	CJabberIqRequestManager()
	{
		InitializeCriticalSection(&m_cs);
		m_dwLastUsedHandle = 0;
		m_pIqs = NULL;
		m_hExpirerThread = NULL;
	}
	~CJabberIqRequestManager()
	{
		ExpireAll();
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
		CJabberIqRequestInfo *pInfo = m_pIqs;
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
	CJabberIqRequestInfo* AddHandler(JABBER_IQ_HANDLER pHandler, int nIqType = JABBER_IQ_TYPE_GET, TCHAR *szReceiver = NULL, DWORD dwParamsToParse = 0, int nIqId = -1, void *pUserData = NULL, DWORD dwGroupId = 0, DWORD dwTimeout = JABBER_DEFAULT_IQ_REQUEST_TIMEOUT)
	{
		CJabberIqRequestInfo *pInfo = new CJabberIqRequestInfo();
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
	BOOL HandleIq(int nIqId, XmlNode *pNode, void *pUserData)
	{
		if (nIqId == -1 || pNode == NULL)
			return FALSE;

		Lock();
		CJabberIqRequestInfo *pInfo = DetachInfo(nIqId, 0);
		Unlock();
		if (pInfo)
		{
			TCHAR *szType = JabberXmlGetAttrValue(pNode, "type");
			pInfo->m_nIqType = JABBER_IQ_TYPE_FAIL;
			if (szType)
			{
				if (!_tcsicmp(szType, _T("result")))
					pInfo->m_nIqType = JABBER_IQ_TYPE_RESULT;
				else if (!_tcsicmp(szType, _T("error")))
					pInfo->m_nIqType = JABBER_IQ_TYPE_ERROR;
				else if (!_tcsicmp(szType, _T("get")))
					pInfo->m_nIqType = JABBER_IQ_TYPE_GET;
				else if (!_tcsicmp(szType, _T("set")))
					pInfo->m_nIqType = JABBER_IQ_TYPE_SET;
			}
			if ((pInfo->m_nIqType & JABBER_IQ_TYPE_GOOD) && (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_CHILD_TAG_NODE))
			{
				pInfo->m_pChildNode = JabberXmlGetFirstChild( pNode );
				if (!pInfo->m_pChildNode)
					pInfo->m_nIqType = JABBER_IQ_TYPE_FAIL;
			}
			if ((pInfo->m_nIqType & JABBER_IQ_TYPE_GOOD) && (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_CHILD_TAG_NAME))
				pInfo->m_szChildTagName = pInfo->m_pChildNode->name;
			if ((pInfo->m_nIqType && JABBER_IQ_TYPE_GOOD) && (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_CHILD_TAG_XMLNS))
				pInfo->m_szChildTagXmlns = JabberXmlGetAttrValue( pInfo->m_pChildNode, "xmlns" );
			
			if (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_TO)
				pInfo->m_szTo = JabberXmlGetAttrValue( pNode, "to" );

			if (pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_FROM)
				pInfo->m_szFrom = JabberXmlGetAttrValue( pNode, "from" );
			if ((pInfo->m_dwParamsToParse & JABBER_IQ_PARSE_HCONTACT) && (pInfo->m_szFrom))
				pInfo->m_hContact = JabberHContactFromJID( pInfo->m_szFrom );

			pInfo->m_pHandler(pNode, pUserData, pInfo);
			delete pInfo;
			return TRUE;
		}
		return FALSE;
	}
	BOOL ExpireIq(int nIqId, void *pUserData = NULL)
	{
		Lock();
		CJabberIqRequestInfo *pInfo = DetachInfo(nIqId, 0);
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
			CJabberIqRequestInfo *pInfo = DetachInfo(-1, dwGroupId);
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
			CJabberIqRequestInfo *pInfo = DetachInfo(pUserData);
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
			CJabberIqRequestInfo *pInfo = m_pIqs;
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
};


extern CJabberIqRequestManager g_JabberIqRequestManager;

#endif
