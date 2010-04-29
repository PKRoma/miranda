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
 * (C) 2005-2010 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 * the sendlater class implementation
 *
 * TODO: create a simple UI to give the user control over the jobs in the
 *       send later queue.
 */

#include "commonheaders.h"
#pragma hdrstop

#if defined(_UNICODE)
	#define U_PREF_UNICODE PREF_UNICODE
#else
	#define U_PREF_UNICODE 0
#endif

INT_PTR CALLBACK PopupDlgProcError(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

/*
 * implementation of the CSendLaterJob class
 */

CSendLaterJob::CSendLaterJob()
{
	ZeroMemory(this, sizeof(CSendLaterJob));
	fSuccess = false;
}
CSendLaterJob::~CSendLaterJob()
{
	if(fSuccess || fFailed) {
		POPUPDATAT ppd = {0};

		if(!M->GetByte("adv_noSendLaterPopups", 0)) {
			/*
			 * show a popup notification, unless they are disabled
			 */
			if (PluginConfig.g_PopupAvail) {
				TCHAR	*tszName = (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR);

				ZeroMemory((void *)&ppd, sizeof(ppd));
				ppd.lchContact = hContact;
				ppd.cbSize = sizeof(ppd);
				mir_sntprintf(ppd.lptzContactName, MAX_CONTACTNAME, _T("%s"), tszName ? tszName : CTranslator::get(CTranslator::GEN_UNKNOWN_CONTACT));
				if(fSuccess && szId[0] == 'S') {
#if defined(_UNICODE)
					TCHAR *msgPreview = Utils::GetPreviewWithEllipsis(reinterpret_cast<TCHAR *>(&pBuf[lstrlenA((char *)pBuf) + 1]), 100);
#else
					TCHAR *msgPreview = Utils::GetPreviewWithEllipsis(reinterpret_cast<TCHAR *>(pBuf), 100);
#endif
					mir_sntprintf(ppd.lptzText, MAX_SECONDLINE, CTranslator::get(CTranslator::GEN_SQ_SENDLATER_SUCCESS_POPUP),
						msgPreview);
					mir_free(msgPreview);
				}
				ppd.colorText = fFailed ? M->GetByte("adv_PopupText_failedjob", RGB(255, 245, 225)) : M->GetByte("adv_PopupText_successjob", RGB(255, 245, 225));
				ppd.colorBack = fFailed ? M->GetByte("adv_PopupBG_failedjob", RGB(191, 0, 0)) : M->GetByte("adv_PopupBG_successjob", RGB(0, 121, 0));
				ppd.PluginWindowProc = (WNDPROC)PopupDlgProcError;
				ppd.lchIcon = PluginConfig.g_iconErr;
				ppd.PluginData = (void *)hContact;
				ppd.iSeconds = -1;
				CallService(MS_POPUP_ADDPOPUPW, (WPARAM)&ppd, 0);
			}
		}
		mir_free(sendBuffer);
		mir_free(pBuf);
	}
}

CSendLater::CSendLater()
{
	m_sendLaterContactList.clear();
	m_sendLaterJobList.clear();
	m_fAvail = M->GetByte("sendLaterAvail", 0) ? true : false;
	m_last_sendlater_processed = time(0);
	m_hwndDlg = 0;
}

/**
 * clear all open send jobs. Only called on system shutdown to remove
 * the jobs from memory. Must _NOT_ delete any sendlater related stuff from
 * the database (only successful sends may do this).
 */
CSendLater::~CSendLater()
{
	if(m_hwndDlg)
		::DestroyWindow(m_hwndDlg);

	if(m_sendLaterJobList.empty())
		return;

	SendLaterJobIterator it = m_sendLaterJobList.begin();

	while(it != m_sendLaterJobList.end()) {
		mir_free((*it)->sendBuffer);
		mir_free((*it)->pBuf);
		(*it)->fSuccess = false;					// avoid clearing jobs from the database
		delete *it;
		it++;
	}
}

/**
 * checks if the current job in the timer-based process queue is subject
 * for deletion (that is, it has failed or succeeded)
 * returns true if the job was deleted
 * 
 * if not, it will send the job and increment the list iterator.
 *
 * this method is called once per tick from the timer based scheduler in
 * hotkeyhandler.cpp.
 *
 * returns true if more jobs are awaiting processing, false otherwise.
 */
bool CSendLater::processCurrentJob()
{
	if(m_sendLaterJobList.empty() || m_jobIterator == m_sendLaterJobList.end())
		return(false);

	if((*m_jobIterator)->fSuccess || (*m_jobIterator)->fFailed) {
		//_DebugTraceA("Removing successful job %s", (*g_jobs)->szId);
		delete *m_jobIterator;
		m_jobIterator = m_sendLaterJobList.erase(m_jobIterator);
		qMgrUpdate();
		return(m_jobIterator == m_sendLaterJobList.end() ? false : true);
	}
	sendIt(*m_jobIterator);
	m_jobIterator++;
	return(m_jobIterator == m_sendLaterJobList.end() ? false : true);
}

/**
 * stub used as enum proc for the database enumeration, collecting
 * all entries in the SendLater module
 * (static function)
 */
int _cdecl CSendLater::addStub(const char *szSetting, LPARAM lParam)
{
	return(sendLater->addJob(szSetting, lParam));
}

/**
 * Process a single contact from the list of contacts with open send later jobs
 * enum the "SendLater" module and add all jobs to the list of open jobs.
 * SendLater_AddJob() will deal with possible duplicates
 * @param hContact HANDLE: contact's handle
 */
void CSendLater::processSingleContact(const HANDLE hContact)
{
	int iCount = M->GetDword(hContact, "SendLater", "count", 0);

	if(iCount) {
		DBCONTACTENUMSETTINGS ces = {0};

		ces.pfnEnumProc = CSendLater::addStub;
		ces.szModule = "SendLater";
		ces.lParam = (LPARAM)hContact;

		CallService(MS_DB_CONTACT_ENUMSETTINGS, (WPARAM)hContact, (LPARAM)&ces);
	}
}

/**
 * called periodically from a timer, check if new contacts were added 
 * and process them
 */
void CSendLater::processContacts()
{
	if(!m_sendLaterContactList.empty()) {
		std::vector<HANDLE>::iterator it = m_sendLaterContactList.begin();
		while(it != m_sendLaterContactList.end()) {
			processSingleContact(*it);
			it++;
		}
		m_sendLaterContactList.clear();
	}
}

/**
 * This function adds a new job to the list of messages to send unattended
 * used by the send later feature and multisend
 *
 * @param 	szSetting is either the name of the database key for a send later
 * 		  	job OR the utf-8 encoded message for a multisend job prefixed with
 * 			a 'M+timestamp'. Send later job ids start with "S".
 *
 * @param 	lParam: a contact handle for which the job should be scheduled
 * @return 	0 on failure, 1 otherwise
 */
int CSendLater::addJob(const char *szSetting, LPARAM lParam)
{
	HANDLE 		hContact = (HANDLE)lParam;
	DBVARIANT 	dbv = {0};
	char		*szOrig_Utf = 0;

	if(!szSetting || !strcmp(szSetting, "count") || lstrlenA(szSetting) < 8)
		return(0);

	if(szSetting[0] != 'S' && szSetting[0] != 'M')
		return(0);

	SendLaterJobIterator it = m_sendLaterJobList.begin();

	/*
	 * check for possible dupes
	 */
	while(it != m_sendLaterJobList.end()) {
		if((*it)->hContact == hContact && !strcmp((*it)->szId, szSetting)) {
			//_DebugTraceA("%s for %d is already on the job list.", szSetting, hContact);
			return(0);
		}
		it++;
	}

	if(szSetting[0] == 'S') {
		if(0 == DBGetContactSettingString(hContact, "SendLater", szSetting, &dbv))
			szOrig_Utf = dbv.pszVal;
		else
			return(0);
	}
	else if(szSetting[0] == 'M') {
		char *szSep = strchr(const_cast<char *>(szSetting), '|');
		if(!szSep)
			return(0);
		*szSep = 0;
		szOrig_Utf = szSep + 1;
	}
	else
		return(0);

	CSendLaterJob *job = new CSendLaterJob;

	strncpy(job->szId, szSetting, 20);
	job->szId[19] = 0;
	job->hContact = hContact;
	job->created = atol(&szSetting[1]);

	char	*szAnsi = 0;
	wchar_t *szWchar = 0;
	UINT	required = 0;

	int iLen = lstrlenA(szOrig_Utf);
	job->sendBuffer = reinterpret_cast<char *>(mir_alloc(iLen + 1));
	strncpy(job->sendBuffer, szOrig_Utf, iLen);
	job->sendBuffer[iLen] = 0;

	/*
	 * construct conventional send buffer
	 */

	szAnsi = M->utf8_decodecp(szOrig_Utf, CP_ACP, &szWchar);
	iLen = lstrlenA(szAnsi);
	if(szWchar)
		required = iLen + 1 + ((lstrlenW(szWchar) + 1) * sizeof(wchar_t));
	else
		required = iLen + 1;

	job->pBuf = (PBYTE)mir_calloc(required);

	strncpy((char *)job->pBuf, szAnsi, iLen);
	job->pBuf[iLen] = 0;
	if(szWchar)
		wcsncpy((wchar_t *)&job->pBuf[iLen + 1], szWchar, lstrlenW(szWchar));

	else if(szSetting[0] == 'S')
		DBFreeVariant(&dbv);

	mir_free(szWchar);

	m_sendLaterJobList.push_back(job);
	qMgrUpdate();
	return(1);
}

/**
 * Try to send an open job from the job list
 * this is ONLY called from the WM_TIMER handler and should never be executed
 * directly.
 */
int CSendLater::sendIt(CSendLaterJob *job)
{
	HANDLE 		hContact = job->hContact;
	time_t 		now = time(0);
	DWORD   	dwFlags = 0;
	DBVARIANT 	dbv = {0};
	const char* szProto = 0;


	if(job->fSuccess || job->fFailed || job->lastSent > now)
		return(0);											// this one is frozen or done (will be removed soon), don't process it now.

	if(now - job->created > SENDLATER_AGE_THRESHOLD) {		// too old, this will be discarded and user informed by popup
		job->fFailed = true;
		job->bCode = CSendLaterJob::JOB_AGE;
		return(0);
	}

	if(job->iSendCount == 5) {
		job->iSendCount = 0;
		job->lastSent = now + 3600;							// after 5 unsuccessful resends, freeze it for an hour
		job->bCode = CSendLaterJob::JOB_DEFERRED;
		return(0);
	}

	if(job->iSendCount > 0 && (now - job->lastSent < SENDLATER_RESEND_THRESHOLD)) {
		//_DebugTraceA("Send it: message %s for %s RESEND but not old enough", job->szId, (char *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0));
		return(0);											// this one was sent, but probably failed. Resend it after a while
	}

	CContactCache *c = CContactCache::getContactCache(hContact);
	if(!c)
		return(0);						// should not happen

	if(!c->isValid()) {
		job->fFailed = true;
		job->bCode = CSendLaterJob::INVALID_CONTACT;
		return(0);						// can happen (contact has been deleted). mark the job as failed
	}

	hContact = c->getActiveContact();
	szProto = c->getActiveProto();

	if(!hContact || szProto == 0)
		return(0);

	WORD wMyStatus = (WORD)CallProtoService(szProto, PS_GETSTATUS, 0, 0);
	WORD wContactStatus = c->getActiveStatus();

	/*
	 * status mode checks
	 */
	if(wMyStatus == ID_STATUS_OFFLINE) {
		job->bCode = CSendLaterJob::JOB_MYSTATUS;
		return(0);
	}
	if(job->szId[0] == 'S') {
		if(!(wMyStatus == ID_STATUS_ONLINE || wMyStatus == ID_STATUS_FREECHAT)) {
			job->bCode = CSendLaterJob::JOB_MYSTATUS;
			return(0);
		}
	}
	if(wContactStatus == ID_STATUS_OFFLINE) {
		job->bCode = CSendLaterJob::JOB_STATUS;
		return(0);
	}

	dwFlags = IsUtfSendAvailable(hContact) ? PREF_UTF : U_PREF_UNICODE;

	char *svcName = SendQueue::MsgServiceName(hContact, 0, dwFlags);

	job->lastSent = now;
	job->iSendCount++;
	job->hTargetContact = hContact;
	job->bCode = CSendLaterJob::JOB_WAITACK;

	if(dwFlags & PREF_UTF)
		job->hProcess = (HANDLE)CallContactService(hContact, svcName, dwFlags, (LPARAM)job->sendBuffer);
	else
		job->hProcess = (HANDLE)CallContactService(hContact, svcName, dwFlags, (LPARAM)job->pBuf);
	return(0);
}

/*
 * add a contact to the list of contacts having open send later jobs.
 * This ist is periodically checked for new additions (processContacts()) 
 * and new jobs are created.
 */

void CSendLater::addContact(const HANDLE hContact)
{
	if(!m_fAvail)
		return;

	std::vector<HANDLE>::iterator it = m_sendLaterContactList.begin();

	if(m_sendLaterContactList.empty()) {
		m_sendLaterContactList.push_back(hContact);
		m_last_sendlater_processed = 0;								// force processing at next tick
		return;
	}

	/*
	 * this list should not have duplicate entries
	 */

	while(it != m_sendLaterContactList.end()) {
		if(*it == hContact)
			return;
		it++;
	}
	m_sendLaterContactList.push_back(hContact);
	m_last_sendlater_processed = 0;								// force processing at next tick
}

/**
 * process ACK messages for the send later job list. Called from the proto ack
 * handler when it does not find a match in the normal send queue
 *
 * Add the message to the database and mark it as successful. The job will be
 * removed later by the job list processing code.
 */
HANDLE CSendLater::processAck(const ACKDATA *ack)
{
	if(m_sendLaterJobList.empty())
		return(0);

	SendLaterJobIterator it = m_sendLaterJobList.begin();

	while(it != m_sendLaterJobList.end()) {
		if((*it)->hProcess == ack->hProcess && (*it)->hTargetContact == ack->hContact && !((*it)->fSuccess || (*it)->fFailed)) {
			DBEVENTINFO dbei = {0};

			if(!(*it)->fSuccess) {
				//_DebugTraceA("ack for: hProcess: %d, job id: %s, job hProcess: %d", ack->hProcess, (*it)->szId, (*it)->hProcess);
				dbei.cbSize = sizeof(dbei);
				dbei.eventType = EVENTTYPE_MESSAGE;
				dbei.flags = DBEF_SENT;
				dbei.szModule = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)((*it)->hContact), 0);
				dbei.timestamp = time(NULL);
				dbei.cbBlob = lstrlenA((*it)->sendBuffer) + 1;

	#if defined( _UNICODE )
				dbei.flags |= DBEF_UTF;
	#endif
				dbei.pBlob = (PBYTE)((*it)->sendBuffer);
				HANDLE hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM)((*it)->hContact), (LPARAM)&dbei);

				if((*it)->szId[0] == 'S') {
					DBDeleteContactSetting((*it)->hContact, "SendLater", (*it)->szId);
					int iCount = M->GetDword((*it)->hContact, "SendLater", "count", 0);
					if(iCount)
						iCount--;
					M->WriteDword((*it)->hContact, "SendLater", "count", iCount);
				}
			}
			(*it)->fSuccess = true;					// mark as successful, job list processing code will remove it later
			(*it)->hProcess = (HANDLE)-1;
			(*it)->bCode = '-';
			qMgrUpdate();
			return(0);
		}
		it++;
	}
	return(0);
}

