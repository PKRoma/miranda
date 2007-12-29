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
extern      BOOL g_skinnedContainers;
extern      BOOL g_framelessSkinmode;


static void MY_CheckDlgButton(HWND hWnd, UINT id, int iCheck)
{
    CheckDlgButton(hWnd, id, iCheck ? BST_CHECKED : BST_UNCHECKED);
}

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

#define NR_O_PAGES 7
#define NR_O_OPTIONSPERPAGE 8

static struct _tagPages {TCHAR *szTitle; TCHAR *szDesc; UINT uIds[8];} o_pages[] = {
    { _T("General options"), NULL, IDC_O_NOTABS, IDC_O_STICKY, IDC_VERTICALMAX, 0, 0, 0, 0, 0},
    { _T("Window layout"), NULL, IDC_CNTNOSTATUSBAR, IDC_HIDEMENUBAR, IDC_TABSATBOTTOM, IDC_UINSTATUSBAR, IDC_HIDETOOLBAR, IDC_INFOPANEL, 0, 0},
    { _T("Notifications"), _T("Select, in which cases you want to see event notifications for this message container. These options are disabled when you are using one of the \"simple\" notifications modes"), IDC_O_DONTREPORT, IDC_DONTREPORTUNFOCUSED2, IDC_ALWAYSPOPUPSINACTIVE, IDC_SYNCSOUNDS, 0, 0, 0, 0},
    { _T("Flashing"), NULL, IDC_O_FLASHDEFAULT, IDC_O_FLASHALWAYS, IDC_O_FLASHNEVER, 0, 0, 0, 0, 0},
    { _T("Title bar"), NULL, IDC_O_HIDETITLE, IDC_STATICICON, IDC_USEPRIVATETITLE, IDC_TITLEFORMAT, 0, 0, 0, 0},
    { _T("Window size and theme"), _T("You can select a private theme (.tabsrmm file) for this container which will then override the default message log theme."), IDC_THEME, IDC_SELECTTHEME, IDC_USEGLOBALSIZE, IDC_SAVESIZEASGLOBAL, IDC_LABEL_PRIVATETHEME, 0, 0, 0},
    { _T("Transparency"), _T("This feature requires Windows 2000 or later and is not available when custom container skins are in use"), IDC_TRANSPARENCY, IDC_TRANSPARENCY_ACTIVE, IDC_TRANSPARENCY_INACTIVE, IDC_TLABEL_ACTIVE, IDC_TLABEL_INACTIVE, IDC_TSLABEL_ACTIVE, IDC_TSLABEL_INACTIVE, 0},
    { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0}
};

static void ShowPage(HWND hwndDlg, int iPage, BOOL fShow)
{
    int i;

    if(iPage >= 0 && iPage < NR_O_PAGES) {
        for(i = 0; i < NR_O_OPTIONSPERPAGE && o_pages[iPage].uIds[i] != 0; i++)
            ShowWindow(GetDlgItem(hwndDlg, o_pages[iPage].uIds[i]), fShow ? SW_SHOW : SW_HIDE);
    }
    if(fShow) {
        SetDlgItemText(hwndDlg, IDC_TITLEBOX, TranslateTS(o_pages[iPage].szTitle));
        if(o_pages[iPage].szDesc)
            SetDlgItemText(hwndDlg, IDC_DESC, TranslateTS(o_pages[iPage].szDesc));
        else
            SetDlgItemText(hwndDlg, IDC_DESC, _T(""));
    }
}

