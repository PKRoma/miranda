/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2006 Miranda ICQ/IM project, 
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
#include "m_clui.h"
#include "clist.h"
#include "m_clc.h"

int cliHotKeysRegister(HWND hwnd);
void HotKeysUnregister(HWND hwnd);
//void LoadContactTree(void);

static BOOL CALLBACK DlgProcItemsOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcGenOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcHotkeyOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcHotKeyOpts2(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgTmplEditorOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcExtraIconsOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

static UINT expertOnlyControls[]={IDC_ALWAYSSTATUS};
int CListOptInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;

	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=-1000000000;
	odp.hInstance=g_hInst;
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_CLIST);
	odp.pfnDlgProc=DlgProcGenOpts;
	odp.pszTitle=Translate("Contact List");
	odp.flags=ODPF_BOLDGROUPS;
	odp.nIDBottomSimpleControl=IDC_STCLISTGROUP;
	odp.expertOnlyControls=expertOnlyControls;
	odp.nExpertOnlyControls=sizeof(expertOnlyControls)/sizeof(expertOnlyControls[0]);
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=-200000000;
	odp.hInstance=g_hInst;
	odp.pfnDlgProc=DlgProcItemsOpts;
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_ITEMS);
	odp.pszGroup=Translate("Contact List");
	odp.pszTitle=Translate("Row items");
	odp.flags=ODPF_BOLDGROUPS;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	/*
	odp.position=-900000000;
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_HOTKEY);
	odp.pszTitle=Translate("Hotkeys");
	odp.pszGroup=Translate("Events");
	odp.pfnDlgProc=DlgProcHotkeyOpts;
	odp.nIDBottomSimpleControl=0;
	odp.nExpertOnlyControls=0;
	odp.expertOnlyControls=NULL;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	*/
	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=-200000000;
	odp.hInstance=g_hInst;
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_HOTKEYS);
	odp.pfnDlgProc=DlgProcHotKeyOpts2;
	odp.pszGroup=Translate("Events");
	odp.pszTitle=Translate("Hotkeys2");
	odp.flags=ODPF_BOLDGROUPS;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}

