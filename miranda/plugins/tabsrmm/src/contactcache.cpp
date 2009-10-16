/*
 * astyle --force-indent=tab=4 --brackets=linux --indent-switches
 *		  --pad=oper --one-line=keep-blocks  --unpad=paren
 *
 * Miranda IM: the free IM client for Microsoft* Windows*
 *
 * Copyright 2000-2009 Miranda ICQ/IM project,
 * all portions of this codebase are copyrighted to the people
 * listed in contributors.txt.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * you should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * part of tabSRMM messaging plugin for Miranda.
 *
 * (C) 2005-2009 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 * contact cache implementation
 *
 * the contact cache provides various services to the message window(s)
 * it also abstracts meta contacts.
 *
 */

#include "commonheaders.h"


CContactCache::CContactCache(const HANDLE hContact)
{
	ZeroMemory(this, sizeof(CContactCache));

	m_Valid = m_isMeta = false;
	m_hContact = hContact;
	m_wOldStatus = m_wStatus = m_wMetaStatus = ID_STATUS_OFFLINE;

	if(hContact) {
		m_szProto = reinterpret_cast<char *>(::CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)m_hContact, 0));
#if defined(_UNICODE)
		if(m_szProto)
			m_tszProto = mir_a2t(m_szProto);
#else
		if(m_szProto)
			m_tszProto = m_szProto;
#endif
		initPhaseTwo();
	}
	else {
		m_szProto = C_INVALID_PROTO;
		m_tszProto = C_INVALID_PROTO_T;
		m_szAccount = C_INVALID_ACCOUNT;
		m_isMeta = false;
		m_Valid = false;
	}
}

/**
 * 2nd part of the object initialization that must be callable during the
 * object's lifetime (not only on construction).
 */
void CContactCache::initPhaseTwo()
{
	PROTOACCOUNT*	acc = 0;

	m_szAccount = 0;
	if(m_szProto) {
		acc = reinterpret_cast<PROTOACCOUNT *>(::CallService(MS_PROTO_GETACCOUNT, (WPARAM)0, (LPARAM)m_szProto));
		if(acc && acc->tszAccountName)
			m_szAccount = acc->tszAccountName;
	}

	m_Valid = (m_szProto != 0 && m_szAccount != 0) ? true : false;
	if(m_Valid) {
		m_isSubcontact = (M->GetByte(m_hContact, PluginConfig.szMetaName, "IsSubcontact", 0) ? true : false);
		m_isMeta = (PluginConfig.bMetaEnabled && !strcmp(m_szProto, PluginConfig.szMetaName)) ? true : false;
		if(m_isMeta) {
			/*
			int 	i = 0;
			char 	buf[20];
			HANDLE	h;
			do {
				mir_snprintf(buf, 20, "Handle%d", i);
				if((h = reinterpret_cast<HANDLE>(M->GetDword(m_hContact, m_szProto, buf, 0))) == 0)
					break;
				CContactCache* _c = CGlobals::getContactCache(h);
				if(_c)
					_c->setMasterContact(m_hContact);
				i++;
			} while(true);
			*/
			updateMeta();
		}
		updateState();
		updateFavorite();
	}
	else {
		m_szProto = C_INVALID_PROTO;
		m_tszProto = C_INVALID_PROTO_T;
		m_szAccount = C_INVALID_ACCOUNT;
		m_isMeta = false;
	}
}

/**
 * reset meta contact information. Used when meta contacts are disabled
 * on user's request.
 */
void CContactCache::resetMeta()
{
	m_isMeta = false;
	m_szMetaProto = 0;
	m_hSubContact = 0;
	m_tszMetaProto[0] = 0;
	initPhaseTwo();
}

/**
 * if the contact has an open message window, close it.
 * window procedure will use setWindowData() to reset m_hwnd to 0.
 */
void CContactCache::closeWindow()
{
	if(m_hwnd)
		::SendMessage(m_hwnd, WM_CLOSE, 1, 2);
}