BOOL CALLBACK DlgProcContainerOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct ContainerWindowData *pContainer = 0;
    HWND   hwndTree = GetDlgItem(hwndDlg, IDC_SECTIONTREE);
    pContainer = (struct ContainerWindowData *) GetWindowLong(hwndDlg, GWL_USERDATA);

    switch (msg) {
        case WM_INITDIALOG:
            {
                TCHAR szNewTitle[128];
                struct ContainerWindowData *pContainer = 0;
				DWORD dwFlags = 0;
                DWORD dwTemp[2];
                int   i, j;
                TVINSERTSTRUCT tvis = {0};
                LOGFONT lf;
                HFONT hFont = (HFONT)SendDlgItemMessage(hwndDlg, IDC_WHITERECT, WM_GETFONT, 0, 0);
                HTREEITEM hItem;

                GetObject(hFont, sizeof(lf), &lf);
                lf.lfHeight=(int)(lf.lfHeight*1.5);
                lf.lfWeight=FW_BOLD;
                hFont = CreateFontIndirect(&lf);
                SendDlgItemMessage(hwndDlg, IDC_WHITERECT, WM_SETFONT, (WPARAM)hFont, 0);

                SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) lParam);
                pContainer = (struct ContainerWindowData *) lParam;
                pContainer->hWndOptions = hwndDlg;
                TranslateDialogDefault(hwndDlg);
				mir_sntprintf(szNewTitle, SIZEOF(szNewTitle), _T("\t%s"), pContainer->szName);
				SetWindowText(hwndDlg, TranslateT("Container options"));

                EnableWindow(GetDlgItem(hwndDlg, IDC_O_HIDETITLE), g_framelessSkinmode ? FALSE : TRUE);
                //ShowWindow(hwndDlg, SW_SHOWNORMAL);
                CheckDlgButton(hwndDlg, IDC_CNTPRIVATE, !(pContainer->dwPrivateFlags & CNT_GLOBALSETTINGS));
                EnableWindow(GetDlgItem(hwndDlg, IDC_TITLEFORMAT), IsDlgButtonChecked(hwndDlg, IDC_USEPRIVATETITLE));

                dwTemp[0] = (pContainer->dwPrivateFlags & CNT_GLOBALSETTINGS ? myGlobals.m_GlobalContainerFlags : pContainer->dwPrivateFlags);
                dwTemp[1] = pContainer->dwTransparency;

                SendMessage(hwndDlg, DM_SC_INITDIALOG, (WPARAM)0, (LPARAM)dwTemp);
                CheckDlgButton(hwndDlg, IDC_USEPRIVATETITLE, pContainer->dwFlags & CNT_TITLE_PRIVATE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_TITLEFORMAT), IsDlgButtonChecked(hwndDlg, IDC_USEPRIVATETITLE));
                SendDlgItemMessage(hwndDlg, IDC_TITLEFORMAT, EM_LIMITTEXT, TITLE_FORMATLEN - 1, 0);
                SetDlgItemText(hwndDlg, IDC_TITLEFORMAT, pContainer->szTitleFormat);
                SetDlgItemTextA(hwndDlg, IDC_THEME, pContainer->szThemeFile);
                SetDlgItemText(hwndDlg, IDC_CURRENTNAME, TranslateT("\tConfigure container options"));
                SetDlgItemText(hwndDlg, IDC_WHITERECT, szNewTitle);
                SetWindowPos(GetDlgItem(hwndDlg, IDC_LOGO), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                for(i = 0; i < NR_O_PAGES; i++) {
					tvis.hParent = NULL;
					tvis.hInsertAfter = TVI_LAST;
					tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
					tvis.item.pszText = TranslateTS(o_pages[i].szTitle);
                    tvis.item.lParam = i;
					hItem = TreeView_InsertItem(hwndTree, &tvis);
                    if(i == 0)
                        SendMessage(hwndTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hItem);
                    for(j = 0; j < NR_O_OPTIONSPERPAGE && o_pages[i].uIds[j] != 0; j++)
                        ShowWindow(GetDlgItem(hwndDlg, o_pages[i].uIds[j]), SW_HIDE);
                }
                SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)myGlobals.g_iconContainer);
                ShowPage(hwndDlg, 0, TRUE);
                SetFocus(hwndTree);
                EnableWindow(GetDlgItem(hwndDlg, IDC_APPLY), FALSE);
				return FALSE;
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
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC:
            if((HWND)lParam==GetDlgItem(hwndDlg,IDC_WHITERECT)
                    || (HWND)lParam==GetDlgItem(hwndDlg,IDC_CURRENTNAME)
                    || (HWND)lParam==GetDlgItem(hwndDlg,IDC_LOGO)) {
                if((HWND)lParam==GetDlgItem(hwndDlg,IDC_WHITERECT))
                    SetTextColor((HDC)wParam,RGB(180,10,10));
                else if((HWND)lParam==GetDlgItem(hwndDlg,IDC_CURRENTNAME))
                    SetTextColor((HDC)wParam,RGB(70,70,70));
                else
                    SetTextColor((HDC)wParam, RGB(0,0,0));
                SetBkColor((HDC)wParam,RGB(255,255,255));
                return (BOOL)GetStockObject(WHITE_BRUSH);
            }
            break;
        case WM_NOTIFY:
            if(wParam == IDC_SECTIONTREE && ((LPNMHDR)lParam)->code == TVN_SELCHANGED) {
               NMTREEVIEW *pmtv = (NMTREEVIEW *)lParam;

               ShowPage(hwndDlg, pmtv->itemOld.lParam, 0);
               ShowPage(hwndDlg, pmtv->itemNew.lParam, 1);
            }
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_CNTPRIVATE:
                {
                    DWORD dwTemp[2];

                    if(IsDlgButtonChecked(hwndDlg, IDC_CNTPRIVATE)) {
                        dwTemp[0] = pContainer->dwPrivateFlags;
                        dwTemp[1] = pContainer->dwTransparency;
                        SendMessage(hwndDlg, DM_SC_INITDIALOG, 0, (LPARAM)dwTemp);
                    }
                    else {
                        dwTemp[0] = myGlobals.m_GlobalContainerFlags;;
                        dwTemp[1] = pContainer->dwTransparency;
                        SendMessage(hwndDlg, DM_SC_INITDIALOG, 0, (LPARAM)dwTemp);
                    }
                    goto do_apply;
                }
                case IDC_TRANSPARENCY:
                {
                    int isTrans = IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENCY);

                    EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCY_ACTIVE), isTrans ? TRUE : FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCY_INACTIVE), isTrans ? TRUE : FALSE);
                    goto do_apply;
                }
                case IDC_SECTIONTREE:
                case IDC_DESC:
                    return 0;
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
                    goto do_apply;
                case IDC_TITLEFORMAT:
                    if (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus())
                        return TRUE;
                    goto do_apply;
                case IDC_SELECTTHEME:
                    {
                        char *szFileName = GetThemeFileName(0);

                        if(PathFileExistsA(szFileName)) {
                            SetDlgItemTextA(hwndDlg, IDC_THEME, szFileName);
                            goto do_apply;
                        }
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
do_apply:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_APPLY), TRUE);
                    break;
            }
            break;
            case DM_SC_INITDIALOG: {
            DWORD *dwTemp = (DWORD *)lParam;

            DWORD dwFlags = dwTemp[0];
            DWORD dwTransparency = dwTemp[1];
            char szBuf[20];
            int  isTrans;

            MY_CheckDlgButton(hwndDlg, IDC_O_HIDETITLE, dwFlags & CNT_NOTITLE);
            MY_CheckDlgButton(hwndDlg, IDC_O_DONTREPORT, dwFlags & CNT_DONTREPORT);
            MY_CheckDlgButton(hwndDlg, IDC_O_NOTABS, dwFlags & CNT_HIDETABS);
            MY_CheckDlgButton(hwndDlg, IDC_O_STICKY, dwFlags & CNT_STICKY);
            MY_CheckDlgButton(hwndDlg, IDC_O_FLASHNEVER, dwFlags & CNT_NOFLASH);
            MY_CheckDlgButton(hwndDlg, IDC_O_FLASHALWAYS, dwFlags & CNT_FLASHALWAYS);
            MY_CheckDlgButton(hwndDlg, IDC_O_FLASHDEFAULT, !((dwFlags & CNT_NOFLASH) || (dwFlags & CNT_FLASHALWAYS)));
            MY_CheckDlgButton(hwndDlg, IDC_TRANSPARENCY, dwFlags & CNT_TRANSPARENCY);
            MY_CheckDlgButton(hwndDlg, IDC_DONTREPORTUNFOCUSED2, dwFlags & CNT_DONTREPORTUNFOCUSED);
            MY_CheckDlgButton(hwndDlg, IDC_ALWAYSPOPUPSINACTIVE, dwFlags & CNT_ALWAYSREPORTINACTIVE);
            MY_CheckDlgButton(hwndDlg, IDC_SYNCSOUNDS, dwFlags & CNT_SYNCSOUNDS);
            MY_CheckDlgButton(hwndDlg, IDC_CNTNOSTATUSBAR, dwFlags & CNT_NOSTATUSBAR);
            MY_CheckDlgButton(hwndDlg, IDC_HIDEMENUBAR, dwFlags & CNT_NOMENUBAR);
            MY_CheckDlgButton(hwndDlg, IDC_TABSATBOTTOM, dwFlags & CNT_TABSBOTTOM);
            MY_CheckDlgButton(hwndDlg, IDC_STATICICON, dwFlags & CNT_STATICICON);
            MY_CheckDlgButton(hwndDlg, IDC_HIDETOOLBAR, dwFlags & CNT_HIDETOOLBAR);
            MY_CheckDlgButton(hwndDlg, IDC_UINSTATUSBAR, dwFlags & CNT_UINSTATUSBAR);
            MY_CheckDlgButton(hwndDlg, IDC_VERTICALMAX, dwFlags & CNT_VERTICALMAX);
            MY_CheckDlgButton(hwndDlg, IDC_USEPRIVATETITLE, dwFlags & CNT_TITLE_PRIVATE);
            MY_CheckDlgButton(hwndDlg, IDC_INFOPANEL, dwFlags & CNT_INFOPANEL);
            MY_CheckDlgButton(hwndDlg, IDC_USEGLOBALSIZE, dwFlags & CNT_GLOBALSIZE);
            
            if (LOBYTE(LOWORD(GetVersion())) >= 5 && !g_skinnedContainers)
                CheckDlgButton(hwndDlg, IDC_TRANSPARENCY, dwFlags & CNT_TRANSPARENCY);
            else
                CheckDlgButton(hwndDlg, IDC_TRANSPARENCY, FALSE);

            EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCY), myGlobals.m_WinVerMajor >= 5 && !g_skinnedContainers ? TRUE : FALSE);

            isTrans = IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENCY);
            EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCY_ACTIVE), isTrans ? TRUE : FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCY_INACTIVE), isTrans ? TRUE : FALSE);

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
        {
            HFONT hFont;

            hFont=(HFONT)SendDlgItemMessage(hwndDlg, IDC_WHITERECT, WM_GETFONT,0,0);
            SendDlgItemMessage(hwndDlg, IDC_WHITERECT, WM_SETFONT, SendDlgItemMessage(hwndDlg, IDOK, WM_GETFONT,0,0),0);
            DeleteObject(hFont);				
            pContainer->hWndOptions = 0;
            SetWindowLong(hwndDlg, GWL_USERDATA, 0);
            break;
        }
    }
    return FALSE;
}

