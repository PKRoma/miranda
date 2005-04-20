/*
  Name: NewEventNotify - Plugin for Miranda ICQ
  File: neweventnotify.h - Main Header File
  Version: 0.0.4
  Description: Notifies you when you receive a message
  Author: icebreaker, <icebreaker@newmail.net>
  Date: 18.07.02 13:59 / Update: 16.09.02 17:45
  Copyright: (C) 2002 Starzinger Michael

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

$Id$

Event popups for tabSRMM - most of the code taken from NewEventNotify (see copyright above)

  Code modified and adapted for tabSRMM by Nightwish (silvercircle@gmail.com)
  Additional code (popup merging, options) by Prezes
  
*/

#include "commonheaders.h"
#pragma hdrstop
#include "msgs.h"
#include "m_popup.h"
#include "nen.h"
#include "../../include/m_icq.h"

extern HINSTANCE g_hInst;
extern NEN_OPTIONS nen_options;
BOOL bWmNotify = FALSE;
extern HANDLE hMessageWindowList;
extern MYGLOBALS myGlobals;

PLUGIN_DATA *PopUpList[20];
static int PopupCount = 0;

extern BOOL CALLBACK DlgProcSetupStatusModes(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int PopupPreview(NEN_OPTIONS *pluginOptions);

int NEN_ReadOptions(void)
{
    NEN_OPTIONS *options = &nen_options;

    options->bPreview = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_PREVIEW, TRUE);
    options->bDefaultColorMsg = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_MESSAGE, FALSE);
    options->bDefaultColorUrl = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_URL, FALSE);
    options->bDefaultColorFile = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_FILE, FALSE);
    options->bDefaultColorOthers = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_OTHERS, FALSE);
    options->colBackMsg = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLBACK_MESSAGE, DEFAULT_COLBACK);
    options->colTextMsg = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLTEXT_MESSAGE, DEFAULT_COLTEXT);
    options->colBackUrl = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLBACK_URL, DEFAULT_COLBACK);
    options->colTextUrl = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLTEXT_URL, DEFAULT_COLTEXT);
    options->colBackFile = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLBACK_FILE, DEFAULT_COLBACK);
    options->colTextFile = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLTEXT_FILE, DEFAULT_COLTEXT);
    options->colBackOthers = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLBACK_OTHERS, DEFAULT_COLBACK);
    options->colTextOthers = (COLORREF)DBGetContactSettingDword(NULL, MODULE, OPT_COLTEXT_OTHERS, DEFAULT_COLTEXT);
    options->maskNotify = (UINT)DBGetContactSettingByte(NULL, MODULE, OPT_MASKNOTIFY, DEFAULT_MASKNOTIFY);
    options->maskActL = (UINT)DBGetContactSettingByte(NULL, MODULE, OPT_MASKACTL, DEFAULT_MASKACTL);
    options->maskActR = (UINT)DBGetContactSettingByte(NULL, MODULE, OPT_MASKACTR, DEFAULT_MASKACTR);
    options->maskActTE = (UINT)DBGetContactSettingByte(NULL, MODULE, OPT_MASKACTTE, DEFAULT_MASKACTR);
    options->bMergePopup = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_MERGEPOPUP, TRUE);
    options->iDelayMsg = (int)DBGetContactSettingDword(NULL, MODULE, OPT_DELAY_MESSAGE, DEFAULT_DELAY);
    options->iDelayUrl = (int)DBGetContactSettingDword(NULL, MODULE, OPT_DELAY_URL, DEFAULT_DELAY);
    options->iDelayFile = (int)DBGetContactSettingDword(NULL, MODULE, OPT_DELAY_FILE, DEFAULT_DELAY);
    options->iDelayOthers = (int)DBGetContactSettingDword(NULL, MODULE, OPT_DELAY_OTHERS, DEFAULT_DELAY);
    options->iDelayDefault = (int)DBGetContactSettingRangedWord(NULL, "PopUp", "Seconds",
                                                                SETTING_LIFETIME_DEFAULT, SETTING_LIFETIME_MIN, SETTING_LIFETIME_MAX);
    options->bShowDate = (BYTE)DBGetContactSettingByte(NULL, MODULE, OPT_SHOW_DATE, TRUE);
    options->bShowTime = (BYTE)DBGetContactSettingByte(NULL, MODULE, OPT_SHOW_TIME, TRUE);
    options->bShowHeaders = (BYTE)DBGetContactSettingByte(NULL, MODULE, OPT_SHOW_HEADERS, TRUE);
    options->iNumberMsg = (BYTE)DBGetContactSettingByte(NULL, MODULE, OPT_NUMBER_MSG, TRUE);
    options->bShowON = (BYTE)DBGetContactSettingByte(NULL, MODULE, OPT_SHOW_ON, TRUE);
    options->bNoRSS = (BOOL)DBGetContactSettingByte(NULL, MODULE, OPT_NORSS, FALSE);
    options->iDisable = (BYTE)DBGetContactSettingByte(NULL, MODULE, OPT_DISABLE, 0);
    options->dwStatusMask = (DWORD)DBGetContactSettingDword(NULL, MODULE, "statusmask", -1);
    return 0;
}

