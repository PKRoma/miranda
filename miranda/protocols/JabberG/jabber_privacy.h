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

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_privacy.h,v $
Revision       : $Revision: 5568 $
Last change on : $Date: 2007-06-02 00:01:00 +0300 (бс, 02 шўэ 2007) $
Last change by : $Author: ghazan $

*/

#ifndef _JABBER_PRIVACY_H_
#define _JABBER_PRIVACY_H_

void JabberProcessIqPrivacyLists( XmlNode* iqNode, void* userdata, CJabberIqInfo* pInfo );
void JabberIqResultPrivacyLists( XmlNode* iqNode, void* userdata, CJabberIqInfo *pInfo );
int JabberMenuHandlePrivacyLists( WPARAM wParam, LPARAM lParam );

#define JABBER_PL_RULE_TYPE_MESSAGE			1
#define JABBER_PL_RULE_TYPE_PRESENCE_IN		2
#define JABBER_PL_RULE_TYPE_PRESENCE_OUT	4
#define JABBER_PL_RULE_TYPE_IQ				8
#define JABBER_PL_RULE_TYPE_ALL				0x0F

enum PrivacyListRuleType
{
	Jid,
	Group,
	Subscription,
	Else
};

struct CPrivacyListModifyUserParam
{
	BOOL m_bAllOk;
	CPrivacyListModifyUserParam()
	{
		ZeroMemory(this, sizeof(CPrivacyListModifyUserParam));
	}
};

class CPrivacyList;
class CPrivacyListRule;
class CPrivacyListRule
{
protected:
	friend class CPrivacyList;
public:
	CPrivacyListRule(PrivacyListRuleType type = Else, TCHAR *szValue = _T(""), BOOL bAction = TRUE, DWORD dwOrder = 90, DWORD dwPackets = 0)
	{
		m_szValue = mir_tstrdup(szValue);
		m_nType = type;
		m_bAction = bAction;
		m_dwOrder = dwOrder;
		m_dwPackets = dwPackets;
		m_pNext = NULL;
	};
	~CPrivacyListRule()
	{
		if (m_szValue)
			mir_free(m_szValue);

		if (m_pNext)
			delete m_pNext;
	};
	CPrivacyListRule* GetNext()
	{
		return m_pNext;
	}
	CPrivacyListRule* SetNext(CPrivacyListRule *pNext)
	{
		CPrivacyListRule *pRetVal = m_pNext;
		m_pNext = pNext;
		return pRetVal;
	}
	DWORD GetOrder()
	{
		return m_dwOrder;
	}
	DWORD SetOrder(DWORD dwOrder)
	{
		DWORD dwRetVal = m_dwOrder;
		m_dwOrder = dwOrder;
		return dwRetVal;
	}
	PrivacyListRuleType GetType()
	{
		return m_nType;
	}
	BOOL SetType( PrivacyListRuleType type )
	{
		m_nType = type;
		return TRUE;
	}
	TCHAR* GetValue()
	{
		return m_szValue;
	}
	BOOL SetValue( TCHAR *szValue )
	{
		replaceStr( m_szValue, szValue );
		return TRUE;
	}
	DWORD GetPackets()
	{
		return m_dwPackets;
	}
	BOOL SetPackets( DWORD dwPackets )
	{
		m_dwPackets = dwPackets;
		return TRUE;
	}
	BOOL GetAction()
	{
		return m_bAction;
	}
	BOOL SetAction( BOOL bAction )
	{
		m_bAction = bAction;
		return TRUE;
	}
protected:
	PrivacyListRuleType m_nType;
	TCHAR *m_szValue;
	BOOL m_bAction;
	DWORD m_dwOrder;
	DWORD m_dwPackets;
	CPrivacyListRule *m_pNext;
};

