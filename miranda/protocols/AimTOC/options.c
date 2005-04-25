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
    /*odp.pszTemplate = MAKEINTRESOURCE(IDD_OPT_AIMGCHAT);
       if (strcmp(AIM_PROTO, AIM_PROTONAME)) {
       mir_snprintf(pszTitle, sizeof(pszTitle), Translate("(%s) AIM Group Chat"), AIM_PROTO);
       }
       else
       mir_snprintf(pszTitle, sizeof(pszTitle), Translate("AIM Group Chat"));
       odp.pszTitle = pszTitle;
       odp.pfnDlgProc = aim_options_gchatoptsproc;
       CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp); */
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
                case IDC_PASSWORD:
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
            CheckDlgButton(hwndDlg, IDC_DOWNLOADCTCS, importBuddies);
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
                    importBuddies = IsDlgButtonChecked(hwndDlg, IDC_DOWNLOADCTCS);
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}

/*
struct FontOptionsList
{
    char *szDescr;
    COLORREF defColour;
    char *szDefFace;
    BYTE defCharset, defStyle;
    char defSize;
    COLORREF colour;
    char szFace[LF_FACESIZE];
    BYTE charset, style;
    char size;
}
static fontOptionsList[] = {
    {"Name", RGB(73, 73, 112), "Arial", DEFAULT_CHARSET, FONTF_BOLD, -11},
    {"Message", RGB(0, 0, 0), "Arial", DEFAULT_CHARSET, 0, -11},
    {"Date", RGB(80, 80, 80), "Arial", DEFAULT_CHARSET, 0, -11},
    {"My Name", RGB(73, 73, 112), "Arial", DEFAULT_CHARSET, FONTF_BOLD, -11},
    {"My Message", RGB(0, 0, 0), "Arial", DEFAULT_CHARSET, 0, -11},
    {"My Date", RGB(80, 80, 80), "Arial", DEFAULT_CHARSET, 0, -11},
    {"Parts/Joins", RGB(169, 52, 52), "Arial", DEFAULT_CHARSET, 0, -11},
    {"Send Area", RGB(0, 0, 0), "Arial", DEFAULT_CHARSET, 0, -11},
    {"User List", RGB(0, 0, 0), "Arial", DEFAULT_CHARSET, 0, -11}
};
const int msgDlgFontCount = sizeof(fontOptionsList) / sizeof(fontOptionsList[0]);

void LoadGroupChatFont(int i, LOGFONT * lf, COLORREF * colour)
{
    char str[32];
    int style;
    DBVARIANT dbv;

    if (colour) {
        mir_snprintf(str, sizeof(str), "GCFont%dColor", i);
        *colour = DBGetContactSettingDword(NULL, AIM_PROTO, str, fontOptionsList[i].defColour);
    }
    if (lf) {
        mir_snprintf(str, sizeof(str), "GCFont%dSize", i);
        lf->lfHeight = (char) DBGetContactSettingByte(NULL, AIM_PROTO, str, fontOptionsList[i].defSize);
        lf->lfWidth = 0;
        lf->lfEscapement = 0;
        lf->lfOrientation = 0;
        mir_snprintf(str, sizeof(str), "GCFont%dStyle", i);
        style = DBGetContactSettingByte(NULL, AIM_PROTO, str, fontOptionsList[i].defStyle);
        lf->lfWeight = style & FONTF_BOLD ? FW_BOLD : FW_NORMAL;
        lf->lfItalic = style & FONTF_ITALIC ? 1 : 0;
        lf->lfUnderline = 0;
        lf->lfStrikeOut = 0;
        mir_snprintf(str, sizeof(str), "GCFont%dCharSet", i);
        lf->lfCharSet = DBGetContactSettingByte(NULL, AIM_PROTO, str, fontOptionsList[i].defCharset);
        lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
        lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
        lf->lfQuality = DEFAULT_QUALITY;
        lf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        mir_snprintf(str, sizeof(str), "GCFont%dFace", i);
        if (DBGetContactSetting(NULL, AIM_PROTO, str, &dbv))
            lstrcpy(lf->lfFaceName, fontOptionsList[i].szDefFace);
        else {
            lstrcpyn(lf->lfFaceName, dbv.pszVal, sizeof(lf->lfFaceName));
            DBFreeVariant(&dbv);
        }
    }
}

// straight from Miranda's browse dialog
static INT CALLBACK aim_options_browsefoldercallback(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
    char szDir[MAX_PATH];
    switch (uMsg) {
        case BFFM_INITIALIZED:
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
            break;
        case BFFM_SELCHANGED:
            if (SHGetPathFromIDList((LPITEMIDLIST) lp, szDir))
                SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM) szDir);
            break;
    }
    return 0;
}

int aim_opts_browsefolder(HWND hwnd, char *szPath)
{
    BROWSEINFO bi = { 0 };
    LPMALLOC pMalloc;
    ITEMIDLIST *pidlResult;
    int result = 0;

    if (SUCCEEDED(OleInitialize(NULL))) {
        if (SUCCEEDED(CoGetMalloc(1, &pMalloc))) {
            bi.hwndOwner = hwnd;
            bi.pszDisplayName = szPath;
            bi.lpszTitle = Translate("Select the Folder to Store AIM Group Chat Logs");
            bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_EDITBOX | BIF_RETURNONLYFSDIRS;       // Use this combo instead of BIF_USENEWUI
            bi.lpfn = aim_options_browsefoldercallback;
            bi.lParam = (LPARAM) szPath;

            pidlResult = SHBrowseForFolder(&bi);
            if (pidlResult) {
                SHGetPathFromIDList(pidlResult, szPath);
                lstrcat(szPath, "\\");
                result = 1;
            }
            pMalloc->lpVtbl->Free(pMalloc, pidlResult);
            pMalloc->lpVtbl->Release(pMalloc);
        }
        OleUninitialize();
    }
    return result;
}

static BOOL CALLBACK aim_options_gchatoptsproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HBRUSH hBkgColourBrush;

    switch (msg) {
        case WM_INITDIALOG:
        {
            char szPath[MAX_PATH];

            TranslateDialogDefault(hwndDlg);
            CheckDlgButton(hwndDlg, IDC_SENDENTER, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GE, AIM_KEY_GE_DEF));
            CheckDlgButton(hwndDlg, IDC_SENDCTRLE, !DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GE, AIM_KEY_GE_DEF));
            CheckDlgButton(hwndDlg, IDC_FLASHWINDOW, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GF, AIM_KEY_GF_DEF));
            CheckDlgButton(hwndDlg, IDC_TIMESTAMP, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GD, AIM_KEY_GD_DEF));
            CheckDlgButton(hwndDlg, IDC_DATE, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GY, AIM_KEY_GY_DEF));
            EnableWindow(GetDlgItem(hwndDlg, IDC_DATE), IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP));
            CheckDlgButton(hwndDlg, IDC_SHOWJOIN, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GJ, AIM_KEY_GJ_DEF));
            CheckDlgButton(hwndDlg, IDC_IGNOREJOIN, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GI, AIM_KEY_GI_DEF));
            CheckDlgButton(hwndDlg, IDC_SHOWJOINMENU, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GM, AIM_KEY_GM_DEF));
            CheckDlgButton(hwndDlg, IDC_MIN2TRAY, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GT, AIM_KEY_GT_DEF));
            CheckDlgButton(hwndDlg, IDC_LOGCHATS, DBGetContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GZ, AIM_KEY_GZ_DEF));
            aim_gchat_getlogdir(szPath, sizeof(szPath));
            SetDlgItemText(hwndDlg, IDC_LOGDIR, szPath);
            EnableWindow(GetDlgItem(hwndDlg, IDC_LOGDIR), IsDlgButtonChecked(hwndDlg, IDC_LOGCHATS));
            EnableWindow(GetDlgItem(hwndDlg, IDC_LOGDIROPEN), IsDlgButtonChecked(hwndDlg, IDC_LOGCHATS));
            SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, AIM_PROTO, AIM_KEY_GB, AIM_KEY_GB_DEF));
            SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETDEFAULTCOLOUR, 0, AIM_KEY_GB_DEF);
            hBkgColourBrush = CreateSolidBrush(SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR, 0, 0));
            {
                int i;
                LOGFONT lf;
                for (i = 0; i < sizeof(fontOptionsList) / sizeof(fontOptionsList[0]); i++) {
                    LoadGroupChatFont(i, &lf, &fontOptionsList[i].colour);
                    lstrcpy(fontOptionsList[i].szFace, lf.lfFaceName);
                    fontOptionsList[i].size = (char) lf.lfHeight;
                    fontOptionsList[i].style = (lf.lfWeight >= FW_BOLD ? FONTF_BOLD : 0) | (lf.lfItalic ? FONTF_ITALIC : 0);
                    fontOptionsList[i].charset = lf.lfCharSet;
                    SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_ADDSTRING, 0, i + 1);
                }
                SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_SETSEL, TRUE, 0);
                SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETCOLOUR, 0, fontOptionsList[0].colour);
                SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETDEFAULTCOLOUR, 0, fontOptionsList[0].defColour);
            }
            break;
        }
        case WM_CTLCOLORLISTBOX:
            SetBkColor((HDC) wParam, SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR, 0, 0));
            return (BOOL) hBkgColourBrush;
        case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *) lParam;
            HFONT hFont, hoFont;
            HDC hdc;
            SIZE fontSize;
            int iItem = mis->itemData - 1;
            hFont = CreateFont(fontOptionsList[iItem].size, 0, 0, 0,
                               fontOptionsList[iItem].style & FONTF_BOLD ? FW_BOLD : FW_NORMAL,
                               fontOptionsList[iItem].style & FONTF_ITALIC ? 1 : 0, 0, 0,
                               fontOptionsList[iItem].charset,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontOptionsList[iItem].szFace);
            hdc = GetDC(GetDlgItem(hwndDlg, mis->CtlID));
            hoFont = (HFONT) SelectObject(hdc, hFont);
            GetTextExtentPoint32(hdc, fontOptionsList[iItem].szDescr, lstrlen(fontOptionsList[iItem].szDescr), &fontSize);
            SelectObject(hdc, hoFont);
            ReleaseDC(GetDlgItem(hwndDlg, mis->CtlID), hdc);
            DeleteObject(hFont);
            mis->itemWidth = fontSize.cx;
            mis->itemHeight = fontSize.cy;
            return TRUE;
        }
        case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *) lParam;
            HFONT hFont, hoFont;
            char *pszText;
            int iItem = dis->itemData - 1;
            hFont = CreateFont(fontOptionsList[iItem].size, 0, 0, 0,
                               fontOptionsList[iItem].style & FONTF_BOLD ? FW_BOLD : FW_NORMAL,
                               fontOptionsList[iItem].style & FONTF_ITALIC ? 1 : 0, 0, 0,
                               fontOptionsList[iItem].charset,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontOptionsList[iItem].szFace);
            hoFont = (HFONT) SelectObject(dis->hDC, hFont);
            SetBkMode(dis->hDC, TRANSPARENT);
            FillRect(dis->hDC, &dis->rcItem, hBkgColourBrush);
            if (dis->itemState & ODS_SELECTED)
                FrameRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
            SetTextColor(dis->hDC, fontOptionsList[iItem].colour);
            pszText = Translate(fontOptionsList[iItem].szDescr);
            TextOut(dis->hDC, dis->rcItem.left, dis->rcItem.top, pszText, lstrlen(pszText));
            SelectObject(dis->hDC, hoFont);
            DeleteObject(hFont);
            return TRUE;
        }
        case WM_COMMAND:
        {
            if ((LOWORD(wParam) == IDC_LOGDIR)
                && (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
                return 0;
            switch (LOWORD(wParam)) {
                case IDC_LOGCHATS:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_LOGDIR), IsDlgButtonChecked(hwndDlg, IDC_LOGCHATS));
                    EnableWindow(GetDlgItem(hwndDlg, IDC_LOGDIROPEN), IsDlgButtonChecked(hwndDlg, IDC_LOGCHATS));
                    break;
                case IDC_LOGDIROPEN:
                {
                    char szPath[MAX_PATH];

                    GetDlgItemText(hwndDlg, IDC_LOGDIR, szPath, sizeof(szPath));
                    if (aim_opts_browsefolder(hwndDlg, szPath)) {
                        SetDlgItemText(hwndDlg, IDC_LOGDIR, szPath);
                        break;
                    }
                    return 0;
                }
                case IDC_TIMESTAMP:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_DATE), IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP));
                    break;
                case IDC_SHOWJOINMENU:
                    ShowWindow(GetDlgItem(hwndDlg, IDC_RELOADREQD), SW_SHOW);
                    break;
                case IDC_BKGCOLOUR:
                    DeleteObject(hBkgColourBrush);
                    hBkgColourBrush = CreateSolidBrush(SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR, 0, 0));
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_FONTLIST), NULL, TRUE);
                    break;
                case IDC_FONTLIST:
                    if (HIWORD(wParam) == LBN_SELCHANGE) {
                        if (SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELCOUNT, 0, 0) > 1) {
                            SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETCOLOUR, 0, GetSysColor(COLOR_3DFACE));
                            SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETDEFAULTCOLOUR, 0, GetSysColor(COLOR_WINDOWTEXT));
                        }
                        else {
                            int i = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA,
                                                       SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETCURSEL, 0, 0), 0) - 1;
                            SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETCOLOUR, 0, fontOptionsList[i].colour);
                            SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_SETDEFAULTCOLOUR, 0, fontOptionsList[i].defColour);
                        }
                    }
                    if (HIWORD(wParam) != LBN_DBLCLK)
                        return TRUE;
                    //fall through
                case IDC_CHOOSEFONT:
                {
                    CHOOSEFONT cf;
                    LOGFONT lf;
                    int i;

                    ZeroMemory(&cf, sizeof(cf));
                    ZeroMemory(&lf, sizeof(lf));
                    i = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETCURSEL, 0, 0),
                                           0) - 1;
                    lf.lfHeight = fontOptionsList[i].size;
                    lf.lfWeight = fontOptionsList[i].style & FONTF_BOLD ? FW_BOLD : FW_NORMAL;
                    lf.lfItalic = fontOptionsList[i].style & FONTF_ITALIC ? 1 : 0;
                    lf.lfCharSet = fontOptionsList[i].charset;
                    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
                    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
                    lf.lfQuality = DEFAULT_QUALITY;
                    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
                    lstrcpy(lf.lfFaceName, fontOptionsList[i].szFace);
                    cf.lStructSize = sizeof(cf);
                    cf.hwndOwner = hwndDlg;
                    cf.lpLogFont = &lf;
                    cf.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
                    if (ChooseFont(&cf)) {
                        int selItems[sizeof(fontOptionsList) / sizeof(fontOptionsList[0])];
                        int sel, selCount;

                        selCount =
                            SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELITEMS, sizeof(fontOptionsList) / sizeof(fontOptionsList[0]),
                                               (LPARAM) selItems);
                        for (sel = 0; sel < selCount; sel++) {
                            i = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[sel], 0) - 1;
                            fontOptionsList[i].size = (char) lf.lfHeight;
                            fontOptionsList[i].style = (lf.lfWeight >= FW_BOLD ? FONTF_BOLD : 0) | (lf.lfItalic ? FONTF_ITALIC : 0);
                            fontOptionsList[i].charset = lf.lfCharSet;
                            lstrcpy(fontOptionsList[i].szFace, lf.lfFaceName);
                            {
                                MEASUREITEMSTRUCT mis;

                                ZeroMemory(&mis, sizeof(mis));
                                mis.CtlID = IDC_FONTLIST;
                                mis.itemData = i + 1;
                                SendMessage(hwndDlg, WM_MEASUREITEM, 0, (LPARAM) & mis);
                                SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_SETITEMHEIGHT, selItems[sel], mis.itemHeight);
                            }
                        }
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_FONTLIST), NULL, TRUE);
                        break;
                    }
                    return TRUE;
                }
                case IDC_FONTCOLOUR:
                {
                    int selItems[sizeof(fontOptionsList) / sizeof(fontOptionsList[0])];
                    int sel, selCount, i;

                    selCount =
                        SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETSELITEMS, sizeof(fontOptionsList) / sizeof(fontOptionsList[0]),
                                           (LPARAM) selItems);
                    for (sel = 0; sel < selCount; sel++) {
                        i = SendDlgItemMessage(hwndDlg, IDC_FONTLIST, LB_GETITEMDATA, selItems[sel], 0) - 1;
                        fontOptionsList[i].colour = SendDlgItemMessage(hwndDlg, IDC_FONTCOLOUR, CPM_GETCOLOUR, 0, 0);
                    }
                    InvalidateRect(GetDlgItem(hwndDlg, IDC_FONTLIST), NULL, FALSE);
                    break;
                }
            }
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            break;
        }
        case WM_DESTROY:
            DeleteObject(hBkgColourBrush);
            break;
        case WM_NOTIFY:
        {
            switch (((LPNMHDR) lParam)->code) {
                case PSN_APPLY:
                {
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SENDENTER));
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GF, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_FLASHWINDOW));
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GD, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_TIMESTAMP));
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GY, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_DATE));
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GJ, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWJOIN));
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GI, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_IGNOREJOIN));
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GM, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWJOINMENU));
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GT, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_MIN2TRAY));
                    DBWriteContactSettingByte(NULL, AIM_PROTO, AIM_KEY_GZ, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_LOGCHATS));
                    DBWriteContactSettingDword(NULL, AIM_PROTO, AIM_KEY_GB, SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR, 0, 0));
                    {
                        char szPath[MAX_PATH];

                        GetDlgItemText(hwndDlg, IDC_LOGDIR, szPath, sizeof(szPath));
                        if (strlen(szPath)) {
                            int len = strlen(szPath);
                            if (szPath[len - 1] != '\\') {
                                lstrcat(szPath, "\\");
                            }
                            DBWriteContactSettingString(NULL, AIM_PROTO, AIM_KEY_GL, szPath);
                        }
                    }
                    {
                        int i;
                        char str[32];
                        for (i = 0; i < sizeof(fontOptionsList) / sizeof(fontOptionsList[0]); i++) {
                            mir_snprintf(str, sizeof(str), "GCFont%dFace", i);
                            DBWriteContactSettingString(NULL, AIM_PROTO, str, fontOptionsList[i].szFace);
                            mir_snprintf(str, sizeof(str), "GCFont%dSize", i);
                            DBWriteContactSettingByte(NULL, AIM_PROTO, str, fontOptionsList[i].size);
                            mir_snprintf(str, sizeof(str), "GCFont%dStyle", i);
                            DBWriteContactSettingByte(NULL, AIM_PROTO, str, fontOptionsList[i].style);
                            mir_snprintf(str, sizeof(str), "GCFont%dCharSet", i);
                            DBWriteContactSettingByte(NULL, AIM_PROTO, str, fontOptionsList[i].charset);
                            mir_snprintf(str, sizeof(str), "GCFont%dColor", i);
                            DBWriteContactSettingDword(NULL, AIM_PROTO, str, fontOptionsList[i].colour);
                        }
                    }
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}*/
