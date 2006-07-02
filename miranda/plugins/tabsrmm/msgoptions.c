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

*/
#include "commonheaders.h"
#pragma hdrstop
#include "uxtheme.h"

#ifdef __MATHMOD_SUPPORT
    #include "m_MathModule.h"
#endif

#define DM_GETSTATUSMASK (WM_USER + 10)

extern		MYGLOBALS myGlobals;
extern		HANDLE hMessageWindowList;
extern		HINSTANCE g_hInst;
extern		struct ContainerWindowData *pFirstContainer;
extern		int g_chat_integration_enabled;
extern      BOOL CALLBACK DlgProcPopupOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern      BOOL CALLBACK DlgProcTabConfig(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern      BOOL CALLBACK DlgProcTemplateEditor(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern      BOOL CALLBACK DlgProcOptions1(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
extern      BOOL (WINAPI *MyEnableThemeDialogTexture)(HANDLE, DWORD);
extern      StatusItems_t StatusItems[];

BOOL CALLBACK DlgProcSetupStatusModes(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK OptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK SkinOptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct FontOptionsList
{
    COLORREF defColour;
    char *szDefFace;
    BYTE defCharset, defStyle;
    char defSize;
    COLORREF colour;
    char szFace[LF_FACESIZE];
    BYTE charset, style;
    char size;
}

static fontOptionsList[] = {
    {RGB(0, 0, 0), "Tahoma", DEFAULT_CHARSET, 0, -10}};
    

static HIMAGELIST g_himlStates = 0;

HIMAGELIST CreateStateImageList()
{
    if(g_himlStates == 0) {
        g_himlStates = ImageList_Create(16, 16, IsWinVerXPPlus() ? ILC_COLOR32 | ILC_MASK : ILC_COLOR8 | ILC_MASK, 4, 0);
        ImageList_AddIcon(g_himlStates, myGlobals.g_IconFolder);
        ImageList_AddIcon(g_himlStates, myGlobals.g_IconFolder);
        ImageList_AddIcon(g_himlStates, myGlobals.g_IconUnchecked);
        ImageList_AddIcon(g_himlStates, myGlobals.g_IconChecked);
    }
    return g_himlStates;
}

void LoadLogfont(int i, LOGFONTA * lf, COLORREF * colour, char *szModule)
{
    char str[20];
    int style;
    DBVARIANT dbv;
    char bSize;

    if (colour) {
        _snprintf(str, sizeof(str), "Font%dCol", i);
        *colour = DBGetContactSettingDword(NULL, szModule, str, GetSysColor(COLOR_BTNTEXT));
    }
    if (lf) {
        mir_snprintf(str, sizeof(str), "Font%dSize", i);
        if(i == H_MSGFONTID_DIVIDERS && !strcmp(szModule, FONTMODULE))
            lf->lfHeight = 5;
        else {
            bSize = (char)DBGetContactSettingByte(NULL, szModule, str, -10);
            if(bSize < 0)
                lf->lfHeight = abs(bSize);
            else
                lf->lfHeight = (LONG)bSize;
        }
        
        lf->lfWidth = 0;
        lf->lfEscapement = 0;
        lf->lfOrientation = 0;
        mir_snprintf(str, sizeof(str), "Font%dSty", i);
        style = DBGetContactSettingByte(NULL, szModule, str, fontOptionsList[0].defStyle);
        lf->lfWeight = style & FONTF_BOLD ? FW_BOLD : FW_NORMAL;
        lf->lfItalic = style & FONTF_ITALIC ? 1 : 0;
        lf->lfUnderline = style & FONTF_UNDERLINE ? 1 : 0;
        lf->lfStrikeOut = 0;
        mir_snprintf(str, sizeof(str), "Font%dSet", i);
        if((i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT) && !strcmp(szModule, FONTMODULE))
            lf->lfCharSet = SYMBOL_CHARSET;
        else
            lf->lfCharSet = DBGetContactSettingByte(NULL, szModule, str, fontOptionsList[0].defCharset);
        lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
        lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
        lf->lfQuality = DEFAULT_QUALITY;
        lf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        mir_snprintf(str, sizeof(str), "Font%d", i);
        if((i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT) && !strcmp(szModule, FONTMODULE)) {
            lstrcpynA(lf->lfFaceName, "Webdings", LF_FACESIZE);
            lf->lfCharSet = SYMBOL_CHARSET;
        }
        else {
			if (DBGetContactSetting(NULL, szModule, str, &dbv)) {
                lstrcpynA(lf->lfFaceName, fontOptionsList[0].szDefFace, LF_FACESIZE);
				lf->lfFaceName[LF_FACESIZE - 1] = 0;
			}
            else {
                lstrcpynA(lf->lfFaceName, dbv.pszVal, LF_FACESIZE);
				lf->lfFaceName[LF_FACESIZE - 1] = 0;
                DBFreeVariant(&dbv);
            }
        }
    }
}


 /*
  * Font Service support
  */

/*
static struct _colornames { char *szName; char *szModule; char *szSetting; DWORD defclr; } colornames[] = {
    "Background", FONTMODULE, SRMSGSET_BKGCOLOUR, 0,
    "Incoming events", FONTMODULE, "inbg", 0,
    "Outgoing events", FONTMODULE, "outbg", 0,
    "Input area background", FONTMODULE, "inputbg", 0,
    "Horizontal grid lines", FONTMODULE, "hgrid", 0,
    NULL, NULL, NULL
};*/


/*
void FS_RegisterFonts()
{
    FontID fid = {0};
    int i = 0;
    char szTemp[50];
    HDC hdc;
    DBVARIANT dbv;
    ColourID cid = {0};

    colornames[0].defclr = colornames[1].defclr = colornames[3].defclr = GetSysColor(COLOR_WINDOW);
    colornames[2].defclr = GetSysColor(COLOR_3DFACE);
    
    cid.cbSize = sizeof(cid);
    strncpy(cid.group, "TabSRMM Message log", sizeof(cid.group));
    strncpy(cid.dbSettingsGroup, FONTMODULE, sizeof(cid.dbSettingsGroup));
    while(colornames[i].szName != NULL) {
        strncpy(cid.name, colornames[i].szName, sizeof(cid.name));
        strncpy(cid.setting, colornames[i].szSetting, sizeof(cid.setting));
        cid.defcolour = DBGetContactSettingDword(NULL, colornames[i].szModule, colornames[i].szSetting, RGB(224, 224, 224));
        CallService(MS_COLOUR_REGISTER, (WPARAM)&cid, 0);
		//MessageBoxA(0, colornames[i].szName, colornames[i].szSetting, MB_OK);
        cid.order++;
        i++;
    }

    fid.cbSize = sizeof(fid);
    strncpy(fid.group, "TabSRMM Message log", sizeof(fid.group));
    strncpy(fid.dbSettingsGroup, FONTMODULE, sizeof(fid.dbSettingsGroup));
    fid.flags = FIDF_DEFAULTVALID | FIDF_ALLOWEFFECTS;
    for(i = 0; i < MSGDLGFONTCOUNT; i++) {
        _snprintf(szTemp, sizeof(szTemp), "Font%d", i);
        strncpy(fid.prefix, szTemp, sizeof(fid.prefix));
        fid.order = i;
        strncpy(fid.name, szFontIdDescr[i], sizeof(fid.name));
        _snprintf(szTemp, sizeof(szTemp), "Font%dCol", i);
        fid.deffontsettings.colour = (COLORREF)DBGetContactSettingDword(NULL, FONTMODULE, szTemp, GetSysColor(COLOR_BTNTEXT));
        
        hdc = GetDC(NULL);
        _snprintf(szTemp, sizeof(szTemp), "Font%dSize", i);
        if(i == H_MSGFONTID_DIVIDERS)
            fid.deffontsettings.size = 5;
        else {
            fid.deffontsettings.size = (BYTE)DBGetContactSettingByte(NULL, FONTMODULE, szTemp, 10);
            fid.deffontsettings.size = -MulDiv(fid.deffontsettings.size, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        }
        ReleaseDC(NULL,hdc);				

        _snprintf(szTemp, sizeof(szTemp), "Font%dSty", i);
        fid.deffontsettings.style = DBGetContactSettingByte(NULL, FONTMODULE, szTemp, fontOptionsList[0].defStyle);
        _snprintf(szTemp, sizeof(szTemp), "Font%dSet", i);
        fid.deffontsettings.charset = DBGetContactSettingByte(NULL, FONTMODULE, szTemp, fontOptionsList[0].defCharset);
        _snprintf(szTemp, sizeof(szTemp), "Font%d", i);
        if(i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT) {
            lstrcpynA(fid.deffontsettings.szFace, "Arial", LF_FACESIZE);
            fid.deffontsettings.charset = 0;
        }
        else {
            if (DBGetContactSetting(NULL, FONTMODULE, szTemp, &dbv))
                lstrcpynA(fid.deffontsettings.szFace, "Tahoma", LF_FACESIZE);
            else {
                lstrcpynA(fid.deffontsettings.szFace, dbv.pszVal, LF_FACESIZE);
                DBFreeVariant(&dbv);
            }
        }
        CallService(MS_FONT_REGISTER, (WPARAM)&fid, 0);
    }

    strncpy(cid.group, "TabSRMM - other", sizeof(cid.group));
    strncpy(cid.name, "Background - Infopanel fields", sizeof(cid.name));
    strncpy(cid.setting, "ipfieldsbg", sizeof(cid.setting));
    cid.defcolour = DBGetContactSettingDword(NULL, FONTMODULE, "ipfieldsbg", GetSysColor(COLOR_3DFACE));
    CallService(MS_COLOUR_REGISTER, (WPARAM)&cid, 0);

    strncpy(cid.name, "Analog clock symbol", sizeof(cid.name));
    strncpy(cid.setting, "col_clock", sizeof(cid.setting));
    cid.defcolour = DBGetContactSettingDword(NULL, FONTMODULE, "col_clock", GetSysColor(COLOR_WINDOWTEXT));
    CallService(MS_COLOUR_REGISTER, (WPARAM)&cid, 0);
    
    strncpy(fid.group, "TabSRMM - other", sizeof(fid.group));
    fid.flags = FIDF_DEFAULTVALID | FIDF_ALLOWEFFECTS | FIDF_SAVEACTUALHEIGHT;
    myGlobals.ipConfig.isValid = TRUE;
    for(i = 0; i < IPFONTCOUNT; i++) {
        _snprintf(szTemp, sizeof(szTemp), "Font%d", 100 + i);
        strncpy(fid.prefix, szTemp, sizeof(fid.prefix));
        fid.order = i;
        strncpy(fid.name, szIPFontDescr[i], sizeof(fid.name));
        _snprintf(szTemp, sizeof(szTemp), "Font%dCol", 100 + i);
        fid.deffontsettings.colour = (COLORREF)DBGetContactSettingDword(NULL, FONTMODULE, szTemp, GetSysColor(COLOR_WINDOWTEXT));

        hdc = GetDC(NULL);
        _snprintf(szTemp, sizeof(szTemp), "Font%dSize", 100 + i);
        fid.deffontsettings.size = (BYTE)DBGetContactSettingByte(NULL, FONTMODULE, szTemp, 8);
        fid.deffontsettings.size = -MulDiv(fid.deffontsettings.size, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ReleaseDC(NULL,hdc);				

        _snprintf(szTemp, sizeof(szTemp), "Font%dSty", 100 + i);
        fid.deffontsettings.style = DBGetContactSettingByte(NULL, FONTMODULE, szTemp, fontOptionsList[0].defStyle);
        _snprintf(szTemp, sizeof(szTemp), "Font%dSet", 100 + i);
        fid.deffontsettings.charset = DBGetContactSettingByte(NULL, FONTMODULE, szTemp, fontOptionsList[0].defCharset);
        _snprintf(szTemp, sizeof(szTemp), "Font%d", 100 + i);
        if (DBGetContactSetting(NULL, FONTMODULE, szTemp, &dbv))
            lstrcpynA(fid.deffontsettings.szFace, "Tahoma", LF_FACESIZE);
        else {
            lstrcpynA(fid.deffontsettings.szFace, dbv.pszVal, LF_FACESIZE);
            DBFreeVariant(&dbv);
        }
        CallService(MS_FONT_REGISTER, (WPARAM)&fid, 0);
    }
}

int FS_ReloadFonts(WPARAM wParam, LPARAM lParam)
{
    CacheLogFonts();
    WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 0, 0);
    return 0;
}

*/

static struct LISTOPTIONSGROUP defaultGroups[] = {
    0, _T("Message window behaviour"),
    0, _T("Sending messages"),
    0, _T("Other options"),
    0, NULL
};

static struct LISTOPTIONSITEM defaultItems[] = {
    0, _T("Send on SHIFT - Enter"), IDC_SENDONSHIFTENTER, LOI_TYPE_SETTING, (UINT_PTR)"sendonshiftenter", 1,
    0, _T("Send message on 'Enter'"), SRMSGDEFSET_SENDONENTER, LOI_TYPE_SETTING, (UINT_PTR)SRMSGSET_SENDONENTER, 1,
    0, _T("Send message on double 'Enter'"), 0, LOI_TYPE_SETTING, (UINT_PTR)"SendOnDblEnter", 1,
    0, _T("Minimize the message window on send"), SRMSGDEFSET_AUTOMIN, LOI_TYPE_SETTING, (UINT_PTR)SRMSGSET_AUTOMIN, 1,
    0, _T("Allow the toolbar to hide the send button"), 1, LOI_TYPE_SETTING, (UINT_PTR)"hidesend", 1,
    0, _T("Flash contact list and tray icons for new events in unfocused windows"), 0, LOI_TYPE_SETTING, (UINT_PTR)"flashcl", 0,
    0, _T("Delete temporary contacts on close"), 0, LOI_TYPE_SETTING, (UINT_PTR)"deletetemp", 0,
    0, _T("Retrieve status message when hovering info panel"), 0, LOI_TYPE_SETTING, (UINT_PTR)"dostatusmsg", 0,
    0, _T("Enable event API (support for third party plugins)"), 0, LOI_TYPE_SETTING, (UINT_PTR)"eventapi", 2,
    0, _T("Don't send UNICODE parts for 7bit ANSI messages (saves db space)"), 1, LOI_TYPE_SETTING, (UINT_PTR)"7bitasANSI", 2,
    0, _T("Always keep the button bar at full width"), 0, LOI_TYPE_SETTING, (UINT_PTR)"alwaysfulltoolbar", 1,
    0, _T("Allow PASTE AND SEND feature"), 0, LOI_TYPE_SETTING, (UINT_PTR)"pasteandsend", 1,
    0, NULL, 0, 0, 0, 0
};

static HIMAGELIST g_himlOptions;

static BOOL CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
			DWORD msgTimeout;
			BOOL translated;
			TVINSERTSTRUCT tvi = {0};
			int i = 0;
            BYTE avMode;

			DWORD dwFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);

			TranslateDialogDefault(hwndDlg);
			SetWindowLong(GetDlgItem(hwndDlg, IDC_WINDOWOPTIONS), GWL_STYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_WINDOWOPTIONS), GWL_STYLE) | (TVS_NOHSCROLL | TVS_CHECKBOXES));

			g_himlOptions = (HIMAGELIST)SendDlgItemMessage(hwndDlg, IDC_WINDOWOPTIONS, TVM_SETIMAGELIST, TVSIL_STATE, (LPARAM)CreateStateImageList());
            if(g_himlOptions)
                ImageList_Destroy(g_himlOptions);
			/*
			* fill the list box, create groups first, then add items
			*/

			while(defaultGroups[i].szName != NULL) {
				tvi.hParent = 0;
				tvi.hInsertAfter = TVI_LAST;
				tvi.item.mask = TVIF_TEXT | TVIF_STATE;
				tvi.item.pszText = TranslateTS(defaultGroups[i].szName);
				tvi.item.stateMask = TVIS_STATEIMAGEMASK | TVIS_EXPANDED | TVIS_BOLD;
				tvi.item.state = INDEXTOSTATEIMAGEMASK(0) | TVIS_EXPANDED | TVIS_BOLD;
				defaultGroups[i++].handle = (LRESULT)TreeView_InsertItem( GetDlgItem(hwndDlg, IDC_WINDOWOPTIONS), &tvi);
			}

			i = 0;

			while(defaultItems[i].szName != 0) {
				tvi.hParent = (HTREEITEM)defaultGroups[defaultItems[i].uGroup].handle;
				tvi.hInsertAfter = TVI_LAST;
				tvi.item.pszText = TranslateTS(defaultItems[i].szName);
				tvi.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
				tvi.item.lParam = i;
				tvi.item.stateMask = TVIS_STATEIMAGEMASK;
				if(defaultItems[i].uType == LOI_TYPE_SETTING)
					tvi.item.state = INDEXTOSTATEIMAGEMASK(DBGetContactSettingByte(NULL, SRMSGMOD_T, (char *)defaultItems[i].lParam, (BYTE)defaultItems[i].id) ? 3 : 2);
				defaultItems[i].handle = (LRESULT)TreeView_InsertItem( GetDlgItem(hwndDlg, IDC_WINDOWOPTIONS), &tvi);
				i++;
			}

			SendDlgItemMessage(hwndDlg, IDC_AVATARBORDER, CB_INSERTSTRING, -1, (LPARAM)TranslateT("None"));
			SendDlgItemMessage(hwndDlg, IDC_AVATARBORDER, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Automatic"));
			SendDlgItemMessage(hwndDlg, IDC_AVATARBORDER, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Sunken"));
			SendDlgItemMessage(hwndDlg, IDC_AVATARBORDER, CB_INSERTSTRING, -1, (LPARAM)TranslateT("1 pixel solid"));
			SendDlgItemMessage(hwndDlg, IDC_AVATARBORDER, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Rounded border"));
			SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, SRMSGMOD_T, "avborderclr", RGB(0, 0, 0)));

			SendDlgItemMessage(hwndDlg, IDC_AVATARBORDER, CB_SETCURSEL, (WPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "avbordertype", 1), 0);

			SendDlgItemMessage(hwndDlg, IDC_AVATARMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Globally on"));
			SendDlgItemMessage(hwndDlg, IDC_AVATARMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("On, if present"));
			SendDlgItemMessage(hwndDlg, IDC_AVATARMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Globally OFF"));

			SendDlgItemMessage(hwndDlg, IDC_OWNAVATARMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Show them if present"));
			SendDlgItemMessage(hwndDlg, IDC_OWNAVATARMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Don't show them"));

			SendDlgItemMessage(hwndDlg, IDC_AVATARDISPLAY, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Dynamically resize"));
			SendDlgItemMessage(hwndDlg, IDC_AVATARDISPLAY, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Static"));

            switch(DBGetContactSettingByte(NULL, SRMSGMOD_T, "avatarmode", 0)) {
                case 4:
                    avMode = 2;
                    break;
                case 3:
                case 2:
                case 1:
                    avMode = 1;
                    break;
                case 0:
                    avMode = 0;
            }
            SendDlgItemMessage(hwndDlg, IDC_AVATARMODE, CB_SETCURSEL, (WPARAM)avMode, 0);
			SendDlgItemMessage(hwndDlg, IDC_OWNAVATARMODE, CB_SETCURSEL, (WPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "ownavatarmode", 0), 0);

			SendDlgItemMessage(hwndDlg, IDC_AVATARDISPLAY, CB_SETCURSEL, (WPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "avatardisplaymode", 0), 0);

			msgTimeout = DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_MSGTIMEOUT, SRMSGDEFSET_MSGTIMEOUT);
			SetDlgItemInt(hwndDlg, IDC_SECONDS, msgTimeout >= SRMSGSET_MSGTIMEOUT_MIN ? msgTimeout / 1000 : SRMSGDEFSET_MSGTIMEOUT / 1000, FALSE);

			SetDlgItemInt(hwndDlg, IDC_MAXAVATARHEIGHT, DBGetContactSettingDword(NULL, SRMSGMOD_T, "avatarheight", 100), FALSE);
			SendDlgItemMessage(hwndDlg, IDC_AVATARSPIN, UDM_SETRANGE, 0, MAKELONG(150, 50));
			SendDlgItemMessage(hwndDlg, IDC_AVATARSPIN, UDM_SETPOS, 0, GetDlgItemInt(hwndDlg, IDC_MAXAVATARHEIGHT, &translated, FALSE));

            EnableWindow(GetDlgItem(hwndDlg, IDC_MAXAVATARHEIGHT), DBGetContactSettingByte(NULL, SRMSGMOD_T, "avatardisplaymode", 0) != 0);

			SetDlgItemInt(hwndDlg, IDC_AUTOCLOSETABTIME, DBGetContactSettingDword(NULL, SRMSGMOD_T, "tabautoclose", 0), FALSE);
			SendDlgItemMessage(hwndDlg, IDC_AUTOCLOSETABSPIN, UDM_SETRANGE, 0, MAKELONG(1800, 0));
			SendDlgItemMessage(hwndDlg, IDC_AUTOCLOSETABSPIN, UDM_SETPOS, 0, GetDlgItemInt(hwndDlg, IDC_AUTOCLOSETABTIME, &translated, FALSE));
			CheckDlgButton(hwndDlg, IDC_AUTOCLOSELAST, DBGetContactSettingByte(NULL, SRMSGMOD_T, "autocloselast", 0));

			SendDlgItemMessage(hwndDlg, IDC_SENDFORMATTING, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Off"));
			SendDlgItemMessage(hwndDlg, IDC_SENDFORMATTING, CB_INSERTSTRING, -1, (LPARAM)TranslateT("BBCode"));

			SendDlgItemMessage(hwndDlg, IDC_SENDFORMATTING, CB_SETCURSEL, (WPARAM)myGlobals.m_SendFormat ? 1 : 0, 0);

			EnableWindow(GetDlgItem(hwndDlg, IDC_AUTOCLOSELAST), GetDlgItemInt(hwndDlg, IDC_AUTOCLOSETABTIME, &translated, FALSE) > 0);
			return TRUE;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SECONDS:
		case IDC_MAXAVATARHEIGHT:
		case IDC_AUTOCLOSETABTIME:
			{
				BOOL translated;

				EnableWindow(GetDlgItem(hwndDlg, IDC_AUTOCLOSELAST), GetDlgItemInt(hwndDlg, IDC_AUTOCLOSETABTIME, &translated, FALSE) > 0);
				if (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus())
					return TRUE;
				break;
			}
		case IDC_AVATARBORDER:
        case IDC_AVATARDISPLAY:
            EnableWindow(GetDlgItem(hwndDlg, IDC_MAXAVATARHEIGHT), SendDlgItemMessage(hwndDlg, IDC_AVATARDISPLAY, CB_GETCURSEL, 0, 0) != 0);
			if(HIWORD(wParam) != CBN_SELCHANGE || (HWND)lParam != GetFocus())
				return TRUE;
			break;
		}
        SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
        break;
		case WM_NOTIFY:
			switch (((LPNMHDR) lParam)->idFrom) {
		case IDC_WINDOWOPTIONS:
			if(((LPNMHDR)lParam)->code==NM_CLICK) {
				TVHITTESTINFO hti;
				TVITEM item = {0};

				item.mask = TVIF_HANDLE | TVIF_STATE;
				item.stateMask = TVIS_STATEIMAGEMASK | TVIS_BOLD;
				hti.pt.x=(short)LOWORD(GetMessagePos());
				hti.pt.y=(short)HIWORD(GetMessagePos());
				ScreenToClient(((LPNMHDR)lParam)->hwndFrom, &hti.pt);
				if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom, &hti)) {
					item.hItem = (HTREEITEM)hti.hItem;
					SendDlgItemMessageA(hwndDlg, IDC_WINDOWOPTIONS, TVM_GETITEMA, 0, (LPARAM)&item);
					if(item.state & TVIS_BOLD && hti.flags & TVHT_ONITEMSTATEICON) {
						item.state = INDEXTOSTATEIMAGEMASK(0) | TVIS_BOLD;
						SendDlgItemMessageA(hwndDlg, IDC_WINDOWOPTIONS, TVM_SETITEMA, 0, (LPARAM)&item);
					}
					else if(hti.flags&TVHT_ONITEMSTATEICON) {
						if(((item.state & TVIS_STATEIMAGEMASK) >> 12) == 3) {
							item.state = INDEXTOSTATEIMAGEMASK(1);
							SendDlgItemMessageA(hwndDlg, IDC_WINDOWOPTIONS, TVM_SETITEMA, 0, (LPARAM)&item);
						}
						SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					}
				}
			}
			break;
		case 0:
			switch (((LPNMHDR) lParam)->code) {
			case PSN_APPLY:
				{
					DWORD msgTimeout;
					DWORD dwFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);
					BOOL translated;
					TVITEM item = {0};
					int i = 0;
                    BYTE avMode;

                    switch(SendDlgItemMessage(hwndDlg, IDC_AVATARMODE, CB_GETCURSEL, 0, 0)) {
                        case 0:
                            avMode = 0;
                            break;
                        case 1:
                            avMode = 3;
                            break;
                        case 2:
                            avMode = 4;
                    }
					DBWriteContactSettingByte(NULL, SRMSGMOD_T, "avbordertype", (BYTE) SendDlgItemMessage(hwndDlg, IDC_AVATARBORDER, CB_GETCURSEL, 0, 0));
					DBWriteContactSettingDword(NULL, SRMSGMOD_T, "avborderclr", SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR, 0, 0));
					DBWriteContactSettingByte(NULL, SRMSGMOD_T, "avatarmode", avMode);
					DBWriteContactSettingByte(NULL, SRMSGMOD_T, "ownavatarmode", (BYTE) SendDlgItemMessage(hwndDlg, IDC_OWNAVATARMODE, CB_GETCURSEL, 0, 0));
					DBWriteContactSettingByte(NULL, SRMSGMOD_T, "avatardisplaymode", (BYTE) SendDlgItemMessage(hwndDlg, IDC_AVATARDISPLAY, CB_GETCURSEL, 0, 0));

					DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", dwFlags);
					DBWriteContactSettingDword(NULL, SRMSGMOD_T, "avatarheight", GetDlgItemInt(hwndDlg, IDC_MAXAVATARHEIGHT, &translated, FALSE));

					DBWriteContactSettingDword(NULL, SRMSGMOD_T, "tabautoclose", GetDlgItemInt(hwndDlg, IDC_AUTOCLOSETABTIME, &translated, FALSE));
					DBWriteContactSettingByte(NULL, SRMSGMOD_T, "autocloselast", IsDlgButtonChecked(hwndDlg, IDC_AUTOCLOSELAST));
					DBWriteContactSettingByte(NULL, SRMSGMOD_T, "sendformat", (BYTE)SendDlgItemMessage(hwndDlg, IDC_SENDFORMATTING, CB_GETCURSEL, 0, 0));

					msgTimeout = GetDlgItemInt(hwndDlg, IDC_SECONDS, NULL, TRUE) >= SRMSGSET_MSGTIMEOUT_MIN / 1000 ? GetDlgItemInt(hwndDlg, IDC_SECONDS, NULL, TRUE) * 1000 : SRMSGDEFSET_MSGTIMEOUT;
					DBWriteContactSettingDword(NULL, SRMSGMOD, SRMSGSET_MSGTIMEOUT, msgTimeout);
					/*
					* scan the tree view and obtain the options...
					*/
					while(defaultItems[i].szName != NULL) {
						item.mask = TVIF_HANDLE | TVIF_STATE;
						item.hItem = (HTREEITEM)defaultItems[i].handle;
						item.stateMask = TVIS_STATEIMAGEMASK;

						SendDlgItemMessageA(hwndDlg, IDC_WINDOWOPTIONS, TVM_GETITEMA, 0, (LPARAM)&item);
						if(defaultItems[i].uType == LOI_TYPE_SETTING)
							DBWriteContactSettingByte(NULL, SRMSGMOD_T, (char *)defaultItems[i].lParam, (item.state >> 12) == 3 ? 1 : 0);
						i++;
					}
					ReloadGlobals();
					WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 1, 0);
					return TRUE;
				}
			}
			break;
		}
		break;
	}
	return FALSE;
}