/*
 * UI stuff (dialog procedures for the queue manager dialog
 */

void CSendLater::qMgrUpdate()
{
	if(m_hwndDlg)
		::SendMessage(m_hwndDlg, WM_USER + 100, 0, 0);		// if qmgr is open, tell it to update
}

LRESULT CSendLater::qMgrAddFilter(const HANDLE hContact, const TCHAR* tszNick)
{
	LRESULT lr;

	lr = ::SendMessage(m_hwndFilter, CB_FINDSTRING, 0, reinterpret_cast<LPARAM>(tszNick));
	if(lr == CB_ERR) {
		lr = ::SendMessage(m_hwndFilter, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(tszNick));
		::SendMessage(m_hwndFilter, CB_SETITEMDATA, lr, reinterpret_cast<LPARAM>(hContact));
		if(hContact == m_hFilter)
			m_sel = lr;
	}
	return(m_sel);
}

/**
 * fills the list of jobs with current contents of the job queue
 * filters by m_hFilter (contact handle)
 *
 * TODO: sorting
 */
void CSendLater::qMgrFillList(bool fClear)
{
	LVITEM			lvItem = {0};
	unsigned		uIndex = 0;
	CContactCache*	c = 0;
	TCHAR*			formatTime = _T("%Y.%m.%d - %H:%M");
	TCHAR			tszTimestamp[30];
	TCHAR			tszStatus[20];
	const TCHAR*	tszStatusText = 0;
	BYTE			bCode = '-';

	if(fClear) {
		::SendMessage(m_hwndList, LVM_DELETEALLITEMS, 0, 0);
		::SendMessage(m_hwndFilter, CB_RESETCONTENT, 0, 0);
	}

	m_sel = 0;
	::SendMessage(m_hwndFilter, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(CTranslator::get(CTranslator::QMGR_FILTER_ALLCONTACTS)));
	::SendMessage(m_hwndFilter, CB_SETITEMDATA, 0, 0);

	lvItem.cchTextMax = 255;
	lvItem.mask = LVIF_TEXT;

	SendLaterJobIterator it = m_sendLaterJobList.begin();

	while(it != m_sendLaterJobList.end()) {

		c = CContactCache::getContactCache((*it)->hContact);
		if(c) {
			const TCHAR*	tszNick = c->getNick();

			if(m_hFilter && m_hFilter != (*it)->hContact) {
				qMgrAddFilter(c->getContact(), tszNick);
				it++;
				continue;
			}

			lvItem.pszText = const_cast<TCHAR *>(tszNick);
			lvItem.iItem = uIndex++;
			lvItem.iSubItem = 0;
			::SendMessage(m_hwndList, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&lvItem));

			qMgrAddFilter(c->getContact(), tszNick);
			_tcsftime(tszTimestamp, 30, formatTime, _localtime32((__time32_t *)&(*it)->created));
			tszTimestamp[29] = 0;
			lvItem.pszText = tszTimestamp;
			lvItem.iSubItem = 1;
			::SendMessage(m_hwndList, LVM_SETITEM, 0, reinterpret_cast<LPARAM>(&lvItem));

			TCHAR* msg = M->utf8_decodeT((*it)->sendBuffer);
			TCHAR* preview = Utils::GetPreviewWithEllipsis(msg, 255);
			lvItem.pszText = preview;
			lvItem.iSubItem = 2;
			::SendMessage(m_hwndList, LVM_SETITEM, 0, reinterpret_cast<LPARAM>(&lvItem));
			mir_free(preview);
			mir_free(msg);

			if((*it)->fFailed)
				tszStatusText = CTranslator::get(CTranslator::QMGR_STATUS_FAILED);
			else if((*it)->fSuccess)
				tszStatusText = CTranslator::get(CTranslator::QMGR_STATUS_SENTOK);
			else {
				switch((*it)->bCode) {
					case 'A':
						tszStatusText = CTranslator::get(CTranslator::QMGR_STATUS_WAITACK);
						break;
					default:
						tszStatusText = CTranslator::get(CTranslator::QMGR_STATUS_PENDING);
						break;
				}
			}
			if((*it)->bCode)
				bCode = (*it)->bCode;
			mir_sntprintf(tszStatus, 20, _T("X/%s[%c] (%d)"), tszStatusText, bCode, (*it)->iSendCount);
			tszStatus[0] = static_cast<TCHAR>((*it)->szId[0]);
			lvItem.pszText = tszStatus;
			lvItem.iSubItem = 3;
			::SendMessage(m_hwndList, LVM_SETITEM, 0, reinterpret_cast<LPARAM>(&lvItem));

			if((*it)->lastSent == 0)
				mir_sntprintf(tszTimestamp, 30, _T("%s"), _T("Never"));
			else {
				_tcsftime(tszTimestamp, 30, formatTime, _localtime32((__time32_t *)&(*it)->lastSent));
				tszTimestamp[29] = 0;
			}
			lvItem.pszText = tszTimestamp;
			lvItem.iSubItem = 4;
			::SendMessage(m_hwndList, LVM_SETITEM, 0, reinterpret_cast<LPARAM>(&lvItem));
		}
		it++;
	}
	if(m_hFilter == 0)
		::SendMessage(m_hwndFilter, CB_SETCURSEL, 0, 0);
	else
		::SendMessage(m_hwndFilter, CB_SETCURSEL, m_sel, 0);
}