static BOOL CALLBACK DlgProcItemRowOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			int i;
			HWND hwndList;

			SendDlgItemMessage(hwndDlg,IDC_MIN_ROW_HEIGHT_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_MIN_ROW_HEIGHT),0);		// set buddy			
			SendDlgItemMessage(hwndDlg,IDC_MIN_ROW_HEIGHT_SPIN,UDM_SETRANGE,0,MAKELONG(255,0));
			SendDlgItemMessage(hwndDlg,IDC_MIN_ROW_HEIGHT_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","MinRowHeight",CLCDEFAULT_ROWHEIGHT),0));

			SendDlgItemMessage(hwndDlg,IDC_ROW_BORDER_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_ROW_BORDER),0);		// set buddy			
			SendDlgItemMessage(hwndDlg,IDC_ROW_BORDER_SPIN,UDM_SETRANGE,0,MAKELONG(255,0));
			SendDlgItemMessage(hwndDlg,IDC_ROW_BORDER_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","RowBorder",1),0));

			CheckDlgButton(hwndDlg, IDC_VARIABLE_ROW_HEIGHT, DBGetContactSettingByte(NULL,"CList","VariableRowHeight",1) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_ALIGN_TO_LEFT, DBGetContactSettingByte(NULL,"CList","AlignLeftItemsToLeft",1) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_ALIGN_TO_RIGHT, DBGetContactSettingByte(NULL,"CList","AlignRightItemsToRight",1) == 1 ? BST_CHECKED : BST_UNCHECKED );

			SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_LEFTMARGIN),0);		// set buddy			
			SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_SETRANGE,0,MAKELONG(64,0));
			SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingByte(NULL,"CLC","LeftMargin",CLCDEFAULT_LEFTMARGIN),0));

			SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_RIGHTMARGIN),0);		// set buddy			
			SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_SETRANGE,0,MAKELONG(64,0));
			SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingByte(NULL,"CLC","RightMargin",CLCDEFAULT_RIGHTMARGIN),0));

			// Listbox
			hwndList = GetDlgItem(hwndDlg, IDC_LIST_ORDER);

			for(i = 0 ; i < NUM_ITEM_TYPE ; i++)
			{
				char tmp[128];
				int type;
				int pos=0;

				mir_snprintf(tmp, sizeof(tmp), "RowPos%d", i);
				type = DBGetContactSettingWord(NULL, "CList", tmp, i);

				switch(type)
				{
				case ITEM_AVATAR:
					{
						pos = SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM) TranslateT("Avatar"));
						break;
					}
				case ITEM_ICON:
					{
						pos = SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM) TranslateT("Icon"));
						break;
					}
				case ITEM_TEXT:
					{
						pos = SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM) TranslateT("Text"));
						break;
					}
				case ITEM_EXTRA_ICONS:
					{
						pos = SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM) TranslateT("Extra icons"));
						break;
					}
				case ITEM_CONTACT_TIME:
					{
						pos = SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM) TranslateT("Contact time"));
						break;
					}
				}
				SendMessage(hwndList, LB_SETITEMDATA, pos, type);
			}

			// Buttons
			switch(SendMessage(hwndList, LB_GETCURSEL, 0, 0))
			{
			case LB_ERR:
				{
					EnableWindow(GetDlgItem(hwndDlg,IDC_UP),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_DOWN),FALSE);
					break;
				}
			case 0:
				{
					EnableWindow(GetDlgItem(hwndDlg,IDC_DOWN),FALSE);
					break;
				}
			case 3:
				{
					EnableWindow(GetDlgItem(hwndDlg,IDC_UP),FALSE);
					break;
				}
			}

			break;
		}
	case WM_COMMAND:
		{
			if (LOWORD(wParam)==IDC_UP)
			{
				if (HIWORD(wParam) == BN_CLICKED)
				{
					HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST_ORDER);
					int pos = SendMessage(hwndList, LB_GETCURSEL, 0, 0);

					if (pos != LB_ERR)
					{
						int type = SendMessage(hwndList, LB_GETITEMDATA, pos, 0);

						// Switch items
						SendMessage(hwndList, LB_DELETESTRING, pos, 0);

						switch(type)
						{
						case ITEM_AVATAR:
							{
								pos = SendMessage(hwndList, LB_INSERTSTRING, pos-1, (LPARAM) TranslateT("Avatar"));
								break;
							}
						case ITEM_ICON:
							{
								pos = SendMessage(hwndList, LB_INSERTSTRING, pos-1, (LPARAM) TranslateT("Icon"));
								break;
							}
						case ITEM_TEXT:
							{
								pos = SendMessage(hwndList, LB_INSERTSTRING, pos-1, (LPARAM) TranslateT("Text"));
								break;
							}
						case ITEM_EXTRA_ICONS:
							{
								pos = SendMessage(hwndList, LB_INSERTSTRING, pos-1, (LPARAM) TranslateT("Extra icons"));
								break;
							}
						case ITEM_CONTACT_TIME:
							{
								pos = SendMessage(hwndList, LB_INSERTSTRING, pos-1, (LPARAM) TranslateT("Contact time"));
								break;
							}
						}
						SendMessage(hwndList, LB_SETITEMDATA, pos, type);
						SendMessage(hwndList, LB_SETCURSEL, pos, 0);

						SendMessage(GetParent(GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
					}
					else return 0;
				}
				else return 0;
			}

			if (LOWORD(wParam)==IDC_DOWN)
			{
				if (HIWORD(wParam) == BN_CLICKED)
				{
					HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST_ORDER);
					int pos = SendMessage(hwndList, LB_GETCURSEL, 0, 0);

					if (pos != LB_ERR)
					{
						int type = SendMessage(hwndList, LB_GETITEMDATA, pos, 0);

						// Switch items
						SendMessage(hwndList, LB_DELETESTRING, pos, 0);

						switch(type)
						{
						case ITEM_AVATAR:
							{
								pos = SendMessage(hwndList, LB_INSERTSTRING, pos+1, (LPARAM) TranslateT("Avatar"));
								break;
							}
						case ITEM_ICON:
							{
								pos = SendMessage(hwndList, LB_INSERTSTRING, pos+1, (LPARAM) TranslateT("Icon"));
								break;
							}
						case ITEM_TEXT:
							{
								pos = SendMessage(hwndList, LB_INSERTSTRING, pos+1, (LPARAM) TranslateT("Text"));
								break;
							}
						case ITEM_EXTRA_ICONS:
							{
								pos = SendMessage(hwndList, LB_INSERTSTRING, pos+1, (LPARAM) TranslateT("Extra icons"));
								break;
							}
						case ITEM_CONTACT_TIME:
							{
								pos = SendMessage(hwndList, LB_INSERTSTRING, pos+1, (LPARAM) TranslateT("Contact time"));
								break;
							}
						}
						SendMessage(hwndList, LB_SETITEMDATA, pos, type);
						SendMessage(hwndList, LB_SETCURSEL, pos, 0);

						SendMessage(GetParent(GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
					}
					else return 0;
				}
				else return 0;
			}

			if (LOWORD(wParam)==IDC_LIST_ORDER || LOWORD(wParam)==IDC_UP || LOWORD(wParam)==IDC_DOWN)
			{
				int pos = SendMessage(GetDlgItem(hwndDlg, IDC_LIST_ORDER), LB_GETCURSEL, 0, 0);

				EnableWindow(GetDlgItem(hwndDlg,IDC_UP),pos != LB_ERR && pos > 0);
				EnableWindow(GetDlgItem(hwndDlg,IDC_DOWN),pos != LB_ERR && pos < 4);
			}

			if (LOWORD(wParam)==IDC_LIST_ORDER) return 0;
			if (LOWORD(wParam)==IDC_MIN_ROW_HEIGHT && HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0; // dont make apply enabled during buddy set crap
			if ((LOWORD(wParam)==IDC_LEFTMARGIN || LOWORD(wParam)==IDC_RIGHTMARGIN || LOWORD(wParam)==IDC_ROW_BORDER) && HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0; // dont make apply enabled during buddy set crap

			SendMessage(GetParent(GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
			break;
		}
	case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->idFrom) 
			{
			case 0:
				{
					switch (((LPNMHDR)lParam)->code)
					{
					case PSN_APPLY:
						{
							int i;
							HWND hwndList;

							DBWriteContactSettingWord(NULL,"CList","MinRowHeight",(WORD)SendDlgItemMessage(hwndDlg,IDC_MIN_ROW_HEIGHT_SPIN,UDM_GETPOS,0,0));
							DBWriteContactSettingWord(NULL,"CList","RowBorder",(WORD)SendDlgItemMessage(hwndDlg,IDC_ROW_BORDER_SPIN,UDM_GETPOS,0,0));
							DBWriteContactSettingByte(NULL,"CList","VariableRowHeight", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_VARIABLE_ROW_HEIGHT));
							DBWriteContactSettingByte(NULL,"CList","AlignLeftItemsToLeft", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_ALIGN_TO_LEFT));
							DBWriteContactSettingByte(NULL,"CList","AlignRightItemsToRight", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_ALIGN_TO_RIGHT));
							DBWriteContactSettingByte(NULL,"CLC","LeftMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_GETPOS,0,0));
							DBWriteContactSettingByte(NULL,"CLC","RightMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_GETPOS,0,0));

							hwndList = GetDlgItem(hwndDlg, IDC_LIST_ORDER);
							for(i = 0 ; i < NUM_ITEM_TYPE ; i++)
							{
								char tmp[128];
								mir_snprintf(tmp, sizeof(tmp), "RowPos%d", i);
								DBWriteContactSettingWord(NULL,"CList",tmp,(WORD)SendMessage(hwndList, LB_GETITEMDATA, i, 0));
							}

							return TRUE;
						}
					}
					break;
				}
			}
			break;
		}
	}

	return 0;
}

static BOOL CALLBACK DlgProcItemAvatarOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			CheckDlgButton(hwndDlg, IDC_SHOW_AVATARS, DBGetContactSettingByte(NULL,"CList","AvatarsShow",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_AVATAR_DRAW_BORDER, DBGetContactSettingByte(NULL,"CList","AvatarsDrawBorders",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_AVATAR_ROUND_CORNERS, DBGetContactSettingByte(NULL,"CList","AvatarsRoundCorners",1) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_AVATAR_CUSTOM_CORNER_SIZE_CHECK, DBGetContactSettingByte(NULL,"CList","AvatarsUseCustomCornerSize",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_AVATAR_IGNORE_SIZE, DBGetContactSettingByte(NULL,"CList","AvatarsIgnoreSizeForRow",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_AVATAR_OVERLAY_ICONS, DBGetContactSettingByte(NULL,"CList","AvatarsDrawOverlay",0) == 1 ? BST_CHECKED : BST_UNCHECKED );

			switch(DBGetContactSettingByte(NULL,"CList","AvatarsOverlayType",SETTING_AVATAR_OVERLAY_TYPE_NORMAL))
			{
			case SETTING_AVATAR_OVERLAY_TYPE_NORMAL:
				{
					CheckDlgButton(hwndDlg, IDC_AVATAR_OVERLAY_ICON_NORMAL, BST_CHECKED);
					break;
				}
			case SETTING_AVATAR_OVERLAY_TYPE_PROTOCOL:
				{
					CheckDlgButton(hwndDlg, IDC_AVATAR_OVERLAY_ICON_PROTOCOL, BST_CHECKED);
					break;
				}
			case SETTING_AVATAR_OVERLAY_TYPE_CONTACT:
				{
					CheckDlgButton(hwndDlg, IDC_AVATAR_OVERLAY_ICON_CONTACT, BST_CHECKED);
					break;
				}
			}

			SendDlgItemMessage(hwndDlg,IDC_AVATAR_SIZE_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_AVATAR_SIZE),0);		// set buddy			
			SendDlgItemMessage(hwndDlg,IDC_AVATAR_SIZE_SPIN,UDM_SETRANGE,0,MAKELONG(255,1));
			SendDlgItemMessage(hwndDlg,IDC_AVATAR_SIZE_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","AvatarsSize",30),0));

			SendDlgItemMessage(hwndDlg,IDC_AVATAR_WIDTH_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_AVATAR_WIDTH),0);		// set buddy			
			SendDlgItemMessage(hwndDlg,IDC_AVATAR_WIDTH_SPIN,UDM_SETRANGE,0,MAKELONG(255,0));
			SendDlgItemMessage(hwndDlg,IDC_AVATAR_WIDTH_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","AvatarsWidth",0),0));

			SendDlgItemMessage(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE),0);		// set buddy
			SendDlgItemMessage(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_SPIN,UDM_SETRANGE,0,MAKELONG(255,1));
			SendDlgItemMessage(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","AvatarsCustomCornerSize",4),0));

			SendDlgItemMessage(hwndDlg, IDC_AVATAR_BORDER_COLOR, CPM_SETCOLOUR, 0, (COLORREF)DBGetContactSettingDword(NULL,"CList","AvatarsBorderColor",0));

			if(!IsDlgButtonChecked(hwndDlg,IDC_SHOW_AVATARS)) 
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_DRAW_BORDER),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_BORDER_COLOR_L),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_BORDER_COLOR),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_ROUND_CORNERS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_IGNORE_SIZE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICONS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_NORMAL),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_PROTOCOL),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_CONTACT),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_L),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_SPIN),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_WIDTH_SPIN),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS2),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS3),FALSE);
			}
			if(!IsDlgButtonChecked(hwndDlg,IDC_AVATAR_DRAW_BORDER)) 
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_BORDER_COLOR_L),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_BORDER_COLOR),FALSE);
			}
			if(!IsDlgButtonChecked(hwndDlg,IDC_AVATAR_ROUND_CORNERS)) 
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_CHECK),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_SPIN),FALSE);
			}  
			if (!IsDlgButtonChecked(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_CHECK))
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_SPIN),FALSE);
			}	
			if(!IsDlgButtonChecked(hwndDlg,IDC_AVATAR_OVERLAY_ICONS)) 
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_NORMAL),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_PROTOCOL),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_CONTACT),FALSE);
			}
			break;
		}
	case WM_COMMAND:
		{
			if (LOWORD(wParam)==IDC_SHOW_AVATARS)
			{
				BOOL enabled = IsDlgButtonChecked(hwndDlg,IDC_SHOW_AVATARS);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_DRAW_BORDER),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_BORDER_COLOR_L),enabled && IsDlgButtonChecked(hwndDlg,IDC_AVATAR_DRAW_BORDER));
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_BORDER_COLOR),enabled && IsDlgButtonChecked(hwndDlg,IDC_AVATAR_DRAW_BORDER));
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_ROUND_CORNERS),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_IGNORE_SIZE),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICONS),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_NORMAL),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_PROTOCOL),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_CONTACT),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_L),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_SPIN),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS),enabled);
				if (DBGetContactSettingByte(NULL,"ModernData","UseAdvancedRowLayout",0)==1)
				{
					EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_WIDTH_SPIN),enabled);
					EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS2),enabled);
					EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS3),enabled);
				}
				else
				{	EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_WIDTH_SPIN),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS2),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS3),FALSE);
				}
			}

			if (LOWORD(wParam)==IDC_AVATAR_DRAW_BORDER)
			{
				BOOL enabled = IsDlgButtonChecked(hwndDlg,IDC_SHOW_AVATARS) && IsDlgButtonChecked(hwndDlg,IDC_AVATAR_DRAW_BORDER);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_BORDER_COLOR_L),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_BORDER_COLOR),enabled);
			}

			if (LOWORD(wParam)==IDC_AVATAR_ROUND_CORNERS)
			{
				BOOL enabled = IsDlgButtonChecked(hwndDlg,IDC_SHOW_AVATARS) && IsDlgButtonChecked(hwndDlg,IDC_AVATAR_ROUND_CORNERS);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_CHECK),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_SPIN),enabled);
			}

			if (LOWORD(wParam)==IDC_AVATAR_CUSTOM_CORNER_SIZE_CHECK)
			{
				BOOL enabled = IsDlgButtonChecked(hwndDlg,IDC_AVATAR_ROUND_CORNERS);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_CHECK),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE),enabled&&IsDlgButtonChecked(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_CHECK));
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_SPIN),enabled&&IsDlgButtonChecked(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_CHECK));
			}
			if(LOWORD(wParam)==IDC_AVATAR_OVERLAY_ICONS) 
			{
				BOOL enabled = IsDlgButtonChecked(hwndDlg,IDC_SHOW_AVATARS) && IsDlgButtonChecked(hwndDlg,IDC_AVATAR_OVERLAY_ICONS);

				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_NORMAL),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_PROTOCOL),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_CONTACT),enabled);
			}
			if (LOWORD(wParam)==IDC_AVATAR_SIZE && HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0; // dont make apply enabled during buddy set crap
			if (LOWORD(wParam)==IDC_AVATAR_CUSTOM_CORNER_SIZE && HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0; // dont make apply enabled during buddy set crap

			SendMessage(GetParent(GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
			break;
		}
	case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->idFrom) 
			{
			case 0:
				{
					switch (((LPNMHDR)lParam)->code)
					{
					case PSN_APPLY:
						{
							DBWriteContactSettingByte(NULL,"CList","AvatarsShow", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOW_AVATARS));
							DBWriteContactSettingByte(NULL,"CList","AvatarsDrawBorders", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_AVATAR_DRAW_BORDER));
							DBWriteContactSettingDword(NULL,"CList","AvatarsBorderColor", (DWORD)SendDlgItemMessage(hwndDlg, IDC_AVATAR_BORDER_COLOR, CPM_GETCOLOUR, 0, 0));
							DBWriteContactSettingByte(NULL,"CList","AvatarsRoundCorners", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_AVATAR_ROUND_CORNERS));
							DBWriteContactSettingByte(NULL,"CList","AvatarsIgnoreSizeForRow", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_AVATAR_IGNORE_SIZE));
							DBWriteContactSettingByte(NULL,"CList","AvatarsUseCustomCornerSize", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_CHECK));
							DBWriteContactSettingWord(NULL,"CList","AvatarsCustomCornerSize",(WORD)SendDlgItemMessage(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_SPIN,UDM_GETPOS,0,0));
							DBWriteContactSettingByte(NULL,"CList","AvatarsDrawOverlay", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_AVATAR_OVERLAY_ICONS));
							DBWriteContactSettingWord(NULL,"CList","AvatarsSize",(WORD)SendDlgItemMessage(hwndDlg,IDC_AVATAR_SIZE_SPIN,UDM_GETPOS,0,0));
							DBWriteContactSettingWord(NULL,"CList","AvatarsWidth",(WORD)SendDlgItemMessage(hwndDlg,IDC_AVATAR_WIDTH_SPIN,UDM_GETPOS,0,0));

							if (IsDlgButtonChecked(hwndDlg,IDC_AVATAR_OVERLAY_ICON_NORMAL))
								DBWriteContactSettingByte(NULL,"CList","AvatarsOverlayType",SETTING_AVATAR_OVERLAY_TYPE_NORMAL);
							else if (IsDlgButtonChecked(hwndDlg,IDC_AVATAR_OVERLAY_ICON_PROTOCOL))
								DBWriteContactSettingByte(NULL,"CList","AvatarsOverlayType",SETTING_AVATAR_OVERLAY_TYPE_PROTOCOL);
							else if (IsDlgButtonChecked(hwndDlg,IDC_AVATAR_OVERLAY_ICON_CONTACT))
								DBWriteContactSettingByte(NULL,"CList","AvatarsOverlayType",SETTING_AVATAR_OVERLAY_TYPE_CONTACT);

							return TRUE;
						}
					}
					break;
				}
			}
			break;
		}
	}

	return 0;
}


