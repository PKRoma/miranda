/*
 * sendqueue.c
 * implements a queued send system 
 * part of tabSRMM, (C) 2004-2005 by Miranda IM project
 * $Id$
 */

#include "commonheaders.h"
#pragma hdrstop
#include "sendqueue.h"

extern      MYGLOBALS myGlobals;
extern      TCHAR *pszIDCSAVE_save, *pszIDCSAVE_close;
extern      const UINT errorControls[5], infoPanelControls[8];
extern      struct SendJob *sendJobs;
extern      NEN_OPTIONS nen_options;

static char *pss_msg = "/SendMsg";
static char *pss_msgw = "/SendMsgW";

char *MsgServiceName(HANDLE hContact, struct MessageWindowData *dat, int dwFlags)
{
#ifdef _UNICODE
    char szServiceName[100];
    char *szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
    if (szProto == NULL)
        return pss_msg;

    if(dat->sendMode & SMODE_FORCEANSI || !(dwFlags & PREF_UNICODE))
        return pss_msg;
    
    _snprintf(szServiceName, sizeof(szServiceName), "%s%sW", szProto, PSS_MESSAGE);
    if (ServiceExists(szServiceName))
        return pss_msgw;
#endif
    return pss_msg;
}

#define MS_INITIAL_DELAY 500

static DWORD WINAPI DoMultiSend(LPVOID param)
{
    int iIndex = (int)param;
    HWND hwndOwner = sendJobs[iIndex].hwndOwner;
    DWORD dwDelay = MS_INITIAL_DELAY;               // start with 1sec delay...
    DWORD dwDelayAdd = 0;
    struct MessageWindowData *dat = (struct MessageWindowData *)GetWindowLong(hwndOwner, GWL_USERDATA);
    int i;
    
    for(i = 0; i < sendJobs[iIndex].sendCount; i++) {
		sendJobs[iIndex].hSendId[i] = (HANDLE) CallContactService(sendJobs[iIndex].hContact[i], MsgServiceName(sendJobs[iIndex].hContact[i], dat, sendJobs[iIndex].dwFlags), (dat->sendMode & SMODE_FORCEANSI) ? (sendJobs[iIndex].dwFlags & ~PREF_UNICODE) : sendJobs[iIndex].dwFlags, (LPARAM) sendJobs[iIndex].sendBuffer);
        SetTimer(sendJobs[iIndex].hwndOwner, TIMERID_MULTISEND_BASE + (iIndex * SENDJOBS_MAX_SENDS) + i, myGlobals.m_MsgTimeout, NULL);
        Sleep((50 * i) + dwDelay + dwDelayAdd);
        if(i > 2)
            dwDelayAdd = 500;
        if(i > 8)
            dwDelayAdd = 1000;
        if(i > 14)
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

int FindNextFailedMsg(HWND hwndDlg, struct MessageWindowData *dat)
{
    int i;

    for(i = 0; i < NR_SENDJOBS; i++) {
        if(sendJobs[i].hOwner == dat->hContact && sendJobs[i].sendCount > 0 && sendJobs[i].iStatus == SQ_ERROR)
            return i;
    }
    return -1;
}
void HandleQueueError(HWND hwndDlg, struct MessageWindowData *dat, int iEntry) 
{
    char szErrorMsg[512];
    
    dat->iCurrentQueueError = iEntry;
    _snprintf(szErrorMsg, 500, "%s", sendJobs[iEntry].szErrorMsg);
#if defined(_UNICODE)
    {
        wchar_t wszErrorMsg[512];
        MultiByteToWideChar(myGlobals.m_LangPackCP, 0, szErrorMsg, -1, wszErrorMsg, 512);
        wszErrorMsg[511] = 0;
        LogErrorMessage(hwndDlg, dat, iEntry, wszErrorMsg);
    }
#else
    LogErrorMessage(hwndDlg, dat, iEntry, szErrorMsg);
#endif
    RecallFailedMessage(hwndDlg, dat, iEntry);
    ShowErrorControls(hwndDlg, dat, TRUE);
    HandleIconFeedback(hwndDlg, dat, myGlobals.g_iconErr);
}
/*
 * add a message to the sending queue.
 * iLen = required size of the memory block to hold the message
 */
int AddToSendQueue(HWND hwndDlg, struct MessageWindowData *dat, int iLen, int dwFlags)
{
    int iLength = 0, i;
    int iFound = NR_SENDJOBS;
    
    if(myGlobals.iSendJobCurrent >= NR_SENDJOBS) {
        _DebugMessage(hwndDlg, dat, "Send queue full");
        return 0;
    }
    /*
     * find a free entry in the send queue...
     */
    for(i = 0; i < NR_SENDJOBS; i++) {
        if(sendJobs[i].hOwner != 0 || sendJobs[i].sendCount != 0 || sendJobs[i].iStatus != 0) {
            // this entry is used, check if it's orphaned and can be removed...
            if(sendJobs[i].hwndOwner && IsWindow(sendJobs[i].hwndOwner))            // window exists, do not reuse it
                continue;
            if(time(NULL) - sendJobs[i].dwTime < 120) {                              // non-acked entry, but not old enough, don't re-use it
                continue;
            }
            //_DebugPopup(sendJobs[i].hOwner, "Removing orphaned job from queue (%d seconds old)", time(NULL) - sendJobs[i].dwTime);
            ClearSendJob(i);
            iFound = i;
            goto entry_found;
        }
        iFound = i;
        break;
    }
entry_found:
    if(iFound == NR_SENDJOBS) {
        _DebugMessage(hwndDlg, dat, "Send queue full");
        return 0;
    }
    iLength = iLen;
    if(iLength > 0) {
        if(sendJobs[iFound].sendBuffer == NULL) {
            if(iLength < HISTORY_INITIAL_ALLOCSIZE)
                iLength = HISTORY_INITIAL_ALLOCSIZE;
            sendJobs[iFound].sendBuffer = (char *)mir_alloc(iLength);
            sendJobs[iFound].dwLen = iLength;
        }
        else {
            if(iLength > sendJobs[iFound].dwLen) {
                sendJobs[iFound].sendBuffer = (char *)mir_realloc(sendJobs[iFound].sendBuffer, iLength);
                sendJobs[iFound].dwLen = iLength;
            }
        }
        CopyMemory(sendJobs[iFound].sendBuffer, dat->sendBuffer, iLen);
    }
    sendJobs[iFound].dwFlags = dwFlags;
    sendJobs[iFound].dwTime = time(NULL);

    SaveInputHistory(hwndDlg, dat, 0, 0);
    SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
    EnableSendButton(hwndDlg, FALSE);
    SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));

    UpdateSaveAndSendButton(hwndDlg, dat);
    SendQueuedMessage(hwndDlg, dat, iFound);
    return 0;
}