int NEN_WriteOptions(void)
{
    NEN_OPTIONS *options = &nen_options;

    DBWriteContactSettingByte(NULL, MODULE, OPT_PREVIEW, (BYTE)options->bPreview);
    DBWriteContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_MESSAGE, (BYTE)options->bDefaultColorMsg);
    DBWriteContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_URL, (BYTE)options->bDefaultColorUrl);
    DBWriteContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_FILE, (BYTE)options->bDefaultColorFile);
    DBWriteContactSettingByte(NULL, MODULE, OPT_COLDEFAULT_OTHERS, (BYTE)options->bDefaultColorOthers);
    DBWriteContactSettingDword(NULL, MODULE, OPT_COLBACK_MESSAGE, (DWORD)options->colBackMsg);
    DBWriteContactSettingDword(NULL, MODULE, OPT_COLTEXT_MESSAGE, (DWORD)options->colTextMsg);
    DBWriteContactSettingDword(NULL, MODULE, OPT_COLBACK_URL, (DWORD)options->colBackUrl);
    DBWriteContactSettingDword(NULL, MODULE, OPT_COLTEXT_URL, (DWORD)options->colTextUrl);
    DBWriteContactSettingDword(NULL, MODULE, OPT_COLBACK_FILE, (DWORD)options->colBackFile);
    DBWriteContactSettingDword(NULL, MODULE, OPT_COLTEXT_FILE, (DWORD)options->colTextFile);
    DBWriteContactSettingDword(NULL, MODULE, OPT_COLBACK_OTHERS, (DWORD)options->colBackOthers);
    DBWriteContactSettingDword(NULL, MODULE, OPT_COLTEXT_OTHERS, (DWORD)options->colTextOthers);
    DBWriteContactSettingByte(NULL, MODULE, OPT_MASKNOTIFY, (BYTE)options->maskNotify);
    DBWriteContactSettingByte(NULL, MODULE, OPT_MASKACTL, (BYTE)options->maskActL);
    DBWriteContactSettingByte(NULL, MODULE, OPT_MASKACTR, (BYTE)options->maskActR);
    DBWriteContactSettingByte(NULL, MODULE, OPT_MASKACTTE, (BYTE)options->maskActTE);
    DBWriteContactSettingByte(NULL, MODULE, OPT_MERGEPOPUP, (BYTE)options->bMergePopup);
    DBWriteContactSettingDword(NULL, MODULE, OPT_DELAY_MESSAGE, (DWORD)options->iDelayMsg);
    DBWriteContactSettingDword(NULL, MODULE, OPT_DELAY_URL, (DWORD)options->iDelayUrl);
    DBWriteContactSettingDword(NULL, MODULE, OPT_DELAY_FILE, (DWORD)options->iDelayFile);
    DBWriteContactSettingDword(NULL, MODULE, OPT_DELAY_OTHERS, (DWORD)options->iDelayOthers);
    DBWriteContactSettingByte(NULL, MODULE, OPT_SHOW_DATE, (BYTE)options->bShowDate);
    DBWriteContactSettingByte(NULL, MODULE, OPT_SHOW_TIME, (BYTE)options->bShowTime);
    DBWriteContactSettingByte(NULL, MODULE, OPT_SHOW_HEADERS, (BYTE)options->bShowHeaders);
    DBWriteContactSettingByte(NULL, MODULE, OPT_NUMBER_MSG, (BYTE)options->iNumberMsg);
    DBWriteContactSettingByte(NULL, MODULE, OPT_SHOW_ON, (BYTE)options->bShowON);
    DBWriteContactSettingByte(NULL, MODULE, OPT_NORSS, (BYTE)options->bNoRSS);
    DBWriteContactSettingByte(NULL, MODULE, OPT_DISABLE, (BYTE)options->iDisable);
    return 0;
}