static BOOL CALLBACK DlgProcItemIconOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			CheckDlgButton(hwndDlg, IDC_HIDE_ICON_ON_AVATAR, DBGetContactSettingByte(NULL,"CList","IconHideOnAvatar",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_DRAW_ON_AVATAR_SPACE, DBGetContactSettingByte(NULL,"CList","IconDrawOnAvatarSpace",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_ICONBLINK, DBGetContactSettingByte(NULL,"CList","NoIconBlink",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_HIDE_GROUPSICON, DBGetContactSettingByte(NULL,"CList","HideGroupsIcon",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_NOTCHECKICONSIZE, DBGetContactSettingByte(NULL,"CList","IconIgnoreSizeForRownHeight",0) == 1 ? BST_CHECKED : BST_UNCHECKED );

			if (!IsDlgButtonChecked(hwndDlg,IDC_HIDE_ICON_ON_AVATAR))
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_DRAW_ON_AVATAR_SPACE),FALSE);
			}

			break;
		}
	case WM_COMMAND:
		{
			if (LOWORD(wParam)==IDC_HIDE_ICON_ON_AVATAR)
			{
				BOOL enabled = IsDlgButtonChecked(hwndDlg,IDC_HIDE_ICON_ON_AVATAR);
				EnableWindow(GetDlgItem(hwndDlg,IDC_DRAW_ON_AVATAR_SPACE),enabled);
			}

			SendMessage(GetParent(GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
			break;
		}
	case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->idFrom) 
			{
			case 0:
				{
					switch (((LPNMHDR)lParam)->code)
					{
					case PSN_APPLY:
						{
							DBWriteContactSettingByte(NULL,"CList","IconHideOnAvatar", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_HIDE_ICON_ON_AVATAR));
							DBWriteContactSettingByte(NULL,"CList","IconDrawOnAvatarSpace", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_DRAW_ON_AVATAR_SPACE));
							DBWriteContactSettingByte(NULL,"CList","HideGroupsIcon", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_HIDE_GROUPSICON));						
							DBWriteContactSettingByte(NULL,"CList","NoIconBlink", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_ICONBLINK));
							DBWriteContactSettingByte(NULL,"CList","IconIgnoreSizeForRownHeight", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_NOTCHECKICONSIZE));

							return TRUE;
						}
					}
					break;
				}
			}
			break;
		}
	}

	return 0;
}


static BOOL CALLBACK DlgProcItemContactTimeOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			CheckDlgButton(hwndDlg, IDC_SHOW, DBGetContactSettingByte(NULL,"CList","ContactTimeShow",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_SHOW_ONLY_IF_DIFFERENT, DBGetContactSettingByte(NULL,"CList","ContactTimeShowOnlyIfDifferent",1) == 1 ? BST_CHECKED : BST_UNCHECKED );

			break;
		}
	case WM_COMMAND:
		{
			SendMessage(GetParent(GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
			break;
		}
	case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->idFrom) 
			{
			case 0:
				{
					switch (((LPNMHDR)lParam)->code)
					{
					case PSN_APPLY:
						{
							DBWriteContactSettingByte(NULL,"CList","ContactTimeShow", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOW));
							DBWriteContactSettingByte(NULL,"CList","ContactTimeShowOnlyIfDifferent", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOW_ONLY_IF_DIFFERENT));

							return TRUE;
						}
					}
					break;
				}
			}
			break;
		}
	}

	return 0;
}


static BOOL CALLBACK DlgProcItemTextOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			CheckDlgButton(hwndDlg, IDC_RTL, DBGetContactSettingByte(NULL,"CList","TextRTL",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
			//TODO: init IDC_ALIGNGROUPCOMBO from DBGetContactSettingByte(NULL,"CList","AlignGroupCaptions",0);
			{
				int i, item;
				TCHAR *align[]={_T("Left align group names"), _T("Center group names"), _T("Right align group names")};
				for (i=0; i<sizeof(align)/sizeof(char*); i++) 
					item=SendDlgItemMessage(hwndDlg,IDC_ALIGNGROUPCOMBO,CB_ADDSTRING,0,(LPARAM)TranslateTS(align[i]));
				SendDlgItemMessage(hwndDlg,IDC_ALIGNGROUPCOMBO,CB_SETCURSEL,DBGetContactSettingByte(NULL,"CList","AlignGroupCaptions",0),0);
			}
			CheckDlgButton(hwndDlg, IDC_ALIGN_RIGHT, DBGetContactSettingByte(NULL,"CList","TextAlignToRight",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_REPLACE_SMILEYS, DBGetContactSettingByte(NULL,"CList","TextReplaceSmileys",1) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_RESIZE_SMILEYS, DBGetContactSettingByte(NULL,"CList","TextResizeSmileys",1) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_USE_PROTOCOL_SMILEYS, DBGetContactSettingByte(NULL,"CList","TextUseProtocolSmileys",1) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_IGNORE_SIZE, DBGetContactSettingByte(NULL,"CList","TextIgnoreSizeForRownHeight",0) == 1 ? BST_CHECKED : BST_UNCHECKED );

			CheckDlgButton(hwndDlg, IDC_DRAW_SMILEYS_ON_FIRST_LINE, DBGetContactSettingByte(NULL,"CList","FirstLineDrawSmileys",1) == 1 ? BST_CHECKED : BST_UNCHECKED );

			ShowWindowNew(GetDlgItem(hwndDlg,IDC_REPLACE_SMILEYS), ServiceExists(MS_SMILEYADD_PARSE) ? SW_SHOW : SW_HIDE);
			ShowWindowNew(GetDlgItem(hwndDlg,IDC_USE_PROTOCOL_SMILEYS), ServiceExists(MS_SMILEYADD_PARSE) ? SW_SHOW : SW_HIDE);
			ShowWindowNew(GetDlgItem(hwndDlg,IDC_RESIZE_SMILEYS), ServiceExists(MS_SMILEYADD_PARSE) ? SW_SHOW : SW_HIDE);
			ShowWindowNew(GetDlgItem(hwndDlg,IDC_DRAW_SMILEYS_ON_FIRST_LINE), ServiceExists(MS_SMILEYADD_PARSE) ? SW_SHOW : SW_HIDE);

			if (!IsDlgButtonChecked(hwndDlg,IDC_REPLACE_SMILEYS))
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_PROTOCOL_SMILEYS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_RESIZE_SMILEYS),FALSE);
			}

			break;
		}
	case WM_COMMAND:
		{
			if (LOWORD(wParam)==IDC_REPLACE_SMILEYS)
			{
				BOOL enabled = IsDlgButtonChecked(hwndDlg,IDC_REPLACE_SMILEYS);
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_PROTOCOL_SMILEYS),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_RESIZE_SMILEYS),enabled);
			}      
			if (LOWORD(wParam)!=IDC_ALIGNGROUPCOMBO || (LOWORD(wParam)==IDC_ALIGNGROUPCOMBO && HIWORD(wParam)==CBN_SELCHANGE))
				SendMessage(GetParent(GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
			break;
		}
	case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->idFrom) 
			{
			case 0:
				{
					switch (((LPNMHDR)lParam)->code)
					{
					case PSN_APPLY:
						{
							DBWriteContactSettingByte(NULL,"CList","TextRTL", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_RTL));
							//TODO: Store IDC_ALIGNGROUPCOMBO at DBGetContactSettingByte(NULL,"CList","AlignGroupCaptions",0);
							DBWriteContactSettingByte(NULL,"CList","AlignGroupCaptions",(BYTE)SendDlgItemMessage(hwndDlg,IDC_ALIGNGROUPCOMBO,CB_GETCURSEL,0,0));
							DBWriteContactSettingByte(NULL,"CList","TextAlignToRight", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_ALIGN_RIGHT));
							DBWriteContactSettingByte(NULL,"CList","TextReplaceSmileys", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_REPLACE_SMILEYS));
							DBWriteContactSettingByte(NULL,"CList","TextResizeSmileys", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_RESIZE_SMILEYS));
							DBWriteContactSettingByte(NULL,"CList","TextUseProtocolSmileys", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_USE_PROTOCOL_SMILEYS));
							DBWriteContactSettingByte(NULL,"CList","TextIgnoreSizeForRownHeight", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_IGNORE_SIZE));
							DBWriteContactSettingByte(NULL,"CList","FirstLineDrawSmileys", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_DRAW_SMILEYS_ON_FIRST_LINE));

							return TRUE;
						}
					}
					break;
				}
			}
			break;
		}
	}

	return 0;
}


