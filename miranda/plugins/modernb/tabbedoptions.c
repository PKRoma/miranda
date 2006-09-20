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
#include "SkinEngine.h"
#include "io.h"
#include "commonprototypes.h"

/************************************************************************/
/*   File implement routines to join setting pages in tabs              */
/************************************************************************/

/* TABBED SKIN SETTINGS */



typedef struct _TabItemOptionData 
{ 
	TabItemOptionConf *conf;
	HWND hwnd;				// dialog handle
} TabItemOptionData; 

typedef struct _TabWndItemsData 
{ 
	TabItemOptionData * items;
	int allocedItems;
	HWND hwndDisplay;
	int selected_item;
} TabWndItemsData; 

TabItemOptionConf * tab_opt_items=NULL;
int tab_opt_items_count=0;
BOOL CALLBACK DlgProcTabbedOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

int CreateTabPage(char *Group, char * Title, WPARAM wParam, DLGPROC DlgProcOpts)
{
	OPTIONSDIALOGPAGE odp;
	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=-100000000;
	odp.hInstance=g_hInst;
	odp.pfnDlgProc=DlgProcOpts;
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_TAB);
	odp.pszGroup=Group;
	odp.pszTitle=Title;
	odp.flags=ODPF_BOLDGROUPS;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}

// DoLockDlgRes - loads and locks a dialog template resource. 
// Returns the address of the locked resource. 
// lpszResName - name of the resource 

static DLGTEMPLATE * DoLockDlgRes(LPSTR lpszResName) 
{ 
	HRSRC hrsrc = FindResourceA(g_hInst, lpszResName, MAKEINTRESOURCEA(5)); 
	HGLOBAL hglb = LoadResource(g_hInst, hrsrc); 
	return (DLGTEMPLATE *) LockResource(hglb); 
} 

static void TabChangeTab(HWND hwndDlg, TabWndItemsData *data, int sel)
{
	HWND hwndTab;
	RECT rc_tab;
	RECT rc_item;

	hwndTab = GetDlgItem(hwndDlg, IDC_SKIN_TAB);

	// Get avaible space
	GetWindowRect(hwndTab, &rc_tab);
	ScreenToClientRect(hwndTab, &rc_tab);
	TabCtrl_AdjustRect(hwndTab, FALSE, &rc_tab); 

	// Get item size
	GetClientRect(data->items[sel].hwnd, &rc_item);

	// Fix rc_item
	rc_item.right -= rc_item.left;	// width
	rc_item.left = rc_tab.left + (rc_tab.right - rc_tab.left - rc_item.right) / 2;

	rc_item.bottom -= rc_item.top;	// height
	rc_item.top = rc_tab.top; // + (rc_tab.bottom - rc_tab.top - rc_item.bottom) / 2;

	// Set pos
	SetWindowPos(data->items[sel].hwnd, HWND_TOP, rc_item.left, rc_item.top, rc_item.right,
		rc_item.bottom, SWP_SHOWWINDOW);

	data->selected_item = sel;
}


BOOL CALLBACK DlgProcTabbedOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		{
			HWND hwndTab;
			TabWndItemsData *data;
			int i;
			TCITEM tie={0}; 

			TranslateDialogDefault(hwndDlg);
			tab_opt_items_count=wParam;
			tab_opt_items=(TabItemOptionConf*)lParam;
			data = (TabWndItemsData *) mir_alloc(sizeof(TabWndItemsData));
			data->items=(TabItemOptionData*)mir_alloc(sizeof(TabItemOptionData)*tab_opt_items_count);
			data->allocedItems=tab_opt_items_count;
			data->selected_item = -1;
			SetWindowLong(hwndDlg, GWL_USERDATA, (long) data); 

			hwndTab = GetDlgItem(hwndDlg, IDC_SKIN_TAB);

			// Add tabs
			tie.mask = TCIF_TEXT | TCIF_IMAGE; 
			tie.iImage = -1; 

			for (i = 0 ; i < tab_opt_items_count; i++)
			{
				DLGTEMPLATE *templ;

				data->items[i].conf = tab_opt_items+i;
				templ = DoLockDlgRes(MAKEINTRESOURCEA(data->items[i].conf->id));
				data->items[i].hwnd = CreateDialogIndirectA(g_hInst, templ, hwndDlg, 
					data->items[i].conf->wnd_proc); 
				TranslateDialogDefault(data->items[i].hwnd);
				CLUI_ShowWindowMod(data->items[i].hwnd, SW_HIDE);

				if (pfEnableThemeDialogTexture != NULL)
					pfEnableThemeDialogTexture(data->items[i].hwnd, ETDT_ENABLETAB);

				tie.pszText = TranslateTS(data->items[i].conf->name); 
				TabCtrl_InsertItem(hwndTab, i, &tie);
			}

			// Show first item
			TabChangeTab(hwndDlg, data, 0);
			return TRUE;

		}
	case WM_NOTIFY: 
		{
			switch (((LPNMHDR)lParam)->code) 
			{
			case TCN_SELCHANGING:
				{
					TabWndItemsData *data = (TabWndItemsData *) GetWindowLong(hwndDlg, GWL_USERDATA);
					CLUI_ShowWindowMod(data->items[data->selected_item].hwnd, SW_HIDE);
					break;
				}
			case TCN_SELCHANGE: 
				{
					TabChangeTab(hwndDlg, 
						(TabWndItemsData *)GetWindowLong(hwndDlg, GWL_USERDATA), 
						TabCtrl_GetCurSel(GetDlgItem(hwndDlg, IDC_SKIN_TAB)));
					break; 
				}

			case PSN_APPLY:
				{
					if (((LPNMHDR)lParam)->idFrom == 0)
					{
						TabWndItemsData *data = (TabWndItemsData *) GetWindowLong(hwndDlg, GWL_USERDATA);
						int i;
						for (i = 0 ; i < data->allocedItems ; i++)
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
			TabWndItemsData *data = (TabWndItemsData *) GetWindowLong(hwndDlg, GWL_USERDATA);

			DestroyWindow(data->hwndDisplay); 

			for (i = 0 ; i < data->allocedItems ; i++)
			{
				DestroyWindow(data->items[i].hwnd); 
				
			}
			mir_free(data->items); 
			mir_free(data); 
			break;
		}
	case PSM_CHANGED:
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
	}

	return 0;
}