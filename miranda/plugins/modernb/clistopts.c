/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project, 
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
#include "commonprototypes.h"


static BOOL CALLBACK DlgProcExtraIconsOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcItemRowOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcItemAvatarOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcItemIconOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcItemContactTimeOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcItemTextOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcItemSecondLineOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcItemThirdLineOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static UINT expertOnlyControls[]={IDC_ALWAYSSTATUS};

static struct _RowItemOptionConf 
{ 
	TCHAR *name;				// Tab name
	int id;					// Dialog id
	DLGPROC wnd_proc;		// Dialog function
} row_opt_items[] = { 
	{ _T("Row"), IDD_OPT_ITEM_ROW, DlgProcItemRowOpts },
#ifdef _DEBUG
	{ _T("Row design"), IDD_OPT_ROWTMPL, DlgTmplEditorOpts },
#endif
	{ _T("Avatar"), IDD_OPT_ITEM_AVATAR, DlgProcItemAvatarOpts },
	{ _T("Icon"), IDD_OPT_ITEM_ICON, DlgProcItemIconOpts },
	{ _T("Contact time"), IDD_OPT_ITEM_CONTACT_TIME, DlgProcItemContactTimeOpts },
	{ _T("Text"), IDD_OPT_ITEM_TEXT, DlgProcItemTextOpts },
	{ _T("Second Line"), IDD_OPT_ITEM_SECOND_LINE, DlgProcItemSecondLineOpts },
	{ _T("Third Line"), IDD_OPT_ITEM_THIRD_LINE, DlgProcItemThirdLineOpts },
	{ _T("Extra Icons"), IDD_OPT_ITEM_EXTRAICONS, DlgProcExtraIconsOpts},
	{	0	}
};


int CListOptInit(WPARAM wParam,LPARAM lParam)
{

	OPTIONSDIALOGPAGE odp;

	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=-200000000;
	odp.hInstance=g_hInst;
	//	odp.pfnDlgProc=DlgProcItemsOpts;
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_ITEMS);
	odp.ptszGroup=LPGENT("Contact List");
	odp.ptszTitle=LPGENT("Row items");
	odp.flags=ODPF_BOLDGROUPS|ODPF_TCHAR;
	//	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	{
		int i;	
		for (i=0; row_opt_items[i].id!=0; i++)
		{
			odp.pszTemplate=MAKEINTRESOURCEA(row_opt_items[i].id);
			odp.ptszTab=TranslateTS(row_opt_items[i].name);
			odp.pfnDlgProc=row_opt_items[i].wnd_proc;
			CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
		}
	}
#ifdef FORCE_HOTKEY_IN_MODERN

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

