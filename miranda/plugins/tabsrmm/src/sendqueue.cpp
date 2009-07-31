/*
astyle --force-indent=tab=4 --brackets=linux --indent-switches
		--pad=oper --one-line=keep-blocks  --unpad=paren

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/*
 * sendqueue.cpp
 * implements a queued send system
 * part of tabSRMM, (C) 2004-2009 by Miranda IM project
 *
 * $Id: sendqueue.c 10390 2009-07-22 19:43:01Z silvercircle $
 */

#include "commonheaders.h"
#pragma hdrstop

SendQueue *sendQueue = 0;

extern      TCHAR *pszIDCSAVE_save, *pszIDCSAVE_close;
extern 		UINT infoPanelControls[];

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
char *SendQueue::MsgServiceName(const HANDLE hContact = 0, const _MessageWindowData *dat = 0, int dwFlags = 0)
{
#ifdef _UNICODE
	char	szServiceName[100];
	char	*szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);

	if (szProto == NULL)
		return pss_msg;

	if (dat->sendMode & SMODE_FORCEANSI || !(dwFlags & PREF_UNICODE))
		return pss_msg;

	_snprintf(szServiceName, sizeof(szServiceName), "%s%sW", szProto, PSS_MESSAGE);
	if (ServiceExists(szServiceName))
		return pss_msgw;
#endif
	return pss_msg;
}

#define MS_INITIAL_DELAY 500

/**
 * thread function to handle sending to multiple contacts
 *
 * @param param  (int) index of the send queue item holding the send job information
 *
 * @return 0 when finished
 */
static unsigned __stdcall WINAPI DoMultiSend(LPVOID param)
{
	int		iIndex = (int)param;
	SendJob *job = sendQueue->getJobByIndex(iIndex);
	HWND	hwndOwner = job->hwndOwner;
	DWORD	dwDelay = MS_INITIAL_DELAY;               // start with 1sec delay...
	DWORD	dwDelayAdd = 0;
	struct	_MessageWindowData *dat = (struct _MessageWindowData *)GetWindowLongPtr(hwndOwner, GWLP_USERDATA);
	int		i;

	for (i = 0; i < job->sendCount; i++) {
		job->hSendId[i] = (HANDLE) CallContactService(job->hContact[i], SendQueue::MsgServiceName(job->hContact[i], dat, job->dwFlags), (dat->sendMode & SMODE_FORCEANSI) ? (job->dwFlags & ~PREF_UNICODE) : job->dwFlags, (LPARAM) job->sendBuffer);
		SetTimer(job->hwndOwner, TIMERID_MULTISEND_BASE + (iIndex * SENDJOBS_MAX_SENDS) + i, _Plugin.m_MsgTimeout, NULL);
		Sleep((50 * i) + dwDelay + dwDelayAdd);
		if (i > 2)
			dwDelayAdd = 500;
		if (i > 8)
			dwDelayAdd = 1000;
		if (i > 14)
			dwDelayAdd = 1500;
	}
	SendMessage(hwndOwner, DM_MULTISENDTHREADCOMPLETE, 0, 0);
	return 0;
}

/*
 * searches the queue for a message belonging to the given contact which has been marked
 * as "failed" by either the ACKRESULT_FAILED or a timeout handler
 * returns: zero-based queue index or -1 if none was found
 */