/*
 * set the column headers
 */
#define QMGR_LIST_NRCOLUMNS 5

static char*  szColFormat = "%d;%d;%d;%d;%d";
static char*  szColDefault = "80;120;80;80;120";

void CSendLater::qMgrSetupColumns()
{
	LVCOLUMN	col = {0};
	int			nWidths[QMGR_LIST_NRCOLUMNS];
	DBVARIANT	dbv = {0};
	RECT		rcList;
	LONG		cxList;

	::GetWindowRect(m_hwndList, &rcList);
	cxList = rcList.right - rcList.left;

	if(0 == M->GetString(0, SRMSGMOD_T, "qmgrListColumns", &dbv)) {
		sscanf(dbv.pszVal, szColFormat, &nWidths[0], &nWidths[1], &nWidths[2], &nWidths[3], &nWidths[4]);
		DBFreeVariant(&dbv);
	}
	else
		sscanf(szColDefault, szColFormat, &nWidths[0], &nWidths[1], &nWidths[2], &nWidths[3], &nWidths[4]);

	col.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
	col.cx = max(nWidths[0], 10);      // width TODO save/restore user-defined values
	col.pszText = const_cast<TCHAR *>(CTranslator::get(CTranslator::GEN_CONTACT));

	::SendMessage(m_hwndList, LVM_INSERTCOLUMN, 0, reinterpret_cast<LPARAM>(&col));

	col.pszText = const_cast<TCHAR *>(CTranslator::get(CTranslator::QMGR_COL_ODATE));
	col.cx = max(nWidths[1], 10);
	::SendMessage(m_hwndList, LVM_INSERTCOLUMN, 1, reinterpret_cast<LPARAM>(&col));

	col.pszText = const_cast<TCHAR *>(CTranslator::get(CTranslator::QMGR_COL_MESSAGETEXT));
	col.cx = max((cxList - nWidths[0] - nWidths[1] - nWidths[3] - nWidths[4] - 10), 10);
	::SendMessage(m_hwndList, LVM_INSERTCOLUMN, 2, reinterpret_cast<LPARAM>(&col));

	col.pszText = const_cast<TCHAR *>(CTranslator::get(CTranslator::QMGR_COL_STATUS));
	col.cx = max(nWidths[3], 10);
	::SendMessage(m_hwndList, LVM_INSERTCOLUMN, 3, reinterpret_cast<LPARAM>(&col));

	col.pszText = const_cast<TCHAR *>(CTranslator::get(CTranslator::QMGR_COL_LASTSENDINFO));
	col.cx = max(nWidths[4], 10);
	::SendMessage(m_hwndList, LVM_INSERTCOLUMN, 4, reinterpret_cast<LPARAM>(&col));

}

