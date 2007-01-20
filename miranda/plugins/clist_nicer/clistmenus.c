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
*/

#include "commonheaders.h"
#include "m_genmenu.h"
#include "m_ignore.h"
#include "CLUIFrames/cluiframes.h"

#pragma hdrstop

extern int      g_shutDown;
extern struct   ClcData *g_clcData;
extern int      g_nextExtraCacheEntry, g_maxExtraCacheEntry;
extern struct   ExtraCache *g_ExtraCache;
extern struct   CluiData g_CluiData;

int CloseAction(WPARAM wParam,LPARAM lParam)
{
	int k;
	g_shutDown = 1;
	k=CallService(MS_SYSTEM_OKTOEXIT,(WPARAM)0,(LPARAM)0);
	if (k) {
		SendMessage( pcli->hwndContactList, WM_DESTROY, 0, 0 );
		PostQuitMessage(0);
		Sleep(0);
	}

	return(0);
}

static HANDLE hWindowListIGN = 0;

/*                                                              
 * dialog procedure for handling the contact ignore dialog (available from the contact
 * menu
 */

static BOOL CALLBACK IgnoreDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact = (HANDLE)GetWindowLong(hWnd, GWL_USERDATA);

	switch(msg) {
	case WM_INITDIALOG:
		{
			DWORD dwMask;
			struct ClcContact *contact = NULL;
			int pCaps;
			HWND hwndAdd;

			hContact = (HANDLE)lParam;
			SetWindowLong(hWnd, GWL_USERDATA, (LONG)hContact);
			dwMask = DBGetContactSettingDword(hContact, "Ignore", "Mask1", 0);
			SendMessage(hWnd, WM_USER + 100, (WPARAM)hContact, dwMask);
			SendMessage(hWnd, WM_USER + 120, 0, 0);
			TranslateDialogDefault(hWnd);
			hwndAdd = GetDlgItem(hWnd, IDC_IGN_ADDPERMANENTLY); // CreateWindowEx(0, _T("CLCButtonClass"), _T("FOO"), WS_VISIBLE | BS_PUSHBUTTON | WS_CHILD | WS_TABSTOP, 200, 276, 106, 24, hWnd, (HMENU)IDC_IGN_ADDPERMANENTLY, g_hInst, NULL);
			SendMessage(hwndAdd, BUTTONSETASFLATBTN, 0, 1);
			SendMessage(hwndAdd, BUTTONSETASFLATBTN + 10, 0, 1);

			SendMessage(hwndAdd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(210), IMAGE_ICON, 16, 16, LR_SHARED));
			SetWindowText(hwndAdd, TranslateT("Add permanently"));
			EnableWindow(hwndAdd, DBGetContactSettingByte(hContact, "CList", "NotOnList", 0));

            SendDlgItemMessage(hWnd, IDC_AVATARDISPMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Default (global setting)"));
            SendDlgItemMessage(hWnd, IDC_AVATARDISPMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Show always when available"));
            SendDlgItemMessage(hWnd, IDC_AVATARDISPMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Hide always"));

            SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Default (global setting)"));
            SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Never"));
            SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always"));
            SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("When space is available"));
            SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("When needed by status message"));

			if(g_clcData) {
				if(FindItem(pcli->hwndContactTree, g_clcData, hContact, &contact, NULL, NULL)) {
					if(contact && contact->type != CLCIT_CONTACT) {
						DestroyWindow(hWnd);
						return FALSE;
					} else if(contact) {
						TCHAR szTitle[512];
                        DWORD dwFlags = DBGetContactSettingDword(hContact, "CList", "CLN_Flags", 0);
                        BYTE  bSecondLine = DBGetContactSettingByte(hContact, "CList", "CLN_2ndline", -1);

						mir_sntprintf(szTitle, 512, TranslateT("Contact list display and ignore options for %s"), contact->szText);
						SetWindowText(hWnd, szTitle);
						SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_OTHER_MIRANDA));
						pCaps = CallProtoService(contact->proto, PS_GETCAPS, PFLAGNUM_1, 0);
						EnableWindow(GetDlgItem(hWnd, IDC_IGN_ALWAYSONLINE), pCaps & PF1_INVISLIST ? TRUE : FALSE);
						EnableWindow(GetDlgItem(hWnd, IDC_IGN_ALWAYSOFFLINE), pCaps & PF1_VISLIST ? TRUE : FALSE);
                        CheckDlgButton(hWnd, IDC_IGN_PRIORITY, contact->flags & CONTACTF_PRIORITY ? TRUE : FALSE);
                        EnableWindow(GetDlgItem(hWnd, IDC_IGN_PRIORITY), TRUE);
                        EnableWindow(GetDlgItem(hWnd, IDC_AVATARDISPMODE), TRUE);
                        EnableWindow(GetDlgItem(hWnd, IDC_SECONDLINEMODE), TRUE);
                        if(dwFlags & ECF_FORCEAVATAR)
                            SendDlgItemMessage(hWnd, IDC_AVATARDISPMODE, CB_SETCURSEL, 1, 0);
                        else if(dwFlags & ECF_HIDEAVATAR)
                            SendDlgItemMessage(hWnd, IDC_AVATARDISPMODE, CB_SETCURSEL, 2, 0);
                        else
                            SendDlgItemMessage(hWnd, IDC_AVATARDISPMODE, CB_SETCURSEL, 0, 0);

                        if(bSecondLine == 0xff)
                            SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_SETCURSEL, 0, 0);
                        else
                            SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_SETCURSEL, (WPARAM)(bSecondLine + 1), 0);
					}
				} else {
					DestroyWindow(hWnd);
					return FALSE;
				}
			}
			WindowList_Add(hWindowListIGN, hWnd, hContact);
			ShowWindow(hWnd, SW_SHOWNORMAL);
			return TRUE;
		}
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
        case IDC_IGN_PRIORITY:
            SendMessage(pcli->hwndContactTree, CLM_TOGGLEPRIORITYCONTACT, (WPARAM)hContact, 0);
            return 0;
	  	case IDC_IGN_ALL:
			SendMessage(hWnd, WM_USER + 100, (WPARAM)hContact, (LPARAM)0xffffffff);
		  	return 0;
	  	case IDC_IGN_NONE:
			SendMessage(hWnd, WM_USER + 100, (WPARAM)hContact, (LPARAM)0);
		  	return 0;
	  	case IDC_IGN_ALWAYSONLINE:
			if(IsDlgButtonChecked(hWnd, IDC_IGN_ALWAYSONLINE))
				CheckDlgButton(hWnd, IDC_IGN_ALWAYSOFFLINE, FALSE);
		  	break;
	  	case IDC_IGN_ALWAYSOFFLINE:
			if(IsDlgButtonChecked(hWnd, IDC_IGN_ALWAYSOFFLINE))
				CheckDlgButton(hWnd, IDC_IGN_ALWAYSONLINE, FALSE);
		  	break;
	  	case IDC_HIDECONTACT:
			DBWriteContactSettingByte(hContact, "CList", "Hidden", (BYTE)(IsDlgButtonChecked(hWnd, IDC_HIDECONTACT) ? 1 : 0));
		  	break;
	  	case IDC_IGN_ADDPERMANENTLY:
			{
				ADDCONTACTSTRUCT acs = {0};

				acs.handle = hContact;
			  	acs.handleType = HANDLE_CONTACT;
			  	acs.szProto = 0;
			  	CallService(MS_ADDCONTACT_SHOW, (WPARAM)hWnd, (LPARAM)&acs);
			  	EnableWindow(GetDlgItem(hWnd, IDC_IGN_ADDPERMANENTLY), DBGetContactSettingByte(hContact, "CList", "NotOnList", 0));
			  	break;
		  	}
	  	case IDOK:
			{
				DWORD newMask = 0;
                struct ClcContact *contact = NULL;

			  	SendMessage(hWnd, WM_USER + 110, 0, (LPARAM)&newMask);
			  	DBWriteContactSettingDword(hContact, "Ignore", "Mask1", newMask);
			  	SendMessage(hWnd, WM_USER + 130, 0, 0);

                if(g_clcData) {
                    if(FindItem(pcli->hwndContactTree, g_clcData, hContact, &contact, NULL, NULL)) {
                        if(contact) {
                            LRESULT iSel = SendDlgItemMessage(hWnd, IDC_AVATARDISPMODE, CB_GETCURSEL, 0, 0);
                            DWORD dwFlags = DBGetContactSettingDword(hContact, "CList", "CLN_Flags", 0);

                            if(iSel != CB_ERR) {
                                dwFlags &= ~(ECF_FORCEAVATAR | ECF_HIDEAVATAR);

                                if(iSel == 1)
                                    dwFlags |= ECF_FORCEAVATAR;
                                else if(iSel == 2)
                                    dwFlags |= ECF_HIDEAVATAR;
                                DBWriteContactSettingDword(hContact, "CList", "CLN_Flags", dwFlags);
                                if(contact->extraCacheEntry >= 0 && contact->extraCacheEntry <= g_maxExtraCacheEntry)
                                    g_ExtraCache[contact->extraCacheEntry].dwDFlags = dwFlags;
                                LoadAvatarForContact(contact);
                            }

                            if((iSel = SendDlgItemMessage(hWnd, IDC_SECONDLINEMODE, CB_GETCURSEL, 0, 0)) != CB_ERR) {
                                if(iSel == 0) {
                                    DBDeleteContactSetting(hContact, "CList", "CLN_2ndline");
                                    contact->bSecondLine = g_CluiData.dualRowMode;
                                }
                                else {
                                    DBWriteContactSettingByte(hContact, "CList", "CLN_2ndline", (BYTE)(iSel - 1));
                                    contact->bSecondLine = (BYTE)(iSel - 1);
                                }
                            }
                            pcli->pfnClcBroadcast(CLM_AUTOREBUILD, 0, 0);
                        }
                    }
                }
		  	}
	  	case IDCANCEL:
			DestroyWindow(hWnd);
		  	break;
		}
		break;
	case WM_USER + 100:                                         // fill dialog (wParam = hContact, lParam = mask)
		{
			CheckDlgButton(hWnd, IDC_IGN_MSGEVENTS, lParam & (1 << (IGNOREEVENT_MESSAGE - 1)) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_IGN_FILEEVENTS, lParam & (1 << (IGNOREEVENT_FILE - 1)) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_IGN_URLEVENTS, lParam & (1 << (IGNOREEVENT_URL - 1)) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_IGN_AUTH, lParam & (1 << (IGNOREEVENT_AUTHORIZATION - 1)) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_IGN_ADD, lParam & (1 << (IGNOREEVENT_YOUWEREADDED - 1)) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hWnd, IDC_IGN_ONLINE, lParam & (1 << (IGNOREEVENT_USERONLINE - 1)) ? BST_CHECKED : BST_UNCHECKED);
			return 0;
		}
	case WM_USER + 110:                                         // retrieve value
		{
			DWORD *dwNewMask = (DWORD *)lParam, dwMask = 0;

			dwMask = (IsDlgButtonChecked(hWnd, IDC_IGN_MSGEVENTS) ? (1 << (IGNOREEVENT_MESSAGE - 1)) : 0) |
				(IsDlgButtonChecked(hWnd, IDC_IGN_FILEEVENTS) ? (1 << (IGNOREEVENT_FILE - 1)) : 0) |
				(IsDlgButtonChecked(hWnd, IDC_IGN_URLEVENTS) ? (1 << (IGNOREEVENT_URL - 1)) : 0) |
				(IsDlgButtonChecked(hWnd, IDC_IGN_AUTH) ? (1 << (IGNOREEVENT_AUTHORIZATION - 1)) : 0) |
				(IsDlgButtonChecked(hWnd, IDC_IGN_ADD) ? (1 << (IGNOREEVENT_YOUWEREADDED - 1)) : 0) |
				(IsDlgButtonChecked(hWnd, IDC_IGN_ONLINE) ? (1 << (IGNOREEVENT_USERONLINE - 1)) : 0);

			if(dwNewMask)
				*dwNewMask = dwMask;
			return 0;
		}
	case WM_USER + 120:                                         // set visibility status
		{
			struct ClcContact *contact = NULL;

			if(FindItem(pcli->hwndContactTree, g_clcData, hContact, &contact, NULL, NULL)) {
				if(contact) {
					WORD wApparentMode = DBGetContactSettingWord(contact->hContact, contact->proto, "ApparentMode", 0);

					CheckDlgButton(hWnd, IDC_IGN_ALWAYSOFFLINE, wApparentMode == ID_STATUS_OFFLINE ? TRUE : FALSE);
					CheckDlgButton(hWnd, IDC_IGN_ALWAYSONLINE, wApparentMode == ID_STATUS_ONLINE ? TRUE : FALSE);
				}
			}
			return 0;
		}
	case WM_USER + 130:                                         // update apparent mode
		{
			struct ClcContact *contact = NULL;

			if(FindItem(pcli->hwndContactTree, g_clcData, hContact, &contact, NULL, NULL)) {
				if(contact) {
					WORD wApparentMode = 0, oldApparentMode = DBGetContactSettingWord(hContact, contact->proto, "ApparentMode", 0);

					if(IsDlgButtonChecked(hWnd, IDC_IGN_ALWAYSONLINE))
						wApparentMode = ID_STATUS_ONLINE;
					else if(IsDlgButtonChecked(hWnd, IDC_IGN_ALWAYSOFFLINE))
						wApparentMode = ID_STATUS_OFFLINE;

					//DBWriteContactSettingWord(hContact, contact->proto, "ApparentMode", wApparentMode);
					//if(oldApparentMode != wApparentMode)
					CallContactService(hContact, PSS_SETAPPARENTMODE, (WPARAM)wApparentMode, 0);
					SendMessage(hWnd, WM_USER + 120, 0, 0);
				}
			}
			return 0;
		}
	case WM_DESTROY:
		SetWindowLong(hWnd, GWL_USERDATA, 0);
		WindowList_Remove(hWindowListIGN, hWnd);
		break;
	}
	return FALSE;
}

