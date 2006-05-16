/*
AOL Instant Messenger Plugin for Miranda IM

Copyright © 2003 Robert Rainwater

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

static HANDLE hMenuPassword;
static char *szNewPassword = NULL;
static HWND hwndPwd = NULL;

static BOOL CALLBACK aim_password_dlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
        {
            TranslateDialogDefault(hwndDlg);
            hwndPwd = hwndDlg;
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PASSWORD)));
            EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
            break;
        }
        case WM_CLOSE:
            EndDialog(hwndDlg, IDCANCEL);
            break;
        case WM_DESTROY:
            hwndPwd = NULL;
            break;
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
                case IDC_LOSTLINK:
                {
                    aim_util_browselostpwd();
                    break;
                }
                case IDC_OLDPASS:
                case IDC_NEWPASS:
                case IDC_NEWPASSV:
                {
                    DBVARIANT dbv;

                    if (!GetWindowTextLength(GetDlgItem(hwndDlg, IDC_OLDPASS)) ||
                        !GetWindowTextLength(GetDlgItem(hwndDlg, IDC_NEWPASS)) || !GetWindowTextLength(GetDlgItem(hwndDlg, IDC_NEWPASSV))) {
                        EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
                        break;
                    }
                    if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_PW, &dbv)) {
                        char str[128];
                        GetDlgItemText(hwndDlg, IDC_OLDPASS, str, sizeof(str));
                        CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
                        if (!strcmp(dbv.pszVal, str)) {
                            char szPass[128], szVPass[128];

                            GetDlgItemText(hwndDlg, IDC_NEWPASS, szPass, sizeof(szPass));
                            GetDlgItemText(hwndDlg, IDC_NEWPASSV, szVPass, sizeof(szVPass));
                            if (!strcmp(szPass, szVPass)) {
                                EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);
                                DBFreeVariant(&dbv);
                                break;
                            }
                        }
                        DBFreeVariant(&dbv);
                    }
                    EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
                    break;
                }
                case IDOK:
                {
                    char szOldPass[128], szNewPass[128], buf[MSG_LEN * 2];

                    GetDlgItemText(hwndDlg, IDC_OLDPASS, szOldPass, sizeof(szOldPass));
                    GetDlgItemText(hwndDlg, IDC_NEWPASS, szNewPass, sizeof(szNewPass));

                    mir_snprintf(buf, sizeof(buf), "toc_change_passwd %s %s", szOldPass, szNewPass);
                    aim_toc_sflapsend(buf, -1, TYPE_DATA);
                    if (szNewPassword) {
                        free(szNewPassword);
                    }
                    szNewPassword = _strdup(szNewPass);
                }               // fall-through
                case IDCANCEL:
                    EndDialog(hwndDlg, LOWORD(wParam));
            }
        }
    }
    return FALSE;
}

int aim_password_change(WPARAM wParam, LPARAM lParam)
{
    if (hwndPwd)
        SetActiveWindow(hwndPwd);
    else
        DialogBox(hInstance, MAKEINTRESOURCE(IDD_PASSWORD), NULL, aim_password_dlgproc);
    return 0;
}

void aim_password_updatemenu()
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
    CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hMenuPassword, (LPARAM) & cli);
}

void aim_password_update(int good)
{
    if (good) {
        if (ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
            aim_util_shownotification(Translate("AIM Password Change"), Translate("Your password has been successfully changed."), NIIF_INFO);
        }
        else
            MessageBox(0, Translate("Your password has been successfully changed."), Translate("AIM Password Change"), MB_OK | MB_ICONINFORMATION);
        if (szNewPassword) {
            CallService(MS_DB_CRYPT_ENCODESTRING, strlen(szNewPassword) + 1, (LPARAM) szNewPassword);
            DBWriteContactSettingString(NULL, AIM_PROTO, AIM_KEY_PW, szNewPassword);
            free(szNewPassword);
            szNewPassword = NULL;
        }
    }
}

void aim_password_init()
{
    CLISTMENUITEM mi;
    char szService[MAX_PATH + 30];
    char pszTitle[256];

    if (strcmp(AIM_PROTO, AIM_PROTONAME)) {
        mir_snprintf(pszTitle, sizeof(pszTitle), "%s (%s)", AIM_PROTONAME, AIM_PROTO);
    }
    else
        mir_snprintf(pszTitle, sizeof(pszTitle), "%s", AIM_PROTO);
    mir_snprintf(szService, sizeof(szService), "%s/AIM/ChangePassword", AIM_PROTO);
    CreateServiceFunction(szService, aim_password_change);
    if (DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_PM, AIM_KEY_PM_DEF)) {
        ZeroMemory(&mi, sizeof(mi));
        mi.cbSize = sizeof(mi);
        mi.pszPopupName = pszTitle;
        mi.popupPosition = 500080000;
        mi.position = 500080000;
        mi.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PASSWORD));
        mi.pszName = Translate("Change Password...");
        mi.pszService = szService;
        hMenuPassword = (HANDLE) CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) & mi);
        aim_password_updatemenu();
    }
}

void aim_password_destroy()
{
    if (hwndPwd)
        SendMessage(hwndPwd, WM_CLOSE, 0, 0);
    if (szNewPassword) {
        free(szNewPassword);
        szNewPassword = NULL;
    }
}
