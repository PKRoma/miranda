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
 * (C) 2005-2014 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 * Implements a queued, asynchronous sending system for tabSRMM.
 *
 */

#include "commonheaders.h"
#pragma hdrstop

SendQueue *sendQueue = 0;

extern      const TCHAR *pszIDCSAVE_save, *pszIDCSAVE_close;

static char *pss_msg = "/SendMsg";
static char *pss_msgw = "/SendMsgW";

/**
 * get the service name to send a message
 *
 * @param hContact (HANDLE) contact's handle
 * @param dat      _MessageWindowData
 * @param dwFlags
 *
 * @return (char *) name of the service to send a message to this contact
 */
char *SendQueue::MsgServiceName(const HANDLE hContact = 0, const TWindowData *dat = 0, int dwFlags = 0)
{
	char	szServiceName[100];
	char	*szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);

	if (szProto == NULL)
		return pss_msg;

	if(dat) {
		if (dat->sendMode & SMODE_FORCEANSI || !(dwFlags & PREF_UNICODE))
			return pss_msg;
	}

	_snprintf(szServiceName, sizeof(szServiceName), "%s%sW", szProto, PSS_MESSAGE);
	if (ServiceExists(szServiceName))
		return pss_msgw;

	return pss_msg;
}

/*
 * searches the queue for a message belonging to the given contact which has been marked
 * as "failed" by either the ACKRESULT_FAILED or a timeout handler
 * returns: zero-based queue index or -1 if none was found
 */

int SendQueue::findNextFailed(const TWindowData *dat) const
{
	if(dat) {
		int i;

		for (i = 0; i < NR_SENDJOBS; i++) {
			if (m_jobs[i].hOwner == dat->hContact && m_jobs[i].iStatus == SQ_ERROR)
				return i;
		}
		return -1;
	}
	return -1;
}
void SendQueue::handleError(TWindowData *dat, const int iEntry) const
{
	if(dat) {
		TCHAR szErrorMsg[500];

		dat->iCurrentQueueError = iEntry;
		mir_sntprintf(szErrorMsg, 500, _T("%s"), m_jobs[iEntry].szErrorMsg);
		logError(dat, iEntry, szErrorMsg);
		recallFailed(dat, iEntry);
		showErrorControls(dat, TRUE);
		::HandleIconFeedback(dat, PluginConfig.g_iconErr);
	}
}
/*
 * add a message to the sending queue.
 * iLen = required size of the memory block to hold the message
 */
int SendQueue::addTo(TWindowData *dat, const int iLen, int dwFlags)
{
	int iLength = 0, i;
	int iFound = NR_SENDJOBS;

	if (m_currentIndex >= NR_SENDJOBS) {
		_DebugPopup(dat->hContact, _T("Send queue full"));
		return 0;
	}
	/*
	 * find a free entry in the send queue...
	 */
	for (i = 0; i < NR_SENDJOBS; i++) {
		if (m_jobs[i].hOwner != 0 || m_jobs[i].iStatus != 0) {
			// this entry is used, check if it's orphaned and can be removed...
			if (m_jobs[i].hwndOwner && IsWindow(m_jobs[i].hwndOwner))           // window exists, do not reuse it
				continue;
			if (time(NULL) - m_jobs[i].dwTime < 120)                              // non-acked entry, but not old enough, don't re-use it
				continue;
			clearJob(i);
			iFound = i;
			goto entry_found;
		}
		iFound = i;
		break;
	}
entry_found:
	if (iFound == NR_SENDJOBS) {
		_DebugPopup(dat->hContact, _T("Send queue full"));
		return 0;
	}
	iLength = iLen;
	if (iLength > 0) {
		if (m_jobs[iFound].sendBuffer == NULL) {
			if (iLength < HISTORY_INITIAL_ALLOCSIZE)
				iLength = HISTORY_INITIAL_ALLOCSIZE;
			m_jobs[iFound].sendBuffer = (char *)malloc(iLength);
			m_jobs[iFound].dwLen = iLength;
		}
		else {
			if (iLength > m_jobs[iFound].dwLen) {
				m_jobs[iFound].sendBuffer = (char *)realloc(m_jobs[iFound].sendBuffer, iLength);
				m_jobs[iFound].dwLen = iLength;
			}
		}
		CopyMemory(m_jobs[iFound].sendBuffer, dat->sendBuffer, iLen);
	}
	m_jobs[iFound].dwFlags = dwFlags;
	m_jobs[iFound].dwTime = time(NULL);

	HWND	hwndDlg = dat->hwnd;

	dat->cache->saveHistory(0, 0);
	::SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
	::SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));

	UpdateSaveAndSendButton(dat);
	sendQueued(dat, iFound);
	return 0;
}

/*
 * threshold for word-wrapping when sending messages in chunks
 */

#define SPLIT_WORD_CUTOFF 20