static struct LISTOPTIONSGROUP lvGroups[] = {
    0, _T("Message log appearance"),
    0, _T("Support for external plugins"),
    0, _T("Other options"),
    0, NULL
};

static struct LISTOPTIONSITEM lvItems[] = {
    0, _T("Show file events"), 1, LOI_TYPE_SETTING, (UINT_PTR)SRMSGSET_SHOWFILES, 0,
    0, _T("Show url events"), 1, LOI_TYPE_SETTING, (UINT_PTR)SRMSGSET_SHOWURLS, 0,
    0, _T("Draw grid lines"), IDC_DRAWGRID, LOI_TYPE_FLAG,  MWF_LOG_GRID, 0,
    0, _T("Show Icons"), 1, LOI_TYPE_FLAG, MWF_LOG_SHOWICONS, 0,
    0, _T("Show Symbols"), 1, LOI_TYPE_FLAG, MWF_LOG_SYMBOLS, 0,
    0, _T("Use Incoming/Outgoing Icons"), 1, LOI_TYPE_FLAG, MWF_LOG_INOUTICONS, 0,
    0, _T("Use Message Grouping"), 1, LOI_TYPE_FLAG, MWF_LOG_GROUPMODE, 0,
    0, _T("Indent message body"), IDC_INDENT, LOI_TYPE_FLAG, MWF_LOG_INDENT, 0,
    0, _T("Simple text formatting (*bold* etc.)"), 0, LOI_TYPE_FLAG, MWF_LOG_TEXTFORMAT, 0,
    0, _T("Support BBCode formatting"), 1, LOI_TYPE_SETTING, (UINT_PTR)"log_bbcode", 0,
    0, _T("Place dividers in inactive sessions"), 0, LOI_TYPE_SETTING, (UINT_PTR)"usedividers", 0,
    0, _T("Use popup configuration for placing dividers"), 0, LOI_TYPE_SETTING, (UINT_PTR)"div_popupconfig", 0,
    0, _T("RTL is default text direction"), 0, LOI_TYPE_SETTING, (UINT_PTR)"rtldefault", 0,
    0, _T("Support Math Module plugin"), 0, LOI_TYPE_SETTING, (UINT_PTR)"wantmathmod", 1,
    0, _T("Log status changes"), 0, LOI_TYPE_SETTING, (UINT_PTR)"logstatus", 2,
    0, _T("Automatically copy selected text"), 0, LOI_TYPE_SETTING, (UINT_PTR)"autocopy", 2,
    0, _T("Use multiple background colors"), IDC_AUTOSELECTCOPY, LOI_TYPE_FLAG, (UINT_PTR)MWF_LOG_INDIVIDUALBKG, 0,
    0, _T("Also draw vertical grid lines"), 0, LOI_TYPE_SETTING, (UINT_PTR)"wantvgrid", 0,
    0, NULL, 0, 0, 0, 0
};

