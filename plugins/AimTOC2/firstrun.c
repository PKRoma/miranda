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

int firstRun = 0;
static BOOL CALLBACK aim_firstrun_dlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

void aim_firstrun_check()
{
    DBVARIANT dbv;

    if (DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_FR, 0))
        return;
    firstRun = 1;
    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_FR, 1);
    if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_UN, &dbv)) {
        DBFreeVariant(&dbv);
        return;
    }
    if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_PW, &dbv)) {
        DBFreeVariant(&dbv);
        return;
    }
    if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_AIMLOGIN), NULL, aim_firstrun_dlgproc) == IDCANCEL) {
        if (MessageBox
            (0, Translate("Do you want to disable the AIM protocol (requires restart)?"), Translate("AIM Protocol"),
             MB_ICONQUESTION | MB_YESNO) == IDYES) {
            char buf[MAX_PATH];

            mir_snprintf(buf, sizeof(buf), "%s.dll", AIM_PROTO);
            DBWriteContactSettingByte(NULL, "PluginDisable", buf, 1);
        }
    }
}

static BOOL CALLBACK aim_firstrun_dlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
        {
            TranslateDialogDefault(hwndDlg);
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInstance, MAKEINTRESOURCE(IDI_AIM)));
            break;
        }
        case WM_CLOSE:
        {
            EndDialog(hwndDlg, IDCANCEL);
            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
                case IDC_REGISTER:
                {
                    CallService(MS_UTILS_OPENURL, (WPARAM) 1, (LPARAM) AIM_REGISTER);
                    break;
                }
                case IDOK:
                {
                    char str[128];

                    GetDlgItemText(hwndDlg, IDC_SN, str, sizeof(str));
                    DBWriteContactSettingString(NULL, AIM_PROTO, AIM_KEY_UN, str);
                    GetDlgItemText(hwndDlg, IDC_PW, str, sizeof(str));
                    CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(str), (LPARAM) str);
                    DBWriteContactSettingString(NULL, AIM_PROTO, AIM_KEY_PW, str);
                }               // fall through
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
