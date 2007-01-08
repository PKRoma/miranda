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

extern int g_shutDown;
extern struct ClcData *g_clcData;

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

			if(g_clcData) {
				if(FindItem(pcli->hwndContactTree, g_clcData, hContact, &contact, NULL, NULL)) {
					if(contact && contact->type != CLCIT_CONTACT) {
						DestroyWindow(hWnd);
						return FALSE;
					} else if(contact) {
						TCHAR szTitle[512];

						mir_sntprintf(szTitle, 512, TranslateT("Ignore options for %s"), contact->szText);
						SetWindowText(hWnd, szTitle);
						SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadSkinnedIcon(SKINICON_OTHER_MIRANDA));
						pCaps = CallProtoService(contact->proto, PS_GETCAPS, PFLAGNUM_1, 0);
						EnableWindow(GetDlgItem(hWnd, IDC_IGN_ALWAYSONLINE), pCaps & PF1_INVISLIST ? TRUE : FALSE);
						EnableWindow(GetDlgItem(hWnd, IDC_IGN_ALWAYSOFFLINE), pCaps & PF1_VISLIST ? TRUE : FALSE);
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
			DBWriteContactSettingByte(hContact, "CList", "Hidden", IsDlgButtonChecked(hWnd, IDC_HIDECONTACT) ? 1 : 0);
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
			  	SendMessage(hWnd, WM_USER + 110, 0, (LPARAM)&newMask);
			  	DBWriteContactSettingDword(hContact, "Ignore", "Mask1", newMask);
			  	SendMessage(hWnd, WM_USER + 130, 0, 0);
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
 * service function: Set or clear a contacts priority flag (this is a toggle service)
 * (clist_nicer+ specific service)
 * 
 * Servicename = CList/SetContactPriority (wParam = contacts handle)
 *
 * priority contacts appear on top of the current sorting order (the priority flag
 * overrides any other sorting, thus putting priority contacts at the top of the list
 * or group). If more than one contact per group have this flag set, they will be
 * sorted using the current contact list sorting rule(s).
*/

static int SetContactPriority(WPARAM wParam, LPARAM lParam)
{
	SendMessage(pcli->hwndContactTree, CLM_TOGGLEPRIORITYCONTACT, wParam, lParam);
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
	CreateServiceFunction("CList/SetContactPriority", SetContactPriority);
	CreateServiceFunction("CList/SetContactFloating", SetContactFloating);
	CreateServiceFunction("CList/SetContactIgnore", SetContactIgnore);
	return 0;
}

void UninitCustomMenus(void)
{
}