static BOOL CALLBACK DlgProcLogOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL translated;
	DWORD dwFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);

	switch (msg) {
	case WM_INITDIALOG:
		{
			TVINSERTSTRUCT tvi = {0};
			int i = 0;
			DWORD maxhist = DBGetContactSettingDword(NULL, SRMSGMOD_T, "maxhist", 0);

			TranslateDialogDefault(hwndDlg);
			switch (DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_LOADHISTORY, SRMSGDEFSET_LOADHISTORY)) {
			case LOADHISTORY_UNREAD:
				CheckDlgButton(hwndDlg, IDC_LOADUNREAD, BST_CHECKED);
				break;
			case LOADHISTORY_COUNT:
				CheckDlgButton(hwndDlg, IDC_LOADCOUNT, BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_LOADCOUNTN), TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_LOADCOUNTSPIN), TRUE);
				break;
			case LOADHISTORY_TIME:
				CheckDlgButton(hwndDlg, IDC_LOADTIME, BST_CHECKED);
				EnableWindow(GetDlgItem(hwndDlg, IDC_LOADTIMEN), TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_LOADTIMESPIN), TRUE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_STMINSOLD), TRUE);
				break;
			}
			SetWindowLong(GetDlgItem(hwndDlg, IDC_LOGOPTIONS), GWL_STYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_LOGOPTIONS), GWL_STYLE) | (TVS_NOHSCROLL | TVS_CHECKBOXES));
			g_himlOptions = (HIMAGELIST)SendDlgItemMessage(hwndDlg, IDC_LOGOPTIONS, TVM_SETIMAGELIST, TVSIL_STATE, (LPARAM)CreateStateImageList());
            if(g_himlOptions)
                ImageList_Destroy(g_himlOptions);

			/*
			* fill the list box, create groups first, then add items
			*/

			while(lvGroups[i].szName != NULL) {
				tvi.hParent = 0;
				tvi.hInsertAfter = TVI_LAST;
				tvi.item.mask = TVIF_TEXT | TVIF_STATE;
				tvi.item.pszText = TranslateTS(lvGroups[i].szName);
				tvi.item.stateMask = TVIS_STATEIMAGEMASK | TVIS_EXPANDED | TVIS_BOLD;
				tvi.item.state = INDEXTOSTATEIMAGEMASK(0)|TVIS_EXPANDED | TVIS_BOLD;
				lvGroups[i++].handle = (LRESULT)TreeView_InsertItem( GetDlgItem(hwndDlg, IDC_LOGOPTIONS), &tvi);
			}

			i = 0;

			while(lvItems[i].szName != 0) {
				tvi.hParent = (HTREEITEM)lvGroups[lvItems[i].uGroup].handle;
				tvi.hInsertAfter = TVI_LAST;
				tvi.item.pszText = TranslateTS(lvItems[i].szName);
				tvi.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
				tvi.item.lParam = i;
				if(lvItems[i].uType == LOI_TYPE_SETTING) {
					if(!strcmp((char *)lvItems[i].lParam, "wantmathmod") && !ServiceExists(MATH_RTF_REPLACE_FORMULAE)) {
						i++;
						continue;
					}
				}
				tvi.item.stateMask=TVIS_STATEIMAGEMASK;
				if(lvItems[i].uType == LOI_TYPE_FLAG)
					tvi.item.state = INDEXTOSTATEIMAGEMASK((dwFlags & (UINT)lvItems[i].lParam) ? 3 : 2);
				else if(lvItems[i].uType == LOI_TYPE_SETTING)
					tvi.item.state = INDEXTOSTATEIMAGEMASK(DBGetContactSettingByte(NULL, SRMSGMOD_T, (char *)lvItems[i].lParam, lvItems[i].id) ? 3 : 2);
				lvItems[i].handle = (LRESULT)TreeView_InsertItem( GetDlgItem(hwndDlg, IDC_LOGOPTIONS), &tvi);
				i++;
			}
			SendDlgItemMessage(hwndDlg, IDC_LOADCOUNTSPIN, UDM_SETRANGE, 0, MAKELONG(100, 0));
			SendDlgItemMessage(hwndDlg, IDC_LOADCOUNTSPIN, UDM_SETPOS, 0, DBGetContactSettingWord(NULL, SRMSGMOD, SRMSGSET_LOADCOUNT, SRMSGDEFSET_LOADCOUNT));
			SendDlgItemMessage(hwndDlg, IDC_LOADTIMESPIN, UDM_SETRANGE, 0, MAKELONG(12 * 60, 0));
			SendDlgItemMessage(hwndDlg, IDC_LOADTIMESPIN, UDM_SETPOS, 0, DBGetContactSettingWord(NULL, SRMSGMOD, SRMSGSET_LOADTIME, SRMSGDEFSET_LOADTIME));

			SetDlgItemInt(hwndDlg, IDC_INDENTAMOUNT, DBGetContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", 0), FALSE);
			SendDlgItemMessage(hwndDlg, IDC_INDENTSPIN, UDM_SETRANGE, 0, MAKELONG(1000, 0));
			SendDlgItemMessage(hwndDlg, IDC_INDENTSPIN, UDM_SETPOS, 0, GetDlgItemInt(hwndDlg, IDC_INDENTAMOUNT, &translated, FALSE));

			SetDlgItemInt(hwndDlg, IDC_RIGHTINDENT, DBGetContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", 0), FALSE);
			SendDlgItemMessage(hwndDlg, IDC_RINDENTSPIN, UDM_SETRANGE, 0, MAKELONG(1000, 0));
			SendDlgItemMessage(hwndDlg, IDC_RINDENTSPIN, UDM_SETPOS, 0, GetDlgItemInt(hwndDlg, IDC_RIGHTINDENT, &translated, FALSE));
			SendMessage(hwndDlg, WM_COMMAND, MAKELONG(IDC_INDENT, 0), 0);

			SetDlgItemInt(hwndDlg, IDC_EXTRAMICROLF, DBGetContactSettingByte(NULL, SRMSGMOD_T, "extramicrolf", 0), FALSE);
			SendDlgItemMessage(hwndDlg, IDC_EXTRALFSPIN, UDM_SETRANGE, 0, MAKELONG(5, 0));
			SendDlgItemMessage(hwndDlg, IDC_EXTRALFSPIN, UDM_SETPOS, 0, GetDlgItemInt(hwndDlg, IDC_EXTRAMICROLF, &translated, FALSE));

			SendDlgItemMessage(hwndDlg, IDC_TRIMSPIN, UDM_SETRANGE, 0, MAKELONG(1000, 5));
			SendDlgItemMessage(hwndDlg, IDC_TRIMSPIN, UDM_SETPOS, 0, maxhist);
			EnableWindow(GetDlgItem(hwndDlg, IDC_TRIMSPIN), maxhist != 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_TRIM), maxhist != 0);
			CheckDlgButton(hwndDlg, IDC_ALWAYSTRIM, maxhist != 0);
			return TRUE;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ALWAYSTRIM:
			EnableWindow(GetDlgItem(hwndDlg, IDC_TRIMSPIN), IsDlgButtonChecked(hwndDlg, IDC_ALWAYSTRIM));
			EnableWindow(GetDlgItem(hwndDlg, IDC_TRIM), IsDlgButtonChecked(hwndDlg, IDC_ALWAYSTRIM));
			break;
		case IDC_LOADUNREAD:
		case IDC_LOADCOUNT:
		case IDC_LOADTIME:
			EnableWindow(GetDlgItem(hwndDlg, IDC_LOADCOUNTN), IsDlgButtonChecked(hwndDlg, IDC_LOADCOUNT));
			EnableWindow(GetDlgItem(hwndDlg, IDC_LOADCOUNTSPIN), IsDlgButtonChecked(hwndDlg, IDC_LOADCOUNT));
			EnableWindow(GetDlgItem(hwndDlg, IDC_LOADTIMEN), IsDlgButtonChecked(hwndDlg, IDC_LOADTIME));
			EnableWindow(GetDlgItem(hwndDlg, IDC_LOADTIMESPIN), IsDlgButtonChecked(hwndDlg, IDC_LOADTIME));
			EnableWindow(GetDlgItem(hwndDlg, IDC_STMINSOLD), IsDlgButtonChecked(hwndDlg, IDC_LOADTIME));
			break;
		case IDC_INDENTAMOUNT:
		case IDC_LOADCOUNTN:
		case IDC_LOADTIMEN:
		case IDC_RIGHTINDENT:
		case IDC_TRIM:
			if (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus())
				return TRUE;
			break;
        /*
		case IDC_TSFIX:
			if(IsDlgButtonChecked(hwndDlg, IDC_TSFIX))
				MessageBoxA(0, "Caution: This attempt to fix the 'future timestamp issue' may have side effects. Also, it only works for events while the session is active, NOT for loading the history", "Warning", MB_OK | MB_ICONHAND);
			break;
        */
		case IDC_MODIFY:
			{
				TemplateEditorNew teNew = {0, 0, hwndDlg};
				CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_TEMPLATEEDIT), hwndDlg, DlgProcTemplateEditor, (LPARAM)&teNew);
				break;
			}
		case IDC_RTLMODIFY:
			{
				TemplateEditorNew teNew = {0, TRUE, hwndDlg};
				CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_TEMPLATEEDIT), hwndDlg, DlgProcTemplateEditor, (LPARAM)&teNew);
				break;
			}
		}
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->idFrom) {
		case IDC_LOGOPTIONS:
			if(((LPNMHDR)lParam)->code==NM_CLICK) {
				TVHITTESTINFO hti;
				TVITEM item = {0};

				item.mask = TVIF_HANDLE | TVIF_STATE;
				item.stateMask = TVIS_STATEIMAGEMASK | TVIS_BOLD;
				hti.pt.x=(short)LOWORD(GetMessagePos());
				hti.pt.y=(short)HIWORD(GetMessagePos());
				ScreenToClient(((LPNMHDR)lParam)->hwndFrom, &hti.pt);
				if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom, &hti)) {
					item.hItem = (HTREEITEM)hti.hItem;
					SendDlgItemMessageA(hwndDlg, IDC_LOGOPTIONS, TVM_GETITEMA, 0, (LPARAM)&item);
					if(item.state & TVIS_BOLD && hti.flags & TVHT_ONITEMSTATEICON) {
						item.state = INDEXTOSTATEIMAGEMASK(0) | TVIS_BOLD;
						SendDlgItemMessageA(hwndDlg, IDC_LOGOPTIONS, TVM_SETITEMA, 0, (LPARAM)&item);
					}
					else if(hti.flags&TVHT_ONITEMSTATEICON) {
						if(((item.state & TVIS_STATEIMAGEMASK) >> 12) == 3) {
							item.state = INDEXTOSTATEIMAGEMASK(1);
							SendDlgItemMessageA(hwndDlg, IDC_LOGOPTIONS, TVM_SETITEMA, 0, (LPARAM)&item);
						}
						SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					}
				}
			}
			break;
		default:
			switch (((LPNMHDR) lParam)->code) {
			case PSN_APPLY: 
				{
					int i = 0;
					TVITEM item = {0};
					dwFlags &= ~(MWF_LOG_INDIVIDUALBKG | MWF_LOG_TEXTFORMAT | MWF_LOG_GRID | MWF_LOG_INDENT | MWF_LOG_SHOWICONS | MWF_LOG_SYMBOLS | MWF_LOG_INOUTICONS | MWF_LOG_GROUPMODE);

					if (IsDlgButtonChecked(hwndDlg, IDC_LOADCOUNT))
						DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_LOADHISTORY, LOADHISTORY_COUNT);
					else if (IsDlgButtonChecked(hwndDlg, IDC_LOADTIME))
						DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_LOADHISTORY, LOADHISTORY_TIME);
					else
						DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_LOADHISTORY, LOADHISTORY_UNREAD);
					DBWriteContactSettingWord(NULL, SRMSGMOD, SRMSGSET_LOADCOUNT, (WORD) SendDlgItemMessage(hwndDlg, IDC_LOADCOUNTSPIN, UDM_GETPOS, 0, 0));
					DBWriteContactSettingWord(NULL, SRMSGMOD, SRMSGSET_LOADTIME, (WORD) SendDlgItemMessage(hwndDlg, IDC_LOADTIMESPIN, UDM_GETPOS, 0, 0));

					DBWriteContactSettingDword(NULL, SRMSGMOD_T, "IndentAmount", (DWORD) GetDlgItemInt(hwndDlg, IDC_INDENTAMOUNT, &translated, FALSE));
					DBWriteContactSettingDword(NULL, SRMSGMOD_T, "RightIndent", (DWORD) GetDlgItemInt(hwndDlg, IDC_RIGHTINDENT, &translated, FALSE));

					/*
					* scan the tree view and obtain the options...
					*/
					while(lvItems[i].szName != NULL) {
						item.mask = TVIF_HANDLE | TVIF_STATE;
						item.hItem = (HTREEITEM)lvItems[i].handle;
						item.stateMask = TVIS_STATEIMAGEMASK;

						SendDlgItemMessageA(hwndDlg, IDC_LOGOPTIONS, TVM_GETITEMA, 0, (LPARAM)&item);
						if(lvItems[i].uType == LOI_TYPE_FLAG)
							dwFlags |= (item.state >> 12) == 3 ? lvItems[i].lParam : 0;
						else if(lvItems[i].uType == LOI_TYPE_SETTING)
							DBWriteContactSettingByte(NULL, SRMSGMOD_T, (char *)lvItems[i].lParam, (item.state >> 12) == 3 ? 1 : 0);
						i++;
					}
					DBWriteContactSettingDword(NULL, SRMSGMOD_T, "mwflags", dwFlags);
					DBWriteContactSettingByte(NULL, SRMSGMOD_T, "extramicrolf", GetDlgItemInt(hwndDlg, IDC_EXTRAMICROLF, &translated, FALSE));
					if(IsDlgButtonChecked(hwndDlg, IDC_ALWAYSTRIM))
						DBWriteContactSettingDword(NULL, SRMSGMOD_T, "maxhist", (DWORD)SendDlgItemMessage(hwndDlg, IDC_TRIMSPIN, UDM_GETPOS, 0, 0));
					else
						DBWriteContactSettingDword(NULL, SRMSGMOD_T, "maxhist", 0);
					ReloadGlobals();