static int SendChunkW(WCHAR *chunk, HANDLE hContact, char *szSvc, DWORD dwFlags)
{
	BYTE	*pBuf = NULL;
	int		wLen = lstrlenW(chunk), id;
	DWORD	memRequired = (wLen + 1) * sizeof(WCHAR);
	DWORD	codePage = DBGetContactSettingDword(hContact, SRMSGMOD_T, "ANSIcodepage", CP_ACP);
	int		mbcsSize = WideCharToMultiByte(codePage, 0, chunk, -1, (char *)pBuf, 0, 0, 0);

	memRequired += mbcsSize;
	pBuf = (BYTE *)mir_alloc(memRequired);
	WideCharToMultiByte(codePage, 0, chunk, -1, (char *)pBuf, mbcsSize, 0, 0);
	CopyMemory(&pBuf[mbcsSize], chunk, (wLen + 1) * sizeof(WCHAR));
	id = CallContactService(hContact, szSvc, dwFlags, (LPARAM)pBuf);
	mir_free(pBuf);
	return id;
}

static int SendChunkA(char *chunk, HANDLE hContact, char *szSvc, DWORD dwFlags)
{
	return(CallContactService(hContact, szSvc, dwFlags, (LPARAM)chunk));
}

static void DoSplitSendW(LPVOID param)
{
	struct  SendJob *job = sendQueue->getJobByIndex((int)param);
	int     id;
	BOOL    fFirstSend = FALSE;
	WCHAR   *wszBegin, *wszTemp, *wszSaved, savedChar;
	int     iLen, iCur = 0, iSavedCur = 0, i;
	BOOL    fSplitting = TRUE;
	char    szServiceName[100], *svcName;
	HANDLE  hContact = job->hOwner;
	DWORD   dwFlags = job->dwFlags;
	int     chunkSize = job->chunkSize / 2;
	char    *szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);

	if (szProto == NULL)
		svcName = pss_msg;
	else {
		_snprintf(szServiceName, sizeof(szServiceName), "%s%sW", szProto, PSS_MESSAGE);
		if (ServiceExists(szServiceName))
			svcName = pss_msgw;
		else
			svcName = pss_msg;
	}

	iLen = lstrlenA(job->sendBuffer);
	wszBegin = (WCHAR *) & job->sendBuffer[iLen + 1];
	wszTemp = (WCHAR *)mir_alloc(sizeof(WCHAR) * (lstrlenW(wszBegin) + 1));
	CopyMemory(wszTemp, wszBegin, sizeof(WCHAR) * (lstrlenW(wszBegin) + 1));
	wszBegin = wszTemp;

	do {
		iCur += chunkSize;
		if (iCur > iLen)
			fSplitting = FALSE;

		/*
		 * try to "word wrap" the chunks - split on word boundaries (space characters), if possible.
		 * SPLIT_WORD_CUTOFF = max length of unbreakable words, longer words may be split.
		*/

		if (fSplitting) {
			i = 0;
			wszSaved = &wszBegin[iCur];
			iSavedCur = iCur;
			while (iCur) {
				if (wszBegin[iCur] == (TCHAR)' ') {
					wszSaved = &wszBegin[iCur];
					break;
				}
				if (i == SPLIT_WORD_CUTOFF) {           // no space found backwards, restore old split position
					iCur = iSavedCur;
					wszSaved = &wszBegin[iCur];
					break;
				}
				i++;
				iCur--;
			}
			savedChar = *wszSaved;
			*wszSaved = 0;
			id = SendChunkW(wszTemp, hContact, svcName, dwFlags);
			if (!fFirstSend) {
				job->hSendId = (HANDLE)id;
				fFirstSend = TRUE;
				PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_SPLITSENDACK, (WPARAM)param, 0);
			}
			*wszSaved = savedChar;
			wszTemp = wszSaved;
			if (savedChar == (TCHAR)' ') {
				wszTemp++;
				iCur++;
			}
		}
		else {
			id = SendChunkW(wszTemp, hContact, svcName, dwFlags);
			if (!fFirstSend) {
				job->hSendId = (HANDLE)id;
				fFirstSend = TRUE;
				PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_SPLITSENDACK, (WPARAM)param, 0);
			}
		}
		Sleep(500L);
	}
	while (fSplitting);
	mir_free(wszBegin);
}