int SendQueue::findNextFailed(const _MessageWindowData *dat) const
{
	if(dat) {
		int i;

		for (i = 0; i < NR_SENDJOBS; i++) {
			if (m_jobs[i].hOwner == dat->hContact && m_jobs[i].sendCount > 0 && m_jobs[i].iStatus == SQ_ERROR)
				return i;
		}
		return -1;
	}
	return -1;
}
void SendQueue::handleError(_MessageWindowData *dat, const int iEntry) const
{
	if(dat) {
		char szErrorMsg[512];

		dat->iCurrentQueueError = iEntry;
		_snprintf(szErrorMsg, 500, "%s", m_jobs[iEntry].szErrorMsg);
#if defined(_UNICODE)
			wchar_t wszErrorMsg[512];
			MultiByteToWideChar(_Plugin.m_LangPackCP, 0, szErrorMsg, -1, wszErrorMsg, 512);
			wszErrorMsg[511] = 0;
			logError(dat, iEntry, wszErrorMsg);
#else
		logError(dat, iEntry, szErrorMsg);
#endif
		recallFailed(dat, iEntry);
		showErrorControls(dat, TRUE);
		::HandleIconFeedback(dat->hwnd, dat, _Plugin.g_iconErr);
	}
}
/*
 * add a message to the sending queue.
 * iLen = required size of the memory block to hold the message
 */
int SendQueue::addTo(_MessageWindowData *dat, const int iLen, int dwFlags)
{
	int iLength = 0, i;
	int iFound = NR_SENDJOBS;

	if (m_currentIndex >= NR_SENDJOBS) {
		_DebugMessage(dat->hwnd, dat, "Send queue full");
		return 0;
	}
	/*
	 * find a free entry in the send queue...
	 */
	for (i = 0; i < NR_SENDJOBS; i++) {
		if (m_jobs[i].hOwner != 0 || m_jobs[i].sendCount != 0 || m_jobs[i].iStatus != 0) {
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
		_DebugMessage(dat->hwnd, dat, "Send queue full");
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

	::SaveInputHistory(hwndDlg, dat, 0, 0);
	::SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
	::EnableSendButton(hwndDlg, FALSE);
	::SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));

	UpdateSaveAndSendButton(dat);
	sendQueued(dat, iFound);
	return 0;
}

/*
 * threshold for word-wrapping when sending messages in chunks
 */

#define SPLIT_WORD_CUTOFF 20

#if defined(_UNICODE)
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
#endif

static int SendChunkA(char *chunk, HANDLE hContact, char *szSvc, DWORD dwFlags)
{
	return(CallContactService(hContact, szSvc, dwFlags, (LPARAM)chunk));
}

#if defined(_UNICODE)

static void DoSplitSendW(LPVOID param)
{
	struct  SendJob *job = sendQueue->getJobByIndex((int)param);
	int     id;
	BOOL    fFirstSend = FALSE;
	WCHAR   *wszBegin, *wszTemp, *wszSaved, savedChar;
	int     iLen, iCur = 0, iSavedCur = 0, i;
	BOOL    fSplitting = TRUE;
	char    szServiceName[100], *svcName;
	HANDLE  hContact = job->hContact[0];
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
				job->hSendId[0] = (HANDLE)id;
				fFirstSend = TRUE;
				PostMessage(_Plugin.g_hwndHotkeyHandler, DM_SPLITSENDACK, (WPARAM)param, 0);
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
				job->hSendId[0] = (HANDLE)id;
				fFirstSend = TRUE;
				PostMessage(_Plugin.g_hwndHotkeyHandler, DM_SPLITSENDACK, (WPARAM)param, 0);
			}
		}
		Sleep(500L);
	}
	while (fSplitting);
	mir_free(wszBegin);
}

#endif

static void DoSplitSendA(LPVOID param)
{
	struct  SendJob *job = sendQueue->getJobByIndex((int)param);
	int     id;
	BOOL    fFirstSend = FALSE;
	char    *szBegin, *szTemp, *szSaved, savedChar;
	int     iLen, iCur = 0, iSavedCur = 0, i;
	BOOL    fSplitting = TRUE;
	char    *svcName;
	HANDLE  hContact = job->hContact[0];
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
				job->hSendId[0] = (HANDLE)id;
				fFirstSend = TRUE;
				PostMessage(_Plugin.g_hwndHotkeyHandler, DM_SPLITSENDACK, (WPARAM)param, 0);
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
				job->hSendId[0] = (HANDLE)id;
				fFirstSend = TRUE;
				PostMessage(_Plugin.g_hwndHotkeyHandler, DM_SPLITSENDACK, (WPARAM)param, 0);
			}
		}
		Sleep(500L);
	}
	while (fSplitting);
	mir_free(szBegin);
}

