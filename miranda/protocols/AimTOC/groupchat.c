/*
AOL Instant Messenger Plugin for Miranda IM

Copyright © 2003-2005 Robert Rainwater

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
#include "aim.h"

#define TIMERID_FLASHWND     0
#define TIMEOUT_FLASHWND     1000

static HANDLE hMenuGroupChat, hGChatMenu, hGChatHook, hGChatPreHook;
static HANDLE hInviteList;
static TTOCRoom *hRooms;
static int hRoomsCount;
static pthread_mutex_t roomMutex, findMutex;
static HCURSOR hCurSplitNS;
static TList *inviteList = NULL;

static void aim_gchat_trayupdate();
static void aim_gchat_activity();
static void aim_gchat_traydestroy();

static void aim_gchat_createdir(char *szDir)
{
    DWORD dwAttributes;
    char *pszLastBackslash, szTestDir[MAX_PATH];

    lstrcpyn(szTestDir, szDir, sizeof(szTestDir));
    if ((dwAttributes = GetFileAttributes(szTestDir)) != 0xffffffff && dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return;
    pszLastBackslash = strrchr(szTestDir, '\\');
    if (pszLastBackslash == NULL)
        return;
    *pszLastBackslash = '\0';
    aim_gchat_createdir(szTestDir);
    CreateDirectory(szTestDir, NULL);
}

void aim_gchat_getlogdir(char *szLog, int size)
{
    DBVARIANT dbv;

    if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_GL, &dbv)) {
        mir_snprintf(szLog, size, "%s", dbv.pszVal);
        DBFreeVariant(&dbv);
    }
    else {
        char szDbPath[MAX_PATH];

        CallService(MS_DB_GETPROFILEPATH, (WPARAM) MAX_PATH, (LPARAM) szDbPath);
        lstrcat(szDbPath, "\\");
        lstrcat(szDbPath, Translate("AIM Chat Logs"));
        lstrcat(szDbPath, "\\");
        mir_snprintf(szLog, size, "%s", szDbPath);
    }
}

void aim_gchat_logchat(char *szRoom, char *szMsg)
{
    if (!DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GZ, AIM_KEY_GZ_DEF))
        return;
    {
        char szFile[MAX_PATH];
        FILE *fp;

        aim_gchat_getlogdir(szFile, MAX_PATH);
        aim_gchat_createdir(szFile);
        lstrcat(szFile, "\\");
        lstrcat(szFile, szRoom);
        lstrcat(szFile, ".log");
        if (fp = fopen(szFile, "a")) {
            fwrite(szMsg, 1, strlen(szMsg), fp);
            fclose(fp);
        }
    }
}

static void aim_gchat_warn(char *szUser, int groupid)
{
    if (aim_util_isonline()) {
        char buf[MSG_LEN * 2];

        mir_snprintf(buf, sizeof(buf), "toc_chat_evil %d %s \"norm\"", groupid, szUser);
        aim_toc_sflapsend(buf, -1, TYPE_DATA);
    }
}

static void aim_gchat_storechat(char *szRoom)
{
    DBVARIANT dbv;
    int i;
    char buf[256], nbuf[256];

    if (!strcmp(szRoom, MIRANDANAME))
        return;
    for (i = 0; i < 10; i++) {
        mir_snprintf(buf, sizeof(buf), "%s%d", AIM_GCHAT_PREFIX, i);
        if (!DBGetContactSetting(NULL, AIM_PROTO, buf, &dbv)) {
            if (!strcmp(dbv.pszVal, szRoom)) {
                DBFreeVariant(&dbv);
                return;
            }
            DBFreeVariant(&dbv);
        }
    }
    for (i = 8; i >= 0; i--) {
        mir_snprintf(buf, sizeof(buf), "%s%d", AIM_GCHAT_PREFIX, i);
        if (!DBGetContactSetting(NULL, AIM_PROTO, buf, &dbv)) {
            mir_snprintf(nbuf, sizeof(nbuf), "%s%d", AIM_GCHAT_PREFIX, i + 1);
            DBWriteContactSettingString(NULL, AIM_PROTO, nbuf, dbv.pszVal);
            DBFreeVariant(&dbv);
        }
    }
    mir_snprintf(buf, sizeof(buf), "%s0", AIM_GCHAT_PREFIX, 0);
    DBWriteContactSettingString(NULL, AIM_PROTO, buf, szRoom);
}

void aim_gchat_joinrequest(char *room, int exchange)
{
    char snd[MSG_LEN * 2];
    char szRoom[MSG_LEN];

    mir_snprintf(szRoom, sizeof(szRoom), "%s", room);
    aim_util_escape(szRoom);
    LOG(LOG_DEBUG, "Sending join request for \"%s\", exchange=%d", room, exchange);
    mir_snprintf(snd, sizeof(snd), "toc_chat_join %d \"%s\"", exchange, szRoom);
    aim_toc_sflapsend(snd, -1, TYPE_DATA);
}

static BOOL CALLBACK aim_gchat_invitedlg(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
        {
            struct aim_gchat_chatinfo *info;
            char msg[256];

            TranslateDialogDefault(hwndDlg);
            aim_util_winlistadd(hInviteList, hwndDlg);
            info = (struct aim_gchat_chatinfo *) lParam;
            aim_util_striphtml(msg, info->szMsg, sizeof(msg));
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT)));
            SetWindowText(GetDlgItem(hwndDlg, IDC_BUDDY), info->szUser);
            SetWindowText(GetDlgItem(hwndDlg, IDC_ROOM), info->szRoom);
            SetWindowText(GetDlgItem(hwndDlg, IDC_MESSAGE), msg);
            SetFocus(GetDlgItem(hwndDlg, IDOK));
            break;
        }
        case WM_CLOSE:
            EndDialog(hwndDlg, IDCANCEL);
            break;
        case WM_DESTROY:
            aim_util_winlistdel(hInviteList, hwndDlg);
            break;
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
                case IDCANCEL:
                case IDOK:
                {
                    EndDialog(hwndDlg, LOWORD(wParam));
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}

static void __cdecl aim_gchat_chatinvitethread(void *cinfo)
{
    struct aim_gchat_chatinfo *info = (struct aim_gchat_chatinfo *) cinfo;
	
    if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_GCHATINVITE), NULL, aim_gchat_invitedlg, (LPARAM) info) == IDOK) {
        char snd[MSG_LEN * 2];
        mir_snprintf(snd, sizeof(snd), "toc_chat_accept %s", info->chatid);
        aim_toc_sflapsend(snd, -1, TYPE_DATA);
    }
	inviteList = tlist_remove(inviteList, (void*)info);
	if (info->szRoom) free(info->szRoom);
	if (info->szUser) free(info->szUser);
	if (info->szMsg) free(info->szMsg);
	if (info->chatid) free(info->chatid);
	free(info);
}

static int aim_gchat_inviteservice(WPARAM wParam, LPARAM lParam) {
	CLISTEVENT *cle = (CLISTEVENT*)lParam;
	struct aim_gchat_chatinfo *info;

	if (!cle) return 0;
	info = (struct aim_gchat_chatinfo *)cle->lParam;
	if (!info) return 0;
	pthread_create(aim_gchat_chatinvitethread, (void *) info);
	return 0;
}

void aim_gchat_chatinvite(char *szRoom, char *szUser, char *chatid, char *msg)
{
    struct aim_gchat_chatinfo *info = (struct aim_gchat_chatinfo *)malloc(sizeof(struct aim_gchat_chatinfo));
    HANDLE hContact;
	char szService[256], szTip[256];
	CLISTEVENT cle;

    ZeroMemory(info, sizeof(info));
    // Check to see if we are ignoring invites
    if (DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GI, AIM_KEY_GI_DEF))
        return;

    // Only accept chat invites from contacts in your list
    hContact = aim_buddy_get(szUser, 0, 0, 0, NULL);
    if (hContact == NULL)
        return;

    lstrcpyn(info->szRoom, szRoom, sizeof(info->szRoom));
    lstrcpyn(info->szUser, szUser, sizeof(info->szUser));
    lstrcpyn(info->szMsg, msg, sizeof(info->szMsg));
    lstrcpyn(info->chatid, chatid, sizeof(info->chatid));
	inviteList = tlist_append(inviteList, (void*)info);
	
	ZeroMemory(&cle, sizeof(cle));
	cle.cbSize=sizeof(cle);
	cle.hContact=(HANDLE)hContact;
	cle.hDbEvent=(HANDLE)NULL;
	cle.lParam = (LPARAM)info;
	cle.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT));
	mir_snprintf(szService, sizeof(szService), "%s/AIM/GroupChatInviteCList", AIM_PROTO);
	cle.pszService = szService;
	mir_snprintf(szTip, sizeof(szTip), Translate("Chat invitation from %s"), (char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0));
	cle.pszTooltip = szTip;
	CallServiceSync(MS_CLIST_ADDEVENT, (WPARAM)hContact, (LPARAM)&cle);
}

static void aim_gchat_exitnotice(int dwRoom)
{
    char snd[MSG_LEN * 2];

    mir_snprintf(snd, sizeof(snd), "toc_chat_leave %d", dwRoom);
    aim_toc_sflapsend(snd, -1, TYPE_DATA);
}

static void aim_gchat_send(int dwRoom, char *msg)
{
    char snd[MSG_LEN * 2];

    aim_util_escape(msg);
    mir_snprintf(snd, sizeof(snd), "toc_chat_send %d \"%s\"", dwRoom, msg);
    aim_toc_sflapsend(snd, -1, TYPE_DATA);
}

static int aim_gchat_find(int dwRoom)
{

    int i;
    pthread_mutex_lock(&findMutex);

    for (i = 0; i < hRoomsCount; i++) {
        if (hRooms[i].dwRoom == dwRoom) {
            pthread_mutex_unlock(&findMutex);
            return i;
        }
    }
    pthread_mutex_unlock(&findMutex);
    return -1;
}

#define DM_RESETROOMLIST	(WM_USER+1)
static BOOL CALLBACK aim_gchat_joinproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
        {
            char ex[10];

            TranslateDialogDefault(hwndDlg);
            mir_snprintf(ex, sizeof(ex), "%d", AIM_GROUPCHAT_DEFEXCHANGE);
            SetDlgItemText(hwndDlg, IDC_EXCHANGE, ex);
            SendDlgItemMessage(hwndDlg, IDC_EXCHANGESPIN, UDM_SETRANGE, 0, MAKELONG(6, 4));
            SendDlgItemMessage(hwndDlg, IDC_EXCHANGESPIN, UDM_SETPOS, 0, MAKELONG(AIM_GROUPCHAT_DEFEXCHANGE, 0));
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT)));
            SendMessage(hwndDlg, DM_RESETROOMLIST, 0, 0);
            break;
        }
        case DM_RESETROOMLIST:
        {
            SendDlgItemMessage(hwndDlg, IDC_CHATNAME, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessage(hwndDlg, IDC_CHATNAME, CB_ADDSTRING, 0, (LPARAM) MIRANDANAME);
            SendDlgItemMessage(hwndDlg, IDC_CHATNAME, CB_SETCURSEL, 0, 0);
            SendDlgItemMessage(hwndDlg, IDC_CHATNAME, EM_SETSEL, 0, -1);
            {
                int i;
                DBVARIANT dbv;
                char buf[64];

                for (i = 0; i < 10; i++) {
                    mir_snprintf(buf, sizeof(buf), "%s%d", AIM_GCHAT_PREFIX, i);
                    if (!DBGetContactSetting(NULL, AIM_PROTO, buf, &dbv)) {
                        SendDlgItemMessage(hwndDlg, IDC_CHATNAME, CB_ADDSTRING, 0, (LPARAM) dbv.pszVal);
                        DBFreeVariant(&dbv);
                    }
                }
            }
            SetFocus(GetDlgItem(hwndDlg, IDC_CHATNAME));
            break;
        }
        case WM_CLOSE:
        {
            EndDialog(hwndDlg, 0);
            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
                case IDC_HISTORY:
                {
                    if (MessageBox
                        (hwndDlg, Translate("Are you sure you want to delete the recent groupchat list?"), Translate("Confirm Delete"),
                         MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        char buf[256];
                        int i;

                        for (i = 0; i < 10; i++) {
                            mir_snprintf(buf, sizeof(buf), "%s%d", AIM_GCHAT_PREFIX, i);
                            DBDeleteContactSetting(NULL, AIM_PROTO, buf);
                        }
                        SendMessage(hwndDlg, DM_RESETROOMLIST, 0, 0);
                    }
                    break;
                }
                case IDC_CHATNAME:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int i;
                        if (i = (int) SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0) != CB_ERR) {
                            EnableWindow(GetDlgItem(hwndDlg, IDOK), (int) SendMessage((HWND) lParam, CB_GETLBTEXTLEN, i, 0));
                        }
                    }
                    else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_EDITUPDATE)
                        EnableWindow(GetDlgItem(hwndDlg, IDOK), GetWindowTextLength((HWND) lParam));
                    break;
                case IDOK:
                {
                    char szRoom[128];
                    int exchange;

                    GetWindowText(GetDlgItem(hwndDlg, IDC_CHATNAME), szRoom, sizeof(szRoom));
                    exchange = GetDlgItemInt(hwndDlg, IDC_EXCHANGE, NULL, TRUE);
                    aim_gchat_joinrequest(szRoom, exchange);
                }
                    // fall-through
                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    break;
            }
        }
    }
    return FALSE;
}

static int aim_gchat_resizer(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL * urc)
{
    struct MessageWindowData *dat = (struct MessageWindowData *) lParam;
    switch (urc->wId) {
        case IDC_USERS:
        case IDC_FRAME:
            return RD_ANCHORX_RIGHT | RD_ANCHORY_TOP;
        case IDC_SPLITTER:
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            return RD_ANCHORX_WIDTH | RD_ANCHORY_BOTTOM;
        case IDC_LOG:
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            return RD_ANCHORX_WIDTH | RD_ANCHORY_HEIGHT;
        case IDC_LIST:
            urc->rcItem.bottom -= dat->splitterY - dat->originalSplitterY;
            return RD_ANCHORX_RIGHT | RD_ANCHORY_HEIGHT;
        case IDC_ENTER:
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            return RD_ANCHORX_RIGHT | RD_ANCHORY_BOTTOM;
        case IDC_MESSAGE:
            urc->rcItem.top -= dat->splitterY - dat->originalSplitterY;
            return RD_ANCHORX_WIDTH | RD_ANCHORY_BOTTOM;
    }
    return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;
}

static WNDPROC OldMessageEditProc;
static WNDPROC OldSplitterProc;

static LRESULT CALLBACK aim_gchat_msgeditsubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_GETDLGCODE:
            return DLGC_WANTALLKEYS;
        case WM_CHAR:
            if (wParam == VK_TAB) {
                return 0;
            }
            if (wParam == 1 && GetKeyState(VK_CONTROL) & 0x8000) {      //ctrl-a
                SendMessage(hwnd, EM_SETSEL, 0, -1);
                return 0;
            }
            if (wParam == '\n' || wParam == '\r') {
                if ((GetKeyState(VK_CONTROL) & 0x8000) && !DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GE, AIM_KEY_GE_DEF)) {
                    PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
                    return 0;
                }
            }
            if (wParam == 13 && DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GE, AIM_KEY_GE_DEF)) {
                PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
                return 0;
            }
            break;
    }
    return CallWindowProc(OldMessageEditProc, hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK aim_gchat_msgsplittersubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_NCHITTEST:
            return HTCLIENT;
        case WM_SETCURSOR:
        {
            SetCursor(hCurSplitNS);
            return TRUE;
        }
        case WM_LBUTTONDOWN:
            SetCapture(hwnd);
            return 0;
        case WM_MOUSEMOVE:
            if (GetCapture() == hwnd) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                SendMessage(GetParent(hwnd), GC_SPLITTERMOVE,
                            rc.right > rc.bottom ? (short) HIWORD(GetMessagePos()) + rc.bottom / 2 : (short) HIWORD(GetMessagePos()) + rc.right / 2,
                            (LPARAM) hwnd);
            }
            return 0;
        case WM_LBUTTONUP:
            ReleaseCapture();
            return 0;
    }
    return CallWindowProc(OldSplitterProc, hwnd, msg, wParam, lParam);
}

static int aim_gchat_processcommand(HWND hwnd, char *msg)
{
    if (!strcmp(msg, "/clear")) {
        SetDlgItemText(hwnd, IDC_LOG, "");
        return 1;
    }
    else if (!strcmp(msg, "/quit") || !strcmp(msg, "/exit")) {
        SendMessage(hwnd, GC_LOGOUT, 0, 0);
        return 1;
    }
    return 0;
}

static BOOL CALLBACK aim_gchat_dlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct MessageWindowData *dat;

    dat = (struct MessageWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);
    switch (msg) {
        case WM_INITDIALOG:
        {
            TranslateDialogDefault(hwndDlg);
            dat = (struct MessageWindowData *) malloc(sizeof(struct MessageWindowData));
            dat->roomid = (int) lParam;
            hRooms[dat->roomid].hwnd = hwndDlg;
            dat->hBkgBrush = NULL;
            dat->nFlash = 0;
            SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) dat);
            SendDlgItemMessage(hwndDlg, IDC_LOG, EM_AUTOURLDETECT, TRUE, 0);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_LIMITTEXT, (WPARAM)sizeof(TCHAR)*0x7FFFFFFF, 0);
            dat->splitterY = (int) DBGetContactSettingDword(NULL, AIM_PROTO, AIM_KEY_GS, (DWORD) - 1);
            {
                RECT rc;
                POINT pt;
                GetWindowRect(GetDlgItem(hwndDlg, IDC_SPLITTER), &rc);
                pt.y = (rc.top + rc.bottom) / 2;
                pt.x = 0;
                ScreenToClient(hwndDlg, &pt);
                dat->originalSplitterY = pt.y;
                if (dat->splitterY == -1)
                    dat->splitterY = dat->originalSplitterY;
            }
            EnableWindow(GetDlgItem(hwndDlg, IDC_ENTER), FALSE);
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT)));
            SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_LINK);
            SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_LIMITTEXT, 250, 0);
            OldMessageEditProc = (WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_WNDPROC, (LONG) aim_gchat_msgeditsubclass);
            OldSplitterProc = (WNDPROC) SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_WNDPROC, (LONG) aim_gchat_msgsplittersubclass);
            SendDlgItemMessage(hwndDlg, IDC_LIST, LB_RESETCONTENT, 0, 0);
            {
                RECT rc;
                GetWindowRect(GetDlgItem(hwndDlg, IDC_LOG), &rc);
                dat->minEditBoxSize.cx = rc.right - rc.left;
                dat->minEditBoxSize.cy = rc.bottom - rc.top;
            }
            if (Utils_RestoreWindowPosition(hwndDlg, NULL, AIM_PROTO, AIM_KEY_GC)) {
				SetWindowPos(hwndDlg, 0, 0, 0, 450, 300, SWP_NOZORDER|SWP_NOMOVE);
			}
            SendMessage(hwndDlg, GC_REFRESH, 0, 0);
            SendMessage(hwndDlg, GC_UPDATECOUNT, 0, 0);
            LOG(LOG_DEBUG, "Groupchat %s (%d) intialized", hRooms[dat->roomid].szRoom, hRooms[dat->roomid].dwRoom);
            SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
            {
                DBTIMETOSTRING dbtts;
                char str[64];
                char log[256];

                dbtts.szFormat = "d t";
                dbtts.cbDest = sizeof(str);
                dbtts.szDest = str;
                CallService(MS_DB_TIME_TIMESTAMPTOSTRING, (WPARAM) time(NULL), (LPARAM) & dbtts);
                mir_snprintf(log, sizeof(log), Translate("[%s] *** Chat session started"), str);
                aim_gchat_logchat(hRooms[dat->roomid].szRoom, log);
                aim_gchat_logchat(hRooms[dat->roomid].szRoom, "\n");
            }
			ShowWindow(hwndDlg, SW_SHOWNORMAL);
            return TRUE;
        }
        case GC_SPLITTERMOVE:
        {
            POINT pt;
            RECT rc;
            RECT rcLog;

            if ((HWND) lParam == GetDlgItem(hwndDlg, IDC_SPLITTER)) {
                int oldSplitterY;

                GetWindowRect(GetDlgItem(hwndDlg, IDC_LOG), &rcLog);
                GetClientRect(hwndDlg, &rc);
                pt.x = 0;
                pt.y = wParam;
                ScreenToClient(hwndDlg, &pt);
                oldSplitterY = dat->splitterY;
                dat->splitterY = rc.bottom - pt.y;
                GetWindowRect(GetDlgItem(hwndDlg, IDC_MESSAGE), &rc);
                if (rc.bottom - rc.top + (dat->splitterY - oldSplitterY) < dat->minEditBoxSize.cy)
                    dat->splitterY = oldSplitterY + dat->minEditBoxSize.cy - (rc.bottom - rc.top);
                if (rcLog.bottom - rcLog.top - (dat->splitterY - oldSplitterY) < dat->minEditBoxSize.cy)
                    dat->splitterY = oldSplitterY - dat->minEditBoxSize.cy + (rcLog.bottom - rcLog.top);
                SendMessage(hwndDlg, WM_SIZE, 0, 0);
            }
            break;
        }
        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *mmi = (MINMAXINFO *) lParam;
            RECT rcWindow, rcLog;
            GetWindowRect(hwndDlg, &rcWindow);
            GetWindowRect(GetDlgItem(hwndDlg, IDC_LOG), &rcLog);
            mmi->ptMinTrackSize.x = rcWindow.right - rcWindow.left - ((rcLog.right - rcLog.left) - dat->minEditBoxSize.cx);
            mmi->ptMinTrackSize.y = rcWindow.bottom - rcWindow.top - ((rcLog.bottom - rcLog.top) - dat->minEditBoxSize.cy);
            return 0;
        }
        case WM_SIZE:
        {
            UTILRESIZEDIALOG urd;

            aim_gchat_trayupdate();
            ZeroMemory(&urd, sizeof(urd));
            if (IsIconic(hwndDlg))
                break;
            urd.cbSize = sizeof(urd);
            urd.hwndDlg = hwndDlg;
            urd.lParam = (LPARAM) dat;
            urd.hInstance = hInstance;
            urd.lpTemplate = MAKEINTRESOURCE(IDD_GROUPCHAT);
            urd.pfnResizer = aim_gchat_resizer;
            CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM) & urd);
            break;
        }
        case WM_CLOSE:
        case GC_LOGOUT:
        {
            aim_gchat_delete(hRooms[dat->roomid].dwRoom);
        }                       // fall-through
        case GC_QUIT:
            EndDialog(hwndDlg, 0);
            break;
        case WM_DESTROY:
        {
            {
                DBTIMETOSTRING dbtts;
                char str[64];
                char log[256];

                dbtts.szFormat = "d t";
                dbtts.cbDest = sizeof(str);
                dbtts.szDest = str;
                CallService(MS_DB_TIME_TIMESTAMPTOSTRING, (WPARAM) time(NULL), (LPARAM) & dbtts);
                mir_snprintf(log, sizeof(log), Translate("[%s] *** Chat session ended"), str);
                aim_gchat_logchat(hRooms[dat->roomid].szRoom, log);
                aim_gchat_logchat(hRooms[dat->roomid].szRoom, "\n");
            }
            hRooms[dat->roomid].dwRoom = -1;
            hRooms[dat->roomid].hwnd = NULL;
            if (dat->hBkgBrush)
                DeleteObject(dat->hBkgBrush);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_MESSAGE), GWL_WNDPROC, (LONG) OldMessageEditProc);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_SPLITTER), GWL_WNDPROC, (LONG) OldSplitterProc);
            DBWriteContactSettingDword(NULL, AIM_PROTO, AIM_KEY_GS, dat->splitterY);
            Utils_SaveWindowPosition(hwndDlg, NULL, AIM_PROTO, AIM_KEY_GC);
            free(dat);
            break;
        }
        case GC_ADDUSER:
        {
            WNDPROC listWndProc;
            HWND hwndList;
            int i, found = 0;
            char buf[128];

            hwndList = GetDlgItem(hwndDlg, IDC_LIST);
            listWndProc = (WNDPROC) GetWindowLong(hwndList, GWL_WNDPROC);
            i = CallWindowProc(listWndProc, hwndList, LB_GETCOUNT, 0, 0);
            while (i > 0) {
                CallWindowProc(listWndProc, hwndList, LB_GETTEXT, i - 1, (LPARAM) buf);
                if (!strcmp(buf, (char *) lParam)) {
                    found = 1;
                    break;
                }
                i--;
            }
            if (!found) {
                LOG(LOG_DEBUG, "User %s has joined channel (%s)", (char *) lParam, hRooms[dat->roomid].szRoom);
                CallWindowProc(listWndProc, hwndList, LB_ADDSTRING, 0, lParam);
            }
            break;
        }
        case GC_REMUSER:
        {
            WNDPROC listWndProc;
            HWND hwndList;
            int i;
            char buf[128];

            hwndList = GetDlgItem(hwndDlg, IDC_LIST);
            listWndProc = (WNDPROC) GetWindowLong(hwndList, GWL_WNDPROC);
            i = CallWindowProc(listWndProc, hwndList, LB_GETCOUNT, 0, 0);
            while (i > 0) {
                CallWindowProc(listWndProc, hwndList, LB_GETTEXT, i - 1, (LPARAM) buf);
                if (!strcmp(buf, (char *) lParam)) {
                    LOG(LOG_DEBUG, "User %s has left channel (%s)", (char *) lParam, hRooms[dat->roomid].szRoom);
                    CallWindowProc(listWndProc, hwndList, LB_DELETESTRING, i - 1, 0);
                    break;
                }
                i--;
            }
            break;
        }
        case GC_UPDATEBUDDY:
        {
            if ((int) wParam) {
                SendMessage(hwndDlg, GC_ADDUSER, 0, lParam);
            }
            else {
                SendMessage(hwndDlg, GC_REMUSER, 0, lParam);
            }
            SendMessage(hwndDlg, GC_UPDATECOUNT, 0, 0);
            break;
        }
        case GC_SCROLLLOGTOBOTTOM:
        {
            SCROLLINFO si;

            ZeroMemory(&si, sizeof(si));
            if ((GetWindowLong(GetDlgItem(hwndDlg, IDC_LOG), GWL_STYLE) & WS_VSCROLL) == 0)
                break;
            si.cbSize = sizeof(si);
            si.fMask = SIF_PAGE | SIF_RANGE;
            GetScrollInfo(GetDlgItem(hwndDlg, IDC_LOG), SB_VERT, &si);
            si.fMask = SIF_POS;
            si.nPos = si.nMax - si.nPage + 1;
            SetScrollInfo(GetDlgItem(hwndDlg, IDC_LOG), SB_VERT, &si, TRUE);
            PostMessage(GetDlgItem(hwndDlg, IDC_LOG), WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, si.nPos), (LPARAM) (HWND) NULL);
            break;
        }
        case GC_FLASHWINDOW:
        {
            if ((GetActiveWindow() != hwndDlg || GetForegroundWindow() != hwndDlg)
                && DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GF, AIM_KEY_GF_DEF)) {
                SetTimer(hwndDlg, TIMERID_FLASHWND, TIMEOUT_FLASHWND, NULL);
            }
            break;
        }
        case GC_REFRESH:
        {
            if (dat->hBkgBrush)
                DeleteObject(dat->hBkgBrush);
            {
                COLORREF colour = DBGetContactSettingDword(NULL, AIM_PROTO, AIM_KEY_GB, AIM_KEY_GB_DEF);
                dat->hBkgBrush = CreateSolidBrush(colour);
                SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETBKGNDCOLOR, 0, colour);
            }
            InvalidateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, FALSE);
            {
                HFONT hFont;
                LOGFONT lf;
                hFont = (HFONT) SendDlgItemMessage(hwndDlg, IDC_MESSAGE, WM_GETFONT, 0, 0);
                if (hFont != NULL && hFont != (HFONT) SendDlgItemMessage(hwndDlg, IDOK, WM_GETFONT, 0, 0))
                    DeleteObject(hFont);
                LoadGroupChatFont(GCHATFONTID_SEND, &lf, NULL);
                hFont = CreateFontIndirect(&lf);
                SendDlgItemMessage(hwndDlg, IDC_MESSAGE, WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0));
            }
            InvalidateRect(GetDlgItem(hwndDlg, IDC_LIST), NULL, FALSE);
            {
                HFONT hFont;
                LOGFONT lf;
                hFont = (HFONT) SendDlgItemMessage(hwndDlg, IDC_LIST, WM_GETFONT, 0, 0);
                if (hFont != NULL && hFont != (HFONT) SendDlgItemMessage(hwndDlg, IDOK, WM_GETFONT, 0, 0))
                    DeleteObject(hFont);
                LoadGroupChatFont(GCHATFONTID_LIST, &lf, NULL);
                hFont = CreateFontIndirect(&lf);
                SendDlgItemMessage(hwndDlg, IDC_LIST, WM_SETFONT, (WPARAM) hFont, MAKELPARAM(TRUE, 0));
            }
            break;
        }
        case WM_CTLCOLORLISTBOX:
        {
            COLORREF colour;
            if ((HWND) lParam == GetDlgItem(hwndDlg, IDC_LIST)) {
                LoadGroupChatFont(GCHATFONTID_LIST, NULL, &colour);
                SetTextColor((HDC) wParam, colour);
                SetBkColor((HDC) wParam, DBGetContactSettingDword(NULL, AIM_PROTO, AIM_KEY_GB, AIM_KEY_GB_DEF));
                return (BOOL) dat->hBkgBrush;
            }
            break;
        }
        case WM_CTLCOLOREDIT:
        {
            COLORREF colour;
            if ((HWND) lParam == GetDlgItem(hwndDlg, IDC_MESSAGE)) {
                LoadGroupChatFont(GCHATFONTID_SEND, NULL, &colour);
                SetTextColor((HDC) wParam, colour);
                SetBkColor((HDC) wParam, DBGetContactSettingDword(NULL, AIM_PROTO, AIM_KEY_GB, AIM_KEY_GB_DEF));
                return (BOOL) dat->hBkgBrush;
            }
            break;
        }
        case WM_SETFOCUS:
            if (dat->nFlash) {
                KillTimer(hwndDlg, TIMERID_FLASHWND);
                FlashWindow(hwndDlg, FALSE);
                dat->nFlash = 0;
            }
            ShowCaret(GetDlgItem(hwndDlg, IDC_MESSAGE));
            SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
            break;
        case WM_ACTIVATE:
            if (wParam != WA_INACTIVE) {
                if (dat->nFlash) {
                    KillTimer(hwndDlg, TIMERID_FLASHWND);
                    FlashWindow(hwndDlg, FALSE);
                    dat->nFlash = 0;
                }
            }
            break;
        case WM_MOUSEACTIVATE:
            if (dat->nFlash) {
                KillTimer(hwndDlg, TIMERID_FLASHWND);
                FlashWindow(hwndDlg, FALSE);
                dat->nFlash = 0;
            }
            break;
        case GC_UPDATECOUNT:
        {
            char buf[256];
            int i = (int) SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETCOUNT, 0, 0);

            mir_snprintf(buf, sizeof(buf), "%d %s", i, i == 1 ? Translate("User") : Translate("Users"));
            SetWindowText(GetDlgItem(hwndDlg, IDC_USERS), buf);
            mir_snprintf(buf, sizeof(buf), Translate("%s - AIM Group Chat, %d user(s)"), hRooms[dat->roomid].szRoom, i);
            SetWindowText(hwndDlg, buf);
            break;
        }
        case WM_TIMER:
        {
            if (wParam == TIMERID_FLASHWND) {
                if (dat->nFlash > 20) {
                    KillTimer(hwndDlg, TIMERID_FLASHWND);
                    FlashWindow(hwndDlg, FALSE);
                    dat->nFlash = 0;
                    break;
                }
                FlashWindow(hwndDlg, TRUE);
                dat->nFlash++;
            }
            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
				case IDC_LIST:
				{
					if (HIWORD(wParam)==LBN_DBLCLK) {
						int i = SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
						if (i!=LB_ERR) {
							char szUser[128];

							SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETTEXT, i, (LPARAM) szUser);
							if (!aim_util_isme(szUser)) {
								HANDLE hContact;

								hContact = aim_buddy_get(szUser, 1, 0, 0, NULL);
								if (hContact) {
									CallServiceSync(MS_MSG_SENDMESSAGE, (WPARAM)hContact, 0);
								}
							}
						}
					}
					break;
				}
                case IDC_MESSAGE:
                {
                    if (!GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE)))
                        EnableWindow(GetDlgItem(hwndDlg, IDC_ENTER), FALSE);
                    else
                        EnableWindow(GetDlgItem(hwndDlg, IDC_ENTER), TRUE);
                    break;
                }
                case IDC_ENTER:
                case IDOK:
                {
                    char msg[MSG_LEN * 2];

                    SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                    GetDlgItemText(hwndDlg, IDC_MESSAGE, msg, sizeof(msg) + 1);
                    if (msg[0] == 0) {
                        LOG(LOG_WARN, "Attempt to send empty message to groupchat");
                        break;
                    }
                    if (aim_gchat_processcommand(hwndDlg, msg)) {
                        SetDlgItemText(hwndDlg, IDC_MESSAGE, "");
                        break;
                    }
                    if (aim_util_isonline()) {
                        SetDlgItemText(hwndDlg, IDC_MESSAGE, "");
                        EnableWindow(GetDlgItem(hwndDlg, IDC_ENTER), FALSE);
                        aim_gchat_send(hRooms[dat->roomid].dwRoom, msg);
                    }
                    else {
                        LOG(LOG_WARN, "Attempt to send message to groupchat while offline");
                    }
                    break;
                }
                case IDCANCEL:
                    SendMessage(hwndDlg, GC_LOGOUT, 0, 0);
                    break;
            }
            break;
        }
        case WM_CONTEXTMENU:
        {
            RECT lb;
            POINT pt;
            int height = (int) SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETITEMHEIGHT, 0, 0);
            int count = (int) SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETCOUNT, 0, 0);

            pt.x = (short) LOWORD(lParam);
            pt.y = (short) HIWORD(lParam);
            GetWindowRect(GetDlgItem(hwndDlg, IDC_LIST), &lb);
            if (PtInRect(&lb, pt)) {
                int i = (pt.y - lb.top) / height;
                if (i >= 0 && i < count) {
                    HMENU hMenu, hSubMenu;
                    HANDLE hContact;
                    char szUser[256];
                    int hIsMe = 0;
                    int add;

                    SendDlgItemMessage(hwndDlg, IDC_LIST, LB_SETCURSEL, i, 0);
                    SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETTEXT, i, (LPARAM) szUser);
                    hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_CONTEXT));
                    hSubMenu = GetSubMenu(hMenu, 2);
                    hContact = aim_buddy_get(szUser, 0, 0, 0, NULL);
                    add = DBGetContactSettingByte(hContact, "CList", "NotOnList", 0);
                    hIsMe = aim_util_isme(szUser);
                    EnableMenuItem(hSubMenu, IDM_WARN, MF_BYCOMMAND | MF_ENABLED);
                    if (hIsMe) {
                        EnableMenuItem(hSubMenu, IDM_WARN, MF_BYCOMMAND | MF_GRAYED);
                        EnableMenuItem(hSubMenu, IDM_ADD, MF_BYCOMMAND | MF_GRAYED);
                    }
                    else if (hContact && !add) {
                        EnableMenuItem(hSubMenu, IDM_ADD, MF_BYCOMMAND | MF_GRAYED);
                    }
                    if (!hIsMe && !DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_WM, AIM_KEY_WM_DEF)) {
                        EnableMenuItem(hSubMenu, IDM_WARN, MF_BYCOMMAND | MF_GRAYED);
                    }
                    CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hSubMenu, 0);
                    switch (TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL)) {
                        case IDM_UI:
                        {
                            if (hIsMe) {
                                CallServiceSync(MS_USERINFO_SHOWDIALOG, 0, 0);
                            }
                            else {
                                HANDLE ctc = aim_buddy_get(szUser, 1, 0, 0, NULL);
                                if (ctc != NULL) {
                                    CallServiceSync(MS_USERINFO_SHOWDIALOG, (WPARAM) ctc, 0);
                                }
                            }
                            break;
                        }
                        case IDM_ADD:
                        {
                            if (!hIsMe) {
                                PROTOSEARCHRESULT psr;
                                ADDCONTACTSTRUCT acs;

                                ZeroMemory(&psr, sizeof(psr));
                                psr.cbSize = sizeof(psr);
                                psr.nick = szUser;
                                acs.handle = NULL;
                                acs.handleType = HANDLE_SEARCHRESULT;
                                acs.szProto = AIM_PROTO;
                                acs.psr = &psr;
                                CallService(MS_ADDCONTACT_SHOW, (WPARAM) hwndDlg, (LPARAM) & acs);
                            }
                            break;
                        }
                        case IDM_WARN:
                            if (!hIsMe) {
                                aim_gchat_warn(szUser, hRooms[dat->roomid].dwRoom);
                            }
                            break;
                    }
                    DestroyMenu(hMenu);
                }
            }
            break;
        }
        case WM_NOTIFY:
        {
            switch (((NMHDR *) lParam)->idFrom) {
                case IDC_LOG:
                    switch (((NMHDR *) lParam)->code) {
                        case EN_MSGFILTER:
                            switch (((MSGFILTER *) lParam)->msg) {
                                case WM_RBUTTONUP:
                                {
                                    HMENU hMenu, hSubMenu;
                                    POINT pt;
                                    CHARRANGE sel, all = { 0, -1 };

                                    hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_CONTEXT));
                                    hSubMenu = GetSubMenu(hMenu, 0);
                                    CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hSubMenu, 0);
                                    SendMessage(((NMHDR *) lParam)->hwndFrom, EM_EXGETSEL, 0, (LPARAM) & sel);
                                    if (sel.cpMin == sel.cpMax)
                                        EnableMenuItem(hSubMenu, IDM_COPY, MF_BYCOMMAND | MF_GRAYED);
                                    pt.x = (short) LOWORD(((ENLINK *) lParam)->lParam);
                                    pt.y = (short) HIWORD(((ENLINK *) lParam)->lParam);
                                    ClientToScreen(((NMHDR *) lParam)->hwndFrom, &pt);
                                    switch (TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL)) {
                                        case IDM_COPY:
                                            SendMessage(((NMHDR *) lParam)->hwndFrom, WM_COPY, 0, 0);
                                            break;
                                        case IDM_COPYALL:
                                            SendMessage(((NMHDR *) lParam)->hwndFrom, EM_EXSETSEL, 0, (LPARAM) & all);
                                            SendMessage(((NMHDR *) lParam)->hwndFrom, WM_COPY, 0, 0);
                                            SendMessage(((NMHDR *) lParam)->hwndFrom, EM_EXSETSEL, 0, (LPARAM) & sel);
                                            break;
                                        case IDM_SELECTALL:
                                            SendMessage(((NMHDR *) lParam)->hwndFrom, EM_EXSETSEL, 0, (LPARAM) & all);
                                            break;
                                        case IDM_CLEAR:
                                            SetDlgItemText(hwndDlg, IDC_LOG, "");
                                            break;
                                    }
                                    DestroyMenu(hMenu);
                                }
                            }
                            break;
                        case EN_LINK:
                            switch (((ENLINK *) lParam)->msg) {
                                case WM_RBUTTONDOWN:
                                case WM_LBUTTONUP:
                                {
                                    TEXTRANGE tr;
                                    CHARRANGE sel;

                                    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXGETSEL, 0, (LPARAM) & sel);
                                    if (sel.cpMin != sel.cpMax)
                                        break;
                                    tr.chrg = ((ENLINK *) lParam)->chrg;
                                    tr.lpstrText = malloc(tr.chrg.cpMax - tr.chrg.cpMin + 8);
                                    SendDlgItemMessage(hwndDlg, IDC_LOG, EM_GETTEXTRANGE, 0, (LPARAM) & tr);
                                    if (strchr(tr.lpstrText, '@') != NULL && strchr(tr.lpstrText, ':') == NULL && strchr(tr.lpstrText, '/') == NULL) {
                                        MoveMemory(tr.lpstrText + 7, tr.lpstrText, tr.chrg.cpMax - tr.chrg.cpMin + 1);
                                        CopyMemory(tr.lpstrText, "mailto:", 7);
                                    }
                                    if (((ENLINK *) lParam)->msg == WM_RBUTTONDOWN) {
                                        HMENU hMenu, hSubMenu;
                                        POINT pt;

                                        hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_CONTEXT));
                                        hSubMenu = GetSubMenu(hMenu, 1);
                                        CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hSubMenu, 0);
                                        pt.x = (short) LOWORD(((ENLINK *) lParam)->lParam);
                                        pt.y = (short) HIWORD(((ENLINK *) lParam)->lParam);
                                        ClientToScreen(((NMHDR *) lParam)->hwndFrom, &pt);
                                        switch (TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL)) {
                                            case IDM_OPENNEW:
                                                CallService(MS_UTILS_OPENURL, 1, (LPARAM) tr.lpstrText);
                                                break;
                                            case IDM_OPENEXISTING:
                                                CallService(MS_UTILS_OPENURL, 0, (LPARAM) tr.lpstrText);
                                                break;
                                            case IDM_COPYLINK:
                                            {
                                                HGLOBAL hData;
                                                if (!OpenClipboard(hwndDlg))
                                                    break;
                                                EmptyClipboard();
                                                hData = GlobalAlloc(GMEM_MOVEABLE, lstrlen(tr.lpstrText) + 1);
                                                lstrcpy((char *) GlobalLock(hData), tr.lpstrText);
                                                GlobalUnlock(hData);
                                                SetClipboardData(CF_TEXT, hData);
                                                CloseClipboard();
                                                break;
                                            }
                                        }
                                        free(tr.lpstrText);
                                        DestroyMenu(hMenu);
                                        SetWindowLong(hwndDlg, DWL_MSGRESULT, TRUE);
                                        return TRUE;
                                    }
                                    else {
                                        CallService(MS_UTILS_OPENURL, 1, (LPARAM) tr.lpstrText);
                                        SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
                                    }

                                    free(tr.lpstrText);
                                    break;
                                }
                            }
                            break;
                    }
            }
            break;
        }
    }
    return FALSE;
}

static int aim_gchat_join(WPARAM wParam, LPARAM lParam)
{
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_GROUPCHATJOIN), NULL, aim_gchat_joinproc);
    return 0;
}

static void __cdecl aim_gchat_open(void *room)
{
	OleInitialize(NULL);
    DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_GROUPCHAT), NULL, aim_gchat_dlgproc, (LPARAM) room);
	OleUninitialize();
}

void aim_gchat_updatechats()
{
    int i;

    for (i = 0; i < hRoomsCount; i++) {
        if (hRooms[i].hwnd) {
            PostMessage(hRooms[i].hwnd, GC_REFRESH, 0, 0);
        }
    }
}

void aim_gchat_updatemenu()
{
    CLISTMENUITEM cli;

    ZeroMemory(&cli, sizeof(cli));
    cli.cbSize = sizeof(cli);
    if (aim_util_isonline()) {
        cli.flags = CMIM_FLAGS;
    }
    else {
        cli.flags = CMIM_FLAGS | CMIF_GRAYED;
    }
    CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hMenuGroupChat, (LPARAM) & cli);
}

void aim_gchat_create(int dwRoom, char *szRoom)
{
    pthread_mutex_lock(&roomMutex);
    if (aim_gchat_find(dwRoom) != -1) {
        pthread_mutex_unlock(&roomMutex);
        return;
    }
    hRooms = (TTOCRoom *) realloc(hRooms, sizeof(TTOCRoom) * (hRoomsCount + 1));
    strcpy(hRooms[hRoomsCount].szRoom, szRoom);
    hRooms[hRoomsCount].dwRoom = dwRoom;
    hRooms[hRoomsCount].dwExchange = 0;
    hRooms[hRoomsCount].hwnd = NULL;
    hRooms[hRoomsCount].dwMin = 0;
    hRoomsCount++;
    pthread_create(aim_gchat_open, (void *) (hRoomsCount - 1));
    pthread_mutex_unlock(&roomMutex);
    aim_gchat_storechat(szRoom);
    return;
}

void aim_gchat_delete(int dwRoom)
{
    int i;
    pthread_mutex_lock(&roomMutex);
    i = aim_gchat_find(dwRoom);
    if (i == -1) {
        LOG(LOG_WARN, "Invalid groupchat close sent to room id %d.  Group is closed?", dwRoom);
        pthread_mutex_unlock(&roomMutex);
        return;
    }
    if (i >= hRoomsCount || i < 0 || hRooms[i].dwRoom == -1) {
        pthread_mutex_unlock(&roomMutex);
        return;
    }
    if (aim_util_isonline())
        aim_gchat_exitnotice(dwRoom);
    pthread_mutex_unlock(&roomMutex);
}

void aim_gchat_sendmessage(int dwRoom, char *szUser, char *szMessage, int whisper)
{
    int i;
    char tmp[512], tmp2[512];
    TTOCChannelMessage cmsg;
    TTOCRoomMessage msg;

    ZeroMemory(&cmsg, sizeof(cmsg));
    ZeroMemory(&msg, sizeof(msg));
    i = aim_gchat_find(dwRoom);
    if (i == -1) {
        LOG(LOG_WARN, "Invalid message to room id %d from %s.  Room may be closed?", dwRoom, szUser);
        return;
    }
    msg.dwRoom = dwRoom;
    strcpy(msg.szUser, szUser);
    strcpy(cmsg.szRoom, hRooms[i].szRoom);
    aim_util_striphtml(tmp, szMessage, sizeof(tmp));
    if (whisper)
        mir_snprintf(tmp2, sizeof(tmp2), "%s %s", Translate("(Private)"), tmp);
    else
        mir_snprintf(tmp2, sizeof(tmp2), "%s", tmp);
    strcpy(msg.szMessage, tmp2);
    cmsg.dwType = AIM_GCHAT_CHANMESS;
    cmsg.msg = &msg;
    while (!hRooms[i].hwnd) {
        SleepEx(250, TRUE);
    }
    if (!aim_util_isme(szUser)) {
        SendMessage(hRooms[i].hwnd, GC_FLASHWINDOW, 0, 0);
        if (hRooms[i].dwMin)
            aim_gchat_activity();
    }
    aim_gchatlog_streaminevent(hRooms[i].hwnd, cmsg);
}

void aim_gchat_updatebuddy(int dwRoom, char *szUser, int joined)
{
    int i;

    i = aim_gchat_find(dwRoom);
    if (i == -1) {
        LOG(LOG_DEBUG, "Invalid groupchat buddy update to room id %d from %s.  Room may be closed?", dwRoom, szUser);
        return;
    }
    else {
        while (!hRooms[i].hwnd) {
            // Damn.  This might suck big time if hwnd is always NULL.
            // FIX ME.
            SleepEx(250, TRUE);
        }
        if (DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GJ, AIM_KEY_GJ_DEF) && !aim_util_isme(szUser)) {
            TTOCChannelMessage msg;

            ZeroMemory(&msg, sizeof(msg));
            msg.dwType = joined ? AIM_GCHAT_CHANJOIN : AIM_GCHAT_CHANPART;
            strcpy(msg.szUser, szUser);
            strcpy(msg.szRoom, hRooms[i].szRoom);
            aim_gchatlog_streaminevent(hRooms[i].hwnd, msg);
            SendMessage(hRooms[i].hwnd, GC_FLASHWINDOW, 0, 0);
            if (hRooms[i].dwMin)
                aim_gchat_activity();
        }
        SendMessage(hRooms[i].hwnd, GC_UPDATEBUDDY, (WPARAM) joined, (LPARAM) szUser);
    }
}

static void aim_gchat_sendinvite(char *szUser, int groupid, char *msg)
{
    char szMsg[MSG_LEN];
    char buf[MSG_LEN * 2];

    mir_snprintf(szMsg, sizeof(szMsg), "%s", msg);
    aim_util_escape(szMsg);
    mir_snprintf(buf, sizeof(buf), "toc_chat_invite %d \"%s\" %s", groupid, szMsg, szUser);
    aim_toc_sflapsend(buf, -1, TYPE_DATA);
}

static BOOL CALLBACK aim_gchat_invitereqdlg(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HANDLE dat = NULL;
    dat = (HANDLE) GetWindowLong(hwndDlg, GWL_USERDATA);
    switch (msg) {
        case WM_INITDIALOG:
        {
            int found = 0;
            DBVARIANT dbv;

            dat = (HANDLE) lParam;
            SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) dat);
            TranslateDialogDefault(hwndDlg);
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT)));
            if (!DBGetContactSetting(dat, AIM_PROTO, AIM_KEY_NK, &dbv)) {
                SetWindowText(GetDlgItem(hwndDlg, IDC_BUDDY), dbv.pszVal);
                DBFreeVariant(&dbv);
            }
            else if (!DBGetContactSetting(dat, AIM_PROTO, AIM_KEY_UN, &dbv)) {
                SetWindowText(GetDlgItem(hwndDlg, IDC_BUDDY), dbv.pszVal);
                DBFreeVariant(&dbv);
            }
            else
                EndDialog(hwndDlg, IDCANCEL);
            SetWindowText(GetDlgItem(hwndDlg, IDC_MESSAGE), Translate("Join me in this chat."));
            {
                int i, item;
                for (i = 0; i < hRoomsCount; i++) {
                    if (hRooms[i].hwnd && hRooms[i].szRoom) {
                        found = 1;
                        item = SendDlgItemMessage(hwndDlg, IDC_ROOM, CB_ADDSTRING, 0, (LPARAM) hRooms[i].szRoom);
                        SendDlgItemMessage(hwndDlg, IDC_ROOM, CB_SETITEMDATA, item, hRooms[i].dwRoom);
                    }
                }
            }
            if (!found) {
                EndDialog(hwndDlg, IDCANCEL);
            }
            SendDlgItemMessage(hwndDlg, IDC_ROOM, CB_SETCURSEL, 0, 0);
            SetFocus(GetDlgItem(hwndDlg, IDOK));
            break;
        }
        case WM_CLOSE:
            EndDialog(hwndDlg, IDCANCEL);
            break;
        case WM_DESTROY:
            break;
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
                case IDOK:
                {
                    char szMsg[128];
                    int groupid;
                    DBVARIANT dbv;

                    GetWindowText(GetDlgItem(hwndDlg, IDC_MESSAGE), szMsg, sizeof(szMsg));
                    groupid = SendDlgItemMessage(hwndDlg, IDC_ROOM, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_ROOM, CB_GETCURSEL, 0, 0), 0);
                    if (!DBGetContactSetting(dat, AIM_PROTO, AIM_KEY_UN, &dbv)) {
                        aim_gchat_sendinvite(dbv.pszVal, groupid, szMsg);
                        DBFreeVariant(&dbv);
                    }
                    else {
                        EndDialog(hwndDlg, IDCANCEL);
                    }
                }               // fall-through
                case IDCANCEL:
                {
                    EndDialog(hwndDlg, LOWORD(wParam));
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}

static int aim_gchat_invitereq(WPARAM wParam, LPARAM lParam)
{
    DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_GCHATINVITEREQ), NULL, aim_gchat_invitereqdlg, (LPARAM) wParam);
    return 0;
}

static int aim_gchat_prebuildmenu(WPARAM wParam, LPARAM lParam)
{
    CLISTMENUITEM mi;
    int i, found = 0;

    for (i = 0; i < hRoomsCount; i++) {
        if (hRooms[i].hwnd && hRooms[i].szRoom) {
            found = 1;
            break;
        }
    }
    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    if (found) {
        mi.flags = CMIM_FLAGS | CMIF_NOTOFFLINE;
    }
    else {
        mi.flags = CMIM_FLAGS | CMIF_NOTOFFLINE | CMIF_HIDDEN;
    }
    CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hGChatMenu, (LPARAM) & mi);
    return 0;
}

int aim_gchat_preshutdown(WPARAM wParam, LPARAM lParam)
{
    int i;

    aim_util_winlistbroadcast(hInviteList, WM_CLOSE, 0, 0);
    pthread_mutex_lock(&roomMutex);
    for (i = 0; i < hRoomsCount; i++) {
        if (hRooms[i].hwnd && IsWindow(hRooms[i].hwnd)) {
            // no need to logout since we are quiting although
            // this is done before we logoff so we could
            SendMessage(hRooms[i].hwnd, GC_QUIT, 0, 0);
        }
    }
    pthread_mutex_unlock(&roomMutex);
    return 0;
}

void aim_gchat_init()
{
    CLISTMENUITEM mi;
    char szService[MAX_PATH + 30];
    char pszTitle[256];

    if (strcmp(AIM_PROTO, AIM_PROTONAME)) {
        mir_snprintf(pszTitle, sizeof(pszTitle), "%s (%s)", AIM_PROTONAME, AIM_PROTO);
    }
    else
        mir_snprintf(pszTitle, sizeof(pszTitle), "%s", AIM_PROTO);
    pthread_mutex_init(&roomMutex);
    pthread_mutex_init(&findMutex);
    hRooms = NULL;
    hRoomsCount = 0;
    mir_snprintf(szService, sizeof(szService), "%s/AIM/GroupChatInviteCList", AIM_PROTO);
    CreateServiceFunction(szService, aim_gchat_inviteservice);
    mir_snprintf(szService, sizeof(szService), "%s/AIM/OpenGroupChat", AIM_PROTO);
    CreateServiceFunction(szService, aim_gchat_join);
    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.pszPopupName = pszTitle;
    mi.popupPosition = 500080000;
    if (DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GM, AIM_KEY_GM_DEF)) {
        mi.position = 500081000;
        mi.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT));
        mi.pszName = Translate("Join/Create Group Chat...");
        mi.pszService = szService;
        hMenuGroupChat = (HANDLE) CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) & mi);
    }
    mir_snprintf(szService, sizeof(szService), "%s/AIM/GroupChatInvite", AIM_PROTO);
    CreateServiceFunction(szService, aim_gchat_invitereq);
    mi.position = -2000080000;
    mi.flags = CMIF_NOTOFFLINE;
    mi.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT));
    mi.pszContactOwner = AIM_PROTO;
    mi.pszName = Translate("Invite to Group Chat...");
    mi.pszService = szService;
    hGChatMenu = (HANDLE) CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);
    hGChatHook = HookEvent(ME_CLIST_PREBUILDCONTACTMENU, aim_gchat_prebuildmenu);
    hGChatPreHook = HookEvent(ME_SYSTEM_PRESHUTDOWN, aim_gchat_preshutdown);
    aim_gchat_updatemenu();
    hCurSplitNS = LoadCursor(NULL, IDC_SIZENS);
    hInviteList = aim_util_winlistalloc();
}

void aim_gchat_destroy()
{
	TList *n = inviteList;

	while (n!=NULL) {
		TList *next = n->next;
		struct aim_gchat_chatinfo *info = (struct aim_gchat_chatinfo *)n->data;

		if (info) {
			if (info->szRoom) free(info->szRoom);
			if (info->szUser) free(info->szUser);
			if (info->szMsg) free(info->szMsg);
			if (info->chatid) free(info->chatid);
			free(info);
		}
		n = next;
	}
	if (inviteList) tlist_free(inviteList);
    LocalEventUnhook(hGChatHook);
    LocalEventUnhook(hGChatPreHook);
    if (hRoomsCount > 0) {
        free(hRooms);
        hRoomsCount = 0;
    }
    aim_gchat_traydestroy();
    DestroyCursor(hCurSplitNS);
    pthread_mutex_destroy(&roomMutex);
    pthread_mutex_destroy(&findMutex);
}

// tray stuff
#define AIMTRAYCLASS "__AIMTrayClass__"
#define TIM_CALLBACK (WM_USER+1323)
#define FLASH_COUNT	 5
#define FLASH_TIME   500
static HWND hTray = NULL;
static ATOM aTray = 0;

static NOTIFYICONDATA *nid = NULL;

static LRESULT CALLBACK aim_gchat_traywndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HMENU hMenu = NULL;
    switch (msg) {
        case TIM_CALLBACK:
        {
            switch (lParam) {
                case WM_LBUTTONDBLCLK:
                case WM_RBUTTONDOWN:
                {
                    int i, c = 0, last = 0;
                    POINT pt;

                    if (hMenu)
                        DestroyMenu(hMenu);
                    hMenu = CreatePopupMenu();
                    if (!hMenu)
                        break;

                    pthread_mutex_lock(&roomMutex);
                    for (i = 0; i < hRoomsCount; i++) {
                        if (hRooms[i].hwnd && IsWindow(hRooms[i].hwnd)) {
                            if (hRooms[i].dwMin) {
                                last = i;
                                c++;
                                AppendMenu(hMenu, MF_STRING, i + 1, hRooms[i].szRoom);
                            }
                        }
                    }
                    pthread_mutex_unlock(&roomMutex);
                    if (c == 1 && lParam == WM_LBUTTONDBLCLK) {
                        CHARRANGE cr = { 0 };

                        cr.cpMin = cr.cpMax = 1;
                        ShowWindow(hRooms[last].hwnd, SW_RESTORE);
                        SendMessage(hRooms[last].hwnd, GC_SCROLLLOGTOBOTTOM, 0, 0);
                        SendDlgItemMessage(hRooms[last].hwnd, IDC_LOG, EM_EXSETSEL, 0, (LPARAM) & cr);
                        SetForegroundWindow(hRooms[last].hwnd);
                        SetFocus(GetDlgItem(hRooms[last].hwnd, IDC_MESSAGE));
                    }
                    else if (c) {
                        SetForegroundWindow(hwnd);
                        SetFocus(hwnd);
                        GetCursorPos(&pt);
                        TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
                    }
                    else
                        aim_gchat_traydestroy();
                    DestroyMenu(hMenu);
                    break;
                }
            }
            return TRUE;
        }
        case WM_COMMAND:
        {
            int i = LOWORD(wParam) - 1;
            if (i >= 0 && i < hRoomsCount) {
                if (hRooms[i].hwnd && IsWindow(hRooms[i].hwnd) && hRooms[i].dwMin) {
                    NOTIFYICONDATA nnid = { 0 };
                    CHARRANGE cr = { 0 };

                    cr.cpMin = cr.cpMax = 1;
                    nnid.cbSize = sizeof(nnid);
                    nnid.uID = 200;
                    nnid.hWnd = hTray;
                    nnid.uFlags = NIF_ICON;
                    nnid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT));
                    if (nid->hIcon) {
                        DestroyIcon(nid->hIcon);
                        nid->hIcon = NULL;
                    }
                    Shell_NotifyIcon(NIM_MODIFY, &nnid);
                    hRooms[i].dwMin = 0;
                    ShowWindow(hRooms[i].hwnd, SW_RESTORE);
                    SendMessage(hRooms[i].hwnd, GC_SCROLLLOGTOBOTTOM, 0, 0);
                    SendDlgItemMessage(hRooms[i].hwnd, IDC_LOG, EM_EXSETSEL, 0, (LPARAM) & cr);
                    SetForegroundWindow(hRooms[i].hwnd);
                    SetFocus(GetDlgItem(hRooms[i].hwnd, IDC_MESSAGE));
                    aim_gchat_trayupdate();
                    break;
                }
            }
            return TRUE;
        }
        case WM_DESTROY:
        {
            if (hMenu)
                DestroyMenu(hMenu);
            return TRUE;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void aim_gchat_trayupdate()
{
    if (DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GT, AIM_KEY_GT_DEF)) {
        int i, dwShow = 0;
        pthread_mutex_lock(&roomMutex);
        for (i = 0; i < hRoomsCount; i++) {
            if (hRooms[i].hwnd && IsWindow(hRooms[i].hwnd)) {
                if (IsIconic(hRooms[i].hwnd)) {
                    dwShow++;
                    hRooms[i].dwMin = 1;
                    ShowWindow(hRooms[i].hwnd, SW_HIDE);
                }
            }
        }
        pthread_mutex_unlock(&roomMutex);
        if (dwShow) {
            if (!hTray) {
                WNDCLASS wc;

                if (hTray || aTray) {
                    return;
                }
                memset(&wc, 0, sizeof(WNDCLASS));
                wc.lpfnWndProc = aim_gchat_traywndproc;
                wc.hInstance = hInstance;
                wc.lpszClassName = AIMTRAYCLASS;
                aTray = RegisterClass(&wc);
                hTray = CreateWindowEx(0, AIMTRAYCLASS, "", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
            }
            if (!nid) {
                nid = (NOTIFYICONDATA *) malloc(sizeof(NOTIFYICONDATA));
                nid->cbSize = sizeof(NOTIFYICONDATA);
                nid->hWnd = hTray;
                nid->uID = 200;
                nid->uFlags = NIF_ICON | NIF_MESSAGE;
                nid->uCallbackMessage = TIM_CALLBACK;
                nid->hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHAT));
                Shell_NotifyIcon(NIM_ADD, nid);
            }
            {
                NOTIFYICONDATA nnid = { 0 };
                char szTmp[32];

                nnid.cbSize = sizeof(nnid);
                nnid.uID = 200;
                nnid.hWnd = hTray;
                nnid.uFlags = NIF_TIP;

                mir_snprintf(szTmp, sizeof(szTmp), Translate("AIM Group Chats (%d)"), dwShow);
                lstrcpyn(nnid.szTip, dwShow > 1 ? szTmp : Translate("AIM Group Chat"), sizeof(nnid.szTip));
                Shell_NotifyIcon(NIM_MODIFY, &nnid);
            }
        }
        else {
            aim_gchat_traydestroy();
        }
    }
}

static void aim_gchat_activity()
{
    if (nid && hTray) {
        NOTIFYICONDATA nnid = { 0 };

        nnid.cbSize = sizeof(nnid);
        nnid.uID = 200;
        nnid.hWnd = hTray;
        nnid.uFlags = NIF_ICON;
        nnid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GCHATA));
        if (nid->hIcon) {
            DestroyIcon(nid->hIcon);
            nid->hIcon = NULL;
        }
        Shell_NotifyIcon(NIM_MODIFY, &nnid);
    }
}

static void aim_gchat_traydestroy()
{
    if (nid) {
        NOTIFYICONDATA nnid = { 0 };

        if (nid->hIcon) {
            DestroyIcon(nid->hIcon);
            nid->hIcon = NULL;
        }
        nnid.cbSize = sizeof(nnid);
        nnid.uID = 200;
        nnid.hWnd = hTray;
        Shell_NotifyIcon(NIM_DELETE, &nnid);
        free(nid);
        nid = NULL;
    }
    if (hTray) {
        DestroyWindow(hTray);
        hTray = NULL;
    }
    if (aTray) {
        UnregisterClass(AIMTRAYCLASS, hInstance);
        aTray = 0;
    }
}
