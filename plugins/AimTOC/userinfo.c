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

static HANDLE hHookUserInfoInit;

static char *aim_getdatefmt(time_t sec)
{
    char *ret = malloc(256);
    time_t now = time(NULL) - sec;
    struct tm *timeinfo = localtime(&now);
    mir_snprintf(ret, 256, "%s", asctime(timeinfo));
    *(ret + strlen(ret) - 1) = '\0';
    return ret;
}

static char *aim_gettimefmt(time_t sec)
{
    int days, hrs, min;
    char *ret = malloc(256);

    if (sec < 0)
        sec = 0;
    days = sec / (60 * 60 * 24);
    hrs = (sec % (60 * 60 * 24)) / (60 * 60);
    min = (sec % (60 * 60)) / 60;
    if (days)
        mir_snprintf(ret, 256, "%d %s, %d %s, %d %s", days, days == 1 ? Translate("day") : Translate("days"), hrs,
                     hrs == 1 ? Translate("hour") : Translate("hours"), min, min == 1 ? Translate("minute") : Translate("minutes"));
    else if (hrs)
        mir_snprintf(ret, 256, "%d %s, %d %s", hrs, hrs == 1 ? Translate("hour") : Translate("hours"), min,
                     min == 1 ? Translate("minute") : Translate("minutes"));
    else if (min || sec == 0)
        mir_snprintf(ret, 256, "%d %s", min, min == 1 ? Translate("minute") : Translate("minutes"));
    else
        mir_snprintf(ret, 256, "%d %s", sec, sec == 1 ? Translate("second") : Translate("seconds"));
    return ret;
}

static char *aim_gettimefmtshort(time_t sec)
{
    int days, hrs, min;
    char *ret = malloc(256);

    if (sec < 0)
        sec = 0;
    days = sec / (60 * 60 * 24);
    hrs = (sec % (60 * 60 * 24)) / (60 * 60);
    min = (sec % (60 * 60)) / 60;
    if (days)
        mir_snprintf(ret, 256, "%d %s", days, days == 1 ? Translate("day") : Translate("days"));
    else if (hrs)
        mir_snprintf(ret, 256, "%d %s, %d %s", hrs, hrs == 1 ? Translate("hr") : Translate("hrs"), min,
                     min == 1 ? Translate("min") : Translate("mins"));
    else if (min || sec == 0)
        mir_snprintf(ret, 256, "%d %s", min, min == 1 ? Translate("min") : Translate("mins"));
    else
        mir_snprintf(ret, 256, "%d %s", sec, sec == 1 ? Translate("sec") : Translate("secs"));
    return ret;
}

static void aim_userinfo_inforeq(HANDLE hContact)
{
    DBVARIANT dbv;

    if (!aim_util_isonline())
        return;
    if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_UN, &dbv)) {
        char buf[MSG_LEN * 2];

        // force a UPDATE_BUDDY to get users latest info
        mir_snprintf(buf, sizeof(buf), "toc_get_status %s", dbv.pszVal);
        aim_toc_sflapsend(buf, -1, TYPE_DATA);

        SleepEx(250, FALSE);
        mir_snprintf(buf, sizeof(buf), "toc_get_info %s", dbv.pszVal);
        aim_toc_sflapsend(buf, -1, TYPE_DATA);
        DBFreeVariant(&dbv);
    }
}

void __cdecl aim_userinfo_infodummyshow(HANDLE hContact)
{
    SleepEx(500, FALSE);
    ProtoBroadcastAck(AIM_PROTO, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

void __cdecl aim_userinfo_infoshow(HANDLE hContact)
{
    // Don't request userinfo for offline users (AIM returns 901 error)
    if (DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_ST, ID_STATUS_OFFLINE) == ID_STATUS_OFFLINE) {
        SleepEx(500, FALSE);
        ProtoBroadcastAck(AIM_PROTO, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
    }
    else {
        aim_userinfo_inforeq(hContact);
    }
}

void aim_userinfo_send()
{
    DBVARIANT dbv;
    char buf[MSG_LEN * 2], buf2[MSG_LEN * 2];

    if (!aim_util_isonline())
        return;
    if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_PR, &dbv)) {
        mir_snprintf(buf, sizeof(buf), "%s", dbv.pszVal);
        DBFreeVariant(&dbv);
    }
    else
        mir_snprintf(buf, sizeof(buf), "%s", Translate(AIM_STR_PR));
    aim_util_escape(buf);
    mir_snprintf(buf2, sizeof(buf2), "toc_set_info \"%s\"", buf);
    aim_toc_sflapsend(buf2, -1, TYPE_DATA);
}

