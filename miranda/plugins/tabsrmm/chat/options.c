/*
astyle --force-indent=tab=4 --brackets=linux --indent-switches
		--pad=oper --one-line=keep-blocks  --unpad=paren

Chat module plugin for Miranda IM

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

Option and settings handling for the group chat module. Implements
the option pages and various support functions.

$Id$

*/

#include "../commonheaders.h"
#include <shlobj.h>
#include <shlwapi.h>
#include "../m_MathModule.h"

extern HBRUSH 			hListBkgBrush;
extern HICON			hIcons[30];
extern FONTINFO			aFonts[OPTIONS_FONTCOUNT];
extern SESSION_INFO		g_TabSession;
extern int              g_chat_integration_enabled;
extern HANDLE           hMessageWindowList;
extern MYGLOBALS        myGlobals;
extern HMODULE          g_hIconDLL;

extern HIMAGELIST       CreateStateImageList();

HANDLE			g_hOptions = NULL;

#define FONTF_BOLD   1
#define FONTF_ITALIC 2
struct FontOptionsList {
	TCHAR*   szDescr;
	COLORREF defColour;
	TCHAR*   szDefFace;
	BYTE     defCharset, defStyle;
	char     defSize;
	COLORREF colour;
	TCHAR    szFace[LF_FACESIZE];
	BYTE     charset, style;
	char     size;
};

//remember to put these in the Translate( ) template file too
static struct FontOptionsList CHAT_fontOptionsList[] = {
	{LPGENT("Timestamp"), RGB(50, 50, 240), _T("Terminal"), DEFAULT_CHARSET, 0, -8}, 
	{LPGENT("Others nicknames"), RGB(0, 0, 0), _T("Verdana"), DEFAULT_CHARSET, FONTF_BOLD, -12}, 
	{LPGENT("Your nickname"), RGB(0, 0, 0), _T("Verdana"), DEFAULT_CHARSET, FONTF_BOLD, -12}, 
	{LPGENT("User has joined"), RGB(90, 160, 90), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("User has left"), RGB(160, 160, 90), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("User has disconnected"), RGB(160, 90, 90), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("User kicked ..."), RGB(100, 100, 100), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("User is now known as ..."), RGB(90, 90, 160), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("Notice from user"), RGB(160, 130, 60), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("Incoming message"), RGB(90, 90, 90), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("Outgoing message"), RGB(90, 90, 90), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("The topic is ..."), RGB(70, 70, 160), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("Information messages"), RGB(130, 130, 195), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("User enables status for ..."), RGB(70, 150, 70), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("User disables status for ..."), RGB(150, 70, 70), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("Action message"), RGB(160, 90, 160), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("Highlighted message"), RGB(180, 150, 80), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("Chat log symbols (Webdings)"), RGB(170, 170, 170), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("User list members (Online)"), RGB(0, 0, 0), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("User list members (away)"), RGB(170, 170, 170), _T("Verdana"), DEFAULT_CHARSET, 0, -12},
};

const int msgDlgFontCount = SIZEOF(CHAT_fontOptionsList);

static struct FontOptionsList IM_fontOptionsList[] = {
	{LPGENT(">> Outgoing messages"), RGB(50, 50, 240), _T("Terminal"), DEFAULT_CHARSET, 0, -8}, 
	{LPGENT(">> Outgoing misc events"), RGB(0, 0, 0), _T("Verdana"), DEFAULT_CHARSET, FONTF_BOLD, -12}, 
	{LPGENT("<< Incoming messages"), RGB(0, 0, 0), _T("Verdana"), DEFAULT_CHARSET, FONTF_BOLD, -12}, 
	{LPGENT("<< Incoming misc events"), RGB(90, 160, 90), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT(">> Outgoing name"), RGB(160, 160, 90), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT(">> Outgoing timestamp"), RGB(160, 90, 90), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("<< Incoming name"), RGB(100, 100, 100), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("<< Incoming timestamp"), RGB(90, 90, 160), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT(">> Outgoing messages (old)"), RGB(160, 130, 60), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT(">> Outgoing misc events (old)"), RGB(90, 90, 90), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("<< Incoming messages (old)"), RGB(90, 90, 90), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("<< Incoming misc events (old)"), RGB(70, 70, 160), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT(">> Outgoing name (old)"), RGB(130, 130, 195), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT(">> Outgoing time (old)"), RGB(70, 150, 70), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("<< Incoming name (old)"), RGB(150, 70, 70), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("<< Incoming time (old)"), RGB(160, 90, 160), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("* Message Input Area"), RGB(180, 150, 80), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("* Status changes"), RGB(0, 0, 40), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("* Dividers"), RGB(0, 0, 0), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("* Error and warning Messages"), RGB(170, 170, 170), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("* Symbols (incoming)"), RGB(170, 170, 170), _T("Verdana"), DEFAULT_CHARSET, 0, -12}, 
	{LPGENT("* Symbols (outgoing)"), RGB(170, 170, 170), _T("Verdana"), DEFAULT_CHARSET, 0, -12},
};

static struct FontOptionsList IP_fontOptionsList[] = {
	{LPGENT("Infopanel / Nickname"), RGB(0, 0, 0), _T("Verdana"), DEFAULT_CHARSET, 0, -10}, 
	{LPGENT("Infopanel / UIN"), RGB(0, 0, 0), _T("Verdana"), DEFAULT_CHARSET, FONTF_BOLD, -10}, 
	{LPGENT("Infopanel / Status"), RGB(0, 0, 0), _T("Verdana"), DEFAULT_CHARSET, FONTF_BOLD, -10}, 
	{LPGENT("Infopanel / Protocol"), RGB(0, 0, 0), _T("Verdana"), DEFAULT_CHARSET, 0, -10}, 
	{LPGENT("Infopanel / Contacts local time"), RGB(255, 0, 0), _T("Verdana"), DEFAULT_CHARSET, 0, -10}, 
	{LPGENT("Window caption (skinned mode)"), RGB(255, 255, 255), _T("Verdana"), DEFAULT_CHARSET, 0, -10},
};

static struct FontOptionsList *fontOptionsList = IM_fontOptionsList;
static int fontCount = MSGDLGFONTCOUNT;

struct branch_t {
	TCHAR*    szDescr;
	char*     szDBName;
	int       iMode;
	BYTE      bDefault;
	HTREEITEM hItem;
};
static struct branch_t branch1[] = {
	{LPGENT("Open new chat rooms in the default container"), "DefaultContainer", 0, 0, NULL}, 
	{LPGENT("Flash window when someone speaks"), "FlashWindow", 0, 0, NULL}, 
	{LPGENT("Flash window when a word is highlighted"), "FlashWindowHighlight", 0, 1, NULL}, 
	{LPGENT("Show list of users in the chat room"), "ShowNicklist", 0, 1, NULL}, 
	{LPGENT("Show button menus when right clicking the buttons"), "RightClickFilter", 0, 0, NULL}, 
	{LPGENT("Show the topic of the room on your contact list (if supported)"), "TopicOnClist", 0, 0, NULL}, 
	{LPGENT("Do not play sounds when the chat room is focused"), "SoundsFocus", 0, 0, NULL}, 
	{LPGENT("Do not pop up the window when joining a chat room"), "PopupOnJoin", 0, 0, NULL}, 
	{LPGENT("Toggle the visible state when double clicking in the contact list"), "ToggleVisibility", 0, 0, NULL}, 
	{LPGENT("Sync splitter position with standard IM sessions"), "SyncSplitter", 0, 0, NULL}, 
	{LPGENT("Show contact statuses if protocol supports them"), "ShowContactStatus", 0, 1, NULL}, 
	{LPGENT("Display contact status icon before user role icon"), "ContactStatusFirst", 0, 0, NULL},
};
static struct branch_t branch2[] = {
	{LPGENT("Prefix all events with a timestamp"), "ShowTimeStamp", 0, 1, NULL}, 
	{LPGENT("Only prefix with timestamp if it has changed"), "ShowTimeStampIfChanged", 0, 0, NULL}, 
	{LPGENT("Timestamp has same colour as the event"), "TimeStampEventColour", 0, 0, NULL}, 
	{LPGENT("Indent the second line of a message"), "LogIndentEnabled", 0, 1, NULL}, 
	{LPGENT("Limit user names in the message log to 20 characters"), "LogLimitNames", 0, 1, NULL}, 
	{LPGENT("Add \':\' to auto-completed user names"), "AddColonToAutoComplete", 0, 1, NULL}, 
	{LPGENT("Start private conversation on doubleclick in nick list (insert nick if unchecked)"), "DoubleClick4Privat", 0, 1, NULL}, 
	{LPGENT("Strip colors from messages in the log"), "StripFormatting", 0, 0, NULL}, 
	{LPGENT("Enable the \'event filter\' for new rooms"), "FilterEnabled", 0, 0, NULL}, 
	{LPGENT("Use IRC style status indicators in the nicklist (@, %, + etc.)"), "ClassicIndicators", 0, 1, NULL}, 
	{LPGENT("Use text symbols instead of icons in the chat log (faster)"), "LogSymbols", 0, 0, NULL}, 
	{LPGENT("Make nicknames clickable hyperlinks"), "ClickableNicks", 0, 0, NULL}, 
	{LPGENT("Colorize nicknames (not when using clickable nicknames)"), "ColorizeNicks", 0, 0, NULL}, 
	{LPGENT("Scale down icons to 10x10 pixels in the chat log"), "ScaleIcons", 0, 1, NULL}, 
	{LPGENT("Draw dividers to mark inactivity"), "UseDividers", 0, 1, NULL}, 
	{LPGENT("Use the containers popup configuration to place dividers"), "DividersUsePopupConfig", 0, 1, NULL}, 
	{LPGENT("Support math module plugin"), "MathModSupport", 0, 0, NULL}
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
	{LPGENT("Show icon for topic changes"), "IconFlags", GC_EVENT_TOPIC, 0, NULL}, 
	{LPGENT("Show icon for users joining"), "IconFlags", GC_EVENT_JOIN, 1, NULL}, 
	{LPGENT("Show icon for users disconnecting"), "IconFlags", GC_EVENT_QUIT, 0, NULL}, 
	{LPGENT("Show icon for messages"), "IconFlags", GC_EVENT_MESSAGE, 0, NULL}, 
	{LPGENT("Show icon for actions"), "IconFlags", GC_EVENT_ACTION, 0, NULL}, 
	{LPGENT("Show icon for highlights"), "IconFlags", GC_EVENT_HIGHLIGHT, 0, NULL}, 
	{LPGENT("Show icon for users leaving"), "IconFlags", GC_EVENT_PART, 0, NULL}, 
	{LPGENT("Show icon for users kicking other user"), "IconFlags", GC_EVENT_KICK, 0, NULL}, 
	{LPGENT("Show icon for notices "), "IconFlags", GC_EVENT_NOTICE, 0, NULL}, 
	{LPGENT("Show icon for name changes"), "IconFlags", GC_EVENT_NICK, 0, NULL}, 
	{LPGENT("Show icon for information messages"), "IconFlags", GC_EVENT_INFORMATION, 0, NULL}, 
	{LPGENT("Show icon for status changes"), "IconFlags", GC_EVENT_ADDSTATUS, 0, NULL},
};