class CPrivacyList;
class CPrivacyList
{
protected:
	CPrivacyListRule *m_pRules;
	TCHAR *m_szListName;
	CPrivacyList *m_pNext;
	BOOL m_bLoaded;
	BOOL m_bModified;
	BOOL m_bDeleted;
public:
	CPrivacyList(TCHAR *szListName)
	{
		m_szListName = mir_tstrdup(szListName);
		m_pRules = NULL;
		m_pNext = NULL;
		m_bLoaded = FALSE;
		m_bModified = FALSE;
		m_bDeleted = FALSE;
	};
	~CPrivacyList()
	{
		if (m_szListName)
			mir_free(m_szListName);
		RemoveAllRules();
		if (m_pNext)
			delete m_pNext;
	};
	BOOL RemoveAllRules()
	{
		if (m_pRules)
			delete m_pRules;
		m_pRules = NULL;
		return TRUE;
	}
	TCHAR* GetListName()
	{
		return m_szListName;
	}
	CPrivacyListRule* GetFirstRule()
	{
		return m_pRules;
	}
	CPrivacyList* GetNext()
	{
		return m_pNext;
	}
	CPrivacyList* SetNext(CPrivacyList *pNext)
	{
		CPrivacyList *pRetVal = m_pNext;
		m_pNext = pNext;
		return pRetVal;
	}
	BOOL AddRule(PrivacyListRuleType type, TCHAR *szValue, BOOL bAction, DWORD dwOrder, DWORD dwPackets)
	{
		CPrivacyListRule *pRule = new CPrivacyListRule( type, szValue, bAction, dwOrder, dwPackets );
		if ( !pRule )
			return FALSE;
		pRule->SetNext( m_pRules );
		m_pRules = pRule;
		return TRUE;
	}
	BOOL AddRule(CPrivacyListRule *pRule)
	{
		pRule->SetNext( m_pRules );
		m_pRules = pRule;
		return TRUE;
	}
	BOOL RemoveRule(CPrivacyListRule *pRuleToRemove)
	{
		if ( !m_pRules )
			return FALSE;
		
		if ( m_pRules == pRuleToRemove ) {
			m_pRules = m_pRules->GetNext();
			pRuleToRemove->SetNext( NULL );
			delete pRuleToRemove;
			return TRUE;
		}

		CPrivacyListRule *pRule = m_pRules;
		while ( pRule->GetNext() ) {
			if ( pRule->GetNext() == pRuleToRemove ) {
				pRule->SetNext( pRule->GetNext()->GetNext() );
				pRuleToRemove->SetNext( NULL );
				delete pRuleToRemove;
				return TRUE;
			}
			pRule = pRule->GetNext();
		}
		return FALSE;
	}
	BOOL Reorder()
	{
		// 0 or 1 rules?
		if ( !m_pRules )
			return TRUE;
		if ( !m_pRules->GetNext() ) {
			m_pRules->SetOrder( 100 );
			return TRUE;
		}

		// get rules count
		DWORD dwCount = 0;
		CPrivacyListRule *pRule = m_pRules;
		while ( pRule ) {
			dwCount++;
			pRule = pRule->GetNext();
		}
		
		// create pointer array for sort procedure
		CPrivacyListRule **pRules = ( CPrivacyListRule ** )mir_alloc( dwCount * sizeof( CPrivacyListRule * ));
		if ( !pRules )
			return FALSE;
		DWORD dwPos = 0;
		pRule = m_pRules;
		while ( pRule ) {
			pRules[dwPos++] = pRule;
			pRule = pRule->GetNext();
		}
		
		// sort array of pointers, slow, but working :)
		DWORD i, j;
		CPrivacyListRule *pTmp;
		for ( i = 0; i < dwCount; i++ ) {
			for ( j = dwCount - 1; j > i; j-- ) {
				if ( pRules[j - 1]->GetOrder() > pRules[j]->GetOrder() ) {
					pTmp = pRules[j - 1];
					pRules[j - 1] = pRules[j];
					pRules[j] = pTmp;
				}
			}
		}

		// reorder linked list
		DWORD dwOrder = 100;
		CPrivacyListRule **ppPtr = &m_pRules;
		for ( i = 0; i < dwCount; i++ ) {
			*ppPtr = pRules[ i ];
			ppPtr = &pRules[ i ]->m_pNext;
			pRules[ i ]->SetOrder( dwOrder );
			dwOrder += 10;
		}
		*ppPtr = NULL;
		mir_free( pRules );

		return TRUE;
	}
	void SetLoaded(BOOL bLoaded = TRUE)
	{
		m_bLoaded = bLoaded;
	}
	BOOL IsLoaded()
	{
		return m_bLoaded;
	}
	void SetModified(BOOL bModified = TRUE)
	{
		m_bModified = bModified;
	}
	BOOL IsModified()
	{
		return m_bModified;
	}
	void SetDeleted(BOOL bDeleted = TRUE)
	{
		m_bDeleted = bDeleted;
	}
	BOOL IsDeleted()
	{
		return m_bDeleted;
	}
};