int SendQueue::sendQueued(_MessageWindowData *dat, const int iEntry)
{
	HWND	hwndDlg = dat->hwnd;

	if (dat->sendMode & SMODE_MULTIPLE) {            // implement multiple later...
		HANDLE hContact, hItem;
		m_jobs[iEntry].sendCount = 0;
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
		do {
			hItem = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM) hContact, 0);
			if (hItem && SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM) hItem, 0)) {
				m_jobs[iEntry].sendCount++;
				m_jobs[iEntry].hContact[m_jobs[iEntry].sendCount - 1] = hContact;
				m_jobs[iEntry].hSendId[m_jobs[iEntry].sendCount - 1] = 0;
			}
		}
		while (hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0));
		if (m_jobs[iEntry].sendCount == 0) {
			logError(dat, -1, TranslateT("You haven't selected any contacts from the list. Click the checkbox box next to a name to send the message to that person."));
			return 0;
		}

		if (m_jobs[iEntry].sendCount > 1)             // disable queuing more messages in multisend mode...
			EnableSending(dat, FALSE);
		m_jobs[iEntry].hOwner = dat->hContact;
		m_jobs[iEntry].iStatus = SQ_INPROGRESS;
		m_jobs[iEntry].hwndOwner = hwndDlg;
		m_jobs[iEntry].iAcksNeeded = m_jobs[iEntry].sendCount;
		dat->hMultiSendThread = (HANDLE)mir_forkthreadex(DoMultiSend, (LPVOID)iEntry, 0, NULL);
	}
	else {
		if (dat->hContact == NULL)
			return 0;  //never happens

		if (dat->sendMode & SMODE_FORCEANSI && M->GetByte(dat->bIsMeta ? dat->hSubContact : dat->hContact, dat->bIsMeta ? dat->szMetaProto : dat->szProto, "UnicodeSend", 1))
			M->WriteByte(dat->bIsMeta ? dat->hSubContact : dat->hContact, dat->bIsMeta ? dat->szMetaProto : dat->szProto, "UnicodeSend", 0);
		else if (!(dat->sendMode & SMODE_FORCEANSI) && !M->GetByte(dat->bIsMeta ? dat->hSubContact : dat->hContact, dat->bIsMeta ? dat->szMetaProto : dat->szProto, "UnicodeSend", 0))
			M->WriteByte(dat->bIsMeta ? dat->hSubContact : dat->hContact, dat->bIsMeta ? dat->szMetaProto : dat->szProto, "UnicodeSend", 1);

		if (M->GetByte("autosplit", 0)) {
			BOOL    fSplit = FALSE;
			DWORD   dwOldFlags;

			::GetMaxMessageLength(hwndDlg, dat);                      // refresh length info
			/*
			 + determine send buffer length
			*/
#if defined(_UNICODE)
			if (m_jobs[iEntry].dwFlags & PREF_UNICODE && !(dat->sendMode & SMODE_FORCEANSI)) {
				int     iLen;
				WCHAR   *wszBuf;
				char    *utf8;
				iLen = lstrlenA(m_jobs[iEntry].sendBuffer);
				wszBuf = (WCHAR *) & m_jobs[iEntry].sendBuffer[iLen + 1];
				utf8 = M->utf8_encodeW(wszBuf);
				if (lstrlenA(utf8) >= dat->nMax)
					fSplit = TRUE;
				mir_free(utf8);
			}
			else {
				if (lstrlenA(m_jobs[iEntry].sendBuffer) >= dat->nMax)
					fSplit = TRUE;
			}
#else
			if (lstrlenA(m_jobs[iEntry].sendBuffer) >= dat->nMax)
				fSplit = TRUE;
#endif

			if (!fSplit)
				goto send_unsplitted;

			m_jobs[iEntry].sendCount = 1;
			m_jobs[iEntry].hContact[0] = dat->hContact;
			m_jobs[iEntry].hOwner = dat->hContact;
			m_jobs[iEntry].hwndOwner = hwndDlg;
			m_jobs[iEntry].iStatus = SQ_INPROGRESS;
			m_jobs[iEntry].iAcksNeeded = 1;
			m_jobs[iEntry].chunkSize = dat->nMax;

			dwOldFlags = m_jobs[iEntry].dwFlags;
			if (dat->sendMode & SMODE_FORCEANSI)
				m_jobs[iEntry].dwFlags &= ~PREF_UNICODE;

#if defined(_UNICODE)
			if (!(m_jobs[iEntry].dwFlags & PREF_UNICODE) || dat->sendMode & SMODE_FORCEANSI)
				mir_forkthread(DoSplitSendA, (LPVOID)iEntry);
			else
				mir_forkthread(DoSplitSendW, (LPVOID)iEntry);
#else
			mir_forkthread(DoSplitSendA, (LPVOID)iEntry);
#endif
			m_jobs[iEntry].dwFlags = dwOldFlags;
		}
		else {

send_unsplitted:

			m_jobs[iEntry].sendCount = 1;
			m_jobs[iEntry].hContact[0] = dat->hContact;
			m_jobs[iEntry].hSendId[0] = (HANDLE) CallContactService(dat->hContact, MsgServiceName(dat->hContact, dat, m_jobs[iEntry].dwFlags), (dat->sendMode & SMODE_FORCEANSI) ? (m_jobs[iEntry].dwFlags & ~PREF_UNICODE) : m_jobs[iEntry].dwFlags, (LPARAM) m_jobs[iEntry].sendBuffer);
			m_jobs[iEntry].hOwner = dat->hContact;
			m_jobs[iEntry].hwndOwner = hwndDlg;
			m_jobs[iEntry].iStatus = SQ_INPROGRESS;
			m_jobs[iEntry].iAcksNeeded = 1;

			if (dat->sendMode & SMODE_NOACK) {              // fake the ack if we are not interested in receiving real acks
				ACKDATA ack = {0};
				ack.hContact = dat->hContact;
				ack.hProcess = m_jobs[iEntry].hSendId[0];
				ack.type = ACKTYPE_MESSAGE;
				ack.result = ACKRESULT_SUCCESS;
				SendMessage(hwndDlg, HM_EVENTSENT, (WPARAM)MAKELONG(iEntry, 0), (LPARAM)&ack);
			}
			else
				SetTimer(hwndDlg, TIMERID_MSGSEND + iEntry, _Plugin.m_MsgTimeout, NULL);
		}
	}
	dat->iOpenJobs++;
	m_currentIndex++;

	// give icon feedback...

	if (dat->pContainer->hwndActive == hwndDlg)
		::UpdateReadChars(hwndDlg, dat);

	if (!(dat->sendMode & SMODE_NOACK))
		::HandleIconFeedback(hwndDlg, dat, _Plugin.g_IconSend);

	if (M->GetByte(SRMSGSET_AUTOMIN, SRMSGDEFSET_AUTOMIN))
		::SendMessage(dat->pContainer->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	return 0;
}