static struct branch_t branch5[] = {
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
	{LPGENT("Skip all popups when no channel window is opened"), "SkipWhenNoWindow", 0, 0, NULL}, 
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

void LoadMsgDlgFont(int i, LOGFONT *lf, COLORREF* colour, char *szMod)
{
	char str[32];
	int style;
	DBVARIANT dbv;
	int j = (i >= 100 ? i - 100 : i);

	if (colour) {
		wsprintfA(str, "Font%dCol", i);
		*colour = DBGetContactSettingDword(NULL, szMod, str, fontOptionsList[j].defColour);
	}
	if (lf) {
		wsprintfA(str, "Font%dSize", i);
		lf->lfHeight = (char) DBGetContactSettingByte(NULL, szMod, str, fontOptionsList[j].defSize);
		lf->lfWidth = 0;
		lf->lfEscapement = 0;
		lf->lfOrientation = 0;
		wsprintfA(str, "Font%dSty", i);
		style = DBGetContactSettingByte(NULL, szMod, str, fontOptionsList[j].defStyle);
		lf->lfWeight = style & FONTF_BOLD ? FW_BOLD : FW_NORMAL;
		lf->lfItalic = style & FONTF_ITALIC ? 1 : 0;
		lf->lfUnderline = 0;
		lf->lfStrikeOut = 0;
		wsprintfA(str, "Font%dSet", i);
		lf->lfCharSet = DBGetContactSettingByte(NULL, szMod, str, fontOptionsList[j].defCharset);
		lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
		lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lf->lfQuality = DEFAULT_QUALITY;
		lf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		wsprintfA(str, "Font%d", i);
		if (i == 17 && !strcmp(szMod, CHAT_FONTMODULE)) {
			lf->lfCharSet = SYMBOL_CHARSET;
			lstrcpyn(lf->lfFaceName, _T("Webdings"), SIZEOF(lf->lfFaceName));
		} else {
			if (DBGetContactSettingTString(NULL, szMod, str, &dbv)) {
				lstrcpy(lf->lfFaceName, fontOptionsList[j].szDefFace);
			} else {
				lstrcpyn(lf->lfFaceName, dbv.ptszVal, SIZEOF(lf->lfFaceName));
				DBFreeVariant(&dbv);
			}
		}
	}
}

static HTREEITEM InsertBranch(HWND hwndTree, TCHAR* pszDescr, BOOL bExpanded)
{
	TVINSERTSTRUCT tvis;

	tvis.hParent = NULL;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT | TVIF_STATE;
	tvis.item.pszText = TranslateTS(pszDescr);
	tvis.item.stateMask = (bExpanded ? TVIS_STATEIMAGEMASK | TVIS_EXPANDED : TVIS_STATEIMAGEMASK) | TVIS_BOLD;
	tvis.item.state = (bExpanded ? INDEXTOSTATEIMAGEMASK(1) | TVIS_EXPANDED : INDEXTOSTATEIMAGEMASK(1)) | TVIS_BOLD;
	return TreeView_InsertItem(hwndTree, &tvis);
}

static void FillBranch(HWND hwndTree, HTREEITEM hParent, struct branch_t *branch, int nValues, DWORD defaultval)
{
	TVINSERTSTRUCT tvis;
	int i;
	int iState;

	if (hParent == 0)
		return;

	tvis.hParent = hParent;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT | TVIF_STATE;
	for (i = 0;i < nValues;i++) {
		tvis.item.pszText = TranslateTS(branch[i].szDescr);
		tvis.item.stateMask = TVIS_STATEIMAGEMASK;
		if (branch[i].iMode)
			iState = ((DBGetContactSettingDword(NULL, "Chat", branch[i].szDBName, defaultval) & branch[i].iMode) & branch[i].iMode) != 0 ? 2 : 1;
		else
			iState = DBGetContactSettingByte(NULL, "Chat", branch[i].szDBName, branch[i].bDefault) != 0 ? 2 : 1;
		tvis.item.state = INDEXTOSTATEIMAGEMASK(iState);
		branch[i].hItem = TreeView_InsertItem(hwndTree, &tvis);
	}
}

static void SaveBranch(HWND hwndTree, struct branch_t *branch, int nValues)
{
	TVITEM tvi;
	BYTE bChecked;
	int i;
	int iState = 0;

	tvi.mask = TVIF_HANDLE | TVIF_STATE;
	for (i = 0;i < nValues;i++) {
		tvi.hItem = branch[i].hItem;
		TreeView_GetItem(hwndTree, &tvi);
		bChecked = ((tvi.state & TVIS_STATEIMAGEMASK) >> 12 == 1) ? 0 : 1;
		if (branch[i].iMode) {
			if (bChecked)
				iState |= branch[i].iMode;
			if (iState & GC_EVENT_ADDSTATUS)
				iState |= GC_EVENT_REMOVESTATUS;
			DBWriteContactSettingDword(NULL, "Chat", branch[i].szDBName, (DWORD)iState);
		} else DBWriteContactSettingByte(NULL, "Chat", branch[i].szDBName, bChecked);
	}
}

static void CheckHeading(HWND hwndTree, HTREEITEM hHeading)
{
	BOOL bChecked = TRUE;
	TVITEM tvi;

	if (hHeading == 0)
		return;

	tvi.mask = TVIF_HANDLE | TVIF_STATE;
	tvi.hItem = TreeView_GetNextItem(hwndTree, hHeading, TVGN_CHILD);
	while (tvi.hItem && bChecked) {
		if (tvi.hItem != branch1[0].hItem && tvi.hItem != branch1[1].hItem) {
			TreeView_GetItem(hwndTree, &tvi);
			if (((tvi.state&TVIS_STATEIMAGEMASK) >> 12 == 1))
				bChecked = FALSE;
		}
		tvi.hItem = TreeView_GetNextSibling(hwndTree, tvi.hItem);
	}
	tvi.stateMask = TVIS_STATEIMAGEMASK;
	tvi.state = INDEXTOSTATEIMAGEMASK(bChecked ? 2 : 1);
	tvi.hItem = hHeading;
	TreeView_SetItem(hwndTree, &tvi);
}

static void CheckBranches(HWND hwndTree, HTREEITEM hHeading)
{
	BOOL bChecked = TRUE;
	TVITEM tvi;

	if (hHeading == 0)
		return;

	tvi.mask = TVIF_HANDLE | TVIF_STATE;
	tvi.hItem = hHeading;
	TreeView_GetItem(hwndTree, &tvi);
	if (((tvi.state&TVIS_STATEIMAGEMASK) >> 12 == 1)) //|| ((tvi.state & TVIS_STATEIMAGEMASK) >> 12 == 1))
		bChecked = FALSE;

	tvi.stateMask = TVIS_STATEIMAGEMASK;
	tvi.state = INDEXTOSTATEIMAGEMASK(bChecked ? 2 : 1);
	TreeView_SetItem(hwndTree, &tvi);
	tvi.hItem = TreeView_GetNextItem(hwndTree, hHeading, TVGN_CHILD);
	while (tvi.hItem) {
		tvi.state = INDEXTOSTATEIMAGEMASK(bChecked ? 2 : 1);
		if (tvi.hItem != branch1[0].hItem && tvi.hItem != branch1[1].hItem)
			TreeView_SetItem(hwndTree, &tvi);
		tvi.hItem = TreeView_GetNextSibling(hwndTree, tvi.hItem);
	}
}

static INT CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData)
{
	char szDir[MAX_PATH];
	switch (uMsg) {
		case BFFM_INITIALIZED:
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
			break;
		case BFFM_SELCHANGED:
			if (SHGetPathFromIDListA((LPITEMIDLIST) lp , szDir))
				SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)szDir);
			break;
	}
	return 0;
}

static void LoadLogFonts(void)
{
	int i;

	for (i = 0; i < OPTIONS_FONTCOUNT; i++)
		LoadMsgDlgFont(i, &aFonts[i].lf, &aFonts[i].color, CHAT_FONTMODULE);
}

static struct _tagicons { char *szDesc; char *szName; int id; UINT size;} _icons[] = {
	LPGEN("Window Icon"), "chat_window", IDI_CHANMGR, 16,
	LPGEN("Background colour"), "chat_bkgcol", IDI_BKGCOLOR, 16,
	LPGEN("Room settings"), "chat_settings", IDI_TOPICBUT, 16,
	LPGEN("Event filter disabled"), "chat_filter", IDI_FILTER, 16,
	LPGEN("Event filter enabled"), "chat_filter2", IDI_FILTER2, 16,
	LPGEN("Icon overlay"), "chat_overlay", IDI_OVERLAY, 16,
	LPGEN("Show nicklist"), "chat_shownicklist", IDI_SHOWNICKLIST, 16,
	LPGEN("Hide nicklist"), "chat_hidenicklist", IDI_HIDENICKLIST, 16,

	LPGEN("Status 1 (10x10)"), "chat_status0", IDI_STATUS0, 16,
	LPGEN("Status 2 (10x10)"), "chat_status1", IDI_STATUS1, 16,
	LPGEN("Status 3 (10x10)"), "chat_status2", IDI_STATUS2, 16,
	LPGEN("Status 4 (10x10)"), "chat_status3", IDI_STATUS3, 16,
	LPGEN("Status 5 (10x10)"), "chat_status4", IDI_STATUS4, 16,
	LPGEN("Status 6 (10x10)"), "chat_status5", IDI_STATUS5, 16,
	NULL, NULL, -1, 0
};

