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
#include "msgs.h"
#include "m_ieview.h"
#include "m_fontservice.h"
#include "msgdlgutils.h"
#include "../../include/m_clc.h"
#include "../../include/m_clui.h"

#ifdef __MATHMOD_SUPPORT
    #include "m_MathModule.h"
#endif

#define DM_GETSTATUSMASK (WM_USER + 10)

extern MYGLOBALS myGlobals;
extern HANDLE hMessageWindowList;
extern HINSTANCE g_hInst;
extern struct ContainerWindowData *pFirstContainer;

extern BOOL CALLBACK DlgProcPopupOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK DlgProcTemplateEditor(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

TCHAR *MY_DBGetContactSettingString(HANDLE hContact, char *szModule, char *szSetting);
HMENU BuildContainerMenu();
void CacheMsgLogIcons(), CacheLogFonts(), ReloadGlobals(), LoadIconTheme(), UnloadIconTheme();
void CreateImageList(BOOL bInitial);
void _DBWriteContactSettingWString(HANDLE hContact, const char *szKey, const char *szSetting, wchar_t *value);
BOOL CALLBACK DlgProcSetupStatusModes(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK OptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void GetDefaultContainerTitleFormat();

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
    

HIMAGELIST CreateStateImageList()
{
    HIMAGELIST himl = ImageList_Create(16, 16, IsWinVerXPPlus() ? ILC_COLOR32 | ILC_MASK : ILC_COLOR8 | ILC_MASK, 4, 0);
    ImageList_AddIcon(himl, myGlobals.g_IconFolder);
    ImageList_AddIcon(himl, myGlobals.g_IconFolder);
    ImageList_AddIcon(himl, myGlobals.g_IconUnchecked);
    ImageList_AddIcon(himl, myGlobals.g_IconChecked);
    return himl;
}

void LoadLogfont(int i, LOGFONTA * lf, COLORREF * colour)
{
    char str[20];
    int style;
    DBVARIANT dbv;
    char bSize;

    if (colour) {
        _snprintf(str, sizeof(str), "Font%dCol", i);
        *colour = DBGetContactSettingDword(NULL, FONTMODULE, str, GetSysColor(COLOR_BTNTEXT));
    }
    if (lf) {
        mir_snprintf(str, sizeof(str), "Font%dSize", i);
        if(i == H_MSGFONTID_DIVIDERS)
            lf->lfHeight = 5;
        else {
            bSize = (char)DBGetContactSettingByte(NULL, FONTMODULE, str, -10);
            if(bSize < 0)
                lf->lfHeight = abs(bSize);
            else
                lf->lfHeight = (LONG)bSize;
        }
        
        lf->lfWidth = 0;
        lf->lfEscapement = 0;
        lf->lfOrientation = 0;
        mir_snprintf(str, sizeof(str), "Font%dSty", i);
        style = DBGetContactSettingByte(NULL, FONTMODULE, str, fontOptionsList[0].defStyle);
        lf->lfWeight = style & FONTF_BOLD ? FW_BOLD : FW_NORMAL;
        lf->lfItalic = style & FONTF_ITALIC ? 1 : 0;
        lf->lfUnderline = style & FONTF_UNDERLINE ? 1 : 0;
        lf->lfStrikeOut = 0;
        mir_snprintf(str, sizeof(str), "Font%dSet", i);
        if(i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT)
            lf->lfCharSet = SYMBOL_CHARSET;
        else
            lf->lfCharSet = DBGetContactSettingByte(NULL, FONTMODULE, str, fontOptionsList[0].defCharset);
        lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
        lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
        lf->lfQuality = DEFAULT_QUALITY;
        lf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        mir_snprintf(str, sizeof(str), "Font%d", i);
        if(i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT) {
            lstrcpynA(lf->lfFaceName, "Webdings", LF_FACESIZE);
            lf->lfCharSet = SYMBOL_CHARSET;
        }
        else {
			if (DBGetContactSetting(NULL, FONTMODULE, str, &dbv)) {
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


static const char *szFontIdDescr[MSGDLGFONTCOUNT] = {
        ">> Outgoing messages", 
        ">> Outgoing misc events",
        "<< Incoming messages",
        "<< Incoming misc events",
        ">> Outgoing name",
        ">> Outgoing timestamp",
        "<< Incoming name",
        "<< Incoming timestamp",
        ">> Outgoing messages (old)",
        ">> Outgoing misc events (old)",
        "<< Incoming messages (old)",
        "<< Incoming misc events (old)",
        ">> Outgoing name (old)",
        ">> Outgoing time (old)",
        "<< Incoming name (old)",
        "<< Incoming time (old)",
        "* Message Input Area",
        "* Status changes",
        "* Dividers",
        "* Error and warning Messages",
        "* Symbols (incoming)",
        "* Symbols (outgoing)"};

static const char *szIPFontDescr[IPFONTCOUNT] = {
        "Infopanel / Nickname", 
        "Infopanel / UIN",
        "Infopanel / Status",
        "Infopanel / Protocol",
        "Infopanel / Contacts local time",
		"Window caption (skinned mode"};

        /*
 * Font Service support
 */

static struct _colornames { char *szName; char *szModule; char *szSetting; DWORD defclr; } colornames[] = {
    "Background", FONTMODULE, SRMSGSET_BKGCOLOUR, 0,
    "Incoming events", FONTMODULE, "inbg", 0,
    "Outgoing events", FONTMODULE, "outbg", 0,
    "Input area background", FONTMODULE, "inputbg", 0,
    "Horizontal grid lines", FONTMODULE, "hgrid", 0,
    NULL, NULL, NULL
};

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
                lstrcpynA(fid.deffontsettings.szFace, fontOptionsList[0].szDefFace, LF_FACESIZE);
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
            lstrcpynA(fid.deffontsettings.szFace, fontOptionsList[0].szDefFace, LF_FACESIZE);
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

static struct LISTOPTIONSGROUP defaultGroups[] = {
    0, _T("Message window behaviour"),
    0, _T("Sending messages"),
    0, _T("Other options"),
    0, NULL
};

static struct LISTOPTIONSITEM defaultItems[] = {
    0, _T("Send on SHIFT - Enter"), IDC_SENDONSHIFTENTER, LOI_TYPE_SETTING, (UINT_PTR)"sendonshiftenter", 1,
    0, _T("Send message on 'Enter'"), SRMSGDEFSET_SENDONENTER, LOI_TYPE_SETTING, (UINT_PTR)SRMSGSET_SENDONENTER, 1,
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

static BOOL CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
			DWORD msgTimeout;
			BOOL translated;
			TVINSERTSTRUCT tvi = {0};
			int i = 0;

			DWORD dwFlags = DBGetContactSettingDword(NULL, SRMSGMOD_T, "mwflags", MWF_LOG_DEFAULT);

			TranslateDialogDefault(hwndDlg);
			SetWindowLong(GetDlgItem(hwndDlg, IDC_WINDOWOPTIONS), GWL_STYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_WINDOWOPTIONS), GWL_STYLE) | (TVS_NOHSCROLL | TVS_CHECKBOXES));

			SendDlgItemMessage(hwndDlg, IDC_WINDOWOPTIONS, TVM_SETIMAGELIST, TVSIL_STATE, (LPARAM)CreateStateImageList());

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
			SendDlgItemMessage(hwndDlg, IDC_AVATARMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("On for protocols with avatar support"));
			SendDlgItemMessage(hwndDlg, IDC_AVATARMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Per contact setting"));
			SendDlgItemMessage(hwndDlg, IDC_AVATARMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("On, if present"));
			SendDlgItemMessage(hwndDlg, IDC_AVATARMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Globally OFF"));

			SendDlgItemMessage(hwndDlg, IDC_OWNAVATARMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Show them if present"));
			SendDlgItemMessage(hwndDlg, IDC_OWNAVATARMODE, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Don't show them"));

			SendDlgItemMessage(hwndDlg, IDC_AVATARDISPLAY, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Dynamically resize"));
			SendDlgItemMessage(hwndDlg, IDC_AVATARDISPLAY, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Static"));

			SendDlgItemMessage(hwndDlg, IDC_AVATARMODE, CB_SETCURSEL, (WPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "avatarmode", 0), 0);
			SendDlgItemMessage(hwndDlg, IDC_OWNAVATARMODE, CB_SETCURSEL, (WPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "ownavatarmode", 0), 0);

			SendDlgItemMessage(hwndDlg, IDC_AVATARDISPLAY, CB_SETCURSEL, (WPARAM)DBGetContactSettingByte(NULL, SRMSGMOD_T, "avatardisplaymode", 0), 0);

			msgTimeout = DBGetContactSettingDword(NULL, SRMSGMOD, SRMSGSET_MSGTIMEOUT, SRMSGDEFSET_MSGTIMEOUT);
			SetDlgItemInt(hwndDlg, IDC_SECONDS, msgTimeout >= SRMSGSET_MSGTIMEOUT_MIN ? msgTimeout / 1000 : SRMSGDEFSET_MSGTIMEOUT / 1000, FALSE);

			SetDlgItemInt(hwndDlg, IDC_MAXAVATARHEIGHT, DBGetContactSettingDword(NULL, SRMSGMOD_T, "avatarheight", 100), FALSE);
			SendDlgItemMessage(hwndDlg, IDC_AVATARSPIN, UDM_SETRANGE, 0, MAKELONG(150, 50));
			SendDlgItemMessage(hwndDlg, IDC_AVATARSPIN, UDM_SETPOS, 0, GetDlgItemInt(hwndDlg, IDC_MAXAVATARHEIGHT, &translated, FALSE));

			SetDlgItemInt(hwndDlg, IDC_AUTOCLOSETABTIME, DBGetContactSettingDword(NULL, SRMSGMOD_T, "tabautoclose", 0), FALSE);
			SendDlgItemMessage(hwndDlg, IDC_AUTOCLOSETABSPIN, UDM_SETRANGE, 0, MAKELONG(1800, 0));
			SendDlgItemMessage(hwndDlg, IDC_AUTOCLOSETABSPIN, UDM_SETPOS, 0, GetDlgItemInt(hwndDlg, IDC_AUTOCLOSETABTIME, &translated, FALSE));
			CheckDlgButton(hwndDlg, IDC_AUTOCLOSELAST, DBGetContactSettingByte(NULL, SRMSGMOD_T, "autocloselast", 0));

			SendDlgItemMessage(hwndDlg, IDC_SENDFORMATTING, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Off"));
			SendDlgItemMessage(hwndDlg, IDC_SENDFORMATTING, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Simple Tags (*/_)"));
			SendDlgItemMessage(hwndDlg, IDC_SENDFORMATTING, CB_INSERTSTRING, -1, (LPARAM)TranslateT("BBCode"));

			SendDlgItemMessage(hwndDlg, IDC_SENDFORMATTING, CB_SETCURSEL, (WPARAM)myGlobals.m_SendFormat, 0);

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

					DBWriteContactSettingByte(NULL, SRMSGMOD_T, "avbordertype", (BYTE) SendDlgItemMessage(hwndDlg, IDC_AVATARBORDER, CB_GETCURSEL, 0, 0));
					DBWriteContactSettingDword(NULL, SRMSGMOD_T, "avborderclr", SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR, 0, 0));
					DBWriteContactSettingByte(NULL, SRMSGMOD_T, "avatarmode", (BYTE) SendDlgItemMessage(hwndDlg, IDC_AVATARMODE, CB_GETCURSEL, 0, 0));
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
	case WM_DESTROY:
		{
			HIMAGELIST himl = (HIMAGELIST)SendDlgItemMessage(hwndDlg, IDC_WINDOWOPTIONS, TVM_GETIMAGELIST, TVSIL_STATE, 0);
			if(himl)
				ImageList_Destroy(himl);
			break;
		}
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
    0, _T("Use IEView as default message log"), 0, LOI_TYPE_SETTING, (UINT_PTR)"want_ieview", 1,
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
			SendDlgItemMessage(hwndDlg, IDC_LOGOPTIONS, TVM_SETIMAGELIST, TVSIL_STATE, (LPARAM)CreateStateImageList());

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
					if(!strcmp((char *)lvItems[i].lParam, "want_ieview") && !ServiceExists(MS_IEVIEW_EVENT)) {
						i++;
						continue;
					}
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

			CheckDlgButton(hwndDlg, IDC_TSFIX, DBGetContactSettingByte(NULL, SRMSGMOD_T, "no_future", 0));

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
		case IDC_TSFIX:
			if(IsDlgButtonChecked(hwndDlg, IDC_TSFIX))
				MessageBoxA(0, "Caution: This attempt to fix the 'future timestamp issue' may have side effects. Also, it only works for events while the session is active, NOT for loading the history", "Warning", MB_OK | MB_ICONHAND);
			break;
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
					DBWriteContactSettingByte(NULL, SRMSGMOD_T, "no_future", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_TSFIX));

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
	case WM_DESTROY:
		{
			HIMAGELIST himl = (HIMAGELIST)SendDlgItemMessage(hwndDlg, IDC_LOGOPTIONS, TVM_GETIMAGELIST, TVSIL_STATE, 0);
			if(himl)
				ImageList_Destroy(himl);
			break;
		}
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
    0, _T("Warn on close message tab"), 0, LOI_TYPE_SETTING, (UINT_PTR)"warnonexit", 0,
    0, _T("Automatically pop up the window/tab when a message is received (has PRIORITY!)"), SRMSGDEFSET_AUTOPOPUP, LOI_TYPE_SETTING, (UINT_PTR)SRMSGSET_AUTOPOPUP, 1,
    0, _T("Create tabs in the background"), 0, LOI_TYPE_SETTING, (UINT_PTR)"autotabs", 1,
    0, _T("Create containers minimized on the taskbar"), 0, LOI_TYPE_SETTING, (UINT_PTR)"autocontainer", 1,
    0, _T("Popup container if minimized"), 0, LOI_TYPE_SETTING, (UINT_PTR)"cpopup", 1,
    0, _T("Automatically switch tabs in minimized containers"), 0, LOI_TYPE_SETTING, (UINT_PTR)"autoswitchtabs", 1,
    0, _T("Don't draw visual styles on toolbar buttons"), 1, LOI_TYPE_SETTING, (UINT_PTR)"nlflat", 2,
    0, _T("Flat toolbar buttons"), 1, LOI_TYPE_SETTING, (UINT_PTR)"tbflat", 2,
    0, _T("Splitters have static edges"), 1, LOI_TYPE_SETTING, (UINT_PTR)"splitteredges", 2,
    0, _T("Flat message log (no static edge)"), 1, LOI_TYPE_SETTING, (UINT_PTR)"flatlog", 2,
    0, _T("Activate autolocale support"), 0, LOI_TYPE_SETTING, (UINT_PTR)"al", 3,
    0, _T("ESC closes sessions (minimizes window, if disabled)"), 0, LOI_TYPE_SETTING, (UINT_PTR)"escmode", 3,
    0, _T("Use global hotkeys (configure modifiers below)"), 0, LOI_TYPE_SETTING, (UINT_PTR)"globalhotkeys", 3,
    0, _T("Perform version check on Icon DLL"), 1, LOI_TYPE_SETTING, (UINT_PTR)"v_check", 3,
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
			SendDlgItemMessage(hwndDlg, IDC_TABMSGOPTIONS, TVM_SETIMAGELIST, TVSIL_STATE, (LPARAM)CreateStateImageList());

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
	case WM_DESTROY:
		{
			HIMAGELIST himl = (HIMAGELIST)SendDlgItemMessage(hwndDlg, IDC_TABMSGOPTIONS, TVM_GETIMAGELIST, TVSIL_STATE, 0);
			if(himl)
				ImageList_Destroy(himl);
			break;
		}
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

			if(!DBGetContactSetting(NULL, SRMSGMOD_T, "ContainerSkin", &dbv))
				SetDlgItemTextA(hwndDlg, IDC_CONTAINERSKIN, dbv.pszVal);
		 return TRUE;
		}
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
				case IDC_GETCONTAINERSKINNAME:
					{
						char szFilename[MAX_PATH];
						OPENFILENAMEA ofn={0};

						szFilename[0] = 0;
						ofn.lpstrFilter = "tabSRMM skins\0*.tsk\0\0";
						ofn.lStructSize= OPENFILENAME_SIZE_VERSION_400;
						ofn.hwndOwner=0;
						ofn.lpstrFile = szFilename;
						ofn.lpstrInitialDir = ".";
						ofn.nMaxFile = MAX_PATH;
						ofn.nMaxFileTitle = MAX_PATH;
						ofn.Flags = OFN_HIDEREADONLY;
						ofn.lpstrDefExt = "tsk";
						if(GetOpenFileNameA(&ofn)) {
							char szFinalName[MAX_PATH];

							CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)szFilename, (LPARAM)szFinalName);
							DBWriteContactSettingString(NULL, SRMSGMOD_T, "ContainerSkin", szFinalName);
							SetDlgItemTextA(hwndDlg, IDC_CONTAINERSKIN, szFinalName);
							ReloadContainerSkin();
						}
						break;
					}
				case IDC_RELOAD:
					ReloadContainerSkin();
					break;
				case IDC_USESKIN:
					{
						BYTE useskin = (IsDlgButtonChecked(hwndDlg, IDC_USESKIN) && IsWinVer2000Plus()) ? 1 : 0;

						EnableWindow(GetDlgItem(hwndDlg, IDC_CONTAINERSKIN), useskin ? TRUE : FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_GETCONTAINERSKINNAME), useskin ? TRUE : FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_RELOAD), useskin ? TRUE : FALSE);
						if(lParam) {
							DBWriteContactSettingByte(NULL, SRMSGMOD_T, "useskin", useskin);
							ReloadContainerSkin();
						}
						break;
					}
                case IDC_TABLIMIT:
				case IDC_CONTAINERSKIN:
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
							char szFilename[MAX_PATH], szFinalName[MAX_PATH];

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
							GetDlgItemTextA(hwndDlg, IDC_CONTAINERSKIN, szFilename, MAX_PATH);
							szFilename[MAX_PATH - 1] = 0;
							CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)szFilename, (LPARAM)szFinalName);
							DBWriteContactSettingString(NULL, SRMSGMOD_T, "ContainerSkin", szFinalName);
                            return TRUE;
                        }
                    }
            }
            break;
        case WM_DESTROY:
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
static WORD fontSameAsDefault[MSGDLGFONTCOUNT + 2]={0x00FF,0x0B00,0x0F00,0x0700,0x0B00,0x0104,0x0D00,0x0B02,0x00FF,0x0B00,0x0F00,0x0700,0x0B00,0x0104,0x0D00,0x0B02,0x00FF,0x0B00,0x0F00};
static char *fontSizes[]={"8","9","10","11","12","13","14","15","18"};
static int fontListOrder[MSGDLGFONTCOUNT + 1] = {
    MSGFONTID_MYMSG,
    MSGFONTID_MYMISC,
    MSGFONTID_YOURMSG,    
    MSGFONTID_YOURMISC,    
    MSGFONTID_MYNAME,     
    MSGFONTID_MYTIME,     
    MSGFONTID_YOURNAME,   
    MSGFONTID_YOURTIME,   
    H_MSGFONTID_MYMSG,
    H_MSGFONTID_MYMISC,   
    H_MSGFONTID_YOURMSG,  
    H_MSGFONTID_YOURMISC,  
    H_MSGFONTID_MYNAME,
    H_MSGFONTID_MYTIME,
    H_MSGFONTID_YOURNAME, 
    H_MSGFONTID_YOURTIME,
    MSGFONTID_MESSAGEAREA,
    H_MSGFONTID_STATUSCHANGES,
    H_MSGFONTID_DIVIDERS,
    MSGFONTID_ERROR,
    MSGFONTID_SYMBOLS_IN,
    MSGFONTID_SYMBOLS_OUT};

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

static int CALLBACK EnumFontsProc(ENUMLOGFONTEXA *lpelfe,NEWTEXTMETRICEXA *lpntme,int FontType,LPARAM lParam)
{
	if(!IsWindow((HWND)lParam)) return FALSE;
	if(SendMessageA((HWND)lParam,CB_FINDSTRINGEXACT,-1,(LPARAM)lpelfe->elfLogFont.lfFaceName)==CB_ERR)
		SendMessageA((HWND)lParam,CB_ADDSTRING,0,(LPARAM)lpelfe->elfLogFont.lfFaceName);
	return TRUE;
}

void FillFontListThread(HWND hwndDlg)
{
	LOGFONTA lf={0};
	HDC hdc=GetDC(hwndDlg);
	lf.lfCharSet=DEFAULT_CHARSET;
	lf.lfFaceName[0]=0;
	lf.lfPitchAndFamily=0;
	EnumFontFamiliesExA(hdc,&lf,(FONTENUMPROCA)EnumFontsProc,(LPARAM)GetDlgItem(hwndDlg,IDC_TYPEFACE),0);
	ReleaseDC(hwndDlg,hdc);
	return;
}

static int CALLBACK EnumFontScriptsProc(ENUMLOGFONTEXA *lpelfe,NEWTEXTMETRICEXA *lpntme,int FontType,LPARAM lParam)
{
	if(SendMessageA((HWND)lParam,CB_FINDSTRINGEXACT,-1,(LPARAM)lpelfe->elfScript)==CB_ERR) {
		int i=SendMessageA((HWND)lParam,CB_ADDSTRING,0,(LPARAM)lpelfe->elfScript);
		SendMessageA((HWND)lParam,CB_SETITEMDATA,i,lpelfe->elfLogFont.lfCharSet);
	}
	return TRUE;
}

static int TextOptsDlgResizer(HWND hwndDlg,LPARAM lParam,UTILRESIZECONTROL *urc)
{
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
}

static void SwitchTextDlgToMode(HWND hwndDlg,int expert)
{
	ShowWindow(GetDlgItem(hwndDlg,IDC_STSAMETEXT),expert?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_SAMETYPE),expert?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_SAMESIZE),expert?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_SAMESTYLE),expert?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_SAMECOLOUR),expert?SW_SHOW:SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg,IDC_STSIZETEXT),expert?SW_HIDE:SW_SHOW);
	ShowWindow(GetDlgItem(hwndDlg,IDC_STCOLOURTEXT),expert?SW_HIDE:SW_SHOW);
	SetDlgItemTextA(hwndDlg,IDC_STASTEXT,Translate(expert?"as:":"based on:"));
	{	UTILRESIZEDIALOG urd={0};
		urd.cbSize=sizeof(urd);
		urd.hwndDlg=hwndDlg;
		urd.hInstance=g_hInst;
		urd.lpTemplate=MAKEINTRESOURCEA(IDD_OPT_MSGWINDOWFONTS);
		urd.pfnResizer=TextOptsDlgResizer;
		CallService(MS_UTILS_RESIZEDIALOG,0,(LPARAM)&urd);
	}
	//resizer breaks the sizing of the edit box
	SendMessageA(hwndDlg,M_REFRESHSAMEASBOXES,SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_GETCURSEL,0,0),0),0);
}
    

