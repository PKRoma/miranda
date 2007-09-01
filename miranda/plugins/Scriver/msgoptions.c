/*
Scriver

Copyright 2000-2005 Miranda ICQ/IM project,
Copyright 2005 Piotr Piastucki

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

extern HINSTANCE g_hInst;
extern HANDLE hEventOptInitialise;
extern HANDLE *hMsgMenuItem;
extern int hMsgMenuItemCount;
extern void ChangeStatusIcons();
extern int    Chat_FontsChanged(WPARAM wParam,LPARAM lParam);
extern int    Chat_IconsChanged(WPARAM wParam,LPARAM lParam);
extern HANDLE hEventSkin2IconsChanged;
extern int    Chat_SmileyOptionsChanged(WPARAM wParam,LPARAM lParam);

static BOOL CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcContainerOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcLogOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcOptions1(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcOptions2(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

typedef struct TabDefStruct {
	DLGPROC dlgProc;
	DWORD dlgId;
	TCHAR *tabName;
} TabDef;

static TabDef tabPages[] = {
						 {DlgProcContainerOptions, IDD_OPT_CONTAINER, _T("Containers")},
						 {DlgProcOptions, IDD_OPT_MSGDLG, _T("Messaging")},
						 {DlgProcLogOptions, IDD_OPT_MSGLOG, _T("Messaging Log")},
						 {DlgProcOptions1, IDD_OPTIONS1, _T("Chat")},
						 {DlgProcOptions2, IDD_OPTIONS2, _T("Chat Log")}
						 };


#define FONTF_BOLD   1
#define FONTF_ITALIC 2
struct FontOptionsList
{
	TCHAR *szDescr;
	COLORREF defColour;
	TCHAR *szDefFace;
	BYTE defStyle;
	char defSize;
	COLORREF colour;
}
static fontOptionsList[] = {
	{LPGENT("Outgoing messages"), RGB(106, 106, 106), _T("Arial"), 0, -12},
	{LPGENT("Incoming messages"), RGB(0, 0, 0), _T("Arial"), 0, -12},
	{LPGENT("Outgoing name"), RGB(89, 89, 89), _T("Arial"), FONTF_BOLD, -12},
	{LPGENT("Outgoing time"), RGB(0, 0, 0), _T("Terminal"), FONTF_BOLD, -9},
	{LPGENT("Outgoing colon"), RGB(89, 89, 89), _T("Arial"), 0, -11},
	{LPGENT("Incoming name"), RGB(215, 0, 0), _T("Arial"), FONTF_BOLD, -12},
	{LPGENT("Incoming time"), RGB(0, 0, 0), _T("Terminal"), FONTF_BOLD, -9},
	{LPGENT("Incoming colon"), RGB(215, 0, 0), _T("Arial"), 0, -11},
	{LPGENT("Message area"), RGB(0, 0, 0), _T("Arial"), 0, -12},
	{LPGENT("Notices"), RGB(90, 90, 160), _T("Arial"), 0, -12},
	{LPGENT("Outgoing URL"), RGB(0, 0, 255), _T("Arial"), 0, -12},
	{LPGENT("Incoming URL"), RGB(0, 0, 255), _T("Arial"), 0, -12},
};

int fontOptionsListSize = SIZEOF(fontOptionsList);

int Chat_FontsChanged(WPARAM wParam,LPARAM lParam);

int FontServiceFontsChanged(WPARAM wParam, LPARAM lParam)
{
	WindowList_Broadcast(g_dat->hMessageWindowList, DM_OPTIONSAPPLIED, 0, 0);
	Chat_FontsChanged(wParam, lParam);
	return 0;
}

#if defined( _UNICODE )
static BYTE MsgDlgGetFontDefaultCharset(const TCHAR* szFont)
{
  return DEFAULT_CHARSET;
}
#else
// get font charset according to current CP
static BYTE MsgDlgGetCPDefaultCharset()
{
	switch (GetACP()) {
		case 1250:
			return EASTEUROPE_CHARSET;
		case 1251:
			return RUSSIAN_CHARSET;
		case 1252:
			return ANSI_CHARSET;
		case 1253:
			return GREEK_CHARSET;
		case 1254:
			return TURKISH_CHARSET;
		case 1255:
			return HEBREW_CHARSET;
		case 1256:
			return ARABIC_CHARSET;
		case 1257:
			return BALTIC_CHARSET;
		case 1361:
			return JOHAB_CHARSET;
		case 874:
			return THAI_CHARSET;
		case 932:
			return SHIFTJIS_CHARSET;
		case 936:
			return GB2312_CHARSET;
		case 949:
			return HANGEUL_CHARSET;
		case 950:
			return CHINESEBIG5_CHARSET;
		default:
			return DEFAULT_CHARSET;
	}
}

static int CALLBACK EnumFontFamExProc(const LOGFONT *lpelfe, const TEXTMETRIC *lpntme, DWORD FontType, LPARAM lParam)
{
	*(int*)lParam = 1;
	return 0;
}

// get font charset according to current CP, if available for specified font
static BYTE MsgDlgGetFontDefaultCharset(const TCHAR* szFont)
{
	HDC hdc;
	LOGFONT lf = {0};
	int found = 0;

	_tcscpy(lf.lfFaceName, szFont);
	lf.lfCharSet = MsgDlgGetCPDefaultCharset();

	// check if the font supports specified charset
	hdc = GetDC(0);
	EnumFontFamiliesEx(hdc, &lf, &EnumFontFamExProc, (LPARAM)&found, 0);
	ReleaseDC(0, hdc);

	if (found)
		return lf.lfCharSet;
	else // no, give default
		return DEFAULT_CHARSET;
}
#endif

void RegisterFontServiceFonts() {
	if (ServiceExists(MS_FONT_REGISTER)) {
		int i;
		char szTemp[100];
		LOGFONT lf;
		FontIDT fid = {0};
		ColourIDT cid = {0};
		fid.cbSize = sizeof(FontIDT);
		_tcsncpy(fid.group, _T("Scriver/Single Messaging"), SIZEOF(fid.group));
		strncpy(fid.dbSettingsGroup, (SRMMMOD), SIZEOF(fid.dbSettingsGroup));
		fid.flags = FIDF_DEFAULTVALID;
		for (i = 0; i < SIZEOF(fontOptionsList); i++) {
			LoadMsgDlgFont(i, &lf, &fontOptionsList[i].colour);
			mir_snprintf(szTemp, SIZEOF(szTemp), "SRMFont%d", i);
			strncpy(fid.prefix, szTemp, SIZEOF(fid.prefix));
			fid.order = i;
			_tcsncpy(fid.name, fontOptionsList[i].szDescr, SIZEOF(fid.name));
			fid.deffontsettings.colour = fontOptionsList[i].colour;
			fid.deffontsettings.size = (char) lf.lfHeight;
			fid.deffontsettings.style = (lf.lfWeight >= FW_BOLD ? FONTF_BOLD : 0) | (lf.lfItalic ? FONTF_ITALIC : 0);
			fid.deffontsettings.charset = lf.lfCharSet;
			_tcsncpy(fid.deffontsettings.szFace, lf.lfFaceName, LF_FACESIZE);
			_tcsncpy(fid.backgroundGroup, _T("Scriver/Single Messaging"), SIZEOF(fid.backgroundGroup));
			switch (i) {
			case MSGFONTID_MYMSG:
			case MSGFONTID_MYNAME:
			case MSGFONTID_MYTIME:
			case MSGFONTID_MYCOLON:
			case MSGFONTID_MYURL:
				_tcsncpy(fid.backgroundName, _T("Outgoing background"), SIZEOF(fid.backgroundName));
				break;
			case MSGFONTID_MESSAGEAREA:
				_tcsncpy(fid.backgroundName, _T("Input area background"), SIZEOF(fid.backgroundName));
				break;
			default:
				_tcsncpy(fid.backgroundName, _T("Incoming background"), SIZEOF(fid.backgroundName));
				break;
			}
			CallService(MS_FONT_REGISTERT, (WPARAM)&fid, 0);
		}
		cid.cbSize = sizeof(ColourIDT);
		_tcsncpy(cid.group, _T("Scriver/Single Messaging"), SIZEOF(fid.group));
		strncpy(cid.dbSettingsGroup, (SRMMMOD), SIZEOF(fid.dbSettingsGroup));
		cid.flags = 0;
		cid.order = 0;
		_tcsncpy(cid.name, _T("Background"), SIZEOF(cid.name));
		strncpy(cid.setting, (SRMSGSET_BKGCOLOUR), SIZEOF(cid.setting));
		cid.defcolour = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_BKGCOLOUR, SRMSGDEFSET_BKGCOLOUR);
		CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);

		cid.order = 1;
		_tcsncpy(cid.name, _T("Input area background"), SIZEOF(cid.name));
		strncpy(cid.setting, (SRMSGSET_INPUTBKGCOLOUR), SIZEOF(cid.setting));
		cid.defcolour = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_INPUTBKGCOLOUR, SRMSGDEFSET_INPUTBKGCOLOUR);
		CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);

		cid.order = 2;
		_tcsncpy(cid.name, _T("Incoming background"), SIZEOF(cid.name));
		strncpy(cid.setting, (SRMSGSET_INCOMINGBKGCOLOUR), SIZEOF(cid.setting));
		cid.defcolour = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_INCOMINGBKGCOLOUR, SRMSGDEFSET_INCOMINGBKGCOLOUR);
		CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);

		cid.order = 3;
		_tcsncpy(cid.name, _T("Outgoing background"), SIZEOF(cid.name));
		strncpy(cid.setting, (SRMSGSET_OUTGOINGBKGCOLOUR), SIZEOF(cid.setting));
		cid.defcolour = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_OUTGOINGBKGCOLOUR, SRMSGDEFSET_OUTGOINGBKGCOLOUR);
		CallService(MS_COLOUR_REGISTERT, (WPARAM)&cid, 0);
	}
}

int IconsChanged(WPARAM wParam, LPARAM lParam)
{
	if ( hMsgMenuItem && !ServiceExists( MS_SKIN2_GETICONBYHANDLE )) {
		int j;
		CLISTMENUITEM mi;
		mi.cbSize = sizeof(mi);
		mi.flags = CMIM_ICON;
		mi.hIcon = LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
		for (j = 0; j < hMsgMenuItemCount; j++) {
			CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hMsgMenuItem[j], (LPARAM) & mi);
		}
		CallService(MS_SKIN2_RELEASEICON,(WPARAM)mi.hIcon, 0);
	}
	FreeMsgLogIcons();
	LoadMsgLogIcons();
	ChangeStatusIcons();
	WindowList_Broadcast(g_dat->hMessageWindowList, DM_REMAKELOG, 0, 0);
	// change all the icons
	WindowList_Broadcast(g_dat->hMessageWindowList, DM_CHANGEICONS, 0, 0);
	WindowList_Broadcast(g_dat->hMessageWindowList, DM_UPDATETITLEBAR, 0, 0);
	Chat_IconsChanged(wParam, lParam);
	return 0;
}

int SmileySettingsChanged(WPARAM wParam, LPARAM lParam)
{
	WindowList_Broadcast(g_dat->hMessageWindowList, DM_REMAKELOG, wParam, 0);
	Chat_SmileyOptionsChanged(wParam, lParam);
	return 0;
}

int IcoLibIconsChanged(WPARAM wParam, LPARAM lParam)
{
	ReleaseGlobalIcons();
	LoadGlobalIcons();
	return IconsChanged(wParam, lParam);
}

void RegisterIcoLibIcons() {
	hEventSkin2IconsChanged = HookEvent_Ex(ME_SKIN2_ICONSCHANGED, IcoLibIconsChanged);
	if (hEventSkin2IconsChanged) {
		SKINICONDESC sid = { 0 };
		TCHAR path[MAX_PATH];
		GetModuleFileName(g_hInst, path, MAX_PATH);
		sid.cbSize = sizeof(SKINICONDESC);
		sid.cx = sid.cy = 16;
		sid.flags = SIDF_ALL_TCHAR;
		sid.ptszSection = LPGENT("Scriver/Messaging");
		sid.ptszDefaultFile = path;
		sid.pszName = (char *) "scriver_ADD";
		sid.iDefaultIndex = -IDI_ADDCONTACT;
		sid.ptszDescription = LPGENT("Add contact");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
		sid.pszName = (char *) "scriver_USERDETAILS";
		sid.iDefaultIndex = -IDI_USERDETAILS;
		sid.ptszDescription = LPGENT("User's details");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
		sid.pszName = (char *) "scriver_HISTORY";
		sid.iDefaultIndex = -IDI_HISTORY;
		sid.ptszDescription = LPGENT("User's history");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
		sid.pszName = (char *) "scriver_SEND";
		sid.iDefaultIndex = -IDI_SEND;
		sid.ptszDescription = LPGENT("Send message");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
		sid.pszName = (char *) "scriver_CANCEL";
		sid.iDefaultIndex = -IDI_CANCEL;
		sid.ptszDescription = LPGENT("Close session");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
		sid.pszName = (char *) "scriver_SMILEY";
		sid.iDefaultIndex = -IDI_SMILEY;
		sid.ptszDescription = LPGENT("Smiley button");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
		sid.pszName = (char *) "scriver_TYPING";
		sid.iDefaultIndex = -IDI_TYPING;
		sid.ptszDescription = LPGENT("User is typing");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);

		sid.pszName = (char *) "scriver_UNICODEON";
		sid.iDefaultIndex = -IDI_UNICODEON;
		sid.ptszDescription = LPGENT("Unicode is on");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
		sid.pszName = (char *) "scriver_UNICODEOFF";
		sid.iDefaultIndex = -IDI_UNICODEOFF;
		sid.ptszDescription = LPGENT("Unicode is off");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);

		sid.pszName = (char *) "scriver_DELIVERING";
		sid.iDefaultIndex = -IDI_TIMESTAMP;
		sid.ptszDescription = LPGENT("Sending");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);

		sid.pszName = (char *) "scriver_QUOTE";
		sid.iDefaultIndex = -IDI_QUOTE;
		sid.ptszDescription = LPGENT("Quote button");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);

		sid.pszName = (char *) "scriver_CLOSEX";
		sid.iDefaultIndex = -IDI_CLOSEX;
		sid.ptszDescription = LPGENT("Close button");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);

		sid.pszName = (char *) "scriver_OVERLAY";
		sid.iDefaultIndex = -IDI_OVERLAY;
		sid.ptszDescription = LPGENT("Icon overlay");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);

		sid.pszName = (char *) "scriver_INCOMING";
		sid.iDefaultIndex = -IDI_INCOMING;
		sid.ptszDescription = LPGENT("Incoming message (10x10)");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
		sid.pszName = (char *) "scriver_OUTGOING";
		sid.iDefaultIndex = -IDI_OUTGOING;
		sid.ptszDescription = LPGENT("Outgoing message (10x10)");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
		sid.pszName = (char *) "scriver_NOTICE";
		sid.iDefaultIndex = -IDI_NOTICE;
		sid.ptszDescription = LPGENT("Notice (10x10)");
		CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
	}
}

void LoadMsgDlgFont(int i, LOGFONT * lf, COLORREF * colour)
{
	char str[32];
	int style;
	DBVARIANT dbv;

	if (colour) {
		wsprintfA(str, "SRMFont%dCol", i);
		*colour = DBGetContactSettingDword(NULL, SRMMMOD, str, fontOptionsList[i].defColour);
	}
	if (lf) {
		wsprintfA(str, "SRMFont%dSize", i);
		lf->lfHeight = (char) DBGetContactSettingByte(NULL, SRMMMOD, str, fontOptionsList[i].defSize);
		lf->lfWidth = 0;
		lf->lfEscapement = 0;
		lf->lfOrientation = 0;
		wsprintfA(str, "SRMFont%dSty", i);
		style = DBGetContactSettingByte(NULL, SRMMMOD, str, fontOptionsList[i].defStyle);
		lf->lfWeight = style & FONTF_BOLD ? FW_BOLD : FW_NORMAL;
		lf->lfItalic = style & FONTF_ITALIC ? 1 : 0;
		lf->lfUnderline = 0;
		lf->lfStrikeOut = 0;
		lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
		lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lf->lfQuality = DEFAULT_QUALITY;
		lf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		wsprintfA(str, "SRMFont%d", i);
		if (DBGetContactSettingTString(NULL, SRMMMOD, str, &dbv))
			lstrcpy(lf->lfFaceName, fontOptionsList[i].szDefFace);
		else {
			_tcsncpy(lf->lfFaceName, dbv.ptszVal, SIZEOF(lf->lfFaceName));
			DBFreeVariant(&dbv);
		}
		wsprintfA(str, "SRMFont%dSet", i);
		lf->lfCharSet = DBGetContactSettingByte(NULL, SRMMMOD, str, MsgDlgGetFontDefaultCharset(lf->lfFaceName));
	}
}

struct CheckBoxValues_t
{
    DWORD  style;
    TCHAR* szDescr;
}

static const statusValues[] =
{
	{ MODEF_OFFLINE,  LPGENT("Offline")       },
	{ PF2_ONLINE,     LPGENT("Online")        },
	{ PF2_SHORTAWAY,  LPGENT("Away")          },
	{ PF2_LONGAWAY,   LPGENT("NA")            },
	{ PF2_LIGHTDND,   LPGENT("Occupied")      },
	{ PF2_HEAVYDND,   LPGENT("DND")           },
	{ PF2_FREECHAT,   LPGENT("Free for chat") },
	{ PF2_INVISIBLE,  LPGENT("Invisible")     },
	{ PF2_OUTTOLUNCH, LPGENT("Out to lunch")  },
	{ PF2_ONTHEPHONE, LPGENT("On the phone")  }
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
		TreeView_InsertItem( hwndTree, &tvis );
}	}

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

static int changed = 0;

static void ApplyChanges(int i) {
	changed &= ~i;
	if (changed == 0) {
		ReloadGlobals();
		WindowList_Broadcast(g_dat->hParentWindowList, DM_OPTIONSAPPLIED, 0, 0);
		WindowList_Broadcast(g_dat->hMessageWindowList, DM_OPTIONSAPPLIED, 0, 0);
	}
}

static void MarkChanges(int i, HWND hWnd) {
	SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
	changed |= i;
}

static BOOL CALLBACK DlgProcContainerOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
		{
			int limitLength;
			int bChecked;
			char str[10];
			DWORD msgTimeout, avatarHeight;
			TranslateDialogDefault(hwndDlg);
			SetWindowLong(GetDlgItem(hwndDlg, IDC_POPLIST), GWL_STYLE, (GetWindowLong(GetDlgItem(hwndDlg, IDC_POPLIST), GWL_STYLE) & ~WS_BORDER) | TVS_NOHSCROLL | TVS_CHECKBOXES);
			FillCheckBoxTree(GetDlgItem(hwndDlg, IDC_POPLIST), statusValues, sizeof(statusValues) / sizeof(statusValues[0]),
                             DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_POPFLAGS, SRMSGDEFSET_POPFLAGS));
			CheckDlgButton(hwndDlg, IDC_SHOWBUTTONLINE, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWBUTTONLINE, SRMSGDEFSET_SHOWBUTTONLINE));
			CheckDlgButton(hwndDlg, IDC_AUTOPOPUP, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOPOPUP, SRMSGDEFSET_AUTOPOPUP));
			CheckDlgButton(hwndDlg, IDC_STAYMINIMIZED, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_STAYMINIMIZED, SRMSGDEFSET_STAYMINIMIZED));
			CheckDlgButton(hwndDlg, IDC_AUTOMIN, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOMIN, SRMSGDEFSET_AUTOMIN));
			CheckDlgButton(hwndDlg, IDC_AUTOCLOSE, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOCLOSE, SRMSGDEFSET_AUTOCLOSE));
			CheckDlgButton(hwndDlg, IDC_SAVEPERCONTACT, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SAVEPERCONTACT, SRMSGDEFSET_SAVEPERCONTACT));
			CheckDlgButton(hwndDlg, IDC_SAVESPLITTERPERCONTACT, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SAVESPLITTERPERCONTACT, SRMSGDEFSET_SAVESPLITTERPERCONTACT));
			CheckDlgButton(hwndDlg, IDC_CASCADE, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_CASCADE, SRMSGDEFSET_CASCADE));
			CheckDlgButton(hwndDlg, IDC_SENDONENTER, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDONENTER, SRMSGDEFSET_SENDONENTER));
			CheckDlgButton(hwndDlg, IDC_SENDONDBLENTER, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDONDBLENTER, SRMSGDEFSET_SENDONDBLENTER));
			CheckDlgButton(hwndDlg, IDC_STATUSWIN, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_STATUSICON, SRMSGDEFSET_STATUSICON));
			CheckDlgButton(hwndDlg, IDC_SAVEDRAFTS, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SAVEDRAFTS, SRMSGDEFSET_SAVEDRAFTS));
			CheckDlgButton(hwndDlg, IDC_SHOWSTATUSBAR, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWSTATUSBAR, SRMSGDEFSET_SHOWSTATUSBAR));
			CheckDlgButton(hwndDlg, IDC_SHOWTITLEBAR, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTITLEBAR, SRMSGDEFSET_SHOWTITLEBAR));
			CheckDlgButton(hwndDlg, IDC_SHOWPROGRESS, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWPROGRESS, SRMSGDEFSET_SHOWPROGRESS));
			CheckDlgButton(hwndDlg, IDC_USETABS, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_USETABS, SRMSGDEFSET_USETABS));
			CheckDlgButton(hwndDlg, IDC_TABSATBOTTOM, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_TABSATBOTTOM, SRMSGDEFSET_TABSATBOTTOM));
			CheckDlgButton(hwndDlg, IDC_SWITCHTOACTIVE, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SWITCHTOACTIVE, SRMSGDEFSET_SWITCHTOACTIVE));
			CheckDlgButton(hwndDlg, IDC_LIMITNAMES, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_LIMITNAMES, SRMSGDEFSET_LIMITNAMES));
			limitLength = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_LIMITNAMESLEN, SRMSGDEFSET_LIMITNAMESLEN);
			SetDlgItemInt(hwndDlg, IDC_LIMITNAMESLEN, limitLength >= SRMSGSET_LIMITNAMESLEN_MIN ? limitLength : SRMSGDEFSET_LIMITNAMESLEN, FALSE);

			CheckDlgButton(hwndDlg, IDC_LIMITTABS, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_LIMITTABS, SRMSGDEFSET_LIMITTABS));
			limitLength = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_LIMITTABSNUM, SRMSGDEFSET_LIMITTABSNUM);
			SetDlgItemInt(hwndDlg, IDC_LIMITTABSNUM, limitLength >= 1 ? limitLength : 1, FALSE);

			CheckDlgButton(hwndDlg, IDC_LIMITCHATSTABS, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_LIMITCHATSTABS, SRMSGDEFSET_LIMITCHATSTABS));
			limitLength = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_LIMITCHATSTABSNUM, SRMSGDEFSET_LIMITCHATSTABSNUM);
			SetDlgItemInt(hwndDlg, IDC_LIMITCHATSTABSNUM, limitLength >= 1 ? limitLength : 1, FALSE);

			CheckDlgButton(hwndDlg, IDC_HIDEONETAB, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_HIDEONETAB, SRMSGDEFSET_HIDEONETAB));
			CheckDlgButton(hwndDlg, IDC_TABCLOSEBUTTON, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_TABCLOSEBUTTON, SRMSGDEFSET_TABCLOSEBUTTON));
			CheckDlgButton(hwndDlg, IDC_SEPARATECHATSCONTAINERS, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SEPARATECHATSCONTAINERS, SRMSGDEFSET_SEPARATECHATSCONTAINERS));
			CheckDlgButton(hwndDlg, IDC_HIDECONTAINERS, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_HIDECONTAINERS, SRMSGDEFSET_HIDECONTAINERS));

			CheckDlgButton(hwndDlg, IDC_TRANSPARENCY, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_USETRANSPARENCY, SRMSGDEFSET_USETRANSPARENCY));
			SendDlgItemMessage(hwndDlg,IDC_ATRANSPARENCYVALUE,TBM_SETRANGE, FALSE, MAKELONG(0,255));
			SendDlgItemMessage(hwndDlg,IDC_ATRANSPARENCYVALUE,TBM_SETPOS, TRUE, DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_ACTIVEALPHA, SRMSGDEFSET_ACTIVEALPHA));
			SendDlgItemMessage(hwndDlg,IDC_ITRANSPARENCYVALUE,TBM_SETRANGE, FALSE, MAKELONG(0,255));
			SendDlgItemMessage(hwndDlg,IDC_ITRANSPARENCYVALUE,TBM_SETPOS, TRUE, DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_INACTIVEALPHA, SRMSGDEFSET_INACTIVEALPHA));
			sprintf(str,"%d%%",(int)(100*SendDlgItemMessage(hwndDlg,IDC_ATRANSPARENCYVALUE,TBM_GETPOS,0,0)/255));
			SetDlgItemTextA(hwndDlg, IDC_ATRANSPARENCYPERC, str);
			sprintf(str,"%d%%",(int)(100*SendDlgItemMessage(hwndDlg,IDC_ITRANSPARENCYVALUE,TBM_GETPOS,0,0)/255));
			SetDlgItemTextA(hwndDlg, IDC_ITRANSPARENCYPERC, str);

			if (pSetLayeredWindowAttributes == NULL) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCY), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ATRANSPARENCYVALUE), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ATRANSPARENCYPERC), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ITRANSPARENCYVALUE), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ITRANSPARENCYPERC), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCYTEXT1), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCYTEXT2), FALSE);
			} else  {
				bChecked = IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENCY);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ATRANSPARENCYVALUE), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ATRANSPARENCYPERC), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ITRANSPARENCYVALUE), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ITRANSPARENCYPERC), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCYTEXT1), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCYTEXT2), bChecked);
			}
			CheckDlgButton(hwndDlg, IDC_AVATARSUPPORT, g_dat->flags&SMF_AVATAR);
//			EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITAVATARH), FALSE);
//			EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHT), FALSE);
			CheckDlgButton(hwndDlg, IDC_LIMITAVATARH, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_LIMITAVHEIGHT, SRMSGDEFSET_LIMITAVHEIGHT));
			avatarHeight = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_AVHEIGHT, SRMSGDEFSET_AVHEIGHT);
			SetDlgItemInt(hwndDlg, IDC_AVATARHEIGHT, avatarHeight, FALSE);
			avatarHeight = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_AVHEIGHTMIN, SRMSGDEFSET_AVHEIGHTMIN);
			SetDlgItemInt(hwndDlg, IDC_AVATARHEIGHTMIN, avatarHeight, FALSE);

			EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITAVATARH), IsDlgButtonChecked(hwndDlg, IDC_AVATARSUPPORT));
			if (!IsDlgButtonChecked(hwndDlg, IDC_AVATARSUPPORT)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHT), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHTMIN), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT1), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT2), FALSE);
			} else {
				bChecked = IsDlgButtonChecked(hwndDlg, IDC_LIMITAVATARH);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHT), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHTMIN), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT1), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT2), bChecked);
			}

			CheckDlgButton(hwndDlg, IDC_CTRLSUPPORT, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_CTRLSUPPORT, SRMSGDEFSET_CTRLSUPPORT));
			CheckDlgButton(hwndDlg, IDC_DELTEMP, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_DELTEMP, SRMSGDEFSET_DELTEMP));
			msgTimeout = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_MSGTIMEOUT, SRMSGDEFSET_MSGTIMEOUT);
			SetDlgItemInt(hwndDlg, IDC_SECONDS, msgTimeout >= SRMSGSET_MSGTIMEOUT_MIN ? msgTimeout / 1000 : SRMSGDEFSET_MSGTIMEOUT / 1000, FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_CTRLSUPPORT), !IsDlgButtonChecked(hwndDlg, IDC_AUTOCLOSE));

			EnableWindow(GetDlgItem(hwndDlg, IDC_STAYMINIMIZED), IsDlgButtonChecked(hwndDlg, IDC_AUTOPOPUP));
			EnableWindow(GetDlgItem(hwndDlg, IDC_POPLIST), IsDlgButtonChecked(hwndDlg, IDC_AUTOPOPUP));
			bChecked = IsDlgButtonChecked(hwndDlg, IDC_USETABS);
			EnableWindow(GetDlgItem(hwndDlg, IDC_SWITCHTOACTIVE), bChecked);
			EnableWindow(GetDlgItem(hwndDlg, IDC_TABSATBOTTOM), bChecked );
			EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITNAMES), bChecked );
			EnableWindow(GetDlgItem(hwndDlg, IDC_HIDEONETAB), bChecked );
			EnableWindow(GetDlgItem(hwndDlg, IDC_TABCLOSEBUTTON), bChecked );
			EnableWindow(GetDlgItem(hwndDlg, IDC_SEPARATECHATSCONTAINERS), bChecked );
			EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITTABS), bChecked );
//			EnableWindow(GetDlgItem(hwndDlg, IDC_CASCADE), !bChecked  );//&& !IsDlgButtonChecked(hwndDlg, IDC_SAVEPERCONTACT));
//			EnableWindow(GetDlgItem(hwndDlg, IDC_SAVEPERCONTACT), !bChecked );
			bChecked = IsDlgButtonChecked(hwndDlg, IDC_USETABS) && IsDlgButtonChecked(hwndDlg, IDC_LIMITNAMES);
			EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITNAMESLEN), bChecked);
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHARS), bChecked);
			bChecked = IsDlgButtonChecked(hwndDlg, IDC_USETABS) && IsDlgButtonChecked(hwndDlg, IDC_LIMITTABS);
			EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITTABSNUM), bChecked );
			bChecked = IsDlgButtonChecked(hwndDlg, IDC_USETABS) && IsDlgButtonChecked(hwndDlg, IDC_SEPARATECHATSCONTAINERS);
			EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITCHATSTABS), bChecked );
			bChecked = bChecked && IsDlgButtonChecked(hwndDlg, IDC_LIMITCHATSTABS);;
			EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITCHATSTABSNUM), bChecked );
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_AUTOPOPUP:
					EnableWindow(GetDlgItem(hwndDlg, IDC_STAYMINIMIZED), IsDlgButtonChecked(hwndDlg, IDC_AUTOPOPUP));
					EnableWindow(GetDlgItem(hwndDlg, IDC_POPLIST), IsDlgButtonChecked(hwndDlg, IDC_AUTOPOPUP));
					break;
				case IDC_AUTOMIN:
					CheckDlgButton(hwndDlg, IDC_AUTOCLOSE, BST_UNCHECKED);
					EnableWindow(GetDlgItem(hwndDlg, IDC_CTRLSUPPORT), !IsDlgButtonChecked(hwndDlg, IDC_AUTOCLOSE));
					break;
				case IDC_USETABS:
					{
						int bChecked = IsDlgButtonChecked(hwndDlg, IDC_USETABS);
						EnableWindow(GetDlgItem(hwndDlg, IDC_SWITCHTOACTIVE), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_TABSATBOTTOM), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITNAMES), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_HIDEONETAB), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_TABCLOSEBUTTON), bChecked );
						EnableWindow(GetDlgItem(hwndDlg, IDC_SEPARATECHATSCONTAINERS), bChecked );
						EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITTABS), bChecked);
//						EnableWindow(GetDlgItem(hwndDlg, IDC_CASCADE), !bChecked );//&& !IsDlgButtonChecked(hwndDlg, IDC_SAVEPERCONTACT));
	//					EnableWindow(GetDlgItem(hwndDlg, IDC_SAVEPERCONTACT), !bChecked);
					}
				case IDC_LIMITTABS:
					{
						int bChecked = IsDlgButtonChecked(hwndDlg, IDC_USETABS) && IsDlgButtonChecked(hwndDlg, IDC_LIMITTABS);
						EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITTABSNUM), bChecked);
					}
				case IDC_SEPARATECHATSCONTAINERS:
					{
						int bChecked = IsDlgButtonChecked(hwndDlg, IDC_USETABS) && IsDlgButtonChecked(hwndDlg, IDC_SEPARATECHATSCONTAINERS);
						EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITCHATSTABS), bChecked);
					}
				case IDC_LIMITCHATSTABS:
					{
						int bChecked = IsDlgButtonChecked(hwndDlg, IDC_USETABS) && IsDlgButtonChecked(hwndDlg, IDC_SEPARATECHATSCONTAINERS) &&
								IsDlgButtonChecked(hwndDlg, IDC_LIMITCHATSTABS);
						EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITCHATSTABSNUM), bChecked);
					}
				case IDC_LIMITNAMES:
					{
						int bChecked = IsDlgButtonChecked(hwndDlg, IDC_LIMITNAMES) && IsDlgButtonChecked(hwndDlg, IDC_USETABS);
						EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITNAMESLEN), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_CHARS), bChecked);
					}
					break;
				case IDC_AUTOCLOSE:
					CheckDlgButton(hwndDlg, IDC_AUTOMIN, BST_UNCHECKED);
					EnableWindow(GetDlgItem(hwndDlg, IDC_CTRLSUPPORT), !IsDlgButtonChecked(hwndDlg, IDC_AUTOCLOSE));
					break;
				case IDC_SENDONENTER:
					CheckDlgButton(hwndDlg, IDC_SENDONDBLENTER, BST_UNCHECKED);
					break;
				case IDC_SENDONDBLENTER:
					CheckDlgButton(hwndDlg, IDC_SENDONENTER, BST_UNCHECKED);
					break;
				case IDC_CASCADE:
					CheckDlgButton(hwndDlg, IDC_SAVEPERCONTACT, BST_UNCHECKED);
					break;
				case IDC_SAVEPERCONTACT:
					CheckDlgButton(hwndDlg, IDC_CASCADE, BST_UNCHECKED);
					//EnableWindow(GetDlgItem(hwndDlg, IDC_CASCADE), !IsDlgButtonChecked(hwndDlg, IDC_SAVEPERCONTACT));
					break;
				case IDC_AVATARSUPPORT:
					EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITAVATARH), IsDlgButtonChecked(hwndDlg, IDC_AVATARSUPPORT));
					if (!IsDlgButtonChecked(hwndDlg, IDC_AVATARSUPPORT)) {
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHT), FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHTMIN), FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT1), FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT2), FALSE);
					} else {
						int bChecked = IsDlgButtonChecked(hwndDlg, IDC_LIMITAVATARH);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHT), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHTMIN), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT1), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT2), bChecked);
					}
					break;
				case IDC_LIMITAVATARH:
					{
						int bChecked = IsDlgButtonChecked(hwndDlg, IDC_LIMITAVATARH);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHT), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHTMIN), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT1), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT2), bChecked);
					}
					break;
				case IDC_LIMITNAMESLEN:
				case IDC_LIMITTABSNUM:
				case IDC_LIMITCHATSTABSNUM:
				case IDC_AVATARHEIGHT:
				case IDC_AVATARHEIGHTMIN:
					if (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus())
						return 0;
					break;
				case IDC_TRANSPARENCY:
					if (pSetLayeredWindowAttributes != NULL) {
						int bChecked = IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENCY);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ATRANSPARENCYVALUE), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ATRANSPARENCYPERC), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ITRANSPARENCYVALUE), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ITRANSPARENCYPERC), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCYTEXT1), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCYTEXT2), bChecked);
					}
					break;
//				default:
//				return 0;
			}
			MarkChanges(1, hwndDlg);
			break;
		case WM_HSCROLL:
			{	char str[10];
				sprintf(str,"%d%%",(int)(100*SendDlgItemMessage(hwndDlg,IDC_ATRANSPARENCYVALUE,TBM_GETPOS,0,0)/256));
				SetDlgItemTextA(hwndDlg, IDC_ATRANSPARENCYPERC, str);
				sprintf(str,"%d%%",(int)(100*SendDlgItemMessage(hwndDlg,IDC_ITRANSPARENCYVALUE,TBM_GETPOS,0,0)/256));
				SetDlgItemTextA(hwndDlg, IDC_ITRANSPARENCYPERC, str);
				MarkChanges(1, hwndDlg);
			}
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR) lParam)->idFrom) {
				 case IDC_POPLIST:
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
								MarkChanges(1, hwndDlg);
                            }
                    }
                    break;
				case 0:
					switch (((LPNMHDR) lParam)->code) {
						case PSN_APPLY:
						{
							int limitLength;
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SAVEPERCONTACT, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SAVEPERCONTACT));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_CASCADE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_CASCADE));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWSTATUSBAR, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWSTATUSBAR));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTITLEBAR, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWTITLEBAR));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_USETABS, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_USETABS));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_TABSATBOTTOM, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_TABSATBOTTOM));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_LIMITNAMES, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_LIMITNAMES));
							limitLength = GetDlgItemInt(hwndDlg, IDC_LIMITNAMESLEN, NULL, TRUE) >= SRMSGSET_LIMITNAMESLEN_MIN ? GetDlgItemInt(hwndDlg, IDC_LIMITNAMESLEN, NULL, TRUE) : SRMSGSET_LIMITNAMESLEN_MIN;
							DBWriteContactSettingDword(NULL, SRMMMOD, SRMSGSET_LIMITNAMESLEN, limitLength);

							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_LIMITTABS, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_LIMITTABS));
							limitLength = GetDlgItemInt(hwndDlg, IDC_LIMITTABSNUM, NULL, TRUE) >= 1 ? GetDlgItemInt(hwndDlg, IDC_LIMITTABSNUM, NULL, TRUE) : 1;
							DBWriteContactSettingDword(NULL, SRMMMOD, SRMSGSET_LIMITTABSNUM, limitLength);
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_LIMITCHATSTABS, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_LIMITCHATSTABS));
							limitLength = GetDlgItemInt(hwndDlg, IDC_LIMITCHATSTABSNUM, NULL, TRUE) >= 1 ? GetDlgItemInt(hwndDlg, IDC_LIMITCHATSTABSNUM, NULL, TRUE) : 1;
							DBWriteContactSettingDword(NULL, SRMMMOD, SRMSGSET_LIMITCHATSTABSNUM, limitLength);

							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_HIDEONETAB, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_HIDEONETAB));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SWITCHTOACTIVE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SWITCHTOACTIVE));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_TABCLOSEBUTTON, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_TABCLOSEBUTTON));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SEPARATECHATSCONTAINERS, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SEPARATECHATSCONTAINERS));

							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_HIDECONTAINERS, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_HIDECONTAINERS));

							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_USETRANSPARENCY, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENCY));
							DBWriteContactSettingDword(NULL, SRMMMOD, SRMSGSET_ACTIVEALPHA, SendDlgItemMessage(hwndDlg,IDC_ATRANSPARENCYVALUE,TBM_GETPOS,0,0));
							DBWriteContactSettingDword(NULL, SRMMMOD, SRMSGSET_INACTIVEALPHA, SendDlgItemMessage(hwndDlg,IDC_ITRANSPARENCYVALUE,TBM_GETPOS,0,0));
							ApplyChanges(1);
							return TRUE;
						}
					}
					break;
			}
			break;
		case WM_DESTROY:
			break;
	}
	return FALSE;
}

static BOOL CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
		{
			int bChecked;
			DWORD msgTimeout, avatarHeight;
			TranslateDialogDefault(hwndDlg);
			SetWindowLong(GetDlgItem(hwndDlg, IDC_POPLIST), GWL_STYLE, (GetWindowLong(GetDlgItem(hwndDlg, IDC_POPLIST), GWL_STYLE) & ~WS_BORDER) | TVS_NOHSCROLL | TVS_CHECKBOXES);
			FillCheckBoxTree(GetDlgItem(hwndDlg, IDC_POPLIST), statusValues, sizeof(statusValues) / sizeof(statusValues[0]),
                             DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_POPFLAGS, SRMSGDEFSET_POPFLAGS));
			CheckDlgButton(hwndDlg, IDC_SHOWBUTTONLINE, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWBUTTONLINE, SRMSGDEFSET_SHOWBUTTONLINE));
			CheckDlgButton(hwndDlg, IDC_AUTOPOPUP, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOPOPUP, SRMSGDEFSET_AUTOPOPUP));
			CheckDlgButton(hwndDlg, IDC_STAYMINIMIZED, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_STAYMINIMIZED, SRMSGDEFSET_STAYMINIMIZED));
			CheckDlgButton(hwndDlg, IDC_AUTOMIN, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOMIN, SRMSGDEFSET_AUTOMIN));
			CheckDlgButton(hwndDlg, IDC_AUTOCLOSE, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOCLOSE, SRMSGDEFSET_AUTOCLOSE));
			CheckDlgButton(hwndDlg, IDC_SAVEPERCONTACT, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SAVEPERCONTACT, SRMSGDEFSET_SAVEPERCONTACT));
			CheckDlgButton(hwndDlg, IDC_SAVESPLITTERPERCONTACT, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SAVESPLITTERPERCONTACT, SRMSGDEFSET_SAVESPLITTERPERCONTACT));
			CheckDlgButton(hwndDlg, IDC_CASCADE, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_CASCADE, SRMSGDEFSET_CASCADE));
			CheckDlgButton(hwndDlg, IDC_SENDONENTER, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDONENTER, SRMSGDEFSET_SENDONENTER));
			CheckDlgButton(hwndDlg, IDC_SENDONDBLENTER, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDONDBLENTER, SRMSGDEFSET_SENDONDBLENTER));
			CheckDlgButton(hwndDlg, IDC_STATUSWIN, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_STATUSICON, SRMSGDEFSET_STATUSICON));
			CheckDlgButton(hwndDlg, IDC_SAVEDRAFTS, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SAVEDRAFTS, SRMSGDEFSET_SAVEDRAFTS));
			CheckDlgButton(hwndDlg, IDC_SHOWSTATUSBAR, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWSTATUSBAR, SRMSGDEFSET_SHOWSTATUSBAR));
			CheckDlgButton(hwndDlg, IDC_SHOWTITLEBAR, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTITLEBAR, SRMSGDEFSET_SHOWTITLEBAR));
			CheckDlgButton(hwndDlg, IDC_SHOWPROGRESS, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWPROGRESS, SRMSGDEFSET_SHOWPROGRESS));
			CheckDlgButton(hwndDlg, IDC_USETABS, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_USETABS, SRMSGDEFSET_USETABS));
			CheckDlgButton(hwndDlg, IDC_TABSATBOTTOM, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_TABSATBOTTOM, SRMSGDEFSET_TABSATBOTTOM));
			CheckDlgButton(hwndDlg, IDC_SWITCHTOACTIVE, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SWITCHTOACTIVE, SRMSGDEFSET_SWITCHTOACTIVE));
			CheckDlgButton(hwndDlg, IDC_LIMITNAMES, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_LIMITNAMES, SRMSGDEFSET_LIMITNAMES));
			CheckDlgButton(hwndDlg, IDC_HIDEONETAB, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_HIDEONETAB, SRMSGDEFSET_HIDEONETAB));
			CheckDlgButton(hwndDlg, IDC_TRANSPARENCY, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_USETRANSPARENCY, SRMSGDEFSET_USETRANSPARENCY));
			SendDlgItemMessage(hwndDlg,IDC_ATRANSPARENCYVALUE,TBM_SETRANGE, FALSE, MAKELONG(0,255));
			SendDlgItemMessage(hwndDlg,IDC_ATRANSPARENCYVALUE,TBM_SETPOS, TRUE, DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_ACTIVEALPHA, SRMSGDEFSET_ACTIVEALPHA));
			SendDlgItemMessage(hwndDlg,IDC_ITRANSPARENCYVALUE,TBM_SETRANGE, FALSE, MAKELONG(0,255));
			SendDlgItemMessage(hwndDlg,IDC_ITRANSPARENCYVALUE,TBM_SETPOS, TRUE, DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_INACTIVEALPHA, SRMSGDEFSET_INACTIVEALPHA));
			{	char str[10];
				sprintf(str,"%d%%",(int)(100*SendDlgItemMessage(hwndDlg,IDC_ATRANSPARENCYVALUE,TBM_GETPOS,0,0)/255));
				SetDlgItemTextA(hwndDlg, IDC_ATRANSPARENCYPERC, str);
				sprintf(str,"%d%%",(int)(100*SendDlgItemMessage(hwndDlg,IDC_ITRANSPARENCYVALUE,TBM_GETPOS,0,0)/255));
				SetDlgItemTextA(hwndDlg, IDC_ITRANSPARENCYPERC, str);
			}

			if (pSetLayeredWindowAttributes == NULL) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCY), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ATRANSPARENCYVALUE), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ATRANSPARENCYPERC), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ITRANSPARENCYVALUE), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ITRANSPARENCYPERC), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCYTEXT1), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCYTEXT2), FALSE);
			} else  {
				int bChecked = IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENCY);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ATRANSPARENCYVALUE), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ATRANSPARENCYPERC), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ITRANSPARENCYVALUE), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ITRANSPARENCYPERC), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCYTEXT1), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCYTEXT2), bChecked);
			}
			CheckDlgButton(hwndDlg, IDC_AVATARSUPPORT, g_dat->flags&SMF_AVATAR);
//			EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITAVATARH), FALSE);
//			EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHT), FALSE);
			CheckDlgButton(hwndDlg, IDC_LIMITAVATARH, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_LIMITAVHEIGHT, SRMSGDEFSET_LIMITAVHEIGHT));
			avatarHeight = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_AVHEIGHT, SRMSGDEFSET_AVHEIGHT);
			SetDlgItemInt(hwndDlg, IDC_AVATARHEIGHT, avatarHeight, FALSE);
			avatarHeight = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_AVHEIGHTMIN, SRMSGDEFSET_AVHEIGHTMIN);
			SetDlgItemInt(hwndDlg, IDC_AVATARHEIGHTMIN, avatarHeight, FALSE);

			EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITAVATARH), IsDlgButtonChecked(hwndDlg, IDC_AVATARSUPPORT));
			if (!IsDlgButtonChecked(hwndDlg, IDC_AVATARSUPPORT)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHT), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHTMIN), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT1), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT2), FALSE);
			} else {
				int bChecked = IsDlgButtonChecked(hwndDlg, IDC_LIMITAVATARH);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHT), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHTMIN), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT1), bChecked);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT2), bChecked);
			}

			CheckDlgButton(hwndDlg, IDC_CTRLSUPPORT, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_CTRLSUPPORT, SRMSGDEFSET_CTRLSUPPORT));
			CheckDlgButton(hwndDlg, IDC_DELTEMP, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_DELTEMP, SRMSGDEFSET_DELTEMP));
			msgTimeout = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_MSGTIMEOUT, SRMSGDEFSET_MSGTIMEOUT);
			SetDlgItemInt(hwndDlg, IDC_SECONDS, msgTimeout >= SRMSGSET_MSGTIMEOUT_MIN ? msgTimeout / 1000 : SRMSGDEFSET_MSGTIMEOUT / 1000, FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_CTRLSUPPORT), !IsDlgButtonChecked(hwndDlg, IDC_AUTOCLOSE));

			EnableWindow(GetDlgItem(hwndDlg, IDC_STAYMINIMIZED), IsDlgButtonChecked(hwndDlg, IDC_AUTOPOPUP));
			EnableWindow(GetDlgItem(hwndDlg, IDC_POPLIST), IsDlgButtonChecked(hwndDlg, IDC_AUTOPOPUP));
			bChecked = IsDlgButtonChecked(hwndDlg, IDC_USETABS);
			EnableWindow(GetDlgItem(hwndDlg, IDC_SWITCHTOACTIVE), bChecked);
			EnableWindow(GetDlgItem(hwndDlg, IDC_TABSATBOTTOM), bChecked );
			EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITNAMES), bChecked );
			EnableWindow(GetDlgItem(hwndDlg, IDC_HIDEONETAB), bChecked );
			EnableWindow(GetDlgItem(hwndDlg, IDC_CASCADE), !bChecked  );//&& !IsDlgButtonChecked(hwndDlg, IDC_SAVEPERCONTACT));
			EnableWindow(GetDlgItem(hwndDlg, IDC_SAVEPERCONTACT), !bChecked );
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_AUTOPOPUP:
					EnableWindow(GetDlgItem(hwndDlg, IDC_STAYMINIMIZED), IsDlgButtonChecked(hwndDlg, IDC_AUTOPOPUP));
					EnableWindow(GetDlgItem(hwndDlg, IDC_POPLIST), IsDlgButtonChecked(hwndDlg, IDC_AUTOPOPUP));
					break;
				case IDC_AUTOMIN:
					CheckDlgButton(hwndDlg, IDC_AUTOCLOSE, BST_UNCHECKED);
					EnableWindow(GetDlgItem(hwndDlg, IDC_CTRLSUPPORT), !IsDlgButtonChecked(hwndDlg, IDC_AUTOCLOSE));
					break;
				case IDC_USETABS:
					{
						int bChecked = IsDlgButtonChecked(hwndDlg, IDC_USETABS);
						EnableWindow(GetDlgItem(hwndDlg, IDC_SWITCHTOACTIVE), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_TABSATBOTTOM), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITNAMES), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_HIDEONETAB), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_CASCADE), !bChecked );//&& !IsDlgButtonChecked(hwndDlg, IDC_SAVEPERCONTACT));
						EnableWindow(GetDlgItem(hwndDlg, IDC_SAVEPERCONTACT), !bChecked);
					}
					break;
				case IDC_AUTOCLOSE:
					CheckDlgButton(hwndDlg, IDC_AUTOMIN, BST_UNCHECKED);
					EnableWindow(GetDlgItem(hwndDlg, IDC_CTRLSUPPORT), !IsDlgButtonChecked(hwndDlg, IDC_AUTOCLOSE));
					break;
				case IDC_SENDONENTER:
					CheckDlgButton(hwndDlg, IDC_SENDONDBLENTER, BST_UNCHECKED);
					break;
				case IDC_SENDONDBLENTER:
					CheckDlgButton(hwndDlg, IDC_SENDONENTER, BST_UNCHECKED);
					break;
				case IDC_CASCADE:
					CheckDlgButton(hwndDlg, IDC_SAVEPERCONTACT, BST_UNCHECKED);
					break;
				case IDC_SAVEPERCONTACT:
					CheckDlgButton(hwndDlg, IDC_CASCADE, BST_UNCHECKED);
					//EnableWindow(GetDlgItem(hwndDlg, IDC_CASCADE), !IsDlgButtonChecked(hwndDlg, IDC_SAVEPERCONTACT));
					break;
				case IDC_SECONDS:
					if (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus())
						return 0;
					break;
				case IDC_AVATARSUPPORT:
					EnableWindow(GetDlgItem(hwndDlg, IDC_LIMITAVATARH), IsDlgButtonChecked(hwndDlg, IDC_AVATARSUPPORT));
					if (!IsDlgButtonChecked(hwndDlg, IDC_AVATARSUPPORT)) {
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHT), FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHTMIN), FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT1), FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT2), FALSE);
					} else {
						int bChecked = IsDlgButtonChecked(hwndDlg, IDC_LIMITAVATARH);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHT), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHTMIN), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT1), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT2), bChecked);
					}
					break;
				case IDC_LIMITAVATARH:
					{
						int bChecked = IsDlgButtonChecked(hwndDlg, IDC_LIMITAVATARH);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHT), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARHEIGHTMIN), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT1), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_AVATARTEXT2), bChecked);
					}
					break;
				case IDC_AVATARHEIGHT:
				case IDC_AVATARHEIGHTMIN:
					if (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus())
						return 0;
					break;
				case IDC_TRANSPARENCY:
					if (pSetLayeredWindowAttributes != NULL) {
						int bChecked = IsDlgButtonChecked(hwndDlg, IDC_TRANSPARENCY);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ATRANSPARENCYVALUE), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ATRANSPARENCYPERC), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ITRANSPARENCYVALUE), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ITRANSPARENCYPERC), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCYTEXT1), bChecked);
						EnableWindow(GetDlgItem(hwndDlg, IDC_TRANSPARENCYTEXT2), bChecked);
					}
					break;
			}
			MarkChanges(2, hwndDlg);
			break;
		case WM_HSCROLL:
			{	char str[10];
				sprintf(str,"%d%%",(int)(100*SendDlgItemMessage(hwndDlg,IDC_ATRANSPARENCYVALUE,TBM_GETPOS,0,0)/256));
				SetDlgItemTextA(hwndDlg, IDC_ATRANSPARENCYPERC, str);
				sprintf(str,"%d%%",(int)(100*SendDlgItemMessage(hwndDlg,IDC_ITRANSPARENCYVALUE,TBM_GETPOS,0,0)/256));
				SetDlgItemTextA(hwndDlg, IDC_ITRANSPARENCYPERC, str);
				MarkChanges(2, hwndDlg);
			}
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR) lParam)->idFrom) {
				 case IDC_POPLIST:
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
								MarkChanges(2, hwndDlg);
							}
					}
				    break;
				case 0:
					switch (((LPNMHDR) lParam)->code) {
						case PSN_APPLY:
						{
							DWORD msgTimeout, avatarHeight;
							DBWriteContactSettingDword(NULL, SRMMMOD, SRMSGSET_POPFLAGS, MakeCheckBoxTreeFlags(GetDlgItem(hwndDlg, IDC_POPLIST)));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWBUTTONLINE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWBUTTONLINE));
							//DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWINFOLINE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWINFOLINE));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOPOPUP, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AUTOPOPUP));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_STAYMINIMIZED, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_STAYMINIMIZED));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOMIN, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AUTOMIN));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_AUTOCLOSE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AUTOCLOSE));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SAVESPLITTERPERCONTACT, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SAVESPLITTERPERCONTACT));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDONENTER, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SENDONENTER));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDONDBLENTER, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SENDONDBLENTER));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_STATUSICON, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_STATUSWIN));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SAVEDRAFTS, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SAVEDRAFTS));

							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWPROGRESS, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWPROGRESS));

							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_AVATARENABLE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_AVATARSUPPORT));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_LIMITAVHEIGHT, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_LIMITAVATARH));
							avatarHeight = GetDlgItemInt(hwndDlg, IDC_AVATARHEIGHT, NULL, TRUE);
							DBWriteContactSettingDword(NULL, SRMMMOD, SRMSGSET_AVHEIGHT, avatarHeight<0?SRMSGDEFSET_AVHEIGHT:avatarHeight);
							avatarHeight = GetDlgItemInt(hwndDlg, IDC_AVATARHEIGHTMIN, NULL, TRUE);
							DBWriteContactSettingDword(NULL, SRMMMOD, SRMSGSET_AVHEIGHTMIN, avatarHeight<0?SRMSGDEFSET_AVHEIGHTMIN:avatarHeight);

							//DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SENDBUTTON, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWSENDBTN));
							//DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_CHARCOUNT, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_CHARCOUNT));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_CTRLSUPPORT, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_CTRLSUPPORT));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_DELTEMP, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_DELTEMP));
							msgTimeout = GetDlgItemInt(hwndDlg, IDC_SECONDS, NULL, TRUE) >= SRMSGSET_MSGTIMEOUT_MIN / 1000 ? GetDlgItemInt(hwndDlg, IDC_SECONDS, NULL, TRUE) * 1000 : SRMSGDEFSET_MSGTIMEOUT;
							DBWriteContactSettingDword(NULL, SRMMMOD, SRMSGSET_MSGTIMEOUT, msgTimeout);
							ApplyChanges(2);

							return TRUE;
						}
					}
					break;
			}
			break;
		case WM_DESTROY:
			break;
	}
	return FALSE;
}


static BOOL CALLBACK DlgProcLogOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			switch (DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_LOADHISTORY, SRMSGDEFSET_LOADHISTORY)) {
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
			SendDlgItemMessage(hwndDlg, IDC_LOADCOUNTSPIN, UDM_SETRANGE, 0, MAKELONG(100, 0));
			SendDlgItemMessage(hwndDlg, IDC_LOADCOUNTSPIN, UDM_SETPOS, 0, DBGetContactSettingWord(NULL, SRMMMOD, SRMSGSET_LOADCOUNT, SRMSGDEFSET_LOADCOUNT));
			SendDlgItemMessage(hwndDlg, IDC_LOADTIMESPIN, UDM_SETRANGE, 0, MAKELONG(12 * 60, 0));
			SendDlgItemMessage(hwndDlg, IDC_LOADTIMESPIN, UDM_SETPOS, 0, DBGetContactSettingWord(NULL, SRMMMOD, SRMSGSET_LOADTIME, SRMSGDEFSET_LOADTIME));

			CheckDlgButton(hwndDlg, IDC_SHOWLOGICONS, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWLOGICONS, SRMSGDEFSET_SHOWLOGICONS));
			CheckDlgButton(hwndDlg, IDC_SHOWNAMES, !DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_HIDENAMES, SRMSGDEFSET_HIDENAMES));
			CheckDlgButton(hwndDlg, IDC_SHOWTIMES, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTIME, SRMSGDEFSET_SHOWTIME));
			CheckDlgButton(hwndDlg, IDC_SHOWSECONDS, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWSECONDS, SRMSGDEFSET_SHOWSECONDS));
			EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWSECONDS), IsDlgButtonChecked(hwndDlg, IDC_SHOWTIMES));
			EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWDATES), IsDlgButtonChecked(hwndDlg, IDC_SHOWTIMES));
			CheckDlgButton(hwndDlg, IDC_SHOWDATES, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWDATE, SRMSGDEFSET_SHOWDATE));
			CheckDlgButton(hwndDlg, IDC_USELONGDATE, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_USELONGDATE, SRMSGDEFSET_USELONGDATE));
			CheckDlgButton(hwndDlg, IDC_USERELATIVEDATE, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_USERELATIVEDATE, SRMSGDEFSET_USERELATIVEDATE));
			EnableWindow(GetDlgItem(hwndDlg, IDC_USELONGDATE), IsDlgButtonChecked(hwndDlg, IDC_SHOWDATES) && IsDlgButtonChecked(hwndDlg, IDC_SHOWTIMES));
			EnableWindow(GetDlgItem(hwndDlg, IDC_USERELATIVEDATE), IsDlgButtonChecked(hwndDlg, IDC_SHOWDATES) && IsDlgButtonChecked(hwndDlg, IDC_SHOWTIMES));

			if (!ServiceExists(MS_IEVIEW_WINDOW)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_USEIEVIEW), FALSE);
			}
			CheckDlgButton(hwndDlg, IDC_USEIEVIEW, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_USEIEVIEW, SRMSGDEFSET_USEIEVIEW));

			CheckDlgButton(hwndDlg, IDC_GROUPMESSAGES, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_GROUPMESSAGES, SRMSGDEFSET_GROUPMESSAGES));
			EnableWindow(GetDlgItem(hwndDlg, IDC_MARKFOLLOWUPS), IsDlgButtonChecked(hwndDlg, IDC_GROUPMESSAGES));
			CheckDlgButton(hwndDlg, IDC_MARKFOLLOWUPS, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_MARKFOLLOWUPS, SRMSGDEFSET_MARKFOLLOWUPS));
			CheckDlgButton(hwndDlg, IDC_MESSAGEONNEWLINE, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_MESSAGEONNEWLINE, SRMSGDEFSET_MESSAGEONNEWLINE));
			CheckDlgButton(hwndDlg, IDC_DRAWLINES, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_DRAWLINES, SRMSGDEFSET_DRAWLINES));
			EnableWindow(GetDlgItem(hwndDlg, IDC_LINECOLOUR), IsDlgButtonChecked(hwndDlg, IDC_DRAWLINES));

			CheckDlgButton(hwndDlg, IDC_INDENTTEXT, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_INDENTTEXT, SRMSGDEFSET_INDENTTEXT));
			EnableWindow(GetDlgItem(hwndDlg, IDC_INDENTSIZE), IsDlgButtonChecked(hwndDlg, IDC_INDENTTEXT));
			EnableWindow(GetDlgItem(hwndDlg, IDC_INDENTSPIN), IsDlgButtonChecked(hwndDlg, IDC_INDENTTEXT));
			SendDlgItemMessage(hwndDlg, IDC_INDENTSPIN, UDM_SETRANGE, 0, MAKELONG(999, 0));
			SendDlgItemMessage(hwndDlg, IDC_INDENTSPIN, UDM_SETPOS, 0, DBGetContactSettingWord(NULL, SRMMMOD, SRMSGSET_INDENTSIZE, SRMSGDEFSET_INDENTSIZE));

			CheckDlgButton(hwndDlg, IDC_SHOWSTATUSCHANGES, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWSTATUSCH, SRMSGDEFSET_SHOWSTATUSCH));

			SendDlgItemMessage(hwndDlg, IDC_LINECOLOUR, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_LINECOLOUR, SRMSGDEFSET_LINECOLOUR));

			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_LOADUNREAD:
				case IDC_LOADCOUNT:
				case IDC_LOADTIME:
					EnableWindow(GetDlgItem(hwndDlg, IDC_LOADCOUNTN), IsDlgButtonChecked(hwndDlg, IDC_LOADCOUNT));
					EnableWindow(GetDlgItem(hwndDlg, IDC_LOADCOUNTSPIN), IsDlgButtonChecked(hwndDlg, IDC_LOADCOUNT));
					EnableWindow(GetDlgItem(hwndDlg, IDC_LOADTIMEN), IsDlgButtonChecked(hwndDlg, IDC_LOADTIME));
					EnableWindow(GetDlgItem(hwndDlg, IDC_LOADTIMESPIN), IsDlgButtonChecked(hwndDlg, IDC_LOADTIME));
					EnableWindow(GetDlgItem(hwndDlg, IDC_STMINSOLD), IsDlgButtonChecked(hwndDlg, IDC_LOADTIME));
					break;
				case IDC_SHOWTIMES:
					EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWSECONDS), IsDlgButtonChecked(hwndDlg, IDC_SHOWTIMES));
					EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWDATES), IsDlgButtonChecked(hwndDlg, IDC_SHOWTIMES));
				case IDC_SHOWDATES:
					EnableWindow(GetDlgItem(hwndDlg, IDC_USELONGDATE), IsDlgButtonChecked(hwndDlg, IDC_SHOWDATES) && IsDlgButtonChecked(hwndDlg, IDC_SHOWTIMES));
					EnableWindow(GetDlgItem(hwndDlg, IDC_USERELATIVEDATE), IsDlgButtonChecked(hwndDlg, IDC_SHOWDATES) && IsDlgButtonChecked(hwndDlg, IDC_SHOWTIMES));
					break;
				case IDC_GROUPMESSAGES:
					EnableWindow(GetDlgItem(hwndDlg, IDC_MARKFOLLOWUPS), IsDlgButtonChecked(hwndDlg, IDC_GROUPMESSAGES));
					break;
				case IDC_DRAWLINES:
					EnableWindow(GetDlgItem(hwndDlg, IDC_LINECOLOUR), IsDlgButtonChecked(hwndDlg, IDC_DRAWLINES));
					break;
				case IDC_INDENTTEXT:
					EnableWindow(GetDlgItem(hwndDlg, IDC_INDENTSIZE), IsDlgButtonChecked(hwndDlg, IDC_INDENTTEXT));
					EnableWindow(GetDlgItem(hwndDlg, IDC_INDENTSPIN), IsDlgButtonChecked(hwndDlg, IDC_INDENTTEXT));
					break;
				case IDC_INDENTSIZE:
				case IDC_LOADCOUNTN:
				case IDC_LOADTIMEN:
					if (HIWORD(wParam) != EN_CHANGE || (HWND) lParam != GetFocus())
						return TRUE;
					break;
			}
			MarkChanges(4, hwndDlg);
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR) lParam)->idFrom) {
				case 0:
					switch (((LPNMHDR) lParam)->code) {
						case PSN_APPLY:
							if (IsDlgButtonChecked(hwndDlg, IDC_LOADCOUNT))
								DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_LOADHISTORY, LOADHISTORY_COUNT);
							else if (IsDlgButtonChecked(hwndDlg, IDC_LOADTIME))
								DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_LOADHISTORY, LOADHISTORY_TIME);
							else
								DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_LOADHISTORY, LOADHISTORY_UNREAD);
							DBWriteContactSettingWord(NULL, SRMMMOD, SRMSGSET_LOADCOUNT, (WORD) SendDlgItemMessage(hwndDlg, IDC_LOADCOUNTSPIN, UDM_GETPOS, 0, 0));
							DBWriteContactSettingWord(NULL, SRMMMOD, SRMSGSET_LOADTIME, (WORD) SendDlgItemMessage(hwndDlg, IDC_LOADTIMESPIN, UDM_GETPOS, 0, 0));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWLOGICONS, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWLOGICONS));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_HIDENAMES, (BYTE) ! IsDlgButtonChecked(hwndDlg, IDC_SHOWNAMES));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTIME, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWTIMES));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWSECONDS, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWSECONDS));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWDATE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWDATES));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_USELONGDATE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_USELONGDATE));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_USERELATIVEDATE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_USERELATIVEDATE));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWSTATUSCH, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWSTATUSCHANGES));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_GROUPMESSAGES, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_GROUPMESSAGES));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_MARKFOLLOWUPS, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_MARKFOLLOWUPS));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_MESSAGEONNEWLINE, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_MESSAGEONNEWLINE));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_DRAWLINES, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_DRAWLINES));
							DBWriteContactSettingDword(NULL, SRMMMOD, SRMSGSET_LINECOLOUR, SendDlgItemMessage(hwndDlg, IDC_LINECOLOUR, CPM_GETCOLOUR, 0, 0));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_USEIEVIEW, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_USEIEVIEW));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_INDENTTEXT, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_INDENTTEXT));
							DBWriteContactSettingWord(NULL, SRMMMOD, SRMSGSET_INDENTSIZE, (WORD) SendDlgItemMessage(hwndDlg, IDC_INDENTSPIN, UDM_GETPOS, 0, 0));

							FreeMsgLogIcons();
							LoadMsgLogIcons();
							ApplyChanges(4);
							return TRUE;
					}
					break;
			}
			break;
		case WM_DESTROY:
			break;
	}
	return FALSE;
}

static void ResetCList(HWND hwndDlg)
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
	BYTE defType = DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_TYPINGNEW, SRMSGDEFSET_TYPINGNEW);

	if (hItemNew && defType) {
		SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM) hItemNew, 1);
	}
	if (hItemUnknown && DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_TYPINGUNKNOWN, SRMSGDEFSET_TYPINGUNKNOWN)) {
		SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM) hItemUnknown, 1);
	}
	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	do {
		hItem = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM) hContact, 0);
		if (hItem && DBGetContactSettingByte(hContact, SRMMMOD, SRMSGSET_TYPING, defType)) {
			SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM) hItem, 1);
		}
	} while ((hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0)));
}

static void SaveList(HWND hwndDlg, HANDLE hItemNew, HANDLE hItemUnknown)
{
	HANDLE hContact, hItem;

	if (hItemNew) {
		DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_TYPINGNEW, (BYTE)(SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM) hItemNew, 0) ? 1 : 0));
	}
	if (hItemUnknown) {
		DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_TYPINGUNKNOWN, (BYTE)(SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM) hItemUnknown, 0) ? 1 : 0));
	}
	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	do {
		hItem = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM) hContact, 0);
		if (hItem) {
			DBWriteContactSettingByte(hContact, SRMMMOD, SRMSGSET_TYPING, (BYTE)(SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM) hItem, 0) ? 1 : 0));
		}
	} while ((hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0)));
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
				cii.pszText = (TCHAR *)TranslateT("** New contacts **");
				hItemNew = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_ADDINFOITEM, 0, (LPARAM) & cii);
				cii.pszText = (TCHAR *)TranslateT("** Unknown contacts **");
				hItemUnknown = (HANDLE) SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_ADDINFOITEM, 0, (LPARAM) & cii);
			}
			SetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_STYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_CLIST), GWL_STYLE) | (CLS_SHOWHIDDEN) | (CLS_NOHIDEOFFLINE));
			ResetCList(hwndDlg);
			RebuildList(hwndDlg, hItemNew, hItemUnknown);
			CheckDlgButton(hwndDlg, IDC_SHOWNOTIFY, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPING, SRMSGDEFSET_SHOWTYPING));
			CheckDlgButton(hwndDlg, IDC_TYPEWIN, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGWIN, SRMSGDEFSET_SHOWTYPINGWIN));
			CheckDlgButton(hwndDlg, IDC_TYPETRAY, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGNOWIN, SRMSGDEFSET_SHOWTYPINGNOWIN));
			CheckDlgButton(hwndDlg, IDC_NOTIFYTRAY, DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGCLIST, SRMSGDEFSET_SHOWTYPINGCLIST));
			CheckDlgButton(hwndDlg, IDC_NOTIFYBALLOON, !DBGetContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGCLIST, SRMSGDEFSET_SHOWTYPINGCLIST));
			EnableWindow(GetDlgItem(hwndDlg, IDC_TYPEWIN), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
			EnableWindow(GetDlgItem(hwndDlg, IDC_TYPETRAY), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
			EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYTRAY), IsDlgButtonChecked(hwndDlg, IDC_TYPETRAY));
			EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), IsDlgButtonChecked(hwndDlg, IDC_TYPETRAY));
			if (!ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), FALSE);
				CheckDlgButton(hwndDlg, IDC_NOTIFYTRAY, BST_CHECKED);
				SetWindowText(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), TranslateT("Show balloon popup (unsupported system)"));
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_TYPETRAY:
					if (IsDlgButtonChecked(hwndDlg, IDC_TYPETRAY)) {
						if (!ServiceExists(MS_CLIST_SYSTRAY_NOTIFY)) {
							EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYTRAY), TRUE);
						}
						else {
							EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYTRAY), TRUE);
							EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), TRUE);
						}
					}
					else {
						EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYTRAY), FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), FALSE);
					}
					MarkChanges(4, hwndDlg);
					break;
				case IDC_SHOWNOTIFY:
					EnableWindow(GetDlgItem(hwndDlg, IDC_TYPEWIN), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
					EnableWindow(GetDlgItem(hwndDlg, IDC_TYPETRAY), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
					EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYTRAY), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
					EnableWindow(GetDlgItem(hwndDlg, IDC_NOTIFYBALLOON), IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY)
								 && ServiceExists(MS_CLIST_SYSTRAY_NOTIFY));
					//fall-thru
				case IDC_TYPEWIN:
				case IDC_NOTIFYTRAY:
				case IDC_NOTIFYBALLOON:
					MarkChanges(4, hwndDlg);
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
							MarkChanges(4, hwndDlg);
							break;
					}
					break;
				case 0:
					switch (((LPNMHDR) lParam)->code) {
						case PSN_APPLY:
						{
							SaveList(hwndDlg, hItemNew, hItemUnknown);
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPING, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_SHOWNOTIFY));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGWIN, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_TYPEWIN));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGNOWIN, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_TYPETRAY));
							DBWriteContactSettingByte(NULL, SRMMMOD, SRMSGSET_SHOWTYPINGCLIST, (BYTE) IsDlgButtonChecked(hwndDlg, IDC_NOTIFYTRAY));
							ReloadGlobals();
							WindowList_Broadcast(g_dat->hMessageWindowList, DM_OPTIONSAPPLIED, 0, 0);
						}
					}
					break;
			}
			break;
	}
	return FALSE;
}

int OptInitialise(WPARAM wParam, LPARAM lParam)
{
	DWORD i;
	OPTIONSDIALOGPAGE odp = { 0 };
	odp.cbSize = sizeof(odp);
	odp.position = 910000000;
	odp.hInstance = g_hInst;
	odp.ptszTitle = LPGENT("Messaging");
	odp.ptszGroup = LPGENT("Message Sessions"); //Events
	odp.flags = ODPF_BOLDGROUPS | ODPF_TCHAR;
	odp.nIDBottomSimpleControl = 0;
	for (i = 0; i < SIZEOF(tabPages); i++) {
		odp.pszTemplate = MAKEINTRESOURCEA(tabPages[i].dlgId);
		odp.pfnDlgProc = tabPages[i].dlgProc;
		odp.ptszTab = tabPages[i].tabName;
		CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp);
	}
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_MSGTYPE);
	odp.ptszTitle = TranslateT("Typing Notify");
	odp.pfnDlgProc = DlgProcTypeOptions;
	odp.ptszTab = NULL;
	CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp);

	return 0;
}