#endif //FORCE_HOTKEY_IN_MODERN

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

            TranslateDialogDefault(hwndDlg);
			SendDlgItemMessage(hwndDlg,IDC_MIN_ROW_HEIGHT_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_MIN_ROW_HEIGHT),0);		// set buddy			
			SendDlgItemMessage(hwndDlg,IDC_MIN_ROW_HEIGHT_SPIN,UDM_SETRANGE,0,MAKELONG(255,0));
			SendDlgItemMessage(hwndDlg,IDC_MIN_ROW_HEIGHT_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","MinRowHeight",CLCDEFAULT_ROWHEIGHT),0));

			SendDlgItemMessage(hwndDlg,IDC_ROW_BORDER_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_ROW_BORDER),0);		// set buddy			
			SendDlgItemMessage(hwndDlg,IDC_ROW_BORDER_SPIN,UDM_SETRANGE,0,MAKELONG(255,0));
			SendDlgItemMessage(hwndDlg,IDC_ROW_BORDER_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","RowBorder",SETTING_ROWBORDER_DEFAULT),0));

			CheckDlgButton(hwndDlg, IDC_VARIABLE_ROW_HEIGHT, DBGetContactSettingByte(NULL,"CList","VariableRowHeight",SETTING_VARIABLEROWHEIGHT_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_ALIGN_TO_LEFT, DBGetContactSettingByte(NULL,"CList","AlignLeftItemsToLeft",SETTING_ALIGNLEFTTOLEFT_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_ALIGN_TO_RIGHT, DBGetContactSettingByte(NULL,"CList","AlignRightItemsToRight",SETTING_ALIGNRIGHTORIGHT_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			
			CheckDlgButton(hwndDlg, IDC_MINIMODE, DBGetContactSettingByte(NULL,"CLC","CompactMode",SETTING_COMPACTMODE_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );

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

						SendMessage((GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
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

						SendMessage((GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
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

			SendMessage((GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
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
							DBWriteContactSettingByte(NULL,"CLC","CompactMode", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_MINIMODE));
							DBWriteContactSettingByte(NULL,"CLC","LeftMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_GETPOS,0,0));
							DBWriteContactSettingByte(NULL,"CLC","RightMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_GETPOS,0,0));

							hwndList = GetDlgItem(hwndDlg, IDC_LIST_ORDER);
							for(i = 0 ; i < NUM_ITEM_TYPE ; i++)
							{
								char tmp[128];
								mir_snprintf(tmp, sizeof(tmp), "RowPos%d", i);
								DBWriteContactSettingWord(NULL,"CList",tmp,(WORD)SendMessage(hwndList, LB_GETITEMDATA, i, 0));
							}

							ClcOptionsChanged();
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
            TranslateDialogDefault(hwndDlg);
			CheckDlgButton(hwndDlg, IDC_SHOW_AVATARS, DBGetContactSettingByte(NULL,"CList","AvatarsShow",SETTINGS_SHOWAVATARS_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_SHOW_ANIAVATARS, DBGetContactSettingByte(NULL,"CList","AvatarsAnimated",(ServiceExists(MS_AV_GETAVATARBITMAP)&&!g_CluiData.fGDIPlusFail)) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_AVATAR_DRAW_BORDER, DBGetContactSettingByte(NULL,"CList","AvatarsDrawBorders",SETTINGS_AVATARDRAWBORDER_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_AVATAR_ROUND_CORNERS, DBGetContactSettingByte(NULL,"CList","AvatarsRoundCorners",SETTINGS_AVATARROUNDCORNERS_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_AVATAR_CUSTOM_CORNER_SIZE_CHECK, DBGetContactSettingByte(NULL,"CList","AvatarsUseCustomCornerSize",SETTINGS_AVATARUSECUTOMCORNERSIZE_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_AVATAR_IGNORE_SIZE, DBGetContactSettingByte(NULL,"CList","AvatarsIgnoreSizeForRow",SETTINGS_AVATARIGNORESIZEFORROW_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_AVATAR_OVERLAY_ICONS, DBGetContactSettingByte(NULL,"CList","AvatarsDrawOverlay",SETTINGS_AVATARDRAWOVERLAY_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );

			switch(DBGetContactSettingByte(NULL,"CList","AvatarsOverlayType",SETTINGS_AVATAROVERLAYTYPE_DEFAULT))
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
			SendDlgItemMessage(hwndDlg,IDC_AVATAR_SIZE_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","AvatarsSize",SETTING_AVATARHEIGHT_DEFAULT),0));

			SendDlgItemMessage(hwndDlg,IDC_AVATAR_WIDTH_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_AVATAR_WIDTH),0);		// set buddy			
			SendDlgItemMessage(hwndDlg,IDC_AVATAR_WIDTH_SPIN,UDM_SETRANGE,0,MAKELONG(255,0));
			SendDlgItemMessage(hwndDlg,IDC_AVATAR_WIDTH_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","AvatarsWidth",SETTING_AVATARWIDTH_DEFAULT),0));

			SendDlgItemMessage(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE),0);		// set buddy
			SendDlgItemMessage(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_SPIN,UDM_SETRANGE,0,MAKELONG(255,1));
			SendDlgItemMessage(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","AvatarsCustomCornerSize",SETTINGS_AVATARCORNERSIZE_DEFAULT),0));

			SendDlgItemMessage(hwndDlg, IDC_AVATAR_BORDER_COLOR, CPM_SETCOLOUR, 0, (COLORREF)DBGetContactSettingDword(NULL,"CList","AvatarsBorderColor",SETTINGS_AVATARBORDERCOLOR_DEFAULT));

			if(!IsDlgButtonChecked(hwndDlg,IDC_SHOW_AVATARS)) 
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_DRAW_BORDER),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_BORDER_COLOR_L),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_BORDER_COLOR),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_ROUND_CORNERS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_CHECK),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_IGNORE_SIZE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICONS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_NORMAL),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_PROTOCOL),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_CONTACT),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_L),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_SPIN),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_WIDTH),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_WIDTH_SPIN),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS2),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_ANIAVATARS),FALSE);
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
			if (!ServiceExists(MS_AV_GETAVATARBITMAP)||g_CluiData.fGDIPlusFail)
			{
			   EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_ANIAVATARS),FALSE);
			   SetDlgItemText(hwndDlg, IDC_SHOW_ANIAVATARS, TranslateT("Animate Avatars (GDI+ and loadavatars.dll are required)"));
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
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_CUSTOM_CORNER_SIZE_CHECK),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_IGNORE_SIZE),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICONS),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_NORMAL),enabled && IsDlgButtonChecked(hwndDlg,IDC_AVATAR_OVERLAY_ICONS));
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_PROTOCOL),enabled && IsDlgButtonChecked(hwndDlg,IDC_AVATAR_OVERLAY_ICONS));
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_OVERLAY_ICON_CONTACT),enabled && IsDlgButtonChecked(hwndDlg,IDC_AVATAR_OVERLAY_ICONS));
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_L),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_SPIN),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS2),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS3),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_ANIAVATARS),enabled && (ServiceExists(MS_AV_GETAVATARBITMAP)&&!g_CluiData.fGDIPlusFail));

				//				if (DBGetContactSettingByte(NULL,"ModernData","UseAdvancedRowLayout",SETTING_ROW_ADVANCEDLAYOUT_DEFAULT)==1)
				//				{
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_WIDTH),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_WIDTH_SPIN),enabled);
				EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS),enabled);
				//				}
				//				else
				//				{
				//					EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_WIDTH),FALSE);
				//					EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_WIDTH_SPIN),FALSE);
				//					EnableWindow(GetDlgItem(hwndDlg,IDC_AVATAR_SIZE_PIXELS),FALSE);
				//				}
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

			SendMessage((GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
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
							DBWriteContactSettingByte(NULL,"CList","AvatarsAnimated", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOW_ANIAVATARS));
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
							ClcOptionsChanged();
							AniAva_UpdateOptions();
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
            TranslateDialogDefault(hwndDlg);
			CheckDlgButton(hwndDlg, IDC_HIDE_ICON_ON_AVATAR, DBGetContactSettingByte(NULL,"CList","IconHideOnAvatar",SETTING_HIDEICONONAVATAR_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_DRAW_ON_AVATAR_SPACE, DBGetContactSettingByte(NULL,"CList","IconDrawOnAvatarSpace",SETTING_ICONONAVATARPLACE_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_ICONBLINK, DBGetContactSettingByte(NULL,"CList","NoIconBlink",SETTING_NOICONBLINF_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_HIDE_GROUPSICON, DBGetContactSettingByte(NULL,"CList","HideGroupsIcon",SETTING_HIDEGROUPSICON_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_NOTCHECKICONSIZE, DBGetContactSettingByte(NULL,"CList","IconIgnoreSizeForRownHeight",SETTING_ICONIGNORESIZE_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_ALWAYS_VISIBLEICON, DBGetContactSettingByte(NULL,"CList","AlwaysShowAlwaysVisIcon",SETTING_ALWAYSVISICON_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_USEXSTATUS, (DBGetContactSettingByte(NULL,"CLC","DrawOverlayedStatus",SETTING_DRAWOVERLAYEDSTATUS_DEFAULT)&1) ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_DRAWSTATUSOVERLAY, (DBGetContactSettingByte(NULL,"CLC","DrawOverlayedStatus",SETTING_DRAWOVERLAYEDSTATUS_DEFAULT)&2) ? BST_CHECKED : BST_UNCHECKED );			
			EnableWindow(GetDlgItem(hwndDlg,IDC_DRAWSTATUSOVERLAY),IsDlgButtonChecked(hwndDlg,IDC_USEXSTATUS));

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
			else if (LOWORD(wParam)==IDC_USEXSTATUS)
				EnableWindow(GetDlgItem(hwndDlg,IDC_DRAWSTATUSOVERLAY),IsDlgButtonChecked(hwndDlg,IDC_USEXSTATUS));

			SendMessage((GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
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
							DBWriteContactSettingByte(NULL,"CList","AlwaysShowAlwaysVisIcon", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_ALWAYS_VISIBLEICON));
						
							{
								BYTE ovr=IsDlgButtonChecked(hwndDlg,IDC_USEXSTATUS)?1:0;
								if (ovr) ovr+=IsDlgButtonChecked(hwndDlg,IDC_DRAWSTATUSOVERLAY)?2:0;
								DBWriteContactSettingByte(NULL,"CLC","DrawOverlayedStatus", ovr );
							}
							ClcOptionsChanged();
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
            TranslateDialogDefault(hwndDlg);
			CheckDlgButton(hwndDlg, IDC_SHOW, DBGetContactSettingByte(NULL,"CList","ContactTimeShow",SETTING_SHOWTIME_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_SHOW_ONLY_IF_DIFFERENT, DBGetContactSettingByte(NULL,"CList","ContactTimeShowOnlyIfDifferent",SETTING_SHOWTIMEIFDIFF_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );

			break;
		}
	case WM_COMMAND:
		{
			SendMessage((GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
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
							ClcOptionsChanged();
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
            TranslateDialogDefault(hwndDlg);
			CheckDlgButton(hwndDlg, IDC_RTL, DBGetContactSettingByte(NULL,"CList","TextRTL",SETTING_TEXT_RTL_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			//TODO: init IDC_ALIGNGROUPCOMBO from DBGetContactSettingByte(NULL,"CList","AlignGroupCaptions",SETTING_ALIGNGROPCAPTION_DEFAULT);
			{
				int i, item;
				TCHAR *align[]={_T("Left align group names"), _T("Center group names"), _T("Right align group names")};
				for (i=0; i<sizeof(align)/sizeof(char*); i++) 
					item=SendDlgItemMessage(hwndDlg,IDC_ALIGNGROUPCOMBO,CB_ADDSTRING,0,(LPARAM)TranslateTS(align[i]));
				SendDlgItemMessage(hwndDlg,IDC_ALIGNGROUPCOMBO,CB_SETCURSEL,DBGetContactSettingByte(NULL,"CList","AlignGroupCaptions",SETTING_ALIGNGROPCAPTION_DEFAULT),0);
			}
			CheckDlgButton(hwndDlg, IDC_ALIGN_RIGHT, DBGetContactSettingByte(NULL,"CList","TextAlignToRight",SETTING_TEXT_RIGHTALIGN_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_REPLACE_SMILEYS, DBGetContactSettingByte(NULL,"CList","TextReplaceSmileys",SETTING_TEXT_SMILEY_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_RESIZE_SMILEYS, DBGetContactSettingByte(NULL,"CList","TextResizeSmileys",SETTING_TEXT_RESIZESMILEY_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_USE_PROTOCOL_SMILEYS, DBGetContactSettingByte(NULL,"CList","TextUseProtocolSmileys",SETTING_TEXT_PROTOSMILEY_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_IGNORE_SIZE, DBGetContactSettingByte(NULL,"CList","TextIgnoreSizeForRownHeight",SETTING_TEXT_IGNORESIZE_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );

			CheckDlgButton(hwndDlg, IDC_DRAW_SMILEYS_ON_FIRST_LINE, DBGetContactSettingByte(NULL,"CList","FirstLineDrawSmileys",SETTING_FIRSTLINE_SMILEYS_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_APPEND_NICK, DBGetContactSettingByte(NULL,"CList","FirstLineAppendNick",SETTING_FIRSTLINE_APPENDNICK_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_TRIM_TEXT, DBGetContactSettingByte(NULL,"CList","TrimText",SETTING_FIRSTLINE_TRIMTEXT_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );

			CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_REPLACE_SMILEYS), ServiceExists(MS_SMILEYADD_PARSE) ? SW_SHOW : SW_HIDE);
			CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_USE_PROTOCOL_SMILEYS), ServiceExists(MS_SMILEYADD_PARSE) ? SW_SHOW : SW_HIDE);
			//CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_RESIZE_SMILEYS), ServiceExists(MS_SMILEYADD_PARSE) ? SW_SHOW : SW_HIDE);
			CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_DRAW_SMILEYS_ON_FIRST_LINE), ServiceExists(MS_SMILEYADD_PARSE) ? SW_SHOW : SW_HIDE);

			if (!IsDlgButtonChecked(hwndDlg,IDC_REPLACE_SMILEYS))
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_PROTOCOL_SMILEYS),FALSE);
				//EnableWindow(GetDlgItem(hwndDlg,IDC_RESIZE_SMILEYS),FALSE);  //Commented out for listening to icon
			}

			break;
		}
	case WM_COMMAND:
		{
			if (LOWORD(wParam)==IDC_REPLACE_SMILEYS)
			{
				BOOL enabled = IsDlgButtonChecked(hwndDlg,IDC_REPLACE_SMILEYS);
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_PROTOCOL_SMILEYS),enabled);
				//EnableWindow(GetDlgItem(hwndDlg,IDC_RESIZE_SMILEYS),enabled); //Commented out for listening to icon
			}      
			if (LOWORD(wParam)!=IDC_ALIGNGROUPCOMBO || (LOWORD(wParam)==IDC_ALIGNGROUPCOMBO && HIWORD(wParam)==CBN_SELCHANGE))
				SendMessage((GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
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
							//TODO: Store IDC_ALIGNGROUPCOMBO at DBGetContactSettingByte(NULL,"CList","AlignGroupCaptions",SETTING_ALIGNGROPCAPTION_DEFAULT);
							DBWriteContactSettingByte(NULL,"CList","AlignGroupCaptions",(BYTE)SendDlgItemMessage(hwndDlg,IDC_ALIGNGROUPCOMBO,CB_GETCURSEL,0,0));
							DBWriteContactSettingByte(NULL,"CList","TextAlignToRight", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_ALIGN_RIGHT));
							DBWriteContactSettingByte(NULL,"CList","TextReplaceSmileys", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_REPLACE_SMILEYS));
							DBWriteContactSettingByte(NULL,"CList","TextResizeSmileys", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_RESIZE_SMILEYS));
							DBWriteContactSettingByte(NULL,"CList","TextUseProtocolSmileys", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_USE_PROTOCOL_SMILEYS));
							DBWriteContactSettingByte(NULL,"CList","TextIgnoreSizeForRownHeight", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_IGNORE_SIZE));
							DBWriteContactSettingByte(NULL,"CList","FirstLineDrawSmileys", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_DRAW_SMILEYS_ON_FIRST_LINE));
							DBWriteContactSettingByte(NULL,"CList","FirstLineAppendNick", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_APPEND_NICK));
							DBWriteContactSettingByte(NULL,"CList","TrimText", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_TRIM_TEXT));
							ClcOptionsChanged();						
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
            TranslateDialogDefault(hwndDlg);
			CheckDlgButton(hwndDlg, IDC_SHOW, DBGetContactSettingByte(NULL,"CList","SecondLineShow",SETTING_SECONDLINE_SHOW_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );

			SendDlgItemMessage(hwndDlg,IDC_TOP_SPACE_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_TOP_SPACE),0);		// set buddy			
			SendDlgItemMessage(hwndDlg,IDC_TOP_SPACE_SPIN,UDM_SETRANGE,0,MAKELONG(255,0));
			SendDlgItemMessage(hwndDlg,IDC_TOP_SPACE_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","SecondLineTopSpace",SETTING_SECONDLINE_TOPSPACE_DEFAULT),0));

			CheckDlgButton(hwndDlg, IDC_DRAW_SMILEYS, DBGetContactSettingByte(NULL,"CList","SecondLineDrawSmileys",SETTING_SECONDLINE_SMILEYS_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED);

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
			CheckDlgButton(hwndDlg, IDC_LISTENING_TO, radio == TEXT_LISTENING_TO ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_CONTACT_TIME, radio == TEXT_CONTACT_TIME ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_TEXT, radio == TEXT_TEXT ? BST_CHECKED : BST_UNCHECKED );

			CheckDlgButton(hwndDlg, IDC_XSTATUS_HAS_PRIORITY, DBGetContactSettingByte(NULL,"CList","SecondLineXStatusHasPriority",SETTING_SECONDLINE_XSTATUS_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOW_STATUS_IF_NOAWAY, DBGetContactSettingByte(NULL,"CList","SecondLineShowStatusIfNoAway",SETTING_SECONDLINE_STATUSIFNOAWAY_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOW_LISTENING_IF_NOAWAY, DBGetContactSettingByte(NULL,"CList","SecondLineShowListeningIfNoAway",SETTING_SECONDLINE_LISTENINGIFNOAWAY_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_USE_NAME_AND_MESSAGE, DBGetContactSettingByte(NULL,"CList","SecondLineUseNameAndMessageForXStatus",SETTING_SECONDLINE_XSTATUSNAMETEXT_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED);

			if (!IsDlgButtonChecked(hwndDlg,IDC_SHOW))
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_PROTOCOL_SMILEYS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOP_SPACE_SPIN),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOP_SPACE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_DRAW_SMILEYS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATUS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_NICKNAME),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATUS_MESSAGE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_LISTENING_TO),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_CONTACT_TIME),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TEXT),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_TOP),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_PIXELS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC_TEXT),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_VARIABLES_L),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_STATUS_IF_NOAWAY),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_LISTENING_IF_NOAWAY),FALSE);
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
				{
					EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_STATUS_IF_NOAWAY),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_LISTENING_IF_NOAWAY),FALSE);
				}
			}

			CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_DRAW_SMILEYS), ServiceExists(MS_SMILEYADD_PARSE) ? SW_SHOW : SW_HIDE);
			CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_VARIABLES_L), ServiceExists(MS_VARS_FORMATSTRING) ? SW_SHOW : SW_HIDE);

			break;
		}
	case WM_COMMAND:
		{
			if (LOWORD(wParam)==IDC_TEXT || LOWORD(wParam)==IDC_STATUS || LOWORD(wParam)==IDC_NICKNAME || LOWORD(wParam)==IDC_STATUS_MESSAGE
				|| LOWORD(wParam)==IDC_LISTENING_TO || LOWORD(wParam)==IDC_CONTACT_TIME)
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
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_LISTENING_IF_NOAWAY), IsDlgButtonChecked(hwndDlg,IDC_SHOW) && 
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
				EnableWindow(GetDlgItem(hwndDlg,IDC_LISTENING_TO),enabled);
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
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_LISTENING_IF_NOAWAY), enabled && IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE));
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_NAME_AND_MESSAGE), enabled && (IsDlgButtonChecked(hwndDlg,IDC_STATUS) 
					|| IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE)));			
			}

			if (LOWORD(wParam)==IDC_TOP_SPACE && HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0; // dont make apply enabled during buddy set crap

			SendMessage((GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
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
							else if (IsDlgButtonChecked(hwndDlg,IDC_LISTENING_TO))
								radio = TEXT_LISTENING_TO;
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
							DBWriteContactSettingByte(NULL,"CList","SecondLineShowListeningIfNoAway", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOW_LISTENING_IF_NOAWAY));
							DBWriteContactSettingByte(NULL,"CList","SecondLineUseNameAndMessageForXStatus", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_USE_NAME_AND_MESSAGE));
							ClcOptionsChanged();
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
            TranslateDialogDefault(hwndDlg);
			CheckDlgButton(hwndDlg, IDC_SHOW, DBGetContactSettingByte(NULL,"CList","ThirdLineShow",SETTING_THIRDLINE_SHOW_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );

			SendDlgItemMessage(hwndDlg,IDC_TOP_SPACE_SPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_TOP_SPACE),0);		// set buddy			
			SendDlgItemMessage(hwndDlg,IDC_TOP_SPACE_SPIN,UDM_SETRANGE,0,MAKELONG(255,0));
			SendDlgItemMessage(hwndDlg,IDC_TOP_SPACE_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","ThirdLineTopSpace",SETTING_THIRDLINE_TOPSPACE_DEFAULT),0));

			CheckDlgButton(hwndDlg, IDC_DRAW_SMILEYS, DBGetContactSettingByte(NULL,"CList","ThirdLineDrawSmileys",SETTING_THIRDLINE_SMILEYS_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );

			{
				DBVARIANT dbv={0};

				if (!DBGetContactSettingTString(NULL, "CList","ThirdLineText", &dbv))
				{
					SetWindowText(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT), dbv.ptszVal);
					DBFreeVariant(&dbv);
				}
			}
			SendMessage(GetDlgItem(hwndDlg,IDC_VARIABLE_TEXT), EM_SETLIMITTEXT, TEXT_TEXT_MAX_LENGTH, 0);

			radio = DBGetContactSettingWord(NULL,"CList","ThirdLineType",SETTING_THIRDLINE_TYPE_DEFAULT);

			CheckDlgButton(hwndDlg, IDC_STATUS, radio == TEXT_STATUS ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_NICKNAME, radio == TEXT_NICKNAME ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_STATUS_MESSAGE, radio == TEXT_STATUS_MESSAGE ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_LISTENING_TO, radio == TEXT_LISTENING_TO ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_CONTACT_TIME, radio == TEXT_CONTACT_TIME ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton(hwndDlg, IDC_TEXT, radio == TEXT_TEXT ? BST_CHECKED : BST_UNCHECKED );

			CheckDlgButton(hwndDlg, IDC_XSTATUS_HAS_PRIORITY, DBGetContactSettingByte(NULL,"CList","ThirdLineXStatusHasPriority",SETTING_THIRDLINE_XSTATUS_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOW_STATUS_IF_NOAWAY, DBGetContactSettingByte(NULL,"CList","ThirdLineShowStatusIfNoAway",SETTING_THIRDLINE_STATUSIFNOAWAY_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOW_LISTENING_IF_NOAWAY, DBGetContactSettingByte(NULL,"CList","ThirdLineShowListeningIfNoAway",SETTING_THIRDLINE_LISTENINGIFNOAWAY_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_USE_NAME_AND_MESSAGE, DBGetContactSettingByte(NULL,"CList","ThirdLineUseNameAndMessageForXStatus",SETTING_THIRDLINE_XSTATUSNAMETEXT_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED);

			if (!IsDlgButtonChecked(hwndDlg,IDC_SHOW))
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_USE_PROTOCOL_SMILEYS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOP_SPACE_SPIN),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOP_SPACE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_DRAW_SMILEYS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATUS),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_NICKNAME),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATUS_MESSAGE),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_LISTENING_TO),FALSE);
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
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_LISTENING_IF_NOAWAY),FALSE);
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
				{
					EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_STATUS_IF_NOAWAY),FALSE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_LISTENING_IF_NOAWAY),FALSE);
				}
			}

			CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_DRAW_SMILEYS), ServiceExists(MS_SMILEYADD_PARSE) ? SW_SHOW : SW_HIDE);
			CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_VARIABLES_L), ServiceExists(MS_VARS_FORMATSTRING) ? SW_SHOW : SW_HIDE);

			break;
		}
	case WM_COMMAND:
		{
			if (LOWORD(wParam)==IDC_TEXT || LOWORD(wParam)==IDC_STATUS || LOWORD(wParam)==IDC_NICKNAME || LOWORD(wParam)==IDC_STATUS_MESSAGE
				|| LOWORD(wParam)==IDC_LISTENING_TO || LOWORD(wParam)==IDC_CONTACT_TIME)
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
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_LISTENING_IF_NOAWAY), IsDlgButtonChecked(hwndDlg,IDC_SHOW) && (IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE)));																	
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
				EnableWindow(GetDlgItem(hwndDlg,IDC_LISTENING_TO),enabled);
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
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOW_LISTENING_IF_NOAWAY), enabled && IsDlgButtonChecked(hwndDlg,IDC_STATUS_MESSAGE));
			}

			if (LOWORD(wParam)==IDC_TOP_SPACE && HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0; // dont make apply enabled during buddy set crap

			SendMessage((GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
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
							else if (IsDlgButtonChecked(hwndDlg,IDC_LISTENING_TO))
								radio = TEXT_LISTENING_TO;
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
							DBWriteContactSettingByte(NULL,"CList","ThirdLineShowListeningIfNoAway", (BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOW_LISTENING_IF_NOAWAY));
							ClcOptionsChanged();
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
/* Extra icons settings */
typedef struct _OrderTreeData 
{ 
	BYTE	ID;
	TCHAR *	Name;
	BYTE	Position;
	char *  KeyName;
	BOOL	Visible;
    BOOL    fReserved;
} *pOrderTreeData, TOrderTreeData;

static TOrderTreeData OrderTreeData[]=
{
	{EXTRA_ICON_VISMODE, _T("Visibility/Chat activity"), EXTRA_ICON_VISMODE,  "EXTRA_ICON_VISMODE", TRUE,0},
	{EXTRA_ICON_EMAIL, _T("E-mail"), EXTRA_ICON_EMAIL, "EXTRA_ICON_EMAIL", TRUE, 0},
	{EXTRA_ICON_PROTO, _T("Protocol"), EXTRA_ICON_PROTO, "EXTRA_ICON_PROTO", TRUE, 0},
	{EXTRA_ICON_SMS, _T("Phone/SMS"), EXTRA_ICON_SMS,	 "EXTRA_ICON_SMS", TRUE, 0},
	{EXTRA_ICON_WEB, _T("Web page"), EXTRA_ICON_WEB,   "EXTRA_ICON_WEB", TRUE, 0},
	{EXTRA_ICON_CLIENT, _T("Client (fingerprint.dll is required)"), EXTRA_ICON_CLIENT,"EXTRA_ICON_CLIENT", TRUE, 0},
	{EXTRA_ICON_ADV1, _T("Advanced #1"), EXTRA_ICON_ADV1,  "EXTRA_ICON_ADV1", TRUE, 0},
	{EXTRA_ICON_ADV2, _T("Advanced #2"), EXTRA_ICON_ADV2,  "EXTRA_ICON_ADV2", TRUE, 0},
	{EXTRA_ICON_ADV3, _T("Advanced #3"), EXTRA_ICON_ADV3,  "EXTRA_ICON_ADV3", TRUE, 0},
	{EXTRA_ICON_ADV4, _T("Advanced #4"), EXTRA_ICON_ADV4,	"EXTRA_ICON_ADV4", TRUE, 0}
};


/*
int ReserveExtraSlot(WPARAM wParam,LPARAM lParam)
{      
    int iLastFree=-1;
    EXTRASLOTINFO * pESINFO=(EXTRASLOTINFO *)wParam;
    if (pESINFO->iSlot==0)
    {
        int i;
        for (i=8;i<SIZEOF(OrderTreeData);i++)
            if (OrderTreeData[i].fReserved==0)
            {  iLastFree=i; break; }       
    }
    else if (pESINFO->iSlot<SIZEOF(OrderTreeData)) 
            iLastFree=pESINFO->iSlot;
    if (iLastFree>0)
    {
        if (!OrderTreeData[iLastFree].fReserved)
        {
            OrderTreeData[iLastFree].fReserved=TRUE;
            OrderTreeData[iLastFree].KeyName=pESINFO->pszSlotID;
#ifdef UNICODE
            if (pESINFO->fUnicode)
                OrderTreeData[iLastFree].Name=pESINFO->ptszSlotName;
            else
            {
                OrderTreeData[iLastFree].Name=mir_a2u(pESINFO->pszSlotName); //potential memory leak but should not be more than once
                OrderTreeData[iLastFree].fReserved=(BOOL)2; //need to free KeyName
            }
#else
            if (pESINFO->fUnicode)
            {
                OrderTreeData[iLastFree].Name=mir_u2a(pESINFO->pszSlotName); //potential memory leak but should not be more than once
                OrderTreeData[iLastFree].fReserved=(BOOL)2; //need to free KeyName
            }
            else
                OrderTreeData[iLastFree].Name=pESINFO->pszSlotName; 
#endif
            //ExtraImage_ReloadExtraIcons();
        }
        else return -1; // Already reserved
    }
    return iLastFree;
};
*/

#define PrVer 4

static char **settingname;
static int nArrayLen;

static int OrderEnumProc (const char *szSetting,LPARAM lParam)
{

	if (szSetting==NULL) return 0;
	if (!wildcmp((char*) szSetting,(char *) lParam,0)) return 0;
	nArrayLen++;
	settingname=(char **)realloc(settingname,nArrayLen*sizeof(char *));
	settingname[nArrayLen-1]=_strdup(szSetting);
	return 0;
};

static int  DeleteAllSettingInOrder()
{
	DBCONTACTENUMSETTINGS dbces;
	nArrayLen=0;

	dbces.pfnEnumProc=OrderEnumProc;
	dbces.szModule=CLUIFrameModule;
	dbces.ofsSettings=0;
	dbces.lParam=(LPARAM)"ORDER_EXTRA_*";

	CallService(MS_DB_CONTACT_ENUMSETTINGS,0,(LPARAM)&dbces);

	//delete all settings
	if (nArrayLen==0){return(0);};
	{
		int i;
		for (i=0;i<nArrayLen;i++)
		{
			DBDeleteContactSetting(0,CLUIFrameModule,settingname[i]);
			free(settingname[i]);
		};
		free(settingname);
		settingname=NULL;
	};
	return(0);
};
static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	pOrderTreeData A=(pOrderTreeData)lParam1;
	pOrderTreeData B=(pOrderTreeData)lParam2;
	return (int)(A->Position)-(int)(B->Position);
}

static int FillOrderTree(HWND hwndDlg, HWND Tree)
{
	int i=0;
	TVINSERTSTRUCT tvis={0};
	TreeView_DeleteAllItems(Tree);
	tvis.hParent=NULL;
	tvis.hInsertAfter=TVI_LAST;
	tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;	
	if (DBGetContactSettingByte(NULL,CLUIFrameModule,"ExtraCountStored",0)!=SIZEOF(OrderTreeData))
	{
		DeleteAllSettingInOrder();
		DBWriteContactSettingByte(NULL,CLUIFrameModule,"ExtraCountStored",SIZEOF(OrderTreeData));
	}
	for (i=0; i<SIZEOF(OrderTreeData); i++)
	{
		char buf[256];
		sprintf(buf,"ORDER_%s",OrderTreeData[i].KeyName);
		OrderTreeData[i].Position=(BYTE)DBGetContactSettingByte(NULL,CLUIFrameModule,buf,i);
		OrderTreeData[i].Visible =(BOOL)DBGetContactSettingByte(NULL,CLUIFrameModule,OrderTreeData[i].KeyName,1);
		tvis.hInsertAfter=TVI_LAST;
		tvis.item.lParam=(LPARAM)(&(OrderTreeData[i]));
		tvis.item.pszText=TranslateTS(OrderTreeData[i].Name);
		tvis.item.iImage=tvis.item.iSelectedImage=OrderTreeData[i].Visible;
		TreeView_InsertItem(Tree,&tvis);
	}
	{
		TVSORTCB sort={0};
		sort.hParent=NULL;
		sort.lParam=0;
		sort.lpfnCompare=CompareFunc;
		TreeView_SortChildrenCB(Tree,&sort,0);
	}
	return 0;
};
static int SaveOrderTree(HWND hwndDlg, HWND Tree)
{
	HTREEITEM ht;
	BYTE pos=0;
	TVITEMA tvi={0};
	tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	ht=TreeView_GetRoot(Tree);
	do
	{
		TOrderTreeData * it=NULL;
		tvi.hItem=ht;
		TreeView_GetItemA(Tree,&tvi);
		it=(TOrderTreeData*)(tvi.lParam);
		{
			char buf[250];
			sprintf(buf,"ORDER_%s",it->KeyName);
			DBWriteContactSettingByte(NULL,CLUIFrameModule,buf,pos);
			DBWriteContactSettingByte(NULL,CLUIFrameModule,it->KeyName, it->Visible);	
		}
		ht=TreeView_GetNextSibling(Tree,ht);
		pos++;
	}while (ht);

	return 0;
}

int LoadPositionsFromDB(BYTE * OrderPos)
{
	int i=0;
	if (DBGetContactSettingByte(NULL,CLUIFrameModule,"ExtraCountStored",0)!=SIZEOF(OrderTreeData))
	{
		DeleteAllSettingInOrder();
		DBWriteContactSettingByte(NULL,CLUIFrameModule,"ExtraCountStored",SIZEOF(OrderTreeData));
	}

	for (i=0; i<SIZEOF(OrderTreeData); i++)
	{
		char buf[256];
		sprintf(buf,"ORDER_%s",OrderTreeData[i].KeyName);
		OrderPos[OrderTreeData[i].ID-1]=(BYTE)DBGetContactSettingByte(NULL,CLUIFrameModule,buf,i);
	}
	return 0;
}
static int dragging=0;
static HANDLE hDragItem=NULL;

BOOL CALLBACK DlgProcExtraIconsOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_EXTRAORDER),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_EXTRAORDER),GWL_STYLE)|TVS_NOHSCROLL);

			{	
				HIMAGELIST himlCheckBoxes;
				himlCheckBoxes=ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,2,2);
				ImageList_AddIcon(himlCheckBoxes,LoadSmallIconShared(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_NOTICK)));
				ImageList_AddIcon(himlCheckBoxes,LoadSmallIconShared(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_TICK)));
				TreeView_SetImageList(GetDlgItem(hwndDlg,IDC_EXTRAORDER),himlCheckBoxes,TVSIL_NORMAL);
			}
			FillOrderTree(hwndDlg,GetDlgItem(hwndDlg,IDC_EXTRAORDER));
			return TRUE;
		}

	case WM_COMMAND:
		{
			SendMessage((GetParent(hwndDlg)), PSM_CHANGED, (WPARAM)hwndDlg, 0);
			break;
		}
	case WM_NOTIFY:
		{
			switch(((LPNMHDR)lParam)->idFrom) 
			{		
			case 0: 
				{
					switch (((LPNMHDR)lParam)->code)
					{
					case PSN_APPLY:
						{
							SaveOrderTree(hwndDlg,GetDlgItem(hwndDlg,IDC_EXTRAORDER));      
							ReAssignExtraIcons();	
							CLUI_ReloadCLUIOptions();
							ClcOptionsChanged();
							return TRUE;
						}
					}
				}
			case IDC_EXTRAORDER:
				{
					switch (((LPNMHDR)lParam)->code) 
					{
					case TVN_BEGINDRAGA:
					case TVN_BEGINDRAGW:
						SetCapture(hwndDlg);
						dragging=1;
						hDragItem=((LPNMTREEVIEWA)lParam)->itemNew.hItem;
						TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_EXTRAORDER),hDragItem);
						break;
					case NM_CLICK:
						{						
							TVHITTESTINFO hti;
							hti.pt.x=(short)LOWORD(GetMessagePos());
							hti.pt.y=(short)HIWORD(GetMessagePos());
							ScreenToClient(((LPNMHDR)lParam)->hwndFrom,&hti.pt);
							if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom,&hti))
								if(hti.flags&TVHT_ONITEMICON) 
								{
									TVITEMA tvi;
									tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
									tvi.hItem=hti.hItem;
									TreeView_GetItemA(((LPNMHDR)lParam)->hwndFrom,&tvi);
									tvi.iImage=tvi.iSelectedImage=!tvi.iImage;
									((TOrderTreeData *)tvi.lParam)->Visible=tvi.iImage;
									TreeView_SetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
									SendMessage((GetParent(hwndDlg)), PSM_CHANGED, (WPARAM)hwndDlg, 0);
								}

						};
					}
					break;
				}
			} 
			break;
		}
	case WM_MOUSEMOVE:
		{
			if(!dragging) break;
			{	
				TVHITTESTINFO hti;
				hti.pt.x=(short)LOWORD(lParam);
				hti.pt.y=(short)HIWORD(lParam);
				ClientToScreen(hwndDlg,&hti.pt);
				ScreenToClient(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&hti.pt);
				TreeView_HitTest(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&hti);
				if(hti.flags&(TVHT_ONITEM|TVHT_ONITEMRIGHT))
				{
					HTREEITEM it=hti.hItem;
					hti.pt.y-=TreeView_GetItemHeight(GetDlgItem(hwndDlg,IDC_EXTRAORDER))/2;
					TreeView_HitTest(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&hti);
					//TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_EXTRAORDER),hti.hItem,1);
					if (!(hti.flags&TVHT_ABOVE))
						TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_EXTRAORDER),hti.hItem,1);
					else 
						TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_EXTRAORDER),it,0);
				}
				else 
				{
					if(hti.flags&TVHT_ABOVE) SendDlgItemMessage(hwndDlg,IDC_EXTRAORDER,WM_VSCROLL,MAKEWPARAM(SB_LINEUP,0),0);
					if(hti.flags&TVHT_BELOW) SendDlgItemMessage(hwndDlg,IDC_EXTRAORDER,WM_VSCROLL,MAKEWPARAM(SB_LINEDOWN,0),0);
					TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_EXTRAORDER),NULL,0);
				}
			}	
		}
		break;
	case WM_LBUTTONUP:
		{
			if(!dragging) break;
			TreeView_SetInsertMark(GetDlgItem(hwndDlg,IDC_EXTRAORDER),NULL,0);
			dragging=0;
			ReleaseCapture();
			{	
				TVHITTESTINFO hti;
				TVITEM tvi;
				hti.pt.x=(short)LOWORD(lParam);
				hti.pt.y=(short)HIWORD(lParam);
				ClientToScreen(hwndDlg,&hti.pt);
				ScreenToClient(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&hti.pt);
				hti.pt.y-=TreeView_GetItemHeight(GetDlgItem(hwndDlg,IDC_EXTRAORDER))/2;
				TreeView_HitTest(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&hti);
				if(hDragItem==hti.hItem) break;
				if (hti.flags&TVHT_ABOVE) hti.hItem=TVI_FIRST;
				tvi.mask=TVIF_HANDLE|TVIF_PARAM;
				tvi.hItem=hDragItem;
				TreeView_GetItem(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&tvi);
				if(hti.flags&(TVHT_ONITEM|TVHT_ONITEMRIGHT)||(hti.hItem==TVI_FIRST)) 
				{
					TVINSERTSTRUCT tvis;
					TCHAR name[128];
					tvis.item.mask=TVIF_HANDLE|TVIF_PARAM|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
					tvis.item.stateMask=0xFFFFFFFF;
					tvis.item.pszText=name;
					tvis.item.cchTextMax=sizeof(name);
					tvis.item.hItem=hDragItem;
					tvis.item.iImage=tvis.item.iSelectedImage=((TOrderTreeData *)tvi.lParam)->Visible;				
					TreeView_GetItem(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&tvis.item);				
					TreeView_DeleteItem(GetDlgItem(hwndDlg,IDC_EXTRAORDER),hDragItem);
					tvis.hParent=NULL;
					tvis.hInsertAfter=hti.hItem;
					TreeView_SelectItem(GetDlgItem(hwndDlg,IDC_EXTRAORDER),TreeView_InsertItem(GetDlgItem(hwndDlg,IDC_EXTRAORDER),&tvis));
					SendMessage((GetParent(hwndDlg)), PSM_CHANGED, (WPARAM)hwndDlg, 0);
				}
			}
		}
		break; 
	}
	return FALSE;
}
