/*
 * sendqueue.c
 * implements a queued send system 
 */

#include "commonheaders.h"
#pragma hdrstop

#include "../../include/m_clc.h"
#include "../../include/m_clui.h"
#include "../../include/m_userinfo.h"
#include "../../include/m_history.h"
#include "../../include/m_addcontact.h"

#include "msgs.h"
#include "m_message.h"
#include "m_popup.h"
#include "m_smileyadd.h"
#include "sendqueue.h"
#include "msgdlgutils.h"

extern MYGLOBALS myGlobals;

extern char *pszIDCSAVE_save, *pszIDCSAVE_close;
extern const UINT errorControls[5];

extern struct SendJob sendJobs[NR_SENDJOBS];

char *MsgServiceName(HANDLE hContact, struct MessageWindowData *dat)
{
#ifdef _UNICODE
    char szServiceName[100];
    char *szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
    if (szProto == NULL)
        return PSS_MESSAGE;

    if(dat->sendMode & SMODE_FORCEANSI)
        return PSS_MESSAGE;
    
    _snprintf(szServiceName, sizeof(szServiceName), "%s%sW", szProto, PSS_MESSAGE);
    if (ServiceExists(szServiceName))
        return PSS_MESSAGE "W";
#endif
    return PSS_MESSAGE;
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
        sendJobs[iIndex].hSendId[i] = (HANDLE) CallContactService(sendJobs[iIndex].hContact[i], MsgServiceName(sendJobs[iIndex].hContact[i], dat), dat->sendMode & SMODE_FORCEANSI ? 0 : SEND_FLAGS, (LPARAM) sendJobs[iIndex].sendBuffer);
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
    HandleIconFeedback(hwndDlg, dat, myGlobals.g_IconError);
}
/*
 * add a message to the sending queue.
 * iLen = length of the message in dat->sendBuffer
 */
int AddToSendQueue(HWND hwndDlg, struct MessageWindowData *dat, int iLen)
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
        if(iLength > (int)sendJobs[iFound].dwLen) {
            if(sendJobs[iFound].sendBuffer == NULL) {
                if(iLength < HISTORY_INITIAL_ALLOCSIZE)
                    iLength = HISTORY_INITIAL_ALLOCSIZE;
                sendJobs[iFound].sendBuffer = (char *)malloc((iLength + 1) * (sizeof(TCHAR) + 1));
            }
            else
                sendJobs[iFound].sendBuffer = (char *)realloc(sendJobs[iFound].sendBuffer, (iLength + 1) * (sizeof(TCHAR) + 1));
            sendJobs[iFound].dwLen = iLength;
        }
        MoveMemory(sendJobs[iFound].sendBuffer, dat->sendBuffer, iLen * (sizeof(TCHAR) + 1));
        //_DebugPopup(dat->hContact, "Added: %s (entry: %d)", dat->sendJobs[dat->iSendJobCurrent].sendBuffer, dat->iSendJobCurrent);
    }
    SaveInputHistory(hwndDlg, dat, 0, 0);
    SetDlgItemText(hwndDlg, IDC_MESSAGE, _T(""));
    EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
    SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));

    UpdateSaveAndSendButton(hwndDlg, dat);
    SendQueuedMessage(hwndDlg, dat, iFound);
    return 0;
}

int SendQueuedMessage(HWND hwndDlg, struct MessageWindowData *dat, int iEntry)
{
    DWORD dwThreadId;
    
    /*
    if(dat->hAckEvent == 0)
        dat->hAckEvent = HookEventMessage(ME_PROTO_ACK, hwndDlg, HM_EVENTSENT);
    */
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
        
        sendJobs[iEntry].sendCount = 1;
        sendJobs[iEntry].hContact[0] = dat->hContact;
        sendJobs[iEntry].hSendId[0] = (HANDLE) CallContactService(dat->hContact, MsgServiceName(dat->hContact, dat), dat->sendMode & SMODE_FORCEANSI ? 0 : SEND_FLAGS, (LPARAM) sendJobs[iEntry].sendBuffer);
        sendJobs[iEntry].hOwner = dat->hContact;
        sendJobs[iEntry].hwndOwner = hwndDlg;
        sendJobs[iEntry].iStatus = SQ_INPROGRESS;
        sendJobs[iEntry].iAcksNeeded = 1;
        SetTimer(hwndDlg, TIMERID_MSGSEND + iEntry, myGlobals.m_MsgTimeout, NULL);
    }
    dat->iOpenJobs++;
    myGlobals.iSendJobCurrent++;

    // give icon feedback...

    if(dat->pContainer->hwndActive == hwndDlg)
        UpdateReadChars(hwndDlg, dat);

    HandleIconFeedback(hwndDlg, dat, myGlobals.g_IconSend);
    
    if (DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_AUTOMIN, SRMSGDEFSET_AUTOMIN))
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
        HandleIconFeedback(hwndDlg, dat, -1);
    }
    else
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
    iMsgLen *= 3;
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
    EnableWindow(GetDlgItem(hwndDlg, IDOK), iMode);
}

/*
 * show or hide the error control button bar on top of the window
 */

void ShowErrorControls(HWND hwndDlg, struct MessageWindowData *dat, int showCmd)
{
    if(showCmd) {
        dat->dwFlags |= MWF_ERRORSTATE;
        dat->iTabImage = myGlobals.g_IconError;
    }
    else {
        dat->dwFlags &= ~MWF_ERRORSTATE;
        dat->iTabImage = GetProtoIconFromList(dat->szProto, dat->wStatus);
    }

#if defined(_STREAMTHREADING)
    SendMessage(GetDlgItem(hwndDlg, IDC_LOG), WM_SETREDRAW, TRUE, 0);
    InvalidateRect(GetDlgItem(hwndDlg, IDC_LOG), NULL, FALSE);
    SendMessage(hwndDlg, WM_SIZE, 0, 0);
#endif    
    ShowMultipleControls(hwndDlg, errorControls, sizeof(errorControls) / sizeof(errorControls[0]), showCmd ? SW_SHOW : SW_HIDE);

    SendMessage(hwndDlg, WM_SIZE, 0, 0);
    SendMessage(hwndDlg, DM_SCROLLLOGTOBOTTOM, 0, 1);
    
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
        SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETTEXTEX, (WPARAM)&stx, (LPARAM)&sendJobs[iEntry].sendBuffer[lstrlenA(sendJobs[iEntry].sendBuffer) + 1]);
#else
        SetDlgItemTextA(hwndDlg, IDC_MESSAGE, (char *)sendJobs[iEntry].sendBuffer);
#endif
        UpdateSaveAndSendButton(hwndDlg, dat);
    }
}

void UpdateSaveAndSendButton(HWND hwndDlg, struct MessageWindowData *dat)
{
    int len = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE));
    
    if(len && !IsWindowEnabled(GetDlgItem(hwndDlg, IDOK)))
        EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);
    else if(len == 0 && IsWindowEnabled(GetDlgItem(hwndDlg, IDOK)))
        EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);

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
        strncpy(ppd.lpzContactName, (char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)dat->hContact,0), MAX_CONTACTNAME);
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
        case WM_RBUTTONUP:
        {
			PUDeletePopUp(hWnd);
			return TRUE;
        }
        default:
            break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