#define SPLIT_WORD_CUTOFF 20

#if defined(_UNICODE)

static int SendChunkW(WCHAR *chunk, HANDLE hContact, char *szSvc, DWORD dwFlags)
{
    BYTE *pBuf = NULL;
    int  wLen = lstrlenW(chunk), id;
    DWORD memRequired = (wLen + 1) * sizeof(WCHAR);
    DWORD codePage = DBGetContactSettingDword(hContact, SRMSGMOD_T, "ANSIcodepage", CP_ACP);
    int mbcsSize = WideCharToMultiByte(codePage, 0, chunk, -1, pBuf, 0, 0, 0);

    memRequired += mbcsSize;
    pBuf = (BYTE *)mir_alloc(memRequired);
    WideCharToMultiByte(codePage, 0, chunk, -1, pBuf, mbcsSize, 0, 0);
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

static DWORD WINAPI DoSplitSendW(LPVOID param)
{
    struct  SendJob *job = &sendJobs[(int)param];
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
    wszBegin = (WCHAR *)&job->sendBuffer[iLen + 1];
    wszTemp = (WCHAR *)malloc(sizeof(WCHAR) * (lstrlenW(wszBegin) + 1));
    CopyMemory(wszTemp, wszBegin, sizeof(WCHAR) * (lstrlenW(wszBegin) + 1));
    wszBegin = wszTemp;

    do {
        iCur += chunkSize;
        if(iCur > iLen)
            fSplitting = FALSE;

        /*
         * try to "word wrap" the chunks - split on word boundaries (space characters), if possible.
         * SPLIT_WORD_CUTOFF = max length of unbreakable words, longer words may be split.
        */

        if(fSplitting) {
            i = 0;
            wszSaved = &wszBegin[iCur];
            iSavedCur = iCur;
            while(iCur) {
                if(wszBegin[iCur] == (TCHAR)' ') {
                    wszSaved = &wszBegin[iCur];
                    break;
                }
                if(i == SPLIT_WORD_CUTOFF) {            // no space found backwards, restore old split position
                    iCur = iSavedCur;
                    wszSaved = &wszBegin[iCur];
                    break;
                }
                i++; iCur--;
            }
            savedChar = *wszSaved;
            *wszSaved = 0;
            id = SendChunkW(wszTemp, hContact, svcName, dwFlags);
            if(!fFirstSend) {
                job->hSendId[0] = (HANDLE)id;
                fFirstSend = TRUE;
                PostMessage(myGlobals.g_hwndHotkeyHandler, DM_SPLITSENDACK, (WPARAM)param, 0);
            }
            *wszSaved = savedChar;
            wszTemp = wszSaved;
            if(savedChar == (TCHAR)' ') {
                wszTemp++;
                iCur++;
            }
        }
        else {
            id = SendChunkW(wszTemp, hContact, svcName, dwFlags);
            if(!fFirstSend) {
                job->hSendId[0] = (HANDLE)id;
                fFirstSend = TRUE;
                PostMessage(myGlobals.g_hwndHotkeyHandler, DM_SPLITSENDACK, (WPARAM)param, 0);
            }
        }
        Sleep(500L);
    } while(fSplitting);
    free(wszBegin);
    return 0;
}

#endif

static DWORD WINAPI DoSplitSendA(LPVOID param)
{
    struct  SendJob *job = &sendJobs[(int)param];
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
        if(iCur > iLen)
            fSplitting = FALSE;

        if(fSplitting) {
            i = 0;
            szSaved = &szBegin[iCur];
            iSavedCur = iCur;
            while(iCur) {
                if(szBegin[iCur] == ' ') {
                    szSaved = &szBegin[iCur];
                    break;
                }
                if(i == SPLIT_WORD_CUTOFF) {
                    iCur = iSavedCur;
                    szSaved = &szBegin[iCur];
                    break;
                }
                i++; iCur--;
            }
            savedChar = *szSaved;
            *szSaved = 0;
            id = SendChunkA(szTemp, hContact, PSS_MESSAGE, dwFlags);
            if(!fFirstSend) {
                job->hSendId[0] = (HANDLE)id;
                fFirstSend = TRUE;
                PostMessage(myGlobals.g_hwndHotkeyHandler, DM_SPLITSENDACK, (WPARAM)param, 0);
            }
            *szSaved = savedChar;
            szTemp = szSaved;
            if(savedChar == ' ') {
                szTemp++;
                iCur++;
            }
        }
        else {
            id = SendChunkA(szTemp, hContact, PSS_MESSAGE, dwFlags);
            if(!fFirstSend) {
                job->hSendId[0] = (HANDLE)id;
                fFirstSend = TRUE;
                PostMessage(myGlobals.g_hwndHotkeyHandler, DM_SPLITSENDACK, (WPARAM)param, 0);
            }
        }
        Sleep(500L);
    } while(fSplitting);
    mir_free(szBegin);
    return 0;
}

