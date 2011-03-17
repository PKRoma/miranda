/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-11  George Hazan
Copyright ( C ) 2005-07  Maxim Mluhov

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

Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#ifndef _JABBER_DISCO_H_
#define _JABBER_DISCO_H_

#ifdef _UNICODE
	#define	CHR_BULLET	((WCHAR)0x2022)
//	#define	STR_BULLET	L" \u2022 "
#else
	#define	CHR_BULLET	'-'
#endif

#define JABBER_DISCO_RESULT_NOT_REQUESTED			0
#define JABBER_DISCO_RESULT_ERROR					-1
#define JABBER_DISCO_RESULT_OK						-2

class CJabberSDIdentity;
class CJabberSDIdentity
{
protected:
	TCHAR *m_szCategory;
	TCHAR *m_szType;
	TCHAR *m_szName;
	CJabberSDIdentity *m_pNext;
public:
	CJabberSDIdentity(const TCHAR *szCategory, const TCHAR *szType, const TCHAR *szName)
	{
		m_szCategory = mir_tstrdup(szCategory);
		m_szType = mir_tstrdup(szType);
		m_szName = mir_tstrdup(szName);
		m_pNext = NULL;
	}
	~CJabberSDIdentity()
	{
		mir_free(m_szCategory);
		mir_free(m_szType);
		mir_free(m_szName);
		if (m_pNext)
			delete m_pNext;
	}
	TCHAR* GetCategory()
	{
		return m_szCategory;
	}
	TCHAR* GetType()
	{
		return m_szType;
	}
	TCHAR* GetName()
	{
		return m_szName;
	}
	CJabberSDIdentity* GetNext()
	{
		return m_pNext;
	}
	CJabberSDIdentity* SetNext(CJabberSDIdentity *pNext)
	{
		CJabberSDIdentity *pRetVal = m_pNext;
		m_pNext = pNext;
		return pRetVal;
	}
};

class CJabberSDFeature;
class CJabberSDFeature
{
protected:
	TCHAR *m_szVar;
	CJabberSDFeature *m_pNext;
public:
	CJabberSDFeature(const TCHAR *szVar)
	{
		m_szVar = szVar ? mir_tstrdup(szVar) : NULL;
		m_pNext = NULL;
	}
	~CJabberSDFeature()
	{
		mir_free(m_szVar);
		if (m_pNext)
			delete m_pNext;
	}
	TCHAR* GetVar()
	{
		return m_szVar;
	}
	CJabberSDFeature* GetNext()
	{
		return m_pNext;
	}
	CJabberSDFeature* SetNext(CJabberSDFeature *pNext)
	{
		CJabberSDFeature *pRetVal = m_pNext;
		m_pNext = pNext;
		return pRetVal;
	}
};