BOOL CALLBACK DlgProcPopupOpts(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    NEN_OPTIONS *options = &nen_options;

    switch (msg) {
        case WM_INITDIALOG:
            TranslateDialogDefault(hWnd);
            bWmNotify = TRUE;
            SendDlgItemMessage(hWnd, IDC_COLBACK_MESSAGE, CPM_SETCOLOUR, 0, options->colBackMsg);
            SendDlgItemMessage(hWnd, IDC_COLTEXT_MESSAGE, CPM_SETCOLOUR, 0, options->colTextMsg);
            SendDlgItemMessage(hWnd, IDC_COLBACK_URL, CPM_SETCOLOUR, 0, options->colBackUrl);
            SendDlgItemMessage(hWnd, IDC_COLTEXT_URL, CPM_SETCOLOUR, 0, options->colTextUrl);
            SendDlgItemMessage(hWnd, IDC_COLBACK_FILE, CPM_SETCOLOUR, 0, options->colBackFile);
            SendDlgItemMessage(hWnd, IDC_COLTEXT_FILE, CPM_SETCOLOUR, 0, options->colTextFile);
            SendDlgItemMessage(hWnd, IDC_COLBACK_OTHERS, CPM_SETCOLOUR, 0, options->colBackOthers);
            SendDlgItemMessage(hWnd, IDC_COLTEXT_OTHERS, CPM_SETCOLOUR, 0, options->colTextOthers);
            CheckDlgButton(hWnd, IDC_CHKDEFAULTCOL_MESSAGE, options->bDefaultColorMsg?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hWnd, IDC_CHKDEFAULTCOL_URL, options->bDefaultColorUrl?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hWnd, IDC_CHKDEFAULTCOL_FILE, options->bDefaultColorFile?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hWnd, IDC_CHKDEFAULTCOL_OTHERS, options->bDefaultColorOthers?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hWnd, IDC_CHKMERGEPOPUP, options->bMergePopup?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hWnd, IDC_CHKNOTIFY_MESSAGE, options->maskNotify & MASK_MESSAGE);
            CheckDlgButton(hWnd, IDC_CHKNOTIFY_URL, options->maskNotify & MASK_URL);
            CheckDlgButton(hWnd, IDC_CHKNOTIFY_FILE, options->maskNotify & MASK_FILE);
            CheckDlgButton(hWnd, IDC_CHKNOTIFY_OTHER, options->maskNotify & MASK_OTHER);
            CheckDlgButton(hWnd, IDC_CHKACTL_DISMISS, options->maskActL & MASK_DISMISS);
            CheckDlgButton(hWnd, IDC_CHKACTL_OPEN, options->maskActL & MASK_OPEN);
            CheckDlgButton(hWnd, IDC_CHKACTL_REMOVE, options->maskActL & MASK_REMOVE);
            CheckDlgButton(hWnd, IDC_CHKACTR_DISMISS, options->maskActR & MASK_DISMISS);
            CheckDlgButton(hWnd, IDC_CHKACTR_OPEN, options->maskActR & MASK_OPEN);
            CheckDlgButton(hWnd, IDC_CHKACTR_REMOVE, options->maskActR & MASK_REMOVE);
            CheckDlgButton(hWnd, IDC_CHKACTTE_DISMISS, options->maskActTE & MASK_DISMISS);
            CheckDlgButton(hWnd, IDC_CHKACTTE_OPEN, options->maskActTE & MASK_OPEN);
            CheckDlgButton(hWnd, IDC_CHKACTTE_REMOVE, options->maskActTE & MASK_REMOVE);
            CheckDlgButton(hWnd, IDC_CHKSHOWDATE, options->bShowDate?BST_CHECKED:BST_UNCHECKED); 
            CheckDlgButton(hWnd, IDC_CHKSHOWTIME, options->bShowTime?BST_CHECKED:BST_UNCHECKED); 
            CheckDlgButton(hWnd, IDC_CHKSHOWHEADERS, options->bShowHeaders?BST_CHECKED:BST_UNCHECKED); 
            CheckDlgButton(hWnd, IDC_RDNEW, options->bShowON?BST_UNCHECKED:BST_CHECKED);
            CheckDlgButton(hWnd, IDC_RDOLD, options->bShowON?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hWnd, IDC_CHKINFINITE_MESSAGE, options->iDelayMsg == -1?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hWnd, IDC_CHKINFINITE_URL, options->iDelayUrl == -1?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hWnd, IDC_CHKINFINITE_FILE, options->iDelayFile == -1?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hWnd, IDC_CHKINFINITE_OTHERS, options->iDelayOthers == -1?BST_CHECKED:BST_UNCHECKED);
            SetDlgItemInt(hWnd, IDC_DELAY_MESSAGE, options->iDelayMsg != -1?options->iDelayMsg:0, TRUE);
            SetDlgItemInt(hWnd, IDC_DELAY_URL, options->iDelayUrl != -1?options->iDelayUrl:0, TRUE);
            SetDlgItemInt(hWnd, IDC_DELAY_FILE, options->iDelayFile != -1?options->iDelayFile:0, TRUE);
            SetDlgItemInt(hWnd, IDC_DELAY_OTHERS, options->iDelayOthers != -1?options->iDelayOthers:0, TRUE);
            SetDlgItemInt(hWnd, IDC_NUMBERMSG, options->iNumberMsg, FALSE);
            //disable color picker when using default colors
            EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_MESSAGE), !options->bDefaultColorMsg);
            EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_MESSAGE), !options->bDefaultColorMsg);
            EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_URL), !options->bDefaultColorUrl);
            EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_URL), !options->bDefaultColorUrl);
            EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_FILE), !options->bDefaultColorFile);
            EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_FILE), !options->bDefaultColorFile);
            EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_OTHERS), !options->bDefaultColorOthers);
            EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_OTHERS), !options->bDefaultColorOthers);
            //disable merge messages options when is not using
            EnableWindow(GetDlgItem(hWnd, IDC_CHKSHOWDATE), options->bMergePopup);
            EnableWindow(GetDlgItem(hWnd, IDC_CHKSHOWTIME), options->bMergePopup);
            EnableWindow(GetDlgItem(hWnd, IDC_CHKSHOWHEADERS), options->bMergePopup);
            EnableWindow(GetDlgItem(hWnd, IDC_CMDEDITHEADERS), options->bMergePopup && options->bShowHeaders);
            EnableWindow(GetDlgItem(hWnd, IDC_NUMBERMSG), options->bMergePopup);
            EnableWindow(GetDlgItem(hWnd, IDC_LBNUMBERMSG), options->bMergePopup);
            EnableWindow(GetDlgItem(hWnd, IDC_RDNEW), options->bMergePopup && options->iNumberMsg);
            EnableWindow(GetDlgItem(hWnd, IDC_RDOLD), options->bMergePopup && options->iNumberMsg);
            //disable delay textbox when infinite is checked
            EnableWindow(GetDlgItem(hWnd, IDC_DELAY_MESSAGE), options->iDelayMsg != -1);
            EnableWindow(GetDlgItem(hWnd, IDC_DELAY_URL), options->iDelayUrl != -1);
            EnableWindow(GetDlgItem(hWnd, IDC_DELAY_FILE), options->iDelayFile != -1);
            EnableWindow(GetDlgItem(hWnd, IDC_DELAY_OTHERS), options->iDelayOthers != -1);
            CheckDlgButton(hWnd, IDC_NORSS, options->bNoRSS);
            CheckDlgButton(hWnd, IDC_CHKPREVIEW, options->bPreview);
            CheckDlgButton(hWnd, IDC_DISABLEALLPOPUPS, options->iDisable);
            bWmNotify = FALSE;
            return TRUE;
        case DM_STATUSMASKSET:
            DBWriteContactSettingDword(0, MODULE, "statusmask", (DWORD)lParam);
            options->dwStatusMask = (int)lParam;
            break;
        case WM_COMMAND:
            if (!bWmNotify) {
                switch (LOWORD(wParam)) {
                    case IDC_PREVIEW:
                        PopupPreview(options);
                        break;
                    case IDC_POPUPSTATUSMODES:
                        {   
                            HWND hwndNew = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CHOOSESTATUSMODES), hWnd, DlgProcSetupStatusModes, DBGetContactSettingDword(0, MODULE, "statusmask", -1));
                            SendMessage(hwndNew, DM_SETPARENTDIALOG, 0, (LPARAM)hWnd);
                            break;
                        }
                    default:
                            //update options
                        options->maskNotify = (IsDlgButtonChecked(hWnd, IDC_CHKNOTIFY_MESSAGE)?MASK_MESSAGE:0) |
                                              (IsDlgButtonChecked(hWnd, IDC_CHKNOTIFY_URL)?MASK_URL:0) |
                                              (IsDlgButtonChecked(hWnd, IDC_CHKNOTIFY_FILE)?MASK_FILE:0) |
                                              (IsDlgButtonChecked(hWnd, IDC_CHKNOTIFY_OTHER)?MASK_OTHER:0);
                        options->maskActL = (IsDlgButtonChecked(hWnd, IDC_CHKACTL_DISMISS)?MASK_DISMISS:0) |
                                            (IsDlgButtonChecked(hWnd, IDC_CHKACTL_OPEN)?MASK_OPEN:0) |
                                            (IsDlgButtonChecked(hWnd, IDC_CHKACTL_REMOVE)?MASK_REMOVE:0);
                        options->maskActR = (IsDlgButtonChecked(hWnd, IDC_CHKACTR_DISMISS)?MASK_DISMISS:0) |
                                            (IsDlgButtonChecked(hWnd, IDC_CHKACTR_OPEN)?MASK_OPEN:0) |
                                            (IsDlgButtonChecked(hWnd, IDC_CHKACTR_REMOVE)?MASK_REMOVE:0);
                        options->maskActTE = (IsDlgButtonChecked(hWnd, IDC_CHKACTTE_DISMISS)?MASK_DISMISS:0) |
                                             (IsDlgButtonChecked(hWnd, IDC_CHKACTTE_OPEN)?MASK_OPEN:0) |
                                             (IsDlgButtonChecked(hWnd, IDC_CHKACTTE_REMOVE)?MASK_REMOVE:0);
                        options->bDefaultColorMsg = IsDlgButtonChecked(hWnd, IDC_CHKDEFAULTCOL_MESSAGE);
                        options->bDefaultColorUrl = IsDlgButtonChecked(hWnd, IDC_CHKDEFAULTCOL_URL);
                        options->bDefaultColorFile = IsDlgButtonChecked(hWnd, IDC_CHKDEFAULTCOL_FILE);
                        options->bDefaultColorOthers = IsDlgButtonChecked(hWnd, IDC_CHKDEFAULTCOL_OTHERS);
                        options->bPreview = IsDlgButtonChecked(hWnd, IDC_CHKPREVIEW);
                        options->iDelayMsg = IsDlgButtonChecked(hWnd, IDC_CHKINFINITE_MESSAGE)?-1:(DWORD)GetDlgItemInt(hWnd, IDC_DELAY_MESSAGE, NULL, FALSE);
                        options->iDelayUrl = IsDlgButtonChecked(hWnd, IDC_CHKINFINITE_URL)?-1:(DWORD)GetDlgItemInt(hWnd, IDC_DELAY_URL, NULL, FALSE);
                        options->iDelayFile = IsDlgButtonChecked(hWnd, IDC_CHKINFINITE_FILE)?-1:(DWORD)GetDlgItemInt(hWnd, IDC_DELAY_FILE, NULL, FALSE);
                        options->iDelayOthers = IsDlgButtonChecked(hWnd, IDC_CHKINFINITE_OTHERS)?-1:(DWORD)GetDlgItemInt(hWnd, IDC_DELAY_OTHERS, NULL, FALSE);
                        options->bMergePopup = IsDlgButtonChecked(hWnd, IDC_CHKMERGEPOPUP);
                        options->bShowDate = IsDlgButtonChecked(hWnd, IDC_CHKSHOWDATE);
                        options->bShowTime = IsDlgButtonChecked(hWnd, IDC_CHKSHOWTIME);
                        options->bShowHeaders = IsDlgButtonChecked(hWnd, IDC_CHKSHOWHEADERS);
                        options->bShowON = IsDlgButtonChecked(hWnd, IDC_RDOLD);
                        options->bShowON = !IsDlgButtonChecked(hWnd, IDC_RDNEW);
                        options->iNumberMsg = GetDlgItemInt(hWnd, IDC_NUMBERMSG, NULL, FALSE);
                        options->bNoRSS = IsDlgButtonChecked(hWnd, IDC_NORSS);
                        options->iDisable = IsDlgButtonChecked(hWnd, IDC_DISABLEALLPOPUPS);

                        EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_MESSAGE), !options->bDefaultColorMsg);
                        EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_MESSAGE), !options->bDefaultColorMsg);
                        EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_URL), !options->bDefaultColorUrl);
                        EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_URL), !options->bDefaultColorUrl);
                        EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_FILE), !options->bDefaultColorFile);
                        EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_FILE), !options->bDefaultColorFile);
                        EnableWindow(GetDlgItem(hWnd, IDC_COLBACK_OTHERS), !options->bDefaultColorOthers);
                        EnableWindow(GetDlgItem(hWnd, IDC_COLTEXT_OTHERS), !options->bDefaultColorOthers);
                            //disable merge messages options when is not using
                        EnableWindow(GetDlgItem(hWnd, IDC_CHKSHOWDATE), options->bMergePopup);
                        EnableWindow(GetDlgItem(hWnd, IDC_CHKSHOWTIME), options->bMergePopup);
                        EnableWindow(GetDlgItem(hWnd, IDC_CHKSHOWHEADERS), options->bMergePopup);
                        EnableWindow(GetDlgItem(hWnd, IDC_CMDEDITHEADERS), options->bMergePopup && options->bShowHeaders);
                        EnableWindow(GetDlgItem(hWnd, IDC_NUMBERMSG), options->bMergePopup);
                        EnableWindow(GetDlgItem(hWnd, IDC_LBNUMBERMSG), options->bMergePopup);
                        EnableWindow(GetDlgItem(hWnd, IDC_RDNEW), options->bMergePopup && options->iNumberMsg);
                        EnableWindow(GetDlgItem(hWnd, IDC_RDOLD), options->bMergePopup && options->iNumberMsg);
                            //disable delay textbox when infinite is checked
                        EnableWindow(GetDlgItem(hWnd, IDC_DELAY_MESSAGE), options->iDelayMsg != -1);
                        EnableWindow(GetDlgItem(hWnd, IDC_DELAY_URL), options->iDelayUrl != -1);
                        EnableWindow(GetDlgItem(hWnd, IDC_DELAY_FILE), options->iDelayFile != -1);
                        EnableWindow(GetDlgItem(hWnd, IDC_DELAY_OTHERS), options->iDelayOthers != -1);
                        if (HIWORD(wParam) == CPN_COLOURCHANGED) {
                            options->colBackMsg = SendDlgItemMessage(hWnd, IDC_COLBACK_MESSAGE, CPM_GETCOLOUR, 0, 0);
                            options->colTextMsg = SendDlgItemMessage(hWnd, IDC_COLTEXT_MESSAGE, CPM_GETCOLOUR, 0, 0);
                            options->colBackUrl = SendDlgItemMessage(hWnd, IDC_COLBACK_URL, CPM_GETCOLOUR, 0, 0);
                            options->colTextUrl = SendDlgItemMessage(hWnd, IDC_COLTEXT_URL, CPM_GETCOLOUR, 0, 0);
                            options->colBackFile = SendDlgItemMessage(hWnd, IDC_COLBACK_FILE, CPM_GETCOLOUR, 0, 0);
                            options->colTextFile = SendDlgItemMessage(hWnd, IDC_COLTEXT_FILE, CPM_GETCOLOUR, 0, 0);
                            options->colBackOthers = SendDlgItemMessage(hWnd, IDC_COLBACK_OTHERS, CPM_GETCOLOUR, 0, 0);
                            options->colTextOthers = SendDlgItemMessage(hWnd, IDC_COLTEXT_OTHERS, CPM_GETCOLOUR, 0, 0);
                        }
                        SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
                        break;
                }
            }
            break;
        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) {
                case PSN_APPLY:
                    NEN_WriteOptions();
                    break;
                case PSN_RESET:
                    NEN_ReadOptions();
                    break;
            }
            break;
        default:
            break;
    }
    return FALSE;
}