#ifdef __MATHMOD_SUPPORT    		
					myGlobals.m_MathModAvail = ServiceExists(MATH_RTF_REPLACE_FORMULAE) && DBGetContactSettingByte(NULL, SRMSGMOD_T, "wantmathmod", 0);
#endif                            
					WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 1, 0);
					return TRUE;
				}
			}
			break;
		}
		break;
	}
	return FALSE;
}

static ResetCList(HWND hwndDlg)
{
    int i;

    if (CallService(MS_CLUI_GETCAPS, 0, 0) & CLUIF_DISABLEGROUPS && !DBGetContactSettingByte(NULL, "CList", "UseGroups", SETTING_USEGROUPS_DEFAULT))
        SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETUSEGROUPS, (WPARAM) FALSE, 0);
    else
        SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETUSEGROUPS, (WPARAM) TRUE, 0);
    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETHIDEEMPTYGROUPS, 1, 0);
    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETGREYOUTFLAGS, 0, 0);
    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETLEFTMARGIN, 2, 0);
    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETBKBITMAP, 0, (LPARAM) (HBITMAP) NULL);
    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETBKCOLOR, GetSysColor(COLOR_WINDOW), 0);
    SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETINDENT, 10, 0);
    for (i = 0; i <= FONTID_MAX; i++)
        SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETTEXTCOLOR, i, GetSysColor(COLOR_WINDOWTEXT));
}

static void RebuildList(HWND hwndDlg, HANDLE hItemNew, HANDLE hItemUnknown)
{
    HANDLE hContact, hItem;
    BYTE defType = DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW);

    if (hItemNew && defType) {
        SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM) hItemNew, 1);
    }
    if (hItemUnknown && DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGUNKNOWN, SRMSGDEFSET_TYPINGUNKNOWN)) {
        SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM) hItemUnknown, 1);
    }
    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    do {
        hItem = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM) hContact, 0);
        if (hItem && DBGetContactSettingByte(hContact, SRMSGMOD, SRMSGSET_TYPING, defType)) {
            SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM) hItem, 1);
        }
    } while (hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0));
}

static void SaveList(HWND hwndDlg, HANDLE hItemNew, HANDLE hItemUnknown)
{
    HANDLE hContact, hItem;

    if (hItemNew) {
        DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGNEW, SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM) hItemNew, 0) ? 1 : 0);
    }
    if (hItemUnknown) {
        DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_TYPINGUNKNOWN, SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM) hItemUnknown, 0) ? 1 : 0);
    }
    hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
    do {
        hItem = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM) hContact, 0);
        if (hItem) {
            DBWriteContactSettingByte(hContact, SRMSGMOD, SRMSGSET_TYPING, SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM) hItem, 0) ? 1 : 0);
        }
    } while (hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0));
}