class CJabberSDNode;
class CJabberSDNode
{
protected:
	TCHAR *m_szJid;
	TCHAR *m_szNode;
	TCHAR *m_szName;
	CJabberSDIdentity *m_pIdentities;
	CJabberSDFeature *m_pFeatures;
	CJabberSDNode *m_pNext;
	CJabberSDNode *m_pChild;
	DWORD m_dwInfoRequestTime;
	DWORD m_dwItemsRequestTime;
	int m_nInfoRequestId;
	int m_nItemsRequestId;
	HTREELISTITEM m_hTreeItem;
	TCHAR *m_szInfoError;
	TCHAR *m_szItemsError;
public:
	CJabberSDNode( const TCHAR *szJid = NULL, const TCHAR *szNode = NULL, const TCHAR *szName = NULL)
	{
		m_szJid = mir_tstrdup(szJid);
		m_szNode = mir_tstrdup(szNode);
		m_szName = mir_tstrdup(szName);
		m_pIdentities = NULL;
		m_pFeatures = NULL;
		m_pNext = NULL;
		m_pChild = NULL;
		m_dwInfoRequestTime = 0;
		m_dwItemsRequestTime = 0;
		m_nInfoRequestId = 0;
		m_nItemsRequestId = 0;
		m_hTreeItem = NULL;
		m_szInfoError = NULL;
		m_szItemsError = NULL;
	}
	~CJabberSDNode()
	{
		RemoveAll();
	}
	BOOL RemoveAll()
	{
		replaceStr( m_szJid, NULL );
		replaceStr( m_szNode, NULL );
		replaceStr( m_szName, NULL );
		replaceStr( m_szInfoError, NULL );
		replaceStr( m_szItemsError, NULL );
		if ( m_pIdentities )
			delete m_pIdentities;
		m_pIdentities = NULL;
		if ( m_pFeatures )
			delete m_pFeatures;
		m_pFeatures = NULL;
		if ( m_pNext )
			delete m_pNext;
		m_pNext = NULL;
		if ( m_pChild )
			delete m_pChild;
		m_pChild = NULL;
		m_nInfoRequestId = JABBER_DISCO_RESULT_NOT_REQUESTED;
		m_nItemsRequestId = JABBER_DISCO_RESULT_NOT_REQUESTED;
		m_dwInfoRequestTime = 0;
		m_dwItemsRequestTime = 0;
		m_hTreeItem = NULL;
		return TRUE;
	}
	BOOL ResetInfo()
	{
		replaceStr( m_szInfoError, NULL );
		replaceStr( m_szItemsError, NULL );
		if ( m_pIdentities )
			delete m_pIdentities;
		m_pIdentities = NULL;
		if ( m_pFeatures )
			delete m_pFeatures;
		m_pFeatures = NULL;
		if ( m_pChild )
			delete m_pChild;
		m_pChild = NULL;
		m_nInfoRequestId = JABBER_DISCO_RESULT_NOT_REQUESTED;
		m_nItemsRequestId = JABBER_DISCO_RESULT_NOT_REQUESTED;
		m_dwInfoRequestTime = 0;
		m_dwItemsRequestTime = 0;
		return TRUE;
	}
	BOOL SetTreeItemHandle(HTREELISTITEM hItem)
	{
		m_hTreeItem = hItem;
		return TRUE;
	}
	HTREELISTITEM GetTreeItemHandle()
	{
		return m_hTreeItem;
	}
	BOOL SetInfoRequestId(int nId)
	{
		m_nInfoRequestId = nId;
		m_dwInfoRequestTime = GetTickCount();
		return TRUE;
	}
	int GetInfoRequestId()
	{
		return m_nInfoRequestId;
	}
	BOOL SetItemsRequestId(int nId)
	{
		m_nItemsRequestId = nId;
		m_dwItemsRequestTime = GetTickCount();
		return TRUE;
	}
	int GetItemsRequestId()
	{
		return m_nItemsRequestId;
	}
	BOOL SetJid(TCHAR *szJid)
	{
		replaceStr(m_szJid, szJid);
		return TRUE;
	}
	TCHAR* GetJid()
	{
		return m_szJid;
	}
	BOOL SetNode(TCHAR *szNode)
	{
		replaceStr(m_szNode, szNode);
		return TRUE;
	}
	TCHAR* GetNode()
	{
		return m_szNode;
	}
	TCHAR* GetName()
	{
		return m_szName;
	}
	CJabberSDIdentity* GetFirstIdentity()
	{
		return m_pIdentities;
	}
	CJabberSDFeature* GetFirstFeature()
	{
		return m_pFeatures;
	}
	CJabberSDNode* GetFirstChildNode()
	{
		return m_pChild;
	}
	CJabberSDNode* GetNext()
	{
		return m_pNext;
	}
	CJabberSDNode* SetNext(CJabberSDNode *pNext)
	{
		CJabberSDNode *pRetVal = m_pNext;
		m_pNext = pNext;
		return pRetVal;
	}
	CJabberSDNode* FindByIqId(int nIqId, BOOL bInfoId = TRUE)
	{
		if (( m_nInfoRequestId == nIqId && bInfoId ) || ( m_nItemsRequestId == nIqId && !bInfoId ))
			return this;

		CJabberSDNode *pNode = NULL;
		if ( m_pChild && (pNode = m_pChild->FindByIqId( nIqId, bInfoId )))
			return pNode;

		CJabberSDNode *pTmpNode = NULL;
		pNode = m_pNext;
		while ( pNode ) {
			if (( pNode->m_nInfoRequestId == nIqId && bInfoId ) || ( pNode->m_nItemsRequestId == nIqId && !bInfoId ))
				return pNode;
			if ( pNode->m_pChild && (pTmpNode = pNode->m_pChild->FindByIqId( nIqId, bInfoId )))
				return pTmpNode;
			pNode = pNode->GetNext();
		}
		return NULL;
	}
	BOOL AddFeature(const TCHAR *szFeature)
	{
		if ( !szFeature )
			return FALSE;

		CJabberSDFeature *pFeature = new CJabberSDFeature( szFeature );
		if ( !pFeature )
			return FALSE;

		pFeature->SetNext( m_pFeatures );
		m_pFeatures = pFeature;

		return TRUE;
	}
	BOOL AddIdentity(const TCHAR *szCategory, const TCHAR *szType, const TCHAR *szName)
	{
		if ( !szCategory || !szType )
			return FALSE;

		CJabberSDIdentity *pIdentity = new CJabberSDIdentity( szCategory, szType, szName );
		if ( !pIdentity )
			return FALSE;

		pIdentity->SetNext( m_pIdentities );
		m_pIdentities = pIdentity;

		return TRUE;
	}
	BOOL AddChildNode(const TCHAR *szJid, const TCHAR *szNode, const TCHAR *szName)
	{
		if ( !szJid )
			return FALSE;

		CJabberSDNode *pNode = new CJabberSDNode( szJid, szNode, szName );
		if ( !pNode )
			return FALSE;

		pNode->SetNext( m_pChild );
		m_pChild = pNode;

		return TRUE;
	}
	BOOL AppendString(TCHAR **ppBuffer, TCHAR *szString)
	{
		if ( !*ppBuffer ) {
			*ppBuffer = mir_tstrdup( szString );
			return TRUE;
		}

		*ppBuffer = (TCHAR *)mir_realloc( *ppBuffer,  (_tcslen( *ppBuffer) + _tcslen(szString) + 1 ) * sizeof( TCHAR ));
		_tcscat(*ppBuffer, szString);

		return TRUE;
	}
	BOOL SetItemsRequestErrorText(TCHAR *szError)
	{
		replaceStr(m_szItemsError, szError);
		return TRUE;
	}
	BOOL SetInfoRequestErrorText(TCHAR *szError)
	{
		replaceStr(m_szInfoError, szError);
		return TRUE;
	}
	BOOL GetTooltipText(TCHAR *szText, int nMaxLength)
	{
		TCHAR *szBuffer = NULL;

		TCHAR szTmp[ 8192 ];

		mir_sntprintf( szTmp, SIZEOF( szTmp ), _T("Jid: %s\r\n"), m_szJid );
		AppendString( &szBuffer, szTmp );

		if ( m_szNode ) {
			mir_sntprintf( szTmp, SIZEOF( szTmp ), _T("%s: %s\r\n"), TranslateT("Node"), m_szNode );
			AppendString( &szBuffer, szTmp );
		}

		if ( m_pIdentities ) {
			mir_sntprintf( szTmp, SIZEOF( szTmp ), _T("\r\n%s:\r\n"), TranslateT("Identities"));
			AppendString( &szBuffer, szTmp );

			CJabberSDIdentity *pIdentity = m_pIdentities;
			while ( pIdentity ) {
				if ( pIdentity->GetName() )
					mir_sntprintf( szTmp, SIZEOF( szTmp ), _T(" %c %s (%s: %s, %s: %s)\r\n"),
						CHR_BULLET, pIdentity->GetName(),
							TranslateT("category"), pIdentity->GetCategory(),
							TranslateT("type"), pIdentity->GetType() );
				else
					mir_sntprintf( szTmp, SIZEOF( szTmp ), _T(" %c %s: %s, %s: %s\r\n"),
						CHR_BULLET,
						TranslateT("Category"), pIdentity->GetCategory(),
						TranslateT("Type"), pIdentity->GetType() );

				AppendString( &szBuffer, szTmp );

				pIdentity = pIdentity->GetNext();
			}
		}

		if ( m_pFeatures ) {
			mir_sntprintf( szTmp, SIZEOF( szTmp ), _T("\r\n%s:\r\n"), TranslateT("Supported features"));
			AppendString( &szBuffer, szTmp );

			CJabberSDFeature *pFeature = m_pFeatures;
			while ( pFeature ) {
				mir_sntprintf( szTmp, SIZEOF( szTmp ), _T(" %c %s\r\n"), CHR_BULLET, pFeature->GetVar() );

				AppendString( &szBuffer, szTmp );

				pFeature = pFeature->GetNext();
			}
		}

		if ( m_szInfoError ) {
			mir_sntprintf( szTmp, SIZEOF( szTmp ), _T("\r\n%s: %s\r\n"), TranslateT("Info request error"), m_szInfoError );
			AppendString( &szBuffer, szTmp );
		}

		if ( m_szItemsError ) {
			mir_sntprintf( szTmp, SIZEOF( szTmp ), _T("\r\n%s: %s\r\n"), TranslateT("Items request error"), m_szItemsError );
			AppendString( &szBuffer, szTmp );
		}

		szBuffer[lstrlen(szBuffer)-2] = 0; // remove CR/LF
		mir_sntprintf( szText, nMaxLength, _T("%s"), szBuffer );

		mir_free( szBuffer );

		return TRUE;
	}
};