static int SendQueuedMessage(HWND hwndDlg, struct MessageWindowData *dat, int iEntry)
{
    DWORD dwThreadId;
    
    if (dat->sendMode & SMODE_MULTIPLE) {            // implement multiple later...
        HANDLE hContact, hItem;
        sendJobs[iEntry].sendCount = 0;
        hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
        do {
            hItem = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM) hContact, 0);
            if (hItem && SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM) hItem, 0)) {
                sendJobs[iEntry].sendCount++;
                sendJobs[iEntry].hContact[sendJobs[iEntry].sendCount - 1] = hContact;
                sendJobs[iEntry].hSendId[sendJobs[iEntry].sendCount - 1] = 0;
            }
        } while (hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0));
        if (sendJobs[iEntry].sendCount == 0) {
            LogErrorMessage(hwndDlg, dat, -1, TranslateT("You haven't selected any contacts from the list. Click the checkbox box next to a name to send the message to that person."));
            return 0;
        }
        
        if(sendJobs[iEntry].sendCount > 1)              // disable queuing more messages in multisend mode...
            EnableSending(hwndDlg, dat, FALSE);
        sendJobs[iEntry].hOwner = dat->hContact;
        sendJobs[iEntry].iStatus = SQ_INPROGRESS;
        sendJobs[iEntry].hwndOwner = hwndDlg;
        sendJobs[iEntry].iAcksNeeded = sendJobs[iEntry].sendCount;
        dat->hMultiSendThread = CreateThread(NULL, 0, DoMultiSend, (LPVOID)iEntry, 0, &dwThreadId);
    } else {
        if (dat->hContact == NULL)
            return 0;  //never happens
        
		if(dat->sendMode & SMODE_FORCEANSI && DBGetContactSettingByte(dat->bIsMeta ? dat->hSubContact : dat->hContact, dat->bIsMeta ? dat->szMetaProto : dat->szProto, "UnicodeSend", 1))
			DBWriteContactSettingByte(dat->bIsMeta ? dat->hSubContact : dat->hContact, dat->bIsMeta ? dat->szMetaProto : dat->szProto, "UnicodeSend", 0);
		else if(!(dat->sendMode & SMODE_FORCEANSI) && !DBGetContactSettingByte(dat->bIsMeta ? dat->hSubContact : dat->hContact, dat->bIsMeta ? dat->szMetaProto : dat->szProto, "UnicodeSend", 0))
			DBWriteContactSettingByte(dat->bIsMeta ? dat->hSubContact : dat->hContact, dat->bIsMeta ? dat->szMetaProto : dat->szProto, "UnicodeSend", 1);

        if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "autosplit", 0)) {
            BOOL    fSplit = FALSE;
            DWORD   dwOldFlags;

            GetMaxMessageLength(hwndDlg, dat);                      // refresh length info
            /*
             + determine send buffer length
            */
#if defined(_UNICODE)
            if(sendJobs[iEntry].dwFlags & PREF_UNICODE && !(dat->sendMode & SMODE_FORCEANSI)) {
	            int     iLen;
		        WCHAR   *wszBuf;
			    char    *utf8;
                iLen = lstrlenA(sendJobs[iEntry].sendBuffer);
                wszBuf = (WCHAR *)&sendJobs[iEntry].sendBuffer[iLen + 1];
                utf8 = Utf8_Encode(wszBuf);
                if(lstrlenA(utf8) >= dat->nMax)
                    fSplit = TRUE;
                free(utf8);
            }
            else {
                if(lstrlenA(sendJobs[iEntry].sendBuffer) >= dat->nMax)
                    fSplit = TRUE;
            }
#else
            if(lstrlenA(sendJobs[iEntry].sendBuffer) >= dat->nMax)
                fSplit = TRUE;
#endif

            if(!fSplit)
                goto send_unsplitted;

            sendJobs[iEntry].sendCount = 1;
            sendJobs[iEntry].hContact[0] = dat->hContact;
            sendJobs[iEntry].hOwner = dat->hContact;
            sendJobs[iEntry].hwndOwner = hwndDlg;
            sendJobs[iEntry].iStatus = SQ_INPROGRESS;
            sendJobs[iEntry].iAcksNeeded = 1;
            sendJobs[iEntry].chunkSize = dat->nMax;

            dwOldFlags = sendJobs[iEntry].dwFlags;
            if(dat->sendMode & SMODE_FORCEANSI)
                sendJobs[iEntry].dwFlags &= ~PREF_UNICODE;

#if defined(_UNICODE)
            if(!(sendJobs[iEntry].dwFlags & PREF_UNICODE) || dat->sendMode & SMODE_FORCEANSI)
                CloseHandle(CreateThread(NULL, 0, DoSplitSendA, (LPVOID)iEntry, 0, &dwThreadId));
            else
                CloseHandle(CreateThread(NULL, 0, DoSplitSendW, (LPVOID)iEntry, 0, &dwThreadId));
#else
            CloseHandle(CreateThread(NULL, 0, DoSplitSendA, (LPVOID)iEntry, 0, &dwThreadId));
#endif
            sendJobs[iEntry].dwFlags = dwOldFlags;
        }
        else {

send_unsplitted:

            sendJobs[iEntry].sendCount = 1;
            sendJobs[iEntry].hContact[0] = dat->hContact;
            sendJobs[iEntry].hSendId[0] = (HANDLE) CallContactService(dat->hContact, MsgServiceName(dat->hContact, dat, sendJobs[iEntry].dwFlags), (dat->sendMode & SMODE_FORCEANSI) ? (sendJobs[iEntry].dwFlags & ~PREF_UNICODE) : sendJobs[iEntry].dwFlags, (LPARAM) sendJobs[iEntry].sendBuffer);
            sendJobs[iEntry].hOwner = dat->hContact;
            sendJobs[iEntry].hwndOwner = hwndDlg;
            sendJobs[iEntry].iStatus = SQ_INPROGRESS;
            sendJobs[iEntry].iAcksNeeded = 1;

            if(dat->sendMode & SMODE_NOACK) {               // fake the ack if we are not interested in receiving real acks
                ACKDATA ack = {0};
                ack.hContact = dat->hContact;
                ack.hProcess = sendJobs[iEntry].hSendId[0];
                ack.type = ACKTYPE_MESSAGE;
                ack.result = ACKRESULT_SUCCESS;
                SendMessage(hwndDlg, HM_EVENTSENT, (WPARAM)MAKELONG(iEntry, 0), (LPARAM)&ack);
            }
            else
                SetTimer(hwndDlg, TIMERID_MSGSEND + iEntry, myGlobals.m_MsgTimeout, NULL);
        }
    }
    dat->iOpenJobs++;
    myGlobals.iSendJobCurrent++;

    // give icon feedback...

    if(dat->pContainer->hwndActive == hwndDlg)
        UpdateReadChars(hwndDlg, dat);

    if(!(dat->sendMode & SMODE_NOACK))
		HandleIconFeedback(hwndDlg, dat, myGlobals.g_IconSend);
    
    if (DBGetContactSettingByte(NULL, SRMSGMOD_T, SRMSGSET_AUTOMIN, SRMSGDEFSET_AUTOMIN))
        SendMessage(dat->pContainer->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
    return 0;
}

