/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

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

containeroptions.c  dialog implementaion for setting the container options.
                    part of tabSRMM
$Id$
*/

#include "commonheaders.h"
#pragma hdrstop

extern      struct ContainerWindowData *pFirstContainer;
extern      MYGLOBALS myGlobals;
extern      NEN_OPTIONS nen_options;

static void ReloadGlobalContainerSettings()
{
    struct ContainerWindowData *pC = pFirstContainer;
    while(pC) {
        if(pC->dwPrivateFlags & CNT_GLOBALSETTINGS) {
            DWORD dwOld = pC->dwFlags;
            pC->dwFlags = myGlobals.m_GlobalContainerFlags;
            SendMessage(pC->hwnd, DM_CONFIGURECONTAINER, 0, 0);
            if((dwOld & CNT_INFOPANEL) != (pC->dwFlags & CNT_INFOPANEL))
                BroadCastContainer(pC, DM_SETINFOPANEL, 0, 0);
        }
        pC = pC->pNextContainer;
    }
}

void ApplyContainerSetting(struct ContainerWindowData *pContainer, DWORD flags, int mode)
{
    if(pContainer->dwPrivateFlags & CNT_GLOBALSETTINGS) {
        myGlobals.m_GlobalContainerFlags = (mode ? myGlobals.m_GlobalContainerFlags | flags : myGlobals.m_GlobalContainerFlags & ~flags);
        pContainer->dwFlags = myGlobals.m_GlobalContainerFlags;
        DBWriteContactSettingDword(NULL, SRMSGMOD_T, "containerflags", myGlobals.m_GlobalContainerFlags);
        if(flags & CNT_INFOPANEL)
            BroadCastContainer(pContainer, DM_SETINFOPANEL, 0, 0);
        if(flags & CNT_SIDEBAR) {
            struct ContainerWindowData *pC = pFirstContainer;
            while(pC) {
                if(pC->dwPrivateFlags & CNT_GLOBALSETTINGS) {
                    pC->dwFlags = myGlobals.m_GlobalContainerFlags;
                    SendMessage(pC->hwnd, WM_COMMAND, IDC_TOGGLESIDEBAR, 0);
                }
                pC = pC->pNextContainer;
            }
        }
        else
            ReloadGlobalContainerSettings();
    } else {
        pContainer->dwPrivateFlags = (mode ? pContainer->dwPrivateFlags | flags : pContainer->dwPrivateFlags & ~flags);
        pContainer->dwFlags = pContainer->dwPrivateFlags;
        if(flags & CNT_SIDEBAR)
            SendMessage(pContainer->hwnd, WM_COMMAND, IDC_TOGGLESIDEBAR, 0);
        else
            SendMessage(pContainer->hwnd, DM_CONFIGURECONTAINER, 0, 0);
        if(flags & CNT_INFOPANEL)
            BroadCastContainer(pContainer, DM_SETINFOPANEL, 0, 0);
    }
}

