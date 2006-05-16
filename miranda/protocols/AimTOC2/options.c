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

int importBuddies = 0;
static BOOL CALLBACK aim_options_generaloptsproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK aim_options_contactoptsproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK aim_options_gchatoptsproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

int aim_options_init(WPARAM wParam, LPARAM lParam)
{
    OPTIONSDIALOGPAGE odp;
    char pszTitle[256];

    ZeroMemory(&odp, sizeof(odp));
    if (strcmp(AIM_PROTO, AIM_PROTONAME)) {
        mir_snprintf(pszTitle, sizeof(pszTitle), "(%s) %s", AIM_PROTO, AIM_PROTONAME);
    }
    else
        mir_snprintf(pszTitle, sizeof(pszTitle), "AIM");
    odp.cbSize = sizeof(odp);
    odp.position = 1003000;
    odp.hInstance = hInstance;
    odp.pszTemplate = MAKEINTRESOURCE(IDD_OPT_AIM);
    odp.pszGroup = Translate("Network");
    odp.pszTitle = pszTitle;
    odp.pfnDlgProc = aim_options_generaloptsproc;
    odp.flags = ODPF_BOLDGROUPS;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp);
    odp.pszTemplate = MAKEINTRESOURCE(IDD_OPT_AIMCONTACTS);
    if (strcmp(AIM_PROTO, AIM_PROTONAME)) {
        mir_snprintf(pszTitle, sizeof(pszTitle), Translate("(%s) AIM Contacts"), AIM_PROTO);
    }
    else
        mir_snprintf(pszTitle, sizeof(pszTitle), Translate("AIM Contacts"));
    odp.pszTitle = pszTitle;
    odp.pfnDlgProc = aim_options_contactoptsproc;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp);
    return 0;
}