void ClearSendJob(int iIndex) 
{
    sendJobs[iIndex].hOwner = 0;
    sendJobs[iIndex].hwndOwner = 0;
    sendJobs[iIndex].sendCount = 0;
    sendJobs[iIndex].iStatus = 0;
    sendJobs[iIndex].iAcksNeeded = 0;
    sendJobs[iIndex].dwFlags = 0;
    sendJobs[iIndex].chunkSize = 0;
    sendJobs[iIndex].dwTime = 0;
    ZeroMemory(sendJobs[iIndex].hContact, sizeof(HANDLE) * SENDJOBS_MAX_SENDS);
    ZeroMemory(sendJobs[iIndex].hSendId, sizeof(HANDLE) * SENDJOBS_MAX_SENDS);
}

/*
 * this is called when:
 *
 * ) a delivery has completed successfully
 * ) user decided to cancel a failed send
 * it removes the completed / canceled send job from the queue and schedules the next job to send (if any)
 */

void CheckSendQueue(HWND hwndDlg, struct MessageWindowData *dat)
{
    if(dat->iOpenJobs == 0) {
        HandleIconFeedback(hwndDlg, dat, (HICON)-1);
    }
    else if(!(dat->sendMode & SMODE_NOACK))
        HandleIconFeedback(hwndDlg, dat, myGlobals.g_IconSend);
    
    if(dat->pContainer->hwndActive == hwndDlg)    
        UpdateReadChars(hwndDlg, dat);
}