void CContactCache::updateState()
{
	updateNick();
	updateStatus();
}

/**
 * update private copy of the nick name. Use contact list name cache
 *
 * @return bool: true if nick has changed.
 */
bool CContactCache::updateNick()
{
	bool	fChanged = false;

	if(m_Valid) {
		TCHAR	*tszNick = reinterpret_cast<TCHAR *>(::CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)m_hContact, GCDNF_TCHAR));
		if(tszNick)
			fChanged = (_tcscmp(m_szNick, tszNick) ? true : false);
		mir_sntprintf(m_szNick, 80, _T("%s"), tszNick ? tszNick : _T("<undef>"));
	}
	return(fChanged);
}

/**
 * update status mode
 * @return	bool: true if status mode has changed, false if not.
 */
bool CContactCache::updateStatus()
{
	if(m_Valid) {
		m_wOldStatus = m_wStatus;
		m_wStatus = (WORD)DBGetContactSettingWord(m_hContact, m_szProto, "Status", ID_STATUS_OFFLINE);

		return(m_wOldStatus != m_wStatus);
	}
	else
		return(false);
}

/**
 * update meta (subcontact and -protocol) status. This runs when the
 * MC protocol fires one of its events OR when a relevant database value changes
 * in the master contact.
 */
void CContactCache::updateMeta()
{
	if(m_Valid) {
		HANDLE hSubContact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, (WPARAM)m_hContact, 0);
		if (hSubContact && hSubContact != m_hSubContact) {
			m_hSubContact = hSubContact;
			m_szMetaProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)m_hSubContact, 0);
			if (m_szMetaProto) {
				PROTOACCOUNT *acc = reinterpret_cast<PROTOACCOUNT *>(::CallService(MS_PROTO_GETACCOUNT, (WPARAM)0, (LPARAM)m_szMetaProto));
				if(acc && acc->tszAccountName)
					m_szAccount = acc->tszAccountName;
				m_wMetaStatus = DBGetContactSettingWord(m_hSubContact, m_szMetaProto, "Status", ID_STATUS_OFFLINE);
#if defined(_UNICODE)
				MultiByteToWideChar(CP_ACP, 0, m_szMetaProto, -1, m_tszMetaProto, 40);
#else
				strncpy(m_tszMetaProto, m_szMetaProto, 40);
#endif
				m_tszMetaProto[39] = 0;
			}
			else {
				m_wMetaStatus = ID_STATUS_OFFLINE;
				m_tszMetaProto[0] = 0;
			}
		}
	}
}

/**
 * obtain the UIN. This is only maintained for open message windows
 * it also run when the subcontact for a MC changes.
 */
void CContactCache::updateUIN()
{
	if(m_Valid) {
		CONTACTINFO ci;
		ZeroMemory((void *)&ci, sizeof(ci));

		ci.hContact = getActiveContact();
		ci.szProto = const_cast<char *>(getActiveProto());
		ci.cbSize = sizeof(ci);

		ci.dwFlag = CNF_DISPLAYUID | CNF_TCHAR;
		if (!CallService(MS_CONTACT_GETCONTACTINFO, 0, (LPARAM) & ci)) {
			switch (ci.type) {
				case CNFT_ASCIIZ:
					mir_sntprintf(m_szUIN, safe_sizeof(m_szUIN), _T("%s"), reinterpret_cast<TCHAR *>(ci.pszVal));
					mir_free((void *)ci.pszVal);
					break;
				case CNFT_DWORD:
					mir_sntprintf(m_szUIN, safe_sizeof(m_szUIN), _T("%u"), ci.dVal);
					break;
				default:
					m_szUIN[0] = 0;
					break;
			}
		} else
			m_szUIN[0] = 0;
	}
	else
		m_szUIN[0] = 0;
}

