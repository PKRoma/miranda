/*
IRC plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson

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

#include "irc.h"

extern PREFERENCES *prefs;
extern HWND nick_hWnd;
extern HWND list_hWnd;
extern HWND whois_hWnd;
extern HWND join_hWnd;
extern HWND quickconn_hWnd;
extern char * IRCPROTONAME;
extern char * pszServerFile;
extern String sChannelModes;
static WNDPROC OldMgrEditProc;
extern HWND manager_hWnd;
extern HMODULE m_ssleay32;

// Callback for the 'CTCP accept' dialog
BOOL CALLBACK MessageboxWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static DCCINFO * pdci = NULL;
	switch (uMsg)
	{
		
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			pdci = (DCCINFO *) lParam;
			SendMessage(GetDlgItem(hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadImage(NULL,IDI_QUESTION,IMAGE_ICON,48, 48,LR_SHARED), 0);

		} break;
		
		case WM_COMMAND:
		{
			if(HIWORD(wParam) == BN_CLICKED)
			{
				switch (LOWORD(wParam))
				{
				case IDN_YES:
					{
					CDccSession * dcc = new CDccSession(pdci);

					CDccSession * olddcc = g_ircSession.FindDCCSession(pdci->hContact);
					if (olddcc)
						olddcc->Disconnect();
					g_ircSession.AddDCCSession(pdci->hContact, dcc);

					dcc->Connect();

					PostMessage (hwndDlg, WM_CLOSE, 0,0);
					}break;
				case IDN_NO:
					PostMessage (hwndDlg, WM_CLOSE, 0,0);
					break;

				default:break;
				}

			}
		} break;
		
		case WM_CLOSE:
		{
			DestroyWindow(hwndDlg);
		} break;
		
		default:break;
	}
	return(false);
}

// Callback for the /WHOIS dialog
BOOL CALLBACK InfoWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
		
		case WM_INITDIALOG:
		{

			TranslateDialogDefault(hwndDlg);
			HFONT hFont;
			LOGFONT lf;
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			GetObject(hFont,sizeof(lf),&lf);
			lf.lfHeight=(int)(lf.lfHeight*1.2);
			lf.lfWeight=FW_BOLD;
			hFont=CreateFontIndirect(&lf);
			SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem(hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadIconEx(IDI_LOGO), 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_INFO_NAME), EM_SETREADONLY, true, 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_INFO_ID), EM_SETREADONLY, true, 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_INFO_ADDRESS), EM_SETREADONLY, true, 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_INFO_CHANNELS), EM_SETREADONLY, true, 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_INFO_AUTH), EM_SETREADONLY, true, 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_INFO_SERVER), EM_SETREADONLY, true, 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_INFO_AWAY2), EM_SETREADONLY, true, 0);
			SendMessage(GetDlgItem(hwndDlg, IDC_INFO_OTHER), EM_SETREADONLY, true, 0);
			SendMessage(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)LoadIconEx(IDI_WHOIS)); // Tell the dialog to use it
					
		} break;

		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		{
			if((HWND)lParam==GetDlgItem(hwndDlg,IDC_WHITERECT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_TEXT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_CAPTION)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_LOGO))
			{
				SetTextColor((HDC)wParam,RGB(0,0,0));
				SetBkColor((HDC)wParam,RGB(255,255,255));
				return (BOOL)GetStockObject(WHITE_BRUSH);
			}
		}break;
		case WM_COMMAND:
		{
			if(HIWORD(wParam) == BN_CLICKED)
			{
				char szTemp[255];
				SendMessage(GetDlgItem(hwndDlg, IDC_INFO_NICK), WM_GETTEXT, 255, (LPARAM) szTemp);
				switch (LOWORD(wParam))
				{
				case ID_INFO_OK:
					PostMessage (hwndDlg, WM_CLOSE, 0,0);
					break;

				case ID_INFO_GO:
					PostIrcMessage( "/WHOIS %s", szTemp);
					break;

				case ID_INFO_QUERY:
					PostIrcMessage( "/QUERY %s", szTemp);
					break;
				case IDC_PING:
					SetDlgItemText(hwndDlg, IDC_REPLY, Translate("Please wait..."));
					PostIrcMessage( "/PRIVMSG %s \001PING %u\001", szTemp, time(0));
					break;
				case IDC_USERINFO:
					SetDlgItemText(hwndDlg, IDC_REPLY, Translate("Please wait..."));
					PostIrcMessage( "/PRIVMSG %s \001USERINFO\001", szTemp);
					break;
				case IDC_TIME:
					SetDlgItemText(hwndDlg, IDC_REPLY, Translate("Please wait..."));
					PostIrcMessage( "/PRIVMSG %s \001TIME\001", szTemp);
					break;
				case IDC_VERSION:
					SetDlgItemText(hwndDlg, IDC_REPLY, Translate("Please wait..."));
					PostIrcMessage( "/PRIVMSG %s \001VERSION\001", szTemp);
					break;
				default:break;
				}

			}

		} break;
		
		case WM_CLOSE:
		{
			ShowWindow(hwndDlg, SW_HIDE);
			SendMessage(hwndDlg, WM_SETREDRAW, FALSE, 0);
		} break;
		
		case WM_DESTROY:
		{
			HFONT hFont;
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDNOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				
			whois_hWnd = NULL;
		} break;


		default:break;
	}
	return(false);
}

// Callback for the 'Change nickname' dialog
BOOL CALLBACK NickWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
		
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			HFONT hFont;
			LOGFONT lf;
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			GetObject(hFont,sizeof(lf),&lf);
			lf.lfHeight=(int)(lf.lfHeight*1.2);
			lf.lfWeight=FW_BOLD;
			hFont=CreateFontIndirect(&lf);
			SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem(hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadIconEx(IDI_LOGO), 0);

			DBVARIANT dbv;
  			if (!DBGetContactSetting(NULL, IRCPROTONAME, "RecentNicks", &dbv) && dbv.type == DBVT_ASCIIZ) 
			{
				for (int i = 0; i<10; i++) {
					if (!GetWord(dbv.pszVal, i).empty())
						SendDlgItemMessage(hwndDlg, IDC_ENICK, CB_ADDSTRING, 0, (LPARAM)GetWord(dbv.pszVal, i).c_str());
				}
				DBFreeVariant(&dbv);
			}
		} break;
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		{
			if((HWND)lParam==GetDlgItem(hwndDlg,IDC_WHITERECT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_TEXT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_CAPTION)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_LOGO))
			{
				SetTextColor((HDC)wParam,RGB(0,0,0));
				SetBkColor((HDC)wParam,RGB(255,255,255));
				return (BOOL)GetStockObject(WHITE_BRUSH);
			}
		}break;
		
		case WM_COMMAND:
		{
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNCANCEL)
			{
				PostMessage (hwndDlg, WM_CLOSE, 0,0);
			}
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNOK)
			{
				char szTemp[255];
				SendMessage(GetDlgItem(hwndDlg, IDC_ENICK), WM_GETTEXT, 255, (LPARAM) szTemp);
				PostIrcMessage( "/NICK %s", szTemp);

				String S = szTemp; 
				DBVARIANT dbv;
	  			if (!DBGetContactSetting(NULL, IRCPROTONAME, "RecentNicks", &dbv) && dbv.type == DBVT_ASCIIZ) 
				{
					for (int i = 0; i<10; i++) {
						if (!GetWord(dbv.pszVal, i).empty() && GetWord(dbv.pszVal, i) != szTemp) {
							S += " ";
							S += GetWord(dbv.pszVal, i);
						}
					}
					DBFreeVariant(&dbv);
				}
				DBWriteContactSettingString(NULL, IRCPROTONAME, "RecentNicks", S.c_str());
				SendMessage (hwndDlg, WM_CLOSE, 0,0);

			}

		} break;
		
		case WM_CLOSE:
		{
			DestroyWindow(hwndDlg);
		} break;
		
		case WM_DESTROY:
		{
			HFONT hFont;
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDNOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				
			nick_hWnd = NULL;
		} break;


		default:break;
	}
	return(false);
}

BOOL CALLBACK ListWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
		case IRC_UPDATELIST:
		{
			int j = ListView_GetItemCount(GetDlgItem(hwndDlg, IDC_INFO_LISTVIEW));
			if (j >0) {
				LVITEM lvm;
				lvm.mask= LVIF_PARAM;
				lvm.iSubItem = 0;
				for( int i =0; i < j; i++)
				{
					lvm.iItem = i;
					lvm.lParam = i;
					ListView_SetItem(GetDlgItem(hwndDlg, IDC_INFO_LISTVIEW),&lvm);
				}	
			}
		} break;
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			RECT screen;

			SystemParametersInfo(SPI_GETWORKAREA, 0,  &screen, 0); 
			LV_COLUMN lvC;
			int COLUMNS_SIZES[4] ={200, 50,50,2000};
			char szBuffer[32];

			lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvC.fmt = LVCFMT_LEFT;
			for (int index=0;index < 4;index++)
			{
				lvC.iSubItem = index;
				lvC.cx = COLUMNS_SIZES[index];

				switch (index)
				{
					case 0: lstrcpy(szBuffer,Translate("Channel")); break;
					case 1: lstrcpy(szBuffer,"#"); break;
					case 2: lstrcpy(szBuffer,Translate("Mode")); break;
					case 3: lstrcpy(szBuffer,Translate("Topic")); break;

				}
				lvC.pszText = szBuffer;
				ListView_InsertColumn(GetDlgItem(hwndDlg, IDC_INFO_LISTVIEW),index,&lvC);
			}
			
			SetWindowPos(hwndDlg, HWND_TOP, (screen.right-screen.left)/2- (prefs->ListSize.x)/2,(screen.bottom-screen.top)/2- (prefs->ListSize.y)/2, (prefs->ListSize.x), (prefs->ListSize.y), 0);
			SendMessage(hwndDlg, WM_SIZE, SIZE_RESTORED, MAKELPARAM(prefs->ListSize.x, prefs->ListSize.y));
			ListView_SetExtendedListViewStyle(GetDlgItem(hwndDlg, IDC_INFO_LISTVIEW), LVS_EX_FULLROWSELECT);
			SendMessage(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)LoadIconEx(IDI_LIST)); // Tell the dialog to use it
		} break;

	case WM_SIZE:
		{

			RECT winRect;
			GetClientRect(hwndDlg, &winRect);
			RECT buttRect;
			GetWindowRect(GetDlgItem(hwndDlg, IDC_JOIN), &buttRect);
			SetWindowPos(GetDlgItem(hwndDlg, IDC_INFO_LISTVIEW), HWND_TOP, 4, 4, winRect.right-8, winRect.bottom-36, 0 );
			SetWindowPos(GetDlgItem(hwndDlg, IDC_CLOSE), HWND_TOP, winRect.right-84, winRect.bottom-28, buttRect.right- buttRect.left, buttRect.bottom- buttRect.top, 0 );
			SetWindowPos(GetDlgItem(hwndDlg, IDC_JOIN), HWND_TOP, winRect.right-174,  winRect.bottom-28, buttRect.right- buttRect.left, buttRect.bottom- buttRect.top, 0 );
			SetWindowPos(GetDlgItem(hwndDlg, IDC_TEXT), HWND_TOP, 4,  winRect.bottom-28, winRect.right-200, buttRect.bottom- buttRect.top, 0 );

			GetWindowRect(hwndDlg, &winRect);
			prefs->ListSize.x = winRect.right-winRect.left;
			prefs->ListSize.y = winRect.bottom-winRect.top;
			DBWriteContactSettingDword(NULL,IRCPROTONAME, "SizeOfListBottom", prefs->ListSize.y);
			DBWriteContactSettingDword(NULL,IRCPROTONAME, "SizeOfListRight", prefs->ListSize.x);
			return 0;		

		} break;
		
		case WM_COMMAND:
		{
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_CLOSE)
			{
				PostMessage (hwndDlg, WM_CLOSE, 0,0);
			}
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_JOIN)
			{
					char szTemp[255];
					int i = ListView_GetSelectionMark(GetDlgItem(hwndDlg, IDC_INFO_LISTVIEW));
					ListView_GetItemText(GetDlgItem(hwndDlg, IDC_INFO_LISTVIEW), i, 0, szTemp, 255); 
					PostIrcMessage( "/JOIN %s", szTemp);
			}
		}break;
		case WM_NOTIFY :
		{
			switch (((NMHDR*)lParam)->code)
			{
				case NM_DBLCLK:
				{
					char szTemp[255];
					int i = ListView_GetSelectionMark(GetDlgItem(hwndDlg, IDC_INFO_LISTVIEW));
					ListView_GetItemText(GetDlgItem(hwndDlg, IDC_INFO_LISTVIEW), i, 0, szTemp, 255); 
					PostIrcMessage( "/JOIN %s", szTemp);

				} break;
				case LVN_COLUMNCLICK :
				{
					LPNMLISTVIEW lv;
					lv = (LPNMLISTVIEW)lParam;
					SendMessage(GetDlgItem(hwndDlg, IDC_INFO_LISTVIEW), LVM_SORTITEMS, (WPARAM)lv->iSubItem, (LPARAM)ListViewSort);
					SendMessage(list_hWnd, IRC_UPDATELIST, 0, 0);
				} break;
			}
		} break;
		
		case WM_CLOSE:
		{
			DestroyWindow(hwndDlg);
		
		} break;

		
		case WM_DESTROY:
		{
			list_hWnd = NULL;
		} break;
		
		default:break;
	}
	return(false);
}

BOOL CALLBACK JoinWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
		
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);					
			HFONT hFont;
			LOGFONT lf;
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			GetObject(hFont,sizeof(lf),&lf);
			lf.lfHeight=(int)(lf.lfHeight*1.2);
			lf.lfWeight=FW_BOLD;
			hFont=CreateFontIndirect(&lf);
			SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem(hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadIconEx(IDI_LOGO), 0);
			DBVARIANT dbv;
  			if (!DBGetContactSetting(NULL, IRCPROTONAME, "RecentChannels", &dbv) && dbv.type == DBVT_ASCIIZ) 
			{
				for (int i = 0; i<20; i++) {
					if (!GetWord(dbv.pszVal, i).empty()) {
						String S = ReplaceString(GetWord(dbv.pszVal, i) ,"%newl" , " ");
						SendDlgItemMessage(hwndDlg, IDC_ENICK, CB_ADDSTRING, 0, (LPARAM)S.c_str());
					}
				}
				DBFreeVariant(&dbv);
			}
		} break;
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		{
			if((HWND)lParam==GetDlgItem(hwndDlg,IDC_WHITERECT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_TEXT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_CAPTION)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_LOGO))
			{
				SetTextColor((HDC)wParam,RGB(0,0,0));
				SetBkColor((HDC)wParam,RGB(255,255,255));
				return (BOOL)GetStockObject(WHITE_BRUSH);
			}
		}break;
		
		case WM_COMMAND:
		{
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNCANCEL)
			{
				PostMessage (hwndDlg, WM_CLOSE, 0,0);
			}
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNOK)
			{
				char szTemp[255];
				SendMessage(GetDlgItem(hwndDlg, IDC_ENICK), WM_GETTEXT, 255, (LPARAM) szTemp);
				if (IsChannel(szTemp))
					PostIrcMessage( "/JOIN %s", szTemp);
				else
					PostIrcMessage( "/JOIN #%s", szTemp);

				String S = ReplaceString(szTemp, " ", "%newl");
				String SL = S;
				DBVARIANT dbv;
	  			if (!DBGetContactSetting(NULL, IRCPROTONAME, "RecentChannels", &dbv) && dbv.type == DBVT_ASCIIZ) 
				{
					for (int i = 0; i<20; i++) {
						if (!GetWord(dbv.pszVal, i).empty() && GetWord(dbv.pszVal, i) != SL) {
							S += " ";
							S += GetWord(dbv.pszVal, i);
						}
					}
					DBFreeVariant(&dbv);
				}
				DBWriteContactSettingString(NULL, IRCPROTONAME, "RecentChannels", S.c_str());
				PostMessage (hwndDlg, WM_CLOSE, 0,0);
			}
		} break;
		
		case WM_CLOSE:
		{
			DestroyWindow(hwndDlg);

		} break;
		case WM_DESTROY:
		{
			HFONT hFont;
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDNOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				
			join_hWnd = NULL;
		} break;
		default:break;
	}
	return(false);
}
BOOL CALLBACK InitWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
		
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);					
			HFONT hFont;
			LOGFONT lf;
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			GetObject(hFont,sizeof(lf),&lf);
			lf.lfHeight=(int)(lf.lfHeight*1.2);
			lf.lfWeight=FW_BOLD;
			hFont=CreateFontIndirect(&lf);
			SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem(hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadIconEx(IDI_LOGO), 0);
		} break;
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		{
			if((HWND)lParam==GetDlgItem(hwndDlg,IDC_WHITERECT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_TEXT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_CAPTION)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_LOGO))
			{
				SetTextColor((HDC)wParam,RGB(0,0,0));
				SetBkColor((HDC)wParam,RGB(255,255,255));
				return (BOOL)GetStockObject(WHITE_BRUSH);
			}
		}break;
		
		case WM_COMMAND:
		{
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNCANCEL)
			{
				PostMessage (hwndDlg, WM_CLOSE, 0,0);
			}
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNOK)
			{
				int i = SendMessage(GetDlgItem(hwndDlg, IDC_EDIT), WM_GETTEXTLENGTH, 0, 0);
				int j = SendMessage(GetDlgItem(hwndDlg, IDC_EDIT2), WM_GETTEXTLENGTH, 0, 0);
				if (i >0 && j > 0)
				{
					char l [30];
					char m [200];
					SendMessage(GetDlgItem(hwndDlg, IDC_EDIT), WM_GETTEXT, 30, (LPARAM) l);
					SendMessage(GetDlgItem(hwndDlg, IDC_EDIT2), WM_GETTEXT, 200, (LPARAM) m);

					DBWriteContactSettingString(NULL, IRCPROTONAME, "PNick", l);
					DBWriteContactSettingString(NULL, IRCPROTONAME, "Nick", l);
					DBWriteContactSettingString(NULL, IRCPROTONAME, "Name", m);
					lstrcpyn(prefs->Nick, l, 30);
					lstrcpyn(prefs->Name, m, 200);
					if (lstrlen(prefs->AlternativeNick) == 0)
					{
						char szTemp[30];
						mir_snprintf(szTemp, sizeof(szTemp), "%s%u", l, rand()%9999);
						DBWriteContactSettingString(NULL, IRCPROTONAME, "AlernativeNick", szTemp);
						lstrcpyn(prefs->AlternativeNick, szTemp, 30);					
					}
				}

				PostMessage (hwndDlg, WM_CLOSE, 0,0);

			}
			
			

		} break;
		
		case WM_CLOSE:
		{
			DestroyWindow(hwndDlg);

		} break;
		case WM_DESTROY:
		{
			HFONT hFont;
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDNOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				
		} break;
		default:break;
	}
	return(false);
}
BOOL CALLBACK QuickWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
		
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			HFONT hFont;
			LOGFONT lf;
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			GetObject(hFont,sizeof(lf),&lf);
			lf.lfHeight=(int)(lf.lfHeight*1.2);
			lf.lfWeight=FW_BOLD;
			hFont=CreateFontIndirect(&lf);
			SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem(hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadIconEx(IDI_LOGO), 0);
			
			char * p1 = pszServerFile;
			char * p2 = pszServerFile;
			if(pszServerFile) 
				while (strchr(p2, 'n'))
				{
					SERVER_INFO * pData = new SERVER_INFO;
					p1 = strchr(p2, '=');
					++p1;
					p2 = strstr(p1, "SERVER:");
					pData->Name=new char[p2-p1+1];
					lstrcpyn(pData->Name, p1, p2-p1+1);
					
					p1 = strchr(p2, ':');
					++p1;
					pData->iSSL = 0;
					if(strstr(p1, "SSL") == p1)
					{
						p1 +=3;
						if(*p1 == '1')
							pData->iSSL = 1;
						else if(*p1 == '2')
							pData->iSSL = 2;
						p1++;
					}

					p2 = strchr(p1, ':');
					pData->Address=new char[p2-p1+1];
					lstrcpyn(pData->Address, p1, p2-p1+1);
					
					p1 = p2;
					p1++;
					while (*p2 !='G' && *p2 != '-')
						p2++;
					pData->PortStart = new char[p2-p1+1];
					lstrcpyn(pData->PortStart, p1, p2-p1+1);

					if (*p2 == 'G'){
						pData->PortEnd = new char[p2-p1+1];
						lstrcpy(pData->PortEnd, pData->PortStart);
					} else {
						p1 = p2;
						p1++;
						p2 = strchr(p1, 'G');
						pData->PortEnd = new char[p2-p1+1];
						lstrcpyn(pData->PortEnd, p1, p2-p1+1);
					}

					p1 = strchr(p2, ':');
					p1++;
					p2 = strchr(p1, '\r');
					if (!p2)
						p2 = strchr(p1, '\n');
					if (!p2)
						p2 = strchr(p1, '\0');
					pData->Group = new char[p2-p1+1];
					lstrcpyn(pData->Group, p1, p2-p1+1);
					int iItem = SendDlgItemMessage(hwndDlg, IDC_SERVERCOMBO, CB_ADDSTRING,0,(LPARAM) pData->Name);
					SendDlgItemMessage(hwndDlg, IDC_SERVERCOMBO, CB_SETITEMDATA, iItem,(LPARAM) pData);

				}
				else
					EnableWindow(GetDlgItem(hwndDlg, IDNOK), false);
					
				SendMessage(GetDlgItem(hwndDlg, IDC_SERVER), EM_SETREADONLY, true, 0);
				SendMessage(GetDlgItem(hwndDlg, IDC_PORT), EM_SETREADONLY, true, 0);
				SendMessage(GetDlgItem(hwndDlg, IDC_PORT2), EM_SETREADONLY, true, 0);

				if (prefs->QuickComboSelection != -1)
				{
					SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_SETCURSEL, prefs->QuickComboSelection,0);				
					SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO, CBN_SELCHANGE), 0);
				}
				else
					EnableWindow(GetDlgItem(hwndDlg, IDNOK), false);
			
		} break;
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		{
			if((HWND)lParam==GetDlgItem(hwndDlg,IDC_WHITERECT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_TEXT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_CAPTION)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_LOGO))
			{
				SetTextColor((HDC)wParam,RGB(0,0,0));
				SetBkColor((HDC)wParam,RGB(255,255,255));
				return (BOOL)GetStockObject(WHITE_BRUSH);
			}
		}break;

		
		case WM_COMMAND:
		{
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNCANCEL)
			{
				PostMessage (hwndDlg, WM_CLOSE, 0,0);
			}
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNOK)
			{
				GetDlgItemText(hwndDlg,IDC_SERVER, prefs->ServerName, 100);				 
				GetDlgItemText(hwndDlg,IDC_PORT, prefs->PortStart, 7);
				GetDlgItemText(hwndDlg,IDC_PORT2, prefs->PortEnd , 7);
				GetDlgItemText(hwndDlg,IDC_PASS, prefs->Password, 500);
				
				int i = SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
				SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
				if (pData && (int)pData != CB_ERR)
				{
					lstrcpy(prefs->Network, pData->Group); 
					if(m_ssleay32)
						prefs->iSSL = pData->iSSL;
					else
						prefs->iSSL = 0;
				}
				
				char windowname[20];
				GetWindowText(hwndDlg, windowname, 20);
				if (lstrcmpi(windowname, "Miranda IRC") == 0)
				{
					prefs->ServerComboSelection = SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
					DBWriteContactSettingDword(NULL,IRCPROTONAME,"ServerComboSelection",prefs->ServerComboSelection);
					DBWriteContactSettingString(NULL,IRCPROTONAME,"ServerName",prefs->ServerName);
					DBWriteContactSettingString(NULL,IRCPROTONAME,"PortStart",prefs->PortStart);
					DBWriteContactSettingString(NULL,IRCPROTONAME,"PortEnd",prefs->PortEnd);
					CallService(MS_DB_CRYPT_ENCODESTRING, 499, (LPARAM)prefs->Password);
					DBWriteContactSettingString(NULL,IRCPROTONAME,"Password",prefs->Password);
					CallService(MS_DB_CRYPT_DECODESTRING, 499, (LPARAM)prefs->Password);
					DBWriteContactSettingString(NULL,IRCPROTONAME,"Network",prefs->Network);
					DBWriteContactSettingByte(NULL,IRCPROTONAME,"UseSSL",prefs->iSSL);

				}
				prefs->QuickComboSelection = SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
				DBWriteContactSettingDword(NULL,IRCPROTONAME,"QuickComboSelection",prefs->QuickComboSelection);
				DisconnectFromServer();
				ConnectToServer();
				PostMessage (hwndDlg, WM_CLOSE, 0,0);
			}


			if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_SERVERCOMBO)
			{


				int i = SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
				SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
				if (i !=CB_ERR)
				{
					SetDlgItemText(hwndDlg,IDC_SERVER,pData->Address);
					SetDlgItemText(hwndDlg,IDC_PORT,pData->PortStart);
					SetDlgItemText(hwndDlg,IDC_PORT2,pData->PortEnd);
					SetDlgItemText(hwndDlg,IDC_PASS,"");
					if(m_ssleay32)
					{
						if(pData->iSSL == 0)
							SetDlgItemText(hwndDlg,IDC_SSL,Translate("Off") );
						if(pData->iSSL == 1)
							SetDlgItemText(hwndDlg,IDC_SSL,Translate("Auto") );
						if(pData->iSSL == 2)
							SetDlgItemText(hwndDlg,IDC_SSL,Translate("On") );
					}
					else
						SetDlgItemText(hwndDlg,IDC_SSL, Translate("N/A"));
					EnableWindow(GetDlgItem(hwndDlg, IDNOK), true);

				}

			}			
			

		} break;
		
		case WM_CLOSE:
		{
			DestroyWindow(hwndDlg);

		} break;
		
		case WM_DESTROY:
		{			
			HFONT hFont;
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDNOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				
			
			int j = (int) SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCOUNT, 0, 0);
			for (int index2 = 0; index2 < j; index2++)
			{
				SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, index2, 0);
				delete []pData->Name;
				delete []pData->Address;
				delete []pData->PortStart;
				delete []pData->PortEnd;
				delete []pData->Group;
				delete pData;						

			}
			
			quickconn_hWnd = NULL;
		} break;


		default:break;
	}
	return(false);
}

int CALLBACK ListViewSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	if (!list_hWnd)
		return 0;
	char temp1[512];
	char temp2[512];
	LVITEM lvm;
	lvm.mask = LVIF_TEXT;
	lvm.iItem = lParam1;
	lvm.iSubItem = lParamSort;
	lvm.pszText = temp1;
	lvm.cchTextMax = 511;
	SendMessage(GetDlgItem(list_hWnd, IDC_INFO_LISTVIEW), LVM_GETITEM, 0, (LPARAM)&lvm);
	lvm.iItem = lParam2;
	lvm.pszText = temp2;
	SendMessage(GetDlgItem(list_hWnd, IDC_INFO_LISTVIEW), LVM_GETITEM, 0, (LPARAM)&lvm);
	if (lParamSort != 1){
		if (lstrlen(temp1) !=0 && lstrlen(temp2) !=0)
			return lstrcmpi(temp1, temp2);
		else
		{
			if(lstrlen(temp1) ==0 )
				return 1;
			else
				return -1;
		}
	}
	else {
		if(StrToInt(temp1) < StrToInt(temp2))
			return 1;
		else 
			return -1;
	}
}
static void __cdecl AckUserInfoSearch(void * lParam)
{
	ProtoBroadcastAck(IRCPROTONAME, (void*)lParam, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
}

#define STR_BASIC "Faster! Searches the network for an exact match of the nickname only. The hostmask is optional and provides further security if used. Wildcards (? and *) are allowed."		
#define STR_ADVANCED "Slower! Searches the network for nicknames matching a wildcard string. The hostmask is mandatory and a minimum of 4 characters is necessary in the \"Nick\" field. Wildcards (? and *) are allowed."		
#define STR_ERROR "Settings could not be saved!\n\nThe \"Nick\" field must contain at least four characters including wildcards,\n and it must also match the default nickname for this contact."		
#define STR_ERROR2 "Settings could not be saved!\n\nA full hostmask must be set for this online detection mode to work."		

BOOL CALLBACK UserDetailsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HANDLE hContact;
	hContact = (HANDLE) GetWindowLong(hwndDlg, GWL_USERDATA);
	switch (msg) {
		case WM_INITDIALOG:
		{
			DBVARIANT dbv;
			hContact = (HANDLE) lParam;
			BYTE bAdvanced = DBGetContactSettingByte(hContact, IRCPROTONAME, "AdvancedMode", 0);

			TranslateDialogDefault(hwndDlg);

			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) hContact);

			CheckDlgButton(hwndDlg, IDC_RADIO1, bAdvanced?BST_UNCHECKED:BST_CHECKED);
			CheckDlgButton(hwndDlg, IDC_RADIO2, bAdvanced?BST_CHECKED:BST_UNCHECKED);
			EnableWindow(GetDlgItem(hwndDlg, IDC_WILDCARD), bAdvanced);

			if(!bAdvanced)
			{
				SetDlgItemText(hwndDlg, IDC_DEFAULT, STR_BASIC);
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "Default", &dbv) && dbv.type == DBVT_ASCIIZ) {
					SetDlgItemText(hwndDlg, IDC_WILDCARD, dbv.pszVal);
					DBFreeVariant(&dbv);
				}
			}
			else 
			{
				SetDlgItemText(hwndDlg, IDC_DEFAULT, STR_ADVANCED);
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "UWildcard", &dbv) && dbv.type == DBVT_ASCIIZ) {
					SetDlgItemText(hwndDlg, IDC_WILDCARD, dbv.pszVal);
					DBFreeVariant(&dbv);
				}
			}

			if (!DBGetContactSetting(hContact, IRCPROTONAME, "UUser", &dbv) && dbv.type == DBVT_ASCIIZ) {
				SetDlgItemText(hwndDlg, IDC_USER, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			if (!DBGetContactSetting(hContact, IRCPROTONAME, "UHost", &dbv) && dbv.type == DBVT_ASCIIZ) {
				SetDlgItemText(hwndDlg, IDC_HOST, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			mir_forkthread(AckUserInfoSearch, hContact);

        }break;
		case WM_COMMAND:
		{

			if(	(LOWORD(wParam)==IDC_WILDCARD
				|| LOWORD(wParam)==IDC_USER
				|| LOWORD(wParam) == IDC_HOST
				)
			&& (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))	return true;		

			EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON), true);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON2), true);

			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_BUTTON)
			{
				char temp[500];
				GetDlgItemText(hwndDlg, IDC_WILDCARD, temp, 500);
				DBVARIANT dbv;
				BYTE bAdvanced = IsDlgButtonChecked(hwndDlg, IDC_RADIO1)?0:1;
				


				if(bAdvanced)
				{
					if(GetWindowTextLength(GetDlgItem(hwndDlg, IDC_WILDCARD)) == 0
						|| GetWindowTextLength(GetDlgItem(hwndDlg, IDC_USER)) == 0
						|| GetWindowTextLength(GetDlgItem(hwndDlg, IDC_HOST)) == 0)
					{
						MessageBox(NULL, Translate(	STR_ERROR2	), Translate("IRC error"), MB_OK|MB_ICONERROR);
						return false;
					}
					if (!DBGetContactSetting(hContact, IRCPROTONAME, "Default", &dbv) && dbv.type == DBVT_ASCIIZ ) 
					{
						String S = STR_ERROR + (String)" (" + dbv.pszVal + (String)")";
						if((lstrlen(temp) <4 && lstrlen(temp)) || !WCCmp(CharLower(temp), CharLower(dbv.pszVal)))
						{
							MessageBox(NULL, Translate(	S.c_str()	), Translate("IRC error"), MB_OK|MB_ICONERROR);
							DBFreeVariant(&dbv);
							return false;
						}
						DBFreeVariant(&dbv);
					}
					GetDlgItemText(hwndDlg, IDC_WILDCARD, temp, 500);
					if (lstrlen(GetWord(temp, 0).c_str()))
						DBWriteContactSettingString(hContact, IRCPROTONAME, "UWildcard", GetWord(temp, 0).c_str());
					else
						DBDeleteContactSetting(hContact, IRCPROTONAME, "UWildcard");
				}

				DBWriteContactSettingByte(hContact, IRCPROTONAME, "AdvancedMode", bAdvanced);

				GetDlgItemText(hwndDlg, IDC_USER, temp, 500);
				if (lstrlen(GetWord(temp, 0).c_str()))
					DBWriteContactSettingString(hContact, IRCPROTONAME, "UUser", GetWord(temp, 0).c_str());
				else
					DBDeleteContactSetting(hContact, IRCPROTONAME, "UUser");

				GetDlgItemText(hwndDlg, IDC_HOST, temp, 500);
				if (lstrlen(GetWord(temp, 0).c_str()))
					DBWriteContactSettingString(hContact, IRCPROTONAME, "UHost", GetWord(temp, 0).c_str());
				else
					DBDeleteContactSetting(hContact, IRCPROTONAME, "UHost");

				EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON), false);

			}
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_BUTTON2)
			{
				if(IsDlgButtonChecked(hwndDlg, IDC_RADIO2))
					SetDlgItemText(hwndDlg, IDC_WILDCARD, "");
				SetDlgItemText(hwndDlg, IDC_HOST, "");
				SetDlgItemText(hwndDlg, IDC_USER, "");
				DBDeleteContactSetting(hContact, IRCPROTONAME, "UWildcard");
				DBDeleteContactSetting(hContact, IRCPROTONAME, "UUser");
				DBDeleteContactSetting(hContact, IRCPROTONAME, "UHost");
				EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON), false);
				EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON2), false);

			}		
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_RADIO1)
			{
				DBVARIANT dbv;
				SetDlgItemText(hwndDlg, IDC_DEFAULT, STR_BASIC);
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "Default", &dbv) && dbv.type == DBVT_ASCIIZ) {
					SetDlgItemText(hwndDlg, IDC_WILDCARD, dbv.pszVal);
					DBFreeVariant(&dbv);
				}
				EnableWindow(GetDlgItem(hwndDlg, IDC_WILDCARD), false);

			}	
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_RADIO2)
			{
				DBVARIANT dbv;
				SetDlgItemText(hwndDlg, IDC_DEFAULT, STR_ADVANCED);
				if (!DBGetContactSetting(hContact, IRCPROTONAME, "UWildcard", &dbv) && dbv.type == DBVT_ASCIIZ) {
					SetDlgItemText(hwndDlg, IDC_WILDCARD, dbv.pszVal);
					DBFreeVariant(&dbv);
				}
				EnableWindow(GetDlgItem(hwndDlg, IDC_WILDCARD), true);

			}	

		}break;
    }
    return FALSE;
}
BOOL CALLBACK QuestionWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
		
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			HFONT hFont;
			LOGFONT lf;
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			GetObject(hFont,sizeof(lf),&lf);
			lf.lfHeight=(int)(lf.lfHeight*1.2);
			lf.lfWeight=FW_BOLD;
			hFont=CreateFontIndirect(&lf);
			SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem(hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadIconEx(IDI_LOGO), 0);

		} break;
		
		case WM_COMMAND:
		{
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNCANCEL)
			{
				PostMessage (hwndDlg, WM_CLOSE, 0,0);
			}
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDNOK)
			{
				int i = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_EDIT));
				if (i > 0)
				{
					char * l= new char[i +2];
					GetDlgItemText(hwndDlg, IDC_EDIT, l, 200);

					int j = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_HIDDENEDIT));
					char * m;
					m = new char[j+2];
					GetDlgItemText(hwndDlg, IDC_HIDDENEDIT, m, j+1);

					char * text = strstr(m, "%question");
					char * p1 = text;
					char * p2 = NULL;
					if (p1)
					{
						p1 += 9;
						if (*p1 == '=' && p1[1] == '\"')
						{
							p1 += 2;
							for	(int k =0; k< 3;k++)
							{
								p2 = strchr(p1, '\"');
								if (p2) 
								{
									p2++;
									if (k == 2 || (*p2 != ',' || (*p2 == ',' && p2[1] != '\"')))
									{
										*p2 = '\0';	

									}
									else
										p2 +=2;
									p1 = p2;
								}
							}

						}
						else
							*p1 = '\0';

					}
					
					char * n;
					n = new char[j+2];
					GetDlgItemText(hwndDlg, IDC_HIDDENEDIT, n, j+1);
				
					String S = ReplaceString(n, text, l);

					PostIrcMessageWnd( NULL, NULL, (char*)S.c_str());

					delete []m;
					delete []l;
					delete []n;
					HWND hwnd = GetParent(hwndDlg);
					if(hwnd)
						SendMessage(hwnd, IRC_QUESTIONAPPLY, 0, 0);

				}
				SendMessage (hwndDlg, WM_CLOSE, 0,0);

			}
			
			

		} break;
		case IRC_ACTIVATE:
		{
			ShowWindow(hwndDlg, SW_SHOW);
			SetActiveWindow(hwndDlg);
		}break;
		case WM_CLOSE:
		{
			HWND hwnd = GetParent(hwndDlg);
			if(hwnd)
				SendMessage(GetParent(hwndDlg), IRC_QUESTIONCLOSE, 0, 0);
			DestroyWindow(hwndDlg);

		} break;
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		{
			if((HWND)lParam==GetDlgItem(hwndDlg,IDC_WHITERECT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_TEXT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_CAPTION)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_LOGO))
			{
				SetTextColor((HDC)wParam,RGB(0,0,0));
				SetBkColor((HDC)wParam,RGB(255,255,255));
				return (BOOL)GetStockObject(WHITE_BRUSH);
			}
		}break;
		case WM_DESTROY:
		{			
			HFONT hFont;
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDNOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				

		} break;	
		
		default:break;
	}
	return(false);
}

BOOL CALLBACK ManagerWndProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
		
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			HFONT hFont;
			LOGFONT lf;
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			GetObject(hFont,sizeof(lf),&lf);
			lf.lfHeight=(int)(lf.lfHeight*1.2);
			lf.lfWeight=FW_BOLD;
			hFont=CreateFontIndirect(&lf);
			SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_SETFONT,(WPARAM)hFont,0);

			POINT pt;
			pt.x = 3; 
			pt.y = 3; 
			HWND hwndEdit = ChildWindowFromPoint(GetDlgItem(hwndDlg, IDC_TOPIC), pt); 
			
			OldMgrEditProc = (WNDPROC)SetWindowLong(hwndEdit, GWL_WNDPROC,(LONG)MgrEditSubclassProc); 
			
			SendDlgItemMessage(hwndDlg,IDC_ADD,BM_SETIMAGE,IMAGE_ICON,(LPARAM)(HICON)LoadIconEx(IDI_ADD));
			SendDlgItemMessage(hwndDlg,IDC_REMOVE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_DELETE));
			SendDlgItemMessage(hwndDlg,IDC_EDIT,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx(IDI_RENAME));
			SendMessage(GetDlgItem(hwndDlg,IDC_ADD), BUTTONADDTOOLTIP, (WPARAM)Translate("Add ban/invite/exception"), 0);
			SendMessage(GetDlgItem(hwndDlg,IDC_EDIT), BUTTONADDTOOLTIP, (WPARAM)Translate("Edit selected ban/invite/exception"), 0);
			SendMessage(GetDlgItem(hwndDlg,IDC_REMOVE), BUTTONADDTOOLTIP, (WPARAM)Translate("Delete selected ban/invite/exception"), 0);

			SendMessage(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)LoadIconEx(IDI_MANAGER));
			SendMessage(GetDlgItem(hwndDlg, IDC_LOGO),STM_SETICON,(LPARAM)(HICON)LoadIconEx(IDI_LOGO), 0);
			SendDlgItemMessage(hwndDlg,IDC_APPLYTOPIC,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx( IDI_GO ));
			SendDlgItemMessage(hwndDlg,IDC_APPLYMODES,BM_SETIMAGE,IMAGE_ICON,(LPARAM)LoadIconEx( IDI_GO ));
			SendMessage(GetDlgItem(hwndDlg,IDC_APPLYTOPIC), BUTTONADDTOOLTIP, (WPARAM)Translate("Set this topic for the channel"), 0);
			SendMessage(GetDlgItem(hwndDlg,IDC_APPLYMODES), BUTTONADDTOOLTIP, (WPARAM)Translate("Set these modes for the channel"), 0);

			SendDlgItemMessage(hwndDlg,IDC_LIST,LB_SETHORIZONTALEXTENT,750,NULL);
			CheckDlgButton(hwndDlg, IDC_RADIO1, BST_CHECKED);
			if (!strchr(sChannelModes.c_str(), 't'))
				EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK1), false);
			if (!strchr(sChannelModes.c_str(), 'n'))
				EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK2), false);
			if (!strchr(sChannelModes.c_str(), 'i'))
				EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK3), false);
			if (!strchr(sChannelModes.c_str(), 'm'))
				EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK4), false);
			if (!strchr(sChannelModes.c_str(), 'k'))
				EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK5), false);
			if (!strchr(sChannelModes.c_str(), 'l'))
				EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK6), false);
			if (!strchr(sChannelModes.c_str(), 'p'))
				EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK7), false);
			if (!strchr(sChannelModes.c_str(), 's'))
				EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK8), false);
		} break;
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		{
			if((HWND)lParam==GetDlgItem(hwndDlg,IDC_WHITERECT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_TEXT)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_CAPTION)
					|| (HWND)lParam==GetDlgItem(hwndDlg,IDC_LOGO))
			{
				SetTextColor((HDC)wParam,RGB(0,0,0));
				SetBkColor((HDC)wParam,RGB(255,255,255));
				return (BOOL)GetStockObject(WHITE_BRUSH);
			}
		}break;
		case IRC_INITMANAGER:
		{
			char * window = (char *) lParam;
			CHANNELINFO* wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, window, NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);

			if(wi)
			{
				if (g_ircSession)
				{
					char temp[1000];
					mir_snprintf(temp, sizeof(temp), "Topic%s%s", window, g_ircSession.GetInfo().sNetwork.c_str());

					DBVARIANT dbv;
  					if (!DBGetContactSetting(NULL, IRCPROTONAME, temp, &dbv) && dbv.type == DBVT_ASCIIZ) 
					{
						for (int i = 0; i<5; i++) {
							if (!GetWord(dbv.pszVal, i).empty()) {
								String S = ReplaceString(GetWord(dbv.pszVal, i) ,"%¤" , " ");
								SendDlgItemMessage(hwndDlg, IDC_TOPIC, CB_ADDSTRING, 0, (LPARAM)S.c_str());
							}
						}
						DBFreeVariant(&dbv);
					}
				}

				if(wi->pszTopic)
					SetDlgItemText(hwndDlg, IDC_TOPIC, wi->pszTopic);

				if (!IsDlgButtonChecked(manager_hWnd, IDC_NOTOP))
					EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), true);


				bool add =false;
				char * p1= wi->pszMode;
				if(p1)
				{
					while (*p1 != '\0' && *p1 != ' ')
					{
						if (*p1 == '+')
							add = true;
						if (*p1 == '-')
							add = false;
						if (*p1 == 't')
							CheckDlgButton(hwndDlg, IDC_CHECK1, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'n')
							CheckDlgButton(hwndDlg, IDC_CHECK2, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'i')
							CheckDlgButton(hwndDlg, IDC_CHECK3, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'm')
							CheckDlgButton(hwndDlg, IDC_CHECK4, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'p')
							CheckDlgButton(hwndDlg, IDC_CHECK7, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 's')
							CheckDlgButton(hwndDlg, IDC_CHECK8, add?(BST_CHECKED) : (BST_UNCHECKED));
						if (*p1 == 'k' && add) {
							CheckDlgButton(hwndDlg, IDC_CHECK5, add?(BST_CHECKED) : (BST_UNCHECKED));
							EnableWindow(GetDlgItem(hwndDlg, IDC_KEY), add?(true) : (false));
							if(wi->pszPassword)
								SetDlgItemText(hwndDlg, IDC_KEY, wi->pszPassword);
						}
						if (*p1 == 'l' && add) {
							CheckDlgButton(hwndDlg, IDC_CHECK6, add?(BST_CHECKED) : (BST_UNCHECKED));
							EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT), add?(true) : (false));
							if(wi->pszLimit)
								SetDlgItemText(hwndDlg, IDC_LIMIT, wi->pszLimit);
						}
						p1++;
						if (wParam ==0)
						{
							EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT), (false));
							EnableWindow(GetDlgItem(hwndDlg, IDC_KEY), (false));
							EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK1),  (false));
							EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK2),  (false));
							EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK3), (false));
							EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK4), (false));
							EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK5),  (false));
							EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK6),  (false));
							EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK7),  (false));
							EnableWindow(GetDlgItem(hwndDlg, IDC_CHECK8), (false));
							EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), (false));
							if(IsDlgButtonChecked(hwndDlg, IDC_CHECK1))
								EnableWindow(GetDlgItem(hwndDlg, IDC_TOPIC), (false));
							CheckDlgButton(hwndDlg, IDC_NOTOP, BST_CHECKED);

						}
						ShowWindow(hwndDlg, SW_SHOW);

					}
				}
			}
			if (strchr(sChannelModes.c_str(), 'b'))
			{
				CheckDlgButton(hwndDlg, IDC_RADIO1, BST_CHECKED);
				PostIrcMessage( "/MODE %s +b", window);
			}

		} break;		
		case WM_COMMAND:
		{
			if(
				(	LOWORD(wParam)==IDC_LIMIT
					|| LOWORD(wParam)==IDC_KEY
				)
				&& (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))	return true;		
			char window[256];
			GetDlgItemText(hwndDlg, IDC_CAPTION, window, 255);

			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDCLOSE)
			{
				PostMessage (hwndDlg, WM_CLOSE, 0,0);
			}
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_REMOVE)
			{
				int i = SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
				if (i != LB_ERR)
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE), false);
					EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), false);
					EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), false);
					char * m = new char[SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETTEXTLEN, i, 0)+2];
					SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETTEXT, i, (LPARAM)m);
					String user = GetWord(m, 0);
					delete[]m;
					char temp[100];
					char mode[3];
					if (IsDlgButtonChecked(hwndDlg, IDC_RADIO1))
					{
						lstrcpy(mode, "-b");
						lstrcpyn(temp, Translate(	"Remove ban?"	), 100);
					}
					if (IsDlgButtonChecked(hwndDlg, IDC_RADIO2))
					{
						lstrcpy(mode, "-I");
						lstrcpyn(temp, Translate(	"Remove invite?"	), 100);
					}
					if (IsDlgButtonChecked(hwndDlg, IDC_RADIO3))
					{
						lstrcpy(mode, "-e");
						lstrcpyn(temp, Translate(	"Remove exception?"	), 100);
					}
					
					if (MessageBox(hwndDlg,user.c_str(), temp, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2) == IDYES)
					{
						PostIrcMessage( "/MODE %s %s %s", window, mode, user.c_str());
						SendMessage(hwndDlg, IRC_QUESTIONAPPLY, 0, 0);

					}
					SendMessage(hwndDlg, IRC_QUESTIONCLOSE, 0, 0);
				}
			}

			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_EDIT)
			{
				if(IsDlgButtonChecked(hwndDlg, IDC_NOTOP))
					break;	

				int i = SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
				if (i != LB_ERR)
				{
					char * m = new char[SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETTEXTLEN, i, 0)+2];
					SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETTEXT, i, (LPARAM)m);
					String user = GetWord(m, 0);
					delete[]m;
					char temp[100];
					char mode[3];
					if (IsDlgButtonChecked(hwndDlg, IDC_RADIO1))
					{
						lstrcpy(mode, "b");
						lstrcpyn(temp, Translate(	"Edit ban"	), 100);
					}
					if (IsDlgButtonChecked(hwndDlg, IDC_RADIO2))
					{
						lstrcpy(mode, "I");
						lstrcpyn(temp, Translate(	"Edit invite?"	), 100);
					}
					if (IsDlgButtonChecked(hwndDlg, IDC_RADIO3))
					{
						lstrcpy(mode, "e");
						lstrcpyn(temp, Translate(	"Edit exception?"	), 100);
					}
						
					HWND addban_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_QUESTION),hwndDlg, QuestionWndProc);
					EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), false);
					EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE), false);
					EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), false);
					SetDlgItemText(addban_hWnd, IDC_CAPTION, temp);
					SetWindowText(GetDlgItem(addban_hWnd, IDC_TEXT), Translate(	"Please enter the hostmask (nick!user@host)"	));
					SetWindowText(GetDlgItem(addban_hWnd, IDC_EDIT), user.c_str());
					char temp2[450];
					mir_snprintf(temp2, 450, "/MODE %s -%s %s%s/MODE %s +%s %s", window, mode, user.c_str(), "%newl", window, mode, "%question");
					SetDlgItemText(addban_hWnd, IDC_HIDDENEDIT, temp2);
					PostMessage(addban_hWnd, IRC_ACTIVATE, 0, 0);
				}
			}			
			if(HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == IDC_LIST)
			{
				if(!IsDlgButtonChecked(hwndDlg, IDC_NOTOP))
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), true);
					EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE), true);
				}
			}
			if(HIWORD(wParam) == LBN_DBLCLK && LOWORD(wParam) == IDC_LIST)
			{
				SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_EDIT, BN_CLICKED), 0);
			}

			if(HIWORD(wParam) == EN_CHANGE && (LOWORD(wParam) == IDC_LIMIT || LOWORD(wParam) == IDC_KEY))
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_APPLYMODES), true);
			}

			if ((HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)  && LOWORD(wParam) == IDC_TOPIC)
			{
					EnableWindow(GetDlgItem(hwndDlg, IDC_APPLYTOPIC), true);

				return false;
			}
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_APPLYTOPIC)
			{
				char temp[470];
				GetDlgItemText(hwndDlg, IDC_TOPIC, temp, 470);
				PostIrcMessage ( "/TOPIC %s %s", window, temp);
				int i = SendDlgItemMessage(hwndDlg, IDC_TOPIC, CB_FINDSTRINGEXACT, -1, (LPARAM)temp);
				if(i!=LB_ERR)
					SendDlgItemMessage(hwndDlg, IDC_TOPIC, CB_DELETESTRING, i, 0);
				SendDlgItemMessage(hwndDlg, IDC_TOPIC, CB_INSERTSTRING, 0, (LPARAM)temp);
				SetDlgItemText(hwndDlg, IDC_TOPIC, temp);
				EnableWindow(GetDlgItem(hwndDlg, IDC_APPLYTOPIC), false);

			}
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_CHECK5)
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_KEY), IsDlgButtonChecked(hwndDlg, IDC_CHECK5)== BST_CHECKED);				
				EnableWindow(GetDlgItem(hwndDlg, IDC_APPLYMODES), true);				

			}
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_ADD)
			{
				char temp[100];
				char mode[3];
				if (IsDlgButtonChecked(hwndDlg, IDC_RADIO1))
				{
					lstrcpy(mode, "+b");
					lstrcpyn(temp, Translate(	"Add ban"	), 100);
				}
				if (IsDlgButtonChecked(hwndDlg, IDC_RADIO2))
				{
					lstrcpy(mode, "+I");
					lstrcpyn(temp, Translate(	"Add invite"	), 100);
				}
				if (IsDlgButtonChecked(hwndDlg, IDC_RADIO3))
				{
					lstrcpy(mode, "+e");
					lstrcpyn(temp, Translate(	"Add exception"	), 100);
				}
				HWND addban_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_QUESTION),hwndDlg, QuestionWndProc);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), false);
				EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE), false);
				EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), false);
				SetDlgItemText(addban_hWnd, IDC_CAPTION, temp);
				SetWindowText(GetDlgItem(addban_hWnd, IDC_TEXT), Translate(	"Please enter the hostmask (nick!user@host)"	));
				char temp2[450];
				mir_snprintf(temp2, 450, "/MODE %s %s %s", window, mode, "%question");
				SetDlgItemText(addban_hWnd, IDC_HIDDENEDIT, temp2);
				PostMessage(addban_hWnd, IRC_ACTIVATE, 0, 0);
			}
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_CHECK6)
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT), IsDlgButtonChecked(hwndDlg, IDC_CHECK6)== BST_CHECKED);				
				EnableWindow(GetDlgItem(hwndDlg, IDC_APPLYMODES), true);				
			}
			if(HIWORD(wParam) == BN_CLICKED && (LOWORD(wParam) == IDC_RADIO1 || LOWORD(wParam) == IDC_RADIO3 || LOWORD(wParam) == IDC_RADIO2))
			{
				SendMessage(hwndDlg, IRC_QUESTIONAPPLY, 0, 0);
			}
			if(HIWORD(wParam) == BN_CLICKED && (LOWORD(wParam) == IDC_CHECK1
				|| LOWORD(wParam) == IDC_CHECK2
				 || LOWORD(wParam) == IDC_CHECK3
				  || LOWORD(wParam) == IDC_CHECK4
				   || LOWORD(wParam) == IDC_CHECK7
				    || LOWORD(wParam) == IDC_CHECK8))
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_APPLYMODES), true);				
			}
			if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_APPLYMODES)
			{
				CHANNELINFO* wi = (CHANNELINFO *)DoEvent(GC_EVENT_GETITEMDATA, window, NULL, NULL, NULL, NULL, NULL, FALSE, FALSE, 0);
				if (wi )
				{
					char toadd[10]; *toadd = '\0';
					char toremove[10]; *toremove = '\0';
					String appendixadd = "";
					String appendixremove = "";
					if(wi->pszMode && strchr(wi->pszMode, 't'))
					{
						if(!IsDlgButtonChecked(hwndDlg, IDC_CHECK1))
							lstrcat(toremove, "t");
					}
					else
					{
						if(IsDlgButtonChecked(hwndDlg, IDC_CHECK1))
							lstrcat(toadd, "t");
					}

					if(wi->pszMode && strchr(wi->pszMode, 'n'))
					{
						if(!IsDlgButtonChecked(hwndDlg, IDC_CHECK2))
							lstrcat(toremove, "n");
					}
					else
					{
						if(IsDlgButtonChecked(hwndDlg, IDC_CHECK2))
							lstrcat(toadd, "n");
					}

					if(wi->pszMode && strchr(wi->pszMode, 'i'))
					{
						if(!IsDlgButtonChecked(hwndDlg, IDC_CHECK3))
							lstrcat(toremove, "i");
					}
					else
					{
						if(IsDlgButtonChecked(hwndDlg, IDC_CHECK3))
							lstrcat(toadd, "i");
					}

					if(wi->pszMode && strchr(wi->pszMode, 'm'))
					{
						if(!IsDlgButtonChecked(hwndDlg, IDC_CHECK4))
							lstrcat(toremove, "m");
					}
					else
					{
						if(IsDlgButtonChecked(hwndDlg, IDC_CHECK4))
							lstrcat(toadd, "m");
					}

					if(wi->pszMode && strchr(wi->pszMode, 'p'))
					{
						if(!IsDlgButtonChecked(hwndDlg, IDC_CHECK7))
							lstrcat(toremove, "p");
					}
					else
					{
						if(IsDlgButtonChecked(hwndDlg, IDC_CHECK7))
							lstrcat(toadd, "p");
					}

					if(wi->pszMode && strchr(wi->pszMode, 's'))
					{
						if(!IsDlgButtonChecked(hwndDlg, IDC_CHECK8))
							lstrcat(toremove, "s");
					}
					else
					{
						if(IsDlgButtonChecked(hwndDlg, IDC_CHECK8))
							lstrcat(toadd, "s");
					}
					String Key = "";
					String Limit = "";
					if(wi->pszMode && wi->pszPassword && strchr(wi->pszMode, 'k'))
					{
						if(!IsDlgButtonChecked(hwndDlg, IDC_CHECK5) )
						{
							lstrcat(toremove, "k");
							appendixremove += " ";
							appendixremove += wi->pszPassword;
						}
						else if(GetWindowTextLength(GetDlgItem(hwndDlg, IDC_KEY)))
						{
							char temp[400];
							GetDlgItemText(hwndDlg, IDC_KEY, temp, 14);

							if(Key != temp) {
								lstrcat(toremove, "k");
								lstrcat(toadd, "k");
								appendixadd += " ";
								appendixadd += temp;
								appendixremove += " ";
								appendixremove += wi->pszPassword;
							}
						}
					}
					else
					{
						if(IsDlgButtonChecked(hwndDlg, IDC_CHECK5) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_KEY)))
						{
							lstrcat(toadd, "k");
							appendixadd +=" ";
							char temp[400];
							GetDlgItemText(hwndDlg, IDC_KEY, temp, 399);
							appendixadd += temp;
						}

					}

					if(strchr(wi->pszMode, 'l'))
					{
						if(!IsDlgButtonChecked(hwndDlg, IDC_CHECK6) )
							lstrcat(toremove, "l");
						else if(GetWindowTextLength(GetDlgItem(hwndDlg, IDC_LIMIT)))
						{
							char temp[15];
							GetDlgItemText(hwndDlg, IDC_LIMIT, temp, 14);
							if(wi->pszLimit && lstrcmpi(wi->pszLimit, temp))
							{
								lstrcat(toadd, "l");
								appendixadd += " ";
								appendixadd += temp;
						
							}
						

						}
					}
					else
					{
						if(IsDlgButtonChecked(hwndDlg, IDC_CHECK6) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_LIMIT)))
						{
							lstrcat(toadd, "l");
							appendixadd +=" ";
							char temp[15];
							GetDlgItemText(hwndDlg, IDC_LIMIT, temp, 14);
							appendixadd += temp;
						}

					}


					if (lstrlen(toadd) || lstrlen(toremove))
					{
						char temp[500];
						lstrcpy(temp, "/mode ");
						lstrcat(temp, window);
						lstrcat(temp, " ");
						if(lstrlen(toremove))
							mir_snprintf(temp, 499, "%s-%s", temp, toremove);
						if(lstrlen(toadd))
							mir_snprintf(temp, 499, "%s+%s", temp, toadd);
						if (!appendixremove.empty())
							lstrcat(temp, appendixremove.c_str());
						if (!appendixadd.empty())
							lstrcat(temp, appendixadd.c_str());
						PostIrcMessage ( temp);
					}
				}
				EnableWindow(GetDlgItem(hwndDlg, IDC_APPLYMODES), false);				

			}

		} break;
		case IRC_QUESTIONCLOSE:
		{
			EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), true);
			if(SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETCURSEL, 0, 0) != LB_ERR)
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE), true);
				EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), true);
			}

		}break;
		case IRC_QUESTIONAPPLY:
		{
			char window[256];
			GetDlgItemText(hwndDlg, IDC_CAPTION, window, 255);

			char mode[3];
			lstrcpy(mode, "+b");
			if (IsDlgButtonChecked(hwndDlg, IDC_RADIO2))
				lstrcpy(mode, "+I");
			if (IsDlgButtonChecked(hwndDlg, IDC_RADIO3))
				lstrcpy(mode, "+e");
			SendDlgItemMessage(hwndDlg, IDC_LIST, LB_RESETCONTENT, 0, 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO1), false);
			EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO2), false);
			EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO3), false);
			EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), false);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), false);
			EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE), false);
			PostIrcMessage( "%s %s %s", "/MODE", window, mode); //wrong overloaded operator if three args
	
		}break;		
		case WM_CLOSE:
		{
			if (lParam != 3)
			{
				if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_APPLYMODES)) || IsWindowEnabled(GetDlgItem(hwndDlg, IDC_APPLYTOPIC)))
				{
					int i = MessageBox(hwndDlg, Translate(	"You have not applied all changes!\n\nApply before exiting?"	), Translate(	"IRC warning"	), MB_YESNOCANCEL|MB_ICONWARNING|MB_DEFBUTTON3);
					if( i== IDCANCEL)
						return false;

					if (i == IDYES)
					{
						if(IsWindowEnabled(GetDlgItem(hwndDlg, IDC_APPLYMODES)))
							SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_APPLYMODES, BN_CLICKED), 0);
						if(IsWindowEnabled(GetDlgItem(hwndDlg, IDC_APPLYTOPIC)))
							SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_APPLYTOPIC, BN_CLICKED), 0);
					}
				}
			}

			char window[256];
			GetDlgItemText(hwndDlg, IDC_CAPTION, window, 255);
			String S = ""; 
			char temp[1000];
			for (int i = 0; i<5; i++) 
			{
				if (SendDlgItemMessage(hwndDlg, IDC_TOPIC, CB_GETLBTEXT, i, (LPARAM)temp) != LB_ERR) 
				{
					S += " ";
					S += ReplaceString(temp ," " , "%¤");
				}
			}

			if (S != "" && g_ircSession)
			{
				mir_snprintf(temp, sizeof(temp), "Topic%s%s", window, g_ircSession.GetInfo().sNetwork.c_str());
				DBWriteContactSettingString(NULL, IRCPROTONAME, temp, S.c_str());
			}
			DestroyWindow(hwndDlg);

		} break;
		case WM_DESTROY:
		{			
			HFONT hFont;
			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_GETFONT,0,0);
			SendDlgItemMessage(hwndDlg,IDC_CAPTION,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDNOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);
			manager_hWnd = NULL;

		} break;	
		default:break;
	}
	return(false);
}

LRESULT CALLBACK MgrEditSubclassProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) 
{ 

	switch(msg) 
	{ 
		case WM_CHAR :
		{
			if (wParam == 21 || wParam == 11 || wParam == 2)
			{
				char w[2];
				if (wParam == 11)
				{
					w[0] = 3;
					w[1] = '\0';
				}
				if (wParam == 2)
				{
					w[0] = 2;
					w[1] = '\0';
				}
				if (wParam == 21)
				{
					w[0] = 31;
					w[1] = '\0';
				}
				SendMessage(hwndDlg, EM_REPLACESEL, false, (LPARAM) w);
				SendMessage(hwndDlg, EM_SCROLLCARET, 0,0);
				return 0;
			}

		} break;
		default: break;
	
	} 
	return CallWindowProc(OldMgrEditProc, hwndDlg, msg, wParam, lParam); 

} 