void SendQueue::clearJob(const int iIndex)
{
	m_jobs[iIndex].hOwner = 0;
	m_jobs[iIndex].hwndOwner = 0;
	m_jobs[iIndex].sendCount = 0;
	m_jobs[iIndex].iStatus = 0;
	m_jobs[iIndex].iAcksNeeded = 0;
	m_jobs[iIndex].dwFlags = 0;
	m_jobs[iIndex].chunkSize = 0;
	m_jobs[iIndex].dwTime = 0;
	ZeroMemory(m_jobs[iIndex].hContact, sizeof(HANDLE) * SENDJOBS_MAX_SENDS);
	ZeroMemory(m_jobs[iIndex].hSendId, sizeof(HANDLE) * SENDJOBS_MAX_SENDS);
}

/*
 * this is called when:
 *
 * ) a delivery has completed successfully
 * ) user decided to cancel a failed send
 * it removes the completed / canceled send job from the queue and schedules the next job to send (if any)
 */

void SendQueue::checkQueue(const _MessageWindowData *dat) const
{
	if(dat) {
		HWND	hwndDlg = dat->hwnd;

		if (dat->iOpenJobs == 0) {
			::HandleIconFeedback(hwndDlg, const_cast<_MessageWindowData *>(dat), (HICON) - 1);
		}
		else if (!(dat->sendMode & SMODE_NOACK))
			::HandleIconFeedback(hwndDlg, const_cast<_MessageWindowData *>(dat), _Plugin.g_IconSend);

		if (dat->pContainer->hwndActive == hwndDlg)
			::UpdateReadChars(hwndDlg, const_cast<_MessageWindowData *>(dat));
	}
}