static BOOL CALLBACK DlgProcItemSecondLineOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			int radio;

			CheckDlgButton(hwndDlg, IDC_SHOW, DBGetContactSettingByte(NULL,"CList","SecondLineShow",1) == 1 ? BST_CHECKED : BST_UNCHECKED );

			SendDlgItemMessage(hwndDlg,IDC_TOP_SPACE_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_TOP_SPACE),0);		// set buddy			
			SendDlgItemMessage(hwndDlg,IDC_TOP_SPACE_SPIN,UDM_SETRANGE,0,MAKELONG(255,0));
			SendDlgItemMessage(hwndDlg,IDC_TOP_SPACE_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","SecondLineTopSpace",2),0));

			CheckDlgButton(hwndDlg, IDC_DRAW_SMILEYS, DBGetContactSettingByte(NULL,"CList","SecondLineDrawSmileys",1) == 1 ? BST_CHECKED : BST_UNCHECKED);

			{
				DBVARIANT dbv={0};

				if (!DBGetContactSettingTString(NULL, "CList","SecondLineText", &dbv))
				{
					SetWindowText(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT), dbv.ptszVal);
					DBFreeVariant(&dbv);
				}
			}
			SendMessage(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT), EM_SETLIMITTEXT, TEXT_TEXT_MAX_LENGTH, 0);

			radio = DBGetContactSettingWord(NULL,"CList","SecondLineType",TEXT_STATUS_MESSAGE);

			CheckDlgButton(hwndDlg, IDC_STATUS, radio == TEXT_STATUS ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_NICKNAME, radio == TEXT_NICKNAME ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_STATUS_MESSAGE, radio == TEXT_STATUS_MESSAGE ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_CONTACT_TIME, radio == TEXT_CONTACT_TIME ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_TEXT, radio == TEXT_TEXT ? BST_CHECKED : BST_UNCHECKED );

			CheckDlgButton(hwndDlg, IDC_XSTATUS_HAS_PRIORITY, DBGetContactSettingByte(NULL,"CList","SecondLineXStatusHasPriority",1) == 1 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOW_STATUS_IF_NOAWAY, DBGetContactSettingByte(NULL,"CList","SecondLineShowStatusIfNoAway",0) == 1 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_USE_NAME_AND_MESSAGE, DBGetContactSettingByte(NULL,"CList","SecondLineUseNameAndMessageForXStatus",0) == 1 ? BST_CHECKED : BST_UNCHECKED);

			if (!IsDlgButtonChecked(hwndDlg,IDC_SHOW))
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_PROTOCOL_SMILEYS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOP_SPACE_SPIN),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOP_SPACE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_DRAW_SMILEYS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATUS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_NICKNAME),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATUS_MESSAGE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_CONTACT_TIME),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TEXT),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_TOP),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_PIXELS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_TEXT),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLES_L),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_STATUS_IF_NOAWAY),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_XSTATUS_HAS_PRIORITY),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_NAME_AND_MESSAGE),FALSE);
			}
			else
			{
				if (!IsDlgButtonChecked(hwndDlg,IDC_TEXT))
				{
					EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLES_L),FALSE);
				}
				if (!IsDlgButtonChecked(hwndDlg,IDC_STATUS) && !IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE))
				{
					EnableWindow(GetDlgItem(hwndDlg,IDC_XSTATUS_HAS_PRIORITY),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_USE_NAME_AND_MESSAGE),FALSE);
				}
				if (!IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE))
					EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_STATUS_IF_NOAWAY),FALSE);
			}

			ShowWindowNew(GetDlgItem(hwndDlg,IDC_DRAW_SMILEYS), ServiceExists(MS_SMILEYADD_PARSE) ? SW_SHOW : SW_HIDE);
			ShowWindowNew(GetDlgItem(hwndDlg,IDC_VARIABLES_L), ServiceExists(MS_VARS_FORMATSTRING) ? SW_SHOW : SW_HIDE);

			break;
		}
	case WM_COMMAND:
		{
			if (LOWORD(wParam)==IDC_TEXT || LOWORD(wParam)==IDC_STATUS || LOWORD(wParam)==IDC_NICKNAME || LOWORD(wParam)==IDC_STATUS_MESSAGE
				 || LOWORD(wParam)==IDC_CONTACT_TIME)
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT), IsDlgButtonChecked(hwndDlg,IDC_TEXT) 
					&& IsDlgButtonChecked(hwndDlg,IDC_SHOW));
				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLES_L), IsDlgButtonChecked(hwndDlg,IDC_TEXT) 
					&& IsDlgButtonChecked(hwndDlg,IDC_SHOW));
				EnableWindow(GetDlgItem(hwndDlg,IDC_XSTATUS_HAS_PRIORITY), IsDlgButtonChecked(hwndDlg,IDC_SHOW) && 
					(IsDlgButtonChecked(hwndDlg,IDC_STATUS) 
					|| IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE)));
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_STATUS_IF_NOAWAY), IsDlgButtonChecked(hwndDlg,IDC_SHOW) && 
					(IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE)));
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_NAME_AND_MESSAGE), IsDlgButtonChecked(hwndDlg,IDC_SHOW) && 
					(IsDlgButtonChecked(hwndDlg,IDC_STATUS) 
					|| IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE)));
			}
			else if (LOWORD(wParam)==IDC_SHOW)
			{
				BOOL enabled = IsDlgButtonChecked(hwndDlg,IDC_SHOW);
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_PROTOCOL_SMILEYS),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOP_SPACE_SPIN),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOP_SPACE),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_DRAW_SMILEYS),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATUS),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_NICKNAME),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_CONTACT_TIME),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATUS_MESSAGE),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TEXT),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT),enabled && IsDlgButtonChecked(hwndDlg,IDC_TEXT));
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_TOP),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_PIXELS),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_TEXT),enabled);

				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT), enabled && IsDlgButtonChecked(hwndDlg,IDC_TEXT));
				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLES_L), enabled && IsDlgButtonChecked(hwndDlg,IDC_TEXT));
				EnableWindow(GetDlgItem(hwndDlg,IDC_XSTATUS_HAS_PRIORITY), enabled && (IsDlgButtonChecked(hwndDlg,IDC_STATUS) 
					|| IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE)));
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_STATUS_IF_NOAWAY), enabled && IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE));
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_NAME_AND_MESSAGE), enabled && (IsDlgButtonChecked(hwndDlg,IDC_STATUS) 
					|| IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE)));			
			}

			if (LOWORD(wParam)==IDC_TOP_SPACE && HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0; // dont make apply enabled during buddy set crap

			SendMessage(GetParent(GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
			break;
		}
	case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->idFrom) 
			{
			case 0:
				{
					switch (((LPNMHDR)lParam)->code)
					{
					case PSN_APPLY:
						{
							int radio;

							DBWriteContactSettingByte(NULL,"CList","SecondLineShow", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOW));
							DBWriteContactSettingWord(NULL,"CList","SecondLineTopSpace", (WORD)SendDlgItemMessage(hwndDlg,IDC_TOP_SPACE_SPIN,UDM_GETPOS,0,0));
							DBWriteContactSettingByte(NULL,"CList","SecondLineDrawSmileys", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_DRAW_SMILEYS));

							if (IsDlgButtonChecked(hwndDlg,IDC_STATUS))
								radio = TEXT_STATUS;
							else if (IsDlgButtonChecked(hwndDlg,IDC_NICKNAME))
								radio = TEXT_NICKNAME;
							else if (IsDlgButtonChecked(hwndDlg,IDC_TEXT))
								radio = TEXT_TEXT;
							else if (IsDlgButtonChecked(hwndDlg,IDC_CONTACT_TIME))
								radio = TEXT_CONTACT_TIME;
							else
								radio = TEXT_STATUS_MESSAGE;
							DBWriteContactSettingWord(NULL,"CList","SecondLineType", (WORD)radio);

							{
								TCHAR t[TEXT_TEXT_MAX_LENGTH];

								GetWindowText(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT), t, sizeof(t));
								t[TEXT_TEXT_MAX_LENGTH - 1] = '\0';

								DBWriteContactSettingTString(NULL, "CList", "SecondLineText", t);
							}

							DBWriteContactSettingByte(NULL,"CList","SecondLineXStatusHasPriority", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_XSTATUS_HAS_PRIORITY));
							DBWriteContactSettingByte(NULL,"CList","SecondLineShowStatusIfNoAway", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOW_STATUS_IF_NOAWAY));
							DBWriteContactSettingByte(NULL,"CList","SecondLineUseNameAndMessageForXStatus", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_USE_NAME_AND_MESSAGE));

							return TRUE;
						}
					}
					break;
				}
			}
			break;
		}
	}

	return 0;
}