static void GetDefaultFontSetting(int i,LOGFONTA *lf,COLORREF *colour)
{
	SystemParametersInfoA(SPI_GETICONTITLELOGFONT,sizeof(LOGFONTA),lf,FALSE);
	*colour=GetSysColor(COLOR_WINDOWTEXT);
	switch(i) {
		case FONTID_GROUPS:
			lf->lfWeight=FW_BOLD;
			break;
		case FONTID_GROUPCOUNTS:
			lf->lfHeight=(int)(lf->lfHeight*.75);
			*colour=GetSysColor(COLOR_3DSHADOW);
			break;
		case FONTID_OFFINVIS:
		case FONTID_INVIS:
			lf->lfItalic=!lf->lfItalic;
			break;
		case FONTID_DIVIDERS:
			lf->lfHeight=(int)(lf->lfHeight*.75);
			break;
		case FONTID_NOTONLIST:
			*colour=GetSysColor(COLOR_3DSHADOW);
			break;
	}
}

#define SRFONTSETTINGMODULE FONTMODULE

void GetFontSetting(int i,LOGFONTA *lf,COLORREF *colour)
{
	DBVARIANT dbv;
	char idstr[12];
	BYTE style;
    
	GetDefaultFontSetting(i,lf,colour);
	_snprintf(idstr, sizeof(idstr), "Font%d",i);
	if(!DBGetContactSetting(NULL,SRFONTSETTINGMODULE,idstr,&dbv)) {
		lstrcpyA(lf->lfFaceName,dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	_snprintf(idstr, sizeof(idstr), "Font%dCol",i);
	*colour=DBGetContactSettingDword(NULL,SRFONTSETTINGMODULE,idstr,*colour);
	_snprintf(idstr, sizeof(idstr), "Font%dSize",i);
    lf->lfHeight = (char)DBGetContactSettingByte(NULL,SRFONTSETTINGMODULE,idstr,lf->lfHeight);
	_snprintf(idstr, sizeof(idstr), "Font%dSty",i);
	style=(BYTE)DBGetContactSettingByte(NULL,SRFONTSETTINGMODULE,idstr,(lf->lfWeight==FW_NORMAL?0:DBFONTF_BOLD)|(lf->lfItalic?DBFONTF_ITALIC:0)|(lf->lfUnderline?DBFONTF_UNDERLINE:0));
	lf->lfWidth=lf->lfEscapement=lf->lfOrientation=0;
	lf->lfWeight=style&DBFONTF_BOLD?FW_BOLD:FW_NORMAL;
	lf->lfItalic=(style&DBFONTF_ITALIC)!=0;
	lf->lfUnderline=(style&DBFONTF_UNDERLINE)!=0;
	lf->lfStrikeOut=0;
	_snprintf(idstr, sizeof(idstr), "Font%dSet",i);
	lf->lfCharSet=DBGetContactSettingByte(NULL,SRFONTSETTINGMODULE,idstr,lf->lfCharSet);
	lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
	lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
	lf->lfQuality=DEFAULT_QUALITY;
	lf->lfPitchAndFamily=DEFAULT_PITCH|FF_DONTCARE;
}

static BOOL CALLBACK DlgProcMsgWindowFonts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HFONT hFontSample;
    
	switch (msg)
	{
		case WM_INITDIALOG:
            hFontSample=NULL;
			SetDlgItemTextA(hwndDlg,IDC_SAMPLE,"Sample");
			TranslateDialogDefault(hwndDlg);
			if(!SendMessage(GetParent(hwndDlg),PSM_ISEXPERT,0,0))
				SwitchTextDlgToMode(hwndDlg,0);
				FillFontListThread(hwndDlg);
			{	int i,itemId,fontId;
				LOGFONTA lf;
				COLORREF colour;
				WORD sameAs;
				char str[32];
                HDC hdc = GetDC(NULL);
                
				for(i=0;i<FONTS_TO_CONFIG;i++) {
					fontId=fontListOrder[i];
					GetFontSetting(fontId,&lf,&colour);
					sprintf(str,"Font%dAs",fontId);
					sameAs=DBGetContactSettingWord(NULL,SRFONTSETTINGMODULE,str,fontSameAsDefault[fontId]);
					fontSettings[fontId].sameAsFlags=HIBYTE(sameAs);
					fontSettings[fontId].sameAs=LOBYTE(sameAs);
					fontSettings[fontId].style=(lf.lfWeight==FW_NORMAL?0:DBFONTF_BOLD)|(lf.lfItalic?DBFONTF_ITALIC:0)|(lf.lfUnderline?DBFONTF_UNDERLINE:0);
    				fontSettings[fontId].size = (char)MulDiv(abs(lf.lfHeight), 72, GetDeviceCaps(hdc, LOGPIXELSY));
					fontSettings[fontId].charset=lf.lfCharSet;
					fontSettings[fontId].colour=colour;
					lstrcpyA(fontSettings[fontId].szFace,lf.lfFaceName);
					itemId=SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_ADDSTRING,0,(LPARAM)Translate(szFontIdDescr[fontId]));
					SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_SETITEMDATA,itemId,fontId);
				}
                ReleaseDC(NULL, hdc);
                SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, FONTMODULE, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR));
                SendDlgItemMessage(hwndDlg, IDC_INPUTBKG, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, FONTMODULE, "inputbg", SRMSGDEFSET_BKGCOLOUR));
                SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETDEFAULTCOLOUR, 0, SRMSGDEFSET_BKGCOLOUR);
                SendDlgItemMessage(hwndDlg, IDC_INPUTBKG, CPM_SETDEFAULTCOLOUR, 0, SRMSGDEFSET_BKGCOLOUR);
				SendDlgItemMessage(hwndDlg, IDC_INFOPANELBG, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, FONTMODULE, "ipfieldsbg", SRMSGDEFSET_BKGCOLOUR));
                SendDlgItemMessage(hwndDlg, IDC_BKGOUTGOING, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, FONTMODULE, "outbg", SRMSGDEFSET_BKGCOLOUR));
                SendDlgItemMessage(hwndDlg, IDC_BKGINCOMING, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, FONTMODULE, "inbg", SRMSGDEFSET_BKGCOLOUR));
                SendDlgItemMessage(hwndDlg, IDC_GRIDLINES, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, FONTMODULE, "hgrid", SRMSGDEFSET_BKGCOLOUR));
                SendDlgItemMessage(hwndDlg, IDC_BKGINCOMING, CPM_SETDEFAULTCOLOUR, 0, SRMSGDEFSET_BKGCOLOUR);
                SendDlgItemMessage(hwndDlg, IDC_BKGOUTGOING, CPM_SETDEFAULTCOLOUR, 0, SRMSGDEFSET_BKGCOLOUR);
                
				SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_SETCURSEL,0,0);
				for(i=0;i<sizeof(fontSizes)/sizeof(fontSizes[0]);i++)
					SendDlgItemMessageA(hwndDlg,IDC_FONTSIZE,CB_ADDSTRING,0,(LPARAM)fontSizes[i]);
			}
            SendMessage(hwndDlg,M_REBUILDFONTGROUP,0,0);
			SendMessage(hwndDlg,M_SAVEFONT,0,0);
			return TRUE;
		case M_REBUILDFONTGROUP:	//remake all the needed controls when the user changes the font selector at the top
		{	int i=SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_GETCURSEL,0,0),0);
			SendMessage(hwndDlg,M_SETSAMEASBOXES,i,0);
			{	int j,id,itemId;
				char szText[256];
				SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_RESETCONTENT,0,0);
				itemId=SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_ADDSTRING,0,(LPARAM)TranslateT("<none>"));
				SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_SETITEMDATA,itemId,0xFF);
				if(0xFF==fontSettings[i].sameAs)
					SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_SETCURSEL,itemId,0);
				for(j=0;j<FONTS_TO_CONFIG;j++) {
					SendDlgItemMessage(hwndDlg,IDC_FONTID,CB_GETLBTEXT,j,(LPARAM)szText);
					id=SendDlgItemMessage(hwndDlg,IDC_FONTID,CB_GETITEMDATA,j,0);
					if(id==i) continue;
					itemId=SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_ADDSTRING,0,(LPARAM)szText);
					SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_SETITEMDATA,itemId,id);
					if(id==fontSettings[i].sameAs)
						SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_SETCURSEL,itemId,0);
				}
			}
			SendMessage(hwndDlg,M_LOADFONT,i,0);
			SendMessage(hwndDlg,M_REFRESHSAMEASBOXES,i,0);
			SendMessage(hwndDlg,M_REMAKESAMPLE,0,0);
			break;
		}
		case M_SETSAMEASBOXES:	//set the check mark in the 'same as' boxes to the right value for fontid wParam
			CheckDlgButton(hwndDlg,IDC_SAMETYPE,fontSettings[wParam].sameAsFlags&SAMEASF_FACE?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_SAMESIZE,fontSettings[wParam].sameAsFlags&SAMEASF_SIZE?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_SAMESTYLE,fontSettings[wParam].sameAsFlags&SAMEASF_STYLE?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_SAMECOLOUR,fontSettings[wParam].sameAsFlags&SAMEASF_COLOUR?BST_CHECKED:BST_UNCHECKED);
			break;
		case M_FILLSCRIPTCOMBO:		  //fill the script combo box and set the selection to the value for fontid wParam
		{	LOGFONTA lf={0};
			int i;
			HDC hdc=GetDC(hwndDlg);
			lf.lfCharSet=DEFAULT_CHARSET;
			GetDlgItemTextA(hwndDlg,IDC_TYPEFACE,lf.lfFaceName,sizeof(lf.lfFaceName));
			lf.lfPitchAndFamily=0;
			SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_RESETCONTENT,0,0);
			EnumFontFamiliesExA(hdc,&lf,(FONTENUMPROCA)EnumFontScriptsProc,(LPARAM)GetDlgItem(hwndDlg,IDC_SCRIPT),0);
			ReleaseDC(hwndDlg,hdc);
			for(i=SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_GETCOUNT,0,0)-1;i>=0;i--) {
				if(SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_GETITEMDATA,i,0)==fontSettings[wParam].charset) {
					SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_SETCURSEL,i,0);
					break;
				}
			}
			if(i<0) SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_SETCURSEL,0,0);
			break;
		}
		case WM_CTLCOLORSTATIC:
			if((HWND)lParam==GetDlgItem(hwndDlg,IDC_SAMPLE)) {
				SetTextColor((HDC)wParam,SendDlgItemMessageA(hwndDlg,IDC_COLOUR,CPM_GETCOLOUR,0,0));
                SetBkColor((HDC)wParam,GetSysColor(COLOR_3DFACE));
				return (BOOL)GetSysColorBrush(COLOR_3DFACE);
			}
			break;
		case M_REFRESHSAMEASBOXES:		  //set the disabled flag on the 'same as' checkboxes to the values for fontid wParam
			EnableWindow(GetDlgItem(hwndDlg,IDC_SAMETYPE),fontSettings[wParam].sameAs!=0xFF);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SAMESIZE),fontSettings[wParam].sameAs!=0xFF);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SAMESTYLE),fontSettings[wParam].sameAs!=0xFF);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SAMECOLOUR),fontSettings[wParam].sameAs!=0xFF);
			if(SendMessage(GetParent(hwndDlg),PSM_ISEXPERT,0,0)) {
				EnableWindow(GetDlgItem(hwndDlg,IDC_TYPEFACE),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_FACE));
				EnableWindow(GetDlgItem(hwndDlg,IDC_SCRIPT),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_FACE));
				EnableWindow(GetDlgItem(hwndDlg,IDC_FONTSIZE),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_SIZE));
				EnableWindow(GetDlgItem(hwndDlg,IDC_BOLD),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_STYLE));
				EnableWindow(GetDlgItem(hwndDlg,IDC_ITALIC),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_STYLE));
				EnableWindow(GetDlgItem(hwndDlg,IDC_UNDERLINE),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_STYLE));
                EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_COLOUR));
            }
			else {
				EnableWindow(GetDlgItem(hwndDlg,IDC_TYPEFACE),TRUE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_SCRIPT),TRUE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_FONTSIZE),TRUE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_BOLD),TRUE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_ITALIC),TRUE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_UNDERLINE),TRUE);
                EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR),TRUE);
			}
			break;
		case M_REMAKESAMPLE:	//remake the sample edit box font based on the settings in the controls
		{	LOGFONTA lf;
            HDC hdc = GetDC(NULL);
			if(hFontSample) {
				SendDlgItemMessageA(hwndDlg,IDC_SAMPLE,WM_SETFONT,SendDlgItemMessageA(hwndDlg,IDC_FONTID,WM_GETFONT,0,0),0);
				DeleteObject(hFontSample);
			}
			lf.lfHeight=GetDlgItemInt(hwndDlg,IDC_FONTSIZE,NULL,FALSE);
            
            lf.lfHeight=-MulDiv(lf.lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
            ReleaseDC(NULL,hdc);				
			lf.lfWidth=lf.lfEscapement=lf.lfOrientation=0;
			lf.lfWeight=IsDlgButtonChecked(hwndDlg,IDC_BOLD)?FW_BOLD:FW_NORMAL;
			lf.lfItalic=IsDlgButtonChecked(hwndDlg,IDC_ITALIC);
			lf.lfUnderline=IsDlgButtonChecked(hwndDlg,IDC_UNDERLINE);
			lf.lfStrikeOut=0;
			lf.lfCharSet=(BYTE)SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_GETCURSEL,0,0),0);
			lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
			lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
			lf.lfQuality=DEFAULT_QUALITY;
			lf.lfPitchAndFamily=DEFAULT_PITCH|FF_DONTCARE;
			GetDlgItemTextA(hwndDlg,IDC_TYPEFACE,lf.lfFaceName,sizeof(lf.lfFaceName));
			hFontSample=CreateFontIndirectA(&lf);
			SendDlgItemMessage(hwndDlg,IDC_SAMPLE,WM_SETFONT,(WPARAM)hFontSample,TRUE);
			break;
		}
		case M_RECALCONEFONT:	   //copy the 'same as' settings for fontid wParam from their sources
			if(fontSettings[wParam].sameAs==0xFF) break;
			if(fontSettings[wParam].sameAsFlags&SAMEASF_FACE) {
				lstrcpyA(fontSettings[wParam].szFace,fontSettings[fontSettings[wParam].sameAs].szFace);
				fontSettings[wParam].charset=fontSettings[fontSettings[wParam].sameAs].charset;
			}
			if(fontSettings[wParam].sameAsFlags&SAMEASF_SIZE)
				fontSettings[wParam].size=fontSettings[fontSettings[wParam].sameAs].size;
			if(fontSettings[wParam].sameAsFlags&SAMEASF_STYLE)
				fontSettings[wParam].style=fontSettings[fontSettings[wParam].sameAs].style;
			if(fontSettings[wParam].sameAsFlags&SAMEASF_COLOUR)
				fontSettings[wParam].colour=fontSettings[fontSettings[wParam].sameAs].colour;
			break;
		case M_RECALCOTHERFONTS:	//recalculate the 'same as' settings for all fonts but wParam
		{	int i;
			for(i=0;i<FONTS_TO_CONFIG;i++) {
				if(i==(int)wParam) continue;
				SendMessage(hwndDlg,M_RECALCONEFONT,i,0);
			}
			break;
		}
        case M_SAVEFONT:	//save the font settings from the controls to font wParam
			fontSettings[wParam].sameAsFlags=(IsDlgButtonChecked(hwndDlg,IDC_SAMETYPE)?SAMEASF_FACE:0)|(IsDlgButtonChecked(hwndDlg,IDC_SAMESIZE)?SAMEASF_SIZE:0)|(IsDlgButtonChecked(hwndDlg,IDC_SAMESTYLE)?SAMEASF_STYLE:0)|(IsDlgButtonChecked(hwndDlg,IDC_SAMECOLOUR)?SAMEASF_COLOUR:0);
			fontSettings[wParam].sameAs=(BYTE)SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_GETCURSEL,0,0),0);
			GetDlgItemTextA(hwndDlg,IDC_TYPEFACE,fontSettings[wParam].szFace,sizeof(fontSettings[wParam].szFace));
			fontSettings[wParam].charset=(BYTE)SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_SCRIPT,CB_GETCURSEL,0,0),0);
			fontSettings[wParam].size=(char)GetDlgItemInt(hwndDlg,IDC_FONTSIZE,NULL,FALSE);
			fontSettings[wParam].style=(IsDlgButtonChecked(hwndDlg,IDC_BOLD)?DBFONTF_BOLD:0)|(IsDlgButtonChecked(hwndDlg,IDC_ITALIC)?DBFONTF_ITALIC:0)|(IsDlgButtonChecked(hwndDlg,IDC_UNDERLINE)?DBFONTF_UNDERLINE:0);
            fontSettings[wParam].colour=SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_GETCOLOUR,0,0);
			SendMessage(hwndDlg,M_REDOROWHEIGHT,0,0);
			break;
		case M_REDOROWHEIGHT:	//recalculate the minimum feasible row height
		{	int i;
			int minHeight=1;//GetSystemMetrics(SM_CYSMICON);
			for(i=0;i<FONTS_TO_CONFIG;i++)
				if(fontSettings[i].size+2>minHeight) minHeight=fontSettings[i].size+2;
			break;
		}
		case M_LOADFONT:	//load font wParam into the controls
			SetDlgItemTextA(hwndDlg,IDC_TYPEFACE,fontSettings[wParam].szFace);
			SendMessage(hwndDlg,M_FILLSCRIPTCOMBO,wParam,0);
			SetDlgItemInt(hwndDlg,IDC_FONTSIZE,fontSettings[wParam].size,FALSE);
			CheckDlgButton(hwndDlg,IDC_BOLD,fontSettings[wParam].style&DBFONTF_BOLD?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_ITALIC,fontSettings[wParam].style&DBFONTF_ITALIC?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_UNDERLINE,fontSettings[wParam].style&DBFONTF_UNDERLINE?BST_CHECKED:BST_UNCHECKED);
			{	LOGFONTA lf;
				COLORREF colour;
				GetDefaultFontSetting(wParam,&lf,&colour);
                SendDlgItemMessageA(hwndDlg,IDC_COLOUR,CPM_SETDEFAULTCOLOUR,0,colour);
			}
			SendDlgItemMessageA(hwndDlg,IDC_COLOUR,CPM_SETCOLOUR,0,fontSettings[wParam].colour);
			break;
		case M_GUESSSAMEASBOXES:   //guess suitable values for the 'same as' checkboxes for fontId wParam
			fontSettings[wParam].sameAsFlags=0;
			if(fontSettings[wParam].sameAs==0xFF) break;
			if(!strcmp(fontSettings[wParam].szFace,fontSettings[fontSettings[wParam].sameAs].szFace) &&
			   fontSettings[wParam].charset==fontSettings[fontSettings[wParam].sameAs].charset)
    			fontSettings[wParam].sameAsFlags|=SAMEASF_FACE;
			if(fontSettings[wParam].size==fontSettings[fontSettings[wParam].sameAs].size)
    			fontSettings[wParam].sameAsFlags|=SAMEASF_SIZE;
			if(fontSettings[wParam].style==fontSettings[fontSettings[wParam].sameAs].style)
    			fontSettings[wParam].sameAsFlags|=SAMEASF_STYLE;
			if(fontSettings[wParam].colour==fontSettings[fontSettings[wParam].sameAs].colour)
    			fontSettings[wParam].sameAsFlags|=SAMEASF_COLOUR;
			SendMessage(hwndDlg,M_SETSAMEASBOXES,wParam,0);
			break;
		case WM_VSCROLL:
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case WM_COMMAND:
		{	int fontId=SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_FONTID,CB_GETCURSEL,0,0),0);
			switch(LOWORD(wParam)) {
				case IDC_FONTID:
					if(HIWORD(wParam)!=CBN_SELCHANGE) return FALSE;
					SendMessage(hwndDlg,M_REBUILDFONTGROUP,0,0);
					return 0;
				case IDC_SAMETYPE:
				case IDC_SAMESIZE:
				case IDC_SAMESTYLE:
				case IDC_SAMECOLOUR:
					SendMessage(hwndDlg,M_SAVEFONT,fontId,0);
					SendMessage(hwndDlg,M_RECALCONEFONT,fontId,0);
					SendMessage(hwndDlg,M_REMAKESAMPLE,0,0);
					SendMessage(hwndDlg,M_REFRESHSAMEASBOXES,fontId,0);
					break;
				case IDC_SAMEAS:
					if(HIWORD(wParam)!=CBN_SELCHANGE) return FALSE;
					if(SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_GETCURSEL,0,0),0)==fontId)
						SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_SETCURSEL,0,0);
					if(!SendMessage(GetParent(hwndDlg),PSM_ISEXPERT,0,0)) {
						int sameAs=SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_GETITEMDATA,SendDlgItemMessageA(hwndDlg,IDC_SAMEAS,CB_GETCURSEL,0,0),0);
						if(sameAs!=0xFF) SendMessage(hwndDlg,M_LOADFONT,sameAs,0);
						SendMessage(hwndDlg,M_SAVEFONT,fontId,0);
						SendMessage(hwndDlg,M_GUESSSAMEASBOXES,fontId,0);
					}
					else SendMessage(hwndDlg,M_SAVEFONT,fontId,0);
					SendMessage(hwndDlg,M_RECALCONEFONT,fontId,0);
					SendMessage(hwndDlg,M_FILLSCRIPTCOMBO,fontId,0);
					SendMessage(hwndDlg,M_REMAKESAMPLE,0,0);
					SendMessage(hwndDlg,M_REFRESHSAMEASBOXES,fontId,0);
					break;
				case IDC_TYPEFACE:
				case IDC_SCRIPT:
				case IDC_FONTSIZE:
					if(HIWORD(wParam)!=CBN_EDITCHANGE && HIWORD(wParam)!=CBN_SELCHANGE) return FALSE;
					if(HIWORD(wParam)==CBN_SELCHANGE) {
						SendDlgItemMessageA(hwndDlg,LOWORD(wParam),CB_SETCURSEL,SendDlgItemMessageA(hwndDlg,LOWORD(wParam),CB_GETCURSEL,0,0),0);
					}
					if(LOWORD(wParam)==IDC_TYPEFACE)
						SendMessage(hwndDlg,M_FILLSCRIPTCOMBO,fontId,0);
					//fall through
				case IDC_BOLD:
				case IDC_ITALIC:
                case IDC_UNDERLINE:
                case IDC_COLOUR:
					SendMessage(hwndDlg,M_SAVEFONT,fontId,0);
					if(!SendMessage(GetParent(hwndDlg),PSM_ISEXPERT,0,0)) {
						SendMessage(hwndDlg,M_GUESSSAMEASBOXES,fontId,0);
						SendMessage(hwndDlg,M_REFRESHSAMEASBOXES,fontId,0);
					}
					SendMessage(hwndDlg,M_RECALCOTHERFONTS,fontId,0);
					SendMessage(hwndDlg,M_REMAKESAMPLE,0,0);
					SendMessage(hwndDlg,M_REDOROWHEIGHT,0,0);
					break;
                case IDC_USEINDIVIDUALBKG:
                    EnableWindow(GetDlgItem(hwndDlg, IDC_BKGOUTGOING), IsDlgButtonChecked(hwndDlg, IDC_USEINDIVIDUALBKG));
                    EnableWindow(GetDlgItem(hwndDlg, IDC_BKGINCOMING), IsDlgButtonChecked(hwndDlg, IDC_USEINDIVIDUALBKG));
                    break;
				case IDC_SAMPLE:
					return 0;
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom) {
				case 0:
					switch (((LPNMHDR)lParam)->code)
					{
						case PSN_APPLY:
							{	int i;
								char str[20];
                                HDC hdc = GetDC(NULL);
                                
								for(i=0;i<FONTS_TO_CONFIG;i++) {
									sprintf(str,"Font%d",i);
									DBWriteContactSettingString(NULL,SRFONTSETTINGMODULE,str,fontSettings[i].szFace);
									sprintf(str,"Font%dSet",i);
									DBWriteContactSettingByte(NULL,SRFONTSETTINGMODULE,str,fontSettings[i].charset);
									sprintf(str,"Font%dSize",i);
									DBWriteContactSettingByte(NULL,SRFONTSETTINGMODULE,str, (char)-MulDiv(fontSettings[i].size, GetDeviceCaps(hdc, LOGPIXELSY), 72));
									sprintf(str,"Font%dSty",i);
									DBWriteContactSettingByte(NULL,SRFONTSETTINGMODULE,str,fontSettings[i].style);
									sprintf(str,"Font%dCol",i);
									DBWriteContactSettingDword(NULL,SRFONTSETTINGMODULE,str,fontSettings[i].colour);
									sprintf(str,"Font%dAs",i);
									DBWriteContactSettingWord(NULL,SRFONTSETTINGMODULE,str,(WORD)((fontSettings[i].sameAsFlags<<8)|fontSettings[i].sameAs));
                                    if(i == MSGFONTID_MESSAGEAREA) {
                                        sprintf(str,"Font%d",0);
                                        DBWriteContactSettingString(NULL,"SRMsg",str,fontSettings[i].szFace);
                                        sprintf(str,"Font%dSet",0);
                                        DBWriteContactSettingByte(NULL,"SRMsg",str,fontSettings[i].charset);
                                        sprintf(str,"Font%dSize",0);
                                        DBWriteContactSettingByte(NULL,"SRMsg",str, (char)-MulDiv(fontSettings[i].size, GetDeviceCaps(hdc, LOGPIXELSY), 72));
                                        sprintf(str,"Font%dSty",0);
                                        DBWriteContactSettingByte(NULL,"SRMsg",str,fontSettings[i].style);
                                        sprintf(str,"Font%dCol",0);
                                        DBWriteContactSettingDword(NULL,"SRMsg",str,fontSettings[i].colour);
                                        sprintf(str,"Font%dAs",0);
                                        DBWriteContactSettingWord(NULL,"SRMsg",str,(WORD)((fontSettings[i].sameAsFlags<<8)|fontSettings[i].sameAs));
                                    }
								}
                                ReleaseDC(NULL, hdc);
                                DBWriteContactSettingDword(NULL, FONTMODULE, SRMSGSET_BKGCOLOUR, SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR, 0, 0));
                                DBWriteContactSettingDword(NULL, FONTMODULE, "inputbg", SendDlgItemMessage(hwndDlg, IDC_INPUTBKG, CPM_GETCOLOUR, 0, 0));
                                DBWriteContactSettingDword(NULL, FONTMODULE, "inbg", SendDlgItemMessage(hwndDlg, IDC_BKGINCOMING, CPM_GETCOLOUR, 0, 0));
                                DBWriteContactSettingDword(NULL, FONTMODULE, "outbg", SendDlgItemMessage(hwndDlg, IDC_BKGOUTGOING, CPM_GETCOLOUR, 0, 0));
                                DBWriteContactSettingDword(NULL, FONTMODULE, "hgrid", SendDlgItemMessage(hwndDlg, IDC_GRIDLINES, CPM_GETCOLOUR, 0, 0));
                                DBWriteContactSettingDword(NULL, FONTMODULE, "ipfieldsbg", SendDlgItemMessage(hwndDlg, IDC_INFOPANELBG, CPM_GETCOLOUR, 0, 0));
                                
                                ReloadGlobals();
                                CacheMsgLogIcons();
                                CacheLogFonts();
                                WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 1, 0);
                         }
							return TRUE;
						case PSN_EXPERTCHANGED:
							SwitchTextDlgToMode(hwndDlg,((PSHNOTIFY*)lParam)->lParam);
							break;
					}
					break;
			}
			break;
		case WM_DESTROY:
			if(hFontSample) {
				SendDlgItemMessageA(hwndDlg,IDC_SAMPLE,WM_SETFONT,SendDlgItemMessageA(hwndDlg,IDC_FONTID,WM_GETFONT,0,0),0);
				DeleteObject(hFontSample);
			}
			break;
	}
	return FALSE;
}
    