static void DoSplitSendA(LPVOID param)
{
	struct  SendJob *job = sendQueue->getJobByIndex((int)param);
	int     id;
	BOOL    fFirstSend = FALSE;
	char    *szBegin, *szTemp, *szSaved, savedChar;
	int     iLen, iCur = 0, iSavedCur = 0, i;
	BOOL    fSplitting = TRUE;
	char    *svcName;
	HANDLE  hContact = job->hOwner;
	DWORD   dwFlags = job->dwFlags;
	int     chunkSize = job->chunkSize;

	svcName = pss_msg;

	iLen = lstrlenA(job->sendBuffer);
	szTemp = (char *)mir_alloc(iLen + 1);
	CopyMemory(szTemp, job->sendBuffer, iLen + 1);
	szBegin = szTemp;

	do {
		iCur += chunkSize;
		if (iCur > iLen)
			fSplitting = FALSE;

		if (fSplitting) {
			i = 0;
			szSaved = &szBegin[iCur];
			iSavedCur = iCur;
			while (iCur) {
				if (szBegin[iCur] == ' ') {
					szSaved = &szBegin[iCur];
					break;
				}
				if (i == SPLIT_WORD_CUTOFF) {
					iCur = iSavedCur;
					szSaved = &szBegin[iCur];
					break;
				}
				i++;
				iCur--;
			}
			savedChar = *szSaved;
			*szSaved = 0;
			id = SendChunkA(szTemp, hContact, PSS_MESSAGE, dwFlags);
			if (!fFirstSend) {
				job->hSendId = (HANDLE)id;
				fFirstSend = TRUE;
				PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_SPLITSENDACK, (WPARAM)param, 0);
			}
			*szSaved = savedChar;
			szTemp = szSaved;
			if (savedChar == ' ') {
				szTemp++;
				iCur++;
			}
		}
		else {
			id = SendChunkA(szTemp, hContact, PSS_MESSAGE, dwFlags);
			if (!fFirstSend) {
				job->hSendId = (HANDLE)id;
				fFirstSend = TRUE;
				PostMessage(PluginConfig.g_hwndHotkeyHandler, DM_SPLITSENDACK, (WPARAM)param, 0);
			}
		}
		Sleep(500L);
	}
	while (fSplitting);
	mir_free(szBegin);
}

/**
 * return effective length of the message in bytes (utf-8 encoded)
 */
int SendQueue::getSendLength(const int iEntry, int sendMode)
{
	if(m_jobs[iEntry].dwFlags & PREF_UNICODE && !(sendMode & SMODE_FORCEANSI)) {
		int     iLen;
		WCHAR   *wszBuf;
		char    *utf8;
		iLen = lstrlenA(m_jobs[iEntry].sendBuffer);
		wszBuf = (WCHAR *) & m_jobs[iEntry].sendBuffer[iLen + 1];
		utf8 = M->utf8_encodeW(wszBuf);
		m_jobs[iEntry].iSendLength = lstrlenA(utf8);
		mir_free(utf8);
		return(m_jobs[iEntry].iSendLength);
	}
	else {
		m_jobs[iEntry].iSendLength = lstrlenA(m_jobs[iEntry].sendBuffer);
		return(m_jobs[iEntry].iSendLength);
	}
}