static BOOL CALLBACK DlgProcTypeOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HANDLE hItemNew, hItemUnknown;

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		{
			CLCINFOITEM cii = { 0 };
			cii.cbSize = sizeof(cii);
			cii.flags = CLCIIF_GROUPFONT | CLCIIF_CHECKBOX;
			cii.pszText = TranslateT("** New contacts **");
			hItemNew = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_ADDINFOITEM, 0, (LPARAM) & cii);
			cii.pszText = TranslateT("** Unknown contacts **");
			hItemUnknown = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_ADDINFOITEM, 0, (LPARAM) & cii);
		}
		SetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_STYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_STYLE) | (CLS_SHOWHIDDEN));
		ResetCList(hwndDlg);
		RebuildList(hwndDlg, hItemNew, hItemUnknown);
		CheckDlgButton(hwndDlg, IDC_SHOWNOTIFY, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPING, SRMSGDEFSET_SHOWTYPING));
		CheckDlgButton(hwndDlg, IDC_TYPEWIN, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGWIN, SRMSGDEFSET_SHOWTYPINGWIN));
		CheckDlgButton(hwndDlg, IDC_TYPETRAY, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGNOWIN, SRMSGDEFSET_SHOWTYPINGNOWIN));
		CheckDlgButton(hwndDlg, IDC_NOTIFYTRAY, DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGCLIST, SRMSGDEFSET_SHOWTYPINGCLIST));
		CheckDlgButton(hwndDlg, IDC_NOTIFYBALLOON, !DBGetContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGCLIST, SRMSGDEFSET_SHOWTYPINGCLIST));
		EnableWindow(GetDlgItem(hwndDlg, IDC_TYPEWIN), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
		EnableWindow(GetDlgItem(hwndDlg, IDC_TYPETRAY), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
		EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYTRAY), IsDlgButtonChecked(hwndDlg, IDC_TYPETRAY));
		EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), IsDlgButtonChecked(hwndDlg, IDC_TYPETRAY));
		if (!ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
			EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), FALSE);
			CheckDlgButton(hwndDlg, IDC_NOTIFYTRAY, BST_CHECKED);
			SetWindowTextA(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), Translate("Show balloon popup (unsupported system)"));
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_TYPETRAY:
			if (IsDlgButtonChecked(hwndDlg, IDC_TYPETRAY)) {
				if (!ServiceExists(MS_CLIST_SYSTRAY_NOTIFY))
					EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYTRAY), FALSE);
				else {
					EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYTRAY), TRUE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), TRUE);
				}
			}
			else {
				EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYTRAY), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), FALSE);
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case IDC_SHOWNOTIFY:
			EnableWindow(GetDlgItem(hwndDlg, IDC_TYPEWIN), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
			EnableWindow(GetDlgItem(hwndDlg, IDC_TYPETRAY), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
			EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYTRAY), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
			EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY)&&ServiceExists(MS_CLIST_SYSTRAY_NOTIFY));
			//fall-thru
		case IDC_TYPEWIN:
		case IDC_NOTIFYTRAY:
		case IDC_NOTIFYBALLOON:
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
		break;
	case WM_NOTIFY:
		switch (((NMHDR *) lParam)->idFrom) {
		case IDC_CLIST:
			switch (((NMHDR *) lParam)->code) {
			case CLN_OPTIONSCHANGED:
				ResetCList(hwndDlg);
				break;
			case CLN_CHECKCHANGED:
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			}
			break;
		case 0:
			switch (((LPNMHDR) lParam)->code) {
			case PSN_APPLY:
				{
					SaveList(hwndDlg, hItemNew, hItemUnknown);
					DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPING, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
					DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGWIN, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_TYPEWIN));
					DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGNOWIN, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_TYPETRAY));
					DBWriteContactSettingByte(NULL, SRMSGMOD, SRMSGSET_SHOWTYPINGCLIST, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_NOTIFYTRAY));
					ReloadGlobals();
					WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 0, 0);
				}
			}
			break;
		}
		break;
	}
	return FALSE;
}

/*
 * options for tabbed messaging got their own page.. finally :)
 */

static struct LISTOPTIONSGROUP tabGroups[] = {
    0, _T("Tab options"),
    0, _T("Message tab and window creation options"),
    0, _T("Message dialog visual settings"),
    0, _T("Miscellaneous options"),
    0, NULL
};

static struct LISTOPTIONSITEM tabItems[] = {
    0, _T("Show status text on tabs"), 0, LOI_TYPE_SETTING, (UINT_PTR)"tabstatus", 0,
    0, _T("Prefer xStatus icons when available"), 0, LOI_TYPE_SETTING, (UINT_PTR)"use_xicons", 0,
    0, _T("Warn when closing a tab or window"), 0, LOI_TYPE_SETTING, (UINT_PTR)"warnonexit", 0,
    0, _T("ALWAYS pop up and activate new message windows (has PRIORITY!)"), SRMSGDEFSET_AUTOPOPUP, LOI_TYPE_SETTING, (UINT_PTR)SRMSGSET_AUTOPOPUP, 1,
    0, _T("Create new windows and tabs in the background"), 0, LOI_TYPE_SETTING, (UINT_PTR)"autotabs", 1,
    0, _T("Create windows minimized and inactive on the taskbar"), 0, LOI_TYPE_SETTING, (UINT_PTR)"autocontainer", 1,
    0, _T("Pop up a minimized window when a new tab is created"), 0, LOI_TYPE_SETTING, (UINT_PTR)"cpopup", 1,
    0, _T("New events will automatically switch tabs in minimized windows"), 0, LOI_TYPE_SETTING, (UINT_PTR)"autoswitchtabs", 1,
    0, _T("Don't draw visual styles on toolbar buttons"), 1, LOI_TYPE_SETTING, (UINT_PTR)"nlflat", 2,
    0, _T("Flat toolbar buttons"), 1, LOI_TYPE_SETTING, (UINT_PTR)"tbflat", 2,
    0, _T("Splitters have static edges (uncheck this to make them invisible)"), 1, LOI_TYPE_SETTING, (UINT_PTR)"splitteredges", 2,
    0, _T("No visible borders on text boxes"), 1, LOI_TYPE_SETTING, (UINT_PTR)"flatlog", 2,
    0, _T("Always use icon pack image on the smiley button"), 0, LOI_TYPE_SETTING, (UINT_PTR)"smbutton_override", 2,
    0, _T("Activate autolocale support"), 0, LOI_TYPE_SETTING, (UINT_PTR)"al", 3,
    0, _T("ESC closes sessions (minimizes window, if disabled)"), 0, LOI_TYPE_SETTING, (UINT_PTR)"escmode", 3,
    0, _T("Use global hotkeys (configure modifiers below)"), 0, LOI_TYPE_SETTING, (UINT_PTR)"globalhotkeys", 3,
    0, _T("Force more aggressive window updates"), 1, LOI_TYPE_SETTING, (UINT_PTR)"aggromode", 3,
    0, _T("Dim icons for idle contacts"), 1, LOI_TYPE_SETTING, (UINT_PTR)"detectidle", 2,
    0, NULL, 0, 0, 0, 0
};

static BOOL CALLBACK DlgProcTabbedOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL translated;
    
	switch (msg) {
	case WM_INITDIALOG:
		{
			TVINSERTSTRUCT tvi = {0};
			int i = 0;

			TranslateDialogDefault(hwndDlg);
			SetWindowLong(GetDlgItem(hwndDlg, IDC_TABMSGOPTIONS), GWL_STYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_TABMSGOPTIONS), GWL_STYLE) | (TVS_NOHSCROLL | TVS_CHECKBOXES));
			g_himlOptions = (HIMAGELIST)SendDlgItemMessage(hwndDlg, IDC_TABMSGOPTIONS, TVM_SETIMAGELIST, TVSIL_STATE, (LPARAM)CreateStateImageList());
            if(g_himlOptions)
                ImageList_Destroy(g_himlOptions);
			/*
			* fill the list box, create groups first, then add items
			*/

			while(tabGroups[i].szName != NULL) {
				tvi.hParent = 0;
				tvi.hInsertAfter = TVI_LAST;
				tvi.item.mask = TVIF_TEXT | TVIF_STATE;
				tvi.item.pszText = TranslateTS(tabGroups[i].szName);
				tvi.item.stateMask = TVIS_STATEIMAGEMASK | TVIS_EXPANDED | TVIS_BOLD;
				tvi.item.state = INDEXTOSTATEIMAGEMASK(0) | TVIS_EXPANDED | TVIS_BOLD;
				tabGroups[i++].handle = (LRESULT)TreeView_InsertItem( GetDlgItem(hwndDlg, IDC_TABMSGOPTIONS), &tvi);
			}

			i = 0;

			while(tabItems[i].szName != 0) {
				tvi.hParent = (HTREEITEM)tabGroups[tabItems[i].uGroup].handle;
				tvi.hInsertAfter = TVI_LAST;
				tvi.item.pszText = TranslateTS(tabItems[i].szName);
				tvi.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
				tvi.item.lParam = i;
				tvi.item.stateMask = TVIS_STATEIMAGEMASK;
				if(tabItems[i].uType == LOI_TYPE_SETTING)
					tvi.item.state = INDEXTOSTATEIMAGEMASK(DBGetContactSettingByte(NULL, SRMSGMOD_T, (char *)tabItems[i].lParam, (BYTE)tabItems[i].id) ? 3 : 2);
				tabItems[i].handle = (LRESULT)TreeView_InsertItem( GetDlgItem(hwndDlg, IDC_TABMSGOPTIONS), &tvi);
				i++;
			}
			// XXX tab support options...
			CheckDlgButton(hwndDlg, IDC_CUT_TABTITLE, DBGetContactSettingByte(NULL, SRMSGMOD_T, "cuttitle", 0));
			SetDlgItemInt(hwndDlg, IDC_CUT_TITLEMAX, DBGetContactSettingWord(NULL, SRMSGMOD_T, "cut_at", 15), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_CUT_TITLEMAX), IsDlgButtonChecked(hwndDlg, IDC_CUT_TABTITLE));

			SendDlgItemMessage(hwndDlg, IDC_HISTORYSIZESPIN, UDM_SETRANGE, 0, MAKELONG(255, 10));
			SendDlgItemMessage(hwndDlg, IDC_HISTORYSIZESPIN, UDM_SETPOS, 0, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "historysize", 0));
			SetDlgItemInt(hwndDlg, IDC_HISTORYSIZE, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "historysize", 10), FALSE);

			SendDlgItemMessage(hwndDlg, IDC_MODIFIERS, CB_INSERTSTRING, -1, (LPARAM)_T("CTRL-SHIFT"));
			SendDlgItemMessage(hwndDlg, IDC_MODIFIERS, CB_INSERTSTRING, -1, (LPARAM)_T("CTRL-ALT"));
			SendDlgItemMessage(hwndDlg, IDC_MODIFIERS, CB_INSERTSTRING, -1, (LPARAM)_T("ALT-SHIFT"));
			SendDlgItemMessage(hwndDlg, IDC_MODIFIERS, CB_SETCURSEL, (WPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "hotkeymodifier", 0), 0);

			SendDlgItemMessage(hwndDlg, IDC_IPFIELDBORDERS, CB_INSERTSTRING, -1, (LPARAM)TranslateT("3D - Sunken"));
			SendDlgItemMessage(hwndDlg, IDC_IPFIELDBORDERS, CB_INSERTSTRING, -1, (LPARAM)TranslateT("3D - Raised inner"));
			SendDlgItemMessage(hwndDlg, IDC_IPFIELDBORDERS, CB_INSERTSTRING, -1, (LPARAM)TranslateT("3D - Raised outer"));
			SendDlgItemMessage(hwndDlg, IDC_IPFIELDBORDERS, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Edged"));
			SendDlgItemMessage(hwndDlg, IDC_IPFIELDBORDERS, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Flat (no border at all)"));
			SendDlgItemMessage(hwndDlg, IDC_IPFIELDBORDERS, CB_SETCURSEL, (WPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "ipfieldborder", IPFIELD_SUNKEN), 0);

			SendDlgItemMessage(hwndDlg, IDC_TOOLBARHIDEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Hide formatting buttons first"));
			SendDlgItemMessage(hwndDlg, IDC_TOOLBARHIDEMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Hide standard buttons first"));
			SendDlgItemMessage(hwndDlg, IDC_TOOLBARHIDEMODE, CB_SETCURSEL, (WPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "tbarhidemode", 0), 0);
			break;
		}
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDC_HISTORYSIZE:
		case IDC_CUT_TITLEMAX:
			if (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus())
				return TRUE;
			break;
		case IDC_SETUPAUTOCREATEMODES:
			{
				HWND hwndNew = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_CHOOSESTATUSMODES), hwndDlg, DlgProcSetupStatusModes, DBGetContactSettingDword(0, SRMSGMOD_T, "autopopupmask", -1));
				SendMessage(hwndNew, DM_SETPARENTDIALOG, 0, (LPARAM)hwndDlg);
				break;
			}
		case IDC_CUT_TABTITLE:
			EnableWindow(GetDlgItem(hwndDlg, IDC_CUT_TITLEMAX), IsDlgButtonChecked(hwndDlg, IDC_CUT_TABTITLE));
			break;
		}
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		break;
	case DM_STATUSMASKSET:
		DBWriteContactSettingDword(0, SRMSGMOD_T, "autopopupmask", (DWORD)lParam);
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR) lParam)->idFrom) {
		case IDC_TABMSGOPTIONS:
			if(((LPNMHDR)lParam)->code==NM_CLICK) {
				TVHITTESTINFO hti;
				TVITEM item = {0};

				item.mask = TVIF_HANDLE | TVIF_STATE;
				item.stateMask = TVIS_STATEIMAGEMASK | TVIS_BOLD;
				hti.pt.x=(short)LOWORD(GetMessagePos());
				hti.pt.y=(short)HIWORD(GetMessagePos());
				ScreenToClient(((LPNMHDR)lParam)->hwndFrom, &hti.pt);
				if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom, &hti)) {
					item.hItem = (HTREEITEM)hti.hItem;
					SendDlgItemMessageA(hwndDlg, IDC_TABMSGOPTIONS, TVM_GETITEMA, 0, (LPARAM)&item);
					if(item.state & TVIS_BOLD && hti.flags & TVHT_ONITEMSTATEICON) {
						item.state = INDEXTOSTATEIMAGEMASK(0) | TVIS_BOLD;
						SendDlgItemMessageA(hwndDlg, IDC_TABMSGOPTIONS, TVM_SETITEMA, 0, (LPARAM)&item);
					}
					else if(hti.flags&TVHT_ONITEMSTATEICON) {
						if(((item.state & TVIS_STATEIMAGEMASK) >> 12) == 3) {
							item.state = INDEXTOSTATEIMAGEMASK(1);
							SendDlgItemMessageA(hwndDlg, IDC_TABMSGOPTIONS, TVM_SETITEMA, 0, (LPARAM)&item);
						}
						SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					}
				}
			}
			break;
		case 0:
			switch (((LPNMHDR) lParam)->code) {
			case PSN_APPLY:
				{
					TVITEM item = {0};
					int i = 0;
					DBWriteContactSettingByte(NULL, SRMSGMOD_T, "cuttitle", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_CUT_TABTITLE));
					DBWriteContactSettingWord(NULL, SRMSGMOD_T, "cut_at", (WORD) GetDlgItemInt(hwndDlg, IDC_CUT_TITLEMAX, &translated, FALSE));
					DBWriteContactSettingByte(NULL, SRMSGMOD_T, "historysize", (BYTE) GetDlgItemInt(hwndDlg, IDC_HISTORYSIZE, &translated, FALSE));
					DBWriteContactSettingByte(NULL, SRMSGMOD_T, "hotkeymodifier", (BYTE) SendDlgItemMessage(hwndDlg, IDC_MODIFIERS, CB_GETCURSEL, 0, 0));
					DBWriteContactSettingByte(NULL, SRMSGMOD_T, "ipfieldborder", (BYTE) SendDlgItemMessage(hwndDlg, IDC_IPFIELDBORDERS, CB_GETCURSEL, 0, 0));
					DBWriteContactSettingByte(NULL, SRMSGMOD_T, "tbarhidemode", (BYTE)SendDlgItemMessage(hwndDlg, IDC_TOOLBARHIDEMODE, CB_GETCURSEL, 0, 0));

					/*
					* scan the tree view and obtain the options...
					*/
					while(tabItems[i].szName != NULL) {
						item.mask = TVIF_HANDLE | TVIF_STATE;
						item.hItem = (HTREEITEM)tabItems[i].handle;
						item.stateMask = TVIS_STATEIMAGEMASK;

						SendDlgItemMessageA(hwndDlg, IDC_TABMSGOPTIONS, TVM_GETITEMA, 0, (LPARAM)&item);
						if(tabItems[i].uType == LOI_TYPE_SETTING)
							DBWriteContactSettingByte(NULL, SRMSGMOD_T, (char *)tabItems[i].lParam, (item.state >> 12) == 3 ? 1 : 0);
						i++;
					}

					ReloadGlobals();
					WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 0, 0);
					SendMessage(myGlobals.g_hwndHotkeyHandler, DM_FORCEUNREGISTERHOTKEYS, 0, 0);
					SendMessage(myGlobals.g_hwndHotkeyHandler, DM_REGISTERHOTKEYS, 0, 0);
					return TRUE;
				}
			}
		}
		break;
	}
	return FALSE;
}