/*
 * logs an error message to the message window. Optionally, appends the original message
 * from the given sendJob (queue index)
 */

void SendQueue::logError(const _MessageWindowData *dat, int iSendJobIndex, const TCHAR *szErrMsg) const
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
#if defined(_UNICODE)
	if (iSendJobIndex >= 0) {
		if (m_jobs[iSendJobIndex].dwFlags & PREF_UNICODE) {
			iMsgLen *= 3;
		}
	}
#endif
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

void SendQueue::EnableSending(const _MessageWindowData *dat, const int iMode)
{
	if(dat) {
		HWND hwndDlg = dat->hwnd;
		::SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETREADONLY, (WPARAM) iMode ? FALSE : TRUE, 0);
		::EnableWindow(GetDlgItem(hwndDlg, IDC_CLIST), iMode ? TRUE : FALSE);
		::EnableSendButton(hwndDlg, iMode);
	}
}

/*
 * show or hide the error control button bar on top of the window
 */

void SendQueue::showErrorControls(_MessageWindowData *dat, const int showCmd) const
{
	UINT	myerrorControls[] = { IDC_STATICERRORICON, IDC_STATICTEXT, IDC_RETRY, IDC_CANCELSEND, IDC_MSGSENDLATER};
	int		i;
	HWND	hwndDlg = dat->hwnd;

	if (showCmd) {
		TCITEM item = {0};
		dat->hTabIcon = _Plugin.g_iconErr;
		item.mask = TCIF_IMAGE;
		item.iImage = 0;
		TabCtrl_SetItem(GetDlgItem(dat->pContainer->hwnd, IDC_MSGTABS), dat->iTabID, &item);
		dat->dwFlags |= MWF_ERRORSTATE;
	}
	else {
		dat->dwFlags &= ~MWF_ERRORSTATE;
		dat->hTabIcon = dat->hTabStatusIcon;
	}
	if (dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
		if (showCmd)
			::ShowMultipleControls(hwndDlg, infoPanelControls, 7, SW_HIDE);
		else
			::SendMessage(hwndDlg, DM_SETINFOPANEL, 0, 0);
	}

	for (i = 0; i < 5; i++) {
		if (IsWindow(GetDlgItem(hwndDlg, myerrorControls[i])))
			ShowWindow(GetDlgItem(hwndDlg, myerrorControls[i]), showCmd ? SW_SHOW : SW_HIDE);
		else
			_DebugPopup(0, _T("%d is not a window"), myerrorControls[i]);
	}

	SendMessage(hwndDlg, WM_SIZE, 0, 0);
	DM_ScrollToBottom(hwndDlg, dat, 0, 1);
	if (m_jobs[0].sendCount > 1)
		EnableSending(dat, TRUE);
}

