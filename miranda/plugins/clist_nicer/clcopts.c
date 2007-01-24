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

UNICODE done

*/
#include "commonheaders.h"
#include "coolsb/coolscroll.h"
#include <uxtheme.h>

#define DBFONTF_BOLD       1
#define DBFONTF_ITALIC     2
#define DBFONTF_UNDERLINE  4

static BOOL CALLBACK DlgProcClcMainOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcClcBkgOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
//static BOOL CALLBACK DlgProcClcTextOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcViewModesSetup(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcFloatingContacts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK OptionsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcCluiOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcSBarOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcPlusOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcGenOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcHotkeyOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL (WINAPI *MyEnableThemeDialogTexture)(HANDLE, DWORD);
extern void ReloadExtraIcons( void );
       
extern struct CluiData g_CluiData;
extern int    g_nextExtraCacheEntry;
extern struct ExtraCache *g_ExtraCache;
extern HIMAGELIST himlExtraImages;

void GetDefaultFontSetting(int i, LOGFONT *lf, COLORREF *colour)
{
	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), lf, FALSE);
	*colour = GetSysColor(COLOR_WINDOWTEXT);
	switch (i) {
	case FONTID_GROUPS:
		lf->lfWeight = FW_BOLD;
		break;
	case FONTID_GROUPCOUNTS:
		lf->lfHeight = (int) (lf->lfHeight * .75);
		*colour = GetSysColor(COLOR_3DSHADOW);
		break;
	case FONTID_OFFINVIS:
	case FONTID_INVIS:
		lf->lfItalic = !lf->lfItalic;
		break;
	case FONTID_DIVIDERS:
		lf->lfHeight = (int) (lf->lfHeight * .75);
		break;
	case FONTID_NOTONLIST:
		*colour = GetSysColor(COLOR_3DSHADOW);
		break;
}	}

static int opt_xicons_changed_flag = 0;

BOOL CALLBACK DlgProcXIcons(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);

			CheckDlgButton(hwndDlg, IDC_SHOWCLIENTICONS, g_CluiData.dwExtraImageMask & EIMG_SHOW_CLIENT);
			CheckDlgButton(hwndDlg, IDC_SHOWEXTENDEDSTATUS, g_CluiData.dwExtraImageMask & EIMG_SHOW_EXTRA);
			CheckDlgButton(hwndDlg, IDC_EXTRAMAIL, g_CluiData.dwExtraImageMask & EIMG_SHOW_MAIL);
			CheckDlgButton(hwndDlg, IDC_EXTRAWEB, g_CluiData.dwExtraImageMask & EIMG_SHOW_URL);
			CheckDlgButton(hwndDlg, IDC_EXTRAPHONE, g_CluiData.dwExtraImageMask & EIMG_SHOW_SMS);
			CheckDlgButton(hwndDlg, IDC_EXTRARESERVED, g_CluiData.dwExtraImageMask & EIMG_SHOW_RESERVED);
			CheckDlgButton(hwndDlg, IDC_EXTRARESERVED2, g_CluiData.dwExtraImageMask & EIMG_SHOW_RESERVED2);
            CheckDlgButton(hwndDlg, IDC_EXTRARESERVED3, g_CluiData.dwExtraImageMask & EIMG_SHOW_RESERVED3);
            CheckDlgButton(hwndDlg, IDC_EXTRARESERVED4, g_CluiData.dwExtraImageMask & EIMG_SHOW_RESERVED4);
            CheckDlgButton(hwndDlg, IDC_EXTRARESERVED5, g_CluiData.dwExtraImageMask & EIMG_SHOW_RESERVED5);
			SendDlgItemMessage(hwndDlg, IDC_EXICONSCALESPIN, UDM_SETRANGE, 0, MAKELONG(20, 8));
			SendDlgItemMessage(hwndDlg, IDC_EXICONSCALESPIN, UDM_SETPOS, 0, (LPARAM)g_CluiData.exIconScale);
            opt_xicons_changed_flag = 0;
			return TRUE;
		}
	case WM_COMMAND:
		if ((LOWORD(wParam) == IDC_EXICONSCALE) && (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
			return 0;
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
        opt_xicons_changed_flag = 1;
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->code) {
		case PSN_APPLY:
			{
				int   oldexIconScale = g_CluiData.exIconScale;
                DWORD oldMask = g_CluiData.dwExtraImageMask;

                int      i;

                if(!opt_xicons_changed_flag)
                    return TRUE;

				g_CluiData.dwExtraImageMask = (IsDlgButtonChecked(hwndDlg, IDC_EXTRAMAIL) ? EIMG_SHOW_MAIL : 0) |
					(IsDlgButtonChecked(hwndDlg, IDC_EXTRAWEB) ? EIMG_SHOW_URL : 0) |
					(IsDlgButtonChecked(hwndDlg, IDC_SHOWEXTENDEDSTATUS) ? EIMG_SHOW_EXTRA : 0) |
					(IsDlgButtonChecked(hwndDlg, IDC_EXTRAPHONE) ? EIMG_SHOW_SMS : 0) |
					(IsDlgButtonChecked(hwndDlg, IDC_EXTRARESERVED) ? EIMG_SHOW_RESERVED : 0) |
					(IsDlgButtonChecked(hwndDlg, IDC_SHOWCLIENTICONS) ? EIMG_SHOW_CLIENT : 0) |
					(IsDlgButtonChecked(hwndDlg, IDC_EXTRARESERVED2) ? EIMG_SHOW_RESERVED2 : 0) |
                    (IsDlgButtonChecked(hwndDlg, IDC_EXTRARESERVED3) ? EIMG_SHOW_RESERVED3 : 0) |
                    (IsDlgButtonChecked(hwndDlg, IDC_EXTRARESERVED4) ? EIMG_SHOW_RESERVED4 : 0) |
                    (IsDlgButtonChecked(hwndDlg, IDC_EXTRARESERVED5) ? EIMG_SHOW_RESERVED5 : 0);

                for(i = 0; i < g_nextExtraCacheEntry; i++)
                    g_ExtraCache[i].dwXMask = CalcXMask(g_ExtraCache[i].hContact);

				DBWriteContactSettingDword(NULL, "CLUI", "ximgmask", g_CluiData.dwExtraImageMask);

				g_CluiData.exIconScale = SendDlgItemMessage(hwndDlg, IDC_EXICONSCALESPIN, UDM_GETPOS, 0, 0);
				g_CluiData.exIconScale = (g_CluiData.exIconScale < 8 || g_CluiData.exIconScale > 20) ? 16 : g_CluiData.exIconScale;

				DBWriteContactSettingByte(NULL, "CLC", "ExIconScale", (BYTE)g_CluiData.exIconScale);

				if(oldexIconScale != g_CluiData.exIconScale) {
					ImageList_RemoveAll(himlExtraImages);
					ImageList_SetIconSize(himlExtraImages, g_CluiData.exIconScale, g_CluiData.exIconScale);
					if(g_CluiData.IcoLib_Avail)
						IcoLibReloadIcons();
					else {
						CLN_LoadAllIcons(0);
						pcli->pfnReloadProtoMenus();
                        //FYR: Not necessary. It is already notified in pfnReloadProtoMenus
                        //NotifyEventHooks(pcli->hPreBuildStatusMenuEvent, 0, 0);
						ReloadExtraIcons();
					}
					pcli->pfnClcBroadcast(CLM_AUTOREBUILD, 0, 0);
				}
                if(oldMask != g_CluiData.dwExtraImageMask)
                    pcli->pfnClcBroadcast(CLM_AUTOREBUILD, 0, 0);

                opt_xicons_changed_flag = 0;
				return TRUE;
			}
		}
		break;
	}
	return FALSE;
}