static struct _tag1icons { char *szDesc; char *szName; int id; UINT size;} _logicons[] = {
	LPGEN("Message in (10x10)"), "chat_log_message_in", IDI_MESSAGE, 16,
	LPGEN("Message out (10x10)"), "chat_log_message_out", IDI_MESSAGEOUT, 16,
	LPGEN("Action (10x10)"), "chat_log_action", IDI_ACTION, 16,
	LPGEN("Add Status (10x10)"), "chat_log_addstatus", IDI_ADDSTATUS, 16,
	LPGEN("Remove Status (10x10)"), "chat_log_removestatus", IDI_REMSTATUS, 16,
	LPGEN("Join (10x10)"), "chat_log_join", IDI_JOIN, 16,
	LPGEN("Leave (10x10)"), "chat_log_part", IDI_PART, 16,
	LPGEN("Quit (10x10)"), "chat_log_quit", IDI_QUIT, 16,
	LPGEN("Kick (10x10)"), "chat_log_kick", IDI_KICK, 16,
	LPGEN("Notice (10x10)"), "chat_log_notice", IDI_NOTICE, 16,
	LPGEN("Nickchange (10x10)"), "chat_log_nick", IDI_NICK, 16,
	LPGEN("Topic (10x10)"), "chat_log_topic", IDI_TOPIC, 16,
	LPGEN("Highlight (10x10)"), "chat_log_highlight", IDI_HIGHLIGHT, 16,
	LPGEN("Information (10x10)"), "chat_log_info", IDI_INFO, 16,
	NULL, NULL, 0, 0
};

// add icons to the skinning module
void Chat_AddIcons(void)
{
	if (ServiceExists(MS_SKIN2_ADDICON)) {
		SKINICONDESC sid = {0};
		char szFile[MAX_PATH];
		int i = 0;

		// 16x16 icons
		sid.cbSize = sizeof(SKINICONDESC);
		sid.pszSection = Translate("TabSRMM/Group chat windows");
		GetModuleFileNameA(g_hIconDLL, szFile, MAX_PATH);
		sid.pszDefaultFile = szFile;

		while (_icons[i].szDesc != NULL) {
			sid.cx = sid.cy = _icons[i].size;
			sid.pszDescription = _icons[i].szDesc;
			sid.pszName = _icons[i].szName;
			sid.iDefaultIndex = -_icons[i].id;
			CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
			i++;
		}
		i = 0;
		sid.pszSection = Translate("TabSRMM/Group chat log");
		while (_logicons[i].szDesc != NULL) {
			sid.cx = sid.cy = _logicons[i].size;
			sid.pszDescription = _logicons[i].szDesc;
			sid.pszName = _logicons[i].szName;
			sid.iDefaultIndex = -_logicons[i].id;
			CallService(MS_SKIN2_ADDICON, 0, (LPARAM)&sid);
			i++;
		}
	}
	return;
}

/*
 * get icon by name from the core icon library service
 */

HICON LoadIconEx(int iIndex, char * pszIcoLibName, int iX, int iY)
{
	char szTemp[256];
	mir_snprintf(szTemp, sizeof(szTemp), "chat_%s", pszIcoLibName);
	return (HICON) CallService(MS_SKIN2_GETICON, 0, (LPARAM)szTemp);
}

static void InitSetting(TCHAR** ppPointer, char* pszSetting, TCHAR* pszDefault)
{
	DBVARIANT dbv;
	if (!DBGetContactSettingTString(NULL, "Chat", pszSetting, &dbv)) {
		replaceStr(ppPointer, dbv.ptszVal);
		DBFreeVariant(&dbv);
	} else
		replaceStr(ppPointer, pszDefault);
}

#define OPT_FIXHEADINGS (WM_USER+1)

static UINT _o1controls[] = {IDC_CHECKBOXES, IDC_GROUP, IDC_STATIC_ADD,
							 IDC_STATIC_OTHER, IDC_LOGGING, IDC_LOGDIRECTORY, IDC_FONTCHOOSE, IDC_CHAT_SPIN3, IDC_LIMIT, IDC_STATIC110, IDC_STATIC112, 0
							};

BOOL CALLBACK DlgProcOptions1(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HTREEITEM hListHeading1 = 0;
	static HTREEITEM hListHeading2 = 0;
	static HTREEITEM hListHeading3 = 0;
	static HTREEITEM hListHeading4 = 0;
	static HTREEITEM hListHeading5 = 0;
	static HTREEITEM hListHeading6 = 0;
	switch (uMsg) {
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			if (g_chat_integration_enabled) {
				char szTemp[MAX_PATH];
				/* HIMAGELIST himlOptions; */

				SetWindowLong(GetDlgItem(hwndDlg, IDC_CHECKBOXES), GWL_STYLE, GetWindowLong(GetDlgItem(hwndDlg, IDC_CHECKBOXES), GWL_STYLE) | TVS_NOHSCROLL | TVS_CHECKBOXES);
				/*
				himlOptions = (HIMAGELIST)SendDlgItemMessage(hwndDlg, IDC_CHECKBOXES, TVM_SETIMAGELIST, TVSIL_STATE, (LPARAM)CreateStateImageList());
				if (himlOptions)
					ImageList_Destroy(himlOptions);
				*/
				hListHeading1 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), TranslateT("Appearance and functionality of chat room windows"), DBGetContactSettingByte(NULL, "Chat", "Branch1Exp", 0) ? TRUE : FALSE);
				hListHeading2 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), TranslateT("Appearance of the message log"), DBGetContactSettingByte(NULL, "Chat", "Branch2Exp", 0) ? TRUE : FALSE);
				hListHeading3 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), TranslateT("Default events to show in new chat rooms if the \'event filter\' is enabled"), DBGetContactSettingByte(NULL, "Chat", "Branch3Exp", 0) ? TRUE : FALSE);
				hListHeading4 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), TranslateT("Icons to display in the message log"), DBGetContactSettingByte(NULL, "Chat", "Branch4Exp", 0) ? TRUE : FALSE);
				hListHeading5 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), TranslateT("Icons to display in the tray and the message window tabs / title"), DBGetContactSettingByte(NULL, "Chat", "Branch5Exp", 0) ? TRUE : FALSE);
				if (myGlobals.g_PopupAvail)
					hListHeading6 = InsertBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), TranslateT("Pop-ups to display"), DBGetContactSettingByte(NULL, "Chat", "Branch6Exp", 0) ? TRUE : FALSE);
				FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading1, branch1, SIZEOF(branch1), 0);
				FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading2, branch2, SIZEOF(branch2), 0);
				FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading3, branch3, SIZEOF(branch3), 0x03E0);
				FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading4, branch4, SIZEOF(branch4), 0x0000);
				FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading5, branch5, SIZEOF(branch5), 0x1000);
				FillBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading6, branch6, SIZEOF(branch6), 0x0000);
				SendMessage(hwndDlg, OPT_FIXHEADINGS, 0, 0);
				{
					TCHAR* pszGroup = NULL;
					InitSetting(&pszGroup, "AddToGroup", _T("Chat rooms"));
					SetWindowText(GetDlgItem(hwndDlg, IDC_GROUP), pszGroup);
					mir_free(pszGroup);
					ShowWindow(GetDlgItem(hwndDlg, IDC_STATIC_MESSAGE), SW_HIDE);
				}
				SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN3, UDM_SETRANGE, 0, MAKELONG(10000, 0));
				SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN3, UDM_SETPOS, 0, MAKELONG(DBGetContactSettingWord(NULL, "Chat", "LoggingLimit", 100), 0));
				CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)g_Settings.pszLogDir, (LPARAM)szTemp);
				SetDlgItemTextA(hwndDlg, IDC_LOGDIRECTORY, szTemp);
				CheckDlgButton(hwndDlg, IDC_LOGGING, g_Settings.LoggingEnabled);
				EnableWindow(GetDlgItem(hwndDlg, IDC_LOGDIRECTORY), g_Settings.LoggingEnabled ? TRUE : FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_FONTCHOOSE), g_Settings.LoggingEnabled ? TRUE : FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT), g_Settings.LoggingEnabled ? TRUE : FALSE);
			} else {
				int i = 0;

				while (_o1controls[i])
					ShowWindow(GetDlgItem(hwndDlg, _o1controls[i++]), SW_HIDE);
			}
			CheckDlgButton(hwndDlg, IDC_CHAT_ENABLE, DBGetContactSettingByte(NULL, SRMSGMOD_T, "enable_chat", 0));
			break;

		case OPT_FIXHEADINGS:
			CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading1);
			CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading2);
			CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading3);
			CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading4);
			CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading5);
			CheckHeading(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading6);
			break;
		case WM_COMMAND:
			if (LOWORD(wParam) == IDC_CHAT_ENABLE) {
				DBWriteContactSettingByte(NULL, "PluginDisable", "chat.dll", (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_CHAT_ENABLE) ? 1 : 0));
				MessageBox(0, TranslateT("You should now immediatly restart Miranda to make this change take effect."), TranslateT("tabSRMM Message"), MB_OK);
			}
			if ((LOWORD(wParam) == IDC_GROUP || LOWORD(wParam) == IDC_LIMIT || LOWORD(wParam) == IDC_LOGDIRECTORY)
					&& (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()))	return 0;

			switch (LOWORD(wParam)) {
				case IDC_LOGGING:
					if (g_chat_integration_enabled) {
						EnableWindow(GetDlgItem(hwndDlg, IDC_LOGDIRECTORY), IsDlgButtonChecked(hwndDlg, IDC_LOGGING) == BST_CHECKED ? TRUE : FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_FONTCHOOSE), IsDlgButtonChecked(hwndDlg, IDC_LOGGING) == BST_CHECKED ? TRUE : FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT), IsDlgButtonChecked(hwndDlg, IDC_LOGGING) == BST_CHECKED ? TRUE : FALSE);
					}
					break;
				case  IDC_FONTCHOOSE: {
					char szDirectory[MAX_PATH];
					LPITEMIDLIST idList;
					LPMALLOC psMalloc;
					BROWSEINFOA bi = {0};

					if (SUCCEEDED(CoGetMalloc(1, &psMalloc))) {
						char szTemp[MAX_PATH];
						bi.hwndOwner = hwndDlg;
						bi.pszDisplayName = szDirectory;
						bi.lpszTitle = Translate("Select Folder");
						bi.ulFlags = BIF_NEWDIALOGSTYLE | BIF_EDITBOX | BIF_RETURNONLYFSDIRS;
						bi.lpfn = BrowseCallbackProc;
						bi.lParam = (LPARAM)szDirectory;

						idList = SHBrowseForFolderA(&bi);
						if (idList) {
							SHGetPathFromIDListA(idList, szDirectory);
							lstrcatA(szDirectory, "\\");
							CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)szDirectory, (LPARAM)szTemp);
							SetWindowTextA(GetDlgItem(hwndDlg, IDC_LOGDIRECTORY), lstrlenA(szTemp) > 1 ? szTemp : "Logs\\");
						}
						psMalloc->lpVtbl->Free(psMalloc, idList);
						psMalloc->lpVtbl->Release(psMalloc);
					}
					break;
				}
			}

			if (lParam != (LPARAM)NULL)
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;

		case WM_NOTIFY: {
			switch (((LPNMHDR)lParam)->idFrom) {
				case IDC_CHECKBOXES:
					if (((LPNMHDR)lParam)->code == NM_CLICK || (((LPNMHDR)lParam)->code == TVN_KEYDOWN && ((LPNMTVKEYDOWN)lParam)->wVKey == VK_SPACE)) {
						TVHITTESTINFO hti;
						hti.pt.x = (short)LOWORD(GetMessagePos());
						hti.pt.y = (short)HIWORD(GetMessagePos());
						ScreenToClient(((LPNMHDR)lParam)->hwndFrom, &hti.pt);
						if (TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom, &hti) || ((LPNMHDR)lParam)->code == TVN_KEYDOWN) {
							if (((LPNMHDR)lParam)->code == TVN_KEYDOWN)
								hti.flags |= TVHT_ONITEMSTATEICON;
							if (hti.flags&TVHT_ONITEMSTATEICON) {
								TVITEM tvi = {0};

								tvi.mask = TVIF_HANDLE | TVIF_STATE;
								tvi.stateMask = TVIS_STATEIMAGEMASK | TVIS_BOLD;

								if (((LPNMHDR)lParam)->code == TVN_KEYDOWN)
									tvi.hItem = TreeView_GetSelection(((LPNMHDR)lParam)->hwndFrom);
								else
									tvi.hItem = (HTREEITEM)hti.hItem;

								TreeView_GetItem(((LPNMHDR)lParam)->hwndFrom, &tvi);

								if (tvi.hItem == hListHeading1)
									CheckBranches(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading1);
								else if (tvi.hItem == hListHeading2)
									CheckBranches(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading2);
								else if (tvi.hItem == hListHeading3)
									CheckBranches(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading3);
								else if (tvi.hItem == hListHeading4)
									CheckBranches(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading4);
								else if (tvi.hItem == hListHeading5)
									CheckBranches(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading5);
								else if (tvi.hItem == hListHeading6)
									CheckBranches(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading6);
								else {

									if (tvi.state & TVIS_BOLD && hti.flags & TVHT_ONITEMSTATEICON) {
										tvi.state = INDEXTOSTATEIMAGEMASK(0) | TVIS_BOLD;
										SendDlgItemMessageA(hwndDlg, IDC_CHECKBOXES, TVM_SETITEMA, 0, (LPARAM)&tvi);
									} else if (hti.flags&TVHT_ONITEMSTATEICON) {
										if (((tvi.state & TVIS_STATEIMAGEMASK) >> 12) == 3) {
											tvi.state = INDEXTOSTATEIMAGEMASK(1);
											SendDlgItemMessageA(hwndDlg, IDC_CHECKBOXES, TVM_SETITEMA, 0, (LPARAM)&tvi);
										}
									}
									PostMessage(hwndDlg, OPT_FIXHEADINGS, 0, 0);
								}
								SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
							}
						}
					}

					break;

				case 0:
					switch (((LPNMHDR)lParam)->code) {
						case PSN_APPLY: {
							if (g_chat_integration_enabled) {
								int iLen;
								TCHAR *pszText = NULL;
								BYTE b;

								iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_LOGDIRECTORY));
								if (iLen > 0) {
									char *pszText1 = malloc(iLen + 2);
									GetDlgItemTextA(hwndDlg, IDC_LOGDIRECTORY, pszText1, iLen + 1);
									DBWriteContactSettingString(NULL, "Chat", "LogDirectory", pszText1);
									CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)pszText, (LPARAM)g_Settings.pszLogDir);
								} else
									DBDeleteContactSetting(NULL, "Chat", "LogDirectory");

								g_Settings.LoggingEnabled = IsDlgButtonChecked(hwndDlg, IDC_LOGGING) == BST_CHECKED ? TRUE : FALSE;
								DBWriteContactSettingByte(NULL, "Chat", "LoggingEnabled", (BYTE)g_Settings.LoggingEnabled);
								if (g_Settings.LoggingEnabled)
									if (!PathIsDirectoryA(g_Settings.pszLogDir))
										CreateDirectoryA(g_Settings.pszLogDir, NULL);

								iLen = SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN3, UDM_GETPOS, 0, 0);
								DBWriteContactSettingWord(NULL, "Chat", "LoggingLimit", (WORD)iLen);

								iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_GROUP));
								if (iLen > 0) {
									pszText = realloc(pszText, (iLen + 2) * sizeof(TCHAR));
									GetDlgItemText(hwndDlg, IDC_GROUP, pszText, iLen + 1);
									DBWriteContactSettingTString(NULL, "Chat", "AddToGroup", pszText);
								} else
									DBWriteContactSettingTString(NULL, "Chat", "AddToGroup", _T(""));

								if (pszText)
									free(pszText);

								b = DBGetContactSettingByte(NULL, "Chat", "Tabs", 1);
								SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch1, sizeof(branch1) / sizeof(branch1[0]));
								SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch2, sizeof(branch2) / sizeof(branch2[0]));
								SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch3, sizeof(branch3) / sizeof(branch3[0]));
								SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch4, sizeof(branch4) / sizeof(branch4[0]));
								SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch5, sizeof(branch5) / sizeof(branch5[0]));
								if (myGlobals.g_PopupAvail)
									SaveBranch(GetDlgItem(hwndDlg, IDC_CHECKBOXES), branch6, sizeof(branch6) / sizeof(branch6[0]));
								LoadGlobalSettings();
								MM_FontsChanged();
								FreeMsgLogBitmaps();
								LoadMsgLogBitmaps();
								SM_BroadcastMessage(NULL, GC_SETWNDPROPS, 0, 0, TRUE);
							}
							DBWriteContactSettingByte(NULL, SRMSGMOD_T, "enable_chat", (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_CHAT_ENABLE) ? 1 : 0));
						}
						return TRUE;
					}
			}
		}
		break;
		case WM_DESTROY: {
			BYTE b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading1, TVIS_EXPANDED) & TVIS_EXPANDED ? 1 : 0;
			DBWriteContactSettingByte(NULL, "Chat", "Branch1Exp", b);
			b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading2, TVIS_EXPANDED) & TVIS_EXPANDED ? 1 : 0;
			DBWriteContactSettingByte(NULL, "Chat", "Branch2Exp", b);
			b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading3, TVIS_EXPANDED) & TVIS_EXPANDED ? 1 : 0;
			DBWriteContactSettingByte(NULL, "Chat", "Branch3Exp", b);
			b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading4, TVIS_EXPANDED) & TVIS_EXPANDED ? 1 : 0;
			DBWriteContactSettingByte(NULL, "Chat", "Branch4Exp", b);
			b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading5, TVIS_EXPANDED) & TVIS_EXPANDED ? 1 : 0;
			DBWriteContactSettingByte(NULL, "Chat", "Branch5Exp", b);
			if (myGlobals.g_PopupAvail) {
				b = TreeView_GetItemState(GetDlgItem(hwndDlg, IDC_CHECKBOXES), hListHeading6, TVIS_EXPANDED) & TVIS_EXPANDED ? 1 : 0;
				DBWriteContactSettingByte(NULL, "Chat", "Branch6Exp", b);
			}
		}
		break;

		default:
			break;
	}
	return FALSE;
}