/*
 * logs an error message to the message window. Optionally, appends the original message
 * from the given sendJob (queue index)
 */

void LogErrorMessage(HWND hwndDlg, struct MessageWindowData *dat, int iSendJobIndex, TCHAR *szErrMsg)
{
    DBEVENTINFO dbei = {0};
    int iMsgLen;
    dbei.eventType = EVENTTYPE_ERRMSG;
    dbei.cbSize = sizeof(dbei);
    if(iSendJobIndex >= 0) {
        dbei.pBlob = (BYTE *)sendJobs[iSendJobIndex].sendBuffer;
        iMsgLen = lstrlenA(sendJobs[iSendJobIndex].sendBuffer) + 1;
    }
    else {
        iMsgLen = 0;
        dbei.pBlob = NULL;
    }
	 if ( sendJobs[iSendJobIndex].dwFlags & PREF_UTF )
		 dbei.flags = DBEF_UTF;
#if defined(_UNICODE)
    if(iSendJobIndex >= 0) {
        if(sendJobs[iSendJobIndex].dwFlags & PREF_UNICODE) {
            iMsgLen *= 3;
        }
    }
#endif
    dbei.cbBlob = iMsgLen;
    dbei.timestamp = time(NULL);
    dbei.szModule = (char *)szErrMsg;
    StreamInEvents(hwndDlg, NULL, 1, 1, &dbei);
}

/*
 * enable or disable the sending controls in the given window
 * ) input area
 * ) multisend contact list instance
 * ) send button
 */

void EnableSending(HWND hwndDlg, struct MessageWindowData *dat, int iMode)
{
    SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETREADONLY, (WPARAM) iMode ? FALSE : TRUE, 0);
    EnableWindow(GetDlgItem(hwndDlg, IDC_CLIST), iMode ? TRUE : FALSE);
    EnableSendButton(hwndDlg, iMode);
}

/*
 * show or hide the error control button bar on top of the window
 */

void ShowErrorControls(HWND hwndDlg, struct MessageWindowData *dat, int showCmd)
{
    UINT myerrorControls[] = { IDC_STATICERRORICON, IDC_STATICTEXT, IDC_RETRY, IDC_CANCELSEND, IDC_MSGSENDLATER};
    int i;

	EnableWindow(GetDlgItem(hwndDlg, IDC_MSGSENDLATER), ServiceExists(BUDDYPOUNCE_SERVICENAME) ? TRUE : FALSE);

    if(showCmd) {
        TCITEM item = {0};
        dat->hTabIcon = myGlobals.g_iconErr;
        item.mask = TCIF_IMAGE;
        item.iImage = 0;
        TabCtrl_SetItem(GetDlgItem(dat->pContainer->hwnd, IDC_MSGTABS), dat->iTabID, &item);
        dat->dwFlags |= MWF_ERRORSTATE;
    }
    else {
        dat->dwFlags &= ~MWF_ERRORSTATE;
        dat->hTabIcon = dat->hTabStatusIcon;
    }
    if(dat->dwFlagsEx & MWF_SHOW_INFOPANEL) {
        if(showCmd)
            ShowMultipleControls(hwndDlg, infoPanelControls, 8, SW_HIDE);
        else
            SendMessage(hwndDlg, DM_SETINFOPANEL, 0, 0);
    }
        
    for(i = 0; i < 5; i++) {
        if(IsWindow(GetDlgItem(hwndDlg, myerrorControls[i])))
           ShowWindow(GetDlgItem(hwndDlg, myerrorControls[i]), showCmd ? SW_SHOW : SW_HIDE);
        else
            _DebugPopup(0, "%d is not a window", myerrorControls[i]);
    }

    SendMessage(hwndDlg, WM_SIZE, 0, 0);
    DM_ScrollToBottom(hwndDlg, dat, 0, 1);
    //EnableWindow(GetDlgItem(hwndDlg, IDC_INFOPANELMENU), showCmd ? FALSE : TRUE);
    if(sendJobs[0].sendCount > 1)
        EnableSending(hwndDlg, dat, TRUE);
}