int SendQueue::sendQueued(TWindowData *dat, const int iEntry)
{
	HWND	hwndDlg = dat->hwnd;

	if (dat->sendMode & SMODE_MULTIPLE) {
		HANDLE			hContact, hItem;
		int				iJobs = 0;
		int				iMinLength = 0;
		CContactCache*	c = 0;

		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

		m_jobs[iEntry].hOwner = dat->hContact;
		m_jobs[iEntry].iStatus = SQ_INPROGRESS;
		m_jobs[iEntry].hwndOwner = hwndDlg;

		int	iSendLength = getSendLength(iEntry, dat->sendMode);

		do {
			hItem = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM) hContact, 0);
			if (hItem && SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM) hItem, 0)) {
				c = CContactCache::getContactCache(hContact);
				if(c)
					iMinLength = (iMinLength == 0 ? c->getMaxMessageLength() : min(c->getMaxMessageLength(), iMinLength));
			}
		} while (hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0));

		if(iSendLength >= iMinLength) {
			TCHAR	tszError[256];

			mir_sntprintf(tszError, 256, CTranslator::get(CTranslator::GEN_SQ_SENDLATER_ERROR_MSG_TOO_LONG), iMinLength);
			::SendMessage(dat->hwnd, DM_ACTIVATETOOLTIP, IDC_MESSAGE, reinterpret_cast<LPARAM>(tszError));
			sendQueue->clearJob(iEntry);
			return(0);
		}

		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		do {
			hItem = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM) hContact, 0);
			if (hItem && SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM) hItem, 0)) {
				doSendLater(iEntry, 0, hContact, false);
				iJobs++;
			}
		} while (hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0));

		sendQueue->clearJob(iEntry);
		if(iJobs)
			sendLater->flushQueue();							// force queue processing
		return(0);
	}
	else {
		if (dat->hContact == NULL)
			return 0;  //never happens

		dat->nMax = dat->cache->getMaxMessageLength();                      // refresh length info

		if (dat->sendMode & SMODE_FORCEANSI && M->GetByte(dat->cache->getActiveContact(), dat->cache->getActiveProto(), "UnicodeSend", 1))
			M->WriteByte(dat->cache->getActiveContact(), dat->cache->getActiveProto(), "UnicodeSend", 0);
		else if (!(dat->sendMode & SMODE_FORCEANSI) && !M->GetByte(dat->cache->getActiveContact(), dat->cache->getActiveProto(), "UnicodeSend", 0))
			M->WriteByte(dat->cache->getActiveContact(), dat->cache->getActiveProto(), "UnicodeSend", 1);

		if (M->GetByte("autosplit", 0) && !(dat->sendMode & SMODE_SENDLATER)) {
			BOOL    fSplit = FALSE;
			DWORD   dwOldFlags;

			/*
			 * determine send buffer length
			 */
			if(getSendLength(iEntry, dat->sendMode) >= dat->nMax)
				fSplit = true;

			if (!fSplit)
				goto send_unsplitted;

			m_jobs[iEntry].hOwner = dat->hContact;
			m_jobs[iEntry].hwndOwner = hwndDlg;
			m_jobs[iEntry].iStatus = SQ_INPROGRESS;
			m_jobs[iEntry].iAcksNeeded = 1;
			m_jobs[iEntry].chunkSize = dat->nMax;

			dwOldFlags = m_jobs[iEntry].dwFlags;
			if (dat->sendMode & SMODE_FORCEANSI)
				m_jobs[iEntry].dwFlags &= ~PREF_UNICODE;

			if (!(m_jobs[iEntry].dwFlags & PREF_UNICODE) || dat->sendMode & SMODE_FORCEANSI)
				mir_forkthread(DoSplitSendA, (LPVOID)iEntry);
			else
				mir_forkthread(DoSplitSendW, (LPVOID)iEntry);
			m_jobs[iEntry].dwFlags = dwOldFlags;
		}
		else {

send_unsplitted:

			m_jobs[iEntry].hOwner = dat->hContact;
			m_jobs[iEntry].hwndOwner = hwndDlg;
			m_jobs[iEntry].iStatus = SQ_INPROGRESS;
			m_jobs[iEntry].iAcksNeeded = 1;
			if(dat->sendMode & SMODE_SENDLATER) {
				TCHAR	tszError[256];

				int iSendLength = getSendLength(iEntry, dat->sendMode);
				if(iSendLength >= dat->nMax) {
					mir_sntprintf(tszError, 256, CTranslator::get(CTranslator::GEN_SQ_SENDLATER_ERROR_MSG_TOO_LONG), dat->nMax);
					SendMessage(dat->hwnd, DM_ACTIVATETOOLTIP, IDC_MESSAGE, reinterpret_cast<LPARAM>(tszError));
					clearJob(iEntry);
					return(0);
				}
				doSendLater(iEntry, dat);
				clearJob(iEntry);
				return(0);
			}
			m_jobs[iEntry].hSendId = (HANDLE) CallContactService(dat->hContact, MsgServiceName(dat->hContact, dat, m_jobs[iEntry].dwFlags), (dat->sendMode & SMODE_FORCEANSI) ? (m_jobs[iEntry].dwFlags & ~PREF_UNICODE) : m_jobs[iEntry].dwFlags, (LPARAM) m_jobs[iEntry].sendBuffer);

			if (dat->sendMode & SMODE_NOACK) {              // fake the ack if we are not interested in receiving real acks
				ACKDATA ack = {0};
				ack.hContact = dat->hContact;
				ack.hProcess = m_jobs[iEntry].hSendId;
				ack.type = ACKTYPE_MESSAGE;
				ack.result = ACKRESULT_SUCCESS;
				SendMessage(hwndDlg, HM_EVENTSENT, (WPARAM)MAKELONG(iEntry, 0), (LPARAM)&ack);
			}
			else
				SetTimer(hwndDlg, TIMERID_MSGSEND + iEntry, PluginConfig.m_MsgTimeout, NULL);
		}
	}
	dat->iOpenJobs++;
	m_currentIndex++;

	// give icon feedback...

	if (dat->pContainer->hwndActive == hwndDlg)
		::UpdateReadChars(dat);

	if (!(dat->sendMode & SMODE_NOACK))
		::HandleIconFeedback(dat, PluginConfig.g_IconSend);

	if (M->GetByte(SRMSGSET_AUTOMIN, SRMSGDEFSET_AUTOMIN))
		::SendMessage(dat->pContainer->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	return 0;
}

void SendQueue::clearJob(const int iIndex)
{
	m_jobs[iIndex].hOwner = 0;
	m_jobs[iIndex].hwndOwner = 0;
	m_jobs[iIndex].iStatus = 0;
	m_jobs[iIndex].iAcksNeeded = 0;
	m_jobs[iIndex].dwFlags = 0;
	m_jobs[iIndex].chunkSize = 0;
	m_jobs[iIndex].dwTime = 0;
	m_jobs[iIndex].hSendId = 0;
	m_jobs[iIndex].iSendLength = 0;
}

/*
 * this is called when:
 *
 * ) a delivery has completed successfully
 * ) user decided to cancel a failed send
 * it removes the completed / canceled send job from the queue and schedules the next job to send (if any)
 */

void SendQueue::checkQueue(const TWindowData *dat) const
{
	if(dat) {
		HWND	hwndDlg = dat->hwnd;

		if (dat->iOpenJobs == 0) {
			::HandleIconFeedback(const_cast<TWindowData *>(dat), (HICON) - 1);
		}
		else if (!(dat->sendMode & SMODE_NOACK))
			::HandleIconFeedback(const_cast<TWindowData *>(dat), PluginConfig.g_IconSend);

		if (dat->pContainer->hwndActive == hwndDlg)
			::UpdateReadChars(const_cast<TWindowData *>(dat));
	}
}