static void LoadFontsToList(HWND hwndDlg)
{
	int i;
	LOGFONT lf;
	char *szModule = fontOptionsList == CHAT_fontOptionsList ? CHAT_FONTMODULE : FONTMODULE;

	for (i = 0; i < fontCount; i++) {
		LoadMsgDlgFont(i + (fontOptionsList == IP_fontOptionsList ? 100 : 0) , &lf, &fontOptionsList[i].colour, szModule);
		lstrcpyn(fontOptionsList[i].szFace, lf.lfFaceName, LF_FACESIZE);
		fontOptionsList[i].szFace[LF_FACESIZE - 1] = 0;
		fontOptionsList[i].size = (char) lf.lfHeight;
		fontOptionsList[i].style = (lf.lfWeight >= FW_BOLD ? FONTF_BOLD : 0) | (lf.lfItalic ? FONTF_ITALIC : 0);
		fontOptionsList[i].charset = lf.lfCharSet;
		SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTLIST, LB_ADDSTRING, 0, i + 1);
	}
	SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTLIST, LB_SETSEL, TRUE, 0);
	SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTCOLOR, CPM_SETCOLOUR, 0, fontOptionsList[0].colour);
	SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTCOLOR, CPM_SETDEFAULTCOLOUR, 0, fontOptionsList[0].defColour);
}

static UINT _o2chatcontrols[] = { IDC_CHAT_SPIN2, IDC_LOGTIMESTAMP, IDC_TIMESTAMP,
								  IDC_OUTSTAMP, IDC_INSTAMP, IDC_HIGHLIGHT, IDC_HIGHLIGHTWORDS, IDC_CHAT_SPIN2, IDC_CHAT_SPIN3, IDC_NICKROW2, IDC_LOGLIMIT,
								  IDC_NICKCOLOR, IDC_NICKCOLORS, 0
								};

