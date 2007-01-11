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

static HANDLE hUserMenu = NULL;
static HANDLE hHookMenu;

static void aim_evil_warnsend(HANDLE hContact)
{
    DBVARIANT dbv;

    if (aim_util_isonline() && !DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_UN, &dbv)) {
        char buf[MSG_LEN * 2];

        mir_snprintf(buf, sizeof(buf), "toc_evil %s \"norm\"", dbv.pszVal);
        aim_toc_sflapsend(buf, -1, TYPE_DATA);
        DBFreeVariant(&dbv);
    }
}

static void aim_evilwarnuser(HANDLE hContact)
{
    DBVARIANT dbv;
    char buf[256];
    char tbuf[128];

    if (!DBGetContactSetting(hContact, AIM_PROTO, AIM_KEY_UN, &dbv)) {
        char *clistName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM) hContact, 0);
        if (strcmp(clistName, dbv.pszVal)) {
            mir_snprintf(tbuf, sizeof(tbuf), Translate("Warn %s (%s)"), clistName, dbv.pszVal);
        }
        else
            mir_snprintf(tbuf, sizeof(tbuf), Translate("Warn %s"), dbv.pszVal);
        mir_snprintf(buf, sizeof(buf),
                     Translate
                     ("Would you like to warn %s?\r\n\r\nYou must have exchanged messages with this user recently in order for this to work."),
                     dbv.pszVal);

        if (MessageBox(0, buf, tbuf, MB_YESNO | MB_ICONQUESTION) == IDYES) {
            aim_evil_warnsend(hContact);
        }
        DBFreeVariant(&dbv);
    }
}

static int aim_peformevilmenu(WPARAM wParam, LPARAM lParam)
{
    aim_evilwarnuser((HANDLE) wParam);
    return 0;
}

static int aim_prebuildevilmenu(WPARAM wParam, LPARAM lParam)
{
    CLISTMENUITEM mi;
    char buf[256];

    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    // Don't show menu item if you set the option to hide or you haven't received a message from 
    // them in the last AIM_EVIL_TO minutes
    // Note: Hide menu for chat rooms too.  Duh!
    // Note: AOL doesn't accept warnings from users who havent sent you a message recently
    if (DBGetContactSettingByte((HANDLE) wParam, AIM_PROTO, AIM_CHAT, 0) ||
        !DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_WM, AIM_KEY_WM_DEF) ||
        (time(NULL) - DBGetContactSettingDword((HANDLE) wParam, AIM_PROTO, AIM_KEY_LM, 0)) / 60 > AIM_EVIL_TO)
        mi.flags = CMIM_FLAGS | CMIF_NOTOFFLINE | CMIF_HIDDEN;
    else {
        mi.flags = CMIM_FLAGS | CMIF_NOTOFFLINE | CMIM_NAME;
        mir_snprintf(buf, sizeof(buf), "%s (%d%%)", Translate("Warn User"), DBGetContactSettingWord((HANDLE) wParam, AIM_PROTO, AIM_KEY_EV, 0));
        mi.pszName = buf;
    }
    CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hUserMenu, (LPARAM) & mi);
    return 0;
}

void aim_evil_init()
{
    CLISTMENUITEM mi;
    char szService[MAX_PATH + 30];

    mir_snprintf(szService, sizeof(szService), "%s%s", AIM_PROTO, AIM_SVC_EVIL);
    CreateServiceFunction(szService, aim_peformevilmenu);
    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    mi.position = 1000090000;
    mi.flags = CMIF_NOTOFFLINE;
    mi.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WARN));
    mi.pszContactOwner = AIM_PROTO;
    mi.pszName = Translate("Warn User");
    mi.pszService = szService;
    hUserMenu = (HANDLE) CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) & mi);
    hHookMenu = HookEvent(ME_CLIST_PREBUILDCONTACTMENU, aim_prebuildevilmenu);
}

void aim_evil_destroy()
{
    LocalEventUnhook(hHookMenu);
    LOG(LOG_DEBUG, "aim_evil_destroy(): unhooked menu");
}

typedef struct warninfo_t
{
    char *szUser;
    int dwWarnLevel;
}
warninfo;

static BOOL CALLBACK aim_warneddlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
        {
            char buf[256];
            warninfo *sinfo;

            TranslateDialogDefault(hwndDlg);
            sinfo = (warninfo *) lParam;
            if (!sinfo)
                EndDialog(hwndDlg, 0);
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WARN)));
            SetWindowText(GetDlgItem(hwndDlg, IDC_ID), sinfo->szUser);
            mir_snprintf(buf, sizeof(buf), "%d%%", sinfo->dwWarnLevel);
            SetWindowText(GetDlgItem(hwndDlg, IDC_WARN), buf);
            SetFocus(hwndDlg);
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
                case IDOK:
                {
                    EndDialog(hwndDlg, 0);
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}

static void __cdecl aim_evil_dialogthread(void *info)
{
    warninfo *sinfo = (warninfo *) info;
    DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_AIMWARNED), NULL, aim_warneddlgproc, (LPARAM) sinfo);
    if (sinfo->szUser)
        free(sinfo->szUser);
    free(sinfo);
    sinfo = NULL;
}

void aim_evil_update(char *szUser, int level)
{
    int oldLevel;

    oldLevel = DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_EV, 0);
    if (level > oldLevel || szUser) {   // This is a real warning
        if (ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
            char buf[256];

            mir_snprintf(buf, sizeof(buf), Translate("You were warned by %s.  Your new warning level is %d%%."),
                         szUser ? szUser : Translate("Anonymous"), level);
            aim_util_shownotification(Translate("AIM Warning"), buf, NIIF_WARNING);
        }
        else {
            warninfo *sinfo;

            sinfo = (warninfo *) malloc(sizeof(warninfo));
            sinfo->szUser = _strdup(szUser ? szUser : Translate("Anonymous"));
            sinfo->dwWarnLevel = level;
            pthread_create(aim_evil_dialogthread, (void *) sinfo);
        }
    }
    else if (oldLevel != 0 && level != 0) {     // Our warning is being lowered
        if (ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {   // only show warning level being lowered for notification enabled people
            char buf[256];

            mir_snprintf(buf, sizeof(buf), Translate("Your warning level has been changed to %d%%."), szUser, level);
            aim_util_shownotification(Translate("AIM Warning Level Change"), buf, NIIF_WARNING);
        }
    }
}