/*
 * logs an error message to the message window. Optionally, appends the original message
 * from the given sendJob (queue index)
 */

void SendQueue::logError(const TWindowData *dat, int iSendJobIndex, const TCHAR *szErrMsg) const
{
	DBEVENTINFO	dbei = {0};
	int				iMsgLen;

	if(dat == 0)
		return;

	dbei.eventType = EVENTTYPE_ERRMSG;
	dbei.cbSize = sizeof(dbei);
	if (iSendJobIndex >= 0) {
		dbei.pBlob = (BYTE *)m_jobs[iSendJobIndex].sendBuffer;
		iMsgLen = lstrlenA(m_jobs[iSendJobIndex].sendBuffer) + 1;
	}
	else {
		iMsgLen = 0;
		dbei.pBlob = NULL;
	}
	if (m_jobs[iSendJobIndex].dwFlags & PREF_UTF)
		dbei.flags = DBEF_UTF;
	if (iSendJobIndex >= 0) {
		if (m_jobs[iSendJobIndex].dwFlags & PREF_UNICODE) {
			iMsgLen *= 3;
		}
	}
	dbei.cbBlob = iMsgLen;
	dbei.timestamp = time(NULL);
	dbei.szModule = (char *)szErrMsg;
	StreamInEvents(dat->hwnd, NULL, 1, 1, &dbei);
}

/*
 * enable or disable the sending controls in the given window
 * ) input area
 * ) multisend contact list instance
 * ) send button
 */

void SendQueue::EnableSending(const TWindowData *dat, const int iMode)
{
	if(dat) {
		HWND hwndDlg = dat->hwnd;
		::SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETREADONLY, (WPARAM) iMode ? FALSE : TRUE, 0);
		Utils::enableDlgControl(hwndDlg, IDC_CLIST, iMode ? TRUE : FALSE);
		::EnableSendButton(dat, iMode);
	}
}

/*
 * show or hide the error control button bar on top of the window
 */

void SendQueue::showErrorControls(TWindowData *dat, const int showCmd) const
{
	UINT	myerrorControls[] = { IDC_STATICERRORICON, IDC_STATICTEXT, IDC_RETRY, IDC_CANCELSEND, IDC_MSGSENDLATER};
	int		i;
	HWND	hwndDlg = dat->hwnd;

	if (showCmd) {
		TCITEM item = {0};
		dat->hTabIcon = PluginConfig.g_iconErr;
		item.mask = TCIF_IMAGE;
		item.iImage = 0;
		TabCtrl_SetItem(GetDlgItem(dat->pContainer->hwnd, IDC_MSGTABS), dat->iTabID, &item);
		dat->dwFlags |= MWF_ERRORSTATE;
	}
	else {
		dat->dwFlags &= ~MWF_ERRORSTATE;
		dat->hTabIcon = dat->hTabStatusIcon;
	}

	for (i = 0; i < 5; i++) {
		if (IsWindow(GetDlgItem(hwndDlg, myerrorControls[i])))
			Utils::showDlgControl(hwndDlg, myerrorControls[i], showCmd ? SW_SHOW : SW_HIDE);
	}

	SendMessage(hwndDlg, WM_SIZE, 0, 0);
	DM_ScrollToBottom(dat, 0, 1);
	if (m_jobs[0].hOwner != 0)
		EnableSending(dat, TRUE);
}

void SendQueue::recallFailed(const TWindowData *dat, int iEntry) const
{
	int		iLen = GetWindowTextLengthA(GetDlgItem(dat->hwnd, IDC_MESSAGE));

	if(dat) {
		NotifyDeliveryFailure(dat);
		if (iLen == 0) {                    // message area is empty, so we can recall the failed message...
			SETTEXTEX stx = {ST_DEFAULT, 1200};
			if (m_jobs[iEntry].dwFlags & PREF_UNICODE)
				SendDlgItemMessage(dat->hwnd, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)&m_jobs[iEntry].sendBuffer[lstrlenA(m_jobs[iEntry].sendBuffer) + 1]);
			else {
				stx.codepage = (m_jobs[iEntry].dwFlags & PREF_UTF) ? CP_UTF8 : CP_ACP;
				SendDlgItemMessage(dat->hwnd, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)m_jobs[iEntry].sendBuffer);
			}
			UpdateSaveAndSendButton(const_cast<TWindowData *>(dat));
			SendDlgItemMessage(dat->hwnd, IDC_MESSAGE, EM_SETSEL, (WPARAM) - 1, (LPARAM) - 1);
		}
	}
}

