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
 * (C) 2005-2013 by silvercircle _at_ gmail _dot_ com and contributors
 *
 * $Id$
 *
 * the sendlater class implementation
 */

#include "commonheaders.h"
#pragma hdrstop

#define U_PREF_UNICODE PREF_UNICODE
/*
 * implementation of the CSendLaterJob class
 */

CSendLaterJob::CSendLaterJob()
{
	ZeroMemory(this, sizeof(CSendLaterJob));
	fSuccess = false;
}

/**
 * return true if this job is persistent (saved to the database).
 * such a job will survive a restart of Miranda
 */
bool CSendLaterJob::isPersistentJob()
{
	return(szId[0] == 'S' ? true : false);
}

/**
 * check conditions for deletion
 */
bool CSendLaterJob::mustDelete()
{
	if(fSuccess)
		return(true);
	else if(fFailed) {
		if(bCode == JOB_REMOVABLE)
			return(true);
	}
	return(false);
}

/**
 * clean database entries for a persistent job (currently: manual send later jobs)
 */
void CSendLaterJob::cleanDB()
{
	if(isPersistentJob()) {
		char	szKey[100];

		DBDeleteContactSetting(hContact, "SendLater", szId);
		int iCount = M->GetDword(hContact, "SendLater", "count", 0);
		if(iCount)
			iCount--;
		M->WriteDword(hContact, "SendLater", "count", iCount);
		/*
		 * delete flags
		 */
		mir_snprintf(szKey, 100, "$%s", szId);
		DBDeleteContactSetting(hContact, "SendLater", szKey);
	}
}

/**
 * read flags for a persistent jobs from the db
 * flag key name is the job id with a "$" prefix.
 */
void CSendLaterJob::readFlags()
{
	if(isPersistentJob()) {
		char	szKey[100];
		DWORD	localFlags;

		mir_snprintf(szKey, 100, "$%s", szId);
		localFlags = M->GetDword(hContact, "SendLater", szKey, 0);

		if(localFlags & SLF_SUSPEND)
			bCode = JOB_HOLD;
	}
}

/**
 * write flags for a persistent jobs from the db
 * flag key name is the job id with a "$" prefix.
 */
void CSendLaterJob::writeFlags()
{
	if(isPersistentJob()) {
		DWORD localFlags = (bCode == JOB_HOLD ? SLF_SUSPEND : 0);
		char	szKey[100];

		mir_snprintf(szKey, 100, "$%s", szId);
		M->WriteDword(hContact, "SendLater", szKey, localFlags);
	}
}

/**
 * delete a send later job
 */