int NumberPopupData(HANDLE hContact)
{
    int n;

    for (n=0;n<20;n++) {
        if (!PopUpList[n] && !hContact)
            return n;

        if (PopUpList[n] && PopUpList[n]->hContact == hContact)
            return n;
    }
    return -1;
}

char* GetPreview(UINT eventType, char* pBlob)
{
    char* comment1 = NULL;
    char* comment2 = NULL;
    char* commentFix = NULL;

    //now get text
    switch (eventType) {
        case EVENTTYPE_MESSAGE:
            if (pBlob) comment1 = pBlob;
            commentFix = Translate(POPUP_COMMENT_MESSAGE);
            break;

        case EVENTTYPE_URL:
            if (pBlob) comment2 = pBlob;
            if (pBlob) comment1 = pBlob + strlen(comment2) + 1;
            commentFix = Translate(POPUP_COMMENT_URL);
            break;

        case EVENTTYPE_FILE:
            if (pBlob) comment2 = pBlob + 4;
            if (pBlob) comment1 = pBlob + strlen(comment2) + 5;
            commentFix = Translate(POPUP_COMMENT_FILE);
            break;

        case EVENTTYPE_CONTACTS:
            commentFix = Translate(POPUP_COMMENT_CONTACTS);
            break;
        case EVENTTYPE_ADDED:
            commentFix = Translate(POPUP_COMMENT_ADDED);
            break;
        case EVENTTYPE_AUTHREQUEST:
            commentFix = Translate(POPUP_COMMENT_AUTH);
            break;

//blob format is:
//ASCIIZ    text, usually "Sender IP: xxx.xxx.xxx.xxx\r\n%s"
//ASCIIZ    from name
//ASCIIZ    from e-mail
        case ICQEVENTTYPE_WEBPAGER:
            if (pBlob) comment1 = pBlob;
//			if (pBlob) comment1 = pBlob + strlen(comment2) + 1;
            commentFix = Translate(POPUP_COMMENT_WEBPAGER);
            break;
//blob format is:
//ASCIIZ    text, usually of the form "Subject: %s\r\n%s"
//ASCIIZ    from name
//ASCIIZ    from e-mail
        case ICQEVENTTYPE_EMAILEXPRESS:
            if (pBlob) comment1 = pBlob;
//			if (pBlob) comment1 = pBlob + strlen(comment2) + 1;
            commentFix = Translate(POPUP_COMMENT_EMAILEXP);
            break;

        default:
            commentFix = Translate(POPUP_COMMENT_OTHER);
            break;
    }

    if (comment1)
        if (strlen(comment1) > 0)
            return comment1;
    if (comment2)
        if (strlen(comment2) > 0)
            return comment2;

    return commentFix;
}