BOOL CALLBACK DlgProcOptions2(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HBRUSH hBkgColourBrush;
	static HBRUSH hListColourBrush;
	static HBRUSH hInfoPanelBrush;
	static HBRUSH hMessageColourBrush;
	switch (uMsg) {
		case WM_INITDIALOG: {
			COLORREF defwndcol;

			TranslateDialogDefault(hwndDlg);

			g_Settings.crLogBackground = (BOOL)DBGetContactSettingDword(NULL, "Chat", "ColorLogBG", GetSysColor(COLOR_WINDOW));
			if (g_chat_integration_enabled) {
				SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN2, UDM_SETRANGE, 0, MAKELONG(5000, 0));
				SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN2, UDM_SETPOS, 0, MAKELONG(DBGetContactSettingWord(NULL, "Chat", "LogLimit", 100), 0));
				SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN3, UDM_SETRANGE, 0, MAKELONG(255, 10));
				SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN3, UDM_SETPOS, 0, MAKELONG(DBGetContactSettingByte(NULL, "Chat", "NicklistRowDist", 12), 0));
				SetDlgItemText(hwndDlg, IDC_HIGHLIGHTWORDS, g_Settings.pszHighlightWords);
				SetDlgItemText(hwndDlg, IDC_LOGTIMESTAMP, g_Settings.pszTimeStampLog);
				SetDlgItemText(hwndDlg, IDC_TIMESTAMP, g_Settings.pszTimeStamp);
				SetDlgItemText(hwndDlg, IDC_OUTSTAMP, g_Settings.pszOutgoingNick);
				SetDlgItemText(hwndDlg, IDC_INSTAMP, g_Settings.pszIncomingNick);
				CheckDlgButton(hwndDlg, IDC_HIGHLIGHT, g_Settings.HighlightEnabled);
				EnableWindow(GetDlgItem(hwndDlg, IDC_HIGHLIGHTWORDS), g_Settings.HighlightEnabled ? TRUE : FALSE);
			} else {
				int i = 0;

				while (_o2chatcontrols[i])
					EnableWindow(GetDlgItem(hwndDlg, _o2chatcontrols[i++]), FALSE);
			}

			SendDlgItemMessage(hwndDlg, IDC_LOGBKG, CPM_SETCOLOUR, 0, g_Settings.crLogBackground);
			SendDlgItemMessage(hwndDlg, IDC_MESSAGEBKG, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, FONTMODULE, "inputbg", GetSysColor(COLOR_WINDOW)));
			SendDlgItemMessage(hwndDlg, IDC_NICKLISTBKG, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, "Chat", "ColorNicklistBG", GetSysColor(COLOR_WINDOW)));
			SendDlgItemMessage(hwndDlg, IDC_LOGBKG, CPM_SETDEFAULTCOLOUR, 0, g_Settings.crLogBackground);
			SendDlgItemMessage(hwndDlg, IDC_MESSAGEBKG, CPM_SETDEFAULTCOLOUR, 0, DBGetContactSettingDword(NULL, "Chat", "ColorNicklistBG", GetSysColor(COLOR_WINDOW)));
			SendDlgItemMessage(hwndDlg, IDC_NICKLISTBKG, CPM_SETDEFAULTCOLOUR, 0, DBGetContactSettingDword(NULL, "Chat", "ColorNicklistBG", GetSysColor(COLOR_WINDOW)));

			SendDlgItemMessage(hwndDlg, IDC_CHAT_CHOOSEFONT, BM_SETIMAGE, IMAGE_ICON, (LPARAM)myGlobals.g_buttonBarIcons[21]);
			SendDlgItemMessage(hwndDlg, IDC_CHAT_CHOOSEFONT, BUTTONADDTOOLTIP, (WPARAM)Translate("Select font"), 0);

			hBkgColourBrush = CreateSolidBrush(SendDlgItemMessage(hwndDlg, IDC_LOGBKG, CPM_GETCOLOUR, 0, 0));
			hListColourBrush = CreateSolidBrush(SendDlgItemMessage(hwndDlg, IDC_NICKLISTBKG, CPM_GETCOLOUR, 0, 0));
			hMessageColourBrush = CreateSolidBrush(SendDlgItemMessage(hwndDlg, IDC_MESSAGEBKG, CPM_GETCOLOUR, 0, 0));

			defwndcol = GetSysColor(COLOR_WINDOW);

			SendDlgItemMessage(hwndDlg, IDC_INFOPANELBG, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, FONTMODULE, "ipfieldsbg",  GetSysColor(COLOR_3DFACE)));
			SendDlgItemMessage(hwndDlg, IDC_INFOPANELBG, CPM_SETDEFAULTCOLOUR, 0, GetSysColor(COLOR_3DFACE));

			hInfoPanelBrush = CreateSolidBrush(SendDlgItemMessage(hwndDlg, IDC_INFOPANELBG, CPM_GETCOLOUR, 0, 0));

			SendDlgItemMessage(hwndDlg, IDC_GRIDLINES, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, FONTMODULE, "hgrid",  RGB(20, 20, 20)));
			SendDlgItemMessage(hwndDlg, IDC_GRIDLINES, CPM_SETDEFAULTCOLOUR, 0, RGB(20, 20, 20));

			SendDlgItemMessage(hwndDlg, IDC_BKGINCOMING, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, FONTMODULE, "inbg",  defwndcol));
			SendDlgItemMessage(hwndDlg, IDC_BKGINCOMING, CPM_SETDEFAULTCOLOUR, 0, defwndcol);

			SendDlgItemMessage(hwndDlg, IDC_BKGOUTGOING, CPM_SETCOLOUR, 0, DBGetContactSettingDword(NULL, FONTMODULE, "outbg",  defwndcol));
			SendDlgItemMessage(hwndDlg, IDC_BKGOUTGOING, CPM_SETDEFAULTCOLOUR, 0, defwndcol);

			SendDlgItemMessage(hwndDlg, IDC_FONTCATEGORY, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Message window fonts"));
			SendDlgItemMessage(hwndDlg, IDC_FONTCATEGORY, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Group chat fonts"));
			SendDlgItemMessage(hwndDlg, IDC_FONTCATEGORY, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Info panel and misc fonts"));

			SendDlgItemMessage(hwndDlg, IDC_NICKCOLORS, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Channel operators"));
			SendDlgItemMessage(hwndDlg, IDC_NICKCOLORS, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Half operators"));
			SendDlgItemMessage(hwndDlg, IDC_NICKCOLORS, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Voiced"));
			SendDlgItemMessage(hwndDlg, IDC_NICKCOLORS, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Extended mode 1"));
			SendDlgItemMessage(hwndDlg, IDC_NICKCOLORS, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Extended mode 2"));
			SendDlgItemMessage(hwndDlg, IDC_NICKCOLORS, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Selection background"));
			SendDlgItemMessage(hwndDlg, IDC_NICKCOLORS, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Selected text"));
			SendDlgItemMessage(hwndDlg, IDC_NICKCOLORS, CB_INSERTSTRING, -1, (LPARAM)TranslateT("Incremental search highlight"));

			SendDlgItemMessage(hwndDlg, IDC_NICKCOLOR, CPM_SETCOLOUR, 0, g_Settings.nickColors[0]);

			SendDlgItemMessage(hwndDlg, IDC_FONTCATEGORY, CB_SETCURSEL, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_NICKCOLORS, CB_SETCURSEL, 0, 0);

			fontOptionsList = IM_fontOptionsList;
			fontCount = MSGDLGFONTCOUNT;

			LoadFontsToList(hwndDlg);
			break;
		}
		case WM_MEASUREITEM: {
			MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *) lParam;
			HFONT hFont, hoFont;
			HDC hdc;
			SIZE fontSize;
			int iItem = mis->itemData - 1;

			hFont = CreateFont(fontOptionsList[iItem].size, 0, 0, 0,
							   fontOptionsList[iItem].style & FONTF_BOLD ? FW_BOLD : FW_NORMAL,
							   fontOptionsList[iItem].style & FONTF_ITALIC ? 1 : 0, 0, 0, fontOptionsList[iItem].charset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontOptionsList[iItem].szFace);
			hdc = GetDC(GetDlgItem(hwndDlg, mis->CtlID));
			hoFont = (HFONT) SelectObject(hdc, hFont);
			GetTextExtentPoint32(hdc, fontOptionsList[iItem].szDescr, lstrlen(fontOptionsList[iItem].szDescr), &fontSize);
			SelectObject(hdc, hoFont);
			ReleaseDC(GetDlgItem(hwndDlg, mis->CtlID), hdc);
			DeleteObject(hFont);
			mis->itemWidth = fontSize.cx;
			mis->itemHeight = fontSize.cy;
			return TRUE;
		}
		case WM_DRAWITEM: {
			DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT *) lParam;
			HFONT hFont, hoFont;
			BOOL fontDelete = FALSE;
			TCHAR* pszText;
			int iItem = dis->itemData - 1;

			if (iItem != 17 || fontOptionsList != CHAT_fontOptionsList) {
				fontDelete = TRUE;
				hFont = CreateFont(fontOptionsList[iItem].size, 0, 0, 0,
								   fontOptionsList[iItem].style & FONTF_BOLD ? FW_BOLD : FW_NORMAL,
								   fontOptionsList[iItem].style & FONTF_ITALIC ? 1 : 0, 0, 0, fontOptionsList[iItem].charset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontOptionsList[iItem].szFace);
			} else
				hFont = GetStockObject(DEFAULT_GUI_FONT);

			hoFont = (HFONT) SelectObject(dis->hDC, hFont);

			SetBkMode(dis->hDC, TRANSPARENT);
			if (fontOptionsList == CHAT_fontOptionsList && (iItem == 18 || iItem == 19))
				FillRect(dis->hDC, &dis->rcItem, hListColourBrush);
			else if (fontOptionsList == IM_fontOptionsList && iItem == MSGFONTID_MESSAGEAREA)
				FillRect(dis->hDC, &dis->rcItem, hMessageColourBrush);
			else if (fontOptionsList == IP_fontOptionsList)
				FillRect(dis->hDC, &dis->rcItem, hInfoPanelBrush);
			else
				FillRect(dis->hDC, &dis->rcItem, hBkgColourBrush);
			if (dis->itemState & ODS_SELECTED)
				FrameRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
			SetTextColor(dis->hDC, fontOptionsList[iItem].colour);
			pszText = TranslateTS(fontOptionsList[iItem].szDescr);
			TextOut(dis->hDC, dis->rcItem.left, dis->rcItem.top, pszText, lstrlen(pszText));
			SelectObject(dis->hDC, hoFont);
			if (fontDelete)
				DeleteObject(hFont);
			return TRUE;
		}

		case WM_CTLCOLORLISTBOX:
			if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_CHAT_FONTLIST)) {
				if (fontOptionsList == IP_fontOptionsList) {
					SetBkColor((HDC) wParam, SendDlgItemMessage(hwndDlg, IDC_INFOPANELBG, CPM_GETCOLOUR, 0, 0));
					return (BOOL) hInfoPanelBrush;
				} else {
					SetBkColor((HDC) wParam, SendDlgItemMessage(hwndDlg, IDC_LOGBKG, CPM_GETCOLOUR, 0, 0));
					return (BOOL) hListColourBrush;
				}
			}
			break;
		case WM_COMMAND:
			if (LOWORD(wParam) == IDC_FONTCATEGORY && HIWORD(wParam) == CBN_SELCHANGE) {
				int sel = SendDlgItemMessage(hwndDlg, IDC_FONTCATEGORY, CB_GETCURSEL, 0, 0);

				SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTLIST, LB_RESETCONTENT, 0, 0);
				switch (sel) {
					case 0:
						fontOptionsList = IM_fontOptionsList;
						fontCount = MSGDLGFONTCOUNT;
						break;
					case 1:
						fontOptionsList = CHAT_fontOptionsList;
						fontCount = msgDlgFontCount;
						break;
					case 2:
						fontOptionsList = IP_fontOptionsList;
						fontCount = IPFONTCOUNT;
						break;
				}
				LoadFontsToList(hwndDlg);
				break;
			} else if (LOWORD(wParam) == IDC_NICKCOLORS && HIWORD(wParam) == CBN_SELCHANGE) {
				int iSel = SendDlgItemMessage(hwndDlg, IDC_NICKCOLORS, CB_GETCURSEL, 0, 0);

				if (iSel >= 0 && iSel <= 7)
					SendDlgItemMessage(hwndDlg, IDC_NICKCOLOR, CPM_SETCOLOUR, 0, g_Settings.nickColors[iSel]);
				break;
			}
			if ((LOWORD(wParam)		  == IDC_INSTAMP
					|| LOWORD(wParam) == IDC_OUTSTAMP
					|| LOWORD(wParam) == IDC_TIMESTAMP
					|| LOWORD(wParam) == IDC_LOGLIMIT
					|| LOWORD(wParam) == IDC_NICKROW2
					|| LOWORD(wParam) == IDC_HIGHLIGHTWORDS
					|| LOWORD(wParam) == IDC_LOGTIMESTAMP)
					&& (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()))	return 0;

			switch (LOWORD(wParam)) {
				case IDC_NICKCOLOR: {
					int iSel = (int)SendDlgItemMessage(hwndDlg, IDC_NICKCOLORS, CB_GETCURSEL, 0, 0);
					if (iSel >= 0 && iSel <= 7)
						g_Settings.nickColors[iSel] = SendDlgItemMessage(hwndDlg, IDC_NICKCOLOR, CPM_GETCOLOUR, 0, 0);
					break;
				}
				case IDC_MESSAGEBKG:
					DeleteObject(hMessageColourBrush);
					hMessageColourBrush = CreateSolidBrush(SendDlgItemMessage(hwndDlg, IDC_MESSAGEBKG, CPM_GETCOLOUR, 0, 0));
					InvalidateRect(GetDlgItem(hwndDlg, IDC_CHAT_FONTLIST), NULL, TRUE);
					break;
				case IDC_NICKLISTBKG:
					DeleteObject(hListColourBrush);
					hListColourBrush = CreateSolidBrush(SendDlgItemMessage(hwndDlg, IDC_NICKLISTBKG, CPM_GETCOLOUR, 0, 0));
					InvalidateRect(GetDlgItem(hwndDlg, IDC_CHAT_FONTLIST), NULL, TRUE);
					break;
				case IDC_INFOPANELBG:
					DeleteObject(hInfoPanelBrush);
					hInfoPanelBrush = CreateSolidBrush(SendDlgItemMessage(hwndDlg, IDC_INFOPANELBG, CPM_GETCOLOUR, 0, 0));
					InvalidateRect(GetDlgItem(hwndDlg, IDC_CHAT_FONTLIST), NULL, TRUE);
					break;
				case IDC_LOGBKG:
					DeleteObject(hBkgColourBrush);
					hBkgColourBrush = CreateSolidBrush(SendDlgItemMessage(hwndDlg, IDC_LOGBKG, CPM_GETCOLOUR, 0, 0));
					InvalidateRect(GetDlgItem(hwndDlg, IDC_CHAT_FONTLIST), NULL, TRUE);
					break;
				case IDC_CHAT_FONTLIST:
					if (HIWORD(wParam) == LBN_SELCHANGE) {
						if (SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTLIST, LB_GETSELCOUNT, 0, 0) > 1) {
							SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTCOLOR, CPM_SETCOLOUR, 0, GetSysColor(COLOR_3DFACE));
							SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTCOLOR, CPM_SETDEFAULTCOLOUR, 0, GetSysColor(COLOR_WINDOWTEXT));
						} else {
							int i = SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTLIST, LB_GETITEMDATA,
													   SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTLIST, LB_GETCURSEL, 0, 0), 0) - 1;
							SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTCOLOR, CPM_SETCOLOUR, 0, fontOptionsList[i].colour);
							SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTCOLOR, CPM_SETDEFAULTCOLOUR, 0, fontOptionsList[i].defColour);
						}
					}
					if (HIWORD(wParam) != LBN_DBLCLK)
						return TRUE;
					//fall through
				case IDC_CHAT_CHOOSEFONT: {
					CHOOSEFONT cf = { 0 };
					LOGFONT lf = { 0 };
					int i = SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTLIST, LB_GETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTLIST, LB_GETCURSEL, 0, 0), 0) - 1;
					lf.lfHeight = fontOptionsList[i].size;
					lf.lfWeight = fontOptionsList[i].style & FONTF_BOLD ? FW_BOLD : FW_NORMAL;
					lf.lfItalic = fontOptionsList[i].style & FONTF_ITALIC ? 1 : 0;
					lf.lfCharSet = fontOptionsList[i].charset;
					lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
					lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
					lf.lfQuality = DEFAULT_QUALITY;
					lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
					lstrcpy(lf.lfFaceName, fontOptionsList[i].szFace);
					cf.lStructSize = sizeof(cf);
					cf.hwndOwner = hwndDlg;
					cf.lpLogFont = &lf;
					cf.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
					if (ChooseFont(&cf)) {
						int selItems[sizeof(IM_fontOptionsList) / sizeof(IM_fontOptionsList[0])];
						int sel, selCount;

						selCount = SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTLIST, LB_GETSELITEMS, MSGDLGFONTCOUNT, (LPARAM) selItems);
						for (sel = 0; sel < selCount; sel++) {
							i = SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTLIST, LB_GETITEMDATA, selItems[sel], 0) - 1;
							fontOptionsList[i].size = (char) lf.lfHeight;
							fontOptionsList[i].style = (lf.lfWeight >= FW_BOLD ? FONTF_BOLD : 0) | (lf.lfItalic ? FONTF_ITALIC : 0);
							fontOptionsList[i].charset = lf.lfCharSet;
							lstrcpy(fontOptionsList[i].szFace, lf.lfFaceName);
							{
								MEASUREITEMSTRUCT mis = { 0 };
								mis.CtlID = IDC_CHAT_FONTLIST;
								mis.itemData = i + 1;
								SendMessage(hwndDlg, WM_MEASUREITEM, 0, (LPARAM) & mis);
								SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTLIST, LB_SETITEMHEIGHT, selItems[sel], mis.itemHeight);
							}
						}
						InvalidateRect(GetDlgItem(hwndDlg, IDC_CHAT_FONTLIST), NULL, TRUE);
						break;
					}
					return TRUE;
				}
				case IDC_CHAT_FONTCOLOR: {
					int selItems[sizeof(IM_fontOptionsList) / sizeof(IM_fontOptionsList[0])];
					int sel, selCount, i;

					selCount = SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTLIST, LB_GETSELITEMS, MSGDLGFONTCOUNT, (LPARAM) selItems);
					for (sel = 0; sel < selCount; sel++) {
						i = SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTLIST, LB_GETITEMDATA, selItems[sel], 0) - 1;
						fontOptionsList[i].colour = SendDlgItemMessage(hwndDlg, IDC_CHAT_FONTCOLOR, CPM_GETCOLOUR, 0, 0);
					}
					InvalidateRect(GetDlgItem(hwndDlg, IDC_CHAT_FONTLIST), NULL, FALSE);
					break;
				}
				case IDC_HIGHLIGHT:
					EnableWindow(GetDlgItem(hwndDlg, IDC_HIGHLIGHTWORDS), IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHT) == BST_CHECKED ? TRUE : FALSE);
					break;
			}

			if (lParam != (LPARAM)NULL)
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;

		case WM_NOTIFY:
			if (((LPNMHDR)lParam)->idFrom == 0 && ((LPNMHDR)lParam)->code == PSN_APPLY) {
				int iLen;
				TCHAR *p2 = NULL;
				char  *pszText = NULL;

				if (g_chat_integration_enabled) {
					iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_HIGHLIGHTWORDS));
					if (iLen > 0) {
						TCHAR *ptszText = malloc((iLen + 2) * sizeof(TCHAR));
						if (ptszText) {
							GetDlgItemText(hwndDlg, IDC_HIGHLIGHTWORDS, ptszText, iLen + 1);
							p2 = _tcschr(ptszText, (TCHAR)',');
							while (p2) {
								*p2 = (TCHAR)' ';
								p2 = _tcschr(ptszText, ',');
							}
							DBWriteContactSettingTString(NULL, "Chat", "HighlightWords", ptszText);
							free(ptszText);
						}
					} else
						DBDeleteContactSetting(NULL, "Chat", "HighlightWords");

					iLen = SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN3, UDM_GETPOS, 0, 0);
					if (iLen > 0)
						DBWriteContactSettingByte(NULL, "Chat", "NicklistRowDist", (BYTE)iLen);
					else
						DBDeleteContactSetting(NULL, "Chat", "NicklistRowDist");

					iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_LOGTIMESTAMP));
					if (iLen > 0) {
						pszText = realloc(pszText, iLen + 1);
						GetDlgItemTextA(hwndDlg, IDC_LOGTIMESTAMP, pszText, iLen + 1);
						DBWriteContactSettingString(NULL, "Chat", "LogTimestamp", pszText);
					} else DBDeleteContactSetting(NULL, "Chat", "LogTimestamp");

					iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_TIMESTAMP));
					if (iLen > 0) {
						pszText = realloc(pszText, iLen + 1);
						GetDlgItemTextA(hwndDlg, IDC_TIMESTAMP, pszText, iLen + 1);
						DBWriteContactSettingString(NULL, "Chat", "HeaderTime", pszText);
					} else DBDeleteContactSetting(NULL, "Chat", "HeaderTime");

					iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_INSTAMP));
					if (iLen > 0) {
						pszText = realloc(pszText, iLen + 1);
						GetDlgItemTextA(hwndDlg, IDC_INSTAMP, pszText, iLen + 1);
						DBWriteContactSettingString(NULL, "Chat", "HeaderIncoming", pszText);
					} else DBDeleteContactSetting(NULL, "Chat", "HeaderIncoming");

					iLen = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_OUTSTAMP));
					if (iLen > 0) {
						pszText = realloc(pszText, iLen + 1);
						GetDlgItemTextA(hwndDlg, IDC_OUTSTAMP, pszText, iLen + 1);
						DBWriteContactSettingString(NULL, "Chat", "HeaderOutgoing", pszText);
					} else DBDeleteContactSetting(NULL, "Chat", "HeaderOutgoing");

					g_Settings.HighlightEnabled = IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHT) == BST_CHECKED ? TRUE : FALSE;
					DBWriteContactSettingByte(NULL, "Chat", "HighlightEnabled", (BYTE)g_Settings.HighlightEnabled);

					iLen = SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN2, UDM_GETPOS, 0, 0);
					DBWriteContactSettingWord(NULL, "Chat", "LogLimit", (WORD)iLen);
				}


				DBWriteContactSettingDword(NULL, "Chat", "ColorLogBG", (DWORD)SendDlgItemMessage(hwndDlg, IDC_LOGBKG, CPM_GETCOLOUR, 0, 0));
				DBWriteContactSettingDword(NULL, "Chat", "ColorNicklistBG", (DWORD)SendDlgItemMessage(hwndDlg, IDC_NICKLISTBKG, CPM_GETCOLOUR, 0, 0));

				DBWriteContactSettingDword(NULL, FONTMODULE, SRMSGSET_BKGCOLOUR, (DWORD)SendDlgItemMessage(hwndDlg, IDC_LOGBKG, CPM_GETCOLOUR, 0, 0));
				DBWriteContactSettingDword(NULL, FONTMODULE, "inputbg", (DWORD)SendDlgItemMessage(hwndDlg, IDC_MESSAGEBKG, CPM_GETCOLOUR, 0, 0));

				DBWriteContactSettingDword(NULL, FONTMODULE, "hgrid", (DWORD)SendDlgItemMessage(hwndDlg, IDC_GRIDLINES, CPM_GETCOLOUR, 0, 0));
				DBWriteContactSettingDword(NULL, FONTMODULE, "inbg", (DWORD)SendDlgItemMessage(hwndDlg, IDC_BKGINCOMING, CPM_GETCOLOUR, 0, 0));
				DBWriteContactSettingDword(NULL, FONTMODULE, "outbg", (DWORD)SendDlgItemMessage(hwndDlg, IDC_BKGOUTGOING, CPM_GETCOLOUR, 0, 0));
				DBWriteContactSettingDword(NULL, FONTMODULE, "ipfieldsbg", (DWORD)SendDlgItemMessage(hwndDlg, IDC_INFOPANELBG, CPM_GETCOLOUR, 0, 0));

				if (pszText != NULL)
					free(pszText);
				if (hListBkgBrush)
					DeleteObject(hListBkgBrush);
				hListBkgBrush = CreateSolidBrush(DBGetContactSettingDword(NULL, "Chat", "ColorNicklistBG", GetSysColor(COLOR_WINDOW)));

				{
					int i, j;
					char str[32];
					char *szMod = fontOptionsList == CHAT_fontOptionsList ? CHAT_FONTMODULE : FONTMODULE;

					for (j = 0; j < fontCount; j++) {
						i = (fontOptionsList == IP_fontOptionsList ? 100 + j : j);
						wsprintfA(str, "Font%d", i);
						if (fontOptionsList == CHAT_fontOptionsList)
							DBWriteContactSettingTString(NULL, szMod, str, fontOptionsList[j].szFace);
						else {
#if defined(_UNICODE)
							char szFacename[LF_FACESIZE + 2];
							WideCharToMultiByte(CP_ACP, 0, fontOptionsList[j].szFace, -1, szFacename, LF_FACESIZE, 0, 0);
							DBWriteContactSettingString(NULL, szMod, str, szFacename);
#else
							DBWriteContactSettingString(NULL, szMod, str, fontOptionsList[j].szFace);
#endif
						}
						wsprintfA(str, "Font%dSize", i);
						DBWriteContactSettingByte(NULL, szMod, str, fontOptionsList[j].size);
						wsprintfA(str, "Font%dSty", i);
						DBWriteContactSettingByte(NULL, szMod, str, fontOptionsList[j].style);
						wsprintfA(str, "Font%dSet", i);
						DBWriteContactSettingByte(NULL, szMod, str, fontOptionsList[j].charset);
						wsprintfA(str, "Font%dCol", i);
						DBWriteContactSettingDword(NULL, szMod, str, fontOptionsList[j].colour);
					}
				}

				if (g_chat_integration_enabled) {
					LOGFONT lf;
					HFONT hFont;
					int iText;
					int i;
					char szBuf[40];

					LoadLogFonts();
					FreeMsgLogBitmaps();
					LoadMsgLogBitmaps();

					for (i = 0; i <= 7; i++) {
						mir_snprintf(szBuf, 20, "NickColor%d", i);
						DBWriteContactSettingDword(NULL, "Chat", szBuf, g_Settings.nickColors[i]);
					}

					LoadMsgDlgFont(0, &lf, NULL, CHAT_FONTMODULE);
					hFont = CreateFontIndirect(&lf);
					iText = GetTextPixelSize(MakeTimeStamp(g_Settings.pszTimeStamp, time(NULL)), hFont, TRUE);
					DeleteObject(hFont);
					g_Settings.LogTextIndent = iText;
					g_Settings.LogTextIndent = g_Settings.LogTextIndent * 12 / 10;
					g_Settings.LogIndentEnabled = (DBGetContactSettingByte(NULL, "Chat", "LogIndentEnabled", 1) != 0) ? TRUE : FALSE;

					LoadGlobalSettings();
					MM_FontsChanged();
					MM_FixColors();
					SM_BroadcastMessage(NULL, GC_SETWNDPROPS, 0, 0, TRUE);
				}

				ReloadGlobals();
				CacheMsgLogIcons();
				CacheLogFonts();
				WindowList_Broadcast(hMessageWindowList, DM_OPTIONSAPPLIED, 1, 0);
				return TRUE;
			}
			break;

		case WM_DESTROY:
			DeleteObject(hBkgColourBrush);
			DeleteObject(hListColourBrush);
			DeleteObject(hMessageColourBrush);
			DeleteObject(hInfoPanelBrush);
			break;
	}
	return FALSE;
}