BOOL CALLBACK DlgProcContainerOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct ContainerWindowData *pContainer = 0;

    pContainer = (struct ContainerWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);

    switch (msg) {
        case WM_INITDIALOG:
            {
                TCHAR szNewTitle[128];
                struct ContainerWindowData *pContainer = 0;
				DWORD dwFlags = 0;

                SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) lParam);
                pContainer = (struct ContainerWindowData *) lParam;
                pContainer->hWndOptions = hwndDlg;
                TranslateDialogDefault(hwndDlg);
				mir_sntprintf(szNewTitle, SIZEOF(szNewTitle), TranslateT("Set Options for: %s"), pContainer->szName);
				SetWindowText(hwndDlg, szNewTitle);
				
                ShowWindow(hwndDlg, SW_SHOWNORMAL);
                CheckDlgButton(hwndDlg, IDC_CNTPRIVATE, !(pContainer->dwPrivateFlags & CNT_GLOBALSETTINGS));
                EnableWindow(GetDlgItem(hwndDlg, IDC_TITLEFORMAT), IsDlgButtonChecked(hwndDlg, IDC_USEPRIVATETITLE));
                SendMessage(hwndDlg, DM_SC_INITDIALOG, (WPARAM)(pContainer->dwPrivateFlags & CNT_GLOBALSETTINGS ? myGlobals.m_GlobalContainerFlags : pContainer->dwPrivateFlags), (LPARAM)pContainer->dwTransparency);
                CheckDlgButton(hwndDlg, IDC_USEPRIVATETITLE, pContainer->dwFlags & CNT_TITLE_PRIVATE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_TITLEFORMAT), IsDlgButtonChecked(hwndDlg, IDC_USEPRIVATETITLE));
                SendDlgItemMessage(hwndDlg, IDC_TITLEFORMAT, EM_LIMITTEXT, TITLE_FORMATLEN - 1, 0);
                SetDlgItemText(hwndDlg, IDC_TITLEFORMAT, pContainer->szTitleFormat);
                SetDlgItemTextA(hwndDlg, IDC_THEME, pContainer->szThemeFile);
				return TRUE;
            }

		case WM_HSCROLL:
            if((HWND)lParam == GetDlgItem(hwndDlg, IDC_TRANSPARENCY_ACTIVE) || (HWND)lParam == GetDlgItem(hwndDlg, IDC_TRANSPARENCY_INACTIVE)) {
                char szBuf[20];
                DWORD dwPos = SendMessage((HWND) lParam, TBM_GETPOS, 0, 0);
                _snprintf(szBuf, 10, "%d", dwPos);
                if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_TRANSPARENCY_ACTIVE))
                    SetWindowTextA(GetDlgItem(hwndDlg, IDC_TLABEL_ACTIVE), szBuf);
                if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_TRANSPARENCY_INACTIVE))
                    SetWindowTextA(GetDlgItem(hwndDlg, IDC_TLABEL_INACTIVE), szBuf);
                EnableWindow(GetDlgItem(hwndDlg, IDC_APPLY), TRUE);
            }
			break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_CNTPRIVATE:
                    if(IsDlgButtonChecked(hwndDlg, IDC_CNTPRIVATE)) 
                        SendMessage(hwndDlg, DM_SC_INITDIALOG, pContainer->dwPrivateFlags, pContainer->dwTransparency);
                    else
                        SendMessage(hwndDlg, DM_SC_INITDIALOG, myGlobals.m_GlobalContainerFlags, pContainer->dwTransparency);
                    break;
                case IDC_SAVESIZEASGLOBAL:
                {
                    WINDOWPLACEMENT wp = {0};

                    wp.length = sizeof(wp);
                    if(GetWindowPlacement(pContainer->hwnd, &wp)) {
                        DBWriteContactSettingDword(NULL, SRMSGMOD_T, "splitx", wp.rcNormalPosition.left);
                        DBWriteContactSettingDword(NULL, SRMSGMOD_T, "splity", wp.rcNormalPosition.top);
                        DBWriteContactSettingDword(NULL, SRMSGMOD_T, "splitwidth", wp.rcNormalPosition.right - wp.rcNormalPosition.left);
                        DBWriteContactSettingDword(NULL, SRMSGMOD_T, "splitheight", wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
                    }
                    break;
                }
                case IDC_USEPRIVATETITLE:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_TITLEFORMAT), IsDlgButtonChecked(hwndDlg, IDC_USEPRIVATETITLE));
                    EnableWindow(GetDlgItem(hwndDlg, IDC_APPLY), TRUE);
                    break;
                case IDC_TITLEFORMAT:
                    if (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus())
                        return TRUE;
                    EnableWindow(GetDlgItem(hwndDlg, IDC_APPLY), TRUE);
                    break;
                case IDC_SELECTTHEME:
                    {
                        char *szFileName = GetThemeFileName(0);

                        if(PathFileExistsA(szFileName))
                            SetDlgItemTextA(hwndDlg, IDC_THEME, szFileName);
                        break;
                    }
				case IDOK:
				case IDC_APPLY:
                    {
						DWORD dwNewFlags = 0, dwNewTrans = 0;
                        
                        SendMessage(hwndDlg, DM_SC_BUILDLIST, 0, (LPARAM)&dwNewFlags);
                        dwNewFlags = (pContainer->dwFlags & CNT_SIDEBAR) | dwNewFlags;
                        
                        dwNewTrans = MAKELONG((WORD)SendDlgItemMessage(hwndDlg, IDC_TRANSPARENCY_ACTIVE, TBM_GETPOS, 0, 0), (WORD)SendDlgItemMessage(hwndDlg, IDC_TRANSPARENCY_INACTIVE, TBM_GETPOS, 0, 0));
                        
                        if(IsDlgButtonChecked(hwndDlg, IDC_CNTPRIVATE)) {
                            pContainer->dwPrivateFlags = pContainer->dwFlags = (dwNewFlags & ~CNT_GLOBALSETTINGS);
                        }
                        else {
                            myGlobals.m_GlobalContainerFlags = dwNewFlags;
                            pContainer->dwPrivateFlags |= CNT_GLOBALSETTINGS;
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "containerflags", dwNewFlags);
                            if(IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENCY)) {
                                myGlobals.m_GlobalContainerTrans = dwNewTrans;
                                DBWriteContactSettingDword(NULL, SRMSGMOD_T, "containertrans", dwNewTrans);
                            }
                        }
                        if(IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENCY))
                            pContainer->dwTransparency = dwNewTrans;
                        
                        if(dwNewFlags & CNT_TITLE_PRIVATE) {
                            GetDlgItemText(hwndDlg, IDC_TITLEFORMAT, pContainer->szTitleFormat, TITLE_FORMATLEN);
                            pContainer->szTitleFormat[TITLE_FORMATLEN - 1] = 0;
                        }
                        else
                            _tcsncpy(pContainer->szTitleFormat, myGlobals.szDefaultTitleFormat, TITLE_FORMATLEN);

                        pContainer->szThemeFile[0] = 0;
                        
                        if(GetWindowTextLengthA(GetDlgItem(hwndDlg, IDC_THEME)) > 0) {
                            char szFilename[MAX_PATH];

                            GetDlgItemTextA(hwndDlg, IDC_THEME, szFilename, MAX_PATH);
                            szFilename[MAX_PATH - 1] = 0;
                            if(PathFileExistsA(szFilename))
                                mir_snprintf(pContainer->szThemeFile, MAX_PATH, "%s", szFilename);
                        }
                            
                        if(!IsDlgButtonChecked(hwndDlg, IDC_CNTPRIVATE))
                            ReloadGlobalContainerSettings();
                        else {
                            SendMessage(pContainer->hwnd, DM_CONFIGURECONTAINER, 0, 0);
                            BroadCastContainer(pContainer, DM_SETINFOPANEL, 0, 0);
                        }
                            
						if (LOWORD(wParam) == IDOK)
							DestroyWindow(hwndDlg);
                        else
                            EnableWindow(GetDlgItem(hwndDlg, IDC_APPLY), FALSE);

						break;
                    }
                case IDCANCEL:
                    DestroyWindow(hwndDlg);
                    return TRUE;
                default:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_APPLY), TRUE);
                    break;
            }
            break;
        case DM_SC_INITDIALOG: {
            DWORD dwFlags = (DWORD) wParam;
            DWORD dwTransparency = (DWORD)lParam;
            char szBuf[20];
            
            CheckDlgButton(hwndDlg, IDC_O_HIDETITLE, dwFlags & CNT_NOTITLE);
            CheckDlgButton(hwndDlg, IDC_O_DONTREPORT, dwFlags & CNT_DONTREPORT);
            CheckDlgButton(hwndDlg, IDC_O_NOTABS, dwFlags & CNT_HIDETABS);
            CheckDlgButton(hwndDlg, IDC_O_STICKY, dwFlags & CNT_STICKY);
            CheckDlgButton(hwndDlg, IDC_O_FLASHNEVER, dwFlags & CNT_NOFLASH);
            CheckDlgButton(hwndDlg, IDC_O_FLASHALWAYS, dwFlags & CNT_FLASHALWAYS);
            CheckDlgButton(hwndDlg, IDC_O_FLASHDEFAULT, !((dwFlags & CNT_NOFLASH) || (dwFlags & CNT_FLASHALWAYS)));
            CheckDlgButton(hwndDlg, IDC_TRANSPARENCY, dwFlags & CNT_TRANSPARENCY);
            CheckDlgButton(hwndDlg, IDC_DONTREPORTUNFOCUSED2, dwFlags & CNT_DONTREPORTUNFOCUSED);
            CheckDlgButton(hwndDlg, IDC_ALWAYSPOPUPSINACTIVE, dwFlags & CNT_ALWAYSREPORTINACTIVE);
            CheckDlgButton(hwndDlg, IDC_SYNCSOUNDS, dwFlags & CNT_SYNCSOUNDS);
            CheckDlgButton(hwndDlg, IDC_CNTNOSTATUSBAR, dwFlags & CNT_NOSTATUSBAR);
            CheckDlgButton(hwndDlg, IDC_HIDEMENUBAR, dwFlags & CNT_NOMENUBAR);
            CheckDlgButton(hwndDlg, IDC_TABSATBOTTOM, dwFlags & CNT_TABSBOTTOM);
            CheckDlgButton(hwndDlg, IDC_STATICICON, dwFlags & CNT_STATICICON);
            CheckDlgButton(hwndDlg, IDC_HIDETOOLBAR, dwFlags & CNT_HIDETOOLBAR);
            CheckDlgButton(hwndDlg, IDC_UINSTATUSBAR, dwFlags & CNT_UINSTATUSBAR);
            CheckDlgButton(hwndDlg, IDC_VERTICALMAX, dwFlags & CNT_VERTICALMAX);
            CheckDlgButton(hwndDlg, IDC_USEPRIVATETITLE, dwFlags & CNT_TITLE_PRIVATE);
            CheckDlgButton(hwndDlg, IDC_INFOPANEL, dwFlags & CNT_INFOPANEL);
            CheckDlgButton(hwndDlg, IDC_USEGLOBALSIZE, dwFlags & CNT_GLOBALSIZE);
            
            if (LOBYTE(LOWORD(GetVersion())) >= 5 ) {
                CheckDlgButton(hwndDlg, IDC_TRANSPARENCY, dwFlags & CNT_TRANSPARENCY);
            } else {
                EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCY), 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCY_ACTIVE), 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCY_INACTIVE), 0);
            }
            _snprintf(szBuf, 10, "%d", LOWORD(dwTransparency));
            SetWindowTextA(GetDlgItem(hwndDlg, IDC_TLABEL_ACTIVE), szBuf);
            _snprintf(szBuf, 10, "%d", HIWORD(dwTransparency));
            SetWindowTextA(GetDlgItem(hwndDlg, IDC_TLABEL_INACTIVE), szBuf);

            SendDlgItemMessage(hwndDlg, IDC_TRANSPARENCY_ACTIVE, TBM_SETRANGE, 0, (LPARAM)MAKELONG(50, 255));
            SendDlgItemMessage(hwndDlg, IDC_TRANSPARENCY_INACTIVE, TBM_SETRANGE, 0, (LPARAM)MAKELONG(50, 255));

            SendDlgItemMessage(hwndDlg, IDC_TRANSPARENCY_ACTIVE, TBM_SETPOS, TRUE, (LPARAM) LOWORD(dwTransparency));
            SendDlgItemMessage(hwndDlg, IDC_TRANSPARENCY_INACTIVE, TBM_SETPOS, TRUE, (LPARAM) HIWORD(dwTransparency));

			EnableWindow(GetDlgItem(hwndDlg, IDC_O_DONTREPORT), nen_options.bSimpleMode == 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_SYNCSOUNDS), nen_options.bSimpleMode == 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_DONTREPORTUNFOCUSED2), nen_options.bSimpleMode == 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_ALWAYSPOPUPSINACTIVE), nen_options.bSimpleMode == 0);

            break;
        }
        case DM_SC_BUILDLIST: {
            DWORD dwNewFlags = 0, *dwOut = (DWORD *)lParam;
            
            dwNewFlags = (IsDlgButtonChecked(hwndDlg, IDC_O_HIDETITLE) ? CNT_NOTITLE : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_O_DONTREPORT) ? CNT_DONTREPORT : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_O_NOTABS) ? CNT_HIDETABS : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_O_STICKY) ? CNT_STICKY : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_O_FLASHALWAYS) ? CNT_FLASHALWAYS : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_O_FLASHNEVER) ? CNT_NOFLASH : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENCY) ? CNT_TRANSPARENCY : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_DONTREPORTUNFOCUSED2) ? CNT_DONTREPORTUNFOCUSED : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_ALWAYSPOPUPSINACTIVE) ? CNT_ALWAYSREPORTINACTIVE : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_SYNCSOUNDS) ? CNT_SYNCSOUNDS : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_CNTNOSTATUSBAR) ? CNT_NOSTATUSBAR : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_HIDEMENUBAR) ? CNT_NOMENUBAR : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_TABSATBOTTOM) ? CNT_TABSBOTTOM : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_STATICICON) ? CNT_STATICICON : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_HIDETOOLBAR) ? CNT_HIDETOOLBAR : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_UINSTATUSBAR) ? CNT_UINSTATUSBAR : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_USEPRIVATETITLE) ? CNT_TITLE_PRIVATE : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_USEGLOBALSIZE) ? CNT_GLOBALSIZE : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_INFOPANEL) ? CNT_INFOPANEL : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_VERTICALMAX) ? CNT_VERTICALMAX : 0) |
                         (CNT_NEWCONTAINERFLAGS);

            if (IsDlgButtonChecked(hwndDlg, IDC_O_FLASHDEFAULT))
                dwNewFlags &= ~(CNT_FLASHALWAYS | CNT_NOFLASH);

            *dwOut = dwNewFlags;
            break;
        }
        case WM_DESTROY:
            pContainer->hWndOptions = 0;
            SetWindowLong(hwndDlg, GWL_USERDATA, 0);
            break;
    }
    return FALSE;
}