int PopupUpdate(HANDLE hContact, HANDLE hEvent)
{
    PLUGIN_DATA* pdata;
    DBEVENTINFO dbe;
    EVENT_DATA_EX* eventData;
    char lpzText[MAX_SECONDLINE] = "";
    char timestamp[MAX_DATASIZE] = "";
    char formatTime[MAX_DATASIZE] = "";
    int iEvent = 0;

    pdata = (PLUGIN_DATA*)PopUpList[NumberPopupData(hContact)];

    if (hEvent) {
        pdata->countEvent++;

        pdata->lastEventData->next = malloc(sizeof(EVENT_DATA_EX));
        pdata->lastEventData->next->prev = pdata->lastEventData;
        pdata->lastEventData = pdata->lastEventData->next;
        pdata->lastEventData->hEvent = hEvent;
        pdata->lastEventData->number = pdata->lastEventData->prev->number + 1;
        pdata->lastEventData->next = NULL;
        if ( pdata->pud && pdata->pud->iSeconds > 1 )
            pdata->pud->iSeconds = pdata->pluginOptions->iDelayMsg;
        //pdata->pud->iSeconds = pdata->pluginOptions->iDelayMsg;
        if (!pdata->pluginOptions->bShowON && pdata->countEvent > pdata->pluginOptions->iNumberMsg && pdata->pluginOptions->iNumberMsg)
            pdata->firstShowEventData = pdata->firstShowEventData->next;
    }

    if (pdata->pluginOptions->bShowHeaders)
        _snprintf(lpzText, sizeof(lpzText), "[b]%s %d[/b]\n", Translate("Number of new message: "), pdata->countEvent);

    if (pdata->firstShowEventData != pdata->firstEventData)
        _snprintf(lpzText, sizeof(lpzText), "%s...\n", lpzText);

    eventData = pdata->firstShowEventData;

    while (TRUE) {
        if (iEvent)
            eventData = eventData->next;
        iEvent++;
        //get DBEVENTINFO with pBlob if preview is needed (when is test then is off)
        dbe.pBlob = NULL;
        if (pdata->pluginOptions->bPreview && hContact) {
            dbe.cbSize = sizeof(dbe);
            dbe.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)eventData->hEvent, 0);
            dbe.pBlob = (PBYTE)malloc(dbe.cbBlob);
            CallService(MS_DB_EVENT_GET, (WPARAM)eventData->hEvent, (LPARAM)&dbe);
        }
        if (pdata->pluginOptions->bShowDate || pdata->pluginOptions->bShowTime) {
            strncpy(formatTime,"",sizeof(formatTime));
            if (pdata->pluginOptions->bShowDate)
                strncpy(formatTime, "%Y.%m.%d ", sizeof(formatTime));
            if (pdata->pluginOptions->bShowTime)
                strncat(formatTime, "%H:%M", sizeof(formatTime));
            strftime(timestamp,sizeof(timestamp), formatTime, localtime(&dbe.timestamp));
            _snprintf(lpzText, sizeof(lpzText), "%s[b][i]%s[/i][/b]\n", lpzText, timestamp);
        }
        _snprintf(lpzText, sizeof(lpzText), "%s%s", lpzText, GetPreview(dbe.eventType, dbe.pBlob));
        if (dbe.pBlob)
            free(dbe.pBlob);
        if ((iEvent >= pdata->pluginOptions->iNumberMsg && pdata->pluginOptions->iNumberMsg) || !eventData->next)
            break;
        _snprintf(lpzText, sizeof(lpzText), "%s\n", lpzText);
    }
    if (pdata->pluginOptions->iNumberMsg && pdata->countEvent > pdata->pluginOptions->iNumberMsg) {
        if (pdata->lastEventData != eventData)
            _snprintf(lpzText, sizeof(lpzText), "%s\n...", lpzText);
    }
    SendMessage(pdata->hWnd, WM_SETREDRAW, FALSE, 0);
    CallService(MS_POPUP_CHANGETEXT, (WPARAM)pdata->hWnd, (LPARAM)lpzText);
    SendMessage(pdata->hWnd, WM_SETREDRAW, TRUE, 0);
    return 0;
}