#define UI_UPDATEINFO	(WM_USER+1)
static BOOL CALLBACK aim_infodlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HANDLE hContact;

    hContact = (HANDLE) GetWindowLong(hwndDlg, GWL_USERDATA);
    switch (msg) {
        case WM_INITDIALOG:
        {
            TranslateDialogDefault(hwndDlg);
            hContact = (HANDLE) lParam;
            SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) hContact);
            SendMessage(GetDlgItem(hwndDlg, IDC_INFO), EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_LINK);
            SendMessage(GetDlgItem(hwndDlg, IDC_INFO), EM_AUTOURLDETECT, TRUE, 0);
            SendMessage(hwndDlg, UI_UPDATEINFO, 0, 0);
            break;
        }
        case UI_UPDATEINFO:
        {
            DBVARIANT dbv;
            char buf[256], buf2[256];
            int type;
            int status;
            time_t t;

            if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_NK, &dbv)) {
                int warnlevel = DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_EV, 0);

                mir_snprintf(buf2, sizeof(buf2), " (%d%%)", warnlevel);
                mir_snprintf(buf, sizeof(buf), "%s%s", dbv.pszVal, warnlevel > 0 ? buf2 : "");
                SetWindowText(GetDlgItem(hwndDlg, IDC_SN), buf);
                DBFreeVariant(&dbv);
            }
            else
                SetWindowText(GetDlgItem(hwndDlg, IDC_SN), Translate("N/A"));
            type = DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_TP, 0);

            if (aim_util_isicquserbyhandle(hContact))
                mir_snprintf(buf, sizeof(buf), Translate("ICQ User"));
            else if (type & USER_ADMIN)
                mir_snprintf(buf, sizeof(buf), Translate("Administrator"));
            else if (type & USER_UNCONFIRMED)
                mir_snprintf(buf, sizeof(buf), (type & USER_AOL) ? Translate("Unconfirmed AOL User") : Translate("Unconfirmed User"));
            else if (type & USER_NORMAL)
                mir_snprintf(buf, sizeof(buf), (type & USER_AOL) ? Translate("AOL User") : Translate("Normal User"));
            else if (type & USER_WIRELESS)
                mir_snprintf(buf, sizeof(buf), (type & USER_AOL) ? Translate("Wireless AOL User") : Translate("Wireless User"));
            else
                mir_snprintf(buf, sizeof(buf), (type & USER_AOL) ? Translate("Unknown AOL User") : Translate("Unknown User"));
            SetWindowText(GetDlgItem(hwndDlg, IDC_TYPE), buf);

            status = DBGetContactSettingWord(hContact, AIM_PROTO, AIM_KEY_ST, ID_STATUS_OFFLINE);
            // Online Time
            if (status == ID_STATUS_OFFLINE) {
                SetWindowText(GetDlgItem(hwndDlg, IDC_OL), Translate("Currently Offline"));
            }
            else {
                time_t t = (time_t) DBGetContactSettingDword(hContact, AIM_PROTO, AIM_KEY_OT, 0);
                if (t > 0) {
                    time_t now = time(NULL);
                    char buf[256];
                    char *d = aim_gettimefmtshort(now - t);
                    char *s = aim_getdatefmt(now - t);
                    mir_snprintf(buf, sizeof(buf), "%s (%s)", s, d);
                    SetWindowText(GetDlgItem(hwndDlg, IDC_OL), buf);
                    free(d);
                    free(s);
                }
                else {
                    SetWindowText(GetDlgItem(hwndDlg, IDC_OL), Translate("N/A"));
                }
            }
            // Idle Time
            if (status == ID_STATUS_OFFLINE) {
                t = 0;
                SetWindowText(GetDlgItem(hwndDlg, IDC_ID), Translate("N/A"));
            }
            else {
                char *s;
                t = (time_t) DBGetContactSettingDword(hContact, AIM_PROTO, AIM_KEY_IT, 0);
                s = aim_gettimefmt(t == 0 ? 0 : (time(NULL) - t));
                SetWindowText(GetDlgItem(hwndDlg, IDC_ID), t > 0 ? s : Translate("No"));
                free(s);
            }
            // Status
            mir_snprintf(buf, sizeof(buf), "%s %s%s%s", (char *) CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, status, 0), t > 0 ? "(" : "",
                         t > 0 ? Translate("Idle") : "", t > 0 ? ")" : "");
            SetWindowText(GetDlgItem(hwndDlg, IDC_STATUS), buf);
            if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_PR, &dbv)) {
                SetWindowText(GetDlgItem(hwndDlg, IDC_INFO), dbv.pszVal);
                DBFreeVariant(&dbv);
            }
            GetWindowText(GetDlgItem(hwndDlg, IDC_INFO), buf, sizeof(buf));
            if (strlen(buf) == 0)
                SetWindowText(GetDlgItem(hwndDlg, IDC_INFO), Translate("No User information Provided"));
            break;
        }
        case WM_NOTIFY:
        {
            switch (((LPNMHDR) lParam)->idFrom) {
                case IDC_INFO:
                    switch (((NMHDR *) lParam)->code) {
                        case EN_LINK:
                            switch (((ENLINK *) lParam)->msg) {
                                case WM_RBUTTONDOWN:
                                case WM_LBUTTONUP:
                                {
                                    TEXTRANGE tr;
                                    CHARRANGE sel;

                                    SendDlgItemMessage(hwndDlg, IDC_INFO, EM_EXGETSEL, 0, (LPARAM) & sel);
                                    if (sel.cpMin != sel.cpMax)
                                        break;
                                    tr.chrg = ((ENLINK *) lParam)->chrg;
                                    tr.lpstrText = malloc(tr.chrg.cpMax - tr.chrg.cpMin + 8);
                                    SendDlgItemMessage(hwndDlg, IDC_INFO, EM_GETTEXTRANGE, 0, (LPARAM) & tr);
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
                                        SetFocus(hwndDlg);
                                    }

                                    free(tr.lpstrText);
                                    break;
                                }
                            }
                            break;
                    }
                    break;
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
                        case PSN_INFOCHANGED:
                            SendMessage(hwndDlg, UI_UPDATEINFO, 0, 0);
                            break;
                    }
                    break;
            }
            break;
        }
    }
    return FALSE;
}