void RecallFailedMessage(HWND hwndDlg, struct MessageWindowData *dat, int iEntry)
{
    int iLen = GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_MESSAGE));
    NotifyDeliveryFailure(hwndDlg, dat);
    if(iLen == 0) {                     // message area is empty, so we can recall the failed message...
#if defined(_UNICODE)
        SETTEXTEX stx = {ST_DEFAULT,1200};
        if(sendJobs[iEntry].dwFlags & PREF_UNICODE)
            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)&sendJobs[iEntry].sendBuffer[lstrlenA(sendJobs[iEntry].sendBuffer) + 1]);
        else {
            stx.codepage = (sendJobs[iEntry].dwFlags & PREF_UTF) ? CP_UTF8 : CP_ACP;
            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)sendJobs[iEntry].sendBuffer);
        }
#else
        SetDlgItemTextA(hwndDlg, IDC_MESSAGE, (char *)sendJobs[iEntry].sendBuffer);
#endif
        UpdateSaveAndSendButton(hwndDlg, dat);
        SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
    }
}

void UpdateSaveAndSendButton(HWND hwndDlg, struct MessageWindowData *dat)
{
    int len;
#if defined(_UNICODE)
    GETTEXTLENGTHEX gtxl = {0};
    gtxl.codepage = CP_UTF8;
    gtxl.flags = GTL_DEFAULT | GTL_PRECISE | GTL_NUMBYTES;

    len = SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETTEXTLENGTHEX, (WPARAM)&gtxl, 0);
#else
    len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE));
#endif
    if(len && GetSendButtonState(hwndDlg) == PBS_DISABLED)
        EnableSendButton(hwndDlg, TRUE);
    else if(len == 0 && GetSendButtonState(hwndDlg) != PBS_DISABLED)
        EnableSendButton(hwndDlg, FALSE);

    if (len) {          // looks complex but avoids flickering on the button while typing.
        if (!(dat->dwFlags & MWF_SAVEBTN_SAV)) {
            SendDlgItemMessage(hwndDlg, IDC_SAVE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[7]);
            SendDlgItemMessage(hwndDlg, IDC_SAVE, BUTTONADDTOOLTIP, (WPARAM) pszIDCSAVE_save, 0);
            dat->dwFlags |= MWF_SAVEBTN_SAV;
        }
    } else {
        SendDlgItemMessage(hwndDlg, IDC_SAVE, BM_SETIMAGE, IMAGE_ICON, (LPARAM) myGlobals.g_buttonBarIcons[6]);
        SendDlgItemMessage(hwndDlg, IDC_SAVE, BUTTONADDTOOLTIP, (WPARAM) pszIDCSAVE_close, 0);
        dat->dwFlags &= ~MWF_SAVEBTN_SAV;
    }
    dat->textLen = len;
    if(myGlobals.m_visualMessageSizeIndicator)
        InvalidateRect(GetDlgItem(hwndDlg, IDC_MSGINDICATOR), NULL, FALSE);
}

void NotifyDeliveryFailure(HWND hwndDlg, struct MessageWindowData *dat)
{
#if defined _UNICODE
    POPUPDATAW ppd;
    int     ibsize = 1023;

    if(CallService(MS_POPUP_QUERY, PUQS_GETSTATUS, 0) == 1) {
        ZeroMemory((void *)&ppd, sizeof(ppd));
        ppd.lchContact = dat->hContact;
        mir_sntprintf(ppd.lpwzContactName, MAX_CONTACTNAME, _T("%s"), dat->szNickname);
        mir_sntprintf(ppd.lpwzText, MAX_SECONDLINE, _T("%s"), TranslateT("A message delivery has failed.\nClick to open the message window."));
        ppd.colorText = RGB(255, 245, 225);
        ppd.colorBack = RGB(191,0,0);
        ppd.PluginData = hwndDlg;
        ppd.PluginWindowProc = (WNDPROC)PopupDlgProc;
        ppd.lchIcon = myGlobals.g_iconErr;
        CallService(MS_POPUP_ADDPOPUPW, (WPARAM)&ppd, 0);
    }
#else
    POPUPDATA ppd;
    int     ibsize = 1023;

    if(CallService(MS_POPUP_QUERY, PUQS_GETSTATUS, 0) == 1) {
        ZeroMemory((void *)&ppd, sizeof(ppd));
        ppd.lchContact = dat->hContact;
        mir_sntprintf(ppd.lpzContactName, MAX_CONTACTNAME, "%s", dat->szNickname);
        mir_sntprintf(ppd.lpzText, MAX_SECONDLINE, "%s", Translate("A message delivery has failed.\nClick to open the message window."));
        ppd.colorText = RGB(255, 245, 225);
        ppd.colorBack = RGB(191,0,0);
        ppd.PluginData = hwndDlg;
        ppd.PluginWindowProc = (WNDPROC)PopupDlgProc;
        ppd.lchIcon = myGlobals.g_iconErr;
        CallService(MS_POPUP_ADDPOPUP, (WPARAM)&ppd, 0);
    }
#endif
}