void SendQueue::recallFailed(const _MessageWindowData *dat, int iEntry) const
{
	int		iLen = GetWindowTextLengthA(GetDlgItem(dat->hwnd, IDC_MESSAGE));

	if(dat) {
		NotifyDeliveryFailure(dat);
		if (iLen == 0) {                    // message area is empty, so we can recall the failed message...
	#if defined(_UNICODE)
			SETTEXTEX stx = {ST_DEFAULT, 1200};
			if (m_jobs[iEntry].dwFlags & PREF_UNICODE)
				SendDlgItemMessage(dat->hwnd, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)&m_jobs[iEntry].sendBuffer[lstrlenA(m_jobs[iEntry].sendBuffer) + 1]);
			else {
				stx.codepage = (m_jobs[iEntry].dwFlags & PREF_UTF) ? CP_UTF8 : CP_ACP;
				SendDlgItemMessage(dat->hwnd, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)m_jobs[iEntry].sendBuffer);
			}
	#else
			SetDlgItemTextA(dat->hwnd, IDC_MESSAGE, (char *)m_jobs[iEntry].sendBuffer);
	#endif
			UpdateSaveAndSendButton(const_cast<_MessageWindowData *>(dat));
			SendDlgItemMessage(dat->hwnd, IDC_MESSAGE, EM_SETSEL, (WPARAM) - 1, (LPARAM) - 1);
		}
	}
}

void SendQueue::UpdateSaveAndSendButton(_MessageWindowData *dat)
{
	if(dat) {
		int		len;
		HWND	hwndDlg = dat->hwnd;

	#if defined(_UNICODE)
		GETTEXTLENGTHEX	gtxl = {0};
		gtxl.codepage = CP_UTF8;
		gtxl.flags = GTL_DEFAULT | GTL_PRECISE | GTL_NUMBYTES;

		len = SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETTEXTLENGTHEX, (WPARAM) & gtxl, 0);
	#else
		len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE));
	#endif
		if (len && GetSendButtonState(hwndDlg) == PBS_DISABLED)
			EnableSendButton(hwndDlg, TRUE);
		else if (len == 0 && GetSendButtonState(hwndDlg) != PBS_DISABLED)
			EnableSendButton(hwndDlg, FALSE);

		if (len) {          // looks complex but avoids flickering on the button while typing.
			if (!(dat->dwFlags & MWF_SAVEBTN_SAV)) {
				SendDlgItemMessage(hwndDlg, IDC_SAVE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) _Plugin.g_buttonBarIcons[7]);
				SendDlgItemMessage(hwndDlg, IDC_SAVE, BUTTONADDTOOLTIP, (WPARAM) pszIDCSAVE_save, 0);
				dat->dwFlags |= MWF_SAVEBTN_SAV;
			}
		}
		else {
			SendDlgItemMessage(hwndDlg, IDC_SAVE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) _Plugin.g_buttonBarIcons[6]);
			SendDlgItemMessage(hwndDlg, IDC_SAVE, BUTTONADDTOOLTIP, (WPARAM) pszIDCSAVE_close, 0);
			dat->dwFlags &= ~MWF_SAVEBTN_SAV;
		}
		dat->textLen = len;
		if (_Plugin.m_visualMessageSizeIndicator)
			InvalidateRect(GetDlgItem(hwndDlg, IDC_MSGINDICATOR), NULL, FALSE);
	}
}