static BOOL CALLBACK DlgProcOptionsPopup(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);

			SendDlgItemMessage(hwndDlg, IDC_BKG, CPM_SETCOLOUR, 0, g_Settings.crPUBkgColour);
			SendDlgItemMessage(hwndDlg, IDC_TEXT, CPM_SETCOLOUR, 0, g_Settings.crPUTextColour);

			if (g_Settings.iPopupStyle == 2)
				CheckDlgButton(hwndDlg, IDC_RADIO2, BST_CHECKED);
			else if (g_Settings.iPopupStyle == 3)
				CheckDlgButton(hwndDlg, IDC_RADIO3, BST_CHECKED);
			else
				CheckDlgButton(hwndDlg, IDC_CHAT_RADIO1, BST_CHECKED);

			EnableWindow(GetDlgItem(hwndDlg, IDC_BKG), IsDlgButtonChecked(hwndDlg, IDC_RADIO3) == BST_CHECKED ? TRUE : FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT), IsDlgButtonChecked(hwndDlg, IDC_RADIO3) == BST_CHECKED ? TRUE : FALSE);

			SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN1, UDM_SETRANGE, 0, MAKELONG(100, -1));
			SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN1, UDM_SETPOS, 0, MAKELONG(g_Settings.iPopupTimeout, 0));
			break;

		case WM_COMMAND:
			if ((LOWORD(wParam)		  == IDC_TIMEOUT)
					&& (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()))	return 0;

			if (lParam != (LPARAM)NULL)
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			switch (LOWORD(wParam)) {

				case IDC_CHAT_RADIO1:
				case IDC_RADIO2:
				case IDC_RADIO3:
					EnableWindow(GetDlgItem(hwndDlg, IDC_BKG), IsDlgButtonChecked(hwndDlg, IDC_RADIO3) == BST_CHECKED ? TRUE : FALSE);
					EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT), IsDlgButtonChecked(hwndDlg, IDC_RADIO3) == BST_CHECKED ? TRUE : FALSE);
					break;
			}
			break;

		case WM_NOTIFY: {
			switch (((LPNMHDR)lParam)->idFrom) {
				case 0:
					switch (((LPNMHDR)lParam)->code) {
						case PSN_APPLY: {
							int iLen;

							if (IsDlgButtonChecked(hwndDlg, IDC_RADIO2) == BST_CHECKED)
								iLen = 2;
							else if (IsDlgButtonChecked(hwndDlg, IDC_RADIO3) == BST_CHECKED)
								iLen = 3;
							else
								iLen = 1;

							g_Settings.iPopupStyle = iLen;
							DBWriteContactSettingByte(NULL, "Chat", "PopupStyle", (BYTE)iLen);

							iLen = SendDlgItemMessage(hwndDlg, IDC_CHAT_SPIN1, UDM_GETPOS, 0, 0);
							g_Settings.iPopupTimeout = iLen;
							DBWriteContactSettingWord(NULL, "Chat", "PopupTimeout", (WORD)iLen);

							g_Settings.crPUBkgColour = SendDlgItemMessage(hwndDlg, IDC_BKG, CPM_GETCOLOUR, 0, 0);
							DBWriteContactSettingDword(NULL, "Chat", "PopupColorBG", (DWORD)SendDlgItemMessage(hwndDlg, IDC_BKG, CPM_GETCOLOUR, 0, 0));
							g_Settings.crPUTextColour = SendDlgItemMessage(hwndDlg, IDC_TEXT, CPM_GETCOLOUR, 0, 0);
							DBWriteContactSettingDword(NULL, "Chat", "PopupColorText", (DWORD)SendDlgItemMessage(hwndDlg, IDC_TEXT, CPM_GETCOLOUR, 0, 0));

						}
						return TRUE;
					}
			}
		}
		break;

		default:
			break;
	}
	return FALSE;
}