static BOOL CALLBACK DlgProcContainerSettings(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
        case WM_INITDIALOG:
        {
			DBVARIANT dbv = {0};

			TranslateDialogDefault(hwndDlg);

            CheckDlgButton(hwndDlg, IDC_CONTAINERGROUPMODE, DBGetContactSettingByte(NULL, SRMSGMOD_T, "useclistgroups", 0));
            CheckDlgButton(hwndDlg, IDC_LIMITTABS, DBGetContactSettingByte(NULL, SRMSGMOD_T, "limittabs", 0));

            SendDlgItemMessage(hwndDlg, IDC_TABLIMITSPIN, UDM_SETRANGE, 0, MAKELONG(1000, 1));
            SendDlgItemMessage(hwndDlg, IDC_TABLIMITSPIN, UDM_SETPOS, 0, (int)DBGetContactSettingDword(NULL, SRMSGMOD_T, "maxtabs", 1));
            SetDlgItemInt(hwndDlg, IDC_TABLIMIT, (int)DBGetContactSettingDword(NULL, SRMSGMOD_T, "maxtabs", 1), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_TABLIMIT), IsDlgButtonChecked(hwndDlg, IDC_LIMITTABS));
            CheckDlgButton(hwndDlg, IDC_SINGLEWINDOWMODE, DBGetContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", 0));
            CheckDlgButton(hwndDlg, IDC_DEFAULTCONTAINERMODE, !(IsDlgButtonChecked(hwndDlg, IDC_CONTAINERGROUPMODE) || IsDlgButtonChecked(hwndDlg, IDC_LIMITTABS) || IsDlgButtonChecked(hwndDlg, IDC_SINGLEWINDOWMODE)));

            SetDlgItemInt(hwndDlg, IDC_NRFLASH, DBGetContactSettingByte(NULL, SRMSGMOD_T, "nrflash", 4), FALSE);
            SendDlgItemMessage(hwndDlg, IDC_NRFLASHSPIN, UDM_SETRANGE, 0, MAKELONG(255, 1));
            SendDlgItemMessage(hwndDlg, IDC_NRFLASHSPIN, UDM_SETPOS, 0, (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "nrflash", 4));

            SetDlgItemInt(hwndDlg, IDC_FLASHINTERVAL, DBGetContactSettingDword(NULL, SRMSGMOD_T, "flashinterval", 1000), FALSE);
            SendDlgItemMessage(hwndDlg, IDC_FLASHINTERVALSPIN, UDM_SETRANGE, 0, MAKELONG(10000, 500));
            SendDlgItemMessage(hwndDlg, IDC_FLASHINTERVALSPIN, UDM_SETPOS, 0, (int)DBGetContactSettingDword(NULL, SRMSGMOD_T, "flashinterval", 1000));
            SendDlgItemMessage(hwndDlg, IDC_FLASHINTERVALSPIN, UDM_SETACCEL, 0, (int)DBGetContactSettingDword(NULL, SRMSGMOD_T, "flashinterval", 1000));
            SetDlgItemText(hwndDlg, IDC_DEFAULTTITLEFORMAT, myGlobals.szDefaultTitleFormat);
            SendDlgItemMessage(hwndDlg, IDC_DEFAULTTITLEFORMAT, EM_LIMITTEXT, TITLE_FORMATLEN - 1, 0);
			CheckDlgButton(hwndDlg, IDC_USESKIN, DBGetContactSettingByte(NULL, SRMSGMOD_T, "useskin", 0) ? 1 : 0);
			SendMessage(hwndDlg, WM_COMMAND, MAKELONG(IDC_USESKIN, BN_CLICKED), 0);
			EnableWindow(GetDlgItem(hwndDlg, IDC_USESKIN), IsWinVer2000Plus() ? TRUE : FALSE);
			CheckDlgButton(hwndDlg, IDC_DROPSHADOW, myGlobals.m_dropShadow);

            if(!ServiceExists("Utils/SnapWindowProc"))
                EnableWindow(GetDlgItem(hwndDlg, IDC_USESNAPPING), FALSE);
            else
                CheckDlgButton(hwndDlg, IDC_USESNAPPING, DBGetContactSettingByte(NULL, SRMSGMOD_T, "usesnapping", 0));

		 return TRUE;
		}
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDC_TABLIMIT:
                    if (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus())
                        return TRUE;
                    break;
                case IDC_LIMITTABS:
                case IDC_SINGLEWINDOWMODE:
                case IDC_CONTAINERGROUPMODE:
                case IDC_DEFAULTCONTAINERMODE:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_TABLIMIT), IsDlgButtonChecked(hwndDlg, IDC_LIMITTABS));
                    break;
            }
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            break;
        case WM_NOTIFY:
            switch (((LPNMHDR) lParam)->idFrom) {
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
						case PSN_APPLY: {
                            BOOL translated;
                            TCHAR szDefaultName[TITLE_FORMATLEN + 2];

                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "useclistgroups", IsDlgButtonChecked(hwndDlg, IDC_CONTAINERGROUPMODE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "limittabs", IsDlgButtonChecked(hwndDlg, IDC_LIMITTABS));
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "maxtabs", GetDlgItemInt(hwndDlg, IDC_TABLIMIT, &translated, FALSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "singlewinmode", IsDlgButtonChecked(hwndDlg, IDC_SINGLEWINDOWMODE));
                            DBWriteContactSettingDword(NULL, SRMSGMOD_T, "flashinterval", GetDlgItemInt(hwndDlg, IDC_FLASHINTERVAL, &translated, FALSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "nrflash", GetDlgItemInt(hwndDlg, IDC_NRFLASH, &translated, FALSE));
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "usesnapping", IsDlgButtonChecked(hwndDlg, IDC_USESNAPPING));
							DBWriteContactSettingByte(NULL, SRMSGMOD_T, "dropshadow", IsDlgButtonChecked(hwndDlg, IDC_DROPSHADOW) ? 1 : 0);
							myGlobals.m_dropShadow = IsDlgButtonChecked(hwndDlg, IDC_DROPSHADOW) ? 1 : 0;

                            GetDlgItemText(hwndDlg, IDC_DEFAULTTITLEFORMAT, szDefaultName, TITLE_FORMATLEN);
#if defined(_UNICODE)
                            DBWriteContactSettingWString(NULL, SRMSGMOD_T, "titleformatW", szDefaultName);
#else                            
                            DBWriteContactSettingString(NULL, SRMSGMOD_T, "titleformat", szDefaultName);
#endif
							GetDefaultContainerTitleFormat();
                            myGlobals.g_wantSnapping = ServiceExists("Utils/SnapWindowProc") && IsDlgButtonChecked(hwndDlg, IDC_USESNAPPING);
                            BuildContainerMenu();
                            return TRUE;
                        }
                    }
            }
            break;
    }
    return FALSE;
}

#define DBFONTF_BOLD       1
#define DBFONTF_ITALIC     2
#define DBFONTF_UNDERLINE  4

#define FONTS_TO_CONFIG MSGDLGFONTCOUNT

#define SAMEASF_FACE   1
#define SAMEASF_SIZE   2
#define SAMEASF_STYLE  4
#define SAMEASF_COLOUR 8
#include <pshpack1.h>

struct {
	BYTE sameAsFlags,sameAs;
	COLORREF colour;
	char size;
	BYTE style;
	BYTE charset;
	char szFace[LF_FACESIZE];
} static fontSettings[MSGDLGFONTCOUNT + 1];

#include <poppack.h>

#define SRFONTSETTINGMODULE FONTMODULE