static BOOL CALLBACK aim_infoownerdlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HANDLE hContact;

    hContact = (HANDLE) GetWindowLong(hwndDlg, GWL_USERDATA);
    switch (msg) {
        case WM_INITDIALOG:
        {
            TranslateDialogDefault(hwndDlg);
            hContact = (HANDLE) lParam;
            SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) hContact);
            SendMessage(hwndDlg, UI_UPDATEINFO, 0, 0);
            break;
        }
        case UI_UPDATEINFO:
        {
            DBVARIANT dbv;

            if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_NO, &dbv)) {
                SetWindowText(GetDlgItem(hwndDlg, IDC_SN), dbv.pszVal);
                DBFreeVariant(&dbv);
            }
            else
                SetWindowText(GetDlgItem(hwndDlg, IDC_SN), Translate("N/A"));
            if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_PR, &dbv)) {
                SetWindowText(GetDlgItem(hwndDlg, IDC_INFO), dbv.pszVal);
                DBFreeVariant(&dbv);
            }
            else
                SetWindowText(GetDlgItem(hwndDlg, IDC_INFO), Translate(AIM_STR_PR));
            EnableWindow(GetDlgItem(hwndDlg, IDC_SAVEPROFILE), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SAVESN), FALSE);
            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
                case IDC_LOSTLINK:
                {
                    aim_util_browselostpwd();
                    break;
                }
                case IDC_CHANGEPW:
                {
                    if (aim_util_isonline()) {
                        aim_password_change(0, 0);
                    }
                    else {
                        DBVARIANT dbv;
                        char buf[256];

                        if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_UN, &dbv)) {
                            mir_snprintf(buf, sizeof(buf), "%s%s", AIM_CHANGEPW, dbv.pszVal);
                            DBFreeVariant(&dbv);
                        }
                        else
                            mir_snprintf(buf, sizeof(buf), "%s", AIM_CHANGEPW);
                        CallService(MS_UTILS_OPENURL, (WPARAM) 1, (LPARAM) buf);
                    }
                    break;
                }
                case IDC_INFO:
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                        EnableWindow(GetDlgItem(hwndDlg, IDC_SAVEPROFILE), TRUE);
                    break;
                }
                case IDC_SN:
                {
                    if (HIWORD(wParam) == EN_CHANGE && aim_util_isonline())
                        EnableWindow(GetDlgItem(hwndDlg, IDC_SAVESN), TRUE);
                    break;
                }
                case IDC_SAVEPROFILE:
                {
                    char buf[1024];

                    GetWindowText(GetDlgItem(hwndDlg, IDC_INFO), buf, sizeof(buf));
                    DBWriteContactSettingString(NULL, AIM_PROTO, AIM_KEY_PR, buf);
                    if (aim_util_isonline()) {
                        aim_userinfo_send();
                    }
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SAVEPROFILE), FALSE);
                    break;
                }
                case IDC_SAVESN:
                {
                    if (aim_util_isonline()) {
                        char buf[256];
                        char *nick;

                        GetWindowText(GetDlgItem(hwndDlg, IDC_SN), buf, sizeof(buf));
                        nick = aim_util_normalize(buf);
                        if (!aim_util_isme(nick)) {
                            MessageBox(hwndDlg, Translate("Formatted screenname does not match your screenname."), Translate("AIM Error"),
                                       MB_OK | MB_ICONERROR);
                        }
                        else {
                            aim_util_formatnick(buf);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_SAVESN), FALSE);
                        }
                    }
                    break;
                }
            }
            break;
        }
        case WM_NOTIFY:
        {
            switch (((LPNMHDR) lParam)->idFrom) {
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
                        case PSN_INFOCHANGED:
                            SendMessage(hwndDlg, UI_UPDATEINFO, 0, 0);
                            break;
                    }
                    break;
                    break;
            }
            break;
        }
    }
    return FALSE;
}