void SendQueue::UpdateSaveAndSendButton(TWindowData *dat)
{
	if(dat) {
		int		len;
		HWND	hwndDlg = dat->hwnd;

		GETTEXTLENGTHEX	gtxl = {0};
		gtxl.codepage = CP_UTF8;
		gtxl.flags = GTL_DEFAULT | GTL_PRECISE | GTL_NUMBYTES;

		len = SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETTEXTLENGTHEX, (WPARAM) & gtxl, 0);
		if (len && GetSendButtonState(hwndDlg) == PBS_DISABLED)
			EnableSendButton(dat, TRUE);
		else if (len == 0 && GetSendButtonState(hwndDlg) != PBS_DISABLED)
			EnableSendButton(dat, FALSE);

		if (len) {          // looks complex but avoids flickering on the button while typing.
			if (!(dat->dwFlags & MWF_SAVEBTN_SAV)) {
				SendDlgItemMessage(hwndDlg, IDC_SAVE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) PluginConfig.g_buttonBarIcons[ICON_BUTTON_SAVE]);
				SendDlgItemMessage(hwndDlg, IDC_SAVE, BUTTONADDTOOLTIP, (WPARAM) pszIDCSAVE_save, 0);
				dat->dwFlags |= MWF_SAVEBTN_SAV;
			}
		}
		else {
			SendDlgItemMessage(hwndDlg, IDC_SAVE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) PluginConfig.g_buttonBarIcons[ICON_BUTTON_CANCEL]);
			SendDlgItemMessage(hwndDlg, IDC_SAVE, BUTTONADDTOOLTIP, (WPARAM) pszIDCSAVE_close, 0);
			dat->dwFlags &= ~MWF_SAVEBTN_SAV;
		}
		dat->textLen = len;
	}
}

void SendQueue::NotifyDeliveryFailure(const TWindowData *dat)
{
	POPUPDATAT_V2	ppd = {0};
	int				ibsize = 1023;

	if(M->GetByte("adv_noErrorPopups", 0))
		return;

	if (CallService(MS_POPUP_QUERY, PUQS_GETSTATUS, 0) == 1) {
		ZeroMemory((void *)&ppd, sizeof(ppd));
		ppd.lchContact = dat->hContact;
		mir_sntprintf(ppd.lptzContactName, MAX_CONTACTNAME, _T("%s"), dat->cache->getNick());
		mir_sntprintf(ppd.lptzText, MAX_SECONDLINE, _T("%s"), CTranslator::get(CTranslator::GEN_SQ_DELIVERYFAILED));
		if (!(BOOL)M->GetByte(MODULE, OPT_COLDEFAULT_ERR, TRUE))
		{
			ppd.colorText = (COLORREF)M->GetDword(MODULE, OPT_COLTEXT_ERR, DEFAULT_COLTEXT);
			ppd.colorBack = (COLORREF)M->GetDword(MODULE, OPT_COLBACK_ERR, DEFAULT_COLBACK);
		}
		else
			ppd.colorText = ppd.colorBack = 0;
		ppd.PluginWindowProc = reinterpret_cast<WNDPROC>(Utils::PopupDlgProcError);
		ppd.lchIcon = PluginConfig.g_iconErr;
		ppd.PluginData = (void *)dat->hContact;
		ppd.iSeconds = (int)M->GetDword(MODULE, OPT_DELAY_ERR, (DWORD)DEFAULT_DELAY);
		CallService(MS_POPUP_ADDPOPUPT, (WPARAM)&ppd, 0);
	}
}

/*
 * searches string for characters typical for RTL text (hebrew and other RTL languages
*/

int SendQueue::RTL_Detect(const WCHAR *pszwText)
{
	WORD	*infoTypeC2;
	int		i, n = 0;
	int		iLen = lstrlenW(pszwText);

	infoTypeC2 = (WORD *)mir_alloc(sizeof(WORD) * (iLen + 2));

	if (infoTypeC2) {
		ZeroMemory(infoTypeC2, sizeof(WORD) * (iLen + 2));

		GetStringTypeW(CT_CTYPE2, pszwText, iLen, infoTypeC2);

		for (i = 0; i < iLen; i++) {
			if (infoTypeC2[i] == C2_RIGHTTOLEFT)
				n++;
		}
		mir_free(infoTypeC2);
		return(n >= 2 ? 1 : 0);
	}
	return 0;
}