/**
 * save user defined column widths to the database
 */
void CSendLater::qMgrSaveColumns()
{
	char		szColFormatNew[100];
	int			nWidths[QMGR_LIST_NRCOLUMNS], i;
	LVCOLUMN	col = {0};

	col.mask = LVCF_WIDTH;
	for(i = 0; i < QMGR_LIST_NRCOLUMNS; i++) {
		::SendMessage(m_hwndList, LVM_GETCOLUMN, i, reinterpret_cast<LPARAM>(&col));
		nWidths[i] = max(col.cx, 10);
	}
	mir_snprintf(szColFormatNew, 100, "%d;%d;%d;%d;%d", nWidths[0], nWidths[1], nWidths[2], nWidths[3], nWidths[4]);
	::DBWriteContactSettingString(0, SRMSGMOD_T, "qmgrListColumns", szColFormatNew);
}

INT_PTR CALLBACK CSendLater::DlgProcStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CSendLater *s = reinterpret_cast<CSendLater *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));

	if(s)
		return(s->DlgProc(hwnd, msg, wParam, lParam));

	switch(msg) {
		case WM_INITDIALOG: {
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
			s = reinterpret_cast<CSendLater *>(lParam);
			return(s->DlgProc(hwnd, msg, wParam, lParam));
		}
		default:
			break;
	}
	return(FALSE);
}