static BOOL CALLBACK TabOptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static int iInit = TRUE;
   
   switch(msg)
   {
      case WM_INITDIALOG:
      {
         TCITEM tci;
         RECT rcClient;
         int oPage = DBGetContactSettingByte(NULL, "CLUI", "opage", 0);

         GetClientRect(hwnd, &rcClient);
         iInit = TRUE;
         tci.mask = TCIF_PARAM|TCIF_TEXT;
         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_CLIST), hwnd, DlgProcGenOpts);
         tci.pszText = TranslateT("General");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 0, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 0 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_CLC), hwnd, DlgProcClcMainOpts);
         tci.pszText = TranslateT("List layout");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 1, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 1 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_CLUI), hwnd, DlgProcCluiOpts);
         tci.pszText = TranslateT("Window");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 2, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 2 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_CLCBKG), hwnd, DlgProcClcBkgOpts);
         tci.pszText = TranslateT("Background");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 3, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 3 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_SBAR), hwnd, DlgProcSBarOpts);
         tci.pszText = TranslateT("Status Bar");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 4, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 4 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_XICONS), hwnd, DlgProcXIcons);
         tci.pszText = TranslateT("Extra icons");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 5, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 5 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         TabCtrl_SetCurSel(GetDlgItem(hwnd, IDC_OPTIONSTAB), oPage);
         iInit = FALSE;
         return FALSE;
      }
      
       case PSM_CHANGED: // used so tabs dont have to call SendMessage(GetParent(GetParent(hwnd)), PSM_CHANGED, 0, 0);
         if(!iInit)
             SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
         break;
      case WM_NOTIFY:
         switch(((LPNMHDR)lParam)->idFrom) {
            case 0:
               switch (((LPNMHDR)lParam)->code)
               {
                  case PSN_APPLY:
                     {
                        TCITEM tci;
                        int i,count;
                        tci.mask = TCIF_PARAM;
                        count = TabCtrl_GetItemCount(GetDlgItem(hwnd,IDC_OPTIONSTAB));
                        for (i=0;i<count;i++)
                        {
                           TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),i,&tci);
                           SendMessage((HWND)tci.lParam,WM_NOTIFY,0,lParam);
                        }
                     }
                  break;
               }
            break;
            case IDC_OPTIONSTAB:
               switch (((LPNMHDR)lParam)->code)
               {
                  case TCN_SELCHANGING:
                     {
                        TCITEM tci;
                        tci.mask = TCIF_PARAM;
                        TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_OPTIONSTAB)),&tci);
                        ShowWindow((HWND)tci.lParam,SW_HIDE);                     
                     }
                  break;
                  case TCN_SELCHANGE:
                     {
                        TCITEM tci;
                        tci.mask = TCIF_PARAM;
                        TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_OPTIONSTAB)),&tci);
                        ShowWindow((HWND)tci.lParam,SW_SHOW);
                        DBWriteContactSettingByte(NULL, "CLUI", "opage", (BYTE)TabCtrl_GetCurSel(GetDlgItem(hwnd, IDC_OPTIONSTAB)));
                     }
                  break;
               }
            break;

         }
      break;
   }
   return FALSE;
}

int ClcOptInit(WPARAM wParam, LPARAM lParam)
{
    OPTIONSDIALOGPAGE odp;

    ZeroMemory(&odp, sizeof(odp));
    odp.cbSize = sizeof(odp);
    odp.position = 0;
    odp.hInstance = g_hInst;
    odp.pszGroup = Translate("Contact List");

    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_FLOATING);
    odp.pszTitle = Translate("Floating contacts");
    odp.pfnDlgProc = DlgProcFloatingContacts;
    odp.flags = ODPF_BOLDGROUPS | ODPF_EXPERTONLY;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_PLUS);
    odp.pszTitle = Translate("Advanced options");
    odp.pfnDlgProc = DlgProcPlusOpts;
    odp.flags = ODPF_BOLDGROUPS | ODPF_EXPERTONLY;
    odp.nIDBottomSimpleControl = 0;
    odp.nExpertOnlyControls = 0;
    odp.expertOnlyControls = NULL;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT);
    odp.pszGroup = Translate("Customize");
    odp.pszTitle = Translate("Contact list skin");
    odp.flags = ODPF_BOLDGROUPS;
    odp.pfnDlgProc = OptionsDlgProc;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

    odp.position = -1000000000;
    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONSDIALOG);
    odp.pszGroup = NULL;
    odp.pszTitle = Translate("Contact List");
    odp.pfnDlgProc = TabOptionsDlgProc;
    odp.flags = ODPF_BOLDGROUPS;
    odp.nIDBottomSimpleControl = 0;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

    odp.position = -900000000;
    odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_HOTKEY);
    odp.pszTitle = Translate("Hotkeys");
    odp.pszGroup = Translate("Events");
    odp.pfnDlgProc = DlgProcHotkeyOpts;
    odp.nIDBottomSimpleControl = 0;
    odp.nExpertOnlyControls = 0;
    odp.expertOnlyControls = NULL;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);
    return 0;
}

struct CheckBoxToStyleEx_t {
    int id;
    DWORD flag;
    int not;
} static const checkBoxToStyleEx[] = {
    {IDC_DISABLEDRAGDROP,CLS_EX_DISABLEDRAGDROP,0}, {IDC_NOTEDITLABELS,CLS_EX_EDITLABELS,1}, {IDC_SHOWSELALWAYS,CLS_EX_SHOWSELALWAYS,0}, {IDC_TRACKSELECT,CLS_EX_TRACKSELECT,0}, {IDC_SHOWGROUPCOUNTS,CLS_EX_SHOWGROUPCOUNTS,0}, {IDC_HIDECOUNTSWHENEMPTY,CLS_EX_HIDECOUNTSWHENEMPTY,0}, {IDC_DIVIDERONOFF,CLS_EX_DIVIDERONOFF,0}, {IDC_NOTNOTRANSLUCENTSEL,CLS_EX_NOTRANSLUCENTSEL,1}, {IDC_LINEWITHGROUPS,CLS_EX_LINEWITHGROUPS,0}, {IDC_QUICKSEARCHVISONLY,CLS_EX_QUICKSEARCHVISONLY,0}, {IDC_SORTGROUPSALPHA,CLS_EX_SORTGROUPSALPHA,0}, {IDC_NOTNOSMOOTHSCROLLING,CLS_EX_NOSMOOTHSCROLLING,1}
};

struct CheckBoxValues_t {
    DWORD style;
    TCHAR *szDescr;
};

static const struct CheckBoxValues_t greyoutValues[] = {
    {GREYF_UNFOCUS,_T("Not focused")}, {MODEF_OFFLINE,_T("Offline")}, {PF2_ONLINE,_T("Online")}, {PF2_SHORTAWAY,_T("Away")}, {PF2_LONGAWAY,_T("NA")}, {PF2_LIGHTDND,_T("Occupied")}, {PF2_HEAVYDND,_T("DND")}, {PF2_FREECHAT,_T("Free for chat")}, {PF2_INVISIBLE,_T("Invisible")}, {PF2_OUTTOLUNCH,_T("Out to lunch")}, {PF2_ONTHEPHONE,_T("On the phone")}
};
static const struct CheckBoxValues_t offlineValues[] = {
    {MODEF_OFFLINE,_T("Offline")}, {PF2_ONLINE,_T("Online")}, {PF2_SHORTAWAY,_T("Away")}, {PF2_LONGAWAY,_T("NA")}, {PF2_LIGHTDND,_T("Occupied")}, {PF2_HEAVYDND,_T("DND")}, {PF2_FREECHAT,_T("Free for chat")}, {PF2_INVISIBLE,_T("Invisible")}, {PF2_OUTTOLUNCH,_T("Out to lunch")}, {PF2_ONTHEPHONE,_T("On the phone")}
};

static void FillCheckBoxTree(HWND hwndTree, const struct CheckBoxValues_t *values, int nValues, DWORD style)
{
	TVINSERTSTRUCT tvis;
	int i;

	tvis.hParent = NULL;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_STATE;
	for (i = 0; i < nValues; i++) {
		tvis.item.lParam = values[i].style;
		tvis.item.pszText = TranslateTS(values[i].szDescr);
		tvis.item.stateMask = TVIS_STATEIMAGEMASK;
		tvis.item.state = INDEXTOSTATEIMAGEMASK((style & tvis.item.lParam) != 0 ? 2 : 1);
		TreeView_InsertItem(hwndTree, &tvis);
	}
}

static DWORD MakeCheckBoxTreeFlags(HWND hwndTree)
{
    DWORD flags = 0;
    TVITEM tvi;

    tvi.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_STATE;
    tvi.hItem = TreeView_GetRoot(hwndTree);
    while (tvi.hItem) {
        TreeView_GetItem(hwndTree, &tvi);
        if (((tvi.state & TVIS_STATEIMAGEMASK) >> 12 == 2))
            flags |= tvi.lParam;
        tvi.hItem = TreeView_GetNextSibling(hwndTree, tvi.hItem);
    }
    return flags;
}

static int opt_clc_main_changed = 0;