void CContactCache::updateStats(int iType, size_t value)
{
	if(m_stats == 0)
		allocStats();

	switch(iType) {
		case TSessionStats::UPDATE_WITH_LAST_RCV:
			if(m_stats->lastReceivedChars) {
				m_stats->iReceived++;
				m_stats->messageCount++;
			}
			else
				return;
			m_stats->iReceivedBytes += m_stats->lastReceivedChars;
			m_stats->lastReceivedChars = 0;
			break;
		case TSessionStats::INIT_TIMER:
			m_stats->started = time(0);
			return;
		case TSessionStats::SET_LAST_RCV:
			m_stats->lastReceivedChars = (unsigned int)value;
			return;
		case TSessionStats::BYTES_SENT:
			m_stats->iSent++;
			m_stats->messageCount++;
			m_stats->iSentBytes += (unsigned int)value;
			break;
	}
	//_DebugTraceW(_T("%s stats dump: %d (%d bytes) in, %d (%d bytes) out, total count: %d"), m_szNick, m_stats->iReceived, m_stats->iReceivedBytes,
	//			 m_stats->iSent, m_stats->iSentBytes, m_stats->messageCount);
}

void CContactCache::allocStats()
{
	if(m_stats == 0) {
		m_stats = new TSessionStats;
		::ZeroMemory(m_stats, sizeof(TSessionStats));
	}
}

/**
 * set the window data for this contact. The window procedure of the message
 * dialog will use this in WM_INITDIALOG and WM_DESTROY to tell the cache
 * that a message window is open for this contact.
 *
 * @param hwnd:		window handle
 * @param dat:		_MessageWindowData - window data structure
 */
void CContactCache::setWindowData(const HWND hwnd, _MessageWindowData *dat)
{
	m_hwnd = hwnd;
	m_dat = dat;
	if(hwnd && dat && m_history == 0)
		allocHistory();
	if(hwnd)
		updateStatusMsg();
	else {
		/* release memory - not needed when window isn't open */
		if(m_szStatusMsg)
			mir_free(m_szStatusMsg);
		if(m_ListeningInfo)
			mir_free(m_ListeningInfo);
		if(m_xStatusMsg)
			mir_free(m_xStatusMsg);
	}

}

/**
 * saves message to the input history.
 * it's using streamout in UTF8 format - no unicode "issues" and all RTF formatting is saved to the history.
 */

void CContactCache::saveHistory(WPARAM wParam, LPARAM lParam)
{
	size_t 	iLength = 0, iStreamLength = 0;
	int 	oldTop = 0;
	char*	szFromStream = NULL;

	if(m_hwnd == 0 || m_dat == 0)
		return;

	if (wParam) {
		oldTop = m_iHistoryTop;
		m_iHistoryTop = (int)wParam;
	}

	szFromStream = ::Message_GetFromStream(GetDlgItem(m_hwnd, IDC_MESSAGE), m_dat, (CP_UTF8 << 16) | (SF_RTFNOOBJS | SFF_PLAINRTF | SF_USECODEPAGE));

	iLength = iStreamLength = (strlen(szFromStream) + 1);

	if (iLength > 0 && m_history != NULL) {
		if ((m_iHistoryTop == m_iHistorySize) && oldTop == 0) {         // shift the stack down...
			TInputHistory ihTemp = m_history[0];
			m_iHistoryTop--;
			::MoveMemory((void *)&m_history[0], (void *)&m_history[1], (m_iHistorySize - 1) * sizeof(TInputHistory));
			m_history[m_iHistoryTop] = ihTemp;
		}
		if (iLength > m_history[m_iHistoryTop].lLen) {
			if (m_history[m_iHistoryTop].szText == NULL) {
				if (iLength < HISTORY_INITIAL_ALLOCSIZE)
					iLength = HISTORY_INITIAL_ALLOCSIZE;
				m_history[m_iHistoryTop].szText = (TCHAR *)malloc(iLength);
				m_history[m_iHistoryTop].lLen = iLength;
			} else {
				if (iLength > m_history[m_iHistoryTop].lLen) {
					m_history[m_iHistoryTop].szText = (TCHAR *)realloc(m_history[m_iHistoryTop].szText, iLength);
					m_history[m_iHistoryTop].lLen = iLength;
				}
			}
		}
		::CopyMemory(m_history[m_iHistoryTop].szText, szFromStream, iStreamLength);
		if (!oldTop) {
			if (m_iHistoryTop < m_iHistorySize) {
				m_iHistoryTop++;
				m_iHistoryCurrent = m_iHistoryTop;
			}
		}
	}
	if (szFromStream)
		free(szFromStream);

	if (oldTop)
		m_iHistoryTop = oldTop;
}

