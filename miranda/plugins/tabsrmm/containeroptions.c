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
*/

#include "commonheaders.h"
#pragma hdrstop
#include "msgs.h"

int _log(const char *fmt, ...); // XXX debuglog
int EnumContainers(HANDLE hContact, DWORD dwAction, const TCHAR *szTarget, const TCHAR *szNew, DWORD dwExtinfo, DWORD dwExtinfoEx);
extern struct ContainerWindowData *pFirstContainer;

BOOL CALLBACK DlgProcContainerOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct ContainerWindowData *pContainer = 0;

    pContainer = (struct ContainerWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);

    switch (msg) {
        case WM_INITDIALOG:
            {
                TCHAR szNewTitle[128], szTemplate[30];
                //RECT rc, rcParent;
                struct ContainerWindowData *pContainer = 0;
				DWORD dwFlags = 0;

                SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) lParam);
                pContainer = (struct ContainerWindowData *) lParam;

                TranslateDialogDefault(hwndDlg);
#if defined(_UNICODE)
                MultiByteToWideChar(CP_ACP, 0, Translate("Set Options for: %s"), -1, szTemplate, 29);
#else
                strcpy(szTemplate, Translate("Set Options for: %s"));
#endif                
				_sntprintf(szNewTitle, 127, szTemplate, pContainer->szName);
				SetWindowText(hwndDlg, szNewTitle);
				
                //GetWindowRect(hwndDlg, &rc);
                //GetWindowRect(GetParent(hwndDlg), &rcParent);
                //SetWindowPos(hwndDlg, HWND_TOP, (rcParent.left + rcParent.right - (rc.right - rc.left)) / 2, (rcParent.top + rcParent.bottom - (rc.bottom - rc.top)) / 2, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
                ShowWindow(hwndDlg, SW_SHOWNORMAL);
				dwFlags = pContainer->dwFlags;
                SendMessage(hwndDlg, DM_SC_INITDIALOG, (WPARAM)dwFlags, (LPARAM)pContainer->dwTransparency);
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
				case IDOK:
				case IDC_APPLY:
                    {
						DWORD dwNewFlags = 0, dwNewTrans = 0;
                        SendMessage(hwndDlg, DM_SC_BUILDLIST, 0, (LPARAM)&dwNewFlags);

						pContainer->dwFlags = dwNewFlags;
						pContainer->dwTransparency = MAKELONG((WORD)SendDlgItemMessage(hwndDlg, IDC_TRANSPARENCY_ACTIVE, TBM_GETPOS, 0, 0), (WORD)SendDlgItemMessage(hwndDlg, IDC_TRANSPARENCY_INACTIVE, TBM_GETPOS, 0, 0));
						dwNewTrans = pContainer->dwTransparency;
                        
                        if (IsDlgButtonChecked(hwndDlg, IDC_APPLYASDEFAULT)) {
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "containerflags", dwNewFlags);
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "containertrans", pContainer->dwTransparency);
                        }

                        if (IsDlgButtonChecked(hwndDlg, IDC_APPLYEXISTING)) {
                            struct ContainerWindowData *pCurrent = pFirstContainer;
                            while(pCurrent) {
                                if(pCurrent != pContainer) {
                                    pCurrent->dwFlags = dwNewFlags;
                                    pCurrent->dwTransparency = pContainer->dwTransparency;
                                    SendMessage(pCurrent->hwnd, DM_CONFIGURECONTAINER, 0, 0);
                                }
                                pCurrent = pCurrent->pNextContainer;
                            }
                            EnumContainers(NULL, CNT_ENUM_WRITEFLAGS, NULL, NULL, dwNewFlags, dwNewTrans);
                        }

                        SendMessage(pContainer->hwnd, DM_CONFIGURECONTAINER, 0, 0);
                        
						if (LOWORD(wParam) == IDOK)
							DestroyWindow(hwndDlg);
                        else
                            EnableWindow(GetDlgItem(hwndDlg, IDC_APPLY), FALSE);
						break;
                    }
                case IDC_LOADDEFAULTS: {
                    DWORD dwFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "containerflags", CNT_FLAGS_DEFAULT);
                    DWORD dwTrans = DBGetContactSettingDword(NULL, SRMSGMOD_T, "containertrans", CNT_TRANS_DEFAULT);
                    SendMessage(hwndDlg, DM_SC_INITDIALOG, (WPARAM) dwFlags, (LPARAM) dwTrans);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_APPLY), TRUE);
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
            CheckDlgButton(hwndDlg, IDC_O_TITLEFRONT, dwFlags & CNT_TITLE_PREFIX);
            CheckDlgButton(hwndDlg, IDC_O_TITLESUFFIX, dwFlags & CNT_TITLE_SUFFIX);
            CheckDlgButton(hwndDlg, IDC_O_TITLENEVER, !(dwFlags & CNT_TITLE_SUFFIX || dwFlags & CNT_TITLE_PREFIX));
            CheckDlgButton(hwndDlg, IDC_SHOWSTATUS, dwFlags & CNT_TITLE_SHOWSTATUS);
            CheckDlgButton(hwndDlg, IDC_SHOWCONTACTNAME, dwFlags & CNT_TITLE_SHOWNAME);
            CheckDlgButton(hwndDlg, IDC_TRANSPARENCY, dwFlags & CNT_TRANSPARENCY);
            CheckDlgButton(hwndDlg, IDC_DONTREPORTUNFOCUSED2, dwFlags & CNT_DONTREPORTUNFOCUSED);
            CheckDlgButton(hwndDlg, IDC_ALWAYSPOPUPSINACTIVE, dwFlags & CNT_ALWAYSREPORTINACTIVE);
            CheckDlgButton(hwndDlg, IDC_SINGLEROWTAB, dwFlags & CNT_SINGLEROWTABCONTROL);
            CheckDlgButton(hwndDlg, IDC_SYNCSOUNDS, dwFlags & CNT_SYNCSOUNDS);
            CheckDlgButton(hwndDlg, IDC_CNTNOSTATUSBAR, dwFlags & CNT_NOSTATUSBAR);
            CheckDlgButton(hwndDlg, IDC_HIDEMENUBAR, dwFlags & CNT_NOMENUBAR);
            CheckDlgButton(hwndDlg, IDC_TABSATBOTTOM, dwFlags & CNT_TABSBOTTOM);
            CheckDlgButton(hwndDlg, IDC_STATICICON, dwFlags & CNT_STATICICON);
            CheckDlgButton(hwndDlg, IDC_SHOWUIN, dwFlags & CNT_TITLE_SHOWUIN);
            CheckDlgButton(hwndDlg, IDC_HIDETOOLBAR, dwFlags & CNT_HIDETOOLBAR);
            CheckDlgButton(hwndDlg, IDC_UINSTATUSBAR, dwFlags & CNT_UINSTATUSBAR);
            CheckDlgButton(hwndDlg, IDC_VERTICALMAX, dwFlags & CNT_VERTICALMAX);
            
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
                         (IsDlgButtonChecked(hwndDlg, IDC_O_TITLEFRONT) ? CNT_TITLE_PREFIX : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_O_TITLESUFFIX) ? CNT_TITLE_SUFFIX : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_SHOWSTATUS) ? CNT_TITLE_SHOWSTATUS : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_DONTREPORTUNFOCUSED2) ? CNT_DONTREPORTUNFOCUSED : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_ALWAYSPOPUPSINACTIVE) ? CNT_ALWAYSREPORTINACTIVE : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_SINGLEROWTAB) ? CNT_SINGLEROWTABCONTROL : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_SYNCSOUNDS) ? CNT_SYNCSOUNDS : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_CNTNOSTATUSBAR) ? CNT_NOSTATUSBAR : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_HIDEMENUBAR) ? CNT_NOMENUBAR : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_TABSATBOTTOM) ? CNT_TABSBOTTOM : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_STATICICON) ? CNT_STATICICON : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_SHOWUIN) ? CNT_TITLE_SHOWUIN : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_HIDETOOLBAR) ? CNT_HIDETOOLBAR : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_UINSTATUSBAR) ? CNT_UINSTATUSBAR : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_VERTICALMAX) ? CNT_VERTICALMAX : 0) |
                         (IsDlgButtonChecked(hwndDlg, IDC_SHOWCONTACTNAME) ? CNT_TITLE_SHOWNAME : 0);

            if (IsDlgButtonChecked(hwndDlg, IDC_O_FLASHDEFAULT))
                dwNewFlags &= ~(CNT_FLASHALWAYS | CNT_NOFLASH);

            *dwOut = dwNewFlags;
            break;
        }
    }
    return FALSE;
}