int SendQueue::ackMessage(TWindowData *dat, WPARAM wParam, LPARAM lParam)
{
	ACKDATA				*ack = (ACKDATA *) lParam;
	DBEVENTINFO			dbei = { 0};
	HANDLE				hNewEvent;
	int					iFound = SendQueue::NR_SENDJOBS, iNextFailed;
	TContainerData *m_pContainer = 0;
	if (dat)
		m_pContainer = dat->pContainer;

	iFound = (int)(LOWORD(wParam));
	//i = (int)(HIWORD(wParam));

	if (m_jobs[iFound].iStatus == SQ_ERROR) {      // received ack for a job which is already in error state...
		if (dat) {                        // window still open
			if (dat->iCurrentQueueError == iFound) {
				dat->iCurrentQueueError = -1;
				showErrorControls(dat, FALSE);
			}
		}
		/*
		 * we must discard this job, because there is no message window open to handle the
		 * error properly. But we display a tray notification to inform the user about the problem.
		 */
		else
			goto inform_and_discard;
	}

	// failed acks are only handled when the window is still open. with no window open, they will be *silently* discarded

	if (ack->result == ACKRESULT_FAILED) {
		if (dat) {
			/*
			 * "hard" errors are handled differently in multisend. There is no option to retry - once failed, they
			 * are discarded and the user is notified with a small log message.
			 */
			if (!nen_options.iNoSounds && !(m_pContainer->dwFlags & CNT_NOSOUND))
				SkinPlaySound("SendError");

			TCHAR *szAckMsg = mir_a2t((char *)ack->lParam);
			mir_sntprintf(m_jobs[iFound].szErrorMsg, safe_sizeof(m_jobs[iFound].szErrorMsg),
						 CTranslator::get(CTranslator::GEN_MSG_DELIVERYFAILURE), szAckMsg);
			m_jobs[iFound].iStatus = SQ_ERROR;
			mir_free(szAckMsg);
			KillTimer(dat->hwnd, TIMERID_MSGSEND + iFound);
			if (!(dat->dwFlags & MWF_ERRORSTATE))
				handleError(dat, iFound);
			return 0;
		}
		else {
inform_and_discard:
			_DebugPopup(m_jobs[iFound].hOwner, CTranslator::get(CTranslator::GEN_SQ_DELIVERYFAILEDLATE));
			clearJob(iFound);
			return 0;
		}
	}

	dbei.cbSize = sizeof(dbei);
	dbei.eventType = EVENTTYPE_MESSAGE;
	dbei.flags = DBEF_SENT;
	dbei.szModule = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) m_jobs[iFound].hOwner, 0);
	dbei.timestamp = time(NULL);
	dbei.cbBlob = lstrlenA(m_jobs[iFound].sendBuffer) + 1;

	if (dat)
		dat->cache->updateStats(TSessionStats::BYTES_SENT, dbei.cbBlob - 1);
	else {
		CContactCache *c = CContactCache::getContactCache(m_jobs[iFound].hOwner);
		if(c)
			c->updateStats(TSessionStats::BYTES_SENT, dbei.cbBlob - 1);
	}

	if (m_jobs[iFound].dwFlags & PREF_UNICODE)
		dbei.cbBlob *= sizeof(TCHAR) + 1;
	if (m_jobs[iFound].dwFlags & PREF_RTL)
		dbei.flags |= DBEF_RTL;
	if (m_jobs[iFound].dwFlags & PREF_UTF)
		dbei.flags |= DBEF_UTF;
	dbei.pBlob = (PBYTE) m_jobs[iFound].sendBuffer;
	hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) m_jobs[iFound].hOwner, (LPARAM) & dbei);

	if (m_pContainer) {
		if (!nen_options.iNoSounds && !(m_pContainer->dwFlags & CNT_NOSOUND))
			SkinPlaySound("SendMsg");
	}

	if (dat && (m_jobs[iFound].hOwner == dat->hContact)) {
		if (dat->hDbEventFirst == NULL) {
			dat->hDbEventFirst = hNewEvent;
			SendMessage(dat->hwnd, DM_REMAKELOG, 0, 0);
		}
	}

	m_jobs[iFound].hSendId = NULL;
	m_jobs[iFound].iAcksNeeded--;

	if (m_jobs[iFound].iAcksNeeded == 0) {              // everything sent
		//if (m_jobs[iFound].hOwner != 0 && dat)
		//	EnableSending(dat, TRUE);
		clearJob(iFound);
		if (dat) {
			KillTimer(dat->hwnd, TIMERID_MSGSEND + iFound);
			dat->iOpenJobs--;
		}
		m_currentIndex--;
	}
	if (dat) {
		checkQueue(dat);
		if ((iNextFailed = findNextFailed(dat)) >= 0 && !(dat->dwFlags & MWF_ERRORSTATE))
			handleError(dat, iNextFailed);
		//MAD: close on send mode
		else {
			if (M->GetByte("AutoClose", 0)) {
				if(M->GetByte("adv_AutoClose_2", 0))
					SendMessage(dat->hwnd, WM_CLOSE, 0, 1);
				else
					SendMessage(dat->pContainer->hwnd, WM_CLOSE, 0, 0);
			}
		}
		//MAD_
	}
	return 0;
}

LRESULT SendQueue::WarnPendingJobs(unsigned int uNrMessages)
{
	return(MessageBox(0, CTranslator::get(CTranslator::GEN_SQ_WARNING),
					  CTranslator::get(CTranslator::GEN_SQ_WARNING_TITLE), MB_YESNO | MB_ICONHAND));
}

/**
 * This just adds the message to the database for later delivery and
 * adds the contact to the list of contacts that have queued messages
 *
 * @param iJobIndex int: index of the send job
 * 		  dat: Message window data
 * 		  fAddHeader: add the "message was sent delayed" header (default = true)
 * 		  hContact  : contact to which the job should be added (default = hOwner of the send job)
 *
 * @return the index on success, -1 on failure
 */