int Chat_OptionsInitialize(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = {0};

	if (!g_chat_integration_enabled)
		return 0;

	if (myGlobals.g_PopupAvail) {
		odp.cbSize = sizeof(odp);
		odp.position = 910000002;
		odp.hInstance = g_hInst;
		odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPTIONSPOPUP);
		odp.pszTitle = "Chat";
		odp.pszGroup = "Popups";
		odp.pfnDlgProc = DlgProcOptionsPopup;
		odp.flags = ODPF_BOLDGROUPS;
		CallService(MS_OPT_ADDPAGE, wParam, (LPARAM)&odp);
	}
	return 0;
}

void LoadGlobalSettings(void)
{
	LOGFONT lf;
	int i;
	char szBuf[40];

	g_Settings.LogLimitNames = DBGetContactSettingByte(NULL, "Chat", "LogLimitNames", 1);
	g_Settings.ShowTime = DBGetContactSettingByte(NULL, "Chat", "ShowTimeStamp", 1);
	g_Settings.SoundsFocus = DBGetContactSettingByte(NULL, "Chat", "SoundsFocus", 0);
	g_Settings.ShowTimeIfChanged = (BOOL)DBGetContactSettingByte(NULL, "Chat", "ShowTimeStampIfChanged", 0);
	g_Settings.TimeStampEventColour = (BOOL)DBGetContactSettingByte(NULL, "Chat", "TimeStampEventColour", 0);
	g_Settings.iEventLimit = DBGetContactSettingWord(NULL, "Chat", "LogLimit", 100);
	g_Settings.dwIconFlags = DBGetContactSettingDword(NULL, "Chat", "IconFlags", 0x0000);
	g_Settings.LoggingLimit = DBGetContactSettingWord(NULL, "Chat", "LoggingLimit", 100);
	g_Settings.LoggingEnabled = (BOOL)DBGetContactSettingByte(NULL, "Chat", "LoggingEnabled", 0);
	g_Settings.OpenInDefault = (BOOL)DBGetContactSettingByte(NULL, "Chat", "DefaultContainer", 0);
	g_Settings.FlashWindow = (BOOL)DBGetContactSettingByte(NULL, "Chat", "FlashWindow", 0);
	g_Settings.FlashWindowHightlight = (BOOL)DBGetContactSettingByte(NULL, "Chat", "FlashWindowHighlight", 0);
	g_Settings.HighlightEnabled = (BOOL)DBGetContactSettingByte(NULL, "Chat", "HighlightEnabled", 1);
	g_Settings.crUserListColor = (BOOL)DBGetContactSettingDword(NULL, CHAT_FONTMODULE, "Font18Col", RGB(0, 0, 0));
	g_Settings.crUserListBGColor = (BOOL)DBGetContactSettingDword(NULL, "Chat", "ColorNicklistBG", GetSysColor(COLOR_WINDOW));
	g_Settings.crUserListHeadingsColor = (BOOL)DBGetContactSettingDword(NULL, CHAT_FONTMODULE, "Font19Col", RGB(170, 170, 170));
	g_Settings.crLogBackground = (BOOL)DBGetContactSettingDword(NULL, "Chat", "ColorLogBG", GetSysColor(COLOR_WINDOW));
	g_Settings.StripFormat = (BOOL)DBGetContactSettingByte(NULL, "Chat", "StripFormatting", 0);
	g_Settings.TrayIconInactiveOnly = (BOOL)DBGetContactSettingByte(NULL, "Chat", "TrayIconInactiveOnly", 1);
	g_Settings.SkipWhenNoWindow = (BOOL)DBGetContactSettingByte(NULL, "Chat", "SkipWhenNoWindow", 0);
	g_Settings.AddColonToAutoComplete = (BOOL)DBGetContactSettingByte(NULL, "Chat", "AddColonToAutoComplete", 1);
	g_Settings.iPopupStyle = DBGetContactSettingByte(NULL, "Chat", "PopupStyle", 1);
	g_Settings.iPopupTimeout = DBGetContactSettingWord(NULL, "Chat", "PopupTimeout", 3);
	g_Settings.crPUBkgColour = DBGetContactSettingDword(NULL, "Chat", "PopupColorBG", GetSysColor(COLOR_WINDOW));
	g_Settings.crPUTextColour = DBGetContactSettingDword(NULL, "Chat", "PopupColorText", 0);
	g_Settings.ClassicIndicators = DBGetContactSettingByte(NULL, "Chat", "ClassicIndicators", 1);
	g_Settings.LogSymbols = DBGetContactSettingByte(NULL, "Chat", "LogSymbols", 0);
	g_Settings.ClickableNicks = DBGetContactSettingByte(NULL, "Chat", "ClickableNicks", 0);
	g_Settings.ColorizeNicks = DBGetContactSettingByte(NULL, "Chat", "ColorizeNicks", 0);
	g_Settings.ScaleIcons = DBGetContactSettingByte(NULL, "Chat", "ScaleIcons", 1);
	g_Settings.UseDividers = DBGetContactSettingByte(NULL, "Chat", "UseDividers", 1);
	g_Settings.DividersUsePopupConfig = DBGetContactSettingByte(NULL, "Chat", "DividersUsePopupConfig", 1);
	g_Settings.MathMod = ServiceExists(MATH_RTF_REPLACE_FORMULAE) && DBGetContactSettingByte(NULL, "Chat", "MathModSupport", 0);

	g_Settings.DoubleClick4Privat = (BOOL)DBGetContactSettingByte(NULL, "Chat", "DoubleClick4Privat", 1);
	g_Settings.ShowContactStatus = DBGetContactSettingByte(NULL, "Chat", "ShowContactStatus", 1);
	g_Settings.ContactStatusFirst = DBGetContactSettingByte(NULL, "Chat", "ContactStatusFirst", 0);


	if (hListBkgBrush)
		DeleteObject(hListBkgBrush);
	hListBkgBrush = CreateSolidBrush(DBGetContactSettingDword(NULL, "Chat", "ColorNicklistBG", GetSysColor(COLOR_WINDOW)));

	InitSetting(&g_Settings.pszTimeStamp, "HeaderTime", _T("[%H:%M]"));
	InitSetting(&g_Settings.pszTimeStampLog, "LogTimestamp", _T("[%d %b %y %H:%M]"));
	InitSetting(&g_Settings.pszIncomingNick, "HeaderIncoming", _T("%n:"));
	InitSetting(&g_Settings.pszOutgoingNick, "HeaderOutgoing", _T("%n:"));
	InitSetting(&g_Settings.pszHighlightWords, "HighlightWords", _T("%m"));

	{
		char pszTemp[MAX_PATH];
		DBVARIANT dbv;
		g_Settings.pszLogDir = (char *)mir_realloc(g_Settings.pszLogDir, MAX_PATH);
		if (!DBGetContactSettingString(NULL, "Chat", "LogDirectory", &dbv)) {
			lstrcpynA(pszTemp, dbv.pszVal, MAX_PATH);
			DBFreeVariant(&dbv);
		} else lstrcpynA(pszTemp, "Logs\\", MAX_PATH);

		CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)pszTemp, (LPARAM)g_Settings.pszLogDir);
	}

	g_Settings.LogIndentEnabled = (DBGetContactSettingByte(NULL, "Chat", "LogIndentEnabled", 1) != 0) ? TRUE : FALSE;

	if (g_Settings.UserListFont)
		DeleteObject(g_Settings.UserListFont);
	LoadMsgDlgFont(18, &lf, NULL, CHAT_FONTMODULE);
	g_Settings.UserListFont = CreateFontIndirect(&lf);

	if (g_Settings.UserListHeadingsFont)
		DeleteObject(g_Settings.UserListHeadingsFont);
	LoadMsgDlgFont(19, &lf, NULL, CHAT_FONTMODULE);
	g_Settings.UserListHeadingsFont = CreateFontIndirect(&lf);

	for (i = 0; i < 7; i++) {
		mir_snprintf(szBuf, 20, "NickColor%d", i);
		g_Settings.nickColors[i] = DBGetContactSettingDword(NULL, "Chat", szBuf, g_Settings.crUserListColor);
	}
	g_Settings.nickColors[5] = DBGetContactSettingDword(NULL, "Chat", "NickColor5", GetSysColor(COLOR_HIGHLIGHT));
	g_Settings.nickColors[6] = DBGetContactSettingDword(NULL, "Chat", "NickColor6", GetSysColor(COLOR_HIGHLIGHTTEXT));
	if (g_Settings.SelectionBGBrush)
		DeleteObject(g_Settings.SelectionBGBrush);
	g_Settings.SelectionBGBrush = CreateSolidBrush(g_Settings.nickColors[5]);
}