static int CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message) {
        case WM_COMMAND:
            if (HIWORD(wParam) == STN_CLICKED) {
                HWND hwnd;
                struct MessageWindowData *dat;

                
                hwnd = (HWND)CallService(MS_POPUP_GETPLUGINDATA, (WPARAM)hWnd,(LPARAM)&hwnd);
                dat = (struct MessageWindowData *)GetWindowLong(hwnd, GWL_USERDATA);
                if(dat) {
                    ActivateExistingTab(dat->pContainer, hwnd);
                }
                return TRUE;
            }
            break;
        case WM_CONTEXTMENU:
        {
			PUDeletePopUp(hWnd);
			return TRUE;
        }
        default:
            break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

/*                                                              
 * searches string for characters typical for RTL text (hebrew and other RTL languages                                                                
*/

#if defined(_UNICODE)
int RTL_Detect(WCHAR *pszwText)
{
    WORD *infoTypeC2;
    int i, n = 0;
    int iLen = lstrlenW(pszwText);
    
    infoTypeC2 = (WORD *)malloc(sizeof(WORD) * (iLen + 2));

    if(infoTypeC2) {
        ZeroMemory(infoTypeC2, sizeof(WORD) * (iLen + 2));

        GetStringTypeW(CT_CTYPE2, pszwText, iLen, infoTypeC2);

        for(i = 0; i < iLen; i++) {
            if(infoTypeC2[i] == C2_RIGHTTOLEFT)
                n++;
        }
        free(infoTypeC2);
        return(n >= 2 ? 1 : 0);
        //_DebugTraceA("NO RTL text detected");
    }
    return 0;
}
#endif

int AckMessage(HWND hwndDlg, struct MessageWindowData *dat, WPARAM wParam, LPARAM lParam)
{
    ACKDATA *ack = (ACKDATA *) lParam;
    DBEVENTINFO dbei = { 0};
    HANDLE hNewEvent;
    int i, iFound = NR_SENDJOBS, iNextFailed;
    struct ContainerWindowData *m_pContainer = 0;

    if(dat)
        m_pContainer = dat->pContainer;

    iFound = (int)(LOWORD(wParam));
    i = (int)(HIWORD(wParam));

    /*      -- not longer needed, none of this happened once in more than 2 years --

    if(iFound < 0 || iFound >= NR_SENDJOBS || i < 0 || i >= SENDJOBS_MAX_SENDS) {       // sanity checks (unlikely to happen).
        _DebugPopup(dat->hContact, "Warning: HM_EVENTSENT with invalid data (sq-index = %d, sendId-index = %d", iFound, i);
        break;
    }
    */
    /*
    if (ack->type != ACKTYPE_MESSAGE) {
        _DebugPopup(dat->hContact, "Warning: HM_EVENTSENT received unknown/invalid ACKTYPE (%d)", ack->type);
        break;                                       // should not happen, but who knows...
    }
    */

    if(sendJobs[iFound].iStatus == SQ_ERROR) {       // received ack for a job which is already in error state...
        if(hwndDlg && dat) {                         // window still open
            if(dat->iCurrentQueueError == iFound) {
                dat->iCurrentQueueError = -1;
                ShowErrorControls(hwndDlg, dat, FALSE);
            }
        }
    }

    // failed acks are only handled when the window is still open. with no window open, they will be *silently* discarded

    if(ack->result == ACKRESULT_FAILED) {
        if(hwndDlg && dat) {
            /*
             * "hard" errors are handled differently in multisend. There is no option to retry - once failed, they
             * are discarded and the user is notified with a small log message.
             */
            if(!nen_options.iNoSounds && !(m_pContainer->dwFlags & CNT_NOSOUND))
                SkinPlaySound("SendError");
            if(sendJobs[iFound].sendCount > 1) {         // multisend is different...
                char szErrMsg[256];
                mir_snprintf(szErrMsg, sizeof(szErrMsg), Translate("Multisend: failed sending to: %s"), dat->szProto);
    #if defined(_UNICODE)
                {
                    wchar_t wszErrMsg[256];
                    MultiByteToWideChar(myGlobals.m_LangPackCP, 0, szErrMsg, -1, wszErrMsg, 256);
                    wszErrMsg[255] = 0;
                    LogErrorMessage(hwndDlg, dat, -1, wszErrMsg);
                }
    #else
                LogErrorMessage(hwndDlg, dat, -1, szErrMsg);
    #endif
                goto verify;
            }
            else {
                mir_snprintf(sendJobs[iFound].szErrorMsg, sizeof(sendJobs[iFound].szErrorMsg), Translate("Delivery failure: %s"), (char *)ack->lParam);
                sendJobs[iFound].iStatus = SQ_ERROR;
                KillTimer(hwndDlg, TIMERID_MSGSEND + iFound);
                if(!(dat->dwFlags & MWF_ERRORSTATE))
                    HandleQueueError(hwndDlg, dat, iFound);
            }
            return 0;
        }
        else {
            ClearSendJob(iFound);
            return 0;
        }
    }

    dbei.cbSize = sizeof(dbei);
    dbei.eventType = EVENTTYPE_MESSAGE;
    dbei.flags = DBEF_SENT;
    dbei.szModule = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) sendJobs[iFound].hContact[i], 0);
    dbei.timestamp = time(NULL);
    dbei.cbBlob = lstrlenA(sendJobs[iFound].sendBuffer) + 1;

    if(dat) {
        dat->stats.iSentBytes += (dbei.cbBlob - 1);
        dat->stats.iSent++;
    }