/**
 * handle the input history scrolling for the message input area
 * @param wParam: VK_ keyboard code (VK_UP or VK_DOWN)
 */
void CContactCache::inputHistoryEvent(WPARAM wParam)
{
	if(m_hwnd == 0 || m_dat == 0)
		return;

	if (m_history != NULL && m_history[0].szText != NULL) {     // at least one entry needs to be alloced, otherwise we get a nice infinite loop ;)
		HWND		hwndEdit = ::GetDlgItem(m_hwnd, IDC_MESSAGE);
		SETTEXTEX 	stx = {ST_DEFAULT, CP_UTF8};

		if (m_dat->dwFlags & MWF_NEEDHISTORYSAVE) {
			m_iHistoryCurrent = m_iHistoryTop;
			if (::GetWindowTextLengthA(hwndEdit) > 0)
				saveHistory((WPARAM)m_iHistorySize, 0);
			else
				m_history[m_iHistorySize].szText[0] = (TCHAR)'\0';
		}
		if (wParam == VK_UP) {
			if (m_iHistoryCurrent == 0)
				return;
			m_iHistoryCurrent--;
		} else {
			m_iHistoryCurrent++;
			if (m_iHistoryCurrent > m_iHistoryTop)
				m_iHistoryCurrent = m_iHistoryTop;
		}
		if (m_iHistoryCurrent == m_iHistoryTop) {
			if (m_history[m_iHistorySize].szText != NULL) {           // replace the temp buffer
				::SetWindowText(hwndEdit, _T(""));
				::SendMessage(hwndEdit, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)m_history[m_iHistorySize].szText);
				::SendMessage(hwndEdit, EM_SETSEL, (WPARAM) - 1, (LPARAM) - 1);
			}
		} else {
			if (m_history[m_iHistoryCurrent].szText != NULL) {
				::SetWindowText(hwndEdit, _T(""));
				::SendMessage(hwndEdit, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)m_history[m_iHistoryCurrent].szText);
				::SendMessage(hwndEdit, EM_SETSEL, (WPARAM) - 1, (LPARAM) - 1);
			} else
				::SetWindowText(hwndEdit, _T(""));
		}
		::SendMessage(m_hwnd, WM_COMMAND, MAKEWPARAM(::GetDlgCtrlID(hwndEdit), EN_CHANGE), (LPARAM)hwndEdit);
		m_dat->dwFlags &= ~MWF_NEEDHISTORYSAVE;
	}
}

/**
 * allocate the input history (on-demand, when it is requested by
 * opening a message window for this contact).
 */
void CContactCache::allocHistory()
{
	m_iHistorySize = M->GetByte("historysize", 15);
	if (m_iHistorySize < 10)
		m_iHistorySize = 10;
	m_history = (TInputHistory *)malloc(sizeof(TInputHistory) * (m_iHistorySize + 1));
	m_iHistoryCurrent = 0;
	m_iHistoryTop = 0;
	if (m_history)
		ZeroMemory(m_history, sizeof(TInputHistory) * m_iHistorySize);
	m_history[m_iHistorySize].szText = (TCHAR *)malloc((HISTORY_INITIAL_ALLOCSIZE + 1) * sizeof(TCHAR));
	m_history[m_iHistorySize].lLen = HISTORY_INITIAL_ALLOCSIZE;
}