int SendQueue::doSendLater(int iJobIndex, TWindowData *dat, HANDLE hContact, bool fIsSendLater)
{
	bool  fAvail = sendLater->isAvail();

	const TCHAR *szNote = 0;

	if(fIsSendLater && dat) {
		if(fAvail)
			szNote = CTranslator::get(CTranslator::GEN_SQ_QUEUED_MESSAGE);
		else
			szNote = CTranslator::get(CTranslator::GEN_SQ_QUEUING_NOT_AVAIL);

		char  *utfText = M->utf8_encodeT(szNote);
		DBEVENTINFO dbei;
		dbei.cbSize = sizeof(dbei);
		dbei.eventType = EVENTTYPE_MESSAGE;
		dbei.flags = DBEF_SENT | DBEF_UTF;
		dbei.szModule = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) dat->hContact, 0);
		dbei.timestamp = time(NULL);
		dbei.cbBlob = lstrlenA(utfText) + 1;
		dbei.pBlob = (PBYTE) utfText;
		StreamInEvents(dat->hwnd,  0, 1, 1, &dbei);
		if (dat->hDbEventFirst == NULL)
			SendMessage(dat->hwnd, DM_REMAKELOG, 0, 0);
		dat->cache->saveHistory(0, 0);
		EnableSendButton(dat, FALSE);
		if (dat->pContainer->hwndActive == dat->hwnd)
			UpdateReadChars(dat);
		SendDlgItemMessage(dat->hwnd, IDC_SAVE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) PluginConfig.g_buttonBarIcons[ICON_BUTTON_CANCEL]);
		SendDlgItemMessage(dat->hwnd, IDC_SAVE, BUTTONADDTOOLTIP, (WPARAM)pszIDCSAVE_close, 0);
		dat->dwFlags &= ~MWF_SAVEBTN_SAV;
		mir_free(utfText);

		if(!fAvail)
			return(0);
	}

	if(iJobIndex >= 0 && iJobIndex < NR_SENDJOBS) {
		SendJob*	job = &m_jobs[iJobIndex];
		char		szKeyName[20];
		TCHAR 		tszTimestamp[30], tszHeader[150];
		time_t 		now = time(0);

		if(fIsSendLater) {
			TCHAR *formatTime = _T("%Y.%m.%d - %H:%M");
			_tcsftime(tszTimestamp, 30, formatTime, _localtime32((__time32_t *)&now));
			tszTimestamp[29] = 0;
			mir_snprintf(szKeyName, 20, "S%d", now);
			mir_sntprintf(tszHeader, safe_sizeof(tszHeader), CTranslator::get(CTranslator::GEN_SQ_SENDLATER_HEADER), tszTimestamp);
		}
		else
			mir_sntprintf(tszHeader, safe_sizeof(tszHeader), _T("M%d|"), time(0));

		if(job->dwFlags & PREF_UTF || !(job->dwFlags & PREF_UNICODE)) {
			char *utf_header = M->utf8_encodeT(tszHeader);
			UINT required = lstrlenA(utf_header) + lstrlenA(job->sendBuffer) + 10;
			char *tszMsg = reinterpret_cast<char *>(mir_alloc(required));

			if(fIsSendLater) {
				mir_snprintf(tszMsg, required, "%s%s", job->sendBuffer, utf_header);
				DBWriteContactSettingString(hContact ? hContact : job->hOwner, "SendLater", szKeyName, tszMsg);
			}
			else {
				mir_snprintf(tszMsg, required, "%s%s", utf_header, job->sendBuffer);
				sendLater->addJob(tszMsg, (LPARAM)hContact);
			}
			mir_free(utf_header);
			mir_free(tszMsg);
		}
		else if(job->dwFlags & PREF_UNICODE) {
			int iLen = lstrlenA(job->sendBuffer);
			wchar_t *wszMsg = (wchar_t *)&job->sendBuffer[iLen + 1];

			UINT required = sizeof(TCHAR) * (lstrlen(tszHeader) + lstrlenW(wszMsg) + 10);

			TCHAR *tszMsg = reinterpret_cast<TCHAR *>(mir_alloc(required));
			if(fIsSendLater)
				mir_sntprintf(tszMsg, required, _T("%s%s"), wszMsg, tszHeader);
			else
				mir_sntprintf(tszMsg, required, _T("%s%s"), tszHeader, wszMsg);
			char *utf = M->utf8_encodeT(tszMsg);
			if(fIsSendLater)
				DBWriteContactSettingString(hContact ? hContact : job->hOwner, "SendLater", szKeyName, utf);
			else
				sendLater->addJob(utf, (LPARAM)hContact);
			mir_free(utf);
			mir_free(tszMsg);
		}
		if(fIsSendLater) {
			int iCount = M->GetDword(hContact ? hContact : job->hOwner, "SendLater", "count", 0);
			iCount++;
			M->WriteDword(hContact ? hContact : job->hOwner, "SendLater", "count", iCount);
			sendLater->addContact(hContact ? hContact : job->hOwner);
		}
		return(iJobIndex);
	}
	return(-1);
}