int PopupAct(HWND hWnd, UINT mask, PLUGIN_DATA* pdata)
{

    EVENT_DATA_EX* eventData;

    if (mask & MASK_OPEN) {
        HWND hwndExisting = 0;

        if((hwndExisting = WindowList_Find(hMessageWindowList, pdata->hContact)) != 0)
            PostMessage(hwndExisting, DM_ACTIVATEME, 0, 0);          // ask the message tab to activate itself (post it, may run in a different thread)
        else
            PostMessage(myGlobals.g_hwndHotkeyHandler, DM_HANDLECLISTEVENT, (WPARAM)pdata->hContact, 0);
    }

    if (mask & MASK_REMOVE) {

        eventData = pdata->lastEventData;
        while (eventData && eventData->prev) {
            CallService(MS_CLIST_REMOVEEVENT, (WPARAM)pdata->hContact, (LPARAM)eventData->hEvent);
            CallService(MS_DB_EVENT_MARKREAD, (WPARAM)pdata->hContact, (LPARAM)eventData->hEvent);
            eventData = eventData->prev;
            free(eventData->next);
            eventData->next = NULL;
        }
        free(eventData);
        pdata->lastEventData = pdata->firstEventData = pdata->firstShowEventData = NULL;
        PopUpList[NumberPopupData(pdata->hContact)] = NULL;
    }

    if (mask & MASK_DISMISS) {
        PopUpList[NumberPopupData(pdata->hContact)] = NULL;
        PUDeletePopUp(hWnd);
    }

    return 0;
}

