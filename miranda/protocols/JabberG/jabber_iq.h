/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-09  George Hazan
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

struct CJabberProto;
typedef void ( CJabberProto::*JABBER_IQ_PFUNC )( HXML iqNode );

typedef struct {
	TCHAR* xmlns;
	JABBER_IQ_PFUNC func;
	BOOL allowSubNs;		// e.g. #info in disco#info
} JABBER_IQ_XMLNS_FUNC;

void  __stdcall replaceStr( char*& dest, const char* src );
void  __stdcall replaceStr( WCHAR*& dest, const WCHAR* src );

// 2 minutes, milliseconds
#define JABBER_DEFAULT_IQ_REQUEST_TIMEOUT		120000

class CJabberIqRequestManager;

typedef void ( CJabberProto::*JABBER_IQ_HANDLER )( HXML iqNode, /*void *usedata,*/ CJabberIqInfo* pInfo );

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
	HXML   m_pChildNode;
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
	void SetReceiver(const TCHAR *szReceiver)
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
	HXML GetChildNode()
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
	CJabberProto* ppro;
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
	void ExpireInfo( CJabberIqInfo* pInfo, void *pUserData = NULL );
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
	CJabberIqManager( CJabberProto* proto )
	{
		InitializeCriticalSection(&m_cs);
		m_dwLastUsedHandle = 0;
		m_pIqs = NULL;
		m_hExpirerThread = NULL;
		m_pPermanentHandlers = NULL;
		ppro = proto;
	}
	~CJabberIqManager()
	{
		ExpireAll();
		if ( m_pPermanentHandlers )
			delete m_pPermanentHandlers;
		DeleteCriticalSection(&m_cs);
	}
	BOOL Start();
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
	CJabberIqInfo* AddHandler(JABBER_IQ_HANDLER pHandler, int nIqType = JABBER_IQ_TYPE_GET, const TCHAR *szReceiver = NULL, DWORD dwParamsToParse = 0, int nIqId = -1, void *pUserData = NULL, DWORD dwGroupId = 0, DWORD dwTimeout = JABBER_DEFAULT_IQ_REQUEST_TIMEOUT);
	CJabberIqPermanentInfo* AddPermanentHandler(JABBER_IQ_HANDLER pHandler, int nIqTypes, DWORD dwParamsToParse, const TCHAR* szXmlns, BOOL bAllowPartialNs, const TCHAR* szTag)
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
	BOOL HandleIq(int nIqId, HXML pNode);
	BOOL HandleIqPermanent(HXML pNode);
	BOOL ExpireIq(int nIqId)
	{
		Lock();
		CJabberIqInfo* pInfo = DetachInfo(nIqId, 0);
		Unlock();
		if (pInfo)
		{
			ExpireInfo(pInfo);
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
	void ExpirerThread( void );
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

#endif