static BOOL CALLBACK DlgProcItemThirdLineOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			int radio;

			CheckDlgButton(hwndDlg, IDC_SHOW, DBGetContactSettingByte(NULL,"CList","ThirdLineShow",0) == 1 ? BST_CHECKED : BST_UNCHECKED );

			SendDlgItemMessage(hwndDlg,IDC_TOP_SPACE_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_TOP_SPACE),0);		// set buddy			
			SendDlgItemMessage(hwndDlg,IDC_TOP_SPACE_SPIN,UDM_SETRANGE,0,MAKELONG(255,0));
			SendDlgItemMessage(hwndDlg,IDC_TOP_SPACE_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","ThirdLineTopSpace",2),0));

			CheckDlgButton(hwndDlg, IDC_DRAW_SMILEYS, DBGetContactSettingByte(NULL,"CList","ThirdLineDrawSmileys",0) == 1 ? BST_CHECKED : BST_UNCHECKED );

			{
				DBVARIANT dbv={0};

				if (!DBGetContactSettingTString(NULL, "CList","ThirdLineText", &dbv))
				{
					SetWindowText(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT), dbv.ptszVal);
					DBFreeVariant(&dbv);
				}
			}
			SendMessage(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT), EM_SETLIMITTEXT, TEXT_TEXT_MAX_LENGTH, 0);

			radio = DBGetContactSettingWord(NULL,"CList","ThirdLineType",TEXT_STATUS);

			CheckDlgButton(hwndDlg, IDC_STATUS, radio == TEXT_STATUS ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_NICKNAME, radio == TEXT_NICKNAME ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_STATUS_MESSAGE, radio == TEXT_STATUS_MESSAGE ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_CONTACT_TIME, radio == TEXT_CONTACT_TIME ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_TEXT, radio == TEXT_TEXT ? BST_CHECKED : BST_UNCHECKED );

			CheckDlgButton(hwndDlg, IDC_XSTATUS_HAS_PRIORITY, DBGetContactSettingByte(NULL,"CList","ThirdLineXStatusHasPriority",1) == 1 ? BST_CHECKED : BST_UNCHECKED);

			CheckDlgButton(hwndDlg, IDC_SHOW_STATUS_IF_NOAWAY, DBGetContactSettingByte(NULL,"CList","ThirdLineShowStatusIfNoAway",0) == 1 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_USE_NAME_AND_MESSAGE, DBGetContactSettingByte(NULL,"CList","ThirdLineUseNameAndMessageForXStatus",0) == 1 ? BST_CHECKED : BST_UNCHECKED);

			if (!IsDlgButtonChecked(hwndDlg,IDC_SHOW))
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_PROTOCOL_SMILEYS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOP_SPACE_SPIN),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOP_SPACE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_DRAW_SMILEYS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATUS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_NICKNAME),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATUS_MESSAGE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_CONTACT_TIME),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TEXT),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_TOP),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_PIXELS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_TEXT),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLES_L),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_XSTATUS_HAS_PRIORITY),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_NAME_AND_MESSAGE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_STATUS_IF_NOAWAY),FALSE);
			}
			else
			{
				if (!IsDlgButtonChecked(hwndDlg,IDC_TEXT))
				{
					EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLES_L),FALSE);
				}
				if (!IsDlgButtonChecked(hwndDlg,IDC_STATUS) && !IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE))
				{
					EnableWindow(GetDlgItem(hwndDlg,IDC_XSTATUS_HAS_PRIORITY),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_USE_NAME_AND_MESSAGE),FALSE);
				}
				if (!IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE))
					EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_STATUS_IF_NOAWAY),FALSE);
			}

			ShowWindowNew(GetDlgItem(hwndDlg,IDC_DRAW_SMILEYS), ServiceExists(MS_SMILEYADD_PARSE) ? SW_SHOW : SW_HIDE);
			ShowWindowNew(GetDlgItem(hwndDlg,IDC_VARIABLES_L), ServiceExists(MS_VARS_FORMATSTRING) ? SW_SHOW : SW_HIDE);

			break;
		}
	case WM_COMMAND:
		{
			if (LOWORD(wParam)==IDC_TEXT || LOWORD(wParam)==IDC_STATUS || LOWORD(wParam)==IDC_NICKNAME || LOWORD(wParam)==IDC_STATUS_MESSAGE
				 || LOWORD(wParam)==IDC_CONTACT_TIME)
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT), IsDlgButtonChecked(hwndDlg,IDC_TEXT) 
					&& IsDlgButtonChecked(hwndDlg,IDC_SHOW));
				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLES_L), IsDlgButtonChecked(hwndDlg,IDC_TEXT) 
					&& IsDlgButtonChecked(hwndDlg,IDC_SHOW));
				EnableWindow(GetDlgItem(hwndDlg,IDC_XSTATUS_HAS_PRIORITY), IsDlgButtonChecked(hwndDlg,IDC_SHOW) && 
					(IsDlgButtonChecked(hwndDlg,IDC_STATUS) 
					|| IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE)));
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_NAME_AND_MESSAGE), IsDlgButtonChecked(hwndDlg,IDC_SHOW) && (IsDlgButtonChecked(hwndDlg,IDC_STATUS) || IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE)));
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_STATUS_IF_NOAWAY), IsDlgButtonChecked(hwndDlg,IDC_SHOW) && (IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE)));																	
			}
			else if (LOWORD(wParam)==IDC_SHOW)
			{
				BOOL enabled = IsDlgButtonChecked(hwndDlg,IDC_SHOW);
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_PROTOCOL_SMILEYS),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOP_SPACE_SPIN),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOP_SPACE),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_DRAW_SMILEYS),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATUS),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_NICKNAME),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATUS_MESSAGE),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_CONTACT_TIME),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TEXT),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT),enabled && IsDlgButtonChecked(hwndDlg,IDC_TEXT));
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_TOP),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_PIXELS),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_TEXT),enabled);

				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT), enabled && IsDlgButtonChecked(hwndDlg,IDC_TEXT));
				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLES_L), enabled && IsDlgButtonChecked(hwndDlg,IDC_TEXT));
				EnableWindow(GetDlgItem(hwndDlg,IDC_XSTATUS_HAS_PRIORITY), enabled && (IsDlgButtonChecked(hwndDlg,IDC_STATUS) 
					|| IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE)));
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_NAME_AND_MESSAGE), enabled && (IsDlgButtonChecked(hwndDlg,IDC_STATUS) 
					|| IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE)));
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_STATUS_IF_NOAWAY), enabled && IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE));
			}

			if (LOWORD(wParam)==IDC_TOP_SPACE && HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0; // dont make apply enabled during buddy set crap

			SendMessage(GetParent(GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
			break;
		}
	case WM_NOTIFY:
		{
			switch (((LPNMHDR)lParam)->idFrom) 
			{
			case 0:
				{
					switch (((LPNMHDR)lParam)->code)
					{
					case PSN_APPLY:
						{
							int radio;

							DBWriteContactSettingByte(NULL,"CList","ThirdLineShow", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOW));
							DBWriteContactSettingWord(NULL,"CList","ThirdLineTopSpace", (WORD)SendDlgItemMessage(hwndDlg,IDC_TOP_SPACE_SPIN,UDM_GETPOS,0,0));
							DBWriteContactSettingByte(NULL,"CList","ThirdLineDrawSmileys", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_DRAW_SMILEYS));

							if (IsDlgButtonChecked(hwndDlg,IDC_STATUS))
								radio = TEXT_STATUS;
							else if (IsDlgButtonChecked(hwndDlg,IDC_NICKNAME))
								radio = TEXT_NICKNAME;
							else if (IsDlgButtonChecked(hwndDlg,IDC_TEXT))
								radio = TEXT_TEXT;
							else if (IsDlgButtonChecked(hwndDlg,IDC_CONTACT_TIME))
								radio = TEXT_CONTACT_TIME;
							else
								radio = TEXT_STATUS_MESSAGE;
							DBWriteContactSettingWord(NULL,"CList","ThirdLineType", (WORD)radio);

							{
								TCHAR t[TEXT_TEXT_MAX_LENGTH];

								GetWindowText(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT), t, sizeof(t));
								t[TEXT_TEXT_MAX_LENGTH - 1] = '\0';

								DBWriteContactSettingTString(NULL, "CList", "ThirdLineText", t);
							}

							DBWriteContactSettingByte(NULL,"CList","ThirdLineXStatusHasPriority", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_XSTATUS_HAS_PRIORITY));
							DBWriteContactSettingByte(NULL,"CList","ThirdLineUseNameAndMessageForXStatus", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_USE_NAME_AND_MESSAGE));
							DBWriteContactSettingByte(NULL,"CList","ThirdLineShowStatusIfNoAway", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOW_STATUS_IF_NOAWAY));

							return TRUE;
						}
					}
					break;
				}
			}
			break;
		}
	}

	return 0;
}


#define NUM_ITEM_OPTION_PAGES 9

typedef struct _ItemOptionConf 
{ 
	TCHAR *name;				// Tab name
	int id;					// Dialog id
	DLGPROC wnd_proc;		// Dialog function
} ItemOptionConf;

typedef struct _ItemOptionData 
{ 
	ItemOptionConf *conf;
	HWND hwnd;				// dialog handle
} ItemOptionData; 

typedef struct _WndItemsData 
{ 
	ItemOptionData items[NUM_ITEM_OPTION_PAGES];
	HWND hwndDisplay;
	int selected_item;
} WndItemsData; 

static ItemOptionConf opt_items[] = { 
{ _T("Row"), IDD_OPT_ITEM_ROW, DlgProcItemRowOpts },
{ _T("Row design"), IDD_OPT_ROWTMPL, DlgTmplEditorOpts },
{ _T("Avatar"), IDD_OPT_ITEM_AVATAR, DlgProcItemAvatarOpts },
{ _T("Icon"), IDD_OPT_ITEM_ICON, DlgProcItemIconOpts },
{ _T("Contact time"), IDD_OPT_ITEM_CONTACT_TIME, DlgProcItemContactTimeOpts },
{ _T("Text"), IDD_OPT_ITEM_TEXT, DlgProcItemTextOpts },
{ _T("Second Line"), IDD_OPT_ITEM_SECOND_LINE, DlgProcItemSecondLineOpts },
{ _T("Third Line"), IDD_OPT_ITEM_THIRD_LINE, DlgProcItemThirdLineOpts },
{ _T("Extra Icons"), IDD_OPT_ITEM_EXTRAICONS, DlgProcExtraIconsOpts}
};




// DoLockDlgRes - loads and locks a dialog template resource. 
// Returns the address of the locked resource. 
// lpszResName - name of the resource 

static DLGTEMPLATE * DoLockDlgRes(LPCTSTR lpszResName) 
{ 
	HRSRC hrsrc = FindResource(g_hInst, lpszResName, MAKEINTRESOURCE(5)); 
	HGLOBAL hglb = LoadResource(g_hInst, hrsrc); 
	return (DLGTEMPLATE *) LockResource(hglb); 
} 

static void ChangeTab(HWND hwndDlg, WndItemsData *data, int sel)
{
	HWND hwndTab;
	RECT rc_tab;
	RECT rc_item;
	int top;

	hwndTab = GetDlgItem(hwndDlg, IDC_TAB);

	// Get avaible space
	GetWindowRect(hwndTab, &rc_tab);
	ScreenToClientRect(hwndDlg, &rc_tab);
	top = rc_tab.top;

	GetWindowRect(hwndTab, &rc_tab);
	ScreenToClientRect(hwndTab, &rc_tab);
	TabCtrl_AdjustRect(hwndTab, FALSE, &rc_tab); 




	// Get item size
	GetClientRect(data->items[sel].hwnd, &rc_item);

	// Fix rc_item
	rc_item.right -= rc_item.left;	// width
	rc_item.left = 0;
	rc_item.bottom -= rc_item.top;	// height
	rc_item.top = 0;

	//OffsetRect(&rc_item,30,0);

	if (rc_item.right < rc_tab.right - rc_tab.left)
		rc_item.left = rc_tab.left + (rc_tab.right - rc_tab.left - rc_item.right) / 2;
	else
		rc_item.left = rc_tab.left;

	if (rc_item.bottom < rc_tab.bottom - rc_tab.top)
		rc_item.top = top + rc_tab.top + (rc_tab.bottom - rc_tab.top - rc_item.bottom) / 2;
	else
		rc_item.top = top + rc_tab.top;

	// Set pos
	SetWindowPos(data->items[sel].hwnd, HWND_TOP, rc_item.left, rc_item.top, rc_item.right,
		rc_item.bottom, SWP_SHOWWINDOW);

	data->selected_item = sel;
}