static void FreeGlobalSettings(void)
{
	mir_free(g_Settings.pszTimeStamp);
	mir_free(g_Settings.pszTimeStampLog);
	mir_free(g_Settings.pszIncomingNick);
	mir_free(g_Settings.pszOutgoingNick);
	mir_free(g_Settings.pszHighlightWords);
	mir_free(g_Settings.pszLogDir);
	if (g_Settings.UserListFont)
		DeleteObject(g_Settings.UserListFont);
	if (g_Settings.UserListHeadingsFont)
		DeleteObject(g_Settings.UserListHeadingsFont);
}

int OptionsInit(void)
{
	LOGFONT lf;

	//g_hOptions = HookEvent(ME_OPT_INITIALISE, OptionsInitialize);

	LoadLogFonts();
	LoadMsgDlgFont(17, &lf, NULL, CHAT_FONTMODULE);
	lstrcpy(lf.lfFaceName, _T("MS Shell Dlg"));
	lf.lfUnderline = lf.lfItalic = lf.lfStrikeOut = 0;
	lf.lfHeight = -17;
	lf.lfWeight = FW_BOLD;
	ZeroMemory(&g_Settings, sizeof(struct GlobalLogSettings_t));
	g_Settings.NameFont = CreateFontIndirect(&lf);
	g_Settings.UserListFont = NULL;
	g_Settings.UserListHeadingsFont = NULL;
	g_Settings.iSplitterX = DBGetContactSettingWord(NULL, "Chat", "SplitterX", 105);
	g_Settings.iSplitterY = DBGetContactSettingWord(NULL, "Chat", "splitY", 50);
	g_Settings.iX = DBGetContactSettingDword(NULL, "Chat", "roomx", -1);
	g_Settings.iY = DBGetContactSettingDword(NULL, "Chat", "roomy", -1);
	g_Settings.iWidth = DBGetContactSettingDword(NULL, "Chat", "roomwidth", -1);
	g_Settings.iHeight = DBGetContactSettingDword(NULL, "Chat", "roomheight", -1);
	LoadGlobalSettings();
	SkinAddNewSoundEx("ChatMessage", "Chat", Translate("Incoming message"));
	SkinAddNewSoundEx("ChatHighlight", "Chat", Translate("Message is highlighted"));
	SkinAddNewSoundEx("ChatAction", "Chat", Translate("User has performed an action"));
	SkinAddNewSoundEx("ChatJoin", "Chat", Translate("User has joined"));
	SkinAddNewSoundEx("ChatPart", "Chat", Translate("User has left"));
	SkinAddNewSoundEx("ChatKick", "Chat", Translate("User has kicked some other user"));
	SkinAddNewSoundEx("ChatMode", "Chat", Translate("User´s status was changed"));
	SkinAddNewSoundEx("ChatNick", "Chat", Translate("User has changed name"));
	SkinAddNewSoundEx("ChatNotice", "Chat", Translate("User has sent a notice"));
	SkinAddNewSoundEx("ChatQuit", "Chat", Translate("User has disconnected"));
	SkinAddNewSoundEx("ChatTopic", "Chat", Translate("The topic has been changed"));

	if (g_Settings.LoggingEnabled)
		if (!PathIsDirectoryA(g_Settings.pszLogDir))
			CreateDirectoryA(g_Settings.pszLogDir, NULL);
	{
		LOGFONT lf;
		HFONT hFont;
		int iText;

		LoadMsgDlgFont(0, &lf, NULL, CHAT_FONTMODULE);
		hFont = CreateFontIndirect(&lf);
		iText = GetTextPixelSize(MakeTimeStamp(g_Settings.pszTimeStamp, time(NULL)), hFont, TRUE);
		DeleteObject(hFont);
		g_Settings.LogTextIndent = iText;
		g_Settings.LogTextIndent = g_Settings.LogTextIndent * 12 / 10;
	}

	return 0;
}


int OptionsUnInit(void)
{
	FreeGlobalSettings();
	UnhookEvent(g_hOptions);
	DeleteObject(hListBkgBrush);
	DeleteObject(g_Settings.NameFont);
	return 0;
}