CSendLaterJob::~CSendLaterJob()
{
	if(fSuccess || fFailed) {
		POPUPDATAT_V2 ppd = {0};

		if((sendLater->haveErrorPopups() && fFailed) || (sendLater->haveSuccessPopups() && fSuccess)) {
			bool fShowPopup = true;

			if(fFailed && bCode == JOB_REMOVABLE)	// no popups for jobs removed on user's request
				fShowPopup = false;
			/*
			 * show a popup notification, unless they are disabled
			 */
			if (PluginConfig.g_PopupAvail && fShowPopup) {
				TCHAR	*tszName = (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, GCDNF_TCHAR);

				ZeroMemory((void *)&ppd, sizeof(ppd));
				ppd.lchContact = hContact;
				ppd.cbSize = sizeof(ppd);
				mir_sntprintf(ppd.lptzContactName, MAX_CONTACTNAME, _T("%s"), tszName ? tszName : CTranslator::get(CTranslator::GEN_UNKNOWN_CONTACT));
				TCHAR *msgPreview = Utils::GetPreviewWithEllipsis(reinterpret_cast<TCHAR *>(&pBuf[lstrlenA((char *)pBuf) + 1]), 100);
				if(fSuccess) {
					mir_sntprintf(ppd.lptzText, MAX_SECONDLINE, CTranslator::get(CTranslator::GEN_SQ_SENDLATER_SUCCESS_POPUP),
								  msgPreview);
					mir_free(msgPreview);
				}
				else if(fFailed) {
					mir_sntprintf(ppd.lptzText, MAX_SECONDLINE, CTranslator::get(CTranslator::GEN_SQ_SENDLATER_FAILED_POPUP),
						msgPreview);
					mir_free(msgPreview);
				}
				/*
				 * use message settings (timeout/colors) for success popups
				 */
				ppd.colorText = fFailed ? RGB(255, 245, 225) : nen_options.colTextMsg;
				ppd.colorBack = fFailed ? RGB(191, 0, 0) : nen_options.colBackMsg;
				ppd.PluginWindowProc = reinterpret_cast<WNDPROC>(Utils::PopupDlgProcError);
				ppd.lchIcon = fFailed ? PluginConfig.g_iconErr : PluginConfig.g_IconMsgEvent;
				ppd.PluginData = (void *)hContact;
				ppd.iSeconds = fFailed ? -1 : nen_options.iDelayMsg;
				CallService(MS_POPUP_ADDPOPUPW, (WPARAM)&ppd, 0);
			}
		}
		if(fFailed && (bCode == JOB_AGE || bCode == JOB_REMOVABLE) && szId[0] == 'S')
			cleanDB();
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
	m_fIsInteractive = false;
	m_fErrorPopups = M->GetByte(0, SRMSGMOD_T, "qmgrErrorPopups", 0) ? true : false;
	m_fSuccessPopups = M->GetByte(0, SRMSGMOD_T, "qmgrSuccessPopups", 0) ? true : false;
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

void CSendLater::startJobListProcess()
{ 
	m_jobIterator = m_sendLaterJobList.begin(); 
			
	if (m_hwndDlg)
		Utils::enableDlgControl(m_hwndDlg, IDC_QMGR_LIST, false);
}

/**
 * checks if the current job in the timer-based process queue is subject
 * for deletion (that is, it has failed or succeeded)
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
		if((*m_jobIterator)->mustDelete()) {
			delete *m_jobIterator;
			m_jobIterator = m_sendLaterJobList.erase(m_jobIterator);
		}
		else
			m_jobIterator++;
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
 * addJob() will deal with possible duplicates
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
	if(m_fAvail && !m_sendLaterContactList.empty()) {
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

	if(!m_fAvail || !szSetting || !strcmp(szSetting, "count") || lstrlenA(szSetting) < 8)
		return(0);

	if(szSetting[0] != 'S' && szSetting[0] != 'M')
		return(0);

	SendLaterJobIterator it = m_sendLaterJobList.begin();

	/*
	 * check for possible dupes
	 */
	while(it != m_sendLaterJobList.end()) {
		if((*it)->hContact == hContact && !strcmp((*it)->szId, szSetting)) {
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

	if(szSetting[0] == 'S')
		DBFreeVariant(&dbv);

	mir_free(szWchar);
	job->readFlags();
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


	if(job->bCode == CSendLaterJob::JOB_HOLD || job->bCode == CSendLaterJob::JOB_DEFERRED || job->fSuccess || job->fFailed || job->lastSent > now)
		return(0);											// this one is frozen or done (will be removed soon), don't process it now.

	if(now - job->created > SENDLATER_AGE_THRESHOLD) {		// too old, this will be discarded and user informed by popup
		job->fFailed = true;
		job->bCode = CSendLaterJob::JOB_AGE;
		return(0);
	}

	/*
	 * mark job as deferred (5 unsuccessful sends). Job will not be removed, but 
	 * the user must manually reset it in order to trigger a new send attempt.
	 */
	if(job->iSendCount == 5) {
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
	if(m_sendLaterJobList.empty() || !m_fAvail)
		return(0);

	SendLaterJobIterator it = m_sendLaterJobList.begin();

	while(it != m_sendLaterJobList.end()) {
		if((*it)->hProcess == ack->hProcess && (*it)->hTargetContact == ack->hContact && !((*it)->fSuccess || (*it)->fFailed)) {
			DBEVENTINFO dbei = {0};

			if(!(*it)->fSuccess) {
				dbei.cbSize = sizeof(dbei);
				dbei.eventType = EVENTTYPE_MESSAGE;
				dbei.flags = DBEF_SENT;
				dbei.szModule = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)((*it)->hContact), 0);
				dbei.timestamp = time(NULL);
				dbei.cbBlob = lstrlenA((*it)->sendBuffer) + 1;
				dbei.flags |= DBEF_UTF;
				dbei.pBlob = (PBYTE)((*it)->sendBuffer);
				HANDLE hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM)((*it)->hContact), (LPARAM)&dbei);
				
				(*it)->cleanDB();
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

void CSendLater::qMgrUpdate(bool fReEnable)
{
	if(m_hwndDlg) {
		if(fReEnable)
			Utils::enableDlgControl(m_hwndDlg, IDC_QMGR_LIST, true);
		::SendMessage(m_hwndDlg, WM_USER + 100, 0, 0);		// if qmgr is open, tell it to update
	}
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
	::SendMessage(m_hwndFilter, CB_INSERTSTRING, -1, 
				  reinterpret_cast<LPARAM>(CTranslator::get(CTranslator::QMGR_FILTER_ALLCONTACTS)));
	::SendMessage(m_hwndFilter, CB_SETITEMDATA, 0, 0);

	lvItem.cchTextMax = 255;

	SendLaterJobIterator it = m_sendLaterJobList.begin();

	while(it != m_sendLaterJobList.end()) {

		c = CContactCache::getContactCache((*it)->hContact);
		if(c) {
			const TCHAR*	tszNick = c->getNick();
			TCHAR			tszBuf[255];

			if(m_hFilter && m_hFilter != (*it)->hContact) {
				qMgrAddFilter(c->getContact(), tszNick);
				it++;
				continue;
			}
			lvItem.mask = LVIF_TEXT|LVIF_PARAM;
			mir_sntprintf(tszBuf, 255, _T("%s [%s]"), tszNick, c->getRealAccount());
			lvItem.pszText = tszBuf;
			lvItem.iItem = uIndex++;
			lvItem.iSubItem = 0;
			lvItem.lParam = reinterpret_cast<LPARAM>(*it);
			::SendMessage(m_hwndList, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&lvItem));
			qMgrAddFilter(c->getContact(), tszNick);

			lvItem.mask = LVIF_TEXT;
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

			if((*it)->fFailed) {
				tszStatusText = (*it)->bCode == CSendLaterJob::JOB_REMOVABLE ? 
					CTranslator::get(CTranslator::QMGR_STATUS_REMOVED) : CTranslator::get(CTranslator::QMGR_STATUS_FAILED);
			}
			else if((*it)->fSuccess)
				tszStatusText = CTranslator::get(CTranslator::QMGR_STATUS_SENTOK);
			else {
				switch((*it)->bCode) {
					case CSendLaterJob::JOB_DEFERRED:
						tszStatusText = CTranslator::get(CTranslator::QMGR_STATUS_DEFERRED);
						break;
					case CSendLaterJob::JOB_AGE:
						tszStatusText = CTranslator::get(CTranslator::QMGR_STATUS_FAILED);
						break;
					case CSendLaterJob::JOB_HOLD:
						tszStatusText = CTranslator::get(CTranslator::QMGR_STATUS_HOLD);
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
static char*  szColDefault = "100;120;80;120;120";

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
	col.cx = max(nWidths[0], 10);
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

			::SetWindowLongPtr(m_hwndList, GWL_STYLE, ::GetWindowLongPtr(m_hwndList, GWL_STYLE) | LVS_SHOWSELALWAYS);
			::SendMessage(m_hwndList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_LABELTIP|LVS_EX_DOUBLEBUFFER);
			qMgrSetupColumns();
			qMgrFillList();
			if(PluginConfig.g_PopupAvail) {
				::CheckDlgButton(m_hwndDlg, IDC_QMGR_SUCCESSPOPUPS, m_fSuccessPopups ? BST_CHECKED : BST_UNCHECKED);
				::CheckDlgButton(m_hwndDlg, IDC_QMGR_ERRORPOPUPS, m_fErrorPopups ? BST_CHECKED : BST_UNCHECKED);
			}
			else {
				Utils::showDlgControl(m_hwndDlg, IDC_QMGR_ERRORPOPUPS, SW_HIDE);
				Utils::showDlgControl(m_hwndDlg, IDC_QMGR_SUCCESSPOPUPS, SW_HIDE);
			}
			::ShowWindow(hwnd, SW_NORMAL);
			return(FALSE);

		case WM_NOTIFY:	{
			NMHDR*	pNMHDR = reinterpret_cast<NMHDR *>(lParam);

			if(pNMHDR->hwndFrom == m_hwndList) {
				switch(pNMHDR->code) {
					case NM_RCLICK: {
						HMENU hMenu = ::LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_TABCONTEXT));
						HMENU hSubMenu = ::GetSubMenu(hMenu, 13);
						::TranslateMenu(hSubMenu);

						POINT pt;
						::GetCursorPos(&pt);
						/*
						 * copy to clipboard only allowed with a single selection
						 */
						if(::SendMessage(m_hwndList, LVM_GETSELECTEDCOUNT, 0, 0) == 1)
							::EnableMenuItem(hSubMenu, ID_QUEUEMANAGER_COPYMESSAGETOCLIPBOARD, MF_ENABLED);

						m_fIsInteractive = true;
						int selection = ::TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hwndDlg, NULL);
						if(selection == ID_QUEUEMANAGER_CANCELALLMULTISENDJOBS) {
							SendLaterJobIterator it = m_sendLaterJobList.begin();
							while(it != m_sendLaterJobList.end()) {
								if((*it)->szId[0] == 'M') {
									(*it)->fFailed = true;
									(*it)->bCode = CSendLaterJob::JOB_REMOVABLE;
								}
								it++;
							}
						}
						else if(selection != 0) {
							::SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_QMGR_REMOVE, LOWORD(selection)), 0);
							m_last_sendlater_processed = 0;			// force a queue check
						}
						::DestroyMenu(hMenu);
						m_fIsInteractive = false;
						break;
					}
					default:
						break;
				}
			}
			break;
		}

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

				case IDC_QMGR_SUCCESSPOPUPS:
					m_fSuccessPopups = ::IsDlgButtonChecked(m_hwndDlg, IDC_QMGR_SUCCESSPOPUPS) ? true : false;
					M->WriteByte(0, SRMSGMOD_T, "qmgrSuccessPopups", m_fSuccessPopups ? 1 : 0);
					break;

				case IDC_QMGR_ERRORPOPUPS:
					m_fErrorPopups = ::IsDlgButtonChecked(m_hwndDlg, IDC_QMGR_ERRORPOPUPS) ? true : false;
					M->WriteByte(0, SRMSGMOD_T, "qmgrErrorPopups", m_fErrorPopups ? 1 : 0);
					break;

				case IDC_QMGR_HELP:
					CallService(MS_UTILS_OPENURL, 0, reinterpret_cast<LPARAM>("http://wiki.miranda.or.at/TabSRMM/SendLater"));
					break;
				/*
				 * this handles all commands sent by the context menu
				 * mark jobs for removal/reset/hold/unhold
				 * exception: kill all open multisend jobs is directly handled from the context menu
				 */
				case IDC_QMGR_REMOVE: {
					if(::SendMessage(m_hwndList, LVM_GETSELECTEDCOUNT, 0, 0) != 0) {
						LVITEM item = {0};
						LRESULT	items = ::SendMessage(m_hwndList, LVM_GETITEMCOUNT, 0, 0);
						item.mask = LVIF_STATE|LVIF_PARAM;
						item.stateMask = LVIS_SELECTED;

						if(HIWORD(wParam) != ID_QUEUEMANAGER_COPYMESSAGETOCLIPBOARD) {
							if(MessageBox(0, CTranslator::get(CTranslator::QMGR_WARNING_REMOVAL), CTranslator::get(CTranslator::QMGR_TITLE),
										  MB_ICONQUESTION | MB_OKCANCEL) == IDCANCEL)
								break;
						}
						for(LRESULT i = 0; i < items; i++) {
							item.iItem = i;
							::SendMessage(m_hwndList, LVM_GETITEM, 0, reinterpret_cast<LPARAM>(&item));
							if(item.state & LVIS_SELECTED) {
								CSendLaterJob* job = reinterpret_cast<CSendLaterJob *>(item.lParam);
								if(job) {
									switch(HIWORD(wParam)) {
										case ID_QUEUEMANAGER_MARKSELECTEDFORREMOVAL:
											job->bCode = CSendLaterJob::JOB_REMOVABLE;
											job->fFailed = true;
											break;
										case ID_QUEUEMANAGER_HOLDSELECTED:
											job->bCode = CSendLaterJob::JOB_HOLD;
											job->writeFlags();
											break;
										case ID_QUEUEMANAGER_RESUMESELECTED:
											job->bCode = 0;
											job->writeFlags();
											break;
										case ID_QUEUEMANAGER_COPYMESSAGETOCLIPBOARD: {
											TCHAR *msg = M->utf8_decodeT(job->sendBuffer);
											Utils::CopyToClipBoard(msg, m_hwndDlg);
											mir_free(msg);
											break;
										}
										/*
										 * reset deferred and aged jobs
										 * so they can be resent at next processing
										 */
										case ID_QUEUEMANAGER_RESETSELECTED: {
											if(job->bCode == CSendLaterJob::JOB_DEFERRED) {
												job->iSendCount = 0;
												job->bCode = '-';
											}
											else if(job->bCode == CSendLaterJob::JOB_AGE) {
												job->fFailed = false;
												job->bCode = '-';
												job->created = time(0);
											}
											break;
										}
										default:
											break;
									}
								}
							}
						}
						qMgrFillList();
					}
					break;
				 }
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

/**
 * invoke queue manager dialog - do nothing if this dialog is already open
 */
void CSendLater::invokeQueueMgrDlg()
{
	if(m_hwndDlg == 0)
		m_hwndDlg = ::CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SENDLATER_QMGR), 0, CSendLater::DlgProcStub, 
										reinterpret_cast<LPARAM>(this));
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
