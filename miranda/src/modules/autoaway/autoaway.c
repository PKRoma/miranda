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
#include "../../core/commonheaders.h"

#define AA_MODULE "AutoAway"
#define AA_USESHORT "IsShortEnabled"
#define AA_USELONG "IsLongEnabled"
#define AA_SHORTSTATUS "ShortStatus"
#define AA_LONGSTATUS "LongStatus"

static int aa_Status[] = {ID_STATUS_AWAY, ID_STATUS_DND, ID_STATUS_NA, ID_STATUS_OCCUPIED, ID_STATUS_INVISIBLE, ID_STATUS_ONTHEPHONE, ID_STATUS_OUTTOLUNCH};

static BOOL CALLBACK DlgProcAutoAwayOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG: 
		{	
			int j;
			TranslateDialogDefault(hwndDlg);			
			// check boxes
			CheckDlgButton(hwndDlg, IDC_AASHORTIDLE, DBGetContactSettingByte(NULL, AA_MODULE, AA_USESHORT, 0) ? BST_CHECKED:BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_AALONGIDLE, DBGetContactSettingByte(NULL, AA_MODULE, AA_USELONG, 0) ? BST_CHECKED:BST_UNCHECKED );
			// enable short idle?
			SendMessage(hwndDlg, WM_USER+1, 0, 0);
			// short idle flags			
			for ( j = 0 ; j<sizeof(aa_Status)/sizeof(aa_Status[0]) ; j++) {
				SendDlgItemMessage(hwndDlg, IDC_AASTATUS, CB_ADDSTRING, 0, (LPARAM)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)aa_Status[j], 0) );
			}
			// long idle flags
			for (j = 0; j<sizeof(aa_Status)/sizeof(aa_Status[0]) ; j++) {
				SendDlgItemMessage(hwndDlg, IDC_AALONGSTATUS, CB_ADDSTRING, 0, (LPARAM)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,(WPARAM)aa_Status[j], 0) );
			}
			// use current setting
			SendDlgItemMessage(hwndDlg, IDC_AASTATUS, CB_SETCURSEL, DBGetContactSettingWord(NULL, AA_MODULE, AA_SHORTSTATUS, 0), 0);
			SendDlgItemMessage(hwndDlg, IDC_AALONGSTATUS, CB_SETCURSEL, DBGetContactSettingWord(NULL, AA_MODULE, AA_LONGSTATUS, 0), 0);
			return TRUE;
		}
		case WM_USER+1:
		{
			EnableWindow(GetDlgItem(hwndDlg, IDC_AASTATUS), IsDlgButtonChecked(hwndDlg, IDC_AASHORTIDLE)==BST_CHECKED?1:0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_AALONGSTATUS), IsDlgButtonChecked(hwndDlg, IDC_AALONGIDLE)==BST_CHECKED?1:0);
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam)) {
				case IDC_AASHORTIDLE:
				case IDC_AALONGIDLE:
				{
					SendMessage(hwndDlg, WM_USER+1,0,0);
					break;
				}
				case IDC_AASTATUS:
				case IDC_AALONGSTATUS:
				{
					if ( HIWORD(wParam) != CBN_SELCHANGE ) return TRUE;
				} //case
			} //switch
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
		case WM_NOTIFY:
		{
			if ( lParam && ((LPNMHDR)lParam)->code == PSN_APPLY ) {
				DBWriteContactSettingByte(NULL, AA_MODULE, AA_USESHORT, IsDlgButtonChecked(hwndDlg,IDC_AASHORTIDLE)==BST_CHECKED?1:0);
				{
					int curSel = SendDlgItemMessage(hwndDlg, IDC_AASTATUS, CB_GETCURSEL, 0, 0);
					if ( curSel != CB_ERR ) {
						DBWriteContactSettingWord(NULL, AA_MODULE, AA_SHORTSTATUS, curSel);
					}
				}
				DBWriteContactSettingByte(NULL, AA_MODULE, AA_USELONG, IsDlgButtonChecked(hwndDlg,IDC_AALONGIDLE)==BST_CHECKED?1:0);
				{
					int curSel = SendDlgItemMessage(hwndDlg, IDC_AALONGSTATUS, CB_GETCURSEL, 0, 0);
					if ( curSel != CB_ERR ) {
						DBWriteContactSettingWord(NULL, AA_MODULE, AA_LONGSTATUS, curSel);
					}
				}
			} //if
		}
	}
	return FALSE;
}