static BOOL CALLBACK DlgProcItemsOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			HWND hwndTab;
			WndItemsData *data;
			int i;
			TCITEM tie; 
			RECT rc_tab;

			TranslateDialogDefault(hwndDlg);

			data = (WndItemsData *) mir_alloc(sizeof(WndItemsData));
			data->selected_item = -1;
			SetWindowLong(hwndDlg, GWL_USERDATA, (long) data); 

			hwndTab = GetDlgItem(hwndDlg, IDC_TAB);

			// Add tabs
			tie.mask = TCIF_TEXT | TCIF_IMAGE; 
			tie.iImage = -1; 

			for (i = 0 ; i < NUM_ITEM_OPTION_PAGES ; i++)
			{
				DLGTEMPLATE *templ;

				data->items[i].conf = &opt_items[i];
				templ = DoLockDlgRes(MAKEINTRESOURCE(data->items[i].conf->id));
				data->items[i].hwnd = CreateDialogIndirect(g_hInst, templ, hwndDlg, 
					data->items[i].conf->wnd_proc); 
				TranslateDialogDefault(data->items[i].hwnd);
				ShowWindowNew(data->items[i].hwnd, SW_HIDE);

				tie.pszText = TranslateTS(data->items[i].conf->name); 
				TabCtrl_InsertItem(hwndTab, i, &tie);
			}

			// Get avaible space
			GetWindowRect(hwndTab, &rc_tab);
			ScreenToClientRect(hwndTab, &rc_tab);
			TabCtrl_AdjustRect(hwndTab, FALSE, &rc_tab); 
			rc_tab.left+=3;
			rc_tab.right-=3;
			// Create big display
			data->hwndDisplay = CreateWindow(TEXT("STATIC"), TEXT(""), WS_CHILD|WS_VISIBLE, 
				rc_tab.left, rc_tab.top, 
				rc_tab.right-rc_tab.left, rc_tab.bottom-rc_tab.top, 
				hwndTab, NULL, g_hInst, NULL); 

			// Show first item
			ChangeTab(hwndDlg, data, 0);

			return TRUE;
			break;
		}
	case WM_NOTIFY: 
		{
			switch (((LPNMHDR)lParam)->code) 
			{
			case TCN_SELCHANGING:
				{
					WndItemsData *data = (WndItemsData *) GetWindowLong(hwndDlg, GWL_USERDATA);
					ShowWindowNew(data->items[data->selected_item].hwnd, SW_HIDE);
					break;
				}
			case TCN_SELCHANGE: 
				{
					ChangeTab(hwndDlg, 
						(WndItemsData *)GetWindowLong(hwndDlg, GWL_USERDATA), 
						TabCtrl_GetCurSel(GetDlgItem(hwndDlg, IDC_TAB)));
					break; 
				}

			case PSN_APPLY:
				{
					if (((LPNMHDR)lParam)->idFrom == 0)
					{
						WndItemsData *data = (WndItemsData *) GetWindowLong(hwndDlg, GWL_USERDATA);
						int i;
						for (i = 0 ; i < NUM_ITEM_OPTION_PAGES ; i++)
						{
							SendMessage(data->items[i].hwnd, msg, wParam, lParam);
						}

						pcli->pfnLoadContactTree();
						//ReLoadContactTree();/* this won't do job properly since it only really works when changes happen */
						ClcOptionsChanged(); // Used to force loading avatar an list height related options
						SendMessage(pcli->hwndContactTree,CLM_AUTOREBUILD,0,0); /* force reshuffle */
						return TRUE;
					}
					break;
				}
			}
			break;
		} 
	case WM_DESTROY: 
		{
			int i;
			WndItemsData *data = (WndItemsData *) GetWindowLong(hwndDlg, GWL_USERDATA);

			DestroyWindow(data->hwndDisplay); 

			for (i = 0 ; i < NUM_ITEM_OPTION_PAGES ; i++)
			{
				DestroyWindow(data->items[i].hwnd); 
			}

			mir_free(data); 
			break;
		}
	}

	return 0;
}