static int aim_initdetails(WPARAM wParam, LPARAM lParam)
{
    char *szProto;
    HANDLE hContact;
    OPTIONSDIALOGPAGE odp;
    char pszTitle[256];

    if (strcmp(AIM_PROTO, AIM_PROTONAME)) {
        mir_snprintf(pszTitle, sizeof(pszTitle), "%s (%s)", AIM_PROTONAME, AIM_PROTO);
    }
    else
        mir_snprintf(pszTitle, sizeof(pszTitle), "%s", AIM_PROTO);
    odp.cbSize = sizeof(odp);
    odp.hIcon = NULL;
    odp.hInstance = hInstance;
    odp.position = -1900000000;
    hContact = (HANDLE) lParam;
    if (!hContact) {
        odp.pfnDlgProc = aim_infoownerdlgproc;
        odp.pszTemplate = MAKEINTRESOURCE(IDD_INFO_OWNER);
        odp.pszTitle = pszTitle;
        CallService(MS_USERINFO_ADDPAGE, wParam, (LPARAM) & odp);
    }
    else {
        szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, lParam, 0);
        if (hContact == NULL || szProto == NULL || strcmp(szProto, AIM_PROTO))
            return 0;
        {
            odp.pfnDlgProc = aim_infodlgproc;
            odp.pszTemplate = MAKEINTRESOURCE(IDD_INFO_AIM);
            odp.pszTitle = pszTitle;
            CallService(MS_USERINFO_ADDPAGE, wParam, (LPARAM) & odp);
        }
    }
    return 0;
}

void aim_userinfo_init()
{
    hHookUserInfoInit = HookEvent(ME_USERINFO_INITIALISE, aim_initdetails);
}

void aim_userinfo_destroy()
{
    LocalEventUnhook(hHookUserInfoInit);
}