static BOOL CALLBACK aim_options_generaloptsproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
        {
            DBVARIANT dbv;

            TranslateDialogDefault(hwndDlg);
            if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_UN, &dbv)) {
                SetDlgItemText(hwndDlg, IDC_SN, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
            if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_PW, &dbv)) {
                CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
                SetDlgItemText(hwndDlg, IDC_PW, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
            if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_NK, &dbv)) {
                SetDlgItemText(hwndDlg, IDC_DN, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
            else if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_UN, &dbv)) {
                SetDlgItemText(hwndDlg, IDC_DN, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
            if (!DBGetContactSetting(NULL, AIM_PROTO, AIM_KEY_TS, &dbv)) {
                SetDlgItemText(hwndDlg, IDC_TOCSERVER, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
            else
                SetDlgItemText(hwndDlg, IDC_TOCSERVER, AIM_TOC_HOST);
            SetDlgItemInt(hwndDlg, IDC_TOCPORT, DBGetContactSettingWord(NULL, AIM_PROTO, AIM_KEY_TT, AIM_TOC_PORT), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_IDLETIMEVAL), IsDlgButtonChecked(hwndDlg, IDC_ENABLEIDLE));
            CheckDlgButton(hwndDlg, IDC_SHOWERRORS, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_SE, AIM_KEY_SE_DEF));
            CheckDlgButton(hwndDlg, IDC_WEBSUPPORT, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_AL, AIM_KEY_AL_DEF));
            CheckDlgButton(hwndDlg, IDC_PASSWORD, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_PM, AIM_KEY_PM_DEF));
            CheckDlgButton(hwndDlg, IDC_SHOWJOINMENU, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GM, AIM_KEY_GM_DEF));
            CheckDlgButton(hwndDlg, IDC_IGNOREJOIN, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GI, AIM_KEY_GI_DEF));
			CheckDlgButton(hwndDlg, IDC_SHOWSYNCMENU, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_LSM,AIM_KEY_SM_DEF));
            break;
        }
        case WM_COMMAND:
        {
            if ((LOWORD(wParam) == IDC_SN || LOWORD(wParam) == IDC_PW || LOWORD(wParam) == IDC_TOCSERVER || LOWORD(wParam) == IDC_TOCPORT
                 || LOWORD(wParam) == IDC_DN) && (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
                return 0;
            switch (LOWORD(wParam)) {
                case IDC_RESETSERVER:
                    SetDlgItemText(hwndDlg, IDC_TOCSERVER, AIM_TOC_HOST);
                    SetDlgItemInt(hwndDlg, IDC_TOCPORT, AIM_TOC_PORT, FALSE);
                    SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
                    return TRUE;
                case IDC_LOSTLINK:
                {
                    aim_util_browselostpwd();
                    return TRUE;
                }
                case IDC_ENABLEIDLE:
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_IDLETIMEVAL), IsDlgButtonChecked(hwndDlg, IDC_ENABLEIDLE));
                    ShowWindow(GetDlgItem(hwndDlg, IDC_RELOADREQD), SW_SHOW);
                    break;
                }
				case IDC_SHOWJOINMENU:
                case IDC_PASSWORD:
				case IDC_SHOWSYNCMENU:
                {
                    ShowWindow(GetDlgItem(hwndDlg, IDC_RELOADREQD), SW_SHOW);
                    break;
                }
            }
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            break;
        }
        case WM_NOTIFY:
        {
            switch (((LPNMHDR) lParam)->code) {
                case PSN_APPLY:
                {
                    char str[128];
                    int val;

                    if (!IsDlgButtonChecked(hwndDlg, IDC_WEBSUPPORT) && DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_AL, AIM_KEY_AL_DEF)) {
                        aim_links_unregister();
                    }
                    if (IsDlgButtonChecked(hwndDlg, IDC_WEBSUPPORT))
                        aim_links_init();
                    else
                        aim_links_destroy();
                    if (IsDlgButtonChecked(hwndDlg, IDC_KEEPALIVE))
                        aim_keepalive_init();
                    else
                        aim_keepalive_destroy();
                    GetDlgItemText(hwndDlg, IDC_SN, str, sizeof(str));
                    DBWriteContactSettingString(NULL, AIM_PROTO, AIM_KEY_UN, aim_util_normalize(str));
                    GetDlgItemText(hwndDlg, IDC_PW, str, sizeof(str));
                    CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(str), (LPARAM) str);
                    DBWriteContactSettingString(NULL, AIM_PROTO, AIM_KEY_PW, str);
                    GetDlgItemText(hwndDlg, IDC_DN, str, sizeof(str));
                    if (strlen(str) > 0)
                        DBWriteContactSettingString(NULL, AIM_PROTO, AIM_KEY_NK, str);
                    else
                        DBDeleteContactSetting(NULL, AIM_PROTO, AIM_KEY_NK);
                    GetDlgItemText(hwndDlg, IDC_TOCSERVER, str, sizeof(str));
                    DBWriteContactSettingString(NULL, AIM_PROTO, AIM_KEY_TS, str);
                    val = GetDlgItemInt(hwndDlg, IDC_TOCPORT, NULL, TRUE);
                    DBWriteContactSettingWord(NULL, AIM_PROTO, AIM_KEY_TT, (WORD) val > 0 || val == 0 ? val : AIM_TOC_PORT);
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_SE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWERRORS));
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_AL, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_WEBSUPPORT));
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_PM, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_PASSWORD));
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GM, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWJOINMENU));
					DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_LSM, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWSYNCMENU));
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GI, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_IGNOREJOIN));
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}

static BOOL CALLBACK aim_options_contactoptsproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
        {
            TranslateDialogDefault(hwndDlg);

            CheckDlgButton(hwndDlg, IDC_OPTWARNMENU, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_WM, AIM_KEY_WM_DEF));
            CheckDlgButton(hwndDlg, IDC_OPTWARNNOTIFY, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_WD, AIM_KEY_WD_DEF));
            CheckDlgButton(hwndDlg, IDC_AWAYRESPONSE, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_AR, AIM_KEY_AR_DEF));
            CheckDlgButton(hwndDlg, IDC_AWAYCLIST, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_AC, AIM_KEY_AC_DEF));
            EnableWindow(GetDlgItem(hwndDlg, IDC_AWAYCLIST), IsDlgButtonChecked(hwndDlg, IDC_AWAYRESPONSE));
            break;
        }
        case WM_COMMAND:
        {
            if ((HWND) lParam != GetFocus())
                return 0;
            switch (LOWORD(wParam)) {
                case IDC_AWAYRESPONSE:
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_AWAYCLIST), IsDlgButtonChecked(hwndDlg, IDC_AWAYRESPONSE));
                    break;
                }
            }
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            break;
        }
        case WM_NOTIFY:
        {
            switch (((LPNMHDR) lParam)->code) {
                case PSN_APPLY:
                {
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_WM, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_OPTWARNMENU));
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_WD, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_OPTWARNNOTIFY));
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_AR, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AWAYRESPONSE));
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_AC, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AWAYCLIST));
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}