#if defined( _UNICODE )
    if(sendJobs[iFound].dwFlags & PREF_UNICODE)
        dbei.cbBlob *= sizeof(TCHAR) + 1;
    if(sendJobs[iFound].dwFlags & PREF_RTL)
        dbei.flags |= DBEF_RTL;
    if(sendJobs[iFound].dwFlags & PREF_UTF)
        dbei.flags |= DBEF_UTF;
#endif
    dbei.pBlob = (PBYTE) sendJobs[iFound].sendBuffer;
    hNewEvent = (HANDLE) CallService(MS_DB_EVENT_ADD, (WPARAM) sendJobs[iFound].hContact[i], (LPARAM) & dbei);

    if(m_pContainer) {
        if(!nen_options.iNoSounds && !(m_pContainer->dwFlags & CNT_NOSOUND))
            SkinPlaySound("SendMsg");
    }

    /*
     * if this is a multisend job, AND the ack was from a different contact (not the session "owner" hContact)
     * then we print a small message telling the user that the message has been delivered to *foo*
     */

    if(hwndDlg && dat) {
        if(sendJobs[iFound].hContact[i] != sendJobs[iFound].hOwner) {
            TCHAR szErrMsg[256];
            TCHAR *szReceiver = (TCHAR *)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)sendJobs[iFound].hContact[i], GCDNF_TCHAR);
            mir_sntprintf(szErrMsg, safe_sizeof(szErrMsg), TranslateT("Multisend: successfully sent to: %s"), szReceiver);
            LogErrorMessage(hwndDlg, dat, -1, szErrMsg);
        }
    }

    if (dat && (sendJobs[iFound].hContact[i] == dat->hContact)) {
        if (dat->hDbEventFirst == NULL) {
            dat->hDbEventFirst = hNewEvent;
            SendMessage(hwndDlg, DM_REMAKELOG, 0, 0);
        }
    }
verify:
    sendJobs[iFound].hSendId[i] = NULL;
    sendJobs[iFound].hContact[i] = NULL;
    sendJobs[iFound].iAcksNeeded--;

    if(sendJobs[iFound].iAcksNeeded == 0) {               // everything sent
        if(sendJobs[iFound].sendCount > 1 && hwndDlg && dat)
            EnableSending(hwndDlg, dat, TRUE);
        ClearSendJob(iFound);
        if(hwndDlg && dat) {
            KillTimer(hwndDlg, TIMERID_MSGSEND + iFound);
            dat->iOpenJobs--;
        }
        myGlobals.iSendJobCurrent--;
    }
    if(hwndDlg && dat) {
        CheckSendQueue(hwndDlg, dat);
        if((iNextFailed = FindNextFailedMsg(hwndDlg, dat)) >= 0 && !(dat->dwFlags & MWF_ERRORSTATE))
            HandleQueueError(hwndDlg, dat, iNextFailed);
    }

    return 0;
}

LRESULT WarnPendingJobs(unsigned int uNrMessages)
{
    return(MessageBox(0, TranslateT("There are unsent messages waiting for confirmation.\nWhen you close the window now, Miranda will try to send them but may be unable to inform you about possible delivery errors.\nDo you really want to close the Window(s)?"), TranslateT("Message Window warning"), MB_YESNO | MB_ICONHAND));
}