static INT_PTR CALLBACK PopupDlgProcError(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)CallService(MS_POPUP_GETPLUGINDATA, (WPARAM)hWnd, (LPARAM)&hContact);

	switch (message) {
		case WM_COMMAND:
			PostMessage(_Plugin.g_hwndHotkeyHandler, DM_HANDLECLISTEVENT, (WPARAM)hContact, 0);
			PUDeletePopUp(hWnd);
			break;
		case WM_CONTEXTMENU:
			PostMessage(_Plugin.g_hwndHotkeyHandler, DM_HANDLECLISTEVENT, (WPARAM)hContact, 0);
			PUDeletePopUp(hWnd);
			break;
		case WM_MOUSEWHEEL:
			break;
		case WM_SETCURSOR:
			break;
		default:
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void SendQueue::NotifyDeliveryFailure(const _MessageWindowData *dat)
{
#if defined _UNICODE
	POPUPDATAW		ppd;
	int				ibsize = 1023;

	if(M->GetByte("adv_noErrorPopups", 0))
		return;

	if (CallService(MS_POPUP_QUERY, PUQS_GETSTATUS, 0) == 1) {
		ZeroMemory((void *)&ppd, sizeof(ppd));
		ppd.lchContact = dat->hContact;
		mir_sntprintf(ppd.lpwzContactName, MAX_CONTACTNAME, _T("%s"), dat->szNickname);
		mir_sntprintf(ppd.lpwzText, MAX_SECONDLINE, _T("%s"), TranslateT("A message delivery has failed.\nClick to open the message window."));
		ppd.colorText = RGB(255, 245, 225);
		ppd.colorBack = RGB(191, 0, 0);
		ppd.PluginWindowProc = (WNDPROC)PopupDlgProcError;
		ppd.lchIcon = _Plugin.g_iconErr;
		ppd.PluginData = (void *)dat->hContact;
		ppd.iSeconds = -1;
		CallService(MS_POPUP_ADDPOPUPW, (WPARAM)&ppd, 0);
	}
#else
	POPUPDATAEX	ppd;
	int			ibsize = 1023;

	if(M->GetByte("adv_noErrorPopups", 0))
		return;

	if (CallService(MS_POPUP_QUERY, PUQS_GETSTATUS, 0) == 1) {
		ZeroMemory((void *)&ppd, sizeof(ppd));
		ppd.lchContact = dat->hContact;
		mir_sntprintf(ppd.lpzContactName, MAX_CONTACTNAME, "%s", dat->szNickname);
		mir_sntprintf(ppd.lpzText, MAX_SECONDLINE, "%s", Translate("A message delivery has failed.\nClick to open the message window."));
		ppd.colorText = RGB(255, 245, 225);
		ppd.colorBack = RGB(191, 0, 0);
		ppd.PluginWindowProc = (WNDPROC)PopupDlgProcError;
		ppd.lchIcon = _Plugin.g_iconErr;
		ppd.PluginData = (void *)dat->hContact;
		ppd.iSeconds = -1;
		CallService(MS_POPUP_ADDPOPUP, (WPARAM)&ppd, 0);
	}
#endif
}

/*
static INT_PTR CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case WM_COMMAND:
			if (HIWORD(wParam) == STN_CLICKED) {
				HWND hwnd;
				struct _MessageWindowData *dat;


				hwnd = (HWND)CallService(MS_POPUP_GETPLUGINDATA, (WPARAM)hWnd, (LPARAM) & hwnd);
				dat = (struct _MessageWindowData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
				if (dat) {
					ActivateExistingTab(dat->pContainer, hwnd);
				}
				return TRUE;
			}
			break;
		case WM_CONTEXTMENU: {
			PUDeletePopUp(hWnd);
			return TRUE;
		}
		default:
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
*/

/*
 * searches string for characters typical for RTL text (hebrew and other RTL languages
*/

#if defined(_UNICODE)
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
#endif

int SendQueue::ackMessage(_MessageWindowData *dat, WPARAM wParam, LPARAM lParam)
{
	ACKDATA				*ack = (ACKDATA *) lParam;
	DBEVENTINFO			dbei = { 0};
	HANDLE				hNewEvent;
	int					i, iFound = SendQueue::NR_SENDJOBS, iNextFailed;
	ContainerWindowData *m_pContainer = 0;
	if (dat)
		m_pContainer = dat->pContainer;

	iFound = (int)(LOWORD(wParam));
	i = (int)(HIWORD(wParam));

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
			if (m_jobs[iFound].sendCount > 1) {        // multisend is different...
				char szErrMsg[256];
				mir_snprintf(szErrMsg, sizeof(szErrMsg), Translate("Multisend: failed sending to: %s"), dat->szProto);
#if defined(_UNICODE)
			{
				wchar_t wszErrMsg[256];
				MultiByteToWideChar(_Plugin.m_LangPackCP, 0, szErrMsg, -1, wszErrMsg, 256);
				wszErrMsg[255] = 0;
				logError(dat, -1, wszErrMsg);
			}
#else
				logError(dat, -1, szErrMsg);
#endif
				goto verify;
			}
			else {
				mir_snprintf(m_jobs[iFound].szErrorMsg, sizeof(m_jobs[iFound].szErrorMsg), Translate("Delivery failure: %s"), (char *)ack->lParam);
				m_jobs[iFound].iStatus = SQ_ERROR;
				KillTimer(dat->hwnd, TIMERID_MSGSEND + iFound);
				if (!(dat->dwFlags & MWF_ERRORSTATE))
					handleError(dat, iFound);
			}
			return 0;
		}
		else {
inform_and_discard:
			_DebugPopup(m_jobs[iFound].hOwner, TranslateT("A message delivery has failed after the contacts chat window was closed. You may want to resend the last message"));
			clearJob(iFound);
			return 0;
		}
	}

	dbei.cbSize = sizeof(dbei);
	dbei.eventType = EVENTTYPE_MESSAGE;
	dbei.flags = DBEF_SENT;
	dbei.szModule = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) m_jobs[iFound].hContact[i], 0);
	dbei.timestamp = time(NULL);
	dbei.cbBlob = lstrlenA(m_jobs[iFound].sendBuffer) + 1;

	if (dat) {
		dat->stats.iSentBytes += (dbei.cbBlob - 1);
		dat->stats.iSent++;
	}