static BOOL CALLBACK DlgProcGenOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_USER+1:
		{
			HANDLE hContact=(HANDLE)wParam;
			DBCONTACTWRITESETTING * ws = (DBCONTACTWRITESETTING *)lParam;
			if ( hContact == NULL && ws != NULL && ws->szModule != NULL && ws->szSetting != NULL
				&& _strcmpi(ws->szModule,"CList")==0 && _strcmpi(ws->szSetting,"UseGroups")==0
				&& IsWindowVisible(hwndDlg) ) {
					CheckDlgButton(hwndDlg,IDC_DISABLEGROUPS,ws->value.bVal == 0);
				}
				break;
		}
	case WM_DESTROY: 
		{
			UnhookEvent( (HANDLE)GetWindowLong(hwndDlg,GWL_USERDATA) );
			break;
		}

	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		SetWindowLong(hwndDlg, GWL_USERDATA, (long)HookEventMessage(ME_DB_CONTACT_SETTINGCHANGED,hwndDlg,WM_USER+1));

		CheckDlgButton(hwndDlg, IDC_ONTOP, DBGetContactSettingByte(NULL,"CList","OnTop",SETTING_ONTOP_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_HIDEOFFLINE, DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_HIDEEMPTYGROUPS, DBGetContactSettingByte(NULL,"CList","HideEmptyGroups",SETTING_HIDEEMPTYGROUPS_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_DISABLEGROUPS, DBGetContactSettingByte(NULL,"CList","UseGroups",SETTING_USEGROUPS_DEFAULT) ? BST_UNCHECKED : BST_CHECKED);
		{
			int i, item;
			TCHAR *sortby[]={_T("Name"), _T("Status"), _T("Last message time"), _T("Protocol"), _T("-Nothing-")};
			for (i=0; i<sizeof(sortby)/sizeof(char*); i++) {
				item=SendDlgItemMessage(hwndDlg,IDC_CLSORT1,CB_ADDSTRING,0,(LPARAM)TranslateTS(sortby[i]));
				SendDlgItemMessage(hwndDlg,IDC_CLSORT1,CB_SETITEMDATA,item,(LPARAM)0);
				item=SendDlgItemMessage(hwndDlg,IDC_CLSORT2,CB_ADDSTRING,0,(LPARAM)TranslateTS(sortby[i]));
				SendDlgItemMessage(hwndDlg,IDC_CLSORT2,CB_SETITEMDATA,item,(LPARAM)0);
				item=SendDlgItemMessage(hwndDlg,IDC_CLSORT3,CB_ADDSTRING,0,(LPARAM)TranslateTS(sortby[i]));
				SendDlgItemMessage(hwndDlg,IDC_CLSORT3,CB_SETITEMDATA,item,(LPARAM)0);
			}
			SendDlgItemMessage(hwndDlg,IDC_CLSORT1,CB_SETCURSEL,DBGetContactSettingByte(NULL,"CList","SortBy1",SETTING_SORTBY1_DEFAULT),0);
			SendDlgItemMessage(hwndDlg,IDC_CLSORT2,CB_SETCURSEL,DBGetContactSettingByte(NULL,"CList","SortBy2",SETTING_SORTBY2_DEFAULT),0);
			SendDlgItemMessage(hwndDlg,IDC_CLSORT3,CB_SETCURSEL,DBGetContactSettingByte(NULL,"CList","SortBy3",SETTING_SORTBY3_DEFAULT),0);


		}
		CheckDlgButton(hwndDlg, IDC_NOOFFLINEMOVE, DBGetContactSettingByte(NULL,"CList","NoOfflineBottom",SETTING_NOOFFLINEBOTTOM_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_OFFLINETOROOT, DBGetContactSettingByte(NULL,"CList","PlaceOfflineToRoot",0) ? BST_CHECKED : BST_UNCHECKED);


		CheckDlgButton(hwndDlg, IDC_CONFIRMDELETE, DBGetContactSettingByte(NULL,"CList","ConfirmDelete",SETTING_CONFIRMDELETE_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_AUTOHIDE, DBGetContactSettingByte(NULL,"CList","AutoHide",SETTING_AUTOHIDE_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIME),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
		EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
		{
			DWORD caps=CallService(MS_CLUI_GETCAPS,CLUICAPS_FLAGS1,0);
			caps=CLUIF_HIDEEMPTYGROUPS|CLUIF_DISABLEGROUPS|CLUIF_HASONTOPOPTION|CLUIF_HASAUTOHIDEOPTION;
			if(!(caps&CLUIF_HIDEEMPTYGROUPS)) ShowWindowNew(GetDlgItem(hwndDlg,IDC_HIDEEMPTYGROUPS),SW_HIDE);
			if(!(caps&CLUIF_DISABLEGROUPS)) ShowWindowNew(GetDlgItem(hwndDlg,IDC_DISABLEGROUPS),SW_HIDE);
			if(caps&CLUIF_HASONTOPOPTION) ShowWindowNew(GetDlgItem(hwndDlg,IDC_ONTOP),SW_HIDE);
			if(caps&CLUIF_HASAUTOHIDEOPTION) {
				ShowWindowNew(GetDlgItem(hwndDlg,IDC_AUTOHIDE),SW_HIDE);
				ShowWindowNew(GetDlgItem(hwndDlg,IDC_HIDETIME),SW_HIDE);
				ShowWindowNew(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN),SW_HIDE);
				ShowWindowNew(GetDlgItem(hwndDlg,IDC_STAUTOHIDESECS),SW_HIDE);
			}
		}
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_SETRANGE,0,MAKELONG(900,1));
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","HideTime",SETTING_HIDETIME_DEFAULT),0));
		CheckDlgButton(hwndDlg, IDC_ONECLK, DBGetContactSettingByte(NULL,"CList","Tray1Click",SETTING_TRAY1CLICK_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		{
			BYTE trayOption=DBGetContactSettingByte(NULL,"CLUI","XStatusTray",15);
			CheckDlgButton(hwndDlg, IDC_SHOWXSTATUS, (trayOption&3) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOWNORMAL,  (trayOption&2) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_TRANSPARENTOVERLAY, (trayOption&4) ? BST_CHECKED : BST_UNCHECKED);

			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWNORMAL),IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENTOVERLAY),IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS)&&IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL));

		}
		CheckDlgButton(hwndDlg, IDC_ALWAYSSTATUS, DBGetContactSettingByte(NULL,"CList","AlwaysStatus",SETTING_ALWAYSSTATUS_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		
		CheckDlgButton(hwndDlg, IDC_ALWAYSPRIMARY, !DBGetContactSettingByte(NULL,"CList","AlwaysPremary",0) ? BST_CHECKED : BST_UNCHECKED);
		
		CheckDlgButton(hwndDlg, IDC_ALWAYSMULTI, !DBGetContactSettingByte(NULL,"CList","AlwaysMulti",SETTING_ALWAYSMULTI_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_DONTCYCLE, DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_SINGLE ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CYCLE, DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_CYCLE ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_MULTITRAY, DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_MULTI ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_DISABLEBLINK, DBGetContactSettingByte(NULL,"CList","DisableTrayFlash",0) == 1 ? BST_CHECKED : BST_UNCHECKED);
		//			CheckDlgButton(hwndDlg, IDC_ICONBLINK, DBGetContactSettingByte(NULL,"CList","NoIconBlink",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton(hwndDlg, IDC_SHOW_AVATARS, DBGetContactSettingByte(NULL,"CList","ShowAvatars",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
		/*			CheckDlgButton(hwndDlg, IDC_AVATAR_DRAW_BORDER, DBGetContactSettingByte(NULL,"CList","AvatarsDrawBorders",1) == 1 ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton(hwndDlg, IDC_AVATAR_ROUND_CORNERS, DBGetContactSettingByte(NULL,"CList","AvatarsRoundCorners",1) == 1 ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton(hwndDlg, IDC_AVATAR_ALIGN_LEFT, DBGetContactSettingByte(NULL,"CList","AvatarAlignLeft",1) == 1 ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton(hwndDlg, IDC_SHOW_STATUS_MSG, DBGetContactSettingByte(NULL,"CList","ShowStatusMsg",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton(hwndDlg, IDC_NOTCHECKFONTSIZE, DBGetContactSettingByte(NULL,"CList","DoNotCheckFontSize",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton(hwndDlg, IDC_NOTCHECKICONSIZE, DBGetContactSettingByte(NULL,"CList","DoNotCheckIconSize",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton(hwndDlg, IDC_VARIABLE_ROW_HEIGHT, DBGetContactSettingByte(NULL,"CList","VariableRowHeight",1) == 1 ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton(hwndDlg, IDC_HIDE_ICON_ON_AVATAR, DBGetContactSettingByte(NULL,"CList","HideIconOnAvatar",0) == 1 ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton(hwndDlg, IDC_AVATAR_OVERLAY_ICONS, DBGetContactSettingByte(NULL,"CList","AvatarsDrawOverlay",0) == 1 ? BST_CHECKED : BST_UNCHECKED );

		SendDlgItemMessage(hwndDlg,IDC_AVATAR_SIZE_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_AVATAR_SIZE),0);		// set buddy			
		SendDlgItemMessage(hwndDlg,IDC_AVATAR_SIZE_SPIN,UDM_SETRANGE,0,MAKELONG(255,1));
		SendDlgItemMessage(hwndDlg,IDC_AVATAR_SIZE_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","AvatarSize",30),0));

		SendDlgItemMessage(hwndDlg,IDC_MIN_ROW_HEIGHT_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_MIN_ROW_HEIGHT),0);		// set buddy			
		SendDlgItemMessage(hwndDlg,IDC_MIN_ROW_HEIGHT_SPIN,UDM_SETRANGE,0,MAKELONG(255,0));
		SendDlgItemMessage(hwndDlg,IDC_MIN_ROW_HEIGHT_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","MinRowHeight",CLCDEFAULT_ROWHEIGHT),0));

		SendDlgItemMessage(hwndDlg,IDC_ROW_BORDER_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_ROW_BORDER),0);		// set buddy			
		SendDlgItemMessage(hwndDlg,IDC_ROW_BORDER_SPIN,UDM_SETRANGE,0,MAKELONG(255,0));
		SendDlgItemMessage(hwndDlg,IDC_ROW_BORDER_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","RowBorder",2),0));

		if(!IsDlgButtonChecked(hwndDlg,IDC_SHOW_AVATARS)) {
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_DRAW_BORDER),FALSE);
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_ROUND_CORNERS),FALSE);
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_ALIGN_LEFT),FALSE);
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_L),FALSE);
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE),FALSE);
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_SPIN),FALSE);
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS),FALSE);
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICONS),FALSE);
		}
		*/
		if(IsDlgButtonChecked(hwndDlg,IDC_DONTCYCLE)) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_CYCLETIMESPIN),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_CYCLETIME),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_ALWAYSMULTI),FALSE);
		}
		if(IsDlgButtonChecked(hwndDlg,IDC_CYCLE)) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_PRIMARYSTATUS),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_ALWAYSMULTI),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_ALWAYSPRIMARY),FALSE);
		}
		if(IsDlgButtonChecked(hwndDlg,IDC_MULTITRAY)) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_CYCLETIMESPIN),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_CYCLETIME),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_PRIMARYSTATUS),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_ALWAYSPRIMARY),FALSE);
		}
		SendDlgItemMessage(hwndDlg,IDC_CYCLETIMESPIN,UDM_SETRANGE,0,MAKELONG(120,1));
		SendDlgItemMessage(hwndDlg,IDC_CYCLETIMESPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","CycleTime",SETTING_CYCLETIME_DEFAULT),0));
		{	int i,count,item;
		PROTOCOLDESCRIPTOR **protos;
		char szName[64];
		DBVARIANT dbv={DBVT_DELETED};
		DBGetContactSetting(NULL,"CList","PrimaryStatus",&dbv);
		CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
		item=SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_ADDSTRING,0,(LPARAM)TranslateT("Global"));
		SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_SETITEMDATA,item,(LPARAM)0);
		for(i=0;i<count;i++) {
			if(protos[i]->type!=PROTOTYPE_PROTOCOL || CallProtoService(protos[i]->szName,PS_GETCAPS,PFLAGNUM_2,0)==0) continue;
			CallProtoService(protos[i]->szName,PS_GETNAME,sizeof(szName),(LPARAM)szName);
			item=SendDlgItemMessageA(hwndDlg,IDC_PRIMARYSTATUS,CB_ADDSTRING,0,(LPARAM)szName);
			SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_SETITEMDATA,item,(LPARAM)protos[i]);
			if((dbv.type==DBVT_ASCIIZ || dbv.type==DBVT_UTF8)&& !strcmp(dbv.pszVal,protos[i]->szName))
				SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_SETCURSEL,item,0);
		}
		DBFreeVariant(&dbv);
		}
		if(-1==(int)SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_GETCURSEL,0,0))
			SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_SETCURSEL,0,0);
		SendDlgItemMessage(hwndDlg,IDC_BLINKSPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_BLINKTIME),0);		// set buddy			
		SendDlgItemMessage(hwndDlg,IDC_BLINKSPIN,UDM_SETRANGE,0,MAKELONG(0x3FFF,250));
		SendDlgItemMessage(hwndDlg,IDC_BLINKSPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","IconFlashTime",550),0));
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam)==IDC_SHOWXSTATUS||LOWORD(wParam)==IDC_SHOWNORMAL)
		{
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWNORMAL),IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENTOVERLAY),IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS)&&IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL));
		}
		if(LOWORD(wParam)==IDC_AUTOHIDE) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIME),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
		}
		if(LOWORD(wParam)==IDC_DONTCYCLE || LOWORD(wParam)==IDC_CYCLE || LOWORD(wParam)==IDC_MULTITRAY) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_PRIMARYSTATUS),IsDlgButtonChecked(hwndDlg,IDC_DONTCYCLE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_CYCLETIME),IsDlgButtonChecked(hwndDlg,IDC_CYCLE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_CYCLETIMESPIN),IsDlgButtonChecked(hwndDlg,IDC_CYCLE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_ALWAYSMULTI),IsDlgButtonChecked(hwndDlg,IDC_MULTITRAY));
			EnableWindow(GetDlgItem(hwndDlg,IDC_ALWAYSPRIMARY),IsDlgButtonChecked(hwndDlg,IDC_DONTCYCLE));
		}
		/*			if (LOWORD(wParam)==IDC_SHOW_AVATARS){
		BOOL enabled = IsDlgButtonChecked(hwndDlg,IDC_SHOW_AVATARS);
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_DRAW_BORDER),enabled);
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_ROUND_CORNERS),enabled);
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_ALIGN_LEFT),enabled);
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_L),enabled);
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE),enabled);
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_SPIN),enabled);
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS),enabled);
		EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICONS),enabled);
		}*/
		if((LOWORD(wParam)==IDC_HIDETIME || LOWORD(wParam)==IDC_CYCLETIME) && HIWORD(wParam)!=EN_CHANGE) break;
		if(LOWORD(wParam)==IDC_PRIMARYSTATUS && HIWORD(wParam)!=CBN_SELCHANGE) break;
		if((LOWORD(wParam)==IDC_HIDETIME || LOWORD(wParam)==IDC_CYCLETIME) && (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus())) return 0;
		if (LOWORD(wParam)==IDC_BLINKTIME && (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus())) return 0; // dont make apply enabled during buddy set crap
		//			if (LOWORD(wParam)==IDC_AVATAR_SIZE && HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0; // dont make apply enabled during buddy set crap
		//			if (LOWORD(wParam)==IDC_MIN_ROW_HEIGHT && HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0; // dont make apply enabled during buddy set crap
		//			if (LOWORD(wParam)==IDC_ROW_BORDER && HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0; // dont make apply enabled during buddy set crap
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->idFrom) {
	case 0:
		switch (((LPNMHDR)lParam)->code)
		{
		case PSN_APPLY:
			DBWriteContactSettingByte(NULL,"CList","OnDesktop",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONDESKTOP));
			DBWriteContactSettingByte(NULL,"CList","HideOffline",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_HIDEOFFLINE));
			{	DWORD caps=CallService(MS_CLUI_GETCAPS,CLUICAPS_FLAGS1,0);
			if(caps&CLUIF_HIDEEMPTYGROUPS) DBWriteContactSettingByte(NULL,"CList","HideEmptyGroups",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_HIDEEMPTYGROUPS));
			if(caps&CLUIF_DISABLEGROUPS) DBWriteContactSettingByte(NULL,"CList","UseGroups",(BYTE)!IsDlgButtonChecked(hwndDlg,IDC_DISABLEGROUPS));
			if(!(caps&CLUIF_HASONTOPOPTION)) {
				DBWriteContactSettingByte(NULL,"CList","OnTop",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONTOP));
				SetWindowPos((HWND)CallService(MS_CLUI_GETHWND,0,0), IsDlgButtonChecked(hwndDlg,IDC_ONTOP)?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			}
			if(!(caps&CLUIF_HASAUTOHIDEOPTION)) {
				DBWriteContactSettingByte(NULL,"CList","AutoHide",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
				DBWriteContactSettingWord(NULL,"CList","HideTime",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_GETPOS,0,0));
			}
			}
			DBWriteContactSettingByte(NULL,"CList","SortBy1",(BYTE)SendDlgItemMessage(hwndDlg,IDC_CLSORT1,CB_GETCURSEL,0,0));
			DBWriteContactSettingByte(NULL,"CList","SortBy2",(BYTE)SendDlgItemMessage(hwndDlg,IDC_CLSORT2,CB_GETCURSEL,0,0));
			DBWriteContactSettingByte(NULL,"CList","SortBy3",(BYTE)SendDlgItemMessage(hwndDlg,IDC_CLSORT3,CB_GETCURSEL,0,0));
			DBWriteContactSettingByte(NULL,"CList","NoOfflineBottom",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_NOOFFLINEMOVE));
			DBWriteContactSettingByte(NULL,"CList","PlaceOfflineToRoot",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_OFFLINETOROOT));

			DBWriteContactSettingByte(NULL,"CList","ConfirmDelete",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_CONFIRMDELETE));
			DBWriteContactSettingByte(NULL,"CList","Tray1Click",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONECLK));
			DBWriteContactSettingByte(NULL,"CList","AlwaysStatus",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ALWAYSSTATUS));
			DBWriteContactSettingByte(NULL,"CList","AlwaysMulti",(BYTE)!IsDlgButtonChecked(hwndDlg,IDC_ALWAYSMULTI));
			DBWriteContactSettingByte(NULL,"CList","AlwaysPrimary",(BYTE)!IsDlgButtonChecked(hwndDlg,IDC_ALWAYSPRIMARY));
			DBWriteContactSettingByte(NULL,"CList","TrayIcon",(BYTE)(IsDlgButtonChecked(hwndDlg,IDC_DONTCYCLE)?SETTING_TRAYICON_SINGLE:(IsDlgButtonChecked(hwndDlg,IDC_CYCLE)?SETTING_TRAYICON_CYCLE:SETTING_TRAYICON_MULTI)));
			DBWriteContactSettingWord(NULL,"CList","CycleTime",(WORD)SendDlgItemMessage(hwndDlg,IDC_CYCLETIMESPIN,UDM_GETPOS,0,0));
			DBWriteContactSettingWord(NULL,"CList","IconFlashTime",(WORD)SendDlgItemMessage(hwndDlg,IDC_BLINKSPIN,UDM_GETPOS,0,0));
			DBWriteContactSettingByte(NULL,"CList","DisableTrayFlash",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_DISABLEBLINK));

			{
				BYTE xOptions=0;
				xOptions=IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS)?1:0;
				xOptions|=(xOptions && IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL))?2:0;
				xOptions|=(xOptions && IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENTOVERLAY))?4:0;
				DBWriteContactSettingByte(NULL,"CLUI","XStatusTray",xOptions);				
			}
			/*							DBWriteContactSettingByte(NULL,"CList","NoIconBlink", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_ICONBLINK));
			DBWriteContactSettingByte(NULL,"CList","ShowAvatars", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOW_AVATARS));
			DBWriteContactSettingByte(NULL,"CList","AvatarsDrawBorders", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_AVATAR_DRAW_BORDER));
			DBWriteContactSettingByte(NULL,"CList","AvatarsRoundCorners", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_AVATAR_ROUND_CORNERS));
			DBWriteContactSettingByte(NULL,"CList","AvatarAlignLeft", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_AVATAR_ALIGN_LEFT));
			DBWriteContactSettingByte(NULL,"CList","ShowStatusMsg", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOW_STATUS_MSG));
			DBWriteContactSettingByte(NULL,"CList","DoNotCheckFontSize", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_NOTCHECKFONTSIZE));
			DBWriteContactSettingByte(NULL,"CList","DoNotCheckIconSize", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_NOTCHECKICONSIZE));
			DBWriteContactSettingWord(NULL,"CList","AvatarSize",(WORD)SendDlgItemMessage(hwndDlg,IDC_AVATAR_SIZE_SPIN,UDM_GETPOS,0,0));
			DBWriteContactSettingWord(NULL,"CList","MinRowHeight",(WORD)SendDlgItemMessage(hwndDlg,IDC_MIN_ROW_HEIGHT_SPIN,UDM_GETPOS,0,0));
			DBWriteContactSettingWord(NULL,"CList","RowBorder",(WORD)SendDlgItemMessage(hwndDlg,IDC_ROW_BORDER_SPIN,UDM_GETPOS,0,0));
			DBWriteContactSettingByte(NULL,"CList","VariableRowHeight", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_VARIABLE_ROW_HEIGHT));
			DBWriteContactSettingByte(NULL,"CList","HideIconOnAvatar", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_HIDE_ICON_ON_AVATAR));
			DBWriteContactSettingByte(NULL,"CList","AvatarsDrawOverlay", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_AVATAR_OVERLAY_ICONS));
			*/
			if (!SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_GETCURSEL,0,0),0)) 
				DBDeleteContactSetting(NULL, "CList","PrimaryStatus");
			else DBWriteContactSettingString(NULL,"CList","PrimaryStatus",((PROTOCOLDESCRIPTOR*)SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_GETCURSEL,0,0),0))->szName);
			pcli->pfnTrayIconIconsChanged();
			pcli->pfnLoadContactTree(); /* this won't do job properly since it only really works when changes happen */
			SendMessage(pcli->hwndContactTree,CLM_AUTOREBUILD,0,0); /* force reshuffle */
			ClcOptionsChanged(); // Used to force loading avatar an list height related options
			return TRUE;
		}
		break;
		}
		break;
	}
	return FALSE;
}