static int OptInitialise(WPARAM wParam, LPARAM lParam)
{
    OPTIONSDIALOGPAGE odp = { 0 };

	Chat_OptionsInitialize(wParam, lParam);

    odp.cbSize = sizeof(odp);
    odp.position = 910000000;
    odp.hInstance = g_hInst;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONSDIALOG);
    odp.pszTitle = Translate("Message Window");
    odp.pfnDlgProc = OptionsDlgProc;
    odp.pszGroup = Translate("Message Sessions");
    odp.nIDBottomSimpleControl = 0;
    odp.flags = ODPF_BOLDGROUPS;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp);
    
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_MSGTYPE);
    odp.pszTitle = Translate("Typing Notify");
    odp.pfnDlgProc = DlgProcTypeOptions;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp);

    odp.pszTemplate = MAKEINTRESOURCEA(IDD_POPUP_OPT);
    odp.pszTitle = Translate("Event notifications");
    odp.pfnDlgProc = DlgProcPopupOpts;
    odp.nIDBottomSimpleControl = 0;
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

    odp.pszTemplate = MAKEINTRESOURCEA(IDD_SKINTABDIALOG);
    odp.pszTitle = Translate("Message window skin");
    odp.pfnDlgProc = SkinOptionsDlgProc;
    odp.nIDBottomSimpleControl = 0;
    odp.pszGroup = Translate("Customize");
    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);

    return 0;
}

int InitOptions(void)
{
    HookEvent(ME_OPT_INITIALISE, OptInitialise);
    return 0;
}

BOOL CALLBACK DlgProcSetupStatusModes(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD dwStatusMask = GetWindowLong(hwndDlg, GWL_USERDATA);
    static DWORD dwNewStatusMask = 0;
    static HWND hwndParent = 0;
    
    switch (msg) {
        case WM_INITDIALOG:
        {
            int i;
            
            TranslateDialogDefault(hwndDlg);
            SetWindowLong(hwndDlg, GWL_USERDATA, lParam);
            dwStatusMask = lParam;
            
            SetWindowText(hwndDlg, TranslateT("Choose status modes"));
            for(i = ID_STATUS_ONLINE; i <= ID_STATUS_OUTTOLUNCH; i++) {
                SetWindowText(GetDlgItem(hwndDlg, i), (TCHAR *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)i, GCMDF_TCHAR));
                if(dwStatusMask != -1 && (dwStatusMask & (1<<(i - ID_STATUS_ONLINE))))
                    CheckDlgButton(hwndDlg, i, TRUE);
                EnableWindow(GetDlgItem(hwndDlg, i), dwStatusMask != -1);
            }
            if(dwStatusMask == -1)
                CheckDlgButton(hwndDlg, IDC_ALWAYS, TRUE);
            ShowWindow(hwndDlg, SW_SHOWNORMAL);
            return TRUE;
        }
        case DM_SETPARENTDIALOG:
            hwndParent = (HWND)lParam;
            break;
        case DM_GETSTATUSMASK:
            {
                if(IsDlgButtonChecked(hwndDlg, IDC_ALWAYS))
                    dwNewStatusMask = -1;
                else {
                    int i;
                    dwNewStatusMask = 0;
                    for(i = ID_STATUS_ONLINE; i <= ID_STATUS_OUTTOLUNCH; i++)
                        dwNewStatusMask |= (IsDlgButtonChecked(hwndDlg, i) ? (1<<(i - ID_STATUS_ONLINE)) : 0);
                }
                break;
            }
        case WM_COMMAND:
            {
                switch (LOWORD(wParam)) {
                    case IDOK:
                    case IDCANCEL:
                        if(LOWORD(wParam) == IDOK) {
                            SendMessage(hwndDlg, DM_GETSTATUSMASK, 0, 0);
                            SendMessage(hwndParent, DM_STATUSMASKSET, 0, (LPARAM)dwNewStatusMask);
                        }
                        DestroyWindow(hwndDlg);
                        break;
                    case IDC_ALWAYS:
                        {
                        int i;
                        for(i = ID_STATUS_ONLINE; i <= ID_STATUS_OUTTOLUNCH; i++)
                            EnableWindow(GetDlgItem(hwndDlg, i), !IsDlgButtonChecked(hwndDlg, IDC_ALWAYS));
                        break;
                        }
                    default:
                        break;
                }
            }
        case WM_DESTROY:
            SetWindowLong(hwndDlg, GWL_USERDATA, 0);
            break;
        default:
            break;
    }
    return FALSE;
}

/*
 * tabbed options dialog
 */

static BOOL CALLBACK OptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static int iInit = TRUE;
   
   switch(msg)
   {
      case WM_INITDIALOG:
      {
         TCITEM tci;
         RECT rcClient;
         int oPage = DBGetContactSettingByte(NULL, SRMSGMOD_T, "opage", 0);

         GetClientRect(hwnd, &rcClient);
         iInit = TRUE;
         tci.mask = TCIF_PARAM|TCIF_TEXT;
         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_MSGDLG), hwnd, DlgProcOptions);
         tci.pszText = TranslateT("General");
			TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 0, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 0 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_TABBEDMSG),hwnd,DlgProcTabbedOptions);
         tci.pszText = TranslateT("Tabs and layout");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 2, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 1 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_CONTAINERS),hwnd,DlgProcContainerSettings);
         tci.pszText = TranslateT("Containers");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 3, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 2 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_MSGLOG),hwnd,DlgProcLogOptions);
         tci.pszText = TranslateT("Message log");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 4, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 3 ? SW_SHOW : SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);
         
         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPTIONS1),hwnd, DlgProcOptions1);
         tci.pszText = TranslateT("Group chats");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 4, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-30,1);
         ShowWindow((HWND)tci.lParam, oPage == 4 ? SW_SHOW : SW_HIDE);
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
                        DBWriteContactSettingByte(NULL, SRMSGMOD_T, "opage", TabCtrl_GetCurSel(GetDlgItem(hwnd, IDC_OPTIONSTAB)));
                     }
                  break;
               }
            break;

         }
      break;
   }
   return FALSE;
}

static HWND hwndTabConfig = 0;

static BOOL CALLBACK DlgProcSkinOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
        {
            DBVARIANT dbv;
            BYTE loadMode = DBGetContactSettingByte(NULL, SRMSGMOD_T, "skin_loadmode", 0);
            TranslateDialogDefault(hwndDlg);

            //SendDlgItemMessage(hwndDlg, IDC_CORNERSPIN, UDM_SETRANGE, 0, MAKELONG(10, 0));
            //SendDlgItemMessage(hwndDlg, IDC_CORNERSPIN, UDM_SETPOS, 0, g_CluiData.cornerRadius);

            CheckDlgButton(hwndDlg, IDC_USESKIN, DBGetContactSettingByte(NULL, SRMSGMOD_T, "useskin", 0) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_SKIN_LOADFONTS, loadMode & THEME_READ_FONTS);
            CheckDlgButton(hwndDlg, IDC_SKIN_LOADTEMPLATES, loadMode & THEME_READ_TEMPLATES);
            CheckDlgButton(hwndDlg, IDC_SKIN_REFRESH, DBGetContactSettingByte(NULL, SRMSGMOD_T, "forceDCrefresh", 0));

            if(!DBGetContactSetting(NULL, SRMSGMOD_T, "ContainerSkin", &dbv)) {
                SetDlgItemTextA(hwndDlg, IDC_SKINFILENAME, dbv.pszVal);
                DBFreeVariant(&dbv);
            }
            else
                SetDlgItemText(hwndDlg, IDC_SKINFILENAME, _T(""));
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_USESKIN:
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "useskin", IsDlgButtonChecked(hwndDlg, IDC_USESKIN) ? 1 : 0);
                    break;
                case IDC_SKIN_LOADFONTS:
                {
                    BYTE loadMode = DBGetContactSettingByte(NULL, SRMSGMOD_T, "skin_loadmode", 0);
                    loadMode = IsDlgButtonChecked(hwndDlg, IDC_SKIN_LOADFONTS) ? loadMode | THEME_READ_FONTS : loadMode & ~THEME_READ_FONTS;
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "skin_loadmode", loadMode);
                    break;
                }
                case IDC_SKIN_LOADTEMPLATES:
                {
                    BYTE loadMode = DBGetContactSettingByte(NULL, SRMSGMOD_T, "skin_loadmode", 0);
                    loadMode = IsDlgButtonChecked(hwndDlg, IDC_SKIN_LOADTEMPLATES) ? loadMode | THEME_READ_TEMPLATES : loadMode & ~THEME_READ_TEMPLATES;
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "skin_loadmode", loadMode);
                    break;
                }
                case IDC_SKIN_REFRESH:
                    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "forceDCrefresh", IsDlgButtonChecked(hwndDlg, IDC_SKIN_REFRESH) ? 1 : 0);
                    break;
                case IDC_UNLOAD:
                    ReloadContainerSkin(0, 0);
                    SendMessage(hwndTabConfig, WM_USER + 100, 0, 0);
                    break;
                case IDC_SELECTSKINFILE:
                    {
                        OPENFILENAMEA ofn = {0};
                        char str[MAX_PATH] = "*.tsk", final_path[MAX_PATH], initDir[MAX_PATH];

                        mir_snprintf(initDir, MAX_PATH, "%s%s", myGlobals.szDataPath, "skins\\");
                        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
                        ofn.hwndOwner = hwndDlg;
                        ofn.hInstance = NULL;
                        ofn.lpstrFilter = "*.tsk";
                        ofn.lpstrFile = str;
                        ofn.lpstrInitialDir = initDir;
                        ofn.Flags = OFN_FILEMUSTEXIST;
                        ofn.nMaxFile = sizeof(str);
                        ofn.nMaxFileTitle = MAX_PATH;
                        ofn.lpstrDefExt = "";
                        if (!GetOpenFileNameA(&ofn))
                            break;
                        CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)str, (LPARAM)final_path);
                        if(PathFileExistsA(str)) {
                            int skinChanged = 0;
                            DBVARIANT dbv = {0};

                            if(!DBGetContactSetting(NULL, SRMSGMOD_T, "ContainerSkin", &dbv)) {
                                if(strcmp(dbv.pszVal, final_path))
                                    skinChanged = TRUE;
                                DBFreeVariant(&dbv);
                            }
                            else
                                skinChanged = TRUE;

                            DBWriteContactSettingString(NULL, SRMSGMOD_T, "ContainerSkin", final_path);
                            DBWriteContactSettingByte(NULL, SRMSGMOD_T, "skin_changed", skinChanged);
                            SetDlgItemTextA(hwndDlg, IDC_SKINFILENAME, final_path);
                        }
                        break;
                    }
                case IDC_RELOADSKIN:
                    ReloadContainerSkin(1, 0);
                    SendMessage(hwndTabConfig, WM_USER + 100, 0, 0);
                    break;
            }
            if ((LOWORD(wParam) == IDC_SKINFILE || LOWORD(wParam) == IDC_SKINFILENAME) 
                && (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus()))
                return 0;
            SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
            break;
        case WM_NOTIFY:
            switch (((LPNMHDR) lParam)->idFrom) {
                case 0:
                    switch (((LPNMHDR) lParam)->code) {
                        case PSN_APPLY:
                            return TRUE;
                    }
                    break;
            }
            break;
    }
    return FALSE;
}