class CPrivacyListManager
{
protected:
	TCHAR *m_szActiveListName;
	TCHAR *m_szDefaultListName;
	CPrivacyList *m_pLists;
	CRITICAL_SECTION m_cs;
	BOOL m_bModified;

public:
	CPrivacyListManager()
	{
		InitializeCriticalSection(&m_cs);
		m_szActiveListName = NULL;
		m_szDefaultListName = NULL;
		m_bModified = FALSE;
	};
	~CPrivacyListManager()
	{
		if (m_szActiveListName)
			mir_free(m_szActiveListName);
		if (m_szDefaultListName)
			mir_free(m_szDefaultListName);
		RemoveAllLists();
		DeleteCriticalSection(&m_cs);
	};
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
	void SetActiveListName(TCHAR *szListName)
	{
		replaceStr(m_szActiveListName, szListName);
	}
	void SetDefaultListName(TCHAR *szListName)
	{
		replaceStr(m_szDefaultListName, szListName);
	}
	TCHAR* GetDefaultListName()
	{
		return m_szDefaultListName;
	}
	TCHAR* GetActiveListName()
	{
		return m_szActiveListName;
	}
	BOOL RemoveAllLists()
	{
		if (m_pLists)
			delete m_pLists;
		m_pLists = NULL;
		return TRUE;
	}
	CPrivacyList* FindList( TCHAR *szListName )
	{
		CPrivacyList *pList = m_pLists;
		while ( pList ) {
			if ( !_tcscmp(pList->GetListName(), szListName ))
				return pList;
			pList = pList->GetNext();
		}
		return NULL;
	}
	CPrivacyList* GetFirstList()
	{
		return m_pLists;
	}
	BOOL AddList( TCHAR *szListName )
	{
		if (FindList(szListName))
			return FALSE;
		CPrivacyList *pList = new CPrivacyList( szListName );
		pList->SetNext( m_pLists );
		m_pLists = pList;
		return TRUE;
	}
	BOOL SetModified(BOOL bModified = TRUE)
	{
		m_bModified = bModified;
		return TRUE;
	}
	BOOL IsModified()
	{
		if ( m_bModified )
			return TRUE;
		CPrivacyList *pList = m_pLists;
		while ( pList ) {
			if ( pList->IsModified() )
				return TRUE;
			pList = pList->GetNext();
		}
		return FALSE;
	}
	BOOL IsAllListsLoaded()
	{
		CPrivacyList *pList = m_pLists;
		while ( pList ) {
			if ( !pList->IsLoaded() )
				return FALSE;
			pList = pList->GetNext();
		}
		return TRUE;
	}

	void QueryLists()
	{
		XmlNodeIq iq( g_JabberIqManager.AddHandler( JabberIqResultPrivacyLists ) );
		XmlNode* query = iq.addQuery( JABBER_FEAT_PRIVACY_LISTS );
		jabberThreadInfo->send( iq );
	}
};

extern CPrivacyListManager g_PrivacyListManager;

#endif //_JABBER_PRIVACY_H_