static int OptInitialise(WPARAM wParam, LPARAM lParam)
{
    OPTIONSDIALOGPAGE odp = { 0 };

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

    if(!ServiceExists(MS_FONT_REGISTER)) {
        odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_MSGWINDOWFONTS);
        odp.pszTitle = Translate("Fonts and colors");
        odp.pfnDlgProc = DlgProcMsgWindowFonts;
        odp.nIDBottomSimpleControl = 0;
        CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) &odp);
    }

    odp.pszTemplate = MAKEINTRESOURCEA(IDD_POPUP_OPT);
    odp.pszTitle = Translate("Event notifications");
    odp.pfnDlgProc = DlgProcPopupOpts;
    odp.nIDBottomSimpleControl = 0;
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
         GetClientRect(hwnd, &rcClient);

		 iInit = TRUE;
         tci.mask = TCIF_PARAM|TCIF_TEXT;
         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_MSGDLG), hwnd, DlgProcOptions);
         tci.pszText = TranslateT("General");
			TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 0, &tci);
         MoveWindow((HWND)tci.lParam,5,26,rcClient.right-8,rcClient.bottom-29,1);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_TABBEDMSG),hwnd,DlgProcTabbedOptions);
         tci.pszText = TranslateT("Tabs and layout");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 2, &tci);
         MoveWindow((HWND)tci.lParam,5,26,rcClient.right-8,rcClient.bottom-29,1);
         ShowWindow((HWND)tci.lParam, SW_HIDE);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_CONTAINERS),hwnd,DlgProcContainerSettings);
         tci.pszText = TranslateT("Containers");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 3, &tci);
         MoveWindow((HWND)tci.lParam,5,26,rcClient.right-8,rcClient.bottom-29,1);
         ShowWindow((HWND)tci.lParam, SW_HIDE);

         tci.lParam = (LPARAM)CreateDialog(g_hInst,MAKEINTRESOURCE(IDD_OPT_MSGLOG),hwnd,DlgProcLogOptions);
         tci.pszText = TranslateT("Message log");
         TabCtrl_InsertItem(GetDlgItem(hwnd, IDC_OPTIONSTAB), 4, &tci);
         MoveWindow((HWND)tci.lParam,5,26,rcClient.right-8,rcClient.bottom-29,1);
         ShowWindow((HWND)tci.lParam, SW_HIDE);
         // add more tabs here if needed
         // activate the final tab
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
                     }
                  break;
               }
            break;

         }
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
     myGlobals.m_SmileyPluginEnabled = (int)DBGetContactSettingByte(NULL, "SmileyAdd", "PluginSupportEnabled", 0);
     myGlobals.m_SendOnShiftEnter = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "sendonshiftenter", 1);
     myGlobals.m_SendOnEnter = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, SRMSGSET_SENDONENTER, SRMSGDEFSET_SENDONENTER);
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
     myGlobals.m_FixFutureTimestamps = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "no_future", 0);
     myGlobals.m_RTLDefault = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "rtldefault", 0);
     myGlobals.m_SplitterSaveOnClose = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "splitsavemode", 1);
     myGlobals.m_WheelDefault = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "no_wheelhack", 0);
     myGlobals.m_MathModAvail = 0;
     myGlobals.m_WinVerMajor = WinVerMajor();
     myGlobals.m_WinVerMinor = WinVerMinor();
     myGlobals.m_SideBarEnabled = (BYTE)DBGetContactSettingByte(NULL, SRMSGMOD_T, "sidebar", 0);
     myGlobals.m_TabAppearance = (int)DBGetContactSettingDword(NULL, SRMSGMOD_T, "tabconfig", TCF_FLASHICON);
     myGlobals.m_ExtraRedraws = (BYTE)DBGetContactSettingByte(NULL, SRMSGMOD_T, "aggromode", 0);
     myGlobals.m_panelHeight = (DWORD)DBGetContactSettingDword(NULL, SRMSGMOD_T, "panelheight", 51);
     myGlobals.m_Send7bitStrictAnsi = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "7bitasANSI", 1);
     myGlobals.m_IdleDetect = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "detectidle", 1);
     myGlobals.m_DoStatusMsg = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "dostatusmsg", 1);
     myGlobals.m_smcxicon = GetSystemMetrics(SM_CXSMICON);
     myGlobals.m_smcyicon = GetSystemMetrics(SM_CYSMICON);
     myGlobals.g_WantIEView = ServiceExists(MS_IEVIEW_WINDOW) && DBGetContactSettingByte(NULL, SRMSGMOD_T, "want_ieview", 0);
     myGlobals.m_PasteAndSend = (int)DBGetContactSettingByte(NULL, SRMSGMOD_T, "pasteandsend", 0);
     myGlobals.m_szNoStatus = TranslateT("No status message available");
     myGlobals.ipConfig.borderStyle = (BYTE)DBGetContactSettingByte(NULL, SRMSGMOD_T, "ipfieldborder", IPFIELD_SUNKEN);
	 myGlobals.bAvatarBoderType = (BYTE)DBGetContactSettingByte(NULL, SRMSGMOD_T, "avbordertype", 1);
     myGlobals.m_dropShadow = (BYTE)DBGetContactSettingByte(NULL, SRMSGMOD_T, "dropshadow", 0);

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
}

