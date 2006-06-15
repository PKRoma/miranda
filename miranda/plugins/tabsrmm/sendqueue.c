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
extern      struct SendJob sendJobs[NR_SENDJOBS];

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

DWORD WINAPI DoMultiSend(LPVOID param)
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
    LogErrorMessage(hwndDlg, dat, iEntry, (char *)szErrorMsg);
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
        if(sendJobs[i].hOwner != 0 || sendJobs[i].sendCount != 0 || sendJobs[i].iStatus != 0)
            continue;
        iFound = i;
        break;
    }
    if(iFound == NR_SENDJOBS) {
        _DebugMessage(hwndDlg, dat, "Send queue full");
        return 0;
    }
    iLength = iLen;
    if(iLength > 0) {
        if(sendJobs[iFound].sendBuffer == NULL) {
            if(iLength < HISTORY_INITIAL_ALLOCSIZE)
                iLength = HISTORY_INITIAL_ALLOCSIZE;
            sendJobs[iFound].sendBuffer = (char *)malloc(iLength);
            sendJobs[iFound].dwLen = iLength;
        }
        else {
            if(iLength > sendJobs[iFound].dwLen) {
                sendJobs[iFound].sendBuffer = (char *)realloc(sendJobs[iFound].sendBuffer, iLength);
                sendJobs[iFound].dwLen = iLength;
            }
        }
        CopyMemory(sendJobs[iFound].sendBuffer, dat->sendBuffer, iLen);
    }
    sendJobs[iFound].dwFlags = dwFlags;
    SaveInputHistory(hwndDlg, dat, 0, 0);
    SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
    EnableSendButton(hwndDlg, FALSE);
    SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));

    UpdateSaveAndSendButton(hwndDlg, dat);
    SendQueuedMessage(hwndDlg, dat, iFound);
    return 0;
}

static int SendQueuedMessage(HWND hwndDlg, struct MessageWindowData *dat, int iEntry)
{
    DWORD dwThreadId;
    
    if (dat->sendMode & SMODE_MULTIPLE) {            // implement multiple later...
        HANDLE hContact, hItem;
        //ClearSendJob(iEntry);
        //_DebugMessage(hwndDlg, dat, "Multisend is temporarily disabled because of drastic changes to the send queue system");
        //return 0;
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
            LogErrorMessage(hwndDlg, dat, -1, Translate("You haven't selected any contacts from the list. Click the checkbox box next to a name to send the message to that person."));
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
        /*
        if(dat->hAckEvent) {
            UnhookEvent(dat->hAckEvent);
            dat->hAckEvent = NULL;
        } */
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

void LogErrorMessage(HWND hwndDlg, struct MessageWindowData *dat, int iSendJobIndex, char *szErrMsg)
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
#if defined(_UNICODE)
    if(iSendJobIndex >= 0) {
        if(sendJobs[iSendJobIndex].dwFlags & PREF_UNICODE) {
            iMsgLen *= 3;
        }
    }
#endif
    dbei.cbBlob = iMsgLen;
    dbei.timestamp = time(NULL);
    dbei.szModule = szErrMsg;
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
    if(dat->dwEventIsShown & MWF_SHOW_INFOPANEL) {
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
    SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 1);
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
            stx.codepage = CP_ACP;
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
    int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE));
    
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
}

void NotifyDeliveryFailure(HWND hwndDlg, struct MessageWindowData *dat)
{
    POPUPDATA ppd;
    int     ibsize = 1023;

    if(CallService(MS_POPUP_QUERY, PUQS_GETSTATUS, 0) == 1) {
        ZeroMemory((void *)&ppd, sizeof(ppd));
        ppd.lchContact = dat->hContact;
        ppd.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
        ppd.lpzContactName[0] = 0;
        //strncpy(ppd.lpzContactName, dat->szNickname, MAX_CONTACTNAME);
        strcpy(ppd.lpzText, Translate("A message delivery has failed.\nClick to open the message window."));
        ppd.colorText = RGB(0,0,0);
        ppd.colorBack = RGB(255,0,0);
        ppd.PluginData = hwndDlg;
        ppd.PluginWindowProc = (WNDPROC)PopupDlgProc;
        ppd.lchIcon = myGlobals.g_iconErr;
        CallService(MS_POPUP_ADDPOPUP, (WPARAM)&ppd, 0);
    }
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
        return(n >= 3 ? 1 : 0);
        //_DebugTraceA("NO RTL text detected");
    }
    return 0;
}
#endif