static BOOL CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PLUGIN_DATA* pdata = NULL;

    pdata = (PLUGIN_DATA*)CallService(MS_POPUP_GETPLUGINDATA, (WPARAM)hWnd, (LPARAM)pdata);
    if (!pdata) return FALSE;

    switch (message) {
        case WM_COMMAND:
            PopupAct(hWnd, pdata->pluginOptions->maskActL, pdata);
            break;
        case WM_CONTEXTMENU:
            PopupAct(hWnd, pdata->pluginOptions->maskActR, pdata);
            break;
        case UM_FREEPLUGINDATA:
            PopupCount--;
            free(pdata);
            return TRUE;
        case UM_INITPOPUP:
            pdata->hWnd = hWnd;
            if (pdata->iSeconds != -1)
                SetTimer(hWnd, TIMER_TO_ACTION, pdata->iSeconds * 1000, NULL);
            break;
        case WM_MOUSEWHEEL:
            if ((short)HIWORD(wParam) > 0 && pdata->firstShowEventData->prev) {
                pdata->firstShowEventData = pdata->firstShowEventData->prev;
                PopupUpdate(pdata->hContact, NULL);
            }
            if ((short)HIWORD(wParam) < 0 && pdata->firstShowEventData->next && 
                pdata->countEvent - pdata->firstShowEventData->number >= pdata->pluginOptions->iNumberMsg) {
                pdata->firstShowEventData = pdata->firstShowEventData->next;
                PopupUpdate(pdata->hContact, NULL);
            }
            break;
        case WM_SETCURSOR:
            SetFocus(hWnd);
            break;
        case WM_TIMER:
            if (wParam != TIMER_TO_ACTION)
                break;
            if (pdata->iSeconds != -1)
                KillTimer(hWnd, TIMER_TO_ACTION);
            PopupAct(hWnd, pdata->pluginOptions->maskActTE, pdata);
            break;
        default:
            break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int PopupShow(NEN_OPTIONS *pluginOptions, HANDLE hContact, HANDLE hEvent, UINT eventType)
{
    POPUPDATAEX pud;
    PLUGIN_DATA* pdata;
    DBEVENTINFO dbe;
    EVENT_DATA_EX* eventData;
    char* sampleEvent;
    long iSeconds;

    //there has to be a maximum number of popups shown at the same time
    if (PopupCount >= MAX_POPUPS)
        return 2;

    //check if we should report this kind of event
    //get the prefered icon as well
    //CHANGE: iSeconds is -1 because I use my timer to hide popup
    switch (eventType) {
        case EVENTTYPE_MESSAGE:
            if (!(pluginOptions->maskNotify&MASK_MESSAGE)) return 1;
            pud.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
            pud.colorBack = pluginOptions->bDefaultColorMsg ? 0 : pluginOptions->colBackMsg;
            pud.colorText = pluginOptions->bDefaultColorMsg ? 0 : pluginOptions->colTextMsg;
            pud.iSeconds = -1; 
            iSeconds = pluginOptions->iDelayMsg;
            sampleEvent = Translate("This is a sample message event :-)");
            break;
        case EVENTTYPE_URL:
            if (!(pluginOptions->maskNotify&MASK_URL)) return 1;
            pud.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_URL);
            pud.colorBack = pluginOptions->bDefaultColorUrl ? 0 : pluginOptions->colBackUrl;
            pud.colorText = pluginOptions->bDefaultColorUrl ? 0 : pluginOptions->colTextUrl;
            pud.iSeconds = -1; 
            iSeconds = pluginOptions->iDelayUrl;
            sampleEvent = Translate("This is a sample URL event ;-)");
            break;
        case EVENTTYPE_FILE:
            if (!(pluginOptions->maskNotify&MASK_FILE)) return 1;
            pud.lchIcon = LoadSkinnedIcon(SKINICON_EVENT_FILE);
            pud.colorBack = pluginOptions->bDefaultColorFile ? 0 : pluginOptions->colBackFile;
            pud.colorText = pluginOptions->bDefaultColorFile ? 0 : pluginOptions->colTextFile;
            pud.iSeconds = -1;
            iSeconds = pluginOptions->iDelayFile;
            sampleEvent = Translate("This is a sample file event :-D");
            break;
        default:
            if (!(pluginOptions->maskNotify&MASK_OTHER)) return 1;
            pud.lchIcon = LoadSkinnedIcon(SKINICON_OTHER_MIRANDA);
            pud.colorBack = pluginOptions->bDefaultColorOthers ? 0 : pluginOptions->colBackOthers;
            pud.colorText = pluginOptions->bDefaultColorOthers ? 0 : pluginOptions->colTextOthers;
            pud.iSeconds = -1;
            iSeconds = pluginOptions->iDelayOthers;
            sampleEvent = Translate("This is a sample other event ;-D");
            break;
    }

    //get DBEVENTINFO with pBlob if preview is needed (when is test then is off)
    dbe.pBlob = NULL;

    if (pluginOptions->bPreview && hContact) {
        dbe.cbSize = sizeof(dbe);
        dbe.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hEvent, 0);
        dbe.pBlob = (PBYTE)malloc(dbe.cbBlob);
        CallService(MS_DB_EVENT_GET, (WPARAM)hEvent, (LPARAM)&dbe);
    }

    eventData = (EVENT_DATA_EX*)malloc(sizeof(EVENT_DATA_EX));
    eventData->hEvent = hEvent;
    eventData->number = 1;
    eventData->next = NULL;
    eventData->prev = NULL;

    //set plugin_data ... will be useable within PopupDlgProc
    pdata = (PLUGIN_DATA*)malloc(sizeof(PLUGIN_DATA));
    pdata->eventType = eventType;
    pdata->hContact = hContact;
    pdata->pluginOptions = pluginOptions;
    pdata->countEvent = 1;
    pdata->pud = &pud;
    pdata->iSeconds = iSeconds ? iSeconds : pluginOptions->iDelayDefault;
    pdata->firstEventData = pdata->firstShowEventData = pdata->lastEventData = eventData;

    //finally create the popup
    pud.lchContact = hContact;
    pud.PluginWindowProc = (WNDPROC)PopupDlgProc;
    pud.PluginData = pdata;

    //if hContact is NULL, then popup is only Test
    if (hContact) {
        //get the needed event data
        strncpy(pud.lpzContactName, (char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0), MAX_CONTACTNAME);
        strncpy(pud.lpzText, GetPreview(dbe.eventType, dbe.pBlob), MAX_SECONDLINE);
    } else {
        strncpy(pud.lpzContactName, "Plugin Test", MAX_CONTACTNAME);
        strncpy(pud.lpzText, sampleEvent, MAX_SECONDLINE);
    }

    PopupCount++;

    PopUpList[NumberPopupData(NULL)] = pdata;
    //send data to popup plugin

    CallService(MS_POPUP_ADDPOPUPEX, (WPARAM)&pud, 0);
    if (dbe.pBlob)
        free(dbe.pBlob);

    return 0;
}