/*
 * called at startup. checks for old font path and moves it to 
 * the new settings group to make them usable with font service
 */

void MoveFonts()
{
    char szTemp[128];
    int i = 0;
    DBVARIANT dbv;
    HDC hdc;
    char bSize = 0;
    int charset;
    
    if(DBGetContactSettingByte(NULL, SRMSGMOD_T, "fontsmoved", -1) != -1)
        return;         // already done

    DBWriteContactSettingByte(NULL, SRMSGMOD_T, "fontsmoved", 1);
    
    hdc = GetDC(NULL);
    
    for(i = 0; i < MSGDLGFONTCOUNT; i++) {
        _snprintf(szTemp, sizeof(szTemp), "Font%d", i);
        if(!DBGetContactSetting(NULL, SRMSGMOD_T, szTemp, &dbv)) {
            if(i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT)
                DBWriteContactSettingString(NULL, FONTMODULE, szTemp, "Arial");
            else
                DBWriteContactSettingString(NULL, FONTMODULE, szTemp, dbv.pszVal);
            DBDeleteContactSetting(NULL, SRMSGMOD_T, szTemp);
            DBFreeVariant(&dbv);
        }
        else
            DBWriteContactSettingString(NULL, FONTMODULE, szTemp, "Arial");
            
        _snprintf(szTemp, sizeof(szTemp), "Font%dCol", i);
        DBWriteContactSettingDword(NULL, FONTMODULE, szTemp, DBGetContactSettingDword(NULL, SRMSGMOD_T, szTemp, GetSysColor(COLOR_BTNTEXT)));
        DBDeleteContactSetting(NULL, SRMSGMOD_T, szTemp);
        
        _snprintf(szTemp, sizeof(szTemp), "Font%dSize", i);
        bSize = (char)DBGetContactSettingByte(NULL, SRMSGMOD_T, szTemp, 10);
        if(bSize > 0)
            bSize = -MulDiv(bSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        DBWriteContactSettingByte(NULL, FONTMODULE, szTemp, bSize);
        DBDeleteContactSetting(NULL, SRMSGMOD_T, szTemp);

        _snprintf(szTemp, sizeof(szTemp), "Font%dSty", i);
        DBWriteContactSettingByte(NULL, FONTMODULE, szTemp, DBGetContactSettingByte(NULL, SRMSGMOD_T, szTemp, 0));
        DBDeleteContactSetting(NULL, SRMSGMOD_T, szTemp);

        _snprintf(szTemp, sizeof(szTemp), "Font%dSet", i);
        charset = DBGetContactSettingByte(NULL, SRMSGMOD_T, szTemp, 0);
        if(i == MSGFONTID_SYMBOLS_IN || i == MSGFONTID_SYMBOLS_OUT)
            charset = 0;
        DBWriteContactSettingByte(NULL, FONTMODULE, szTemp, charset);
        DBDeleteContactSetting(NULL, SRMSGMOD_T, szTemp);

        _snprintf(szTemp, sizeof(szTemp), "Font%dAs", i);
        DBWriteContactSettingWord(NULL, FONTMODULE, szTemp, DBGetContactSettingWord(NULL, SRMSGMOD_T, szTemp, 0));
        DBDeleteContactSetting(NULL, SRMSGMOD_T, szTemp);
    }
    ReleaseDC(NULL, hdc);
    i = 0;

    while(colornames[i].szName!= NULL) {
        DBWriteContactSettingDword(NULL, FONTMODULE, colornames[i].szSetting, DBGetContactSettingDword(NULL, SRMSGMOD_T, colornames[i].szSetting, RGB(224, 224, 224)));
        DBDeleteContactSetting(NULL, SRMSGMOD_T, colornames[i].szSetting);
        i++;
    }
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