static int AutoAwayOptInitialise(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;

	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=1000000000;
	odp.hInstance=GetModuleHandle(NULL);
	odp.pszTemplate=MAKEINTRESOURCE(IDD_OPT_AUTOAWAY);
	odp.pszTitle=Translate("Auto Away");
	odp.pszGroup=Translate("Status");
	odp.pfnDlgProc=DlgProcAutoAwayOpts;
	odp.flags=ODPF_BOLDGROUPS;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}

static int AutoAwayEvent(WPARAM wParam, LPARAM lParam)
{
	PROTOCOLDESCRIPTOR **proto=0;
	int protoCount=0;
	int j;
	// We might get idle-on/off and other status bits which we might not know
	// the other thing we do understand is short/long if anything comes in like force
	// the setting for 'short' will be used
	int newStatus=ID_STATUS_ONLINE;
	if ( lParam & IDF_SHORT || lParam & IDF_ONFORCE ) {

		if ( DBGetContactSettingByte(NULL,AA_MODULE,AA_USESHORT,0)==0 ) return 0; 
		// get the index of the status we should use
		newStatus = aa_Status[ DBGetContactSettingWord(NULL,AA_MODULE,AA_SHORTSTATUS,0) ];

	} else if ( lParam & IDF_LONG ) {

		if ( DBGetContactSettingByte(NULL,AA_MODULE,AA_USELONG,0)==0 ) return 0; 
		newStatus = aa_Status[ DBGetContactSettingWord(NULL,AA_MODULE,AA_LONGSTATUS,0) ];

	} else {
		// no idea what we should do here
		return 1;
	}
	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);
	for (j = 0 ; j < protoCount ; j++) {
		int caps=0;
		if ( proto[j]->type != PROTOTYPE_PROTOCOL ) continue;
		caps = CallProtoService(proto[j]->szName, PS_GETCAPS, PFLAGNUM_2, 0);
		{
			int newStatusFlag = Proto_Status2Flag(newStatus);
			int newStatusTemp = 0;
			// if it doesn't support online then don't bother
			if ( !(caps & PF2_ONLINE) ) continue;
			// does it support the user setting
			if ( !(caps & newStatusFlag) ) {
				// no, does it support basic away?
				if ( !(caps & PF2_SHORTAWAY) ) continue; 
				// yes, change to that instead
				newStatus = ID_STATUS_AWAY;
			}
			// if idle then use the new mode, otherwise online
			newStatusTemp = IDF_ISIDLE&lParam ? newStatus : ID_STATUS_ONLINE;
			// set it only if it isn't already set
			if ( CallProtoService(proto[j]->szName, PS_GETSTATUS, 0, 0) != newStatusTemp ) { 
				char * awayMsg=0;
				awayMsg = (char *) CallService(MS_AWAYMSG_GETSTATUSMSG, (WPARAM)newStatusTemp, 0);
				CallProtoService(proto[j]->szName, PS_SETSTATUS, newStatusTemp, 0);
				CallProtoService(proto[j]->szName, PS_SETAWAYMSG, newStatusTemp, (LPARAM) awayMsg);
			} //if
		}
	} //for
	return 0;
}

static int AutoAwayShutdown(WPARAM wParam,LPARAM lParam)
{
	return 0;
}

int LoadAutoAwayModule(void)
{
	HookEvent(ME_OPT_INITIALISE,AutoAwayOptInitialise);
	HookEvent(ME_SYSTEM_SHUTDOWN,AutoAwayShutdown);
	HookEvent(ME_IDLE_CHANGED, AutoAwayEvent);
	return 0;
}