/*                                                              
 * service function: Open ignore settings dialog for the contact handle in wParam
 * (clist_nicer+ specific service)
 * 
 * Servicename = CList/SetContactIgnore
 *
 * ensure that dialog is only opened once (the dialog proc saves the window handle of an open dialog 
 * of this type to the contacts database record).
 *
 * if dialog is already open, focus it.
*/

static int SetContactIgnore(WPARAM wParam, LPARAM lParam)
{
	HANDLE hWnd = 0;

	if(hWindowListIGN == 0)
		hWindowListIGN = (HANDLE)CallService(MS_UTILS_ALLOCWINDOWLIST, 0, 0);

	hWnd = WindowList_Find(hWindowListIGN, (HANDLE)wParam);
	if ( wParam ) {
		if ( hWnd == 0 )
			CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_QUICKIGNORE), 0, IgnoreDialogProc, (LPARAM)wParam);
		else if ( IsWindow( hWnd ))
			SetFocus(hWnd);
	}
	return 0;
}

/*                                                              
 * service function: Set a contacts floating status.
 * (clist_nicer+ specific service)
 * 
 * Servicename = CList/SetContactFloating
 *
 * a floating contact appears as a small independent top level window anywhere on
 * the desktop.
*/

static int SetContactFloating(WPARAM wParam, LPARAM lParam)
{
	SendMessage(pcli->hwndContactTree, CLM_TOGGLEFLOATINGCONTACT, wParam, lParam);
	return 0;
}

int InitCustomMenus(void)
{
	CreateServiceFunction("CloseAction",CloseAction);
	CreateServiceFunction("CList/SetContactFloating", SetContactFloating);
	CreateServiceFunction("CList/SetContactIgnore", SetContactIgnore);
    {
        //FYR: Visibility and ignore item moved back to clist_nicer from core
        HANDLE hIgnoreItem = 0;  // FYR: moved from global it is never used globally
        CLISTMENUITEM mi = { 0 };
        mi.cbSize = sizeof( mi );

        if ( !hIgnoreItem ) {
            mi.position = 200000;
            mi.pszPopupName = ( char* )-1;
            mi.pszService = "CList/SetContactIgnore";
            mi.pszName = Translate("&Contact list settings...");
            hIgnoreItem = (HANDLE)CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM)&mi);
        }
    }
	return 0;
}

void UninitCustomMenus(void)
{

}