#if defined( _UNICODE )
	if (m_jobs[iFound].dwFlags & PREF_UNICODE)
		dbei.cbBlob *= sizeof(TCHAR) + 1;
	if (m_jobs[iFound].dwFlags & PREF_RTL)
		dbei.flags |= DBEF_RTL;
	if (m_jobs[iFound].dwFlags & PREF_UTF)
		dbei.flags |= DBEF_UTF;
#endif
	dbei.pBlob = (PBYTE) m_jobs[iFound].sendBuffer;
	hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) m_jobs[iFound].hContact[i], (LPARAM) & dbei);

	if (m_pContainer) {
		if (!nen_options.iNoSounds && !(m_pContainer->dwFlags & CNT_NOSOUND))
			SkinPlaySound("SendMsg");
	}

/*
 * if this is a multisend job, AND the ack was from a different contact (not the session "owner" hContact)
 * then we print a small message telling the user that the message has been delivered to *foo*
 */

	if (dat) {
		if (m_jobs[iFound].hContact[i] != m_jobs[iFound].hOwner) {
			TCHAR szErrMsg[256];
			TCHAR *szReceiver = (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)m_jobs[iFound].hContact[i], GCDNF_TCHAR);
			mir_sntprintf(szErrMsg, safe_sizeof(szErrMsg), TranslateT("Multisend: successfully sent to: %s"), szReceiver);
			logError(dat, -1, szErrMsg);
		}
	}

	if (dat && (m_jobs[iFound].hContact[i] == dat->hContact)) {
		if (dat->hDbEventFirst == NULL) {
			dat->hDbEventFirst = hNewEvent;
			SendMessage(dat->hwnd, DM_REMAKELOG, 0, 0);
		}
	}
verify:
	m_jobs[iFound].hSendId[i] = NULL;
	m_jobs[iFound].hContact[i] = NULL;
	m_jobs[iFound].iAcksNeeded--;

	if (m_jobs[iFound].iAcksNeeded == 0) {              // everything sent
		if (m_jobs[iFound].sendCount > 1 && dat)
			EnableSending(dat, TRUE);
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
	return(MessageBox(0, TranslateT("There are unsent messages waiting for confirmation.\nWhen you close the window now, Miranda will try to send them but may be unable to inform you about possible delivery errors.\nDo you really want to close the Window(s)?"), TranslateT("Message Window warning"), MB_YESNO | MB_ICONHAND));
}