/**
 * release additional memory resources
 */
void CContactCache::releaseAlloced()
{
	int i;

	if(m_stats) {
		delete m_stats;
		m_stats = 0;
	}

	if (m_history) {
		for (i = 0; i <= m_iHistorySize; i++) {
			if (m_history[i].szText != 0) {
				free(m_history[i].szText);
			}
		}
		free(m_history);
		m_history = 0;
	}
}

/**
 * when a contact is deleted, mark it as invalid in the cache and release
 * all memory it has allocated.
 */
void CContactCache::deletedHandler()
{
	m_Valid = false;
	if(m_hwnd)
		::SendMessage(m_hwnd, WM_CLOSE, 1, 2);

	releaseAlloced();
	m_hContact = (HANDLE)-1;
}

/**
 * udpate favorite or recent state. runs when user manually adds
 * or removes a user from that list or when database setting is
 * changed from elsewhere
 */
void CContactCache::updateFavorite()
{
	m_isFavorite = M->GetByte(m_hContact, SRMSGMOD_T, "isFavorite", 0) ? true : false;
	m_isRecent = M->GetDword(m_hContact, "isRecent", 0) ? true : false;
}

/**
 * update all or only the given status message information from the database
 *
 * @param szKey: char* database key name or 0 to reload all messages
 */
void CContactCache::updateStatusMsg(const char *szKey)
{
	DBVARIANT 	dbv = {0};
	BYTE 		bStatusMsgValid = 0;
	INT_PTR		res = 0;

	if(!m_Valid)
		return;

	if(szKey == 0 || (szKey && !strcmp("StatusMsg", szKey))) {
		if(m_szStatusMsg)
			mir_free(m_szStatusMsg);
		m_szStatusMsg = 0;
		res = M->GetTString(m_hContact, "CList", "StatusMsg", &dbv);
		if(res == 0) {
			m_szStatusMsg = (lstrlen(dbv.ptszVal) > 0 ? mir_tstrdup(dbv.ptszVal) : 0);
			DBFreeVariant(&dbv);
		}
	}
	if(szKey == 0 || (szKey && !strcmp("ListeningTo", szKey))) {
		if(m_ListeningInfo)
			mir_free(m_ListeningInfo);
		m_ListeningInfo = 0;
		res = M->GetTString(m_hContact, m_szProto, "ListeningTo", &dbv);
		if(res == 0) {
			m_ListeningInfo = (lstrlen(dbv.ptszVal) > 0 ? mir_tstrdup(dbv.ptszVal) : 0);
			DBFreeVariant(&dbv);
		}
	}
	if(szKey == 0 || (szKey && !strcmp("XStatusMsg", szKey))) {
		if(m_xStatusMsg)
			mir_free(m_xStatusMsg);
		m_xStatusMsg = 0;
		res = M->GetTString(m_hContact, m_szProto, "XStatusMsg", &dbv);
		if(res == 0) {
			m_xStatusMsg = (lstrlen(dbv.ptszVal) > 0 ? mir_tstrdup(dbv.ptszVal) : 0);
			DBFreeVariant(&dbv);
		}
	}
	m_xStatus = M->GetByte(m_hContact, m_szProto, "XStatusId", 0);

	/*
	if (bStatusMsgValid > STATUSMSG_XSTATUSNAME) {
		int j = 0, i, iLen;
		iLen = lstrlen(dbv.ptszVal);
		for (i = 0; i < iLen && j < 1024; i++) {
			if (dbv.ptszVal[i] == (TCHAR)0x0d)
				continue;
			dat->statusMsg[j] = dbv.ptszVal[i] == (TCHAR)0x0a ? (TCHAR)' ' : dbv.ptszVal[i];
			j++;
		}
		dat->statusMsg[j] = (TCHAR)0;
		mir_free(dbv.ptszVal);
	}
	*/
}