static BOOL CALLBACK DlgProcClcMainOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
            TranslateDialogDefault(hwndDlg);
            opt_clc_main_changed = 0;
            SetWindowLong(GetDlgItem(hwndDlg, IDC_GREYOUTOPTS), GWL_STYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_GREYOUTOPTS), GWL_STYLE) | TVS_NOHSCROLL | TVS_CHECKBOXES);
            SetWindowLong(GetDlgItem(hwndDlg, IDC_HIDEOFFLINEOPTS), GWL_STYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_HIDEOFFLINEOPTS), GWL_STYLE) | TVS_NOHSCROLL | TVS_CHECKBOXES); {
                int i;
                DWORD exStyle = DBGetContactSettingDword(NULL, "CLC", "ExStyle", pcli->pfnGetDefaultExStyle());
                for (i = 0; i < sizeof(checkBoxToStyleEx) / sizeof(checkBoxToStyleEx[0]); i++) {
                    CheckDlgButton(hwndDlg, checkBoxToStyleEx[i].id, (exStyle & checkBoxToStyleEx[i].flag) ^ (checkBoxToStyleEx[i].flag * checkBoxToStyleEx[i].not) ? BST_CHECKED : BST_UNCHECKED);
                }
            } {
                UDACCEL accel[2] = {
                    {0,10}, {2,50}
                };
                SendDlgItemMessage(hwndDlg, IDC_SMOOTHTIMESPIN, UDM_SETRANGE, 0, MAKELONG(999, 0));
                SendDlgItemMessage(hwndDlg, IDC_SMOOTHTIMESPIN, UDM_SETACCEL, sizeof(accel) / sizeof(accel[0]), (LPARAM) &accel);
                SendDlgItemMessage(hwndDlg, IDC_SMOOTHTIMESPIN, UDM_SETPOS, 0, MAKELONG(DBGetContactSettingWord(NULL, "CLC", "ScrollTime", CLCDEFAULT_SCROLLTIME), 0));
            }
            CheckDlgButton(hwndDlg, IDC_IDLE, DBGetContactSettingByte(NULL, "CLC", "ShowIdle", CLCDEFAULT_SHOWIDLE) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_FULLROWSELECT, (g_CluiData.dwFlags & CLUI_FULLROWSELECT) ? BST_CHECKED : BST_UNCHECKED);
            SendDlgItemMessage(hwndDlg, IDC_LEFTMARGINSPIN, UDM_SETRANGE, 0, MAKELONG(64, 0));
            SendDlgItemMessage(hwndDlg, IDC_LEFTMARGINSPIN, UDM_SETPOS, 0, MAKELONG(DBGetContactSettingByte(NULL, "CLC", "LeftMargin", CLCDEFAULT_LEFTMARGIN), 0));
            SendDlgItemMessage(hwndDlg, IDC_RIGHTMARGINSPIN, UDM_SETRANGE, 0, MAKELONG(64, 0));
            SendDlgItemMessage(hwndDlg, IDC_RIGHTMARGINSPIN, UDM_SETPOS, 0, MAKELONG(DBGetContactSettingByte(NULL, "CLC", "RightMargin", CLCDEFAULT_LEFTMARGIN), 0));
            SendDlgItemMessage(hwndDlg, IDC_ROWGAPSPIN, UDM_SETRANGE, 0, MAKELONG(10, 0));
            SendDlgItemMessage(hwndDlg, IDC_ROWGAPSPIN, UDM_SETPOS, 0, g_CluiData.bRowSpacing);
            CheckDlgButton(hwndDlg, IDC_DBLCLKAVATARS, g_CluiData.bDblClkAvatars);
            CheckDlgButton(hwndDlg, IDC_NOGROUPICON, (g_CluiData.dwFlags & CLUI_FRAME_NOGROUPICON) ? BST_CHECKED : BST_UNCHECKED);
            SendDlgItemMessage(hwndDlg, IDC_GROUPINDENTSPIN, UDM_SETRANGE, 0, MAKELONG(50, 0));
            SendDlgItemMessage(hwndDlg, IDC_GROUPINDENTSPIN, UDM_SETPOS, 0, MAKELONG(DBGetContactSettingByte(NULL, "CLC", "GroupIndent", CLCDEFAULT_GROUPINDENT), 0));
            CheckDlgButton(hwndDlg, IDC_GREYOUT, DBGetContactSettingDword(NULL, "CLC", "GreyoutFlags", CLCDEFAULT_GREYOUTFLAGS) ? BST_CHECKED : BST_UNCHECKED);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SMOOTHTIME), IsDlgButtonChecked(hwndDlg, IDC_NOTNOSMOOTHSCROLLING));
            EnableWindow(GetDlgItem(hwndDlg, IDC_GREYOUTOPTS), IsDlgButtonChecked(hwndDlg, IDC_GREYOUT));
            FillCheckBoxTree(GetDlgItem(hwndDlg, IDC_GREYOUTOPTS), greyoutValues, sizeof(greyoutValues) / sizeof(greyoutValues[0]), DBGetContactSettingDword(NULL, "CLC", "FullGreyoutFlags", CLCDEFAULT_FULLGREYOUTFLAGS));
            FillCheckBoxTree(GetDlgItem(hwndDlg, IDC_HIDEOFFLINEOPTS), offlineValues, sizeof(offlineValues) / sizeof(offlineValues[0]), DBGetContactSettingDword(NULL, "CLC", "OfflineModes", CLCDEFAULT_OFFLINEMODES));
            CheckDlgButton(hwndDlg, IDC_NOSCROLLBAR, DBGetContactSettingByte(NULL, "CLC", "NoVScrollBar", 0) ? BST_CHECKED : BST_UNCHECKED);
            SendDlgItemMessage(hwndDlg, IDC_ROWHEIGHTSPIN, UDM_SETRANGE, 0, MAKELONG(255, 8));
            SendDlgItemMessage(hwndDlg, IDC_ROWHEIGHTSPIN, UDM_SETPOS, 0, MAKELONG(DBGetContactSettingByte(NULL, "CLC", "RowHeight", CLCDEFAULT_ROWHEIGHT), 0));
            SendDlgItemMessage(hwndDlg, IDC_GROUPROWHEIGHTSPIN, UDM_SETRANGE, 0, MAKELONG(255, 8));
            SendDlgItemMessage(hwndDlg, IDC_GROUPROWHEIGHTSPIN, UDM_SETPOS, 0, MAKELONG(DBGetContactSettingByte(NULL, "CLC", "GRowHeight", CLCDEFAULT_ROWHEIGHT), 0));
            CheckDlgButton(hwndDlg, IDC_CENTERGROUPNAMES, DBGetContactSettingByte(NULL, "CLCExt", "EXBK_CenterGroupnames", 0));

            SendDlgItemMessage(hwndDlg, IDC_GROUPALIGN, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always Left"));
            SendDlgItemMessage(hwndDlg, IDC_GROUPALIGN, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Always Right"));
            SendDlgItemMessage(hwndDlg, IDC_GROUPALIGN, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Automatic (RTL)"));
			SendDlgItemMessage(hwndDlg, IDC_GROUPALIGN, CB_SETCURSEL, g_CluiData.bGroupAlign, 0);

            return TRUE;
        case WM_VSCROLL:
            opt_clc_main_changed = 1;
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_NOTNOSMOOTHSCROLLING)
                EnableWindow(GetDlgItem(hwndDlg, IDC_SMOOTHTIME), IsDlgButtonChecked(hwndDlg, IDC_NOTNOSMOOTHSCROLLING));
            if (LOWORD(wParam) == IDC_GREYOUT)
                EnableWindow(GetDlgItem(hwndDlg, IDC_GREYOUTOPTS), IsDlgButtonChecked(hwndDlg, IDC_GREYOUT));
            if ((LOWORD(wParam) == IDC_ROWHEIGHT || LOWORD(wParam) == IDC_ROWGAP || LOWORD(wParam) == IDC_RIGHTMARGIN || LOWORD(wParam) == IDC_LEFTMARGIN || LOWORD(wParam) == IDC_SMOOTHTIME || LOWORD(wParam) == IDC_GROUPINDENT || LOWORD(wParam) == IDC_GROUPROWHEIGHT) 
                && (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
                return 0;
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            opt_clc_main_changed = 1;
            break;
        case WM_NOTIFY:
            switch (((LPNMHDR) lParam)->idFrom) {
                case IDC_GREYOUTOPTS:
                case IDC_HIDEOFFLINEOPTS:
                    if (((LPNMHDR) lParam)->code == NM_CLICK) {
                        TVHITTESTINFO hti;
                        hti.pt.x = (short) LOWORD(GetMessagePos());
                        hti.pt.y = (short) HIWORD(GetMessagePos());
                        ScreenToClient(((LPNMHDR) lParam)->hwndFrom, &hti.pt);
                        if (TreeView_HitTest(((LPNMHDR) lParam)->hwndFrom, &hti))
                            if (hti.flags & TVHT_ONITEMSTATEICON) {
                                TVITEM tvi;
                                tvi.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                                tvi.hItem = hti.hItem;
                                TreeView_GetItem(((LPNMHDR) lParam)->hwndFrom, &tvi);
                                tvi.iImage = tvi.iSelectedImage = tvi.iImage == 1 ? 2 : 1;
                                TreeView_SetItem(((LPNMHDR) lParam)->hwndFrom, &tvi);
                                opt_clc_main_changed = 1;
                                SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
                            }
                    }
                    break;
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
                        case PSN_APPLY:
                            {
                                int i;
                                DWORD exStyle = 0;

                                if(!opt_clc_main_changed)
                                    return TRUE;

                                for (i = 0; i < sizeof(checkBoxToStyleEx) / sizeof(checkBoxToStyleEx[0]); i++) {
                                    if ((IsDlgButtonChecked(hwndDlg, checkBoxToStyleEx[i].id) == 0) == checkBoxToStyleEx[i].not)
                                        exStyle |= checkBoxToStyleEx[i]. flag;
                                }
                                DBWriteContactSettingDword(NULL, "CLC", "ExStyle", exStyle);
                            } {
                                DWORD fullGreyoutFlags = MakeCheckBoxTreeFlags(GetDlgItem(hwndDlg, IDC_GREYOUTOPTS));
                                DBWriteContactSettingDword(NULL, "CLC", "FullGreyoutFlags", fullGreyoutFlags);
                                if (IsDlgButtonChecked(hwndDlg, IDC_GREYOUT))
                                    DBWriteContactSettingDword(NULL, "CLC", "GreyoutFlags", fullGreyoutFlags);
                                else
                                    DBWriteContactSettingDword(NULL, "CLC", "GreyoutFlags", 0);
                            }
							g_CluiData.bGroupAlign = (BYTE)SendDlgItemMessage(hwndDlg, IDC_GROUPALIGN, CB_GETCURSEL, 0, 0);
							DBWriteContactSettingByte(NULL, "CLC", "GroupAlign", g_CluiData.bGroupAlign);

                            DBWriteContactSettingByte(NULL, "CLC", "ShowIdle", (BYTE) (IsDlgButtonChecked(hwndDlg, IDC_IDLE) ? 1 : 0));
                            DBWriteContactSettingDword(NULL, "CLC", "OfflineModes", MakeCheckBoxTreeFlags(GetDlgItem(hwndDlg, IDC_HIDEOFFLINEOPTS)));
                            DBWriteContactSettingByte(NULL, "CLC", "LeftMargin", (BYTE) SendDlgItemMessage(hwndDlg, IDC_LEFTMARGINSPIN, UDM_GETPOS, 0, 0));
                            DBWriteContactSettingByte(NULL, "CLC", "RightMargin", (BYTE) SendDlgItemMessage(hwndDlg, IDC_RIGHTMARGINSPIN, UDM_GETPOS, 0, 0));
                            DBWriteContactSettingByte(NULL, "CLC", "RowGap", (BYTE) SendDlgItemMessage(hwndDlg, IDC_ROWGAPSPIN, UDM_GETPOS, 0, 0));
                            DBWriteContactSettingWord(NULL, "CLC", "ScrollTime", (WORD) SendDlgItemMessage(hwndDlg, IDC_SMOOTHTIMESPIN, UDM_GETPOS, 0, 0));
                            DBWriteContactSettingByte(NULL, "CLC", "GroupIndent", (BYTE) SendDlgItemMessage(hwndDlg, IDC_GROUPINDENTSPIN, UDM_GETPOS, 0, 0));
                            DBWriteContactSettingByte(NULL, "CLC", "NoVScrollBar", (BYTE) (IsDlgButtonChecked(hwndDlg, IDC_NOSCROLLBAR) ? 1 : 0));
                            g_CluiData.dwFlags = IsDlgButtonChecked(hwndDlg, IDC_FULLROWSELECT) ? g_CluiData.dwFlags | CLUI_FULLROWSELECT : g_CluiData.dwFlags & ~CLUI_FULLROWSELECT;
                            g_CluiData.dwFlags = IsDlgButtonChecked(hwndDlg, IDC_NOGROUPICON) ? g_CluiData.dwFlags | CLUI_FRAME_NOGROUPICON : g_CluiData.dwFlags & ~CLUI_FRAME_NOGROUPICON;
                            g_CluiData.bRowSpacing = (BYTE) SendDlgItemMessage(hwndDlg, IDC_ROWGAPSPIN, UDM_GETPOS, 0, 0);
                            g_CluiData.bDblClkAvatars = IsDlgButtonChecked(hwndDlg, IDC_DBLCLKAVATARS) ? TRUE : FALSE;
                            DBWriteContactSettingByte(NULL, "CLC", "RowHeight", (BYTE) SendDlgItemMessage(hwndDlg, IDC_ROWHEIGHTSPIN, UDM_GETPOS, 0, 0));
                            DBWriteContactSettingByte(NULL, "CLC", "GRowHeight", (BYTE) SendDlgItemMessage(hwndDlg, IDC_GROUPROWHEIGHTSPIN, UDM_GETPOS, 0, 0));
                            DBWriteContactSettingByte(NULL, "CLC", "dblclkav", (BYTE)g_CluiData.bDblClkAvatars);
                            DBWriteContactSettingDword(NULL, "CLUI", "Frameflags", g_CluiData.dwFlags);
                            DBWriteContactSettingByte(NULL, "CLCExt", "EXBK_CenterGroupnames", (BYTE)IsDlgButtonChecked(hwndDlg, IDC_CENTERGROUPNAMES));
                            pcli->pfnClcOptionsChanged();
                            CoolSB_SetupScrollBar();
                            PostMessage(pcli->hwndContactList, CLUIINTM_REDRAW, 0, 0);
                            opt_clc_main_changed = 0;
                            return TRUE;
                    }
                    break;
            }
            break;
        case WM_DESTROY:
            ImageList_Destroy(TreeView_GetImageList(GetDlgItem(hwndDlg, IDC_GREYOUTOPTS), TVSIL_NORMAL));
            break;
    }
    return FALSE;
}

static int opt_clc_bkg_changed = 0;

static BOOL CALLBACK DlgProcClcBkgOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
            opt_clc_bkg_changed = 0;
            TranslateDialogDefault(hwndDlg);
            CheckDlgButton(hwndDlg, IDC_BITMAP, DBGetContactSettingByte(NULL, "CLC", "UseBitmap", CLCDEFAULT_USEBITMAP) ? BST_CHECKED : BST_UNCHECKED);
            SendMessage(hwndDlg, WM_USER + 10, 0, 0);           
            SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETDEFAULTCOLOUR, 0, CLCDEFAULT_BKCOLOUR);
            SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, "CLC", "BkColour", CLCDEFAULT_BKCOLOUR));   
            CheckDlgButton(hwndDlg, IDC_WINCOLOUR, DBGetContactSettingByte(NULL, "CLC", "UseWinColours", 0));
            CheckDlgButton(hwndDlg, IDC_SKINMODE, g_CluiData.bWallpaperMode);
            SendMessage(hwndDlg, WM_USER + 11, 0, 0); {
                DBVARIANT dbv;
                
                if (!DBGetContactSetting(NULL, "CLC", "BkBitmap", &dbv)) {
                    if (ServiceExists(MS_UTILS_PATHTOABSOLUTE)) {
                        char szPath[MAX_PATH];

                        if (CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM) dbv.pszVal, (LPARAM) szPath))
                            SetDlgItemTextA(hwndDlg, IDC_FILENAME, szPath);
                    }
                    DBFreeVariant(&dbv);
                }
            } {
                WORD bmpUse = DBGetContactSettingWord(NULL, "CLC", "BkBmpUse", CLCDEFAULT_BKBMPUSE);
                CheckDlgButton(hwndDlg, IDC_STRETCHH, bmpUse & CLB_STRETCHH ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_STRETCHV, bmpUse & CLB_STRETCHV ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_TILEH, bmpUse & CLBF_TILEH ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_TILEV, bmpUse & CLBF_TILEV ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_SCROLL, bmpUse & CLBF_SCROLL ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_PROPORTIONAL, bmpUse & CLBF_PROPORTIONAL ? BST_CHECKED : BST_UNCHECKED);
            } {
                HRESULT (STDAPICALLTYPE *MySHAutoComplete)(HWND, DWORD);
                MySHAutoComplete = (HRESULT(STDAPICALLTYPE *)(HWND, DWORD))GetProcAddress(GetModuleHandleA("shlwapi"), "SHAutoComplete");
                if (MySHAutoComplete)
                    MySHAutoComplete(GetDlgItem(hwndDlg, IDC_FILENAME), 1);
            }
            return TRUE;
        case WM_USER+10:
            EnableWindow(GetDlgItem(hwndDlg, IDC_FILENAME), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            EnableWindow(GetDlgItem(hwndDlg, IDC_STRETCHH), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            EnableWindow(GetDlgItem(hwndDlg, IDC_STRETCHV), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            EnableWindow(GetDlgItem(hwndDlg, IDC_TILEH), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            EnableWindow(GetDlgItem(hwndDlg, IDC_TILEV), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            EnableWindow(GetDlgItem(hwndDlg, IDC_SCROLL), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            EnableWindow(GetDlgItem(hwndDlg, IDC_PROPORTIONAL), IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
            break;
        case WM_USER+11:
            {
                BOOL b = IsDlgButtonChecked(hwndDlg, IDC_WINCOLOUR);
                EnableWindow(GetDlgItem(hwndDlg, IDC_BKGCOLOUR), !b);           
                break;
            }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BROWSE) {
                char str[MAX_PATH];
                OPENFILENAMEA ofn = {
                    0
                };
                char filter[512];

                GetDlgItemTextA(hwndDlg, IDC_FILENAME, str, sizeof(str));
                ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
                ofn.hwndOwner = hwndDlg;
                ofn.hInstance = NULL;
                CallService(MS_UTILS_GETBITMAPFILTERSTRINGS, sizeof(filter), (LPARAM) filter);
                ofn.lpstrFilter = filter;
                ofn.lpstrFile = str;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
                ofn.nMaxFile = sizeof(str);
                ofn.nMaxFileTitle = MAX_PATH;
                ofn.lpstrDefExt = "bmp";
                if (!GetOpenFileNameA(&ofn))
                    break;
                SetDlgItemTextA(hwndDlg, IDC_FILENAME, str);
            } else if (LOWORD(wParam) == IDC_FILENAME && HIWORD(wParam) != EN_CHANGE)
                break;
            if (LOWORD(wParam) == IDC_BITMAP)
                SendMessage(hwndDlg, WM_USER + 10, 0, 0);
            if (LOWORD(wParam) == IDC_WINCOLOUR)
                SendMessage(hwndDlg, WM_USER + 11, 0, 0);
            if (LOWORD(wParam) == IDC_FILENAME && (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
                return 0;
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            opt_clc_bkg_changed = 1;
            break;
        case WM_NOTIFY:
            switch (((LPNMHDR) lParam)->idFrom) {
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
                        case PSN_APPLY:
                                if(!opt_clc_bkg_changed)
                                    return TRUE;

                                DBWriteContactSettingByte(NULL, "CLC", "UseBitmap", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_BITMAP)); {
                                COLORREF col;
                                col = SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR, 0, 0);
                                if (col == CLCDEFAULT_BKCOLOUR)
                                    DBDeleteContactSetting(NULL, "CLC", "BkColour");
                                else
                                    DBWriteContactSettingDword(NULL, "CLC", "BkColour", col);
                                DBWriteContactSettingByte(NULL, "CLC", "UseWinColours", (BYTE)IsDlgButtonChecked(hwndDlg, IDC_WINCOLOUR));
                            } {
                                char str[MAX_PATH], strrel[MAX_PATH];
                                
                                GetDlgItemTextA(hwndDlg, IDC_FILENAME, str, sizeof(str));
                                if (CallService(MS_UTILS_PATHTORELATIVE, (WPARAM) str, (LPARAM) strrel))
                                    DBWriteContactSettingString(NULL, "CLC", "BkBitmap", strrel);
                                else
                                    DBWriteContactSettingString(NULL, "CLC", "BkBitmap", str);
                            } {
                                WORD flags = 0;
                                if (IsDlgButtonChecked(hwndDlg, IDC_STRETCHH))
                                    flags |= CLB_STRETCHH;
                                if (IsDlgButtonChecked(hwndDlg, IDC_STRETCHV))
                                    flags |= CLB_STRETCHV;
                                if (IsDlgButtonChecked(hwndDlg, IDC_TILEH))
                                    flags |= CLBF_TILEH;
                                if (IsDlgButtonChecked(hwndDlg, IDC_TILEV))
                                    flags |= CLBF_TILEV;
                                if (IsDlgButtonChecked(hwndDlg, IDC_SCROLL))
                                    flags |= CLBF_SCROLL;
                                if (IsDlgButtonChecked(hwndDlg, IDC_PROPORTIONAL))
                                    flags |= CLBF_PROPORTIONAL;
                                DBWriteContactSettingWord(NULL, "CLC", "BkBmpUse", flags);
                                g_CluiData.bWallpaperMode = IsDlgButtonChecked(hwndDlg, IDC_SKINMODE) ? 1 : 0;
                                DBWriteContactSettingByte(NULL, "CLUI", "UseBkSkin", (BYTE)g_CluiData.bWallpaperMode);
                            }
                            pcli->pfnClcOptionsChanged();
                            PostMessage(pcli->hwndContactList, CLUIINTM_REDRAW, 0, 0);
                            opt_clc_bkg_changed = 0;
                            return TRUE;
                    }
                    break;
            }
            break;
    }
    return FALSE;
}

static const TCHAR *szFontIdDescr[FONTID_LAST+ 1] = {
    _T("Standard contacts"), 
    _T("Online contacts to whom you have a different visibility"), 
    _T("Offline contacts"), 
    _T("Contacts which are 'not on list'"),
    _T("Groups"), 
    _T("Group member counts"), 
    _T("Dividers"), 
    _T("Offline contacts to whom you have a different visibility"), 
    _T("Status mode"), 
    _T("Frame titles"), 
    _T("Event area"),
    _T("Contacts local time")
};

#define SAMEASF_FACE   1
#define SAMEASF_SIZE   2
#define SAMEASF_STYLE  4
#define SAMEASF_COLOUR 8
#include <pshpack1.h>
struct {
    BYTE sameAsFlags, sameAs;
    COLORREF colour;
    char size;
    BYTE style;
    BYTE charset;
    TCHAR szFace[LF_FACESIZE];
} static fontSettings[FONTID_LAST+1];
#include <poppack.h>
static WORD fontSameAsDefault[FONTID_LAST+1] = {
    0x00FF, 0x0B00, 0x0F00, 0x0700, 0x0B00, 0x0104, 0x0D00, 0x0000, 0x0000
};
static char *fontSizes[] = {
    "8", "10", "14", "16", "18", "20", "24", "8", "8", "8", "8", "8", "8"
};
static int fontListOrder[FONTID_LAST+1] = {
    FONTID_CONTACTS, FONTID_INVIS, FONTID_OFFLINE, FONTID_OFFINVIS, FONTID_NOTONLIST, FONTID_GROUPS, FONTID_GROUPCOUNTS, 
    FONTID_DIVIDERS, FONTID_STATUS, FONTID_FRAMETITLE, FONTID_EVENTAREA, FONTID_TIMESTAMP
};

#define M_REBUILDFONTGROUP   (WM_USER+10)
#define M_REMAKESAMPLE       (WM_USER+11)
#define M_RECALCONEFONT      (WM_USER+12)
#define M_RECALCOTHERFONTS   (WM_USER+13)
#define M_SAVEFONT           (WM_USER+14)
#define M_REFRESHSAMEASBOXES (WM_USER+15)
#define M_FILLSCRIPTCOMBO    (WM_USER+16)
#define M_REDOROWHEIGHT      (WM_USER+17)
#define M_LOADFONT           (WM_USER+18)
#define M_GUESSSAMEASBOXES   (WM_USER+19)
#define M_SETSAMEASBOXES     (WM_USER+20)

/*
 * The following code has been disabled, because Miranda 0.7 has the font services
 * integrated. "Per plugin" font settings are no longer needed.
 */

/*
static int CALLBACK EnumFontsProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, int FontType, LPARAM lParam)
{
    if (!IsWindow((HWND) lParam))
        return FALSE;
    if (SendMessage((HWND) lParam, CB_FINDSTRINGEXACT, -1, (LPARAM) lpelfe->elfLogFont.lfFaceName) == CB_ERR)
        SendMessage((HWND) lParam, CB_ADDSTRING, 0, (LPARAM) lpelfe->elfLogFont.lfFaceName);
    return TRUE;
}

void FillFontListThread(HWND hwndDlg)
{
    LOGFONT lf = {
        0
    };
    HDC hdc = GetDC(hwndDlg);
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfFaceName[0] = 0;
    lf.lfPitchAndFamily = 0;
    EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC) EnumFontsProc, (LPARAM) GetDlgItem(hwndDlg, IDC_TYPEFACE), 0);
    ReleaseDC(hwndDlg, hdc);
    return;
}

static int CALLBACK EnumFontScriptsProc(ENUMLOGFONTEXA *lpelfe, NEWTEXTMETRICEXA *lpntme, int FontType, LPARAM lParam)
{
    if (SendMessageA((HWND) lParam, CB_FINDSTRINGEXACT, -1, (LPARAM) lpelfe->elfScript) == CB_ERR) {
        int i = SendMessageA((HWND) lParam, CB_ADDSTRING, 0, (LPARAM) lpelfe->elfScript);
        SendMessage((HWND) lParam, CB_SETITEMDATA, i, lpelfe->elfLogFont.lfCharSet);
    }
    return TRUE;
}

static int TextOptsDlgResizer(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL *urc)
{
    return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;
}
*/

/*
static BOOL CALLBACK DlgProcClcTextOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HFONT hFontSample;

    switch (msg) {
        case WM_INITDIALOG:
            hFontSample = NULL;
            SetDlgItemTextA(hwndDlg, IDC_SAMPLE, "Sample");
            TranslateDialogDefault(hwndDlg);
            forkthread(FillFontListThread, 0, hwndDlg); {
                int i, itemId, fontId;
                LOGFONT lf;
                COLORREF colour;
                WORD sameAs;
                char str[32];

                for (i = 0; i <= FONTID_LAST; i++) {
                    fontId = fontListOrder[i];
                    pcli->pfnGetFontSetting(fontId, &lf, &colour);
                    wsprintfA(str, "Font%dAs", fontId);
                    sameAs = DBGetContactSettingWord(NULL, "CLC", str, fontSameAsDefault[fontId]);
                    fontSettings[fontId]. sameAsFlags = HIBYTE(sameAs);
                    fontSettings[fontId]. sameAs = LOBYTE(sameAs);
                    fontSettings[fontId]. style = (lf.lfWeight == FW_NORMAL ? 0 : DBFONTF_BOLD) | (lf.lfItalic ? DBFONTF_ITALIC : 0) | (lf.lfUnderline ? DBFONTF_UNDERLINE : 0);
                    if (lf.lfHeight < 0) {
                        HDC hdc;
                        SIZE size;
                        HFONT hFont = CreateFontIndirect(&lf);
                        hdc = GetDC(hwndDlg);
                        SelectObject(hdc, hFont);
                        GetTextExtentPoint32A(hdc, "_W", 2, &size);
                        ReleaseDC(hwndDlg, hdc);
                        DeleteObject(hFont);
                        fontSettings[fontId]. size = (char) size.cy;
                    } else
                        fontSettings[fontId]. size = (char) lf.lfHeight;
                    fontSettings[fontId]. charset = lf.lfCharSet;
                    fontSettings[fontId]. colour = colour;
                    lstrcpy(fontSettings[fontId].szFace, lf.lfFaceName);
                    itemId = SendDlgItemMessage(hwndDlg, IDC_FONTID, CB_ADDSTRING, 0, (LPARAM) TranslateTS(szFontIdDescr[fontId]));
                    SendDlgItemMessage(hwndDlg, IDC_FONTID, CB_SETITEMDATA, itemId, fontId);
                }
                SendDlgItemMessage(hwndDlg, IDC_FONTID, CB_SETCURSEL, 0, 0);
                for (i = 0; i < sizeof(fontSizes) / sizeof(fontSizes[0]); i++) {
                    SendDlgItemMessageA(hwndDlg, IDC_FONTSIZE, CB_ADDSTRING, 0, (LPARAM) fontSizes[i]);
                }
            }
            SendMessage(hwndDlg, M_REBUILDFONTGROUP, 0, 0);
            SendMessage(hwndDlg, M_SAVEFONT, 0, 0);
            SendDlgItemMessage(hwndDlg, IDC_HOTCOLOUR, CPM_SETDEFAULTCOLOUR, 0, CLCDEFAULT_HOTTEXTCOLOUR);
            SendDlgItemMessage(hwndDlg, IDC_HOTCOLOUR, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, "CLC", "HotTextColour", CLCDEFAULT_HOTTEXTCOLOUR));
            CheckDlgButton(hwndDlg, IDC_GAMMACORRECT, DBGetContactSettingByte(NULL, "CLC", "GammaCorrect", CLCDEFAULT_GAMMACORRECT) ? BST_CHECKED : BST_UNCHECKED);

            SendDlgItemMessage(hwndDlg, IDC_SELCOLOUR, CPM_SETDEFAULTCOLOUR, 0, CLCDEFAULT_SELTEXTCOLOUR);
            SendDlgItemMessage(hwndDlg, IDC_SELCOLOUR, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, "CLC", "SelTextColour", CLCDEFAULT_SELTEXTCOLOUR));
            SendDlgItemMessage(hwndDlg, IDC_QUICKCOLOUR, CPM_SETDEFAULTCOLOUR, 0, CLCDEFAULT_QUICKSEARCHCOLOUR);
            SendDlgItemMessage(hwndDlg, IDC_QUICKCOLOUR, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, "CLC", "QuickSearchColour", CLCDEFAULT_QUICKSEARCHCOLOUR));
            return TRUE;
        case M_REBUILDFONTGROUP:
    //remake all the needed controls when the user changes the font selector at the top
            {
                int i = SendDlgItemMessage(hwndDlg, IDC_FONTID, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_FONTID, CB_GETCURSEL, 0, 0), 0);
                SendMessage(hwndDlg, M_SETSAMEASBOXES, i, 0); {
                    int j, id, itemId;
                    char szText[256];
                    SendDlgItemMessage(hwndDlg, IDC_SAMEAS, CB_RESETCONTENT, 0, 0);
                    itemId = SendDlgItemMessage(hwndDlg, IDC_SAMEAS, CB_ADDSTRING, 0, (LPARAM) TranslateT("<none>"));
                    SendDlgItemMessage(hwndDlg, IDC_SAMEAS, CB_SETITEMDATA, itemId, 0xFF);
                    if (0xFF == fontSettings[i].sameAs)
                        SendDlgItemMessage(hwndDlg, IDC_SAMEAS, CB_SETCURSEL, itemId, 0);
                    for (j = 0; j <= FONTID_LAST; j++) {
                        SendDlgItemMessageA(hwndDlg, IDC_FONTID, CB_GETLBTEXT, j, (LPARAM) szText);
                        id = SendDlgItemMessage(hwndDlg, IDC_FONTID, CB_GETITEMDATA, j, 0);
                        if (id == i)
                            continue;
                        itemId = SendDlgItemMessageA(hwndDlg, IDC_SAMEAS, CB_ADDSTRING, 0, (LPARAM) szText);
                        SendDlgItemMessage(hwndDlg, IDC_SAMEAS, CB_SETITEMDATA, itemId, id);
                        if (id == fontSettings[i].sameAs)
                            SendDlgItemMessage(hwndDlg, IDC_SAMEAS, CB_SETCURSEL, itemId, 0);
                    }
                }
                SendMessage(hwndDlg, M_LOADFONT, i, 0);
                SendMessage(hwndDlg, M_REFRESHSAMEASBOXES, i, 0);
                SendMessage(hwndDlg, M_REMAKESAMPLE, 0, 0);
                break;
            }
        case M_SETSAMEASBOXES:
    //set the check mark in the 'same as' boxes to the right value for fontid wParam
            CheckDlgButton(hwndDlg, IDC_SAMETYPE, fontSettings[wParam].sameAsFlags & SAMEASF_FACE ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_SAMESIZE, fontSettings[wParam].sameAsFlags & SAMEASF_SIZE ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_SAMESTYLE, fontSettings[wParam].sameAsFlags & SAMEASF_STYLE ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_SAMECOLOUR, fontSettings[wParam].sameAsFlags & SAMEASF_COLOUR ? BST_CHECKED : BST_UNCHECKED);
            break;
        case M_FILLSCRIPTCOMBO:
    //fill the script combo box and set the selection to the value for fontid wParam
            {
                LOGFONTA lf = {
                    0
                };
                int i;
                HDC hdc = GetDC(hwndDlg);
                lf.lfCharSet = DEFAULT_CHARSET;
                GetDlgItemTextA(hwndDlg, IDC_TYPEFACE, lf.lfFaceName, sizeof(lf.lfFaceName));
                lf.lfPitchAndFamily = 0;
                SendDlgItemMessageA(hwndDlg, IDC_SCRIPT, CB_RESETCONTENT, 0, 0);
                EnumFontFamiliesExA(hdc, &lf, (FONTENUMPROCA) EnumFontScriptsProc, (LPARAM) GetDlgItem(hwndDlg, IDC_SCRIPT), 0);
                ReleaseDC(hwndDlg, hdc);
                for (i = SendDlgItemMessage(hwndDlg, IDC_SCRIPT, CB_GETCOUNT, 0, 0) - 1; i >= 0; i--) {
                    if (SendDlgItemMessage(hwndDlg, IDC_SCRIPT, CB_GETITEMDATA, i, 0) == fontSettings[wParam].charset) {
                        SendDlgItemMessage(hwndDlg, IDC_SCRIPT, CB_SETCURSEL, i, 0);
                        break;
                    }
                }
                if (i < 0)
                    SendDlgItemMessage(hwndDlg, IDC_SCRIPT, CB_SETCURSEL, 0, 0);
                break;
            }
        case WM_CTLCOLORSTATIC:
            if ((HWND) lParam == GetDlgItem(hwndDlg, IDC_SAMPLE)) {
                SetTextColor((HDC) wParam, SendDlgItemMessage(hwndDlg, IDC_COLOUR, CPM_GETCOLOUR, 0, 0));
                SetBkColor((HDC) wParam, GetSysColor(COLOR_3DFACE));
                return(BOOL) GetSysColorBrush(COLOR_3DFACE);
            }
            break;
        case M_REFRESHSAMEASBOXES:
    //set the disabled flag on the 'same as' checkboxes to the values for fontid wParam
            EnableWindow(GetDlgItem(hwndDlg, IDC_SAMETYPE), fontSettings[wParam].sameAs != 0xFF);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SAMESIZE), fontSettings[wParam].sameAs != 0xFF);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SAMESTYLE), fontSettings[wParam].sameAs != 0xFF);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SAMECOLOUR), fontSettings[wParam].sameAs != 0xFF);
            if (SendMessage(GetParent(hwndDlg), PSM_ISEXPERT, 0, 0)) {
                EnableWindow(GetDlgItem(hwndDlg, IDC_TYPEFACE), fontSettings[wParam].sameAs == 0xFF || !(fontSettings[wParam].sameAsFlags & SAMEASF_FACE));
                EnableWindow(GetDlgItem(hwndDlg, IDC_SCRIPT), fontSettings[wParam].sameAs == 0xFF || !(fontSettings[wParam].sameAsFlags & SAMEASF_FACE));
                EnableWindow(GetDlgItem(hwndDlg, IDC_FONTSIZE), fontSettings[wParam].sameAs == 0xFF || !(fontSettings[wParam].sameAsFlags & SAMEASF_SIZE));
                EnableWindow(GetDlgItem(hwndDlg, IDC_BOLD), fontSettings[wParam].sameAs == 0xFF || !(fontSettings[wParam].sameAsFlags & SAMEASF_STYLE));
                EnableWindow(GetDlgItem(hwndDlg, IDC_ITALIC), fontSettings[wParam].sameAs == 0xFF || !(fontSettings[wParam].sameAsFlags & SAMEASF_STYLE));
                EnableWindow(GetDlgItem(hwndDlg, IDC_UNDERLINE), fontSettings[wParam].sameAs == 0xFF || !(fontSettings[wParam].sameAsFlags & SAMEASF_STYLE));
                EnableWindow(GetDlgItem(hwndDlg, IDC_COLOUR), fontSettings[wParam].sameAs == 0xFF || !(fontSettings[wParam].sameAsFlags & SAMEASF_COLOUR));
            } else {
                EnableWindow(GetDlgItem(hwndDlg, IDC_TYPEFACE), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_SCRIPT), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_FONTSIZE), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_BOLD), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ITALIC), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_UNDERLINE), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_COLOUR), TRUE);
            }
            break;
        case M_REMAKESAMPLE:
    //remake the sample edit box font based on the settings in the controls
            {
                LOGFONTA lf;
                if (hFontSample) {
                    SendDlgItemMessage(hwndDlg, IDC_SAMPLE, WM_SETFONT, SendDlgItemMessage(hwndDlg, IDC_FONTID, WM_GETFONT, 0, 0), 0);
                    DeleteObject(hFontSample);
                }
                lf.lfHeight = GetDlgItemInt(hwndDlg, IDC_FONTSIZE, NULL, FALSE); {
                    HDC hdc = GetDC(NULL);              
                    lf.lfHeight = -MulDiv(lf.lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
                    ReleaseDC(NULL, hdc);
                }
                lf.lfWidth = lf.lfEscapement = lf.lfOrientation = 0;
                lf.lfWeight = IsDlgButtonChecked(hwndDlg, IDC_BOLD) ? FW_BOLD : FW_NORMAL;
                lf.lfItalic = IsDlgButtonChecked(hwndDlg, IDC_ITALIC);
                lf.lfUnderline = IsDlgButtonChecked(hwndDlg, IDC_UNDERLINE);
                lf.lfStrikeOut = 0;
                lf.lfCharSet = (BYTE) SendDlgItemMessage(hwndDlg, IDC_SCRIPT, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_SCRIPT, CB_GETCURSEL, 0, 0), 0);
                lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
                lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
                lf.lfQuality = DEFAULT_QUALITY;
                lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
                GetDlgItemTextA(hwndDlg, IDC_TYPEFACE, lf.lfFaceName, sizeof(lf.lfFaceName));
                hFontSample = CreateFontIndirectA(&lf);
                SendDlgItemMessage(hwndDlg, IDC_SAMPLE, WM_SETFONT, (WPARAM) hFontSample, TRUE);
                break;
            }
        case M_RECALCONEFONT:
    //copy the 'same as' settings for fontid wParam from their sources
            if (fontSettings[wParam].sameAs == 0xFF)
                break;
            if (fontSettings[wParam].sameAsFlags & SAMEASF_FACE) {
                lstrcpy(fontSettings[wParam].szFace, fontSettings[fontSettings[wParam].sameAs].szFace);
                fontSettings[wParam]. charset = fontSettings[fontSettings[wParam].sameAs].charset;
            }
            if (fontSettings[wParam].sameAsFlags & SAMEASF_SIZE)
                fontSettings[wParam]. size = fontSettings[fontSettings[wParam].sameAs].size;
            if (fontSettings[wParam].sameAsFlags & SAMEASF_STYLE)
                fontSettings[wParam]. style = fontSettings[fontSettings[wParam].sameAs].style;
            if (fontSettings[wParam].sameAsFlags & SAMEASF_COLOUR)
                fontSettings[wParam]. colour = fontSettings[fontSettings[wParam].sameAs].colour;
            break;
        case M_RECALCOTHERFONTS:
    //recalculate the 'same as' settings for all fonts but wParam
            {
                int i;
                for (i = 0; i <= FONTID_LAST; i++) {
                    if (i == (int) wParam)
                        continue;
                    SendMessage(hwndDlg, M_RECALCONEFONT, i, 0);
                }
                break;
            }
        case M_SAVEFONT:
    //save the font settings from the controls to font wParam
            fontSettings[wParam]. sameAsFlags = (IsDlgButtonChecked(hwndDlg, IDC_SAMETYPE) ? SAMEASF_FACE : 0) | (IsDlgButtonChecked(hwndDlg, IDC_SAMESIZE) ? SAMEASF_SIZE : 0) | (IsDlgButtonChecked(hwndDlg, IDC_SAMESTYLE) ? SAMEASF_STYLE : 0) | (IsDlgButtonChecked(hwndDlg, IDC_SAMECOLOUR) ? SAMEASF_COLOUR : 0);
            fontSettings[wParam]. sameAs = (BYTE) SendDlgItemMessage(hwndDlg, IDC_SAMEAS, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_SAMEAS, CB_GETCURSEL, 0, 0), 0);
            GetDlgItemText(hwndDlg, IDC_TYPEFACE, fontSettings[wParam].szFace, sizeof(fontSettings[wParam].szFace));
            fontSettings[wParam]. charset = (BYTE) SendDlgItemMessage(hwndDlg, IDC_SCRIPT, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_SCRIPT, CB_GETCURSEL, 0, 0), 0);
            fontSettings[wParam]. size = (char) GetDlgItemInt(hwndDlg, IDC_FONTSIZE, NULL, FALSE);
            fontSettings[wParam]. style = (IsDlgButtonChecked(hwndDlg, IDC_BOLD) ? DBFONTF_BOLD : 0) | (IsDlgButtonChecked(hwndDlg, IDC_ITALIC) ? DBFONTF_ITALIC : 0) | (IsDlgButtonChecked(hwndDlg, IDC_UNDERLINE) ? DBFONTF_UNDERLINE : 0);
            fontSettings[wParam]. colour = SendDlgItemMessage(hwndDlg, IDC_COLOUR, CPM_GETCOLOUR, 0, 0);
            SendMessage(hwndDlg, M_REDOROWHEIGHT, 0, 0);
            break;
        case M_REDOROWHEIGHT:
    //recalculate the minimum feasible row height
            {
                int i;
                int minHeight = 10;
                for (i = 0; i <= FONTID_LAST; i++) {
                    if (fontSettings[i].size > minHeight)
                        minHeight = fontSettings[i].size;
                }
                break;
            }
        case M_LOADFONT:
    //load font wParam into the controls
            SetDlgItemText(hwndDlg, IDC_TYPEFACE, fontSettings[wParam].szFace);
            SendMessage(hwndDlg, M_FILLSCRIPTCOMBO, wParam, 0);
            SetDlgItemInt(hwndDlg, IDC_FONTSIZE, fontSettings[wParam].size, FALSE);
            CheckDlgButton(hwndDlg, IDC_BOLD, fontSettings[wParam].style & DBFONTF_BOLD ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_ITALIC, fontSettings[wParam].style & DBFONTF_ITALIC ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_UNDERLINE, fontSettings[wParam].style & DBFONTF_UNDERLINE ? BST_CHECKED : BST_UNCHECKED); {
                LOGFONT lf;
                COLORREF colour;
                GetDefaultFontSetting(wParam, &lf, &colour);
                SendDlgItemMessage(hwndDlg, IDC_COLOUR, CPM_SETDEFAULTCOLOUR, 0, colour);
            }
            SendDlgItemMessage(hwndDlg, IDC_COLOUR, CPM_SETCOLOUR, 0, fontSettings[wParam].colour);
            break;
        case M_GUESSSAMEASBOXES:
    //guess suitable values for the 'same as' checkboxes for fontId wParam
            fontSettings[wParam]. sameAsFlags = 0;
            if (fontSettings[wParam].sameAs == 0xFF)
                break;
            if (!lstrcmp(fontSettings[wParam].szFace, fontSettings[fontSettings[wParam].sameAs].szFace) && fontSettings[wParam].charset == fontSettings[fontSettings[wParam].sameAs].charset)
                fontSettings[wParam].sameAsFlags |= SAMEASF_FACE;
            if (fontSettings[wParam].size == fontSettings[fontSettings[wParam].sameAs].size)
                fontSettings[wParam].sameAsFlags |= SAMEASF_SIZE;
            if (fontSettings[wParam].style == fontSettings[fontSettings[wParam].sameAs].style)
                fontSettings[wParam].sameAsFlags |= SAMEASF_STYLE;
            if (fontSettings[wParam].colour == fontSettings[fontSettings[wParam].sameAs].colour)
                fontSettings[wParam].sameAsFlags |= SAMEASF_COLOUR;
            SendMessage(hwndDlg, M_SETSAMEASBOXES, wParam, 0);
            break;
        case WM_VSCROLL:
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            break;
        case WM_COMMAND:
            {
                int fontId = SendDlgItemMessage(hwndDlg, IDC_FONTID, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_FONTID, CB_GETCURSEL, 0, 0), 0);
                switch (LOWORD(wParam)) {
                    case IDC_FONTID:
                        if (HIWORD(wParam) != CBN_SELCHANGE)
                            return FALSE;
                        SendMessage(hwndDlg, M_REBUILDFONTGROUP, 0, 0);
                        return 0;
                    case IDC_SAMETYPE:
                    case IDC_SAMESIZE:
                    case IDC_SAMESTYLE:
                    case IDC_SAMECOLOUR:
                        SendMessage(hwndDlg, M_SAVEFONT, fontId, 0);
                        SendMessage(hwndDlg, M_RECALCONEFONT, fontId, 0);
                        SendMessage(hwndDlg, M_REMAKESAMPLE, 0, 0);
                        SendMessage(hwndDlg, M_REFRESHSAMEASBOXES, fontId, 0);
                        break;
                    case IDC_SAMEAS:
                        if (HIWORD(wParam) != CBN_SELCHANGE)
                            return FALSE;
                        if (SendDlgItemMessage(hwndDlg, IDC_SAMEAS, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_SAMEAS, CB_GETCURSEL, 0, 0), 0) == fontId)
                            SendDlgItemMessage(hwndDlg, IDC_SAMEAS, CB_SETCURSEL, 0, 0);
                        if (!SendMessage(GetParent(hwndDlg), PSM_ISEXPERT, 0, 0)) {
                            int sameAs = SendDlgItemMessage(hwndDlg, IDC_SAMEAS, CB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_SAMEAS, CB_GETCURSEL, 0, 0), 0);
                            if (sameAs != 0xFF)
                                SendMessage(hwndDlg, M_LOADFONT, sameAs, 0);
                            SendMessage(hwndDlg, M_SAVEFONT, fontId, 0);
                            SendMessage(hwndDlg, M_GUESSSAMEASBOXES, fontId, 0);
                        } else
                            SendMessage(hwndDlg, M_SAVEFONT, fontId, 0);
                        SendMessage(hwndDlg, M_RECALCONEFONT, fontId, 0);
                        SendMessage(hwndDlg, M_FILLSCRIPTCOMBO, fontId, 0);
                        SendMessage(hwndDlg, M_REMAKESAMPLE, 0, 0);
                        SendMessage(hwndDlg, M_REFRESHSAMEASBOXES, fontId, 0);
                        break;
                    case IDC_TYPEFACE:
                    case IDC_SCRIPT:
                    case IDC_FONTSIZE:
                        if (HIWORD(wParam) != CBN_EDITCHANGE && HIWORD(wParam) != CBN_SELCHANGE)
                            return FALSE;
                        if (HIWORD(wParam) == CBN_SELCHANGE) {
                            SendDlgItemMessage(hwndDlg, LOWORD(wParam), CB_SETCURSEL, SendDlgItemMessage(hwndDlg, LOWORD(wParam), CB_GETCURSEL, 0, 0), 0);
                        }
                        if (LOWORD(wParam) == IDC_TYPEFACE)
                            SendMessage(hwndDlg, M_FILLSCRIPTCOMBO, fontId, 0);
        //fall through
                    case IDC_BOLD:
                    case IDC_ITALIC:
                    case IDC_UNDERLINE:
                    case IDC_COLOUR:
                        SendMessage(hwndDlg, M_SAVEFONT, fontId, 0);
                        if (!SendMessage(GetParent(hwndDlg), PSM_ISEXPERT, 0, 0)) {
                            SendMessage(hwndDlg, M_GUESSSAMEASBOXES, fontId, 0);
                            SendMessage(hwndDlg, M_REFRESHSAMEASBOXES, fontId, 0);
                        }
                        SendMessage(hwndDlg, M_RECALCOTHERFONTS, fontId, 0);
                        SendMessage(hwndDlg, M_REMAKESAMPLE, 0, 0);
                        SendMessage(hwndDlg, M_REDOROWHEIGHT, 0, 0);
                        break;
                    case IDC_SAMPLE:
                        return 0;
                }
                SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
                break;
            }
        case WM_NOTIFY:
            switch (((LPNMHDR) lParam)->idFrom) {
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
                        case PSN_APPLY:
                            {
                                int i;
                                char str[20];

                                for (i = 0; i <= FONTID_LAST; i++) {
                                    wsprintfA(str, "Font%dName", i);
                                    DBWriteContactSettingTString(NULL, "CLC", str, fontSettings[i].szFace);
                                    wsprintfA(str, "Font%dSet", i);
                                    DBWriteContactSettingByte(NULL, "CLC", str, fontSettings[i].charset);
                                    wsprintfA(str, "Font%dSize", i);
                                    DBWriteContactSettingByte(NULL, "CLC", str, fontSettings[i].size);
                                    wsprintfA(str, "Font%dSty", i);
                                    DBWriteContactSettingByte(NULL, "CLC", str, fontSettings[i].style);
                                    wsprintfA(str, "Font%dCol", i);
                                    DBWriteContactSettingDword(NULL, "CLC", str, fontSettings[i].colour);
                                    wsprintfA(str, "Font%dAs", i);
                                    DBWriteContactSettingWord(NULL, "CLC", str, (WORD) ((fontSettings[i].sameAsFlags << 8) | fontSettings[i].sameAs));
                                }
                            } {
                                COLORREF col;
                                col = SendDlgItemMessage(hwndDlg, IDC_SELCOLOUR, CPM_GETCOLOUR, 0, 0);
                                if (col == CLCDEFAULT_SELTEXTCOLOUR)
                                    DBDeleteContactSetting(NULL, "CLC", "SelTextColour");
                                else
                                    DBWriteContactSettingDword(NULL, "CLC", "SelTextColour", col);
                                col = SendDlgItemMessage(hwndDlg, IDC_HOTCOLOUR, CPM_GETCOLOUR, 0, 0);
                                if (col == CLCDEFAULT_HOTTEXTCOLOUR)
                                    DBDeleteContactSetting(NULL, "CLC", "HotTextColour");
                                else
                                    DBWriteContactSettingDword(NULL, "CLC", "HotTextColour", col);
                                col = SendDlgItemMessage(hwndDlg, IDC_QUICKCOLOUR, CPM_GETCOLOUR, 0, 0);
                                if (col == CLCDEFAULT_QUICKSEARCHCOLOUR)
                                    DBDeleteContactSetting(NULL, "CLC", "QuickSearchColour");
                                else
                                    DBWriteContactSettingDword(NULL, "CLC", "QuickSearchColour", col);
                            }
                            DBWriteContactSettingByte(NULL, "CLC", "GammaCorrect", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_GAMMACORRECT));
                            pcli->pfnClcOptionsChanged();
                            PostMessage(pcli->hwndContactList, CLUIINTM_REDRAW, 0, 0);
                            return TRUE;
                    }
                    break;
            }
            break;
        case WM_DESTROY:
            if (hFontSample) {
                SendDlgItemMessage(hwndDlg, IDC_SAMPLE, WM_SETFONT, SendDlgItemMessage(hwndDlg, IDC_FONTID, WM_GETFONT, 0, 0), 0);
                DeleteObject(hFontSample);
            }
            break;
    }
    return FALSE;
}
*/