INT_PTR CALLBACK CSendLater::DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_INITDIALOG:
			m_hwndDlg = hwnd;
			TranslateDialogDefault(hwnd);
			m_hwndList = ::GetDlgItem(m_hwndDlg, IDC_QMGR_LIST);
			m_hwndFilter = ::GetDlgItem(m_hwndDlg, IDC_QMGR_FILTER);
			m_hFilter = reinterpret_cast<HANDLE>(M->GetDword(0, SRMSGMOD_T, "qmgrFilterContact", 0));

			::SendMessage(m_hwndList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
			qMgrSetupColumns();
			qMgrFillList();
			::ShowWindow(hwnd, SW_NORMAL);
			return(FALSE);

		case WM_COMMAND:
			if(HIWORD(wParam) == CBN_SELCHANGE && reinterpret_cast<HWND>(lParam) == m_hwndFilter) {
				LRESULT lr = ::SendMessage(m_hwndFilter, CB_GETCURSEL, 0, 0);
				if(lr != CB_ERR) {
					m_hFilter = reinterpret_cast<HANDLE>(::SendMessage(m_hwndFilter, CB_GETITEMDATA, lr, 0));
					qMgrFillList();
				}
				break;
			}
			switch(LOWORD(wParam)) {
				case IDOK:
				case IDCANCEL:
					qMgrSaveColumns();
					::DestroyWindow(hwnd);
					break;
			}
			break;

		case WM_USER + 100:
			qMgrFillList();
			break;
		case WM_NCDESTROY: {
			m_hwndDlg = 0;
			M->WriteDword(0, SRMSGMOD_T, "qmgrFilterContact", reinterpret_cast<DWORD>(m_hFilter));
			break;
		}
	}
	return(FALSE);
}

void CSendLater::invokeQueueMgrDlg()
{
	if(m_hwndDlg == 0) {
		m_hwndDlg = ::CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SENDLATER_QMGR), 0, CSendLater::DlgProcStub, 
										reinterpret_cast<LPARAM>(this));
	}
}

/*
 * service function to invoke the queue manager
 */

INT_PTR CSendLater::svcQMgr(WPARAM wParam, LPARAM lParam)
{
	sendLater->invokeQueueMgrDlg();
	return(0);
}


CSendLater* sendLater = 0;