static BOOL CALLBACK SkinOptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static int iInit = TRUE;
   static HWND hwndSkinEdit = 0;

   switch(msg)
   {
      case WM_INITDIALOG:
      {
         TCITEM tci;
         RECT rcClient;
         int oPage = DBGetContactSettingByte(NULL, SRMSGMOD_T, "skin_opage", 0);
         SKINDESCRIPTION sd;

         GetClientRect(hwnd, &rcClient);
         iInit = TRUE;
         tci.mask = TCIF_PARAM|TCIF_TEXT;
         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_SKIN), hwnd, DlgProcSkinOpts);
         tci.pszText = TranslateT("Load and apply");
			TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 0, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-60,1);
         ShowWindow((HWND)tci.lParam, SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_TABCONFIG), hwnd, DlgProcTabConfig);
         hwndTabConfig = (HWND)tci.lParam;

         tci.pszText = TranslateT("Tab appearance");
            TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 1, &tci);
         MoveWindow((HWND)tci.lParam,5,25,rcClient.right-9,rcClient.bottom-60,1);
         ShowWindow((HWND)tci.lParam, SW_HIDE);
         if(MyEnableThemeDialogTexture)
             MyEnableThemeDialogTexture((HWND)tci.lParam, ETDT_ENABLETAB);

         if(ServiceExists(MS_CLNSE_INVOKE)) {

             ZeroMemory(&sd, sizeof(sd));
             sd.cbSize = sizeof(sd);
             sd.StatusItems = &StatusItems[0];
             sd.hWndParent = hwnd;
             sd.hWndTab = GetDlgItem(hwnd, IDC_OPTIONSTAB);
             sd.pfnSaveCompleteStruct = 0;
             sd.lastItem = ID_EXTBK_LAST;
             sd.firstItem = 0;
             sd.pfnClcOptionsChanged = 0;
             sd.hwndCLUI = 0;
             hwndSkinEdit = (HWND)CallService(MS_CLNSE_INVOKE, 0, (LPARAM)&sd);
         }

         if(hwndSkinEdit) {
             ShowWindow(hwndSkinEdit, SW_HIDE);
             ShowWindow(sd.hwndImageEdit, SW_HIDE);
             TabCtrl_SetCurSel(GetDlgItem(hwnd, IDC_OPTIONSTAB), oPage);
             if(MyEnableThemeDialogTexture) {
                 MyEnableThemeDialogTexture(hwndSkinEdit, ETDT_ENABLETAB);
                 MyEnableThemeDialogTexture(sd.hwndImageEdit, ETDT_ENABLETAB);
             }
         }
         {
             TCITEM item = {0};
             int iTabs = TabCtrl_GetItemCount(GetDlgItem(hwnd, IDC_OPTIONSTAB));

             if(oPage >= iTabs)
                 oPage = iTabs - 1;

             item.mask = TCIF_PARAM;
             TabCtrl_GetItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), oPage, &item);
             ShowWindow((HWND)item.lParam, SW_SHOW);
             TabCtrl_SetCurSel(GetDlgItem(hwnd, IDC_OPTIONSTAB), oPage);
         }
         //EnableWindow(GetDlgItem(hwnd, IDC_EXPORT), TabCtrl_GetCurSel(GetDlgItem(hwnd, IDC_OPTIONSTAB)) != 0);
         //EnableWindow(GetDlgItem(hwnd, IDC_IMPORT), TabCtrl_GetCurSel(GetDlgItem(hwnd, IDC_OPTIONSTAB)) != 0);
         iInit = FALSE;
         return FALSE;
      }
      
      case PSM_CHANGED: // used so tabs dont have to call SendMessage(GetParent(GetParent(hwnd)), PSM_CHANGED, 0, 0);
         if(!iInit)
             SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
         break;
      case WM_COMMAND:
          switch(LOWORD(wParam)) {
              case IDC_EXPORT:
                  {
                      char str[MAX_PATH] = "*.clist";
                      OPENFILENAMEA ofn = {0};
                      ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
                      ofn.hwndOwner = hwnd;
                      ofn.hInstance = NULL;
                      ofn.lpstrFilter = "*.clist";
                      ofn.lpstrFile = str;
                      ofn.Flags = OFN_HIDEREADONLY;
                      ofn.nMaxFile = sizeof(str);
                      ofn.nMaxFileTitle = MAX_PATH;
                      ofn.lpstrDefExt = "clist";
                      if (!GetSaveFileNameA(&ofn))
                          break;
                      //extbk_export(str);
                      break;
                  }
              case IDC_IMPORT:
                  {
                      char str[MAX_PATH] = "*.clist";
                      OPENFILENAMEA ofn = {0};

                      ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
                      ofn.hwndOwner = hwnd;
                      ofn.hInstance = NULL;
                      ofn.lpstrFilter = "*.clist";
                      ofn.lpstrFile = str;
                      ofn.Flags = OFN_FILEMUSTEXIST;
                      ofn.nMaxFile = sizeof(str);
                      ofn.nMaxFileTitle = MAX_PATH;
                      ofn.lpstrDefExt = "";
                      if (!GetOpenFileNameA(&ofn))
                          break;
                      //extbk_import(str, hwndSkinEdit);
                      SendMessage(hwndSkinEdit, WM_USER + 101, 0, 0);
                      break;
                  }
          }
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
                        DBWriteContactSettingByte(NULL, SRMSGMOD_T, "skin_opage", TabCtrl_GetCurSel(GetDlgItem(hwnd, IDC_OPTIONSTAB)));
                        EnableWindow(GetDlgItem(hwnd, IDC_EXPORT), TabCtrl_GetCurSel(GetDlgItem(hwnd, IDC_OPTIONSTAB)) != 0);
                        EnableWindow(GetDlgItem(hwnd, IDC_IMPORT), TabCtrl_GetCurSel(GetDlgItem(hwnd, IDC_OPTIONSTAB)) != 0);
                     }
                  break;
               }
            break;

         }
      break;
      case WM_DESTROY:
          hwndSkinEdit = 0;
          break;
   }
   return FALSE;
}

/*
 * reload options which may change during M is running and put them in our global option
 * struct to minimize the number of DB reads...
 */

void ReloadGlobals()
{
     //myGlobals.m_SmileyPluginEnabled = (int)DBGetContactSettingByte(NULL, "SmileyAdd", "PluginSupportEnabled", 0);
     myGlobals.m_SendOnShiftEnter = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "sendonshiftenter", 1);
     myGlobals.m_SendOnEnter = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, SRMSGSET_SENDONENTER, SRMSGDEFSET_SENDONENTER);
     myGlobals.m_SendOnDblEnter = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "SendOnDblEnter", 0);
     myGlobals.m_AutoLocaleSupport = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "al", 0);
     myGlobals.m_AutoSwitchTabs = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "autoswitchtabs", 0);
     myGlobals.m_CutContactNameTo = (int) DBGetContactSettingWord(NULL, SRMSGMOD_T, "cut_at", 15);
     myGlobals.m_CutContactNameOnTabs = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "cuttitle", 0);
     myGlobals.m_StatusOnTabs = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "tabstatus", 0);
     myGlobals.m_LogStatusChanges = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "logstatus", 0);
     myGlobals.m_UseDividers = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "usedividers", 0);
     myGlobals.m_DividersUsePopupConfig = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "div_popupconfig", 0);
     myGlobals.m_MsgTimeout = (int)DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_MSGTIMEOUT, SRMSGDEFSET_MSGTIMEOUT);
     myGlobals.m_EscapeCloses = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "escmode", 0);
     myGlobals.m_WarnOnClose = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "warnonexit", 0);
     myGlobals.m_ExtraMicroLF = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "extramicrolf", 0);
     myGlobals.m_AvatarMode = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "avatarmode", 0);

     if(myGlobals.m_AvatarMode == 1 || myGlobals.m_AvatarMode == 2)
         myGlobals.m_AvatarMode = 3;

	 myGlobals.m_OwnAvatarMode = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "ownavatarmode", 0);
     myGlobals.m_FlashOnClist = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "flashcl", 0);
     myGlobals.m_TabAutoClose = (int)DBGetContactSettingDword(NULL, SRMSGMOD_T, "tabautoclose", 0);
     myGlobals.m_AlwaysFullToolbarWidth = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "alwaysfulltoolbar", 1);
     myGlobals.m_LimitStaticAvatarHeight = (int)DBGetContactSettingDword(NULL, SRMSGMOD_T, "avatarheight", 100);
     myGlobals.m_AvatarDisplayMode = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "avatardisplaymode", 0);
     myGlobals.m_SendFormat = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "sendformat", 0);
     myGlobals.m_FormatWholeWordsOnly = 1;
     myGlobals.m_AllowSendButtonHidden = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "hidesend", 0);
     myGlobals.m_ToolbarHideMode = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "tbarhidemode", 0);
     myGlobals.m_FixFutureTimestamps = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "do_fft", 1);
     myGlobals.m_RTLDefault = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "rtldefault", 0);
     myGlobals.m_SplitterSaveOnClose = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "splitsavemode", 1);
     myGlobals.m_WheelDefault = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "no_wheelhack", 0);
     myGlobals.m_MathModAvail = 0;
     myGlobals.m_WinVerMajor = WinVerMajor();
     myGlobals.m_WinVerMinor = WinVerMinor();
     myGlobals.m_bIsXP = IsWinVerXPPlus();
     //myGlobals.m_SideBarEnabled = (BYTE)DBGetContactSettingByte(NULL, SRMSGMOD_T, "sidebar", 0);
     myGlobals.m_TabAppearance = (int)DBGetContactSettingDword(NULL, SRMSGMOD_T, "tabconfig", TCF_FLASHICON);
     myGlobals.m_ExtraRedraws = (BYTE)DBGetContactSettingByte(NULL, SRMSGMOD_T, "aggromode", 0);
     myGlobals.m_panelHeight = (DWORD)DBGetContactSettingDword(NULL, SRMSGMOD_T, "panelheight", 51);
     myGlobals.m_Send7bitStrictAnsi = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "7bitasANSI", 1);
     myGlobals.m_IdleDetect = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "detectidle", 1);
     myGlobals.m_DoStatusMsg = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "dostatusmsg", 1);
     myGlobals.m_smcxicon = GetSystemMetrics(SM_CXSMICON);
     myGlobals.m_smcyicon = GetSystemMetrics(SM_CYSMICON);
     myGlobals.g_WantIEView = ServiceExists(MS_IEVIEW_WINDOW);
     myGlobals.m_PasteAndSend = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "pasteandsend", 0);
     myGlobals.m_szNoStatus = TranslateT("No status message available");
     myGlobals.ipConfig.borderStyle = (BYTE)DBGetContactSettingByte(NULL, SRMSGMOD_T, "ipfieldborder", IPFIELD_SUNKEN);
	 myGlobals.bAvatarBoderType = (BYTE)DBGetContactSettingByte(NULL, SRMSGMOD_T, "avbordertype", 1);
     myGlobals.m_dropShadow = (BYTE)DBGetContactSettingByte(NULL, SRMSGMOD_T, "dropshadow", 0);
	 myGlobals.m_LangPackCP = ServiceExists(MS_LANGPACK_GETCODEPAGE) ? CallService(MS_LANGPACK_GETCODEPAGE, 0, 0) : CP_ACP;
     myGlobals.m_SmileyButtonOverride = (BYTE)DBGetContactSettingByte(NULL, SRMSGMOD_T, "smbutton_override", 0);

     switch(myGlobals.ipConfig.borderStyle) {
         case IPFIELD_SUNKEN:
             myGlobals.ipConfig.edgeType = BDR_SUNKENINNER;
             break;
         case IPFIELD_RAISEDINNER:
             myGlobals.ipConfig.edgeType = BDR_RAISEDINNER;
             break;
         case IPFIELD_RAISEDOUTER:
             myGlobals.ipConfig.edgeType = BDR_RAISEDOUTER;
             break;
         case IPFIELD_EDGE:
             myGlobals.ipConfig.edgeType = EDGE_BUMP;
             break;
     }
     myGlobals.ipConfig.edgeFlags = BF_RECT | BF_ADJUST;
     // checkversion..
     {
         char str[512];
         CallService(MS_SYSTEM_GETVERSIONTEXT, (WPARAM)500, (LPARAM)(char*)str);
         if(strstr(str, "Unicode"))
             myGlobals.bUnicodeBuild = TRUE;
         else
             myGlobals.bUnicodeBuild = FALSE;
     }
     myGlobals.ncm.cbSize = sizeof(NONCLIENTMETRICS);
     SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &myGlobals.ncm, 0);
}

void GetDefaultContainerTitleFormat()
{
	DBVARIANT dbv = {0};
#if defined(_UNICODE)
	if(DBGetContactSettingTString(NULL, SRMSGMOD_T, "titleformatW", &dbv)) {
        DBWriteContactSettingTString(NULL, SRMSGMOD_T, "titleformatW", _T("%n - %s"));
		_tcsncpy(myGlobals.szDefaultTitleFormat, L"%n - %s", safe_sizeof(myGlobals.szDefaultTitleFormat));
	}
	else {
		_tcsncpy(myGlobals.szDefaultTitleFormat, dbv.ptszVal, safe_sizeof(myGlobals.szDefaultTitleFormat));
		DBFreeVariant(&dbv);
	}
	myGlobals.szDefaultTitleFormat[255] = 0;
#else
	if(DBGetContactSetting(NULL, SRMSGMOD_T, "titleformat", &dbv)) {
        DBWriteContactSettingString(NULL, SRMSGMOD_T, "titleformat", "%n - %s");
		_tcsncpy(myGlobals.szDefaultTitleFormat, "%n - %s", sizeof(myGlobals.szDefaultTitleFormat));
	}
	else {
		_tcsncpy(myGlobals.szDefaultTitleFormat, dbv.pszVal, sizeof(myGlobals.szDefaultTitleFormat));
		DBFreeVariant(&dbv);
	}
	myGlobals.szDefaultTitleFormat[255] = 0;
#endif
}