int PopupPreview(NEN_OPTIONS *pluginOptions)
{
    PopupShow(pluginOptions, NULL, NULL, EVENTTYPE_MESSAGE);
    PopupShow(pluginOptions, NULL, NULL, EVENTTYPE_URL);
    PopupShow(pluginOptions, NULL, NULL, EVENTTYPE_FILE);
    PopupShow(pluginOptions, NULL, NULL, -1);

    return 0;
}

int tabSRMM_ShowPopup(WPARAM wParam, LPARAM lParam, WORD eventType, int windowOpen, struct ContainerWindowData *pContainer, HWND hwndChild, char *szProto)
{
    if(nen_options.iDisable)                          // no popups at all. Period
        return 0;
    
    /*
     * check the status mode against the status mask
     */
    _DebugPopup(0, "check proto: %s, mask: %d", szProto, nen_options.dwStatusMask);
    if(nen_options.dwStatusMask != -1) {
        DWORD dwStatus = 0;
        if(szProto != NULL) {
            dwStatus = (DWORD)CallProtoService(szProto, PS_GETSTATUS, 0, 0);
            if(!(dwStatus == 0 || dwStatus <= ID_STATUS_OFFLINE || ((1<<(dwStatus - ID_STATUS_ONLINE)) & nen_options.dwStatusMask)))              // should never happen, but...
                return 0;
        }
    }
    if(windowOpen && pContainer != 0) {                // message window is open, need to check the container config if we want to see a popup nonetheless
        if (pContainer->dwFlags & CNT_DONTREPORT && IsIconic(pContainer->hwnd))
                goto passed;
        if (pContainer->dwFlags & CNT_DONTREPORTUNFOCUSED) {
            if (!IsIconic(pContainer->hwnd) && GetForegroundWindow() != pContainer->hwnd && GetActiveWindow() != pContainer->hwnd)
                goto passed;
        }
        if (pContainer->dwFlags & CNT_ALWAYSREPORTINACTIVE) {
            if((GetForegroundWindow() == pContainer->hwnd || GetActiveWindow() == pContainer->hwnd) && pContainer->hwndActive == hwndChild)
                return 0;
            else {
                if(pContainer->hwndActive == hwndChild && pContainer->dwFlags & CNT_STICKY)
                    return 0;
                else
                    goto passed;
            }
        }
        return 0;
    }
passed:    
    if (NumberPopupData((HANDLE)wParam) != -1 && nen_options.bMergePopup && eventType == EVENTTYPE_MESSAGE) {
        PopupUpdate((HANDLE)wParam, (HANDLE)lParam);
    } else {
        //now finally show a plugin
        PopupShow(&nen_options, (HANDLE)wParam, (HANDLE)lParam, (UINT)eventType);
    }
    return 0;
}