class CJabberSDManager
{
protected:
	CRITICAL_SECTION m_cs;
	CJabberSDNode *m_pPrimaryNodes;
public:
	CJabberSDManager()
	{
		m_pPrimaryNodes = NULL;
		InitializeCriticalSection(&m_cs);
	}
	~CJabberSDManager()
	{
		DeleteCriticalSection(&m_cs);
		RemoveAll();
	}
	void RemoveAll()
	{
		delete m_pPrimaryNodes;
		m_pPrimaryNodes = NULL;
	}
	BOOL Lock()
	{
		EnterCriticalSection(&m_cs);
		return TRUE;
	}
	BOOL Unlock()
	{
		LeaveCriticalSection(&m_cs);
		return TRUE;
	}
	CJabberSDNode* GetPrimaryNode()
	{
		return m_pPrimaryNodes;
	}
	CJabberSDNode* AddPrimaryNode(const TCHAR *szJid, const TCHAR *szNode, const TCHAR *szName)
	{
		if ( !szJid )
			return FALSE;

		CJabberSDNode *pNode = new CJabberSDNode( szJid, szNode, szName );
		if ( !pNode )
			return NULL;

		pNode->SetNext( m_pPrimaryNodes );
		m_pPrimaryNodes = pNode;

		return pNode;
	}
	CJabberSDNode* FindByIqId(int nIqId, BOOL bInfoId = TRUE)
	{
		CJabberSDNode *pNode = NULL;
		CJabberSDNode *pTmpNode = NULL;
		pNode = m_pPrimaryNodes;
		while ( pNode ) {
			if ( pTmpNode = pNode->FindByIqId( nIqId, bInfoId ) )
				return pTmpNode;
			pNode = pNode->GetNext();
		}
		return NULL;
	}
};

#undef STR_BULLET // used for formatting

#endif // _JABBER_DISCO_H_
