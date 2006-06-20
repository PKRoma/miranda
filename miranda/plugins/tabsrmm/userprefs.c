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

$Id$

Dialog to setup per contact (user) prefs.
Invoked by the contact menu entry, handled by the SetUserPrefs Service
function.

Sets things like:

* global/local message log options
* local (per user) template overrides
* view mode (ieview/default)
* text formatting

*/

#include "commonheaders.h"
#pragma hdrstop

extern MYGLOBALS myGlobals;
extern HANDLE hMessageWindowList, hUserPrefsWindowList;
extern struct CPTABLE cpTable[];

static HWND hCpCombo;

static BOOL CALLBACK FillCpCombo(LPCTSTR str)
{
	int i;
	UINT cp;

    cp = _ttoi(str);
	for (i=0; cpTable[i].cpName != NULL && cpTable[i].cpId!=cp; i++);
	if (cpTable[i].cpName != NULL) {
        LRESULT iIndex = SendMessage(hCpCombo, CB_ADDSTRING, -1, (LPARAM) TranslateTS(cpTable[i].cpName));
        SendMessage(hCpCombo, CB_SETITEMDATA, (WPARAM)iIndex, cpTable[i].cpId);
    }
	return TRUE;
}

BOOL CALLBACK DlgProcUserPrefs(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HANDLE hContact = (HANDLE)GetWindowLong(hwndDlg, GWL_USERDATA);
    
    switch (msg) {
        case WM_INITDIALOG:
        {
            DBVARIANT dbv;
            TCHAR szBuffer[180];
            DWORD sCodePage, contact_gmt_diff;
            int i, offset;
			DWORD maxhist = DBGetContactSettingDword((HANDLE)lParam, SRMSGMOD_T, "maxhist", 0);
            BYTE bOverride = DBGetContactSettingByte((HANDLE)lParam, SRMSGMOD_T, "mwoverride", 0);
            BYTE bIEView = DBGetContactSettingByte((HANDLE)lParam, SRMSGMOD_T, "ieview", 0);
            int iLocalFormat = DBGetContactSettingDword((HANDLE)lParam, SRMSGMOD_T, "sendformat", 0);
            BYTE bRTL = DBGetContactSettingByte((HANDLE)lParam, SRMSGMOD_T, "RTL", 0);
            BYTE bLTR = DBGetContactSettingByte((HANDLE)lParam, SRMSGMOD_T, "RTL", 1);
            BYTE bSplit = DBGetContactSettingByte((HANDLE)lParam, SRMSGMOD_T, "splitoverride", 0);
			BYTE bInfoPanel = DBGetContactSettingByte((HANDLE)lParam, SRMSGMOD_T, "infopanel", 0);
            TCHAR contactName[100];
            char *szProto = (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)lParam, 0);

            hContact = (HANDLE)lParam;
            
            WindowList_Add(hUserPrefsWindowList, hwndDlg, hContact);
            TranslateDialogDefault(hwndDlg);
            SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)lParam);
            EnableWindow(GetDlgItem(hwndDlg, IDC_IEVIEWMODE), ServiceExists(MS_IEVIEW_WINDOW) ? TRUE : FALSE);

            SendDlgItemMessage(hwndDlg, IDC_INFOPANEL, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Use Global Setting"));
            SendDlgItemMessage(hwndDlg, IDC_INFOPANEL, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always On"));
            SendDlgItemMessage(hwndDlg, IDC_INFOPANEL, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always Off"));
            SendDlgItemMessage(hwndDlg, IDC_INFOPANEL, CB_SETCURSEL, bInfoPanel == 0 ? 0 : (bInfoPanel == 1 ? 1 : 2), 0);

			SendDlgItemMessage(hwndDlg, IDC_IEVIEWMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Use global Setting"));
            SendDlgItemMessage(hwndDlg, IDC_IEVIEWMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Force IEView"));
            SendDlgItemMessage(hwndDlg, IDC_IEVIEWMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Force Default Message Log"));
            SendDlgItemMessage(hwndDlg, IDC_IEVIEWMODE, CB_SETCURSEL, bIEView == 0 ? 0 : (bIEView == 1 ? 1 : 2), 0);
            
            SendDlgItemMessage(hwndDlg, IDC_TEXTFORMATTING, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Global Setting"));
            SendDlgItemMessage(hwndDlg, IDC_TEXTFORMATTING, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Simple Tags (*/_)"));
            SendDlgItemMessage(hwndDlg, IDC_TEXTFORMATTING, CB_INSERTSTRING, -1, (LPARAM)TranslateT("BBCode"));
            SendDlgItemMessage(hwndDlg, IDC_TEXTFORMATTING, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Force Off"));
            SendDlgItemMessage(hwndDlg, IDC_TEXTFORMATTING, CB_SETCURSEL, iLocalFormat == 0 ? 0 : (iLocalFormat == -1 ? 3 : (iLocalFormat == SENDFORMAT_BBCODE ? 1 : 2)), 0);
            
            SendDlgItemMessage(hwndDlg, IDC_BIDI, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Use Default"));
            SendDlgItemMessage(hwndDlg, IDC_BIDI, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always LTR"));
            SendDlgItemMessage(hwndDlg, IDC_BIDI, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always RTL"));
            SendDlgItemMessage(hwndDlg, IDC_BIDI, CB_SETCURSEL, (bLTR == 1 && bRTL == 0) ? 0 : (bLTR == 0 ? 1 : 2), 0);
            
            if(CheckMenuItem(myGlobals.g_hMenuFavorites, (UINT_PTR)lParam, MF_BYCOMMAND | MF_UNCHECKED) == -1)
                CheckDlgButton(hwndDlg, IDC_ISFAVORITE, FALSE);
            else
                CheckDlgButton(hwndDlg, IDC_ISFAVORITE, TRUE);

			CheckDlgButton(hwndDlg, IDC_LOGISGLOBAL, bOverride == 0);
            CheckDlgButton(hwndDlg, IDC_LOGISPRIVATE, bOverride != 0);
            CheckDlgButton(hwndDlg, IDC_PRIVATESPLITTER, bSplit);
            CheckDlgButton(hwndDlg, IDC_TEMPLOVERRIDE, DBGetContactSettingByte(hContact, TEMPLATES_MODULE, "enabled", 0));
            CheckDlgButton(hwndDlg, IDC_RTLTEMPLOVERRIDE, DBGetContactSettingByte(hContact, RTLTEMPLATES_MODULE, "enabled", 0));

            SendDlgItemMessage(hwndDlg, IDC_TRIMSPIN, UDM_SETRANGE, 0, MAKELONG(1000, 5));
            SendDlgItemMessage(hwndDlg, IDC_TRIMSPIN, UDM_SETPOS, 0, maxhist);
			EnableWindow(GetDlgItem(hwndDlg, IDC_TRIMSPIN), maxhist != 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_TRIM), maxhist != 0);
			CheckDlgButton(hwndDlg, IDC_ALWAYSTRIM2, maxhist != 0);

#if defined(_UNICODE)
            hCpCombo = GetDlgItem(hwndDlg, IDC_CODEPAGES);
            sCodePage = DBGetContactSettingDword(hContact, SRMSGMOD_T, "ANSIcodepage", 0);
            EnumSystemCodePages(FillCpCombo, CP_INSTALLED);
            SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_INSERTSTRING, 0, (LPARAM)TranslateT("Use default codepage"));
            if(sCodePage == 0)
                SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_SETCURSEL, (WPARAM)0, 0);
            else {
                for(i = 0; i < SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_GETCOUNT, 0, 0); i++) {
                    if(SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_GETITEMDATA, (WPARAM)i, 0) == sCodePage)
                        SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_SETCURSEL, (WPARAM)i, 0);
                }
            }
            MY_GetContactDisplayNameW(hContact, contactName, 84, szProto, sCodePage);
            _sntprintf(szBuffer, safe_sizeof(szBuffer), TranslateT("Set options for %s"), contactName);
            szBuffer[safe_sizeof(szBuffer) - 1] = 0;
            CheckDlgButton(hwndDlg, IDC_FORCEANSI, DBGetContactSettingByte(hContact, SRMSGMOD_T, "forceansi", 0) ? 1 : 0);
#else
            mir_snprintf(szBuffer, sizeof(szBuffer), Translate("Set options for %s"), (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0));
            EnableWindow(GetDlgItem(hwndDlg, IDC_CODEPAGES), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_FORCEANSI), FALSE);
#endif            
            CheckDlgButton(hwndDlg, IDC_IGNORETIMEOUTS, DBGetContactSettingByte(hContact, SRMSGMOD_T, "no_ack", 0));
            SetWindowText(hwndDlg, szBuffer);
            if(DBGetContactSettingByte(hContact, SRMSGMOD_T, "private_bg", 0)) {
                CheckDlgButton(hwndDlg, IDC_USEPRIVATEIMAGE, TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_GETBGIMAGE), TRUE);
            }
            else {
                CheckDlgButton(hwndDlg, IDC_USEPRIVATEIMAGE, FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_GETBGIMAGE), FALSE);
            }
            if(DBGetContactSetting(hContact, SRMSGMOD_T, "bgimage", &dbv) == 0) {
                SetDlgItemTextA(hwndDlg, IDC_BACKGROUNDIMAGE, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
            SendDlgItemMessage(hwndDlg, IDC_TIMEZONE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("<default, no change>"));
            timezone = (DWORD)DBGetContactSettingByte(hContact,"UserInfo","Timezone", DBGetContactSettingByte(hContact, (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0), "Timezone",-1));
            for(i = -12; i <= 12; i++) {
                _sntprintf(szBuffer, 20, TranslateT("GMT %c %d"), i < 0 ? '-' : '+', abs(i));
                SendDlgItemMessage(hwndDlg, IDC_TIMEZONE, CB_INSERTSTRING, -1, (LPARAM)szBuffer);
            }
            if(timezone != -1) {
                contact_gmt_diff = timezone > 128 ? 256 - timezone : 0 - timezone;
                offset = 13 + ((int)contact_gmt_diff / 2);
                SendDlgItemMessage(hwndDlg, IDC_TIMEZONE, CB_SETCURSEL, (WPARAM)offset, 0);
            }
            else
                SendDlgItemMessage(hwndDlg, IDC_TIMEZONE, CB_SETCURSEL, 0, 0);
            ShowWindow(hwndDlg, SW_SHOW);
            return TRUE;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDCANCEL:
                    DestroyWindow(hwndDlg);
                    break;
				case IDC_ALWAYSTRIM2:
					EnableWindow(GetDlgItem(hwndDlg, IDC_TRIMSPIN), IsDlgButtonChecked(hwndDlg, IDC_ALWAYSTRIM2));
					EnableWindow(GetDlgItem(hwndDlg, IDC_TRIM), IsDlgButtonChecked(hwndDlg, IDC_ALWAYSTRIM2));
					break;
                case IDC_USEPRIVATEIMAGE:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_GETBGIMAGE), IsDlgButtonChecked(hwndDlg, IDC_USEPRIVATEIMAGE));
                    break;
                case IDC_GETBGIMAGE:
                    {
                        char FileName[MAX_PATH];
                        char Filters[512];
                        OPENFILENAMEA ofn={0};

                        CallService(MS_UTILS_GETBITMAPFILTERSTRINGS,sizeof(Filters),(LPARAM)(char*)Filters);
                        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
                        ofn.hwndOwner=0;
                        ofn.lpstrFile = FileName;
                        ofn.lpstrFilter = Filters;
                        ofn.nMaxFile = MAX_PATH;
                        ofn.nMaxFileTitle = MAX_PATH;
                        ofn.Flags=OFN_HIDEREADONLY;
                        ofn.lpstrInitialDir = ".";
                        *FileName = '\0';
                        ofn.lpstrDefExt="";
                        if (GetOpenFileNameA(&ofn))
                            SetDlgItemTextA(hwndDlg, IDC_BACKGROUNDIMAGE, FileName);
                        break;
                    }
                case IDOK:
                {
                    struct MessageWindowData *dat = 0;
                    int iIndex = CB_ERR;
                    LRESULT newCodePage;
                    int offset;
                    HWND hWnd = WindowList_Find(hMessageWindowList, hContact);
                    DWORD sCodePage = DBGetContactSettingDword(hContact, SRMSGMOD_T, "ANSIcodepage", 0);
                    DWORD oldTZ = (DWORD)DBGetContactSettingByte(hContact,"UserInfo","Timezone", DBGetContactSettingByte(hContact, (char *)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0), "Timezone",-1));
                    BYTE bInfoPanel, bOldInfoPanel = DBGetContactSettingByte(hContact, SRMSGMOD_T, "infopanel", 0);

                    if(hWnd)
                        dat = (struct MessageWindowData *)GetWindowLong(hWnd, GWL_USERDATA);

                    if(ServiceExists(MS_IEVIEW_EVENT) && (iIndex = SendDlgItemMessage(hwndDlg, IDC_IEVIEWMODE, CB_GETCURSEL, 0, 0)) != CB_ERR) {
                        DBWriteContactSettingByte(hContact, SRMSGMOD_T, "ieview", iIndex == 2 ? -1 : iIndex);
                        if(hWnd && dat) {
                            SwitchMessageLog(hWnd, dat, GetIEViewMode(hWnd, dat));
                        }
                    }
                    if((iIndex = SendDlgItemMessage(hwndDlg, IDC_BIDI, CB_GETCURSEL, 0, 0)) != CB_ERR) {
                        if(iIndex == 0)
                            DBDeleteContactSetting(hContact, SRMSGMOD_T, "RTL");
                        else
                            DBWriteContactSettingByte(hContact, SRMSGMOD_T, "RTL", iIndex == 1 ? 0 : 1);
                    }
                    if(IsDlgButtonChecked(hwndDlg, IDC_LOGISGLOBAL)) {
                        DBWriteContactSettingByte(hContact, SRMSGMOD_T, "mwoverride", 0);
                        if(hWnd && dat) {
                            SendMessage(hWnd, DM_OPTIONSAPPLIED, 1, 0);
                        }
                    }
                    if(IsDlgButtonChecked(hwndDlg, IDC_LOGISPRIVATE)) {
                        DBWriteContactSettingByte(hContact, SRMSGMOD_T, "mwoverride", 1);
                        if(hWnd && dat) {
                            dat->dwFlags &= ~(MWF_LOG_ALL);
                            dat->dwFlags = DBGetContactSettingDword(hContact, SRMSGMOD_T, "mwflags", DBGetContactSettingDword(0, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT));
                            SendMessage(hWnd, DM_OPTIONSAPPLIED, 0, 0);
                        }
                    }
                    if((iIndex = SendDlgItemMessage(hwndDlg, IDC_TEXTFORMATTING, CB_GETCURSEL, 0, 0)) != CB_ERR) {
                        DBWriteContactSettingDword(hContact, SRMSGMOD_T, "sendformat", iIndex == 3 ? -1 : iIndex);
                        if(iIndex == 0)
                            DBDeleteContactSetting(hContact, SRMSGMOD_T, "sendformat");
                    }
#if defined(_UNICODE)
                    iIndex = SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_GETCURSEL, 0, 0);
                    if((newCodePage = SendDlgItemMessage(hwndDlg, IDC_CODEPAGES, CB_GETITEMDATA, (WPARAM)iIndex, 0)) != sCodePage) {
                        DBWriteContactSettingDword(hContact, SRMSGMOD_T, "ANSIcodepage", (DWORD)newCodePage);
                        if(hWnd && dat) {
                            dat->codePage = newCodePage;
                            dat->wOldStatus = 0;
                            SendMessage(hWnd, DM_UPDATETITLE, 0, 0);
                        }
                    }
                    if((IsDlgButtonChecked(hwndDlg, IDC_FORCEANSI) ? 1 : 0) != DBGetContactSettingByte(hContact, SRMSGMOD_T, "forceansi", 0)) {
                        DBWriteContactSettingByte(hContact, SRMSGMOD_T, "forceansi", IsDlgButtonChecked(hwndDlg, IDC_FORCEANSI) ? 1 : 0);
                        if(hWnd && dat)
                            dat->sendMode = IsDlgButtonChecked(hwndDlg, IDC_FORCEANSI) ? dat->sendMode | SMODE_FORCEANSI : dat->sendMode & ~SMODE_FORCEANSI;
                    }
#else
					newCodePage = CP_ACP;
#endif
                    if(IsDlgButtonChecked(hwndDlg, IDC_ISFAVORITE)) {
                        if(!DBGetContactSettingWord(hContact, SRMSGMOD_T, "isFavorite", 0))
                            AddContactToFavorites(hContact, NULL, NULL, NULL, 0, 0, 1, myGlobals.g_hMenuFavorites, newCodePage);
                    }
                    else
                        DeleteMenu(myGlobals.g_hMenuFavorites, (UINT_PTR)hContact, MF_BYCOMMAND);

                    DBWriteContactSettingWord(hContact, SRMSGMOD_T, "isFavorite", IsDlgButtonChecked(hwndDlg, IDC_ISFAVORITE) ? 1 : 0);
                    DBWriteContactSettingByte(hContact, SRMSGMOD_T, "splitoverride", IsDlgButtonChecked(hwndDlg, IDC_PRIVATESPLITTER) ? 1 : 0);

                    DBWriteContactSettingByte(hContact, TEMPLATES_MODULE, "enabled", IsDlgButtonChecked(hwndDlg, IDC_TEMPLOVERRIDE));
                    DBWriteContactSettingByte(hContact, RTLTEMPLATES_MODULE, "enabled", IsDlgButtonChecked(hwndDlg, IDC_RTLTEMPLOVERRIDE));

                    DBWriteContactSettingByte(hContact, SRMSGMOD_T, "private_bg", IsDlgButtonChecked(hwndDlg, IDC_USEPRIVATEIMAGE));
                    if(IsDlgButtonChecked(hwndDlg, IDC_USEPRIVATEIMAGE)) {
                        char szFilename[MAX_PATH];
                        GetDlgItemTextA(hwndDlg, IDC_BACKGROUNDIMAGE, szFilename, MAX_PATH - 1);
                        DBWriteContactSettingString(hContact, SRMSGMOD_T, "bgimage", szFilename);
                    }
                    offset = SendDlgItemMessage(hwndDlg, IDC_TIMEZONE, CB_GETCURSEL, 0, 0);
                    if(offset > 0) {
                        BYTE timezone = (13 - offset) * 2;
                        if(timezone != (BYTE)oldTZ)
                            DBWriteContactSettingByte(hContact, "UserInfo", "Timezone", (BYTE)timezone);
                    }
					else
						DBDeleteContactSetting(hContact, "UserInfo", "Timezone");

                    if(hWnd && dat) {
                        LoadTimeZone(hWnd, dat);
                        InvalidateRect(GetDlgItem(hWnd, IDC_PANELUIN), NULL, FALSE);
                    }

					bInfoPanel = (BYTE)SendDlgItemMessage(hwndDlg, IDC_INFOPANEL, CB_GETCURSEL, 0, 0);
					if(bInfoPanel != bOldInfoPanel) {
						DBWriteContactSettingByte(hContact, SRMSGMOD_T, "infopanel", bInfoPanel == 0 ? 0 : (bInfoPanel == 1 ? 1 : -1));
						if(hWnd && dat)
							SendMessage(hWnd, DM_SETINFOPANEL, 0, 0);
					}
					if(IsDlgButtonChecked(hwndDlg, IDC_ALWAYSTRIM2))
						DBWriteContactSettingDword(hContact, SRMSGMOD_T, "maxhist", (DWORD)SendDlgItemMessage(hwndDlg, IDC_TRIMSPIN, UDM_GETPOS, 0, 0));
					else
						DBWriteContactSettingDword(hContact, SRMSGMOD_T, "maxhist", 0);

					if(IsDlgButtonChecked(hwndDlg, IDC_IGNORETIMEOUTS)) {
						DBWriteContactSettingByte(hContact, SRMSGMOD_T, "no_ack", 1);
						if(hWnd && dat)
							dat->sendMode |= SMODE_NOACK;
					}
					else {
						DBDeleteContactSetting(hContact, SRMSGMOD_T, "no_ack");
						if(hWnd && dat)
							dat->sendMode &= ~SMODE_NOACK;
					}
					if(hWnd && dat)
                        SendMessage(hWnd, DM_CONFIGURETOOLBAR, 0, 1);
                    DestroyWindow(hwndDlg);
                    break;
                }
                default:
                    break;
            }
            break;
        case WM_DESTROY:
            WindowList_Remove(hUserPrefsWindowList, hwndDlg);
            break;
    }
    return FALSE;
}