static BOOL CALLBACK DlgProcHotkeyOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{	DBVARIANT dbv={0};

		TranslateDialogDefault(hwndDlg);

		CheckDlgButton(hwndDlg, IDC_SHOWHIDE, DBGetContactSettingByte(NULL,"CList","HKEnShowHide",0) ? BST_CHECKED : BST_UNCHECKED);			
		CheckDlgButton(hwndDlg, IDC_READMSG, DBGetContactSettingByte(NULL,"CList","HKEnReadMsg",0) ? BST_CHECKED : BST_UNCHECKED);			
		CheckDlgButton(hwndDlg, IDC_NETSEARCH, DBGetContactSettingByte(NULL,"CList","HKEnNetSearch",0) ? BST_CHECKED : BST_UNCHECKED);			
		CheckDlgButton(hwndDlg, IDC_SHOWOPTIONS, DBGetContactSettingByte(NULL,"CList","HKEnShowOptions",0) ? BST_CHECKED : BST_UNCHECKED);

		EnableWindow(GetDlgItem(hwndDlg,IDC_HKSHOWHIDE),IsDlgButtonChecked(hwndDlg,IDC_SHOWHIDE));
		EnableWindow(GetDlgItem(hwndDlg,IDC_HKREADMSG),IsDlgButtonChecked(hwndDlg,IDC_READMSG));
		EnableWindow(GetDlgItem(hwndDlg,IDC_HKSEARCH),IsDlgButtonChecked(hwndDlg,IDC_NETSEARCH));
		EnableWindow(GetDlgItem(hwndDlg,IDC_SEARCHURL),IsDlgButtonChecked(hwndDlg,IDC_NETSEARCH));
		EnableWindow(GetDlgItem(hwndDlg,IDC_SEARCHNEWWND),IsDlgButtonChecked(hwndDlg,IDC_NETSEARCH));
		EnableWindow(GetDlgItem(hwndDlg,IDC_HOTKEYURLSTR),IsDlgButtonChecked(hwndDlg,IDC_NETSEARCH));					
		EnableWindow(GetDlgItem(hwndDlg,IDC_HKSHOWOPTIONS),IsDlgButtonChecked(hwndDlg,IDC_SHOWOPTIONS));

		SendDlgItemMessage(hwndDlg,IDC_HKSHOWHIDE,HKM_SETHOTKEY,DBGetContactSettingWord(NULL,"CList","HKShowHide",MAKEWORD('A',HOTKEYF_CONTROL|HOTKEYF_SHIFT)),0);
		SendDlgItemMessage(hwndDlg,IDC_HKREADMSG,HKM_SETHOTKEY,DBGetContactSettingWord(NULL,"CList","HKReadMsg",MAKEWORD('I',HOTKEYF_CONTROL|HOTKEYF_SHIFT)),0);
		SendDlgItemMessage(hwndDlg,IDC_HKSEARCH,HKM_SETHOTKEY,DBGetContactSettingWord(NULL,"CList","HKNetSearch",MAKEWORD('S',HOTKEYF_CONTROL|HOTKEYF_SHIFT)),0);
		SendDlgItemMessage(hwndDlg,IDC_HKSHOWOPTIONS,HKM_SETHOTKEY,DBGetContactSettingWord(NULL,"CList","HKShowOptions",MAKEWORD('O',HOTKEYF_CONTROL|HOTKEYF_SHIFT)),0);
		if(!DBGetContactSetting(NULL,"CList","SearchUrl",&dbv)) {
			SetDlgItemTextA(hwndDlg,IDC_SEARCHURL,dbv.pszVal);
			DBFreeVariant(&dbv);
		}
		else SetDlgItemTextA(hwndDlg,IDC_SEARCHURL,"http://www.google.com/");
		CheckDlgButton(hwndDlg, IDC_SEARCHNEWWND, DBGetContactSettingByte(NULL,"CList","HKSearchNewWnd",0) ? BST_CHECKED : BST_UNCHECKED);
		return TRUE;
		}
	case WM_COMMAND:
		if(LOWORD(wParam)==IDC_SEARCHURL && (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus())) return 0;
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
		switch(LOWORD(wParam)) {
	case IDC_SHOWHIDE:
		EnableWindow(GetDlgItem(hwndDlg,IDC_HKSHOWHIDE),IsDlgButtonChecked(hwndDlg,IDC_SHOWHIDE));
		break;
	case IDC_READMSG:
		EnableWindow(GetDlgItem(hwndDlg,IDC_HKREADMSG),IsDlgButtonChecked(hwndDlg,IDC_READMSG));
		break;
	case IDC_NETSEARCH:
		EnableWindow(GetDlgItem(hwndDlg,IDC_HKSEARCH),IsDlgButtonChecked(hwndDlg,IDC_NETSEARCH));
		EnableWindow(GetDlgItem(hwndDlg,IDC_SEARCHURL),IsDlgButtonChecked(hwndDlg,IDC_NETSEARCH));
		EnableWindow(GetDlgItem(hwndDlg,IDC_SEARCHNEWWND),IsDlgButtonChecked(hwndDlg,IDC_NETSEARCH));					
		EnableWindow(GetDlgItem(hwndDlg,IDC_HOTKEYURLSTR),IsDlgButtonChecked(hwndDlg,IDC_NETSEARCH));					
		break;
	case IDC_SHOWOPTIONS:
		EnableWindow(GetDlgItem(hwndDlg,IDC_HKSHOWOPTIONS),IsDlgButtonChecked(hwndDlg,IDC_SHOWOPTIONS));
		break;
		}
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case PSN_APPLY:
			{	char str[256];
			HotKeysUnregister((HWND)CallService(MS_CLUI_GETHWND,0,0));
			DBWriteContactSettingByte(NULL,"CList","HKEnShowHide",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWHIDE));
			DBWriteContactSettingWord(NULL,"CList","HKShowHide",(WORD)SendDlgItemMessage(hwndDlg,IDC_HKSHOWHIDE,HKM_GETHOTKEY,0,0));
			DBWriteContactSettingByte(NULL,"CList","HKEnReadMsg",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_READMSG));
			DBWriteContactSettingWord(NULL,"CList","HKReadMsg",(WORD)SendDlgItemMessage(hwndDlg,IDC_HKREADMSG,HKM_GETHOTKEY,0,0));
			DBWriteContactSettingByte(NULL,"CList","HKEnNetSearch",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_NETSEARCH));
			DBWriteContactSettingWord(NULL,"CList","HKNetSearch",(WORD)SendDlgItemMessage(hwndDlg,IDC_HKSEARCH,HKM_GETHOTKEY,0,0));
			GetDlgItemTextA(hwndDlg,IDC_SEARCHURL,str,sizeof(str));
			DBWriteContactSettingString(NULL,"CList","SearchUrl",str);
			DBWriteContactSettingByte(NULL,"CList","HKSearchNewWnd",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SEARCHNEWWND));
			DBWriteContactSettingByte(NULL,"CList","HKEnShowOptions",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWOPTIONS));
			DBWriteContactSettingWord(NULL,"CList","HKShowOptions",(WORD)SendDlgItemMessage(hwndDlg,IDC_HKSHOWOPTIONS,HKM_GETHOTKEY,0,0));
			cliHotKeysRegister((HWND)CallService(MS_CLUI_GETHWND,0,0));
			return TRUE;
			}
		}
		break;
	}
	return FALSE;
}