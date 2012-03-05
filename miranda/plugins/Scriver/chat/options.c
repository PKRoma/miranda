/*
Chat module plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson
Copyright 2003-2009 Miranda ICQ/IM project,

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

#include "../commonheaders.h"
#include "chat.h"
#include <shlobj.h>
#include <shlwapi.h>

#define UM_CHECKSTATECHANGE (WM_USER+100)

extern HANDLE			g_hInst;
extern HBRUSH 			hListBkgBrush;
extern HBRUSH 			hListSelectedBkgBrush;
extern FONTINFO			aFonts[OPTIONS_FONTCOUNT];

HANDLE			g_hOptions = NULL;
static HWND hPathTip = 0;

#define FONTF_BOLD   1
#define FONTF_ITALIC 2
struct FontOptionsList
{
    TCHAR*   szDescr;
    COLORREF defColour;
    TCHAR*   szDefFace;
    BYTE     defCharset, defStyle;
    char     defSize;
    COLORREF colour;
    TCHAR    szFace[LF_FACESIZE];
    BYTE     charset, style;
    char     size;
}

//remeber to put these in the Translate( ) template file too
static fontOptionsList[] = {
	{LPGENT("Timestamp"), RGB(50, 50, 240), _T("Terminal"), DEFAULT_CHARSET, 0, -8},
	{LPGENT("Others nicknames"), RGB(0, 0, 0), _T("Verdana"), DEFAULT_CHARSET, FONTF_BOLD, -13},
	{LPGENT("Your nickname"), RGB(0, 0, 0), _T("Verdana"), DEFAULT_CHARSET, FONTF_BOLD, -13},
	{LPGENT("User has joined"), RGB(90, 160, 90), _T("Verdana"), DEFAULT_CHARSET, 0, -13},
	{LPGENT("User has left"), RGB(160, 160, 90), _T("Verdana"), DEFAULT_CHARSET, 0, -13},
	{LPGENT("User has disconnected"), RGB(160, 90, 90), _T("Verdana"), DEFAULT_CHARSET, 0, -13},
	{LPGENT("User kicked ..."), RGB(100, 100, 100), _T("Verdana"), DEFAULT_CHARSET, 0, -13},
	{LPGENT("User is now known as ..."), RGB(90, 90, 160), _T("Verdana"), DEFAULT_CHARSET, 0, -13},
	{LPGENT("Notice from user"), RGB(160, 130, 60), _T("Verdana"), DEFAULT_CHARSET, 0, -13},
	{LPGENT("Incoming message"), RGB(90, 90, 90), _T("Verdana"), DEFAULT_CHARSET, 0, -13},
	{LPGENT("Outgoing message"), RGB(90, 90, 90), _T("Verdana"), DEFAULT_CHARSET, 0, -13},
	{LPGENT("The topic is ..."), RGB(70, 70, 160), _T("Verdana"), DEFAULT_CHARSET, 0, -13},
	{LPGENT("Information messages"), RGB(130, 130, 195), _T("Verdana"), DEFAULT_CHARSET, 0, -13},
	{LPGENT("User enables status for ..."), RGB(70, 150, 70), _T("Verdana"), DEFAULT_CHARSET, 0, -13},
	{LPGENT("User disables status for ..."), RGB(150, 70, 70), _T("Verdana"), DEFAULT_CHARSET, 0, -13},
	{LPGENT("Action message"), RGB(160, 90, 160), _T("Verdana"), DEFAULT_CHARSET, 0, -13},
	{LPGENT("Highlighted message"), RGB(180, 150, 80), _T("Verdana"), DEFAULT_CHARSET, 0, -13},
	{LPGENT("Message typing area"), RGB(0, 0, 40), _T("Verdana"), DEFAULT_CHARSET, 0, -14},
	{LPGENT("User list members (online)"), RGB(0,0, 0), _T("Verdana"), DEFAULT_CHARSET, 0, -12},
	{LPGENT("User list members (away)"), RGB(170, 170, 170), _T("Verdana"), DEFAULT_CHARSET, 0, -12},
};

struct branch_t
{
	TCHAR*    szDescr;
	char*     szDBName;
	int       iMode;
	BYTE      bDefault;
	HTREEITEM hItem;
};

static struct branch_t branch1[] = {
	{LPGENT("Flash when someone speaks"), "FlashWindow", 0,0, NULL},
	{LPGENT("Flash when a word is highlighted"), "FlashWindowHighlight", 0,1, NULL},
	{LPGENT("Show chat user list"), "ShowNicklist", 0,1, NULL},
	{LPGENT("Enable button context menus"), "RightClickFilter", 0,0, NULL},
	{LPGENT("Show topic on your contact list (if supported)"), "TopicOnClist", 0, 0, NULL},
	{LPGENT("Do not play sounds when focused"), "SoundsFocus", 0, 0, NULL},
	{LPGENT("Do not pop up when joining"), "PopupOnJoin", 0,0, NULL},
	{LPGENT("Show and hide by double clicking in the contact list"), "ToggleVisibility", 0,0, NULL},
    {LPGENT("Show contact statuses (if supported)"), "ShowContactStatus", 0,0, NULL},
    {LPGENT("Display contact status icon before role icon"), "ContactStatusFirst", 0,0, NULL},
	{LPGENT("Add \':\' to auto-completed names"), "AddColonToAutoComplete", 0, 1, NULL}
};
static struct branch_t branch2[] = {
	{LPGENT("Show icons"), "IconFlags", 	GC_EVENT_TOPIC|GC_EVENT_JOIN|GC_EVENT_QUIT|
				GC_EVENT_MESSAGE|GC_EVENT_ACTION|GC_EVENT_HIGHLIGHT|GC_EVENT_PART|
				GC_EVENT_KICK|GC_EVENT_NOTICE|GC_EVENT_NICK|GC_EVENT_INFORMATION|GC_EVENT_ADDSTATUS, 0, NULL},
	{LPGENT("Prefix all events with a timestamp"), "ShowTimeStamp", 0,1, NULL},
	{LPGENT("Only prefix with timestamp if it has changed"), "ShowTimeStampIfChanged", 0,0, NULL},
	{LPGENT("Timestamp has same colour as event"), "TimeStampEventColour", 0,0, NULL},
	{LPGENT("Indent the second line of a message"), "LogIndentEnabled", 0,1, NULL},
	{LPGENT("Limit user names to 20 characters"), "LogLimitNames", 0,1, NULL},
	{LPGENT("Strip colors from messages"), "StripFormatting", 0, 0, NULL},
	{LPGENT("Enable \'event filter\' for new rooms"), "FilterEnabled", 0,0, NULL}
};
static struct branch_t branch3[] = {
	{LPGENT("Show topic changes"), "FilterFlags", GC_EVENT_TOPIC, 0, NULL},
	{LPGENT("Show users joining"), "FilterFlags", GC_EVENT_JOIN, 0, NULL},
	{LPGENT("Show users disconnecting"), "FilterFlags", GC_EVENT_QUIT, 0, NULL},
	{LPGENT("Show messages"), "FilterFlags", GC_EVENT_MESSAGE, 1, NULL},
	{LPGENT("Show actions"), "FilterFlags", GC_EVENT_ACTION, 1, NULL},
	{LPGENT("Show users leaving"), "FilterFlags", GC_EVENT_PART, 0, NULL},
	{LPGENT("Show users being kicked"), "FilterFlags", GC_EVENT_KICK, 1, NULL},
	{LPGENT("Show notices"), "FilterFlags", GC_EVENT_NOTICE, 1, NULL},
	{LPGENT("Show users changing name"), "FilterFlags", GC_EVENT_NICK, 0, NULL},
	{LPGENT("Show information messages"), "FilterFlags", GC_EVENT_INFORMATION, 1, NULL},
	{LPGENT("Show status changes of users"), "FilterFlags", GC_EVENT_ADDSTATUS, 0, NULL},
};

static struct branch_t branch4[] = {
	{LPGENT("Show icons in tray only when the chat room is not active"), "TrayIconInactiveOnly", 0, 1, NULL},
	{LPGENT("Show icon in tray for topic changes"), "TrayIconFlags", GC_EVENT_TOPIC, 0, NULL},
	{LPGENT("Show icon in tray for users joining"), "TrayIconFlags", GC_EVENT_JOIN, 0, NULL},
	{LPGENT("Show icon in tray for users disconnecting"), "TrayIconFlags", GC_EVENT_QUIT, 0, NULL},
	{LPGENT("Show icon in tray for messages"), "TrayIconFlags", GC_EVENT_MESSAGE, 0, NULL},
	{LPGENT("Show icon in tray for actions"), "TrayIconFlags", GC_EVENT_ACTION, 0, NULL},
	{LPGENT("Show icon in tray for highlights"), "TrayIconFlags", GC_EVENT_HIGHLIGHT, 1, NULL},
	{LPGENT("Show icon in tray for users leaving"), "TrayIconFlags", GC_EVENT_PART, 0, NULL},
	{LPGENT("Show icon in tray for users kicking other user"), "TrayIconFlags", GC_EVENT_KICK, 0, NULL},
	{LPGENT("Show icon in tray for notices "), "TrayIconFlags", GC_EVENT_NOTICE, 0, NULL},
	{LPGENT("Show icon in tray for name changes"), "TrayIconFlags", GC_EVENT_NICK, 0, NULL},
	{LPGENT("Show icon in tray for information messages"), "TrayIconFlags", GC_EVENT_INFORMATION, 0, NULL},
	{LPGENT("Show icon in tray for status changes"), "TrayIconFlags", GC_EVENT_ADDSTATUS, 0, NULL},
};

static struct branch_t branch6[] = {
	{LPGENT("Show pop-ups only when the chat room is not active"), "PopUpInactiveOnly", 0, 1, NULL},
	{LPGENT("Show pop-up for topic changes"), "PopupFlags", GC_EVENT_TOPIC, 0, NULL},
	{LPGENT("Show pop-up for users joining"), "PopupFlags", GC_EVENT_JOIN, 0, NULL},
	{LPGENT("Show pop-up for users disconnecting"), "PopupFlags", GC_EVENT_QUIT, 0, NULL},
	{LPGENT("Show pop-up for messages"), "PopupFlags", GC_EVENT_MESSAGE, 0, NULL},
	{LPGENT("Show pop-up for actions"), "PopupFlags", GC_EVENT_ACTION, 0, NULL},
	{LPGENT("Show pop-up for highlights"), "PopupFlags", GC_EVENT_HIGHLIGHT, 0, NULL},
	{LPGENT("Show pop-up for users leaving"), "PopupFlags", GC_EVENT_PART, 0, NULL},
	{LPGENT("Show pop-up for users kicking other user"), "PopupFlags", GC_EVENT_KICK, 0, NULL},
	{LPGENT("Show pop-up for notices "), "PopupFlags", GC_EVENT_NOTICE, 0, NULL},
	{LPGENT("Show pop-up for name changes"), "PopupFlags", GC_EVENT_NICK, 0, NULL},
	{LPGENT("Show pop-up for information messages"), "PopupFlags", GC_EVENT_INFORMATION, 0, NULL},
	{LPGENT("Show pop-up for status changes"), "PopupFlags", GC_EVENT_ADDSTATUS, 0, NULL},
};

static HTREEITEM InsertBranch(HWND hwndTree, TCHAR* pszDescr, BOOL bExpanded)
{
	TVINSERTSTRUCT tvis;

	tvis.hParent=NULL;
	tvis.hInsertAfter=TVI_LAST;
	tvis.item.mask=TVIF_TEXT|TVIF_STATE;
	tvis.item.pszText=TranslateTS(pszDescr);
	tvis.item.stateMask=bExpanded?TVIS_STATEIMAGEMASK|TVIS_EXPANDED:TVIS_STATEIMAGEMASK;
	tvis.item.state=bExpanded?INDEXTOSTATEIMAGEMASK(1)|TVIS_EXPANDED:INDEXTOSTATEIMAGEMASK(1);
	return TreeView_InsertItem(hwndTree, &tvis);
}

static void FillBranch(HWND hwndTree, HTREEITEM hParent, struct branch_t *branch, int nValues, DWORD defaultval)
{
	TVINSERTSTRUCT tvis;
	int i;
	int iState;

	tvis.hParent=hParent;
	tvis.hInsertAfter=TVI_LAST;
	tvis.item.mask=TVIF_TEXT|TVIF_STATE;
	for(i=0;i<nValues;i++) {
		tvis.item.pszText = TranslateTS(branch[i].szDescr);
		tvis.item.stateMask = TVIS_STATEIMAGEMASK;
		if (branch[i].iMode)
			iState = ((DBGetContactSettingDword(NULL, "Chat", branch[i].szDBName, defaultval)&branch[i].iMode)&branch[i].iMode)!=0?2:1;
		else
			iState = DBGetContactSettingByte(NULL, "Chat", branch[i].szDBName, branch[i].bDefault)!=0?2:1;
		tvis.item.state=INDEXTOSTATEIMAGEMASK(iState);
		branch[i].hItem = TreeView_InsertItem(hwndTree, &tvis);
	}
}
static void SaveBranch(HWND hwndTree, struct branch_t *branch, int nValues)
{
	TVITEM tvi;
	BYTE bChecked;
	int i;
	int iState = 0;

	tvi.mask=TVIF_HANDLE|TVIF_STATE;
	for(i=0;i<nValues;i++) {
		tvi.hItem = branch[i].hItem;
		TreeView_GetItem(hwndTree,&tvi);
		bChecked = ((tvi.state&TVIS_STATEIMAGEMASK)>>12==1)?0:1;
		if(branch[i].iMode)
		{
			if (bChecked)
				iState |= branch[i].iMode;
			if (iState&GC_EVENT_ADDSTATUS)
				iState |= GC_EVENT_REMOVESTATUS;
			DBWriteContactSettingDword(NULL, "Chat", branch[i].szDBName, (DWORD)iState);
		}
		else DBWriteContactSettingByte(NULL, "Chat", branch[i].szDBName, bChecked);
	}	}

static void CheckHeading(HWND hwndTree, HTREEITEM hHeading)
{
	BOOL bChecked = TRUE;
	TVITEM tvi;

	if(hHeading == 0)
		return;

	tvi.mask=TVIF_HANDLE|TVIF_STATE;
	tvi.hItem=TreeView_GetNextItem(hwndTree, hHeading, TVGN_CHILD);
	while(tvi.hItem && bChecked) {
		if(tvi.hItem != branch1[0].hItem && tvi.hItem != branch1[1].hItem )
		{
			TreeView_GetItem(hwndTree,&tvi);
			if(((tvi.state&TVIS_STATEIMAGEMASK)>>12==1))
				bChecked = FALSE;
		}
		tvi.hItem=TreeView_GetNextSibling(hwndTree,tvi.hItem);
	}
	tvi.stateMask = TVIS_STATEIMAGEMASK;
	tvi.state=INDEXTOSTATEIMAGEMASK(bChecked?2:1);
	tvi.hItem = hHeading;
	TreeView_SetItem(hwndTree,&tvi);
}
static void CheckBranches(HWND hwndTree, HTREEITEM hHeading)
{
	BOOL bChecked = TRUE;
	TVITEM tvi;

	if(hHeading == 0)
		return;

	tvi.mask=TVIF_HANDLE|TVIF_STATE;
	tvi.hItem = hHeading;
	TreeView_GetItem(hwndTree,&tvi);
	if(((tvi.state&TVIS_STATEIMAGEMASK)>>12==2))
		bChecked = FALSE;
	tvi.hItem=TreeView_GetNextItem(hwndTree, hHeading, TVGN_CHILD);
	tvi.stateMask = TVIS_STATEIMAGEMASK;
	while(tvi.hItem) {
		tvi.state=INDEXTOSTATEIMAGEMASK(bChecked?2:1);
		if(tvi.hItem != branch1[0].hItem && tvi.hItem != branch1[1].hItem )
			TreeView_SetItem(hwndTree,&tvi);
		tvi.hItem=TreeView_GetNextSibling(hwndTree,tvi.hItem);
	}
}

static INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
	char szDir[MAX_PATH];
	switch(uMsg) {
		case BFFM_INITIALIZED:
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
			break;
		case BFFM_SELCHANGED:
			if (SHGetPathFromIDListA((LPITEMIDLIST) lp ,szDir))
					SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)szDir);
			break;
	}
	return 0;
}

void LoadLogFonts(void)
{
	int i;

	for( i = 0; i<OPTIONS_FONTCOUNT; i++)
		Chat_LoadMsgDlgFont(i, &aFonts[i].lf, &aFonts[i].color);
}

void Chat_LoadMsgDlgFont(int i, LOGFONT* lf, COLORREF* colour)
{
    char str[32];
    int style;
    DBVARIANT dbv;

    if (colour) {
        wsprintfA(str, "Font%dCol", i);
        *colour = DBGetContactSettingDword(NULL, "ChatFonts", str, fontOptionsList[i].defColour);
    }
    if (lf) {
        wsprintfA(str, "Font%dSize", i);
        lf->lfHeight = (char) DBGetContactSettingByte(NULL, "ChatFonts", str, fontOptionsList[i].defSize);
        lf->lfWidth = 0;
        lf->lfEscapement = 0;
        lf->lfOrientation = 0;
        wsprintfA(str, "Font%dSty", i);
        style = DBGetContactSettingByte(NULL, "ChatFonts", str, fontOptionsList[i].defStyle);
        lf->lfWeight = style & FONTF_BOLD ? FW_BOLD : FW_NORMAL;
        lf->lfItalic = style & FONTF_ITALIC ? 1 : 0;
        lf->lfUnderline = 0;
        lf->lfStrikeOut = 0;
        wsprintfA(str, "Font%dSet", i);
        lf->lfCharSet = DBGetContactSettingByte(NULL, "ChatFonts", str, fontOptionsList[i].defCharset);
        lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
        lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
        lf->lfQuality = DEFAULT_QUALITY;
        lf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        wsprintfA(str, "Font%d", i);
        if (DBGetContactSettingTString(NULL, "ChatFonts", str, &dbv))
            lstrcpy(lf->lfFaceName, fontOptionsList[i].szDefFace);
        else {
			lstrcpyn(lf->lfFaceName, dbv.ptszVal, SIZEOF(lf->lfFaceName));
            DBFreeVariant(&dbv);
        }
    }
}

void RegisterFonts( void )
{
	FontIDT fontid = {0};
	ColourIDT colourid;
	char idstr[10];
	int index = 0, i;

	fontid.cbSize = sizeof(FontIDT);
	fontid.flags = FIDF_ALLOWREREGISTER | FIDF_DEFAULTVALID | FIDF_NEEDRESTART;
	for (i = 0; i < SIZEOF(fontOptionsList); i++, index++) {
		strncpy(fontid.dbSettingsGroup, "ChatFonts", sizeof(fontid.dbSettingsGroup));
                mir_sntprintf(fontid.group, SIZEOF(fontid.group), _T("%s/%s"), LPGENT("Messaging"), LPGENT("Group Chats"));
		_tcsncpy(fontid.name, fontOptionsList[i].szDescr, SIZEOF(fontid.name));
		sprintf(idstr, "Font%d", index);
		strncpy(fontid.prefix, idstr, sizeof(fontid.prefix));
		fontid.order = index;

		fontid.deffontsettings.charset = fontOptionsList[i].defCharset;
		fontid.deffontsettings.colour = fontOptionsList[i].defColour;
		fontid.deffontsettings.size = fontOptionsList[i].defSize;
		fontid.deffontsettings.style = fontOptionsList[i].defStyle;
		_tcsncpy(fontid.deffontsettings.szFace, fontOptionsList[i].szDefFace, SIZEOF(fontid.deffontsettings.szFace));
                mir_sntprintf(fontid.backgroundGroup, SIZEOF(fontid.backgroundGroup), _T("%s/%s"), LPGENT("Messaging"), LPGENT("Group Chats"));
		switch (i) {
		case 17:
			_tcsncpy(fontid.backgroundName, _T("Message background"), SIZEOF(fontid.backgroundName));
			break;
		case 18:
		case 19:
			_tcsncpy(fontid.backgroundName, _T("User list background"), SIZEOF(fontid.backgroundName));
			break;
		default:
			_tcsncpy(fontid.backgroundName, _T("Background"), SIZEOF(fontid.backgroundName));
			break;
		}
		CallService(MS_FONT_REGISTERT, (WPARAM)&fontid, 0);
	}

	colourid.cbSize = sizeof(ColourIDT);
	colourid.order = 0;
	strncpy(colourid.dbSettingsGroup, "Chat", sizeof(colourid.dbSettingsGroup));

	strncpy(colourid.setting, "ColorLogBG", SIZEOF(colourid.setting));
	_tcsncpy(colourid.name, LPGENT("Background"), SIZEOF(colourid.name));
        mir_sntprintf(colourid.group, SIZEOF(colourid.group), _T("%s/%s"), LPGENT("Messaging"), LPGENT("Group Chats"));
	colourid.defcolour = GetSysColor(COLOR_WINDOW);
	CallService(MS_COLOUR_REGISTERT, (WPARAM)&colourid, 0);

	strncpy(colourid.setting, "ColorMessageBG", SIZEOF(colourid.setting));
	_tcsncpy(colourid.name, LPGENT("Message background"), SIZEOF(colourid.name));
	colourid.defcolour = GetSysColor(COLOR_WINDOW);
	CallService(MS_COLOUR_REGISTERT, (WPARAM)&colourid, 0);

	strncpy(colourid.setting, "ColorNicklistBG", SIZEOF(colourid.setting));
	_tcsncpy(colourid.name, LPGENT("User list background"), SIZEOF(colourid.name));
	colourid.defcolour = GetSysColor(COLOR_WINDOW);
	CallService(MS_COLOUR_REGISTERT, (WPARAM)&colourid, 0);

	strncpy(colourid.setting, "ColorNicklistLines", SIZEOF(colourid.setting));
	_tcsncpy(colourid.name, LPGENT("User list lines"), SIZEOF(colourid.name));
	colourid.defcolour = GetSysColor(COLOR_INACTIVEBORDER);
	CallService(MS_COLOUR_REGISTERT, (WPARAM)&colourid, 0);

	strncpy(colourid.setting, "ColorNicklistSelectedBG", SIZEOF(colourid.setting));
	_tcsncpy(colourid.name, LPGENT("User list background (selected)"), SIZEOF(colourid.name));
	colourid.defcolour = GetSysColor(COLOR_HIGHLIGHT);
	CallService(MS_COLOUR_REGISTERT, (WPARAM)&colourid, 0);
}


static void InitSetting(TCHAR** ppPointer, char* pszSetting, TCHAR* pszDefault)
{
	DBVARIANT dbv;
	if ( !DBGetContactSettingTString(NULL, "Chat", pszSetting, &dbv )) {
		replaceStr( ppPointer, dbv.ptszVal );
		DBFreeVariant(&dbv);
	}
	else replaceStr( ppPointer, pszDefault );
	}

#define OPT_FIXHEADINGS (WM_USER+1)


INT_PTR CALLBACK DlgProcOptions1(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static HTREEITEM hListHeading1 = 0;
	static HTREEITEM hListHeading4= 0;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		SetWindowLong(GetDlgItem(hwndDlg,IDC_CHAT_CHECKBOXES),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_CHAT_CHECKBOXES),GWL_STYLE)|TVS_NOHSCROLL|TVS_CHECKBOXES);
		SendDlgItemMessage(hwndDlg,IDC_CHAT_SPIN2,UDM_SETRANGE,0,MAKELONG(255,10));
		SendDlgItemMessage(hwndDlg,IDC_CHAT_SPIN2,UDM_SETPOS,0,MAKELONG(DBGetContactSettingByte(NULL,"Chat","NicklistRowDist",12),0));
		hListHeading1 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), TranslateT("Appearance and functionality of chat windows"), DBGetContactSettingByte(NULL, "Chat", "Branch1Exp", 0)?TRUE:FALSE);
		hListHeading4 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), TranslateT("Icons to display in the tray"), DBGetContactSettingByte(NULL, "Chat", "Branch5Exp", 0)?TRUE:FALSE);
		FillBranch(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading1, branch1, SIZEOF(branch1), 0);
		FillBranch(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading4, branch4, SIZEOF(branch4), 0x1000);
		SendMessage(hwndDlg, OPT_FIXHEADINGS, 0, 0);
		{
			TCHAR* pszGroup = NULL;
			InitSetting(&pszGroup, "AddToGroup", _T("Chat rooms"));
			SetWindowText(GetDlgItem(hwndDlg, IDC_CHAT_GROUP), pszGroup);
			mir_free(pszGroup);
		}
		break;

	case OPT_FIXHEADINGS:
		CheckHeading(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading1);
		CheckHeading(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading4);
		break;

	case WM_COMMAND:
		if (	(LOWORD(wParam) == IDC_CHAT_NICKROW
			|| LOWORD(wParam) == IDC_CHAT_GROUP)
				&& (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))	return 0;

		if(lParam != (LPARAM)NULL)
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		break;
	case UM_CHECKSTATECHANGE:
		{
			TVITEM tvi = {0};
			tvi.mask=TVIF_HANDLE|TVIF_STATE;
			tvi.hItem=(HTREEITEM) lParam;
			TreeView_GetItem((HWND)wParam,&tvi);
			if (tvi.hItem == hListHeading1)
				CheckBranches(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading1);
			else if (tvi.hItem == hListHeading4)
				CheckBranches(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading4);
			else
				PostMessage(hwndDlg, OPT_FIXHEADINGS, 0, 0);
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}

	case WM_NOTIFY:
	{
		switch(((LPNMHDR)lParam)->idFrom)
		{
		case IDC_CHAT_CHECKBOXES:
			if(((LPNMHDR)lParam)->code==NM_CLICK) {
				TVHITTESTINFO hti;
				hti.pt.x=(short)LOWORD(GetMessagePos());
				hti.pt.y=(short)HIWORD(GetMessagePos());
				ScreenToClient(((LPNMHDR)lParam)->hwndFrom,&hti.pt);
				if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom,&hti))
					if(hti.flags&TVHT_ONITEMSTATEICON) {
						SendMessage(hwndDlg, UM_CHECKSTATECHANGE, (WPARAM)((LPNMHDR)lParam)->hwndFrom, (LPARAM)hti.hItem);
					}

			} else if (((LPNMHDR) lParam)->code == TVN_KEYDOWN) {
				if (((LPNMTVKEYDOWN) lParam)->wVKey == VK_SPACE) {
					SendMessage(hwndDlg, UM_CHECKSTATECHANGE, (WPARAM)((LPNMHDR)lParam)->hwndFrom,
						(LPARAM)TreeView_GetSelection(((LPNMHDR)lParam)->hwndFrom));
				}
			}
			break;

			case 0:
				switch (((LPNMHDR)lParam)->code)
				{
				case PSN_APPLY:
					{
						int iLen;
						char * pszText = NULL;

						iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_CHAT_GROUP));
						if(iLen > 0)
						{
							pszText = mir_realloc(pszText, iLen+1);
							GetDlgItemTextA(hwndDlg, IDC_CHAT_GROUP, pszText,iLen+1);
							DBWriteContactSettingString(NULL, "Chat", "AddToGroup", pszText);
						}
						else DBWriteContactSettingString(NULL, "Chat", "AddToGroup", "");
						mir_free(pszText);

						iLen = SendDlgItemMessage(hwndDlg,IDC_CHAT_SPIN2,UDM_GETPOS,0,0);
						if(iLen > 0)
							DBWriteContactSettingByte(NULL, "Chat", "NicklistRowDist", (BYTE)iLen);
						else
							DBDeleteContactSetting(NULL, "Chat", "NicklistRowDist");
						SaveBranch(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), branch1, SIZEOF(branch1));
						SaveBranch(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), branch4, SIZEOF(branch4));

						SM_BroadcastMessage(NULL, GC_SETWNDPROPS, 0, 0, TRUE);
					}
					return TRUE;
				}
		}
	}break;
	case WM_DESTROY:
		{
		BYTE b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading1, TVIS_EXPANDED)&TVIS_EXPANDED?1:0;
		DBWriteContactSettingByte(NULL, "Chat", "Branch1Exp", b);
		b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading4, TVIS_EXPANDED)&TVIS_EXPANDED?1:0;
		DBWriteContactSettingByte(NULL, "Chat", "Branch5Exp", b);
		}break;

	default:break;
	}
	return FALSE;
}

BOOL CALLBACK DlgProcOptions2(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static HTREEITEM hListHeading2= 0;
	static HTREEITEM hListHeading3= 0;
	switch (uMsg) {
	case WM_INITDIALOG:
	{
		TCHAR tszTemp[MAX_PATH];

		TranslateDialogDefault(hwndDlg);
		SetWindowLong(GetDlgItem(hwndDlg,IDC_CHAT_CHECKBOXES),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_CHAT_CHECKBOXES),GWL_STYLE)|TVS_NOHSCROLL|TVS_CHECKBOXES);
		SendDlgItemMessage(hwndDlg,IDC_CHAT_SPIN2,UDM_SETRANGE,0,MAKELONG(5000,0));
		SendDlgItemMessage(hwndDlg,IDC_CHAT_SPIN2,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"Chat","LogLimit",100),0));
		SendDlgItemMessage(hwndDlg,IDC_CHAT_SPIN3,UDM_SETRANGE,0,MAKELONG(10000,0));
		SendDlgItemMessage(hwndDlg,IDC_CHAT_SPIN3,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"Chat","LoggingLimit",100),0));
		CallService(MS_UTILS_PATHTORELATIVET, (WPARAM)g_Settings.pszLogDir, (LPARAM)tszTemp );
		SetDlgItemText(hwndDlg, IDC_CHAT_LOGDIRECTORY, tszTemp);

		if (ServiceExists(MS_UTILS_REPLACEVARS)) {
			TCHAR tszTooltipText[2048];
			RECT rect;

			mir_sntprintf(tszTooltipText, SIZEOF(tszTooltipText), 
				_T("%s - %s\n%s - %s\n%s - %s\n\n")
				_T("%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n\n")
				_T("%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s"),
				// contact vars
				_T("%nick%"),					TranslateT("nick of current contact (if defined)"),
				_T("%proto%"),					TranslateT("protocol name of current contact (if defined). Account name is used when protocol supports multiaccounts"),
				_T("%userid%"),					TranslateT("UserID of current contact (if defined). It is like UIN Number for ICQ, JID for Jabber, etc."),
				// global vars
				_T("%miranda_path%"),			TranslateT("path to root miranda folder"),
				_T("%miranda_profile%"),		TranslateT("path to current miranda profile"),
				_T("%miranda_profilename%"),	TranslateT("name of current miranda profile (filename, without extension)"),
				_T("%miranda_userdata%"),		TranslateT("will return parsed string %miranda_profile%\\Profiles\\%miranda_profilename%"),
				_T("%appdata%"),				TranslateT("same as environment variable %APPDATA% for currently logged-on Windows user"),
				_T("%username%"),				TranslateT("username for currently logged-on Windows user"),
				_T("%mydocuments%"),			TranslateT("\"My Documents\" folder for currently logged-on Windows user"),
				_T("%desktop%"),				TranslateT("\"Desktop\" folder for currently logged-on Windows user"),
				_T("%xxxxxxx%"),				TranslateT("any environment variable defined in current Windows session (like %systemroot%, %allusersprofile%, etc.)"),
				// date/time vars
				_T("%d%"),			TranslateT("day of month, 1-31"),
				_T("%dd%"),			TranslateT("day of month, 01-31"),
				_T("%m%"),			TranslateT("month number, 1-12"),
				_T("%mm%"),			TranslateT("month number, 01-12"),
				_T("%mon%"),		TranslateT("abbreviated month name"),
				_T("%month%"),		TranslateT("full month name"),
				_T("%yy%"),			TranslateT("year without century, 01-  99"),
				_T("%yyyy%"),		TranslateT("year with century, 1901-9999"),
				_T("%wday%"),		TranslateT("abbreviated weekday name"),
				_T("%weekday%"),	TranslateT("full weekday name") );
			GetClientRect (GetDlgItem(hwndDlg, IDC_CHAT_LOGDIRECTORY), &rect);
			rect.left = -85;
			hPathTip = CreateToolTip(GetDlgItem(hwndDlg, IDC_CHAT_LOGDIRECTORY), tszTooltipText, TranslateT("Variables"), &rect);
			SetTimer(hwndDlg, 0, 3000, NULL);
		}


		SetDlgItemText(hwndDlg, IDC_CHAT_HIGHLIGHTWORDS, g_Settings.pszHighlightWords);
		SetDlgItemText(hwndDlg, IDC_CHAT_LOGTIMESTAMP, g_Settings.pszTimeStampLog);
		SetDlgItemText(hwndDlg, IDC_CHAT_TIMESTAMP, g_Settings.pszTimeStamp);
		SetDlgItemText(hwndDlg, IDC_CHAT_OUTSTAMP, g_Settings.pszOutgoingNick);
		SetDlgItemText(hwndDlg, IDC_CHAT_INSTAMP, g_Settings.pszIncomingNick);
		CheckDlgButton(hwndDlg, IDC_CHAT_HIGHLIGHT, g_Settings.HighlightEnabled);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_HIGHLIGHTWORDS), g_Settings.HighlightEnabled?TRUE:FALSE);
		CheckDlgButton(hwndDlg, IDC_CHAT_LOGGING, g_Settings.LoggingEnabled);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_LOGDIRECTORY), g_Settings.LoggingEnabled?TRUE:FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_LOGDIRCHOOSE), g_Settings.LoggingEnabled?TRUE:FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_LIMIT), g_Settings.LoggingEnabled?TRUE:FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_LIMITTEXT2), g_Settings.LoggingEnabled?TRUE:FALSE);

		hListHeading2 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), TranslateT("Appearance"), DBGetContactSettingByte(NULL, "Chat", "Branch2Exp", 0)?TRUE:FALSE);
		hListHeading3 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), TranslateT("Default events to show in new chat rooms if the \'event filter\' is enabled"), DBGetContactSettingByte(NULL, "Chat", "Branch3Exp", 0)?TRUE:FALSE);
		FillBranch(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading2, branch2, SIZEOF(branch2), 0x0);
		FillBranch(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading3, branch3, SIZEOF(branch3), 0x03E0);
		SendMessage(hwndDlg, OPT_FIXHEADINGS, 0, 0);

		break;
	}
	case OPT_FIXHEADINGS:
		CheckHeading(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading2);
		CheckHeading(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading3);
		break;
	case WM_COMMAND:
		if(	(LOWORD(wParam)		  == IDC_CHAT_INSTAMP
				|| LOWORD(wParam) == IDC_CHAT_OUTSTAMP
				|| LOWORD(wParam) == IDC_CHAT_TIMESTAMP
				|| LOWORD(wParam) == IDC_CHAT_LOGLIMIT
				|| LOWORD(wParam) == IDC_CHAT_HIGHLIGHTWORDS
				|| LOWORD(wParam) == IDC_CHAT_LOGDIRECTORY
				|| LOWORD(wParam) == IDC_CHAT_LOGTIMESTAMP
				|| LOWORD(wParam) == IDC_CHAT_LIMIT)
				&& (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))	return 0;

		switch (LOWORD(wParam)) {
		case IDC_CHAT_LOGGING:
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_LOGDIRECTORY), IsDlgButtonChecked(hwndDlg, IDC_CHAT_LOGGING) == BST_CHECKED?TRUE:FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_LOGDIRCHOOSE), IsDlgButtonChecked(hwndDlg, IDC_CHAT_LOGGING) == BST_CHECKED?TRUE:FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_LIMIT), IsDlgButtonChecked(hwndDlg, IDC_CHAT_LOGGING) == BST_CHECKED?TRUE:FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_LIMITTEXT2), IsDlgButtonChecked(hwndDlg, IDC_CHAT_LOGGING) == BST_CHECKED?TRUE:FALSE);
			break;
		case  IDC_CHAT_LOGDIRCHOOSE:
		{
			TCHAR tszDirectory[MAX_PATH];
			LPITEMIDLIST idList;
			LPMALLOC psMalloc;
			BROWSEINFO bi = {0};

			if(SUCCEEDED(CoGetMalloc(1,&psMalloc)))
			{
				TCHAR tszTemp[MAX_PATH];
				bi.hwndOwner=hwndDlg;
				bi.pszDisplayName=tszDirectory;
				bi.lpszTitle=TranslateT("Select Folder");
				bi.ulFlags=BIF_NEWDIALOGSTYLE|BIF_EDITBOX|BIF_RETURNONLYFSDIRS;
				bi.lpfn=BrowseCallbackProc;
				bi.lParam=(LPARAM)tszDirectory;

				idList=SHBrowseForFolder(&bi);
				if(idList) {
                                    SHGetPathFromIDList(idList,tszDirectory);
                                    lstrcat(tszDirectory, _T("\\"));
                                    CallService(MS_UTILS_PATHTORELATIVET, (WPARAM)tszDirectory, (LPARAM)tszTemp );
                                    SetWindowText(GetDlgItem(hwndDlg, IDC_CHAT_LOGDIRECTORY), lstrlen(tszTemp) > 1?tszTemp:DEFLOGFILENAME);
				}
				psMalloc->lpVtbl->Free(psMalloc,idList);
				psMalloc->lpVtbl->Release(psMalloc);
			}
			break;
		}
		case IDC_CHAT_HIGHLIGHT:
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_HIGHLIGHTWORDS), IsDlgButtonChecked(hwndDlg, IDC_CHAT_HIGHLIGHT) == BST_CHECKED?TRUE:FALSE);
			break;
		}

		if(lParam != (LPARAM)NULL)
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		break;

	case UM_CHECKSTATECHANGE:
		{
			TVITEM tvi = {0};
			tvi.mask=TVIF_HANDLE|TVIF_STATE;
			tvi.hItem=(HTREEITEM) lParam;
			TreeView_GetItem((HWND)wParam,&tvi);
			if (tvi.hItem == hListHeading2)
				CheckBranches(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading2);
			else if (tvi.hItem == hListHeading3)
				CheckBranches(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading3);
			else
				PostMessage(hwndDlg, OPT_FIXHEADINGS, 0, 0);
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->idFrom == IDC_CHAT_CHECKBOXES) {
			if (((LPNMHDR)lParam)->code==NM_CLICK) {
				TVHITTESTINFO hti;
				hti.pt.x=(short)LOWORD(GetMessagePos());
				hti.pt.y=(short)HIWORD(GetMessagePos());
				ScreenToClient(((LPNMHDR)lParam)->hwndFrom,&hti.pt);
				if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom,&hti))
					if(hti.flags&TVHT_ONITEMSTATEICON) {
						SendMessage(hwndDlg, UM_CHECKSTATECHANGE, (WPARAM)((LPNMHDR)lParam)->hwndFrom, (LPARAM)hti.hItem);
					}

			} else if (((LPNMHDR) lParam)->code == TVN_KEYDOWN) {
				if (((LPNMTVKEYDOWN) lParam)->wVKey == VK_SPACE) {
					SendMessage(hwndDlg, UM_CHECKSTATECHANGE, (WPARAM)((LPNMHDR)lParam)->hwndFrom,
						(LPARAM)TreeView_GetSelection(((LPNMHDR)lParam)->hwndFrom));
				}
			}
		} else if (((LPNMHDR)lParam)->idFrom == 0 && ((LPNMHDR)lParam)->code == PSN_APPLY ) {
			char *pszText = NULL;
			int iLen;

			iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_CHAT_HIGHLIGHTWORDS));
			if ( iLen > 0 ) {
				TCHAR *ptszText = mir_alloc((iLen+2) * sizeof(TCHAR));
				TCHAR *p2 = NULL;

				if(ptszText) {
				    GetDlgItemText(hwndDlg, IDC_CHAT_HIGHLIGHTWORDS, ptszText, iLen + 1);
				    p2 = _tcschr(ptszText, (TCHAR)',');
				    while ( p2 ) {
					   *p2 = ' ';
					   p2 = _tcschr(ptszText, (TCHAR)',');
				    }
				    DBWriteContactSettingTString(NULL, "Chat", "HighlightWords", ptszText);
				    mir_free(ptszText);
				}
			}
			else DBDeleteContactSetting(NULL, "Chat", "HighlightWords");

			iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_CHAT_LOGDIRECTORY));
			if ( iLen > 0 ) {
				TCHAR *pszText1 = malloc(iLen*sizeof(TCHAR) + 2);
				GetDlgItemText(hwndDlg, IDC_CHAT_LOGDIRECTORY, pszText1, iLen + 1);
				DBWriteContactSettingTString(NULL, "Chat", "LogDirectory", pszText1);
				CallService(MS_UTILS_PATHTOABSOLUTET, (WPARAM)pszText1, (LPARAM)g_Settings.pszLogDir);
				free(pszText1);
			}
			else {
				lstrcpyn(g_Settings.pszLogDir, DEFLOGFILENAME, MAX_PATH);
				DBDeleteContactSetting(NULL, "Chat", "LogDirectory");
			}

			iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_CHAT_LOGTIMESTAMP));
			if ( iLen > 0 ) {
                pszText = mir_realloc(pszText, iLen+1);
				GetDlgItemTextA(hwndDlg, IDC_CHAT_LOGTIMESTAMP, pszText,iLen+1);
				DBWriteContactSettingString(NULL, "Chat", "LogTimestamp", pszText);
			}
			else DBDeleteContactSetting(NULL, "Chat", "LogTimestamp");

			iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_CHAT_TIMESTAMP));
			if( iLen > 0 ) {
                pszText = mir_realloc(pszText, iLen+1);
				GetDlgItemTextA(hwndDlg, IDC_CHAT_TIMESTAMP, pszText,iLen+1);
				DBWriteContactSettingString(NULL, "Chat", "HeaderTime", pszText);
			}
			else DBDeleteContactSetting(NULL, "Chat", "HeaderTime");

			iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_CHAT_INSTAMP));
			if ( iLen > 0 ) {
                pszText = mir_realloc(pszText, iLen+1);
				GetDlgItemTextA(hwndDlg, IDC_CHAT_INSTAMP, pszText,iLen+1);
				DBWriteContactSettingString(NULL, "Chat", "HeaderIncoming", pszText);
			}
			else DBDeleteContactSetting(NULL, "Chat", "HeaderIncoming");

			iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_CHAT_OUTSTAMP));
			if( iLen > 0 ) {
			pszText = mir_realloc(pszText, iLen+1);
				GetDlgItemTextA(hwndDlg, IDC_CHAT_OUTSTAMP, pszText,iLen+1);
				DBWriteContactSettingString(NULL, "Chat", "HeaderOutgoing", pszText);
			}
			else DBDeleteContactSetting(NULL, "Chat", "HeaderOutgoing");

			g_Settings.HighlightEnabled = IsDlgButtonChecked(hwndDlg, IDC_CHAT_HIGHLIGHT) == BST_CHECKED?TRUE:FALSE;
			DBWriteContactSettingByte(NULL, "Chat", "HighlightEnabled", (BYTE)g_Settings.HighlightEnabled);

			g_Settings.LoggingEnabled = IsDlgButtonChecked(hwndDlg, IDC_CHAT_LOGGING) == BST_CHECKED?TRUE:FALSE;
			DBWriteContactSettingByte(NULL, "Chat", "LoggingEnabled", (BYTE)g_Settings.LoggingEnabled);

			iLen = SendDlgItemMessage(hwndDlg,IDC_CHAT_SPIN2,UDM_GETPOS,0,0);
			DBWriteContactSettingWord(NULL, "Chat", "LogLimit", (WORD)iLen);
			iLen = SendDlgItemMessage(hwndDlg,IDC_CHAT_SPIN3,UDM_GETPOS,0,0);
			DBWriteContactSettingWord(NULL, "Chat", "LoggingLimit", (WORD)iLen);

			SaveBranch(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), branch2, SIZEOF(branch2));
			SaveBranch(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), branch3, SIZEOF(branch3));

			mir_free(pszText);

			g_Settings.dwIconFlags = DBGetContactSettingDword(NULL, "Chat", "IconFlags", 0x0000);
			g_Settings.dwTrayIconFlags = DBGetContactSettingDword(NULL, "Chat", "TrayIconFlags", 0x1000);
			g_Settings.dwPopupFlags = DBGetContactSettingDword(NULL, "Chat", "PopupFlags", 0x0000);
			g_Settings.StripFormat = (BOOL)DBGetContactSettingByte(NULL, "Chat", "TrimFormatting", 0);
			g_Settings.TrayIconInactiveOnly = (BOOL)DBGetContactSettingByte(NULL, "Chat", "TrayIconInactiveOnly", 1);
			g_Settings.PopUpInactiveOnly = (BOOL)DBGetContactSettingByte(NULL, "Chat", "PopUpInactiveOnly", 1);

			g_Settings.LogIndentEnabled = (DBGetContactSettingByte(NULL, "Chat", "LogIndentEnabled", 1) != 0)?TRUE:FALSE;
			MM_FontsChanged();
			SM_BroadcastMessage(NULL, GC_SETWNDPROPS, 0, 0, TRUE);
			return TRUE;
		}
		break;
	case WM_TIMER:
		if(IsWindow(hPathTip))
			KillTimer(hPathTip, 4); // It will prevent tooltip autoclosing
		break;
	case WM_DESTROY:
		if (hPathTip)
		{
			KillTimer(hwndDlg, 0);
			DestroyWindow(hPathTip);
			hPathTip = 0;
		}
		{
			BYTE b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading2, TVIS_EXPANDED)&TVIS_EXPANDED?1:0;
			DBWriteContactSettingByte(NULL, "Chat", "Branch2Exp", b);
			b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), hListHeading3, TVIS_EXPANDED)&TVIS_EXPANDED?1:0;
			DBWriteContactSettingByte(NULL, "Chat", "Branch3Exp", b);
		}
		break;
	}
	return FALSE;
}

static INT_PTR CALLBACK DlgProcOptionsPopup(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);

			SetWindowLong(GetDlgItem(hwndDlg,IDC_CHAT_CHECKBOXES),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_CHAT_CHECKBOXES),GWL_STYLE)|TVS_NOHSCROLL|TVS_CHECKBOXES);
			SendDlgItemMessage(hwndDlg, IDC_CHAT_BKG, CPM_SETCOLOUR,0,g_Settings.crPUBkgColour);
			SendDlgItemMessage(hwndDlg, IDC_CHAT_TEXT, CPM_SETCOLOUR,0,g_Settings.crPUTextColour);

			if(g_Settings.iPopupStyle ==2)
				CheckDlgButton(hwndDlg, IDC_CHAT_RADIO2, BST_CHECKED);
			else if(g_Settings.iPopupStyle ==3)
				CheckDlgButton(hwndDlg, IDC_CHAT_RADIO3, BST_CHECKED);
			else
				CheckDlgButton(hwndDlg, IDC_CHAT_RADIO1, BST_CHECKED);

			EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_BKG), IsDlgButtonChecked(hwndDlg, IDC_CHAT_RADIO3) ==BST_CHECKED?TRUE:FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_TEXT), IsDlgButtonChecked(hwndDlg, IDC_CHAT_RADIO3) ==BST_CHECKED?TRUE:FALSE);

			SendDlgItemMessage(hwndDlg,IDC_CHAT_SPIN1,UDM_SETRANGE,0,MAKELONG(100,-1));
			SendDlgItemMessage(hwndDlg,IDC_CHAT_SPIN1,UDM_SETPOS,0,MAKELONG(g_Settings.iPopupTimeout,0));
			//hListHeading6 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), TranslateT("Pop-ups to display"), TRUE);
			FillBranch(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), NULL, branch6, SIZEOF(branch6), 0x0000);
		}break;

	case WM_COMMAND:
		if(	(LOWORD(wParam)		  == IDC_CHAT_TIMEOUT)
				&& (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))	return 0;

		if(lParam != (LPARAM)NULL)
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		switch (LOWORD(wParam)) {

		case IDC_CHAT_RADIO1:
		case IDC_CHAT_RADIO2:
		case IDC_CHAT_RADIO3:
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_BKG), IsDlgButtonChecked(hwndDlg, IDC_CHAT_RADIO3) ==BST_CHECKED?TRUE:FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHAT_TEXT), IsDlgButtonChecked(hwndDlg, IDC_CHAT_RADIO3) ==BST_CHECKED?TRUE:FALSE);
			break;

		default:break;
		}

		break;

	case WM_NOTIFY:
	{
		switch(((LPNMHDR)lParam)->idFrom)
		{
			case IDC_CHAT_CHECKBOXES:
				if(((LPNMHDR)lParam)->code==NM_CLICK) {
					TVHITTESTINFO hti;
					hti.pt.x=(short)LOWORD(GetMessagePos());
					hti.pt.y=(short)HIWORD(GetMessagePos());
					ScreenToClient(((LPNMHDR)lParam)->hwndFrom,&hti.pt);
					if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom,&hti))
						if(hti.flags&TVHT_ONITEMSTATEICON) {
							SendMessage(hwndDlg, UM_CHECKSTATECHANGE, (WPARAM)((LPNMHDR)lParam)->hwndFrom, (LPARAM)hti.hItem);
						}

				} else if (((LPNMHDR) lParam)->code == TVN_KEYDOWN) {
					if (((LPNMTVKEYDOWN) lParam)->wVKey == VK_SPACE) {
						SendMessage(hwndDlg, UM_CHECKSTATECHANGE, (WPARAM)((LPNMHDR)lParam)->hwndFrom,
							(LPARAM)TreeView_GetSelection(((LPNMHDR)lParam)->hwndFrom));
					}
				}
			break;
			case 0:
				switch (((LPNMHDR)lParam)->code)
				{
				case PSN_APPLY:
					{
						int iLen;

						if(IsDlgButtonChecked(hwndDlg, IDC_CHAT_RADIO2) == BST_CHECKED)
							iLen = 2;
						else if(IsDlgButtonChecked(hwndDlg, IDC_CHAT_RADIO3) == BST_CHECKED)
							iLen = 3;
						else
							iLen = 1;

						g_Settings.iPopupStyle = iLen;
						DBWriteContactSettingByte(NULL, "Chat", "PopupStyle", (BYTE)iLen);

						iLen = SendDlgItemMessage(hwndDlg,IDC_CHAT_SPIN1,UDM_GETPOS,0,0);
						g_Settings.iPopupTimeout = iLen;
						DBWriteContactSettingWord(NULL, "Chat", "PopupTimeout", (WORD)iLen);

						g_Settings.crPUBkgColour = SendDlgItemMessage(hwndDlg,IDC_CHAT_BKG,CPM_GETCOLOUR,0,0);
						DBWriteContactSettingDword(NULL, "Chat", "PopupColorBG", (DWORD)SendDlgItemMessage(hwndDlg,IDC_CHAT_BKG,CPM_GETCOLOUR,0,0));
						g_Settings.crPUTextColour = SendDlgItemMessage(hwndDlg,IDC_CHAT_TEXT,CPM_GETCOLOUR,0,0);
						DBWriteContactSettingDword(NULL, "Chat", "PopupColorText", (DWORD)SendDlgItemMessage(hwndDlg,IDC_CHAT_TEXT,CPM_GETCOLOUR,0,0));
						SaveBranch(GetDlgItem(hwndDlg, IDC_CHAT_CHECKBOXES), branch6, SIZEOF(branch6));
					}
					return TRUE;
				}
		}
	}break;
	case UM_CHECKSTATECHANGE:
		{
			PostMessage(hwndDlg, OPT_FIXHEADINGS, 0, 0);
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}

	default:break;
	}
	return FALSE;
}

static int OptionsInitialize(WPARAM wParam, LPARAM lParam)
{

	OPTIONSDIALOGPAGE odp = {0};
	if(g_dat->popupInstalled)
	{
		odp.cbSize = sizeof(odp);
		odp.position = 910000002;
		odp.hInstance = g_hInst;
		odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONSPOPUP);
		odp.ptszTitle = LPGENT("Messaging");
		odp.ptszGroup = LPGENT("Popups");
		odp.pfnDlgProc = DlgProcOptionsPopup;
		odp.flags = ODPF_BOLDGROUPS | ODPF_TCHAR;
		odp.ptszTab = NULL;
		CallService(MS_OPT_ADDPAGE, wParam, (LPARAM)&odp);
	}

	return 0;
}

void LoadGlobalSettings(void)
{
	LOGFONT lf;

	g_Settings.LogLimitNames = DBGetContactSettingByte(NULL, "Chat", "LogLimitNames", 1);
	g_Settings.ShowTime = DBGetContactSettingByte(NULL, "Chat", "ShowTimeStamp", 1);
	g_Settings.SoundsFocus = DBGetContactSettingByte(NULL, "Chat", "SoundsFocus", 0);
	g_Settings.ShowTimeIfChanged = (BOOL)DBGetContactSettingByte(NULL, "Chat", "ShowTimeStampIfChanged", 0);
	g_Settings.TimeStampEventColour = (BOOL)DBGetContactSettingByte(NULL, "Chat", "TimeStampEventColour", 0);
	g_Settings.iEventLimit = DBGetContactSettingWord(NULL, "Chat", "LogLimit", 100);
	g_Settings.dwIconFlags = DBGetContactSettingDword(NULL, "Chat", "IconFlags", 0x0000);
	g_Settings.dwTrayIconFlags = DBGetContactSettingDword(NULL, "Chat", "TrayIconFlags", 0x1000);
	g_Settings.dwPopupFlags = DBGetContactSettingDword(NULL, "Chat", "PopupFlags", 0x0000);
	g_Settings.LoggingLimit = DBGetContactSettingWord(NULL, "Chat", "LoggingLimit", 100);
	g_Settings.LoggingEnabled = (BOOL)DBGetContactSettingByte(NULL, "Chat", "LoggingEnabled", 0);
	g_Settings.FlashWindow = (BOOL)DBGetContactSettingByte(NULL, "Chat", "FlashWindow", 0);
	g_Settings.HighlightEnabled = (BOOL)DBGetContactSettingByte(NULL, "Chat", "HighlightEnabled", 1);
	g_Settings.crUserListColor = DBGetContactSettingDword(NULL, "ChatFonts", "Font18Col", RGB(0,0,0));
	g_Settings.crUserListBGColor = DBGetContactSettingDword(NULL, "Chat", "ColorNicklistBG", GetSysColor(COLOR_WINDOW));
	g_Settings.crUserListSelectedBGColor = DBGetContactSettingDword(NULL, "Chat", "ColorNicklistSelectedBG", GetSysColor(COLOR_HIGHLIGHT));
	g_Settings.crUserListHeadingsColor = DBGetContactSettingDword(NULL, "ChatFonts", "Font19Col", RGB(170,170,170));
	g_Settings.crLogBackground = DBGetContactSettingDword(NULL, "Chat", "ColorLogBG", GetSysColor(COLOR_WINDOW));
	g_Settings.StripFormat = (BOOL)DBGetContactSettingByte(NULL, "Chat", "StripFormatting", 0);
	g_Settings.TrayIconInactiveOnly = (BOOL)DBGetContactSettingByte(NULL, "Chat", "TrayIconInactiveOnly", 1);
	g_Settings.PopUpInactiveOnly = (BOOL)DBGetContactSettingByte(NULL, "Chat", "PopUpInactiveOnly", 1);
	g_Settings.AddColonToAutoComplete = (BOOL)DBGetContactSettingByte(NULL, "Chat", "AddColonToAutoComplete", 1);
	g_Settings.iPopupStyle = DBGetContactSettingByte(NULL, "Chat", "PopupStyle", 1);
	g_Settings.iPopupTimeout = DBGetContactSettingWord(NULL, "Chat", "PopupTimeout", 3);
	g_Settings.crPUBkgColour = DBGetContactSettingDword(NULL, "Chat", "PopupColorBG", GetSysColor(COLOR_WINDOW));
	g_Settings.crPUTextColour = DBGetContactSettingDword(NULL, "Chat", "PopupColorText", 0);
	g_Settings.ShowContactStatus = DBGetContactSettingByte(NULL, "Chat", "ShowContactStatus", 0);
	g_Settings.ContactStatusFirst = DBGetContactSettingByte(NULL, "Chat", "ContactStatusFirst", 0);

	InitSetting( &g_Settings.pszTimeStamp, "HeaderTime", _T("[%H:%M]"));
	InitSetting( &g_Settings.pszTimeStampLog, "LogTimestamp", _T("[%d %b %y %H:%M]"));
	InitSetting( &g_Settings.pszIncomingNick, "HeaderIncoming", _T("%n:"));
	InitSetting( &g_Settings.pszOutgoingNick, "HeaderOutgoing", _T("%n:"));
	InitSetting( &g_Settings.pszHighlightWords, "HighlightWords", _T("%m"));

	{
	DBVARIANT dbv;
		g_Settings.pszLogDir = (TCHAR *)mir_realloc(g_Settings.pszLogDir, MAX_PATH*sizeof(TCHAR));
		if (!DBGetContactSettingTString(NULL, "Chat", "LogDirectory", &dbv)) {
			lstrcpyn(g_Settings.pszLogDir, dbv.ptszVal, MAX_PATH);
			DBFreeVariant(&dbv);
		} else lstrcpyn(g_Settings.pszLogDir, DEFLOGFILENAME, MAX_PATH);
	}

	g_Settings.LogIndentEnabled = (DBGetContactSettingByte(NULL, "Chat", "LogIndentEnabled", 1) != 0)?TRUE:FALSE;

	if(g_Settings.MessageBoxFont)
		DeleteObject(g_Settings.MessageBoxFont);
	Chat_LoadMsgDlgFont(17, &lf, NULL);
	g_Settings.MessageBoxFont = CreateFontIndirect(&lf);

	if(g_Settings.UserListFont)
		DeleteObject(g_Settings.UserListFont);
	Chat_LoadMsgDlgFont(18, &lf, NULL);
	g_Settings.UserListFont = CreateFontIndirect(&lf);

	if(g_Settings.UserListHeadingsFont)
		DeleteObject(g_Settings.UserListHeadingsFont);
	Chat_LoadMsgDlgFont(19, &lf, NULL);
	g_Settings.UserListHeadingsFont = CreateFontIndirect(&lf);
	if (hListBkgBrush != NULL) {
		DeleteObject(hListBkgBrush);
	}
	hListBkgBrush = CreateSolidBrush(DBGetContactSettingDword(NULL, "Chat", "ColorNicklistBG", GetSysColor(COLOR_WINDOW)));
	if (hListSelectedBkgBrush != NULL) {
		DeleteObject(hListSelectedBkgBrush);
	}
	hListSelectedBkgBrush = CreateSolidBrush(DBGetContactSettingDword(NULL, "Chat", "ColorNicklistSelectedBG", GetSysColor(COLOR_HIGHLIGHT)));
}

static void FreeGlobalSettings(void)
{
	mir_free(g_Settings.pszTimeStamp);
	mir_free(g_Settings.pszTimeStampLog);
	mir_free(g_Settings.pszIncomingNick);
	mir_free(g_Settings.pszOutgoingNick);
	mir_free(g_Settings.pszHighlightWords);
	mir_free(g_Settings.pszLogDir);
	if(g_Settings.MessageBoxFont)
		DeleteObject(g_Settings.MessageBoxFont);
	if(g_Settings.UserListFont)
		DeleteObject(g_Settings.UserListFont);
	if(g_Settings.UserListHeadingsFont)
		DeleteObject(g_Settings.UserListHeadingsFont);
}

void SetIndentSize() 
{
	if (g_Settings.ShowTime) {
		LOGFONT lf;
		HFONT hFont;
		int iText;

		Chat_LoadMsgDlgFont(0, &lf, NULL);
		hFont = CreateFontIndirect(&lf);
		iText = GetTextPixelSize(MakeTimeStamp(g_Settings.pszTimeStamp, time(NULL)),hFont, TRUE);
		DeleteObject(hFont);
		g_Settings.LogTextIndent = iText*12/10;
	} else {
		g_Settings.LogTextIndent = 0;
	}

}

int OptionsInit(void)
{
	LOGFONT lf;

	g_hOptions = HookEvent(ME_OPT_INITIALISE, OptionsInitialize);

	LoadLogFonts();
	Chat_LoadMsgDlgFont(18, &lf, NULL);
	lstrcpy(lf.lfFaceName, _T("MS Shell Dlg"));
	lf.lfUnderline = lf.lfItalic = lf.lfStrikeOut = 0;
	lf.lfHeight = -17;
	lf.lfWeight = FW_BOLD;
	g_Settings.NameFont = CreateFontIndirect(&lf);
	g_Settings.UserListFont = NULL;
	g_Settings.UserListHeadingsFont = NULL;
	g_Settings.MessageBoxFont = NULL;
	g_Settings.iSplitterX = DBGetContactSettingWord(NULL, "Chat", "SplitterX", 105);
	g_Settings.iSplitterY = DBGetContactSettingWord(NULL, "Chat", "SplitterY", 90);
	LoadGlobalSettings();

	SkinAddNewSoundEx("ChatMessage", LPGEN("Group chats"), LPGEN("Incoming message"));
	SkinAddNewSoundEx("ChatHighlight", LPGEN("Group chats"), LPGEN("Message is highlighted"));
	SkinAddNewSoundEx("ChatAction", LPGEN("Group chats"), LPGEN("User has performed an action"));
	SkinAddNewSoundEx("ChatJoin", LPGEN("Group chats"), LPGEN("User has joined"));
	SkinAddNewSoundEx("ChatPart", LPGEN("Group chats"), LPGEN("User has left"));
	SkinAddNewSoundEx("ChatKick", LPGEN("Group chats"), LPGEN("User has kicked some other user"));
	SkinAddNewSoundEx("ChatMode", LPGEN("Group chats"), LPGEN("User's status was changed"));
	SkinAddNewSoundEx("ChatNick", LPGEN("Group chats"), LPGEN("User has changed name"));
	SkinAddNewSoundEx("ChatNotice", LPGEN("Group chats"), LPGEN("User has sent a notice"));
	SkinAddNewSoundEx("ChatQuit", LPGEN("Group chats"), LPGEN("User has disconnected"));
	SkinAddNewSoundEx("ChatTopic", LPGEN("Group chats"), LPGEN("The topic has been changed"));
	SetIndentSize();
	return 0;
}


int OptionsUnInit(void)
{
	FreeGlobalSettings();
	UnhookEvent(g_hOptions);
	DeleteObject(hListBkgBrush);
	DeleteObject(hListSelectedBkgBrush);
	DeleteObject(g_Settings.NameFont);
	return 0;
}
